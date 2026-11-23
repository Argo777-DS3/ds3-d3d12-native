#include "model_draw.h"
#include "backend.h"
#include "../frame/collect_profile.h"
#include <atomic>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace ds3native {
using Microsoft::WRL::ComPtr;
namespace {
constexpr UINT cbv_count=14,srv_count=128,sampler_count=16;
constexpr UINT stage_resources=cbv_count+srv_count;
// C4-1b: the bounded registers (model_draw.h) are descriptor CBVs in the stage tables, the others root CBVs.
constexpr bool BoundedConstantRegister(UINT stage,UINT slot){return NativeModelBoundedConstant(stage,slot);}
constexpr UINT root_constant_base=4;   // after the four stage tables
// The root parameter of a root-CBV register: the vertex registers without b5, b6, b7, b12, then the pixel ones without b4.
constexpr UINT RootConstantIndex(UINT stage,UINT slot){
    UINT index=root_constant_base;for(UINT s=0;s<=stage;++s)for(UINT r=0;r<cbv_count && (s<stage || r<slot);++r)if(!BoundedConstantRegister(s,r))++index;
    return index;}
constexpr UINT root_constant_count=RootConstantIndex(1,cbv_count)-root_constant_base;   // 10 + 13 = 23 (50 of 64 root DWORDs)
static_assert(RootConstantIndex(0,0)==4 && RootConstantIndex(0,8)==9 && RootConstantIndex(1,0)==14 && root_constant_count==23);
// W7: the tess root signature appends the hull and domain registers (NativeModelTessConstant) as root CBVs: HS b0, b5,
// b6, DS b0, b5, b6, b8 (64 of 64 root DWORDs; b0 is the depth passes' cbMatrixWVP, Dpt_PN).
constexpr UINT tess_constant_base=root_constant_base+root_constant_count,tess_constant_count=7;
constexpr std::pair<UINT,UINT> tess_registers[tess_constant_count]{{2,0},{2,5},{2,6},{3,0},{3,5},{3,6},{3,8}};
static_assert(root_constant_base+2*(root_constant_count+tess_constant_count)<=64,"tess root DWORD budget");
UINT TessRootIndex(UINT stage,UINT slot){
    for(UINT i=0;i<tess_constant_count;++i)if(tess_registers[i].first==stage && tess_registers[i].second==slot)return tess_constant_base+i;
    throw std::invalid_argument("native model hull/domain register outside the tess root");}
void Require(bool value,const char* why){if(!value)throw std::invalid_argument(why);}
void Check(HRESULT result,const char* why){if(FAILED(result))throw std::runtime_error(why);}
// 0 vertex, 1 pixel, 2 hull, 3 domain (W7).
UINT Stage(ds3content::ShaderStage stage){
    switch(stage){case ds3content::ShaderStage::Vertex:return 0u;case ds3content::ShaderStage::Pixel:return 1u;
        case ds3content::ShaderStage::Hull:return 2u;case ds3content::ShaderStage::Domain:return 3u;default:break;}
    throw std::invalid_argument("native model binding stage");
}
D3D12_CPU_DESCRIPTOR_HANDLE CPU(ID3D12DescriptorHeap* heap,UINT stride,UINT slot){
    auto result=heap->GetCPUDescriptorHandleForHeapStart();result.ptr+=SIZE_T(stride)*slot;return result;
}
D3D12_GPU_DESCRIPTOR_HANDLE GPU(ID3D12DescriptorHeap* heap,UINT stride,UINT slot){
    auto result=heap->GetGPUDescriptorHandleForHeapStart();result.ptr+=UINT64(stride)*slot;return result;
}
void SameDevice(ID3D12Device* device,ID3D12Resource* resource){
    ComPtr<IUnknown> expected,actual;Check(device->QueryInterface(IID_PPV_ARGS(&expected)),"native model device identity");
    Check(resource->GetDevice(IID_PPV_ARGS(&actual)),"native model resource device");
    Require(expected.Get()==actual.Get(),"native model resident resource belongs to another device");
}
bool ValidSampler(const D3D12_SAMPLER_DESC& value){
    constexpr UINT filters[]{0,1,4,5,0x10,0x11,0x14,0x15,0x55,0x80,0x81,0x84,0x85,0x90,0x91,0x94,0x95,0xD5};
    if(std::find(std::begin(filters),std::end(filters),UINT(value.Filter))==std::end(filters) ||
       value.AddressU<D3D12_TEXTURE_ADDRESS_MODE_WRAP || value.AddressU>D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE ||
       value.AddressV<D3D12_TEXTURE_ADDRESS_MODE_WRAP || value.AddressV>D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE ||
       value.AddressW<D3D12_TEXTURE_ADDRESS_MODE_WRAP || value.AddressW>D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE ||
       value.MaxAnisotropy<1 || value.MaxAnisotropy>16 || value.ComparisonFunc<D3D12_COMPARISON_FUNC_NEVER ||
       value.ComparisonFunc>D3D12_COMPARISON_FUNC_ALWAYS || !std::isfinite(value.MipLODBias) ||
       value.MipLODBias<D3D12_MIP_LOD_BIAS_MIN || value.MipLODBias>D3D12_MIP_LOD_BIAS_MAX ||
       !std::isfinite(value.MinLOD) || !std::isfinite(value.MaxLOD) || value.MaxLOD<value.MinLOD)return false;
    return std::all_of(std::begin(value.BorderColor),std::end(value.BorderColor),[](float c){return std::isfinite(c) && c>=0.f && c<=1.f;});
}
void ValidateGeometry(ID3D12Device* device,const ModelGeometryBinding& geometry){
    Require(geometry.vertex_resource && geometry.index_resource && geometry.storage_generation &&
        geometry.index_count && geometry.index_count%3==0 && geometry.vertices.BufferLocation &&
        geometry.vertices.StrideInBytes && geometry.vertices.SizeInBytes &&
        geometry.vertices.SizeInBytes%geometry.vertices.StrideInBytes==0 && geometry.indices.BufferLocation &&
        (geometry.indices.Format==DXGI_FORMAT_R16_UINT || geometry.indices.Format==DXGI_FORMAT_R32_UINT) &&
        UINT64(geometry.index_count)*(geometry.indices.Format==DXGI_FORMAT_R32_UINT?4:2)<=geometry.indices.SizeInBytes,
        "native model one-stream indexed triangle geometry");
    SameDevice(device,geometry.vertex_resource.Get());SameDevice(device,geometry.index_resource.Get());
    const auto bounds=[](ID3D12Resource* resource,UINT64 location,UINT bytes){
        const auto desc=resource->GetDesc();const auto base=resource->GetGPUVirtualAddress();
        return desc.Dimension==D3D12_RESOURCE_DIMENSION_BUFFER && base && location>=base &&
            location-base<=desc.Width && bytes<=desc.Width-(location-base);
    };
    Require(bounds(geometry.vertex_resource.Get(),geometry.vertices.BufferLocation,geometry.vertices.SizeInBytes) &&
        bounds(geometry.index_resource.Get(),geometry.indices.BufferLocation,geometry.indices.SizeInBytes),"native model resident geometry view extent");
    if(geometry.vertices2.BufferLocation)Require(geometry.vertices2.StrideInBytes && geometry.vertices2.SizeInBytes%geometry.vertices2.StrideInBytes==0 &&
        geometry.vertices2.SizeInBytes/geometry.vertices2.StrideInBytes==geometry.vertices.SizeInBytes/geometry.vertices.StrideInBytes &&
        bounds(geometry.index_resource.Get(),geometry.vertices2.BufferLocation,geometry.vertices2.SizeInBytes),"native model second stream extent");
    // I-0134: the previous simulated copy of a dynamic slot 0, its layout (BindModelPreviousStream).
    if(geometry.previous.BufferLocation || geometry.previous_resource){Require(geometry.previous_resource && geometry.stream_generation && geometry.stream_owner &&
        geometry.previous.StrideInBytes==geometry.vertices.StrideInBytes && geometry.previous.SizeInBytes==geometry.vertices.SizeInBytes,"native model previous stream layout");
        SameDevice(device,geometry.previous_resource.Get());
        Require(bounds(geometry.previous_resource.Get(),geometry.previous.BufferLocation,geometry.previous.SizeInBytes),"native model previous stream extent");}
}
std::mutex root_shapes_lock;std::vector<std::array<uint16_t,6>> root_shapes;
void ValidateBindings(const NativeModelDrawInput& input,const std::array<std::reference_wrapper<const ds3content::ShaderInterface>,4>& interfaces,bool tess){
    std::array<uint16_t,6> shape{};
    const std::array<const ds3content::ShaderInterface*,4> shaders{&interfaces[0].get(),&interfaces[1].get(),&interfaces[2].get(),&interfaces[3].get()};
    std::array<std::array<bool,cbv_count>,4> constants{};
    std::array<std::array<bool,srv_count>,4> srvs{};
    std::array<std::array<bool,sampler_count>,4> samplers{};
    for(const auto& binding:input.constants){const auto stage=Stage(binding.stage),slot=binding.shader_register;
        Require(slot<cbv_count && !constants[stage][slot] && !binding.Payload().empty() && binding.Payload().size()<=65536,"native model constant binding");
        // W7: a hull or domain register only on a tessellated draw, and only the tess root's ones.
        Require(stage<2 || (tess && NativeModelTessConstant(stage,slot)),"native model hull/domain constant binding");
        // C4-1: a frame register takes only a frame binding (a root CBV), and a frame binding only a frame register.
        Require(binding.frame==(NativeModelFrameRootOf(stage,slot)<FrameRootCount),"native model frame register binding");constants[stage][slot]=true;}
    for(const auto& binding:input.srvs){const auto stage=Stage(binding.stage),slot=binding.shader_register;
        Require(stage<2 && slot<srv_count && !srvs[stage][slot] && binding.descriptor.ptr && binding.resource && binding.owner,"native model SRV binding");srvs[stage][slot]=true;}
    for(const auto& binding:input.samplers){const auto stage=Stage(binding.stage),slot=binding.shader_register;
        Require(stage<2 && slot<sampler_count && !samplers[stage][slot] && ValidSampler(binding.description),"native model sampler binding");samplers[stage][slot]=true;}
    for(UINT stage=0;stage<4;++stage){
        std::array<bool,cbv_count> declared_constants{};std::array<bool,srv_count> declared_srvs{};std::array<bool,sampler_count> declared_samplers{};
        for(const auto& resource:shaders[stage]->resources){
            const auto slot=resource.shader_register,count=resource.count;Require(count,"native model empty RDEF resource range");
            if(resource.input_type==0){
                if(!(count==1 && slot<cbv_count && !declared_constants[slot] && constants[stage][slot])){   // D-0193: which one, for the log
                    char why[160]{};sprintf_s(why,"native model required RDEF constant unavailable (stage %u b%u %.60s)",stage,slot,resource.name.c_str());
                    throw std::invalid_argument(why);}
                declared_constants[slot]=true;const ds3content::ShaderConstantDeclaration* declaration=nullptr;
                for(const auto& candidate:shaders[stage]->constants)if(candidate.name==resource.name){Require(!declaration,"native model ambiguous RDEF constant");declaration=&candidate;}
                Require(declaration && !declaration->buffer_type && declaration->declared_bytes && declaration->declared_bytes<=65536 && declaration->declared_bytes%16==0,"native model unsupported RDEF constant extent");
                const auto binding=std::find_if(input.constants.begin(),input.constants.end(),[&](const auto& value){return Stage(value.stage)==stage && value.shader_register==slot;});
                if(binding==input.constants.end() || binding->Payload().size()<declaration->declared_bytes){   // D-0193: which one, for the log
                    char why[160]{};sprintf_s(why,"native model constant shorter than RDEF declaration (stage %u b%u: %zu < %u bytes)",stage,slot,
                        binding==input.constants.end()?size_t{0}:binding->Payload().size(),static_cast<unsigned>(declaration->declared_bytes));throw std::invalid_argument(why);}
            }else if(resource.input_type==3){
                Require(slot<sampler_count && count<=sampler_count-slot,"native model RDEF sampler extent");
                for(UINT current=slot;current<slot+count;++current){Require(!declared_samplers[current] && samplers[stage][current],"native model required RDEF sampler unavailable");declared_samplers[current]=true;}
            }else{
                Require(resource.input_type==1 || resource.input_type==2 || resource.input_type==5 || resource.input_type==7,"native model unsupported RDEF resource kind");
                Require(slot<srv_count && count<=srv_count-slot,"native model RDEF SRV extent");
                for(UINT current=slot;current<slot+count;++current){Require(!declared_srvs[current] && srvs[stage][current],"native model required RDEF SRV unavailable");declared_srvs[current]=true;}
            }
        }
        if(stage>=2)continue;   // the root-shape statistics keep the vertex and pixel stages
        for(uint16_t slot=0;slot<cbv_count;++slot)if(declared_constants[slot])shape[stage*3]=static_cast<uint16_t>(slot+1);
        for(uint16_t slot=0;slot<srv_count;++slot)if(declared_srvs[slot])shape[stage*3+1]=static_cast<uint16_t>(slot+1);
        for(uint16_t slot=0;slot<sampler_count;++slot)if(declared_samplers[slot])shape[stage*3+2]=static_cast<uint16_t>(slot+1);
    }
    const std::lock_guard hold(root_shapes_lock);
    if(std::find(root_shapes.begin(),root_shapes.end(),shape)==root_shapes.end() && root_shapes.size()<4096)root_shapes.push_back(shape);
}
}

