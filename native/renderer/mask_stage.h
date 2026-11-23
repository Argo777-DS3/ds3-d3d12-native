#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <functional>
#include <string>

namespace ds3native {
// D-0180: where a stage records its barriers and declares its passes (the renderer routes them through its frame-graph shadow; a
// test records plain barriers). `Use` follows `Pass` for every resource the pass reads or writes. I-0126 R7c (D-0251): kept from
// the SSAO stage's interface, which the sun shadow layer shares; the SSAO itself is removed.
struct StageRecorder {
    virtual void Barrier(ID3D12GraphicsCommandList* list,ID3D12Resource* resource,D3D12_RESOURCE_STATES from,D3D12_RESOURCE_STATES to)=0;
    virtual void Pass(const char* name)=0;
    // plane: of a depth-stencil resource, 0 the depth, 1 the stencil (-1: the caller's default, the depth plane of the scene depth).
    virtual void Use(ID3D12Resource* resource,D3D12_RESOURCE_STATES state,bool write,int plane=-1)=0;
    virtual ~StageRecorder()=default;
};
// Per-frame constants: 256-byte aligned upload memory live until the list completes (the renderer's frame arena).
struct StageConstants {void* cpu{};D3D12_GPU_VIRTUAL_ADDRESS gpu{};};
using StageConstantSink=std::function<StageConstants(uint64_t bytes)>;
// I-0126 R7c (D-0251): the shadow mask t32 (RGBA8 at the render extent, the caller writes it with GXRend_CompositeShadowMask through
// MaskRtv, the raster sun shadow layer and the traced sun drawing into it), the part of the removed SSAO stage that stays. The
// target is created cleared white (its optimised clear value) and PIXEL_SHADER_RESOURCE between frames.
class MaskStage {
public:
    MaskStage()=default;
    MaskStage(const MaskStage&)=delete;
    MaskStage& operator=(const MaskStage&)=delete;
    static constexpr DXGI_FORMAT ColorFormat() noexcept {return DXGI_FORMAT_R8G8B8A8_UNORM;}
    // The mask target and its RTV heap; false with a reason (the frame then has no mask).
    bool Open(ID3D12Device* device,uint32_t width,uint32_t height,std::string& reason);
    bool IsOpen() const noexcept {return mask_!=nullptr;}
    uint32_t Width() const noexcept {return width_;}
    uint32_t Height() const noexcept {return height_;}
    // After the caller's drain: the target at the new extent (the generation advances).
    void Resize(uint32_t width,uint32_t height);
    ID3D12Resource* Mask() const noexcept {return mask_.Get();}
    D3D12_CPU_DESCRIPTOR_HANDLE MaskRtv() const noexcept {return rtv_->GetCPUDescriptorHandleForHeapStart();}
    uint64_t TargetGeneration() const noexcept {return target_generation_;}
    uint64_t TargetBytes() const noexcept;
    void Close() noexcept;
    void QuarantineToProcessExit() noexcept;   // a failed close: every object leaked (a list in flight may use them)
private:
    void MakeTarget();
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtv_;
    Microsoft::WRL::ComPtr<ID3D12Resource> mask_;
    uint32_t width_{},height_{};
    uint64_t target_generation_{};
};
}
