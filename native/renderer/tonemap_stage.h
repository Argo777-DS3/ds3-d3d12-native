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
// I-0138 tranche 8 (ROADMAP workstream 11): no longer linked into the module (our pass, PostToneStage, draws the tone map); kept
// as the offline tests' oracle only: the original's programmes read from the player's executable at test time (post_tone_stage_test).
// W10 TM-2 (D-0175, W10-NATIVE-PASSES 3.2): our tone-map pass, one draw of the original's programmes vs_2D_Tex2D5 and
// ps_TonemapHDR_Dither_Exposure_Matrix_Gamma (DXBC from the image, checked against yebis_pins.h by the caller) from the scene HDR
// into the caller's 8-bit target, in place of the sRGB composition. Inputs as the original's draw (Reference1632 draw 45): t0 the
// scene HDR, t1 the glare composite (the native glare's; a null view, black, without it), t2 the adapted exposure (a null view: in
// the manual-exposure case the Offset words make its multiplier of t0 and t1 1 and its additive term 0), t3 the 4x4 dither texture; b0 the `$Globals` block the
// caller builds (yebis::TonemapGlobals). Seam (method 4): the inputs are t0..t3 and the block, the output the caller's target.
// pixel_vignette (optional): ps_TonemapHDR_Dither_Vignette_Exposure_Matrix_Gamma (TM-3), the same inputs and block, the vignette
// from the vertices' TEXCOORD2 (yebis::VignetteCoordinates).
struct TonemapBlobs {std::vector<uint8_t> vertex,pixel,pixel_vignette;};
struct TonemapConstantMemory {void* cpu{};D3D12_GPU_VIRTUAL_ADDRESS gpu{};};
using TonemapConstantSink=std::function<TonemapConstantMemory(uint64_t bytes)>;
class TonemapStage {
public:
    TonemapStage()=default;
    TonemapStage(const TonemapStage&)=delete;
    TonemapStage& operator=(const TonemapStage&)=delete;
    static constexpr uint32_t GlobalsBytes=3584;
    // Root, pipeline, vertices of the extent and the dither texture (uploaded by the first Record); false with a reason.
    bool Open(ID3D12Device* device,const TonemapBlobs& blobs,const std::array<uint8_t,64>& dither,DXGI_FORMAT target_format,
        uint32_t width,uint32_t height,std::string& reason);
    bool IsOpen() const noexcept {return pso_!=nullptr;}
    uint32_t Width() const noexcept {return width_;}
    uint32_t Height() const noexcept {return height_;}
    void Resize(uint32_t width,uint32_t height);
    // scene (R16G16B16A16_FLOAT, the stage's extent) is in PIXEL_SHADER_RESOURCE on entry and exit; target_rtv (the target format)
    // is a render target. globals: GlobalsBytes bytes. glare: the glare composite (R11G11B10_FLOAT, any extent, PIXEL_SHADER_RESOURCE),
    // or null for black (no glare this frame; the offline oracle test binds the original's). The descriptor heap
    // and root signature are the stage's afterwards.
    void Record(ID3D12GraphicsCommandList* list,ID3D12Resource* scene,uint64_t scene_generation,D3D12_CPU_DESCRIPTOR_HANDLE target_rtv,
        const uint8_t* globals,const TonemapConstantSink& constants,ID3D12Resource* glare=nullptr,
        const std::array<std::array<float,2>,3>* vignette=nullptr,   // TM-3: the vignette variant with these TEXCOORD2 (needs pixel_vignette)
        uint64_t glare_generation=0,    // GL-3: the glare surface's generation (its view is rewritten when the pair changes)
        ID3D12Resource* adapted=nullptr);   // I-0132 R11h: the automatic exposure's 1x1 RGBA32F factor (PIXEL_SHADER_RESOURCE) as t2, or null
    bool HasVignette() const noexcept {return pso_vignette_!=nullptr;}
    uint64_t Frames() const noexcept {return frames_.load();}
    void Close() noexcept;
    void QuarantineToProcessExit() noexcept;
private:
    void MakeVertices();
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_,pso_vignette_;
    std::array<float,36> vertex_words_{};   // the three static vertices (per-frame vignette vertices copy them with their TEXCOORD2)
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srv_;
    Microsoft::WRL::ComPtr<ID3D12Resource> vertices_,dither_,dither_upload_;
    ID3D12Resource* scene_view_{};uint64_t scene_view_generation_{};ID3D12Resource* glare_view_{};uint64_t glare_view_generation_{};ID3D12Resource* adapted_view_{};bool dither_ready_{};
    uint32_t width_{},height_{},srv_stride_{};
    std::atomic<uint64_t> frames_{};
};
}
