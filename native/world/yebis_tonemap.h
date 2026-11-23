// W10 under D-0175 (W10-NATIVE-PASSES 2): the constants our own tone-map pass gives the original programme
// `TonemapHDR_Dither_Exposure_Matrix_Gamma`, computed natively from the YEBIS globals and the Tone Map record the native feed
// writes (bit-identical to the original at V1), with the formulas of the executable; checked against the original's captured
// `$Globals` (Reference1632, m37).
#pragma once
#include <array>
#include <cstdint>
#include <functional>

namespace ds3native::yebis {
// The executable's data the formulas read: the 21-entry curve table at 142B60C40, 2.0891273 at 142B60CD0, 1/3 at 143D7EDA4,
// 1.0 at 143D7EE10, 20 at 143D7EF70.
// GL-4 (m50, ColorH 1): the colour matrix's data (E69A0 and the library's setters FUN_1416CBB90/BBF0/BC30): pi [143D7EEC0] and
// 180 [143D7EFE8] for the hue angle, the Rec. 709 weights [142B4D038/074/010] of the saturation matrix FUN_1416D1D90, the 13
// coefficients of the hue matrix FUN_1416DDB90 (Haeberli's rotation about the luminance axis).
// I-0113 tranche 20 (GhidraMCP, E69A0 1400e70fd..1400e7117 and the setters): the Tone Map's Contrust (bank dword 8, globals
// 1449621B8 = E69A0's param_3+0x248, FUN_1400E42A0) multiplies the matrix last through FUN_1416CBC90 -> 1416FC150: FUN_1416D20C0
// (c, c, c), diag(c, c, c, 1) with row 3 ((1 - c) half) x3, half = [143D7EDB4] 0.5; ColorV's FUN_1416CBC30 the diag(v, v, v, 1)
// before it. The guards of the library's tone setter 1416F71B0 (E69A0's FUN_1416CA450): the exposure max(e, [143D7ECDC] FLT_MIN)
// (COMISS, CMOVBE: zero, negative and NaN give FLT_MIN, a black scene term), the gamma max(g, 0) (NaN 0), and the draw
// FUN_141753F00 takes a zero gamma as [143D7EE90] 2.2.
struct ColorData {float pi{},half_turn{};std::array<float,3> luma{};std::array<float,13> hue{};float half{},min_exposure{},default_gamma{};bool ok{};};
// Reads them through read(rva, out, bytes) (the image or the executable file).
bool LoadColorData(ColorData& c,const std::function<bool(uint32_t,void*,size_t)>& read);
struct TonemapData {std::array<float,21> curve{};float tx_scale{},third{},one{},steps{};ColorData color{};};
struct TonemapInputs {float exposure{},gamma{},filmic_contrast{},color_h{},color_s{},color_v{},contrast{1.0f};   // contrast: the record's Contrust (entry 8)
    uint32_t tone_type{2};float mapping_factor{32.0f};};   // I-0138 part 4c: the record's ToneType (entry 2) and ToneMappingFactor (entry 3)
struct TonemapConstants {
    float exposure{};                      // afRGBA_Modulate[0].x (word 616)
    float gamma_correction{};              // fParam_GammaCorrection (word 808): 1 / gamma
    std::array<float,2> dither{};          // fParam_DitherOffsetScale (words 809, 810): 1/255, -0.5/255
    std::array<float,4> curve{};           // fParam_TonemapMaxMappingLuminance (words 832-835: .xy the sensitometric curve's, .zw 0; the four the Reinhard curve's)
    bool reinhard{};                       // I-0138 part 4c: tone type 3, the `_REINHARD` technique
    std::array<float,16> color_matrix{};   // m44_ColorTransformMatrix (words 840-855)
    bool matrix_supported{};               // false when the hue rotation, the value scale or the contrast is needed without the colour data
    bool guarded{};                        // false when the exposure or the gamma is not positive without the colour data (its guards)
};
// FUN_141743210: x clamped to [0, 1], f = x / (1 / steps), the table lerped between floor(f) and min(floor(f) + 1, 20).
float CurveTable(const TonemapData& d,float x);
// FUN_141718390 case 10: T.x = c^(1/3) x 2.0891273 + 1, T.y = CurveTable(c^(1/3)).
std::array<float,2> TonemapCurve(const TonemapData& d,float filmic_contrast);
// I-0138 part 4c: YebisTonemapCurveConstants_141718390 case 2 (tone type 3 is technique index 2, `_REINHARD`; FUN_1417432c0 leaves
// a and b as they are for it), a = F the record's ToneMappingFactor (YebisToneSetter_1416f71b0 with the library's +0x456 clear:
// +0x6F4 = F, +0x6F8 = 1), b = 1: r = a / b, k = 1 / r^2, c208 = (k x 4 - 2, 1 / b, k, a^2 / (b x 2)), single precision, each
// operation rounded as the SSE code does (MULSS, SUBSS, DIVSS; the lane order read on the disassembly). The constants 4, 2, 1
// are [143D7EED8], [143D7EE7C], [143D7EE10]. Static reading: evidence/I-0138/artifacts/tranche-4c-reinhard-static.md.
std::array<float,4> ReinhardCurve(float mapping_factor);
TonemapConstants BuildTonemap(const TonemapData& d,const TonemapInputs& in);

// W10 TM-2: the `$Globals` block our tone-map pass binds to b0 of the original programmes (vs_2D_Tex2D5 and
// ps_TonemapHDR_Dither_Exposure_Matrix_Gamma): 3584 bytes, the size of the original's block; every other word is zero.
// Words written: 20-21 fParam_ScreenSpaceScale (1, -1, the VS's position scale), 616-619 afRGBA_Modulate[0] (exposure on t0, 1 on
// the glare input, 1, 1), 620-623 afRGBA_Modulate[1] (0, 0, 0, 1: the output alpha is t0's), 744-759 afRGBA_Offset[0..3] (the
// adapted-exposure remaps of t2: (2, -1, 0, 0), (0.5, 0, 0, 0), (2, -1, 0, 1), (0.5, 0, 0, 0), which make t2's multiplier of t0 and t1 1 and its
// additive term 0 whatever t2 holds), 808-810 gamma correction and dither, 832-833 the curve, 840-855 the colour matrix.
// The Modulate[0].yzw, Modulate[1] and Offset words are the original's captured values at m37 (Reference1632 draw 45); their
// producer in the library is not read: they were captured in the manual-exposure case (Tone Map AutoExposure 0), the only one our pass serves
// (the frame input is refused under auto exposure, row native_yebis_tonemap).
inline constexpr uint32_t tonemap_globals_bytes=3584;
void TonemapGlobals(const TonemapConstants& t,uint8_t* bytes);
// The 4x4 RGBA8 dither texture of 141719770 (t3, sampler mode 13: point, wrap): the Bayer table at 142B60D30..142B60D6F loaded in
// the row order D60, D40, D50, D30, each value v -> v x [143D7ED5C] (1/16) + [142A5F3CC] (1/32), x [142B60CE4] (255.9999), truncated:
// byte 16 v + 7 (the product is just under 16 v + 8 and truncates); texel (r, c) = (v[r][c], v[r][0], v[r][0], v[r][c]), the
// interleave of 141719770, equal to the original's texture captured at m37 (i66-reference-1637). The PS reads only the red channel.
std::array<uint8_t,64> TonemapDither(const std::array<float,16>& bayer_rows,float scale,float bias,float to_byte);
// W10 TM-3, the vignette variant (`TonemapHDR_Dither_Vignette_Exposure_Matrix_Gamma`, drawn where the Tone Map record's Vignette
// Power, word 26, is positive: m31, m45, m50): the PS darkens by (1 - |uv2|^2)^4 (saturated), uv2 the vertex TEXCOORD2, whose
// rectangle at the screen corners FUN_141751A60 computes (the power path, +0x16C = 0):
//   v = (tan(pi/8) (1 - d) + d fov) power      tan(pi/8): [142A66FD0] = pi/8 through FUN_141FD59E0; d = VignetteFovDependence
//                                                 (+0x168), power = Vignette Power (+0x164), fov = the glare object's +0x138
//   h = v ((A / k) / (m / k))                   FUN_141712A60 of the glare object's +0x108, +0x10C (FUN_1417524B0 from CPostEffect
//                                                 ::Initialize: A the render aspect +0x328, k = +0x32C 4/3 [0x3FAAAAA8] and m = +0x388
//                                                 2/3 [0x3F2AAAE3], the init-flag bit 29 and 30 values)
//   rect = (-h - ox, -v - oy, h - ox, v - oy)   ox, oy = (2h) +0x13C / 2, (2v) +0x140 / 2: the lens offsets, 0 here
// I-0113 tranche 20: the fov term (d != 0, the cutscene records) served: +0x138 is tan(+0x134 / 2), the library's S+0x570 from
// E69A0's camera field of view (W10 LS-2: the shafts read the same two words), natively tan(vertical fov / 2) of the frame's camera
// (FUN_141751A60 read again in GhidraMCP: (fVar3 (1 - d) + d [+0x138]) power, the same operation order).
struct VignetteData {float tan_eighth_pi{},k{},m{};};
std::array<float,4> VignetteRect(const VignetteData& d,float power,float aspect,float fov_dependence=0.0f,float tan_half_fov=0.0f);
// The glare object's +0x138 from the camera's vertical fov, for the vignette and the shafts alike: tan(fov / 2) (the CPU's tanf,
// bit-identical to the captured +0x138 at the shaft captures' gameplay fovs, yebis_shafts_test), tan(pi/8) when it is not
// positive (FUN_141753790: COMISS 0, t; JC keeps t, so a NaN stays NaN, as here; the tone map then refuses a vignette whose d != 0).
float VignetteFovTerm(const VignetteData& d,float vertical_fov);
// The TEXCOORD2 of the three vertices of the draw (uv0 (0, 1), (0, -1), (2, 1)): rect.xy + uv0 (rect.zw - rect.xy).
std::array<std::array<float,2>,3> VignetteCoordinates(const std::array<float,4>& rect);
}
