// W5e (WAVE1-PREP 2.4 T1a/T4a/T5a, WATER-TESS-WAVE1-READY 3.4 TS-1/TS-4, MENUS-MAP 67/73): the tessellation fields a material's
// GX04/GX90 item writes (FUN_140141070) and the two constant blocks the hull and domain stages read: cbPNTriangles
// (FUN_1402302C0, per material) and cbTessScene (per view). Pure functions; no D3D12.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace ds3native::tess {
// The instance fields: +0x250 mode, +0x254 displacement (byte), +0x258/+0x25C edge/inside factors (integers), +0x260 d7,
// +0x264/+0x268 (the constructor's 0), +0x26C d11, +0x270 d10, +0x274/+0x278 fade start/end, +0xDF item present, +0xE4 GX90.
struct Material {uint32_t mode{};uint8_t displacement{};int32_t edge{},inside{};float d7{},mat264{};uint32_t mat268{};float d11{},d10{},fade_start{},fade_end{};
    bool present{},gx90{};};
// GX04: data words after the 12-byte header (d[3..] of MENUS-MAP 67): +0x254 = d[3] & 1, +0x250 = d[4], +0x258/+0x25C =
// (int)float d[5..6], +0x260 = d[7], +0x274 = d[8], +0x278 = d[9], +0x270 = d[10], +0x26C = d[11], +0xDF = 1 (9 words).
bool FromGx04(const uint8_t* data,size_t words,Material& out);
// GX90: +0x254 = 1, +0x250 = 2, +0x258/+0x25C = (int)float d[3..4], +0x260 = d[5], +0x274 = d[6], +0x278 = d[7], +0x270 = d[8],
// +0x26C = d[9], +0xDF = +0xE4 = 1 (7 words).
bool FromGx90(const uint8_t* data,size_t words,Material& out);
// 1402302C0 (runs when +0xDF and the mode are non-zero): row 0 (min(float edge, 64), min(float inside, 64), fade start,
// end - start or 0.001 when that is <= 0.001), row 1 (1 - d7, min(+0x264, 0.99), 1, float(+0x268)), row 2
// (displacement ? (GX90 ? exparams+0x24 : 1) x d11 : 0, d10, 0, 0). False when it would not run.
bool PnTriangles(const Material& material,float exparam24,uint8_t out[0x30]);
// cbTessScene (0xB4 bytes; TESSSCENE-SSMASK-NATIVE 1.3): @0 transpose(view x projection) (absolute world), @64 the eye
// (w 1), @80 -forward normalised (w 0), @96 (1, 0, 0, 0), @112 the four planes row3 + row0, row3 - row0, row3 + row1,
// row3 - row1 of the column-vector matrix each divided by the length of its xyz, @176 the clock in seconds.
// `view_projection` is row-vector (v' = v M), as the native camera matrices.
void TessScene(const std::array<float,16>& view_projection,const std::array<float,3>& eye,const std::array<float,3>& forward,float clock,uint8_t out[0xB4]);
// W7 TS-4b: TessScene's inputs from the view's cbSceneParam image (at least 448 bytes): the eye is SC_CameraPos (@48) and the
// absolute-world view-projection is the camera-relative VC_MatrixViewProj (@384, the stored rows the ordinary vertex
// programmes apply to position - SC_CameraPos) with each stored row's w less its xyz dot the eye, returned row-vector.
void AbsoluteViewProjection(const uint8_t* scene_param,std::array<float,16>& view_projection,std::array<float,3>& eye);
}
