// R13 cR3 host test (render-thread sheet 10.1): T-R1 (depth 1, order over 10,000 jobs, the pumped wait dispatches a window
// message, stop joins), the worker part of T-R3 (a job's exception reaches the owner at the next Post and the worker
// takes no further job; the deadline is a fault) and of T-R4 (no post while quiesced; Quiesce returns only when idle).
#include "../renderer/render_worker.h"
#include "../renderer/work_pool.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <functional>
#include <thread>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>
using namespace ds3native;
namespace {
void Require(bool condition,const char* what) {if(!condition){std::fprintf(stderr,"FAIL %s\n",what);std::exit(1);}}
template<class E,class F> bool Throws(F action) {try{action();}catch(const E&){return true;}catch(...){return false;}return false;}
std::atomic<bool> window_message_seen{};
// T-R9: a module lock a re-entered window procedure wants (as the engine's window procedure can reach a native hook).
std::mutex module_lock;std::atomic<int> reentry_taken{},reentry_contended{};
LRESULT CALLBACK Procedure(HWND window,UINT message,WPARAM w,LPARAM l) {
    if(message==WM_APP+7){window_message_seen.store(true);return 0;}
    // try_lock stands for the lock the handler would wait for: contended = a deadlock with a post under that lock.
    if(message==WM_APP+9){if(module_lock.try_lock()){module_lock.unlock();++reentry_taken;}else ++reentry_contended;return 0;}
    return DefWindowProcW(window,message,w,l);
}
}
int main() {
    // T-R1 depth 1: a second Post blocks until the first job is done.
    {RenderWorker worker({30000,false});worker.Start();Require(worker.ThreadId() && !worker.OnWorker(),"worker thread id");
        HANDLE release=CreateEventW(nullptr,TRUE,FALSE,nullptr);std::atomic<int> state{};
        worker.Post([&]{WaitForSingleObject(release,INFINITE);state.store(1);});
        std::atomic<bool> second_returned{};
        std::thread poster([&]{worker.Post([&]{state.store(2);});second_returned.store(true);});
        Sleep(100);Require(!second_returned.load() && state.load()==0,"second post did not wait for the first job");
        SetEvent(release);poster.join();Require(second_returned.load(),"second post returned");
        worker.Quiesce();Require(state.load()==2 && worker.Completed()==2,"both jobs ran in order");worker.Resume();
        CloseHandle(release);}
    // T-R1 order over 10,000 jobs, and every job runs on the worker thread.
    {RenderWorker worker({30000,false});worker.Start();std::vector<int> seen;seen.reserve(10000);std::atomic<bool> off_thread{};
        for(int k=0;k<10000;++k)worker.Post([&,k]{if(!worker.OnWorker())off_thread.store(true);seen.push_back(k);});
        worker.Quiesce();Require(!off_thread.load(),"job ran off the worker");Require(seen.size()==10000,"job count");
        for(int k=0;k<10000;++k)Require(seen[k]==k,"job order");
        Require(worker.TakeWaitMs()>=0 && worker.TakeWaitMs()==0,"wait counter reset");}
    // T-R1 pumped wait (W2): a message posted to the owner's window is dispatched while Post waits; the job only finishes
    // once it has seen the message, so a wait that does not pump would end in the deadline.
    {WNDCLASSW type{};type.lpfnWndProc=Procedure;type.hInstance=GetModuleHandleW(nullptr);type.lpszClassName=L"ds3-render-worker-test";
        Require(RegisterClassW(&type)!=0,"window class");
        HWND window=CreateWindowExW(0,type.lpszClassName,L"",0,0,0,0,0,HWND_MESSAGE,nullptr,type.hInstance,nullptr);Require(window!=nullptr,"message window");
        RenderWorker worker({5000,true});worker.Start();std::atomic<bool> saw{};
        worker.Post([&]{PostMessageW(window,WM_APP+7,0,0);for(int k=0;k<400 && !window_message_seen.load();++k)Sleep(5);saw.store(window_message_seen.load());});
        worker.Post([]{});Require(saw.load(),"pumped wait did not dispatch the window message");Require(worker.Dispatched()>=1,"dispatch witness");
        // T-R9 pump re-entrancy: a message whose handler wants a module lock, posted while the owner waits in Post. With
        // the post outside every module lock (the rule of render_worker.h) the handler takes it; the same post made under
        // the lock finds it held (the handler would wait for the owner, which waits for the worker: a deadlock).
        const auto reenter=[&]{worker.Post([&]{PostMessageW(window,WM_APP+9,0,0);Sleep(100);});worker.Quiesce();worker.Resume();};
        reenter();Require(reentry_taken.load()==1 && reentry_contended.load()==0,"re-entered handler could not take the module lock");
        // The lock is held across the post by a helper thread standing for the owner's own scope (try_lock on a mutex the
        // calling thread owns would be undefined).
        {HANDLE held=CreateEventW(nullptr,TRUE,FALSE,nullptr),release=CreateEventW(nullptr,TRUE,FALSE,nullptr);
            std::thread holder([&]{const std::lock_guard hold(module_lock);SetEvent(held);WaitForSingleObject(release,INFINITE);});
            WaitForSingleObject(held,INFINITE);reenter();SetEvent(release);holder.join();CloseHandle(held);CloseHandle(release);}
        Require(reentry_contended.load()==1,"post under the module lock not detected (negative control)");
        // A quit request met by the pumped wait is put back once the wait ends, never lost.
        worker.Post([]{});PostQuitMessage(3);worker.Post([]{Sleep(30);});worker.Quiesce();worker.Resume();
        {MSG message{};bool quit{};while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE))if(message.message==WM_QUIT && message.wParam==3)quit=true;
            Require(quit,"quit request lost by the pumped wait");}
        worker.Stop();DestroyWindow(window);}
    // T-R3 (worker part): an exception in a job reaches the owner at the next Post, as a runtime_error carrying its message;
    // a faulted worker takes no further job.
    {RenderWorker worker({30000,false});worker.Start();
        worker.Post([]{throw std::invalid_argument("injected job failure");});
        std::string message;try{worker.Post([]{});}catch(const std::runtime_error& e){message=e.what();}
        Require(message.find("injected job failure")!=std::string::npos,"fault not rethrown at the next post");
        Require(worker.Faulted() && worker.Completed()==1,"faulted worker ran another job");
        Require(Throws<std::runtime_error>([&]{worker.Post([]{});}),"faulted worker accepted a job");}
    // T-R3: the deadline is a fault (shortened here); the late job still completes before the worker stops.
    {RenderWorker worker({50,false});worker.Start();HANDLE release=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        worker.Post([&]{WaitForSingleObject(release,INFINITE);});
        Require(Throws<std::runtime_error>([&]{worker.Post([]{});}),"deadline not a fault");
        SetEvent(release);worker.Stop();Require(worker.Completed()==1,"late job not completed");CloseHandle(release);}
    // T-R4 (worker part): Quiesce returns only when the running job is done; no post while quiesced; Resume reopens.
    {RenderWorker worker({30000,false});worker.Start();std::atomic<bool> done{};
        worker.Post([&]{Sleep(50);done.store(true);});worker.Quiesce();Require(done.load(),"quiesce returned before idle");
        Require(Throws<std::logic_error>([&]{worker.Post([]{});}),"post accepted while quiesced");
        worker.Resume();worker.Post([]{});worker.Stop();Require(worker.Completed()==2 && !worker.Running(),"resume and stop");
        Require(Throws<std::logic_error>([&]{worker.Post([]{});}),"post accepted after stop");}
    // I-0088 P1: Wait joins the posted job (its effects visible after it) and rethrows a job's fault; the thread takes its name.
    {RenderWorker worker({30000,false,L"ds3-native-ui"});worker.Start();int value{};
        worker.Post([&]{Sleep(20);value=7;});worker.Wait();Require(value==7 && worker.Completed()==1,"wait returned before the job");
        worker.Wait();Require(worker.Completed()==1,"a second wait without a job");
        PWSTR name{};const HANDLE thread=OpenThread(THREAD_QUERY_LIMITED_INFORMATION,FALSE,worker.ThreadId());
        Require(thread && SUCCEEDED(GetThreadDescription(thread,&name)) && name && std::wstring(name)==L"ds3-native-ui","worker thread name");
        LocalFree(name);CloseHandle(thread);
        worker.Post([]{throw std::invalid_argument("injected display failure");});
        std::string message;try{worker.Wait();}catch(const std::runtime_error& e){message=e.what();}
        Require(message.find("injected display failure")!=std::string::npos,"fault not rethrown at the wait");}
    // I-0097: the storage queue (TaskQueue): jobs in the order posted on one thread other than the poster's, Post never
    // waiting for a job, a throwing job counted and the next one run.
    {auto& queue=*new TaskQueue("test");std::vector<int> order;   // never destroyed, as in production (a joinable thread)
        std::atomic<DWORD> thread{};HANDLE gate=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        queue.Post([&]{WaitForSingleObject(gate,10000);});   // holds the queue: the posts below must not wait for it
        const auto before=GetTickCount64();
        for(int i=0;i<1000;++i)queue.Post([&,i]{order.push_back(i);thread.store(GetCurrentThreadId());});
        queue.Post([]{throw std::runtime_error("injected storage failure");});queue.Post([&]{order.push_back(1000);});
        Require(GetTickCount64()-before<5000 && queue.Done()==0,"task queue post waited for a job");
        SetEvent(gate);const auto deadline=GetTickCount64()+10000;
        while(queue.Done()<queue.Posted() && GetTickCount64()<deadline)Sleep(1);
        Require(queue.Done()==1003 && queue.Faults()==1 && order.size()==1001,"task queue completion and fault count");
        for(int i=0;i<=1000;++i)Require(order[i]==i,"task queue order");
        Require(thread.load() && thread.load()!=GetCurrentThreadId(),"task queue thread");CloseHandle(gate);}
    // I-0132 R11g: the work pool shared by two callers at once. A run's caller is worker 0 and each worker joins once with its own
    // index; a second thread's run while the first is open gets the workers (the former pool ran it alone, on its caller); a
    // worker's exception reaches its run's caller after every joined call returned; a run from inside a job runs alone.
    {auto& pool=*new WorkPool(3,0,"test");Require(pool.Workers()>=2,"work pool workers");   // never destroyed, as in production
        std::mutex seen_lock;std::vector<size_t> seen;std::atomic<size_t> ticket{};
        const std::function<void(size_t)> once=[&](size_t worker){{const std::lock_guard hold(seen_lock);seen.push_back(worker);}
            while(ticket.fetch_add(1)<400)Sleep(0);};
        pool.Run(once);
        {std::vector<size_t> sorted=seen;std::sort(sorted.begin(),sorted.end());
            Require(!sorted.empty() && sorted[0]==0 && std::adjacent_find(sorted.begin(),sorted.end())==sorted.end() && sorted.back()<pool.Workers(),"work pool indices once each");}
        // Run A: its caller holds it open 300 ms, its workers return at once; run B from another thread meanwhile must see a worker.
        HANDLE b_done=CreateEventW(nullptr,TRUE,FALSE,nullptr);std::atomic<bool> b_helped{},b_finished{};
        const std::function<void(size_t)> a=[&](size_t worker){if(!worker)WaitForSingleObject(b_done,300);};
        std::thread other([&]{Sleep(30);std::atomic<size_t> next{};
            const std::function<void(size_t)> b=[&](size_t worker){for(;next.fetch_add(1)<200;){if(worker)b_helped.store(true);Sleep(1);}};
            pool.Run(b);b_finished.store(true);SetEvent(b_done);});
        pool.Run(a);other.join();CloseHandle(b_done);
        Require(b_finished.load() && b_helped.load(),"work pool second caller's run not joined by a worker while the first was open");
        // A worker's failure rethrown to the caller once every call has returned.
        std::atomic<int> active{};std::atomic<bool> thrown{};
        const std::function<void(size_t)> failing=[&](size_t worker){++active;
            if(worker && !thrown.exchange(true)){--active;throw std::invalid_argument("injected pool failure");}
            if(!worker)for(int i=0;i<2000 && !thrown.load();++i)Sleep(1);   // the caller waits for a worker to have thrown
            Sleep(40);--active;};
        std::string message;try{pool.Run(failing);}catch(const std::invalid_argument& e){message=e.what();}
        Require(message=="injected pool failure" && active.load()==0,"work pool failure not rethrown after every call returned");
        // Its step review's L6: three callers at once, 300 runs each of 64 items by ticket: every item done exactly once a run, every
        // run returned with its joined calls (a lost wakeup or a round used after its return would hang or miscount).
        {std::atomic<uint64_t> done{};std::vector<std::thread> callers;std::atomic<int> bad{};
            for(int t=0;t<3;++t)callers.emplace_back([&]{for(int r=0;r<300;++r){std::array<std::atomic<int>,64> items{};std::atomic<size_t> next{};
                const std::function<void(size_t)> work=[&](size_t){for(size_t i;(i=next.fetch_add(1))<items.size();){items[i].fetch_add(1);done.fetch_add(1);}};
                pool.Run(work);for(const auto& item:items)if(item.load()!=1)bad.fetch_add(1);}});
            for(auto& caller:callers)caller.join();
            Require(bad.load()==0 && done.load()==3ull*300*64,"work pool concurrent callers' items not done exactly once");}
        // The final review's L11: a worker's failure in caller A's run never surfaces in caller B's run at the same time.
        // A stays open until a worker threw in it and B's run has returned (its step review's L1: B returns first, so a failure kept
        // for whichever run returns next, or carried by a worker into its next run, would reach B).
        {std::atomic<bool> a_threw{},b_threw{},a_open{};std::atomic<size_t> b_next{};HANDLE b_returned=CreateEventW(nullptr,TRUE,FALSE,nullptr);
            const std::function<void(size_t)> a_job=[&](size_t worker){
                if(!worker){a_open.store(true);for(int i=0;i<2000 && !a_threw.load();++i)Sleep(1);WaitForSingleObject(b_returned,10000);return;}
                if(!a_threw.exchange(true))throw std::invalid_argument("injected failure in A");};
            std::thread b_caller([&]{for(int i=0;i<2000 && !(a_open.load() && a_threw.load());++i)Sleep(1);
                const std::function<void(size_t)> b_job=[&](size_t){for(;b_next.fetch_add(1)<20;)Sleep(1);};
                try{pool.Run(b_job);}catch(...){b_threw.store(true);}SetEvent(b_returned);});
            bool a_failed=false;try{pool.Run(a_job);}catch(const std::invalid_argument&){a_failed=true;}
            b_caller.join();CloseHandle(b_returned);Require(a_failed && !b_threw.load(),"work pool failure crossed to another caller's run");}
        // Nested: a run inside a job runs alone on the calling thread.
        std::atomic<bool> nested_alone{true};
        const std::function<void(size_t)> inner=[&](size_t worker){if(worker)nested_alone.store(false);};
        const std::function<void(size_t)> outer=[&](size_t){pool.Run(inner);};
        pool.Run(outer);Require(nested_alone.load(),"work pool nested run not alone");}
    std::printf("PASS render worker (with I-0132 R11g's work pool shared by two callers, I-0088's Wait and thread name, I-0097's task queue: order, no wait at post, a fault counted): depth-1 queue (a second post waits for the first job), order over 10,000 jobs on the worker thread, pumped wait dispatches the owner's window message (W2), a job's exception reaches the owner at the next post and stops the worker, the deadline is a fault, quiesce waits for idle and refuses posts until resume, stop joins, a re-entered window handler takes a module lock the post does not hold and finds it held when the post is made under it, a quit request survives the pumped wait (T-R1, T-R9, worker parts of T-R3 and T-R4)\n");
    return 0;
}
