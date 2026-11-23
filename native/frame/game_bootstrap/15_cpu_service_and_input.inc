// The CPU task service, the fault handler, UI and input event witnesses, world layer witnesses, default-bound textures and
// BeginFrame. Part of game_bootstrap.cpp's single translation unit (the order is there): it relies on every earlier part
// (14_scene_step_and_storage.inc before it).
#pragma once
#include "14_scene_step_and_storage.inc"
namespace {
void RegisterCpuService(void* step) {
    // D53EE0 and its base construct the original CPU task service, timing
    // controller and fake-load controller. Its +C8 render-ready flag stays 0.
    // The original D3D11 bootstrap D56E90 is never invoked here.
    const auto allocate=reinterpret_cast<void*(__fastcall*)(size_t,size_t,void*)>(image+0x1777A00);
    const auto construct=reinterpret_cast<void*(__fastcall*)(void*)>(image+0xD53EE0);
    const auto register_service=reinterpret_cast<void(__fastcall*)(void*,unsigned,int,void*)>(image+0xF0FA50);
    uintptr_t registry{},existing{},allocator{},content_allocator{},temporary_allocator{};
    const auto receiver=reinterpret_cast<uintptr_t>(step);
    if(!Read(image+0x489F348,&registry,8) || !registry ||
       !Read(image+0x479C258,&allocator,8) || !allocator ||
       !Read(image+0x479C270,&content_allocator,8) || !content_allocator ||
       !Read(image+0x479C278,&temporary_allocator,8) ||
       !Read(receiver+0x140,&existing,8) || existing) Refuse("cpu_registry_not_pristine");
    // D56E90's descriptor and B5CD0's initial assignments publish CPU
    // allocators alongside GPU initialization. Preserve these genuine heap
    // services without constructing the lower D3D11 renderer.
    const uintptr_t cpu_heaps[][2]{{0x4961B80,content_allocator},
        {0x4961B78,temporary_allocator?temporary_allocator:content_allocator},
        {0x4961B70,content_allocator},{0x4961B68,content_allocator},{0x4961B58,content_allocator}};
    for(const auto& heap:cpu_heaps) {
        uintptr_t prior{};
        if(!Read(image+heap[0],&prior,8) || (prior && prior!=heap[1])) Refuse("cpu_heap_global_conflict");
        *reinterpret_cast<uintptr_t*>(image+heap[0])=heap[1];
    }
    Log("{\"event\":\"cpu_content_heaps_published\",\"original_gpu_created\":false}\n");
    // D59D40.asm: R8 is explicitly loaded from 14479C258. Ghidra's
    // recovered C prototype omits this required allocator argument.
    void* storage=allocate(0xD0,8,reinterpret_cast<void*>(allocator));
    if(!storage) Refuse("cpu_service_allocation_failed");
    void* service=construct(storage);
    BYTE ready{};
    if(service!=storage || !Read(reinterpret_cast<uintptr_t>(service)+0xC8,&ready,1) || ready)
        Refuse("cpu_service_constructor_mismatch");
    *reinterpret_cast<void**>(receiver+0x140)=service;
    register_service(reinterpret_cast<void*>(registry),0x58,0,service);
    cpu_registered=true;
    // This acknowledges completed native startup and genuine CPU registration,
    // not resource readiness. Later original CPU loading transactions may run.
    reinterpret_cast<Phase>(image+0xD5B250)(step);
    *reinterpret_cast<BYTE*>(receiver+0x58)=1;
    Log("{\"event\":\"cpu_service_registered\",\"key\":88,\"render_ready\":false,\"startup_completed\":true}\n");
    Observe(step,"native_start","leave");
}
// I-0082: the exceptions that end a process when nobody handles them (C++ exceptions, debugger notices and the like excluded).
constexpr DWORD heap_corruption=0xC0000374;   // ntstatus.h (a __fastfail, 0xC0000409, never reaches a handler)
bool FatalException(DWORD code) noexcept {
    switch(code){
    case EXCEPTION_ACCESS_VIOLATION:case EXCEPTION_ILLEGAL_INSTRUCTION:case EXCEPTION_PRIV_INSTRUCTION:case EXCEPTION_STACK_OVERFLOW:
    case EXCEPTION_INT_DIVIDE_BY_ZERO:case EXCEPTION_IN_PAGE_ERROR:case EXCEPTION_NONCONTINUABLE_EXCEPTION:case heap_corruption:return true;
    default:return false;}
}
// The fault rows (registers, code bytes, module) of an access violation or illegal instruction; the stack is the crash
// report's (I-0082). Out of line, so that Fault's own frame stays small for a stack overflow.
__declspec(noinline) void FaultRows(EXCEPTION_POINTERS* p) noexcept {
    if(p->ExceptionRecord->ExceptionCode==EXCEPTION_ACCESS_VIOLATION ||
       p->ExceptionRecord->ExceptionCode==EXCEPTION_ILLEGAL_INSTRUCTION) {
        InterlockedIncrement64(&control->faults);
        char row[512]{};
        // W4 FW-2: the data address of an access violation, and whether it lies in the effect system's device token page.
        const auto data=p->ExceptionRecord->ExceptionCode==EXCEPTION_ACCESS_VIOLATION && p->ExceptionRecord->NumberParameters>=2?
            p->ExceptionRecord->ExceptionInformation[1]:0;
        sprintf_s(row,"{\"event\":\"exception\",\"code\":%lu,\"rip\":%llu,\"image\":%llu,\"rcx\":%llu,\"rdx\":%llu,\"thread\":%lu,\"address\":%llu,\"effect_token\":%s}\n",
            p->ExceptionRecord->ExceptionCode,p->ContextRecord->Rip,static_cast<unsigned long long>(image),
            p->ContextRecord->Rcx,p->ContextRecord->Rdx,GetCurrentThreadId(),static_cast<unsigned long long>(data),
            effect_system::IsToken(data,native_effect_system)?"true":"false");
        Log(row);
        // 1172: the other integer registers and, for each, the vtable it points to (an object's class), if any.
        {const auto& c=*p->ContextRecord;const std::pair<const char*,DWORD64> regs[]{{"rax",c.Rax},{"rbx",c.Rbx},{"rsi",c.Rsi},{"rdi",c.Rdi},
            {"r8",c.R8},{"r9",c.R9},{"r12",c.R12},{"r13",c.R13},{"r14",c.R14},{"r15",c.R15},{"rbp",c.Rbp}};
            std::string cells;for(const auto& [label,value]:regs){uintptr_t table{};long long rva=-1;
                if(value>0x10000 && Read(value,&table,8) && table>=image && table<image+0x6000000)rva=static_cast<long long>(table-image);
                char cell[96]{};sprintf_s(cell,"%s\"%s\":[%llu,%lld]",cells.empty()?"":",",label,static_cast<unsigned long long>(value),rva);cells+=cell;}
            std::string registers="{\"event\":\"exception_registers\","+cells+"}\n";Log(registers.c_str());}
        // Live code bytes at the fault (encrypted-at-rest code is only readable here).
        if(BYTE code[64]{};p->ContextRecord->Rip>=image+0x1000 && Read(p->ContextRecord->Rip-32,code,sizeof(code))){
            char* at=row+sprintf_s(row,"{\"event\":\"exception_code\",\"from\":%llu,\"bytes\":\"",p->ContextRecord->Rip-32-image);
            for(BYTE b:code)at+=sprintf_s(at,4,"%02x",b);
            sprintf_s(at,8,"\"}\n");Log(row);}
        HMODULE module{}; char path[MAX_PATH]{};
        if(GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(p->ExceptionRecord->ExceptionAddress),&module) && GetModuleFileNameA(module,path,MAX_PATH)) {
            const char* basename=std::strrchr(path,'\\'); basename=basename?basename+1:path;
            sprintf_s(row,"{\"event\":\"exception_module\",\"name\":\"%s\",\"base\":%llu}\n",basename,
                static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(module))); Log(row);
        }
    }
}
LONG CALLBACK Fault(EXCEPTION_POINTERS* p) noexcept {
    const DWORD code=p->ExceptionRecord->ExceptionCode;
    // 1132: an unreadable field in Read() is an expected, locally handled outcome, not a fault. I-0082: so is any fault inside
    // the module's other guarded calls (they set the same flag) and on the sampler's thread (sample-profile.flag).
    if((guarded_copy_active || sampling::OnSamplerThread()) && FatalException(code))return EXCEPTION_CONTINUE_SEARCH;
    // I-0097 tranche 4: a read fault inside the engine buffer GuardedEngineRead reads (its own range only, final review 1).
    if(EngineReadFault(*p->ExceptionRecord))return EXCEPTION_CONTINUE_SEARCH;
    // I-0082 (DESIGN 3): a fatal-class exception asks for a minidump at once, at first chance, its context intact (seen here
    // before any handler; a native run has none handled, `faults` stays 0). On a stack overflow only the small request is
    // made (the controller writes the dump). Since I-0099's fix (DS3's network thread stopped at ~DLNRD::SocketManager, VERIFIED at
    // five quits, K-0484) a fault at a quit has no exception of its own: it is dumped at once like any other (STUDY step 4). The
    // Reference stays the unmodified original, whose quit still raises that heap fault (the fix is native only): there a heap
    // corruption is a row, not a dump (step review M1).
    if(code==EXCEPTION_STACK_OVERFLOW){
        if(PostCrashRequest(bootstrap::CrashKind::Exception,code,p,"stack overflow",S_OK)!=CrashPost::None)InterlockedExchange(&crash_request_busy,0);
        return EXCEPTION_CONTINUE_SEARCH;}
    if(code==heap_corruption && control && control->mode==bootstrap::Mode::Reference){static volatile LONG rows{};
        if(InterlockedIncrement(&rows)<=4){char row[120]{};sprintf_s(row,"{\"event\":\"reference_heap_corruption\",\"thread\":%lu}\n",GetCurrentThreadId());Log(row);}
        return EXCEPTION_CONTINUE_SEARCH;}
    if(FatalException(code)){
        RequestCrashDump(bootstrap::CrashKind::Exception,code,p,code==EXCEPTION_ACCESS_VIOLATION?"access violation":code==heap_corruption?"heap corruption":"fatal exception");}
    if(code==EXCEPTION_ACCESS_VIOLATION || code==EXCEPTION_ILLEGAL_INSTRUCTION)FaultRows(p);
    return EXCEPTION_CONTINUE_SEARCH;
}
// I-0099 (FIX-PLAN §2): at a quit, DS3's singleton list destroys DLNRD::SocketManager (1419853A0) without its stop
// (SocketChannelManager::Stop, 14198E1E0, which only DLNRD's finalize calls): the channel manager at +0x50 loses its mutex while
// its network thread still polls (+0x164 never cleared; ~DLThread joins only a thread never started), and the thread's next
// lock of the destroyed mutex is the exit heap fault 0xC0000374. At the destructor's entry, while that thread runs: Stop's first
// block (its +0x170 mutex, +0x164 cleared), then a join bounded at 2 s; then the original destructor, unchanged. The mutex is
// DS3's critical-section mutex (DLLifecycleAdapter<DLPlainLightMutex>, table 142766008, set by the channel manager's
// constructor 14198DE35 over the base's 1439F76B0; final review H1): accepted when its TryEnter and Leave slots (3 and 4) are
// 141799470 and 1417994A0 and its initialised byte +0x30 is set (cleared by its destruction, 1417993F0); taken through
// TryEnter for at most 500 ms (step review L1: Stop's slot 2 waits without a bound), released through Leave. Every quit writes
// one row, a skip included with its reason (I-0124: after the sound thread's stop and its own row, StopSoundThread below). Native
// mode only; no-exit-order-fix.flag leaves the original as it is (the three stops).
using SocketManagerDestroy=void(__fastcall*)(uintptr_t);
SocketManagerDestroy socket_manager_destroy_original{};
// I-0124: at the same quit, the sound thread. DS3's FMOD sound manager (DLMO::MOFmodSoundManager, the global 1449323B8, vtable
// 143A7AD48; its thread's handle at +0x10) runs until +0x3B1 is cleared and its queues drain; its stop (141A2DE20: two messages posted
// to its queue, +0x3B1 cleared, an infinite join, then its +0x6A0 object destroyed and freed) comes only from SprjSoundStep::
// STEP_Finish's teardown of SprjSoundImp, which a quit does not reach, so the thread still runs when the CRT's exit chain destroys the
// static heap its read queue frees into (14465ABA8, its atexit destructor 1427173B0 leaving a vtable of _purecall), and its next
// pass's pure call is the fault at 140F14C52 (the full control's full-1103-night, I-0114, I-0115). Here, inside that chain, DS3's own
// stop runs on a helper thread (its join has no bound) and the helper's end, so the whole stop, is awaited at most 2 s; the sound
// thread's exit releases FMOD into the heap. Only while the heap still lives (its live vtable 142990838: the step review's M2, the
// order of the atexit registrations being INFERRED) and the thread was started (DLThread's byte +0xC8). no-sound-stop.flag leaves this
// stop out (the network's kept). One row.
const char* StopSoundThread(ULONGLONG& waited) noexcept {
    const ULONGLONG began=GetTickCount64();waited=0;
    if(ModuleFlag(L"no-sound-stop.flag"))return "skipped_flag";
    uintptr_t manager{},table{},heap_table{};BYTE running{},started{};HANDLE handle{};
    if(!Read(image+0x49323B8,&manager,sizeof manager) || !manager)return "skipped_none";
    if(!Read(manager,&table,sizeof table))return "skipped_unreadable";
    if(table!=image+0x3A7AD48)return "skipped_type";   // final review L4: another or a destroyed object, told apart
    if(!Read(manager+0x3B1,&running,1) || !Read(manager+0xC8,&started,1) || !Read(manager+0x10,&handle,sizeof handle))return "skipped_unreadable";
    if(!running || !started || !handle)return "skipped_not_running";
    if(!Read(image+0x465ABA8,&heap_table,sizeof heap_table) || heap_table!=image+0x2990838)return "skipped_heap_destroyed";
    if(const DWORD state=WaitForSingleObject(handle,0);state!=WAIT_TIMEOUT)return state==WAIT_OBJECT_0?"skipped_ended":"skipped_bad_handle";
    const HANDLE stopper=CreateThread(nullptr,0,[](void* m) -> DWORD {reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0x1A2DE20)(reinterpret_cast<uintptr_t>(m));return 0;},
        reinterpret_cast<void*>(manager),0,nullptr);
    if(!stopper)return "no_helper";
    const DWORD wait=WaitForSingleObject(stopper,2000);CloseHandle(stopper);
    waited=GetTickCount64()-began;
    return wait==WAIT_OBJECT_0?"signalled":wait==WAIT_TIMEOUT?"timeout_helper_left":"failed";
}
// I-0137 tranche 6 (ROADMAP workstream 4 step 5; I-0113's g24i, the « exit fault at F14C52, not attributed »): the third thread of the
// same kind. FromSoftware's online client (the holder at the global 14478ECC0, its ClientLib::FdpClientImpl at [holder], vtable
// 143AA7428) owns at [client+0x60] its FdpSv::RPCSystemImpl (vtable 143AB0948, constructor 141B5C040), whose DLKR::DLRunnable base
// at +8 runs the thread loop 141B5F770: until the byte +0x431 is set it polls its TCP (+0x10) and UDP (+0x20) request systems and its
// task system (+0x30), waiting at most 100 ms (the event +0x438, or a request system's own wait). Its only stop is the class's
// slot 2 (141B5D6C0), reached from the game's finalizer (140EDE1F0 > 140556ED0 > 140C76AE0 > 140C81320 > the client's slot 2, the
// thunk 141B1F430 > 141B23BD0), which a quit does not reach: the thread polled on into the CRT's exit chain and its UDP update
// (1422B00E0 > 1422BA280) met a service already destroyed, the _purecall at 140F14C52 (READ with GhidraMCP, 2026-11-12; the
// fault's class INFERRED from its stack). Here, before the network's stop, only the thread part of 141B5D6C0: +0x431 set, the
// event +0x438 set (SetEvent on its handle +0x440, what 1417F1B60 does for a set), the thread awaited at most 2 s on its handle
// (DLThread +8, as 1417E8850 joins); nothing freed (the process ends), no call into DS3's objects (the step review's M1: its other
// wakes go through the request systems, which the exit chain may already have reached; the loop's waits are bounded at 100 ms,
// 1417E8850's microseconds). Only for a threaded RPC system (+0x430) whose thread still runs. no-fdp-stop.flag leaves it out. One
// row.
const char* StopFdpRpcThread(ULONGLONG& waited) noexcept {
    const ULONGLONG began=GetTickCount64();waited=0;
    if(ModuleFlag(L"no-fdp-stop.flag"))return "skipped_flag";
    uintptr_t holder{},client{},client_table{},rpc{},rpc_table{},thread{};BYTE threaded{},stopping{};HANDLE handle{},event{};
    if(!Read(image+0x478ECC0,&holder,sizeof holder) || !holder)return "skipped_none";
    if(!Read(holder,&client,sizeof client) || !client)return "skipped_none";
    if(!Read(client,&client_table,sizeof client_table) || client_table!=image+0x3AA7428)return "skipped_type";
    if(!Read(client+0x60,&rpc,sizeof rpc) || !rpc)return "skipped_none";
    if(!Read(rpc,&rpc_table,sizeof rpc_table) || rpc_table!=image+0x3AB0948)return "skipped_type";
    if(!Read(rpc+0x430,&threaded,1) || !Read(rpc+0x431,&stopping,1) || !Read(rpc+0x428,&thread,sizeof thread) ||
       !Read(rpc+0x440,&event,sizeof event))return "skipped_unreadable";
    if(!threaded || stopping || !thread)return "skipped_not_running";
    if(!Read(thread+8,&handle,sizeof handle) || !handle)return "skipped_unreadable";
    if(const DWORD state=WaitForSingleObject(handle,0);state!=WAIT_TIMEOUT)return state==WAIT_OBJECT_0?"skipped_ended":"skipped_bad_handle";
    *reinterpret_cast<volatile BYTE*>(rpc+0x431)=1;
    if(event)SetEvent(event);
    const DWORD wait=WaitForSingleObject(handle,2000);
    waited=GetTickCount64()-began;
    return wait==WAIT_OBJECT_0?"signalled":wait==WAIT_TIMEOUT?"timeout":"failed";
}
void __fastcall StopNetworkThreadThenDestroy(uintptr_t manager) noexcept {
    {ULONGLONG waited{};const char* sound=StopSoundThread(waited);char row[160]{};
        sprintf_s(row,"{\"event\":\"native_exit_sound_stop\",\"result\":\"%s\",\"waited_ms\":%llu}\n",sound,waited);Log(row);}   // I-0124
    {ULONGLONG waited{};const char* fdp=StopFdpRpcThread(waited);char row[160]{};
        sprintf_s(row,"{\"event\":\"native_exit_fdp_stop\",\"result\":\"%s\",\"waited_ms\":%llu}\n",fdp,waited);Log(row);}   // I-0137 tranche 6
    const uintptr_t channels=manager+0x50,thread=channels+0x98,mutex=channels+0x170;
    BYTE started{},initialised{};HANDLE handle{};uintptr_t table{},try_enter{},leave{};
    const char* result="signalled";const ULONGLONG began=GetTickCount64();
    if(!Read(thread+0xC0,&started,1) || !Read(thread+8,&handle,sizeof handle) || !Read(mutex,&table,sizeof table) || !Read(mutex+0x30,&initialised,1))result="skipped_unreadable";
    else if(!started || !handle)result="skipped_not_started";
    else if(!table || !Read(table+0x18,&try_enter,sizeof try_enter) || !Read(table+0x20,&leave,sizeof leave) ||
            try_enter!=image+0x1799470 || leave!=image+0x17994A0 || !initialised)result="skipped_mutex";
    else if(WaitForSingleObject(handle,0)!=WAIT_TIMEOUT)result="skipped_ended";
    else{bool locked{};
        while(!(locked=reinterpret_cast<int(__fastcall*)(uintptr_t)>(try_enter)(mutex)==0) && GetTickCount64()-began<500)Sleep(1);
        if(!locked)result="lock_timeout";
        else{*reinterpret_cast<volatile BYTE*>(channels+0x164)=0;reinterpret_cast<void(__fastcall*)(uintptr_t)>(leave)(mutex);
            const DWORD wait=WaitForSingleObject(handle,2000);result=wait==WAIT_OBJECT_0?"signalled":wait==WAIT_TIMEOUT?"timeout":"failed";}}
    char row[220]{};sprintf_s(row,"{\"event\":\"native_exit_network_stop\",\"result\":\"%s\",\"waited_ms\":%llu,\"thread\":%lu,\"owner_thread\":%s}\n",
        result,GetTickCount64()-began,GetCurrentThreadId(),GetCurrentThreadId()==owner_thread?"true":"false");Log(row);
    socket_manager_destroy_original(manager);
}
// Bounded diagnostic witnesses of genuine CPU event delivery. The actual
// calls, arguments and return values remain unchanged in both executions.
struct UiEventObservation {
    uintptr_t identity{},player{};
    uint64_t last_tick{};
    uint32_t first{},second{};
};
std::mutex ui_event_observation_lock;
std::array<UiEventObservation,32> ui_pointer_observations{},ui_eligibility_observations{},ui_condition_observations{},ui_movie_observations{},input_confirm_observations{},input_step_observations{};
UiEventObservation* UiObservation(std::array<UiEventObservation,32>& entries,uintptr_t identity) noexcept {
    for(auto& entry:entries)if(entry.identity==identity)return &entry;
    for(auto& entry:entries)if(!entry.identity){entry.identity=identity;return &entry;}
    return nullptr;
}
const char* UiObservationMode() noexcept {return control->mode==bootstrap::Mode::Reference?"original":"native";}
void ObserveUiPointerResult(uintptr_t context,uintptr_t output,uintptr_t caller) noexcept {
    if(control->stop)return;
    struct Pointer {int32_t x,y;uint32_t buttons;int16_t wheel;uint16_t pad;};Pointer pointer{};
    if(!Read(output,&pointer,sizeof(pointer)))return;
    const std::lock_guard<std::mutex> lock(ui_event_observation_lock);
    static uint32_t button_records{},position_records{};
    auto* state=UiObservation(ui_pointer_observations,context);if(!state)return;
    const auto tick=GetTickCount64();
    const bool buttons_changed=!state->last_tick || state->first!=pointer.buttons || state->second!=static_cast<uint16_t>(pointer.wheel);
    const bool periodic=!state->last_tick || tick-state->last_tick>=5000;
    if((buttons_changed && button_records<128) || (periodic && position_records<64)){
        if(buttons_changed)++button_records;else ++position_records;
        char row[512]{};sprintf_s(row,"{\"event\":\"ui_event_pointer\",\"mode\":\"%s\",\"tick\":%llu,\"context\":%llu,\"caller_rva\":%llu,\"x\":%d,\"y\":%d,\"buttons\":%u,\"wheel\":%d,\"position_validity_proved\":false,\"cpu_output_only\":true}\n",
            UiObservationMode(),tick,static_cast<unsigned long long>(context),static_cast<unsigned long long>(caller-image),pointer.x,pointer.y,pointer.buttons,static_cast<int>(pointer.wheel));Log(row);state->last_tick=tick;
    }
    state->first=pointer.buttons;state->second=static_cast<uint16_t>(pointer.wheel);
}
uintptr_t __fastcall ObserveOriginalUiPointer(uintptr_t context,uintptr_t output) noexcept {
    const auto result=ui_pointer_original(context,output);
    ObserveUiPointerResult(context,output,reinterpret_cast<uintptr_t>(_ReturnAddress()));return result;
}
void ObserveUiPredicate(std::array<UiEventObservation,32>& entries,const char* event,uintptr_t context,uint8_t result,uintptr_t caller) noexcept {
    if(control->stop)return;
    const std::lock_guard<std::mutex> lock(ui_event_observation_lock);
    static uint32_t records{};auto* state=UiObservation(entries,context);if(!state || records>=256)return;
    const auto tick=GetTickCount64();
    if(!state->last_tick || state->first!=result){
        ++records;char row[384]{};sprintf_s(row,"{\"event\":\"%s\",\"mode\":\"%s\",\"tick\":%llu,\"context\":%llu,\"caller_rva\":%llu,\"result\":%u,\"return_unchanged\":true}\n",
            event,UiObservationMode(),tick,static_cast<unsigned long long>(context),static_cast<unsigned long long>(caller-image),result);Log(row);state->last_tick=tick;
    }
    state->first=result;
}
uint8_t __fastcall ObserveUiInputEligibility(uintptr_t context) noexcept {
    const auto result=ui_input_eligible_original(context);
    ObserveUiPredicate(ui_eligibility_observations,"ui_event_input_eligibility",context,result,reinterpret_cast<uintptr_t>(_ReturnAddress()));
    return result;
}
uint8_t __fastcall ObserveUiCondition13(uintptr_t context) noexcept {
    const auto result=ui_condition13_original(context);
    ObserveUiPredicate(ui_condition_observations,"ui_event_condition13",context,result,reinterpret_cast<uintptr_t>(_ReturnAddress()));return result;
}
void __fastcall ObserveInputConfirmUpdate(uintptr_t action,uintptr_t frame,uintptr_t r8,uintptr_t r9) noexcept {
    // EBB5B0 is the genuine per-frame update of the action object whose +C4
    // is read by EBB510(code0x13) as condition13: binding query14233DE40(0x13),
    // then press/auto-repeat timer+C0. Arguments and effects stay unchanged.
    input_confirm_update_original(action,frame,r8,r9);
    if(control->stop)return;
    BYTE pressed{};float timer{};
    if(!Read(action+0xC4,&pressed,1) || !Read(action+0xC0,&timer,4))return;
    const std::lock_guard<std::mutex> lock(ui_event_observation_lock);
    static uint32_t records{},heartbeats{};auto* state=UiObservation(input_confirm_observations,action);if(!state)return;
    const auto tick=GetTickCount64();++state->second;
    const bool first=!state->last_tick,changed=!first && state->first!=pressed,periodic=!first && tick-state->last_tick>=5000;
    if(first || (changed && records<128) || (periodic && heartbeats<64)){
        if(changed)++records;else if(periodic)++heartbeats;
        char frames[256]{};
        if(first){std::array<void*,10> trace{};const auto count=RtlCaptureStackBackTrace(1,static_cast<DWORD>(trace.size()),trace.data(),nullptr);
            size_t used=0;for(USHORT n=0;n<count && used+24<sizeof(frames);++n){const auto address=reinterpret_cast<uintptr_t>(trace[n]);
                used+=static_cast<size_t>(sprintf_s(frames+used,sizeof(frames)-used,"%s%llu",n?",":"",static_cast<unsigned long long>(address>=image?address-image:address)));}}
        char row[640]{};sprintf_s(row,"{\"event\":\"input_confirm_update\",\"mode\":\"%s\",\"tick\":%llu,\"action\":%llu,\"kind\":\"%s\",\"pressed_c4\":%u,\"timer_c0\":%.9g,\"updates\":%u,\"first_stack_rva\":[%s],\"arguments_unchanged\":true}\n",
            UiObservationMode(),tick,static_cast<unsigned long long>(action),first?"first":changed?"transition":"heartbeat",pressed,static_cast<double>(timer),state->second,frames);Log(row);
        state->last_tick=tick;
    }
    state->first=pressed;
}
// World layer insertions (980-981): dynamic tree layer C89A0 and flat layer
// C24E0 receive (layer, scene-node interface). Pass-through witness of which
// object classes reach the world in each mode and from which callers; the
// native collector walks these layers (SELECTED-VIEW-CPU-700).
using LayerInsert=uintptr_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t);
LayerInsert dynamic_layer_insert_original{},flat_layer_insert_original{};
std::mutex layer_insert_lock;
std::map<std::tuple<uint64_t,uint64_t,uint64_t>,uint64_t> layer_insert_kinds;
uint64_t layer_inserts{};
void ObserveLayerInsert(const char* layer_kind,uintptr_t layer,uintptr_t node,uintptr_t caller) noexcept {
    uintptr_t layer_table{},node_table{};
    Read(layer,&layer_table,8);if(node)Read(node,&node_table,8);
    const auto rva=[](uintptr_t a){return static_cast<uint64_t>(a>=image && a<image+0x6000000?a-image:a);};
    const std::lock_guard hold(layer_insert_lock);++layer_inserts;
    auto& count=layer_insert_kinds[{rva(node_table),0,rva(caller)}];
    if(++count==1 && layer_insert_kinds.size()<=160){char row[384]{};sprintf_s(row,
        "{\"event\":\"world_layer_insert_kind\",\"mode\":\"%s\",\"layer\":\"%s\",\"layer_table_rva\":%llu,\"node_table_rva\":%llu,\"caller_rva\":%llu,\"total\":%llu}\n",
        UiObservationMode(),layer_kind,rva(layer_table),rva(node_table),rva(caller),layer_inserts);Log(row);}
    if(layer_inserts%2000==0){char row[160]{};sprintf_s(row,"{\"event\":\"world_layer_inserts\",\"total\":%llu,\"kinds\":%zu}\n",layer_inserts,layer_insert_kinds.size());Log(row);}
}
uintptr_t __fastcall ObserveDynamicLayerInsert(uintptr_t layer,uintptr_t node,uintptr_t r8,uintptr_t r9) noexcept {
    ObserveLayerInsert("dynamic",layer,node,reinterpret_cast<uintptr_t>(_ReturnAddress()));
    return dynamic_layer_insert_original(layer,node,r8,r9);
}
uintptr_t __fastcall ObserveFlatLayerInsert(uintptr_t layer,uintptr_t node,uintptr_t r8,uintptr_t r9) noexcept {
    ObserveLayerInsert("flat",layer,node,reinterpret_cast<uintptr_t>(_ReturnAddress()));
    return flat_layer_insert_original(layer,node,r8,r9);
}
// 1037 witness: D514C0 attaches a texture (or, when null, the default holder
// 4792EC0+60) to a named texture record (name +10, holder +60). Pass-through.
using TextureBind=void(__fastcall*)(uintptr_t,uintptr_t);
TextureBind texture_bind_original{};
// 1115: a record the loader binds with no texture takes the default holder 4792EC0
// (SYSTEX_DUMMY, D4A7D0/D4A910), whose texture is GX system entry 0 B580(owner,0) =
// SYSTEX_WHITE. Natively that registry is absent: remember the names so material
// lookups resolve them to the native SYSTEX_WHITE (e.g. SYSTEX_DummyVector, dummy128).
std::mutex default_bound_lock;
std::unordered_set<std::wstring> default_bound_textures;
bool NativeDefaultBoundTextureFolded(const std::wstring& folded) {const std::lock_guard hold(default_bound_lock);return default_bound_textures.contains(folded);}
void __fastcall ObserveTextureBind(uintptr_t record,uintptr_t texture) noexcept {
    texture_bind_original(record,texture);
    if(control->stop)return;
    static LONG64 total{},absent{},map_real{},map_absent{};
    InterlockedIncrement64(&total);if(!texture)InterlockedIncrement64(&absent);
    std::wstring name;uint64_t length{},capacity{};uintptr_t chars=record+0x10;
    if(Read(record+0x20,&length,8) && Read(record+0x28,&capacity,8) && length && length<512 && capacity>=length &&
       (capacity<=7 || Read(record+0x10,&chars,8))){name.resize(static_cast<size_t>(length));Read(chars,name.data(),name.size()*2);}
    if(!texture && !name.empty())try{std::wstring key=name;for(auto& ch:key)ch=static_cast<wchar_t>(towlower(ch));
        const std::lock_guard hold(default_bound_lock);default_bound_textures.insert(std::move(key));}catch(...){}
    const bool map=name.find(L"m40_")!=std::wstring::npos || name.find(L"/map/")!=std::wstring::npos;
    if(map)InterlockedIncrement64(texture?&map_real:&map_absent);
    static LONG64 samples{};
    if((map || !texture) && InterlockedIncrement64(&samples)<=24){uintptr_t fallback{},fallback_holder{};
        Read(image+0x4792EC0,&fallback,8);if(fallback)Read(fallback+0x60,&fallback_holder,8);std::string text;
        for(const auto ch:name)text.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'/');
        char row[512]{};sprintf_s(row,"{\"event\":\"native_texture_bind\",\"texture\":%s,\"default_registry\":%s,\"default_holder\":%s,\"total\":%lld,\"absent\":%lld,\"map_real\":%lld,\"map_absent\":%lld,\"name\":\"%.240s\"}\n",
            texture?"true":"false",fallback?"true":"false",fallback_holder?"true":"false",total,absent,map_real,map_absent,text.c_str());Log(row);}
    if(total%1000==0){char row[256]{};sprintf_s(row,"{\"event\":\"native_texture_bind_totals\",\"total\":%lld,\"absent\":%lld,\"map_real\":%lld,\"map_absent\":%lld}\n",
        total,absent,map_real,map_absent);Log(row);}
}
void __fastcall ObserveInputStepRun(uintptr_t step,uintptr_t frame,uintptr_t r8,uintptr_t r9) noexcept {
    // EB5B40 runs a CPU step state machine: state+54, state table+10 (16-byte
    // entries), virtual20 suspension test. Reference894/Native893 share its call
    // site on the confirm update's stack; observe which steps stop advancing.
    uint32_t before{};Read(step+0x54,&before,4);
    input_step_run_original(step,frame,r8,r9);
    if(control->stop)return;
    uintptr_t table{},vtable{},states{},handler{};uint32_t after{};
    if(!Read(step,&vtable,8) || !Read(step+0x10,&states,8) || !Read(step+0x54,&after,4))return;
    if(after<32)Read(states+uintptr_t(after)*0x10,&handler,8);(void)table;
    const std::lock_guard<std::mutex> lock(ui_event_observation_lock);
    static uint32_t records{},heartbeats{};auto* state=UiObservation(input_step_observations,step);if(!state)return;
    const auto tick=GetTickCount64();++state->second;
    const bool first=!state->last_tick,changed=!first && state->first!=after,periodic=!first && tick-state->last_tick>=10000;
    if(first || (changed && records<256) || (periodic && heartbeats<400)){
        if(changed)++records;else if(periodic)++heartbeats;
        char row[480]{};sprintf_s(row,"{\"event\":\"input_step_run\",\"mode\":\"%s\",\"tick\":%llu,\"step\":%llu,\"vtable_rva\":%llu,\"kind\":\"%s\",\"state_before\":%u,\"state_after\":%u,\"handler_rva\":%llu,\"runs\":%u,\"arguments_unchanged\":true}\n",
            UiObservationMode(),tick,static_cast<unsigned long long>(step),static_cast<unsigned long long>(vtable>=image?vtable-image:vtable),
            first?"first":changed?"transition":"heartbeat",before,after,static_cast<unsigned long long>(handler>=image?handler-image:handler),state->second);Log(row);
        state->last_tick=tick;
    }
    state->first=after;
}
// I-0132 R12e: the title screen's state for the overlay's « Rekindled » title (16b): the last tick the title's logo movie advanced
// (05_001_Title_Logo), and the last one of the movies that hide it (the loading screens, character creation), dim it (the title's
// pages, the Options) or open a message box over it (below); a film's frames cover it too (16 NativeMoviePlayerRender). The name read at each
// advance (a freed movie's address can come back as another movie).
std::atomic<uint64_t> rekindled_title_logo_tick{},rekindled_title_cover_tick{},rekindled_title_dim_tick{},rekindled_title_notice_tick{};
void NoteRekindledTitle(uintptr_t name) noexcept {
    std::array<char16_t,24> text{};if(!name || !Read(name,text.data(),sizeof(text)))return;
    size_t n=0;while(n<text.size() && text[n])++n;const std::u16string_view view(text.data(),n);
    const auto starts=[&](std::u16string_view prefix){return view.substr(0,prefix.size())==prefix;};
    if(view==u"05_001_Title_Logo")rekindled_title_logo_tick.store(GetTickCount64(),std::memory_order_relaxed);
    // The final review's M2: every title page but the title's own two (05_000, 05_001): Load Game, New Game's brightness and controls
    // (05_031, 05_032), ...; the Options and the loading screens (02_*); character creation (04_*). Not the message boxes (Thomas,
    // 2026-11-12 02:15: the title kept after a button, the start notices over it; they leave the logo shown); formerly covers too
    // (01_01*, the last step review's L6).
    // Thomas (2026-11-12 02:46: « il doit définitivement remplacer l'ancien écran titre »): the title's pages (05_0*, the Options 02_0*)
    // only dim it, as DS3 dims its logo behind them; the loading screens (02_9*) and character creation (04_*) hide it.
    else if(starts(u"01_01"))rekindled_title_notice_tick.store(GetTickCount64(),std::memory_order_relaxed);   // a message box (the Fable review's M2)
    else if(starts(u"02_9") || starts(u"04_"))rekindled_title_cover_tick.store(GetTickCount64(),std::memory_order_relaxed);
    else if((starts(u"05_0") && !starts(u"05_000_")) || starts(u"02_"))rekindled_title_dim_tick.store(GetTickCount64(),std::memory_order_relaxed);}
