// Module state: the engine hooks' original entries, the native registries and their locks, the module flags, the model
// layout types, the hook census and the guarded engine read. Part of game_bootstrap.cpp's single translation unit (the
// order is there): it relies on every earlier part.
#pragma once
#include "../game_bootstrap_includes.h"
#include "../../renderer/dlss_mode.h"
#include "../../renderer/rekindled_settings.h"
#include "../../world/view_blend.h"
#include "../../world/logic_rate.h"
#include "../../world/frame_rate_option.h"
namespace {
using namespace ds3native;
using Phase=void(__fastcall*)(void*);
Phase start_original{},view_original{},resources_original{};
Phase cpu_wait_original{},ui_task_progress_original{};
using UiCpuStep=void(__fastcall*)(void*,uintptr_t);
UiCpuStep ui_consumers_original{};
UiCpuStep ui_task_dispatch_original{};
using UiPointerQuery=uintptr_t(__fastcall*)(uintptr_t,uintptr_t);
using UiInputPredicate=uint8_t(__fastcall*)(uintptr_t);
using UiMovieAdvance=void(__fastcall*)(uintptr_t,uintptr_t,uint32_t,uint32_t);
UiPointerQuery ui_pointer_original{};
UiInputPredicate ui_input_eligible_original{},ui_condition13_original{};
UiMovieAdvance ui_movie_advance_original{};
using UiManagerDestroy=void(__fastcall*)(uintptr_t);
UiManagerDestroy ui_manager_destroy_original{};
using InputActionUpdate=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t);
InputActionUpdate input_confirm_update_original{},input_step_run_original{};
using MouseDeviceUpdate=void(__fastcall*)(uintptr_t,uintptr_t);
MouseDeviceUpdate mouse_device_update_original{};
using DynamicMeshBufferSet=uint8_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t);
DynamicMeshBufferSet dynamic_mesh_buffer_set_original{};
using ObjectProbeLightBlend=void(__fastcall*)(uintptr_t,float);
ObjectProbeLightBlend object_probe_light_blend_original{};
using DrawableRegistration=void(__fastcall*)(uintptr_t,uintptr_t);
DrawableRegistration drawable_registration_original{};
// Serializes native UI frame preparation (render owner thread) with the genuine
// engine destruction of the UI manager (CPU task thread).
std::timed_mutex ui_frame_preparation_lock;
std::atomic<uintptr_t> ui_startup_task{};
std::atomic<uint32_t> active_ui_dispatches{};
std::atomic<uintptr_t> graphics_startup_task{};
Phase effect_retire_original{};
Phase material_retire_original{};
Phase gparam_retire_original{};
using DecalCpuRetire=void(__fastcall*)(uintptr_t);
DecalCpuRetire decal_retire_original{};
using DecalRequest=uint32_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t);
DecalRequest decal_request_original{};
using CpuWorldQuery=void(__fastcall*)(uintptr_t,uintptr_t);
CpuWorldQuery decal_world_query_original{};
// 1072 witness: SprjAsmModelDrawEntity update (vtable 2947DB8+D8 = D01610).
using AsmEntityUpdate=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t);
AsmEntityUpdate asm_update_original{};
using ModelEntityUpdate=void(__fastcall*)(uintptr_t,uintptr_t);
ModelEntityUpdate model_entity_update_original{};
// 1074 witness: ChrAsmModel / SprjAsmModel vtable+18 (C34420 / C47CD0) -> D00040, and
// their common vtable+10 C48180(model, float dt) that applies dirty flags then calls +18.
using AsmModelUpdate=void(__fastcall*)(uintptr_t,float,uintptr_t,uintptr_t);
AsmModelUpdate chr_asm_model_update_original{},sprj_asm_model_update_original{};
using AsmModelTick=void(__fastcall*)(uintptr_t,float);
AsmModelTick asm_model_tick_original{};
// 1076 witness: PlayerIns/ReplayGhostIns vtable+120 = 8C1BC0(chr, float dt), the per-frame
// character model update that ticks ChrAsmModel ([chr+20B0] vtable+10 C48180).
AsmModelTick player_model_update_original{};
// 1080 witnesses: ChrIns EzTasks (SprjEzVoidTask<SprjEzRabbitNoUpdateTask,ChrIns>, vtable 284F440:
// +10 execute 888C00 runs [task+28]([task+20]) once when armed (+18), +20 register 65CAC0 arms it)
// and PlayerIns vtable+118 8CA1E0 (sets [chr+1A0D] bit0, the 891AB3 model-update gate).
using EzTaskExecute=void(__fastcall*)(uintptr_t);
EzTaskExecute chr_eztask_execute_original{};
using EzTaskRegister=void(__fastcall*)(uintptr_t,uint64_t,uint64_t);
EzTaskRegister eztask_register_original{};
using ChrRequest=void(__fastcall*)(uintptr_t,uintptr_t);
ChrRequest chr_model_request_original{};
// 1082 witness: ChrIns task scheduling 88E1F0(chr, mode) from WorldChrMan 8DD2E0: mode 0 full
// (model task), 1 staggered, 2 gated out ([chr+18]+20 flags, [chr+18]+10 == 4, chr+110 interval,
// chr+1EE9 bit3). Logged for the main player [[14477FDB8]+80].
using ChrSchedule=void(__fastcall*)(uintptr_t,int32_t);
ChrSchedule chr_schedule_original{};
// 1084 witness: WorldChrMan per-frame scheduling 8DD2E0(manager, float dt): calls, its chr sets
// (+1C8 count, +1D0 stride 0x18 -> [set] count) and whether the main player is in one.
using ChrManagerSchedule=void(__fastcall*)(uintptr_t,float);
ChrManagerSchedule chr_manager_schedule_original{};
// I-0138 tranche 8g: SprjRemoIns's Play_Wait step CD5D80(Ins, step context), reached through its step table (144A3001C), once a
// game frame while a Remo cutscene plays (the WorldChrMan update above is not armed then unless the Remo flags' bit 5 is set,
// or in a frame where the one-shot [step+1F6] is set (its setters not read) and neither 4780769 nor [step+A9] clears it after: 8FA3E0's running flag
// below, cleared by 8F2CA0). Its result is passed
// through untouched (the detour returns the original's own, nothing run after it).
using RemoPlayWait=uintptr_t(__fastcall*)(uintptr_t,uintptr_t);
RemoPlayWait remo_play_wait_original{};
// 1086 witness: 8FA3E0(step, running, dt, p4), called once per frame by 8F2CA0; it tail-calls
// 8DBDA0(WorldChrManImp, mode): mode 0 (running) arms the WorldChrMan update RabbitTask +31A8
// (8DF920 -> 8DD2E0), modes 1/2 only +3238/+3268/+3298. running = [step+270], cleared by
// [step+A9], [step+98], SprjRemo [[4791248]+8]+154 bit0 && !bit5, SprjEventMan [[4752F80]+8]
// +C/+D, [step+1F5] or the one-shot 4780769; forced by the one-shot [step+1F6] (cleared when used; 4780769 and [step+A9],
// tested after it, still clear it).
using WorldUpdateGate=void(__fastcall*)(uintptr_t,uint8_t,float,uint8_t);
WorldUpdateGate world_update_gate_original{};
// 1104 witness: D2D040(binding, pose receiver part+50, matrices, retarget, ...) per character part (D01610 type-1 parts).
using PartPoseUpdate=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
PartPoseUpdate part_pose_update_original{};
struct NativeDecalCpuIntent {
    uint32_t handle{},world_layers{},kind{};   // kind 1: the inert request class (no prepared description)
    // Engine pointers inside these copied CPU descriptions are provenance,
    // not native material/resource identities. Consumer admission is separate.
    std::array<BYTE,0x118> descriptor{};
    std::array<BYTE,0xF8> prepared{};
};
struct NativeDecalCpuRegistry {
    uintptr_t heap{},observer{},buckets{},generations{};uint32_t count{};
    // Borrowed authentic CPU bank entries. 51780 clears these without releasing
    // them; the genuine shader bank owns their lifetime. GPU residency remains
    // separate and decal emission refuses until its native encoder exists.
    std::array<uintptr_t,47> programmes{};
    std::array<uintptr_t,14> constant_descriptions{};
    uintptr_t compute_programme{};
    std::unordered_map<uint32_t,NativeDecalCpuIntent> requests;
};
std::recursive_mutex decal_cpu_lock;
std::unordered_map<uintptr_t,NativeDecalCpuRegistry> decal_cpu_registries;
// DECALS-PAINT-READY (D2): the native paint target of each struck character's GXDecalReceiver (vtable 3D41DD8), made at the
// receiver's first paint (the original makes its textures lazily there, 4CFB0), with the views t41 (diffuse, sRGB) and t42
// (specular) of the _Bloody materials; retired when the receiver's vtable or slot-0 target changes. a model record
// bound to a receiver keeps its t41 view and is re-admitted when it changes (C7-2, I-0120 B1).
struct NativePaintRecord {
    uintptr_t slot{};   // the receiver's slot-0 GXPaintRenderTarget (+0x20) when the target was made
    std::shared_ptr<const NativePaintTarget> target;
    std::shared_ptr<const bootstrap::NativeTextureRecord> diffuse,specular;
    std::shared_ptr<const bootstrap::NativeTextureRecord> occlusion;   // D-0193 rung 2b: the R16 view of the target's projector depth map
    uint32_t announce{};   // collects that still list the target for its first clear (the renderer clears it once)
};
std::mutex native_paint_lock;
std::unordered_map<uintptr_t,NativePaintRecord> native_paints;   // key: the receiver
struct NativePaintViews {std::shared_ptr<const bootstrap::NativeTextureRecord> diffuse,specular;};
struct NativeDecalQueryScope {uintptr_t manager{},descriptor{},callback{};uint32_t completed{},layers{};};
thread_local NativeDecalQueryScope* native_decal_query_scope{};
struct NativeSceneTreeCpu {uintptr_t backing_heap{},heap_wrapper{};uint64_t arena_bytes{};};
std::mutex scene_tree_cpu_lock;
std::unordered_map<uintptr_t,NativeSceneTreeCpu> scene_tree_cpu_owners;
// Engine-facing CPU scene/view state. These prefixes expose authored camera,
// clear and feature intent; they never contain a legacy GPU object or pool ID.
// I-0140 tranche 7: `shadow_record` the native's copy of the context's ShadowParam record (the engine's +0x90, which 843C0
// fills with 839F0's defaults; the native context is zeroed, so this copy starts from those defaults), which 88A10's blend (DAF40)
// partly keeps from one frame to the next; CollectNativeShadowState owns it. The other writer of +0x90, E00270 (a stage's own sun
// direction, from DFD270), is not run natively (the step review's L4: a lead).
struct NativeSceneContextCpu {uintptr_t heap{},secondary{};std::array<uint8_t,0x94> shadow_record{};bool shadow_record_ready{};};
struct NativeSceneViewCpu {uintptr_t heap{},context{};uint64_t generation{};std::array<BYTE,0x26> features{};};
std::recursive_mutex scene_view_cpu_lock;
std::unordered_map<uintptr_t,NativeSceneContextCpu> scene_context_cpu_owners;
std::unordered_map<uintptr_t,NativeSceneViewCpu> scene_view_cpu_owners;
uintptr_t active_scene_context{};
uint64_t scene_view_generation{};
std::optional<SceneViewInput> native_scene_camera;
// W5a SW-7: the CED670 camera output bytes of native_scene_camera (+0x00..+0x87: pose, lens, extent), the input of the
// genuine 14006E9E0 that builds a view's frustum record (FCE30 builds the light-visibility record that way).
std::optional<std::array<uint8_t,0x88>> native_scene_camera_bytes;
std::optional<ingress::SceneCameraConstants> native_scene_camera_constants;
std::array<std::shared_ptr<const bootstrap::NativeTextureRecord>,4> native_stage_textures;   // D1E8A0's records (only pinned here; I-0105 tranche 10 resolves the root's holders again)
uint64_t native_stage_texture_generation{};
std::shared_ptr<const void> native_region_snapshot_last;   // I-0113 tranche 14: CollectNativeRegionSnapshot's last (its ready cubes kept), released at Close
void ClearNativeRegionMemos();   // I-0113 tranche 14 (09_model_admission), at Close
std::shared_ptr<const void> native_snow_textures_last;   // I-0105 tranche 10: CollectNativeSnowTextures' last value (NativeSelectedScene::SnowTextures), released at Close
// FFX-DRAW (the whiter bonfire core, Reference2039 / Native2040): the blended Tone Map exposure of the world view (the YEBIS feed, word 0, when
// the view's pipeline bit 10 is set), which 140089140 answers from [[GXRS+0x4DD8]+0xCB8]+0x21B0 in the original; 0: none.
std::atomic<float> native_effect_exposure{0.f};
struct NativePostTransitionIntent {
    std::array<float,2> controls{};
    std::array<uint32_t,4> intervals{};
    std::array<uint32_t,18> targets{};
    uint64_t generation{};
};
std::atomic<uint64_t> native_post_transition_generation{};   // I-0137 (I-0120 A8): the intent itself was stored and never read: only its count
using SceneCameraProducer=void(__fastcall*)(void*,void*);
SceneCameraProducer scene_camera_original{};
struct NativeSceneAuxiliaryView {
    std::array<float,16> projection{};
    std::array<float,3> position{};
    uint32_t width{},height{};
};   // I-0137 (I-0120 A8): built and written to the engine's service, no longer stored (never read)
// Native scene root (957-968, LEGACY-ROOT-INVENTORY): 4961B50 is the engine's
// own scene root class (213A0, 0x53B0) constructed on the CPU heap 4961B80,
// with its genuine CPU sub-managers (486A0: lighting params +41C0, env regions
// +42B0, ambient probes +46E8 ...). Its device members +4DD0/+4DD8 stay null:
// the engine's own device-absent branches (tested at ~150 sites) apply. B5E00's
// display/device initialization (48B80, 85540, 773A0, 78940, E2A70) is not run.
uintptr_t native_scene_root{};
std::mutex native_texture_constant_lock;
// 1148: cube textures admitted from TPF containers, by entry name (env-map region cubes).
std::mutex native_cube_name_lock;
std::map<std::string,uintptr_t> native_cube_names;
std::atomic<uint64_t> native_cube_names_generation{};   // I-0113 tranche 14: bumped (under native_cube_name_lock) at each name published
// W10 GR: the colour grading LUT textures (2D TPF entries named mAA_BB_CGrading<id>) admitted from TPF containers, by entry name; the
// grading record asks the texture repository by that name (14021CC20), which is absent natively. Guarded by native_cube_name_lock.
std::map<std::string,uintptr_t> native_grading_names;
std::unordered_map<uintptr_t,std::array<float,4>> native_texture_constants;
struct NativeEnvRegionCpuRegistry {
    uintptr_t manager{}; // the native scene root's genuine manager, root+0x42B0
    uintptr_t heap{},buckets{},packed{};
    uint32_t count{512};
    // Real CPU wrappers/records, never old graphics objects or resource IDs.
    std::unordered_map<uintptr_t,uint32_t> wrappers;
};
std::recursive_mutex env_region_cpu_lock;
std::unique_ptr<NativeEnvRegionCpuRegistry> env_region_cpu;
bool env_region_cpu_closed{};
// The registry's calls into the game's manager made without its lock (65D60 from a request, every 664D0 through its hook): the close
// waits for them before 66640 (the step reviews of the final review's answers, M1 and M2).
std::atomic<uint32_t> env_region_cpu_calls{};
using EnvRegionCpuRelease=BYTE(__fastcall*)(uintptr_t,uintptr_t);
EnvRegionCpuRelease env_region_release_original{};
// I-0140 tranche 28 (its third step review): 66640, the clear-all, hooked as 664D0 (the game's own callers D0D060 and 65B70, the
// manager's destructor's); the close calls its trampoline. The closed manager is kept for 652D0's answers after the close.
using EnvRegionCpuClear=void(__fastcall*)(uintptr_t);
EnvRegionCpuClear env_region_clear_original{};
uintptr_t env_region_cpu_closed_manager{};
using EnvRegionCpuRequest=uintptr_t(__fastcall*)(uintptr_t,uintptr_t);
EnvRegionCpuRequest env_region_request_original{};
using EnvRegionCpuDestroy=uintptr_t(__fastcall*)(uintptr_t,BYTE);
EnvRegionCpuDestroy env_region_destroy_original{};
void CloseNativeEnvRegions();
using ProcessedPrimitive=void(__fastcall*)(void*,void*,uintptr_t,uintptr_t);
ProcessedPrimitive processed_primitive_original{};
volatile LONG64 processed_primitive_calls{};
using UiFactory=void(__fastcall*)(uintptr_t);
UiFactory ui_factory_original{};
volatile LONG64 ui_factory_calls{};
using MeshWriteDestinations=void(__fastcall*)(void*,void*,uintptr_t*,uintptr_t*);
MeshWriteDestinations mesh_write_original{};
volatile LONG64 mesh_write_calls{};
using CpuMeshWrite=void(__fastcall*)(void*,uintptr_t,uintptr_t,uintptr_t,BYTE);
CpuMeshWrite cpu_mesh_write_original{};
struct CpuMeshContext {uintptr_t cache{},batch{},meshes{};};
thread_local CpuMeshContext cpu_mesh_context;
using CpuRawImage=uintptr_t(__fastcall*)(uint32_t,int,uintptr_t,uint32_t,uintptr_t,uintptr_t);
CpuRawImage cpu_raw_image_original{};
Phase cpu_image_decode_original{};
using CpuImageDelivery=void(__fastcall*)(void*,uintptr_t);
CpuImageDelivery cpu_image_delivery_original{};
struct CpuImagePlane {uint32_t width{},height{};uint64_t pitch{},bytes{};uintptr_t data{};};
struct CpuImageData {uint32_t format{},use{};uint8_t flags{},mips{};uint16_t planes{};uint32_t padding{};uintptr_t table{},palette{};CpuImagePlane inline_plane;};
static_assert(sizeof(CpuImagePlane)==32 && sizeof(CpuImageData)==64);
struct CpuDecodeScope {LONG64 call{};std::vector<CpuImagePlane> allocations;std::vector<std::shared_ptr<void>> owners;};
thread_local CpuDecodeScope* cpu_decode_scope{};
volatile LONG64 cpu_decode_calls{},cpu_image_deliveries{};
using CpuImageUpdate=uint64_t(__fastcall*)(void*,uintptr_t,uint32_t,uint32_t);
CpuImageUpdate cpu_image_update_original{};
volatile LONG64 cpu_image_update_calls{};
struct CpuImageUpdateRecord {CpuImagePlane source;int32_t x1{},y1{},x2{},y2{};uint32_t plane{},padding{};};
static_assert(sizeof(CpuImageUpdateRecord)==56);
bootstrap::Control* control{};
uintptr_t image{};
// 1087: MoveMapStep (state table 459F630, STEP_MoveMap = 8F2CA0) calls seen by the 8FA3E0 witness.
volatile LONG64 world_gate_calls{};
volatile uintptr_t world_gate_step{};
HANDLE journal{};
std::mutex journal_lock;
// R13 cF.7 (phase F exit semantics, PILOT D-10): the renderer is never destroyed. ExitProcess ends the other threads before
// the module's static destructors run, so a destructor that drains or takes a lock could hang; the process releases it.
SceneRenderer& renderer=*new SceneRenderer;
// cF.7: the single process exit of the module (Refuse and the fault exits call it): one function pointer, so a test host
// can replace it (no setter yet: the module has no test host; cR3 adds one with its fault tests).
// I-0082: `reason` (the refusal or the failure's text) goes to the crash record; the exit classifies a lost GPU itself.
[[noreturn]] void ExitNative(UINT code,const char* reason=nullptr,bool failure=false) noexcept;   // failure: a refusal that follows a native failure
void(*exit_native)(UINT) noexcept=[](UINT code) noexcept {ExitProcess(code);};
DeviceQueues& gpu=renderer.CpuIngressDevice();
// I-0109 (R0): the ray-tracing path's bindless heap, opened by the start self-test; engine thread until R2 gives it a frame owner.
BindlessHeap& bindless=*new BindlessHeap;
// I-0117 (R2 T0): the models' BLAS (the rays' scene, set at the start once the device offers DXR 1.1 and the self-test did not fail).
std::atomic<bool> native_rt_scene{};RtBlasRegistry& native_rt_blas=*new RtBlasRegistry;
// I-0129 R9a (R9-PLAN §2): the traced probes' states (rt_probes.h), by draw group (the forward rows' objects) and, R9b, by an effect's
// cell: the renderer's completions deliver them, the admissions read them (ApplyNativeRtProbe). Never destroyed (the render worker's
// completions may outlive the module's shutdown order).
std::shared_ptr<RtProbeStates>& native_rt_probe_states=*new std::shared_ptr<RtProbeStates>(std::make_shared<RtProbeStates>());
// I-0129 R9b (R9-PLAN §1.2): DS3's effect probe query 1DB60 (its probe volumes at an effect instance's world position: the classic
// draws' PSCommonBuffer SH rows through 264230, the GPU particles' cbCubeEnvSH1 rows through 2A4EB0 and its twins), detoured in the
// native mode: the original's call, then the position's cell noted for the next requests and, when the cell's traced probe answers,
// its rows (times the query's own scale, param 7, as the original's end) in place of the SH output and its sun share in the float4
// mask; DS3's later choices (a manual SH, the light set's hemisphere, the point lights, the particles' smoothing) still apply after it.
// On only with the rays' probes (effect_probe::on, set with renderer.rt_probes and cleared when they fail: the collect's row); the
// cells at most kRtProbesMost a collect (the first noted). Its callers read no return value (264230's JMP after the call, 2A4EB0's
// statement call, OBSERVED; 2ADC80 and 2AE5A0, its twins, INFERRED).
bool ModuleFlag(const wchar_t* name) noexcept;void Log(const char* text) noexcept;   // defined below (the detour's diagnostic)
namespace effect_probe {
using QueryFn=void(__fastcall*)(uintptr_t,float*,float*,float*,uint64_t,float*,float);
QueryFn original{};
std::atomic<bool> on{};
std::mutex lock;std::vector<uint64_t> cells;std::unordered_set<uint64_t> noted;
void Note(uint64_t key) noexcept {try{const std::lock_guard hold(lock);if(cells.size()<kRtProbesMost && noted.insert(key).second)cells.push_back(key);}catch(...){}}
std::vector<uint64_t> Take() {const std::lock_guard hold(lock);noted.clear();return std::exchange(cells,{});}
void __fastcall Query(uintptr_t manager,float* position,float* sh,float* mask,uint64_t fifth,float* box,float scale) noexcept {
    original(manager,position,sh,mask,fifth,box,scale);
    if(!on.load(std::memory_order_relaxed) || !position || !sh || !std::isfinite(position[0]) || !std::isfinite(position[1]) || !std::isfinite(position[2]))return;
    const uint64_t key=RtProbeCellKey(position);Note(key);
    try{const uint64_t now=GetTickCount64();auto state=native_rt_probe_states->Find(key,now);
        // The final review's M2: a cell not answered yet (an effect that just moved into it) takes a face neighbour's answer, so a
        // moving effect does not fall back to DS3's rows for the frames in flight at each cell it enters.
        uint64_t answered=key;
        if(!state)for(int axis=0;axis<3 && !state;++axis)for(const float step:{-kRtProbeCell,kRtProbeCell}){float q[3]{position[0],position[1],position[2]};q[axis]+=step;
            if((state=native_rt_probe_states->Find(answered=RtProbeCellKey(q),now)))break;}
        if(!state)return;
        static const bool observe=ModuleFlag(L"rt-probe-observe.flag");float before[4]{sh[3],sh[7],sh[11],mask?mask[0]:-1.f};
        if(state->ambient)for(size_t k=0;k<12;++k)sh[k]=state->rows[k]*scale;
        if(state->sun_valid && mask)for(size_t k=0;k<4;++k)mask[k]*=state->sun;   // M1: times DS3's own, as the forward rows' mask
        // M5: the detour's writes, a cell's first 4 then every 600th (at most 4,000 rows), with the key that answered (a neighbour's: M2).
        if(observe){static std::mutex seen_lock;static std::unordered_map<uint64_t,uint32_t> seen;static uint32_t rows{};bool show{};
            {const std::lock_guard hold(seen_lock);const auto n=seen[key]++;show=rows<4000 && (n<4 || n%600==0);if(show)++rows;}
            if(show){char row[360]{};
            sprintf_s(row,"{\"event\":\"native_rt_probe_effect\",\"cell\":%llu,\"neighbour\":%s,\"ambient\":%s,\"sun\":%s,\"baked_l0\":[%.4g,%.4g,%.4g],\"traced_l0\":[%.4g,%.4g,%.4g],\"mask\":[%.4g,%.4g]}\n",
                static_cast<unsigned long long>(key),answered!=key?"true":"false",state->ambient?"true":"false",state->sun_valid?"true":"false",before[0],before[1],before[2],sh[3],sh[7],sh[11],before[3],mask?mask[0]:-1.f);Log(row);}}
    }catch(...){}
}
bool Install() noexcept {return MH_CreateHook(reinterpret_cast<void*>(image+0x1DB60),reinterpret_cast<void*>(&Query),reinterpret_cast<void**>(&original))==MH_OK;}
}
void CloseNativeRtSkinned() noexcept;   // I-0117 (R2 T2): the skinned instances kept between walks (12_scene_collect.inc), at the shutdown's drain
void CloseNativeRtMaterials() noexcept;     // I-0117 (R2 T3), I-0119 (R3a): the material sets kept between walks, alike
void ResetNativeRtKept() noexcept;      // I-0117 (R2 T3): both at a scene reset
// R13 cR3 (render-thread sheet section 8): the native render worker, by default since cR6 (null under inline-render.flag: every frame
// job runs inline in Clear(), as before). Created at Start, stopped by Close after its quiesce; leaked like the renderer
// (process exit ends its thread first; cF.7, PILOT D-10). Owner thread only.
RenderWorker* render_worker{};
// I-0088 P1 (I-0086 ER-STUDY §3: ER runs its UI task beside the scene render and joins it after; DS3's E5B940 queued the per-movie
// display on GX's command queue): the UI display (the movies' selection and E52EE0 with the native HAL behind it, PrepareFrame)
// runs on its own worker from the collect's entry, beside the collect, and the render step joins it. No DS3 step runs during the
// DrawStep row (ENGINE-STEP-THREAD-MAP), so no movie is advanced or destroyed meanwhile; E52EE0 takes the movies' own locks (not
// ER's global lock held over the whole draw). The HAL's thread word (+0x50) names the thread that built the session, a pool
// thread in 472 of 475 runs (ENGINE-STEP-THREAD-MAP), never the owner: the display ran off that thread before and still does,
// so a reader comparing it takes the same branch (step review M2). Null under inline-render.flag (the render step prepares inline, as before). The
// job writes native_ui_display; the owner reads it only after Wait. Owner thread only.
RenderWorker* ui_display_worker{};
// I-0088 P2 (ER-STUDY §9 P2: the collect's independent parts as tasks beside the walk and admission, joined per group, not pool-
// wide): the main view's effect record (RecordNativeEffects) on its own worker from the set-up, joined before the main walk's
// admission (I-0105: its model particles are admitted with the walk's candidates). Null under inline-render.flag or no-collect-side.flag (A/B): recorded inline as before. Owner thread only.
RenderWorker* collect_side_worker{};
// I-0132 R11g: the rays' walk of the main view's collect (CollectNativeRtScene) on its own worker, posted before the main walk and
// joined before the rays' forward blocks (12_scene_collect). Null under inline-render.flag, no-rays-aside.flag (A/B) or under twelve
// hardware threads: walked on the engine thread after the admission as before. Owner thread only.
RenderWorker* collect_rays_worker{};
struct NativeUiDisplay {bool posted{};uint64_t serial{};uint32_t width{},height{},movies{};
    std::optional<frame::PrimitiveWorkPacket> packet;std::vector<std::string> reports;};
