#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "gbuffer_encoder.h"
#include "model_geometry.h"
#include "../content/model_vertex_layout.h"
#include "../content/shader_interface.h"
#include <array>
#include <atomic>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace ds3native {
struct NativeModelConstantBinding {
    ds3content::ShaderStage stage{};
    uint32_t shader_register{};
    // Exact current payload, including the full effective pose palette when
    // supplied. This component never clips it to the RDEF declaration.
    std::vector<uint8_t> bytes;
    // PREPARE-REDUCTION cP5: an immutable shared payload in place of `bytes` (then empty), so the prepare copies nothing.
    // C4-1b: the recording thread copies each payload object once per view (NativeModelConstantUploads); Native1310-1311
    // had measured a per-chunk-range deduplication in the prepare slower than a copy per draw.
    std::shared_ptr<const std::vector<uint8_t>> shared;
    // C4-1 (PERSISTENT-RECORDS-DESIGN 2): a frame-level block (VS b0, b8, b9 and PS b8, b9, NativeModelFrameRoot): the
    // payload is this programme's packed block, kept for validation only; the draw never uploads it, Encode binds the
    // pass's shared copy as a root CBV. Every binding at a frame register must be one, and no other binding may be.
    bool frame{};
    const std::vector<uint8_t>& Payload() const noexcept {return shared?*shared:bytes;}
};
struct NativeModelSrvBinding {
    ds3content::ShaderStage stage{};
    uint32_t shader_register{};
    D3D12_CPU_DESCRIPTOR_HANDLE descriptor{};
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    // Retains the CPU descriptor heap and current resource/view version.
    std::shared_ptr<const void> owner;
};
struct NativeModelSamplerBinding {
    ds3content::ShaderStage stage{};
    uint32_t shader_register{};
    D3D12_SAMPLER_DESC description{};
};
// PREPARE-REDUCTION cP1/cP3/cP8: the programmes and vertex elements of one programme recipe, shared by every draw of that
// recipe (no per-draw copy), with what Prepare derives from them kept on the set: the verified programme entries, the
// pipelines by fixed-function state and the sampler tables by sampler bank, valid for one device epoch and read and
// written under the draw cache's lock. A miss runs the former path (programme lookup, pipeline key string and lookup,
// sampler sets and their tables), so a kept value is the one that path gives.
struct NativeModelProgramSet {
    ds3content::CpuShaderProgram vertex,pixel;
    std::vector<ds3content::ModelVertexElement> elements;
    // W7 (WATER-TESS-WAVE1-READY 3.4 TS-3): the hull and domain programmes of a tessellated recipe (`_PN`), both or none.
    ds3content::CpuShaderProgram hull,domain;
    struct Derived;
    std::shared_ptr<Derived> derived;
};
std::shared_ptr<const NativeModelProgramSet> MakeNativeModelProgramSet(ds3content::CpuShaderProgram vertex,
    ds3content::CpuShaderProgram pixel,std::vector<ds3content::ModelVertexElement> elements,
    ds3content::CpuShaderProgram hull={},ds3content::CpuShaderProgram domain={});
// I-0134 (the cloth's own velocity; scene_pass_velocity.cpp): the set of a simulated cloth mesh's Velo row. `base` is the row's
// own set (the game's cloth Velo vertex programme, `game_vertex` its declarations, and its pixel programme, GXFlver_Common_Velo
// or the family's _Velo_[A]); the native vertex programme takes its place, the pixel programme kept: the current clip position
// from slot 0's POSITION placed by VC_World (b7 rows 0-2), the previous one from slot 2's PREVPOSITION (the previous simulated
// copy, slot 0's layout) placed by VC_PrevWorld (b7 rows 3-5), where the game's programme places slot 0 twice. Null when the
// game's programme is not a cloth Velo one (VC_isEnableCloth, VC_World, VC_PrevWorld, VC_MatrixWVP, VC_PrevMatrixWVP and
// VC_TexScroll_0 read; SV_Position and TEXCOORD0 to 3 written) or the layout lacks a float POSITION0 in slot 0 or an integer
// TEXCOORD0. The native programme: the build's bytes (shaders/cloth_velocity.hlsl).
std::shared_ptr<const NativeModelProgramSet> MakeNativeClothVelocitySet(const NativeModelProgramSet& base,
    const ds3content::ShaderInterface& game_vertex);
