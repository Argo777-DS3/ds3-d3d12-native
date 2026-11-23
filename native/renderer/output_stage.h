#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <d3d12.h>
#include <wrl/client.h>
#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include "hdr_output.h"

namespace ds3native {
// I-0138 tranche 5 (ROADMAP workstream 11): the output of an HDR10 chain (shaders/post_output.hlsl), in place of the game's
// brightness pass (DisplayStage::RecordPass, which stays the SDR chain's). The stage owns the frame's float picture
// (R16G16B16A16_FLOAT, display-encoded): our tone map's HDR entry and the glow draw into it, then the game's UI and our overlay
// in place, with the game's own blend states (as the SDR chain draws them on its 8-bit picture).
//  - RecordCopy: a frame's 8-bit picture into the float picture, value for value (a world frame without the tone map's HDR
//    entry), before the UI;
//  - RecordScene: the float picture through the transform into an R10G10B10A2_UNORM target (the back buffer, or DLSS-G's
//    HUD-less colour before the UI, which its guide asks in the back buffer's colour space).
// Seam: the caller owns every state of the sources and the targets (a source PIXEL_SHADER_RESOURCE and a target RENDER_TARGET
// during a Record; the stage's picture RENDER_TARGET between frames) and declares them to the frame graph; the stage owns its
// root, pipelines, views and the blue-noise texture (uploaded by the first Record, its barriers inside).
struct OutputConstantMemory {void* cpu{};D3D12_GPU_VIRTUAL_ADDRESS gpu{};};
using OutputConstantSink=std::function<OutputConstantMemory(uint64_t bytes)>;
class OutputStage {
public:
    OutputStage()=default;
    OutputStage(const OutputStage&)=delete;
    OutputStage& operator=(const OutputStage&)=delete;
    static constexpr DXGI_FORMAT Format() noexcept {return DXGI_FORMAT_R10G10B10A2_UNORM;}
    static constexpr DXGI_FORMAT SceneFormat() noexcept {return DXGI_FORMAT_R16G16B16A16_FLOAT;}   // also the UI's and the overlay's pipelines' format on an HDR10 chain
    static constexpr DXGI_FORMAT PictureFormat() noexcept {return DXGI_FORMAT_R8G8B8A8_UNORM;}     // RecordCopy's source
    bool Open(ID3D12Device* device,std::string& reason);
    bool IsOpen() const noexcept {return scene_pso_!=nullptr;}
    // The float picture at the presentation's extent, made on first use and again for another extent; false with a reason. The
    // former picture is retired by ticket: `recording` on a thread that records a frame (it follows that frame's list); false
    // between frames (a resize, on the engine thread, which submits no list once the render worker runs: the final review's M1,
    // a picture kept for the process's life at every resize): it follows the work submitted so far.
    bool Targets(uint32_t width,uint32_t height,std::string& reason,bool recording=true);
    ID3D12Resource* Scene() const noexcept {return scene_.Get();}
    D3D12_CPU_DESCRIPTOR_HANDLE SceneTarget() const noexcept;   // its clear value: black, alpha 1
    // The 8-bit picture (PictureFormat, the stage's extent) into the float picture. picture_generation: the caller's count of
    // that picture's resources (a new resource may take a released one's address). The descriptor heap and the root signature
    // are the stage's afterwards.
    void RecordCopy(ID3D12GraphicsCommandList* list,ID3D12Resource* picture,uint64_t picture_generation,const D3D12_VIEWPORT& viewport,const D3D12_RECT& scissor);
    // The float picture into target_rtv.
    void RecordScene(ID3D12GraphicsCommandList* list,D3D12_CPU_DESCRIPTOR_HANDLE target_rtv,const hdr::OutputParams& params,
        const OutputConstantSink& constants,const D3D12_VIEWPORT& viewport,const D3D12_RECT& scissor);
    uint64_t ScenePasses() const noexcept {return scene_passes_.load();}
    uint64_t CopyPasses() const noexcept {return copy_passes_.load();}
    void Close() noexcept;
    void QuarantineToProcessExit() noexcept;
private:
    // The second step review's H1: two heaps, each made again only by the Record that reads it, from the resources that Record
    // is given: the 8-bit picture's (RecordCopy) and the stage's own picture's (RecordScene). Never a view made from a pointer
    // kept from an earlier frame (the display stage's intermediate is released at a resize).
    void PictureViews(ID3D12Resource* picture,uint64_t picture_generation);
    void SceneViews();
    void Draw(ID3D12GraphicsCommandList* list,ID3D12PipelineState* pso,ID3D12DescriptorHeap* views,D3D12_CPU_DESCRIPTOR_HANDLE target_rtv,D3D12_GPU_VIRTUAL_ADDRESS constants,
        const D3D12_VIEWPORT& viewport,const D3D12_RECT& scissor);
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> copy_pso_,scene_pso_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> picture_views_,scene_views_,rtv_;
    Microsoft::WRL::ComPtr<ID3D12Resource> scene_,noise_,noise_upload_;
    ID3D12Resource* picture_view_{};uint64_t picture_view_generation_{};   // the 8-bit picture picture_views_ was made for (compared, never dereferenced)
    uint64_t targets_generation_{},targets_view_generation_{};
    uint32_t width_{},height_{},stride_{};bool noise_ready_{};
    std::atomic<uint64_t> scene_passes_{},copy_passes_{};
};
}
