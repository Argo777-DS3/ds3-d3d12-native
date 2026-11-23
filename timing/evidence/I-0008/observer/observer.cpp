#include "capture.hpp"
#include <tlhelp32.h>
#include <atomic>
#include <vector>
#include <cstdio>

using namespace clock_observer;
namespace {
constexpr unsigned capacity=65536;
Record records[capacity]{};
std::atomic<unsigned> count{},active_handlers{};
std::atomic<bool> recording{};
uintptr_t image;
struct ThreadState{HANDLE handle;DWORD tid;CONTEXT original;};
std::vector<ThreadState> threads;
bool Same(const CONTEXT&a,const CONTEXT&b){return a.Dr0==b.Dr0&&a.Dr1==b.Dr1&&a.Dr2==b.Dr2&&a.Dr3==b.Dr3&&a.Dr6==b.Dr6&&a.Dr7==b.Dr7;}
LONG CALLBACK Handler(EXCEPTION_POINTERS* e){
    if(!e||e->ExceptionRecord->ExceptionCode!=EXCEPTION_SINGLE_STEP)return EXCEPTION_CONTINUE_SEARCH;
    CONTEXT& c=*e->ContextRecord;unsigned slot=4;
    for(unsigned i=0;i<4;i++)if(c.Rip==image+sites[i]&&(c.Dr6&(1ull<<i)))slot=i;
    if(slot==4)return EXCEPTION_CONTINUE_SEARCH;
    active_handlers.fetch_add(1);
    if(recording.load()){
        auto r=Capture(slot,c,image);
        // Getter events are not presumed to belong to the player. Retain their identities.
        if(r.valid){
            unsigned n=count.fetch_add(1);
            if(n<capacity){LARGE_INTEGER q;QueryPerformanceCounter(&q);r.sequence=n;r.qpc=q.QuadPart;
                r.thread=GetCurrentThreadId();records[n]=r;}
            else recording.store(false);
        }
    }
    c.Dr6&=~0xFull;c.EFlags|=0x10000u;
    active_handlers.fetch_sub(1);
    return EXCEPTION_CONTINUE_EXECUTION;
}
bool Restore(){
    bool ok=true;
    for(auto& t:threads){
        if(WaitForSingleObject(t.handle,0)==WAIT_OBJECT_0)continue;
        if(SuspendThread(t.handle)==static_cast<DWORD>(-1)){ok=false;continue;}
        CONTEXT back=t.original;back.ContextFlags=CONTEXT_DEBUG_REGISTERS;
        bool one=SetThreadContext(t.handle,&back)!=FALSE;
        CONTEXT verified{};verified.ContextFlags=CONTEXT_DEBUG_REGISTERS;
        one=one&&GetThreadContext(t.handle,&verified)&&Same(verified,t.original);
        if(ResumeThread(t.handle)==static_cast<DWORD>(-1))one=false;
        ok=one&&ok;
    }
    return ok;
}
bool Arm(){
    HANDLE snap=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);if(snap==INVALID_HANDLE_VALUE)return false;
    THREADENTRY32 entry{};entry.dwSize=sizeof entry;bool ok=Thread32First(snap,&entry)!=FALSE;
    if(ok)do{
        if(entry.th32OwnerProcessID!=GetCurrentProcessId()||entry.th32ThreadID==GetCurrentThreadId())continue;
        HANDLE h=OpenThread(THREAD_SUSPEND_RESUME|THREAD_GET_CONTEXT|THREAD_SET_CONTEXT|SYNCHRONIZE,FALSE,entry.th32ThreadID);
        if(!h){ok=false;break;}
        if(SuspendThread(h)==static_cast<DWORD>(-1)){CloseHandle(h);ok=false;break;}
        CONTEXT old{};old.ContextFlags=CONTEXT_DEBUG_REGISTERS;
        bool ready=GetThreadContext(h,&old)!=FALSE;
        // Existing breakpoints are a hard conflict; never replace them.
        if(!ready||(old.Dr7&0xFF)){ResumeThread(h);CloseHandle(h);ok=false;break;}
        threads.push_back({h,entry.th32ThreadID,old});
        CONTEXT c=old;c.Dr0=image+sites[0];c.Dr1=image+sites[1];c.Dr2=image+sites[2];c.Dr3=image+sites[3];
        c.Dr6=0;c.Dr7=ExecuteDr7(c.Dr7);
        ready=SetThreadContext(h,&c)!=FALSE;
        if(ResumeThread(h)==static_cast<DWORD>(-1))ready=false;
        if(!ready){ok=false;break;}
    }while(Thread32Next(snap,&entry));
    CloseHandle(snap);return ok&&!threads.empty();
}
bool NewThreads(){
    HANDLE snap=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);if(snap==INVALID_HANDLE_VALUE)return true;
    THREADENTRY32 e{};e.dwSize=sizeof e;bool extra=false;
    if(!Thread32First(snap,&e)){CloseHandle(snap);return true;}
    do{if(e.th32OwnerProcessID!=GetCurrentProcessId()||e.th32ThreadID==GetCurrentThreadId())continue;
        bool found=false;for(auto&t:threads)if(t.tid==e.th32ThreadID)found=true;
        if(!found)extra=true;
    }while(Thread32Next(snap,&e));CloseHandle(snap);return extra;
}
bool Bytes(){
    const unsigned char a[]={0xF3,0x0F,0x10,0x81,0x18,1,0,0};
    const unsigned char b[]={0xF3,0x0F,0x10,0x81,0x40,1,0,0};
    const unsigned char c[]={0xF3,0x0F,0x11,0x78,4};
    const unsigned char d[]={0x4C,0x8D,0x5C,0x24,0x70};
    return !std::memcmp(reinterpret_cast<void*>(image+sites[0]),a,sizeof a)&&
        !std::memcmp(reinterpret_cast<void*>(image+sites[1]),b,sizeof b)&&
        !std::memcmp(reinterpret_cast<void*>(image+sites[2]),c,sizeof c)&&
        !std::memcmp(reinterpret_cast<void*>(image+sites[3]),d,sizeof d);
}
DWORD WINAPI Worker(void*){
    // Loading alone does not arm. A separate explicit operator command signals ARM.
    wchar_t arm_name[80],stop_name[80],complete_name[80];
    swprintf_s(arm_name,L"Local\\DS3_I0008_RING_ARM_%lu",GetCurrentProcessId());
    swprintf_s(stop_name,L"Local\\DS3_I0008_RING_STOP_%lu",GetCurrentProcessId());
    swprintf_s(complete_name,L"Local\\DS3_I0008_RING_COMPLETE_%lu",GetCurrentProcessId());
    HANDLE arm=CreateEventW(nullptr,TRUE,FALSE,arm_name);
    HANDLE stop=CreateEventW(nullptr,TRUE,FALSE,stop_name);
    HANDLE complete=CreateEventW(nullptr,TRUE,FALSE,complete_name);
    if(!arm||!stop||!complete)return 1;
    HANDLE waits[]={stop,arm};
    if(WaitForMultipleObjects(2,waits,FALSE,60000)!=WAIT_OBJECT_0+1){CloseHandle(arm);CloseHandle(stop);return 2;}
    image=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    wchar_t path[MAX_PATH]{},temp[MAX_PATH]{};
    DWORD length=GetTempPathW(MAX_PATH,temp);
    if(!length||length>=MAX_PATH||!Bytes()){CloseHandle(arm);CloseHandle(stop);return 3;}
    swprintf_s(path,L"%lsI0008-ring-%lu-%llu.bin",temp,GetCurrentProcessId(),GetTickCount64());
    FILE* file=nullptr;if(_wfopen_s(&file,path,L"wb")||!file)return 4;
    auto veh=AddVectoredExceptionHandler(1,Handler);
    bool armed=veh&&Arm();
    if(armed){recording.store(true);WaitForSingleObject(stop,3000);}
    recording.store(false);
    bool extra=NewThreads();bool restored=Restore();
    // Never remove the handler if restoration failed. The loaded DLL stays inert.
    while(active_handlers.load())Sleep(1);
    if(restored&&veh)RemoveVectoredExceptionHandler(veh);
    for(auto&t:threads)CloseHandle(t.handle);
    LARGE_INTEGER frequency;QueryPerformanceFrequency(&frequency);
    unsigned n=count.load();unsigned kept=n<capacity?n:capacity;
    uint64_t header[]={0x4930303852494E47ull,sizeof(Record),kept,static_cast<uint64_t>(frequency.QuadPart),
        armed?1ull:0ull,restored?1ull:0ull,n>=capacity?1ull:0ull,extra?1ull:0ull,image};
    bool written=fwrite(header,sizeof header,1,file)==1;
    written=fwrite(records,sizeof(Record),kept,file)==kept&&written;
    written=fclose(file)==0&&written;
    if(written)SetEvent(complete);
    CloseHandle(arm);CloseHandle(stop);CloseHandle(complete);return restored&&written?0:5;
}
}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID){
    if(reason==DLL_PROCESS_ATTACH){DisableThreadLibraryCalls(instance);HANDLE h=CreateThread(nullptr,0,Worker,nullptr,0,nullptr);if(h)CloseHandle(h);}
    return TRUE;
}
