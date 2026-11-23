// W10 DoF (W10-NATIVE-PASSES 9): the depth of field's constants, computed natively from the DoF holder's state (CDepthOfField,
// [library+0x110]) with the original's single-precision order. Checked against the captured holder and layer records
// (Reference1676-1678) by yebis_dof_test.
#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include "yebis_glare.h"
#include "yebis_shafts.h"

namespace ds3native::yebis {
// One layer record's inputs (the holder's [+0x470] + i x 0x108): its height (+4) and its size words (+0x20, +0x30).
struct DofLayer {uint32_t height{};float radius{},size{};};
// FUN_141756580's outputs per layer: +0xC0..+0xDC (the far and near ramps and their copies scaled by the holder's +0x1B0),
// +0xE8/+0xEC (the radius in texels) and +0xF0/+0xF4 (scaled by +0x1BC/+0x1C0 when +0x154 == 1).
struct DofLayerWords {float c0{},c4{},c8{},cc{},d0{},d4{},d8{},dc{},e8{},ec{},f0{},f4{};};
struct DofHolder {uint32_t flags{};int mode154{};float scale1b0{},scale1bc{1.0f},offset1c0{};};   // +0x140, +0x154, +0x1B0, +0x1BC, +0x1C0
std::vector<DofLayerWords> DofLayerRamps(const DofHolder& h,const std::vector<DofLayer>& layers) noexcept;

// W10 DF-1: the frame of the depth of field (FUN_1416E5BC0's two DoF steps): the depth factor into the scene's alpha
// (FUN_1416E0040 -> FUN_1416DF6E0 with the DoF table: FUN_141727DA0 -> FUN_1417274F0, technique table +0x147F4), then
// CDepthOfField::ApplyDepthOfField (FUN_141767D80): the front and back level counts and the source chain (FUN_14175ED50, its
// downsamples through FUN_14172A660 and FUN_1416D6570), the aperture filter of each level (FUN_14175EA40 -> FUN_14175C7E0: the
// front-blur blend factor FUN_141727350, then the level's blur FUN_14175BB40 kind 0: FUN_14173B2C0 (3, five bilinear taps of
// FUN_1416D7300) or FUN_14173B3D0 (5, nine taps of FUN_1416D7540)), and the composite (FUN_14175F850 mode 2 -> FUN_1417266B0).
// One layer as the frame reads it.
struct DofLevel {
    uint32_t width{},height{};          // +0x00, +0x04: the level's extent
    int surface{};                      // [+0x38]: the source surface it is drawn from (levels sharing one have the same id)
    uint32_t surface_width{},surface_height{};   // that surface's extent
    int kind{};                         // +0x10: the aperture filter (0, the tap blurs, is the only one served)
    int taps{};                         // +0x18: the blur's tap rule (3 or 5 served)
    DofLayerWords ramps;                // FUN_141756580
};
struct DofFrame {
    uint32_t width{},height{};          // the scene (+0x144, +0x148, the source surface [+0x200])
    uint32_t flags{};                   // +0x140 (2: front blur; 0: the plain composite, part 4b; bits 4 and 8 not served)
    int mode154{};float scale1bc{1.0f},offset1c0{};   // +0x154, +0x1BC, +0x1C0
    float near_plane{},far_plane{};     // +0x168, +0x16C
    float near_override{},far_override{};   // +0x1E8, +0x1EC (0: the planes)
    float focus{};                      // +0x188
    std::array<int,4> limits{-1,-1,-1,-1};   // +0x1C4, +0x1C8 (front, back), +0x1CC, +0x1D0
    int source_column{},level_column{}; // +0x1F0, +0x1F4: the downsamples' technique column (0 served)
    std::vector<DofLevel> levels;
    // The depth factor (library S = [1448E5E38]): the depth spaces of FUN_1416F4AF0 (space 3 scaled by [S+0x110]+0x1B0 when
    // S+0xC34), the depth source's scale and bias (S+0x448, S+0x44C when S+0x430 or S+0x50, else 1 and 0), the depth format code
    // FUN_1416FDEF0(S+0x438) (0: the R value), the table's spaces S+0x74 (lookup) and S+0x78 (factor), its row S+0x80, the
    // conversion words S+0xC38..+0xC44 and the table's extent.
    DepthSpaces spaces;
    bool source_object{true};float source_scale{1.0f},source_bias{};
    int depth_format{},lookup_space{},factor_space{3};float table_row{};
    std::array<float,4> conversion{1.0f,0.0f,1.0f,-0.0f};
    uint32_t table_width{2048},table_height{2};
};
// The executable's words the blurs read: [142B4D0A8] and [142B4D0A0] (FUN_1416D7540's two tap distances) and [142B4D118] (the
// last term of its normalisation). W10 closure (m40): the nine-tap level's 25-tap kernel (FUN_14173B850 -> FUN_1416D77E0) reads ten
// distances: [143D7EE38], [143D7F0CC], [142B4D148], [142B4D1B0], [142B4D13C], [142B4D18C], [142B4D1A8], [142B4D1AC], [142B4D0A4],
// [142B4D144], in that order in kernel25.
struct DofData {float axis{},diagonal{},norm_last{};std::array<float,10> kernel25{};};
// The executable's words of DofData, through read (rva, out, bytes).
bool LoadDofDrawData(DofData& d,const std::function<bool(uint32_t,void*,size_t)>& read);
// FUN_14175ED50's level counts: +0x1D8 (front), +0x1DC (back), +0x1D4 (the larger).
struct DofCounts {int front{},back{},both{};};
DofCounts DofLevelCounts(const DofFrame& f) noexcept;
// The plan: pre[0] the depth factor (target -1 the scene's alpha, write mask 8; input -4 the scene depth, -5 the DoF table),
// then the source downsamples, each front level's blend factor and blur, each other drawn level's blur, the composite into the
// output surface (the DoF scene, the scene's extent), which then stands for the scene (Alpha_R, the glare, the tone map).
GlarePlan BuildDof(const DofData& d,const DofFrame& f);

// W10 DF-2: the holder natively. Its creation (the library's Initialize FUN_1416EBF80, DoF block: the base size from the DoF quality's
// row of the table 14469B510 (0x1C per row: width scale, levels, creation flags index, -, preset, small width, small scale), the
// render aspect W/H and CPostEffect's +0x388 (0x3F2AAAE3); FUN_141762310 -> FUN_141756870 -> FUN_141755ED0: the levels from the
// preset table (seven presets of seven (a, b) pairs on FUN_141755ED0's stack), halved when the preset's a changes, the small-level
// table 142B65858; FUN_141754B60 -> FUN_141754D30: the taps (142B659C0 for kind 0) and the radius x 1.05 (1429C86F0)).
struct DofSetupData {
    std::array<std::array<uint32_t,7>,9> quality{};   // 14469B510 + 0x1C k
    std::array<std::array<int32_t,2>,49> presets{};   // FUN_141755ED0's stack table (row 21's b is computed: aspect < 1.2)
    std::array<int32_t,5> taps{};                     // 142B659C0: kind 0's taps by the preset's b
    float radius{};                                   // [1429C86F0] 1.05
    std::array<float,4> small_sizes{};                    // 142B65858
    float base_ceiling{},height_rounding{},half{};    // [143D7EE00] 0.99, [142B55250] 0.499999, [143D7EDB4] 0.5
    float dof_aspect{};                               // CPostEffect +0x388 (0x3F2AAAE3, the library's S+0x388)
    float aspect_limit{};                             // [143D7EE18] 1.2
};
bool LoadDofSetupData(DofSetupData& d,const std::function<bool(uint32_t,void*,size_t)>& read);
struct DofSetupLevel {
    uint32_t width{},height{},surface_width{},surface_height{};
    int kind{},preset_b{},taps{},small_index{};
    float radius{},size{},size_scale{};               // +0x20, +0x24 (+0x2C added when the small table is used), +0x30 = +0x24 / height
};
struct DofSetup {
    uint32_t width{},height{};                         // the scene extent (+0x144, +0x148)
    uint32_t base_width{},base_height{};               // +0xC0, +0xC4
    uint32_t creation_flags{};                         // +0xC8 (FUN_1416FDD80 of the row's index: 0, 0, 2, 6, 6)
    int levels{},preset{};                             // +0xCC, +0xD8
    uint32_t small_width{};float small_scale{};        // +0x118, +0x11C
    std::vector<DofSetupLevel> level;
    std::string refused;
};
// quality: the DoF quality (globals word 0x4C, the Initialize argument); levels: the library's S+0x384 (7).
DofSetup BuildDofSetup(const DofSetupData& d,uint32_t width,uint32_t height,int quality,int levels);
// The frame's lens (E69A0's setters FUN_1416C9450 and FUN_1416C98D0 from the camera record and the globals block 144961F70, then
// FUN_1416E9050 -> FUN_141763320 -> FUN_141762CF0, aperture type 0): the holder's +0x168.. +0x1C0 and the library's DoF branch.
struct DofLensInputs {
    float near_plane{},far_plane{},fov{};              // the camera record +0x54, +0x58, +0x50
    float aspect{};                                    // +0x64 (or +0x5C / +0x60 when 0)
    uint32_t extent_width{},extent_height{};           // the render extent (E5110's [+0x3A8] x [+0x3AC])
    int quality{};                                     // globals word 0x4C ([144961FBC])
    float focus{},f_number{},p6d0{},angle{},diagonal{};// globals +0x1B0, +0x1B4, +0x1B8, +0x1BC (degrees), +0x1C0 (the film diagonal)
    bool enable{};                                     // globals +0x1AC (and E69A0's fifth argument)
};
struct DofLens {
    float near_plane{},far_plane{},tan_half{},fov{};   // +0x168, +0x16C, +0x174, +0x170
    float sensor{},f_number{};                         // S+0x6C4, S+0x6CC (= +0x190, +0x18C)
    float focus{},focal{},aperture{},coc{},scale{};    // +0x188, +0x180, +0x184, +0x19C, +0x1B0
    std::array<float,4> words{1.0f,0.0f,1.0f,0.0f};    // +0x1B4..+0x1C0
    bool enable{};                                     // +0x164 (S+0xC34)
    bool branch{};                                     // S+0xC33: the depth-of-field branch
};
struct DofConstants {float one{},half{},two{},pi{},degrees{},milli{},full_hd{},quality_steps{};};   // [143D7EE10], [143D7EDB4], [143D7EE7C], [143D7EEC0], [143D7EFE8], [143D7ED10], [143D7F088], [143D7EF48]
DofLens DofLensState(const DofConstants& k,const DofLensInputs& in);
// The frame from the setup, the lens and the globals (+0x1D0, +0x1D4 the level limits, +0x240 the holder flags' index).
DofFrame NativeDofFrame(const DofSetup& setup,const DofLens& lens,int front_limit,int back_limit,uint32_t flags);
}
