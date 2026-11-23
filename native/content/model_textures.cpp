#include "model_textures.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace ds3content {
namespace {
uint64_t Number(std::span<const uint8_t> bytes,size_t at,size_t count) {
    if(at>bytes.size() || count>bytes.size()-at)throw std::runtime_error("SPX texture field extent");
    uint64_t value{};for(size_t i=0;i<count;++i)value|=uint64_t(bytes[at+i])<<(8*i);return value;
}
std::wstring Name(std::span<const uint8_t> bytes,uint64_t at) {
    if(at&1 || at>bytes.size())throw std::runtime_error("SPX texture name extent");
    std::wstring name;
    for(size_t i=0;i<=2048;++i){
        const auto ch=static_cast<wchar_t>(Number(bytes,static_cast<size_t>(at),2));
        if(!ch){if(name.empty())throw std::runtime_error("SPX texture empty name");return name;}
        if(i==2048)throw std::runtime_error("SPX texture name bound");
        name.push_back(ch);at+=2;
    }
    throw std::runtime_error("SPX texture name bound");
}
bool ValidName(const std::wstring& value,bool empty=false) {
    return (empty || !value.empty()) && value.size()<=2048 && value.find(L'\0')==std::wstring::npos;
}
}
std::vector<ModelTextureDeclaration> ReadModelTextureDeclarations(std::span<const uint8_t> bytes) {
    if(bytes.size()<0x98 || Number(bytes,8,4)<=1)throw std::runtime_error("SPX texture version envelope");
    const auto count=static_cast<size_t>(Number(bytes,0xC,1));
    if(count>(bytes.size()-0x98)/0x30)throw std::runtime_error("SPX texture table extent");
    constexpr uint32_t kinds[]{0,1,3,2,5,6,12,11,0,0,13};
    std::vector<ModelTextureDeclaration> result;result.reserve(count);
    for(size_t i=0;i<count;++i){const auto at=0x98+i*0x30;
        const auto kind=Number(bytes,at+0xB,1);
        ModelTextureDeclaration row{Name(bytes,Number(bytes,at,8)),
            static_cast<uint32_t>(Number(bytes,at+9,1)),kind<std::size(kinds)?kinds[kind]:0,
            static_cast<uint8_t>(Number(bytes,at+8,1)),static_cast<int32_t>(Number(bytes,at+0xC,4))};
        if(row.slot>=128)throw std::runtime_error("SPX texture slot envelope");
        result.push_back(std::move(row)); // First exact name has source precedence.
    }
    return result;
}
bool PrepareModelTextureBindings(std::span<const ModelTextureInput> inputs,bool spx_owner,
    std::span<const ModelTextureDeclaration> declarations,std::span<const ModelTextureDeclaration> builtins,
    std::vector<ModelTextureBinding>& output) {
    if(inputs.size()>65536 || declarations.size()>255 || builtins.size()>128 ||
       (!spx_owner && !declarations.empty()))return false;
    for(const auto rows:{declarations,builtins})for(const auto& row:rows)
        if(!ValidName(row.name) || row.slot>=128)return false;
    std::vector<ModelTextureBinding> prepared;
    for(const auto& input:inputs){
        if(!ValidName(input.path,true) || !ValidName(input.type) ||
           !std::isfinite(input.scale[0]) || !std::isfinite(input.scale[1]))return false;
        const auto find=[&](const auto rows){return std::find_if(rows.begin(),rows.end(),
            [&](const auto& row){return row.name==input.type;});};
        auto selected=find(declarations);ModelTextureDeclaration declaration;
        if(selected!=declarations.end())declaration=*selected;
        else {selected=find(builtins);if(selected==builtins.end())continue;
            declaration=*selected;declaration.remap=-1;if(spx_owner)declaration.flags=0;}
        const auto duplicate=std::find_if(prepared.begin(),prepared.end(),
            [&](const auto& row){return row.declaration.slot==declaration.slot;});
        if(duplicate==prepared.end())prepared.push_back({std::move(declaration),input.scale,input.metadata,
            input.authored_ordinal,input.authored_ordinal,input.path});
        else {duplicate->path=input.path;duplicate->current_authored_ordinal=input.authored_ordinal;}
    }
    output=std::move(prepared);return true;
}
const ShaderResourceDeclaration* UnsupportedModelShaderTexture(const ShaderInterface& shader) {
    for(const auto& resource:shader.resources){
        if(resource.input_type==0 || resource.input_type==3)continue;
        // Structured buffers (5) and cubes (dimension 9) are engine/scene inputs
        // (forward lights, IBL), never material textures.
        const bool structured=resource.input_type==5;
        if(resource.input_type!=2 && !structured)return &resource;
        // C3 (I-0068 NPC-FAMILIES-STATIC §4.2): vertex programmes may sample material textures too (FlowmapVertexOffset t2).
        if((shader.stage!=ShaderStage::Pixel && shader.stage!=ShaderStage::Vertex) || resource.count!=1 ||
           (structured?resource.dimension!=1:(resource.dimension!=4 && resource.dimension!=9)) ||
           resource.shader_register>=64 ||
           (!structured && resource.return_type!=1 && resource.return_type!=2 && resource.return_type!=5))
            return &resource;
    }
    return nullptr;
}
ModelTextureStatus SelectModelShaderTextures(const ShaderInterface& shader,
    std::span<const ModelTextureBinding> bindings,std::vector<ModelShaderTexture>& output) {
    return SelectModelShaderTextures(shader,bindings,{},output);
}
ModelTextureStatus SelectModelShaderTextures(const ShaderInterface& shader,
    std::span<const ModelTextureBinding> bindings,std::span<const ModelSceneTextureBinding> scene,
    std::vector<ModelShaderTexture>& output) {
    for(size_t index=0;index<scene.size();++index){
        if(scene[index].slot>=64)return ModelTextureStatus::Invalid;
        for(size_t prior=0;prior<index;++prior)if(scene[prior].slot==scene[index].slot)return ModelTextureStatus::Invalid;
    }
    if(UnsupportedModelShaderTexture(shader))return ModelTextureStatus::Unsupported;
    std::vector<ModelShaderTexture> selected;
    for(const auto& resource:shader.resources){
        if(resource.input_type==0 || resource.input_type==3)continue;
        if(std::any_of(selected.begin(),selected.end(),[&](const auto& row){return row.slot==resource.shader_register;}))
            return ModelTextureStatus::Invalid;
        const auto found=std::find_if(bindings.begin(),bindings.end(),
            [&](const auto& row){return row.declaration.slot==resource.shader_register;});
        const auto common=std::find_if(scene.begin(),scene.end(),[&](const auto& row){return row.slot==resource.shader_register;});
        if(found!=bindings.end() && common!=scene.end())return ModelTextureStatus::Invalid;
        if(found!=bindings.end())selected.push_back({resource.shader_register,static_cast<size_t>(found-bindings.begin()),ModelTextureSource::Material});
        else if(common!=scene.end())selected.push_back({resource.shader_register,static_cast<size_t>(common-scene.begin()),ModelTextureSource::Scene});
        else return ModelTextureStatus::Missing;
    }
    output=std::move(selected);return ModelTextureStatus::Ready;
}
bool ProduceModelTextureTiling(std::span<const ModelTextureBinding> bindings,
    const ModelTextureTilingInputs& inputs,ModelTextureTiling& output) noexcept {
    if(bindings.size()>128 || inputs.buffer_register>=14 ||
       (inputs.offsets.available&~0xFFFFFu))return false;
    ModelTextureTiling candidate;candidate.buffer_register=inputs.buffer_register;
    // m45 collect cost: one buffer per thread, each used write reset (not 128 writes initialised per call: 2,907 calls a collect).
    thread_local std::array<TextureTilingWrite,128> writes;size_t count{};
    for(const auto& binding:bindings) {
        const auto& declaration=binding.declaration;
        if(declaration.slot>=128)return false;
        if(declaration.kind==8 && inputs.skip_kind8)continue;
        candidate.stages[0]|=(declaration.flags&1)!=0;
        candidate.stages[1]|=(declaration.flags&4)!=0 ||
                            (declaration.kind==5 && inputs.special_kind5);
        if(!((inputs.buffer_register>=0 && declaration.slot<25) || declaration.remap>=0))continue;
        candidate.stages[2]=true;
        const auto destination=declaration.remap>=0?static_cast<uint32_t>(declaration.remap):declaration.slot;
        if(destination>=25)continue; // Original has no destination write.
        auto& write=writes[count++];write=TextureTilingWrite{};
        write.row.source_slot=declaration.slot;write.row.array_index=destination;
        write.row.scale=binding.scale;
        write.row.animated=(inputs.offsets.mask&(1u<<(declaration.slot&31)))!=0;
        if(write.row.animated) {
            write.available=declaration.slot<20 && (inputs.offsets.available&(1u<<declaration.slot));
            if(write.available)write.row.animation_offset=inputs.offsets.pairs[declaration.slot];
        }
    }
    if(!ProduceTextureTiling(std::span(writes.data(),count),candidate.values))return false;
    // A negative register prevents the original buffer upload/bind entirely,
    // even though remapped CPU rows can still have been calculated.
    if(candidate.buffer_register<0)candidate.stages={};
    output=candidate;return true;
}
ShaderRegisterBlock PrepareModelTextureTiling(const ShaderInterface& shader,
    const ModelTextureTiling& tiling) {
    if(tiling.buffer_register<0)return {};
    if(tiling.buffer_register>=14)return {ShaderConstantStatus::Invalid};
    const size_t stage=shader.stage==ShaderStage::Vertex?0:shader.stage==ShaderStage::Pixel?2:1;
    if(stage==1)return {ShaderConstantStatus::Unsupported};
    std::vector<ShaderConstantRange> ranges;
    for(size_t word=0;word<tiling.values.written.size();) {
        if(tiling.values.written[word]>1)return {ShaderConstantStatus::Invalid};
        if(!tiling.values.written[word]){++word;continue;}
        const auto begin=word++;
        while(word<tiling.values.written.size() && tiling.values.written[word]==1)++word;
        ranges.push_back({static_cast<uint32_t>(begin*4),std::span(tiling.values.bytes).subspan(begin*4,(word-begin)*4)});
    }
    auto block=PrepareShaderRegister(shader,static_cast<uint32_t>(tiling.buffer_register),ranges);
    if(block.required && !tiling.stages[stage]){block.status=ShaderConstantStatus::Missing;block.bytes.clear();}
    return block;
}
}