NativeUiDisplay native_ui_display;
// m45 frame cost (sample-profile.flag): the sampler of the owner and render worker threads starts once 1,200 world frames
// were presented (the zone loaded and settled) and stops after 30,000 samples a thread (sample_profile.h).
std::atomic<uint64_t> native_sample_frames{},native_sample_active{};
// I-0074: the frame mark of each sample (the frames finished so far, FinishNativeFrame), and whether the loader threads met at
// a texture or model admission are added to the sampler (SampleLoaderThread).
std::atomic<uint64_t> native_sample_mark{};std::atomic<bool> native_sample_loaders{};
// Optimisation review: with sample-profile-load.flag the sampler starts at the first world frame instead (the zone load's
// first frames, where the play hitches are), and takes 75,000 samples a thread (I-0074: a zone walk after its load).
uint64_t native_sample_start_frame=1200;
// I-0097: sample-ds3-threads.flag adds every thread DS3 itself started (its start address in the game's image: its task
// workers, whose phases the engine thread joins) to a running sampler, as far as the added table goes; the threads added.
// Tranche 4: the native sampler and the original's (Reference) both; the threads the sampler started with are skipped.
uint32_t AddDs3SampledThreads(std::span<const sampling::Thread> started) noexcept {
    using Query=LONG(NTAPI*)(HANDLE,ULONG,PVOID,ULONG,PULONG);
    const auto query=reinterpret_cast<Query>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"NtQueryInformationThread"));
    const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(image);const auto image_end=image+reinterpret_cast<const IMAGE_NT_HEADERS64*>(image+dos->e_lfanew)->OptionalHeader.SizeOfImage;
    const HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);uint32_t added{};THREADENTRY32 entry{sizeof entry};
    if(query && snapshot!=INVALID_HANDLE_VALUE){for(BOOL more=Thread32First(snapshot,&entry);more;more=Thread32Next(snapshot,&entry)){
            if(entry.th32OwnerProcessID!=GetCurrentProcessId() ||
               std::any_of(started.begin(),started.end(),[&](const sampling::Thread& t){return t.id==entry.th32ThreadID;}))continue;
            if(const HANDLE thread=OpenThread(THREAD_QUERY_INFORMATION,FALSE,entry.th32ThreadID)){uintptr_t start{};
                if(query(thread,9,&start,sizeof start,nullptr)>=0 && start>=image && start<image_end && sampling::AddThread(entry.th32ThreadID,"ds3"))++added;   // 9: ThreadQuerySetWin32StartAddress
                CloseHandle(thread);}}}
    if(snapshot!=INVALID_HANDLE_VALUE)CloseHandle(snapshot);
    return added;
}
// The programme analyses kept by content (AnalyzeNativeProgram): reuses, analyses run and their QPC ticks.
std::atomic<uint64_t> native_program_analysis_hits{},native_program_analysis_misses{},native_program_analysis_ticks{};
// I-0100 tranche 7 (crossing-observe.flag): a layout's programme set built (AcquireNativeModelPrograms past its kept choice), its
// QPC ticks by part: 0 the authored material, D24430 and the effect; 1 the effect found by name; 2 the layout's lock; 3 the
// parameters, texture bindings and policy; 4 the numeric parameters; 5 the recipes' selection; 6 their analyses; 7 the block
// plans; 8 the texture plans.
std::array<std::atomic<uint64_t>,9> native_programs_build_ticks{};std::atomic<uint64_t> native_programs_builds{},native_programs_kept{},native_recipe_sets_taken{};
// The encoded texture admissions (AdmitEncodedTextureFamily): count, container bytes read, decode ticks, those on the owner thread.
std::atomic<uint64_t> native_texture_decodes{},native_texture_decode_bytes{},native_texture_decode_ticks{},native_texture_decodes_owner{};
// I-0074: the whole encoded admissions (container copy, decode, upload recording, metadata): QPC ticks and the longest one.
std::atomic<uint64_t> native_texture_admit_ticks{},native_texture_admit_max{};
// I-0100 tranche 8 (popin-observe.flag, POPIN-STUDY M2): the encoded textures DS3 created so far (both renderers: the native's
// admissions, the original's 2B260/AC110 passed through), and the QPC tick the last loading page's window was asked to close.
std::atomic<uint64_t> popin_texture_creations{};std::atomic<int64_t> popin_page_closed{};
// I-0075: the main view's rows prepared in chunks of prepare_chunk_rows on the recording pool (no-prepare-chunks.flag for
// A/B, set at startup before any prepare); the chunked prepares and their chunks so far.
constexpr size_t prepare_chunk_rows=128;
bool native_prepare_chunks_disabled{};std::atomic<uint64_t> native_prepare_chunked{},native_prepare_chunks{};
// The gparam bank publications (PublishNativeGparamBanks, under gparam_lock): count, ticks, the longest, those on the owner thread.
std::atomic<uint64_t> native_gparam_publishes{},native_gparam_publish_ticks{},native_gparam_publish_max{},native_gparam_publishes_owner{};
// R13 cR5/cR6: the world frame is prepared on the render worker (by default; inline-prepare.flag keeps it on the engine thread).
bool native_prepare_on_worker{};
// The render worker is running a frame job (the census's retirement rows report it).
std::atomic<bool> render_job_in_flight{};
// Process termination (DllMain's DLL_PROCESS_DETACH with a non-null reserved argument, before this module's static
// destructors): the engine's own statics and heaps may already be gone, so releases into engine memory are skipped and
// the process reclaims everything (PILOT D-10, phase F exit semantics; Native1367/1370: a parsed FXR buffer held by a
// static store was freed into a destroyed engine heap, a pure virtual call at 0xF14C52).
std::atomic<bool> native_process_terminating{};
// R13 cF.4: the lock order of the module (phase F sheet 3.3); a lock may be taken only after the ones above it.
//  0  render worker Post / Quiesce / idle wait (cR3): never under any lock below
//  1  ui_frame_preparation_lock -> ui_sessions_lock (UI frame, before gpu_submissions)
//  1b the engine's UI locks under it (manager+0xC38, E4C800's +0x10, each movie's +0x38/+0x90): taken by PrepareFrame on the UI
//     display worker (I-0088) beside the collect; the collect never takes them, and the owner joins that worker holding no lock
//  2  scene_view_cpu_lock (recursive; taken briefly by collect and AcquireNativeSelectedScene)
//  3  gpu_submissions (Clear, Close, Start, ReadStartupView, CloseNativeModelStreams, the UI session constructor)
//  4  model_storage_lock (Close, after gpu_submissions)
//  5  the short registry locks (model_stream, pose, policy, material, model_material, layout, cloth, shader, effect,
//     composed, gparam, sss, simulation_time, native_scene_copy, common_image, native_cube_name, native_texture_constant),
//     each taken alone (cF.6: AcquireNativeLightingPrograms no longer holds its static mutex across shader_lock)
//  6  under gpu_submissions in Clear: TextureIngress::mutex_, native_movie::lock, native_offscreen_lock, descriptor pools
//  6b I-0120 G1: native_offscreen_idle, waited on under native_offscreen_lock (released while waiting) by the offscreen retire
//     hooks, holding no other module lock, only for a collector on another thread (which takes no lock the hooks hold)
//  6c I-0120 C1: texture_registry_readers / _writers are counts, not locks: a lookup never waits; a registry mutator waits only
//     for the lookups in flight (D24490 alone, which takes no lock)
//  7  DeviceQueues: submit_mutex_ -> ingress_lock_, submit_mutex_ -> retire_lock_; ModelDrawCaches::lock -> retire_lock_,
//     -> root_shapes_lock; events_lock_ and the Programs locks are leaves
//  8  leaf: journal_lock (Log)
// CloseNativeModelStreams no longer holds model_stream_lock while it takes gpu_submissions (cF.4, PILOT D-2).
std::mutex gpu_submissions;
bootstrap::TextureIngress textures(gpu,gpu_submissions);
struct NativeCommonImages {
    std::array<std::shared_ptr<const bootstrap::NativeTextureRecord>,16> dither;   // 7ECB0's rotation noise (the SSAO's; since I-0126 R7c only [0], the raster sun shadow's)
    // I-0113 tranche 22: FF630's 4x4 A8 ordered dither and its inverse, the model programmes' g_DitherTexture t38 (13B450).
    std::array<std::shared_ptr<const bootstrap::NativeTextureRecord>,2> fade_dither;
    std::shared_ptr<const bootstrap::NativeTextureRecord> absent_decal;
    // 06C00 system textures SYSTEX_WHITE..CYAN, bound by name like D4A2D0 registers them.
    std::array<std::pair<std::wstring,std::shared_ptr<const bootstrap::NativeTextureRecord>>,14> system;
    // Forward pass explicit open policy (1114): IBL t30/t31 a black cube, local-light buffers t59..t62 empty (zero counts).
    // I-0113 tranche 10: for a group whose env-map keys are the default {0, -1} (the FFX model particles, the crystals) the
    // original's 75B30 binds the global REM cube [GXRS+4DD0]+178, else the GX common black cube +170 (index 14): black where the
    // REM is null (m40, OBSERVED). Not native yet: a REM where one exists, and the region cubes 5C0E0 selects for a world object's
    // forward rows (by its position, through 4E2A0) (RESULT, tranche 10).
    std::shared_ptr<const bootstrap::NativeTextureRecord> neutral_ibl;
    std::array<std::shared_ptr<const bootstrap::NativeTextureRecord>,4> empty_light_buffers;
};
std::mutex common_image_lock;
std::shared_ptr<const NativeCommonImages> native_common_images;
std::array<BYTE,38> native_common_image_keys{};
bool common_images_closed{};
uint32_t output_width{},output_height{};
// I-0111 tranche 3 (UPSCALER-DLSS-READY §3.1, D-0151): the world's render extent in a scaled DLSS mode, 0 when it is the output
// extent; changed with the renderer's (SetRenderExtent) by FollowNativeRenderExtent, the worker quiesced. The engine's own records
// (the scene contexts, views and camera outputs) keep the output extent; the main camera handed to the native passes takes this one.
uint32_t render_width{},render_height{};
uint32_t NativeRenderWidth() noexcept {return render_width?render_width:output_width;}
uint32_t NativeRenderHeight() noexcept {return render_height?render_height:output_height;}
bool native_output_follow_disabled{};   // I-0076: no-output-follow.flag keeps the start extent (FollowNativeOutputExtent)
bool native_display_cycle{};   // I-0076 test: display-cycle.flag applies screen modes and resolutions as the options do (StepNativeDisplayCycle)
// Fable review: the window client at start (the window-change check) and the output extent chosen at start (the engine's size S
// when it differs from the clamped client, else the client).
uint32_t start_client_width{},start_client_height{},start_width{},start_height{};
bool view_system_owned{};
bool opened{};
DWORD owner_thread{};
uint64_t serial{};
std::atomic<uint64_t> world_burst_first{},world_burst_serial{};   // I-0132 (world-burst-after-<n>.flag): the burst's first serial once known and the
// frame serial published with it (02, each frame once the burst is armed), read by camera_laws (04) from its own thread
std::optional<frame::PrimitiveWorkPacket> native_ui_frame;
bool cpu_registered{};
bool repository_ready_reported{};
bool cpu_resources_requested{};
std::atomic<bool> native_resource_consumers{};
std::mutex colour_settings_lock;
std::array<float,6> native_colour_settings{};
uint64_t native_colour_generation{};
std::mutex ui_sessions_lock;
// Sessions are closed only at engine boundaries: the genuine manager destructor
// (NativeUiManagerDestroy) or the explicit native Close(). The registry is never
// destroyed by CRT process detach, which must not call into engine CPU services
// or into native registries already destroyed in reverse static order (RCA879).
auto& ui_sessions=*new std::unordered_map<uintptr_t,std::shared_ptr<bootstrap::GameUiSession>>();
// FLVER's CPU shared-reference prefix remains engine-owned. Native buffer
// bytes/resources live separately; original GPU slots +18/+20 stay nil.
// I-0097: an allocator whose resize leaves the bytes unset; a storage's bytes are read whole at once right after (value-
// initialising them first zeroed every model's bytes once more, on DS3's loading task that the engine thread joins).
template<class T> struct UnsetAllocator:std::allocator<T> {
    template<class U> struct rebind {using other=UnsetAllocator<U>;};
    UnsetAllocator()=default;template<class U> UnsetAllocator(const UnsetAllocator<U>&) noexcept {}
    template<class U> void construct(U* p) noexcept {::new(static_cast<void*>(p)) U;}
    template<class U,class... A> void construct(U* p,A&&... a) {::new(static_cast<void*>(p)) U(std::forward<A>(a)...);}
};
struct NativeModelStorage {
    uint64_t generation{};
    uint32_t index_buffer_count{};
    std::vector<MeshSlice> meshes;
    std::vector<BYTE,UnsetAllocator<BYTE>> bytes;
    std::atomic<bool> geometry_pending{};   // I-0097: its geometry job queued (StorageQueue), the meshes not yet published
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    std::shared_ptr<RtModelBlas> blas;   // I-0117 (R2 T0): its BLAS, with the rays' scene
    bool rt_readable{};   // I-0117 T0 final review L3: its buffer left readable by the BLAS build (the flag read once, at its upload)
};
std::shared_mutex model_storage_lock;   // I-0078: shared by the admission workers' lookups
std::unordered_map<uintptr_t,std::shared_ptr<NativeModelStorage>> model_storage;
std::mutex model_storage_withdrawn_lock;std::vector<std::weak_ptr<NativeModelStorage>> model_storage_withdrawn;   // I-0096 census: destroyed storages
// 1062 witness: the CopyModelGeometry check that refused a storage's join (by storage).
std::unordered_map<uintptr_t,int> model_geometry_refusal;
uint64_t model_storage_generation{};
void ResetNativeSelectedScene();
void NoteNativeKeptDrawReset() noexcept;   // C4-2, with the kept draws
uint64_t NativeKeptDrawResets() noexcept;   // I-0100 tranche 2: the scene resets so far (a load each)
size_t ApplyPendingNativeKeptDrawReset();   // I-0096, when no world is drawn
void CloseNativeKeptDraws();
struct NativeSelectedScene;
std::shared_ptr<const NativeSelectedScene> AcquireNativeSelectedScene();
std::shared_ptr<NativeModelFrameInput> PrepareNativeModelFrame(const std::shared_ptr<const NativeSelectedScene>& scene,uint64_t presentation_serial,bool offscreen=false);
bool NativeSceneHasVelocity(const std::shared_ptr<const NativeSelectedScene>& scene) noexcept;   // cR5, after the scene type
bool NativeSceneGlow(const std::shared_ptr<const NativeSelectedScene>& scene,NativeGlowInput& glow,uint32_t& width,uint32_t& height) noexcept;   // W6 T7 row, after the scene type
void CollectNativeLightBlocks(NativeSelectedScene& scene,const std::optional<std::array<uint32_t,2>>& fixed_key={});
void CollectNativeOffscreenViews(NativeFrameJob& job) noexcept;
void CollectNativeMovieFrame(NativeFrameJob& job) noexcept;
// I-0141 tranche 17's list: the scene's own key blocks by set index (its light_keys, 05_selected_scene.inc), and a JSON number or null.
std::vector<std::array<uint32_t,10>> NativeSceneLightKeys(const std::shared_ptr<const NativeSelectedScene>& scene);
std::string PixelKeyNumber(float v);
void CollectNativeOffscreenScenes(uint64_t selection_serial);
void CloseNativeSimulationTime();
// W3 (GAMMA-COLOUR-PATH-READY 3.2-3.4): the pinned gamma-pass shaders from the image, and the curve of the current
// brightness made by the engine's own generator on the engine thread (one object per change, carried by the frame job).
bool LoadNativeDisplayBlobs(DisplayBlobs& blobs);
DisplayCurve NativeDisplayCurve();
bool native_display_source{};   // W3: the renderer has the gamma source (set once before the renderer starts)
void CloseNativeModelStreams();
void CloseNativeClothStreams();
// 1184 (MENUS-MAP 65): the current simulated stream-0 bytes of each cloth mesh, keyed by the engine's GX buffer
// object (set+0x18[mesh], descriptor +0x40). Each upload is an immutable native resource; a selected draw keeps
// the version it bound until its submission completes.
struct NativeClothStream {uintptr_t set{};uint32_t bytes{};uint64_t generation{},fingerprint{},first{};bool simulated{};Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    std::shared_ptr<const UploadVersion> version;   // cD5: the pooled buffer behind `resource` (empty on the legacy path)
    // I-0134: the copy this upload replaced (the previous simulated frame's, the one VC_PrevWorld = I+0x90 places), kept for the
    // Velo rows' previous position; its resource and version only, never its stream (no chain of earlier copies).
    Microsoft::WRL::ComPtr<ID3D12Resource> previous_resource;std::shared_ptr<const UploadVersion> previous_version;
    uint64_t frame{};};   // I-0134: native_cloth_frames at the upload (the upload pairs only the last frame's copy, the admission binds
                          // a previous copy only beside this frame's upload)