// W0a (MODEL-DRAW-ENVELOPE-READY 3.1): the target set a model draw writes, with one specification table every check reads.
// GBuffer6 the six G-buffer targets; Forward1 the lit HDR target; Forward2 the lit HDR target and GB5 (glow RT1, wave 1);
// Velocity1 the velocity target; DepthOnly no colour target (the shadow atlas, wave 1), the pixel programme optional;
// Decal7 the deferred decals' seven targets (DECALS-WAVE1: SpecularAcc, GB1, GB2, GB3, GB4, GB5, GB0, as 51AA0 binds them);
// Paint2 a struck character's paint target (DECALS-PAINT-READY: the diffuse sRGB and specular RGBA8 textures, shader pass 8).
// I-0140: the TAA's fade mask (scene_pass, shaders/taa.hlsl): cleared to 1, 0 where a fading row draws.
inline constexpr DXGI_FORMAT kFadeMaskFormat=DXGI_FORMAT_R8_UNORM;
inline constexpr uint32_t kFadeMaskPipeline=3;   // Encode's `local` for the fade-mask pipeline (1 and 2 the local-face ones)
enum class ModelPassEnvelope : uint8_t {GBuffer6,Forward1,Forward2,Velocity1,DepthOnly,Decal7,Paint2};
// W6 GL-6 (GLOW-WAVE1-READY 5.6): a forward pixel programme writes GB5 (the glow source) when it declares SV_Target1; such a
// row takes Forward2, a one-output programme Forward1.
inline bool WritesSecondTarget(const ds3content::ShaderInterface& pixel) noexcept {
    for(const auto& output:pixel.outputs)if(output.semantic_index==1 && _stricmp(output.semantic.c_str(),"SV_Target")==0)return true;
    return false;}
struct ModelEnvelopeSpec {uint8_t targets{};bool pixel_required{};};
constexpr ModelEnvelopeSpec EnvelopeSpec(ModelPassEnvelope envelope) noexcept {
    switch(envelope){case ModelPassEnvelope::GBuffer6:return {6,true};case ModelPassEnvelope::Forward2:return {2,true};
        case ModelPassEnvelope::DepthOnly:return {0,false};case ModelPassEnvelope::Decal7:return {7,true};case ModelPassEnvelope::Paint2:return {2,true};
        case ModelPassEnvelope::Forward1:case ModelPassEnvelope::Velocity1:return {1,true};}
    return {0xFF,true};}   // an out-of-range value matches no count (review: no default case, a new envelope is a C4062 error)
