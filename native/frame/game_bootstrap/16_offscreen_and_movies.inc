// Menu offscreen model views, the audio census and Bink movies (players, planes, steps and the movie frame). Part of
// game_bootstrap.cpp's single translation unit (the order is there): it relies on every earlier part
// (15_cpu_service_and_input.inc before it).
#pragma once
#include "15_cpu_service_and_input.inc"
namespace {
void __fastcall NativeUiFactory(uintptr_t manager) noexcept;
// 1162: D64C30 builds each menu offscreen model render's scene (portraits, Status player view, character-creation
// face; MENU-OFFSCREEN-MODELS.md). Native1161: its old scene factory (+48, GXSgCompositeScene) faults natively when
// the Status screen opens. Native version: the CPU objects are the genuine ones (two views +20 and a view +28 from
// 4AFF0, the scene +30 CED370 linked by CED790 and C1B20, the light +50 from D039E0 with its .data colours), the GX
// objects +38 (GX scene), +40 (scene list) and +48 (composite scene and its target) stay nil, the old-device clear
// and the render callback registration (+60) are not made; +68/+6C keep the extent. Every later user checks +48
// (D64B50/D65140/D66900) or the callback registration, so the menus run without the offscreen image (explicit
// visual omission until the native offscreen draw exists).
// gparam_id: D64C30 argument 6 = the MenuOffscrRendParam row's gparam id (+0x2C; Status body 11), key {1, id}.
// I-0140 tranche 31 (D-0303, MENUS-MAP 48): `shadowed`, D64C30's argument 5 (the view pipeline's descriptor, null for the default)
// with its flags (+8) bit 0 set: the Player family's 0x400852201F (ChrMakePreview, StatusBody, RankingBody), whose pipeline has the
// shadow sub-scene; the Face and Prof families' 0x8422014 (and the default's) have none. READ: D65DD0 builds that descriptor.
struct NativeOffscreenProxy {uintptr_t holder{};uint32_t width{},height{},gparam_id{};bool shadowed{};};
// Background of a menu view: 87FB0 overwrites the view pipeline's clear +6C each frame with the Setting Info bank's
// [GXRS+4610] "BG Color" (record +80..+8C, clamped) of the view key {D27770(1), id} (MENUS-MAP 27: id 11 = 18/255);
// the set-up constant 0x141414 stays when the bank lacks the key. 1173: natively the engine's Setting Info bank is
// empty (NativeGparamRecords), so the same key is read from the admitted gparam file (default.gparam for area 1),
// exact key only like 15EC10. The value is fixed per view: it is also the target's optimized clear value.
std::array<float,4> NativeOffscreenBackground(const std::array<uint32_t,2>& key,int* state) {
    std::array<float,4> background{20.f/255,20.f/255,20.f/255,1};int bank_state=0;
    std::shared_ptr<const ds3light::Banks> banks;{const std::lock_guard hold(gparam_lock);banks=gparam_banks;}
    float colour[4]{};if(banks && banks->BackgroundColor(key[0],key[1],colour)){bank_state=4;
        for(size_t c=0;c<4;++c)background[c]=std::clamp(std::isfinite(colour[c])?colour[c]:0.f,0.f,1.f);}
    if(state)*state=bank_state;return background;
}
// I-0113 tranche 27 (YEBIS-CHAIN-CHECKLIST P23, world/menu_tonemap.h): the view's GX tone map from the Tone Map group's second
// block at the same key (FUN_1400FB890 blends the bank [GXRS+0x4618] for it each frame; natively the admitted gparam file is read,
// as for the background, with GroupEntries' key rule). `state`: "tabled", "mode N" (not ported), "no table" or the refusal.
// The step review's L1: GroupEntries' fallback to {1, 0} for an id the group lacks is the native's rule, not read for the bank
// [GXRS+0x4618] (INFERRED alike); `source` names the file used, for the row.
ds3native::NativeOffscreenTone NativeOffscreenToneOf(const std::array<uint32_t,2>& key,std::string& state,std::string& source) {
    ds3native::NativeOffscreenTone tone;state="no banks";
    std::shared_ptr<const ds3light::Banks> banks;{const std::lock_guard hold(gparam_lock);banks=gparam_banks;}
    std::vector<ds3light::Banks::BankEntry> entries;
    if(!banks || !banks->GroupEntries(key[0],key[1],"Tone Map",entries,&source)){if(banks)state="no Tone Map group";return tone;}
    std::vector<ds3native::menu_tone::Entry> named;named.reserve(entries.size());
    for(const auto& e:entries)named.push_back({e.name,e.type,e.raw[0]});
    ds3native::menu_tone::Params p;if(!ds3native::menu_tone::FromEntries(named,p,state))return tone;
    tone.mode=p.mode;tone.scale=ds3native::menu_tone::Scale(p);tone.contrast=p.contrast;tone.gamma=p.gamma;tone.scaler=p.scaler;
    // The final review's L4: a scale or gamma the curve cannot take is refused by name (the sRGB encode then).
    const bool usable=std::isfinite(tone.scale) && tone.scale>0.0f && p.gamma>0.0f;
    tone.tabled=ds3native::menu_tone::Tabled(p) && usable;
    state=tone.tabled?"tabled":!p.use_table?"no table":!ds3native::menu_tone::Tabled(p)?"mode "+std::to_string(p.mode):"scale or gamma refused";
    if(!std::isfinite(tone.scale))tone.scale=0.0f;   // the row prints finite numbers
    return tone;
}
std::array<float,4> NativeOffscreenBackground(uint32_t gparam_id) {
    return NativeOffscreenBackground({reinterpret_cast<uint32_t(__fastcall*)(uint32_t)>(image+0xD27770)(1),gparam_id},nullptr);
}
std::mutex native_offscreen_lock;
// I-0140 tranche 41 (its step review's M1): each holder's last collected scene queued for drawing, by its uid (unique per collect: the
// selection serial repeats when a collect runs twice before Clear(), the final review's L1) (CollectNativeOffscreenViews,
// job.views), under native_offscreen_lock; the menu collect resets a view's TAA history (an unjittered frame, its own samples) when its
// last published collect was not queued (a target not resident, a frame unprepared, a collect replaced before its presentation).
std::unordered_map<uintptr_t,uint64_t> native_offscreen_queued;
// I-0120 G1: the holder whose model the offscreen collect is reading (its part drawables go to engine code), and the
// collecting thread, under native_offscreen_lock. The retire hooks wait until the collect has left that holder before the
// original frees its model; the collector's own thread never waits (a retire inside its engine calls would deadlock).
uintptr_t native_offscreen_busy_holder{};DWORD native_offscreen_busy_thread{};std::condition_variable native_offscreen_idle;
void WaitNativeOffscreenHolderIdle(std::unique_lock<std::mutex>& hold,uintptr_t holder) {
    if(native_offscreen_busy_holder!=holder || native_offscreen_busy_thread==GetCurrentThreadId())return;
    // Never freed under the collector (the step review's L4): past 5 s a row says so, and the hook waits on.
    if(native_offscreen_idle.wait_for(hold,std::chrono::seconds(5),[&]{return native_offscreen_busy_holder!=holder;}))return;
    {static LONG64 rows{};if(InterlockedIncrement64(&rows)<=4){char row[160]{};
        sprintf_s(row,"{\"event\":\"native_offscreen_retire_waiting\",\"holder\":%llu}\n",static_cast<unsigned long long>(holder));Log(row);}}
    native_offscreen_idle.wait(hold,[&]{return native_offscreen_busy_holder!=holder;});
}
std::unordered_map<uintptr_t,NativeOffscreenProxy> native_offscreen_proxies;
// 1166: C3B810(model, holder) stores model+318 = the offscreen holder (D690A0, the menu model's setup): the native
// offscreen draw finds each view's model through it. Witness: the model, its +8 draw entity and the camera of the
// holder's scene object +30 (pose +10, fov/aspect/near/far +50..+5C, written by D6B8D0).
std::unordered_map<uintptr_t,uintptr_t> native_offscreen_models;   // holder -> model
// 1168 witness: C3AD90(model, context) attaches the ChrAsmModel's parts (model +28 + 8i, drawable [part+18])
// to the context's layer through C4C3C0 (STEP_Wait_Setup). Logs the part drawables' tables.
using OffscreenPartsAttach=void(__fastcall*)(uintptr_t,uintptr_t);
OffscreenPartsAttach offscreen_parts_attach_original{};
void __fastcall NativeOffscreenPartsAttach(uintptr_t model,uintptr_t context) noexcept {
    offscreen_parts_attach_original(model,context);
    if(control->stop)return;
    static LONG64 reported{};if(InterlockedIncrement64(&reported)>8)return;
    uintptr_t layer{};if(context)Read(context,&layer,8);
    std::string parts;uint32_t present{};
    for(uint32_t i=0;i<24;++i){uintptr_t part{},drawable{},table{};
        if(!Read(model+0x28+8ull*i,&part,8) || !part)continue;++present;
        Read(part+0x18,&drawable,8);if(drawable)Read(drawable,&table,8);
        char cell[64]{};sprintf_s(cell,"%s[%u,%lld]",parts.empty()?"":",",i,table?static_cast<long long>(table-image):0ll);parts+=cell;}
    std::string row="{\"event\":\"native_offscreen_parts_attached\",\"context\":"+std::to_string(context)+",\"layer\":"+std::to_string(layer)+
        ",\"parts\":"+std::to_string(present)+",\"drawables\":["+parts+"]}\n";Log(row.c_str());
}
// 1169: menu view lifetime. Init_Play (D677D0) lists the holder (D64B50) once its parts are loaded and the next step
// (D676D0) unlists it (D65140) on its first call: the original draws each menu view for the frames it stays listed
// and the UI keeps sampling the image. Native policy (explicit): a holder seen listed is armed and drawn every frame
// until its renderer's destructor (D66280: model frees, D65140 and D65160 on [this+B8]) retires it, so textures that
// become resident after the listing frame still reach the image. Witness: the first listings with their callers.
std::unordered_set<uintptr_t> native_offscreen_armed;   // holders (under native_offscreen_lock)
// 1170: the thread that runs the menu step machines (D64B50 caller), and whether the native scene CPU update selected
// the views since the last presentation (it does not run at the title and in Load Game).
std::atomic<DWORD> native_menu_thread{};std::atomic<bool> native_offscreen_selected_since_present{};
using OffscreenList=void(__fastcall*)(uintptr_t);using OffscreenUnlist=uint64_t(__fastcall*)(uintptr_t);
OffscreenList offscreen_list_original{};OffscreenUnlist offscreen_unlist_original{};
void LogOffscreenListing(const char* what,uintptr_t holder,uintptr_t caller) noexcept {
    static LONG64 reported{};if(InterlockedIncrement64(&reported)>60)return;
    BYTE listed{};Read(holder+0x58,&listed,1);uint32_t extent[2]{};Read(holder+0x68,extent,8);float w{},h{};std::memcpy(&w,&extent[0],4);std::memcpy(&h,&extent[1],4);
    char row[256]{};sprintf_s(row,"{\"event\":\"native_offscreen_listing\",\"what\":\"%s\",\"holder\":%llu,\"listed_before\":%u,\"extent\":[%g,%g],\"caller\":%llu}\n",
        what,static_cast<unsigned long long>(holder),listed,w,h,static_cast<unsigned long long>(caller-image));Log(row);
}
void __fastcall NativeOffscreenList(uintptr_t holder) noexcept {
    if(!control->stop){native_menu_thread=GetCurrentThreadId();LogOffscreenListing("list",holder,reinterpret_cast<uintptr_t>(_ReturnAddress()));
        const std::lock_guard hold(native_offscreen_lock);native_offscreen_armed.insert(holder);}
    offscreen_list_original(holder);}
uint64_t __fastcall NativeOffscreenUnlist(uintptr_t holder) noexcept {
    if(!control->stop)LogOffscreenListing("unlist",holder,reinterpret_cast<uintptr_t>(_ReturnAddress()));return offscreen_unlist_original(holder);}
struct NativeOffscreenScene {uintptr_t holder{},proxy{};std::shared_ptr<const NativeSelectedScene> scene;std::array<float,4> background{};ds3native::NativeOffscreenTone tone{};};
std::vector<NativeOffscreenScene> native_offscreen_scenes;   // the latest selection per armed view (under native_offscreen_lock)
// I-0113 witness (a Load Game portrait missing now and then, i100-tr7-night v6): each armed view's outcome when it changes (a
// skip reason, or queued for drawing with its prepared meshes and draws; « unplaced », I-0113 tranche 19: in those two fields the
// models admitted and the items skipped as non-finite). A view never listed or armed leaves no row (final
// review L5). At most 600 rows; once they are spent the witness costs one atomic read (step review L2).
void NoteNativeOffscreenViewState(uintptr_t holder,uint32_t width,const char* state,size_t prepared,size_t draws) noexcept {
    static std::mutex lock;static std::unordered_map<uintptr_t,std::string> last;static std::atomic<int> rows{};
    if(rows.load(std::memory_order_relaxed)>=600)return;
    char key[96]{};sprintf_s(key,"%s:%zu:%zu",state,prepared,draws);
    {const std::lock_guard hold(lock);if(rows.load(std::memory_order_relaxed)>=600)return;auto& previous=last[holder];if(previous==key)return;previous=key;rows.fetch_add(1,std::memory_order_relaxed);}
    char row[256]{};sprintf_s(row,"{\"event\":\"native_offscreen_view_state\",\"holder\":%llu,\"width\":%u,\"state\":\"%s\",\"prepared\":%zu,\"draws\":%zu}\n",
        static_cast<unsigned long long>(holder),width,state,prepared,draws);Log(row);}
// 1172 RCA witness (both modes): the order of menu-model teardown events at shutdown (renderer destructor D66280,
// model release D68EB0, body-scale modifier destructor D123F0), with a shared sequence number and the caller.
std::atomic<LONG64> teardown_sequence{};
void LogTeardown(const char* what,uintptr_t object,uintptr_t caller) noexcept {
    const auto n=++teardown_sequence;if(n>80)return;
    char row[256]{};sprintf_s(row,"{\"event\":\"menu_model_teardown\",\"sequence\":%lld,\"tick\":%llu,\"what\":\"%s\",\"object\":%llu,\"caller\":%llu,\"thread\":%lu}\n",
        static_cast<long long>(n),static_cast<unsigned long long>(GetTickCount64()),what,static_cast<unsigned long long>(object),static_cast<unsigned long long>(caller-image),GetCurrentThreadId());Log(row);
}
using BodyScaleModifierRetire=void(__fastcall*)(uintptr_t);
BodyScaleModifierRetire body_scale_modifier_retire_original{};
void __fastcall ObserveBodyScaleModifierRetire(uintptr_t modifier) noexcept {
    LogTeardown("body_scale_modifier_dtor",modifier,reinterpret_cast<uintptr_t>(_ReturnAddress()));body_scale_modifier_retire_original(modifier);}
void ForgetBodyScaleModifier(uintptr_t modifier) noexcept;   // below: the D126B0 guard's record (I-0137 tranche 7)
// I-0137 tranche 7 (its step review's L3): natively the modifier's destructor D123F0 also leaves the guard's record; its teardown row
// stays the witness's (update-observe.flag), as ObserveBodyScaleModifierRetire's.
void __fastcall NativeBodyScaleModifierRetire(uintptr_t modifier) noexcept {
    static const bool witness=ModuleFlag(L"update-observe.flag");
    if(witness)LogTeardown("body_scale_modifier_dtor",modifier,reinterpret_cast<uintptr_t>(_ReturnAddress()));
    ForgetBodyScaleModifier(modifier);body_scale_modifier_retire_original(modifier);}
// 1172 RCA witness (both modes): the deferred-deletion queue of menu objects (8190B0 enqueues into [14477D778] ->
// [144964930]; AED060 queues the ten portraits) and its per-frame flush step EE42B0 (142355A10 + 819450).
using DeferredEnqueue=BYTE(__fastcall*)(uintptr_t,uintptr_t);DeferredEnqueue deferred_enqueue_original{};
using DeferredFlush=void(__fastcall*)(uintptr_t);DeferredFlush deferred_flush_original{};
BYTE __fastcall ObserveDeferredEnqueue(uintptr_t queue,uintptr_t object) noexcept {
    uintptr_t table{};if(object)Read(object,&table,8);
    LogTeardown("deferred_enqueue",table?table:object,reinterpret_cast<uintptr_t>(_ReturnAddress()));return deferred_enqueue_original(queue,object);}
void __fastcall ObserveDeferredFlush(uintptr_t step) noexcept {
    static LONG64 calls{};const auto n=InterlockedIncrement64(&calls);
    if(n<=3 || n%600==0){char row[160]{};sprintf_s(row,"{\"event\":\"deferred_flush_step\",\"call\":%lld,\"thread\":%lu}\n",static_cast<long long>(n),GetCurrentThreadId());Log(row);}
    deferred_flush_original(step);}
using TitleDialogRetire=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t);TitleDialogRetire title_dialog_retire_original{};
void __fastcall ObserveTitleDialogRetire(uintptr_t dialog,uintptr_t a,uintptr_t b,uintptr_t c) noexcept {
    LogTeardown("title_top_dialog_dtor",dialog,reinterpret_cast<uintptr_t>(_ReturnAddress()));
    {void* frames[16]{};const auto depth=RtlCaptureStackBackTrace(1,16,frames,nullptr);std::string stack;   // 1173: who deletes the title
        for(USHORT i=0;i<depth;++i){const auto at=reinterpret_cast<uintptr_t>(frames[i]);char cell[24]{};
            sprintf_s(cell,"%s%llu",stack.empty()?"":",",static_cast<unsigned long long>(at>=image?at-image:0));stack+=cell;}
        const std::string row="{\"event\":\"title_top_dialog_dtor_stack\",\"tick\":"+std::to_string(GetTickCount64())+",\"stack\":["+stack+"]}\n";Log(row.c_str());}
    title_dialog_retire_original(dialog,a,b,c);}
