// W4 FW-2/FW-3 (FFX-WAVE1-READY section 2, EFFECTS-PHASE1-NATIVE sections 2-7): the engine's GX effect system without its
// GX statements. Every CPU statement of SprjSfxImp's constructor E6C9D0 that the native manager left out, of the scene
// control constructor 24B260 and of the device initialisation 24C530 runs here in the original's order; each statement
// that would create or read a GX object is replaced: the display object by a pointer into a reserved no-access page (the
// device token), so that a missed transcription faults at a named address instead of reading garbage; GX buffers created on
// that device by native records (CreateBuffer, reached through the native hook of FUN_1419254F0); the GPU ring and the
// shader set is built on that device with native programme records (CreateProgram); the GPU ring stays null (draw-side only).
#pragma once
#include "../renderer/work_pool.h"
#include <array>
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace ds3native::effect_system {
struct Engine {
    uintptr_t image{};
    bool (*read)(uintptr_t,void*,size_t) noexcept{};
    void (*log)(const char*) noexcept{};
    bool run_cpu_items{};   // FFX-DRAW: execute the update list's CPU items (the particle simulation 1ADC100, the messages 141ADC190), see Update
    bool program_tokens{};  // FFX-DRAW N6: Build opens an effect_draw::ProgramScope around the shader set 291520
    // I-0092: the pool the node refills and the particle items run on (DS3's ranged GX jobs of 10); null: on the calling thread.
    ds3native::WorkPool* pool{};
    bool count_kinds{};     // I-0092: count the work lists' items by kind this update (ItemKinds, the logged update only)
};
// The objects FW-2 builds; `published` becomes true at FW-3 when SprjSfxImp+0x58 is stored.
struct System {
    uintptr_t manager{};        // SprjSfxImp (native_sfx_manager)
    uintptr_t scene_control{};  // GXFfxSceneCtrl, 0x6C0
    uintptr_t ffx_manager{};    // GXFfxManager, 0x28 (sc+0x18)
    uintptr_t entity{};         // scene entity, 0x80 (sc+0x08)
    uintptr_t resources{};      // resource manager, 0x198 (sc+0x28)
    uintptr_t token{};          // device token: reserved, never committed
    bool published{};
};
// The device token's reserved range: an access anywhere inside faults (FFX-WAVE1-READY 2.2 item 1). GX objects the effect
// system creates on that device are native records behind handles from a second reserved range (64 bytes apart), so a
// GX use of one (a draw-side read, a release) faults at a named handle too.
constexpr size_t token_bytes=0x10000,handle_bytes=0x400000,handle_stride=0x40;
bool IsToken(uintptr_t address,const System& system) noexcept;
bool IsEffectDevice(uintptr_t device) noexcept;
// FUN_1419254F0 (GX buffer creation: device, out handle, D3D11-style buffer description {ByteWidth, Usage, BindFlags,
// CPUAccessFlags, MiscFlags, StructureByteStride}, optional initial data {pointer, pitch, slice}) on the effect device:
// the same description check as the original (8 when refused), then a native record keeping the description and the
// initial bytes (the future D3D12 resource of the effect draw families), and a handle written to *out; 0 on success.
uint32_t CreateBuffer(uintptr_t out,const uint32_t* description,uintptr_t initial) noexcept;
struct BufferStats {uint64_t buffers,bytes,released;};
// A GX release (FUN_1400D8780 deferred, FUN_14019AD90 direct) of a native effect object: the record is dropped; false when
// the handle is not a live effect object (the original release then runs).
bool Release(uintptr_t handle) noexcept;
BufferStats Buffers() noexcept;
// FUN_141925BF0 (shader view, kind 2) and FUN_141925B10 (unordered view, kind 3) on a native effect object: a native
// view record over that object (its description's first 32 bytes kept), its handle written to *out; 0 on success.
bool IsEffectObject(uintptr_t handle) noexcept;
// FFX-DRAW: a live effect object's kind (1 buffer, 2 shader view, 3 unordered view, 4 programme) and description (a buffer's
// ByteWidth first, a programme's bytecode size first); false when the handle names no live object.
bool Describe(uintptr_t handle,uint32_t& kind,std::array<uint32_t,8>& description) noexcept;
// FFX-DRAW N6: a live effect buffer's initial bytes (CreateBuffer's data; empty when created without); false when not a buffer.
bool BufferBytes(uintptr_t handle,std::vector<uint8_t>& out);
// The view a live effect view record names (its buffer), 0 when not a view.
uintptr_t ViewParent(uintptr_t handle) noexcept;
// A live effect object's creation serial (unique for the process; 0 when not live): the renderer's key for its D3D12 resource.
uint64_t Serial(uintptr_t handle) noexcept;
// FUN_141925210 / FUN_1419250A0 (shader programme creation from bytecode) on the effect device: a native programme record (kind 4)
// keeping the bytecode, its handle written to *out; 0 on success.
uint32_t CreateProgram(uintptr_t out,uintptr_t bytes,uint64_t size) noexcept;
uint32_t CreateView(uint32_t kind,uintptr_t out,uintptr_t resource,const uint32_t* description) noexcept;
// FW-2b (D-0173): the effect GX module reads the GX display object [144961AB0] (null natively) at the 7-byte sites of
// ffx_display_sites.h. Their RIP-relative displacements are rewritten to read a native slot instead, which holds the
// effect display: one read-only page, zero (the GX feature absent, as natively) but for the fields the module's GX
// statements need: +0x000 the device (the device token), +0x060 the context array whose first context's +8 is a token (the
// main context, stored by the update and proceed passes), +0x2B8 the deferred-release queue (a token address). A read of an
// unlisted field returns zero; a dereference of it faults at a small address named by the exception row's RIP. The rest of
// the engine keeps the null display.
// All sites are checked against their original bytes and target first; nothing is written unless every one matches.
// Must run before MinHook copies any prologue (Setup, before the hooks are created).
struct Redirect {int sites{};uintptr_t slot{},display{};const char* refused{};};
Redirect RedirectDisplay(uintptr_t image,bool draw_side=false) noexcept;   // draw_side: also +0x270/+0x2A0 (FFX-DRAW N6)
uintptr_t Display() noexcept;
// FW-2: build without publishing. Throws std::runtime_error naming the refused step. `manager` is the native SprjSfxImp.
System Build(const Engine& engine,uintptr_t manager);
// T-F1 at run time: the construction invariants of FFX-WAVE1-READY section 7, one row `native_effect_system_built`.
bool Check(const Engine& engine,const System& system);
// FW-3: SceneCtrl slot 8 (FUN_14024D000), the single-threaded branch transcribed (EFFECTS-PHASE1 section 5 with
// FFX-WAVE1-READY 2.3): the environment factors, the resource tick, the hide-mask swap, the sound, the time scale, the
// registry controllers, the node-manager update (the job body FUN_14025B1F0 per node: I-0092, on Engine::pool by DS3's ranges),
// the update and proceed passes (FUN_140296E20/296690 called; their main context comes from the effect display) combined by
// FUN_140297110, the library update through the manager's slot 3, the two work lists (below), the passes destroyed, then the
// light container FUN_140266DE0.
// The work lists (FFX-DRAW §2, Reference1950): the original hands them to GX jobs, the proceed list to GXFfxUpdateRootJob slot 1
// (296D80: per bucket of [proceed+0x98], item vt+8(item, proceed+0x2300, data, 0)) and the update list to GXFfxUpdateJob slot 1
// (296FF0: per 16-byte {item, data} of [update+0x2D30], item vt+8(item, update+0x2270, data, 0)). The update list is not only
// draw-side: its items whose vt+8 is 1ADC100 are the CPU particle simulation of the effect nodes (1ADD510: emission, ageing,
// the live count at node+0x70 the draw needs), and those whose vt+8 is 141ADC190 deliver the messages queued to effects during the
// update (I-0113 tranche 12: an FFX model animator's bone matrices, message 1000; run after the particle items, in list order on
// this thread). With run_cpu_items those items run here, the particle items (I-0092) on Engine::pool by tickets of
// DS3's range of 10 (its ranged GXFfxUpdateJob; the item reads its own node, its random generator is per thread), joined before
// the messages and before the passes are destroyed. Every other item is counted by its vt+8 (ItemKinds, when Engine::count_kinds) and not executed.
struct UpdateStats {uint64_t updates{},proceed_items{},update_items{},nodes{},node_refills{},cpu_items_run{};};
void Update(const Engine& engine,uintptr_t scene_control,float dt);
UpdateStats Updates() noexcept;
// The work-list items counted since the last call (I-0092: only at updates with Engine::count_kinds, the logged one), by list and
// vt+8 RVA, as a JSON object; the counts are then cleared.
std::string ItemKinds();
// The node-manager update FUN_14025B870, transcribed: its node pointers copied under the manager's lock (+0x58), each node
// refilled (I-0092: on Engine::pool by DS3's ranges of 10, as FUN_14025B870's ranged job). Returns the nodes visited.
uint32_t UpdateNodes(const Engine& engine,uintptr_t manager,float dt);
}
