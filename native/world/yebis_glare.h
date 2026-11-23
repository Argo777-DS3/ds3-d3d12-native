// W10 GL-2 (D-0175, W10-NATIVE-PASSES 5): the glare family's draws and constants, computed natively from the library's globals
// (written by the native feed's converters, bit-identical to the original's at V1) and the extent, with the library's formulas read
// in CRenderGlare (FUN_1417538E0 the frame, FUN_14174BED0/FUN_14174B830/FUN_141736FE0/FUN_141722480 the source and bright pass,
// FUN_14174C160 the pyramid, FUN_14173FDF0/FUN_14173DB50 the blurs with FUN_1416DC220/FUN_1416DB880 their weights, FUN_14174C9E0 the
// composite, FUN_141751DE0 its luminance, FUN_141749790 the level surfaces). Checked word for word against the original's captured
// blocks (Reference1648 m37) by yebis_glare_test; the library's own state at that frame (Reference1649) fixes the intermediate fields.
#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace ds3native::yebis {
// The executable's words the formulas read.
struct GlareData {
    float luminance_scale{};            // [142899374] 0.035 (FUN_141751DE0)
    float radius_c{};                   // [143D7EE90] 2.2 (C; 1.8 [142836360] under the alternate flag +0xEE, not served)
    float level_radius{};               // [143D7EDE8] 0.75 (its square root scales the radius of the levels after the first)
    std::array<float,3> wavelength{};   // [142B63170] red 615e-9, [142B6316C] green 540e-9, [142B63168] blue 465e-9
    float two_pi{};                     // [143D7EEFC]
    float sigma_floor{};                // [1427D5950] 1e-8: a radius at or below it is replaced by [142B4CF88]
    float sigma_replacement{};
    float base_ceiling{};               // [143D7EE00] 0.99
    float height_rounding{};            // [142B55250] 0.499999
    float up_offset{};                  // [142B60CA0] 0.375: the four taps of the 2x upsample at +-0.375 source texel
    std::array<float,8> quality_scale{};// 14469B4D0: {scale, levels} per glare quality, scale words
    float glare_aspect{};               // CPostEffect +0x350: 1.5 (init flag bit 29)
    // I-0138 tranche 8f: GlareShape 3's entry of the library's shape table (14469BD70 + 3 x 0x38, after its name: d0 to d10, d5 the
    // preset's index) and that preset's row (14469BC90 + d5 x 0x20, after its name: the arm count, a, the length, the odd arms'
    // length, the flag); FUN_141748760 makes the shape record from them (yebis::ShapeRecord).
    std::array<uint32_t,11> shape3{};std::array<uint32_t,5> preset3{};
};
// I-0138 tranche 8f: the shape record CRenderGlare +0x150 points at (FUN_141748760, READ: evidence/I-0138/t8d-readings/
// glare-shape3-passes.md section 0), the fields the shape-3 draws read; its offset in the record named.
struct GlareShapeRecord {
    float bloom{},scale{},tint{};       // +0x08 d0, +0x0C d1, +0x10 |d7| x 0.8 (the bloom combine's colour cycle)
    int tint_levels{};                  // +0x14: 4
    float ghost{},ghost_scale{},ghost_size{};   // +0x18 d2, +0x1C 1.0, +0x20 d3
    bool lens_flare{};                  // +0x24: d3 >= 0.05 (the ghost's lens-flare mode, not served)
    float streak{},angle{},colour{};    // +0x28 d4, +0x2C d6, +0x30 d7 (x 0.36 when |d7| >= 1)
    bool colour_flag{};                 // +0x34: |d7| >= 1 (the cross filters' mode, not served)
    float after_time{},after_weight{};  // +0x38 d8, +0x3C d9
    int arms{};float arm_a{},arm_length{},odd_length{};bool preset_flag{};   // the preset: +0x50, +0x54, +0x58, +0x5C, +0x60
};
GlareShapeRecord ShapeRecord(const GlareData& d);   // GlareShape 3's
// I-0138 tranche 8f: the ghost's trim texture (CRenderGlare +0x268, 64x64 A8_UNORM, one level), filled on the CPU by
// CRenderGlare_GenerateGlare_Source_14174b2d0 through 141710bd0 and 141744420 (READ, evidence/I-0138/t8f-readings/
// trim-circle-reading.md): 1 inside half the radius, 0 outside it, a linear ramp between, rows of 64 bytes.
// Inline (the module, the stage and its tests take it without the plan builder's object); /fp:precise contracts nothing (as
// yebis_glare.cpp's fp_contract off), so every unit computes the same texels.
inline std::array<uint8_t,64*64> TrimCircle(){
    // CRenderGlare_GenerateGlare_Source_14174b2d0's loop (14174b6b6..b796, READ: evidence/I-0138/t8f-readings/trim-circle-reading.md):
    // u = (x + 0.5) / 64; 141710bd0 with the scale s = 1/64: t = ((u - s 0.5) / s) / (1 / s - 1) x 2 - 1, r = sqrt(ty^2 + tx^2); 1 to
    // r = 0.5, then (1 - r) / 0.5 to 0 at r = 1; 141744420: 255 at 1 and above, 0 at or below 0, else trunc(v x 255.99989) (0x437FFFF9).
    std::array<uint8_t,64*64> texels{};const float s=0.015625f,inv=1.0f/s;const uint32_t k_bits=0x437FFFF9u;float k;std::memcpy(&k,&k_bits,4);
    for(uint32_t y=0;y<64;++y)for(uint32_t x=0;x<64;++x){
        const float u=(float(x)+0.5f)*s,v=(float(y)+0.5f)*s;
        const float tx=(((u-s*0.5f)*inv)/(inv-1.0f))*2.0f-1.0f,ty=(((v-s*0.5f)*inv)/(inv-1.0f))*2.0f-1.0f;
        const float r=std::sqrt(ty*ty+tx*tx);
        const float value=r<=0.5f?1.0f:r>=1.0f?0.0f:(1.0f-r)/(1.0f-0.5f);
        texels[size_t(y)*64+x]=value>=1.0f?uint8_t(255):!(0.0f<value)?uint8_t(0):uint8_t(int(value*k));}
    return texels;}
