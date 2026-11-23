#include "yebis_tonemap.h"
#include <cmath>
#include <cstring>

#pragma fp_contract(off)   // the formulas reproduce the original's float roundings one operation at a time (no fused multiply-add)

namespace ds3native::yebis {
float CurveTable(const TonemapData& d,float x) {
    if(!(0.0f<=x))x=0.0f;else if(d.one<x)x=d.one;   // 141743227..141743236 (COMISS order)
    const float f=(x-0.0f)/((d.one-0.0f)/d.steps);
    const int i=static_cast<int>(std::floor(f));
    const int j=i+1<20?i+1:20;
    return (d.curve[static_cast<size_t>(j)]-d.curve[static_cast<size_t>(i)])*(f-static_cast<float>(i))+d.curve[static_cast<size_t>(i)];
}
std::array<float,2> TonemapCurve(const TonemapData& d,float c) {
    const float root=std::pow(c,d.third);
    return {root*d.tx_scale+d.one,CurveTable(d,root)};
}
bool LoadColorData(ColorData& c,const std::function<bool(uint32_t,void*,size_t)>& read){
    static constexpr uint32_t hue[13]{0x2B4D024,0x2B4D184,0x2B4D180,0x27D5958,0x2B4D07C,0x2B4D178,0x2B4D008,0x2B4D160,0x2B4D06C,0x2B4D174,0x2B4D040,0x2B4D048,0x2B4D078};
    c.ok=read(0x3D7EEC0,&c.pi,4) && read(0x3D7EFE8,&c.half_turn,4) && read(0x2B4D038,&c.luma[0],4) && read(0x2B4D074,&c.luma[1],4) && read(0x2B4D010,&c.luma[2],4) &&
        read(0x3D7EDB4,&c.half,4) && read(0x3D7ECDC,&c.min_exposure,4) && read(0x3D7EE90,&c.default_gamma,4);
    for(size_t i=0;i<13 && c.ok;++i)c.ok=read(hue[i],&c.hue[i],4);
    return c.ok;}
namespace {
using Matrix=std::array<float,16>;
// FUN_1416DD9B0: out = a b (row-major), each element ((b0j a_i0 + b1j a_i1) + b2j a_i2) + b3j a_i3.
Matrix Multiply(const Matrix& a,const Matrix& b){Matrix o{};
    for(size_t i=0;i<4;++i)for(size_t j=0;j<4;++j)o[i*4+j]=((b[j]*a[i*4]+b[4+j]*a[i*4+1])+b[8+j]*a[i*4+2])+b[12+j]*a[i*4+3];
    return o;}
// FUN_1416DDB90 (the luminance path, flag 1): s = sin, c = cos of the angle; coefficients in [142B4D024 ...] order (hue[0..12]:
// 0.143, -0.787, -0.715, 0.140, 0.928, -0.283, 0.072, -0.072, 0.715, -0.213, 0.213, 0.285, 0.787).
Matrix Hue(const ColorData& d,float angle){const float s=std::sin(angle),c=std::cos(angle);const auto& h=d.hue;Matrix m{};
    const float f9=s*h[0],f10=s*h[1],f11=s*h[2],f12=s*h[3],f14=s*h[4],f15=s*h[5],f16=s*h[6],f4=c*h[7]+h[6],f13=s*h[8],f7=c*h[9]+h[10],
        f5=c*h[4]+h[6],f8=c*h[2]+h[8],f17=c*h[11]+h[8],f6=c*0.0f+1.0f,zero=s*0.0f+c*0.0f+0.0f;
    m[0]=c*h[12]+h[10]+s*h[9];m[1]=f7+f9;m[2]=f7+f10;m[3]=zero;m[4]=f8+f11;m[5]=f17+f12;m[6]=f8+f13;m[7]=zero;
    m[8]=f4+f14;m[9]=f4+f15;m[10]=f5+f16;m[11]=m[12]=m[13]=m[14]=zero;m[15]=f6+s*0.0f;return m;}
// FUN_1416D1D90 (flag 0, the Rec. 709 weights): row r holds (0 - L_r) S + L_r off the diagonal and (1 - L_r) S + L_r on it.
Matrix Saturation(const ColorData& d,float s){Matrix m{};
    for(size_t r=0;r<3;++r)for(size_t k=0;k<3;++k)m[r*4+k]=((r==k?1.0f:0.0f)-d.luma[r])*s+d.luma[r];
    m[15]=1.0f;return m;}
}
std::array<float,4> ReinhardCurve(float mapping_factor) {
    const float one=1.0f,four=4.0f,two=2.0f;   // scalar SSE2 (no /arch), no FMA: the library's separate MULSS, SUBSS, DIVSS
    const float a=mapping_factor,b=one;const float r=a/b;const float rr=r*r;const float k=one/rr;
    const float k4=k*four;const float x=k4-two;const float y=one/b;const float aa=a*a;const float b2=b*two;const float w=aa/b2;
    return {x,y,k,w};
}
TonemapConstants BuildTonemap(const TonemapData& d,const TonemapInputs& in) {
    TonemapConstants t;t.exposure=in.exposure;float gamma=in.gamma;
    if(d.color.ok){t.guarded=true;
        if(!(d.color.min_exposure<in.exposure))t.exposure=d.color.min_exposure;   // 1416F71B0: max(e, FLT_MIN), NaN to FLT_MIN
        if(!(0.0f<gamma))gamma=0.0f;                                             // max(g, 0), NaN to 0
        if(gamma==0.0f)gamma=d.color.default_gamma;}                             // FUN_141753F00: a zero gamma is 2.2
    else t.guarded=0.0f<in.exposure && 0.0f<in.gamma;
    t.gamma_correction=d.one/gamma;
    t.dither={1.0f/255.0f,-0.5f/255.0f};
    t.reinhard=in.tone_type==3u;
    if(t.reinhard)t.curve=ReinhardCurve(in.mapping_factor);
    else{const auto c=TonemapCurve(d,in.filmic_contrast);t.curve={c[0],c[1],0.0f,0.0f};}
    // GL-4: E69A0's colour matrix: from the identity, x the hue rotation (ColorH != 0, angle (h pi) / 180), x the saturation
    // (ColorS != 1), x diag(v, v, v, 1) (ColorV != 1), each through FUN_1416DD9B0; the block holds its transpose (the observer's
    // order for float4x4 variables). Without the colour data only the saturation case is served (TM-3's order, bit for bit at
    // m37 and m31: (L - s L) + s on the diagonal).
    if(d.color.ok){Matrix m{};m[0]=m[5]=m[10]=m[15]=1.0f;
        if(in.color_h!=0.0f)m=Multiply(m,Hue(d.color,(in.color_h*d.color.pi)/d.color.half_turn));
        if(in.color_s!=1.0f)m=Multiply(m,Saturation(d.color,in.color_s));
        if(in.color_v!=1.0f){Matrix v{};v[0]=v[5]=v[10]=in.color_v;v[15]=1.0f;m=Multiply(m,v);}   // FUN_1416CBC30
        if(in.contrast!=1.0f){Matrix c{};const float k=in.contrast,offset=(d.one-k)*d.color.half;   // FUN_1416D20C0's (1 - c) x half
            c[0]=c[5]=c[10]=k;c[12]=c[13]=c[14]=offset;c[15]=1.0f;m=Multiply(m,c);}
        for(size_t r=0;r<4;++r)for(size_t c=0;c<4;++c)t.color_matrix[r*4+c]=m[c*4+r];
        t.matrix_supported=true;}
    else{const float L[3]{0.2126f,0.7152f,0.0722f};const float s=in.color_s;
        for(int r=0;r<3;++r)for(int c=0;c<3;++c)t.color_matrix[size_t(r*4+c)]=(L[c]-s*L[c])+(r==c?s:0.0f);
        t.color_matrix[15]=1.0f;
        t.matrix_supported=in.color_h==0.0f && in.color_v==1.0f && in.contrast==1.0f;}
    return t;
}
void TonemapGlobals(const TonemapConstants& t,uint8_t* bytes) {
    std::memset(bytes,0,tonemap_globals_bytes);
    const auto put=[bytes](uint32_t word,float value){std::memcpy(bytes+4*word,&value,4);};
    put(20,1.0f);put(21,-1.0f);
    put(616,t.exposure);put(617,1.0f);put(618,1.0f);put(619,1.0f);put(623,1.0f);
    put(744,2.0f);put(745,-1.0f);put(748,0.5f);put(752,2.0f);put(753,-1.0f);put(755,1.0f);put(756,0.5f);
    put(808,t.gamma_correction);put(809,t.dither[0]);put(810,t.dither[1]);
    put(832,t.curve[0]);put(833,t.curve[1]);if(t.reinhard){put(834,t.curve[2]);put(835,t.curve[3]);}
    for(uint32_t i=0;i<16;++i)put(840+i,t.color_matrix[i]);
}
std::array<uint8_t,64> TonemapDither(const std::array<float,16>& bayer_rows,float scale,float bias,float to_byte) {
    std::array<uint8_t,64> texels{};
    const auto byte=[&](size_t i){return static_cast<uint8_t>(static_cast<int64_t>((bayer_rows[i]*scale+bias)*to_byte));};
    // Texel (row r, column c) = (v[r][c], v[r][0], v[r][0], v[r][c]): 141719770's interleave, confirmed by the original's capture.
    for(size_t r=0;r<4;++r)for(size_t c=0;c<4;++c){const size_t i=4*r+c;
        texels[4*i]=texels[4*i+3]=byte(i);texels[4*i+1]=texels[4*i+2]=byte(4*r);}
    return texels;
}
std::array<float,4> VignetteRect(const VignetteData& d,float power,float aspect,float fov_dependence,float tan_half_fov) {
    const float v=(d.tan_eighth_pi*(1.0f-fov_dependence)+fov_dependence*tan_half_fov)*power;   // FUN_141751A60, the power path (+0x16C = 0)
    const float h=v*((aspect/d.k)/(d.m/d.k));
    const float oy=(v-(-v))*0.0f*0.5f,ox=(h-(-h))*0.0f*0.5f;
    return {-h-ox,-v-oy,h-ox,v-oy};
}
float VignetteFovTerm(const VignetteData& d,float vertical_fov) {
    const float t=std::tan(vertical_fov*0.5f);return t<=0.0f?d.tan_eighth_pi:t;}
std::array<std::array<float,2>,3> VignetteCoordinates(const std::array<float,4>& r) {
    constexpr float uv[3][2]{{0,1},{0,-1},{2,1}};std::array<std::array<float,2>,3> out{};
    for(int i=0;i<3;++i){out[size_t(i)][0]=r[0]+uv[i][0]*(r[2]-r[0]);out[size_t(i)][1]=r[1]+uv[i][1]*(r[3]-r[1]);}
    return out;
}
}