// I-0134: the engine frames ended (with lifecycle_frames, on the engine thread), read by the cloth uploads and the admissions.
std::atomic<uint64_t> native_cloth_frames{};
std::mutex cloth_stream_lock;
std::unordered_map<uintptr_t,std::shared_ptr<const NativeClothStream>> cloth_streams;
void CloseNativeFaceImages();
bool NativeDefaultBoundTextureFolded(const std::wstring& folded);   // the stem already in lower case
Microsoft::WRL::ComPtr<ID3D12Resource> AdmitNativeStaticBuffer(std::span<const BYTE> bytes,bool rt_readable=false);
// D3D12 phase D cD5 (B5): the dynamic stream versions take pooled buffers (UploadVersionPool, no creation once warm).
UploadVersionPool stream_versions;
Microsoft::WRL::ComPtr<ID3D12Resource> PublishNativeStreamBytes(const std::vector<BYTE>& bytes,std::shared_ptr<const UploadVersion>& version);
using ModelCpuLoad=uint64_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,BYTE,uint32_t);
ModelCpuLoad model_cpu_load_original{};
bool model_storage_closed{};
// I-0081: storage admissions from their closed check on; I-0097: until their queued jobs (StorageQueue) have finished.
std::atomic<uint32_t> model_storage_admitting{};
// I-0097 (after ER: loading work published when complete, never joined by the engine thread): a model storage's GPU buffer and
// its geometry check run here, off DS3's loading task, which the engine thread joins in its frame phases (the path's slow
// frames: those admissions were about half that task's time, i97-prof-3). The queue is in order, so a storage's buffer exists
// before its geometry job publishes the meshes; a model met before then is drawn from a later frame (D-0197). Normal priority
// (step review: a lower one could be preempted while it holds the registry lock the collect reads under). Jobs are counted in
// model_storage_admitting until they finish; the post-to-publish delay is kept for the census row (the backlog after a load).
TaskQueue& StorageQueue() {static auto* queue=new TaskQueue("storage");return *queue;}
std::atomic<uint64_t> storage_queue_delay_max_us{};
frame::PrimitiveProgramBank primitive_programs;
uint64_t lifecycle_frames{};
struct NativeShader { std::vector<BYTE> bytes; UINT version{}; uint64_t generation{}; };
std::mutex shader_lock;
std::unordered_map<uintptr_t,std::shared_ptr<const NativeShader>> shaders;
uint64_t shader_generation{};
// Optimisation review: every admission or retirement of a shader (FindLoadedProgramme keeps its answers while this holds).
std::atomic<uint64_t> shader_registry_changes{1};
struct EffectProgram { std::wstring name;uint32_t kind{};std::shared_ptr<const NativeShader> program; };
struct NativeEffect { uint64_t generation{};std::vector<BYTE> archive,declarations;std::vector<EffectProgram> programs;
    std::vector<ds3content::ModelProgramName> program_names;
    std::vector<ds3content::ModelParameterDeclaration> parameter_declarations;
    std::vector<ds3content::ModelTextureDeclaration> texture_declarations; };