namespace {
// 1121: per-device caches and pools for prepared model draws. Before 1121 every draw of every frame created its root
// signature, pipeline state, two shader-visible descriptor heaps and one committed upload buffer per constant block
// (Native1121: 680 ms of CPU per in-game frame). Pipelines, the root signature, shader interfaces and format queries
// are now created once per distinct input; descriptor tables are suballocated from pooled chunks that return to their
// pool only when every draw holding them is destroyed (after the caller's Complete), so a pooled range is never
// rewritten while a recorded draw can still read it. C4-1b: constants are root CBVs copied at recording
// (NativeModelConstantUploads), so the former constant chunks are gone.
constexpr UINT table_chunk_draws=64; // 64 x 32 samplers = the 2048 shader-visible sampler heap limit
struct TableChunk { ID3D12Device* device{};ComPtr<ID3D12DescriptorHeap> resources,samplers;UINT used{}; };
struct ModelDrawCaches {
    std::recursive_mutex lock;
    uint64_t epoch{1};   // PREPARE-REDUCTION: incremented when the device changes; program sets keep values for one epoch
    ID3D12Device* device{};
    UINT resource_stride{},sampler_stride{};
    ComPtr<ID3D12RootSignature> root,tess_root;   // W7: tess_root adds the hull and domain root CBVs
    ComPtr<ID3D12DescriptorHeap> null_resources,null_samplers; // CPU-only templates of one draw's unused slots
    std::unordered_map<std::string,ComPtr<ID3D12PipelineState>> pipelines;
    // 1134: programs by fingerprint (size, strided bytes, tail), each verified by a full byte comparison; the
    // former byte-string keys copied and hashed every shader twice per draw per frame.
    struct CachedProgram { ds3content::ShaderStage stage{};uint32_t model_major{};std::vector<uint8_t> bytes;std::shared_ptr<const ds3content::ShaderInterface> shader_interface; };
    std::unordered_map<uint64_t,std::vector<std::unique_ptr<CachedProgram>>> programs;
    std::unordered_map<UINT,bool> vertex_formats;
    std::vector<std::unique_ptr<TableChunk>> free_tables;std::shared_ptr<TableChunk> tables;
    // D3D12 review R6 (C2): one shader-visible CBV/SRV heap for every model draw, carved into ranges (one per draw since C4-2) that return by
    // ticket, each draw writing only the extent its bindings reach (Resource Binding Tier 3: unwritten slots of a bound
    // table are never read); one sampler heap whose 16-sampler stage tables are cached by content for the device's life.
    // Draws that do not fit (another tier, the test hook SetModelDrawLegacyTables, a full sampler cache, no free range) keep the
    // former per-draw tables above.
    bool tier3{};
    ComPtr<ID3D12DescriptorHeap> global_resources,global_samplers;
    // C4-2 (Native1431): a draw's descriptor range is its own, returned by ticket when that draw is gone, so a draw kept
    // across frames pins only its range (with 8192-descriptor chunks shared by a frame's draws, kept draws pinned all 32
    // chunks and later draws fell back to per-draw tables). Ranges are rounded to 16 descriptors. I-0074: a returned range
    // merges with its free neighbours (and gives back the heap's untouched end when it reaches it), and a range is taken
    // from the smallest free one that holds it, split, before the untouched end: at Archdragon Peak the arrival used the
    // whole heap once, and the unmerged small free ranges then left about 53 draws a frame on per-draw tables while half
    // the heap was free (i74-native-17). When no range is in use the heap starts afresh. A range returns with its
    // device epoch (a device change and back must not return an old range) through a separately locked list, drained
    // at the next allocation, so a draw released on the recording thread never waits for a prepare holding the cache lock.
    std::map<UINT,UINT> free_global_ranges;std::multimap<UINT,UINT> free_global_sizes;   // base to size; size to base
    UINT global_bump{},global_in_use{};
    struct GlobalRange {uint64_t epoch{};UINT base{},size{};};
    struct SamplerTable {std::array<D3D12_SAMPLER_DESC,sampler_count> set{};UINT index{},extent{};};   // index: the table's base
    std::unordered_map<uint64_t,std::vector<SamplerTable>> sampler_tables;UINT sampler_tables_used{};   // used: the heap's bump
};
ModelDrawCaches& Caches(){static auto* caches=new ModelDrawCaches;return *caches;} // process lifetime by design
// C4-2 with frame slots: 64 chunks (was 32), room for a kept draw's table copy per slot (resource binding tier 3).
// I-0074: 122 chunks, the largest heap every binding tier allows (1,000,000 descriptors with the two null templates): at
// Archdragon Peak the 64 chunks were all in use (4,200-5,200 draws, two table copies each), and about 146 draws a frame
// fell back to per-draw tables, two shader-visible heaps created a frame and a prepare of 13 ms against 2.2 at Firelink
// (i74-native-07).
constexpr UINT global_chunk_descriptors=8192,global_chunks=122,global_sampler_descriptors=2048;   // the shader-visible sampler heap bound
static_assert(global_chunks*global_chunk_descriptors+stage_resources*2<=1000000,"shader-visible CBV/SRV/UAV heap bound (tiers 1 and 2)");
std::atomic<bool> legacy_tables{};
std::atomic<uint64_t> pso_created{},global_table_draws{},legacy_table_draws{};
std::array<std::atomic<uint64_t>,4> prepare_ticks{};
D3D12_SAMPLER_DESC UnusedSampler(){
    // CreateSampler has no null descriptor. This physical initializer is only
    // for unused slots; RDEF availability was checked against explicit inputs.
    D3D12_SAMPLER_DESC unused{};unused.Filter=D3D12_FILTER_MIN_MAG_MIP_POINT;unused.AddressU=unused.AddressV=unused.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    unused.MaxAnisotropy=1;unused.ComparisonFunc=D3D12_COMPARISON_FUNC_NEVER;unused.MaxLOD=D3D12_FLOAT32_MAX;return unused;
}
// C4-2 review: ranges whose draws are gone and whose submitted work has completed, returned by the deleter.
std::mutex returned_ranges_lock;std::vector<ModelDrawCaches::GlobalRange> returned_ranges;

// I-0074: a returned range into the free ranges, merged with the free ranges beside it; a free range reaching the untouched
// end gives it back (under the cache lock).
void FreeGlobalRange(ModelDrawCaches& c,UINT base,UINT size){
    const auto erase=[&](std::map<UINT,UINT>::iterator at){const auto [from,to]=c.free_global_sizes.equal_range(at->second);
        for(auto s=from;s!=to;++s)if(s->second==at->first){c.free_global_sizes.erase(s);break;}
        return c.free_global_ranges.erase(at);};
    auto next=c.free_global_ranges.lower_bound(base);
    // Review L8: a range returned twice or overlapping a free one would hand out overlapping ranges later: refused loudly.
    if(base+size>c.global_bump || (next!=c.free_global_ranges.end() && base+size>next->first) ||
       (next!=c.free_global_ranges.begin() && std::prev(next)->first+std::prev(next)->second>base))
        throw std::logic_error("native model descriptor range returned twice or overlapping");
    if(next!=c.free_global_ranges.begin()){auto previous=std::prev(next);
        if(previous->first+previous->second==base){base=previous->first;size+=previous->second;next=erase(previous);}}
    if(next!=c.free_global_ranges.end() && base+size==next->first){size+=next->second;erase(next);}
    if(base+size==c.global_bump){c.global_bump=base;return;}
    c.free_global_ranges.emplace(base,size);c.free_global_sizes.emplace(size,base);
}
// A draw's range of `needed` descriptors (under the cache lock); null when the heap has no room (the draw then keeps
// per-draw tables).
std::shared_ptr<ModelDrawCaches::GlobalRange> GlobalRangeOf(ModelDrawCaches& c,UINT needed){
    {std::vector<ModelDrawCaches::GlobalRange> returned;{const std::lock_guard hold(returned_ranges_lock);returned.swap(returned_ranges);}
        for(const auto& range:returned)if(range.epoch==c.epoch){FreeGlobalRange(c,range.base,range.size);c.global_in_use-=range.size;}
        if(!returned.empty() && !c.global_in_use){c.free_global_ranges.clear();c.free_global_sizes.clear();c.global_bump=0;}}
    const UINT size=(needed+15u)&~15u;UINT base{};
    if(const auto fit=c.free_global_sizes.lower_bound(size);fit!=c.free_global_sizes.end()){
        const UINT free_size=fit->first;base=fit->second;c.free_global_sizes.erase(fit);c.free_global_ranges.erase(base);
        if(free_size>size){c.free_global_ranges.emplace(base+size,free_size-size);c.free_global_sizes.emplace(free_size-size,base+size);}}
    else if(c.global_bump+size<=global_chunks*global_chunk_descriptors){base=c.global_bump;c.global_bump+=size;}
    else return nullptr;
    auto range=std::make_unique<ModelDrawCaches::GlobalRange>(ModelDrawCaches::GlobalRange{c.epoch,base,size});   // before the count
    c.global_in_use+=size;
    // To the return list once the last holder is gone and the queue has completed what was submitted until then.
    return std::shared_ptr<ModelDrawCaches::GlobalRange>(range.release(),[](ModelDrawCaches::GlobalRange* returned){
        try{RetireWhenSubmittedWorkCompletes(std::shared_ptr<void>(returned,[](void* held){
            std::unique_ptr<ModelDrawCaches::GlobalRange> range(static_cast<ModelDrawCaches::GlobalRange*>(held));
            try{const std::lock_guard hold(returned_ranges_lock);returned_ranges.push_back(*range);}catch(...){}}));}catch(...){}});
}
// The cached sampler table of one stage's first `extent` samplers (by content), created on first use: its base in the global
// sampler heap; UINT_MAX when the heap is full. I-0074: a table takes only the extent its bindings reach (Resource Binding
// Tier 3: a bound table's slots past it are never read; its 16-slot window stays inside the heap), as the resource ranges do:
// 16 slots for every table filled the 2048-sampler heap with 128 tables at Archdragon Peak (52 draws a frame fell back to
// per-draw tables, i74-native-09).
UINT SamplerTableOf(ModelDrawCaches& c,const std::array<D3D12_SAMPLER_DESC,sampler_count>& set,UINT extent){
    extent=std::clamp(extent,1u,sampler_count);
    uint64_t h=1469598103934665603ULL^extent;const auto* bytes=reinterpret_cast<const uint8_t*>(set.data());
    for(size_t i=0;i<extent*sizeof(D3D12_SAMPLER_DESC);++i){h^=bytes[i];h*=1099511628211ULL;}
    auto& bucket=c.sampler_tables[h];
    for(const auto& known:bucket)if(known.extent==extent && !std::memcmp(known.set.data(),set.data(),extent*sizeof(D3D12_SAMPLER_DESC)))return known.index;
    if(c.sampler_tables_used+sampler_count>global_sampler_descriptors)return UINT_MAX;
    const auto base=c.sampler_tables_used;c.sampler_tables_used+=extent;
    for(UINT slot=0;slot<extent;++slot)c.device->CreateSampler(&set[slot],CPU(c.global_samplers.Get(),c.sampler_stride,base+slot));
    bucket.push_back({set,base,extent});return base;
}
// D3D12 phase D cD0: highest number of global descriptor chunks in use since the last read.
std::atomic<uint64_t> global_chunks_in_use_peak{},global_bump_chunks_peak{};   // C4-2: and the heap's used extent (fragmentation shows as a bump far above the use)
void NotePeak(std::atomic<uint64_t>& peak,uint64_t value) noexcept {auto known=peak.load();while(value>known && !peak.compare_exchange_weak(known,value)){}}
template<class T> void Append(std::string& key,const T& value){key.append(reinterpret_cast<const char*>(&value),sizeof(value));}
std::shared_ptr<TableChunk> PooledTables(ModelDrawCaches& c){
    std::unique_ptr<TableChunk> chunk;
    if(!c.free_tables.empty()){chunk=std::move(c.free_tables.back());c.free_tables.pop_back();}
    else{chunk=std::make_unique<TableChunk>();chunk->device=c.device;
        D3D12_DESCRIPTOR_HEAP_DESC heap{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,stage_resources*2*table_chunk_draws,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
        Check(CountedHeap(c.device)->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&chunk->resources)),"native model resource table chunk allocation");
        heap={D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,sampler_count*2*table_chunk_draws,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
        Check(CountedHeap(c.device)->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&chunk->samplers)),"native model sampler table chunk allocation");}
    chunk->used=0;
    return std::shared_ptr<TableChunk>(chunk.release(),[](TableChunk* returned){auto& caches=Caches();
        const std::lock_guard hold(caches.lock);
        if(returned->device==caches.device)caches.free_tables.emplace_back(returned);else delete returned;});
}
void BindDevice(ModelDrawCaches& c,ID3D12Device* device){
    if(c.device==device)return;
    c.device=device;++c.epoch;c.root.Reset();c.tess_root.Reset();c.null_resources.Reset();c.null_samplers.Reset();c.pipelines.clear();c.programs.clear();
    c.vertex_formats.clear();c.free_tables.clear();c.tables.reset();
    c.free_global_ranges.clear();c.free_global_sizes.clear();c.global_bump=c.global_in_use=0;c.sampler_tables.clear();c.sampler_tables_used=0;c.global_resources.Reset();c.global_samplers.Reset();
    c.resource_stride=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    c.sampler_stride=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    // C4-1b: each stage table keeps its slot = register layout (CBVs 0..13, SRVs from slot 14) and holds the bounded CBVs
    // (VS b5, b6, b7, b12; PS b4) and the SRVs; every other constant register is a root CBV (RootConstantIndex).
    // Root signature 1.0: descriptors are volatile, which the bounded CBVs written at encode (after other draws' tables were
    // set in the list) require; a move to 1.1 must declare DESCRIPTORS_VOLATILE on those ranges (review).
    D3D12_DESCRIPTOR_RANGE ranges[7]{{D3D12_DESCRIPTOR_RANGE_TYPE_CBV,3,5,0,5},{D3D12_DESCRIPTOR_RANGE_TYPE_CBV,1,12,0,12},
        {D3D12_DESCRIPTOR_RANGE_TYPE_SRV,srv_count,0,0,cbv_count},
        {D3D12_DESCRIPTOR_RANGE_TYPE_CBV,1,4,0,4},{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,srv_count,0,0,cbv_count},
        {D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,sampler_count,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,sampler_count,0,0,0}};
    D3D12_ROOT_PARAMETER parameters[root_constant_base+root_constant_count]{};
    for(UINT stage=0;stage<2;++stage){
        const auto visibility=stage?D3D12_SHADER_VISIBILITY_PIXEL:D3D12_SHADER_VISIBILITY_VERTEX;
        parameters[stage].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[stage].ShaderVisibility=visibility;
        parameters[stage].DescriptorTable=stage?D3D12_ROOT_DESCRIPTOR_TABLE{2,&ranges[3]}:D3D12_ROOT_DESCRIPTOR_TABLE{3,&ranges[0]};
        parameters[stage+2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[stage+2].DescriptorTable={1,&ranges[stage+5]};parameters[stage+2].ShaderVisibility=visibility;
        for(UINT slot=0;slot<cbv_count;++slot)if(!BoundedConstantRegister(stage,slot)){auto& parameter=parameters[RootConstantIndex(stage,slot)];
            parameter.ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;parameter.Descriptor={slot,0};parameter.ShaderVisibility=visibility;}
    }
    const D3D12_ROOT_SIGNATURE_DESC root_desc{root_constant_base+root_constant_count,parameters,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT|
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS|D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS|D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS};
    ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&root_desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"native model root serialization");
    Check(Pipelines().Root(device,blob->GetBufferPointer(),blob->GetBufferSize(),c.root.ReleaseAndGetAddressOf()),"native model root creation");   // I-0072: the cache's roots
    {   // W7: the same parameters, then HS b0, b5, b6 and DS b0, b5, b6, b8 as root CBVs; hull and domain access allowed.
        D3D12_ROOT_PARAMETER tess[root_constant_base+root_constant_count+tess_constant_count]{};std::copy(std::begin(parameters),std::end(parameters),tess);
        for(const auto& [stage,slot]:tess_registers){auto& parameter=tess[TessRootIndex(stage,slot)];parameter.ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;
            parameter.Descriptor={slot,0};parameter.ShaderVisibility=stage==2?D3D12_SHADER_VISIBILITY_HULL:D3D12_SHADER_VISIBILITY_DOMAIN;}
        const D3D12_ROOT_SIGNATURE_DESC tess_desc{static_cast<UINT>(std::size(tess)),tess,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT|
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS};
        ComPtr<ID3DBlob> tess_blob,tess_error;Check(D3D12SerializeRootSignature(&tess_desc,D3D_ROOT_SIGNATURE_VERSION_1,&tess_blob,&tess_error),"native model tess root serialization");
        Check(Pipelines().Root(device,tess_blob->GetBufferPointer(),tess_blob->GetBufferSize(),c.tess_root.ReleaseAndGetAddressOf()),"native model tess root creation");}
    D3D12_DESCRIPTOR_HEAP_DESC heap{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,stage_resources*2,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
    Check(CountedHeap(device)->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&c.null_resources)),"native model null resource template");
    heap={D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,sampler_count*2,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
    Check(CountedHeap(device)->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&c.null_samplers)),"native model null sampler template");
    D3D12_SHADER_RESOURCE_VIEW_DESC null_srv{};null_srv.Format=DXGI_FORMAT_R8G8B8A8_UNORM;null_srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
    null_srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;null_srv.Texture2D.MipLevels=1;
    // CreateSampler has no null descriptor. This physical initializer is only
    // for unused slots; RDEF availability was checked against explicit inputs.
    const auto unused=UnusedSampler();
    for(UINT stage=0;stage<2;++stage){
        for(UINT slot=0;slot<cbv_count;++slot)device->CreateConstantBufferView(nullptr,CPU(c.null_resources.Get(),c.resource_stride,stage*stage_resources+slot));
        for(UINT slot=0;slot<srv_count;++slot)device->CreateShaderResourceView(nullptr,&null_srv,CPU(c.null_resources.Get(),c.resource_stride,stage*stage_resources+cbv_count+slot));
        for(UINT slot=0;slot<sampler_count;++slot)device->CreateSampler(&unused,CPU(c.null_samplers.Get(),c.sampler_stride,stage*sampler_count+slot));
    }
    D3D12_FEATURE_DATA_D3D12_OPTIONS options{};
    c.tier3=SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS,&options,sizeof(options))) &&
        options.ResourceBindingTier>=D3D12_RESOURCE_BINDING_TIER_3;
    if(c.tier3){
        // Slack past the last chunk: a bound table spans its full root extent from its base, written or not.
        heap={D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,global_chunks*global_chunk_descriptors+stage_resources*2,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
        Check(CountedHeap(device)->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&c.global_resources)),"native model global resource heap");
        heap={D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,global_sampler_descriptors,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
        Check(CountedHeap(device)->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&c.global_samplers)),"native model global sampler heap");
        c.free_global_ranges.clear();c.free_global_sizes.clear();c.global_bump=c.global_in_use=0;   // C4-2: per-draw ranges from the start of the heap
    }
}
const ModelDrawCaches::CachedProgram& CachedProgramOf(ModelDrawCaches& c,const ds3content::CpuShaderProgram& program){
    const auto& b=program.bytecode;uint64_t h=1469598103934665603ULL;const auto mix=[&](uint64_t v){h^=v;h*=1099511628211ULL;};
    mix(static_cast<uint64_t>(program.stage));mix(program.model_major);mix(b.size());
    for(size_t i=0;i<b.size();i+=61)mix(b[i]);
    for(size_t i=b.size()>64?b.size()-64:0;i<b.size();++i)mix(b[i]);
    auto& bucket=c.programs[h];
    for(const auto& known:bucket)if(known->stage==program.stage && known->model_major==program.model_major && known->bytes==b)return *known;
    auto entry=std::make_unique<ModelDrawCaches::CachedProgram>();entry->stage=program.stage;entry->model_major=program.model_major;entry->bytes=b;
    entry->shader_interface=std::make_shared<const ds3content::ShaderInterface>(ds3content::InspectShaderInterface(program));
    bucket.push_back(std::move(entry));return *bucket.back();
}
void AppendDepthStencil(std::string& key,const D3D12_DEPTH_STENCIL_DESC& d){
    Append(key,d.DepthEnable);Append(key,d.DepthWriteMask);Append(key,d.DepthFunc);Append(key,d.StencilEnable);
    Append(key,d.StencilReadMask);Append(key,d.StencilWriteMask);
    for(const auto& face:{d.FrontFace,d.BackFace}){Append(key,face.StencilFailOp);Append(key,face.StencilDepthFailOp);Append(key,face.StencilPassOp);Append(key,face.StencilFunc);}
}
void AppendBlend(std::string& key,const D3D12_BLEND_DESC& b){
    Append(key,b.AlphaToCoverageEnable);Append(key,b.IndependentBlendEnable);
    for(const auto& t:b.RenderTarget){Append(key,t.BlendEnable);Append(key,t.LogicOpEnable);Append(key,t.SrcBlend);Append(key,t.DestBlend);Append(key,t.BlendOp);
        Append(key,t.SrcBlendAlpha);Append(key,t.DestBlendAlpha);Append(key,t.BlendOpAlpha);Append(key,t.LogicOp);Append(key,t.RenderTargetWriteMask);}
}
}

