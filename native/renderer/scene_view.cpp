#include "scene_view.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace ds3native {
namespace {
bool Finite(const Matrix4& m) noexcept {
    return std::all_of(m.begin(),m.end(),[](float v){return std::isfinite(v);});
}
bool Inverse(const Matrix4& input,Matrix4& output) noexcept {
    double rows[4][8]{};
    for(unsigned r=0;r<4;++r) for(unsigned c=0;c<4;++c) {
        rows[r][c]=input[r*4+c]; rows[r][c+4]=r==c?1.0:0.0;
    }
    for(unsigned c=0;c<4;++c) {
        unsigned pivot=c;
        for(unsigned r=c+1;r<4;++r)
            if(std::abs(rows[r][c])>std::abs(rows[pivot][c])) pivot=r;
        if(std::abs(rows[pivot][c])<1e-12) return false;
        if(pivot!=c) for(unsigned j=0;j<8;++j) std::swap(rows[pivot][j],rows[c][j]);
        const double divisor=rows[c][c];
        for(unsigned j=0;j<8;++j) rows[c][j]/=divisor;
        for(unsigned r=0;r<4;++r) if(r!=c) {
            const double factor=rows[r][c];
            for(unsigned j=0;j<8;++j) rows[r][j]-=factor*rows[c][j];
        }
    }
    for(unsigned r=0;r<4;++r) for(unsigned c=0;c<4;++c)
        output[r*4+c]=static_cast<float>(rows[r][c+4]);
    return Finite(output);
}
}
Matrix4 Multiply(const Matrix4& left,const Matrix4& right) noexcept {
    Matrix4 result{};
    for(unsigned r=0;r<4;++r) for(unsigned c=0;c<4;++c)
        for(unsigned k=0;k<4;++k) result[r*4+c]+=left[r*4+k]*right[k*4+c];
    return result;
}
Matrix4 Transpose(const Matrix4& input) noexcept {
    Matrix4 result{};
    for(unsigned r=0;r<4;++r) for(unsigned c=0;c<4;++c) result[r*4+c]=input[c*4+r];
    return result;
}
std::array<float,4> Transform(const std::array<float,4>& row,const Matrix4& matrix) noexcept {
    std::array<float,4> result{};
    for(unsigned c=0;c<4;++c) for(unsigned k=0;k<4;++k) result[c]+=row[k]*matrix[k*4+c];
    return result;
}
bool BuildSceneView(const SceneViewInput& input,SceneView& output) noexcept {
    if(input.version!=1 || !input.serial || !input.width || !input.height ||
       input.width>16384 || input.height>16384 || input.projection_flags || input.tile_count ||
       !Finite(input.camera_pose) || !std::isfinite(input.vertical_fov) ||
       !std::isfinite(input.near_plane) || !std::isfinite(input.far_plane) ||
       !std::isfinite(input.explicit_aspect) || input.vertical_fov<=0 ||
       input.vertical_fov>=3.14159265358979323846f || input.near_plane<=0 ||
       input.far_plane<=input.near_plane || input.explicit_aspect<0 ||
       !(std::abs(input.jitter_x)<=0.5f) || !(std::abs(input.jitter_y)<=0.5f)) return false;
    const auto& pose=input.camera_pose;
    if(std::abs(pose[3])>1e-6f || std::abs(pose[7])>1e-6f ||
       std::abs(pose[11])>1e-6f || std::abs(pose[15]-1.0f)>1e-6f) return false;
    SceneView next{}; next.serial=input.serial; next.width=input.width; next.height=input.height;
    if(!Inverse(pose,next.view)) return false;
    const float aspect=input.explicit_aspect==0?
        static_cast<float>(input.width)/static_cast<float>(input.height):input.explicit_aspect;
    const float y=std::cos(input.vertical_fov*.5f)/std::sin(input.vertical_fov*.5f);
    const float z=input.far_plane/(input.far_plane-input.near_plane);
    next.projection={y/aspect,0,0,0, 0,y,0,0, 0,0,z,1, 0,0,-z*input.near_plane,0};
    next.view_projection=Multiply(next.view,next.projection);
    // Original material VS subtracts the camera world position before projection.
    // T(camera_position)*view*projection cancels that subtraction exactly.
    Matrix4 camera_translation={1,0,0,0, 0,1,0,0, 0,0,1,0, pose[12],pose[13],pose[14],1};
    next.camera_relative_view_projection=Multiply(camera_translation,next.view_projection);
    next.camera_position={pose[12],pose[13],pose[14],1};
    if(!Finite(next.projection) || !Finite(next.view_projection) ||
       !Finite(next.camera_relative_view_projection)) return false;
    output=next; return true;
}
}
