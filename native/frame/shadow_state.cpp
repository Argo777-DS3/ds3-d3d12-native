// W5a SW-2: see shadow_state.h. Statements follow decompiled/DS3-1152-F-001B5720 and native/world ShadowFieldsFromBanks.
#include "shadow_state.h"
#include <cstring>

namespace ds3native::shadow {
namespace {
float F(const uint8_t* record,uint32_t at) {float v;std::memcpy(&v,record+at,4);return v;}
int32_t I(const uint8_t* record,uint32_t at) {int32_t v;std::memcpy(&v,record+at,4);return v;}
}
uint32_t ClassSize(int32_t resolution_class) {
    switch(resolution_class){case 1:return 1024;case 3:return 4096;default:return 2048;}
}
Sizes ShadowSizes(const uint8_t* record,const QualityRow* row,bool double_size,int32_t double_factor) {
    uint32_t edge=ClassSize(I(record,0x90));
    if(double_size)edge=static_cast<uint32_t>(static_cast<int32_t>(edge)*double_factor);
    Sizes sizes{edge,edge,0};int32_t blur_add=0;
    if(row){
        // (uint)(num*edge)/den, then: below min -> min; above max -> max (1B5720's unsigned comparisons).
        uint32_t s=(row->num*edge)/row->den;
        if(s<row->min)s=row->min;else if(row->max<s)s=row->max;
        sizes.width=sizes.height=s;blur_add=row->blur_add;
    }
    const int32_t passes=I(record,0x84)+blur_add;
    sizes.blur_passes=passes<1?0u:static_cast<uint32_t>(passes);
    return sizes;
}
State ShadowClamps(const uint8_t* r,float eps) {
    State s;s.near_z=F(r,0x34);s.fade_dist=F(r,0x3C);
    const auto later=[](float candidate,float split){return candidate<split?split:candidate;};
    s.split0=later(F(r,0x34)+eps,F(r,0x40));
    s.split1=later(F(r,0x40)+eps,F(r,0x44));
    s.split2=later(F(r,0x44)+eps,F(r,0x48));
    s.fade_start=later(F(r,0x48)+eps,F(r,0x38));
    return s;
}
void SceneWords(const uint8_t* r,const State& st,uint32_t edge,uint32_t random_texture,const Constants& c,uint8_t* block) {
    const float fade_end=st.fade_start+st.fade_dist,size=static_cast<float>(edge);
    const float a90[4]{fade_end,c.one/(st.fade_dist>c.tiny?st.fade_dist:c.tiny),c.half/size,c.half/size};
    const uint8_t* col=r+0x8C;
    const float cr=static_cast<float>(col[2])/c.c255,cg=static_cast<float>(col[1])/c.c255,cb=static_cast<float>(col[0])/c.c255,ca=static_cast<float>(col[3])/c.c255;
    const float ab0[4]{ca*(c.k[0]-cr),ca*(c.k[1]-cg),ca*(c.k[2]-cb),F(r,0x68)};
    const float ac0[4]{F(r,0x74),F(r,0x78),F(r,0x7C),random_texture?c.one/static_cast<float>(random_texture):0.0f};
    const float ad0[4]{c.one/size,c.one/size,F(r,0x80),0.0f};
    const float d0=st.split0-st.near_z;float den[3]{st.split1-st.split0,st.split2-st.split1,fade_end-st.split2};
    for(float& d:den)if(!(d>0.0f))d=d0+c.one;
    const float ae0[4]{c.one,d0/den[0],d0/den[1],d0/den[2]};
    std::memcpy(block+0x1C0,a90,16);std::memcpy(block+0x1D0,ab0,16);std::memcpy(block+0x230,ac0,16);
    std::memcpy(block+0x240,ad0,16);std::memcpy(block+0x250,ae0,16);
}
}
