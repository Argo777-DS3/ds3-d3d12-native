#include "primitive_constants.h"
#include "game_ui_shaders.h"
#include "game_ui_primitive.h"
#include <fstream>
#include <iterator>
#include <cstdio>
#include <cstring>
#include <stdexcept>
using namespace ds3native;
using namespace ds3native::frame;
namespace catalogue {
struct Recipe {uint32_t selector,record;int32_t vertex,pixel;bool available;};
struct ProgramPin {uint32_t stage;int32_t index;uint32_t bytes;const char* sha256;};
#include "primitive_program_catalog.inc"
}
void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void CpuFailure(const char* reason) noexcept {std::fprintf(stderr,"FAIL native CPU callback: %s\n",reason);std::terminate();}
uintptr_t CpuPrepareControl(uintptr_t,uintptr_t fill,uint32_t* flags,uint32_t,uintptr_t,uint32_t,uintptr_t,uintptr_t,uintptr_t writer){
    // A control for the callback ABI, never installed as game content/producer.
    const auto* table=*reinterpret_cast<const uintptr_t* const*>(writer);
    using Select=uint8_t(*)(uintptr_t,uint32_t,uintptr_t);
    using Write=void(*)(uintptr_t,uintptr_t,uint32_t,uintptr_t,uint32_t,int32_t,uint32_t);
    Require(reinterpret_cast<Select>(table[2])(writer,1,0)==0,"CPU selector claimed legacy GPU readiness");
    Require(!*reinterpret_cast<uintptr_t*>(writer+0xFF8) && !*reinterpret_cast<uintptr_t*>(writer+0x1008) &&
        *reinterpret_cast<uintptr_t*>(writer+0x1000) && *reinterpret_cast<uintptr_t*>(writer+0x1010),"CPU metadata/GPU slot contract");
    reinterpret_cast<Write>(table[1])(writer,writer+0xFF8,13,fill,8,0,0);
    if(!(*flags&1))reinterpret_cast<Write>(table[1])(writer,writer+0xFF8,21,fill+32,8,0,0);
    *flags|=8;return writer+0xFF8;
}
int main(int argc,char** argv){
    try{
        bootstrap::UiPrimitiveState state;const PrimitiveViewport viewport{3,5,79,57,0,0};
        Require(bootstrap::DecodeUiPrimitiveState(1,0,1,0,0,viewport,state) && state.blend==PrimitiveBlend::StraightAlpha &&
            state.stencil==PrimitiveStencil::Disabled && state.viewport.x==3,"actual normal alpha state");
        // Mask contract since5e78b3c2 (sources503-505): depth3 clamps to the
        // current level, depth4 writes a level, depth5 reads level1 or deeper.
        Require(bootstrap::DecodeUiPrimitiveState(3,0,11,0,0,viewport,state) && state.blend==PrimitiveBlend::NoColor &&
            state.stencil==PrimitiveStencil::ClampToReference && state.stencil_reference==0,"actual mask state ignores current color blend");
        Require(bootstrap::DecodeUiPrimitiveState(4,2,1,0,0,viewport,state) && state.blend==PrimitiveBlend::NoColor &&
            state.stencil==PrimitiveStencil::WriteMaskLevel && state.stencil_reference==2,"actual nested mask level write");
        Require(bootstrap::DecodeUiPrimitiveState(5,1,1,1,0,viewport,state) && state.blend==PrimitiveBlend::PremultipliedAlpha &&
            state.stencil==PrimitiveStencil::ReadOne,"actual premultiplied masked state");
        Require(bootstrap::DecodeUiPrimitiveState(5,2,1,0,0,viewport,state) && state.stencil==PrimitiveStencil::ReadMaskLevel &&
            state.stencil_reference==2,"actual nested masked read");
        Require(!bootstrap::DecodeUiPrimitiveState(5,0,1,0,0,viewport,state) && state.stencil==PrimitiveStencil::ReadMaskLevel &&
            !bootstrap::DecodeUiPrimitiveState(4,255,1,0,0,viewport,state) &&
            !bootstrap::DecodeUiPrimitiveState(1,0,9,0,0,viewport,state),"unknown stencil reference/blend silently substituted");
        Require(argc==2,"installed executable required");std::ifstream file(argv[1],std::ios::binary);
        Require(bool(file),"installed executable read");std::vector<uint8_t> image((std::istreambuf_iterator<char>(file)),{});
        const PrimitiveProgramVersion version{51,7};PrimitiveProgramBank bank;
        Require(DecodeOriginalPrimitiveProgramBank(image,version,bank)==PrimitiveProgramStatus::Ready,"installed CPU metadata");
        image.clear();image.shrink_to_fit();PrimitiveProgramPair single,batch;
        Require(SelectPrimitivePrograms(bank,version,3,1,single)==PrimitiveProgramStatus::Ready && single.vertex_uniforms.installed && single.pixel_uniforms.installed,"single metadata owned");
        Require(SelectPrimitivePrograms(bank,version,3,3,batch)==PrimitiveProgramStatus::Ready,"batch metadata owned");
        size_t supported{},unsupported{},unavailable{};
        for(const auto& recipe:catalogue::recipes){
            PrimitiveProgramPair pair;const auto selected=SelectPrimitivePrograms(bank,version,3,recipe.selector,pair);
            if(!recipe.available){Require(selected==PrimitiveProgramStatus::Missing,"unavailable recipe substituted");++unavailable;continue;}
            Require(selected==PrimitiveProgramStatus::Ready,"available installed recipe missing");PrimitiveUniformSource audit;
            const auto status=audit.Begin(pair,11,70);
            if(status==PrimitiveConstantStatus::Ready)++supported;
            else{Require(status==PrimitiveConstantStatus::Unsupported,"installed metadata invalid");if(unsupported++<8)std::printf("UNSUPPORTED current UI uniform layout: selector%u VS%d PS%d\n",recipe.selector,pair.vertex.program.id,pair.pixel.program.id);}
        }
        std::printf("Installed UI uniform layout inventory: supported%zu unsupported%zu unavailable%zu\n",supported,unsupported,unavailable);
        PrimitiveUniformSource values;PrimitiveDrawConstants output{900,{}};
        Require(values.Begin(single,12,71)==PrimitiveConstantStatus::Ready,"single layout matches actual RDEF");
        Require(values.Resolve(version,12,output)==PrimitiveConstantStatus::Missing && output.draw_identity==900,"unwritten matrix accepted");
        const std::array<float,8> matrix{1,2,3,4,5,6,7,8},texture{9,10,11,12,13,14,15,16};
        Require(values.Write(13,matrix)==PrimitiveConstantStatus::Ready,"matrix semantic");
        Require(values.Resolve(version,12,output)==PrimitiveConstantStatus::Missing,"missing texture coordinates accepted");
        Require(values.Write(21,texture)==PrimitiveConstantStatus::Ready && values.Resolve(version,12,output)==PrimitiveConstantStatus::Ready,"current complete values");
        std::vector<SceneConstantBinding> packed;Require(PackPrimitiveConstants(single,output,packed)==PrimitiveConstantStatus::Ready,"current packed values");
        Require(packed.size()==1 && packed[0].stage==SceneShaderStage::Vertex && packed[0].bytes.size()==64 &&
            !std::memcmp(packed[0].bytes.data(),matrix.data(),32) && !std::memcmp(packed[0].bytes.data()+32,texture.data(),32),"register mapping differs from explicit expected bytes");
        std::array<float,16> current_values;std::copy(matrix.begin(),matrix.end(),current_values.begin());
        std::copy(texture.begin(),texture.end(),current_values.begin()+8);
        std::array<uint8_t,32> cpu_node{};uintptr_t node=reinterpret_cast<uintptr_t>(cpu_node.data()),header=reinterpret_cast<uintptr_t>(&node);
        const std::array<uintptr_t,2> mesh_nodes{header,1};
        bootstrap::UiCpuPrepareInputs prepare{1,reinterpret_cast<uintptr_t>(current_values.data()),1,1,reinterpret_cast<uintptr_t>(mesh_nodes.data()),0,0,1};
        bootstrap::UiCpuPreparedShader prepared;
        Require(bootstrap::PrepareNativeUiCpu(CpuPrepareControl,bank,version,3,12,71,prepare,prepared,CpuFailure)==PrimitiveConstantStatus::Ready &&
            prepared.serial==12 && prepared.draw_identity==71 && prepared.uniform_writes==2 && prepared.flags==8 &&
            prepared.constants.size()==1 && prepared.constants[0].bytes==packed[0].bytes && prepare.flags==0,"owned CPU prepare ABI/current bytes/caller flags");
        current_values.fill(0);Require(prepared.constants[0].bytes==packed[0].bytes,"borrowed CPU values retained");
        prepare.flags=1;Require(bootstrap::PrepareNativeUiCpu(CpuPrepareControl,bank,version,3,13,72,prepare,prepared,CpuFailure)==PrimitiveConstantStatus::Missing &&
            prepared.serial==12,"incomplete CPU prepare replaced complete output");
        cpu_node[15]=0x10;Require(bootstrap::PrepareNativeUiCpu(CpuPrepareControl,bank,version,3,13,72,prepare,prepared,CpuFailure)==PrimitiveConstantStatus::Unsupported,
            "unqualified3D view dispatched");cpu_node[15]=0;
        Require(bootstrap::PrepareNativeUiCpu(CpuPrepareControl,bank,{51,8},3,13,72,prepare,prepared,CpuFailure)==PrimitiveConstantStatus::Stale,"stale CPU prepare bank admitted");
        Require(values.Write(13,matrix,1)==PrimitiveConstantStatus::Invalid && values.Write(25,matrix)==PrimitiveConstantStatus::Invalid,"overflow/unknown semantic accepted");
        Require(values.Resolve({51,8},12,output)==PrimitiveConstantStatus::Stale && values.Resolve(version,13,output)==PrimitiveConstantStatus::Stale,"stale source accepted");
        Require(values.Begin(single,13,72)==PrimitiveConstantStatus::Ready && values.Resolve(version,13,output)==PrimitiveConstantStatus::Missing,"old frame retained");
        Require(values.Begin(batch,14,73)==PrimitiveConstantStatus::Ready,"batch array matches actual RDEF");
        std::array<float,384> expected{};
        for(uint32_t mesh=0;mesh<24;++mesh){
            std::array<float,8> m{},t{};for(size_t k=0;k<8;++k){m[k]=float(mesh*16+k+1);t[k]=float(mesh*16+k+9);}
            Require(values.Write(13,m,0,mesh)==PrimitiveConstantStatus::Ready && values.Write(21,t,0,mesh)==PrimitiveConstantStatus::Ready,"actual semantic remap/instance stride");
            std::copy(m.begin(),m.end(),expected.begin()+mesh*16);std::copy(t.begin(),t.end(),expected.begin()+mesh*16+8);
        }
        Require(values.Resolve(version,14,output)==PrimitiveConstantStatus::Ready && PackPrimitiveConstants(batch,output,packed)==PrimitiveConstantStatus::Ready,"complete current batch array");
        Require(packed.size()==1 && packed[0].bytes.size()==1536 && !std::memcmp(packed[0].bytes.data(),expected.data(),1536),"batched CPU semantic values differ from explicit interleaved mesh values");
        Require(values.Write(13,matrix,0,24)==PrimitiveConstantStatus::Invalid,"out of range mesh accepted");
        auto absent=single;absent.vertex_uniforms.installed=false;Require(values.Begin(absent,15,74)==PrimitiveConstantStatus::Missing,"unqualified layout accepted");
        std::puts("PASS installed UI uniforms: owned CPU metadata/RDEF agreement, current matrix/texture values,24-mesh semantic remap/stride, exact native bytes, missing/stale/overflow/old-frame refusal");return 0;
    }catch(const std::exception& error){std::fprintf(stderr,"FAIL: %s\n",error.what());return 1;}
}
