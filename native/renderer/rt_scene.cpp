#include <condition_variable>
#include "rt_scene.h"
#include "rt_skin.dxil.h"   // dxc output (Build-GameBootstrap.ps1): const unsigned char rt_skin_dxil[]
#include "rt_view.dxil.h"   // I-0117 T4: rt_view_dxil[]
#include "rt_shadow_vs.dxil.h"      // I-0117 R5: rt_shadow_vs_dxil[], rt_shadow_fill_dxil[], rt_shadow_trace_dxil[]
#include "rt_shadow_fill.dxil.h"
#include "rt_shadow_trace.dxil.h"
#include "rt_shadow_blur.dxil.h"       // I-0140 tranche 35: rt_shadow_blur_dxil[]
#include "rt_lights_vs.dxil.h"       // I-0122 R4a: rt_lights_vs_dxil[], rt_lights_shade_dxil[]
#include "rt_lights_shade.dxil.h"
#include "rt_lights_pt.dxil.h"   // I-0122 R4c: rt_lights_pt_dxil[]
#include "rt_reflect_vs.dxil.h"   // I-0125 R6b: rt_reflect_vs_dxil[], rt_reflect_trace_dxil[]
#include "rt_reflect_trace.dxil.h"
#include "rt_reflect_trace_hit.dxil.h"   // I-0125 R6c: rt_reflect_trace_hit_dxil[], rt_reflect_water_dxil[]
#include "rt_reflect_trace_hit_path.dxil.h"   // I-0128 R8d: with PathFrom (RT_PATHS 1)
#include "rt_reflect_gi_path.dxil.h"
#include "rt_reflect_water.dxil.h"
#include "rt_reflect_gi.dxil.h"   // I-0126 R7a: rt_reflect_gi_dxil[]
#include "rt_reflect_nrd_signal.dxil.h"    // I-0126 R7c: rt_reflect_nrd_signal_dxil[] (I-0132 R11: the specular's input too), rt_reflect_gi_compose_dxil[]
#include "rt_reflect_gi_compose.dxil.h"
#include "rt_reflect_spec_compose.dxil.h"   // I-0132 R11: rt_reflect_spec_compose_dxil[]
#include "rt_reflect_gi_temporal.dxil.h"   // I-0126 R7d: ReSTIR GI's draws
#include "rt_reflect_gi_spatial.dxil.h"
#include "rt_reflect_gi_shade.dxil.h"
#include "rt_reflect_ser.dxil.h"   // I-0132 R11b: rt_reflect_ser_dxil[] (lib_6_9: trace_hit_ser, gi_temporal_ser)
#include "rt_reflect_ser_apply.dxil.h"   // rt_reflect_ser_apply_dxil[]
#include "rt_denoise.h"   // kRtDenoiseInputFormats
#include "rt_reflect_cache_update.dxil.h"    // I-0126 R7b: rt_reflect_cache_update_dxil[], rt_reflect_cache_resolve_dxil[]
#include "rt_reflect_cache_resolve.dxil.h"
#include "rt_reflect_probe.dxil.h"           // I-0129 R9a: rt_reflect_probe_dxil[]
#include "rr_guides_vs.dxil.h"       // I-0123 RR-a: rr_guides_vs_dxil[], rr_guides_guide_dxil[]
#include "rr_guides_guide.dxil.h"
#include <cmath>
#include <algorithm>
#include <bit>
#include <cstring>
#include <stdexcept>

namespace ds3native {
using Microsoft::WRL::ComPtr;

bool RtSliceEligible(const MeshSlice& s) noexcept {
    const bool wide=s.index_width==4;
    return s.vertex_count>0 && s.index_count>=3 && s.index_count%3==0 && s.stride>=12 && s.stride%4==0 && s.vertex_offset%4==0 &&
        (s.index_width==2 || wide) && s.index_offset%s.index_width==0;
}

D3D12_RAYTRACING_GEOMETRY_DESC RtSliceGeometry(D3D12_GPU_VIRTUAL_ADDRESS base,const MeshSlice& s) noexcept {
    D3D12_RAYTRACING_GEOMETRY_DESC g{D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES,D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE};
    g.Triangles.VertexFormat=DXGI_FORMAT_R32G32B32_FLOAT;g.Triangles.VertexCount=s.vertex_count;
    g.Triangles.VertexBuffer={base+s.vertex_offset,s.stride};
    g.Triangles.IndexFormat=s.index_width==4?DXGI_FORMAT_R32_UINT:DXGI_FORMAT_R16_UINT;g.Triangles.IndexCount=s.index_count;
    g.Triangles.IndexBuffer=base+s.index_offset;
    return g;
}

namespace {
D3D12_HEAP_PROPERTIES HeapOf(D3D12_HEAP_TYPE type) noexcept {D3D12_HEAP_PROPERTIES h{};h.Type=type;return h;}
D3D12_RESOURCE_DESC BufferDesc(uint64_t bytes,D3D12_RESOURCE_FLAGS flags) noexcept {
    D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=bytes;d.Height=1;d.DepthOrArraySize=d.MipLevels=1;
    d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;d.Flags=flags;return d;
}
void Check(HRESULT hr,const char* what){if(FAILED(hr))throw std::runtime_error(what);}
// A structure in the pooled buffer heaps (DeviceQueues::CreateStreamed), in the acceleration-structure state.
ComPtr<ID3D12Resource> Structure(DeviceQueues& gpu,uint64_t bytes) {
    return gpu.CreateStreamed(BufferDesc(RtAlign(bytes),D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,PlacedKind::Buffer);
}
// Released with the list that carried a step, once its submission completed: the step's state advances then. A list never
// submitted (its recording dropped without a commit) fails the model instead.
// `done` (I-0120 F1), when given, is set then too, whether or not the model still lives.
std::shared_ptr<void> CompletionToken(const std::shared_ptr<RtModelBlas>& blas,RtModelBlas::State from,RtModelBlas::State to,std::shared_ptr<std::atomic<bool>> committed,
    std::shared_ptr<std::atomic<bool>> done={}) {
    // A real object (IngressRecording::Keep drops a null owner at once).
    struct Token {std::weak_ptr<RtModelBlas> weak;RtModelBlas::State from,to;std::shared_ptr<std::atomic<bool>> committed,done;
        ~Token(){if(const auto b=weak.lock()){auto expected=from;b->state.compare_exchange_strong(expected,committed->load()?to:RtModelBlas::State::failed);}
            if(done)done->store(true,std::memory_order_release);}};
    auto token=std::make_shared<Token>();token->weak=blas;token->from=from;token->to=to;token->committed=std::move(committed);token->done=std::move(done);return token;   // never a copied Token (its destructor would fire)
}
}

void RtModelBlas::SetStructure(ComPtr<ID3D12Resource> structure,uint64_t placed) {
    const std::lock_guard hold(structure_lock_);
    if(live_){if(!placed_)live_->models.fetch_add(1,std::memory_order_relaxed);live_->placed_bytes.fetch_add(placed-placed_,std::memory_order_relaxed);}
    structure_=std::move(structure);placed_=placed;
}
RtModelBlas::~RtModelBlas() {
    if(live_ && placed_){live_->models.fetch_sub(1,std::memory_order_relaxed);live_->placed_bytes.fetch_sub(placed_,std::memory_order_relaxed);}
}

// The compacted sizes: one UAV buffer the builds write (each into its model's slot) and one persistently mapped readback copy.
// The UAV buffer starts each list in COMMON (a buffer is created there and decays back at the end of each submission): the build's
// write promotes it to UNORDERED_ACCESS, the list moves it to COPY_SOURCE and back, so lists recorded on any thread agree.
struct RtBlasRegistry::Postbuild {
    ComPtr<ID3D12Resource> sizes,readback;const uint64_t* mapped{};
    std::vector<uint32_t> free;
};

std::shared_ptr<RtModelBlas> RtBlasRegistry::Build(DeviceQueues& gpu,ID3D12Resource* buffer,std::span<const MeshSlice> meshes) {
    auto* device=gpu.device5.Get();
    if(!device || !buffer)throw std::runtime_error("rt blas device");
    auto blas=std::make_shared<RtModelBlas>();
    std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometries;geometries.reserve(meshes.size());
    const auto base=buffer->GetGPUVirtualAddress();
    for(const auto& slice:meshes){
        // Every slice keeps its index (GeometryIndex() = the slice): an ineligible one is a degenerate geometry with no triangle.
        if(RtSliceEligible(slice)){geometries.push_back(RtSliceGeometry(base,slice));blas->triangles+=slice.index_count/3;}
        else{auto g=RtSliceGeometry(base,MeshSlice{});g.Triangles.VertexCount=0;g.Triangles.IndexCount=0;g.Triangles.VertexBuffer={0,12};
            g.Triangles.IndexBuffer=0;g.Triangles.IndexFormat=DXGI_FORMAT_UNKNOWN;geometries.push_back(g);++blas->skipped;}}
    if(geometries.empty() || blas->skipped==geometries.size())return nullptr;
    blas->geometries=static_cast<uint32_t>(geometries.size());
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL,
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE|D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION,
        blas->geometries,D3D12_ELEMENTS_LAYOUT_ARRAY};
    inputs.pGeometryDescs=geometries.data();
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO sizes{};device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs,&sizes);
    if(!sizes.ResultDataMaxSizeInBytes)throw std::runtime_error("rt blas prebuild");
    auto structure=Structure(gpu,sizes.ResultDataMaxSizeInBytes);blas->built_bytes=RtAlign(sizes.ResultDataMaxSizeInBytes);
    // Placed in the pooled buffer heaps, not committed (step review L1: a committed creation waits on the paging fence, I-0079);
    // its range goes back once the list that holds it completed. A buffer starts in COMMON; the build promotes it.
    auto scratch=gpu.CreateStreamed(BufferDesc(RtAlign((std::max)(sizes.ScratchDataSizeInBytes,uint64_t(256))),D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
        D3D12_RESOURCE_STATE_COMMON,PlacedKind::Buffer);
    std::shared_ptr<Postbuild> postbuild;
    {const std::lock_guard hold(lock_);
        if(!postbuild_){auto p=std::make_shared<Postbuild>();
            {const auto heap=HeapOf(D3D12_HEAP_TYPE_DEFAULT);const auto desc=BufferDesc(8ull*kPostbuildSlots,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
                Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&p->sizes)),"rt postbuild sizes");}
            {const auto heap=HeapOf(D3D12_HEAP_TYPE_READBACK);const auto desc=BufferDesc(8ull*kPostbuildSlots,D3D12_RESOURCE_FLAG_NONE);
                Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&p->readback)),"rt postbuild readback");}
            void* mapped{};Check(p->readback->Map(0,nullptr,&mapped),"rt postbuild map");p->mapped=static_cast<const uint64_t*>(mapped);
            p->free.reserve(kPostbuildSlots);for(uint32_t i=kPostbuildSlots;i-->0;)p->free.push_back(i);
            postbuild_=std::move(p);}
        postbuild=postbuild_;
        if(!postbuild->free.empty()){blas->postbuild_slot=postbuild->free.back();postbuild->free.pop_back();}}
    // Step review M2: a failure before the list is committed gives its slot back (after a commit the pending entry owns it).
    struct SlotBack {RtBlasRegistry* self;Postbuild* p;uint32_t slot;
        ~SlotBack(){if(slot!=UINT32_MAX){const std::lock_guard hold(self->lock_);p->free.push_back(slot);}}} slot_back{this,postbuild.get(),blas->postbuild_slot};
    auto recording=gpu.RecordIngress();
    ComPtr<ID3D12GraphicsCommandList4> list;Check(recording.List()->QueryInterface(IID_PPV_ARGS(&list)),"rt blas list4");
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build{};build.Inputs=inputs;
    build.DestAccelerationStructureData=structure->GetGPUVirtualAddress();build.ScratchAccelerationStructureData=scratch->GetGPUVirtualAddress();
    const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC info{postbuild->sizes->GetGPUVirtualAddress()+8ull*blas->postbuild_slot,
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE};
    const bool compact=blas->postbuild_slot!=UINT32_MAX;   // no slot left: the model keeps its built structure (counted ready at once)
    list->BuildRaytracingAccelerationStructure(&build,compact?1:0,compact?&info:nullptr);
    if(compact){
        D3D12_RESOURCE_BARRIER to_copy{};to_copy.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        to_copy.Transition={postbuild->sizes.Get(),0,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&to_copy);
        list->CopyBufferRegion(postbuild->readback.Get(),8ull*blas->postbuild_slot,postbuild->sizes.Get(),8ull*blas->postbuild_slot,8);
        std::swap(to_copy.Transition.StateBefore,to_copy.Transition.StateAfter);list->ResourceBarrier(1,&to_copy);}
    auto committed=std::make_shared<std::atomic<bool>>(false);
    auto built=std::make_shared<std::atomic<bool>>(false);
    recording.Keep(CompletionToken(blas,RtModelBlas::State::building,compact?RtModelBlas::State::built:RtModelBlas::State::ready,committed,built));
    recording.Keep(std::make_shared<ComPtr<ID3D12Resource>>(std::move(scratch)));
    recording.Keep(std::make_shared<ComPtr<ID3D12Resource>>(buffer));   // the source, read by the build
    // The destination too: a storage retired before this list is submitted retires against an earlier ticket (step review).
    recording.Keep(std::make_shared<ComPtr<ID3D12Resource>>(structure));
    recording.Keep(postbuild);
    committed->store(true);try{(void)recording.Commit();}catch(...){committed->store(false);throw;}   // stored first: its completion may come before this thread runs on
    slot_back.slot=UINT32_MAX;
    blas->live_=live_;blas->SetStructure(std::move(structure),RtPlaced(blas->built_bytes));   // counted alive once its list is committed
    {const std::lock_guard hold(lock_);pending_.push_back({blas,blas->postbuild_slot,std::move(built)});++counters_.models;counters_.geometries+=blas->geometries;counters_.skipped+=blas->skipped;
        counters_.triangles+=blas->triangles;counters_.built_bytes+=blas->built_bytes;}
    return blas;
}

size_t RtBlasRegistry::Advance(DeviceQueues& gpu,size_t budget) {
    std::vector<std::shared_ptr<RtModelBlas>> built;size_t advanced{};std::shared_ptr<Postbuild> postbuild;
    {const std::lock_guard hold(lock_);postbuild=postbuild_;
        // One pass (step review L2): the ready and failed models leave with their slot, and so do the slots of models destroyed
        // before they were reaped, once their build list completed (I-0120 F1).
        std::erase_if(pending_,[&](const Pending& entry){const auto b=entry.blas.lock();
            // I-0120 F1: a model destroyed before its build list completed keeps its slot until then (a build reusing it in the same
            // submission could read this one's compacted size).
            if(!b && entry.built && !entry.built->load(std::memory_order_acquire))return false;
            const auto state=b?b->state.load():RtModelBlas::State::failed;
            if(b && state!=RtModelBlas::State::ready && state!=RtModelBlas::State::failed){if(state==RtModelBlas::State::built && built.size()<budget)built.push_back(b);return false;}
            if(b){if(state==RtModelBlas::State::ready){++counters_.ready;counters_.compacted_bytes+=b->compacted_bytes?b->compacted_bytes:b->built_bytes;}else ++counters_.failed;
                b->postbuild_slot=UINT32_MAX;++advanced;}
            if(entry.slot!=UINT32_MAX && postbuild)postbuild->free.push_back(entry.slot);
            return true;});}
    if(built.empty() || !postbuild)return advanced;
    // Every compacted structure placed before anything is recorded (step review L3): one that cannot be placed leaves its model
    // built, retried at a later Advance, and the others go on.
    std::vector<std::pair<std::shared_ptr<RtModelBlas>,ComPtr<ID3D12Resource>>> copies;copies.reserve(built.size());
    for(const auto& b:built){
        const uint64_t size=postbuild->mapped[b->postbuild_slot];
        if(!size || size>b->built_bytes){b->state.store(RtModelBlas::State::ready);continue;}   // nothing to gain, or a size not trusted: kept as built
        try{copies.emplace_back(b,Structure(gpu,size));}catch(const std::exception&){break;}}
    if(copies.empty())return advanced;
    auto recording=gpu.RecordIngress();
    ComPtr<ID3D12GraphicsCommandList4> list;Check(recording.List()->QueryInterface(IID_PPV_ARGS(&list)),"rt compact list4");
    auto committed=std::make_shared<std::atomic<bool>>(false);
    for(auto& [b,compacted]:copies){
        auto large=b->Structure();
        list->CopyRaytracingAccelerationStructure(compacted->GetGPUVirtualAddress(),large->GetGPUVirtualAddress(),D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT);
        recording.Keep(std::make_shared<ComPtr<ID3D12Resource>>(std::move(large)));   // the large one, until the copy completed
        recording.Keep(std::make_shared<ComPtr<ID3D12Resource>>(compacted));      // the copy's destination, as the build's
        recording.Keep(CompletionToken(b,RtModelBlas::State::compacting,RtModelBlas::State::ready,committed));
        b->state.store(RtModelBlas::State::compacting);++advanced;}
    committed->store(true);try{(void)recording.Commit();}catch(...){committed->store(false);throw;}   // stored first: its completion may come before this thread runs on
    // Swapped only once the copies are committed (T1 step review M1): a frame list sent after this point runs after them on the
    // queue; one sent before still reads the built structure, which the copy list keeps until it completed.
    for(auto& [b,compacted]:copies){b->compacted_bytes=RtAlign(postbuild->mapped[b->postbuild_slot]);b->SetStructure(std::move(compacted),RtPlaced(b->compacted_bytes));}
    return advanced;
}