std::mutex effect_lock;
std::unordered_map<uintptr_t,std::shared_ptr<const NativeEffect>> effects;
uint64_t effects_changes{};   // I-0100 tranche 7: inserts and erasures of `effects`, under effect_lock (the by-name answers' key)
uint64_t effect_generation{};
// Workstream K P6 (K-COLLECT-IMPLEMENTATION-SHEET 2.6): bumped after every change of the registries a model's programme
// choice reads (shaders, effects, materials, composed groups); a choice made under one value stays valid while it holds.
std::atomic<uint64_t> program_registry_generation{1};
void BumpProgramRegistry() noexcept {program_registry_generation.fetch_add(1,std::memory_order_acq_rel);}
// C7-2 (PERSISTENT-RECORDS-DESIGN 1): bumped after every insert or erase in the registries a model record's meshes were
// resolved from (materials, layouts, model storage); a record made under one value is checked only while it holds.
std::atomic<uint64_t> model_registry_epoch{1};
void BumpModelRegistry() noexcept {model_registry_epoch.fetch_add(1,std::memory_order_acq_rel);}
struct NativeSfxAsset {
    uint64_t generation{};
    uint32_t identifier{};
    std::wstring name;
    std::vector<BYTE> bytes;
    std::shared_ptr<void> parsed;
};
// Latest unavailable optional visual request, not a complete frame event queue.
// Its immutable input retains genuine FXR content for the existing host source.
struct NativeSfxDeferredRequest {
    uint64_t generation{};
    uint32_t identifier{},flags{};
    std::array<BYTE,32> transform{};
    std::shared_ptr<const NativeSfxAsset> asset;
};
uint64_t sfx_deferred_request_generation{};
std::mutex sfx_asset_lock;
std::unordered_map<uint32_t,std::shared_ptr<const NativeSfxAsset>> sfx_assets;
std::unordered_map<uintptr_t,std::shared_ptr<const NativeSfxAsset>> sfx_asset_records;
using SfxCpuConstruct=uintptr_t(__fastcall*)(uintptr_t);
SfxCpuConstruct sfx_cpu_construct_original{};
uint64_t sfx_asset_generation{};
bool sfx_cpu_types_initialized{};
// Native SFX manager (930-938, LEGACY-ROOT-INVENTORY): the engine's own SFX
// manager class (vtable 2992EF8, 0xF0) published at 4799C98 with only the CPU
// members of E6C9D0. Omitted, and tested by the engine before use (the GX effect
// system +0x58, 24B260/24C530, published natively since W4 FW-9 by default, effect_system.cpp) with its resource handlers +0x38..+0x50/+0xD8 and
// GX flags, the view draw list +0x60, the debug view +0xE0 and the debug-menu
// registration E6F630. Built since I-0141 13d: the stop table +0x68/4773698. The two methods that pass
// +0x58 unconditionally to GX (view attach E6EA10, detach E6EC60) are native.
uintptr_t native_sfx_manager{};
uint64_t sfx_scene_cpu_reset_generation{},sfx_view_attachments{};
// W4 FW-2 (FFX-WAVE1-READY section 6): the GX effect system without GX statements (effect_system.cpp), built right after the
// manager and published (FW-3); since FW-9 the default, no-native-effects.flag (runner -EffectFlags no-native-effects) disables W4.
effect_system::System native_effect_system{};
// W4 (FW-9): read once in Setup; no-native-effects.flag turns W4 off for the whole run (review: one decision per run).
bool native_effects_enabled{};
bool native_ffx_draw{};   // FFX-DRAW: the effect draw side (set once at setup: native mode, effect system on, no no-ffx-draw.flag)
// Published state read on the loader and retirement threads: stored with release after +0x58 (review).
std::atomic<bool> native_effect_published{};
// FUN_1419254F0, the GX buffer creation: on the effect system's device token it makes a native record (effect_system::
// CreateBuffer); any other device reaches the original, as before (natively no other caller exists: the GX device is absent).
using GxBufferCreate=uint32_t(__fastcall*)(uintptr_t,uintptr_t,const uint32_t*,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
GxBufferCreate gx_buffer_create_original{};
uint32_t __fastcall NativeGxBufferCreate(uintptr_t device,uintptr_t out,const uint32_t* description,uintptr_t initial,
    uintptr_t a5,uintptr_t a6,uintptr_t a7) noexcept {
    if(effect_system::IsEffectDevice(device))return effect_system::CreateBuffer(out,description,initial);
    return gx_buffer_create_original(device,out,description,initial,a5,a6,a7);
}
// FUN_141925BF0 / FUN_141925B10 (shader / unordered view of a GX resource): on the effect device, a native view record.
using GxViewCreate=uint32_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,const uint32_t*,uintptr_t);
GxViewCreate gx_shader_view_original{},gx_unordered_view_original{};
uint32_t __fastcall NativeGxShaderView(uintptr_t device,uintptr_t out,uintptr_t resource,const uint32_t* description,uintptr_t a5) noexcept {
    if(effect_system::IsEffectDevice(device))return effect_system::CreateView(2,out,resource,description);
    return gx_shader_view_original(device,out,resource,description,a5);
}
uint32_t __fastcall NativeGxUnorderedView(uintptr_t device,uintptr_t out,uintptr_t resource,const uint32_t* description,uintptr_t a5) noexcept {
    if(effect_system::IsEffectDevice(device))return effect_system::CreateView(3,out,resource,description);
    return gx_unordered_view_original(device,out,resource,description,a5);
}
// FUN_141925210 / FUN_1419250A0 (shader programme from bytecode: device, out, bytecode, size, ...; the second with a stream
// output declaration): on the effect device, a native programme record.
using GxProgramCreate=uint32_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uint64_t,uintptr_t,uintptr_t,uintptr_t);
using GxStreamProgramCreate=uint32_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uint64_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
GxProgramCreate gx_program_create_original{};
GxStreamProgramCreate gx_stream_program_create_original{};
uint32_t __fastcall NativeGxProgramCreate(uintptr_t device,uintptr_t out,uintptr_t bytes,uint64_t size,uintptr_t a5,uintptr_t a6,uintptr_t a7) noexcept {
    if(effect_system::IsEffectDevice(device))return effect_system::CreateProgram(out,bytes,static_cast<uint32_t>(size));
    return gx_program_create_original(device,out,bytes,size,a5,a6,a7);
}
uint32_t __fastcall NativeGxStreamProgramCreate(uintptr_t device,uintptr_t out,uintptr_t bytes,uint64_t size,uintptr_t a5,uintptr_t a6,uintptr_t a7,uintptr_t a8) noexcept {
    if(effect_system::IsEffectDevice(device))return effect_system::CreateProgram(out,bytes,static_cast<uint32_t>(size));
    return gx_stream_program_create_original(device,out,bytes,size,a5,a6,a7,a8);
}
// FUN_1400D8780 (deferred release through the display's queue +0x2B8) and FUN_14019AD90 (release of a GX resource): an effect
// object is dropped natively; anything else reaches the original.
using GxDeferredRelease=uint64_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t);
using GxResourceRelease=void(__fastcall*)(uintptr_t);
GxDeferredRelease gx_deferred_release_original{};
GxResourceRelease gx_resource_release_original{};
uint64_t __fastcall NativeGxDeferredRelease(uintptr_t queue,uintptr_t object,uintptr_t tag) noexcept {
    if(effect_system::IsEffectObject(object)){const auto creation=effect_system::Serial(object);if(!tag || !effect_system::Release(object))return 0;effect_draw::Forget(object,creation);return 1;}
    return gx_deferred_release_original(queue,object,tag);
}
void __fastcall NativeGxResourceRelease(uintptr_t object) noexcept {
    if(effect_system::IsEffectObject(object)){const auto creation=effect_system::Serial(object);if(effect_system::Release(object))effect_draw::Forget(object,creation);return;}
    gx_resource_release_original(object);
}
// The path of a file next to this module (an empty name gives the folder), empty if the module's path cannot be read: the one
// lookup of the module's folder (I-0102, before it 17 copies).
std::wstring ModuleSibling(const wchar_t* name) noexcept {
    std::array<wchar_t,MAX_PATH> module{};HMODULE self{};
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&ModuleSibling),&self))return {};
    if(const auto chars=GetModuleFileNameW(self,module.data(),MAX_PATH);!chars || chars>=MAX_PATH)return {};   // review L3: a truncated path is no path
    std::wstring path(module.data());return path.substr(0,path.find_last_of(L"\\/")+1)+name;
}
// Security audit M1 (Thomas, 2026-11-23): the witness files (witness-*.txt beside the module: the player's position, phantom, event
// flags, cutscenes, all kept in the save) are read only in a bounded run (a runner's controller, 1 to 7200 s), never in a play session
// (both launchers run the controller unbounded), so a file arriving beside the play module during play cannot change the save.
// The bound is read once in Setup, before any hook (the section stays writable by the game and the user's other processes).
bool bounded_run{};
bool WitnessToolsAllowed() noexcept {return bounded_run;}
// I-0141 tranche 26 (test tooling, a bounded run only): the native shares a cutscene hold's runner switches off live, so every variant
// is shot at the same held moment of one run (witness-variant.txt, FollowWitnessShareVariant): bit 1 the cutscene sets' lights 1 to
// 3 (D-0279, D-0282: NativeGparamRecords::Authored false, D-0262's rule), bit 2 the glare family served empty (the bloom and the
// shafts, as the library's own skip; the depth of field and the blur kept), bits 4, 8 and 16 the traced rays' sky misses, the
// cache's ambient and the hits' local lights (kRtReflectGiNoSky, NoAmbient, NoLocal: R7a's diagnostics, ORed directly by EncodeRtGi,
// EncodeRtNrd (the GI's and the specular rays on the frames NRD denoises, RR off) and EncodeRtCache only; the probes, the water and
// the specular on the frames NRD does not denoise (RR's, or NRD refused) never get them: they keep their own misses' sky and their hits'
// local lights, and read cache cells built under the bits once the cache has turned over (the final review's H1, the step reviews' M1
// and L4). At the PT level bit 16 does nothing (HitLight's PT branch samples one local light whatever the bit). 0 outside a bounded run.
std::atomic<uint32_t> witness_share_variant{};
// I-0141 tranche 27: bit 32 the traced diffuse ambient not composed (EncodeRtGi), bit 64 the specular ambient (SpecularAcc, the traced
// reflections) cleared, the main view's deferred surfaces alone (scene_pass.h's share_variant_off): the forward rows keep their probes'
// SH and their cubes, which the original's bit 64 takes from every draw its hooks reach and bit 32 from the pass kinds 0, 3 and 4 (04c;
// the second step review's M1, the third's L1).
// I-0141 tranche 28: bits 128 and 256, natively the sets' lights 1 to 3 and light 0 (the sun) out of the traced hits' light (kRtReflectHitNoFill,
// HitNoSun: ORed as bits 4 to 16, so the same passes: the GI's rays, the specular rays on NRD's frames, the cache's update), the pixels'
// own direct light kept: the fill lights' and the sun's bounce apart. Bit 512, in a Reference run only, changes no picture: while it is
// set, 04c's census logs once per (model, pass kind) the instance SH rows 5B3D0 wrote, with their source (reference_census_epoch: a new
// census each file read that sets it).
constexpr uint32_t kShareNoFill=1,kShareNoGlare=2,kShareNoSky=4,kShareNoAmbient=8,kShareNoLocal=16,kShareNoGi=32,kShareNoSpecular=64,
    kShareHitNoFill=128,kShareHitNoSun=256,kShareCensus=512,kShareAll=511;
std::atomic<uint32_t> reference_census_epoch{};
// I-0141 tranche 27: the same variant in a Reference run (reference-share-variant.flag, 04c's ReferenceShare* hooks): the original's
// own shares, bits 1, 2, 32 and 64 (the original has no rays). Set by Setup once its hooks are made; the hooks' counts by path, printed in
// each witness_share_variant row (cumulative), say which paths a variant reached: 0 the deferred entries 1496D0 built for 137910, 1 those
// the blend 14A2D0 built, 2 the forward sets 149CB0 built, 3 those the blend 14AB20 built, 4 the 1496D0 calls of other callers (the
// effects' 264230, 32650's array: left whole), 5 the glare shape stores made 0, 6 the forward records not read whole (left as they are),
// 7 to 9 the ambient's (bits 32 and 64, 04c: the instance records' SH, their cube scales, the region passes emptied).
bool reference_share_variant{};
std::array<std::atomic<uint64_t>,10> reference_share_counts{};
bool ModuleFlag(const wchar_t* name) noexcept {
    const auto flag=ModuleSibling(name);return !flag.empty() && GetFileAttributesW(flag.c_str())!=INVALID_FILE_ATTRIBUTES;
}
// W10 closure (YEBIS-WAVE1-READY 7, rule 1): our YEBIS passes are the default once witnessed at the five zones plus m30 and m40. The
// feed (pass "") and each pass (tonemap, glare, shafts, dof, motionblur, grading) are on unless no-yebis.flag (all of them) or
// no-yebis-<pass>.flag (that one) lies next to the module; the earlier opt-in files (yebis.flag, yebis-<pass>.flag) change nothing.
bool NativeYebisPass(const wchar_t* pass) noexcept {
    if(ModuleFlag(L"no-yebis.flag"))return false;
    if(!*pass)return true;
    return !ModuleFlag((std::wstring(L"no-yebis-")+pass+L".flag").c_str());
}
struct NativeMaterial {
    uint64_t generation{},incarnation{};
    std::vector<BYTE> bytes;
    std::wstring name,shader;
    // D40790's CPU texture indices and authored parameter values. Excludes
    // the subscription, strings and original GPU association at +248..+290.
    std::array<BYTE,0x210> values{};
    std::vector<ds3prod::MtdParam> parameters;
    std::vector<ds3content::ModelNumericParameter> numeric_parameters;
    std::vector<std::string> unsupported_parameters;
};
std::mutex material_lock;
std::unordered_map<uintptr_t,std::shared_ptr<const NativeMaterial>> materials;
uint64_t material_generation{};
struct NativeModelMaterial {
    std::array<uint32_t,8> description{};
    std::wstring name,mtd;
    std::shared_ptr<const NativeMaterial> authored;
    uintptr_t authored_identity{};
    bool engine_default_mtd{};
    // Exact FLVER texture records and parameters, kept outside old GPU state.
    std::vector<ds3content::ModelTextureInput> texture_inputs;
    std::vector<uint8_t> gx_records;
    std::shared_ptr<ds3content::TextureScrollOwner> scroll;
    // m50 sky (Fable review 2026-10-17, 141070's helpers 22BEC0/22BD90): the MTD's g_DiffuseColor PoweredColor parameter, the
    // provider 22E930 reads at +0x360 (colour +0x30 = vt+0x70, power +0x50, and the two bytes +0x54 and +0x41, taken here as the
    // MTD bools g_DiffuseColor_UseCAParam and g_DiffuseColorPower_UseCAParam: INFERRED from the AllMaterialBnd data, the
    // PoweredColor layout not read);
    // natively no parameter object exists, its values are kept here.
    struct DiffuseProvider {std::array<float,3> colour{1,1,1};float power{1};bool use_colour{},use_power{};};
    std::optional<DiffuseProvider> diffuse_provider;
    // I-0078: the engine material's record bytes as last read (NativeMaterialRecordBytes), shared by the model records using it.
    struct RecordBytes {std::mutex lock;uintptr_t address{};uint64_t serial{};std::shared_ptr<const std::vector<uint8_t>> bytes;};
    mutable RecordBytes record_bytes;
    // I-0085: the layout prefixes built for this material (BuildModelLayout, under model_layout_lock), retired with it.
    std::vector<uintptr_t> layouts;
    // I-0101 (STUDY §2): what the original's slot 7 returns, its GX MTD object (gx_mtd_image.h), built with the material;
    // never stored at +0x80 (FUN_140140560 releases that through a reference count).
    std::shared_ptr<bootstrap::GxMtdImage> gx_mtd;
};
// I-0077: shared by the readers (the admission workers' lookups and every reused record's material compare, i74-native-61:
// the workers queued on it), exclusive for an insert, an erase and the layout-material handoff.
std::shared_mutex model_material_lock;
std::unordered_map<uintptr_t,std::shared_ptr<NativeModelMaterial>> model_materials;
struct NativeSimulationTime {uint64_t generation{1};float now{};};
std::mutex simulation_time_lock;
std::shared_ptr<const NativeSimulationTime> native_simulation_time;
ds3content::EngineSamplerBankOwner native_sampler_bank;
// O2 (options sheet section 4, M-6): the graphics options of a frame, read once by the render step on the owner thread
// (UpdateNativeOptions, where 23790's comparison with the applied copy is made) and published as an immutable value. Each
// collect attaches the current one to its scene, and the consumers (sampler bank, MotionBlur row, Shade gate, point-light
// fade) read the scene's copy, never the live block: one source, one moment, one validation.
struct NativeOptionSnapshot {
    uint64_t generation{};
    std::array<int32_t,12> levels{};                              // 0..4, slot 8 the applied copy (no file key)
    std::array<uint8_t,bootstrap::QualityObjectBytes> object{};   // [GXRS+0x5360] as written for those levels
    bool object_valid{};                                          // false until the root's object exists
    uint32_t configured_width{},configured_height{};              // the configured render size S (image+0x46601E8)
    std::shared_ptr<const ds3content::EngineSamplerBankVersion> sampler_bank;   // the Texture level's bank (O1: last valid kept)
};
std::mutex native_options_lock;
std::shared_ptr<const NativeOptionSnapshot> native_options;
uint64_t native_options_generation{};   // under native_options_lock
std::shared_ptr<const NativeOptionSnapshot> AcquireNativeOptions(){const std::lock_guard hold(native_options_lock);return native_options;}
using SimulationTimeStep=void(__fastcall*)(uintptr_t,uintptr_t);
SimulationTimeStep simulation_time_original{};
// Programme recipe index: Gbuf (shader pass 6) 0-2, Gbuf2 (7) 3-5, Fwd (0) 6-8, Velo (1, 1186) 9-11, by pixel variant;
// D-0180 C2: Dpt (shader pass 2) 20 and its alpha-tested DptA 21. WATER-TESS-WAVE1-READY WT-1: WatDpt (shader pass 12) 22.
constexpr size_t native_recipe_count=23;
// C5: pseudo shader pass of the alternate set's forward recipes (<family>_Phantom_Fwd[_[A]], recipes 12-13).
constexpr uint32_t native_phantom_forward_pass=0x100;
constexpr uint32_t NativeRecipe(uint32_t shader_pass,uint32_t pixel) noexcept {
    return shader_pass==native_phantom_forward_pass?12+pixel:shader_pass==0?6+pixel:shader_pass==1?9+pixel:shader_pass==2?20+(pixel?1:0):shader_pass==12?22:(shader_pass-6)*3+pixel;}
// W7 TS-2b (WATER-TESS-WAVE1-READY 3.4): the tessellated (`_PN`, deformation 3) recipes of shader passes 6 (GBuf_PN) 14-16
// and 0 (Fwd_PN) 17-19, by pixel variant; native_recipe_count for any other pass (Gbuf2 and Velo have no `_PN` programme).
constexpr uint32_t NativeTessRecipe(uint32_t shader_pass,uint32_t pixel) noexcept {
    return pixel>2?uint32_t(native_recipe_count):shader_pass==6?14+pixel:shader_pass==0?17+pixel:uint32_t(native_recipe_count);}