// I-0100 tranche 8 (popin-observe.flag, Reference): DS3's encoded texture and cube creations (2B260, AC110, the sites the native
// replaces with AdmitEncodedTexture and AdmitEncodedCube) passed through and counted, the original's side of popin_texture_creations.
using EncodedTextureCreate=void(__fastcall*)(void*,void*);EncodedTextureCreate popin_texture_create_original{},popin_cube_create_original{};
void __fastcall PopinTextureCreate(void* wrapper,void* source) noexcept {popin_texture_create_original(wrapper,source);popin_texture_creations.fetch_add(1,std::memory_order_relaxed);}
void __fastcall PopinCubeCreate(void* wrapper,void* source) noexcept {popin_cube_create_original(wrapper,source);popin_texture_creations.fetch_add(1,std::memory_order_relaxed);}
using MovieFactory=uintptr_t(__fastcall*)();MovieFactory movie_factory_original{};
uintptr_t __fastcall ObserveMovieFactory() noexcept {
    const auto player=movie_factory_original();LogTeardown("movie_player_created",player,reinterpret_cast<uintptr_t>(_ReturnAddress()));return player;}
// 1173 RCA (both modes, pass-through): AppMenu window close A4E7C0 (vt+0x50; +0x358 = close already requested), with
// the window's vtable and the callers, to see whether TitleTopDialog (vtable 28D39C8) is ever asked to close natively.
using WindowClose=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t);WindowClose window_close_original{};
void __fastcall ObserveWindowClose(uintptr_t window,uintptr_t result,uintptr_t c,uintptr_t d) noexcept {
    static LONG64 calls{};
    // I-0100 tranche 8 (popin-observe.flag, M2): the loading page's window (KnowledgeLoadingScreen, vtable 28EF918) asked to close.
    if(PopinObserve()){uintptr_t vtable{};if(Read(window,&vtable,8) && vtable==image+0x28EF918){LARGE_INTEGER now{};QueryPerformanceCounter(&now);
        popin_page_closed.store(now.QuadPart,std::memory_order_relaxed);char row[160]{};
        sprintf_s(row,"{\"event\":\"popin_page_close\",\"tick\":%llu,\"qpc\":%lld,\"textures\":%llu}\n",static_cast<unsigned long long>(GetTickCount64()),
            static_cast<long long>(now.QuadPart),static_cast<unsigned long long>(popin_texture_creations.load(std::memory_order_relaxed)));Log(row);}}
    if(InterlockedIncrement64(&calls)<=64){uintptr_t vtable{};BYTE requested{};Read(window,&vtable,8);Read(window+0x358,&requested,1);
        void* frames[6]{};const auto depth=RtlCaptureStackBackTrace(1,6,frames,nullptr);std::string stack;
        for(USHORT i=0;i<depth;++i){const auto at=reinterpret_cast<uintptr_t>(frames[i]);char cell[24]{};
            sprintf_s(cell,"%s%llu",stack.empty()?"":",",static_cast<unsigned long long>(at>=image?at-image:0));stack+=cell;}
        char row[384]{};sprintf_s(row,"{\"event\":\"menu_window_close\",\"tick\":%llu,\"window\":%llu,\"vtable\":%llu,\"requested_before\":%u,\"result\":%llu,\"stack\":[%s],\"thread\":%lu}\n",
            static_cast<unsigned long long>(GetTickCount64()),static_cast<unsigned long long>(window),static_cast<unsigned long long>(vtable>=image?vtable-image:0),
            requested,static_cast<unsigned long long>(result),stack.c_str(),GetCurrentThreadId());Log(row);}
    window_close_original(window,result,c,d);}
// 1173 RCA (both modes, pass-through): TitleStep (vtable 28651B0) state machine. Slots +0x10 (90A4A0) and +0x20 (90A520)
// run the state [step+0x10] through the table [step+0x20]; the title dialog dies with the step (909EB0, Reference1173).
using TitleStepRun=uintptr_t(__fastcall*)(uintptr_t,float,uintptr_t,uintptr_t);TitleStepRun title_step_run_a_original{},title_step_run_b_original{};
void ObserveTitleStepState(uintptr_t step,const char* slot) noexcept {
    static std::mutex lock;static std::unordered_map<uintptr_t,int32_t> last;static LONG64 rows{};
    int32_t state{};uintptr_t table{},function{};int32_t mode{};
    if(!Read(step+0x10,&state,4) || !Read(step+0x20,&table,8))return;
    if(table)Read(table+static_cast<uintptr_t>(static_cast<int64_t>(state))*0x10,&function,8);Read(image+0x478D868,&mode,4);
    const std::lock_guard hold(lock);auto [at,fresh]=last.try_emplace(step,state);
    if(!fresh && at->second==state)return;at->second=state;if(++rows>200)return;
    uintptr_t vtable{};Read(step,&vtable,8);
    char row[320]{};sprintf_s(row,"{\"event\":\"title_step_state\",\"tick\":%llu,\"slot\":\"%s\",\"step\":%llu,\"vtable\":%llu,\"state\":%d,\"function\":%llu,\"new_game_mode\":%d}\n",
        static_cast<unsigned long long>(GetTickCount64()),slot,static_cast<unsigned long long>(step),static_cast<unsigned long long>(vtable>=image?vtable-image:0),
        state,static_cast<unsigned long long>(function>=image?function-image:0),mode);Log(row);}
uintptr_t __fastcall ObserveTitleStepRunA(uintptr_t step,float dt,uintptr_t c,uintptr_t d) noexcept {
    ObserveTitleStepState(step,"10");const auto r=title_step_run_a_original(step,dt,c,d);ObserveTitleStepState(step,"10");return r;}
uintptr_t __fastcall ObserveTitleStepRunB(uintptr_t step,float dt,uintptr_t c,uintptr_t d) noexcept {
    ObserveTitleStepState(step,"20");const auto r=title_step_run_b_original(step,dt,c,d);ObserveTitleStepState(step,"20");return r;}
using TitleStepRetire=void(__fastcall*)(uintptr_t);TitleStepRetire title_step_retire_original{};
std::atomic<bool> title_step_retired{};
void __fastcall ObserveTitleStepRetire(uintptr_t step) noexcept {
    LogTeardown("title_step_dtor",step,reinterpret_cast<uintptr_t>(_ReturnAddress()));title_step_retire_original(step);title_step_retired=true;}
// 1173 RCA (pass-through, behind update-observe.flag natively; NativeBodyScaleUpdate below is the native guard): CSFD4LocationBodyScaleModifier update D126B0(modifier, matrices, count, first) reads the c0000
// body-scale table [[modifier+0x58]+0x18]. Rows when a modifier is first seen, when that table pointer changes, and
// every call after the title step is gone (quit), to compare its lifetime with the original's.
// I-0137 tranche 7 (its last step review's H2, READ in the bytes): slots 6 and 7 take (this, matrices, count, first): D126B0 and
// D12AA0 keep r8d and r9d live into D1A7D0 / D1A870, which forward them to the parent location's same slot (D1A420 loops `count` bones
// from `first`); the detours declared two arguments and so called the original with clobbered r8 and r9 (slot 7's Reference witness since
// 2d67d08a, 2026-10-10, unconditionally in the Reference mode; its native guard since tranche 4).
using BodyScaleUpdate=uint64_t(__fastcall*)(uintptr_t,uintptr_t,uint32_t,int32_t);BodyScaleUpdate body_scale_update_original{};
void BodyScaleWitness(uintptr_t modifier) noexcept {
    {static std::mutex lock;static std::unordered_map<uintptr_t,uintptr_t> last;static LONG64 rows{},quit_rows{};
        uintptr_t data{},table{};Read(modifier+0x58,&data,8);if(data)Read(data+0x18,&table,8);
        const bool quit=title_step_retired.load();const std::lock_guard hold(lock);
        auto [at,fresh]=last.try_emplace(modifier,table);const bool changed=!fresh && at->second!=table;at->second=table;
        if(((fresh || changed) && ++rows<=200) || (quit && ++quit_rows<=60)){
            char row[320]{};sprintf_s(row,"{\"event\":\"body_scale_update\",\"tick\":%llu,\"modifier\":%llu,\"data\":%llu,\"table\":%llu,\"fresh\":%u,\"changed\":%u,\"after_title_step\":%u,\"modifiers\":%zu,\"thread\":%lu}\n",
                static_cast<unsigned long long>(GetTickCount64()),static_cast<unsigned long long>(modifier),static_cast<unsigned long long>(data),
                static_cast<unsigned long long>(table),fresh?1u:0u,changed?1u:0u,quit?1u:0u,last.size(),GetCurrentThreadId());Log(row);}}}
uint64_t __fastcall ObserveBodyScaleUpdate(uintptr_t modifier,uintptr_t matrices,uint32_t count,int32_t first) noexcept {
    BodyScaleWitness(modifier);return body_scale_update_original(modifier,matrices,count,first);}
// ROADMAP workstream 4 step 5 (I-0137): DS3's own fault at a window close during the character creation (1173: D12759 in both modes,
// [[modifier+0x58]+0x18] the c0000 body-scale table already released while the location update still runs). Natively the update
// of a modifier whose table is gone changes none of this modifier's scale: the parent location's slot runs (D1A7D0, below) and its
// count is returned; a row at the 1st, 2nd, 4th, 8th... skip with the count (the final review's M2:
// the title step's end is known only to a witness hook; a skip in play shows as rows before the exit's). The Reference mode keeps
// the original's behaviour.
// I-0137 tranche 7 (the witness `i66-native-i137-t6-charmake`, 2026-11-13 06:39: at least two skips with the table null, then an
// update past the guard faulted at D12759 on a table pointer that was other data, 0x3EB294A93E8B8421: the block had been freed and
// reused). READ (evidence/I-0137/t7-bodyscale-ghidra.md): a modifier's +0x58 is an interior pointer into a FlverResCap's FlverModel
// block, [node+0x68] + 0xC8, given by its constructor D12370's second argument (two callers: the menu model D690A0 and the cutscene
// actor CC2AD0, both on the c0000 node); that block is freed only in the node's destructor body D23910 (after the node has left the
// repository's name map), which clears [node+0x68] after the free. So the module records each modifier's block at its construction
// and marks it dead when D23910 frees the block that holds it; an update of a dead modifier is skipped as above (`released` in its
// row). Only the module's own state is read on the update path, under its lock; a release on another thread between that check and
// the original's read is not covered (both were on the engine thread in the witness).
struct BodyScaleModifierBlock {uintptr_t data{};bool released{};};
std::mutex body_scale_modifiers_lock;
std::unordered_map<uintptr_t,BodyScaleModifierBlock>& body_scale_modifiers=*new std::unordered_map<uintptr_t,BodyScaleModifierBlock>;
using BodyScaleModifierConstruct=uintptr_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
BodyScaleModifierConstruct body_scale_modifier_construct_original{};
uintptr_t __fastcall NativeBodyScaleModifierConstruct(uintptr_t self,uintptr_t data,uintptr_t third,uintptr_t fourth,uintptr_t fifth) noexcept {
    const auto result=body_scale_modifier_construct_original(self,data,third,fourth,fifth);
    try{const std::lock_guard hold(body_scale_modifiers_lock);
        if(body_scale_modifiers.size()>=4096)std::erase_if(body_scale_modifiers,[](const auto& entry){return entry.second.released;});
        body_scale_modifiers.insert_or_assign(self,BodyScaleModifierBlock{data,false});}catch(...){}
    return result;}
using FlverResCapRelease=uintptr_t(__fastcall*)(uintptr_t);FlverResCapRelease flver_res_cap_release_original{};
void ForgetBodyScaleModifier(uintptr_t modifier) noexcept {
    try{const std::lock_guard hold(body_scale_modifiers_lock);body_scale_modifiers.erase(modifier);}catch(...){}}
uintptr_t __fastcall NativeFlverResCapRelease(uintptr_t node) noexcept {
    uintptr_t block{};
    if(Read(node+0x68,&block,8) && block){const std::lock_guard hold(body_scale_modifiers_lock);
        for(auto& [modifier,entry]:body_scale_modifiers)if(entry.data>=block && entry.data<block+0x148)entry.released=true;}
    return flver_res_cap_release_original(node);}
// I-0137 tranche 8 (the RCA of DS3's fault at the window close during the creation, RESULT « Tranche 8 »): DS3's own quit request,
// the byte 14489D5B8 its main loop tick EDAAE0 sets once the window is gone (READ: `D5B8 |= !(window->vt+0x10() && !runtime->vt+0x98())`,
// the runtime's slot constant false in this build; its only writer, an OR, never cleared: its step review's search). From it the title
// step leaves its state 9 for 10, its dialog dies and the c0000 resources go (I-0066 MENU-OFFSCREEN-MODELS « Corrected (1173) »),
// while the creation's menu model renderers and their body-scale modifiers stay in the update tasks until the step shuts down: the
// original faults there too. Natively three readers of the menus' models are gated on the quit request: the renderer's update D68520,
// the body-scale slots and the module's own collect of the menu views. Latched once seen (the byte only goes from 0 to 1).
bool NativeQuitRequested() noexcept {
    static std::atomic<bool> latched{};if(latched.load(std::memory_order_relaxed))return true;
    BYTE quit{};if(!Read(image+0x489D5B8,&quit,1) || !quit)return false;
    latched.store(true,std::memory_order_relaxed);return true;}
// Whether a modifier's table may be read: not released (its record's block the one its +0x58 still points into: the step review's
// L4), +0x58 and the table not null; else one row at the 1st, 2nd, 4th... skip of any slot, named.
bool BodyScaleModifierReadable(uintptr_t modifier,const char* slot) noexcept {
    uintptr_t data{},table{};const bool pointer=Read(modifier+0x58,&data,8) && data,quit=NativeQuitRequested();bool released{};
    if(pointer && !quit){const std::lock_guard hold(body_scale_modifiers_lock);const auto found=body_scale_modifiers.find(modifier);
        released=found!=body_scale_modifiers.end() && found->second.released && found->second.data==data;}
    if(pointer && !quit && !released && Read(data+0x18,&table,8) && table)return true;
    static LONG64 skipped{};if(const auto n=InterlockedIncrement64(&skipped);!(n&(n-1))){char row[224]{};
        sprintf_s(row,"{\"event\":\"native_body_scale_update_skipped\",\"slot\":\"%s\",\"quit\":%s,\"released\":%s,\"data\":%s,\"table\":%s,\"count\":%lld}\n",
            slot,quit?"true":"false",released?"true":"false",data?"true":"false",table?"true":"false",n);Log(row);}
    return false;}
// A skipped call runs what the original runs before its own scaling, with the same arguments: D1A7D0 (slot 7), D1A830 (4), D1A790 (5)
// or D1A870 (6) on +0x38, the parent location's same slot when there is one (READ: `call D1A000; ... jmp [rax+slot]` with rdx and r8 forwarded,
// and r9 for slots 6 and 7), and returns its count; only this modifier's body scale is left out.
uint64_t __fastcall NativeBodyScaleUpdate(uintptr_t modifier,uintptr_t matrices,uint32_t count,int32_t first) noexcept {
    static const bool witness=ModuleFlag(L"update-observe.flag");if(witness)BodyScaleWitness(modifier);
    if(!BodyScaleModifierReadable(modifier,"update"))
        return reinterpret_cast<BodyScaleUpdate>(image+0xD1A7D0)(modifier+0x38,matrices,count,first)&0xFFFFFFFFull;
    return body_scale_update_original(modifier,matrices,count,first);}
// The step review's M1: slots 4, 5 and 6 of the same class (D12930, D124B0, D12AA0) read the same table the same way (READ: each
// `[[modifier+0x58]+0x18]` indexed by the mapped bone), so each is guarded alike. Slots 4 and 5 take (this, matrix, bone), slot 6 the
// four arguments of slot 7 (READ in the bytes, the last step review).
using BodyScaleBoneCall=uint64_t(__fastcall*)(uintptr_t,uintptr_t,int32_t);
BodyScaleBoneCall body_scale_slot4_original{},body_scale_slot5_original{};BodyScaleUpdate body_scale_slot6_original{};
uint64_t __fastcall NativeBodyScaleSlot4(uintptr_t modifier,uintptr_t matrix,int32_t bone) noexcept {
    if(!BodyScaleModifierReadable(modifier,"slot4"))return reinterpret_cast<BodyScaleBoneCall>(image+0xD1A830)(modifier+0x38,matrix,bone)&0xFFFFFFFFull;
    return body_scale_slot4_original(modifier,matrix,bone);}
uint64_t __fastcall NativeBodyScaleSlot5(uintptr_t modifier,uintptr_t matrix,int32_t bone) noexcept {
    if(!BodyScaleModifierReadable(modifier,"slot5"))return reinterpret_cast<BodyScaleBoneCall>(image+0xD1A790)(modifier+0x38,matrix,bone)&0xFFFFFFFFull;
    return body_scale_slot5_original(modifier,matrix,bone);}
uint64_t __fastcall NativeBodyScaleSlot6(uintptr_t modifier,uintptr_t matrices,uint32_t count,int32_t first) noexcept {
    if(!BodyScaleModifierReadable(modifier,"slot6"))return reinterpret_cast<BodyScaleUpdate>(image+0xD1A870)(modifier+0x38,matrices,count,first)&0xFFFFFFFFull;
    return body_scale_slot6_original(modifier,matrices,count,first);}
