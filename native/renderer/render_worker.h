#pragma once
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <exception>
#include <functional>
#include <mutex>
#include <thread>

namespace ds3native {
// R13 cR3 (R13/R14 render-thread sheet, section 8): the native render thread. One job slot (depth 1, as the original's
// render thread): Post waits for the previous job, then hands over the next one. The wait is W2 (section 8.1): a
// MsgWaitForMultipleObjects on the idle event that dispatches the owner thread's window messages whenever they arrive,
// with a proposed deadline (30 s, then fault; the original's wait has none).
//
// Lock order (phase F sheet 3.3, written once here; cF.4): the idle wait comes before any native lock. Post, Wait, Quiesce
// and Stop are never called under gpu_submissions, model_storage_lock, model_stream_lock, native_offscreen_lock or any other
// lock of the module, because the pumped messages may re-enter the engine's window procedure (T-R9) and the job may take
// those locks. The worker never takes gpu_submissions.
//
// Fault channel (section 8.2): an exception in a job is kept; the owner sees it at the next Post (rethrown as a
// std::runtime_error carrying the message) or through Faulted(); a faulted worker accepts no further job.
class RenderWorker {
public:
    // I-0088: `name` is the thread's description (the UI display and the collect side workers are two more instances).
    struct Options {DWORD deadline_ms=30000;bool pump=true;const wchar_t* name=L"ds3-native-render";};
    RenderWorker()=default;
    explicit RenderWorker(Options options):options_(options){}
    RenderWorker(const RenderWorker&)=delete;
    RenderWorker& operator=(const RenderWorker&)=delete;
    // Tests only: production leaks the worker (process exit ends its thread first; cF.7, PILOT D-10).
    ~RenderWorker();
    void Start();
    bool Running() const noexcept {return thread_.joinable();}
    // Owner thread: waits for the previous job (pumping), rethrows its fault, then posts. Refused while quiesced.
    void Post(std::function<void()> job);
    // Owner thread: waits for the posted job (pumping when the options say so), then rethrows its fault (I-0088: a job the
    // owner joins within the same frame).
    void Wait() {if(thread_.joinable()){WaitIdle(options_.deadline_ms);RethrowFault();}}
    // Owner thread: waits until the worker is idle and refuses posts until Resume. Before Drain, Resize, Close, depth
    // recreation (section 5.3).
    void Quiesce();
    void Resume() noexcept {quiesced_.store(false,std::memory_order_release);}
    bool Quiesced() const noexcept {return quiesced_.load(std::memory_order_acquire);}
    // Owner thread: waits for the last job without a deadline and joins (tests and an orderly Close only).
    void Stop() noexcept;
    bool Faulted() const noexcept {return faulted_.load(std::memory_order_acquire);}
    void RethrowFault() const;
    DWORD ThreadId() const noexcept {return thread_id_.load(std::memory_order_acquire);}
    bool OnWorker() const noexcept {return GetCurrentThreadId()==ThreadId();}
    uint64_t Completed() const noexcept {return completed_.load(std::memory_order_acquire);}
    // queue_wait_ms: owner-thread milliseconds spent waiting in Post and Quiesce since the last call.
    double TakeWaitMs() noexcept;
    // Witness of the pumped wait: messages dispatched while waiting.
    uint64_t Dispatched() const noexcept {return dispatched_.load(std::memory_order_relaxed);}
private:
    void Loop() noexcept;
    void WaitIdle(DWORD deadline_ms);
    Options options_{};
    HANDLE idle_{},wake_{};
    mutable std::mutex lock_;std::function<void()> job_;bool stop_{};std::exception_ptr fault_;
    std::atomic<bool> faulted_{},quiesced_{};std::atomic<DWORD> thread_id_{};
    std::atomic<uint64_t> completed_{},dispatched_{},wait_ticks_{};
    std::thread thread_;
};
}