constexpr bool NativeTessellatedRecipe(size_t recipe) noexcept {return recipe>=14 && recipe<20;}
// W5e TS-1 / W7: native-tess.flag releases the W0a-6 flat fallback (read once).
bool NativeTessellationEnabled() noexcept {static const bool enabled=ModuleFlag(L"native-tess.flag");return enabled;}
// I-0111 (R1), tranche 4 step 2 (Thomas, 2026-10-30 23:41: « Oui dans l'option anti-aliasing »): Streamline is loaded at every
// start (its signed DLLs beside the module; without them, or refused, DLSS is only off), unless no-dlss.flag (dlss.flag, the
// earlier runs' flag, changes nothing now). A frame runs DLSS when Streamline is ready, a DLSS mode is chosen (NativeDlssChosen) and
// the game's own Antialiasing option is on (NativeDlssActive). I-0132 R11j (D-0265): with the option on and no DLSS running, the TAA
// (NativeTaaActive, unless no-taa.flag or its latch); no anti-aliasing where neither resolves a frame (I-0138 tranche 5c, Thomas
// 2026-11-14: DS3's own filter is removed) nor with the option off. While a frame runs DLSS or the TAA the main world camera jitters (Halton (2, 3), ceil(8 (output / render)^2) phases,
// 8 for the TAA); every other view keeps none.
bool NativeDlssRequested() noexcept {static const bool on=!ModuleFlag(L"no-dlss.flag");return on;}
// The option: the quality object's AA byte ([[[0x144961B50]+0x5360]+8], the gate of the original's own filter) read once a frame in BeginFrame on
// the engine thread (FollowNativeDlssOption). The engine thread's reads (the jitter, the render extent, the velocity and the mip
// bias) take it from there; the frame's own decision travels with its selected scene (NativeSelectedScene::dlss, final review M1),
// which the prepare on the render worker reads instead of this value.
std::atomic<bool> native_dlss_active{};
// Step review L-a: the engine thread's reads within a frame take only the value BeginFrame set (it includes Streamline's readiness
// then); a latch during the frame takes effect at the next BeginFrame. The prepare and the renderer check DlssReady themselves.
bool NativeDlssActive() noexcept {return native_dlss_active.load(std::memory_order_relaxed);}
// I-0132 R11j: the TAA on this engine frame (set with DLSS's state in BeginFrame, FollowNativeDlssOption).
std::atomic<bool> native_taa_active{};
bool NativeTaaActive() noexcept {return native_taa_active.load(std::memory_order_relaxed);}
// I-0140 tranche 40 (D-0305, Thomas 2026-11-22): the menu views' own anti-aliasing on this engine frame, whenever the main view has
// one (the option on, the TAA or a DLSS mode running): the TAA per view (one history a view context, the view's own jitter and camera
// motion), DLAA per view instead while a DLSS mode runs (the view's own Streamline viewport); none with the option off.
// no-menu-taa.flag (a test tool) and the renderer's latch (MenuTaaFailed) leave the views without it, as before. Set in BeginFrame with the TAA's state (FollowNativeDlssOption).
std::atomic<bool> native_menu_taa_active{};
bool NativeMenuTaaActive() noexcept {return native_menu_taa_active.load(std::memory_order_relaxed);}
bool NativeMenuTaaRequested() noexcept {static const bool off=ModuleFlag(L"no-menu-taa.flag");return !off;}
// I-0140 tranche 41 (D-0306): a menu view's TAA reconstruction (TaaParams::kernel, exp(-k d^2) over the 3x3), chosen once each time
// the view is listed (an opening): kMenuTaaKernel, 8 (RESULT, tranche 41: the Status body's sharpness against the original's DLAA
// rises from 0.72 at 2.29 to about 0.78 at 6 and no further at 8 or 12, the portrait's up to 8-12, two openings a kernel; the films'
// frame-to-frame change per unit of gradient at lag 1 about 20 % above 2.29's, at lag 8 0 to 15 %: the jitter's, INFERRED; not
// compared with the original's DLAA). menu-taa-kernel.txt (a test tool, the calibration's): coefficients
// separated by spaces, taken in turn by the listings of the views of one extent (the menu collect's count, `listing`).
constexpr float kMenuTaaKernel=8.0f;
float NativeMenuTaaKernel(uint32_t listing) {
    static const std::vector<float> values=[]{std::vector<float> v;std::wifstream file(ModuleSibling(L"menu-taa-kernel.txt"));float x{};
        while(file && file>>x)if(x>0.0f && x<=30.0f)v.push_back(x);return v;}();   // at most 30: above about 40 every weight of a far phase falls under the resolve's 1e-6 (its final review's L2)
    return values.empty()?kMenuTaaKernel:values[listing%values.size()];}
// I-0138 tranche 9 (the TAA's static detail, I-0140 tranche 25): the main view's TAA reconstruction and its material mip bias, test
// tools of the calibration: taa-kernel.txt (one coefficient, (0, 30]: TaaParams::kernel) and taa-mip-bias.txt (one bias, [-2, 0]);
// without them the stage's 2.29 and the TAA frames' -0.5 (NativeMaterialBank); taa-no-jitter.flag (the main camera unjittered under the
// TAA, ApplyUpscalerJitter); taa-reset-always.flag (every main-view TAA frame a reset, its own sample when unjittered: the resolve
// without its history, to split the history path's loss from the frame's). One row names the files and flags this run found; they
// act only under the main view's TAA (without DLSS): under DLSS the bias applied is native_material_mip_bias's and the camera jitters,
// without the TAA nothing is applied.
struct NativeTaaCalibration {float kernel{};float mip_bias{-0.5f};bool kernel_file{},bias_file{},no_jitter{},reset_always{};};
const NativeTaaCalibration& NativeTaaTest() noexcept {
    static const NativeTaaCalibration value=[]{NativeTaaCalibration c;float x{};
        {std::wifstream file(ModuleSibling(L"taa-kernel.txt"));if(file && file>>x && x>0.0f && x<=30.0f){c.kernel=x;c.kernel_file=true;}}
        {std::wifstream file(ModuleSibling(L"taa-mip-bias.txt"));if(file && file>>x && x>=-2.0f && x<=0.0f){c.mip_bias=x;c.bias_file=true;}}
        c.no_jitter=ModuleFlag(L"taa-no-jitter.flag");c.reset_always=ModuleFlag(L"taa-reset-always.flag");
        char row[256]{};sprintf_s(row,"{\"event\":\"native_taa_calibration\",\"kernel\":%.4f,\"kernel_file\":%s,\"mip_bias\":%.4f,\"bias_file\":%s,\"no_jitter\":%s,\"reset_always\":%s}\n",
            c.kernel>0.0f?c.kernel:2.29f,c.kernel_file?"true":"false",c.mip_bias,c.bias_file?"true":"false",c.no_jitter?"true":"false",c.reset_always?"true":"false");Log(row);
        return c;}();
    return value;}
// I-0132 R12b (D-0265, D-0266): the player's settings, rekindled-settings.ini beside the module (rekindled_settings.h: its grammar,
// its defaults, D-0265's rules), written by the play launcher (and R12c's page), read once at start by the first reader below. Each
// reader keeps its test flags first (the runners' folders have no such file: the defaults there); the values are the requested ones,
// as the one-word files were (the renderer and Streamline still decide what runs; the model's Effective() is the page's, R12c). One
// row names the file's state (native_settings, NativeSettingsRow). The old one-word files are no longer read.
bool native_settings_present{};uint32_t native_settings_bytes{};
const ds3native::settings::ParseResult& NativeSettingsFile() noexcept {
    static const ds3native::settings::ParseResult file=[]{
        const auto path=ModuleSibling(L"rekindled-settings.ini");FILE* handle{};std::string text;
        if(!path.empty() && !_wfopen_s(&handle,path.c_str(),L"rb") && handle){native_settings_present=true;char bytes[4096];size_t n{};
            while(text.size()<65536 && (n=fread(bytes,1,sizeof(bytes),handle))>0)text.append(bytes,n);fclose(handle);}
        native_settings_bytes=static_cast<uint32_t>(text.size());
        return ds3native::settings::Parse(text);}();
    return file;}
const ds3native::settings::Settings& NativeSettings() noexcept {return NativeSettingsFile().settings;}
// The row: whether the file was found, its keys accepted and refused, its unknown lines, its values and what the model's rules would
// run from them (« rules », Effective(): the page's view, not what the renderer runs today, which takes the requested values; the
// final review's L5. The test flags are not in it: each reader's row names its own source).
std::string NativeSettingsRow() {
    namespace st=ds3native::settings;const auto& file=NativeSettingsFile();
    const auto list=[](const std::vector<std::string>& keys){std::string out="[";for(size_t i=0;i<keys.size() && i<32;++i){if(i)out+=',';out+='"';out+=keys[i];out+='"';}return out+"]";};
    const auto values=[](const st::Settings& s){char text[640]{};
        sprintf_s(text,"{\"level\":\"%s\",\"bounces\":%u,\"dlss\":\"%s\",\"rr\":%s,\"reflex\":\"%s\",\"frame_generation\":%u,\"taa\":%s,\"sharpen\":%.3f,\"frame_rate\":%.0f,\"vsync\":%s,\"borderless\":%s,\"vignette\":%s,\"hdr\":%s,\"hdr_peak\":%u,\"hdr_paper_white\":%u,\"look\":\"%s\",\"aberration\":%s,\"bloom\":%u,\"shafts\":%s,\"grain\":%u,\"fov\":%u}",
            st::Name(s.level),s.bounces,st::Name(s.dlss),s.rr?"true":"false",st::Name(s.reflex),s.frame_generation,s.taa?"true":"false",double(s.sharpen),double(s.frame_rate),
            s.vsync?"true":"false",s.borderless?"true":"false",s.vignette?"true":"false",s.hdr?"true":"false",s.hdr_peak,s.hdr_paper_white,
            st::Name(s.look),s.aberration?"true":"false",s.bloom,s.shafts?"true":"false",s.grain,s.fov);return std::string(text);};   // the last six: I-0138 tranche 7
    char head[160]{};sprintf_s(head,"{\"event\":\"native_settings\",\"file\":%s,\"bytes\":%u,\"unknown\":%u,",native_settings_present?"true":"false",native_settings_bytes,file.unknown);
    return std::string(head)+"\"accepted\":"+list(file.accepted)+",\"refused\":"+list(file.refused)+",\"values\":"+values(file.settings)+",\"rules\":"+values(st::Effective(file.settings))+"}\n";}
// I-0132 R12f (Thomas, 2026-11-11 06:21 and 06:22: « ça peut pas se faire en direct ? », « c'est pas possible de tout faire en temps
// réel ? »): the settings live. The copy the page edits (EditNativeSettings, 02b, on the engine thread, once the file is written) is
// what the readers below take each time they are asked, so a row's change reaches the next engine frame (BeginFrame's followers:
// FollowNativeDlssOption, FollowNativeReflex, FollowNativeLiveSettings, the render extent) without a relaunch. Each reader's test flags
// still come first and are read once (the runs' tools stay fixed for the session). A short lock: no reader waits for the disk.
std::mutex native_settings_lock;
ds3native::settings::Settings& NativeSettingsLiveLocked() noexcept {static ds3native::settings::Settings live=NativeSettings();return live;}
ds3native::settings::Settings NativeSettingsCurrent() noexcept {const std::lock_guard hold(native_settings_lock);return NativeSettingsLiveLocked();}
bool NativeTaaRequested() noexcept {static const bool off=ModuleFlag(L"no-taa.flag");return !off && NativeSettingsCurrent().taa;}
// I-0113 tranche 39 (STUDY-REACTIVE-MASK): the TAA's reactive weighting, always on; no-taa-reactive.flag, a test switch, leaves it out.
bool NativeTaaReactive() noexcept {static const bool off=ModuleFlag(L"no-taa-reactive.flag");return !off;}
// I-0113 tranche 41: taa-reactive-view.flag, a test view: the reactive weight painted red in the TAA's output.
bool NativeTaaReactiveView() noexcept {static const bool on=ModuleFlag(L"taa-reactive-view.flag");return on;}
// Thomas (2026-11-10 04:28): the sharpening after the TAA (and after DLSS, Thomas 2026-11-11 01:05, I-0132 R12c), 0 (none, the default) to 1: sharpen-50.flag and sharpen-100.flag (the
// test runs), else the settings' sharpen (0 to 1, clamped; R12b), live (R12f).
float NativeSharpen() noexcept {
    static const float flag=ModuleFlag(L"sharpen-100.flag")?1.0f:ModuleFlag(L"sharpen-50.flag")?0.5f:-1.0f;
    return flag>=0?flag:NativeSettingsCurrent().sharpen;}
// I-0123 RR-a: Ray Reconstruction for the session (set at startup with the rays unless no-rr.flag; R12f: followed live,
// FollowNativeLiveSettings), and on this frame: DLSS active and Streamline's RR ready (the renderer's or Streamline's latch clears it);
// its frames jitter over 32 phases at least (RR's guide §3.6) and render at RR's own optimal extent.
std::atomic<bool> native_rr_session{},native_rr_active{};   // step review L5: the active state taken with DLSS's at the frame's start
// The player's option (Thomas, 2026-11-03: « ça doit rester une option en jeu comme les autres jeux »; RT-PT-PLAN §6: On by default,
// Off allowed in ray tracing): the settings' rr (R12b), live (R12f); no-rr.flag (the test runs) turns it off.
bool NativeRtPtRequested() noexcept;   // below
bool NativeRrRequested() noexcept {
    static const bool off=ModuleFlag(L"no-rr.flag");if(off)return false;
    // I-0132 R12c (the final review's H1): D-0265's rule read as the page shows it: at the PT level RR is On whatever the file's rr
    // (the PT runs only on the frames RR denoises), settings::Effective's.
    return NativeSettingsCurrent().rr || NativeRtPtRequested();}   // the step review's L1: rt-pt.flag's PT too
bool NativeRrActive() noexcept {return native_rr_active.load(std::memory_order_relaxed);}
// I-0122 R4c: the rays' level (RT-PT-PLAN §6, the player's option): the settings' level (R12b), « pt » the path-traced level (every
// light casts, one sampled a pixel: ReSTIR's first stage; on the frames RR denoises), else the ray-traced level (R4b); rt-pt.flag (the
// test runs) the PT level too. Live (R12f).
bool NativeRtPtRequested() noexcept {
    static const bool flag=ModuleFlag(L"rt-pt.flag");
    return flag || NativeSettingsCurrent().level==ds3native::settings::Level::Pt;}
// I-0128 R8b (R8-PLAN §3): the PT level's traced vertices a path: the test runs' rt-bounces-N.flag (N 1 to kRtPathVerticesMost), else
// the settings' bounces (R12b: 1 to kRtPathVerticesMost, kRtPathVerticesDefault by default), live (R12f).
static_assert(ds3native::settings::kBouncesDefault==kRtPathVerticesDefault && ds3native::settings::kBouncesMost==kRtPathVerticesMost);
uint32_t NativeRtBouncesRequested() noexcept {
    static const uint32_t flag=[]{for(uint32_t n=1;n<=kRtPathVerticesMost;++n){wchar_t name[32]{};swprintf_s(name,L"rt-bounces-%u.flag",n);if(ModuleFlag(name))return n;}return 0u;}();
    return flag?flag:NativeSettingsCurrent().bounces;}
// I-0132 R12g: the PT level's GI paths a pixel: the test runs' rt-gi-paths-N.flag (N 1 to kRtGiPathsMost), else one (the setting's row
// removed, ROADMAP workstream 2: no gain measured in motion).
uint32_t NativeRtGiPathsRequested() noexcept {
    static const uint32_t flag=[]{for(uint32_t n=1;n<=kRtGiPathsMost;++n){wchar_t name[32]{};swprintf_s(name,L"rt-gi-paths-%u.flag",n);if(ModuleFlag(name))return n;}return 0u;}();
    return flag?flag:kRtGiPathsDefault;}
// I-0132 R12g (D-0266, Thomas 2026-11-11 06:25 to 06:34): DS3's tone-map vignette (where the area's Vignette Power asks it), the settings'
// vignette, live; no-vignette.flag (the test runs) turns it off.
bool NativeVignetteWanted() noexcept {static const bool off=ModuleFlag(L"no-vignette.flag");return !off && NativeSettingsCurrent().vignette;}
// I-0138 tranche 5: what the HDR10 output is asked. The test runs' flags first, read once (hdr-output-force.flag: an HDR10 chain on
// any screen, the compositor converting it for an SDR one; hdr-output.flag: where Windows' HDR is on for the game's screen;
// hdr-paper-80.flag: the paper white at 80 nits, the compositor's own white for an SDR screen); else the settings' hdr, hdr_peak
// and hdr_paper_white, live (FollowNativeLiveSettings). Engine thread.
ds3native::hdr::Request NativeHdrRequest() noexcept {
    static const int flag=ModuleFlag(L"hdr-output-force.flag")?2:ModuleFlag(L"hdr-output.flag")?1:0;static const bool paper80=ModuleFlag(L"hdr-paper-80.flag");
    ds3native::hdr::Request request;
    // hdr-cycle.flag (a test tool: the live switch witnessed without driving the menu): asked once at the start and once an engine
    // frame, the request alternates every 600 frames between Off and a forced HDR10 chain, from Off.
    static const bool cycle=ModuleFlag(L"hdr-cycle.flag");
    if(cycle){static uint64_t asked{};request.mode=((asked++/600)&1)?ds3native::hdr::Mode::Force:ds3native::hdr::Mode::Off;if(paper80)request.paper_white=80.0f;return request;}
    if(flag){request.mode=flag==2?ds3native::hdr::Mode::Force:ds3native::hdr::Mode::Auto;if(paper80)request.paper_white=80.0f;return request;}
    const auto s=NativeSettingsCurrent();
    request.mode=s.hdr?ds3native::hdr::Mode::Auto:ds3native::hdr::Mode::Off;request.paper_white=float(s.hdr_paper_white);request.peak=float(s.hdr_peak);return request;}
