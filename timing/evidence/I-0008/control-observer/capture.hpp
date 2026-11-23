#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <cstring>

namespace control_observer {
constexpr uintptr_t sites[4]={0x10768C0,0x1284D70,0x1284EA8,0x9B4F8D};
struct Record {
    uint64_t sequence,qpc,thread,site,object,context,control,out_ptr,loops_ptr;
    uint64_t player,module,object_vtable,control_vtable,destination,caller;
    uint32_t valid,delta,central_delta,generator_time,control_current,speed,start,end;
    uint32_t duration,mode,flags,loop_limit,forward_count,reverse_count,out_time,out_loops;
    uint32_t id,ring_a,ring_b,ring_duration;
};
static_assert(sizeof(Record)==200,"Capture schema changed");
using Reader=bool(*)(uintptr_t,void*,size_t);
inline bool LocalRead(uintptr_t at,void*out,size_t size){
    __try{std::memcpy(out,reinterpret_cast<const void*>(at),size);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
template<class T>inline bool Read(Reader read,uintptr_t at,T&value){return read(at,&value,sizeof value);}
inline bool ReadControl(Record&r,Reader read){
    uintptr_t binding=0,asset=0;
    return r.control&&Read(read,r.control,r.control_vtable)&&
        (r.control_vtable==0x2A5D758+r.destination||r.control_vtable==0x2841A08+r.destination)&&
        Read(read,r.control+0x10,r.control_current)&&Read(read,r.control+0x5C,r.speed)&&
        Read(read,r.control+0x9C,r.start)&&Read(read,r.control+0xA0,r.end)&&
        Read(read,r.control+0x60,r.forward_count)&&Read(read,r.control+0x64,r.reverse_count)&&
        Read(read,r.control+0x68,r.loop_limit)&&Read(read,r.control+0x38,binding)&&binding&&
        Read(read,binding+0x18,asset)&&asset&&Read(read,asset+0x14,r.duration);
}
// destination temporarily carries image base while validating control vtables.
inline Record Capture(unsigned slot,const CONTEXT&c,uintptr_t image,Reader read=LocalRead){
    Record r{};r.site=sites[slot];r.destination=image;
    uintptr_t world=0,modules=0,central=0;
    Read(read,image+0x477FDB8,world);if(world)Read(read,world+0x80,r.player);
    if(r.player&&Read(read,r.player+0x1F90,modules)&&modules)Read(read,modules+0x10,r.module);
    if(Read(read,image+0x489DD10,central)&&central)Read(read,central+0x268,r.central_delta);
    if(slot==0){
        r.object=c.Rcx;r.context=c.Rdx;r.control=0;r.delta=static_cast<uint32_t>(c.Xmm2.Low);
        if(!Read(read,r.object,r.object_vtable)||r.object_vtable!=image+0x2A0B9A8||
            !Read(read,r.object+0xD0,r.control)||!Read(read,r.object+0x140,r.generator_time))return r;
        uint8_t mode=0,flags=0;Read(read,r.object+0xBE,mode);Read(read,r.object+0xBF,flags);
        r.mode=mode;r.flags=flags;
        if(!ReadControl(r,read))return r;r.destination=0;r.valid=1;return r;
    }
    if(slot==1||slot==2){
        r.control=slot==1?c.Rcx:c.R10;
        if(slot==1)r.delta=static_cast<uint32_t>(c.Xmm1.Low);
        r.out_ptr=c.R8;r.loops_ptr=c.R9;if(!ReadControl(r,read))return r;
        if(slot==1){Read(read,c.Rsp,r.caller);r.destination=0;r.valid=1;return r;}
        if(!Read(read,r.out_ptr,r.out_time)||!Read(read,r.loops_ptr,r.out_loops))return r;
        r.destination=0;r.valid=1;return r;
    }
    if(!r.module)return r;
    uint64_t module_vtable=0;uint32_t index=0;
    if(!Read(read,r.module,module_vtable)||module_vtable!=image+0x2895238||
        !Read(read,r.module+0xC0,index)||index>=10||c.Rbx!=r.module||
        c.Rax!=r.module+0x20+16ull*index)return r;
    r.destination=c.Rax;r.id=static_cast<uint32_t>(c.Rsi);
    r.ring_a=static_cast<uint32_t>(c.Xmm7.Low);r.ring_b=static_cast<uint32_t>(c.Xmm6.Low);
    r.ring_duration=static_cast<uint32_t>(c.Xmm8.Low);r.forward_count=index;
    r.valid=1;return r;
}
inline DWORD64 ExecuteDr7(DWORD64 value){for(unsigned i=0;i<4;i++){value&=~(3ull<<(i*2));value&=~(15ull<<(16+i*4));value|=1ull<<(i*2);}return value;}
}
