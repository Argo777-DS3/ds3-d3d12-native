// Console test of ComputeDecalConstants (native/content/decal_constants.cpp) on the decal oracle of the Reference run.
// Usage: decal_constants_test.exe [startup.jsonl [-v]]   (with a path, every decal_oracle row that has bytes is checked; without, the
// first row of i66-reference-2124 embedded below)
// 1. Byte-for-byte comparison (i66-reference-2134-decal-oracle and later): rows carry the buffers the original uploaded for the draw
//    (vs_cb10 80 B, ps_cb10 416 B, ps_cb2 80 B); the 2134 run (27 rows, 1280x720) matches bit for bit, so any bit difference fails.
// 2. Geometric consistency checks (the 2124 oracle had no buffers):
//  (a) the decal centre through VC_DecalTransform (dp4 rows, as the VS) == its world position through the view's view-projection;
//  (b) the clip position of the centre through FC_ProjToDecalSpace (ndc.xy, depth, 1; divided by w, as the PS) == the decal-space
//      centre: (0.5, 0.5, 0.5) for a box decal; for a perspective decal the projection-axis coordinate is the perspective depth
//      fraction f/(f-n)*(1-n/y') of the pyramid (the code's convention), the other two stay 0.5;
//  (c) points of the unit cube widened as the VS does (x, z by lerp(1, VC_WidthScale, (y+1)/2)) map to the analytic decal-space
//      coordinates, so the VS widening and the PS mapping agree (x = +-1 gives u = 1 / 0 at every depth).
#include "decal_constants.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace {
using namespace ds3content;

// First decal_oracle row of i66-reference-2124 (decal object 0x420 bytes, view context bytes 0..0x28F, manager+0x400..+0x42F, batch).
const char* kDecalHex = "D8B821E2F67F0000888FB1E2F67F0000004BD8ADF37F0000EDA982436D4A56C2ED3D16441CE2C53E6B2B043FA3A5433F5E4569BF7585AF3D384BCE3EA3D0C83EA3D0C83E0000B04054555541000000000000000000000000000000000000803F60F3B6A5F37F000070F5B6A5F37F00000000000000000000C0F4B6A5F37F0000000000000000000050FCB6A5F37F0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000010000000000803FD1D0503D8180803B8180003C0000803F0000803F0000803F0000803F0000803F0000F0208FC2753D0000003F007F00000000803F00000041000080420000803F00000000000000000000000000000000103CDBADF37F00000200010000000000A3D0483E00003040A3D0483E000000005E4569BF7585AF3D384BCE3E000000001CE2C5BE6B2B04BFA3A543BF000000006EF1113E4B245ABF20EA003F00000000E22182434BF85BC26BB715440000803F60F3B6A5F37F000070F5B6A5F37F00000000000000000000C0F4B6A5F37F00000000000000000000000000000000000050FCB6A5F37F0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000100000000004040000000000000803FD1D0503D8180803B8180003C0000803F0000803F0000803F0000803F0000803FA3D0C83EA3D0C83E0000803F5455554100000000010000008FC2753D0000003F000000000000803F00000041000080420000803F007F000010C24FADF37F0000509FEBABF37F00000000803F0000803F00000000000000000000803F0000803F00000000000000000000803F0000803F00000000000000000000803F0000803F0000000000000000000000000000000000000000000000000000803F0000803F0000803F0000803F0000803F0000803F0000803F0000803F003B580800000000003B58080000000000000000FF0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000FFFFFFFF00000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF0000803F0000803F0000803F0000803F000000000000000000000000000000000600000001000000003B580864000000003B5808640000000000000000000000003B580800000000003B580800000000000000000000803F0000000000000000000000000000000000000000000000000000803F0000803F0000000000000000000000000000C8C20000C8C20000C8C20000C8420000C8420000C84200000000000000000000000000000000000000000101403F0AD7A33D00401C4600050000000000000000000000000000F37F0000";
const char* kViewHex = "282321E2F67F00008090B1E2F67F00007CBC55BF5CFA3ABE90EB04BF00000000010000B36280713FFADBA9BE0000000068E60C3F17D18DBE9FA149BF000000002A73E1C27D838443BCD314440000803F5CC8B63F0000000000000000000000000000000035792240000000000000000000000080000000804300803F0000803F000000000000000060D7A3BD00000000489B98BFEB55EDBED6EB04BF90EB04BF5DC836B39645194053DCA9BEFADBA9BE2934493FEB0234BF09A249BF9FA149BF53F820C3E5332844EBCE1444BCD314447BBC55BF0000000068E60C3F000000005BFA3ABE5F80713F15D18DBE000000008FEB04BFFADBA9BE9EA149BF00000000D1AF8343931452C2191517440000803F0000803F0000803F0000803F0000803F00000000450000000C00000009000000582D20E2F67F000000000000000000007CBC55BF5CFA3ABE90EB04BF00000000010000B36280713FFADBA9BE0000000068E60C3F17D18DBE9FA149BF000000002A73E1C27D838443BCD314440000803F7BBC55BFFCFF7FB267E60C3F000000005AFA3ABE5E80713F14D18DBE000000008FEB04BFF9DBA9BE9EA149BF00000000D1AF8343951452C21A1517440000803F5DC8B63F0000000000000000000000000000000036792240000000000000000000000000000000004300803F0000803F000000000000000060D7A3BD00000000499B98BFEC55EDBED6EB04BF90EB04BF5EC836B39745194053DCA9BEFADBA9BE2A34493FED0234BF09A249BF9FA149BF54F820C3E6332844EBCE1444BCD3144400050000D0020000000000000000000090EB04BFFADBA9BE9FA149BFBCD3144490EB04BFFADBA9BE9FA149BFBCD314443720403F0AD7A33D00401C46398EE33F0000803F000000000000000000000000";
const char* kManagerHex = "D007000002000000FFFFFFFF000000000000803E000080BF0000003FDB0F494000000000000000004070CAE0F67F0000";
const char* kBatchHex = "B00DECABF37F0000000DECABF37F0000";

