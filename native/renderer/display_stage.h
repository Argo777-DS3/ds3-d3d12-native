#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "display_curve.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace ds3native {
// W3 (GAMMA-COLOUR-PATH-READY 3.1): the original's last pass. The frame (composition and UI) is drawn into an R8G8B8A8_UNORM
// intermediate, then one full-screen pass draws it into the back buffer through the 1025-entry display curve (the
// original's own shaders, read from the image by the game module and pinned by hash). The stage owns the intermediate, its
// RTV and SRV, the curve texture and the pipeline; the caller owns every transition of the intermediate and the back
// buffer (so the frame graph sees them) and keeps a returned staging buffer alive until its list completes.
struct DisplayBlobs {std::vector<uint8_t> vertex,pixel;};
// The interleaved curve (DisplayCurveBytes), produced on the engine thread and carried by value in the frame job.
using DisplayCurve=std::shared_ptr<const std::vector<uint8_t>>;
class DisplayStage {
public:
    DisplayStage()=default;
    DisplayStage(const DisplayStage&)=delete;
    DisplayStage& operator=(const DisplayStage&)=delete;
    // Root, pipeline, curve texture (the identity curve, uploaded at the first frame) and the intermediate of the extent;
    // false with a reason when any part cannot be made (the caller then draws straight into the back buffer).
    bool Open(ID3D12Device* device,const DisplayBlobs& blobs,uint32_t width,uint32_t height,std::string& reason);
    bool IsOpen() const noexcept {return pipeline_!=nullptr;}
    uint32_t Width() const noexcept {return width_;}
    uint32_t Height() const noexcept {return height_;}
    // After the caller's drain: a new intermediate and its views; the pipeline and the curve stay.
    void Resize(uint32_t width,uint32_t height);
    // The curve of the frame (null keeps the current one); a different curve object with other bytes is uploaded next.
    void SetCurve(const DisplayCurve& curve);
    D3D12_CPU_DESCRIPTOR_HANDLE Target() const noexcept;
    ID3D12Resource* Intermediate() const noexcept {return intermediate_.Get();}
    uint64_t IntermediateGeneration() const noexcept {return intermediate_generation_;}   // I-0138 tranche 5: counts the intermediates made (Open, Resize)
    ID3D12Resource* Curve() const noexcept {return curve_.Get();}
    // Records the copy of a pending curve (raw barriers on the curve texture, left shader-readable) and returns its
    // staging buffer, to hold until the list has completed; null when nothing was pending.
    Microsoft::WRL::ComPtr<ID3D12Resource> RecordUploadIfDirty(ID3D12GraphicsCommandList* list);
    // I-0131 R10b: DLSS-G's HUD-less colour, the back buffer's size and format: made on first use (and again after a Resize or
    // for another format, I-0138 tranche 5: an HDR10 chain's is R10G10B10A2_UNORM, the former one retired by ticket),
    // RENDER_TARGET between frames; the caller draws the pass into it before the UI. Null when it cannot be made.
    ID3D12Resource* Hudless(DXGI_FORMAT format=Format());
    D3D12_CPU_DESCRIPTOR_HANDLE HudlessTarget() const noexcept;
    // The pass: the intermediate (PIXEL_SHADER_RESOURCE) through the curve into `target` (RENDER_TARGET, R8G8B8A8_UNORM).
    void RecordPass(ID3D12GraphicsCommandList* list,D3D12_CPU_DESCRIPTOR_HANDLE target,const D3D12_VIEWPORT& viewport,const D3D12_RECT& scissor) const;
    uint64_t Uploads() const noexcept {return uploads_.load();}
    uint64_t Passes() const noexcept {return passes_.load();}
    void Close() noexcept;
    void QuarantineToProcessExit() noexcept;   // a failed close: every object leaked (a list in flight may use them)
    static constexpr DXGI_FORMAT Format() noexcept {return DXGI_FORMAT_R8G8B8A8_UNORM;}
private:
    void Targets();
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtv_,srv_;
    Microsoft::WRL::ComPtr<ID3D12Resource> intermediate_,curve_,hudless_;   // I-0131 R10b: hudless_ on first use
    std::vector<uint8_t> uploaded_;   // the bytes the curve texture holds (or will hold once the pending copy runs)
    std::vector<uint8_t> pending_;    // bytes to upload at the next RecordUploadIfDirty (empty: none)
    DisplayCurve last_;
    uint32_t width_{},height_{},srv_stride_{};uint64_t intermediate_generation_{};
    bool curve_initialised_{};mutable std::atomic<uint64_t> passes_{};std::atomic<uint64_t> uploads_{};   // read by the owner thread
};
}