// The stage family (W0a-5: the hull and domain stages of the tessellated rows) and the topology class: Model with Triangle,
// or (W7) ModelTess with Patch3 (a 3-control-point patch list, the hull and domain programmes of the set, the tess root
// signature whose hull and domain registers are root CBVs: HS b0, b5, b6 and DS b0, b5, b6, b8; no hull/domain SRV or
// sampler, so the `_D` domains (t5/s4, t12) are not reachable: TS-6).
enum class ModelRootFamily : uint8_t {Model,ModelTess};
enum class ModelTopologyClass : uint8_t {Triangle,Patch3};
// Every field W0a adds lives here (the immutable part a kept draw is keyed on with the programmes, formats and states).
struct NativeModelDrawShape {
    ModelPassEnvelope envelope{ModelPassEnvelope::GBuffer6};
    ModelRootFamily family{ModelRootFamily::Model};
    ModelTopologyClass topology{ModelTopologyClass::Triangle};
    bool operator==(const NativeModelDrawShape&) const=default;
};
struct NativeModelDrawInput {
    // With program_set, vertex_shader, pixel_shader and elements stay empty and the set's are used.
    std::shared_ptr<const NativeModelProgramSet> program_set;
    // With program_set: the identity and version of the sampler descriptions every sampler binding comes from (the
    // scene's sampler bank), so the set can keep the draw's sampler tables; null keeps the per-draw sets.
    const void* sampler_bank{};uint64_t sampler_bank_generation{};
    ds3content::CpuShaderProgram vertex_shader,pixel_shader;
    ModelGeometryBinding geometry;
    std::vector<ds3content::ModelVertexElement> elements;
    D3D12_RASTERIZER_DESC rasterizer{};
    // I-0113 gap 8: a caster row's rasterizers for the local-light faces (the original's mode-3 states, 140101420), [0] the face
    // set's cull class as is, [1] flipped (64610's row 1); Prepare makes a pipeline with each, Encode(..., local) binds one.
    bool local_rasterizer_set{};std::array<D3D12_RASTERIZER_DESC,2> local_rasterizers{};
    // I-0140 (gap 23, a fading body under the TAA): a fading 0x29 forward row; Prepare also makes its fade-mask pipeline (the same
    // programmes and rasterizer into one kFadeMaskFormat target, the depth tested LESS_EQUAL and not written, no stencil, the target
    // written 0 by a ZERO/ZERO blend where a pixel is drawn), which Encode(..., kFadeMaskPipeline) binds.
    bool fade_mask{};
    D3D12_DEPTH_STENCIL_DESC depth_stencil{};
    D3D12_BLEND_DESC blend{};
    std::array<DXGI_FORMAT,8> render_target_formats{};   // DECALS-WAVE1: up to D3D12's eight (Decal7 uses seven)
    // The producer sets the count and formats as before and the envelope beside them; Prepare requires them to agree
    // (W0a: a forgotten envelope keeps GBuffer6 and fails there, not as a 6-target pipeline in the scene's checks).
    uint32_t render_target_count{6};
    NativeModelDrawShape shape;
    DXGI_FORMAT depth_format{};
    UINT sample_mask{};
    std::array<float,4> blend_factor{};
    UINT8 stencil_reference{};
    std::vector<NativeModelConstantBinding> constants;
    std::vector<NativeModelSrvBinding> srvs;
    std::vector<NativeModelSamplerBinding> samplers;
    std::vector<std::shared_ptr<const void>> cpu_owners;
    // C4-2 with frame slots (frames-in-flight workstream, 2026-10-18): copies of the draw's stage tables (1 to 3, the
    // renderer's frame slots for a kept draw). A draw with bounded CBVs encoded in numbered frame F writes copy F % copies, so
    // a kept draw is encoded again while its previous frame is still in flight (ReEncodable); one copy keeps C4-2a's rule.
    uint32_t table_copies{1};
};

// C4-1 (PERSISTENT-RECORDS-DESIGN 2): the five frame-level constant registers: VS b0 cbMatrixWVP, VS b8 cbSceneParam, VS b9
// cbPerFrame, PS b8 cbSceneParam, PS b9 cbPerFrame. Each programme's packed block of such a register is a prefix of one
// canonical block per pass (the packer copies the same source ranges into a zeroed buffer of the declared size), so one
// copy per pass serves every draw.
enum NativeModelFrameRoot : uint32_t {FrameVsView,FrameVsScene,FrameVsPerFrame,FramePsScene,FramePsPerFrame,FrameRootCount};
// The frame root index of a stage (0 vertex, 1 pixel, 2 hull, 3 domain) and register; FrameRootCount when the register is
// not a frame one (no hull or domain register is: W7 binds them per draw).
constexpr uint32_t NativeModelFrameRootOf(uint32_t stage,uint32_t slot) noexcept {
    return !stage?(slot==0?FrameVsView:slot==8?FrameVsScene:slot==9?FrameVsPerFrame:FrameRootCount):
        stage==1?(slot==8?FramePsScene:slot==9?FramePsPerFrame:FrameRootCount):FrameRootCount;}