std::vector<uint8_t> FromHex(const std::string& h) {
    std::vector<uint8_t> b(h.size() / 2);
    for (size_t i = 0; i < b.size(); ++i) b[i] = static_cast<uint8_t>(std::strtoul(h.substr(2 * i, 2).c_str(), nullptr, 16));
    return b;
}
float Rf(const std::vector<uint8_t>& b, size_t off) { float v; std::memcpy(&v, b.data() + off, 4); return v; }

struct Sample { std::string decal, view, manager, batch, vs_cb10, ps_cb10, ps_cb2; };
uint32_t g_screen_w = 1920, g_screen_h = 1080;   // the capture's screen size (1280x720 for i66-reference-2134)

bool Extract(const std::string& line, const char* key, std::string& value) {
    const std::string k = std::string("\"") + key + "\":\"";
    const size_t p = line.find(k);
    if (p == std::string::npos) return false;
    const size_t b = p + k.size(), e = line.find('"', b);
    if (e == std::string::npos) return false;
    value = line.substr(b, e - b);
    return true;
}

void PrintRow(const char* name, const float* v) { std::printf("  %-36s % 13.6f % 13.6f % 13.6f % 13.6f\n", name, v[0], v[1], v[2], v[3]); }

int g_fail = 0, g_pass = 0;
void Expect(bool ok, const char* what, double err, double tol) {
    std::printf("  [%s] %s (max error %.3g, tolerance %.3g)\n", ok ? "ok" : "FAIL", what, err, tol);
    if (ok) ++g_pass; else ++g_fail;
}

// y = x * M for a 4-vector (row-vector convention)
void VecMat(const double* x, const float* m, double* y) { for (int c = 0; c < 4; ++c) y[c] = x[0] * m[c] + x[1] * m[4 + c] + x[2] * m[8 + c] + x[3] * m[12 + c]; }
// dp4 against the rows of a constant buffer: out[i] = dot(cb[i], v)
void Dp4(const float* cb, const double* v, double* out, int rows) { for (int i = 0; i < rows; ++i) out[i] = double(cb[4 * i]) * v[0] + double(cb[4 * i + 1]) * v[1] + double(cb[4 * i + 2]) * v[2] + double(cb[4 * i + 3]) * v[3]; }

void SelfTests() {
    double err = 0;
    for (double a : {0.0, 0.5, 1.0, 2.0, 3.14159274, 4.0, -1.0, -3.0, 6.0, 10.0, -7.5}) err = std::fmax(err, std::fabs(DecalCos(float(a)) - std::cos(a)));
    Expect(err < 2e-6, "DecalCos (141778230 polynomial) against std::cos", err, 2e-6);
    const float m[16] = {1.2f, 0.1f, 0.0f, 0.3f, -0.2f, 0.9f, 0.4f, 0.0f, 0.05f, -0.3f, 1.1f, 0.2f, 3.0f, -2.0f, 5.0f, 1.0f};
    float inv[16], id[16];
    DecalMatInverse(m, inv, nullptr);
    DecalMatMul(m, inv, id);
    err = 0;
    for (int i = 0; i < 16; ++i) err = std::fmax(err, std::fabs(id[i] - ((i % 5) == 0 ? 1.0f : 0.0f)));
    Expect(err < 1e-5, "DecalMatInverse (14004BCA0): m * inverse(m) == identity", err, 1e-5);
}

