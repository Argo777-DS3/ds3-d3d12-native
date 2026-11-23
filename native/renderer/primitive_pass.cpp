#include "primitive_pass.h"
#include "../frame/primitive_work.h"
#include <algorithm>
#include <cstring>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <set>
#include <source_location>
#include <unordered_map>

namespace ds3native {
using Microsoft::WRL::ComPtr;
namespace {
void Check(HRESULT hr,const char* why){if(FAILED(hr))throw std::runtime_error(why);}
D3D12_HEAP_PROPERTIES Heap(D3D12_HEAP_TYPE type){D3D12_HEAP_PROPERTIES h{};h.Type=type;h.CreationNodeMask=h.VisibleNodeMask=1;return h;}
ComPtr<ID3D12Resource> Upload(ID3D12Device* device,const std::vector<uint8_t>& bytes,uint64_t minimum=0) {
    D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=std::max<uint64_t>(minimum,bytes.size());
    d.Height=1;d.DepthOrArraySize=d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    const auto heap=Heap(D3D12_HEAP_TYPE_UPLOAD);ComPtr<ID3D12Resource> result;
    Check(CountedResource(device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&result)),"primitive upload allocation");
    void* memory{};const D3D12_RANGE empty{};Check(result->Map(0,&empty,&memory),"primitive upload map");
    std::memset(memory,0,static_cast<size_t>(d.Width));std::memcpy(memory,bytes.data(),bytes.size());result->Unmap(0,nullptr);return result;
}
D3D12_CPU_DESCRIPTOR_HANDLE CPU(ID3D12DescriptorHeap* heap,uint32_t stride,uint32_t index){auto h=heap->GetCPUDescriptorHandleForHeapStart();h.ptr+=SIZE_T(stride)*index;return h;}
D3D12_GPU_DESCRIPTOR_HANDLE GPU(ID3D12DescriptorHeap* heap,uint32_t stride,uint32_t index){auto h=heap->GetGPUDescriptorHandleForHeapStart();h.ptr+=UINT64(stride)*index;return h;}
// 1129: per-device caches for the UI pass. Before 1129 every frame created the root signature, one pipeline per
// draw and re-uploaded every UI texture (Native1129: ~120 ms per in-game frame). The root signature and
// pipelines are reused per distinct description; a texture is reused only when a previous upload had exactly
// the same format, extent and mip bytes (byte comparison), and is evicted after 600 unused builds.
struct CachedTexture { std::shared_ptr<const SceneTexture> source;ComPtr<ID3D12Resource> resource;uint64_t last_build{}; };
struct PrimitiveCaches {
    std::recursive_mutex lock;ID3D12Device* device{};ComPtr<ID3D12RootSignature> root;uint64_t builds{};
    std::unordered_map<std::string,ComPtr<ID3D12PipelineState>> pipelines;
    std::unordered_multimap<uint64_t,CachedTexture> textures;
    // 1131: pooled upload chunks for UI geometry and constant blocks (returned when every holding build is cleared).
    struct UploadChunk {ID3D12Device* device{};ComPtr<ID3D12Resource> buffer;uint8_t* memory{};uint64_t used{};};
    std::vector<std::unique_ptr<UploadChunk>> free_uploads;std::shared_ptr<UploadChunk> uploads;
    // F1: free intermediate targets by size (colour R8G8B8A8_UNORM in PIXEL_SHADER_RESOURCE, D32S8 stencil in
    // DEPTH_WRITE when free); a build returns them when Clear() drops it after its fenced retirement.
    struct PooledTarget {ID3D12Device* device{};uint32_t width{},height{};ComPtr<ID3D12Resource> color,stencil;};
    std::vector<std::unique_ptr<PooledTarget>> free_targets;
    // m45 frame cost (Native1841 sample profile: a fresh shader-visible heap per build, two a frame, through the driver's
    // allocation): free descriptor heaps by type, a power-of-two capacity, returned when the build is dropped after its
    // fenced retirement, as the upload chunks.
    struct PooledHeap {ID3D12Device* device{};D3D12_DESCRIPTOR_HEAP_TYPE type{};UINT capacity{};ComPtr<ID3D12DescriptorHeap> heap;};
    std::vector<std::unique_ptr<PooledHeap>> free_heaps;
};
PrimitiveCaches& Caches(){static auto* caches=new PrimitiveCaches;return *caches;} // process lifetime by design
constexpr uint64_t upload_chunk_bytes=4ull<<20;
std::shared_ptr<PrimitiveCaches::UploadChunk> PooledUploads(PrimitiveCaches& c,ID3D12Device* device){
    std::unique_ptr<PrimitiveCaches::UploadChunk> chunk;
    if(!c.free_uploads.empty()){chunk=std::move(c.free_uploads.back());c.free_uploads.pop_back();}
    else{chunk=std::make_unique<PrimitiveCaches::UploadChunk>();chunk->device=device;
        D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=upload_chunk_bytes;d.Height=1;d.DepthOrArraySize=d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        const auto heap=Heap(D3D12_HEAP_TYPE_UPLOAD);
        Check(CountedResource(device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&chunk->buffer)),"primitive upload chunk allocation");
        void* memory{};const D3D12_RANGE empty{};Check(chunk->buffer->Map(0,&empty,&memory),"primitive upload chunk map");chunk->memory=static_cast<uint8_t*>(memory);}
    chunk->used=0;
    // R2b: back to the pool once the last holder is gone and the queue has completed what was submitted until then.
    return std::shared_ptr<PrimitiveCaches::UploadChunk>(chunk.release(),[](PrimitiveCaches::UploadChunk* held){
        RetireWhenSubmittedWorkCompletes(std::shared_ptr<void>(held,[](void* p){auto* returned=static_cast<PrimitiveCaches::UploadChunk*>(p);auto& caches=Caches();
            const std::lock_guard hold(caches.lock);if(returned->device==caches.device)caches.free_uploads.emplace_back(returned);else delete returned;}));});
}
std::shared_ptr<PrimitiveCaches::PooledTarget> PooledTarget(PrimitiveCaches& c,ID3D12Device* device,uint32_t width,uint32_t height){
    std::unique_ptr<PrimitiveCaches::PooledTarget> target;
    for(auto it=c.free_targets.begin();it!=c.free_targets.end();++it)
        if((*it)->width==width && (*it)->height==height){target=std::move(*it);c.free_targets.erase(it);break;}
    if(!target){target=std::make_unique<PrimitiveCaches::PooledTarget>();target->device=device;target->width=width;target->height=height;
        const auto heap=Heap(D3D12_HEAP_TYPE_DEFAULT);
        D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=width;d.Height=height;d.DepthOrArraySize=1;d.MipLevels=1;
        d.Format=DXGI_FORMAT_R8G8B8A8_UNORM;d.SampleDesc.Count=1;d.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        const D3D12_CLEAR_VALUE transparent{DXGI_FORMAT_R8G8B8A8_UNORM,{0,0,0,0}};
        Check(CountedResource(device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,&transparent,IID_PPV_ARGS(&target->color)),"primitive target allocation");
        d.Format=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;d.Flags=D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL|D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        D3D12_CLEAR_VALUE zero{};zero.Format=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        Check(CountedResource(device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_DEPTH_WRITE,&zero,IID_PPV_ARGS(&target->stencil)),"primitive target stencil allocation");}
    return std::shared_ptr<PrimitiveCaches::PooledTarget>(target.release(),[](PrimitiveCaches::PooledTarget* held){
        RetireWhenSubmittedWorkCompletes(std::shared_ptr<void>(held,[](void* p){auto* returned=static_cast<PrimitiveCaches::PooledTarget*>(p);auto& caches=Caches();
            const std::lock_guard hold(caches.lock);
            if(returned->device==caches.device && caches.free_targets.size()<64)caches.free_targets.emplace_back(returned);else delete returned;}));});
}
std::shared_ptr<PrimitiveCaches::PooledHeap> PooledDescriptorHeap(PrimitiveCaches& c,ID3D12Device* device,D3D12_DESCRIPTOR_HEAP_TYPE type,UINT count){
    UINT capacity=64;while(capacity<count)capacity*=2;
    std::unique_ptr<PrimitiveCaches::PooledHeap> pooled;
    for(auto it=c.free_heaps.begin();it!=c.free_heaps.end();++it)
        if((*it)->type==type && (*it)->capacity==capacity){pooled=std::move(*it);c.free_heaps.erase(it);break;}
    if(!pooled){pooled=std::make_unique<PrimitiveCaches::PooledHeap>();pooled->device=device;pooled->type=type;pooled->capacity=capacity;
        const D3D12_DESCRIPTOR_HEAP_DESC heap{type,capacity,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
        Check(CountedHeap(device)->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&pooled->heap)),
            type==D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER?"primitive sampler descriptors":"primitive texture descriptors");}
    return std::shared_ptr<PrimitiveCaches::PooledHeap>(pooled.release(),[](PrimitiveCaches::PooledHeap* held){
        RetireWhenSubmittedWorkCompletes(std::shared_ptr<void>(held,[](void* p){auto* returned=static_cast<PrimitiveCaches::PooledHeap*>(p);auto& caches=Caches();
            const std::lock_guard hold(caches.lock);
            if(returned->device==caches.device && caches.free_heaps.size()<32)caches.free_heaps.emplace_back(returned);else delete returned;}));});
}
uint64_t TextureKey(const SceneTexture& t){uint64_t h=1469598103934665603ULL;
    const auto mix=[&](uint64_t v){h^=v;h*=1099511628211ULL;};mix(t.format);mix(t.width);mix(t.height);mix(t.mips.size());
    for(const auto& mip:t.mips){mix(mip.size());const size_t step=mip.size()>65536?61:1;for(size_t i=0;i<mip.size();i+=step)mix(mip[i]);}
    return h;}
