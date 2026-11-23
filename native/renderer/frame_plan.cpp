#include "frame_plan.h"
#include <algorithm>
#include <atomic>

namespace ds3native {
namespace {
std::atomic<uint64_t> plan_passes_peak{};
// E1 review: whether the ranges written by earlier passes cover `range` (a range with count max reaches the last
// subresource, so only a write that also reaches it can cover its end).
bool Covered(const std::vector<SubresourceRange>& written,const SubresourceRange& range){
    constexpr auto all=std::numeric_limits<uint32_t>::max();
    const uint64_t end=range.count==all?UINT64_MAX:uint64_t(range.first)+range.count;uint64_t at=range.first;
    while(at<end){bool advanced=false;
        for(const auto& w:written){const uint64_t w_end=w.count==all?UINT64_MAX:uint64_t(w.first)+w.count;
            if(w.first<=at && w_end>at){at=w_end;advanced=true;break;}}
        if(!advanced)return false;}
    return true;
}
}
uint64_t TakePlanPassesPeak() noexcept {return plan_passes_peak.exchange(0);}
CompiledFramePlan CompileFramePlan(const FramePlan& input) {
    CompiledFramePlan result;
    auto fail=[&](PlanError error,uint32_t pass=NoPlanIndex,uint32_t resource=NoPlanIndex) {
        result.status_={error,pass,resource};return result;
    };
    if(!input.serial || input.resources.empty() || input.resources.size()>16384 ||
       input.passes.empty() || input.passes.size()>MaxPlanPasses)return fail(PlanError::Extent);
    const auto resource_count=input.resources.size(),pass_count=input.passes.size();
    for(size_t r=0;r<resource_count;++r) {
        const auto& resource=input.resources[r];
        if(!resource.key.identity || !resource.key.generation)return fail(PlanError::ResourceIdentity,NoPlanIndex,static_cast<uint32_t>(r));
        if(resource.kind>ResourceKind::Presentation)return fail(PlanError::Kind,NoPlanIndex,static_cast<uint32_t>(r));
        for(size_t before=0;before<r;++before)
            if(input.resources[before].key.identity==resource.key.identity)
                return fail(PlanError::ResourceIdentity,NoPlanIndex,static_cast<uint32_t>(r));
    }
    std::vector<std::vector<bool>> depends(pass_count,std::vector<bool>(pass_count));
    std::vector<uint32_t> writer(resource_count,NoPlanIndex),first(resource_count,NoPlanIndex),last(resource_count);
    std::vector<std::vector<uint32_t>> readers(resource_count);
    std::vector<std::vector<SubresourceRange>> written(resource_count);   // E1 review: the ranges earlier passes wrote
    uint32_t presentations{};
    for(size_t p=0;p<pass_count;++p) {
        const auto& pass=input.passes[p];const auto ordinal=static_cast<uint32_t>(p);
        if(!pass.identity)return fail(PlanError::PassIdentity,ordinal);
        for(size_t before=0;before<p;++before)
            if(input.passes[before].identity==pass.identity)return fail(PlanError::PassIdentity,ordinal);
        if(pass.kind>PassKind::Present)return fail(PlanError::Kind,ordinal);
        if(pass.resources.empty() || pass.resources.size()>16384 || pass.after.size()>MaxPassDependencies)return fail(PlanError::Extent,ordinal);
        std::vector<uint32_t> dependencies;
        for(const auto identity:pass.after) {
            size_t parent{};while(parent<p && input.passes[parent].identity!=identity)++parent;
            if(parent==p || std::find(dependencies.begin(),dependencies.end(),identity)!=dependencies.end())
                return fail(PlanError::Dependency,ordinal);
            dependencies.push_back(identity);depends[p][parent]=true;
            for(size_t ancestor=0;ancestor<parent;++ancestor)
                depends[p][ancestor]=depends[p][ancestor] || depends[parent][ancestor];
        }
        const bool present=pass.kind==PassKind::Present;
        if(present) {
            ++presentations;
            if(p+1!=pass_count || pass.resources.size()!=1)return fail(PlanError::Presentation,ordinal);
            for(size_t before=0;before<p;++before)
                if(!depends[p][before])return fail(PlanError::Presentation,ordinal);
        }
        std::vector<std::pair<size_t,SubresourceRange>> used,pass_writes;
        for(const auto& use:pass.resources) {
            size_t r{};while(r<resource_count && input.resources[r].key!=use.key)++r;
            if(r==resource_count)return fail(PlanError::ResourceVersion,ordinal);
            const auto resource_ordinal=static_cast<uint32_t>(r);
            // E1: a second use of the resource in the pass only in a disjoint subresource range.
            if(!use.range.count)return fail(PlanError::Extent,ordinal,resource_ordinal);
            if(std::any_of(used.begin(),used.end(),[&](const auto& u){return u.first==r && u.second.Overlaps(use.range);}))
                return fail(PlanError::DuplicateUse,ordinal,resource_ordinal);
            used.push_back({r,use.range});
            if(use.access>ResourceAccess::ReadWrite)return fail(PlanError::Kind,ordinal,resource_ordinal);
            if(present && (input.resources[r].kind!=ResourceKind::Presentation || use.access!=ResourceAccess::Read || writer[r]==NoPlanIndex))
                return fail(PlanError::Presentation,ordinal,resource_ordinal);
            const bool reads=use.access!=ResourceAccess::Write,writes=use.access!=ResourceAccess::Read;
            // E1 review: the read range must have been written by earlier passes (in whole), unless the owner attests it.
            if(reads && !input.resources[r].initialized && !Covered(written[r],use.range))
                return fail(PlanError::UninitializedRead,ordinal,resource_ordinal);
            if(writes)pass_writes.push_back({r,use.range});
            if(writer[r]!=NoPlanIndex && writer[r]!=ordinal && !depends[p][writer[r]])   // E1: another range of this pass is no dependency
                return fail(PlanError::MissingDependency,ordinal,resource_ordinal);
            if(writes) {
                for(const auto reader:readers[r])
                    if(reader!=ordinal && !depends[p][reader])return fail(PlanError::MissingDependency,ordinal,resource_ordinal);
                readers[r].clear();writer[r]=ordinal;
            } else readers[r].push_back(ordinal);
            if(first[r]==NoPlanIndex)first[r]=ordinal;
            last[r]=ordinal;
        }
        for(const auto& [r,range]:pass_writes)written[r].push_back(range);   // visible to later passes only
    }
    if(presentations!=1)return fail(PlanError::Presentation);
    result.plan_=input;
    result.status_={};
    {auto known=plan_passes_peak.load();while(pass_count>known && !plan_passes_peak.compare_exchange_weak(known,pass_count)){}}   // E1: plan_passes_peak
    for(size_t r=0;r<resource_count;++r)
        if(first[r]!=NoPlanIndex)result.lifetimes_.push_back({input.resources[r].key,first[r],last[r]});
    return result;
}
} // namespace ds3native