RtBlasCounters RtBlasRegistry::Counters() const {
    const std::lock_guard hold(lock_);auto c=counters_;c.pending=pending_.size();c.slots_free=postbuild_?postbuild_->free.size():kPostbuildSlots;
    c.live_models=live_->models.load(std::memory_order_relaxed);c.live_placed_bytes=live_->placed_bytes.load(std::memory_order_relaxed);return c;
}

void RtBlasRegistry::Close() noexcept {
    const std::lock_guard hold(lock_);pending_.clear();
    if(postbuild_ && postbuild_->readback)postbuild_->readback->Unmap(0,nullptr);
    postbuild_.reset();
}

bool RtSliceCasts(uint32_t cls,bool surface,bool decal,bool no_cast,bool water) noexcept {
    return !decal && !no_cast && !water && (surface || (cls&kRtTransparentClass));
}
uint8_t RtSliceKeep(bool surface,bool casts,bool water) noexcept {
    const uint8_t own=water?RtMaskWater:0;
    if(!surface)return static_cast<uint8_t>(RtMaskForward|RtMaskReflection|own|(casts?RtMaskSun|RtMaskLocal:0));
    return static_cast<uint8_t>((0xFF&~RtMaskForward&~RtMaskWater&~(casts?0:RtMaskSun|RtMaskLocal))|own);
}
uint8_t RtInstanceMask(uint8_t engine_bits,const RtSurfaceClasses& surface) noexcept {
    uint8_t bits=engine_bits&static_cast<uint8_t>(~RtMaskOther);
    if(surface.known){
        if(bits&RtMaskView){if(!surface.surfaces)bits=static_cast<uint8_t>((bits&~RtMaskView)|RtMaskForward);else if(surface.overlays)bits|=RtMaskForward;}
        if(!surface.casts)bits&=static_cast<uint8_t>(~(RtMaskSun|RtMaskLocal));
        if(surface.water && (bits&(RtMaskView|RtMaskForward)))bits|=RtMaskWater;}   // I-0125 R6c: a water piece the eye sees (step review L1)
    return bits?bits:static_cast<uint8_t>(RtMaskOther);
}

void RtFillInstanceDescs(std::span<const RtInstance> instances,D3D12_RAYTRACING_INSTANCE_DESC* out,std::span<const uint32_t> firsts) noexcept {
    for(size_t k=0;k<instances.size();++k){const auto& instance=instances[k];
        D3D12_RAYTRACING_INSTANCE_DESC d{};std::memcpy(d.Transform,instance.transform.data(),sizeof d.Transform);
        d.InstanceID=instance.id&0xFFFFFFu;d.InstanceMask=instance.mask;d.AccelerationStructure=instance.blas?instance.blas->GetGPUVirtualAddress():0;
        if(k<firsts.size()){d.InstanceContributionToHitGroupIndex=firsts[k]&0xFFFFFFu;   // R3a: its first record (T3: an alpha-tested one non-opaque)
            if(instance.materials && instance.materials->non_opaque)d.Flags=D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE;}
        *out++=d;}
}

RtSkinRows RtSkinRowsOf(std::span<const uint8_t> bytes,std::span<const RtSkinSlice> slices) {
    // One bit a row a role (T3 step review L4: no sort of every vertex's indices), then the rows in order.
    RtSkinRows out;std::array<std::vector<uint64_t>,3> seen;
    for(const auto& s:slices){if(s.mode!=1 && s.mode!=2)continue;
        const size_t role=s.palette?1:0;const uint64_t width=s.index_format?8:4;
        const uint64_t end=uint64_t(s.slice.vertex_offset)+uint64_t(s.slice.vertex_count)*s.slice.stride;
        // T5: the indices in the second stream (a blended slice's blend_stream).
        const bool second=s.mode==2 && s.blend_stream;const uint32_t base=second?s.slice.vertex_offset2:s.slice.vertex_offset,stride=second?s.slice.stride2:s.slice.stride;
        const uint64_t last=uint64_t(base)+uint64_t(s.slice.vertex_count)*stride;
        if(s.source || s.index_format>1 || uint64_t(s.index_offset)+width>stride || end>bytes.size() || last>bytes.size()){out.all[role]=true;if(s.cancel)out.all[2]=true;continue;}
        for(const size_t r:{role,size_t(2)}){if(r==2 && !s.cancel)continue;if(seen[r].empty())seen[r].assign(1024,0);}   // 65,536 rows: R16 indices
        const auto mark=[&](uint32_t index){seen[role][index>>6]|=uint64_t(1)<<(index&63);if(s.cancel)seen[2][index>>6]|=uint64_t(1)<<(index&63);};
        for(uint32_t v=0;v<s.slice.vertex_count;++v){const uint8_t* at=bytes.data()+base+uint64_t(v)*stride+s.index_offset;
            std::array<uint32_t,4> index{};
            if(!s.index_format)index={at[0],at[1],at[2],at[3]};
            else{uint16_t w[4];std::memcpy(w,at,8);index={w[0],w[1],w[2],w[3]};}
            if(s.mode==1)mark(index[3]);else for(const uint32_t i:index)mark(i);}}
    for(size_t r=0;r<3;++r)for(size_t word=0;word<seen[r].size();++word)for(uint64_t bits=seen[r][word];bits;bits&=bits-1)
        out.rows[r].push_back(static_cast<uint32_t>(64*word+static_cast<size_t>(std::countr_zero(bits))));
    return out;
}

uint64_t RtSkinRecords(std::span<const RtSkinSlice> slices,std::span<const uint64_t> source_bytes,std::vector<RtSkinRecord>& out) {
    out.assign(slices.size(),RtSkinRecord{});uint64_t total{};
    static constexpr uint32_t index_bytes[2]{4,8},weight_bytes[4]{4,8,8,16};
    for(size_t i=0;i<slices.size();++i){const auto& s=slices[i];auto& r=out[i];r.first=static_cast<uint32_t>(total);
        if(!s.mode || s.mode>3 || !RtSliceEligible(s.slice) || s.index_format>1 || (s.mode==2 && s.weight_format>3) || s.palette>1 ||
           s.source>=source_bytes.size() || source_bytes.empty())continue;
        const auto& m=s.slice;const uint32_t from=s.source?s.source_offset:m.vertex_offset;
        // T5: blend elements in the second stream: the storage's (a physics stream rewrites slot 0 only, so none with it).
        const bool second=s.mode==2 && s.blend_stream;const uint32_t blend_stride=second?m.stride2:m.stride;
        if(second && (s.source || !m.stride2 || m.stride2%4 || m.vertex_offset2%4 || uint64_t(m.vertex_offset2)+uint64_t(m.vertex_count)*m.stride2>source_bytes[0]))continue;
        const bool fits=from%4==0 && uint64_t(from)+uint64_t(m.vertex_count)*m.stride<=source_bytes[s.source] &&
            uint64_t(m.index_offset)+uint64_t(m.index_count)*m.index_width<=source_bytes[0] &&
            (s.mode==3 || (s.index_offset%4==0 && s.index_offset+index_bytes[s.index_format]<=(s.mode==2?blend_stride:m.stride))) &&
            (s.mode!=2 || (s.weight_offset%4==0 && s.weight_offset+weight_bytes[s.weight_format]<=blend_stride));
        // A slice's vertices within a dispatch's 65,535 groups of 64 (T2 step review L4); 64 M vertices an instance at most.
        if(!fits || m.vertex_count>65535u*64u || total+m.vertex_count>(1ull<<26))continue;
        // T6: TEXCOORD1 in the vertex (12 bytes as float3, 8 as half4, 4 as UNORM8), else the rest pose.
        const uint32_t wind_bytes=s.wind==RtWindFloat3?12u:s.wind==RtWindHalf4?8u:(s.wind==RtWindUnorm8 || s.wind==RtWindBgra8)?4u:0u;
        const bool wind=wind_bytes && s.wind_offset%4==0 && uint64_t(s.wind_offset)+wind_bytes<=m.stride;
        r={from,m.stride,m.vertex_count,static_cast<uint32_t>(total),s.mode,s.index_offset,s.index_format,s.weight_offset,s.weight_format,s.palette,s.cancel,s.source,
            second?m.vertex_offset2:0u,second?m.stride2:0u,wind?s.wind:0u,wind?s.wind_offset:0u};
        total+=m.vertex_count;}
    return total;
}

namespace {
// A skinned instance's geometries: its slices' positions (float3, 12 bytes) and the storage's indices; the excluded slices empty.
std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> SkinnedGeometries(const RtSkinnedInstance& item,std::span<const RtSkinRecord> records,D3D12_GPU_VIRTUAL_ADDRESS positions) {
    std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometries;geometries.reserve(records.size());const auto base=item.source->GetGPUVirtualAddress();
    for(size_t i=0;i<records.size();++i){const auto& s=(*item.slices)[i].slice;
        D3D12_RAYTRACING_GEOMETRY_DESC g{D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES,D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE};
        g.Triangles.VertexFormat=DXGI_FORMAT_R32G32B32_FLOAT;g.Triangles.VertexBuffer={0,12};
        if(records[i].vertices){g.Triangles.VertexCount=records[i].vertices;g.Triangles.VertexBuffer={positions+12ull*records[i].first,12};
            g.Triangles.IndexFormat=s.index_width==4?DXGI_FORMAT_R32_UINT:DXGI_FORMAT_R16_UINT;g.Triangles.IndexCount=s.index_count;g.Triangles.IndexBuffer=base+s.index_offset;}
        geometries.push_back(g);}
    return geometries;
}
}

void RtSkinner::Encode(DeviceQueues& gpu,ID3D12GraphicsCommandList4* list,const Upload& upload,std::span<const RtSkinnedInstance> skinned,
    std::vector<RtInstance>& out,RtSkinCounters& counters) {
    if(skinned.empty())return;
    auto* device=gpu.device5.Get();if(!device || !list)throw std::runtime_error("rt skin device");
    if(!pipeline_){
        Check(device->CreateRootSignature(0,rt_skin_dxil,sizeof rt_skin_dxil,IID_PPV_ARGS(&root_)),"rt skin root");
        D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root_.Get();pd.CS={rt_skin_dxil,sizeof rt_skin_dxil};
        Check(device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pipeline_)),"rt skin pipeline");}
    const auto instance=[&](const RtSkinnedInstance& item){RtInstance i;i.transform={1,0,0,0, 0,1,0,0, 0,0,1,0};i.blas=item.blas->structure;i.id=item.id;i.mask=item.mask;
        i.materials=item.materials;i.scroll=item.scroll;i.skin=item.skin;i.ambient=item.ambient;   // T3: the storage's UVs and indices, as its slices'
        i.positions=item.blas->positions;i.bases=item.blas->bases;if(item.framed)i.frame=item.world;   // R3a: its world positions; T6: its frame
        out.push_back(std::move(i));};
    // T6 (the final review's M1): a placed model moved by the wind that cannot be skinned this frame is traced in its rest pose, the
    // storage's BLAS at its placement, as before T6, rather than left out of the rays.
    const auto rest=[&](const RtSkinnedInstance& item){if(!item.rest)return;RtInstance i;i.transform=item.world;i.blas=item.rest;i.id=item.id;i.mask=item.mask;
        i.materials=item.materials;i.scroll=item.scroll;i.skin=item.skin;i.ambient=item.ambient;out.push_back(std::move(i));};
    struct Work {const RtSkinnedInstance* item;std::vector<RtSkinRecord> records;uint32_t widest{};bool rebuild{};uint64_t scratch_offset{};};
    std::vector<Work> work;uint64_t scratch_need{};
    for(const auto& item:skinned){
        if(!item.blas || !item.source || !item.slices || item.blas->failed){++counters.failed;rest(item);continue;}
        auto& b=*item.blas;++counters.instances;counters.bytes+=b.positions_bytes+b.structure_bytes;
        if(b.retry){--b.retry;++counters.failed;rest(item);continue;}   // a placement refused lately: tried again later
        if(b.built && (item.hold || b.generation==item.generation)){instance(item);continue;}   // its pose unchanged since it was written, or held
        if(item.hold){++counters.failed;continue;}   // never written and its rows not finite: left out
        Work w{&item};std::vector<uint64_t> bytes{item.source->GetDesc().Width};
        for(const auto& s:item.sources)bytes.push_back(s?s->GetDesc().Width:0);
        const uint64_t vertices=RtSkinRecords(*item.slices,bytes,w.records);
        // T6: only the palette's slices need its rows (a placed instance moved by the wind has none, its slices by the world row).
        const bool posed=std::any_of(w.records.begin(),w.records.end(),[](const RtSkinRecord& r){return r.vertices && r.mode!=3;});
        if(!vertices){b.failed=true;++counters.failed;rest(item);continue;}
        // I-0120 F3: an empty palette is a frame's state, not the model's: this frame keeps the last positions (none yet: left out).
        if(posed && (!item.palettes[0] || item.palettes[0]->empty())){if(b.built)instance(item);else{++counters.failed;rest(item);}continue;}
        if(!item.wind)for(auto& r:w.records)r.wind=r.wind_offset=0;   // T6: no block, no wind read
        // A palette the collect did not bind is never read: the auxiliary route falls back to the current rows, no cancel applied.
        for(auto& r:w.records){if(r.palette==1 && (!item.palettes[1] || item.palettes[1]->empty()))r.palette=0;
            if(r.cancel && (!item.palettes[2] || item.palettes[2]->empty()))r.cancel=0;}
        for(const auto& r:w.records)w.widest=(std::max)(w.widest,r.vertices);
        // A slice whose source changed size since the first build (a stream's) would change the geometries a refit must keep: this
        // frame keeps the instance's last positions (none yet: left out).
        if(b.positions && (b.vertices.size()!=w.records.size() || !std::equal(b.vertices.begin(),b.vertices.end(),w.records.begin(),
            [](uint32_t v,const RtSkinRecord& r){return v==r.vertices;}))){if(b.built)instance(item);else{++counters.failed;rest(item);}continue;}
        if(!b.positions)try{   // first written: its positions and its structure, sized once (the collect makes a new one if its slices change)
            b.positions=gpu.CreateStreamed(BufferDesc(RtAlign(vertices*12),D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),D3D12_RESOURCE_STATE_COMMON,PlacedKind::Buffer);
            b.positions_bytes=RtAlign(vertices*12);b.vertices.clear();for(const auto& r:w.records)b.vertices.push_back(r.vertices);
            {auto bases=std::make_shared<std::vector<uint32_t>>();for(const auto& r:w.records)bases->push_back(r.first);b.bases=std::move(bases);}
            const auto geometries=SkinnedGeometries(item,w.records,b.positions->GetGPUVirtualAddress());
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL,
                D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE|D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,
                static_cast<UINT>(geometries.size()),D3D12_ELEMENTS_LAYOUT_ARRAY};
            inputs.pGeometryDescs=geometries.data();
            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO sizes{};device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs,&sizes);
            if(!sizes.ResultDataMaxSizeInBytes)throw std::runtime_error("rt skin prebuild");
            b.structure=Structure(gpu,sizes.ResultDataMaxSizeInBytes);b.structure_bytes=RtAlign(sizes.ResultDataMaxSizeInBytes);
            b.build_scratch=RtAlign((std::max)(sizes.ScratchDataSizeInBytes,uint64_t(256)));b.update_scratch=RtAlign((std::max)(sizes.UpdateScratchDataSizeInBytes,uint64_t(256)));
        }catch(const std::exception&){b.positions.Reset();b.structure.Reset();b.retry=120;++counters.failed;rest(item);continue;}   // a placement refused: left out 120 frames (T6: or in its rest pose), the others go on
        w.rebuild=!b.built || b.updates>=kRebuildAfter;w.scratch_offset=scratch_need;scratch_need+=w.rebuild?b.build_scratch:b.update_scratch;
        work.push_back(std::move(w));}
    if(work.empty())return;
    // One scratch for the frame's builds, each its own range (they may overlap on the GPU); grown by half again, the former kept until
    // the list being recorded completed (as the top level's).
    if(!scratch_ || scratch_bytes_<scratch_need){
        const uint64_t size=RtAlign((std::max)(scratch_need,scratch_bytes_+scratch_bytes_/2));
        if(scratch_)RetireAfterCurrentRecording(std::make_shared<ComPtr<ID3D12Resource>>(std::move(scratch_)));
        const auto heap=HeapOf(D3D12_HEAP_TYPE_DEFAULT);const auto desc=BufferDesc(size,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&scratch_)),"rt skin scratch");scratch_bytes_=size;}
    // The previous frame's builds on this scratch and their reads of the positions done before these writes.
    D3D12_RESOURCE_BARRIER before{};before.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;list->ResourceBarrier(1,&before);
    list->SetComputeRootSignature(root_.Get());list->SetPipelineState(pipeline_.Get());
    std::vector<D3D12_RESOURCE_BARRIER> written;written.reserve(work.size());
    for(const auto& w:work){const auto& item=*w.item;
        const auto put=[&](const void* bytes,uint64_t size){const auto range=upload(size);std::memcpy(range.cpu,bytes,size);return range.gpu;};
        // The records, then the instance's world row (rt_skin.hlsl reads it after the last record, for its mode-3 slices), then T6
        // its wind block (read by the slices with a wind).
        const size_t record_bytes=w.records.size()*sizeof(RtSkinRecord),wind_bytes=item.wind?item.wind->size():0;
        std::vector<uint8_t> table(record_bytes+sizeof item.world+wind_bytes);std::memcpy(table.data(),w.records.data(),record_bytes);
        std::memcpy(table.data()+record_bytes,item.world.data(),sizeof item.world);
        if(item.wind)std::memcpy(table.data()+record_bytes+sizeof item.world,item.wind->data(),wind_bytes);
        const auto records=put(table.data(),table.size());
        std::array<D3D12_GPU_VIRTUAL_ADDRESS,3> palettes{};uint32_t rows[5]{0,0,0,static_cast<uint32_t>(w.records.size()),0};
        for(size_t p=0;p<3;++p)if(item.palettes[p] && !item.palettes[p]->empty()){palettes[p]=put(item.palettes[p]->data(),item.palettes[p]->size()*sizeof(std::array<float,12>));rows[p]=static_cast<uint32_t>(item.palettes[p]->size());}
        list->SetComputeRootShaderResourceView(2,records);
        // A palette not bound reads the current rows' address with no row (its count 0: every index reads a zero row); with no
        // current rows (T6: a placed instance) the records' address, never read.
        for(size_t p=0;p<3;++p)list->SetComputeRootShaderResourceView(3+static_cast<UINT>(p),palettes[p]?palettes[p]:palettes[0]?palettes[0]:records);
        list->SetComputeRootUnorderedAccessView(6,item.blas->positions->GetGPUVirtualAddress());
        // One dispatch a source its slices read (the storage, each physics stream): the others' rows return at once.
        for(uint32_t source=0;source<=item.sources.size();++source){
            if(std::none_of(w.records.begin(),w.records.end(),[&](const RtSkinRecord& r){return r.vertices && r.source==source;}))continue;
            rows[4]=source;list->SetComputeRoot32BitConstants(0,5,rows,0);
            list->SetComputeRootShaderResourceView(1,(source?item.sources[source-1]:item.source)->GetGPUVirtualAddress());
            list->Dispatch((w.widest+63)/64,static_cast<UINT>(w.records.size()),1);}
        D3D12_RESOURCE_BARRIER t{};t.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;   // promoted to UNORDERED_ACCESS by the dispatch (a buffer starts each submission in COMMON)
        t.Transition={item.blas->positions.Get(),0,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE};written.push_back(t);
        bool winds{};for(const auto& r:w.records){counters.vertices+=r.vertices;if(r.wind){counters.wind_vertices+=r.vertices;winds=true;}}
        counters.winds+=winds;}
    list->ResourceBarrier(static_cast<UINT>(written.size()),written.data());
    for(const auto& w:work){const auto& item=*w.item;auto& b=*item.blas;
        const auto geometries=SkinnedGeometries(item,w.records,b.positions->GetGPUVirtualAddress());
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build{};
        build.Inputs={D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL,D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE|
            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE|(w.rebuild?D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE:D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE),
            static_cast<UINT>(geometries.size()),D3D12_ELEMENTS_LAYOUT_ARRAY};
        build.Inputs.pGeometryDescs=geometries.data();
        build.DestAccelerationStructureData=b.structure->GetGPUVirtualAddress();
        build.SourceAccelerationStructureData=w.rebuild?0:b.structure->GetGPUVirtualAddress();   // a refit in place
        build.ScratchAccelerationStructureData=scratch_->GetGPUVirtualAddress()+w.scratch_offset;
        list->BuildRaytracingAccelerationStructure(&build,0,nullptr);
        if(w.rebuild){++counters.builds;b.updates=0;}else{++counters.refits;++b.updates;}
        b.built=true;b.generation=item.generation;instance(item);}
}