// 1175 (both modes, pass-through): menu sound requests. 4AD510(out, category, id, -1) builds the FMOD event of every menu
// sound and music request (MENUS-MAP 3, 28: 76B930 category 2 'f', 76B920 category 6 'm'); the MenuMan sound object
// [144763868] plays it. Rows for the first 120 requests, with the sound object, to compare the menu sound traffic.
// Audio census (audio-census.flag, both modes): every FMOD::Event::start the game makes, named through Event::getInfo. The
// first character of an event name is its category in the game's banks (the menu requests of 4AD510 use 'f' and 'm'); a row
// for each new name (at most 3000), and a summary by category and result every 2000 starts and every 30 s of starts.
using AudioEventStart=int(__fastcall*)(void*);AudioEventStart audio_event_start_original{};
using AudioEventInfo=int(__fastcall*)(void*,int*,char**,void*);AudioEventInfo audio_event_info{};
std::mutex audio_census_lock;std::map<std::string,uint64_t> audio_census_names;std::map<char,uint64_t> audio_census_categories;
uint64_t audio_census_starts{},audio_census_failures{},audio_census_last_tick{};
int __fastcall ObserveAudioEventStart(void* event) noexcept {
    char* name{};int index{};const int info=audio_event_info?audio_event_info(event,&index,&name,nullptr):-1;
    const int result=audio_event_start_original(event);
    try{std::string text;if(!info && name)for(size_t i=0;i<96 && name[i];++i){const char c=name[i];text.push_back(c>=32 && c<127 && c!='"' && c!='\\'?c:'?');}
        const std::lock_guard hold(audio_census_lock);++audio_census_starts;if(result)++audio_census_failures;
        ++audio_census_categories[text.empty()?'?':text[0]];
        const bool fresh=++audio_census_names[text]==1;const auto tick=GetTickCount64();
        if(fresh && audio_census_names.size()<=3000){   // with the game-image return addresses of the call chain (the producer)
            void* frames[16]{};const auto depth=RtlCaptureStackBackTrace(1,16,frames,nullptr);std::string stack;
            for(USHORT i=0;i<depth;++i){const auto at=reinterpret_cast<uintptr_t>(frames[i]);
                if(at>=image && at<image+0x4800000){stack+=stack.empty()?"":",";char hex[16]{};sprintf_s(hex,"%llX",static_cast<unsigned long long>(at-image));stack+=hex;}}
            char row[512]{};sprintf_s(row,"{\"event\":\"audio_event_name\",\"name\":\"%s\",\"result\":%d,\"tick\":%llu,\"stack\":\"%.300s\"}\n",
            text.c_str(),result,static_cast<unsigned long long>(tick),stack.c_str());Log(row);}
        if(audio_census_starts%2000==0 || tick-audio_census_last_tick>=30000){audio_census_last_tick=tick;std::string categories;
            for(const auto& [c,n]:audio_census_categories){categories+=categories.empty()?"":",";categories+="\""+std::string(1,c)+"\":"+std::to_string(n);}
            const std::string row="{\"event\":\"audio_census\",\"starts\":"+std::to_string(audio_census_starts)+",\"failures\":"+std::to_string(audio_census_failures)+
                ",\"names\":"+std::to_string(audio_census_names.size())+",\"categories\":{"+categories+"},\"tick\":"+std::to_string(tick)+"}\n";Log(row.c_str());}
    }catch(...){}
    return result;
}
// The request side (audio-census.flag): FUN_1404AD2D0(out, category, id) forms every sound name (the category letter of the
// table 142786578 and nine digits) for its twelve callers (menu 4AD510/4AD240, the sound module 7CEDE0, effects D53940, ...).
// A row for each new name, with the game-image return addresses of its caller chain, so a name the native path never forms
// shows which producer is silent.
using AudioNameBuild=uintptr_t(__fastcall*)(uintptr_t,uint32_t,int32_t);AudioNameBuild audio_name_build_original{};
std::set<uint64_t> audio_census_requests;
thread_local bool audio_in_play{};thread_local uint64_t audio_formed{};
uintptr_t __fastcall ObserveAudioNameBuild(uintptr_t out,uint32_t category,int32_t id) noexcept {
    const auto result=audio_name_build_original(out,category,id);
    if(audio_in_play && audio_formed==UINT64_MAX)audio_formed=uint64_t(category)<<32|uint32_t(id);   // the first name of the play step
    try{const uint64_t key=uint64_t(category)<<32|uint32_t(id);bool fresh{};
        {const std::lock_guard hold(audio_census_lock);fresh=audio_census_requests.size()<3000 && audio_census_requests.insert(key).second;}
        if(fresh){void* frames[16]{};const auto depth=RtlCaptureStackBackTrace(1,16,frames,nullptr);std::string stack;
            for(USHORT i=0;i<depth;++i){const auto at=reinterpret_cast<uintptr_t>(frames[i]);
                if(at>=image && at<image+0x4800000){stack+=stack.empty()?"":",";char hex[16]{};sprintf_s(hex,"%llX",static_cast<unsigned long long>(at-image));stack+=hex;}}
            char row[512]{};sprintf_s(row,"{\"event\":\"audio_request_name\",\"category\":%u,\"id\":%d,\"tick\":%llu,\"stack\":\"%.300s\"}\n",category,id,
                static_cast<unsigned long long>(GetTickCount64()),stack.c_str());Log(row);}
    }catch(...){}
    return result;
}
// FUN_140E72D50(owner, position, request {category, id}, filter, emitter): the character chain's play step. Per call the
// incoming request and what it formed (the filter's refusal leaves nothing; the emitter may rewrite the id): one row for each new
// pair (incoming, formed), then counts in audio_play_summary every 4000 calls.
using AudioPlay=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);AudioPlay audio_play_original{};
std::map<std::pair<uint64_t,uint64_t>,uint64_t> audio_play_pairs;uint64_t audio_play_calls{},audio_play_refused{};
void __fastcall ObserveAudioPlay(uintptr_t owner,uintptr_t position,uintptr_t request,uintptr_t filter,uintptr_t emitter) noexcept {
    uint64_t incoming{};Read(request,&incoming,8);
    const bool outer_in_play=audio_in_play;const uint64_t outer_formed=audio_formed;audio_in_play=true;audio_formed=UINT64_MAX;
    audio_play_original(owner,position,request,filter,emitter);
    const auto formed=audio_formed;audio_in_play=outer_in_play;audio_formed=outer_formed;   // nested steps keep the outer probe
    try{const std::lock_guard hold(audio_census_lock);++audio_play_calls;if(formed==UINT64_MAX)++audio_play_refused;
        const bool fresh=++audio_play_pairs[{incoming,formed}]==1;
        if(fresh && audio_play_pairs.size()<=3000){char row[256]{};
            sprintf_s(row,"{\"event\":\"audio_play_pair\",\"in\":[%u,%d],\"formed\":[%d,%d],\"tick\":%llu}\n",uint32_t(incoming),int32_t(incoming>>32),
                formed==UINT64_MAX?-1:int32_t(formed>>32),formed==UINT64_MAX?-1:int32_t(uint32_t(formed)),static_cast<unsigned long long>(GetTickCount64()));Log(row);}
        if(audio_play_calls%4000==0){char row[160]{};sprintf_s(row,"{\"event\":\"audio_play_summary\",\"calls\":%llu,\"refused\":%llu,\"pairs\":%zu}\n",
            static_cast<unsigned long long>(audio_play_calls),static_cast<unsigned long long>(audio_play_refused),audio_play_pairs.size());Log(row);}
    }catch(...){}
}
using SoundEvent=uintptr_t(__fastcall*)(uintptr_t,uint32_t,int32_t,int32_t);SoundEvent sound_event_original{};
uintptr_t __fastcall ObserveSoundEvent(uintptr_t out,uint32_t category,int32_t id,int32_t extra) noexcept {
    const auto result=sound_event_original(out,category,id,extra);
    // I-0132 R12h's third tranche (the fires' sound for R, the coordinator 13:57): each distinct (category, id) logged once with its first
    // caller, up to 600 of them (a world's first seconds request far more than the 120 rows the menus needed).
    static std::mutex sound_seen_lock;static std::set<std::pair<uint32_t,int32_t>> sound_seen;
    static const bool distinct=ModuleFlag(L"sound-observe.flag");   // the step review's L9: the Reference's witness keeps its first 120 rows
    static std::atomic<bool> full{};
    if(distinct){if(full.load(std::memory_order_relaxed))return result;
        const std::lock_guard hold(sound_seen_lock);if(sound_seen.size()>=600){full=true;return result;}if(!sound_seen.emplace(category,id).second)return result;}
    static LONG64 rows{};if(InterlockedIncrement64(&rows)<=(distinct?600:120)){uintptr_t menu_sound{};Read(image+0x4763868,&menu_sound,8);
        char row[256]{};sprintf_s(row,"{\"event\":\"sound_event_request\",\"tick\":%llu,\"category\":%u,\"id\":%d,\"menu_sound_object\":%u,\"caller\":%llu}\n",
            static_cast<unsigned long long>(GetTickCount64()),category,id,menu_sound?1u:0u,static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(_ReturnAddress())-image));Log(row);}
    return result;}
// 1176 (both modes, pass-through): C4D160(part, mask) writes a character part's mesh-selection mask into its drawable's
// +0x2B0/+0x2B8 (the capability words the passes test, group +0xC0/+0xC8): the mask passed and the part's slot.
using PartMeshMask=void(__fastcall*)(uintptr_t,const uint64_t*);PartMeshMask part_mesh_mask_original{};
void __fastcall ObservePartMeshMask(uintptr_t part,const uint64_t* mask) noexcept {
    part_mesh_mask_original(part,mask);
    static LONG64 rows{};if(InterlockedIncrement64(&rows)<=80){uint64_t words[2]{},applied[2]{};uintptr_t entity{};int32_t slot{-1};
        if(mask)Read(reinterpret_cast<uintptr_t>(mask),words,16);Read(part+0x118,&slot,4);if(Read(part+0x18,&entity,8) && entity)Read(entity+0x2B0,applied,16);
        char row[320]{};sprintf_s(row,"{\"event\":\"part_mesh_mask\",\"mode\":\"%s\",\"tick\":%llu,\"part\":%llu,\"slot\":%d,\"mask\":[\"%016llX\",\"%016llX\"],\"applied\":[\"%016llX\",\"%016llX\"],\"caller\":%llu}\n",
            UiObservationMode(),static_cast<unsigned long long>(GetTickCount64()),static_cast<unsigned long long>(part),slot,static_cast<unsigned long long>(words[0]),static_cast<unsigned long long>(words[1]),
            static_cast<unsigned long long>(applied[0]),static_cast<unsigned long long>(applied[1]),static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(_ReturnAddress())-image));Log(row);}}
// 1177 (both modes, pass-through): D3DA20(item, pass, group, entity, ...) draws a disp item (here a menu part): the group colour +0xD0
// (instance row +0x70, the hair PS alpha) and tint +0x40 it is drawn with, and the item's own colour +0x780.
using MenuItemDraw=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);MenuItemDraw menu_item_draw_original{};
void __fastcall ObserveMenuItemDraw(uintptr_t item,uintptr_t pass,uintptr_t group,uintptr_t entity,uintptr_t e,uintptr_t f,uintptr_t g) noexcept {
    menu_item_draw_original(item,pass,group,entity,e,f,g);
    static LONG64 rows{};if(InterlockedIncrement64(&rows)<=200){std::array<float,4> colour{},tint{},own{};Read(group+0xD0,colour.data(),16);Read(group+0x40,tint.data(),16);Read(item+0x780,own.data(),16);
        uint32_t pass_type{};Read(pass+0x124,&pass_type,4);BYTE use{},pick{};uintptr_t model{},first{},second{};Read(item+0x6F2,&use,1);Read(item+0x6F3,&pick,1);
        if(Read(item+0x58,&model,8) && model){Read(model+0x358,&first,8);Read(model+0x360,&second,8);}
        char row[512]{};sprintf_s(row,"{\"event\":\"menu_item_draw\",\"mode\":\"%s\",\"pass\":%u,\"group_colour\":[%g,%g,%g,%g],\"group_tint\":[%g,%g,%g,%g],\"item_colour\":[%g,%g,%g,%g],\"use_model_texture\":%u,\"pick\":%u,\"t358\":%u,\"t360\":%u}\n",
            UiObservationMode(),pass_type,colour[0],colour[1],colour[2],colour[3],tint[0],tint[1],tint[2],tint[3],own[0],own[1],own[2],own[3],use,pick,first?1u:0u,second?1u:0u);Log(row);}}
using LocationStepFinish=uint64_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t);LocationStepFinish location_step_finish_original{};
uint64_t __fastcall ObserveLocationStepFinish(uintptr_t step,uintptr_t b,uintptr_t c,uintptr_t d) noexcept {
    LogTeardown("location_step_finish",step,reinterpret_cast<uintptr_t>(_ReturnAddress()));return location_step_finish_original(step,b,c,d);}
using OffscreenRendererRetire=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t);
OffscreenRendererRetire offscreen_renderer_retire_original{};
void __fastcall NativeOffscreenRendererRetire(uintptr_t menu_renderer,uintptr_t a,uintptr_t b,uintptr_t c) noexcept {
    LogTeardown("renderer_dtor",menu_renderer,reinterpret_cast<uintptr_t>(_ReturnAddress()));
    uintptr_t holder{};std::vector<uintptr_t> proxies;
    if(Read(menu_renderer+0xB8,&holder,8) && holder){std::unique_lock hold(native_offscreen_lock);
        native_offscreen_armed.erase(holder);native_offscreen_models.erase(holder);WaitNativeOffscreenHolderIdle(hold,holder);
        std::erase_if(native_offscreen_scenes,[&](const NativeOffscreenScene& s){return s.holder==holder;});
        try{for(const auto& [proxy,owned]:native_offscreen_proxies)if(owned.holder==holder)proxies.push_back(proxy);}catch(...){}
        std::erase_if(native_offscreen_proxies,[&](const auto& entry){return entry.second.holder==holder;});
        static LONG64 reported{};if(InterlockedIncrement64(&reported)<=8){char row[160]{};
            sprintf_s(row,"{\"event\":\"native_offscreen_view_retired\",\"holder\":%llu,\"targets\":%zu}\n",static_cast<unsigned long long>(holder),proxies.size());Log(row);}}
    // I-0087: the view's targets leave with it (each Load Game opening made new portrait targets, a committed target and a
    // CPU descriptor each, kept until the same proxy address came back). A UI image that imported one keeps its own
    // reference until it is destroyed; a later import of a proxy without a view answers 0, as before its scene (I-0137 tranche 4).
    for(const auto proxy:proxies){try{textures.Retire(proxy);}catch(...){}}
    offscreen_renderer_retire_original(menu_renderer,a,b,c);
}
// 1169: D68EB0 frees a renderer's model objects (ChrAsmModel +6E8, +6F0, +870, +820, +818...), called by the face
// views after their single draw ("instant", MENUS-MAP 25) and by the destructor. The view stops being selected
// before its model goes; its target (the last image) stays with the proxy for the UI.
using OffscreenModelRelease=void(__fastcall*)(uintptr_t);
OffscreenModelRelease offscreen_model_release_original{};
// I-0137 tranche 8: a menu model renderer's update D68520 (READ: the method of the renderer's update task at renderer+0xC0, set by
// the constructor D65DD0, called from the task's slot +0x18 under 1423C1D40, which discards its result; it runs the renderer's slots
// +0xA8, its FD4 step machine D66B80, and +0xB0, then its ChrAsm model +0x6E8, the animation +0x810 made from c0000 and its parts,
// then +0xD8 = 1, whose readers are unknown) is not run once DS3's quit is requested (NativeQuitRequested, above): +0xD8 set as its
// end sets it, a row the first time. Its step slots are left out too (the final review: their states reach the ChrAsm model +0x6E8
// and construct objects, D677D0, D67730, D678C0, and a first form calling them dropped the context D66B80 forwards to its children);
// what that costs is the menus' steps no longer advancing during the post-loop, bounded by F14BB0's 100 passes (the step review's L3,
// to read in the witness). The crash `i66-native-i137-t7-charmake` faulted inside D68520's model work (> 14081C2E0 > ... 1412842DD, a
// null at +0x20) once the D126B0 guard had held.
using MenuModelUpdate=void(__fastcall*)(uintptr_t,uintptr_t);MenuModelUpdate menu_model_update_original{};
void __fastcall NativeMenuModelUpdate(uintptr_t menu_renderer,uintptr_t context) noexcept {
    if(NativeQuitRequested()){*reinterpret_cast<uint32_t*>(menu_renderer+0xD8)=1;
        static LONG64 skipped{};if(InterlockedIncrement64(&skipped)==1)Log("{\"event\":\"native_menu_model_update_skipped\",\"reason\":\"quit_requested\"}\n");
        return;}
    menu_model_update_original(menu_renderer,context);}
