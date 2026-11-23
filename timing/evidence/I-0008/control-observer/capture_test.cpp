#include "capture.hpp"
#include <array>
#include <cstdio>
using namespace control_observer;
namespace{
uintptr_t image=0x140000000,world_ptr=0,central_ptr=0;
std::array<unsigned char,0x2100>player{};std::array<unsigned char,0x100>world{},modules{};
std::array<unsigned char,0xD8>ring{};std::array<unsigned char,0x160>generator{};
std::array<unsigned char,0xC0>control{};std::array<unsigned char,0x30>binding{},asset{},central{};
template<class T>void Put(unsigned char*p,size_t at,T value){std::memcpy(p+at,&value,sizeof value);}
bool ReadTest(uintptr_t at,void*out,size_t size){
    if(at==image+0x477FDB8){std::memcpy(out,&world_ptr,size);return true;}
    if(at==image+0x489DD10){std::memcpy(out,&central_ptr,size);return true;}
    return LocalRead(at,out,size);
}
}
int main(){
    world_ptr=reinterpret_cast<uintptr_t>(world.data());central_ptr=reinterpret_cast<uintptr_t>(central.data());
    Put(world.data(),0x80,reinterpret_cast<uintptr_t>(player.data()));Put(player.data(),0x1F90,reinterpret_cast<uintptr_t>(modules.data()));
    Put(modules.data(),0x10,reinterpret_cast<uintptr_t>(ring.data()));Put(ring.data(),0,image+0x2895238);Put(ring.data(),0xC0,3u);
    Put(generator.data(),0,image+0x2A0B9A8);Put(generator.data(),0xD0,reinterpret_cast<uintptr_t>(control.data()));Put(generator.data(),0x140,0x3f100000u);
    Put(control.data(),0,image+0x2A5D758);Put(control.data(),0x10,0x3f100000u);Put(control.data(),0x5C,0x3f800000u);
    Put(control.data(),0x9C,0u);Put(control.data(),0xA0,0u);Put(control.data(),0x60,0u);Put(control.data(),0x64,0u);Put(control.data(),0x68,0xffffffffu);
    Put(control.data(),0x38,reinterpret_cast<uintptr_t>(binding.data()));Put(binding.data(),0x18,reinterpret_cast<uintptr_t>(asset.data()));Put(asset.data(),0x14,0x40000000u);
    Put(central.data(),0x268,0x3c088853u);
    CONTEXT c{};c.Rcx=reinterpret_cast<uintptr_t>(generator.data());c.Rdx=0x12345678;c.Xmm2.Low=0x3c088853;
    auto r0=Capture(0,c,image,ReadTest);if(!r0.valid||r0.control!=reinterpret_cast<uintptr_t>(control.data())||r0.delta!=r0.central_delta)return 1;
    float out=0;int loops=0;c.Rcx=reinterpret_cast<uintptr_t>(control.data());c.Xmm1.Low=0x3c088853;c.R8=reinterpret_cast<uintptr_t>(&out);c.R9=reinterpret_cast<uintptr_t>(&loops);
    auto r1=Capture(1,c,image,ReadTest);if(!r1.valid||r1.control_current!=0x3f100000)return 2;
    out=0.5833333f;c.R10=c.Rcx;uint32_t out_bits=0;std::memcpy(&out_bits,&out,4);
    auto r2=Capture(2,c,image,ReadTest);if(!r2.valid||r2.out_time!=out_bits||r2.out_loops)return 3;
    c.Rbx=reinterpret_cast<uintptr_t>(ring.data());c.Rax=c.Rbx+0x50;c.Rsi=0;c.Xmm7.Low=0x3f100000;c.Xmm6.Low=r2.out_time;c.Xmm8.Low=0x40000000;
    auto r3=Capture(3,c,image,ReadTest);if(!r3.valid||r3.ring_a!=r1.control_current||r3.ring_b!=r2.out_time)return 4;
    c.Rax++;if(Capture(3,c,image,ReadTest).valid)return 5;c.Rax--;
    Put(generator.data(),0,image+1);c.Rcx=reinterpret_cast<uintptr_t>(generator.data());if(Capture(0,c,image,ReadTest).valid)return 6;
    std::puts("PASS: generator/control/exit/ring capture, exact identities and values, invalid vtable/destination rejection.");return 0;
}
