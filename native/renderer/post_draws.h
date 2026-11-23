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
#include <cstdint>
#include <string>
#include <vector>

namespace ds3native {
// W10 GL-1 (D-0175, W10-NATIVE-PASSES 5): the draw of the library's 2D post programmes (vs_2D_Tex2D1, vs_2D_OffsetTex2Dn with
// ps_MadT1_Sn, ps_BrightPass*, ps_MadLumGammaTn and the like) as the original issues it, observed per draw (i66-reference-1648):
// three vertices of stride 16 (POSITION, TEXCOORD0: (-1, 1) (0, 1), (-1, -3) (0, -1), (3, 1) (2, 1); the VS scales the position by
// fParam_ScreenSpaceScale and adds afUV_TexCoordOffsetV16[i] to the coordinate), the `$Globals` block in b0 for both stages, up to six
// textures t0..t5 and their samplers s0..s5 as the original binds them for that draw, opaque output into one target at its full
// extent. The caller owns the targets and the blocks; the class owns the root signature, the pipelines (one per programme pair and
// target format), the shader-visible heaps (SRV tables of eight and sampler tables of eight, with the original's sampler descriptions:
// filter and address modes as D3D11 gave them, which D3D12 numbers alike; border colour transparent black; MaxLOD 0 unless a sampler gives one) and the vertices.
struct PostSampler {D3D12_FILTER filter{D3D12_FILTER_MIN_MAG_MIP_POINT};D3D12_TEXTURE_ADDRESS_MODE address{D3D12_TEXTURE_ADDRESS_MODE_CLAMP};
    float max_lod{};                    // LS-3: the original's MaxLOD (the film grain's sampler: FLT_MAX, its CPU mips)
    bool operator==(const PostSampler&) const=default;};
using PostSamplers=std::array<PostSampler,8>;
class PostDraws {
public:
    static constexpr uint32_t Inputs=8;   // I-0138 tranche 8b: the constant block's size is the programmes' (the glare family's: yebis::glare_block_bytes)
    PostDraws()=default;
    PostDraws(const PostDraws&)=delete;
    PostDraws& operator=(const PostDraws&)=delete;
    // tables: the number of eight-SRV tables the caller addresses (each filled with null views until SetInput).
    bool Open(ID3D12Device* device,uint32_t tables,std::string& reason);
    bool IsOpen() const noexcept {return root_!=nullptr;}
    // A pipeline for this programme pair and target format (built once; the index stays valid until Close); throws on failure.
    // GL-4: texcoords 2, the layout of stride 24 with TEXCOORD1 (the vignette bright pass, vs_2D_Tex2D2); LS-3: 3 and 4 (the light shafts'
    // mask and radial blurs, vs_2D_Tex2D3 and vs_2D_Tex2D4); with more than one, the draw's vertices are given per draw.
    // LS-1: write_mask, the target's channel mask (D3D12_COLOR_WRITE_ENABLE_*; Alpha_R writes the alpha only, 8).
    uint32_t Pipeline(const std::vector<uint8_t>& vertex,const std::vector<uint8_t>& pixel,DXGI_FORMAT target,uint32_t texcoords=1,uint8_t write_mask=D3D12_COLOR_WRITE_ENABLE_ALL);
    // Writes slot (0..7) of a table: a 2D view of resource in view_format, or a null view (samples 0) when resource is null.
    void SetInput(uint32_t table,uint32_t slot,ID3D12Resource* resource,DXGI_FORMAT view_format);
    // Optimisation review axis 2: a fresh SRV heap (every slot a null view) in place of the current one, which is retired by
    // ticket (RetireWhenSubmittedWorkCompletes), so a layout whose tables are rewritten drains nothing; the caller then writes
    // every input again.
    void RenewTables();
    // The sampler table for these eight descriptions (cached; at most 32 distinct sets).
    uint32_t SamplerTable(const PostSamplers& samplers);
    // Heaps and root signature; then any number of Draw (each sets its topology). The heaps and root signature stay bound afterwards.
    void Begin(ID3D12GraphicsCommandList* list) const;
    // globals: the block the pipeline's programmes read, at a 256-aligned GPU address. target: a render target view of the pipeline's format, drawn at
    // (0, 0, width, height).
    void Draw(ID3D12GraphicsCommandList* list,uint32_t pipeline,uint32_t table,uint32_t samplers,D3D12_GPU_VIRTUAL_ADDRESS globals,
        D3D12_CPU_DESCRIPTOR_HANDLE target,uint32_t width,uint32_t height,const D3D12_VERTEX_BUFFER_VIEW* vertices=nullptr,   // vertices: a pipeline with more than one texcoord pair
        bool strip=false) const;   // I-0138 tranche 8f: the ghosts' four-vertex triangle strip (our own vertices only)
    void Close() noexcept;
    void QuarantineToProcessExit() noexcept;
private:
    struct PipelineKey {std::vector<uint8_t> vertex,pixel;DXGI_FORMAT format{};uint32_t texcoords{};uint8_t write_mask{};};
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_;
    std::vector<Microsoft::WRL::ComPtr<ID3D12PipelineState>> pipelines_;std::vector<PipelineKey> keys_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srv_,samplers_;
    Microsoft::WRL::ComPtr<ID3D12Resource> vertices_;
    std::vector<PostSamplers> sampler_sets_;
    uint32_t tables_{},srv_stride_{},sampler_stride_{};
};
}