// I-0132 R12h's third tranche: while the precompile page plays Thomas's music (16b page_fx), the game's BGM request FUN_14076B920(id)
// (I-0069 MENU-AUDIO-NATIVE §3: category 6 m<id>, the title dialog's m1) is held, its id kept; the first menu movie advance after the
// page lets it go from rekindled_overlay::Step, on the engine thread where the game makes these requests (r12h-mu-page2: thread 37552;
// its final review's M2). The BGM stop FUN_14076B9F0 drops a held id (its L1: a Continue right after the page). Both detours in native runs (22_setup).
std::atomic<bool> title_bgm_held{};std::atomic<int> title_bgm_deferred{-1};
using BgmPlay=void(__fastcall*)(int32_t);BgmPlay bgm_play_original{};
void __fastcall HoldTitleBgm(int32_t id) noexcept {
    if(title_bgm_held.load(std::memory_order_relaxed)){
        if(title_bgm_deferred.exchange(id)!=id){char row[96]{};sprintf_s(row,"{\"event\":\"native_title_bgm\",\"held\":%d,\"thread\":%lu}\n",id,GetCurrentThreadId());Log(row);}
        return;}
    bgm_play_original(id);}
using BgmStop=void(__fastcall*)();BgmStop bgm_stop_original{};
void(*title_crackle_stop)() noexcept{};   // I-0132 R12e: the title's crackle stops with the music (16b title_crackle sets it)
void __fastcall DropTitleBgm() noexcept {title_bgm_deferred.store(-1,std::memory_order_relaxed);bgm_stop_original();if(title_crackle_stop)title_crackle_stop();}
void ReleaseTitleBgm() noexcept {
    if(title_bgm_held.load(std::memory_order_relaxed) || !bgm_play_original)return;
    const int32_t id=title_bgm_deferred.exchange(-1);if(id<0)return;
    char row[96]{};sprintf_s(row,"{\"event\":\"native_title_bgm\",\"released\":%d,\"thread\":%lu}\n",id,GetCurrentThreadId());Log(row);bgm_play_original(id);}
