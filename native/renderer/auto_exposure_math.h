#pragma once
// I-0132 R11h: the automatic exposure's math on the CPU, the same steps as auto_exposure.cpp's kernels (shaders/auto_exposure.hlsl) in the
// same order, for the unit tests (auto_exposure_test.cpp; auto_exposure_stage_test.cpp runs the kernels on WARP against it). The tone-map
// values it reads are our pass's parameter block's (PostToneParams, I-0138 tranche 3): the exposure, 1 / gamma, the curve's
// constants (the Reinhard curve's three where the block's flag says so, I-0138 part 4c), the colour matrix's rows (the original programme's `$Globals` words 616, 808, 832-833 and 840-851, R11-PLAN §9).
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include "post_params.h"

namespace ds3native::exposure {
struct ToneWords {float exposure{},gamma_correction{},curve[3]{},rows[3][4]{};bool reinhard{};};   // reinhard, curve[2]: I-0138 part 4c
inline ToneWords ToneWordsOf(const PostToneParams& p){ToneWords t;
    t.exposure=p.exposure;t.gamma_correction=p.inv_gamma;t.curve[0]=p.curve_a;t.curve[1]=p.curve_b;t.curve[2]=p.curve_c;t.reinhard=(p.flags&kPostToneReinhard)!=0;
    for(uint32_t r=0;r<3;++r)for(uint32_t c=0;c<4;++c)t.rows[r][c]=p.colour[r][c];return t;}
// The displayed luma (Rec. 709 weights on the encoded channels) of a grey whose exposed linear value is x, the glare, the dither
// and the grading left out: the programme's curve y = sat((1 - e) (1 - T.y e)^2), e = 2^(-x T.x 1.442695), x at least 6.1e-5,
// then sat(M (y, y, y, 1)) and the power 1 / gamma; with the Reinhard curve (I-0138 part 4c) y = sat(s (1 + s T.z) / (1 + x T.y)), s = x T.y.
inline float DisplayGrey(const ToneWords& t,float x){
    x=std::max(x,0.000061f);
    float y;
    if(t.reinhard){const float s=x*t.curve[1];y=std::clamp(s*(1.0f+s*t.curve[2])/(1.0f+x*t.curve[1]),0.0f,1.0f);}   // I-0138 part 4c
    else{const float e=std::exp2(-x*t.curve[0]*1.442695f),k=1.0f-e*t.curve[1];y=std::clamp((1.0f-e)*(k*k),0.0f,1.0f);}
    float o[3];for(int r=0;r<3;++r){o[r]=std::clamp(y*t.rows[r][0]+y*t.rows[r][1]+y*t.rows[r][2]+t.rows[r][3],0.0f,1.0f);
        o[r]=o[r]>0.0f?std::exp2(std::log2(o[r])*t.gamma_correction):0.0f;}
    return o[0]*0.2126f+o[1]*0.7152f+o[2]*0.0722f;
}
// The kept weight of each bin: its pixels between the low and the high percentile of all of them.
template<size_t N> std::array<float,N> KeptWeights(const std::array<uint32_t,N>& bins,float low,float high){
    uint64_t total{};for(const auto b:bins)total+=b;
    const float lo=low*float(total),hi=high*float(total);float acc{};std::array<float,N> w{};
    for(size_t i=0;i<N;++i){const float a0=acc,a1=acc+float(bins[i]);w[i]=std::max(0.0f,std::min(a1,hi)-std::max(a0,lo));acc=a1;}
    return w;}
// The bisection on log2 a over [min_log2, max_log2], 22 halvings: the factor whose mean displayed grey over the kept bins is the target.
template<size_t N> float SolveLog2(const ToneWords& t,const std::array<float,N>& w,float log2_min,float bin_width,float target,float min_log2,float max_log2){
    float kept{};for(const auto v:w)kept+=v;
    float lo=min_log2,hi=max_log2;
    for(int k=0;k<22;++k){const float mid=0.5f*(lo+hi);float sum{};
        for(size_t i=0;i<N;++i)if(w[i]>0.0f)sum+=w[i]*DisplayGrey(t,std::exp2(log2_min+(float(i)+0.5f)*bin_width)*t.exposure*std::exp2(mid));
        if(sum/std::max(kept,1e-6f)<target)lo=mid;else hi=mid;}
    return 0.5f*(lo+hi);}
// One frame's adaptation from `current` towards `goal` (log2 a): linear at `speed` stops a second beyond `transition` stops, then
// exponential with the same rate at the boundary (a continuous first derivative); towards a larger factor (a darker scene) at
// speed_darker, a smaller one at speed_brighter.
inline float Adapt(float current,float goal,float dt,float speed_brighter,float speed_darker,float transition){
    const float diff=goal-current,distance=std::abs(diff),speed=diff>0.0f?speed_darker:speed_brighter;
    float step=distance>transition?std::min(speed*dt,distance-transition):distance*(1.0f-std::exp2(-dt*speed/transition*1.442695f));
    step=std::min(step,distance);
    return current+(diff>0.0f?step:-step);}
}
