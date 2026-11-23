#include "glow_math.h"
#include <cmath>
#include <cstring>

namespace ds3native::glow {
namespace {
float Float(uint32_t bits) {float f;std::memcpy(&f,&bits,4);return f;}
}
void TapImage(float scale,uint8_t out[256],const uint32_t* taps,uint32_t k_word,uint32_t two_pi_word) {
    std::memset(out,0,256);
    const float k=Float(k_word),norm=std::sqrt(Float(two_pi_word));
    float w[13]{},sum=0.0f;
    for(int i=0;i<13;++i){const float x=Float(taps[2*i]),y=Float(taps[2*i+1]);w[i]=std::exp((y*y+x*x)*k)/norm;
        const float v[4]{x*scale,y*scale,w[i],0.0f};std::memcpy(out+16*i,v,16);sum=sum+w[i];}
    for(int i=0;i<13;++i){const float v=w[i]/sum;std::memcpy(out+16*i+8,&v,4);}
}
void FilterParamImage(uint32_t width,uint32_t height,uint8_t out[32]) {
    const float w=static_cast<float>(width),h=static_cast<float>(height);
    const float v[8]{w*0.015625f,h*0.015625f,0.0f,0.0f,1.0f/w,1.0f/h,0.0f,0.0f};std::memcpy(out,v,32);
}
float FadeSelect(float one,float value) {return 0.0f<=value?(value<=one?value:one):0.0f;}
void ParamImage(float g,uint32_t alpha_bits,float factor,uint8_t out[16]) {
    const float select=FadeSelect(1.0f,1.0f-factor),v=(0.0f-g)*select+g;const uint32_t zero=0;
    std::memcpy(out,&v,4);std::memcpy(out+4,&alpha_bits,4);std::memcpy(out+8,&zero,4);std::memcpy(out+12,&zero,4);
}
Size LevelSize(uint32_t width,uint32_t height,unsigned level) {
    const uint32_t d=4u<<level;return {(width+d-1)/d,(height+d-1)/d};
}
}
