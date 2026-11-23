#pragma once
#include "post_draws.h"
#include "../world/yebis_glare.h"
#include <functional>
#include <map>
#include <memory>

namespace ds3native {
// W10 GL-3 (D-0175, W10-NATIVE-PASSES 5): our glare family, the plan of yebis::BuildGlare drawn with the library's programmes
// through PostDraws into surfaces the stage owns (RGBA16F for the source chain and the bright pass, R11G11B10 for the pyramid),
// from the scene HDR to the tone map's t1 (2x the base size). Seam (method 4): the input is the scene HDR and the plan, the output
// the t1 surface; a modern bloom can replace the stage behind the same seam.
struct GlareBlobs {std::vector<uint8_t> vertex,pixel;};
using GlarePrograms=std::map<std::string,GlareBlobs>;   // by technique
struct GlareConstantMemory {void* cpu{};D3D12_GPU_VIRTUAL_ADDRESS gpu{};};
using GlareConstantSink=std::function<GlareConstantMemory(uint64_t bytes)>;
class GlareStage {
public:
    static constexpr uint64_t vertex_request_bytes=256;   // a draw's vertices from the constant sink (the block's request is yebis::glare_block_bytes)
    GlareStage()=default;
    GlareStage(const GlareStage&)=delete;
    GlareStage& operator=(const GlareStage&)=delete;
    bool Open(ID3D12Device* device,std::string& reason);
    bool IsOpen() const noexcept {return post_.IsOpen();}
    // Draws the plan from scene (R16G16B16A16_FLOAT, PIXEL_SHADER_RESOURCE on entry and exit); the surfaces follow the plan's
    // layout (recreated when it changes; since the optimisation review's axis 2 the former objects are retired by ticket, no drain).
    // Returns the plan's output surface (R11G11B10_FLOAT, PIXEL_SHADER_RESOURCE), or null with a reason. The heaps and root
    // signature are the stage's afterwards.
    // LS-3: the shafts' film grain (input -3) is the executable's block with its CPU mips (yebis::FilmGrainLevels), uploaded on the
    // first layout that reads it; film_grain may be null for a plan without shafts.
    // MB-2: a draw's input -4 reads depth's plane 0 (R32G8X24, DEPTH_READ | PIXEL_SHADER_RESOURCE during the call) and -6 the engine's
    // object velocity (R8G8B8A8_UNORM, PIXEL_SHADER_RESOURCE): the motion blur's velocity draw.
    // I-0138 tranche 8f: input -7 is the ghost's trim (yebis::TrimCircle, uploaded once); a draw with copy_to copies its target into that
    // surface after it (the afterimage's history: black when a layout creates it, kept across one at its size and format); a strip draw takes PostDraws' four-vertex strip.
    ID3D12Resource* Record(ID3D12GraphicsCommandList* list,ID3D12Resource* scene,uint64_t scene_generation,const yebis::GlarePlan& plan,
        const GlarePrograms& programs,const GlareConstantSink& constants,std::string& reason,const std::vector<std::vector<uint8_t>>* film_grain=nullptr,
        ID3D12Resource* depth=nullptr,ID3D12Resource* object_velocity=nullptr,
        bool clear_history=false);   // I-0138 tranche 8f: the afterimage's history cleared before the draws (a camera cut)
    // LS-1: the plan's pre draws (Alpha_R) into the scene: scene in RENDER_TARGET and the depth's plane 0 readable
    // (DEPTH_READ | PIXEL_SHADER_RESOURCE) on entry and exit, the caller's transitions; false with a reason.
    bool RecordPre(ID3D12GraphicsCommandList* list,ID3D12Resource* scene,ID3D12Resource* depth,uint64_t scene_generation,const yebis::GlarePlan& plan,
        const GlarePrograms& programs,const GlareConstantSink& constants,std::string& reason,const std::vector<std::vector<uint8_t>>* film_grain=nullptr,
        const std::vector<float>* table=nullptr);   // DF-1: the DoF table (input -5) of this frame, table_width x table_height values
    // Whether this plan needs new surfaces (Record renews them; the former ones are retired by ticket). I-0138 tranche 8e: a plan of
    // pre draws alone keeps the main layout of the last full plan (only its pre draws are compared; their post tables sit at the top
    // of the heap, the main draws' at its bottom, and the main draws' inputs are rewritten from the layout's own record).
    bool NeedsLayout(const yebis::GlarePlan& plan) const;
    uint64_t Frames() const noexcept {return frames_;}
    uint64_t Generation() const noexcept {return generation_;}   // advances with each new surface layout
    ID3D12Resource* Surface(size_t i) const noexcept {return i<surfaces_.size()?surfaces_[i].Get():nullptr;}   // for the offline chain test
    void Close() noexcept;
    void QuarantineToProcessExit() noexcept;
private:
    void ClearHistory(ID3D12GraphicsCommandList* list,const yebis::GlarePlan& plan);
    bool Layout(ID3D12GraphicsCommandList* list,const yebis::GlarePlan& plan,const GlarePrograms& programs,const std::vector<std::vector<uint8_t>>* film_grain,std::string& reason);
    // Axis 2 (pre-merge review): the plan's fixed inputs (the surfaces, the film grain, the DoF table, null elsewhere) written
    // into the post tables, after Layout and after RenewTables when a frame input changed (the scene, the depth, the velocity),
    // so that no table is rewritten in place while a frame in flight reads it.
    // The frame inputs are the caller's current resources (a null one leaves its slots null); the markers take them, so a
    // stale resource (one released since, as a target recreated at a resize) is never viewed again.
    void WriteDrawInputs(const yebis::GlarePlan& plan,ID3D12Resource* scene,ID3D12Resource* depth,ID3D12Resource* velocity,ID3D12Resource* pre_depth);
    bool Draw(ID3D12GraphicsCommandList* list,const yebis::GlareDraw& g,size_t index,uint32_t table,D3D12_CPU_DESCRIPTOR_HANDLE rtv,uint32_t width,uint32_t height,
        const GlareConstantSink& constants,std::string& reason);
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    PostDraws post_;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> surfaces_;std::vector<yebis::GlareSurface> layout_;
    int history_=-1;   // I-0138 tranche 8f: the laid-out plan's history surface (kept across a relayout at the same size and format)
    std::vector<std::string> techniques_;std::vector<std::array<int8_t,8>> inputs_;
    std::vector<uint32_t> pipelines_,samplers_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtv_;uint32_t rtv_stride_{};
    ID3D12Resource* scene_view_{};uint64_t scene_view_generation_{};
    ID3D12Resource* depth_view_{};ID3D12Resource* velocity_view_{};   // MB-2: the views of inputs -4 and -6 Record wrote
    ID3D12Resource* pre_scene_{};ID3D12Resource* pre_depth_{};uint64_t pre_generation_{};   // LS-1: the views the pre draws were written for
    Microsoft::WRL::ComPtr<ID3D12Resource> film_grain_,film_grain_upload_;   // LS-3: the film grain and its upload (kept for the session)
    Microsoft::WRL::ComPtr<ID3D12Resource> trim_,trim_upload_;   // I-0138 tranche 8f: the ghost's trim (input -7) and its upload (kept for the session)
    Microsoft::WRL::ComPtr<ID3D12Resource> table_;std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,8> table_uploads_;   // DF-1: the DoF table, its uploads
    uint32_t table_width_{},table_height_{};uint64_t table_next_{};
    std::vector<uint32_t> pre_pipelines_,pre_samplers_;std::vector<yebis::GlareDraw> pre_shape_;
    uint64_t frames_{},generation_{};std::vector<yebis::GlareDraw> shape_;   // shape_: the draws' fields the cached state depends on (blocks empty)
};
}
