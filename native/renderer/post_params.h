// I-0138 tranche 1 (ROADMAP workstream 11): the parameter block of our tone-map pass (PostToneStage, shaders/post_tone.hlsl), in
// place of the YEBIS library's `$Globals` block. One constant buffer of our own, the layout of the programme's cbuffer; built from
// the blended Tone Map record by world/post_tone.h.
#pragma once
#include <cstdint>

namespace ds3native {
// I-0138 tranche 5: where the HDR entries leave the SDR picture: our tone map's value (the largest channel, 0 to 1) above which
// the highlights are continued (hdr::Highlight). 0.75: three quarters of the paper white in linear light.
inline constexpr float kPostToneHdrKnee=0.75f;
// I-0138 tranche 5e (Thomas, 2026-11-14, D-0280): the game's lower bound of the automatic exposure's factor (log2 a): it never
// darkens under the area's own exposure (the full-strength test tool included); the stage's own default bounds stay wide (its
// unit tests').
inline constexpr float kAutoExposureMinLog2=0.0f;
struct PostToneParams {
    float colour[3][4]{};          // the colour matrix's rows: c[i] = dot((y, 1), colour[i])
    float alpha_weights[4]{};      // the output alpha's weights on the scene texel
    float vignette_uv[3][4]{};     // .xy: the vignette's coordinates at the pass's three vertices
    float exposure{};              // the record's Exposure after the library's guard
    float curve_a{},curve_b{};     // the curve's constants: the sensitometric curve's two (the record's FilmicContrast through
                                   // yebis::TonemapCurve), or the Reinhard curve's c208.x and .y (I-0138 part 4c)
    float inv_gamma{};             // 1 / gamma after the guards
    float glare_scale{};
    float dither_scale{},dither_offset{};
    uint32_t flags{};              // PostToneFlags
    float lut_scale{},lut_offset{};   // (N - 1) / N and 1 / 2N of the N x N x N LUT
    float curve_c{};               // the Reinhard curve's c208.z (1 / F^2); 0 for the sensitometric curve
    float hdr_knee{kPostToneHdrKnee};   // the HDR entries' highlights' knee (I-0138 tranche 5; hdr_output.h: Highlight)
    // I-0138 tranche 6 (the candidate looks' entries only; the original's programmes read none of them):
    float look_scale{1.0f};        // on the exposed scene before a candidate's curve (tone_looks.h: the middle grey matched)
    float look_a{3.0e38f},look_b{}; // the candidates' HDR entries: the grey scene value (after look_scale) shown at hdr_knee and the
                                   // candidate's slope there (tone_looks.h: KneeTangent); the defaults: no gain
    float look_c{};                // the candidate after Elden Ring: its curve's contrast
    float er_dispersion[4]{};      // that candidate's lateral dispersion: the green channel's share of 2 uv - 1 (.xy), the blue's (.zw)
    float er_lift[4]{1.0f,0.0f,0.0f,0.0f};   // I-0138 tranche 7: that candidate's midtone lift, .x the power on its curve's output (1: none);
                                   // tranche 7b, the vivid variant (look::ErVivid, D-0296; shaders/post_tone.hlsl ErVivid): .y the contrast, .z the vibrance,
                                   // .w the cool black's tint (0: none; ApplyToneLook sets them for that candidate)
    // I-0138 tranche 7: the film grain (world/post_tone.h SetFilmGrain; shaders/post_tone.hlsl, Grain), every entry's:
    float grain[3]{0.0f,1.0f,0.0f};  // its strength on the display-encoded picture (0: none), its grid in pixels, unused
    uint32_t grain_seed{};         // the frame's seed
};
static_assert(sizeof(PostToneParams)==224);
// vignette: the record's (set with the block), the original's programmes and the ER candidate's body alike (I-0141 tranche 11,
// Thomas 2026-11-17: DS3's own vignette by area in the Rekindled look, ER's dropped); adapted: set by the stage when the frame binds the automatic exposure's factor.
// reinhard (I-0138 part 4c): the record's tone type 3, the library's `_REINHARD` programme's curve in place of the sensitometric one.
// hdr_lut (I-0138 tranche 5): set by the stage for an HDR draw with a LUT (read in that draw, no second one).
// The look (I-0138 tranche 6, bits 4 and 5: a ToneLook's number, 0 the original): the caller's, it picks the candidate's
// programme in place of the record's curve; no programme reads these bits.
// grain_later (I-0138 tranche 7): set by the stage when a LUT draw follows the tone map's (the grain added there, after the LUT).
enum PostToneFlags:uint32_t {kPostToneVignette=1u,kPostToneAdapted=2u,kPostToneReinhard=4u,kPostToneHdrLut=8u,kPostToneLookMask=0x30u,kPostToneGrainLater=0x40u};
inline constexpr uint32_t kPostToneLookShift=4u;   // the look's number in the flags: (flags & kPostToneLookMask) >> kPostToneLookShift
}
