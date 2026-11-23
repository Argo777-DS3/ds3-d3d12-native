// The engine's scene CPU step (UpdateNativeSceneCpu), model storage admission and retirement, CPU geometry and texture
// admission. Part of game_bootstrap.cpp's single translation unit (the order is there): it relies on every earlier part
// (13b_prepare_model_frame.inc before it).
#pragma once
#include "13b_prepare_model_frame.inc"
#include "../dark_logo_kept.h"   // D-0182 (I-0135)
namespace {
// The genuine UI manager's native session, once the manager and its HAL exist (E50230's container +8 owns the manager), or none.
std::shared_ptr<bootstrap::GameUiSession> NativeUiFrameSession(){
    uintptr_t container{},manager{},hal{};
    if(!Read(image+0x4798EF8,&container,8))Refuse("native_ui_container_unreadable");
    if(!container)return {};
    if(!Read(container+8,&manager,8) || !manager || !Read(manager+0x980,&hal,8))Refuse("native_ui_container_contract");
    if(!hal)return {};
    const std::lock_guard<std::mutex> lock(ui_sessions_lock);
    const auto found=ui_sessions.find(manager);return found!=ui_sessions.end()?found->second:nullptr;
}
// I-0088 P1: the UI display of frame serial+1 at the output extent posted to the UI worker (once a frame, at the collect's
// entry); the render step joins it (TakeNativeUiDisplay) and EndFrame always does (FinishNativeUiDisplay).
void PostNativeUiDisplay(){
    // Step review M1: a stop request (Close stops the worker and may defer the rest) leaves the display to the render step.
    if(!ui_display_worker || native_ui_display.posted || control->stop || !ui_display_worker->Running())return;
    auto session=NativeUiFrameSession();if(!session)return;
    native_ui_display=NativeUiDisplay{true,serial+1,output_width,output_height};
    ui_display_worker->Post([session=std::move(session)]{
        if(const auto hook=WorkPool::worker_hook.load(std::memory_order_relaxed))hook("ui");   // a sample-profile run sees the worker
        const std::lock_guard<std::timed_mutex> preparation(ui_frame_preparation_lock);auto& display=native_ui_display;
        display.movies=session->PrepareFrame(primitive_programs,display.serial,display.width,display.height);
        display.packet=session->TakePreparedFrame();display.reports=session->TakeFilterReports();});
}
// The render step's join: the posted display and its filter reports. Its frame is the step's (serial moves only in Clear); its extent
// the collect's, which BeginFrame can change between the collect and the render step (ENGINE-STEP-THREAD-MAP: collect, BeginFrame,
// render step; I-0132 R12f's run r12f-live5: the Resolution row's change faulted the worker's UI admission, « primitive admission
// identity »). Either change is logged; a packet of another frame or made at the former extent is not kept (that frame is presented
// without its UI: the movies' prepared flags were taken by the collect's preparation, so none can be prepared again for it; its step
// review's M1), the next frame's UI at the new extent.
bool TakeNativeUiDisplay(uint32_t& movies){
    if(!native_ui_display.posted)return false;
    ui_display_worker->Wait();auto& display=native_ui_display;
    for(const auto& report:display.reports)Log(report.c_str());
    display.reports.clear();
    const bool same_frame=display.serial==serial+1,same_extent=display.width==output_width && display.height==output_height;
    native_ui_display_dropped=!same_frame || !same_extent;   // I-0137 tranche 7: the frame's cause if it has no UI packet
    if(same_frame && same_extent){movies=display.movies;if(display.packet)native_ui_frame=std::move(display.packet);}
    if(!same_frame || !same_extent){static LONG64 changed{};const auto n=InterlockedIncrement64(&changed);if(n<=8 || !(n%100)){char row[192]{};
        sprintf_s(row,"{\"event\":\"native_ui_display_changed\",\"count\":%lld,\"serial_changed\":%s,\"extent_changed\":%s}\n",static_cast<long long>(n),
            same_frame?"false":"true",same_extent?"false":"true");Log(row);}}
    display.packet.reset();return true;
}
void FinishNativeUiDisplay(){
    if(!native_ui_display.posted)return;
    ui_display_worker->Wait();for(const auto& report:native_ui_display.reports)Log(report.c_str());   // step review L6: not lost
    native_ui_display=NativeUiDisplay{};
}
// I-0138 tranche 8h: the original's draw gate (READ, GhidraMCP and the exe's bytes). D55190 (the render step NativeRenderStep replaces)
// draws only when FUN_140d56da0([144793348]) is true: the resource root's +0xCA clear, then the timing object [14489DD10]'s +0x27B
// clear. +0x27B is set by SprjRemoIns Play_Wait (FUN_140eee230, its only caller) when the current cut's frame is past its end, and
// cleared at the limiter's exit. +0xCA is written once a step by FUN_140d56e00, reached from SprjDrawStep's slot 0x80 (140D55170,
// through FUN_140d56d20, which first runs the root's +0x60 list FUN_140d6e1e0) only while the root's +0x98 is set: +0xCA = +0xC9 ? 0
// : (+0xC8 or FUN_140eed7f0, i.e. +0x27B, else the limiter's catch-up +0x278), then +0xC8 and +0xC9 cleared. The root's constructor
// FUN_140d55db0 writes +0xC8 to +0xCA as 0 (140D55E6F) and +0x98 as 0 (140D55E24); D573B0, the root's GX set-up, sets +0x98 at its
// end and makes the +0x60 object. Natively D573B0 is not run (20_startup_phases.inc): +0x98 stays 0, +0x60 stays null.
// I-0138 tranche 8i: the +0x60 object is the screenshot manager (SprjScreenCapture, made at 140D5757C); +0xC9's only setter
// FUN_140d56d90 (140D6E60C, 140D6E928, 140D6EA95) runs only while a screenshot request is pending, and in play nothing makes one (the
// key path FUN_140a27a10(0x2c) a stub; READ, evidence/I-0138/t8i-readings/draw-gate-c9-list.md; that no other path makes one in the
// retail game is INFERRED). So the original's +0xCA is FUN_140eed7f0's answer on every drawn-or-not step of play: +0x27B, else
// +0x278. +0x278 is written only by the limiter (140EEDD6C: forced while +0x27B is set; 140EEDEE1: set when the frame window, +0x270
// or +0x274 by focus at 140EED88E, is over budget by more than 1 % of the period; INFERRED from the listing, the limiter's head
// encrypted). The module computes +0xCA natively by calling the original's FUN_140d56e00 once a step (below): +0xC9 stays 0 as in
// play, so the native gate closes on +0x27B and on +0x278 as the original's: a cut start's first steps, the step after each +0x27B
// run (+0x278 still set; drawn one step early before 8i, OBSERVED i138-t8h3), and the limiter's catch-up steps. The order differs on
// one point: the original computes +0xCA before Play_Wait, the module at the step's first reading of the gate (the collect's, after
// it), so a +0x27B run's first step reads +0xCA 1 natively and 0 in the original (OBSERVED i138-t8h4); +0x27B closes it in both.
// Natively a closed step runs no collect, no render step UI and no frame in EndFrame (no render, no present, no pacing; the original's
// limiter skips its wait on +0x27B, CADENCE-STUDY A0, INFERRED for a step closed by +0x278 alone): the screen keeps the last drawn
// frame. That the original presents nothing on such a step is INFERRED (its frame end still posts a slot to its render thread, and
// the steps come several to a 15.6 ms tick); its post waits for the previous job (D3D12-R13-R14-RENDER-THREAD-PACING-SHEET), the
// native's closed step has no wait of its own (DS3's limiter still runs after it). Without the root or the timing object (start-up)
// the gate is not read and the frame is drawn as before (the original's D55190 draws nothing without the root and asserts without
// the timing object). The other readers of FUN_140d56da0, the cloth vertex update (140DD8C40 to 140DD8F70), read the computed +0xCA
// (whether they run before or after the step's first gate reading, so on this step's value or the last one's, is not read: open).
struct NativeDrawGate {bool known{};BYTE skip{},remo{},catch_up{};bool Open() const noexcept {return !known || (!skip && !remo);}};
// +0xCA computed once an engine step natively (the root's +0x98 clear), by the first of the collect, the render step and EndFrame to
// read the gate. The called code is checked once, byte for byte to its returns: FUN_140d56e00 (140D56E00 to 140D56E73) and
// FUN_140eed7f0 (20 bytes). Its assert path (a null timing object, 140D56E2F) is not reached: the timing pointer is read non-null
// just before the call, on the engine thread (the callers' thread, unchecked here; that no other thread writes [14489DD10] is
// INFERRED).
void ComputeNativeDrawSkip(uintptr_t root) noexcept {
    static const bool code_known=[]{
        static constexpr BYTE body[]{0x40,0x53,0x48,0x83,0xEC,0x20,0x48,0x8B,0xD9,0x48,0x8B,0x0D,0x00,0x6F,0xB4,0x03,0x48,0x85,0xC9,0x75,0x26,0x4C,0x8D,0x0D,0x54,0x9F,0xB0,0x01,
            0x4C,0x8D,0x05,0xBD,0xC9,0x01,0x03,0x48,0x8D,0x0D,0x36,0xC7,0x9E,0x01,0xBA,0xB1,0x00,0x00,0x00,0xE8,0xAC,0x0F,0xA2,0x00,0x48,0x8B,0x0D,0xD5,
            0x6E,0xB4,0x03,0xE8,0xB0,0x69,0x19,0x00,0x84,0xC0,0x74,0x07,0xC6,0x83,0xC8,0x00,0x00,0x00,0x01,0x80,0xBB,0xC9,0x00,0x00,0x00,0x00,0x74,0x04,
            0x33,0xC0,0xEB,0x07,0x0F,0xB6,0x83,0xC8,0x00,0x00,0x00,0x88,0x83,0xCA,0x00,0x00,0x00,0x66,0xC7,0x83,0xC8,0x00,0x00,0x00,0x00,0x00,
            0x48,0x83,0xC4,0x20,0x5B,0xC3};
        static constexpr BYTE test[]{0x80,0xB9,0x7B,0x02,0x00,0x00,0x00,0xB0,0x01,0x75,0x07,0x0F,0xB6,0x81,0x78,0x02,0x00,0x00,0xF3,0xC3};
        BYTE live[sizeof(body)]{};BYTE live_test[sizeof(test)]{};
        return Read(image+0xD56E00,live,sizeof(live)) && !std::memcmp(live,body,sizeof(body)) &&
            Read(image+0xEED7F0,live_test,sizeof(live_test)) && !std::memcmp(live_test,test,sizeof(test));}();
    if(!code_known)Refuse("native_draw_gate_code_identity");
    uintptr_t timing{};if(!Read(image+0x489DD10,&timing,8) || !timing)return;   // the assert path stays unreached whoever calls
    BYTE ready{};if(Read(root+0x98,&ready,1) && !ready)reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0xD56E00)(root);
}
bool native_draw_gate_read{};   // this step read the gate, and so computed +0xCA (the engine thread; cleared by EndFrame)
NativeDrawGate ReadNativeDrawGate() noexcept {
    const bool first=!std::exchange(native_draw_gate_read,true);
    NativeDrawGate gate{};uintptr_t root{},timing{};
    if(!Read(image+0x4793348,&root,8) || !root || !Read(image+0x489DD10,&timing,8) || !timing)return gate;
    if(first)ComputeNativeDrawSkip(root);
    gate.known=Read(root+0xCA,&gate.skip,1) && Read(timing+0x27B,&gate.remo,1);
    if(!gate.known)gate={};else if(!Read(timing+0x278,&gate.catch_up,1))gate.catch_up=0xFF;   // +0x278 for the row (+0xCA carries it)
    return gate;
}
// The collect's reading of this engine step (0 no collect, 1 open, 2 closed): when a collect ran, the render step follows it (the
// drawn frame is the collected one, native_velocity_history, the native velocity's previous camera, and the published scene move
// only with drawn frames; the game camera's previous sample steps as in the original) and counts a reading of its own that differs; set and read on the engine thread only, cleared by EndFrame.
int native_collect_gate{};
NativeDrawGate native_collect_reading{};   // the collect's reading, counted by EndFrame when no render step read the gate
bool native_draw_skipped{};   // set by the collect, then NativeRenderStep, for EndFrame (the engine thread)
// Rows: one when a closed run starts and at its end (the first 64 runs, then every 100th), one every 600th step of a run still closed
// (a gate that stays closed freezes the screen, as the original's would: INFERRED), each with the run's steps by cause (+0x27B, +0xCA,
// +0x278, and the steps closed without +0x27B (8i: the limiter's catch-up, +0xCA alone), read by the render step, or by the collect when no render step read the gate: EndFrame counts that step), the steps whose render step read the gate otherwise than their collect, and the scene clock when a
// witness scene holds one.
void NoteNativeDrawGate(bool closed,const NativeDrawGate& gate,bool disagreed) noexcept {
    struct Run {uint64_t steps{},remo{},skip{},catch_up{},without_remo{},disagreed{};};static Run run;static uint64_t runs{},disagreements{};
    if(disagreed)++disagreements;
    const auto row=[&](const char* phase){char text[512]{};   // about 370 characters at most (sprintf_s ends the process on an overflow)
        sprintf_s(text,"{\"event\":\"native_draw_gate_skipped\",\"phase\":\"%s\",\"run\":%llu,\"steps\":%llu,\"remo\":%llu,\"skip\":%llu,\"catch_up\":%llu,\"without_remo\":%llu,\"disagreed\":%llu,\"disagreements\":%llu%s}\n",
            phase,static_cast<unsigned long long>(runs),static_cast<unsigned long long>(run.steps),static_cast<unsigned long long>(run.remo),
            static_cast<unsigned long long>(run.skip),static_cast<unsigned long long>(run.catch_up),static_cast<unsigned long long>(run.without_remo),
            static_cast<unsigned long long>(run.disagreed),
            static_cast<unsigned long long>(disagreements),CutsceneClockCell().c_str());Log(text);};
    if(closed){const bool first=!run.steps;if(first)++runs;
        ++run.steps;run.remo+=gate.remo!=0;run.skip+=gate.skip!=0;run.catch_up+=gate.catch_up==1;run.without_remo+=!gate.remo;run.disagreed+=disagreed;
        if(first && (runs<=64 || !(runs%100)))row("start");
        if(!(run.steps%600))row("closed");return;}
    if(run.steps){if(runs<=64 || !(runs%100))row("end");run={};}
    else if(disagreed && disagreements<=16){char text[160]{};
        sprintf_s(text,"{\"event\":\"native_draw_gate_disagreed\",\"count\":%llu}\n",static_cast<unsigned long long>(disagreements));Log(text);}
}
void __fastcall UpdateNativeSceneCpu(uintptr_t stage,uintptr_t tick) noexcept {
    census::Note(census::SceneCpu);   // R13 cR0
    kprofile::Mark(kprofile::UpdateEntry);const kprofile::MarkOnExit update_exit{kprofile::UpdateExit};
    try {
        // I-0138 tranche 8h: a step the original's draw gate leaves undrawn collects nothing (no UI display, no selection, no menu
        // views): the published scene stays the last drawn one, and native_velocity_history (the native velocity's previous
        // camera) moves only with drawn frames; the game camera's previous sample, the models' poses and the cloth frames still step.
        native_collect_reading=ReadNativeDrawGate();const bool draw_gate_open=native_collect_reading.Open();
        native_collect_gate=draw_gate_open?1:2;native_draw_skipped=!draw_gate_open;   // the decision if the render step reads none
        if(draw_gate_open)PostNativeUiDisplay();   // I-0088 P1: beside the collect
        const auto pointer=[](uintptr_t address){uintptr_t value{};
            if(!Read(address,&value,8))Refuse("native_scene_update_pointer_unreadable");return value;};
        const auto copy=[](uintptr_t destination,uintptr_t source,size_t bytes){
            if(!destination || !source || !Read(source,reinterpret_cast<void*>(destination),bytes))
                Refuse("native_scene_update_cpu_copy_failed");};
        const auto call2=[](uintptr_t rva,uintptr_t first,uintptr_t second){
            reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t)>(image+rva)(first,second);};
        if(pointer(stage)!=image+0x297A2C0)Refuse("native_scene_update_stage_identity");
        static std::once_flag qualified;
        std::call_once(qualified,[]{for(const auto& pin:loaded_scene_update_pins)
            if(std::memcmp(reinterpret_cast<const void*>(image+pin.rva),pin.bytes,sizeof(pin.bytes)))
                Refuse("native_scene_update_loaded_code_identity");});
        uint32_t delta{};if(!Read(tick+8,&delta,4))Refuse("native_scene_update_tick_unreadable");
        *reinterpret_cast<uint32_t*>(stage+0x18)=delta;
        const auto camera=pointer(stage+0x38),current=pointer(stage+0x28),previous=pointer(stage+0x30);
        if(!camera || pointer(camera)!=image+0x27D9F08 || !current || !previous)
            Refuse("native_scene_update_camera_identity");
        const auto view=pointer(stage+0x340);
        uintptr_t context{};
        {
            const std::lock_guard<std::recursive_mutex> lock(scene_view_cpu_lock);
            const auto found=scene_view_cpu_owners.find(view);
            if(found==scene_view_cpu_owners.end() || active_scene_context!=found->second.context)
                Refuse("native_scene_update_view_not_owned");
            context=found->second.context;
        }
        // Complete E006A0 CPU update, preserving the genuine current game
        // camera, previous sample, CPU frustum and transition algorithms.
        // Its old renderer+41D0 camera mirror is replaced by owned camera intent.
        const auto camera_service=pointer(image+0x4789CD8);
        if(!camera_service)Refuse("native_scene_update_camera_service_missing");
        const auto source_camera=pointer(camera_service+8);
        if(!source_camera)Refuse("native_scene_update_source_camera_missing");
        copy(camera+0x50,source_camera+0x50,16);copy(camera+0x10,source_camera+0x10,64);
        // E006A0's previous sample is a single camera prefix. C1B20 also
        // writes a second history sample and is only valid for stage+20's
        // larger CPU camera owner, not this previous-sample allocation.
        copy(previous,current,0x68);copy(previous+0x6C,current+0x6C,0x18);
        scene_camera_original(reinterpret_cast<void*>(camera),reinterpret_cast<void*>(current));
        native_scene_camera_object.store(current,std::memory_order_release);   // FFX draw N5a: the pass context's camera
        *reinterpret_cast<uint32_t*>(current+0x5C)=output_width;
        *reinterpret_cast<uint32_t*>(current+0x60)=output_height;
        SceneViewInput produced;
        if(!ingress::CopyProducedSceneView(camera,current,image,serial+1,produced)){
            // 1179 witness: the camera output the native view refused (High Wall load, Argo).
            std::array<float,16> pose{};std::array<uint32_t,9> lens{};Read(current,pose.data(),64);Read(current+0x50,lens.data(),sizeof(lens));
            char row[640]{};sprintf_s(row,"{\"event\":\"native_scene_camera_refused\",\"pose_w\":[%g,%g,%g,%g],\"position\":[%g,%g,%g],\"lens_words\":[%u,%u,%u,%u,%u,%u,%u,%u,%u]}\n",
                pose[3],pose[7],pose[11],pose[15],pose[12],pose[13],pose[14],lens[0],lens[1],lens[2],lens[3],lens[4],lens[5],lens[6],lens[7],lens[8]);Log(row);
            Refuse("native_scene_update_camera_projection_unsupported");}
        ApplyUpscalerJitter(produced);   // I-0111
        ingress::SceneCameraConstants constants;
        if(!ingress::ProduceCameraConstants(produced,constants))Refuse("native_scene_camera_existing_producer_failed");
        {
            const std::lock_guard<std::recursive_mutex> lock(scene_view_cpu_lock);native_scene_camera=produced;
            {std::array<uint8_t,0x88> bytes{};if(Read(current,bytes.data(),bytes.size()))native_scene_camera_bytes=bytes;else native_scene_camera_bytes.reset();}
            native_scene_camera_constants=constants;
        }
        call2(0xC1B20,pointer(stage+0x20),current);
        const auto ui=pointer(image+0x4799C98);
        if(ui)call2(0xE6ECB0,ui,camera+0x10);
        // Original85 loaded source proves this notification is RET 0.
        const auto notification=pointer(image+0x476A288);
        if(notification)reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t)>(image+0x7D9C50)
            (notification,camera,image+0x27D9F48);
        call2(0x06E9E0,stage+0x60,current);
        std::array<uint32_t,2> screen_request{};
        reinterpret_cast<void(__fastcall*)(void*)>(image+0x06E750)(screen_request.data());
        if(!Read(current+0x60,&screen_request[0],4) || !Read(current+0x50,&screen_request[1],4))
            Refuse("native_scene_update_screen_request_unreadable");
        // D-0222 (option A): this call goes through DS3's entry 6E770, which KeepLodReference intercepts so the reference global keeps its
        // launch pair; keep it a call of that entry, never a direct write of the global.
        reinterpret_cast<void(__fastcall*)(void*)>(image+0x06E770)(screen_request.data());
        const auto decals=pointer(stage+0x560);if(decals)call2(0xCFA750,decals,tick);
        call2(0xD52AF0,stage+0x348,tick);
        // 1052 witness (tone map keys, lighting map): 8FBD20 sets stage+508 from
        // the draw-param manager [4791248]+8 (+154 bit1, holder +158); D52B70
        // applies a holder only when its tokens +190/+194 equal [46402E0/E4].
        {static LONG64 holder_witness{};const auto n=InterlockedIncrement64(&holder_witness);
            if(n<=2 || (n%120==0 && n<=1800)){uintptr_t manager{},area{},area_holder{},set508{},set500{};uint32_t flags{};
                std::array<uint32_t,2> tokens{},expected{};uintptr_t chosen{};
                Read(image+0x4791248,&manager,8);if(manager)Read(manager+8,&area,8);
                if(area){Read(area+0x154,&flags,4);Read(area+0x158,&area_holder,8);}
                Read(stage+0x508,&set508,8);Read(stage+0x500,&set500,8);chosen=set508?set508:set500?set500:stage+0x348;
                Read(chosen+0x190,tokens.data(),8);Read(image+0x46402E0,expected.data(),8);
                // Holder category 1 (ToneMap, D52B70): keys +30/+38.
                std::array<uint32_t,4> area_tone{};if(area_holder){Read(area_holder+0x30,&area_tone[0],8);Read(area_holder+0x38,&area_tone[2],8);}
                char row[480]{};sprintf_s(row,"{\"event\":\"native_drawparam_holder\",\"manager\":%s,\"area\":%s,\"flags\":%u,\"area_holder\":%s,\"stage508\":%s,\"stage500\":%s,\"tokens\":[%u,%u],\"expected\":[%u,%u],\"area_tonemap\":[%u,%u,%u,%u]}\n",
                    manager?"true":"false",area?"true":"false",flags,area_holder?"true":"false",set508?"true":"false",set500?"true":"false",tokens[0],tokens[1],expected[0],expected[1],
                    area_tone[0],area_tone[1],area_tone[2],area_tone[3]);Log(row);}}
        auto transitions=pointer(stage+0x508);
        if(!transitions)transitions=pointer(stage+0x500);
        if(!transitions)transitions=stage+0x348;
        call2(0xD52B70,transitions,context);
        *reinterpret_cast<uintptr_t*>(stage+0x508)=0;
        *reinterpret_cast<uintptr_t*>(stage+0x500)=0;
        BYTE reset{};if(!Read(stage+0x510,&reset,1))Refuse("native_scene_update_transition_reset_unreadable");
        if(reset){
            const std::array<uint32_t,2> empty_target{0,0xFFFFFFFF};
            reinterpret_cast<void(__fastcall*)(uintptr_t,int,const void*)>(image+0x0884F0)(context,2,empty_target.data());
            *reinterpret_cast<BYTE*>(stage+0x510)=0;
        }
        std::array<float,3> neutral{},tint{};float one{},amount{};BYTE tint_enabled{};
        if(!Read(image+0x3D7FEF0,neutral.data(),sizeof(neutral)) || !Read(image+0x3D7EE10,&one,4) ||
           !Read(stage+0x4F8,&tint_enabled,1) || !Read(stage+0x4FC,&amount,4) || !std::isfinite(amount))
            Refuse("native_scene_update_tint_unreadable");
        float blend{};tint=neutral;
        if(tint_enabled && amount>0){blend=amount<one?amount:one;
            if(!Read(image+0x39F2490,tint.data(),sizeof(tint)))Refuse("native_scene_update_tint_source_unreadable");}
        *reinterpret_cast<float*>(context+0x3928)=blend;*reinterpret_cast<uint32_t*>(context+0x392C)=0;
        std::memcpy(reinterpret_cast<void*>(context+0x3930),tint.data(),sizeof(tint));
        std::memcpy(reinterpret_cast<void*>(context+0x393C),neutral.data(),sizeof(neutral));
        std::memcpy(reinterpret_cast<void*>(context+0x3948),neutral.data(),sizeof(neutral));
        *reinterpret_cast<BYTE*>(stage+0x4F8)=0;*reinterpret_cast<float*>(stage+0x4FC)=0;
        const auto layer=pointer(stage+0x568);
        if(layer){
            if(pointer(layer)!=image+0x295A0C8)Refuse("native_scene_update_layer_identity");
            copy(layer+0xF0,camera+0x10,64);
            const auto target=pointer(pointer(layer)+0xA8);
            if(target!=image+0xD5E550)Refuse("native_scene_update_layer_method_identity");
            reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t)>(target)(layer,tick);
        }
        const auto debug_requests=pointer(stage+0x578);
        if(debug_requests)UpdateNativeSceneDebugRequests(debug_requests);
        const auto auxiliary=pointer(image+0x478CBA0);
        if(auxiliary)UpdateNativeSceneAuxiliaryView(auxiliary,current);
        uintptr_t selection_heap{};
        {const std::lock_guard<std::recursive_mutex> lock(scene_view_cpu_lock);
            selection_heap=scene_view_cpu_owners.at(view).heap;}
        if(draw_gate_open){LARGE_INTEGER t0{},t1{};QueryPerformanceCounter(&t0);
        {const kprofile::Scope k(kprofile::SceneSetup);CollectNativeSelectedScene(view,stage+0x60,selection_heap,produced,constants);}
        {const kprofile::Scope k(kprofile::Offscreen);CollectNativeOffscreenScenes(produced.serial);}   // 1169: armed menu views, same engine frame point
        QueryPerformanceCounter(&t1);native_phase_ticks[0]+=t1.QuadPart-t0.QuadPart;++native_phase_counts[0];}
        static LONG64 updates{};if(InterlockedIncrement64(&updates)<=4)
            Log("{\"event\":\"native_scene_cpu_updated\",\"genuine_camera\":true,\"existing_native_camera_producer\":true,\"cpu_frustum_and_transitions\":true,\"legacy_renderer_mirror\":false,\"scene_draw_submitted\":false}\n");
    }catch(...){Refuse("native_scene_cpu_update_failed");}
}
[[noreturn]] void __fastcall UnsupportedModelStorage() noexcept {
    char row[160]{};sprintf_s(row,"{\"event\":\"native_model_method_unowned\",\"caller_rva\":%llu}\n",
        static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(_ReturnAddress())-image));Log(row);
    Refuse("native_model_storage_method_not_owned");
}
uintptr_t __fastcall DestroyModelStorage(uintptr_t receiver,uint32_t flags) noexcept {
    census::NoteRetirement(census::StorageDestroy,_ReturnAddress());   // R13 cR0
    try {
        uintptr_t heap{},old_handles[2]{};int32_t references{};
        if((flags&~1u) || !Read(receiver+8,&heap,8) || !heap ||
           !Read(receiver+0x10,&references,4) || references ||
           !Read(receiver+0x18,old_handles,sizeof(old_handles)) || old_handles[0] || old_handles[1])
            Refuse("native_model_storage_retirement_contract");
        {std::lock_guard<std::shared_mutex> hold(model_storage_lock);
            const auto found=model_storage.find(receiver);
            if(found==model_storage.end())Refuse("native_model_storage_foreign_retirement");
            // B6 (R3): kept until the queue completes what was submitted until now, without a wait.
            if(found->second->resource)RetireWhenSubmittedWorkCompletes(found->second);
            {const std::lock_guard census(model_storage_withdrawn_lock);model_storage_withdrawn.push_back(found->second);}
            model_storage.erase(found);model_geometry_refusal.erase(receiver);BumpModelRegistry();}   // I-0120 A9: a reused address takes no old refusal
        if(flags&1u)FreeModelPrefix(receiver,heap);
        return receiver;
    }catch(...){Refuse("native_model_storage_retirement_failed");}
}
const std::array<uintptr_t,64>& ModelStorageTable() {
    static const auto table=[] {std::array<uintptr_t,64> result{};
        result.fill(reinterpret_cast<uintptr_t>(&UnsupportedModelStorage));
        result[0]=reinterpret_cast<uintptr_t>(&DestroyModelStorage);return result;}();
    return table;
}
void __fastcall RetireModelStorage(void* model) noexcept {
    census::NoteRetirement(census::StorageRetire,_ReturnAddress());   // R13 cR0
    const auto address=reinterpret_cast<uintptr_t>(model);uintptr_t receiver{},heap{};
    if(!Read(address+0xE8,&receiver,8))Refuse("native_model_storage_model_unreadable");
    if(!receiver)return;
    {const std::shared_lock hold(model_storage_lock);   // I-0081: a lookup, shared
        if(!model_storage.contains(receiver))Refuse("native_model_storage_foreign_reference");}
    if(!Read(receiver+8,&heap,8) || !heap)Refuse("native_model_storage_heap_missing");
    // Withdraw the model reference first. CPU vertex/index views retain theirs
    // through the game's real 17A35D0/17A3610 shared-reference operations.
    *reinterpret_cast<uintptr_t*>(address+0xE8)=0;
    if(reinterpret_cast<int(__fastcall*)(uintptr_t)>(image+0x17A3610)(receiver+0x10)==1) {
        {const census::Internal internal;DestroyModelStorage(receiver,0);}
        try{FreeModelPrefix(receiver,heap);}catch(...){Refuse("native_model_storage_heap_failed");}
    }
}
Microsoft::WRL::ComPtr<ID3D12Resource> PublishNativeStreamBytes(const std::vector<BYTE>& bytes,std::shared_ptr<const UploadVersion>& version) {
    if(bytes.empty() || !gpu.device)throw std::runtime_error("native model buffer input");
    version=stream_versions.Publish(gpu.device.Get(),bytes.data(),bytes.size());return version->resource;
}
// D3D12 review R4 (B4): static FLVER storage in DEFAULT memory, filled from a staging range by a copy on the ingress
// context (ahead of every later submission) and left as a vertex and index source. The staging range goes back with the
// ingress list; the CPU bytes stay with the storage (palette validation reads them). I-0079: the buffer is placed in a pooled
// buffer heap and staged through a pooled upload chunk (DeviceQueues::CreateStreamed and AllocateStaging): two committed
// allocations a buffer, each waiting on the paging fence on DS3's loading threads, before.
Microsoft::WRL::ComPtr<ID3D12Resource> AdmitNativeStaticBuffer(std::span<const BYTE> bytes,bool rt_readable) {
    if(bytes.empty() || !gpu.device)throw std::runtime_error("native model buffer input");
    const uint64_t aligned=(uint64_t(bytes.size())+3)&~uint64_t(3);
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=aligned;
    desc.Height=1;desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    const auto resource=gpu.CreateStreamed(desc,D3D12_RESOURCE_STATE_COMMON,ds3native::PlacedKind::Buffer);
    auto staging=gpu.AllocateStaging(aligned,4);
    std::memcpy(staging.cpu,bytes.data(),bytes.size());
    if(aligned>bytes.size())std::memset(staging.cpu+bytes.size(),0,static_cast<size_t>(aligned-bytes.size()));
    auto recording=gpu.RecordIngress();auto* list=recording.List();
    list->CopyBufferRegion(resource.Get(),0,staging.resource,staging.offset,aligned);   // COMMON promotes to COPY_DEST
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    // I-0117 (R2 T0): with the rays' scene the buffer is also left readable by the BLAS builds (NON_PIXEL_SHADER_RESOURCE, a read
    // state combined with the vertex and index ones); I-0120 F2: and by the traced shadow's pixel shader (rt_shadow.hlsl through
    // rt_material.hlsli reads the storage's buffer), PIXEL_SHADER_RESOURCE.
    const auto read=D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER|D3D12_RESOURCE_STATE_INDEX_BUFFER|
        (rt_readable?D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:D3D12_RESOURCE_STATE_COMMON);
    barrier.Transition={resource.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_COPY_DEST,static_cast<D3D12_RESOURCE_STATES>(read)};
    list->ResourceBarrier(1,&barrier);
    recording.Keep(std::move(staging.lease));
    recording.Keep(std::make_shared<Microsoft::WRL::ComPtr<ID3D12Resource>>(resource));
    (void)recording.Commit();return resource;
}
// I-0074: an engine buffer is read in place (the engine holds it for the call). I-0097 tranche 4: under an SEH guard instead of
// checking its extent first with VirtualQuery: on the replayed path that check was 59 % of the texture admissions inside DS3's
// loading step, which the engine thread joins (i97-t4-prof-1: 586 of 986 samples; the native step 2,549 samples against the
// original's 1,371, i97-t4-orig-prof-1). `body` reads the buffer [begin, begin+size) and keeps its own C++ failures; a read
// fault inside that range while it runs (unreadable, uncommitted, guarded, a failed page-in) returns false, as the extent
// check did; any other fault (one of the module's own, final review 1) is left to the fault handler and its dump. Its C++
// objects are not unwound then: the caller refuses the run.
struct GuardedEngineRange {uintptr_t begin{},end{};};
thread_local GuardedEngineRange guarded_engine_range;
bool EngineReadFault(const EXCEPTION_RECORD& r) noexcept {
    const auto& range=guarded_engine_range;
    if(!range.end || (r.ExceptionCode!=EXCEPTION_ACCESS_VIOLATION && r.ExceptionCode!=STATUS_GUARD_PAGE_VIOLATION &&
       r.ExceptionCode!=EXCEPTION_IN_PAGE_ERROR) || r.NumberParameters<2 || r.ExceptionInformation[0]!=0)return false;   // 0: a read
    const auto address=static_cast<uintptr_t>(r.ExceptionInformation[1]);return address>=range.begin && address<range.end;
}
__declspec(noinline) bool GuardedEngineRead(void(*body)(void*),void* context,uintptr_t begin,size_t size) noexcept {
    const auto outer=guarded_engine_range;guarded_engine_range={begin,size>UINTPTR_MAX-begin?UINTPTR_MAX:begin+size};bool ok=true;
    __try{body(context);}
    __except(EngineReadFault(*GetExceptionInformation()->ExceptionRecord)?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){ok=false;}
    guarded_engine_range=outer;return ok;
}
// The encoded texture from the engine's bytes, in two steps under GuardedEngineRead. Locate: the direct DDS (its extent from its
// header) or the container's entry (validated whole; a DCX container is inflated into `expanded`), then its content key (D-0211:
// the hash of its DDS bytes and of its float metadata, their size, its name, family, format tag and mip count). Decode, only when
// nothing is kept under the key: its subresources copied out. A failure of either step itself is kept in `error`; `fatal` when it
// is not a refusal of the data (no memory, an exception that is not the decoder's): the admission then refuses, not skips (A3).
struct NativeTextureDecode {uintptr_t data{};size_t size{};uint32_t index{};bool direct{},cube{};std::array<uint8_t,128> header{};
    std::vector<uint8_t> expanded;std::optional<ds3content::CpuContainerEntryView> entry;bootstrap::TextureIngress::ContentKey key;
    ds3content::CpuTextureResource decoded;std::string container_name,error;bool missing_entry{},fatal{};};
