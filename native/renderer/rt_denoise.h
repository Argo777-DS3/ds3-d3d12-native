#pragma once
// I-0126 R7c (R7-PLAN §7, D-0252): NVIDIA's NRD denoising the traced GI on the frames Ray Reconstruction does not denoise; I-0132 R11
// (Thomas, 2026-11-09 09:24: « NRD, qui s'active quand le RR est désactivé puis passe le relais au RR »): REBLUR_DIFFUSE_SPECULAR, the
// traced specular with it. NRD makes no graphics call: it describes its pipelines, its texture pools and each frame's dispatches, which this stage
// records with the engine's own D3D12 objects (its root signature, its descriptor heaps, its textures, its barriers). The library
// and its programmes are compiled by Build-GameBootstrap.ps1 from NRD 4.17.3's sources outside the repository (nrd_shaders.h maps
// each pipeline's shaderIdentifier to its DXIL). This software contains source code provided by NVIDIA Corporation.
#include "backend.h"
#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace nrd {struct Instance;}

namespace ds3native {
// The signal pass's targets (rt_reflect.hlsl's nrd_signal), NRD's inputs, in this order: IN_DIFF_RADIANCE_HITDIST (the GI's radiance
// demodulated, its normalized hit distance), IN_NORMAL_ROUGHNESS (NRD_FrontEnd_PackNormalAndRoughness, NRD_NORMAL_ENCODING 2),
// IN_VIEWZ (the view's +Z distance), IN_MV (pixels, previous minus current), IN_SPEC_RADIANCE_HITDIST (the specular lobe's radiance
// before its term, its normalized hit distance). The outputs OUT_DIFF_RADIANCE_HITDIST and OUT_SPEC_RADIANCE_HITDIST are RGBA16F.
inline constexpr uint32_t kRtDenoiseInputs=5,kRtDenoiseOutputs=2;
inline constexpr std::array<DXGI_FORMAT,kRtDenoiseInputs> kRtDenoiseInputFormats{DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R10G10B10A2_UNORM,
    DXGI_FORMAT_R32_FLOAT,DXGI_FORMAT_R16G16_FLOAT,DXGI_FORMAT_R16G16B16A16_FLOAT};
inline constexpr DXGI_FORMAT kRtDenoiseOutputFormat=DXGI_FORMAT_R16G16B16A16_FLOAT;
enum class RtDenoised : uint32_t {Diffuse=0,Specular=1};
// REBLUR's hit distance normalization (ReblurHitDistanceParameters: A, B, C), given to NRD and to the signal pass alike.
inline constexpr std::array<float,3> kRtDenoiseHitDistance{3.0f,0.1f,20.0f};
// What a pixel past this view Z holds is not denoised (the sky: the signal pass writes kRtDenoiseSkyZ there).
inline constexpr float kRtDenoiseRange=1.0e5f,kRtDenoiseSkyZ=2.0e5f;
// One frame's camera for NRD: column-major (a column vector), unjittered; jitter in pixels ([-0.5, 0.5]); `restart` drops the
// history (a cut, a size change, a gap in the frames NRD served).
struct RtDenoiseCamera {
    std::array<float,16> view_to_clip{},world_to_view{};
    std::array<float,2> jitter{};
    bool restart{};
};
class RtDenoiser {
public:
    RtDenoiser()=default;
    RtDenoiser(const RtDenoiser&)=delete;
    RtDenoiser& operator=(const RtDenoiser&)=delete;
    ~RtDenoiser();
    // The instance (REBLUR_DIFFUSE_SPECULAR), its root signature and pipelines (made once), its pools and the inputs and outputs at
    // width x height (made again on a new size; the former textures dropped: a list in flight holds what it reads, Hold). False
    // with a reason (the frames without RR then keep no traced GI and no specular: unserved, I-0132 R11e).
    bool Open(ID3D12Device* device,uint32_t width,uint32_t height,std::string& reason);
    bool IsOpen() const noexcept {return instance_!=nullptr && inputs_[0]!=nullptr;}
    uint64_t Generation() const noexcept {return generation_;}   // advances with every new set of textures
    // The signal pass's render target views (kRtDenoiseInputFormats); the inputs are RENDER_TARGET between frames.
    D3D12_CPU_DESCRIPTOR_HANDLE InputRtv(uint32_t index) const noexcept;
    // The denoised radiances (PIXEL_SHADER_RESOURCE between frames), read by the compose passes.
    ID3D12Resource* Output(RtDenoised which) const noexcept {return outputs_[uint32_t(which)].Get();}
    // NRD's dispatches for this frame on `list`: the inputs moved to NON_PIXEL_SHADER_RESOURCE and back, the outputs to
    // UNORDERED_ACCESS and back, the pools in the states NRD's dispatches ask (tracked across frames); the stage's root
    // signature and descriptor heap set (the caller's set again after it); `upload` the frame's constants. False with a reason, and
    // nothing recorded, when NRD refuses the frame (its settings, a dispatch the stage cannot bind).
    bool Record(ID3D12GraphicsCommandList* list,const RtDenoiseCamera& camera,const std::function<UploadArena::Range(uint64_t)>& upload,std::string& reason);
    // Every texture this frame's dispatches read or write (the caller's slot holds them until its list completes).
    void Hold(std::vector<std::shared_ptr<const void>>& kept) const;
    void Close() noexcept;   // after the queue's drain
private:
    struct Texture {Microsoft::WRL::ComPtr<ID3D12Resource> resource;D3D12_RESOURCE_STATES state{};};
    void MakePipelines(std::string& reason);
    void MakeTextures(uint32_t width,uint32_t height);
    Texture* Find(uint32_t type,uint16_t index) noexcept;
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    nrd::Instance* instance_{};
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_;
    std::vector<Microsoft::WRL::ComPtr<ID3D12PipelineState>> pipelines_;
    std::vector<Texture> permanent_,transient_;
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,kRtDenoiseInputs> inputs_;D3D12_RESOURCE_STATES input_state_{D3D12_RESOURCE_STATE_RENDER_TARGET};
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,kRtDenoiseOutputs> outputs_;
    // Every texture's SRV and UAV prepared once a size (non-shader-visible), copied into the shader-visible ring per dispatch.
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> views_,ring_,rtvs_;
    uint32_t view_stride_{},rtv_stride_{};bool settings_set_{};
    RtDenoiseCamera previous_{};   // the last recorded frame's camera (NRD's previous matrices and jitter)
    uint32_t width_{},height_{},frame_index_{};uint64_t generation_{},frames_{};
    uint32_t srv_max_{},uav_max_{};
};
}
