// I-0138 tranche 6 (ROADMAP workstream 11): the candidate looks of the « Rekindled » colorimetry, without D3D: their curves in
// double, as the tone-map pass's candidate entries apply them (shaders/post_tone.hlsl), the models of post_tone_stage_test, and
// the scale that matches a candidate's middle grey to the area's own curve. AgX and ACES replace the record's curve inside the
// pass and nothing else of the frame: the same exposed scene in (linear Rec. 709), the same convention out (linear, 0 to 1,
// before the record's gamma). The candidate after Elden Ring has a body of its own: its curve, the bloom before it, its lateral
// dispersion, never the area's colour matrix or LUT; its vignette is the area's own (I-0141 tranche 11: Elden Ring's dropped).
//
// AgX: after Google Filament's filament/src/ToneMapper.cpp (AgxToneMapper, the look NONE) and the colour spaces around it
// (filament/src/details/ColorGrading.cpp: sRGB_to_Rec2020 before the tone mapper, Rec2020_to_sRGB after it; ColorSpaceUtils.h's
// matrices), read 2026-11-14 (evidence/I-0138/artifacts/tranche-6-licences). Copyright (C) 2021 The Android Open Source
// Project, licensed under the Apache License, Version 2.0; changed here: written again in C++ doubles and in HLSL, the
// matrices as rows. Filament credits the matrices to Blender's AgX (EaryChow's AgXBaseRec2020.py) and the contrast curve to
// iolite's minimal implementation.
// ACES: Stephen Hill's fit (@self_shadow), as in Baking Lab's ACES.hlsl by MJP and David Neubelt, MIT license, Copyright (c)
// 2016 MJP. The notices and licence texts: native/THIRD-PARTY-NOTICES.md.
// The candidate after Elden Ring: no code of Elden Ring's; its formulas and shipped values as the project read them
// (investigations/I-0138-modern-post-chain/ER-VALUES.md §6, evidence/I-0138/artifacts/er-tonemap-constants/REPORT.md).
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace ds3native::look {
// The numbers are PostToneFlags' look bits (post_params.h).
enum class ToneLook:uint8_t {Original=0,Agx=1,Aces=2,Er=3};
inline const char* Name(ToneLook look) noexcept {
    switch(look){case ToneLook::Agx:return "agx";case ToneLook::Aces:return "aces";case ToneLook::Er:return "er";default:return "original";}}
using Vec=std::array<double,3>;
using Mat=std::array<Vec,3>;   // rows
constexpr Vec Mul(const Mat& m,const Vec& v) noexcept {
    return {m[0][0]*v[0]+m[0][1]*v[1]+m[0][2]*v[2],m[1][0]*v[0]+m[1][1]*v[1]+m[1][2]*v[2],m[2][0]*v[0]+m[2][1]*v[1]+m[2][2]*v[2]};}
constexpr Mat Mul(const Mat& a,const Mat& b) noexcept {
    Mat out{};for(int r=0;r<3;++r)for(int c=0;c<3;++c)out[r][c]=a[r][0]*b[0][c]+a[r][1]*b[1][c]+a[r][2]*b[2][c];return out;}
constexpr Mat Inverse(const Mat& m) noexcept {
    const double a=m[0][0],b=m[0][1],c=m[0][2],d=m[1][0],e=m[1][1],f=m[1][2],g=m[2][0],h=m[2][1],i=m[2][2];
    const double det=a*(e*i-f*h)-b*(d*i-f*g)+c*(d*h-e*g);
    return {{{(e*i-f*h)/det,(c*h-b*i)/det,(b*f-c*e)/det},{(f*g-d*i)/det,(a*i-c*g)/det,(c*d-a*f)/det},{(d*h-e*g)/det,(b*g-a*h)/det,(a*e-b*d)/det}}};}
// Filament's mat3f literals are columns (each literal line one column): the same nine numbers in their order, as rows here.
constexpr Mat Columns(double a,double b,double c,double d,double e,double f,double g,double h,double i) noexcept {
    return {{{a,d,g},{b,e,h},{c,f,i}}};}
// ColorSpaceUtils.h.
inline constexpr Mat kXyzToSrgb=Columns(3.2404542,-0.9692660,0.0556434,-1.5371385,1.8760108,-0.2040259,-0.4985314,0.0415560,1.0572252);
inline constexpr Mat kSrgbToXyz=Columns(0.4124560,0.2126730,0.0193339,0.3575760,0.7151520,0.1191920,0.1804380,0.0721750,0.9503040);
inline constexpr Mat kRec2020ToXyz=Columns(0.6369530,0.2626983,0.0000000,0.1446169,0.6780088,0.0280731,0.1688558,0.0592929,1.0608272);
inline constexpr Mat kXyzToRec2020=Columns(1.7166634,-0.6666738,0.0176425,-0.3556733,1.6164557,-0.0427770,-0.2533681,0.0157683,0.9422433);
inline constexpr Mat kSrgbToRec2020=Mul(kXyzToRec2020,kSrgbToXyz),kRec2020ToSrgb=Mul(kXyzToSrgb,kRec2020ToXyz);
// ToneMapper.cpp: « These matrices taken from Blender's implementation of AgX, which works with Rec.2020 primaries ».
inline constexpr Mat kAgxInset=Columns(0.856627153315983,0.137318972929847,0.11189821299995,0.0951212405381588,0.761241990602591,0.0767994186031903,
    0.0482516061458583,0.101439036467562,0.811302368396859);
inline constexpr Mat kAgxOutsetInverse=Columns(0.899796955911611,0.11142098895748,0.11142098895748,0.0871996192028351,0.875575586156966,0.0871996192028349,
    0.013003424885555,0.0130034248855548,0.801379391839686);
inline constexpr Mat kAgxOutset=Inverse(kAgxOutsetInverse);
inline constexpr double kAgxMinEv=-12.47393,kAgxMaxEv=4.026069;   // log2(2^-10 x 0.18), log2(2^6.5 x 0.18)
inline double AgxContrast(double x) noexcept {   // agxDefaultContrastApprox
    const double x2=x*x,x4=x2*x2,x6=x4*x2;
    return -17.86*x6*x+78.01*x6-126.7*x4*x+92.06*x4-28.72*x2*x+4.361*x2-0.1718*x+0.002857;}
// Linear Rec. 709 in, linear Rec. 709 out (0 to 1): ColorGrading.cpp's hdrColorAt around AgxToneMapper::operator().
inline Vec Agx(Vec v) noexcept {
    v=Mul(kSrgbToRec2020,v);
    for(double& c:v)c=(std::max)(c,0.0);
    v=Mul(kAgxInset,v);
    for(double& c:v){c=std::log2((std::max)(c,1e-10));c=(std::min)((std::max)((c-kAgxMinEv)/(kAgxMaxEv-kAgxMinEv),0.0),1.0);c=AgxContrast(c);}
    v=Mul(kAgxOutset,v);
    for(double& c:v)c=std::pow((std::max)(c,0.0),2.2);
    v=Mul(kRec2020ToSrgb,v);
    for(double& c:v)c=(std::min)((std::max)(c,0.0),1.0);
    return v;}
// ACES.hlsl: « sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT » and « ODT_SAT => XYZ => D60_2_D65 => sRGB », rows as written there.
inline constexpr Mat kAcesInput{{{0.59719,0.35458,0.04823},{0.07600,0.90834,0.01566},{0.02840,0.13383,0.83777}}};
inline constexpr Mat kAcesOutput{{{1.60475,-0.53108,-0.07367},{-0.10208,1.10813,-0.00605},{-0.00327,-0.07276,1.07602}}};
inline double AcesFit(double v) noexcept {return (v*(v+0.0245786)-0.000090537)/(v*(0.983729*v+0.4329510)+0.238081);}   // RRTAndODTFit
inline Vec Aces(Vec v) noexcept {   // ACESFitted
    v=Mul(kAcesInput,v);
    for(double& c:v)c=AcesFit(c);
    v=Mul(kAcesOutput,v);
    for(double& c:v)c=(std::min)((std::max)(c,0.0),1.0);
    return v;}
// The scene value a rising curve sends to `value` (bisection; 0 when it never reaches it under `most`), and the display's
// middle grey (linear).
inline constexpr double kMiddleGrey=0.18;
template<class Curve> double InputAt(const Curve& curve,double value,double most=4096.0) noexcept {
    if(!(curve(most)>=value))return 0.0;
    double low=0.0,high=most;
    for(int i=0;i<200 && high-low>1e-15*high;++i){const double mid=0.5*(low+high);(curve(mid)<value?low:high)=mid;}
    return 0.5*(low+high);}
template<class Curve> double GreyInput(const Curve& curve,double most=4096.0) noexcept {return InputAt(curve,kMiddleGrey,most);}
// The candidate after Elden Ring. Its curve (ReinhardEx, ER's shipping tone-map mode 8): t = (scale x)^contrast, y = t / (t + 1);
// the contrast its ReinhardContrast row's median over the game's areas (1.4; 1.3 to 1.55 from the tenth to the ninetieth
// percentile), the scale ours (the grey match). Its lateral dispersion's rows (0.0012 across, 0.0001 down: the medians).
inline constexpr double kErContrast=1.4,kErDispersionX=0.0012,kErDispersionY=0.0001;
inline double ErCurve(double c) noexcept {const double t=std::pow((std::max)(c,0.0),kErContrast);return t/(t+1.0);}
// I-0138 tranche 7 (D-0285, Thomas 2026-11-15 09:18: « Oui »): the lift of the midtones and the shadows that Elden Ring's colour-grading
// LUTs make (RESULT « ER's LUTs, read »: the grey node 8, 136 in, at 149 to 173 over the overworld's day, the median about 154; the
// white kept), as a curve of the project's own since those LUTs are not distributed (D-0274): a power on the curve's output, the
// encoded value's own (the power commutes with the display gamma), y^0.8 sending 136 to about 154. After the grey
// match, so the midtones rise against the original's (the match is the curve's own, GreyInput, without the lift).
inline constexpr double kErLift=0.8;
inline double ErLift(double y) noexcept {return std::pow((std::max)(y,0.0),kErLift);}
// I-0138 tranche 7b (Thomas on tranche 7's sheets, 2026-11-16 09:55: « Je vois pas beaucoup de différence entre default et rekindled »;
// the vivid variant, the Rekindled look's since Thomas's « Oui j'aime bien » on its sheets (12:06; first a test flag), its numbers measured on ER's 76 LUTs by a Fable 5.2 subagent, read-only): ER has
// no saturation operator (ColorS 1 in 278 of 279 entries); its LUTs are per-channel tone curves (the grey median 68 -> 68, 136 -> 146)
// with cool shadows (red lowered as much as blue raised, gone by mid-grey). The lift above takes 68 to 89, flattening the shadows. On
// the encoded value, after the lift, the three steps: a contrast per channel e (1 - s (1 - e)^3), bringing the lifted
// grey curve back to ER's median (s 0.8); a vibrance keeping the largest channel, the others moved away from it by
// 1 + v (1 - S) (1 - skin / 2), S the saturation (max - min) / max and skin a weight, full from 20 to 40 degrees of hue, none at 0 and 60, only where
// red >= green >= blue (v 0.25: of taste,
// not ER's); a cool black, red times 1 - 10 t w^2 and blue plus t w^2, w = saturate(1 - 2 max) (t 0.04: black (0, 0, 10) of 255).
inline constexpr double kErVividContrast=0.8,kErVividVibrance=0.25,kErVividTint=0.04;
inline Vec ErVivid(Vec e,double s=kErVividContrast,double v=kErVividVibrance,double t=kErVividTint) noexcept {
    for(double& c:e){const double d=(std::min)((std::max)(1.0-c,0.0),1.0);c*=1.0-s*d*d*d;}
    const double mx=(std::max)(e[0],(std::max)(e[1],e[2])),mn=(std::min)(e[0],(std::min)(e[1],e[2]));
    const double sat=(mx-mn)/(std::max)(mx,1e-4);
    const double skin=(std::min)((std::max)(3.0*(std::min)(e[1]-e[2],e[0]-e[1])/(std::max)(e[0]-e[2],1e-4),0.0),1.0);
    for(double& c:e)c=mx-(mx-c)*(1.0+v*(1.0-sat)*(1.0-0.5*skin));
    const double w=(std::min)((std::max)(1.0-2.0*mx,0.0),1.0),w2=w*w;
    e[0]*=1.0-10.0*t*w2;e[2]+=t*w2;
    return e;}
// ER's g_vChromaticAberrationRG and B for positive dispersions (X(d) = min(d, 0), Y(d) = -|d| / 2, Z(d) = -max(d, 0)): the red
// channel in place, the green read at uv - (d / 2)(2 uv - 1), the blue at uv - d (2 uv - 1); the words: the green's share (x, y),
// the blue's (z, w).
inline std::array<double,4> ErDispersionWords() noexcept {return {-0.5*kErDispersionX,-0.5*kErDispersionY,-kErDispersionX,-kErDispersionY};}
// A candidate's value of a grey scene value (the green channel, as the HDR entries read it).
inline double Grey(ToneLook look,double scene) noexcept {
    switch(look){case ToneLook::Agx:return Agx({scene,scene,scene})[1];case ToneLook::Aces:return Aces({scene,scene,scene})[1];
    case ToneLook::Er:return ErCurve(scene);default:return 0.0;}}
// A candidate's own grey input (a grey scene value: the candidates keep a grey grey within their matrices' rounding; the
// green channel is read).
inline double GreyInput(ToneLook look) noexcept {
    switch(look){
    case ToneLook::Agx:{static const double x=GreyInput([](double s){return Grey(ToneLook::Agx,s);});return x;}
    case ToneLook::Aces:{static const double x=GreyInput([](double s){return Grey(ToneLook::Aces,s);});return x;}
    case ToneLook::Er:{static const double x=GreyInput([](double s){return Grey(ToneLook::Er,s);});return x;}
    default:return 0.0;}}
// The HDR entries' highlights of a candidate (shaders/post_tone.hlsl): the grey scene value it shows at the knee and its slope
// there (a central difference over 0.02 % of that value). Above it the picture follows that tangent in the scene's value, which
// is what hdr::Highlight does for the sensitometric curve (for y = 1 - exp(-a x) the tangent at the knee is the logarithm
// there). No gain (an input no scene value reaches, slope 0) for a knee of 1 or more, or one the candidate never reaches.
struct Tangent {double input{3.0e38},slope{};};
inline Tangent KneeTangent(ToneLook look,double knee) noexcept {
    if(look==ToneLook::Original || !(knee<1.0))return {};
    const double x=InputAt([&](double s){return Grey(look,s);},knee);if(!(x>0.0))return {};
    const double h=1e-4*x;return {x,(Grey(look,x+h)-Grey(look,x-h))/(2.0*h)};}
// What an HDR entry's gain makes of a candidate's picture for a pixel whose largest scene channel is `top`: the factor on the
// three channels in linear light.
inline double HighlightGain(ToneLook look,const Tangent& tangent,double knee,double top) noexcept {
    return top>tangent.input?(knee+tangent.slope*(top-tangent.input))/(std::max)(Grey(look,top),1e-4):1.0;}
// The scale on the exposed scene before a candidate's curve, so that the scene value the area's own curve shows as middle grey
// is shown as middle grey by the candidate too: a sheet then compares looks, not brightnesses. `original_grey`: GreyInput of the
// area's curve (world/post_tone.h); 1 when either side has no answer.
inline double GreyScale(ToneLook look,double original_grey) noexcept {
    const double own=GreyInput(look);return own>0.0 && original_grey>0.0?own/original_grey:1.0;}
}
