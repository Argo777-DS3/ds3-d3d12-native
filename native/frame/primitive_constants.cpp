#include "primitive_constants.h"
#include <algorithm>
#include <cstring>
#include <set>
#include <stdexcept>

namespace ds3native::frame {
namespace {
struct CpuUniform {int16_t shader_register,source_float,total_floats;uint8_t element_floats,kind,instance_stride;};
CpuUniform Uniform(const PrimitiveUniformLayout& layout,size_t index){
    const auto* row=layout.uniforms.data()+index*10;CpuUniform value{};
    std::memcpy(&value.shader_register,row,2);std::memcpy(&value.source_float,row+2,2);
    std::memcpy(&value.total_floats,row+6,2);value.element_floats=row[4];value.kind=row[8];value.instance_stride=row[9];return value;
}
struct UniformVariable {SceneShaderStage stage;std::string name;size_t source,count;};
PrimitiveConstantStatus UniformVariables(const PrimitiveProgramPair& pair,std::vector<UniformVariable>& output){
    if(!pair.vertex_uniforms.installed || !pair.pixel_uniforms.installed)return PrimitiveConstantStatus::Missing;
    std::vector<UniformVariable> candidate;
    for(unsigned stage=0;stage<2;++stage){
        const auto& program=stage?pair.pixel.program:pair.vertex.program;
        const auto& layout=stage?pair.pixel_uniforms:pair.vertex_uniforms;
        const auto shared=ds3content::InspectShaderInterfaceShared(program);const auto& shader=*shared;
        std::vector<const ds3content::ShaderResourceDeclaration*> blocks;
        for(const auto& resource:shader.resources)if(resource.input_type==0)blocks.push_back(&resource);
        if(blocks.size()>1 || (!blocks.empty() && (blocks[0]->shader_register || blocks[0]->count!=1)))return PrimitiveConstantStatus::Unsupported;
        std::set<size_t> matched;
        if(!blocks.empty()){
            const auto block=std::find_if(shader.constants.begin(),shader.constants.end(),[&](const auto& c){return c.name==blocks[0]->name;});
            if(block==shader.constants.end() || block->buffer_type || !block->declared_bytes || block->declared_bytes>65536)return PrimitiveConstantStatus::Unsupported;
            auto vars=block->variables;std::sort(vars.begin(),vars.end(),[](const auto& a,const auto& b){return a.byte_offset<b.byte_offset;});size_t position{};
            for(const auto& var:vars){
                const auto elements=var.elements?size_t(var.elements):size_t(1);
                if(var.value_class!=1 || var.scalar_type!=3 || var.rows!=1 || var.columns!=4 || var.members ||
                    var.bytes!=elements*16 || var.byte_offset!=position)return PrimitiveConstantStatus::Unsupported;
                size_t found=25;
                for(size_t index=0;index<25;++index){const auto row=Uniform(layout,index);
                    if(row.kind || row.shader_register<0 || !row.total_floats)continue;
                    if(size_t(row.shader_register)*16==var.byte_offset){if(found!=25)return PrimitiveConstantStatus::Unsupported;found=index;}
                }
                if(found==25)return PrimitiveConstantStatus::Unsupported;
                const auto row=Uniform(layout,found);
                if(row.total_floats<0 || size_t(row.total_floats)*4!=var.bytes || row.source_float<0 || !row.element_floats ||
                    size_t(row.source_float)+size_t(row.total_floats)>1000 || layout.uniforms[found*10+5])return PrimitiveConstantStatus::Unsupported;
                matched.insert(found);candidate.push_back({stage?SceneShaderStage::Pixel:SceneShaderStage::Vertex,var.name,size_t(row.source_float),elements});position+=var.bytes;
            }
            if(position!=block->declared_bytes)return PrimitiveConstantStatus::Unsupported;
        }
        for(size_t index=0;index<25;++index){const auto row=Uniform(layout,index);
            if(!row.kind && row.total_floats && !matched.contains(index))return PrimitiveConstantStatus::Unsupported;
        }
    }
    output=std::move(candidate);return PrimitiveConstantStatus::Ready;
}
}
PrimitiveConstantStatus PrimitiveUniformSource::Begin(const PrimitiveProgramPair& pair,uint64_t serial,uint64_t draw_identity){
    if(!serial || !draw_identity || !pair.version.incarnation || !pair.version.generation)return PrimitiveConstantStatus::Invalid;
    try{std::vector<UniformVariable> vars;const auto status=UniformVariables(pair,vars);if(status!=PrimitiveConstantStatus::Ready)return status;
        pair_=pair;serial_=serial;draw_identity_=draw_identity;shadow_.fill(0);written_.fill(false);return PrimitiveConstantStatus::Ready;
    }catch(const std::runtime_error&){return PrimitiveConstantStatus::Invalid;}
}
PrimitiveConstantStatus PrimitiveUniformSource::Write(uint32_t semantic,std::span<const float> values,uint32_t element_offset,uint32_t instance){
    if(!serial_ || semantic>=25 || values.empty() || values.size()>1000)return PrimitiveConstantStatus::Invalid;
    uint32_t uniform=semantic;uint64_t offset=element_offset;
    // Original CPU semantic selection checks the vertex triple before pixel.
    for(const auto* layout:{&pair_.vertex_uniforms,&pair_.pixel_uniforms}){
        const auto at=size_t(semantic)*3;
        if(layout->semantics[at+1]<0)continue;
        const auto selected=layout->semantics[at];if(selected<0 || selected>=25)return PrimitiveConstantStatus::Unsupported;
        uniform=static_cast<uint32_t>(selected);offset+=uint8_t(layout->semantics[at+1])+uint64_t(Uniform(*layout,uniform).instance_stride)*instance;break;
    }
    return WriteUniform(uniform,values,offset);
}
PrimitiveConstantStatus PrimitiveUniformSource::WriteUniform(uint32_t uniform,std::span<const float> values,uint64_t offset){
    if(!serial_ || uniform>=25 || values.empty() || values.size()>1000 || offset>UINT32_MAX)return PrimitiveConstantStatus::Invalid;
    auto shadow=shadow_;auto written=written_;
    for(const auto* layout:{&pair_.vertex_uniforms,&pair_.pixel_uniforms}){
        const auto row=Uniform(*layout,uniform);if(!row.total_floats)continue;
        if(row.kind || row.total_floats<0 || row.source_float<0 || !row.element_floats)return PrimitiveConstantStatus::Unsupported;
        const auto displacement=uint64_t(row.element_floats)*offset;
        if(displacement>uint64_t(row.total_floats) || values.size()>uint64_t(row.total_floats)-displacement)return PrimitiveConstantStatus::Invalid;
        const auto start=size_t(row.source_float)+size_t(displacement);
        if(start>shadow.size() || values.size()>shadow.size()-start)return PrimitiveConstantStatus::Invalid;
        std::copy(values.begin(),values.end(),shadow.begin()+start);std::fill_n(written.begin()+start,values.size(),true);
    }
    shadow_=shadow;written_=written;return PrimitiveConstantStatus::Ready;
}
void PrimitiveUniformSource::CompleteWrittenRows(){
    for(const auto* layout:{&pair_.vertex_uniforms,&pair_.pixel_uniforms})for(size_t index=0;index<25;++index){const auto row=Uniform(*layout,index);
        if(row.kind || row.total_floats<=0 || row.source_float<0 || !row.element_floats)continue;
        const auto begin=size_t(row.source_float),end=std::min(shadow_.size(),begin+size_t(row.total_floats));
        const auto first=row.instance_stride?std::min(end,begin+size_t(row.instance_stride)*row.element_floats):end;
        if(begin>=end || std::none_of(written_.begin()+begin,written_.begin()+first,[](bool w){return w;}))continue;
        for(size_t at=begin;at<end;++at)if(!written_[at]){shadow_[at]=0;written_[at]=true;}}
}
PrimitiveConstantStatus PrimitiveUniformSource::Resolve(PrimitiveProgramVersion current,uint64_t serial,PrimitiveDrawConstants& output) const{
    if(!serial_)return PrimitiveConstantStatus::Invalid;
    if(pair_.version!=current || serial_!=serial)return PrimitiveConstantStatus::Stale;
    try{std::vector<UniformVariable> vars;const auto status=UniformVariables(pair_,vars);if(status!=PrimitiveConstantStatus::Ready)return status;
        PrimitiveDrawConstants candidate{draw_identity_,{}};
        for(const auto& var:vars){
            const auto floats=var.count*4;
            if(!std::all_of(written_.begin()+var.source,written_.begin()+var.source+floats,[](bool value){return value;}))return PrimitiveConstantStatus::Missing;
            PrimitiveFloat4Values values{var.stage,var.name,{}};values.values.resize(var.count);
            std::memcpy(values.values.data(),shadow_.data()+var.source,floats*sizeof(float));candidate.variables.push_back(std::move(values));
        }
        std::vector<SceneConstantBinding> verified;const auto packed=PackPrimitiveConstants(pair_,candidate,verified);
        if(packed!=PrimitiveConstantStatus::Ready)return packed;
        output=std::move(candidate);return PrimitiveConstantStatus::Ready;
    }catch(const std::runtime_error&){return PrimitiveConstantStatus::Invalid;}
}
PrimitiveConstantStatus PackPrimitiveConstants(const PrimitiveProgramPair& pair,const PrimitiveDrawConstants& values,std::vector<SceneConstantBinding>& output){
    if(!values.draw_identity || !pair.version.incarnation || !pair.version.generation || values.variables.size()>256)return PrimitiveConstantStatus::Invalid;
    try{
        std::set<std::pair<SceneShaderStage,std::string>> supplied,required;
        for(const auto& v:values.variables)if((v.stage!=SceneShaderStage::Vertex && v.stage!=SceneShaderStage::Pixel) ||
            v.variable.empty() || v.variable.size()>2048 || v.values.empty() || v.values.size()>4096 || !supplied.emplace(v.stage,v.variable).second)return PrimitiveConstantStatus::Invalid;
        std::vector<SceneConstantBinding> candidate;
        for(const auto* program:{&pair.vertex.program,&pair.pixel.program}){
            const auto stage=program==&pair.vertex.program?SceneShaderStage::Vertex:SceneShaderStage::Pixel;
            if(program->stage!=(stage==SceneShaderStage::Vertex?ds3content::ShaderStage::Vertex:ds3content::ShaderStage::Pixel))return PrimitiveConstantStatus::Invalid;
            const auto shared=ds3content::InspectShaderInterfaceShared(*program);const auto& declaration=*shared;
            std::vector<const ds3content::ShaderResourceDeclaration*> blocks;
            for(const auto& resource:declaration.resources)if(resource.input_type==0)blocks.push_back(&resource);
            if(blocks.empty())continue;
            if(blocks.size()!=1 || blocks[0]->shader_register || blocks[0]->count!=1)return PrimitiveConstantStatus::Unsupported;
            const auto block=std::find_if(declaration.constants.begin(),declaration.constants.end(),[&](const auto& c){return c.name==blocks[0]->name;});
            if(block==declaration.constants.end() || block->buffer_type || !block->declared_bytes || block->declared_bytes>65536 || block->variables.empty())return PrimitiveConstantStatus::Unsupported;
            auto variables=block->variables;std::sort(variables.begin(),variables.end(),[](const auto& a,const auto& b){return a.byte_offset<b.byte_offset;});
            SceneConstantBinding binding{stage,0,{}};binding.bytes.reserve(block->declared_bytes);size_t position{};
            for(const auto& v:variables){
                // RDEF class VECTOR=1, type FLOAT=3. This policy never guesses
                // array stride, scalar/matrix packing, holes or struct layout.
                const auto count=v.elements?size_t(v.elements):size_t(1);
                if(v.value_class!=1 || v.scalar_type!=3 || v.rows!=1 || v.columns!=4 || v.members ||
                    count>4096 || v.bytes!=count*16 || v.byte_offset!=position)return PrimitiveConstantStatus::Unsupported;
                required.emplace(stage,v.name);
                const auto input=std::find_if(values.variables.begin(),values.variables.end(),[&](const auto& x){return x.stage==stage && x.variable==v.name;});
                if(input==values.variables.end())return PrimitiveConstantStatus::Missing;
                if(input->values.size()!=count)return PrimitiveConstantStatus::Invalid;
                binding.bytes.resize(position+v.bytes);std::memcpy(binding.bytes.data()+position,input->values.data(),v.bytes);position+=v.bytes;
            }
            if(position!=block->declared_bytes)return PrimitiveConstantStatus::Unsupported;
            candidate.push_back(std::move(binding));
        }
        if(supplied!=required)return PrimitiveConstantStatus::Invalid;
        output=std::move(candidate);return PrimitiveConstantStatus::Ready;
    }catch(const std::runtime_error&){return PrimitiveConstantStatus::Invalid;}
}
}
