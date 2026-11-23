#pragma once
#include <cstdint>
#include <vector>

// I-0047: DS3's light tile binning (140234E40, called by the point-light pass 1A1DA0), transcribed instruction by
// instruction with the same SSE operations so the plane bits and the sphere tests match the engine. The screen is cut
// into tw x th tiles; a light is kept for a tile row when its view-space sphere passes the row's two planes, then for a
// tile when it passes the tile's eight planes (2442D0: dot(plane, (c, 1)) <= r for every plane). Lights keep the
// order of the input list.
namespace ds3tiles {
// The view object [pass+0x418] fields the binning reads.
struct View {
    float fov{};          // +0x50 (radians, full vertical angle)
    float aspect{};       // +0x64 (0: +0x5C / +0x60)
    uint32_t aspect_w{}, aspect_h{};   // +0x5C, +0x60
    uint32_t views{};     // +0x70 (> 1: every light kept, no test)
    uint32_t view_index{};// +0x74
    uint32_t border{};    // +0x78
    uint32_t width{}, height{};   // +0x7C, +0x80
};
struct Sphere { float x{},y{},z{},r{}; };   // light object +0x70 (view space centre, radius)
// Tiles of the rectangle [x0, x1) x [y0, y1) (zero x1 / y1: the whole screen incl. border). Result: per global tile
// index (row * columns + column), the indices of the lights binned to it, in input order. columns = out value.
bool Bin(const View& view,const std::vector<Sphere>& lights,uint32_t tw,uint32_t th,int x0,int y0,int x1,int y1,
    std::vector<std::vector<uint32_t>>& tiles,uint32_t& columns);
}
namespace ds3tiles {
// I-0063: a light the binning tests by its bounding points (spot lights: the light's vtable[0] 140214310 hands the plane set
// its shape at +0xF0, tested by 140244520): `count` points (float4, w replaced by 1) transformed by the row-vector matrix
// m (shape +0x10..+0x4F); a plane rejects the light when every transformed point lies on its positive side.
struct Hull { bool valid{}; bool all{}; uint32_t count{}; float points[8][4]{}; float m[16]{}; };   // all: diagnostic, kept in every tile
// Bin with hulls: lights[i] is tested as hulls[i] when hulls[i].valid (hulls may be shorter than lights).
bool Bin(const View& view,const std::vector<Sphere>& lights,const std::vector<Hull>& hulls,uint32_t tw,uint32_t th,int x0,int y0,int x1,int y1,
    std::vector<std::vector<uint32_t>>& tiles,uint32_t& columns);
}
