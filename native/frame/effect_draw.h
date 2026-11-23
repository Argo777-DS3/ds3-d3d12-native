// FFX-DRAW N5b, I-0104 (the native effect base, DESIGN §2): the native effect context. The original executes each recorded effect
// command {FUN_140299E40, entity, data} with the recording pass context (Reference1953: +0x124 0x45, +0x128 the engine pass, +0x12C
// the key, +0x450 a deferred GX context, +0x458 its selector, the ring sc+0xD0[[+0x450]+0x21F8]); 299E40 builds the GXFfxDrawContext
// and calls the renderer, which generates its vertices into the ring and draws through its builder (385F70, 396F30, 39E900) after
// slot 27 (264230). Every GX statement those reach goes through the GX context wrappers 19202xx-1921xxx, whose first argument is the
// inner context ([outer+8]). Natively, as Elden Ring's GX device context under the same engine code (I-0104 ER-EFFECTS-STUDY), the
// outer and inner contexts are native blocks (readable state caches, the inner's +0 a trap object) and the wrappers are hooked: on the
// native inner context each statement sets the context's state, and a draw, dispatch, copy or clear hands that state to the caller
// (Execute's `emit`), which makes the final native draw at once. The ring and the constant blocks live in one native arena (the
// frame's: vertices 16-aligned, constant blocks 256-aligned, its first bytes zero); the state bank [display+0x2A0] and the default
// texture provider [display+0x270] are native tokens. Nothing is drawn here (the renderer's effect stage encodes the frame).
#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace ds3native::effect_draw {
struct Engine {
    uintptr_t image{};
    bool (*read)(uintptr_t,void*,size_t) noexcept{};
    void (*log)(const char*) noexcept{};
    bool gpu_particles{};   // E07 (on by default, no-ffx-gpu.flag opts out): the GXVfx appearances run too
    uint32_t width{},height{};   // the scene viewport the outer context reports (+0xC8 count, +0xCC D3D11 viewports: 263C70 and 264230 divide by it)
    bool witness{};   // ffx-record.flag: the witness rows (native_ffx_gpu_object) and the string-keyed counters (calls, bound, draws)
    bool model_particles{};   // E03 (I-0105, on by default, no-ffx-models.flag opts out): the model particles' class 3D65F88 (27EA70) runs too
    bool trails{};   // E02 trails (I-0105, on by default, no-ffx-trails.flag opts out): the classes 3D7E380 (606, 3709B0) and 3D660B8 (10012, 2891A0) run too
    bool light_shafts{};   // E08 (I-0140 tranche 10, on by default, no-ffx-lightshafts.flag opts out): the class 3D65A88 (10003, slot 8 271B40) runs too
};
// Native token objects handed to engine code in place of GX objects. Layout: +0x00 a vtable of traps (a call logs its slot and
// returns 0), +0x08 zero, +0x10 the magic, +0x14 a value, +0x18 a payload.
constexpr uint32_t texture_magic=0x5458464E,   // 'NFXT': an FFX texture, payload the GXTexture2D TextureIngress owns
    state_magic=0x5358464E,                    // 'NFXS': a state of the bank, value its bank offset
    default_magic=0x4458464E,                  // 'NFXD': the default texture of [display+0x270]
    ring_magic=0x5258464E,                     // 'NFXR': the ring's buffer, value 0
    program_magic=0x5058464E,                  // 'NFXP': a programme, payload the GX shader object the native registry keys
    copy_magic=0x4358464E,                     // 'NFXC': the scene-colour copy (the pass context's frame copy +0x4C0)
    depth_magic=0x5A58464E,                    // 'NFXZ': the scene depth (the pass context's +0x4C8)
    target_magic=0x4958464E;                   // 'NFXI': a render target (I-0140 tranche 10), value TargetValue(index, role)
