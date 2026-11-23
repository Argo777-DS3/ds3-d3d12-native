#pragma once
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <exception>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace ds3native {
// I-0071 point 3 (ER A: the entity Draw jobs, GXEntityDrawParallelJob, over the global job pool with an atomic ticket and the
// caller taking part), shared since I-0074 (ER 141B3DA20: the render command runner's chunk tasks on the same kind of pool):
// a fixed set of workers kept for the process's life (never joined: the process ends with them parked).
class WorkPool {
public:
    // At most `limit` workers beside the caller, and never more than the hardware threads minus `reserve`.
    WorkPool(unsigned limit,unsigned reserve,const char* name):name_(name) {
        const unsigned hardware=(std::max)(std::thread::hardware_concurrency(),reserve);
        for(unsigned i=0;i+reserve<hardware && i<limit && i<63;++i)threads_.emplace_back([this,i]{Loop(i+1);});
    }
    WorkPool(const WorkPool&)=delete;
    WorkPool& operator=(const WorkPool&)=delete;
    size_t Workers() const noexcept {return threads_.size()+1;}   // with the caller
    // I-0077 (sample-profile runs): when set, each worker calls it with its pool's name before every job it runs (the sampler
    // announces the thread once), so a profile sees the workers beside the caller.
    static inline std::atomic<void(*)(const char*) noexcept> worker_hook{};
    // job(worker) on the caller (worker 0) and on each worker (1..Workers()-1) free to join it, each worker at most once a run;
    // returns once the caller's call and every worker's call that joined have returned. An exception of any of them (a failed
    // allocation) is rethrown here once all have returned, so the caller's own handling sees it and no worker still uses the
    // caller's frame. The jobs take their work by ticket, so a run joined by fewer workers (all busy) still completes, the caller
    // doing what is left. I-0132 R11g: two threads' runs at once share the workers (each worker joins the oldest open run it has not
    // joined yet, so a worker done with one run's tickets helps the other) instead of the second caller running its job alone; the
    // rays' walk on its own thread and the main collect overlap on the same workers.
    void Run(const std::function<void(size_t)>& job) {
        // Review L2: a job run from inside a pool job (a worker, or the caller while it runs one) runs alone too.
        if(inside){job(0);return;}
        Round round{&job};
        {const std::lock_guard hold(lock_);open_.push_back(&round);}
        wake_.notify_all();
        {struct Inside {Inside(){WorkPool::inside=true;}~Inside(){WorkPool::inside=false;}} mark;
            try {job(0);} catch(...) {const std::lock_guard hold(lock_);if(!round.failure)round.failure=std::current_exception();}}
        std::unique_lock hold(lock_);
        open_.erase(std::find(open_.begin(),open_.end(),&round));   // closed: no worker joins it from here
        done_.wait(hold,[&]{return round.active==0;});
        if(round.failure){auto failure=std::exchange(round.failure,nullptr);hold.unlock();std::rethrow_exception(failure);}
    }
private:
    // One caller's run: its job, the workers that joined it (a bit each) and those still inside it, its first failure.
    struct Round {const std::function<void(size_t)>* job{};uint64_t joined{};size_t active{};std::exception_ptr failure;};
    void Loop(size_t index) {
        const uint64_t bit=uint64_t(1)<<index;
        for(;;){Round* round{};
            {std::unique_lock hold(lock_);
                wake_.wait(hold,[&]{for(auto* open:open_)if(!(open->joined&bit)){round=open;return true;}return false;});
                round->joined|=bit;++round->active;}
            if(const auto hook=worker_hook.load(std::memory_order_relaxed))hook(name_);
            std::exception_ptr failure;try {inside=true;(*round->job)(index);} catch(...) {failure=std::current_exception();}inside=false;
            {const std::lock_guard hold(lock_);if(failure && !round->failure)round->failure=failure;--round->active;}   // `round` not touched after this
            done_.notify_all();}
    }
    static inline thread_local bool inside{};
    std::mutex lock_;std::condition_variable wake_,done_;
    std::vector<Round*> open_;   // the runs workers may join, oldest first
    const char* name_;
    std::vector<std::thread> threads_;
};
// The engine thread's collect (I-0071 point 3): at most seven workers, two hardware threads left to the render worker and
// the rest of the game.
inline WorkPool& AdmissionPool() {static auto* pool=new WorkPool(7,3,"admit");return *pool;}
// I-0074 point 3c: the render worker's chunked recording, which overlaps the next frame's collect: three workers beside the
// render worker, and only on a machine with room for both pools (twelve hardware threads and up).
inline WorkPool& RecordingPool() {static auto* pool=new WorkPool(3,9,"record");return *pool;}
// I-0097 (ER: loading work is published when complete and never joined by the engine thread, I-0073 ER-STUDY): jobs run in the
// order posted, one at a time, on one thread of their own kept for the process's life (never joined). Post never waits. A job
// handles its own failures; one that throws anyway is counted and the next runs.
class TaskQueue {
public:
    explicit TaskQueue(const char* name):name_(name),thread_([this]{Loop();}){}
    TaskQueue(const TaskQueue&)=delete;
    TaskQueue& operator=(const TaskQueue&)=delete;
    void Post(std::function<void()> job){{const std::lock_guard hold(lock_);jobs_.push_back(std::move(job));++posted_;}wake_.notify_one();}
    uint64_t Posted() const {const std::lock_guard hold(lock_);return posted_;}
    uint64_t Done() const noexcept {return done_.load(std::memory_order_acquire);}
    uint64_t Faults() const noexcept {return faults_.load(std::memory_order_relaxed);}
    std::thread::native_handle_type Handle() {return thread_.native_handle();}
private:
    void Loop() {
        for(;;){std::function<void()> job;
            {std::unique_lock hold(lock_);wake_.wait(hold,[&]{return !jobs_.empty();});job=std::move(jobs_.front());jobs_.pop_front();}
            if(const auto hook=WorkPool::worker_hook.load(std::memory_order_relaxed))hook(name_);
            try {job();} catch(...) {faults_.fetch_add(1,std::memory_order_relaxed);}
            done_.fetch_add(1,std::memory_order_release);}
    }
    const char* name_;
    mutable std::mutex lock_;std::condition_variable wake_;std::deque<std::function<void()>> jobs_;uint64_t posted_{};
    std::atomic<uint64_t> done_{},faults_{};
    std::thread thread_;   // last: started once the members above exist
};
}