bool BuildInput(const Sample& s, DecalConstantInput& in) {
    const std::vector<uint8_t> d = FromHex(s.decal), v = FromHex(s.view), m = FromHex(s.manager), b = FromHex(s.batch);
    if (d.size() < kDecalObjectSize || v.size() < 0x280 || m.size() < kDecalManagerBytes || b.size() < 9) return false;
    std::memcpy(in.decal, d.data(), kDecalObjectSize);
    std::memcpy(in.manager_400, m.data(), kDecalManagerBytes);
    in.batch_emissive = b[8] != 0;
    std::memcpy(in.view.world_to_view, v.data() + 0x10, 64);
    std::memcpy(in.view.projection, v.data() + 0x50, 64);
    std::memcpy(in.view.view_projection, v.data() + 0x90, 64);
    std::memcpy(in.view.camera_world, v.data() + 0xd0, 64);
    in.view.fov = Rf(v, 0x270); in.view.near_plane = Rf(v, 0x274); in.view.far_plane = Rf(v, 0x278); in.view.aspect = Rf(v, 0x27c);
    in.view.screen_width = g_screen_w; in.view.screen_height = g_screen_h;   // [view+0x418] is not in view_bytes
    return true;
}

// The camera-inside flag never fires in the capture (the camera is far from the decals): move the camera into the volume.
void InsideTests(const Sample& s) {
    for (int box = 0; box < 2; ++box) {
        DecalConstantInput in;
        if (!BuildInput(s, in)) return;
        const float one = 1.0f;
        if (box) std::memcpy(in.decal + 0x244, &one, 4);   // width scale 1: a box decal (non-perspective path)
        float P[3]; std::memcpy(P, in.decal + 0x180, 12);
        struct Case { const char* what; float along; bool expect; } cases[] = {{"camera at the decal centre", 0.0f, true}, {"camera 40 units behind the decal centre", -40.0f, false}};
        for (const Case& c : cases) {
            DecalConstantInput t = in;
            float* cam = t.view.camera_world;
            for (int k = 0; k < 3; ++k) cam[12 + k] = P[k] + c.along * cam[8 + k];   // along the camera forward axis (row 2)
            DecalMatInverse(cam, t.view.world_to_view);   // consistent view matrix for the pyramid path
            DecalConstants out;
            const bool ok = ComputeDecalConstants(t, out);
            Expect(ok && out.camera_inside == c.expect, (std::string(box ? "(d) box decal, " : "(d) pyramid decal, ") + c.what + (c.expect ? " -> inside" : " -> outside")).c_str(), 0, 0);
        }
    }
}