template<class T> void Append(std::string& key,const T& value){key.append(reinterpret_cast<const char*>(&value),sizeof(value));}
void Transition(ID3D12GraphicsCommandList* list,ID3D12Resource* image,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after,
    std::source_location where=std::source_location::current()){   // E0: counted with its call site
    NoteBarrier(1,where.line());D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={image,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};list->ResourceBarrier(1,&b);
}
}
PrimitivePass::PrimitivePass()=default;
PrimitivePass::~PrimitivePass()=default;
PrimitivePass::PrimitivePass(PrimitivePass&& other) noexcept:packet_(std::move(other.packet_)),root_(std::move(other.root_)),
    bindings_(std::move(other.bindings_)),samplers_(std::move(other.samplers_)),buffers_(std::move(other.buffers_)),textures_(std::move(other.textures_)),
    uploads_(std::move(other.uploads_)),pooled_(std::move(other.pooled_)),pooled_buffers_(std::move(other.pooled_buffers_)),draws_(std::move(other.draws_)),
    binding_stride_(other.binding_stride_),sampler_stride_(other.sampler_stride_),targets_(std::move(other.targets_)),target_slots_(std::move(other.target_slots_)),
    target_rtvs_(std::move(other.target_rtvs_)),target_dsvs_(std::move(other.target_dsvs_)),rtv_stride_(other.rtv_stride_),dsv_stride_(other.dsv_stride_),float_target_(other.float_target_){other.Clear();}
