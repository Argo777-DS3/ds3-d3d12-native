#pragma once
// I-0113 tranche 14 (a world object's region IBL, RESULT « Tranche 14 »): the region choice 5C0E0 makes for an object's rows when
// the scene context's +0x3ECF is set, on a copy of the region manager root+0x42B0 taken once per collect (game_bootstrap 07). A
// region's box is its 20 floats at +0x10: the half extents (4), three axis rows (4 each), the centre (4). 66C80 picks the last
// listed region whose box holds the point and whose handle's masks meet the view's (+0x4B8/+0x4BC), else the default handle's
// region; 66DB0 gives the blend (1 outside that region's box, 0 when its fade +0xCC is not positive, else 1 minus the clamped
// ratio of the distance to its nearest face, at most the fade, over the fade); with a positive blend 5C0E0 takes as second region
// the last listed one of a lower priority (+0xD0) that passes the same tests, else the default's; otherwise the first again.
// Header-only: unit-tested alone (env_region_test).
#include <cmath>
#include <cstdint>
#include <vector>

namespace ds3native::env_region {
struct Region {uint32_t key[2]{};float box[20]{};float fade{};uint32_t priority{};uint32_t masks[2]{};bool handle{};};
// 1417EFEA0.
inline bool Inside(const float* b,const float* p) noexcept {
    const float x=p[0]-b[16],y=p[1]-b[17],z=p[2]-b[18];
    const float l0=b[4]*x+b[5]*y+b[6]*z,l1=b[8]*x+b[9]*y+b[10]*z,l2=b[12]*x+b[13]*y+b[14]*z;
    return -b[0]<=l0 && l0<=b[0] && -b[1]<=l1 && l1<=b[1] && -b[2]<=l2 && l2<=b[2] && -b[3]<=0.f && 0.f<=b[3];
}
// 1421090C0: the squared distance from p to the face {edge u, edge v, corner o}, each edge's parameter clamped on its own.
inline float FaceDistanceSquared(const float* p,const float* u,const float* v,const float* o) noexcept {
    const float d[3]{o[0]-p[0],o[1]-p[1],o[2]-p[2]};
    const float uu=u[0]*u[0]+u[1]*u[1]+u[2]*u[2],vv=v[0]*v[0]+v[1]*v[1]+v[2]*v[2];
    const float ud=u[0]*d[0]+u[1]*d[1]+u[2]*d[2],vd=v[0]*d[0]+v[1]*d[1]+v[2]*d[2];
    float s=d[0]*d[0]+d[1]*d[1]+d[2]*d[2];
    if(0.f<=-ud)s=uu< -ud?ud*2.f+uu+s:(-ud/uu)*ud+s;
    if(0.f<=-vd)s=vv< -vd?s+vd*2.f+vv:s+(-vd/vv)*vd;
    return std::fabs(s);
}
// 66DB0 (with 1417F0630's six faces of the box).
inline float Blend(const Region& r,const float* p) noexcept {
    if(!Inside(r.box,p))return 1.f;
    if(!(0.f<r.fade) && !std::isnan(r.fade))return 0.f;
    const float* b=r.box;float a[3][3];
    for(int k=0;k<3;++k)for(int i=0;i<3;++i)a[k][i]=b[k]*b[4+4*k+i];
    float u0[3],u1[3],u2[3],p000[3],p100[3],p010[3],p001[3],p101[3],n0[3],n2[3];
    for(int i=0;i<3;++i){u0[i]=2.f*a[0][i];u1[i]=2.f*a[1][i];u2[i]=2.f*a[2][i];n0[i]=-u0[i];n2[i]=-u2[i];
        p000[i]=b[16+i]-a[0][i]-a[1][i]-a[2][i];p100[i]=b[16+i]+a[0][i]-a[1][i]-a[2][i];p010[i]=b[16+i]-a[0][i]+a[1][i]-a[2][i];
        p001[i]=b[16+i]-a[0][i]-a[1][i]+a[2][i];p101[i]=b[16+i]+a[0][i]-a[1][i]+a[2][i];}
    const float* faces[6][3]{{u2,u0,p000},{u0,u1,p000},{u2,u1,p100},{n0,u1,p101},{n2,u1,p001},{u0,u2,p010}};
    float nearest=r.fade;
    for(const auto& f:faces){const float d=std::sqrt(FaceDistanceSquared(p,f[0],f[1],f[2]));if(nearest>d)nearest=d;}
    const float t=nearest/r.fade;
    return 1.f-(t<0.f?0.f:(t>1.f?1.f:t));   // unordered (a NaN fade) keeps t, as CMOVBE does
}
struct Choice {const Region* first{};const Region* second{};float blend{};};
// 66C80 and 5C0E0's priority search; `fallback` is the default handle's region.
inline Choice Choose(const std::vector<Region>& listed,const Region& fallback,const float* p,const uint32_t* view) noexcept {
    const auto passes=[&](const Region& r){return Inside(r.box,p) && r.handle && (view[0]&r.masks[0]) && (r.masks[1]&view[1]);};
    Choice c{&fallback,nullptr,0.f};
    for(auto r=listed.rbegin();r!=listed.rend();++r)if(passes(*r)){c.first=&*r;break;}
    c.blend=Blend(*c.first,p);c.second=c.first;
    if(0.f<c.blend){c.second=&fallback;
        for(auto r=listed.rbegin();r!=listed.rend();++r)if(r->priority<c.first->priority && passes(*r)){c.second=&*r;break;}}
    return c;
}
}
