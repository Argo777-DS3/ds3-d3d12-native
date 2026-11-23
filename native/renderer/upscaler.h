#pragma once
// I-0111 (R1, UPSCALER-DLSS-READY §3.2-3.3): the temporal upscaler's camera jitter and the matrices that keep the motion vectors
// unjittered. Pure functions (no device, no Streamline): the scene camera producer, the velocity collect and the tests share
// them. Row-vector convention (v' = v M), as velocity.h; jitter in render pixels, +x right, +y down.
#include "velocity.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <unordered_set>

namespace ds3native::upscaler {
// Halton radical inverse of `index` (from 1) in `base`.
inline double Halton(uint32_t index,uint32_t base) noexcept {
    double result=0,fraction=1;
    for(;index;index/=base){fraction/=base;result+=fraction*double(index%base);}
    return result;
}
// The sequence's length for a render-to-output ratio: ceil(8 (output / render)^2) (8 at DLAA, 18 at Quality).
inline uint32_t JitterPhases(uint32_t render_width,uint32_t output_width) noexcept {
    if(!render_width || !output_width)return 8;
    const double ratio=double(output_width)/double(render_width);
    return uint32_t(std::ceil(8.0*ratio*ratio-1e-9));
}
// The jitter of frame `serial`: Halton (2, 3) at index serial mod phases, plus one, minus 0.5, in render pixels.
inline std::array<float,2> Jitter(uint64_t serial,uint32_t phases) noexcept {
    const uint32_t index=uint32_t(serial%(phases?phases:1))+1;
    return {float(Halton(index,2)-0.5),float(Halton(index,3)-0.5)};
}
// The projection's off-centre words for a jitter at a render size (m20 = 2 jx / W, m21 = -2 jy / H: image y down, NDC y up).
// Zero jitter keeps the engine's -0.0 words bit for bit (ProduceSceneCamera).
inline std::array<float,2> JitterNdc(const std::array<float,2>& jitter,uint32_t width,uint32_t height) noexcept {
    if(jitter[0]==0 && jitter[1]==0)return {-0.0f,-0.0f};
    return {2.0f*jitter[0]/float(width),-2.0f*jitter[1]/float(height)};
}
// I-0116 tranche 1: a projection or a world-to-clip matrix the engine stored in a constant block (16 floats), shifted by the
// jitter's NDC offset as the scene camera's (clip.xy += ndc · clip.w), stored transposed (each register one output: x, y, z, w,
// as the camera's FC_MatrixProj; `layout` 1) or row by row (each register one input row; `layout` 2). With `layout` 0 (a pure
// projection) the layout is told by its w vector (0, 0, ±1, 0) (ProjectionLayout): 0, nothing written, when neither or both
// hold. Returns the layout shifted.
inline int ProjectionLayout(const float (&f)[16]) noexcept {
    const auto w=[](float a,float b,float c,float d){return a==0 && b==0 && (c==1 || c==-1) && d==0;};
    const bool transposed=w(f[12],f[13],f[14],f[15]),rows=w(f[3],f[7],f[11],f[15]);
    return transposed==rows?0:transposed?1:2;
}
// A pure projection's off-centre words (m20, m21) in its layout: zero (the engine's -0.0 included) when not already jittered.
inline bool ProjectionCentred(const float (&f)[16],int layout) noexcept {
    return layout==1?f[2]==0 && f[6]==0:layout==2?f[8]==0 && f[9]==0:false;
}
inline int ShiftProjectionWords(float (&f)[16],const std::array<float,2>& ndc,int layout=0) noexcept {
    if(!layout)layout=ProjectionLayout(f);
    if(layout!=1 && layout!=2)return 0;
    for(unsigned i=0;i<4;++i){
        if(layout==1){f[i]+=ndc[0]*f[12+i];f[4+i]+=ndc[1]*f[12+i];}
        else{f[i*4+0]+=ndc[0]*f[i*4+3];f[i*4+1]+=ndc[1]*f[i*4+3];}}
    return layout;
}
// I-0116 tranche 1 (second step review M-a; third M1, L1, L3): a world-to-clip block's layout told by what it is rather than by
// its declared class alone. Read in a layout as a matrix M (row vectors):
// - a view-projection (`world` false) is the scene camera's: Q = VP⁻¹·M (VP the camera's unjittered view-projection, its
//   `inverse` given) maps the camera's clip space to the block's, so its x, y and w columns are the identity's (its z column,
//   the depth mapping, may differ: another near or far plane); another view or field of view is refused, a jitter of the
//   game's own is not told (under the tolerance far from the origin; the blocks are the game's, unjittered);
// - a world-view-projection (`world` true, W·V·P with W the object's) has a perspective w: its z column is a times its w column
//   for the three object axes (clip.z = a·view.z + b), which only the right layout shows; its view is not checked.
// Within 1e-4 of the magnitudes summed. Returns the layout that holds alone, else 0.
inline int WorldClipLayout(const float (&f)[16],const VelocityMatrix& inverse,bool world) noexcept {
    int found=0;
    for(int layout=1;layout<=2;++layout){bool holds=true;
        const auto m=[&](unsigned r,unsigned c){return double(layout==1?f[c*4+r]:f[r*4+c]);};
        if(world){double w2=0,zw=0,z2=0;for(unsigned r=0;r<3;++r){w2+=m(r,3)*m(r,3);zw+=m(r,2)*m(r,3);z2+=m(r,2)*m(r,2);}
            const double a=w2>0?zw/w2:0;double residual=0;for(unsigned r=0;r<3;++r){const double d=m(r,2)-a*m(r,3);residual+=d*d;}
            holds=a!=0 && residual<=1e-8*z2;}
        else for(unsigned r=0;r<4 && holds;++r)for(unsigned c=0;c<4 && holds;++c){
            if(c==2)continue;
            double q=0,magnitude=1;for(unsigned k=0;k<4;++k){const double t=inverse[r*4+k]*m(k,c);q+=t;magnitude+=std::fabs(t);}
            holds=std::fabs(q-(r==c?1.0:0.0))<=1e-4*magnitude;}
        if(holds){if(found)return 0;found=layout;}}
    return found;
}
// The effects' jitter counters (21_effects.inc's rows), each once a block: projections shifted (by kind: pure, view-projection,
// world-view-projection), refused (not a perspective projection), outside their block, disagreeing with their declared class,
// unmatched (not the scene camera's unjittered matrix: a world-to-clip block in neither layout or not its view, a projection
// already off-centre, final review L2).
struct JitterCounts {uint64_t shifted[3]{},refused{},outside{},disagreed{},unmatched{};};
// A draw's projection site: its byte offset in its block, its kind (0 a pure projection, 1 a view-projection, 2 a
// world-view-projection) and its declared class's layout (column_major 1, row_major 2).
struct ProjectionSite {uint32_t offset{};int kind{};int declared{};};
// One site of a block (`range_offset`, `range_size`) in the frame's arena copy shifted by the scene camera's NDC offset, once a
// block: `done` holds the arena offsets of the sites already met, so a block two draws or two stages share is met once. A site
// beyond its block or the arena is counted outside and nothing is written; a world-to-clip site without the camera's matrices
// (`inverse` null) or not told by them, or a pure projection already off-centre, is unmatched and left. Returns the layout
// shifted, else 0.
inline int ShiftSite(uint8_t* arena,size_t arena_size,uint64_t range_offset,uint64_t range_size,const ProjectionSite& site,
    const std::array<float,2>& ndc,const VelocityMatrix* inverse,std::unordered_set<uint64_t>& done,JitterCounts& c) {
    const uint64_t at=range_offset+site.offset;if(!done.insert(at).second)return 0;
    if(!range_size || uint64_t(site.offset)+64>range_size || range_offset+range_size>arena_size){++c.outside;return 0;}
    float words[16]{};std::memcpy(words,arena+at,64);
    int layout=0;
    if(site.kind){if(!inverse || !(layout=WorldClipLayout(words,*inverse,site.kind==2))){++c.unmatched;return 0;}}
    else if((layout=ProjectionLayout(words)) && !ProjectionCentred(words,layout)){++c.unmatched;return 0;}
    if(!(layout=ShiftProjectionWords(words,ndc,layout))){++c.refused;return 0;}
    if(layout!=site.declared)++c.disagreed;
    std::memcpy(arena+at,words,64);++c.shifted[site.kind>=0 && site.kind<3?site.kind:0];return layout;
}
// A world-to-clip matrix shifted by the jitter's constant NDC offset: clip.xy += w (m20, m21), i.e. column i += m2i column 3.
inline VelocityMatrix ShiftClip(const VelocityMatrix& m,const std::array<float,2>& ndc) noexcept {
    VelocityMatrix r=m;
    for(unsigned row=0;row<4;++row){r[row*4+0]+=double(ndc[0])*m[row*4+3];r[row*4+1]+=double(ndc[1])*m[row*4+3];}
    return r;
}
// The resolve's current-to-previous screen matrix (VelocityCurrentToPrevious of the jittered current and the unjittered previous
// view-projection), with the jitter J = (jx / W, jy / H) added to its homogeneous output uv, so that the resolve's
// previous - uv is the unjittered previous position minus the unjittered current one (uv - J): §3.3's vector-resolve row.
inline void FoldJitter(Matrix4& m,const std::array<float,2>& jitter,uint32_t width,uint32_t height) noexcept {
    const float j[2]{jitter[0]/float(width),jitter[1]/float(height)};
    for(unsigned row=0;row<4;++row){m[row*4+0]+=j[0]*m[row*4+3];m[row*4+1]+=j[1]*m[row*4+3];}
}
// I-0140 tranche 40 (D-0305): a menu view's camera motion for its own TAA, from the view's own previous camera (`history`, one a view):
// the resolve's matrix as the main view's (the jittered current view-projection inverted, the previous unjittered one, the jitter
// folded back, so the vectors stay unjittered). `cut` (the caller's camera-cut rule) or no valid history: `reset`, the previous camera
// the current one (zero camera motion). False when the view cannot be projected or inverted: no motion this frame, the history
// dropped (the next frame resets). The history keeps the unjittered view-projection. `clip_to_previous_clip` (optional): the
// unjittered current clip -> previous clip, DLSS's constants (the collect's for the world, 12_scene_collect.inc).
struct MenuViewMotionHistory {VelocityMatrix view_projection{};bool valid{};};
inline bool MenuViewMotion(const SceneViewInput& camera,bool cut,MenuViewMotionHistory& history,Matrix4& current_to_previous,bool& reset,
    Matrix4* clip_to_previous_clip=nullptr) noexcept {
    VelocityMatrix current{};if(!VelocityViewProjection(camera,current)){history.valid=false;return false;}
    const std::array<float,2> jitter{camera.jitter_x,camera.jitter_y};const bool jittered=jitter[0]!=0 || jitter[1]!=0;
    reset=cut || !history.valid;const VelocityMatrix previous=reset?current:history.view_projection;
    if(!VelocityCurrentToPrevious(jittered?ShiftClip(current,JitterNdc(jitter,camera.width,camera.height)):current,previous,current_to_previous)){history.valid=false;return false;}
    if(jittered)FoldJitter(current_to_previous,jitter,camera.width,camera.height);
    if(clip_to_previous_clip){VelocityMatrix inverse{};if(!VelocityInverse(current,inverse)){history.valid=false;return false;}
        const auto m=VelocityMultiply(inverse,previous);for(unsigned i=0;i<16;++i)(*clip_to_previous_clip)[i]=static_cast<float>(m[i]);}
    history={current,true};return true;
}
}
