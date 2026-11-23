#pragma once
#include "shader_constants.h"
#include "../world/cb_reads.h"

namespace ds3content {
// The interface and conservative reads must describe the same owned DXBC.
// Static reads outside its declaration are invalid, rather than clipped by
// FloatMask. Relative reads require the entire declared buffer. Supplying it
// does not establish the runtime index bounds, pose or draw readiness.
// Unavailable analysis retains the original strict RDEF coverage rule.
// Workstream K P1: everything before the pack depends only on the interface, the
// reads, the slot and the name; it is the plan, computed once per programme.
inline ShaderRegisterPlan PlanReadShaderRegister(const ShaderInterface& shader,
    const ds3reads::Reads& reads,uint32_t slot,std::string_view expected_name={}) {
    if(!reads.ok)return PlanShaderRegister(shader,slot,expected_name);
    ShaderRegisterPlan failed;failed.outcome.required=true;failed.outcome.status=ShaderConstantStatus::Invalid;
    const auto fixed=reads.fixed.find(slot),dynamic=reads.dynamic.find(slot);
    const bool referenced=(fixed!=reads.fixed.end() &&
        std::any_of(fixed->second.begin(),fixed->second.end(),[](uint8_t mask){return mask!=0;})) ||
        dynamic!=reads.dynamic.end();
    const ShaderResourceDeclaration* resource=nullptr;
    for(const auto& input:shader.resources)if(input.input_type==0 && input.shader_register==slot){
        if(resource || input.count!=1){failed.outcome.status=ShaderConstantStatus::Unsupported;return failed;}
        resource=&input;
    }
    if(!resource){if(referenced)return failed;return PlanShaderRegister(shader,slot,expected_name);}
    if(!expected_name.empty() && resource->name!=expected_name){failed.outcome.status=ShaderConstantStatus::Unsupported;return failed;}
    const ShaderConstantDeclaration* declaration=nullptr;
    for(const auto& input:shader.constants)if(input.name==resource->name){
        if(declaration)return failed;declaration=&input;
    }
    if(!declaration)return failed;
    const auto bytes=declaration->declared_bytes;
    if(!bytes || bytes>65536 || bytes%16 || declaration->buffer_type){
        failed.outcome.status=ShaderConstantStatus::Unsupported;return failed;}
    if(fixed!=reads.fixed.end())for(size_t row=0;row<fixed->second.size();++row){
        const auto mask=fixed->second[row];if(mask>15)return failed;
        for(size_t component=0;component<4;++component)
            if((mask&(1u<<component)) && row*4+component>=bytes/4)return failed;
    }
    if(dynamic!=reads.dynamic.end() && (dynamic->second.size()!=1 || dynamic->second[0]!=15 ||
        reads.stride.contains(slot)))return failed;
    auto plan=PlanShaderRegister(shader,slot,expected_name);
    if(plan.declaration>=0)plan.read_words=ds3reads::FloatMask(reads,slot,bytes,static_cast<bool*>(nullptr));
    return plan;
}
inline ShaderRegisterBlock PrepareReadShaderRegister(const ShaderInterface& shader,
    const ds3reads::Reads& reads,uint32_t slot,std::span<const ShaderConstantRange> ranges,
    std::string_view expected_name={}) {
    return PackShaderRegisterPlan(shader,PlanReadShaderRegister(shader,reads,slot,expected_name),ranges);
}
}
