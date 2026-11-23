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
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ds3native {
// FFX-DRAW N6, I-0104 (the native effect base, DESIGN §2): the FFX draws of a frame, made on the engine thread by the native effect
// context (frame/effect_draw: the original's draw side, 299E40, slot 27 and the builders, draws through it), encoded here in D3D12.
// Everything a draw needs is its own: the programmes (the original's DXBC), the input layout (the image's GX element table), the GX
// common state bank's entries (blend, depth, raster, sampler, by bank offset: their descriptors are the original's, Reference1956),
// the constant blocks slot 27 uploaded (VS b0, DS b0, PS b0, b1, b6) and the vertex and index bytes the builder wrote into the ring,
// all as ranges of the frame's one arena, the textures t0..t4.
struct NativeEffectElement {std::string semantic;uint32_t index{};DXGI_FORMAT format{};uint32_t offset{};};
struct NativeEffectLayout {uint64_t key{};std::vector<NativeEffectElement> elements;};   // key: the element table's address
struct NativeEffectProgram {uint64_t key{};std::shared_ptr<const std::vector<uint8_t>> bytes;};   // key: the engine's shader object
// t0..t4: an admitted texture (its CPU view, the frame keeps its record), the scene depth or the scene-colour copy (the renderer's
// own views; natively the original's [GXRS+0x4E10]+0x48 and ctx+0x990 are absent), or nothing (a null view).
enum class NativeEffectTextureKind : uint8_t {None,Texture,Depth,SceneCopy,Object,Target};   // Object: an effect record's view (E07; key in `object`)
// Target (I-0140 tranche 10): one of the stage's two half-size effect targets (`object` 1 or 2), the light shafts' (effect_draw.h).
struct NativeEffectTexture {NativeEffectTextureKind kind{};D3D12_CPU_DESCRIPTOR_HANDLE view{};uint64_t object{};};
enum NativeEffectConstant : uint32_t {EffectVsB0,EffectDsB0,EffectPsB0,EffectPsB1,EffectPsB6,EffectGsB9,EffectPsB9,EffectPsB11,EffectCsB0,EffectCsB1,
    EffectCsB5,EffectConstantSlots};
// A constant block in the frame's arena: 256-aligned, padded to 256 (the arena's first effect_arena_zero bytes are zero: a block
// never written, or a slot with nothing bound, is {0, 0}; frame/effect_draw's zero_bytes).
struct NativeEffectRange {uint32_t offset{},size{};};
constexpr uint32_t effect_arena_zero=0x1000;
// E07: the GPU-particle statements of the stream, in order with the draws: a compute dispatch (direct or indirect), a buffer copy, a
// UAV clear and the particles' indirect draw. Their buffers and views are the effect device's native records (keys: the records'
// creation serials), described in NativeEffectFrame::objects and made persistent D3D12 resources by the stage.
// CopyRect (I-0113 tranche 23): the draw context's copy slot 140260440, before each distortion quad: the quad's screen rectangle of
// the target copied into the scene copy at the same place (copy_kind 0: copy_rect, left, top, right, bottom in [0, 1]; 1: the whole
// target; 2: from (0, 0) to copy_size, pixels; 0 and 2 widened by 16 pixels and clamped), and the draw context's slot 20 (262A70)
// for the trails (the whole target).
// ClearTarget, CopyTarget (I-0140 tranche 10): a half-size effect target cleared (target, clear_colour) or copied whole into the other
// (target the destination, copy_source the source).
enum class NativeEffectOp : uint8_t {Draw,Dispatch,DispatchIndirect,Copy,ClearUav,DrawIndirect,CopyRect,ClearTarget,CopyTarget};
struct NativeEffectObject {
    uint64_t key{},parent{};uint32_t kind{};   // 1 buffer, 2 shader view, 3 unordered view (parent: its buffer)
    std::array<uint32_t,8> description{};      // a buffer: {bytes, usage, bind, CPU access, misc, stride}; a view: {format, dimension, first, count, ...}
    std::shared_ptr<const std::vector<uint8_t>> initial;   // a buffer's creation bytes (empty: zero)
};
struct NativeEffectDraw {
    NativeEffectOp op{NativeEffectOp::Draw};
    int32_t pass{};                 // the engine pass (0x29, 0x2D, 0x31)
    uint32_t key{};                 // I-0113 tranche 15: its command's order key (BF210, as the pass context recorded it)
    uint32_t topology{};            // D3D_PRIMITIVE_TOPOLOGY (the GX value converted as 1929BA0)
    NativeEffectProgram vs,hs,ds,gs,ps,cs;
    std::shared_ptr<const NativeEffectLayout> layout;
    uint32_t blend{},depth{},raster{},sampler{};   // GX common state bank offsets
    uint32_t targets{1};            // 2: the pixel programme writes SV_Target1 (the glow twins: GB5)
    std::array<NativeEffectRange,EffectConstantSlots> constants;
    std::array<NativeEffectTexture,5> textures;
    uint32_t vertex_offset{},stride{},index_offset{};DXGI_FORMAT index_format{DXGI_FORMAT_UNKNOWN};
    bool indexed{};uint32_t count{},start{};int32_t base{};
    // E07 (object keys, 0: none).
    std::array<NativeEffectTexture,5> cs_resources{};std::array<uint64_t,3> cs_uavs{};std::array<NativeEffectTexture,2> vs_resources{};
    uint32_t groups[3]{};uint64_t args{},destination{},source{},clear_view{};std::array<uint32_t,4> clear_values{};
    std::array<uint32_t,3> cs_uav_counts{};uint8_t cs_uav_counts_set{};   // u0..u2 initial counts (bit set: written into the view's counter before the dispatch)
    std::array<float,4> copy_rect{};uint8_t copy_kind{};std::array<int32_t,2> copy_size{};   // CopyRect
    // I-0140 tranche 10: the draw's target (0 the pass's own, 1 or 2 a half-size effect target), the viewport the context was given
    // (x, y, width, height, min and max depth; used for a half-size target, the pass's own keeping the stage's) and its scissor
    // rectangle (left, top, right, bottom in the target's pixels; applied when the draw's raster entry enables the scissor).
    uint8_t target{},copy_source{};bool viewport_set{},scissor_set{};std::array<float,6> viewport{};std::array<int32_t,4> scissor{};std::array<float,4> clear_colour{};
};
// I-0127 (effect-measure.flag): one frame's recording by the stage (`recorded`: Record ran for it). `calls` counts the list calls
// the classic draws make by kind; `repeats` those not made because the list already holds that state from the stage's previous
// call since its last bind (the 0x29 rows and the GPU-particle statements leave it unknown), and the views of a texture table
// already written this frame; `ticks` the stage's render-thread time (QueryPerformanceCounter) without the callers' rows and
// scene copy (before_pass, before_draw), `callback_ticks` theirs.
enum EffectCall : uint32_t {EffectCallHeaps,EffectCallPipeline,EffectCallTopology,EffectCallTargets,EffectCallTextureTable,EffectCallSamplerTable,
    EffectCallConstants,EffectCallVertices,EffectCallIndices,EffectCallViews,EffectCallDraw,EffectCallKinds};
struct EffectFrameMeasure {bool recorded{};uint64_t ticks{},callback_ticks{},arena_bytes{};uint32_t draws{},statements{},pipeline_changes{};
    std::array<uint32_t,EffectCallKinds> calls{},repeats{};};
struct NativeEffectFrame {
    bool witness{};                 // ffx-record.flag: the E07 readback witnesses run
    bool measure{};                 // I-0127 effect-measure.flag: the stage's calls, filtered calls and times (EffectFrameMeasure)
    bool scan{};                    // I-0113 tranche 29 effect-scan.flag: the scene copy and the lit HDR scanned for NaN, inf and negatives (EffectScanRow)
    bool scan_draws{};              // I-0113 tranche 30 effect-scan-draws.flag: the lit HDR's negatives counted on the GPU after each 0x29 and (tranche 31) 0x2D statement (EffectDrawScanRow)
    // FFX-DRAW (the distortion rectangle, Native2027): the passes after 0x29 sample a scene copy that holds the 0x29 effects, so the copy
    // is remade before the first of them that samples it (no-ffx-effect-copy.flag: the forward pass's copy only, A/B witness).
    bool effect_copy{true};
    // I-0113 tranche 17 (STUDY-PASS-31): pass 0x31 drawn into a half-size layer composited into the scene, as the original
    // (no-effect-layer.flag: at full size into the scene, A/B witness).
    bool effect_layer{true};
    // I-0113 tranche 17 (STUDY-T17-LAYER-ALPHA §5b): the layer composited by the bilateral SampleBilateralDepthBlur, as the
    // original when its Effects quality row sets « Force HQ UpScale » (Effects MAX), else by the plain SampleColor1.
    bool layer_bilateral{};
    std::vector<uint8_t> arena;     // the ring bytes and constant blocks the draws address (its first effect_arena_zero bytes zero)
    std::vector<NativeEffectDraw> draws;
    std::vector<std::shared_ptr<const void>> owners;   // the texture records the views belong to, kept for the frame
    std::vector<NativeEffectObject> objects;   // E07: every record the statements reach (views with their buffers)
    std::vector<uint64_t> released;            // E07: keys released since the previous frame (their resources retire)
    std::array<uint32_t,2> target_size{};      // I-0140 tranche 10: the half-size effect targets' width and height (R8G8B8A8_UNORM)
    // I-0120 D1: set when the stage takes the releases and creation bytes (Objects, Retain); a frame dropped before that (its
    // scene not prepared, a render-extent change, the DLSS latch, Encode stopped first) gives them back to the next frame.
    mutable std::atomic<bool> consumed{};
};
// The GX common state bank's blend and depth entries by bank offset (I-0105: an FFX model particle draws with the effect context's
// states, as the original's 1A86C0 binds none of its own). False for an offset outside the known entries.
bool NativeEffectBankBlend(uint32_t offset,D3D12_BLEND_DESC& out);
bool NativeEffectBankDepth(uint32_t offset,D3D12_DEPTH_STENCIL_DESC& out);
// The encoder: one root signature (VS b0, DS b0, PS b0/b1/b6 root views, PS t0..t4 table, PS s9 table, s1 static), PSOs cached by
// the draw's programmes, layout, bank entries, topology type and target count, a shader-visible view ring (one region per frame
// slot) and the bank's samplers. Render thread only.
class EffectStage {
public:
    struct Targets {
        // dsv: the scene depth's writable view (I-0113 tranche 15: the 0x29 rows drawn between the effects write it; the effects'
        // pipelines keep a zero write mask; the programmes that sample the depth read `depth`, a copy).
        D3D12_CPU_DESCRIPTOR_HANDLE hdr{},gb5{},dsv{};
        DXGI_FORMAT hdr_format{},gb5_format{},depth_format{};
        ID3D12Resource* depth{};ID3D12Resource* scene_copy{};   // the depth and copy tokens' resources (the stage makes their views; null: none)
        uint64_t target_generation{};   // changes when the depth or the copy is recreated (the views are remade)
        // Called before the first statement of each pass that has one (the effect copy), and before each statement with its pass
        // and key (I-0113 tranche 15: the forward rows keyed before it); each returns whether it recorded draws of its own (the
        // stage then binds its state again).
        std::function<bool(int32_t)> before_pass;
        std::function<bool(int32_t,uint32_t)> before_draw;
        // I-0113 tranche 30 (effect-scan-draws.flag): called after each drawing statement (a classic draw or a GPU-particle
        // statement), returning whether it recorded work of its own (the stage then binds its state again).
        std::function<bool(const NativeEffectDraw&)> after_draw;
        // A CopyRect statement outside the half-size layer (I-0113 tranche 23): its rectangle copied from the target into the scene copy.
        std::function<void(const NativeEffectDraw&)> copy_rect;
        DXGI_FORMAT depth_view_format{},scene_copy_format{};
        // I-0113 tranche 17 (STUDY-PASS-31): pass 0x31's half-size layer, when given: its one target (hdr), the half depth's
        // read-only view (dsv) and resource (depth, t3), its formats, viewport and scissor; its hooks and copy are the main
        // targets'. A 0x31 draw with more than one target draws its first there (the original binds one RTV).
        const Targets* layer{};D3D12_VIEWPORT viewport{};D3D12_RECT scissor{};
    };
    struct Stats {uint64_t draws{},skipped{},pipelines{},pipeline_failures{},textures{},missing_textures{},other_passes{},gpu_statements{},dispatches{},indirect_dispatches{},copies{},clears{},particle_draws{},gpu_skipped{},counter_writes{},row_runs{};std::array<uint32_t,8> args{},counters{};std::array<float,88> particles{};std::string last_failure;uint64_t copy_rects{},target_draws{},target_clears{},target_copies{};EffectFrameMeasure measure;};
    // Records the frame's draws of the given engine passes, in order; `allocate(bytes, zero)` hands out upload memory (256-aligned;
    // `zero` false: the caller writes every byte).
    template<class Allocate> void Encode(ID3D12Device* device,ID3D12GraphicsCommandList* list,const NativeEffectFrame& frame,
        const std::vector<int32_t>& passes,const Targets& targets,const D3D12_VIEWPORT& viewport,const D3D12_RECT& scissor,uint32_t slot,Allocate&& allocate) {
        stats_.measure=EffectFrameMeasure{};   // step review L3: a frame Prepare stops is not taken for the previous one
        Prepare(device);
        struct Sink : UploadSink {Allocate& a;explicit Sink(Allocate& f):a(f){}
            Memory Get(uint64_t bytes,bool zero) override {return a(bytes,zero);}} sink{allocate};
        Record(device,list,frame,passes,targets,viewport,scissor,slot,sink);
    }
    const Stats& Statistics() const noexcept {return stats_;}
    void Close() noexcept;                    // the device's objects released (the owner's close; a reopen prepares again)
    // E07 (review): a frame the owner does not encode keeps its released records and its buffers' creation bytes for the next one.
    void Retain(const NativeEffectFrame& frame);
    void QuarantineToProcessExit() noexcept;  // unproved completion: kept to process exit
    struct UploadSink {struct Memory{uint8_t* cpu{};D3D12_GPU_VIRTUAL_ADDRESS gpu{};ID3D12Resource* resource{};uint64_t offset{};};virtual Memory Get(uint64_t bytes,bool zero=true)=0;virtual ~UploadSink()=default;};
    // E07: the resources of released records, to be held by the caller's frame slot until its work completes.
    std::vector<std::shared_ptr<const void>> TakeRetired() {std::vector<std::shared_ptr<const void>> out;out.swap(retired_);return out;}
private:
    void Prepare(ID3D12Device* device);
    void Record(ID3D12Device* device,ID3D12GraphicsCommandList* list,const NativeEffectFrame& frame,const std::vector<int32_t>& passes,
        const Targets& targets,const D3D12_VIEWPORT& viewport,const D3D12_RECT& scissor,uint32_t slot,UploadSink& uploads);
    ID3D12PipelineState* Pipeline(ID3D12Device* device,const NativeEffectDraw& draw,const Targets& targets);
    // E07: the GPU-particle side (EFFECTS-E07 §5): persistent buffers per effect record, compute and particle root signatures,
    // indirect command signatures, the statements of the stream.
    struct GpuBuffer {Microsoft::WRL::ComPtr<ID3D12Resource> resource;D3D12_RESOURCE_STATES state{D3D12_RESOURCE_STATE_COMMON};uint64_t bytes{};};
    std::unordered_map<uint64_t,GpuBuffer> buffers_;std::unordered_map<uint64_t,NativeEffectObject> objects_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> compute_root_,particle_root_;
    Microsoft::WRL::ComPtr<ID3D12CommandSignature> dispatch_signature_,draw_signature_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> clear_views_;uint32_t clear_next_{};
    std::vector<std::shared_ptr<const void>> retired_;
    // E07 witness: the first particle draw's arguments and the dispatch arguments of the frame, copied into a readback ring of 8 (read 8
    // frames later, past the frames in flight).
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,8> readbacks_{};uint64_t readback_frames_{};
    void PrepareGpu(ID3D12Device* device);
    void Objects(ID3D12Device* device,ID3D12GraphicsCommandList* list,const NativeEffectFrame& frame,UploadSink& uploads);
    GpuBuffer* BufferOf(uint64_t view_or_buffer) noexcept;
    void Require(ID3D12GraphicsCommandList* list,GpuBuffer& buffer,D3D12_RESOURCE_STATES state);
    bool View(ID3D12Device* device,uint64_t key,D3D12_CPU_DESCRIPTOR_HANDLE at,bool unordered,ID3D12GraphicsCommandList* counter_list=nullptr,UploadSink* uploads=nullptr);
    // The hidden counter of a structured UAV (the particle programmes append and consume through views made without the D3D11 append
    // flag, Reference1987: the driver's hidden counter, INFERRED kept per view; a bind's initial count other than -1 is written into it before
    // the dispatch, as D3D11's CSSetUnorderedAccessViews does): one 4-byte slot
    // per view key in pooled buffers (64 slots of 4096 bytes, the D3D12 counter placement), zero when assigned, freed on release.
    struct CounterSlot {ID3D12Resource* resource{};uint64_t offset{};};
    CounterSlot Counter(ID3D12Device* device,ID3D12GraphicsCommandList* list,uint64_t view,UploadSink& uploads);
    std::vector<GpuBuffer> counter_pools_;std::unordered_map<uint64_t,uint32_t> counter_slots_;std::vector<uint32_t> free_counters_;uint32_t counter_next_{};
    std::vector<uint64_t> pending_released_;   // releases of frames not encoded (Retain)
    std::unordered_map<uint64_t,std::shared_ptr<const std::vector<uint8_t>>> initials_;   // buffers' creation bytes, until released
    ID3D12PipelineState* ComputePipeline(ID3D12Device* device,const NativeEffectDraw& draw);
    ID3D12PipelineState* ParticlePipeline(ID3D12Device* device,const NativeEffectDraw& draw,const Targets& targets);
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> views_,samplers_,cpu_views_;   // cpu_views_: the depth copy, scene-copy and 0x31 layer's half-depth views
    ID3D12Resource* viewed_depth_{};ID3D12Resource* viewed_copy_{};ID3D12Resource* viewed_layer_depth_{};uint64_t viewed_generation_{UINT64_MAX};bool prepared_{};
    uint32_t view_stride_{},sampler_stride_{};
    Stats stats_;
    // I-0140 tranche 10: the two half-size effect targets (the original's scene control +0x238 and +0x240, made at view attach), made
    // at the frame's size when a frame uses them and remade when it changes (the old ones retired through TakeRetired); their
    // render-target views (target_rtvs_) and shader views (cpu_views_ 3 and 4). They rest in PIXEL_SHADER_RESOURCE between frames.
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,2> effect_targets_{};std::array<D3D12_RESOURCE_STATES,2> effect_target_states_{};
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> target_rtvs_;uint32_t rtv_stride_{};std::array<uint32_t,2> effect_target_size_{};
    bool EffectTargets(ID3D12Device* device,const std::array<uint32_t,2>& size);
    void EffectTargetState(ID3D12GraphicsCommandList* list,uint32_t index,D3D12_RESOURCE_STATES state);
    using TextureTuple=std::array<std::pair<uint32_t,uint64_t>,5>;   // I-0127: a draw's t0..t4 by kind and source
    struct TextureTupleHash {size_t operator()(const TextureTuple& tuple) const noexcept {uint64_t h=0xCBF29CE484222325ull;
        for(const auto& [kind,source]:tuple){h=(h^kind)*0x100000001B3ull;h=(h^source)*0x100000001B3ull;h^=h>>29;}return static_cast<size_t>(h);}};
    std::unordered_map<TextureTuple,uint32_t,TextureTupleHash> tables_;   // this frame's texture tables (the tuple, its first view in the ring)
public:
    // I-0104: a pipeline's identity (kind 0 the classic draws, 1 the particles, 2 compute; I-0140: 3 and 4 the classic draws and particles of pass 0x21, which keep
    // the bank's depth write): the programmes and layout by key, the bank
    // entries, the topology type, the target count and formats; hashed, never formatted.
    struct PipelineKey {
        uint64_t vs{},hs{},ds{},gs{},ps{},cs{},layout{};uint32_t blend{},depth{},raster{},topology_type{},targets{},kind{};
        DXGI_FORMAT hdr{},gb5{},depth_format{};
        bool operator==(const PipelineKey&) const = default;};
    struct PipelineKeyHash {size_t operator()(const PipelineKey& key) const noexcept;};
private:
    std::unordered_map<PipelineKey,Microsoft::WRL::ComPtr<ID3D12PipelineState>,PipelineKeyHash> pipelines_;
    std::unordered_map<PipelineKey,std::string,PipelineKeyHash> failed_;
public:
    static constexpr uint32_t views_per_slot=4096;   // 5 views a draw: 819 draws a frame slot
};
}