D3D12_GPU_VIRTUAL_ADDRESS RtTlas::Build(ID3D12Device5* device,ID3D12GraphicsCommandList4* list,D3D12_GPU_VIRTUAL_ADDRESS descs,uint32_t count) {
    if(!count)return 0;
    if(!device || !list || !descs || descs%D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT)throw std::invalid_argument("rt tlas input");
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD,count,D3D12_ELEMENTS_LAYOUT_ARRAY};
    inputs.InstanceDescs=descs;
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO sizes{};device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs,&sizes);
    if(!sizes.ResultDataMaxSizeInBytes)throw std::runtime_error("rt tlas prebuild");
    // Grown by half again (rare: a zone's first frames), the former kept until the list being recorded completed.
    const auto grow=[&](ComPtr<ID3D12Resource>& buffer,uint64_t& bytes,uint64_t need,D3D12_RESOURCE_STATES state,const char* what){
        if(buffer && bytes>=need)return;
        const uint64_t size=RtAlign((std::max)(need,bytes+bytes/2));
        if(buffer)RetireAfterCurrentRecording(std::make_shared<ComPtr<ID3D12Resource>>(std::move(buffer)));
        const auto heap=HeapOf(D3D12_HEAP_TYPE_DEFAULT);const auto desc=BufferDesc(size,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&buffer)),what);bytes=size;};
    grow(structure_,structure_bytes_,sizes.ResultDataMaxSizeInBytes,D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,"rt tlas structure");
    grow(scratch_,scratch_bytes_,(std::max)(sizes.ScratchDataSizeInBytes,uint64_t(256)),D3D12_RESOURCE_STATE_COMMON,"rt tlas scratch");   // promoted by the build
    // A null UAV barrier first (T1 final review M1): the BLAS builds and compaction copies on the ingress lists go in the same
    // ExecuteCommandLists call as this list, where lists may overlap; their writes complete before this build reads them.
    D3D12_RESOURCE_BARRIER written{};written.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;list->ResourceBarrier(1,&written);
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build{};build.Inputs=inputs;
    build.DestAccelerationStructureData=structure_->GetGPUVirtualAddress();build.ScratchAccelerationStructureData=scratch_->GetGPUVirtualAddress();
    list->BuildRaytracingAccelerationStructure(&build,0,nullptr);
    // A null UAV barrier: the structure readable, and the scratch free for a later build on the same list (T1 step review L1).
    D3D12_RESOURCE_BARRIER done{};done.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;list->ResourceBarrier(1,&done);
    return structure_->GetGPUVirtualAddress();
}