// I-0140 tranche 28 (D-0301): the menu views' own region. The constructor D65DD0 calls D0C2F0, which adds the region GILM0010 (the
// 60-unit cube at the origin, masks {0x11, 2}: the menu views' pair {1, 2} takes it, the main view's {1, 1} does not) to the
// manager GXRS+0x42B0 only when GILM0010 is registered, the key {1, 10} is in the engine's env-map bank (41990 on GXRS+0x42A8) and
// the manager is on. Natively that bank holds only its default entry (NativeEnvMapCube): the native reads the env-map rows from
// its own gparam banks (NativeEnvMapScale), so the key gate failed on all 11 calls (OBSERVED, i140-t28d-witness-nat: registered 1,
// key_known 0; the original's 11 calls passed it, i140-t28d-witness-orig) and the menu views had no region. D0C2F0 runs, its key
// gate answered from the native banks for that call alone: 41990 on the engine's bank, else whether the key's area file has the id
// in its « EnvMap:ColorScale » row (Banks::EnvColorScale; for the key {1, 10}: default.gparam's row 10). Its two manager calls are
// the native CPU registry's (03_engine_cpu_owners.inc): 65D60 admits D0C2F0's call and turns the manager on when it is the first
// request (natively off at the title, on in the original at every call), 65320 answers 0 to a null handle from D0C2F0.
bool NativeEnvBankAnswer(uintptr_t table,const uint32_t* key,bool* engine=nullptr) noexcept {   // engine: the engine bank's own answer
    const bool own=env_bank_has_original(table,key);if(engine)*engine=own;
    if(own)return true;
    if(!menu_region_key_from_banks || !key)return false;
    uintptr_t root{},bank{};if(!Read(image+0x4961B50,&root,8) || !root || !Read(root+0x42A8,&bank,8) || bank!=table)return false;
    std::shared_ptr<const ds3light::Banks> banks;{const std::lock_guard hold(gparam_lock);banks=gparam_banks;}
    float scale{};menu_region_key_answered=banks && banks->EnvColorScale(key[0],key[1],scale);
    return menu_region_key_answered;
}
// 41990 natively: NativeEnvBankAnswer; with light-observe.flag the setup installs NativeEnvBankHasObserved instead, which notes each
// call by its site (I-0140 tranche 29, 04c's env_bank_witness).
bool __fastcall NativeEnvBankHas(uintptr_t table,const uint32_t* key) noexcept {return NativeEnvBankAnswer(table,key);}
bool __fastcall NativeEnvBankHasObserved(uintptr_t table,const uint32_t* key) noexcept {
    bool engine{};const bool answer=NativeEnvBankAnswer(table,key,&engine);
    env_bank_witness::Note(table,reinterpret_cast<uintptr_t>(_ReturnAddress()),0,key,engine?1:0,answer?1:0,nullptr,false);
    return answer;
}
void __fastcall NativeMenuRegionAdd(uintptr_t slot) noexcept {
    static const bool witness=ModuleFlag(L"light-observe.flag");
    struct Scope {bool was;Scope():was(menu_region_key_from_banks){menu_region_key_from_banks=true;}
        ~Scope(){menu_region_key_from_banks=was;}} scope;   // restored on any exit (the step review's L1)
    if(witness)ObserveMenuRegionAdd(slot);else menu_region_add_original(slot);
}
// Its release: the renderer destructor D66280 calls D0C990(view+0x6D0), which has the same key gate (D0C9F0's key, 41990 on
// GXRS+0x42A8), then returns on a null handle, else tail-calls 664D0(GXRS+0x42B0, handle) (READ: its disassembly). Natively,
// outside D0C2F0's scope, that gate would fail (the engine's bank lacks {1, 10}: INFERRED from key_known 0 at the adds) and the
// region would stay listed (the second step review's H1). Natively the gate is the registry's membership instead: a handle the region
// registry holds goes to 664D0 itself (the same work, its manager check included; the wrapper's slot 0 reaches the registry's 652A0
// replacement); no registry (closed or never made) or a foreign handle: nothing, as the original's gate when it fails. The registry's
// lock is not held across 664D0: 664D0 takes the manager's lock +0x2A0 then, through 652A0's replacement, the registry's, as the
// game's own 664D0 calls do (D09F47's, the map's regions), one order for both (the third step review's M1; the request side, 65D60
// under RequestNativeEnvRegionCpu, no longer holds the registry's lock either: the final review's M1). 664D0 goes through its hook
// ReleaseNativeEnvRegionCpu, which counts it for the module's close and answers 0 after it (a wrapper retired between the
// membership check and the call). A row per release, its first 16, with the thread.
uintptr_t __fastcall NativeMenuRegionRelease(const uintptr_t* slot) noexcept {
    try {
        const uintptr_t wrapper=slot?*slot:0;bool held{};uintptr_t manager{};
        {const std::lock_guard hold(env_region_cpu_lock);
            held=wrapper && env_region_cpu && !env_region_cpu_closed && env_region_cpu->wrappers.contains(wrapper);
            if(held)manager=env_region_cpu->manager;}
        const auto released=held?reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uintptr_t)>(image+0x664D0)(manager,wrapper)&0xFF:0;
        static LONG64 rows{};if(InterlockedIncrement64(&rows)<=16){char row[200]{};
            sprintf_s(row,"{\"event\":\"native_menu_region_release\",\"handle\":\"%llX\",\"held\":%d,\"released\":%d,\"thread\":%lu}\n",
                static_cast<unsigned long long>(wrapper),int(held),int(released),GetCurrentThreadId());Log(row);}
        return released;
    }catch(...){Refuse("native_menu_region_release_failed");}
}
void __fastcall NativeOffscreenModelRelease(uintptr_t menu_renderer) noexcept {
    LogTeardown("model_release",menu_renderer,reinterpret_cast<uintptr_t>(_ReturnAddress()));
    // 1176 (both modes): the parts of the released menu model (renderer +0x6E8, parts at +0x28 + 8i): slot type +0x118 and
    // the item's FLVER model [item+0x58] mesh count +0xA0 / storage +0xE8 (native portraits lose a part: hair?).
    {static LONG64 rows{};uintptr_t model{};
        if(Read(menu_renderer+0x6E8,&model,8) && model && InterlockedIncrement64(&rows)<=24){std::string parts;
            for(uint32_t k=0;k<24;++k){uintptr_t part{},item{},flver{},storage{};int32_t slot{-1},meshes{-1};
                if(!Read(model+0x28+8ull*k,&part,8) || !part)continue;Read(part+0x118,&slot,4);
                if(Read(part+0x10,&item,8) && item && Read(item+0x58,&flver,8) && flver){Read(flver+0xA0,&meshes,4);Read(flver+0xE8,&storage,8);}
                char cell[64]{};sprintf_s(cell,"%s[%u,%d,%d,%u]",parts.empty()?"":",",k,slot,meshes,storage?1u:0u);parts+=cell;}
            uintptr_t table{};Read(menu_renderer,&table,8);
            const std::string row="{\"event\":\"menu_model_parts_at_release\",\"mode\":\""+std::string(UiObservationMode())+"\",\"renderer_table\":"+
                std::to_string(table>=image?table-image:0)+",\"parts\":["+parts+"]}\n";Log(row.c_str());}}
    uintptr_t holder{};
    if(Read(menu_renderer+0xB8,&holder,8) && holder){std::unique_lock hold(native_offscreen_lock);
        const bool armed=native_offscreen_armed.erase(holder)!=0;native_offscreen_models.erase(holder);WaitNativeOffscreenHolderIdle(hold,holder);
        std::erase_if(native_offscreen_scenes,[&](const NativeOffscreenScene& s){return s.holder==holder;});
        static LONG64 reported{};if(armed && InterlockedIncrement64(&reported)<=8){char row[160]{};
            sprintf_s(row,"{\"event\":\"native_offscreen_view_model_released\",\"holder\":%llu}\n",static_cast<unsigned long long>(holder));Log(row);}}
    offscreen_model_release_original(menu_renderer);
}
// I-0140 tranche 31 (D-0303): a Player menu view's traced self-shadow: its shadow state (CollectNativeMenuShadowState) and its
// traced scene from the candidates its admission gathered (CollectNativeMenuRtScene, at this collect's menu walk, taken at the first
// traced view: `walk`), both or neither (the prepare then gives the frame the traced sun, 13b). A failure leaves the view unshadowed
// (its row once), never the view itself. A row on the first 8 views and when a view's outcome changes (by its holder: the creation
// records four Player views of one key and extent a frame; at most 64 rows).
void CollectNativeMenuSelfShadow(NativeSelectedScene& scene,const std::array<uint32_t,2>& key,const std::vector<NativeDeferredCandidate>& rays,uint32_t width,uintptr_t holder,uint64_t& walk) {
    if(!walk)walk=++native_rt_menu_walk;
    NativeRtWalkCounts counts;ds3light::ShadowSunSource source{};const char* failed{};
    try{if(CollectNativeMenuShadowState(scene,key,source))scene.rt_scene=CollectNativeMenuRtScene(rays,walk,counts);}
    catch(const std::exception&){failed="exception";scene.rt_scene.reset();}
    catch(...){failed="exception";scene.rt_scene.reset();}
    if(!scene.rt_scene)scene.shadow_frame.reset();
    // I-0140 tranche 37 (tranche 36's next step: hard and blurred compared in one run): menu-shadow-blur-toggle.flag, a test tool read
    // once, makes this collect look for menu-shadow-hard.now beside the module at most every 250 ms; while that file is there the Player
    // views' blur is off (the traced mask as tranches 31 and 34 drew it), so a runner alternates both states between reopenings. A row
    // at each change, with its tick (the module's clock). `looked` and `hard`: written only here, under native_offscreen_collect_lock.
    static const bool blur_toggle=ModuleFlag(L"menu-shadow-blur-toggle.flag");
    if(blur_toggle && scene.shadow_frame){static uint64_t looked{};static bool hard{};const uint64_t now=GetTickCount64();
        if(!looked || now-looked>=250){looked=now;if(const bool file=ModuleFlag(L"menu-shadow-hard.now");file!=hard){hard=file;char row[128]{};
            sprintf_s(row,"{\"event\":\"native_menu_shadow_blur_test\",\"hard\":%s,\"tick\":%llu}\n",hard?"true":"false",static_cast<unsigned long long>(now));Log(row);}}
        if(hard){scene.shadow_frame->blur_passes=0;scene.shadow_frame->blur_scale=0.f;}}
    static std::mutex lock;static std::unordered_map<uintptr_t,std::array<uint64_t,4>> last;static int rows{};bool fresh{};
    uint64_t blur_key{};if(scene.shadow_frame){uint32_t bits{};std::memcpy(&bits,&scene.shadow_frame->blur_scale,4);blur_key=uint64_t(scene.shadow_frame->blur_passes)<<32|bits;}   // the step review L3
    const std::array<uint64_t,4> now{counts.instances,counts.skinned_instances,uint64_t(source)|(scene.rt_scene?0x100u:0u),blur_key};
    {const std::lock_guard hold(lock);if(rows<64){auto& seen=last[holder];fresh=rows<8 || seen!=now;if(fresh){seen=now;++rows;}}}
    if(!fresh)return;
    static const char* const sources[]{"missing","angle","light_set","blend","cut_light_set","cut_blend"};
    float sun[4]{},colour[3]{},blur_scale{};uint32_t blur_passes{};
    if(scene.shadow_frame){std::memcpy(sun,scene.shadow_frame->sun.data(),16);std::memcpy(colour,scene.shadow_frame->scene_words.data()+0x10,12);
        blur_passes=scene.shadow_frame->blur_passes;blur_scale=scene.shadow_frame->blur_scale;}   // I-0140 t35
    char row[800]{};sprintf_s(row,"{\"event\":\"native_menu_self_shadow\",\"key\":[%u,%u],\"width\":%u,\"walk\":%llu,\"traced\":%s,\"failed\":\"%s\",\"sun_source\":\"%s\",\"sun\":[%g,%g,%g],\"shadow_colour\":[%g,%g,%g],\"blur\":[%u,%g],"
        "\"candidates\":%zu,\"instances\":%llu,\"skinned\":%llu,\"no_storage\":%llu,\"no_storage_kinds\":[%llu,%llu,%llu,%llu],\"no_blas\":%llu,\"no_inputs\":%llu,\"no_pose\":%llu,\"nonfinite\":%llu,\"duplicates\":%llu,\"held\":%llu,\"alpha_instances\":%llu}\n",
        key[0],key[1],width,static_cast<unsigned long long>(walk),scene.rt_scene?"true":"false",failed?failed:"",sources[static_cast<size_t>(source)%6],sun[0],sun[1],sun[2],colour[0],colour[1],colour[2],
        blur_passes,blur_scale,rays.size(),static_cast<unsigned long long>(counts.instances),static_cast<unsigned long long>(counts.skinned_instances),static_cast<unsigned long long>(counts.no_storage),
        static_cast<unsigned long long>(counts.menu_no_storage[0]),static_cast<unsigned long long>(counts.menu_no_storage[1]),static_cast<unsigned long long>(counts.menu_no_storage[2]),
        static_cast<unsigned long long>(counts.menu_no_storage[3]),static_cast<unsigned long long>(counts.no_blas),static_cast<unsigned long long>(counts.no_inputs),static_cast<unsigned long long>(counts.no_pose),
        static_cast<unsigned long long>(counts.nonfinite),static_cast<unsigned long long>(counts.duplicates),static_cast<unsigned long long>(counts.held),static_cast<unsigned long long>(counts.alpha_instances));Log(row);
}
// 1169: each armed menu view is selected on the native scene CPU update, right after the main view's selection (the
// same point in the engine frame as the world entities it already reads): the ChrAsmModel's part drawables
// (SprjModelDispEntity, the map-piece class the selection admits) with the view's camera (the holder's view +28:
// pose +0 = camera world matrix, lens +50 like the main camera output; the holder's own extent replaces the view's
// fixed 1280x720). The light, fog, IBL and tone blocks of the view's own gparam key {D27770(1), id}; no point lights; no shadow
// but a Player view's traced self-shadow (I-0140 tranche 31, D-0303: CollectNativeMenuSelfShadow).
// review 1180: the scene CPU update (under scene_view_cpu_lock) and a presentation on the menu thread (under the
// submission lock) can both collect; the selection path is not reentrant, so one collection at a time. Neither caller
// waits: a collection already running publishes this frame's views, and blocking here would order this lock against
// scene_view_cpu_lock in both directions.
static std::mutex native_offscreen_collect_lock;
void CollectNativeOffscreenScenes(uint64_t selection_serial) {
    if(NativeQuitRequested())return;   // I-0137 tranche 8 (its step review's L4): the menu views read no released c0000 data after the quit
    const std::unique_lock collecting(native_offscreen_collect_lock,std::try_to_lock);if(!collecting.owns_lock())return;
    struct View {uintptr_t holder{},model{},proxy{};uint32_t width{},height{},gparam_id{};bool shadowed{};};
    std::vector<View> views;
    {const std::lock_guard hold(native_offscreen_lock);
        for(const auto& [proxy,owned]:native_offscreen_proxies){const auto model=native_offscreen_models.find(owned.holder);
            if(model==native_offscreen_models.end() || !native_offscreen_armed.contains(owned.holder))continue;
            views.push_back({owned.holder,model->second,proxy,owned.width,owned.height,owned.gparam_id,owned.shadowed});}}
    std::vector<NativeOffscreenScene> scenes;
    static const bool no_menu_shadow=ModuleFlag(L"no-menu-shadow.flag");uint64_t menu_walk{};   // I-0140 t31: this collect's menu walk (0: none yet)
    const auto skip=[](const char* reason,const View& v){NoteNativeOffscreenViewState(v.holder,v.width,reason,0,0);static LONG64 skipped{};if(InterlockedIncrement64(&skipped)>12)return;char row[200]{};
        sprintf_s(row,"{\"event\":\"native_offscreen_view_skipped\",\"reason\":\"%s\",\"width\":%u,\"height\":%u}\n",reason,v.width,v.height);Log(row);};
    // G1: each view is marked busy while it is read (from its camera to its lighting), only if still armed: a retire hook
    // that ran since the listing has unarmed it and is not waited on.
    // And only for the model it was listed with (L5: a holder address retired and bound again since is not read).
    struct Busy {bool owned{};Busy(uintptr_t holder,uintptr_t model){const std::lock_guard hold(native_offscreen_lock);if(!native_offscreen_armed.contains(holder))return;
            if(const auto bound=native_offscreen_models.find(holder);bound==native_offscreen_models.end() || bound->second!=model)return;
            native_offscreen_busy_holder=holder;native_offscreen_busy_thread=GetCurrentThreadId();owned=true;}
        ~Busy(){if(!owned)return;{const std::lock_guard hold(native_offscreen_lock);native_offscreen_busy_holder=0;native_offscreen_busy_thread=0;}native_offscreen_idle.notify_all();}};
    // I-0113 tranche 19: each view's first non-finite collect's tick, and the views selected complete (no item skipped) at least once since they were listed;
    // forgotten here, before the views, once a holder is no longer listed (the step review's L2: also when none is), so a reused
    // address starts afresh. Under native_offscreen_collect_lock.
    static std::unordered_map<uintptr_t,uint64_t> waits;static std::unordered_set<uintptr_t> drawn;
    // I-0140 tranche 40 (D-0305): each view's previous camera for its TAA's motion (upscaler::MenuViewMotion) and the model it was
    // listed with (a holder bound to another model starts afresh), forgotten with the others once the holder is no longer listed.
    struct MenuMotionSeen {upscaler::MenuViewMotionHistory history;Matrix4 pose{};uintptr_t model{};uint64_t published{};};   // I-0140 tranche 41: published, the scene uid its previous collect handed to the presentation (0: that collect did not publish)
    static std::unordered_map<uint64_t,uint32_t> kernel_listings;   // I-0140 tranche 41: by view extent, the listings that chose a kernel (menu-taa-kernel.txt's turn)
    static std::unordered_map<uintptr_t,float> listed_kernels;   // I-0140 tranche 41: each listed holder's kernel, forgotten only once unlisted (its second step review's L3)
    static std::unordered_map<uintptr_t,MenuMotionSeen> motion_seen;
    // I-0140 tranche 30's IBL witness (below): each holder's order of first admission, its admitted collects logged and its model,
    // forgotten here with the others (the fix's step review's M1: also when no view is listed); nothing kept once its rows are spent.
    // A holder bound to another model counts as new (the second fix's M1: an address re-armed before a collect sees it unlisted;
    // `waits` and `drawn` keep the address alone, left as they were).
    struct IblSeen {uint32_t order{},count{};uintptr_t model{};bool set{};};
    static uint32_t ibl_next_order{};static uint32_t ibl_rows_left=ModuleFlag(L"light-observe.flag")?384u:0u;static std::unordered_map<uintptr_t,IblSeen> ibl_seen;   // the final review's L3: a witness's flag
    {const auto unlisted=[&](uintptr_t holder){return std::none_of(views.begin(),views.end(),[&](const View& listed){return listed.holder==holder;});};
        std::erase_if(waits,[&](const auto& w){return unlisted(w.first);});std::erase_if(drawn,unlisted);
        std::erase_if(motion_seen,[&](const auto& m){return unlisted(m.first);});std::erase_if(listed_kernels,[&](const auto& k){return unlisted(k.first);});
        {const std::lock_guard hold(native_offscreen_lock);std::erase_if(native_offscreen_queued,[&](const auto& q){return unlisted(q.first);});}
        if(!ibl_rows_left)ibl_seen.clear();else std::erase_if(ibl_seen,[&](const auto& s){return unlisted(s.first);});}
    for(const auto& v:views){
        // I-0140 tranche 41, its second step review's M1: the holder's last published collect taken (and cleared) before any exit, so a
        // collect that stops here (Busy, the camera) leaves 0 and the next one resets unjittered.
        uint64_t published{};if(const auto m=motion_seen.find(v.holder);m!=motion_seen.end()){published=m->second.published;m->second.published=0;}
        const Busy busy(v.holder,v.model);if(!busy.owned)continue;
        uintptr_t camera_view{},manager{};std::array<uint8_t,0x24> lens{};
        SceneViewInput camera{};camera.serial=selection_serial;
        if(!Read(v.holder+0x28,&camera_view,8) || !camera_view || !Read(camera_view,camera.camera_pose.data(),64) ||
           !Read(camera_view+0x50,lens.data(),lens.size()) || !Read(image+0x4796298,&manager,8) || !manager){skip("camera",v);continue;}
        std::memcpy(&camera.vertical_fov,lens.data(),4);std::memcpy(&camera.near_plane,lens.data()+4,4);std::memcpy(&camera.far_plane,lens.data()+8,4);
        // The menu view is a plain perspective (MENUS-MAP 21: fov, aspect w/h, near/far 0.08/10000); its lens words +68/+70
        // are not the main camera output's projection flags / tile count (Native1169: +68 = 584.0f): explicit zero.
        std::memcpy(&camera.explicit_aspect,lens.data()+20,4);camera.projection_flags=0;camera.tile_count=0;
        camera.width=v.width;camera.height=v.height;
        // I-0140 tranche 40 (D-0305): with the menu views' TAA the view's camera jitters in its own pixels (Halton (2, 3) over 8 phases,
        // the main view's TAA sequence, at the view's extent); the projection words carry it (ProduceCameraConstants).
        // Its camera motion from its own previous camera (a cut by the main view's rule, YB-4b.5); a camera that cannot be projected
        // keeps no jitter and has no TAA this frame. Without the menu TAA the history is dropped (the view's next TAA frame resets).
        // Only a tabled view (its tone read here, before the camera: the step review's M1): an untabled one composes through the
        // depth's mask, which the jitter would move every frame over a background the TAA never saw.
        // Key {D27770(1), id}: D65DD0/D64C30 pass the area index 1 through D27770 (table 14294EFB0), not the literal 1.
        const std::array<uint32_t,2> gparam_key{reinterpret_cast<uint32_t(__fastcall*)(uint32_t)>(image+0xD27770)(1),v.gparam_id};
        std::string tone_state,tone_source;const auto tone=NativeOffscreenToneOf(gparam_key,tone_state,tone_source);   // I-0113 tranche 27
        std::optional<NativeSelectedScene::MenuMotion> menu_motion;
        // I-0140 tranche 41 (D-0306): a frame that resets the view's history is drawn unjittered, and the renderer's TAA takes its own
        // samples there (TaaParams::own_reset): the frame the history restarts from is the scene as drawn, not a reconstruction of a
        // shifted frame (the face's sharpness 12 s after an opening as tranche 40's without it, across runs: MEASURED; why, not read);
        // DLAA receives it unjittered too. A reset: the view's first collect, a cut, or a collect after one not queued for
        // drawing (« unplaced » below, a portrait's first collects, a target not resident, a frame unprepared, a collect replaced before
        // its presentation: native_offscreen_queued; the renderer did not draw it, its history restarts; the motion then reset too, as
        // on a cut). Its kernel (NativeMenuTaaKernel) chosen once while the holder stays listed (an opening: listed_kernels).
        // The renderer's own resets (DLAA refused, the switches from and to DLAA, a new history target, a gap the collect could not see)
        // restart the history from the collect's jittered frame, a reconstruction that fades at the feedback's rate (its second step
        // review's L1, accepted).
        if(NativeMenuTaaActive() && tone.tabled){
            auto& seen=motion_seen[v.holder];if(seen.model!=v.model){seen={};seen.model=v.model;published=0;}
            bool queued=false;if(published){const std::lock_guard hold(native_offscreen_lock);const auto q=native_offscreen_queued.find(v.holder);queued=q!=native_offscreen_queued.end() && q->second==published;}
            const bool cut=seen.history.valid && (!queued || ds3native::yebis::CameraCut(seen.pose.data(),camera.camera_pose.data()));
            auto kernel=listed_kernels.find(v.holder);if(kernel==listed_kernels.end())kernel=listed_kernels.emplace(v.holder,NativeMenuTaaKernel(kernel_listings[uint64_t(v.width)<<32|v.height]++)).first;
            if(cut || !seen.history.valid)camera.jitter_x=camera.jitter_y=0;
            else{const auto jitter=ds3native::upscaler::Jitter(selection_serial,8);camera.jitter_x=jitter[0];camera.jitter_y=jitter[1];}
            NativeSelectedScene::MenuMotion motion;motion.dlss=NativeDlssActive();motion.kernel=kernel->second;   // DLAA on the view while the option runs DLSS (D-0305)
            if(upscaler::MenuViewMotion(camera,cut,seen.history,motion.current_to_previous,motion.reset,&motion.clip_to_previous_clip)){seen.pose=camera.camera_pose;
                auto still=seen.history;bool still_reset{};(void)upscaler::MenuViewMotion(camera,true,still,motion.still_to_previous,still_reset);menu_motion=motion;}
            else camera.jitter_x=camera.jitter_y=0;}
        else motion_seen.erase(v.holder);
        ingress::SceneCameraConstants constants;
        if(!ingress::ProduceCameraConstants(camera,constants)){skip("camera_constants",v);
            static LONG64 lens_reported{};if(InterlockedIncrement64(&lens_reported)<=3){const auto& m=camera.camera_pose;char row[512]{};
                sprintf_s(row,"{\"event\":\"native_offscreen_view_lens\",\"fov\":%g,\"near\":%g,\"far\":%g,\"aspect\":%g,\"flags\":%u,\"tiles\":%u,\"pose_w\":[%g,%g,%g,%g],\"eye\":[%g,%g,%g]}\n",
                    camera.vertical_fov,camera.near_plane,camera.far_plane,camera.explicit_aspect,camera.projection_flags,camera.tile_count,m[3],m[7],m[11],m[15],m[12],m[13],m[14]);Log(row);}
            continue;}
        // A menu model is listed by Init_Play as soon as its parts are loaded, possibly before its first pose (Native1169: nonfinite
        // item placement). I-0113 gap 20 (the System menu's 512x256 portrait black in play): one item whose placement stayed non-finite
        // kept the whole view « unplaced » for the session (Thomas's journals of 2026-10-31 08:24 and 08:59); the original draws the
        // view and the GPU drops only that item's triangles (as I-0101's pose rows), and the admission already skips a model whose
        // placement is not finite (nonfinite_placement, the result's `nonfinite`): after a bounded wait (below) the view is drawn without them.
        std::vector<uintptr_t> drawables;
        for(uint32_t i=0;i<24;++i){uintptr_t part{},drawable{},table{};
            if(!Read(v.model+0x28+8ull*i,&part,8) || !part || !Read(part+0x18,&drawable,8) || !drawable || !Read(drawable,&table,8) ||
               table!=image+0x29489A0)continue;
            drawables.push_back(drawable);}
        auto result=std::make_shared<NativeSelectedScene>();result->serial=selection_serial;
        result->common_images=AcquireNativeCommonImages();
        {std::lock_guard<std::mutex> hold(simulation_time_lock);result->simulation_time=native_simulation_time;}
        result->options=AcquireNativeOptions();if(result->options)result->sampler_bank=result->options->sampler_bank;   // O2
        result->wet_shader_catalog=NativeWetShaderCatalog();
        {std::array<uint8_t,16> row{};if(Read(image+0x4929B20,row.data(),row.size()))result->default_param_row=row;}   // cP6
        BYTE variant_enabled{},category_override{};bool split_engine5{};float category_scalar{};
        if(Read(image+0x473D39E,&variant_enabled,1))result->global_variant_enabled=variant_enabled!=0;
        if(Read(image+0x4962314,&category_override,1))result->category_override=category_override!=0;
        if(ingress::ReadModelSplitPassSetting(image,split_engine5))result->split_engine5=split_engine5;
        if(Read(image+0x3D7EE10,&category_scalar,4))result->category_scalar=category_scalar;
        result->pixel_policy=ds3content::NativeModelPixelPolicy::Default;
        result->camera=camera;result->camera_constants=constants;
        if(!ingress::ProduceCameraViewConstants(camera,constants,result->view_constants)){skip("view_constants",v);continue;}
        if(menu_motion){result->menu_motion=menu_motion;result->taa=true;}   // I-0140 tranche 40
        NativeSelectionVisitor visitor;visitor.result=result.get();
        visitor.world_mask=reinterpret_cast<uint32_t(__fastcall*)(uintptr_t)>(image+0xDFD240)(manager);
        const int32_t inside=1;std::memcpy(visitor.base.data()+0x24,&inside,4);   // the whole view: no frustum classification
        native_menu_ibl_tally={};
        // I-0140 tranche 31 (D-0303): a Player view's candidates also gathered for its traced self-shadow (with the rays on and the
        // renderer's menu shadow not failed; no-menu-shadow.flag, a test tool, leaves every view unshadowed as before).
        const bool traced=v.shadowed && !no_menu_shadow && native_rt_scene.load(std::memory_order_relaxed) && !renderer.RtSceneFailed() && !renderer.MenuShadowFailed() && renderer.MenuShadowReady();
        std::vector<NativeDeferredCandidate> rays;if(traced)visitor.rays_also=&rays;
        native_offscreen_preparing=true;for(const auto drawable:drawables)AdmitNativeSelectedObject(&visitor,drawable);native_offscreen_preparing=false;
        visitor.rays_also=nullptr;
        // I-0140 tranche 30: the view's admissions by region-IBL outcome, its first 3 admitted collects (a collect skipped above, Busy
        // or a camera refused, is not counted), with its order of first admission and the collect's non-finite items (a collect with
        // some may be « unplaced » below and not drawn); at most 384 rows. The five counts are the admissions that reached the IBL choice, some refused after it: not a partition of
        // `models` (the admitted models). Its step review's L1 to L3: a holder no longer listed is forgotten before the views (a reused
        // address starts afresh, with a new order). With light-observe.flag only, at most 384 rows; the tally's reset and counts
        // (09_model_admission's NativeMenuRegionIbl) go on without it, a few increments a model.
        if(ibl_rows_left){
            auto& seen=ibl_seen[v.holder];if(!seen.set || seen.model!=v.model)seen={ibl_next_order++,0u,v.model,true};
            if(seen.count<3){--ibl_rows_left;++seen.count;const auto& t=native_menu_ibl_tally;char row[340]{};
                sprintf_s(row,"{\"event\":\"native_offscreen_view_ibl\",\"width\":%u,\"gparam\":%u,\"holder_order\":%u,\"collect\":%u,\"tick\":%llu,\"models\":%zu,\"nonfinite\":%u,\"category\":%u,\"memo\":%u,\"term\":%u,\"no_cube\":%u,\"bound\":%u}\n",
                    v.width,v.gparam_id,seen.order,seen.count,static_cast<unsigned long long>(GetTickCount64()),result->models.size(),unsigned(result->nonfinite.load()),t[0],t[1],t[2],t[3],t[4]);Log(row);}}
        // Tranche 11's final review M1: a view whose items are non-finite waits (keeps its previous picture, « unplaced ») for up to 2 s
        // in a row (since tranche 19 only as the paragraph below says: a menu model before its first pose; the Load Game portraits are drawn once and unlisted); past that it is
        // drawn without them, once noted per holder (native_offscreen_view_partial). Its step review: by time, not collects (the rate is
        // unlocked, I-0114).
        // I-0113 tranche 19 (gap 20 again: Thomas, 2026-11-06 18:29, the System menu's portrait absent; R's night endurance
        // full-1106-night: 2 of 14 openings of the 512x256 view « unplaced » to their close, none past the wait): a holder never selected
        // complete has no previous picture to keep: once some of its models are admitted it is drawn at once (every frame) without the
        // non-finite items; the wait stays for a holder selected complete before and for one with no model admitted yet (INFERRED: a menu
        // model before its first pose admits none, the « unplaced » fields its witness; a Load Game portrait drawn once and unlisted).
        // Its row « unplaced » names, in place of the prepared meshes and draws, the models admitted and the items skipped as non-finite.
        {if(const auto skipped=result->nonfinite.load()){
                const uint64_t now=GetTickCount64();const uint64_t since=waits.try_emplace(v.holder,now).first->second;
                if((drawn.contains(v.holder) || result->models.empty()) && now-since<2000){NoteNativeOffscreenViewState(v.holder,v.width,"unplaced",result->models.size(),skipped);continue;}
                static std::unordered_set<uintptr_t> told;if(told.size()<64 && told.insert(v.holder).second){char row[200]{};
                    sprintf_s(row,"{\"event\":\"native_offscreen_view_partial\",\"width\":%u,\"height\":%u,\"nonfinite\":%u,\"waited_ms\":%llu}\n",v.width,v.height,unsigned(skipped),static_cast<unsigned long long>(now-since));Log(row);}}
            else{waits.erase(v.holder);if(!result->models.empty())drawn.insert(v.holder);}}   // tranche 19's final review M1 (and its fix review's L-a):
                // only a complete selection with a model is a picture to keep
        PrepareNativeSelectedPasses(*result);CollectNativeLightBlocks(*result,gparam_key);
        if(traced)CollectNativeMenuSelfShadow(*result,gparam_key,rays,v.width,v.holder,menu_walk);   // I-0140 t31
        CollectNativeForwardLights(*result,false);ProduceNativeSceneLighting(*result,true);   // cP7
        // 1176 witness: preparation gates of a menu view, overall and per material family (Load Game portraits lose hair).
        {static LONG64 gate_rows{};if(InterlockedIncrement64(&gate_rows)<=12){std::string gates,families;
            for(size_t i=0;i<result->refusals.size();++i)if(result->refusals[i]){char item[32]{};sprintf_s(item,"%s\"%zu\":%u",gates.empty()?"":",",i,result->refusals[i]);gates+=item;}
            for(const auto& [family,codes]:result->asm_family_gates){std::string inner;
                for(const auto& [code,count]:codes){char item[32]{};sprintf_s(item,"%s\"%d\":%u",inner.empty()?"":",",code,count);inner+=item;}
                families+=(families.empty()?"":",")+std::string("\"")+family.substr(0,80)+"\":{"+inner+"}";}
            const std::string row="{\"event\":\"native_offscreen_view_gates\",\"width\":"+std::to_string(v.width)+",\"eligible\":"+std::to_string(result->eligible_meshes)+
                ",\"prepared\":"+std::to_string(result->prepared_meshes.size())+",\"gates\":{"+gates+"},\"families\":{"+families+"}}\n";Log(row.c_str());
            // and each prepared mesh: engine pass, model/mesh ordinal, material family, blend mode and alpha reference.
            std::string meshes;
            for(const auto& p:result->prepared_meshes){const auto& m=result->models[p.model].meshes[p.mesh];if(!m.programs)continue;std::string family;
                for(const auto ch:m.programs->program_path){if(family.size()>40)break;family.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');}
                char cell[160]{};sprintf_s(cell,"%s[%u,%zu,%u,\"%s\",%u,%u]",meshes.empty()?"":",",p.engine_pass,p.model,m.current.ordinal,family.c_str(),
                    m.programs->policy?static_cast<uint32_t>(m.programs->policy->blend_mode):99u,m.programs->policy?static_cast<uint32_t>(m.programs->policy->alpha_ref):999u);meshes+=cell;}
            const std::string list="{\"event\":\"native_offscreen_view_meshes\",\"width\":"+std::to_string(v.width)+",\"meshes\":["+meshes+"]}\n";Log(list.c_str());}}
        static LONG64 reported{};const auto n=InterlockedIncrement64(&reported);
        if(n<=6 || n%300==0){const auto& m=camera.camera_pose;char row[640]{};sprintf_s(row,"{\"event\":\"native_offscreen_view_selected\",\"width\":%u,\"height\":%u,\"objects\":%u,\"models\":%zu,\"prepared_meshes\":%zu,\"fov\":%g,\"aspect\":%g,\"pose\":[%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g]}\n",
            v.width,v.height,result->objects.load(),result->models.size(),result->prepared_meshes.size(),camera.vertical_fov,camera.explicit_aspect,
            m[0],m[1],m[2],m[3],m[4],m[5],m[6],m[7],m[8],m[9],m[10],m[11],m[12],m[13],m[14],m[15]);Log(row);}
        int bank_state=0;const auto background=NativeOffscreenBackground(gparam_key,&bank_state);
        {static LONG64 background_reported{};if(InterlockedIncrement64(&background_reported)<=4){char row[256]{};
            sprintf_s(row,"{\"event\":\"native_offscreen_view_background\",\"key\":[%u,%u],\"bank_state\":%d,\"colour\":[%g,%g,%g,%g]}\n",gparam_key[0],gparam_key[1],bank_state,background[0],background[1],background[2],background[3]);Log(row);}}
        // The step review's L2 and the final review's L3: a row when a key's state changes (up to 16 keys), so a first collect
        // before the banks are published does not hide the later state.
        {static std::mutex tone_lock;static std::map<std::pair<uint32_t,uint32_t>,std::string> tone_keys;bool fresh{};
            {const std::lock_guard hold(tone_lock);const std::pair key_pair{gparam_key[0],gparam_key[1]};const auto it=tone_keys.find(key_pair);
             if(it==tone_keys.end()){fresh=tone_keys.size()<16;if(fresh)tone_keys.emplace(key_pair,tone_state);}
             else if(it->second!=tone_state){fresh=true;it->second=tone_state;}}
            if(fresh){char row[512]{};for(auto& ch:tone_source)if(ch=='\\' || ch=='"' || ch<32)ch='/';
            sprintf_s(row,"{\"event\":\"native_offscreen_view_tone\",\"key\":[%u,%u],\"width\":%u,\"state\":\"%.60s\",\"source\":\"%.120s\",\"mode\":%u,\"scale\":%g,\"contrast\":%g,\"gamma\":%g,\"scaler\":[%g,%g,%g]}\n",
                gparam_key[0],gparam_key[1],v.width,tone_state.c_str(),tone_source.c_str(),tone.mode,tone.scale,tone.contrast,tone.gamma,tone.scaler[0],tone.scaler[1],tone.scaler[2]);Log(row);}}
        if(menu_motion)motion_seen[v.holder].published=result->uid;   // I-0140 tranche 41 (the scene's uid, its final review's L1)
        scenes.push_back({v.holder,v.proxy,std::move(result),background,tone});
    }
    // I-0140 t31: what this walk and the one before did not name; the step review's M2: 2.5 s after the last collect that traced a
    // view (the menus closed or no Player view left), the menu's caches emptied. By time, not collects (the rate is unlocked,
    // I-0114; the last step review's L1), and longer than the 2 s a view held « unplaced » skips its collect (above).
    static bool menu_traced{};static uint64_t traced_tick{};
    if(menu_walk){EvictNativeMenuRtCaches(menu_walk);menu_traced=true;traced_tick=GetTickCount64();}
    else if(menu_traced && GetTickCount64()-traced_tick>=2500){ClearNativeMenuRtCaches();menu_traced=false;}
    // The step review's L2: the renderer's side, native_menu_self_shadow_renderer: its failure once, and its counts on the first 4
    // traced walks and every 600th: traced, the masks traced; past_slots, the views past a frame's kMenuRtSlots pairs (drawn
    // unshadowed); trace_refused, the traced views whose trace drew nothing, their mask cleared white (the other white clears, of
    // held frames and of refused rewrites, not counted; the last step review's L4); held, the recordings of a view's context that kept
    // its mask untraced and still named it; neutral, the successful renames of a view's mask views to the neutral (I-0140 tranche 34:
    // tranche 31's L7 could not see either). Both count events, not frames or views: a context recorded several times a frame counts
    // each time, a view renamed at each reopening each time (its step review's L2). As they also move once tracing has stopped (no traced
    // walk, so no periodic row), the row is then also written when either changed since the last row, at most once a second (its M1;
    // `walk` 0); while views are traced the periodic rows alone (held moves then too: an untraced recording of a traced view's size, the
    // review of 49aa2aa00, L1). And one row on the first collect with no traced walk after one that had one (the final review's L5: the
    // counts after the last 600th walk; the renderer's last recording may still be a frame behind then).
    {static bool failure_reported{};static uint64_t walks{};static uint64_t printed_held{};static uint64_t printed_neutral{};static uint64_t printed_tick{};   // one a declaration: the thread-affinity checker reads only a declaration's first name (the review of f1d53c7a1)
     static bool walked{};
     const bool failed=renderer.MenuShadowFailed() && !failure_reported;if(failed)failure_reported=true;
     const bool periodic=menu_walk && (++walks<=4 || walks%600==0),ended=!menu_walk && walked;walked=menu_walk!=0;
     const auto n=renderer.MenuShadowCounts();const uint64_t tick=GetTickCount64();
     const bool moved=!menu_walk && (n[3]!=printed_held || n[4]!=printed_neutral) && tick-printed_tick>=1000;
     if(failed || periodic || ended || moved){printed_held=n[3];printed_neutral=n[4];printed_tick=tick;std::string why=renderer.MenuShadowFailure();for(auto& ch:why)if(ch=='\\' || ch=='"' || ch<32)ch='/';
        char row[448]{};
        sprintf_s(row,"{\"event\":\"native_menu_self_shadow_renderer\",\"walk\":%llu,\"failed\":\"%.160s\",\"traced\":%llu,\"past_slots\":%llu,\"trace_refused\":%llu,\"held\":%llu,\"neutral\":%llu,\"blurred\":%llu}\n",
            static_cast<unsigned long long>(menu_walk),why.c_str(),static_cast<unsigned long long>(n[0]),static_cast<unsigned long long>(n[1]),static_cast<unsigned long long>(n[2]),
            static_cast<unsigned long long>(n[3]),static_cast<unsigned long long>(n[4]),static_cast<unsigned long long>(n[5]));Log(row);}}   // I-0140 t35: blurred, the traced recordings whose blur passes all ran
    const std::lock_guard hold(native_offscreen_lock);
    native_offscreen_selected_since_present=true;
    // A view retired while it was being selected is not published.
    std::erase_if(scenes,[](const NativeOffscreenScene& s){return !native_offscreen_armed.contains(s.holder);});
    native_offscreen_scenes=std::move(scenes);
}
// 1169: at presentation (before the UI samples the images) each selected menu view is prepared and drawn on its
// own target set into the view's native target. R13 cR2: prepared here on the engine thread and collected into the frame
// job in order (the target record held by the job); the frame job draws them before the frame.
void CollectNativeOffscreenViews(NativeFrameJob& job) noexcept {
    const auto frame_serial=job.serial;
    // 1170: without a native scene CPU update (title, Load Game) the views are selected here, only on the menu thread
    // itself (presentation and menu steps then run on one thread, so no menu teardown can interleave).
    if(!native_offscreen_selected_since_present.exchange(false) && native_menu_thread && GetCurrentThreadId()==native_menu_thread){
        try{CollectNativeOffscreenScenes(frame_serial);native_offscreen_selected_since_present=false;}
        catch(const std::exception& e){char row[400]{};sprintf_s(row,"{\"event\":\"native_offscreen_view_failure\",\"message\":\"%.300s\"}\n",e.what());Log(row);Refuse("native_offscreen_view_failed");}}
    if(native_menu_thread){static LONG64 threads{};if(InterlockedIncrement64(&threads)==1){char row[160]{};
        sprintf_s(row,"{\"event\":\"native_offscreen_threads\",\"menu\":%lu,\"presentation\":%lu}\n",static_cast<unsigned long>(native_menu_thread.load()),GetCurrentThreadId());Log(row);}}
    std::vector<NativeOffscreenScene> scenes;
    {const std::lock_guard hold(native_offscreen_lock);scenes=native_offscreen_scenes;}
    for(const auto& s:scenes){
        try {
            const auto target=textures.Acquire(s.proxy);
            if(!target || !target->resident || !target->resource){NoteNativeOffscreenViewState(s.holder,s.scene->camera.width,"target_absent",s.scene->prepared_meshes.size(),0);static LONG64 absent{};if(InterlockedIncrement64(&absent)<=6){char row[200]{};
                sprintf_s(row,"{\"event\":\"native_offscreen_view_target_absent\",\"width\":%u,\"height\":%u,\"record\":%s}\n",s.scene->camera.width,s.scene->camera.height,target?"true":"false");Log(row);}
                continue;}
            const auto frame=PrepareNativeModelFrame(s.scene,frame_serial,true);   // cF.1: prepare takes the view kind as its parameter
            static LONG64 reported{};const auto n=InterlockedIncrement64(&reported);
            if(n<=6 || n%300==0){char row[256]{};sprintf_s(row,"{\"event\":\"native_offscreen_view\",\"width\":%u,\"height\":%u,\"prepared_meshes\":%zu,\"draws\":%zu,\"forward_draws\":%zu,\"lit\":%s}\n",
                s.scene->camera.width,s.scene->camera.height,s.scene->prepared_meshes.size(),frame?frame->draws.size():0,frame?frame->forward_draws.size():0,frame && frame->lighting?"true":"false");Log(row);}
            NoteNativeOffscreenViewState(s.holder,s.scene->camera.width,!frame || !frame->lighting?"unprepared":"queued",s.scene->prepared_meshes.size(),frame?frame->draws.size()+frame->forward_draws.size():0);
            if(!frame || !frame->lighting){static LONG64 unlit{};if(InterlockedIncrement64(&unlit)<=6){char row[200]{};
                sprintf_s(row,"{\"event\":\"native_offscreen_view_unprepared\",\"width\":%u,\"height\":%u,\"frame\":%s}\n",s.scene->camera.width,s.scene->camera.height,frame?"true":"false");Log(row);}
                continue;}
            // Witness: the composed image of each view on its 20th and 200th draw, next to the trace (raw RGBA rows).
            static std::unordered_map<uintptr_t,uint32_t> drawn;std::wstring pixel_file;
            if(const auto count=++drawn[s.holder];WitnessPictures() && (count==20 || count==200))   // no readback and file in a play run
                pixel_file=JournalSibling(L"offscreen-"+std::to_wstring(s.scene->camera.width)+L"x"+std::to_wstring(s.scene->camera.height)+L"-"+std::to_wstring(count)+L".rgba");
            job.views.push_back({frame,target->resource,target,s.background,pixel_file,s.tone});
            {const std::lock_guard hold(native_offscreen_lock);native_offscreen_queued[s.holder]=s.scene->uid;}   // I-0140 tranche 41's M1 (the uid: its final review's L1)
            if(!pixel_file.empty()){char row[160]{};sprintf_s(row,"{\"event\":\"native_offscreen_pixels\",\"width\":%u,\"height\":%u,\"draw\":%u}\n",
                s.scene->camera.width,s.scene->camera.height,drawn[s.holder]);Log(row);}
        }catch(const std::exception& e){char row[1100]{};sprintf_s(row,"{\"event\":\"native_offscreen_view_failure\",\"message\":\"%.1000s\"}\n",e.what());Log(row);Refuse("native_offscreen_view_failed");}
        catch(...){Refuse("native_offscreen_view_failed");}
    }
    // I-0125 R6c: the water's t54 marked written once the traced pass recorded into it (the main frame's, before the forward rows
    // sample it), between collects (R6f, D-0251: no mirror view drawn here any more).
    if(NativeReflectionEnabled()){const std::lock_guard hold(native_reflection.lock);
        if(const auto drawn=renderer.RtWaterDrawn();drawn && drawn==native_reflection.target.Get())native_reflection.written=true;}
}
using OffscreenModelBind=void(__fastcall*)(uintptr_t,uintptr_t);
OffscreenModelBind offscreen_model_bind_original{};
void __fastcall NativeOffscreenModelBind(uintptr_t model,uintptr_t holder) noexcept {
    offscreen_model_bind_original(model,holder);
    if(control->stop || !model)return;
    {const std::lock_guard hold(native_offscreen_lock);if(holder)native_offscreen_models.insert_or_assign(holder,model);}
    uintptr_t model_table{},entity{},entity_table{},scene{};uint32_t parts{};float camera[4]{};
    Read(model,&model_table,8);Read(model+8,&entity,8);if(entity){Read(entity,&entity_table,8);Read(entity+0xAF8,&parts,4);}
    if(holder && Read(holder+0x30,&scene,8) && scene)Read(scene+0x50,camera,16);
    static LONG64 reported{};if(InterlockedIncrement64(&reported)<=16){char row[360]{};
        sprintf_s(row,"{\"event\":\"native_offscreen_model\",\"holder\":%llu,\"model_table_rva\":%lld,\"entity_table_rva\":%lld,\"asm_entity\":%s,\"parts\":%u,\"camera\":[%g,%g,%g,%g]}\n",
            static_cast<unsigned long long>(holder),model_table?static_cast<long long>(model_table-image):0ll,entity_table?static_cast<long long>(entity_table-image):0ll,
            entity_table==image+0x2947DB8?"true":"false",parts,camera[0],camera[1],camera[2],camera[3]);Log(row);}
}
void __fastcall NativeOffscreenSceneGraph(uintptr_t texture,uint32_t width,uint32_t height,uintptr_t,uintptr_t descriptor,uint32_t gparam_id) noexcept {
    try {
        uintptr_t heap{};if(!Read(image+0x479C270,&heap,8) || !heap)Refuse("native_offscreen_scene_heap");
        const auto allocate=[&](size_t bytes,size_t alignment){
            const auto storage=reinterpret_cast<uintptr_t(__fastcall*)(size_t,size_t,uintptr_t)>(image+0x1777A00)(bytes,alignment,heap);
            if(!storage)throw std::bad_alloc();return storage;};
        const auto view=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0x4AFF0);
        const auto views=allocate(0x120,0x10);view(views);view(views+0x90);
        const auto single=allocate(0x90,0x10);view(single);
        const auto scene=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0xCED370)(allocate(0x60,8));
        *reinterpret_cast<uintptr_t*>(texture+0x20)=views;*reinterpret_cast<uintptr_t*>(texture+0x28)=single;*reinterpret_cast<uintptr_t*>(texture+0x30)=scene;
        reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t)>(image+0xCED790)(scene,single);
        reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t)>(image+0xC1B20)(views,single);
        uintptr_t light_heap{};if(!Read(image+0x479C288,&light_heap,8) || !light_heap)Refuse("native_offscreen_light_heap");
        const auto light_storage=reinterpret_cast<uintptr_t(__fastcall*)(size_t,size_t,uintptr_t)>(image+0x1777A00)(0x40,8,light_heap);
        if(!light_storage)throw std::bad_alloc();
        *reinterpret_cast<uintptr_t*>(texture+0x50)=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0xD039E0)(light_storage);
        // 1167: the scene +38 (4FDB0, the same CPU world class as the native main view, table 3D20398, with its
        // spatial root 4FEC0) and the flat layer +40 (C2400, table 3D4A0A8) are CPU objects: build them genuinely
        // (refcounted setters D649C0/D648A0, C1E30 attaches the layer) and make the light the layer's registration
        // context (D03AC0), so the menu model's parts register there (C3AD90 -> C4C3C0) like world characters.
        {   const auto gx_scene=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0x4FDB0)(allocate(0x38,8));
            reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t)>(image+0xD649C0)(texture+0x38,gx_scene);
            uint32_t tree_mode=1;reinterpret_cast<BYTE(__fastcall*)(uintptr_t,uint32_t*)>(image+0x4FEC0)(gx_scene,&tree_mode);
            const auto layer=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0xC2400)(allocate(0x38,8));
            reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t)>(image+0xD648A0)(texture+0x40,layer);
            reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uintptr_t)>(image+0xC1E30)(gx_scene,layer);
            reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uintptr_t)>(image+0xD03AC0)(*reinterpret_cast<uintptr_t*>(texture+0x50),layer);}
        *reinterpret_cast<float*>(texture+0x68)=static_cast<float>(width);*reinterpret_cast<float*>(texture+0x6C)=static_cast<float>(height);
        // 1165: the named proxy (holder +8, CSOffscreenGxTexture) gets its native w x h target on first UI import.
        uintptr_t proxy{};
        if(Read(texture+8,&proxy,8) && proxy){textures.Retire(proxy);
            uint64_t flags{};const bool shadowed=descriptor && Read(descriptor+8,&flags,8) && (flags&1);   // I-0140 t31: the Player family
            const std::lock_guard hold(native_offscreen_lock);native_offscreen_proxies.insert_or_assign(proxy,NativeOffscreenProxy{texture,width,height,gparam_id,shadowed});}
        static LONG64 reported{};if(InterlockedIncrement64(&reported)<=12){char row[240]{};uint64_t flags{};if(descriptor)Read(descriptor+8,&flags,8);
            sprintf_s(row,"{\"event\":\"native_offscreen_scene_cpu\",\"width\":%u,\"height\":%u,\"gparam_id\":%u,\"flags\":\"%llX\",\"cpu_scene_and_layer\":true,\"composite_scene\":false}\n",
                width,height,gparam_id,static_cast<unsigned long long>(flags));Log(row);}
    }catch(...){Refuse("native_offscreen_scene_failed");}
}
// I-0140 tranche 30: D65410(holder, id) re-keys a menu view's banks to {D27770(1), id} (88AF0 on the object [[holder+48]]+0x18
// returns, when +48 and it are not nil; 88AF0 re-keys ShadowParam too, MENUS-MAP §22: a later shadow of the Player views takes it from
// the proxy's id); D68260 calls it from D6ABD0, the Player view's preset switch, with the new MenuOffscrRendParam row's id
// (character creation's face close-up: row 1, id 10, where row 0 has 11; READ). Natively +48 is nil (NativeOffscreenSceneGraph),
// so the original wrote nothing and the view kept its creation id: the close-up was lit with the body's light set, fog, IBL,
// background and tone. Here the id goes to the view's proxy, which the collect reads each frame (CollectNativeOffscreenScenes'
// key {D27770(1), id}). The target's optimized clear value is the id the UI's first import read (20_startup_phases.inc); a view
// that is not tabled and whose new background differs is cleared with another colour (a debug-layer clear-value notice, no wrong pixel).
void __fastcall NativeOffscreenRekey(uintptr_t holder,uint32_t gparam_id) noexcept {
    if(!holder)return;
    uint32_t width{},from{};bool found=false;
    {const std::lock_guard hold(native_offscreen_lock);
        for(auto& [proxy,owned]:native_offscreen_proxies)if(owned.holder==holder){from=owned.gparam_id;owned.gparam_id=gparam_id;width=owned.width;found=true;}}
    static LONG64 reported{};if(InterlockedIncrement64(&reported)<=24){char row[200]{};
        sprintf_s(row,"{\"event\":\"native_offscreen_view_rekey\",\"width\":%u,\"from\":%u,\"to\":%u,\"found\":%s}\n",width,from,gparam_id,found?"true":"false");Log(row);}
}
// 1250 (I-0068 part J BINK-NATIVE-READY, I-0069 BINK-MOVIES-NATIVE): the native Bink movie player. 1171 omitted it
// (the factory 141ADE670 returned no player) because the movie step EAF7E0 hands the player the old device
// [*144961AB0], nil natively (access violation at EAF8F6). Now Bink's CPU decode, its four decode threads, its pacing
// (BinkWait, BinkShouldSkip) and its own audio (XAudio2 or DirectSound) run as in the original. Transcribed without
// the old device: the two steps EAF7E0/EAFAC0, the player's open/close/render 1ADF230/1ADE850/1ADEAF0 (no GX output,
// no GX state save) and the publish EADE50. Native: the graphics object's init/planes/draw 1AE0830/1AE0BA0/1ADFF10.
// The draw copies the ready plane set on the game thread (the decode threads reuse the buffers) with the game's own
// 9 x float4 block; at presentation the game's conversion programmes draw it into a native R8G8B8A8 target admitted
// for the movie texture proxy (CSMovieGxTexture, vtable 1429A4968) at open, which the menu page samples.
namespace native_movie {
uintptr_t P(uintptr_t address) noexcept {uintptr_t value{};Read(address,&value,8);return value;}
uint32_t U32(uintptr_t address) noexcept {uint32_t value{};Read(address,&value,4);return value;}
float F32(uintptr_t address) noexcept {float value{};Read(address,&value,4);return value;}
uint8_t U8(uintptr_t address) noexcept {uint8_t value{};Read(address,&value,1);return value;}
template<class R,class... A> R VCall(uintptr_t object,size_t slot,A... args) noexcept {
    return reinterpret_cast<R(__fastcall*)(uintptr_t,A...)>(P(P(object)+slot))(object,args...);}
// Bink through the game's own import cells; each cell is checked once to resolve into binkw64.dll.
template<uintptr_t cell> uintptr_t Entry() noexcept {
    static const uintptr_t function=[]{uintptr_t entry{};HMODULE module{};
        if(!Read(image+cell,&entry,8) || !entry || !GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(entry),&module) || module!=GetModuleHandleW(L"binkw64.dll"))Refuse("native_movie_bink_import");
        return entry;}();
    return function;
}
void BinkGetFrameBuffersInfo(uintptr_t bink,uintptr_t buffers) noexcept {reinterpret_cast<void(__stdcall*)(uintptr_t,uintptr_t)>(Entry<0x4DBC074>())(bink,buffers);}
int32_t BinkAllocateFrameBuffers(uintptr_t bink,uintptr_t buffers,uint32_t alignment) noexcept {
    return reinterpret_cast<int32_t(__stdcall*)(uintptr_t,uintptr_t,uint32_t)>(Entry<0x4DBC124>())(bink,buffers,alignment);}
