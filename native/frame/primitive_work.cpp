#include "primitive_work.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <set>
#include <stdexcept>

namespace ds3native::frame {
namespace {
struct Unsupported : std::exception {};
struct Missing {PrimitiveWorkStatus status;};
void Valid(bool value){if(!value)throw std::runtime_error("invalid owned primitive input");}
void Supported(bool value){if(!value)throw Unsupported{};}
uint32_t ElementBytes(DXGI_FORMAT format) {
    switch(format) {
    case DXGI_FORMAT_R32G32_FLOAT:return 8;
    case DXGI_FORMAT_R8G8B8A8_UNORM:return 4;
    case DXGI_FORMAT_R8_UINT:return 1;
    default:return 0;
    }
}
uint32_t ElementType(DXGI_FORMAT format){return format==DXGI_FORMAT_R8_UINT?1u:ElementBytes(format)?3u:0u;}
void Interface(const ds3content::ShaderInterface& vs,const ds3content::ShaderInterface& ps,const PrimitiveDraw& draw) {
    Supported(ps.outputs.size()==1 && ps.outputs[0].semantic=="SV_Target" && !ps.outputs[0].semantic_index &&
        ps.outputs[0].component_type==3 && !ps.outputs[0].shader_register);
    for(const auto& signature:vs.inputs) {
        // Native instanced execution supplies the two observed system inputs.
        if(signature.system_value==6 || signature.system_value==8) {
            Supported(signature.component_type==1 && signature.mask==1);continue;
        }
        const auto element=std::find_if(draw.elements.begin(),draw.elements.end(),[&](const auto& e){
            return e.semantic==signature.semantic && e.semantic_index==signature.semantic_index;});
        // IA formats supply defined missing components (XYZ0/W1). Actual DS3
        // float2 position layouts intentionally feed a float4 VS declaration.
        // Retain the real format; never invent extra vertex payload bytes.
        Supported(!signature.system_value && element!=draw.elements.end() && ElementType(element->format)==signature.component_type);
    }
    for(const auto& signature:ps.inputs) {
        const auto element=std::find_if(vs.outputs.begin(),vs.outputs.end(),[&](const auto& e){
            return e.semantic==signature.semantic && e.semantic_index==signature.semantic_index;});
        Supported(element!=vs.outputs.end() && element->component_type==signature.component_type &&
            element->system_value==signature.system_value && !(signature.usage_mask&~element->mask));
    }
    std::set<SceneShaderStage> required_constants;
    std::array<bool,2> texture{},sampler{};
    for(const auto* shader:{&vs,&ps}) {
        const auto stage=shader==&vs?SceneShaderStage::Vertex:SceneShaderStage::Pixel;
        std::set<std::pair<uint32_t,uint32_t>> declarations;
        for(const auto& resource:shader->resources) {
            Supported(resource.count>=1 && resource.count<=2 && declarations.emplace(resource.input_type,resource.shader_register).second);
            if(resource.input_type==0) {
                Supported(resource.count==1 && !resource.shader_register);
                const auto block=std::find_if(shader->constants.begin(),shader->constants.end(),[&](const auto& c){return c.name==resource.name;});
                Valid(block!=shader->constants.end());Supported(!block->buffer_type && block->declared_bytes && block->declared_bytes<=65536);
                const auto input=std::find_if(draw.constants.begin(),draw.constants.end(),[&](const auto& b){return b.stage==stage && !b.shader_register;});
                if(input==draw.constants.end())throw Missing{PrimitiveWorkStatus::MissingConstants};
                Valid(input->bytes.size()==block->declared_bytes);required_constants.insert(stage);
            } else if(resource.input_type==2) {
                Supported(stage==SceneShaderStage::Pixel && resource.dimension==4 && (resource.return_type==1 || resource.return_type==2 || resource.return_type==5));
                Supported(resource.shader_register<2 && resource.count<=2-resource.shader_register);
                for(uint32_t slot=resource.shader_register;slot<resource.shader_register+resource.count;++slot)texture[slot]=true;
            } else if(resource.input_type==3) {
                Supported(stage==SceneShaderStage::Pixel && resource.shader_register<2 && resource.count<=2-resource.shader_register);
                for(uint32_t slot=resource.shader_register;slot<resource.shader_register+resource.count;++slot)sampler[slot]=true;
            } else throw Unsupported{};
        }
    }
    Valid(draw.constants.size()==required_constants.size());
    for(const auto& cb:draw.constants)Valid(required_constants.contains(cb.stage) && !cb.shader_register);
    Valid(texture[0]==bool(draw.texture) && sampler[0]==(draw.sampler!=PrimitiveSampler::None) &&
        texture[1]==bool(draw.secondary.texture) && sampler[1]==(draw.secondary.sampler!=PrimitiveSampler::None));
}
}
bool GetPrimitiveTextureLayout(uint32_t width,uint32_t height,DXGI_FORMAT format,PrimitiveTextureLayout& output) noexcept {
    if(!width || !height || width>8192 || height>8192)return false;
    PrimitiveTextureLayout candidate{};
    if(format==DXGI_FORMAT_R8_UNORM)candidate={width,height,1,1,1};
    else if(format==DXGI_FORMAT_R8G8B8A8_UNORM || format==DXGI_FORMAT_B8G8R8A8_UNORM)candidate={width*4,height,1,1,4};
    else if(format==DXGI_FORMAT_BC1_UNORM)candidate={std::max(1u,(width+3)/4)*8,std::max(1u,(height+3)/4),4,4,8};
    else if(format==DXGI_FORMAT_BC2_UNORM || format==DXGI_FORMAT_BC3_UNORM || format==DXGI_FORMAT_BC7_UNORM)
        candidate={std::max(1u,(width+3)/4)*16,std::max(1u,(height+3)/4),4,4,16};
    else return false;
    output=candidate;return true;
}
namespace {
bool ValidPrimitiveTextureShape(const SceneTexture& t,bool external) noexcept {
    if(t.shader_slot || !t.width || t.width>8192 || !t.height || t.height>8192 ||
       t.mips.empty() || t.mips.size()>14 || (1u<<(t.mips.size()-1))>std::max(t.width,t.height))return false;
    for(size_t i=0;i<t.mips.size();++i){
        PrimitiveTextureLayout layout{};
        if(!GetPrimitiveTextureLayout(std::max(1u,t.width>>i),std::max(1u,t.height>>i),t.format,layout) ||
           (t.mips[i].size()!=uint64_t(layout.row_bytes)*layout.rows && !(external && t.mips[i].empty())))return false;
    }
    return true;
}
}
bool ValidPrimitiveTexturePayload(const SceneTexture& t) noexcept {return ValidPrimitiveTextureShape(t,false);}
bool ValidPrimitiveTexture(const PrimitiveTexture& t) noexcept {return ValidPrimitiveTextureShape(t.texture,t.external || t.target);}
static PrimitiveWorkResult CheckPrimitiveWork(const PrimitiveWorkSource& input) {
    PrimitiveWorkResult result{};
    try {
        Valid(input.content_incarnation && input.generation && input.texture_generation && input.serial &&
            input.width && input.width<=8192 && input.height && input.height<=8192 &&
            !input.draws.empty() && input.draws.size()<=4096 && input.programs.size()<=2048 &&
            input.buffers.size()<=8192 && input.textures.size()<=4096);   // F6: filter passes and their target samples raise the UI frame extents
        std::map<uint64_t,const PrimitiveProgram*> programs;
        std::map<uint64_t,std::shared_ptr<const ds3content::ShaderInterface>> interfaces;
        std::map<uint64_t,const PrimitiveBuffer*> buffers;
        std::map<uint64_t,const PrimitiveTexture*> textures;
        std::map<uint64_t,const PrimitiveTarget*> targets;
        Valid(input.targets.size()<=4096);
        for(const auto& target:input.targets)
            Valid(target.identity && targets.emplace(target.identity,&target).second && target.width && target.width<=8192 && target.height && target.height<=8192);
        size_t bytes{};
        auto Count=[&](size_t size){Valid(size && size<=256u*1024u*1024u-bytes);bytes+=size;};
        for(const auto& program:input.programs) {
            Valid(program.identity && programs.emplace(program.identity,&program).second);
            Supported(program.program.stage==ds3content::ShaderStage::Vertex || program.program.stage==ds3content::ShaderStage::Pixel);
            Count(program.program.bytecode.size());interfaces.emplace(program.identity,ds3content::InspectShaderInterfaceShared(program.program));
        }
        for(const auto& buffer:input.buffers) {
            Valid(buffer.identity && buffers.emplace(buffer.identity,&buffer).second && buffer.bytes.size()<=64u*1024u*1024u);Count(buffer.bytes.size());
        }
        for(const auto& texture:input.textures) {
            Valid(texture.identity && textures.emplace(texture.identity,&texture).second);
            if(texture.target){const auto found=targets.find(texture.target);   // F1: a packet target sampled as drawn
                Valid(found!=targets.end() && !texture.external && texture.texture.width==found->second->width && texture.texture.height==found->second->height &&
                    texture.texture.format==DXGI_FORMAT_R8G8B8A8_UNORM && texture.texture.mips.size()==1 && texture.texture.mips[0].empty());}
            const auto& t=texture.texture;Valid(!t.shader_slot && t.width && t.width<=8192 && t.height && t.height<=8192 &&
                !t.mips.empty() && t.mips.size()<=14 && (1u<<(t.mips.size()-1))<=std::max(t.width,t.height));
            PrimitiveTextureLayout layout{};Supported(GetPrimitiveTextureLayout(t.width,t.height,t.format,layout));
            for(size_t i=0;i<t.mips.size();++i) {
                Valid(GetPrimitiveTextureLayout(std::max(1u,t.width>>i),std::max(1u,t.height>>i),t.format,layout) &&
                      (t.mips[i].size()==uint64_t(layout.row_bytes)*layout.rows || ((texture.external || texture.target) && t.mips[i].empty())));
                if(!t.mips[i].empty())Count(t.mips[i].size());
            }
        }
        std::set<uint64_t> draw_ids;
        uint64_t work_items{};
        for(const auto& draw:input.draws) {
            result.draw_identity=draw.identity;
            Valid(draw.identity && draw_ids.insert(draw.identity).second && draw.count && draw.count<=1024u*1024u &&
                draw.instances && draw.instances<=4096 && draw.stride && draw.stride<=256 && draw.elements.size()<=16 &&
                !draw.elements.empty() && draw.constants.size()<=2 && draw.blend<=PrimitiveBlend::PremultipliedAdditive &&
                draw.stencil<=PrimitiveStencil::ClampToReference && draw.stencil_reference<=255 && draw.sampler<=PrimitiveSampler::PointClamp && draw.secondary.sampler<=PrimitiveSampler::PointClamp);
            const auto count=uint64_t(draw.count)*draw.instances;
            Valid(count<=8u*1024u*1024u-work_items);work_items+=count;
            if(!programs.contains(draw.vertex_program) || !programs.contains(draw.pixel_program))throw Missing{PrimitiveWorkStatus::MissingProgram};
            Valid(programs.at(draw.vertex_program)->program.stage==ds3content::ShaderStage::Vertex &&
                programs.at(draw.pixel_program)->program.stage==ds3content::ShaderStage::Pixel);
            if(!buffers.contains(draw.vertices) || (draw.indexed && !buffers.contains(draw.indices)))throw Missing{PrimitiveWorkStatus::MissingBuffer};
            if(draw.texture && !textures.contains(draw.texture))throw Missing{PrimitiveWorkStatus::MissingTexture};
            if(draw.secondary.texture && !textures.contains(draw.secondary.texture))throw Missing{PrimitiveWorkStatus::MissingTexture};
            // F1: the draw's target extent bounds its viewport and scissor; a draw never samples the target it writes.
            uint32_t target_width=input.width,target_height=input.height;
            if(draw.target){const auto found=targets.find(draw.target);Valid(found!=targets.end());
                target_width=found->second->width;target_height=found->second->height;
                for(const auto sampled:{draw.texture,draw.secondary.texture})Valid(!sampled || textures.at(sampled)->target!=draw.target);}
            std::set<std::pair<std::string,uint32_t>> semantics;
            for(const auto& element:draw.elements) {
                const auto size=ElementBytes(element.format);Supported(size!=0);
                Valid(!element.semantic.empty() && element.semantic.size()<=32 && element.semantic_index<=15 &&
                    element.byte_offset<=draw.stride && size<=draw.stride-element.byte_offset &&
                    semantics.emplace(element.semantic,element.semantic_index).second &&
                    std::all_of(element.semantic.begin(),element.semantic.end(),[](char c){return (c>='A'&&c<='Z') || (c>='a'&&c<='z') || c=='_';}));
            }
            Interface(*interfaces.at(draw.vertex_program),*interfaces.at(draw.pixel_program),draw);
            for(const auto& cb:draw.constants)Count(cb.bytes.size());
            const auto& viewport=draw.viewport;
            Valid(std::isfinite(viewport.x) && std::isfinite(viewport.y) && std::isfinite(viewport.width) && std::isfinite(viewport.height) &&
                std::isfinite(viewport.min_depth) && std::isfinite(viewport.max_depth) && viewport.width>0 && viewport.height>0 &&
                viewport.x>=0 && viewport.y>=0 && viewport.x+viewport.width<=target_width && viewport.y+viewport.height<=target_height &&
                viewport.min_depth>=0 && viewport.max_depth<=1 && viewport.min_depth<=viewport.max_depth);
            if(draw.scissor_enabled)Valid(draw.scissor[0]>=0 && draw.scissor[1]>=0 && draw.scissor[2]>draw.scissor[0] &&
                draw.scissor[3]>draw.scissor[1] && uint32_t(draw.scissor[2])<=target_width && uint32_t(draw.scissor[3])<=target_height);
            Valid(std::isfinite(draw.mip_bias) && std::isfinite(draw.min_lod) && std::isfinite(draw.max_lod) &&
                draw.min_lod>=0 && draw.min_lod<=draw.max_lod && draw.mip_bias>=-16 && draw.mip_bias<=15.99f);
            Valid(std::isfinite(draw.secondary.mip_bias) && std::isfinite(draw.secondary.min_lod) && std::isfinite(draw.secondary.max_lod) &&
                draw.secondary.min_lod>=0 && draw.secondary.min_lod<=draw.secondary.max_lod && draw.secondary.mip_bias>=-16 && draw.secondary.mip_bias<=15.99f);
            Supported((draw.stencil==PrimitiveStencil::ReplaceZero || draw.stencil==PrimitiveStencil::IncrementEqualZero || draw.stencil==PrimitiveStencil::WriteMaskLevel || draw.stencil==PrimitiveStencil::ClampToReference)?
                draw.blend==PrimitiveBlend::NoColor:draw.blend!=PrimitiveBlend::NoColor);
            Valid(draw.stencil!=PrimitiveStencil::WriteMaskLevel || draw.stencil_reference<255);
            Valid(draw.stencil!=PrimitiveStencil::ReadMaskLevel || draw.stencil_reference>0);
            uint64_t first=draw.start,last=uint64_t(draw.start)+draw.count;
            if(draw.indexed) {
                Supported(draw.index_width==2 || draw.index_width==4);
                const auto& ib=buffers.at(draw.indices)->bytes;
                const uint64_t start=uint64_t(draw.index_offset)+uint64_t(draw.start)*draw.index_width;
                Valid(start<=ib.size() && uint64_t(draw.count)*draw.index_width<=ib.size()-start);
                first=UINT64_MAX;last=0;
                for(uint64_t i=0;i<draw.count;++i) {
                    uint32_t index{};std::memcpy(&index,ib.data()+start+i*draw.index_width,draw.index_width);
                    const int64_t vertex=int64_t(index)+draw.base_vertex;Valid(vertex>=0);
                    first=std::min(first,uint64_t(vertex));last=std::max(last,uint64_t(vertex)+1);
                }
            } else Valid(!draw.indices && !draw.index_width && !draw.index_offset && !draw.base_vertex);
            const auto& vb=buffers.at(draw.vertices)->bytes;
            Valid(first<last && draw.vertex_offset<=vb.size() && last<=uint64_t(vb.size()-draw.vertex_offset)/draw.stride);
        }
        result.status=PrimitiveWorkStatus::Ready;
    } catch(const Missing& missing) {result.status=missing.status;}
      catch(const Unsupported&) {result.status=PrimitiveWorkStatus::Unsupported;}
      catch(const std::runtime_error&) {result.status=PrimitiveWorkStatus::Invalid;}
    return result;
}
PrimitiveWorkResult ProducePrimitiveWork(const PrimitiveWorkSource& input,PrimitiveWorkPacket& output) {
    const auto result=CheckPrimitiveWork(input);
    if(result.status==PrimitiveWorkStatus::Ready)output=PrimitiveWorkPacket{input.content_incarnation,input.generation,input.texture_generation,input.serial,
        input.width,input.height,input.programs,input.buffers,input.textures,input.draws,input.targets};
    return result;
}
PrimitiveWorkResult ProducePrimitiveWork(PrimitiveWorkSource&& input,PrimitiveWorkPacket& output) {
    const auto result=CheckPrimitiveWork(input);
    if(result.status==PrimitiveWorkStatus::Ready)output=PrimitiveWorkPacket{input.content_incarnation,input.generation,input.texture_generation,input.serial,
        input.width,input.height,std::move(input.programs),std::move(input.buffers),std::move(input.textures),std::move(input.draws),std::move(input.targets)};
    return result;
}
}
