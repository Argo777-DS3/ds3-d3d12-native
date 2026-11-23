#include "backend.h"
#include "streamline_host.h"
#include <dxgidebug.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <exception>
#include <limits>
#include <map>
#include <stdexcept>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace ds3native {
using Microsoft::WRL::ComPtr;
namespace {
void Check(HRESULT status, const char* operation) {
    if (FAILED(status)) {
        char message[160]{};
        sprintf_s(message,"%s: HRESULT %08lX",operation,static_cast<unsigned long>(status));
        throw std::runtime_error(message);
    }
}
D3D12_HEAP_PROPERTIES Heap(D3D12_HEAP_TYPE type) {
    D3D12_HEAP_PROPERTIES h{}; h.Type=type; h.CreationNodeMask=1; h.VisibleNodeMask=1; return h;
}
// I-0132 R11f (its step review's L4): an UPLOAD buffer's state is GENERIC_READ; a GPU upload heap's buffer starts COMMON (promoted
// to each read it is bound for, as a default heap's buffer).
D3D12_RESOURCE_STATES UploadHeapState(D3D12_HEAP_TYPE type) noexcept {return type==D3D12_HEAP_TYPE_GPU_UPLOAD?D3D12_RESOURCE_STATE_COMMON:D3D12_RESOURCE_STATE_GENERIC_READ;}
D3D12_RESOURCE_DESC Buffer(uint64_t bytes) {
    D3D12_RESOURCE_DESC d{}; d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;
    d.Width=bytes; d.Height=1; d.DepthOrArraySize=1; d.MipLevels=1;
    d.SampleDesc.Count=1; d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR; return d;
}
HRESULT SystemFactory(UINT flags, REFIID iid, void** output) {
    // The game directory contains a DXGI proxy. Resolve the owned bootstrap
    // explicitly from the system library, irrespective of its loaded namesake.
    using CreateFactory=HRESULT(WINAPI*)(UINT,REFIID,void**);
    static const CreateFactory create=[] {
        wchar_t directory[MAX_PATH]{};
        const UINT length=GetSystemDirectoryW(directory,MAX_PATH);
        if (!length || length>=MAX_PATH) throw std::runtime_error("system directory");
        const std::wstring path=std::wstring(directory)+L"\\dxgi.dll";
        HMODULE module=LoadLibraryExW(path.c_str(),nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!module) throw std::runtime_error("system DXGI load");
        wchar_t loaded[MAX_PATH]{};
        const DWORD count=GetModuleFileNameW(module,loaded,MAX_PATH);
        const auto procedure=GetProcAddress(module,"CreateDXGIFactory2");
        HMODULE pinned{};
        const bool valid=count && count<MAX_PATH && !_wcsicmp(loaded,path.c_str()) && procedure &&
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
                reinterpret_cast<LPCWSTR>(procedure),&pinned) && pinned==module;
        FreeLibrary(module);
        if (!valid) throw std::runtime_error("system DXGI identity/export pin");
        return reinterpret_cast<CreateFactory>(procedure);
    }();
    return create(flags,iid,output);
}
void __stdcall DiagnosticCallback(D3D12_MESSAGE_CATEGORY,D3D12_MESSAGE_SEVERITY severity,
    D3D12_MESSAGE_ID,const char* text,void*) {
    if (severity<=D3D12_MESSAGE_SEVERITY_WARNING && !AgilityCapabilityNotice(severity,text) && !FrameGenerationFenceNotice(severity,text,false)) {   // I-0132 R11f, R12f
        std::fprintf(stderr,"NATIVE_DIAGNOSTIC %s\n",text); std::fflush(stderr);
    }
}
// Optional read-only development receipt around the actual DXGI creation call.
// No process-wide queue clear/filter change, HWND mutation or retry is performed.
class PresentationTrace {
public:
    PresentationTrace(bool debug,HWND window) noexcept {
        alive_before_=IsWindow(window);thread_before_=GetWindowThreadProcessId(window,&pid_before_);
        if(!debug)return;
        try {
            wchar_t directory[MAX_PATH]{};const UINT n=GetSystemDirectoryW(directory,MAX_PATH);
            if(!n || n>=MAX_PATH)return;
            const auto path=std::wstring(directory)+L"\\dxgi.dll";
            HMODULE module=LoadLibraryExW(path.c_str(),nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
            if(!module)return;
            using GetDebug=HRESULT(WINAPI*)(UINT,REFIID,void**);
            const auto get=reinterpret_cast<GetDebug>(GetProcAddress(module,"DXGIGetDebugInterface1"));
            if(get)status_=get(0,IID_PPV_ARGS(&queue_));
            FreeLibrary(module);
            if(queue_)begin_=queue_->GetNumStoredMessagesAllowedByRetrievalFilters(DXGI_DEBUG_ALL);
        }catch(...){}
    }
    void Complete(HWND window,ID3D12CommandQueue* queue,const DXGI_SWAP_CHAIN_DESC1& desc,
                  HRESULT status) const noexcept {
        try {
            DWORD pid{};const DWORD thread=GetWindowThreadProcessId(window,&pid);RECT client{};
            const BOOL client_ok=GetClientRect(window,&client);wchar_t window_class[256]{};GetClassNameW(window,window_class,256);
            char utf8[1024]{};WideCharToMultiByte(CP_UTF8,0,window_class,-1,utf8,1024,nullptr,nullptr);
            const auto command=queue->GetDesc();ComPtr<ID3D12Device> queue_device;
            const auto device_status=queue->GetDevice(IID_PPV_ARGS(&queue_device));LUID adapter{};
            if(queue_device)adapter=queue_device->GetAdapterLuid();std::ostringstream out;
            out<<"hwnd="<<reinterpret_cast<uintptr_t>(window)<<" alive="<<IsWindow(window)
               <<" alive_before="<<alive_before_<<" pid_before="<<pid_before_<<" window_thread_before="<<thread_before_
               <<" pid="<<pid<<" window_thread="<<thread<<" caller_thread="<<GetCurrentThreadId()
               <<" class="<<utf8<<" class_style="<<GetClassLongPtrW(window,GCL_STYLE)
               <<" style="<<GetWindowLongPtrW(window,GWL_STYLE)<<" ex_style="<<GetWindowLongPtrW(window,GWL_EXSTYLE)
               <<" visible="<<IsWindowVisible(window)<<" iconic="<<IsIconic(window)<<" client_ok="<<client_ok
               <<" client="<<client.left<<","<<client.top<<","<<client.right<<","<<client.bottom
               <<" queue="<<reinterpret_cast<uintptr_t>(queue)<<" type="<<command.Type<<" priority="<<command.Priority
               <<" queue_flags="<<command.Flags<<" node="<<command.NodeMask
               <<" queue_device="<<reinterpret_cast<uintptr_t>(queue_device.Get())
               <<" adapter_luid="<<adapter.HighPart<<","<<adapter.LowPart<<" queue_device_status="<<std::hex<<static_cast<unsigned long>(device_status)<<std::dec
               <<" desc="<<desc.Width<<","<<desc.Height<<","<<desc.Format<<","<<desc.SampleDesc.Count
               <<","<<desc.BufferCount<<","<<desc.SwapEffect<<","<<desc.Flags
               <<" status="<<std::hex<<static_cast<unsigned long>(status)
               <<" dxgi_queue_status="<<static_cast<unsigned long>(status_)<<std::dec;
            if(queue_) {
                const auto end=queue_->GetNumStoredMessagesAllowedByRetrievalFilters(DXGI_DEBUG_ALL);
                out<<" dxgi_begin="<<begin_<<" dxgi_end="<<end;
                // Global queue messages are bracketed candidates; concurrent producers may exist.
                for(UINT64 i=begin_;i<end && i-begin_<16;++i) {
                    SIZE_T bytes{};const auto sized=queue_->GetMessage(DXGI_DEBUG_ALL,i,nullptr,&bytes);
                    if(FAILED(sized) || bytes<sizeof(DXGI_INFO_QUEUE_MESSAGE) || bytes>16384){out<<" unreadable="<<i;continue;}
                    std::vector<uint64_t> storage((bytes+7)/8);
                    auto message=reinterpret_cast<DXGI_INFO_QUEUE_MESSAGE*>(storage.data());
                    if(FAILED(queue_->GetMessage(DXGI_DEBUG_ALL,i,message,&bytes))){out<<" unreadable="<<i;continue;}
                    out<<" DXGI["<<i<<",id="<<message->ID<<",severity="<<message->Severity<<"]=";
                    if(message->pDescription)for(SIZE_T j=0;j<message->DescriptionByteLength && j<2048 && message->pDescription[j];++j) {
                        const char c=message->pDescription[j];out<<(c=='\r'||c=='\n'||c=='\t'?' ':c);
                    }
                }
                if(end<begin_)out<<" queue_reset_observed=1";
                if(end>=begin_ && end-begin_>16)out<<" messages_truncated=1";
            }
            const auto text=out.str();
            std::fprintf(stderr,"NATIVE_PRESENTATION_CREATE %s\n",text.c_str());std::fflush(stderr);
        }catch(...) {}
    }
private:
    ComPtr<IDXGIInfoQueue> queue_;
    HRESULT status_{E_NOINTERFACE};UINT64 begin_{};
    BOOL alive_before_{};DWORD pid_before_{},thread_before_{};
};
}

struct DeviceQueues::DescriptorPool {
    std::mutex lock;ID3D12Device* device{};UINT stride{};size_t in_use{};
    std::vector<ComPtr<ID3D12DescriptorHeap>> heaps;std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> free;
};
// I-0079: the placed pools (backend.h). Each heap keeps its free ranges by offset, merged with their neighbours when a range
// returns; an allocation takes the first range of any heap that holds it aligned (heaps are few: 64 MB each). A heap
// with every range free beyond the one kept is released by TrimStreamedMemory (its resources hold it too); the pool goes at
// Close, after the drain.
namespace {
// After ER (I-0079 ER-STUDY 2): 8 MB at most placed, the 4 KB-placed textures in heaps of their own (32 MB).
constexpr uint64_t placed_heap_bytes=64ull<<20,placed_small_heap_bytes=32ull<<20,placed_bound=8ull<<20;
constexpr uint64_t staging_chunk_bytes=16ull<<20;constexpr size_t staging_pooled=4;   // step review M3: 64 MB kept after a burst
// Whether a texture may take the 4 KB placement: its first level within 16 tiles of 4 KB in the standard tile shape of its
// element size (64x64 elements at 1 byte down to 16x16 at 16; a block-compressed element is its 4x4 block). Formats outside
// the list keep the 64 KB placement.
bool SmallPlacement(const D3D12_RESOURCE_DESC& desc) {
    if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D || desc.SampleDesc.Count!=1 || desc.DepthOrArraySize!=1 ||   /* review L8: as D3D12MA */
       (desc.Flags&(D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET|D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)))return false;
    uint32_t bytes{},block{1};
    switch(desc.Format){
    case DXGI_FORMAT_BC1_TYPELESS:case DXGI_FORMAT_BC1_UNORM:case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:case DXGI_FORMAT_BC4_UNORM:case DXGI_FORMAT_BC4_SNORM:bytes=8;block=4;break;
    case DXGI_FORMAT_BC2_TYPELESS:case DXGI_FORMAT_BC2_UNORM:case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:case DXGI_FORMAT_BC3_UNORM:case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC5_TYPELESS:case DXGI_FORMAT_BC5_UNORM:case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:case DXGI_FORMAT_BC6H_UF16:case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:case DXGI_FORMAT_BC7_UNORM:case DXGI_FORMAT_BC7_UNORM_SRGB:bytes=16;block=4;break;
    case DXGI_FORMAT_R8_UNORM:case DXGI_FORMAT_A8_UNORM:bytes=1;break;
    case DXGI_FORMAT_R8G8_UNORM:case DXGI_FORMAT_R16_FLOAT:case DXGI_FORMAT_R16_UNORM:bytes=2;break;
    case DXGI_FORMAT_R8G8B8A8_UNORM:case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:case DXGI_FORMAT_B8G8R8X8_UNORM:case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R10G10B10A2_UNORM:case DXGI_FORMAT_R11G11B10_FLOAT:case DXGI_FORMAT_R16G16_FLOAT:bytes=4;break;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:case DXGI_FORMAT_R16G16B16A16_UNORM:case DXGI_FORMAT_R32G32_FLOAT:bytes=8;break;
    case DXGI_FORMAT_R32G32B32A32_FLOAT:bytes=16;break;
    default:return false;}
    const uint32_t tile_x=bytes==1?64:bytes==2?64:bytes==4?32:bytes==8?32:16,tile_y=bytes==1?64:bytes==2?32:bytes==4?32:bytes==8?16:16;
    const uint64_t x=(desc.Width+block-1)/block,y=(uint64_t(desc.Height)+block-1)/block;
    return ((x+tile_x-1)/tile_x)*((y+tile_y-1)/tile_y)*desc.DepthOrArraySize<=16;
}
// {6B2F8A31-3C0E-4E7A-9C55-0D7915C3A079}: the private-data slot of a placed resource's lease.
constexpr GUID placed_lease_slot={0x6b2f8a31,0x3c0e,0x4e7a,{0x9c,0x55,0x0d,0x79,0x15,0xc3,0xa0,0x79}};
}
struct DeviceQueues::PlacedPool {
    struct Heap {ComPtr<ID3D12Heap> heap;std::map<uint64_t,uint64_t> free;};
    // A returned range waits for the queue to pass the last ticket submitted when its resource was destroyed (step review
    // M2: a committed allocation's release was deferred by the video memory manager; a placed range is reused at once).
    struct Returned {Heap* heap{};uint64_t offset{},bytes{},ticket{};};
    std::mutex lock;ID3D12Device* device{};D3D12_HEAP_FLAGS flags{};bool not_zeroed{true};uint64_t heap_bytes{placed_heap_bytes};
    std::vector<std::unique_ptr<Heap>> heaps;std::vector<Returned> returned;
    ID3D12Fence* fence{};const std::atomic<uint64_t>* submitted{};   // the owner's queue fence and last submitted ticket
    std::atomic<uint64_t> placed{},committed{},live_bytes{};
    // Under `lock`: the returned ranges the queue has passed (`completed`, read before the lock: pre-merge review M2, no D3D12
    // call under a lock the runtime's release of a resource also takes) become free again.
    void Reclaim(uint64_t completed) {
        if(returned.empty())return;
        std::erase_if(returned,[&](const Returned& r){if(r.ticket>completed)return false;Give(r.heap,r.offset,r.bytes);return true;});
    }
    // A range [offset, offset+bytes) of `heap`, or {nullptr} when no heap holds it: the free range that leaves the least
    // behind (best fit; first fit fragmented the heaps, 28 heaps for 1.07 GB live in i79-15).
    std::pair<Heap*,uint64_t> Take(uint64_t bytes,uint64_t align,uint64_t completed) {
        Reclaim(completed);
        Heap* best{};std::map<uint64_t,uint64_t>::iterator best_at;uint64_t best_left=UINT64_MAX;
        for(const auto& heap:heaps)for(auto at=heap->free.begin();at!=heap->free.end();++at){
            const uint64_t start=(at->first+align-1)&~(align-1);
            if(start+bytes>at->first+at->second)continue;
            const uint64_t left=at->second-bytes;if(left<best_left){best=heap.get();best_at=at;best_left=left;if(!left)break;}}
        if(!best)return {nullptr,0};
        const uint64_t range_start=best_at->first,range_end=best_at->first+best_at->second,start=(range_start+align-1)&~(align-1);
        best->free.erase(best_at);
        if(start>range_start)best->free.emplace(range_start,start-range_start);
        if(range_end>start+bytes)best->free.emplace(start+bytes,range_end-start-bytes);
        return {best,start};
    }
    void Give(Heap* heap,uint64_t offset,uint64_t bytes) {
        auto [at,inserted]=heap->free.emplace(offset,bytes);(void)inserted;
        if(at!=heap->free.begin()){auto before=std::prev(at);if(before->first+before->second==at->first){before->second+=at->second;heap->free.erase(at);at=before;}}
        if(auto after=std::next(at);after!=heap->free.end() && at->first+at->second==after->first){at->second+=after->second;heap->free.erase(after);}
    }
};
namespace {
// The lease in a placed resource's private data: released by the runtime with the resource object, it returns the range.
class PlacedLease final:public IUnknown {
public:
    PlacedLease(std::weak_ptr<DeviceQueues::PlacedPool> pool,DeviceQueues::PlacedPool::Heap* heap,uint64_t offset,uint64_t bytes) noexcept
        :pool_(std::move(pool)),heap_(heap),offset_(offset),bytes_(bytes){}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out) override {
        if(!out)return E_POINTER;if(iid==__uuidof(IUnknown)){*out=static_cast<IUnknown*>(this);AddRef();return S_OK;}*out=nullptr;return E_NOINTERFACE;}
    ULONG STDMETHODCALLTYPE AddRef() override {return ++refs_;}
    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG left=--refs_;if(left)return left;
        if(const auto pool=pool_.lock()){
            const std::lock_guard hold(pool->lock);const uint64_t ticket=pool->submitted?pool->submitted->load():0;
            pool->returned.push_back({heap_,offset_,bytes_,ticket});pool->live_bytes-=bytes_;}
        delete this;return 0;}
