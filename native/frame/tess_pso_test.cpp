// W7 TS-3 T-Q7 (WATER-TESS-WAVE1-READY 6): every `<base>_PN` hull/domain pair of the I-0063 programme cache, with its `_PN`
// vertex programme and the family's pixel programme of that pass, makes a D3D12 pipeline with a patch topology type on the
// given adapter (the original DXBC SM5, unchanged). The root signature is a permissive one (every stage: CBV b0-b13, SRV
// t0-t127, s0-s15); the input layout follows the vertex programme's input signature. Argument: warp|hardware and the
// programme directory (skipped with a message without it).
#include "../content/shader_interface.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

using Microsoft::WRL::ComPtr;
namespace {
void Check(HRESULT result,const char* what){if(FAILED(result))throw std::runtime_error(what);}
std::vector<uint8_t> FileBytes(const std::filesystem::path& path){std::ifstream file(path,std::ios::binary);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());}
std::string Lower(std::string s){for(auto& c:s)c=static_cast<char>(tolower(static_cast<unsigned char>(c)));return s;}
DXGI_FORMAT FormatOf(const ds3content::ShaderSignatureDeclaration& input){
    int count=0;for(int bit=0;bit<4;++bit)if(input.mask>>bit&1)count=bit+1;
    static const DXGI_FORMAT floats[4]{DXGI_FORMAT_R32_FLOAT,DXGI_FORMAT_R32G32_FLOAT,DXGI_FORMAT_R32G32B32_FLOAT,DXGI_FORMAT_R32G32B32A32_FLOAT};
    static const DXGI_FORMAT uints[4]{DXGI_FORMAT_R32_UINT,DXGI_FORMAT_R32G32_UINT,DXGI_FORMAT_R32G32B32_UINT,DXGI_FORMAT_R32G32B32A32_UINT};
    static const DXGI_FORMAT sints[4]{DXGI_FORMAT_R32_SINT,DXGI_FORMAT_R32G32_SINT,DXGI_FORMAT_R32G32B32_SINT,DXGI_FORMAT_R32G32B32A32_SINT};
    if(!count)count=4;
    return input.component_type==1?uints[count-1]:input.component_type==2?sints[count-1]:floats[count-1];}
}
int main(int argc,char** argv){try{
    if(argc!=3){std::printf("usage: tess_pso_test warp|hardware <programme directory>\n");return 1;}
    const bool warp=!std::strcmp(argv[1],"warp");const std::filesystem::path dir=argv[2];
    if(!std::filesystem::exists(dir)){std::printf("PASS tess pso %s: skipped, no programme directory\n",argv[1]);return 0;}
    ComPtr<IDXGIFactory6> factory;Check(CreateDXGIFactory2(0,IID_PPV_ARGS(&factory)),"factory");ComPtr<IDXGIAdapter1> adapter;
    if(warp)Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)),"WARP adapter");
    else Check(factory->EnumAdapterByGpuPreference(0,DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,IID_PPV_ARGS(&adapter)),"hardware adapter");
    ComPtr<ID3D12Device> device;Check(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)),"device");
    // Permissive root: per stage one table (CBV 0-13, SRV 0-127) and one sampler table (0-15).
    const D3D12_DESCRIPTOR_RANGE resources[2]{{D3D12_DESCRIPTOR_RANGE_TYPE_CBV,14,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,128,0,0,14}};
    const D3D12_DESCRIPTOR_RANGE samplers{D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,16,0,0,0};
    D3D12_ROOT_PARAMETER parameters[8]{};const D3D12_SHADER_VISIBILITY stages[4]{D3D12_SHADER_VISIBILITY_VERTEX,D3D12_SHADER_VISIBILITY_PIXEL,D3D12_SHADER_VISIBILITY_HULL,D3D12_SHADER_VISIBILITY_DOMAIN};
    for(int s=0;s<4;++s){parameters[2*s].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[2*s].DescriptorTable={2,resources};parameters[2*s].ShaderVisibility=stages[s];
        parameters[2*s+1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[2*s+1].DescriptorTable={1,&samplers};parameters[2*s+1].ShaderVisibility=stages[s];}
    const D3D12_ROOT_SIGNATURE_DESC root_desc{8,parameters,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT};
    ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&root_desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"root serialization");
    ComPtr<ID3D12RootSignature> root;Check(device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)),"root");
    std::map<std::string,std::filesystem::path> files;for(const auto& e:std::filesystem::directory_iterator(dir))files[Lower(e.path().filename().string())]=e.path();
    size_t pairs{},made{},no_vertex{};std::string failures,outside;
    for(const auto& [name,path]:files){
        if(name.size()<7 || name.compare(name.size()-7,7,"_pn.hpo"))continue;
        const auto base=name.substr(0,name.size()-4);   // <family>_<pass>_pn
        const auto domain=files.find(base+".dpo");if(domain==files.end())continue;
        const auto vertex=files.find(base+".vpo");if(vertex==files.end()){++no_vertex;continue;}
        ++pairs;const auto pixel=files.find(base.substr(0,base.size()-3)+".ppo");   // the pass's ordinary pixel programme
        const auto vs=FileBytes(vertex->second),hs=FileBytes(path),ds=FileBytes(domain->second);std::vector<uint8_t> ps;if(pixel!=files.end())ps=FileBytes(pixel->second);
        ds3content::CpuShaderProgram program;program.stage=ds3content::ShaderStage::Vertex;program.model_major=5;program.bytecode=vs;
        std::vector<D3D12_INPUT_ELEMENT_DESC> layout;std::vector<std::string> semantics;
        try{const auto signature=ds3content::InspectShaderInterface(program);
            for(const auto& input:signature.inputs){if(input.system_value)continue;semantics.push_back(input.semantic);}
            UINT offset=0;size_t i=0;
            for(const auto& input:signature.inputs){if(input.system_value)continue;
                layout.push_back({semantics[i++].c_str(),input.semantic_index,FormatOf(input),0,offset,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0});offset+=16;}}
        catch(const std::exception& e){failures+=" "+base+"(signature: "+e.what()+")";continue;}
        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};pso.pRootSignature=root.Get();pso.VS={vs.data(),vs.size()};pso.HS={hs.data(),hs.size()};pso.DS={ds.data(),ds.size()};
        if(!ps.empty())pso.PS={ps.data(),ps.size()};pso.InputLayout={layout.data(),static_cast<UINT>(layout.size())};
        pso.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;pso.RasterizerState.CullMode=D3D12_CULL_MODE_BACK;pso.RasterizerState.DepthClipEnable=TRUE;
        pso.BlendState.RenderTarget[0].RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;pso.BlendState.RenderTarget[0].SrcBlend=pso.BlendState.RenderTarget[0].SrcBlendAlpha=D3D12_BLEND_ONE;
        pso.BlendState.RenderTarget[0].DestBlend=pso.BlendState.RenderTarget[0].DestBlendAlpha=D3D12_BLEND_ZERO;pso.BlendState.RenderTarget[0].BlendOp=pso.BlendState.RenderTarget[0].BlendOpAlpha=D3D12_BLEND_OP_ADD;
        pso.BlendState.RenderTarget[0].LogicOp=D3D12_LOGIC_OP_NOOP;
        pso.SampleMask=UINT_MAX;pso.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;pso.SampleDesc.Count=1;
        pso.NumRenderTargets=6;for(int t=0;t<6;++t)pso.RTVFormats[t]=DXGI_FORMAT_R16G16B16A16_FLOAT;pso.DSVFormat=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        pso.DepthStencilState.DepthEnable=TRUE;pso.DepthStencilState.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ALL;pso.DepthStencilState.DepthFunc=D3D12_COMPARISON_FUNC_LESS_EQUAL;
        // The tess root's register set: the hull may declare only b0, b5 and b6, the domain only b0, b5, b6 and b8, and neither a
        // texture or sampler (no `_D` programme here: the base is `_PN`).
        try{for(const auto& [stage,bytes]:{std::pair{ds3content::ShaderStage::Hull,&hs},std::pair{ds3content::ShaderStage::Domain,&ds}}){
                ds3content::CpuShaderProgram tess_program;tess_program.stage=stage;tess_program.model_major=5;tess_program.bytecode=*bytes;
                const auto declared=ds3content::InspectShaderInterface(tess_program);
                for(const auto& resource:declared.resources){const bool constant=resource.input_type==0;const auto slot=resource.shader_register;
                    const bool allowed=constant && (slot==0 || slot==5 || slot==6 || (stage==ds3content::ShaderStage::Domain && slot==8));
                    if(!allowed){outside+=" "+base+(stage==ds3content::ShaderStage::Hull?":hs":":ds")+(constant?"b":"t/s")+std::to_string(slot);}}}}
        catch(const std::exception& e){outside+=" "+base+"(interface: "+e.what()+")";}
        ComPtr<ID3D12PipelineState> pipeline;
        if(SUCCEEDED(device->CreateGraphicsPipelineState(&pso,IID_PPV_ARGS(&pipeline))))++made;else if(failures.size()<2000)failures+=" "+base;
    }
    std::printf("tess pso: %zu pairs with a vertex programme (%zu without), %zu pipelines made; registers outside the tess root:%s\n",pairs,no_vertex,made,outside.empty()?" none":outside.substr(0,1500).c_str());
    if(!pairs || made!=pairs || !outside.empty()){std::printf("FAIL tess pso %s:%s\n",argv[1],failures.c_str());return 1;}
    std::printf("PASS tess pso %s: T-Q7 every _PN hull/domain pair of the cache makes a patch pipeline with its vertex and pixel programmes (%zu)\n",argv[1],made);
    return 0;
}catch(const std::exception& e){std::printf("FAIL %s\n",e.what());return 1;}}
