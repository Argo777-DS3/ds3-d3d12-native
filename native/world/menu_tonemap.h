// I-0113 tranche 27: the GX tone map of the menu views (the offscreen views with pipeline flag bit 10 clear: the Quit Game and
// Load Game portraits, Status, character creation), YEBIS-CHAIN-CHECKLIST P23 as read on 2026-11-07 (GhidraMCP; I-0113 RESULT
// « Tranche 27 »). Per frame FUN_1400FB890 blends the Tone Map bank [GXRS+0x4618] for the view's key into the filter's block
// pipeline+0x3080.. and FUN_1401C48E0 draws, the auto exposure, bloom and 3D LUT off for these views (flags 0x8422014):
//   - the table writer FUN_1401C5800 (mode 0 WriteFilmicToneMappingTablePS, mode 7 WriteSensitometricToneMappingTablePS, any
//     other mode none) into a 1024x1 target of GX format 0x1C (INFERRED R8G8B8A8_UNORM, DXGI numbering);
//   - the final draw FUN_1401C61B0: with UseTable and mode 0 or 7, TabledToneMapPS: x = scene * e (e = 2^Exposure /
//     AvgLuminance, cbToneMap c3; the glare term t3 * c7.x is black with bloom off), y = x / (x + 0.2), out = Table(y) * Scaler
//     (c1), the table sampled SS_ClampLinear at (y, 0); the other modes are not ported (FilmicToneMapPS's formula not read).
// The record's second block in file order (the struct dwords [0x23..0x2F] of FUN_1400F8D70): Exposure, AvgLuminance,
// LuminanceBalance R/G/B, Scaler R/G/B, ToneMapMode, Contrast, Gamma, UseTable (default.gparam id 12, the Quit Game
// portrait's key {1, 12}: 2.8, 3, 1, 1, 1, 1, 1, 1, 0, 0.5, 1.8, 1).
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace ds3native::menu_tone {
struct Params {
    uint32_t mode{};float exposure{},avg_luminance{1.0f},contrast{0.5f},gamma{2.2f};
    std::array<float,3> scaler{1.0f,1.0f,1.0f};bool use_table{};
};
// One entry of the group in file order (light_producers' BankEntry without the dependency): its short name and first word.
struct Entry {std::string name;uint8_t type{};uint32_t word{};};
// The second block from the Tone Map group's entries: the parameters after the second « Exposure » (the first is YEBIS's).
// False (with the reason) when one is missing or not finite.
bool FromEntries(const std::vector<Entry>& entries,Params& out,std::string& refused);
// e = 2^Exposure / AvgLuminance (FUN_1401C48E0's call at 1401c4939); 0 when AvgLuminance is not above 0.
float Scale(const Params& p);
// The TabledToneMapPS path: mode 0 or 7 with UseTable.
bool Tabled(const Params& p);
// The table writer's value at u in (0, 1): mode 7 Sensitometric (A + Contrast (B^3 - A))^(1/Gamma), mode 0 Filmic
// (X (6.2 X + 0.5) / (X (6.2 X + 1.7) + 0.06))^(2.2/Gamma) with X = max(0.2 u / (1 - u) - 0.004, 0); A = 1 - 2^-X,
// B = 1 - 2^(-3.089127 X) with X = 0.2 u / (1 - u).
float TableValue(uint32_t mode,float u,float contrast,float gamma);
// The table entry i of 1024 (its texel centre (i + 0.5) / 1024), quantized to 8 bits (the INFERRED UNORM format).
float TableEntry(uint32_t mode,uint32_t i,float contrast,float gamma);
// TabledToneMapPS for one linear channel (the CPU reference of the native shader): Table(y) * scaler with the 1024-entry table
// read with linear filtering and clamp at u = y. Explicit policy (the step review's L3): the colour is clamped at 0 first, as
// the native composition always did; the original does not clamp (an x below -0.2 gives y above 1, the table's last entry),
// which matters only if the native lit HDR goes negative.
float Apply(const Params& p,float linear,uint32_t channel);
// I-0113 tranche 29: the menu view's background as the original draws it (GhidraMCP, I-0113 RESULT « Tranche 29 »): with
// pipeline+0x78 set (FUN_140D64C30, the menu views), FUN_140103510's plan key 0x1A order 1 binds the forward HDR group (RT and
// DS) and its closure FUN_1401072C0 draws a full-screen quad of FUN_140112b40(pipeline+0x6C) (depth bank +0x1E8: LESS_EQUAL, no
// write, from FORWARD-DEPTH-NATIVE §1.1), before the forward rows; the tone map (key 0x39) then covers every pixel, no depth
// test. +0x6C is the Setting Info « BG Color » packed to bytes by FUN_140087FB0 (clamped to 0..1, x 255; its rounding INFERRED
// nearest); FUN_140112b40 divides R, G, B by 255, squares them (alpha not), clamps, x 255, rounds to nearest (CVTPS2DQ) and
// repacks: 18/255 gives 1/255. Returns the quad's colour in 0..1.
// Inline: the renderer's objects use it without menu_tonemap.obj (startup_primitive_test links them alone).
inline std::array<float,4> BackgroundQuadColour(const std::array<float,4>& bg) {
    std::array<float,4> out{};
    for(size_t c=0;c<4;++c){const float v=std::isfinite(bg[c])?std::clamp(bg[c],0.0f,1.0f):0.0f;
        const float byte=std::nearbyint(v*255.0f);   // FUN_140087FB0's packing (rounding INFERRED nearest)
        if(c==3){out[c]=byte/255.0f;continue;}         // FUN_140112b40: alpha not squared
        const float n=byte/255.0f;out[c]=std::nearbyint(std::clamp(n*n,0.0f,1.0f)*255.0f)/255.0f;}
    return out;
}
// The offscreen composition's programme (VS, PS, BackgroundPS): native/renderer/shaders/offscreen_composition.hlsl, built by fxc
// (I-0132 R12h step 2), the HLSL of Apply() above for a tabled view.
}