namespace {
bool SameSlice(const RtMaterialSlice& x,const RtMaterialSlice& y) noexcept {
    for(uint32_t k=0;k<RtTextureRoles;++k)if(x.views[k].ptr!=y.views[k].ptr || x.textures[k]!=y.textures[k] || x.tiling[k]!=y.tiling[k])return false;
    if(x.index_offset!=y.index_offset || x.index_width!=y.index_width || x.position_offset!=y.position_offset || x.position_stride!=y.position_stride ||
       x.uv_offset!=y.uv_offset || x.uv_stride!=y.uv_stride || x.uv_format!=y.uv_format || x.normal_offset!=y.normal_offset || x.normal_stride!=y.normal_stride ||
       x.tangent_offset!=y.tangent_offset || x.tangent_stride!=y.tangent_stride || x.albedo!=y.albedo || x.emissive!=y.emissive || x.material!=y.material ||
       x.alpha!=y.alpha || x.alpha_texture!=y.alpha_texture || x.ref!=y.ref || x.colour_offset!=y.colour_offset || x.colour_stride!=y.colour_stride ||
       x.colour_format!=y.colour_format || x.keep!=y.keep || x.family!=y.family || x.unscrolled!=y.unscrolled || x.normal_half!=y.normal_half ||
       x.tangent_half!=y.tangent_half || x.mul!=y.mul || x.mul_mask!=y.mul_mask || x.skin!=y.skin || x.hair!=y.hair || x.face!=y.face || x.face_both!=y.face_both || x.wind!=y.wind || x.wind_offset!=y.wind_offset ||
       x.ambient!=y.ambient || x.lightmap_offset!=y.lightmap_offset || x.lightmap_stride!=y.lightmap_stride || x.lightmap_format!=y.lightmap_format || x.layers.size()!=y.layers.size() ||
       x.forward!=y.forward || x.forward_blend!=y.forward_blend || x.forward_colour_offset!=y.forward_colour_offset ||
       x.forward_colour_stride!=y.forward_colour_stride || x.forward_colour_format!=y.forward_colour_format || x.forward_kind!=y.forward_kind)return false;   // I-0125 R6d
    for(size_t k=0;k<x.layers.size();++k)if(!SameSlice(x.layers[k],y.layers[k]))return false;   // R3c: a blend's layers
    return true;
}
}
bool RtSameMaterials(const RtMaterialSet& a,const RtMaterialSet& b) noexcept {
    if(a.buffer!=b.buffer || a.non_opaque!=b.non_opaque || a.skin!=b.skin || a.wind!=b.wind || a.slices.size()!=b.slices.size())return false;
    if(a.dol!=b.dol || a.sh!=b.sh || a.forward!=b.forward)return false;   // I-0125 R6a, R6d
    for(size_t s=0;s<a.slices.size();++s)if(!SameSlice(a.slices[s],b.slices[s]))return false;
    return true;
}
uint32_t RtMaterialTable::RawView(ID3D12Device* device,BindlessHeap& heap,uint64_t completed,View& view,ID3D12Resource* buffer) {
    if(view.slot!=BindlessIndices::none && view.buffer)return view.slot;
    view.slot=heap.Allocate(completed);if(view.slot==BindlessIndices::none)return view.slot;
    D3D12_SHADER_RESOURCE_VIEW_DESC d{DXGI_FORMAT_R32_TYPELESS,D3D12_SRV_DIMENSION_BUFFER,D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING};
    d.Buffer.NumElements=static_cast<UINT>(buffer->GetDesc().Width/4);d.Buffer.Flags=D3D12_BUFFER_SRV_FLAG_RAW;
    device->CreateShaderResourceView(buffer,&d,heap.Cpu(view.slot));view.buffer=buffer;return view.slot;
}
uint32_t RtMaterialTable::TextureView(ID3D12Device* device,BindlessHeap& heap,uint64_t completed,const std::shared_ptr<const void>& texture,
    D3D12_CPU_DESCRIPTOR_HANDLE view) {
    // A view is counted by the entries naming it and keeps a weak reference to its texture, a make_shared record whose block (and
    // so its address) outlives it: its address is never a new texture's while the view lives (final review L3: no expired branch).
    auto& v=textures_[texture.get()];   // I-0120 S13: its ticket parameter dropped (unused: the release by the last use takes its own)
    if(!v.uses){v.slot=heap.Allocate(completed);if(v.slot==BindlessIndices::none){textures_.erase(texture.get());++full_;return BindlessIndices::none;}
        v.texture=texture;device->CopyDescriptorsSimple(1,heap.Cpu(v.slot),view,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);}
    ++v.uses;return v.slot;
}
void RtMaterialTable::Retire(BindlessHeap& heap,uint64_t ticket,const Entry& entry) noexcept {
    const auto unuse=[&](auto& map,auto key){const auto found=map.find(key);if(found==map.end() || !found->second.uses)return;
        if(--found->second.uses==0){if(found->second.slot!=BindlessIndices::none)try{heap.Release(found->second.slot,ticket);}catch(...){}map.erase(found);}};
    for(const void* texture:entry.textures)unuse(textures_,texture);
    if(entry.buffer)unuse(buffers_,entry.buffer);
    records_-=entry.count;try{Free(entry.first,entry.count);}catch(...){}   // step review L3: a range not given back is lost, never a terminate
}
void RtMaterialTable::Free(uint32_t first,uint32_t count) {
    if(!count)return;
    auto at=std::lower_bound(free_.begin(),free_.end(),std::pair<uint32_t,uint32_t>{first,0});at=free_.insert(at,{first,count});
    if(at+1!=free_.end() && at->first+at->second==(at+1)->first){at->second+=(at+1)->second;free_.erase(at+1);}   // joined with the next
    if(at!=free_.begin() && (at-1)->first+(at-1)->second==at->first){(at-1)->second+=at->second;free_.erase(at);}   // and the former
}
uint32_t RtMaterialTable::Allocate(uint32_t count) {
    for(auto at=free_.begin();at!=free_.end();++at)if(at->second>=count){const uint32_t first=at->first;   // the first range that fits
        at->first+=count;at->second-=count;if(!at->second)free_.erase(at);return first;}
    return UINT32_MAX;
}
RtMaterialTable::Counters RtMaterialTable::Totals() const noexcept {
    return {entries_.size(),records_,buffers_.size()+positions_.size(),textures_.size(),written_,full_,capacity_,encode_ms_last_,
        encode_calls_?encode_ms_sum_/double(encode_calls_):0.0};
}
RtMaterialTable::Frame RtMaterialTable::Encode(ID3D12Device* device,ID3D12GraphicsCommandList* list,BindlessHeap& heap,const Upload& upload,uint64_t ticket,
    uint64_t completed,std::initializer_list<std::span<const RtInstance>> groups) {
    ++frame_;Frame out;
    LARGE_INTEGER started{};QueryPerformanceCounter(&started);
    // Final review M3: the time of each call that names instances, for the row.
    struct Timed {RtMaterialTable& t;LARGE_INTEGER a;bool on{};~Timed(){if(!on)return;LARGE_INTEGER b{},f{};QueryPerformanceCounter(&b);QueryPerformanceFrequency(&f);
        t.encode_ms_last_=1000.0*double(b.QuadPart-a.QuadPart)/double(f.QuadPart);t.encode_ms_sum_+=t.encode_ms_last_;++t.encode_calls_;}} timed{*this,started};
    // Every 16th frame (final review M3), the sets gone (no instance holds them: every frame that read their records is submitted)
    // and those no frame named for kRetireFrames frames: their views unused and their ranges freed; a set made at a gone one's
    // address meanwhile is told apart at its lookup (its entry's set expired) and replaces it.
    if(frame_%kSweepFrames==0){
        for(auto it=entries_.begin();it!=entries_.end();){
            if(it->second.set.expired() || it->second.named+kRetireFrames<frame_){Retire(heap,ticket,it->second);it=entries_.erase(it);}else ++it;}
        for(auto it=positions_.begin();it!=positions_.end();){
            if(it->second.named+kRetireFrames<frame_){if(it->second.slot!=BindlessIndices::none)heap.Release(it->second.slot,ticket);it=positions_.erase(it);}else ++it;}
        for(auto it=lightmaps_.begin();it!=lightmaps_.end();){   // I-0125 R6a: as the positions
            if(it->second.named+kRetireFrames<frame_ || it->second.texture.expired()){if(it->second.slot!=BindlessIndices::none)heap.Release(it->second.slot,ticket);it=lightmaps_.erase(it);}else ++it;}}
    // I-0125 R6d: the forward blocks (RtForwardRow) after the bases, one a block however many instances share it (a draw group's:
    // CollectNativeRtForwardBlocks), in the order first met.
    uint32_t instances{};uint64_t bases{};std::unordered_map<const RtForwardRow*,uint32_t> block_of;
    for(const auto& group:groups)for(const auto& instance:group){++instances;if(instance.positions && instance.bases)bases+=instance.bases->size();
        if(instance.ambient.forward)block_of.try_emplace(instance.ambient.forward.get(),static_cast<uint32_t>(block_of.size()));}
    const uint64_t forwards=block_of.size();
    if(!instances)return out;
    if(!device || !list)throw std::invalid_argument("rt material table input");
    timed.on=true;
    std::vector<const Entry*> named;named.reserve(instances);   // each instance's entry (null: no set), for its row
    // The sets first named this frame: their records made (their views counted) into the CPU table.
    std::vector<std::pair<uint32_t,uint32_t>> dirty;bool grown{};
    for(const auto& group:groups)for(const auto& instance:group){if(!instance.materials){named.push_back(nullptr);continue;}
        const auto* set=instance.materials.get();auto [it,fresh]=entries_.try_emplace(set);
        // Defensive (the entry's weak reference keeps a make_shared set's address from reuse): an expired entry met at a lookup
        // is replaced, never read.
        if(!fresh && it->second.set.expired()){Retire(heap,ticket,it->second);it->second=Entry{};fresh=true;}
        auto& entry=it->second;entry.named=frame_;named.push_back(&entry);
        if(!fresh)continue;
        // Final review L4: the entry filled only once its range is held (a throw leaves an empty entry, which retires as nothing).
        // R3c: the slices' records, then their map blends' layer records (RtMaterialSlice::layers), in the slices' order.
        uint64_t layered{};for(const auto& slice:set->slices)layered+=slice.layers.size();
        if(set->slices.size()+layered>0xFFFFFFu){entries_.erase(it);throw std::runtime_error("rt material set past 24 bits");}
        const auto count=static_cast<uint32_t>(set->slices.size()+layered);
        uint32_t first=Allocate(count);
        if(first==UINT32_MAX){   // the table grows by half again (the new buffer written whole below)
            const uint32_t old=capacity_,need=static_cast<uint32_t>((std::max)(uint64_t(old)+count,uint64_t(old)+old/2+256));
            if(need>0xFFFFFFu){entries_.erase(it);throw std::runtime_error("rt material table past 24 bits");}
            Free(old,need-old);capacity_=need;shadow_.resize(need);grown=true;first=Allocate(count);}
        entry.set=instance.materials;entry.count=count;entry.first=first;records_+=count;written_+=count;dirty.push_back({first,count});
        uint32_t buffer=BindlessIndices::none;
        if(set->buffer){auto& v=buffers_[set->buffer.Get()];buffer=RawView(device,heap,completed,v,set->buffer.Get());
            if(buffer==BindlessIndices::none){buffers_.erase(set->buffer.Get());++full_;}else{++v.uses;entry.buffer=set->buffer.Get();}}
        const auto write=[&](const RtMaterialSlice& slice,RtMaterialRecord& r,bool layer){r=RtMaterialRecord{};
            // I-0120 S1: a slice the collect never reached (no programme set, no policy, failed bounds: no position stride) takes no
            // view either, so a hit on it shades with the ray's own normal, not its index and position fields left at zero. A blend's
            // layer record carries no triangle of its own (its slice's), so it keeps the view (the final review's H1).
            r.buffer=layer || slice.position_stride?buffer:BindlessIndices::none;r.keep=slice.keep;r.family=slice.family;for(auto& t:r.textures)t=BindlessIndices::none;
            r.tiling=slice.tiling;r.albedo=slice.albedo;r.emissive=slice.emissive;
            if(r.buffer==BindlessIndices::none)return false;   // no view of its triangles: opaque, no material (a view the heap refused)
            r.index_offset=slice.index_offset;r.index_width=slice.index_width;r.position_offset=slice.position_offset;r.position_stride=slice.position_stride;
            r.uv_offset=slice.uv_offset;r.uv_stride=slice.uv_stride;r.uv_format=slice.uv_format;
            r.normal_offset=slice.normal_offset;r.normal_stride=slice.normal_stride;r.tangent_offset=slice.tangent_offset;r.tangent_stride=slice.tangent_stride;
            r.colour_offset=slice.colour_offset;r.colour_stride=slice.colour_stride;r.colour_format=slice.colour_format;r.ref=slice.ref;
            for(uint32_t k=0;k<RtTextureRoles;++k)if(slice.views[k].ptr && slice.textures[k]){
                r.textures[k]=TextureView(device,heap,completed,slice.textures[k],slice.views[k]);
                if(r.textures[k]!=BindlessIndices::none)entry.textures.push_back(slice.textures[k].get());}
            r.flags=(slice.material?RtRecordMaterial:0u)|(slice.normal_stride?RtRecordNormal:0u)|(slice.tangent_stride?RtRecordTangent:0u)|
                (slice.unscrolled?RtRecordUnscrolled:0u)|(slice.normal_half?RtRecordNormalHalf:0u)|(slice.tangent_half?RtRecordTangentHalf:0u)|
                (slice.mul?RtRecordMul:0u)|(slice.mul_mask?RtRecordMulMask:0u)|(slice.skin?RtRecordSkin:0u)|(slice.hair?RtRecordHair:0u)|(slice.face?RtRecordFace:0u)|(slice.face_both?RtRecordFaceBoth:0u);
            // I-0120 S4: a material whose diffuse view the heap refused is unread (its hits a miss), not FC_DifMapMulCol over white.
            if(slice.material && r.textures[RtTextureDiffuse]==BindlessIndices::none)r.flags&=~RtRecordMaterial;
            // T3's alpha test: its texture's view refused, the slice left opaque (T3 step review L4: never the rays turned off).
            if(slice.alpha && (!slice.alpha_texture || r.textures[RtTextureDiffuse]!=BindlessIndices::none))
                r.flags|=RtRecordAlpha|(slice.alpha_texture?RtRecordAlphaTexture:0u);
            r.ambient=slice.ambient;r.lightmap_offset=slice.lightmap_offset;r.lightmap_stride=slice.lightmap_stride;r.lightmap_format=slice.lightmap_format;   // I-0125 R6a
            // I-0125 R6d: a forward slice whose sampled roles have their views (none: no record, the rays pass it over as a forward-only
            // slice): the diffuse for GXFlver_Col, the four roles for the zenith's, three for the cloud sheets' (the fixes' step review M2).
            const uint32_t sampled=slice.forward_kind==RtForwardBlendMask?4u:slice.forward_kind==RtForwardOpacity?3u:1u;
            if(slice.forward && std::all_of(r.textures.begin(),r.textures.begin()+sampled,[](uint32_t t){return t!=BindlessIndices::none;})){r.flags|=RtRecordForward;r.forward=slice.forward_blend|slice.forward_kind<<8;
                r.forward_colour_offset=slice.forward_colour_offset;r.forward_colour_stride=slice.forward_colour_stride;r.forward_colour_format=slice.forward_colour_format;}
            return true;};
        uint32_t next_layer=first+static_cast<uint32_t>(set->slices.size());
        for(size_t s=0;s<set->slices.size();++s){const auto& slice=set->slices[s];auto& r=shadow_[first+s];
            const bool viewed=write(slice,r,false);
            if(slice.layers.empty())continue;
            // (R6d, the fixes' step review L1: a forward kind's layer record is its parts, not a blend's: no RtRecordLayers.)
            if(viewed){r.flags|=slice.mul || slice.forward?0u:RtRecordLayers;r.extension=next_layer;}   // R3c: a Mul piece's second layer (RtRecordMul) or a blend's
            for(const auto& layer:slice.layers)(void)write(layer,shadow_[next_layer++],true);}}
    // The table: created or grown (written whole from the CPU table, the former buffer retired after this list), else the new
    // ranges copied in; then readable by every shader stage (a buffer starts each submission in COMMON: the copy promotes it).
    const auto copy_in=[&](ID3D12Resource* into,uint64_t at,const void* bytes,uint64_t size){const auto range=upload(size);
        if(!range.cpu || !range.resource)throw std::runtime_error("rt material upload");std::memcpy(range.cpu,bytes,size);
        list->CopyBufferRegion(into,at,range.resource,range.offset,size);};
    std::vector<D3D12_RESOURCE_BARRIER> readable;
    const auto read_after=[&](ID3D12Resource* buffer){D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition={buffer,0,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE};readable.push_back(b);};
    if(grown || (!table_ && capacity_)){
        if(table_)RetireAfterCurrentRecording(std::make_shared<ComPtr<ID3D12Resource>>(std::move(table_)));
        const auto heap_props=HeapOf(D3D12_HEAP_TYPE_DEFAULT);const auto desc=BufferDesc(uint64_t(capacity_)*sizeof(RtMaterialRecord),D3D12_RESOURCE_FLAG_NONE);
        Check(device->CreateCommittedResource(&heap_props,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&table_)),"rt material table");
        copy_in(table_.Get(),0,shadow_.data(),uint64_t(capacity_)*sizeof(RtMaterialRecord));read_after(table_.Get());}
    else if(!dirty.empty()){
        std::sort(dirty.begin(),dirty.end());
        for(size_t k=0;k<dirty.size();){uint32_t first=dirty[k].first,end=first+dirty[k].second;   // adjacent ranges in one copy
            for(++k;k<dirty.size() && dirty[k].first==end;++k)end+=dirty[k].second;
            copy_in(table_.Get(),uint64_t(first)*sizeof(RtMaterialRecord),shadow_.data()+first,uint64_t(end-first)*sizeof(RtMaterialRecord));}
        read_after(table_.Get());}
    // The rows (one an instance, in the top level's order), then the skinned instances' slice bases, then the forward blocks.
    const uint64_t forward_at=uint64_t(instances)*sizeof(RtInstanceRow)+bases*4;
    const uint64_t rows_size=RtAlign(forward_at+forwards*sizeof(RtForwardRow));
    if(forward_at+forwards*sizeof(RtForwardRow)>UINT32_MAX)throw std::runtime_error("rt material rows size");
    if(!rows_ || rows_bytes_<rows_size){
        if(rows_)RetireAfterCurrentRecording(std::make_shared<ComPtr<ID3D12Resource>>(std::move(rows_)));
        const uint64_t size=RtAlign((std::max)(rows_size,rows_bytes_+rows_bytes_/2));
        const auto heap_props=HeapOf(D3D12_HEAP_TYPE_DEFAULT);const auto desc=BufferDesc(size,D3D12_RESOURCE_FLAG_NONE);
        Check(device->CreateCommittedResource(&heap_props,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&rows_)),"rt material rows");rows_bytes_=size;}
    const auto frame=upload(rows_size);if(!frame.cpu || !frame.resource)throw std::runtime_error("rt material upload");
    auto* rows=reinterpret_cast<RtInstanceRow*>(frame.cpu);
    uint32_t* base=reinterpret_cast<uint32_t*>(frame.cpu+uint64_t(instances)*sizeof(RtInstanceRow));uint32_t next_base=0;
    auto* forward=reinterpret_cast<RtForwardRow*>(frame.cpu+forward_at);for(const auto& [block,at]:block_of)forward[at]=*block;
    out.firsts.reserve(instances);
    size_t k=0;
    for(const auto& group:groups)for(const auto& instance:group){auto& row=*rows++;row.positions=row.bases=BindlessIndices::none;
        std::memcpy(row.scroll.data(),&instance.scroll[0],16);std::memcpy(row.scroll1.data(),&instance.scroll[4],16);std::memcpy(row.scroll3.data(),&instance.scroll[12],16);
        row.skin=instance.skin;row.frame=instance.frame;row.first=row.count=0;
        row.atlas=instance.ambient.atlas;row.rotate=instance.ambient.rotate;row.sh=instance.ambient.sh;std::memcpy(row.hdr.data(),instance.ambient.hdr.data(),12);
        row.lit={instance.ambient.sh_read?1u:0u,BindlessIndices::none,BindlessIndices::none,instance.ambient.light_set};
        row.forward=BindlessIndices::none;   // I-0125 R6d: its forward block's byte
        if(instance.ambient.forward)row.forward=static_cast<uint32_t>(forward_at+block_of.at(instance.ambient.forward.get())*sizeof(RtForwardRow));
        // I-0125 R6a: its lightmaps' views, one slot a texture, named by this frame (released kRetireFrames after the last frame naming it).
        for(uint32_t m=0;m<2;++m)if(const auto& texture=instance.ambient.lightmaps[m];texture && instance.ambient.views[m].ptr){auto& v=lightmaps_[texture.get()];
            // The final review's L2: a held texture's slot never rewritten. The weak reference keeps a make_shared record's block (every
            // NativeTextureRecord is one), so no new texture takes a live entry's address.
            if(v.slot==BindlessIndices::none){v.slot=heap.Allocate(completed);
                if(v.slot==BindlessIndices::none){lightmaps_.erase(texture.get());++full_;continue;}
                device->CopyDescriptorsSimple(1,heap.Cpu(v.slot),instance.ambient.views[m],D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);v.texture=texture;}
            v.named=frame_;row.lit[1+m]=v.slot;}
        // I-0132 R11f's step review M2: the row's first from a local (the row lives in the upload arena, a GPU upload heap: never read back).
        const Entry* entry=named[k++];uint32_t first=0;if(entry){first=entry->first;row.first=first;row.count=entry->count;}
        out.firsts.push_back(first);
        if(entry && instance.positions && instance.bases){   // a skinned instance: its positions' view, named by this frame (I-0120 S7: only with a set)
            auto& v=positions_[instance.positions.Get()];v.named=frame_;
            // Step review L1: a view the heap refused: no record read at its hits (its bind-pose positions are not its placed ones).
            if(RawView(device,heap,completed,v,instance.positions.Get())==BindlessIndices::none){positions_.erase(instance.positions.Get());++full_;row.count=0;}
            else{row.positions=v.slot;row.bases=static_cast<uint32_t>(uint64_t(instances)*sizeof(RtInstanceRow)+4ull*next_base);
                for(const uint32_t b:*instance.bases)base[next_base++]=b;}}}
    list->CopyBufferRegion(rows_.Get(),0,frame.resource,frame.offset,rows_size);read_after(rows_.Get());
    list->ResourceBarrier(static_cast<UINT>(readable.size()),readable.data());
    out.table=table_?table_->GetGPUVirtualAddress():0;out.rows=rows_->GetGPUVirtualAddress();out.records=static_cast<uint32_t>(records_);out.instances=instances;
    return out;
}
void RtMaterialTable::Close(BindlessHeap* heap,uint64_t ticket) noexcept {
    if(heap)try{for(const auto* map:{&buffers_,&positions_})for(const auto& [k,v]:*map)if(v.slot!=BindlessIndices::none)heap->Release(v.slot,ticket);
        for(const auto& [k,v]:lightmaps_)if(v.slot!=BindlessIndices::none)heap->Release(v.slot,ticket);   // I-0125 R6a
        for(const auto& [k,v]:textures_)if(v.slot!=BindlessIndices::none)heap->Release(v.slot,ticket);}catch(...){}
    buffers_.clear();positions_.clear();textures_.clear();lightmaps_.clear();entries_.clear();free_.clear();shadow_.clear();
    table_.Reset();rows_.Reset();capacity_=0;rows_bytes_=0;records_=0;
}

