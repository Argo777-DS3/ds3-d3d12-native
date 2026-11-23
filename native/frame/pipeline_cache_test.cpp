// I-0072 controls: the device's pipeline cache (PipelineStore) on a real device with the debug layer. One root object per root content (I-0132);
// a description compiled once (memory hits after, one build for many threads asking at once); the precompile list written,
// read back and compiled on workers (every listed pipeline then found in memory, none built on the frames' path); a damaged
// list gives an empty, rebuilt cache; a root signature not made by the cache is created
// directly; an unused record ages out. No driver library (I-0113 gap 4). Synthetic programmes compiled here.
#include "../renderer/backend.h"
#include "pipeline_cache_depth.dxil.h"    // dxc (Build-GameBootstrap.ps1): pipeline_cache_depth_dxil[], SV_Depth
#include "pipeline_cache_colour.dxil.h"   // pipeline_cache_colour_dxil[], colour only
#include "pipeline_cache_red.dxil.h"      // pipeline_cache_red_dxil[] and pipeline_cache_green_dxil[]: two blobs embedding one root
#include "pipeline_cache_green.dxil.h"
#include <d3dcompiler.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace ds3native;
using Microsoft::WRL::ComPtr;
namespace {
void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void Check(HRESULT result,const char* message){if(FAILED(result))throw std::runtime_error(message);}
ComPtr<ID3DBlob> Compile(const char* source,const char* target){
    ComPtr<ID3DBlob> code,errors;
    Check(D3DCompile(source,std::strlen(source),nullptr,nullptr,nullptr,"main",target,0,0,&code,&errors),target);return code;
}
// The stored messages from `from` on at the warning level or worse (the module counts a warning a fault), printed.
uint64_t WarningsSince(DeviceQueues& gpu,UINT64 from){
    if(!gpu.diagnostics)return 0;uint64_t count{};
    for(UINT64 index=from;index<gpu.diagnostics->GetNumStoredMessages();++index){SIZE_T size{};gpu.diagnostics->GetMessage(index,nullptr,&size);
        std::vector<uint8_t> bytes(size);auto* message=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());
        if(SUCCEEDED(gpu.diagnostics->GetMessage(index,message,&size)) && message->Severity<=D3D12_MESSAGE_SEVERITY_WARNING &&
           !AgilityCapabilityNotice(message->Severity,message->pDescription)){++count;std::fprintf(stderr,"%s\n",message->pDescription);}}
    return count;
}
// backend.cpp's list checksum (its HashBytes' first lane), to edit a list as an older build wrote it.
uint64_t ListChecksum(const uint8_t* p,size_t size){
    constexpr uint64_t seed=0x5053432D44533343ULL;uint64_t high=0xcbf29ce484222325ULL^seed;
    for(size_t i=0;i<size;++i){high^=p[i];high*=1099511628211ULL;}
    high^=size;high*=1099511628211ULL;return high;
}
// backend.cpp's HashBytes (both lanes), to write a list as list version 3 named its roots (by the blob's bytes).
PipelineStore::Name HashBytes(uint64_t seed,const void* bytes,size_t size){
    uint64_t high=0xcbf29ce484222325ULL^seed,low=0x84222325cbf29ce4ULL^(seed*0x100000001B3ULL);const auto* p=static_cast<const uint8_t*>(bytes);
    for(size_t i=0;i<size;++i){high^=p[i];high*=1099511628211ULL;low^=p[i];low*=0x100000001B3ULL;low^=high>>29;}
    high^=size;high*=1099511628211ULL;low^=size;low*=0x100000001B3ULL;return {high,low};
}
// A list's sections (backend.cpp's Save): header, roots and programmes (name, size, bytes), records (age, size, encoding), checksum.
struct ListParts {uint32_t version{};std::vector<std::pair<PipelineStore::Name,std::vector<uint8_t>>> roots,programmes;std::vector<std::pair<uint8_t,std::vector<uint8_t>>> records;};
ListParts ParseList(const std::vector<uint8_t>& b){
    ListParts l;size_t at=0;Require(b.size()>=16,"list size");
    const auto take=[&](void* to,size_t n){Require(at+n<=b.size()-8,"list layout");if(n)std::memcpy(to,b.data()+at,n);at+=n;};
    uint32_t magic{};take(&magic,4);take(&l.version,4);Require(magic==0x43535044,"list magic");
    for(auto* section:{&l.roots,&l.programmes}){uint32_t n{};take(&n,4);
        for(uint32_t i=0;i<n;++i){PipelineStore::Name name{};take(&name.high,8);take(&name.low,8);uint32_t size{};take(&size,4);std::vector<uint8_t> bytes(size);take(bytes.data(),size);section->emplace_back(name,std::move(bytes));}}
    uint32_t n{};take(&n,4);
    for(uint32_t i=0;i<n;++i){uint8_t idle{};take(&idle,1);uint32_t size{};take(&size,4);std::vector<uint8_t> encoding(size);take(encoding.data(),size);l.records.emplace_back(idle,std::move(encoding));}
    Require(at==b.size()-8,"list parsed whole");return l;
}
std::vector<uint8_t> WriteList(const ListParts& l){
    std::vector<uint8_t> b;const auto put=[&](const void* p,size_t n){const auto* c=static_cast<const uint8_t*>(p);b.insert(b.end(),c,c+n);};
    const uint32_t magic=0x43535044;put(&magic,4);put(&l.version,4);
    for(const auto* section:{&l.roots,&l.programmes}){const auto n=static_cast<uint32_t>(section->size());put(&n,4);
        for(const auto& [name,bytes]:*section){put(&name.high,8);put(&name.low,8);const auto size=static_cast<uint32_t>(bytes.size());put(&size,4);put(bytes.data(),bytes.size());}}
    const auto n=static_cast<uint32_t>(l.records.size());put(&n,4);
    for(const auto& [idle,encoding]:l.records){put(&idle,1);const auto size=static_cast<uint32_t>(encoding.size());put(&size,4);put(encoding.data(),encoding.size());}
    const uint64_t sum=ListChecksum(b.data(),b.size());put(&sum,8);return b;
}
uint64_t DebugErrors(DeviceQueues& gpu){
    if(!gpu.diagnostics)return 0;uint64_t errors{};
    for(UINT64 index=0;index<gpu.diagnostics->GetNumStoredMessages();++index){SIZE_T size{};gpu.diagnostics->GetMessage(index,nullptr,&size);
        std::vector<uint8_t> bytes(size);auto* message=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());
        if(SUCCEEDED(gpu.diagnostics->GetMessage(index,message,&size)) &&
           (message->Severity==D3D12_MESSAGE_SEVERITY_ERROR || message->Severity==D3D12_MESSAGE_SEVERITY_CORRUPTION)){++errors;std::fprintf(stderr,"%s\n",message->pDescription);}}
    return errors;
}
void WaitPrecompile(PipelineStore& store){
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(60);
    while(!store.Stats().precompile_done){Require(std::chrono::steady_clock::now()<deadline,"precompile did not finish");std::this_thread::sleep_for(std::chrono::milliseconds(5));}
}
std::vector<uint8_t> ReadFile(const std::wstring& path){std::vector<uint8_t> bytes;FILE* f{};if(_wfopen_s(&f,path.c_str(),L"rb") || !f)return bytes;
    std::fseek(f,0,SEEK_END);bytes.resize(static_cast<size_t>(std::ftell(f)));std::fseek(f,0,SEEK_SET);
    if(std::fread(bytes.data(),1,bytes.size(),f)!=bytes.size())bytes.clear();std::fclose(f);return bytes;}