void BinkRegisterFrameBuffers(uintptr_t bink,uintptr_t buffers) noexcept {reinterpret_cast<void(__stdcall*)(uintptr_t,uintptr_t)>(Entry<0x4DBC0C4>())(bink,buffers);}
void BinkClose(uintptr_t bink) noexcept {reinterpret_cast<void(__stdcall*)(uintptr_t)>(Entry<0x4DBC104>())(bink);}
uintptr_t BinkOpen(const char* file,uint32_t flags) noexcept {return reinterpret_cast<uintptr_t(__stdcall*)(const char*,uint32_t)>(Entry<0x4DBC11C>())(file,flags);}
void BinkSetSoundTrack(uint32_t count,uintptr_t tracks) noexcept {reinterpret_cast<void(__stdcall*)(uint32_t,uintptr_t)>(Entry<0x4DBC0A4>())(count,tracks);}
void BinkSetIOSize(uint32_t size) noexcept {reinterpret_cast<void(__stdcall*)(uint32_t)>(Entry<0x4DBC09C>())(size);}
void BinkSetSpeakerVolumes(uintptr_t bink,uint32_t track,const int32_t* volumes) noexcept {
    reinterpret_cast<void(__stdcall*)(uintptr_t,uint32_t,const uint32_t*,const int32_t*,uint32_t)>(Entry<0x4DBC0EC>())(bink,track,nullptr,volumes,8);}