// I-0140 tranche 10 (the light shafts 10003, STUDY-LIGHTSHAFT-10003): the render targets the effects name. Index 0 the pass's own
// target (the scene's lit HDR, bound in the outer context's cache +0x10/+0x18 for the execution, as the original's pass binds it),
// 1 and 2 the scene control's two half-size R8G8B8A8 targets +0x238 and +0x240 (made by the original at view attach, 24F2C0, read
// only by 271B40). Roles: the render-target view, the shader view, the resource (what CopyResource takes).
enum class TargetRole : uint32_t {Rtv,Srv,Resource};
constexpr uint32_t effect_targets=3;
constexpr uint8_t target_unbound=0xFE,target_foreign=0xFF;   // EffectDrawRecord::target: no view bound, or a view not ours
constexpr uint32_t TargetValue(uint32_t index,TargetRole role) {return index<<2|static_cast<uint32_t>(role);}
// The half-size targets' size this execution (the scene's halved as 24F2C0 halves the view's, integer division, at least 1).
std::array<uint32_t,2> TargetSize() noexcept;
struct Token {uintptr_t vtable{},none{};uint32_t magic{},value{};uintptr_t payload{};std::array<uintptr_t,4> zero{};};
// The FFX texture token naming a GXTexture2D (one per GXTexture2D, kept for the process); the GXTexture2D a token names or 0.
uintptr_t TextureToken(uintptr_t receiver);
const Token* AsToken(uintptr_t pointer) noexcept;
// The shader set (291520) stores each programme as its GX shader object's handle (the object's vt+0x10, FUN_14006D570, [+0x10]),
// null natively: the native registry keeps the bytecode under the object itself (Native1958). While a programme scope is open on
// this thread (effect_system::Build around 291520), the bootstrap's hook of 6D570 answers a null handle of a registered object
// with the programme token naming that object (one per object, kept for the process).
uintptr_t ProgramToken(uintptr_t shader_object);
struct ProgramScope {ProgramScope() noexcept;~ProgramScope();ProgramScope(const ProgramScope&)=delete;ProgramScope& operator=(const ProgramScope&)=delete;};
bool ProgramScopeOpen() noexcept;
// The effect display's two fields (EffectSystem::RedirectDisplay stores them before protecting the page): the state bank (0x300
// bytes: blend +0x08..+0x28 and +0x1A0..+0x1B8, depth +0x1E0/+0x1E8, raster +0x208/+0x218, samplers +0x2C8.., +0x2E8, each a
// state token) and the default texture provider (an object whose vt+0x10 returns the default texture token).
uintptr_t StateBank() noexcept;
uintptr_t DefaultTextureProvider() noexcept;
// The pass context's frame copy (+0x4C0) and depth (+0x4C8): GX texture objects (299E40 reads their +0x48, the view) whose view is the
// copy or depth token; slot 27 binds them where the original binds its frame copy (t0 of the distortion programmes, t4) and depth (t3).
uintptr_t FrameCopyTexture() noexcept;
uintptr_t DepthTexture() noexcept;
// I-0088 final review M1: the settings shadow Execute names at [[GXRS]+0x4DD0] while it runs (E07); the module's readers of that
// member treat it as absent (natively the member is null), whichever thread asks during the record.
uintptr_t DeviceShadow() noexcept;
// I-0120 D3: the engine bodies the module calls on other threads that test [[GXRS]+0x4DD0] and, with it set, sample through
// root+0x41E0 (61950, 1AC840: null natively, Native899) run only while the shadow is not named there. EnterEngineShUse returns
// true when the caller may run such a body now (then LeaveEngineShUse after it), false while Execute names the shadow (the caller
// then takes the device-absent path itself or waits); Execute names it only once no such body runs (a Dekker handshake).
bool EnterEngineShUse() noexcept;
void LeaveEngineShUse() noexcept;
// EnterEngineShUse, waiting for the record to end while the shadow is named (never on the thread that executes the record);
// true when it gave up after 2 s and the caller runs with the shadow possibly named (as before D3).
bool WaitEngineShUse() noexcept;