// I-0132 R12d: the native passes' pipeline warm-up. The pipeline store's list holds what earlier sessions created; a pass first used in
// a session (the PT level's paths and ReSTIR GI's draws at a live switch: 10 to 12 s frozen in I-0132 R12g's runs, the frame's record
// creating them) would compile then. So at start, with the rays usable, a worker thread (below normal, beside the precompile's) makes
// the rays' pipelines through the store (RtReflections::Warm, RtLocalLights::Warm: the same descriptions as their passes), which then
// find them in its memory, or wait for that name only while it is built. no-native-warm.flag (the test runs' A/B) leaves it out. Its
// progress feeds the precompile screen (16b); the thread is joined before the store closes (02_frame_lifecycle.inc).
// The thread object is never destroyed (the step review's H1: a joinable std::thread global destroyed at the module's unload, in a
// session that never ran Close, as a play session, ends the process), as the module's other owners of threads (*new).
std::atomic<bool> native_warm_cancel{},native_warm_finished{};std::atomic<uint32_t> native_warm_done{},native_warm_total{};std::thread& native_warm_thread=*new std::thread;
// `ser`: SER's state object too (the renderer's rt_ser: none to build when SER is off, the step review's M1).
void StartNativeWarm(ID3D12Device* device,bool ser){
    static const bool off=ModuleFlag(L"no-native-warm.flag");
    if(off || !device || native_warm_thread.joinable()){native_warm_finished=true;return;}
    // The final review's L4: without SER neither its state object nor its apply pipeline (kind 12).
    native_warm_total=ds3native::RtReflections::WarmTotal(ser)+ds3native::RtLocalLights::kWarmPipelines;
    try{native_warm_thread=std::thread([device,ser]{SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_BELOW_NORMAL);
        LARGE_INTEGER t0{},t1{},f{};QueryPerformanceCounter(&t0);QueryPerformanceFrequency(&f);
        // The final review's L4: the lights' two before SER's state object (its 8 s on a cold driver cache last).
        uint32_t made=ds3native::RtReflections::Warm(device,native_warm_cancel,native_warm_done,ser)+ds3native::RtLocalLights::Warm(device,native_warm_cancel,native_warm_done);
        if(ser)made+=ds3native::RtReflections::WarmSer(device,native_warm_cancel,native_warm_done);
        QueryPerformanceCounter(&t1);native_warm_finished=true;
        char row[192]{};sprintf_s(row,"{\"event\":\"native_pipeline_warm\",\"made\":%u,\"tried\":%u,\"of\":%u,\"ms\":%.0f,\"cancelled\":%s}\n",made,native_warm_done.load(),native_warm_total.load(),
            double(t1.QuadPart-t0.QuadPart)*1000.0/double(f.QuadPart),native_warm_cancel.load()?"true":"false");Log(row);});}
    catch(...){native_warm_finished=true;Log("{\"event\":\"native_pipeline_warm\",\"started\":false}\n");}}
// Joined before the store closes, and the warmed SER object released before the device goes (the step review's M1).
void StopNativeWarm() noexcept {native_warm_cancel=true;try{if(native_warm_thread.joinable())native_warm_thread.join();}catch(...){}ds3native::RtReflections::DropWarmed();}
// I-0111 tranche 2 (the modern cadence, form B): display-clock.flag, the engine's frame-start wait on the frame-latency object.
bool NativeDisplayClockRequested() noexcept {static const bool on=ModuleFlag(L"display-clock.flag");return on;}
// The mode: the test flags first (fixed for the session), else the settings' dlss (R12b; dlss_mode.h's words: off, dlaa, quality,
// balanced, performance or ultra-performance), live (R12f). I-0132 R11j (D-0265): no DLSS mode by default (no file or key, « off » or a
// refused word: NativeDlssChosen false, the TAA then; the mode then reads DLAA, unused); dlss-dlaa.flag the test runs' DLAA. The start's
// source (default, flag, file or file_unrecognised) and the settings' word are named in one row by FollowNativeDlssOption (Log is
// defined later than this part), each later change in its native_dlss_option row.
const char* native_dlss_mode_source{"default"};std::string native_dlss_mode_text;
struct NativeDlssChoice {ds3native::streamline::DlssMode mode{ds3native::streamline::DlssMode::Dlaa};bool chosen{};};
NativeDlssChoice NativeDlssNow() noexcept {
    using ds3native::streamline::DlssMode;using ds3native::settings::Dlss;
    static const std::optional<DlssMode> flag=[]()->std::optional<DlssMode>{
        if(ModuleFlag(L"dlss-dlaa.flag"))return DlssMode::Dlaa;
        if(ModuleFlag(L"dlss-quality.flag"))return DlssMode::Quality;if(ModuleFlag(L"dlss-balanced.flag"))return DlssMode::Balanced;
        if(ModuleFlag(L"dlss-performance.flag"))return DlssMode::Performance;if(ModuleFlag(L"dlss-ultra-performance.flag"))return DlssMode::UltraPerformance;
        return std::nullopt;}();
    static const bool named=[]{   // the start's source, for the first row
        const auto name=[](const char* source,const std::string& text){std::string shown=text.substr(0,40);
            for(auto& c:shown)if(c=='"' || c=='\\' || c<32 || c>126)c='?';native_dlss_mode_source=source;native_dlss_mode_text=shown;};
        const auto& file=NativeSettingsFile();
        if(flag)name("flag","");
        else if(file.Refused("dlss") && !file.Accepted("dlss"))name("file_unrecognised",file.RefusedValue("dlss"));
        else if(!file.Accepted("dlss"))name("default","");
        else name("file",ds3native::settings::Name(file.settings.dlss));
        return true;}();
    (void)named;
    if(flag)return {*flag,true};
    switch(NativeSettingsCurrent().dlss){case Dlss::Dlaa:return {DlssMode::Dlaa,true};case Dlss::Quality:return {DlssMode::Quality,true};
        case Dlss::Balanced:return {DlssMode::Balanced,true};case Dlss::Performance:return {DlssMode::Performance,true};
        case Dlss::UltraPerformance:return {DlssMode::UltraPerformance,true};default:return {DlssMode::Dlaa,false};}}
ds3native::streamline::DlssMode NativeDlssMode() noexcept {return NativeDlssNow().mode;}
bool NativeDlssChosen() noexcept {return NativeDlssNow().chosen;}
// I-0111 tranche 2 (Reflex, D-0240: a player option): the test flags reflex-on.flag and reflex-boost.flag first, else the settings'
// reflex (R12b: off, on or boost; Off by default, Thomas's choice), live (R12f: FollowNativeReflex sends a change at the next frame).
bool native_reflex_on{};   // the engine thread's: Reflex On or Boost sent and ready (FollowNativeReflex, each BeginFrame)
// I-0132 R12f (the step review's H1): the requested rate above 0 on this engine frame (BeginFrame): the follow camera's laws, which take
// their own k from the call's dt, run the original's update at DS3's own rate; the rate pinned to the start's while the corrections are
// not installed (an observing flag, no-unlock-fix.flag, original-unlock.flag: the step review's L3), set by the hooks' setup.
std::atomic<bool> native_rate_unlocked{};bool native_rate_pinned{};float native_rate_start{};
ds3native::streamline::ReflexMode NativeReflexMode() noexcept {
    using ds3native::streamline::ReflexMode;
    static const std::optional<ReflexMode> flag=[]()->std::optional<ReflexMode>{
        if(ModuleFlag(L"reflex-boost.flag"))return ReflexMode::Boost;if(ModuleFlag(L"reflex-on.flag"))return ReflexMode::On;return std::nullopt;}();
    if(flag)return *flag;
    using ds3native::settings::Reflex;const Reflex chosen=NativeSettingsCurrent().reflex;
    return chosen==Reflex::On?ReflexMode::On:chosen==Reflex::Boost?ReflexMode::Boost:ReflexMode::Off;}
// I-0131 R10b (R10-PLAN §3, RT-PT-PLAN §6: the player's option): DLSS frame generation's multiplier, fg-on.flag (x2) or fg-x3.flag,
// fg-x4.flag (test tools), else the settings' frame_generation (R12b: off, on or x2, x3, x4; Thomas, 2026-11-08 18:06: x3 and x4 for
// the RTX 50, multi frame generation; the adapter's own maximum caps it); 0 (off) by default. I-0132 R12f: live (the test flags
// fixed); it runs only with a DLSS mode (settings::Effective: NativeFrameGenerationWanted), switched by FollowNativeLiveSettings.
uint32_t NativeFrameGenerationMultiplier() noexcept {
    static const uint32_t flag=ModuleFlag(L"fg-x4.flag")?4u:ModuleFlag(L"fg-x3.flag")?3u:ModuleFlag(L"fg-on.flag")?2u:0u;
    return flag?flag:NativeSettingsCurrent().frame_generation;}
bool NativeFrameGenerationRequested() noexcept {return NativeFrameGenerationMultiplier()>=2;}
uint32_t NativeFrameGenerationWanted() noexcept {return NativeFrameGenerationRequested() && NativeDlssChosen()?NativeFrameGenerationMultiplier():0u;}
// I-0111 tranche 3: the main world camera at the render extent while DLSS runs a scaled mode (the engine's camera output keeps the
// output extent: its aspect becomes explicit, so the lens stays the output's), then its jitter for the mode's phases.
void ApplyUpscalerJitter(SceneViewInput& camera) noexcept {
    // I-0132 R11j: the TAA's jitter at the output extent (8 phases, ER's JitterCount default), its camera unscaled.
    // I-0138 tranche 9: taa-no-jitter.flag (NativeTaaTest's), a test tool of the calibration: the TAA on an unjittered camera (its
    // reconstruction still filters the frame at 2.29, nearly its own sample from a kernel of about 8). Under DLSS the flag does nothing.
    if(!NativeDlssActive()){if(NativeTaaActive() && !NativeTaaTest().no_jitter){const auto jitter=ds3native::upscaler::Jitter(camera.serial,8);camera.jitter_x=jitter[0];camera.jitter_y=jitter[1];}
        return;}
    if(render_width && render_height && camera.width==output_width && camera.height==output_height){
        if(!camera.explicit_aspect)camera.explicit_aspect=float(camera.width)/float(camera.height);
        camera.width=render_width;camera.height=render_height;}
    const auto phases=ds3native::upscaler::JitterPhases(camera.width,output_width);
    const auto jitter=ds3native::upscaler::Jitter(camera.serial,NativeRrActive()?(std::max)(phases,32u):phases);   // I-0123
    camera.jitter_x=jitter[0];camera.jitter_y=jitter[1];}
// I-0111 tranche 3: the extent the post chain (the YEBIS passes, the vignette, the glow) runs at for a main-view camera: the output
// extent, which a scaled DLSS mode's camera is smaller than.
std::array<uint32_t,2> NativePostExtent(const SceneViewInput& camera) noexcept {
    if(render_width && camera.width==render_width && camera.height==render_height)return {output_width,output_height};
    return {camera.width,camera.height};}
// W7 TS-2b: the tessellation fields of a material's GX04 or GX90 item (the C2 admission's parse, MENUS-MAP 67), the last
// such item winning as there; none without the item.
std::optional<ds3native::tess::Material> NativeMaterialTess(const std::vector<uint8_t>& gx) {
    std::optional<ds3native::tess::Material> result;
    for(size_t at=0,item=0;item<65536 && at+12<=gx.size();++item){
        uint32_t tag{};int32_t length{};std::memcpy(&tag,gx.data()+at,4);std::memcpy(&length,gx.data()+at+8,4);
        if(tag==0x7FFFFFFF || length<12 || static_cast<size_t>(length)>gx.size()-at)break;
        const auto words=(static_cast<size_t>(length)-12)/4;const auto* data=gx.data()+at+12;
        if((tag==0x34305847 || tag==0x30395847) && words>=7){ds3native::tess::Material t;
            if(tag==0x34305847?ds3native::tess::FromGx04(data,words,t):ds3native::tess::FromGx90(data,words,t))result=t;}
        at+=static_cast<size_t>(length);}
    return result;
}

