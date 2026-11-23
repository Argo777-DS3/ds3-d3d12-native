// I-0109 (R0): the bindless slots' reuse rule; the ray-tracing self-test on the hardware device and on WARP, with the debug layer
// when the Graphics Tools are present (WARP's result is reported, not required: its ray tracing depends on the Windows build; its
// debug layer must stay silent as the hardware's); and the self-test whose wait fails keeps its objects and slots to their ticket.
#include "../renderer/raytracing_base.h"
#include <cstdio>
#include <stdexcept>
#include <string_view>
#include <vector>

using namespace ds3native;
namespace {
void Require(bool condition,const char* what){if(!condition)throw std::runtime_error(what);}

void Slots() {
    BindlessIndices slots(3);
    Require(slots.Allocate(0)==0 && slots.Allocate(0)==1 && slots.Allocate(0)==2,"lowest first");
    Require(slots.Allocate(0)==BindlessIndices::none,"full");
    slots.Release(1,5);
    Require(slots.Allocate(4)==BindlessIndices::none,"a released slot waits for its ticket");
    Require(slots.Allocate(5)==1 && slots.Live()==3,"reused once the fence passed its ticket");
    slots.Release(0,6);slots.Release(2,7);
    Require(slots.Allocate(7)!=BindlessIndices::none && slots.Allocate(7)!=BindlessIndices::none && slots.Live()==3,"both reused");
    const auto refused=[](auto&& release){try{release();}catch(const std::logic_error&){return true;}return false;};
    Require(refused([]{BindlessIndices fresh(2);fresh.Release(0,1);}),"a slot never handed out is refused");
    Require(refused([]{BindlessIndices twice(2);const uint32_t slot=twice.Allocate(0);(void)twice.Allocate(0);twice.Release(slot,1);twice.Release(slot,2);}),
        "a slot released twice is refused");
}

// The debug layer's warnings and errors (the sibling stage suites require none).
uint64_t DebugMessages(DeviceQueues& gpu) {
    if(!gpu.diagnostics)return 0;uint64_t count{};
    for(UINT64 index=0;index<gpu.diagnostics->GetNumStoredMessages();++index){SIZE_T size{};gpu.diagnostics->GetMessage(index,nullptr,&size);
        std::vector<uint8_t> bytes(size);auto* message=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());
        if(SUCCEEDED(gpu.diagnostics->GetMessage(index,message,&size)) && message->Severity<=D3D12_MESSAGE_SEVERITY_WARNING){++count;std::printf("debug layer: %s\n",message->pDescription);}}
    return count;
}

RaytracingSelfTest SelfTest(bool warp) {
    DeviceQueues gpu;gpu.Open(warp,true);BindlessHeap heap;
    const auto r=RunRaytracingSelfTest(gpu,heap);
    const auto& c=gpu.Capabilities();
    std::printf("%s: level %x, raytracing %d, ran %d, passed %d, stage %s, hresult 0x%08lX, status %u, t %.4f, %.2f ms, debug layer %d\n",
        warp?"WARP":"hardware",unsigned(c.feature_level),int(c.raytracing),int(r.ran),int(r.passed),r.stage,static_cast<unsigned long>(r.failure),
        r.status,r.t,r.milliseconds,int(gpu.DebugLayerActive()));
    if(r.ran)Require(heap.IsOpen() && heap.Indices().Live()==0,"both slots released after the self-test");
    Require(DebugMessages(gpu)==0,"no debug-layer warning or error");
    gpu.Close();
    return r;
}

// Final review M3: a queue held behind a fence nobody signals yet: the self-test's wait times out (5 s), so it reports "wait",
// retires its objects after its ticket and keeps both slots pending; once the queue moves on, both come back.
void FailedWait() {
    DeviceQueues gpu;gpu.Open(false,false);BindlessHeap heap;
    Microsoft::WRL::ComPtr<ID3D12Fence> gate;Require(SUCCEEDED(gpu.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&gate))),"gate fence");
    gpu.queue->Wait(gate.Get(),1);
    // Step review 1: the queue is let go on every exit, so a failed check never releases what a waiting list still reads.
    struct Release {ID3D12Fence* gate;DeviceQueues& gpu;~Release(){if(!gate)return;(void)gate->Signal(1);try{gpu.Drain();}catch(...){}}} release{gate.Get(),gpu};
    const auto r=RunRaytracingSelfTest(gpu,heap);
    if(!r.ran){std::printf("failed wait: skipped (%s)\n",r.stage);return;}
    std::printf("failed wait: passed %d, stage %s, pending retirements %zu, live slots %u\n",int(r.passed),r.stage,gpu.PendingRetirements(),
        heap.Indices().Live());
    Require(!r.passed && std::string_view(r.stage)=="wait","the failed wait named");
    Require(gpu.PendingRetirements()==1 && heap.Indices().Live()==0,"the self-test's objects retired after its ticket, its slots pending");
    const uint32_t fresh=heap.Allocate(gpu.CompletedValue());
    Require(fresh==2,"both slots kept from reuse while the list may run");
    Require(SUCCEEDED(gate->Signal(1)),"gate signal");gpu.Drain();
    Require(gpu.CollectCompleted()==1 && gpu.PendingRetirements()==0,"the objects released once the queue passed the ticket");
    const uint32_t first=heap.Allocate(gpu.CompletedValue()),second=heap.Allocate(gpu.CompletedValue());
    Require(first+second==1 && first!=second,"both slots reused once the queue passed the ticket");
    for(const uint32_t slot:{fresh,first,second})heap.Release(slot,gpu.LastSubmitted());
    release.gate=nullptr;gpu.Close();
}
}

int main() {
    try {
        Slots();
        const auto hardware=SelfTest(false);
        // Thomas's PC has DXR 1.2 (I-0109 RESULT): there the self-test must pass; an adapter without DXR 1.1 reports « unsupported ».
        Require(hardware.passed || (!hardware.ran && std::string_view(hardware.stage)=="unsupported"),"the hardware self-test");
        (void)SelfTest(true);
        FailedWait();
        std::printf("PASS raytracing base: bindless slots, self-test, failed wait\n");
        return 0;
    } catch(const std::exception& e) {std::printf("FAIL raytracing base: %s\n",e.what());return 1;}
}