void Run(const Sample& s, int index) {
    DecalConstantInput in;
    if (!BuildInput(s, in)) { std::printf("sample %d: truncated, skipped\n", index); return; }
    const std::vector<uint8_t> d(in.decal, in.decal + kDecalObjectSize);
    DecalConstants out;
    const bool ok = ComputeDecalConstants(in, out);
    std::printf("=== sample %d: %s, pixel programme %d, vertex description %d, perspective %d, camera inside %d\n", index,
                ok ? "drawn" : "refused", out.pixel_programme, out.vertex_description, out.perspective, out.camera_inside);
    if (!ok) { ++g_fail; return; }

    std::printf("vs b10 cbMatrixWorld (transposed VC_DecalTransform, VC_WidthScale):\n");
    for (int i = 0; i < 4; ++i) { char n[32]; std::snprintf(n, sizeof n, "VC_DecalTransform[%d]", i); PrintRow(n, out.vs_b10 + 4 * i); }
    PrintRow("VC_WidthScale", out.vs_b10 + 16);
    static const char* names[26] = {"ProjToDecalSpace[0]", "ProjToDecalSpace[1]", "ProjToDecalSpace[2]", "ProjToDecalSpace[3]",
        "InvProj[0]", "InvProj[1]", "InvProj[2]", "InvProj[3]", "CameraMatrix[0]", "CameraMatrix[1]", "CameraMatrix[2]", "CameraMatrix[3]",
        "ScreenSize (w h 1/w 1/h)", "DecalWorldRotation[0]", "DecalWorldRotation[1]", "DecalWorldRotation[2]", "EyeInDecalSpace",
        "ParallaxScale", "DecalSoakThreshold", "CubeCenterOffset", "DiffuseScale", "Reflect/ShininessScale", "LightInDecalSpace",
        "EmissiveIntensity", "NearStart/Recip FarStart/Recip", "NormalThreshold (start recip)"};
    std::printf("ps b10 cbDecalParam:\n");
    for (int i = 0; i < 26; ++i) { char n[48]; std::snprintf(n, sizeof n, "[%2d] %s", i, names[i]); PrintRow(n, out.ps_b10 + 4 * i); }
    std::printf("ps b2 cbMatDynParam (last float4, then the tail past the 80 bytes):\n");
    PrintRow("[4] (+25C, 0, +260, +264)", out.ps_b2 + 16);
    PrintRow("tail (+268, 1, 1, 1)", out.ps_b2_tail);

    const double hx = Rf(d, 0x140), hy = Rf(d, 0x144), hz = Rf(d, 0x148);
    const double P[4] = {Rf(d, 0x180), Rf(d, 0x184), Rf(d, 0x188), 1.0};
    const double ws = Rf(d, 0x244), d238 = Rf(d, 0x238), d23c = Rf(d, 0x23c);
    const bool flip = d[0x259] != 0;
    const double flip_sign = flip ? -1.0 : 1.0;
    const bool persp = out.perspective;

    // (a)
    {
        const double pos[4] = {0, 0, 0, 1};
        double clipC[4]; Dp4(out.vs_b10, pos, clipC, 4);
        double clipV[4]; VecMat(P, in.view.view_projection, clipV);
        double err = std::fabs(clipC[3] - clipV[3]) / std::fabs(clipV[3]);
        for (int k = 0; k < 3; ++k) err = std::fmax(err, std::fabs(clipC[k] / clipC[3] - clipV[k] / clipV[3]));
        Expect(err < 1e-3, "(a) VC_DecalTransform * centre == world position * view-projection (ndc xyz and w)", err, 1e-3);
    }

    // PS side: (ndc.xy, depth, 1) of a VS clip position -> decal space (what the PS does with FC_ProjToDecalSpace)
    const auto decal_space = [&](const double* clip, double* ds) {
        const double in4[4] = {clip[0] / clip[3], clip[1] / clip[3], clip[2] / clip[3], 1.0};
        double r[4]; Dp4(out.ps_b10, in4, r, 4);
        ds[0] = r[0] / r[3]; ds[1] = r[1] / r[3]; ds[2] = r[2] / r[3];
    };
    // VS: the unit-cube position (x, y, z) -> clip, with the far-end widening
    const auto vs_clip = [&](double x, double y, double z, double* clip) {
        const double s = (y + 1.0) * 0.5;
        const double pos[4] = {(s * (out.vs_b10[16] - 1.0) + 1.0) * x, y, (s * (out.vs_b10[17] - 1.0) + 1.0) * z, 1.0};
        Dp4(out.vs_b10, pos, clip, 4);
    };
    // expected decal space of a unit-cube point: box K maps [-1,1] -> [0,1]; pyramid: see the header comment
    const double L = persp ? 2.0 * hy / (ws - 1.0) : 0.0;
    const auto expected = [&](double x, double y, double z, double* e) {
        if (!persp) { e[0] = 0.5 + 0.5 * flip_sign * x; e[1] = 0.5 + 0.5 * y; e[2] = 0.5 + 0.5 * z; return; }
        const double n = L, f = L + 2.0 * hy, yp = L + (y + 1.0) * hy;
        e[0] = 0.5 + 0.5 * flip_sign * x * (2.0 * hx / d238);
        e[1] = f / (f - n) * (1.0 - n / yp);
        e[2] = 0.5 + 0.5 * z * (2.0 * hz / d23c);
    };

    // (b)
    {
        double clip[4]; vs_clip(0, 0, 0, clip);
        double ds[3], ex[3]; decal_space(clip, ds); expected(0, 0, 0, ex);
        double err = 0; for (int k = 0; k < 3; ++k) err = std::fmax(err, std::fabs(ds[k] - ex[k]));
        std::printf("  centre in decal space: (%.5f, %.5f, %.5f), expected (%.5f, %.5f, %.5f)%s\n", ds[0], ds[1], ds[2], ex[0], ex[1], ex[2],
                    persp ? "  [perspective: axis coordinate = f/(f-n)*(1-n/y')]" : "");
        Expect(err < 5e-3, "(b) FC_ProjToDecalSpace * clip(centre) == decal-space centre", err, 5e-3);
    }
    // (c)
    {
        double err = 0;
        const double xs[3] = {-1, 0, 1}, ys[5] = {-1, -0.5, 0, 0.5, 1}, zs[2] = {0.0, 0.5};
        for (double x : xs) for (double y : ys) for (double z : zs) {
            if (!persp && std::fabs(ws - 1.0) > 1e-6) continue;   // a box decal with a width scale != 1 is not a case the shader maps
            double clip[4], ds[3], ex[3];
            vs_clip(x, y, z, clip); decal_space(clip, ds); expected(x, y, z, ex);
            for (int k = 0; k < 3; ++k) err = std::fmax(err, std::fabs(ds[k] - ex[k]));
        }
        Expect(err < 5e-3, "(c) widened unit-cube points through the VS then the PS mapping == analytic decal-space coordinates", err, 5e-3);
    }
}

