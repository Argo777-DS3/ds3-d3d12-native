#pragma once
#include "backend.h"
#include <map>
#include <memory>

namespace ds3native {
namespace frame {struct PrimitiveWorkPacket;}
// Reusable ordered family encoder. Device, recorder, queue, targets and final
// presentation remain with SceneRenderer; this class creates no owner/window.
class PrimitivePass {
public:
    PrimitivePass();
    ~PrimitivePass();
    PrimitivePass(const PrimitivePass&)=delete;
    PrimitivePass& operator=(const PrimitivePass&)=delete;
    // B6: a build moves whole into a holder retired by ticket; the source is left cleared.
    PrimitivePass(PrimitivePass&& other) noexcept;
    // I-0138 tranche 5: `float_target`: the frame's own target is an R16G16B16A16_FLOAT picture (an HDR10 chain: the output
    // stage's, display-encoded), so the pipelines of the draws into it are made for that format; the blend states are the same
    // (the UI is drawn in place, as on the 8-bit picture). The draws into the packet's own targets are not concerned.
    void Build(ID3D12Device* device,ID3D12GraphicsCommandList* uploads,const frame::PrimitiveWorkPacket& packet,bool float_target=false);
    void Build(ID3D12Device* device,ID3D12GraphicsCommandList* uploads,frame::PrimitiveWorkPacket&& packet,bool float_target=false);   // the build keeps the moved packet
    bool FloatTarget() const noexcept {return float_target_;}
    void Encode(ID3D12GraphicsCommandList* list,D3D12_CPU_DESCRIPTOR_HANDLE color,D3D12_CPU_DESCRIPTOR_HANDLE stencil,bool draw) const;
    void Clear() noexcept;
    void QuarantineToProcessExit() noexcept;
    const frame::PrimitiveWorkPacket* Packet() const noexcept {return packet_.get();}
private:
    struct Draw {
        Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline;
        // 1131: GPU addresses of the two constant blocks, suballocated from pooled upload chunks.
        std::array<D3D12_GPU_VIRTUAL_ADDRESS,2> constants{};
        D3D12_VERTEX_BUFFER_VIEW vertices{};
        D3D12_INDEX_BUFFER_VIEW indices{};
        uint32_t texture_slot{},sampler_slot{};
        std::array<uint64_t,2> reads{};   // F1: the packet targets this draw samples (t0, t1), 0 for none
    };
    std::unique_ptr<frame::PrimitiveWorkPacket> packet_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> bindings_,samplers_;
    std::map<uint64_t,Microsoft::WRL::ComPtr<ID3D12Resource>> buffers_,textures_;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> uploads_;
    // 1131: pooled upload chunks held by this build (returned when Clear() drops them after retirement).
    std::vector<std::shared_ptr<void>> pooled_;
    struct Suballocation {ID3D12Resource* resource{};D3D12_GPU_VIRTUAL_ADDRESS address{};uint64_t size{};};
    std::map<uint64_t,Suballocation> pooled_buffers_;
    std::vector<Draw> draws_;
    uint32_t binding_stride_{},sampler_stride_{};
    // F1 (I-0067 SCALEFORM-FILTERS-NATIVE): the resources backing the packet's targets (pooled by size, held until this
    // build's fenced retirement; one resource serves every target whose life does not overlap another's on it), their
    // views (slot = index), and each packet target's resource. Every resource is in PIXEL_SHADER_RESOURCE outside Encode.
    struct Target {std::shared_ptr<void> pooled;ID3D12Resource* color{};ID3D12Resource* stencil{};uint32_t width{},height{};};
    std::vector<Target> targets_;
    std::map<uint64_t,uint32_t> target_slots_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> target_rtvs_,target_dsvs_;
    uint32_t rtv_stride_{},dsv_stride_{};bool float_target_{};
};
}