// W7: the hull and domain registers a tessellated draw binds (root CBVs of the tess root signature).
constexpr bool NativeModelTessConstant(uint32_t stage,uint32_t slot) noexcept {return stage==2?(slot==0 || slot==5 || slot==6):stage==3?(slot==0 || slot==5 || slot==6 || slot==8):false;}
// The GPU addresses of a pass's frame blocks (256-byte aligned, each at least as long as every draw's declaration).
using NativeModelFrameAddresses=std::array<D3D12_GPU_VIRTUAL_ADDRESS,FrameRootCount>;
// C4-1b review: the registers the programmes index dynamically (VS b5 cbObjMatrix, b6 cbPrevObjMatrix, b7 cbvModelParam of
// the Grass programmes (Native1403), b12
// cbClothCancelObjMatrix; PS b4 cbInstanceData, native_constant_dynamic_read) stay descriptor CBVs in the stage tables: a
// descriptor CBV is bounds-checked (a read past its view returns zero, as the D3D11 original relies on), a root CBV is not.
// The prepare refuses a row whose programme indexes any other register dynamically.
constexpr bool NativeModelBoundedConstant(uint32_t stage,uint32_t slot) noexcept {return !stage?(slot>=5 && slot<=7) || slot==12:stage==1 && slot==4;}
// C4-1b: every constant register is a root CBV (parameters 4 to 13 the vertex ones, 14 to 26 the pixel ones) except the
// bounded ones (NativeModelBoundedConstant), descriptor CBVs in the stage tables. A draw keeps its payloads; the recording thread copies each payload object once per view
// through these uploads (keyed by the payload object, which its draws keep alive and unchanged) and binds its address.
// `allocate` returns a 256-byte aligned range of at least the asked size (the slot's upload arena); Address writes the
// payload and zeros to its 256-byte size. VS b5, b6, b7, b12 and PS b4 stay bounded descriptor CBVs (see model_draw.cpp).
class NativeModelConstantUploads {
public:
    struct Range {uint8_t* cpu{};D3D12_GPU_VIRTUAL_ADDRESS gpu{};};
    struct Uploaded {const void* key{};D3D12_GPU_VIRTUAL_ADDRESS gpu{};};   // an entry of the copied-payload table (below)
    // C4-2a: `frame` numbers the recorded frame (1 and up; 0 = unnumbered) and `completed` the frames the GPU has finished
    // (every frame up to it), so a draw kept across frames may rewrite its bounded CBVs once its previous frame is done.
    explicit NativeModelConstantUploads(std::function<Range(uint64_t bytes)> allocate,uint64_t frame=0,uint64_t completed=0);
    ~NativeModelConstantUploads();
    NativeModelConstantUploads(const NativeModelConstantUploads&)=delete;
    NativeModelConstantUploads& operator=(const NativeModelConstantUploads&)=delete;
    uint64_t Frame() const noexcept {return frame_;}
    uint64_t Completed() const noexcept {return completed_;}
    D3D12_GPU_VIRTUAL_ADDRESS Address(const std::vector<uint8_t>& payload);
    uint64_t Blocks() const noexcept {return blocks_;}   // payloads copied
    uint64_t Bytes() const noexcept {return bytes_;}     // bytes copied
private:
    std::function<Range(uint64_t)> allocate_;
    // m45 frame cost (Native1841 sample profile: the node map and its node allocations took a fifth of the render worker):
    // the payloads already copied, an open-addressing table keyed by the payload's address (linear probing, a power-of-two
    // size kept at most half full). Its storage is taken from and given back to a pool of the thread, so a steady frame
    // allocates nothing for it.
    std::vector<Uploaded> uploaded_;size_t uploaded_count_{};
    uint64_t blocks_{},bytes_{},frame_{},completed_{};
};
// One prepared native draw, sharing the startup device and resident geometry
// and SRVs. No queue, submission, target allocation or presentation is owned.
// Caller establishes current producer ownership, actual shader-read coverage,
// dynamic index/palette bounds, vertex format widths within stride and pass
// eligibility before preparation/recording. Structural RDEF availability alone
// does not establish a game draw. Keep this object through Complete and Drain;
// a CPU generation is not completion. Resource states also remain caller-owned.
class OwnedNativeModelDraw {
public:
    OwnedNativeModelDraw(const OwnedNativeModelDraw&)=delete;
    OwnedNativeModelDraw& operator=(const OwnedNativeModelDraw&)=delete;
    const NativeModelDrawInput& Input() const noexcept {return input_;}
    // Returned spans reference this owner (targets reference the caller).
    // If encoding Describe() manually, also set Input().stencil_reference and
    // Input().blend_factor and every constant's root CBV (C4-1b); Encode() does all three.
    GBufferDraw Describe(std::span<const D3D12_CPU_DESCRIPTOR_HANDLE> targets,
        D3D12_CPU_DESCRIPTOR_HANDLE depth,D3D12_VIEWPORT viewport,D3D12_RECT scissor,uint32_t copy=0,uint32_t local=0) const;
    // C4-1: `frame` holds the pass's frame block addresses, the ones this draw binds must be set; C4-1b: the draw's other
    // constant payloads are copied (once per view) through `uploads`.
    void Encode(ID3D12GraphicsCommandList* list,
        std::span<const D3D12_CPU_DESCRIPTOR_HANDLE> targets,
        D3D12_CPU_DESCRIPTOR_HANDLE depth,D3D12_VIEWPORT viewport,D3D12_RECT scissor,const NativeModelFrameAddresses& frame,
        NativeModelConstantUploads& uploads,GBufferBindState* state=nullptr,uint32_t local=0) const;   // m45: `state` filters the loop's repeated state calls; I-0113: `local` 1 + the local-face pipeline (0 none)
    uint32_t FrameMask() const noexcept {return state_->frame_mask;}   // C4-1: bit i = frame root i bound by this draw
    // C4-2 with frame slots: whether the draw may be encoded in every numbered frame of a ring of `slots` (no bounded CBV, or
    // a table copy per slot); otherwise only once its previous frame is complete (C4-2a).
    bool ReEncodable(uint32_t slots) const noexcept {return !state_->bounded || state_->table_copies>=slots;}
    // I-0090: whether this draw took its prepared state from an earlier draw (PrepareNativeModelDraw with a base).
    bool ReusedState() const noexcept {return reused_;}
private:
    friend std::unique_ptr<OwnedNativeModelDraw> PrepareNativeModelDraw(ID3D12Device*,NativeModelDrawInput,const OwnedNativeModelDraw*);
    explicit OwnedNativeModelDraw(NativeModelDrawInput input):input_(std::move(input)){}
    void Prepare(ID3D12Device* device);
    bool Adopt(ID3D12Device* device,const OwnedNativeModelDraw& base);
    NativeModelDrawInput input_;
    // I-0090 (ER keeps the material and pipeline state and rebuilds only the per-frame draw packet, ER-STUDY 5): what Prepare
    // makes from the draw's state inputs (everything but the constant payloads, the blend factor, the stencil reference and
    // the CPU owners), shared by the draws of one row whose state inputs are unchanged (Adopt); the last holder returns the
    // pooled descriptors. The C4-2a encode record lives here, so a draw that takes the state keeps its predecessors' frames.
    struct State {
        Microsoft::WRL::ComPtr<ID3D12RootSignature> root;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline;std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>,2> local_pipelines;   // I-0113: with local_rasterizer_set
        Microsoft::WRL::ComPtr<ID3D12PipelineState> fade_mask_pipeline;   // I-0140: with fade_mask
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> resources,samplers;
        std::array<ID3D12DescriptorHeap*,2> heaps{};
        std::array<D3D12_GPU_DESCRIPTOR_HANDLE,4> tables{};
        // Per table copy, the four tables Describe hands out (the stage tables moved to that copy, C4-2), kept here because
        // GBufferDraw holds spans.
        std::array<std::array<D3D12_GPU_DESCRIPTOR_HANDLE,4>,3> copy_tables{};
        // 1063: the bound vertex streams (slot 0, and slot 1 for two-buffer meshes; I-0134: slot 2, a cloth Velo row's previous copy).
        std::array<D3D12_VERTEX_BUFFER_VIEW,3> streams{};
        UINT stream_count{1};
        uint32_t frame_mask{};
        // C4-1b: the CPU handles of the two stage tables, where Encode writes the bounded CBVs (VS b5, b6, b12; PS b4) of the
        // view's copies; a table copy is written once per numbered frame (its table belongs to the state until its holders are gone).
        std::array<D3D12_CPU_DESCRIPTOR_HANDLE,2> table_cpu{};Microsoft::WRL::ComPtr<ID3D12Device> device;UINT resource_stride{};
        // C4-2a: per table copy, the frame of the last encode of a draw with bounded CBVs (0 never; UINT64_MAX an unnumbered
        // encode). A copy's slots are rewritten per encode, so a later encode into it needs a later numbered frame whose uploads say
        // the last one is complete. Frame F uses copy F % table_copies (C4-2 with frame slots), unnumbered encodes copy 0.
        bool bounded{};uint32_t table_copies{1};UINT copy_stride{};   // copy_stride: descriptors between two copies
        bool global{};   // the tables are ranges of the global heaps (not per-draw tables)
        std::array<std::atomic<uint64_t>,3> bounded_frame{};
        std::array<const NativeModelConstantUploads*,3> bounded_uploads{};   // D-0180 C3: the uploads of the frame's first encode
        std::array<const OwnedNativeModelDraw*,3> bounded_draw{};   // I-0090: the draw of that encode (one draw of a state a frame)
        // 1121: pooled descriptor chunks this state suballocates from (returned when every holder is gone).
        std::vector<std::shared_ptr<void>> pooled;
        void CopyTables() noexcept {for(size_t copy=0;copy<copy_tables.size();++copy){auto& to=copy_tables[copy];to=tables;
            for(size_t stage=0;stage<2;++stage)to[stage].ptr+=UINT64(copy)*copy_stride*resource_stride;}}
    };
    std::shared_ptr<State> state_;
    bool reused_{};
};
// I-0090: with `base` (the row's previous draw), the new draw takes base's prepared state when every state input is the same
// (programme set and sampler bank, geometry, fixed-function state, targets, SRVs, samplers, table copies, and the constant
// bindings' stages, registers, kinds and payload sizes); otherwise, or without a base, it is prepared in full.
std::unique_ptr<OwnedNativeModelDraw> PrepareNativeModelDraw(ID3D12Device* device,NativeModelDrawInput input,const OwnedNativeModelDraw* base=nullptr);
// W0a (MODEL-DRAW-ENVELOPE-READY 3.2): whether a draw writes the expected target set: its envelope, as many targets as the
// envelope with the expected formats in order, and the expected depth format. The scene's lists and the tests share it.
bool EnvelopeMatches(const NativeModelDrawInput& input,ModelPassEnvelope expected,std::span<const DXGI_FORMAT> formats,
    DXGI_FORMAT depth_format) noexcept;
