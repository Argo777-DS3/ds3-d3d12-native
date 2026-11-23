// Startup phases: Start, Views, the CPU resource requests, the UI factory and manager, UI texture import, the scene root,
// the resource consumers and the main service and UI steps. Part of game_bootstrap.cpp's single translation unit (the order
// is there): it relies on every earlier part (19_ui_witnesses.inc before it).
#pragma once
#include "19_ui_witnesses.inc"
namespace {
// I-0132 R12h, the window black before our Start (the second part, Thomas 2026-11-12: « le blanc agresse les yeux »): with Start's
// fill below, r12h-page1fr2's desktop captures still showed the game's window white 2.4 s (13.95 s to 16.33 s), from its showing to
// our first step: the game's own start. The module is injected as soon as the process exists (game_bootstrap_ctl, the runner polling
// every 10 ms), ahead of that window (0.58 s ahead in r12h-page1fr3). How DS3 registers and shows its window is not read (its user32 calls are resolved at run
// time; the step review's M1): Setup, right after MH_Initialize, hooks every path we know of, and the rows say which one the game takes:
// user32's RegisterClassExW/A and RegisterClassW/A (the non-Ex calls do not go through the Ex exports): a class whose window procedure
// lies in the game's image registered with a black background (r12h-page1fr3: DS3 registers « FDPclass » through
// RegisterClassExW, its procedure at RVA 0xEF5BD0, a JMP into the protected section, whether it reaches FUN_140EF4D10 not read; its
// brush 6 = COLOR_WINDOW+1, the white), a
// native_window_black_class row each; CreateWindowExW/A (a window made visible) and ShowWindow: such a window of this process,
// top-level and visible, filled black with its class's background black, a native_window_black_early row each; and a window made
// before the hooks blackened at once. Start's first step turns them into pass-throughs (never the window procedure; no MinHook
// freeze at run time). SetWinEventHook was not taken: an in-context hook is removed with the thread that set it (Setup's ends
// early). no-black-window.flag the A/B (neither these hooks nor Start's fill). The game's original brush is a system colour index
// (6), not a GDI object: nothing to delete (the final review's L9).
namespace black_window {
using RegisterExWFn=ATOM(WINAPI*)(const WNDCLASSEXW*);using RegisterExAFn=ATOM(WINAPI*)(const WNDCLASSEXA*);
using RegisterWFn=ATOM(WINAPI*)(const WNDCLASSW*);using RegisterAFn=ATOM(WINAPI*)(const WNDCLASSA*);using ShowFn=BOOL(WINAPI*)(HWND,int);
using CreateWFn=HWND(WINAPI*)(DWORD,LPCWSTR,LPCWSTR,DWORD,int,int,int,int,HWND,HMENU,HINSTANCE,LPVOID);
using CreateAFn=HWND(WINAPI*)(DWORD,LPCSTR,LPCSTR,DWORD,int,int,int,int,HWND,HMENU,HINSTANCE,LPVOID);
RegisterExWFn register_ex_w_original{};RegisterExAFn register_ex_a_original{};RegisterWFn register_w_original{};RegisterAFn register_a_original{};
ShowFn show_original{};CreateWFn create_w_original{};CreateAFn create_a_original{};
std::atomic<bool> active{};uintptr_t image_end{};
bool InImage(uintptr_t a) noexcept {return a>=image && a<image_end;}
HBRUSH Black() noexcept {return static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));}
void ClassRow(const char* api,const char* name,uintptr_t proc,HBRUSH was) noexcept {
    char row[320]{};sprintf_s(row,"{\"event\":\"native_window_black_class\",\"api\":\"%s\",\"class\":\"%.64s\",\"proc_rva\":\"0x%llX\",\"brush_was\":%llu,\"changed\":%s,\"tick\":%llu}\n",
        api,name,static_cast<unsigned long long>(proc-image),static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(was)),
        was!=Black()?"true":"false",static_cast<unsigned long long>(GetTickCount64()));Log(row);}
void Name(char (&out)[72],const wchar_t* w,const char* a) noexcept {   // a class name or its atom; quotes and controls replaced
    if(w?IS_INTRESOURCE(w):IS_INTRESOURCE(a)){sprintf_s(out,"#%u",unsigned(reinterpret_cast<uintptr_t>(w?static_cast<const void*>(w):a)));return;}
    size_t i=0;for(;i<64;++i){const unsigned c=w?unsigned(w[i]):static_cast<unsigned char>(a[i]);if(!c)break;out[i]=(c<32 || c>126 || c=='"' || c=='\\')?'?':char(c);}out[i]=0;}
// The four registrations: a game class (its procedure in the image) registered with a black background, the rest untouched.
template<class Class,class Fn> ATOM Register(const char* api,Fn original,const Class* c,const wchar_t* w,const char* a) {
    if(!active.load(std::memory_order_acquire) || !c || !InImage(reinterpret_cast<uintptr_t>(c->lpfnWndProc)))return original(c);
    Class copy=*c;copy.hbrBackground=Black();char name[72]{};Name(name,w,a);ClassRow(api,name,reinterpret_cast<uintptr_t>(c->lpfnWndProc),c->hbrBackground);
    return original(&copy);}
ATOM WINAPI RegisterExW(const WNDCLASSEXW* c) {
    if(c && c->cbSize<sizeof(WNDCLASSEXW))return register_ex_w_original(c);
    return Register("RegisterClassExW",register_ex_w_original,c,c?c->lpszClassName:nullptr,nullptr);}
ATOM WINAPI RegisterExA(const WNDCLASSEXA* c) {
    if(c && c->cbSize<sizeof(WNDCLASSEXA))return register_ex_a_original(c);
    return Register("RegisterClassExA",register_ex_a_original,c,nullptr,c?c->lpszClassName:"");}
ATOM WINAPI RegisterW(const WNDCLASSW* c) {return Register("RegisterClassW",register_w_original,c,c?c->lpszClassName:nullptr,nullptr);}
ATOM WINAPI RegisterA(const WNDCLASSA* c) {return Register("RegisterClassA",register_a_original,c,nullptr,c?c->lpszClassName:"");}
// The game's window: this process's, top-level, its class's procedure in the game's image (an ANSI class read through the A call:
// the W call gives a CallProc handle; the step review's L2).
bool Ours(HWND hwnd) noexcept {
    DWORD pid{};if(!hwnd || !GetWindowThreadProcessId(hwnd,&pid) || pid!=GetCurrentProcessId() || GetAncestor(hwnd,GA_ROOT)!=hwnd)return false;
    return InImage(static_cast<uintptr_t>(IsWindowUnicode(hwnd)?GetClassLongPtrW(hwnd,GCLP_WNDPROC):GetClassLongPtrA(hwnd,GCLP_WNDPROC)));}
// The window's title bar dark (the coordinator, 2026-11-12 07:57: in windowed mode the light theme's white bar stayed; Windows 11's
// DWMWA_USE_IMMERSIVE_DARK_MODE, 20; an older Windows 10 refuses it: ignored, its HRESULT in the rows).
HRESULT Dark(HWND hwnd) noexcept {const BOOL on=TRUE;return DwmSetWindowAttribute(hwnd,20,&on,sizeof(on));}
// A window's class background black, its title bar dark and its client filled black (GDI into DWM's redirection surface, no message
// loop needed); shared with Start's fill below. Its client size in *client, the title bar's HRESULT in *dark.
bool Blacken(HWND hwnd,RECT* client,HRESULT* dark) noexcept {
    SetClassLongPtrW(hwnd,GCLP_HBRBACKGROUND,reinterpret_cast<LONG_PTR>(Black()));*dark=Dark(hwnd);
    bool filled=false;
    if(IsWindowVisible(hwnd) && GetClientRect(hwnd,client)){if(const HDC dc=GetDC(hwnd)){filled=FillRect(dc,client,Black())!=0;GdiFlush();ReleaseDC(hwnd,dc);}}
    return filled;}
// The game's window: its class's background black, visible or not (a class registered white before the hooks; the final review's
// L3), and its client filled black when visible (every showing while the hooks are active: a splash shown first does not hide the
// main window's; the step review's L5); a row when filled.
void Fill(HWND hwnd,const char* at) noexcept {
    if(!Ours(hwnd))return;
    RECT client{};HRESULT dark{};const bool filled=Blacken(hwnd,&client,&dark);
    if(!filled)return;
    char row[256]{};sprintf_s(row,"{\"event\":\"native_window_black_early\",\"at\":\"%s\",\"filled\":%s,\"width\":%ld,\"height\":%ld,\"dark_title\":\"0x%08lX\",\"tick\":%llu}\n",
        at,filled?"true":"false",client.right,client.bottom,static_cast<unsigned long>(dark),static_cast<unsigned long long>(GetTickCount64()));Log(row);}
BOOL WINAPI Show(HWND hwnd,int command) {
    // The class's background black before the showing erases it (the final review's L3), the client filled after it.
    if(active.load(std::memory_order_acquire) && command!=SW_HIDE && Ours(hwnd)){SetClassLongPtrW(hwnd,GCLP_HBRBACKGROUND,reinterpret_cast<LONG_PTR>(Black()));Dark(hwnd);}
    const BOOL was=show_original(hwnd,command);
    if(active.load(std::memory_order_acquire) && command!=SW_HIDE)Fill(hwnd,"show");
    return was;}
HWND WINAPI CreateW(DWORD ex,LPCWSTR cls,LPCWSTR title,DWORD style,int x,int y,int w,int h,HWND parent,HMENU menu,HINSTANCE instance,LPVOID param) {
    const HWND made=create_w_original(ex,cls,title,style,x,y,w,h,parent,menu,instance,param);
    if(made && active.load(std::memory_order_acquire))Fill(made,"create");
    return made;}
HWND WINAPI CreateA(DWORD ex,LPCSTR cls,LPCSTR title,DWORD style,int x,int y,int w,int h,HWND parent,HMENU menu,HINSTANCE instance,LPVOID param) {
    const HWND made=create_a_original(ex,cls,title,style,x,y,w,h,parent,menu,instance,param);
    if(made && active.load(std::memory_order_acquire))Fill(made,"create");
    return made;}
// Setup, right after MH_Initialize (native runs, unless no-black-window.flag). A hook created but not enabled is removed at once (the
// step review's L3: else Setup's MH_EnableHook(MH_ALL_HOOKS) would retry it and a second refusal stop the start); a refused hook is
// named in the install row and the start goes on.
void Install() noexcept {
    const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(image);
    image_end=image+reinterpret_cast<const IMAGE_NT_HEADERS64*>(image+dos->e_lfanew)->OptionalHeader.SizeOfImage;
    const HMODULE user32=GetModuleHandleW(L"user32.dll");int hooked=0;char refused[160]{};
    const auto hook=[&](const char* name,void* detour,void** original){
        void* target=user32?reinterpret_cast<void*>(GetProcAddress(user32,name)):nullptr;bool ok=false;
        if(target && MH_CreateHook(target,detour,original)==MH_OK){if(MH_EnableHook(target)==MH_OK)ok=true;else MH_RemoveHook(target);}
        if(ok)++hooked;else{strcat_s(refused,refused[0]?",":"");strcat_s(refused,name);}};
    active.store(true,std::memory_order_release);
    hook("RegisterClassExW",reinterpret_cast<void*>(&RegisterExW),reinterpret_cast<void**>(&register_ex_w_original));
    hook("RegisterClassExA",reinterpret_cast<void*>(&RegisterExA),reinterpret_cast<void**>(&register_ex_a_original));
    hook("RegisterClassW",reinterpret_cast<void*>(&RegisterW),reinterpret_cast<void**>(&register_w_original));
    hook("RegisterClassA",reinterpret_cast<void*>(&RegisterA),reinterpret_cast<void**>(&register_a_original));
    hook("CreateWindowExW",reinterpret_cast<void*>(&CreateW),reinterpret_cast<void**>(&create_w_original));
    hook("CreateWindowExA",reinterpret_cast<void*>(&CreateA),reinterpret_cast<void**>(&create_a_original));
    hook("ShowWindow",reinterpret_cast<void*>(&Show),reinterpret_cast<void**>(&show_original));
    // A window the game made before these hooks (the injection racing its start): black now.
    int existing=0;EnumWindows([](HWND hwnd,LPARAM count)->BOOL{if(Ours(hwnd)){++*reinterpret_cast<int*>(count);Fill(hwnd,"install");}return TRUE;},reinterpret_cast<LPARAM>(&existing));
    char row[288]{};sprintf_s(row,"{\"event\":\"native_window_black_install\",\"hooks\":%d,\"refused\":\"%s\",\"existing_windows\":%d,\"tick\":%llu}\n",hooked,refused,existing,
        static_cast<unsigned long long>(GetTickCount64()));Log(row);}
