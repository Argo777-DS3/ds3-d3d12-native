#pragma once
// I-0128 R8a (R8-PLAN §2; rt-gpu-measure.flag, a diagnostic): the GPU time of named zones of the main view's frame, a timestamp
// pair a zone and a frame slot on the main queue (EndQuery before and after the zone's commands, the slot's written pairs resolved
// into a readback at the end of its recording, read by the slot's completion once its fence passed). A zone's time includes the
// tail of the commands before it that no barrier separates from it (the GPU overlaps them): stated with the figures. A zone is
// timed at most once a frame (a second Begin in the same frame is ignored). Never fails a frame: the heap's or the readback's
// failure is kept for the device (no retry a frame), and the zones go untimed.
#include <d3d12.h>
#include <wrl/client.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <mutex>
namespace ds3native {
enum class GpuZone : uint32_t {Frame,RtScene,RtSun,RtCache,RtReflect,RtWater,RtLights,RtGi,RrGuides,Dlss,RtProbe,Taa,Count};   // I-0129 R9a: RtProbe, the traced probes; I-0132 R11j: Taa
inline constexpr uint32_t kGpuZones=static_cast<uint32_t>(GpuZone::Count);
inline constexpr const char* kGpuZoneNames[kGpuZones]{"frame","rt_scene","rt_sun","rt_cache","rt_reflect","rt_water","rt_lights","rt_gi","rr_guides","dlss","rt_probe","taa"};
// The frames since the last Take: per zone the frames it was timed, the sum and the largest of its ticks.
struct GpuZoneWindow {
    uint64_t frequency{};uint64_t frames{};uint32_t width{},height{};   // the last frame's render extent
    uint64_t pt_frames{};   // the frames traced at the PT level (its paths' vertices theirs; the others the RT level's: the fixes' step review L4)
    std::array<uint64_t,kGpuZones> count{},ticks{},ticks_max{};
};
// One resolved slot added to a window (pure: the unit tests'): the zones of `mask` whose end follows their start.
inline void AccumulateGpuZones(GpuZoneWindow& w,uint32_t mask,const uint64_t* stamps) {
    ++w.frames;
    for(uint32_t z=0;z<kGpuZones;++z){
        if(!(mask&(1u<<z)) || !(stamps[2*z+1]>stamps[2*z]))continue;
        const uint64_t t=stamps[2*z+1]-stamps[2*z];++w.count[z];w.ticks[z]+=t;if(t>w.ticks_max[z])w.ticks_max[z]=t;}
}
template<uint32_t Slots> class GpuZoneTimer {
public:
    // The heap and its readback on the first frame that times (false: off for the device, the zones untimed).
    bool Ready(ID3D12Device* device,ID3D12CommandQueue* queue) {
        if(heap_)return true;
        if(failed_)return false;
        failed_=true;
        D3D12_QUERY_HEAP_DESC q{D3D12_QUERY_HEAP_TYPE_TIMESTAMP,2*kGpuZones*Slots,0};
        Microsoft::WRL::ComPtr<ID3D12QueryHeap> heap;UINT64 frequency{};
        if(FAILED(device->CreateQueryHeap(&q,IID_PPV_ARGS(&heap))) || FAILED(queue->GetTimestampFrequency(&frequency)) || !frequency)return false;
        D3D12_HEAP_PROPERTIES h{};h.Type=D3D12_HEAP_TYPE_READBACK;
        D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=kSlotBytes*Slots;d.Height=1;d.DepthOrArraySize=1;d.MipLevels=1;
        d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Microsoft::WRL::ComPtr<ID3D12Resource> readback;
        if(FAILED(device->CreateCommittedResource(&h,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback))))return false;
        {const std::lock_guard hold(lock_);frequency_=frequency;}
        heap_=heap;readback_=readback;failed_=false;return true;}
    // A frame's recording starts on `slot` (its pairs from the slot's last frame were read at that frame's completion).
    void Start(uint32_t slot){if(slot<Slots)open_[slot]=written_[slot]=0;}
    void Begin(ID3D12GraphicsCommandList* list,uint32_t slot,GpuZone zone){
        const uint32_t z=static_cast<uint32_t>(zone);
        if(!heap_ || slot>=Slots || ((open_[slot]|written_[slot])&(1u<<z)))return;
        list->EndQuery(heap_.Get(),D3D12_QUERY_TYPE_TIMESTAMP,Index(slot,z));open_[slot]|=1u<<z;}
    void End(ID3D12GraphicsCommandList* list,uint32_t slot,GpuZone zone){
        const uint32_t z=static_cast<uint32_t>(zone);
        if(!heap_ || slot>=Slots || !(open_[slot]&(1u<<z)))return;
        list->EndQuery(heap_.Get(),D3D12_QUERY_TYPE_TIMESTAMP,Index(slot,z)+1);open_[slot]&=~(1u<<z);written_[slot]|=1u<<z;}
    // A zone begun whose pass did not run after all (a refused traced pass): dropped, its pair not resolved.
    void Cancel(uint32_t slot,GpuZone zone){if(slot<Slots)open_[slot]&=~(1u<<static_cast<uint32_t>(zone));}
    // The slot's written pairs resolved into its range of the readback, each pair alone (no query this frame left unwritten is
    // resolved: the step review's L4); a zone left open is dropped. The mask the completion reads.
    uint32_t Resolve(ID3D12GraphicsCommandList* list,uint32_t slot){
        if(!heap_ || slot>=Slots || !written_[slot])return 0;
        for(uint32_t z=0;z<kGpuZones;++z)if(written_[slot]&(1u<<z))
            list->ResolveQueryData(heap_.Get(),D3D12_QUERY_TYPE_TIMESTAMP,Index(slot,z),2,readback_.Get(),uint64_t(kSlotBytes)*slot+16ull*z);
        return written_[slot];}
    // At the slot's completion (its fence passed): its pairs added to the window.
    void Complete(ID3D12Resource* readback,uint32_t slot,uint32_t mask,uint32_t width,uint32_t height,bool pt){
        if(!readback || !mask || slot>=Slots)return;
        void* p{};const D3D12_RANGE range{SIZE_T(kSlotBytes)*slot,SIZE_T(kSlotBytes)*(slot+1)};
        if(FAILED(readback->Map(0,&range,&p)))return;
        uint64_t stamps[2*kGpuZones]{};std::memcpy(stamps,static_cast<const uint8_t*>(p)+kSlotBytes*slot,kSlotBytes);
        const D3D12_RANGE none{};readback->Unmap(0,&none);
        const std::lock_guard hold(lock_);AccumulateGpuZones(window_,mask,stamps);window_.width=width;window_.height=height;window_.pt_frames+=pt?1:0;}
    GpuZoneWindow Take(){GpuZoneWindow out;const std::lock_guard hold(lock_);std::swap(out,window_);out.frequency=frequency_;return out;}
    // The device closed: its objects go, a new device may time again.
    void Reset(){heap_.Reset();readback_.Reset();failed_=false;open_={};written_={};const std::lock_guard hold(lock_);window_={};frequency_=0;}
    ID3D12QueryHeap* Heap() const noexcept {return heap_.Get();}
    ID3D12Resource* Readback() const noexcept {return readback_.Get();}
private:
    static constexpr uint32_t kSlotBytes=2*kGpuZones*8;
    static UINT Index(uint32_t slot,uint32_t z){return UINT(2*(kGpuZones*slot+z));}
    Microsoft::WRL::ComPtr<ID3D12QueryHeap> heap_;Microsoft::WRL::ComPtr<ID3D12Resource> readback_;bool failed_{};
    std::array<uint32_t,Slots> open_{},written_{};   // the recording thread's
    std::mutex lock_;GpuZoneWindow window_;uint64_t frequency_{};   // the completion's and the reader's
};
}
