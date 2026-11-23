#pragma once
// I-0114 (camera_step, game_bootstrap 04): the display's view between two ticks of the follow camera's fixed 1/60 logic. A view
// is 20 floats, the follow camera's +0x10..0x5F: four rows of four (right, up, forward, position) and the lens (+0x50 the field
// of view, then near, far, aspect). Blend(previous, current, alpha) gives previous at alpha 0 and current at alpha 1 (the state
// shown runs one tick behind, as a fixed step's interpolation does): the rows blended and made orthonormal again (forward
// first, up against it, right their cross product turned to the blended right's side), the position and the field of view
// blended, every other float the current tick's. Header-only: unit-tested alone (view_blend_test).
#include <cmath>
#include <cstring>

namespace ds3native::view_blend {
inline void Blend(const float* previous,const float* current,float alpha,float* out) noexcept {
    std::memcpy(out,current,20*sizeof(float));
    const auto lerp=[&](int i){return previous[i]+(current[i]-previous[i])*alpha;};
    float r[3]{lerp(0),lerp(1),lerp(2)},u[3]{lerp(4),lerp(5),lerp(6)},f[3]{lerp(8),lerp(9),lerp(10)};
    const auto norm=[](float* v){const float n=std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);if(n>1e-6f){v[0]/=n;v[1]/=n;v[2]/=n;}};
    norm(f);const float uf=u[0]*f[0]+u[1]*f[1]+u[2]*f[2];for(int i=0;i<3;++i)u[i]-=uf*f[i];norm(u);
    float c[3]{u[1]*f[2]-u[2]*f[1],u[2]*f[0]-u[0]*f[2],u[0]*f[1]-u[1]*f[0]};
    if(c[0]*r[0]+c[1]*r[1]+c[2]*r[2]<0){c[0]=-c[0];c[1]=-c[1];c[2]=-c[2];}
    for(int i=0;i<3;++i){out[i]=c[i];out[4+i]=u[i];out[8+i]=f[i];out[12+i]=lerp(12+i);}
    out[16]=lerp(16);
}
}