int32_t BinkWait(uintptr_t bink) noexcept {return reinterpret_cast<int32_t(__stdcall*)(uintptr_t)>(Entry<0x4DBC10C>())(bink);}
int32_t BinkDoFrameAsyncWait(uintptr_t bink,int32_t microseconds) noexcept {return reinterpret_cast<int32_t(__stdcall*)(uintptr_t,int32_t)>(Entry<0x4DBC12C>())(bink,microseconds);}
int32_t BinkDoFrameAsyncMulti(uintptr_t bink,uintptr_t threads,int32_t count) noexcept {
    return reinterpret_cast<int32_t(__stdcall*)(uintptr_t,uintptr_t,int32_t)>(Entry<0x4DBC0CC>())(bink,threads,count);}
int32_t BinkShouldSkip(uintptr_t bink) noexcept {return reinterpret_cast<int32_t(__stdcall*)(uintptr_t)>(Entry<0x4DBC0E4>())(bink);}
void BinkNextFrame(uintptr_t bink) noexcept {reinterpret_cast<void(__stdcall*)(uintptr_t)>(Entry<0x4DBC114>())(bink);}
// The engine's string (0x30: inline buffer 16, size, capacity, allocator, encoding byte); heap text past the inline capacity.
using EngineString=std::array<uint64_t,6>;
void FreeEngineString(EngineString& text,uint64_t inline_capacity) noexcept {
    if(text[3]>inline_capacity)VCall<void>(text[4],0x68,text[0]);
    text[3]=inline_capacity;text[2]=0;text[0]=0;
}
// The native output: one per open, held in the player's output texture/view/target fields (+0x30/+0x38/+0x40).
struct Output {uintptr_t player{},proxy{};uint32_t width{},height{};bool published{};uint64_t drawn{},dropped{},skipped{};};
std::mutex lock;
std::unordered_set<Output*> outputs;
struct Pending {uintptr_t proxy{};uint32_t frame_number{};NativeMovieFrame frame;};
std::optional<Pending> pending;
// The device token the player stores (+0x28) and passes to the graphics object (+0x18): never dereferenced.
uint8_t device_token{};
Output* Find(uintptr_t token) noexcept {
    const std::lock_guard hold(lock);const auto found=outputs.find(reinterpret_cast<Output*>(token));return found==outputs.end()?nullptr:*found;}