void WriteFile(const std::wstring& path,const std::vector<uint8_t>& bytes){FILE* f{};Require(!_wfopen_s(&f,path.c_str(),L"wb") && f,"test file write");
    std::fwrite(bytes.data(),1,bytes.size(),f);std::fclose(f);}
}

int main(int argc,char** argv){
    try{
        Require(argc==2,"warp or hardware required");const bool warp=!std::strcmp(argv[1],"warp");Require(warp || !std::strcmp(argv[1],"hardware"),"unknown profile");
        // I-0132 R11i (the step review's M2 a): the hardware profile on the Agility SDK 1.620's runtime and debug layer (the build's
        // D3D12 folder beside the test, as beside the module), its refusals checked as warnings below; WARP keeps the inbox runtime.
        if(!warp){wchar_t path[MAX_PATH]{};const DWORD n=GetModuleFileNameW(nullptr,path,MAX_PATH);std::wstring folder(path,n);
            folder.resize(folder.find_last_of(L"\\/")+1);folder+=L"D3D12\\";
            if(GetFileAttributesW((folder+L"D3D12Core.dll").c_str())!=INVALID_FILE_ATTRIBUTES)agility_sdk_folder=folder;}
        DeviceQueues gpu;gpu.Open(warp,true);auto* device=gpu.device.Get();
        if(!agility_sdk_folder.empty())Require(gpu.Capabilities().sdk_version==kAgilitySdkVersion && gpu.AgilityFailure().empty(),"the Agility SDK's runtime asked, not opened");
        std::printf("runtime: sdk %u%s\n",gpu.Capabilities().sdk_version,agility_sdk_folder.empty()?" (inbox)":" (Agility SDK)");
        const auto vs=Compile("float4 main(float4 p:POSITION,float2 t:TEXCOORD0):SV_Position{return p+float4(t,0,0);}","vs_5_0");
        const auto ps=Compile("float4 main():SV_Target{return float4(1,0,0,1);}","ps_5_0");
        const auto ps2=Compile("float4 main():SV_Target{return float4(0,1,0,1);}","ps_5_0");
        const auto cs=Compile("RWBuffer<uint> b:register(u0);[numthreads(8,1,1)]void main(uint3 i:SV_DispatchThreadID){b[i.x]=i.x;}","cs_5_0");
        D3D12_ROOT_SIGNATURE_DESC root_desc{0,nullptr,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT};
        ComPtr<ID3DBlob> root_blob,error;Check(D3D12SerializeRootSignature(&root_desc,D3D_ROOT_SIGNATURE_VERSION_1,&root_blob,&error),"root serialize");
        D3D12_DESCRIPTOR_RANGE range{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,0};D3D12_ROOT_PARAMETER table{};table.ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;table.DescriptorTable={1,&range};
        D3D12_ROOT_SIGNATURE_DESC compute_desc{1,&table,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
        ComPtr<ID3DBlob> compute_blob;Check(D3D12SerializeRootSignature(&compute_desc,D3D_ROOT_SIGNATURE_VERSION_1,&compute_blob,&error),"compute root serialize");
        wchar_t temp[MAX_PATH]{};GetTempPathW(MAX_PATH,temp);
        // Named by the process (I-0105: two builds running their tests at once deleted each other's files: « T1 saved once » failed).
        const std::wstring list=std::wstring(temp)+L"ds3-pipeline-cache-test-"+(warp?L"warp-":L"hardware-")+std::to_wstring(GetCurrentProcessId())+L".psc";
        DeleteFileW(list.c_str());
        const D3D12_INPUT_ELEMENT_DESC elements[]{{"POSITION",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
            {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,16,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}};
        const auto describe=[&](ID3D12RootSignature* root,ID3DBlob* pixel,D3D12_CULL_MODE cull){D3D12_GRAPHICS_PIPELINE_STATE_DESC d{};d.pRootSignature=root;
            d.VS={vs->GetBufferPointer(),vs->GetBufferSize()};d.PS={pixel->GetBufferPointer(),pixel->GetBufferSize()};d.InputLayout={elements,2};
            d.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;d.RasterizerState.CullMode=cull;d.RasterizerState.DepthClipEnable=TRUE;
            for(auto& t:d.BlendState.RenderTarget){t.SrcBlend=t.SrcBlendAlpha=D3D12_BLEND_ONE;t.DestBlend=t.DestBlendAlpha=D3D12_BLEND_ZERO;t.BlendOp=t.BlendOpAlpha=D3D12_BLEND_OP_ADD;t.LogicOp=D3D12_LOGIC_OP_NOOP;t.RenderTargetWriteMask=15;}
            d.DepthStencilState.DepthFunc=D3D12_COMPARISON_FUNC_LESS;
            for(auto* face:{&d.DepthStencilState.FrontFace,&d.DepthStencilState.BackFace}){face->StencilFailOp=face->StencilDepthFailOp=face->StencilPassOp=D3D12_STENCIL_OP_KEEP;face->StencilFunc=D3D12_COMPARISON_FUNC_ALWAYS;}
            d.SampleMask=UINT_MAX;d.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;d.NumRenderTargets=1;d.RTVFormats[0]=DXGI_FORMAT_R8G8B8A8_UNORM;d.SampleDesc.Count=1;return d;};

        // 1. A cold start: no files; the descriptions compiled once each, then found in memory.
        ComPtr<ID3D12PipelineState> first,second,third,compute_pipeline;
        {PipelineStore store;store.Open(device,list);auto stats=store.Stats();
            Require(stats.open && !stats.list_loaded && !stats.records,"T1 cold open");
            ComPtr<ID3D12RootSignature> root,again,compute_root;
            Check(store.Root(device,root_blob->GetBufferPointer(),root_blob->GetBufferSize(),&root),"T1 root");
            Check(store.Root(device,root_blob->GetBufferPointer(),root_blob->GetBufferSize(),&again),"T1 root again");
            Check(store.Root(device,compute_blob->GetBufferPointer(),compute_blob->GetBufferSize(),&compute_root),"T1 compute root");
            Require(root.Get()==again.Get() && root.Get()!=compute_root.Get(),"T1 one root object per blob");
            bool created{};Check(store.Graphics(device,describe(root.Get(),ps.Get(),D3D12_CULL_MODE_BACK),&first,&created),"T1 graphics");Require(created,"T1 first compiled");
            ComPtr<ID3D12PipelineState> hit;Check(store.Graphics(device,describe(root.Get(),ps.Get(),D3D12_CULL_MODE_BACK),&hit,&created),"T1 graphics again");
            Require(!created && hit.Get()==first.Get(),"T1 the same description found in memory");
            Check(store.Graphics(device,describe(root.Get(),ps.Get(),D3D12_CULL_MODE_NONE),&second,&created),"T1 cull variant");Require(created && second.Get()!=first.Get(),"T1 a field of the description names a new pipeline");
            // Eight threads asking a new description at once: one build, one object.
            std::vector<ComPtr<ID3D12PipelineState>> raced(8);std::vector<std::thread> threads;const auto before=store.Stats().created;
            for(auto& slot:raced)threads.emplace_back([&,target=slot.GetAddressOf()]{Check(store.Graphics(device,describe(root.Get(),ps2.Get(),D3D12_CULL_MODE_BACK),target),"T1 race");});
            for(auto& thread:threads)thread.join();
            Require(store.Stats().created==before+1,"T1 a description asked at once is built once");
            for(const auto& slot:raced)Require(slot.Get()==raced[0].Get(),"T1 the racing threads share one object");third=raced[0];
            D3D12_COMPUTE_PIPELINE_STATE_DESC c{};c.pRootSignature=compute_root.Get();c.CS={cs->GetBufferPointer(),cs->GetBufferSize()};
            Check(store.Compute(device,c,&compute_pipeline,&created),"T1 compute");Require(created,"T1 compute compiled");
            // A root the cache did not make: created directly, not listed.
            // (another blob than the cache's roots: the runtime may return one object for identical blobs, review L9)
            D3D12_ROOT_SIGNATURE_DESC foreign_desc{0,nullptr,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT|D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS};
            ComPtr<ID3DBlob> foreign_blob;Check(D3D12SerializeRootSignature(&foreign_desc,D3D_ROOT_SIGNATURE_VERSION_1,&foreign_blob,&error),"T1 foreign serialize");
            ComPtr<ID3D12RootSignature> foreign;Check(device->CreateRootSignature(0,foreign_blob->GetBufferPointer(),foreign_blob->GetBufferSize(),IID_PPV_ARGS(&foreign)),"T1 foreign root");
            {ComPtr<ID3D12PipelineState> direct;Check(store.Graphics(device,describe(foreign.Get(),ps.Get(),D3D12_CULL_MODE_FRONT),&direct,&created),"T1 direct");
                Require(created && store.Stats().direct==1,"T1 a foreign root is created directly");}
            stats=store.Stats();Require(stats.records==4 && stats.created==4 && stats.memory_hits==8 && stats.frame_created==5,"T1 counters (one hit, seven racing threads)");
            Require(store.Save() && !store.Save(),"T1 saved once");store.Close();}
        Require(!ReadFile(list).empty(),"T2 list written");

        // 2. A warm start: the list read, every record compiled by the precompile on two workers; the frames' requests then hit
        // memory (nothing built on their path).
        {PipelineStore store;store.Open(device,list);auto stats=store.Stats();
            Require(stats.list_loaded && stats.records==4,"T2 warm open");
            store.Precompile(2);WaitPrecompile(store);stats=store.Stats();
            Require(stats.precompile_total==4 && stats.precompiled==4 && stats.created==4 && !stats.precompile_failed,"T2 precompiled from the list");
            ComPtr<ID3D12RootSignature> root;Check(store.Root(device,root_blob->GetBufferPointer(),root_blob->GetBufferSize(),&root),"T2 root");
            bool created{true};ComPtr<ID3D12PipelineState> hit;Check(store.Graphics(device,describe(root.Get(),ps.Get(),D3D12_CULL_MODE_BACK),&hit,&created),"T2 graphics");
            stats=store.Stats();Require(!created && stats.memory_hits==1 && stats.created==4,"T2 the frame's request found in memory");
            Require(!stats.frame_created,"T2 nothing built by the frame's requests");
            Require(store.Save() && !store.Save(),"T2 the records' ages written once");store.Close();
            // Close then Open again on the same store: a second start.
            store.Open(device,list);store.Precompile(3);WaitPrecompile(store);stats=store.Stats();
            Require(stats.list_loaded && stats.precompiled==4 && stats.created==4,"T2 reopened and precompiled again");store.Close();}

        // 3. A damaged list: discarded; the start is cold and rebuilds it.
        {auto bytes=ReadFile(list);bytes[bytes.size()/2]^=0x5A;WriteFile(list,bytes);
            PipelineStore store;store.Open(device,list);const auto stats=store.Stats();
            Require(stats.open && !stats.list_loaded && !stats.records,"T3 damaged list refused");
            ComPtr<ID3D12RootSignature> root;Check(store.Root(device,root_blob->GetBufferPointer(),root_blob->GetBufferSize(),&root),"T3 root");
            bool created{};ComPtr<ID3D12PipelineState> again;Check(store.Graphics(device,describe(root.Get(),ps.Get(),D3D12_CULL_MODE_BACK),&again,&created),"T3 graphics");
            Require(created && store.Save(),"T3 rebuilt and saved");store.Close();}

        // 4. A record no request asks for ages out after eight starts (each start writes the ages; the precompile does not count).
        {uint64_t pruned{},records{};for(int start=0;start<9;++start){PipelineStore store;store.Open(device,list);const auto stats=store.Stats();pruned+=stats.pruned;records=stats.records;
                store.Precompile(1);WaitPrecompile(store);store.Close();}
            Require(pruned==1 && records==0,"T4 an unused record aged out");}
        DeleteFileW(list.c_str());

        // 5. I-0132 R11i: a depth-stencil format the description does not use (the tests off, no SV_Depth written) is dropped, so
        // the description with it and the one without name one pipeline; a description that uses depth, or a programme writing
        // SV_Depth, keeps its format. The output signature read from the container (an unreadable blob keeps the format).
        {const auto ps_depth=Compile("struct O{float4 c:SV_Target;float d:SV_Depth;};O main(){O o;o.c=float4(1,1,1,1);o.d=0.5;return o;}","ps_5_0");
            const auto ps_greater=Compile("struct O{float4 c:SV_Target;float d:SV_DepthGreaterEqual;};O main(){O o;o.c=float4(0,0,0,1);o.d=0.5;return o;}","ps_5_0");
            Require(!PixelShaderWritesDepth({ps->GetBufferPointer(),ps->GetBufferSize()}),"T5 a colour programme writes no depth");
            Require(PixelShaderWritesDepth({ps_depth->GetBufferPointer(),ps_depth->GetBufferSize()}),"T5 SV_Depth read from the output signature");
            Require(PixelShaderWritesDepth({ps_greater->GetBufferPointer(),ps_greater->GetBufferSize()}),"T5 SV_DepthGreaterEqual read");
            Require(!PixelShaderWritesDepth({}),"T5 no pixel shader writes no depth");
            {std::vector<uint8_t> junk(64,0x41);Require(PixelShaderWritesDepth({junk.data(),junk.size()}),"T5 an unreadable container keeps the format");
                auto cut=std::vector<uint8_t>(static_cast<const uint8_t*>(ps->GetBufferPointer()),static_cast<const uint8_t*>(ps->GetBufferPointer())+40);
                Require(PixelShaderWritesDepth({cut.data(),cut.size()}),"T5 a truncated container keeps the format");}
            // The step review's M2: dxc's DXIL containers (ps_6_6, reflection stripped, as the module's own: OSG1).
            Require(PixelShaderWritesDepth({pipeline_cache_depth_dxil,sizeof pipeline_cache_depth_dxil}),"T5 SV_Depth read from a DXIL container");
            Require(!PixelShaderWritesDepth({pipeline_cache_colour_dxil,sizeof pipeline_cache_colour_dxil}),"T5 a DXIL colour programme writes no depth");
            PipelineStore store;ComPtr<ID3D12RootSignature> root;Check(store.Root(device,root_blob->GetBufferPointer(),root_blob->GetBufferSize(),&root),"T5 root");
            auto without=describe(root.Get(),ps.Get(),D3D12_CULL_MODE_BACK);auto with=without;with.DSVFormat=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
            Require(UnusedDepthStencilFormat(with) && !UnusedDepthStencilFormat(without),"T5 the unused format found");
            ComPtr<ID3D12PipelineState> a,b,c,d;bool created{};const UINT64 from=gpu.diagnostics?gpu.diagnostics->GetNumStoredMessages():0;
            Check(store.Graphics(device,with,&a,&created),"T5 with the format");Require(created,"T5 compiled");
            Require(!WarningsSince(gpu,from),"T5 the description with an unused format warned (the 1.620 layer's refusal)");
            Check(store.Graphics(device,without,&b,&created),"T5 without the format");Require(!created && a.Get()==b.Get(),"T5 one pipeline with and without an unused format");
            auto tested=with;tested.DepthStencilState.DepthEnable=TRUE;tested.DepthStencilState.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ALL;
            Require(!UnusedDepthStencilFormat(tested),"T5 a depth test keeps the format");
            Check(store.Graphics(device,tested,&c,&created),"T5 depth tested");Require(created && c.Get()!=a.Get(),"T5 a depth-tested description its own pipeline");
            auto stencil=with;stencil.DepthStencilState.StencilEnable=TRUE;stencil.DepthStencilState.StencilReadMask=stencil.DepthStencilState.StencilWriteMask=0xFF;
            Require(!UnusedDepthStencilFormat(stencil),"T5 a stencil test keeps the format");
            auto written=with;written.PS={ps_depth->GetBufferPointer(),ps_depth->GetBufferSize()};
            Require(!UnusedDepthStencilFormat(written),"T5 SV_Depth keeps the format");
            Check(store.Graphics(device,written,&d,&created),"T5 depth written");Require(created && d.Get()!=a.Get(),"T5 a depth-writing description its own pipeline");
            store.Close();}

        // 6. The step review's M1: a draw with the dropped format while a depth-stencil view is bound (as the traced passes keep the
        // scene's bound) raises no warning on this runtime.
        {PipelineStore store;ComPtr<ID3D12RootSignature> root;Check(store.Root(device,root_blob->GetBufferPointer(),root_blob->GetBufferSize(),&root),"T6 root");
            const auto strip=Compile("float4 main(uint i:SV_VertexID):SV_Position{return float4((i&1)*4.0-1.0,(i>>1)*4.0-1.0,0,1);}","vs_5_0");
            auto d=describe(root.Get(),ps.Get(),D3D12_CULL_MODE_NONE);d.VS={strip->GetBufferPointer(),strip->GetBufferSize()};d.InputLayout={nullptr,0};
            d.DSVFormat=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;Require(UnusedDepthStencilFormat(d),"T6 the format unused");
            ComPtr<ID3D12PipelineState> pipeline;Check(store.Graphics(device,d,&pipeline),"T6 pipeline");
            D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
            D3D12_RESOURCE_DESC image{};image.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;image.Width=64;image.Height=64;image.DepthOrArraySize=1;image.MipLevels=1;
            image.SampleDesc.Count=1;image.Format=DXGI_FORMAT_R8G8B8A8_UNORM;image.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            ComPtr<ID3D12Resource> colour,depth;Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&image,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,IID_PPV_ARGS(&colour)),"T6 colour");
            image.Format=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;image.Flags=D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            D3D12_CLEAR_VALUE clear{};clear.Format=image.Format;clear.DepthStencil.Depth=1;
            Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&image,D3D12_RESOURCE_STATE_DEPTH_WRITE,&clear,IID_PPV_ARGS(&depth)),"T6 depth");
            ComPtr<ID3D12DescriptorHeap> rtvs,dsvs;D3D12_DESCRIPTOR_HEAP_DESC hd{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,1,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
            Check(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&rtvs)),"T6 rtv heap");hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_DSV;Check(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&dsvs)),"T6 dsv heap");
            const auto rtv=rtvs->GetCPUDescriptorHandleForHeapStart(),dsv=dsvs->GetCPUDescriptorHandleForHeapStart();
            device->CreateRenderTargetView(colour.Get(),nullptr,rtv);device->CreateDepthStencilView(depth.Get(),nullptr,dsv);
            ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> commands;
            Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"T6 allocator");
            Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&commands)),"T6 list");
            const UINT64 from=gpu.diagnostics?gpu.diagnostics->GetNumStoredMessages():0;
            const D3D12_VIEWPORT viewport{0,0,64,64,0,1};const D3D12_RECT scissor{0,0,64,64};
            commands->OMSetRenderTargets(1,&rtv,FALSE,&dsv);commands->RSSetViewports(1,&viewport);commands->RSSetScissorRects(1,&scissor);
            commands->SetGraphicsRootSignature(root.Get());commands->SetPipelineState(pipeline.Get());commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            commands->DrawInstanced(3,1,0,0);Check(commands->Close(),"T6 close");
            gpu.Execute(commands.Get());gpu.Drain();
            Require(!WarningsSince(gpu,from),"T6 a draw with the dropped format and a bound depth-stencil view warned");
            store.Close();}

        // 7. The step review's M2 (c): a list written before the rule (a record whose description names a format it does not use)
        // migrates at the precompile: compiled under its new name, the old record replaced, counted precompiled, not failed; the
        // next start lists the new record alone.
        {DeleteFileW(list.c_str());
            {PipelineStore store;store.Open(device,list);ComPtr<ID3D12RootSignature> root;Check(store.Root(device,root_blob->GetBufferPointer(),root_blob->GetBufferSize(),&root),"T7 root");
                auto tested=describe(root.Get(),ps.Get(),D3D12_CULL_MODE_BACK);tested.DSVFormat=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;tested.DepthStencilState.DepthEnable=TRUE;
                ComPtr<ID3D12PipelineState> pipeline;Check(store.Graphics(device,tested,&pipeline),"T7 the record");Require(store.Save(),"T7 saved");store.Close();}
            // The record's DepthEnable turned off in place: kind, version, root (18 bytes), VS and PS named (24 each), DS, HS, GS empty
            // (8 each), the blend (82 words), the sample mask, the rasterizer (11 words): byte 466.
            auto bytes=ReadFile(list);Require(bytes.size()>16,"T7 list read");
            size_t at=8;const auto u32=[&](size_t o){uint32_t v{};Require(o+4<=bytes.size()-8,"T7 list layout");std::memcpy(&v,bytes.data()+o,4);return v;};
            for(int section=0;section<2;++section){const uint32_t n=u32(at);at+=4;for(uint32_t i=0;i<n;++i){at+=16;const uint32_t size=u32(at);at+=4+size;}}
            Require(u32(at)==1,"T7 one record");at+=4;at+=1;const uint32_t size=u32(at);at+=4;
            constexpr size_t depth_enable=18+24+24+24+82*4+4+11*4;Require(size>depth_enable+4 && u32(at+depth_enable)==1,"T7 the record's DepthEnable found");
            const uint32_t off=0;std::memcpy(bytes.data()+at+depth_enable,&off,4);
            const uint64_t sum=ListChecksum(bytes.data(),bytes.size()-8);std::memcpy(bytes.data()+bytes.size()-8,&sum,8);WriteFile(list,bytes);
            auto unused=describe(nullptr,ps.Get(),D3D12_CULL_MODE_BACK);unused.DSVFormat=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
            for(int start=0;start<2;++start){PipelineStore store;store.Open(device,list);auto stats=store.Stats();Require(stats.list_loaded && stats.records==1,"T7 the list read");
                store.Precompile(1);WaitPrecompile(store);stats=store.Stats();
                Require(stats.precompile_total==1 && stats.precompiled==1 && !stats.precompile_failed && stats.records==1,start?"T7 the migrated record precompiled":"T7 the old record migrated");
                ComPtr<ID3D12RootSignature> root;Check(store.Root(device,root_blob->GetBufferPointer(),root_blob->GetBufferSize(),&root),"T7 root again");unused.pRootSignature=root.Get();
                bool created{true};ComPtr<ID3D12PipelineState> hit;Check(store.Graphics(device,unused,&hit,&created),"T7 request");Require(!created,"T7 the request found the migrated pipeline");
                Require(start || store.Save(),"T7 the migration saved");store.Close();}
            DeleteFileW(list.c_str());}

        // 8. I-0132 (the root aliases): two DXIL programmes embedding one root (an older build's and a newer's) name one root, one
        // object: a pipeline listed with either blob's root is precompiled and then found whichever blob a stage gives, in either
        // order (the store named a root by its blob, so the pipeline's name followed the blob that made the object last); a
        // version-3 list (roots named by their blobs) is renamed by content at open, its two alias records made one, then saved as
        // version 4 with one root, this build's blob.
        {DeleteFileW(list.c_str());
            const D3D12_SHADER_BYTECODE red{pipeline_cache_red_dxil,sizeof pipeline_cache_red_dxil},green{pipeline_cache_green_dxil,sizeof pipeline_cache_green_dxil};
            const auto bytes_of=[](const D3D12_SHADER_BYTECODE& blob){const auto* p=static_cast<const uint8_t*>(blob.pShaderBytecode);return std::vector<uint8_t>(p,p+blob.BytecodeLength);};
            Require(red.BytecodeLength!=green.BytecodeLength || std::memcmp(red.pShaderBytecode,green.pShaderBytecode,red.BytecodeLength),"T8 two different blobs");
            {ComPtr<ID3D12RootSignature> a,b;Check(device->CreateRootSignature(0,red.pShaderBytecode,red.BytecodeLength,IID_PPV_ARGS(&a)),"T8 runtime root a");
                Check(device->CreateRootSignature(0,green.pShaderBytecode,green.BytecodeLength,IID_PPV_ARGS(&b)),"T8 runtime root b");
                std::printf("T8: the runtime's objects for the two blobs %s\n",a.Get()==b.Get()?"one (the aliases)":"two");}
            {PipelineStore store;store.Open(device,list);ComPtr<ID3D12RootSignature> a,b;
                Check(store.Root(device,red.pShaderBytecode,red.BytecodeLength,&a),"T8 root red");Check(store.Root(device,green.pShaderBytecode,green.BytecodeLength,&b),"T8 root green");
                Require(a.Get()==b.Get(),"T8 one root object for one content");
                ComPtr<ID3D12PipelineState> pipeline;Check(store.Graphics(device,describe(b.Get(),ps.Get(),D3D12_CULL_MODE_BACK),&pipeline),"T8 the record");
                Require(!store.Stats().root_aliases && store.Save(),"T8 saved");store.Close();}
            for(int order=0;order<2;++order){PipelineStore store;store.Open(device,list);store.Precompile(1);WaitPrecompile(store);auto stats=store.Stats();
                Require(stats.precompiled==1 && !stats.precompile_failed,"T8 precompiled");
                const auto& x=order?green:red;const auto& y=order?red:green;ComPtr<ID3D12RootSignature> one,two;
                Check(store.Root(device,x.pShaderBytecode,x.BytecodeLength,&one),"T8 first root");Check(store.Root(device,y.pShaderBytecode,y.BytecodeLength,&two),"T8 second root");
                Require(one.Get()==two.Get(),"T8 one object after the precompile");
                for(auto* root:{one.Get(),two.Get()}){bool created{true};ComPtr<ID3D12PipelineState> hit;Check(store.Graphics(device,describe(root,ps.Get(),D3D12_CULL_MODE_BACK),&hit,&created),"T8 request");
                    Require(!created,"T8 the request found the precompiled pipeline, whichever blob gave its root");}
                stats=store.Stats();Require(!stats.frame_created && !stats.root_aliases,"T8 nothing compiled in play");store.Close();}
            // The list as version 3 wrote it after both blobs were seen: each root named by its blob, one record for each; green's root
            // first, so Open keeps green's blob and only Root(red) below makes the saved blob red's (the step review's L1).
            {auto parts=ParseList(ReadFile(list));Require(parts.version==4 && parts.roots.size()==1 && parts.records.size()==1,"T8 the version-4 list");
                const auto named=[&](const D3D12_SHADER_BYTECODE& blob){return HashBytes(0x524F4F545349474EULL,blob.pShaderBytecode,blob.BytecodeLength);};
                const auto record=parts.records[0].second;parts.version=3;parts.roots={{named(green),bytes_of(green)},{named(red),bytes_of(red)}};parts.records.clear();
                for(const auto& root:{named(red),named(green)}){auto encoding=record;std::memcpy(encoding.data()+2,&root.high,8);std::memcpy(encoding.data()+10,&root.low,8);parts.records.emplace_back(uint8_t(0),std::move(encoding));}
                WriteFile(list,WriteList(parts));}
            for(int start=0;start<2;++start){PipelineStore store;store.Open(device,list);auto stats=store.Stats();
                Require(stats.list_loaded && stats.records==1,"T8 the version-3 list read, its alias records one");
                Require(start?(!stats.roots_renamed && !stats.records_merged):(stats.roots_renamed==2 && stats.records_merged==1),"T8 the roots renamed by content once");
                store.Precompile(1);WaitPrecompile(store);stats=store.Stats();Require(stats.precompiled==1 && !stats.precompile_failed,"T8 the renamed record precompiled");
                ComPtr<ID3D12RootSignature> root;Check(store.Root(device,red.pShaderBytecode,red.BytecodeLength,&root),"T8 root after the renaming");
                bool created{true};ComPtr<ID3D12PipelineState> hit;Check(store.Graphics(device,describe(root.Get(),ps.Get(),D3D12_CULL_MODE_BACK),&hit,&created),"T8 request after the renaming");
                Require(!created,"T8 the renamed record found");
                Require(start || store.Save(),"T8 the renamed list saved");store.Close();
                const auto saved=ParseList(ReadFile(list));
                Require(saved.version==4 && saved.roots.size()==1 && saved.records.size()==1 && saved.roots[0].second==bytes_of(red),"T8 saved as version 4, one root, this build's blob");}
            DeleteFileW(list.c_str());}
        Require(!DebugErrors(gpu),"debug layer errors");
        std::printf("PASS pipeline cache %s: roots, memory hits, one build per name, list round trip, precompile, damaged list, ageing, unused depth-stencil format (DXBC and DXIL, a draw, a list migrated), roots named by content (two blobs of one root, a version-3 list renamed)\n",argv[1]);
        return 0;
    }catch(const std::exception& error){std::fprintf(stderr,"FAIL pipeline cache: %s\n",error.what());return 1;}
}
