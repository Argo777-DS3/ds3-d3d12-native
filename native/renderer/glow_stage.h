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
#include <functional>
#include <string>
#include <vector>

namespace ds3native {
// W6 GL-5 (GLOW-WAVE1-READY 5.3, 5.4): the original's glow chain, mode B of FUN_1401BB810, as ten fixed draws of a 4-vertex
// strip: extraction (GB5 -> S, EmissiveToGlowPS), four 13-tap blurs halving down (S -> L0 a quarter -> L1 -> L2 -> L3,
// Sample13Weight), three additive composites up (L3 -> L2 -> L1 -> L0) and one onto S (GlowCompositeSinglePS with Blur RgbScale),
// then S onto the caller's final colour target (NewGlow Final ColorScale). S and L0..L3 are R11G11B10_FLOAT render targets
// owned by the stage; the programmes are the original's DXBC (the caller checks the pins below before Open).
struct GlowBlobs {std::vector<uint8_t> vertex,extract,blur,composite;};
// GLOW-WAVE1-READY 2: the four containers of the gxposteffect bundle (ScreenSpaceQuadShader, EmissiveToGlowPS, Sample13Weight,
// GlowCompositeSinglePS), bytes and SHA-256 over the whole container, in GlowBlobs order.
struct GlowPin {uint32_t bytes;const char* sha256;};
inline constexpr std::array<GlowPin,4> glow_pins{{
    {616,"94EFD8245EE14DBA8B5018F3E762C487DB55788E3E6D90800737E45149DA168A"},
    {744,"BC02ECF1710A075266122E4E28D6DF6AE0BCAB7B2B349FFB1C13D1C22D4E933B"},
    {1384,"CD4CE825F5B81AE66E9624C125E863E68B7AD8BE953A7DA6CB95532328932AA2"},
    {936,"F85C2B7C55774BE4418369DFC7CBFE4ABDDCA4AB48EC9E8A82495255F65D75B3"}}};
// The frame's values (GLOW-WAVE1-READY 3): BlurSampleScale (cbSampleOffset), the composites' cbGlowParam.x already faded
// (Rgb for passes 6-9, Final for pass 10) and AlphaScale's raw dword (cbGlowParam.y, read by no shader).
struct GlowValues {float blur_scale{1.0f},rgb_scale{},final_scale{};uint32_t alpha_bits{};};
// Where the stage records its barriers and declares its passes (the renderer routes them through its frame-graph shadow; a
// test records plain barriers). `Pass` follows the barriers of its draw: `read` is sampled, `write` is the render target.
struct GlowRecorder {
    virtual void Barrier(ID3D12GraphicsCommandList* list,ID3D12Resource* resource,D3D12_RESOURCE_STATES from,D3D12_RESOURCE_STATES to)=0;
    virtual void Pass(const char* name,ID3D12Resource* read,ID3D12Resource* write,bool blend)=0;
    virtual ~GlowRecorder()=default;
};
// Per-frame constants: 256-byte aligned upload memory live until the list completes (the renderer's frame arena).
struct GlowConstants {void* cpu{};D3D12_GPU_VIRTUAL_ADDRESS gpu{};};
using GlowConstantSink=std::function<GlowConstants(uint64_t bytes)>;
class GlowStage {
public:
    GlowStage()=default;
    GlowStage(const GlowStage&)=delete;
    GlowStage& operator=(const GlowStage&)=delete;
    static constexpr DXGI_FORMAT TargetFormat() noexcept {return DXGI_FORMAT_R11G11B10_FLOAT;}
    // I-0138 tranche 5: the final composite's second pipeline, onto the HDR10 chain's float scene (Record's `float_final`).
    static constexpr DXGI_FORMAT FloatFinalFormat() noexcept {return DXGI_FORMAT_R16G16B16A16_FLOAT;}
    static constexpr uint32_t PassesPerFrame=10;
    static constexpr uint64_t ConstantBytes=8*256;   // cbSampleOffset, four cbFilterParam, two cbGlowParam, one zero block
    // Root, four pipelines, quad and the five targets of the extent; false with a reason (the frame then has no glow).
    bool Open(ID3D12Device* device,const GlowBlobs& blobs,DXGI_FORMAT final_format,uint32_t width,uint32_t height,std::string& reason);
    // I-0138 tranche 5: the final composite's pipeline for a FloatFinalFormat target, made at the first call (an HDR10 chain's
    // frame; nothing on an SDR chain); false with a reason, the stage left as it was.
    bool FloatFinal(const GlowBlobs& blobs,std::string& reason);
    bool IsOpen() const noexcept {return extract_!=nullptr;}
    uint32_t Width() const noexcept {return width_;}
    uint32_t Height() const noexcept {return height_;}
    // After the caller's drain: the five targets and their views at the new extent.
    void Resize(uint32_t width,uint32_t height);
    // The five glow targets, S first (render targets between frames), for the caller's frame graph.
    std::array<ID3D12Resource*,5> Targets() const noexcept;
    // The ten passes. gb5 (format gb5_format) is in RENDER_TARGET on entry and exit; final_rtv (the final format, the stage's
    // extent) is a render target the caller keeps in RENDER_TARGET. The descriptor heap and root signature are the stage's
    // afterwards (the caller sets its own before its next draw).
    // gb5_generation: the caller's target generation (a recreated GB5 at a reused address still gets a new view).
    void Record(ID3D12GraphicsCommandList* list,ID3D12Resource* gb5,DXGI_FORMAT gb5_format,uint64_t gb5_generation,D3D12_CPU_DESCRIPTOR_HANDLE final_rtv,
        const GlowValues& values,const GlowConstantSink& constants,GlowRecorder& recorder,bool float_final=false);
    uint64_t Passes() const noexcept {return passes_.load();}
    uint64_t Frames() const noexcept {return frames_.load();}
    uint64_t TargetBytes() const noexcept;
    void Close() noexcept;
    void QuarantineToProcessExit() noexcept;   // a failed close: every object leaked (a list in flight may use them)
private:
    void MakeTargets();
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_;
    void MakePipeline(const GlowBlobs& blobs,const std::vector<uint8_t>& pixel,DXGI_FORMAT format,bool additive,Microsoft::WRL::ComPtr<ID3D12PipelineState>& out,const char* what);
    Microsoft::WRL::ComPtr<ID3D12PipelineState> extract_,blur_,level_,final_,final_float_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtv_,srv_;
    Microsoft::WRL::ComPtr<ID3D12Resource> quad_;
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,5> targets_;   // S, L0, L1, L2, L3
    ID3D12Resource* gb5_view_{};DXGI_FORMAT gb5_view_format_{};uint64_t gb5_view_generation_{};   // the resource GB5's view was written for
    uint32_t width_{},height_{},rtv_stride_{},srv_stride_{};
    std::atomic<uint64_t> passes_{},frames_{};
};
}
