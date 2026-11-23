#include "capture.hpp"
#include <array>
#include <cstdio>
using namespace clock_observer;
namespace {
std::array<unsigned char,0x2100> player{};
std::array<unsigned char,0x100> world{},modules{},owner{};
uintptr_t image=0x140000000,worldptr=0;
bool ReaderTest(uintptr_t at,void* out,size_t n) {
    if(at==image+0x477FDB8){std::memcpy(out,&worldptr,n);return true;}
    if(at==image+0x489DD10)return false;
    return LocalRead(at,out,n);
}
template<class T> void Put(unsigned char* p,size_t off,T value){std::memcpy(p+off,&value,sizeof value);}
}
int main(){
    worldptr=reinterpret_cast<uintptr_t>(world.data());
    Put(world.data(),0x80,reinterpret_cast<uintptr_t>(player.data()));
    Put(player.data(),0x1F90,reinterpret_cast<uintptr_t>(modules.data()));
    Put(modules.data(),0x10,reinterpret_cast<uintptr_t>(owner.data()));
    Put(owner.data(),0,image+0x2895238);Put(owner.data(),0xC4,0u);
    CONTEXT c{};c.Rbx=reinterpret_cast<uintptr_t>(owner.data());c.Rsi=300020210;
    c.Xmm7.Low=0x3f123456;c.Xmm6.Low=0x3f234567;c.Xmm8.Low=0x40000000;
    unsigned checks=0;
    for(unsigned i=0;i<10;i++){
        Put(owner.data(),0xC0,i);c.Rax=c.Rbx+0x20+16ull*i;
        auto before=owner;auto context=c;auto r=Capture(2,c,image,ReaderTest);
        if(!r.valid||r.index!=i||r.a!=0x3f123456||r.b!=0x3f234567||r.duration!=0x40000000||
            owner!=before||std::memcmp(&c,&context,sizeof c))return 1;
        Put(owner.data(),0x20+16*i,r.id);Put(owner.data(),0x24+16*i,r.a);
        Put(owner.data(),0x28+16*i,r.b);Put(owner.data(),0x2C+16*i,r.duration);
        Put(owner.data(),0xC0,(i+1)%10);auto post=Capture(3,c,image,ReaderTest);
        if(!post.valid||post.destination!=r.destination||post.a!=r.a||post.b!=r.b||post.duration!=r.duration)return 2;
        checks++;
    }
    c.Rax++;if(Capture(2,c,image,ReaderTest).valid)return 3;
    c.Rbx++;if(Capture(3,c,image,ReaderTest).valid)return 4;c.Rbx--;
    Put(owner.data(),0xC0,10u);if(Capture(3,c,image,ReaderTest).valid)return 5;
    Put(owner.data(),0xC0,0u);Put(owner.data(),0,image+1);if(Capture(3,c,image,ReaderTest).valid)return 6;
    std::array<unsigned char,0x150> source{};uint64_t caller=0x140F192A7;
    Put(source.data(),0x118,0x3f765432u);Put(source.data(),0x140,0x3f654321u);
    c.Rcx=reinterpret_cast<uintptr_t>(source.data());c.Rsp=reinterpret_cast<uintptr_t>(&caller);
    if(Capture(0,c,image,ReaderTest).source_time!=0x3f765432||Capture(1,c,image,ReaderTest).source_time!=0x3f654321)return 7;
    c.Rcx=1;if(Capture(0,c,image,ReaderTest).valid)return 8;
    std::printf("PASS: %u ring positions, wrap, bit-exact pre/post values, read-only context/memory, invalid owner/index/address and two getter layouts.\n",checks);
    return 0;
}
