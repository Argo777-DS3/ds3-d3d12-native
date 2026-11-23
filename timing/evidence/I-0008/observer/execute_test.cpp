// Real Windows hardware-breakpoint test of the same controller and VEH as the DLL.
// Uses synthetic executable memory only; never opens or searches for DS3.
#include "observer.cpp"
#include <array>
namespace {
HANDLE ready,go,done,quit;
std::array<unsigned char,0x150> source{};
DWORD WINAPI Synthetic(void*){
    SetEvent(ready);WaitForSingleObject(go,10000);
    using Fn=void(*)(void*);
    for(unsigned n=0;n<256;n++)for(unsigned i=0;i<4;i++)reinterpret_cast<Fn>(image+sites[i])(source.data());
    SetEvent(done);WaitForSingleObject(quit,10000);return 0;
}
}
int main(){
    constexpr size_t size=0x489E000;
    image=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    if(!image)return 1;
    const unsigned char a[]={0xF3,0x0F,0x10,0x81,0x18,1,0,0,0xC3};
    const unsigned char b[]={0xF3,0x0F,0x10,0x81,0x40,1,0,0,0xC3};
    std::memcpy(reinterpret_cast<void*>(image+sites[0]),a,sizeof a);
    std::memcpy(reinterpret_cast<void*>(image+sites[1]),b,sizeof b);
    *reinterpret_cast<unsigned char*>(image+sites[2])=0xC3;
    *reinterpret_cast<unsigned char*>(image+sites[3])=0xC3;
    for(unsigned i=0;i<4;i++){DWORD old=0;if(!VirtualProtect(reinterpret_cast<void*>(image+sites[i]),16,PAGE_EXECUTE_READ,&old))return 2;}
    FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(image),size);
    uint32_t t0=0x3f123456,t1=0x3f654321;
    std::memcpy(source.data()+0x118,&t0,4);std::memcpy(source.data()+0x140,&t1,4);
    auto veh=AddVectoredExceptionHandler(1,Handler);if(!veh)return 3;
    for(unsigned run=0;run<2;run++){
        ready=CreateEventW(nullptr,TRUE,FALSE,nullptr);go=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        done=CreateEventW(nullptr,TRUE,FALSE,nullptr);quit=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        HANDLE target=CreateThread(nullptr,0,Synthetic,nullptr,0,nullptr);
        if(!target||WaitForSingleObject(ready,10000)!=WAIT_OBJECT_0)return 4;
        count.store(0);if(!Arm())return 5;
        recording.store(true);SetEvent(go);if(WaitForSingleObject(done,10000)!=WAIT_OBJECT_0)return 6;
        recording.store(false);bool restored=Restore();while(active_handlers.load())Sleep(1);
        if(!restored||count.load()!=512)return 7;
        for(unsigned i=0;i<512;i++)if(records[i].source_time!=(i%2?t1:t0)||!records[i].caller)return 8;
        for(auto&t:threads)CloseHandle(t.handle);threads.clear();
        SetEvent(quit);WaitForSingleObject(target,10000);CloseHandle(target);
        CloseHandle(ready);CloseHandle(go);CloseHandle(done);CloseHandle(quit);
    }
    RemoveVectoredExceptionHandler(veh);VirtualFree(reinterpret_cast<void*>(image),0,MEM_RELEASE);
    std::puts("PASS: actual DLL controller/VEH, two 4x256 execute arms, 1024 bit-exact getter captures, caller reads, RF resume, invalid ring rejection and verified exact debug-state restoration.");
    return 0;
}
