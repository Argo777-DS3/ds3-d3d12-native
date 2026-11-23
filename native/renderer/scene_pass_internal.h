#pragma once
// The scene renderer's shared helpers, for its parts only (scene_pass.cpp: start, targets, presentation, the startup
// frame, resize and close; scene_pass_lighting, _velocity, _decals, _views and _frame). Internal linkage, as in the former
// single file (I-0102: the split moved the code and changed none of it but the barrier sites' file).
#include "scene_pass.h"
#include "gbuffer_encoder.h"
#include "cadence_trace.h"
#include "../frame/primitive_work.h"
#include <set>
#include <source_location>
#include <map>
#include <tuple>
#include <d3dcompiler.h>
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <filesystem>
#include <limits>
#include <stdexcept>

#include <string_view>
#include <utility>

namespace ds3native {
using Microsoft::WRL::ComPtr;
namespace {
constexpr uint32_t texture_capacity=64,composition_binding=24+texture_capacity;
constexpr std::array<DXGI_FORMAT,6> color_formats={DXGI_FORMAT_R16G16B16A16_FLOAT,
    DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
    DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_R11G11B10_FLOAT,DXGI_FORMAT_R16G16B16A16_FLOAT};
// W0a: the target formats of the forward (lit HDR) and velocity envelopes.
constexpr std::array<DXGI_FORMAT,1> forward_formats{SceneRenderer::ForwardTargetFormat()},velocity_formats{SceneRenderer::VelocityTargetFormat()};
// W6 GL-6: Forward2, the lit HDR target and GB5 (the glow source the forward programmes with SV_Target1 write).
constexpr std::array<DXGI_FORMAT,2> forward2_formats{SceneRenderer::ForwardTargetFormat(),color_formats[5]};
inline void Check(HRESULT status,const char* what) {
    if(FAILED(status)) {
        char message[160]{};sprintf_s(message,"%s: HRESULT %08lX",what,static_cast<unsigned long>(status));
        throw std::runtime_error(message);
    }
}
inline D3D12_HEAP_PROPERTIES Heap(D3D12_HEAP_TYPE type) {
    D3D12_HEAP_PROPERTIES value{};value.Type=type;value.CreationNodeMask=value.VisibleNodeMask=1;return value;
}
inline D3D12_RESOURCE_DESC Buffer(uint64_t size) {
    D3D12_RESOURCE_DESC value{};value.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;
    value.Width=size;value.Height=1;value.DepthOrArraySize=value.MipLevels=1;
    value.SampleDesc.Count=1;value.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;return value;
}
inline D3D12_RESOURCE_DESC Image(uint32_t width,uint32_t height,uint32_t mips,DXGI_FORMAT format,D3D12_RESOURCE_FLAGS flags=D3D12_RESOURCE_FLAG_NONE) {
    D3D12_RESOURCE_DESC value{};value.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    value.Width=width;value.Height=height;value.DepthOrArraySize=1;value.MipLevels=static_cast<UINT16>(mips);
    value.Format=format;value.SampleDesc.Count=1;value.Flags=flags;return value;
}
inline ComPtr<ID3D12Resource> NewBuffer(ID3D12Device* device,uint64_t size,D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state) {
    ComPtr<ID3D12Resource> result;const auto heap=Heap(type);const auto desc=Buffer(size);
    Check(CountedResource(device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&result)),"scene buffer allocation");return result;
}
inline ComPtr<ID3D12Resource> Upload(ID3D12Device* device,const void* bytes,size_t size,size_t allocation=0) {
    auto result=NewBuffer(device,std::max(size,allocation),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);
    void* memory{};const D3D12_RANGE empty{};Check(result->Map(0,&empty,&memory),"scene buffer map");
    std::memset(memory,0,std::max(size,allocation));std::memcpy(memory,bytes,size);result->Unmap(0,nullptr);return result;
}
inline D3D12_CPU_DESCRIPTOR_HANDLE CPU(ID3D12DescriptorHeap* heap,uint32_t stride,uint32_t index) {
    auto result=heap->GetCPUDescriptorHandleForHeapStart();result.ptr+=static_cast<SIZE_T>(stride)*index;return result;
}
inline D3D12_GPU_DESCRIPTOR_HANDLE GPU(ID3D12DescriptorHeap* heap,uint32_t stride,uint32_t index) {
    auto result=heap->GetGPUDescriptorHandleForHeapStart();result.ptr+=static_cast<UINT64>(stride)*index;return result;
}
// E0: each hand-written transition is counted with its call site: its part's file (NoteBarrier's file; primitive_pass.cpp is
// 1, scene_pass.cpp 0, the other parts from 2; the graph shadow's line is file*4096+line) and its line. The name is the one
// source_location gives, the same pointer at every site of a part, so a thread keeps the last one it resolved.
inline uint32_t ScenePassFile(const char* name) noexcept {
    thread_local const char* last{};thread_local uint32_t file{};
    if(name==last)return file;
    constexpr std::array<std::pair<std::string_view,uint32_t>,6> parts{{{"scene_pass.cpp",0},{"scene_pass_lighting.cpp",2},
        {"scene_pass_velocity.cpp",3},{"scene_pass_decals.cpp",4},{"scene_pass_views.cpp",5},{"scene_pass_frame.cpp",6}}};
    const std::string_view path(name?name:"");file=0;
    for(const auto& [part,value]:parts)if(path.ends_with(part)){file=value;break;}
    last=name;return file;
}
inline void Transition(ID3D12GraphicsCommandList* list,ID3D12Resource* resource,D3D12_RESOURCE_STATES from,D3D12_RESOURCE_STATES to,
    std::source_location where=std::source_location::current()) {
    // E2: recorded by the shadow; E3: a resource of a frame the graph drives takes the graph's barriers instead.
    const uint32_t file=ScenePassFile(where.file_name());
    if(auto* shadow=active_graph_shadow){if(shadow->Suppress(resource))return;shadow->Transition(resource,NoPlanIndex,from,to,file*4096+where.line());}
    NoteBarrier(file,where.line());
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition={resource,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,from,to};list->ResourceBarrier(1,&barrier);
}
inline void DepthPlaneTransition(ID3D12GraphicsCommandList* list,ID3D12Resource* resource,D3D12_RESOURCE_STATES from,D3D12_RESOURCE_STATES to,
    std::source_location where=std::source_location::current()) {
    const uint32_t file=ScenePassFile(where.file_name());
    if(auto* shadow=active_graph_shadow){if(shadow->Suppress(resource))return;shadow->Transition(resource,0,from,to,file*4096+where.line());}   // E2/E3
    NoteBarrier(file,where.line());
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    // One mip/array slice: depth plane0 is subresource0; stencil remains writable.
    barrier.Transition={resource,0,from,to};list->ResourceBarrier(1,&barrier);
}
inline void Blend(D3D12_GRAPHICS_PIPELINE_STATE_DESC& pso) {
    for(auto& rt:pso.BlendState.RenderTarget) {
        rt.SrcBlend=rt.SrcBlendAlpha=D3D12_BLEND_ONE;rt.DestBlend=rt.DestBlendAlpha=D3D12_BLEND_ZERO;
        rt.BlendOp=rt.BlendOpAlpha=D3D12_BLEND_OP_ADD;rt.LogicOp=D3D12_LOGIC_OP_NOOP;
        rt.RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;
    }
    pso.SampleMask=UINT_MAX;pso.SampleDesc.Count=1;pso.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;pso.RasterizerState.DepthClipEnable=TRUE;
    pso.DepthStencilState.DepthFunc=D3D12_COMPARISON_FUNC_ALWAYS;
    pso.DepthStencilState.FrontFace={D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_COMPARISON_FUNC_ALWAYS};
    pso.DepthStencilState.BackFace=pso.DepthStencilState.FrontFace;
}
inline NativeModelFrameAddresses FrameAddressesOf(const std::array<D3D12_GPU_VIRTUAL_ADDRESS,FrameBlockKindCount>& kinds,NativeModelPass pass) {
    NativeModelFrameAddresses frame{};const auto map=NativeFrameBlocksOf(pass);
    for(uint32_t root=0;root<FrameRootCount;++root)frame[root]=kinds[map[root]];
    return frame;
}
}
// Lighting descriptor sets: one 11-view block per pass {t0..t8, t17, t32}.
constexpr uint32_t light_pass_views=11,light_passes=4;
// 1137 SSS views after the t33 view: t1 depth, t2 null, t3 GBuffer1, then t36 = irradiance (BlurX), temporary (BlurY).
constexpr uint32_t sss_views=light_pass_views*light_passes+1;
// 1147 env-map regions: t0 GBuffer0, t1 depth, t3 GBuffer1, t5 GBuffer3, then one t30 cube per drawn region (at most 64).
// The fixed part of a lighting view heap (the CPU mirror, copied whole into the shader-visible heap) ends after these four views;
// the cube ranges follow it in the shader-visible heap only.
constexpr uint32_t env_views=sss_views+5,env_cube_views=env_views+4,env_region_limit=64,light_fixed_views=env_cube_views;
// cD3 (sheet D1): each recording of a view that draws env regions copies its cubes into its own range of the context's heap,
// a range per recording of the frame and per frame slot (the list of a frame may record several views of one extent, and
// a slot's list may still be read while the next slot records).
constexpr uint32_t env_ranges_per_slot=32,env_range_count=env_ranges_per_slot*max_frame_slots;
}
