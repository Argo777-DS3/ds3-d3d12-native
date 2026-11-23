// W4 FW-1 T-F4 (FFX-WAVE1-READY section 7): the observer's histograms, rows and order counters, and the signature-free
// thunk: integer, floating-point and stack arguments reach the observed function unchanged, the return value comes back,
// the observer decodes the creator's identifier, flags and +0x58 branch, and the unwinder finds the thunk's frame.
#include "ffx_observe.h"
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace ds3native;
namespace {
int failures{};
std::vector<std::string> rows;
void Expect(bool condition,const char* what) {if(!condition){++failures;std::printf("FAIL %s\n",what);}}
bool TestRead(uintptr_t p,void* out,size_t size) noexcept {   // guarded, as the module's Read (the light vector row reads image-relative addresses)
    if(p<0x10000)return false;__try{std::memcpy(out,reinterpret_cast<void*>(p),size);}__except(EXCEPTION_EXECUTE_HANDLER){return false;}return true;}
void TestLog(const char* text) noexcept {rows.emplace_back(text);}
__declspec(noinline) double Create(uint64_t self,double x,const uint32_t* id,double y,uint32_t flags) {
    return x*y+*id+flags+(self?1000.0:0.0);}
__declspec(noinline) double Floats(double a,double b,double c,double d) {return a+b*10+c*100+d*1000;}
__declspec(noinline) uint64_t Six(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f) {return a+b*2+c*3+d*4+e*5+f*6;}
}
int main() {
    {ffx_observe::Histogram<2> h;h.Add(5);h.Add(5);h.Add(7);h.Add(9);h.Add(9);
        Expect(h.used==2 && h.keys[0]==5 && h.counts[0]==2 && h.keys[1]==7 && h.counts[1]==1 && h.other==2,"bounded histogram");
        h.Clear();Expect(h.used==0 && h.other==0,"histogram clear");}
    ffx_observe::Configure(0,&TestRead,&TestLog,"test");
    // A creator: rcx = SprjSfxImp (+0x58 set), XMM1 and XMM3 floats, r8 = &id, the flags on the stack.
    {std::array<uint64_t,16> sfx{};sfx[0x58/8]=1;const uint32_t id=800081;
        *ffx_observe::Original(ffx_observe::SingleCreate)=reinterpret_cast<void*>(&Create);
        auto* thunk=reinterpret_cast<double(*)(uint64_t,double,const uint32_t*,double,uint32_t)>(ffx_observe::Thunk(ffx_observe::SingleCreate));
        Expect(thunk!=nullptr,"thunk page");
        if(thunk){const double direct=Create(reinterpret_cast<uint64_t>(sfx.data()),1.5,&id,2.25,8);
            const double through=thunk(reinterpret_cast<uint64_t>(sfx.data()),1.5,&id,2.25,8);
            Expect(direct==through,"creator arguments and result through the thunk");
            const auto w=ffx_observe::Snapshot(ffx_observe::SingleCreate);
            Expect(w.calls==1 && w.system==1,"creator call and +0x58 branch");
            Expect(w.ids.used==1 && w.ids.keys[0]==800081,"creator identifier");
            Expect(w.flags.used==1 && w.flags.keys[0]==8,"creator flags from the stack");
            Expect(w.callers.used==1 && w.callers.keys[0]!=0,"creator return address");
            DWORD64 base{};Expect(RtlLookupFunctionEntry(reinterpret_cast<DWORD64>(thunk)+0x20,&base,nullptr)!=nullptr,"thunk unwind entry");}}
    // XMM0-XMM3 and six integer arguments (two on the stack) through entries whose observer reads nothing.
    {*ffx_observe::Original(ffx_observe::BucketAppend)=reinterpret_cast<void*>(&Floats);
        auto* f=reinterpret_cast<double(*)(double,double,double,double)>(ffx_observe::Thunk(ffx_observe::BucketAppend));
        if(f)Expect(f(1.25,2.5,3.75,4.5)==Floats(1.25,2.5,3.75,4.5),"four float arguments through the thunk");
        *ffx_observe::Original(ffx_observe::WindEval)=reinterpret_cast<void*>(&Six);
        auto* s=reinterpret_cast<uint64_t(*)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t)>(ffx_observe::Thunk(ffx_observe::WindEval));
        if(s)Expect(s(1,2,3,4,5,6)==Six(1,2,3,4,5,6),"six integer arguments through the thunk");
        Expect(ffx_observe::Snapshot(ffx_observe::BucketAppend).calls==1 && ffx_observe::Snapshot(ffx_observe::WindEval).calls==1,"pass-through counts");}
    // Rows: frames 1-3, then every 600th; order of the effect update and the light collect.
    rows.clear();
    ffx_observe::Note(ffx_observe::GraphicsStep,0x1000,0,0,0,0);
    Expect(rows.size()==1 && rows[0].find("\"single_create\":{\"calls\":1,\"system\":1")!=std::string::npos &&
        rows[0].find("\"ids\":[[800081,1]]")!=std::string::npos,"first row carries the window");
    ffx_observe::Note(ffx_observe::SfxUpdate,0x2000,0,0,0,0);
    ffx_observe::Note(ffx_observe::LightCollect,0x3000,0,0,0,0);
    ffx_observe::Note(ffx_observe::LightCollect,0x3000,0,0,0,0);
    ffx_observe::Note(ffx_observe::WorldRequest,0x4000,0,30001,0,8);
    ffx_observe::Note(ffx_observe::GraphicsStep,0x1000,0,0,0,0);
    Expect(rows.size()==2 && rows[1].find("\"update_before_collect\":1,\"collect_without_update\":1")!=std::string::npos,"update and collect order");
    Expect(rows[1].find("\"world_request\":{\"calls\":1,\"callers\":[[\"4000\",1]],\"ids\":[[30001,1]],\"flags\":[[8,1]]}")!=std::string::npos,"world request row");
    for(int i=0;i<597;++i)ffx_observe::Note(ffx_observe::GraphicsStep,0x1000,0,0,0,0);
    Expect(rows.size()==3,"third row at frame 3");
    ffx_observe::Note(ffx_observe::GraphicsStep,0x1000,0,0,0,0);
    Expect(rows.size()==4 && rows[3].find("\"frame\":600,\"window_frames\":597")!=std::string::npos,"row at frame 600");
    std::printf("ffx_observe_test: %s (%d failures)\n",failures?"FAILED":"passed",failures);
    return failures?1:0;
}