// The library's globals the glare reads (block 144961F70) and the extent.
struct GlareInputs {
    uint32_t width{},height{};          // the scene extent (the library's +0xA8, +0xAC)
    int quality{};                      // [144961FB8] GlareQuality (the base scale)
    float exposure{};                   // [144962008] the bright pass's scene factor (+0x144 = exposure / 1, the manual case)
    float factor{};                     // CRenderGlare +0x114 (FUN_141752BB0: +0x2BC x +0x2CC for the R11G11B10 pyramid, 64 x 1)
    float luminance{};                  // [14496204C] (+0x118)
    float threshold{};                  // [144962050] GlareThreshold (+0x11C)
    int levels{};                       // [144962054] GlareNumLevels (+0x2B8)
    float radius_scale{};               // [144962068] GlareBloomGaussianRadiusScale (+0x2E4)
    float composite_gamma{};            // [14496206C] GlareBloomLuminanceGamma (+0x1B8: the composite's exponent is its inverse)
    float bloom_gamma{};                // CRenderGlare +0x1B0 (CPostEffect +0xC2C; 1 at m37: the levels' weight and the L factor)
    float remap{};                      // [1449621B4] RemapFactor (+0x124, remap mode 3)
    float sampling_threshold{};         // [144962274] GlareSamplingThreshold (+0x2E0: the blur tap count)
    float shape_bloom{},shape_scale{},shape_tint{};   // the shape record [+0x150] +8, +0xC, +0x10 (1, 1, 0 for GlareShape 1)
    // GL-4: the vignette of the bright pass (CRenderGlare +0x160, set where the Tone Map record's Vignette Power is positive: m31,
    // m45, m50): BrightPassT1_Vignette with TEXCOORD1 the tone map's vignette coordinates (yebis::VignetteCoordinates, the captured
    // m50 vertices hold the same words as its tone-map draw's TEXCOORD2).
    bool vignette{};std::array<std::array<float,2>,3> vignette_uv{};
    // I-0138 tranche 8f: GlareShape 3 (« Standard »), the bloom with four-arm streaks, a ghost and an afterimage; 1 the bloom alone.
    int shape{1};GlareShapeRecord record{};   // the record of a shape other than 1 (shape 1's words are shape_bloom, shape_scale, shape_tint)
    float star_threshold{};             // +0x2E8: globals +0xEC GlareStarFilterThreshold (the streaks' pass count)
    float star_fov{};                   // +0x1B4: globals +0xF0 GlareStarLengthFovDependence (the streaks' length)
    float star_softness{};              // +0x2EC: globals +0xF4 GlareStarSoftness (the streaks' source mix)
    float ghost_distortion{};           // +0x1A8: globals +0x100 GlareGhostConcentricDistortion (the ghosts' exponents)
    float tan_half{};                   // +0x138: the streaks' length factor
    float ev{};                         // +0x15C: the arms' angle and the ghosts' cosines
    int base_level{};                   // +0x2F8: the ghost's and the colour cycle's first level
    float frame_time{};                 // +0x158 (CPostEffect +0x694): the afterimage's decay
    bool afterimage_skip{};             // CPostEffect +0x51B: the afterimage not drawn (its history neither written nor flipped); not read natively yet (false)
};
enum class GlareKind : uint8_t {Downsample,BrightPass,Blur,Upsample,Composite,Mix,Streak,StreakMax,Ghost,Afterimage,Layers};
// A surface of the plan: half_float RGBA16F (the source chain and the bright pass), else R11G11B10 (the pyramid).
struct GlareSurface {uint32_t width{},height{};bool half_float{};};
// I-0138 tranche 8b: the family's constant block in a layout of our own. The library's block is 228 float4 (c0..c227, 4,096 bytes
// as uploaded) of which our programmes (shaders/post_glare.hlsl) read 91 (73 before tranche 8f's ghost); ours holds those alone, in the order of the library's
// registers they stand for: slot i is the library's register glare_block_registers[i], and post_glare.hlsl's packoffsets are the
// slots. The builders keep the library's word numbers (their transcription of its writes: word w is register w / 4, component
// w % 4) and PutGlareWord places each word in its slot, so that the word-for-word tests against the original's captured blocks
// read our block back through the same table (frame/glare_library_block.h).
inline constexpr std::array<uint8_t,91> glare_block_registers{
    0,5,10,15,17,                                           // depth decode, screen scale, shaft grain, shaft mask (or the motion blur's planes), DoF factor
    19,20,21,22,23,24,25,                                   // the DoF composite's back levels
    34,35,36,37,38,39,40,41,                                // the first eight taps' offsets (the ghosts' centres)
    42,43,44,45,46,47,48,49,                                // I-0138 tranche 8f: the ghosts' offsets (afUV_TexCoordOffsetV16's second half)
    50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,     // taps 8 to 24's offsets (and the DoF table's row; the ghosts' exponents and scales)
    67,68,                                                  // the last two ghosts' distortion offsets
    146,147,148,149,150,151,152,153,                        // the ghosts' scales (afParam_TexCoordScaler8)
    154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,   // the weights
    186,187,188,189,190,                                    // Alpha_R's offset (or the motion blur's decode, step, range, test, rectangle)
    202,209,                                                // the composite's gamma, the bright pass's remap
    224,225,226,227};                                       // the motion blur's previous-clip transform
