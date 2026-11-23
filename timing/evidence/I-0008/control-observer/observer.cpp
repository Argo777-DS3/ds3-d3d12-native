#include "capture.hpp"
#include <tlhelp32.h>
#include <atomic>
#include <vector>
#include <cstdio>
#include <cmath>

using namespace control_observer;
namespace {
constexpr unsigned capacity=65536;
Record records[capacity]{};
std::atomic<unsigned> count{},active_handlers{};
std::atomic<unsigned long long> hits[4]{};
std::atomic<bool> recording{};
uintptr_t image;
struct ThreadState{HANDLE handle;DWORD tid;CONTEXT original;};
std::vector<ThreadState> threads;
struct TimingState{uintptr_t object;float original_fps;unsigned char original_enable;float restored_fps;unsigned char restored_enable;bool applied;bool restored;};
TimingState timing{};
uint32_t FloatBits(float value){uint32_t bits=0;std::memcpy(&bits,&value,sizeof bits);return bits;}
template<class T>bool LocalWrite(uintptr_t at,const T&value){
    __try{*reinterpret_cast<T*>(at)=value;return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool ApplyTiming(){
    uintptr_t object=0;if(!LocalRead(image+0x489DD10,&object,sizeof object)||!object)return false;
    float fps=0;unsigned char enabled=0;
    if(!LocalRead(object+0x354,&fps,sizeof fps)||!LocalRead(object+0x358,&enabled,sizeof enabled))return false;
    timing.object=object;timing.original_fps=fps;timing.original_enable=enabled;
    const float target=165.0f;const unsigned char one=1;
    if(!LocalWrite(object+0x354,target)||!LocalWrite(object+0x358,one))return false;
    float after=0;unsigned char after_enabled=0;
    timing.applied=LocalRead(object+0x354,&after,sizeof after)&&LocalRead(object+0x358,&after_enabled,sizeof after_enabled)&&
        std::fabs(after-target)<0.001f&&after_enabled==one;
    return timing.applied;
}
bool TimingStable(){
    uintptr_t object=0;float fps=0;unsigned char enabled=0;
    return timing.applied&&LocalRead(image+0x489DD10,&object,sizeof object)&&object==timing.object&&
        LocalRead(object+0x354,&fps,sizeof fps)&&LocalRead(object+0x358,&enabled,sizeof enabled)&&
        std::fabs(fps-165.0f)<0.001f&&enabled==1;
}
bool RestoreTiming(){
    if(!timing.object)return false;
    uintptr_t current=0;LocalRead(image+0x489DD10,&current,sizeof current);
    const unsigned char zero=0;
    bool ok=LocalWrite(timing.object+0x358,zero)&&
        LocalWrite(timing.object+0x354,timing.original_fps)&&LocalWrite(timing.object+0x358,timing.original_enable)&&
        LocalRead(timing.object+0x354,&timing.restored_fps,sizeof timing.restored_fps)&&
        LocalRead(timing.object+0x358,&timing.restored_enable,sizeof timing.restored_enable);
    timing.restored=ok&&current==timing.object&&std::fabs(timing.restored_fps-timing.original_fps)<0.001f&&timing.restored_enable==timing.original_enable;
    return timing.restored;
}
bool Same(const CONTEXT&a,const CONTEXT&b){return a.Dr0==b.Dr0&&a.Dr1==b.Dr1&&a.Dr2==b.Dr2&&a.Dr3==b.Dr3&&a.Dr6==b.Dr6&&a.Dr7==b.Dr7;}
LONG CALLBACK Handler(EXCEPTION_POINTERS* e){
    if(!e||e->ExceptionRecord->ExceptionCode!=EXCEPTION_SINGLE_STEP)return EXCEPTION_CONTINUE_SEARCH;
    CONTEXT& c=*e->ContextRecord;unsigned slot=4;
    for(unsigned i=0;i<4;i++)if(c.Rip==image+sites[i]&&(c.Dr6&(1ull<<i)))slot=i;
    if(slot==4)return EXCEPTION_CONTINUE_SEARCH;
    active_handlers.fetch_add(1);hits[slot].fetch_add(1);
    if(recording.load()){
        auto r=Capture(slot,c,image);
        if(r.valid){
            unsigned n=count.fetch_add(1);
            if(n<capacity){LARGE_INTEGER q;QueryPerformanceCounter(&q);r.sequence=n;r.qpc=q.QuadPart;
                r.thread=GetCurrentThreadId();records[n]=r;}
            else recording.store(false);
        }
    }
    c.Dr6&=~0xFull;c.EFlags|=0x10000u;
    active_handlers.fetch_sub(1);return EXCEPTION_CONTINUE_EXECUTION;
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
        CONTEXT old{};old.ContextFlags=CONTEXT_DEBUG_REGISTERS;bool ready=GetThreadContext(h,&old)!=FALSE;
        if(!ready||(old.Dr7&0xFF)){ResumeThread(h);CloseHandle(h);ok=false;break;}
        threads.push_back({h,entry.th32ThreadID,old});
        CONTEXT c=old;c.Dr0=image+sites[0];c.Dr1=image+sites[1];c.Dr2=image+sites[2];c.Dr3=image+sites[3];
        c.Dr6=0;c.Dr7=ExecuteDr7(c.Dr7);ready=SetThreadContext(h,&c)!=FALSE;
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
        bool found=false;for(auto&t:threads)if(t.tid==e.th32ThreadID)found=true;if(!found)extra=true;
    }while(Thread32Next(snap,&e));CloseHandle(snap);return extra;
}
bool Bytes(){
    const unsigned char a[]={0x4C,0x8B,0xDC,0x49,0x89,0x5B,0x18};
    const unsigned char b[]={0x48,0x8B,0x41,0x38,0xF3,0x0F,0x10,0xA1,0x9C,0,0,0};
    const unsigned char c[]={0xC3};
    const unsigned char d[]={0xF3,0x0F,0x11,0x78,0x04};
    return !std::memcmp(reinterpret_cast<void*>(image+sites[0]),a,sizeof a)&&
        !std::memcmp(reinterpret_cast<void*>(image+sites[1]),b,sizeof b)&&
        !std::memcmp(reinterpret_cast<void*>(image+sites[2]),c,sizeof c)&&
        !std::memcmp(reinterpret_cast<void*>(image+sites[3]),d,sizeof d);
}
DWORD WINAPI Worker(void*){
    wchar_t arm_name[96],stop_name[96],complete_name[96];
    swprintf_s(arm_name,L"Local\\DS3_I0008_CONTROL_ARM_%lu",GetCurrentProcessId());
    swprintf_s(stop_name,L"Local\\DS3_I0008_CONTROL_STOP_%lu",GetCurrentProcessId());
    swprintf_s(complete_name,L"Local\\DS3_I0008_CONTROL_COMPLETE_%lu",GetCurrentProcessId());
    HANDLE arm=CreateEventW(nullptr,TRUE,FALSE,arm_name),stop=CreateEventW(nullptr,TRUE,FALSE,stop_name);
    HANDLE complete=CreateEventW(nullptr,TRUE,FALSE,complete_name);if(!arm||!stop||!complete)return 1;
    HANDLE waits[]={stop,arm};
    if(WaitForMultipleObjects(2,waits,FALSE,60000)!=WAIT_OBJECT_0+1){CloseHandle(arm);CloseHandle(stop);CloseHandle(complete);return 2;}
    image=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    wchar_t path[MAX_PATH]{},temp[MAX_PATH]{};DWORD length=GetTempPathW(MAX_PATH,temp);
    if(!length||length>=MAX_PATH||!Bytes()){CloseHandle(arm);CloseHandle(stop);CloseHandle(complete);return 3;}
    swprintf_s(path,L"%lsI0008-control-%lu-%llu.bin",temp,GetCurrentProcessId(),GetTickCount64());
    FILE* file=nullptr;if(_wfopen_s(&file,path,L"wb")||!file)return 4;
    auto veh=AddVectoredExceptionHandler(1,Handler);bool timing_applied=ApplyTiming();
    if(timing_applied)Sleep(1000);
    bool armed=timing_applied&&TimingStable()&&veh&&Arm();
    if(armed){recording.store(true);WaitForSingleObject(stop,3000);}
    recording.store(false);bool extra=NewThreads(),restored=Restore();while(active_handlers.load())Sleep(1);
    if(restored&&veh)RemoveVectoredExceptionHandler(veh);for(auto&t:threads)CloseHandle(t.handle);
    bool timing_restored=RestoreTiming();
    LARGE_INTEGER frequency;QueryPerformanceFrequency(&frequency);unsigned n=count.load(),kept=n<capacity?n:capacity;
    uint64_t header[]={0x493030384354524Cull,sizeof(Record),kept,static_cast<uint64_t>(frequency.QuadPart),
        armed?1ull:0ull,restored?1ull:0ull,n>=capacity?1ull:0ull,extra?1ull:0ull,image,
        hits[0].load(),hits[1].load(),hits[2].load(),hits[3].load(),timing.applied?1ull:0ull,
        timing_restored?1ull:0ull,timing.object,FloatBits(timing.original_fps),timing.original_enable,
        0x43250000ull,1ull,FloatBits(timing.restored_fps),timing.restored_enable};
    bool written=fwrite(header,sizeof header,1,file)==1;
    written=fwrite(records,sizeof(Record),kept,file)==kept&&written;written=fclose(file)==0&&written;
    if(written)SetEvent(complete);CloseHandle(arm);CloseHandle(stop);CloseHandle(complete);return restored&&timing_restored&&written?0:5;
}
}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID){
    if(reason==DLL_PROCESS_ATTACH){DisableThreadLibraryCalls(instance);HANDLE h=CreateThread(nullptr,0,Worker,nullptr,0,nullptr);if(h)CloseHandle(h);}return TRUE;
}
