#pragma once
// I-0132 R12a (STUDY-R12-SETTINGS-PAGE §3.2): a glyph's outline (font_outline.h) made into an 8-bit coverage image, the menus'
// drop shadow beside it (a blurred copy), and a shelf atlas that keeps them for the Rekindled page's text. The coverage is the exact
// signed area of the outline in each pixel (the accumulation method of font-rs: each line adds its covered area to a row's cells,
// a running sum turns it into coverage; the curves flattened to lines), nonzero-like: |sum| clamped to 1. Header-only, unit-tested
// alone (font_raster_test); no Windows and no D3D12 here.
#include "font_outline.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace ds3native::font {
struct Coverage {
    uint32_t width{},height{};
    float left{},top{};            // the image's top-left in pixels from the pen (x right, y down; top is negative above the baseline)
    std::vector<uint8_t> pixels;   // width*height, 0 to 255
};

namespace detail {
// font-rs's accumulation: a line from a to b (pixels, y down) adds its signed area to the cells it crosses.
inline void Line(std::vector<float>& acc,uint32_t w,uint32_t h,Point a,Point b) noexcept {
    if(a.y==b.y)return;
    const float dir=a.y<b.y?1.f:-1.f;if(dir<0)std::swap(a,b);
    const float dxdy=(b.x-a.x)/(b.y-a.y);float x=a.x;
    const int y0=(std::max)(0,int(std::floor(a.y))),y1=(std::min)(int(h),int(std::ceil(b.y)));
    if(a.y<0)x-=a.y*dxdy;
    for(int y=y0;y<y1;++y){
        const float dy=(std::min)(float(y+1),b.y)-(std::max)(float(y),a.y);
        const float xnext=x+dxdy*dy,d=dy*dir;
        float x0=x,x1=xnext;if(x0>x1)std::swap(x0,x1);
        const size_t row=size_t(y)*(w+2);
        const float x0f=std::floor(x0);const int x0i=int(x0f);const float x0floor=x0f,x1ceil=std::ceil(x1);const int x1i=int(x1ceil);
        auto add=[&](int i,float v){const int c=(std::clamp)(i,0,int(w)+1);acc[row+size_t(c)]+=v;};
        if(x1i<=x0i+1){const float xmf=0.5f*(x+xnext)-x0floor;add(x0i,d-d*xmf);add(x0i+1,d*xmf);}
        else{const float s=1.f/(x1-x0),x0f2=x0-x0floor,a0=0.5f*s*(1-x0f2)*(1-x0f2),x1f=x1-x1ceil+1,am=0.5f*s*x1f*x1f;
            add(x0i,d*a0);
            if(x1i==x0i+2)add(x0i+1,d*(1-a0-am));
            else{const float a1=s*(1.5f-x0f2);add(x0i+1,d*(a1-a0));for(int xi=x0i+2;xi<x1i-1;++xi)add(xi,d*s);
                const float a2=a1+float(x1i-x0i-3)*s;add(x1i-1,d*(1-a2-am));}
            add(x1i,d*am);}
        x=xnext;}
}
}

// One glyph at `pixels_per_em`, with `pad` empty pixels around it (room for the shadow's blur). An empty glyph (a space) gives a
// zero-sized image.
inline Coverage Rasterize(const Glyph& glyph,float pixels_per_em,uint32_t pad=0) {
    Coverage out;const float scale=pixels_per_em/kEmUnits;
    float minx=1e30f,miny=1e30f,maxx=-1e30f,maxy=-1e30f;
    const auto grow=[&](Point p){minx=(std::min)(minx,p.x);miny=(std::min)(miny,p.y);maxx=(std::max)(maxx,p.x);maxy=(std::max)(maxy,p.y);};
    for(const auto& c:glyph.contours){grow(c.start);for(const auto& s:c.segments){grow(s.to);if(s.curve)grow(s.control);}}
    if(minx>maxx)return out;
    const float left=std::floor(minx*scale)-float(pad),top=std::floor(miny*scale)-float(pad);
    out.left=left;out.top=top;
    out.width=uint32_t(std::ceil(maxx*scale)-left)+pad;out.height=uint32_t(std::ceil(maxy*scale)-top)+pad;
    if(out.width>4096 || out.height>4096){out={};return out;}
    std::vector<float> acc(size_t(out.width+2)*out.height+1,0.f);
    const auto at=[&](Point p){return Point{p.x*scale-left,p.y*scale-top};};
    for(const auto& c:glyph.contours){
        Point pen=at(c.start);
        for(const auto& s:c.segments){
            const Point to=at(s.to);
            if(!s.curve){detail::Line(acc,out.width,out.height,pen,to);pen=to;continue;}
            const Point ctl=at(s.control);
            const float dd=std::hypot(pen.x-2*ctl.x+to.x,pen.y-2*ctl.y+to.y);
            const int n=(std::clamp)(int(std::ceil(std::sqrt(dd*4.f))),1,64);
            Point prev=pen;
            for(int i=1;i<=n;++i){const float t=float(i)/float(n),u=1-t;
                const Point p{u*u*pen.x+2*u*t*ctl.x+t*t*to.x,u*u*pen.y+2*u*t*ctl.y+t*t*to.y};detail::Line(acc,out.width,out.height,prev,p);prev=p;}
            pen=to;}
        const Point start=at(c.start);if(pen.x!=start.x || pen.y!=start.y)detail::Line(acc,out.width,out.height,pen,start);   // close it
    }
    out.pixels.resize(size_t(out.width)*out.height);
    for(uint32_t y=0;y<out.height;++y){float sum=0;const size_t row=size_t(y)*(out.width+2);
        for(uint32_t x=0;x<out.width;++x){sum+=acc[row+x];out.pixels[size_t(y)*out.width+x]=uint8_t(std::lround((std::min)(1.f,std::fabs(sum))*255.f));}}
    return out;}

// The menus' drop shadow (MENUS-MAP §78.2: black, blurred, distance 2): a box blur of `radius` pixels of the coverage (a 3x3 box at
// radius 1), the same size (the glyph needs pad >= radius).
inline std::vector<uint8_t> Blur(const Coverage& c,uint32_t radius) {
    std::vector<uint8_t> out(c.pixels.size());if(!radius){out=c.pixels;return out;}
    std::vector<uint32_t> tmp(c.pixels.size());const int r=int(radius),w=int(c.width),h=int(c.height),n=2*r+1;
    for(int y=0;y<h;++y)for(int x=0;x<w;++x){uint32_t s=0;for(int k=-r;k<=r;++k){const int xx=x+k;if(xx>=0 && xx<w)s+=c.pixels[size_t(y)*w+xx];}tmp[size_t(y)*w+x]=s;}
    for(int y=0;y<h;++y)for(int x=0;x<w;++x){uint32_t s=0;for(int k=-r;k<=r;++k){const int yy=y+k;if(yy>=0 && yy<h)s+=tmp[size_t(yy)*w+x];}
        out[size_t(y)*w+x]=uint8_t((s+uint32_t(n*n/2))/uint32_t(n*n));}
    return out;}

// A shelf packer over one square atlas; a full atlas refuses (the caller starts a new page or drops a glyph and says so).
struct Shelf {
    uint32_t size{};uint32_t x{},y{},row{};
    explicit Shelf(uint32_t s):size(s){}
    bool Place(uint32_t w,uint32_t h,uint32_t& ox,uint32_t& oy) noexcept {
        if(w>size || h>size)return false;
        if(x+w>size){y+=row+1;x=0;row=0;}
        if(y+h>size)return false;
        ox=x;oy=y;x+=w+1;row=(std::max)(row,h);return true;}
};
}