// The game's programmes and unit quad (141AE0830's sources), checked once: DXBC containers whose size matches the cell.
struct Programmes {std::vector<uint8_t> vertex,pixel;std::array<float,8> quad{};bool valid{};};
const Programmes& GameProgrammes() noexcept {
    static const Programmes programmes=[]{Programmes p;
        const auto blob=[](uintptr_t cell,std::vector<uint8_t>& out){const auto address=P(image+cell);const auto size=U32(image+cell+8);
            if(!address || size<0x20 || size>0x10000)return false;out.resize(size);
            uint32_t declared{};return Read(address,out.data(),size) && std::memcmp(out.data(),"DXBC",4)==0 && (std::memcpy(&declared,out.data()+0x18,4),declared==size);};
        p.valid=blob(0x46CAD40,p.vertex) && blob(0x46CB4C8,p.pixel) && Read(image+0x3A9CE38,p.quad.data(),sizeof(p.quad));
        char row[200]{};sprintf_s(row,"{\"event\":\"native_movie_programmes\",\"valid\":%s,\"vertex_bytes\":%zu,\"pixel_bytes\":%zu}\n",
            p.valid?"true":"false",p.vertex.size(),p.pixel.size());Log(row);
        return p;}();
    return programmes;
}
// Witnesses, both modes: the open (Bink header, colour block, plane buffers), the first frame, every 240th frame, the close.
struct Witness {uintptr_t player{};uint32_t bucket{};uint64_t open_tick{};};
Witness witness;
void WitnessRender(uintptr_t player,uint8_t drawn_before) noexcept {
    const auto bink=P(player+0x48);if(!bink)return;
    std::array<uint32_t,7> head{};Read(bink,head.data(),sizeof(head));   // width, height, frames, frame number, -, rate, rate divisor
    const auto tick=GetTickCount64();
    if(witness.player!=player){witness={player,0,tick};
        std::array<float,16> colour{};Read(bink+0xD0,colour.data(),sizeof(colour));
        const auto graphics=P(player+0x68);std::array<uint32_t,5> buffers{};if(graphics)Read(graphics+0x68,buffers.data(),sizeof(buffers));
        std::string block;for(const auto value:colour){char cell[24]{};sprintf_s(cell,"%s%.6g",block.empty()?"":",",value);block+=cell;}
        char row[640]{};sprintf_s(row,"{\"event\":\"movie_witness\",\"edge\":\"open\",\"tick\":%llu,\"width\":%u,\"height\":%u,\"frames\":%u,\"frame\":%u,"
            "\"rate\":%u,\"rate_divisor\":%u,\"sets\":%u,\"luma\":[%u,%u],\"chroma\":[%u,%u],\"colour\":[%s]}\n",static_cast<unsigned long long>(tick),
            head[0],head[1],head[2],head[3],head[5],head[6],buffers[0],buffers[1],buffers[2],buffers[3],buffers[4],block.c_str());Log(row);}
    const auto drawn=U8(player+0x92);
    if(!drawn_before && drawn){char row[200]{};sprintf_s(row,"{\"event\":\"movie_witness\",\"edge\":\"first_frame\",\"tick\":%llu,\"after_open_ms\":%llu,\"frame\":%u}\n",
        static_cast<unsigned long long>(tick),static_cast<unsigned long long>(tick-witness.open_tick),head[3]);Log(row);}
    if(drawn && head[3]/240!=witness.bucket){witness.bucket=head[3]/240;char row[200]{};
        sprintf_s(row,"{\"event\":\"movie_witness\",\"edge\":\"progress\",\"tick\":%llu,\"after_open_ms\":%llu,\"frame\":%u}\n",
            static_cast<unsigned long long>(tick),static_cast<unsigned long long>(tick-witness.open_tick),head[3]);Log(row);}
}
void WitnessClose(uintptr_t player) noexcept {
    const auto bink=P(player+0x48);std::array<uint32_t,4> head{};if(bink)Read(bink,head.data(),sizeof(head));
    const auto tick=GetTickCount64();char row[240]{};
    sprintf_s(row,"{\"event\":\"movie_witness\",\"edge\":\"close\",\"tick\":%llu,\"after_open_ms\":%llu,\"frame\":%u,\"frames\":%u,\"play_state\":%u}\n",
        static_cast<unsigned long long>(tick),static_cast<unsigned long long>(witness.player==player?tick-witness.open_tick:0),head[3],head[2],U8(player+0x90));Log(row);
    if(witness.player==player)witness={};
}
}
// Player +0x18 open 141ADF230: every Bink call and the path helpers as the original; the GX output texture with its
// view and target (141925 6D0/B10/A30 on the old device) becomes one native output token.
bool __fastcall NativeMoviePlayerOpen(uintptr_t player,uintptr_t name,uintptr_t desc) noexcept {
    using namespace native_movie;
    auto& state=*reinterpret_cast<int32_t*>(player+0x18);
    const auto close=[&]{VCall<bool>(player,0x28);return false;};
    if(!name)return close();
    if(state)return false;
    if(!U8(player+0x91))return close();
    state=3;
    alignas(16) std::array<uint8_t,0x40> path{};const auto path_object=reinterpret_cast<uintptr_t>(path.data());
    reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uint8_t)>(image+0x17930E0)(path_object,name,P(player+8),0,0);
    auto source=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0x1793820)(path_object);
    if(P(source+0x18)>=8)source=P(source);
    const auto allocator=reinterpret_cast<uintptr_t(__fastcall*)()>(image+0x177A330)();
    {uint64_t scratch{};const auto flags=VCall<const uint32_t*>(allocator,0x18,&scratch,uintptr_t{0});
        if(!((*flags>>5)&1))reinterpret_cast<void(__fastcall*)(uintptr_t,uint32_t,uintptr_t,uintptr_t)>(image+0x1777DE0)(0,0x3E,image+0x39F23D0,0);}
    EngineString wide{};wide[3]=7;wide[4]=allocator;
    size_t length{};while(reinterpret_cast<const wchar_t*>(source)[length])++length;
    reinterpret_cast<void(__fastcall*)(EngineString*,uintptr_t,size_t)>(image+0x23C0)(&wide,source,length);
    reinterpret_cast<uint8_t*>(&wide)[0x28]=1;
    EngineString narrow{};
    {uint64_t scratch{};narrow[4]=*reinterpret_cast<uintptr_t*(__fastcall*)(EngineString*,uint64_t*)>(image+0x1C040)(&wide,&scratch);}
    narrow[3]=0xF;reinterpret_cast<uint8_t*>(&narrow)[0x28]=3;
    {bool convert=true;const auto text=wide[3]>=8?wide[0]:reinterpret_cast<uintptr_t>(&wide);
        reinterpret_cast<void(__fastcall*)(EngineString*,uintptr_t,uintptr_t,uint64_t,bool*)>(image+0x63170)(&narrow,0,text,wide[2],&convert);}
    FreeEngineString(wide,7);
    uint32_t flags=U8(player+0x93)?0x100:0x400;
    const auto tracks=P(desc);const auto count=U32(desc+8);
    if(tracks){BinkSetSoundTrack(count,tracks);flags|=0x4000;}
    if(const auto io=U32(desc+0x10)){BinkSetIOSize(io);flags|=0x1000000;}
    const auto bink=BinkOpen(narrow[3]>=0x10?reinterpret_cast<const char*>(narrow[0]):reinterpret_cast<const char*>(&narrow),flags);
    *reinterpret_cast<uintptr_t*>(player+0x48)=bink;
    bool planes{};
    if(bink){
        // Per track index: left and right, centre, LFE, the rear pair, then the next pair (8 speaker volumes, x 32768).
        const auto volume=static_cast<int32_t>(F32(desc+0xC)*F32(image+0x3A9CD98));
        static constexpr std::array<std::array<int,2>,5> speakers{{{0,1},{2,-1},{3,-1},{4,5},{6,7}}};
        for(uint32_t i=0;i<5 && i<count && tracks;++i){std::array<int32_t,8> volumes{};for(const auto s:speakers[i])if(s>=0)volumes[s]=volume;
            BinkSetSpeakerVolumes(bink,U32(tracks+4*i),volumes.data());}
        planes=VCall<bool>(P(player+0x68),0x18,bink);
    }
    FreeEngineString(narrow,0xF);
    reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0x17931A0)(path_object);
    if(!planes)return close();
    const auto width=U32(bink),height=U32(bink+4);
    *reinterpret_cast<uint32_t*>(player+0x1C)=width;*reinterpret_cast<uint32_t*>(player+0x20)=height;
    if(!width || !height || width>4096 || height>4096)return close();
    auto* output=new(std::nothrow) Output{player,0,width,height};if(!output)return close();
    {const std::lock_guard hold(lock);outputs.insert(output);}
    for(const auto field:{0x30,0x38,0x40})*reinterpret_cast<uintptr_t*>(player+field)=reinterpret_cast<uintptr_t>(output);
    *reinterpret_cast<uint8_t*>(player+0x94)=U8(desc+0x14);
    state=2;
    char row[200]{};sprintf_s(row,"{\"event\":\"native_movie_open\",\"width\":%u,\"height\":%u,\"flags\":%u,\"tracks\":%u}\n",width,height,flags,count);Log(row);
    return true;
}
// Player +0x28 close 141ADE850: the native output (and the proxy's native target) instead of the three GX objects.
bool __fastcall NativeMoviePlayerClose(uintptr_t player) noexcept {
    using namespace native_movie;
    if(!VCall<bool>(player,0x40))return false;
    WitnessClose(player);
    if(const auto token=P(player+0x30)){
        auto* output=Find(token);if(!output)Refuse("native_movie_output_unknown");
        {const std::lock_guard hold(lock);outputs.erase(output);if(pending && pending->proxy==output->proxy)pending.reset();}
        if(output->proxy)textures.Retire(output->proxy);
        char row[240]{};sprintf_s(row,"{\"event\":\"native_movie_close\",\"drawn\":%llu,\"dropped\":%llu,\"skipped\":%llu,\"published\":%s}\n",
            output->drawn,output->dropped,output->skipped,output->published?"true":"false");Log(row);
        delete output;}
    for(const auto field:{0x30,0x38,0x40})*reinterpret_cast<uintptr_t*>(player+field)=0;
    if(const auto graphics=P(player+0x68))VCall<void>(graphics,0x20,P(player+0x48));
    if(const auto bink=P(player+0x48)){BinkClose(bink);*reinterpret_cast<uintptr_t*>(player+0x48)=0;}
    *reinterpret_cast<uint8_t*>(player+0x94)=0;*reinterpret_cast<int32_t*>(player+0x18)=0;
    return true;
}
// Player +0x68 render 141ADEAF0: the original sequence without the GX state save/restore (141ADF7C0/141ADEED0).
bool __fastcall NativeMoviePlayerRender(uintptr_t player,uintptr_t context,uintptr_t view) noexcept {
    using namespace native_movie;
    if(VCall<bool>(player,0x40))return false;
    // I-0132 R12e: a playing film covers the « Rekindled » title (16b), as a loading screen does: New Game's opening film is a Bink
    // film, not a UI movie, and the title's logo movie goes on advancing under it (i66-native-i137-t4-charmake2: the title drawn over
    // the film for about 200 s; no film plays at the title itself in the r12e runs).
    rekindled_title_cover_tick.store(GetTickCount64(),std::memory_order_relaxed);
    const auto bink=P(player+0x48),graphics=P(player+0x68),threads=P(player+0x58);const auto count=static_cast<int32_t>(U32(player+0x50));
    const auto drawn_before=U8(player+0x92);
    if(!BinkWait(bink) && BinkDoFrameAsyncWait(bink,1000)){
        auto& play=*reinterpret_cast<uint8_t*>(player+0x90);
        if(play==2){VCall<void>(graphics,0x28,bink,context);BinkDoFrameAsyncMulti(bink,threads,count);play=1;}
        VCall<void>(graphics,0x30,bink,context);
        uint64_t skipped{};
        while(BinkShouldSkip(bink)){BinkNextFrame(bink);VCall<void>(graphics,0x28,bink,context);BinkDoFrameAsyncMulti(bink,threads,count);
            VCall<void>(graphics,0x30,bink,context);++skipped;}
        BinkNextFrame(bink);VCall<void>(graphics,0x28,bink,context);BinkDoFrameAsyncMulti(bink,threads,count);
        VCall<void>(graphics,0x38,bink,context,P(player+0x40));
        if(skipped)if(auto* output=Find(P(player+0x40)))output->skipped+=skipped;
        if(!drawn_before)*reinterpret_cast<uint8_t*>(player+0x92)=1;
    }
    WitnessRender(player,drawn_before);
    if(!U8(player+0x92))return false;
    *reinterpret_cast<uintptr_t*>(view)=P(player+0x38);
    return true;
}
// Graphics +0x08 init 141AE0830: the GX sampler/states/programmes/buffers become the renderer's lazily built root
// signature and pipeline (scene_pass RenderNativeMovie) from the same programmes, checked here.
bool __fastcall NativeMovieGraphicsInit(uintptr_t graphics,uintptr_t allocator,uintptr_t secondary,uintptr_t device) noexcept {
    using namespace native_movie;
    if(U8(graphics+0x218))return true;
    const auto release=[&]{VCall<void>(graphics,0x10);return false;};
    if(!allocator || !secondary || !device)return release();
    *reinterpret_cast<uintptr_t*>(graphics+8)=allocator;*reinterpret_cast<uintptr_t*>(graphics+0x10)=secondary;
    *reinterpret_cast<uintptr_t*>(graphics+0x18)=device;
    if(P(graphics+0x20) || !GameProgrammes().valid)return release();
    *reinterpret_cast<uint8_t*>(graphics+0x218)=1;
    return true;
}
// Graphics +0x18 planes 141AE0BA0: Bink's frame buffers as the original; the R8 plane textures are the renderer's
// (recreated with the buffer size), so the GX texture and view fields stay empty.
bool __fastcall NativeMovieGraphicsPlanes(uintptr_t graphics,uintptr_t bink) noexcept {
    using namespace native_movie;
    if(U8(graphics+0x219))return true;
    if(bink){BinkGetFrameBuffersInfo(bink,graphics+0x68);
        if(P(graphics+0x80) || BinkAllocateFrameBuffers(bink,graphics+0x68,0)){
            BinkRegisterFrameBuffers(bink,graphics+0x68);*reinterpret_cast<uint8_t*>(graphics+0x219)=1;
            std::array<uint32_t,5> buffers{};Read(graphics+0x68,buffers.data(),sizeof(buffers));
            char row[200]{};sprintf_s(row,"{\"event\":\"native_movie_planes\",\"sets\":%u,\"luma\":[%u,%u],\"chroma\":[%u,%u],\"alpha\":%u}\n",
                buffers[0],buffers[1],buffers[2],buffers[3],buffers[4],U32(graphics+0x80+0x38));Log(row);
            return true;}}
    VCall<void>(graphics,0x20,bink);
    return false;
}
// Graphics +0x38 draw 141ADFF10: the ready set's planes and the 9 x float4 block, queued for presentation (the latest
// frame wins; the output keeps its image when no set is ready, as the original redraws its last upload).
void __fastcall NativeMovieGraphicsDraw(uintptr_t graphics,uintptr_t bink,uintptr_t,uintptr_t target) noexcept {
    using namespace native_movie;
    if(!U8(graphics+0x219))return;
    // I-0137 tranche 6 (ROADMAP workstream 4 step 2): a frame whose ready set, plane shape or plane buffer Bink describes out of
    // the module's reach is the decoder's data: that frame is skipped and counted, the output keeping its last image (as when no
    // set is ready); an output token the module does not own stays a refusal (its own registry).
    const auto set=U32(graphics+0x7C);
    if(set>2 || set>=U32(graphics+0x68)){NoteNativeSkip("movie_plane_set",set);return;}
    auto& ready=*reinterpret_cast<uint8_t*>(graphics+0x180+set*0x48);
    if(!ready)return;
    ready=0;
    auto* output=Find(target);if(!output)Refuse("native_movie_output_unknown");
    const auto& programmes=GameProgrammes();
    NativeMovieFrame frame;frame.width=U32(bink);frame.height=U32(bink+4);
    frame.luma_width=U32(graphics+0x6C);frame.luma_height=U32(graphics+0x70);frame.chroma_width=U32(graphics+0x74);frame.chroma_height=U32(graphics+0x78);
    if(!frame.luma_width || !frame.luma_height || !frame.chroma_width || !frame.chroma_height || frame.width!=output->width || frame.height!=output->height)
        {NoteNativeSkip("movie_plane_shape",frame.luma_width);return;}
    const auto planes=graphics+0x80+uintptr_t(set)*0x40;   // {buffer +0, allocate +8, pitch +0xC} for Y, cR, cB, A
    for(const auto& [plane,width]:{std::pair{planes,frame.luma_width},std::pair{planes+0x10,frame.chroma_width},std::pair{planes+0x20,frame.chroma_width}})
        if(!P(plane) || U32(plane+0xC)<width){NoteNativeSkip("movie_plane_buffer",plane-planes);return;}
    const auto copy=[&](uintptr_t plane,uint32_t width,uint32_t height,std::vector<uint8_t>& out){
        const auto buffer=P(plane);const auto pitch=U32(plane+0xC);
        out.resize(size_t(width)*height);
        for(uint32_t row=0;row<height;++row)std::memcpy(out.data()+size_t(row)*width,reinterpret_cast<const void*>(buffer+uintptr_t(row)*pitch),width);};
    copy(planes,frame.luma_width,frame.luma_height,frame.luma);
    copy(planes+0x10,frame.chroma_width,frame.chroma_height,frame.chroma_r);
    copy(planes+0x20,frame.chroma_width,frame.chroma_height,frame.chroma_b);
    // c0 colour (143D7FEF0); c1..c4 the 64 bytes at HBINK+0xD0; c5 position (2, -2, -1, 1); c6/c7 the luma and chroma
    // crops; c8 zero (the products with 0 of 141ADFF10).
    auto& c=frame.constants;
    Read(image+0x3D7FEF0,c.data(),16);Read(bink+0xD0,c.data()+4,64);
    const float one=F32(image+0x3D7EE10),two=F32(image+0x3D7EE7C),minus_two=F32(image+0x3D7F0D8),scale=one-0.0f;
    const float luma_x=float(frame.width)/float(frame.luma_width),luma_y=float(frame.height)/float(frame.luma_height);
    const float chroma_x=float(frame.width>>1)/float(frame.chroma_width),chroma_y=float(frame.height>>1)/float(frame.chroma_height);
    const std::array<float,16> tail{scale*two,scale*minus_two,two*0.0f-one,one-two*0.0f, scale*luma_x,0,scale*chroma_x,0,
        0,scale*luma_y,0,scale*chroma_y, luma_x*0.0f,luma_y*0.0f,chroma_x*0.0f,chroma_y*0.0f};
    std::copy(tail.begin(),tail.end(),c.begin()+20);
    frame.vertex_shader=programmes.vertex;frame.pixel_shader=programmes.pixel;frame.quad=programmes.quad;
    const std::lock_guard hold(lock);
    if(pending)++output->dropped;
    ++output->drawn;
    pending=Pending{output->proxy,U32(bink+0xC),std::move(frame)};
}
// 140EADE50 publish: the proxy's GX view slot (+0x18, with its description +0x20 and size +0x38) stays empty natively;
// the native output is bound to the proxy in the texture registry at open, and no GX reader of the proxy is reached.
void __fastcall NativeMoviePublish(uintptr_t proxy,uintptr_t view) noexcept {
    using namespace native_movie;
    if(P(proxy+0x18))*reinterpret_cast<uintptr_t*>(proxy+0x18)=0;
    if(!view)return;
    auto* output=Find(view);if(!output)Refuse("native_movie_output_unknown");
    if(!output->published){output->published=true;
        char row[160]{};sprintf_s(row,"{\"event\":\"native_movie_published\",\"tick\":%llu,\"proxy_bound\":%s}\n",
            static_cast<unsigned long long>(GetTickCount64()),output->proxy?"true":"false");Log(row);}
}
// CSMovieIns state 2 STEP_Init_Setup 140EAF7E0 with the native device token in the player init descriptor; after a
// successful open, the movie proxy (+0xB8) gets its native target at the video size, black until the first frame.
void __fastcall NativeMovieStepInit(uintptr_t self) noexcept {
    using namespace native_movie;
    reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t,uint64_t,uint64_t)>(image+0xE350)(self+0x150,P(image+0x479C258),0x7800000,0);
    const auto own_heap=[&]{return reinterpret_cast<bool(__fastcall*)(uintptr_t)>(image+0x17F2750)(self+0x178);};
    const auto cells=[]{return reinterpret_cast<uintptr_t*(__fastcall*)()>(image+0x1ADE5B0)();};
    if(!own_heap()){auto* globals=cells();
        const auto object=reinterpret_cast<uintptr_t(__fastcall*)(size_t,size_t,uintptr_t)>(image+0x1777A00)(0x18,8,P(image+0x479C288));
        if(object){auto* fields=reinterpret_cast<uintptr_t*>(object);fields[0]=image+0x279B538;fields[1]=P(image+0x479C258);fields[2]=P(image+0x479C2C8);}
        *reinterpret_cast<uintptr_t*>(self+0x5F8)=object;globals[0]=object;globals[1]=P(image+0x479BF30);}
    else{auto* globals=cells();globals[0]=self+0x168;globals[1]=P(image+0x479BF30);}
    auto& player=*reinterpret_cast<uintptr_t*>(self+0xC8);
    const auto drop=[&]{if(player){reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0x1ADE6E0)(player);player=0;}
        if(own_heap())reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0xE400)(self+0x150);};
    player=reinterpret_cast<uintptr_t(__fastcall*)()>(image+0x1ADE670)();
    if(player){struct alignas(8) Init {uintptr_t device;uint32_t threads;uint8_t no_graphics;} init{reinterpret_cast<uintptr_t>(&device_token),4,0};
        if(!VCall<bool>(player,0x08,&init))drop();}
    if(player){
        std::array<uint32_t,4> surround{0,1,2,3};uint32_t stereo=4;
        struct alignas(8) Open {const uint32_t* tracks;uint32_t count;float volume;uint32_t io;uint8_t loop;} open{};
        if(const auto mode=U32(self+0x110);mode==0){open.tracks=surround.data();open.count=4;}else if(mode==1){open.tracks=&stereo;open.count=1;}
        open.volume=F32(self+0x108);open.loop=U8(self+0x10C);
        EngineString name{};
        auto text=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,EngineString*)>(image+0x62000)(self+0xD0,&name);
        if(P(text+0x18)>=8)text=P(text);
        const bool movie_opened=VCall<bool>(player,0x18,text,&open);
        FreeEngineString(name,7);
        if(!movie_opened){VCall<void>(player,0x10);drop();}
        if(player){
            const auto proxy=P(self+0xB8);
            if(auto* output=Find(P(player+0x30));output && proxy){
                std::shared_ptr<const bootstrap::NativeTextureRecord> target;
                try{target=textures.AdmitOffscreenTarget(proxy,output->width,output->height,{0,0,0,1});}
                catch(const std::exception& e){char row[300]{};sprintf_s(row,"{\"event\":\"native_movie_target_failure\",\"message\":\"%.200s\"}\n",e.what());Log(row);Refuse("native_movie_target_failed");}
                {const std::lock_guard hold(lock);output->proxy=proxy;}
                uintptr_t table{};Read(proxy,&table,8);
                char row[200]{};sprintf_s(row,"{\"event\":\"native_movie_target\",\"width\":%u,\"height\":%u,\"proxy_table_rva\":%lld,\"resident\":%s}\n",
                    output->width,output->height,table?static_cast<long long>(table-image):-1ll,target && target->resident?"true":"false");Log(row);}
            reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0xEAFD80)(self);VCall<void>(self,0x30);return;}
    }
    *reinterpret_cast<uint16_t*>(self+0x148)=0;*reinterpret_cast<uint8_t*>(self+0x14A)=0;
    VCall<void>(self,0x20,uint32_t{1});VCall<void>(self,0x30);
}
// CSMovieIns state 6 STEP_Wait_Play 140EAFAC0: the render receives no GX context (the native render records none).
void __fastcall NativeMovieStepPlay(uintptr_t self,uintptr_t step) noexcept {
    using namespace native_movie;
    const auto player=P(self+0xC8);
    if(!player){reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0xEAFD80)(self);VCall<void>(self,0x30);return;}
    if(U8(self+0x149))VCall<void>(player,0x38);
    reinterpret_cast<void(__fastcall*)(uint32_t)>(image+0x17E8070)(2000);
    if(const auto pause=U8(self+0x14B);U8(self+0x14A)!=pause){VCall<void>(player,0x50,uint32_t{pause});*reinterpret_cast<uint8_t*>(self+0x14A)=pause;}
    VCall<void>(player,0x48);
    uintptr_t view{};
    if(VCall<bool>(player,0x68,uintptr_t{0},&view))reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t)>(image+0xEADE50)(P(self+0xB8),view);
    reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t)>(image+0xEB0890)(self,step);
    if(!P(image+0x489DD10))reinterpret_cast<void(__fastcall*)(uintptr_t,uint32_t,uintptr_t,uintptr_t)>(image+0x1777DE0)(image+0x2743560,0xB1,image+0x3D737E0,image+0x2860D70);
    reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0xEEE240)(P(image+0x489DD10));
    if(VCall<bool>(player,0x40))reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0xEAFD80)(self);
}
// Presentation: the latest queued movie frame into the proxy's native target, before the menu page samples it.
// R13 cR2: collected into the frame job (the target record held by the job); the frame job draws it.
void CollectNativeMovieFrame(NativeFrameJob& job) noexcept {
    std::optional<native_movie::Pending> frame;
    {const std::lock_guard hold(native_movie::lock);frame.swap(native_movie::pending);}
    if(!frame)return;
    try {
        const auto target=textures.Acquire(frame->proxy);
        if(!target || !target->resident || !target->resource){static LONG64 absent{};if(InterlockedIncrement64(&absent)<=6)
            Log("{\"event\":\"native_movie_target_absent\"}\n");return;}
        std::wstring pixel_file;
        static uint32_t next_picture=240;   // witness: the first presented frames at or past Bink frames 240 and 2400 of the process's first movie
        if(WitnessPictures() && frame->frame_number>=next_picture && next_picture<=2400){next_picture=next_picture==240?2400:UINT32_MAX;   // Fable review: not in a play run
            pixel_file=JournalSibling(L"movie-"+std::to_wstring(frame->frame_number)+L".rgba");}
        job.movie=std::move(frame->frame);job.movie_target=target->resource;job.movie_target_owner=target;job.movie_pixel_file=pixel_file;
        static LONG64 drawn{};const auto n=InterlockedIncrement64(&drawn);
        if(n<=3 || n%480==0 || !pixel_file.empty()){char row[200]{};sprintf_s(row,"{\"event\":\"native_movie_frame\",\"frame\":%u,\"presented\":%lld,\"pixels\":%s}\n",
            frame->frame_number,static_cast<long long>(n),pixel_file.empty()?"false":"true");Log(row);}
    }catch(const std::exception& e){char row[400]{};sprintf_s(row,"{\"event\":\"native_movie_frame_failure\",\"message\":\"%.300s\"}\n",e.what());Log(row);
        Refuse("native_movie_frame_failed");}
}
// Reference witnesses (pass-through): the same open/first-frame/progress/close rows from the original player.
using MovieRender=bool(__fastcall*)(uintptr_t,uintptr_t,uintptr_t);MovieRender movie_render_original{};
bool __fastcall ObserveMovieRender(uintptr_t player,uintptr_t context,uintptr_t view) noexcept {
    const auto drawn_before=native_movie::U8(player+0x92);const bool result=movie_render_original(player,context,view);
    native_movie::WitnessRender(player,drawn_before);return result;}
