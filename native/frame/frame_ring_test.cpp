// D3D12 phase D cD1 host controls (D3D12-PHASE-D-IMPLEMENTATION-SHEET section 6, complement 4.4): the frame ring on a real
// device with the debug layer. T-D1 with a forced overlap (the queue waits on a fence the CPU signals later, so the GPU is
// provably still on frame N while N+1 records), the release-after-completion rule of the complement, and T-D4 at the ring
// level (a quarantined ring releases nothing). Synthetic clears only.
#include "../renderer/backend.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <algorithm>
#include <atomic>

using namespace ds3native;
using Microsoft::WRL::ComPtr;
namespace {
void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void Check(HRESULT result,const char* message){if(FAILED(result))throw std::runtime_error(message);}
template<class F> void Refuses(F action,const char* message){bool refused{};try{action();}catch(const std::invalid_argument&){refused=true;}Require(refused,message);}
struct Target {
    ComPtr<ID3D12Resource> texture;ComPtr<ID3D12DescriptorHeap> rtv;
    explicit Target(ID3D12Device* device){
        D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Width=desc.Height=64;desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,IID_PPV_ARGS(&texture)),"ring target");
        const D3D12_DESCRIPTOR_HEAP_DESC heaps{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,1};Check(device->CreateDescriptorHeap(&heaps,IID_PPV_ARGS(&rtv)),"ring rtv heap");
        device->CreateRenderTargetView(texture.Get(),nullptr,rtv->GetCPUDescriptorHandleForHeapStart());
    }
};
// One frame: clears recorded into the slot's list, submitted; the slot keeps the target and a marker object.
uint64_t Frame(DeviceQueues& gpu,FrameRing& ring,const Target& target,std::weak_ptr<const void>& marker,uint32_t frame,
    std::shared_ptr<const void> also={}){
    auto& slot=ring.Begin(gpu,WaitOther);
    const float colour[4]{frame/16.f,0,0,1};
    for(int clear=0;clear<64;++clear)slot.list->ClearRenderTargetView(target.rtv->GetCPUDescriptorHandleForHeapStart(),colour,0,nullptr);
    Check(slot.list->Close(),"ring list close");
    auto held=std::make_shared<const uint32_t>(frame);marker=held;slot.kept={held,HoldResource(target.texture.Get())};
    if(also)slot.kept.push_back(std::move(also));
    const auto ticket=gpu.Execute(slot.list.Get());ring.End(gpu,slot,ticket,WaitOther);return ticket;
}
uint64_t DebugErrors(DeviceQueues& gpu){
    if(!gpu.diagnostics)return 0;uint64_t errors{};
    for(UINT64 index=0;index<gpu.diagnostics->GetNumStoredMessages();++index){SIZE_T size{};gpu.diagnostics->GetMessage(index,nullptr,&size);
        std::vector<uint8_t> bytes(size);auto* message=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());
        if(SUCCEEDED(gpu.diagnostics->GetMessage(index,message,&size)) &&
           (message->Severity==D3D12_MESSAGE_SEVERITY_ERROR || message->Severity==D3D12_MESSAGE_SEVERITY_CORRUPTION)){++errors;std::fprintf(stderr,"%s\n",message->pDescription);}}
    return errors;
}
}

