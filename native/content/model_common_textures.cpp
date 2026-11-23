#include "model_common_textures.h"
#include <bit>
#include <cstdint>
#include <immintrin.h>

namespace ds3content {
namespace {
constexpr uint32_t Seed=0x075BCD15u;
constexpr uint64_t Multiplier=0x5DEECE66Dull,Increment=11;
constexpr float Pi=std::bit_cast<float>(0x40490FDBu);
constexpr float TwoPi=std::bit_cast<float>(0x40C90FDBu);
alignas(16) constexpr uint32_t Sine[3][4]={
    {0x3F800000u,0xBE2AAAABu,0x3C088889u,0xB9500D01u},
    {0x3638EF1Du,0xB2D7322Bu,0x2F309231u,0xAB573F9Fu},
    {0x274A963Cu,0xA317A4DAu,0x1EB8DC78u,0x9A3B0DA1u}};
alignas(16) constexpr uint32_t Cosine[3][4]={
    {0x3F800000u,0xBF000000u,0x3D2AAAABu,0xBAB60B61u},
    {0x37D00D01u,0xB493F27Eu,0x310F76C8u,0xAD49CBA5u},
    {0x29573F9Fu,0xA53413C3u,0x20F2A15Du,0x9C8671CBu}};
__m128 Sum(__m128 value) noexcept {
    auto partial=_mm_add_ps(_mm_shuffle_ps(value,value,0x40),value);
    partial=_mm_add_ps(_mm_shuffle_ps(partial,partial,0x30),partial);
    return _mm_shuffle_ps(partial,partial,0xAA);
}
//141778330/230 use XMM0 despite void decompiler signatures. Preserve SIMD
//operation order and bit constants rather than substituting library trig.
float NoiseMath(float angle,bool sine) noexcept {
    auto x=_mm_set_ss(angle);
    const auto bias=std::bit_cast<float>(std::bit_cast<uint32_t>(Pi)|(std::bit_cast<uint32_t>(angle)&0x80000000u));
    const auto quotient=_mm_cvttss_si32(_mm_div_ss(_mm_add_ss(_mm_set_ss(bias),x),_mm_set_ss(TwoPi)));
    x=_mm_sub_ss(x,_mm_mul_ss(_mm_cvtepi32_ps(_mm_cvtsi32_si128(quotient)),_mm_set_ss(TwoPi)));
    const auto square=_mm_mul_ss(x,x),cube=_mm_mul_ss(square,x);
    auto powers=_mm_unpacklo_ps(_mm_unpacklo_ps(_mm_set_ss(1),square),_mm_unpacklo_ps(x,cube));
    powers=_mm_mul_ps(powers,powers);
    const auto step=_mm_shuffle_ps(sine?x:square,sine?x:square,0);
    if(sine)powers=_mm_mul_ps(powers,step);
    const auto eighth=_mm_mul_ps(_mm_shuffle_ps(powers,powers,0xFF),step);
    const auto& coefficients=sine?Sine:Cosine;
    const auto first=_mm_mul_ps(powers,_mm_castsi128_ps(_mm_load_si128(reinterpret_cast<const __m128i*>(coefficients[0]))));
    const auto high=_mm_mul_ps(powers,eighth);
    const auto second=_mm_mul_ps(high,_mm_castsi128_ps(_mm_load_si128(reinterpret_cast<const __m128i*>(coefficients[1]))));
    const auto third=_mm_mul_ps(_mm_mul_ps(high,eighth),_mm_castsi128_ps(_mm_load_si128(reinterpret_cast<const __m128i*>(coefficients[2]))));
    return _mm_cvtss_f32(_mm_add_ss(_mm_add_ss(Sum(second),Sum(first)),Sum(third)));
}
std::array<uint32_t,4> InitialState() noexcept {
    //1828FD0 consumes incoming EDX forwarded through17AFFD0/17B0040.
    //The local64-bit recurrence has no global state or time dependency.
    uint64_t state=uint64_t(Seed)*Multiplier+Increment;
    std::array<uint32_t,4> result{};
    for(auto& word:result){word=static_cast<uint32_t>(state>>32);state=state*Multiplier+Increment;}
    return result;
}
uint32_t Next(std::array<uint32_t,4>& state) noexcept {
    const auto t=(state[0]<<11)^state[0];
    const auto word=(((state[3]>>11)^t)>>8)^state[3]^t;
    state={state[1],state[2],state[3],word};return word;
}
uint8_t Quantize(float value) noexcept {
    return static_cast<uint8_t>(static_cast<int64_t>((double(value)*0.5+0.5)*255.0));
}
CpuTextureResource Image(uint32_t width,uint32_t height,DXGI_FORMAT format,const char* name) {
    CpuTextureResource image;image.name=name;image.width=width;image.height=height;
    image.mip_levels=image.array_slices=1;image.format=format;
    image.subresources.push_back({width,height,width*4,height,std::vector<uint8_t>(size_t(width)*height*4)});
    return image;
}
}
std::array<CpuTextureResource,16> ProduceModelDitherTextures() {
    auto state=InitialState();std::array<CpuTextureResource,16> bank;
    for(auto& image:bank){
        image=Image(64,64,DXGI_FORMAT_R8G8B8A8_UNORM,"engine-common-dither");
        auto& bytes=image.subresources[0].bytes;
        for(size_t pixel=0;pixel<64*64;++pixel){
            const float angle=(std::bit_cast<float>(Next(state)%0x800001u+0x3F800000u)-1.0f)*TwoPi+0.0f;
            const float sine=NoiseMath(angle,true),cosine=NoiseMath(angle,false);
            bytes[pixel*4]=Quantize(cosine);bytes[pixel*4+1]=Quantize(sine);
        }
    }
    return bank;
}
std::array<CpuTextureResource,2> ProduceModelFadeDitherTextures() {
    // FF630 (140100530..1401006E4, I-0113 tranche 22): the 4x4 ordered-dither matrix as 16 ints, each byte v * 255 / 15 (v * 17),
    // row r at bytes r * 4 of the 4-byte rows; the second texture takes 255 - byte. Created 4x4, A8_UNORM, one mip (14002AF40).
    constexpr uint8_t bayer[16]{0,8,2,10, 12,4,14,6, 3,11,1,9, 15,7,13,5};
    std::array<CpuTextureResource,2> pair;
    for(size_t index=0;index<pair.size();++index){
        auto& image=pair[index];image.name=index?"engine-fade-dither-inverse":"engine-fade-dither";image.width=image.height=4;
        image.mip_levels=image.array_slices=1;image.format=DXGI_FORMAT_A8_UNORM;
        CpuTextureSubresource plane{4,4,4,4,std::vector<uint8_t>(16)};
        for(size_t i=0;i<16;++i){const auto value=static_cast<uint8_t>(bayer[i]*255u/15u);plane.bytes[i]=index?static_cast<uint8_t>(255u-value):value;}
        image.subresources.push_back(std::move(plane));
    }
    return pair;
}
CpuTextureResource ProduceCommonDecalTexture() {
    auto image=Image(1,1,DXGI_FORMAT_B8G8R8A8_UNORM,"engine-common-absent-decal");
    //06C00 writes FF000000 little endian, then creates1x1 DXGI87. This is
    //the actual engine absence texture, independently of host withholding.
    image.subresources[0].bytes[3]=255;return image;
}
}
namespace ds3content {
std::array<SystemTexture,14> ProduceSystemTextures() {
    //06C00 entries0..13 (1x1, DXGI87 except entry5 DXGI91) registered by
    //D4A2D0 as SYSTEX_* through B580(owner,index). Colours are its exact
    //little-endian words; cubes14..16 are not part of this 2D set.
    constexpr const char* names[14]{"SYSTEX_WHITE","SYSTEX_BLACK","SYSTEX_GRAY","SYSTEX_SPEC","SYSTEX_BLACKSPEC",
        "SYSTEX_18PERCENT_GRAY","SYSTEX_NORMAL","SYSTEX_ZERO","SYSTEX_RED","SYSTEX_YELLOW","SYSTEX_MAGENTA",
        "SYSTEX_GREEN","SYSTEX_BLUE","SYSTEX_CYAN"};
    constexpr uint32_t colours[14]{0xFFFFFFFFu,0xFF000000u,0xFF808080u,0x800A0A0Au,0x01000000u,0xFF757575u,
        0xFF8080FFu,0x00000000u,0xFFFF0000u,0xFFFFFF00u,0xFFFF00FFu,0xFF00FF00u,0xFF0000FFu,0xFF00FFFFu};
    std::array<SystemTexture,14> result;
    for(size_t i=0;i<14;++i){
        result[i].name=names[i];
        result[i].image=Image(1,1,i==5?DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:DXGI_FORMAT_B8G8R8A8_UNORM,names[i]);
        for(size_t b=0;b<4;++b)result[i].image.subresources[0].bytes[b]=static_cast<uint8_t>(colours[i]>>(8*b));
    }
    return result;
}
}