void __fastcall ObserveUiMovieAdvance(uintptr_t movie,uintptr_t frame,uint32_t enabled,uint32_t viewport) noexcept {
    ui_movie_advance_original(movie,frame,enabled,viewport);
    if(control->stop)return;
    uintptr_t player{},name{};float delta{};uint32_t advance{},prepared{};
    if(!Read(movie+0x18,&player,8) || !Read(movie+0x28,&name,8) || !Read(frame+8,&delta,4) ||
       !Read(movie+0x88,&advance,4) || !Read(movie+0x8C,&prepared,4))return;
    NoteRekindledTitle(name);   // I-0132 R12e
    const std::lock_guard<std::mutex> lock(ui_event_observation_lock);
    // 1173: every movie's first advance is recorded (the periodic rows below stop at 256, before New Game).
    {static std::unordered_set<uintptr_t> seen;if(seen.size()<256 && seen.insert(movie).second){std::array<uint16_t,48> text{};std::string ascii;
        if(Read(name,text.data(),sizeof(text)))for(size_t n=0;n<text.size() && text[n];++n)ascii+=text[n]<128 && text[n]>=32 && text[n]!='"' && text[n]!='\\'?static_cast<char>(text[n]):'?';
        char row[256]{};sprintf_s(row,"{\"event\":\"ui_movie_first_advance\",\"mode\":\"%s\",\"tick\":%llu,\"movie\":%llu,\"name\":\"%s\"}\n",
            UiObservationMode(),static_cast<unsigned long long>(GetTickCount64()),static_cast<unsigned long long>(movie),ascii.c_str());Log(row);}}
    static uint32_t records{};auto* state=UiObservation(ui_movie_observations,movie);if(!state || records>=256)return;
    const auto tick=GetTickCount64();
    if(!state->last_tick || state->player!=player || state->first!=enabled || state->second!=viewport || tick-state->last_tick>=1000){
        ++records;std::array<uint16_t,48> text{};char hex[193]{};constexpr char digits[]="0123456789abcdef";
        if(Read(name,text.data(),sizeof(text)))for(size_t n=0;n<text.size() && text[n];++n)
            for(size_t digit=0;digit<4;++digit)hex[n*4+digit]=digits[(text[n]>>(12-digit*4))&15];
        char row[768]{};sprintf_s(row,"{\"event\":\"ui_event_movie_advance\",\"mode\":\"%s\",\"tick\":%llu,\"movie\":%llu,\"player\":%llu,\"name_field_u16hex\":\"%s\",\"frame_delta\":%.9g,\"enabled_argument\":%u,\"viewport_argument\":%u,\"field88\":%u,\"field8C\":%u,\"arguments_unchanged\":true}\n",
            UiObservationMode(),tick,static_cast<unsigned long long>(movie),static_cast<unsigned long long>(player),hex,static_cast<double>(delta),enabled,viewport,advance,prepared);Log(row);
        state->last_tick=tick;state->player=player;state->first=enabled;state->second=viewport;
    }
}
void __fastcall ObserveOriginalUiManagerDestroy(uintptr_t manager) noexcept {
    uintptr_t wrapper{},hal{},loader{};
    const bool fields=Read(manager+0x978,&wrapper,8) && Read(manager+0x980,&hal,8) && Read(manager+0x988,&loader,8);
    char row[384]{};sprintf_s(row,"{\"event\":\"ui_event_manager_destroy_begin\",\"tick\":%llu,\"manager\":%llu,\"fields_read\":%s,\"wrapper\":%llu,\"hal\":%llu,\"loader\":%llu,\"thread\":%lu,\"original_destructor_unchanged\":true}\n",
        GetTickCount64(),static_cast<unsigned long long>(manager),fields?"true":"false",static_cast<unsigned long long>(wrapper),static_cast<unsigned long long>(hal),static_cast<unsigned long long>(loader),GetCurrentThreadId());Log(row);
    ui_manager_destroy_original(manager);
    sprintf_s(row,"{\"event\":\"ui_event_manager_destroy_end\",\"tick\":%llu,\"manager\":%llu,\"original_destructor_unchanged\":true}\n",GetTickCount64(),static_cast<unsigned long long>(manager));Log(row);
}
// I-0114 tranche 1 (the unlocked frame rate, method B, D-0240; UNLOCK-READY §2): the requested rate DS3's limiter reads
// ([timing+0x354] frames a second, its enable +0x358, K-0019), from a flag while the logic's corrections are written (a rate
// above 60 is offered to Thomas only once they are): unlock-30, unlock-120, unlock-240 or unlock-unlimited.flag (a requested 1000,
// the frame its own bound). 0: DS3's own rate, nothing written. original-unlock.flag (I-0088, measurement) keeps its 360.
// Tranche 3 (the rate in the options): without a test flag, the settings' frame_rate (I-0132 R12b: rekindled-settings.ini, the play
// launcher's -FrameRate; frame_rate_option.h: 20 to 1000 or unlimited, 60 DS3's own); its start's source named in one row. I-0132 R12f:
// the settings' rate live (the page's frame rate row, at the next engine frame); a test flag stays fixed for the session.
const char* native_frame_rate_source{"default"};
float NativeRequestedRate() noexcept {
    static const float flag=[]{
        if(ModuleFlag(L"unlock-20.flag")){native_frame_rate_source="flag";return 20.f;}   // I-0113 gap 27's A/B: the corrections installed (k = 3 with unlock-fix-below-60.flag, else 1)
        if(ModuleFlag(L"unlock-30.flag")){native_frame_rate_source="flag";return 30.f;}
        if(ModuleFlag(L"unlock-120.flag")){native_frame_rate_source="flag";return 120.f;}
        if(ModuleFlag(L"unlock-240.flag")){native_frame_rate_source="flag";return 240.f;}
        if(ModuleFlag(L"unlock-unlimited.flag")){native_frame_rate_source="flag";return 1000.f;}
        const auto& file=NativeSettingsFile();
        if(file.Accepted("frame_rate"))native_frame_rate_source="file";
        else if(file.Refused("frame_rate"))native_frame_rate_source="file_unrecognised";
        return -1.f;}();
    if(flag>=0)return flag;
    if(native_rate_pinned)return native_rate_start;   // the corrections not installed (22_setup.inc): the start's rate kept
    return NativeSettingsCurrent().frame_rate;
}
// The logic's frame factor k = dt / (1/60), dt the one DS3's timeline hands its systems ([timing+0x268], the mean of the last three
// frame times, each at most 50 ms; CADENCE-REFLEX-READY §8.2), read once a frame in BeginFrame on the engine thread; exactly 1 when
// within 1e-4 of it, so every corrected site is the original's bits at 60 (UNLOCK-READY §0 point 2), and 1 under 60 frames/s too
// (logic_rate::FrameK, I-0113 gap 27: the game's own laws when it runs slower than 60). The corrections read it.
std::atomic<float> native_logic_k{1.f};   // written by the engine thread in BeginFrame (step review L6: read by the logger thread too)
// I-0114 tranche 2 (UNLOCK-READY §3, the locomotion first): the corrections of the logic's per-call updates, hooked with an unlock
// flag only. At k = 1 each detour calls the original (its bits at 60); otherwise it runs the original's own steps with the laws of
// native/world/logic_rate.h.
namespace unlock_fix {
// 1405C72B0 (LTF's site; GhidraMCP 2026-10-30, the disassembly read whole): the locomotion factor [this+0x25C]. Its steps, in the
// original's order: the mover [[[this+8]+0x1F90]+0x68] advanced by 1409DC090 with a time object {0x1427457D8, [[[this+8]+0x1F90]
// +0x90]+0x1C}; the character's virtual +0x1A8 (false: nothing more); 1405C5AC0's mode; the distance branch when the mode is 3 or
// [this+0x260] > [0x143D7EEE0], and [mover+0xA2] set: the distance from the mover's position +0x90 to 1409D9650's (a getter).
using LocomotionFn=void(__fastcall*)(uintptr_t);
LocomotionFn locomotion_original{};
void __fastcall Locomotion(uintptr_t self) noexcept {
    const float k=native_logic_k.load(std::memory_order_relaxed);
    if(k==1.f){locomotion_original(self);return;}
    const auto chr=*reinterpret_cast<uintptr_t*>(self+8);const auto owner=*reinterpret_cast<uintptr_t*>(chr+0x1F90);
    const auto mover=*reinterpret_cast<uintptr_t*>(owner+0x68);const auto times=*reinterpret_cast<uintptr_t*>(owner+0x90);
    struct alignas(16) TimeObject {const void* table;float dt;float pad;} time{reinterpret_cast<const void*>(image+0x27457D8),*reinterpret_cast<float*>(times+0x1C),0};
    reinterpret_cast<void(__fastcall*)(uintptr_t,TimeObject*)>(image+0x9DC090)(mover,&time);
    const auto actor=*reinterpret_cast<uintptr_t*>(self+8);const auto table=*reinterpret_cast<uintptr_t*>(actor);
    if(!reinterpret_cast<bool(__fastcall*)(uintptr_t)>(*reinterpret_cast<uintptr_t*>(table+0x1A8))(actor))return;
    const int mode=reinterpret_cast<int(__fastcall*)(uintptr_t)>(image+0x5C5AC0)(self);
    const auto constant=[](uint32_t rva){return *reinterpret_cast<const float*>(image+rva);};
    const logic_rate::LocomotionConstants c{constant(0x3D7EE10),constant(0x3D7EF84),constant(0x3D7EDB4),constant(0x3D7EDF0),constant(0x3D7EE18)};
    const bool branch=(mode==3 || constant(0x3D7EEE0)<*reinterpret_cast<float*>(self+0x260)) && *reinterpret_cast<uint8_t*>(mover+0xA2);
    float distance{};
    if(branch){const auto m=*reinterpret_cast<uintptr_t*>(*reinterpret_cast<uintptr_t*>(*reinterpret_cast<uintptr_t*>(self+8)+0x1F90)+0x68);
        const auto* now=reinterpret_cast<const float*>(m+0x90);alignas(16) float there[4]{};
        reinterpret_cast<float*(__fastcall*)(uintptr_t,float*)>(image+0x9D9650)(m,there);
        const float dx=there[0]-now[0],dy=there[1]-now[1],dz=there[2]-now[2];distance=std::sqrt(dx*dx+dz*dz+dy*dy);}
    auto& factor=*reinterpret_cast<float*>(self+0x25C);
    factor=logic_rate::LocomotionFactor(factor,branch,distance,k,c);
}
// 1405D0010 (turning; GhidraMCP 2026-10-30): out (RCX), the step's dt (XMM1), the target (R8), the rate (XMM3), the coefficients
// (stack: [0] the turning speed's gain a call, [1] its cap, [2] a bound), the previous state (stack). The speed grows by
// rate·coeff[0] a call without dt, then the angle by speed·dt: the gain is a step (logic_rate::Step: coeff[0]·k), passed in a copy
// of the coefficients; the cap and the bound unchanged. Not corrected (step review L5): its « error not growing » test compares
// with the previous call's error, not the one 1/60 s before; inert for the player (coeff[2] 0 makes the bound test always pass,
// series 10), rate-dependent for a character whose coeff[2] is above 0.
// 14050C7B0 (a scalar tracker, 7 calls a step at Firelink; GhidraMCP 2026-10-30): unless [this+0x5C], mode [this+0x58] 0 moves
// [this+0x1C] toward [this+0x18] by the gain [this+0x20] a call without dt (mode 1 steps by the time object's dt: right already).
// Mode 0's gain takes the gain law (logic_rate::Gain) for the call, restored after it.
using TrackerFn=void(__fastcall*)(uintptr_t,uint64_t*,uint64_t,uint32_t);
TrackerFn tracker_original{};
void __fastcall Tracker(uintptr_t self,uint64_t* time,uint64_t third,uint32_t fourth) noexcept {
    const float k=native_logic_k.load(std::memory_order_relaxed);
    if(k==1.f || *reinterpret_cast<uint8_t*>(self+0x5C) || *reinterpret_cast<int32_t*>(self+0x58)!=0){tracker_original(self,time,third,fourth);return;}
    auto& gain=*reinterpret_cast<float*>(self+0x20);const float kept=gain;
    if(!(kept>=0.f && kept<=1.f)){tracker_original(self,time,third,fourth);return;}   // final review M4: the law is defined on [0, 1] only
    gain=logic_rate::Gain(kept,k);tracker_original(self,time,third,fourth);gain=kept;
}
using TurningFn=float*(__fastcall*)(float*,float,int64_t,float,const float*,const uint64_t*);
TurningFn turning_original{};
bool active{};   // the corrections installed (22_setup); false when the detour only observes (locomotion-observe.flag at 60 or with no-unlock-fix.flag)
float* __fastcall Turning(float* out,float dt,int64_t target,float rate,const float* coefficients,const uint64_t* previous) noexcept {
    const float k=native_logic_k.load(std::memory_order_relaxed);
    float* result{};
    if(k==1.f || !coefficients || !active)result=turning_original(out,dt,target,rate,coefficients,previous);
    else{const float scaled[3]{coefficients[0]*k,coefficients[1],coefficients[2]};result=turning_original(out,dt,target,rate,scaled,previous);}
    player_turn::Note(out,dt,rate,coefficients,previous);   // locomotion-observe.flag's rows (04); nothing without it
    return result;
}
// I-0114 tranche 3 (UNLOCK-READY §4 point 2, slopes and stairs): the terrain response [this+0x2DC], TRTF's site 1405E6580 (the timing
// project's I-0013: the hopping on slopes and stairs at 240) and its siblings 1405D2280 and 1405CA3F0 (GhidraMCP 2026-10-31, read
// whole), each with a dt in XMM1 it does not use for this field:
// - 1405E6580: unless it hands the call to 1405CA3F0 (a predicate on the character), its ground probes 1407D7180 or 1407D7780
//   (fourth argument this+0x120, the answer through the sixth, true when they found ground; 1407D7780 writes no answer, so its
//   « found » adds 0.2·the caller's 0) give x + 0.2·min(the answers found);
//   none found, the decay [this+0x378]·x; 0 when the character has no ground state; then clamped to ±0.5.
// - 1405D2280: its probes' answers and hits kept in the object (+0x358 the probes, +0x360 the answers, +0x368 the hits, +0x370
//   their count, the misses' answers set to FLT_MAX): x + 0.2·min, else x·0.6; clamped to ±0.5.
// - 1405CA3F0: nothing without the character's ground predicate; x·0.6 (a hold [+0x2EC] and [+0x2F0], or no ground query); or x
//   clamped to the ground query's bounds (1409A21E0: true
//   when it found ground, its answer, less the mover's vertical speed [+0x1A4] when that is negative, through its second argument,
//   the bounds through its third and fourth), then x + 0.2·answer or x·0.6.
// Each call is corrected after it from the value before it (logic_rate's terrain laws: the response's step times k, the decay to
// the power k), its branch told from what the call left and checked against the original's own expression; a call that matches none
// is left as the original wrote it and counted (terrain_watch, 04). The probes and the ground query are only seen (their answers
// are not changed), within the call that asked them.
struct TerrainScope {uintptr_t self{},probe_owner{};int hits{};float least{std::numeric_limits<float>::max()};bool delegated{};TerrainScope* outer{};};
thread_local TerrainScope* terrain_scope{};
struct GroundQuery {bool seen{},found{};float answer{},high{},low{};GroundQuery* outer{};};
thread_local GroundQuery* ground_query{};
bool terrain_active{};   // the terrain laws applied (otherwise the detours only observe: locomotion-observe.flag at 60, no-terrain-fix.flag)
const logic_rate::TerrainConstants& TerrainConstantsOf() noexcept {
    static const logic_rate::TerrainConstants c{*reinterpret_cast<const float*>(image+0x3D7ED94),*reinterpret_cast<const float*>(image+0x3D7EDCC),
        *reinterpret_cast<const float*>(image+0x3D7EDB4),*reinterpret_cast<const float*>(image+0x3D7F0C0)};
    return c;
}
float& TerrainValue(uintptr_t self) noexcept {return *reinterpret_cast<float*>(self+0x2DC);}
// Each of the three returns the field in XMM0 (its epilogue's last load; series 23's fault: detours returning nothing left the
// caller a stale XMM0, a NaN in the mover's physics): the original's return, or the corrected field when the detour wrote it.
float TerrainReturned(uintptr_t self,float after,float returned) noexcept {const float now=TerrainValue(self);return logic_rate::SameBits(now,after)?returned:now;}
// The value the call leaves, or the corrected one when the laws apply (k ≠ 1) and the branch is known; the call noted.
void TerrainSettle(uintptr_t self,int site,terrain_watch::Branch kind,float before,float corrected,float k) noexcept {
    auto& x=TerrainValue(self);const bool write=terrain_active && k!=1.f && (kind==terrain_watch::Response || kind==terrain_watch::Decay) && !logic_rate::SameBits(x,corrected);
    if(write)x=corrected;
    terrain_watch::Note(self,site,kind,before,x,write);
}
using ProbeFn=uint8_t(__fastcall*)(uintptr_t,float,uintptr_t,uintptr_t,uint8_t,float*,uintptr_t,float,uint32_t);
ProbeFn probe_d7180_original{},probe_d7780_original{};
void SeeProbe(uintptr_t owner,uint8_t hit,const float* answer) noexcept {
    if(auto* s=terrain_scope;s && s->probe_owner && s->probe_owner==owner && hit && answer){++s->hits;if(*answer<=s->least)s->least=*answer;}
}
uint8_t __fastcall ProbeD7180(uintptr_t self,float dt,uintptr_t third,uintptr_t owner,uint8_t fifth,float* answer,uintptr_t seventh,float eighth,uint32_t ninth) noexcept {
    const uint8_t hit=probe_d7180_original(self,dt,third,owner,fifth,answer,seventh,eighth,ninth);SeeProbe(owner,hit,answer);return hit;
}
uint8_t __fastcall ProbeD7780(uintptr_t self,float dt,uintptr_t third,uintptr_t owner,uint8_t fifth,float* answer,uintptr_t seventh,float eighth,uint32_t ninth) noexcept {
    const uint8_t hit=probe_d7780_original(self,dt,third,owner,fifth,answer,seventh,eighth,ninth);SeeProbe(owner,hit,answer);return hit;
}
using GroundQueryFn=uint64_t(__fastcall*)(uintptr_t,float*,float*,float*);
GroundQueryFn ground_query_original{};
uint64_t __fastcall GroundQueryDetour(uintptr_t self,float* answer,float* high,float* low) noexcept {
    const uint64_t found=ground_query_original(self,answer,high,low);
    if(auto* q=ground_query;q && !q->seen && answer && high && low){q->seen=true;q->found=static_cast<uint8_t>(found)!=0;q->answer=*answer;q->high=*high;q->low=*low;}
    return found;
}
using TerrainFn=float(__fastcall*)(uintptr_t,float,uint64_t,uint64_t,uint8_t);
TerrainFn terrain_original{};
float __fastcall Terrain(uintptr_t self,float dt,uint64_t third,uint64_t fourth,uint8_t fifth) noexcept {
    const float k=native_logic_k.load(std::memory_order_relaxed);
    if(k==1.f && !terrain_watch::observe)return terrain_original(self,dt,third,fourth,fifth);
    const float before=TerrainValue(self);
    TerrainScope scope{self,self+0x120};scope.outer=terrain_scope;terrain_scope=&scope;
    const float returned=terrain_original(self,dt,third,fourth,fifth);
    terrain_scope=scope.outer;
    const auto& c=TerrainConstantsOf();const float after=TerrainValue(self);
    const float r=*reinterpret_cast<const float*>(self+0x378);   // final review: read where the original reads it, at its end
    using logic_rate::SameBits;
    if(scope.delegated)return returned;   // 1405CA3F0's own detour corrected and noted it (step review L2: not counted twice)
    if(scope.hits){
        if(SameBits(after,logic_rate::ClampHalf(scope.least*c.gain+before,c)))TerrainSettle(self,0,terrain_watch::Response,before,logic_rate::ClampHalf(logic_rate::TerrainResponse(before,scope.least,k,c),c),k);
        else TerrainSettle(self,0,terrain_watch::Mismatch,before,after,k);
        return TerrainReturned(self,after,returned);}
    if(SameBits(after,logic_rate::ClampHalf(r*before,c)))
        TerrainSettle(self,0,terrain_watch::Decay,before,logic_rate::TerrainDecayDefined(r)?logic_rate::ClampHalf(logic_rate::Ratio(before,r,k),c):after,k);
    else TerrainSettle(self,0,after==0.f?terrain_watch::Reset:terrain_watch::Mismatch,before,after,k);
    return TerrainReturned(self,after,returned);
}
using TerrainSiblingBFn=float(__fastcall*)(uintptr_t,float,uint64_t);
TerrainSiblingBFn terrain_d2280_original{};
float __fastcall TerrainD2280(uintptr_t self,float dt,uint64_t third) noexcept {
    const float k=native_logic_k.load(std::memory_order_relaxed);
    if(k==1.f && !terrain_watch::observe)return terrain_d2280_original(self,dt,third);
    const float before=TerrainValue(self);
    TerrainScope scope{self,0};scope.outer=terrain_scope;terrain_scope=&scope;   // its probes answer into the object: not seen
    const float returned=terrain_d2280_original(self,dt,third);
    terrain_scope=scope.outer;
    const auto& c=TerrainConstantsOf();const float after=TerrainValue(self);
    if(scope.delegated)return returned;
    const auto probes=*reinterpret_cast<const uintptr_t*>(self+0x358);const auto* answers=*reinterpret_cast<const float* const*>(self+0x360);
    const auto* hits=*reinterpret_cast<const uint8_t* const*>(self+0x368);const uint8_t count=*reinterpret_cast<const uint8_t*>(self+0x370);
    bool any=false;float least=std::numeric_limits<float>::max();
    if(probes && count && answers && hits)for(uint8_t i=0;i<count;++i){any|=hits[i]!=0;if(answers[i]<=least && least!=answers[i])least=answers[i];}   // the original's loop
    using logic_rate::SameBits;
    if(any && SameBits(after,logic_rate::ClampHalf(least*c.gain+before,c)))
        TerrainSettle(self,1,terrain_watch::Response,before,logic_rate::ClampHalf(logic_rate::TerrainResponse(before,least,k,c),c),k);
    else if(!any && SameBits(after,logic_rate::ClampHalf(before*c.decay,c)))
        TerrainSettle(self,1,terrain_watch::Decay,before,logic_rate::ClampHalf(logic_rate::Ratio(before,c.decay,k),c),k);
    else TerrainSettle(self,1,terrain_watch::Mismatch,before,after,k);
    return TerrainReturned(self,after,returned);
}
// 1405CA3F0's query branch (step review M1, the final review's refinement): when the bounds bind (held is a bound, not the field),
// each call restarts from that bound, so a result left outside [low, high] is the same at any rate (the next call clamps it back):
// the original's is kept (branch bounded). A corrected result inside the bounds (the field re-entering them, or never clamped) is
// a true step from the field, and is written.
void TerrainSettleBounded(uintptr_t self,terrain_watch::Branch kind,float before,float held,float corrected,float after,const GroundQuery& q,float k) noexcept {
    const bool inside=corrected>=q.low && corrected<=q.high;
    if(logic_rate::SameBits(held,before) || inside)TerrainSettle(self,2,kind,before,corrected,k);
    else TerrainSettle(self,2,terrain_watch::Bounded,before,after,k);
}
using TerrainSiblingAFn=float(__fastcall*)(uintptr_t,float);
TerrainSiblingAFn terrain_ca3f0_original{};
float __fastcall TerrainCA3F0(uintptr_t self,float dt) noexcept {
    const float k=native_logic_k.load(std::memory_order_relaxed);
    if(auto* s=terrain_scope;s && s->self==self)s->delegated=true;
    if(k==1.f && !terrain_watch::observe)return terrain_ca3f0_original(self,dt);
    const float before=TerrainValue(self);
    GroundQuery query{};query.outer=ground_query;ground_query=&query;
    const float returned=terrain_ca3f0_original(self,dt);
    ground_query=query.outer;
    const auto& c=TerrainConstantsOf();const float after=TerrainValue(self);
    using logic_rate::SameBits;
    if(query.seen){
        const float held=query.low<=before?(before<=query.high?before:query.high):query.low;   // the original's clamp to the query's bounds
        if(query.found){float answer=query.answer;
            const auto chr=*reinterpret_cast<const uintptr_t*>(self+8);const auto mover=*reinterpret_cast<const uintptr_t*>(*reinterpret_cast<const uintptr_t*>(chr+0x1F90)+0x68);
            if(const float rise=*reinterpret_cast<const float*>(mover+0x1A4);rise<0.f)answer=answer-rise;
            if(SameBits(after,held+answer*c.gain))TerrainSettleBounded(self,terrain_watch::Response,before,held,logic_rate::TerrainResponse(held,answer,k,c),after,query,k);
            else TerrainSettle(self,2,terrain_watch::Mismatch,before,after,k);}
        else if(SameBits(after,held*c.decay))TerrainSettleBounded(self,terrain_watch::Decay,before,held,logic_rate::Ratio(held,c.decay,k),after,query,k);
        else TerrainSettle(self,2,terrain_watch::Mismatch,before,after,k);}
    else if(SameBits(after,before))TerrainSettle(self,2,terrain_watch::Unchanged,before,after,k);
    else if(SameBits(after,before*c.decay))TerrainSettle(self,2,terrain_watch::Decay,before,logic_rate::Ratio(before,c.decay,k),k);
    else TerrainSettle(self,2,terrain_watch::Mismatch,before,after,k);
    return TerrainReturned(self,after,returned);
}
// The six detours (the three sites, the two probes, the ground query), before MH_EnableHook; false when one cannot be made.
bool InstallTerrain() noexcept {
    const auto hook=[](uint32_t rva,void* detour,void** original){return MH_CreateHook(reinterpret_cast<void*>(image+rva),detour,original)==MH_OK;};
    return hook(0x5E6580,reinterpret_cast<void*>(&Terrain),reinterpret_cast<void**>(&terrain_original)) &&
        hook(0x5D2280,reinterpret_cast<void*>(&TerrainD2280),reinterpret_cast<void**>(&terrain_d2280_original)) &&
        hook(0x5CA3F0,reinterpret_cast<void*>(&TerrainCA3F0),reinterpret_cast<void**>(&terrain_ca3f0_original)) &&
        hook(0x7D7180,reinterpret_cast<void*>(&ProbeD7180),reinterpret_cast<void**>(&probe_d7180_original)) &&
        hook(0x7D7780,reinterpret_cast<void*>(&ProbeD7780),reinterpret_cast<void**>(&probe_d7780_original)) &&
        hook(0x9A21E0,reinterpret_cast<void*>(&GroundQueryDetour),reinterpret_cast<void**>(&ground_query_original));
}
// I-0114 tranche 3 (UNLOCK-READY §4 point 4, the animation and effect ramps; GhidraMCP 2026-10-31, read whole): three ramps stepped
// by a constant a call with no dt, each used or copied inside the call that steps it, so each is pre-biased (logic_rate::PreBias:
// the original's own step lands on x + c·k) and the call's other work sees the corrected value:
// - 1407CB390 (a virtual, 2 calls a game step at Firelink): [this+0x24] up by [this+0x28]/60 to [this+0x28] while [this+0x20] bit 0
//   is set and [this+0x2C] < 0, else (bit 0 clear) down by the same step and, under 0, reset with its owner released: a 60-call fade.
// - 1407F5260 (a character's ragdoll blend, not called on the Firelink path): [this+0x1E0] up by 1/30 to 1 while under 1, copied
//   into the call's type-7 objects (+0x140, +0x160, +0x180): a 30-call ramp, its pre-bias kept under 1 so the step is still taken.
// - 1409B6D80 (one call a character a step): [this+0x60] up by 0.1 to 1 under a state bit of the character (bit 0 of its module's
//   +0x9CC), the factor of the call's transform blend: a 10-call ramp. Its condition is read inside the call, so the field is put
//   back when the call left it as written.
// Each detour returns the original's RAX and leaves its XMM0 (the three are void as read; the terrain sites were not: series 23).
// The pre-bias written into a ramp's field, and put back when the call left it as written (its step not taken: step review M3;
// UNLOCK-READY §1 method A), compared as integers so the call's XMM0 is left as it returned.
struct PreBiasField {
    volatile uint32_t* bits;uint32_t kept,given;
    PreBiasField(uintptr_t field,float value) noexcept:bits(reinterpret_cast<volatile uint32_t*>(field)),kept(*bits),given{} {std::memcpy(&given,&value,4);*bits=given;}
    void PutBackIfUnstepped() const noexcept {if(given!=kept && *bits==given)*bits=kept;}
};
using RampFadeFn=uint64_t(__fastcall*)(uintptr_t,uint64_t,uintptr_t);
RampFadeFn ramp_fade_original{};
uint64_t __fastcall RampFade(uintptr_t self,uint64_t second,uintptr_t context) noexcept {
    const float k=native_logic_k.load(std::memory_order_relaxed);
    if(k==1.f || !active)return ramp_fade_original(self,second,context);
    auto& x=*reinterpret_cast<float*>(self+0x24);const float span=*reinterpret_cast<const float*>(self+0x28);
    const float c=span/(*reinterpret_cast<const float*>(image+0x3D7EFA8));   // the original's [+0x28] / 60.0
    const bool up=(*reinterpret_cast<const uint8_t*>(self+0x20)&1) && *reinterpret_cast<const int32_t*>(self+0x2C)<0,down=!(*reinterpret_cast<const uint8_t*>(self+0x20)&1);
    if(!std::isfinite(c) || !(up || down))return ramp_fade_original(self,second,context);
    const auto biased=PreBiasField(self+0x24,up?logic_rate::PreBias(x,c,k):logic_rate::PreBias(x,-c,k));
    const uint64_t returned=ramp_fade_original(self,second,context);
    biased.PutBackIfUnstepped();return returned;
}
using RampRagdollFn=uint64_t(__fastcall*)(uintptr_t,float,uint64_t,uint64_t);
RampRagdollFn ramp_ragdoll_original{};
uint64_t __fastcall RampRagdoll(uintptr_t self,float blend,uint64_t third,uint64_t fourth) noexcept {
    const float k=native_logic_k.load(std::memory_order_relaxed);
    auto& x=*reinterpret_cast<float*>(self+0x1E0);const float end=*reinterpret_cast<const float*>(image+0x3D7EE10);   // 1.0
    if(k==1.f || !active || !(x<end))return ramp_ragdoll_original(self,blend,third,fourth);
    const auto biased=PreBiasField(self+0x1E0,logic_rate::PreBias(x,*reinterpret_cast<const float*>(image+0x3D7ED50),k,end));   // + 1/30
    const uint64_t returned=ramp_ragdoll_original(self,blend,third,fourth);
    biased.PutBackIfUnstepped();return returned;
}
using RampBlendFn=uint64_t(__fastcall*)(uintptr_t,uintptr_t);
RampBlendFn ramp_blend_original{};
uint64_t __fastcall RampBlend(uintptr_t self,uintptr_t time) noexcept {
    const float k=native_logic_k.load(std::memory_order_relaxed);
    if(k==1.f || !active)return ramp_blend_original(self,time);
    const auto biased=PreBiasField(self+0x60,logic_rate::PreBias(*reinterpret_cast<const float*>(self+0x60),*reinterpret_cast<const float*>(image+0x3D7ED70),k));   // + 0.1
    const uint64_t returned=ramp_blend_original(self,time);
    biased.PutBackIfUnstepped();return returned;
}
// I-0114 tranche 4 (the jump 3-5 % short above 60, series 27-29): 1409DEE60 (GhidraMCP 2026-11-01, read whole; one caller, the
// mover's step 1409DC090; it returns its second argument in RAX, no float), a character's mover making the velocity it hands its
// Havok proxy: from the call's displacement and dt while the ground state [mover+0x1DA] (or [+0x1DC]) is set; with both clear and
// [+0x1DB] (a jump) set, the velocity read back from Havok, i.e. the last step's, held through the air with gravity added. A jump
// clears [+0x1DA] at its takeoff, its animation three game steps ahead of its state (logic_rate::TakeoffDelay): at a shorter step the
// held speed is taken earlier on the jump's rising root motion (6.84 m/s at 240 against 7.47 at 60, series 29). The takeoff is
// told per mover by [+0x1DA] going from set to clear under [+0x1DB]; for TakeoffDelay(dt) from it the call is handed [+0x1DA] set
// (the state the original's step is in at 60 then) and the byte put back after it unless the call wrote it (under 60 nothing:
// logic_rate::TakeoffDelay). no-takeoff-fix.flag keeps the original's (the A/B).
namespace takeoff {
struct Slot {uintptr_t mover{};float owed{};uint8_t ground{};ULONGLONG seen{};};
std::array<Slot,128> slots{};SRWLOCK lock=SRWLOCK_INIT;
// A mover's slot while it is in a jump only (step review M2: freed when its jump ends, so the table holds the movers jumping
// now): found, or with insert the first empty one, else the first unseen for a second (final review L4: a mover destroyed in a
// jump never ends it), else its hash's; its ground state the call's.
Slot* Of(uintptr_t mover,bool insert,uint8_t ground) noexcept {
    const ULONGLONG now=GetTickCount64();Slot *empty{},*stale{};
    for(auto& s:slots){if(s.mover==mover){s.seen=now;return &s;}if(!s.mover){if(!empty)empty=&s;}else if(now-s.seen>1000 && !stale)stale=&s;}
    if(!insert)return nullptr;
    Slot& s=empty?*empty:stale?*stale:slots[(mover>>4)%slots.size()];s=Slot{mover,0.f,ground,now};return &s;
}
}
using MoverVelocityFn=float*(__fastcall*)(uintptr_t,float*,uintptr_t,uintptr_t);
MoverVelocityFn mover_velocity_original{};
float* __fastcall MoverVelocity(uintptr_t mover,float* velocity,uintptr_t target,uintptr_t step) noexcept {
    const float k=native_logic_k.load(std::memory_order_relaxed);
    if(k==1.f || !active || !step)return mover_velocity_original(mover,velocity,target,step);
    auto* ground=reinterpret_cast<volatile uint8_t*>(mover+0x1DA);
    const bool jump=*reinterpret_cast<const uint8_t*>(mover+0x1DB)!=0 && *reinterpret_cast<const uint8_t*>(mover+0x1DC)==0;
    const float dt=*reinterpret_cast<const float*>(step+8);const uint8_t now=*ground;bool hand{};
    AcquireSRWLockExclusive(&takeoff::lock);
    if(auto* s=takeoff::Of(mover,jump,now)){
        if(!jump)s->mover=0;
        else{if(!now && s->ground)s->owed=logic_rate::TakeoffDelay(dt);
            s->ground=now;
            hand=!now && std::isfinite(dt) && dt>0.f && logic_rate::TakeoffHand(s->owed,dt);}}
    ReleaseSRWLockExclusive(&takeoff::lock);
    if(!hand)return mover_velocity_original(mover,velocity,target,step);
    *ground=1;float* returned=mover_velocity_original(mover,velocity,target,step);
    if(*ground==1)*ground=0;
    return returned;
}
bool InstallTakeoff() noexcept {
    return MH_CreateHook(reinterpret_cast<void*>(image+0x9DEE60),reinterpret_cast<void*>(&MoverVelocity),reinterpret_cast<void**>(&mover_velocity_original))==MH_OK;
}
// I-0114 tranche 4 (UNLOCK-READY §3-§4, animation): 140828FE0 (GhidraMCP 2026-11-01, read whole; a bone constraint, not called on
// the paths run), its angle [this+0x1C] moved toward [this+0x18] by the gain [this+0x28] a call without dt: the gain law
// (logic_rate::Gain) for the call, the original's bits put back after it unless the call wrote the field. It returns its second
// argument in RAX (no float the caller reads): the detour forwards RAX and does no float work after the call.
using BoneConstraintFn=float*(__fastcall*)(uintptr_t,float*,uint64_t,uintptr_t);
BoneConstraintFn bone_constraint_original{};
float* __fastcall BoneConstraint(uintptr_t self,float* out,uint64_t third,uintptr_t pose) noexcept {
    const float k=native_logic_k.load(std::memory_order_relaxed);const float g=*reinterpret_cast<const float*>(self+0x28);
    if(k==1.f || !active || !(g>=0.f && g<=1.f))return bone_constraint_original(self,out,third,pose);
    const auto biased=PreBiasField(self+0x28,logic_rate::Gain(g,k));
    float* returned=bone_constraint_original(self,out,third,pose);
    biased.PutBackIfUnstepped();return returned;
}
// 141180110 (GhidraMCP 2026-11-01, read whole; bone weights, not called on the paths run): each constrained bone's weight w rises
// by (1 − w)·[this+0x70] a call while its target is held, else falls to w·(1 − [this+0x6C]); its pose blends toward the target by
// [this+0x68] a call. The three gains take the gain law for the call (1 − (1 − g)^k is the decay's law too), the original's bits
// put back after it; RAX forwarded, no float work after the call.
using BoneWeightsFn=uint64_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t);
BoneWeightsFn bone_weights_original{};
uint64_t __fastcall BoneWeights(uintptr_t self,uintptr_t second,uintptr_t pose) noexcept {
    const float k=native_logic_k.load(std::memory_order_relaxed);
    const auto gain=[&](uint32_t at){return *reinterpret_cast<const float*>(self+at);};
    const auto fits=[](float g){return g>=0.f && g<=1.f;};
    if(k==1.f || !active || !fits(gain(0x68)) || !fits(gain(0x6C)) || !fits(gain(0x70)))return bone_weights_original(self,second,pose);
    const PreBiasField a(self+0x68,logic_rate::Gain(gain(0x68),k)),b(self+0x6C,logic_rate::Gain(gain(0x6C),k)),c(self+0x70,logic_rate::Gain(gain(0x70),k));
    const uint64_t returned=bone_weights_original(self,second,pose);
    c.PutBackIfUnstepped();b.PutBackIfUnstepped();a.PutBackIfUnstepped();return returned;
}
// I-0114 tranche 5 (UNLOCK-READY §6's candidates, triaged with GhidraMCP 2026-11-01): 1408651E0, INFERRED the lock-on target
// manager's update ([0x14477DBE0]; two callers, 1408DF7B0 and a tail jump in 1408DFD60, passing it, the frame's dt in XMM1 and an
// object in R8; void, no caller reads a return; it writes the target's position [ChrIns+0x1080]), moves three lock-on ranges
// [this+0x2920..0x2928] toward a row's by the gain
// [this+0x2958] (0.1, its one writer 1408637A0) a call, its timers already by dt: the gain law for the call, the bits put back.
using RadiiFn=void(__fastcall*)(uintptr_t,float,uintptr_t,uintptr_t);
RadiiFn radii_original{};
void __fastcall Radii(uintptr_t self,float dt,uintptr_t third,uintptr_t fourth) noexcept {
    const float k=native_logic_k.load(std::memory_order_relaxed);
    if(k==1.f || !active || !self){radii_original(self,dt,third,fourth);return;}
    const float g=*reinterpret_cast<const float*>(self+0x2958);
    if(!(g>=0.f && g<=1.f)){radii_original(self,dt,third,fourth);return;}
    const auto biased=PreBiasField(self+0x2958,logic_rate::Gain(g,k));
    radii_original(self,dt,third,fourth);
    biased.PutBackIfUnstepped();
}
// I-0114 tranche 4 (UNLOCK-READY §3-§4, effects): 140245F90 (GhidraMCP 2026-11-01, read whole; 15.7 calls a game step at
// Firelink), an effect owner's two banks of 8 slots stepped by 1/30 a call with no dt: its one load of the step (140246240, movss
// xmm15,[0x143D7ED50], the shared 1/30) feeds each slot's age and life (1402459A0: += and −= the step), its fade-in timers
// ([+0xD10], [+0xD30]: + the step, the fade their ratio to 0.1), its drift (a velocity × the step) and a smoothing gain
// min(step, 0.95). The load is pointed at a module float (rip_float, method D) that BeginFrame sets to (1/30)·k: the timers and the
// drift exact (steps), the gain to first order (k·g against 1 − (1 − g)^k: 1.3 % at 240). no-effect-fix.flag keeps the original's.
// The same for a task evaluation (UNLOCK-READY §3, interface): 140C2A7B0 (GhidraMCP 2026-11-01, read whole) hands 140A4CDF0 a time
// object of 1/60 (its load at 140C2ACB6, movss xmm0,[0x143D7ED3C], the shared 1/60), the step it advances by; at k its 1/60·k.
struct StepLoad {float* slot{};float at60{};};
StepLoad effect_step{},task_step{};
template<size_t N> bool InstallStepLoad(StepLoad& load,uint32_t at,const BYTE (&opcode)[N],uint32_t constant) noexcept {
    load.slot=rip_float::Patch(image+at,opcode,image+constant);if(load.slot)load.at60=*load.slot;
    return load.slot!=nullptr;
}
bool InstallEffectStep() noexcept {constexpr BYTE kLoad[5]{0xF3,0x44,0x0F,0x10,0x3D};return InstallStepLoad(effect_step,0x246240,kLoad,0x3D7ED50);}
bool InstallTaskStep() noexcept {constexpr BYTE kLoad[4]{0xF3,0x0F,0x10,0x05};return InstallStepLoad(task_step,0xC2ACB6,kLoad,0x3D7ED3C);}
// Each frame (BeginFrame): the original's constant times k (its own bits at k = 1).
void SetStepLoads(float k) noexcept {
    for(auto* load:{&effect_step,&task_step})if(load->slot)*reinterpret_cast<volatile float*>(load->slot)=load->at60*k;
}
// 1401FD120 (GhidraMCP 2026-11-01, read whole; UNLOCK-READY §3 « child solver », not called on the paths run): a simulation's step,
// its dt (XMM1) kept at [this+0x60] and added to its time [this+0x64]; its only callers (1400A5AA0, two calls) pass 1/30 a call.
// Method B: that dt times k (the 60 frames/s reference's relative law). Void as read; the detour ends on the original's call.
using ChildSolverFn=void(__fastcall*)(uintptr_t,float);
ChildSolverFn child_solver_original{};
void __fastcall ChildSolver(uintptr_t self,float dt) noexcept {
    const float k=native_logic_k.load(std::memory_order_relaxed);
    child_solver_original(self,k==1.f || !active?dt:dt*k);
}
// 1406DE400 (GhidraMCP 2026-11-01, read whole; interface, not called on the paths run): [this+0x38] lowered by 1.5 a call while the
// id [this+0x40] stays the one of the last call ([this+0x44]), else 0: a hold counted in calls; then, with [this+0x48], a virtual
// of [this+0x20] whose answer is the return, which may read the hold. Pre-biased when the call will take the step (the original's
// own −1.5 lands on the value less 1.5·k, UNLOCK-READY §1 method A; step review L1: a correction after the call came after that
// virtual). It returns an integer; the detour forwards RAX.
using HoldValueFn=uint64_t(__fastcall*)(uintptr_t);
HoldValueFn hold_value_original{};
uint64_t __fastcall HoldValue(uintptr_t self) noexcept {
    const float k=native_logic_k.load(std::memory_order_relaxed);
    const uint32_t id=*reinterpret_cast<const uint32_t*>(self+0x40);const float x=*reinterpret_cast<const float*>(self+0x38);
    if(k==1.f || !active || id==0xFFFFFFFFu || *reinterpret_cast<const uint32_t*>(self+0x44)!=id || !std::isfinite(x))return hold_value_original(self);
    const auto biased=PreBiasField(self+0x38,logic_rate::PreBias(x,-*reinterpret_cast<const float*>(image+0x3D7EE38),k));   // − 1.5
    const uint64_t returned=hold_value_original(self);
    biased.PutBackIfUnstepped();return returned;
}
// I-0114 tranche 7 (UNLOCK-READY §3, the target timer; GhidraMCP 2026-11-01, read whole): 140D71920, a virtual of the enemies'
// Havok AI character NS_SPRJ::SprjHkAiCharacter (its vtable 14295EBB8, logged in series 35 at the Undead Settlement and Anor
// Londo; its RTTI read), returns a target's position ([this+0x218], its +0x20) plus an offset (INFERRED its steering goal).
// Two per-call laws without dt: the offset's timer [this+0x240] −1/30 a call (a new offset from 140D71DE0 when under 0; in its
// state 2 the offset kept until then), pre-biased as the hold (the original's own −1/30 lands on −1/30·k, the bits put back when
// the call did not step it); and a ring of the last N offsets (N [this+0x238] = [0x144549970], 20 at the start; 16-byte samples
// from [this+0x30], 30 at most before the target pointer [+0x218]; start +0x230, next +0x234) summed and normalised (140D73250):
// N - 1 samples, 317 ms at 60 and 79 at 240, its calls following the game steps (series 36: 1.85 a step at 60, 1.77 at 240).
// The ring held to one sample a 1/60 s (logic_rate::RingByTicks: the newest replaced until a call completes its tick, repeated
// for ticks a longer call completes), per object; a call left to the original (k = 1) ends its object's tick in progress (the final
// review's L2). no-target-timer-fix.flag keeps the original's (the A/B).
namespace target_timer {
struct Slot {uintptr_t self{};float owed{};bool pending{};ULONGLONG seen{};};
std::array<Slot,256> slots{};SRWLOCK lock=SRWLOCK_INIT;
std::atomic<LONG64>& calls=target_timer_watch::calls;std::array<std::atomic<uintptr_t>,4> vtables{};
bool active{},observe{};   // the correction (no-target-timer-fix.flag); the calls counted and the vtables logged (jump-observe.flag)
Slot& Of(uintptr_t self) noexcept {
    const ULONGLONG now=GetTickCount64();Slot *empty{},*stale{};
    for(auto& s:slots){if(s.self==self){s.seen=now;return s;}if(!s.self){if(!empty)empty=&s;}else if(now-s.seen>5000 && !stale)stale=&s;}
    Slot& s=empty?*empty:stale?*stale:slots[(self>>4)%slots.size()];s=Slot{self,0.f,false,now};return s;
}
// A call the original made alone: the object's newest sample is a whole call's, no tick in progress (its time owed kept).
void Forget(uintptr_t self) noexcept {for(auto& s:slots)if(s.self==self){s.pending=false;return;}}
void SeeVtable(uintptr_t self) noexcept {
    const uintptr_t table=*reinterpret_cast<const uintptr_t*>(self);
    for(auto& v:vtables){uintptr_t expected=0;if(v.load(std::memory_order_relaxed)==table)return;
        if(v.compare_exchange_strong(expected,table)){char row[160]{};sprintf_s(row,"{\"event\":\"unlock_target_timer_seen\",\"vtable\":\"0x%llX\",\"calls\":%lld}\n",
                static_cast<unsigned long long>(table-image+0x140000000ull),static_cast<long long>(calls.load(std::memory_order_relaxed)));Log(row);return;}}
}
}
using TargetTimerFn=float*(__fastcall*)(uintptr_t,float*);
TargetTimerFn target_timer_original{};
float* __fastcall TargetTimer(uintptr_t self,float* out) noexcept {
    if(target_timer::observe){target_timer::calls.fetch_add(1,std::memory_order_relaxed);if(self)target_timer::SeeVtable(self);}
    const float k=native_logic_k.load(std::memory_order_relaxed);
    if(k==1.f || !active || !target_timer::active || !self || !*reinterpret_cast<const uintptr_t*>(self+0x218)){
        if(k==1.f && active && target_timer::active && self){AcquireSRWLockExclusive(&target_timer::lock);target_timer::Forget(self);ReleaseSRWLockExclusive(&target_timer::lock);}
        return target_timer_original(self,out);}
    // The ring (its size as the call will set it, from the global; the indices checked against it and its 30 slots).
    const uint32_t size=*reinterpret_cast<const uint32_t*>(image+0x4549970);
    auto* start=reinterpret_cast<uint32_t*>(self+0x230);auto* next=reinterpret_cast<uint32_t*>(self+0x234);
    if(size>=2 && size<=30 && *start<30 && *next<size){int ticks{};bool pending{};
        AcquireSRWLockExclusive(&target_timer::lock);
        {auto& s=target_timer::Of(self);ticks=logic_rate::SixtiethTicks(s.owed,k/60.f);pending=s.pending;s.pending=ticks==0;}
        ReleaseSRWLockExclusive(&target_timer::lock);
        const auto step=logic_rate::RingByTicks(ticks,pending);logic_rate::RingIndex r{*start,*next,size};
        if(step.rewind)logic_rate::RingRewind(r);
        if(r.start!=r.next)for(int j=0;j<step.copies;++j){
            std::memcpy(reinterpret_cast<void*>(self+0x30+r.next*16),reinterpret_cast<const void*>(self+0x30+((r.next+size-1)%size)*16),16);logic_rate::RingPush(r);}
        *start=r.start;*next=r.next;}
    // The timer, pre-biased (− 1/30, the call's own constant [0x143D7ED50]).
    const float x=*reinterpret_cast<const float*>(self+0x240);
    if(!std::isfinite(x))return target_timer_original(self,out);
    const auto biased=PreBiasField(self+0x240,logic_rate::PreBias(x,-*reinterpret_cast<const float*>(image+0x3D7ED50),k));
    float* returned=target_timer_original(self,out);
    biased.PutBackIfUnstepped();return returned;   // RAX the result's pointer (its XMM0 a leftover of its last store: no float work here)
}
// Its detour alone: with the ramps, and at 60 with jump-observe.flag (the original's call, only counted: the step review's M1).
bool InstallTargetTimer() noexcept {return MH_CreateHook(reinterpret_cast<void*>(image+0xD71920),reinterpret_cast<void*>(&TargetTimer),reinterpret_cast<void**>(&target_timer_original))==MH_OK;}
bool InstallRamps() noexcept {
    const auto hook=[](uint32_t rva,void* detour,void** original){return MH_CreateHook(reinterpret_cast<void*>(image+rva),detour,original)==MH_OK;};
    return hook(0x7CB390,reinterpret_cast<void*>(&RampFade),reinterpret_cast<void**>(&ramp_fade_original)) &&
        hook(0x7F5260,reinterpret_cast<void*>(&RampRagdoll),reinterpret_cast<void**>(&ramp_ragdoll_original)) &&
        hook(0x9B6D80,reinterpret_cast<void*>(&RampBlend),reinterpret_cast<void**>(&ramp_blend_original)) &&
        hook(0x828FE0,reinterpret_cast<void*>(&BoneConstraint),reinterpret_cast<void**>(&bone_constraint_original)) &&
        hook(0x1180110,reinterpret_cast<void*>(&BoneWeights),reinterpret_cast<void**>(&bone_weights_original)) &&
        hook(0x8651E0,reinterpret_cast<void*>(&Radii),reinterpret_cast<void**>(&radii_original)) &&
        hook(0x6DE400,reinterpret_cast<void*>(&HoldValue),reinterpret_cast<void**>(&hold_value_original)) &&
        hook(0x1FD120,reinterpret_cast<void*>(&ChildSolver),reinterpret_cast<void**>(&child_solver_original)) &&
        InstallTargetTimer();
}
}
// I-0114 tranche 1 (unlock-observe.flag, UNLOCK-READY §4's diagnostic): calls of each frame-dependent site, counted by a thunk at
// its entry (lock inc of its counter, then a jump to the original: no register but RAX and the flags touched, so any signature
// passes through), with DS3's game step 8F2CA0 as the frame count; a row every ten seconds; native runs only (a Reference run returns
// from Setup before it: step review L2). A site a correction hooks is skipped and named (not counted: « 0 » there means skipped).
namespace unlock_observe {
struct Site {uint32_t rva;const char* name;};
constexpr Site sites[]{{0x8F2CA0,"game_step"},{0x5C72B0,"locomotion"},{0x5DC510,"gait"},{0x9CF010,"lookahead"},{0x5E6580,"terrain"},
    {0x5CA3F0,"terrain_sibling_a"},{0x5D2280,"terrain_sibling_b"},{0x5D0010,"turning"},{0x9B6D80,"transform_lerp"},{0x7F5260,"ramp_1e0"},
    {0x7CB390,"ramp_24"},{0x828FE0,"bone_constraint"},{0x1180110,"bone_weights"},{0x50C7B0,"tracker"},{0xD71920,"target_timer"},
    {0x2A5D60,"camera_a"},{0x2AF510,"camera_b"},{0x6DE400,"hold_value"},{0x1FD120,"child_solver"},{0x245F90,"effect_slots"},
    {0x16F27E0,"yebis_step"},{0xA4CDF0,"task_eval"},{0x741A30,"ui_virtual"},{0xEF2CE0,"network_timers"},{0x663300,"cycle_a"},
    {0x663340,"cycle_b"},{0x72FEC0,"delay_4"},{0x87E8A0,"owner_1a4c"},{0xDD4B60,"dt_selector"},{0x100DDE0,"broadphase"},
    {0x5C9DC0,"candidate_5c9dc0"},{0x5CA570,"candidate_5ca570"},{0x5D1B70,"candidate_5d1b70"},{0x5E57F0,"candidate_5e57f0"}};
constexpr size_t count=std::size(sites);
std::array<volatile LONG64,count> calls{};
std::array<void*,count> originals{};
// mov rax,&calls[i]; lock inc qword [rax]; mov rax,&originals[i]; jmp qword [rax]
void* Thunk(size_t i,BYTE* memory){
    BYTE* p=memory;const auto emit=[&](std::initializer_list<BYTE> b){for(const auto x:b)*p++=x;};
    const auto imm=[&](const void* v){const auto u=reinterpret_cast<uint64_t>(v);std::memcpy(p,&u,8);p+=8;};
    emit({0x48,0xB8});imm(const_cast<LONG64*>(&calls[i]));emit({0xF0,0x48,0xFF,0x00});
    emit({0x48,0xB8});imm(&originals[i]);emit({0xFF,0x20});
    return memory;
}
// The thunks and the hooks, before MH_EnableHook; false when one cannot be made.
bool Install(){
    auto* memory=static_cast<BYTE*>(VirtualAlloc(nullptr,count*32,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if(!memory)return false;
    std::array<void*,count> thunks{};
    for(size_t i=0;i<count;++i)thunks[i]=Thunk(i,memory+i*32);
    DWORD old{};if(!VirtualProtect(memory,count*32,PAGE_EXECUTE_READ,&old))return false;
    FlushInstructionCache(GetCurrentProcess(),memory,count*32);
    // A site a correction already hooks (I-0114 tranche 2) is not counted: one detour an entry; it is named (step review L3).
    for(size_t i=0;i<count;++i)if(const auto made=MH_CreateHook(reinterpret_cast<void*>(image+sites[i].rva),thunks[i],&originals[i]);made==MH_ERROR_ALREADY_CREATED){
            char row[160]{};sprintf_s(row,"{\"event\":\"unlock_observe_site_skipped\",\"site\":\"%s\",\"reason\":\"corrected\"}\n",sites[i].name);Log(row);continue;}
        else if(made!=MH_OK){
        char row[160]{};sprintf_s(row,"{\"event\":\"unlock_observe_hook_failed\",\"site\":\"%s\"}\n",sites[i].name);Log(row);return false;}
    std::thread([]{std::array<LONG64,count> last{};
        while(!control->stop){Sleep(10000);std::string row="{\"event\":\"unlock_observe\",\"seconds\":10,\"calls\":{";
            const LONG64 steps=calls[0]-last[0];
            for(size_t i=0;i<count;++i){const LONG64 n=calls[i]-last[i];last[i]+=n;char cell[96]{};
                sprintf_s(cell,"%s\"%s\":[%lld,%.3f]",i?",":"",sites[i].name,static_cast<long long>(n),steps?double(n)/double(steps):0.0);row+=cell;}
            char tail[96]{};sprintf_s(tail,"},\"k\":%.4f}\n",native_logic_k.load(std::memory_order_relaxed));row+=tail;Log(row.c_str());}}).detach();
    return true;
}
}
// I-0111 tranche 4 step 2: the frame's DLSS from the game's Antialiasing option (native_dlss_active, 01_state.inc); its row names
// the option's byte and whether DLSS is ready (final review L3: the byte read whatever Streamline's state).
void FollowNativeDlssOption() noexcept {
    uintptr_t root{},quality{};BYTE aa{};
    const bool read=Read(image+0x4961B50,&root,8) && root && Read(root+0x5360,&quality,8) && quality && Read(quality+8,&aa,1);
    const auto choice=NativeDlssNow();   // I-0132 R12f: the settings' mode (live), taken once for the frame
    const bool ready=ds3native::streamline::DlssReady(),on=ready && read && aa && choice.chosen;
    // I-0132 R11j: the TAA while the option is on and no DLSS mode runs (the renderer's latch stops it, and with it the jitter).
    const bool taa=read && aa && !on && NativeTaaRequested() && !renderer.TaaFailed();native_taa_active.store(taa,std::memory_order_relaxed);
    // I-0140 tranche 40 (D-0305): the menu views' TAA whenever the main view has an anti-aliasing (the TAA or a DLSS mode).
    const bool menu_taa=read && aa && (taa || on) && NativeMenuTaaRequested() && !renderer.MenuTaaFailed();native_menu_taa_active.store(menu_taa,std::memory_order_relaxed);
    {static bool named=false;if(!named){named=true;const auto mode=NativeDlssMode();char row[192]{};
        sprintf_s(row,"{\"event\":\"native_dlss_mode\",\"mode\":\"%s\",\"chosen\":%s,\"source\":\"%s\",\"text\":\"%s\"}\n",ds3native::streamline::DlssModeName(mode),NativeDlssChosen()?"true":"false",native_dlss_mode_source,native_dlss_mode_text.c_str());Log(row);}}
    // Step review L-b: the row at the first frame and at each change of what it names (the byte, its read, Streamline's state).
    // I-0132 R12f: and at each change of the mode the settings choose.
    static int last_key=-1;const int key=int(read)|(int(aa!=0)<<1)|(int(ready)<<2)|(int(taa)<<3)|(int(choice.chosen)<<4)|(int(choice.mode)<<5)|(int(menu_taa)<<9);native_dlss_active.store(on,std::memory_order_relaxed);
    // I-0123 (step review L5): Ray Reconstruction's state taken here too, so the frame's jitter and render extent agree on it.
    // Final review L3: and the renderer's own flag (its latch), so a restarted Streamline does not bring RR back on the game thread alone.
    native_rr_active.store(on && native_rr_session.load(std::memory_order_relaxed) && renderer.rr.load(std::memory_order_relaxed) &&
        ds3native::streamline::RrReady(),std::memory_order_relaxed);
    if(key!=last_key){last_key=key;char row[256]{};
        sprintf_s(row,"{\"event\":\"native_dlss_option\",\"active\":%s,\"taa\":%s,\"menu_taa\":%s,\"aa_byte\":%u,\"read\":%s,\"dlss_ready\":%s,\"mode\":\"%s\",\"chosen\":%s}\n",on?"true":"false",taa?"true":"false",menu_taa?"true":"false",
            unsigned(aa),read?"true":"false",ready?"true":"false",ds3native::streamline::DlssModeName(choice.mode),choice.chosen?"true":"false");Log(row);}
}
// I-0111 tranche 2 (Reflex, D-0240): the mode of the settings' reflex sent to Streamline (at least once, Off included; again only on a
// change, inside SetReflexMode), each frame until Reflex answers; one row at each change of what it reports. Reflex on: its sleep is
// the frame-start wait (EndFrame), in place of the display clock's object wait, and the display clock is not engaged (Clear).
void FollowNativeReflex() noexcept {
    namespace slh=ds3native::streamline;
    if(!slh::Initialised()){native_reflex_on=false;return;}   // step review L4
    // I-0131 R10b (ProgrammingGuideDLSS_G §8: required): Reflex On at least while DLSS-G is loaded.
    // I-0132 R12f (NIS's step review L1): while the plugin is loaded, wanted or not (its pacer owns the frame-latency object, §12.1).
    auto mode=NativeReflexMode();if(renderer.FrameGenerationLoaded() && mode==slh::ReflexMode::Off)mode=slh::ReflexMode::On;
    std::string reason;const bool sent=slh::SetReflexMode(mode,reason);
    native_reflex_on=sent && mode!=slh::ReflexMode::Off;
    // A row at each change, with the driver's own low-latency flag (step review L5: « sent » is Streamline's acceptance only).
    static int last=-1;const int low=sent?slh::ReflexLowLatencyAvailable():-1;const int key=int(sent)|(int(mode)<<1)|((low+1)<<4);
    if(key!=last && (sent || reason.find("not answered yet")==std::string::npos)){last=key;char row[288]{};
        sprintf_s(row,"{\"event\":\"native_reflex\",\"mode\":\"%s\",\"sent\":%s,\"low_latency_available\":%d,\"reason\":\"%.120s\"}\n",slh::ReflexModeName(mode),sent?"true":"false",low,sent?"":reason.c_str());Log(row);}
}
// I-0132 R12f (the settings live): what the start hands the renderer once (20_startup_phases.inc) followed at each engine frame: the
// rays' level, the PT level's bounces and Ray Reconstruction (only where the rays were set up at start, rt_asked; each written on a
// change only, so the renderer's own RR latch holds until the player changes RR), the V-sync (vsync-off.flag keeps it off) and the
// unlocked rate's presentation (Present(1) or (0) without the 1/60 s pacing). The TAA, the sharpening, the DLSS mode, Reflex and the
// rate itself are read live by their own followers. One row at each change, the first at the first frame.
// I-0138 tranche 5: the HDR10 output's row, at the start and at each change while the game runs (`when`).
void LogNativeHdrOutput(const char* when) noexcept {
    const auto h=renderer.HdrOutput();
    std::string why=h.failure.substr(0,120);for(auto& c:why)if(c=='"' || c=='\\' || c<32)c='\'';
    char row[720]{};sprintf_s(row,"{\"event\":\"native_hdr_output\",\"when\":\"%s\",\"mode\":\"%s\",\"hdr10\":%s,\"format10\":%s,\"paper_white\":%.1f,\"peak\":%.1f,"
        "\"screen_found\":%s,\"screen_hdr\":%s,\"screen_color_space\":%u,\"screen_bits\":%u,\"screen_max_nits\":%.1f,\"screen_max_full_frame_nits\":%.1f,\"screen_min_nits\":%.4f,\"failure\":\"%s\"}\n",
        when,h.request.mode==ds3native::hdr::Mode::Force?"force":h.request.mode==ds3native::hdr::Mode::Auto?"auto":"off",h.hdr10?"true":"false",h.format10?"true":"false",double(h.request.paper_white),double(h.peak),
        h.display.found?"true":"false",h.display.hdr?"true":"false",h.display.color_space,h.display.bits,double(h.display.max_nits),double(h.display.max_full_frame_nits),double(h.display.min_nits),why.c_str());
    Log(row);}
void FollowNativeLiveSettings() noexcept {
    static const bool vsync_flag_off=ModuleFlag(L"vsync-off.flag");
    const bool rt=renderer.rt_asked.load(std::memory_order_relaxed);
    const bool pt=rt && NativeRtPtRequested(),rr=rt && NativeRrRequested();const uint32_t bounces=NativeRtBouncesRequested(),gi_paths=NativeRtGiPathsRequested();
    const bool sync=!vsync_flag_off && NativeSettingsCurrent().vsync;const float rate=NativeRequestedRate();
    static int last_pt=-1,last_rr=-1;static uint32_t last_bounces{},last_gi_paths{};static int last_sync=-1;static float last_rate=-1.f;
    if(rt){if(int(pt)!=last_pt)renderer.rt_pt.store(pt,std::memory_order_relaxed);if(bounces!=last_bounces)renderer.rt_bounces.store(bounces,std::memory_order_relaxed);
        if(gi_paths!=last_gi_paths)renderer.rt_gi_paths.store(gi_paths,std::memory_order_relaxed);   // I-0132 R12g
        if(int(rr)!=last_rr){renderer.rr.store(rr,std::memory_order_relaxed);native_rr_session.store(rr,std::memory_order_relaxed);}}
    if(int(sync)!=last_sync)renderer.SetPresentSyncLive(sync);
    // I-0132 R12g (R12f's final review L7; Thomas asked 2026-11-11, no answer by the checkpoint: the former R pilot's recommendation
    // applied, RESULT « R12g »): frame generation held off while an in-game menu is open, the NewMenuSystem's root window
    // ([14478DA40] + 0xA0, MENUS-MAP §2: non-zero while the in-game top menu or one opened from it is open); a row at each change.
    {uintptr_t system{},root{};const bool menu=renderer.FrameGeneration() && Read(image+0x478DA40,&system,8) && system && Read(system+0xA0,&root,8) && root;
        static int last_menu=-1;if(int(menu)!=last_menu){last_menu=int(menu);renderer.fg_menu_hold.store(menu,std::memory_order_relaxed);
            char row[96]{};sprintf_s(row,"{\"event\":\"native_frame_generation_menu\",\"held\":%s}\n",menu?"true":"false");Log(row);}}
    if(rate!=last_rate)renderer.SetPresentUnpacedLive(rate>0);
    // Frame generation (the second tranche): its chain made again when it is switched on or off (SetFrameGenerationLive, NVIDIA's §18),
    // the worker quiesced; the multiplier alone changed in place. A refusal is named once a value; the next change tries again. A busy
    // refusal (a presentation change, or the exclusive full screen not left now) is tried again every 30th frame, its row the first time
    // and every 20th try (its step review's M1: no quiesce and drain each frame while DXGI keeps refusing).
    {const uint32_t fg=NativeFrameGenerationWanted();static uint32_t last_fg=UINT32_MAX,busy_wait{},busy_tries{};
        if(last_fg==UINT32_MAX)last_fg=fg;   // the start made the chain for it
        else if(busy_wait && fg==last_fg)--busy_wait;
        else if(fg!=last_fg || busy_tries){last_fg=fg;busy_wait=0;const bool was=renderer.FrameGeneration();bool ok=false,busy=false;std::string reason;
            LARGE_INTEGER frequency{},start{},end{};QueryPerformanceFrequency(&frequency);QueryPerformanceCounter(&start);
            try{WithPresentationIdle([&]{ok=renderer.SetFrameGenerationLive(fg,reason);});busy=!ok && reason.rfind("busy",0)==0;}
            catch(const NativePresentationBusy&){reason="busy: a presentation change";busy=true;}
            catch(const std::exception& e){LogNativeFailure("frame generation change: ",e.what());Refuse("native_frame_generation_change_failed");}
            busy_tries=busy?busy_tries+1:0;busy_wait=busy?30u:0u;
            QueryPerformanceCounter(&end);for(auto& c:reason)if(c=='"' || c=='\\' || c<32)c=' ';
            if(!busy || busy_tries==1 || busy_tries%20==0){char row[352]{};
                sprintf_s(row,"{\"event\":\"native_frame_generation_live\",\"asked\":%u,\"was\":%s,\"now\":%s,\"ok\":%s,\"ms\":%.1f,\"busy_tries\":%u,\"reason\":\"%.160s\"}\n",fg,
                    was?"true":"false",renderer.FrameGeneration()?"true":"false",ok?"true":"false",1000.0*double(end.QuadPart-start.QuadPart)/double(frequency.QuadPart),busy_tries,reason.c_str());Log(row);}}}
    // I-0138 tranche 5: the HDR10 output followed: the rows' request, and the screen's answer when the window is on another screen
    // or DXGI's factory is no longer current (Windows' HDR switched); between frames, the worker quiesced, the chain resized only
    // when its format changes. A presentation change under way: tried again thirty frames later. A resize the system refused (the
    // chain kept its format: the final review's L1): asked again thirty frames later, five times, then left until the request or
    // the screen changes.
    // I-0138 tranche 5b: the chain's kind for the game's Brightness row (02b), read at every engine frame (its final review's L2: a
    // resize the system refuses can leave an SDR chain without a row of ours).
    native_hdr10_chain.store(renderer.Hdr10Chain(),std::memory_order_relaxed);
    {const auto request=NativeHdrRequest();static ds3native::hdr::Request last=renderer.HdrOutput().request,tried=last;static uint32_t busy_wait{},refused{};   // what Start gave the renderer
        const bool changed=request.mode!=last.mode || request.paper_white!=last.paper_white || request.peak!=last.peak;
        if(request.mode!=tried.mode || request.paper_white!=tried.paper_white || request.peak!=tried.peak){tried=request;refused=0;}   // another request: its own five tries
        if(busy_wait)--busy_wait;
        else if(changed || refused || renderer.HdrOutputStale()){bool resized=false,done=false;
            try{WithPresentationIdle([&]{resized=renderer.SetHdrOutputLive(request);});done=true;}
            catch(const NativePresentationBusy&){busy_wait=30;}
            catch(const std::exception& e){LogNativeFailure("HDR output change: ",e.what());Refuse("native_hdr_output_change_failed");}
            if(done){const bool settled=renderer.HdrOutputSettled(),left=!settled && ++refused>=5;
                if(settled || left){last=request;refused=0;}else busy_wait=30;
                LogNativeHdrOutput(!resized?"live":settled?"live, the chain resized":left?"live, the resize refused, left":"live, the resize refused");}}}
    if(int(pt)!=last_pt || int(rr)!=last_rr || bounces!=last_bounces || gi_paths!=last_gi_paths || int(sync)!=last_sync || rate!=last_rate){
        char row[240]{};sprintf_s(row,"{\"event\":\"native_settings_live\",\"first\":%s,\"rt\":%s,\"level\":\"%s\",\"rr\":%s,\"bounces\":%u,\"gi_paths\":%u,\"vsync\":%s,\"frame_rate\":%.0f}\n",
            last_sync<0?"true":"false",rt?"true":"false",pt?"pt":"rt",rr?"true":"false",bounces,gi_paths,sync?"true":"false",double(rate));Log(row);
        last_pt=int(pt);last_rr=int(rr);last_bounces=bounces;last_gi_paths=gi_paths;last_sync=int(sync);last_rate=rate;}
}
void __fastcall BeginFrame(void* manager) noexcept {
    kprofile::Mark(kprofile::FrameBegin);
    if(control->stop && control->closed) return;
    if(!opened) Refuse("frame_begin_without_native_owner");
    if(GetCurrentThreadId()!=owner_thread) Refuse("frame_begin_thread_changed");
    // R13 cR3 (sheet 8.2): a fault of the render worker's last job ends the process here (this hook is noexcept: its row,
    // then the refusal exit), before the engine starts another frame.
    if(render_worker && render_worker->Faulted()){
        try{render_worker->RethrowFault();}catch(const std::exception& e){LogNativeFailure("",e.what());}
        Refuse("native_render_worker_faulted");}
    StepRekindledGraphics();   // I-0132 R12c: the Graphics rows' changes into the settings file and the window, Auto-detect kept OFF, GraphicsConfig.xml written once applied
    FollowNativeLiveSettings();   // I-0132 R12f: the rows' changes applied (before the DLSS option takes RR's state)
    FollowWitnessShareVariant();   // I-0141 tranche 26: a bounded run's share variant (test tooling)
    FollowNativeDlssOption();   // I-0111 tranche 4 step 2: the game's Antialiasing option, before the render extent follows it
    FollowNativeReflex();
    try{StepNativeDisplayCycle();FollowNativePresentationState();ApplyNativeDisplayRequest();FollowNativeOutputExtent();FollowNativeRenderExtent();}   // I-0076: the screen mode, then the engine's size S (after this frame's collect, before its render step, INFERRED: ENGINE-STEP-THREAD-MAP's best reading, and r12f-live5's UI packet made at the former extent; I-0132 R12f); I-0101: a lost presentation followed first
    catch(const NativePresentationBusy&){}   // review M1: not reached from the frame start (no presentation change encloses it)
    catch(const std::exception& e){LogNativeFailure("display or output change: ",e.what());Refuse("native_output_resize_failed");}
    uintptr_t clock{};
    if(!Read(image+0x489DD10,&clock,8) || !clock) Refuse("frame_clock_unavailable");
    // I-0088 P0: original-unlock.flag in a native run (measurement only): the game's requested rate 360 and its enable byte, the
    // pair the reference probe writes (timing/ I-0009, I-0011), so the engine thread is measured without DS3's 60 cap.
    // I-0132 R12f: the rate live; DS3's own pair (as the game left it before the first write) put back when the player returns to DS3's
    // 60 (the rate 0), and the logic's k then held at 1 (the corrections, installed at every native start, run the original's calls).
    static const bool unlock=ModuleFlag(L"original-unlock.flag");const float rate=unlock?0.f:NativeRequestedRate();
    native_rate_unlocked.store(unlock || rate>0,std::memory_order_relaxed);   // FG's step review L2: k's own rule
    {static bool kept{};static float kept_rate{};static BYTE kept_enable{};
        if(unlock){*reinterpret_cast<volatile float*>(clock+0x354)=360.f;*reinterpret_cast<volatile BYTE*>(clock+0x358)=1;}
        // I-0114 tranche 1: the requested rate (written each frame, as above, so a reset by the game does not stick).
        else if(rate){if(!kept){kept=Read(clock+0x354,&kept_rate,4) && Read(clock+0x358,&kept_enable,1);}
            *reinterpret_cast<volatile float*>(clock+0x354)=rate;*reinterpret_cast<volatile BYTE*>(clock+0x358)=1;}
        else if(kept){kept=false;*reinterpret_cast<volatile float*>(clock+0x354)=kept_rate;*reinterpret_cast<volatile BYTE*>(clock+0x358)=kept_enable;
            char row[128]{};sprintf_s(row,"{\"event\":\"native_frame_rate_restored\",\"rate\":%g,\"enable\":%u}\n",double(kept_rate),unsigned(kept_enable));Log(row);}}
    {float dt{};const bool read=Read(clock+0x268,&dt,4);
        // I-0113 gap 27: k capped at 1 under 60 frames/s (logic_rate::FrameK); unlock-fix-below-60.flag keeps k > 1 (the A/B). I-0132 R12f:
        // held at 1 at DS3's own rate (the rate 0; the corrections installed at every native start).
        static const bool below_60=ModuleFlag(L"unlock-fix-below-60.flag");
        native_logic_k.store(!unlock && rate<=0?1.f:read?logic_rate::FrameK(dt,below_60):1.f,std::memory_order_relaxed);if(read && std::isfinite(dt) && dt>0)native_logic_seconds+=dt;
        // I-0114 tranche 4: the effect slots' step (unlock_fix::InstallEffectStep), the original's 1/30 at k = 1.
        unlock_fix::SetStepLoads(native_logic_k.load(std::memory_order_relaxed));}
    // I-0114 (CAMERA-FOLLOW-SITES §4.1, a test of the 240 sprint's drift): no-wall-recovery.flag clears the follow camera's automatic
    // wall recovery gate [0x14455BA10] each frame (its only writer 1405105A5 copies it from the options); its value before the first
    // clear and each change the game writes are named in one row.
    {static const bool off=ModuleFlag(L"no-wall-recovery.flag");static int last=-1;
        if(off){const int was=*reinterpret_cast<volatile BYTE*>(image+0x455BA10);*reinterpret_cast<volatile BYTE*>(image+0x455BA10)=0;
            if(was!=last && (last==-1 || was)){last=was;char row[96]{};sprintf_s(row,"{\"event\":\"wall_recovery_cleared\",\"was\":%d}\n",was);Log(row);}}}
    reinterpret_cast<Phase>(image+0xEEE130)(reinterpret_cast<void*>(clock));
    const auto receiver=reinterpret_cast<uintptr_t>(manager);
    *reinterpret_cast<int*>(receiver+0xBC)=1;
    // B8 tracks the original request for renderer binding. Native command
    // recording owns that binding; no original GPU-state routine is called.
    *reinterpret_cast<int*>(receiver+0xB8)=
        (*reinterpret_cast<int*>(receiver+0xC0)==0 && *reinterpret_cast<int*>(receiver+0xC4)==0)?1:0;
    reinterpret_cast<Phase>(image+0xEEE100)(reinterpret_cast<void*>(clock));
}
}  // namespace
