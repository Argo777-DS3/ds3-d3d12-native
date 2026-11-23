#include "sample_profile.h"
#include <psapi.h>
#include <winternl.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

// Output (text, one record per line):
//   module <base hex> <size hex> <path>
//   thread <index> <name> <samples> <failures>
//   s <thread index> <pc hex> <caller pc hex> ...      (innermost first, at most max_depth frames)
//   s <thread index> @<mark> <pc hex> ...              (with a mark: its decimal value when the sample was taken)
namespace ds3native::sampling {
namespace {
constexpr uint32_t max_depth=48;
constexpr size_t max_stack=1024*1024;
struct Sampled {HANDLE handle{};const char* name{};uintptr_t stack_base{};std::vector<uint64_t> records;uint32_t samples{};uint64_t failures{};};
struct Sampler {
    std::wstring output;std::vector<Sampled> threads;const std::atomic<uint64_t>* active{};const std::atomic<uint64_t>* mark{};uint64_t end_mark{~0ull};uint32_t limit{};size_t initial{};   // initial: the threads Start named; I-0079: end_mark, the mark that ends sampling
    std::unique_ptr<uint8_t[]> copy;
};
// I-0074: the threads AddThread announced; the sampler loop opens each once (`taken` counts those it has seen).
struct Added {std::atomic<DWORD> id{};const char* name{};};
std::array<Added,32> added{};std::atomic<uint32_t> added_count{};
bool Open(Sampled& sampled,DWORD id,const char* name,uint32_t limit) noexcept;
// The thread's stack base (NT_TIB of its TEB), from ThreadBasicInformation.
uintptr_t StackBaseOf(HANDLE thread) noexcept {
    struct Basic {LONG exit_status;PVOID teb;CLIENT_ID client;ULONG_PTR affinity;LONG priority,base_priority;} basic{};
    using Query=LONG(NTAPI*)(HANDLE,ULONG,PVOID,ULONG,PULONG);
    const auto query=reinterpret_cast<Query>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"NtQueryInformationThread"));
    if(!query || query(thread,0,&basic,sizeof basic,nullptr)<0 || !basic.teb)return 0;
    return reinterpret_cast<uintptr_t>(static_cast<NT_TIB*>(basic.teb)->StackBase);
}
// While the thread is suspended: its used stack [Rsp, base) into the copy. No allocation, no lock.
bool CopyStack(uintptr_t rsp,uintptr_t base,uint8_t* copy,size_t& bytes) noexcept {
    if(rsp>=base || base-rsp>max_stack)return false;
    bytes=static_cast<size_t>(base-rsp);
    __try{std::memcpy(copy,reinterpret_cast<const void*>(rsp),bytes);}__except(EXCEPTION_EXECUTE_HANDLER){return false;}
    return true;
}
// After the resume: unwinds the copy. Words and registers that point into the original stack are moved to the copy first,
// so the unwinder's reads land there. Returns the number of frames written to `out`.
uint32_t UnwindCopy(CONTEXT context,uintptr_t rsp,uintptr_t base,uint8_t* copy,size_t bytes,uint64_t* out) noexcept {
    const auto delta=static_cast<int64_t>(reinterpret_cast<uintptr_t>(copy))-static_cast<int64_t>(rsp);
    const auto move=[&](DWORD64& value){if(value>=rsp && value<base)value=static_cast<DWORD64>(static_cast<int64_t>(value)+delta);};
    for(size_t at=0;at+8<=bytes;at+=8){DWORD64 word;std::memcpy(&word,copy+at,8);if(word>=rsp && word<base){move(word);std::memcpy(copy+at,&word,8);}}
    for(DWORD64* reg:{&context.Rax,&context.Rcx,&context.Rdx,&context.Rbx,&context.Rsp,&context.Rbp,&context.Rsi,&context.Rdi,
                      &context.R8,&context.R9,&context.R10,&context.R11,&context.R12,&context.R13,&context.R14,&context.R15})move(*reg);
    const auto low=reinterpret_cast<uintptr_t>(copy),high=low+bytes;
    uint32_t depth=0;
    __try{
        while(depth<max_depth){
            out[depth++]=context.Rip;
            DWORD64 image{};const auto entry=RtlLookupFunctionEntry(context.Rip,&image,nullptr);
            if(entry){PVOID handler{};DWORD64 frame{};RtlVirtualUnwind(UNW_FLAG_NHANDLER,image,context.Rip,entry,&context,&handler,&frame,nullptr);}
            else{if(context.Rsp<low || context.Rsp+8>high)break;std::memcpy(&context.Rip,reinterpret_cast<const void*>(context.Rsp),8);context.Rsp+=8;}
            if(!context.Rip || context.Rsp<low || context.Rsp>=high)break;
        }
    }__except(EXCEPTION_EXECUTE_HANDLER){}
    return depth;
}
void Write(const Sampler& s) noexcept {
    FILE* file{};if(_wfopen_s(&file,s.output.c_str(),L"wb") || !file)return;
    std::vector<HMODULE> modules(1024);DWORD needed{};
    if(EnumProcessModules(GetCurrentProcess(),modules.data(),static_cast<DWORD>(modules.size()*sizeof(HMODULE)),&needed)){
        modules.resize(std::min<size_t>(modules.size(),needed/sizeof(HMODULE)));
        for(const auto module:modules){MODULEINFO info{};char path[MAX_PATH]{};
            if(GetModuleInformation(GetCurrentProcess(),module,&info,sizeof info) && GetModuleFileNameA(module,path,MAX_PATH))
                std::fprintf(file,"module %llx %lx %s\n",static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(info.lpBaseOfDll)),info.SizeOfImage,path);}}
    for(size_t t=0;t<s.threads.size();++t){const auto& thread=s.threads[t];
        std::fprintf(file,"thread %zu %s %u %llu\n",t,thread.name,thread.samples,static_cast<unsigned long long>(thread.failures));
        for(size_t at=0;at<thread.records.size();){const auto depth=thread.records[at++];std::fprintf(file,"s %zu",t);
            if(s.mark && at<thread.records.size())std::fprintf(file," @%llu",static_cast<unsigned long long>(thread.records[at++]));
            for(uint64_t i=0;i<depth && at<thread.records.size();++i)std::fprintf(file," %llx",static_cast<unsigned long long>(thread.records[at++]));
            std::fputc('\n',file);}}
    std::fclose(file);
}
thread_local bool sampler_thread{};
DWORD WINAPI Loop(void* parameter) noexcept {
    sampler_thread=true;
    std::unique_ptr<Sampler> s(static_cast<Sampler*>(parameter));
    SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
    const HANDLE timer=CreateWaitableTimerExW(nullptr,nullptr,CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,TIMER_ALL_ACCESS);
    LARGE_INTEGER due{};due.QuadPart=-10000;
    if(!timer || !SetWaitableTimer(timer,&due,1,nullptr,nullptr,FALSE))return 0;
    std::array<uint64_t,max_depth> frames{};uint32_t taken{};
    for(;;){
        WaitForSingleObject(timer,INFINITE);
        if(!s->active->load(std::memory_order_relaxed))continue;
        for(const auto count=std::min<uint32_t>(added_count.load(std::memory_order_acquire),uint32_t(added.size()));taken<count;++taken){
            const DWORD id=added[taken].id.load(std::memory_order_acquire);if(!id)break;   // announced, not yet written: next tick
            Sampled sampled;if(Open(sampled,id,added[taken].name,s->limit))s->threads.push_back(std::move(sampled));}
        const uint64_t mark=s->mark?s->mark->load(std::memory_order_relaxed):0;
        if(s->mark && mark>=s->end_mark)break;   // I-0079: the frame mark reached the end the caller chose
        bool done=true;
        // I-0074: the threads Start named decide the end; an added thread is sampled while they run (it may end short of the limit).
        for(size_t index=0;index<s->threads.size();++index){auto& thread=s->threads[index];
            if(thread.samples>=s->limit || thread.failures>=1000)continue;if(index<s->initial)done=false;   // review: a thread gone stops counting
            if(SuspendThread(thread.handle)==static_cast<DWORD>(-1)){++thread.failures;continue;}
            CONTEXT context{};context.ContextFlags=CONTEXT_CONTROL|CONTEXT_INTEGER;size_t bytes{};uintptr_t rsp{};
            const bool copied=GetThreadContext(thread.handle,&context) && CopyStack(rsp=context.Rsp,thread.stack_base,s->copy.get(),bytes);
            ResumeThread(thread.handle);
            if(!copied){++thread.failures;continue;}
            const auto depth=UnwindCopy(context,rsp,thread.stack_base,s->copy.get(),bytes,frames.data());
            thread.records.push_back(depth);if(s->mark)thread.records.push_back(mark);
            thread.records.insert(thread.records.end(),frames.begin(),frames.begin()+depth);++thread.samples;}
        if(done)break;
    }
    CancelWaitableTimer(timer);CloseHandle(timer);
    Write(*s);
    for(const auto& thread:s->threads)CloseHandle(thread.handle);
    return 0;
}
bool Open(Sampled& sampled,DWORD id,const char* name,uint32_t limit) noexcept {
    const HANDLE handle=OpenThread(THREAD_SUSPEND_RESUME|THREAD_GET_CONTEXT|THREAD_QUERY_INFORMATION,FALSE,id);
    if(!handle)return false;
    sampled.handle=handle;sampled.name=name;sampled.stack_base=StackBaseOf(handle);
    if(!sampled.stack_base){CloseHandle(handle);return false;}
    try{sampled.records.reserve(size_t(limit)*8);}catch(...){CloseHandle(handle);return false;}
    return true;
}
}
bool OnSamplerThread() noexcept {return sampler_thread;}
bool AddThread(DWORD id,const char* name) noexcept {
    if(!id)return false;
    const auto count=std::min<uint32_t>(added_count.load(std::memory_order_acquire),uint32_t(added.size()));
    for(uint32_t i=0;i<count;++i)if(added[i].id.load(std::memory_order_acquire)==id)return true;
    // Two callers may announce the same thread at once: the loop then samples it twice under one name (diagnostic only).
    const auto slot=added_count.fetch_add(1,std::memory_order_acq_rel);
    if(slot>=added.size())return false;
    added[slot].name=name;added[slot].id.store(id,std::memory_order_release);return true;
}
bool Start(const std::wstring& output,std::span<const Thread> threads,const std::atomic<uint64_t>& active,uint32_t limit,
    const std::atomic<uint64_t>* mark,uint64_t end_mark) noexcept {
    static std::atomic<bool> started{};
    if(started.exchange(true))return false;
    try{
        auto s=std::make_unique<Sampler>();s->output=output;s->active=&active;s->mark=mark;s->end_mark=end_mark;s->limit=limit;s->copy=std::make_unique<uint8_t[]>(max_stack);
        for(const auto& thread:threads){Sampled sampled;
            if(!Open(sampled,thread.id,thread.name,limit)){for(const auto& opened:s->threads)CloseHandle(opened.handle);return false;}
            s->threads.push_back(std::move(sampled));}
        s->initial=s->threads.size();
        const HANDLE sampler=CreateThread(nullptr,0,&Loop,s.get(),0,nullptr);
        if(!sampler){for(const auto& opened:s->threads)CloseHandle(opened.handle);return false;}
        s.release();CloseHandle(sampler);return true;
    }catch(...){return false;}
}
}
