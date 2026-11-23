#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <d3d12.h>
#include <wrl/client.h>
#include "post_params.h"
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <string>

namespace ds3native {
// I-0132 R11h (R11-PLAN §9 step 1, Thomas 2026-11-09 06:22-06:27): the automatic exposure, our own pass (not YEBIS's). A compute pass
// makes a histogram of the log2 luminance of the HDR the tone map reads; one group then solves and adapts on the GPU (nothing read back
// on the frame's path): the exposure factor `a` whose predicted displayed grey, over the pixels between two percentiles, has the game's
// one target mean (the original's mean displayed level at the witness views; one universal rule, no case by area), found by bisection with the tone map's own curve, matrix and gamma (our
// tone-map pass's parameter block, b0, since I-0138 tranche 3; the YEBIS tone map's `$Globals` before); then adapted towards it (stops a second, exponentially near it) and written as a 1x1 RGBA32F (a, a, a, 1), the
// tone map's adapted factor (PostToneStage), and as the exposure DLSS reads (the area's exposure x a) at
// the next frame. The math the shader runs is mirrored on the CPU (auto_exposure_math.h) for the unit test.
struct AutoExposureParams {
    float target{};            // the target: the mean displayed grey (0..1) over the kept pixels (one for the game)
    float low{0.10f},high{0.90f};   // the kept pixels' percentiles
    float speed_brighter{3.0f},speed_darker{1.0f},transition{1.5f};   // stops a second (Unreal's law as its documentation states it)
    float min_log2{-8.0f},max_log2{10.0f};   // the factor's bounds (log2 a)
    float strength{1.0f};      // the adaptation's strength: the goal is strength x the solved log2 a (1 the full adaptation, 0 the area's own exposure)
    bool reset{};              // snap to the solved factor (a cut)
    float fixed_dt{-1.0f};     // the unit test's step (seconds); negative: the CPU's time between two Records
};
struct AutoExposureConstantMemory {void* cpu{};D3D12_GPU_VIRTUAL_ADDRESS gpu{};};
using AutoExposureConstantSink=std::function<AutoExposureConstantMemory(uint64_t bytes)>;
// What the pass found, read back off the frame's path (eight frames late).
struct AutoExposureReading {float log2_factor{},goal_log2{},mean{},area_exposure{},target{};uint32_t pixels{};bool valid{};};
class AutoExposureStage {
public:
    AutoExposureStage()=default;
    AutoExposureStage(const AutoExposureStage&)=delete;
    AutoExposureStage& operator=(const AutoExposureStage&)=delete;
    static constexpr uint32_t bins=256;
    static constexpr float log2_min=-20.0f,bin_width=0.125f;   // log2 luminance over [-20, 12)
    bool Open(ID3D12Device* device,std::string& reason);
    bool IsOpen() const noexcept {return pso_adapt_!=nullptr;}
    // scene: the HDR the tone map reads (R16G16B16A16_FLOAT) in NON_PIXEL_SHADER_RESOURCE (the caller's
    // transitions: the frame graph drives that resource's barriers); the stage's own resources keep their barriers inside it. tone: the tone-map pass's parameter block of this frame (its exposure, curve, colour matrix and 1 / gamma read). slot: the frame slot.
    void Record(ID3D12GraphicsCommandList* list,ID3D12Resource* scene,uint32_t slot,const PostToneParams& tone,const AutoExposureParams& params,
        const AutoExposureConstantSink& constants);
    // The 1x1 factor (PIXEL_SHADER_RESOURCE after Record, until the next Record), or null before the first Record.
    ID3D12Resource* Factor() const noexcept {return recorded_?factor_.Get():nullptr;}
    // Copies the exposure DLSS reads (the area's exposure x a, the last Record's) into `exposure` (a 1x1 R32_FLOAT in COPY_DEST);
    // false before the first Record (the caller then uploads its own).
    bool CopyDlssExposure(ID3D12GraphicsCommandList* list,ID3D12Resource* exposure);
    AutoExposureReading LastReading() const;
    uint64_t Frames() const noexcept {return frames_.load();}
    void Close() noexcept;
    void QuarantineToProcessExit() noexcept;
private:
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_histogram_,pso_adapt_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> views_;uint32_t view_stride_{};
    Microsoft::WRL::ComPtr<ID3D12Resource> histogram_,state_,factor_;
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,8> readbacks_;uint64_t readback_frames_{};
    std::chrono::steady_clock::time_point last_{};bool timed_{},recorded_{};
    AutoExposureReading reading_{};
    std::atomic<uint64_t> frames_{};
};
}
