#pragma once
#include <array>
#include <cstdint>
#include <d3d12.h>
#include <span>
#include <string>
#include <vector>

namespace ds3content {
using ModelVertexLayoutRow=std::array<uint32_t,7>;
struct ModelVertexElement {
    std::string semantic;
    uint32_t semantic_index{};
    DXGI_FORMAT format{};
    uint32_t input_slot{},byte_offset{};
    D3D12_INPUT_CLASSIFICATION input_class{};
    uint32_t instance_step_rate{};
    bool operator==(const ModelVertexElement&) const = default;
};

//192F090's owned CPU conversion, before any legacy device/cache/wrapper call.
//929A90 supplies the exact semantic names;9294A0 maps formats1..99 identically.
//929B70 maps classes0/1; other authored enums are refused rather than coercing
// a missing classification/semantic/format to a usable native description.
// Names are owned, with no pointers into authored rows or this vector's strings.
// Empty input yields an empty layout. Stream extents, offsets/strides, format
// support and the selected shader's input signature are caller responsibilities.
// Failure (including allocation failure) leaves output unchanged.
inline bool DecodeModelVertexLayout(std::span<const ModelVertexLayoutRow> rows,
                                    std::vector<ModelVertexElement>& output) noexcept {
    if(rows.size()>256)return false;
    constexpr const char* semantics[]{"POSITION","BLENDWEIGHT","BLENDINDICES","NORMAL",
        "PSIZE","TEXCOORD","TANGENT","BINORMAL","POSITIONT","COLOR"};
    for(const auto& row:rows)
        if(row[0]>=std::size(semantics) || row[2]<1 || row[2]>99 || row[5]>1)return false;
    try {
        std::vector<ModelVertexElement> candidate;candidate.reserve(rows.size());
        for(const auto& row:rows)
            candidate.push_back({semantics[row[0]],row[1],static_cast<DXGI_FORMAT>(row[2]),
                row[3],row[4],row[5]?D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA:
                                  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,row[6]});
        output.swap(candidate);
        return true;
    }catch(...){return false;}
}
}