// PREPARE-REDUCTION cP1/cP3/cP8: what a program set keeps (under the draw cache's lock; see NativeModelProgramSet).
struct NativeModelProgramSet::Derived {
    uint64_t epoch{};                         // the draw cache's device epoch these values belong to (0 = none)
    const void* vertex{},*pixel{},*hull{},*domain{};   // the verified programme entries (ModelDrawCaches::CachedProgram)
    struct Pipeline {std::vector<uint32_t> state;ComPtr<ID3D12PipelineState> pipeline;};
    std::vector<Pipeline> pipelines;          // by fixed-function state words (bounded)
    const void* sampler_bank{};uint64_t sampler_generation{};bool samplers_kept{};std::array<UINT,2> sampler_tables{};
};
std::shared_ptr<const NativeModelProgramSet> MakeNativeModelProgramSet(ds3content::CpuShaderProgram vertex,
    ds3content::CpuShaderProgram pixel,std::vector<ds3content::ModelVertexElement> elements,
    ds3content::CpuShaderProgram hull,ds3content::CpuShaderProgram domain){
    auto set=std::make_shared<NativeModelProgramSet>();set->vertex=std::move(vertex);set->pixel=std::move(pixel);
    set->elements=std::move(elements);set->hull=std::move(hull);set->domain=std::move(domain);
    set->derived=std::make_shared<NativeModelProgramSet::Derived>();return set;
}
namespace {
std::atomic<uint32_t> shadow_period{};std::atomic<uint64_t> shadow_draws{},shadow_mismatches{};
// The fixed-function state of a pipeline as 32-bit words, field by field (no struct padding is compared): the state part
// of the former key string (the programmes and elements are the set's).
std::vector<uint32_t> PipelineStateWords(const NativeModelDrawInput& input,const D3D12_RASTERIZER_DESC& r){
    std::vector<uint32_t> w;w.reserve(128);
    for(const uint32_t v:{uint32_t(r.FillMode),uint32_t(r.CullMode),uint32_t(r.FrontCounterClockwise),uint32_t(r.DepthBias)})w.push_back(v);
    uint32_t f{};std::memcpy(&f,&r.DepthBiasClamp,4);w.push_back(f);std::memcpy(&f,&r.SlopeScaledDepthBias,4);w.push_back(f);
    for(const uint32_t v:{uint32_t(r.DepthClipEnable),uint32_t(r.MultisampleEnable),uint32_t(r.AntialiasedLineEnable),r.ForcedSampleCount,uint32_t(r.ConservativeRaster)})w.push_back(v);
    const auto& d=input.depth_stencil;
    for(const uint32_t v:{uint32_t(d.DepthEnable),uint32_t(d.DepthWriteMask),uint32_t(d.DepthFunc),uint32_t(d.StencilEnable),uint32_t(d.StencilReadMask),uint32_t(d.StencilWriteMask)})w.push_back(v);
    for(const auto& face:{d.FrontFace,d.BackFace})for(const uint32_t v:{uint32_t(face.StencilFailOp),uint32_t(face.StencilDepthFailOp),uint32_t(face.StencilPassOp),uint32_t(face.StencilFunc)})w.push_back(v);
    const auto& b=input.blend;w.push_back(uint32_t(b.AlphaToCoverageEnable));w.push_back(uint32_t(b.IndependentBlendEnable));
    for(const auto& t:b.RenderTarget)for(const uint32_t v:{uint32_t(t.BlendEnable),uint32_t(t.LogicOpEnable),uint32_t(t.SrcBlend),uint32_t(t.DestBlend),uint32_t(t.BlendOp),
        uint32_t(t.SrcBlendAlpha),uint32_t(t.DestBlendAlpha),uint32_t(t.BlendOpAlpha),uint32_t(t.LogicOp),uint32_t(t.RenderTargetWriteMask)})w.push_back(v);
    w.push_back(input.sample_mask);w.push_back(input.render_target_count);w.push_back(uint32_t(input.shape.envelope));w.push_back(uint32_t(input.shape.family));w.push_back(uint32_t(input.shape.topology));   // W0a: the shape (the set's pipelines are keyed on these words)
    for(UINT target=0;target<input.render_target_count;++target)w.push_back(uint32_t(input.render_target_formats[target]));
    w.push_back(uint32_t(input.depth_format));
    return w;
}
}

