#pragma once
#include "content.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <stdexcept>

namespace ds3native {
// A selected draw retains the same buffer allocations admitted by the native
// engine resource owners. No second geometry upload or CPU packet is created.
struct ModelGeometryBinding {
    uint64_t storage_generation{},stream_generation{};
    Microsoft::WRL::ComPtr<ID3D12Resource> vertex_resource,index_resource;
    D3D12_VERTEX_BUFFER_VIEW vertices{};
    // 1063: second stream (input slot 1) of a two-buffer mesh, same storage; empty otherwise.
    D3D12_VERTEX_BUFFER_VIEW vertices2{};
    D3D12_INDEX_BUFFER_VIEW indices{};
    uint32_t index_count{};
    // Phase D cD5 (B5): the owner of a dynamic stream's version (its pooled upload buffer goes back only when every binding
    // that draws it is gone); empty for authored storage. A ComPtr to the buffer alone does not protect its contents: a
    // dynamic stream is refused without its owner (review).
    std::shared_ptr<const void> stream_owner;
    // I-0134: input slot 2 of a simulated cloth mesh's Velo rows, the previous simulated copy (the same layout as slot 0's
    // dynamic stream, whose owner keeps its buffer); empty otherwise (BindModelPreviousStream).
    Microsoft::WRL::ComPtr<ID3D12Resource> previous_resource;
    D3D12_VERTEX_BUFFER_VIEW previous{};
};
inline ModelGeometryBinding BindModelGeometry(const MeshSlice& mesh,uint64_t storage_generation,
    const Microsoft::WRL::ComPtr<ID3D12Resource>& storage,uint64_t stream_generation=0,
    const Microsoft::WRL::ComPtr<ID3D12Resource>& stream={},std::shared_ptr<const void> stream_owner={}) {
    if(!storage || !storage_generation || !mesh.vertex_count || !mesh.stride || !mesh.index_count ||
       mesh.index_count%3 || (mesh.index_width!=2 && mesh.index_width!=4) || mesh.index_offset%mesh.index_width ||
       bool(stream)!=bool(stream_generation) || (stream && !stream_owner))
        throw std::invalid_argument("native selected geometry contract");
    const auto authored=storage->GetDesc();const auto selected=stream?stream->GetDesc():authored;
    const uint64_t vertex_bytes=uint64_t(mesh.vertex_count)*mesh.stride,index_bytes=uint64_t(mesh.index_count)*mesh.index_width;
    const uint64_t vertex_offset=stream?0:mesh.vertex_offset;
    if(authored.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER || selected.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER ||
       vertex_bytes>UINT32_MAX || index_bytes>UINT32_MAX || vertex_offset>selected.Width ||
       vertex_bytes>selected.Width-vertex_offset || mesh.index_offset>authored.Width ||
       index_bytes>authored.Width-mesh.index_offset)
        throw std::invalid_argument("native selected geometry extent");
    ModelGeometryBinding bound;bound.storage_generation=storage_generation;bound.stream_generation=stream_generation;bound.stream_owner=std::move(stream_owner);
    bound.vertex_resource=stream?stream:storage;bound.index_resource=storage;bound.index_count=mesh.index_count;
    const auto vb=bound.vertex_resource->GetGPUVirtualAddress(),ib=storage->GetGPUVirtualAddress();
    if(!vb || !ib || vb>UINT64_MAX-vertex_offset || ib>UINT64_MAX-mesh.index_offset)
        throw std::invalid_argument("native selected geometry address");
    bound.vertices={vb+vertex_offset,static_cast<UINT>(vertex_bytes),mesh.stride};
    bound.indices={ib+mesh.index_offset,static_cast<UINT>(index_bytes),mesh.index_width==4?DXGI_FORMAT_R32_UINT:DXGI_FORMAT_R16_UINT};
    if(mesh.stride2){
        const uint64_t second=uint64_t(mesh.vertex_count)*mesh.stride2;
        // A dynamic stream (D3FA10 patch) replaces slot 0 only; slot 1 stays authored.
        if(second>UINT32_MAX || mesh.vertex_offset2>authored.Width || second>authored.Width-mesh.vertex_offset2)
            throw std::invalid_argument("native selected second stream extent");
        bound.vertices2={ib+mesh.vertex_offset2,static_cast<UINT>(second),mesh.stride2};
    }
    return bound;
}
// I-0134: a dynamic slot 0's previous version as slot 2, as long as slot 0 and of its stride; false (the binding unchanged)
// when the binding has no dynamic stream or the buffer is shorter.
inline bool BindModelPreviousStream(ModelGeometryBinding& bound,const Microsoft::WRL::ComPtr<ID3D12Resource>& previous) {
    if(!previous || !bound.stream_generation || !bound.stream_owner || !bound.vertices.BufferLocation || !bound.vertices.StrideInBytes)return false;
    const auto desc=previous->GetDesc();const auto address=previous->GetGPUVirtualAddress();
    if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER || !address || desc.Width<bound.vertices.SizeInBytes)return false;
    bound.previous_resource=previous;bound.previous={address,bound.vertices.SizeInBytes,bound.vertices.StrideInBytes};
    return true;
}
}
