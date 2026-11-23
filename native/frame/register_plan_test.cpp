// Workstream K P1 control: PrepareReadShaderRegister = PackShaderRegisterPlan(PlanReadShaderRegister) must equal the
// pre-K implementation (copied below verbatim as the reference) for every outcome: absent register, ambiguous resource,
// unexpected name, missing or ambiguous declaration, unsupported extent, reads outside the declaration, relative reads,
// unavailable analysis, and packs of empty, complete, partial and overlapping ranges.
#include "../content/shader_reads.h"
#include <cstdio>
#include <random>

namespace reference {
using namespace ds3content;
inline ShaderRegisterBlock LegacyPrepareShaderRegister(const ShaderInterface& shader,uint32_t slot,
    std::span<const ShaderConstantRange> ranges,std::string_view expected_name={},
    std::span<const uint8_t> read_words={}) {
    ShaderRegisterBlock output;
    const ShaderResourceDeclaration* resource=nullptr;
    for(const auto& input:shader.resources)if(input.input_type==0 && input.shader_register==slot){
        output.required=true;
        if(resource || input.count!=1){output.status=ShaderConstantStatus::Unsupported;return output;}
        resource=&input;
    }
    if(!resource){output.status=ShaderConstantStatus::Ready;return output;}
    if(!expected_name.empty() && resource->name!=expected_name){output.status=ShaderConstantStatus::Unsupported;return output;}
    const ShaderConstantDeclaration* declaration=nullptr;
    for(const auto& input:shader.constants)if(input.name==resource->name){
        if(declaration){output.status=ShaderConstantStatus::Invalid;return output;}
        declaration=&input;
    }
    if(!declaration){output.status=ShaderConstantStatus::Invalid;return output;}
    output.status=PackShaderConstant(*declaration,ranges,output.bytes,read_words);return output;
}
inline ShaderRegisterBlock LegacyPrepareReadShaderRegister(const ShaderInterface& shader,
    const ds3reads::Reads& reads,uint32_t slot,std::span<const ShaderConstantRange> ranges,
    std::string_view expected_name={}) {
    if(!reads.ok)return LegacyPrepareShaderRegister(shader,slot,ranges,expected_name);
    ShaderRegisterBlock failed;failed.required=true;failed.status=ShaderConstantStatus::Invalid;
    const auto fixed=reads.fixed.find(slot),dynamic=reads.dynamic.find(slot);
    const bool referenced=(fixed!=reads.fixed.end() &&
        std::any_of(fixed->second.begin(),fixed->second.end(),[](uint8_t mask){return mask!=0;})) ||
        dynamic!=reads.dynamic.end();
    const ShaderResourceDeclaration* resource=nullptr;
    for(const auto& input:shader.resources)if(input.input_type==0 && input.shader_register==slot){
        if(resource || input.count!=1){failed.status=ShaderConstantStatus::Unsupported;return failed;}
        resource=&input;
    }
    if(!resource){if(referenced)return failed;return LegacyPrepareShaderRegister(shader,slot,ranges,expected_name);}
    if(!expected_name.empty() && resource->name!=expected_name){failed.status=ShaderConstantStatus::Unsupported;return failed;}
    const ShaderConstantDeclaration* declaration=nullptr;
    for(const auto& input:shader.constants)if(input.name==resource->name){
        if(declaration)return failed;declaration=&input;
    }
    if(!declaration)return failed;
    const auto bytes=declaration->declared_bytes;
    if(!bytes || bytes>65536 || bytes%16 || declaration->buffer_type){
        failed.status=ShaderConstantStatus::Unsupported;return failed;}
    if(fixed!=reads.fixed.end())for(size_t row=0;row<fixed->second.size();++row){
        const auto mask=fixed->second[row];if(mask>15)return failed;
        for(size_t component=0;component<4;++component)
            if((mask&(1u<<component)) && row*4+component>=bytes/4)return failed;
    }
    if(dynamic!=reads.dynamic.end() && (dynamic->second.size()!=1 || dynamic->second[0]!=15 ||
        reads.stride.contains(slot)))return failed;
    const auto words=ds3reads::FloatMask(reads,slot,bytes,static_cast<bool*>(nullptr));
    return LegacyPrepareShaderRegister(shader,slot,ranges,expected_name,words);
}
}