bool RtViewCameraOf(const SceneViewInput& camera,RtViewConstants& out) noexcept {
    SceneView checked{};if(!BuildSceneView(camera,checked))return false;
    const auto& pose=camera.camera_pose;
    const float aspect=camera.explicit_aspect==0?float(camera.width)/float(camera.height):camera.explicit_aspect;
    const float sy=std::cos(camera.vertical_fov*.5f)/std::sin(camera.vertical_fov*.5f),sx=sy/aspect;   // BuildSceneView's lens
    out.right={pose[0],pose[1],pose[2],camera.near_plane};out.up={pose[4],pose[5],pose[6],camera.far_plane};
    out.forward={pose[8],pose[9],pose[10],sx};out.position={pose[12],pose[13],pose[14],sy};
    out.jitter={2*camera.jitter_x/float(camera.width),-2*camera.jitter_y/float(camera.height),1/float(camera.width),1/float(camera.height)};
    out.width=camera.width;out.height=camera.height;
    return true;
}
RtViewCounts RtViewCountsOf(const uint32_t w[16]) noexcept {return {w[0],w[1],w[2],w[3],w[4],w[5],w[6],w[7],w[8]};}
void RtView::Encode(ID3D12Device* device,ID3D12GraphicsCommandList* list,BindlessHeap& heap,const std::function<UploadArena::Range(uint64_t)>& upload,
    D3D12_GPU_VIRTUAL_ADDRESS scene,D3D12_GPU_VIRTUAL_ADDRESS table,D3D12_GPU_VIRTUAL_ADDRESS rows,RtViewConstants constants,ID3D12Resource* counts) {
    if(!device || !list || !scene || !counts || !heap.IsOpen() || !constants.width || !constants.height)throw std::invalid_argument("rt view input");
    if(!pipeline_){
        Check(device->CreateRootSignature(0,rt_view_dxil,sizeof rt_view_dxil,IID_PPV_ARGS(&root_)),"rt view root");
        D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root_.Get();pd.CS={rt_view_dxil,sizeof rt_view_dxil};
        Check(device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pipeline_)),"rt view pipeline");}
    constants.materials=table && rows?1:0;
    const auto put=[&](const RtViewConstants& c){const auto range=upload(256);std::memcpy(range.cpu,&c,sizeof c);return range.gpu;};
    auto clear=constants;clear.clear=1;
    const auto clear_block=put(clear),view_block=put(constants);
    ID3D12DescriptorHeap* heaps[]{heap.Heap()};list->SetDescriptorHeaps(1,heaps);
    list->SetComputeRootSignature(root_.Get());list->SetPipelineState(pipeline_.Get());
    list->SetComputeRootShaderResourceView(1,scene);
    list->SetComputeRootShaderResourceView(2,constants.materials?table:scene);   // no table: never read (every candidate opaque)
    list->SetComputeRootShaderResourceView(3,constants.materials?rows:scene);
    list->SetComputeRootUnorderedAccessView(4,counts->GetGPUVirtualAddress());   // promoted to UNORDERED_ACCESS from COMMON (a buffer)
    list->SetComputeRootConstantBufferView(0,clear_block);list->Dispatch(1,1,1);
    D3D12_RESOURCE_BARRIER cleared{};cleared.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;cleared.UAV.pResource=counts;list->ResourceBarrier(1,&cleared);
    list->SetComputeRootConstantBufferView(0,view_block);list->Dispatch((constants.width+7)/8,(constants.height+7)/8,1);
    list->ResourceBarrier(1,&cleared);
}
bool RtSunConstantsOf(const RtViewConstants& view,const std::array<float,4>& sun,RtSunConstants& out) noexcept {
    const float n=std::sqrt(sun[0]*sun[0]+sun[1]*sun[1]+sun[2]*sun[2]);
    if(!std::isfinite(n) || n<1e-6f)return false;
    out.right=view.right;out.up=view.up;out.forward=view.forward;out.position=view.position;out.jitter=view.jitter;
    out.width=view.width;out.height=view.height;out.sun={sun[0]/n,sun[1]/n,sun[2]/n,0};return true;
}
void RtSunShadow::Encode(ID3D12Device* device,ID3D12GraphicsCommandList* list,BindlessHeap& heap,const std::function<UploadArena::Range(uint64_t)>& upload,
    D3D12_GPU_VIRTUAL_ADDRESS scene,D3D12_GPU_VIRTUAL_ADDRESS table,D3D12_GPU_VIRTUAL_ADDRESS rows,RtSunConstants constants,D3D12_CPU_DESCRIPTOR_HANDLE mask_rtv,
    D3D12_CPU_DESCRIPTOR_HANDLE stencil,DXGI_FORMAT mask_format) {
    // Step review M1: no top level is the caller's (no rays to trace); L3: the pipelines made once, through the device's pipeline
    // cache, for one mask format (the AO stage's, constant): another is refused rather than the objects replaced under frames in flight.
    if(!device || !list || !scene || !heap.IsOpen() || !constants.width || !constants.height || !mask_rtv.ptr || !stencil.ptr)throw std::invalid_argument("rt sun shadow input");
    if(trace_ && format_!=mask_format)throw std::invalid_argument("rt sun shadow mask format");
    if(!trace_){
        Check(Pipelines().Root(device,rt_shadow_trace_dxil,sizeof rt_shadow_trace_dxil,root_.ReleaseAndGetAddressOf()),"rt sun shadow root");
        MakePipeline(device,rt_shadow_fill_dxil,sizeof rt_shadow_fill_dxil,false,mask_format,fill_,"rt sun shadow fill pipeline");
        MakePipeline(device,rt_shadow_trace_dxil,sizeof rt_shadow_trace_dxil,true,mask_format,trace_,"rt sun shadow trace pipeline");format_=mask_format;}
    constants.materials=table && rows?1:0;
    const auto block=upload(256);std::memcpy(block.cpu,&constants,sizeof constants);
    ID3D12DescriptorHeap* heaps[]{heap.Heap()};list->SetDescriptorHeaps(1,heaps);
    list->SetGraphicsRootSignature(root_.Get());list->SetGraphicsRootConstantBufferView(0,block.gpu);
    list->SetGraphicsRootShaderResourceView(1,scene);   // no table: the table and rows never read
    list->SetGraphicsRootShaderResourceView(2,constants.materials?table:scene);list->SetGraphicsRootShaderResourceView(3,constants.materials?rows:scene);
    list->OMSetRenderTargets(1,&mask_rtv,FALSE,&stencil);list->OMSetStencilRef(0xFF);
    list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);list->IASetVertexBuffers(0,0,nullptr);list->IASetIndexBuffer(nullptr);
    list->SetPipelineState(fill_.Get());list->DrawInstanced(3,1,0,0);
    list->SetPipelineState(trace_.Get());list->DrawInstanced(3,1,0,0);
}
// The original's mask layer states (SunShadowStage, 1D60C0): depth off, the stencil's bit 0x08 read (written 0, ops keep) against
// the reference 0xFF: EQUAL for the traced term (and the blur), NOT_EQUAL for OutputColor's value; no blend.
void RtSunShadow::MakePipeline(ID3D12Device* device,const unsigned char* pixel,size_t size,bool written,DXGI_FORMAT format,
    Microsoft::WRL::ComPtr<ID3D12PipelineState>& out,const char* what) {
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};pso.pRootSignature=root_.Get();
    pso.VS={rt_shadow_vs_dxil,sizeof rt_shadow_vs_dxil};pso.PS={pixel,size};
    auto& target=pso.BlendState.RenderTarget[0];target.LogicOp=D3D12_LOGIC_OP_NOOP;target.RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;
    target.SrcBlend=target.SrcBlendAlpha=D3D12_BLEND_ONE;target.DestBlend=target.DestBlendAlpha=D3D12_BLEND_ZERO;target.BlendOp=target.BlendOpAlpha=D3D12_BLEND_OP_ADD;
    pso.SampleMask=UINT_MAX;pso.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;pso.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;
    pso.RasterizerState.DepthClipEnable=TRUE;pso.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;pso.SampleDesc.Count=1;
    pso.NumRenderTargets=1;pso.RTVFormats[0]=format;pso.DSVFormat=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    auto& depth=pso.DepthStencilState;depth.DepthEnable=FALSE;depth.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ZERO;depth.StencilEnable=TRUE;
    depth.StencilReadMask=0x08;depth.StencilWriteMask=0;
    depth.FrontFace={D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,written?D3D12_COMPARISON_FUNC_EQUAL:D3D12_COMPARISON_FUNC_NOT_EQUAL};
    depth.BackFace=depth.FrontFace;
    Check(Pipelines().Graphics(device,pso,out.ReleaseAndGetAddressOf()),what);
}
void RtSunShadow::EncodeBlur(ID3D12Device* device,ID3D12GraphicsCommandList* list,BindlessHeap& heap,const std::function<UploadArena::Range(uint64_t)>& upload,
    D3D12_GPU_VIRTUAL_ADDRESS scene,const RtSunConstants& constants,D3D12_CPU_DESCRIPTOR_HANDLE target_rtv,D3D12_CPU_DESCRIPTOR_HANDLE stencil) {
    if(!trace_)throw std::logic_error("rt sun shadow blur before its pipelines");
    if(!device || !list || !scene || !heap.IsOpen() || !constants.width || !constants.height || !target_rtv.ptr || !stencil.ptr ||
        !std::isfinite(constants.blur) || !(constants.blur>0.f))throw std::invalid_argument("rt sun shadow blur input");
    // The step review's M1: the blur's pipeline made on its first use (the Player menu views' only), so that its failure reaches
    // the menu's caller alone, never the main view's traced sun made in Encode.
    if(!blur_)MakePipeline(device,rt_shadow_blur_dxil,sizeof rt_shadow_blur_dxil,true,format_,blur_,"rt sun shadow blur pipeline");   // I-0140 tranche 35
    const auto block=upload(256);std::memcpy(block.cpu,&constants,sizeof constants);
    ID3D12DescriptorHeap* heaps[]{heap.Heap()};list->SetDescriptorHeaps(1,heaps);
    list->SetGraphicsRootSignature(root_.Get());list->SetGraphicsRootConstantBufferView(0,block.gpu);
    for(UINT slot=1;slot<=3;++slot)list->SetGraphicsRootShaderResourceView(slot,scene);   // unread by the blur; every root slot set
    list->OMSetRenderTargets(1,&target_rtv,FALSE,&stencil);list->OMSetStencilRef(0xFF);
    list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);list->IASetVertexBuffers(0,0,nullptr);list->IASetIndexBuffer(nullptr);
    list->SetPipelineState(fill_.Get());list->DrawInstanced(3,1,0,0);
    list->SetPipelineState(blur_.Get());list->DrawInstanced(3,1,0,0);
}

bool RtPointLightOf(const std::array<float,16>& row,const std::array<float,3>& world,bool legacy,RtLight& out) noexcept {
    if(!std::all_of(row.begin(),row.end(),[](float v){return std::isfinite(v);}) || !std::all_of(world.begin(),world.end(),[](float v){return std::isfinite(v);}))return false;
    out={};out.position={world[0],world[1],world[2],row[3]};out.diffuse={row[4],row[5],row[6],row[7]};out.specular={row[8],row[9],row[10]};
    out.kind=legacy?kRtLightLegacy:kRtLightPoint;out.trans={row[12],row[13],0,0};
    return true;
}
bool RtSpotLightOf(const std::array<float,32>& row,const float pose[16],const float view[16],RtLight& out) noexcept {
    if(!std::all_of(row.begin(),row.end(),[](float v){return std::isfinite(v);}))return false;
    out={};
    // The position: the view-space point times the pose (row vectors: world[c] = sum p[j] pose[4j + c] + pose[12 + c]).
    for(int c=0;c<3;++c)out.position[c]=row[0]*pose[c]+row[1]*pose[4+c]+row[2]*pose[8+c]+pose[12+c];
    out.position[3]=row[3];out.diffuse={row[4],row[5],row[6],row[7]};out.specular={row[8],row[9],row[10]};
    out.kind=kRtLightSpot;out.trans={row[12],row[13],0,row[15]};
    // A row r applied to the view-space point (p_view[j] = sum_k pw[k] view[4k + j] + view[12 + j]): r'[k] = sum_j r[j] view[4k + j],
    // r'[3] = sum_j r[j] view[12 + j] + r[3].
    for(int i=0;i<4;++i){const float* r=&row[16+4*i];float* w=&out.to_light[4*i];
        for(int k=0;k<3;++k)w[k]=r[0]*view[4*k]+r[1]*view[4*k+1]+r[2]*view[4*k+2];
        w[3]=r[0]*view[12]+r[1]*view[13]+r[2]*view[14]+r[3];}
    return std::all_of(out.position.begin(),out.position.end(),[](float v){return std::isfinite(v);}) &&
        std::all_of(out.to_light.begin(),out.to_light.end(),[](float v){return std::isfinite(v);});
}
bool RtPhysicalLight(RtLight& light,float scale,float e_min) noexcept {
    const uint32_t kind=light.kind&0xFFu;
    double range{};if(kind==kRtLightSpot)range=light.position[3];else if(light.position[3]>0.f)range=1/std::sqrt(double(light.position[3]));
    const bool square=kind==kRtLightPoint || (kind==kRtLightSpot && light.trans[3]>0.f);
    const double factor=square?1.0:range*range/16.0;
    if(!(factor>0) || !std::isfinite(factor))return false;
    // The second step review's L5: the reach from the brighter of the diffuse and the specular.
    const auto luma=[](const auto& c){return 0.2126*c[0]+0.7152*c[1]+0.0722*c[2];};
    const double luminance=std::max(luma(light.diffuse),luma(light.specular))*factor;
    const double reach=std::sqrt(std::max(luminance,0.0)/double(e_min));
    if(!(reach>0) || !std::isfinite(reach) || !(scale>0))return false;
    for(int c=0;c<3;++c){light.diffuse[c]=float(light.diffuse[c]*factor*scale);light.specular[c]=float(light.specular[c]*factor*scale);}
    // The second step review's M1: DS3's range kept (a half float in the kind's upper 16 bits) for the soft shadow's sphere and the
    // hits' stop, so the sheets change the falloff alone, not the light's size.
    uint32_t half{};{const float f=float(std::min(range,60000.0));uint32_t bits{};std::memcpy(&bits,&f,4);
        const int exponent=int((bits>>23)&0xFF)-127+15;half=exponent<=0?0u:exponent>=31?0x7BFFu:uint32_t((exponent<<10)|((bits>>13)&0x3FF));}
    light.position[3]=float(1/(reach*reach));light.kind=(light.kind&0xFFFFu)|kRtLightPhysical|(half<<16);
    return true;
}
bool RtShadowedOf(std::span<const float> shadow,RtLight& record) noexcept {
    const bool spot=(record.kind&0xFF)==kRtLightSpot;
    if(shadow.size()<(spot?1u:4u))return false;
    const float strength=spot?shadow[0]:shadow[3];
    if(!std::isfinite(strength))return false;
    record.kind|=kRtLightShadowed;record.trans[2]=strength;
    if(!spot){const float face_near=shadow[0]>0.f?-shadow[1]/shadow[0]:0.f;record.trans[3]=std::isfinite(face_near) && face_near>0.f?face_near:0.f;}
    return true;
}
// I-0132 R12d: the RT level's pipeline and the PT level's (another pixel shader, the same state): Encode's and Warm's description.
D3D12_GRAPHICS_PIPELINE_STATE_DESC RtLocalLights::Describe(bool pt,ID3D12RootSignature* root) noexcept {
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};pso.pRootSignature=root;
    pso.VS={rt_lights_vs_dxil,sizeof rt_lights_vs_dxil};pso.PS={rt_lights_shade_dxil,sizeof rt_lights_shade_dxil};
    for(auto& target:pso.BlendState.RenderTarget){target.LogicOp=D3D12_LOGIC_OP_NOOP;target.RenderTargetWriteMask=7;
        target.SrcBlend=target.SrcBlendAlpha=D3D12_BLEND_ONE;target.DestBlend=target.DestBlendAlpha=D3D12_BLEND_ZERO;target.BlendOp=target.BlendOpAlpha=D3D12_BLEND_OP_ADD;}
    auto& target=pso.BlendState.RenderTarget[0];target.BlendEnable=TRUE;target.DestBlend=D3D12_BLEND_ONE;
    pso.SampleMask=UINT_MAX;pso.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;pso.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;
    pso.RasterizerState.DepthClipEnable=TRUE;pso.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;pso.SampleDesc.Count=1;
    pso.NumRenderTargets=2;pso.RTVFormats[0]=pso.RTVFormats[1]=DXGI_FORMAT_R11G11B10_FLOAT;
    pso.DepthStencilState.DepthEnable=FALSE;pso.DepthStencilState.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ZERO;
    if(pt)pso.PS={rt_lights_pt_dxil,sizeof rt_lights_pt_dxil};
    return pso;}
// I-0132 R12d: both made once through the store at start (RtReflections::Warm's rule).
uint32_t RtLocalLights::Warm(ID3D12Device* device,const std::atomic<bool>& cancel,std::atomic<uint32_t>& done) noexcept {
    uint32_t made{};
    try{Microsoft::WRL::ComPtr<ID3D12RootSignature> root;
        if(FAILED(Pipelines().Root(device,rt_lights_shade_dxil,sizeof rt_lights_shade_dxil,root.ReleaseAndGetAddressOf())) || !root)return 0;
        for(const bool pt:{false,true}){if(cancel.load(std::memory_order_relaxed))break;Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline;
            if(SUCCEEDED(Pipelines().Graphics(device,Describe(pt,root.Get()),pipeline.ReleaseAndGetAddressOf())) && pipeline)++made;
            done.fetch_add(1,std::memory_order_relaxed);}}
    catch(...){}
    return made;}
