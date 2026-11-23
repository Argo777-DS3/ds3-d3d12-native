#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <cstring>

namespace clock_observer {
constexpr uintptr_t sites[4]={0xF1B070,0x1078330,0x9B4F8D,0x9B4FC3};
struct Record {
    uint64_t sequence,qpc,thread,site,source,caller,player,module,vtable;
    uint64_t destination;
    uint32_t valid,index,previous_index,id,a,b,duration,source_time,central_delta;
    uint32_t ring[40];
};
static_assert(sizeof(Record)==280,"Capture file layout must match analyzer schema");
using Reader=bool(*)(uintptr_t,void*,size_t);
inline bool LocalRead(uintptr_t address,void* out,size_t size) {
    __try { std::memcpy(out,reinterpret_cast<const void*>(address),size); return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
template<class T> inline bool Read(Reader read,uintptr_t at,T& value){return read(at,&value,sizeof value);}
// Observes only. No write to game memory or input registers occurs here.
inline Record Capture(unsigned slot,const CONTEXT& c,uintptr_t image,Reader read=LocalRead) {
    Record r{};r.site=sites[slot];r.source=c.Rcx;
    uintptr_t world=0,modules=0,central=0;
    bool chain=Read(read,image+0x477FDB8,world)&&world&&Read(read,world+0x80,r.player)&&r.player&&
        Read(read,r.player+0x1F90,modules)&&modules&&Read(read,modules+0x10,r.module)&&r.module;
    if(chain) {
        chain=Read(read,r.module,r.vtable)&&r.vtable==image+0x2895238&&
            Read(read,r.module+0xC0,r.index)&&r.index<10&&Read(read,r.module+0xC4,r.previous_index)&&
            r.previous_index<10&&read(r.module+0x20,r.ring,sizeof r.ring);
    }
    if(Read(read,image+0x489DD10,central)&&central)Read(read,central+0x268,r.central_delta);
    if(slot<2) {
        r.valid=Read(read,c.Rcx+(slot==0?0x118:0x140),r.source_time)&&Read(read,c.Rsp,r.caller);
        return r;
    }
    if(!chain||c.Rbx!=r.module)return r;
    if(slot==2) {
        r.destination=c.Rax;
        if(c.Rax!=r.module+0x20+16ull*r.index)return r;
        r.id=static_cast<uint32_t>(c.Rsi);
        r.a=static_cast<uint32_t>(c.Xmm7.Low);r.b=static_cast<uint32_t>(c.Xmm6.Low);
        r.duration=static_cast<uint32_t>(c.Xmm8.Low);
    } else {
        unsigned last=(r.index+9)%10;
        r.destination=r.module+0x20+16ull*last;
        r.id=r.ring[last*4];r.a=r.ring[last*4+1];r.b=r.ring[last*4+2];r.duration=r.ring[last*4+3];
    }
    r.valid=1;return r;
}
inline DWORD64 ExecuteDr7(DWORD64 value) {
    for(unsigned i=0;i<4;i++){value&=~(3ull<<(i*2));value&=~(15ull<<(16+i*4));value|=1ull<<(i*2);}
    return value;
}
}
