#include "frame_graph.h"
#include <algorithm>

namespace ds3native {
namespace {
constexpr D3D12_RESOURCE_STATES read_only_states=D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER|D3D12_RESOURCE_STATE_INDEX_BUFFER|
    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT|
    D3D12_RESOURCE_STATE_COPY_SOURCE|D3D12_RESOURCE_STATE_DEPTH_READ;
bool ReadOnly(D3D12_RESOURCE_STATES state) noexcept {return state && !(state&~read_only_states);}
constexpr D3D12_RESOURCE_STATES write_states=D3D12_RESOURCE_STATE_RENDER_TARGET|D3D12_RESOURCE_STATE_UNORDERED_ACCESS|
    D3D12_RESOURCE_STATE_DEPTH_WRITE|D3D12_RESOURCE_STATE_STREAM_OUT|D3D12_RESOURCE_STATE_COPY_DEST|D3D12_RESOURCE_STATE_RESOLVE_DEST;
// G8: a state the graph may require: PRESENT/COMMON (0), one write state alone, or a combination of read-only states.
bool ValidState(D3D12_RESOURCE_STATES state) noexcept {
    if(!state)return true;
    if(state&~(read_only_states|write_states))return false;
    const auto bits=static_cast<uint32_t>(state);
    if(state&write_states)return (bits&(bits-1))==0;   // exactly one bit
    return true;
}
size_t DescOf(const std::vector<FrameResourceDesc>& descs,const ResourceKey& key) noexcept {
    for(size_t i=0;i<descs.size();++i)if(descs[i].key==key)return i;
    return descs.size();
}
// Brings subresources [first, end) of one resource to `to`, as one whole-resource transition when they span the whole
// resource from one common state, else one transition per subresource that needs it.
void Bring(BarrierBatch& batch,const ResourceKey& key,std::vector<D3D12_RESOURCE_STATES>& states,uint32_t first,uint32_t end,
    D3D12_RESOURCE_STATES to,bool exact) {
    bool any=false;for(uint32_t s=first;s<end;++s)if(exact?states[s]!=to:!GraphStateSatisfies(states[s],to)){any=true;break;}
    if(!any)return;
    const bool whole=first==0 && end==states.size() && std::all_of(states.begin(),states.end(),[&](auto state){return state==states[0];});
    if(whole && states.size()>1){batch.transitions.push_back({key,NoPlanIndex,states[0],to});std::fill(states.begin(),states.end(),to);return;}
    for(uint32_t s=first;s<end;++s)if(exact?states[s]!=to:!GraphStateSatisfies(states[s],to)){
        batch.transitions.push_back({key,states.size()==1?NoPlanIndex:s,states[s],to});states[s]=to;}
}
}
bool GraphStateSatisfies(D3D12_RESOURCE_STATES current,D3D12_RESOURCE_STATES required) noexcept {
    return current==required || (ReadOnly(current) && ReadOnly(required) && (current&required)==required);
}
size_t BarrierPlan::TransitionCount() const noexcept {
    size_t n=end.transitions.size();for(const auto& batch:before_pass)n+=batch.transitions.size();
    for(const auto& batch:after_pass)n+=batch.transitions.size();return n;
}
BarrierPlan PlanFrameBarriers(const CompiledFramePlan& compiled,const std::vector<FrameResourceDesc>& descs) {
    BarrierPlan result;
    const auto fail=[&](GraphError error,uint32_t pass=NoPlanIndex,uint32_t resource=NoPlanIndex){
        result.status={error,pass,resource};result.before_pass.clear();result.after_pass.clear();result.end={};result.first_uses.clear();result.use_states.clear();return result;};
    if(!compiled)return fail(GraphError::Plan);
    const auto& plan=compiled.Plan();
    std::vector<size_t> desc_of(plan.resources.size());
    std::vector<std::vector<D3D12_RESOURCE_STATES>> states(plan.resources.size());
    for(size_t r=0;r<plan.resources.size();++r){
        const auto d=DescOf(descs,plan.resources[r].key);
        if(d==descs.size() || !descs[d].subresources || descs[d].subresources>4096)return fail(GraphError::Description,NoPlanIndex,static_cast<uint32_t>(r));
        desc_of[r]=d;states[r].assign(descs[d].subresources,descs[d].start_declared?descs[d].start:descs[d].home);
    }
    std::vector<bool> seen(plan.resources.size());std::vector<uint32_t> last_uav(plan.resources.size(),NoPlanIndex);
    result.before_pass.resize(plan.passes.size());result.after_pass.resize(plan.passes.size());
    std::vector<uint32_t> last_pass(plan.resources.size(),NoPlanIndex);
    for(const auto& lifetime:compiled.Lifetimes())
        for(size_t r=0;r<plan.resources.size();++r)if(plan.resources[r].key==lifetime.key)last_pass[r]=lifetime.last;
    for(size_t p=0;p<plan.passes.size();++p){
        const auto& pass=plan.passes[p];auto& batch=result.before_pass[p];const auto ordinal=static_cast<uint32_t>(p);
        for(size_t u=0;u<pass.resources.size();++u){
            const auto& use=pass.resources[u];
            size_t r{};while(r<plan.resources.size() && plan.resources[r].key!=use.key)++r;   // the compile checked the key
            const auto& desc=descs[desc_of[r]];auto& state=states[r];
            if(!seen[r] && desc.lifetime==FrameLifetime::Transient){
                FirstUse action=FirstUse::None;
                if(use.access!=ResourceAccess::Read)action=pass.kind==PassKind::Clear?FirstUse::Clear:
                    use.tracked && use.required_state==D3D12_RESOURCE_STATE_COPY_DEST?FirstUse::CopyInit:FirstUse::Discard;
                result.first_uses.push_back({use.key,ordinal,action});}
            seen[r]=true;
            if(!use.tracked)continue;
            const auto subresources=static_cast<uint32_t>(state.size());
            if(use.range.first>=subresources || (use.range.count!=NoPlanIndex && uint64_t(use.range.first)+use.range.count>subresources))
                return fail(GraphError::Description,ordinal,static_cast<uint32_t>(r));
            const uint32_t end=use.range.count==NoPlanIndex?subresources:use.range.first+use.range.count;
            const auto required=static_cast<D3D12_RESOURCE_STATES>(use.required_state);
            if(!ValidState(required) || (use.access!=ResourceAccess::Read && (ReadOnly(required) || !required)))
                return fail(GraphError::UnknownState,ordinal,static_cast<uint32_t>(r));
            // A UAV use after a UAV write of an earlier pass, already in that state: a UAV barrier, no transition.
            if(required==D3D12_RESOURCE_STATE_UNORDERED_ACCESS && last_uav[r]!=NoPlanIndex && last_uav[r]<ordinal &&
               std::all_of(state.begin()+use.range.first,state.begin()+end,[](auto s){return s==D3D12_RESOURCE_STATE_UNORDERED_ACCESS;}))
                batch.uav.push_back(use.key);
            Bring(batch,use.key,state,use.range.first,end,required,!ReadOnly(required));
            if(required==D3D12_RESOURCE_STATE_UNORDERED_ACCESS && use.access!=ResourceAccess::Read)last_uav[r]=ordinal;
            for(uint32_t s=use.range.first;s<end;++s)result.use_states.push_back({ordinal,static_cast<uint32_t>(u),s,state[s]});
        }
        // G2: a transient whose last pass this is goes back to its home state now.
        for(size_t r=0;r<plan.resources.size();++r)if(last_pass[r]==ordinal && descs[desc_of[r]].lifetime==FrameLifetime::Transient)
            Bring(result.after_pass[p],plan.resources[r].key,states[r],0,static_cast<uint32_t>(states[r].size()),descs[desc_of[r]].home,true);
    }
    // G5: persistent and external resources back to their boundary state (transients are already home).
    for(size_t r=0;r<plan.resources.size();++r)
        Bring(result.end,plan.resources[r].key,states[r],0,static_cast<uint32_t>(states[r].size()),descs[desc_of[r]].home,true);
    return result;
}
PlacementPlan PlanFramePlacement(const CompiledFramePlan& compiled,const std::vector<FrameResourceDesc>& descs,bool tier2) {
    PlacementPlan result;
    if(!compiled){result.status={GraphError::Plan};return result;}
    struct Item {ResourceKey key;uint64_t size{},alignment{};size_t heap{};uint32_t first{},last{};};
    std::vector<Item> items;
    for(const auto& lifetime:compiled.Lifetimes()){
        const auto d=DescOf(descs,lifetime.key);
        if(d==descs.size()){result.status={GraphError::Description};return result;}
        const auto& desc=descs[d];if(desc.lifetime!=FrameLifetime::Transient)continue;
        if(!desc.size || !desc.alignment || (desc.alignment&(desc.alignment-1))){result.status={GraphError::Description};return result;}
        items.push_back({desc.key,desc.size,desc.alignment,tier2?size_t{0}:static_cast<size_t>(desc.heap_class),lifetime.first,lifetime.last});
    }
    // Largest first; ties by key, so the placement is deterministic.
    std::stable_sort(items.begin(),items.end(),[](const Item& a,const Item& b){
        return a.size!=b.size?a.size>b.size:a.key.identity!=b.key.identity?a.key.identity<b.key.identity:a.key.generation<b.key.generation;});
    struct Placed {uint64_t offset{},end{};size_t heap{};uint32_t first{},last{};};
    std::vector<Placed> placed;
    const auto align=[](uint64_t value,uint64_t alignment){return (value+alignment-1)&~(alignment-1);};
    for(const auto& item:items){
        std::vector<const Placed*> conflicts;
        for(const auto& p:placed)if(p.heap==item.heap && p.first<=item.last && item.first<=p.last)conflicts.push_back(&p);
        std::vector<uint64_t> candidates{0};for(const auto* c:conflicts)candidates.push_back(align(c->end,item.alignment));
        std::sort(candidates.begin(),candidates.end());
        uint64_t offset=UINT64_MAX;
        for(const auto candidate:candidates){
            if(std::none_of(conflicts.begin(),conflicts.end(),[&](const Placed* c){return candidate<c->end && c->offset<candidate+item.size;})){offset=candidate;break;}}
        if(offset==UINT64_MAX){result.status={GraphError::Placement};result.placements.clear();return result;}
        placed.push_back({offset,offset+item.size,item.heap,item.first,item.last});
        result.placements.push_back({item.key,static_cast<FrameHeapClass>(item.heap),offset});
        result.heap_size[item.heap]=std::max(result.heap_size[item.heap],offset+item.size);
        result.separate_size[item.heap]+=align(item.size,item.alignment);
    }
    return result;
}
}
