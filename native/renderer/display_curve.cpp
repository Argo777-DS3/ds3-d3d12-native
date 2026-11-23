#include "display_curve.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace ds3native {
bool GenerateDisplayCurve(const std::array<float,6>& v,std::span<uint8_t,DisplayCurveBytes> interleaved) noexcept {
    for(const auto value:v)if(!std::isfinite(value) || value<=0.f)return false;
    for(size_t i=0;i<DisplayCurveEntries;++i){const float x=float(i)*0.0009765625f;
        for(size_t c=0;c<3;++c){float y=std::pow(x,v[c]/v[c+3]);y=y>1.f?1.f:y<0.f?0.f:y;
            std::memcpy(interleaved.data()+(i*3+c)*sizeof(float),&y,sizeof(float));}}
    return true;
}
void DeinterleaveDisplayCurve(std::span<const uint8_t,DisplayCurveBytes> interleaved,std::array<std::array<float,DisplayCurveEntries>,3>& slices) noexcept {
    for(size_t i=0;i<DisplayCurveEntries;++i)for(size_t c=0;c<3;++c)std::memcpy(&slices[c][i],interleaved.data()+(i*3+c)*sizeof(float),sizeof(float));
}
uint8_t DisplayPassModel(const std::array<float,DisplayCurveEntries>& slice,uint8_t value) noexcept {
    const double u=double(value)/255.0,texel=u*double(DisplayCurveEntries)-0.5;
    const double base=std::floor(texel);const double weight=std::round((texel-base)*256.0)/256.0;
    const auto at=[&](double index){return double(slice[size_t(std::clamp(index,0.0,double(DisplayCurveEntries-1)))]);};
    const double sampled=at(base)*(1.0-weight)+at(base+1.0)*weight;
    return static_cast<uint8_t>(std::clamp(std::lround(sampled*255.0),0L,255L));
}
}
