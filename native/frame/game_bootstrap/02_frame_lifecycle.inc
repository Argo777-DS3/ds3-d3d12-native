// Frame lifecycle: protected-code observation, the refusal exit, the frame job and its outcome (Clear, FinishNativeFrame),
// the common images and Close. Part of game_bootstrap.cpp's single translation unit (the order is there): it relies on
// every earlier part (01_state.inc before it).
#pragma once
#include "01_state.inc"
namespace {
void ObserveCpuRequestCode() noexcept {
    // This one request entry is protected in the installed file. Observe its
    // loaded CPU code in the separate original reference, without patching it
    // or inferring its behavior from the encrypted on-disk bytes.
    std::array<BYTE,512> code{};
    if(!Read(image+0x506450,code.data(),code.size())) {
        Log("{\"event\":\"cpu_request_code_unavailable\",\"rva\":5268560}\n");return;
    }
    char hex[1025]{};constexpr char digits[]="0123456789abcdef";
    for(size_t i=0;i<code.size();++i) {hex[2*i]=digits[code[i]>>4];hex[2*i+1]=digits[code[i]&15];}
    char row[1280]{};sprintf_s(row,"{\"event\":\"reference_cpu_request_code\",\"rva\":%u,\"bytes\":%zu,\"hex\":\"%s\"}\n",0x506450,code.size(),hex);Log(row);
}
void Observe(void* receiver,const char* name,const char* edge) noexcept {
    std::array<uint8_t,0x168> object{};
    const bool ok=Read(reinterpret_cast<uintptr_t>(receiver),object.data(),object.size());
    int32_t current{},progress{}; uintptr_t loading{}; BYTE state{};
    if(ok) {
        std::memcpy(&current,object.data()+0x50,4); std::memcpy(&progress,object.data()+0x54,4);
        std::memcpy(&loading,object.data()+0xF0,8);
    }
    const bool state_ok=Read(loading?loading+0x70:0,&state,1);
    char text[512]{};
    sprintf_s(text,"{\"event\":\"phase\",\"name\":\"%s\",\"edge\":\"%s\",\"thread\":%lu,"
        "\"tick\":%llu,\"receiver\":%llu,\"read\":%s,\"current\":%d,\"progress\":%d,"
        "\"pending\":%u,\"loading_state_read\":%s,\"loading_state\":%u}\n",name,edge,
        GetCurrentThreadId(),GetTickCount64(),static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(receiver)),
        ok?"true":"false",current,progress,object[0x58],state_ok?"true":"false",state);
    Log(text);
}
// I-0082 (DESIGN 3): a minidump for a native fault. The controller, the one process that survives the game, writes it from
// outside (game_bootstrap_ctl); when it does not take the request within 3 s the module writes it itself from a thread of its
// own. One request at a time (a second faulting thread waits for the first), at most 4 a run. No heap use before the dump.
volatile LONG crash_request_busy{};
enum class CrashPost {None,Served,Untaken};   // nothing posted; the controller took and finished it; nobody took it
// Its frame is small on purpose: on a stack overflow it runs on the exhausted stack, and the dump is then the controller's
// alone. Returns with crash_request_busy held unless None.
__declspec(noinline) CrashPost PostCrashRequest(bootstrap::CrashKind kind,uint32_t code,EXCEPTION_POINTERS* exception,const char* reason,HRESULT removed) noexcept {
    if(!control)return CrashPost::None;   // Setup admitted a version-3 controller before any hook could fault
    for(int waited=0;InterlockedCompareExchange(&crash_request_busy,1,0);++waited){if(waited>=12000)return CrashPost::None;Sleep(10);}
    auto& crash=control->crash;
    // The record always names the latest fault (the controller's final row and the player's message read it); the cap of 4
    // stops only the dump.
    crash.kind=kind;crash.code=code;crash.thread=GetCurrentThreadId();crash.exception=reinterpret_cast<uint64_t>(exception);
    crash.removed_reason=removed;strncpy_s(crash.reason,reason?reason:"",_TRUNCATE);
    if(crash.dumps>=4){InterlockedExchange(&crash_request_busy,0);return CrashPost::None;}
    crash.dump[0]=0;
    constexpr auto requested=static_cast<LONG>(bootstrap::CrashState::Requested),taken=static_cast<LONG>(bootstrap::CrashState::Taken);
    InterlockedExchange(&crash.state,requested);
    const auto asked=GetTickCount64();
    while(crash.state==requested && GetTickCount64()-asked<3000)Sleep(5);
    if(InterlockedCompareExchange(&crash.state,static_cast<LONG>(bootstrap::CrashState::Failed),requested)==requested)return CrashPost::Untaken;
    const auto taken_at=GetTickCount64();
    while(crash.state==taken && GetTickCount64()-taken_at<120000)Sleep(10);
    return CrashPost::Served;
}
// The fallback's state is static (the busy flag makes it exclusive): a writer thread that outlives its wait never writes into
// a reused stack frame.
struct FallbackDump {DWORD thread{};uint64_t exception{};wchar_t path[1100]{};bool written{};} fallback_dump;
DWORD WINAPI WriteFallbackDump(void*) {
    fallback_dump.written=crash::WriteDump(GetCurrentProcess(),GetCurrentProcessId(),fallback_dump.thread,fallback_dump.exception,fallback_dump.path,L"game_bootstrap.dll",bounded_run);
    return 0;
}
__declspec(noinline) void FinishCrashRequest(CrashPost post,bootstrap::CrashKind kind,uint32_t code,EXCEPTION_POINTERS* exception) noexcept {
    auto& crash=control->crash;
    if(post==CrashPost::Untaken){
        // No controller: <journal folder>\crashes\<journal name>-crash<N>.dmp, written here. Paths are cut, never overflowed
        // (an overflowing _s copy would end the process without a dump).
        auto& dump=fallback_dump;dump.thread=GetCurrentThreadId();dump.exception=reinterpret_cast<uint64_t>(exception);dump.written=false;
        wcsncpy_s(dump.path,control->journal,_TRUNCATE);
        wchar_t* slash=wcsrchr(dump.path,L'\\');wchar_t* other=wcsrchr(dump.path,L'/');if(other>slash)slash=other;
        wchar_t name[260]{};wcsncpy_s(name,slash?slash+1:dump.path,_TRUNCATE);if(wchar_t* dot=wcsrchr(name,L'.'))*dot=0;
        if(slash)slash[1]=0;else dump.path[0]=0;
        wcsncat_s(dump.path,L"crashes",_TRUNCATE);CreateDirectoryW(dump.path,nullptr);
        wchar_t file[300]{};swprintf_s(file,L"\\%.200s-crash%ld.dmp",name,crash.dumps+1);wcsncat_s(dump.path,file,_TRUNCATE);
        const HANDLE thread=CreateThread(nullptr,0,&WriteFallbackDump,nullptr,0,nullptr);
        // A writer that does not finish (a lock the faulting thread holds) must not let the game go on half suspended.
        if(thread && WaitForSingleObject(thread,120000)!=WAIT_OBJECT_0)TerminateProcess(GetCurrentProcess(),code?code:0xD3126607);
        if(thread)CloseHandle(thread);
        if(dump.written){wcsncpy_s(crash.dump,dump.path,_TRUNCATE);InterlockedIncrement(&crash.dumps);}
        InterlockedExchange(&crash.state,static_cast<LONG>(dump.written?bootstrap::CrashState::Written:bootstrap::CrashState::Failed));}
    char path[600]{};WideCharToMultiByte(CP_UTF8,0,crash.dump,-1,path,sizeof path-1,nullptr,nullptr);for(auto& c:path)if(c=='\\')c='/';else if(c=='"')c='_';
    char row[900]{};sprintf_s(row,"{\"event\":\"native_crash_dump\",\"kind\":%u,\"code\":\"0x%08X\",\"thread\":%lu,\"written\":%s,\"writer\":\"%s\",\"dumps\":%ld,\"path\":\"%s\"}\n",
        static_cast<unsigned>(kind),code,GetCurrentThreadId(),crash.state==static_cast<LONG>(bootstrap::CrashState::Written)?"true":"false",
        post==CrashPost::Served?"controller":"module",crash.dumps,path);Log(row);
    InterlockedExchange(&crash_request_busy,0);
}
void RequestCrashDump(bootstrap::CrashKind kind,uint32_t code,EXCEPTION_POINTERS* exception,const char* reason,HRESULT removed=S_OK) noexcept {
    if(const auto post=PostCrashRequest(kind,code,exception,reason,removed);post!=CrashPost::None)FinishCrashRequest(post,kind,code,exception);
}
// The single fault exit (refusals 0xD3126606, native failures 0xD3126607). I-0082: before it ends the process, a lost GPU is
// classified from the device (removed, or hung: the 5 s completion deadline passed), its DRED report logged when the device
// is removed, and the dump requested. A GPU fault Windows detects (TDR after 2 s, page fault) removes the device itself, with
// DRED's breadcrumbs; the 5 s deadline catches the stalls it does not see (a queue waiting on a fence nobody signals), where
// DRED records nothing (it records GPU faults only, not an application's RemoveDevice: I-0082 RESULT), so none is forced.
// A seam that returns (a test host that ends its own thread instead) still ends the process.
[[noreturn]] void ExitNative(UINT code,const char* reason,bool failure) noexcept {
    auto kind=code==0xD3126606 && !failure?bootstrap::CrashKind::Refusal:bootstrap::CrashKind::NativeFailure;HRESULT removed=S_OK;
    if(const auto device=gpu.device){
        const bool hang=gpu_hang_detected.load();removed=device->GetDeviceRemovedReason();
        if(hang)kind=bootstrap::CrashKind::GpuHang;else if(FAILED(removed))kind=bootstrap::CrashKind::DeviceRemoved;
        if(FAILED(removed))try{
            const auto report=DescribeRemoval(device.Get());
            char row[200]{};sprintf_s(row,"{\"event\":\"native_device_removed\",\"reason\":\"0x%08lX\",\"hang\":%s,\"dred\":%s}\n",static_cast<unsigned long>(removed),
                hang?"true":"false",gpu.RemovalDataActive()?"true":"false");Log(row);
            for(const auto& line:report.rows)Log((line+"\n").c_str());
        }catch(...){Log("{\"event\":\"native_device_removed\",\"report\":\"failed\"}\n");}}
    RequestCrashDump(kind,code,nullptr,reason,removed);
    exit_native(code);ExitProcess(code);
}
// The native_failure row; I-0082: its text is kept for the crash record of the refusal that follows it.
char last_native_failure[200]{};
void LogNativeFailure(const char* context,const char* what) noexcept {
    char row[512]{};sprintf_s(row,"{\"event\":\"native_failure\",\"message\":\"%s%.420s\"}\n",context,what);Log(row);
    strncpy_s(last_native_failure,what,_TRUNCATE);
}
[[noreturn]] void Refuse(const char* reason) noexcept {
    char row[256]{}; sprintf_s(row,"{\"event\":\"refusal\",\"reason\":\"%s\"}\n",reason); Log(row);
    InterlockedIncrement(&control->legacy_refused);
    // An unsupported original entry is never forwarded or reported successful.
    char cause[256]{};if(last_native_failure[0])sprintf_s(cause,"%.60s: %.190s",reason,last_native_failure);
    ExitNative(0xD3126606,cause[0]?cause:reason,cause[0]!=0);   // the exit code stays a refusal's; the record says failure
}
// I-0101 (WORKFLOW « Skip the object and go on »): one object of the game's data skipped where the module used to refuse;
// counted per reason, a row `native_object_skipped` its first four times and every 1,000th. Any thread.
void NoteNativeSkip(const char* reason,uintptr_t object=0) noexcept {
    static std::mutex lock;static std::unordered_map<const char*,uint64_t> counts;uint64_t n{};
    try{const std::lock_guard hold(lock);n=++counts[reason];}catch(...){return;}
    if(n>4 && n%1000)return;
    char row[256]{};sprintf_s(row,"{\"event\":\"native_object_skipped\",\"reason\":\"%.120s\",\"object\":%llu,\"times\":%llu}\n",reason,
        static_cast<unsigned long long>(object),static_cast<unsigned long long>(n));Log(row);
}
// I-0082: the native failure exit of every hook's catch (one copy instead of five).
[[noreturn]] void NativeFailure(const char* what) noexcept {
    LogNativeFailure("",what);InterlockedIncrement64(&control->faults);ExitNative(0xD3126607,what);
}
[[noreturn]] void __fastcall LegacyDevice() noexcept { Refuse("legacy_device_factory"); }
[[noreturn]] void __fastcall LegacyResources() noexcept { Refuse("legacy_resource_bootstrap"); }
[[noreturn]] void __fastcall LegacySwap() noexcept { Refuse("legacy_swapchain_create"); }
[[noreturn]] void __fastcall LegacyPresent() noexcept { Refuse("legacy_present"); }
[[noreturn]] void __fastcall LegacyTextureSource() noexcept {
    char row[160]{};sprintf_s(row,"{\"event\":\"unowned_texture_source\",\"caller_rva\":%llu}\n",
        static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(_ReturnAddress())-image));Log(row);
    Refuse("legacy_encoded_texture_source");
}
// 1087 witness: every 300 native frames, whether the character-update witness hooks are still
// in place (first byte E9 = MinHook jump) and the MoveMapStep words +08..+3C, so a silent
// witness distinguishes "the engine stopped calling" from "the hook was undone".
void ProbeWorldUpdateHooks(uint64_t frame) noexcept {
    constexpr DWORD rvas[]{0x8FA3E0,0x8DD2E0,0x88E1F0,0x8C1BC0,0xC48180,0xD01610,0x16000,0x65CAC0,0x888C00};
    std::string bytes;for(const auto rva:rvas){uint8_t b{};Read(image+rva,&b,1);char cell[24]{};sprintf_s(cell,"%s\"%X\":%u",bytes.empty()?"":",",rva,b);bytes+=cell;}
    std::string words;const auto step=world_gate_step;
    for(int k=2;step && k<16;++k){uint32_t w{};Read(step+4ull*k,&w,4);char cell[24]{};sprintf_s(cell,"%s%u",words.empty()?"":",",w);words+=cell;}
    // 1088: the main player's ChrAsmModel entity ([[[WorldChrMan]+80]+20B0]+8).
    uintptr_t manager{},player{},asm_model{},entity{};
    if(Read(image+0x477FDB8,&manager,8) && manager && Read(manager+0x80,&player,8) && player &&
       Read(player+0x20B0,&asm_model,8) && asm_model)Read(asm_model+8,&entity,8);
    uintptr_t part_objects{};if(entity)Read(entity+0xB10,&part_objects,8);
    char row[800]{};sprintf_s(row,"{\"event\":\"world_update_probe\",\"frame\":%llu,\"gate_calls\":%lld,\"player_asm_entity\":%llu,\"player_part_objects\":%llu,\"first_bytes\":{%.300s},\"step_words_08\":[%.200s]}\n",
        frame,world_gate_calls,static_cast<unsigned long long>(entity),static_cast<unsigned long long>(part_objects),bytes.c_str(),words.c_str());Log(row);
}
// 1120 witness: CPU ticks of the native frame phases (0 scene collection, 1 frame preparation, 2 recording/submission
// and presentation), summed per pacing window.
std::array<std::atomic<int64_t>,3> native_phase_ticks{};std::array<std::atomic<int64_t>,3> native_phase_counts{};
std::array<double,5> native_ui_admission_ms{};uint32_t native_ui_admissions{}; // 1130 witness (render-thread only)
// I-0078: with the render worker, whether a UI build is current (the worker builds and owns it): set when a frame carries a
// packet, read again from the renderer only while the worker is quiesced (a presentation resize may retire it).
bool native_ui_build_current{};
// I-0137 tranche 7: why this frame's UI has no packet, set by the render step (17_render_step.inc) and taken by Clear() (reset to
// « step not run » once read): the game's UI displayed no movie, movies displayed and none drew, the posted display made for another
// frame or extent (TakeNativeUiDisplay drops its packet), no UI session or HAL, the render step not run for this frame.
enum class NativeUiStepCause : uint8_t {NoMovie,MoviesWithoutPacket,DisplayChanged,NoUiSession,StepNotRun};
NativeUiStepCause native_ui_step_cause{NativeUiStepCause::StepNotRun};
bool native_ui_display_dropped{};   // TakeNativeUiDisplay's last join: its packet not kept (another frame or extent)
bool NativeUiBuildCurrent() {return render_worker?native_ui_build_current:renderer.PrimitiveBuildCurrent();}
// I-0137 tranche 7: a world frame presented without its UI while a UI build is current, counted by cause (NativeUiStepCause; its
// first four times and every 1,000th per cause, `native_ui_overlay_absent`): `no_movie` (the game's UI displayed nothing),
// `movies_without_packet` (every draw of the displayed movies skipped as the game's data, I-0101) and `display_changed` (I-0132 R12f's
// choice: a frame made for another extent presented without its UI). The module's own states with no such reason, no UI session or
// HAL and the render step not run, still refuse as cR2's rule did (the integrity review). No gate reads the row. Any thread.
void NoteNativeUiOverlayAbsent(NativeUiStepCause cause) {
    if(cause==NativeUiStepCause::NoUiSession || cause==NativeUiStepCause::StepNotRun)
        throw std::runtime_error(cause==NativeUiStepCause::NoUiSession?"current native model UI overlay: no UI session":"current native model UI overlay: render step not run");
    static constexpr const char* names[]{"no_movie","movies_without_packet","display_changed","no_ui_session","step_not_run"};
    static std::atomic<uint64_t> counts[std::size(names)]{};const auto index=static_cast<size_t>(cause);if(index>=std::size(names))return;
    const auto n=counts[index].fetch_add(1,std::memory_order_relaxed)+1;
    if(n>4 && n%1000)return;
    char row[160]{};sprintf_s(row,"{\"event\":\"native_ui_overlay_absent\",\"cause\":\"%s\",\"times\":%llu}\n",names[index],static_cast<unsigned long long>(n));Log(row);
}
std::array<std::atomic<int64_t>,3> native_prep_ticks{}; // cF.2: atomic (prepare may run on the render worker). 1133 witness: palette index validation, resource/constant binding, draw preparation
// C4: the renderer's scene-colour copy wrapped as a scene texture (slot 45), refreshed by the render thread when the
// renderer recreates it; the selection binds the current record.
std::mutex native_scene_copy_lock;
std::shared_ptr<const bootstrap::NativeTextureRecord> native_scene_copy;
void PublishNativeSceneCopy() {
    const auto copy=renderer.SceneColourCopy();if(!copy)return;
    {const std::lock_guard hold(native_scene_copy_lock);if(native_scene_copy && native_scene_copy->resource.Get()==copy.Get())return;}
    auto record=std::make_shared<bootstrap::NativeTextureRecord>();record->resource=copy;record->resident=true;record->render_target=true;
    // 1205: the selection checks the bound record's shape against the shader resource (2D: one slice, a known format);
    // without a description the RecursiveCrystal stage was refused as Unsupported (Native1202-1203).
    {const auto desc=copy->GetDesc();record->description={static_cast<uint32_t>(desc.Width),desc.Height,1,SceneRenderer::ForwardTargetFormat(),1,false};}
    try{record->view=gpu.AllocateCpuDescriptor();}catch(...){Refuse("native_scene_copy_view");}
    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=SceneRenderer::ForwardTargetFormat();srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;
    gpu.device->CreateShaderResourceView(copy.Get(),&srv,record->view.handle);
    const std::lock_guard hold(native_scene_copy_lock);native_scene_copy=std::move(record);
    Log("{\"event\":\"native_scene_copy_published\",\"slot\":45}\n");
}
// I-0107: the light grid's four buffers the forward programmes read, t59 g_LightIndices (stride 8), t60 g_LightLookupBuffer
// (12), t61 g_LightBuffer (64) and t62 g_SpotLightBuffer (128), wrapped as structured-buffer scene textures when the renderer
// (re)makes the grid; the selection binds the current set (the record epochs carry it, so a new set re-admits the models).
std::shared_ptr<const NativeLightingPrograms> AcquireNativeLightingPrograms();   // 06_pass_programmes.inc
bool NativeForwardLightsOn() noexcept {static const bool on=!ModuleFlag(L"no-forward-lights.flag");return on;}
struct NativeLightGridRecords {std::array<std::shared_ptr<const bootstrap::NativeTextureRecord>,4> buffers;uint32_t tiles16{};};
std::mutex native_light_grid_lock;
std::shared_ptr<const NativeLightGridRecords> native_light_grid;
void PublishNativeLightGrid(const NativeFrameJob::LightGridSet& grid) {
    if(!grid.indices || !grid.lookup || !grid.points || !grid.spots)return;
    {const std::lock_guard hold(native_light_grid_lock);if(native_light_grid && native_light_grid->buffers[0]->resource.Get()==grid.indices.Get())return;}
    auto records=std::make_shared<NativeLightGridRecords>();records->tiles16=grid.tiles16;
    struct View {ID3D12Resource* resource;uint32_t stride,elements;};
    const View views[4]{{grid.indices.Get(),8u,grid.tiles16*1024},{grid.lookup.Get(),12u,grid.tiles16},{grid.points.Get(),64u,1024u},{grid.spots.Get(),128u,1024u}};
    for(size_t i=0;i<4;++i){const auto& [resource,stride,elements]=views[i];
        auto record=std::make_shared<bootstrap::NativeTextureRecord>();record->resource=resource;record->resident=true;record->render_target=true;
        record->description={elements,1,1,DXGI_FORMAT_UNKNOWN,1,false};   // a structured buffer, as AdmitStructuredZero describes it
        try{record->view=gpu.AllocateCpuDescriptor();}catch(...){Refuse("native_light_grid_view");}
        D3D12_SHADER_RESOURCE_VIEW_DESC view{};view.Format=DXGI_FORMAT_UNKNOWN;view.ViewDimension=D3D12_SRV_DIMENSION_BUFFER;
        view.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;view.Buffer.NumElements=elements;view.Buffer.StructureByteStride=stride;
        gpu.device->CreateShaderResourceView(resource,&view,record->view.handle);
        record->content_bytes=uint64_t(stride)*elements;records->buffers[i]=std::move(record);}
    {const std::lock_guard hold(native_light_grid_lock);native_light_grid=std::move(records);}
    char row[160]{};sprintf_s(row,"{\"event\":\"native_light_grid_published\",\"tiles16\":%u}\n",grid.tiles16);Log(row);
}
// D3D12 review phase A (D3D12-PHASES-A-C-PREP 3.2): set once at start from measure.flag.
bool measure_run{};
// The witness pictures next to the trace (raw rows for Convert-RgbaToPng.ps1), only with world-pictures.flag (I-0102: every test
// run wrote the velocity, reflection, menu-view and movie pictures, 18 MB a start check); those four never in a measure run, the
// world pictures (`world`) in any run, as before (final review M1).
bool WitnessPictures(bool world=false) {static const bool on=ModuleFlag(L"world-pictures.flag");return on && (world || !measure_run);}
// I-0138 tranche 6: the world pictures taken so far (the count in the high 24 bits) and the last one's presentation (the low 40),
// for tone-look-cycle.flag (04: a look a picture, whenever the pictures start and whatever makes them wait).
std::atomic<uint64_t> native_world_picture_mark{};
// I-0140 tranche 5 (a test tool): with pictures-from-cutscene.flag the world pictures' and the control view's 300th-frame pictures
// are counted only once witness-cutscene.txt's cutscene was handed to the game (ApplyWitnessCutscene), so their cap is spent on
// the cutscene, not on the title, the load and the arrival before it.
// Without witness-cutscene.txt, or with a hand-over refused, it takes no such picture in the whole run (the final review's L6).
std::atomic<bool> witness_cutscene_handed{};
bool PicturesWait() {static const bool from_cutscene=ModuleFlag(L"pictures-from-cutscene.flag");return from_cutscene && !witness_cutscene_handed.load(std::memory_order_relaxed);}
// A file next to the trace (an empty name gives its folder): the pictures, the sampler's output, the observers' dumps (I-0102: one helper).
std::wstring JournalSibling(const std::wstring& name) {
    std::wstring directory(control->journal);if(const auto slash=directory.find_last_of(L"\\/");slash!=std::wstring::npos)directory.resize(slash+1);else directory.clear();
    return directory+name;
}
std::atomic<uint64_t> retirements_pending_peak{};   // D3D12 phase D cD0: highest PendingRetirements() of the pacing window
bool collect_no_profile{};   // no-kprofile.flag: a measurement run without the K split (C-0c)
Microsoft::WRL::ComPtr<IDXGIAdapter3> native_adapter;
// The adapter, its resource binding tier (decides whether R6 may leave table slots unwritten) and root signature tier.
void LogNativeAdapter() {
    if(!gpu.device || !gpu.factory)return;
    D3D12_FEATURE_DATA_D3D12_OPTIONS options{};
    const bool read=SUCCEEDED(gpu.device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS,&options,sizeof(options)));
    D3D12_FEATURE_DATA_ROOT_SIGNATURE root{D3D_ROOT_SIGNATURE_VERSION_1_1};
    if(FAILED(gpu.device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE,&root,sizeof(root))))root.HighestVersion=D3D_ROOT_SIGNATURE_VERSION_1_0;
    DXGI_ADAPTER_DESC1 desc{};
    if(SUCCEEDED(gpu.factory->EnumAdapterByLuid(gpu.device->GetAdapterLuid(),IID_PPV_ARGS(&native_adapter))))native_adapter->GetDesc1(&desc);
    char name[128]{};WideCharToMultiByte(CP_UTF8,0,desc.Description,-1,name,sizeof(name)-1,nullptr,nullptr);
    for(auto& c:name)if(c=='"' || c=='\\')c='_';
    // Phase E E0: the resource heap tier (Tier 1 keeps render-target, other texture and buffer placements in separate heaps)
    // and the API's default placement alignment (the device's own per target is the audit row's target_alignment), for E6.
    char row[448]{};sprintf_s(row,"{\"event\":\"native_adapter\",\"name\":\"%s\",\"dedicated_mb\":%llu,\"resource_binding_tier\":%d,\"tiled_tier\":%d,\"resource_heap_tier\":%d,\"placement_alignment\":%u,\"root_signature\":\"%s\"}\n",
        name,static_cast<unsigned long long>(desc.DedicatedVideoMemory>>20),read?static_cast<int>(options.ResourceBindingTier):-1,read?static_cast<int>(options.TiledResourcesTier):-1,
        read?static_cast<int>(options.ResourceHeapTier):-1,static_cast<unsigned>(D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT),
        root.HighestVersion==D3D_ROOT_SIGNATURE_VERSION_1_1?"1.1":"1.0");Log(row);
}
// Process private bytes and the adapter's video memory, in MB (once per pacing window): I-0084 (lead D1) adds the budgets the
// system gives this process (local: the card's memory, I-0132 R11f the upload arena and the streams' versions with it where GPU
// upload heaps are offered; non-local: the system memory the GPU reads, the UPLOAD heaps: the staging chunks).
struct NativeMemory {uint64_t private_mb{},local_mb{},local_budget_mb{},nonlocal_mb{},nonlocal_budget_mb{};};
NativeMemory NativeMemoryMb() {
    PROCESS_MEMORY_COUNTERS_EX counters{};NativeMemory m;
    K32GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),sizeof(counters));m.private_mb=static_cast<uint64_t>(counters.PrivateUsage)>>20;
    DXGI_QUERY_VIDEO_MEMORY_INFO info{};
    if(native_adapter && SUCCEEDED(native_adapter->QueryVideoMemoryInfo(0,DXGI_MEMORY_SEGMENT_GROUP_LOCAL,&info))){m.local_mb=info.CurrentUsage>>20;m.local_budget_mb=info.Budget>>20;}
    if(native_adapter && SUCCEEDED(native_adapter->QueryVideoMemoryInfo(0,DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL,&info))){m.nonlocal_mb=info.CurrentUsage>>20;m.nonlocal_budget_mb=info.Budget>>20;}
    return m;
}
// I-0084 (lead D1): ER reads the budget once at start and never reacts (I-0080 ER-STUDY 4). Here, at each pacing window, the
// backend releases what it holds on its own account, down to the budget each segment is over: free placed heaps for the local
// one (the card; the upload arena's pooled chunks, local with GPU upload heaps, are not released here: I-0132 R11f's final review
// L5), idle staging chunks for the non-local one (the UPLOAD heaps in system memory), one row per window while it
// lasts; what the game keeps is never withdrawn (D-0197, no stand-ins), and the frame's targets carry a high residency priority
// (SceneRenderer::NoteTargetBytes) so a demotion takes textures first.
void ReactToVideoMemory(const NativeMemory& m,long long presented) {
    const uint64_t local_over=m.local_budget_mb && m.local_mb>m.local_budget_mb?(m.local_mb-m.local_budget_mb)<<20:0,
        nonlocal_over=m.nonlocal_budget_mb && m.nonlocal_mb>m.nonlocal_budget_mb?(m.nonlocal_mb-m.nonlocal_budget_mb)<<20:0;
    if(!local_over && !nonlocal_over)return;
    // I-0097 tranche 4 (D-0211, step review M1): the kept texture contents no record uses go first (the least recently retired),
    // so the heaps they held can be released below.
    const auto kept=local_over?textures.TrimKept(local_over):0;
    const auto released=gpu.TrimStreamedMemory(local_over,nonlocal_over);
    char row[320]{};sprintf_s(row,"{\"event\":\"native_video_memory_over_budget\",\"frame\":%lld,\"local_mb\":%llu,\"local_budget_mb\":%llu,\"nonlocal_mb\":%llu,\"nonlocal_budget_mb\":%llu,\"released\":%zu,\"kept_released_mb\":%.1f,\"targets_high_priority\":%llu}\n",
        presented,static_cast<unsigned long long>(m.local_mb),static_cast<unsigned long long>(m.local_budget_mb),static_cast<unsigned long long>(m.nonlocal_mb),
        static_cast<unsigned long long>(m.nonlocal_budget_mb),released,double(kept)/1048576.0,static_cast<unsigned long long>(gpu_audit.target_priority_raised.load()));Log(row);
}
// I-0084 (lead D1's check, announced runs only): video-memory-budget-mb.txt next to the module (Run-StartupTrace
// -VideoMemoryBudgetMb) holds a size in MB; a ballast of committed video memory at maximum residency priority then leaves
// that much headroom above the local usage measured at the device's opening (about 0.5 GB then), standing for a smaller card
// (D3D12 has no budget override). Released at close, before the device.
// Final review 2: held through a leaked object, so nothing of it runs at the module's unload if Close is skipped.
std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>& native_memory_ballast=*new std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>;
void StartNativeMemoryBallast() {
    uint64_t target{};const auto budget=ModuleSibling(L"video-memory-budget-mb.txt");if(budget.empty())return;
    {std::wifstream file(budget);if(!file || !(file>>target) || !target)return;}
    const auto before=NativeMemoryMb();
    const uint64_t ballast_mb=before.local_budget_mb>target+before.local_mb?before.local_budget_mb-target-before.local_mb:0;
    Microsoft::WRL::ComPtr<ID3D12Device1> residency;(void)gpu.device.As(&residency);uint64_t allocated_mb{};
    for(uint64_t left=ballast_mb;left;){const uint64_t mb=std::min<uint64_t>(left,256);
        D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;heap.CreationNodeMask=heap.VisibleNodeMask=1;
        D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=mb<<20;desc.Height=1;desc.DepthOrArraySize=1;desc.MipLevels=1;
        desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
        if(FAILED(gpu.device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&buffer))))break;
        if(residency){ID3D12Pageable* pageable=buffer.Get();const D3D12_RESIDENCY_PRIORITY maximum=D3D12_RESIDENCY_PRIORITY_MAXIMUM;(void)residency->SetResidencyPriority(1,&pageable,&maximum);}
        native_memory_ballast.push_back(std::move(buffer));left-=mb;allocated_mb+=mb;}
    const auto after=NativeMemoryMb();
    char row[320]{};sprintf_s(row,"{\"event\":\"native_video_memory_ballast\",\"target_mb\":%llu,\"requested_mb\":%llu,\"allocated_mb\":%llu,\"buffers\":%zu,\"local_mb\":%llu,\"local_budget_mb\":%llu}\n",
        static_cast<unsigned long long>(target),static_cast<unsigned long long>(ballast_mb),static_cast<unsigned long long>(allocated_mb),native_memory_ballast.size(),
        static_cast<unsigned long long>(after.local_mb),static_cast<unsigned long long>(after.local_budget_mb));Log(row);
}
// R13 cR3 (render-thread sheet X10): what the owner thread checks and logs once a frame job has run: inline right after the
// job, or with the render worker after the next post (the receipt of the previous job). Only values of the world frame,
// so the frame input itself is released with the job.
struct NativeFrameOutcome {
    uint64_t serial{};FrameReceipt receipt;bool lit{},velocity_reset{};size_t velocity_draws{};
    bool ui_built{};std::array<float,5> ui_admission_ms{};   // I-0078: the UI packet the worker built for this job, and its timings
};
void FinishNativeFrame(const NativeFrameOutcome& outcome);
// R13 cR3: the render worker's job time (its slot wait included), the audit row's worker_job_ms; render_ms stays the owner
// thread's time after prepare in Clear() (inline: the build and the job, as before cR3; worker: the build only).
std::atomic<int64_t> native_worker_job_ticks{},native_worker_jobs{};
// The frame job, on the render worker or inline: its slot (worker), then the job; a movie or view failure keeps its own
// refusal (Refuse ends the process from either thread). On the worker any other failure is logged here with its message,
// then kept by the worker (the owner rethrows it at its next frame).
// R13 cR5 (worker prepare, the default since cR6): `prepare` prepares the job's world frame on the worker, before the slot wait (so it overlaps
// the GPU's previous frame), and the frame is handed to ExecuteFrame; null inline and under inline-prepare.flag.
using NativeWorldPrepare=std::function<std::shared_ptr<const NativeModelFrameInput>()>;
void RunNativeFrameJob(const NativeFrameJob& job,NativeFrameOutcome& outcome,const NativeWorldPrepare& prepare={}) {
    struct InFlight {bool on;explicit InFlight(bool worker):on(worker){if(on)render_job_in_flight.store(true,std::memory_order_release);}
        ~InFlight(){if(on)render_job_in_flight.store(false,std::memory_order_release);}} in_flight(render_worker!=nullptr);
    LARGE_INTEGER start{},end{};QueryPerformanceCounter(&start);
    try{
        const auto prepared=prepare?prepare():nullptr;
        if(render_worker){renderer.SetJobDisplayInterval(job.display_interval);renderer.WaitFrameSlot();}   // cD4: inline, Clear() waits for the slot before its lock
        // I-0111 tranche 2 (PCL): the render submission of world frame job.serial starts (its end and present: PresentPaced).
        ds3native::streamline::Marker(ds3native::streamline::PclMarker::RenderSubmitStart,job.serial);renderer.SetJobSerial(job.serial);
        struct SerialEnd {~SerialEnd(){renderer.SetJobSerial(0);}} serial_end;
        outcome.receipt=renderer.ExecuteFrame(job,prepared);
        if(job.ui_packet){outcome.ui_built=true;outcome.ui_admission_ms=renderer.primitive_admission_ms;}}   // I-0078: read on the thread that built it
    catch(const NativeFrameStageError& e){char row[1100]{};const bool movie=e.stage==NativeFrameStageError::Stage::Movie;
        sprintf_s(row,"{\"event\":\"%s\",\"message\":\"%.1000s\"}\n",movie?"native_movie_frame_failure":"native_offscreen_view_failure",e.what());Log(row);
        Refuse(movie?"native_movie_frame_failed":"native_offscreen_view_failed");}
    catch(const std::exception& e){if(render_worker){char row[600]{};sprintf_s(row,"{\"event\":\"native_render_job_failure\",\"serial\":%llu,\"message\":\"%.500s\"}\n",
            static_cast<unsigned long long>(job.serial),e.what());Log(row);}throw;}
    if(render_worker){QueryPerformanceCounter(&end);native_worker_job_ticks+=end.QuadPart-start.QuadPart;++native_worker_jobs;}
}
// R13 cR3: the outcome of the job posted last (owner thread; with the render worker only).
std::shared_ptr<NativeFrameOutcome> pending_frame_outcome;
// D-0167 measurement (measure runs): Clear() split, ticks summed over the pacing window: 0 scene acquisition and publication,
// 1 movie and menu views, 2 UI admission, 3 the rest under the submission lock, 4 the post (waits for the previous job),
// 5 the previous job's outcome.
std::array<std::atomic<uint64_t>,6> native_clear_ticks{};
// I-0111 tranche 2: set by EndFrame around its Clear (the engine thread), so only a frame EndFrame waits for decides the display clock.
bool native_end_frame_clear{};
void Clear() {
    kprofile::Mark(kprofile::ClearEntry);const kprofile::MarkOnExit clear_exit{kprofile::ClearExit};
    const bool clear_timed=kprofile::enabled.load(std::memory_order_relaxed);int64_t clear_last=clear_timed?kprofile::Now():0;
    const auto clear_mark=[&](size_t k){if(clear_timed){const auto now=kprofile::Now();native_clear_ticks[k].fetch_add(uint64_t(now-clear_last),std::memory_order_relaxed);clear_last=now;}};
    // Acquire current CPU selection before the submission lock; resource
    // retirement uses submission -> CPU-owner order elsewhere.
    // R13 cR3: a fault of the render worker's last job ends this frame with the job's own message (before the UI admission,
    // which would otherwise refuse on the faulted renderer under its own name).
    if(render_worker && render_worker->Faulted())render_worker->RethrowFault();
    const auto selected=AcquireNativeSelectedScene();
    // D3D12 phase D cD4 (sheet R5): the wait for the frame slot this frame reuses, before the submission lock (none with one
    // slot; on the ring's own event, so no other waiter on the queue can take its wake-up). R13 cR3: with the render worker
    // the job waits for its slot on the worker.
    if(!render_worker)renderer.WaitFrameSlot();
    auto job=std::make_shared<NativeFrameJob>();const auto outcome=std::make_shared<NativeFrameOutcome>();NativeWorldPrepare prepare;   // cR5
    // I-0111 tranche 2 (display-clock.flag): the engine's frame-start wait for this frame decided before its job is posted, so the
    // worker and the wait at the end of EndFrame agree (step review M1); DS3's limiter period [clock+0x18] read only. With the render
    // worker only (inline, Clear waits the object above).
    if(render_worker && renderer.DisplayClockRequested() && !native_reflex_on && std::exchange(native_end_frame_clear,false)){uintptr_t timing{};float limiter_period{};   // step review L-C: EndFrame's only
        if(Read(image+0x489DD10,&timing,8) && timing && Read(timing+0x18,&limiter_period,4))job->display_interval=renderer.DecideDisplayClock(limiter_period);
        else job->display_interval=renderer.DecideDisplayClock(0);}
    {std::lock_guard<std::mutex> submission(gpu_submissions);
    ++serial;
    if(selected)PublishNativeSceneCopy();
    clear_mark(0);
    LARGE_INTEGER phase_start{},phase_mid{};QueryPerformanceCounter(&phase_start);
    // R13 cR5 (with the render worker, unless inline-prepare.flag): the world frame is prepared on the worker (prepare reads no
    // engine memory since cP6/cP7; the scene is immutable once published); inline otherwise, as before.
    const bool prepare_on_worker=render_worker && native_prepare_on_worker;
    std::shared_ptr<NativeModelFrameInput> model_frame;
    if(!prepare_on_worker){model_frame=PrepareNativeModelFrame(selected,serial);QueryPerformanceCounter(&phase_mid);
        native_phase_ticks[1]+=phase_mid.QuadPart-phase_start.QuadPart;++native_phase_counts[1];}
    else phase_mid=phase_start;
    // D3D12 phase D cD3: the movie, the menu views and the frame below record into one list, submitted once by the frame.
    // R13 cR2: they form one immutable frame job, built here on the engine thread. R13 cR3: run by the render worker under
    // by default (cR6), inline under inline-render.flag.
    job->serial=serial;   // the world frame enters the job complete, after the UI fields below
    if(selected)job->scene_copy=renderer.SceneColourCopy();   // X2: the copy this frame's forward pass writes
    // I-0107: the light grid (no-forward-lights.flag opts out), made only once GXLightOrganize is resolved (else nothing writes it).
    if(selected && NativeForwardLightsOn()){
        static bool reported{};
        if(!renderer.LightGridFailed()){reported=false;   // a reopened renderer's latch is reported again
            static std::shared_ptr<const NativeLightingPrograms> programs;if(!programs)programs=AcquireNativeLightingPrograms();   // published once (I-0137 C4): asked until then
            if(programs && !programs->organize.empty()){job->light_grid=renderer.LightGridSources();if(!renderer.LightGridFailed())PublishNativeLightGrid(job->light_grid);}}
        // Final review I2/I3, step review L1/L2: while the latch holds, from the frame that set it, the grid is withdrawn (the rows go
        // back to the empty buffers and zero words); reported once per latch.
        if(renderer.LightGridFailed()){
            if(!reported){reported=true;std::string failure=renderer.LightGridFailure();for(auto& ch:failure)if(ch=='"' || ch=='\\' || ch<32)ch='?';
                Log(("{\"event\":\"native_light_grid_refused\",\"why\":\""+failure.substr(0,200)+"\"}\n").c_str());}
            const std::lock_guard hold(native_light_grid_lock);native_light_grid.reset();}}
    CollectNativeMovieFrame(*job);   // 1250: the latest Bink frame into the movie proxy's native target, before the UI samples it
    CollectNativeOffscreenViews(*job);   // 1169: listed menu views into their native targets, before the UI samples them
    clear_mark(1);
    // 1186 witness (world-pictures.flag): the motion vectors of every 90th frame with velocity (at most 12 pictures).
    {static LONG64 pictures{};const bool velocity=prepare_on_worker?NativeSceneHasVelocity(selected):model_frame && model_frame->velocity;   // cR5: the scene's velocity
        if(WitnessPictures() && velocity && serial%90==0 && InterlockedIncrement64(&pictures)<=12)
            job->velocity_file=JournalSibling(L"velocity-"+std::to_wstring(serial)+L".rgba");}   // R13 cR3: by value in the job
    // W10 TM-2 witness (world-pictures.flag): the presented picture of every 300th world frame (at most 8, 16 with
    // gbuffer-pictures.flag: I-0113, a walk's pictures after its settle) next to the trace, R8G8B8A8 at the presentation extent
    // (Convert-RgbaToPng.ps1).
    // I-0128 R8d (world-burst.flag with it, the RR stability's figure): also the 32 consecutive world frames from serial 2400, each
    // picture alone (pixel_only: no HDR, G-buffer or GI mean beside it), 2400 itself the 300th frame's (with its witnesses) unless the
    // pictures' cap refused it (then alone: the step review's L5).
    // I-0132 (ROADMAP workstream 2, the GI quality in motion, D-0273): with world-burst-after-<n>.flag (n 300, 600, 900, 1200 or 1800)
    // the burst starts n frames after the first world frame instead of at serial 2400 (a serial that depends on the title's and the
    // load's lengths), one row naming its first frame, so a camera turn the runner starts from that row covers it.
    {static LONG64 pictures{};static const LONG64 picture_cap=renderer.gbuffer_pictures?16:8;static const bool burst=ModuleFlag(L"world-burst.flag");
        static const uint64_t burst_after=[]{for(const uint64_t n:{300u,600u,900u,1200u,1800u}){
            const std::wstring name=L"world-burst-after-"+std::to_wstring(n)+L".flag";if(ModuleFlag(name.c_str()))return n;}return uint64_t{};}();
        static uint64_t burst_first=burst_after?0:2400;constexpr uint64_t kBurstFrames=32;
        if(burst && burst_after && !burst_first && selected){burst_first=serial+burst_after;world_burst_first.store(burst_first,std::memory_order_relaxed);char row[160]{};
            sprintf_s(row,"{\"event\":\"world_burst_window\",\"first_world\":%llu,\"after\":%llu,\"from\":%llu,\"frames\":%llu}\n",
                (unsigned long long)serial,(unsigned long long)burst_after,(unsigned long long)burst_first,(unsigned long long)kBurstFrames);Log(row);}
        if(burst_first && burst_after)world_burst_serial.store(serial,std::memory_order_relaxed);
        const bool in_burst=burst && burst_first && serial>=burst_first && serial<burst_first+kBurstFrames;
        // The step review's L3: a 300th frame inside a moved burst is taken as the burst's (pixel_only), so no frame of it carries the
        // witnesses' extra readbacks.
        // I-0141 tranche 17 (pixel-keys.flag): every 1200th world frame instead (16 pictures over about 5 minutes at 60 frames a second,
        // a scene's state after a cutscene included), each with this frame's scene's own key list by set index (native_pixel_key_list,
        // the scene's light_keys: the snapshot its sets were built from, so GB3.w x 255 of this picture reads as that list's key) and
        // the render extent at the picture's request (the GB files carry no header).
        static const bool pixel_keys=ModuleFlag(L"pixel-keys.flag");const uint64_t cadence=pixel_keys?1200:300;
        if(WitnessPictures(true) && selected && serial%cadence==0 && !(burst_after && in_burst) && !PicturesWait() && InterlockedIncrement64(&pictures)<=picture_cap){
            job->pixel_file=JournalSibling(L"world-"+std::to_wstring(serial)+L".rgba");
            if(pixel_keys){const auto keys=NativeSceneLightKeys(selected);std::string row="{\"event\":\"native_pixel_key_list\",\"serial\":"+std::to_string(serial)+
                    ",\"width\":"+std::to_string(renderer.RenderExtent()[0])+",\"height\":"+std::to_string(renderer.RenderExtent()[1])+",\"keys\":[";
                for(size_t i=0;i<keys.size();++i){char cell[200]{};float t[2]{};std::memcpy(t,&keys[i][8],8);
                    sprintf_s(cell,"%s[%u,%u,%u,%u,%u,%u,%u,%u,%s,%s]",i?",":"",keys[i][0],keys[i][1],keys[i][2],keys[i][3],keys[i][4],keys[i][5],keys[i][6],keys[i][7],
                        PixelKeyNumber(t[0]).c_str(),PixelKeyNumber(t[1]).c_str());row+=cell;}
                Log((row+"]}\n").c_str());}
            native_world_picture_mark.store((((native_world_picture_mark.load(std::memory_order_relaxed)>>40)+1)<<40)|(serial&0xFFFFFFFFFFull),std::memory_order_release);}
        else if(WitnessPictures(true) && selected && in_burst){
            job->pixel_file=JournalSibling(L"world-"+std::to_wstring(serial)+L".rgba");job->pixel_only=true;}}
    // I-0117 T4 (rt-view.flag): the control view's two pictures (traced, classes) of every 300th world frame (at most 16) next to the
    // trace, R8G8B8A8 at the render extent (Convert-RgbaToPng.ps1).
    // I-0125 R6d (rt-view-last.flag, a diagnostic): the last 16 instead, in 16 rolling slots (rt-view-slot-0 to 15: a run's end, where a
    // travelled zone is).
    {static LONG64 pictures{};static const bool last=ModuleFlag(L"rt-view-last.flag");
        if(renderer.rt_view && selected && serial%300==0 && !PicturesWait()){const auto n=InterlockedIncrement64(&pictures);
            if(last)job->rt_view_file=JournalSibling(L"rt-view-slot-"+std::to_wstring((n-1)%16));
            else if(n<=16)job->rt_view_file=JournalSibling(L"rt-view-"+std::to_wstring(serial));}}
    job->display_curve=NativeDisplayCurve();   // W3: the gamma curve of this frame (the same object until the brightness changes)
    {static bool reported{};if(!reported && renderer.DisplayFailed()){reported=true;char row[400]{};   // W3: the stage could not open (frames go straight to the back buffer)
        sprintf_s(row,"{\"event\":\"native_display_stage\",\"open\":false,\"reason\":\"%.300s\"}\n",renderer.DisplayFailure().c_str());Log(row);}
     // Review: the stage at work: one row when it is first seen open, then every 1800th frame (passes and curve uploads).
     static bool display_logged{};if(renderer.DisplayOpen() && (!display_logged || serial%1800==0)){display_logged=true;char row[160]{};
        sprintf_s(row,"{\"event\":\"native_display_stage\",\"open\":true,\"passes\":%llu,\"uploads\":%llu}\n",
            static_cast<unsigned long long>(renderer.DisplayPasses()),static_cast<unsigned long long>(renderer.DisplayUploads()));Log(row);}}
    // W6 GL-7 (T7): the glow stage at work, when it first records and every 1800th frame: frames and passes (ten per frame) and,
    // when this frame's prepared input is at hand, its extent, the five target sizes and the scales; or once its failure.
    {static bool failure{},seen{};const auto frames=renderer.GlowFrames();
        if(!failure && renderer.GlowFailed()){failure=true;char row[400]{};
            sprintf_s(row,"{\"event\":\"native_glow_frame\",\"open\":false,\"reason\":\"%.300s\"}\n",renderer.GlowFailure().c_str());Log(row);}
        if(frames && (!seen || serial%1800==0)){seen=true;std::string sizes;float rgb{},final_scale{},blur{};
            NativeGlowInput glow{};uint32_t w{},h{};bool have=false;
            if(model_frame && model_frame->glow){glow=*model_frame->glow;const auto post=NativePostExtent(model_frame->camera);w=post[0];h=post[1];have=true;}
            else have=NativeSceneGlow(selected,glow,w,h);   // cR5: prepared on the worker, the scene's input
            if(have){sizes="["+std::to_string(w)+","+std::to_string(h)+"]";
                for(unsigned level=0;level<4;++level){const auto s=ds3native::glow::LevelSize(w,h,level);sizes+=",["+std::to_string(s.width)+","+std::to_string(s.height)+"]";}
                rgb=glow.rgb_scale;final_scale=glow.final_scale;blur=glow.blur_scale;}
            char row[400]{};sprintf_s(row,"{\"event\":\"native_glow_frame\",\"open\":true,\"frames\":%llu,\"passes\":%llu,\"sizes\":[%s],\"blur\":%g,\"rgb\":%g,\"final\":%g}\n",
            static_cast<unsigned long long>(frames),static_cast<unsigned long long>(renderer.GlowPasses()),sizes.c_str(),blur,rgb,final_scale);Log(row);}}
    // FFX-DRAW N6: the effect stage's counters every 1800th frame (with the draw side, on by default).
    {if(native_ffx_draw && serial%1800==0)Log(("{\"event\":\"native_effect_stage\",\"stage\":"+renderer.EffectSummary()+"}\n").c_str());}
    // I-0127 (effect-measure.flag): the effect stage's measure every 600th frame: its render-thread time (without the 0x29 rows and
    // the scene copy its hooks draw), the effect block's GPU time, the draws, and the list calls by kind with those that repeat
    // the state already bound (heaps, pipeline, topology, targets, texture table, sampler table, constants, vertices, indices,
    // views, draws), per frame; times in microseconds, percentiles from 10-microsecond buckets.
    {static const bool measure=ModuleFlag(L"effect-measure.flag");
        if(measure && native_ffx_draw && serial%600==0){const auto w=renderer.TakeEffectMeasure();
            if(w.frames){const auto us=[](uint64_t ticks,uint64_t f){return f?double(ticks)*1e6/double(f):0.0;};
                const auto pct=[](const std::array<uint32_t,128>& h,uint64_t n,double q){if(!n)return 0.0;const uint64_t at=uint64_t(std::ceil(q*double(n)));uint64_t seen{};
                    for(size_t i=0;i<h.size();++i){seen+=h[i];if(seen>=at)return double(i+1)*10.0;}return 1280.0;};
                const double n=double(w.frames),g=double(std::max<uint64_t>(w.gpu_frames,1));
                std::string calls,repeats;for(uint32_t k=0;k<EffectCallKinds;++k){char c[32]{};sprintf_s(c,"%s%.2f",k?",":"",double(w.calls[k])/n);calls+=c;
                    sprintf_s(c,"%s%.2f",k?",":"",double(w.repeats[k])/n);repeats+=c;}
                char row[1400]{};sprintf_s(row,"{\"event\":\"native_effect_measure\",\"serial\":%llu,\"frames\":%llu,\"cpu_us\":{\"mean\":%.2f,\"p50\":%.0f,\"p99\":%.0f,\"max\":%.2f},"
                    "\"hooks_us_mean\":%.2f,\"gpu_frames\":%llu,\"gpu_us\":{\"mean\":%.2f,\"p50\":%.0f,\"p99\":%.0f,\"max\":%.2f},\"draws\":{\"mean\":%.2f,\"max\":%llu},"
                    "\"statements_mean\":%.2f,\"pipeline_changes_mean\":%.2f,\"arena_kb\":{\"mean\":%.1f,\"max\":%.1f},\"calls\":[%s],\"repeats\":[%s]}\n",
                    static_cast<unsigned long long>(serial),static_cast<unsigned long long>(w.frames),us(w.cpu_ticks,w.cpu_frequency)/n,pct(w.cpu_histogram,w.frames,0.5),
                    pct(w.cpu_histogram,w.frames,0.99),us(w.cpu_ticks_max,w.cpu_frequency),us(w.callback_ticks,w.cpu_frequency)/n,static_cast<unsigned long long>(w.gpu_frames),
                    us(w.gpu_ticks,w.gpu_frequency)/g,pct(w.gpu_histogram,w.gpu_frames,0.5),pct(w.gpu_histogram,w.gpu_frames,0.99),us(w.gpu_ticks_max,w.gpu_frequency),
                    double(w.draws)/n,static_cast<unsigned long long>(w.draws_max),double(w.statements)/n,double(w.pipeline_changes)/n,double(w.arena_bytes)/n/1024.0,
                    double(w.arena_max)/1024.0,calls.c_str(),repeats.c_str());Log(row);}}}
    // I-0113 tranche 29 (effect-scan.flag, gap 16): every 30th frame the scans read since the last call, one row each picture that
    // holds a NaN, an inf or a negative half (stage 0: the scene copy the passes after 0x29 sample; 1: the lit HDR after them;
    // 2: the copy after them; 3: the lit HDR before them, EffectScanRow), and every 600th frame the pictures scanned, flagged and skipped since the start (the
    // scan's own life sign).
    // I-0113 tranche 30 (effect-scan-draws.flag): with them, one row each point whose negative pixels grew (EffectDrawScanRow: the
    // statement before it wrote them; kind 0 before the block, 1 after a 0x29 drawing statement, 2 after the 0x29 rows before `key`,
    // 3 after the 0x29 pass; tranche 31: 4 after a 0x2D drawing statement, 5 after the 0x2D pass), its programmes written beside the journal once a key and content (ffx-<vs|ps>-<key>-<hash>.dxbc), and every 600th frame
    // the frames and points read, the rows kept and dropped, the frames not scanned.
    {static const bool scan=ModuleFlag(L"effect-scan.flag");static const bool scan_draws=ModuleFlag(L"effect-scan-draws.flag");static uint64_t scanned{},flagged{},skipped{};
        static uint64_t draw_frames{},draw_points{},draw_flagged{},draw_skipped{},draw_logged{},draw_capped{};
        // The step review's L3: a non-finite value written null (a -inf half gives an infinite peak), so every row stays JSON.
        const auto number=[](float v){char c[32]{};if(std::isfinite(v))sprintf_s(c,"%.4g",v);else strcpy_s(c,"null");return std::string(c);};
        // The step review's L4: a programme's file named by its key and a hash of its bytes (FNV-1a), a key reused by another programme
        // after an area load written again under its own name; the row carries the hash.
        const auto hash_of=[](const std::shared_ptr<const std::vector<uint8_t>>& b){uint32_t h=2166136261u;if(b)for(const auto x:*b){h^=x;h*=16777619u;}return h;};
        EffectScanWindow w;if((scan || scan_draws) && native_ffx_draw && serial%30==0)w=renderer.TakeEffectScan();   // one window for both flags
        if(scan_draws && native_ffx_draw && serial%30==0){draw_frames+=w.draw_frames;draw_points+=w.draw_points;
            draw_flagged+=w.draw_flagged;draw_skipped+=w.draw_skipped;draw_capped+=w.draw_capped;static std::set<std::pair<uint64_t,uint32_t>> written;
            for(const auto& r:w.draw_rows){++draw_logged;const auto& p=r.at;const uint32_t vs_hash=hash_of(p.vs_bytes),ps_hash=hash_of(p.ps_bytes);char row[900]{};
                sprintf_s(row,"{\"event\":\"native_effect_draw_scan\",\"serial\":%llu,\"point\":%u,\"points\":%u,\"kind\":%u,\"pass\":%d,\"key\":%u,\"op\":%u,"
                    "\"ps\":\"%llX\",\"ps_hash\":\"%08X\",\"vs\":\"%llX\",\"vs_hash\":\"%08X\",\"blend\":%u,\"depth\":%u,\"raster\":%u,\"targets\":%u,\"textures\":\"%u%u%u%u%u\",\"pixels\":%u,\"before\":%u,"
                    "\"neg\":[%u,%u,%u,%u],\"peak\":[%s,%s,%s],\"peak_before\":[%s,%s,%s],\"box\":[%u,%u,%u,%u],\"nan\":%u}\n",
                    static_cast<unsigned long long>(r.serial),r.point,r.points,p.kind,p.pass,p.key,p.op,static_cast<unsigned long long>(p.ps),ps_hash,static_cast<unsigned long long>(p.vs),vs_hash,
                    p.blend,p.depth,p.raster,p.targets,p.textures[0],p.textures[1],p.textures[2],p.textures[3],p.textures[4],r.pixels,r.before,
                    r.neg[0],r.neg[1],r.neg[2],r.neg[3],number(r.peak[0]).c_str(),number(r.peak[1]).c_str(),number(r.peak[2]).c_str(),
                    number(r.peak_before[0]).c_str(),number(r.peak_before[1]).c_str(),number(r.peak_before[2]).c_str(),r.box[0],r.box[1],r.box[2],r.box[3],r.nan);Log(row);
                for(const auto& [bytes,stage,key,hash]:{std::tuple{&p.vs_bytes,L"vs",p.vs,vs_hash},{&p.ps_bytes,L"ps",p.ps,ps_hash}})
                    if(*bytes && !(*bytes)->empty() && written.emplace(key,hash).second){wchar_t name[80]{};swprintf_s(name,L"ffx-%s-%llX-%08X.dxbc",stage,static_cast<unsigned long long>(key),hash);
                        if(FILE* file{};!_wfopen_s(&file,JournalSibling(name).c_str(),L"wb") && file){fwrite((*bytes)->data(),1,(*bytes)->size(),file);fclose(file);}}}}
        if(scan_draws && native_ffx_draw && serial%600==0){char row[256]{};sprintf_s(row,"{\"event\":\"native_effect_draw_scan_window\",\"serial\":%llu,\"frames\":%llu,\"points\":%llu,\"flagged\":%llu,\"logged\":%llu,\"skipped\":%llu,\"capped\":%llu}\n",
            static_cast<unsigned long long>(serial),static_cast<unsigned long long>(draw_frames),static_cast<unsigned long long>(draw_points),static_cast<unsigned long long>(draw_flagged),
            static_cast<unsigned long long>(draw_logged),static_cast<unsigned long long>(draw_skipped),static_cast<unsigned long long>(draw_capped));Log(row);}
        if(scan && native_ffx_draw && serial%30==0){scanned+=w.frames;flagged+=w.flagged;skipped+=w.skipped;
            const auto four=[](const std::array<uint32_t,4>& v){char c[64]{};sprintf_s(c,"[%u,%u,%u,%u]",v[0],v[1],v[2],v[3]);return std::string(c);};
            for(const auto& r:w.rows){char row[512]{};sprintf_s(row,"{\"event\":\"native_effect_scan\",\"serial\":%llu,\"stage\":\"%s\",\"pass\":%u,\"size\":[%u,%u],\"nan\":%s,\"inf\":%s,\"neg\":%s,\"bad_box\":%s,\"neg_box\":%s}\n",
                static_cast<unsigned long long>(r.serial),r.stage==3?"before":r.stage==2?"copy_after":r.stage==1?"after":"copy",r.pass,r.width,r.height,four(r.nan).c_str(),four(r.inf).c_str(),four(r.neg).c_str(),four(r.bad_box).c_str(),four(r.neg_box).c_str());Log(row);}
            if(w.flagged>w.rows.size()){char row[160]{};sprintf_s(row,"{\"event\":\"native_effect_scan_dropped\",\"serial\":%llu,\"rows\":%llu}\n",static_cast<unsigned long long>(serial),
                static_cast<unsigned long long>(w.flagged-w.rows.size()));Log(row);}}
        if(scan && native_ffx_draw && serial%600==0){char row[160]{};sprintf_s(row,"{\"event\":\"native_effect_scan_window\",\"serial\":%llu,\"scanned\":%llu,\"flagged\":%llu,\"skipped\":%llu}\n",
            static_cast<unsigned long long>(serial),static_cast<unsigned long long>(scanned),static_cast<unsigned long long>(flagged),static_cast<unsigned long long>(skipped));Log(row);}}
    // I-0128 R8a (rt-gpu-measure.flag): the main view's GPU times every 600th frame: per zone (GpuZone: the frame, then the traced
    // passes) the frames it was timed, its mean and largest time in microseconds; the level, ReSTIR, R8b's path vertices and its
    // reference as set at startup.
    {if(renderer.rt_gpu_measure.load(std::memory_order_relaxed) && serial%600==0){const auto w=renderer.TakeRtGpuMeasure();
        if(w.frames && w.frequency){std::string zones;
            for(uint32_t z=0;z<kGpuZones;++z){char cell[96]{};const double n=double(std::max<uint64_t>(w.count[z],1));
                sprintf_s(cell,"%s\"%s\":[%llu,%.1f,%.1f]",z?",":"",kGpuZoneNames[z],static_cast<unsigned long long>(w.count[z]),
                    double(w.ticks[z])*1e6/double(w.frequency)/n,double(w.ticks_max[z])*1e6/double(w.frequency));zones+=cell;}
            char head[384]{};sprintf_s(head,"{\"event\":\"native_rt_gpu\",\"serial\":%llu,\"frames\":%llu,\"width\":%u,\"height\":%u,\"level\":\"%s\",\"restir\":%s,\"bounces\":%u,\"reference\":%s,\"pt_frames\":%llu,\"rt_light_grid\":%s,\"roulette\":%s,\"zones\":{",
                static_cast<unsigned long long>(serial),static_cast<unsigned long long>(w.frames),w.width,w.height,renderer.rt_pt.load()?"pt":"rt",renderer.rt_restir.load()?"true":"false",
                renderer.rt_pt.load()?renderer.rt_bounces.load():1u,renderer.rt_pt.load() && renderer.rt_path_reference.load()?"true":"false",
                static_cast<unsigned long long>(w.pt_frames),renderer.rt_light_grid.load()?"true":"false",
                renderer.rt_pt.load() && renderer.rt_bounces.load()>1 && !renderer.rt_path_reference.load() && renderer.rt_roulette.load()?"true":"false");   // I-0132 R11a: the roulette; I-0128 R8c: the hits' light grid; the step review's L6: the RT level's paths 1; the fixes' L4: the frames the PT level traced
            Log((head+zones+"}}\n").c_str());}}}
    // W10 TM-2: the tone-map stage at work (frames), when it first records and every 1800th frame, or once its failure.
    {static bool failure{},seen{};const auto frames=renderer.TonemapFrames();
        if(!failure && renderer.TonemapFailed()){failure=true;char row[400]{};
            sprintf_s(row,"{\"event\":\"native_tonemap_frame\",\"open\":false,\"reason\":\"%.300s\"}\n",renderer.TonemapFailure().c_str());Log(row);}
        if(frames && (!seen || serial%1800==0)){seen=true;char row[160]{};
            sprintf_s(row,"{\"event\":\"native_tonemap_frame\",\"open\":true,\"frames\":%llu}\n",static_cast<unsigned long long>(frames));Log(row);}}
    // I-0138 tranche 5: the HDR10 output at work (only on an HDR10 chain): the output passes, the tone map's HDR draws, the
    // frames whose 8-bit picture was copied into the float picture, the frames whose UI was not drawn (a build made for the other
    // chain format: none expected); or once why the tone map's HDR entry was given up.
    {static bool look_failure{};if(!look_failure && renderer.ToneLookFailed()){look_failure=true;std::string why=renderer.ToneLookFailure().substr(0,300);for(auto& c:why)if(c=='"' || c=='\\' || c<32)c='\'';   // I-0138 tranche 6
            char row[420]{};sprintf_s(row,"{\"event\":\"native_tone_look\",\"failed\":\"%s\"}\n",why.c_str());Log(row);}}
    {static bool failure{},seen{};const auto passes=renderer.OutputPasses();
        if(!failure && renderer.HdrSceneFailed()){failure=true;std::string why=renderer.HdrSceneFailure().substr(0,300);for(auto& c:why)if(c=='"' || c=='\\' || c<32)c='\'';
            char row[420]{};sprintf_s(row,"{\"event\":\"native_hdr_frame\",\"scene\":false,\"reason\":\"%s\"}\n",why.c_str());Log(row);}
        if(passes && (!seen || serial%1800==0)){seen=true;char row[400]{};
            sprintf_s(row,"{\"event\":\"native_hdr_frame\",\"output_passes\":%llu,\"tone_hdr_frames\":%llu,\"copied_frames\":%llu,\"ui_format_skips\":%llu}\n",
                static_cast<unsigned long long>(passes),static_cast<unsigned long long>(renderer.ToneHdrFrames()),static_cast<unsigned long long>(renderer.HdrCopyPasses()),
                static_cast<unsigned long long>(renderer.UiFormatSkips()));Log(row);}}
    // W10 MB-4: the motion blur stage at work (frames drawn), when it first records and every 1800th frame, or once its failure.
    {static bool failure{},seen{};const auto frames=renderer.MotionBlurFrames();
        if(!failure && renderer.MotionBlurFailed()){failure=true;char row[400]{};
            sprintf_s(row,"{\"event\":\"native_motionblur_frame\",\"open\":false,\"reason\":\"%.300s\"}\n",renderer.MotionBlurFailure().c_str());Log(row);}
        if(frames && (!seen || serial%1800==0)){seen=true;char row[160]{};
            sprintf_s(row,"{\"event\":\"native_motionblur_frame\",\"open\":true,\"frames\":%llu}\n",static_cast<unsigned long long>(frames));Log(row);}}
    // W10 GR: the colour grading stage at work (frames drawn), when it first records and every 1800th frame, or once its failure.
    {static bool failure{},seen{};const auto frames=renderer.GradingFrames();
        if(!failure && renderer.GradingFailed()){failure=true;char row[400]{};
            sprintf_s(row,"{\"event\":\"native_grading_frame\",\"open\":false,\"reason\":\"%.300s\"}\n",renderer.GradingFailure().c_str());Log(row);}
        if(frames && (!seen || serial%1800==0)){seen=true;char row[160]{};
            sprintf_s(row,"{\"event\":\"native_grading_frame\",\"open\":true,\"frames\":%llu}\n",static_cast<unsigned long long>(frames));Log(row);}}
    // I-0126 R7c (W8 AO-7 before): the mask stage's resource: its bytes when they first appear or change, or once its failure.
    {static bool failure{};static uint64_t reported{};const auto bytes=renderer.MaskTargetBytes();
        if(!failure && renderer.MaskFailed()){failure=true;char row[400]{};
            sprintf_s(row,"{\"event\":\"native_mask_resources\",\"open\":false,\"reason\":\"%.300s\"}\n",renderer.MaskFailure().c_str());Log(row);}
        if(bytes!=reported){reported=bytes;char row[160]{};sprintf_s(row,"{\"event\":\"native_mask_resources\",\"open\":%s,\"target_bytes\":%llu}\n",
            bytes?"true":"false",static_cast<unsigned long long>(bytes));Log(row);}}
    if(native_ui_frame){
        if(native_ui_frame->serial!=serial)throw std::runtime_error("native UI current presentation serial");
        const auto ui_generation=native_ui_frame->generation;
        // I-0078 (ER: recording on the render thread; i74-native-83: the build was 0.5 ms of the engine thread): with the render
        // worker the packet goes in the job and the worker builds it before the frame (its timings come back in the outcome).
        if(render_worker){job->ui_packet=std::make_shared<frame::PrimitiveWorkPacket>(std::move(*native_ui_frame));native_ui_build_current=true;}
        else{job->ui=renderer.AdmitPrimitiveWork(std::move(*native_ui_frame));   // X1: the build this frame draws, immutable from here (its payloads moved in)
            for(size_t k=0;k<5;++k)native_ui_admission_ms[k]+=renderer.primitive_admission_ms[k];++native_ui_admissions;}
        job->ui_generation=ui_generation;job->initialize_ui_stencil=true;
        if(model_frame){model_frame->primitive_generation=ui_generation;model_frame->initialize_primitive_stencil=true;}
        native_ui_frame.reset();
    }else if(model_frame && NativeUiBuildCurrent())NoteNativeUiOverlayAbsent(native_ui_step_cause);   // I-0137 tranche 7, below
    const auto ui_cause=native_ui_step_cause;native_ui_step_cause=NativeUiStepCause::StepNotRun;   // read once a frame (the step review's L2)
    clear_mark(2);
    // cR5: the world frame prepared on the worker, with the job's UI fields; the outcome's frame fields are written there
    // (FinishNativeFrame reads them after the job, X10).
    // I-0137 tranche 7 (I-0139's tour: Friede's and the Abyss Watchers' second phase, `i139-t2-b22-friede-nat` and
    // `-b05-watchers-nat`, the render worker closed on « current native model UI overlay »): a world frame that comes with no UI
    // packet while an earlier UI build is current (the UI's display prepared nothing for it, or its packet was made for another frame
    // or extent, TakeNativeUiDisplay) was a refusal since cR2 (« never a world frame without its UI »). The original draws that frame
    // with whatever its UI drew, nothing included (INFERRED: the native selects the UI's movies as E5B940 does and has no packet when
    // none is displayed or none draws); so the native presents it without the overlay (the renderer's world frame with no UI: primitive
    // generation 0, no stencil reset; the next packet starts its stencil afresh), counted by NoteNativeUiOverlayAbsent.
    if(prepare_on_worker && selected)prepare=[selected,frame_serial=serial,ui=bool(job->ui || job->ui_packet),ui_generation=job->ui_generation,
        overlay_current=!job->ui && !job->ui_packet && NativeUiBuildCurrent(),ui_cause,outcome]()->std::shared_ptr<const NativeModelFrameInput>{
        LARGE_INTEGER start{},end{};QueryPerformanceCounter(&start);
        auto frame=PrepareNativeModelFrame(selected,frame_serial);
        QueryPerformanceCounter(&end);native_phase_ticks[1]+=end.QuadPart-start.QuadPart;++native_phase_counts[1];
        if(!frame)return nullptr;
        if(ui){frame->primitive_generation=ui_generation;frame->initialize_primitive_stencil=true;}
        else if(overlay_current)NoteNativeUiOverlayAbsent(ui_cause);
        outcome->lit=bool(frame->lighting);outcome->velocity_draws=frame->velocity_draws.size();
        outcome->velocity_reset=frame->velocity && frame->velocity->reset;
        return frame;};
    job->models=model_frame;
    outcome->serial=serial;
    if(model_frame){outcome->lit=bool(model_frame->lighting);outcome->velocity_draws=model_frame->velocity_draws.size();
        outcome->velocity_reset=model_frame->velocity && model_frame->velocity->reset;}
    model_frame.reset();
    if(!render_worker){RunNativeFrameJob(*job,*outcome);
        LARGE_INTEGER phase_end{};QueryPerformanceCounter(&phase_end);native_phase_ticks[2]+=phase_end.QuadPart-phase_mid.QuadPart;++native_phase_counts[2];
        clear_mark(3);job.reset();FinishNativeFrame(*outcome);return;}   // the job's release after the timer, as before cR3 (inline: the frame in Clear split bucket 3)
    {LARGE_INTEGER phase_end{};QueryPerformanceCounter(&phase_end);native_phase_ticks[2]+=phase_end.QuadPart-phase_mid.QuadPart;++native_phase_counts[2];}
    }
    clear_mark(3);
    // R13 cR3: the post comes after the lock scope (render_worker.h: the idle wait before any native lock, since its pumped
    // wait may re-enter the window procedure). It waits for the previous job, whose outcome is then complete (X10): checked
    // and logged here under the submission lock, as inline.
    render_worker->Post([job=std::shared_ptr<const NativeFrameJob>(std::move(job)),outcome,prepare=std::move(prepare)]{RunNativeFrameJob(*job,*outcome,prepare);});
    clear_mark(4);
    if(const auto previous=std::exchange(pending_frame_outcome,outcome)){
        std::lock_guard<std::mutex> submission(gpu_submissions);FinishNativeFrame(*previous);}
    clear_mark(5);
}
// I-0076 (H2; D-0151: the resolution is an input): the engine's size S (image+46601E8/EC, written by FUN_140EDAA60 on
// WM_SIZE and by the graphics options) becomes the native output extent at the start of the next engine frame, as the
// original's GX resize FUN_14000B700 follows it: the render worker quiesced and its last frame finished, the presentation and
// its targets resized, the scene contexts and views given the new size (their CPU fields the original's resize listeners
// keep), then the worker resumed. A size outside 320x240..16384 is left (the engine's own transient values);
// no-output-follow.flag keeps the start extent (A/B).
void FinishNativeFrame(const NativeFrameOutcome& outcome);
// I-0076: a presentation change between frames (engine thread): the render worker quiesced and its last frame finished, the
// submissions locked while `change` runs, then the worker resumed (also when `change` throws: the caller's refusal ends it).
// The DXGI calls inside send window messages synchronously to the engine's window procedure on this thread; one reaching a
// presentation change again (CSWindowImp's retry -> FUN_14000B590, review M1) is refused with NativePresentationBusy instead
// of locking gpu_submissions twice: the toggle answers "not switched" and the engine retries.
struct NativePresentationBusy {};
thread_local bool native_presentation_changing{};
template<class Change> void WithPresentationIdle(Change&& change) {
    if(native_presentation_changing)throw NativePresentationBusy{};
    struct Changing {Changing(){native_presentation_changing=true;}~Changing(){native_presentation_changing=false;}} changing;
    if(render_worker){render_worker->Quiesce();render_worker->RethrowFault();}
    struct Resume {~Resume(){if(render_worker)render_worker->Resume();}} resume;
    std::lock_guard<std::mutex> submission(gpu_submissions);
    if(const auto previous=std::exchange(pending_frame_outcome,{}))FinishNativeFrame(*previous);
    change();
    native_ui_build_current=renderer.PrimitiveBuildCurrent();   // I-0078: the worker quiesced; a presentation change may retire the build
}
void FollowNativeOutputExtent() {
    if(!opened || native_output_follow_disabled)return;
    uint32_t configured[2]{};
    if(!Read(image+0x46601E8,configured,sizeof(configured)) || (configured[0]==output_width && configured[1]==output_height))return;
    if(configured[0]<320 || configured[1]<240 || configured[0]>16384 || configured[1]>16384)return;
    LARGE_INTEGER frequency{},start{},end{};QueryPerformanceFrequency(&frequency);QueryPerformanceCounter(&start);
    const auto old_width=output_width,old_height=output_height;
    // I-0111 tranche 3: another output extent returns the world to it (as the renderer's Resize); FollowNativeRenderExtent sets the
    // mode's render extent for the new one.
    WithPresentationIdle([&]{renderer.ResizePresentation(configured[0],configured[1]);output_width=configured[0];output_height=configured[1];render_width=render_height=0;});
    // Review M2: the scene fields after the submissions are released (scene_view_cpu_lock comes before gpu_submissions in the
    // lock order); the worker holds no job until this frame posts one.
    {const std::lock_guard<std::recursive_mutex> lock(scene_view_cpu_lock);
        const auto sized=[](uintptr_t context){*reinterpret_cast<uint32_t*>(context+0x20)=output_width;*reinterpret_cast<uint32_t*>(context+0x24)=output_height;
            *reinterpret_cast<uint32_t*>(context+0x3F3C)=output_width;*reinterpret_cast<uint32_t*>(context+0x3F40)=output_height;};
        for(const auto& [context,owner]:scene_context_cpu_owners){(void)owner;sized(context);}
        for(const auto& [view,owner]:scene_view_cpu_owners){*reinterpret_cast<uint32_t*>(view+0xAC)=output_width;*reinterpret_cast<uint32_t*>(view+0xB0)=output_height;
            if(owner.context && !scene_context_cpu_owners.contains(owner.context))sized(owner.context);}}
    QueryPerformanceCounter(&end);
    char row[256]{};sprintf_s(row,"{\"event\":\"native_output_resized\",\"from\":[%u,%u],\"to\":[%u,%u],\"ms\":%.1f}\n",old_width,old_height,output_width,output_height,
        1000.0*double(end.QuadPart-start.QuadPart)/double(frequency.QuadPart));Log(row);
}
// I-0111 tranche 3 (UPSCALER-DLSS-READY §3.1, D-0151): the world's render extent follows the DLSS mode and the output extent, at
// the start of the engine frame after the output follows the engine's size S: the mode's render size (slDLSSGetOptimalSettings)
// while a frame runs DLSS, the output extent otherwise (DLAA, the Antialiasing option off, DLSS refused or latched off). A change quiesces the worker as an output
// change does; frames prepared for the former extent are refused by the prepare (the camera's extent). A row at each change and at
// each new reason the optimal settings are refused for (the output extent kept until the feature answers).
void FollowNativeRenderExtent() {
    if(!opened || !output_width || !output_height)return;
    namespace sl=ds3native::streamline;
    const auto mode=NativeDlssMode();uint32_t width=output_width,height=output_height;
    static uint64_t asked_key{},refused_key{},refused_frames{};static uint32_t asked_width{},asked_height{};static std::string last_refusal;
    if(NativeDlssActive() && mode!=sl::DlssMode::Dlaa){   // tranche 4 step 2: the Antialiasing option off returns the world to the output
        const bool rr=NativeRrActive();   // I-0123: Ray Reconstruction's own optimal extent (its plugin creates NGX at it)
        const uint64_t key=(uint64_t(output_width)<<32)^(uint64_t(output_height)<<8)^uint64_t(mode)^(uint64_t(rr)<<7);
        // Step review L2: a refused key is asked again every 600th frame, not each frame (the query takes Streamline's lock, which an
        // evaluation holds).
        if(key!=asked_key && (key!=refused_key || ++refused_frames%600==0)){std::string reason;uint32_t w{},h{};refused_key=key;
            if(sl::DlssRenderExtent(mode,output_width,output_height,w,h,reason,rr)){asked_key=key;asked_width=w;asked_height=h;last_refusal.clear();}
            else if(reason!=last_refusal){last_refusal=reason;for(auto& c:reason)if(c=='"' || c=='\\' || c<32)c='\'';char row[320]{};
                sprintf_s(row,"{\"event\":\"native_render_extent_refused\",\"mode\":\"%s\",\"output\":[%u,%u],\"reason\":\"%.200s\"}\n",sl::DlssModeName(mode),output_width,output_height,reason.c_str());Log(row);}}
        if(key==asked_key){width=asked_width;height=asked_height;}}
    const bool scaled=width!=output_width || height!=output_height;
    const auto current=renderer.RenderExtent();
    if(width==current[0] && height==current[1] && render_width==(scaled?width:0) && render_height==(scaled?height:0))return;
    LARGE_INTEGER frequency{},start{},end{};QueryPerformanceFrequency(&frequency);QueryPerformanceCounter(&start);
    WithPresentationIdle([&]{renderer.SetRenderExtent(width,height,scaled?mode:sl::DlssMode::Dlaa);render_width=scaled?width:0;render_height=scaled?height:0;});
    QueryPerformanceCounter(&end);
    char row[256]{};sprintf_s(row,"{\"event\":\"native_render_extent\",\"mode\":\"%s\",\"output\":[%u,%u],\"render\":[%u,%u],\"ms\":%.1f}\n",scaled?sl::DlssModeName(mode):"output",
        output_width,output_height,width,height,1000.0*double(end.QuadPart-start.QuadPart)/double(frequency.QuadPart));Log(row);
}
// I-0076 (H1): the role of the original's GX display [4961AB0] for the screen modes, absent natively (DISPLAY-SWAPCHAIN-NATIVE
// 4; ER 1419EF6E0): CSWindowImp's full-screen retry FUN_140EF1D00 asks FUN_14000B540 (the swap chains' full-screen state)
// and calls FUN_14000B590 (the toggle) without testing the display (the crash of WAVE1-CLOSURE 2.3); its mode request
// FUN_140EF54A0 -> FUN_14000C240 is skipped without a display. Natively: the state is the native swap chain's, the toggle
// switches it, and the request is queued and applied at the start of the next engine frame (as the original's GX frame
// start FUN_140009EA0 does), before the output follows the engine's size S.
struct NativeDisplayRequest {bool fullscreen{},borderless{};uint32_t width{},height{};HWND window{};};
std::mutex native_display_lock;std::optional<NativeDisplayRequest> native_display_request;
// I-0076 (D-0170, the remaster's borderless window; ER WM+0x48): borderless.flag sets DS3's own switch, CSWindowImp +0x40,
// which turns full screen (its mode 1) into screen mode 2 (FUN_140EF5590, FUN_140EF1DC0: a popup window, no exclusive
// state; FUN_140EF1D00 then asks for no full-screen state). The original's start-up FUN_140EF46A0 reads it from an encrypted
// launch-option getter (FUN_140ED1080), so it is set after the start-up, and a full-screen target mode set to 2 again.
// I-0132 R12b: or the settings' borderless (R12c's screen-mode write; I-0132 R12f: the play launcher's -Borderless writes that key when the
// file has none; borderless.flag is the test runs' tool).
bool NativeBorderlessOption() noexcept {static const bool on=ModuleFlag(L"borderless.flag") || NativeSettings().borderless;return on;}
void MarkNativeBorderless(uintptr_t window) noexcept {
    if(!NativeBorderlessOption() || !window)return;
    uint8_t current{};int32_t mode{};uintptr_t state{};
    if(!Read(window+0x40,&current,1) || !Read(window+0x78,&mode,4) || !Read(window+0x38,&state,8) || current)return;
    *reinterpret_cast<uint8_t*>(window+0x40)=1;
    if(mode==1 && state)reinterpret_cast<void(__fastcall*)(uintptr_t,uint32_t)>(image+0xEF1C50)(state,2);
    char row[128]{};sprintf_s(row,"{\"event\":\"native_borderless_option\",\"mode\":%d}\n",mode);Log(row);
}
using WindowStartupFn=void(__fastcall*)(uintptr_t);WindowStartupFn window_startup_original{};
void __fastcall NativeWindowStartup(uintptr_t window) noexcept {window_startup_original(window);MarkNativeBorderless(window);}
// At the presentation's start (the window's start-up may precede the hooks): the switch set, and a full-screen start placed
// borderless at the first engine frame.
void StartNativeBorderless() noexcept {
    if(!NativeBorderlessOption())return;
    uintptr_t window{};if(!Read(image+0x489F118,&window,8) || !window)return;
    MarkNativeBorderless(window);
    int32_t mode{};HWND hwnd{};if(!Read(window+0x78,&mode,4) || mode!=1 || !Read(window+8,&hwnd,8))return;
    const std::lock_guard hold(native_display_lock);native_display_request=NativeDisplayRequest{false,true,output_width,output_height,hwnd};
}
uint64_t __fastcall NativeDisplayFullscreen(uintptr_t) noexcept {
    if(!opened)return 0;
    try{return renderer.PresentationFullscreen()?1:0;}catch(...){return 0;}
}
uint32_t __fastcall NativeDisplayToggleFullscreen(uintptr_t) noexcept {
    if(!opened)return 0;
    bool switched{};
    try{WithPresentationIdle([&]{switched=renderer.TogglePresentationFullscreen();});}
    catch(const NativePresentationBusy&){return 0;}   // review M1: inside another presentation change; the engine retries
    catch(const std::exception& e){LogNativeFailure("full-screen toggle: ",e.what());Refuse("native_display_toggle_failed");}
    char row[160]{};sprintf_s(row,"{\"event\":\"native_display_toggle\",\"switched\":%s,\"fullscreen\":%s}\n",switched?"true":"false",renderer.PresentationFullscreen()?"true":"false");Log(row);
    return switched?1u:0u;
}
using NativeDisplayRequestFn=bool(__fastcall*)(uintptr_t,uint64_t);NativeDisplayRequestFn display_request_original{};
bool __fastcall QueueNativeDisplayRequest(uintptr_t window,uint64_t resolution) noexcept {
    const bool requested=display_request_original(window,resolution);   // the window's own part; its GX request is skipped without a display
    if(!opened)return requested;
    int32_t mode{};HWND hwnd{};uint8_t borderless{};
    if(!Read(window+0x78,&mode,4) || !Read(window+8,&hwnd,8) || !Read(window+0x40,&borderless,1))return requested;
    // The original asks GX for windowed = (mode != 1); with the +0x40 switch, full screen is borderless (screen mode 2).
    const NativeDisplayRequest request{mode==1 && !borderless,mode==1 && borderless,static_cast<uint32_t>(resolution),static_cast<uint32_t>(resolution>>32),hwnd};
    {const std::lock_guard hold(native_display_lock);native_display_request=request;}
    if(hwnd){SetForegroundWindow(hwnd);SetActiveWindow(hwnd);}
    char row[192]{};sprintf_s(row,"{\"event\":\"native_display_request\",\"fullscreen\":%s,\"borderless\":%s,\"width\":%u,\"height\":%u,\"mode\":%d}\n",
        request.fullscreen?"true":"false",request.borderless?"true":"false",request.width,request.height,mode);Log(row);
    return true;   // as the original with its GX display (FUN_140EF54A0 returns display != 0): the request is queued natively
}
void ApplyNativeDisplayRequest() {
    std::optional<NativeDisplayRequest> request;
    {const std::lock_guard hold(native_display_lock);request.swap(native_display_request);}
    if(!request || !opened)return;
    if(request->width<320 || request->height<240 || request->width>16384 || request->height>16384)return;
    WithPresentationIdle([&]{renderer.ApplyDisplayMode(request->fullscreen,request->borderless,request->width,request->height);});
    char row[192]{};sprintf_s(row,"{\"event\":\"native_display_mode\",\"fullscreen\":%s,\"borderless\":%s,\"width\":%u,\"height\":%u,\"state\":%s}\n",request->fullscreen?"true":"false",
        request->borderless?"true":"false",request->width,request->height,renderer.PresentationFullscreen()?"true":"false");Log(row);
    // A borderless placement that does not change the window's size (a full-screen start whose window already covers the
    // monitor) sends no WM_SIZE, so the engine's size S (FUN_140EF50D0 -> FUN_140EDAA60) kept its start value (run 47: S
    // 1280x720 in a 1920x1080 client). The engine's window procedure is given the size the client has.
    RECT client{};uint32_t size[2]{};
    // I-0097 tranche 3 (i97-t3m1-nb280): a mode the game asked for and the presentation applied is no window change of the
    // start-up's views: their client becomes the one the mode gave (a borderless start on a 2560x1440 screen grew the 1920x1080
    // client and the views refused, native_view_window_extent_changed).
    if(request->window && GetClientRect(request->window,&client) && client.right>0 && client.bottom>0){
        start_client_width=uint32_t(client.right);start_client_height=uint32_t(client.bottom);}
    if(request->borderless && request->window && GetClientRect(request->window,&client) && Read(image+0x46601E8,size,sizeof(size)) &&
       (size[0]!=uint32_t(client.right) || size[1]!=uint32_t(client.bottom)))
        PostMessageW(request->window,WM_SIZE,SIZE_RESTORED,MAKELPARAM(client.right,client.bottom));
}
// I-0101 (STUDY §3.6; ER FUN_1419ED670 and FUN_140E91A70/FUN_140E92690, DS3 FUN_140EF5590): the presentation followed at
// the engine frame's start, before a mode request. 1) A Present failure admitted by the worker, or an exclusive full screen
// DXGI left by itself (a focus loss, the capture tool): the buffers resized between frames (`native_display_lost`). 2) The
// minimise both games do when their target mode is exclusive full screen and the chain is not (DS3's branch is gated by the
// GX display, null natively): once per loss, not every frame, since natively WM_SIZE's re-entry toggle is gated too and a
// restored window would be minimised again before it is exclusive. 3) The re-entry: DS3's ungated retry FUN_140EF1D00
// toggles back through our hook when it is armed; as ER on WM_ACTIVATEAPP, the follow also queues the full-screen mode once
// when the window is in front again, restored and still not exclusive. 4) DS3's focus state (STUDY §3.6 step 4): the gated
// half of FUN_140EF5590's end, with our state for FUN_14000B540: in front, CSWindowImp's +0x64 above 1, and +0x62 changed or
// +0x63 set while exclusive: FUN_1417DC0C0(DirectInput manager [1449644B8], f == 0), whose +0x24C chooses the keyboard's
// DISCL_NOWINKEY at its next acquisition (FUN_14183BEF0); natively it was never lifted after a focus loss.
bool native_fullscreen_reentry{};
void FollowNativePresentationState() {
    if(!opened)return;
    uintptr_t window{};int32_t mode{};uint8_t borderless{};HWND hwnd{};
    const bool engine_window=Read(image+0x489F118,&window,8) && window && Read(window+0x78,&mode,4) && Read(window+0x40,&borderless,1) && Read(window+8,&hwnd,8) && hwnd;
    const bool target_exclusive=engine_window && mode==1 && !borderless;
    if(renderer.PresentationNeedsFollow()){
        std::optional<SceneRenderer::PresentationLoss> loss;
        WithPresentationIdle([&]{loss=renderer.FollowPresentation();});
        if(loss){
            // The minimise and the re-entry belong to the loss of the exclusive state (review M2: not to a Present that keeps failing).
            const bool minimise=target_exclusive && loss->was_exclusive && !loss->fullscreen && !IsIconic(hwnd);
            if(minimise)ShowWindow(hwnd,SW_MINIMIZE);
            if(loss->was_exclusive)native_fullscreen_reentry=target_exclusive && !loss->fullscreen;
            if(loss->failing<=3 || loss->failing%600==0){char row[640]{};sprintf_s(row,"{\"event\":\"native_display_lost\",\"present_hresult\":\"0x%08lX\",\"present_failures\":%llu,\"failing\":%llu,\"resized\":%s,\"resize_hresult\":\"0x%08lX\",\"was_exclusive\":%s,\"fullscreen\":%s,\"failed_windowed\":%s,\"failed_iconic\":%s,\"failed_foreground\":%s,\"failed_buffers\":[%u,%u],\"failed_client\":[%u,%u],\"target_exclusive\":%s,\"minimised\":%s,\"stand_in\":%s}\n",
                static_cast<unsigned long>(loss->status),static_cast<unsigned long long>(loss->failures),static_cast<unsigned long long>(loss->failing),loss->resized?"true":"false",static_cast<unsigned long>(loss->resize_status),
                loss->was_exclusive?"true":"false",loss->fullscreen?"true":"false",
                loss->windowed?"true":"false",loss->iconic?"true":"false",loss->foreground?"true":"false",loss->width,loss->height,loss->client_width,loss->client_height,
                target_exclusive?"true":"false",minimise?"true":"false",loss->stand_in?"true":"false");Log(row);}}}
    if(!engine_window)return;
    const bool front=GetForegroundWindow()==hwnd;
    if(native_fullscreen_reentry && (!target_exclusive || renderer.PresentationFullscreen()))native_fullscreen_reentry=false;
    else if(native_fullscreen_reentry && front && !IsIconic(hwnd)){
        native_fullscreen_reentry=false;uint64_t resolution{};
        if(Read(window+0xB8,&resolution,8) && uint32_t(resolution) && uint32_t(resolution>>32)){
            {const std::lock_guard hold(native_display_lock);if(!native_display_request)native_display_request=NativeDisplayRequest{true,false,uint32_t(resolution),uint32_t(resolution>>32),hwnd};}
            char row[160]{};sprintf_s(row,"{\"event\":\"native_display_reentry\",\"width\":%u,\"height\":%u}\n",uint32_t(resolution),uint32_t(resolution>>32));Log(row);}}
    int16_t settled{};uint8_t last{},lost_focus{};uintptr_t manager{};
    if(!front || !Read(window+0x64,&settled,2) || settled<=1 || !Read(window+0x62,&last,1) || !Read(window+0x63,&lost_focus,1))return;
    const uint8_t f=renderer.PresentationFullscreen()?1:0;
    if(last!=f || (lost_focus && f)){
        if(!Read(image+0x49644B8,&manager,8) || !manager)return;   // review L5: no manager yet, the change kept for a later frame
        reinterpret_cast<void(__fastcall*)(uintptr_t,uint8_t)>(image+0x17DC0C0)(manager,f==0);
        *reinterpret_cast<uint8_t*>(window+0x63)=f==0;}
    *reinterpret_cast<uint8_t*>(window+0x62)=f;
}
// I-0076 test (display-cycle.flag): the graphics options' Apply, done as its setters do (FUN_140B07C40: CSWindowImp
// [14489F118] +0xC0 = full screen; FUN_140B07CE0: +0xB0 the windowed, +0xB8 the full-screen resolution), at fixed presented
// frames; CSWindowImp's own frame step then applies it. A state row follows each step by 240 frames.
void StepNativeDisplayCycle() {
    if(!native_display_cycle || !opened)return;
    struct Step {LONG64 frame;int32_t fullscreen;uint32_t width,height;};
    static constexpr Step steps[]{{3000,0,1600,900},{3600,1,2560,1440},{4200,1,1920,1080},{4800,0,1280,720},{5400,1,2560,1440}};
    static size_t next{},state{};
    const LONG64 presented=control->native_frames;
    uintptr_t window{};if(!Read(image+0x489F118,&window,8) || !window)return;
    if(state<std::size(steps) && presented>=steps[state].frame+240 && state<next){
        HWND hwnd{};Read(window+8,&hwnd,8);RECT client{},frame{};if(hwnd){GetClientRect(hwnd,&client);GetWindowRect(hwnd,&frame);}
        uint32_t size[2]{};Read(image+0x46601E8,size,8);int32_t mode{};Read(window+0x78,&mode,4);
        char row[384]{};sprintf_s(row,"{\"event\":\"native_display_cycle_state\",\"step\":%zu,\"frame\":%lld,\"mode\":%d,\"fullscreen\":%s,\"engine_size\":[%u,%u],\"output\":[%u,%u],\"client\":[%ld,%ld],\"window\":[%ld,%ld,%ld,%ld],\"style\":\"0x%08lX\"}\n",
            state,presented,mode,renderer.PresentationFullscreen()?"true":"false",size[0],size[1],output_width,output_height,client.right,client.bottom,
            frame.left,frame.top,frame.right,frame.bottom,hwnd?static_cast<unsigned long>(GetWindowLongW(hwnd,GWL_STYLE)):0ul);Log(row);++state;}
    if(next>=std::size(steps) || presented<steps[next].frame)return;
    const auto& step=steps[next++];
    const uint64_t resolution=uint64_t(step.width)|(uint64_t(step.height)<<32);
    *reinterpret_cast<uint64_t*>(window+(step.fullscreen?0xB8:0xB0))=resolution;
    *reinterpret_cast<uint32_t*>(window+0xC0)=step.fullscreen?1u:0u;
    char row[192]{};sprintf_s(row,"{\"event\":\"native_display_cycle_step\",\"step\":%zu,\"frame\":%lld,\"fullscreen\":%s,\"width\":%u,\"height\":%u}\n",
        next-1,presented,step.fullscreen?"true":"false",step.width,step.height);Log(row);
}
// I-0082 (DESIGN 5): each fault path proved on purpose, behind its flag (announced test runs only), at the 1,200th presented
// frame (the title screen, no pilot needed), on the engine thread: a write through a null pointer, a removed device, a queue
// waiting on a fence nobody signals (the next completion wait reaches its 5 s deadline). Confined to this process's device.
void StepNativeFaultTests(LONG64 presented) {
    static const bool crash=ModuleFlag(L"test-crash.flag"),removed=ModuleFlag(L"test-device-removed.flag"),hang=ModuleFlag(L"test-gpu-hang.flag");
    if(presented!=1200 || !(crash || removed || hang))return;
    char row[160]{};sprintf_s(row,"{\"event\":\"native_fault_test\",\"frame\":%lld,\"test\":\"%s\"}\n",static_cast<long long>(presented),crash?"crash":removed?"device_removed":"gpu_hang");Log(row);
    if(crash){volatile int* volatile target=nullptr;*target=1;}
    else if(removed){Microsoft::WRL::ComPtr<ID3D12Device5> removable;if(SUCCEEDED(gpu.device.As(&removable)))removable->RemoveDevice();}
    else{static Microsoft::WRL::ComPtr<ID3D12Fence> never;if(SUCCEEDED(gpu.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&never))))gpu.queue->Wait(never.Get(),1);}
}
void FinishNativeFrame(const NativeFrameOutcome& outcome) {
    const auto& receipt=outcome.receipt;const auto frame_serial=outcome.serial;
    if(outcome.ui_built){for(size_t k=0;k<5;++k)native_ui_admission_ms[k]+=outcome.ui_admission_ms[k];++native_ui_admissions;}   // I-0078: built on the worker
    if(receipt.serial!=frame_serial || !receipt.fence || !receipt.presentation_fence || receipt.debug_errors ||
        (FAILED(receipt.present_status) && !receipt.present_lost)) throw std::runtime_error("common renderer startup receipt");   // I-0101: an admitted Present failure completes its frame
    char row[512]{};sprintf_s(row,"{\"event\":\"%s\",\"serial\":%llu,\"draws\":%u,\"content_draws\":%u,\"fence\":%llu,\"presentation_fence\":%llu,\"width\":%u,\"height\":%u,\"present_hresult\":%lu,\"debug_errors\":%u,\"lit\":%s,\"zone_rendered\":false}\n",
        receipt.content_draw_count?"native_model_draw_submitted":receipt.primitive_draw_count?"native_ui_draw_submitted":"native_clear",frame_serial,receipt.primitive_draw_count,receipt.content_draw_count,receipt.fence,receipt.presentation_fence,output_width,output_height,
        static_cast<unsigned long>(receipt.present_status),receipt.debug_errors,outcome.lit?"true":"false");
    const auto presented=InterlockedIncrement64(&control->native_frames);
    native_sample_mark.store(static_cast<uint64_t>(presented),std::memory_order_relaxed);   // I-0074: the samples after this carry it
    // R15: a measurement run keeps one frame row per pacing window and the rows where the frame kind changes.
    {static int last_kind=-1;const int kind=receipt.content_draw_count?2:receipt.primitive_draw_count?1:0;
        if(!measure_run || kind!=last_kind || presented%120==1)Log(row);last_kind=kind;}
    if(presented%300==0)ProbeWorldUpdateHooks(frame_serial);
    StepNativeFaultTests(presented);
    if(presented%600==1){const auto census_row=census::Row(static_cast<uint64_t>(presented));Log(census_row.c_str());}   // R13 cR0 (TM-1, TM-4)
    // I-0117 (R2 T0): each frame the models' BLAS whose build completed compacted (step review M1: not in the 120-frame window
    // below, so the large structures and their postbuild slots go a frame after their build); every 600 frames the registry's row
    // (since the start, then alive now). T0 final review L2: after a failure the next Advance waits 60 frames, its row rate-limited.
    if(native_rt_scene.load(std::memory_order_relaxed)){
        static int64_t rt_advance_after{};static uint64_t rt_advance_failures{};
        if(presented>=rt_advance_after)try{(void)native_rt_blas.Advance(gpu);}catch(const std::exception& error){rt_advance_after=presented+60;
            std::string why;for(const char* c=error.what();c && *c && why.size()<160;++c)why.push_back(*c=='"' || *c=='\\' || *c<32 || *c>126?'?':*c);   // T2 step review L5
            if(++rt_advance_failures<=8 || rt_advance_failures%600==0){char rt_row[240]{};
                sprintf_s(rt_row,"{\"event\":\"native_rt_blas_advance_failed\",\"failures\":%llu,\"reason\":\"%.160s\"}\n",static_cast<unsigned long long>(rt_advance_failures),why.c_str());Log(rt_row);}}
        if(presented%600==60){const auto c=native_rt_blas.Counters();char rt_row[720]{};
            sprintf_s(rt_row,"{\"event\":\"native_rt_blas\",\"frame\":%lld,\"models\":%llu,\"geometries\":%llu,\"skipped\":%llu,\"triangles\":%llu,\"built_mb\":%.1f,\"compacted_mb\":%.1f,\"ready\":%llu,\"failed\":%llu,\"pending\":%llu,\"slots_free\":%llu,\"live_models\":%llu,\"live_placed_mb\":%.1f,\"tlas_mb\":%.2f,\"tlas_failed\":\"%.120s\"}\n",
                static_cast<long long>(presented),static_cast<unsigned long long>(c.models),static_cast<unsigned long long>(c.geometries),static_cast<unsigned long long>(c.skipped),
                static_cast<unsigned long long>(c.triangles),double(c.built_bytes)/1048576.0,double(c.compacted_bytes)/1048576.0,static_cast<unsigned long long>(c.ready),
                static_cast<unsigned long long>(c.failed),static_cast<unsigned long long>(c.pending),static_cast<unsigned long long>(c.slots_free),
                static_cast<unsigned long long>(c.live_models),double(c.live_placed_bytes)/1048576.0,double(renderer.RtSceneBytes())/1048576.0,renderer.RtSceneFailure().c_str());Log(rt_row);}}
    // I-0079: the streamed creation memory: a spare empty heap released every 120 frames (at most one of each kind), the
    // counters every 600 (heaps, placed and committed creations, live placed bytes, staging chunks).
    // I-0096: with no world drawn, a pending reset of the kept draws is applied first (the zone's resources go back to their
    // heaps once the queue passes them) and every spare empty heap is released, not one a call.
    // I-0113 tranche 11 (gap 13, a frame of a spell's burst): with pix-capture.flag, F8 (game window in front) captures the next
    // frame too (pix-f8-<frame>.wpix beside the module). Tranche 12 (gap 12, Flame Fan's fan lasts about half a second and the
    // pilot's keys land 0.5 to 1 s after they are sent): the first F8 of a session takes a burst of 4 captures, the first `delay`
    // presented frames after the key and then every `spacing` frames, both from pix-f8.txt beside the module (« delay spacing »,
    // default 0 6); later presses are ignored.
    {static const bool pix=ModuleFlag(L"pix-capture.flag");
        if(pix){static bool f8_down{};static long long burst_from=-1;static int taken{};
            static const std::pair<int,int> timing=[]{int delay=0,spacing=6;const auto path=ModuleSibling(L"pix-f8.txt");FILE* file{};
                if(!path.empty() && !_wfopen_s(&file,path.c_str(),L"rb") && file){if(fscanf_s(file,"%d %d",&delay,&spacing)!=2){delay=0;spacing=6;}fclose(file);}
                return std::pair<int,int>{std::clamp(delay,0,600),std::clamp(spacing,1,120)};}();
            if(taken<4){DWORD owner{};GetWindowThreadProcessId(GetForegroundWindow(),&owner);
                const bool down=owner==GetCurrentProcessId() && (GetAsyncKeyState(VK_F8)&0x8000);
                if(down && !f8_down && burst_from<0)burst_from=presented+timing.first;
                f8_down=down;
                if(burst_from>=0 && presented>=burst_from+static_cast<long long>(timing.second)*taken){++taken;using CaptureNext=HRESULT(WINAPI*)(PCWSTR,UINT32);HRESULT hr=E_FAIL;
                    const auto capture=reinterpret_cast<CaptureNext>(GetProcAddress(GetModuleHandleW(L"WinPixGpuCapturer.dll"),"CaptureNextFrame"));
                    if(capture){const auto file=ModuleSibling((L"pix-f8-"+std::to_wstring(presented)+L".wpix").c_str());if(!file.empty())hr=capture(file.c_str(),1);}
                    char pix_row[160]{};sprintf_s(pix_row,"{\"event\":\"native_pix_capture\",\"frame\":%lld,\"key\":\"F8\",\"burst\":%d,\"export\":%s,\"hr\":\"0x%08X\"}\n",static_cast<long long>(presented),taken,capture?"true":"false",static_cast<unsigned>(hr));Log(pix_row);}}}}
    if(presented%120==60){const bool idle=!receipt.content_draw_count;const size_t kept_released=idle?ApplyPendingNativeKeptDrawReset():0;
        if(kept_released){char kept_row[160]{};sprintf_s(kept_row,"{\"event\":\"native_kept_draws_released\",\"frame\":%lld,\"draws\":%zu}\n",static_cast<long long>(presented),kept_released);Log(kept_row);}
        // I-0100 tranche 2 (pix-capture.flag): a PIX GPU capture of one frame at presented frames 3060, 3660, 4260 and 4860 (the
        // replayed paths' world after their settle), beside the module as pix-<frame>.wpix, opened afterwards with pixtool.
        // I-0113 tranche 12: not when pix-f8.txt sets an F8 burst (2.8 GB and four stalls saved). Five capture runs closed by
        // ExitProcess ended on a fault in WinPixGpuCapturer.dll under the static destruction of the module's D3D12 objects at the DLL's
        // detach (native_reflection's textures; one closed through the module did not): the journal and captures stand.
        {static const bool pix=ModuleFlag(L"pix-capture.flag") && [] {const auto path=ModuleSibling(L"pix-f8.txt");return path.empty() || GetFileAttributesW(path.c_str())==INVALID_FILE_ATTRIBUTES;}();
            if(pix && presented>=3000 && presented<=4900 && presented%600==60){   // inside the 120-frame window (presented%120==60)
                using CaptureNext=HRESULT(WINAPI*)(PCWSTR,UINT32);   // the capturer's export (file, frames): i100-t2g-pix3 faulted in lstrlenW on a flags word first
                const auto capture=reinterpret_cast<CaptureNext>(GetProcAddress(GetModuleHandleW(L"WinPixGpuCapturer.dll"),"CaptureNextFrame"));
                HRESULT hr=E_FAIL;
                if(capture){const auto file=ModuleSibling((L"pix-"+std::to_wstring(presented)+L".wpix").c_str());if(!file.empty())hr=capture(file.c_str(),1);}
                char pix_row[160]{};sprintf_s(pix_row,"{\"event\":\"native_pix_capture\",\"frame\":%lld,\"export\":%s,\"hr\":\"0x%08X\"}\n",static_cast<long long>(presented),capture?"true":"false",static_cast<unsigned>(hr));Log(pix_row);}}
        // Step review 3, final review 1: all spares only after two windows without a world and without an admission (the title),
        // not on a loading screen, where the next zone takes heaps again.
        static bool idle_before{};static uint64_t admissions_before{};
        uint64_t admissions=textures.Version();{const std::shared_lock hold(model_storage_lock);admissions+=model_storage_generation;}
        const bool settled=idle && idle_before && admissions==admissions_before;idle_before=idle;admissions_before=admissions;
        // I-0097 tranche 4 (D-0211): the kept texture contents released first once settled (the title keeps the memory it had,
        // I-0096), so their heaps can go with the spares below; every 600 frames their hits, misses and size.
        if(settled)if(const auto kept=textures.TrimKept()){char kept_row[128]{};sprintf_s(kept_row,"{\"event\":\"native_texture_kept_released\",\"frame\":%lld,\"mb\":%.1f}\n",
            static_cast<long long>(presented),double(kept)/1048576.0);Log(kept_row);}
        if(presented%600==60){const auto k=textures.KeptCount();char kept_row[256]{};sprintf_s(kept_row,"{\"event\":\"native_texture_kept\",\"frame\":%lld,\"hits\":%llu,\"misses\":%llu,\"entries\":%llu,\"retired\":%llu,\"retired_mb\":%.1f,\"placed_mb\":%.1f}\n",
            static_cast<long long>(presented),static_cast<unsigned long long>(k.hits),static_cast<unsigned long long>(k.misses),static_cast<unsigned long long>(k.entries),
            static_cast<unsigned long long>(k.retired),double(k.retired_bytes)/1048576.0,double(k.placed_bytes)/1048576.0);Log(kept_row);}
        const auto trimmed=gpu.TrimStreamedMemory(0,0,settled);
        if(presented%600==60 || trimmed){const auto m=gpu.StreamedMemory();char memory_row[640]{};
            // I-0096: per pool (textures, small textures, buffers) the heaps' MB, the live MB and the heaps with every range free.
            const auto mb=[](uint64_t bytes){return double(bytes)/1048576.0;};const auto& p=m.pools;
            sprintf_s(memory_row,
            "{\"event\":\"native_streamed_memory\",\"frame\":%lld,\"heaps\":%llu,\"placed\":%llu,\"committed\":%llu,\"placed_live_mb\":%.1f,\"staging_chunks\":%llu,\"staging_dedicated\":%llu,\"staging_mb\":%.1f,\"trimmed\":%zu,"
            "\"pool_heap_live_mb_empty\":[[%.0f,%.1f,%llu],[%.0f,%.1f,%llu],[%.0f,%.1f,%llu]]}\n",
            static_cast<long long>(presented),static_cast<unsigned long long>(m.heaps),static_cast<unsigned long long>(m.placed),static_cast<unsigned long long>(m.committed),
            double(m.placed_live_bytes)/1048576.0,static_cast<unsigned long long>(m.staging_chunks),static_cast<unsigned long long>(m.staging_dedicated),double(m.staging_bytes)/1048576.0,trimmed,
            mb(p[0].heap_bytes),mb(p[0].live_bytes),static_cast<unsigned long long>(p[0].empty_heaps),mb(p[1].heap_bytes),mb(p[1].live_bytes),static_cast<unsigned long long>(p[1].empty_heaps),
            mb(p[2].heap_bytes),mb(p[2].live_bytes),static_cast<unsigned long long>(p[2].empty_heaps));Log(memory_row);}}
    // I-0085: the model registry's sizes every 600 frames (the night endurance test's leak check: the layouts and materials
    // live, the layouts retired with their material, the spare prefixes waiting for a build).
    if(presented%600==90){size_t materials_live{},layouts_live{},spares{};uint64_t retired{};
        {const std::shared_lock hold(model_material_lock);materials_live=model_materials.size();}
        {const std::shared_lock hold(model_layout_lock);layouts_live=model_layouts.size();spares=model_layout_spare_prefixes.size();retired=model_layouts_retired;}
        const auto ingress=textures.Count();   // I-0087: the texture records and their render targets
        // I-0096: the FLVER storages and the CPU bytes they keep; the destroyed storages something still holds, and their buffers.
        // No HeapSummary here: it walks the process heap under its lock, and every 600 frames it held the engine thread's
        // allocations long enough to make a game step over 34 ms (22 of 23 in i96-ab-branch-m1 followed this row; gate i96-final-b).
        size_t storages{},storages_held{};uint64_t storage_bytes{},storages_held_gpu{};
        {const std::shared_lock hold(model_storage_lock);storages=model_storage.size();for(const auto& [wrapper,storage]:model_storage){(void)wrapper;storage_bytes+=storage->bytes.size();}}
        std::vector<std::shared_ptr<NativeModelStorage>> held_storages;   // step review 2: released after the lock
        {const std::lock_guard census(model_storage_withdrawn_lock);
            std::erase_if(model_storage_withdrawn,[&](const std::weak_ptr<NativeModelStorage>& weak){auto storage=weak.lock();if(!storage)return true;held_storages.push_back(std::move(storage));return false;});}
        {const std::shared_lock hold(model_storage_lock);   // I-0097 step review: a queued buffer job may still set one
            for(const auto& storage:held_storages){++storages_held;storages_held_gpu+=storage->resource?storage->resource->GetDesc().Width:0;}}
        char registry_row[1536]{};sprintf_s(registry_row,"{\"event\":\"native_model_registry\",\"frame\":%lld,\"materials\":%zu,\"layouts\":%zu,\"layouts_retired\":%llu,\"spare_prefixes\":%zu,\"texture_records\":%zu,\"render_targets\":%zu,\"render_target_mb\":%.1f,\"texture_cpu_image_mb\":%.1f,\"textures_withdrawn_held\":%zu,\"textures_withdrawn_held_mb\":%.1f,\"textures_withdrawn_largest_kb\":[%s],\"model_storages\":%zu,\"model_storage_cpu_mb\":%.1f,\"storages_destroyed_held\":%zu,\"storages_destroyed_held_gpu_mb\":%.1f,\"storage_queue_pending\":%llu,\"storage_queue_max_ms\":%.1f}\n",
            static_cast<long long>(presented),materials_live,layouts_live,static_cast<unsigned long long>(retired),spares,ingress.records,ingress.render_targets,double(ingress.render_target_bytes)/1048576.0,
            double(ingress.cpu_image_bytes)/1048576.0,ingress.withdrawn_held,double(ingress.withdrawn_held_bytes)/1048576.0,ingress.withdrawn_largest.c_str(),
            storages,double(storage_bytes)/1048576.0,storages_held,double(storages_held_gpu)/1048576.0,
            [&]{const auto done=StorageQueue().Done();const auto posted=StorageQueue().Posted();return static_cast<unsigned long long>(posted>done?posted-done:0);}(),double(storage_queue_delay_max_us.exchange(0))/1000.0);Log(registry_row);}
    // cD4 (R6, X3): the frames whose slots were released during this one (this frame itself with one slot). A debug-layer
    // error already ended the frame function; the velocity statistics of a sampled frame are logged with its own serial.
    for(const auto& completed:renderer.TakeCompleted()){
        if(completed.debug_errors)throw std::runtime_error("native completed frame diagnostic error: "+completed.diagnostic);
        if(!completed.velocity.empty()){static LONG64 rows{};if(InterlockedIncrement64(&rows)<=200){
            const bool current=completed.serial==frame_serial;
            const std::string velocity_row="{\"event\":\"native_velocity_frame\",\"serial\":"+std::to_string(completed.serial)+",\"draws\":"+
                (current?std::to_string(outcome.velocity_draws):std::string("null"))+",\"reset\":"+
                (current?(outcome.velocity_reset?"true":"false"):"null")+
                ",\"stats\":"+completed.velocity+"}\n";Log(velocity_row.c_str());}}}
    // 1120: native presentation pacing (wall time per window of 120 presented frames, slowest frame); a
    // measurement of the development build (validation layer, per-frame trace rows), not a performance claim.
    {static LARGE_INTEGER frequency{},window_start{},last{};static double slowest{};LARGE_INTEGER now{};
        static double record_sum{},gpu_sum{},present_sum{};static uint32_t render_count{};
        if(receipt.content_draw_count){record_sum+=receipt.record_ms;gpu_sum+=receipt.gpu_wait_ms;present_sum+=receipt.present_ms;++render_count;}
        if(receipt.content_draw_count && native_sample_frames.fetch_add(1,std::memory_order_relaxed)+1==native_sample_start_frame)native_sample_active.store(1,std::memory_order_relaxed);
        if(!frequency.QuadPart)QueryPerformanceFrequency(&frequency);QueryPerformanceCounter(&now);
        {const uint64_t pending=gpu.PendingRetirements();auto known=retirements_pending_peak.load();   // cD0: peak over the window
            while(pending>known && !retirements_pending_peak.compare_exchange_weak(known,pending)){}}
        if(last.QuadPart)slowest=std::max(slowest,static_cast<double>(now.QuadPart-last.QuadPart)/frequency.QuadPart);
        // Frame pacing (Thomas, 2026-10-12): every frame interval of the window, summarised in native_frame_times.
        static std::vector<float> intervals;const bool timed=last.QuadPart!=0;if(timed)intervals.push_back(float(1000.0*double(now.QuadPart-last.QuadPart)/frequency.QuadPart));last=now;
        // Optimisation review: a frame over 34 ms (I-0079; 50 ms before) names what its interval held (the phase times since the previous frame,
        // the pipeline states created, the programmes analysed, the textures admitted, the ingress lists); 1,000 rows a run.
        {static std::array<int64_t,3> previous_phase{};static uint64_t previous_pso{},previous_analysed{},previous_textures{},previous_ingress{};static int slow_rows{};
            std::array<double,3> phase_ms{};
            for(size_t k=0;k<3;++k){const auto t=native_phase_ticks[k].load(std::memory_order_relaxed);phase_ms[k]=1000.0*double(t>=previous_phase[k]?t-previous_phase[k]:t)/frequency.QuadPart;previous_phase[k]=t;}
            const uint64_t pso=NativeModelDrawCounters().pso_created,analysed=native_program_analysis_misses.load(std::memory_order_relaxed),
                admitted=native_texture_decodes.load(std::memory_order_relaxed),ingress=gpu.ingress_lists;
            if(timed && intervals.back()>34.f && slow_rows<1000){++slow_rows;char slow[520]{};
                sprintf_s(slow,"{\"event\":\"native_slow_frame\",\"serial\":%llu,\"presented\":%lld,\"interval_ms\":%.1f,\"collect_ms\":%.1f,\"prepare_ms\":%.1f,\"render_ms\":%.1f,\"record_ms\":%.2f,\"gpu_wait_ms\":%.2f,\"present_ms\":%.2f,\"pso_created\":%llu,\"programmes_analysed\":%llu,\"textures_admitted\":%llu,\"ingress_lists\":%llu,\"content_draws\":%u,\"tick_ms\":%llu}\n",
                    static_cast<unsigned long long>(frame_serial),static_cast<long long>(presented),intervals.back(),phase_ms[0],phase_ms[1],phase_ms[2],receipt.record_ms,receipt.gpu_wait_ms,receipt.present_ms,
                    static_cast<unsigned long long>(pso-previous_pso),static_cast<unsigned long long>(analysed-previous_analysed),static_cast<unsigned long long>(admitted-previous_textures),
                    static_cast<unsigned long long>(ingress-previous_ingress),receipt.content_draw_count,static_cast<unsigned long long>(GetTickCount64()));Log(slow);}
            previous_pso=pso;previous_analysed=analysed;previous_textures=admitted;previous_ingress=ingress;}
        // Queue counter baselines start with the first window (not at device creation).
        static uint64_t last_executes{},last_drains{},last_completes{},last_arena_bytes{},last_ingress{};
        if(presented%120==1 && !window_start.QuadPart){last_executes=gpu.executes;last_drains=gpu.drains;last_completes=gpu.completes;last_arena_bytes=UploadArenaBytesAllocated();last_ingress=gpu.ingress_lists;
            for(auto& ticks:native_clear_ticks)ticks.store(0,std::memory_order_relaxed);(void)bootstrap::GameUiSession::TakeProfile();}   // the measurement splits start with the first window too
        if(presented%120==1){if(window_start.QuadPart){const double wall=static_cast<double>(now.QuadPart-window_start.QuadPart)/frequency.QuadPart;
                std::array<double,3> ms{};std::array<uint64_t,3> phase_n{};for(size_t k=0;k<3;++k){const auto n=native_phase_counts[k].exchange(0);phase_n[k]=static_cast<uint64_t>(n);const auto t=native_phase_ticks[k].exchange(0);
                    ms[k]=n?1000.0*static_cast<double>(t)/frequency.QuadPart/n:0.0;}
                const double rn=render_count?render_count:1.0;
                // Phase A: queue submissions and CPU waits per frame over the window, process private and local video memory.
                const uint64_t executes=gpu.executes,drains=gpu.drains,completes=gpu.completes,arena_bytes=UploadArenaBytesAllocated(),ingress=gpu.ingress_lists;const auto draw_counters=NativeModelDrawCounters();
                static std::array<uint64_t,4> last_split{};std::array<double,4> split{};
                for(size_t k=0;k<4;++k){split[k]=1000.0*static_cast<double>(draw_counters.prepare_ticks[k]-last_split[k])/frequency.QuadPart/rn;last_split[k]=draw_counters.prepare_ticks[k];}
                const auto memory=NativeMemoryMb();ReactToVideoMemory(memory,presented);
                char pacing[1664]{};sprintf_s(pacing,"{\"event\":\"native_present_pacing\",\"frame\":%lld,\"wall_s\":%.3f,\"fps\":%.2f,\"slowest_ms\":%.1f,\"content_draws\":%u,\"collect_ms\":%.2f,\"prepare_ms\":%.2f,\"render_ms\":%.2f,\"record_ms\":%.2f,\"gpu_wait_ms\":%.2f,\"present_ms\":%.2f,\"ui_produce_drain_build_wait_diag_ms\":[%.2f,%.2f,%.2f,%.2f,%.2f],\"prep_validate_resources_draw_ms\":[%.2f,%.2f,%.2f],\"queue_execute_drain_complete_per_frame\":[%.2f,%.2f,%.2f],\"ingress_lists_per_frame\":%.2f,\"memory_private_local_mb\":[%llu,%llu],\"local_budget_mb\":%llu,\"nonlocal_usage_budget_mb\":[%llu,%llu],\"arena_chunks_created\":%llu,\"arena_kb_per_frame\":%.1f,\"retirements_pending\":%zu,\"cpu_descriptors\":%zu,\"pso_created\":%llu,\"draws_global_legacy\":[%llu,%llu],\"model_prepare_programs_validate_pipeline_tables_ms\":[%.2f,%.2f,%.2f,%.2f],\"measure\":%s}\n",
                    presented,wall,120.0/wall,slowest*1000.0,receipt.content_draw_count,ms[0],ms[1],ms[2],record_sum/rn,gpu_sum/rn,present_sum/rn,
                    native_ui_admission_ms[0]/std::max(1u,native_ui_admissions),native_ui_admission_ms[1]/std::max(1u,native_ui_admissions),native_ui_admission_ms[2]/std::max(1u,native_ui_admissions),
                    native_ui_admission_ms[3]/std::max(1u,native_ui_admissions),native_ui_admission_ms[4]/std::max(1u,native_ui_admissions),
                    1000.0*native_prep_ticks[0].load()/frequency.QuadPart/rn,1000.0*native_prep_ticks[1].load()/frequency.QuadPart/rn,1000.0*native_prep_ticks[2].load()/frequency.QuadPart/rn,
                    (executes-last_executes)/120.0,(drains-last_drains)/120.0,(completes-last_completes)/120.0,(ingress-last_ingress)/120.0,
                    static_cast<unsigned long long>(memory.private_mb),static_cast<unsigned long long>(memory.local_mb),static_cast<unsigned long long>(memory.local_budget_mb),
                    static_cast<unsigned long long>(memory.nonlocal_mb),static_cast<unsigned long long>(memory.nonlocal_budget_mb),
                    static_cast<unsigned long long>(UploadArenaChunksCreated()),(arena_bytes-last_arena_bytes)/1024.0/120.0,gpu.PendingRetirements(),gpu.CpuDescriptorsInUse(),static_cast<unsigned long long>(draw_counters.pso_created),static_cast<unsigned long long>(draw_counters.global_table_draws),static_cast<unsigned long long>(draw_counters.legacy_table_draws),split[0],split[1],split[2],split[3],measure_run?"true":"false");Log(pacing);
                {char analysis_row[640]{};sprintf_s(analysis_row,"{\"event\":\"native_program_analysis\",\"frame\":%lld,\"analysed\":%llu,\"reused\":%llu,\"analysis_ms\":%.1f,\"texture_decodes\":%llu,\"texture_decodes_owner\":%llu,\"texture_container_mb\":%.1f,\"texture_decode_ms\":%.1f,\"texture_admit_ms\":%.1f,\"texture_admit_max_ms\":%.1f,\"gparam_publishes\":%llu,\"gparam_publishes_owner\":%llu,\"gparam_publish_ms\":%.1f,\"gparam_publish_max_ms\":%.1f}\n",static_cast<long long>(presented),
                    static_cast<unsigned long long>(native_program_analysis_misses.load()),static_cast<unsigned long long>(native_program_analysis_hits.load()),1000.0*double(native_program_analysis_ticks.load())/frequency.QuadPart,
                    static_cast<unsigned long long>(native_texture_decodes.load()),static_cast<unsigned long long>(native_texture_decodes_owner.load()),double(native_texture_decode_bytes.load())/1048576.0,1000.0*double(native_texture_decode_ticks.load())/frequency.QuadPart,1000.0*double(native_texture_admit_ticks.load())/frequency.QuadPart,1000.0*double(native_texture_admit_max.load())/frequency.QuadPart,
                    static_cast<unsigned long long>(native_gparam_publishes.load()),static_cast<unsigned long long>(native_gparam_publishes_owner.load()),1000.0*double(native_gparam_publish_ticks.load())/frequency.QuadPart,1000.0*double(native_gparam_publish_max.load())/frequency.QuadPart);Log(analysis_row);}
                // I-0100 tranche 7 (crossing-observe.flag): the layouts' programme sets built so far and their parts (01_state.inc).
                {static const bool programs_split=ModuleFlag(L"crossing-observe.flag");
                    if(programs_split){std::string parts;for(size_t k=0;k<native_programs_build_ticks.size();++k){char cell[24]{};
                            sprintf_s(cell,"%s%.1f",k?",":"",1000.0*double(native_programs_build_ticks[k].load(std::memory_order_relaxed))/frequency.QuadPart);parts+=cell;}
                        char head[192]{};sprintf_s(head,"{\"event\":\"native_programs_build\",\"frame\":%lld,\"built\":%llu,\"kept\":%llu,\"recipe_sets_taken\":%llu,\"parts_ms\":[",static_cast<long long>(presented),
                            static_cast<unsigned long long>(native_programs_builds.load()),static_cast<unsigned long long>(native_programs_kept.load()),static_cast<unsigned long long>(native_recipe_sets_taken.load()));
                        Log((std::string(head)+parts+"]}\n").c_str());}}
                // I-0074 point 3c: the chunked G-buffer recordings so far (frames, chunks, constant payloads the chunks copied).
                {const auto chunks=renderer.RecordChunks();char chunk_row[256]{};sprintf_s(chunk_row,"{\"event\":\"native_record_chunks\",\"frame\":%lld,\"frames\":%llu,\"chunks\":%llu,\"blocks\":%llu,\"prepares\":%llu,\"prepare_chunks\":%llu}\n",
                    static_cast<long long>(presented),static_cast<unsigned long long>(chunks.frames),static_cast<unsigned long long>(chunks.chunks),static_cast<unsigned long long>(chunks.blocks),
                    static_cast<unsigned long long>(native_prepare_chunked.load(std::memory_order_relaxed)),static_cast<unsigned long long>(native_prepare_chunks.load(std::memory_order_relaxed)));Log(chunk_row);}
                // D-0167: the UI capture split (GameUiSession::Profile), milliseconds per PrepareFrame call over this window.
                if(kprofile::enabled){const auto ui=bootstrap::GameUiSession::TakeProfile();if(ui.calls){const double per=1000.0/frequency.QuadPart/double(ui.calls);char ui_row[512]{};
                    sprintf_s(ui_row,"{\"event\":\"native_ui_split\",\"frame\":%lld,\"calls\":%llu,\"prepare_ms\":%.3f,\"display_ms\":%.3f,\"submit_ms\":%.3f,\"packet_ms\":%.3f,\"submissions\":%.1f,\"submit_shader_ms\":%.3f,\"submit_primitive_ms\":%.3f,\"image_kb\":%.1f,\"reused_image_kb\":%.1f}\n",
                        presented,static_cast<unsigned long long>(ui.calls),ui.frame_ticks*per,ui.display_ticks*per,ui.submit_ticks*per,ui.packet_ticks*per,double(ui.submissions)/double(ui.calls),ui.shader_ticks*per,ui.primitive_ticks*per,double(ui.image_bytes)/1024.0/double(ui.calls),double(ui.reused_image_bytes)/1024.0/double(ui.calls));Log(ui_row);}}
                if(kprofile::enabled){std::array<double,6> clear_ms{};for(size_t k=0;k<6;++k)clear_ms[k]=1000.0*double(native_clear_ticks[k].exchange(0))/frequency.QuadPart/120.0;
                    char clear_row[320]{};sprintf_s(clear_row,"{\"event\":\"native_clear_split\",\"frame\":%lld,\"scene_ms\":%.3f,\"views_ms\":%.3f,\"ui_ms\":%.3f,\"locked_ms\":%.3f,\"post_ms\":%.3f,\"finish_ms\":%.3f}\n",
                        presented,clear_ms[0],clear_ms[1],clear_ms[2],clear_ms[3],clear_ms[4],clear_ms[5]);Log(clear_row);}
                // K1: the collect split, self milliseconds per collect over this window, and the counts per collect.
                if(kprofile::enabled){static std::array<uint64_t,kprofile::Count> last_ticks{},last_calls{};static std::array<uint64_t,kprofile::CounterCount> last_counters{};
                    const double collects=phase_n[0]?double(phase_n[0]):1.0;std::string collect_row="{\"event\":\"native_collect_split\",\"frame\":"+std::to_string(presented)+",\"collects\":"+std::to_string(phase_n[0])+",\"collect_ms\":";
                    char cell[64]{};sprintf_s(cell,"%.2f",ms[0]);collect_row+=cell;collect_row+=",\"self_ms\":[";double sum{};
                    for(size_t k=0;k<kprofile::CollectCount;++k){const uint64_t t=kprofile::totals.ticks[k];const double v=1000.0*double(t-last_ticks[k])/frequency.QuadPart/collects;last_ticks[k]=t;sum+=v;
                        sprintf_s(cell,"%s%.2f",k?",":"",v);collect_row+=cell;}
                    collect_row+="],\"calls\":[";for(size_t k=0;k<kprofile::CollectCount;++k){const uint64_t c=kprofile::totals.calls[k];sprintf_s(cell,"%s%.1f",k?",":"",double(c-last_calls[k])/collects);last_calls[k]=c;collect_row+=cell;}
                    collect_row+="],\"counts\":[";for(size_t k=0;k<kprofile::PrepRows;++k){const uint64_t c=kprofile::totals.counters[k];sprintf_s(cell,"%s%.1f",k?",":"",double(c-last_counters[k])/collects);last_counters[k]=c;collect_row+=cell;}
                    sprintf_s(cell,"],\"split_sum_ms\":%.2f",sum);collect_row+=cell;
                    // C-0b: milliseconds per frame ending at each stamp (FrameBegin, UpdateEntry, UpdateExit, RenderStepEntry,
                    // UiPrepareEntry, UiPrepareExit, RenderStepExit, EndFrameEntry, ClearEntry, ClearExit) and stamps per frame.
                    {static std::array<uint64_t,kprofile::StampCount> last_stamp_ticks{},last_stamp_calls{};
                        const uint64_t frames_now=kprofile::stamp_calls[kprofile::FrameBegin],frames=frames_now-last_stamp_calls[kprofile::FrameBegin];
                        const double per=frames?double(frames):1.0;collect_row+=",\"stamp_ms\":[";
                        for(size_t k=0;k<kprofile::StampCount;++k){const uint64_t t=kprofile::stamp_ticks[k];
                            sprintf_s(cell,"%s%.2f",k?",":"",1000.0*double(t-last_stamp_ticks[k])/frequency.QuadPart/per);last_stamp_ticks[k]=t;collect_row+=cell;}
                        collect_row+="],\"stamp_calls\":[";
                        for(size_t k=0;k<kprofile::StampCount;++k){const uint64_t c=kprofile::stamp_calls[k];
                            sprintf_s(cell,"%s%.2f",k?",":"",double(c-last_stamp_calls[k])/per);last_stamp_calls[k]=c;collect_row+=cell;}
                        collect_row+="]";}
                    // C-5: admitted models of the main view per collect whose per-frame inputs were unchanged, changed or new.
                    {static std::array<uint64_t,kprofile::ReuseCount> last_reuse{};collect_row+=",\"reuse\":[";
                        for(size_t k=0;k<kprofile::ReuseCount;++k){const uint64_t r=kprofile::reuse[k];
                            sprintf_s(cell,"%s%.1f",k?",":"",double(r-last_reuse[k])/collects);last_reuse[k]=r;collect_row+=cell;}
                        collect_row+="]";}
                    collect_row+="}\n";Log(collect_row.c_str());
                    // PREPARE-REDUCTION cP0: the main view's prepare split, self milliseconds per prepare call over this window.
                    const double prepares=phase_n[1]?double(phase_n[1]):1.0;std::string prepare_row="{\"event\":\"native_prepare_split\",\"frame\":"+std::to_string(presented)+
                        ",\"prepares\":"+std::to_string(phase_n[1])+",\"prepare_ms\":";sprintf_s(cell,"%.2f",ms[1]);prepare_row+=cell;prepare_row+=",\"self_ms\":[";double prepare_sum{};
                    for(size_t k=kprofile::CollectCount;k<kprofile::Count;++k){const uint64_t t=kprofile::totals.ticks[k];const double v=1000.0*double(t-last_ticks[k])/frequency.QuadPart/prepares;
                        last_ticks[k]=t;prepare_sum+=v;sprintf_s(cell,"%s%.2f",k>kprofile::CollectCount?",":"",v);prepare_row+=cell;}
                    prepare_row+="],\"calls\":[";
                    for(size_t k=kprofile::CollectCount;k<kprofile::Count;++k){const uint64_t c=kprofile::totals.calls[k];sprintf_s(cell,"%s%.1f",k>kprofile::CollectCount?",":"",double(c-last_calls[k])/prepares);
                        last_calls[k]=c;prepare_row+=cell;}
                    // the prepare counters per prepare call (rows, draws, constant blocks and bytes, SRVs, palette checks and in range)
                    prepare_row+="],\"counts\":[";for(size_t k=kprofile::PrepRows;k<kprofile::CounterCount;++k){const uint64_t c=kprofile::totals.counters[k];
                        sprintf_s(cell,"%s%.1f",k>kprofile::PrepRows?",":"",double(c-last_counters[k])/prepares);last_counters[k]=c;prepare_row+=cell;}
                    sprintf_s(cell,"],\"split_sum_ms\":%.2f}\n",prepare_sum);prepare_row+=cell;Log(prepare_row.c_str());}
                // D3D12 phase D cD0 (D3D12-PHASE-D-IMPLEMENTATION-SHEET 9.1): per frame over the window, drains and completion
                // waits by site (other, main frame, offscreen view, movie, depth recreation, movie planes, startup, resize,
                // close, fixture), GPU objects created, depth recreations, lighting programme changes, dynamic stream
                // uploads and kilobytes; peaks of the window: constant and global descriptor chunks in use, retirements pending.
                {static std::array<uint64_t,WaitSiteCount> last_site_drains{},last_site_completes{};
                    static uint64_t last_resources{},last_heaps{},last_depth{},last_programmes{},last_stream_updates{},last_stream_bytes{};
                    std::string audit="{\"event\":\"native_frame_audit\",\"frame\":"+std::to_string(presented)+",\"drains_by_site\":[";char value[48]{};
                    for(size_t k=0;k<WaitSiteCount;++k){const uint64_t d=gpu_audit.drains[k];sprintf_s(value,"%s%.2f",k?",":"",(d-last_site_drains[k])/120.0);last_site_drains[k]=d;audit+=value;}
                    audit+="],\"completes_by_site\":[";
                    for(size_t k=0;k<WaitSiteCount;++k){const uint64_t c=gpu_audit.completes[k];sprintf_s(value,"%s%.2f",k?",":"",(c-last_site_completes[k])/120.0);last_site_completes[k]=c;audit+=value;}
                    const auto per=[&](const std::atomic<uint64_t>& counter,uint64_t& last,double scale=1.0){const uint64_t v=counter;const double r=(v-last)/120.0/scale;last=v;return r;};
                    const auto global_chunk_peak=TakeModelDrawGlobalChunkPeak();
                    sprintf_s(value,"],\"resources_created\":%.2f",per(gpu_audit.resources_created,last_resources));audit+=value;
                    sprintf_s(value,",\"heaps_created\":%.2f",per(gpu_audit.heaps_created,last_heaps));audit+=value;
                    // I-0072 (I-0113: no driver library): the pipeline store since start: pipelines compiled, list writes, memory hits,
                    // precompiled of the listed, failed, waits for a pipeline being built, direct creations, precompile ms, done, the frame
                    // requests' compilations (the precompile's excluded), the records aged out at open, then (I-0132) the roots renamed by
                    // content at open, the records they merged and the root objects returned for two contents; the list written on the
                    // store's own thread when it changed.
                    {const auto stats=Pipelines().Stats();
                        char cache[400]{};sprintf_s(cache,",\"pipeline_store\":[%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%u,%llu,%llu,%llu,%llu,%llu]",static_cast<unsigned long long>(stats.created),
                            static_cast<unsigned long long>(stats.saves),static_cast<unsigned long long>(stats.memory_hits),static_cast<unsigned long long>(stats.precompiled),
                            static_cast<unsigned long long>(stats.precompile_total),static_cast<unsigned long long>(stats.precompile_failed),static_cast<unsigned long long>(stats.waits),
                            static_cast<unsigned long long>(stats.direct),static_cast<unsigned long long>(stats.precompile_ms),stats.precompile_done?1u:0u,
                            static_cast<unsigned long long>(stats.frame_created),static_cast<unsigned long long>(stats.pruned),static_cast<unsigned long long>(stats.roots_renamed),
                            static_cast<unsigned long long>(stats.records_merged),static_cast<unsigned long long>(stats.root_aliases));audit+=cache;
                        if(stats.open && stats.unsaved && !native_process_terminating.load(std::memory_order_relaxed))Pipelines().SaveAsync();}   // the store's own thread, joined at Close
                    sprintf_s(value,",\"depth_recreations\":%.2f",per(gpu_audit.depth_recreations,last_depth));audit+=value;
                    sprintf_s(value,",\"lighting_programme_changes\":%.2f",per(gpu_audit.lighting_programme_changes,last_programmes));audit+=value;
                    sprintf_s(value,",\"dynamic_stream_updates\":%.2f",per(gpu_audit.dynamic_stream_updates,last_stream_updates));audit+=value;
                    sprintf_s(value,",\"dynamic_stream_kb\":%.1f",per(gpu_audit.dynamic_stream_bytes,last_stream_bytes,1024.0));audit+=value;
                    sprintf_s(value,",\"global_chunks_peak\":%llu",static_cast<unsigned long long>(global_chunk_peak));audit+=value;
                    sprintf_s(value,",\"global_bump_peak\":%llu",static_cast<unsigned long long>(TakeModelDrawGlobalBumpPeak()));audit+=value;   // C4-2: the heap's used extent
                    // Phase E E2: the shadowed frames, those with a mismatch and the tracked uses compared (since start); the first mismatch rows as they come.
                    {char e2[160]{};sprintf_s(e2,",\"graph_shadow\":[%llu,%llu,%llu,%llu]",static_cast<unsigned long long>(renderer.GraphShadowFrames()),
                        static_cast<unsigned long long>(renderer.GraphShadowMismatchFrames()),static_cast<unsigned long long>(renderer.GraphShadowUses()),
                        static_cast<unsigned long long>(renderer.GraphExecutedFrames()));audit+=e2;   // E3: and the frames the graph drove
                     {static uint64_t last_graph_barriers{},last_graph_calls{};const uint64_t barriers=renderer.GraphBarriers(),calls=renderer.GraphBarrierCalls();
                      sprintf_s(e2,",\"graph_barriers\":[%.2f,%.2f]",(barriers-last_graph_barriers)/120.0,(calls-last_graph_calls)/120.0);audit+=e2;   // per frame: transitions, calls
                      last_graph_barriers=barriers;last_graph_calls=calls;}
                     for(const auto& shadow_row:renderer.TakeGraphShadowRows())Log(shadow_row.c_str());}
                    // Phase E E0: hand-written transition calls per frame and the distinct sites seen; the committed target bytes.
                    {static uint64_t last_barriers{};char e0[192]{};sprintf_s(e0,",\"barrier_calls\":%.2f,\"barrier_sites\":%llu",per(gpu_audit.barrier_calls,last_barriers),
                        static_cast<unsigned long long>(gpu_audit.barrier_sites.load()));audit+=e0;
                    sprintf_s(e0,",\"target_mb\":[%.2f,%.2f],\"target_resources\":%llu,\"target_extents\":%llu,\"target_alignment\":%llu",gpu_audit.target_bytes_main.load()/1048576.0,
                        gpu_audit.target_bytes_views.load()/1048576.0,static_cast<unsigned long long>(gpu_audit.target_resources.load()),
                        static_cast<unsigned long long>(gpu_audit.target_extents.load()),static_cast<unsigned long long>(gpu_audit.target_alignment.load()));audit+=e0;}
                    sprintf_s(value,",\"slot_wait_ms\":%.3f",renderer.TakeSlotWaitMs()/120.0);audit+=value;   // cD4: milliseconds per frame in the slot wait
                    sprintf_s(value,",\"slot_release_ms\":%.3f",renderer.TakeSlotReleaseMs()/120.0);audit+=value;   // cD6: and in slot releases
                    sprintf_s(value,",\"release_thread_ms\":%.3f",renderer.TakeReleaseThreadMs()/120.0);audit+=value;   // I-0090: the release thread's share
                    sprintf_s(value,",\"waitable_wait_ms\":%.3f",renderer.TakeWaitableWaitMs()/120.0);audit+=value;   // cD8: frame-latency waits
                    sprintf_s(value,",\"present_call_ms\":%.3f",renderer.TakePresentCallMs()/120.0);audit+=value;   // cD8: the Present calls (W2)
                    sprintf_s(value,",\"waitable_timeouts\":%llu",static_cast<unsigned long long>(renderer.WaitableTimeouts()));audit+=value;
                    // R13 cR0/cR3: the owner thread's milliseconds per frame waiting in the render worker's post (render_ms
                    // is then the worker's job time), and the window messages its pumped wait dispatched.
                    if(render_worker){sprintf_s(value,",\"queue_wait_ms\":%.3f",render_worker->TakeWaitMs()/120.0);audit+=value;
                        {const auto jobs=native_worker_jobs.exchange(0);const auto ticks=native_worker_job_ticks.exchange(0);
                            sprintf_s(value,",\"worker_job_ms\":%.3f",jobs?1000.0*double(ticks)/double(frequency.QuadPart)/double(jobs):0.0);audit+=value;}
                        sprintf_s(value,",\"pumped_messages\":%llu",static_cast<unsigned long long>(render_worker->Dispatched()));audit+=value;}
                    // cD5 gate: dynamic stream buffers created in the window (0 in steady state after warm-up) and pooled now.
                    {static uint64_t last_stream_buffers{};const uint64_t created=stream_versions.Created();
                        sprintf_s(value,",\"stream_buffers_created\":%llu",static_cast<unsigned long long>(created-last_stream_buffers));audit+=value;
                        sprintf_s(value,",\"stream_buffers_free\":%zu",stream_versions.FreeBuffers());audit+=value;last_stream_buffers=created;}
                    sprintf_s(value,",\"retirements_pending_peak\":%llu}\n",static_cast<unsigned long long>(retirements_pending_peak.exchange(0)));audit+=value;
                    Log(audit.c_str());}
                last_executes=executes;last_drains=drains;last_completes=completes;last_arena_bytes=arena_bytes;last_ingress=ingress;
                // Phase A (R7 C3a): root shapes first seen in this window ({VS cbv, srv, sampler, PS cbv, srv, sampler} highest + 1).
                {static size_t logged{};const auto shapes=NativeModelRootShapes();
                    if(shapes.size()>logged){std::string shape_row="{\"event\":\"native_model_root_shapes\",\"count\":"+std::to_string(shapes.size())+",\"new\":[";
                        for(size_t s=logged;s<shapes.size();++s){shape_row+=(s>logged?",[":"[");
                            for(size_t k=0;k<6;++k)shape_row+=(k?",":"")+std::to_string(shapes[s][k]);shape_row+="]";}
                        shape_row+="]}\n";Log(shape_row.c_str());logged=shapes.size();}}
                for(auto& ticks:native_prep_ticks)ticks.store(0,std::memory_order_relaxed);
                native_ui_admission_ms={};native_ui_admissions=0;
                record_sum=gpu_sum=present_sum=0;render_count=0;}
            // native_frame_times: the engine thread's frame intervals; native_present_times (cR4): the intervals between presents.
            const auto summarise=[&](const std::vector<float>& samples,const char* event){if(samples.size()<60)return;std::vector<float> sorted(samples);std::sort(sorted.begin(),sorted.end());
                const auto at=[&](double q){return sorted[std::min(sorted.size()-1,size_t(q*double(sorted.size())))];};
                double mean{},deviation{};for(const auto v:sorted)mean+=v;mean/=double(sorted.size());for(const auto v:sorted)deviation+=(v-mean)*(v-mean);deviation=std::sqrt(deviation/double(sorted.size()));
                const auto over=[&](float limit){return std::count_if(sorted.begin(),sorted.end(),[&](float v){return v>limit;});};char times[360]{};
                sprintf_s(times,"{\"event\":\"%s\",\"frame\":%lld,\"frames\":%zu,\"content\":%s,\"ms_p50_p99_max\":[%.2f,%.2f,%.2f],\"ms_mean_stdev\":[%.2f,%.2f],\"over_17_25_34_ms\":[%lld,%lld,%lld],\"refresh_hz\":%u,\"tick_ms\":%llu}\n",
                    event,presented,sorted.size(),receipt.content_draw_count?"true":"false",at(.5),at(.99),sorted.back(),mean,deviation,static_cast<long long>(over(17.5f)),static_cast<long long>(over(25.f)),static_cast<long long>(over(34.f)),renderer.RefreshHz(),static_cast<unsigned long long>(GetTickCount64()));LogFrameTimes(times,samples);};
            summarise(intervals,"native_frame_times");summarise(renderer.TakePresentIntervals(),"native_present_times");
            // I-0097: what the display showed over the window (I-0095 §4: the former held count sampled late frames badly).
            LogDisplayRefreshes(renderer.TakeDisplayRefreshes(),"native",presented,renderer.RefreshHz());
            intervals.clear();
            window_start=now;slowest=0;}}
}
void InitializeNativeCommonImages() {
    {const std::lock_guard<std::mutex> hold(common_image_lock);
        if(common_images_closed || native_common_images)throw std::runtime_error("native common image initialization state");}
    const auto content=ds3content::ProduceModelDitherTextures();
    auto candidate=std::make_shared<NativeCommonImages>();
    for(size_t index=0;index<content.size();++index)
        candidate->dither[index]=textures.AdmitOwned(reinterpret_cast<uintptr_t>(&native_common_image_keys[index]),content[index]);
    {const auto fade=ds3content::ProduceModelFadeDitherTextures();
        for(size_t index=0;index<fade.size();++index)candidate->fade_dither[index]=textures.AdmitOwned(reinterpret_cast<uintptr_t>(&native_common_image_keys[36+index]),fade[index]);}
    candidate->absent_decal=textures.AdmitOwned(reinterpret_cast<uintptr_t>(&native_common_image_keys[16]),
        ds3content::ProduceCommonDecalTexture());
    {const auto system=ds3content::ProduceSystemTextures();
        for(size_t index=0;index<system.size();++index)candidate->system[index]={std::wstring(system[index].name.begin(),system[index].name.end()),
            textures.AdmitOwned(reinterpret_cast<uintptr_t>(&native_common_image_keys[17+index]),system[index].image)};}
    {ds3content::CpuTextureResource cube;cube.name="native-forward-neutral-ibl";cube.width=cube.height=1;cube.mip_levels=1;
        cube.array_slices=6;cube.cube=true;cube.format=DXGI_FORMAT_R8G8B8A8_UNORM;
        for(int face=0;face<6;++face){ds3content::CpuTextureSubresource plane;plane.width=plane.height=1;plane.row_bytes=4;plane.rows=1;
            plane.bytes.assign(4,0);cube.subresources.push_back(std::move(plane));}
        candidate->neutral_ibl=textures.AdmitOwned(reinterpret_cast<uintptr_t>(&native_common_image_keys[31]),cube);
        // RDEF strides of g_LightIndices, g_LightLookupBuffer, g_LightBuffer, g_SpotLightBuffer.
        constexpr uint32_t strides[4]{8,12,64,128};
        for(size_t index=0;index<4;++index)candidate->empty_light_buffers[index]=
            textures.AdmitStructuredZero(reinterpret_cast<uintptr_t>(&native_common_image_keys[32+index]),strides[index],1);}
    const std::lock_guard<std::mutex> hold(common_image_lock);
    if(common_images_closed || native_common_images)throw std::runtime_error("native common image publication state");
    native_common_images=std::move(candidate);
    Log("{\"event\":\"native_common_dither_images_owned\",\"textures\":16,\"fade_textures\":2,\"actual_cpu_producer\":true,\"legacy_gpu_slots\":false,\"ordinary_phase_policy\":0,\"original_temporal_phase_parity\":false}\n");
    Log("{\"event\":\"native_common_decal_image_owned\",\"textures\":1,\"actual_cpu_producer\":true,\"legacy_gpu_slots\":false,\"selection_requires_current_instance_choice\":true}\n");
}
std::shared_ptr<const NativeCommonImages> AcquireNativeCommonImages() {
    const std::lock_guard<std::mutex> hold(common_image_lock);return common_images_closed?nullptr:native_common_images;
}
void CloseNativeCommonImages() {
    const std::lock_guard<std::mutex> hold(common_image_lock);common_images_closed=true;native_common_images.reset();
    for(auto& key:native_common_image_keys)textures.Retire(reinterpret_cast<uintptr_t>(&key));
}
std::atomic<bool> precompile_sound_stop{};   // I-0132 R12h's third tranche: the page's mixer thread stops without its fade (16b)
void Close() {
    precompile_sound_stop=true;   // I-0132 R12h (its second tranche's reviews' L8, L3; its third's mixer): the precompile page's sound stopped first, no lock held
    if(opened) {
        // R13 cR3 (sheet 5.3): the render worker is quiesced and stopped before any native lock: its last job is waited for,
        // a fault it kept ends here (EndFrame's single exit), the last outcome is checked and logged, then the worker
        // thread is joined; everything below runs on the owner thread alone, as before cR3.
        if(render_worker && render_worker->Running()){
            render_worker->Quiesce();render_worker->RethrowFault();
            if(const auto previous=std::exchange(pending_frame_outcome,{})){std::lock_guard<std::mutex> submission(gpu_submissions);FinishNativeFrame(*previous);}
            render_worker->Stop();Log("{\"event\":\"native_render_thread_stopped\"}\n");}
        if(ui_display_worker && ui_display_worker->Running()){ui_display_worker->Wait();ui_display_worker->Stop();native_ui_display=NativeUiDisplay{};}   // I-0088 P1
        if(collect_side_worker && collect_side_worker->Running()){collect_side_worker->Wait();collect_side_worker->Stop();}   // I-0088 P2
        if(collect_rays_worker && collect_rays_worker->Running()){try{collect_rays_worker->Wait();}catch(...){}collect_rays_worker->Stop();}   // I-0132 R11g
        // Stop admission in the real CPU dispatcher before retiring its HAL
        // and loader. An executing UI step may still call them after a frame
        // cutoff; let that actual scope return instead of freeing underneath.
        if(active_ui_dispatches.load(std::memory_order_acquire)){
            static bool reported{};if(!reported){reported=true;
                Log("{\"event\":\"native_close_deferred_cpu_ui_job\",\"session_retained\":true}\n");}
            return;
        }
        {const std::lock_guard<std::mutex> lock(ui_sessions_lock);ui_sessions.clear();}
        {const std::lock_guard<std::mutex> lock(sfx_asset_lock);sfx_assets.clear();}
        {const std::lock_guard<std::recursive_mutex> lock(scene_view_cpu_lock);native_stage_textures={};native_snow_textures_last.reset();native_region_snapshot_last.reset();ClearNativeRegionMemos();}
        CloseNativeEnvRegions();
        ResetNativeSelectedScene();
        CloseNativeKeptDraws();   // C4-2: the kept draws released before the device closes
        CloseNativeCommonImages();
        native_sampler_bank.Withdraw();
        {const std::lock_guard hold(native_options_lock);native_options.reset();}
        CloseNativeSimulationTime();
        CloseNativeModelStreams();
        CloseNativeClothStreams();
        CloseNativeFaceImages();
        // I-0081: the storage admissions create their buffer with no registry lock: the registry is closed first, then the ones
        // in flight finish (their copies recorded and committed, or dropped at their insertion) before the drain.
        {std::lock_guard<std::shared_mutex> hold(model_storage_lock);model_storage_closed=true;}
        while(model_storage_admitting.load(std::memory_order_acquire))SwitchToThread();
        {std::lock_guard<std::mutex> submission(gpu_submissions);
            std::lock_guard<std::shared_mutex> hold(model_storage_lock);gpu.Drain(WaitClose);
            // CPU consumers can still release their genuine references later.
            // Withdraw native residency without freeing their CPU prefixes.
            for(auto& [receiver,storage]:model_storage){(void)receiver;storage->resource.Reset();storage->blas.reset();}
            native_rt_blas.Close();CloseNativeRtSkinned();CloseNativeRtMaterials();   // I-0117 (R2 T0, T2, T3): after the drain
            native_memory_ballast.clear();}   // I-0084 step review 6: before the device closes, never from the module's unload
        StopNativeWarm();   // I-0132 R12d: the warm-up joined before the store closes (the final review's L3: before the submissions' lock)
        textures.Close();std::lock_guard<std::mutex> submission(gpu_submissions);
        {const auto before=Pipelines().Stats().saves;Pipelines().Close();const auto stats=Pipelines().Stats();   // I-0072: the precompile cancelled, the list written before the device goes
            char row[600]{};sprintf_s(row,"{\"event\":\"native_pipeline_list_close\",\"saved\":%s,\"created\":%llu,\"saves\":%llu,\"records\":%llu,\"list_bytes\":%llu,\"memory_hits\":%llu,\"precompiled\":%llu,\"precompile_total\":%llu,\"precompile_ms\":%llu,\"waits\":%llu,\"direct\":%llu,\"frame_created\":%llu,\"precompile_failed\":%llu,\"roots_renamed\":%llu,\"records_merged\":%llu,\"root_aliases\":%llu}\n",
                stats.saves>before?"true":"false",static_cast<unsigned long long>(stats.created),static_cast<unsigned long long>(stats.saves),static_cast<unsigned long long>(stats.records),
                static_cast<unsigned long long>(stats.list_bytes),static_cast<unsigned long long>(stats.memory_hits),static_cast<unsigned long long>(stats.precompiled),
                static_cast<unsigned long long>(stats.precompile_total),static_cast<unsigned long long>(stats.precompile_ms),static_cast<unsigned long long>(stats.waits),
                static_cast<unsigned long long>(stats.direct),static_cast<unsigned long long>(stats.frame_created),
                static_cast<unsigned long long>(stats.precompile_failed),static_cast<unsigned long long>(stats.roots_renamed),static_cast<unsigned long long>(stats.records_merged),
                static_cast<unsigned long long>(stats.root_aliases));Log(row);}
        renderer.Close();stream_versions.Reset();opened=false;   // cD5: the free stream buffers after the queue drained
        // cD6: the frames that were still in flight are released by the close; their debug-layer errors are reported here.
        {uint32_t errors{};for(const auto& completed:renderer.TakeCompleted())errors+=completed.debug_errors;
            char row[96]{};sprintf_s(row,"{\"event\":\"native_close_frames\",\"debug_errors\":%u}\n",errors);Log(row);}
    }
    InterlockedExchange(&control->closed,1);Log("{\"event\":\"native_closed\"}\n");
    // ROADMAP workstream 4 step 5 (I-0137): Close runs only at the controller's stop (a bounded test run; play never sets it), on the
    // engine's own thread (i66-native-i137-t4-charmake: the window thread), which would otherwise run the menus' next frame on the
    // withdrawn Scaleform HAL and freed UI images (E602F0's null HAL, I-0113 tranche 31 and I-0132 R12g's RCA; then E5B1F0 on a
    // released image). The thread waits for the controller's TerminateProcess, which follows `closed` within one 20 ms poll
    // (only game_bootstrap_ctl.cpp sets `stop`); the 15 s bound only keeps the wait finite.
    if(control->stop)for(const auto until=GetTickCount64()+15000;GetTickCount64()<until;)Sleep(100);
}
void FreeModelPrefix(uintptr_t receiver,uintptr_t heap) {
    if(native_process_terminating.load(std::memory_order_acquire))return;
    uintptr_t table{},free{};
    if(!Read(heap,&table,8) || !Read(table+0x68,&free,8) || !free)
        throw std::runtime_error("native model CPU heap release");
    reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t)>(free)(heap,receiver);
}
}  // namespace
