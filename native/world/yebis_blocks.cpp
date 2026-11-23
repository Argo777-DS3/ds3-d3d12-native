#include "yebis_blocks.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace ds3native::yebis {
namespace {
float Float(uint32_t bits) {float f;std::memcpy(&f,&bits,4);return f;}
uint32_t Bits(float f) {uint32_t u;std::memcpy(&u,&f,4);return u;}
uint32_t Lerp(uint32_t a,uint32_t b,float t) {const float x=Float(a),y=Float(b);return Bits((y-x)*t+x);}
template<size_t N> std::array<uint32_t,N> Blend(const std::array<uint32_t,N>& a,const std::array<uint32_t,N>& b,float t,float eps,
    std::initializer_list<size_t> copied) {
    if(std::fabs(t)<=eps)return a;
    if(std::fabs(t-1.0f)<=eps)return b;
    std::array<uint32_t,N> r{};for(size_t i=0;i<N;++i)r[i]=Lerp(a[i],b[i],t);
    for(const size_t i:copied)r[i]=b[i];
    return r;
}
}
bool ToneBlock(const Entries& entries,std::array<uint32_t,35>& out) {
    if(entries.size()<35)return false;
    std::array<uint32_t,35> words{};for(size_t i=0;i<35;++i)words[i]=entries[i].raw[0];
    out=words;return true;
}
bool BloomBlock(const Entries& entries,std::array<uint32_t,42>& out) {
    std::vector<uint32_t> f;
    for(size_t i=0;i<36 && i<entries.size();++i)for(uint8_t k=0;k<entries[i].words && k<4;++k)f.push_back(entries[i].raw[k]);
    if(f.size()!=42)return false;
    std::array<uint32_t,42> e{};
    for(size_t j=0;j<4;++j)e[j]=f[j];
    e[4]=0x3F800000u;
    for(size_t j=4;j<12;++j)e[j+1]=f[j];
    e[13]=f[30];e[14]=f[31];e[15]=f[32];
    for(size_t j=33;j<41;++j)e[j-17]=f[j];
    for(size_t j=12;j<30;++j)e[j+12]=f[j];
    out=e;return true;
}
bool SettingBlock(const Entries& entries,std::array<uint32_t,21>& out) {
    std::array<uint32_t,21> w{};size_t index{};bool scale=false;
    for(const auto& e:entries){
        if(e.type==3){if(index<18)w[index]=e.raw[0];++index;}
        else if(e.name=="SceneLuminanceScale"){w[20]=e.raw[0];scale=true;}}
    if(index<18 || !scale)return false;
    out=w;return true;
}
std::array<uint32_t,35> BlendTone(const std::array<uint32_t,35>& a,const std::array<uint32_t,35>& b,float t,float eps) {
    return Blend(a,b,t,eps,{2,9,28,29,34});
}
std::array<uint32_t,42> BlendBloom(const std::array<uint32_t,42>& a,const std::array<uint32_t,42>& b,float t,float eps) {
    return Blend(a,b,t,eps,{8,9,13,27,31,33,36,39});
}
std::array<uint32_t,21> BlendSetting(const std::array<uint32_t,21>& a,const std::array<uint32_t,21>& b,float t,float eps) {
    if(std::fabs(t)<=eps)return a;
    if(std::fabs(t-1.0f)<=eps)return b;
    std::array<uint32_t,21> r=b;r[20]=Lerp(a[20],b[20],t);return r;
}
}
namespace ds3native::yebis {
bool LightShaftBlock(const Entries& entries,std::array<uint32_t,28>& out) {
    if(entries.size()<15 || entries[1].words!=4 || entries[14].words!=4)return false;
    for(size_t i:{0u,2u,3u,4u,5u,6u,7u,8u,9u,10u,11u,12u,13u})if(entries[i].words!=1)return false;   // the shipped layout only
    std::array<uint32_t,28> w{};
    w[0]=entries[0].raw[0];
    for(size_t k=0;k<4;++k)w[4+k]=entries[1].raw[k];
    w[8]=entries[2].raw[0];w[9]=0;w[10]=0x3F7FFF58u;w[11]=entries[3].raw[0];
    for(size_t i=4;i<=13;++i)w[8+i]=entries[i].raw[0];   // entries 4..13 -> words 12..21
    for(size_t k=0;k<4;++k)w[24+k]=entries[14].raw[k];
    out=w;return true;
}
std::array<uint32_t,28> BlendLightShaft(const std::array<uint32_t,28>& a,const std::array<uint32_t,28>& b,float t,float eps) {
    if(std::fabs(t)<=eps)return a;
    if(std::fabs(t-1.0f)<=eps)return b;
    std::array<uint32_t,28> r{};r[0]=b[0];r[9]=b[9];
    for(size_t i=4;i<=8;++i)r[i]=Lerp(a[i],b[i],t);
    for(size_t i=10;i<=21;++i)r[i]=Lerp(a[i],b[i],t);
    for(size_t i=24;i<=27;++i)r[i]=Lerp(a[i],b[i],t);
    return r;
}
}
namespace ds3native::yebis {
bool AntiAliasBlock(const Entries& entries,std::array<uint32_t,9>& out) {
    if(entries.size()<9)return false;
    for(size_t i=0;i<9;++i)if(entries[i].words!=1 || (i<2) != (entries[i].type==3))return false;
    for(size_t i=0;i<9;++i)out[i]=entries[i].raw[0];
    return true;
}
std::array<uint32_t,9> BlendAntiAlias(const std::array<uint32_t,9>& a,const std::array<uint32_t,9>& b,float t,float eps) {
    if(std::fabs(t)<=eps)return a;
    std::array<uint32_t,9> r{};r[0]=b[0];r[1]=b[1];
    if(std::fabs(t-1.0f)<=eps){for(size_t i=2;i<9;++i)r[i]=b[i];return r;}
    for(size_t i=2;i<9;++i)r[i]=Lerp(a[i],b[i],t);
    return r;
}
bool FeedbackBlock(const Entries& entries,std::array<uint32_t,8>& out) {
    if(entries.size()<7 || entries[0].type!=3)return false;
    for(size_t i=0;i<7;++i)if(entries[i].words!=1)return false;
    std::array<uint32_t,8> w{};w[0]=entries[0].raw[0];w[1]=entries[1].raw[0];w[2]=0x3F000000u;for(size_t i=2;i<7;++i)w[i+1]=entries[i].raw[0];
    out=w;return true;
}
std::array<uint32_t,8> BlendFeedback(const std::array<uint32_t,8>& a,const std::array<uint32_t,8>& b,float t) {
    if(!(0.0f<t))return a;
    std::array<uint32_t,8> r{};r[0]=b[0];
    if(1.0f<=t){for(size_t i=1;i<8;++i)r[i]=b[i];return r;}
    for(size_t i=1;i<8;++i)r[i]=Lerp(a[i],b[i],t);
    return r;
}
}
namespace ds3native::yebis {
bool DofBlock(const Entries& entries,std::array<uint32_t,34>& out) {
    if(entries.size()<34)return false;
    for(size_t i=0;i<34;++i)if(entries[i].words!=1)return false;
    std::array<uint32_t,34> w{};
    for(size_t k=0;k<12;++k)w[k]=entries[k].raw[0];
    w[12]=entries[13].raw[0];
    for(size_t k=13;k<33;++k)w[k]=entries[k+1].raw[0];
    w[33]=entries[12].raw[0];
    out=w;return true;
}
std::array<uint32_t,34> BlendDof(const std::array<uint32_t,34>& a,const std::array<uint32_t,34>& b,float t,float eps) {
    if(std::fabs(t)<=eps)return a;
    if(std::fabs(t-1.0f)<=eps)return b;
    std::array<uint32_t,34> r{};
    for(size_t i=0;i<34;++i){const bool copied=i==0 || (i>=8 && i<=13) || i==18 || i==24 || i==25;r[i]=copied?b[i]:Lerp(a[i],b[i],t);}
    return r;
}
bool MotionBlurBlock(const Entries& entries,std::array<uint32_t,21>& out) {
    if(entries.size()<21)return false;
    for(size_t i=0;i<21;++i)if(entries[i].words!=1)return false;
    const auto e=[&](size_t i){return entries[i].raw[0];};
    std::array<uint32_t,21> w{};
    w[0]=e(0);w[1]=2;w[2]=e(1);w[3]=e(3);w[4]=e(2);w[5]=e(5);w[6]=e(6);w[7]=e(7);w[8]=e(8);w[9]=0;w[10]=e(9);w[11]=e(10);w[12]=e(4);w[13]=e(11);
    w[14]=e(12)&0xFFu;w[15]=e(13);w[16]=e(14);w[17]=e(15);w[18]=e(16);
    w[19]=(e(17)&0xFFu)|((e(18)&0xFFu)<<8)|((e(19)&0xFFu)<<16);w[20]=e(20);
    out=w;return true;
}
std::array<uint32_t,21> BlendMotionBlur(const std::array<uint32_t,21>& a,const std::array<uint32_t,21>& b,float t,float eps) {
    if(std::fabs(t)<=eps)return a;
    if(std::fabs(t-1.0f)<=eps)return b;
    std::array<uint32_t,21> r{};
    for(size_t i=0;i<21;++i){const bool copied=i<=1 || i==4 || i==5 || i==9 || i==13 || i==14 || i==18 || i==19;r[i]=copied?b[i]:Lerp(a[i],b[i],t);}
    return r;   // word 12 lerped here; the caller puts the words the original leaves unwritten (game_bootstrap: the DoF record's)
}
}
namespace ds3native::yebis {
bool GradingBlock(const Entries& entries,std::array<uint32_t,5>& out) {
    if(entries.size()<2 || entries[0].type!=3 || entries[1].type!=3)return false;
    out={entries[0].raw[0],entries[1].raw[0],0xFFFFFFFFu,0xFFFFFFFFu,0u};return true;
}
std::array<uint32_t,5> BlendGrading(const std::array<uint32_t,5>& a,const std::array<uint32_t,5>& b,float t) {
    if(t<=0.0f)return {a[0],a[1],a[1],b[1],0u};
    if(1.0f<=t)return {b[0],b[1],b[1],b[1],0x3F800000u};
    uint32_t w;std::memcpy(&w,&t,4);return {b[0],a[1],a[1],b[1],w};
}
}
namespace ds3native::yebis {
namespace {
bool Positive(uint32_t word) {return static_cast<int32_t>(word)>0;}
}
std::array<uint32_t,2> BlendToneStored(const ToneStored& a,const ToneStored& b,float t,float eps) {
    const auto snap=[](const ToneStored& r)->std::array<uint32_t,2>{return {Positive(r.w9)?r.w35:r.w0,Positive(r.w30)?r.w36:r.w23};};
    if(std::fabs(t)<=eps)return snap(a);
    if(std::fabs(t-1.0f)<=eps)return snap(b);
    const auto pick=[&](uint32_t fa,uint32_t fb,uint32_t pa,uint32_t sa,uint32_t pb,uint32_t sb){
        return Lerp(Positive(fa)?sa:pa,Positive(fb)?sb:pb,t);};
    return {pick(a.w9,b.w9,a.w0,a.w35,b.w0,b.w35),pick(a.w30,b.w30,a.w23,a.w36,b.w23,b.w36)};
}
AutoFlags ToneAutoFlags(const Entries& tone) {
    AutoFlags f;
    if(tone.size()>9)f.exposure=Positive(tone[9].raw[0]);
    for(const auto& e:tone)if(e.name=="Enable AutoExposure"){f.exposure_gx=Positive(e.raw[0]);break;}
    return f;
}
bool DofAutoFocus(const std::array<uint32_t,34>& dof) {return Positive(dof[24]);}
bool CameraCut(const float* a,const float* b) noexcept {
    const double dx=double(b[12])-a[12],dy=double(b[13])-a[13],dz=double(b[14])-a[14];
    const double forward=double(a[8])*b[8]+double(a[9])*b[9]+double(a[10])*b[10];
    return dx*dx+dy*dy+dz*dz>100.0 || forward<0.5;
}
}
