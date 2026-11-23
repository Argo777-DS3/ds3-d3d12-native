#pragma once
#include <d3d12.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <span>
#include <stdexcept>

namespace ds3native {
// Command encoding extracted from the former live host's G-buffer pass. Inputs are already
// resolved native views/bindings, with no capture/resource-numbering contract.
// The caller owns transitions, shader-read coverage, selection/order and keeps
// every referenced resource, heap and PSO alive through its submission fence.
inline void ClearGBuffer(ID3D12GraphicsCommandList* list,
    std::span<const D3D12_CPU_DESCRIPTOR_HANDLE> targets,
    D3D12_CPU_DESCRIPTOR_HANDLE depth,float clear_depth,UINT8 clear_stencil) {
    if(!list || targets.empty() || targets.size()>8 || !depth.ptr ||
       (clear_depth!=0.f && clear_depth!=1.f))throw std::invalid_argument("GBuffer clear contract");
    constexpr float empty[4]{};
    for(const auto target:targets)list->ClearRenderTargetView(target,empty,0,nullptr);
    list->ClearDepthStencilView(depth,D3D12_CLEAR_FLAG_DEPTH|D3D12_CLEAR_FLAG_STENCIL,
        clear_depth,clear_stencil,0,nullptr);
}

struct GBufferDraw {
    std::span<const D3D12_CPU_DESCRIPTOR_HANDLE> targets;
    D3D12_CPU_DESCRIPTOR_HANDLE depth{};
    D3D12_VIEWPORT viewport{};
    D3D12_RECT scissor{};
    ID3D12RootSignature* root{};
    ID3D12PipelineState* pipeline{};
    std::span<ID3D12DescriptorHeap* const> heaps;
    // Root table indices are contiguous from zero, as in the existing native
    // GBuffer roots (host: two tables; SceneRenderer: four tables).
    std::span<const D3D12_GPU_DESCRIPTOR_HANDLE> tables;
    D3D12_PRIMITIVE_TOPOLOGY topology{};
    std::span<const D3D12_VERTEX_BUFFER_VIEW> vertices;
    D3D12_INDEX_BUFFER_VIEW indices{};
    UINT index_count{},instance_count{},start_index{},start_instance{};
    INT base_vertex{};
};

// W0a: the depth of a depth-only target set (no colour target), cleared on its own; a DSV carries no format, so the caller
// says whether it has a stencil plane (D16 has none).
inline void ClearDepth(ID3D12GraphicsCommandList* list,D3D12_CPU_DESCRIPTOR_HANDLE depth,float clear_depth,UINT8 clear_stencil,bool has_stencil) {
    if(!list || !depth.ptr || (clear_depth!=0.f && clear_depth!=1.f))throw std::invalid_argument("depth clear contract");
    list->ClearDepthStencilView(depth,has_stencil?D3D12_CLEAR_FLAG_DEPTH|D3D12_CLEAR_FLAG_STENCIL:D3D12_CLEAR_FLAG_DEPTH,clear_depth,clear_stencil,0,nullptr);
}

// W0a: an empty target span is a depth-only draw (the depth view is then required, as for every draw).
inline void BindGBufferDraw(ID3D12GraphicsCommandList* list,const GBufferDraw& draw) {
    if(!list || draw.targets.size()>8 || !draw.depth.ptr ||
       !draw.root || !draw.pipeline || draw.heaps.empty() || draw.heaps.size()>2 ||
       draw.tables.empty() || draw.tables.size()>4 || draw.vertices.empty() || draw.vertices.size()>3 ||
       !draw.instance_count || !draw.indices.BufferLocation ||
       draw.viewport.Width<=0 || draw.viewport.Height<=0 ||
       (draw.topology!=D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST && draw.topology!=D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP && draw.topology!=D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST))   // W7: tessellated rows
        throw std::invalid_argument("GBuffer draw contract");
    list->OMSetRenderTargets(static_cast<UINT>(draw.targets.size()),draw.targets.empty()?nullptr:draw.targets.data(),FALSE,&draw.depth);
    list->RSSetViewports(1,&draw.viewport);list->RSSetScissorRects(1,&draw.scissor);
    list->SetGraphicsRootSignature(draw.root);
    list->SetDescriptorHeaps(static_cast<UINT>(draw.heaps.size()),draw.heaps.data());
    for(UINT slot=0;slot<draw.tables.size();++slot)list->SetGraphicsRootDescriptorTable(slot,draw.tables[slot]);
    list->SetPipelineState(draw.pipeline);
    list->IASetPrimitiveTopology(draw.topology);
    list->IASetVertexBuffers(0,static_cast<UINT>(draw.vertices.size()),draw.vertices.data());
    list->IASetIndexBuffer(&draw.indices);
}
// m45 frame cost (Native1841 sample profile: the per-draw state calls of the model loops): what one draw loop of a pass has
// already set on its command list, so a draw sets only what differs from the loop's previous draw. D3D12 keeps a list's
// state from one draw to the next; a root signature change resets the root arguments and a descriptor heap change the
// tables, so those are set again after one. A loop starts with a fresh state: other passes may have changed the list.
struct GBufferBindState {
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE,8> targets{};size_t target_count{SIZE_MAX};D3D12_CPU_DESCRIPTOR_HANDLE depth{};
    D3D12_VIEWPORT viewport{};D3D12_RECT scissor{};bool view{};
    ID3D12RootSignature* root{};std::array<ID3D12DescriptorHeap*,2> heaps{};size_t heap_count{};
    std::array<D3D12_GPU_DESCRIPTOR_HANDLE,4> tables{};std::array<bool,4> table_set{};
    ID3D12PipelineState* pipeline{};D3D12_PRIMITIVE_TOPOLOGY topology{D3D_PRIMITIVE_TOPOLOGY_UNDEFINED};
    std::array<D3D12_VERTEX_BUFFER_VIEW,3> vertices{};size_t vertex_count{SIZE_MAX};D3D12_INDEX_BUFFER_VIEW indices{};bool index_set{};
    UINT stencil{};bool stencil_set{};std::array<float,4> blend{};bool blend_set{};
    std::array<D3D12_GPU_VIRTUAL_ADDRESS,64> root_cbvs{};   // by root index; 0: not set since the root signature
};
// The draw's bindings through the loop's state (as BindGBufferDraw, which the state only filters).
inline void BindGBufferDraw(ID3D12GraphicsCommandList* list,const GBufferDraw& draw,GBufferBindState& state) {
    if(!list || draw.targets.size()>8 || !draw.depth.ptr ||
       !draw.root || !draw.pipeline || draw.heaps.empty() || draw.heaps.size()>2 ||
       draw.tables.empty() || draw.tables.size()>4 || draw.vertices.empty() || draw.vertices.size()>3 ||
       !draw.instance_count || !draw.indices.BufferLocation ||
       draw.viewport.Width<=0 || draw.viewport.Height<=0 ||
       (draw.topology!=D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST && draw.topology!=D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP && draw.topology!=D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST))
        throw std::invalid_argument("GBuffer draw contract");
    const auto same_targets=[&]{if(state.target_count!=draw.targets.size() || state.depth.ptr!=draw.depth.ptr)return false;
        for(size_t i=0;i<draw.targets.size();++i)if(state.targets[i].ptr!=draw.targets[i].ptr)return false;return true;};
    if(!same_targets()){
        list->OMSetRenderTargets(static_cast<UINT>(draw.targets.size()),draw.targets.empty()?nullptr:draw.targets.data(),FALSE,&draw.depth);
        state.target_count=draw.targets.size();state.depth=draw.depth;std::copy(draw.targets.begin(),draw.targets.end(),state.targets.begin());}
    if(!state.view || std::memcmp(&state.viewport,&draw.viewport,sizeof draw.viewport) || std::memcmp(&state.scissor,&draw.scissor,sizeof draw.scissor)){
        list->RSSetViewports(1,&draw.viewport);list->RSSetScissorRects(1,&draw.scissor);state.viewport=draw.viewport;state.scissor=draw.scissor;state.view=true;}
    if(state.root!=draw.root){list->SetGraphicsRootSignature(draw.root);state.root=draw.root;state.table_set={};state.root_cbvs={};}
    if(state.heap_count!=draw.heaps.size() || !std::equal(draw.heaps.begin(),draw.heaps.end(),state.heaps.begin())){
        list->SetDescriptorHeaps(static_cast<UINT>(draw.heaps.size()),draw.heaps.data());
        state.heap_count=draw.heaps.size();std::copy(draw.heaps.begin(),draw.heaps.end(),state.heaps.begin());state.table_set={};}
    for(UINT slot=0;slot<draw.tables.size();++slot)if(!state.table_set[slot] || state.tables[slot].ptr!=draw.tables[slot].ptr){
        list->SetGraphicsRootDescriptorTable(slot,draw.tables[slot]);state.tables[slot]=draw.tables[slot];state.table_set[slot]=true;}
    if(state.pipeline!=draw.pipeline){list->SetPipelineState(draw.pipeline);state.pipeline=draw.pipeline;}
    if(state.topology!=draw.topology){list->IASetPrimitiveTopology(draw.topology);state.topology=draw.topology;}
    if(state.vertex_count!=draw.vertices.size() || std::memcmp(state.vertices.data(),draw.vertices.data(),draw.vertices.size()*sizeof(D3D12_VERTEX_BUFFER_VIEW))){
        list->IASetVertexBuffers(0,static_cast<UINT>(draw.vertices.size()),draw.vertices.data());
        state.vertex_count=draw.vertices.size();std::copy(draw.vertices.begin(),draw.vertices.end(),state.vertices.begin());}
    if(!state.index_set || std::memcmp(&state.indices,&draw.indices,sizeof draw.indices)){list->IASetIndexBuffer(&draw.indices);state.indices=draw.indices;state.index_set=true;}
}
inline void RecordGBufferDraw(ID3D12GraphicsCommandList* list,const GBufferDraw& draw) {
    BindGBufferDraw(list,draw);
    list->DrawIndexedInstanced(draw.index_count,draw.instance_count,draw.start_index,draw.base_vertex,draw.start_instance);
}
}