void Stop() noexcept {active.store(false,std::memory_order_release);}
}  // namespace black_window
void __fastcall Start(void* step) noexcept {
    const auto call=InterlockedIncrement64(&control->phase_calls);
    if(control->mode==bootstrap::Mode::Reference) {
        if(call<=8) Observe(step,"start","enter");
        start_original(step);
        if(call<=8) Observe(step,"start","leave");
        return;
    }
    try {
        if(control->stop) { Close(); return; }
        if(call<=8) Observe(step,"native_start","enter");
        if(!opened) {
            LARGE_INTEGER start_entered{};QueryPerformanceCounter(&start_entered);   // R12h: native_window_black's time from here
            // Early attachment alone does not prove that original GPU creation
            // has not happened. Authenticate the actual initial callback state.
            int32_t current{},progress{}; uintptr_t lower{},table{},callback{};
            const auto receiver=reinterpret_cast<uintptr_t>(step);
            if(!Read(receiver+0x50,&current,4) || !Read(receiver+0x54,&progress,4) ||
               !Read(image+0x4961AB0,&lower,8) || !Read(receiver+0x10,&table,8) ||
               current<0 || current>=19 || progress!=current || lower ||
               !Read(table+static_cast<uintptr_t>(current)*16,&callback,8) || callback!=image+0xD59D40) {
                Refuse("native_entry_not_pristine");
            }
            // D56E90's first operation initializes CPU resource registry locks.
            // Source-qualified D6EB00 contains no device/resource construction.
            reinterpret_cast<void(__fastcall*)()>(image+0xD6EB00)();
            Log("{\"event\":\"cpu_registry_locks_initialized\"}\n");
            frame::window::WindowSnapshot w{};
            if(!frame::window::SnapshotWindow(reinterpret_cast<void*>(image),0,w)) { Refuse("real_window_unavailable"); }
            owner_thread=GetCurrentThreadId();
            char row[320]{}; sprintf_s(row,"{\"event\":\"native_window\",\"hwnd\":%llu,\"thread\":%lu,"
                "\"window_thread\":%lu,\"width\":%u,\"height\":%u}\n",
                static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(w.hwnd)),owner_thread,w.window_thread_id,
                w.client_width,w.client_height); Log(row);
            // I-0132 R12h (Thomas, 2026-11-12: « le blanc agresse les yeux »): start-732bb5ed5's desktop captures showed the game's
            // window white from 18.25 s to 28.05 s, before any logo: its client never painted until our first Present, this Start
            // holding the window's thread (no messages) about 7 s (renderer.Start: the device, Streamline). Here, once, on the window's
            // own thread and before the swap chain: the class's background black and the client filled black (GDI draws into DWM's
            // redirection surface without a message loop; the flip-model chain replaces it at its first Present). The game's own
            // start before this Start: black_window's early hooks (above); this fill the fallback when a hook was refused or lost the
            // race (the final review's L4). no-black-window.flag the A/B.
            black_window::Stop();   // the early hooks pass-throughs from here (above)
            if(!ModuleFlag(L"no-black-window.flag")){
                RECT client{};HRESULT dark{};const bool filled=black_window::Blacken(w.hwnd,&client,&dark);
                const bool classed=reinterpret_cast<HBRUSH>(GetClassLongPtrW(w.hwnd,GCLP_HBRBACKGROUND))==black_window::Black();
                LARGE_INTEGER now{},frequency{};QueryPerformanceCounter(&now);QueryPerformanceFrequency(&frequency);char black_row[224]{};
                sprintf_s(black_row,"{\"event\":\"native_window_black\",\"filled\":%s,\"class\":%s,\"width\":%ld,\"height\":%ld,\"dark_title\":\"0x%08lX\",\"ms\":%.2f}\n",filled?"true":"false",classed?"true":"false",
                    client.right,client.bottom,static_cast<unsigned long>(dark),double(now.QuadPart-start_entered.QuadPart)*1000.0/double(frequency.QuadPart));Log(black_row);}
            else Log("{\"event\":\"native_window_black\",\"off\":true}\n");
            {
                std::lock_guard<std::mutex> submission(gpu_submissions);
                // I-0082 (I-0080 lead G1, replacing 1118's no-debug-layer.flag): the D3D12 validation layer only on request,
                // debug-layer.flag next to this module (the runner places it unless -NoDebugLayer, recorded in its baseline), or
                // gpu-validation.flag (runner -GpuValidation), which adds GPU-based validation to it. Play and measurement runs
                // place neither; a PC without the Windows Graphics Tools starts without the layer (DeviceQueues::Open).
                // I-0117 T7: never with PIX's GPU capturer (pix-capture.flag): right after a capture the layer reported acceleration
                // structures in the pooled heaps as unbacked on calls the module was not making then (i117-t5j, i117-t7, i117-t7b: an
                // EmitRaytracingAccelerationStructurePostbuildInfo, then a CopyRaytracingAccelerationStructure with no compaction pending;
                // INFERRED the capturer's, saving the structures), a fault no run without PIX shows.
                const bool pix_capture=ModuleFlag(L"pix-capture.flag");
                gpu_based_validation=!pix_capture && ModuleFlag(L"gpu-validation.flag");
                const bool debug_layer=!pix_capture && (gpu_based_validation || ModuleFlag(L"debug-layer.flag"));
                // A layer asked for beside it (a flag left by a killed run) is refused before the device, not dropped unseen.
                if(pix_capture && (ModuleFlag(L"debug-layer.flag") || ModuleFlag(L"gpu-validation.flag")))Refuse("debug_layer_with_pix_capture");
                device_removal_data=!ModuleFlag(L"no-dred.flag");   // I-0082: DRED breadcrumbs unless no-dred.flag (A/B)
                // D3D12 review R15 (phase A): measure.flag next to this module (placed and removed by the runner) takes the
                // development witnesses out of the frame path: no periodic readback, one frame row per pacing window.
                measure_run=ModuleFlag(L"measure.flag");
                Log(measure_run?"{\"event\":\"native_measure_run\",\"witness_diagnostics\":false}\n":"{\"event\":\"native_measure_run\",\"witness_diagnostics\":true}\n");
                {
                    collect_no_profile=ModuleFlag(L"no-kprofile.flag");
                    native_model_records_disabled=ModuleFlag(L"no-model-records.flag");   // C7-2 A/B
                    Log(native_model_records_disabled?"{\"event\":\"native_model_records_mode\",\"enabled\":false}\n":"{\"event\":\"native_model_records_mode\",\"enabled\":true,\"revalidation_period\":512}\n");
                    native_kept_draws_disabled=ModuleFlag(L"no-persistent-draws.flag");   // C4-2 A/B
                    native_state_reuse_disabled=ModuleFlag(L"no-state-reuse.flag");   // I-0090 A/B
                    native_prepare_chunks_disabled=ModuleFlag(L"no-prepare-chunks.flag");   // I-0075 A/B
                    native_output_follow_disabled=ModuleFlag(L"no-output-follow.flag");   // I-0076 A/B
                    native_display_cycle=ModuleFlag(L"display-cycle.flag");   // I-0076 test
                    {const bool graph_shadow=ModuleFlag(L"graph-shadow.flag");   // phase E E2
                     // D-0171: the frame graph drives the barriers by default (E3, its shadow included); hand-barriers.flag (runner
                     // -HandBarriers) keeps the hand-written ones.
                     const bool frame_graph=!ModuleFlag(L"hand-barriers.flag");
                        renderer.SetGraphShadow(graph_shadow);renderer.SetFrameGraph(frame_graph);
                        char graph_row[160]{};sprintf_s(graph_row,"{\"event\":\"native_graph_shadow\",\"enabled\":%s,\"executor\":%s}\n",
                            graph_shadow || frame_graph?"true":"false",frame_graph?"true":"false");Log(graph_row);}
                    char mode_row[256]{};sprintf_s(mode_row,"{\"event\":\"native_collect_mode\",\"no_profile\":%s}\n",
                        collect_no_profile?"true":"false");Log(mode_row);}
                {
                    // D3D12 phase D cD6, the default since the frames-in-flight workstream (Thomas, 2026-10-18): two frame slots,
                    // the CPU records frame N+1 while the GPU runs frame N; one-frame-slot.flag (runner -OneFrameSlot) keeps one.
                    const bool in_flight=!ModuleFlag(L"one-frame-slot.flag");renderer.SetFrameSlots(in_flight?2:1);
                    renderer.SetDeferredRelease(!ModuleFlag(L"inline-retire.flag"));   // I-0090: the frame's holders released off the render worker (A/B)
                    frame_store::Shared().Keep(!ModuleFlag(L"no-frame-store.flag"));   // I-0091: the collect's arrays on kept blocks (A/B)
                    Log(in_flight?"{\"event\":\"native_frame_ring_requested\",\"slots\":2}\n":"{\"event\":\"native_frame_ring_requested\",\"slots\":1}\n");
                    // D3D12 phase D cD8 (R14a-b), the default with the two slots: three back buffers and a frame-latency waitable
                    // object (maximum latency = the slot count); no-waitable-present.flag (runner -NoWaitablePresent) keeps the
                    // former two-buffer chain.
                    const bool waitable=!ModuleFlag(L"no-waitable-present.flag");renderer.SetPresentation(waitable?3:2,waitable);
                    Log(waitable?"{\"event\":\"native_presentation\",\"buffers\":3,\"waitable\":true}\n":"{\"event\":\"native_presentation\",\"buffers\":2,\"waitable\":false}\n");
                    // R14 cR4 (with cD9): the present pacer by default; legacy-present.flag (runner -DisplayFlags legacy-present)
                    // presents at once with interval 0, as before.
                    // I-0088 P0: original-unlock.flag in a native run presents at once too (the game's rate is raised in BeginFrame).
                    // I-0097 tranche 4 (Thomas, 2026-10-24): vsync-off.flag (the test runs' tool since I-0132 R12f; the launcher writes the key) paces as ever, then presents
                    // with Present(0) on a chain that may tear (his screen's VRR and the driver's V-sync decide).
                    const bool unlock=ModuleFlag(L"original-unlock.flag"),legacy_present=ModuleFlag(L"legacy-present.flag") || unlock;
                    const bool vsync_off=ModuleFlag(L"vsync-off.flag") || !NativeSettings().vsync;   // I-0132 R12b: or the settings' vsync off
                    // I-0132 R12f: the paced chain always allows tearing (where the system does), so the V-sync changes live
                    // (FollowNativeLiveSettings); a Present tears only without it (original-unlock.flag's measure: with it too).
                    renderer.SetPresentPacing(!legacy_present);renderer.SetPresentSync(!vsync_off);renderer.SetPresentTearing(unlock || !legacy_present);renderer.SetPresentTearSynced(unlock);renderer.SetDisplayHolds(ModuleFlag(L"frame-dump.flag"));renderer.SetDisplayClock(NativeDisplayClockRequested());renderer.SetPresentUnpaced(NativeRequestedRate()>0);   // I-0114
                    Log(unlock?"{\"event\":\"native_present_pacing_mode\",\"paced\":false,\"unlock\":true}\n":legacy_present?"{\"event\":\"native_present_pacing_mode\",\"paced\":false}\n":
                        vsync_off?"{\"event\":\"native_present_pacing_mode\",\"paced\":true,\"interval\":1,\"vsync\":false}\n":"{\"event\":\"native_present_pacing_mode\",\"paced\":true,\"interval\":1,\"vsync\":true}\n");
                    Log(NativeSettingsRow().c_str());}   // I-0132 R12b: the settings file's row
                renderer.witness_diagnostics=!measure_run;renderer.gbuffer_pictures=ModuleFlag(L"gbuffer-pictures.flag");kprofile::enabled=measure_run && !collect_no_profile;bootstrap::GameUiSession::SetProfiling(kprofile::enabled);
                {const bool cache=ModuleFlag(L"ui-cache.flag");   // W1 F8a: opt-in ui-cache.flag (runner -DisplayFlags ui-cache) until 04_030 is witnessed
                    bootstrap::GameUiSession::SetUiCache(cache);Log(cache?"{\"event\":\"native_ui_cache\",\"cached_quads\":true}\n":"{\"event\":\"native_ui_cache\",\"cached_quads\":false}\n");}
                // W3: the gamma pass with the original's pinned shaders; no-gamma.flag (runner -DisplayFlags no-gamma) keeps the former
                // path (the frame straight into the back buffer), as does a blob that fails its identity.
                {const bool gamma=!ModuleFlag(L"no-gamma.flag");
                    DisplayBlobs blobs;const bool loaded=gamma && LoadNativeDisplayBlobs(blobs);if(loaded){renderer.SetDisplaySource(std::move(blobs));native_display_source=true;}
                    Log(loaded?"{\"event\":\"native_display_source\",\"gamma\":true}\n":gamma?"{\"event\":\"native_display_source\",\"gamma\":false,\"reason\":\"blob_identity\"}\n":
                        "{\"event\":\"native_display_source\",\"gamma\":false,\"reason\":\"no_gamma_flag\"}\n");}
                // I-0138 tranche 5 (HDR10): what the output is asked.
                renderer.SetHdrOutput(NativeHdrRequest());   // 01_state.inc: the flags, else the settings' rows
                // Fable review (the loading screen's edges at 1920x1080 windowed, Thomas 2026-10-18): the original's buffers and
                // GX target take the engine's size S ([46601E8/EC], the configured resolution, 1920x1080) and DXGI stretches them
                // into a client the desktop clamped to 1920x1061; the native took the client once and fitted the 16:9 movie into
                // it. The output extent is S when the engine has decided one (MENU-TESTS-PREP 4.2 item 1), the client otherwise.
                start_client_width=w.client_width;start_client_height=w.client_height;
                {uint32_t configured[2]{};if(Read(image+0x46601E8,configured,sizeof(configured)) && configured[0]>=320 && configured[1]>=240 && configured[0]<=16384 && configured[1]<=16384 &&
                    (configured[0]!=w.client_width || configured[1]!=w.client_height)){start_width=configured[0];start_height=configured[1];
                    char extent_row[200]{};sprintf_s(extent_row,"{\"event\":\"native_output_extent\",\"configured\":[%u,%u],\"client\":[%u,%u],\"rule\":\"engine size S\"}\n",configured[0],configured[1],w.client_width,w.client_height);Log(extent_row);}
                 else{start_width=w.client_width;start_height=w.client_height;}}
                // I-0084 (lead G3): warp.flag (announced runs) opens the device on WARP, Microsoft's software adapter, the only
                // other vendor's implementation this PC has (Thomas has an NVIDIA card only; AMD and Intel testers come later).
                const bool warp=ModuleFlag(L"warp.flag");
                // I-0097 tranche 3 (Thomas, 2026-10-24 08:32: the screen chosen at launch): display-screen.txt next to the module
                // names a screen by its device name (\\.\DISPLAY2, its first line); the window moves there before its swap chain is
                // made, so the chain is the screen's from its first present (K-0479's lead 2).
                {std::wstring screen;
                    if(const auto file=ModuleSibling(L"display-screen.txt");!file.empty()){
                        std::wifstream in(file);if(in)std::getline(in,screen);while(!screen.empty() && iswspace(screen.back()))screen.pop_back();}
                    if(!screen.empty()){const bool placed=ds3native::PlaceWindowOnScreen(w.hwnd,screen.c_str());RECT moved{};
                        // Final review: a move that changes the client (another DPI scale, a smaller screen) gives the start its new size.
                        if(placed && GetClientRect(w.hwnd,&moved) && moved.right>0 && moved.bottom>0 && (uint32_t(moved.right)!=start_client_width || uint32_t(moved.bottom)!=start_client_height)){
                            if(start_width==start_client_width && start_height==start_client_height){start_width=uint32_t(moved.right);start_height=uint32_t(moved.bottom);}
                            start_client_width=uint32_t(moved.right);start_client_height=uint32_t(moved.bottom);}std::wstring shown=screen.substr(0,32);for(auto& c:shown)if(c==L'\\' || c==L'"' || c<32)c=L'/';char screen_row[200]{};
                        sprintf_s(screen_row,"{\"event\":\"native_display_screen\",\"requested\":\"%ls\",\"placed\":%s,\"refresh_hz\":%u}\n",shown.c_str(),placed?"true":"false",ds3native::RefreshRateOf(w.hwnd));Log(screen_row);}}
                // I-0111 (R1, tranche 4 step 2): unless no-dlss.flag, the renderer gets Streamline's folder (the module's: its signed DLLs travel with the build).
                // I-0132 R12f: frame generation offered (DLSS-G loaded at Streamline's start, unloaded until asked) unless no-fg-offer.flag
                // (the earlier start: DLSS-G only when asked); asked only with a DLSS mode (NativeFrameGenerationWanted).
                if(NativeDlssRequested())renderer.SetUpscaler(ModuleSibling(L""),[](const char* row){Log(row);},NativeFrameGenerationWanted(),!ModuleFlag(L"no-fg-offer.flag"));   // I-0131 R10b
                // I-0132 R11f (Thomas, 2026-11-09 04:48): the device on the Agility SDK 1.620.1 (the module's D3D12 folder, its DLLs linked
                // there by the build); I-0132 R11i (Thomas, 2026-11-10 00:20): by default, its stricter debug layer's pipeline states
                // cleaned (PipelineStore: an unused depth-stencil format dropped), no-agility.flag the test tool (Windows' inbox runtime);
                // WARP and PIX keep the inbox runtime (the step review's L7: never under pix-capture.flag). GPU upload heaps where the
                // device offers them (the inbox runtime does too) unless no-gpu-upload.flag.
                ds3native::agility_sdk_folder=!ModuleFlag(L"no-agility.flag") && !pix_capture && !warp?ModuleSibling(L"D3D12\\"):std::wstring{};
                ds3native::gpu_upload_allowed=!ModuleFlag(L"no-gpu-upload.flag");
                renderer.Start(w.hwnd,warp,debug_layer,start_width,start_height);native_game_window.store(w.hwnd);   // I-0132 R11e: the fatal close's window
                if(NativeDlssRequested()){std::string state=renderer.UpscalerState().substr(0,300);for(auto& c:state)if(c=='"' || c=='\\' || c<32)c='\'';
                    char dlss_row[480]{};sprintf_s(dlss_row,"{\"event\":\"native_streamline\",\"state\":\"%s\",\"dlss_ready\":%s,\"frame_generation_requested\":%u,\"frame_generation\":%s}\n",state.c_str(),
                        ds3native::streamline::DlssReady()?"true":"false",NativeFrameGenerationMultiplier(),renderer.FrameGeneration()?"true":"false");Log(dlss_row);}
                if(warp)Log("{\"event\":\"native_warp_adapter\"}\n");
                if(renderer.HdrOutput().request.mode!=ds3native::hdr::Mode::Off)LogNativeHdrOutput("start");   // I-0138 tranche 5: only when asked (an SDR run's journal is unchanged)
                {char layer_row[256]{};sprintf_s(layer_row,"{\"event\":\"native_debug_layer\",\"requested\":%s,\"enabled\":%s,\"gpu_validation\":%s,\"dred\":%s,\"pix_capture\":%s,\"dxgi_debug\":%s}\n",
                    debug_layer?"true":"false",gpu.DebugLayerActive()?"true":"false",gpu.DebugLayerActive() && gpu_based_validation?"true":"false",
                    gpu.RemovalDataActive()?"true":"false",pix_capture?"true":"false",gpu.DxgiDebug()?"true":"false");Log(layer_row);}   // I-0132 R11f: dxgi_debug
                // I-0109 (R0): the device's feature level and what it offers the ray-tracing path; I-0132 R11f: its runtime's SDK version
                // (620: the Agility SDK's), why the Agility SDK was not used (empty when it was), the GPU upload heaps offered and used.
                {const auto& c=gpu.Capabilities();std::string agility=gpu.AgilityFailure().substr(0,160);for(auto& ch:agility)if(ch=='"' || ch=='\\' || ch<32)ch='\'';char caps_row[640]{};
                    sprintf_s(caps_row,"{\"event\":\"native_device_caps\",\"feature_level\":\"%x\",\"shader_model\":\"%x\",\"raytracing_tier\":%d,\"binding_tier\":%d,\"mesh_shader_tier\":%d,\"enhanced_barriers\":%s,\"native_16bit\":%s,\"sdk_version\":%u,\"agility_asked\":%s,\"agility_failure\":\"%s\",\"gpu_upload\":%s,\"gpu_upload_used\":%s}\n",
                        unsigned(c.feature_level),unsigned(c.shader_model),int(c.raytracing),int(c.binding),int(c.mesh),c.enhanced_barriers?"true":"false",c.native_16bit?"true":"false",
                        c.sdk_version,ds3native::agility_sdk_folder.empty()?"false":"true",agility.c_str(),c.gpu_upload?"true":"false",ds3native::gpu_upload_heaps.load()?"true":"false");Log(caps_row);}
                // I-0109 (R0): one inline ray against one triangle through the bindless heap, before the frames start; it never refuses
                // (a failure only names its step). I-0132 R11e (D-0262, no fallback): always run (no-rt-selftest.flag removed).
                bool rt_usable=gpu.device5 && gpu.Capabilities().raytracing>=D3D12_RAYTRACING_TIER_1_1;
                {const auto t=RunRaytracingSelfTest(gpu,bindless);char rt_row[256]{};
                    sprintf_s(rt_row,"{\"event\":\"native_rt_selftest\",\"ran\":%s,\"passed\":%s,\"stage\":\"%s\",\"hresult\":\"0x%08lX\",\"status\":%u,\"t\":%.4f,\"ms\":%.2f}\n",
                        t.ran?"true":"false",t.passed?"true":"false",t.stage,static_cast<unsigned long>(t.failure),t.status,std::isfinite(t.t)?t.t:-1.0f,t.milliseconds);Log(rt_row);
                    rt_usable=rt_usable && t.passed;}
                // I-0128 R8a: the main view's frame and its traced passes timed on the GPU (rt-gpu-measure.flag, a diagnostic: native_rt_gpu
                // rows every 600 frames).
                if(ModuleFlag(L"rt-gpu-measure.flag")){renderer.rt_gpu_measure=true;Log("{\"event\":\"native_rt_gpu\",\"on\":true}\n");}
                // I-0132 R11d's test tool, read first: R11e's fatal close shows no box in the test runs.
                renderer.rt_fail_magenta=ModuleFlag(L"rt-fail-magenta.flag");
                // I-0117 (R2 T0): the rays' scene, every admitted model's BLAS from here on; the default since I-0125 R6f (Thomas,
                // 2026-11-07 00:48, D-0253: the play sessions traced). I-0132 R11e (D-0262, no fallback): the world is drawn only with the
                // rays (no-rt-scene.flag and its raster world removed); an adapter without DXR 1.1 or a failed self-test closes the game
                // with a message (NativeRtFatal).
                {native_rt_scene.store(rt_usable,std::memory_order_relaxed);
                    if(rt_usable)renderer.SetRtHeap(&bindless);   // I-0117 (R2 T3): the rays' views (the alpha records' buffers and textures)
                    Log(rt_usable?"{\"event\":\"native_rt_scene\",\"on\":true}\n":"{\"event\":\"native_rt_scene\",\"on\":false,\"reason\":\"no_dxr_or_selftest_not_passed\"}\n");
                    if(!rt_usable)renderer.rt_asked=true;   // R11e's step review M4: the main view's frames judged (black), never DS3's lighting
                    if(!rt_usable)NativeRtFatal(!gpu.device5 || gpu.Capabilities().raytracing<D3D12_RAYTRACING_TIER_1_1?
                        "this graphics card or its driver offers no DirectX Raytracing 1.1":"the ray-tracing self-test failed at startup");
                    // I-0117 (R2 T4): rt-view.flag, the control view (a test tool): the rays against the raster depth (native_rt_view rows,
                    // the rt-view pictures).
                    if(rt_usable && ModuleFlag(L"rt-view.flag")){renderer.rt_view=true;Log("{\"event\":\"native_rt_view\",\"on\":true}\n");}
                    // I-0117 R5: the sun's shadow traced in place of the cascades (their casters, atlas and mask layer); on with the
                    // rays since I-0118 (D-0237: no raster shadow kept behind a flag; without the rays' scene the cascades).
                    if(rt_usable){renderer.rt_shadow=true;renderer.rt_shadow_witness=ModuleFlag(L"rt-shadow-witness.flag");
                        Log(renderer.rt_shadow_witness?"{\"event\":\"native_rt_shadow\",\"on\":true,\"witness\":true}\n":"{\"event\":\"native_rt_shadow\",\"on\":true}\n");}
                    // I-0122 R4a: the unshadowed local lights shaded natively from one world-space list, on with the rays (their batches
                    // without the rays' scene).
                    if(rt_usable){renderer.rt_lights=true;Log("{\"event\":\"native_rt_lights\",\"on\":true}\n");}
                    // I-0125 R6b: the deferred surfaces' specular ambient traced in place of the region cubes' draws (I-0131 R10a: the
                    // cubes' raster pass and no-rt-reflect.flag are removed; a frame the rays do not serve has no specular ambient).
                    if(rt_usable){renderer.rt_reflect=true;Log("{\"event\":\"native_rt_reflect\",\"on\":true}\n");}
                    // I-0129 R9a: the traced probes (the forward rows' ambient on the GI frames, their sun's mask with the traced sun);
                    // no-rt-probes.flag keeps DS3's baked probes (a test tool).
                    if(rt_usable && !ModuleFlag(L"no-rt-probes.flag")){renderer.rt_probes=true;effect_probe::on=true;Log("{\"event\":\"native_rt_probes\",\"on\":true}\n");}
                    // I-0125 R6c: the water's g_ReflectionTexture traced (R6f, D-0251: the mirror view removed; no-water-reflection.flag:
                    // the black default).
                    if(rt_usable && NativeReflectionEnabled()){renderer.rt_water=true;Log("{\"event\":\"native_rt_water\",\"on\":true}\n");
                        // I-0125 R6d: the water's texels coloured by what their ray met, a diagnostic (journaled: R6d's final review).
                        if(ModuleFlag(L"rt-sky-witness.flag")){renderer.rt_sky_witness=true;Log("{\"event\":\"native_rt_sky_witness\",\"on\":true}\n");}}
                    // I-0126 R7a: the deferred surfaces' diffuse ambient traced, denoised by RR or, R7c, NRD (whenever the rays are usable: R7c,
                    // D-0251, removed the no-rt-gi.flag comparison and the SSAO).
                    if(rt_usable){renderer.rt_gi=true;
                        // R7a's diagnostics: the GI's hits without their ambient (the cache's, since I-0132 R11d) or the local lights.
                        renderer.rt_gi_diagnostic=(ModuleFlag(L"rt-gi-no-ambient.flag")?kRtReflectGiNoAmbient:0u)|(ModuleFlag(L"rt-gi-no-local.flag")?kRtReflectGiNoLocal:0u)|
                            (ModuleFlag(L"rt-gi-no-sky.flag")?kRtReflectGiNoSky:0u)|(ModuleFlag(L"rt-cache-witness.flag")?kRtReflectCacheWitness:0u)|   // R7b: the misses' share
                            (ModuleFlag(L"rt-gi-no-glossy.flag")?kRtReflectGiNoGlossy:0u);   // I-0126 R7d's diagnostic
                        // I-0126 R7d: ReSTIR GI at the PT level, its stages (the bias check's diagnostics). I-0132 R11i (Thomas, 2026-11-10
                        // 00:49, D-0265): on by default wherever the PT level runs (unbiased over the picture, R8d; as steady as the one ray at
                        // Firelink, a little less at the High Wall, about +5 to +6.5 ms at 1920x1080: I-0132 RESULT « R11i »); no-rt-restir.flag the test tool that cuts it (rt-restir.flag, the earlier opt-in,
                        // changes nothing now). The PT level runs only on the frames RR denoises (FillRtReflectFrame), so ReSTIR GI too.
                        renderer.rt_restir=!ModuleFlag(L"no-rt-restir.flag");
                        // I-0128 R8d (R8-PLAN §2): its decorrelation for RR: the temporal neighbour permuted (rt-restir-no-permute.flag R7d's
                        // form), the history's cap (rt-restir-history-N.flag, N 1, 2, 4 or 16; else the shader's 8), the share shaded from the
                        // initial sample (102 / 255, RTXDI's ReSTIR PT default factor 0.4; rt-restir-no-decorrelate.flag none: R8d's slot, its steadiest form).
                        uint32_t restir_stages=(ModuleFlag(L"rt-gi-no-temporal.flag")?0u:kRtRestirTemporal)|(ModuleFlag(L"rt-gi-no-spatial.flag")?0u:kRtRestirSpatial)|
                            (ModuleFlag(L"rt-gi-jacobian-wide.flag")?kRtRestirWide:0u)|(ModuleFlag(L"rt-restir-no-permute.flag")?0u:kRtRestirPermute);
                        for(const uint32_t cap:{1u,2u,4u,16u}){wchar_t name[40]{};swprintf_s(name,L"rt-restir-history-%u.flag",cap);if(ModuleFlag(name)){restir_stages|=cap<<kRtRestirCapShift;break;}}
                        if(!ModuleFlag(L"rt-restir-no-decorrelate.flag"))restir_stages|=102u<<kRtRestirShareShift;
                        renderer.rt_restir_stages=restir_stages;
                        if(renderer.rt_restir.load()){char restir_row[192]{};const uint32_t cap=RtRestirCapOf(restir_stages);
                            sprintf_s(restir_row,"{\"event\":\"native_rt_restir\",\"stages\":%u,\"permute\":%s,\"history\":%u,\"decorrelate\":%.3f}\n",restir_stages,
                                (restir_stages&kRtRestirPermute)?"true":"false",cap?cap:8u,RtRestirShareOf(restir_stages)/255.0);Log(restir_row);}
                        renderer.rt_gi_mean=ModuleFlag(L"rt-gi-mean.flag");   // I-0126 R7d: the GI's bias check (R7-PLAN §8), beside the world pictures
                        // I-0126 R7b: the radiance cache at the hits (no-rt-cache.flag, a diagnostic: the hits get no ambient at all; I-0132 R11d, D-0262: R7a's baked ambient removed).
                        const bool cache=!ModuleFlag(L"no-rt-cache.flag");renderer.rt_cache=cache;
                        char gi_row[160]{};sprintf_s(gi_row,"{\"event\":\"native_rt_gi\",\"on\":true,\"cache\":%s}\n",cache?"true":"false");Log(gi_row);}
                    // I-0122 R4c: the PT level's direct light (the settings' level pt or rt-pt.flag), on the frames RR denoises.
                    // I-0128 R8b: its paths' vertices (the settings' bounces) and the bias check's reference (rt-pt-no-cache.flag, a diagnostic).
                    if(rt_usable){const bool pt=NativeRtPtRequested();renderer.rt_pt=pt;const uint32_t bounces=NativeRtBouncesRequested();renderer.rt_bounces=bounces;renderer.rt_gi_paths=NativeRtGiPathsRequested();   // I-0132 R12g: its GI paths
                        const bool reference=ModuleFlag(L"rt-pt-no-cache.flag");renderer.rt_path_reference=reference;
                        const bool grid=!ModuleFlag(L"no-rt-light-grid.flag");renderer.rt_light_grid=grid;   // I-0128 R8c: the hits' light grid
                        const bool roulette=!ModuleFlag(L"no-rt-roulette.flag");renderer.rt_roulette=roulette;   // I-0132 R11a: Russian roulette past the first vertex (the default)
                        renderer.rt_ser=!ModuleFlag(L"no-rt-ser.flag");   // I-0132 R11b: shader execution reordering on the PT level's paths (the default)
                        // I-0132 R11d's test tools for the sources' sheets (D-0262): the local lights' colours, the hits' emission.
                        // Thomas's choice on R11d's sheets (2026-11-09 05:11, « Va pour x2 »): x2 until D-0293 (2026-11-16): DS3's own (x1) by default;
                        // rt-local-x2.flag and rt-local-x4.flag test tools (rt-local-x1.flag, still accepted by the runner, now does nothing).
                        renderer.rt_local_scale=ModuleFlag(L"rt-local-x4.flag")?4.f:ModuleFlag(L"rt-local-x2.flag")?2.f:1.f;   // D-0293 (Thomas, 2026-11-16, the A/B of I-0141 tranche 10): DS3's own strength for good
                        // I-0132 R11h (test tools, D-0264): the local lights in physical terms at 1/10, 1/50, 1/200 of DS3's x1 against the sun
                        // (rt-local-physical-10/-50/-200.flag); off by default: DS3's falloff with rt_local_scale (x1 since D-0293) keeps DS3's balance (Thomas,
                        // 2026-11-09 23:59, « Fais ça oui », after finding the physical sheets ugly: the fog and the daylight veil the lifted interiors).
                        renderer.rt_local_physical=ModuleFlag(L"rt-local-physical-10.flag")?0.1f:ModuleFlag(L"rt-local-physical-50.flag")?0.02f:ModuleFlag(L"rt-local-physical-200.flag")?0.005f:0.f;
                        renderer.rt_asked=true;   // the frames the rays do not serve judged (R11d, R11e; rt-fail-magenta.flag read above)
                        renderer.rt_emissive_bits=ModuleFlag(L"rt-emissive-x16.flag")?uint32_t(kRtReflectEmissiveX16):ModuleFlag(L"rt-emissive-x4.flag")?uint32_t(kRtReflectEmissiveX4):0u;
                        char level_row[320]{};sprintf_s(level_row,"{\"event\":\"native_rt_level\",\"level\":\"%s\",\"bounces\":%u,\"reference\":%s,\"rt_light_grid\":%s,\"roulette\":%s,\"local_scale\":%.0f,\"local_physical\":%g,\"emissive_bits\":%u,\"fail_magenta\":%s}\n",pt?"pt":"rt",pt?bounces:1u,pt && reference?"true":"false",grid?"true":"false",
                            pt && bounces>1 && !reference && roulette?"true":"false",double(renderer.rt_local_scale.load()),double(renderer.rt_local_physical.load()),renderer.rt_emissive_bits.load(),renderer.rt_fail_magenta.load()?"true":"false");Log(level_row);}
                    // I-0123 RR-a: DLSS Ray Reconstruction in place of DLSS on the traced frames (the player's option: the settings' rr off,
                    // or no-rr.flag, keeps DLSS Super Resolution; it runs only where Streamline's RR is ready, which the first DLSS frame's
                    // row and a latch's row tell).
                    if(rt_usable){const bool rr=NativeRrRequested();renderer.rr=rr;native_rr_session.store(rr,std::memory_order_relaxed);
                        char rr_row[160]{};sprintf_s(rr_row,"{\"event\":\"native_rr\",\"on\":%s,\"ready\":%s}\n",rr?"true":"false",ds3native::streamline::RrReady()?"true":"false");Log(rr_row);}}
                // Only the runner places the flag: a validation run without the layer would report 0 errors unvalidated (with
                // pix-capture.flag the layer is never turned on and the runner records such a run as without it: I-0117 T7).
                if(debug_layer && !gpu.DebugLayerActive())Refuse("debug_layer_requested_unavailable");
                // I-0072 (D-0172, ER-PIPELINE-STUDY): the device's pipeline cache next to the module: its precompile list
                // (native-pipelines.v3.psc), read at this start and written when it changed; the listed pipelines are compiled at
                // once on worker threads (below normal priority) while the game boots, so the frames find them in memory. A
                // pipeline's name is its whole description, so one list serves every module, adapter, driver, WARP and the debug
                // layer. No driver library (I-0113 gap 4: its loaded pipelines sampled their static samplers as point samplers).
                // The v1 files (names without the root signature) are removed. no-pipeline-library.flag (a historical name) keeps the cache in memory only (A/B).
                {std::wstring list;
                    if(const auto folder=ModuleSibling(L"");!folder.empty()){
                        list=folder+L"native-pipelines.v3.psc";DeleteFileW((folder+L"native-pipelines.v1.library").c_str());DeleteFileW((folder+L"native-pipelines.v1.library.names").c_str());}
                    const bool enabled=!ModuleFlag(L"no-pipeline-library.flag");
                    Pipelines().Open(renderer.CpuIngressDevice().device.Get(),list,enabled);
                    const unsigned workers=std::clamp(std::thread::hardware_concurrency()/4u,1u,4u);Pipelines().Precompile(workers);const auto stats=Pipelines().Stats();
                    // I-0132 R12d: not under WARP (the step review's M2: minutes of compiles a bounded run's close would wait for).
                    if(native_rt_scene.load(std::memory_order_relaxed) && !warp)StartNativeWarm(renderer.CpuIngressDevice().device.Get(),renderer.rt_ser.load());else native_warm_finished=true;
                    char list_row[256]{};
                    sprintf_s(list_row,"{\"event\":\"native_pipeline_list\",\"enabled\":%s,\"open\":%s,\"list_loaded\":%s,\"list_bytes\":%llu,\"records\":%llu,\"pruned\":%llu,\"precompile_workers\":%u}\n",
                        enabled?"true":"false",stats.open?"true":"false",stats.list_loaded?"true":"false",static_cast<unsigned long long>(stats.list_bytes),
                        static_cast<unsigned long long>(stats.records),static_cast<unsigned long long>(stats.pruned),workers);Log(list_row);}
                // R13 cR3/cR6: every frame job runs on the native render worker (W2 pumped post wait, 30 s deadline) and the
                // world frame is prepared there (cR5), by default since cR6 (D-0168); inline-render.flag (runner -InlineRender)
                // runs the jobs inline in Clear() as before cR3, inline-prepare.flag (-InlinePrepare) prepares on the engine thread.
                {const bool requested=!ModuleFlag(L"inline-render.flag"),prepare=!ModuleFlag(L"inline-prepare.flag");const std::wstring module_folder=ModuleSibling(L"");
                    if(requested){native_ui_build_current=renderer.PrimitiveBuildCurrent();render_worker=warp?new RenderWorker(RenderWorker::Options{600000,true}):new RenderWorker;render_worker->Start();renderer.SetRenderThread(render_worker->ThreadId());   // I-0084: a 600 s deadline for WARP's slow frames
                        native_prepare_on_worker=prepare;
                        // I-0088 P1: the UI display worker (no pumping: the owner joins it within the frame, holding no lock);
                        // no-ui-worker.flag (A/B) prepares the UI in the render step as before.
                        if(!ModuleFlag(L"no-ui-worker.flag")){ui_display_worker=new RenderWorker(RenderWorker::Options{warp?600000ul:30000ul,false,L"ds3-native-ui"});ui_display_worker->Start();}
                        // I-0088 P2: the collect's side worker (the effect record beside the walk); no-collect-side.flag (A/B) records inline.
                        if(!ModuleFlag(L"no-collect-side.flag")){collect_side_worker=new RenderWorker(RenderWorker::Options{warp?600000ul:30000ul,false,L"ds3-native-side"});collect_side_worker->Start();}
                        // I-0132 R11g: the collect's rays worker (the rays' walk beside the main walk); no-rays-aside.flag (A/B) walks inline.
                        // Only with twelve hardware threads and up (the final review's L10, as the recording pool: measured on sixteen).
                        if(!ModuleFlag(L"no-rays-aside.flag") && std::thread::hardware_concurrency()>=12){collect_rays_worker=new RenderWorker(RenderWorker::Options{warp?600000ul:30000ul,false,L"ds3-native-rays"});collect_rays_worker->Start();}}
                    // I-0074 point 3c: the G-buffer recorded in chunks on the recording pool unless no-record-chunks.flag (A/B).
                    renderer.SetRecordChunks(!ModuleFlag(L"no-record-chunks.flag"));
                    {char chunk_row[160]{};sprintf_s(chunk_row,"{\"event\":\"native_record_chunks_mode\",\"enabled\":%s,\"pool_workers\":%zu}\n",
                        ModuleFlag(L"no-record-chunks.flag")?"false":"true",RecordingPool().Workers());Log(chunk_row);}
                    char thread_row[200]{};sprintf_s(thread_row,"{\"event\":\"native_render_thread\",\"enabled\":%s,\"thread\":%lu,\"owner\":%lu,\"wait\":\"W2\",\"prepare_on_worker\":%s}\n",
                        requested?"true":"false",requested?render_worker->ThreadId():0ul,GetCurrentThreadId(),native_prepare_on_worker?"true":"false");Log(thread_row);
                    if(renderer.PresentTearing())Log("{\"event\":\"native_present_tearing\",\"enabled\":true}\n");   // I-0088 P0 (original-unlock.flag)
                    if(requested && ModuleFlag(L"sample-profile-load.flag"))native_sample_start_frame=1;
                    // I-0074: sample-profile-start.txt next to the module (a world frame number, written by the series runner for
                    // one run) starts the sampler at that frame, 75,000 samples a thread, as the load flag does from the first.
                    // I-0079: an optional second number, the presented-frame mark at which sampling ends and the samples are written.
                    bool start_file=false;uint64_t sample_end_mark=~0ull;
                    if(requested && !module_folder.empty()){std::wifstream start(module_folder+L"sample-profile-start.txt");uint64_t frame{};
                        if(start && (start>>frame) && frame){native_sample_start_frame=frame;start_file=true;uint64_t end{};if(start>>end && end)sample_end_mark=end;}}
                    if(requested && (ModuleFlag(L"sample-profile.flag") || native_sample_start_frame==1 || start_file)){
                        const auto output=JournalSibling(L"sample-profile.txt");
                        const std::array<sampling::Thread,2> threads{{{owner_thread,"owner"},{render_worker->ThreadId(),"worker"}}};
                        const bool started=sampling::Start(output,threads,native_sample_active,native_sample_start_frame==1 || start_file?75000:30000,&native_sample_mark,sample_end_mark);
                        native_sample_loaders.store(started,std::memory_order_release);
                        if(started)WorkPool::worker_hook.store(&SamplePoolThread,std::memory_order_release);   // I-0077: the pool workers too
                        // I-0097: with sample-ds3-threads.flag, every thread DS3 itself started is sampled too (AddDs3SampledThreads).
                        if(started && ModuleFlag(L"sample-ds3-threads.flag")){
                            char ds3_row[120]{};sprintf_s(ds3_row,"{\"event\":\"native_sample_ds3_threads\",\"added\":%u}\n",AddDs3SampledThreads(threads));Log(ds3_row);}
                        char sample_row[120]{};sprintf_s(sample_row,"{\"event\":\"native_sample_profile\",\"started\":%s}\n",started?"true":"false");Log(sample_row);}}
                // cD6: the slot count the renderer opened its frame ring with.
                Log(renderer.FrameSlots()==2?"{\"event\":\"native_frame_ring\",\"slots\":2}\n":"{\"event\":\"native_frame_ring\",\"slots\":1}\n");
                // C4-2: the main view keeps its prepared draws (no-persistent-draws.flag absent; any slot count).
                {char kept_row[200]{};sprintf_s(kept_row,"{\"event\":\"native_persistent_draws_mode\",\"enabled\":%s,\"flag_off\":%s,\"slots\":%u,\"sweep_prepares\":8}\n",
                    !native_kept_draws_disabled?"true":"false",
                    native_kept_draws_disabled?"true":"false",renderer.FrameSlots());Log(kept_row);}
                LogNativeAdapter();StartNativeMemoryBallast();   // I-0084: video-memory-budget-mb.txt only (announced runs)
                output_width=start_width;output_height=start_height;   // Fable review: the engine's size S (above), not the clamped client
            }
            opened=true;
            StartNativeBorderless();   // I-0076: borderless.flag
            InitializeNativeCommonImages();
            std::array<wchar_t,32768> executable{};
            const auto chars=GetModuleFileNameW(nullptr,executable.data(),static_cast<DWORD>(executable.size()));
            if(!chars || chars>=executable.size())Refuse("primitive_program_executable_path");
            std::ifstream original(std::filesystem::path(executable.data()),std::ios::binary|std::ios::ate);
            if(!original || original.tellg()!=std::streamoff{88960032})Refuse("primitive_program_executable_extent");
            std::vector<uint8_t> original_bytes(88960032);original.seekg(0);
            if(!original.read(reinterpret_cast<char*>(original_bytes.data()),static_cast<std::streamsize>(original_bytes.size())) ||
               frame::DecodeOriginalPrimitiveProgramBank(original_bytes,{1,1},primitive_programs)!=frame::PrimitiveProgramStatus::Ready)
                Refuse("primitive_program_original_cpu_admission");
            char programs_row[224]{};sprintf_s(programs_row,"{\"event\":\"native_primitive_programs_cpu_owned\",\"programs\":%zu,\"mode\":3,\"original_gpu_objects\":false,\"pipelines_ready\":false}\n",primitive_programs.ProgramCount());Log(programs_row);
        } else if(GetCurrentThreadId()!=owner_thread) { Refuse("bootstrap_thread_changed"); }
        Clear();
        if(!cpu_registered){InitializeNativeSimulationTime();RegisterCpuService(step);}
    } catch(const std::exception& e) {NativeFailure(e.what());}
}
void __fastcall Views(void* step) noexcept {
    if(control->mode==bootstrap::Mode::Reference) {view_original(step);return;}
    try {
        if(control->stop) {Close();return;}
        if(!opened || !cpu_registered || GetCurrentThreadId()!=owner_thread)Refuse("native_view_owner_unavailable");
        InterlockedIncrement64(&control->phase_calls);Observe(step,"native_views","enter");
        const auto receiver=reinterpret_cast<uintptr_t>(step);uintptr_t repository{};BYTE state{};
        if(!Read(receiver+0xE8,&repository,8) || !repository || !Read(repository+0x70,&state,1))Refuse("view_shader_repository_unavailable");
        if(state!=4)return;
        int current{},progress{};BYTE pending{};uintptr_t table{},callback{};
        if(!Read(receiver+0x50,&current,4) || !Read(receiver+0x54,&progress,4) || !Read(receiver+0x58,&pending,1) ||
            current<0 || current>=19 || progress!=current || pending || view_system_owned ||
            !Read(receiver+0x10,&table,8) || !Read(table+static_cast<uintptr_t>(current)*16,&callback,8) ||
            callback!=image+0xD59E20)Refuse("native_view_phase_mismatch");
        uint32_t width{},height{};frame::window::WindowSnapshot window{};
        // R13 cR3 (X4): the renderer's view state is read with the render worker idle (quiesced before the lock).
        if(render_worker)render_worker->Quiesce();
        {
            std::lock_guard<std::mutex> submission(gpu_submissions);
            if(!renderer.ReadStartupView(width,height))Refuse("native_view_not_resident");
        }
        if(render_worker)render_worker->Resume();
        // Fable review: the window is compared with the client recorded at start (the presenter may be the engine's size S,
        // which DXGI stretches into a smaller client), so only a real window change refuses.
        if(!frame::window::SnapshotWindow(reinterpret_cast<void*>(image),0,window) ||
            window.client_width!=start_client_width || window.client_height!=start_client_height)Refuse("native_view_window_extent_changed");
        (void)width;(void)height;
        // The common renderer owns the main loading view and its presentation.
        // Original nine-context/GX view graph globals stay null. Additional
        // scene views belong to native pass composition when actual work arrives.
        view_system_owned=true;
        char row[320]{};sprintf_s(row,"{\"event\":\"native_view_system_owned\",\"width\":%u,\"height\":%u,\"views\":1,\"shader_repository_state\":%u,\"scene_ready\":false,\"legacy_graph\":false}\n",width,height,state);Log(row);
        reinterpret_cast<Phase>(image+0xD5B250)(step);*reinterpret_cast<BYTE*>(receiver+0x58)=1;
        Observe(step,"native_views","leave");
    } catch(const std::exception& e) {NativeFailure(e.what());}
}
void RequestCpuResources(void* step) {
    const auto receiver=reinterpret_cast<uintptr_t>(step);
    int current{},progress{};BYTE pending{};uintptr_t table{},callback{},manager{},vtable{},submit{};
    uintptr_t slots[5]{};
    if(!Read(receiver+0x50,&current,4) || !Read(receiver+0x54,&progress,4) || !Read(receiver+0x58,&pending,1) ||
        current<0 || current>=19 || progress!=current || pending ||
        !Read(receiver+0x10,&table,8) || !Read(table+static_cast<uintptr_t>(current)*16,&callback,8) ||
        callback!=image+0xD59E90 || !Read(receiver+0x100,&slots[0],8) || !Read(receiver+0x120,&slots[1],8) ||
        !Read(receiver+0x148,&slots[2],24) || slots[0] || slots[1] || slots[2] || slots[3] || slots[4])
        Refuse("cpu_resource_request_phase_mismatch");
    if(!Read(image+0x47559C8,&manager,8) || !manager || !Read(manager,&vtable,8) ||
        !Read(vtable+0x18,&submit,8) || submit<image)Refuse("cpu_resource_manager_unavailable");
    // Steam protects these two CPU functions on disk. Pins come from separate
    // authenticated original observations, and are checked only after unpacking.
    for(const auto& pin:loaded_request_pins) {
        BYTE prefix[16]{};
        if(!Read(image+pin.rva,prefix,sizeof(prefix)) || std::memcmp(prefix,pin.bytes,sizeof(prefix)))
            Refuse("loaded_cpu_request_identity_mismatch");
    }
    char manager_row[192]{};sprintf_s(manager_row,"{\"event\":\"cpu_resource_manager\",\"vtable_rva\":%llu,\"submit_rva\":%llu}\n",
        static_cast<unsigned long long>(vtable-image),static_cast<unsigned long long>(submit-image));Log(manager_row);
    using Request=uintptr_t(__fastcall*)(uintptr_t,const wchar_t*,uintptr_t);
    const auto request=[&](DWORD rva,const wchar_t* name,const char* family,int index) {
        const auto handle=reinterpret_cast<Request>(image+rva)(manager,name,0);
        if(!handle)Refuse("cpu_resource_request_returned_null");
        char row[256]{};sprintf_s(row,"{\"event\":\"cpu_resource_requested\",\"family\":\"%s\",\"index\":%d,\"entry_rva\":%lu,\"handle\":%llu,\"native_ready\":false}\n",
            family,index,rva,static_cast<unsigned long long>(handle));Log(row);
        return handle;
    };
    *reinterpret_cast<uintptr_t*>(receiver+0x100)=request(0x506320,reinterpret_cast<const wchar_t*>(image+0x2959500),"shader",0);
    // Preserve the real callback's overwrite; do not manufacture an extra ref.
    *reinterpret_cast<uintptr_t*>(receiver+0x100)=request(0x506450,reinterpret_cast<const wchar_t*>(image+0x2959550),"debug_shader",0);
    *reinterpret_cast<uintptr_t*>(receiver+0x120)=request(0x5052F0,reinterpret_cast<const wchar_t*>(image+0x29595A0),"mtd",0);
    for(const uint32_t index:{1u,2u,3u,4u,5u,12u,6u,10u}) {
        struct EngineString { BYTE inline_bytes[16];uint64_t length,capacity;uintptr_t allocator;BYTE flag;BYTE pad[7]; };
        static_assert(sizeof(EngineString)==0x30);
        EngineString name{};
        const auto result=reinterpret_cast<void*(__fastcall*)(void*,uint32_t)>(image+0xD27D30)(&name,index);
        if(result!=&name || name.length>256 || !name.allocator || name.capacity<name.length)
            Refuse("gparam_cpu_name_contract");
        uintptr_t chars=reinterpret_cast<uintptr_t>(name.inline_bytes);
        if(name.capacity>7)std::memcpy(&chars,name.inline_bytes,sizeof(chars));
        std::array<wchar_t,257> owned_name{};
        if(!Read(chars,owned_name.data(),static_cast<size_t>(name.length+1)*2) || owned_name[name.length])
            Refuse("gparam_cpu_name_unreadable");
        const auto handle=request(0x503E60,owned_name.data(),"gparam",static_cast<int>(index));
        if(name.capacity>7) {
            uintptr_t heap_vtable{},release{};
            if(!Read(name.allocator,&heap_vtable,8) || !Read(heap_vtable+0x68,&release,8) || !release)
                Refuse("gparam_cpu_name_heap_unavailable");
            reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t)>(release)(name.allocator,chars);
        }
        auto vector=reinterpret_cast<uintptr_t*>(receiver+0x148);
        if(vector[1]==vector[2])reinterpret_cast<void(__fastcall*)(uintptr_t*,size_t)>(image+0x6120F0)(vector,1);
        if(!vector[0] || vector[1]<vector[0] || vector[1]>=vector[2])Refuse("gparam_cpu_vector_unavailable");
        *reinterpret_cast<uintptr_t*>(vector[1])=handle;vector[1]+=sizeof(uintptr_t);
    }
    cpu_resources_requested=true;
    Log("{\"event\":\"cpu_resources_requested\",\"count\":11,\"render_ready\":false,\"phase_advanced\":false}\n");
}
void __fastcall NativeUiFactory(uintptr_t manager) noexcept {
    try {
        if(!opened || !native_resource_consumers)Refuse("native_ui_factory_without_resource_services");
        const std::lock_guard<std::mutex> lock(ui_sessions_lock);
        if(const auto found=ui_sessions.find(manager);found!=ui_sessions.end()){
            uintptr_t hal{},loader{};
            if(!Read(manager+0x980,&hal,8) || !Read(manager+0x988,&loader,8) || hal!=found->second->Hal() || loader!=found->second->Loader())
                Refuse("native_ui_factory_owned_publication_changed");
            return; // A later genuine CPU job reaches the already owned factory.
        }
        // I-0137 (G5): the UI mesh items' growth, a row at each power of two from 4,096 live items.
        bootstrap::SetUiMeshItemsWitness([](size_t live){char row[96]{};sprintf_s(row,"{\"event\":\"native_ui_mesh_items\",\"live\":%zu}\n",live);Log(row);});
        auto session=std::make_shared<bootstrap::GameUiSession>(image,manager,textures,gpu,gpu_submissions,1,&Refuse);
        char row[256]{};sprintf_s(row,"{\"event\":\"native_ui_session_created\",\"manager\":%llu,\"hal\":%llu,\"loader\":%llu,\"thread\":%lu,\"legacy_gpu\":false}\n",
            static_cast<unsigned long long>(manager),static_cast<unsigned long long>(session->Hal()),
            static_cast<unsigned long long>(session->Loader()),GetCurrentThreadId());Log(row);
        ui_sessions.emplace(manager,std::move(session));
    }catch(const std::exception& e){char row[320]{};sprintf_s(row,"{\"event\":\"native_ui_session_failure\",\"message\":\"%s\"}\n",e.what());Log(row);Refuse("native_ui_session_creation_failed");}
}
void __fastcall NativeUiManagerDestroy(uintptr_t manager) noexcept {
    // Genuine normal UI teardown (E50160 -> E4D720 -> E59260 -> E586E0).
    // Its destructor releases HAL980 by shared release, wrapper978 by virtual0
    // and loader988 by virtual8 plus heap68. The native session owns HAL and
    // wrapper, so it withdraws all three publications first: the loader
    // through that same genuine contract, HAL/wrapper through their native
    // owners. The original destructor then retires its remaining CPU state.
    try {
        std::shared_ptr<bootstrap::GameUiSession> session;
        {const std::lock_guard<std::mutex> lock(ui_sessions_lock);
            if(const auto found=ui_sessions.find(manager);found!=ui_sessions.end()){session=std::move(found->second);ui_sessions.erase(found);}}
        if(session){
            std::unique_lock<std::timed_mutex> preparation(ui_frame_preparation_lock,std::defer_lock);
            if(!preparation.try_lock_for(std::chrono::seconds(2)))Refuse("native_ui_session_destroy_preparation_contended");
            session->Close();session.reset();
            uintptr_t wrapper{},hal{},loader{};
            if(!Read(manager+0x978,&wrapper,8) || !Read(manager+0x980,&hal,8) || !Read(manager+0x988,&loader,8) || wrapper || hal || loader)
                Refuse("native_ui_session_publications_retained");
            char row[256]{};sprintf_s(row,"{\"event\":\"native_ui_session_withdrawn\",\"manager\":%llu,\"thread\":%lu,\"boundary\":\"genuine_manager_destructor\"}\n",
                static_cast<unsigned long long>(manager),GetCurrentThreadId());Log(row);
        }
    }catch(const std::exception& e){char row[320]{};sprintf_s(row,"{\"event\":\"native_ui_session_withdraw_failure\",\"message\":\"%s\"}\n",e.what());Log(row);Refuse("native_ui_session_withdraw_failed");}
    ui_manager_destroy_original(manager);
}
// I-0137 (I-0120 G6 and A3's sequel, « skip the object and go on »): the original 140E49320 returns 0 when its manager makes no
// image from the texture (and when its 0x70-byte allocation fails). Natively the same answer, with a counted row, for an asset
// that has no resident native texture (a texture A3 left empty), for a holder of another class and for an image the UI set
// does not take because of what it is (its format) or of the set's bound (PrimitiveTextureLimit). The one caller, 140E48980
// (the repository's lookup by name), stores an image only when it is not 0, so a 0 is asked again at the next lookup. A refusal
// stays for the module's own failures (no session for the manager, a closed owner, a set that refuses an import it has room for).
// I-0137 (I-0120 A2, in part): a movie frame texture without its native target answers 0 the same way, not the SYSTEX_BLACK
// stand-in (D-0197) it took before: its witness (i66-native-i137-a2-black and -none, the character creation) met that case, the
// movie frame imported after the opening movie's close, and showed the creation screen alike afterwards with and without the
// stand-in (the transition's own frames not filmed). I-0140 tranche 9: a movie proxy without a frame (before its movie's open
// and after its close) imports its inner texture, as the original's (below; the credits' plate), which reverses that A2 case. I-0137 tranche 4: the menu offscreen proxy's stand-in removed too (it answers
// 0 and is asked again): the Status page and Load Game witness (i66-native-i137-t4-status) made 12 proxy targets and imported none
// without its target, as every current portrait run (tranche 3's final review).
uintptr_t __fastcall NativeUiTextureImport(uintptr_t,uintptr_t name,uintptr_t manager,uintptr_t holder) noexcept {
    if(!manager || !holder)return 0; // Original optional missing-source contract.
    uintptr_t source_image{},prefix{};
    try {
        uintptr_t table{},texture{},heap{},heap_table{},allocate{};
        if(!Read(holder,&table,8) || table!=image+0x29559E8 || !Read(holder+0x10,&texture,8)){NoteNativeSkip("ui_import_holder_family",holder);return 0;}
        auto resident=textures.Acquire(texture);
        // 1163: a menu offscreen model render's proxy (CSOffscreenGxTexture, vtable 14295DA78: SYSTEX_Menu_StatusBody,
        // _Profile00.., _ChrMake*) has no native image until its scene is built (Native1162: the Status screen stopped here):
        // the menu's import answers 0 until then (I-0120 A2, I-0137 tranche 4: no SYSTEX_BLACK stand-in).
        // 1165: a proxy whose scene NativeOffscreenSceneGraph built gets its own native target, cleared to the
        // view background (NativeOffscreenBackground, 1173) until the native offscreen draw renders into it.
        if(!resident){std::optional<NativeOffscreenProxy> owned;
            {const std::lock_guard hold(native_offscreen_lock);if(const auto found=native_offscreen_proxies.find(texture);found!=native_offscreen_proxies.end())owned=found->second;}
            if(owned){resident=textures.AdmitOffscreenTarget(texture,owned->width,owned->height,NativeOffscreenBackground(owned->gparam_id));
                static LONG64 reported{};if(InterlockedIncrement64(&reported)<=12){char row[200]{};
                    sprintf_s(row,"{\"event\":\"native_ui_offscreen_target\",\"width\":%u,\"height\":%u,\"offscreen_draw\":false}\n",owned->width,owned->height);Log(row);}}}
        // I-0140 (the credits' « MOVIE » plate, the tour's i139-t5-flag20-nat; a regression of I-0137 tranche 3, 2026-11-12): a movie
        // frame proxy (CSMovieGxTexture, vtable 1429A4968) has no native target before its movie's open and after its close
        // (NativeMoviePlayerClose retires it; between the open and the first frame the target is admitted cleared black, as the
        // original's forward is), and a 0 here left the
        // credits page without an image for the movie screen's per-update import to replace (the October witness
        // i66-native-2065-credits: the stand-in imported before the open, then the 1920x1080 target at each update). The original's
        // proxy forwards to its inner texture until its first frame (MENUS-MAP 9: +0x10, EADB80 sets it to [[144792E58]+0x60], the
        // texture D4A2D0 registers as « SYSTEX_BLACK »: READ, GhidraMCP 2026-11-15; its vt+0x20, EADCD0, answers [+0x18]+8 while a
        // frame is set and jumps to the inner texture's vt+0x20 otherwise, VERIFIED in its bytes), so E49320, which takes the
        // holder's vt+0x20 texture, imports that inner texture then: the same natively, the inner texture's native record, or the
        // native SYSTEX_BLACK common image when the inner one is that entry's. After the close too: the original's STEP_Finish_Play
        // EAF670 publishes a null frame (EADE50 clears +0x18), so the proxy forwards to its inner texture again (READ, 2026-11-15).
        // The menu offscreen proxy (295DA78, D6C250) forwards
        // the same way, but natively its 0 and the next lookup reach its own target (I-0137 tranche 4's Status witness), so left.
        if(!resident && texture){uintptr_t proxy_table{},inner{};
            if(Read(texture,&proxy_table,8) && proxy_table==image+0x29A4968 && Read(texture+0x10,&inner,8) && inner){
                uintptr_t black_entry{},black{};if(Read(image+0x4792E58,&black_entry,8) && black_entry)Read(black_entry+0x60,&black,8);   // the step review's L1
                resident=textures.Acquire(inner);
                if(!resident && black && black==inner)
                    if(const auto common=AcquireNativeCommonImages())for(const auto& [system_name,record]:common->system)
                        if(system_name==L"SYSTEX_BLACK"){resident=record;break;}
                static LONG64 reported{};if(InterlockedIncrement64(&reported)<=8){char row[200]{};
                    sprintf_s(row,"{\"event\":\"native_ui_movie_proxy_inner\",\"inner_is_black\":%s,\"resident\":%s}\n",black && black==inner?"true":"false",resident?"true":"false");Log(row);}}}
        if(!resident || !resident->resident || !resident->content){
            // 1171 witness: which texture (its class and the imported name) is unavailable.
            static LONG64 unavailable{};if(InterlockedIncrement64(&unavailable)<=32){
            uintptr_t texture_table{};if(texture)Read(texture,&texture_table,8);
            std::array<wchar_t,64> wide{};std::array<char,64> narrow{};if(name){Read(name,wide.data(),sizeof(wide)-2);Read(name,narrow.data(),sizeof(narrow)-1);}
            std::string label;for(const auto ch:wide){if(!ch)break;label.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');}
            std::string bytes;for(const auto ch:narrow){if(!ch)break;bytes.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');}
            char row[512]{};sprintf_s(row,"{\"event\":\"native_ui_asset_unavailable\",\"texture\":%s,\"texture_table_rva\":%lld,\"record\":%s,\"name_wide\":\"%.60s\",\"name_narrow\":\"%.60s\"}\n",
                texture?"true":"false",texture_table?static_cast<long long>(texture_table-image):-1ll,resident?"true":"false",label.c_str(),bytes.c_str());Log(row);}
            NoteNativeSkip("ui_import_asset_unavailable",texture);return 0;}
        const char* not_taken="no_session";
        {const std::lock_guard<std::mutex> lock(ui_sessions_lock);
            for(const auto& [receiver,session]:ui_sessions){(void)receiver;
                if(session->TextureManager()==manager){source_image=session->ImportTexture(resident,&not_taken);break;}}}
        if(!source_image){
            // The data's reasons are skipped; the module's own (no session, a closed owner, a set refusing with room left) refuse.
            if(!std::strcmp(not_taken,"format")){NoteNativeSkip("ui_import_format_not_sampled",texture);return 0;}
            if(!std::strcmp(not_taken,"set_full")){NoteNativeSkip("ui_import_set_full",texture);return 0;}
            throw std::runtime_error(std::string("native UI imported image family: ")+not_taken);}
        if(!Read(image+0x4936CC0,&heap,8) || !Read(heap,&heap_table,8) || !Read(heap_table+0x50,&allocate,8))
            throw std::runtime_error("native UI import CPU heap");
        prefix=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,size_t,uintptr_t)>(allocate)(heap,0x70,0);
        if(!prefix){reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0x1BB6D00)(source_image);NoteNativeSkip("ui_import_allocation_failed",texture);return 0;}
        const auto image_table=*reinterpret_cast<uintptr_t*>(source_image);
        const auto format=reinterpret_cast<uint32_t(__fastcall*)(uintptr_t)>(*reinterpret_cast<uintptr_t*>(image_table+0x20))(source_image);
        const std::array<uint32_t,2> size{resident->description.width,resident->description.height};
        // Actual CPU ImageSource/name construction and refcounts; the native
        // image supplies genuine CPU format/extent and a resident D3D12 lease.
        using Construct=uintptr_t(__fastcall*)(uintptr_t,uintptr_t,uint32_t,const uint32_t*,uint32_t,uintptr_t,uintptr_t);
        reinterpret_cast<Construct>(image+0xE478F0)(prefix,name,format,size.data(),0x100,source_image,0);
        reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0x1BB6D00)(source_image);source_image=0;
        // I-0140 tranche 9's final review (L7): the movie screen imports its frame at every update (26,031 rows in October); its step
        // review (M1): MENUS-MAP's check counts these rows at a menu opening, so every import of a texture generation other than the
        // last one written keeps its row, and a repeat of the same generation one in every 1000; « n » numbers the imports.
        static std::atomic<uint64_t> imported_rows{},last_generation{~0ull};const auto imported_n=imported_rows.fetch_add(1)+1;
        if(last_generation.exchange(resident->generation)!=resident->generation || imported_n%1000==0){char row[280]{};sprintf_s(row,"{\"event\":\"native_ui_asset_image_imported\",\"n\":%llu,\"texture_generation\":%llu,\"width\":%u,\"height\":%u,\"format\":%u,\"shared_native_resource\":true,\"original_gpu_input\":false}\n",
            static_cast<unsigned long long>(imported_n),resident->generation,size[0],size[1],format);Log(row);}
        return prefix;
    }catch(const std::exception& error){
        if(source_image)reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0x1BB6D00)(source_image);
        char row[320]{};sprintf_s(row,"{\"event\":\"native_ui_asset_import_error\",\"reason\":\"%s\"}\n",error.what());Log(row);
        Refuse("native_ui_asset_import_failed");
    }
}
void RegisterNativeCpuTextureScopes() {
    uintptr_t registry{},events{};
    if(!Read(image+0x4792F58,&registry,8) || !registry || !Read(image+0x4798338,&events,8) || !events)Refuse("native_cpu_texture_registry_unavailable");
    // D4A2D0 binds named render texture scopes to old GX's target array.
    // This CPU registry also already contains authored loaded texture scopes.
    // Retain the genuine scopes and only accept nonempty texture sources that
    // TextureIngress independently owns. Native render targets remain open.
    constexpr DWORD keys[]{0x2955558,0x2955578,0x2955598,0x29555B0,0x29555C8,0x29555E8,0x2955608,
        0x2955628,0x2955640,0x2955658,0x2955670,0x2955698,0x29556C8,0x29556E8};
    for(size_t n=0;n<std::size(keys);++n){
        const auto global=image+0x4792E50+n*8;uintptr_t prior{};
        if(!Read(global,&prior,8) || prior)Refuse("native_cpu_texture_scope_not_pristine");
        const auto scope=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t)>(image+0xD4A120)(registry,image+keys[n],0);
        uintptr_t table{},holder{},sink{};
        const bool fields=scope && Read(scope,&table,8) && Read(scope+0x60,&holder,8) && holder && Read(holder+0x10,&sink,8);
        uintptr_t holder_table{},sink_table{};if(holder)Read(holder,&holder_table,8);if(sink)Read(sink,&sink_table,8);
        char row[512]{};sprintf_s(row,"{\"event\":\"native_cpu_texture_scope_state\",\"index\":%zu,\"scope\":%llu,\"fields_read\":%s,\"table_rva\":%llu,\"holder_table_rva\":%llu,\"sink\":%llu,\"sink_table_rva\":%llu}\n",
            n,static_cast<unsigned long long>(scope),fields?"true":"false",static_cast<unsigned long long>(table-image),
            static_cast<unsigned long long>(holder_table-image),static_cast<unsigned long long>(sink),static_cast<unsigned long long>(sink?sink_table-image:0));Log(row);
        if(!fields || (table!=image+0x2955DD8 && table!=image+0x2956900) || holder_table!=image+0x29559E8)
            Refuse("native_cpu_texture_scope_contract");
        if(sink){
            const auto owned=textures.Acquire(sink);
            if(sink_table!=image+0x3D1D528 || !owned || !owned->resident || !owned->content || !owned->resource)
                Refuse("native_cpu_texture_scope_source_not_owned");
        }
        *reinterpret_cast<uintptr_t*>(global)=scope;
    }
    Log("{\"event\":\"native_cpu_texture_scopes_registered\",\"scopes\":14,\"genuine_cpu_objects\":true,\"original_gpu_contexts\":false,\"native_render_targets_complete\":false}\n");
}
void PublishNativeSceneRoot() {
    uintptr_t prior{},heap{};
    if(!Read(image+0x4961B50,&prior,8) || prior || native_scene_root)Refuse("native_scene_root_not_pristine");
    if(!Read(image+0x4961B80,&heap,8) || !heap)Refuse("native_scene_root_heap");
    const auto storage=reinterpret_cast<uintptr_t(__fastcall*)(size_t,size_t,uintptr_t)>(image+0x1777A00)(0x53B0,0x10,heap);
    if(!storage)throw std::bad_alloc();
    // 213A0 calls 20350, whose code is encrypted at rest (overlay935): construct
    // only once its decrypted code is present.
    static constexpr BYTE transform_code[18]{0x33,0xD2,0xC7,0x41,0x04,0x00,0x00,0x80,0x3F,0x4C,0x8B,0xC9,0x89,0x11,0x4D,0x8D,0x41,0x60};
    BYTE live[sizeof(transform_code)]{};
    if(!Read(image+0x20350,live,sizeof(live)) || std::memcmp(live,transform_code,sizeof(live)))Refuse("native_scene_root_code_not_decrypted");
    reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t)>(image+0x213A0)(storage,heap);
    uintptr_t published{},device[2]{};
    if(!Read(image+0x4961B50,&published,8) || published!=storage ||
       !Read(storage+0x4DD0,device,sizeof(device)) || device[0] || device[1])Refuse("native_scene_root_constructor");
    // Optimisation review (the per-instance SH, Native2243 against Reference2241): the global byte 144961ABC selects the probe
    // volumes' layout. The original's GX scene set-up D573B0 (from D59E90, not run natively) sets it to 1 once the root exists;
    // with it at 0 the map's probe volume (1D2D0 -> F3AA0) builds the other layout and keeps an empty bounding box, so the probe
    // pick 1D650 never overlaps it and every character and object lost its probe SH. Its readers (F3AA0, F47D0, F5840) are pure
    // CPU over the volume and the map's probe file. With picks found, 1D650 also sets the suppress bytes of the entities it
    // lights (E+0xA5D, item+0x71A), which the main walk honours as the original's views do (AdmitNativeModelCandidateBody).
    *reinterpret_cast<volatile uint8_t*>(image+0x4961ABC)=1;
    // Wave-1 closure (FOG-BYTE-4961ABD, Reference2311 / Native2312): D573B0 also sets the global byte 144961ABD to 1, for the
    // whole session and every zone; the engine's cbFog builder 14E900 then takes the height-fog factor from the fog set's
    // +0x18 instead of 1 / +0x14 with the sign of byte +0x1C. Left at 0 natively, the height fog took that other branch
    // (m45: -0.01 against the original's +0.01). D573B0's root fields +0x47F0..+0x4858 and +0x4E18
    // are read by the old renderer's 24740 only in the decompiled export (not run natively) and are not reproduced.
    *reinterpret_cast<volatile uint8_t*>(image+0x4961ABD)=1;
    // B5E00's CPU member +5368 (127010, read under the root's presence by E4460,
    // FB890 ...). +5358/+5360 (quality table and object) since 1255 (below); +20,
    // +4DF0..+4E08 and the display/device members are not built; every consumer
    // of +5360/+4E08 tests them.
    const auto allocate=[&](size_t bytes,size_t alignment){
        const auto object=reinterpret_cast<uintptr_t(__fastcall*)(size_t,size_t,uintptr_t)>(image+0x1777A00)(bytes,alignment,heap);
        if(!object)throw std::bad_alloc();return object;};
    uintptr_t root_heap{};if(!Read(storage+0x10,&root_heap,8) || root_heap!=heap)Refuse("native_scene_root_heap_member");
    *reinterpret_cast<uintptr_t*>(storage+0x5368)=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uintptr_t)>(image+0x127010)(allocate(0x1F8,8),root_heap);
    // 1255 (I-0067 QUALITY-OBJECT-NATIVE Q1-Q2, Q5): +5358/+5360 as 220A0 builds them: the 0x24C quality table (219B0, a pure
    // CPU builder, into a scratch copy first: it writes 0x260 bytes), the levels 2 then eleven 3, and the 0x78 object
    // written from them (23790's rewrite) before both pointers are published (readers test the pointer, then read fields).
    {std::array<uint8_t,0x260> built{};reinterpret_cast<void(__fastcall*)(void*)>(image+0x219B0)(built.data());
        const auto table=allocate(bootstrap::QualityTableBytes,4),object=allocate(bootstrap::QualityObjectBytes,4);
        std::memcpy(reinterpret_cast<void*>(table),built.data(),bootstrap::QualityTableBytes);
        std::memset(reinterpret_cast<void*>(object),0,bootstrap::QualityObjectBytes);
        const std::array<int32_t,12> levels{2,3,3,3,3,3,3,3,3,3,3,3};
        if(!bootstrap::WriteQualityObject(reinterpret_cast<const uint8_t*>(table),levels,reinterpret_cast<uint8_t*>(object)))Refuse("native_quality_object_initial");
        std::memcpy(reinterpret_cast<void*>(storage+0x5320),levels.data(),sizeof(levels));std::memcpy(reinterpret_cast<void*>(storage+0x52F0),levels.data(),sizeof(levels));
        *reinterpret_cast<uint8_t*>(storage+0x5350)=0;
        *reinterpret_cast<uintptr_t*>(storage+0x5358)=table;*reinterpret_cast<uintptr_t*>(storage+0x5360)=object;
        Log("{\"event\":\"native_quality_object\",\"edge\":\"published\",\"levels\":[2,3,3,3,3,3,3,3,3,3,3,3]}\n");
        // O2: the snapshot follows the object from its publication, with the sampler bank of the current Texture selector:
        // the first collect after the root runs before the next render step and needs a bank to draw (review).
        NativeOptionSnapshot next;if(const auto current=AcquireNativeOptions())next=*current;
        next.levels=levels;std::memcpy(next.object.data(),reinterpret_cast<const void*>(object),next.object.size());next.object_valid=true;
        {uintptr_t settings{};std::array<int32_t,12> words{};const bool read=Read(image+0x4796298,&settings,8) && ReadNativeOptionLevels(settings,words);
            next.sampler_bank=NativeSamplerBankOf(read,static_cast<uint32_t>(words[0]));}
        PublishNativeOptions(std::move(next));}
    // 48B80 on +41C0: the genuine CPU constructors of its sub-objects, in its
    // order, on the manager's heap. Their GPU initializers (B27A0, 2FC10 ...
    // 187A50, 65790, 152050) are not run.
    struct Child {uint32_t offset;uint32_t bytes;uint32_t alignment;DWORD rva;};
    static constexpr Child children[]{
        {0x10,0xF0,0x10,0x17EF0},{0x18,0x2F8,8,0xB24E0},{0x20,0xA80,0x10,0x2F9B0},{0x28,0x270,0x10,0x366D0},
        {0x30,0x1E0,8,0x3AE30},{0xE8,0x260,8,0x3F720},{0x430,0x4D0,0x10,0x153FF0},{0x438,0x490,0x10,0x12DEE0},
        {0x440,0x180,8,0x44930},{0x448,0x238,8,0x1595C0},{0x450,0x300,0x10,0x15CC40},{0x458,0x400,8,0x161150},
        {0x460,0x3F0,0x10,0x1658D0},{0x468,0x2D0,0x10,0x169A20},{0x470,0x300,8,0x16CD30},{0x478,0x1D8,8,0x170E70},
        {0x480,0x268,8,0x1740B0},{0x488,0x1C8,8,0x177950},{0x490,0xAA8,8,0x17AAE0},{0x498,0x1B0,8,0x17DBE0},
        {0x4A0,0x2A0,0x10,0x180BA0},{0x4A8,0x268,8,0x183C90},{0x4D8,0x3F0,0x10,0x1877E0},{0x4B0,0x370,0x10,0x18ACA0},
        {0x4B8,0x1C0,8,0x18E480},{0x4C0,0x1F8,8,0x133490},{0x4C8,0x1B0,8,0x191470},{0x4D0,0x198,8,0x194330},
        {0x5C8,0x4F8,8,0x936E0}};
    const auto lighting=storage+0x41C0;uintptr_t lighting_heap{};
    if(!Read(lighting+8,&lighting_heap,8) || lighting_heap!=heap)Refuse("native_scene_root_lighting_heap");
    for(const auto& child:children){
        uintptr_t prior_child{};if(!Read(lighting+child.offset,&prior_child,8) || prior_child)Refuse("native_scene_root_child_not_pristine");
        const auto object=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uintptr_t)>(image+child.rva)(allocate(child.bytes,child.alignment),lighting_heap);
        if(!object)Refuse("native_scene_root_child_constructor");
        *reinterpret_cast<uintptr_t*>(lighting+child.offset)=object;
    }
    reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0x93CE0)(*reinterpret_cast<uintptr_t*>(lighting+0x5C8));
    // I-0140 (Friede's intro 45000000 crashed natively in its IngameDrawParam act): the GX parameter registry +0x5C8 (93CE0 sets
    // its 26 group slots +0x30..+0x94 to -1) has its slots written in the original by the 26 GPU initializers 48B80 calls after
    // 93CE0, each with the id 1F7090 returns for its group: the registry's size before its push, so the registration order (0 to 25;
    // 1F7090's 26 callers are those initializers, in 48B80's call order). Natively they do not run, so the slots stayed -1, and the
    // readers of the six slots +0x80..+0x94 that fall back to an uninitialised local (380A0, 387D0, 31A20, 3C900, 40DD0, 45CA0) wrote
    // +0x428/+0x490 out of bounds with it (the crash at 97600+0x1E). Those six are written here with the original's ids; the registry
    // stays empty (its groups' fields and editors are the initializers' work). Their 36 readers then: write +0x428/+0x490[id] and
    // insert keys in the maps +0x2E0/+0x330 (97600, 97740, 96560, 973C0, 6CAD0's bank +0x1C) as the original does, and find nothing in
    // +0x290/+0x2E0/+0x330/+0x3D8 to act on; the vectors +0x198+0x80 and +0x218 they index (1E1960, 1E1C20, 151810 ...) are filled by
    // the initializers' callees, so empty natively, and bounded; the registry's direct readers (964F0, 1F7D00, 1F72A0, 3F9E0) are only
    // the initializers. I-0140 tranche 5: the other 20 slots too. Their 80 readers (evidence/I-0140/ghidra-gx-slots20/slot_table.tsv)
    // are four a slot: the bind and its apply (reached only through 49D00, which BindNativeGparamScene replaces natively), the release
    // (through 49450, the children's release, INFERRED; whether it runs natively not read; its id indexes only the bounded, empty
    // +0x198+0x80 and +0x218) and the withdraw, inside 4A060, reachable natively (READ, not observed): D27040 (CE6350's layer-1 file
    // switch, LoadMapOverrideGparam 619BD0) and D26E40 (CE6230) take an area out of the holder's area list (written by the genuine
    // D26AB0 and D26C00) and call 4A060 without the +0x70 handle test D26800, D26DD0 and CDCCD0 make. With -1 each withdraw (Lens
    // Simulation's, 185280 through 4A060's tail, into 96B70) took a stale register or stack word as the group id and looked it up in the
    // map +0x2E0 that the six groups above fill: a stale word equal to one of their ids could clear that group's (area, bank) entry
    // (INFERRED). With the original's ids they find no node, no bind having run natively for those groups (READ, GhidraMCP:
    // evidence/I-0140/ghidra-gx-slots20/pilot-readings.md).
    {struct GroupSlot {uint32_t offset;int32_t id;};
        static constexpr GroupSlot slots[]{
            {0x88,0},{0x80,1},{0x84,2},{0x8C,3},{0x50,4},{0x30,5},{0x34,6},{0x3C,7},{0x54,8},{0x38,9},{0x4C,10},{0x48,11},{0x44,12},
            {0x40,13},{0x58,14},{0x74,15},{0x78,16},{0x7C,17},{0x94,18},{0x6C,19},{0x70,20},{0x90,21},{0x5C,22},{0x60,23},{0x64,24},{0x68,25}};
        static_assert(std::size(slots)==26);
        static_assert([]{uint32_t offsets{};uint32_t ids{};for(const auto& s:slots){if(s.offset<0x30 || s.offset>0x94 || (s.offset&3) || s.id<0 || s.id>25)return false;
            offsets|=1u<<((s.offset-0x30)/4);ids|=1u<<s.id;}return offsets==(1u<<26)-1 && ids==(1u<<26)-1;}(),"one id per slot, each slot once");
        const auto registry=*reinterpret_cast<uintptr_t*>(lighting+0x5C8);uint32_t written{};
        for(const auto& s:slots){int32_t slot{};if(!Read(registry+s.offset,&slot,4) || slot!=-1)Refuse("native_gx_param_group_slot_not_reset");
            *reinterpret_cast<int32_t*>(registry+s.offset)=s.id;++written;}
        char row[128]{};sprintf_s(row,"{\"event\":\"native_gx_param_group_ids\",\"slots\":%u,\"registry_groups\":0}\n",written);Log(row);}
    native_scene_root=storage;
    Log("{\"event\":\"native_scene_root_published\",\"genuine_class\":true,\"bytes\":21424,\"device\":false,\"display\":false,\"cpu_submanagers\":true,\"lighting_children\":29,\"gpu_initializers\":false}\n");
}
// 23410 (root frame update, from D57900 when the root exists): the root clock
// 126EA0 is genuine. The lighting manager's frame update 499C0 walks the old
// renderer pass objects (their GPU initializers never ran) and is an explicit
// omission: native passes own that work. +5368's 127240 and the device's
// 775C0 are GPU; 49622F8 is the old GX job owner (absent natively).
uint64_t native_scene_root_frames{};
void __fastcall NativeSceneRootFrame(uintptr_t root,float delta) noexcept {
    if(!root || root!=native_scene_root)Refuse("native_scene_root_frame_foreign");
    reinterpret_cast<void(__fastcall*)(uintptr_t,float)>(image+0x126EA0)(root+0x4E30,delta);
    if(++native_scene_root_frames==1){uintptr_t jobs{};Read(image+0x49622F8,&jobs,8);
        char row[256]{};sprintf_s(row,"{\"event\":\"native_scene_root_frame\",\"genuine_clock\":true,\"lighting_pass_update\":false,\"gx_jobs_present\":%s}\n",jobs?"true":"false");Log(row);}
}
void ConstructNativeResourceConsumers() {
    if(!native_scene_root)try{PublishNativeSceneRoot();}catch(const std::exception&){Refuse("native_scene_root_construction");}
    uintptr_t root{},heap{};
    if(!Read(image+0x4793348,&root,8) || !root || !Read(image+0x479C270,&heap,8) || !heap)Refuse("native_resource_root_unavailable");
    const auto construct=[&](size_t offset,size_t bytes,size_t alignment,DWORD rva){
        uintptr_t prior{};if(!Read(root+offset,&prior,8) || prior)Refuse("native_resource_child_not_pristine");
        const auto storage=reinterpret_cast<uintptr_t(*)(size_t,size_t,uintptr_t)>(image+0x1777A00)(bytes,alignment,heap);
        if(!storage)throw std::bad_alloc();
        const auto object=reinterpret_cast<uintptr_t(*)(uintptr_t)>(image+rva)(storage);
        if(object!=storage)Refuse("native_resource_cpu_constructor");
        *reinterpret_cast<uintptr_t*>(root+offset)=object;
    };
    // Genuine CPU prefixes/dictionaries consumed by later game tasks. Their
    // old GPU initialization methods are deliberately replaced by the native
    // shader/material/texture registries already serving real loading jobs.
    construct(0x50,0x14A0,0x10,0xD1E0A0);construct(0x58,0xCD0,0x10,0xD1ED70);
    construct(0x60,0x28,8,0xD6DE70);construct(0x68,0x30,8,0xD60ED0);
    {   // D573B0 also creates the model render manager +78: genuine CPU object
        // B1DC0(storage,heap) (vtable, heap, record pool header). Its GPU
        // initializer B1F60 -> B1E50 (400 old device queries 19C790 and two
        // old-API buffers +60/+68) is not run: occlusion queries and those
        // buffers are explicit initial omissions. The render service scene
        // (DFDAB0 -> DFEEB0 -> B2410 -> C9510) then builds its genuine drawable
        // scene from this manager when a world is entered (inventory917).
        uintptr_t prior{};if(!Read(root+0x78,&prior,8) || prior)Refuse("native_model_render_manager_not_pristine");
        const auto storage=reinterpret_cast<uintptr_t(*)(size_t,size_t,uintptr_t)>(image+0x1777A00)(0x70,8,heap);
        if(!storage)throw std::bad_alloc();
        const auto manager=reinterpret_cast<uintptr_t(*)(uintptr_t,uintptr_t)>(image+0xB1DC0)(storage,heap);
        if(manager!=storage)Refuse("native_model_render_manager_constructor");
        *reinterpret_cast<uintptr_t*>(root+0x78)=manager;
        Log("{\"event\":\"native_model_render_manager_cpu\",\"genuine_cpu_object\":true,\"occlusion_queries\":0,\"legacy_gpu_buffers\":false}\n");
    }
    ConstructNativeModelResolver(root,heap);
    RegisterNativeCpuTextureScopes();
    native_resource_consumers=true;
    Log("{\"event\":\"native_resource_consumers\",\"cpu_children\":4,\"native_asset_registries\":true,\"ui_factory_armed\":true,\"legacy_graph_constructed\":false,\"world_passes_complete\":false}\n");
}
void CompleteNativeResourcePhase(void* step) {
    const auto receiver=reinterpret_cast<uintptr_t>(step);
    uintptr_t service{};
    if(!Read(receiver+0x140,&service,8) || !service)Refuse("native_resource_continuation_services");
    //2355720/3A4AC0 initialize the optional debug-font D3D11 pipeline.
    //Keep that absent feature unpublished in this initial native build. The
    //game's actual Scaleform font loader is independent and remains required.
    Log("{\"event\":\"native_optional_visual_deferred\",\"family\":\"debug_font\",\"legacy_gpu_created\":false}\n");
    // I-0105 tranche 10: D59E90's root snow members (global_snow_a, _n, _r, _s; StoreNativeSurfaceTextures).
    {uintptr_t repository{};if(!Read(image+0x4792F58,&repository,8) || !repository)Refuse("native_surface_textures_repository_missing");
        StoreNativeSurfaceTextures(repository,{image+0x29595D8,image+0x2959638,image+0x29595F8,image+0x2959618});}
    // Genuine loading and actual native resource consumers are now available.
    // C8 enables native frame callbacks; it does not promise complete visuals.
    *reinterpret_cast<BYTE*>(service+0xC8)=1;
    reinterpret_cast<Phase>(image+0xD5B250)(step);*reinterpret_cast<BYTE*>(receiver+0x58)=1;
    Log("{\"event\":\"native_resources_completed\",\"native_frame_callbacks\":true,\"phase_advanced\":true,\"visual_coverage_complete\":false}\n");
}
void __fastcall ObserveNativeCpuWait(void* step) noexcept {
    if(control->stop && control->closed)return;
    static uint64_t previous=~uint64_t{},samples{};
    const auto receiver=reinterpret_cast<uintptr_t>(step);
    constexpr size_t offsets[]{0xC8,0xE0,0xE8,0xF0,0xF8,0x110,0x118,0x128,0x130,0x138};
    uint64_t states{};std::array<BYTE,10> values{};
    for(size_t n=0;n<values.size();++n){uintptr_t handle{};
        if(!Read(receiver+offsets[n],&handle,8) || (handle && !Read(handle+0x70,&values[n],1)))Refuse("native_cpu_wait_field_unreadable");
        if(!handle)values[n]=0xF;states|=uint64_t(values[n]&0xF)<<(n*4);
    }
    if(states!=previous && samples++<16){previous=states;char row[512]{};
        sprintf_s(row,"{\"event\":\"native_cpu_wait\",\"states_c8_e0_e8_f0_f8_110_118_128_130_138\":[%u,%u,%u,%u,%u,%u,%u,%u,%u,%u],\"missing_value\":15}\n",
            values[0],values[1],values[2],values[3],values[4],values[5],values[6],values[7],values[8],values[9]);Log(row);
    }
    // Recovered D5AAC0 is solely the genuine CPU resource-ready conjunction
    //and phase advancement; it contains no legacy rendering operation.
    cpu_wait_original(step);
}
void __fastcall NativeMainServicesStep(void* step) noexcept {
    try {
        if(control->stop && control->closed)return;
        if(!opened || !native_resource_consumers)return;
        uintptr_t cpu_service{},menus{},heap{};const auto task=reinterpret_cast<uintptr_t>(step);uint32_t current{};
        if(!Read(task+0x50,&current,4) || current!=7 || !Read(image+0x49647A0,&cpu_service,8) ||
           !Read(image+0x4763258,&menus,8) || !Read(image+0x479C258,&heap,8) || !heap)Refuse("native_main_services_contract");
        if(!cpu_service || !menus){static bool reported{};if(!reported){reported=true;char row[256]{};
            sprintf_s(row,"{\"event\":\"native_main_services_wait\",\"cpu49647a0_present\":%s,\"menus_present\":%s}\n",cpu_service?"true":"false",menus?"true":"false");Log(row);}return;}
        const auto ensure=[&](DWORD global,size_t bytes,DWORD ctor){uintptr_t object{};
            if(!Read(image+global,&object,8))Refuse("native_main_service_global_unreadable");
            if(object)return;
            const auto storage=reinterpret_cast<uintptr_t(__fastcall*)(size_t,size_t,uintptr_t)>(image+0x1777A00)(bytes,8,heap);
            if(!storage)throw std::bad_alloc();object=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+ctor)(storage);
            if(object!=storage)Refuse("native_main_service_constructor");*reinterpret_cast<uintptr_t*>(image+global)=object;
        };
        const auto call=[&](DWORD rva){reinterpret_cast<void(__fastcall*)()>(image+rva)();};
        const auto service=[&](DWORD global,DWORD rva){uintptr_t object{};
            if(!Read(image+global,&object,8) || !object)Refuse("native_main_cpu_service_missing");
            reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+rva)(object);
        };
        // Preserve EDF140's actual CPU initialization after real native and
        // CPU dependencies. 4799C98 is the native SFX manager (NativeSfxStartup).
        ensure(0x4751560,1,0x450370);ensure(0x47957B8,0xE8,0xDEEDD0);
        call(0xEDE120);ensure(0x4795348,0x18,0xDE9860);call(0x627760);
        const auto sound=reinterpret_cast<uintptr_t(__fastcall*)()>(image+0x814B50)();
        if(!sound)Refuse("native_main_audio_service_missing");reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0x8153B0)(sound);
        service(0x477DA08,0x82AF80);service(0x489D9B8,0xEE1460);service(0x489C7A8,0xEBF740);
        service(0x479B870,0x12C73DF);call(0xEDE0D0);
        reinterpret_cast<Phase>(image+0xEDF9B0)(step);*reinterpret_cast<BYTE*>(task+0x58)=1;
        Log("{\"event\":\"native_main_services_completed\",\"genuine_cpu_initializers\":true,\"sfx_visuals_present\":false,\"fake_sfx_manager\":false}\n");
    }catch(const std::exception& e){char row[320]{};sprintf_s(row,"{\"event\":\"native_main_services_failure\",\"message\":\"%s\"}\n",e.what());Log(row);Refuse("native_main_services_failed");}
}
void __fastcall NativeMainGraphicsWait(void* step) noexcept {
    if(control->stop && control->closed)return;
    if(!opened || !native_resource_consumers || !view_system_owned)return;
    const auto task=reinterpret_cast<uintptr_t>(step);uint32_t current{};uintptr_t other{};
    if(!Read(task+0x50,&current,4) || current!=4 || !Read(task+0x250,&other,8) || !other)Refuse("native_main_graphics_wait_contract");
    // Original EDF460 checks GX-root98 plus the genuine auxiliary CPU task.
    // Replace only graphics availability with actual native services. Keep
    // that independent CPU dependency and original MainStep advancement.
    if(!reinterpret_cast<bool(__fastcall*)(uintptr_t)>(image+0xDAE640)(other))return;
    reinterpret_cast<Phase>(image+0xEDF9B0)(step);*reinterpret_cast<BYTE*>(task+0x58)=1;
    Log("{\"event\":\"native_main_graphics_wait_completed\",\"native_services\":true,\"cpu_dependency_ready\":true,\"legacy_root_flag_written\":false,\"world_visuals_complete\":false}\n");
}
void __fastcall NativeUiStartupTask(void* step,uintptr_t frame) noexcept {
    try {
        if(control->stop && control->closed)return;
        if(!opened || !native_resource_consumers)return;
        const auto task=reinterpret_cast<uintptr_t>(step);uint32_t current{};uintptr_t container{},manager{};
        if(!Read(task+0x50,&current,4) || current!=1 || !Read(image+0x4798EF8,&container,8) || !container ||
           !Read(container+8,&manager,8) || !manager)Refuse("native_ui_startup_task_contract");
        // E500C0's legacy renderer wait is replaced by actual native startup
        // services. Preserve the CPU factory, real task counter and update.
        // No original readiness object/bit is synthesized or consulted.
        NativeUiFactory(manager);
        ui_task_progress_original(step);
        char row[256]{};sprintf_s(row,"{\"event\":\"native_ui_startup_completed\",\"task\":%llu,\"frame\":%llu,\"thread\":%lu,\"legacy_ready\":false}\n",
            static_cast<unsigned long long>(task),static_cast<unsigned long long>(frame),GetCurrentThreadId());Log(row);
        float delta{};
        if(!Read(frame+8,&delta,4) || !std::isfinite(delta))Refuse("native_ui_frame_delta_unreadable");
        reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t)>(image+0xE4DD70)(container,frame);
    }catch(const std::exception& e){char row[320]{};sprintf_s(row,"{\"event\":\"native_ui_startup_failure\",\"message\":\"%s\"}\n",e.what());Log(row);Refuse("native_ui_startup_failed");}
}
void __fastcall WaitUiCpuConsumers(void* step,uintptr_t frame) noexcept {
    if(control->stop && control->closed)return;
    uintptr_t menus{},container{};
    if(!Read(image+0x4763258,&menus,8) || !Read(image+0x4798EF8,&container,8))Refuse("native_ui_cpu_consumers_unreadable");
    if(menus){ui_consumers_original(step,frame);return;}
    //765630 acknowledges the genuine CPU menu/input controller created by
    //EDED40 ->763C10. Preserve that dependency instead of faking its object.
    static bool reported{};if(!reported){reported=true;Log("{\"event\":\"native_ui_wait_cpu_controller\",\"global_rva\":74855000,\"fake_controller\":false}\n");}
    if(container)reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t)>(image+0xE4DD70)(container,frame);
}
void __fastcall ObserveUiTaskProgress(void* step) noexcept {
    uintptr_t expected{};const auto receiver=reinterpret_cast<uintptr_t>(step);
    if(!ui_startup_task.compare_exchange_strong(expected,receiver) && expected!=receiver)
        Refuse("native_ui_progress_foreign_task");
    ui_task_progress_original(step);
    static LONG64 samples{};const auto count=InterlockedIncrement64(&samples);
    if(count<=16){uint32_t current{},pending{};const auto task=reinterpret_cast<uintptr_t>(step);
        if(!Read(task+0x50,&current,4) || !Read(task+0x54,&pending,4))Refuse("native_ui_task_counter_unreadable");
        char row[256]{};sprintf_s(row,"{\"event\":\"native_ui_task_progress\",\"task\":%llu,\"current\":%u,\"pending\":%u,\"return_rva\":%llu,\"thread\":%lu}\n",
            static_cast<unsigned long long>(task),current,pending,static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(_ReturnAddress())-image),GetCurrentThreadId());Log(row);
    }
}
void __fastcall NativeUiTaskDispatch(void* step,uintptr_t frame) noexcept {
    census::Note(census::UiTaskDispatch);   // R13 cR0
    const auto task=ui_startup_task.load(std::memory_order_acquire);
    // Before the first genuine E50820 progress event identifies the UI task,
    // cover the initial dispatcher scope too. Later foreign tasks retain their
    // original CPU dispatch and are outside this UI lifetime qualification.
    if(task && task!=reinterpret_cast<uintptr_t>(step)){ui_task_dispatch_original(step,frame);return;}
    if(control->stop)return;
    active_ui_dispatches.fetch_add(1,std::memory_order_acq_rel);
    struct Release {~Release(){active_ui_dispatches.fetch_sub(1,std::memory_order_acq_rel);}} release;
    // Close can win before the increment, but cannot race an admitted scope.
    if(control->stop)return;
    ui_task_dispatch_original(step,frame);
}
}  // namespace
