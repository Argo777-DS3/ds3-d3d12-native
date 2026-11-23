#pragma once
#include "shader_interface.h"
#include <span>
#include <algorithm>
#include <string_view>

namespace ds3content {
struct ShaderConstantRange {
    uint32_t byte_offset{};
    std::span<const uint8_t> bytes;
};
enum class ShaderConstantStatus : uint8_t { Ready,Missing,Unsupported,Invalid };
struct ShaderRegisterBlock {
    ShaderConstantStatus status{ShaderConstantStatus::Missing};
    bool required{};
    std::vector<uint8_t> bytes;
};
// Extracted from material admission's actual RDEF USED-variable rule. Current
// producers supply ranges directly; the synchronous call retains no spans.
// Without an explicit same-program read set, every USED variable, including
// full array/struct extent, requires coverage. An explicit set requires every
// read word instead; zeroed unproduced words are then proved unread padding.
// Zeroes only unused bytes. Other family-wide producer ranges may lie outside
// this programme's declared extent; overlapping ownership is invalid.
// Refusal leaves owned output unchanged; this is one block, not draw readiness.
inline ShaderConstantStatus PackShaderConstant(const ShaderConstantDeclaration& declaration,
    std::span<const ShaderConstantRange> ranges,std::vector<uint8_t>& output,
    std::span<const uint8_t> read_words={}) {
    if(declaration.buffer_type || !declaration.declared_bytes || declaration.declared_bytes>65536 ||
       declaration.declared_bytes%16)return ShaderConstantStatus::Unsupported;
    if(ranges.size()>4096)return ShaderConstantStatus::Invalid;
    if(!read_words.empty() && (read_words.size()!=declaration.declared_bytes/4 ||
       std::any_of(read_words.begin(),read_words.end(),[](uint8_t word){return word>1;})))
        return ShaderConstantStatus::Invalid;
    // I-0091 step 3 (i90-07: two heap allocations a packed block on the collect): the range and written scratch kept by each thread.
    thread_local std::vector<std::pair<uint32_t,uint32_t>> ownership;ownership.clear();
    for(const auto& range:ranges){
        if(range.bytes.empty() || range.byte_offset%4 || range.bytes.size()%4 || range.byte_offset>65536 ||
           range.bytes.size()>65536-range.byte_offset)return ShaderConstantStatus::Invalid;
        ownership.emplace_back(range.byte_offset,range.byte_offset+static_cast<uint32_t>(range.bytes.size()));
    }
    std::sort(ownership.begin(),ownership.end());
    for(size_t i=1;i<ownership.size();++i)if(ownership[i].first<ownership[i-1].second)return ShaderConstantStatus::Invalid;
    std::vector<uint8_t> candidate(declaration.declared_bytes);thread_local std::vector<uint8_t> written;written.assign(declaration.declared_bytes,0);
    for(const auto& range:ranges){
        if(range.byte_offset<candidate.size()){
            const auto count=std::min(range.bytes.size(),candidate.size()-range.byte_offset);
            std::copy_n(range.bytes.begin(),count,candidate.begin()+range.byte_offset);
            std::fill_n(written.begin()+range.byte_offset,count,uint8_t(1));
        }
    }
    for(const auto& variable:declaration.variables){
        if(variable.flags&~3u)return ShaderConstantStatus::Unsupported;
        if(!variable.bytes || variable.byte_offset>candidate.size() ||
           variable.bytes>candidate.size()-variable.byte_offset)return ShaderConstantStatus::Invalid;
        if(read_words.empty() && (variable.flags&2u)){const auto begin=written.begin()+variable.byte_offset;
            if(std::any_of(begin,begin+variable.bytes,[](uint8_t byte){return byte==0;}))return ShaderConstantStatus::Missing;}
    }
    for(size_t word=0;word<read_words.size();++word)if(read_words[word])
        for(size_t byte=0;byte<4;++byte)if(!written[word*4+byte])return ShaderConstantStatus::Missing;
    output=std::move(candidate);return ShaderConstantStatus::Ready;
}
// Workstream K P1: a register's preparation splits into a plan, which depends only
// on the interface, the slot, the expected name and the read words, and a pack of
// the current ranges. The plan is either a final outcome (no pack) or the index of
// the declaration to pack; it holds no pointer into the interface, so it stays
// valid wherever the interface is copied. Prepare = Pack(Plan), by construction.
struct ShaderRegisterPlan {
    ShaderRegisterBlock outcome;          // the result when no declaration is packed
    int32_t declaration{-1};              // index in ShaderInterface::constants to pack
    std::vector<uint8_t> read_words;      // empty: the strict RDEF USED rule
};
// Resolve the actual RDEF register/name pair. An absent block requires no
// binding; an ambiguous declaration cannot acquire readiness from another one.
inline ShaderRegisterPlan PlanShaderRegister(const ShaderInterface& shader,uint32_t slot,
    std::string_view expected_name={}) {
    ShaderRegisterPlan plan;auto& output=plan.outcome;
    const ShaderResourceDeclaration* resource=nullptr;
    for(const auto& input:shader.resources)if(input.input_type==0 && input.shader_register==slot){
        output.required=true;
        if(resource || input.count!=1){output.status=ShaderConstantStatus::Unsupported;return plan;}
        resource=&input;
    }
    if(!resource){output.status=ShaderConstantStatus::Ready;return plan;}
    if(!expected_name.empty() && resource->name!=expected_name){output.status=ShaderConstantStatus::Unsupported;return plan;}
    int32_t declaration=-1;
    for(size_t index=0;index<shader.constants.size();++index)if(shader.constants[index].name==resource->name){
        if(declaration>=0){output.status=ShaderConstantStatus::Invalid;return plan;}
        declaration=static_cast<int32_t>(index);
    }
    if(declaration<0){output.status=ShaderConstantStatus::Invalid;return plan;}
    plan.declaration=declaration;return plan;
}
inline ShaderRegisterBlock PackShaderRegisterPlan(const ShaderInterface& shader,const ShaderRegisterPlan& plan,
    std::span<const ShaderConstantRange> ranges) {
    if(plan.declaration<0)return plan.outcome;
    ShaderRegisterBlock output;output.required=true;
    output.status=PackShaderConstant(shader.constants.at(static_cast<size_t>(plan.declaration)),ranges,output.bytes,plan.read_words);
    return output;
}
inline ShaderRegisterBlock PrepareShaderRegister(const ShaderInterface& shader,uint32_t slot,
    std::span<const ShaderConstantRange> ranges,std::string_view expected_name={},
    std::span<const uint8_t> read_words={}) {
    auto plan=PlanShaderRegister(shader,slot,expected_name);
    if(plan.declaration>=0)plan.read_words.assign(read_words.begin(),read_words.end());
    return PackShaderRegisterPlan(shader,plan,ranges);
}
}