// Byte-for-byte comparison against the constant buffers the original uploaded for this decal's draw (i66-reference-2134 rows
// carry vs_cb10 / ps_cb10 / ps_cb2). Reports, per float4, max abs and relative error and the number of bit-different floats.
struct CompareStats { int float4s = 0, bit_different = 0, beyond_tol = 0; double max_abs = 0, max_rel = 0; };

void CompareBuffer(const char* what, const float* mine, const std::vector<uint8_t>& orig, int first_float, int float_count,
                   const char* const* names, bool verbose, CompareStats& total) {
    CompareStats rowstats;
    for (int f = first_float; f < first_float + float_count; f += 4) {
        double max_abs = 0, max_rel = 0; int bitdiff = 0;
        for (int k = 0; k < 4; ++k) {
            float o; std::memcpy(&o, orig.data() + 4 * (f + k), 4);
            const float m = mine[f + k];
            uint32_t ob, mb; std::memcpy(&ob, &o, 4); std::memcpy(&mb, &m, 4);
            if (ob != mb) ++bitdiff;
            const double a = std::fabs(double(o) - double(m));
            const double scale = std::fmax(std::fabs(double(o)), std::fabs(double(m)));
            const double rel = scale > 0 ? a / scale : 0.0;
            if (ob != mb) { max_abs = std::fmax(max_abs, a); max_rel = std::fmax(max_rel, rel); }
            if (ob != mb && rel > 1e-5 && a > 1e-30) ++rowstats.beyond_tol;
            if (verbose && ob != mb)
                std::printf("      %s[%d].%d: original % .9g (%08X)  native % .9g (%08X)  rel %.3g\n", what, f / 4, k, o, ob, m, mb, rel);
        }
        ++rowstats.float4s; rowstats.bit_different += bitdiff;
        rowstats.max_abs = std::fmax(rowstats.max_abs, max_abs); rowstats.max_rel = std::fmax(rowstats.max_rel, max_rel);
        if (bitdiff)
            std::printf("    %s float4 %2d (%s): %d bit-different, max abs %.3g, max rel %.3g\n", what, f / 4, names ? names[f / 4] : "", bitdiff, max_abs, max_rel);
    }
    total.float4s += rowstats.float4s; total.bit_different += rowstats.bit_different; total.beyond_tol += rowstats.beyond_tol;
    total.max_abs = std::fmax(total.max_abs, rowstats.max_abs); total.max_rel = std::fmax(total.max_rel, rowstats.max_rel);
}

const char* const kCbNames[26] = {"ProjToDecalSpace[0]", "ProjToDecalSpace[1]", "ProjToDecalSpace[2]", "ProjToDecalSpace[3]",
    "InvProj[0]", "InvProj[1]", "InvProj[2]", "InvProj[3]", "CameraMatrix[0]", "CameraMatrix[1]", "CameraMatrix[2]", "CameraMatrix[3]",
    "ScreenSize", "DecalWorldRotation[0]", "DecalWorldRotation[1]", "DecalWorldRotation[2]", "EyeInDecalSpace (unused)",
    "ParallaxScale", "DecalSoakThreshold", "CubeCenterOffset", "DiffuseScale", "Reflect/ShininessScale", "LightInDecalSpace",
    "EmissiveIntensity", "NearStart/Recip FarStart/Recip", "NormalThreshold"};
