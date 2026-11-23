#include "render_worker.h"
#include <stdexcept>
#include <string>

namespace ds3native {
namespace {
uint64_t Ticks() noexcept {LARGE_INTEGER t{};QueryPerformanceCounter(&t);return static_cast<uint64_t>(t.QuadPart);}
}
RenderWorker::~RenderWorker() {
    Stop();
    if(idle_)CloseHandle(idle_);
    if(wake_)CloseHandle(wake_);
}
void RenderWorker::Start() {
    if(thread_.joinable())throw std::logic_error("native render worker started twice");
    // Idle: manual reset, set while no job runs (the original's thread+0x48); wake: auto reset.
    if(!idle_ && !(idle_=CreateEventW(nullptr,TRUE,TRUE,nullptr)))throw std::runtime_error("native render worker idle event");
    if(!wake_ && !(wake_=CreateEventW(nullptr,FALSE,FALSE,nullptr)))throw std::runtime_error("native render worker wake event");
    stop_=false;
    thread_=std::thread([this]{Loop();});
    while(!thread_id_.load(std::memory_order_acquire))SwitchToThread();
}
void RenderWorker::Loop() noexcept {
    SetThreadDescription(GetCurrentThread(),options_.name);
    thread_id_.store(GetCurrentThreadId(),std::memory_order_release);
    for(;;){
        WaitForSingleObject(wake_,INFINITE);
        std::function<void()> job;
        {const std::lock_guard hold(lock_);job.swap(job_);if(!job && stop_)return;}
        if(!job)continue;
        try{job();}
        catch(...){const std::lock_guard hold(lock_);fault_=std::current_exception();faulted_.store(true,std::memory_order_release);}
        job=nullptr;   // the job's owners go on the worker, before idle
        completed_.fetch_add(1,std::memory_order_acq_rel);
        SetEvent(idle_);
    }
}
void RenderWorker::WaitIdle(DWORD deadline_ms) {
    const auto start=GetTickCount64();const auto begin=Ticks();
    // A quit request taken from the queue stops the pumping for the rest of this wait (re-posted at once it would wake the
    // wait again and again) and is put back for the engine's own loop when the wait ends.
    bool quit{};WPARAM quit_code{};
    const auto finish=[&]{wait_ticks_.fetch_add(Ticks()-begin,std::memory_order_relaxed);if(quit)PostQuitMessage(static_cast<int>(quit_code));};
    for(;;){
        const auto elapsed=GetTickCount64()-start;
        const DWORD remaining=deadline_ms==INFINITE?INFINITE:(elapsed>=deadline_ms?0:static_cast<DWORD>(deadline_ms-elapsed));
        // After a quit request only sent messages are processed (DXGI on the render worker may send to the window's thread).
        const bool pump=options_.pump && !quit;
        const DWORD result=options_.pump?MsgWaitForMultipleObjectsEx(1,&idle_,remaining,quit?QS_SENDMESSAGE:QS_ALLINPUT,quit?0:MWMO_INPUTAVAILABLE)
                                        :WaitForSingleObject(idle_,remaining);
        if(result==WAIT_OBJECT_0)break;
        if(options_.pump && quit && result==WAIT_OBJECT_0+1){MSG sent{};(void)PeekMessageW(&sent,nullptr,0,0,PM_NOREMOVE|PM_QS_SENDMESSAGE);continue;}
        if(pump && result==WAIT_OBJECT_0+1){
            // W2: dispatch what arrived (the owner thread is the window's thread).
            MSG message{};
            while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){
                if(message.message==WM_QUIT){quit=true;quit_code=message.wParam;break;}
                TranslateMessage(&message);DispatchMessageW(&message);dispatched_.fetch_add(1,std::memory_order_relaxed);}
            continue;
        }
        finish();
        if(result==WAIT_TIMEOUT)throw std::runtime_error("native render worker deadline");
        throw std::runtime_error("native render worker wait");
    }
    finish();
}
void RenderWorker::RethrowFault() const {
    std::exception_ptr fault;
    {const std::lock_guard hold(lock_);fault=fault_;}
    if(!fault)return;
    // EndFrame's catch handles std::exception only: the worker's exception reaches it as a runtime_error with its message.
    try{std::rethrow_exception(fault);}
    catch(const std::exception& e){throw std::runtime_error(std::string("native render worker: ")+e.what());}
    catch(...){throw std::runtime_error("native render worker: unknown exception");}
}
void RenderWorker::Post(std::function<void()> job) {
    if(!thread_.joinable())throw std::logic_error("native render worker not started");
    if(OnWorker())throw std::logic_error("native render worker posting to itself");
    if(Quiesced())throw std::logic_error("native render worker quiesced");
    WaitIdle(options_.deadline_ms);
    RethrowFault();
    {const std::lock_guard hold(lock_);job_=std::move(job);}
    ResetEvent(idle_);SetEvent(wake_);
}
void RenderWorker::Quiesce() {
    if(!thread_.joinable())return;
    if(OnWorker())throw std::logic_error("native render worker quiescing itself");
    quiesced_.store(true,std::memory_order_release);
    WaitIdle(options_.deadline_ms);
}
void RenderWorker::Stop() noexcept {
    if(!thread_.joinable())return;
    WaitForSingleObject(idle_,INFINITE);
    {const std::lock_guard hold(lock_);stop_=true;}
    SetEvent(wake_);thread_.join();thread_id_.store(0,std::memory_order_release);
}
double RenderWorker::TakeWaitMs() noexcept {
    LARGE_INTEGER frequency{};QueryPerformanceFrequency(&frequency);
    return 1000.0*static_cast<double>(wait_ticks_.exchange(0,std::memory_order_relaxed))/static_cast<double>(frequency.QuadPart);
}
}
