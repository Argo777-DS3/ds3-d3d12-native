#pragma once
#include "game_primitive_geometry.h"
#include <span>

namespace ds3native::bootstrap {
struct CpuPrimitiveVertexElement {uint32_t offset{},attributes{};};
struct CpuPrimitiveVertexFormat {
    uint32_t stride{};
    std::vector<CpuPrimitiveVertexElement> elements;
};
struct CpuStagedPrimitiveMesh {
    uint32_t vertices{},indices{};
    std::span<const uint8_t> vertex_bytes,index_bytes;
};
// CPU staging upstream of1CF8750's GPU allocation/map. Formats and spans must
// describe the current locked producer transaction. Copies into owned storage;
// no game cache/GPU object, mapped GPU destination or deferred pointer retained.
// Qualified conversions: identical scalar fields;1CF1DB0 signed16x2->float2;
// 1CF1EA0 packed color red/blue exchange. Unknown/generated fields refuse.
// All meshes/formats/indices validated before replacing any published geometry.
std::shared_ptr<const PrimitiveGeometry> ProduceStagedPrimitiveGeometry(
    PrimitiveGeometryStorage& storage,uint64_t key,const CpuPrimitiveVertexFormat& source,
    const CpuPrimitiveVertexFormat& destination,std::span<const CpuStagedPrimitiveMesh> meshes);
}