void OwnedNativeModelDraw::Prepare(ID3D12Device* device){
    state_=std::make_shared<State>();reused_=false;State& s=*state_;
    const auto* set=input_.program_set.get();
    const auto& vertex_source=set?set->vertex:input_.vertex_shader;const auto& pixel_source=set?set->pixel:input_.pixel_shader;
    const auto& element_source=set?set->elements:input_.elements;
    // W0a: the envelope agrees with the producer's count; a pixel programme is optional only where the envelope says so
    // (DepthOnly), and an absent one takes no pixel binding. The tessellation family and patch topology wait for W0a-5.
    const auto spec=EnvelopeSpec(input_.shape.envelope);const bool pixel_present=!pixel_source.bytecode.empty();
    Require(input_.render_target_count==spec.targets && input_.render_target_count<=input_.render_target_formats.size(),"native model target envelope count");
    // W7: ModelTess draws a 3-control-point patch list through the set's hull and domain programmes; Model a triangle list.
    const bool tess=input_.shape.family==ModelRootFamily::ModelTess;
    Require(tess?input_.shape.topology==ModelTopologyClass::Patch3 && set && !set->hull.bytecode.empty() && !set->domain.bytecode.empty() &&
            set->hull.stage==ds3content::ShaderStage::Hull && set->domain.stage==ds3content::ShaderStage::Domain:
        input_.shape.family==ModelRootFamily::Model && input_.shape.topology==ModelTopologyClass::Triangle && (!set || (set->hull.bytecode.empty() && set->domain.bytecode.empty())),
        "native model stage family");
    Require(pixel_present || !spec.pixel_required,"native model pixel programme required by the envelope");
    if(input_.shape.envelope==ModelPassEnvelope::DepthOnly)Require(input_.depth_format!=DXGI_FORMAT_UNKNOWN && input_.depth_stencil.DepthEnable,"native model depth-only draw without a depth target");
    if(!pixel_present)Require(std::none_of(input_.constants.begin(),input_.constants.end(),[](const auto& b){return b.stage==ds3content::ShaderStage::Pixel;}) &&
        std::none_of(input_.srvs.begin(),input_.srvs.end(),[](const auto& b){return b.stage==ds3content::ShaderStage::Pixel;}) &&
        std::none_of(input_.samplers.begin(),input_.samplers.end(),[](const auto& b){return b.stage==ds3content::ShaderStage::Pixel;}),
        "native model pixel binding without a pixel programme");
    Require(device && vertex_source.stage==ds3content::ShaderStage::Vertex &&
        (!pixel_present || pixel_source.stage==ds3content::ShaderStage::Pixel),"native model shader stage/device");
    // Phase C measurement: QPC ticks of programs, validation, pipeline and tables, summed over every Prepare.
    struct Clock {std::array<LARGE_INTEGER,4> marks{};Clock(){QueryPerformanceCounter(&marks[0]);marks[1]=marks[2]=marks[3]=marks[0];}
        void Mark(size_t i){QueryPerformanceCounter(&marks[i]);}
        ~Clock(){LARGE_INTEGER end{};QueryPerformanceCounter(&end);for(size_t i=1;i<4;++i)if(marks[i].QuadPart<marks[i-1].QuadPart)marks[i]=marks[i-1];for(size_t i=0;i<4;++i)prepare_ticks[i]+=uint64_t((i<3?marks[i+1]:end).QuadPart-marks[i].QuadPart);}} clock;
    kprofile::SwitchIfActive(kprofile::PrepDrawPrograms);   // PREPARE-REDUCTION cP0 (main view only)
    auto& caches=Caches();std::unique_lock hold(caches.lock);BindDevice(caches,device);
    auto* derived=set?set->derived.get():nullptr;
    if(derived && derived->epoch!=caches.epoch){*derived={};derived->epoch=caches.epoch;}
    // Shadow: every Nth draw with a set also runs the former paths below and compares.
    const auto period=shadow_period.load(std::memory_order_relaxed);
    const bool shadow=derived && period && shadow_draws.fetch_add(1,std::memory_order_relaxed)%period==0;
    const auto mismatch=[&]{shadow_mismatches.fetch_add(1,std::memory_order_relaxed);};
    const ModelDrawCaches::CachedProgram* vertex_entry{};const ModelDrawCaches::CachedProgram* pixel_entry{};
    const ModelDrawCaches::CachedProgram* hull_entry{};const ModelDrawCaches::CachedProgram* domain_entry{};
    const auto pixel_of=[&]()->const ModelDrawCaches::CachedProgram*{return pixel_present?&CachedProgramOf(caches,pixel_source):nullptr;};
    if(derived && derived->vertex){vertex_entry=static_cast<const ModelDrawCaches::CachedProgram*>(derived->vertex);pixel_entry=static_cast<const ModelDrawCaches::CachedProgram*>(derived->pixel);
        hull_entry=static_cast<const ModelDrawCaches::CachedProgram*>(derived->hull);domain_entry=static_cast<const ModelDrawCaches::CachedProgram*>(derived->domain);
        if(shadow && (&CachedProgramOf(caches,vertex_source)!=vertex_entry || pixel_of()!=pixel_entry)){mismatch();vertex_entry=pixel_entry=nullptr;}}
    if(!vertex_entry){vertex_entry=&CachedProgramOf(caches,vertex_source);pixel_entry=pixel_of();
        hull_entry=tess?&CachedProgramOf(caches,set->hull):nullptr;domain_entry=tess?&CachedProgramOf(caches,set->domain):nullptr;
        if(derived){derived->vertex=vertex_entry;derived->pixel=pixel_entry;derived->hull=hull_entry;derived->domain=domain_entry;}}
    // W0a: a depth-only draw without a pixel programme validates its pixel stage against an empty interface (W7: and a draw
    // without hull and domain programmes those stages).
    static const ds3content::ShaderInterface no_pixel_interface{};
    const auto& vertex_program=*vertex_entry;
    const auto vertex_interface=vertex_program.shader_interface;clock.Mark(1);kprofile::SwitchIfActive(kprofile::PrepDrawValidate);
    ValidateBindings(input_,{*vertex_interface,pixel_entry?*pixel_entry->shader_interface:no_pixel_interface,
        hull_entry?*hull_entry->shader_interface:no_pixel_interface,domain_entry?*domain_entry->shader_interface:no_pixel_interface},tess);ValidateGeometry(device,input_.geometry);
    for(const auto& binding:input_.constants)if(binding.frame)s.frame_mask|=1u<<NativeModelFrameRootOf(Stage(binding.stage),binding.shader_register);
    Require(!element_source.empty() && element_source.size()<=256,"native model input elements");
    std::vector<D3D12_INPUT_ELEMENT_DESC> elements;elements.reserve(element_source.size());
    for(size_t ordinal=0;ordinal<element_source.size();++ordinal){const auto& element=element_source[ordinal];
        const bool second=element.input_slot==1 && input_.geometry.vertices2.BufferLocation;
        const bool third=element.input_slot==2 && input_.geometry.previous.BufferLocation;   // I-0134: slot 2, slot 0's layout
        Require(!element.semantic.empty() && element.semantic.size()<=32 && (element.input_slot==0 || second || third) &&
            element.input_class==D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA && !element.instance_step_rate &&
            element.byte_offset<(second?input_.geometry.vertices2.StrideInBytes:input_.geometry.vertices.StrideInBytes),"native model input element stream");
        for(size_t previous=0;previous<ordinal;++previous)Require(element_source[previous].semantic!=element.semantic ||
            element_source[previous].semantic_index!=element.semantic_index,"native model duplicate input semantic");
        auto format=caches.vertex_formats.find(UINT(element.format));
        if(format==caches.vertex_formats.end()){D3D12_FEATURE_DATA_FORMAT_SUPPORT support{element.format};
            Check(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT,&support,sizeof(support)),"native model vertex format query");
            format=caches.vertex_formats.emplace(UINT(element.format),(support.Support1&D3D12_FORMAT_SUPPORT1_IA_VERTEX_BUFFER)!=0).first;}
        Require(format->second,"native model unsupported vertex format");
        elements.push_back({element.semantic.c_str(),element.semantic_index,element.format,element.input_slot,
            element.byte_offset,element.input_class,element.instance_step_rate});
    }
    for(const auto factor:input_.blend_factor)Require(std::isfinite(factor),"native model blend factor");
    for(const auto& owner:input_.cpu_owners)Require(bool(owner),"native model empty CPU owner");
    for(const auto& srv:input_.srvs)SameDevice(device,srv.resource.Get());
    clock.Mark(2);s.root=tess?caches.tess_root:caches.root;kprofile::SwitchIfActive(kprofile::PrepDrawPipeline);
    // cP3: the set's pipeline for this state; a miss (or a shadow draw) takes the former key string and lookup. I-0113 gap 8: the
    // same for a caster row's two local-face rasterizers, into s.local_pipelines.
    // I-0140: `mask` (the fade-mask pipeline) takes its own depth, blend and target in place of the draw's, and not the set's cache.
    const auto resolve=[&](const D3D12_RASTERIZER_DESC& raster,ComPtr<ID3D12PipelineState>& out,bool mask=false){
    D3D12_DEPTH_STENCIL_DESC depth_stencil=input_.depth_stencil;D3D12_BLEND_DESC blend=input_.blend;
    UINT target_count=input_.render_target_count;std::array<DXGI_FORMAT,8> target_formats=input_.render_target_formats;
    if(mask){depth_stencil.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ZERO;if(depth_stencil.DepthEnable)depth_stencil.DepthFunc=D3D12_COMPARISON_FUNC_LESS_EQUAL;
        depth_stencil.StencilEnable=FALSE;blend={};auto& t=blend.RenderTarget[0];t.BlendEnable=TRUE;t.SrcBlend=t.SrcBlendAlpha=D3D12_BLEND_ZERO;
        t.DestBlend=t.DestBlendAlpha=D3D12_BLEND_ZERO;t.BlendOp=t.BlendOpAlpha=D3D12_BLEND_OP_ADD;t.LogicOp=D3D12_LOGIC_OP_NOOP;
        t.RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_RED;target_count=1;target_formats={};target_formats[0]=kFadeMaskFormat;}
    std::vector<uint32_t> state;ComPtr<ID3D12PipelineState> kept;
    if(derived && !mask){state=PipelineStateWords(input_,raster);
        for(const auto& entry:derived->pipelines)if(entry.state==state){kept=entry.pipeline;break;}}
    if(kept && !shadow)out=kept;
    else{
        std::string key;Append(key,&vertex_program);Append(key,pixel_entry); // verified program entries, stable addresses (W0a: none for a depth-only draw)
        Append(key,hull_entry);Append(key,domain_entry);   // W7 (null for a triangle draw)
        Append(key,input_.shape.envelope);Append(key,input_.shape.family);Append(key,input_.shape.topology);
        Append(key,elements.size());
        for(const auto& element:elements){key.append(element.SemanticName).push_back('\0');Append(key,element.SemanticIndex);Append(key,element.Format);
            Append(key,element.InputSlot);Append(key,element.AlignedByteOffset);Append(key,element.InputSlotClass);Append(key,element.InstanceDataStepRate);}
        Append(key,raster);AppendDepthStencil(key,depth_stencil);AppendBlend(key,blend);Append(key,input_.sample_mask);
        Append(key,target_count);
        for(UINT target=0;target<target_count;++target)Append(key,target_formats[target]);
        Append(key,input_.depth_format);
        if(const auto found=caches.pipelines.find(key);found!=caches.pipelines.end())out=found->second;
        else{
            // D3D12 review R9 (C1c): created without the cache lock, so one new pipeline no longer stalls every thread
            // preparing a draw; the first inserted wins a race (same description, same state). The program entries and the
            // root have device lifetime; this draw's input (or its set) owns the bytecode and the semantic names.
            // The epoch, not the device pointer: a device change and back (or a new device at a freed address) during the
            // creation clears the programme entries this key and pipeline refer to (pre-merge review).
            const auto device_epoch=caches.epoch;hold.unlock();
            D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};pso.pRootSignature=s.root.Get();pso.VS={vertex_source.bytecode.data(),vertex_source.bytecode.size()};
            if(pixel_present)pso.PS={pixel_source.bytecode.data(),pixel_source.bytecode.size()};pso.InputLayout={elements.data(),static_cast<UINT>(elements.size())};
            pso.RasterizerState=raster;pso.DepthStencilState=depth_stencil;pso.BlendState=blend;pso.SampleMask=input_.sample_mask;
            pso.SampleDesc.Count=1;pso.PrimitiveTopologyType=tess?D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH:D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;pso.NumRenderTargets=target_count;
            if(tess){pso.HS={set->hull.bytecode.data(),set->hull.bytecode.size()};pso.DS={set->domain.bytecode.data(),set->domain.bytecode.size()};}
            std::copy_n(target_formats.begin(),target_count,pso.RTVFormats);pso.DSVFormat=input_.depth_format;
            // I-0072: the device's pipeline cache (precompiled at start, or the library's object, or compiled now).
            bool created{};Check(Pipelines().Graphics(device,pso,out.ReleaseAndGetAddressOf(),&created),"native model selected-program pipeline");
            if(created)++pso_created;
            hold.lock();Require(caches.epoch==device_epoch,"native model device changed during pipeline creation");
            out=caches.pipelines.emplace(std::move(key),out).first->second;
        }
        if(kept && kept!=out)mismatch();
        if(derived && !mask && kept!=out){
            std::erase_if(derived->pipelines,[&](const auto& entry){return entry.state==state;});
            if(derived->pipelines.size()<64)derived->pipelines.push_back({state,out});}
    }};
    resolve(input_.rasterizer,s.pipeline);
    if(input_.local_rasterizer_set)for(size_t k=0;k<2;++k)resolve(input_.local_rasterizers[k],s.local_pipelines[k]);
    if(input_.fade_mask)resolve(input_.rasterizer,s.fade_mask_pipeline,true);   // I-0140
    clock.Mark(3);const auto resource_stride=caches.resource_stride,sampler_stride=caches.sampler_stride;kprofile::SwitchIfActive(kprofile::PrepDrawSamplers);
    // I-0134: slot 2 (the previous cloth copy) bound only for a programme set that reads it; slot 1 then stays as the mesh has it.
    const bool reads_previous=std::any_of(element_source.begin(),element_source.end(),[](const auto& element){return element.input_slot==2;});
    s.streams={input_.geometry.vertices,input_.geometry.vertices2,reads_previous?input_.geometry.previous:D3D12_VERTEX_BUFFER_VIEW{}};
    s.stream_count=reads_previous?3u:input_.geometry.vertices2.BufferLocation?2u:1u;
    // D3D12 review R6 (C2): the global heaps. Per stage the table holds the bounded CBVs (slot = register, written at encode)
    // and the SRVs (slot = 14 + register) since C4-1b, from
    // the table base up to the highest bound one); only bound slots are written (ValidateBindings requires every
    // declared register to be bound). Sampler tables come from the content cache.
    for(const auto& binding:input_.constants)s.bounded|=BoundedConstantRegister(Stage(binding.stage),binding.shader_register);
    Require(input_.table_copies>=1 && input_.table_copies<=s.bounded_frame.size(),"native model table copies");
    if(!legacy_tables && caches.tier3){
        std::array<UINT,2> extent{1,1};
        for(const auto& binding:input_.constants){const auto stage=Stage(binding.stage);   // C4-1b: the bounded CBVs are table slots
            if(BoundedConstantRegister(stage,binding.shader_register))extent[stage]=std::max(extent[stage],binding.shader_register+1);}
        for(const auto& binding:input_.srvs){const auto stage=Stage(binding.stage);extent[stage]=std::max(extent[stage],cbv_count+binding.shader_register+1);}
        // cP8: the set keeps the two sampler tables of its sampler bank version; a miss (or a shadow draw) builds the sets.
        const bool bank_keyed=derived && input_.sampler_bank;
        std::array<UINT,2> sampler_tables{};
        const bool samplers_kept=bank_keyed && derived->samplers_kept && derived->sampler_bank==input_.sampler_bank &&
            derived->sampler_generation==input_.sampler_bank_generation;
        if(samplers_kept && !shadow)sampler_tables=derived->sampler_tables;
        else{
            std::array<std::array<D3D12_SAMPLER_DESC,sampler_count>,2> sets{};
            for(auto& set_descs:sets)set_descs.fill(UnusedSampler());
            std::array<UINT,2> sampler_extent{1,1};
            for(const auto& binding:input_.samplers){const auto stage=Stage(binding.stage);sets[stage][binding.shader_register]=binding.description;
                sampler_extent[stage]=std::max(sampler_extent[stage],binding.shader_register+1);}
            sampler_tables={SamplerTableOf(caches,sets[0],sampler_extent[0]),SamplerTableOf(caches,sets[1],sampler_extent[1])};
            if(samplers_kept && sampler_tables!=derived->sampler_tables)mismatch();
            if(bank_keyed){derived->sampler_bank=input_.sampler_bank;derived->sampler_generation=input_.sampler_bank_generation;
                derived->sampler_tables=sampler_tables;derived->samplers_kept=true;}
        }
        kprofile::SwitchIfActive(kprofile::PrepDrawConstants);
        if(sampler_tables[0]!=UINT_MAX && sampler_tables[1]!=UINT_MAX){
            // C4-2 with frame slots: a draw with bounded CBVs takes one copy of its two tables per asked copy (the SRVs in each).
            const UINT needed=extent[0]+extent[1],copies=s.bounded?input_.table_copies:1u;
            if(const auto range=GlobalRangeOf(caches,needed*copies)){
                // The peak in chunk equivalents (8192 descriptors in use), as the audit row has counted since cD0.
                NotePeak(global_chunks_in_use_peak,(caches.global_in_use+global_chunk_descriptors-1)/global_chunk_descriptors);
                NotePeak(global_bump_chunks_peak,(caches.global_bump+global_chunk_descriptors-1)/global_chunk_descriptors);
                const std::array<UINT,2> base{range->base,range->base+extent[0]};s.pooled.push_back(range);
                auto* heap=caches.global_resources.Get();
                kprofile::SwitchIfActive(kprofile::PrepDrawSrvs);
                for(UINT copy=0;copy<copies;++copy)for(const auto& binding:input_.srvs)
                    device->CopyDescriptorsSimple(1,CPU(heap,resource_stride,base[Stage(binding.stage)]+copy*needed+cbv_count+binding.shader_register),
                        binding.descriptor,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                s.table_copies=copies;s.copy_stride=needed;
                s.resources=caches.global_resources;s.samplers=caches.global_samplers;
                s.heaps={s.resources.Get(),s.samplers.Get()};
                s.table_cpu={CPU(heap,resource_stride,base[0]),CPU(heap,resource_stride,base[1])};s.device=device;s.resource_stride=resource_stride;
                s.tables={GPU(heap,resource_stride,base[0]),GPU(heap,resource_stride,base[1]),
                    GPU(s.samplers.Get(),sampler_stride,sampler_tables[0]),GPU(s.samplers.Get(),sampler_stride,sampler_tables[1])};
                s.global=true;s.CopyTables();++global_table_draws;return;
            }
        }
    }
    ++legacy_table_draws;kprofile::SwitchIfActive(kprofile::PrepDrawConstants);
    if(!caches.tables || caches.tables->used==table_chunk_draws)caches.tables=PooledTables(caches);
    const auto table=caches.tables->used++;s.pooled.push_back(caches.tables);
    s.resources=caches.tables->resources;s.samplers=caches.tables->samplers;
    const UINT resource_base=table*stage_resources*2,sampler_base=table*sampler_count*2;
    device->CopyDescriptorsSimple(stage_resources*2,CPU(s.resources.Get(),resource_stride,resource_base),
        caches.null_resources->GetCPUDescriptorHandleForHeapStart(),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    device->CopyDescriptorsSimple(sampler_count*2,CPU(s.samplers.Get(),sampler_stride,sampler_base),
        caches.null_samplers->GetCPUDescriptorHandleForHeapStart(),D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    for(const auto& binding:input_.srvs)device->CopyDescriptorsSimple(1,CPU(s.resources.Get(),resource_stride,resource_base+Stage(binding.stage)*stage_resources+cbv_count+binding.shader_register),binding.descriptor,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    for(const auto& binding:input_.samplers)device->CreateSampler(&binding.description,CPU(s.samplers.Get(),sampler_stride,sampler_base+Stage(binding.stage)*sampler_count+binding.shader_register));
    s.table_cpu={CPU(s.resources.Get(),resource_stride,resource_base),CPU(s.resources.Get(),resource_stride,resource_base+stage_resources)};s.device=device;s.resource_stride=resource_stride;
    s.heaps={s.resources.Get(),s.samplers.Get()};s.tables={GPU(s.resources.Get(),resource_stride,resource_base),GPU(s.resources.Get(),resource_stride,resource_base+stage_resources),
        GPU(s.samplers.Get(),sampler_stride,sampler_base),GPU(s.samplers.Get(),sampler_stride,sampler_base+sampler_count)};
    s.CopyTables();
}
GBufferDraw OwnedNativeModelDraw::Describe(std::span<const D3D12_CPU_DESCRIPTOR_HANDLE> targets,
    D3D12_CPU_DESCRIPTOR_HANDLE depth,D3D12_VIEWPORT viewport,D3D12_RECT scissor,uint32_t copy,uint32_t local) const {
    const State& s=*state_;
    const bool mask=local==kFadeMaskPipeline;   // I-0140: the fade-mask pipeline, one target
    Require(targets.size()==(mask?1u:input_.render_target_count) && copy<s.table_copies,"native model target envelope");
    Require(!local || (mask?bool(s.fade_mask_pipeline):local<=2 && s.local_pipelines[local-1]),"native model local-face or fade-mask pipeline");   // I-0113 gap 8
    // I-0090: the copy's tables from the state (GBufferDraw holds spans: the former local copy of the tables dangled once
    // Describe returned).
    return {targets,depth,viewport,scissor,s.root.Get(),mask?s.fade_mask_pipeline.Get():local?s.local_pipelines[local-1].Get():s.pipeline.Get(),s.heaps,s.copy_tables[copy],
        input_.shape.topology==ModelTopologyClass::Patch3?D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST:D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
        {s.streams.data(),s.stream_count},input_.geometry.indices,input_.geometry.index_count,1,0,0,0};
}
void OwnedNativeModelDraw::Encode(ID3D12GraphicsCommandList* list,std::span<const D3D12_CPU_DESCRIPTOR_HANDLE> targets,
    D3D12_CPU_DESCRIPTOR_HANDLE depth,D3D12_VIEWPORT viewport,D3D12_RECT scissor,const NativeModelFrameAddresses& frame,
    NativeModelConstantUploads& uploads,GBufferBindState* state,uint32_t local) const {
    State& s=*state_;
    // C4-2 with frame slots: numbered frame F binds (and for bounded CBVs writes) table copy F % copies; unnumbered, copy 0.
    const uint32_t copy=static_cast<uint32_t>(uploads.Frame()%s.table_copies);
    Require(list!=nullptr,"native model command list");const auto draw=Describe(targets,depth,viewport,scissor,copy,local);
    for(uint32_t root=0;root<FrameRootCount;++root)if(s.frame_mask>>root&1u)
        Require(frame[root] && !(frame[root]%D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT),"native model frame block address");
    // C4-1b: every constant payload's address before any command (a failed copy records nothing); a bounded register's
    // descriptor CBV (view = the payload's 256-byte size) is written into the state's table slot (one draw of a state a frame).
    std::array<D3D12_GPU_VIRTUAL_ADDRESS,4*cbv_count> addresses{};bool bounded=false;   // W7: four stages
    for(const auto& binding:input_.constants){const auto stage=Stage(binding.stage),slot=binding.shader_register;
        addresses[stage*cbv_count+slot]=binding.frame?frame[NativeModelFrameRootOf(stage,slot)]:uploads.Address(binding.Payload());
        bounded|=BoundedConstantRegister(stage,slot);}
    if(bounded){Require(s.table_cpu[0].ptr && s.table_cpu[1].ptr && s.device,"native model bounded constant table");
        // C4-2a: a first encode, or (numbered frames) a later frame once the GPU has finished the previous one: the draw's
        // table slots are then no longer read. An unnumbered encode is the only one.
        const auto now=uploads.Frame()?uploads.Frame():UINT64_MAX;auto& last=s.bounded_frame[copy];auto previous=last.load();
        // D-0180 C3: a caster drawn into several cascades of one numbered frame is encoded once per cascade with the same uploads
        // object, so its bounded payloads have the same addresses: the table written by the frame's first encode stands.
        const bool again=uploads.Frame() && previous==now && s.bounded_uploads[copy]==&uploads;
        // I-0090: the draws sharing a state are one row's successive draws, one a frame; a second draw of the state in the same
        // frame would overwrite the first one's bounded CBVs (refused loudly).
        Require(!again || s.bounded_draw[copy]==this,"native model shared state encoded by two draws in one frame");
        if(!again){
        do Require(!previous || (uploads.Frame() && previous!=UINT64_MAX && previous<uploads.Frame() && previous<=uploads.Completed()),
            "native model bounded constant encode");
        while(!last.compare_exchange_weak(previous,now));
        s.bounded_uploads[copy]=&uploads;s.bounded_draw[copy]=this;}
        if(!again)for(const auto& binding:input_.constants){const auto stage=Stage(binding.stage),slot=binding.shader_register;if(!BoundedConstantRegister(stage,slot))continue;
            const D3D12_CONSTANT_BUFFER_VIEW_DESC view{addresses[stage*cbv_count+slot],static_cast<UINT>((binding.Payload().size()+255)&~size_t(255))};
            auto destination=s.table_cpu[stage];destination.ptr+=SIZE_T(s.resource_stride)*(SIZE_T(copy)*s.copy_stride+slot);s.device->CreateConstantBufferView(&view,destination);}}
    if(!state){list->OMSetStencilRef(input_.stencil_reference);list->OMSetBlendFactor(input_.blend_factor.data());BindGBufferDraw(list,draw);}
    else{
        if(!state->stencil_set || state->stencil!=input_.stencil_reference){list->OMSetStencilRef(input_.stencil_reference);state->stencil=input_.stencil_reference;state->stencil_set=true;}
        if(!state->blend_set || state->blend!=input_.blend_factor){list->OMSetBlendFactor(input_.blend_factor.data());state->blend=input_.blend_factor;state->blend_set=true;}
        BindGBufferDraw(list,draw,*state);}
    // After the root signature the bind set: each root-CBV register's address (C4-1 frame blocks from the pass).
    for(const auto& binding:input_.constants){const auto stage=Stage(binding.stage),slot=binding.shader_register;if(BoundedConstantRegister(stage,slot))continue;
        const UINT root=stage<2?RootConstantIndex(stage,slot):TessRootIndex(stage,slot);const auto address=addresses[stage*cbv_count+slot];
        if(state && root<state->root_cbvs.size()){if(state->root_cbvs[root]==address)continue;state->root_cbvs[root]=address;}
        list->SetGraphicsRootConstantBufferView(root,address);}
    list->DrawIndexedInstanced(draw.index_count,draw.instance_count,draw.start_index,draw.base_vertex,draw.start_instance);
}
namespace {
// The upload tables' storage, kept by the thread between views (each table is emptied before it returns here).
thread_local std::vector<std::vector<NativeModelConstantUploads::Uploaded>> uploaded_pool;
constexpr size_t uploaded_initial=4096;
size_t UploadedSlot(const void* key,size_t mask) noexcept {return static_cast<size_t>((reinterpret_cast<uintptr_t>(key)>>4)*0x9E3779B97F4A7C15ull>>17)&mask;}
}
NativeModelConstantUploads::NativeModelConstantUploads(std::function<Range(uint64_t bytes)> allocate,uint64_t frame,uint64_t completed):
    allocate_(std::move(allocate)),frame_(frame),completed_(completed){
    if(!uploaded_pool.empty()){uploaded_=std::move(uploaded_pool.back());uploaded_pool.pop_back();}
    if(uploaded_.size()<uploaded_initial)uploaded_.assign(uploaded_initial,{});
}
NativeModelConstantUploads::~NativeModelConstantUploads(){
    if(uploaded_count_)std::fill(uploaded_.begin(),uploaded_.end(),Uploaded{});
    try{uploaded_pool.push_back(std::move(uploaded_));}catch(...){}
}
D3D12_GPU_VIRTUAL_ADDRESS NativeModelConstantUploads::Address(const std::vector<uint8_t>& payload){
    const void* const key=&payload;
    size_t mask=uploaded_.size()-1,slot=UploadedSlot(key,mask);
    for(;uploaded_[slot].key;slot=(slot+1)&mask)if(uploaded_[slot].key==key)return uploaded_[slot].gpu;
    // ValidateBindings bounded every payload (1 to 65536 bytes, at least its declaration); the range is zero-filled to
    // its 256-byte size, so no read of the declared buffer goes past it (only API padding, not producer coverage).
    Require(!payload.empty() && payload.size()<=65536,"native model constant upload extent");
    const auto rounded=(uint64_t(payload.size())+255)&~uint64_t(255);const auto range=allocate_(rounded);
    Require(range.cpu && range.gpu && !(range.gpu%D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT),"native model constant upload range");
    // The payload, then zeros to the 256-byte size (the allocator need not zero: review).
    std::memcpy(range.cpu,payload.data(),payload.size());std::memset(range.cpu+payload.size(),0,static_cast<size_t>(rounded-payload.size()));
    ++blocks_;bytes_+=payload.size();
    if(2*(uploaded_count_+1)>uploaded_.size()){   // grow: the table doubles and its entries are placed again
        std::vector<Uploaded> larger(uploaded_.size()*2);mask=larger.size()-1;
        for(const auto& entry:uploaded_)if(entry.key){auto at=UploadedSlot(entry.key,mask);while(larger[at].key)at=(at+1)&mask;larger[at]=entry;}
        uploaded_.swap(larger);slot=UploadedSlot(key,mask);while(uploaded_[slot].key)slot=(slot+1)&mask;}
    uploaded_[slot]={key,range.gpu};++uploaded_count_;return range.gpu;
}
namespace {
std::atomic<uint64_t> state_shadow_turn{};
// I-0090: whether two inputs make the same prepared state: everything Prepare reads except the constant payloads' bytes (their
// stages, registers, kinds and sizes are compared: the validation against the declarations reads only those), the blend factor
// and stencil reference (set at encode) and the CPU owners (holders only).
bool SameStateInputs(const NativeModelDrawInput& a,const NativeModelDrawInput& b) noexcept {
    if(!a.program_set || a.program_set!=b.program_set || a.sampler_bank!=b.sampler_bank || a.sampler_bank_generation!=b.sampler_bank_generation ||
       !a.vertex_shader.bytecode.empty() || !b.vertex_shader.bytecode.empty() || !a.pixel_shader.bytecode.empty() || !b.pixel_shader.bytecode.empty() ||
       !a.elements.empty() || !b.elements.empty() || a.table_copies!=b.table_copies)return false;
    const auto same=[](const auto& x,const auto& y){return !std::memcmp(&x,&y,sizeof(x));};
    const auto &g=a.geometry,&h=b.geometry;
    if(g.storage_generation!=h.storage_generation || g.stream_generation!=h.stream_generation || g.vertex_resource!=h.vertex_resource ||
       g.index_resource!=h.index_resource || !same(g.vertices,h.vertices) || !same(g.vertices2,h.vertices2) || !same(g.indices,h.indices) ||
       g.index_count!=h.index_count || g.stream_owner!=h.stream_owner || g.previous_resource!=h.previous_resource || !same(g.previous,h.previous))return false;
    if(!same(a.rasterizer,b.rasterizer) || a.fade_mask!=b.fade_mask || a.local_rasterizer_set!=b.local_rasterizer_set || (a.local_rasterizer_set && !same(a.local_rasterizers,b.local_rasterizers)) ||
       !same(a.depth_stencil,b.depth_stencil) || !same(a.blend,b.blend) || a.render_target_formats!=b.render_target_formats ||
       a.render_target_count!=b.render_target_count || a.shape!=b.shape || a.depth_format!=b.depth_format || a.sample_mask!=b.sample_mask)return false;
    if(a.constants.size()!=b.constants.size() || a.srvs.size()!=b.srvs.size() || a.samplers.size()!=b.samplers.size())return false;
    for(size_t i=0;i<a.constants.size();++i){const auto &x=a.constants[i],&y=b.constants[i];
        if(x.stage!=y.stage || x.shader_register!=y.shader_register || x.frame!=y.frame || x.Payload().size()!=y.Payload().size())return false;}
    for(size_t i=0;i<a.srvs.size();++i){const auto &x=a.srvs[i],&y=b.srvs[i];
        if(x.stage!=y.stage || x.shader_register!=y.shader_register || x.descriptor.ptr!=y.descriptor.ptr || x.resource!=y.resource || x.owner!=y.owner)return false;}
    for(size_t i=0;i<a.samplers.size();++i){const auto &x=a.samplers[i],&y=b.samplers[i];
        if(x.stage!=y.stage || x.shader_register!=y.shader_register || !same(x.description,y.description))return false;}
    return true;
}
}
bool OwnedNativeModelDraw::Adopt(ID3D12Device* device,const OwnedNativeModelDraw& base){
    if(!base.state_ || base.state_->device.Get()!=device || !SameStateInputs(base.input_,input_))return false;
    // What Prepare checks of the parts that are not compared (the payloads' sizes equal base's, which met the declarations):
    // the blend factor and the owners.
    for(const auto factor:input_.blend_factor)if(!std::isfinite(factor))return false;
    for(const auto& owner:input_.cpu_owners)if(!owner)return false;
    // Shadow (its period, set by the unit tests only): every Nth adoption is also prepared in full and compared; on a difference the
    // full state is kept.
    if(const auto period=shadow_period.load(std::memory_order_relaxed);period && state_shadow_turn.fetch_add(1,std::memory_order_relaxed)%period==0){
        // A full prepare that fails (a device change during a pipeline creation) leaves the adoption, which was valid.
        OwnedNativeModelDraw full(input_);try{full.Prepare(device);}catch(...){full.state_.reset();}
        if(full.state_){const auto &f=*full.state_,&k=*base.state_;
        const auto same=[](const auto& x,const auto& y){return !std::memcmp(&x,&y,sizeof(x));};
        // The sampler tables are compared when both states are on the global heaps (a state on per-draw tables has its own).
        const bool global=f.global && k.global;
        if(f.pipeline!=k.pipeline || f.local_pipelines!=k.local_pipelines || f.fade_mask_pipeline!=k.fade_mask_pipeline || f.root!=k.root || f.frame_mask!=k.frame_mask || f.bounded!=k.bounded || f.stream_count!=k.stream_count ||
           !same(f.streams,k.streams) || (global && (f.table_copies!=k.table_copies || f.copy_stride!=k.copy_stride ||
           f.tables[2].ptr!=k.tables[2].ptr || f.tables[3].ptr!=k.tables[3].ptr))){
            shadow_mismatches.fetch_add(1,std::memory_order_relaxed);state_=full.state_;return true;}}}
    state_=base.state_;reused_=true;return true;
}
std::unique_ptr<OwnedNativeModelDraw> PrepareNativeModelDraw(ID3D12Device* device,NativeModelDrawInput input,const OwnedNativeModelDraw* base){
    auto draw=std::unique_ptr<OwnedNativeModelDraw>(new OwnedNativeModelDraw(std::move(input)));
    if(!base || !draw->Adopt(device,*base))draw->Prepare(device);
    return draw;
}
bool EnvelopeMatches(const NativeModelDrawInput& input,ModelPassEnvelope expected,std::span<const DXGI_FORMAT> formats,DXGI_FORMAT depth_format) noexcept {
    const auto targets=EnvelopeSpec(expected).targets;
    if(input.shape.envelope!=expected || input.render_target_count!=targets || formats.size()!=targets || input.depth_format!=depth_format)return false;
    for(size_t target=0;target<targets;++target)if(input.render_target_formats[target]!=formats[target])return false;
    return true;
}
}
namespace ds3native {
std::vector<std::array<uint16_t,6>> NativeModelRootShapes(){const std::lock_guard hold(root_shapes_lock);return root_shapes;}
}
namespace ds3native {
uint64_t TakeModelDrawGlobalChunkPeak() noexcept {return global_chunks_in_use_peak.exchange(0);}
uint64_t TakeModelDrawGlobalBumpPeak() noexcept {return global_bump_chunks_peak.exchange(0);}
ModelDrawCounters NativeModelDrawCounters() noexcept {return {pso_created,global_table_draws,legacy_table_draws,{prepare_ticks[0],prepare_ticks[1],prepare_ticks[2],prepare_ticks[3]}};}
void SetModelDrawLegacyTables(bool legacy) noexcept {legacy_tables=legacy;}
void SetModelDrawShadowPeriod(uint32_t period) noexcept {shadow_period=period;}
uint64_t ModelDrawShadowMismatches() noexcept {return shadow_mismatches;}
}
