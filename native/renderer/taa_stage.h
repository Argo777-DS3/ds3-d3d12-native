#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <d3d12.h>
#include <wrl/client.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <string>

namespace ds3native {
// I-0132 R11j (R11-PLAN §11, D-0265): the temporal anti-aliasing, the default anti-aliasing without DLSS. ER's mechanism (ER-TAA-STUDY:
// the TempAA mode of GXAntiAliasingFilter, on the HDR scene colour before the tone map, Halton (2, 3) jitter over 8 phases, a ping-pong
// history, the motion dilated) with DS3's inputs: the lit HDR, its depth and the native motion vectors, where DLSS runs (shaders/taa.hlsl).
// I-0140 (gap 23): the fade mask's format (model_draw.h's kFadeMaskFormat, the producer's; scene_pass_velocity.cpp asserts them equal).
inline constexpr DXGI_FORMAT kTaaFadeMaskFormat=DXGI_FORMAT_R8_UNORM;
struct TaaParams {
    float jitter_x{},jitter_y{};   // this frame's camera jitter (render pixels, +x right, +y down)
    float exposure{1.0f};          // the tone map's exposure: the perceptual weights only
    float feedback{0.9f};          // the history's weight at rest
    float clip_gamma{1.25f};       // the variance box's half width in sigmas
    bool reset{};                  // the current colour alone (a cut, a new extent, a frame the TAA did not resolve before)
    float sharpen{};               // Thomas's sharpening, 0 (none, the default as ER's) to 1 (shaders/taa.hlsl's Sharpen)
    // I-0140 tranche 41 (D-0306): the reconstruction's Gaussian, exp(-kernel d^2) over the 3x3 (2.29, the Blackman-Harris fit, the main
    // view's; 0 or less: 2.29), and `own_reset`: a reset frame drawn unjittered (both jitters 0) outputs the pixel's own sample instead.
    float kernel{2.29f};
    bool own_reset{};
    // I-0113 tranche 39: the lit HDR before the 0x29 rows and the effects (the colour's format and extent, in
    // PIXEL_SHADER_RESOURCE|NON_PIXEL_SHADER_RESOURCE, the caller's state); null: no reactive weighting (shaders/taa.hlsl).
    ID3D12Resource* base{};
    // I-0140 (gap 23): the fade mask (kTaaFadeMaskFormat at the colour's extent, 0 where a fading row drew, in
    // PIXEL_SHADER_RESOURCE|NON_PIXEL_SHADER_RESOURCE, the caller's state), read only with `base`; null: none this frame.
    ID3D12Resource* fade_mask{};
    bool reactive_view{};          // I-0113 tranche 41: the weight taken painted red (taa-reactive-view.flag, a test view)
};
class TaaStage {
public:
    TaaStage()=default;
    TaaStage(const TaaStage&)=delete;
    TaaStage& operator=(const TaaStage&)=delete;
    bool Open(ID3D12Device* device,std::string& reason);
    bool IsOpen() const noexcept {return pipeline_!=nullptr;}
    // The step review's H1 and M1: the inputs checked (the colour's format; the depth R32G8X24_TYPELESS, the scene's, viewed as
    // R32_FLOAT_X8X24; the motion, depth and output at the colour's extent, the output in its format) and the history pair made
    // or remade for their extent, before the caller records any barrier: false with `reason` (the caller latches the TAA off; nothing
    // recorded). Record then takes the same resources.
    bool Prepare(ID3D12Resource* colour,ID3D12Resource* depth,ID3D12Resource* motion,ID3D12Resource* output,std::string& reason);
    // colour: the lit HDR (R16G16B16A16_FLOAT) in PIXEL_SHADER_RESOURCE|NON_PIXEL_SHADER_RESOURCE; depth (the scene's) in
    // DEPTH_READ|NON_PIXEL_SHADER_RESOURCE; motion (R16G16_FLOAT) in NON_PIXEL_SHADER_RESOURCE; output (the colour's format and extent,
    // unordered access) in UNORDERED_ACCESS: the caller's states (the frame graph's resources), unchanged by the stage. The history
    // pair is the stage's (its barriers inside it), remade by Prepare at a new extent, and then this Record resets whatever `params`
    // says. slot: the frame slot (its descriptors, whose previous list completed). Returns whether the frame was a reset. Throws
    // std::logic_error when Prepare did not accept these resources (a caller's defect).
    bool Record(ID3D12GraphicsCommandList* list,ID3D12Resource* colour,ID3D12Resource* depth,ID3D12Resource* motion,ID3D12Resource* output,
        const TaaParams& params,uint32_t slot);
    // The history pair, for the caller's slot to hold while its frame is in flight (a new extent remakes the pair).
    std::array<ID3D12Resource*,2> History() const noexcept {return {history_[0].Get(),history_[1].Get()};}
    // I-0132 R12c (Thomas, 2026-11-11 01:05: « garde le actif avec DLSS »): the same sharpening on DLSS's output, in place. `target`
    // (DLSS's output, R16G16B16A16_FLOAT, unordered access) in UNORDERED_ACCESS, the caller's state, unchanged: copied by a compute pass
    // into the stage's scratch (made at the target's extent), then sharpened from it back into the target. slot: the frame slot. Throws
    // on a refused scratch or a target of another format (the caller latches it off).
    void RecordSharpen(ID3D12GraphicsCommandList* list,ID3D12Resource* target,float exposure,float sharpen,uint32_t slot);
    ID3D12Resource* SharpenScratch() const noexcept {return scratch_.Get();}   // for the caller's slot to hold while its frame is in flight
    uint64_t SharpenFrames() const noexcept {return sharpen_frames_.load();}
    uint64_t Frames() const noexcept {return frames_.load();}
    uint64_t Resets() const noexcept {return resets_.load();}
    uint64_t ReactiveFrames() const noexcept {return reactive_frames_.load();}   // I-0113 tranche 39: frames resolved with a base
    uint64_t ViewFrames() const noexcept {return view_frames_.load();}   // I-0113 tranche 41: frames whose output the view painted
    void Close() noexcept;
    void QuarantineToProcessExit() noexcept;
    static constexpr uint32_t slots=3;   // max_frame_slots (backend.h), checked where both are seen
private:
    void MakeHistory(uint32_t width,uint32_t height,DXGI_FORMAT format);
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_,sharpen_,mirror_;
    Microsoft::WRL::ComPtr<ID3D12Resource> scratch_;   // RecordSharpen's copy of the target, NON_PIXEL_SHADER_RESOURCE between frames
    std::atomic<uint64_t> sharpen_frames_{};
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> views_;uint32_t view_stride_{};
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,2> history_;   // UNORDERED_ACCESS between frames
    uint32_t written_{};   // the history the last Record wrote
    bool history_valid_{};  // false until a Record wrote the current pair
    ID3D12Resource* prepared_{};   // the colour the last Prepare accepted (Record checks it is the same)
    std::atomic<uint64_t> frames_{},resets_{},reactive_frames_{},view_frames_{};
};
}
