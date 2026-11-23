#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <array>
#include <atomic>
#include <cstdio>

#include "spatial_debug_config.hpp"

namespace {
std::atomic<DWORD> target_tid{};
std::array<std::atomic<unsigned>,4> hits{};
std::atomic<unsigned> caller_reads{},caller_read_failures{};
HANDLE ready_event{},go_events[2]{},done_events[2]{},exit_event{};
volatile unsigned values[4]{};
constexpr unsigned kIterationsPerArm = 1024;

__declspec(noinline) void First() { values[0]=1; }
__declspec(noinline) void Second() { values[1]=2; }
__declspec(noinline) void Third() { values[2]=3; }
__declspec(noinline) void Fourth() { values[3]=4; }

LONG CALLBACK Handler(EXCEPTION_POINTERS* pointers) {
    if (!pointers || !pointers->ExceptionRecord || !pointers->ContextRecord ||
        pointers->ExceptionRecord->ExceptionCode!=EXCEPTION_SINGLE_STEP ||
        GetCurrentThreadId()!=target_tid.load(std::memory_order_acquire))
        return EXCEPTION_CONTINUE_SEARCH;
    CONTEXT* context=pointers->ContextRecord;
    const DWORD64 mask=context->Dr6&0xFull;
    if (!mask) return EXCEPTION_CONTINUE_SEARCH;
    DWORD64 caller=0;
    __try { caller=*reinterpret_cast<const DWORD64*>(context->Rsp); }
    __except(EXCEPTION_EXECUTE_HANDLER) { ++caller_read_failures; }
    if(caller) ++caller_reads; else ++caller_read_failures;
    for (unsigned slot=0;slot<4;++slot) if (mask&(1ull<<slot)) ++hits[slot];
    context->Dr6=0;
    context->EFlags|=0x10000u;
    return EXCEPTION_CONTINUE_EXECUTION;
}

DWORD WINAPI Target(void*) {
    target_tid.store(GetCurrentThreadId(),std::memory_order_release);
    SetEvent(ready_event);
    for (unsigned arm=0;arm<2;++arm) {
        if (WaitForSingleObject(go_events[arm],10000)!=WAIT_OBJECT_0) return 1;
        for (unsigned iteration=0;iteration<kIterationsPerArm;++iteration) {
            First(); Second(); Third(); Fourth();
        }
        SetEvent(done_events[arm]);
    }
    return WaitForSingleObject(exit_event,10000)==WAIT_OBJECT_0 ? 0 : 2;
}

bool SameDebugState(const CONTEXT& a,const CONTEXT& b) {
    return a.Dr0==b.Dr0&&a.Dr1==b.Dr1&&a.Dr2==b.Dr2&&a.Dr3==b.Dr3&&
        a.Dr6==b.Dr6&&a.Dr7==b.Dr7;
}

bool ArmAndRun(HANDLE thread,const CONTEXT& original,unsigned arm) {
    if(SuspendThread(thread)==static_cast<DWORD>(-1)) return false;
    CONTEXT armed=original; armed.ContextFlags=CONTEXT_DEBUG_REGISTERS;
    armed.Dr0=reinterpret_cast<DWORD64>(&First);
    armed.Dr1=reinterpret_cast<DWORD64>(&Second);
    armed.Dr2=reinterpret_cast<DWORD64>(&Third);
    armed.Dr3=reinterpret_cast<DWORD64>(&Fourth);
    armed.Dr6=0; armed.Dr7=ConfigureSpatialExecuteDr7(armed.Dr7);
    const bool armed_ok=SetThreadContext(thread,&armed)!=FALSE;
    ResumeThread(thread);
    SetEvent(go_events[arm]);
    if(!armed_ok||WaitForSingleObject(done_events[arm],10000)!=WAIT_OBJECT_0) return false;
    if(SuspendThread(thread)==static_cast<DWORD>(-1)) return false;
    CONTEXT current{}; current.ContextFlags=CONTEXT_DEBUG_REGISTERS;
    bool restored=GetThreadContext(thread,&current)!=FALSE;
    current.Dr0=original.Dr0; current.Dr1=original.Dr1;
    current.Dr2=original.Dr2; current.Dr3=original.Dr3;
    current.Dr6=original.Dr6; current.Dr7=original.Dr7;
    restored=restored&&SetThreadContext(thread,&current)!=FALSE;
    CONTEXT final{}; final.ContextFlags=CONTEXT_DEBUG_REGISTERS;
    restored=restored&&GetThreadContext(thread,&final)!=FALSE&&SameDebugState(original,final);
    ResumeThread(thread);
    return restored;
}
}

int main() {
    ready_event=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    for(unsigned arm=0;arm<2;++arm){
        go_events[arm]=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        done_events[arm]=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    }
    exit_event=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    PVOID veh=AddVectoredExceptionHandler(1,Handler);
    HANDLE thread=CreateThread(nullptr,0,Target,nullptr,0,nullptr);
    if(!ready_event||!go_events[0]||!go_events[1]||!done_events[0]||!done_events[1]||!exit_event||!veh||!thread||
       WaitForSingleObject(ready_event,2000)!=WAIT_OBJECT_0) return 1;
    if(SuspendThread(thread)==static_cast<DWORD>(-1)) return 2;
    CONTEXT original{}; original.ContextFlags=CONTEXT_DEBUG_REGISTERS;
    if(!GetThreadContext(thread,&original)) return 3;
    ResumeThread(thread);
    const bool first_restored=ArmAndRun(thread,original,0);
    const bool second_restored=first_restored&&ArmAndRun(thread,original,1);
    SetEvent(exit_event);
    WaitForSingleObject(thread,10000);
    RemoveVectoredExceptionHandler(veh);
    CloseHandle(thread); CloseHandle(ready_event);
    for(unsigned arm=0;arm<2;++arm){CloseHandle(go_events[arm]);CloseHandle(done_events[arm]);}
    CloseHandle(exit_event);
    const unsigned expected=2*kIterationsPerArm;
    const bool counts=hits[0]==expected&&hits[1]==expected&&hits[2]==expected&&hits[3]==expected&&
        caller_reads==4*expected&&caller_read_failures==0&&
        values[0]==1&&values[1]==2&&values[2]==3&&values[3]==4;
    if(!second_restored||!counts){std::fprintf(stderr,"FAIL: restored=%u/%u hits=%u/%u/%u/%u expected=%u\n",
        first_restored?1u:0u,second_restored?1u:0u,hits[0].load(),hits[1].load(),
        hits[2].load(),hits[3].load(),expected);return 6;}
    std::puts("PASS: two 4x1024 execute-breakpoint arms, entry caller reads, RF resume and exact debug-state restoration.");
    return 0;
}
