#include "primitive_programs.h"
#include <algorithm>
#include <fstream>
#include <iterator>
#include <cstdio>
#include <stdexcept>

using namespace ds3native::frame;
void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
bool SameInterface(const ds3content::ShaderInterface& a,const ds3content::ShaderInterface& b){
    const auto resource=[](const auto& x,const auto& y){return x.name==y.name && x.input_type==y.input_type && x.return_type==y.return_type && x.dimension==y.dimension &&
        x.samples==y.samples && x.shader_register==y.shader_register && x.count==y.count && x.flags==y.flags;};
    const auto variable=[](const auto& x,const auto& y){return x.name==y.name && x.byte_offset==y.byte_offset && x.bytes==y.bytes && x.flags==y.flags &&
        x.value_class==y.value_class && x.scalar_type==y.scalar_type && x.rows==y.rows && x.columns==y.columns && x.elements==y.elements && x.members==y.members;};
    const auto constant=[&](const auto& x,const auto& y){return x.name==y.name && x.declared_bytes==y.declared_bytes && x.flags==y.flags && x.buffer_type==y.buffer_type &&
        std::equal(x.variables.begin(),x.variables.end(),y.variables.begin(),y.variables.end(),variable);};
    const auto signature=[](const auto& x,const auto& y){return x.semantic==y.semantic && x.semantic_index==y.semantic_index && x.system_value==y.system_value &&
        x.component_type==y.component_type && x.shader_register==y.shader_register && x.mask==y.mask && x.usage_mask==y.usage_mask;};
    return a.stage==b.stage && std::equal(a.resources.begin(),a.resources.end(),b.resources.begin(),b.resources.end(),resource) &&
        std::equal(a.constants.begin(),a.constants.end(),b.constants.begin(),b.constants.end(),constant) &&
        std::equal(a.inputs.begin(),a.inputs.end(),b.inputs.begin(),b.inputs.end(),signature) &&
        std::equal(a.outputs.begin(),a.outputs.end(),b.outputs.begin(),b.outputs.end(),signature);
}
int main(int argc,char** argv) {
    try {
        Require(argc==2,"original executable required");std::ifstream file(argv[1],std::ios::binary);
        Require(bool(file),"read original executable");std::vector<uint8_t> image((std::istreambuf_iterator<char>(file)),{});
        PrimitiveProgramBank bank;const PrimitiveProgramVersion version{31,2};
        Require(DecodeOriginalPrimitiveProgramBank(image,version,bank)==PrimitiveProgramStatus::Ready && bank.ProgramCount()==603,"installed CPU programme admission");
        PrimitiveProgramPair pair;Require(SelectPrimitivePrograms(bank,version,3,1,pair)==PrimitiveProgramStatus::Ready,"qualified selector1");
        const auto vertex=pair.vertex.program.bytecode,pixel=pair.pixel.program.bytecode;
        {   // Shared declarations: a decoded hit is exactly the owned decode; equal checksum with other bytes, or another stage, never hits.
            const auto first=ds3content::InspectShaderInterfaceShared(pair.vertex.program),again=ds3content::InspectShaderInterfaceShared(pair.vertex.program);
            const auto owned=ds3content::InspectShaderInterface(pair.vertex.program);
            Require(first && first==again && SameInterface(*first,owned) && !owned.resources.empty() && !owned.inputs.empty(),"shared interface hit differs from its decode");
            const auto pixel_shared=ds3content::InspectShaderInterfaceShared(pair.pixel.program);
            Require(pixel_shared && pixel_shared!=first && SameInterface(*pixel_shared,ds3content::InspectShaderInterface(pair.pixel.program)),"shared pixel interface differs from its decode");
            auto altered=pair.vertex.program;altered.bytecode.back()^=1;std::shared_ptr<const ds3content::ShaderInterface> other;
            try{other=ds3content::InspectShaderInterfaceShared(altered);}catch(const std::runtime_error&){}
            Require(other!=first,"altered bytecode under the same checksum hit the shared interface");
            auto staged=pair.vertex.program;staged.stage=ds3content::ShaderStage::Pixel;std::shared_ptr<const ds3content::ShaderInterface> pixel_view;
            try{pixel_view=ds3content::InspectShaderInterfaceShared(staged);}catch(const std::runtime_error&){}
            Require(pixel_view!=first,"another stage hit the shared interface");
        }
        PrimitiveProgramPair vector;
        Require(SelectPrimitivePrograms(bank,version,3,1057,vector)==PrimitiveProgramStatus::Ready &&
            vector.vertex_layout.installed && vector.vertex_layout.supported && vector.vertex_layout.stride==16 &&
            vector.vertex_layout.elements.size()==3,"installed vector input metadata");
        const auto& elements=vector.vertex_layout.elements;
        Require(elements[0].semantic=="COLOR" && elements[0].semantic_index==0 && elements[0].byte_offset==0 && elements[0].format==DXGI_FORMAT_R8G8B8A8_UNORM &&
            elements[1].semantic=="COLOR" && elements[1].semantic_index==1 && elements[1].byte_offset==4 && elements[1].format==DXGI_FORMAT_R8G8B8A8_UNORM &&
            elements[2].semantic=="SV_Position" && elements[2].semantic_index==0 && elements[2].byte_offset==8 && elements[2].format==DXGI_FORMAT_R32G32_FLOAT,
            "vector metadata differs from actual descriptor346's explicit color/factor/position contract");
        Require(!vertex.empty() && !pixel.empty() && pair.vertex.program.stage==ds3content::ShaderStage::Vertex && pair.pixel.program.stage==ds3content::ShaderStage::Pixel,"actual stage pair");
        image[0]^=1;Require(DecodeOriginalPrimitiveProgramBank(image,{31,3},bank)==PrimitiveProgramStatus::Invalid && bank.Version()==version,"changed image replaced bank");
        image.clear();image.shrink_to_fit();
        Require(SelectPrimitivePrograms(bank,version,3,1073,vector)==PrimitiveProgramStatus::Ready &&
            vector.vertex_layout.elements[2].semantic=="SV_Position" && vector.vertex_layout.stride==16,"owned vertex metadata lifetime");
        PrimitiveProgramPair retained;Require(SelectPrimitivePrograms(bank,version,3,1,retained)==PrimitiveProgramStatus::Ready && retained.vertex.program.bytecode==vertex && retained.pixel.program.bytecode==pixel,"borrowed image lifetime retained");
        Require(SelectPrimitivePrograms(bank,{31,3},3,1,retained)==PrimitiveProgramStatus::Stale,"stale generation accepted");
        Require(SelectPrimitivePrograms(bank,version,2,1,retained)==PrimitiveProgramStatus::UnsupportedMode,"unqualified mode accepted");
        Require(SelectPrimitivePrograms(bank,version,3,0,retained)==PrimitiveProgramStatus::UnknownSelector,"unknown selector accepted");
        std::puts("PASS installed primitive programmes: exact original PE,603 owned DXBCs, qualified pair, shared interface hit/altered bytes/stage, source mutation/lifetime, stale/mode/selector refusal");return 0;
    }catch(const std::exception& error){std::fprintf(stderr,"FAIL: %s\n",error.what());return 1;}
}