int main(int argc,char** argv){
    try{
        Require(argc==2,"warp or hardware required");const bool warp=!std::strcmp(argv[1],"warp");Require(warp || !std::strcmp(argv[1],"hardware"),"unknown profile");
        DeviceQueues gpu;gpu.Open(warp,true);SetRetirementQueue(&gpu);auto* device=gpu.device.Get();
        const Target target(device);
        ComPtr<ID3D12Fence> gate;Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&gate)),"ring gate");uint64_t gate_value{};
        // A list outside the ring (the later submission of the release rule).
        ComPtr<ID3D12CommandAllocator> extra_allocator;ComPtr<ID3D12GraphicsCommandList> extra;
        Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&extra_allocator)),"extra allocator");
        Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,extra_allocator.Get(),nullptr,IID_PPV_ARGS(&extra)),"extra list");Check(extra->Close(),"extra close");

        // One slot: End waits and releases at once, as before the ring; a second Begin while recording is refused.
        {FrameRing ring;ring.Open(device,1);std::weak_ptr<const void> marker;
            for(uint32_t frame=0;frame<4;++frame){Frame(gpu,ring,target,marker,frame);Require(marker.expired() && !ring.Recording(),"T-D1 one slot releases at End");}
            auto& slot=ring.Begin(gpu,WaitOther);Refuses([&]{ring.Begin(gpu,WaitOther);},"T-D1 Begin while recording accepted");
            Refuses([&]{ring.End(gpu,slot,0,WaitOther);},"T-D1 zero ticket accepted");Check(slot.list->Close(),"ring close");
            ring.End(gpu,slot,gpu.Execute(slot.list.Get()),WaitOther);ring.Close();}

        // Two and three slots with the forced overlap. Frame 0 waits on gate v, frames 1.. on gate v+1, so the GPU is provably
        // still before frame 0 while frames 1.. record and submit (their slots are fresh: no wait). A thread opens gate v after
        // 200 ms: Begin on frame 0's slot must block for it, count exactly one completion wait, and return while frames 1..
        // are still pending (it waited for its own slot only); frame 0's contents are released exactly then, the others are
        // not. While that slot is released, an owner reaching RetireWhenSubmittedWorkCompletes goes at once although a later
        // submission is still pending (complement 4.4).
        for(uint32_t slots=2;slots<=max_frame_slots;++slots){
            FrameRing ring;ring.Open(device,slots);std::vector<std::weak_ptr<const void>> markers(slots);std::vector<uint64_t> tickets(slots);
            const auto first_gate=++gate_value,second_gate=++gate_value;
            Check(gpu.queue->Wait(gate.Get(),first_gate),"ring queue wait");
            // Frame 0's slot also holds an owner whose release hands `watched` to RetireWhenSubmittedWorkCompletes.
            std::weak_ptr<void> sentinel;
            {auto watched=std::make_shared<int>(7);sentinel=watched;
                const std::shared_ptr<const void> retiring(new int(0),[watched](const void* value){delete static_cast<const int*>(value);RetireWhenSubmittedWorkCompletes(watched);});
                tickets[0]=Frame(gpu,ring,target,markers[0],0,retiring);}
            Check(gpu.queue->Wait(gate.Get(),second_gate),"ring queue wait 2");
            const auto completes_before=gpu.completes.load();
            for(uint32_t frame=1;frame<slots;++frame)tickets[frame]=Frame(gpu,ring,target,markers[frame],frame);
            const auto pending=gpu.Execute(extra.Get());   // a later submission, also behind gate v+1
            Require(gpu.completes.load()==completes_before,"T-D1 a fresh slot waited");
            Require(gpu.CompletedValue()<tickets[0] && !markers[0].expired(),"T-D1 no overlap (frame 0 complete or released early)");
            std::thread opener([&]{Sleep(200);(void)gate->Signal(first_gate);});
            LARGE_INTEGER frequency{},before{},after{};QueryPerformanceFrequency(&frequency);QueryPerformanceCounter(&before);
            auto& reused=ring.Begin(gpu,WaitOther);QueryPerformanceCounter(&after);opener.join();
            const double waited_ms=1000.0*double(after.QuadPart-before.QuadPart)/double(frequency.QuadPart);
            Require(waited_ms>=150 && gpu.completes.load()==completes_before+1,"T-D1 Begin did not wait for the slot it reuses");
            Require(gpu.CompletedValue()<tickets[1],"T-D1 Begin waited for slots it does not reuse");
            Require(markers[0].expired(),"T-D1 frame 0 not released at its slot's reuse");
            for(uint32_t frame=1;frame<slots;++frame)Require(!markers[frame].expired(),"T-D1 another slot released early");
            Require(gpu.CompletedValue()<pending && sentinel.expired(),"complement 4.4 owner not released at once after completion");
            Check(reused.list->Close(),"ring close");Check(gate->Signal(second_gate),"ring gate release 2");
            ring.End(gpu,reused,gpu.Execute(reused.list.Get()),WaitOther);ring.WaitIdle(gpu,WaitOther);
            for(const auto& marker:markers)Require(marker.expired(),"T-D1 WaitIdle left a slot held");
            ring.Close();gpu.CollectCompleted();}

        // T-D4 at the ring level: a ring quarantined with frames in flight releases nothing, even after it is destroyed: a
        // two-slot ring after a normal End, and a one-slot ring left recording after its Execute (a throw between Execute
        // and End, as the renderer's catch leaves it).
        std::weak_ptr<const void> quarantined,abandoned;
        {const auto quarantine_gate=++gate_value;Check(gpu.queue->Wait(gate.Get(),quarantine_gate),"ring queue wait 3");
            {FrameRing ring;ring.Open(device,2);Frame(gpu,ring,target,quarantined,99);ring.QuarantineToProcessExit();}
            {FrameRing ring;ring.Open(device,1);auto& slot=ring.Begin(gpu,WaitOther);
                const float colour[4]{1,1,1,1};slot.list->ClearRenderTargetView(target.rtv->GetCPUDescriptorHandleForHeapStart(),colour,0,nullptr);
                Check(slot.list->Close(),"ring close 3");auto held=std::make_shared<const uint32_t>(98u);abandoned=held;slot.kept={held};
                (void)gpu.Execute(slot.list.Get());Require(ring.Recording(),"T-D4 ring not left recording");ring.QuarantineToProcessExit();}
            Require(!quarantined.expired() && !abandoned.expired(),"T-D4 quarantined slot released");
            Check(gate->Signal(quarantine_gate),"ring gate release 3");}
        gpu.Drain(WaitOther);gpu.CollectCompleted();

        // cD4 (T-D3 at ring level, T-D5): completion work runs when its slot is released, not at End. Two slots under the
        // forced overlap: frame 0 registers completions; End leaves them pending (its ticket has not completed); WaitNext,
        // the slot wait done ahead of Begin, blocks on the ring's own event while another thread waits on the queue's
        // shared event for a later ticket, and both return once the gate opens; the completions run in order before the
        // slot's kept objects go, and an exception in one still releases the slot and is rethrown.
        {FrameRing ring;ring.Open(device,2);const auto gate_ticket=++gate_value;
            Check(gpu.queue->Wait(gate.Get(),gate_ticket),"ring queue wait 4");
            std::vector<int> order;std::weak_ptr<const void> kept;
            auto& slot0=ring.Begin(gpu,WaitOther);const float colour[4]{0,1,0,1};
            slot0.list->ClearRenderTargetView(target.rtv->GetCPUDescriptorHandleForHeapStart(),colour,0,nullptr);Check(slot0.list->Close(),"ring close 5");
            {auto held=std::make_shared<const int>(5);kept=held;slot0.kept={held};}
            slot0.completions.push_back([&]{Require(!kept.expired(),"T-D3 kept released before its completion");order.push_back(1);});
            slot0.completions.push_back([&]{order.push_back(2);});
            const auto ticket0=gpu.Execute(slot0.list.Get());ring.End(gpu,slot0,ticket0,WaitOther);
            Require(order.empty() && !kept.expired(),"T-D3 completions ran at End with two slots");
            auto& slot1=ring.Begin(gpu,WaitOther);Check(slot1.list->Close(),"ring close 6");
            const auto ticket1=gpu.Execute(slot1.list.Get());ring.End(gpu,slot1,ticket1,WaitOther);
            Require(gpu.CompletedValue()<ticket0,"T-D5 no overlap");
            bool other_waited{},other_failed{};
            std::thread other([&]{try{gpu.Complete(ticket1,WaitOther);other_waited=true;}catch(...){other_failed=true;}});
            std::thread opener([&]{Sleep(200);(void)gate->Signal(gate_ticket);});
            ring.WaitNext(gpu,WaitOther);opener.join();other.join();
            Require(other_waited && !other_failed && gpu.CompletedValue()>=ticket0,"T-D5 WaitNext with a concurrent waiter");
            Require(order.empty() && !kept.expired(),"T-D3 WaitNext released the slot (the release belongs to Begin)");
            Require(ring.TakeWaitMs()>=150.0 && ring.TakeWaitMs()==0.0,"T-D5 slot wait not measured");
            auto& again=ring.Begin(gpu,WaitOther);
            Require(order==std::vector<int>{1,2} && kept.expired(),"T-D3 completions not run in order at the release");
            Check(again.list->Close(),"ring close 7");
            again.completions.push_back([]{throw std::runtime_error("completion failure");});
            std::weak_ptr<const void> after;{auto held=std::make_shared<const int>(6);after=held;again.kept={held};}
            ring.End(gpu,again,gpu.Execute(again.list.Get()),WaitOther);
            bool rethrown{};try{ring.WaitIdle(gpu,WaitOther);}catch(const std::runtime_error&){rethrown=true;}
            Require(rethrown && after.expired() && !slot1.ticket,"T-D3 a failing completion kept a slot or was swallowed");
            ring.Close();gpu.CollectCompleted();}



        // cD3 (complement 2.1(b), T-D11): one list per frame. Several recordings continue the slot that is recording
        // (Current), and a slot that references a raw resource it does not hold is refused before its submission.
        {FrameRing ring;ring.Open(device,1);Refuses([&]{ring.Current();},"T-D11 Current without a recording");
            const auto frames=ring.Frames();auto& slot=ring.Begin(gpu,WaitOther);Require(&ring.Current()==&slot && ring.Frames()==frames+1,"T-D11 current slot");
            const Target view(device),movie(device);
            slot.referenced.push_back(view.texture.Get());slot.kept.push_back(HoldResource(view.texture.Get()));
            slot.referenced.push_back(movie.texture.Get());
            bool refused{};try{FrameRing::RequireHeld(slot);}catch(const std::logic_error&){refused=true;}
            Require(refused,"T-D11 an unheld raw resource accepted");
            slot.kept.push_back(HoldResource(movie.texture.Get()));FrameRing::RequireHeld(slot);
            Check(slot.list->Close(),"ring close 4");ring.End(gpu,slot,gpu.Execute(slot.list.Get()),WaitOther);
            Require(slot.referenced.empty() && slot.kept.empty(),"T-D11 slot not released");ring.Close();}

        // I-0090: with deferred release a retired slot's holders are destroyed on the ring's release thread, never on the
        // recording thread; a holder retired there releases its own completion-retired owner at once (complement 4.4); Flush
        // and WaitIdle leave nothing held.
        {FrameRing ring;ring.Open(device,2);ring.SetDeferredRelease(true);
            const auto recorder=std::this_thread::get_id();std::atomic<uint32_t> on_recorder{},elsewhere{};
            struct Bundle {std::thread::id recorder;std::atomic<uint32_t>* on_recorder{};std::atomic<uint32_t>* elsewhere{};std::shared_ptr<const void> retiring;
                ~Bundle(){if(on_recorder)(std::this_thread::get_id()==recorder?*on_recorder:*elsewhere)+=1;}};
            std::weak_ptr<const void> marker,inner;
            for(uint32_t frame=0;frame<6;++frame){
                auto owner=std::make_shared<uint32_t>(frame);inner=owner;
                auto bundle=std::make_shared<Bundle>();bundle->recorder=recorder;bundle->on_recorder=&on_recorder;bundle->elsewhere=&elsewhere;
                // A holder whose deleter retires another owner on completion, as the descriptor ranges do.
                bundle->retiring=std::shared_ptr<const void>(owner.get(),[owner](const void*) mutable {RetireWhenSubmittedWorkCompletes(std::move(owner));});
                owner.reset();Frame(gpu,ring,target,marker,frame,std::move(bundle));
            }
            ring.Flush();
            Require(on_recorder.load()==0 && elsewhere.load()==4,"I-0090 retired holders not destroyed on the release thread");
            Require(!inner.expired(),"I-0090 the last frame's holder released early");
            ring.WaitIdle(gpu,WaitOther);Require(marker.expired() && inner.expired() && elsewhere.load()==6 && on_recorder.load()==0,"I-0090 WaitIdle left a holder");
            Require(ring.TakeReleaseThreadMs()>=0.0,"I-0090 release thread time");ring.Close();}
        // Without deferred release (the default) the holders go in Retire, on the recording thread.
        {FrameRing ring;ring.Open(device,2);std::weak_ptr<const void> marker;
            const auto recorder=std::this_thread::get_id();std::atomic<uint32_t> on_recorder{},elsewhere{};
            struct Probe {std::thread::id recorder;std::atomic<uint32_t>* on_recorder{};std::atomic<uint32_t>* elsewhere{};
                ~Probe(){if(on_recorder)(std::this_thread::get_id()==recorder?*on_recorder:*elsewhere)+=1;}};
            for(uint32_t frame=0;frame<3;++frame){auto probe=std::make_shared<Probe>();probe->recorder=recorder;probe->on_recorder=&on_recorder;probe->elsewhere=&elsewhere;
                Frame(gpu,ring,target,marker,frame,std::move(probe));}
            auto& slot=ring.Begin(gpu,WaitOther);Require(slot.kept.empty() && on_recorder.load()==2 && elsewhere.load()==0,"I-0090 inline release not on the recording thread");Check(slot.list->Close(),"ring close inline");
            ring.End(gpu,slot,gpu.Execute(slot.list.Get()),WaitOther);ring.WaitIdle(gpu,WaitOther);Require(marker.expired(),"I-0090 inline WaitIdle");ring.Close();}

        // cD3 (sheet A2, T-D6 arena part): size classes. A range above 1 MB takes a pooled 4 MB chunk (then 16 and 64 MB),
        // the next small range continues that chunk, chunks return to their own class at the ticket and are reused there;
        // a range above 64 MB takes a chunk of its own, never pooled; an aligned range honours its alignment.
        {UploadArena arena;const auto created=UploadArenaChunksCreated();
            const auto first=arena.Allocate(device,100);const auto movie=arena.Allocate(device,(1ull<<20)+(400ull<<10),512,false);
            Require(arena.UsedChunks()==2 && movie.resource && movie.offset%512==0 && movie.gpu==movie.resource->GetGPUVirtualAddress()+movie.offset,"arena 4 MB class range");
            const auto after=arena.Allocate(device,256);Require(after.resource==movie.resource && arena.UsedChunks()==2,"arena class chunk not continued");
            const auto stream=arena.Allocate(device,10ull<<20);Require(arena.UsedChunks()==3 && stream.resource->GetDesc().Width==(16ull<<20),"arena 16 MB class");
            (void)first;arena.Retire(gpu,gpu.Drain(WaitOther));gpu.CollectCompleted();
            Require(arena.FreeChunks(0)==1 && arena.FreeChunks(1)==1 && arena.FreeChunks(2)==1 && UploadArenaChunksCreated()==created+3,"arena classes not pooled");
            const auto again=arena.Allocate(device,3ull<<20);Require(again.resource==movie.resource && arena.FreeChunks(1)==0,"arena 4 MB chunk not reused");
            const auto huge=arena.Allocate(device,(64ull<<20)+1);Require(huge.resource->GetDesc().Width==(64ull<<20)+256,"arena unpooled range size");
            arena.Retire(gpu,gpu.Drain(WaitOther));gpu.CollectCompleted();
            Require(arena.FreeChunks(1)==1 && arena.FreeChunks(3)==0 && arena.FreeChunks()==3,"arena unpooled chunk pooled");
            Refuses([&]{arena.Allocate(device,64,384);},"arena accepted a non power of two alignment");
            arena.Reset();Require(!arena.FreeChunks(),"arena reset kept chunks");}
        // cD5 (B5, pooled form; T-D8 at pool level): a published version is immutable and holds its buffer; its buffer goes
        // back to the pool only after its last holder is gone and the queue completed the work submitted by then; the next
        // version of that size reuses it (no creation once warm); a version still held by a pending list is never reused;
        // a size that differs takes another buffer; a reset pool takes nothing back.
        {UploadVersionPool versions;std::vector<BYTE> bytes(10000,0x5A);const auto gate_ticket=++gate_value;
            auto first=versions.Publish(device,bytes.data(),bytes.size());
            Require(first->resource && first->bytes==10000 && first->resource->GetDesc().Width==12288 && versions.Created()==1,"B5 version buffer");
            ID3D12Resource* const first_buffer=first->resource.Get();
            // A list that reads the version, held by a slot until its ticket (behind a gate: still pending).
            FrameRing ring;ring.Open(device,2);Check(gpu.queue->Wait(gate.Get(),gate_ticket),"pool queue wait");
            auto& slot=ring.Begin(gpu,WaitOther);Check(slot.list->Close(),"pool list close");slot.kept={first};
            ring.End(gpu,slot,gpu.Execute(slot.list.Get()),WaitOther);
            first.reset();   // the owner drops it; the slot still holds it
            auto second=versions.Publish(device,bytes.data(),bytes.size());
            Require(second->resource.Get()!=first_buffer && versions.Created()==2,"B5 a held version's buffer was reused");
            Check(gate->Signal(gate_ticket),"pool gate release");ring.WaitIdle(gpu,WaitOther);gpu.CollectCompleted();
            Require(versions.FreeBuffers()==1,"B5 released version not back in the pool");
            second.reset();gpu.Drain(WaitOther);gpu.CollectCompleted();
            Require(versions.FreeBuffers()==2,"B5 second version not back after completion");
            const auto warm=versions.Created();
            for(int frame=0;frame<20;++frame){auto version=versions.Publish(device,bytes.data(),bytes.size());
                Require(version->bytes==bytes.size(),"B5 version");version.reset();gpu.Drain(WaitOther);gpu.CollectCompleted();}
            Require(versions.Created()==warm,"B5 steady state created a buffer");
            auto other=versions.Publish(device,bytes.data(),5000);Require(other->resource->GetDesc().Width==8192 && versions.Created()==warm+1,"B5 size class");
            other.reset();versions.Reset();gpu.Drain(WaitOther);gpu.CollectCompleted();Require(!versions.FreeBuffers(),"B5 reset pool took a buffer back");
            ring.Close();}


        // R13 cR1 (T-R2): four threads submit and wait concurrently, 1,500 times each, behind a queue wait on a fence the
        // test opens later (forced overlap), while a fifth commits and submits ingress lists and a sampler reads the fence:
        // tickets strictly increase per thread and are unique, the completed value never decreases, every wait returns
        // with its ticket complete, and no wait throws (no lost or stolen wake-up); a sixth thread signals and drains. The
        // sheet's Signal-order hook and the negative control against the former queue are not built (the fence sampler and
        // the 5 s deadlines are the detectors).
        {const auto gate_ticket=++gate_value;Check(gpu.queue->Wait(gate.Get(),gate_ticket),"T-R2 queue wait");
            constexpr int threads=4,submissions=1500;std::atomic<bool> failed{},sampling{true};std::atomic<int> ready{};
            std::vector<std::vector<uint64_t>> tickets(threads);std::vector<std::thread> workers;
            std::vector<ComPtr<ID3D12CommandAllocator>> allocators(threads);std::vector<ComPtr<ID3D12GraphicsCommandList>> lists(threads);
            for(int i=0;i<threads;++i){Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocators[i])),"T-R2 allocator");
                Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocators[i].Get(),nullptr,IID_PPV_ARGS(&lists[i])),"T-R2 list");
                const float colour[4]{0,0,1,1};lists[i]->ClearRenderTargetView(target.rtv->GetCPUDescriptorHandleForHeapStart(),colour,0,nullptr);
                Check(lists[i]->Close(),"T-R2 close");}
            std::thread sampler([&]{try{uint64_t last{};while(sampling){const auto value=gpu.CompletedValue();if(value<last)failed=true;last=value;}}catch(...){failed=true;}});
            for(int i=0;i<threads;++i)workers.emplace_back([&,i]{try{++ready;
                for(int n=0;n<submissions;++n){const auto ticket=gpu.Execute(lists[i].Get());
                    if(!tickets[i].empty() && ticket<=tickets[i].back())failed=true;tickets[i].push_back(ticket);
                    gpu.Complete(ticket,WaitOther);}   // a list runs again only after its previous execution completed
                gpu.Complete(tickets[i].back(),WaitOther);if(gpu.CompletedValue()<tickets[i].back())failed=true;}catch(...){failed=true;}});
            std::thread ingress([&]{try{for(int n=0;n<200;++n){auto recording=gpu.RecordIngress();(void)recording.Commit();if(n%3==0)(void)gpu.SubmitIngress();}}catch(...){failed=true;}});
            std::thread drainer([&]{try{for(int n=0;n<100;++n){(void)gpu.Signal();(void)gpu.Drain(WaitOther);}}catch(...){failed=true;}});   // Drain and Signal() concurrently too
            while(ready<threads)Sleep(1);Sleep(100);Check(gate->Signal(gate_ticket),"T-R2 gate release");
            for(auto& worker:workers)worker.join();ingress.join();drainer.join();(void)gpu.SubmitIngress();sampling=false;sampler.join();
            std::vector<uint64_t> all;for(const auto& list:tickets)all.insert(all.end(),list.begin(),list.end());
            std::sort(all.begin(),all.end());
            Require(!failed && all.size()==size_t(threads)*submissions && std::adjacent_find(all.begin(),all.end())==all.end(),"T-R2 concurrent submissions and waits");
            gpu.Drain(WaitOther);gpu.CollectCompleted();}


        Require(DebugErrors(gpu)==0,"debug layer errors");
        WithdrawRetirementQueue(&gpu);gpu.Close();
        std::printf("PASS frame ring %s: one slot releases at End; I-0090 deferred release on the ring's thread (Flush, WaitIdle, completion-retired owners at once); two and three slots record under a forced overlap without waiting for "
            "slots they do not reuse, Begin blocks only for the slot it reuses, a slot is released exactly at its reuse; owners released with a completed slot go at once while "
            "a later submission is pending; a quarantined ring (after End, or left recording after its Execute) releases nothing; completions run at the slot's release, WaitNext waits on its own event beside a concurrent queue waiter (T-D3, T-D5); one list per frame "
            "refuses an unheld raw resource (T-D11); arena size classes pool and reuse 1, 4 and 16 MB chunks, a range above 64 MB is not pooled; dynamic stream versions reuse pooled buffers only after their holders and the queue are done (B5); four threads submit and wait concurrently without a lost or stolen wake-up, tickets in order (T-R2); D3D12 validation\n",argv[1]);
        return 0;
    }catch(const std::exception& error){std::fprintf(stderr,"FAIL %s\n",error.what());return 1;}
}
