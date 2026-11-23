// Real Windows hardware-breakpoint/restoration test using synthetic executable memory only.
// It never opens, searches for or starts Dark Souls III.
#include "observer.cpp"
namespace {
HANDLE ready,go,done,quit_event;
DWORD WINAPI Synthetic(void*){
    SetEvent(ready);WaitForSingleObject(go,10000);using Fn=void(*)();
    for(unsigned n=0;n<256;n++)for(unsigned i=0;i<4;i++)reinterpret_cast<Fn>(image+sites[i])();
    SetEvent(done);WaitForSingleObject(quit_event,10000);return 0;
}
}
int main(){
    constexpr size_t size=0x489E000;image=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(!image)return 1;
    for(unsigned i=0;i<4;i++)*reinterpret_cast<unsigned char*>(image+sites[i])=0xC3;
    for(unsigned i=0;i<4;i++){DWORD old=0;if(!VirtualProtect(reinterpret_cast<void*>(image+sites[i]),16,PAGE_EXECUTE_READ,&old))return 2;}
    FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(image),size);
    const uintptr_t central=image+0x2000;*reinterpret_cast<uintptr_t*>(image+0x489DD10)=central;
    *reinterpret_cast<float*>(central+0x354)=30.0f;*reinterpret_cast<unsigned char*>(central+0x358)=0;
    timing={};if(!ApplyTiming()||!TimingStable()||*reinterpret_cast<float*>(central+0x354)!=165.0f||*reinterpret_cast<unsigned char*>(central+0x358)!=1)return 3;
    if(!RestoreTiming()||*reinterpret_cast<float*>(central+0x354)!=30.0f||*reinterpret_cast<unsigned char*>(central+0x358)!=0)return 4;
    auto veh=AddVectoredExceptionHandler(1,Handler);if(!veh)return 5;
    for(unsigned run=0;run<2;run++){
        ready=CreateEventW(nullptr,TRUE,FALSE,nullptr);go=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        done=CreateEventW(nullptr,TRUE,FALSE,nullptr);quit_event=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        HANDLE target=CreateThread(nullptr,0,Synthetic,nullptr,0,nullptr);if(!target||WaitForSingleObject(ready,10000)!=WAIT_OBJECT_0)return 6;
        count.store(0);for(auto&hit:hits)hit.store(0);if(!Arm())return 7;
        recording.store(true);SetEvent(go);if(WaitForSingleObject(done,10000)!=WAIT_OBJECT_0)return 8;
        recording.store(false);bool restored=Restore();while(active_handlers.load())Sleep(1);
        if(!restored||count.load()!=0)return 9;for(auto&hit:hits)if(hit.load()!=256)return 10;
        for(auto&t:threads)CloseHandle(t.handle);threads.clear();SetEvent(quit_event);WaitForSingleObject(target,10000);CloseHandle(target);
        CloseHandle(ready);CloseHandle(go);CloseHandle(done);CloseHandle(quit_event);
    }
    RemoveVectoredExceptionHandler(veh);VirtualFree(reinterpret_cast<void*>(image),0,MEM_RELEASE);
    std::puts("PASS: timing 30/0 -> 165/1 -> 30/0, two hardware-breakpoint arms, all four sites, RF resume and exact debug-state restoration.");return 0;
}
