// W4 FW-1 (FFX-WAVE1-READY section 6): pass-through observers of the effect door, the effect update, the wind chain and
// the light collect. No behaviour change: every observed engine function runs unchanged after its entry is counted.
// Engine entries are reached through a signature-free thunk (one per entry): it saves the four integer argument registers
// and XMM0-XMM5, calls Record with the entry and the saved block, restores them and jumps to MinHook's trampoline, so no
// prototype is guessed (a float argument in XMM1-3 passes through untouched). Entries the native build already hooks
// (7CB820 replaced by the deferral, D07710 observed) and the native light collect report through Note instead.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace ds3native::ffx_observe {
enum Entry : uint32_t {
    SingleCreate,    // E6DCD0(SprjSfxImp, handle, &id, transform, flags): the creation of world-transform effects
    MatrixCreate,    // E6DA30(SprjSfxImp, handle, &id, matrix, flags)
    CutsceneCreate,  // E6DBC0(SprjSfxImp, handle, &id, transform, flags)
    WorldRequest,    // 7CB820(handle, id, transform, flags): natively the deferral, which reports through Note
    BlockRequest,    // 7CB8F0(handle, blockSfx, id, transform)
    MatrixRequest,   // 7CB750(handle, id, matrix, flags)
    GraphicsStep,    // D5B140 SprjGraphicsStep::STEP_Update: the frame of these rows
    SfxUpdate,       // E6E750(SprjSfxImp, tick): pending SfxIns started, then slot 8 when +0x58 is set
    ItemWind,        // D3E950: a grass item's wind node update and evaluation
    EntityUpdate,    // D07710(entity, context): natively observed already, reports through Note
    WindEval,        // 245F90(nodeHandle, forceObject)
    NodeUpdate,      // 25B870(node manager, ...): [mgr+0x30] live nodes
    RegistryAdd,     // 259F60(registry, &key, &desc): a force-field entry added
    BucketAppend,    // 2552C0(world, entry): the entry appended to the area's bucket
    LightCollect,    // native CollectNativePointLights (Note only)
    EntryCount};
struct Site {Entry entry;uint32_t rva;const char* name;};
inline constexpr std::array<Site,EntryCount> sites{{
    {SingleCreate,0xE6DCD0,"single_create"},{MatrixCreate,0xE6DA30,"matrix_create"},{CutsceneCreate,0xE6DBC0,"cutscene_create"},
    {WorldRequest,0x7CB820,"world_request"},{BlockRequest,0x7CB8F0,"block_request"},{MatrixRequest,0x7CB750,"matrix_request"},
    {GraphicsStep,0xD5B140,"graphics_step"},{SfxUpdate,0xE6E750,"sfx_update"},{ItemWind,0xD3E950,"item_wind"},
    {EntityUpdate,0xD07710,"entity_update"},{WindEval,0x245F90,"wind_eval"},{NodeUpdate,0x25B870,"node_update"},
    {RegistryAdd,0x259F60,"registry_add"},{BucketAppend,0x2552C0,"bucket_append"},{LightCollect,0,"light_collect"}}};

// A bounded key histogram: the first N distinct keys are counted, every later key adds to `other`.
template<size_t N> struct Histogram {
    std::array<uint64_t,N> keys{};std::array<uint64_t,N> counts{};size_t used{};uint64_t other{};
    void Add(uint64_t key) noexcept {
        for(size_t i=0;i<used;++i)if(keys[i]==key){++counts[i];return;}
        if(used<N){keys[used]=key;counts[used]=1;++used;return;}
        ++other;
    }
    void Clear() noexcept {used=0;other=0;}
};

// The block the thunk saves on its frame, in this order (offsets checked by the thunk test).
struct Saved {uint64_t rcx,rdx,r8,r9;alignas(16) std::array<std::array<uint8_t,16>,6> xmm;};
static_assert(offsetof(Saved,xmm)==0x20 && sizeof(Saved)==0x80);

using ReadFn=bool(*)(uintptr_t,void*,size_t) noexcept;
using LogFn=void(*)(const char*) noexcept;
// Starts the observer (rows name `mode`); without it every Record and Note returns at once.
void Configure(uintptr_t image,ReadFn read,LogFn log,const char* mode) noexcept;
// The thunk of an engine entry: MinHook writes the trampoline into *Original(entry) at MH_CreateHook. Null when the
// executable page could not be made.
void* Thunk(Entry entry) noexcept;
void** Original(Entry entry) noexcept;
// Entries the native build already hooks: the integer arguments as the engine passed them, and the caller.
void Note(Entry entry,uintptr_t return_address,uint64_t a0,uint64_t a1,uint64_t a2,uint64_t a3) noexcept;

// Test access (ffx_observe_test): the counts since the last row.
struct Window {uint64_t calls{};uint64_t system{};Histogram<12> callers;Histogram<24> ids;Histogram<6> flags;};
Window Snapshot(Entry entry) noexcept;
}