const char* const kVsNames[5] = {"DecalTransform[0]", "DecalTransform[1]", "DecalTransform[2]", "DecalTransform[3]", "WidthScale"};

CompareStats g_vs_stats, g_ps_stats, g_ps2_stats;

void CompareRow(const Sample& s, int index, bool verbose) {
    DecalConstantInput in;
    if (!BuildInput(s, in)) return;
    DecalConstants out;
    const bool ok = ComputeDecalConstants(in, out);
    const std::vector<uint8_t> vs = FromHex(s.vs_cb10), ps = FromHex(s.ps_cb10), ps2 = FromHex(s.ps_cb2);
    std::printf("--- oracle comparison, row %d: %s, pixel programme %d, vertex description %d, perspective %d, camera inside %d\n", index,
                ok ? "drawn" : "refused", out.pixel_programme, out.vertex_description, out.perspective, out.camera_inside);
    if (!ok || vs.size() < 80 || ps.size() < 416 || ps2.size() < 80) { std::printf("  (no comparison)\n"); return; }
    CompareBuffer("vs_b10", out.vs_b10, vs, 0, 20, kVsNames, verbose, g_vs_stats);
    CompareBuffer("ps_b10", out.ps_b10, ps, 0, 104, kCbNames, verbose, g_ps_stats);
    CompareBuffer("ps_b2 ", out.ps_b2, ps2, 16, 4, nullptr, verbose, g_ps2_stats);
}

void PrintDecoded(const Sample& s) {
    DecalConstantInput in;
    if (!BuildInput(s, in)) return;
    DecalConstants out;
    ComputeDecalConstants(in, out);
    const std::vector<uint8_t> ps = FromHex(s.ps_cb10);
    std::printf("=== decoded rows of the first oracle row (native | original), camera_inside %d, pixel programme %d, vertex description %d\n",
                out.camera_inside, out.pixel_programme, out.vertex_description);
    for (int r : {17, 20, 24, 25}) {
        std::printf("  [%2d] %-32s native", r, kCbNames[r]);
        for (int k = 0; k < 4; ++k) std::printf(" % .9g", out.ps_b10[4 * r + k]);
        std::printf("   original");
        for (int k = 0; k < 4; ++k) std::printf(" % .9g", Rf(ps, 16 * r + 4 * k));
        std::printf("\n");
    }
}
}  // namespace

int main(int argc, char** argv) {
    SelfTests();
    int index = 0;
    bool verbose = false;
    if (argc > 1) {
        // decal_constants_test.exe startup.jsonl [-v]   (rows of i66-reference-2134 or later also carry the original's buffers)
        verbose = argc > 2 && std::strcmp(argv[2], "-v") == 0;
        g_screen_w = 1280; g_screen_h = 720;   // the screen size of that run (cb10 row 12 of the oracle: 1280, 720, 1/1280, 1/720)
        std::ifstream f(argv[1]);
        std::string line;
        bool first = true;
        while (std::getline(f, line)) {
            if (line.find("\"decal_oracle\"") == std::string::npos) continue;
            Sample s;
            if (!Extract(line, "decal_bytes", s.decal) || !Extract(line, "view_bytes", s.view) || !Extract(line, "manager_400", s.manager) || !Extract(line, "batch", s.batch)) continue;
            ++index;
            if (Extract(line, "vs_cb10", s.vs_cb10) && Extract(line, "ps_cb10", s.ps_cb10) && Extract(line, "ps_cb2", s.ps_cb2)) {
                if (first) { PrintDecoded(s); first = false; }
                CompareRow(s, index, verbose);
            }
            if (verbose) Run(s, index);
        }
        const auto line_of = [](const char* n, const CompareStats& c) {
            std::printf("  %-6s %3d float4: %d bit-different floats, %d beyond 1e-5 relative, max abs %.3g, max rel %.3g\n", n, c.float4s, c.bit_different, c.beyond_tol, c.max_abs, c.max_rel);
        };
        std::printf("\noracle totals over %d rows:\n", index);
        line_of("vs_b10", g_vs_stats); line_of("ps_b10", g_ps_stats); line_of("ps_b2", g_ps2_stats);
        g_fail += g_ps_stats.bit_different + g_vs_stats.bit_different + g_ps2_stats.bit_different;   // bit identity is the bar
    } else {
        Run(Sample{kDecalHex, kViewHex, kManagerHex, kBatchHex}, ++index);
    }
    InsideTests(Sample{kDecalHex, kViewHex, kManagerHex, kBatchHex});
    std::printf("\n%d checks passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
