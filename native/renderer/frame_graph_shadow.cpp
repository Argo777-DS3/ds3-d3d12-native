#include "frame_graph_shadow.h"
#include <algorithm>
#include <cstdio>

namespace ds3native {
namespace {
ResourceKey KeyOf(ID3D12Resource* resource) noexcept {return {reinterpret_cast<uintptr_t>(resource),1};}
}
void GraphShadow::Begin(uint64_t serial,ID3D12GraphicsCommandList* list) {
    active_=true;serial_=serial;descs_.clear();names_.clear();tracked_.clear();plan_={};plan_.serial=serial?serial:1;
    pass_names_.clear();recorded_.clear();mismatches_.clear();list_=list;executing_=nullptr;executed_passes_=0;key_.clear();verify_=true;
}
void GraphShadow::Execute(uint64_t structure) {
    if(!active_)return;
    key_={structure};
    for(size_t i=0;i<descs_.size();++i){const auto& d=descs_[i];uint64_t name=1469598103934665603ULL;for(const char ch:names_[i]){name^=uint8_t(ch);name*=1099511628211ULL;}
        key_.push_back(name);key_.push_back(d.subresources);key_.push_back(static_cast<uint64_t>(d.home));key_.push_back(static_cast<uint64_t>(d.lifetime));
        key_.push_back(d.start_declared?static_cast<uint64_t>(d.start)+1:0);}
    if(!executor_ || !list_)return;
    for(auto& cached:cache_)if(cached.key==key_){executing_=&cached;cached.last_used=frames_+1;
        // Axis 3: a plan verified on its first executed frames is then checked on a sample of them.
        verify_=cached.verified<verified_before_sampling || (++cached.executions)%sampling_period==0;break;}
}
// Records one or two batches in one ResourceBarrier call (the after-batch of a pass holds transients at their last use, the
// before-batch of the next pass other resources, so they never touch the same subresource) and moves the tracked states.
// The cached plan's resources are its frame's slots; this frame's resource of the same slot is used.
void GraphShadow::Apply(const BarrierBatch& first,const BarrierBatch* second) {
    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    for(const auto* batch:{&first,second}){if(!batch)continue;
        if(!batch->uav.empty())throw std::logic_error("native frame graph executor: UAV barriers are not executed yet");
        for(const auto& t:batch->transitions){
            size_t slot=0;while(slot<executing_->slots.size() && executing_->slots[slot]!=t.key.identity)++slot;
            if(slot>=descs_.size())throw std::logic_error("native frame graph executor: a plan resource has no slot");
            auto* resource=reinterpret_cast<ID3D12Resource*>(descs_[slot].key.identity);
            D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            b.Transition={resource,t.subresource==NoPlanIndex?D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES:t.subresource,t.before,t.after};barriers.push_back(b);
            const auto found=tracked_.find(resource);
            if(found!=tracked_.end())for(uint32_t s=0;s<found->second.state.size();++s)if(t.subresource==NoPlanIndex || s==t.subresource)found->second.state[s]=t.after;}}
    if(barriers.empty())return;
    list_->ResourceBarrier(static_cast<UINT>(barriers.size()),barriers.data());graph_barriers_+=barriers.size();++barrier_calls_;
}
void GraphShadow::Mismatch(std::string row) {if(mismatches_.size()<16)mismatches_.push_back(std::move(row));}
void GraphShadow::Declare(ID3D12Resource* resource,uint32_t subresources,D3D12_RESOURCE_STATES home,FrameLifetime lifetime,const char* name,bool output) {
    if(!active_ || !resource || tracked_.contains(resource))return;
    const auto index=static_cast<uint32_t>(descs_.size());
    descs_.push_back({KeyOf(resource),lifetime,FrameHeapClass::Target,subresources,home,1,65536});names_.push_back(name);
    tracked_[resource]={index,std::vector<D3D12_RESOURCE_STATES>(subresources,home)};
    const auto kind=output || (home==D3D12_RESOURCE_STATE_PRESENT && lifetime==FrameLifetime::External)?ResourceKind::Presentation:
        subresources>1?ResourceKind::Depth:ResourceKind::Color;
    plan_.resources.push_back({KeyOf(resource),kind,lifetime!=FrameLifetime::Transient});
}
void GraphShadow::DeclareStarting(ID3D12Resource* resource,D3D12_RESOURCE_STATES start,D3D12_RESOURCE_STATES home,FrameLifetime lifetime,const char* name) {
    if(!active_ || !resource || tracked_.contains(resource))return;
    Declare(resource,1,home,lifetime,name);
    if(start!=home){auto& desc=descs_.back();desc.start_declared=true;desc.start=start;tracked_[resource].state.assign(1,start);}
}
void GraphShadow::Transition(ID3D12Resource* resource,uint32_t subresource,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after,uint32_t line) {
    if(!active_)return;
    const auto found=tracked_.find(resource);
    if(found==tracked_.end()){   // not a resource of the plan: counted by line (review)
        auto line_hit=std::find_if(untracked_lines_.begin(),untracked_lines_.end(),[&](const auto& e){return e.first==line;});
        if(line_hit!=untracked_lines_.end())++line_hit->second;else if(untracked_lines_.size()<64)untracked_lines_.push_back({line,1});
        return;}
    auto& states=found->second.state;
    for(uint32_t s=0;s<states.size();++s){if(subresource!=NoPlanIndex && s!=subresource)continue;
        if(states[s]!=before){char row[320]{};sprintf_s(row,"{\"event\":\"graph_barrier_mismatch\",\"serial\":%llu,\"kind\":\"hand_before\",\"pass\":\"%s\",\"resource\":\"%s\",\"subresource\":%u,\"expected\":%u,\"actual\":%u,\"line\":%u}\n",
            static_cast<unsigned long long>(serial_),pass_names_.empty()?"(start)":pass_names_.back().c_str(),names_[found->second.index].c_str(),s,
            static_cast<unsigned>(states[s]),static_cast<unsigned>(before),line);Mismatch(row);}
        states[s]=after;}
}
void GraphShadow::Pass(const char* name,PassKind kind) {
    if(!active_)return;
    const auto identity=static_cast<uint32_t>(plan_.passes.size()+1);
    if(executing_){   // E3: the previous pass's after-batch, then this pass's before-batch
        if(executed_passes_>=executing_->pass_names.size() || executing_->pass_names[executed_passes_]!=name)
            throw std::logic_error("native frame graph executor: the frame's passes differ from its plan");
        if(executed_passes_)Apply(executing_->barriers.after_pass[executed_passes_-1],&executing_->barriers.before_pass[executed_passes_]);
        else Apply(executing_->barriers.before_pass[executed_passes_]);
        ++executed_passes_;}
    plan_.passes.push_back({identity,kind,identity>1?std::vector<uint32_t>{identity-1}:std::vector<uint32_t>{},{}});pass_names_.push_back(name);
    auto hit=std::find_if(pass_hits_.begin(),pass_hits_.end(),[&](const auto& e){return e.first==name;});
    if(hit!=pass_hits_.end())++hit->second;else if(pass_hits_.size()<64)pass_hits_.push_back({name,1});
}
void GraphShadow::Use(ID3D12Resource* resource,ResourceAccess access,D3D12_RESOURCE_STATES required,SubresourceRange range) {
    if(!active_ || plan_.passes.empty())return;
    const auto found=tracked_.find(resource);
    if(found==tracked_.end()){char row[256]{};sprintf_s(row,"{\"event\":\"graph_barrier_mismatch\",\"serial\":%llu,\"kind\":\"undeclared\",\"pass\":\"%s\"}\n",
        static_cast<unsigned long long>(serial_),pass_names_.back().c_str());Mismatch(row);return;}
    if(!verify_)return;   // axis 3: a sampled-out executed frame records no use (nothing is compiled or compared at its end)
    auto& pass=plan_.passes.back();const auto pass_index=static_cast<uint32_t>(plan_.passes.size()-1),use_index=static_cast<uint32_t>(pass.resources.size());
    pass.resources.push_back({KeyOf(resource),access,range,static_cast<uint32_t>(required),true});++uses_;
    const auto& states=found->second.state;const auto count=static_cast<uint32_t>(states.size());
    const uint32_t end=range.count==NoPlanIndex?count:static_cast<uint32_t>(std::min<uint64_t>(count,uint64_t(range.first)+range.count));
    for(uint32_t s=range.first;s<end;++s)recorded_.push_back({pass_index,use_index,s,states[s]});
}
std::string GraphShadow::Summary() const {
    std::string row="{\"event\":\"graph_shadow_summary\",\"frames\":"+std::to_string(frames_)+",\"uses\":"+std::to_string(uses_)+",\"passes\":{";
    for(size_t i=0;i<pass_hits_.size();++i)row+=(i?",\"":"\"")+pass_hits_[i].first+"\":"+std::to_string(pass_hits_[i].second);
    row+="},\"untracked_transitions_by_line\":{";
    for(size_t i=0;i<untracked_lines_.size();++i)row+=(i?",\"":"\"")+std::to_string(untracked_lines_[i].first)+"\":"+std::to_string(untracked_lines_[i].second);
    return row+"}}\n";
}
std::vector<std::string> GraphShadow::End() {
    if(!active_)return {};
    if(executing_){   // E3: the last pass's after-batch and the end batch, before the list closes
        if(executed_passes_!=executing_->pass_names.size())throw std::logic_error("native frame graph executor: the frame ended before its plan");
        Apply(executing_->barriers.after_pass[executed_passes_-1],&executing_->barriers.end);++executed_frames_;}
    const bool executed=executing_!=nullptr;CachedPlan* const plan=executing_;executing_=nullptr;
    active_=false;++frames_;
    char row[384]{};
    for(const auto& [resource,tracked]:tracked_)for(uint32_t s=0;s<tracked.state.size();++s)if(tracked.state[s]!=descs_[tracked.index].home){
        sprintf_s(row,"{\"event\":\"graph_barrier_mismatch\",\"serial\":%llu,\"kind\":\"end_state\",\"resource\":\"%s\",\"subresource\":%u,\"expected\":%u,\"actual\":%u}\n",
            static_cast<unsigned long long>(serial_),names_[tracked.index].c_str(),s,static_cast<unsigned>(descs_[tracked.index].home),static_cast<unsigned>(tracked.state[s]));Mismatch(row);}
    // Axis 3: a sampled-out executed frame ends here (its end states checked above); an end-state mismatch still evicts its plan.
    if(executed && !verify_){++sampled_out_frames_;
        if(!mismatches_.empty()){if(std::erase_if(cache_,[&](const CachedPlan& c){return c.key==key_;}))++evicted_;
            for(auto& m:mismatches_)if(m.size()>2)m.insert(m.size()-2,",\"executed\":true");}
        auto result=std::move(mismatches_);mismatches_.clear();return result;}
    ++compared_frames_;
    const auto compiled=CompileFramePlan(plan_);
    if(!compiled){const auto status=compiled.Status();
        sprintf_s(row,"{\"event\":\"graph_barrier_mismatch\",\"serial\":%llu,\"kind\":\"plan\",\"error\":%u,\"pass\":\"%s\",\"resource\":\"%s\"}\n",static_cast<unsigned long long>(serial_),
            static_cast<unsigned>(status.error),status.pass<pass_names_.size()?pass_names_[status.pass].c_str():"",status.resource<names_.size()?names_[status.resource].c_str():"");
        Mismatch(row);auto result=std::move(mismatches_);mismatches_.clear();return result;}
    const auto barriers=PlanFrameBarriers(compiled,descs_);
    if(!barriers.status){const auto status=barriers.status;
        sprintf_s(row,"{\"event\":\"graph_barrier_mismatch\",\"serial\":%llu,\"kind\":\"graph\",\"error\":%u,\"pass\":\"%s\",\"resource\":\"%s\"}\n",static_cast<unsigned long long>(serial_),
            static_cast<unsigned>(status.error),status.pass<pass_names_.size()?pass_names_[status.pass].c_str():"",status.resource<names_.size()?names_[status.resource].c_str():"");
        Mismatch(row);auto result=std::move(mismatches_);mismatches_.clear();return result;}
    // Criterion (sheet 5.3): at every use and subresource, the graph's state equals the hand-written state.
    size_t g=0;
    for(const auto& hand:recorded_){
        while(g<barriers.use_states.size() && (barriers.use_states[g].pass<hand.pass || (barriers.use_states[g].pass==hand.pass &&
              (barriers.use_states[g].use<hand.use || (barriers.use_states[g].use==hand.use && barriers.use_states[g].subresource<hand.subresource)))))++g;
        const bool found=g<barriers.use_states.size() && barriers.use_states[g].pass==hand.pass && barriers.use_states[g].use==hand.use && barriers.use_states[g].subresource==hand.subresource;
        const auto graph=found?barriers.use_states[g].state:static_cast<D3D12_RESOURCE_STATES>(0xFFFFFFFFu);
        if(graph!=hand.hand){const auto& use=plan_.passes[hand.pass].resources[hand.use];size_t r=0;while(r<descs_.size() && !(descs_[r].key==use.key))++r;
            sprintf_s(row,"{\"event\":\"graph_barrier_mismatch\",\"serial\":%llu,\"kind\":\"use_state\",\"pass\":\"%s\",\"resource\":\"%s\",\"subresource\":%u,\"expected\":%u,\"actual\":%u}\n",
                static_cast<unsigned long long>(serial_),pass_names_[hand.pass].c_str(),r<names_.size()?names_[r].c_str():"?",hand.subresource,
                static_cast<unsigned>(graph),static_cast<unsigned>(hand.hand));Mismatch(row);}
    }
    // E3: a hand-written frame equal to its plan makes the plan executable for its key (32 keys at most, the least recently
    // used replaced); an executed frame that mismatches drops its plan (review), and its rows say it was executed.
    if(executor_ && executed && !mismatches_.empty()){
        if(std::erase_if(cache_,[&](const CachedPlan& c){return c.key==key_;}))++evicted_;
        for(auto& m:mismatches_)if(m.size()>2)m.insert(m.size()-2,",\"executed\":true");}
    else if(executed && plan)++plan->verified;   // axis 3: one more executed frame fully compared without a mismatch
    if(executor_ && !executed && mismatches_.empty() && !key_.empty() &&
       std::none_of(cache_.begin(),cache_.end(),[&](const CachedPlan& c){return c.key==key_;})){
        if(cache_.size()>=32)cache_.erase(std::min_element(cache_.begin(),cache_.end(),[](const CachedPlan& a,const CachedPlan& b){return a.last_used<b.last_used;}));
        std::vector<uint64_t> slots;for(const auto& d:descs_)slots.push_back(d.key.identity);
        cache_.push_back({key_,pass_names_,barriers,std::move(slots),frames_});}
    auto result=std::move(mismatches_);mismatches_.clear();return result;
}
}