using MovieClose=bool(__fastcall*)(uintptr_t);MovieClose movie_close_original{};
bool __fastcall ObserveMovieClose(uintptr_t player) noexcept {
    if(native_movie::VCall<bool>(player,0x40))native_movie::WitnessClose(player);return movie_close_original(player);}
// 1261 (Native1257/1260, quit in game): 14000B4E0(device) flushes the old GX device's command queue ([device+0x380]) unless
// called on its render thread; its callers (area unload 8E9EE0, resource retirements ...) pass [144961AB0], the old
// display (nil natively), or, in the device's own methods, the device itself (never present natively): the call faulted at
// B4F2 with this = 0. The native renderer owns its own GPU
// synchronisation and no GX command queue exists: with no device there is nothing to flush (explicit omission); a
// device, never present natively, is refused.
void __fastcall NativeGxDeviceSync(uintptr_t device) noexcept {
    if(device)Refuse("native_gx_device_sync_unowned");
    static LONG64 calls{};const auto n=InterlockedIncrement64(&calls);
    if(n<=8 || n%500==0){char row[160]{};sprintf_s(row,"{\"event\":\"native_gx_device_sync_omitted\",\"call\":%lld,\"caller\":%llu}\n",
        static_cast<long long>(n),static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(_ReturnAddress())-image));Log(row);}
}
}  // namespace
