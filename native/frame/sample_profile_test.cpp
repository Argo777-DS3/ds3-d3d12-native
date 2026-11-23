// m45 frame cost: the sampler on a known call chain. A thread spins in SampleTestLeaf, called by SampleTestMiddle; the
// sampler takes 400 samples of it, and most samples must hold the leaf with the middle as its caller (the copy unwound).
// I-0074: with a frame mark and a second thread added after the start. The build then names the same file with sample_symbolize.exe and finds both functions in the report.
#include "sample_profile.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

std::atomic<bool> stop_spinning{};
std::atomic<uint64_t> sink{};
extern "C" __declspec(noinline) void SampleTestLeaf(){uint64_t x=1;while(!stop_spinning.load(std::memory_order_relaxed)){for(int i=0;i<1000;++i)x=x*6364136223846793005ull+1;sink.store(x,std::memory_order_relaxed);}}
extern "C" __declspec(noinline) void SampleTestMiddle(){volatile char pad[256]{};pad[0]=1;SampleTestLeaf();sink.fetch_add(pad[0]);}

int wmain(int argc,wchar_t** argv){
    if(argc<2){std::fprintf(stderr,"usage: sample_profile_test <output.txt>\n");return 2;}
    DeleteFileW(argv[1]);   // the wait below must read the file this run's sampler writes, not an earlier run's
    std::atomic<DWORD> id{},second{};std::thread spinner([&]{id=GetCurrentThreadId();SampleTestMiddle();});
    // I-0074: a second spinner announced with AddThread after Start, sampled from the next tick under its own name.
    std::thread added([&]{second=GetCurrentThreadId();SampleTestMiddle();});
    while(!id.load() || !second.load())Sleep(1);
    const std::atomic<uint64_t> active{1};std::atomic<uint64_t> mark{7};
    const ds3native::sampling::Thread threads[]{{id.load(),"spinner"}};
    if(!ds3native::sampling::Start(argv[1],threads,active,400,&mark) || !ds3native::sampling::AddThread(second.load(),"added")){
        std::puts("FAIL sampler did not start");stop_spinning=true;spinner.join();added.join();return 1;}
    Sleep(100);mark.store(8);   // the mark each sample records moves while the sampler runs
    std::ifstream in;for(int wait=0;wait<200;++wait){Sleep(50);in.open(argv[1]);std::string first;if(in && std::getline(in,first)){in.seekg(0);break;}in.close();}
    Sleep(200);   // the writer closes the file after its last line
    stop_spinning=true;spinner.join();added.join();
    in.close();in.open(argv[1]);
    const auto range=[](const void* function){const auto at=reinterpret_cast<uint64_t>(function);return std::pair<uint64_t,uint64_t>{at,at+0x100};};
    const auto leaf=range(reinterpret_cast<const void*>(&SampleTestLeaf)),middle=range(reinterpret_cast<const void*>(&SampleTestMiddle));
    uint64_t first_pc{};
    size_t samples{},in_leaf{},with_caller{},modules{},second_samples{},marked{},marks_seen{};std::string line;bool saw7{},saw8{};
    while(std::getline(in,line)){std::istringstream fields(line);std::string kind;fields>>kind;
        if(kind=="module")++modules;
        if(kind!="s")continue;size_t thread{};fields>>thread;
        if(fields.peek()==' ')fields.get();
        if(fields.peek()=='@'){fields.get();uint64_t value{};fields>>value;++marked;saw7|=value==7;saw8|=value==8;}
        std::vector<uint64_t> stack;uint64_t pc{};while(fields>>std::hex>>pc)stack.push_back(pc);
        if(thread==1){++second_samples;continue;}
        ++samples;
        if(!first_pc && !stack.empty())first_pc=stack[0];
        if(!stack.empty() && stack[0]>=leaf.first && stack[0]<leaf.second){++in_leaf;
            for(size_t i=1;i<stack.size();++i)if(stack[i]>middle.first && stack[i]<=middle.second){++with_caller;break;}}}
    marks_seen=size_t(saw7)+size_t(saw8);
    std::printf("sampler: %zu modules, %zu samples, %zu in the leaf, %zu of them with the caller (leaf at %llx, first pc %llx); added thread %zu samples; %zu marked, %zu mark values\n",modules,samples,in_leaf,with_caller,
        static_cast<unsigned long long>(leaf.first),static_cast<unsigned long long>(first_pc),second_samples,marked,marks_seen);
    const bool ok=modules>0 && samples==400 && in_leaf>=300 && with_caller>=in_leaf*9/10 && second_samples>=300 && marked==samples+second_samples && marks_seen==2;
    std::puts(ok?"PASS sampler":"FAIL sampler");
    return ok?0:1;
}
