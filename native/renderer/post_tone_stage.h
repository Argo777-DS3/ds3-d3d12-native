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
#include "post_params.h"

namespace ds3native {
// I-0138 tranche 1 (ROADMAP workstream 11, D-0262): our tone-map pass, in place of the draws of YEBIS's programmes (TonemapStage
// and GradingStage, now only the offline tests' oracle: tranche 8). Our own programmes (shaders/post_tone.hlsl)
// and parameter block (PostToneParams), the same bytes as the original's draws from the same inputs (post_tone_stage_test):
//  - the tone map, one draw from the lit HDR into the caller's 8-bit target: the exposure, the vignette, the sensitometric curve, the
//    glare, the colour matrix, the gamma, the dither;
//  - with a LUT, the tone map draws into the stage's 8-bit intermediate and a second draw takes it through the 16x16x16 LUT into
//    the caller's target, as the original does (the card's own 8-bit write between the two is part of the picture).
// Inputs: t0 the lit HDR at the stage's extent; t1 the glare composite (any extent; null: black); t2 the automatic exposure's 1x1
// factor (null: 1); the 4x4 dither texels (yebis::TonemapDither); the frame's LUT strip (null: no LUT); b0 PostToneParams.
// The LUT volume is the stage's: the map's 16x256 strip (16 slices of 16x16 stacked) copied into a 16x16x16 R32G32B32A32_FLOAT
// volume by our compute (cs_lut_copy, GX's LutCopy2DTo3DCs transcribed, tranche 8: 3D[x][y][z] = 2D[x][16z + y]), when the
// strip or its generation changes. Seam: the caller owns the scene's, the glare's, the factor's, the strip's and the target's
// states; the intermediate's and the volume's barriers stay inside the stage.
struct PostToneConstantMemory {void* cpu{};D3D12_GPU_VIRTUAL_ADDRESS gpu{};};
using PostToneConstantSink=std::function<PostToneConstantMemory(uint64_t bytes)>;
// The 16x256 strip (R8G8B8A8_UNORM family, a shader-resource state that includes NON_PIXEL_SHADER_RESOURCE).
struct PostToneLut {ID3D12Resource* strip{};DXGI_FORMAT format{DXGI_FORMAT_UNKNOWN};uint64_t generation{};};
class PostToneStage {
public:
    PostToneStage()=default;
    PostToneStage(const PostToneStage&)=delete;
    PostToneStage& operator=(const PostToneStage&)=delete;
    static constexpr uint32_t LutEdge=16;
    // Root, the tone map's pipeline and the dither texture (uploaded by the first Record); false with a reason.
    bool Open(ID3D12Device* device,const std::array<uint8_t,64>& dither,DXGI_FORMAT target_format,uint32_t width,uint32_t height,std::string& reason);
    bool IsOpen() const noexcept {return pso_!=nullptr;}
    uint32_t Width() const noexcept {return width_;}
    uint32_t Height() const noexcept {return height_;}
    void Resize(uint32_t width,uint32_t height);   // the caller drains the frames that read the intermediate first
    // The LUT's side, once before a Record with a LUT: the copy's pipeline (cs_lut_copy), the LUT draw's pipeline, the volume and
    // the intermediate; false with a reason.
    bool OpenLut(std::string& reason);
    bool HasLut() const noexcept {return lut_pso_!=nullptr;}
    // I-0138 tranche 5: the HDR entries' pipelines (ps_hdr, ps_hdr_reinhard) into a target of `format` (the output stage's
    // float scene); false with a reason. Once, before a Record with `hdr`.
    bool OpenHdr(DXGI_FORMAT format,std::string& reason);
    // I-0138 tranche 6: a candidate look's pipeline (`look`: a ToneLook's number, 1 AgX, 2 ACES, 3 after Elden Ring; tone_looks.h), its 8-bit entry or,
    // with `hdr` (after OpenHdr), its HDR entry; made once, when a frame first asks that look; false with a reason.
    bool OpenLook(uint32_t look,bool hdr,std::string& reason);
    // The tone map's draw, into target_rtv or, with a LUT, into the intermediate (RecordLut then follows on the same list). scene
    // (R16G16B16A16_FLOAT, the stage's extent), glare (R11G11B10_FLOAT) and adapted (R32G32B32A32_FLOAT) are in
    // PIXEL_SHADER_RESOURCE on entry and exit; target_rtv (the target format) is a render target. With `hdr` (I-0138 tranche 5): one
    // draw of the HDR entry into target_rtv (OpenHdr's format), the LUT read in it (no RecordLut follows). params.flags: the vignette bit is
    // the caller's, the Reinhard bit (I-0138 part 4c) the caller's and picks the pipeline, the look bits (I-0138 tranche 6) the caller's
    // and pick a candidate's pipeline in place of the record's curve's (after OpenLook), the factor's bit follows adapted. The descriptor heap and root signatures are the stage's afterwards.
    void Record(ID3D12GraphicsCommandList* list,ID3D12Resource* scene,uint64_t scene_generation,D3D12_CPU_DESCRIPTOR_HANDLE target_rtv,
        const PostToneParams& params,const PostToneConstantSink& constants,ID3D12Resource* glare=nullptr,uint64_t glare_generation=0,
        ID3D12Resource* adapted=nullptr,const PostToneLut* lut=nullptr,bool hdr=false);
    // The LUT's draw of a frame whose Record had a LUT: the intermediate through the volume into target_rtv.
    void RecordLut(ID3D12GraphicsCommandList* list,D3D12_CPU_DESCRIPTOR_HANDLE target_rtv);
    // With the LUT's side open: the tone map's 8-bit output of a frame with a LUT (RENDER_TARGET between frames) and the volume
    // (PIXEL_SHADER_RESOURCE between frames), for the witnesses and the test.
    ID3D12Resource* Intermediate() const noexcept {return intermediate_.Get();}
    ID3D12Resource* Volume() const noexcept {return volume_.Get();}
    uint64_t Frames() const noexcept {return frames_.load();}
    uint64_t LutFrames() const noexcept {return lut_frames_.load();}
    uint64_t HdrFrames() const noexcept {return hdr_frames_.load();}
    void Close() noexcept;
    void QuarantineToProcessExit() noexcept;
private:
    void MakeIntermediate();
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_,copy_root_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_,reinhard_pso_,lut_pso_,copy_pso_,hdr_pso_,hdr_reinhard_pso_;
    static constexpr uint32_t kLooks=3;   // AgX, ACES, after Elden Ring
    Microsoft::WRL::ComPtr<ID3D12PipelineState> look_pso_[kLooks],hdr_look_pso_[kLooks];DXGI_FORMAT hdr_format_{DXGI_FORMAT_UNKNOWN};
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> views_,rtv_;
    Microsoft::WRL::ComPtr<ID3D12Resource> dither_,dither_upload_,volume_,intermediate_;
    ID3D12Resource* scene_view_{};uint64_t scene_view_generation_{};ID3D12Resource* glare_view_{};uint64_t glare_view_generation_{};
    ID3D12Resource* adapted_view_{};ID3D12Resource* strip_view_{};uint64_t strip_view_generation_{};ID3D12Resource* intermediate_view_{};
    bool dither_ready_{};D3D12_GPU_VIRTUAL_ADDRESS lut_block_{};   // lut_block_: the frame's block, from Record with a LUT to RecordLut
    DXGI_FORMAT format_{DXGI_FORMAT_UNKNOWN};uint32_t width_{},height_{},stride_{};
    std::atomic<uint64_t> frames_{},lut_frames_{},hdr_frames_{};
};
}
