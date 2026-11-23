#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <xmmintrin.h>
#include <array>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cmath>
float Add(float a,float b){return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a),_mm_set_ss(b)));}
float Sub(float a,float b){return _mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(a),_mm_set_ss(b)));}
float Mul(float a,float b){return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a),_mm_set_ss(b)));}
float Div(float a,float b){return _mm_cvtss_f32(_mm_div_ss(_mm_set_ss(a),_mm_set_ss(b)));}
float Formula(float current,float duration,float start,float end,float speed,float delta,int cap,int forward,int reverse,int& loops){
    float length=Sub(Sub(duration,end),start),out=current;loops=0;
    if(length>0){
        out=Add(Sub(current,start),Mul(delta,speed));
        if(out>length){
            loops=static_cast<int>(Div(out,length));
            if(cap<1||static_cast<uint32_t>(forward+loops)<static_cast<uint32_t>(cap))out=Sub(out,Mul(static_cast<float>(loops),length));
            else{out=length;loops=cap-forward-1;}
        }else if(out<0){
            loops=static_cast<int>(Div(out,length))-1;
            if(cap<1||static_cast<uint32_t>(reverse-loops)<static_cast<uint32_t>(cap+1))out=Sub(out,Mul(static_cast<float>(loops),length));
            else{out=0;loops=cap-reverse;}
        }
        out=Add(out,start);
    }
    return _mm_cvtss_f32(_mm_max_ss(_mm_set_ss(out),_mm_setzero_ps()));
}
template<class T>void Put(unsigned char*p,size_t off,T x){std::memcpy(p+off,&x,sizeof x);}
int main(int argc,char**argv){
    if(argc!=2)return 1;FILE*f=nullptr;if(fopen_s(&f,argv[1],"rb"))return 2;
    std::array<unsigned char,313> code{};if(fread(code.data(),1,code.size(),f)!=code.size()||fgetc(f)!=EOF)return 3;fclose(f);
    void* executable=VirtualAlloc(nullptr,code.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!executable)return 4;
    std::memcpy(executable,code.data(),code.size());DWORD old=0;if(!VirtualProtect(executable,code.size(),PAGE_EXECUTE_READ,&old))return 5;
    FlushInstructionCache(GetCurrentProcess(),executable,code.size());
    using Native=void(*)(void*,float,float*,int*);auto native=reinterpret_cast<Native>(executable);
    std::array<unsigned char,0xC0> control{};std::array<unsigned char,0x30> binding{},animation{};
    Put(control.data(),0x38,reinterpret_cast<uintptr_t>(binding.data()));Put(binding.data(),0x18,reinterpret_cast<uintptr_t>(animation.data()));
    unsigned checked=0;
    for(float duration:{0.f,0.25f,2.f})for(float start:{0.f,0.2f})for(float end:{0.f,0.1f})
    for(float current:{-2.f,0.f,0.1f,0.25f,1.f,1.99f,2.f,4.f})for(float speed:{-3.f,-1.f,0.f,0.5f,1.f,2.f})
    for(float delta:{0.f,1.f/60,1.f/90,1.f/120,1.f/165,0.2f})for(int cap:{-1,0,1,2,5})for(int count:{0,1,3}){
        Put(animation.data(),0x14,duration);Put(control.data(),0x10,current);Put(control.data(),0x9c,start);Put(control.data(),0xa0,end);
        Put(control.data(),0x5c,speed);Put(control.data(),0x68,cap);Put(control.data(),0x60,count);Put(control.data(),0x64,count);
        auto before=control;float observed=0;int observedLoops=999;native(control.data(),delta,&observed,&observedLoops);
        int expectedLoops=999;float expected=Formula(current,duration,start,end,speed,delta,cap,count,count,expectedLoops);
        if(std::memcmp(&observed,&expected,4)||observedLoops!=expectedLoops||before!=control){
            std::printf("FAIL case%u observed=%a/%d expected=%a/%d\n",checked,observed,observedLoops,expected,expectedLoops);return 6;
        }checked++;
    }
    VirtualFree(executable,0,MEM_RELEASE);
    std::printf("PASS: %u exact original-kernel versus reconstruction cases; float bits, loop counts, input immutability; MXCSR=%08x. Finite bounded inputs only.\n",checked,_mm_getcsr());return 0;
}