void PrimitivePass::Clear() noexcept {packet_.reset();draws_.clear();uploads_.clear();buffers_.clear();textures_.clear();pooled_buffers_.clear();pooled_.clear();root_.Reset();bindings_.Reset();samplers_.Reset();binding_stride_=sampler_stride_=0;
    targets_.clear();target_slots_.clear();target_rtvs_.Reset();target_dsvs_.Reset();rtv_stride_=dsv_stride_=0;float_target_=false;}
void PrimitivePass::QuarantineToProcessExit() noexcept {
    for(auto& draw:draws_)(void)draw.pipeline.Detach();
    for(const auto& chunk:pooled_)(void)new std::shared_ptr<void>(chunk); // 1131: pooled chunks stay pinned with the quarantined work
    for(const auto& target:targets_)(void)new std::shared_ptr<void>(target.pooled);   // F1: same for the targets
    (void)target_rtvs_.Detach();(void)target_dsvs_.Detach();
    for(auto& [id,buffer]:buffers_){(void)id;(void)buffer.Detach();}for(auto& [id,texture]:textures_){(void)id;(void)texture.Detach();}
    for(auto& upload:uploads_)(void)upload.Detach();(void)root_.Detach();(void)bindings_.Detach();(void)samplers_.Detach();
}
void PrimitivePass::Build(ID3D12Device* device,ID3D12GraphicsCommandList* list,const frame::PrimitiveWorkPacket& packet,bool float_target) {
    Build(device,list,frame::PrimitiveWorkPacket(packet),float_target);
}
void PrimitivePass::Build(ID3D12Device* device,ID3D12GraphicsCommandList* list,frame::PrimitiveWorkPacket&& incoming,bool float_target) {
    if(!device || !list || packet_)throw std::invalid_argument("primitive residency build state");
    // SceneRenderer performs complete CPU preflight before retiring old work.
    // Keep every recorded resource in this owner until its fenced retirement.
    packet_=std::make_unique<frame::PrimitiveWorkPacket>(std::move(incoming));const auto& packet=*packet_;float_target_=float_target;
    const D3D12_DESCRIPTOR_RANGE ranges[]={
        {D3D12_DESCRIPTOR_RANGE_TYPE_SRV,2,0,0,0},
        {D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,2,0,0,0}};
    D3D12_ROOT_PARAMETER parameters[4]{};
    for(unsigned i=0;i<2;++i){parameters[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;parameters[i].Descriptor={0,0};parameters[i].ShaderVisibility=i?D3D12_SHADER_VISIBILITY_PIXEL:D3D12_SHADER_VISIBILITY_VERTEX;}
    for(unsigned i=0;i<2;++i){parameters[2+i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[2+i].DescriptorTable={1,&ranges[i]};parameters[2+i].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;}
    D3D12_ROOT_SIGNATURE_DESC signature{4,parameters,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT|
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS|D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS|D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS};
    auto& caches=Caches();const std::lock_guard hold(caches.lock);
    if(caches.device!=device){caches.device=device;caches.root.Reset();caches.pipelines.clear();caches.textures.clear();caches.free_uploads.clear();caches.uploads.reset();caches.free_targets.clear();caches.free_heaps.clear();}
    // F1: the packet's targets live from their first to their last referencing draw (written or sampled). In draw
    // order, a target starting at a draw takes a free resource of its size (or a new pooled one) before the targets
    // ending at that draw return theirs, so a draw never reads and writes the same resource.
    if(!packet.targets.empty()){
        std::map<uint64_t,const frame::PrimitiveTarget*> wanted;for(const auto& target:packet.targets)wanted.emplace(target.identity,&target);
        std::map<uint64_t,uint64_t> sampled;for(const auto& texture:packet.textures)if(texture.target)sampled.emplace(texture.identity,texture.target);
        std::map<uint64_t,std::pair<size_t,size_t>> lives;
        for(size_t ordinal=0;ordinal<packet.draws.size();++ordinal){const auto& d=packet.draws[ordinal];
            for(const auto id:{d.target,d.texture?(sampled.contains(d.texture)?sampled.at(d.texture):0):0,d.secondary.texture?(sampled.contains(d.secondary.texture)?sampled.at(d.secondary.texture):0):0})
                if(id){auto [life,fresh]=lives.try_emplace(id,ordinal,ordinal);if(!fresh)life->second.second=ordinal;}}
        std::vector<std::vector<uint64_t>> starts(packet.draws.size()),ends(packet.draws.size());
        for(const auto& [id,life]:lives){starts[life.first].push_back(id);ends[life.second].push_back(id);}
        std::multimap<std::pair<uint32_t,uint32_t>,uint32_t> free;
        for(size_t ordinal=0;ordinal<packet.draws.size();++ordinal){
            for(const auto id:starts[ordinal]){const auto& t=*wanted.at(id);const auto size=std::make_pair(t.width,t.height);
                if(const auto found=free.find(size);found!=free.end()){target_slots_.emplace(id,found->second);free.erase(found);continue;}
                auto pooled=PooledTarget(caches,device,t.width,t.height);
                target_slots_.emplace(id,static_cast<uint32_t>(targets_.size()));
                targets_.push_back(Target{pooled,pooled->color.Get(),pooled->stencil.Get(),t.width,t.height});}
            for(const auto id:ends[ordinal]){const auto slot=target_slots_.at(id);free.emplace(std::make_pair(targets_[slot].width,targets_[slot].height),slot);}
        }
        if(!targets_.empty()){
            D3D12_DESCRIPTOR_HEAP_DESC views{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,static_cast<UINT>(targets_.size()),D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
            Check(CountedHeap(device)->CreateDescriptorHeap(&views,IID_PPV_ARGS(&target_rtvs_)),"primitive target views");rtv_stride_=device->GetDescriptorHandleIncrementSize(views.Type);
            views.Type=D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
            Check(CountedHeap(device)->CreateDescriptorHeap(&views,IID_PPV_ARGS(&target_dsvs_)),"primitive target stencil views");dsv_stride_=device->GetDescriptorHandleIncrementSize(views.Type);
            for(uint32_t slot=0;slot<targets_.size();++slot){
                device->CreateRenderTargetView(targets_[slot].color,nullptr,CPU(target_rtvs_.Get(),rtv_stride_,slot));
                device->CreateDepthStencilView(targets_[slot].stencil,nullptr,CPU(target_dsvs_.Get(),dsv_stride_,slot));}}}
    // Same contents as the former per-buffer upload: the bytes, zero padding up to the requested extent.
    const auto suballocate=[&](const std::vector<uint8_t>& bytes,uint64_t minimum)->Suballocation{
        const uint64_t size=std::max<uint64_t>(minimum,bytes.size()),allocation=(size+255)&~uint64_t(255);
        if(allocation>upload_chunk_bytes){uploads_.push_back(Upload(device,bytes,minimum));return {uploads_.back().Get(),uploads_.back()->GetGPUVirtualAddress(),size};}
        if(!caches.uploads || caches.uploads->used+allocation>upload_chunk_bytes)caches.uploads=PooledUploads(caches,device);
        if(pooled_.empty() || pooled_.back()!=caches.uploads)pooled_.push_back(caches.uploads);
        auto& chunk=*caches.uploads;const auto offset=chunk.used;chunk.used+=allocation;
        std::memset(chunk.memory+offset,0,static_cast<size_t>(allocation));std::memcpy(chunk.memory+offset,bytes.data(),bytes.size());
        return {chunk.buffer.Get(),chunk.buffer->GetGPUVirtualAddress()+offset,size};};
    ++caches.builds;
    if(!caches.root){ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&signature,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"primitive root serialization");
        Check(Pipelines().Root(device,blob->GetBufferPointer(),blob->GetBufferSize(),caches.root.ReleaseAndGetAddressOf()),"primitive root allocation");}
    root_=caches.root;
    {auto pooled=PooledDescriptorHeap(caches,device,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,static_cast<UINT>(packet.draws.size()*2));bindings_=pooled->heap;pooled_.push_back(std::move(pooled));}
    binding_stride_=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    // One sampler pair (s0, s1) per distinct description: a shader-visible sampler heap holds at most 2048 descriptors.
    std::map<std::string,uint32_t> sampler_pairs;std::vector<uint32_t> draw_pairs;draw_pairs.reserve(packet.draws.size());
    for(const auto& d:packet.draws){std::string key;for(const auto& image:{frame::PrimitiveImageBinding{0,d.sampler,d.mip_bias,d.min_lod,d.max_lod},d.secondary}){
            Append(key,image.sampler);Append(key,image.mip_bias);Append(key,image.min_lod);Append(key,image.max_lod);}
        draw_pairs.push_back(sampler_pairs.try_emplace(key,static_cast<uint32_t>(sampler_pairs.size())).first->second);}
    if(sampler_pairs.size()>1024)throw std::runtime_error("primitive sampler pair extent");
    std::vector<bool> sampler_written(sampler_pairs.size());
    {auto pooled=PooledDescriptorHeap(caches,device,D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,static_cast<UINT>(sampler_pairs.size()*2));samplers_=pooled->heap;pooled_.push_back(std::move(pooled));}
    sampler_stride_=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    for(const auto& buffer:packet.buffers)pooled_buffers_.emplace(buffer.identity,suballocate(buffer.bytes,0));
    for(size_t t=0;t<packet.textures.size();++t){
        const auto& entry=packet.textures[t];const auto& image=entry.texture;
        if(entry.target){   // F1: sampled as earlier draws left it (a target no draw references has no resource)
            if(const auto slot=target_slots_.find(entry.target);slot!=target_slots_.end())textures_[entry.identity]=targets_[slot->second].color;
            continue;}
        if(entry.external){   // 1169/1250: a native render target or resident imported asset sampled as is (PIXEL|NON_PIXEL_SHADER_RESOURCE)
            const auto actual=entry.external->GetDesc();
            if(actual.Width!=image.width || actual.Height!=image.height || actual.Format!=image.format || actual.MipLevels!=image.mips.size())
                throw std::runtime_error("primitive external texture description");
            textures_[entry.identity]=entry.external;continue;}
        D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=image.width;d.Height=image.height;d.DepthOrArraySize=1;
        d.MipLevels=static_cast<UINT16>(image.mips.size());d.Format=image.format;d.SampleDesc.Count=1;
        const auto key=TextureKey(image);auto& resource=textures_[entry.identity];
        {const auto [first,last]=caches.textures.equal_range(key);bool reused=false;
            for(auto it=first;it!=last;++it)if(it->second.source->format==image.format && it->second.source->width==image.width &&
                it->second.source->height==image.height && it->second.source->mips==image.mips){resource=it->second.resource;it->second.last_build=caches.builds;reused=true;break;}
            if(reused)continue;}
        const auto h=Heap(D3D12_HEAP_TYPE_DEFAULT);
        Check(CountedResource(device)->CreateCommittedResource(&h,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&resource)),"primitive texture allocation");
        std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(image.mips.size());std::vector<UINT> rows(image.mips.size());std::vector<UINT64> row_bytes(image.mips.size());UINT64 total{};
        device->GetCopyableFootprints(&d,0,d.MipLevels,0,footprints.data(),rows.data(),row_bytes.data(),&total);
        std::vector<uint8_t> bytes(static_cast<size_t>(total));
        for(size_t mip=0;mip<image.mips.size();++mip){
            if(UINT64(rows[mip])*row_bytes[mip]!=image.mips[mip].size())throw std::runtime_error("primitive exact texture footprint");
            for(UINT row=0;row<rows[mip];++row)std::memcpy(bytes.data()+footprints[mip].Offset+UINT64(row)*footprints[mip].Footprint.RowPitch,
                image.mips[mip].data()+UINT64(row)*row_bytes[mip],static_cast<size_t>(row_bytes[mip]));
        }
        uploads_.push_back(Upload(device,bytes));
        for(UINT mip=0;mip<d.MipLevels;++mip){D3D12_TEXTURE_COPY_LOCATION from{},to{};from.pResource=uploads_.back().Get();from.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;from.PlacedFootprint=footprints[mip];to.pResource=resource.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;to.SubresourceIndex=mip;list->CopyTextureRegion(&to,0,0,0,&from,nullptr);}
        Transition(list,resource.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        caches.textures.emplace(key,CachedTexture{std::make_shared<const SceneTexture>(image),resource,caches.builds});
    }
    for(auto it=caches.textures.begin();it!=caches.textures.end();)if(it->second.last_build+600<caches.builds)it=caches.textures.erase(it);else ++it;
    std::map<uint64_t,const frame::PrimitiveProgram*> programs;for(const auto& p:packet.programs)programs.emplace(p.identity,&p);
    std::unordered_map<uint64_t,uint64_t> texture_targets;for(const auto& t:packet.textures)if(t.target)texture_targets.emplace(t.identity,t.target);
    for(size_t ordinal=0;ordinal<packet.draws.size();++ordinal){
        const auto& source=packet.draws[ordinal];draws_.emplace_back();auto& resident=draws_.back();resident.texture_slot=static_cast<uint32_t>(ordinal*2);resident.sampler_slot=draw_pairs[ordinal]*2;
        const bool new_samplers=!sampler_written[draw_pairs[ordinal]];sampler_written[draw_pairs[ordinal]]=true;
        const auto& vertex=pooled_buffers_.at(source.vertices);resident.vertices={vertex.address+source.vertex_offset,static_cast<UINT>(vertex.size-source.vertex_offset),source.stride};
        if(source.indexed){const auto& index=pooled_buffers_.at(source.indices);resident.indices={index.address+source.index_offset,static_cast<UINT>(index.size-source.index_offset),source.index_width==2?DXGI_FORMAT_R16_UINT:DXGI_FORMAT_R32_UINT};}
        for(const auto& cb:source.constants){const auto slot=cb.stage==SceneShaderStage::Vertex?0u:1u;resident.constants[slot]=suballocate(cb.bytes,(cb.bytes.size()+255u)&~UINT64(255)).address;}
        const frame::PrimitiveImageBinding images[]{{source.texture,source.sampler,source.mip_bias,source.min_lod,source.max_lod},source.secondary};
        for(uint32_t slot=0;slot<2;++slot)if(const auto read=texture_targets.find(images[slot].texture);read!=texture_targets.end())resident.reads[slot]=read->second;
        for(uint32_t slot=0;slot<2;++slot){const auto& image=images[slot];
        ID3D12Resource* resource=image.texture?textures_.at(image.texture).Get():nullptr;
        D3D12_SHADER_RESOURCE_VIEW_DESC view{};view.Format=resource?resource->GetDesc().Format:DXGI_FORMAT_R8G8B8A8_UNORM;
        view.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;view.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        view.Texture2D.MipLevels=resource?resource->GetDesc().MipLevels:1;
        device->CreateShaderResourceView(resource,&view,CPU(bindings_.Get(),binding_stride_,resident.texture_slot+slot));
        D3D12_SAMPLER_DESC sampler{};
        sampler.Filter=(image.sampler==frame::PrimitiveSampler::PointWrap || image.sampler==frame::PrimitiveSampler::PointClamp)?D3D12_FILTER_MIN_MAG_MIP_POINT:D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU=sampler.AddressV=(image.sampler==frame::PrimitiveSampler::LinearWrap || image.sampler==frame::PrimitiveSampler::PointWrap)?D3D12_TEXTURE_ADDRESS_MODE_WRAP:D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        // These supported filters are non-comparison. The original ALWAYS
        // field is ignored; NEVER avoids the D3D12 debug-layer warning.
        sampler.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;sampler.MipLODBias=image.mip_bias;sampler.MaxAnisotropy=1;sampler.ComparisonFunc=D3D12_COMPARISON_FUNC_NEVER;sampler.MinLOD=image.min_lod;sampler.MaxLOD=image.max_lod;
        if(new_samplers)device->CreateSampler(&sampler,CPU(samplers_.Get(),sampler_stride_,resident.sampler_slot+slot));}
        std::vector<D3D12_INPUT_ELEMENT_DESC> elements;for(const auto& e:source.elements)elements.push_back({e.semantic.c_str(),e.semantic_index,e.format,0,e.byte_offset,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0});
        const auto& vs=programs.at(source.vertex_program)->program.bytecode;const auto& ps=programs.at(source.pixel_program)->program.bytecode;
        D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline{};pipeline.pRootSignature=root_.Get();pipeline.VS={vs.data(),vs.size()};pipeline.PS={ps.data(),ps.size()};pipeline.InputLayout={elements.data(),static_cast<UINT>(elements.size())};
        pipeline.SampleMask=UINT_MAX;pipeline.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;pipeline.NumRenderTargets=1;pipeline.RTVFormats[0]=float_target && !source.target?DXGI_FORMAT_R16G16B16A16_FLOAT:DXGI_FORMAT_R8G8B8A8_UNORM;pipeline.DSVFormat=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;   // I-0138 tranche 5 (the header)
       pipeline.SampleDesc.Count=1;
        pipeline.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;pipeline.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;pipeline.RasterizerState.DepthClipEnable=TRUE;
        auto& blend=pipeline.BlendState.RenderTarget[0];blend.BlendOp=blend.BlendOpAlpha=D3D12_BLEND_OP_ADD;blend.SrcBlend=blend.SrcBlendAlpha=D3D12_BLEND_ONE;blend.DestBlend=blend.DestBlendAlpha=D3D12_BLEND_ZERO;
        blend.RenderTargetWriteMask=source.blend==frame::PrimitiveBlend::NoColor?0:D3D12_COLOR_WRITE_ENABLE_ALL;
        if(source.blend!=frame::PrimitiveBlend::NoColor){blend.BlendEnable=TRUE;blend.SrcBlend=(source.blend==frame::PrimitiveBlend::PremultipliedAlpha || source.blend==frame::PrimitiveBlend::PremultipliedAdditive)?D3D12_BLEND_ONE:D3D12_BLEND_SRC_ALPHA;
            blend.DestBlend=blend.DestBlendAlpha=(source.blend==frame::PrimitiveBlend::AdditiveAlpha || source.blend==frame::PrimitiveBlend::PremultipliedAdditive)?D3D12_BLEND_ONE:D3D12_BLEND_INV_SRC_ALPHA;
            // Authored rows3/4 (1F79850, state tables183). Premultiplication
            // only replaces SRC_ALPHA, so these color factors stay unchanged.
            if(source.blend==frame::PrimitiveBlend::Multiply)blend.SrcBlend=D3D12_BLEND_DEST_COLOR;
            if(source.blend==frame::PrimitiveBlend::Screen){blend.SrcBlend=D3D12_BLEND_INV_DEST_COLOR;blend.DestBlend=D3D12_BLEND_ONE;}}
        auto& depth=pipeline.DepthStencilState;depth.DepthFunc=D3D12_COMPARISON_FUNC_ALWAYS;depth.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ZERO;depth.StencilEnable=source.stencil!=frame::PrimitiveStencil::Disabled;depth.StencilReadMask=depth.StencilWriteMask=255;
        auto& stencil=depth.FrontFace;stencil.StencilFailOp=stencil.StencilDepthFailOp=stencil.StencilPassOp=D3D12_STENCIL_OP_KEEP;stencil.StencilFunc=D3D12_COMPARISON_FUNC_ALWAYS;
        if(source.stencil==frame::PrimitiveStencil::ReplaceZero)stencil.StencilPassOp=D3D12_STENCIL_OP_REPLACE;
        if(source.stencil==frame::PrimitiveStencil::IncrementEqualZero || source.stencil==frame::PrimitiveStencil::WriteMaskLevel){stencil.StencilFunc=D3D12_COMPARISON_FUNC_EQUAL;stencil.StencilPassOp=D3D12_STENCIL_OP_INCR;}
        if(source.stencil==frame::PrimitiveStencil::ReadOne || source.stencil==frame::PrimitiveStencil::ReadMaskLevel)stencil.StencilFunc=D3D12_COMPARISON_FUNC_LESS_EQUAL;
        if(source.stencil==frame::PrimitiveStencil::ClampToReference){stencil.StencilFunc=D3D12_COMPARISON_FUNC_LESS_EQUAL;stencil.StencilPassOp=D3D12_STENCIL_OP_REPLACE;}
        depth.BackFace=stencil;
        std::string key;Append(key,vs.size());key.append(reinterpret_cast<const char*>(vs.data()),vs.size());Append(key,ps.size());key.append(reinterpret_cast<const char*>(ps.data()),ps.size());
        for(const auto& e:source.elements){key.append(e.semantic).push_back('\0');Append(key,e.semantic_index);Append(key,e.format);Append(key,e.byte_offset);}
        Append(key,source.blend);Append(key,source.stencil);
        if(float_target && !source.target)key.push_back('F');   // I-0138 tranche 5: the float target's pipeline is another one
        if(const auto found=caches.pipelines.find(key);found!=caches.pipelines.end())resident.pipeline=found->second;
        else{Check(Pipelines().Graphics(device,pipeline,resident.pipeline.ReleaseAndGetAddressOf()),"primitive real-program pipeline");caches.pipelines.emplace(std::move(key),resident.pipeline);}
    }
}
void PrimitivePass::Encode(ID3D12GraphicsCommandList* list,D3D12_CPU_DESCRIPTOR_HANDLE color,D3D12_CPU_DESCRIPTOR_HANDLE stencil,bool draw) const {
    if(!packet_ || draws_.size()!=packet_->draws.size())throw std::invalid_argument("primitive resident group missing");
    list->OMSetRenderTargets(1,&color,FALSE,&stencil);list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D12DescriptorHeap* heaps[]={bindings_.Get(),samplers_.Get()};list->SetDescriptorHeaps(2,heaps);list->SetGraphicsRootSignature(root_.Get());
    // F1: draws are encoded in order. A packet target is cleared to transparent with stencil 0 at its first reference
    // (written or sampled); at each change of target the resource left becomes PIXEL_SHADER_RESOURCE (later draws may
    // sample it) and the resource entered becomes RENDER_TARGET. Every resource is back in PIXEL_SHADER_RESOURCE at the end.
    uint64_t current{};std::set<uint64_t> begun;
    auto bound_stencil=stencil;LONG bound_width=static_cast<LONG>(packet_->width),bound_height=static_cast<LONG>(packet_->height);
    const auto bind=[&](uint64_t next){
        if(next==current)return;
        if(current)Transition(list,targets_[target_slots_.at(current)].color,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        current=next;
        if(!next){list->OMSetRenderTargets(1,&color,FALSE,&stencil);bound_stencil=stencil;
            bound_width=static_cast<LONG>(packet_->width);bound_height=static_cast<LONG>(packet_->height);return;}
        const auto slot=target_slots_.at(next);const auto& target=targets_[slot];
        Transition(list,target.color,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);
        const auto rtv=CPU(target_rtvs_.Get(),rtv_stride_,slot),dsv=CPU(target_dsvs_.Get(),dsv_stride_,slot);
        list->OMSetRenderTargets(1,&rtv,FALSE,&dsv);bound_stencil=dsv;bound_width=static_cast<LONG>(target.width);bound_height=static_cast<LONG>(target.height);};
    const auto begin=[&](uint64_t id){
        if(!id || !begun.insert(id).second)return;
        const auto slot=target_slots_.at(id);
        // The resource may still be bound for an earlier target whose life ended at the previous draw.
        if(current && target_slots_.at(current)==slot)bind(0);
        bind(id);const float transparent[4]{};
        list->ClearRenderTargetView(CPU(target_rtvs_.Get(),rtv_stride_,slot),transparent,0,nullptr);
        list->ClearDepthStencilView(CPU(target_dsvs_.Get(),dsv_stride_,slot),D3D12_CLEAR_FLAG_STENCIL,0,0,0,nullptr);};
    for(size_t ordinal=0;ordinal<draws_.size();++ordinal){const auto& source=packet_->draws[ordinal];const auto& resident=draws_[ordinal];
        for(const auto id:{resident.reads[0],resident.reads[1],source.target})begin(id);
        bind(source.target);
        if(source.clear_stencil_before)list->ClearDepthStencilView(bound_stencil,D3D12_CLEAR_FLAG_STENCIL,0,0,0,nullptr);
        const auto& v=source.viewport;const D3D12_VIEWPORT viewport{v.x,v.y,v.width,v.height,v.min_depth,v.max_depth};
        const D3D12_RECT scissor=source.scissor_enabled?D3D12_RECT{source.scissor[0],source.scissor[1],source.scissor[2],source.scissor[3]}:D3D12_RECT{0,0,bound_width,bound_height};
        list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
        list->SetPipelineState(resident.pipeline.Get());
        list->IASetVertexBuffers(0,1,&resident.vertices);list->IASetIndexBuffer(source.indexed?&resident.indices:nullptr);list->OMSetStencilRef(source.stencil==frame::PrimitiveStencil::ReadOne?1u:source.stencil_reference);
        const float factors[]={1,1,1,1};list->OMSetBlendFactor(factors);
        for(unsigned stage=0;stage<2;++stage)if(resident.constants[stage])list->SetGraphicsRootConstantBufferView(stage,resident.constants[stage]);
        list->SetGraphicsRootDescriptorTable(2,GPU(bindings_.Get(),binding_stride_,resident.texture_slot));list->SetGraphicsRootDescriptorTable(3,GPU(samplers_.Get(),sampler_stride_,resident.sampler_slot));
        if(draw){if(source.indexed)list->DrawIndexedInstanced(source.count,source.instances,source.start,source.base_vertex,source.first_instance);else list->DrawInstanced(source.count,source.instances,source.start,source.first_instance);}
    }
    bind(0);
}
}