void RtLocalLights::Encode(ID3D12Device* device,ID3D12GraphicsCommandList* list,BindlessHeap& heap,const std::function<UploadArena::Range(uint64_t)>& upload,
    D3D12_GPU_VIRTUAL_ADDRESS lights,D3D12_GPU_VIRTUAL_ADDRESS tiles,D3D12_GPU_VIRTUAL_ADDRESS scene,D3D12_GPU_VIRTUAL_ADDRESS table,
    D3D12_GPU_VIRTUAL_ADDRESS rows,RtLightsConstants constants,const D3D12_CPU_DESCRIPTOR_HANDLE rtvs[2],bool pt) {
    if(!device || !list || !lights || !tiles || !heap.IsOpen() || !constants.width || !constants.height || !constants.columns || !constants.tile || !rtvs[0].ptr || !rtvs[1].ptr)
        throw std::invalid_argument("rt local lights input");
    if(!pipeline_){
        Check(Pipelines().Root(device,rt_lights_shade_dxil,sizeof rt_lights_shade_dxil,root_.ReleaseAndGetAddressOf()),"rt local lights root");
        // The light programmes' state (NativeLightingResources' point pipelines): depth off, no stencil, the pair R11G11B10, RGB
        // written, the first target's ONE/ONE add applied to both (IndependentBlendEnable false).
        const auto pso=Describe(false,root_.Get());
        Check(Pipelines().Graphics(device,pso,pipeline_.ReleaseAndGetAddressOf()),"rt local lights pipeline");
        }
    // R4c (final review L4): the PT level's pipeline made on its first frame, in the same state; a refusal leaves the RT level drawn.
    if(pt && scene && !pt_ && !pt_refused_){const auto pso=Describe(true,root_.Get());
        if(FAILED(Pipelines().Graphics(device,pso,pt_.ReleaseAndGetAddressOf())))pt_refused_=true;}
    // R4b: no top level, the rays never traced (the lights' buffer bound in its place); no table, the table and rows never read.
    constants.traced=scene?1:2;   // I-0131 R10a: 2, no top level: the shadowed lights shaded unshadowed (no atlas, no batch)
    constants.materials=scene && table && rows?1:0;
    const auto block=upload(256);std::memcpy(block.cpu,&constants,sizeof constants);
    ID3D12DescriptorHeap* heaps[]{heap.Heap()};list->SetDescriptorHeaps(1,heaps);
    list->SetGraphicsRootSignature(root_.Get());list->SetGraphicsRootConstantBufferView(0,block.gpu);
    list->SetGraphicsRootShaderResourceView(1,lights);list->SetGraphicsRootShaderResourceView(2,tiles);
    list->SetGraphicsRootShaderResourceView(3,scene?scene:lights);
    list->SetGraphicsRootShaderResourceView(4,constants.materials?table:lights);list->SetGraphicsRootShaderResourceView(5,constants.materials?rows:lights);
    list->OMSetRenderTargets(2,rtvs,FALSE,nullptr);
    list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);list->IASetVertexBuffers(0,0,nullptr);list->IASetIndexBuffer(nullptr);
    list->SetPipelineState(pt && scene && pt_?pt_.Get():pipeline_.Get());list->DrawInstanced(3,1,0,0);
}
void RtReflections::Encode(ID3D12Device* device,ID3D12GraphicsCommandList* list,BindlessHeap& heap,const std::function<UploadArena::Range(uint64_t)>& upload,
    D3D12_GPU_VIRTUAL_ADDRESS scene,D3D12_GPU_VIRTUAL_ADDRESS table,D3D12_GPU_VIRTUAL_ADDRESS rows,D3D12_GPU_VIRTUAL_ADDRESS lights,
    D3D12_GPU_VIRTUAL_ADDRESS regions,D3D12_GPU_VIRTUAL_ADDRESS sets,D3D12_GPU_VIRTUAL_ADDRESS fogs,RtReflectConstants constants,D3D12_CPU_DESCRIPTOR_HANDLE rtv,D3D12_CPU_DESCRIPTOR_HANDLE dsv,
    D3D12_CPU_DESCRIPTOR_HANDLE distance) {
    if(!device || !list || !heap.IsOpen() || !constants.width || !constants.height || !regions || !sets || !fogs || !rtv.ptr || !dsv.ptr)throw std::invalid_argument("rt reflections input");
    // I-0128 R8d: the paths past their first hit only on RR's frames (the PT level's), whose programme has them; else refused (no silent
    // first-hit picture).
    const bool paths=RtPathsOf(constants);if(paths && !distance.ptr)throw std::invalid_argument("rt reflections paths without rr");
    const D3D12_CPU_DESCRIPTOR_HANDLE rtvs[2]{rtv,distance};
    // I-0132 R11b: the path build reordered (SER: constants.ser set by the caller after MakeSer), into the scratch targets, then copied
    // by trace_hit's pipeline state (its stencil test and targets).
    // The step review's M1: everything that can fail is prepared before the first barrier; a failure there draws this frame with the
    // pixel shader and is reported (SerFailure: the caller turns SER off), never a latch of the reflections.
    if(paths && constants.ser[0]!=UINT32_MAX){
        ComPtr<ID3D12GraphicsCommandList4> list4;ID3D12PipelineState* apply=nullptr;
        try{if(!ser_state_ || ser_width_!=constants.width || ser_height_!=constants.height || constants.ser!=ser_slots_)throw std::invalid_argument("rt reflections ser input");
            Check(list->QueryInterface(IID_PPV_ARGS(&list4)),"rt ser list4");apply=Make(device,12);}
        catch(const std::exception& error){apply=nullptr;try{if(ser_failure_.empty())ser_failure_=*error.what()?error.what():"unknown";}catch(...){}}
        if(apply){Bind(list,heap,upload,scene,table,rows,lights,regions,sets,fogs,constants,true);DispatchSer(list4.Get(),0,constants.width,constants.height);SerBarrier(list,true);
            Bind(list,heap,upload,scene,table,rows,lights,regions,sets,fogs,constants);
            list->OMSetRenderTargets(2,rtvs,FALSE,&dsv);list->SetPipelineState(apply);list->DrawInstanced(3,1,0,0);SerBarrier(list,false);
            return;}}
    auto* const pipeline=Make(device,paths?9:distance.ptr?1:0);Bind(list,heap,upload,scene,table,rows,lights,regions,sets,fogs,constants);
    list->OMSetRenderTargets(distance.ptr?2:1,rtvs,FALSE,&dsv);
    list->SetPipelineState(pipeline);list->DrawInstanced(3,1,0,0);
}
void RtReflections::EncodeWater(ID3D12Device* device,ID3D12GraphicsCommandList* list,BindlessHeap& heap,const std::function<UploadArena::Range(uint64_t)>& upload,
    D3D12_GPU_VIRTUAL_ADDRESS scene,D3D12_GPU_VIRTUAL_ADDRESS table,D3D12_GPU_VIRTUAL_ADDRESS rows,D3D12_GPU_VIRTUAL_ADDRESS lights,
    D3D12_GPU_VIRTUAL_ADDRESS regions,D3D12_GPU_VIRTUAL_ADDRESS sets,D3D12_GPU_VIRTUAL_ADDRESS fogs,RtReflectConstants constants,D3D12_CPU_DESCRIPTOR_HANDLE rtv) {
    if(!device || !list || !heap.IsOpen() || !constants.water_width || !constants.water_height || !regions || !sets || !fogs || !rtv.ptr)throw std::invalid_argument("rt water input");
    auto* const pipeline=Make(device,2);Bind(list,heap,upload,scene,table,rows,lights,regions,sets,fogs,constants);
    const D3D12_VIEWPORT viewport{0,0,float(constants.water_width),float(constants.water_height),0,1};const D3D12_RECT scissor{0,0,LONG(constants.water_width),LONG(constants.water_height)};
    list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
    list->OMSetRenderTargets(1,&rtv,FALSE,nullptr);
    list->SetPipelineState(pipeline);list->DrawInstanced(3,1,0,0);
}
void RtReflections::EncodeGi(ID3D12Device* device,ID3D12GraphicsCommandList* list,BindlessHeap& heap,const std::function<UploadArena::Range(uint64_t)>& upload,
    D3D12_GPU_VIRTUAL_ADDRESS scene,D3D12_GPU_VIRTUAL_ADDRESS table,D3D12_GPU_VIRTUAL_ADDRESS rows,D3D12_GPU_VIRTUAL_ADDRESS lights,
    D3D12_GPU_VIRTUAL_ADDRESS regions,D3D12_GPU_VIRTUAL_ADDRESS sets,D3D12_GPU_VIRTUAL_ADDRESS fogs,RtReflectConstants constants,D3D12_CPU_DESCRIPTOR_HANDLE rtv) {
    if(!device || !list || !heap.IsOpen() || !constants.width || !constants.height || !regions || !sets || !fogs || !rtv.ptr || constants.gb2==UINT32_MAX)
        throw std::invalid_argument("rt gi input");
    auto* const pipeline=Make(device,RtPathsOf(constants)?10:3);Bind(list,heap,upload,scene,table,rows,lights,regions,sets,fogs,constants);   // I-0128 R8d: the `_path` build where the paths go further
    list->OMSetRenderTargets(1,&rtv,FALSE,nullptr);
    list->SetPipelineState(pipeline);list->DrawInstanced(3,1,0,0);
}
void RtReflections::EncodeNrdSignal(ID3D12Device* device,ID3D12GraphicsCommandList* list,BindlessHeap& heap,const std::function<UploadArena::Range(uint64_t)>& upload,
    D3D12_GPU_VIRTUAL_ADDRESS scene,D3D12_GPU_VIRTUAL_ADDRESS table,D3D12_GPU_VIRTUAL_ADDRESS rows,D3D12_GPU_VIRTUAL_ADDRESS lights,
    D3D12_GPU_VIRTUAL_ADDRESS regions,D3D12_GPU_VIRTUAL_ADDRESS sets,D3D12_GPU_VIRTUAL_ADDRESS fogs,RtReflectConstants constants,const D3D12_CPU_DESCRIPTOR_HANDLE rtvs[kRtDenoiseInputs]) {
    if(!device || !list || !heap.IsOpen() || !constants.width || !constants.height || !regions || !sets || !fogs || !rtvs || constants.velocity==UINT32_MAX)
        throw std::invalid_argument("rt nrd signal input");
    for(uint32_t i=0;i<kRtDenoiseInputs;++i)if(!rtvs[i].ptr)throw std::invalid_argument("rt nrd signal targets");
    if(RtPathsOf(constants))throw std::invalid_argument("rt nrd signal paths");   // I-0128 R8d: NRD's frames are the RT level's (its programme has no PathFrom)
    auto* const pipeline=Make(device,4);Bind(list,heap,upload,scene,table,rows,lights,regions,sets,fogs,constants);
    list->OMSetRenderTargets(kRtDenoiseInputs,rtvs,FALSE,nullptr);
    list->SetPipelineState(pipeline);list->DrawInstanced(3,1,0,0);
}
void RtReflections::EncodeSpecCompose(ID3D12Device* device,ID3D12GraphicsCommandList* list,BindlessHeap& heap,const std::function<UploadArena::Range(uint64_t)>& upload,
    D3D12_GPU_VIRTUAL_ADDRESS scene,D3D12_GPU_VIRTUAL_ADDRESS table,D3D12_GPU_VIRTUAL_ADDRESS rows,D3D12_GPU_VIRTUAL_ADDRESS lights,
    D3D12_GPU_VIRTUAL_ADDRESS regions,D3D12_GPU_VIRTUAL_ADDRESS sets,D3D12_GPU_VIRTUAL_ADDRESS fogs,RtReflectConstants constants,D3D12_CPU_DESCRIPTOR_HANDLE rtv,D3D12_CPU_DESCRIPTOR_HANDLE dsv) {
    if(!device || !list || !heap.IsOpen() || !constants.width || !constants.height || !regions || !sets || !fogs || !rtv.ptr || !dsv.ptr || constants.denoised==UINT32_MAX)
        throw std::invalid_argument("rt spec compose input");
    auto* const pipeline=Make(device,11);Bind(list,heap,upload,scene,table,rows,lights,regions,sets,fogs,constants);
    list->OMSetRenderTargets(1,&rtv,FALSE,&dsv);
    list->SetPipelineState(pipeline);list->DrawInstanced(3,1,0,0);
}
void RtReflections::EncodeGiCompose(ID3D12Device* device,ID3D12GraphicsCommandList* list,BindlessHeap& heap,const std::function<UploadArena::Range(uint64_t)>& upload,
    D3D12_GPU_VIRTUAL_ADDRESS scene,D3D12_GPU_VIRTUAL_ADDRESS table,D3D12_GPU_VIRTUAL_ADDRESS rows,D3D12_GPU_VIRTUAL_ADDRESS lights,
    D3D12_GPU_VIRTUAL_ADDRESS regions,D3D12_GPU_VIRTUAL_ADDRESS sets,D3D12_GPU_VIRTUAL_ADDRESS fogs,RtReflectConstants constants,D3D12_CPU_DESCRIPTOR_HANDLE rtv) {
    if(!device || !list || !heap.IsOpen() || !constants.width || !constants.height || !regions || !sets || !fogs || !rtv.ptr || constants.gb2==UINT32_MAX || constants.denoised==UINT32_MAX)
        throw std::invalid_argument("rt gi compose input");
    auto* const pipeline=Make(device,5);Bind(list,heap,upload,scene,table,rows,lights,regions,sets,fogs,constants);
    list->OMSetRenderTargets(1,&rtv,FALSE,nullptr);
    list->SetPipelineState(pipeline);list->DrawInstanced(3,1,0,0);
}
// The pipelines on one root, each made when first used (a refusal of one leaves the others; the step review's L4): 0 the
// specular ambient (R11G11B10 with the region draws' stencil test, bit 0x08 EQUAL to the reference, no depth test), 1 the same with
// RR's hit distance as its second target, 2 the water's (RGBA16F, no depth or stencil), 3 the GI's (I-0126 R7a: RGBA16F, its RGB
// added ONE + ONE, no depth or stencil); I-0126 R7c: 4 NRD's inputs (kRtDenoiseInputFormats, written; I-0132 R11: five, the
// specular's with them), 5 the GI's compose (as 3); I-0132 R11: 11 the specular's compose (as 0: its stencil test, R11G11B10).
// I-0128 R8d: 9 and 10, 1 and 3 with PathFrom compiled in (RT_PATHS 1), for a frame whose paths go past their first hit (RtPathsOf).
// (I-0129 R9d's kind 11, the fill lights' visibility, removed with D-0262; kind 11 is I-0132 R11's specular compose.)
// I-0132 R12d: a kind's description (Make's and Warm's: one description, so the warm-up's pipeline is the one Make finds in the store).
D3D12_GRAPHICS_PIPELINE_STATE_DESC RtReflections::Describe(int kind,ID3D12RootSignature* root) noexcept {
    // Written, not blended (the regions' blend is the pass's own).
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};pso.pRootSignature=root;
    pso.VS={rt_reflect_vs_dxil,sizeof rt_reflect_vs_dxil};pso.PS={rt_reflect_trace_dxil,sizeof rt_reflect_trace_dxil};
    for(auto& target:pso.BlendState.RenderTarget){target.LogicOp=D3D12_LOGIC_OP_NOOP;target.RenderTargetWriteMask=7;
        target.SrcBlend=target.SrcBlendAlpha=D3D12_BLEND_ONE;target.DestBlend=target.DestBlendAlpha=D3D12_BLEND_ZERO;target.BlendOp=target.BlendOpAlpha=D3D12_BLEND_OP_ADD;}
    pso.SampleMask=UINT_MAX;pso.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;pso.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;
    pso.RasterizerState.DepthClipEnable=TRUE;pso.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;pso.SampleDesc.Count=1;
    pso.NumRenderTargets=1;pso.RTVFormats[0]=DXGI_FORMAT_R11G11B10_FLOAT;pso.DSVFormat=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    auto& depth=pso.DepthStencilState;depth.DepthEnable=FALSE;depth.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ZERO;depth.DepthFunc=D3D12_COMPARISON_FUNC_ALWAYS;
    depth.StencilEnable=TRUE;depth.StencilReadMask=8;depth.StencilWriteMask=0;
    depth.FrontFace={D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_COMPARISON_FUNC_EQUAL};depth.BackFace=depth.FrontFace;
    // RT0's write mask (RGB) holds for the hit distance's R too (no independent blend).
    // I-0132 R11b: 12, ser_apply with 9's states (SER's scratch copied where trace_hit's path build writes).
    if(kind==1 || kind==9 || kind==12){pso.PS=kind==1?D3D12_SHADER_BYTECODE{rt_reflect_trace_hit_dxil,sizeof rt_reflect_trace_hit_dxil}:kind==9?D3D12_SHADER_BYTECODE{rt_reflect_trace_hit_path_dxil,sizeof rt_reflect_trace_hit_path_dxil}:
            D3D12_SHADER_BYTECODE{rt_reflect_ser_apply_dxil,sizeof rt_reflect_ser_apply_dxil};
        pso.NumRenderTargets=2;pso.RTVFormats[1]=kRrHitDistanceFormat;}
    else if(kind==2){pso.PS={rt_reflect_water_dxil,sizeof rt_reflect_water_dxil};pso.RTVFormats[0]=kRtWaterFormat;pso.BlendState.RenderTarget[0].RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;
        pso.DSVFormat=DXGI_FORMAT_UNKNOWN;pso.DepthStencilState.StencilEnable=FALSE;}
    else if(kind==3 || kind==5 || kind==10){pso.PS=kind==3?D3D12_SHADER_BYTECODE{rt_reflect_gi_dxil,sizeof rt_reflect_gi_dxil}:kind==10?D3D12_SHADER_BYTECODE{rt_reflect_gi_path_dxil,sizeof rt_reflect_gi_path_dxil}:
            D3D12_SHADER_BYTECODE{rt_reflect_gi_compose_dxil,sizeof rt_reflect_gi_compose_dxil};
        pso.RTVFormats[0]=DXGI_FORMAT_R16G16B16A16_FLOAT;
        auto& add=pso.BlendState.RenderTarget[0];add.BlendEnable=TRUE;add.DestBlend=D3D12_BLEND_ONE;   // RGB added (the write mask 7)
        pso.DSVFormat=DXGI_FORMAT_UNKNOWN;pso.DepthStencilState.StencilEnable=FALSE;}
    else if(kind==4){pso.PS={rt_reflect_nrd_signal_dxil,sizeof rt_reflect_nrd_signal_dxil};pso.NumRenderTargets=kRtDenoiseInputs;
        for(uint32_t i=0;i<kRtDenoiseInputs;++i){pso.RTVFormats[i]=kRtDenoiseInputFormats[i];pso.BlendState.RenderTarget[i].RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;}
        pso.BlendState.IndependentBlendEnable=TRUE;pso.DSVFormat=DXGI_FORMAT_UNKNOWN;pso.DepthStencilState.StencilEnable=FALSE;}
    // I-0126 R7d: 6 and 7 ReSTIR GI's resampling (no target: their UAVs only), 8 its shading (as 3).
    else if(kind==6 || kind==7){pso.PS=kind==6?D3D12_SHADER_BYTECODE{rt_reflect_gi_temporal_dxil,sizeof rt_reflect_gi_temporal_dxil}:D3D12_SHADER_BYTECODE{rt_reflect_gi_spatial_dxil,sizeof rt_reflect_gi_spatial_dxil};
        pso.NumRenderTargets=0;pso.RTVFormats[0]=DXGI_FORMAT_UNKNOWN;for(auto& target:pso.BlendState.RenderTarget)target.RenderTargetWriteMask=0;
        pso.DSVFormat=DXGI_FORMAT_UNKNOWN;pso.DepthStencilState.StencilEnable=FALSE;}
    else if(kind==8){pso.PS={rt_reflect_gi_shade_dxil,sizeof rt_reflect_gi_shade_dxil};pso.RTVFormats[0]=DXGI_FORMAT_R16G16B16A16_FLOAT;
        auto& add=pso.BlendState.RenderTarget[0];add.BlendEnable=TRUE;add.DestBlend=D3D12_BLEND_ONE;
        pso.DSVFormat=DXGI_FORMAT_UNKNOWN;pso.DepthStencilState.StencilEnable=FALSE;}
    else if(kind==11)pso.PS={rt_reflect_spec_compose_dxil,sizeof rt_reflect_spec_compose_dxil};   // I-0132 R11: kind 0's states
    return pso;}