private:
    std::atomic<ULONG> refs_{1};std::weak_ptr<DeviceQueues::PlacedPool> pool_;DeviceQueues::PlacedPool::Heap* heap_{};uint64_t offset_{},bytes_{};
};
}
struct DeviceQueues::StagingPool {
    struct Chunk {ComPtr<ID3D12Resource> buffer;uint8_t* cpu{};uint64_t bytes{},used{};size_t users{};bool current{};};
    std::mutex lock;ID3D12Device* device{};
    std::vector<std::unique_ptr<Chunk>> chunks;std::vector<Chunk*> free;Chunk* current{};
    std::atomic<uint64_t> created{},dedicated{},bytes{};
    // Under `lock`: a chunk nobody uses and that takes no more ranges goes back to the free list, or past the pooled count is
    // handed back to be released once the lock is dropped (pre-merge review M1: no D3D12 call under the pool's lock).
    std::unique_ptr<Chunk> Settle(Chunk* chunk) {
        if(chunk->users || chunk->current)return nullptr;
        if(free.size()<staging_pooled){chunk->used=0;free.push_back(chunk);return nullptr;}
        bytes-=chunk->bytes;std::unique_ptr<Chunk> out;
        std::erase_if(chunks,[&](auto& owned){if(owned.get()!=chunk)return false;out=std::move(owned);return true;});
        return out;
    }
};
namespace {
// A persistently mapped UPLOAD buffer of `size` bytes (created with no pool lock held).
ComPtr<ID3D12Resource> MappedUpload(ID3D12Device* device,uint64_t size,uint8_t*& cpu) {
    const auto heap=Heap(D3D12_HEAP_TYPE_UPLOAD);const auto desc=Buffer(size);ComPtr<ID3D12Resource> buffer;
    Check(CountedResource(device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&buffer)),"streamed staging buffer");
    void* mapped{};const D3D12_RANGE none{};Check(buffer->Map(0,&none,&mapped),"streamed staging map");cpu=static_cast<uint8_t*>(mapped);return buffer;
}
}
ComPtr<ID3D12Resource> DeviceQueues::CreateStreamed(D3D12_RESOURCE_DESC desc,D3D12_RESOURCE_STATES initial,PlacedKind kind) {
    if(!device)throw std::runtime_error("streamed resource device unavailable");
    auto pool=kind==PlacedKind::Texture?placed_textures_:placed_buffers_;   // a copy (step review L5)
    ComPtr<ID3D12Resource> resource;
    if(pool){
        // A small texture takes the 4 KB placement when the device grants it (as a committed one could not).
        D3D12_RESOURCE_ALLOCATION_INFO info{};
        // The runtime grants it only when the most detailed level fits 16 standard 4 KB tiles (it reports any other request
        // as an error, i79-05 and i79-07), counted here as the D3D12 memory allocator does, every array slice included.
        desc.Alignment=0;
        if(kind==PlacedKind::Texture && SmallPlacement(desc)){
            desc.Alignment=D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT;info=device->GetResourceAllocationInfo(0,1,&desc);
            if(info.Alignment!=D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT){desc.Alignment=0;info=device->GetResourceAllocationInfo(0,1,&desc);}}
        else info=device->GetResourceAllocationInfo(0,1,&desc);
        if(kind==PlacedKind::Texture && desc.Alignment==D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT && placed_small_)pool=placed_small_;
        if(info.SizeInBytes && info.SizeInBytes!=UINT64_MAX && info.SizeInBytes<=placed_bound){
            const uint64_t align=std::max<uint64_t>(info.Alignment,D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT);
            const uint64_t completed=pool->fence?pool->fence->GetCompletedValue():0;
            std::unique_lock hold(pool->lock);auto taken=pool->Take(info.SizeInBytes,align,completed);
            if(!taken.first){   // a new heap of the kind (rare: 64 MB at a time), created outside the lock (step review M1: a
                // range returned meanwhile must not wait on it); two threads may each add one, the spare is trimmed later
                const bool not_zeroed=pool->not_zeroed;hold.unlock();
                D3D12_HEAP_DESC heap_desc{};heap_desc.SizeInBytes=pool->heap_bytes;heap_desc.Properties=Heap(D3D12_HEAP_TYPE_DEFAULT);
                heap_desc.Alignment=D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;heap_desc.Flags=pool->flags|(not_zeroed?D3D12_HEAP_FLAG_CREATE_NOT_ZEROED:D3D12_HEAP_FLAG_NONE);
                auto created=std::make_unique<PlacedPool::Heap>();
                HRESULT status=CountedHeap(device)->CreateHeap(&heap_desc,IID_PPV_ARGS(&created->heap));
                bool flag_refused=false;
                if(status==E_INVALIDARG && not_zeroed){heap_desc.Flags=pool->flags;status=CountedHeap(device)->CreateHeap(&heap_desc,IID_PPV_ARGS(&created->heap));flag_refused=SUCCEEDED(status);}   // review L5: only a refused flag
                hold.lock();if(flag_refused)pool->not_zeroed=false;   // this runtime refuses the flag: never asked again
                if(SUCCEEDED(status)){created->free.emplace(0,pool->heap_bytes);pool->heaps.push_back(std::move(created));
                    taken=pool->Take(info.SizeInBytes,align,completed);}}
            if(auto* heap=taken.first){const uint64_t offset=taken.second;
                const auto placed_heap=heap->heap;hold.unlock();
                const HRESULT status=CountedResource(device)->CreatePlacedResource(placed_heap.Get(),offset,&desc,initial,nullptr,IID_PPV_ARGS(&resource));
                if(SUCCEEDED(status)){
                    auto* lease=new PlacedLease(pool,heap,offset,info.SizeInBytes);
                    pool->live_bytes+=info.SizeInBytes;
                    const HRESULT kept=resource->SetPrivateDataInterface(placed_lease_slot,lease);lease->Release();   // the resource holds it now
                    if(FAILED(kept))throw std::runtime_error("streamed resource lease");
                    ++pool->placed;return resource;}
                const std::lock_guard back(pool->lock);pool->Give(heap,offset,info.SizeInBytes);}}
        desc.Alignment=0;
    }
    const auto heap=Heap(D3D12_HEAP_TYPE_DEFAULT);
    Check(CountedResource(device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,initial,nullptr,IID_PPV_ARGS(&resource)),"streamed resource allocation");
    if(pool)++pool->committed;
    return resource;
}
StagingRange DeviceQueues::AllocateStaging(uint64_t bytes,uint64_t align) {
    if(!device || !bytes || !align || (align&(align-1)))throw std::invalid_argument("streamed staging request");
    StagingRange range;range.bytes=bytes;
    const auto pool=staging_;
    if(!pool || bytes+align>staging_chunk_bytes){   // a range above a chunk (or a closed pool): a buffer of its own, never pooled
        auto buffer=MappedUpload(device.Get(),bytes,range.cpu);range.resource=buffer.Get();
        if(pool){++pool->dedicated;}
        range.lease=std::make_shared<ComPtr<ID3D12Resource>>(std::move(buffer));return range;}
    std::unique_lock hold(pool->lock);std::vector<std::unique_ptr<StagingPool::Chunk>> spares;   // released after the lock
    StagingPool::Chunk* chunk{};uint64_t start{};
    for(;;){
        if(pool->current){start=(pool->current->used+align-1)&~(align-1);
            if(start+bytes<=pool->current->bytes){chunk=pool->current;break;}
            auto* full=pool->current;full->current=false;pool->current=nullptr;if(auto out=pool->Settle(full))spares.push_back(std::move(out));}
        if(!pool->free.empty()){pool->current=pool->free.back();pool->free.pop_back();pool->current->used=0;pool->current->current=true;continue;}
        // A new chunk, created with the lock dropped (pre-merge review M1); another thread may have set one meanwhile.
        hold.unlock();spares.clear();auto created=std::make_unique<StagingPool::Chunk>();created->bytes=staging_chunk_bytes;
        created->buffer=MappedUpload(device.Get(),staging_chunk_bytes,created->cpu);hold.lock();
        ++pool->created;pool->bytes+=staging_chunk_bytes;auto* added=created.get();pool->chunks.push_back(std::move(created));
        if(!pool->current){added->current=true;pool->current=added;}else if(auto out=pool->Settle(added))spares.push_back(std::move(out));
    }
    chunk->used=start+bytes;++chunk->users;
    range.cpu=chunk->cpu+start;range.resource=chunk->buffer.Get();range.offset=start;
    // The lease: the last holder (the recording's retirement) gives the range back; the pool may be gone by then (Close).
    struct Use {std::weak_ptr<StagingPool> pool;StagingPool::Chunk* chunk{};
        ~Use(){std::unique_ptr<StagingPool::Chunk> released;
            if(const auto owner=pool.lock()){const std::lock_guard hold(owner->lock);--chunk->users;released=owner->Settle(chunk);}}};   // released after the lock
    auto use=std::make_shared<Use>();use->pool=pool;use->chunk=chunk;range.lease=std::move(use);
    hold.unlock();spares.clear();
    return range;
}
size_t DeviceQueues::TrimStreamedMemory(uint64_t local_over,uint64_t nonlocal_over,bool no_world) {
    // After ER's 141B04700 (I-0079 ER-STUDY 2): of the heaps a pool holds with every range free, one is kept and one more is
    // released per call; released outside the pool's lock (a heap's release frees its allocation). Over a budget (I-0084), where
    // ER never looks (it reads the budget once, I-0080 ER-STUDY 4): free heaps (local) and idle staging chunks (non-local, the
    // UPLOAD heaps) until the excess is covered, the kept spare last (step review 4: no release of everything every window).
    // I-0096 `no_world` (no world drawn: the title, a loading screen): every empty heap beyond the kept one at once, as ER's trim
    // does each call; in the world one a call, so a streaming walk never pays several heap releases in one frame.
    size_t released{};uint64_t local_released{},nonlocal_released{};
    for(const bool spare_pass:{false,true})for(auto* pool:{placed_textures_.get(),placed_small_.get(),placed_buffers_.get()})if(pool){
        if(spare_pass && (!local_over || local_released>=local_over))break;
        std::vector<std::unique_ptr<PlacedPool::Heap>> spares;
        const uint64_t completed=pool->fence?pool->fence->GetCompletedValue():0;
        {const std::lock_guard hold(pool->lock);pool->Reclaim(completed);bool kept=spare_pass;
            for(auto at=pool->heaps.begin();at!=pool->heaps.end();){const auto& free=(*at)->free;
                if(free.size()!=1 || free.begin()->first!=0 || free.begin()->second!=pool->heap_bytes){++at;continue;}
                if(!kept){kept=true;++at;continue;}
                if(local_over && local_released>=local_over)break;
                local_released+=pool->heap_bytes;spares.push_back(std::move(*at));at=pool->heaps.erase(at);if(!local_over && !no_world)break;}}
        released+=spares.size();spares.clear();}
    if(nonlocal_over)if(const auto pool=staging_){std::vector<std::unique_ptr<StagingPool::Chunk>> idle;
        {const std::lock_guard hold(pool->lock);
            while(!pool->free.empty() && nonlocal_released<nonlocal_over){auto* chunk=pool->free.back();pool->free.pop_back();pool->bytes-=chunk->bytes;nonlocal_released+=chunk->bytes;
                std::erase_if(pool->chunks,[&](auto& owned){if(owned.get()!=chunk)return false;idle.push_back(std::move(owned));return true;});}}
        released+=idle.size();}   // released after the lock (pre-merge review M1 of I-0079)
    return released;
}
StreamedMemoryCounters DeviceQueues::StreamedMemory() const {
    StreamedMemoryCounters c;size_t index{};
    for(auto* pool:{placed_textures_.get(),placed_small_.get(),placed_buffers_.get()}){auto& p=c.pools[index++];if(!pool)continue;
        {const std::lock_guard hold(pool->lock);c.heaps+=pool->heaps.size();p.heaps=pool->heaps.size();p.heap_bytes=p.heaps*pool->heap_bytes;
            for(const auto& heap:pool->heaps)if(heap->free.size()==1 && !heap->free.begin()->first && heap->free.begin()->second==pool->heap_bytes)++p.empty_heaps;}
        p.live_bytes=pool->live_bytes;
        c.placed+=pool->placed;c.committed+=pool->committed;c.placed_live_bytes+=pool->live_bytes;}
    if(staging_){c.staging_chunks=staging_->created;c.staging_dedicated=staging_->dedicated;c.staging_bytes=staging_->bytes;}
    return c;
}
void DeviceQueues::Open(bool warp, bool debug) {
    if (device) throw std::logic_error("device already open");
    {const std::lock_guard hold(ingress_lock_);committed_.clear();}   // a late commit against a closed device is never submitted
    // I-0082 (I-0080 lead G1): the layer is asked for, never required: without the Windows Graphics Tools the device opens
    // without it, as ER tolerates a missing layer (I-0080 ER-STUDY 7).
    // I-0132 R11f: the Agility SDK's device factory when its folder is given (not on WARP: the software adapter keeps the inbox
    // runtime); the debug layer and DRED are then the factory's configuration (its devices only), else the process's as before. A
    // factory that cannot be made or configured, or whose device cannot be created, leaves the inbox runtime (agility_failure_ says
    // why; the start row native_device_caps gives the runtime's version).
    agility_failure_.clear();ComPtr<ID3D12DeviceFactory> device_factory;
    const auto failed=[&](const char* step,HRESULT status){char text[160]{};sprintf_s(text,"%s: HRESULT %08lX",step,static_cast<unsigned long>(status));
        agility_failure_=text;device_factory.Reset();};
    if(!agility_sdk_folder.empty() && warp)agility_failure_="warp: the inbox runtime";   // the step review's L3
    // The system DXGI loaded before the Agility SDK's factory: with Windows 11's D3D12 runtime 26100.9818 (KB5132061, 2026-11-11),
    // a DXGI loaded after CreateDeviceFactory makes the hardware adapter refuse a device (DXGI_ERROR_UNSUPPORTED, the factory's and the
    // inbox runtime's alike), loaded before it the device opens (I-0132 R12f, the launcher's rebuild of 2a9e0b7b: its hardware tests;
    // DS3's process has DXGI loaded already, so the game was not affected).
    if(!agility_sdk_folder.empty() && !warp){ComPtr<IDXGIFactory1> loaded;(void)SystemFactory(0,IID_PPV_ARGS(&loaded));}
    if(!agility_sdk_folder.empty() && !warp){
        // The final review's L4: the path as plain ASCII (its short form where the folder's long name is not), so its encoding cannot
        // matter; L1: D3D12GetInterface looked up, not imported (a runtime without it keeps the inbox device instead of refusing the module).
        std::wstring wide=agility_sdk_folder;
        if(std::any_of(wide.begin(),wide.end(),[](wchar_t c){return c>127;})){std::wstring shortened(MAX_PATH,L'\0');
            const DWORD n=GetShortPathNameW(agility_sdk_folder.c_str(),shortened.data(),MAX_PATH);wide=n && n<MAX_PATH?shortened.substr(0,n):std::wstring{};}
        using GetInterface=HRESULT(WINAPI*)(REFCLSID,REFIID,void**);
        const HMODULE runtime=GetModuleHandleW(L"d3d12.dll");
        const auto get_interface=runtime?reinterpret_cast<GetInterface>(GetProcAddress(runtime,"D3D12GetInterface")):nullptr;
        if(wide.empty() || std::any_of(wide.begin(),wide.end(),[](wchar_t c){return c>127;}))agility_failure_="sdk folder: no ASCII path";
        else if(!get_interface)agility_failure_="sdk configuration: D3D12GetInterface missing";
        else{std::string folder;folder.reserve(wide.size());for(const wchar_t c:wide)folder.push_back(static_cast<char>(c));   // ASCII: checked above
            ComPtr<ID3D12SDKConfiguration1> configuration;
            HRESULT status=get_interface(CLSID_D3D12SDKConfiguration,IID_PPV_ARGS(&configuration));
            if(FAILED(status))failed("sdk configuration",status);
            else if(FAILED(status=configuration->CreateDeviceFactory(kAgilitySdkVersion,folder.c_str(),IID_PPV_ARGS(&device_factory))))failed("device factory",status);}}
    // The debug layer and DRED (I-0082 DESIGN 4: auto-breadcrumbs and page faults, read by device_removal.cpp when the device is lost),
    // set before the device exists, from the factory or the process.
    const auto configure=[&](ID3D12DeviceFactory* from){
        debug_layer_active_=false;removal_data_active_=false;
        const auto get=[&](REFCLSID clsid,REFIID iid,void** out){return from?from->GetConfigurationInterface(clsid,iid,out):D3D12GetDebugInterface(iid,out);};
        if(debug){ComPtr<ID3D12Debug> layer;
            if(SUCCEEDED(get(CLSID_D3D12Debug,IID_PPV_ARGS(&layer)))){
                layer->EnableDebugLayer();debug_layer_active_=true;
                if(gpu_based_validation.load()){ComPtr<ID3D12Debug1> validation;Check(layer.As(&validation),"GPU-based validation interface");validation->SetEnableGPUBasedValidation(TRUE);}}}
        if(device_removal_data.load()){ComPtr<ID3D12DeviceRemovedExtendedDataSettings> dred;
            if(SUCCEEDED(get(CLSID_D3D12DeviceRemovedExtendedData,IID_PPV_ARGS(&dred)))){dred->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
                dred->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);removal_data_active_=true;}}};
    // The final review's L1: a factory whose configuration throws (GPU-based validation's interface) leaves the inbox runtime too.
    if(device_factory){try{configure(device_factory.Get());}catch(const std::exception& error){agility_failure_=std::string("factory configuration: ")+error.what();device_factory.Reset();}}
    if(!device_factory)configure(nullptr);
    // The DXGI factory with its debug flag where the layer is on; the step review's M1: the Agility SDK ships its own debug layer, so
    // the layer can be on without the Graphics Tools' dxgidebug.dll: the factory is then made without the flag (I-0082: the debug
    // layer asked for, never required).
    bool factory_debug=debug_layer_active_;
    const auto make_factory=[&]{if(factory_debug && SUCCEEDED(SystemFactory(DXGI_CREATE_FACTORY_DEBUG,IID_PPV_ARGS(&factory))))return;
        factory_debug=false;factory.Reset();Check(SystemFactory(0,IID_PPV_ARGS(&factory)),"system factory");};
    make_factory();dxgi_debug_=factory_debug;
    ComPtr<IDXGIAdapter1> adapter;LUID adapter_luid{};
    // I-0084 (lead G3): WARP, the software adapter, renders a world frame in seconds: its completion deadline is 120 s, not 5 s.
    completion_deadline_ms_=warp?120000:5000;
    if (warp) Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)),"WARP");
    else {
        for (UINT i=0;;++i) {
            auto result=factory->EnumAdapterByGpuPreference(i,DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,IID_PPV_ARGS(&adapter));
            if (result==DXGI_ERROR_NOT_FOUND) throw std::runtime_error("no native hardware adapter");
            Check(result,"adapter");
            DXGI_ADAPTER_DESC1 desc{}; Check(adapter->GetDesc1(&desc),"adapter description");adapter_luid=desc.AdapterLuid;   // I-0132 R11f
            if (!(desc.Flags&DXGI_ADAPTER_FLAG_SOFTWARE) &&
                SUCCEEDED(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,__uuidof(ID3D12Device),nullptr))) break;
            adapter.Reset();
        }
    }
    if(device_factory){const HRESULT status=device_factory->CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device));
        if(FAILED(status)){failed("factory device",status);device.Reset();configure(nullptr);   // the inbox runtime, configured as before
            // The step review's L2: the DXGI factory made again when the inbox layer differs, the adapter taken again from it by its
            // LUID (the final review's L3: the swap chain is made on this factory).
            if(debug_layer_active_!=factory_debug){factory_debug=debug_layer_active_;make_factory();dxgi_debug_=factory_debug;adapter.Reset();
                Check(factory->EnumAdapterByLuid(adapter_luid,IID_PPV_ARGS(&adapter)),"adapter by luid");}}}   // the luid the loop read (its step review L2)
    debug=debug_layer_active_;
    if(!device)Check(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)),"native device");
    // I-0109 (R0, RT-PT-PLAN §3.1): the level asked is only a minimum (one device per adapter, final review L4): what the RT path
    // needs is the device's highest level and its capabilities, read here (12_2, DXR 1.2 and SM 6.8 from the inbox runtime on
    // Thomas's PC; I-0132 R11f: the Agility SDK's runtime when its factory made the device); DS3's DXBC programmes and the raster
    // passes run as before.
    capabilities_={};capabilities_.feature_level=D3D_FEATURE_LEVEL_11_0;   // guaranteed by the creation, should the query fail
    {const D3D_FEATURE_LEVEL asked[]{D3D_FEATURE_LEVEL_11_0,D3D_FEATURE_LEVEL_11_1,D3D_FEATURE_LEVEL_12_0,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_2};
        D3D12_FEATURE_DATA_FEATURE_LEVELS levels{UINT(std::size(asked)),asked};
        if(SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS,&levels,sizeof levels)))capabilities_.feature_level=levels.MaxSupportedFeatureLevel;}
    (void)device.As(&device5);
    // An older runtime answers E_INVALIDARG to a shader model it does not know: ask again one model lower.
    for(int model=D3D_HIGHEST_SHADER_MODEL;model>=D3D_SHADER_MODEL_6_0;--model){
        D3D12_FEATURE_DATA_SHADER_MODEL highest{static_cast<D3D_SHADER_MODEL>(model)};
        if(SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL,&highest,sizeof highest))){capabilities_.shader_model=highest.HighestShaderModel;break;}}
    {D3D12_FEATURE_DATA_D3D12_OPTIONS o{};if(SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS,&o,sizeof o)))capabilities_.binding=o.ResourceBindingTier;}
    {D3D12_FEATURE_DATA_D3D12_OPTIONS4 o{};if(SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4,&o,sizeof o)))capabilities_.native_16bit=o.Native16BitShaderOpsSupported;}
    {D3D12_FEATURE_DATA_D3D12_OPTIONS5 o{};if(SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5,&o,sizeof o)))capabilities_.raytracing=o.RaytracingTier;}
    {D3D12_FEATURE_DATA_D3D12_OPTIONS7 o{};if(SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7,&o,sizeof o)))capabilities_.mesh=o.MeshShaderTier;}
    {D3D12_FEATURE_DATA_D3D12_OPTIONS12 o{};if(SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12,&o,sizeof o)))capabilities_.enhanced_barriers=o.EnhancedBarriersSupported;}
    {D3D12_FEATURE_DATA_D3D12_OPTIONS16 o{};if(SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS16,&o,sizeof o)))capabilities_.gpu_upload=o.GPUUploadHeapSupported;}   // I-0132 R11f
    {ComPtr<ID3D12DeviceConfiguration> configuration;if(SUCCEEDED(device.As(&configuration)))capabilities_.sdk_version=configuration->GetDesc().SDKVersion;}   // I-0132 R11f
    gpu_upload_heaps.store(capabilities_.gpu_upload && gpu_upload_allowed.load(std::memory_order_relaxed),std::memory_order_relaxed);
    if(!device5)capabilities_.raytracing=D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
    if (debug) Check(device.As(&diagnostics),"diagnostics");
    if (debug) {
        Check(device.As(&callbacks_),"diagnostic callback interface");
        Check(callbacks_->RegisterMessageCallback(DiagnosticCallback,
            D3D12_MESSAGE_CALLBACK_FLAG_NONE,nullptr,&callback_cookie_),"diagnostic callback");
        callback_registered_=true;
    }
    D3D12_COMMAND_QUEUE_DESC q{}; q.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;
    Check(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)),"direct queue");
    queue->SetName(L"native direct queue");   // I-0082: the DRED breadcrumbs name it
    Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence_)),"fence");
    ingress_=std::make_shared<IngressPool>();
    descriptors_=std::make_shared<DescriptorPool>();descriptors_->device=device.Get();
    descriptors_->stride=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    // I-0079: the streamed creation memory (CreateStreamed, AllocateStaging).
    placed_textures_=std::make_shared<PlacedPool>();placed_textures_->device=device.Get();placed_textures_->flags=D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
    placed_small_=std::make_shared<PlacedPool>();placed_small_->device=device.Get();placed_small_->flags=D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;placed_small_->heap_bytes=placed_small_heap_bytes;
    placed_buffers_=std::make_shared<PlacedPool>();placed_buffers_->device=device.Get();placed_buffers_->flags=D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
    for(auto* pool:{placed_textures_.get(),placed_small_.get(),placed_buffers_.get()}){pool->fence=fence_.Get();pool->submitted=&submitted_;}
    staging_=std::make_shared<StagingPool>();staging_->device=device.Get();
}
struct IngressRecording::Pending {
    ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> list;
    std::vector<std::shared_ptr<void>> kept;uint64_t serial{};
};
struct DeviceQueues::IngressPool {std::mutex lock;std::vector<std::unique_ptr<IngressRecording::Pending>> free;};
IngressRecording::IngressRecording(DeviceQueues& owner,std::unique_ptr<Pending> pending) noexcept:owner_(&owner),pending_(std::move(pending)){}
IngressRecording::IngressRecording(IngressRecording&& other) noexcept:owner_(other.owner_),pending_(std::move(other.pending_)){}
IngressRecording::~IngressRecording() {
    // Never submitted: close it and return it for reuse.
    if(pending_){(void)pending_->list->Close();DeviceQueues::RecycleIngress(owner_->ingress_,std::move(pending_));}
}
ID3D12GraphicsCommandList* IngressRecording::List() const noexcept {return pending_?pending_->list.Get():nullptr;}
void IngressRecording::Keep(std::shared_ptr<void> owner) {
    if(!pending_)throw std::logic_error("ingress recording already committed");
    if(owner)pending_->kept.push_back(std::move(owner));
}
uint64_t IngressRecording::Commit() {
    if(!pending_)throw std::logic_error("ingress recording already committed");
    Check(pending_->list->Close(),"ingress list close");
    const std::lock_guard hold(owner_->ingress_lock_);
    const auto serial=pending_->serial=++owner_->ingress_serial_;owner_->committed_.push_back(std::move(pending_));
    return serial;
}
void DeviceQueues::RecycleIngress(const std::weak_ptr<IngressPool>& pool,std::unique_ptr<IngressRecording::Pending> pending) noexcept {
    // Back to the pool (what it kept released here); beyond 32 free lists, or once the pool is gone, destroyed.
    pending->kept.clear();
    if(const auto owner=pool.lock()){const std::lock_guard hold(owner->lock);if(owner->free.size()<32)owner->free.push_back(std::move(pending));}
}
IngressRecording DeviceQueues::RecordIngress() {
    if(!device || !ingress_)throw std::runtime_error("ingress device unavailable");
    std::unique_ptr<IngressRecording::Pending> pending;
    {const std::lock_guard hold(ingress_->lock);if(!ingress_->free.empty()){pending=std::move(ingress_->free.back());ingress_->free.pop_back();}}
    if(pending){
        // Returned only after the submission that carried it completed (or never submitted).
        Check(pending->allocator->Reset(),"ingress allocator reset");Check(pending->list->Reset(pending->allocator.Get(),nullptr),"ingress list reset");
    }else{
        pending=std::make_unique<IngressRecording::Pending>();
        Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&pending->allocator)),"ingress allocator");
        Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,pending->allocator.Get(),nullptr,IID_PPV_ARGS(&pending->list)),"ingress list");
        pending->list->SetName(L"native ingress");   // I-0082: the DRED breadcrumbs name it
    }
    return IngressRecording(*this,std::move(pending));
}
CpuDescriptor DeviceQueues::AllocateCpuDescriptor() {
    if(!device || !descriptors_)throw std::runtime_error("descriptor device unavailable");
    auto pool=descriptors_;D3D12_CPU_DESCRIPTOR_HANDLE handle{};
    {const std::lock_guard hold(pool->lock);
        if(pool->free.empty()){
            constexpr UINT count=1024;D3D12_DESCRIPTOR_HEAP_DESC desc{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,count,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
            ComPtr<ID3D12DescriptorHeap> heap;Check(CountedHeap(pool->device)->CreateDescriptorHeap(&desc,IID_PPV_ARGS(&heap)),"pooled CPU descriptor heap");
            const auto start=heap->GetCPUDescriptorHandleForHeapStart();pool->heaps.push_back(heap);pool->free.reserve((pool->heaps.size())*count);   // every slot can come back without a reallocation in a deleter
            for(UINT i=count;i-->0;)pool->free.push_back({start.ptr+SIZE_T(i)*pool->stride});}
        handle=pool->free.back();pool->free.pop_back();++pool->in_use;}
    // The lease keeps the pool (and its heaps) alive; it returns the slot when the last holder lets go. If the control
    // block cannot be allocated, the deleter runs at once and the slot goes back.
    CpuDescriptor result;result.handle=handle;
    result.lease=std::shared_ptr<void>(reinterpret_cast<void*>(handle.ptr),[pool](void* slot){
        const std::lock_guard hold(pool->lock);pool->free.push_back({reinterpret_cast<SIZE_T>(slot)});--pool->in_use;});
    return result;
}
size_t DeviceQueues::CpuDescriptorsInUse() const {
    const auto pool=descriptors_;if(!pool)return 0;const std::lock_guard hold(pool->lock);return pool->in_use;
}
uint64_t DeviceQueues::SubmitIngress() {return SubmitWithIngress({});}
uint64_t DeviceQueues::Execute(ID3D12CommandList* list) {
    if(!list)throw std::invalid_argument("native submission list");
    return SubmitWithIngress({&list,1});
}
uint64_t DeviceQueues::Execute(std::span<ID3D12CommandList* const> lists) {
    if(lists.empty() || std::find(lists.begin(),lists.end(),nullptr)!=lists.end())throw std::invalid_argument("native submission lists");
    return SubmitWithIngress(lists);
}
uint64_t DeviceQueues::SubmitWithIngress(std::span<ID3D12CommandList* const> submitted) {
    const bool list=!submitted.empty();
    // R13 cR1: the whole submission under the submission mutex (ingress swap to retirement records), so the tickets reach
    // the fence in submission order and the ingress lists swapped here run ahead of this caller's list.
    const std::lock_guard submission(submit_mutex_);
    // B3a: the committed ingress lists go first, in commit order, in the same call.
    std::vector<std::unique_ptr<IngressRecording::Pending>> ingress;
    {const std::lock_guard hold(ingress_lock_);ingress.swap(committed_);}
    std::vector<ID3D12CommandList*> lists;lists.reserve(ingress.size()+submitted.size());
    for(const auto& pending:ingress)lists.push_back(pending->list.Get());
    lists.insert(lists.end(),submitted.begin(),submitted.end());
    if(lists.empty())return 0;
    ++executes;ingress_lists+=ingress.size();
    queue->ExecuteCommandLists(static_cast<UINT>(lists.size()),lists.data());
    // Submission and completion are separate facts. Failure stops this owner;
    // no legacy rendering call can be used as a fallback.
    const uint64_t ticket=next_++;
    try {Check(queue->Signal(fence_.Get(),ticket),"native signal");}
    catch(...) {for(auto& pending:ingress)(void)pending.release();throw;}   // submitted without proof of completion
    submitted_=ticket;
    // Each owner exists before its list moves into it; a list whose retirement could not be recorded is in flight and
    // kept to process exit, never recycled early.
    struct Held {
        std::weak_ptr<IngressPool> pool;std::unique_ptr<IngressRecording::Pending> pending;
        ~Held(){if(pending)RecycleIngress(pool,std::move(pending));}
    };
    try {
        for(auto& pending:ingress){
            auto held=std::make_shared<Held>();held->pool=ingress_;held->pending=std::move(pending);
            try {RetireAfter(ticket,held);}catch(...){(void)held->pending.release();throw;}   // kept to process exit
        }
    }catch(...){for(auto& pending:ingress)(void)pending.release();throw;}
    if(list)RetireRecordingOwners(*this,ticket);   // pre-merge review: the owners retired while this thread recorded `list`
    return ticket;
}
uint64_t DeviceQueues::Signal() {
    const std::lock_guard submission(submit_mutex_);
    const uint64_t ticket=next_++;
    Check(queue->Signal(fence_.Get(),ticket),"owner frame signal");submitted_=ticket;return ticket;
}
uint64_t DeviceQueues::Drain(uint8_t site) {
    uint64_t ticket{};
    {const std::lock_guard submission(submit_mutex_);   // the ticket and its signal; the wait outside the mutex
        ++drains;++completes;   // a drain is one more completion wait, as before cD0
        gpu_audit.drains[site<WaitSiteCount?site:WaitOther].fetch_add(1,std::memory_order_relaxed);
        ticket=next_++;
        Check(queue->Signal(fence_.Get(),ticket),"owner drain signal"); submitted_=ticket;}
    Wait(ticket); return ticket;
}
void DeviceQueues::Complete(uint64_t value,uint8_t site) {
    // "completes" counts calls; a call whose fence has already passed does not block.
    ++completes;gpu_audit.completes[site<WaitSiteCount?site:WaitOther].fetch_add(1,std::memory_order_relaxed);
    Wait(value);
}
void DeviceQueues::CompleteOn(uint64_t value,HANDLE event,uint8_t site) {
    if(!event)throw std::invalid_argument("completion event");
    ++completes;gpu_audit.completes[site<WaitSiteCount?site:WaitOther].fetch_add(1,std::memory_order_relaxed);
    WaitOn(value,event);
}
void DeviceQueues::WaitOn(uint64_t value,HANDLE event) {
    if (!fence_ || fence_->GetCompletedValue()==UINT64_MAX) throw std::runtime_error("native device removed");
    if (fence_->GetCompletedValue()<value) {
        ResetEvent(event);   // a registration left by an earlier wait must not wake this one
        Check(fence_->SetEventOnCompletion(value,event),"completion registration");
        // R13 cR1: a wake-up below the value (a stale registration) waits again, until the deadline.
        const auto deadline=GetTickCount64()+completion_deadline_ms_;
        while (fence_->GetCompletedValue()<value) {
            const auto now=GetTickCount64();
            if (now>=deadline) {gpu_hang_detected.store(true);throw std::runtime_error("native completion timeout");}   // I-0082: reported as a GPU hang
            if (WaitForSingleObject(event,static_cast<DWORD>(deadline-now))==WAIT_FAILED) throw std::runtime_error("native completion wait failed");
        }
    }
    if (fence_->GetCompletedValue()<value || fence_->GetCompletedValue()==UINT64_MAX) throw std::runtime_error("invalid completion");
}
void DeviceQueues::Wait(uint64_t value) {
    // R13 cR1: an event of its own, from the pool; one whose wait failed keeps its registration and is never reused.
    if (fence_ && fence_->GetCompletedValue()>=value && fence_->GetCompletedValue()!=UINT64_MAX) return;   // passed: no event (review)
    HANDLE event{};
    {const std::lock_guard hold(events_lock_);if(!free_events_.empty()){event=free_events_.back();free_events_.pop_back();}}
    if (!event) {event=CreateEventW(nullptr,FALSE,FALSE,nullptr);if (!event) throw std::runtime_error("completion event");}
    WaitOn(value,event);
    {const std::lock_guard hold(events_lock_);free_events_.push_back(event);}
}
void DeviceQueues::RetireAfter(uint64_t ticket,std::shared_ptr<void> owner) {
    if (!owner) return;
    if (!ticket || ticket>=next_) throw std::invalid_argument("retirement ticket not submitted");
    const std::lock_guard hold(retire_lock_);retired_.emplace_back(ticket,std::move(owner));
}
size_t DeviceQueues::CollectCompleted() {
    if (!fence_) return 0;
    const auto completed=fence_->GetCompletedValue();
    if (completed==UINT64_MAX) throw std::runtime_error("native device removed");
    std::vector<std::shared_ptr<void>> released;
    // C4-2 review: one pass (a reset can retire thousands of kept draws at once; an erase per entry was quadratic).
    {const std::lock_guard hold(retire_lock_);
        const auto kept=std::stable_partition(retired_.begin(),retired_.end(),[&](const auto& entry){return entry.first>completed;});
        for (auto it=kept;it!=retired_.end();++it) released.push_back(std::move(it->second));
        retired_.erase(kept,retired_.end());}
    return released.size();   // owners released here, outside the lock
}
uint64_t DeviceQueues::CompletedValue() const {
    const auto completed=fence_?fence_->GetCompletedValue():0;
    if(completed==UINT64_MAX)throw std::runtime_error("native device removed");
    return completed;
}
size_t DeviceQueues::PendingRetirements() const {const std::lock_guard hold(retire_lock_);return retired_.size();}
void DeviceQueues::Close() {
    // Present may enqueue additional work after the rendering fence. Drain the
    // owner queue before release even on an exception during presentation.
    if (queue && fence_) Drain();
    // Everything submitted has completed. Owners are released outside the lock, repeatedly: releasing one may retire another.
    for(;;){std::vector<std::pair<uint64_t,std::shared_ptr<void>>> released;
        {const std::lock_guard hold(retire_lock_);released.swap(retired_);}if(released.empty())break;}
    {const std::lock_guard hold(ingress_lock_);committed_.clear();}   // committed, never submitted
    ingress_.reset();descriptors_.reset();   // leased descriptors keep their pool
    placed_textures_.reset();placed_small_.reset();placed_buffers_.reset();staging_.reset();   // I-0079: live placed resources keep their heaps; their leases find no pool
    if (callback_registered_) {
        Check(callbacks_->UnregisterMessageCallback(callback_cookie_),"unregister diagnostic callback");
        callback_registered_=false;
    }
    callbacks_.Reset();
    fence_.Reset(); queue.Reset(); diagnostics.Reset(); device5.Reset(); device.Reset(); factory.Reset();
    {const std::lock_guard hold(events_lock_);for (const auto event:free_events_) CloseHandle(event);free_events_.clear();}
    submitted_=0; next_=1;
}
void DeviceQueues::QuarantineToProcessExit() noexcept {
    // A retained device may call its diagnostic function later. Pin that code's
    // module before losing ownership; inability to retain it is process-fatal.
    if (callback_registered_) {
        HMODULE module{};
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&DiagnosticCallback),&module)) {
            TerminateProcess(GetCurrentProcess(),0x0036);
            std::terminate();
        }
    }
    {const std::lock_guard hold(retire_lock_);for (auto& entry:retired_)(void)new std::shared_ptr<void>(std::move(entry.second));retired_.clear();}
    {const std::lock_guard hold(ingress_lock_);for (auto& pending:committed_)(void)pending.release();committed_.clear();}
    if (ingress_) (void)new std::shared_ptr<IngressPool>(std::move(ingress_));
    if (descriptors_) (void)new std::shared_ptr<DescriptorPool>(std::move(descriptors_));
    for(auto* pool:{&placed_textures_,&placed_small_,&placed_buffers_})if(*pool){
        {const std::lock_guard hold((*pool)->lock);(*pool)->fence=nullptr;(*pool)->submitted=nullptr;}   // review L4: leases may outlive this object
        (void)new std::shared_ptr<PlacedPool>(std::move(*pool));}
    if (staging_) (void)new std::shared_ptr<StagingPool>(std::move(staging_));
    (void)callbacks_.Detach();
    (void)fence_.Detach(); (void)queue.Detach(); (void)diagnostics.Detach();
    (void)device5.Detach(); (void)device.Detach(); (void)factory.Detach();
    // An outstanding fence registration can still target this event. Keep it
    // and the entire GPU graph until process exit on unproved completion.
    {const std::lock_guard hold(events_lock_);free_events_.clear();}   // cR1: events kept to process exit (a registration may target them)
}
DeviceQueues::DeviceQueues()=default;
DeviceQueues::~DeviceQueues() {
    // Device-loss path ends this experimental owner; normal shutdown must pass
    // Close explicitly. No unqualified completed/retired receipt is emitted here.
    for (const auto event:free_events_) CloseHandle(event);   // cR1: events of completed waits (a failed wait's event is kept)
}

