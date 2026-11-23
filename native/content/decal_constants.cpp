#include "decal_constants.h"
#include <array>
#include <cmath>
#include <cstring>
#include <vector>

// Transcription of FUN_140198910 (the per-decal draw of the deferred box decals), constants only. Decompile + disassembly
// read 2026-10-18; the structure of the function:
//   1. gating and the pixel/vertex programme selection (iVar36 / iVar37, tables 14473C860 / 14473C7F0);
//   2. the decal's box as rows  W = [hx*A0; hy*A1; hz*A2; P]  (hx.. = +0x140 half extents, A0..A2 = +0x150/+0x160/+0x170 axes,
//      P = +0x180), masked-lane sums with 143D4A6E0 / 143D1F6D0 / 143D1F6E0 (lane x / y / z) and 143D1C080 (lane w);
//   3. the camera-relative clip transform  C = [rows of W with P - camera] * inverse(R_cam) * Proj  -> VS b10;
//   4. the clip -> decal-space matrix  Z = inverse(C or its unit-axis twin) * Q,  Q = T_apex * S * Pm * S * F * K -> PS b10[0..3];
//   5. the remaining PS b10 rows (inverse projection, camera, screen size, decal rotation, scales, fades, normal threshold).
// All matrices are row-major with row vectors (v' = v * M); the buffers hold the transposed matrices.
namespace ds3content {
namespace {
using M4 = float[16];

uint32_t Bits(float f) { uint32_t u; std::memcpy(&u, &f, 4); return u; }
float FromBits(uint32_t u) { float f; std::memcpy(&f, &u, 4); return f; }

// .rdata constants (value, RVA of the original). Dumped from the game image.
constexpr uint32_t kOneBits = 0x3f800000;           // 143D7EE10: 1.0f
constexpr float kHalf = 0.5f;                       // 143D7EDB4: 0.5f
constexpr float kTwo = 2.0f;                        // 143D7EE7C: 2.0f
constexpr float kNegHalf = -0.5f;                   // 143D7F0C0: -0.5f
constexpr float kParallaxEpsilon = 9.99999975e-5f;  // 143D7ED00: 0x38D1B717 (bump decal "has parallax" test)
constexpr float kTinyRange = 1.0e-5f;               // 143D7ECF4: 0x3727C5AC (fade range floor)
constexpr float kInfiniteRecip = 100000.0f;         // 143D7F07C: 0x47C35000 (normal-fade reciprocal range when the range is empty)
constexpr float kPi = 3.14159274f;                  // 143D7EEC0: 0x40490FDB
constexpr float kTwoPi = 6.28318548f;               // 143D7EEFC: 0x40C90FDB
// 1439F2DA0 / 1439F2DB0 / 1439F2DC0: the cosine Taylor coefficients (1, -1/2!, 1/4!, -1/6!, 1/8!, ... -1/22!) used by 141778230.
constexpr uint32_t kCosC0[4] = {0x3f800000, 0xbf000000, 0x3d2aaaab, 0xbab60b61};
constexpr uint32_t kCosC1[4] = {0x37d00d01, 0xb493f27e, 0x310f76c8, 0xad49cba5};   // I-0120 C7: 1439F2DBC is 0xAD49CBA5 (model_common_textures.cpp)
constexpr uint32_t kCosC2[4] = {0x29573f9f, 0xa53413c3, 0x20f2a15d, 0x9c8671cb};

float Rf(const uint8_t* b, unsigned off) { float v; std::memcpy(&v, b + off, 4); return v; }
int32_t Ri(const uint8_t* b, unsigned off) { int32_t v; std::memcpy(&v, b + off, 4); return v; }
uint64_t Ru64(const uint8_t* b, unsigned off) { uint64_t v; std::memcpy(&v, b + off, 8); return v; }

void Copy16(float* dst, const float* src) { std::memcpy(dst, src, 64); }
void Identity(float* m) { static const float i[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1}; Copy16(m, i); }

// Constant rows used by the matrix constructions (values at the RVA in the comment).
const float kRowX[4] = {1, 0, 0, 0};    // 143D7F1A0 / 143D1BF90 / 14469C9E0
const float kRowY[4] = {0, 1, 0, 0};    // 143D7F370 / 143D1BFA0
const float kRowZ[4] = {0, 0, 1, 0};    // 143D7F690 / 143D1BFB0
const float kRowW[4] = {0, 0, 0, 1};    // 143D7FB40 / 1439F2500

// Tangent as the game computes it (141FD59E0 works in double).
float TanF(float x) { return static_cast<float>(std::tan(static_cast<double>(x))); }

// Row of a unit-cube-to-texture matrix K (c38 in the decompile).
void MakeK(int kind, float* k) {   // 0 = box (143D7F180, 143D7F2E0, 143D7F610, 143D7FD80), 1 = perspective (.., 143D7F370, .., 143D7FD60), 2 = identity
    if (kind == 2) { Identity(k); return; }
    const float r0[4] = {0.5f, 0, 0, 0};                                   // 143D7F180
    const float r1[4] = {0, kind == 0 ? 0.5f : 1.0f, 0, 0};                // 143D7F2E0 / 143D7F370
    const float r2[4] = {0, 0, 0.5f, 0};                                   // 143D7F610
    const float r3[4] = {0.5f, kind == 0 ? 0.5f : 0.0f, 0.5f, 1.0f};       // 143D7FD80 / 143D7FD60
    std::memcpy(k, r0, 16); std::memcpy(k + 4, r1, 16); std::memcpy(k + 8, r2, 16); std::memcpy(k + 12, r3, 16);
}

// 14006EF80: the eight corners of the view frustum from (fov, near, far, aspect): far (+,+) (-,+) (+,-) (-,-), near the same.
void FrustumCorners(float fov, float near_plane, float far_plane, float aspect, float c[8][4]) {
    const float t = TanF(fov * kHalf);
    float fVar14 = t * aspect;
    float fVar16 = t * far_plane;
    float fVar11 = t * near_plane;
    const float fVar15 = fVar14 * far_plane;
    fVar14 = fVar14 * near_plane;
    const float fx[2] = {fVar15, -fVar15}, fy[2] = {fVar16, -fVar16}, nx[2] = {fVar14, -fVar14}, ny[2] = {fVar11, -fVar11};
    for (int i = 0; i < 4; ++i) {
        c[i][0] = fx[i & 1]; c[i][1] = fy[i >> 1]; c[i][2] = far_plane; c[i][3] = 0;
        c[4 + i][0] = nx[i & 1]; c[4 + i][1] = ny[i >> 1]; c[4 + i][2] = near_plane; c[4 + i][3] = 0;
    }
}

// 1417B66B0: sphere (x, y, z, radius) against the oriented box [half; A0; A1; A2; centre] (80 bytes, w lanes ignored).
bool SphereVsBox(const float s[4], const float box[20]) {
    const float dx = s[0] - box[16], dy = s[1] - box[17], dz = s[2] - box[18], dw = 0.0f;   // w lane: (1.0 - centre.w) & 0
    const float f12 = dy * box[5] + dw * box[7] + dx * box[4] + dz * box[6];
    const float f4 = box[9] * dy + box[11] * dw + box[8] * dx + box[10] * dz;
    const float f5 = box[13] * dy + box[15] * dw + box[12] * dx + box[14] * dz;
    const auto excess = [](float f, float e) { return e < f ? f - e : (f < 0.0f - e ? f - (0.0f - e) : 0.0f); };
    const float e0 = excess(f12, box[0]), e1 = excess(f4, box[1]), e2 = excess(f5, box[2]);
    return e0 * e0 + e1 * e1 + e2 * e2 <= s[3] * s[3];
}

// Convex clipping of a polygon against half-spaces n.p + d <= 0 (Sutherland-Hodgman); true when something remains.
struct Plane { double n[3]; double d; };
bool PolygonTouches(std::vector<std::array<double, 3>> poly, const std::vector<Plane>& planes) {
    for (const Plane& pl : planes) {
        if (poly.empty()) return false;
        std::vector<std::array<double, 3>> next;
        for (size_t i = 0; i < poly.size(); ++i) {
            const auto& a = poly[i]; const auto& b = poly[(i + 1) % poly.size()];
            const double da = pl.n[0] * a[0] + pl.n[1] * a[1] + pl.n[2] * a[2] + pl.d;
            const double db = pl.n[0] * b[0] + pl.n[1] * b[1] + pl.n[2] * b[2] + pl.d;
            if (da <= 0.0) next.push_back(a);
            if ((da < 0.0 && db > 0.0) || (da > 0.0 && db < 0.0)) {
                const double t = da / (da - db);
                next.push_back({a[0] + t * (b[0] - a[0]), a[1] + t * (b[1] - a[1]), a[2] + t * (b[2] - a[2])});
            }
        }
        poly.swap(next);
    }
    return !poly.empty();
}

std::vector<std::array<double, 3>> QuadPolygon(const float o[3], const float e1[3], const float e2[3]) {
    std::vector<std::array<double, 3>> p(4);
    for (int k = 0; k < 3; ++k) {
        p[0][k] = o[k]; p[1][k] = double(o[k]) + e1[k]; p[2][k] = double(o[k]) + e1[k] + e2[k]; p[3][k] = double(o[k]) + e2[k];
    }
    return p;
}

// 14019AA40 + 14004BB40: the six planes of the off-centre perspective volume (l, r, b, t, n, f), stored as in the original:
// -(col3 + col0), -(col3 - col0), -(col3 + col1), -(col3 - col1), -col2, -(col3 - col2); a point is inside when n.p + d < 0.
void PyramidPlanes(float l, float r, float b, float t, float n, float f, std::vector<Plane>& planes) {
    float m[16] = {};
    const float w = 1.0f / (r - l), h = 1.0f / (t - b), fn = f / (f - n);
    m[0] = w * (n + n); m[5] = h * (n + n);
    m[8] = -((l + r) * w); m[9] = -((b + t) * h); m[10] = fn; m[11] = 1.0f;
    m[14] = -(fn * n);
    planes.clear();
    const auto add = [&](float sa, int ca, float sb, int cb) {   // -(sa*col_ca + sb*col_cb)
        const int c0 = ca, c1 = cb; Plane p{};
        for (int k = 0; k < 4; ++k) {
            const double v = -(double(sa) * m[4 * k + c0] + double(sb) * m[4 * k + c1]);
            if (k < 3) p.n[k] = v; else p.d = v;
        }
        planes.push_back(p);
    };
    add(1, 3, 1, 0); add(1, 3, -1, 0); add(1, 3, 1, 1); add(1, 3, -1, 1);
    Plane p{}; for (int k = 0; k < 4; ++k) { const double v = -double(m[4 * k + 2]); if (k < 3) p.n[k] = v; else p.d = v; }
    planes.push_back(p);
    add(1, 3, -1, 2);
}
}  // namespace

// FUN_14001CE40: out = a * b. The disassembly (MULPS by the splatted lanes, then ADDPS) sums (a2*b8 + a0*b0) + (a3*b12 + a1*b4); the
// decompile listing a flat chain was misleading (the pair order matters for bit identity).

void DecalMatMul(const float* a, const float* b, float* out) {
    float r[16];
    for (int i = 0; i < 4; ++i) {
        const float a0 = a[4 * i], a1 = a[4 * i + 1], a2 = a[4 * i + 2], a3 = a[4 * i + 3];
        for (int c = 0; c < 4; ++c) r[4 * i + c] = (a2 * b[8 + c] + a0 * b[c]) + (a3 * b[12 + c] + a1 * b[4 + c]);
    }
    Copy16(out, r);
}

// FUN_14004BCA0: inverse by cofactors of 2x2 sub-determinants, then one divps of 1.0 by the determinant.
void DecalMatInverse(const float* B, float* out, float* det4) {
    const float f7 = B[8], f16 = B[9], f8 = B[10], f9 = B[11], f3 = B[14], f5 = B[15], f1 = B[12], f13 = B[13];
    const float f10 = B[0], f17 = B[1], f11 = B[2], f12 = B[3], f4 = B[6], f6 = B[7], f2 = B[4], f15 = B[5];
    const float f26 = f9 * f11 - f8 * f12, f27 = f5 * f11 - f3 * f12, f28 = f9 * f4 - f8 * f6, f29 = f5 * f4 - f3 * f6;
    const float f30 = f10 * f16 - f7 * f17, f31 = f10 * f13 - f1 * f17, f32 = f2 * f16 - f7 * f15, f33 = f2 * f13 - f1 * f15;
    const float f18 = f11 * f6 - f4 * f12, f20 = f8 * f5 - f3 * f9, f21 = f10 * f15 - f2 * f17, f23 = f7 * f13 - f1 * f16;
    const float f19 = (f15 * f20 - f16 * f29) + f28 * f13;
    const float f22 = (f17 * f29 - f15 * f27) + f18 * f13;
    const float f24 = (f16 * f27 - f13 * f26) - f20 * f17;
    const float f25 = (f15 * f26 - f16 * f18) - f28 * f17;
    const float det = (f10 * f19 + f7 * f22) + (f2 * f24 + f1 * f25);   // the SIMD horizontal sum: (lane0 + lane2) + (lane1 + lane3)
    if (det4) det4[0] = det4[1] = det4[2] = det4[3] = det;
    const float r = 1.0f / det;
    float o[16];
    o[4] = ((f7 * f29 - f1 * f28) - f2 * f20) * r;
    o[5] = ((f10 * f20 - f7 * f27) + f1 * f26) * r;
    o[6] = ((f2 * f27 - f1 * f18) - f10 * f29) * r;
    o[7] = ((f10 * f28 - f2 * f26) + f7 * f18) * r;
    o[8] = ((f6 * f23 - f9 * f33) + f32 * f5) * r;
    o[9] = ((f9 * f31 - f5 * f30) - f23 * f12) * r;
    o[10] = ((f12 * f33 - f6 * f31) + f21 * f5) * r;
    o[11] = ((f6 * f30 - f9 * f21) - f32 * f12) * r;
    o[0] = r * f19; o[1] = r * f24; o[2] = r * f22; o[3] = r * f25;
    o[12] = ((f33 * f8 - f32 * f3) - f23 * f4) * r;
    o[13] = ((f23 * f11 - f31 * f8) + f30 * f3) * r;
    o[14] = ((f31 * f4 - f21 * f3) - f33 * f11) * r;
    o[15] = ((f32 * f11 - f30 * f4) + f21 * f8) * r;
    Copy16(out, o);
}

// FUN_141778230 (read from the disassembly, the decompile lost its XMM argument): cosine. Range reduction
// x -= trunc((x + copysign(pi, x)) / 2pi) * 2pi, then the lane vector (1, x, x^2, x^3), its square (1, x^2, x^4, x^6) times three
// coefficient vectors scaled by (x^6 * x^2)^0, ^1, ^2, each horizontally summed as (v1 + v3) + (v0 + v2), and the three sums added.
float DecalCos(float x) {
    const float s = FromBits((Bits(x) & 0x80000000u) | Bits(kPi));
    const int k = static_cast<int>((s + x) / kTwoPi);
    x = x - static_cast<float>(k) * kTwoPi;
    const float z = x * x, x3 = z * x;
    const float a[4] = {FromBits(kOneBits), x, z, x3};
    float sq[4]; for (int i = 0; i < 4; ++i) sq[i] = a[i] * a[i];
    const float bc = sq[3] * a[2];
    float t0[4], t1[4], t2[4];
    for (int i = 0; i < 4; ++i) {
        t0[i] = sq[i] * FromBits(kCosC0[i]);
        t1[i] = (sq[i] * bc) * FromBits(kCosC1[i]);
        t2[i] = ((sq[i] * bc) * bc) * FromBits(kCosC2[i]);
    }
    const auto hsum = [](const float* v) { return (v[1] + v[3]) + (v[0] + v[2]); };
    return (hsum(t1) + hsum(t0)) + hsum(t2);
}

bool ComputeDecalConstants(const DecalConstantInput& in, DecalConstants& out) {
    out = DecalConstants{};
    const uint8_t* D = in.decal;
    const uint8_t* G = in.manager_400;
    const DecalView& V = in.view;
    const auto manager_f = [&](unsigned off) { return Rf(G, off - 0x400); };

    // ---- gating (198910: material object, then the diffuse texture of the family) --------------------------------------
    if (!in.material_present) return false;
    const bool blend_weight = D[0x26c] != 0;
    if (Ru64(D, blend_weight ? 0x1b0 : 0x190) == 0) return false;

    // ---- emissive intensity (fStack_e98) -----------------------------------------------------------------------------
    float emissive = Rf(D, 0x234);
    if (Ru64(D, 0x1b8) != 0 && Ru64(D, 0x130) != 0 && in.emissive_curve.found)
        emissive = (in.emissive_curve.value - Rf(D, 0x234)) * in.emissive_curve.weight + Rf(D, 0x234);

    // ---- programme selection (iVar36 pixel, iVar37 vertex description) -------------------------------------------------
    const bool mask_tex = Ru64(D, 0x1c0) != 0;
    const int variant = (mask_tex ? 1 : 0) + (in.batch_emissive ? 2 : 0);   // Am + 2*E
    const bool has_spec = Ru64(D, 0x198) != 0;
    int ps = 0, vs = 1;
    const auto pick = [&](const int (&t)[4]) { return t[variant]; };
    if (Ri(D, 0x248) == 1) {   // cubic decals
        if (D[0x25a] == 0) {
            if (!has_spec) { static const int t[4] = {0x1f, 0x27, 0x23, 0x2b}; ps = pick(t); vs = 10; }
            else { static const int t[4] = {0x20, 0x28, 0x24, 0x2c}; ps = pick(t); vs = 0xb; }
        } else if (!has_spec) { static const int t[4] = {0x21, 0x29, 0x25, 0x2d}; ps = pick(t); vs = 0xc; }
        else { static const int t[4] = {0x22, 0x2a, 0x26, 0x2e}; ps = pick(t); vs = 0xd; }
    } else if (D[0x26c] == 0) {
        if (D[0x25b] == 0) {   // not POM
            if (D[0x258] == 0) {   // not blood
                if (D[0x25a] == 0) {
                    if (!has_spec) { static const int t[4] = {1, 0xf, 8, 0x16}; ps = pick(t); vs = 1; }
                    else { static const int t[4] = {2, 0x10, 9, 0x17}; ps = pick(t); vs = 2; }
                } else if (!has_spec) {
                    if (std::fabs(Rf(D, 0x210) - 0.0f) <= kParallaxEpsilon) { static const int t[4] = {3, 0x11, 10, 0x18}; ps = pick(t); vs = 3; }
                    else { static const int t[4] = {4, 0x12, 0xb, 0x19}; ps = pick(t); vs = 4; }
                } else { static const int t[4] = {6, 0x14, 0xd, 0x1b}; ps = pick(t); vs = 6; }
            } else { ps = 0x1d; vs = 8; }   // Decal_Blood_DifSpcBump
        } else { static const int t[4] = {5, 0x13, 0xc, 0x1a}; ps = pick(t); vs = 5; }   // POM
    } else { ps = 0x1e; vs = 9; }   // Decal_BlendWeight
    out.pixel_programme = ps;
    out.vertex_description = vs;
    out.pixel_table_index = in.table_variant * 0x2f + ps;
    out.vertex_table_index = in.table_variant * 0x0e + vs;

    // ---- the box rows (masked-lane sums, operation order of the decompile) -------------------------------------------
    float A0[4], A1[4], A2[4], P[4], H[4];
    std::memcpy(H, D + 0x140, 16); std::memcpy(A0, D + 0x150, 16); std::memcpy(A1, D + 0x160, 16);
    std::memcpy(A2, D + 0x170, 16); std::memcpy(P, D + 0x180, 16);
    const float hx = H[0], hy = H[1], hz = H[2], hw = H[3];
    // mask results: only the selected lane keeps its half extent, the others are +0.0f (x & 0)
    float r0[4], r1[4], r2[4], p3[4];
    for (int j = 0; j < 4; ++j) {
        r0[j] = 0.0f * P[j] + 0.0f * A1[j] + 0.0f * A2[j] + hx * A0[j];   // fVar60,58,59 = 0, fVar52 = hx   (143D4A6E0: lane x)
        r1[j] = 0.0f * P[j] + hy * A1[j] + 0.0f * A2[j] + 0.0f * A0[j];   // fVar68,65,67,61                  (143D1F6D0: lane y)
        r2[j] = 0.0f * P[j] + 0.0f * A1[j] + hz * A2[j] + 0.0f * A0[j];   // fVar75,72,74,70                  (143D1F6E0: lane z)
        p3[j] = 1.0f * P[j] + 0.0f * A1[j] + 0.0f * A2[j] + 0.0f * A0[j]; // 143D1C080 = (0,0,0,1): the position row
    }
    (void)hw;
    const float ws = Rf(D, 0x244);
    const bool cubic = Ri(D, 0x248) == 1;
    const bool persp = !(cubic || !(ws > 1.0f));
    out.perspective = persp;

    M4 Wscaled;   // da8: [r0; r1; r2; P]
    std::memcpy(Wscaled, r0, 16); std::memcpy(Wscaled + 4, r1, 16); std::memcpy(Wscaled + 8, r2, 16); std::memcpy(Wscaled + 12, p3, 16);
    M4 Wd;        // b78..b38: [A0; A1; A2; P] (unit axes)
    std::memcpy(Wd, A0, 16); std::memcpy(Wd + 4, A1, 16); std::memcpy(Wd + 8, A2, 16); std::memcpy(Wd + 12, P, 16);

    // ---- "camera inside" (fStack_ea4 low byte): near-plane quad of the camera against the decal volume ---------------
    {
        const float t = TanF(V.fov * kHalf);
        const float n = V.near_plane;
        float a = n * t;
        float b = t * V.aspect;
        a = a * a;
        b = b * n;
        float nn = n * n;
        b = b * b;
        a = a + b;
        a = a + nn;
        const float radius = std::sqrt(a);   // 141FD1F60
        const float sphere[4] = {V.camera_world[12], V.camera_world[13], V.camera_world[14], radius};
        if (!persp) {
            // OBB [half; A0; A1; A2; P] straight from the decal bytes (+0x140..+0x18F)
            float obb[20]; std::memcpy(obb, D + 0x140, 80);
            if (SphereVsBox(sphere, obb)) {
                float c[8][4];
                FrustumCorners(V.fov, V.near_plane, V.far_plane, V.aspect, c);
                // quad rows: edge1 = c5 - c4, edge2 = c6 - c4, origin = c4 (w = 1); then 14006C380 moves it with the camera world matrix
                float e1[4], e2[4], o[4];
                for (int k = 0; k < 3; ++k) { e1[k] = c[5][k] - c[4][k]; e2[k] = c[6][k] - c[4][k]; o[k] = c[4][k]; }
                e1[3] = e2[3] = 0; o[3] = 1.0f;
                const float* M = V.camera_world;
                float E1[3], E2[3], O[3];
                for (int k = 0; k < 3; ++k) {
                    E1[k] = ((e1[0] * M[k] + e1[1] * M[4 + k]) + 0.0f * M[12 + k]) + e1[2] * M[8 + k];
                    E2[k] = ((e2[0] * M[k] + e2[1] * M[4 + k]) + 0.0f * M[12 + k]) + e2[2] * M[8 + k];
                    O[k] = ((o[2] * M[8 + k] + o[0] * M[k]) + o[1] * M[4 + k]) + M[12 + k];
                }
                // 1417B5A60: quad against the oriented box. Exact test: quad in box coordinates, clipped by the six faces.
                std::vector<std::array<double, 3>> poly = QuadPolygon(O, E1, E2);
                for (auto& pt : poly) {
                    const double d[3] = {pt[0] - P[0], pt[1] - P[1], pt[2] - P[2]};
                    const float* ax[3] = {A0, A1, A2};
                    for (int k = 0; k < 3; ++k) pt[k] = d[0] * ax[k][0] + d[1] * ax[k][1] + d[2] * ax[k][2];
                }
                std::vector<Plane> faces;
                for (int k = 0; k < 3; ++k) {
                    Plane p{}; p.n[k] = 1; p.d = -double(H[k]); faces.push_back(p);
                    Plane q{}; q.n[k] = -1; q.d = -double(H[k]); faces.push_back(q);
                }
                out.camera_inside = PolygonTouches(std::move(poly), faces);
            }
        } else {
            // widened OBB: extents (ws*hx, hy, ws*hz, 1.0*hw), same axes and centre
            float obb[20] = {ws * hx, 1.0f * hy, ws * hz, 1.0f * hw};
            std::memcpy(obb + 4, A0, 16); std::memcpy(obb + 8, A1, 16); std::memcpy(obb + 12, A2, 16); std::memcpy(obb + 16, P, 16);
            if (SphereVsBox(sphere, obb)) {
                const float d238 = Rf(D, 0x238), d23c = Rf(D, 0x23c);
                // near distance of the pyramid used by this test (not the L of the render projection below):
                float n2 = (hy * kTwo * d238 * kHalf) / (d238 * ws - d238 * kHalf);
                const float far2 = hy * kTwo + n2;
                std::vector<Plane> planes;
                PyramidPlanes(d238 * kNegHalf, d238 * kHalf, d23c * kNegHalf, d23c * kHalf, n2, far2, planes);
                // camera frustum corners (view space) -> pyramid space: inverse(Wd * View) * T(0, hy + n2, 0) * S
                float X1[16], I[16];
                DecalMatMul(Wd, V.world_to_view, X1);
                DecalMatInverse(X1, I);
                M4 T; Identity(T); T[13] = hy + n2;
                const float S[16] = {1,0,0,0, 0,0,1,0, 0,1,0,0, 0,0,0,1};
                float TS[16], Minv[16];
                DecalMatMul(T, S, TS);
                DecalMatMul(I, TS, Minv);
                float c[8][4];
                FrustumCorners(V.fov, V.near_plane, V.far_plane, V.aspect, c);
                float e1[3], e2[3], o[3], E1[3], E2[3], O[3];
                for (int k = 0; k < 3; ++k) { e1[k] = c[5][k] - c[4][k]; e2[k] = c[6][k] - c[4][k]; o[k] = c[4][k]; }
                for (int k = 0; k < 3; ++k) {
                    E1[k] = e1[2] * Minv[8 + k] + e1[1] * Minv[4 + k] + e1[0] * Minv[k];
                    E2[k] = e2[2] * Minv[8 + k] + e2[1] * Minv[4 + k] + e2[0] * Minv[k];
                    O[k] = o[1] * Minv[4 + k] + o[0] * Minv[k] + o[2] * Minv[8 + k] + Minv[12 + k];
                }
                out.camera_inside = PolygonTouches(QuadPolygon(O, E1, E2), planes);
            }
        }
    }

    // ---- camera-relative clip transform ----------------------------------------------------------------------------------
    const float cx = V.camera_world[12], cy = V.camera_world[13], cz = V.camera_world[14];
    M4 Rcam, Rinv, VPrel;
    std::memcpy(Rcam, V.camera_world, 48);   // rows view+0xD0, +0xE0, +0xF0 (full four floats, w included)
    std::memcpy(Rcam + 12, kRowW, 16);       // row 3 = (0,0,0,1) (1439F2500)
    float det4[4];
    DecalMatInverse(Rcam, Rinv, det4);
    DecalMatMul(Rinv, V.projection, VPrel);

    M4 Wrel;
    std::memcpy(Wrel, r0, 16); std::memcpy(Wrel + 4, r1, 16); std::memcpy(Wrel + 8, r2, 16);
    Wrel[12] = p3[0] - cx; Wrel[13] = p3[1] - cy; Wrel[14] = p3[2] - cz; Wrel[15] = p3[3] - 0.0f;   // fVar73 = 1439F249C = 0
    M4 C;   // decal local -> clip (camera relative), the VS VC_DecalTransform
    DecalMatMul(Wrel, VPrel, C);
    M4 Cz = {};   // the matrix whose inverse maps clip back to decal space
    std::memcpy(Cz, C, 64);
    if (persp) {
        M4 Wu;   // [A0; A1; A2; P - camera]
        std::memcpy(Wu, A0, 16); std::memcpy(Wu + 4, A1, 16); std::memcpy(Wu + 8, A2, 16);
        Wu[12] = P[0] - cx; Wu[13] = P[1] - cy; Wu[14] = P[2] - cz; Wu[15] = P[3] - 0.0f;
        DecalMatMul(Wu, VPrel, Cz);
    }

    // ---- vs b10 (cbMatrixWorld) ----------------------------------------------------------------------------------------------
    for (int c = 0; c < 4; ++c) for (int r = 0; r < 4; ++r) out.vs_b10[4 * c + r] = C[4 * r + c];
    out.vs_b10[16] = out.vs_b10[17] = ws;   // VC_WidthScale = (w, w, 0, 0)
    out.vs_b10[18] = out.vs_b10[19] = 0.0f;

    // ---- Q = T_apex * S * Pm * S * F * K ---------------------------------------------------------------------------------
    M4 Cinv, K, F, Tapex, Pm, S, Q;
    DecalMatInverse(Cz, Cinv, det4);
    MakeK(cubic ? 2 : (persp ? 1 : 0), K);
    Identity(F);
    if (D[0x259] != 0) F[0] = -1.0f;   // 143D7F240 = (-1,0,0,0) + rows Y, Z, W: mirrors the decal's x axis
    std::memcpy(S, kRowX, 16); std::memcpy(S + 4, kRowZ, 16); std::memcpy(S + 8, kRowY, 16); std::memcpy(S + 12, kRowW, 16);   // swaps y and z
    Identity(Tapex); Identity(Pm);
    if (persp) {
        const float d238 = Rf(D, 0x238), d23c = Rf(D, 0x23c);
        float two_hy = hy * kTwo;
        float L = (d238 * two_hy) / (d238 * ws - d238);          // distance of the pyramid apex before the narrow end
        // 14019AB60(out, width = +0x238, height = +0x23C, near = L, far = L + 2*hy): left-handed perspective
        const float nearv = L, farv = L + two_hy;
        float p5 = farv / (farv - nearv);
        const float p2 = (nearv + nearv) / d238;
        const float p3v = (nearv + nearv) / d23c;
        const uint32_t u7 = Bits(p5 * nearv) ^ 0x80000000u;
        Pm[0] = p2; Pm[1] = Pm[2] = Pm[3] = 0;
        Pm[4] = 0; Pm[5] = p3v; Pm[6] = Pm[7] = 0;
        Pm[8] = Pm[9] = 0; Pm[10] = p5; Pm[11] = 1.0f;
        Pm[12] = Pm[13] = 0; Pm[14] = FromBits(u7); Pm[15] = 0;
        Tapex[13] = L + hy;   // translation (0, L + hy, 0): the apex becomes the origin
    }
    {
        M4 t3, t4, t5;
        DecalMatMul(Tapex, S, t3);
        DecalMatMul(t3, Pm, t4);
        DecalMatMul(t4, S, t5);
        DecalMatMul(t5, F, t3);
        DecalMatMul(t3, K, Q);
    }
    M4 Z;   // clip -> decal space
    DecalMatMul(Cinv, Q, Z);

    // ---- ps b10 (cbDecalParam) -----------------------------------------------------------------------------------------
    float* cb = out.ps_b10;
    const auto put_transposed = [&](int row0, const float* m) {
        for (int c = 0; c < 4; ++c) for (int r = 0; r < 4; ++r) cb[4 * (row0 + c) + r] = m[4 * r + c];
    };
    put_transposed(0, Z);                                             // cb[0..3]   FC_ProjToDecalSpace
    M4 invProj; DecalMatInverse(V.projection, invProj, det4);
    put_transposed(4, invProj);                                       // cb[4..7]   g_InvProjMatrix
    put_transposed(8, V.camera_world);                                // cb[8..11]  g_CameraMatrix (columns: .w = position)
    const float width = static_cast<float>(V.screen_width), height = static_cast<float>(V.screen_height);
    cb[48] = width; cb[49] = height; cb[50] = 1.0f / width; cb[51] = 1.0f / height;   // cb[12] unused_FC_ScreenSize
    M4 FW;   // F * [A0; A1; A2; P]
    DecalMatMul(F, Wd, FW);
    std::memcpy(cb + 52, FW, 48);                                     // cb[13..15] FC_DecalWorldRotation rows 0-2
    {
        // cb[16] FC_EyeInDecalSpace (unused): row 3 of inverse(Wscaled * View) * Q
        M4 X, Xi, Y;
        DecalMatMul(Wscaled, V.world_to_view, X);
        DecalMatInverse(X, Xi, det4);
        DecalMatMul(Xi, Q, Y);
        std::memcpy(cb + 64, Y + 12, 16);
    }
    cb[68] = Rf(D, 0x210);                                            // cb[17] FC_ParallaxScale
    cb[69] = Ru64(D, 0x1a8) != 0 ? 1.0f : 0.0f;
    cb[70] = has_spec ? Rf(D, 0x214) : 0.0f;
    cb[71] = 0.0f;
    cb[72] = Rf(D, 0x250); cb[73] = Rf(D, 0x254); cb[74] = cb[75] = 0.0f;   // cb[18] FC_DecalSoakThreshold
    cb[76] = cb[77] = cb[78] = cb[79] = 0.0f;                         // cb[19] FC_CubeCenterOffset (1439F2490 / 1439F2498: zeros)
    {
        // cb[20] FC_DiffuseScale = (+0x218.. * material, lifetime fade * material[3]); 14019B880 lifetime fade
        float life = FromBits(kOneBits);
        const float d418 = Rf(D, 0x418);
        if (d418 != 0.0f) {
            const float q = Rf(D, 0x414) / d418;
            life = FromBits(kOneBits) - (q <= FromBits(kOneBits) ? q : FromBits(kOneBits));
        }
        const float* m = in.material_scale;
        cb[80] = Rf(D, 0x218) * m[0]; cb[81] = Rf(D, 0x21c) * m[1]; cb[82] = Rf(D, 0x220) * m[2];
        cb[83] = life * m[3];
    }
    std::memcpy(cb + 84, D + 0x224, 16);                              // cb[21] FC_ReflectanceScale (3) + FC_ShininessScale
    cb[88] = cb[89] = cb[90] = cb[91] = 0.0f;                         // cb[22] FC_LightInDecalSpace
    cb[92] = emissive; cb[93] = cb[94] = cb[95] = 0.0f;               // cb[23] FC_EmissiveIntensity
    {
        // cb[24] near / far fade, divided by the decal depth (2*hy, or the floor when it is zero)
        float range = hy * kTwo;
        if (range == 0.0f) range = kTinyRange;
        float near_range = kTinyRange;
        const float m40c = manager_f(0x40c), m410 = manager_f(0x410), m414 = manager_f(0x414), m418 = manager_f(0x418);
        if (0.0f < m410) near_range = m410 / range;
        float far_start = m414;
        if (far_start < 0.0f || range <= far_start) far_start = range - m418;
        far_start = far_start / range;
        float far_range = kTinyRange;
        if (0.0f < m418) far_range = m418 / range;
        cb[96] = m40c / range;
        cb[97] = FromBits(kOneBits) / near_range;
        cb[98] = far_start;
        cb[99] = FromBits(kOneBits) / far_range;
    }
    {
        // cb[25] normal fade: cos of the manager angles; start = cos(min(+0x420 + +0x41C, pi)), range = 1 / (cos(+0x41C) - start)
        const float m41c = manager_f(0x41c), m420 = manager_f(0x420);
        const float sum = m420 + m41c;
        const float arg = (kPi <= sum) ? kPi : sum;
        const float start = DecalCos(arg);
        const float end = DecalCos(m41c);
        cb[100] = start;
        cb[101] = (0.0f < end - start) ? FromBits(kOneBits) / (end - start) : kInfiniteRecip;
        cb[102] = cb[103] = 0.0f;
    }

    // ---- ps b2 (cbMatDynParam) ---------------------------------------------------------------------------------------------
    out.ps_b2[16] = Rf(D, 0x25c); out.ps_b2[17] = 0.0f; out.ps_b2[18] = Rf(D, 0x260); out.ps_b2[19] = Rf(D, 0x264);
    out.ps_b2_tail[0] = Rf(D, 0x268); out.ps_b2_tail[1] = out.ps_b2_tail[2] = out.ps_b2_tail[3] = 1.0f;
    return true;
}
}  // namespace ds3content