// The context's state at a statement (a draw, or an E07 compute, copy, clear or indirect statement): the GX state current at it
// (state is sticky across statements, as GX's), by slot. The constant buffers bound at VS b0, DS b0, PS b0, PS b1 and PS b6 (slot
// 27's binds), and the E07 slots GS b9, PS b9 and b11, CS b0, b1 and b5, are given as arena ranges holding the bytes of their last
// UpdateSubresource (a buffer never updated: its creation bytes; none: the zero block); other slots are kept as objects only.
enum ConstantSlot : uint32_t {VsB0,DsB0,PsB0,PsB1,PsB6,GsB9,PsB9,PsB11,CsB0,CsB1,CsB5,ConstantSlots};
// CopyRect (I-0113 tranche 23): the draw context's copy slot 140260440 (a distortion quad's screen rectangle copied into the frame copy).
// ClearTarget, CopyTarget (I-0140 tranche 10): a clear of one of the targets above (clear_target, clear_colour) and a whole copy
// between two of them (copy_target_destination, copy_target_source), the light shafts' ClearRenderTargetView and CopyResource.
enum class Op : uint8_t {Draw,Dispatch,DispatchIndirect,Copy,ClearUav,DrawIndirect,CopyRect,ClearTarget,CopyTarget};
struct Range {uint32_t offset{},size{};};   // bytes of the arena; {0, 0}: the zero block (zero_bytes)
constexpr uint32_t zero_bytes=0x1000;        // the arena's first bytes, zero
struct EffectDrawRecord {
    Op op{Op::Draw};
    int32_t pass{};uint32_t key{};
    uintptr_t cs{};std::array<uintptr_t,16> gs_constants{},vs_resources{},cs_constants{},cs_resources{},cs_uavs{};
    uint32_t groups[3]{};uintptr_t args{},destination{},source{},clear_view{};std::array<uint32_t,4> clear_values{};
    std::array<uint32_t,16> cs_uav_counts{};uint16_t cs_uav_counts_set{};   // the initial counts of the last CS UAV bind (bit set: not -1), applied once
    uint32_t topology{};uintptr_t layout{},vs{},hs{},ds{},gs{},ps{},blend{},depth{},raster{};
    std::array<uintptr_t,16> vs_constants{},ds_constants{},ps_constants{},ps_resources{},ps_samplers{};
    std::array<Range,ConstantSlots> constants{};
    bool constants_invalid{};   // a bound block's last update was partial or unreadable, or the arena is full
    uintptr_t vertex_buffer{};uint32_t stride{},vertex_offset{};
    uintptr_t index_buffer{};uint32_t index_format{},index_offset{};
    bool indexed{};uint32_t count{},start{};int32_t base{};
    // CopyRect: copy_kind 0 the rectangle copy_rect (left, top, right, bottom in [0, 1] of the target), 1 the whole target, 2 the
    // box (0, 0) to copy_size (pixels: the draw context's +0xA88 branch, [+0x7C] + 2 x [+0x78], [+0x80] + 2 x [+0x78]); kinds 0 and
    // 2 are widened by 16 pixels on each side and clamped to the target by the renderer, as the original's.
    std::array<float,4> copy_rect{};uint8_t copy_kind{};std::array<int32_t,2> copy_size{};
    // I-0140 tranche 10: the render target bound (OMSetRenderTargets, 1920F40): its index above (0 the pass's own, the default),
    // target_unbound when the bind named no view, target_foreign when it named something else (an engine view): the draws are then
    // dropped;
    // the first viewport (RSSetViewports, 1920CD0: x, y, width, height, min and max depth) and scissor rectangle (RSSetScissorRects,
    // 1920CF0: left, top, right, bottom) the context was given, each with its flag (none given: the stage's own).
    uint8_t target{};
    bool viewport_set{};std::array<float,6> viewport{};bool scissor_set{};std::array<int32_t,4> scissor{};
    uint8_t clear_target{},copy_target_destination{},copy_target_source{};std::array<float,4> clear_colour{};
};
// The arena this frame (the vertices, indices and constant blocks the ranges and offsets address), its used bytes; a block appended
// to it (a static buffer's bytes; `alignment` a power of two): its offset, UINT32_MAX when the arena is full.
const std::vector<uint8_t>& Arena() noexcept;
size_t ArenaUsed() noexcept;
// I-0113 gap 12 (Flame Fan's fan, ffx-record.flag): the next n patch draws (one control point, GX 10, D3D 33) each log their DS b0 words 84..111
// (near/far, eye, the near and far fades, the camera, the soft-sprite scale) and PS b0 g_vColorScale, as reference_ffx_patch_draw.
void RequestPatchRows(int n) noexcept;
// I-0141 (the ceremony's smoke): the next n VSCommonBuffer images (0x150 bytes, the effects' vertex block) logged whole, rows
// ffx_vs_image (ffx-record.flag; asked by F9 with the pixel-shader blocks).
void RequestVsRows(int n) noexcept;
uint32_t Append(const void* bytes,size_t size,size_t alignment);
// A released effect object (any thread): its captured constant bytes are forgotten at the next Execute (a reused handle starts
// as created).
void Forget(uintptr_t handle,uint64_t serial);
// E07: the serials of the effect objects released since the previous call (the renderer retires their resources); GiveBack returns
// serials taken for a frame that was not made (they go with the next one).
std::vector<uint64_t> TakeReleased();
void GiveBack(const std::vector<uint64_t>& serials);
struct Command {uintptr_t callback{},entity{},data{};uint32_t key{};int32_t pass{};};
// E03 (FXR 605, I-0105): one FLVER model the model-particle renderer draws (27EA70, per particle its GXFfxFlverModelRenderer's slot
// +0x20, 29ADF0) on the native pass context: what the original hands the engine's mesh loop (1400636F0 → 140063CB0) through the pass
// context's model context +0x460, which is null natively (Native2035's fault, in 5A120's 149CB0). The model, its LOD (args+0x18,
// clamped as 636F0 clamps it), the material table override (args+0x1A8; null: the model's own), the 400-byte draw state (args+0x240,
// 1A8580's copy of the state 29ADF0 builds from the FXR fields), the particle's light (args+0x3E0..+0x427: three SH rows, the blend
// scale +0x410, the strength +0x420 and the flag +0x424 that 29ADF0 sets after 1A8580 from the FXR's +0x50..+0x98, which 59470
// takes instead of the state's light banks), the bone rows (3x4 floats each, the model's +0x1C of them, from the array 2BA3A0
// passes: [instance+0x70], args+0x48 being clear) and the command's engine pass and key. Natively the GX uploads of 29ADF0 (59470,
// 1A86C0) and the mesh loop do not run: the native model path draws the submission.
// blend, depth: the bank offsets of the states the effect context had bound when 636F0 ran (0: none known; i105-bolt-ref-11); the
// mesh loop rebinds both (I-0113 gap 17: NativeEffectModelBlend, Begin's depth): the blend serves only when that rule bails, the
// depth only as a witness.
// The command that bound a state on the native context (its engine pass, key and renderer class RVA; pass -1: none).
struct StateOrigin {int32_t pass{-1};uint32_t key{};uintptr_t renderer{};};
// ffx-record.flag (I-0113 gap 5): the commands run in this execution up to the model particle's own (the last), oldest first (at most
// 16), with the blends each bound (`blends`), against the Reference's reference_e03_states sequence.
struct CommandRun {StateOrigin command{};uint32_t blends{};};
// blend_origin, depth_origin (I-0113 gap 5, witness only since gap 17): the commands that bound them (a command binding nothing, as a
// model particle's, passes on what it found). skipped_origin: the last command the native did not run before the particle.
// arg_flags, arg_depth (I-0113 gap 17): the draw arguments' +0x200 word (29ADF0's bit 21; 5A120's bits 14 and 15 clear) and
// +0x188 (29ADF0 writes -(command[0] != 0) there after 59470, 14029B23E) at 636F0, which the mesh loop's material blend-mode
// parameter (140200840) and Begin (140140680) read for the draw's blend and depth.
// arg_cull, arg_cull_mode (I-0113 gap 12, Flame Fan's s08280): the draw arguments' +0x180 word and +0x184, from which 64610 picks the
// face sets' cull row of 14473C9F8: row 2 {none, none, none} for bit 9 or +0x184 = 2, row 1 {none, back, front} for bit 7 or
// +0x184 = 1, else row 0 {none, front, back}.
struct ModelSubmission {uintptr_t model{},materials{},bones{},renderer{};uint32_t lod{},key{},blend{},depth{},arg_flags{},arg_cull{};int32_t arg_depth{},arg_cull_mode{},pass{};
    StateOrigin blend_origin{},depth_origin{},skipped_origin{};
    std::array<uint8_t,0x190> state{};std::array<uint8_t,0x48> light{};std::vector<std::array<float,12>> rows;std::vector<CommandRun> sequence;};
// The GX wrappers hooked natively and (E03) the engine's model draw functions 59470, 1A86C0 and 636F0, with their detours; the
// bootstrap creates the hooks and stores the originals.
struct Hook {uint32_t rva;void* detour;void** original;};
std::vector<Hook> Hooks();
// Executes the commands in the original's order (engine passes 0x21, 0x29, 0x2D, 0x20, 0x31, then key order within a pass) with
// the pass context `pass_context` (the native recording context); each draw, dispatch, copy or clear calls `emit` with the context's
// state, in order, and each model particle's FLVER draw calls `models`. `sc` is the scene control (its ring slot sc+0xD0[0] receives
// the native ring). Returns the commands run.
uint64_t Execute(const Engine& engine,uintptr_t pass_context,uintptr_t sc,std::vector<Command> commands,const std::function<void(const EffectDrawRecord&)>& emit,
    const std::function<void(ModelSubmission&&)>& models);
// Counters since the last call (traps, ring bytes, refusals; with ffx-record.flag also the draws by pass/topology, the wrapper calls
// by kind and the token kinds of the bound resources), as one JSON object; cleared.
std::string Summary();
}