ID3D12PipelineState* RtReflections::Make(ID3D12Device* device,int kind) {
    auto& made=kind==0?pipeline_:kind==1?hit_:kind==2?water_:kind==3?gi_:kind==4?signal_:kind==5?compose_:kind==6?temporal_:kind==7?spatial_:kind==8?shade_:kind==9?hit_path_:kind==10?gi_path_:kind==12?ser_apply_:spec_compose_;
    MakeRoot(device);   // the root and the cache's stand-by buffer (Bind binds it), whatever the pipeline
    if(made)return made.Get();
    const auto pso=Describe(kind,root_.Get());
    Check(Pipelines().Graphics(device,pso,made.ReleaseAndGetAddressOf()),kind==0?"rt reflections pipeline":kind==1?"rt reflections hit pipeline":kind==2?"rt water pipeline":
        kind==3?"rt gi pipeline":kind==4?"rt nrd signal pipeline":kind==5?"rt gi compose pipeline":kind==6?"rt gi temporal pipeline":kind==7?"rt gi spatial pipeline":kind==8?"rt gi shade pipeline":
        kind==9?"rt reflections hit path pipeline":kind==10?"rt gi path pipeline":kind==12?"rt ser apply pipeline":"rt spec compose pipeline");
    return made.Get();
}
// I-0132 R12d: SER's state object as the warm-up made it, for MakeSer (below).  MakeSer waits while the warm-up builds it (`ser_warm_busy`), so it is never compiled twice; once MakeSer has asked (`ser_warm_taken`)
// the warm-up no longer makes one.
// The slot never destroyed at the module's unload (the step review's M1: DropWarmed releases it before the device closes).
namespace {std::mutex ser_warm_lock;std::condition_variable ser_warm_cv;bool ser_warm_busy{},ser_warm_taken{};ComPtr<ID3D12StateObject>& ser_warmed=*new ComPtr<ID3D12StateObject>;ID3D12RootSignature* ser_warmed_root{};}
void RtReflections::DropWarmed() noexcept {const std::lock_guard hold(ser_warm_lock);ser_warmed.Reset();ser_warmed_root=nullptr;ser_warm_taken=true;}
// I-0132 R12d: every kind made once through the device's pipeline store at start (a worker thread, while the game boots), so a kind's
// first frame (the PT level's paths and ReSTIR GI's draws at a live switch: a 10 to 12 s freeze in I-0132 R12g's runs) finds it in the
// store's memory: no member written (Make keeps its own), a refusal skipped (Make reports it as before). `done` counted per kind tried.
uint32_t RtReflections::Warm(ID3D12Device* device,const std::atomic<bool>& cancel,std::atomic<uint32_t>& done,bool ser) noexcept {
    uint32_t made{};
    try{Microsoft::WRL::ComPtr<ID3D12RootSignature> root;
        if(FAILED(Pipelines().Root(device,rt_reflect_trace_dxil,sizeof rt_reflect_trace_dxil,root.ReleaseAndGetAddressOf())) || !root)return 0;
        for(int kind=0;kind<kWarmKinds && !cancel.load(std::memory_order_relaxed);++kind){if(kind==kSerApplyKind && !ser)continue;   // SER's apply only with SER (WarmTotal)
            Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline;
            if(SUCCEEDED(Pipelines().Graphics(device,Describe(kind,root.Get()),pipeline.ReleaseAndGetAddressOf())) && pipeline)++made;
            done.fetch_add(1,std::memory_order_relaxed);}}
    catch(...){}
    return made;}
// SER's state object, kept for MakeSer (the root is the store's: MakeRoot gets the same object); a refusal leaves MakeSer to try. Called
// after RtLocalLights::Warm (the final review's L4: its 8 s on a cold driver cache last).
uint32_t RtReflections::WarmSer(ID3D12Device* device,const std::atomic<bool>& cancel,std::atomic<uint32_t>& done) noexcept {
    uint32_t made{};
    try{Microsoft::WRL::ComPtr<ID3D12RootSignature> root;
        if(SUCCEEDED(Pipelines().Root(device,rt_reflect_trace_dxil,sizeof rt_reflect_trace_dxil,root.ReleaseAndGetAddressOf())) && root){
            bool build=false;{const std::lock_guard hold(ser_warm_lock);build=!ser_warm_taken && !cancel.load(std::memory_order_relaxed);ser_warm_busy=build;}
        if(build){ComPtr<ID3D12StateObject> state;try{state=CreateSerState(device,root.Get());}catch(...){}
            {const std::lock_guard hold(ser_warm_lock);if(state){ser_warmed=std::move(state);ser_warmed_root=root.Get();++made;}ser_warm_busy=false;}
            ser_warm_cv.notify_all();}
        }}
    catch(...){}
    done.fetch_add(1,std::memory_order_relaxed);
    return made;}
void RtReflections::Bind(ID3D12GraphicsCommandList* list,BindlessHeap& heap,const std::function<UploadArena::Range(uint64_t)>& upload,D3D12_GPU_VIRTUAL_ADDRESS scene,
    D3D12_GPU_VIRTUAL_ADDRESS table,D3D12_GPU_VIRTUAL_ADDRESS rows,D3D12_GPU_VIRTUAL_ADDRESS lights,D3D12_GPU_VIRTUAL_ADDRESS regions,D3D12_GPU_VIRTUAL_ADDRESS sets,D3D12_GPU_VIRTUAL_ADDRESS fogs,
    RtReflectConstants constants,bool compute) {
    // No top level: the traced bit cleared, so the passes give nothing (I-0132 R11e: the cubes removed; such a frame is not shown, its
    // world black); the top level's slot bound to the regions' buffer, never read; no table, never read. R7b: no top level or table, no cache read.
    // I-0132 R11d (D-0262): a traced frame's rays that meet nothing bring nothing.
    if(!scene)constants.mode&=~uint32_t(kRtReflectTraced|kRtReflectCache);
    constants.materials=scene && table && rows?1:0;if(!constants.materials)constants.mode&=~uint32_t(kRtReflectTraced|kRtReflectCache);
    // I-0128 R8c: the light grid, its words right after the list's records in the same upload (SceneRenderer::UploadRtLights), read only
    // with kRtReflectLightGrid (cleared without a list).
    const bool grid=lights && constants.lights && (constants.mode&kRtReflectLightGrid);if(!grid)constants.mode&=~uint32_t(kRtReflectLightGrid);
    constexpr uint32_t entries=kRtCacheEntries;std::memcpy(&constants.previous[3],&entries,4);   // R7b: the cache's capacity, the shader's (step review L3)
    const auto block=upload((sizeof constants+255)&~size_t(255));std::memcpy(block.cpu,&constants,sizeof constants);   // 432 bytes since I-0132 R11b (512 uploaded)
    ID3D12DescriptorHeap* heaps[]{heap.Heap()};list->SetDescriptorHeaps(1,heaps);
    // The same parameters on the graphics or, R7b's resolve, the compute root.
    const auto cbv=[&](UINT i,D3D12_GPU_VIRTUAL_ADDRESS a){if(compute)list->SetComputeRootConstantBufferView(i,a);else list->SetGraphicsRootConstantBufferView(i,a);};
    const auto srv=[&](UINT i,D3D12_GPU_VIRTUAL_ADDRESS a){if(compute)list->SetComputeRootShaderResourceView(i,a);else list->SetGraphicsRootShaderResourceView(i,a);};
    const auto uav=[&](UINT i,D3D12_GPU_VIRTUAL_ADDRESS a){if(compute)list->SetComputeRootUnorderedAccessView(i,a);else list->SetGraphicsRootUnorderedAccessView(i,a);};
    if(compute)list->SetComputeRootSignature(root_.Get());else list->SetGraphicsRootSignature(root_.Get());
    cbv(0,block.gpu);
    srv(1,scene?scene:regions);
    srv(2,constants.materials?table:regions);srv(3,constants.materials?rows:regions);
    srv(4,lights?lights:regions);srv(5,regions);srv(6,sets);
    srv(7,fogs);   // R6d: the hits' fog
    for(UINT i=0;i<3;++i)uav(8+i,(cache_[i]?cache_[i]:cache_none_)->GetGPUVirtualAddress());   // R7b: the cache, else the small buffer (MakeRoot)
    // I-0126 R7d: ReSTIR GI's buffers bound only by its own draws (constants.restir's pitch set: EncodeGiRestir), else the stand-by buffer
    // (the fixes' review L-c: only its frames hold them).
    for(UINT i=0;i<2;++i)uav(11+i,(constants.restir[0] && restir_[i]?restir_[i]:cache_none_)->GetGPUVirtualAddress());
    srv(13,grid?lights+sizeof(RtLight)*constants.lights:regions);   // I-0128 R8c: t7 (never read without the grid)
    if(!compute){list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);list->IASetVertexBuffers(0,0,nullptr);list->IASetIndexBuffer(nullptr);}
}
// I-0126 R7b: the root and the small buffer bound in the cache's place until the cache is made (step review L2), once.
void RtReflections::MakeRoot(ID3D12Device* device) {   // Make's and MakeCache's (the final review's L3)
    if(!root_)Check(Pipelines().Root(device,rt_reflect_trace_dxil,sizeof rt_reflect_trace_dxil,root_.ReleaseAndGetAddressOf()),"rt reflections root");
    if(!cache_none_){const auto heap=HeapOf(D3D12_HEAP_TYPE_DEFAULT);const auto desc=BufferDesc(256,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(cache_none_.ReleaseAndGetAddressOf())),"rt cache stand-by");}
}
// The cache's three buffers (kRtCacheEntries entries of 8, 16 and 16 bytes, UAV-capable, created in COMMON: a buffer is promoted to
// UNORDERED_ACCESS by its first use in a list and decays back after it), made on the first GI frame.
void RtReflections::MakeCache(ID3D12Device* device) {
    MakeRoot(device);
    if(cache_[0] && cache_[1] && cache_[2])return;
    const auto heap=HeapOf(D3D12_HEAP_TYPE_DEFAULT);constexpr uint64_t strides[3]{8,16,16};
    static constexpr const char* what[3]{"rt cache keys","rt cache accumulation","rt cache cells"};
    for(uint32_t i=0;i<3;++i){const auto desc=BufferDesc(strides[i]*kRtCacheEntries,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(cache_[i].ReleaseAndGetAddressOf())),what[i]);}
    cache_cleared_=false;previous_set_=false;
}
void RtReflections::EncodeCache(ID3D12Device* device,ID3D12GraphicsCommandList* list,BindlessHeap& heap,const std::function<UploadArena::Range(uint64_t)>& upload,
    D3D12_GPU_VIRTUAL_ADDRESS scene,D3D12_GPU_VIRTUAL_ADDRESS table,D3D12_GPU_VIRTUAL_ADDRESS rows,D3D12_GPU_VIRTUAL_ADDRESS lights,
    D3D12_GPU_VIRTUAL_ADDRESS regions,D3D12_GPU_VIRTUAL_ADDRESS sets,D3D12_GPU_VIRTUAL_ADDRESS fogs,RtReflectConstants constants) {
    if(!device || !list || !heap.IsOpen() || !constants.width || !constants.height || !scene || !table || !rows || !regions || !sets || !fogs)throw std::invalid_argument("rt cache input");
    MakeCache(device);
    if(!update_){D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};pso.pRootSignature=root_.Get();
        pso.VS={rt_reflect_vs_dxil,sizeof rt_reflect_vs_dxil};pso.PS={rt_reflect_cache_update_dxil,sizeof rt_reflect_cache_update_dxil};
        for(auto& target:pso.BlendState.RenderTarget){target.LogicOp=D3D12_LOGIC_OP_NOOP;target.RenderTargetWriteMask=0;
            target.SrcBlend=target.SrcBlendAlpha=D3D12_BLEND_ONE;target.DestBlend=target.DestBlendAlpha=D3D12_BLEND_ZERO;target.BlendOp=target.BlendOpAlpha=D3D12_BLEND_OP_ADD;}
        pso.SampleMask=UINT_MAX;pso.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;pso.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;
        pso.RasterizerState.DepthClipEnable=TRUE;pso.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;pso.SampleDesc.Count=1;
        pso.NumRenderTargets=0;pso.DepthStencilState.DepthEnable=FALSE;pso.DepthStencilState.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ZERO;   // no target: its UAVs only
        Check(Pipelines().Graphics(device,pso,update_.ReleaseAndGetAddressOf()),"rt cache update pipeline");}
    if(!resolve_){D3D12_COMPUTE_PIPELINE_STATE_DESC pso{};pso.pRootSignature=root_.Get();pso.CS={rt_reflect_cache_resolve_dxil,sizeof rt_reflect_cache_resolve_dxil};
        Check(Pipelines().Compute(device,pso,resolve_.ReleaseAndGetAddressOf()),"rt cache resolve pipeline");}
    // The camera's position at the last resolve (SHaRC's blend of adjacent levels when it moves): this frame's on the first. A camera
    // more than kRtCacheJump metres from it (a warp, a map change, a load between GI frames) clears the cache first: no cell of the
    // place left answers at once for a new one (step review L6).
    if(previous_set_){const float dx=constants.position[0]-previous_[0],dy=constants.position[1]-previous_[1],dz=constants.position[2]-previous_[2];
        if(!(dx*dx+dy*dy+dz*dz<=kRtCacheJump*kRtCacheJump))cache_cleared_=false;}
    if(!previous_set_ || !cache_cleared_){previous_=constants.position;previous_set_=true;}
    // The update reads the cells the last resolve left (its last hit's ambient, SHaRC's resampling): kRtReflectCache on (zeros after the clear).
    constants.previous=previous_;constants.mode=(constants.mode&~uint32_t(kRtReflectCacheClear))|kRtReflectCache;
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;   // every UAV access (the three buffers)
    constexpr UINT groups=kRtCacheEntries/kRtCacheGroup;
    if(!cache_cleared_){auto clear=constants;clear.mode|=kRtReflectCacheClear;
        Bind(list,heap,upload,scene,table,rows,lights,regions,sets,fogs,clear,true);list->SetPipelineState(resolve_.Get());list->Dispatch(groups,1,1);
        list->ResourceBarrier(1,&barrier);cache_cleared_=true;}
    Bind(list,heap,upload,scene,table,rows,lights,regions,sets,fogs,constants);
    const UINT columns=(constants.width+kRtCacheBlock-1)/kRtCacheBlock,lines=(constants.height+kRtCacheBlock-1)/kRtCacheBlock;
    const D3D12_VIEWPORT viewport{0,0,float(columns),float(lines),0,1};const D3D12_RECT scissor{0,0,LONG(columns),LONG(lines)};
    list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);list->OMSetRenderTargets(0,nullptr,FALSE,nullptr);
    list->SetPipelineState(update_.Get());list->DrawInstanced(3,1,0,0);
    list->ResourceBarrier(1,&barrier);
    Bind(list,heap,upload,scene,table,rows,lights,regions,sets,fogs,constants,true);list->SetPipelineState(resolve_.Get());list->Dispatch(groups,1,1);
    list->ResourceBarrier(1,&barrier);
    previous_=constants.position;
}
ID3D12Resource* RtReflections::EncodeProbes(ID3D12Device* device,ID3D12GraphicsCommandList* list,BindlessHeap& heap,const std::function<UploadArena::Range(uint64_t)>& upload,
    D3D12_GPU_VIRTUAL_ADDRESS scene,D3D12_GPU_VIRTUAL_ADDRESS table,D3D12_GPU_VIRTUAL_ADDRESS rows,D3D12_GPU_VIRTUAL_ADDRESS lights,
    D3D12_GPU_VIRTUAL_ADDRESS regions,D3D12_GPU_VIRTUAL_ADDRESS sets,D3D12_GPU_VIRTUAL_ADDRESS fogs,RtReflectConstants constants,
    std::span<const RtProbeRequest> requests,uint32_t slot,uint64_t completed) {
    if(!device || !list || !heap.IsOpen() || !scene || !table || !rows || !regions || !sets || !fogs || requests.empty() || requests.size()>kRtProbesMost ||
       slot>=probe_slots_.size())throw std::invalid_argument("rt probes input");
    MakeRoot(device);
    if(!probe_){D3D12_COMPUTE_PIPELINE_STATE_DESC pso{};pso.pRootSignature=root_.Get();pso.CS={rt_reflect_probe_dxil,sizeof rt_reflect_probe_dxil};
        Check(Pipelines().Compute(device,pso,probe_.ReleaseAndGetAddressOf()),"rt probes pipeline");}
    auto& own=probe_slots_[slot];constexpr uint64_t bytes=uint64_t(kRtProbesMost)*sizeof(RtProbeResult);
    if(!own.results){const auto heap_type=HeapOf(D3D12_HEAP_TYPE_DEFAULT);const auto desc=BufferDesc(bytes,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        Check(device->CreateCommittedResource(&heap_type,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(own.results.ReleaseAndGetAddressOf())),"rt probes results");}
    if(!own.readback){const auto heap_type=HeapOf(D3D12_HEAP_TYPE_READBACK);const auto desc=BufferDesc(bytes,D3D12_RESOURCE_FLAG_NONE);
        Check(device->CreateCommittedResource(&heap_type,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(own.readback.ReleaseAndGetAddressOf())),"rt probes readback");}
    for(auto* view:{&own.requests_view,&own.results_view})if(*view==UINT32_MAX){*view=heap.Allocate(completed);if(*view==BindlessIndices::none){*view=UINT32_MAX;throw std::runtime_error("rt probes slots");}
        if(view==&own.results_view){D3D12_UNORDERED_ACCESS_VIEW_DESC u{};u.Format=DXGI_FORMAT_R32_TYPELESS;u.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;
            u.Buffer.NumElements=UINT(bytes/4);u.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;device->CreateUnorderedAccessView(own.results.Get(),nullptr,&u,heap.Cpu(*view));}}
    // The requests in this frame's upload, seen through this slot's view (written again only once the slot's last frame completed).
    const uint64_t request_bytes=requests.size()*sizeof(RtProbeRequest);
    const auto range=upload((request_bytes+255)&~uint64_t(255));std::memcpy(range.cpu,requests.data(),request_bytes);
    if(!range.resource || range.offset%D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT)throw std::logic_error("rt probes upload");
    {D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Format=DXGI_FORMAT_R32_TYPELESS;v.ViewDimension=D3D12_SRV_DIMENSION_BUFFER;v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        v.Buffer.FirstElement=range.offset/4;v.Buffer.NumElements=UINT(request_bytes/4);v.Buffer.Flags=D3D12_BUFFER_SRV_FLAG_RAW;
        device->CreateShaderResourceView(range.resource,&v,heap.Cpu(own.requests_view));}
    constants.probe={own.requests_view,own.results_view,uint32_t(requests.size()),0};
    Bind(list,heap,upload,scene,table,rows,lights,regions,sets,fogs,constants,true);
    list->SetPipelineState(probe_.Get());list->Dispatch(UINT(requests.size()),1,1);
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;barrier.Transition.pResource=own.results.Get();
    barrier.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;barrier.Transition.StateBefore=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_SOURCE;list->ResourceBarrier(1,&barrier);
    list->CopyBufferRegion(own.readback.Get(),0,own.results.Get(),0,requests.size()*sizeof(RtProbeResult));
    std::swap(barrier.Transition.StateBefore,barrier.Transition.StateAfter);barrier.Transition.StateAfter=D3D12_RESOURCE_STATE_COMMON;list->ResourceBarrier(1,&barrier);
    return own.readback.Get();
}
// I-0126 R7d: ReSTIR GI's two buffers at this extent (RTXDI's layout: 16 x 16 pixel blocks, an array a block row pitch times the block
// rows), made again on a new extent (the former ones held by the frames that used them). I-0128 R8d: with `initial`, the surfaces'
// buffer a third part, the frame's initial estimates (a decorrelated share, rt_reflect.hlsl's gi_shade).
bool RtReflections::MakeRestir(ID3D12Device* device,uint32_t width,uint32_t height,bool initial) {
    if(!device || !width || !height)throw std::invalid_argument("rt restir extent");
    MakeRoot(device);
    if(restir_[0] && restir_[1] && restir_width_==width && restir_height_==height && restir_initial_==initial)return false;
    constexpr uint32_t block=16;
    const uint32_t row=((width+block-1)/block)*block*block,array=row*((height+block-1)/block);
    const auto heap=HeapOf(D3D12_HEAP_TYPE_DEFAULT);
    const uint64_t sizes[2]{uint64_t(array)*2*32,uint64_t(width)*height*(initial?3:2)*16};
    static constexpr const char* what[2]{"rt restir reservoirs","rt restir surfaces"};
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,2> made;
    for(uint32_t i=0;i<2;++i){const auto desc=BufferDesc(sizes[i],D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(made[i].ReleaseAndGetAddressOf())),what[i]);}
    restir_=std::move(made);restir_width_=width;restir_height_=height;restir_row_pitch_=row;restir_array_pitch_=array;restir_initial_=initial;
    return true;
}
// I-0132 R11b (R11-PLAN §13): SER's state object and shader table (once), its scratch targets and slots at this extent.
// I-0132 R12d: SER's state object (its two ray generation exports on the reflections' root), outside the pipeline store: MakeSer's, and
// the warm-up's (Warm below), which keeps the one it made for MakeSer (`ser_warmed`, under its lock, for that root): its first creation
// took 8.3 s at the first PT frame of `r12d-pt1` (a cold driver cache) once the store's pipelines were warmed.
// (Its warm-up slot, ser_warm_lock and the rest, is declared above RtReflections::Warm.)
ComPtr<ID3D12StateObject> RtReflections::CreateSerState(ID3D12Device* device,ID3D12RootSignature* root) {
    ComPtr<ID3D12Device5> device5;Check(device->QueryInterface(IID_PPV_ARGS(&device5)),"rt ser device5");
    D3D12_EXPORT_DESC exports[2]{{L"trace_hit_ser",nullptr,D3D12_EXPORT_FLAG_NONE},{L"gi_temporal_ser",nullptr,D3D12_EXPORT_FLAG_NONE}};
    D3D12_DXIL_LIBRARY_DESC library{{rt_reflect_ser_dxil,sizeof rt_reflect_ser_dxil},2,exports};
    D3D12_RAYTRACING_SHADER_CONFIG shader{4,8};   // no TraceRay (inline queries only): the smallest payload and the triangles' attributes
    D3D12_GLOBAL_ROOT_SIGNATURE global{root};D3D12_RAYTRACING_PIPELINE_CONFIG config{1};
    const D3D12_STATE_SUBOBJECT subobjects[4]{{D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,&library},{D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG,&shader},
        {D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE,&global},{D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG,&config}};
    const D3D12_STATE_OBJECT_DESC desc{D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,4,subobjects};
    ComPtr<ID3D12StateObject> made;Check(device5->CreateStateObject(&desc,IID_PPV_ARGS(&made)),"rt ser state object");return made;}