inline constexpr uint32_t glare_block_bytes=uint32_t(glare_block_registers.size())*16;   // 1,456 bytes (the library's: 4,096)
inline constexpr uint32_t glare_library_registers=228;
// The table strictly increasing within the library's registers (the step review's L2: a register twice would leave a slot unwritten).
static_assert([]{for(size_t i=0;i<glare_block_registers.size();++i)if(glare_block_registers[i]>=glare_library_registers ||
    (i && glare_block_registers[i]<=glare_block_registers[i-1]))return false;return true;}(),"glare_block_registers: strictly increasing, below 228");
// The slot of a library register in our block, -1 for a register our programmes do not read (int8_t: indexes 0 to 127; the check keeps one in reserve).
static_assert(glare_block_registers.size()<=127,"glare_block_slots holds a slot in an int8_t");
inline constexpr auto glare_block_slots=[]{std::array<int8_t,glare_library_registers> slots{};slots.fill(-1);
    for(size_t i=0;i<glare_block_registers.size();++i)slots[glare_block_registers[i]]=int8_t(i);return slots;}();
struct GlareDraw {
    GlareKind kind{};
    std::string technique;              // the library's technique (its programmes are found by their pins)
    std::array<int8_t,8> inputs{-2,-2,-2,-2,-2,-2,-2,-2};   // -1 the scene, k >= 0 surface k, -2 none
    int8_t target{};
    std::array<bool,8> point{};         // the input's sampler: point (else bilinear)
    bool border{};                      // address border (transparent black), else clamp
    std::array<uint8_t,glare_block_bytes> block{};   // our block (glare_block_registers): the words the programmes read; zero elsewhere
    bool unplaced{};                    // a builder put a word that has no slot (a register our programmes do not read): the stage refuses the plan
    bool texcoord1{};std::array<std::array<float,2>,3> uv1{};   // GL-4: vertices with a TEXCOORD1 (the vignette bright pass)
    // LS-3 (W10-NATIVE-PASSES 8): the light shafts' draws. texcoords > 2: the three vertices given whole (position then the pairs,
    // stride 8 + 8 texcoords); input -3 is the film grain (sampled linear with its mips, wrapped: film_grain[i]); target -1 with
    // write_mask 8 is the scene's alpha (Alpha_R, input -4 the scene depth).
    uint8_t texcoords{1};std::vector<float> vertices;std::array<bool,8> film_grain{};uint8_t write_mask{0xF};
    // DF-1 (W10-NATIVE-PASSES 9): a slot's address mode (a D3D12_TEXTURE_ADDRESS_MODE: the mode the library's texture object
    // carries, mirror for the DoF blurs, wrap for the DoF table) when not 0, else border or clamp as above; input -5 is the DoF
    // table (2048x2 R32_FLOAT, the depth factor's t2).
    std::array<uint8_t,8> address{};
    // I-0138 tranche 8f: the ghost's draws are 4-vertex strips over the target (POSITION, TEXCOORD0: (-1, 1) (0, 1), (-1, -1) (0, 0),
    // (1, 1) (1, 1), (1, -1) (1, 0), the oracle's draw 41 and 42); input -7 is the trim texture (TrimCircle, A8_UNORM).
    bool strip{};
    // The afterimage's history: after the draw its target is copied into surface copy_to (-2 none), the next frame's input. The library
    // flips a pair instead (YebisAfterimage_141750000): the same texels.
    int8_t copy_to{-2};
};
struct GlarePlan {
    std::vector<GlareSurface> surfaces;
    std::vector<GlareDraw> draws;
    int8_t output{-1};                  // the surface the tone map takes as t1 (2x the base width, at most half the extent)
    std::string refused;                // empty when served
    // Intermediate values, for the test against the library's state (Reference1649).
    uint32_t base_width{},base_height{};
    float radius{},luminance{};int levels{};
    uint32_t width{},height{};          // the extent the plan was built for (the renderer draws it only on a source of that extent)
    // LS-1/LS-3: Alpha_R into the scene's alpha before the source chain (target -1, input -4 the scene depth), and the light
    // shafts appended after the composite (AppendShafts): their draws come before the 2x draw, which then reads their MadT2 (shape 1),
    // or before the layers' composite, which takes their output as its last layer (shape 3, tranche 8g).
    std::vector<GlareDraw> pre;
    int shafts{};                       // the shaft records appended
    uint32_t table_width{},table_height{};   // DF-1: the DoF table's extent (input -5, R32_FLOAT), given with the frame's values to RecordPre
    int8_t history{-1};                 // I-0138 tranche 8f: the afterimage's history surface (GlareStage: black when a layout creates it, kept across one at its size and format)
    // Intermediate values of shape 3 (diagnostics: the arms and the passes per arm the plan drew).
    int streak_arms{},streak_passes{};
};
// Writes value at the library's word `word` (register word / 4, component word % 4) into our block, in its register's slot; a word
// without a slot is not written and marks the draw (unplaced).
inline void PutGlareWord(GlareDraw& g,uint32_t word,float value) noexcept {
    const uint32_t reg=word/4;const int slot=reg<glare_library_registers?glare_block_slots[reg]:-1;
    if(slot<0){g.unplaced=true;return;}
    std::memcpy(g.block.data()+16*size_t(slot)+4*(word%4),&value,4);}