// W7: whether a mesh's vertex elements feed every non-system input of a vertex programme (the `_PN` vertex programmes read
// TEXCOORD1, an int4 the ordinary ones do not: a mesh authored without it cannot take the `_PN` recipe).
bool NativeLayoutFeeds(const ds3content::ShaderInterface& vertex,const std::vector<ds3content::ModelVertexElement>& elements) {
    for(const auto& input:vertex.inputs){if(input.system_value)continue;
        if(std::none_of(elements.begin(),elements.end(),[&](const auto& element){
            return element.semantic_index==input.semantic_index && _stricmp(element.semantic.c_str(),input.semantic.c_str())==0;}))return false;}
    return true;
}
struct NativeModelLayout {
    uintptr_t resolver{};
    uintptr_t heap{};   // I-0085: the resolver's heap its prefix came from (a spare prefix is reused within one heap)
    uint32_t type{},flags{};
    BYTE instanced{};
    uint64_t key{};   // I-0081: NativeModelLayoutKey, its entry in model_layout_keys
    std::vector<std::array<uint32_t,7>> elements;
    std::vector<ds3content::ModelVertexElement> vertex_elements;
    bool vertex_elements_available{};
    std::shared_ptr<NativeModelMaterial> material;
    struct Programs {
        std::shared_ptr<const NativeEffect> effect;
        std::shared_ptr<const NativeMaterial> authored;
        // Pass6/7 x pixel variant0/1/2, deliberately distinct from eligibility.
        std::array<ds3content::ModelProgramSelection,native_recipe_count> recipes{}; // NativeRecipe(pass,pixel): Gbuf 0-2, Gbuf2 3-5, Fwd 6-8, Velo 9-11
        // I-0100 tranche 7: the interfaces and reads are the kept programme analyses' own (AnalyzeNativeProgram), shared by every
        // layout of the same recipe set (NativeRecipeSet, 08_model_programmes.inc), never copied per layout.
        std::array<std::array<std::shared_ptr<const ds3content::ShaderInterface>,2>,native_recipe_count> interfaces;
        std::array<std::array<std::shared_ptr<const ds3reads::Reads>,2>,native_recipe_count> reads;
        const ds3reads::Reads& ReadsOf(size_t recipe,size_t stage) const {static const ds3reads::Reads none{};
            const auto& kept=reads.at(recipe)[stage];return kept?*kept:none;}
        std::vector<ds3content::PreparedModelParameter> numeric_parameters;
        std::optional<ds3content::ModelGeneratedParameters> generated_parameters;
        std::optional<ds3content::ModelDamageParameters> damage_parameters;
        std::vector<std::wstring> unavailable_parameters;
        std::optional<ds3content::ModelMaterialPolicy> policy;
        std::wstring shader_family;
        // Programme source path: the authored .spx, or 82220's group name when
        // the authored group is not a loaded effect (composed from loaded programmes).
        std::wstring program_path;
        bool hair_family{};   // m45 frame cost: the path names a Hair family (the 1181 witness tests it on every refused row)
        std::vector<ds3content::ModelTextureBinding> texture_bindings;
        bool texture_bindings_available{};
        bool prefer_channel{};
        // 1186: the shared loose Velo pixel programme (GXFlver_Common_Velo) of the plain Velo recipe (shared_pixel).
        std::shared_ptr<const NativeShader> velocity_common_pixel;
        std::vector<std::wstring> absent;   // I-0120 C6: the loose programmes its recipe set asked for and did not find (NativeRecipeSet)
        // C6 (the final review's M1): when they were last asked again (shader_registry_changes, tick), under the layout's programs_lock.
        mutable uint64_t absent_changes{};mutable ULONGLONG absent_at{};
        // Summoned phantoms (Native1922): A9EF0 loads <shader>_Phantom_Fwd[_[A]] from the family's bundle, or from the loaded
        // repository [GXRS+0x4DD0] without one; the gxflvershader .spx families (skin, face, eye, Equip) carry none in their
        // effect, so the phantom recipes 12-13 take the loose programme by name (shared_pixel with this slot).
        std::array<std::shared_ptr<const NativeShader>,2> phantom_loose_pixel;
        // Workstream K P1 (K-COLLECT-IMPLEMENTATION-SHEET 2.1): what the collect's block families need from a recipe and
        // stage that depends only on this object, built with the interfaces: the register plans, the cbMtdParam variables
        // with their MTD fallback words (1112), the zero tail of cbInstanceData, and the final cbMtdParam block of a
        // composed group (no SPX declarations), which replaces the numeric block whatever the mesh's writes are.
        struct BlockPlan {
            ds3content::ShaderRegisterPlan numeric,scene,view,instance;
            struct MtdVariable {uint32_t offset{},bytes{};bool fallback{};std::vector<std::pair<uint32_t,uint32_t>> words;};
            bool mtd_declared{};std::vector<MtdVariable> mtd_variables;
            uint32_t instance_tail{};
            std::shared_ptr<const ds3content::ShaderRegisterBlock> composed_numeric;
        };
        // I-0100 tranche 7: shared by the layouts of one recipe set and authored material (NativeRecipeSet::plans, 08_model_programmes.inc).
        using BlockPlans=std::array<std::array<BlockPlan,2>,native_recipe_count>;
        std::shared_ptr<const BlockPlans> block_plans;
        const BlockPlan& PlanOf(size_t recipe,size_t stage) const {return block_plans->at(recipe)[stage];}
        // Workstream K P3 (K-COLLECT-IMPLEMENTATION-SHEET 2.3): the numeric (b3) blocks of every recipe and stage for one
        // exact list of numeric writes (offset, bytes, written words), compared whole on lookup; least recently used
        // entry replaced beyond 32. Guarded by its own mutex: the menu thread's fallback collect reads it too.
        struct NumericEntry {
            std::vector<ds3content::ModelParameterWrite> writes;
            std::array<std::array<std::shared_ptr<const ds3content::ShaderRegisterBlock>,2>,native_recipe_count> blocks;
            uint64_t used{};
        };
        mutable std::mutex numeric_lock;
        mutable std::vector<NumericEntry> numeric_cache;
        mutable uint64_t numeric_clock{};
        // I-0097 tranche 2: the uid of the last scene a collect thread claimed this object's scene tables for
        // (PlanNativeProgramTables): only the thread that moves it takes the scene's tables lock; the others of that scene skip.
        mutable std::atomic<uint64_t> planned_scene{};
        // I-0117 (R2 T2): the G-buffer vertex programme's shape for the rays (NativeRtShapeOf, 12_scene_collect.inc), read once a
        // programme set: 0 not read yet, else 0x80 with its bits.
        mutable std::atomic<uint8_t> rt_shape{};
        // Workstream K P4a: what the texture admission derives from each binding's path alone, once: the FaceGen slot
        // test (D39170 stem FG_face_gen), the file stem for the SYSTEX fallbacks, and the ordered lookup candidates
        // (the path, its "_l" low-resolution package, and for a DOL lightmap its "_atlas000" package); with the scene
        // slots no material binding names (slot 45 and the forward slots 30, 31, 33, 44, 59-62, bit = slot - 30).
        struct TextureLookup {bool facegen{};std::wstring leaf,leaf_folded;std::vector<std::wstring> candidates;};   // leaf_folded: lower case (optimisation review)
        std::vector<TextureLookup> texture_lookups;
        // I-0071 (ER B, D-0195; replaces workstream K P4b/P4c and I-0070's kept choices): per binding, the engine's holder of each
        // lookup candidate (D24490, SprjFlverResourceFactory slot 6: find-only; the cap and its holder SprjGxTexture exist from the
        // model's request, seeded with SYSTEX_DUMMY, and an arrival swaps the holder's inner in place). Holders are stable for the
        // cap's life, so a choice stands under the same root; one without the first candidate's holder is looked up again when the
        // registry moved or after native_texture_fallback_ms (a cap made through a mutator not observed: K sheet gap (c)). What a
        // holder resolves to is read at each use (NativeHolderInner), never kept here.
        // I-0100 tranche 2 (Anor Londo after a death, Thomas 2026-10-26; witnessed: after a death a lookup failed while a fresh D24490 found a resident holder, RESULT tranche 2 item 7): a load may end the caps' lives
        // (a death's reload releases the map's lightmap packages and makes new caps), so a choice also stands only within the scene
        // it was made in (NativeKeptDrawResets).
        struct TextureChoice {uint64_t registry{};uintptr_t root{};std::array<uintptr_t,4> holders{};uint64_t looked_up_ms{};uint64_t scene{};};
        mutable std::mutex texture_choice_lock;
        mutable std::vector<TextureChoice> texture_choices;   // parallel to texture_bindings
        bool names_slot45{};uint64_t named_scene_slots{};
        bool samples_depth56{};   // I-0105: a pixel programme reads t56 g_DepthTexture (the feathered effect models, GXFlver_ColZFeather)
        uint8_t samples_snow{};   // I-0105 tranche 10: bit i, a programme reads t47+i (the snow materials' g_Snow* textures)
        // Workstream K P5 (K-COLLECT-IMPLEMENTATION-SHEET 2.5): SelectModelShaderTextures of every recipe and stage for one
        // ordered list of scene slots (status and registers before the per-mesh record checks); 8 entries, least
        // recently used replaced, under its own mutex.
        struct ShaderTextureEntry {
            std::vector<uint32_t> scene_slots;
            std::array<std::array<std::pair<ds3content::ModelTextureStatus,std::vector<ds3content::ModelShaderTexture>>,2>,native_recipe_count> selections;
            uint64_t used{};
        };
        mutable std::mutex shader_texture_lock;
        mutable std::vector<ShaderTextureEntry> shader_texture_cache;
        mutable uint64_t shader_texture_clock{};
        // Workstream K R8a (D-0164): the plans of the registers PrepareNativeModelFrame packs per draw, made at first use
        // per recipe, stage, slot and expected name, so the layout analysis is not redone per draw; own mutex.
        struct NamedPlan {std::string name;std::shared_ptr<const ds3content::ShaderRegisterPlan> plan;};
        // PREPARE-REDUCTION cP1: each recipe's draw programme set (bytecode and vertex elements, shared by every draw of the
        // recipe), made at its first preparation. cP4: palette index checks by their whole key (256 entries, oldest
        // dropped). Each under its own mutex.
        mutable std::mutex program_set_lock;
        mutable std::array<std::shared_ptr<const NativeModelProgramSet>,native_recipe_count> program_sets;
        // I-0134: a recipe's cloth Velo set (MakeNativeClothVelocitySet), tried once (null: not a cloth Velo programme), same mutex.
        mutable std::array<std::shared_ptr<const NativeModelProgramSet>,native_recipe_count> cloth_velocity_sets;
        mutable std::array<bool,native_recipe_count> cloth_velocity_tried{};
        struct PaletteCheckKey {
            uint32_t recipe{};uint64_t stream_generation{},storage_generation{};uint32_t patch{};uint64_t vertex_offset{};
            uint32_t vertex_count{},stride{},object_rows{},cancel_rows{};size_t bytes{};
            bool operator==(const PaletteCheckKey&) const=default;
        };
        struct PaletteCheck {ds3content::ModelPaletteIndexStatus status{};uint32_t max_index{};};
        // PREPARE-REDUCTION cP5: b2 cbMatDynParam blocks by their whole input (recipe, stage, inactive-parameter inputs, the
        // mesh's dynamic bytes and coverage), 64 entries, oldest dropped.
        struct MatDynEntry {uint32_t recipe{},stage{};bool inactive{};int8_t wet{};std::vector<uint8_t> bytes,written;
            std::shared_ptr<const ds3content::ShaderRegisterBlock> block;};
        mutable std::mutex matdyn_lock;
        mutable std::vector<MatDynEntry> matdyn_blocks,tiling_blocks;   // b1 cbTextureTilingScale blocks by the row's tiling values, same lock
        mutable std::mutex palette_lock;
        mutable std::vector<std::pair<PaletteCheckKey,PaletteCheck>> palette_checks;
        mutable std::mutex draw_plan_lock;
        mutable std::array<std::array<std::array<std::vector<NamedPlan>,14>,2>,native_recipe_count> draw_plans;
        // W7 TS-2b: the hull and domain interfaces of a tessellated recipe ([0] hull, [1] domain), and the material's
        // cbPNTriangles block (1402302C0 from its GX04/GX90 item), made when native-tess.flag builds the recipes.
        std::array<std::array<std::optional<ds3content::ShaderInterface>,2>,native_recipe_count> tess_interfaces;
        std::shared_ptr<const std::vector<uint8_t>> pn_triangles;
        // Stage 0 vertex, 1 pixel, 2 hull, 3 domain (the last two of a tessellated recipe only).
        const std::vector<uint8_t>& ProgramBytes(size_t recipe,size_t stage) const {
            const auto& selection=recipes.at(recipe);
            if(stage>=2){if(!selection.tessellated)throw std::logic_error("native hull/domain programme of a flat recipe");
                return effect->programs.at(stage==2?selection.hull:selection.domain).program->bytes;}
            if(stage && selection.shared_pixel && (recipe==12 || recipe==13)){const auto& loose=phantom_loose_pixel[recipe-12];
                if(!loose)throw std::logic_error("native loose phantom pixel programme");return loose->bytes;}
            if(stage && selection.shared_pixel){if(!velocity_common_pixel)throw std::logic_error("native shared Velo pixel programme");return velocity_common_pixel->bytes;}
            return effect->programs.at(stage?selection.pixel:selection.vertex).program->bytes;}
    };
    std::mutex programs_lock;
    // Workstream K P6: one Programs per prefer_channel (models sharing a layout may differ by group+98 bit 1; one slot
    // rebuilt the programmes on every alternation), and the last successful choice per prefer_channel with the registry
    // generation and the material/authored identities it was made under.
    std::array<std::shared_ptr<const Programs>,2> programs;
    struct ProgramsChoice {uint64_t generation{};std::weak_ptr<const NativeModelMaterial> material;const void* authored{};std::shared_ptr<const Programs> programs;};
    std::array<ProgramsChoice,2> choices;
};
// m45 frame cost (Native1842 sample profile: the node map of each model's planned programmes allocated even when empty): the
// programmes one model's instance table was filled for, a flat set by owner address (a model has a few).
struct NativePlannedPrograms {
    std::vector<std::pair<const void*,std::shared_ptr<const NativeModelLayout::Programs>>> items;
    bool contains(const void* key) const noexcept {for(const auto& item:items)if(item.first==key)return true;return false;}
    std::pair<size_t,bool> try_emplace(const void* key,const std::shared_ptr<const NativeModelLayout::Programs>& value){
        for(size_t i=0;i<items.size();++i)if(items[i].first==key)return {i,false};
        items.emplace_back(key,value);return {items.size()-1,true};}
};
// I-0081: shared for the lookups (the collect's record checks and the admission workers take it per mesh), exclusive for an
// insertion or erase; a build finds its equal layout by its key (NativeModelLayoutKey) instead of a scan of every layout.
std::shared_mutex model_layout_lock;
std::unordered_map<uintptr_t,std::shared_ptr<NativeModelLayout>> model_layouts;
std::unordered_multimap<uint64_t,uintptr_t> model_layout_keys;
// I-0085 (the night endurance test: about 1,100 layouts a zone load, never retired, 150 MB of private memory a visit): a
// layout is keyed by its material object, which every FLVER load makes anew, and the engine never retires the prefix (its
// resolver cache owns it in the original, keyed by the shared MTD shader, and the native path skips that cache). So the layouts
// leave with their material (DestroyModelMaterial: the FLVER resource's meshes go with it), and their engine prefixes, still
// valid objects of ours, are kept here by heap and handed to the next builds instead of new engine allocations.
std::vector<std::pair<uintptr_t,uintptr_t>> model_layout_spare_prefixes;   // (prefix, heap), under model_layout_lock
uint64_t model_layouts_retired{};                                            // under model_layout_lock
thread_local std::shared_ptr<NativeModelMaterial> model_layout_material;
struct NativeGparam {
    uint64_t generation{};uint32_t kind{};int32_t area{},bank{};
    std::wstring name;std::vector<uint8_t> bytes;   // I-0097 tranche 4: parsed once, by the banks (Banks::Load), not again here
};
std::mutex gparam_lock;
std::unordered_map<uintptr_t,std::shared_ptr<const NativeGparam>> gparams;
uint64_t gparam_generation{};
struct NativeGparamSceneBinding {
    std::shared_ptr<const NativeGparam> resource;
    uint32_t area{},bank{},weight_bits{};
    std::array<BYTE,0x60> metadata{};   // receiver +0x80..+0xDF: the 3x4 transform first (49D00's fifth argument)
    uint64_t sequence{};                // binding order (the latest area file feeds Banks)
};
// Keyed {area, bank, receiver}: one area can hold several files on one bank (the area controller's layers).
std::map<std::tuple<uint32_t,uint32_t,uintptr_t>,NativeGparamSceneBinding> gparam_scene_bindings;
uint64_t gparam_binding_sequence{};
std::shared_ptr<const ds3light::Banks> gparam_banks;
// I-0141 tranche 3: the cutscene act's overrides of the map's light and fog records (18_resource_admission.inc, RecordRemoOverride),
// given to every Banks the module publishes; guarded by gparam_lock.
ds3light::Banks::RecordOverrides remo_record_overrides;
void NativeLightRecordBlend(uint8_t* out,const uint8_t* a,const uint8_t* b,float t);
void NativeFogRecordBlend(uint8_t* out,const uint8_t* a,const uint8_t* b,float t);
const std::filesystem::path native_host_root=L"@ds3-native-startup";
std::set<std::filesystem::path> gparam_host_paths;
std::once_flag native_host_inputs_once;
void PublishNativeGparamBanks();
void BindNativeGparamFileFrom(uintptr_t caller,uintptr_t cap,int32_t bank,float weight);
// Generated from the exact original executable by the build. Refuse every
// mismatch before publication. No diagnostic code patch is reused as a pin.
#include "game_bootstrap_pins.inc"
void Log(const char* text) noexcept {
    std::lock_guard<std::mutex> hold(journal_lock);
    DWORD written{}; const auto size=static_cast<DWORD>(std::strlen(text));
    // 1132: no FlushFileBuffers per row (a disk flush per trace row); WriteFile data survives a process fault.
    if(!WriteFile(journal,text,size,&written,nullptr) || written!=size) {
        if(control) InterlockedIncrement64(&control->faults);
    }
}
// I-0097 (I-0095 §8): with frame-dump.flag a timing row (native_frame_times, native_present_times, game_frame_times,
// native_original_frames) also carries its window's frame times in order ("ms"), so the lows and the band's catch-up pattern are
// computed from every frame, not bounded by the windows' counts. Without the flag the row is logged as it is.
void LogFrameTimes(const char* row,const std::vector<float>& in_order) noexcept {
    static const bool dump=ModuleFlag(L"frame-dump.flag");const size_t length=std::strlen(row);
    if(!dump || in_order.empty() || length<2 || row[length-2]!='}'){Log(row);return;}
    try{std::string out(row,length-2);out+=",\"ms\":[";char value[16]{};
        for(size_t i=0;i<in_order.size();++i){sprintf_s(value,"%s%.3f",i?",":"",in_order[i]);out+=value;}   // I-0098: three decimals (the relative limits are not on a 0.01 ms grid)
        out+="]}\n";Log(out.c_str());}
    catch(...){Log(row);}
}
// I-0097: the display_refreshes row, one per timing window, alike for both renderers (the native chain, the original's in its
// probe): the frames the display showed, the refreshes it scanned, those that repeated a frame and the samples with a repeat.
void LogDisplayRefreshes(const ds3native::DisplayRefreshes::Counts& d,const char* renderer_name,int64_t frame,uint32_t refresh_hz) noexcept {
    if(d.presented<60)return;   // no statistics (occluded, a chain without them) or a partial window
    char row[300]{};sprintf_s(row,"{\"event\":\"display_refreshes\",\"renderer\":\"%s\",\"frame\":%lld,\"samples\":%llu,\"presented\":%llu,\"refreshes\":%llu,\"repeated\":%llu,\"hitches\":%llu,\"shortfalls\":%llu,\"refresh_hz\":%u,\"tick_ms\":%llu}\n",
        renderer_name,static_cast<long long>(frame),static_cast<unsigned long long>(d.samples),static_cast<unsigned long long>(d.presented),static_cast<unsigned long long>(d.refreshes),
        static_cast<unsigned long long>(d.repeated),static_cast<unsigned long long>(d.hitches),static_cast<unsigned long long>(d.shortfalls),refresh_hz,static_cast<unsigned long long>(GetTickCount64()));
    // I-0098: with frame-dump.flag, each sample's frames and refreshes in order (pairs), for a count against the screen's cadence.
    const size_t length=std::strlen(row);if(d.holds.empty() || length<2 || row[length-2]!='}'){Log(row);return;}
    try{std::string out(row,length-2);out+=",\"holds\":[";char value[24]{};
        for(size_t i=0;i<d.holds.size();++i){sprintf_s(value,"%s%u",i?",":"",d.holds[i]);out+=value;}
        out+="]}\n";Log(out.c_str());}
    catch(...){Log(row);}
}
// I-0097 tranche 5 (cadence-trace.flag, diagnostics only, both renderers): the marks of ds3native::cadence written once a second
// by a thread of their own (one cadence_trace row a kind: its first tick, then the ticks' differences in counter units), and the
// screen's vertical blanks marked by another, waiting on the screen the game's window is on (D3DKMTWaitForVerticalBlankEvent from
// gdi32, the screen re-read every second: the pilot moves the window at the title). Neither touches the game or its threads.
void StartCadenceTrace() noexcept {
    auto& s=ds3native::cadence::Shared();
    try{for(auto& v:s.ticks)v.reserve(ds3native::cadence::capacity);}catch(...){return;}
    ds3native::cadence::on.store(true);
    const auto writer=CreateThread(nullptr,0,[](void*)->DWORD{
        auto& s=ds3native::cadence::Shared();LARGE_INTEGER frequency{};QueryPerformanceFrequency(&frequency);
        std::vector<int64_t> taken[ds3native::cadence::Count];uint64_t dropped[ds3native::cadence::Count]{};
        for(uint64_t second=0;;++second){Sleep(1000);
            try{for(auto& v:taken){v.clear();v.reserve(ds3native::cadence::capacity);}}catch(...){continue;}
            {const std::lock_guard hold(s.lock);for(uint32_t k=0;k<ds3native::cadence::Count;++k){s.ticks[k].swap(taken[k]);dropped[k]=std::exchange(s.dropped[k],0);}}
            // The counter read just after the tick count steps (step review L6: within the counter's read, not 15.6 ms): the reader
            // places the motion marks (tick_ms) on the counter's line.
            LARGE_INTEGER counter{};ULONGLONG tick=GetTickCount64();for(const ULONGLONG first=tick;tick==first;){YieldProcessor();tick=GetTickCount64();}
            QueryPerformanceCounter(&counter);
            // One journal write a second (step review L3: the journal lock taken once).
            std::string rows;
            for(uint32_t k=0;k<ds3native::cadence::Count;++k){const auto& v=taken[k];if(v.empty() && !dropped[k])continue;
                const bool value=k==ds3native::cadence::GameDt;   // values, not ticks: written as they are
                try{rows.reserve(rows.size()+160+v.size()*7);char cell[256]{};
                    sprintf_s(cell,"{\"event\":\"cadence_trace\",\"kind\":\"%s\",\"second\":%llu,\"tick_ms\":%llu,\"qpc\":%lld,\"frequency\":%lld,\"dropped\":%llu,\"first\":%lld,\"d\":[",
                        ds3native::cadence::names[k],static_cast<unsigned long long>(second),static_cast<unsigned long long>(tick),static_cast<long long>(counter.QuadPart),
                        static_cast<long long>(frequency.QuadPart),static_cast<unsigned long long>(dropped[k]),static_cast<long long>(v.empty()?0:v.front()));rows+=cell;
                    for(size_t i=1;i<v.size();++i){sprintf_s(cell,"%s%lld",i>1?",":"",static_cast<long long>(value?v[i]:v[i]-v[i-1]));rows+=cell;}
                    rows+="]}\n";}catch(...){}}
            if(!rows.empty())Log(rows.c_str());}
        },nullptr,0,nullptr);
    if(writer)CloseHandle(writer);   // normal priority (step review L3: a preempted writer would hold the locks the marks take)
    const auto vblank=CreateThread(nullptr,0,[](void*)->DWORD{
        struct OpenFromName {WCHAR device[32];UINT adapter;LUID luid;UINT source;};
        struct WaitVBlank {UINT adapter;UINT device;UINT source;};
        struct CloseAdapter {UINT adapter;};
        const HMODULE gdi=LoadLibraryW(L"gdi32.dll");if(!gdi)return 1;
        const auto open=reinterpret_cast<LONG(APIENTRY*)(OpenFromName*)>(GetProcAddress(gdi,"D3DKMTOpenAdapterFromGdiDisplayName"));
        const auto wait=reinterpret_cast<LONG(APIENTRY*)(const WaitVBlank*)>(GetProcAddress(gdi,"D3DKMTWaitForVerticalBlankEvent"));
        const auto close=reinterpret_cast<LONG(APIENTRY*)(const CloseAdapter*)>(GetProcAddress(gdi,"D3DKMTCloseAdapter"));
        if(!open || !wait || !close){Log("{\"event\":\"cadence_vblank\",\"started\":false}\n");return 2;}
        // The game's window: this process's largest visible top-level window.
        const auto game_window=[]{struct Find {HWND best{};LONG area{};} f;
            EnumWindows([](HWND w,LPARAM p)->BOOL{DWORD pid{};GetWindowThreadProcessId(w,&pid);RECT r{};
                if(pid==GetCurrentProcessId() && IsWindowVisible(w) && GetWindowRect(w,&r)){auto& f=*reinterpret_cast<Find*>(p);
                    const LONG area=(r.right-r.left)*(r.bottom-r.top);if(area>f.area){f.area=area;f.best=w;}}return TRUE;},reinterpret_cast<LPARAM>(&f));
            return f.best;};
        OpenFromName adapter_name{};std::wstring screen;bool have{};ULONGLONG checked{};int64_t last_vblank{};
        const int64_t frequency=[]{LARGE_INTEGER f{};QueryPerformanceFrequency(&f);return f.QuadPart;}();
        for(;;){const ULONGLONG now=GetTickCount64();
            if(!have || now-checked>=1000){checked=now;MONITORINFOEXW monitor{};monitor.cbSize=sizeof(monitor);const HWND w=game_window();
                const std::wstring found=w && GetMonitorInfoW(MonitorFromWindow(w,MONITOR_DEFAULTTONEAREST),&monitor)?monitor.szDevice:L"";
                if(found.empty()){if(!have){Sleep(100);continue;}}
                else if(!have || found!=screen){if(have){const CloseAdapter c{adapter_name.adapter};close(&c);have=false;}
                    adapter_name={};wcsncpy_s(adapter_name.device,found.c_str(),_TRUNCATE);
                    have=open(&adapter_name)>=0;screen=found;
                    char row[160]{};sprintf_s(row,"{\"event\":\"cadence_vblank\",\"screen\":\"%ls\",\"opened\":%s,\"source\":%u}\n",found.substr(found.find_last_of(L'\\')+1).c_str(),   // step review L5: DISPLAY1, no backslash in the row
                        have?"true":"false",adapter_name.source);Log(row);
                    if(!have){Sleep(1000);continue;}}}
            const WaitVBlank v{adapter_name.adapter,0,adapter_name.source};
            // Step review L4: a wait that returns at once (a sleeping screen, an inactive source) must not spin this thread.
            if(wait(&v)>=0){const int64_t at=ds3native::cadence::Now();ds3native::cadence::MarkValue(ds3native::cadence::VBlank,at);
                if(at-last_vblank<frequency/1000)Sleep(1);last_vblank=at;}else{const CloseAdapter c{adapter_name.adapter};close(&c);have=false;Sleep(100);}}
        },nullptr,0,nullptr);
    if(vblank){SetThreadPriority(vblank,THREAD_PRIORITY_TIME_CRITICAL);CloseHandle(vblank);}
    Log("{\"event\":\"cadence_trace_started\"}\n");
}
// Menu tests FL5-FL7 (Thomas, 2026-10-13; FONTS-LANGUAGES-NATIVE 1): test-language.flag next to the module holds a Steam language
// name. The engine's Steam language id (FUN_140EF3720: SteamApps()->GetCurrentGameLanguage() hashed against its 26 names, id in
// EAX) is then that name's id, as Steam itself would give it, without touching the user's Steam settings; the rest of the chain
// (the id table 1429A24F0, the filter FUN_140EA61F0, the files and fonts) is the engine's own. A test aid for announced runs only.
bool Read(uintptr_t p,void* out,size_t size) noexcept;
using SteamLanguageFn=int(__fastcall*)();
SteamLanguageFn steam_language_original{};
std::string TestLanguageName() noexcept {
    const auto file=ModuleSibling(L"test-language.flag");if(file.empty())return {};
    std::ifstream in(file);std::string name;if(!in || !std::getline(in,name))return {};
    while(!name.empty() && (name.back()=='\r' || name.back()==' '))name.pop_back();
    return name;}
