#include "game_ui_mesh_output.h"
#include "../content/acquire.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace ds3native::bootstrap {
namespace {
bool Copy(uintptr_t source,void* destination,size_t bytes) noexcept {
    if(!bytes)return true;
    // C7-0 rule: through the installed guarded copy (ReadProcessMemory where none is installed).
    return source && bytes<=UINTPTR_MAX-source && ds3content::Read(source,destination,bytes);
}
struct Fill {uint32_t vertices,indices;uintptr_t format;std::array<uint32_t,2> fills;uint32_t flags,ordinal;};
struct Format {uint32_t stride,pad;uintptr_t elements;};
static_assert(sizeof(Fill)==32 && sizeof(Format)==16);
}
struct NativeCpuMeshOutput::State {
    struct InterfacePrefix {uintptr_t table;State* state;} abi_instance{};
    std::array<uintptr_t,5> table{};
    mutable std::mutex mutex;
    std::thread::id thread{std::this_thread::get_id()};
    CpuUiMeshOutput data;
    std::vector<uint8_t> vertex_coverage,index_coverage;
    CpuUiMeshOutputStatus status{CpuUiMeshOutputStatus::Missing};
    bool begun{},ended{};
    bool Current() noexcept {
        if(thread!=std::this_thread::get_id()){status=CpuUiMeshOutputStatus::Invalid;return false;}
        if(status==CpuUiMeshOutputStatus::Ready){status=CpuUiMeshOutputStatus::Invalid;return false;}
        return status==CpuUiMeshOutputStatus::Missing;
    }
};
namespace {
auto& Owner(uintptr_t object){return *reinterpret_cast<NativeCpuMeshOutput::State::InterfacePrefix*>(object)->state;}
uintptr_t Destroy(uintptr_t object,uint32_t) noexcept {
    auto& state=Owner(object);const std::lock_guard<std::mutex> hold(state.mutex);
    // Borrowed synchronous output has no deleting ownership transfer.
    state.status=CpuUiMeshOutputStatus::Invalid;return object;
}
uint8_t Begin(uintptr_t object,uintptr_t records,uint64_t count,uintptr_t matrix) noexcept {
    auto& state=Owner(object);const std::lock_guard<std::mutex> hold(state.mutex);
    try{
        if(!state.Current())return 0;
        if(state.begun || state.ended){state.status=CpuUiMeshOutputStatus::Invalid;return 0;}
        if(count!=1){state.status=CpuUiMeshOutputStatus::Unsupported;return 0;}
        Fill fill{};Format format{};std::array<float,8> transform{};
        if(!Copy(records,&fill,sizeof(fill)) || !Copy(fill.format,&format,sizeof(format)) ||
           !Copy(matrix,transform.data(),sizeof(transform))){state.status=CpuUiMeshOutputStatus::Invalid;return 0;}
        if(!fill.vertices || fill.vertices>65536 || !fill.indices || fill.indices>(8u<<20) ||
           !format.stride || format.stride>256 || uint64_t(fill.vertices)*format.stride>(16u<<20) ||
           !format.elements){state.status=CpuUiMeshOutputStatus::Unsupported;return 0;}
        if(!std::all_of(transform.begin(),transform.end(),[](float f){return std::isfinite(f);})){state.status=CpuUiMeshOutputStatus::Invalid;return 0;}
        CpuPrimitiveVertexFormat owned;owned.stride=format.stride;bool terminated{};
        for(uint32_t n=0;n<16;++n){CpuPrimitiveVertexElement element{};
            const auto offset=size_t(n)*sizeof(element);
            if(offset>UINTPTR_MAX-format.elements || !Copy(format.elements+offset,&element,sizeof(element))){state.status=CpuUiMeshOutputStatus::Invalid;return 0;}
            if(!element.attributes){terminated=true;break;}
            if(element.offset>=format.stride){state.status=CpuUiMeshOutputStatus::Invalid;return 0;}
            owned.elements.push_back(element);
        }
        if(!terminated || owned.elements.empty()){state.status=CpuUiMeshOutputStatus::Unsupported;return 0;}
        CpuUiMeshOutput next;next.format=std::move(owned);next.vertices=fill.vertices;next.indices=fill.indices;
        next.fills=fill.fills;next.flags=fill.flags;next.mesh_ordinal=fill.ordinal;next.transform=transform;
        next.vertex_bytes.resize(size_t(fill.vertices)*format.stride);next.index_bytes.resize(size_t(fill.indices)*2);
        state.vertex_coverage.resize(fill.vertices);state.index_coverage.resize(fill.indices);
        state.data=std::move(next);state.begun=true;return 1;
    }catch(...){state.status=CpuUiMeshOutputStatus::Invalid;return 0;}
}
void Write(uintptr_t object,uint32_t fill,uint32_t offset,uintptr_t source,uint32_t count,bool vertices) noexcept {
    auto& state=Owner(object);const std::lock_guard<std::mutex> hold(state.mutex);
    if(!state.Current())return;
    if(!state.begun || state.ended || fill){state.status=CpuUiMeshOutputStatus::Invalid;return;}
    auto& bytes=vertices?state.data.vertex_bytes:state.data.index_bytes;
    auto& coverage=vertices?state.vertex_coverage:state.index_coverage;
    const size_t stride=vertices?state.data.format.stride:2;
    if(offset>coverage.size() || count>coverage.size()-offset || !Copy(source,bytes.data()+size_t(offset)*stride,size_t(count)*stride)){
        state.status=CpuUiMeshOutputStatus::Invalid;return;
    }
    std::fill(coverage.begin()+offset,coverage.begin()+offset+count,uint8_t{1});
}
void Vertices(uintptr_t object,uint32_t fill,uint32_t offset,uintptr_t source,uint32_t count) noexcept {Write(object,fill,offset,source,count,true);}
void Indices(uintptr_t object,uint32_t fill,uint32_t offset,uintptr_t source,uint32_t count) noexcept {Write(object,fill,offset,source,count,false);}
void End(uintptr_t object) noexcept {
    auto& state=Owner(object);const std::lock_guard<std::mutex> hold(state.mutex);
    if(!state.Current())return;
    if(!state.begun || state.ended){state.status=CpuUiMeshOutputStatus::Invalid;return;}
    state.ended=true;
    if(std::find(state.vertex_coverage.begin(),state.vertex_coverage.end(),uint8_t{0})!=state.vertex_coverage.end() ||
       std::find(state.index_coverage.begin(),state.index_coverage.end(),uint8_t{0})!=state.index_coverage.end())return;
    for(size_t n=0;n<state.data.index_bytes.size();n+=2){
        const auto index=uint32_t(state.data.index_bytes[n]) | (uint32_t(state.data.index_bytes[n+1])<<8);
        if(index>=state.data.vertices){state.status=CpuUiMeshOutputStatus::Invalid;return;}
    }
    state.status=CpuUiMeshOutputStatus::Ready;
}
}
NativeCpuMeshOutput::NativeCpuMeshOutput():state_(std::make_unique<State>()){
    state_->table={reinterpret_cast<uintptr_t>(&Destroy),reinterpret_cast<uintptr_t>(&Begin),reinterpret_cast<uintptr_t>(&End),
        reinterpret_cast<uintptr_t>(&Vertices),reinterpret_cast<uintptr_t>(&Indices)};
    state_->abi_instance={reinterpret_cast<uintptr_t>(state_->table.data()),state_.get()};
}
NativeCpuMeshOutput::~NativeCpuMeshOutput()=default;
uintptr_t NativeCpuMeshOutput::Interface() const noexcept {return reinterpret_cast<uintptr_t>(&state_->abi_instance);}
CpuUiMeshOutputStatus NativeCpuMeshOutput::Resolve(CpuUiMeshOutput& output) const {
    const std::lock_guard<std::mutex> hold(state_->mutex);
    if(state_->thread!=std::this_thread::get_id())return CpuUiMeshOutputStatus::Invalid;
    if(state_->status!=CpuUiMeshOutputStatus::Ready)return state_->status;
    auto next=state_->data;output=std::move(next);return CpuUiMeshOutputStatus::Ready;
}
CpuUiMeshOutputStatus ProduceNativeCpuUiMesh(CpuUiMeshGenerator generator,uintptr_t receiver,uintptr_t renderer,
    uintptr_t mesh,uint32_t flags,CpuUiMeshOutput& output){
    if(!generator || !receiver || !renderer || !mesh)return CpuUiMeshOutputStatus::Invalid;
    NativeCpuMeshOutput native;
    generator(receiver,renderer,mesh,native.Interface(),flags);
    return native.Resolve(output);
}
}