using namespace ds3content;
namespace {
int failures{},cases{};
bool Same(const ShaderRegisterBlock& a,const ShaderRegisterBlock& b) {return a.status==b.status && a.required==b.required && a.bytes==b.bytes;}
ShaderConstantDeclaration Buffer(std::string name,uint32_t bytes,std::vector<std::pair<uint32_t,uint32_t>> variables,uint32_t flags=2) {
    ShaderConstantDeclaration declaration;declaration.name=std::move(name);declaration.declared_bytes=bytes;
    for(size_t i=0;i<variables.size();++i){ShaderVariableDeclaration variable;variable.name="v"+std::to_string(i);
        variable.byte_offset=variables[i].first;variable.bytes=variables[i].second;variable.flags=flags;declaration.variables.push_back(variable);}
    return declaration;
}
ShaderResourceDeclaration Resource(std::string name,uint32_t slot,uint32_t count=1) {
    ShaderResourceDeclaration resource;resource.name=std::move(name);resource.input_type=0;resource.shader_register=slot;resource.count=count;return resource;
}
void Check(const char* label,const ShaderInterface& shader,const ds3reads::Reads& reads,uint32_t slot,
    std::span<const ShaderConstantRange> ranges,std::string_view name) {
    ++cases;
    const auto expected=reference::LegacyPrepareReadShaderRegister(shader,reads,slot,ranges,name);
    const auto current=PrepareReadShaderRegister(shader,reads,slot,ranges,name);
    const auto plan=PlanReadShaderRegister(shader,reads,slot,name);
    const auto packed=PackShaderRegisterPlan(shader,plan,ranges);
    // The plan holds no pointer into the interface: packing it against a copy of the interface gives the same block.
    const auto copy=shader;   // the plan holds an index, so it stays valid on a copy of the interface
    const auto copied=PackShaderRegisterPlan(copy,plan,ranges);
    const auto strict_expected=reference::LegacyPrepareShaderRegister(shader,slot,ranges,name);
    const auto strict_current=PrepareShaderRegister(shader,slot,ranges,name);
    if(!Same(expected,current) || !Same(expected,packed) || !Same(expected,copied) || !Same(strict_expected,strict_current)){
        ++failures;std::printf("FAIL %s slot %u: expected status %u required %d bytes %zu, got %u %d %zu\n",label,slot,
            static_cast<unsigned>(expected.status),expected.required?1:0,expected.bytes.size(),
            static_cast<unsigned>(current.status),current.required?1:0,current.bytes.size());}
}
}
int main() {
    std::mt19937 random(1289);
    std::vector<uint8_t> payload(4096);for(auto& byte:payload)byte=static_cast<uint8_t>(random());
    const auto range=[&](uint32_t offset,uint32_t bytes){return ShaderConstantRange{offset,std::span<const uint8_t>(payload.data()+offset,bytes)};};
    std::vector<std::vector<ShaderConstantRange>> range_sets{
        {},
        {range(0,64)},
        {range(0,16),range(32,16)},
        {range(0,128)},
        {range(0,32),range(16,32)},           // overlapping ownership: Invalid
        {range(4,8)},                          // partial
        {range(0,256)},                        // beyond the declaration: clipped copy
        {range(2,8)},                          // misaligned: Invalid
    };
    // Interfaces: a model programme shape (cbMtdParam b3, cbSceneParam b8, cbMatrixWVP b0, cbInstanceData b4) and
    // degenerate ones.
    ShaderInterface model;
    model.resources={Resource("cbMatrixWVP",0),Resource("cbMtdParam",3),Resource("cbInstanceData",4),Resource("cbSceneParam",8)};
    model.constants={Buffer("cbMatrixWVP",64,{{0,64}}),Buffer("cbMtdParam",96,{{0,16},{16,12},{32,64}}),
        Buffer("cbInstanceData",544,{{0,272},{272,272}}),Buffer("cbSceneParam",128,{{0,16},{16,48},{64,64}},0)};
    ShaderInterface ambiguous_resource=model;ambiguous_resource.resources.push_back(Resource("cbMtdParam2",3));
    ShaderInterface array_resource=model;array_resource.resources[1].count=2;
    ShaderInterface ambiguous_declaration=model;ambiguous_declaration.constants.push_back(Buffer("cbMtdParam",32,{{0,32}}));
    ShaderInterface missing_declaration=model;missing_declaration.constants.erase(missing_declaration.constants.begin()+1);
    ShaderInterface odd_extent=model;odd_extent.constants[1].declared_bytes=40;
    ShaderInterface structured=model;structured.constants[1].buffer_type=1;
    ShaderInterface unknown_flags=model;unknown_flags.constants[1].variables[0].flags=4;
    const std::vector<std::pair<const char*,const ShaderInterface*>> interfaces{{"model",&model},{"ambiguous_resource",&ambiguous_resource},
        {"array_resource",&array_resource},{"ambiguous_declaration",&ambiguous_declaration},{"missing_declaration",&missing_declaration},
        {"odd_extent",&odd_extent},{"structured",&structured},{"unknown_flags",&unknown_flags}};
    // Reads: unavailable, none, fixed inside, fixed outside, relative (whole buffer), relative with a stride, bad mask.
    std::vector<std::pair<const char*,ds3reads::Reads>> reads_sets;
    {ds3reads::Reads r;reads_sets.push_back({"unavailable",r});}
    {ds3reads::Reads r;r.ok=true;reads_sets.push_back({"none",r});}
    {ds3reads::Reads r;r.ok=true;for(unsigned slot:{0u,3u,4u,8u,5u})r.fixed[slot]={0xF,0x3,0x0,0x8};reads_sets.push_back({"fixed",r});}
    {ds3reads::Reads r;r.ok=true;for(unsigned slot:{0u,3u,4u,8u})r.fixed[slot]=std::vector<uint8_t>(40,0x1);reads_sets.push_back({"outside",r});}
    {ds3reads::Reads r;r.ok=true;for(unsigned slot:{0u,3u,4u,8u})r.dynamic[slot]={0xF};reads_sets.push_back({"relative",r});}
    {ds3reads::Reads r;r.ok=true;for(unsigned slot:{0u,3u,4u,8u}){r.dynamic[slot]={0xF};r.stride[slot]=3;}reads_sets.push_back({"stride",r});}
    {ds3reads::Reads r;r.ok=true;for(unsigned slot:{0u,3u,4u,8u})r.fixed[slot]={0x1F};reads_sets.push_back({"bad_mask",r});}
    {ds3reads::Reads r;r.ok=true;r.fixed[5]={0x1};reads_sets.push_back({"unbound_referenced",r});}
    const std::vector<std::pair<uint32_t,const char*>> slots{{3,""},{3,"cbMtdParam"},{3,"cbOther"},{8,"cbSceneParam"},{0,"cbMatrixWVP"},
        {4,"cbInstanceData"},{5,""},{5,"cbObjMatrix"}};
    for(const auto& [interface_label,shader]:interfaces)for(const auto& [reads_label,reads]:reads_sets)
        for(const auto& [slot,name]:slots)for(const auto& ranges:range_sets){
            char label[160]{};std::snprintf(label,sizeof(label),"%s/%s/%s",interface_label,reads_label,name);
            Check(label,*shader,reads,slot,ranges,name);}
    std::printf("register plan cases %d failures %d\n",cases,failures);
    return failures?1:0;
}