int __fastcall NativeTestSteamLanguage() noexcept {
    const int steam=steam_language_original();
    // FUN_140EF3720's table (14489E9C0): the Steam names and their ids.
    static constexpr std::pair<const char*,int> names[]{{"brazilian",1},{"bulgarian",2},{"czech",3},{"danish",4},{"dutch",5},{"english",6},
        {"finnish",7},{"french",8},{"german",9},{"greek",10},{"hungarian",11},{"italian",12},{"japanese",13},{"koreana",14},{"norwegian",15},
        {"polish",16},{"romanian",18},{"russian",19},{"schinese",20},{"spanish",21},{"swedish",22},{"tchinese",23},{"thai",24},{"turkish",25},
        {"ukrainian",26}};
    const std::string wanted=TestLanguageName();int id=steam;
    for(const auto& [name,value]:names)if(wanted==name)id=value;
    char row[200]{};sprintf_s(row,"{\"event\":\"native_test_language\",\"name\":\"%.24s\",\"steam_id\":%d,\"id\":%d}\n",wanted.c_str(),steam,id);Log(row);
    // Row FL1, once the startup initializer has run (20 s later): the region 14479BA04, the flag BA08, the text language BA0C and the
    // live ordinal BA10, and DAT_14479BEDC (the single-language SKU switch).
    static bool state_logged{};if(!state_logged){state_logged=true;std::thread([]{Sleep(20000);
        int32_t region{},flag{},text{},live{},sku{};Read(image+0x479BA04,&region,4);Read(image+0x479BA08,&flag,4);Read(image+0x479BA0C,&text,4);
        Read(image+0x479BA10,&live,4);Read(image+0x479BEDC,&sku,4);
        char state[200]{};sprintf_s(state,"{\"event\":\"native_language_state\",\"region\":%d,\"flag\":%d,\"text\":%d,\"live\":%d,\"single_sku\":%d}\n",region,flag,text,live,sku);Log(state);}).detach();}
    return id;}
// W7 TS-2b row native_tess_rows: the deformed rows (native-tess.flag) by deformation, shader pass and outcome (the `_PN`
// recipe or the flat one: why, "pass" no `_PN` recipe for the deformation and pass, "recipe" its status, "block" no
// cbPNTriangles, "textures" a stage's texture selection, "layout" a vertex input the mesh lacks), the first 16 and every 4096th, with the running totals.
constexpr const char* native_tess_outcomes[]{"pn","pass","recipe","block","textures","layout"};
LONG64 native_tess_row_count{},native_tess_outcome_rows[6]{};
// The first 8 rows of each outcome (with the family) and every 4096th row, with the totals by outcome.
bool NativeTessRowLogged(const char* outcome) noexcept {
    size_t k=0;while(k<5 && std::strcmp(native_tess_outcomes[k],outcome))++k;
    const auto n=InterlockedIncrement64(&native_tess_row_count);return InterlockedIncrement64(&native_tess_outcome_rows[k])<=8 || n%4096==0;}
void NoteNativeTessRow(uint32_t deformation,uint32_t shader_pass,const char* outcome,int status,const std::string& family) noexcept {
    char row[512]{};sprintf_s(row,"{\"event\":\"native_tess_rows\",\"rows\":%lld,\"by_outcome\":[%lld,%lld,%lld,%lld,%lld,%lld],\"deformation\":%u,\"shader_pass\":%u,\"outcome\":\"%s\",\"status\":%d,\"family\":\"%.120s\"}\n",
        native_tess_row_count,native_tess_outcome_rows[0],native_tess_outcome_rows[1],native_tess_outcome_rows[2],native_tess_outcome_rows[3],native_tess_outcome_rows[4],
        native_tess_outcome_rows[5],deformation,shader_pass,outcome,status,family.c_str());Log(row);}
// R13 cR0(ENGINE-STEP-THREAD-MAP TM-1, TM-4, TM-6): which threads call the engine hooks the render thread will share
// data with. Each hook keeps up to eight (thread, count) pairs, lock-free; the census row (every 600th presented frame)
// marks the owner thread; the first retirements of each kind log their caller and whether a render job is in flight
// (always false without the render worker: Clear() then runs the job inside EndFrame).
namespace census {
enum Hook : uint8_t {StorageAdmit,StorageRetire,StorageDestroy,CpuGeometry,TextureAdmit,TextureEncoded,TextureRetire,PoseInit,
    PoseFull,PoseSingle,PosePublic,PoseRetire,StreamInit,StreamUpdate,StreamRetire,ClothUpload,ClothRelease,UiTaskDispatch,
    SceneCpu,HookCount};
constexpr const char* names[HookCount]{"storage_admit","storage_retire","storage_destroy","cpu_geometry","texture_admit",
    "texture_encoded","texture_retire","pose_init","pose_full","pose_single","pose_public","pose_retire","stream_init",
    "stream_update","stream_retire","cloth_upload","cloth_release","ui_task_dispatch","scene_cpu"};
struct alignas(64) Entry {std::atomic<uint32_t> thread{};std::atomic<uint64_t> count{};char padding[48]{};};   // one cache line each (review)
// A retirement hook called by the backend itself (an admission replacing its record): not an engine retirement.
thread_local int internal_calls{};
struct Internal {Internal() noexcept {++internal_calls;} ~Internal() {--internal_calls;}};
std::array<std::array<Entry,8>,HookCount> table;
std::array<std::atomic<uint64_t>,HookCount> overflow{};
void Note(Hook hook) noexcept {
    const uint32_t id=GetCurrentThreadId();
    for(auto& entry:table[hook]){uint32_t known=entry.thread.load(std::memory_order_relaxed);
        if(known==id || (!known && (entry.thread.compare_exchange_strong(known,id) || known==id))){entry.count.fetch_add(1,std::memory_order_relaxed);return;}}
    overflow[hook].fetch_add(1,std::memory_order_relaxed);
}
// TM-6: the first four retirements of a kind: thread, owner or not, caller RVA, render job in flight.
std::array<std::atomic<uint32_t>,HookCount> retirements_logged{};
void NoteRetirement(Hook hook,const void* caller) noexcept {
    if(internal_calls)return;   // the backend's own replacement, not the engine's (review)
    Note(hook);
    if(retirements_logged[hook].load(std::memory_order_relaxed)>=4 || retirements_logged[hook].fetch_add(1,std::memory_order_relaxed)>=4)return;
    // The caller RVA inside the game image only (-1 elsewhere: this module or another).
    static const uintptr_t image_end=[]{if(!image)return uintptr_t{};const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(image);
        const auto* nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(image+dos->e_lfanew);return image+nt->OptionalHeader.SizeOfImage;}();
    const auto at=reinterpret_cast<uintptr_t>(caller);char row[200]{};
    sprintf_s(row,"{\"event\":\"native_retirement_caller\",\"hook\":\"%s\",\"thread\":%lu,\"owner\":%s,\"caller_rva\":%lld,\"job_in_flight\":%s}\n",
        names[hook],GetCurrentThreadId(),GetCurrentThreadId()==owner_thread?"true":"false",image && at>image && at<image_end?static_cast<long long>(at-image):-1ll,
        render_job_in_flight.load(std::memory_order_acquire)?"true":"false");Log(row);
}
std::string Row(uint64_t frame) {
    std::string row="{\"event\":\"native_thread_census\",\"frame\":"+std::to_string(frame)+",\"owner\":"+std::to_string(owner_thread)+",\"hooks\":{";
    bool first_hook=true;
    for(size_t hook=0;hook<HookCount;++hook){std::string threads;
        for(const auto& entry:table[hook]){const auto thread=entry.thread.load(std::memory_order_relaxed);if(!thread)continue;
            if(!threads.empty())threads+=",";threads+="["+std::to_string(thread)+","+std::to_string(entry.count.load(std::memory_order_relaxed))+"]";}
        if(threads.empty())continue;
        row+=(first_hook?"\"":",\"")+std::string(names[hook])+"\":{\"threads\":["+threads+"],\"overflow\":"+std::to_string(overflow[hook].load())+"}";first_hook=false;}
    return row+"}}\n";
}
}
// 1132: an in-process copy guarded by SEH replaces ReadProcessMemory on our own process (one system call per
// field read, tens of thousands per in-game frame). Same contract: false when any byte is unreadable.
thread_local volatile bool guarded_copy_active{}; // our fault handler ignores access violations raised inside it
__declspec(noinline) static bool GuardedCopy(void* out,const void* in,size_t size) noexcept {
    // C7-0 review: the flag is restored, not cleared (a hook's read inside a guarded engine call keeps that call guarded).
    const bool outer=guarded_copy_active;guarded_copy_active=true;bool copied=true;
    // m45 frame cost (Native1841: the guarded reads were a ninth of the engine thread): the common word sizes as single moves.
    __try {
        switch(size){
        case 1:std::memcpy(out,in,1);break;
        case 2:std::memcpy(out,in,2);break;
        case 4:std::memcpy(out,in,4);break;
        case 8:std::memcpy(out,in,8);break;
        case 16:std::memcpy(out,in,16);break;
        default:std::memcpy(out,in,size);}
    } __except(EXCEPTION_EXECUTE_HANDLER) { copied=false; }
    guarded_copy_active=outer;return copied;
}
// I-0078 (i74-native-71: a reused model's admission read its draw group and its model object field by field, about twenty
// guarded reads of the same 0x190 group bytes): an engine block copied once under the guard, from which this thread's reads
// inside it are served while the window is open (NativeReadWindow, one admission). The bytes are those of the window's
// opening; nothing in the admission writes them.
struct NativeReadWindows {static constexpr unsigned limit=1;struct Window {uintptr_t begin{},end{};std::array<uint8_t,0x190> bytes;};
    std::array<Window,limit> windows;unsigned open{};};
thread_local NativeReadWindows native_read_windows;
bool Read(uintptr_t p,void* out,size_t size) noexcept {
    auto& reads=native_read_windows;
    for(unsigned i=0;i<reads.open;++i){const auto& w=reads.windows[i];
        if(p>=w.begin && size<=w.end-w.begin && p-w.begin<=w.end-w.begin-size){std::memcpy(out,w.bytes.data()+(p-w.begin),size);return true;}}
    // I-0105: above the user-mode range (a model particle's key) nothing is read, without the exception a guarded copy would take.
    return p>=0x10000 && p<0x0000800000000000ull && GuardedCopy(out,reinterpret_cast<const void*>(p),size);
}
// Opens a window on [p, p + size) for this thread when the bytes are readable and a slot is free; closed at the end of scope.
class NativeReadWindow {
public:
    NativeReadWindow(uintptr_t p,size_t size) noexcept {auto& reads=native_read_windows;
        if(reads.open>=reads.limit || size>reads.windows[0].bytes.size() || p<0x10000)return;auto& w=reads.windows[reads.open];
        if(!GuardedCopy(w.bytes.data(),reinterpret_cast<const void*>(p),size))return;
        w.begin=p;w.end=p+size;++reads.open;opened_=true;}
    ~NativeReadWindow(){if(opened_)--native_read_windows.open;}
    NativeReadWindow(const NativeReadWindow&)=delete;NativeReadWindow& operator=(const NativeReadWindow&)=delete;
private:
    bool opened_{};
};
// Optimisation review: the engine bytes at p equal the kept ones, compared in place under the same guard (a model record's
// check compared every reused mesh's 0x4F8 material bytes through a copy each collect); false when unreadable or different.
__declspec(noinline) static bool GuardedSame(uintptr_t p,const void* kept,size_t size) noexcept {
    if(p<0x10000)return false;
    const bool outer=guarded_copy_active;guarded_copy_active=true;bool same=false;
    __try {same=std::memcmp(reinterpret_cast<const void*>(p),kept,size)==0;}
    __except(EXCEPTION_EXECUTE_HANDLER) {same=false;}
    guarded_copy_active=outer;return same;
}// I-0066 C7-0: the content readers linked into this module (model selection, scene geometry, scene ingress) take the same
// guarded copy instead of ReadProcessMemory; installed at module initialisation, before any hook runs.
const bool content_reads_guarded=(ds3content::SetProcessRead(&Read),true);
}  // namespace
