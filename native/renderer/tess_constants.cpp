#include "tess_constants.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace ds3native::tess {
namespace {
float F(const uint8_t* data,size_t i) {float f;std::memcpy(&f,data+4*i,4);return f;}
uint32_t U(const uint8_t* data,size_t i) {uint32_t u;std::memcpy(&u,data+4*i,4);return u;}
void Put(uint8_t* out,size_t i,float v) {std::memcpy(out+4*i,&v,4);}
}
bool FromGx04(const uint8_t* d,size_t words,Material& m) {
    if(words<9)return false;
    m.displacement=static_cast<uint8_t>(U(d,0)&1u);m.mode=U(d,1);m.edge=static_cast<int32_t>(F(d,2));m.inside=static_cast<int32_t>(F(d,3));
    m.d7=F(d,4);m.fade_start=F(d,5);m.fade_end=F(d,6);m.d10=F(d,7);m.d11=F(d,8);m.present=true;return true;
}
bool FromGx90(const uint8_t* d,size_t words,Material& m) {
    if(words<7)return false;
    m.displacement=1;m.mode=2;m.edge=static_cast<int32_t>(F(d,0));m.inside=static_cast<int32_t>(F(d,1));
    m.d7=F(d,2);m.fade_start=F(d,3);m.fade_end=F(d,4);m.d10=F(d,5);m.d11=F(d,6);m.present=true;m.gx90=true;return true;
}
bool PnTriangles(const Material& m,float exparam24,uint8_t out[0x30]) {
    if(!m.present || !m.mode)return false;
    std::memset(out,0,0x30);
    Put(out,0,std::min(static_cast<float>(static_cast<uint32_t>(m.edge)),64.0f));
    Put(out,1,std::min(static_cast<float>(static_cast<uint32_t>(m.inside)),64.0f));
    Put(out,2,m.fade_start);
    const float range=m.fade_end-m.fade_start;Put(out,3,range<=0.001f?0.001f:range);
    Put(out,4,1.0f-m.d7);Put(out,5,std::min(m.mat264,0.99f));Put(out,6,1.0f);Put(out,7,static_cast<float>(m.mat268));
    Put(out,8,m.displacement?(m.gx90?exparam24:1.0f)*m.d11:0.0f);Put(out,9,m.d10);
    return true;
}
void TessScene(const std::array<float,16>& vp,const std::array<float,3>& eye,const std::array<float,3>& forward,float clock,uint8_t out[0xB4]) {
    std::memset(out,0,0xB4);
    // transpose(view x projection): row r of the block = column r of the row-vector matrix.
    for(size_t r=0;r<4;++r)for(size_t c=0;c<4;++c)Put(out,4*r+c,vp[4*c+r]);
    Put(out,16,eye[0]);Put(out,17,eye[1]);Put(out,18,eye[2]);Put(out,19,1.0f);
    const float n=std::sqrt(forward[0]*forward[0]+forward[1]*forward[1]+forward[2]*forward[2]);
    for(size_t k=0;k<3;++k)Put(out,20+k,n>0.0f?-forward[k]/n:0.0f);
    Put(out,24,1.0f);
    // Column-vector rows of the matrix = the transposed block rows 0..3.
    const auto row=[&](size_t r,size_t c){return vp[4*c+r];};
    const float sign[4]{1,-1,1,-1};const size_t axis[4]{0,0,1,1};
    for(size_t p=0;p<4;++p){float plane[4];for(size_t c=0;c<4;++c)plane[c]=row(3,c)+sign[p]*row(axis[p],c);
        const float length=std::sqrt(plane[0]*plane[0]+plane[1]*plane[1]+plane[2]*plane[2]);
        for(size_t c=0;c<4;++c)Put(out,28+4*p+c,length>0.0f?plane[c]/length:0.0f);}
    Put(out,44,clock);
}
void AbsoluteViewProjection(const uint8_t* scene,std::array<float,16>& vp,std::array<float,3>& eye) {
    for(size_t k=0;k<3;++k)eye[k]=F(scene,12+k);
    for(size_t r=0;r<4;++r){float row[4];for(size_t k=0;k<4;++k)row[k]=F(scene,96+4*r+k);
        row[3]-=row[0]*eye[0]+row[1]*eye[1]+row[2]*eye[2];
        for(size_t k=0;k<4;++k)vp[4*k+r]=row[k];}   // row-vector: column r is stored row r
}
}
