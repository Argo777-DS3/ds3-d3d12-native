// I-0113 tranche 27: the menu views' GX tone map (menu_tonemap.h).
#include "menu_tonemap.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace ds3native::menu_tone {
namespace {
float AsFloat(uint32_t word) {float f{};std::memcpy(&f,&word,sizeof f);return f;}
}
bool FromEntries(const std::vector<Entry>& entries,Params& out,std::string& refused) {
    // The second block starts at the second « Exposure » (file order: the YEBIS block's Exposure first).
    size_t start=entries.size();int seen=0;
    for(size_t i=0;i<entries.size();++i)if(entries[i].name=="Exposure" && ++seen==2){start=i;break;}
    if(start==entries.size()){refused="no second Exposure";return false;}
    Params p;bool has[9]{};
    const char* names[9]{"Exposure","AvgLuminance","Scaler R","Scaler G","Scaler B","ToneMapMode","Contrast","Gamma","UseTable"};
    for(size_t i=start;i<entries.size();++i){const auto& e=entries[i];
        for(int k=0;k<9;++k)if(!has[k] && e.name==names[k]){has[k]=true;
            // The step review's L1: the floats are gparam type 9, ToneMapMode and UseTable type 3 (default.gparam's dump).
            if(e.type!=((k==5 || k==8)?3:9)){refused=std::string("type of ")+names[k];return false;}
            switch(k){case 0:p.exposure=AsFloat(e.word);break;case 1:p.avg_luminance=AsFloat(e.word);break;
                case 2:case 3:case 4:p.scaler[k-2]=AsFloat(e.word);break;case 5:p.mode=e.word;break;case 6:p.contrast=AsFloat(e.word);break;
                case 7:p.gamma=AsFloat(e.word);break;case 8:p.use_table=e.word!=0;break;}
            break;}}
    for(int k=0;k<9;++k)if(!has[k]){refused=std::string("no ")+names[k];return false;}
    const float floats[]{p.exposure,p.avg_luminance,p.scaler[0],p.scaler[1],p.scaler[2],p.contrast,p.gamma};
    for(const auto f:floats)if(!std::isfinite(f)){refused="not finite";return false;}
    out=p;return true;
}
float Scale(const Params& p) {return p.avg_luminance>0.0f?std::exp2(p.exposure)/p.avg_luminance:0.0f;}
bool Tabled(const Params& p) {return p.use_table && (p.mode==0 || p.mode==7);}
float TableValue(uint32_t mode,float u,float contrast,float gamma) {
    const float x=0.2f*u/(1.0f-u);
    if(mode==7){const float a=1.0f-std::exp2(-x),b=1.0f-std::exp2(-3.089127f*x);
        const float t=a+contrast*(b*b*b-a);return std::pow(t,1.0f/gamma);}   // log/exp of the DXBC: a non-positive t gives 0 or NaN there too
    const float f=std::max(x-0.004f,0.0f);
    const float t=f*(6.2f*f+0.5f)/(f*(6.2f*f+1.7f)+0.06f);return std::pow(t,2.2f/gamma);
}
float TableEntry(uint32_t mode,uint32_t i,float contrast,float gamma) {
    const float v=TableValue(mode,(static_cast<float>(i)+0.5f)/1024.0f,contrast,gamma);
    return std::round(std::clamp(std::isnan(v)?0.0f:v,0.0f,1.0f)*255.0f)/255.0f;   // as the HLSL: NaN 0, +inf saturates to 1 (the step review's L3)
}
float Apply(const Params& p,float linear,uint32_t channel) {
    const float x=std::max(linear,0.0f)*Scale(p),y=x/(x+0.2f);
    const float f=std::clamp(y*1024.0f-0.5f,0.0f,1023.0f);const auto i=static_cast<uint32_t>(f);const auto j=std::min(i+1,1023u);
    const float t=f-static_cast<float>(i);
    const float v=TableEntry(p.mode,i,p.contrast,p.gamma)*(1.0f-t)+TableEntry(p.mode,j,p.contrast,p.gamma)*t;
    return v*p.scaler[std::min(channel,2u)];
}
}
