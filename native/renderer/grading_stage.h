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
#include <vector>

namespace ds3native {
// I-0138 tranche 8 (ROADMAP workstream 11): no longer linked into the module (our pass, PostToneStage, draws the colour grading); kept
// as the offline tests' oracle only: the original's programmes read at test time from the player's executable (the draw) and the
// I-0063 programme cache (GX's LutCopy2DTo3DCs) (post_tone_stage_test, grading_stage_test).
// W10 GR (D-0175, W10-NATIVE-PASSES 12): our colour grading pass, as the original draws it after the tone map where the blended
// ColorGrading record is enabled and its LUT texture exists (m31: Reference1696 draw 50). Two steps, each with the original's
// programme (DXBC from the image and the engine's compute repository, checked against their pins by the caller):
//  1. GX's LutCopy2DTo3DCs (FUN_1400E5110, each frame the grading is on): the 16x256 LUT texture (16 slices of 16x16 stacked, the
//     map's mAA_BB_CGrading<id>) into the 16x16x16 R32G32B32A32_FLOAT volume, 3D[x][y][z] = 2D[x][16z + y], Dispatch(1, 1, 16);
//  2. tech_MadColorGradingLutT1_S1 (vs_2D_OffsetTex2D1, ps_MadColorGradingLutT1_S1): the tone-mapped image t0 (s0) through the volume t16
//     (s1), both samplers linear with point mips, clamp, MaxLOD 0 (the original's 20/3/3/0): rgb = LUT(t0.rgb x Modulate[0].rgb
//     x Offset[0].rgb + Offset[1].rgb), a = t0.a x Modulate[0].a, into the caller's 8-bit target.
// The stage owns the tone map's target while it is on (Source(): the tone map draws there), the volume and its programmes. Seam
// (method 4): the inputs are the LUT texture and the tone-mapped image, the output the caller's target.
struct GradingBlobs {std::vector<uint8_t> vertex,pixel,lut_copy;};
struct GradingConstantMemory {void* cpu{};D3D12_GPU_VIRTUAL_ADDRESS gpu{};};
using GradingConstantSink=std::function<GradingConstantMemory(uint64_t bytes)>;
class GradingStage {
public:
    GradingStage()=default;
    GradingStage(const GradingStage&)=delete;
    GradingStage& operator=(const GradingStage&)=delete;
    static constexpr uint32_t GlobalsBytes=4096,LutEdge=16;
    // The `$Globals` words the two programmes read, as the original binds them for the draw (Reference1696 draw 50): the VS's
    // fParam_ScreenSpaceScale (1, -1) at byte 80 and afUV_TexCoordOffsetV16[0] (zero) at 544; the PS's afRGBA_Modulate[0] (1, 1, 1, 1)
    // at 2464, afRGBA_Offset[0] ((N - 1) / N, three times, then 0) at 2976 and afRGBA_Offset[1] (1 / 2N, three times, then 0.5) at
    // 2992, N = 16, the LUT edge FUN_1400E5110 checks (the 0.5 is the captured word, its producer not read: INFERRED unused, the PS
    // reads .xyz). Every other byte zero.
    static void Globals(uint8_t* out);
    bool Open(ID3D12Device* device,const GradingBlobs& blobs,DXGI_FORMAT target_format,uint32_t width,uint32_t height,std::string& reason);
    bool IsOpen() const noexcept {return pso_!=nullptr;}
    uint32_t Width() const noexcept {return width_;}
    uint32_t Height() const noexcept {return height_;}
    void Resize(uint32_t width,uint32_t height);   // the caller drains the frames that read the old target first
    // The tone map's target while the grading is on: a render target of the stage's format and extent, in RENDER_TARGET between
    // frames (Record leaves it there).
    D3D12_CPU_DESCRIPTOR_HANDLE Source() const noexcept {return source_rtv_;}
    ID3D12Resource* SourceResource() const noexcept {return source_.Get();}
    // True when lut (with its generation) is not the texture the copy's view holds: Record renews the view in a fresh heap
    // and the former heap is retired by ticket (the frames that still read it keep it).
    bool NeedsLutView(ID3D12Resource* lut,uint64_t generation) const noexcept {return lut!=lut_view_ || generation!=lut_view_generation_;}
    // lut: the 16x256 LUT texture (R8G8B8A8_UNORM family, any shader-resource state that includes NON_PIXEL_SHADER_RESOURCE);
    // the stage's source must hold this frame's tone-mapped image (RENDER_TARGET on entry and exit). target_rtv: the caller's
    // render target of the stage's format and extent.
    void Record(ID3D12GraphicsCommandList* list,ID3D12Resource* lut,DXGI_FORMAT lut_format,uint64_t lut_generation,
        D3D12_CPU_DESCRIPTOR_HANDLE target_rtv,const GradingConstantSink& constants);
    ID3D12Resource* Volume() const noexcept {return volume_.Get();}   // the 16x16x16 LUT (PIXEL_SHADER_RESOURCE between frames), for the oracle test
    uint64_t Frames() const noexcept {return frames_.load();}
    void Close() noexcept;
    void QuarantineToProcessExit() noexcept;
private:
    void MakeSource();
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_,compute_root_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_,compute_pso_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srv_,rtv_;
    Microsoft::WRL::ComPtr<ID3D12Resource> vertices_,volume_,source_;
    D3D12_CPU_DESCRIPTOR_HANDLE source_rtv_{};
    ID3D12Resource* lut_view_{};uint64_t lut_view_generation_{};
    DXGI_FORMAT format_{};uint32_t width_{},height_{},srv_stride_{};
    std::atomic<uint64_t> frames_{};
};
}