GlarePlan BuildGlare(const GlareData& d,const GlareInputs& in);
// I-0138 tranche 8f: the native sources of GlareShape 3's object fields (READ, evidence/I-0138/t8f-readings/glare-ev-base-reading.md).
// The base size (CPostEffect::Initialize, the plan's base_width and base_height): {0, 0} for a quality outside 0..7 or an empty extent.
std::array<uint32_t,2> GlareBaseSize(const GlareData& d,uint32_t width,uint32_t height,int quality);
// +0x2F8 (YebisGlareFrame_1417538e0's head): (int) trunc(logf(base width / width) / logf(2)) + 2, -1 at 157 of 1280.
int GlareBaseLevel(uint32_t base_width,uint32_t width);
// +0x15C (FUN_1416eb490): the diaphragm's rotation the arms and the ghosts' cosines take, from tan_half (+0x138, VignetteFovTerm) through
// the library's fov round trip (ctx+0x56C = atanf(t) x 2, then tanf(that x 0.5)) and the globals' DiaphragmRotate (+0x1CC, the mode),
// DofRotateScale (+0x1C4) and DofRotateOffsetInRadians (+0x1C8, in degrees: the library multiplies it by pi/180). The lens term is 0
// (INFERRED: DS3 sets no lens profile). DS3: mode 1, -ln t - 0.0959763.
float GlareRotation(float tan_half,int mode,float scale,float offset_degrees);
// I-0138 tranche 8c: the library's gate on the glare family (EndPostEffectScene_InternalProcess, its shape and luminance tests before the
// bright pass; READ, evidence/I-0138/t8c-readings/glare-shape-reading.md): the whole family, light shafts included, is skipped when the
// glare luminance is not positive (a NaN too: COMISS, SETA) or the shape is 0. The shape here is the globals' word (the Bloom record's
// word 8, 0 to 14): 1 and 3 (I-0138 tranche 8f) are drawn natively; 2 and 4 to 14, which the library draws, are refused (named by the caller).
enum class GlareGate {Draw,SkipLuminance,SkipShape0,RefuseShape};
constexpr GlareGate GlareShapeGate(int shape,float luminance) noexcept {
    if(!(luminance>0.0f))return GlareGate::SkipLuminance;
    if(shape==0)return GlareGate::SkipShape0;
    return shape==1 || shape==3?GlareGate::Draw:GlareGate::RefuseShape;}
// The point-sampled blur's weights over its own taps: FUN_1416DB880 (3 taps, 1 distance), FUN_1416DBA50 (7, 3), FUN_1416DBD30 (11, 5),
// FUN_1416DC220 (15, 7; the generic path draws its first n): exp(-(j s)^2 / (2 sigma^2)) for j = 1..distances and the centre,
// normalised by g(1) x 2 + centre + g(2) x 2 + ... (or by sqrt(2 pi) sigma when analytic and that is at least 1); output k: the
// centre, then +1, -1, +2, -2 ...; offsets j / size.
void BlurWeights(const GlareData& d,uint32_t size,float sigma,bool analytic,uint32_t distances,std::array<float,15>& offsets,std::array<float,15>& weights);
}