void LocateNativeTextureBody(void* context) noexcept {
    auto& d=*static_cast<NativeTextureDecode*>(context);
    try{if(d.direct){
            ds3content::CpuContainerEntryView input;input.name="engine-direct-dds";input.type=d.cube?1u:0u;
            input.dds={reinterpret_cast<const uint8_t*>(d.data),d.size};d.entry=std::move(input);}
        else{d.entry=ds3content::ViewCpuTextureContainerEntry({reinterpret_cast<const uint8_t*>(d.data),d.size},d.index,d.expanded);
            if(!d.entry){d.missing_entry=true;return;}
            d.container_name=d.entry->name;}
        const auto& e=*d.entry;uint64_t hash=ds3content::ContentHash64(e.dds);
        if(e.float_metadata){const auto& words=e.float_metadata->words;
            hash=ds3content::ContentHash64({reinterpret_cast<const uint8_t*>(words.data()),words.size()*4},hash^(uint64_t(e.float_metadata->tag)<<1|1));}
        d.key={hash,e.dds.size(),e.name,uint32_t(d.direct)|uint32_t(d.cube)<<1|uint32_t(e.type)<<2|uint32_t(e.format_tag)<<8|uint32_t(e.mip_count)<<16};
    }catch(const std::bad_alloc&){d.error="locate: no memory";d.fatal=true;}
    catch(const std::exception& e){d.error=e.what();if(d.error.empty())d.error="locate";}
    catch(...){d.error="locate";d.fatal=true;}
}
void DecodeNativeTextureBody(void* context) noexcept {
    auto& d=*static_cast<NativeTextureDecode*>(context);
    try{d.decoded=ds3content::DecodeNativeTextureDDS(*d.entry);}
    catch(const std::bad_alloc&){d.error="decode: no memory";d.fatal=true;}
    catch(const std::exception& e){d.error=e.what();if(d.error.empty())d.error="decode";}
    catch(...){d.error="decode";d.fatal=true;}
}
// I-0074: a thread other than the engine thread met at a texture or model admission joins the sampler (sample-profile runs).
void SampleLoaderThread() noexcept {
    if(!native_sample_loaders.load(std::memory_order_relaxed))return;
    thread_local bool added{};if(added)return;added=true;
    if(GetCurrentThreadId()!=owner_thread)(void)sampling::AddThread(GetCurrentThreadId(),"loader");
}
// I-0077: a pool worker (WorkPool::worker_hook, set when the sampler starts) joins the sampler once under its pool's name.
void SamplePoolThread(const char* name) noexcept {
    thread_local bool added{};if(added)return;added=true;
    (void)sampling::AddThread(GetCurrentThreadId(),name);
}
void __fastcall AdmitModelStorage(void* model,uintptr_t file) noexcept {
    census::Note(census::StorageAdmit);   // R13 cR0
    SampleLoaderThread();
    uintptr_t receiver{},heap{};
    try {
        {const census::Internal internal;RetireModelStorage(model);}
        uintptr_t header{},source{};uint32_t bytes{};
        if(!Read(file,&header,8) || !Read(header+0x10,&bytes,4))throw std::runtime_error("FLVER storage header");
        if(!bytes)return;
        if(bytes>128u*1024u*1024u || !Read(file+0x58,&source,8) || !source ||
           bytes>UINTPTR_MAX-source)throw std::runtime_error("FLVER storage source extent");
        auto storage=std::make_shared<NativeModelStorage>();storage->bytes.resize(bytes);
        // Actual 1FA030 index-table count comes from the serialized FLVER
        // header+50, not an inferred adjacent engine vector field.
        if(!Read(header+0x50,&storage->index_buffer_count,4))throw std::runtime_error("FLVER index table count");
        if(!Read(source,storage->bytes.data(),bytes))throw std::runtime_error("FLVER CPU storage unreadable");
        const uint64_t aligned=(uint64_t(bytes)+3)&~uint64_t(3);
        if(!Read(image+0x479C258,&heap,8) || !heap)throw std::runtime_error("FLVER CPU heap missing");
        receiver=reinterpret_cast<uintptr_t(__fastcall*)(size_t,size_t,uintptr_t)>(image+0x1777A00)(0x28,8,heap);
        if(!receiver)throw std::bad_alloc();
        std::memset(reinterpret_cast<void*>(receiver),0,0x28);
        // 1934260 is only CPU allocator/refcount construction. Never call
        // 1932410 with old GPU payload or the mixed 1926720 buffer factory.
        reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uintptr_t)>(image+0x1934260)(receiver,heap);
        *reinterpret_cast<uintptr_t*>(receiver)=reinterpret_cast<uintptr_t>(ModelStorageTable().data());
        // B4: no frame lock, the copy is on the ingress context. I-0081: the buffer is created and its copy recorded with no
        // registry lock held; only the insertion is exclusive, so the collect's lookups never wait on a creation. I-0097: the
        // storage is registered at once, without its buffer, and the buffer is created on the storage queue (StorageQueue), not
        // on DS3's loading task; the storage draws nothing until its geometry job has published its meshes, after the buffer.
        // The admission is counted in model_storage_admitting from the closed check until its queued creation finishes, and
        // Close waits for the count before its drain; one that finds the registry closed at its insertion is dropped.
        // I-0120 A7's sibling (I-0137): an admission after the close (a test run's or the quit's late loads) is dropped as the
        // insertion's own closed path drops it, its prefix freed, instead of a refusal.
        bool closed_now=false;
        {const std::shared_lock hold(model_storage_lock);
            if(model_storage_closed || !gpu.device)closed_now=true;
            else model_storage_admitting.fetch_add(1,std::memory_order_acq_rel);}
        if(closed_now){FreeModelPrefix(receiver,heap);Log("{\"event\":\"native_model_storage_dropped\",\"reason\":\"closed\"}\n");return;}
        bool dropped=false;
        {std::lock_guard<std::shared_mutex> hold(model_storage_lock);
            if(model_storage_closed)dropped=true;
            else{storage->generation=++model_storage_generation;model_storage.emplace(receiver,storage);BumpModelRegistry();}}
        if(dropped){model_storage_admitting.fetch_sub(1,std::memory_order_acq_rel);FreeModelPrefix(receiver,heap);Log("{\"event\":\"native_model_storage_dropped\",\"reason\":\"closed\"}\n");return;}
        // Step review: the job's storage reference is released before its count (a Close that sees the count at 0 finds no
        // buffer still held by the queue thread); a storage withdrawn or a registry closed before the job runs gets no buffer.
        StorageQueue().Post([storage,receiver]() mutable {
            struct Done {~Done(){model_storage_admitting.fetch_sub(1,std::memory_order_acq_rel);}} const done;
            const auto held=std::move(storage);
            try{{const std::shared_lock hold(model_storage_lock);const auto found=model_storage.find(receiver);
                    if(model_storage_closed || found==model_storage.end() || found->second!=held)return;}
                const bool rt_readable=native_rt_scene.load(std::memory_order_relaxed);auto resource=AdmitNativeStaticBuffer(held->bytes,rt_readable);
                const std::lock_guard<std::shared_mutex> hold(model_storage_lock);held->resource=std::move(resource);held->rt_readable=rt_readable;}
            catch(const std::exception& error){char row[320]{};sprintf_s(row,"{\"event\":\"native_model_storage_error\",\"reason\":\"%s\"}\n",error.what());Log(row);
                Refuse("native_model_storage_admission_failed");}
            catch(...){Refuse("native_model_storage_admission_failed");}});
        // Transfer the constructor's single reference to the model; genuine
        // 1FA030/1FAD80 CPU consumers add their own references and view offsets.
        *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(model)+0xE8)=receiver;
        char row[256]{};sprintf_s(row,"{\"event\":\"native_model_storage_admitted\",\"generation\":%llu,\"bytes\":%u,\"aligned_bytes\":%llu,\"original_gpu_slots_nil\":true,\"drawn\":false}\n",storage->generation,bytes,aligned);Log(row);
    }catch(const std::exception& error){
        if(receiver){try{FreeModelPrefix(receiver,heap);}catch(...){}}
        char row[320]{};sprintf_s(row,"{\"event\":\"native_model_storage_error\",\"reason\":\"%s\"}\n",error.what());Log(row);
        Refuse("native_model_storage_admission_failed");
    }
}
uint64_t __fastcall AdmitModelCpuGeometry(uintptr_t model,uintptr_t asset,uintptr_t material_inputs,BYTE cpu_only,uint32_t flags) noexcept {
    census::Note(census::CpuGeometry);   // R13 cR0
    SampleLoaderThread();
    // The genuine CPU load already runs in native mode with its buffer and
    // material responsibilities replaced below. Join only after it has built
    // the real vertex/index CPU views and completed its mesh associations.
    const auto result=model_cpu_load_original(model,asset,material_inputs,cpu_only,flags);
    if(!(result&0xFF) || cpu_only)return result;
    try {
        uintptr_t wrapper{};if(!Read(model+0xE8,&wrapper,8) || !wrapper)return result;
        std::shared_ptr<NativeModelStorage> storage;
        {const std::shared_lock lock(model_storage_lock);   // I-0081: a lookup, shared
            const auto found=model_storage.find(wrapper);if(found==model_storage.end())return result;storage=found->second;}
        std::vector<MeshSlice> meshes;int refusal{};
        if(!ingress::CopyModelGeometry(model,image,wrapper,storage->index_buffer_count,storage->bytes.size(),meshes,&refusal)){
            // 1058 witness: which CopyOnce check refused a model's geometry (histogram).
            static std::mutex refusal_lock;static std::array<uint32_t,20> refusals{};static LONG64 refused{};
            {const std::lock_guard<std::shared_mutex> hold(model_storage_lock);model_geometry_refusal[wrapper]=refusal;}
            {const std::lock_guard hold(refusal_lock);++refusals[static_cast<size_t>(std::clamp(refusal&0xFF,0,19))];}
            const auto n=InterlockedIncrement64(&refused);
            if(n<=2 || n%25==0){std::string row="{\"event\":\"native_model_geometry_refusals\",\"total\":"+std::to_string(n)+",\"by_check\":[";
                {const std::lock_guard hold(refusal_lock);for(size_t i=0;i<refusals.size();++i)row+=(i?",":"")+std::to_string(refusals[i]);}
                row+="]}\n";Log(row.c_str());}
            return result;}
        // I-0097: the slices (read from the engine's CPU views above, inside this call) are checked where the storage holds its
        // bytes, and published, on the storage queue after the storage's buffer (StorageQueue), not on DS3's loading task.
        // geometry_pending marks the wait (the collect's missing-geometry names skip it); the post-to-publish delay feeds the
        // census row's storage_queue_max_ms.
        model_storage_admitting.fetch_add(1,std::memory_order_acq_rel);storage->geometry_pending.store(true,std::memory_order_release);
        LARGE_INTEGER posted{};QueryPerformanceCounter(&posted);
        StorageQueue().Post([storage,wrapper,meshes=std::move(meshes),posted]() mutable {
            struct Done {~Done(){model_storage_admitting.fetch_sub(1,std::memory_order_acq_rel);}} const done;
            const auto held=std::move(storage);
            struct Settled {const std::shared_ptr<NativeModelStorage>& s;~Settled(){s->geometry_pending.store(false,std::memory_order_release);}} const settled{held};
            try{const char* dropped=nullptr;size_t published{};
                {const std::shared_lock hold(model_storage_lock);if(model_storage_closed)return;}   // final review: Close waits for the queue
                if(!ValidContent(held->bytes,meshes))dropped="invalid_content";
                else{const std::lock_guard<std::shared_mutex> lock(model_storage_lock);
                    const auto found=model_storage.find(wrapper);
                    if(model_storage_closed || found==model_storage.end() || found->second!=held)dropped="withdrawn";
                    else if(!held->resource)dropped="no_buffer";
                    else{held->meshes=std::move(meshes);published=held->meshes.size();}}
                if(dropped){char row[160]{};sprintf_s(row,"{\"event\":\"native_model_cpu_geometry_dropped\",\"storage_generation\":%llu,\"reason\":\"%s\"}\n",held->generation,dropped);Log(row);return;}
                // I-0117 (R2 T0): the model's BLAS from its buffer, recorded after the buffer's upload (both on this queue); a failure
                // leaves the model out of the rays and is named (the raster is untouched).
                if(held->rt_readable){
                    try{auto blas=native_rt_blas.Build(gpu,held->resource.Get(),held->meshes);
                        const std::lock_guard<std::shared_mutex> lock(model_storage_lock);held->blas=std::move(blas);}
                    catch(const std::exception& error){char row[256]{};sprintf_s(row,"{\"event\":\"native_rt_blas_failed\",\"storage_generation\":%llu,\"reason\":\"%s\"}\n",held->generation,error.what());Log(row);}}
                LARGE_INTEGER now{},frequency{};QueryPerformanceCounter(&now);QueryPerformanceFrequency(&frequency);
                const auto delay=uint64_t(1e6*double(now.QuadPart-posted.QuadPart)/double(frequency.QuadPart));auto known=storage_queue_delay_max_us.load(std::memory_order_relaxed);
                while(delay>known && !storage_queue_delay_max_us.compare_exchange_weak(known,delay,std::memory_order_relaxed)){}
                char row[320]{};sprintf_s(row,"{\"event\":\"native_model_cpu_geometry_joined\",\"storage_generation\":%llu,\"meshes\":%zu,\"source_bytes\":%zu,\"actual_material_ids\":true,\"selection\":\"native_lod0_r16_r32\",\"scene_instance_selected\":false,\"drawn\":false}\n",
                    held->generation,published,held->bytes.size());Log(row);}
            catch(...){Refuse("native_model_cpu_geometry_join_failed");}});
    }catch(...){Refuse("native_model_cpu_geometry_join_failed");}
    return result;
}
void __fastcall RetireTexture(void* wrapper) noexcept {
    census::NoteRetirement(census::TextureRetire,_ReturnAddress());   // R13 cR0
    const auto receiver=reinterpret_cast<uintptr_t>(wrapper);
    uintptr_t handles[3]{};
    if(!Read(receiver+0x40,handles,sizeof(handles)) || handles[0] || handles[1] || handles[2])
        Refuse("unexpected_original_texture_handle");
    textures.Retire(receiver);
    {const std::lock_guard hold(native_cube_name_lock);
        for(auto it=native_cube_names.begin();it!=native_cube_names.end();)if(it->second==receiver)it=native_cube_names.erase(it);else ++it;
        for(auto it=native_grading_names.begin();it!=native_grading_names.end();)if(it->second==receiver)it=native_grading_names.erase(it);else ++it;}
    {const std::lock_guard hold(native_texture_constant_lock);native_texture_constants.erase(receiver);}   // review: a reused address takes no retired constant
    // These are CPU metadata/reset fields from 2AD40. Native GPU resources
    // retire through the registry; no original renderer/profiler is consulted.
    *reinterpret_cast<uint32_t*>(receiver+0x58)=0;
    *reinterpret_cast<float*>(receiver+0x10)=1.0f;
}
bool __fastcall AdmitTexture(void* wrapper,uint32_t width,uint32_t height,int format,int mips,
    uintptr_t source,BYTE unordered_access,uint32_t flags,uintptr_t allocator,uintptr_t usage) noexcept {
    census::Note(census::TextureAdmit);   // R13 cR0
    if(control->stop) return false;
    SampleLoaderThread();
    char row[384]{};
    sprintf_s(row,"{\"event\":\"native_texture_request\",\"width\":%u,\"height\":%u,\"format\":%d,\"mips\":%d,\"data\":%s,\"uav\":%u,\"flags\":%u,\"usage\":%llu}\n",
        width,height,format,mips,source?"true":"false",unordered_access,flags,static_cast<unsigned long long>(usage)); Log(row);
    if(unordered_access || mips<0) Refuse("texture_asset_family_not_owned");
    try {
        {const census::Internal internal;RetireTexture(wrapper);}
        const auto receiver=reinterpret_cast<uintptr_t>(wrapper);
        const auto record=textures.Admit(receiver,{width,height,static_cast<uint32_t>(mips),static_cast<DXGI_FORMAT>(format)},source);
        if(!allocator && !Read(image+0x4961B80,&allocator,8)) Refuse("texture_cpu_allocator_unavailable");
        *reinterpret_cast<uint32_t*>(receiver+0x58)=static_cast<uint32_t>(record->content_bytes);
        *reinterpret_cast<uintptr_t*>(receiver+0x60)=allocator;
        sprintf_s(row,"{\"event\":\"native_texture_resident\",\"generation\":%llu,\"bytes\":%llu,\"fence\":%llu,\"width\":%u,\"height\":%u,\"mips\":%u,\"legacy_handle\":false}\n",
            record->generation,record->content_bytes,record->upload_fence,record->description.width,
            record->description.height,record->description.mips); Log(row);
        return true;
    } catch(const std::exception& e) {
        sprintf_s(row,"{\"event\":\"native_texture_failure\",\"message\":\"%s\"}\n",e.what()); Log(row);
        Refuse("native_texture_admission_failed");
    }
}
void AdmitEncodedTextureFamily(void* wrapper,void* source_receiver,bool cube_family,uintptr_t caller) noexcept {
    if(control->stop) return;
    SampleLoaderThread();
    struct Timed {LARGE_INTEGER start{};Timed(){QueryPerformanceCounter(&start);}
        ~Timed(){LARGE_INTEGER end{};QueryPerformanceCounter(&end);const auto ticks=uint64_t(end.QuadPart-start.QuadPart);
            native_texture_admit_ticks.fetch_add(ticks,std::memory_order_relaxed);auto known=native_texture_admit_max.load(std::memory_order_relaxed);
            while(ticks>known && !native_texture_admit_max.compare_exchange_weak(known,ticks,std::memory_order_relaxed)){}}} const timed;   // I-0074
    // I-0137 (I-0120 A3, « skip the object and go on »): a texture the game's data makes unreadable or the decoder refuses is
    // skipped as the original does it: 14002B260 and 1400AC110 retire the wrapper first and, when the source makes no texture
    // (its vtable +0x50 fails), return with the wrapper left empty. Natively: the wrapper retired, no record, a counted row;
    // what names the texture then takes the path of a texture bound with no data. A refusal stays for the module's own
    // failures (the device, the ingress, a global of the image).
    const auto skip=[&](const char* why,const std::string& detail={}){
        {const census::Internal internal;RetireTexture(wrapper);}
        static LONG64 rows{};if(!detail.empty() && InterlockedIncrement64(&rows)<=32){std::string text;
            for(const auto ch:detail)if(text.size()<160)text.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');
            char row[320]{};sprintf_s(row,"{\"event\":\"native_encoded_texture_failure\",\"skipped\":true,\"reason\":\"%s\",\"message\":\"%s\"}\n",why,text.c_str());Log(row);}
        NoteNativeSkip(why,reinterpret_cast<uintptr_t>(wrapper));};
    try {
        struct CpuSource {uintptr_t vtable,data;uint32_t size,index;uintptr_t allocator,usage;};
        CpuSource source{};
        const auto address=reinterpret_cast<uintptr_t>(source_receiver);
        bool source_read=Read(address,&source,16);
        const bool direct=source_read && source.vtable==image+0x3D1AF68;
        NativeTextureDecode decode;decode.cube=cube_family;
        if(direct){
            //004BC0/004BD0/004C10 consume only the real DDS pointer at+8.
            // This class is16 bytes: adjacent stack words are not size/index.
            if(!Read(source.data,decode.header.data(),decode.header.size())){skip("direct_dds_header_unreadable");return;}
            // I-0074: read where it lies (the engine holds it for this call); tranche 4: under the guard, no extent query first.
            decode.direct=true;decode.data=source.data;
            std::string refused;
            try{decode.size=ds3content::NativeLegacyDdsByteCount(std::vector<uint8_t>(decode.header.begin(),decode.header.end()));}
            catch(const std::bad_alloc&){throw;}   // the module's, not the data's: refused below
            catch(const std::exception& e){refused=e.what();if(refused.empty())refused="header";}
            if(!refused.empty()){skip("direct_dds_header_refused",refused);return;}
            if(!GuardedEngineRead(&LocateNativeTextureBody,&decode,decode.data,decode.size)){skip("direct_dds_payload_unreadable");return;}
            if(decode.fatal)throw std::runtime_error(decode.error);
            if(!decode.error.empty()){skip("direct_dds_refused",decode.error);return;}
        }else{
        if(source_read)source_read=Read(address,&source,sizeof(source));
        if(!source_read || source.vtable!=image+0x3D48E08 || source.size<16 || source.size>128u*1024u*1024u){
            static LONG64 unowned{};if(InterlockedIncrement64(&unowned)<=32){
            char row[320]{};sprintf_s(row,"{\"event\":\"native_encoded_source_unowned\",\"read\":%s,\"table_rva\":%llu,\"caller_rva\":%llu,\"size\":%u,\"index\":%u,\"data_present\":%s,\"cube\":%s}\n",
                source_read?"true":"false",static_cast<unsigned long long>(source.vtable-image),static_cast<unsigned long long>(caller-image),source.size,source.index,source.data?"true":"false",cube_family?"true":"false");Log(row);}
            skip("unqualified_encoded_texture_source");return;
        }
        // I-0074: the container is read where it lies (the engine holds it for this call), validated whole as before; only
        // this entry's subresources are copied. Each admission used to allocate, zero, copy and free the whole container (6 MB
        // on average, 15.6 GB a Firelink walk: i74-native-02). Tranche 4: under the guard, no extent query first.
        LARGE_INTEGER start{},end{};QueryPerformanceCounter(&start);   // the counters as before G4: the view and the decode, containers only
        decode.data=source.data;decode.size=source.size;decode.index=source.index;
        if(!GuardedEngineRead(&LocateNativeTextureBody,&decode,decode.data,decode.size)){skip("encoded_texture_bytes_unavailable");return;}
        if(decode.missing_entry){skip("encoded_texture_index_unavailable");return;}
        if(decode.fatal)throw std::runtime_error(decode.error);
        if(!decode.error.empty()){skip("encoded_texture_container_refused",decode.error);return;}
        QueryPerformanceCounter(&end);native_texture_decode_ticks.fetch_add(uint64_t(end.QuadPart-start.QuadPart),std::memory_order_relaxed);
        native_texture_decode_bytes.fetch_add(source.size,std::memory_order_relaxed);native_texture_decodes.fetch_add(1,std::memory_order_relaxed);
        if(GetCurrentThreadId()==owner_thread)native_texture_decodes_owner.fetch_add(1,std::memory_order_relaxed);
        }
        // I-0097 tranche 4 (D-0211): the same content prepared before (DS3 reloading a map part) is found by its key (FindKept):
        // the receiver gets a resident record at once, sharing the kept resource (AdmitKept, its family in the key); otherwise
        // it is decoded and uploaded as before, and kept. The former record is retired only then, as before (step review L2).
        const auto receiver=reinterpret_cast<uintptr_t>(wrapper);
        const auto kept=textures.FindKept(decode.key);
        if(!kept){LARGE_INTEGER start{},end{};QueryPerformanceCounter(&start);
            if(!GuardedEngineRead(&DecodeNativeTextureBody,&decode,decode.data,decode.size)){skip(direct?"direct_dds_payload_unreadable":"encoded_texture_bytes_unavailable");return;}
            if(decode.fatal)throw std::runtime_error(decode.error);
            if(!decode.error.empty()){skip("encoded_texture_decode_refused",decode.error);return;}
            if(!direct){QueryPerformanceCounter(&end);native_texture_decode_ticks.fetch_add(uint64_t(end.QuadPart-start.QuadPart),std::memory_order_relaxed);}
            if(decode.decoded.cube!=cube_family){skip("encoded_texture_wrapper_family_mismatch");return;}
            // What the ingress would refuse of the decoded image (TextureIngress::AdmitOwnedKept's first check) is the data's too.
            if(!ds3content::ValidCpuTextureResource(decode.decoded)){skip("encoded_texture_resource_invalid");return;}}
        {const census::Internal internal;RetireTexture(wrapper);}
        // D-0182 (I-0135, the coordinator's option a): the Bandai Namco logo's first mip kept once, chosen by its TPF entry name,
        // with the resource it is uploaded to (dark_logo_kept.h); the UI session makes the page's logo for black from it.
        std::vector<uint8_t> dark_logo_bytes;
        if(!kept && !cube_family && decode.container_name=="MENU_BNE_Logo" && decode.decoded.format==DXGI_FORMAT_BC1_UNORM &&
           decode.decoded.width==768 && decode.decoded.height==768 && !decode.decoded.subresources.empty()){   // final review M1
            auto& keep=frame::DarkLogoKept();const std::lock_guard hold(keep.lock);
            if(!keep.closed)dark_logo_bytes=decode.decoded.subresources.front().bytes;}
        const auto record=kept?textures.AdmitKept(receiver,decode.key,*kept):
            textures.AdmitOwned(receiver,std::move(decode.decoded),decode.key);   // I-0074: moved into the record, not copied
        const auto& texture=*record->content;
        if(!dark_logo_bytes.empty()){auto& keep=frame::DarkLogoKept();const std::lock_guard hold(keep.lock);
            if(!keep.closed){   // step review L1: released meanwhile, nothing kept
            keep.resource=record->resource;keep.bc1=std::move(dark_logo_bytes);keep.width=texture.width;keep.height=texture.height;
            char row[160]{};sprintf_s(row,"{\"event\":\"native_ui_dark_logo_kept\",\"width\":%u,\"height\":%u,\"format\":%u,\"bytes\":%zu}\n",
                texture.width,texture.height,unsigned(texture.format),keep.bc1.size());Log(row);}}
        // 1148: cube textures by their TPF entry name (the Envmap TPF's "EnvMap_0304" ...), standing in for the
        // provider [[GXRS+42A8]+250] that 14006CED0 asks by name and that is absent natively.
        if(cube_family && !decode.container_name.empty()){const std::lock_guard hold(native_cube_name_lock);native_cube_names.insert_or_assign(decode.container_name,receiver);native_cube_names_generation.fetch_add(1,std::memory_order_release);}
        // W10 GR: the grading LUT strips by their entry name (the map's mAA_cgrading.tpf).
        if(!cube_family && decode.container_name.find("_CGrading")!=std::string::npos){const std::lock_guard hold(native_cube_name_lock);native_grading_names.insert_or_assign(decode.container_name,receiver);}
        // Genuine authored CPU metadata stays in the game. No old texture
        // object, SRV or profiler is created by this entry.
        const auto format=static_cast<uint32_t>(texture.format);
        // 147090/147110 read the texture's authored constant (CPU) and then
        // 1471E0 creates an old-device constant buffer from the display 4961AB0
        // (absent natively). Native keeps their CPU part: 147090's +10 store,
        // and the constant itself in native_texture_constants; the old buffer
        // at +30 is not created (tex975/976).
        std::optional<std::array<float,4>> constant;
        if(!direct && ((format>=70 && format<=72) || (format>=76 && format<=78))) {
            using Count=uint32_t(__fastcall*)(void*);
            const auto count=reinterpret_cast<Count>(image+0xAC7F0)(source_receiver);
            if(count) {
                // A3: an authored constant that cannot be read leaves the texture without one (RetireTexture's 1.0 at +0x10), counted.
                uintptr_t table{},method{},value{};int32_t size{};
                if(!Read(reinterpret_cast<uintptr_t>(source_receiver),&table,8) || !Read(table+0x30,&method,8) || !method)
                    NoteNativeSkip("texture_constant_source_unreadable",receiver);
                else{
                reinterpret_cast<void(__fastcall*)(void*,int32_t,uintptr_t*,int32_t*)>(method)(source_receiver,static_cast<int32_t>(count)-1,&value,&size);
                if(size==4){uint32_t bits{};
                    if(!Read(value,&bits,4))NoteNativeSkip("texture_constant_value_unreadable",receiver);
                    else{*reinterpret_cast<uint32_t*>(receiver+0x10)=bits;float v{};std::memcpy(&v,&bits,4);constant=std::array<float,4>{v,v,v,1.0f};}}}
            }
        } else if(!direct && !cube_family && source.data) {
            using EntryCount=int32_t(__fastcall*)(uintptr_t,uint32_t);
            using Entry=uintptr_t(__fastcall*)(uintptr_t,uint32_t,int32_t,void*,int32_t*);
            const auto entries=reinterpret_cast<EntryCount>(image+0x2139C00)(source.data,source.index);
            for(int32_t n=0;n<entries;++n){std::array<BYTE,8> scratch{};int32_t tag{};
                const auto value=reinterpret_cast<Entry>(image+0x2139C90)(source.data,source.index,n,scratch.data(),&tag);
                if(tag!=0x3FC)continue;
                std::array<float,4> v{0,0,0,1.0f};if(!Read(value,v.data(),12)){NoteNativeSkip("texture_constant_entry_unreadable",receiver);break;}
                constant=v;break;}
        }
        if(!constant){const std::lock_guard hold(native_texture_constant_lock);native_texture_constants.erase(receiver);}
        if(constant){const std::lock_guard hold(native_texture_constant_lock);native_texture_constants.insert_or_assign(receiver,*constant);
            static LONG64 reported{};if(InterlockedIncrement64(&reported)<=4)
                Log("{\"event\":\"native_texture_constant_cpu\",\"legacy_constant_buffer\":false}\n");}
        *reinterpret_cast<uint32_t*>(receiver+0x58)=static_cast<uint32_t>(record->content_bytes);
        // The 2D class is 0x70 bytes; the cube class is only 0x60. Its native
        // resource lifetime is external, never written beyond the CPU object.
        if(!cube_family){if(direct && !Read(image+0x4961B80,&source.allocator,8))Refuse("direct_dds_cpu_allocator_unreadable");
            *reinterpret_cast<uintptr_t*>(receiver+0x60)=source.allocator;}
        char row[384]{};
        sprintf_s(row,"{\"event\":\"native_encoded_texture_resident\",\"generation\":%llu,\"index\":%u,\"bytes\":%llu,\"fence\":%llu,\"width\":%u,\"height\":%u,\"mips\":%u,\"format\":%u,\"slices\":%u,\"cube\":%s,\"direct_dds\":%s,\"legacy_handle\":false}\n",
            record->generation,source.index,record->content_bytes,record->upload_fence,texture.width,texture.height,
            texture.mip_levels,format,texture.array_slices,texture.cube?"true":"false",direct?"true":"false");Log(row);
        if(PopinObserve())popin_texture_creations.fetch_add(1,std::memory_order_relaxed);   // I-0100 tranche 8 (M2)
    } catch(const std::exception& e) {
        char row[256]{};sprintf_s(row,"{\"event\":\"native_encoded_texture_failure\",\"message\":\"%s\"}\n",e.what());Log(row);
        Refuse("native_encoded_texture_admission_failed");
    }
}
void __fastcall AdmitEncodedTexture(void* wrapper,void* source) noexcept {census::Note(census::TextureEncoded);AdmitEncodedTextureFamily(wrapper,source,false,reinterpret_cast<uintptr_t>(_ReturnAddress()));}
void __fastcall AdmitEncodedCube(void* wrapper,void* source) noexcept {census::Note(census::TextureEncoded);AdmitEncodedTextureFamily(wrapper,source,true,reinterpret_cast<uintptr_t>(_ReturnAddress()));}
uint32_t __fastcall TextureWidth(void* wrapper,uint32_t mip) noexcept {
    const auto record=textures.Acquire(reinterpret_cast<uintptr_t>(wrapper));
    return record && mip<record->description.mips?record->description.width>>mip:0;
}
uint32_t __fastcall TextureHeight(void* wrapper,uint32_t mip) noexcept {
    const auto record=textures.Acquire(reinterpret_cast<uintptr_t>(wrapper));
    return record && mip<record->description.mips?record->description.height>>mip:0;
}
uint32_t __fastcall TextureFormat(void* wrapper) noexcept {
    const auto record=textures.Acquire(reinterpret_cast<uintptr_t>(wrapper));
    return record?static_cast<uint32_t>(record->description.format):0;
}
uint32_t __fastcall TextureMips(void* wrapper) noexcept {
    const auto record=textures.Acquire(reinterpret_cast<uintptr_t>(wrapper));
    return record?record->description.mips:0;
}
uint64_t __fastcall TextureValid(void* wrapper) noexcept {
    return textures.Acquire(reinterpret_cast<uintptr_t>(wrapper))?1:0;
}
}  // namespace