void RtReflections::MakeSer(ID3D12Device* device,BindlessHeap& heap,uint32_t width,uint32_t height,uint64_t completed,uint64_t submitted) {
    if(!device || !heap.IsOpen() || !width || !height)throw std::invalid_argument("rt ser input");
    MakeRoot(device);
    if(!ser_state_){
        ComPtr<ID3D12StateObject> made;
        {std::unique_lock hold(ser_warm_lock);ser_warm_taken=true;ser_warm_cv.wait(hold,[]{return !ser_warm_busy;});
            if(ser_warmed && ser_warmed_root==root_.Get())made=std::move(ser_warmed);ser_warmed.Reset();ser_warmed_root=nullptr;}
        if(!made)made=CreateSerState(device,root_.Get());
        ComPtr<ID3D12StateObjectProperties> properties;Check(made.As(&properties),"rt ser properties");
        const void* ids[2]{properties->GetShaderIdentifier(L"trace_hit_ser"),properties->GetShaderIdentifier(L"gi_temporal_ser")};
        if(!ids[0] || !ids[1])throw std::runtime_error("rt ser shader identifiers");
        const auto heap_type=HeapOf(D3D12_HEAP_TYPE_UPLOAD);const auto table_desc=BufferDesc(2*D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT,D3D12_RESOURCE_FLAG_NONE);
        ComPtr<ID3D12Resource> table;
        Check(device->CreateCommittedResource(&heap_type,D3D12_HEAP_FLAG_NONE,&table_desc,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&table)),"rt ser shader table");
        void* mapped=nullptr;const D3D12_RANGE none{0,0};Check(table->Map(0,&none,&mapped),"rt ser shader table map");
        for(int i=0;i<2;++i)std::memcpy(static_cast<unsigned char*>(mapped)+i*D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT,ids[i],D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        table->Unmap(0,nullptr);
        Make(device,12);   // the step review's M1: the apply draw's pipeline with the state object (a refusal here turns SER off, not the reflections)
        ser_state_=std::move(made);ser_table_=std::move(table);}
    if(ser_targets_[0] && ser_targets_[1] && ser_width_==width && ser_height_==height)return;
    // The step review's L1: the colour in trace_hit's own target format (R11G11B10), the distance in RR's (R16): the copy is exact.
    constexpr DXGI_FORMAT formats[2]{DXGI_FORMAT_R11G11B10_FLOAT,kRrHitDistanceFormat};
    for(const auto format:formats){D3D12_FEATURE_DATA_FORMAT_SUPPORT support{format};
        if(FAILED(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT,&support,sizeof support)) || !(support.Support2&D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE))
            throw std::runtime_error("rt ser target format");}
    std::array<ComPtr<ID3D12Resource>,2> targets;std::array<uint32_t,4> slots{UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX};
    try{const auto heap_type=HeapOf(D3D12_HEAP_TYPE_DEFAULT);
        for(int i=0;i<2;++i){D3D12_RESOURCE_DESC image{};image.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;image.Width=width;image.Height=height;image.DepthOrArraySize=1;
            image.MipLevels=1;image.Format=formats[i];image.SampleDesc.Count=1;image.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            Check(device->CreateCommittedResource(&heap_type,D3D12_HEAP_FLAG_NONE,&image,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&targets[i])),"rt ser target");}
        for(auto& slot:slots){slot=heap.Allocate(completed);if(slot==BindlessIndices::none){slot=UINT32_MAX;throw std::runtime_error("rt ser slots");}}
        for(int i=0;i<2;++i){D3D12_UNORDERED_ACCESS_VIEW_DESC u{};u.Format=formats[i];u.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
            device->CreateUnorderedAccessView(targets[i].Get(),nullptr,&u,heap.Cpu(slots[i]));
            D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Format=formats[i];v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            v.Texture2D.MipLevels=1;device->CreateShaderResourceView(targets[i].Get(),&v,heap.Cpu(slots[2+i]));}
    }catch(...){for(auto slot:slots)if(slot!=UINT32_MAX){try{heap.Release(slot,submitted);}catch(...){}}throw;}
    ReleaseSerViews(heap,submitted);   // the former extent's slots, after the frames that read them
    ser_targets_=std::move(targets);ser_slots_=slots;ser_width_=width;ser_height_=height;
}
void RtReflections::HoldSer(std::vector<std::shared_ptr<const void>>& kept) const {for(const auto& target:ser_targets_)if(target)kept.push_back(HoldResource(target.Get()));}
// One ray generation record of SER's table (0 trace_hit_ser, 1 gi_temporal_ser) over width x height, on the compute root Bind set.
void RtReflections::DispatchSer(ID3D12GraphicsCommandList4* list4,uint32_t record,uint32_t width,uint32_t height) {
    list4->SetPipelineState1(ser_state_.Get());ser_dispatches_.fetch_add(1,std::memory_order_relaxed);
    D3D12_DISPATCH_RAYS_DESC rays{};rays.RayGenerationShaderRecord={ser_table_->GetGPUVirtualAddress()+record*D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT,D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES};
    rays.Width=width;rays.Height=height;rays.Depth=1;list4->DispatchRays(&rays);
}
// The scratch targets to PIXEL_SHADER_RESOURCE after the dispatch (`to_read`), back to UNORDERED_ACCESS after the apply draw.
void RtReflections::SerBarrier(ID3D12GraphicsCommandList* list,bool to_read) {
    D3D12_RESOURCE_BARRIER barriers[2]{};
    for(int i=0;i<2;++i){auto& b=barriers[i];b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition.pResource=ser_targets_[i].Get();
        b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        b.Transition.StateBefore=to_read?D3D12_RESOURCE_STATE_UNORDERED_ACCESS:D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        b.Transition.StateAfter=to_read?D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:D3D12_RESOURCE_STATE_UNORDERED_ACCESS;}
    list->ResourceBarrier(2,barriers);
}
void RtReflections::HoldRestir(std::vector<std::shared_ptr<const void>>& kept) const {for(const auto& buffer:restir_)if(buffer)kept.push_back(HoldResource(buffer.Get()));}
void RtReflections::EncodeGiRestir(ID3D12Device* device,ID3D12GraphicsCommandList* list,BindlessHeap& heap,const std::function<UploadArena::Range(uint64_t)>& upload,
    D3D12_GPU_VIRTUAL_ADDRESS scene,D3D12_GPU_VIRTUAL_ADDRESS table,D3D12_GPU_VIRTUAL_ADDRESS rows,D3D12_GPU_VIRTUAL_ADDRESS lights,
    D3D12_GPU_VIRTUAL_ADDRESS regions,D3D12_GPU_VIRTUAL_ADDRESS sets,D3D12_GPU_VIRTUAL_ADDRESS fogs,RtReflectConstants constants,D3D12_CPU_DESCRIPTOR_HANDLE rtv) {
    if(!device || !list || !heap.IsOpen() || !constants.width || !constants.height || !regions || !sets || !fogs || !rtv.ptr || constants.gb2==UINT32_MAX ||
       !restir_[0] || !restir_[1] || restir_width_!=constants.width || restir_height_!=constants.height ||
       (RtRestirShareOf(constants.restir[2]) && !restir_initial_))throw std::invalid_argument("rt gi restir input");   // I-0128 R8d: a share needs the third part
    auto* const temporal=Make(device,6);auto* const spatial=Make(device,7);auto* const shade=Make(device,8);
    constants.restir[0]=restir_row_pitch_;constants.restir[1]=restir_array_pitch_;constants.restir[3]=constants.width*constants.height;
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;   // every UAV access (both buffers)
    // I-0132 R11b: the initial sample's path build reordered (SER), its reservoirs written as gi_temporal's.
    ComPtr<ID3D12GraphicsCommandList4> list4;   // the step review's M1: prepared first; a failure keeps the pixel shader this frame, reported
    if(RtPathsOf(constants) && constants.ser[0]!=UINT32_MAX){
        try{if(!ser_state_)throw std::invalid_argument("rt gi restir ser input");Check(list->QueryInterface(IID_PPV_ARGS(&list4)),"rt ser list4");}
        catch(const std::exception& error){list4.Reset();try{if(ser_failure_.empty())ser_failure_=*error.what()?error.what():"unknown";}catch(...){}}}
    if(list4){
        Bind(list,heap,upload,scene,table,rows,lights,regions,sets,fogs,constants,true);DispatchSer(list4.Get(),1,constants.width,constants.height);list->ResourceBarrier(1,&barrier);
        Bind(list,heap,upload,scene,table,rows,lights,regions,sets,fogs,constants);list->OMSetRenderTargets(0,nullptr,FALSE,nullptr);}
    else{Bind(list,heap,upload,scene,table,rows,lights,regions,sets,fogs,constants);
        list->OMSetRenderTargets(0,nullptr,FALSE,nullptr);
        list->SetPipelineState(temporal);list->DrawInstanced(3,1,0,0);list->ResourceBarrier(1,&barrier);}
    list->SetPipelineState(spatial);list->DrawInstanced(3,1,0,0);list->ResourceBarrier(1,&barrier);
    list->OMSetRenderTargets(1,&rtv,FALSE,nullptr);
    list->SetPipelineState(shade);list->DrawInstanced(3,1,0,0);
}
void RrGuides::Encode(ID3D12Device* device,ID3D12GraphicsCommandList* list,BindlessHeap& heap,const std::function<UploadArena::Range(uint64_t)>& upload,
    const RrGuidesConstants& constants,const D3D12_CPU_DESCRIPTOR_HANDLE rtvs[3]) {
    if(!device || !list || !heap.IsOpen() || !constants.width || !constants.height || !rtvs[0].ptr || !rtvs[1].ptr || !rtvs[2].ptr)
        throw std::invalid_argument("rr guides input");
    if(!pipeline_){
        Check(Pipelines().Root(device,rr_guides_guide_dxil,sizeof rr_guides_guide_dxil,root_.ReleaseAndGetAddressOf()),"rr guides root");
        // Written, not blended: every channel of the three targets; no depth or stencil.
        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};pso.pRootSignature=root_.Get();
        pso.VS={rr_guides_vs_dxil,sizeof rr_guides_vs_dxil};pso.PS={rr_guides_guide_dxil,sizeof rr_guides_guide_dxil};
        for(auto& target:pso.BlendState.RenderTarget){target.LogicOp=D3D12_LOGIC_OP_NOOP;target.RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;
            target.SrcBlend=target.SrcBlendAlpha=D3D12_BLEND_ONE;target.DestBlend=target.DestBlendAlpha=D3D12_BLEND_ZERO;target.BlendOp=target.BlendOpAlpha=D3D12_BLEND_OP_ADD;}
        pso.SampleMask=UINT_MAX;pso.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;pso.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;
        pso.RasterizerState.DepthClipEnable=TRUE;pso.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;pso.SampleDesc.Count=1;
        pso.NumRenderTargets=3;for(uint32_t i=0;i<3;++i)pso.RTVFormats[i]=kRrGuideFormats[i];
        pso.DepthStencilState.DepthEnable=FALSE;pso.DepthStencilState.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ZERO;
        Check(Pipelines().Graphics(device,pso,pipeline_.ReleaseAndGetAddressOf()),"rr guides pipeline");}
    const auto block=upload(256);std::memcpy(block.cpu,&constants,sizeof constants);
    ID3D12DescriptorHeap* heaps[]{heap.Heap()};list->SetDescriptorHeaps(1,heaps);
    list->SetGraphicsRootSignature(root_.Get());list->SetGraphicsRootConstantBufferView(0,block.gpu);
    list->OMSetRenderTargets(3,rtvs,FALSE,nullptr);
    list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);list->IASetVertexBuffers(0,0,nullptr);list->IASetIndexBuffer(nullptr);
    list->SetPipelineState(pipeline_.Get());list->DrawInstanced(3,1,0,0);
}

}
