// I-0138 tranches 2 and 4: synthetic inputs for the comparison of our post programmes with the library's away from the captures
// (glare_stage_test, dof_stage_test, mb_stage_test). Header-only, for the offline tests. Deterministic: a size gives one picture.
#pragma once
#include <DirectXPackedVector.h>
#include <cstdint>
#include <cstring>
#include <vector>

namespace ds3native::synthetic {
// A scene HDR (RGBA16F, tight rows) in four vertical bands: ramps; noise around the bright pass's thresholds; negatives (the
// alpha too); the half-float range's top (above the bright pass's 64,000 clamp) against zeros. With `special`, about one texel
// in 97 also takes an infinity, a NaN, a denormal or a negative zero in one channel.
inline std::vector<uint8_t> SyntheticScene(uint32_t W,uint32_t H,bool special){
    using DirectX::PackedVector::XMConvertFloatToHalf;
    std::vector<uint16_t> texels(size_t(W)*H*4);uint32_t seed=0x1380u+W*31u+H;
    const auto next=[&]{seed=seed*1664525u+1013904223u;return seed>>8;};const auto unit=[&]{return float(next()&0xFFFF)/65535.0f;};
    for(uint32_t y=0;y<H;++y)for(uint32_t x=0;x<W;++x){auto* t=texels.data()+(size_t(y)*W+x)*4;const float u=(float(x)+0.5f)/float(W),v=(float(y)+0.5f)/float(H);float c[4]{};
        switch(x*4/W){
        case 0:c[0]=u*32.0f;c[1]=v*4.0f;c[2]=u+v;c[3]=v;break;
        case 1:for(auto& e:c)e=unit()*16.0f;c[3]=unit();break;
        case 2:for(auto& e:c)e=unit()*4.0f-2.0f;break;
        default:c[0]=(next()&7)?unit()*2.0f:65000.0f;c[1]=unit()*65000.0f;c[2]=((x^y)&1)?0.0f:64000.0f;c[3]=unit()*2.0f;break;}
        for(int i=0;i<4;++i)t[i]=XMConvertFloatToHalf(c[i]);
        if(special && next()%97==0){static constexpr uint16_t bits[5]{0x7C00,0xFC00,0x7E00,0x0001,0x8000};const uint32_t channel=next()&3,which=next()%5;t[channel]=bits[which];}}
    std::vector<uint8_t> bytes(texels.size()*2);std::memcpy(bytes.data(),texels.data(),bytes.size());return bytes;}
// A depth's plane 0 (one float a texel, tight rows) in three horizontal bands: a ramp from the near to the far plane; noise over
// the whole range; noise close to the far plane (where a reversed or a standard depth crowds its values), with exact zeros and
// ones among it.
inline std::vector<uint8_t> SyntheticDepth(uint32_t W,uint32_t H){
    std::vector<float> texels(size_t(W)*H);uint32_t seed=0x4a17u+W*17u+H;
    const auto next=[&]{seed=seed*1664525u+1013904223u;return seed>>8;};const auto unit=[&]{return float(next()&0xFFFF)/65535.0f;};
    for(uint32_t y=0;y<H;++y)for(uint32_t x=0;x<W;++x){float d{};
        switch(y*3/H){
        case 0:d=(float(x)+0.5f)/float(W);break;
        case 1:d=unit();break;
        default:{const uint32_t pick=next()%16;d=pick==0?0.0f:pick==1?1.0f:1.0f-unit()*0.02f;break;}}
        texels[size_t(y)*W+x]=d;}
    std::vector<uint8_t> bytes(texels.size()*4);std::memcpy(bytes.data(),texels.data(),bytes.size());return bytes;}
// The engine's half-size object velocity (RGBA8, tight rows): blocks of 16 texels with one velocity and a covered alpha, blocks
// of noise, and blocks without an object (alpha 0), so that the camera's and the objects' velocities both reach the blur.
inline std::vector<uint8_t> SyntheticVelocity(uint32_t W,uint32_t H){
    std::vector<uint8_t> bytes(size_t(W)*H*4);uint32_t seed=0x77e1u+W*13u+H;
    const auto next=[&]{seed=seed*1664525u+1013904223u;return seed>>8;};
    std::vector<uint32_t> blocks((size_t(W)/16+1)*(size_t(H)/16+1));for(auto& b:blocks)b=next();
    for(uint32_t y=0;y<H;++y)for(uint32_t x=0;x<W;++x){auto* t=bytes.data()+(size_t(y)*W+x)*4;const uint32_t block=blocks[size_t(y/16)*(W/16+1)+x/16];
        switch(block%3){
        case 0:t[0]=uint8_t(block>>4);t[1]=uint8_t(block>>12);t[2]=0;t[3]=255;break;
        case 1:{const uint32_t r=next();t[0]=uint8_t(r);t[1]=uint8_t(r>>8);t[2]=uint8_t(r>>16);t[3]=uint8_t(next());break;}
        default:t[0]=t[1]=128;t[2]=t[3]=0;break;}}
    return bytes;}
}
