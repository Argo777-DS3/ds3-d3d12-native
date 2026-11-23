#include "light_tiles.h"
#include "light_producers.h"
#include <cmath>
#include <cstring>
#include <xmmintrin.h>

namespace ds3tiles {
namespace {
struct Constants {
    __m128 origin, xyz, m13, len_bad, len_fill, w_one, v[4]; float half{}, two{}, one{}; bool ok{};
    Constants() {
        auto vec=[&](uint32_t rva,__m128& out) { float f[4]{}; ok=ok && ds3light::Rdata(rva,f,16); out=_mm_loadu_ps(f); };
        auto sc=[&](uint32_t rva,float& out) { ok=ok && ds3light::Rdata(rva,&out,4); };
        ok=true;
        vec(0x39F2490,origin); vec(0x39F2720,xyz); vec(0x3D1C0E0,m13); vec(0x3D1C160,len_bad); vec(0x3D1C170,len_fill); vec(0x39F2500,w_one);
        vec(0x3D7F390,v[0]); vec(0x3D7F380,v[1]); vec(0x3D7F4C0,v[2]); vec(0x3D7F4B0,v[3]);
        sc(0x3D7EDB4,half); sc(0x3D7EE7C,two); sc(0x3D7EE10,one);
    }
};
const Constants& K() { static const Constants k; return k; }
#define SH(a,b,i) _mm_shuffle_ps(a,b,i)
// Normalise a cross product and append the plane distance (the sequence after every cross in 140234E40).
__m128 Finish(__m128 c) {
    const Constants& k=K(); const __m128 zero=_mm_setzero_ps();
    __m128 s=_mm_mul_ps(c,c); __m128 t=SH(s,s,0xEE); s=_mm_add_ps(s,t); s=SH(s,s,0x40); t=SH(t,s,0xF0); s=_mm_add_ps(s,t); s=SH(s,s,0xAA);
    const __m128 len=_mm_sqrt_ps(s); const __m128 good=_mm_cmpneq_ps(s,k.len_bad); c=_mm_div_ps(c,len);
    c=_mm_and_ps(c,_mm_cmpneq_ps(zero,len)); c=_mm_and_ps(c,good); c=_mm_or_ps(c,_mm_andnot_ps(good,k.len_fill));
    __m128 d=_mm_mul_ps(c,k.origin); __m128 d1=SH(d,d,0x99); d=_mm_add_ss(d,d1); d=_mm_add_ss(d,SH(d1,d1,0x55)); d=SH(d,d,0);
    return _mm_or_ps(_mm_andnot_ps(k.xyz,_mm_sub_ps(zero,d)),_mm_and_ps(c,k.xyz));
}
// Plane through the origin vector and the points a, b (a feeds the 0xD2 shuffle, b the 0xC9 one).
__m128 PlaneAB(__m128 a,__m128 b) {
    const Constants& k=K(); a=_mm_sub_ps(a,k.origin); b=_mm_sub_ps(b,k.origin);
    a=SH(a,a,0xD2); b=SH(b,b,0xC9); const __m128 a2=SH(a,a,0xD2), b2=SH(b,b,0xC9);
    __m128 c=_mm_sub_ps(_mm_mul_ps(a,b),_mm_mul_ps(a2,b2));
    c=_mm_and_ps(_mm_and_ps(c,k.m13),k.xyz); c=_mm_or_ps(c,_mm_andnot_ps(k.xyz,_mm_setzero_ps()));
    return Finish(c);
}
// Plane through the point p (relative to the origin vector) and the constant direction v: cross(cross(p, v), p).
__m128 PlaneV(__m128 p,const __m128& v) {
    const Constants& k=K(); p=_mm_sub_ps(p,k.origin);
    const __m128 vs=SH(v,v,0xC9), vss=SH(vs,vs,0xC9);
    __m128 pd=SH(p,p,0xD2); const __m128 pc=SH(p,p,0xC9); const __m128 pdd=SH(pd,pd,0xD2);
    __m128 t=_mm_sub_ps(_mm_mul_ps(pd,vs),_mm_mul_ps(pdd,vss)); t=_mm_and_ps(t,k.m13);
    t=SH(t,t,0xD2); const __m128 t2=SH(t,t,0xD2), pcc=SH(pc,pc,0xC9);
    __m128 c=_mm_sub_ps(_mm_mul_ps(t,pc),_mm_mul_ps(t2,pcc)); c=_mm_and_ps(_mm_and_ps(c,k.m13),k.xyz);
    return Finish(c);
}
__m128 Point(float x,float y,float one) { return _mm_setr_ps(x,y,one,one); }
// 2442D0: 0 = outside some plane.
bool Inside(const Sphere& s,const __m128* planes,unsigned n) {
    const Constants& k=K(); __m128 p=_mm_setr_ps(s.x,s.y,s.z,s.r); p=_mm_add_ps(_mm_and_ps(p,k.xyz),k.w_one);
    for(unsigned i=0;i<n;++i) { const __m128 m=_mm_mul_ps(p,planes[i]); __m128 a=SH(p,m,0x40); a=_mm_add_ps(a,m); __m128 b=SH(m,a,0x30); b=_mm_add_ps(b,a);
        if(_mm_cvtss_f32(SH(b,b,0xAA))>s.r) return false; }
    return true;
}
// 140244520: false when every transformed point lies on the positive side of one plane (the decompiled sum order).
bool InsideHull(const Hull& h,const __m128* planes,unsigned n) {
    if(h.all) return true;   /* diagnostic (LH_SPOT_ALL) */
    if(!h.count) return false;   /* 140244520: no point means every plane counts all (zero) points outside: rejected */
    for(unsigned i=0;i<n;++i) { float pl[4]; _mm_storeu_ps(pl,planes[i]); uint32_t out=0;
        for(uint32_t k=0;k<h.count && k<8;++k) { const float* q=h.points[k]; const float x=q[0], y=q[1], z=q[2], w=1.0f; const float* m=h.m;
            const float a=x*m[0]+y*m[4]+z*m[8]+w*m[12], b=x*m[1]+y*m[5]+z*m[9]+w*m[13], c=x*m[2]+y*m[6]+z*m[10]+w*m[14], d=x*m[3]+y*m[7]+z*m[11]+w*m[15];
            if(0.0f<pl[1]*b+pl[3]*d+pl[0]*a+pl[2]*c) ++out; }
        if(out==h.count) return false; }
    return true;
}
}

bool Bin(const View& view,const std::vector<Sphere>& lights,uint32_t tw,uint32_t th,int x0,int y0,int x1,int y1,std::vector<std::vector<uint32_t>>& tiles,uint32_t& columns) {
    return Bin(view,lights,{},tw,th,x0,y0,x1,y1,tiles,columns); }
bool Bin(const View& view,const std::vector<Sphere>& lights,const std::vector<Hull>& hulls,uint32_t tw,uint32_t th,int x0,int y0,int x1,int y1,std::vector<std::vector<uint32_t>>& tiles,uint32_t& columns) {
    const Constants& k=K(); if(!k.ok || !tw || !th) return false;
    auto Test=[&](uint32_t i,const __m128* planes,unsigned n) { return i<hulls.size() && hulls[i].valid ? InsideHull(hulls[i],planes,n) : Inside(lights[i],planes,n); };
    const uint32_t w2=view.width+2*view.border, h2=view.height+2*view.border;
    if(!x1) x1=static_cast<int>(w2); if(!y1) y1=static_cast<int>(h2);
    columns=(w2-1+tw)/tw; const uint32_t rows=(h2-1+th)/th; tiles.assign(static_cast<size_t>(columns)*rows,{});
    float aspect=view.aspect; if(aspect==0.0f) aspect=static_cast<float>(static_cast<int64_t>(view.aspect_w))/static_cast<float>(static_cast<int64_t>(view.aspect_h));
    const float tan_y=std::tan(view.fov*k.half), right=aspect*tan_y;
    const bool all=view.views>1;
    float left,width,height,top=tan_y;
    if(all) {   // several views: the multi-view frustum split (every light is kept, the planes are unused)
        left=-right; width=right*k.two; height=tan_y*k.two;
    } else { uint32_t lw; const float l=right; std::memcpy(&lw,&l,4); lw^=0x80000000u; std::memcpy(&left,&lw,4); width=right*k.two; height=tan_y*k.two; }
    const float hf=static_cast<float>(static_cast<int64_t>(h2)), wf=static_cast<float>(static_cast<int64_t>(w2));
    for(int y=y0;y<y1;y+=static_cast<int>(th)) {
        const float yb=top-(static_cast<float>(y+static_cast<int>(th))*height)/hf, yt=top-(static_cast<float>(y)*height)/hf;
        __m128 row[2]={PlaneAB(Point(right,yt,k.one),Point(left,yt,k.one)),PlaneAB(Point(left,yb,k.one),Point(right,yb,k.one))};
        std::vector<uint32_t> kept; for(uint32_t i=0;i<lights.size();++i) if(all || Test(i,row,2)) kept.push_back(i);
        if(kept.empty()) continue;
        uint32_t tile=static_cast<uint32_t>(y/static_cast<int>(th))*columns+static_cast<uint32_t>(x0/static_cast<int>(tw));
        for(int x=x0;x<x1;x+=static_cast<int>(tw),++tile) {
            const float xr=(static_cast<float>(x+static_cast<int>(tw))*width)/wf+left, xl=(static_cast<float>(x)*width)/wf+left;
            const __m128 rt=Point(xr,yt,k.one), rb=Point(xr,yb,k.one), lt=Point(xl,yt,k.one), lb=Point(xl,yb,k.one);
            __m128 p[8]={row[0],row[1],PlaneAB(rb,rt),PlaneAB(lt,lb),PlaneV(lt,k.v[0]),PlaneV(rt,k.v[1]),PlaneV(lb,k.v[2]),PlaneV(rb,k.v[3])};
            if(tile>=tiles.size()) break;
            for(uint32_t i:kept) if(all || Test(i,p,8)) tiles[tile].push_back(i);
        }
    }
    return true;
}
}
