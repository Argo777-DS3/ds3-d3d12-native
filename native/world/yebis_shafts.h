// W10 LS-1 (W10-NATIVE-PASSES 7): the light shafts' first pass, the scene depth into the scene target's alpha (tech_Alpha_R,
// FUN_1416DF6E0 -> FUN_141727DA0), computed natively. The library converts the depth between its spaces with FUN_1416F4AF0
// (0 the device depth, 1 and 2 through the words +0xC38..+0xC48 FUN_1416E9050 derives), then Alpha_R's two `$Globals` float4:
// (s, s, s, s) then the colour (r, r, r, 1) at word 616, (o, o, o, o) at word 744; the pixel programme writes saturate(s d + o)
// into the alpha only (the original's blend: write mask 8, no blend, Reference1660). Checked bit for bit against the captured
// words by yebis_shafts_test.
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include "yebis_glare.h"

namespace ds3native::yebis {
// The library's depth words (S = [1448E5E38]): near and far (+0x564, +0x568, from E69A0's camera record +0x54 and +0x58 through
// FUN_1416C9450), the source space (+0x43C), the conversion words +0xC38..+0xC48 and FUN_1416F52A0's factor (space 3 only).
struct DepthSpaces {float near_plane{},far_plane{},c38{1.0f},c3c{},c40{1.0f},c44{},c48{},space3{1.0f};int source{};};
// FUN_1416F4AF0(S, out, from, to, scale, bias), 6 naming the source space: out = {a, b, 1/a, -(b/a)}, the map x -> a x + b.
std::array<float,4> DepthConversion(const DepthSpaces& d,int from,int to,float scale=1.0f,float bias=0.0f) noexcept;
// FUN_1416E9050's conversion words in the depth-of-field branch (S+0xC33: +0x6CC > 0, 0 < near < far, the DoF holder's +0x164 and
// S+0x924): c48 = S+0x6C0 (the focus distance, globals 144962120), c38..c44 = the DoF holder's +0x1B4..+0x1C0, then c40 = 1/c38 and
// c44 = -(c3c/c38) as the original rewrites them.
DepthSpaces DofDepthSpaces(float near_plane,float far_plane,float focus,const std::array<float,4>& dof_words) noexcept;
// I-0138 tranche 8d: FUN_1416E9050's words outside the depth-of-field branch (S+0xC33 clear; READ, GhidraMCP 2026-11-21): c38 = 1 and
// c3c = 0 (the 8-byte store 0x3F800000; the rescale by 64000 [142B552F0] needs S+0x720, which only CPostEffect's Initialize writes,
// 0), c40 = 1 / c38 and c44 = -(c3c / c38), and c48 the start of the depth range the stages ask for: with the motion blur at mode 2
// (S+0x9C5, S+0x9C8 2) or a shaft record (FUN_1416EA000), the range [near, far] clamped to [S+0x718, S+0x71C], which are the near and
// far planes while S+0x710/+0x714 keep Initialize's 0 (their setter 1416F5FA0 is reached only from 1416CAF10, which nothing in the
// file references; never called INFERRED, the encrypted-at-rest code of D-0272 not covered by a byte scan), so c48 = near; S+0x720
// likewise (Initialize's 0 READ, no other caller INFERRED). Three ranges would narrow it, none served or checked natively: the
// AntiAlias stage S+0xBA4 (the globals' +0x220, 0 in every shipped file), the stage S+0xA40 (Initialize's 0, its other setter
// unreferenced) and the motion blur's falloff at mode 1 (S+0x9C8 1 with a valid S+0x9F0/+0x9F4; every native_yebis_motionblur row in
// the journals reads mode 2, and the native blur refuses another mode or a falloff, but the shafts would then use near where the
// library narrows). The library converts a shaft record's depth in FUN_1416EA000, called before these stores, so with the previous
// frame's words; the native converts the record with the previous served frame's spaces too since tranche 8i (AppendNativeShafts),
// Alpha_R with the current frame's, as the library; still apart: a record's first frame outside the branch (the library's previous
// c48 the far plane, the native's the near) and the first frame after frames the shafts were not served (the native's own spaces).
// Without the blur or a shaft c48 is the far plane (OBSERVED: i141-t14a-c45000030-orig's reference_shaft_depth rows with dof 0).
// This is the blur's (and a kept shaft record's) case: c48 = near.
DepthSpaces NearDepthSpaces(float near_plane,float far_plane) noexcept;
// I-0138 tranche 8i: a shaft record's depth spaces at FUN_1416EA000 (inside FUN_1416E9050, before its stores of c38..c48): this frame's
// near, far, source and space-3 factor (E69A0's FUN_1416C9450 stores them before, INFERRED from the readings above, not re-read in
// Ghidra) with the previous frame's c38..c48; without a previous frame (nullptr), this frame's.
DepthSpaces RecordDepthSpaces(const DepthSpaces& current,const DepthSpaces* previous) noexcept;
// Whether the stored spaces are the previous frame's: stored at all (previous_serial not ~0) and at the presentation serial just before
// this one (the step review of 050f3c888, L2: the rule tested on its own).
constexpr bool PreviousSpacesApply(uint64_t previous_serial,uint64_t presentation) noexcept {return previous_serial!=~0ull && previous_serial+1==presentation;}
// Alpha_R's inputs from FUN_1416DF6E0: the depth source's (scale, bias) S+0x448/+0x44C when a source object is set (S+0x430 or
// S+0x50), else (1, 0); the colour r = (S+0x454 ? S+0x6E8 x S+0x69C : S+0x698) / S+0x698.
struct AlphaRInputs {DepthSpaces spaces;bool source_object{true};float source_scale{1.0f},source_bias{};float colour{1.0f};};
struct AlphaRBlock {float scale{},offset{};std::array<float,4> colour{};};
// The block FUN_141727DA0 writes: s = d[0] x scale, o = d[1] + bias x d[0], with d = DepthConversion(6, 2, 1, 0).
AlphaRBlock AlphaR(const AlphaRInputs& in) noexcept;

// W10 LS-2 (W10-NATIVE-PASSES 8): one light shaft's draws (FUN_141750620 for a record of CRenderGlare +0x1C8): the mask
// (FUN_1417372A0 -> FUN_1417290B0, tech_LightShaftMask_SM3) into the shaft surface, then n radial blurs (FUN_14171E8E0 ->
// FUN_14171D490 -> FUN_141718C00, tech_TexCoordMadT1_S{taps}) ping-ponging between two surfaces, the last into the output.
// A surface by its extent; its texel scale is 1/extent and its bias 0 (the library's +0x88..+0x94 for the surfaces it allocates).
struct ShaftSurface {uint32_t width{},height{};};
struct ShaftInputs {
    std::array<float,20> record{};      // the 0x50-byte record (+0x00 colour, +0x10.., +0x34 second colour, +0x44 the sun, +0x4C its depth)
    uint32_t base_width{};              // CRenderGlare +0xB0 (the glare base width)
    float factor{},luminance{};         // +0x114, +0x118 (the glare's factor and luminance)
    float fov{},tan_half{};             // +0x134, +0x138 (the library's S+0x56C and S+0x570: the field of view and tan(fov / 2))
    float exposure{};                   // +0x148 (the bright pass's exposure, globals 144962008)
    float shape_bloom{};                // [+0x150]+8 (the glare shape's bloom luminance)
    bool float_surface{true};           // the shaft surface's format is a float format (the 0.001 base; 8-bit UNORM takes 1/255)
    ShaftSurface mask_source,shaft,output;std::array<ShaftSurface,2> pingpong{};
};
enum class ShaftSlot : int8_t {None=-1,MaskSource,FilmGrain,Shaft,Ping0,Ping1,Output};
struct ShaftDraw {
    std::string technique;
    std::array<ShaftSlot,2> inputs{ShaftSlot::None,ShaftSlot::None};
    ShaftSlot target{ShaftSlot::None};
    std::vector<std::pair<uint32_t,float>> words;   // the `$Globals` words the programmes read (word index, value)
    uint32_t texcoords{};               // texture-coordinate pairs per vertex after the position
    std::array<std::vector<float>,3> vertices;      // the covering triangle: position then the texcoords pairs, per vertex
};
struct ShaftPlan {
    std::vector<ShaftDraw> draws;
    std::string refused;                // empty when served
    int passes{},last_taps{};           // the radial blur's pass count and the last pass's taps
};
ShaftPlan BuildShaft(const ShaftInputs& in);
}
namespace ds3native::yebis {
// LS-2b: CRenderGlare's record from the library's slot (the slots at the pointer [S+0x6B0], 0x58 bytes each, written by E69A0's
// setters FUN_1416C97F0, FUN_1416C9830, FUN_1416C9870): words 0..18 as they are, the sun's depth (slot +0x4C, the device depth)
// converted into depth space 2 like Alpha_R's (DepthConversion(6, 2): a d + b). The copy (FUN_1416EA000 -> FUN_141752DB0, E60, EF0)
// READ in I-0141 tranche 8 (it also scales the colour by S+0x6F8 when S+0x456 is set, which the native's tone setter leaves clear);
// the record observed alike in both modes in 45000030 once the original's onset is over (words 13 to 16 at 1; the sun's depth
// word 0.9974 against 1: i141-t12a's reference_shaft_depth and native_shaft_depth rows).
std::array<float,20> ShaftRecordFromSlot(const std::array<float,22>& slot,const DepthSpaces& spaces) noexcept;
// LS-2b: E69A0's light-shaft section (1400E7416..1400E79A8) for slot 0: the sun direction from the globals' elevation and
// azimuth (1449620D0, 1449620D4, degrees) and distance (globals +0x24C), its view-space position through the inverse of the
// camera pose (FUN_14004BCA0) and the projection (FUN_14004B070), its screen position and depth, the visibility (the normalised
// direction's x and z dotted with the pose's third row, + 0.3, clamped to [0, 1]) scaling both colours (CPostEffect
// +0x21E0..+0x21EC and +0x2200..+0x220C, written by E4700), and the globals words the setters copy (FUN_1416F3870,
// FUN_1416C9830, FUN_1416C9870, FUN_1416F37C0). The vector arithmetic keeps the original's SSE lanes and order.
struct ShaftDriverInputs {
    std::array<float,16> pose{};        // the camera record's pose (+0x00, row-major: the camera's world matrix)
    std::array<float,16> projection{};  // FUN_14004B070 on the camera record
    std::array<uint32_t,0x370/4> globals{};   // the block 144961F70
    std::array<float,4> colour{},colour2{};   // CPostEffect +0x21E0..+0x21EC (rgb, scale) and +0x2200..+0x220C
};
std::array<float,22> DriverSlot(const ShaftDriverInputs& in) noexcept;
// LS-3: the film grain (the mask's t1): the executable's 256x256 RGBA8 block at RVA 0x2B68A40 (SHA-256 below) as level 0, then
// the CPU mip chain FUN_141711350 builds (FUN_141719770 calls it with both dither flags): each level a 2x2 box of the previous,
// (offset + sum of the four bytes) / 4 truncated, the offset 2 or, from a 4-texel level up, the 4x4 table at [y & 3 + 4 (x & 3)]
// (0, 2, 0.5, 2.5, 3, 1, 3.5, 1.5, 0.75, 2.75, 0.25, 2.25, 3.75, 1.75, 3.25, 1.25: [143D7ED9C] x 0, [143D7EE7C], [143D7EDB4],
// [1427CDAB4], [142B60D20..2C], [143D7EDE8], [142B60CD4], [143D7ED9C], [14278D024], [142B60D10..1C]); nine levels down to 1x1.
inline constexpr uint32_t film_grain_rva=0x2B68A40,film_grain_size=256;
inline constexpr const char* film_grain_sha256="11C7FAD64F41EB04E249BEE63F37C2F7CBCBD340AED9D0EAC8EC3713C031354C";
std::vector<std::vector<uint8_t>> FilmGrainLevels(const std::vector<uint8_t>& level0);
}
namespace ds3native::yebis {
// LS-3: Alpha_R as the glare plan's pre draw (target -1, the scene's alpha; input -4, the scene depth; samplers as Reference1667:
// s0 point border).
GlareDraw AlphaRDraw(const AlphaRBlock& block);
// LS-3: one shaft appended to the glare plan: four R11G11B10 surfaces at the shaft extent (the shaft, the ping-pong pair, the
// output) and one at the composite's for MadT2; the mask reads the bright pass's target and the film grain (-3), the radial blurs
// ping-pong, MadT2 composes the glare composite and the shafts (weights 1), and the 2x draw reads MadT2 instead of the composite.
// Samplers as Reference1667: s0 linear border for the mask and the blurs (s1 the film grain), point clamp for MadT2.
bool AppendShafts(GlarePlan& plan,const ShaftInputs& in,const ShaftPlan& shafts,std::string& refused);
}
