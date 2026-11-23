#include "game_ui_mesh_owner.h"
#include "../content/acquire.h"   // C7-0 rule: in-process field reads through the installed guarded copy
#include <atomic>
#include <cstring>
#include <stdexcept>
#include <optional>
#include <source_location>
#include <cstdio>
#include <intrin.h>

namespace ds3native::bootstrap {
namespace {
template<class T>T Load(uintptr_t p,const std::source_location location=std::source_location::current()){T value{};
    if(!p || !ds3content::Read(p,&value,sizeof(value))){
        char reason[160]{};sprintf_s(reason,"native mesh CPU field unreadable: line%u address%llX bytes%zu",location.line(),static_cast<unsigned long long>(p),sizeof(value));
        throw std::runtime_error(reason);}
    return value;
}
template<class T>void Put(uintptr_t p,T value){std::memcpy(reinterpret_cast<void*>(p),&value,sizeof(value));}
template<class F>F Entry(uintptr_t module,uint32_t rva){return reinterpret_cast<F>(module+rva);}
struct MeshSet {uintptr_t meshes;uint32_t count,pad;uint64_t stride;uint32_t hash,pad2;};
static_assert(sizeof(MeshSet)==32);
CpuPrimitiveVertexFormat ReadFormat(uintptr_t address){
    struct Format {uint32_t stride,pad;uintptr_t elements;};const auto layout=Load<Format>(address);
    if(!layout.stride || layout.stride>256 || !layout.elements)throw std::runtime_error("native CPU mesh format extent");
    CpuPrimitiveVertexFormat result;result.stride=layout.stride;
    for(size_t n=0;n<16;++n){const auto element=Load<CpuPrimitiveVertexElement>(layout.elements+n*8);
        if(!element.attributes){if(result.elements.empty())throw std::runtime_error("empty native CPU mesh format");return result;}
        result.elements.push_back(element);
    }
    throw std::runtime_error("unterminated native CPU mesh format");
}
void QualifiedUnstagedMesh(uintptr_t module,uintptr_t mesh,const std::source_location location=std::source_location::current()){
    const auto provider=Load<uintptr_t>(mesh+0x50);
    const auto provider_table=Load<uintptr_t>(provider);
    const bool normal=provider_table==module+0x3AE0C90 && Load<uintptr_t>(provider_table+0x20)==module+0x1C689F0;
    const bool text=provider_table==module+0x3AE5350 && Load<uintptr_t>(provider_table+0x18)==module+0x1CFB810 &&
        Load<uintptr_t>(provider_table+0x20)==module+0x1C3BA40; // Actual CPU invalidate is RET.
    // D0 selects actual CPU direct-cache output (1CF7C10), not GPU readiness.
    // Both staging-free modes use our Allocate/Map/Seal and owned CPU spans.
    if(Load<uintptr_t>(mesh)!=module+0x3AE0BE0 || (!normal && !text) || Load<uintptr_t>(mesh+0x58) || Load<uint8_t>(mesh+0xD0)>1){
        const auto mesh_table=Load<uintptr_t>(mesh);
        char reason[256]{};sprintf_s(reason,"unqualified native CPU mesh: caller%u meshVT%llX providerVT%llX invalidate%llX data58%llX flagD0%X",
            location.line(),static_cast<unsigned long long>(mesh_table-module),static_cast<unsigned long long>(provider_table-module),
            static_cast<unsigned long long>(Load<uintptr_t>(provider_table+0x20)-module),
            static_cast<unsigned long long>(Load<uintptr_t>(mesh+0x58)),mesh_table==module+0x3AE0BE0?Load<uint8_t>(mesh+0xD0):0xFFFFu);
        throw std::runtime_error(reason);
    }
}
}
struct GameUiMeshOwner::State {
    uintptr_t module{},cache{};Failure failure{};
    std::recursive_mutex mutex;
    PrimitiveGeometryStorage storage;
    std::unordered_map<uintptr_t,PrimitiveGeometryStorage::Writer> items;
    size_t items_witnessed{};   // G5: the largest live count reported (under mutex): one row per power of two, not one per frame at it
    std::array<uintptr_t,19> table{};
    std::array<uintptr_t,3> secondary{};
    std::array<uintptr_t,1> item_table{};
    bool closed{};
    std::optional<CpuUiShapeTolerances> shape_policy;
    [[noreturn]] void Fail(const char* why) const noexcept {failure(why);std::terminate();}
    uintptr_t Heap() const{return Load<uintptr_t>(module+0x4936CC0);}
    uintptr_t Allocate(size_t bytes) const {
        const auto heap=Heap();const uint32_t hint=0x48;
        const auto p=reinterpret_cast<uintptr_t(*)(uintptr_t,size_t,const uint32_t*)>(Load<uintptr_t>(Load<uintptr_t>(heap)+0x50))(heap,bytes,&hint);
        if(!p)throw std::bad_alloc();return p;
    }
    void Free(uintptr_t p) const {const auto heap=Heap();reinterpret_cast<void(*)(uintptr_t,uintptr_t)>(Load<uintptr_t>(Load<uintptr_t>(heap)+0x60))(heap,p);}
};
namespace {
std::atomic<void(*)(size_t)> items_witness{};
std::mutex owners_mutex;
std::unordered_map<uintptr_t,std::shared_ptr<GameUiMeshOwner::State>> cache_owners;
auto Owner(uintptr_t cache){const std::lock_guard<std::mutex> hold(owners_mutex);
    const auto it=cache_owners.find(cache);
    if(it==cache_owners.end())throw std::runtime_error("native mesh cache owner unavailable");return it->second;
}
void BindCache(const std::shared_ptr<GameUiMeshOwner::State>& state,uintptr_t cache){
    // Exact CPU constructor state only. An initialized original GPU cache or
    // previously used CPU staging/item pool is not an admissible owner input.
    if(Load<uintptr_t>(cache)!=state->module+0x3B4E570 || Load<uintptr_t>(cache+8)!=state->module+0x3B4E608 ||
       Load<uintptr_t>(cache+0x10)!=state->Heap() || Load<uintptr_t>(cache+0x68) || Load<uint64_t>(cache+0x70) ||
       Load<uint8_t>(cache+0x78) || Load<uintptr_t>(cache+0x90)!=cache+0x80 || Load<uintptr_t>(cache+0x98) ||
       Load<uintptr_t>(cache+0x2D*8) || Load<uintptr_t>(cache+0x36*8))
        throw std::runtime_error("native embedded mesh cache is not a fresh CPU constructor");
    if(Load<uintptr_t>(cache+0xC8)!=cache)throw std::runtime_error("native embedded cache CPU item owner");
    for(uint32_t n=0;n<6;++n){const auto head=cache+0xC8+size_t(n)*0x18;
        if(Load<uintptr_t>(head+8)!=head || Load<uintptr_t>(head+0x10)!=head || Load<uint64_t>(head+0x18))
            throw std::runtime_error("native embedded cache already contains CPU items");
    }
    {const std::lock_guard<std::mutex> hold(owners_mutex);
        if(!cache_owners.emplace(cache,state).second)throw std::runtime_error("native mesh cache already has an owner");
    }
    state->cache=cache;
    Put<uintptr_t>(cache,reinterpret_cast<uintptr_t>(state->table.data()));Put<uintptr_t>(cache+8,reinterpret_cast<uintptr_t>(state->secondary.data()));
}
auto ItemOwner(uintptr_t item){return Owner(Load<uintptr_t>(Load<uintptr_t>(item+0x38)));}
template<size_t Slot>[[noreturn]] void Unsupported(uintptr_t cache){char reason[96]{};
    sprintf_s(reason,"unqualified_native_mesh_cache_method_%03zX",Slot*8);Owner(cache)->Fail(reason);}
template<size_t... Slots>void DefaultMethods(GameUiMeshOwner::State& state,std::index_sequence<Slots...>){
    state.table={reinterpret_cast<uintptr_t>(&Unsupported<Slots>)...};}
[[noreturn]] void UnsupportedSecondary(uintptr_t secondary){Owner(secondary-8)->Fail("unqualified_native_mesh_cache_secondary");}
uintptr_t DestroyItem(uintptr_t item,uint32_t flags){const auto state=ItemOwner(item);const std::lock_guard<std::recursive_mutex> hold(state->mutex);
    if(Load<uint32_t>(item+0x30)!=2 || Load<uintptr_t>(item+0x18) || Load<uintptr_t>(item+0x68) || Load<uintptr_t>(item+0x70))
        state->Fail("native_mesh_item_cpu_destructor_contract");
    state->storage.Withdraw(item);state->items.erase(item);
    Entry<void(*)(uintptr_t)>(state->module,0x1CF7760)(item);
    if(flags&1)state->Free(item);return item;
}
void RetireItem(const std::shared_ptr<GameUiMeshOwner::State>& state,uintptr_t item){
    if(!state->items.contains(item))state->Fail("native_mesh_retire_foreign_item");
    // Unstaged normal meshes use the actual qualified CPU provider invalidation,
    // including its CPU registry/ref retirement. No old GPU retirement enters.
    const auto type=Load<uint32_t>(item+0x30);
    if(Load<uintptr_t>(item+0x18) || type>1)state->Fail("native_mesh_retire_cpu_family");
    const auto count=Load<uint32_t>(item+0x58);
    const auto meshes=Load<uintptr_t>(item+0x48);
    for(uint32_t n=0;n<count;++n){const auto mesh=Load<uintptr_t>(meshes+size_t(n)*8);
        if(!type && !Load<uint64_t>(mesh+0x28))QualifiedUnstagedMesh(state->module,mesh);
    }
    // Actual CPU detach removes list/hash/mesh associations, preserves temporary
    // CPU retain/release, invokes our destructor(0), then frees via engine heap.
    Entry<void(*)(uintptr_t,uintptr_t,uint8_t)>(state->module,0x1CF9020)(item,0,1);
}
uint32_t AllocateItem(uintptr_t cache,uintptr_t output,uint32_t type,uintptr_t mesh_set,uint64_t bytes,
    uint32_t vertices,uint32_t indices,uint8_t){const auto state=Owner(cache);const std::lock_guard<std::recursive_mutex> hold(state->mutex);
    if(state->closed || !output)state->Fail("native_mesh_allocation_after_close");Put<uintptr_t>(output,0);
    const auto set=Load<MeshSet>(mesh_set);
    // Actual cache60/1F7D0A0 has eight arguments, no vertex-format pointer.
    // Single-mesh allocations declare exact vertex bytes and count; derive
    // their storage stride without reading an undefined ninth stack argument.
    const auto stride=vertices && bytes%vertices==0?bytes/vertices:0;
    if(type>1 || !set.count || set.count>24 || (type==1 && set.count!=1) || !set.meshes || set.stride<8 || set.stride>4096 || !stride || stride>256 ||
       !vertices || vertices>65536 || !indices || bytes!=uint64_t(vertices)*stride || bytes>(16u<<20) || indices>(8u<<20)){char reason[240]{};
        // I-0137 (I-0120 G5): no bound on the number of live items any more (4,096 closed the game). An item lives as long as
        // the engine's mesh (EvictItem and DestroyItem below, called when the mesh dies); the original evicts by its GPU pool's
        // budget, which has no native equivalent: the native items are CPU memory, as many as the interface keeps meshes.
        sprintf_s(reason,"native_mesh_allocation_cpu_extent_type%u_meshes%u_step%llu_bytes%llu_vertices%u_indices%u_stride%llu_caller%llX",
            type,set.count,static_cast<unsigned long long>(set.stride),static_cast<unsigned long long>(bytes),vertices,indices,
            static_cast<unsigned long long>(stride),static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(_ReturnAddress())-state->module));
        state->Fail(reason);}
    for(uint32_t n=0;n<set.count;++n){const auto mesh=Load<uintptr_t>(set.meshes+size_t(n)*set.stride);
        if(!type){if(!Load<uint64_t>(mesh+0x28))QualifiedUnstagedMesh(state->module,mesh);}
        else {
            const auto provider=Load<uintptr_t>(mesh+0x50),vtable=Load<uintptr_t>(provider);
            if(Load<uintptr_t>(mesh+0x28) || Load<uintptr_t>(mesh+0xA8) ||
               !((vtable==state->module+0x3AE0C90 && Load<uintptr_t>(vtable+0x20)==state->module+0x1C689F0) ||
                 (vtable==state->module+0x3AE5350 && Load<uintptr_t>(vtable+0x20)==state->module+0x1C3BA40)))
                state->Fail("native_complex_mesh_cpu_provider");
            const auto fills=Load<uint64_t>(mesh+0xE8),records=Load<uintptr_t>(mesh+0xE0);
            if(!fills || fills>1024 || !records)state->Fail("native_complex_mesh_cpu_fills");
            uint64_t vertex_total{},index_total{};
            for(uint64_t fill=0;fill<fills;++fill){const auto record=records+fill*0x38;
                const auto format=ReadFormat(Load<uintptr_t>(record+8));
                const auto count=Load<uint32_t>(record+0x28),index_count=Load<uint32_t>(record+0x1C);
                if(format.stride!=stride || !count || !index_count ||
                   Load<uint64_t>(record+0x20)!=vertex_total*stride || Load<uint32_t>(record+0x18)!=index_total)
                    state->Fail("native_complex_mesh_uniform_cpu_layout");
                vertex_total+=count;index_total+=index_count;
            }
            if(vertex_total!=vertices || index_total!=indices)state->Fail("native_complex_mesh_cpu_counts");
        }
    }
    const auto item=state->Allocate(0x98+size_t(set.count)*8);
    PrimitiveGeometryStorage::Writer writer;
    try{writer=state->storage.Allocate(item,static_cast<uint32_t>(stride),vertices,indices);}catch(...){state->Free(item);throw;}
    using Construct=uintptr_t(*)(uintptr_t,uint32_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uint64_t,uint64_t,uint32_t,uint64_t,uint64_t,uint32_t);
    Entry<Construct>(state->module,0x1F7CA70)(item,type,cache+0xC8,mesh_set,0,0,0,bytes,vertices,0,uint64_t(indices)*2,indices);
    Put<uintptr_t>(item,reinterpret_cast<uintptr_t>(state->item_table.data()));
    try{state->items.emplace(item,std::move(writer));}
    catch(...){state->Fail("native_mesh_cpu_item_publication_failed");}
    if(const auto live=state->items.size();live>=4096 && !(live&(live-1)) && live>state->items_witnessed){state->items_witnessed=live;   // G5
        if(const auto witness=items_witness.load(std::memory_order_relaxed))witness(live);}
    Put<uintptr_t>(output,item);return 3; // Actual owned CPU write allocation.
}
void MapItem(uintptr_t cache,uintptr_t item,uintptr_t* vertices,uintptr_t* indices){const auto state=Owner(cache);const std::lock_guard<std::recursive_mutex> hold(state->mutex);
    if(state->closed || !vertices || !indices || !state->items.contains(item))state->Fail("native_mesh_map_foreign_or_closed");
    const auto& writer=state->items.at(item);*vertices=reinterpret_cast<uintptr_t>(writer->Vertices());*indices=reinterpret_cast<uintptr_t>(writer->Indices());
}
void SealItem(uintptr_t cache,uintptr_t item){const auto state=Owner(cache);const std::lock_guard<std::recursive_mutex> hold(state->mutex);
    if(state->closed || !state->items.contains(item))state->Fail("native_mesh_seal_foreign_or_closed");
    state->storage.Seal(state->items.at(item));
}
uint64_t EvictItem(uintptr_t cache,uintptr_t item,uintptr_t pool,uintptr_t dying_mesh){
    const auto state=Owner(cache);const std::lock_guard<std::recursive_mutex> hold(state->mutex);
    if(pool || !state->items.contains(item))state->Fail("native_mesh_evict_foreign_pool_or_item");
    const auto type=Load<uint32_t>(item+0x30);
    if(Load<uintptr_t>(item+0x18) || Load<uintptr_t>(item+0x68) || Load<uintptr_t>(item+0x70) || type>1)
        state->Fail("native_mesh_evict_gpu_or_retired_item");
    const auto count=Load<uint32_t>(item+0x58);const auto meshes=Load<uintptr_t>(item+0x48);bool found=!dying_mesh;
    for(uint32_t n=0;n<count;++n){const auto mesh=Load<uintptr_t>(meshes+size_t(n)*8);found|=mesh==dying_mesh;
        if(!type && mesh!=dying_mesh && !Load<uint64_t>(mesh+0x28))QualifiedUnstagedMesh(state->module,mesh);
    }
    if(!found)state->Fail("native_mesh_evict_foreign_mesh");
    // Nil old GPU buffers imply zero freed old-pool bytes. Immutable geometry
    // leases survive immediate CPU item destruction, so no old fence is needed.
    Entry<void(*)(uintptr_t,uintptr_t,uint8_t)>(state->module,0x1CF9020)(item,dying_mesh,1);return 0;
}
void WriteMeshes(uintptr_t cache,uintptr_t,uintptr_t batch,uintptr_t mesh_set,uint8_t wait) try {
    const auto state=Owner(cache);const std::lock_guard<std::recursive_mutex> hold(state->mutex);
    if(state->closed || !state->shape_policy || !batch)state->Fail("native_cpu_mesh_writer_scope");
    const auto previous=Load<uintptr_t>(batch+0x20);
    if(previous && !state->items.contains(previous))state->Fail("native_cpu_mesh_writer_foreign_bound_batch");
    const auto set=Load<MeshSet>(mesh_set);
    if(!set.count || set.count>24 || !set.meshes || set.stride<8 || set.stride>4096)
        state->Fail("native_cpu_mesh_writer_extent");
    const auto primitive=Load<uintptr_t>(batch+0x40),fill=Load<uintptr_t>(primitive+0x18);
    const auto fill_type=Load<uint32_t>(fill+0x10);const auto destination_pointer=Load<uintptr_t>(batch+0x30);
    // The actual normal-shape generator selects tessellation from the mesh's
    // stroke/style data, independently of the primitive's image/gradient fill.
    // Admit those real fill intents only through exact generator-format matching
    // below; shader/image support is checked again when producing the draw.
    // Fill1 uses the same genuine normal CPU generator/format transaction;
    // its actual shader selector is the untextured0x80000 family (1F79270).
    if(fill_type<1 || fill_type>15){char reason[96]{};
        sprintf_s(reason,"native_cpu_mesh_writer_fill_%u_unavailable",fill_type);state->Fail(reason);}
    const auto source_format=ReadFormat(Load<uintptr_t>(fill+0x30)),destination=ReadFormat(destination_pointer);
    GameUiCpuShapeContext context(state->module,*state->shape_policy,state->failure);
    std::vector<uintptr_t> inputs;std::vector<CpuUiMeshOutput> outputs;inputs.reserve(set.count);outputs.reserve(set.count);
    for(uint32_t n=0;n<set.count;++n){const auto mesh=Load<uintptr_t>(set.meshes+size_t(n)*set.stride);
        QualifiedUnstagedMesh(state->module,mesh);
        if(Load<uint64_t>(mesh+0x28))state->Fail("native_cpu_mesh_writer_original_staging");
        const auto provider=Load<uintptr_t>(mesh+0x50);const auto index=Load<uint32_t>(mesh+0x84);
        uintptr_t receiver{};uint32_t generator{};
        if(Load<uintptr_t>(provider)==state->module+0x3AE0C90){
            const auto delegate=Load<uintptr_t>(provider+0x20),leaf_table=Load<uintptr_t>(delegate+0x10),styles=Load<uintptr_t>(delegate+0x28);
            // 1171: the style word +0xC (non-zero in the character creation menu) no longer refuses by itself: the same
            // genuine generator 1C3AB10 runs on our CPU interface, and its output format is checked below.
            // 1216: the same delegate also carries the leaf class 3AE0EB0 (Firelink, Native1215), whose generate slot (+0x18)
            // is 1CFB9C0; it runs on the same CPU interface and its output is checked below like 1C3AB10's.
            const auto leaf_generator=leaf_table?Load<uintptr_t>(leaf_table+0x18):0;
            const bool known_leaf=(leaf_table==state->module+0x3ADF440 && leaf_generator==state->module+0x1C3AB10) ||
                (leaf_table==state->module+0x3AE0EB0 && leaf_generator==state->module+0x1CFB9C0);
            if(!known_leaf || index>=4096 || !styles){
                // 1216: name the refused shape (Native1215, teleported to Firelink): leaf table and generator RVAs, index, styles.
                const auto generator_rva=leaf_generator?leaf_generator-state->module:0;char reason[160]{};
                sprintf_s(reason,"native_cpu_mesh_writer_generator_family_%llx_%llx_%u_%d",static_cast<unsigned long long>(leaf_table-state->module),
                    static_cast<unsigned long long>(generator_rva),index,styles?1:0);state->Fail(reason);}
            receiver=delegate+0x10;generator=static_cast<uint32_t>(leaf_generator-state->module);
        }else{
            const auto layers=Load<uintptr_t>(provider+0x68),count=Load<uint64_t>(provider+0x70);
            if(!layers || count>4096 || index>=count)state->Fail("native_cpu_text_layer_extent");
            const auto kind=Load<uint32_t>(layers+size_t(index)*0x40);
            // Actual glyph/shadow quads and rectangle layers write solely to
            // our five-method CPU output. Vector outlines and image UV getters
            // have separate dependencies and remain explicit unsupported kinds.
            // 1119: kind 7 is an inline image quad (1CFB810 case 7): TextureImage::GetTexture ([image+10]),
            // the texture's UV-gen matrix 1F83AC0 from the CPU extent of our native image, the optional image
            // matrix (1BCFB00), then four pos/uv vertices and six indices to the same output. All CPU.
            // Menu tests FL6 (Native2055, Simplified Chinese at the load): kind 1 (1D00920) fills the layer's clipped rectangles (records
            // +0x48, 0x28 each) as a path through the context's scratch tessellator (+0xF0) and the policy, then the same output: all CPU.
            if(kind!=0 && kind!=1 && kind!=2 && kind!=3 && kind!=4 && kind!=5 && kind!=6 && kind!=7 && kind!=10 && kind!=11){char reason[96]{};
                sprintf_s(reason,"native_cpu_text_layer_%u_unavailable",kind);state->Fail(reason);}
            receiver=provider;generator=0x1CFB810;
        }
        CpuUiMeshOutput output;
        if(ProduceNativeCpuUiMesh(Entry<CpuUiMeshGenerator>(state->module,generator),receiver,context.Interface(),
            mesh,Load<uint32_t>(mesh+0x88),output)!=CpuUiMeshOutputStatus::Ready)state->Fail("native_cpu_mesh_writer_generator_output");
        if(output.format.stride!=source_format.stride || output.format.elements.size()!=source_format.elements.size() ||
           !std::equal(output.format.elements.begin(),output.format.elements.end(),source_format.elements.begin(),
              [](const auto& a,const auto& b){return a.offset==b.offset && a.attributes==b.attributes;}))
            state->Fail("native_cpu_mesh_writer_source_format");
        inputs.push_back(mesh);outputs.push_back(std::move(output));
    }
    context.Close();
    std::vector<CpuStagedPrimitiveMesh> generated;generated.reserve(outputs.size());
    for(const auto& output:outputs)generated.push_back({output.vertices,output.indices,output.vertex_bytes,output.index_bytes});
    PrimitiveGeometryStorage transaction;
    const auto geometry=ProduceStagedPrimitiveGeometry(transaction,batch,source_format,destination,generated);
    uintptr_t item{};
    if(AllocateItem(cache,reinterpret_cast<uintptr_t>(&item),0,mesh_set,geometry->vertices.bytes.size(),
        geometry->vertex_count,geometry->index_count,wait)!=3)state->Fail("native_cpu_mesh_writer_allocate");
    uintptr_t vertices{},indices{};MapItem(cache,item,&vertices,&indices);
    std::memcpy(reinterpret_cast<void*>(vertices),geometry->vertices.bytes.data(),geometry->vertices.bytes.size());
    std::memcpy(reinterpret_cast<void*>(indices),geometry->indices.bytes.data(),geometry->indices.bytes.size());SealItem(cache,item);
    // Publish the new association before retiring the previous item so the
    // provider cannot invalidate a mesh still needed by this new transaction.
    if(previous){Entry<void(*)(uintptr_t,uintptr_t)>(state->module,0x1CF6A50)(batch,0);RetireItem(state,previous);}
    // Publish real generator metadata only after the native immutable geometry
    // is ready. M+28 remains zero: no invented old staging/readiness flag.
    for(size_t n=0;n<inputs.size();++n){Put<uint32_t>(inputs[n]+0x44,outputs[n].vertices);Put<uint32_t>(inputs[n]+0x48,outputs[n].indices);
        std::memcpy(reinterpret_cast<void*>(inputs[n]+0xB0),outputs[n].transform.data(),sizeof(outputs[n].transform));
    }
    Entry<void(*)(uintptr_t,uintptr_t)>(state->module,0x1CF6A50)(batch,item);
}catch(const std::exception& error){
    // Refuse at the native writer boundary. The game's CPU render cleanup can
    // otherwise mask a conversion exception behind an unfinished display scope.
    Owner(cache)->Fail(error.what());
}
void CloseCache(const std::shared_ptr<GameUiMeshOwner::State>& state){const std::lock_guard<std::recursive_mutex> hold(state->mutex);
    if(state->closed)return;state->closed=true;
    while(!state->items.empty())RetireItem(state,state->items.begin()->first);
    // The owner has not admitted unknown CPU staging-provider work. Its actual
    // base destructor frees the owned CPU hash/staging allocation, never GPU.
    if(Load<uintptr_t>(state->cache+0x90)!=state->cache+0x80 ||
       Load<uintptr_t>(state->cache+0x2D*8) || Load<uintptr_t>(state->cache+0x36*8))state->Fail("native_mesh_cache_cpu_close_contract");
    Entry<void(*)(uintptr_t)>(state->module,0x1CF76B0)(state->cache);
}
uintptr_t DestroyCache(uintptr_t cache,uint32_t flags){const auto state=Owner(cache);const std::lock_guard<std::recursive_mutex> hold(state->mutex);
    if(flags&1)state->Fail("native_embedded_cache_storage_delete");   // the storage is the HAL's (AdoptCache)
    if(state->cache!=cache)state->Fail("native_mesh_cache_duplicate_destruction");CloseCache(state);
    {const std::lock_guard<std::mutex> registry_hold(owners_mutex);cache_owners.erase(cache);}
    state->cache=0;return cache;
}
uintptr_t DestroySecondary(uintptr_t secondary,uint32_t flags){return DestroyCache(secondary-8,flags);}
}
void SetUiMeshItemsWitness(void(*witness)(size_t live)) noexcept {items_witness.store(witness,std::memory_order_relaxed);}
GameUiMeshOwner::GameUiMeshOwner(uintptr_t module,Failure failure){
    if(!module || !failure)throw std::invalid_argument("native mesh owner services");
    state_=std::make_shared<State>();state_->module=module;state_->failure=failure;
    DefaultMethods(*state_,std::make_index_sequence<19>{});
    state_->secondary={reinterpret_cast<uintptr_t>(&DestroySecondary),reinterpret_cast<uintptr_t>(&UnsupportedSecondary),module+0x1CF8560};
    state_->table[0]=reinterpret_cast<uintptr_t>(&DestroyCache);state_->table[0x60/8]=reinterpret_cast<uintptr_t>(&AllocateItem);
    // Pure CPU single-cache policy, frame entry and current map flag. These
    // are not a legacy GPU allocation or fence-ready result.
    state_->table[0x10/8]=module+0x1F7E060;state_->table[0x18/8]=module+0x1F7D360;
    state_->table[0x30/8]=module+0x1F7D350;
    state_->table[0x40/8]=reinterpret_cast<uintptr_t>(&EvictItem);state_->table[0x48/8]=reinterpret_cast<uintptr_t>(&WriteMeshes);
    state_->table[0x50/8]=module+0x1CF8D60;state_->table[0x58/8]=module+0x1CF8D30;
    state_->table[0x68/8]=reinterpret_cast<uintptr_t>(&MapItem);state_->table[0x70/8]=reinterpret_cast<uintptr_t>(&SealItem);
    state_->item_table[0]=reinterpret_cast<uintptr_t>(&DestroyItem);
}
GameUiMeshOwner::~GameUiMeshOwner(){Close();if(state_->cache)DestroyCache(state_->cache,0u);}
void* GameUiMeshOwner::AdoptCache(void* constructed){const std::lock_guard<std::recursive_mutex> hold(state_->mutex);
    if(!constructed || state_->closed || state_->cache)throw std::runtime_error("native embedded cache owner state");
    BindCache(state_,reinterpret_cast<uintptr_t>(constructed));return constructed;
}
void GameUiMeshOwner::SetShapeTolerances(const CpuUiShapeTolerances& policy){const std::lock_guard<std::recursive_mutex> hold(state_->mutex);
    if(state_->closed)throw std::runtime_error("closed native CPU mesh policy");
    // The constructor validates the exact tolerance layout and proves its
    // scratch lifetime before admitting this policy for subsequent writes.
    GameUiCpuShapeContext validation(state_->module,policy,state_->failure);validation.Close();state_->shape_policy=policy;
}
std::shared_ptr<const PrimitiveGeometry> GameUiMeshOwner::Acquire(uintptr_t item) const {return state_->storage.Acquire(item);}
void GameUiMeshOwner::Close(){const std::lock_guard<std::recursive_mutex> hold(state_->mutex);
    if(state_->cache)CloseCache(state_);else state_->closed=true;
}
}
