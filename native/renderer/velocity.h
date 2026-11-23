#pragma once
#include "scene_view.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

namespace ds3native {
// I-0066 1186, velocity family (I-0068 VELOCITY-MOTIONBLUR-NATIVE, VELOCITY-IMPLEMENTATION-CHECKLIST): the camera matrices
// of the object Velo rows and of the native motion-vector resolve. Row-vector convention (v' = v M), unjittered.
using VelocityMatrix=std::array<double,16>;

inline VelocityMatrix VelocityMultiply(const VelocityMatrix& a,const VelocityMatrix& b) noexcept {
    VelocityMatrix r{};
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)for(unsigned k=0;k<4;++k)r[i*4+j]+=a[i*4+k]*b[k*4+j];
    return r;
}
inline bool VelocityInverse(const VelocityMatrix& input,VelocityMatrix& output) noexcept {
    double rows[4][8]{};
    for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c){rows[r][c]=input[r*4+c];rows[r][c+4]=r==c?1.0:0.0;}
    for(unsigned c=0;c<4;++c){
        unsigned pivot=c;
        for(unsigned r=c+1;r<4;++r)if(std::abs(rows[r][c])>std::abs(rows[pivot][c]))pivot=r;
        if(std::abs(rows[pivot][c])<1e-300)return false;
        if(pivot!=c)for(unsigned j=0;j<8;++j)std::swap(rows[pivot][j],rows[c][j]);
        const double divisor=rows[c][c];for(unsigned j=0;j<8;++j)rows[c][j]/=divisor;
        for(unsigned r=0;r<4;++r)if(r!=c){const double factor=rows[r][c];for(unsigned j=0;j<8;++j)rows[r][j]-=factor*rows[c][j];}
    }
    for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)output[r*4+c]=rows[r][c+4];
    for(const auto value:output)if(!std::isfinite(value))return false;
    return true;
}
// World -> view and view -> clip of a selected view (BuildSceneView's lens: left-handed, depth 0 at near, 1 at far), in double.
inline bool VelocityViewAndProjection(const SceneViewInput& input,VelocityMatrix& view,VelocityMatrix& projection) noexcept {
    SceneView checked{};if(!BuildSceneView(input,checked))return false;
    VelocityMatrix pose{};for(unsigned i=0;i<16;++i)pose[i]=input.camera_pose[i];
    if(!VelocityInverse(pose,view))return false;
    const double aspect=input.explicit_aspect==0?double(input.width)/double(input.height):double(input.explicit_aspect);
    const double y=std::cos(double(input.vertical_fov)*.5)/std::sin(double(input.vertical_fov)*.5);
    const double z=double(input.far_plane)/(double(input.far_plane)-double(input.near_plane));
    projection={y/aspect,0,0,0, 0,y,0,0, 0,0,z,1, 0,0,-z*double(input.near_plane),0};
    return true;
}
// World -> clip of a selected view, in double.
inline bool VelocityViewProjection(const SceneViewInput& input,VelocityMatrix& output) noexcept {
    VelocityMatrix view{},projection{};if(!VelocityViewAndProjection(input,view,projection))return false;
    output=VelocityMultiply(view,projection);return true;
}
// 13BFC0's g_mCurToPrevScreen in the same convention: (u, v, depth, 1) of the current frame -> the previous frame's
// homogeneous texture coordinate (u' w, v' w, ., w): NDC from uv (x = 2u - 1, y = 1 - 2v), the inverse current
// view-projection, the previous view-projection, then uv from NDC.
inline bool VelocityCurrentToPrevious(const VelocityMatrix& current,const VelocityMatrix& previous,Matrix4& output) noexcept {
    const VelocityMatrix from_uv{2,0,0,0, 0,-2,0,0, 0,0,1,0, -1,1,0,1};
    const VelocityMatrix to_uv{.5,0,0,0, 0,-.5,0,0, 0,0,1,0, .5,.5,0,1};
    VelocityMatrix inverse{};if(!VelocityInverse(current,inverse))return false;
    const auto result=VelocityMultiply(VelocityMultiply(VelocityMultiply(from_uv,inverse),previous),to_uv);
    for(unsigned i=0;i<16;++i){output[i]=static_cast<float>(result[i]);if(!std::isfinite(output[i]))return false;}
    return true;
}
// Velo VS b0 cbMatrixWVP (192 bytes): VC_MatrixWVP @0 and VC_PrevMatrixWVP @64 as the four columns each (the VS forms
// clip.i = dp4(position, cb0[i])), VC_NowToPrevView @128 (declared, never read) left zero.
inline std::array<uint8_t,192> VelocityMatrixBlock(const VelocityMatrix& current,const VelocityMatrix& previous) noexcept {
    std::array<uint8_t,192> bytes{};
    for(unsigned m=0;m<2;++m){const auto& source=m?previous:current;
        for(unsigned column=0;column<4;++column)for(unsigned row=0;row<4;++row){
            const float value=static_cast<float>(source[row*4+column]);std::memcpy(bytes.data()+64*m+16*column+4*row,&value,4);}}
    return bytes;
}
// 5A120 case 5 (the Velo preparation): draw state +0x310 = FC_DifColMul = (aspect*0.5*k, -0.5*k, 0, 1), aspect = camera+0x64
// when non-zero else width/height, k = 1 / (view+0xCA4 when positive, else the .rdata float 143D7ECF4 = 1.0E-5, read by the caller).
inline std::array<float,4> VelocityScale(float aspect,float view_scale,float epsilon) noexcept {
    const float k=1.0f/(view_scale>0.0f?view_scale:epsilon);
    return {aspect*0.5f*k,-0.5f*k,0.0f,1.0f};
}
}
