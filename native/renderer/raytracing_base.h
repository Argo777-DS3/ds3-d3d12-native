#pragma once
// I-0109 (R0, RT-PT-PLAN §3.1): the ray-tracing path's base: its bindless descriptor heap and the start self-test that proves
// DXR, DXIL and the heap work inside DS3's process. Nothing here draws; the raster stages keep their own heaps until
// their traced replacements remove them (D-0237).
#include "backend.h"
#include <deque>
#include <stdexcept>

namespace ds3native {

// The slots of the bindless heap: released slots first (the most recent first), then never-used ones in order; a released slot
// is reused only once the queue fence has passed the ticket of the last submission that may read it. No device: unit-tested alone.
class BindlessIndices {
public:
    static constexpr uint32_t none=UINT32_MAX;
    explicit BindlessIndices(uint32_t capacity):capacity_(capacity),state_(capacity,free_slot){}
    // A free slot, or `none` when every slot is live or still awaits its fence; `completed` is the queue fence's completed value.
    uint32_t Allocate(uint64_t completed) {
        while(!pending_.empty() && pending_.front().second<=completed){
            state_[pending_.front().first]=free_slot;free_.push_back(pending_.front().first);pending_.pop_front();}
        uint32_t slot=none;
        if(!free_.empty()){slot=free_.back();free_.pop_back();}
        else if(next_<capacity_)slot=next_++;
        if(slot!=none){state_[slot]=live_slot;++live_;}
        return slot;
    }
    // `ticket`: the submission after which no list reads the slot (a ticket released out of order only delays the slots after it).
    void Release(uint32_t slot,uint64_t ticket) {
        // Step review L3: only a live slot may be released (a double release would hand one slot to two owners).
        if(slot>=capacity_ || state_[slot]!=live_slot)throw std::logic_error("bindless slot not live");
        state_[slot]=pending_slot;--live_;pending_.emplace_back(slot,ticket);
    }
    uint32_t Live() const noexcept {return live_;}
    uint32_t Capacity() const noexcept {return capacity_;}
private:
    enum : uint8_t {free_slot,live_slot,pending_slot};
    uint32_t capacity_{},next_{},live_{};
    std::vector<uint8_t> state_;
    std::vector<uint32_t> free_;
    std::deque<std::pair<uint32_t,uint64_t>> pending_;
};

// One shader-visible CBV/SRV/UAV heap for the ray-tracing path, its slots reached by index from SM 6.6 shaders
// (ResourceDescriptorHeap). Engine-thread owned until the RT passes give it a frame owner (R2).
class BindlessHeap {
public:
    // 2^18 slots: room for every DS3 texture and mesh buffer of the loaded world (R2), 8 MB of descriptors on NVIDIA.
    static constexpr uint32_t default_capacity=1u<<18;
    // The heap's HRESULT when it cannot be made (the self-test names the step "bindless_heap").
    HRESULT Open(ID3D12Device* device,uint32_t capacity=default_capacity);
    bool IsOpen() const noexcept {return heap_!=nullptr;}
    uint32_t Allocate(uint64_t completed) {return indices_.Allocate(completed);}
    void Release(uint32_t slot,uint64_t ticket) {indices_.Release(slot,ticket);}
    D3D12_CPU_DESCRIPTOR_HANDLE Cpu(uint32_t slot) const {   // final review L5: never a descriptor outside the heap
        if(!heap_ || slot>=indices_.Capacity())throw std::logic_error("bindless slot outside the heap");
        return {cpu_.ptr+size_t(slot)*stride_};}
    ID3D12DescriptorHeap* Heap() const noexcept {return heap_.Get();}
    const BindlessIndices& Indices() const noexcept {return indices_;}
private:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_;
    D3D12_CPU_DESCRIPTOR_HANDLE cpu_{};uint32_t stride_{};
    BindlessIndices indices_{0};
};

// The start self-test: one triangle's BLAS and TLAS, one inline ray in a DXIL compute shader reading both through the bindless
// heap, read back. Never throws: a failure is reported (and the RT path stays off), the game goes on.
struct RaytracingSelfTest {
    bool ran{},passed{};
    // The step that failed ("submit" or "wait" for the queue), "done" when the ray came back, "unsupported" below DXR 1.1,
    // binding tier 3 or SM 6.6.
    const char* stage{"not_run"};
    HRESULT failure{S_OK};
    uint32_t status{};float t{};double milliseconds{};
};
RaytracingSelfTest RunRaytracingSelfTest(DeviceQueues& gpu,BindlessHeap& heap) noexcept;

}
