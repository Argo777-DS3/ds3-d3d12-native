#pragma once
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <d3d12.h>

namespace ds3content {
// Exact 13 CPU words consumed by 192FA20; no original wrapper or GPU getter.
// Offsets: filter0, addressUVW4/8/C, bias10, aniso14, comparison18,
// borderRGBA1C..28, MinLOD2C, MaxLOD30. The original bank's trailing word is
// padding, not part of this value. Raw words, including unused fields, survive.
struct EngineSamplerValues { std::array<uint32_t,13> words{}; };
static_assert(sizeof(EngineSamplerValues)==52);

// 192A040/192A120/1929EF0 mappings, bounded to their recovered valid enums.
// Native refusal replaces the original unknown-enum fallback. Consumed floats
// are copied exactly, never clamped. All floats must be finite, border0..1,
// SDK bias bounds and MaxLOD>=MinLOD. Unused anisotropy/comparison are canonical
// 1/NEVER as in primitive_pass; their exact raw input remains in values.
// A decoded description proves neither current bank ownership nor slot choice.
inline bool DecodeEngineSampler(const EngineSamplerValues& values,D3D12_SAMPLER_DESC& out) noexcept {
    constexpr std::array<uint32_t,18> filters{
        0,1,4,5,0x10,0x11,0x14,0x15,0x55,0x80,0x81,0x84,0x85,0x90,0x91,0x94,0x95,0xD5};
    const auto& words=values.words;
    if(words[0]>=filters.size() || words[1]>4 || words[2]>4 || words[3]>4 || words[6]>7)return false;
    D3D12_SAMPLER_DESC candidate{};
    candidate.Filter=static_cast<D3D12_FILTER>(filters[words[0]]);
    candidate.AddressU=static_cast<D3D12_TEXTURE_ADDRESS_MODE>(words[1]+1);
    candidate.AddressV=static_cast<D3D12_TEXTURE_ADDRESS_MODE>(words[2]+1);
    candidate.AddressW=static_cast<D3D12_TEXTURE_ADDRESS_MODE>(words[3]+1);
    candidate.MipLODBias=std::bit_cast<float>(words[4]);
    if(!std::isfinite(candidate.MipLODBias) || candidate.MipLODBias<D3D12_MIP_LOD_BIAS_MIN ||
        candidate.MipLODBias>D3D12_MIP_LOD_BIAS_MAX)return false;
    const bool anisotropic=D3D12_DECODE_IS_ANISOTROPIC_FILTER(candidate.Filter);
    if(anisotropic && (words[5]<1 || words[5]>16))return false;
    candidate.MaxAnisotropy=anisotropic?words[5]:1;
    candidate.ComparisonFunc=D3D12_DECODE_IS_COMPARISON_FILTER(candidate.Filter)?
        static_cast<D3D12_COMPARISON_FUNC>(words[6]+1):D3D12_COMPARISON_FUNC_NEVER;
    for(size_t component=0;component<4;++component) {
        const auto value=std::bit_cast<float>(words[7+component]);
        if(!std::isfinite(value) || value<0 || value>1)return false;
        candidate.BorderColor[component]=value;
    }
    candidate.MinLOD=std::bit_cast<float>(words[11]);
    candidate.MaxLOD=std::bit_cast<float>(words[12]);
    if(!std::isfinite(candidate.MinLOD) || !std::isfinite(candidate.MaxLOD) ||
        candidate.MaxLOD<candidate.MinLOD)return false;
    out=candidate;return true;
}

// D3650's settings delta only: level0..3 -> filter0/6/7/8, supplied anisotropy,
// rows12/13 words0/5. Validate both resulting rows before any mutation; no bank
// initialization, unrelated-row validation, pass inheritance or GPU work.
inline bool ApplyEngineSamplerSettings(std::array<EngineSamplerValues,16>& bank,
    uint32_t level,uint32_t anisotropy) noexcept {
    constexpr std::array<uint32_t,4> filters{0,6,7,8};
    if(level>=filters.size())return false;
    auto row12=bank[12],row13=bank[13];
    row12.words[0]=row13.words[0]=filters[level];
    row12.words[5]=row13.words[5]=anisotropy;
    D3D12_SAMPLER_DESC decoded{};
    if(!DecodeEngineSampler(row12,decoded) || !DecodeEngineSampler(row13,decoded))return false;
    bank[12]=row12;bank[13]=row13;return true;
}
}
