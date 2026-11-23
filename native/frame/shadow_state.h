// W5a SW-2 (SHADOWS-WAVE1-READY 1.3, 2.2): the cascaded-shadow state of FUN_1401B5720 as pure functions on the ShadowParam
// record R (0x94 bytes, the layout of ds3light::Banks::Shadow) and the quality object's shadow row, and the cbSceneParam
// shadow words of FUN_14023A0C0's case (the same derivation as native/world ShadowFieldsFromBanks, without snapshots).
// No GX object: 1B5720's re-creation of the atlas (1B55F0, 1B5390) is not part of it; the resources are SW-6.
#pragma once
#include <array>
#include <cstdint>

namespace ds3native::shadow {
// The quality object's shadow row ([GXRS+0x5360] +0x2C num, +0x30 den, +0x34 min, +0x38 max, +0x3C blur passes added).
struct QualityRow {uint32_t num{1},den{1},min{0},max{0xFFFFFFFF};int32_t blur_add{};};
// The rows the quality object holds per level (DISABLE, LOW, MEDIUM, HIGH, MAX; SHADOWS-WAVE1-READY 0.3, verified): test data;
// at run time the row is read from the quality object itself.
inline constexpr std::array<QualityRow,5> quality_rows{{{1,4,128,1024,-4},{1,4,128,1024,-4},{1,2,128,2048,-1},{1,1,128,4096,0},{2,1,128,8192,2}}};
// Resolution class R+0x90 to the base edge: 0 -> 2048, 1 -> 1024, 2 -> 2048 (unchanged), 3 -> 4096, any other -> 2048.
uint32_t ClassSize(int32_t resolution_class);
struct Sizes {uint32_t width{},height{};uint32_t blur_passes{};};
// 1B5720: base edge (x the double-size factor when the object's +0xC09 flag is set), then clamp(num*edge/den, min, max) as
// unsigned integers (the row is absent: the base edge); blur passes = R+0x84 + row blur_add, 0 when below 1.
Sizes ShadowSizes(const uint8_t* record,const QualityRow* row,bool double_size,int32_t double_factor);
// 1B5720's state words: +0xA70 near, +0xA7C/+0xA80/+0xA84 the splits, +0xA74 fade start, +0xA78 fade distance; each split is
// max(previous + eps, record split) with eps = [143D7ED70].
struct State {float near_z{},split0{},split1{},split2{},fade_start{},fade_dist{};};
State ShadowClamps(const uint8_t* record,float eps);
// The engine constants the words use (.rdata): tiny [143D7ECF0], half [143D7EDB4], one [143D7EE10], k [143D7FEF0], 255 [143D7EFF4].
struct Constants {float tiny{1e-6f},half{0.5f},one{1.0f};std::array<float,4> k{1,1,1,1};float c255{255.0f};};
// cbSceneParam +0x1C0 (FC_ShadowMapParam), +0x1D0 (FC_ShadowColor), +0x230 (soft-filter words, .w = 1/random texture edge),
// +0x240 (1/edge, 1/edge, R+0x80, 0) and +0x250 (cascade blend ratios) for an atlas edge; block holds at least 0x260 bytes.
void SceneWords(const uint8_t* record,const State& state,uint32_t edge,uint32_t random_texture,const Constants& constants,uint8_t* block);
}
