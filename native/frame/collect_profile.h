#pragma once
// Workstream K1 (K-COLLECT-IMPLEMENTATION-SHEET section 1): where the native collect spends its time. Active only on
// measurement runs (measure.flag). Each thread attributes elapsed time to its current phase; a Scope switches to a
// phase and back, so nested work is counted once, as the inner phase's self time, and a Switch moves the current phase
// along a sequence of regions (the mesh loop). Totals are process-wide and read by the pacing window.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <array>
#include <atomic>
#include <cstdint>

namespace ds3native::kprofile {
enum Phase : uint8_t {
    SceneSetup,      // CollectNativeSelectedScene outside the regions below (images, samplers, rows, velocity inputs)
    Traversal,       // the engine world walk (C1FB0) outside the visitor callbacks         [C-1]
    EntityFilter,    // AdmitNativeSelectedObject self time                                 [C-2]
    ModelAdmission,  // AdmitNativeModelCandidate outside the mesh regions                  [C-3a]
    MeshLookups,     // per mesh: materials, layouts, streams, cloth, geometry binding      [C-3b]
    MeshPrograms,    // per mesh: AcquireNativeModelPrograms                                [C-3b']
    MeshTextures,    // per mesh: texture inputs (D24490 chain) and scene textures          [C-3c]
    MeshShaderTex,   // per mesh: SelectModelShaderTextures and the record checks           [C-3d]
    MeshMatDyn,      // per mesh: generated, damage, wet, phantom, dither, 22E930 blocks    [C-3e]
    MeshBlocks,      // per mesh: numeric parameters and the recipe x stage block loop      [C-3f]
    Passes,          // PrepareNativeSelectedPasses                                         [C-4]
    Lights,          // CollectNativeLightBlocks and CollectNativePointLights               [C-4]
    Offscreen,       // CollectNativeOffscreenScenes (menu views)
    Teardown,        // publication of the new scene, release of the previous one
    CollectCount,    // the collect phases end here (native_collect_split); the prepare phases follow (native_prepare_split)
    // PREPARE-REDUCTION cP0: PrepareNativeModelFrame of the main view (menu views are left out), self time.
    PrepRowSetup=CollectCount,   // per row: layout check, input copies, pose bindings, state set-up, witnesses
    PrepPaletteCheck,  // ValidateModelPaletteIndices                                       [PR-4]
    PrepForwardLight,  // the model's forward entries (built at collect since cP7) [PR-15]
    PrepStored,        // the collect's stored blocks copied (b3, b8, b4, b0)               [PR-11]
    PrepMatDyn,        // b2 cbMatDynParam: ranges, inactive parameters, pack
    PrepModelParams,   // b7 cbvModelParam (VS) and cbModelParam (PS)                       [PR-9]
    PrepPalettes,      // VS b5, b6, b12 pose palettes                                      [PR-10]
    PrepFrameBlocks,   // forward PS b8, b9 cbPerFrame, velocity b0, forward PS b5/b6       [PR-8]
    PrepOtherBlocks,   // b1 tiling, b11 scroll, cbTexHdrScaleParam, unmatched slots        [PR-22, PR-23]
    PrepSamplersSrvs,  // sampler and texture bindings of the row
    PrepDrawPrograms,  // OwnedNativeModelDraw::Prepare: programme lookup                   [PR-3]
    PrepDrawValidate,  // bindings, geometry, input elements, SRV devices                   [PR-12]
    PrepDrawPipeline,  // pipeline key and lookup (or creation)                             [PR-6]
    PrepDrawSamplers,  // sampler sets and their table lookup                               [PR-13]
    PrepDrawConstants, // since C4-1b: the global descriptor chunk choice (no constant copy) [PR-14]
    PrepDrawSrvs,      // SRV descriptor copies                                             [PR-14]
    PrepFrameTail,     // after the rows: summaries, velocity, witnesses (lighting at collect since cP7) [PR-16]
    PrepKept,          // C4-2: the kept-draw lookup of a row and, on a hit, its frame registers
    Count,
    None=Count       // outside any measured region: not accumulated
};
// ProgramsBuilt: NativeModelLayout::Programs objects built (AcquireNativeModelPrograms misses, K P1 plans included).
// SharedBlocksBuilt: scene, view and instance blocks packed (P2 misses); NumericBlockMisses: meshes whose numeric blocks
// were not found in their programme's P3 cache.
// ShaderTextureMisses: meshes whose shader texture selections were not found in their programme's P5 cache.
// TextureLookups: genuine D24490 lookup chains run (P4b misses); TextureFallbacksKept: later-candidate or absent choices kept (P4c). SceneTablesCarried: main collects that took the previous scene and view tables; InstanceTablesCarried: models that took their previous instance table (C7-1c).
// RecipeStagePairs (since C7-2): pairs visited by the numeric block loop and by the programme table loop.
// C7-2 model records (main view): models reused from their record, records built, records found invalid (inputs or epochs
// changed), models not recordable (volatile), revalidations (fresh admission of a valid record) and their mismatches.
// Prepare (cP0, main view): rows, draws prepared, constant blocks and bytes bound per draw (since C4-1 without the frame registers, bound once per view), SRVs bound, palette checks run and in range.
// C4-2 PD-0: the dry run's four slots, kept for the row's positions (the dry run was removed by I-0102; they stay at zero).
// C4-2 (kept draws, main view): rows drawn with their kept draw, rows without one, rows whose model version or own inputs
// (tiling, instance blocks) changed, the former prepare-shadow slot (zero since I-0102), and hits whose frame register was refused.
enum Counter : uint8_t {RecipeStagePairs,AdmittedMeshes,AdmittedModels,ProgramsBuilt,SharedBlocksBuilt,NumericBlockMisses,ShaderTextureMisses,TextureLookups,TextureFallbacksKept,SceneTablesCarried,InstanceTablesCarried,
    ModelRecordsReused,ModelRecordsBuilt,ModelRecordsInvalid,ModelRecordsVolatile,ModelRecordsRevalidated,ModelRecordMismatches,
    PrepRows,PrepDraws,PrepConstantBlocks,PrepConstantBytes,PrepSrvs,PrepPaletteChecks,PrepPaletteInRange,
    PrepDryRows,PrepDryIdentityHits,PrepDryContentHits,PrepDryHitDrawUs,
    PrepKeptHits,PrepKeptNew,PrepKeptModel,PrepKeptRow,PrepKeptShadow,PrepKeptFrameRefusals,CounterCount};
struct Totals {
    std::array<std::atomic<uint64_t>,Count> ticks{},calls{};
    std::array<std::atomic<uint64_t>,CounterCount> counters{};
};
inline std::atomic<bool> enabled{};
inline Totals totals;
struct ThreadState {Phase current{None};int64_t since{};};
inline thread_local ThreadState state;
// I-0097 tranche 2 (crossing-observe.flag): the same ticks per thread, so one admission's split can be read (a difference).
inline thread_local std::array<int64_t,Count> thread_ticks{};
inline int64_t Now() noexcept {LARGE_INTEGER t{};QueryPerformanceCounter(&t);return t.QuadPart;}
inline void Switch(Phase phase) noexcept {
    if(!enabled.load(std::memory_order_relaxed))return;
    const auto now=Now();
    if(state.current!=None){totals.ticks[state.current].fetch_add(uint64_t(now-state.since),std::memory_order_relaxed);thread_ticks[state.current]+=now-state.since;}
    if(phase!=None)totals.calls[phase].fetch_add(1,std::memory_order_relaxed);
    state.current=phase;state.since=now;
}
// I-0097 tranche 2: the current phase's time so far added (totals and thread_ticks), no call counted (crossing-observe.flag's
// snapshots around one admission).
inline void Flush() noexcept {
    if(!enabled.load(std::memory_order_relaxed) || state.current==None)return;
    const auto now=Now();totals.ticks[state.current].fetch_add(uint64_t(now-state.since),std::memory_order_relaxed);
    thread_ticks[state.current]+=now-state.since;state.since=now;
}
struct Scope {
    Phase previous{None};bool on{};
    explicit Scope(Phase phase) noexcept:previous(state.current),on(enabled.load(std::memory_order_relaxed)){if(on)Switch(phase);}
    ~Scope(){if(on){const auto calls_before=previous;Switch(previous);if(calls_before!=None)totals.calls[calls_before].fetch_sub(1,std::memory_order_relaxed);}}
    Scope(const Scope&)=delete;Scope& operator=(const Scope&)=delete;
};
inline void Add(Counter counter,uint64_t n=1) noexcept {if(enabled.load(std::memory_order_relaxed))totals.counters[counter].fetch_add(n,std::memory_order_relaxed);}
// C-0b (sheet 1.2): the frame outside the three phases. Each stamp adds the time since the previous stamp, whatever its
// kind, to the bucket of this stamp's kind, so a window's buckets sum to its wall time; with the owner thread's order
// (FrameBegin, UpdateEntry, UpdateExit, RenderStepEntry, UiPrepareEntry, UiPrepareExit, RenderStepExit, EndFrameEntry,
// ClearEntry, ClearExit) each bucket is the span that ends at its stamp.
enum Stamp : uint8_t {FrameBegin,UpdateEntry,UpdateExit,RenderStepEntry,UiPrepareEntry,UiPrepareExit,RenderStepExit,
    EndFrameEntry,ClearEntry,ClearExit,StampCount};
inline std::array<std::atomic<uint64_t>,StampCount> stamp_ticks{},stamp_calls{};
inline std::atomic<int64_t> last_stamp{};
inline void Mark(Stamp stamp) noexcept {
    if(!enabled.load(std::memory_order_relaxed))return;
    const auto now=Now();const auto last=last_stamp.exchange(now,std::memory_order_relaxed);
    if(last)stamp_ticks[stamp].fetch_add(uint64_t(now-last),std::memory_order_relaxed);
    stamp_calls[stamp].fetch_add(1,std::memory_order_relaxed);
}
struct MarkOnExit {Stamp stamp;~MarkOnExit(){Mark(stamp);}};
// Switch only inside a measured region (the prepare of a menu view runs under a None scope and is not attributed).
inline void SwitchIfActive(Phase phase) noexcept {if(enabled.load(std::memory_order_relaxed) && state.current!=None)Switch(phase);}
inline void AddIfActive(Counter counter,uint64_t n=1) noexcept {if(state.current!=None)Add(counter,n);}
// C-5 (sheet 1.2): how many admitted models keep the same per-frame inputs from one collect to the next (the reuse a
// persistent admission record could reach). Signatures per entity, entries unseen for 120 collects are forgotten.
// C7-M (PERSISTENT-RECORDS-DESIGN 1): for the models seen in the previous collect, whether their mesh set (every mesh's
// admission outputs except the scene, view and instance blocks: material, layout, programmes, geometry, numeric writes,
// dynamic bytes, textures, shader textures, numeric blocks) and their instance records changed: the reuse a persistent
// collect record could reach.
enum Reuse : uint8_t {ModelsUnchanged,ModelsChanged,ModelsNew,MeshSetsUnchanged,MeshSetsChanged,InstancesChanged,ReuseCount};
inline std::array<std::atomic<uint64_t>,ReuseCount> reuse{};
}
