#pragma once
// I-0128 R8c (R8-PLAN §6): the frame's local lights binned into a world-space grid about the camera, so a ray's hit (the reflections',
// the GI's, a path's vertex, the cache update's: rt_reflect.hlsl's HitLight) loops over the lights that can reach its cell instead of
// the whole list. A light reaches only the points inside its support (rt_lights.hlsli: every term of RtLocalLight is exactly zero
// outside it), so a cell's list holds every light whose support's bounding sphere meets the cell; a hit outside the grid loops over the
// whole list. The cells keep the list's order (the light index ascending), so a hit's sums and choices are the whole list's with its
// zero terms left out: the same picture, a lower cost.
// The supports (OBSERVED in rt_lights.hlsli): a point's and a legacy point's terms vanish from d² ≥ R² (position.w = 1 / R²: the
// diffuse max(1/d² − 1/R², 0), the specular 1 − sat(d²/R²), the legacy's (1 − sqrt(sat(d²/R²)))²); a spot in its ShadingMode
// (trans.w > 0) from d ≥ R (position.w = R, the same two falloffs); a spot without the mode outside { q.z > 0, q.w < R, |q.xy| < q.w }
// (q its to_light rows at the point: sat(1 − q.w / R)² × max(1 − |q.xy / q.w|², 0)², nothing where q.z ≤ 0). When q.z ≤ 0 on the
// backward cone (q.w < 0, where the range's term is 1: checked through q.z's expression over q.x, q.y, q.w), that set lies in the pyramid
// { 0 ≤ q.w ≤ R, |q.x| ≤ q.w, |q.y| ≤ q.w }, the convex hull of its apex (q = 0) and its four corners at q.w = R: the sphere about
// the light through the farthest of those five points bounds it. A spot whose rows fail the check, or whose x, y, w rows are not
// independent, goes into every cell (never left out). Each reach takes a relative and an absolute margin (kRtLightGridMargin*), wider
// than the float rounding of the falloffs at R and of the cell's index at a cell's face, so no light of non-zero term is left out.
// I-0132 R11h: a physical light (kRtLightPhysical, every kind) vanishes from d ≥ r, position.w = 1 / r² (its window): a sphere.
#include "rt_scene.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>
namespace ds3native {
inline constexpr uint32_t kRtLightGridX=32,kRtLightGridY=16,kRtLightGridZ=32;   // cells (x, y the world's up, z)
inline constexpr float kRtLightGridCell=8.f;                                     // metres a cell (a power of two: the cell's index exact)
inline constexpr uint32_t kRtLightGridCells=kRtLightGridX*kRtLightGridY*kRtLightGridZ;
inline constexpr uint32_t kRtLightGridHeader=8;   // words before the cells: rt_reflect.hlsl's LightCell reads the cells from word 8
static_assert(kRtLightGridHeader==8,"rt_reflect.hlsl's LightCell (4 * (8 + 2 * cell)): change both");   // the final review's L6
inline constexpr float kRtLightGridMarginShare=1.001f,kRtLightGridMarginMetres=0.01f;
// The grid's words, as rt_reflect.hlsl reads them (a ByteAddressBuffer): the header (the origin's x, y, z, the corner of cell 0
// snapped to the cell size about the camera, and the cell's size, as floats; the cells along x, y, z and their count), then each
// cell's {first word, count} (cell (x, y, z) at (z × Y + y) × X + x), then the light indices the firsts point at.
// The indices a grid may hold (4 MiB; 64 unbounded lights): past them `words` is left empty (`dropped`).
inline constexpr uint64_t kRtLightGridEntriesMost=1ull<<20;
struct RtLightGridStats {uint32_t everywhere{},outside{},filled{},longest{};uint64_t entries{};bool dropped{};};   // the row's figures
// The support's bounding sphere's radius of `light` about its position, the margins taken; a negative value: not bounded (every cell).
inline float RtLightReach(const RtLight& light) noexcept {
    const auto margin=[](double reach){const double r=reach*kRtLightGridMarginShare+kRtLightGridMarginMetres;return std::isfinite(r) && r>0 && r<1e30?float(r):-1.f;};
    const uint32_t kind=light.kind&0xFFu;
    if(kind!=kRtLightSpot || (light.kind&kRtLightPhysical)){const double inv=light.position[3];   // I-0132 R11h: a physical light's window r (1 / r^2)
        if(!(inv>0) || !std::isfinite(inv))return -1.f;return margin(1/std::sqrt(inv));}
    const double range=light.position[3];if(!(range>0) || !std::isfinite(range))return -1.f;
    if(light.trans[3]>0.f)return margin(range);   // the ShadingMode: d < R
    const float* rx=&light.to_light[0];const float* ry=&light.to_light[4];const float* rz=&light.to_light[8];const float* rw=&light.to_light[12];
    const auto dot=[](const float* a,const float* b){return double(a[0])*b[0]+double(a[1])*b[1]+double(a[2])*b[2];};
    // The rows' system M p = q − b (M the x, y, w rows' xyz, b their w), by its inverse (Cramer): q = (q.x, q.y, q.w) to the world.
    const double m[3][3]{{rx[0],rx[1],rx[2]},{ry[0],ry[1],ry[2]},{rw[0],rw[1],rw[2]}};const double b[3]{rx[3],ry[3],rw[3]};
    const double det=m[0][0]*(m[1][1]*m[2][2]-m[1][2]*m[2][1])-m[0][1]*(m[1][0]*m[2][2]-m[1][2]*m[2][0])+m[0][2]*(m[1][0]*m[2][1]-m[1][1]*m[2][0]);
    const double scale=std::sqrt(dot(rx,rx)*dot(ry,ry)*dot(rw,rw));
    if(!(std::abs(det)>1e-6*scale) || !std::isfinite(det))return -1.f;
    double inv[3][3];
    for(int i=0;i<3;++i)for(int j=0;j<3;++j){const int i1=(j+1)%3,i2=(j+2)%3,j1=(i+1)%3,j2=(i+2)%3;inv[i][j]=(m[i1][j1]*m[i2][j2]-m[i1][j2]*m[i2][j1])/det;}
    // q.z > 0 nowhere on the backward cone (q.w < 0, |q.xy| < |q.w|: there `far` is 1, the cone's term positive): q.z written over q
    // (the step review's L1: exact, not a parallel test), q.z = α q.x + β q.y + γ q.w + δ, is at most (|α| + |β| − γ) s + δ at the depth
    // s = −q.w on the square about that cone, so the slope must be negative and its zero s₀ = max(δ, 0) / (γ − |α| − |β|) within
    // R / 10⁴ of the apex (the sliver there inside the margins: its points within 2 × 10⁻⁴ of the reach).
    double coef[3]{},delta=rz[3];
    for(int j=0;j<3;++j){for(int k=0;k<3;++k)coef[j]+=double(rz[k])*inv[k][j];delta-=coef[j]*b[j];}
    const double slope=std::abs(coef[0])+std::abs(coef[1])-coef[2];
    // The fixes' step review L4: the slope clearly negative (a thousandth of the coefficients), not within the shader's float rounding of q.z.
    if(!(slope<-1e-3*(std::abs(coef[0])+std::abs(coef[1])+std::abs(coef[2]))) || !std::isfinite(delta) || std::max(delta,0.0)/-slope>1e-4*range)return -1.f;
    double farthest=0;
    const double corners[5][3]{{0,0,0},{range,range,range},{-range,range,range},{range,-range,range},{-range,-range,range}};
    for(const auto& t:corners){double d2=0;
        for(int i=0;i<3;++i){double p=0;for(int j=0;j<3;++j)p+=inv[i][j]*(t[j]-b[j]);const double e=p-light.position[i];d2+=e*e;}
        farthest=std::max(farthest,d2);}
    return margin(std::sqrt(farthest));
}
// The cell of `p` (world) in the grid at `origin`, as rt_reflect.hlsl's LightCell computes it; false outside the grid.
inline bool RtLightGridCellOf(const std::array<float,3>& origin,const float p[3],uint32_t& cell) noexcept {
    const uint32_t dims[3]{kRtLightGridX,kRtLightGridY,kRtLightGridZ};uint32_t at[3]{};
    for(int a=0;a<3;++a){const float c=std::floor((p[a]-origin[a])/kRtLightGridCell);if(!(c>=0.f) || !(c<float(dims[a])))return false;at[a]=uint32_t(c);}
    cell=(at[2]*kRtLightGridY+at[1])*kRtLightGridX+at[0];return true;
}
// Bins `lights` about `eye` (world) into `words` (the layout above; empty past kRtLightGridEntriesMost).
inline void BuildRtLightGrid(const std::vector<RtLight>& lights,const std::array<float,3>& eye,std::vector<uint32_t>& words,RtLightGridStats& stats) {
    const uint32_t dims[3]{kRtLightGridX,kRtLightGridY,kRtLightGridZ};std::array<float,3> origin{};
    for(int a=0;a<3;++a)origin[a]=(std::floor(eye[a]/kRtLightGridCell)-float(dims[a]/2))*kRtLightGridCell;
    stats={};
    // I-0132 R11b (R11-PLAN §13 item 4, I-0128 « R8c » L7): the boxes and the cells' counts and fills (about 130 KiB) kept between
    // frames by the calling thread (the main view's collect alone; the function does not recurse), zeroed in place; `words` stays the
    // frame's own (the list is handed to the frame).
    struct Box {uint32_t lo[3],hi[3];bool any;};thread_local std::vector<Box> boxes;thread_local std::vector<uint32_t> counts,fill;
    boxes.resize(lights.size());
    for(size_t i=0;i<lights.size();++i){Box& box=boxes[i];box.any=false;const float reach=RtLightReach(lights[i]);
        if(reach<0.f){for(int a=0;a<3;++a){box.lo[a]=0;box.hi[a]=dims[a]-1;}box.any=true;++stats.everywhere;}
        else{bool inside=true;
            for(int a=0;a<3;++a){const float lo=std::floor((lights[i].position[a]-reach-origin[a])/kRtLightGridCell),hi=std::floor((lights[i].position[a]+reach-origin[a])/kRtLightGridCell);
                if(!std::isfinite(lo) || !std::isfinite(hi) || hi<0.f || lo>float(dims[a]-1)){inside=false;break;}
                box.lo[a]=uint32_t(std::max(lo,0.f));box.hi[a]=uint32_t(std::min(hi,float(dims[a]-1)));}
            box.any=inside;stats.outside+=!inside;}
        if(box.any)stats.entries+=uint64_t(box.hi[0]-box.lo[0]+1)*(box.hi[1]-box.lo[1]+1)*(box.hi[2]-box.lo[2]+1);}
    // The step review's L5: past the budget (many unbounded or long-reaching lights) no grid this frame: the hits loop over the whole
    // list, the same picture; the final review's L7: known from the boxes' volumes before any cell is counted.
    if(stats.entries>kRtLightGridEntriesMost){words.clear();stats.dropped=true;return;}
    counts.assign(kRtLightGridCells,0);
    for(const Box& box:boxes){if(!box.any)continue;
        for(uint32_t z=box.lo[2];z<=box.hi[2];++z)for(uint32_t y=box.lo[1];y<=box.hi[1];++y)for(uint32_t x=box.lo[0];x<=box.hi[0];++x)++counts[(z*kRtLightGridY+y)*kRtLightGridX+x];}
    words.assign(kRtLightGridHeader+2*size_t(kRtLightGridCells),0);
    for(int a=0;a<3;++a){std::memcpy(&words[a],&origin[a],4);words[4+a]=dims[a];}
    std::memcpy(&words[3],&kRtLightGridCell,4);words[7]=kRtLightGridCells;
    uint64_t first=kRtLightGridHeader+2ull*kRtLightGridCells;
    for(uint32_t c=0;c<kRtLightGridCells;++c){words[kRtLightGridHeader+2*c]=uint32_t(first);first+=counts[c];
        stats.filled+=counts[c]!=0;stats.longest=std::max(stats.longest,counts[c]);}
    words.resize(size_t(first));fill.assign(kRtLightGridCells,0);
    for(size_t i=0;i<lights.size();++i){const Box& box=boxes[i];if(!box.any)continue;
        for(uint32_t z=box.lo[2];z<=box.hi[2];++z)for(uint32_t y=box.lo[1];y<=box.hi[1];++y)for(uint32_t x=box.lo[0];x<=box.hi[0];++x){
            const uint32_t c=(z*kRtLightGridY+y)*kRtLightGridX+x;words[words[kRtLightGridHeader+2*c]+fill[c]++]=uint32_t(i);}}
    for(uint32_t c=0;c<kRtLightGridCells;++c)words[kRtLightGridHeader+2*c+1]=fill[c];
}
}
