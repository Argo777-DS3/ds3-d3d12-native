// Resources and Setup: the hook installation and the module's start (mode, flags, renderer, worker). Part of
// game_bootstrap.cpp's single translation unit (the order is there): it relies on every earlier part (21_effects.inc before
// it).
#pragma once
#include "21_effects.inc"
namespace {
void __fastcall Resources(void* step) noexcept {
    if(control->mode==bootstrap::Mode::NativeBootstrap) {
        try {
            if(control->stop) { Close(); return; }
            if(!opened || !cpu_registered || !view_system_owned || GetCurrentThreadId()!=owner_thread)
                Refuse("resource_phase_without_native_owner");
            const auto call=InterlockedIncrement64(&control->phase_calls);
            if(call<=16) Observe(step,"native_resources","enter");
            uintptr_t repository{}; BYTE state{};
            if(!Read(reinterpret_cast<uintptr_t>(step)+0xF0,&repository,8) || !repository ||
               !Read(repository+0x70,&state,1)) Refuse("genuine_repository_unavailable");
            if(state==4 && !repository_ready_reported) {
                repository_ready_reported=true;
                Log("{\"event\":\"genuine_repository_ready\",\"state\":4,\"native_resources_implemented\":false}\n");
            }
            graphics_startup_task.store(reinterpret_cast<uintptr_t>(step));
            if(state==4 && !cpu_resources_requested) RequestCpuResources(step);
            if(state==4 && cpu_resources_requested){
                if(!native_resource_consumers)ConstructNativeResourceConsumers();
                CompleteNativeResourcePhase(step);
            }
        } catch(const std::exception& e) {NativeFailure(e.what());}
        return;
    }
    const auto call=InterlockedIncrement64(&control->phase_calls);
    if(call<=16) Observe(step,"resources","enter");
    if(call==2)ObserveCpuRequestCode();
    resources_original(step);
    static const bool surface_witness=ModuleFlag(L"ffx-draw-observe.flag");   // with D1E8A0 and D1EA10 hooked (step review L-a)
    if(surface_witness)LogNativeSurfaceStore("D59E90");   // I-0105 tranche 10: the root snow members after it (a row on a change only)
    if(call<=16) Observe(step,"resources","leave");
}
// I-0078 (Thomas, 2026-10-20: the original as a reference study, not to copy): with original-probe.flag in a reference run,
// none of the observation hooks is installed (I-0083: except the world update gate's frame timer); only the original's present pacer D5CB0 (K-0009: it waits to its deadline or
// lets DXGI sync, then presents) is timed: the time between two calls (the frame) and the time inside it (the wait and the
// present), so the presenting thread's own work per frame is their difference whatever caps the output (V-sync, VRR, a
// driver cap, a 60 Hz screen). With original-unlock.flag the requested interval is 0 (K-0009: no wait, Present(0)), in memory
// only. A row every 120 frames: native_original_frames. I-0079: the window's frames over 17, 25 and 34 ms and its slowest, and
// one original_slow_frame row per frame over 34 ms (1,000 a run), as the native journal's native_slow_frame.
using OriginalPacerFn=uint64_t(__fastcall*)(uint64_t,uint64_t,uint64_t,uint64_t);
OriginalPacerFn original_pacer{};
uint64_t __fastcall ProbeOriginalPacer(uint64_t a,uint64_t interval,uint64_t c,uint64_t d) noexcept {
    static const bool unlock=ModuleFlag(L"original-unlock.flag");
    struct Window {int64_t last_enter{};uint64_t frames{},calls{};double frame_ms{},inside_ms{},last_inside{};std::vector<float> frame_samples;};
    static Window w;static int slow_rows{};
    static const int64_t frequency=[]{LARGE_INTEGER f{};QueryPerformanceFrequency(&f);return f.QuadPart;}();
    // With sample-profile.flag, the sampler on the presenting thread and the process's first thread (the main loop, when it is
    // another), from the 1,200th frame, 30,000 samples each: where the frame's time goes, its waits included. I-0079: each
    // sample carries the pacer calls finished so far (the frame mark, as the native sampler's: the interval that ends at call
    // N holds the samples marked N), and sample-profile-start.txt next to the module (a call number) starts it there for 75,000
    // samples a thread; its optional second number is the call that ends it (the samples written then).
    static const bool sample=ModuleFlag(L"sample-profile.flag");static uint64_t calls{};static std::atomic<uint64_t> sampling_active{},frame_mark{};
    static uint64_t sample_end=~0ull;static const uint64_t sample_start=[]{uint64_t frame{};
        if(const auto file=ModuleSibling(L"sample-profile-start.txt");!file.empty()){
            std::wifstream start(file);if(!(start && (start>>frame)))frame=0;else{uint64_t end{};if(start>>end && end)sample_end=end;}}
        return frame;}();
    if(sample && ++calls==(sample_start?sample_start:1200)){std::vector<sampling::Thread> threads{{GetCurrentThreadId(),"presenter"}};
        {const HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);DWORD first{};uint64_t earliest=~0ull;THREADENTRY32 entry{sizeof entry};
            if(snapshot!=INVALID_HANDLE_VALUE){for(BOOL more=Thread32First(snapshot,&entry);more;more=Thread32Next(snapshot,&entry)){
                    if(entry.th32OwnerProcessID!=GetCurrentProcessId())continue;
                    if(const HANDLE thread=OpenThread(THREAD_QUERY_LIMITED_INFORMATION,FALSE,entry.th32ThreadID)){FILETIME created{},exited{},kernel{},user{};
                        if(GetThreadTimes(thread,&created,&exited,&kernel,&user)){const uint64_t at=(uint64_t(created.dwHighDateTime)<<32)|created.dwLowDateTime;
                            if(at<earliest){earliest=at;first=entry.th32ThreadID;}}CloseHandle(thread);}}
                CloseHandle(snapshot);}
            if(first && first!=GetCurrentThreadId())threads.push_back({first,"main"});}
        const auto output=JournalSibling(L"sample-profile.txt");
        const bool started=sampling::Start(output,threads,sampling_active,sample_start?75000:30000,&frame_mark,sample_end);sampling_active.store(started?1:0);
        // I-0097 tranche 4: with sample-ds3-threads.flag, DS3's task workers and loaders too (the native runs' comparison).
        const uint32_t ds3_threads=started && ModuleFlag(L"sample-ds3-threads.flag")?AddDs3SampledThreads(threads):0;
        char row[240]{};sprintf_s(row,"{\"event\":\"native_original_sample\",\"started\":%s,\"presenter\":%lu,\"main\":%lu,\"ds3_threads\":%u}\n",started?"true":"false",
            GetCurrentThreadId(),threads.size()>1?threads[1].id:GetCurrentThreadId(),ds3_threads);Log(row);}
    LARGE_INTEGER enter{},leave{};QueryPerformanceCounter(&enter);ds3native::cadence::MarkValue(ds3native::cadence::OriginalPacerEnter,enter.QuadPart);
    const auto result=original_pacer(a,unlock?0:interval,c,d);
    QueryPerformanceCounter(&leave);ds3native::cadence::MarkValue(ds3native::cadence::OriginalPacerLeave,leave.QuadPart);
    // I-0097: the original's display count, as the native's (DisplayRefreshes): the swap chain's frame statistics after its
    // Present. D5CB0's first argument holds the Present thunk's object (02139260), whose first field is the IDXGISwapChain
    // (K-0005, K-0007: vtable slot 8 = Present); used only when its vtable lies in dxgi.dll, re-read each frame (a resize may
    // replace the chain). Reference runs with original-probe.flag only, as the rest of this probe. The first frames' outcome is
    // logged once (original_display_probe: the chain found, its swap effect, the statistics' HRESULT): a windowed blt-model
    // chain has no frame statistics, and the rows would then be absent without a reason. Final review: once that row has found
    // the statistics unsupported (the original's windowed blt-model chain, D-0208), the probe stops reading them.
    static ds3native::DisplayRefreshes display;static uint32_t display_hz{};static bool display_unsupported{};
    static const bool display_holds=[]{try{display.KeepHolds(ModuleFlag(L"frame-dump.flag"));}catch(...){}return true;}();(void)display_holds;   // I-0098
    if(!display_unsupported){static const HMODULE dxgi=GetModuleHandleW(L"dxgi.dll");static int reported{};uintptr_t holder{},chain{},table{};HMODULE owner{};
        HRESULT read=E_POINTER;DXGI_FRAME_STATISTICS statistics{};
        const bool found=dxgi && Read(a,&holder,8) && holder && Read(holder,&chain,8) && chain && Read(chain,&table,8) && table &&
           GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(table),&owner) && owner==dxgi;
        if(found)read=reinterpret_cast<IDXGISwapChain*>(chain)->GetFrameStatistics(&statistics);
        if(found && !(w.calls%120)){DXGI_SWAP_CHAIN_DESC desc{};if(SUCCEEDED(reinterpret_cast<IDXGISwapChain*>(chain)->GetDesc(&desc)))display_hz=ds3native::RefreshRateOf(desc.OutputWindow);}
        if(w.calls==600 && !reported++){DXGI_SWAP_CHAIN_DESC desc{};const bool described=found && SUCCEEDED(reinterpret_cast<IDXGISwapChain*>(chain)->GetDesc(&desc));
            char row[260]{};sprintf_s(row,"{\"event\":\"original_display_probe\",\"chain_found\":%s,\"swap_effect\":%d,\"buffers\":%u,\"windowed\":%s,\"statistics_hresult\":\"0x%08X\",\"refresh_hz\":%u}\n",
                found?"true":"false",described?int(desc.SwapEffect):-1,described?desc.BufferCount:0u,described && desc.Windowed?"true":"false",static_cast<unsigned>(read),display_hz);Log(row);}
        display.Add(SUCCEEDED(read),statistics.PresentCount,statistics.PresentRefreshCount);
        if(w.calls>=600 && read==DXGI_ERROR_UNSUPPORTED)display_unsupported=true;}
    // The game's own cadence (timing/ I-0009, I-0011, imported from the timing project LTF-TRTF: the central timing object
    // [0x14489DD10], requested rate float +0x354 and its enable byte +0x358, the pair DS3DebugFPS writes): with
    // original-unlock.flag, 360 frames/s requested (the highest value that project used; it has no unlimited value), in memory;
    // reference runs only. Written after the call (review L4: nothing touches the pacer's registers before it), when the page is
    // writable (review M2).
    if(unlock){uintptr_t timing{};MEMORY_BASIC_INFORMATION page{};if(Read(image+0x489DD10,&timing,8) && timing && VirtualQuery(reinterpret_cast<void*>(timing+0x354),&page,sizeof page) &&
           page.State==MEM_COMMIT && (page.Protect&(PAGE_READWRITE|PAGE_EXECUTE_READWRITE)) && !(page.Protect&PAGE_GUARD)){float rate{};BYTE on{};
            if(Read(timing+0x354,&rate,4) && Read(timing+0x358,&on,1) && (rate!=360.f || !on)){
                *reinterpret_cast<volatile float*>(timing+0x354)=360.f;*reinterpret_cast<volatile BYTE*>(timing+0x358)=1;}}}
    if(w.last_enter){const double frame=1000.0*double(enter.QuadPart-w.last_enter)/double(frequency);
        w.frame_ms+=frame;w.inside_ms+=1000.0*double(leave.QuadPart-enter.QuadPart)/double(frequency);w.frame_samples.push_back(static_cast<float>(frame));
        if(frame>34.0 && slow_rows<1000){++slow_rows;char slow[200]{};sprintf_s(slow,"{\"event\":\"original_slow_frame\",\"call\":%llu,\"interval_ms\":%.1f,\"previous_pacer_ms\":%.2f,\"tick_ms\":%llu}\n",
            static_cast<unsigned long long>(w.calls),frame,w.last_inside,static_cast<unsigned long long>(GetTickCount64()));Log(slow);}
        if(++w.frames==120){std::vector<float> sorted(w.frame_samples);std::sort(sorted.begin(),sorted.end());size_t over17{},over25{},over34{};
            for(const auto v:sorted){over17+=v>17.f;over25+=v>25.f;over34+=v>34.f;}
            char row[400]{};sprintf_s(row,"{\"event\":\"native_original_frames\",\"frames\":120,\"thread\":%lu,\"requested_interval\":%llu,\"unlocked\":%s,\"frame_ms_mean\":%.3f,\"frame_ms_p50\":%.3f,\"frame_ms_p99\":%.3f,\"frame_ms_max\":%.3f,\"over_17_25_34_ms\":[%zu,%zu,%zu],\"call\":%llu,\"pacer_ms_mean\":%.3f,\"work_ms_mean\":%.3f,\"tick_ms\":%llu}\n",
                GetCurrentThreadId(),static_cast<unsigned long long>(interval&0xFFFFFFFFull),unlock?"true":"false",w.frame_ms/120.0,sorted[60],sorted[118],sorted[119],
                over17,over25,over34,static_cast<unsigned long long>(w.calls),w.inside_ms/120.0,(w.frame_ms-w.inside_ms)/120.0,static_cast<unsigned long long>(GetTickCount64()));LogFrameTimes(row,w.frame_samples);
            LogDisplayRefreshes(display.Take(ds3native::DisplayRefreshes::SpareHolds(display.KeepsHolds())),"original",static_cast<int64_t>(w.calls),display_hz);
            w.frames=0;w.frame_ms=w.inside_ms=0;w.frame_samples.clear();}}
    w.last_enter=enter.QuadPart;w.last_inside=1000.0*double(leave.QuadPart-enter.QuadPart)/double(frequency);
    frame_mark.store(++w.calls,std::memory_order_relaxed);
    return result;
}
DWORD WINAPI Setup(void*) {
    wchar_t name[96]{}; bootstrap::Name(name,GetCurrentProcessId());
    HANDLE map=OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,name);
    if(!map) return 1;
    control=static_cast<bootstrap::Control*>(MapViewOfFile(map,FILE_MAP_ALL_ACCESS,0,0,sizeof(bootstrap::Control)));
    if(!control || control->magic!=bootstrap::signature || control->version!=bootstrap::control_version || control->pid!=GetCurrentProcessId() ||
       (control->mode!=bootstrap::Mode::Reference && control->mode!=bootstrap::Mode::NativeBootstrap)) return 2;
    control->journal[1023]=0;
    bounded_run=control->bounded_seconds>0;   // security audit M1/M2: kept here, before any hook
    journal=CreateFileW(control->journal,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(journal==INVALID_HANDLE_VALUE) { InterlockedExchange(&control->ready,-1); return 3; }
    HMODULE own{};
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&Setup),&own)) { InterlockedExchange(&control->ready,-2); return 4; }
    image=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    // I-0097: the pins are the installed file's bytes. Refusals came 5 times in about 640 native runs, mostly at the first launch
    // of a new build; at i97-final-m1, 2 of 639 differed, the first 0x810E90, a function encrypted at rest whose bytes were already
    // its decrypted code (21_effects.inc checks that prefix before calling it): this setup had run after the game decrypted it.
    // Tranche 2: a pin whose bytes are the known decrypted code (decrypted_pins) is accepted; every other differing pin refuses
    // the launch, and the refusal names each one (its RVA and the 16 bytes found), at most 12.
    {struct Decrypted {DWORD rva;const BYTE* bytes;};
        static constexpr BYTE quality_prefix[16]{0x40,0x57,0x48,0x83,0xEC,0x70,0x48,0xC7,0x44,0x24,0x28,0xFE,0xFF,0xFF,0xFF,0x48};   // 21_effects.inc's quality_code
        static_assert(sizeof(loaded_sfx_scene_reset_code)>=16);
        static constexpr Decrypted decrypted_pins[]{{0x810E90,quality_prefix},
            // pin-census.flag, i97-t2-obs-1: the only other pin the game changes after a normal setup (two minutes in), the leaf
            // 18_resource_admission.inc checks whole (the generated loaded_sfx_scene_reset_code, its first 16 bytes here)
            {0xE6E620,loaded_sfx_scene_reset_code}};
        const auto hex=[](const Pin& pin){std::string text;char cell[4]{};
            for(size_t i=0;i<16;++i){sprintf_s(cell,"%02X",reinterpret_cast<const BYTE*>(image+pin.rva)[i]);text+=cell;}return text;};
        size_t differing{},accepted{};std::string named,decrypted;
        for(const auto& pin:pins){if(!std::memcmp(reinterpret_cast<void*>(image+pin.rva),pin.bytes,sizeof(pin.bytes)))continue;
            const bool known=std::any_of(std::begin(decrypted_pins),std::end(decrypted_pins),[&](const Decrypted& d){
                return d.rva==pin.rva && !std::memcmp(reinterpret_cast<void*>(image+pin.rva),d.bytes,16);});
            auto& list=known?decrypted:named;if((known?accepted:differing)<12)list+=(list.empty()?"":",")+std::string("[")+std::to_string(pin.rva)+",\""+hex(pin)+"\"]";
            ++(known?accepted:differing);}
        if(accepted){const std::string row="{\"event\":\"prefix_decrypted_accepted\",\"pins\":"+std::to_string(accepted)+",\"found\":["+decrypted+"]}\n";Log(row.c_str());}
        if(differing){const std::string row="{\"event\":\"prefix_refused\",\"differing\":"+std::to_string(differing)+",\"of\":"+std::to_string(std::size(pins))+
                ",\"accepted_decrypted\":"+std::to_string(accepted)+",\"found\":["+named+"]}\n";Log(row.c_str());
            InterlockedExchange(&control->ready,-3); return 5;}}
    // I-0097 tranche 2 (pin-census.flag, a diagnostic): the pins' bytes kept here, before any hook; two minutes later, every pin
    // whose bytes changed since (the functions the game decrypted after this setup, which a later setup would meet), once, from a
    // thread of its own. A changed pin that now starts with a jump (E9, the hooks' patch) is counted apart, not listed (step review).
    if(ModuleFlag(L"pin-census.flag")){auto* kept=new std::vector<std::array<BYTE,16>>(std::size(pins));
        for(size_t i=0;i<std::size(pins);++i)std::memcpy((*kept)[i].data(),reinterpret_cast<const void*>(image+pins[i].rva),16);
        if(const HANDLE census=CreateThread(nullptr,0,[](void* argument)->DWORD{Sleep(120000);
                const std::unique_ptr<std::vector<std::array<BYTE,16>>> before(static_cast<std::vector<std::array<BYTE,16>>*>(argument));
                std::string rows;size_t changed{},jumps{},unreadable{};
                for(size_t i=0;i<std::size(pins);++i){std::array<BYTE,16> now{};if(!Read(image+pins[i].rva,now.data(),now.size())){++unreadable;continue;}
                    if(now==(*before)[i])continue;if(now[0]==0xE9){++jumps;continue;}
                    std::string text;char cell[4]{};for(const auto byte:now){sprintf_s(cell,"%02X",byte);text+=cell;}
                    rows+=(changed++?",":"")+std::string("[")+std::to_string(pins[i].rva)+",\""+text+"\"]";}
                const std::string row="{\"event\":\"pin_census\",\"of\":"+std::to_string(std::size(pins))+",\"changed\":"+std::to_string(changed)+
                    ",\"hooked\":"+std::to_string(jumps)+",\"unreadable\":"+std::to_string(unreadable)+",\"found\":["+rows+"]}\n";Log(row.c_str());return 0;},kept,0,nullptr))CloseHandle(census);
        else delete kept;}
    // I-0082: what the fault exits need, set before they can run.
    // A std::terminate in the module (a throw out of a noexcept hook, on any thread) ends in the CRT's abort, whose fast fail
    // no handler sees: the module's own CRT (static) raises SIGABRT instead, process-wide, and its handler asks for the dump.
    _set_abort_behavior(0,_CALL_REPORTFAULT);
    signal(SIGABRT,[](int){ExitNative(0xD3126608,"abort (std::terminate)");});   // the single fault exit (dump, then exit)
    AddVectoredExceptionHandler(1,&Fault);
    if(MH_Initialize()!=MH_OK) { InterlockedExchange(&control->ready,-4); return 6; }
    // I-0132 R12h: the game's window black from its creation (20_startup_phases.inc's black_window), first, before the game makes it.
    if(control->mode==bootstrap::Mode::NativeBootstrap && !ModuleFlag(L"no-black-window.flag"))black_window::Install();
    if(ModuleFlag(L"cadence-trace.flag"))StartCadenceTrace();   // I-0097 tranche 5: both renderers (diagnostics only)
    // I-0100 tranche 2 (WORKFLOW « Tools »: pixtool): with pix-capture.flag, PIX's GPU capturer loaded before the native device, so
    // the presentation can take programmatic captures of chosen frames (FinishNativeFrame, at presented frames 3060, 3660, 4260 and 4860: pix-<frame>.wpix beside the module, about 1 GB each); development runs only.
    if(ModuleFlag(L"pix-capture.flag")){const auto capturer=LoadLibraryW(L"C:\\Program Files\\Microsoft PIX\\2603.25\\WinPixGpuCapturer.dll");
        char row[120]{};sprintf_s(row,"{\"event\":\"native_pix_capturer\",\"loaded\":%s}\n",capturer?"true":"false");Log(row);}
    // I-0141 (the ceremony's smoke, the original's side; WORKFLOW « Tools »: RenderDoc for a frame of the original): with
    // renderdoc-capture.flag in the Reference mode, RenderDoc 1.47's capture layer loaded before the game's D3D11 device, with its
    // defaults (capture keys F12 and Print Screen, captures under %TEMP%\RenderDoc as <exe>_<date>_frame<N>.rdc), read afterwards by
    // renderdoccmd / qrenderdoc; development runs only, never in the play launcher (its folder carries no flag).
    if(control->mode==bootstrap::Mode::Reference && ModuleFlag(L"renderdoc-capture.flag")){
        const auto layer=LoadLibraryW(L"C:\\Program Files\\RenderDoc\\renderdoc.dll");
        char row[160]{};sprintf_s(row,"{\"event\":\"reference_renderdoc_layer\",\"loaded\":%s,\"d3d11_before\":%s}\n",layer?"true":"false",
            GetModuleHandleW(L"d3d11.dll")?"true":"false");Log(row);}
    if(control->mode==bootstrap::Mode::Reference && ModuleFlag(L"original-probe.flag")){   // I-0078: the original's frames alone
        // I-0100 tranche 4 step review M1: the characters' schedule hook (test-death.flag) is not installed in this mode: refused
        // loudly rather than a run without its death; lod-launch-scale.flag takes its 6E770 hook here too (below).
        if(ModuleFlag(L"test-death.flag")){Log("{\"event\":\"test_death_refused\",\"reason\":\"original-probe mode: use plain Reference\"}\n");InterlockedExchange(&control->ready,-7);return 9;}
        // I-0140 tranche 30 (its step review's M1 after the final review's L5): test-region-close.flag rides the same hook, refused here too.
        if(ModuleFlag(L"test-region-close.flag")){Log("{\"event\":\"test_region_close_refused\",\"reason\":\"original-probe mode: native mode only\"}\n");InterlockedExchange(&control->ready,-7);return 9;}
        if(ModuleFlag(L"reference-no-ffx-draw.flag")){Log("{\"event\":\"reference_no_ffx_draw\",\"refused\":\"original-probe mode: use plain Reference\"}\n");InterlockedExchange(&control->ready,-7);return 9;}   // I-0141 tranche 15
        if(ModuleFlag(L"reference-no-occlusion.flag")){Log("{\"event\":\"reference_no_occlusion\",\"refused\":\"original-probe mode: use plain Reference\"}\n");InterlockedExchange(&control->ready,-7);return 9;}   // I-0141 tranche 18 (step review M3)
        if(ModuleFlag(L"aa-observe.flag")){Log("{\"event\":\"reference_aa_filter\",\"refused\":\"original-probe mode: use plain Reference\"}\n");InterlockedExchange(&control->ready,-7);return 9;}   // I-0140 tranche 39
        if(ModuleFlag(L"reference-share-variant.flag")){Log("{\"event\":\"reference_share_variant\",\"refused\":\"original-probe mode: use plain Reference\"}\n");InterlockedExchange(&control->ready,-7);return 9;}   // I-0141 tranche 27
        if(ModuleFlag(L"hold-world-freeze.flag")){Log("{\"event\":\"witness_hold_world_freeze\",\"refused\":\"original-probe mode: use plain Reference\"}\n");InterlockedExchange(&control->ready,-7);return 9;}   // the final review's L6
        // I-0083: and its game step at the world update gate (game_frame_times, the native's point).
        if(MH_CreateHook(reinterpret_cast<void*>(image+0xD5CB0),reinterpret_cast<void*>(&ProbeOriginalPacer),reinterpret_cast<void**>(&original_pacer))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x8FA3E0),reinterpret_cast<void*>(&ProbeOriginalWorldGate),reinterpret_cast<void**>(&world_update_gate_original))!=MH_OK ||
           // I-0100 T0 (popin-observe.flag): the main-view draws (items, draw entities, character parts) and the LOD step's camera, pass-through.
           (PopinObserve() && (MH_CreateHook(reinterpret_cast<void*>(image+0xD3DA20),reinterpret_cast<void*>(&PopinItemDraw),reinterpret_cast<void**>(&popin_item_draw_original))!=MH_OK ||
               MH_CreateHook(reinterpret_cast<void*>(image+0xD079C0),reinterpret_cast<void*>(&PopinDrawEmit),reinterpret_cast<void**>(&popin_draw_emit_original))!=MH_OK ||
               MH_CreateHook(reinterpret_cast<void*>(image+0xCFDE70),reinterpret_cast<void*>(&PopinPartDraw),reinterpret_cast<void**>(&popin_part_draw_original))!=MH_OK ||
               MH_CreateHook(reinterpret_cast<void*>(image+0xD1CCE0),reinterpret_cast<void*>(&ObserveLodStep),reinterpret_cast<void**>(&lod_step_original))!=MH_OK ||
               MH_CreateHook(reinterpret_cast<void*>(image+0x66BBB0),reinterpret_cast<void*>(&ObserveObjectSync),reinterpret_cast<void**>(&object_sync_original))!=MH_OK ||   // tranche 5
               // I-0100 tranche 2: the depth rows (the shadows' casters), pass-through.
               MH_CreateHook(reinterpret_cast<void*>(image+0x63CB0),reinterpret_cast<void*>(&ObserveRowExecute),reinterpret_cast<void**>(&row_execute_original))!=MH_OK ||
               // I-0100 tranche 8 (M2): DS3's texture creations and the loading page's close, pass-through.
               MH_CreateHook(reinterpret_cast<void*>(image+0x2B260),reinterpret_cast<void*>(&PopinTextureCreate),reinterpret_cast<void**>(&popin_texture_create_original))!=MH_OK ||
               MH_CreateHook(reinterpret_cast<void*>(image+0xAC110),reinterpret_cast<void*>(&PopinCubeCreate),reinterpret_cast<void**>(&popin_cube_create_original))!=MH_OK ||
               MH_CreateHook(reinterpret_cast<void*>(image+0xA4E7C0),reinterpret_cast<void*>(&ObserveWindowClose),reinterpret_cast<void**>(&window_close_original))!=MH_OK ||
               // I-0100 tranche 5 (caster-place.flag): the FLVER instance draws, for the shadow passes' casters by place, pass-through.
               (ModuleFlag(L"caster-place.flag") && (MH_CreateHook(reinterpret_cast<void*>(image+0x5CEC0),reinterpret_cast<void*>(&ObserveRigidInstanceDraw),reinterpret_cast<void**>(&rigid_instance_draw_original))!=MH_OK ||
                   MH_CreateHook(reinterpret_cast<void*>(image+0x5D5C0),reinterpret_cast<void*>(&ObserveSkinnedInstanceDraw),reinterpret_cast<void**>(&skinned_instance_draw_original))!=MH_OK ||
                   // tranche 6: the sun cascade passes' caster cullers.
                   MH_CreateHook(reinterpret_cast<void*>(image+0x23C1E0),reinterpret_cast<void*>(&ObserveCasterCullerBuild),reinterpret_cast<void**>(&caster_culler_build_original))!=MH_OK ||
                   MH_CreateHook(reinterpret_cast<void*>(image+0x57A20),reinterpret_cast<void*>(&ObserveCasterRows),reinterpret_cast<void**>(&caster_rows_original))!=MH_OK)))) ||
           // I-0100 tranche 2 (light-observe.flag): the instance records 5B3D0 writes (the ambient rows by row type), pass-through.
           (ModuleFlag(L"light-observe.flag") && (MH_CreateHook(reinterpret_cast<void*>(image+0x5B3D0),reinterpret_cast<void*>(&ObserveInstanceRecord),
               reinterpret_cast<void**>(&instance_record_original))!=MH_OK ||
               MH_CreateHook(reinterpret_cast<void*>(image+0x31A20),reinterpret_cast<void*>(&ObserveLightOverride),reinterpret_cast<void**>(&light_override_original))!=MH_OK ||   // I-0141: the cutscene act's bank writes
               MH_CreateHook(reinterpret_cast<void*>(image+0x380A0),reinterpret_cast<void*>(&ObserveFogOverride),reinterpret_cast<void**>(&fog_override_original))!=MH_OK ||
               MH_CreateHook(reinterpret_cast<void*>(image+0x320B0),reinterpret_cast<void*>(&ObserveLightOverrideRemove),reinterpret_cast<void**>(&light_override_remove_original))!=MH_OK ||
               MH_CreateHook(reinterpret_cast<void*>(image+0x387D0),reinterpret_cast<void*>(&ObserveFogOverrideRemove),reinterpret_cast<void**>(&fog_override_remove_original))!=MH_OK ||
               // I-0113 tranche 10: the forward rows' IBL binder 75B30.
               MH_CreateHook(reinterpret_cast<void*>(image+0x75B30),reinterpret_cast<void*>(&ObserveForwardIblBind),
               reinterpret_cast<void**>(&forward_ibl_bind_original))!=MH_OK ||
               // I-0140 tranche 28 (D-0301's witness): the menu views' region adder D0C2F0 and the region pass's selection 67070.
               MH_CreateHook(reinterpret_cast<void*>(image+0xD0C2F0),reinterpret_cast<void*>(&ObserveMenuRegionAdd),reinterpret_cast<void**>(&menu_region_add_original))!=MH_OK ||
               MH_CreateHook(reinterpret_cast<void*>(image+0x67070),reinterpret_cast<void*>(&ObserveRegionSelect),reinterpret_cast<void**>(&region_select_original))!=MH_OK)) ||
           (ModuleFlag(L"lod-launch-scale.flag") && MH_CreateHook(reinterpret_cast<void*>(image+0x6E770),reinterpret_cast<void*>(&KeepLodReference),
               reinterpret_cast<void**>(&lod_reference_set_original))!=MH_OK) ||
           MH_EnableHook(MH_ALL_HOOKS)!=MH_OK){InterlockedExchange(&control->ready,-7);return 9;}
        Log("{\"event\":\"ready\",\"mode\":\"reference\",\"original_probe\":true}\n");InterlockedExchange(&control->ready,1);return 0;}
    // W4 FW-2b (D-0173), the default since FW-9 (no-native-effects.flag disables W4): the effect GX module reads the native effect display instead of the null GX
    // display object; done before any hook copies a prologue.
    native_effects_enabled=control->mode==bootstrap::Mode::NativeBootstrap && !ModuleFlag(L"no-native-effects.flag");
    // FFX-DRAW (on by default with the effect system, no-ffx-draw.flag opts out): the classic FFX draws recorded, executed and encoded.
    native_ffx_draw=native_effects_enabled && !ModuleFlag(L"no-ffx-draw.flag");
    if(native_effects_enabled){
        const auto redirect=effect_system::RedirectDisplay(image,native_ffx_draw);
        char row[256]{};sprintf_s(row,"{\"event\":\"native_effect_display_redirect\",\"sites\":%d,\"refused\":\"%s\"}\n",redirect.sites,redirect.refused?redirect.refused:"");Log(row);
        if(redirect.refused){InterlockedExchange(&control->ready,-18);return 20;}
    }
    struct Hook { DWORD rva; void* entry; void** original; };
    const Hook ui_events[]{
        {0xA5B930,reinterpret_cast<void*>(&ObserveUiInputEligibility),reinterpret_cast<void**>(&ui_input_eligible_original)},
        {0xA5C010,reinterpret_cast<void*>(&ObserveUiCondition13),reinterpret_cast<void**>(&ui_condition13_original)},
        {0xE53580,reinterpret_cast<void*>(&ObserveUiMovieAdvance),reinterpret_cast<void**>(&ui_movie_advance_original)},
        {0xEBB5B0,reinterpret_cast<void*>(&ObserveInputConfirmUpdate),reinterpret_cast<void**>(&input_confirm_update_original)},
        {0xEB5B40,reinterpret_cast<void*>(&ObserveInputStepRun),reinterpret_cast<void**>(&input_step_run_original)},
        {0xC89A0,reinterpret_cast<void*>(&ObserveDynamicLayerInsert),reinterpret_cast<void**>(&dynamic_layer_insert_original)},
        {0xC24E0,reinterpret_cast<void*>(&ObserveFlatLayerInsert),reinterpret_cast<void**>(&flat_layer_insert_original)},
        {0xD514C0,reinterpret_cast<void*>(&ObserveTextureBind),reinterpret_cast<void**>(&texture_bind_original)},
        {0xD1CCE0,reinterpret_cast<void*>(&ObserveLodStep),reinterpret_cast<void**>(&lod_step_original)},
        {0xD40010,reinterpret_cast<void*>(&ObserveProbeSource),reinterpret_cast<void**>(&probe_source_original)},
        {0xD3FFB0,reinterpret_cast<void*>(&ObserveProbePick),reinterpret_cast<void**>(&probe_pick_original)},   // Fable review: the LOD fade step (rows with light-observe.flag)
        {0x6E770,reinterpret_cast<void*>(&KeepLodReference),reinterpret_cast<void**>(&lod_reference_set_original)}};   // I-0100: option A, by default natively (D-0222)
    static const bool lod_rows=ModuleFlag(L"light-observe.flag");   // pre-merge review: the LOD step hook only with its rows (a failed hook must not stop a play); I-0100 T0: or popin-observe.flag (the camera)
    // I-0094 (BOTTLENECKS item 5, ER-STUDY P9): the pass-through witnesses on DS3's update and UI paths, which only log (their rows
    // came from earlier investigations), are installed in a native run only with update-observe.flag; the Reference mode keeps them
    // for its comparisons. Not witnesses: the menu movies' advance (the pilots wait for its first-advance rows), the texture bind
    // (the default-bound registry), the characters' schedule (the test tools
    // and witness moves), the world update gate (the game step measure).
    static const bool update_witnesses=control->mode!=bootstrap::Mode::NativeBootstrap || ModuleFlag(L"update-observe.flag");
    const auto witness_only=[](const void* entry){
        static const void* const witnesses[]{reinterpret_cast<const void*>(&ObserveUiInputEligibility),reinterpret_cast<const void*>(&ObserveUiCondition13),
            reinterpret_cast<const void*>(&ObserveInputConfirmUpdate),reinterpret_cast<const void*>(&ObserveInputStepRun),
            reinterpret_cast<const void*>(&ObserveDynamicLayerInsert),reinterpret_cast<const void*>(&ObserveFlatLayerInsert),reinterpret_cast<const void*>(&ObserveNativeAsmUpdate),
            reinterpret_cast<const void*>(&ObserveNativeModelEntityUpdate),reinterpret_cast<const void*>(&ObserveChrAsmModelUpdate),reinterpret_cast<const void*>(&ObserveSprjAsmModelUpdate),
            reinterpret_cast<const void*>(&ObserveAsmModelTick),reinterpret_cast<const void*>(&ObservePlayerModelUpdate),reinterpret_cast<const void*>(&ObserveChrEzTaskExecute),
            reinterpret_cast<const void*>(&ObserveEzTaskRegister),reinterpret_cast<const void*>(&ObserveChrModelRequest),reinterpret_cast<const void*>(&ObserveChrSchedule),
            reinterpret_cast<const void*>(&ObservePartPoseUpdate),
            // I-0120 A5: the 1172-1177 RCA pass-through witnesses (log-only: teardown rows, body-scale update, sound, part mask, menu draw); not
            // ObserveWindowClose (popin-observe reads popin_page_closed), nor the title step's runs (the runners wait on its title_step_state
            // row for a loaded save), nor the native offscreen retire and release hooks.
            reinterpret_cast<const void*>(&ObserveBodyScaleModifierRetire),reinterpret_cast<const void*>(&ObserveDeferredEnqueue),reinterpret_cast<const void*>(&ObserveDeferredFlush),
            reinterpret_cast<const void*>(&ObserveTitleDialogRetire),
            reinterpret_cast<const void*>(&ObserveTitleStepRetire),reinterpret_cast<const void*>(&ObserveBodyScaleUpdate),reinterpret_cast<const void*>(&ObserveLocationStepFinish),
            reinterpret_cast<const void*>(&ObserveSoundEvent),reinterpret_cast<const void*>(&ObservePartMeshMask),reinterpret_cast<const void*>(&ObserveMenuItemDraw)};
        return std::find(std::begin(witnesses),std::end(witnesses),entry)!=std::end(witnesses);};
    // Option A, Thomas's choice (2026-10-27 02:05, D-0222): the launch LOD scale kept through the session, by default in a native
    // run (no-lod-launch-scale.flag gives DS3's own behaviour); the Reference mode stays the original unless lod-launch-scale.flag.
    static const bool lod_launch_scale=control->mode==bootstrap::Mode::NativeBootstrap?!ModuleFlag(L"no-lod-launch-scale.flag"):ModuleFlag(L"lod-launch-scale.flag");
    for(const auto& h:ui_events)if(!((h.rva==0xD1CCE0 && !lod_rows && !PopinObserve()) || ((h.rva==0xD40010 || h.rva==0xD3FFB0) && !lod_rows) || (h.rva==0x6E770 && !lod_launch_scale)) && (update_witnesses || !witness_only(h.entry)) && MH_CreateHook(reinterpret_cast<void*>(image+h.rva),h.entry,h.original)!=MH_OK){
        InterlockedExchange(&control->ready,-15);return 17;
    }
    if(control->mode==bootstrap::Mode::Reference &&
       MH_CreateHook(reinterpret_cast<void*>(image+0xA60700),reinterpret_cast<void*>(&ObserveOriginalUiPointer),
            reinterpret_cast<void**>(&ui_pointer_original))!=MH_OK){InterlockedExchange(&control->ready,-15);return 17;}
    if(MH_CreateHook(reinterpret_cast<void*>(image+0xE586E0),control->mode==bootstrap::Mode::Reference?
            reinterpret_cast<void*>(&ObserveOriginalUiManagerDestroy):reinterpret_cast<void*>(&NativeUiManagerDestroy),
            reinterpret_cast<void**>(&ui_manager_destroy_original))!=MH_OK){InterlockedExchange(&control->ready,-15);return 17;}
    const Hook phases[]{ {0xD59D40,reinterpret_cast<void*>(&Start),reinterpret_cast<void**>(&start_original)},
        {0xD59E90,reinterpret_cast<void*>(&Resources),reinterpret_cast<void**>(&resources_original)},
        {0xD59E20,reinterpret_cast<void*>(&Views),reinterpret_cast<void**>(&view_original)} };
    for(const auto& h:phases) if(MH_CreateHook(reinterpret_cast<void*>(image+h.rva),h.entry,h.original)!=MH_OK) {
        InterlockedExchange(&control->ready,-5); return 7;
    }
    {
        constexpr BYTE factory_pin[16]{0x48,0x8B,0xC4,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x83};
        BYTE prefix[16]{};
        if(!Read(image+0xE5B250,prefix,sizeof(prefix)) || std::memcmp(prefix,factory_pin,sizeof(prefix)) ||
           MH_CreateHook(reinterpret_cast<void*>(image+0xE5B250),control->mode==bootstrap::Mode::Reference?reinterpret_cast<void*>(&ObserveUiFactory):reinterpret_cast<void*>(&NativeUiFactory),
               reinterpret_cast<void**>(&ui_factory_original))!=MH_OK){InterlockedExchange(&control->ready,-14);return 16;}
    }
    if(control->mode==bootstrap::Mode::Reference &&
       MH_CreateHook(reinterpret_cast<void*>(image+0x1F741F0),reinterpret_cast<void*>(&ObserveProcessedPrimitive),
           reinterpret_cast<void**>(&processed_primitive_original))!=MH_OK) {
        InterlockedExchange(&control->ready,-8);return 10;
    }
    if(control->mode==bootstrap::Mode::Reference &&
       MH_CreateHook(reinterpret_cast<void*>(image+0x1F7E360),reinterpret_cast<void*>(&ObserveMeshWriteDestinations),
           reinterpret_cast<void**>(&mesh_write_original))!=MH_OK) {
        InterlockedExchange(&control->ready,-9);return 11;
    }
    if(control->mode==bootstrap::Mode::Reference &&
       MH_CreateHook(reinterpret_cast<void*>(image+0x1CF8750),reinterpret_cast<void*>(&ObserveCpuMeshWrite),
           reinterpret_cast<void**>(&cpu_mesh_write_original))!=MH_OK) {
        InterlockedExchange(&control->ready,-10);return 12;
    }
    if(control->mode==bootstrap::Mode::Reference) {
        // 1076: the same pass-through character update witnesses under the original renderer.
        const Hook character_updates[]{
            {0x8C1BC0,reinterpret_cast<void*>(&ObservePlayerModelUpdate),reinterpret_cast<void**>(&player_model_update_original)},
            {0xC48180,reinterpret_cast<void*>(&ObserveAsmModelTick),reinterpret_cast<void**>(&asm_model_tick_original)},
            {0xD01610,reinterpret_cast<void*>(&ObserveNativeAsmUpdate),reinterpret_cast<void**>(&asm_update_original)},
            {0x888C00,reinterpret_cast<void*>(&ObserveChrEzTaskExecute),reinterpret_cast<void**>(&chr_eztask_execute_original)},
            {0x65CAC0,reinterpret_cast<void*>(&ObserveEzTaskRegister),reinterpret_cast<void**>(&eztask_register_original)},
            {0x8CA1E0,reinterpret_cast<void*>(&ObserveChrModelRequest),reinterpret_cast<void**>(&chr_model_request_original)},
            {0x88E1F0,reinterpret_cast<void*>(&ObserveChrSchedule),reinterpret_cast<void**>(&chr_schedule_original)},
            {0x8DD2E0,reinterpret_cast<void*>(&ObserveChrManagerSchedule),reinterpret_cast<void**>(&chr_manager_schedule_original)},
            {0xCD5D80,reinterpret_cast<void*>(&ObserveRemoPlayWait),reinterpret_cast<void**>(&remo_play_wait_original)},   // I-0138 t8g: the Remo clock
            {0x8FA3E0,reinterpret_cast<void*>(&ObserveWorldUpdateGate),reinterpret_cast<void**>(&world_update_gate_original)},
            {0xD2D040,reinterpret_cast<void*>(&ObservePartPoseUpdate),reinterpret_cast<void**>(&part_pose_update_original)},
            {0x140680,reinterpret_cast<void*>(&ObserveMaterialPrepare),reinterpret_cast<void**>(&material_prepare_original)},
            // 1172 RCA: menu-model teardown order under the original renderer (pass-through witnesses).
            {0xD66280,reinterpret_cast<void*>(&NativeOffscreenRendererRetire),reinterpret_cast<void**>(&offscreen_renderer_retire_original)},
            {0xD68EB0,reinterpret_cast<void*>(&NativeOffscreenModelRelease),reinterpret_cast<void**>(&offscreen_model_release_original)},
            {0xD123F0,reinterpret_cast<void*>(&ObserveBodyScaleModifierRetire),reinterpret_cast<void**>(&body_scale_modifier_retire_original)},
            {0x8190B0,reinterpret_cast<void*>(&ObserveDeferredEnqueue),reinterpret_cast<void**>(&deferred_enqueue_original)},
            {0xEE42B0,reinterpret_cast<void*>(&ObserveDeferredFlush),reinterpret_cast<void**>(&deferred_flush_original)},
            {0xAD9820,reinterpret_cast<void*>(&ObserveTitleDialogRetire),reinterpret_cast<void**>(&title_dialog_retire_original)},
            {0xA4E7C0,reinterpret_cast<void*>(&ObserveWindowClose),reinterpret_cast<void**>(&window_close_original)},
            {0x90A4A0,reinterpret_cast<void*>(&ObserveTitleStepRunA),reinterpret_cast<void**>(&title_step_run_a_original)},
            {0x90A520,reinterpret_cast<void*>(&ObserveTitleStepRunB),reinterpret_cast<void**>(&title_step_run_b_original)},
            {0x909EB0,reinterpret_cast<void*>(&ObserveTitleStepRetire),reinterpret_cast<void**>(&title_step_retire_original)},
            {0xD126B0,reinterpret_cast<void*>(&ObserveBodyScaleUpdate),reinterpret_cast<void**>(&body_scale_update_original)},
            {0xD1C2B0,reinterpret_cast<void*>(&ObserveLocationStepFinish),reinterpret_cast<void**>(&location_step_finish_original)},
            {0x4AD510,reinterpret_cast<void*>(&ObserveSoundEvent),reinterpret_cast<void**>(&sound_event_original)},
            {0xC4D160,reinterpret_cast<void*>(&ObservePartMeshMask),reinterpret_cast<void**>(&part_mesh_mask_original)},
            {0xD3DA20,reinterpret_cast<void*>(&ObserveMenuItemDraw),reinterpret_cast<void**>(&menu_item_draw_original)},
            {0x1ADE670,reinterpret_cast<void*>(&ObserveMovieFactory),reinterpret_cast<void**>(&movie_factory_original)},
            {0x1ADEAF0,reinterpret_cast<void*>(&ObserveMovieRender),reinterpret_cast<void**>(&movie_render_original)},
            {0x1ADE850,reinterpret_cast<void*>(&ObserveMovieClose),reinterpret_cast<void**>(&movie_close_original)},
            // 1186 velocity witnesses (pass-through).
            {0x102380,reinterpret_cast<void*>(&ObserveVelocityPass),reinterpret_cast<void**>(&velocity_pass_original)},
            {0x19CA80,reinterpret_cast<void*>(&ObserveLightmapBind),reinterpret_cast<void**>(&lightmap_bind_original)},   // lightmap floors
            {0x22E930,reinterpret_cast<void*>(&ObserveMatDynUpdate),reinterpret_cast<void**>(&matdyn_update_original)},   // m50 sky
            {0x64610,reinterpret_cast<void*>(&ObserveGeometryDraw),reinterpret_cast<void**>(&geometry_draw_original)},   // phantoms: cull rows
            {0x13BFC0,reinterpret_cast<void*>(&ObserveCameraVelocityRecord),reinterpret_cast<void**>(&camera_velocity_record_original)},
            {0x13D810,reinterpret_cast<void*>(&ObserveCameraVelocityDraw),reinterpret_cast<void**>(&camera_velocity_draw_original)},
            {0xD079C0,reinterpret_cast<void*>(&ObserveDrawEntityEmit),reinterpret_cast<void**>(&draw_entity_emit_original)},
            {0x16050,reinterpret_cast<void*>(&ObservePoseRingUpdate),reinterpret_cast<void**>(&pose_ring_update_original)},
            {0x57A20,reinterpret_cast<void*>(&ObserveModelRows),reinterpret_cast<void**>(&model_rows_original)},
            {0x5A120,reinterpret_cast<void*>(&ObserveRowPreparation),reinterpret_cast<void**>(&row_preparation_original)}};
        for(const auto& h:character_updates)if(MH_CreateHook(reinterpret_cast<void*>(image+h.rva),h.entry,h.original)!=MH_OK){
            InterlockedExchange(&control->ready,-11);return 13;}
        const Hook image_hooks[]{
            {0x1BCE9C0,reinterpret_cast<void*>(&ObserveCpuRawImage),reinterpret_cast<void**>(&cpu_raw_image_original)},
            {0x1BD1310,reinterpret_cast<void*>(&ObserveCpuImageDecode),reinterpret_cast<void**>(&cpu_image_decode_original)},
            {0x1F85430,reinterpret_cast<void*>(&ObserveCpuImageUpload),reinterpret_cast<void**>(&cpu_image_delivery_original)},
            {0x1F84DA0,reinterpret_cast<void*>(&ObserveCpuImageUpdate),reinterpret_cast<void**>(&cpu_image_update_original)}};
        for(const auto& h:image_hooks)if(MH_CreateHook(reinterpret_cast<void*>(image+h.rva),h.entry,h.original)!=MH_OK) {
            InterlockedExchange(&control->ready,-11);return 13;
        }
        // I-0100 tranche 5: the map objects' pose hand-over (ObserveObjectSync), with popin-observe.
        if(PopinObserve() && MH_CreateHook(reinterpret_cast<void*>(image+0x66BBB0),reinterpret_cast<void*>(&ObserveObjectSync),
               reinterpret_cast<void**>(&object_sync_original))!=MH_OK){InterlockedExchange(&control->ready,-11);return 13;}
        // I-0100 tranche 3: the public pose update counted by receiver and distance (PopinPoseRate), with popin-observe and pose-observe.
        if(PopinObserve() && ModuleFlag(L"pose-observe.flag") && MH_CreateHook(reinterpret_cast<void*>(image+0x16000),reinterpret_cast<void*>(&ObservePosePublic),
               reinterpret_cast<void**>(&pose_public_original))!=MH_OK){InterlockedExchange(&control->ready,-11);return 13;}
        // D-0180 C4: the sun block builder observer, only with light-observe.flag.
        if(ModuleFlag(L"light-observe.flag") && (MH_CreateHook(reinterpret_cast<void*>(image+0x1496D0),reinterpret_cast<void*>(&ObserveSunBlock),
               reinterpret_cast<void**>(&sun_block_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x5B3D0),reinterpret_cast<void*>(&ObserveInstanceRecord),
               reinterpret_cast<void**>(&instance_record_original))!=MH_OK ||
               MH_CreateHook(reinterpret_cast<void*>(image+0x75B30),reinterpret_cast<void*>(&ObserveForwardIblBind),
               reinterpret_cast<void**>(&forward_ibl_bind_original))!=MH_OK ||   // I-0113 tranche 10: the forward rows' IBL binder
           MH_CreateHook(reinterpret_cast<void*>(image+0xD0C2F0),reinterpret_cast<void*>(&ObserveMenuRegionAdd),
               reinterpret_cast<void**>(&menu_region_add_original))!=MH_OK ||   // I-0140 tranche 28: the menu views' region adder (D-0301's witness)
           MH_CreateHook(reinterpret_cast<void*>(image+0x67070),reinterpret_cast<void*>(&ObserveRegionSelect),
               reinterpret_cast<void**>(&region_select_original))!=MH_OK ||   // and the region pass's selection (this block is plain Reference's)
           MH_CreateHook(reinterpret_cast<void*>(image+0x5C0E0),reinterpret_cast<void*>(&ObserveForwardRowSubmit),
               reinterpret_cast<void**>(&forward_row_submit_observed_original))!=MH_OK ||   // tranche 10c: 5C0E0's position input
           MH_CreateHook(reinterpret_cast<void*>(image+0x4E2A0),reinterpret_cast<void*>(&ObserveRegionKeys),
               reinterpret_cast<void**>(&region_keys_observed_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0xD04B90),reinterpret_cast<void*>(&ObserveDispEmit),reinterpret_cast<void**>(&disp_emit_original))!=MH_OK)){InterlockedExchange(&control->ready,-11);return 13;}
        // Cascade observers: the keep test and the cascade record, and the map piece emit unless light-observe.flag holds it,
        // only with cascade-observe.flag.
        if(ModuleFlag(L"cascade-observe.flag") && !ModuleFlag(L"light-observe.flag") &&
           MH_CreateHook(reinterpret_cast<void*>(image+0xD04B90),reinterpret_cast<void*>(&ObserveDispEmit),reinterpret_cast<void**>(&disp_emit_original))!=MH_OK){
            InterlockedExchange(&control->ready,-11);return 13;}
        if(ModuleFlag(L"cascade-observe.flag")){void* entries[7]{};   // the atlas draws (d3d11.dll's draw entries) and the vertex programmes
            if(!ReferenceAtlasDrawEntries(entries[0],entries[1],entries[2],entries[3],entries[4],entries[5],entries[6]) || !entries[6] ||
               MH_CreateHook(entries[6],reinterpret_cast<void*>(&ObserveCreateVertexShader),reinterpret_cast<void**>(&device_create_vs_original))!=MH_OK ||
               MH_CreateHook(entries[0],reinterpret_cast<void*>(&ObserveContextDrawIndexed),reinterpret_cast<void**>(&context_draw_indexed_original))!=MH_OK ||
               MH_CreateHook(entries[1],reinterpret_cast<void*>(&ObserveContextDraw),reinterpret_cast<void**>(&context_draw_original))!=MH_OK ||
               MH_CreateHook(entries[2],reinterpret_cast<void*>(&ObserveContextDrawIndexedInstanced),reinterpret_cast<void**>(&context_draw_indexed_instanced_original))!=MH_OK ||
               (entries[3] && MH_CreateHook(entries[3],reinterpret_cast<void*>(&ObserveDeferredDrawIndexed),reinterpret_cast<void**>(&deferred_draw_indexed_original))!=MH_OK) ||
               (entries[4] && MH_CreateHook(entries[4],reinterpret_cast<void*>(&ObserveDeferredDraw),reinterpret_cast<void**>(&deferred_draw_original))!=MH_OK) ||
               (entries[5] && MH_CreateHook(entries[5],reinterpret_cast<void*>(&ObserveDeferredDrawIndexedInstanced),reinterpret_cast<void**>(&deferred_draw_indexed_instanced_original))!=MH_OK)){
                Log("{\"event\":\"reference_atlas_draw_hooks\",\"failed\":true}\n");}   // diagnostic only: the run goes on without them
            else{char row[160]{};sprintf_s(row,"{\"event\":\"reference_atlas_draw_hooks\",\"deferred\":[%d,%d,%d]}\n",entries[3]?1:0,entries[4]?1:0,entries[5]?1:0);Log(row);}}
        if(ModuleFlag(L"cascade-observe.flag") && MH_CreateHook(reinterpret_cast<void*>(image+0x63CB0),reinterpret_cast<void*>(&ObserveRowExecute),
               reinterpret_cast<void**>(&row_execute_original))!=MH_OK){InterlockedExchange(&control->ready,-11);return 13;}   // the depth rows' models
        if(ModuleFlag(L"cascade-observe.flag")){uintptr_t admit{};
            if(!Read(image+0x3D52988+0x50,&admit,8) || admit<image || admit>=image+0x4000000 ||
               MH_CreateHook(reinterpret_cast<void*>(admit),reinterpret_cast<void*>(&ObservePassAdmit),reinterpret_cast<void**>(&pass_admit_original))!=MH_OK){
                InterlockedExchange(&control->ready,-11);return 13;}}
        if(ModuleFlag(L"cascade-observe.flag") && (MH_CreateHook(reinterpret_cast<void*>(image+0x1B5B40),reinterpret_cast<void*>(&ObserveCascadeKeep),
               reinterpret_cast<void**>(&cascade_keep_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x1B6020),reinterpret_cast<void*>(&ObserveCascadeRecord),reinterpret_cast<void**>(&cascade_record_original))!=MH_OK)){
            InterlockedExchange(&control->ready,-11);return 13;}
        // W5a SL4: the cascade-fit observer, only with shadow-observe.flag.
        if(ModuleFlag(L"shadow-observe.flag") && (MH_CreateHook(reinterpret_cast<void*>(image+0x239610),reinterpret_cast<void*>(&ObserveCascadeFit),
               reinterpret_cast<void**>(&cascade_fit_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x13AA60),reinterpret_cast<void*>(&ObserveSceneImage),reinterpret_cast<void**>(&scene_image_upload_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x1D6D20),reinterpret_cast<void*>(&ObservePerScene),reinterpret_cast<void**>(&per_scene_build_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x212C40),reinterpret_cast<void*>(&ObservePointShadowSlot),reinterpret_cast<void**>(&point_shadow_slot_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x214350),reinterpret_cast<void*>(&ObserveSpotShadowSlot),reinterpret_cast<void**>(&spot_shadow_slot_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x10A220),reinterpret_cast<void*>(&ObserveLightRecording),reinterpret_cast<void**>(&light_recording_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0xE006A0),reinterpret_cast<void*>(&ObserveSceneCpuUpdate),reinterpret_cast<void**>(&scene_cpu_update_original))!=MH_OK ||   // Fable review: the scene holders' keys
           MH_CreateHook(reinterpret_cast<void*>(image+0x222FF0),reinterpret_cast<void*>(&ObserveLightVisitor),reinterpret_cast<void**>(&light_visitor_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x1D7850),reinterpret_cast<void*>(&ObserveMaskBlur),reinterpret_cast<void**>(&mask_blur_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x1BB810),reinterpret_cast<void*>(&ObserveGlowFilter),reinterpret_cast<void**>(&glow_filter_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x1B26E0),reinterpret_cast<void*>(&ObserveAoPasses),reinterpret_cast<void**>(&ao_passes_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x49D00),reinterpret_cast<void*>(&ObserveGparamBind),reinterpret_cast<void**>(&gparam_bind_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0xD275F0),reinterpret_cast<void*>(&ObserveGparamFileBind),reinterpret_cast<void**>(&gparam_file_bind_original))!=MH_OK)){
            InterlockedExchange(&control->ready,-11);return 13;}
    }
    // Menu tests FL5-FL7: the Steam language id, only with test-language.flag, in both modes so Reference and Native runs compare
    // (it sat in the Reference block until Native2049, whose menus stayed English).
    if(!TestLanguageName().empty() && MH_CreateHook(reinterpret_cast<void*>(image+0xEF3720),reinterpret_cast<void*>(&NativeTestSteamLanguage),
           reinterpret_cast<void**>(&steam_language_original))!=MH_OK){InterlockedExchange(&control->ready,-11);return 13;}
    // I-0141 tranche 2 (light-observe.flag, both modes): the cutscene act's update and its overrides in the light and fog banks,
    // pass-through, rows only. They sat in the Reference block (the merge's reviews: a native run observed nothing,
    // i141-t2b-c32000010-nat). Diagnostic only: a failed hook is a row and the run goes on; the row is also the proof, in a run's
    // journal, that the observers were there (a run without rows of theirs says nothing otherwise): one word a hook (the step
    // review of the merge fixes, L1). The original-probe block above returns before this part: it keeps its own four, without
    // the act's observer and without this row. I-0141 tranche 3: natively the four bank functions are the native list's own hooks
    // and the act's update (NativeRemoLightOverride and its siblings, NativeRemoDrawParamAct, 18_resource_admission.inc: the overrides
    // given to Banks, these rows written by them under the same flag; a failed one stops the start), so here only in the Reference;
    // natively the row says true for them before the native list makes them (the step review's L11).
    // I-0140 tranche 29's witness (light-observe.flag, both modes): the env-map bank's readers by call site (04c's env_bank_witness):
    // 416A0 in both modes, 41990 in the Reference (natively the native list installs NativeEnvBankHasObserved, which notes its own calls).
    // Diagnostic only: a failed hook is a row and the run goes on (the original-probe block returns before this part).
    if(env_bank_witness::Enabled()){
        const bool reference=control->mode!=bootstrap::Mode::NativeBootstrap;
        const bool has=!reference || (MH_CreateHook(reinterpret_cast<void*>(image+0x41990),reinterpret_cast<void*>(&ObserveEnvBankHas),reinterpret_cast<void**>(&env_bank_has_original))==MH_OK &&
            MH_EnableHook(reinterpret_cast<void*>(image+0x41990))==MH_OK);
        const bool get=MH_CreateHook(reinterpret_cast<void*>(image+0x416A0),reinterpret_cast<void*>(&ObserveEnvBankGet),reinterpret_cast<void**>(&env_bank_get_original))==MH_OK &&
            MH_EnableHook(reinterpret_cast<void*>(image+0x416A0))==MH_OK;
        char row[128]{};sprintf_s(row,"{\"event\":\"env_bank_observer\",\"has_41990\":%s,\"get_416A0\":%s}\n",has?"true":"false",get?"true":"false");Log(row);}
    // I-0140 tranche 8's witness (effect-fog-observe.flag): the original's asks of the engine's fog bank (07_scene_lighting's
    // ObserveEngineFogLookup); natively the detour NativeEngineFogLookup writes the same rows.
    if(ModuleFlag(L"effect-fog-observe.flag") && control->mode!=bootstrap::Mode::NativeBootstrap){
        const bool hooked=MH_CreateHook(reinterpret_cast<void*>(image+0x38E70),reinterpret_cast<void*>(&ObserveEngineFogLookup),reinterpret_cast<void**>(&engine_fog_lookup_original))==MH_OK &&
            MH_EnableHook(reinterpret_cast<void*>(image+0x38E70))==MH_OK;
        Log(hooked?"{\"event\":\"effect_fog_observer\",\"hooked\":true}\n":"{\"event\":\"effect_fog_observer\",\"hooked\":false}\n");}
    // I-0141 tranche 6's witness (effect-light-observe.flag): the original's asks of the engine's light bank (07_scene_lighting's
    // ObserveEngineLightLookup); natively the detour NativeEngineLightLookup writes the same rows (none with no-effect-light-banks.flag,
    // which leaves 32770 unhooked; none in an original-probe run, whose block returns before this part). The row below says which.
    if(ModuleFlag(L"effect-light-observe.flag") && control->mode!=bootstrap::Mode::NativeBootstrap){
        const bool hooked=MH_CreateHook(reinterpret_cast<void*>(image+0x32770),reinterpret_cast<void*>(&ObserveEngineLightLookup),reinterpret_cast<void**>(&engine_light_lookup_original))==MH_OK &&
            MH_EnableHook(reinterpret_cast<void*>(image+0x32770))==MH_OK;
        Log(hooked?"{\"event\":\"effect_light_observer\",\"hooked\":true}\n":"{\"event\":\"effect_light_observer\",\"hooked\":false}\n");}
    // I-0141: the library's shaft keeper in the original (shaft-observe.flag; 07_scene_lighting's ObserveShaftKeeper).
    if(ModuleFlag(L"shaft-observe.flag") && control->mode!=bootstrap::Mode::NativeBootstrap){
        const bool hooked=MH_CreateHook(reinterpret_cast<void*>(image+0x16EA000),reinterpret_cast<void*>(&ObserveShaftKeeper),reinterpret_cast<void**>(&shaft_keeper_original))==MH_OK &&
            MH_EnableHook(reinterpret_cast<void*>(image+0x16EA000))==MH_OK;
        Log(hooked?"{\"event\":\"shaft_observer\",\"hooked\":true}\n":"{\"event\":\"shaft_observer\",\"hooked\":false}\n");
        // I-0141: and the DoF record's blend (07 ObserveDofBlend).
        const bool blend=MH_CreateHook(reinterpret_cast<void*>(image+0x21E720),reinterpret_cast<void*>(&ObserveDofBlend),reinterpret_cast<void**>(&dof_blend_original))==MH_OK &&
            MH_EnableHook(reinterpret_cast<void*>(image+0x21E720))==MH_OK;
        Log(blend?"{\"event\":\"dof_blend_observer\",\"hooked\":true}\n":"{\"event\":\"dof_blend_observer\",\"hooked\":false}\n");
        // I-0138 tranche 8h: the post feed's keys at FB890's entry (07 ObservePostFeed).
        const bool feed=MH_CreateHook(reinterpret_cast<void*>(image+0xFB890),reinterpret_cast<void*>(&ObservePostFeed),reinterpret_cast<void**>(&post_feed_original))==MH_OK &&
            MH_EnableHook(reinterpret_cast<void*>(image+0xFB890))==MH_OK;
        Log(feed?"{\"event\":\"post_feed_observer\",\"hooked\":true}\n":"{\"event\":\"post_feed_observer\",\"hooked\":false}\n");}
    // I-0138 tranche 8h: the keys' writer AreaControllerSet_PerFrameApply_140d52b70, in both modes (shaft-observe.flag; 07
    // ObserveAreaSetApply): natively UpdateNativeSceneCpu calls its entry, so the detour sees that call too.
    if(ModuleFlag(L"shaft-observe.flag")){
        const bool hooked=MH_CreateHook(reinterpret_cast<void*>(image+0xD52B70),reinterpret_cast<void*>(&ObserveAreaSetApply),reinterpret_cast<void**>(&area_set_apply_original))==MH_OK &&
            MH_EnableHook(reinterpret_cast<void*>(image+0xD52B70))==MH_OK;
        Log(hooked?"{\"event\":\"area_set_apply_observer\",\"hooked\":true}\n":"{\"event\":\"area_set_apply_observer\",\"hooked\":false}\n");}
    // I-0141: the cbFog blocks 14E900 builds, in both modes (fog-build-observe.flag; 07_scene_lighting's ObserveFogBlockBuild).
    if(ModuleFlag(L"fog-build-observe.flag")){
        const bool hooked=MH_CreateHook(reinterpret_cast<void*>(image+0x14E900),reinterpret_cast<void*>(&ObserveFogBlockBuild),reinterpret_cast<void**>(&fog_block_build_original))==MH_OK &&
            MH_EnableHook(reinterpret_cast<void*>(image+0x14E900))==MH_OK;
        Log(hooked?"{\"event\":\"fog_build_observer\",\"hooked\":true}\n":"{\"event\":\"fog_build_observer\",\"hooked\":false}\n");}
    if(ModuleFlag(L"light-observe.flag")){const bool native=control->mode==bootstrap::Mode::NativeBootstrap;
        const bool light=native || MH_CreateHook(reinterpret_cast<void*>(image+0x31A20),reinterpret_cast<void*>(&ObserveLightOverride),reinterpret_cast<void**>(&light_override_original))==MH_OK;
        const bool fog=native || MH_CreateHook(reinterpret_cast<void*>(image+0x380A0),reinterpret_cast<void*>(&ObserveFogOverride),reinterpret_cast<void**>(&fog_override_original))==MH_OK;
        const bool light_remove=native || MH_CreateHook(reinterpret_cast<void*>(image+0x320B0),reinterpret_cast<void*>(&ObserveLightOverrideRemove),reinterpret_cast<void**>(&light_override_remove_original))==MH_OK;
        const bool fog_remove=native || MH_CreateHook(reinterpret_cast<void*>(image+0x387D0),reinterpret_cast<void*>(&ObserveFogOverrideRemove),reinterpret_cast<void**>(&fog_override_remove_original))==MH_OK;
        const bool act=native || MH_CreateHook(reinterpret_cast<void*>(image+0xCC8180),reinterpret_cast<void*>(&ObserveRemoDrawParamAct),reinterpret_cast<void**>(&remo_draw_param_act_original))==MH_OK;
        const auto word=[](bool on){return on?"true":"false";};char row[200]{};
        sprintf_s(row,"{\"event\":\"remo_bank_observers\",\"light\":%s,\"fog\":%s,\"light_remove\":%s,\"fog_remove\":%s,\"act\":%s}\n",
            word(light),word(fog),word(light_remove),word(fog_remove),word(act));Log(row);}
    // Menu tests CL1/CL5 (SUBTITLES-DIALOGUE-NATIVE 6, caption-observe.flag, both modes): the dialogue queue [144763258]+0x1654..+0x1663
    // (two slots {id, 0x00FF, kind}), polled every 16 ms on a thread of its own, one row per change (at most 400).
    // Menu audit, fade plates (fade-observe.flag, both modes): SprjFadeImp [144791A38], its system +8, the composite +0x20..0x2C and plates
    // 0..6 (+0x10 + 8i: current RGBA +0x10..+0x1C, remaining +0x38, flags +0x40), polled every 16 ms, one row per change (at most 600).
    if(ModuleFlag(L"fade-observe.flag"))std::thread([]{std::string last;int rows{};
        while(rows<600 && !native_process_terminating.load(std::memory_order_acquire)){Sleep(16);uintptr_t fade{},system{};std::array<float,4> c{};
            if(!Read(image+0x4791A38,&fade,8) || !fade || !Read(fade+8,&system,8) || !system){if(last!="none"){last="none";Log("{\"event\":\"fade_observe\",\"system\":false}\n");++rows;}continue;}
            Read(system+0x20,c.data(),16);char head[160]{};sprintf_s(head,"{\"event\":\"fade_observe\",\"composite\":[%.3f,%.3f,%.3f,%.3f],\"plates\":[",c[0],c[1],c[2],c[3]);std::string row=head;
            for(uint32_t i=0;i<7;++i){uintptr_t plate{};std::array<float,4> p{};float remaining{};uint32_t flags{};
                if(Read(fade+0x10+8ull*i,&plate,8) && plate){Read(plate+0x10,p.data(),16);Read(plate+0x38,&remaining,4);Read(plate+0x40,&flags,4);}
                char cell[96]{};sprintf_s(cell,"%s[%.2f,%.2f,%u]",i?",":"",p[3],remaining,flags);row+=cell;}
            row+="]}\n";if(row==last)continue;last=row;++rows;Log(row.c_str());}}).detach();
    if(ModuleFlag(L"caption-observe.flag"))std::thread([]{std::array<uint8_t,16> last{};int rows{};
        while(rows<400 && !native_process_terminating.load(std::memory_order_acquire)){Sleep(16);uintptr_t menus{};std::array<uint8_t,16> now{};
            if(!Read(image+0x4763258,&menus,8) || !menus || !Read(menus+0x1654,now.data(),16) || now==last)continue;last=now;++rows;
            int32_t a{},b{};uint16_t fa{},fb{};uint8_t ka{},kb{};std::memcpy(&a,now.data(),4);std::memcpy(&fa,now.data()+4,2);ka=now[6];
            std::memcpy(&b,now.data()+8,4);std::memcpy(&fb,now.data()+12,2);kb=now[14];
            char row[200]{};sprintf_s(row,"{\"event\":\"caption_queue\",\"slot0\":[%d,%u,%u],\"slot1\":[%d,%u,%u]}\n",a,fa,ka,b,fb,kb);Log(row);}}).detach();
    if(control->mode==bootstrap::Mode::NativeBootstrap) {
        const Hook guards[]{ {0x192D650,reinterpret_cast<void*>(&LegacyDevice),nullptr},
            {0x50720,reinterpret_cast<void*>(&ConstructNativeDecalCpuRegistry),nullptr},
            {0x505E0,reinterpret_cast<void*>(&RetireNativeDecalCpuRegistry),reinterpret_cast<void**>(&decal_retire_original)},
            {0x51BF0,reinterpret_cast<void*>(&RequestNativeDecalCpu),reinterpret_cast<void**>(&decal_request_original)},
            {0x65D60,reinterpret_cast<void*>(&RequestNativeEnvRegionCpu),reinterpret_cast<void**>(&env_region_request_original)},
            {0x652D0,reinterpret_cast<void*>(&GetNativeEnvRegionCpu),nullptr},
            {0x65320,reinterpret_cast<void*>(&SetNativeEnvRegionCpuMetadata),nullptr},
            {0x652A0,reinterpret_cast<void*>(&DestroyNativeEnvRegionCpuWrapper),reinterpret_cast<void**>(&env_region_destroy_original)},
            {0x664D0,reinterpret_cast<void*>(&ReleaseNativeEnvRegionCpu),reinterpret_cast<void**>(&env_region_release_original)},   // I-0140 tranche 28: counted for the close
            {0x66640,reinterpret_cast<void*>(&ClearNativeEnvRegionCpu),reinterpret_cast<void**>(&env_region_clear_original)},   // I-0140 tranche 28: counted, a no-op after the close
            {0xC1FB0,reinterpret_cast<void*>(&QueryNativeDecalCpuWorld),reinterpret_cast<void**>(&decal_world_query_original)},
            {0x19B690,reinterpret_cast<void*>(&UnownedNativeDecalSceneConsumer),nullptr},
            {0x198910,reinterpret_cast<void*>(&UnownedNativeDecalSceneConsumer),nullptr},
            {0x51320,reinterpret_cast<void*>(&PrepareNativeDecalProgrammes),nullptr},
            {0x20A000,reinterpret_cast<void*>(&ConstructNativeSceneTreeCpu),nullptr},
            {0x20A200,reinterpret_cast<void*>(&RetireNativeSceneTreeCpu),nullptr},
            {0xC30D0,reinterpret_cast<void*>(&ConstructNativeSceneView),nullptr},
            {0x017E30,reinterpret_cast<void*>(&PublishNativeSceneContext),nullptr},
            {0xD1E8A0,reinterpret_cast<void*>(&PublishNativeStageTextures),nullptr},
            {0xE319B0,reinterpret_cast<void*>(&PublishNativePostTransitionIntent),nullptr},
            {0xE006A0,reinterpret_cast<void*>(&UpdateNativeSceneCpu),nullptr},
            {0xD57900,reinterpret_cast<void*>(&UpdateNativeSimulationTime),reinterpret_cast<void**>(&simulation_time_original)},
            {0xCED670,reinterpret_cast<void*>(&ProduceNativeSceneCamera),reinterpret_cast<void**>(&scene_camera_original)},
            {0xA0430,reinterpret_cast<void*>(&AdmitModelStorage),nullptr},
            {0xA0170,reinterpret_cast<void*>(&AdmitModelCpuGeometry),reinterpret_cast<void**>(&model_cpu_load_original)},
            {0xA05C0,reinterpret_cast<void*>(&RetireModelStorage),nullptr},
            {0x15490,reinterpret_cast<void*>(&InitializeNativeModelPose),nullptr},
            {0x15B10,reinterpret_cast<void*>(&RetireNativeModelPose),nullptr},
            {0x16050,reinterpret_cast<void*>(&UpdateNativeModelPoseFull),nullptr},
            {0x15FC0,reinterpret_cast<void*>(&UpdateNativeModelPoseSingle),nullptr},
            {0x16000,reinterpret_cast<void*>(&UpdateNativeModelPosePublic),nullptr},
            {0xD01610,reinterpret_cast<void*>(&ObserveNativeAsmUpdate),reinterpret_cast<void**>(&asm_update_original)},
            {0xD07710,reinterpret_cast<void*>(&ObserveNativeModelEntityUpdate),reinterpret_cast<void**>(&model_entity_update_original)},
            {0xC34420,reinterpret_cast<void*>(&ObserveChrAsmModelUpdate),reinterpret_cast<void**>(&chr_asm_model_update_original)},
            {0xC47CD0,reinterpret_cast<void*>(&ObserveSprjAsmModelUpdate),reinterpret_cast<void**>(&sprj_asm_model_update_original)},
            {0xC48180,reinterpret_cast<void*>(&ObserveAsmModelTick),reinterpret_cast<void**>(&asm_model_tick_original)},
            {0x8C1BC0,reinterpret_cast<void*>(&ObservePlayerModelUpdate),reinterpret_cast<void**>(&player_model_update_original)},
            {0x888C00,reinterpret_cast<void*>(&ObserveChrEzTaskExecute),reinterpret_cast<void**>(&chr_eztask_execute_original)},
            {0x65CAC0,reinterpret_cast<void*>(&ObserveEzTaskRegister),reinterpret_cast<void**>(&eztask_register_original)},
            {0x8CA1E0,reinterpret_cast<void*>(&ObserveChrModelRequest),reinterpret_cast<void**>(&chr_model_request_original)},
            {0x88E1F0,reinterpret_cast<void*>(&ObserveChrSchedule),reinterpret_cast<void**>(&chr_schedule_original)},
            {0x8DD2E0,reinterpret_cast<void*>(&ObserveChrManagerSchedule),reinterpret_cast<void**>(&chr_manager_schedule_original)},
            {0xCD5D80,reinterpret_cast<void*>(&ObserveRemoPlayWait),reinterpret_cast<void**>(&remo_play_wait_original)},   // I-0138 t8g: the Remo clock
            {0x8FA3E0,reinterpret_cast<void*>(&ObserveWorldUpdateGate),reinterpret_cast<void**>(&world_update_gate_original)},
            {0xD2D040,reinterpret_cast<void*>(&ObservePartPoseUpdate),reinterpret_cast<void**>(&part_pose_update_original)},
            {0xD3F7C0,reinterpret_cast<void*>(&InitializeNativeModelStreams),nullptr},
            {0xD3F710,reinterpret_cast<void*>(&RetireNativeModelStreams),nullptr},
            {0x51B940,reinterpret_cast<void*>(&UpdateNativePhysicalModelStreams),nullptr},
            {0x835820,reinterpret_cast<void*>(&InitializeNativeFaceImage),nullptr},
            {0x8362C0,reinterpret_cast<void*>(&UpdateNativeFaceImage),reinterpret_cast<void**>(&face_cpu_update_original)},
            {0x835BD0,reinterpret_cast<void*>(&RetireNativeFaceImage),nullptr},
            {0x836E40,reinterpret_cast<void*>(&ComposeNativeFaceImage),nullptr},
            {0x600A0,reinterpret_cast<void*>(&NativeModelShaderTagPredicate),nullptr},
            {0xD573B0,reinterpret_cast<void*>(&LegacyResources),nullptr},
            {0xD5AA40,reinterpret_cast<void*>(&NativeSfxStartup),nullptr},
            {0x23410,reinterpret_cast<void*>(&NativeSceneRootFrame),nullptr},
            {0xE6EA10,reinterpret_cast<void*>(&NativeSfxViewAttach),nullptr},
            {0xE6EC60,reinterpret_cast<void*>(&NativeSfxViewDetach),nullptr},
            {0xD5AAC0,reinterpret_cast<void*>(&ObserveNativeCpuWait),reinterpret_cast<void**>(&cpu_wait_original)},
            {0xE50820,reinterpret_cast<void*>(&ObserveUiTaskProgress),reinterpret_cast<void**>(&ui_task_progress_original)},
            {0xE4FB30,reinterpret_cast<void*>(&NativeUiTaskDispatch),reinterpret_cast<void**>(&ui_task_dispatch_original)},
            {0xE53880,reinterpret_cast<void*>(&NativeUiMovieViewport),nullptr},
            {0xA60700,reinterpret_cast<void*>(&NativeUiPointerState),nullptr},
            {0x23891E0,reinterpret_cast<void*>(&NativeMouseDeviceUpdate),reinterpret_cast<void**>(&mouse_device_update_original)},
            {0x183C540,reinterpret_cast<void*>(&NativeDirectInputMousePoll),reinterpret_cast<void**>(&native_di_mouse_poll_original)},
            {0x183BE70,reinterpret_cast<void*>(&NativeDirectInputKeyboardPoll),reinterpret_cast<void**>(&native_di_keyboard_poll_original)},
            {0x23B7960,reinterpret_cast<void*>(&NativeDynamicMeshBufferSet),reinterpret_cast<void**>(&dynamic_mesh_buffer_set_original)},
            {0x23D5BC0,reinterpret_cast<void*>(&NativeClothVertexUpload),reinterpret_cast<void**>(&cloth_vertex_upload_original)},
            {0x23B7280,reinterpret_cast<void*>(&NativeClothSetRelease),reinterpret_cast<void**>(&cloth_set_release_original)},
            {0x1AC840,reinterpret_cast<void*>(&NativeObjectProbeLightBlend),reinterpret_cast<void**>(&object_probe_light_blend_original)},
            {0x61950,reinterpret_cast<void*>(&NativeShRecordSet),reinterpret_cast<void**>(&sh_record_set_original)},
            {0x2D860,reinterpret_cast<void*>(&NativeDrawableRegistration),reinterpret_cast<void**>(&drawable_registration_original)},
            {0xADEF50,reinterpret_cast<void*>(&NativeLoadGameSummaryPortraits),nullptr},
            {0xB4E0,reinterpret_cast<void*>(&NativeGxDeviceSync),nullptr},
            // 1250 native Bink player (I-0068 part J): steps, player open/close/render, graphics init/planes/draw, publish.
            {0xEAF7E0,reinterpret_cast<void*>(&NativeMovieStepInit),nullptr},{0xEAFAC0,reinterpret_cast<void*>(&NativeMovieStepPlay),nullptr},
            {0x1ADF230,reinterpret_cast<void*>(&NativeMoviePlayerOpen),nullptr},{0x1ADE850,reinterpret_cast<void*>(&NativeMoviePlayerClose),nullptr},
            {0x1ADEAF0,reinterpret_cast<void*>(&NativeMoviePlayerRender),nullptr},{0x1AE0830,reinterpret_cast<void*>(&NativeMovieGraphicsInit),nullptr},
            {0x1AE0BA0,reinterpret_cast<void*>(&NativeMovieGraphicsPlanes),nullptr},{0x1ADFF10,reinterpret_cast<void*>(&NativeMovieGraphicsDraw),nullptr},
            {0xEADE50,reinterpret_cast<void*>(&NativeMoviePublish),nullptr},
            {0xD64C30,reinterpret_cast<void*>(&NativeOffscreenSceneGraph),nullptr},
            {0xD65410,reinterpret_cast<void*>(&NativeOffscreenRekey),nullptr},   // I-0140 tranche 30: a view's re-key (the creation's close-up)
            // I-0140 tranche 28 (D-0301): the menu views' region, D0C2F0's key gate answered from the native banks (its witness row
            // menu_region_add with light-observe.flag).
            {0xD0C2F0,reinterpret_cast<void*>(&NativeMenuRegionAdd),reinterpret_cast<void**>(&menu_region_add_original)},
            {0x41990,env_bank_witness::Enabled()?reinterpret_cast<void*>(&NativeEnvBankHasObserved):reinterpret_cast<void*>(&NativeEnvBankHas),reinterpret_cast<void**>(&env_bank_has_original)},
            {0xD0C990,reinterpret_cast<void*>(&NativeMenuRegionRelease),nullptr},   // its release (the renderer destructor's)
            {0xC3B810,reinterpret_cast<void*>(&NativeOffscreenModelBind),reinterpret_cast<void**>(&offscreen_model_bind_original)},
            {0xC3AD90,reinterpret_cast<void*>(&NativeOffscreenPartsAttach),reinterpret_cast<void**>(&offscreen_parts_attach_original)},
            {0xD64B50,reinterpret_cast<void*>(&NativeOffscreenList),reinterpret_cast<void**>(&offscreen_list_original)},
            {0xD65140,reinterpret_cast<void*>(&NativeOffscreenUnlist),reinterpret_cast<void**>(&offscreen_unlist_original)},
            {0xD66280,reinterpret_cast<void*>(&NativeOffscreenRendererRetire),reinterpret_cast<void**>(&offscreen_renderer_retire_original)},
            {0xD68EB0,reinterpret_cast<void*>(&NativeOffscreenModelRelease),reinterpret_cast<void**>(&offscreen_model_release_original)},
            {0xD123F0,reinterpret_cast<void*>(&NativeBodyScaleModifierRetire),reinterpret_cast<void**>(&body_scale_modifier_retire_original)},   // I-0137 tranche 7: always (its row behind update-observe.flag)
            {0x8190B0,reinterpret_cast<void*>(&ObserveDeferredEnqueue),reinterpret_cast<void**>(&deferred_enqueue_original)},
            {0xEE42B0,reinterpret_cast<void*>(&ObserveDeferredFlush),reinterpret_cast<void**>(&deferred_flush_original)},
            {0xAD9820,reinterpret_cast<void*>(&ObserveTitleDialogRetire),reinterpret_cast<void**>(&title_dialog_retire_original)},
            {0xA4E7C0,reinterpret_cast<void*>(&ObserveWindowClose),reinterpret_cast<void**>(&window_close_original)},
            {0x90A4A0,reinterpret_cast<void*>(&ObserveTitleStepRunA),reinterpret_cast<void**>(&title_step_run_a_original)},
            {0x90A520,reinterpret_cast<void*>(&ObserveTitleStepRunB),reinterpret_cast<void**>(&title_step_run_b_original)},
            {0x909EB0,reinterpret_cast<void*>(&ObserveTitleStepRetire),reinterpret_cast<void**>(&title_step_retire_original)},
            {0xD126B0,reinterpret_cast<void*>(&NativeBodyScaleUpdate),reinterpret_cast<void**>(&body_scale_update_original)},   // I-0137: its guard, with the witness behind update-observe.flag
            {0xD12370,reinterpret_cast<void*>(&NativeBodyScaleModifierConstruct),reinterpret_cast<void**>(&body_scale_modifier_construct_original)},   // I-0137 tranche 7: the guard's blocks
            {0xD23910,reinterpret_cast<void*>(&NativeFlverResCapRelease),reinterpret_cast<void**>(&flver_res_cap_release_original)},
            {0xD12930,reinterpret_cast<void*>(&NativeBodyScaleSlot4),reinterpret_cast<void**>(&body_scale_slot4_original)},
            {0xD68520,reinterpret_cast<void*>(&NativeMenuModelUpdate),reinterpret_cast<void**>(&menu_model_update_original)},   // I-0137 tranche 8
            {0xD124B0,reinterpret_cast<void*>(&NativeBodyScaleSlot5),reinterpret_cast<void**>(&body_scale_slot5_original)},
            {0xD12AA0,reinterpret_cast<void*>(&NativeBodyScaleSlot6),reinterpret_cast<void**>(&body_scale_slot6_original)},
            {0xD1C2B0,reinterpret_cast<void*>(&ObserveLocationStepFinish),reinterpret_cast<void**>(&location_step_finish_original)},
            {0x4AD510,reinterpret_cast<void*>(&ObserveSoundEvent),reinterpret_cast<void**>(&sound_event_original)},
            {0xC4D160,reinterpret_cast<void*>(&ObservePartMeshMask),reinterpret_cast<void**>(&part_mesh_mask_original)},
            {0xD3DA20,reinterpret_cast<void*>(&ObserveMenuItemDraw),reinterpret_cast<void**>(&menu_item_draw_original)},
            {0xE500C0,reinterpret_cast<void*>(&NativeUiStartupTask),nullptr},
            {0xE506C0,reinterpret_cast<void*>(&WaitUiCpuConsumers),reinterpret_cast<void**>(&ui_consumers_original)},
            {0xEDF460,reinterpret_cast<void*>(&NativeMainGraphicsWait),nullptr},
            {0xEDF140,reinterpret_cast<void*>(&NativeMainServicesStep),nullptr}, {0x192C220,reinterpret_cast<void*>(&LegacySwap),nullptr},
            {0x192CF00,reinterpret_cast<void*>(&LegacyPresent),nullptr},
            {0xD55190,reinterpret_cast<void*>(&NativeRenderStep),nullptr},
            {0xD56780,reinterpret_cast<void*>(&BeginFrame),nullptr},
            {0xD56930,reinterpret_cast<void*>(&EndFrame),nullptr},
            {0x6D700,reinterpret_cast<void*>(&AdmitShader),nullptr},
            {0x6D8F0,reinterpret_cast<void*>(&AdmitShader),nullptr},
            {0x6DAE0,reinterpret_cast<void*>(&AdmitShader),nullptr},
            {0x6DCF0,reinterpret_cast<void*>(&AdmitShader),nullptr},
            {0x6DFD0,reinterpret_cast<void*>(&AdmitShader),nullptr},
            {0x6E230,reinterpret_cast<void*>(&AdmitShader),nullptr},
            {0x6D680,reinterpret_cast<void*>(&RetireShader),nullptr},
            {0x6D870,reinterpret_cast<void*>(&RetireShader),nullptr},
            {0x6DA60,reinterpret_cast<void*>(&RetireShader),nullptr},
            {0x6DC50,reinterpret_cast<void*>(&RetireShader),nullptr},
            {0x6DF50,reinterpret_cast<void*>(&RetireShader),nullptr},
            {0x6E180,reinterpret_cast<void*>(&RetireShader),nullptr},
            {0x2AD40,reinterpret_cast<void*>(&RetireTexture),nullptr},
            {0x2AF40,reinterpret_cast<void*>(&AdmitTexture),nullptr},
            {0xB540,reinterpret_cast<void*>(&NativeDisplayFullscreen),nullptr},   // I-0076: the GX display's screen-mode role
            {0xB590,reinterpret_cast<void*>(&NativeDisplayToggleFullscreen),nullptr},
            {0xEF54A0,reinterpret_cast<void*>(&QueueNativeDisplayRequest),reinterpret_cast<void**>(&display_request_original)},
            {0xEF46A0,reinterpret_cast<void*>(&NativeWindowStartup),reinterpret_cast<void**>(&window_startup_original)},   // borderless.flag
            {0x2B260,reinterpret_cast<void*>(&AdmitEncodedTexture),nullptr},
            {0xAC110,reinterpret_cast<void*>(&AdmitEncodedCube),nullptr},
            {0xAC020,reinterpret_cast<void*>(&RetireTexture),nullptr},
            {0xAC830,reinterpret_cast<void*>(&LegacyTextureSource),nullptr},
            {0xE0DD10,reinterpret_cast<void*>(&NativeSfxAssetLoad),nullptr},
            {0xE0DAA0,reinterpret_cast<void*>(&ConstructNativeSfxCpuRecord),reinterpret_cast<void**>(&sfx_cpu_construct_original)},
            {0xE0DB20,reinterpret_cast<void*>(&RetireNativeSfxCpuRecord),nullptr},
            {0x7CB820,reinterpret_cast<void*>(&DeferNativeSfxSceneRequest),reinterpret_cast<void**>(&sfx_world_request_original)},
            {0x19254F0,reinterpret_cast<void*>(&NativeGxBufferCreate),reinterpret_cast<void**>(&gx_buffer_create_original)},   // W4 FW-2
            {0x1925BF0,reinterpret_cast<void*>(&NativeGxShaderView),reinterpret_cast<void**>(&gx_shader_view_original)},
            {0x1925B10,reinterpret_cast<void*>(&NativeGxUnorderedView),reinterpret_cast<void**>(&gx_unordered_view_original)},
            {0xD8780,reinterpret_cast<void*>(&NativeGxDeferredRelease),reinterpret_cast<void**>(&gx_deferred_release_original)},
            {0x1925210,reinterpret_cast<void*>(&NativeGxProgramCreate),reinterpret_cast<void**>(&gx_program_create_original)},
            {0x19250A0,reinterpret_cast<void*>(&NativeGxStreamProgramCreate),reinterpret_cast<void**>(&gx_stream_program_create_original)},
            {0x24D000,reinterpret_cast<void*>(&NativeSceneCtrlUpdate),reinterpret_cast<void**>(&scene_ctrl_update_original)},   // W4 FW-3
            {0x19AD90,reinterpret_cast<void*>(&NativeGxResourceRelease),reinterpret_cast<void**>(&gx_resource_release_original)},
            {0xE49320,reinterpret_cast<void*>(&NativeUiTextureImport),nullptr},
            {0xABD90,reinterpret_cast<void*>(&TextureFormat),nullptr},
            {0xABDD0,reinterpret_cast<void*>(&TextureMips),nullptr},
            {0xABE10,reinterpret_cast<void*>(&TextureWidth),nullptr},
            {0xABE60,reinterpret_cast<void*>(&TextureHeight),nullptr},
            {0xABED0,reinterpret_cast<void*>(&TextureValid),nullptr},
            {0x2AA80,reinterpret_cast<void*>(&TextureWidth),nullptr},
            {0x2AAD0,reinterpret_cast<void*>(&TextureHeight),nullptr},
            {0x2AB40,reinterpret_cast<void*>(&TextureFormat),nullptr},
            {0x2ABA0,reinterpret_cast<void*>(&TextureMips),nullptr},
            {0x2ABE0,reinterpret_cast<void*>(&TextureValid),nullptr},
            {0xA9370,reinterpret_cast<void*>(&AdmitEffect),nullptr},
            {0xA9230,reinterpret_cast<void*>(&RetireEffect),reinterpret_cast<void**>(&effect_retire_original)},
            {0xD43BB0,reinterpret_cast<void*>(&AdmitMaterial),nullptr},
            {0xD439A0,reinterpret_cast<void*>(&RetireMaterial),reinterpret_cast<void**>(&material_retire_original)},
            {0x49D00,reinterpret_cast<void*>(&BindNativeGparamScene),nullptr},
            {0x8C68F0,reinterpret_cast<void*>(&RetireNativeZoneCpu),nullptr},
            {0xD272C0,reinterpret_cast<void*>(&AdmitGparam),nullptr},
            {0xD275F0,reinterpret_cast<void*>(&BindNativeGparamFile),nullptr},   // sun source: the area file's binding
            {0xCE6350,reinterpret_cast<void*>(&NativeAreaControllerStep),reinterpret_cast<void**>(&area_controller_step_original)},   // its unbinding
            {0xCDEE50,reinterpret_cast<void*>(&NativeRemoGparamStep),reinterpret_cast<void**>(&remo_gparam_step_original)},   // I-0140 tranche 7: a cutscene file's binding
            {0xD26800,reinterpret_cast<void*>(&RetireGparam),reinterpret_cast<void**>(&gparam_retire_original)},
            // I-0141 tranche 3: the cutscene act's overrides of the map's light and fog records, given to Banks, and their ends.
            {0x31A20,reinterpret_cast<void*>(&NativeRemoLightOverride),reinterpret_cast<void**>(&light_override_original)},
            {0x380A0,reinterpret_cast<void*>(&NativeRemoFogOverride),reinterpret_cast<void**>(&fog_override_original)},
            {0x320B0,reinterpret_cast<void*>(&NativeRemoLightOverrideRemove),reinterpret_cast<void**>(&light_override_remove_original)},
            {0x387D0,reinterpret_cast<void*>(&NativeRemoFogOverrideRemove),reinterpret_cast<void**>(&fog_override_remove_original)},
            {0xCDCCD0,reinterpret_cast<void*>(&NativeRemoMapHolderDestroy),reinterpret_cast<void**>(&remo_map_holder_destroy_original)},
            {0xCC8180,reinterpret_cast<void*>(&NativeRemoDrawParamAct),reinterpret_cast<void**>(&remo_draw_param_act_original)},
            // I-0141 tranche 4 (its final review, M2): the act's three other banks, followed in the ledger for the withdrawal.
            {0x3C900,reinterpret_cast<void*>(&NativeRemoBankWrite<2>),reinterpret_cast<void**>(&remo_bank_write_original[2])},
            {0x40DD0,reinterpret_cast<void*>(&NativeRemoBankWrite<3>),reinterpret_cast<void**>(&remo_bank_write_original[3])},
            {0x45CA0,reinterpret_cast<void*>(&NativeRemoBankWrite<4>),reinterpret_cast<void**>(&remo_bank_write_original[4])},
            {0x3D080,reinterpret_cast<void*>(&NativeRemoBankRemove<2>),reinterpret_cast<void**>(&remo_bank_remove_original[2])},
            {0x40690,reinterpret_cast<void*>(&NativeRemoBankRemove<3>),reinterpret_cast<void**>(&remo_bank_remove_original[3])},
            {0x45560,reinterpret_cast<void*>(&NativeRemoBankRemove<4>),reinterpret_cast<void**>(&remo_bank_remove_original[4])},
            // Workstream K P4b: pass-through observers of the named texture registry's mutators (generation bump after each).
            {0xD4F250,reinterpret_cast<void*>(&ObserveTextureRegistry<0>),reinterpret_cast<void**>(&texture_registry_original[0])},
            {0xD4E100,reinterpret_cast<void*>(&ObserveTextureRegistry<1>),reinterpret_cast<void**>(&texture_registry_original[1])},
            {0xD4F690,reinterpret_cast<void*>(&ObserveTextureRegistry<2>),reinterpret_cast<void**>(&texture_registry_original[2])},
            {0xD4EE10,reinterpret_cast<void*>(&ObserveTextureRegistry<3>),reinterpret_cast<void**>(&texture_registry_original[3])},
            {0xD4F4B0,reinterpret_cast<void*>(&ObserveTextureRegistry<4>),reinterpret_cast<void**>(&texture_registry_original[4])},
            {0xD4F7A0,reinterpret_cast<void*>(&ObserveTextureRegistry<5>),reinterpret_cast<void**>(&texture_registry_original[5])} };
        for(const auto& h:guards) if((update_witnesses || !witness_only(h.entry)) && MH_CreateHook(reinterpret_cast<void*>(image+h.rva),h.entry,h.original)!=MH_OK) {
            InterlockedExchange(&control->ready,-6); return 8;
        }
        // I-0140 tranche 8 (07_scene_lighting): the engine's fog lookup answered from the native banks for the effects' cbFog;
        // no-effect-fog-banks.flag leaves 38E70 unhooked (the step review's L3: the flag also spares the install).
        if(!ModuleFlag(L"no-effect-fog-banks.flag") && MH_CreateHook(reinterpret_cast<void*>(image+0x38E70),reinterpret_cast<void*>(&NativeEngineFogLookup),
               reinterpret_cast<void**>(&engine_fog_lookup_original))!=MH_OK){InterlockedExchange(&control->ready,-6); return 8;}
        // I-0141 tranche 6 (07_scene_lighting): the engine's light lookup answered from the native banks for the effects' cbLight;
        // no-effect-light-banks.flag leaves 32770 unhooked.
        if(!ModuleFlag(L"no-effect-light-banks.flag") && MH_CreateHook(reinterpret_cast<void*>(image+0x32770),reinterpret_cast<void*>(&NativeEngineLightLookup),
               reinterpret_cast<void**>(&engine_light_lookup_original))!=MH_OK){InterlockedExchange(&control->ready,-6); return 8;}
        Log(engine_light_lookup_original?"{\"event\":\"native_effect_light_detour\",\"hooked\":true}\n":"{\"event\":\"native_effect_light_detour\",\"hooked\":false}\n");
        // I-0141 tranche 9 (07_scene_lighting's NativeExternalFocus): the game camera's focus distance as the game asks it, in play and
        // in cutscenes; no flag (the game's own request, without which the DoF record's distance stands).
        if(MH_CreateHook(reinterpret_cast<void*>(image+0x17EA0),reinterpret_cast<void*>(&NativeExternalFocus),reinterpret_cast<void**>(&external_focus_original))!=MH_OK){
            InterlockedExchange(&control->ready,-6); return 8;}
        // I-0099 (FIX-PLAN §2): DS3's forgotten stop of its network thread, at ~DLNRD::SocketManager (15_cpu_service_and_input);
        // no-exit-order-fix.flag leaves the original's quit as it is.
        if(!ModuleFlag(L"no-exit-order-fix.flag") && MH_CreateHook(reinterpret_cast<void*>(image+0x19853A0),reinterpret_cast<void*>(&StopNetworkThreadThenDestroy),
               reinterpret_cast<void**>(&socket_manager_destroy_original))!=MH_OK){InterlockedExchange(&control->ready,-6); return 8;}
        // I-0132 R12c (D-0271; 02b_rekindled_graphics.inc, R12C-STATIC §8): the game's Graphics tab and its Advanced Settings page
        // with our rows (the game's row functions, its films), our words through the menu text reader; no-rekindled-graphics.flag
        // leaves both pages as the game makes them.
        if(!ModuleFlag(L"no-rekindled-graphics.flag")){
            const Hook rekindled[]{
                {0xE1A060,reinterpret_cast<void*>(&RekindledMenuText),reinterpret_cast<void**>(&menu_text_original)},
                {0xAB07D0,reinterpret_cast<void*>(&RekindledGraphicsTab),reinterpret_cast<void**>(&graphics_rows_original)},
                {0xAAFD10,reinterpret_cast<void*>(&RekindledAdvancedPage),reinterpret_cast<void**>(&advanced_rows_original)},
                {0xAAE950,reinterpret_cast<void*>(&RekindledAdvancedDefaults),reinterpret_cast<void**>(&advanced_defaults_original)},
                {0xAAC6A0,reinterpret_cast<void*>(&RekindledBrightnessRows),reinterpret_cast<void**>(&brightness_rows_original)},   // I-0138 tranche 5b: the game's Brightness row on an HDR10 chain
                {0xA4CA90,reinterpret_cast<void*>(&RekindledWindowUpdate),reinterpret_cast<void**>(&window_update_original)}};
            for(const auto& h:rekindled)if(MH_CreateHook(reinterpret_cast<void*>(image+h.rva),h.entry,h.original)!=MH_OK){InterlockedExchange(&control->ready,-45);return 47;}
            rekindled_hooks_installed.store(true);
            Log("{\"event\":\"native_rekindled_graphics\",\"hooks\":6}\n");}
        // I-0132 R12c (R12C-STATIC §4): sound-observe.flag, the menu sounds' names (FUN_1404AD510, pass-through, the Reference's
        // ObserveSoundEvent) in a native run too, to read which sound a page's cursor, value change and cancel play.
        if(ModuleFlag(L"sound-observe.flag") && !update_witnesses && MH_CreateHook(reinterpret_cast<void*>(image+0x4AD510),reinterpret_cast<void*>(&ObserveSoundEvent),
               reinterpret_cast<void**>(&sound_event_original))!=MH_OK){InterlockedExchange(&control->ready,-46);return 48;}
    }
    // Audio census (ROADMAP, Thomas 2026-10-13): audio-census.flag hooks FMOD::Event::start in fmod_event64.dll (the FMOD Designer
    // event API the game imports for every cue: music, voices, effects, ambience) in both modes, pass-through.
    if(ModuleFlag(L"audio-census.flag")){
        const HMODULE events=GetModuleHandleW(L"fmod_event64.dll");
        const auto start=events?GetProcAddress(events,"?start@Event@FMOD@@QEAA?AW4FMOD_RESULT@@XZ"):nullptr;
        audio_event_info=events?reinterpret_cast<AudioEventInfo>(GetProcAddress(events,"?getInfo@Event@FMOD@@QEAA?AW4FMOD_RESULT@@PEAHPEAPEADPEAUFMOD_EVENT_INFO@@@Z")):nullptr;
        if(!start || !audio_event_info || MH_CreateHook(reinterpret_cast<void*>(start),reinterpret_cast<void*>(&ObserveAudioEventStart),reinterpret_cast<void**>(&audio_event_start_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x4AD2D0),reinterpret_cast<void*>(&ObserveAudioNameBuild),reinterpret_cast<void**>(&audio_name_build_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0xE72D50),reinterpret_cast<void*>(&ObserveAudioPlay),reinterpret_cast<void**>(&audio_play_original))!=MH_OK){
            InterlockedExchange(&control->ready,-24);return 26;}   // the census (fmod_event64.dll exports or the hooks)
        Log(control->mode==bootstrap::Mode::NativeBootstrap?"{\"event\":\"audio_census_mode\",\"mode\":\"native\"}\n":"{\"event\":\"audio_census_mode\",\"mode\":\"reference\"}\n");
    }
    {   // W4 FW-1 (FFX-WAVE1-READY section 6): ffx-observe.flag next to the module, pass-through counting thunks on the effect
        // door, the effect update and the wind chain in both modes; the entries the native build hooks already report by Note.
        if(ModuleFlag(L"ffx-observe.flag")){
            const bool native_mode=control->mode==bootstrap::Mode::NativeBootstrap;
            int thunks{};
            ffx_observe::Configure(image,&Read,&Log,native_mode?"native":"reference");
            for(const auto& site:ffx_observe::sites){
                if(!site.rva || (native_mode && (site.entry==ffx_observe::WorldRequest || (site.entry==ffx_observe::EntityUpdate && update_witnesses))))continue;
                void* thunk=ffx_observe::Thunk(site.entry);
                if(!thunk || MH_CreateHook(reinterpret_cast<void*>(image+site.rva),thunk,ffx_observe::Original(site.entry))!=MH_OK){
                    InterlockedExchange(&control->ready,-17);return 19;}
                ++thunks;
            }
            char row[160]{};sprintf_s(row,"{\"event\":\"native_ffx_observe\",\"mode\":\"%s\",\"thunks\":%d}\n",native_mode?"native":"reference",thunks);Log(row);}
    }
    // FFX-DRAW (on by default, no-ffx-draw.flag opts out; FFX-DRAW §2): the native pass context's recording (command stream, sort
    // blocks), the FFX texture slot 7, the GX context wrappers of the draw side and the shader handle getters. ffx-record.flag adds
    // the witnesses (the draw pass's slot 4 and the particle node update, counted).
    if(native_ffx_draw){using namespace native_ffx;
        if(MH_CreateHook(reinterpret_cast<void*>(image+0xBF4C0),reinterpret_cast<void*>(&CommandAlloc),reinterpret_cast<void**>(&command_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x89140),reinterpret_cast<void*>(&NativePipelineExposure),reinterpret_cast<void**>(&pipeline_exposure_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0xBF2D0),reinterpret_cast<void*>(&CommandCommit),reinterpret_cast<void**>(&commit_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x20E570),reinterpret_cast<void*>(&BlockAllocate),reinterpret_cast<void**>(&block_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x28F7B0),reinterpret_cast<void*>(&NativeTexture),reinterpret_cast<void**>(&texture_lookup_original))!=MH_OK){
            InterlockedExchange(&control->ready,-28);return 30;}
        if(ModuleFlag(L"ffx-record.flag") &&
           (MH_CreateHook(reinterpret_cast<void*>(image+0x264230),reinterpret_cast<void*>(&Slot27Observed),reinterpret_cast<void**>(&slot27_original))!=MH_OK ||
            MH_CreateHook(reinterpret_cast<void*>(image+0x2997E0),reinterpret_cast<void*>(&EntityOffered),reinterpret_cast<void**>(&entity_original))!=MH_OK ||
            MH_CreateHook(reinterpret_cast<void*>(image+0x24ED90),reinterpret_cast<void*>(&CreateEffectObserved),reinterpret_cast<void**>(&create_effect_original))!=MH_OK ||   // I-0141: ffx_create
            MH_CreateHook(reinterpret_cast<void*>(image+0x1ACCA20),reinterpret_cast<void*>(&SpawnObserved),reinterpret_cast<void**>(&spawn_original))!=MH_OK ||   // I-0141: ffx_spawn
            MH_CreateHook(reinterpret_cast<void*>(image+0x1AB18C0),reinterpret_cast<void*>(&StopHardObserved),reinterpret_cast<void**>(&stop_hard_original))!=MH_OK ||   // I-0141: ffx_instance_flag
            MH_CreateHook(reinterpret_cast<void*>(image+0x1AB1080),reinterpret_cast<void*>(&StopSoftObserved),reinterpret_cast<void**>(&stop_soft_original))!=MH_OK ||
            MH_CreateHook(reinterpret_cast<void*>(image+0x1AD71A0),reinterpret_cast<void*>(&State3Observed),reinterpret_cast<void**>(&state3_original))!=MH_OK ||   // I-0141 tranche 13: ffx_state3
            MH_CreateHook(reinterpret_cast<void*>(image+0x1AD6520),reinterpret_cast<void*>(&DurationObserved),reinterpret_cast<void**>(&duration_original))!=MH_OK ||
            MH_CreateHook(reinterpret_cast<void*>(image+0x1ACA640),reinterpret_cast<void*>(&MachineObserved),reinterpret_cast<void**>(&machine_original))!=MH_OK ||   // I-0141 tranche 13b: ffx_machine
            MH_CreateHook(reinterpret_cast<void*>(image+0x1AB1A60),reinterpret_cast<void*>(&ExtSetObserved),reinterpret_cast<void**>(&ext_set_original))!=MH_OK ||   // I-0141 tranche 13c: ffx_ext_set
            MH_CreateHook(reinterpret_cast<void*>(image+0x1ADD510),reinterpret_cast<void*>(&NodeUpdateObserved),reinterpret_cast<void**>(&node_update_original))!=MH_OK ||
            (ModuleFlag(L"ffx-entity-life.flag") && MH_CreateHook(reinterpret_cast<void*>(image+0x1ADDE80),reinterpret_cast<void*>(&NodeDrawObserved),reinterpret_cast<void**>(&node_draw_original))!=MH_OK) ||   // I-0141 tranche 14: ffx_newborn_draw
            MH_CreateHook(reinterpret_cast<void*>(image+0x373D00),reinterpret_cast<void*>(&TracerAppendObserved),reinterpret_cast<void**>(&tracer_append_original))!=MH_OK ||
            MH_CreateHook(reinterpret_cast<void*>(image+0x370620),reinterpret_cast<void*>(&TracerRebaseObserved),reinterpret_cast<void**>(&tracer_rebase_original))!=MH_OK ||
            MH_CreateHook(reinterpret_cast<void*>(image+0xE6B950),reinterpret_cast<void*>(&AnimatorCreateObserved),reinterpret_cast<void**>(&animator_create_original))!=MH_OK ||
            MH_CreateHook(reinterpret_cast<void*>(image+0xE6B470),reinterpret_cast<void*>(&AnimatorConstructObserved),reinterpret_cast<void**>(&animator_construct_original))!=MH_OK ||
            MH_CreateHook(reinterpret_cast<void*>(image+0xE6B9D0),reinterpret_cast<void*>(&AnimatorStepObserved),reinterpret_cast<void**>(&animator_step_original))!=MH_OK ||
            MH_CreateHook(reinterpret_cast<void*>(image+0x257660),reinterpret_cast<void*>(&AnimatorSendObserved),reinterpret_cast<void**>(&animator_send_original))!=MH_OK ||
            MH_CreateHook(reinterpret_cast<void*>(image+0x2823E0),reinterpret_cast<void*>(&PoseMessageObserved),reinterpret_cast<void**>(&pose_message_original))!=MH_OK ||
            MH_CreateHook(reinterpret_cast<void*>(image+0x27EA70),reinterpret_cast<void*>(&ModelSetDrawObserved),reinterpret_cast<void**>(&model_set_draw_original))!=MH_OK ||
            MH_CreateHook(reinterpret_cast<void*>(image+0x1ADC190),reinterpret_cast<void*>(&MessageDispatchObserved),reinterpret_cast<void**>(&message_dispatch_original))!=MH_OK)){
            InterlockedExchange(&control->ready,-28);return 30;}
        if(!ModuleFlag(L"no-ffx-gpu.flag") &&
           MH_CreateHook(reinterpret_cast<void*>(image+0x2A6A30),reinterpret_cast<void*>(&SimulationNative),reinterpret_cast<void**>(&simulation_original))!=MH_OK){
            InterlockedExchange(&control->ready,-28);return 30;}
        for(const auto& h:effect_draw::Hooks())
            if(MH_CreateHook(reinterpret_cast<void*>(image+h.rva),h.detour,h.original)!=MH_OK){InterlockedExchange(&control->ready,-29);return 31;}
        const std::array<void*,6> getters{reinterpret_cast<void*>(&ShaderHandle<0>),reinterpret_cast<void*>(&ShaderHandle<1>),reinterpret_cast<void*>(&ShaderHandle<2>),
            reinterpret_cast<void*>(&ShaderHandle<3>),reinterpret_cast<void*>(&ShaderHandle<4>),reinterpret_cast<void*>(&ShaderHandle<5>)};
        for(size_t i=0;i<6;++i)if(MH_CreateHook(reinterpret_cast<void*>(image+shader_handle_rvas[i]),getters[i],reinterpret_cast<void**>(&shader_handle_original[i]))!=MH_OK){
            InterlockedExchange(&control->ready,-29);return 31;}
        {char row[80]{};sprintf_s(row,"{\"event\":\"native_ffx_draw_hooks\",\"hooks\":%zu}\n",effect_draw::Hooks().size()+11);Log(row);}}   // + the five above and the six getters
    // I-0113 tranche 31 (the final review's L5): the Reference's 0x2D census rides on ffx-draw-observe's draw hooks; without them it counts nothing.
    if(control->mode==bootstrap::Mode::Reference && ModuleFlag(L"effect-census.flag") && !ModuleFlag(L"ffx-draw-observe.flag"))
        Log("{\"event\":\"effect_census_refused\",\"reason\":\"the Reference census needs ffx-draw-observe.flag\"}\n");
    // I-0141 tranche 15: reference-no-ffx-draw.flag (21_effects.inc, ReferenceSceneDrawSkipped). Beside ffx-draw-observe.flag, which
    // hooks 24E790 too, or when its hook fails, the start stops with a row (a picture taken with the effects drawn would be read as one
    // without them); original-probe.flag refuses it in its own branch above.
    if(control->mode==bootstrap::Mode::Reference && ModuleFlag(L"reference-no-ffx-draw.flag")){
        if(ModuleFlag(L"ffx-draw-observe.flag")){Log("{\"event\":\"reference_no_ffx_draw\",\"refused\":\"ffx-draw-observe.flag hooks 24E790 too\"}\n");InterlockedExchange(&control->ready,-27);return 29;}
        if(MH_CreateHook(reinterpret_cast<void*>(image+0x24E790),reinterpret_cast<void*>(&native_ffx::ReferenceSceneDrawSkipped),reinterpret_cast<void**>(&native_ffx::reference_scene_draw_original))!=MH_OK){
            Log("{\"event\":\"reference_no_ffx_draw\",\"refused\":\"the 24E790 hook failed\"}\n");InterlockedExchange(&control->ready,-27);return 29;}
        Log("{\"event\":\"reference_no_ffx_draw\",\"hooked\":true}\n");}
    // I-0141 tranche 18: reference-no-occlusion.flag (04c_scene_witnesses.inc, ReferenceOcclusionCellsVisible), Reference runs alone.
    if(control->mode==bootstrap::Mode::Reference && ModuleFlag(L"reference-no-occlusion.flag")){
        if(MH_CreateHook(reinterpret_cast<void*>(image+0xCA050),reinterpret_cast<void*>(&ReferenceOcclusionCellsVisible),reinterpret_cast<void**>(&occlusion_cell_update_original))!=MH_OK){
            Log("{\"event\":\"reference_no_occlusion\",\"refused\":\"the CA050 hook failed\"}\n");InterlockedExchange(&control->ready,-27);return 29;}
        Log("{\"event\":\"reference_no_occlusion\",\"hooked\":true}\n");}
    else if(control->mode!=bootstrap::Mode::Reference && ModuleFlag(L"reference-no-occlusion.flag"))
        Log("{\"event\":\"reference_no_occlusion\",\"ignored\":\"not a Reference run\"}\n");
    // I-0141 tranche 27: reference-share-variant.flag (04c's ReferenceShare* hooks), a bounded Reference run alone; beside
    // light-observe.flag, which hooks 1496D0 too, or when a hook fails, the start stops with a row (a variant shot with the shares on
    // would be read as one without them).
    if(control->mode==bootstrap::Mode::Reference && ModuleFlag(L"reference-share-variant.flag")){
        const auto refuse=[&](const char* why){Log((std::string("{\"event\":\"reference_share_variant\",\"refused\":\"")+why+"\"}\n").c_str());InterlockedExchange(&control->ready,-27);};
        if(!WitnessToolsAllowed()){refuse("not a bounded run");return 29;}
        if(ModuleFlag(L"light-observe.flag")){refuse("light-observe.flag hooks 1496D0 too");return 29;}
        const Hook share_hooks[]{
            {0x1496D0,reinterpret_cast<void*>(&ReferenceShareSunBlock),reinterpret_cast<void**>(&reference_sun_block_original)},
            {0x14A2D0,reinterpret_cast<void*>(&ReferenceShareSunBlend),reinterpret_cast<void**>(&reference_sun_blend_original)},
            {0x149CB0,reinterpret_cast<void*>(&ReferenceShareForwardSun),reinterpret_cast<void**>(&reference_forward_sun_original)},
            {0x14AB20,reinterpret_cast<void*>(&ReferenceShareForwardBlend),reinterpret_cast<void**>(&reference_forward_blend_original)},
            {0x16C9690,reinterpret_cast<void*>(&ReferenceShareGlareShape),reinterpret_cast<void**>(&reference_glare_shape_original)},
            {0x5B3D0,reinterpret_cast<void*>(&ReferenceShareInstanceParams),reinterpret_cast<void**>(&reference_instance_params_original)},   // bits 32, 64
            {0x675B0,reinterpret_cast<void*>(&ReferenceShareEnvRegions),reinterpret_cast<void**>(&reference_env_regions_original)}};
        for(const auto& h:share_hooks)if(MH_CreateHook(reinterpret_cast<void*>(image+h.rva),h.entry,h.original)!=MH_OK){refuse("a hook failed");return 29;}
        reference_share_variant=true;
        Log("{\"event\":\"reference_share_variant\",\"hooked\":true}\n");}
    else if(control->mode!=bootstrap::Mode::Reference && ModuleFlag(L"reference-share-variant.flag"))
        Log("{\"event\":\"reference_share_variant\",\"ignored\":\"not a Reference run\"}\n");
    // I-0141 tranche 27: hold-world-freeze.flag (04_updates_and_test_tools.inc, FreezeTimelineDuringHold), a bounded run in either mode;
    // a failed hook stops the start (a hold read as frozen would not be).
    if(ModuleFlag(L"hold-world-freeze.flag")){
        const auto refuse=[&](const char* why){Log((std::string("{\"event\":\"witness_hold_world_freeze\",\"refused\":\"")+why+"\"}\n").c_str());InterlockedExchange(&control->ready,-27);};
        if(!WitnessToolsAllowed()){refuse("not a bounded run");return 29;}
        if(MH_CreateHook(reinterpret_cast<void*>(image+0xF0FC90),reinterpret_cast<void*>(&FreezeTimelineDuringHold),reinterpret_cast<void**>(&timeline_run_original))!=MH_OK){refuse("the F0FC90 hook failed");return 29;}
        hold_world_freeze=true;
        Log("{\"event\":\"witness_hold_world_freeze\",\"hooked\":true}\n");}
    // I-0140 tranche 39: aa-observe.flag (04c_scene_witnesses.inc, ReferenceAaFilterObserved), Reference runs alone.
    if(control->mode==bootstrap::Mode::Reference && ModuleFlag(L"aa-observe.flag")){
        if(MH_CreateHook(reinterpret_cast<void*>(image+0x1B9790),reinterpret_cast<void*>(&ReferenceAaFilterObserved),reinterpret_cast<void**>(&aa_filter_run_original))!=MH_OK){
            Log("{\"event\":\"reference_aa_filter\",\"refused\":\"the 1B9790 hook failed\"}\n");InterlockedExchange(&control->ready,-27);return 29;}
        Log("{\"event\":\"reference_aa_filter\",\"hooked\":true}\n");}
    else if(control->mode!=bootstrap::Mode::Reference && ModuleFlag(L"aa-observe.flag"))
        Log("{\"event\":\"reference_aa_filter\",\"ignored\":\"not a Reference run\"}\n");
    if(control->mode!=bootstrap::Mode::Reference && ModuleFlag(L"reference-no-ffx-draw.flag"))   // the final review's L4: said, not silent
        Log("{\"event\":\"reference_no_ffx_draw\",\"ignored\":\"not a Reference run (natively: no-ffx-draw.flag)\"}\n");
    // FFX draw N5 (FFX-DRAW §2): the original's effect draw side, Reference runs with ffx-draw-observe.flag.
    if(control->mode==bootstrap::Mode::Reference && ModuleFlag(L"ffx-draw-observe.flag")){using namespace ffx_draw_observe;native_ffx::tracer_mode="reference";
        if(MH_CreateHook(reinterpret_cast<void*>(image+0x24E790),reinterpret_cast<void*>(&SceneDraw),reinterpret_cast<void**>(&draw_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0xBF4C0),reinterpret_cast<void*>(&Command),reinterpret_cast<void**>(&alloc_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x299E40),reinterpret_cast<void*>(&Execute),reinterpret_cast<void**>(&callback_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x1920420),reinterpret_cast<void*>(&SetTopology),reinterpret_cast<void**>(&topology_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x1921370),reinterpret_cast<void*>(&DrawVertices),reinterpret_cast<void**>(&count_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x1921390),reinterpret_cast<void*>(&DrawIndices),reinterpret_cast<void**>(&indexed_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x28F7B0),reinterpret_cast<void*>(&native_ffx::TextureLookupObserved),reinterpret_cast<void**>(&native_ffx::texture_lookup_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x1ADD510),reinterpret_cast<void*>(&native_ffx::NodeUpdateObserved),reinterpret_cast<void**>(&native_ffx::node_update_original))!=MH_OK ||
           (ModuleFlag(L"ffx-entity-life.flag") && MH_CreateHook(reinterpret_cast<void*>(image+0x1ADDE80),reinterpret_cast<void*>(&native_ffx::NodeDrawObserved),reinterpret_cast<void**>(&native_ffx::node_draw_original))!=MH_OK) ||   // I-0141 tranche 14: ffx_newborn_draw
           MH_CreateHook(reinterpret_cast<void*>(image+0x296FF0),reinterpret_cast<void*>(&UpdateBody),reinterpret_cast<void**>(&update_body_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x296D80),reinterpret_cast<void*>(&ProceedBody),reinterpret_cast<void**>(&proceed_body_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x1925B10),reinterpret_cast<void*>(&UavCreated),reinterpret_cast<void**>(&uav_create_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x2A6A30),reinterpret_cast<void*>(&native_ffx::SimulationObserved),reinterpret_cast<void**>(&native_ffx::simulation_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x373D00),reinterpret_cast<void*>(&native_ffx::TracerAppendObserved),reinterpret_cast<void**>(&native_ffx::tracer_append_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x370620),reinterpret_cast<void*>(&native_ffx::TracerRebaseObserved),reinterpret_cast<void**>(&native_ffx::tracer_rebase_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x264230),reinterpret_cast<void*>(&native_ffx::Slot27Observed),reinterpret_cast<void**>(&native_ffx::slot27_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x2997E0),reinterpret_cast<void*>(&native_ffx::EntityOffered),reinterpret_cast<void**>(&native_ffx::entity_original))!=MH_OK ||   // I-0141: ffx_entity_offered
           MH_CreateHook(reinterpret_cast<void*>(image+0x24ED90),reinterpret_cast<void*>(&native_ffx::CreateEffectObserved),reinterpret_cast<void**>(&native_ffx::create_effect_original))!=MH_OK ||   // I-0141: ffx_create
           MH_CreateHook(reinterpret_cast<void*>(image+0x1ACCA20),reinterpret_cast<void*>(&native_ffx::SpawnObserved),reinterpret_cast<void**>(&native_ffx::spawn_original))!=MH_OK ||   // I-0141: ffx_spawn
           MH_CreateHook(reinterpret_cast<void*>(image+0x1AB18C0),reinterpret_cast<void*>(&native_ffx::StopHardObserved),reinterpret_cast<void**>(&native_ffx::stop_hard_original))!=MH_OK ||   // I-0141: ffx_instance_flag
           MH_CreateHook(reinterpret_cast<void*>(image+0x1AB1080),reinterpret_cast<void*>(&native_ffx::StopSoftObserved),reinterpret_cast<void**>(&native_ffx::stop_soft_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x1AD71A0),reinterpret_cast<void*>(&native_ffx::State3Observed),reinterpret_cast<void**>(&native_ffx::state3_original))!=MH_OK ||   // I-0141 tranche 13: ffx_state3
           MH_CreateHook(reinterpret_cast<void*>(image+0x1AD6520),reinterpret_cast<void*>(&native_ffx::DurationObserved),reinterpret_cast<void**>(&native_ffx::duration_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x1ACA640),reinterpret_cast<void*>(&native_ffx::MachineObserved),reinterpret_cast<void**>(&native_ffx::machine_original))!=MH_OK ||   // I-0141 tranche 13b: ffx_machine
           MH_CreateHook(reinterpret_cast<void*>(image+0x1AB1A60),reinterpret_cast<void*>(&native_ffx::ExtSetObserved),reinterpret_cast<void**>(&native_ffx::ext_set_original))!=MH_OK ||   // I-0141 tranche 13c: ffx_ext_set
           MH_CreateHook(reinterpret_cast<void*>(image+0x1920300),reinterpret_cast<void*>(&native_ffx::GxUpdateObserved),reinterpret_cast<void**>(&native_ffx::gx_update_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x131780),reinterpret_cast<void*>(&native_ffx::SnowBlendObserved),reinterpret_cast<void**>(&native_ffx::snow_blend_original))!=MH_OK ||   // I-0105 tranche 10
           MH_CreateHook(reinterpret_cast<void*>(image+0xD1E8A0),reinterpret_cast<void*>(&SnowStoreObserved),reinterpret_cast<void**>(&snow_store_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0xD1EA10),reinterpret_cast<void*>(&DustStoreObserved),reinterpret_cast<void**>(&dust_store_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x636F0),reinterpret_cast<void*>(&ModelDrawObserved),reinterpret_cast<void**>(&model_draw_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x13EEB0),reinterpret_cast<void*>(&ChunkRunObserved),reinterpret_cast<void**>(&chunk_run_original))!=MH_OK ||   // I-0113 gap 5
           MH_CreateHook(reinterpret_cast<void*>(image+0x1920ED0),reinterpret_cast<void*>(&BlendSetObserved),reinterpret_cast<void**>(&blend_set_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x1920F00),reinterpret_cast<void*>(&DepthSetObserved),reinterpret_cast<void**>(&depth_set_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x1920CA0),reinterpret_cast<void*>(&RasterSetObserved),reinterpret_cast<void**>(&raster_set_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x19209D0),reinterpret_cast<void*>(&DsConstantsObserved),reinterpret_cast<void**>(&ds_cb_original))!=MH_OK ||   // I-0105 tracer witness
           MH_CreateHook(reinterpret_cast<void*>(image+0x1920E20),reinterpret_cast<void*>(&PsConstantsObserved),reinterpret_cast<void**>(&ps_cb_original))!=MH_OK){
            InterlockedExchange(&control->ready,-27);return 29;}
        Log(ModuleFlag(L"ffx-entity-life.flag")?"{\"event\":\"reference_ffx_draw_observe\",\"hooks\":36}\n":"{\"event\":\"reference_ffx_draw_observe\",\"hooks\":35}\n");   // 1ADDE80 with ffx-entity-life.flag only
        // I-0113 tranche 31 (effect-census.flag): the pixel programmes' DXBC checksums for the 0x2D census (diagnostic only: the run goes on).
        if(ModuleFlag(L"effect-census.flag"))
            if(void* create_ps=ReferenceDeviceEntry(15);!create_ps || MH_CreateHook(create_ps,reinterpret_cast<void*>(&ObserveCreatePixelShader),reinterpret_cast<void**>(&device_create_ps_original))!=MH_OK)
                Log("{\"event\":\"effect_census_ps_hook\",\"failed\":true}\n");
        // I-0113 tranche 16 (FINISH-LINE line 7): the depth copies' order against the effect passes (reference_depth_copy).
        std::array<void*,5> copies{};
        if(!ReferenceCopyEntries(copies) ||
           MH_CreateHook(copies[0],reinterpret_cast<void*>(&ObserveCopyResource),reinterpret_cast<void**>(&copy_resource_original))!=MH_OK ||
           MH_CreateHook(copies[1],reinterpret_cast<void*>(&ObserveCopyRegion),reinterpret_cast<void**>(&copy_region_original))!=MH_OK ||
           (copies[2] && MH_CreateHook(copies[2],reinterpret_cast<void*>(&ObserveDeferredCopyResource),reinterpret_cast<void**>(&deferred_copy_resource_original))!=MH_OK) ||
           (copies[3] && MH_CreateHook(copies[3],reinterpret_cast<void*>(&ObserveDeferredCopyRegion),reinterpret_cast<void**>(&deferred_copy_region_original))!=MH_OK) ||
           // I-0113 tranche 30 (effect-scan.flag in the Reference): the original's scene target read where it is copied for the effects.
           (reference_scan::On() && MH_CreateHook(copies[4],reinterpret_cast<void*>(&reference_scan::ExecuteObserved),reinterpret_cast<void**>(&reference_scan::execute_original))!=MH_OK) ||
           // STUDY-NOT-YET-NATIVE row 16: the face's mark layer and the quad replays that draw it (reference_face_mark, reference_quad_replay).
           MH_CreateHook(reinterpret_cast<void*>(image+0x836E40),reinterpret_cast<void*>(&FaceComposeObserved),reinterpret_cast<void**>(&face_compose_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x2379C90),reinterpret_cast<void*>(&QuadReplayObserved),reinterpret_cast<void**>(&quad_replay_original))!=MH_OK ||
           // I-0113 tranche 17 (STUDY-PASS-31 §3): the half-size effect layer's composite (reference_layer_composite).
           MH_CreateHook(reinterpret_cast<void*>(image+0x102A80),reinterpret_cast<void*>(&LayerCompositeObserved),reinterpret_cast<void**>(&layer_composite_original))!=MH_OK){
            Log("{\"event\":\"reference_witness_hooks_failed\",\"group\":\"depth_copies_and_face_mark\"}\n");   // told apart from the block above
            InterlockedExchange(&control->ready,-27);return 29;}
        char hooked[160]{};sprintf_s(hooked,"{\"event\":\"reference_witness_hooks\",\"group\":\"depth_copies_and_face_mark\",\"deferred_copies\":%d}\n",int(copies[2]!=nullptr)+int(copies[3]!=nullptr));Log(hooked);}
    // I-0113 tranche 23 (gap 22): the distortion series (ffx-series.flag) reads the draw context after slot 27 (264230) in both
    // modes; hooked here alone when neither ffx-record.flag (Native) nor ffx-draw-observe.flag (Reference) has hooked it above.
    if(ModuleFlag(L"ffx-series.flag") && ((control->mode==bootstrap::Mode::Reference && !ModuleFlag(L"ffx-draw-observe.flag")) ||
                                          (control->mode!=bootstrap::Mode::Reference && native_ffx_draw && !ModuleFlag(L"ffx-record.flag")))){
        if(MH_CreateHook(reinterpret_cast<void*>(image+0x264230),reinterpret_cast<void*>(&native_ffx::Slot27Observed),reinterpret_cast<void**>(&native_ffx::slot27_original))!=MH_OK){
            InterlockedExchange(&control->ready,-27);return 29;}
        Log("{\"event\":\"ffx_distortion_series_hook\"}\n");}
    // Decals by family (decal-observe.flag): the original's manager draw, per-decal draw, character paint patches and paint
    // clears (Reference); natively the request completion row carries the object (RequestNativeDecalCpu).
    if(ModuleFlag(L"decal-observe.flag")){using namespace decal_observe;const bool reference=control->mode==bootstrap::Mode::Reference;   // natively 51AA0 and 198910 never run (the consumer refuses)
        if((reference && (MH_CreateHook(reinterpret_cast<void*>(image+0x51AA0),reinterpret_cast<void*>(&ManagerDrawObserved),reinterpret_cast<void**>(&manager_draw_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x198910),reinterpret_cast<void*>(&DecalDrawObserved),reinterpret_cast<void**>(&decal_draw_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x1AAA70),reinterpret_cast<void*>(&PaintDrawObserved),reinterpret_cast<void**>(&paint_draw_original))!=MH_OK ||
           (!ModuleFlag(L"cascade-observe.flag") &&   // step review M1: cascade-observe hooks 63CB0 itself; the named oracle is then not installed
            MH_CreateHook(reinterpret_cast<void*>(image+0x63CB0),reinterpret_cast<void*>(&MeshLoopObserved),reinterpret_cast<void**>(&mesh_loop_original))!=MH_OK))) ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x51F80),reinterpret_cast<void*>(&PatchCreateObserved),reinterpret_cast<void**>(&patch_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x4CD70),reinterpret_cast<void*>(&PaintClearObserved),reinterpret_cast<void**>(&clear_original))!=MH_OK){
            InterlockedExchange(&control->ready,-27);return 29;}
        // The oracle's GX wrappers (Reference, not with ffx-draw-observe.flag, which hooks 1920300, 1921390, 1920E20, 1920ED0, 1920F00 and 1920CA0 itself).
        if(reference && !ModuleFlag(L"ffx-draw-observe.flag")){const std::array<std::tuple<uint32_t,void*,void**>,12> oracle_hooks{{
                {0x1920300,reinterpret_cast<void*>(&OUpdate),reinterpret_cast<void**>(&update_original)},{0x1920660,reinterpret_cast<void*>(&OVsCb),reinterpret_cast<void**>(&vs_cb_original)},
                {0x1920E20,reinterpret_cast<void*>(&OPsCb),reinterpret_cast<void**>(&ps_cb_original)},{0x1920DA0,reinterpret_cast<void*>(&OPsSrv),reinterpret_cast<void**>(&ps_srv_original)},
                {0x1920D20,reinterpret_cast<void*>(&OPsSampler),reinterpret_cast<void**>(&ps_sampler_original)},{0x19206E0,reinterpret_cast<void*>(&OVs),reinterpret_cast<void**>(&vs_original)},
                {0x1920EA0,reinterpret_cast<void*>(&OPs),reinterpret_cast<void**>(&ps_original)},{0x1920CA0,reinterpret_cast<void*>(&ORaster),reinterpret_cast<void**>(&raster_original)},
                {0x1920ED0,reinterpret_cast<void*>(&OBlend),reinterpret_cast<void**>(&blend_original)},{0x1920F00,reinterpret_cast<void*>(&ODepth),reinterpret_cast<void**>(&depth_original)},
                {0x1921390,reinterpret_cast<void*>(&ODrawIndexed),reinterpret_cast<void**>(&indexed_original)},
                {0x19213F0,reinterpret_cast<void*>(&ODrawIndexedInstanced),reinterpret_cast<void**>(&indexed_instanced_original)}}};
            for(const auto& [rva,detour,original]:oracle_hooks)if(MH_CreateHook(reinterpret_cast<void*>(image+rva),detour,original)!=MH_OK){InterlockedExchange(&control->ready,-27);return 29;}
            // I-0113: the pixel programmes' DXBC checksums for the named oracle (diagnostic only: the run goes on without it).
            if(void* create_ps=ReferenceDeviceEntry(15);!create_ps || MH_CreateHook(create_ps,reinterpret_cast<void*>(&ObserveCreatePixelShader),reinterpret_cast<void**>(&device_create_ps_original))!=MH_OK)
                Log("{\"event\":\"decal_observe_ps_hook\",\"failed\":true}\n");
            // I-0113 tranche 10: the 1x1 cubes' initial texels (diagnostic only).
            if(void* create_texture=ReferenceDeviceEntry(5);!create_texture || MH_CreateHook(create_texture,reinterpret_cast<void*>(&ObserveCreateTexture2D),reinterpret_cast<void**>(&device_create_texture2d_original))!=MH_OK)
                Log("{\"event\":\"reference_unit_cube_hook\",\"failed\":true}\n");}
        // The hooks installed in Reference: the six above (five without the named loop), the twelve wrappers without ffx-draw-observe.flag.
        if(reference){char row[160]{};sprintf_s(row,"{\"event\":\"decal_observe\",\"hooks\":%d,\"named_loop\":%s}\n",(ModuleFlag(L"cascade-observe.flag")?5:6)+(ModuleFlag(L"ffx-draw-observe.flag")?0:12),
            ModuleFlag(L"cascade-observe.flag")?"false":"true");Log(row);}
        else Log("{\"event\":\"decal_observe\",\"hooks\":2}\n");}
    // I-0100 tranche 5: the map objects' pose hand-over in a native run too (the plain Reference installs it above).
    if(control->mode!=bootstrap::Mode::Reference && PopinObserve() && MH_CreateHook(reinterpret_cast<void*>(image+0x66BBB0),reinterpret_cast<void*>(&ObserveObjectSync),
           reinterpret_cast<void**>(&object_sync_original))!=MH_OK){InterlockedExchange(&control->ready,-11);return 13;}
    // I-0114 tranche 2: the logic's corrections with an unlocked rate (native only; inert at k = 1). The locomotion factor 1405C72B0.
    // With unlock-observe.flag the counters skip this entry (one detour an entry; their row names it).
    // no-unlock-fix.flag: the unlocked rate without the corrections (the A/B of the witness runs).
    // I-0132 R12f (the rate live): installed at every native start, so the page's frame rate can leave DS3's 60 while the game runs; at
    // the rate 0 (DS3's own) BeginFrame holds k at 1, each detour then the original's call. Not at the rate 0 under the observing flags
    // (unlock-observe, locomotion-observe, jump-observe: their own detours at 60, as before), whose rate stays fixed for the run.
    // The step review's L2: not at the rate 0 under original-unlock.flag either (its measure without the corrections, as before); L3: where
    // they are not installed the rate stays the start's (native_rate_pinned).
    const bool unlock_hooks=control->mode!=bootstrap::Mode::Reference && !ModuleFlag(L"no-unlock-fix.flag") &&
        (NativeRequestedRate()>0 || !(ModuleFlag(L"unlock-observe.flag") || ModuleFlag(L"locomotion-observe.flag") || ModuleFlag(L"jump-observe.flag") || ModuleFlag(L"original-unlock.flag") || ModuleFlag(L"camera-fixed-step.flag")));   // the final review's L16: the fixed camera step a test tool
    if(!unlock_hooks){native_rate_start=NativeRequestedRate();native_rate_pinned=true;}
    if(unlock_hooks){
        if(MH_CreateHook(reinterpret_cast<void*>(image+0x5C72B0),reinterpret_cast<void*>(&unlock_fix::Locomotion),reinterpret_cast<void**>(&unlock_fix::locomotion_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x5D0010),reinterpret_cast<void*>(&unlock_fix::Turning),reinterpret_cast<void**>(&unlock_fix::turning_original))!=MH_OK ||
           MH_CreateHook(reinterpret_cast<void*>(image+0x50C7B0),reinterpret_cast<void*>(&unlock_fix::Tracker),reinterpret_cast<void**>(&unlock_fix::tracker_original))!=MH_OK){
            InterlockedExchange(&control->ready,-30);return 32;}
        unlock_fix::active=true;
        // Tranche 3: the terrain response (slopes and stairs, unlock_fix::InstallTerrain, 15); no-terrain-fix.flag keeps it the
        // original's (the A/B), its detours then installed only to observe, with locomotion-observe.flag.
        const bool terrain=!ModuleFlag(L"no-terrain-fix.flag");
        if(terrain || ModuleFlag(L"locomotion-observe.flag")){if(!unlock_fix::InstallTerrain()){InterlockedExchange(&control->ready,-36);return 38;}unlock_fix::terrain_active=terrain;}
        // The animation and effect ramps, the bone constraint and the interface's hold value (unlock_fix::InstallRamps, 15).
        // Tranche 7: the target timer and its ring (unlock_fix::TargetTimer, 15, installed with the ramps); no-target-timer-fix.flag keeps
        // the original's (the A/B; the detour then the original's call, its calls counted and its vtables logged with jump-observe.flag).
        unlock_fix::target_timer::active=!ModuleFlag(L"no-target-timer-fix.flag");
        unlock_fix::target_timer::observe=ModuleFlag(L"jump-observe.flag") && ModuleFlag(L"locomotion-observe.flag");
        if(!unlock_fix::InstallRamps()){InterlockedExchange(&control->ready,-38);return 40;}
        // Tranche 4: the jump's takeoff (unlock_fix::MoverVelocity, 15); no-takeoff-fix.flag keeps the original's (the A/B).
        const bool takeoff=!ModuleFlag(L"no-takeoff-fix.flag");
        if(takeoff && !unlock_fix::InstallTakeoff()){InterlockedExchange(&control->ready,-39);return 41;}
        // The effect slots' and the task evaluation's steps (unlock_fix::InstallEffectStep, InstallTaskStep, 15, method D); each
        // refused by name (that system then the original's); no-effect-fix.flag keeps the effects' (the A/B).
        const bool effect_step=!ModuleFlag(L"no-effect-fix.flag") && unlock_fix::InstallEffectStep();
        if(!effect_step && !ModuleFlag(L"no-effect-fix.flag"))Log("{\"event\":\"unlock_effect_step_refused\",\"reason\":\"load_bytes_or_page\"}\n");
        const bool task_step=unlock_fix::InstallTaskStep();
        if(!task_step)Log("{\"event\":\"unlock_task_step_refused\",\"reason\":\"load_bytes_or_page\"}\n");
        // The follow camera (D-0243, Thomas: « Méthode B »): its per-call laws corrected by k (camera_laws, 04); its logic at a fixed
        // 1/60 (camera_step, 04, series 15-19: method A for that system) only as the reference, behind camera-fixed-step.flag.
        const bool fixed_camera=ModuleFlag(L"camera-fixed-step.flag");
        if(fixed_camera){if(!camera_step::Install()){InterlockedExchange(&control->ready,-34);return 36;}
            Log("{\"event\":\"camera_fixed_step\",\"step\":0.016667}\n");}
        else if(!camera_laws::Install()){InterlockedExchange(&control->ready,-35);return 37;}
        char row[256]{};sprintf_s(row,"{\"event\":\"unlock_fix\",\"rate\":%.0f,\"source\":\"%s\",\"sites\":[\"locomotion\",\"turning\",\"tracker\",\"ramps\",\"%s\"%s]}\n",NativeRequestedRate(),native_frame_rate_source,
            fixed_camera?"follow_camera_fixed_step":"follow_camera_laws",(std::string(terrain?",\"terrain\"":"")+(takeoff?",\"takeoff\"":"")+(effect_step?",\"effects\"":"")+(task_step?",\"task_step\"":"")+(unlock_fix::target_timer::active?",\"target_timer\"":"")).c_str());Log(row);}
    else if(NativeRequestedRate()==0 && !std::strcmp(native_frame_rate_source,"file_unrecognised"))Log("{\"event\":\"unlock_fix\",\"rate\":0,\"source\":\"file_unrecognised\"}\n");
    // I-0114 tranche 7 (the step review's M1): at 60 with jump-observe.flag the target timer's calls are counted too (its detour alone,
    // the original's call), so its calls a second at 60 and at an unlocked rate compare.
    else if(control->mode!=bootstrap::Mode::Reference && ModuleFlag(L"jump-observe.flag") && ModuleFlag(L"locomotion-observe.flag")){
        unlock_fix::target_timer::observe=true;
        if(!unlock_fix::InstallTargetTimer())Log("{\"event\":\"unlock_target_timer_observe\",\"refused\":\"hook\"}\n");}
    // I-0114 tranche 1 (unlock-observe.flag): the frame-dependent sites' call counters, in both renderers (the original at 60 the reference).
    // Step review L5: never with an unlocked rate (two runs faulted at about 1,650 frames with both, I-0114 RESULT); L4: its own code.
    // Step review L5 (tranche 3): the rate is the native's only (a frame_rate in the play folder's rekindled-settings.ini does not refuse the original's counters).
    if(ModuleFlag(L"unlock-observe.flag") && NativeRequestedRate()>0 && control->mode!=bootstrap::Mode::Reference)Log("{\"event\":\"unlock_observe_refused\",\"reason\":\"unlocked_rate\"}\n");
    else if(ModuleFlag(L"unlock-observe.flag")){if(!unlock_observe::Install()){InterlockedExchange(&control->ready,-31);return 33;}
        char row[96]{};sprintf_s(row,"{\"event\":\"unlock_observe\",\"sites\":%zu}\n",unlock_observe::count);Log(row);}
    // I-0113 gap 26 (input_focus, 04): the return from the desktop fixed unless no-input-focus-fix.flag; input-observe.flag its rows;
    // the pad's part also unless no-pad-focus-fix.flag; the pad's poll hooked for it or the rows (a refused hook turns off the pad's
    // part alone, the final review's L6), native runs only. I-0132 R12h's L7: the input held back under the precompile page unless
    // no-precompile-input-block.flag, the pad's poll hooked for it too (a refused hook leaves the pad unblocked under the page).
    if(control->mode!=bootstrap::Mode::Reference){input_focus::fix=!ModuleFlag(L"no-input-focus-fix.flag");input_focus::observe=ModuleFlag(L"input-observe.flag");
        input_focus::pad_fix=input_focus::fix && !ModuleFlag(L"no-pad-focus-fix.flag");
        input_focus::page_block_enabled=!ModuleFlag(L"no-precompile-input-block.flag");
        if(input_focus::pad_fix || input_focus::observe || input_focus::page_block_enabled){
            input_focus::pad_hooked=input_focus::Install();
            if(!input_focus::pad_hooked){const bool observed=input_focus::observe;input_focus::pad_fix=false;input_focus::observe=false;   // R12h's step review L2: observe_off only when it was on
                Log(observed?"{\"event\":\"input_focus\",\"refused\":\"pad_hook\",\"observe_off\":1}\n":"{\"event\":\"input_focus\",\"refused\":\"pad_hook\"}\n");}}}
    // I-0113 gap 24 (tranches 38 and 40): Firelink's loading walls never made and their doorway's collision held off (firelink_wall, 04),
    // native runs only; a refused hook (ffx-observe.flag hooks E6DCD0 first) leaves that part as the original's.
    if(control->mode!=bootstrap::Mode::Reference && !firelink_wall::Install())Log("{\"event\":\"firelink_wall\",\"refused\":\"hook\"}\n");
    // I-0132 R12h's third tranche: the game's BGM request held while the precompile page plays Thomas's music (15 HoldTitleBgm), native
    // runs only; a refused hook leaves both musics playing together (a row says so).
    if(control->mode!=bootstrap::Mode::Reference && MH_CreateHook(reinterpret_cast<void*>(image+0x76B920),reinterpret_cast<void*>(&HoldTitleBgm),reinterpret_cast<void**>(&bgm_play_original))!=MH_OK){
        bgm_play_original=nullptr;Log("{\"event\":\"native_title_bgm\",\"refused\":\"hook\"}\n");}
    if(control->mode!=bootstrap::Mode::Reference && bgm_play_original && MH_CreateHook(reinterpret_cast<void*>(image+0x76B9F0),reinterpret_cast<void*>(&DropTitleBgm),reinterpret_cast<void**>(&bgm_stop_original))!=MH_OK)
        {Log("{\"event\":\"native_title_bgm\",\"refused\":\"stop_hook\"}\n");bgm_stop_original=nullptr;}   // its final review's L1
    // I-0132 R12e: the bonfires' crackle at the title (16b title_crackle), an observer of the title music's switch, only with R12h's
    // detour of the music's stop (DropTitleBgm stops it: the final review's L1, L2); a refused hook writes a row, the game starts
    // without the crackle.
    if(control->mode!=bootstrap::Mode::Reference && !ModuleFlag(L"no-title-crackle.flag")){namespace tc=rekindled_overlay::title_crackle;
        const bool hooked=bgm_stop_original && MH_CreateHook(reinterpret_cast<void*>(image+0x76BBA0),reinterpret_cast<void*>(&tc::ObserveSwitchBgm),reinterpret_cast<void**>(&tc::switch_bgm_original))==MH_OK;
        if(hooked)title_crackle_stop=&tc::Stop;
        Log(hooked?"{\"event\":\"native_title_crackle\",\"hooks\":1}\n":bgm_stop_original?"{\"event\":\"native_title_crackle\",\"refused\":\"hook\"}\n":"{\"event\":\"native_title_crackle\",\"refused\":\"no_stop_detour\"}\n");}
    if(control->mode!=bootstrap::Mode::Reference && !firelink_wall::InstallCollision())Log("{\"event\":\"firelink_wall\",\"refused\":\"collision_hook\"}\n");
    // I-0114 (locomotion-observe.flag): the follow camera's update seen at its entry (follow_camera, 04), native runs only.
    if(control->mode!=bootstrap::Mode::Reference && ModuleFlag(L"locomotion-observe.flag")){
        if(!follow_camera::Install()){InterlockedExchange(&control->ready,-32);return 34;}
        // The turn site's detour at 60 too (its k = 1 path is the original's call), for the player's turn rows (player_turn, 04).
        // Final review M1: not under unlock-observe.flag, whose counter holds the entry (named).
        if(!unlock_hooks && ModuleFlag(L"unlock-observe.flag"))Log("{\"event\":\"player_turn_refused\",\"reason\":\"unlock_observe\"}\n");
        else if(!unlock_hooks &&
           MH_CreateHook(reinterpret_cast<void*>(image+0x5D0010),reinterpret_cast<void*>(&unlock_fix::Turning),reinterpret_cast<void**>(&unlock_fix::turning_original))!=MH_OK){
            InterlockedExchange(&control->ready,-33);return 35;}
        // Tranche 3: the terrain detours observing at 60 (their k = 1 path is the original's call; the counters hold those entries).
        if(!unlock_hooks){
            if(ModuleFlag(L"unlock-observe.flag"))Log("{\"event\":\"terrain_observe_refused\",\"reason\":\"unlock_observe\"}\n");
            else if(!unlock_fix::InstallTerrain()){InterlockedExchange(&control->ready,-37);return 39;}}
        Log("{\"event\":\"follow_camera_observe\",\"site\":\"515790\"}\n");}
    // I-0129 R9b: the effects' probe query (01_state.inc's effect_probe), native runs only; it answers only once the rays' probes run.
    if(control->mode!=bootstrap::Mode::Reference && !ModuleFlag(L"no-rt-probes.flag") && !effect_probe::Install()){InterlockedExchange(&control->ready,-40);return 42;}
    // I-0132 R12a: the overlay's observer of the Scaleform repository's insertions (DS3's fonts, 16b); by default since R12d (the
    // precompile screen's words), no-rekindled-overlay.flag leaves it out.
    // A refused observer leaves the screens without DS3's fonts (its row; the step review's L3; the precompile screen's bar still drawn),
    // the start goes on; the overlay test's runs still stop.
    if(control->mode!=bootstrap::Mode::Reference && rekindled_overlay::Enabled() && !rekindled_overlay::InstallRepositoryObserver() && rekindled_overlay::TestRequested()){InterlockedExchange(&control->ready,-41);return 43;}
    if(MH_EnableHook(MH_ALL_HOOKS)!=MH_OK) { InterlockedExchange(&control->ready,-7); return 9; }
    Log(control->mode==bootstrap::Mode::Reference?"{\"event\":\"ready\",\"mode\":\"reference\"}\n":
        "{\"event\":\"ready\",\"mode\":\"native_bootstrap\",\"zone_loading_implemented\":false}\n");
    InterlockedExchange(&control->ready,1); return 0;
}
}  // namespace