void FrameRecorder::Open(ID3D12Device* device) {
    Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"allocator");
    Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)),"list");
    Check(list->Close(),"initial list close");
}
std::shared_ptr<const void> HoldResource(ID3D12Resource* resource){
    if(!resource)throw std::invalid_argument("held frame resource");
    resource->AddRef();return std::shared_ptr<const void>(resource,[](const void* held){static_cast<ID3D12Resource*>(const_cast<void*>(held))->Release();});
}
std::shared_ptr<void> HoldObject(IUnknown* object){
    if(!object)throw std::invalid_argument("held object");
    object->AddRef();return std::shared_ptr<void>(object,[](void* held){static_cast<IUnknown*>(held)->Release();});
}
void FrameRing::Open(ID3D12Device* device,uint32_t slots){
    if(!device || !slots || slots>max_frame_slots || !slots_.empty())throw std::invalid_argument("frame ring");
    slots_.resize(slots);
    for(auto& slot:slots_){
        Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&slot.allocator)),"frame slot allocator");
        Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,slot.allocator.Get(),nullptr,IID_PPV_ARGS(&slot.list)),"frame slot list");
        slot.list->SetName(L"native frame slot");   // I-0082: the DRED breadcrumbs name it
        Check(slot.list->Close(),"frame slot initial close");
    }
    next_=0;recording_=false;
}
void FrameRing::Retire(FrameSlot& slot){
    LARGE_INTEGER frequency{},before{};QueryPerformanceFrequency(&frequency);QueryPerformanceCounter(&before);
    struct Timed {std::atomic<double>& total;LARGE_INTEGER frequency,before;~Timed(){LARGE_INTEGER after{};QueryPerformanceCounter(&after);
        total.fetch_add(1000.0*double(after.QuadPart-before.QuadPart)/double(frequency.QuadPart));}} timed{release_ms_,frequency,before};
    // Only once the slot ticket has completed: the chunks its draws return come back at once (complement 4.4).
    // cD4: the slot's completion work first (its readbacks are among the kept objects); the slot is released even when a
    // completion throws, and the first exception is rethrown after.
    auto completions=std::move(slot.completions);slot.completions.clear();std::exception_ptr failure;
    for(auto& completion:completions){try{completion();}catch(...){if(!failure)failure=std::current_exception();}}
    {const ReleaseAfterCompletion completed;
        if(!HandOff(slot)){slot.draws.clear();slot.kept.clear();}   // I-0090: or to the release thread
        slot.referenced.clear();slot.ticket=0;}
    if(failure)std::rethrow_exception(failure);
}
// I-0090: the release thread's batches (a retired slot's draws and kept objects) and the emptied vectors kept for reuse.
struct FrameRing::Releaser {
    struct Batch {std::vector<std::shared_ptr<const void>> draws,kept;};
    std::mutex lock;std::condition_variable wake,idle;
    std::vector<Batch> queue,spare;   // queue: handed, not yet destroyed; spare: emptied, their capacity kept for the next hand-off
    bool stop{},busy{};
    std::thread thread;
};
// The slot's holders to the release thread (started at the first hand-off); false when they are to be released here: deferred
// release off, a thread that cannot start, or four batches already waiting (the thread is behind).
bool FrameRing::HandOff(FrameSlot& slot) noexcept {
    if(!deferred_release_)return false;
    if(slot.draws.empty() && slot.kept.empty())return true;
    try{
        if(!releaser_){auto releaser=std::make_unique<Releaser>();auto* r=releaser.get();auto* spent=&release_thread_ms_;
            releaser->thread=std::thread([r,spent]{
                SetThreadDescription(GetCurrentThread(),L"ds3-native-release");
                const ReleaseAfterCompletion completed;   // every batch's slot ticket has passed (Retire runs after its wait)
                LARGE_INTEGER frequency{};QueryPerformanceFrequency(&frequency);
                std::unique_lock hold(r->lock);
                for(;;){r->wake.wait(hold,[&]{return r->stop || !r->queue.empty();});
                    if(r->queue.empty())break;   // stopped, nothing left
                    auto batch=std::move(r->queue.front());r->queue.erase(r->queue.begin());r->busy=true;hold.unlock();
                    LARGE_INTEGER before{},after{};QueryPerformanceCounter(&before);
                    batch.draws.clear();batch.kept.clear();
                    QueryPerformanceCounter(&after);spent->fetch_add(1000.0*double(after.QuadPart-before.QuadPart)/double(frequency.QuadPart));
                    hold.lock();r->busy=false;
                    if(r->spare.size()<4){try{r->spare.push_back(std::move(batch));}catch(...){}}
                    r->idle.notify_all();}
                r->idle.notify_all();});
            releaser_=std::move(releaser);}
        const std::lock_guard hold(releaser_->lock);
        if(releaser_->stop || releaser_->queue.size()>=4)return false;
        Releaser::Batch batch;if(!releaser_->spare.empty()){batch=std::move(releaser_->spare.back());releaser_->spare.pop_back();}
        releaser_->queue.push_back(std::move(batch));auto& handed=releaser_->queue.back();
        handed.draws.swap(slot.draws);handed.kept.swap(slot.kept);   // the slot takes the emptied vectors
        releaser_->wake.notify_one();return true;
    }catch(...){return false;}
}
void FrameRing::Flush() noexcept {
    if(!releaser_)return;
    std::unique_lock hold(releaser_->lock);releaser_->idle.wait(hold,[&]{return releaser_->queue.empty() && !releaser_->busy;});
}
void FrameRing::StopReleaser() noexcept {
    if(!releaser_)return;
    {const std::lock_guard hold(releaser_->lock);releaser_->stop=true;}releaser_->wake.notify_all();
    if(releaser_->thread.joinable())releaser_->thread.join();   // the thread empties its queue first
    releaser_.reset();
}
void FrameRing::WaitSlot(DeviceQueues& queues,FrameSlot& slot,uint8_t site,bool release){
    if(!slot.ticket)return;
    if(queues.CompletedValue()<slot.ticket){
        LARGE_INTEGER frequency{},before{},after{};QueryPerformanceFrequency(&frequency);QueryPerformanceCounter(&before);
        if(!event_){event_=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!event_)throw std::runtime_error("frame ring event");}
        queues.CompleteOn(slot.ticket,event_,site);
        QueryPerformanceCounter(&after);wait_ms_.fetch_add(1000.0*double(after.QuadPart-before.QuadPart)/double(frequency.QuadPart));}
    if(release)Retire(slot);
}
void FrameRing::WaitNext(DeviceQueues& queues,uint8_t site){
    // The wait only: the release (completions, draws, chunks) stays in Begin, under the caller's submission lock (review); I-0090:
    // with deferred release the holders' destruction then runs on the ring's release thread.
    if(slots_.empty() || recording_)throw std::invalid_argument("frame ring wait");
    WaitSlot(queues,slots_[next_],site,false);
}
FrameRing::FrameRing()=default;
FrameRing::~FrameRing(){StopReleaser();if(event_)CloseHandle(event_);}
FrameSlot& FrameRing::Begin(DeviceQueues& queues,uint8_t site,ID3D12PipelineState* initial){
    if(slots_.empty() || recording_)throw std::invalid_argument("frame ring begin");
    auto& slot=slots_[next_];
    WaitSlot(queues,slot,site);
    Check(slot.allocator->Reset(),"frame slot allocator reset");Check(slot.list->Reset(slot.allocator.Get(),initial),"frame slot list reset");
    slot.extras_used=0;recording_=true;++frames_;++numbered_;return slot;
}
ID3D12GraphicsCommandList* FrameRing::ExtraList(ID3D12Device* device,FrameSlot& slot){
    if(!recording_ || &slot!=&slots_[next_] || !device)throw std::invalid_argument("frame ring extra list");
    if(slot.extras_used==slot.extras.size()){FrameSlot::Extra extra;
        Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&extra.allocator)),"frame slot extra allocator");
        Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,extra.allocator.Get(),nullptr,IID_PPV_ARGS(&extra.list)),"frame slot extra list");
        extra.list->SetName(L"native frame chunk");
        slot.extras.push_back(std::move(extra));return slot.extras[slot.extras_used++].list.Get();}
    auto& extra=slot.extras[slot.extras_used++];   // Begin waited for the slot's ticket: nothing of its last recording is in flight
    Check(extra.allocator->Reset(),"frame slot extra allocator reset");Check(extra.list->Reset(extra.allocator.Get(),nullptr),"frame slot extra list reset");
    return extra.list.Get();
}
FrameSlot& FrameRing::Current(){
    if(slots_.empty() || !recording_)throw std::invalid_argument("frame ring not recording");
    return slots_[next_];
}
void FrameRing::RequireHeld(const FrameSlot& slot){
    for(const auto* resource:slot.referenced)
        if(std::none_of(slot.kept.begin(),slot.kept.end(),[&](const std::shared_ptr<const void>& held){return held.get()==resource;}))
            throw std::logic_error("frame slot references a resource it does not hold");
}
void FrameRing::End(DeviceQueues& queues,FrameSlot& slot,uint64_t ticket,uint8_t site){
    if(!recording_ || &slot!=&slots_[next_] || !ticket)throw std::invalid_argument("frame ring end");
    // The slot is submitted from here on, whatever the wait below does: a throw leaves it held (the renderer is then
    // faulted, and Close after its drain or the quarantine settles the slot).
    slot.ticket=ticket;recording_=false;next_=(next_+1)%slots_.size();
    if(slots_.size()==1)WaitSlot(queues,slot,site);
}
void FrameRing::WaitIdle(DeviceQueues& queues,uint8_t site){
    if(recording_)throw std::invalid_argument("frame ring idle while recording");
    // Every slot, even after one failed (review): the first exception is rethrown once all are waited for and released.
    std::exception_ptr failure;
    for(auto& slot:slots_){try{WaitSlot(queues,slot,site);}catch(...){if(!failure)failure=std::current_exception();}}
    Flush();   // I-0090: idle means every retired holder destroyed
    if(failure)std::rethrow_exception(failure);
}
void FrameRing::Close() noexcept {StopReleaser();slots_.clear();next_=0;frames_=0;recording_=false;}
void FrameRing::QuarantineToProcessExit() noexcept {
    event_=nullptr;   // a failed wait may still be registered on it: never closed
    StopReleaser();   // I-0090: its batches were retired after their tickets passed
    for(auto& slot:slots_)if(!slot.completions.empty())(void)new std::vector<std::function<void()>>(std::move(slot.completions));
    for(auto& slot:slots_){(void)slot.allocator.Detach();(void)slot.list.Detach();
        for(auto& extra:slot.extras){(void)extra.allocator.Detach();(void)extra.list.Detach();}   // I-0074 review M1: the chunk lists too
        if(!slot.draws.empty())(void)new std::vector<std::shared_ptr<const void>>(std::move(slot.draws));
        if(!slot.kept.empty())(void)new std::vector<std::shared_ptr<const void>>(std::move(slot.kept));}
    slots_.clear();next_=0;recording_=false;
}
void FramePresenter::Acquire(ID3D12Device* device) {
    const auto base=rtv->GetCPUDescriptorHandleForHeapStart();
    buffers.resize(count);
    for (UINT i=0;i<count;++i) {
        Check(swapchain->GetBuffer(i,IID_PPV_ARGS(&buffers[i])),"swapchain buffer");
        const D3D12_CPU_DESCRIPTOR_HANDLE view{base.ptr+static_cast<SIZE_T>(i)*stride};
        device->CreateRenderTargetView(buffers[i].Get(),nullptr,view);
    }
    stand_in=false;   // final review L1: the chain's own buffers again
}
void FramePresenter::Open(DeviceQueues& owner, HWND window, uint32_t w, uint32_t h) {
    width=w; height=h; presented=window; upgraded=false; upgrade_failure.clear();
    format=hdr_wanted?DXGI_FORMAT_R10G10B10A2_UNORM:DXGI_FORMAT_R8G8B8A8_UNORM;hdr10=hdr_refused=false;hdr_failure.clear();   // I-0138 tranche 5
    DXGI_SWAP_CHAIN_DESC1 desc{}; desc.Width=w; desc.Height=h; desc.Format=format;
    if (count<2 || count>4) throw std::invalid_argument("native presentation buffer count");
    desc.SampleDesc.Count=1; desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT; desc.BufferCount=count;
    desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_DISCARD;
    // I-0076 (ER 141E9EAC0, DS3 FUN_14192C220): windowed at creation, exclusive full screen allowed to switch the display mode.
    if (tearing) {BOOL allowed=FALSE;
        tearing=SUCCEEDED(owner.factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING,&allowed,sizeof allowed)) && allowed;}
    desc.Flags=DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH|(waitable?DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT:0)|(tearing?DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING:0);
    ComPtr<IDXGISwapChain1> swap;
    // I-0111 (R1, manual hooking): while DLSS is ready the chain is made through Streamline's proxy of the factory, so that its
    // Present reaches Streamline's per-frame bookkeeping; a refused upgrade ends Streamline (DLSS off), the chain made as before.
    ComPtr<IDXGIFactory2> chain_factory=owner.factory;
    if(streamline::DlssReady()){ComPtr<IDXGIFactory4> proxy;std::string reason;
        if(streamline::UpgradeFactory(owner.factory.Get(),proxy.GetAddressOf(),reason) && SUCCEEDED(proxy.As(&chain_factory)))upgraded=true;
        else{chain_factory=owner.factory;upgrade_failure=reason.empty()?"proxy factory interface":reason;streamline::Shutdown();}}
    PresentationTrace trace(owner.diagnostics!=nullptr,window);
    auto status=chain_factory->CreateSwapChainForHwnd(owner.queue.Get(),window,&desc,nullptr,nullptr,&swap);
    // Step review L4: a chain the proxy refuses is made by the native factory, Streamline ended (DLSS off), the refusal named.
    if(FAILED(status) && upgraded && !IsDeviceLoss(status)){char text[64]{};sprintf_s(text,"proxy chain HRESULT %08X",unsigned(status));upgrade_failure=text;upgraded=false;
        streamline::Shutdown();swap.Reset();status=owner.factory->CreateSwapChainForHwnd(owner.queue.Get(),window,&desc,nullptr,nullptr,&swap);}
    trace.Complete(window,owner.queue.Get(),desc,status);
    Check(status,"native presentation chain");
    Check(swap.As(&swapchain),"swapchain3");
    // I-0138 tranche 5: the HDR10 colour space; a chain that refuses it is made 8-bit again (no buffer is held yet).
    if(Hdr10Format()){ApplyColorSpace();
        if(!hdr10){const std::string why=hdr_failure;format=DXGI_FORMAT_R8G8B8A8_UNORM;hdr_refused=true;
            Check(swapchain->ResizeBuffers(count,w,h,format,desc.Flags),"native presentation chain (8-bit again)");ApplyColorSpace();hdr_failure=why;}}
    if (waitable) {
        Check(swapchain->SetMaximumFrameLatency(max_latency),"native frame latency");
        latency=swapchain->GetFrameLatencyWaitableObject();
        if (!latency) throw std::runtime_error("native frame latency object");
    }
    // I-0076: as the original and ER (MakeWindowAssociation(hwnd, 3)): DXGI never changes the window or the mode by itself; the
    // engine's window code does (Alt+Enter included).
    Check(owner.factory->MakeWindowAssociation(window,DXGI_MWA_NO_WINDOW_CHANGES|DXGI_MWA_NO_ALT_ENTER),"window association");
    D3D12_DESCRIPTOR_HEAP_DESC views{}; views.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV; views.NumDescriptors=count;
    Check(CountedHeap(owner.device)->CreateDescriptorHeap(&views,IID_PPV_ARGS(&rtv)),"RTV heap");
    stride=owner.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV); Acquire(owner.device.Get());
}
FramePresenter::~FramePresenter(){if(latency)CloseHandle(latency);}
HRESULT FramePresenter::ResizeChain(uint32_t w,uint32_t h) {
    for (auto& buffer:buffers) buffer.Reset();
    const UINT flags=DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH|
        (waitable?DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT:0)|(tearing?DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING:0);   // I-0088 P0: the creation's flags kept
    // I-0138 tranche 5: the format asked now (hdr_wanted); a refused resize leaves the chain, its format and its colour space as
    // they were. The colour space is said again after an accepted resize of an HDR10 chain or to another format; a chain that
    // refuses HDR10's is made 8-bit again.
    const DXGI_FORMAT asked=hdr_wanted && !hdr_refused?DXGI_FORMAT_R10G10B10A2_UNORM:DXGI_FORMAT_R8G8B8A8_UNORM,before=format;
    auto status=swapchain->ResizeBuffers(count,w,h,asked,flags);
    if(FAILED(status))return status;
    format=asked;if(!hdr_refused)hdr_failure.clear();
    if(asked!=before || Hdr10Format())ApplyColorSpace();   // an SDR chain that stays one: nothing said, as before this tranche
    if(Hdr10Format() && !hdr10){const std::string why=hdr_failure;hdr_refused=true;status=swapchain->ResizeBuffers(count,w,h,DXGI_FORMAT_R8G8B8A8_UNORM,flags);
        format=DXGI_FORMAT_R8G8B8A8_UNORM;   // also when this resize is refused (the header: stand-ins then)
        if(SUCCEEDED(status))ApplyColorSpace();hdr_failure=why;}
    return status;
}
void FramePresenter::ApplyColorSpace() {
    hdr10=false;
    if(!Hdr10Format()){(void)swapchain->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709);return;}   // after an HDR10 chain: sRGB's again
    constexpr auto space=DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;UINT support{};
    const auto checked=swapchain->CheckColorSpaceSupport(space,&support);char text[96]{};
    if(FAILED(checked) || !(support&DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT)){
        sprintf_s(text,"HDR10 colour space not supported (HRESULT %08X, support %u)",unsigned(checked),support);hdr_failure=text;return;}
    const auto set=swapchain->SetColorSpace1(space);
    if(FAILED(set)){sprintf_s(text,"HDR10 colour space refused (HRESULT %08X)",unsigned(set));hdr_failure=text;return;}
    hdr10=true;
}
hdr::Display ProbeHdrDisplay(HWND window,IDXGIFactory1** kept) noexcept {
    hdr::Display display{};if(kept)*kept=nullptr;
    try{
        ComPtr<IDXGIFactory1> factory;if(FAILED(SystemFactory(0,IID_PPV_ARGS(&factory))))return display;
        if(kept)factory.CopyTo(kept);
        const HMONITOR monitor=MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST);
        ComPtr<IDXGIAdapter1> adapter;
        for(UINT a=0;factory->EnumAdapters1(a,adapter.ReleaseAndGetAddressOf())!=DXGI_ERROR_NOT_FOUND;++a){
            ComPtr<IDXGIOutput> output;
            for(UINT o=0;adapter->EnumOutputs(o,output.ReleaseAndGetAddressOf())!=DXGI_ERROR_NOT_FOUND;++o){
                ComPtr<IDXGIOutput6> six;DXGI_OUTPUT_DESC1 desc{};
                if(FAILED(output.As(&six)) || FAILED(six->GetDesc1(&desc)) || desc.Monitor!=monitor)continue;
                display.found=true;display.color_space=uint32_t(desc.ColorSpace);display.bits=desc.BitsPerColor;
                display.hdr=desc.ColorSpace==DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
                display.max_nits=desc.MaxLuminance;display.max_full_frame_nits=desc.MaxFullFrameLuminance;display.min_nits=desc.MinLuminance;
                return display;}}
    }catch(...){}
    return display;
}
HRESULT FramePresenter::TryResizeBuffers(DeviceQueues& owner) {
    const auto status=ResizeChain(width,height);
    if(IsDeviceLoss(status))return status;   // final review L1: the caller refuses
    if(SUCCEEDED(status)){Acquire(owner.device.Get());return status;}
    // Failed: the chain's buffers taken again, as they were (ER's premise). I-0111 (R1; Thomas's 09:01 play, reproduced by
    // i111-fs-quit-1): closing the game in exclusive full screen, ResizeBuffers answers E_ACCESSDENIED through Streamline's proxy
    // and the chain then refuses GetBuffer (DXGI_ERROR_INVALID_CALL); stand-in textures of the chain's size and format take the
    // buffers' place, so the frames record as before; while they are in place every frame follows the presentation again
    // (PresentationNeedsFollow, final review M1: a Present on that chain is expected to fail, not observed to), resizing at the
    // follows' limit, and a resize that succeeds takes the chain's buffers back.
    if(!TryAcquire(owner.device.Get()))StandIn(owner.device.Get());
    return status;
}
bool FramePresenter::TryAcquire(ID3D12Device* device) {
    const auto base=rtv->GetCPUDescriptorHandleForHeapStart();
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> taken(count);
    for(UINT i=0;i<count;++i)if(FAILED(swapchain->GetBuffer(i,IID_PPV_ARGS(&taken[i]))))return false;
    // E3: after a refused resize to another size the chain still holds its former buffers: not the frames' targets.
    // I-0138 tranche 5 (step review L1): nor buffers of another format than the presenter's.
    for(const auto& buffer:taken){const auto made=buffer->GetDesc();if(made.Width!=width || made.Height!=height || made.Format!=format)return false;}
    buffers=std::move(taken);stand_in=false;
    for(UINT i=0;i<count;++i)device->CreateRenderTargetView(buffers[i].Get(),nullptr,{base.ptr+static_cast<SIZE_T>(i)*stride});
    return true;
}
void FramePresenter::StandIn(ID3D12Device* device) {
    const auto base=rtv->GetCPUDescriptorHandleForHeapStart();
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=width;desc.Height=height;desc.DepthOrArraySize=1;
    desc.MipLevels=1;desc.Format=format;desc.SampleDesc.Count=1;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;   // I-0138 tranche 5: the chain's format
    D3D12_CLEAR_VALUE clear{};clear.Format=format;clear.Color[3]=1;   // final review L3: the frames clear to black
    buffers.assign(count,nullptr);
    for(UINT i=0;i<count;++i){   // in the state a back buffer is in between frames (PRESENT, the common state)
        Check(CountedResource(device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_PRESENT,&clear,IID_PPV_ARGS(&buffers[i])),"stand-in back buffer");
        buffers[i]->SetName(L"stand-in back buffer");
        device->CreateRenderTargetView(buffers[i].Get(),nullptr,{base.ptr+static_cast<SIZE_T>(i)*stride});}
    stand_in=true;
}
// I-0137 (I-0120 E3): TryResizeBuffers' rule on this path too (a resize of the output, a full-screen toggle, a display mode):
// only a lost device throws. Another failure (E_ACCESSDENIED after a focus loss, i101-fs-alttab-1, or through Streamline's
// proxy, i111-fs-quit-1) leaves the chain as DXGI left it: its buffers are taken again when it gives them and they are of the
// asked size (TryAcquire); otherwise stand-ins of the asked size take their place, which makes every frame follow the presentation
// (PresentationNeedsFollow) until a resize of the chain succeeds and its buffers come back. The journal's native_display_lost
// rows carry the follow's status and `stand_in`.
void FramePresenter::Resize(DeviceQueues& owner,uint32_t w,uint32_t h) {
    const auto status=ResizeChain(w,h);
    if(IsDeviceLoss(status))Check(status,"native resize");
    width=w; height=h;
    if(SUCCEEDED(status)){Acquire(owner.device.Get());return;}
    if(!TryAcquire(owner.device.Get()))StandIn(owner.device.Get());
}
} // namespace ds3native
namespace ds3native {
namespace {std::atomic<DeviceQueues*> retirement_queue{};}
void SetRetirementQueue(DeviceQueues* queues) noexcept {retirement_queue=queues;}
void WithdrawRetirementQueue(DeviceQueues* queues) noexcept {auto expected=queues;retirement_queue.compare_exchange_strong(expected,nullptr);}
namespace {thread_local bool released_after_completion{};}
ReleaseAfterCompletion::ReleaseAfterCompletion() noexcept:previous_(released_after_completion){released_after_completion=true;}
ReleaseAfterCompletion::~ReleaseAfterCompletion(){released_after_completion=previous_;}
namespace {thread_local std::vector<std::shared_ptr<void>>* recording_retirements{};}
void RetireAfterCurrentRecording(std::shared_ptr<void> owner){
    if(!owner)return;
    if(!retirement_queue.load()){RetireWhenSubmittedWorkCompletes(std::move(owner));return;}   // the process-life keep below
    if(!recording_retirements)recording_retirements=new std::vector<std::shared_ptr<void>>;   // per thread, process lifetime
    recording_retirements->push_back(std::move(owner));
}
// Called by SubmitWithIngress after a list from this thread is signalled: the owners retired while it was recorded follow its ticket.
void RetireRecordingOwners(DeviceQueues& queues,uint64_t ticket){
    if(!recording_retirements || recording_retirements->empty())return;
    std::vector<std::shared_ptr<void>> owners;owners.swap(*recording_retirements);
    for(auto& owner:owners)queues.RetireAfter(ticket,std::move(owner));
}
void RetireWhenSubmittedWorkCompletes(std::shared_ptr<void> owner){
    if(released_after_completion)return;   // released here (ReleaseAfterCompletion)
    auto* queues=retirement_queue.load();const auto ticket=queues?queues->LastSubmitted():0;
    if(ticket){queues->RetireAfter(ticket,std::move(owner));return;}
    // Axis 2: without a retirement queue (the stage control suites drive the stages with their own queue), an object a list
    // not yet executed may still reference (a renewed heap, a retired surface) is kept for the process's life rather than
    // released here (a list never destroyed: a static vector would release COM objects after their device at exit); the
    // production path always has its queue.
    static auto* const lock=new std::mutex;static auto* const kept=new std::vector<std::shared_ptr<void>>;
    const std::lock_guard hold(*lock);if(kept->size()<65536)kept->push_back(std::move(owner));else owner.reset();
}
namespace {std::atomic<uint64_t> arena_chunks_created{},arena_bytes_allocated{};}
uint64_t UploadArenaChunksCreated() noexcept {return arena_chunks_created;}
uint64_t UploadArenaBytesAllocated() noexcept {return arena_bytes_allocated;}
struct UploadArena::Chunk {ID3D12Device* device{};ComPtr<ID3D12Resource> buffer;uint8_t* memory{};uint64_t size{},used{};};
struct UploadArena::Pool {mutable std::mutex lock;ID3D12Device* device{};std::array<std::vector<std::unique_ptr<Chunk>>,class_bytes.size()> free;};
namespace {
// The size class of a chunk of `size` bytes (class_bytes index), or class_bytes.size() for an unpooled chunk.
size_t ArenaClass(uint64_t size) noexcept {
    for(size_t i=0;i<UploadArena::class_bytes.size();++i)if(size==UploadArena::class_bytes[i])return i;
    return UploadArena::class_bytes.size();
}
}
UploadArena::UploadArena():pool_(std::make_shared<Pool>()){}
UploadArena::~UploadArena()=default;
UploadArena::Range UploadArena::Allocate(ID3D12Device* device,uint64_t bytes,uint64_t align,bool zero) {
    if(!device || !bytes || align<alignment || (align&(align-1)))throw std::invalid_argument("upload arena allocation");
    const uint64_t size=(bytes+alignment-1)&~(alignment-1);
    const auto aligned=[&](const Chunk& chunk){return (chunk.used+align-1)&~(align-1);};
    if(used_.empty() || used_.back()->device!=device || aligned(*used_.back())+size>used_.back()->size){
        // The smallest class that holds the range; above the largest, a chunk of its own.
        const auto fit=std::find_if(class_bytes.begin(),class_bytes.end(),[&](uint64_t bytes_of_class){return size<=bytes_of_class;});
        const size_t size_class=static_cast<size_t>(fit-class_bytes.begin());
        std::unique_ptr<Chunk> chunk;
        {const std::lock_guard hold(pool_->lock);
            if(pool_->device!=device){for(auto& list:pool_->free)list.clear();pool_->device=device;}
            if(size_class<class_bytes.size() && !pool_->free[size_class].empty()){
                chunk=std::move(pool_->free[size_class].back());pool_->free[size_class].pop_back();}}
        if(!chunk){chunk=std::make_unique<Chunk>();chunk->device=device;chunk->size=size_class<class_bytes.size()?class_bytes[size_class]:size;
            const auto heap=Heap(UploadHeapType());const auto desc=Buffer(chunk->size);   // I-0132 R11f: GPU upload where offered
            Check(CountedResource(device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,UploadHeapState(heap.Type),nullptr,IID_PPV_ARGS(&chunk->buffer)),"upload arena chunk");
            void* memory{};const D3D12_RANGE none{};Check(chunk->buffer->Map(0,&none,&memory),"upload arena chunk map");
            chunk->memory=static_cast<uint8_t*>(memory);++arena_chunks_created;}
        chunk->used=0;used_.push_back(std::move(chunk));
    }
    auto& chunk=*used_.back();const auto offset=aligned(chunk);
    Range range{chunk.memory+offset,chunk.buffer->GetGPUVirtualAddress()+offset,bytes,chunk.buffer.Get(),offset};
    if(zero)std::memset(range.cpu,0,static_cast<size_t>(bytes));
    arena_bytes_allocated+=offset+size-chunk.used;chunk.used=offset+size;
    return range;
}
void UploadArena::Retire(DeviceQueues& queues,uint64_t ticket) {
    // Every range handed out since the previous call was recorded in the list submitted as `ticket`, or earlier.
    if(!ticket || ticket>queues.LastSubmitted())throw std::invalid_argument("upload arena retirement ticket not submitted");
    // The owner exists before the chunk moves into it; a chunk whose retirement could not be recorded is kept to
    // process exit (it may be in flight), never returned early.
    struct Held {
        std::weak_ptr<Pool> pool;std::unique_ptr<Chunk> chunk;
        ~Held(){const auto size_class=chunk?ArenaClass(chunk->size):class_bytes.size();
            if(const auto owner=pool.lock();owner && size_class<class_bytes.size()){const std::lock_guard hold(owner->lock);
                if(owner->device==chunk->device && owner->free[size_class].size()<class_pooled[size_class])owner->free[size_class].push_back(std::move(chunk));}}
    };
    size_t moved{};
    try {
        for(;moved<used_.size();++moved){
            auto held=std::make_shared<Held>();held->pool=pool_;held->chunk=std::move(used_[moved]);
            try {queues.RetireAfter(ticket,held);}catch(...){(void)held->chunk.release();throw;}   // kept to process exit
        }
    }catch(...){std::erase_if(used_,[](const std::unique_ptr<Chunk>& chunk){return !chunk;});throw;}   // the rest retire later
    used_.clear();
}
void UploadArena::Reset() noexcept {
    used_.clear();const std::lock_guard hold(pool_->lock);for(auto& list:pool_->free)list.clear();pool_->device=nullptr;
}
void UploadArena::QuarantineToProcessExit() noexcept {
    for(auto& chunk:used_)(void)chunk.release();used_.clear();
}
size_t UploadArena::FreeChunks() const {
    const std::lock_guard hold(pool_->lock);size_t total{};for(const auto& list:pool_->free)total+=list.size();return total;
}
size_t UploadArena::FreeChunks(size_t size_class) const {
    if(size_class>=class_bytes.size())throw std::invalid_argument("upload arena size class");
    const std::lock_guard hold(pool_->lock);return pool_->free[size_class].size();
}
// D3D12 phase D cD5: the pool of dynamic stream versions.
struct UploadVersion::Pool {
    struct Free {Microsoft::WRL::ComPtr<ID3D12Resource> resource;uint8_t* memory{};};
    mutable std::mutex lock;ID3D12Device* device{};
    std::unordered_map<uint64_t,std::vector<Free>> free;   // by capacity
    uint64_t free_bytes{};std::atomic<uint64_t> created{};
};
UploadVersion::~UploadVersion(){
    const auto pool=pool_.lock();if(!pool || !resource)return;
    // The buffer goes back once the queue completed what was submitted until now (L2: every list that drew this version
    // held it, so none recorded later can read it). A returner whose retirement could not be recorded is disarmed: the
    // buffer is then kept to process exit (it may be in flight), never pooled early (review).
    struct Returner {
        std::weak_ptr<Pool> pool;Pool::Free buffer;uint64_t capacity{};ID3D12Device* device{};bool armed{true};
        ~Returner(){
            if(!armed){(void)buffer.resource.Detach();return;}
            try{const auto owner=pool.lock();if(!owner)return;const std::lock_guard hold(owner->lock);
                auto& list=owner->free[capacity];
                if(owner->device==device && list.size()<UploadVersionPool::pooled_per_size && owner->free_bytes+capacity<=UploadVersionPool::pooled_bytes){
                    owner->free_bytes+=capacity;list.push_back(std::move(buffer));}}
            catch(...){}   // not pooled: the buffer is released with the returner
        }
    };
    std::shared_ptr<Returner> keep;
    try{keep=std::make_shared<Returner>();keep->pool=pool;keep->buffer={std::move(resource),memory_};keep->capacity=capacity_;keep->device=device_;
        RetireWhenSubmittedWorkCompletes(keep);}
    catch(...){if(keep)keep->armed=false;}
}

UploadVersionPool::UploadVersionPool():pool_(std::make_shared<UploadVersion::Pool>()){}
std::shared_ptr<const UploadVersion> UploadVersionPool::Publish(ID3D12Device* device,const void* data,size_t bytes){
    if(!device || !data || !bytes)throw std::invalid_argument("upload version");
    const uint64_t capacity=(uint64_t(bytes)+granularity-1)&~(granularity-1);
    UploadVersion::Pool::Free buffer;
    {const std::lock_guard hold(pool_->lock);
        if(pool_->device!=device){pool_->free.clear();pool_->free_bytes=0;pool_->device=device;}
        if(auto found=pool_->free.find(capacity);found!=pool_->free.end() && !found->second.empty()){
            buffer=std::move(found->second.back());found->second.pop_back();pool_->free_bytes-=capacity;}}
    if(!buffer.resource){
        const auto heap=Heap(UploadHeapType());const auto desc=Buffer(capacity);   // I-0132 R11f: GPU upload where offered
        Check(CountedResource(device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,UploadHeapState(heap.Type),nullptr,IID_PPV_ARGS(&buffer.resource)),"upload version buffer");
        void* memory{};const D3D12_RANGE none{};Check(buffer.resource->Map(0,&none,&memory),"upload version map");
        buffer.memory=static_cast<uint8_t*>(memory);++pool_->created;}
    std::memcpy(buffer.memory,data,bytes);
    if(capacity>bytes)std::memset(buffer.memory+bytes,0,static_cast<size_t>(capacity-bytes));
    auto version=std::make_shared<UploadVersion>();version->resource=std::move(buffer.resource);version->bytes=bytes;
    version->pool_=pool_;version->memory_=buffer.memory;version->capacity_=capacity;version->device_=device;
    return version;
}
size_t UploadVersionPool::FreeBuffers() const {
    const std::lock_guard hold(pool_->lock);size_t total{};for(const auto& [capacity,list]:pool_->free){(void)capacity;total+=list.size();}return total;
}
uint64_t UploadVersionPool::Created() const noexcept {return pool_->created;}
void UploadVersionPool::Reset() noexcept {
    const std::lock_guard hold(pool_->lock);pool_->free.clear();pool_->free_bytes=0;pool_->device=nullptr;
}
// ---- I-0072: the device's pipeline cache (ER's PSO cache and its precompile list; no driver library: I-0113) ----
PipelineStore& Pipelines(){static auto* store=new PipelineStore;return *store;}
namespace {
using Name=PipelineStore::Name;
// Two FNV-1a 64 lanes over the bytes (the second lane also folds the first), then the size.
Name HashBytes(uint64_t seed,const void* bytes,size_t size) noexcept {
    uint64_t high=0xcbf29ce484222325ULL^seed,low=0x84222325cbf29ce4ULL^(seed*0x100000001B3ULL);
    const auto* p=static_cast<const uint8_t*>(bytes);
    for(size_t i=0;i<size;++i){high^=p[i];high*=1099511628211ULL;low^=p[i];low*=0x100000001B3ULL;low^=high>>29;}
    high^=size;high*=1099511628211ULL;low^=size;low*=0x100000001B3ULL;
    return {high,low};
}
uint64_t Checksum(const uint8_t* bytes,size_t size) noexcept {return HashBytes(0x5053432D44533343ULL,bytes,size).high;}
struct Writer {
    std::vector<uint8_t>& out;
    void Bytes(const void* data,size_t size){const auto* p=static_cast<const uint8_t*>(data);out.insert(out.end(),p,p+size);}
    void U8(uint8_t v){out.push_back(v);}
    void U32(uint32_t v){Bytes(&v,4);}
    void U64(uint64_t v){Bytes(&v,8);}
    void F32(float v){uint32_t bits{};std::memcpy(&bits,&v,4);U32(bits);}
    void N(const Name& n){U64(n.high);U64(n.low);}
};
struct Reader {
    const uint8_t* at;const uint8_t* end;bool ok=true;
    bool Has(size_t size){if(ok && static_cast<size_t>(end-at)<size)ok=false;return ok;}
    const uint8_t* Bytes(size_t size){if(!Has(size))return nullptr;const auto* p=at;at+=size;return p;}
    uint8_t U8(){const auto* p=Bytes(1);return p?*p:0;}
    uint32_t U32(){uint32_t v{};if(const auto* p=Bytes(4))std::memcpy(&v,p,4);return v;}
    uint64_t U64(){uint64_t v{};if(const auto* p=Bytes(8))std::memcpy(&v,p,8);return v;}
    float F32(){const uint32_t bits=U32();float v{};std::memcpy(&v,&bits,4);return v;}
    Name N(){const auto high=U64();return {high,U64()};}
};
constexpr uint8_t graphics_kind='G',compute_kind='C',encoding_version=1;
constexpr uint32_t list_magic=0x43535044,list_version=4;   // 'DPSC'; 3: no library stamp nor environment (I-0113); 4: roots named by content (I-0132)
constexpr uint64_t root_seed=0x524F4F545349474EULL;
// A description decoded from the list: the desc points into this object and into the store's programmes.
struct DecodedPipeline {
    bool compute{};D3D12_GRAPHICS_PIPELINE_STATE_DESC graphics{};D3D12_COMPUTE_PIPELINE_STATE_DESC cs{};Name root{};
    std::vector<std::string> semantics;std::vector<D3D12_INPUT_ELEMENT_DESC> elements;
    std::vector<std::shared_ptr<const std::vector<uint8_t>>> programmes;   // held while the pipeline is built
};
// The inverse of PipelineStore::Encode; `programme` finds a programme's bytes by name (null when the list lacks it).
template<class Find> bool Decode(const std::vector<uint8_t>& encoding,Find programme,DecodedPipeline& out){
    Reader r{encoding.data(),encoding.data()+encoding.size()};
    const auto kind=r.U8();if(r.U8()!=encoding_version || (kind!=graphics_kind && kind!=compute_kind))return false;
    out.compute=kind==compute_kind;out.root=r.N();
    const auto shader=[&](D3D12_SHADER_BYTECODE& bytecode){const auto size=r.U64();if(!size){bytecode={};return r.ok;}
        const auto name=r.N();auto bytes=programme(name);if(!r.ok || !bytes || bytes->size()!=size)return false;
        bytecode={bytes->data(),bytes->size()};out.programmes.push_back(std::move(bytes));return true;};
    if(out.compute){if(!shader(out.cs.CS))return false;out.cs.NodeMask=r.U32();out.cs.Flags=static_cast<D3D12_PIPELINE_STATE_FLAGS>(r.U32());return r.ok && r.at==r.end;}
    auto& d=out.graphics;
    for(auto* stage:{&d.VS,&d.PS,&d.DS,&d.HS,&d.GS})if(!shader(*stage))return false;
    d.BlendState.AlphaToCoverageEnable=r.U32();d.BlendState.IndependentBlendEnable=r.U32();
    for(auto& t:d.BlendState.RenderTarget){t.BlendEnable=r.U32();t.LogicOpEnable=r.U32();t.SrcBlend=static_cast<D3D12_BLEND>(r.U32());t.DestBlend=static_cast<D3D12_BLEND>(r.U32());
        t.BlendOp=static_cast<D3D12_BLEND_OP>(r.U32());t.SrcBlendAlpha=static_cast<D3D12_BLEND>(r.U32());t.DestBlendAlpha=static_cast<D3D12_BLEND>(r.U32());
        t.BlendOpAlpha=static_cast<D3D12_BLEND_OP>(r.U32());t.LogicOp=static_cast<D3D12_LOGIC_OP>(r.U32());t.RenderTargetWriteMask=static_cast<UINT8>(r.U32());}
    d.SampleMask=r.U32();
    auto& ra=d.RasterizerState;ra.FillMode=static_cast<D3D12_FILL_MODE>(r.U32());ra.CullMode=static_cast<D3D12_CULL_MODE>(r.U32());ra.FrontCounterClockwise=r.U32();
    ra.DepthBias=static_cast<INT>(r.U32());ra.DepthBiasClamp=r.F32();ra.SlopeScaledDepthBias=r.F32();ra.DepthClipEnable=r.U32();ra.MultisampleEnable=r.U32();
    ra.AntialiasedLineEnable=r.U32();ra.ForcedSampleCount=r.U32();ra.ConservativeRaster=static_cast<D3D12_CONSERVATIVE_RASTERIZATION_MODE>(r.U32());
    auto& ds=d.DepthStencilState;ds.DepthEnable=r.U32();ds.DepthWriteMask=static_cast<D3D12_DEPTH_WRITE_MASK>(r.U32());ds.DepthFunc=static_cast<D3D12_COMPARISON_FUNC>(r.U32());
    ds.StencilEnable=r.U32();ds.StencilReadMask=static_cast<UINT8>(r.U32());ds.StencilWriteMask=static_cast<UINT8>(r.U32());
    for(auto* face:{&ds.FrontFace,&ds.BackFace}){face->StencilFailOp=static_cast<D3D12_STENCIL_OP>(r.U32());face->StencilDepthFailOp=static_cast<D3D12_STENCIL_OP>(r.U32());
        face->StencilPassOp=static_cast<D3D12_STENCIL_OP>(r.U32());face->StencilFunc=static_cast<D3D12_COMPARISON_FUNC>(r.U32());}
    const auto elements=r.U32();if(!r.ok || elements>256)return false;
    out.semantics.resize(elements);out.elements.resize(elements);
    for(uint32_t i=0;i<elements;++i){const auto length=r.U32();if(length>256)return false;const auto* text=r.Bytes(length);if(!text)return false;
        out.semantics[i].assign(reinterpret_cast<const char*>(text),length);auto& e=out.elements[i];
        e.SemanticIndex=r.U32();e.Format=static_cast<DXGI_FORMAT>(r.U32());e.InputSlot=r.U32();e.AlignedByteOffset=r.U32();
        e.InputSlotClass=static_cast<D3D12_INPUT_CLASSIFICATION>(r.U32());e.InstanceDataStepRate=r.U32();}
    for(uint32_t i=0;i<elements;++i)out.elements[i].SemanticName=out.semantics[i].c_str();
    d.InputLayout={elements?out.elements.data():nullptr,elements};
    d.IBStripCutValue=static_cast<D3D12_INDEX_BUFFER_STRIP_CUT_VALUE>(r.U32());d.PrimitiveTopologyType=static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(r.U32());
    d.NumRenderTargets=r.U32();for(auto& format:d.RTVFormats)format=static_cast<DXGI_FORMAT>(r.U32());d.DSVFormat=static_cast<DXGI_FORMAT>(r.U32());
    d.SampleDesc.Count=r.U32();d.SampleDesc.Quality=r.U32();d.NodeMask=r.U32();d.Flags=static_cast<D3D12_PIPELINE_STATE_FLAGS>(r.U32());
    return r.ok && r.at==r.end && d.NumRenderTargets<=8;
}
Name NameOf(const std::vector<uint8_t>& encoding) noexcept {return HashBytes(0x4453334E50534F31ULL,encoding.data(),encoding.size());}
constexpr uint32_t FourCC(const char (&c)[5]) noexcept {return uint32_t(uint8_t(c[0]))|uint32_t(uint8_t(c[1]))<<8|uint32_t(uint8_t(c[2]))<<16|uint32_t(uint8_t(c[3]))<<24;}
// The container: 'DXBC', a 16-byte digest, the version, the size, the chunk count at 28, then the chunks' offsets; a chunk is its
// four-character code, its size, then its data. The first chunk whose code is one of `codes`: its code, data offset and length;
// Found, Absent, or Malformed (a header, an offset or a chunk outside the blob).
enum class Chunk {Found,Absent,Malformed};
Chunk FindChunk(const uint8_t* base,size_t size,std::initializer_list<uint32_t> codes,uint32_t& code,size_t& data,uint32_t& length) noexcept {
    const auto word=[&](size_t at,uint32_t& value){if(!base || at>size || size-at<4)return false;std::memcpy(&value,base+at,4);return true;};
    uint32_t magic{},chunks{};
    if(!word(0,magic) || magic!=FourCC("DXBC") || !word(28,chunks) || chunks>256)return Chunk::Malformed;
    for(uint32_t i=0;i<chunks;++i){uint32_t offset{};
        if(!word(32+4*size_t(i),offset) || !word(offset,code) || !word(size_t(offset)+4,length))return Chunk::Malformed;
        if(std::find(codes.begin(),codes.end(),code)==codes.end())continue;
        data=size_t(offset)+8;return data>size || length>size-data?Chunk::Malformed:Chunk::Found;}
    return Chunk::Absent;
}
// I-0132 (the root aliases): a root is named by its content, the serialised root signature (the container's RTS0 chunk), not by the
// blob carrying it: two builds' DXIL programmes embedding one root (the runtime returned one object for them, I-0132 T8 on 616
// and 620), or a programme and the bare serialised root (itself a container with that one chunk), name one root, one object. A blob that is not a container with an RTS0 chunk
// is named by its bytes. (List version 3 named every root by its blob's bytes: Open renames them.)
Name RootName(const void* blob,size_t size) noexcept {
    const auto* base=static_cast<const uint8_t*>(blob);uint32_t code{},length{};size_t data{};
    if(FindChunk(base,size,{FourCC("RTS0")},code,data,length)==Chunk::Found)return HashBytes(root_seed,base+data,length);
    return HashBytes(root_seed,blob,size);
}
}
// An output signature's data: the element count, the elements' offset, then the elements (OSGN 24 bytes, OSG5 a stream word first,
// OSG1 a stream word first and a precision word last), each starting (after the stream) with its name's offset from the chunk's data.
bool PixelShaderWritesDepth(const D3D12_SHADER_BYTECODE& ps) noexcept {
    if(!ps.pShaderBytecode || !ps.BytecodeLength)return false;
    const auto* base=static_cast<const uint8_t*>(ps.pShaderBytecode);const size_t size=ps.BytecodeLength;
    const auto word=[&](size_t at,uint32_t& value){if(at>size || size-at<4)return false;std::memcpy(&value,base+at,4);return true;};
    uint32_t code{},length{};size_t data{};
    const auto found=FindChunk(base,size,{FourCC("OSGN"),FourCC("OSG5"),FourCC("OSG1")},code,data,length);
    if(found!=Chunk::Found)return true;   // unreadable, or no output signature: the format kept
    const size_t stride=code==FourCC("OSGN")?24:code==FourCC("OSG5")?28:32,lead=code==FourCC("OSGN")?0:4;
    uint32_t count{},first{};if(length<8 || !word(data,count) || !word(data+4,first) || first>length || count>(length-first)/stride)return true;
    for(uint32_t e=0;e<count;++e){
        const size_t element=data+first+size_t(e)*stride+lead;
        uint32_t name{},system{};if(!word(element,name) || name>=length || !word(element+8,system))return true;
        // The step review's L3: the system value too (D3D_NAME_DEPTH 65, DEPTH_GREATER_EQUAL 67, DEPTH_LESS_EQUAL 68), whatever the name.
        if(system==65 || system==67 || system==68)return true;
        const char* text=reinterpret_cast<const char*>(base+data+name);const size_t room=length-name;
        constexpr char depth[]="SV_Depth";constexpr size_t n=sizeof(depth)-1;   // SV_Depth, SV_DepthGreaterEqual, SV_DepthLessEqual
        if(room>=n && !_strnicmp(text,depth,n))return true;}
    return false;   // the output signature read: no depth written
}
bool UnusedDepthStencilFormat(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& d) noexcept {
    return d.DSVFormat!=DXGI_FORMAT_UNKNOWN && !d.DepthStencilState.DepthEnable && !d.DepthStencilState.StencilEnable && !PixelShaderWritesDepth(d.PS);}
// The canonical encoding of a description (field by field: no padding byte is read), the programmes by name, the root by its
// content's name (RootName). Registers the programme bytes for the list. False for what the list cannot hold (an unknown root, stream output,
// a cached blob).
bool PipelineStore::Encode(const D3D12_GRAPHICS_PIPELINE_STATE_DESC* graphics,const D3D12_COMPUTE_PIPELINE_STATE_DESC* compute,const std::array<Name,5>& stages,
    std::vector<uint8_t>& out){
    out.clear();Writer w{out};
    const auto root=root_names_.find(graphics?graphics->pRootSignature:compute->pRootSignature);if(root==root_names_.end())return false;
    w.U8(graphics?graphics_kind:compute_kind);w.U8(encoding_version);w.N(root->second);
    size_t stage=0;
    const auto shader=[&](const D3D12_SHADER_BYTECODE& bytecode){const auto& name=stages[stage++];w.U64(bytecode.pShaderBytecode?bytecode.BytecodeLength:0);
        if(!bytecode.pShaderBytecode || !bytecode.BytecodeLength)return;
        w.N(name);
        if(!programmes_.contains(name)){const auto* p=static_cast<const uint8_t*>(bytecode.pShaderBytecode);
            programmes_.emplace(name,std::make_shared<const std::vector<uint8_t>>(p,p+bytecode.BytecodeLength));}};
    if(compute){if(compute->CachedPSO.pCachedBlob)return false;shader(compute->CS);w.U32(compute->NodeMask);w.U32(compute->Flags);return true;}
    const auto& d=*graphics;
    // Review L2: stream output, including a rasterized stream, is not part of the encoding: such a description goes direct.
    if(d.CachedPSO.pCachedBlob || d.StreamOutput.NumEntries || d.StreamOutput.NumStrides || d.StreamOutput.RasterizedStream ||
       d.NumRenderTargets>8 || d.InputLayout.NumElements>256)return false;
    for(const auto* s:{&d.VS,&d.PS,&d.DS,&d.HS,&d.GS})shader(*s);
    w.U32(d.BlendState.AlphaToCoverageEnable);w.U32(d.BlendState.IndependentBlendEnable);
    for(const auto& t:d.BlendState.RenderTarget)for(const uint32_t v:{uint32_t(t.BlendEnable),uint32_t(t.LogicOpEnable),uint32_t(t.SrcBlend),uint32_t(t.DestBlend),uint32_t(t.BlendOp),
        uint32_t(t.SrcBlendAlpha),uint32_t(t.DestBlendAlpha),uint32_t(t.BlendOpAlpha),uint32_t(t.LogicOp),uint32_t(t.RenderTargetWriteMask)})w.U32(v);
    w.U32(d.SampleMask);
    const auto& r=d.RasterizerState;w.U32(r.FillMode);w.U32(r.CullMode);w.U32(r.FrontCounterClockwise);w.U32(static_cast<uint32_t>(r.DepthBias));w.F32(r.DepthBiasClamp);
    w.F32(r.SlopeScaledDepthBias);w.U32(r.DepthClipEnable);w.U32(r.MultisampleEnable);w.U32(r.AntialiasedLineEnable);w.U32(r.ForcedSampleCount);w.U32(r.ConservativeRaster);
    const auto& ds=d.DepthStencilState;w.U32(ds.DepthEnable);w.U32(ds.DepthWriteMask);w.U32(ds.DepthFunc);w.U32(ds.StencilEnable);w.U32(ds.StencilReadMask);w.U32(ds.StencilWriteMask);
    for(const auto* face:{&ds.FrontFace,&ds.BackFace}){w.U32(face->StencilFailOp);w.U32(face->StencilDepthFailOp);w.U32(face->StencilPassOp);w.U32(face->StencilFunc);}
    w.U32(d.InputLayout.NumElements);
    for(UINT i=0;i<d.InputLayout.NumElements;++i){const auto& e=d.InputLayout.pInputElementDescs[i];const size_t length=e.SemanticName?std::strlen(e.SemanticName):0;
        if(length>256)return false;w.U32(static_cast<uint32_t>(length));w.Bytes(e.SemanticName,length);
        w.U32(e.SemanticIndex);w.U32(e.Format);w.U32(e.InputSlot);w.U32(e.AlignedByteOffset);w.U32(e.InputSlotClass);w.U32(e.InstanceDataStepRate);}
    w.U32(d.IBStripCutValue);w.U32(d.PrimitiveTopologyType);w.U32(d.NumRenderTargets);for(const auto format:d.RTVFormats)w.U32(format);w.U32(d.DSVFormat);
    w.U32(d.SampleDesc.Count);w.U32(d.SampleDesc.Quality);w.U32(d.NodeMask);w.U32(d.Flags);
    return true;
}
void PipelineStore::Open(ID3D12Device* device,std::wstring path,bool enabled){
    std::unique_lock hold(lock_);
    open_=false;path_=std::move(path);counters_={};unsaved_=0;
    // Roots, pipelines and records made for another device are forgotten; this device's (made before Open) stay (review L7).
    if(device_.Get()!=device){roots_.clear();root_names_.clear();pipelines_.clear();records_.clear();programmes_.clear();}
    device_=device;
    if(!enabled || !device)return;
    std::vector<uint8_t> list;
    {FILE* f{};if(!path_.empty() && !_wfopen_s(&f,path_.c_str(),L"rb") && f){
        std::fseek(f,0,SEEK_END);const long size=std::ftell(f);std::fseek(f,0,SEEK_SET);
        if(size>0){list.resize(static_cast<size_t>(size));if(std::fread(list.data(),1,list.size(),f)!=list.size())list.clear();}
        std::fclose(f);}}
    // The list: header, roots, programmes, records, checksum. Every name is recomputed from the bytes it names; one mismatch, a
    // wrong checksum or another format discards the whole file (a damaged list costs only the next start's precompile). Each
    // record read is one start older; a record idle for max_idle starts is dropped.
    if(list.size()>=16 && Checksum(list.data(),list.size()-8)==[&]{uint64_t v{};std::memcpy(&v,list.data()+list.size()-8,8);return v;}()){
        Reader r{list.data(),list.data()+list.size()-8};
        bool valid=r.U32()==list_magic;const auto version=r.U32();valid=valid && (version==list_version || version==3);
        std::unordered_map<Name,RootEntry,NameHash> roots;std::unordered_map<Name,std::shared_ptr<const std::vector<uint8_t>>,NameHash> programmes;std::unordered_map<Name,Record,NameHash> records;
        // I-0132: a version-3 list named a root by its blob's bytes; each is renamed by its content and its records with it (two
        // blobs of one root, an older build's and a newer's, become one root, their identical records one record).
        std::unordered_map<Name,Name,NameHash> renamed;
        for(uint32_t n=valid?r.U32():0,i=0;valid && i<n;++i){const auto name=r.N();const auto size=r.U32();const auto* bytes=r.Bytes(size);
            const auto content=bytes?RootName(bytes,size):Name{};
            valid=bytes && (version==3?HashBytes(root_seed,bytes,size)==name:content==name);if(!valid)break;
            if(!(content==name))renamed.emplace(name,content);
            if(auto& entry=roots[content];entry.blob.empty())entry.blob.assign(bytes,bytes+size);}
        for(uint32_t n=valid?r.U32():0,i=0;valid && i<n;++i){const auto name=r.N();const auto size=r.U32();const auto* bytes=r.Bytes(size);
            valid=bytes && HashBytes(0x50524F4752414D45ULL,bytes,size)==name;if(valid)programmes.emplace(name,std::make_shared<const std::vector<uint8_t>>(bytes,bytes+size));}
        uint64_t pruned{};
        for(uint32_t n=valid?r.U32():0,i=0;valid && i<n;++i){const auto stored_idle=r.U8();const auto size=r.U32();const auto* bytes=r.Bytes(size);valid=bytes!=nullptr;
            const uint8_t idle=static_cast<uint8_t>(stored_idle+1);
            if(valid && idle>=max_idle)++pruned;
            else if(valid){Record record{{bytes,bytes+size},idle};
                if(!renamed.empty() && record.encoding.size()>=18){Name root{};std::memcpy(&root.high,record.encoding.data()+2,8);std::memcpy(&root.low,record.encoding.data()+10,8);
                    if(const auto to=renamed.find(root);to!=renamed.end()){std::memcpy(record.encoding.data()+2,&to->second.high,8);std::memcpy(record.encoding.data()+10,&to->second.low,8);}}
                const auto name=NameOf(record.encoding);
                // Two records made one by the renaming keep the younger age.
                if(const auto same=records.find(name);same!=records.end()){same->second.idle=(std::min)(same->second.idle,record.idle);++counters_.records_merged;}
                else records.emplace(name,std::move(record));}}
        if(valid && r.ok && r.at==r.end){
            for(auto& [name,entry]:roots)if(!roots_.contains(name))roots_.emplace(name,std::move(entry));
            for(auto& [name,bytes]:programmes)programmes_.try_emplace(name,std::move(bytes));
            for(auto& [name,record]:records)records_.try_emplace(name,std::move(record));
            // Every start writes the list once (the records' ages moved), so a description no build asks for any more ages out.
            counters_.list_loaded=true;counters_.list_bytes=list.size();counters_.pruned=pruned;counters_.roots_renamed=renamed.size();
            if(pruned || !records.empty() || !renamed.empty())++unsaved_;}
        else counters_.records_merged=0;}
    open_=true;counters_.records=records_.size();counters_.open=true;
}
bool PipelineStore::IsOpen() const {const std::lock_guard hold(lock_);return open_;}
PipelineStore::Counters PipelineStore::Stats() const {const std::lock_guard hold(lock_);auto counters=counters_;counters.unsaved=unsaved_;counters.records=records_.size();return counters;}
HRESULT PipelineStore::Root(ID3D12Device* device,const void* blob,size_t size,ID3D12RootSignature** out){
    if(!out)return E_POINTER;*out=nullptr;
    const auto name=RootName(blob,size);
    std::unique_lock hold(lock_);
    if(device!=device_.Get() && device_){hold.unlock();return device->CreateRootSignature(0,blob,size,IID_PPV_ARGS(out));}   // another device: not cached
    if(!device_)device_=device;   // before Open: this device's roots are kept for it
    const auto [found,fresh]=roots_.try_emplace(name);auto& entry=found->second;
    // The step review's L3: a failed creation erases only an entry it made (a listed root and its records stay).
    if(!entry.root){const HRESULT hr=device->CreateRootSignature(0,blob,size,IID_PPV_ARGS(&entry.root));if(FAILED(hr)){if(fresh)roots_.erase(found);return hr;}
        NameRoot(entry.root.Get(),name);}
    // The blob this build gives replaces a listed one of the same root (an older build's programme), so the list carries this build's.
    // The final review's L1: a listed blob replaced marks the list unsaved (once a root a start), so the list carries it from then on.
    {const auto* p=static_cast<const uint8_t*>(blob);if(entry.blob.size()!=size || std::memcmp(entry.blob.data(),p,size)){if(!entry.blob.empty())++unsaved_;entry.blob.assign(p,p+size);}}
    *out=entry.root.Get();entry.root->AddRef();return S_OK;
}
// Under lock_. An object keeps its first name: one the runtime returns for two contents (two names) is counted, never renamed, so
// a pipeline's name never follows which blob made its root last (I-0132).
void PipelineStore::NameRoot(ID3D12RootSignature* root,const Name& name){
    if(const auto [named,fresh]=root_names_.try_emplace(root,name);!fresh && !(named->second==name))++counters_.root_aliases;
}
HRESULT PipelineStore::Acquire(ID3D12Device* device,const D3D12_GRAPHICS_PIPELINE_STATE_DESC* graphics,const D3D12_COMPUTE_PIPELINE_STATE_DESC* compute,
    ID3D12PipelineState** out,bool* created,bool precompile,uint8_t idle_if_new){
    if(!out)return E_POINTER;*out=nullptr;if(created)*created=false;
    // I-0132 R11i (backend.h): a depth-stencil format the description does not use is dropped before the description is named.
    D3D12_GRAPHICS_PIPELINE_STATE_DESC normalized;
    if(graphics && UnusedDepthStencilFormat(*graphics)){normalized=*graphics;normalized.DSVFormat=DXGI_FORMAT_UNKNOWN;graphics=&normalized;}
    const auto create=[&](ID3D12PipelineState** target){return graphics?device->CreateGraphicsPipelineState(graphics,IID_PPV_ARGS(target)):device->CreateComputePipelineState(compute,IID_PPV_ARGS(target));};
    // The programmes' names before the lock (a pure function of their bytes).
    std::array<Name,5> stages{};
    {const D3D12_SHADER_BYTECODE* bytecodes[5]{};size_t count=1;
        if(graphics){bytecodes[0]=&graphics->VS;bytecodes[1]=&graphics->PS;bytecodes[2]=&graphics->DS;bytecodes[3]=&graphics->HS;bytecodes[4]=&graphics->GS;count=5;}
        else bytecodes[0]=&compute->CS;
        for(size_t i=0;i<count;++i)if(bytecodes[i]->pShaderBytecode && bytecodes[i]->BytecodeLength)
            stages[i]=HashBytes(0x50524F4752414D45ULL,bytecodes[i]->pShaderBytecode,bytecodes[i]->BytecodeLength);}
    std::unique_lock hold(lock_);
    std::vector<uint8_t> encoding;
    if(device!=device_.Get() || !Encode(graphics,compute,stages,encoding)){++counters_.direct;hold.unlock();const HRESULT hr=create(out);
        if(SUCCEEDED(hr)){if(created)*created=true;const std::lock_guard again(lock_);++counters_.frame_created;}return hr;}
    const auto name=NameOf(encoding);
    // One thread builds a name; the others wait for that name only (the cache lock is not held while the driver works).
    // A frame may wait for a precompile worker at below-normal priority: boot only, accepted (review L6).
    if(building_.contains(name)){++counters_.waits;built_.wait(hold,[&]{return !building_.contains(name);});}
    if(const auto found=pipelines_.find(name);found!=pipelines_.end()){++counters_.memory_hits;
        if(!precompile)if(const auto record=records_.find(name);record!=records_.end())record->second.idle=0;
        *out=found->second.Get();(*out)->AddRef();return S_OK;}
    building_.insert(name);
    hold.unlock();
    // The driver's compilation (its own shader cache makes a description seen before cheap), never a library's object (I-0113).
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline;const HRESULT hr=create(pipeline.GetAddressOf());
    hold.lock();building_.erase(name);built_.notify_all();
    if(FAILED(hr))return hr;
    ++counters_.created;if(!precompile)++counters_.frame_created;
    auto& entry=records_[name];
    if(entry.encoding.empty()){entry.encoding=std::move(encoding);entry.idle=idle_if_new;++unsaved_;}
    if(!precompile)entry.idle=0;
    pipelines_.emplace(name,pipeline);
    *out=pipeline.Detach();if(created)*created=true;return S_OK;
}
HRESULT PipelineStore::Graphics(ID3D12Device* device,const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc,ID3D12PipelineState** out,bool* created){return Acquire(device,&desc,nullptr,out,created,false);}
HRESULT PipelineStore::Compute(ID3D12Device* device,const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc,ID3D12PipelineState** out,bool* created){return Acquire(device,nullptr,&desc,out,created,false);}
void PipelineStore::Precompile(unsigned workers){
    workers=(std::max)(1u,workers);
    {const std::lock_guard hold(lock_);
        if(!device_ || precompiling_)return;   // once per store (review L5: the flag is set under the lock)
        precompiling_=true;precompile_list_.clear();precompile_list_.reserve(records_.size());
        for(const auto& [name,record]:records_)if(!pipelines_.contains(name))precompile_list_.emplace_back(name,record.encoding);
        counters_.precompile_total=precompile_list_.size();counters_.precompile_done=precompile_list_.empty();
        if(precompile_list_.empty())return;
        precompile_next_=0;precompile_left_=workers;precompile_cancel_=false;QueryPerformanceCounter(&precompile_start_);}
    for(unsigned i=0;i<workers;++i){
        try{precompilers_.emplace_back([this]{SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_BELOW_NORMAL);PrecompileWorker();});}
        catch(...){if(precompile_left_.fetch_sub(1)==1){const std::lock_guard hold(lock_);counters_.precompile_done=true;}}}
}
void PipelineStore::PrecompileWorker(){
    for(;;){
        const size_t index=precompile_next_.fetch_add(1);
        if(precompile_cancel_.load(std::memory_order_relaxed) || index>=precompile_list_.size())break;   // the list is fixed while workers run
        const auto& [name,encoding]=precompile_list_[index];
        DecodedPipeline decoded;bool ok{};Microsoft::WRL::ComPtr<ID3D12RootSignature> root;Microsoft::WRL::ComPtr<ID3D12Device> device;uint8_t idle{};
        {const std::lock_guard hold(lock_);device=device_;if(const auto listed=records_.find(name);listed!=records_.end())idle=listed->second.idle;
            ok=device && Decode(encoding,[&](const Name& programme)->std::shared_ptr<const std::vector<uint8_t>>{const auto found=programmes_.find(programme);return found==programmes_.end()?nullptr:found->second;},decoded);
            // The record's root from its blob: the same object a stage asking Root for any blob of that content receives.
            const auto entry=ok?roots_.find(decoded.root):roots_.end();ok=entry!=roots_.end();
            if(ok && !entry->second.root && SUCCEEDED(device->CreateRootSignature(0,entry->second.blob.data(),entry->second.blob.size(),IID_PPV_ARGS(&entry->second.root))))
                NameRoot(entry->second.root.Get(),decoded.root);
            ok=ok && entry->second.root;if(ok)root=entry->second.root;}
        Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline;HRESULT hr=E_FAIL;
        // I-0132 R11i: a record listed before the rule names a format its pipeline no longer has: compiled under its new name
        // (Acquire), the old record replaced with its age (the step review's L2), a migration rather than a failure.
        const bool migrated=ok && !decoded.compute && UnusedDepthStencilFormat(decoded.graphics);
        if(ok){if(decoded.compute){decoded.cs.pRootSignature=root.Get();hr=Acquire(device.Get(),nullptr,&decoded.cs,&pipeline,nullptr,true);}
            else{decoded.graphics.pRootSignature=root.Get();hr=Acquire(device.Get(),&decoded.graphics,nullptr,&pipeline,nullptr,true,migrated?idle:uint8_t(0));}}
        const std::lock_guard hold(lock_);
        // A record whose description no longer names itself (a damaged or foreign record) is dropped from the list.
        if(SUCCEEDED(hr) && !pipelines_.contains(name)){records_.erase(name);++unsaved_;ok=migrated;}
        if(ok && SUCCEEDED(hr))++counters_.precompiled;else ++counters_.precompile_failed;
    }
    if(precompile_left_.fetch_sub(1)==1){LARGE_INTEGER end{},frequency{};QueryPerformanceCounter(&end);QueryPerformanceFrequency(&frequency);
        const std::lock_guard hold(lock_);counters_.precompile_done=true;
        counters_.precompile_ms=static_cast<uint64_t>(1000.0*double(end.QuadPart-precompile_start_.QuadPart)/double(frequency.QuadPart));}
}
bool PipelineStore::Save(){
    // Under the lock only a snapshot of the list (review M2: the frame requests and the workers must not wait for the checksums,
    // the list's assembly or the disk).
    std::wstring path;uint64_t unsaved{};
    std::vector<std::pair<uint8_t,std::vector<uint8_t>>> records;std::unordered_map<Name,std::vector<uint8_t>,NameHash> roots;
    std::unordered_map<Name,std::shared_ptr<const std::vector<uint8_t>>,NameHash> programmes;
    {const std::lock_guard hold(lock_);
        if(!open_ || !unsaved_ || path_.empty())return false;
        // Only complete records (their root and programmes held: review L3), with the roots and programmes they name.
        for(const auto& [name,record]:records_){Reader r{record.encoding.data(),record.encoding.data()+record.encoding.size()};
            const bool compute=r.U8()==compute_kind;r.U8();const auto root=r.N();const auto found=roots_.find(root);
            bool complete=found!=roots_.end() && !found->second.blob.empty();std::vector<Name> used;
            for(int stage=0;complete && stage<(compute?1:5);++stage)if(r.U64()){const auto programme=r.N();complete=programmes_.contains(programme);used.push_back(programme);}
            if(!complete || !r.ok)continue;
            roots.try_emplace(root,found->second.blob);for(const auto& programme:used)programmes.try_emplace(programme,programmes_.at(programme));
            records.emplace_back(record.idle,record.encoding);}
        path=path_;unsaved=unsaved_;}
    std::vector<uint8_t> list;Writer w{list};
    w.U32(list_magic);w.U32(list_version);
    w.U32(static_cast<uint32_t>(roots.size()));
    for(const auto& [name,blob]:roots){w.N(name);w.U32(static_cast<uint32_t>(blob.size()));w.Bytes(blob.data(),blob.size());}
    w.U32(static_cast<uint32_t>(programmes.size()));
    for(const auto& [name,bytes]:programmes){w.N(name);w.U32(static_cast<uint32_t>(bytes->size()));w.Bytes(bytes->data(),bytes->size());}
    w.U32(static_cast<uint32_t>(records.size()));
    for(const auto& [idle,encoding]:records){w.U8(idle);w.U32(static_cast<uint32_t>(encoding.size()));w.Bytes(encoding.data(),encoding.size());}
    w.U64(Checksum(list.data(),list.size()));
    // Written whole into a sibling file, then moved over the list (a reader never sees a half-written file).
    {const std::wstring temp=path+L".writing";FILE* file{};
        if(_wfopen_s(&file,temp.c_str(),L"wb") || !file)return false;
        const bool written=std::fwrite(list.data(),1,list.size(),file)==list.size();std::fclose(file);
        if(!written || !MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING)){DeleteFileW(temp.c_str());return false;}}
    const std::lock_guard hold(lock_);
    if(unsaved_>=unsaved)unsaved_-=unsaved;++counters_.saves;counters_.list_bytes=list.size();return true;
}
void PipelineStore::SaveAsync(){
    if(saver_.joinable()){if(saving_.load(std::memory_order_acquire))return;saver_.join();}   // the previous save, finished: joined
    saving_.store(true,std::memory_order_release);
    try{saver_=std::thread([this]{try{Save();}catch(...){}saving_.store(false,std::memory_order_release);});}
    catch(...){saving_.store(false,std::memory_order_release);}   // no thread: the next window or Close saves
}
void PipelineStore::Close() noexcept {
    precompile_cancel_=true;
    for(auto& worker:precompilers_)try{if(worker.joinable())worker.join();}catch(...){}
    precompilers_.clear();
    try{if(saver_.joinable())saver_.join();}catch(...){}
    try{Save();}catch(...){}
    const std::lock_guard hold(lock_);open_=false;pipelines_.clear();precompile_list_.clear();precompiling_=false;
    roots_.clear();root_names_.clear();records_.clear();programmes_.clear();device_.Reset();counters_.open=false;
}
}
