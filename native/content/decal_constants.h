#pragma once
#include <cstdint>

namespace ds3content {
// Deferred "box" decals (gxdecal, I-0068 DECALS-NATIVE): the CPU computation of the constant buffers that the original
// per-decal draw function FUN_140198910 uploads (VS b10 cbMatrixWorld 80 B, PS b10 cbDecalParam 416 B, PS b2 cbMatDynParam
// 80 B), plus the pixel / vertex programme selection and the "camera inside the decal volume" flag. Pure CPU: no engine
// access. The original pixel and vertex programmes stay the game's own DXBC; only these constants are produced natively.
//
// Conventions of the original (all kept): matrices are 16 floats, row-major, row-vector convention (v' = v * M); the
// buffers hold the TRANSPOSE of the CPU matrices (so the shader's dp4 rows are the CPU matrix columns).
//
// Out of scope here: PS b7 (cbModelParam) and PS b11 (cbCubeEnvSH1), both produced by FUN_140059470 over a draw state
// built by FUN_1401a8580 from the decal's material object (+0x270); PS b3 (the frame's global block
// [[144961B50]+0x4DD0]+0x280); texture, blend, depth-stencil and raster state; the optional stencil pre-pass (manager+0x511,
// off by default) and its depth ratio (manager+0x510 branch of 198910).

constexpr unsigned kDecalObjectSize = 0x420;     // the decal object (vtable 3D5B8D8): only bytes the draw reads matter
constexpr unsigned kDecalManagerBytes = 0x30;    // manager+0x400 .. +0x42F

// The view context 198910 receives as param_3, expressed as the values it reads. Matrices: 16 floats, row-major.
struct DecalView {
    float world_to_view[16]{};      // view+0x10..+0x4F
    float projection[16]{};         // view+0x50..+0x8F (left-handed perspective, row-vector convention)
    float view_projection[16]{};    // view+0x90..+0xCF (not read by the constants; 198910 reads it only for the pre-pass depth ratio)
    float camera_world[16]{};       // view+0xD0..+0x10F: rows 0-2 camera axes (w = 0), row 3 (+0x100) camera position, w = 1
    float fov = 0, near_plane = 0, far_plane = 0, aspect = 0;   // view+0x270..+0x27C (14006EF80 reads the same four through view+0x130+0x140..)
    uint32_t screen_width = 0, screen_height = 0;               // [view+0x418]+0x5C / +0x60 (cb[12] only)
};

// The animated emissive curve: the result of FUN_140146850(*(*(decal+0x130)+8), 0, 4) when the decal has a curve
// object (+0x130 != 0) and an emissive texture (+0x1B8 != 0). The curve value is its first float, the weight its sixth.
struct DecalEmissiveCurve {
    bool found = false;
    float value = 0, weight = 0;
};

struct DecalConstantInput {
    uint8_t decal[kDecalObjectSize]{};        // the decal object bytes (pointer fields only need to be zero / non-zero)
    uint8_t manager_400[kDecalManagerBytes]{};  // manager+0x400..+0x42F: floats +0x40C near fade start, +0x410 near fade range,
                                                // +0x414 far fade start, +0x418 far fade range, +0x41C / +0x420 normal-fade angles
    bool batch_emissive = false;              // the batch's anyEmissive byte, *(byte*)(batch + 8)
    bool material_present = true;             // decal+0x270 != 0 (198910 returns otherwise)
    float material_scale[4]{1, 1, 1, 1};      // the material object's floats +0xD0..+0xDC (diffuse r g b scale, opacity scale)
    DecalEmissiveCurve emissive_curve;
    DecalView view;
    int table_variant = 0;                    // the global at 143D5B7B0 (0); selects the programme tables (47 / 14 entries per variant)
};

struct DecalConstants {
    float vs_b10[20]{};      // cbMatrixWorld: [0..15] VC_DecalTransform (transposed), [16..19] VC_WidthScale = (w, w, 0, 0)
    float ps_b10[104]{};     // cbDecalParam, 26 float4 (see decal_constants.cpp for the row table)
    float ps_b2[20]{};       // cbMatDynParam (80 B): 198910 only writes [16..19] = (+0x25C, 0, +0x260, +0x264); [0..15] are not written
    float ps_b2_tail[4]{};   // (+0x268, 1, 1, 1): the 5th float4 198910 builds just past the 80 bytes (not uploaded)
    int pixel_programme = -1;    // iVar36: 0..46, index in the 47-entry name table (14473C860)
    int vertex_description = -1; // iVar37: 0..13, index in the 14-entry description table (14473C7F0); also the material-CB slot
    int pixel_table_index = -1;  // table_variant * 0x2F + pixel_programme  (manager+0x98 table)
    int vertex_table_index = -1; // table_variant * 0x0E + vertex_description (manager+0x210 table and manager+0x280 buffers)
    bool camera_inside = false;  // low byte of fStack_ea4: the camera's near plane touches the decal volume (depth state 0, raster mode 2)
    bool perspective = false;    // cStack_ea7: not cubic and width scale (+0x244) > 1: the volume is a truncated pyramid
};

// Faithful transcription of FUN_140198910's constant arithmetic (operation order and float32 kept). Returns false when
// 198910 would return before drawing (no material object, no diffuse texture). The "camera inside" flag is the one
// exception to bit fidelity: the original runs a chain of SIMD intersection helpers (1417B5A60, 1417BAD70, ...); this
// uses an exact geometric intersection (convex clip) of the same camera near-plane quad against the same volume.
bool ComputeDecalConstants(const DecalConstantInput& in, DecalConstants& out);

// Helpers exposed for the tests / the draw code.
void DecalMatMul(const float* a, const float* b, float* out);   // FUN_14001CE40: out = a * b
void DecalMatInverse(const float* in, float* out, float* det4 = nullptr);   // FUN_14004BCA0 (adjugate / determinant)
float DecalCos(float angle);                                    // FUN_141778230 (the thunk target the normal fade uses)
}