// D3D12 review phase A (R7 C3a): the distinct root shapes the validated draws need, one entry per
// {VS CBV, VS SRV, VS sampler, PS CBV, PS SRV, PS sampler} highest declared register + 1, in order of first use.
std::vector<std::array<uint16_t,6>> NativeModelRootShapes();
// D3D12 phase D cD0: global descriptor chunks in use at their highest since the previous call.
uint64_t TakeModelDrawGlobalChunkPeak() noexcept;
// C4-2: the highest used extent of the global heap since the previous call, in 8192-descriptor chunk equivalents.
uint64_t TakeModelDrawGlobalBumpPeak() noexcept;
// Phase C counters: pipelines created (R9), draws prepared on the global heaps (R6) and on per-draw tables.
struct ModelDrawCounters {uint64_t pso_created{},global_table_draws{},legacy_table_draws{};std::array<uint64_t,4> prepare_ticks{};};   // ticks: programs, validation, pipeline, tables
ModelDrawCounters NativeModelDrawCounters() noexcept;
// PREPARE-REDUCTION shadow (the unit tests' check of the caches; the module no longer sets it): every Nth draw with a program
// set also runs the former programme, pipeline and sampler-table paths and compares; the mismatches since start.
void SetModelDrawShadowPeriod(uint32_t period) noexcept;
uint64_t ModelDrawShadowMismatches() noexcept;
// Test hook (model_draw_test): every draw on the per-draw tables, the path of the draws that do not fit the global ones.
void SetModelDrawLegacyTables(bool legacy) noexcept;
}
