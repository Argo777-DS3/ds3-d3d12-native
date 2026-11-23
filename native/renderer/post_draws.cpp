#include "post_draws.h"
#include "backend.h"
#include <cstring>
#include <stdexcept>

namespace ds3native {
using Microsoft::WRL::ComPtr;
namespace {
void Check(HRESULT status,const char* what){if(FAILED(status))throw std::runtime_error(what);}
constexpr uint32_t sampler_sets_max=32;
// The original's vertices for every 2D post draw of the glare family (i66-reference-1648 draws 14-44: stride 16, indices 0, 1, 2),
// then (I-0138 tranche 8f) the ghosts' four-vertex strip (i138-t8e-oracle-240 draws 41 and 42: stride 16, indices 0 to 3).
constexpr float vertex_words[28]{-1,1,0,1, -1,-3,0,-1, 3,1,2,1, -1,1,0,1, -1,-1,0,0, 1,1,1,1, 1,-1,1,0};
constexpr UINT list_bytes=12*sizeof(float),strip_bytes=16*sizeof(float);
}
bool PostDraws::Open(ID3D12Device* device,uint32_t tables,std::string& reason){
    Close();
    try{
        if(!device || !tables)throw std::invalid_argument("post draw inputs");
        device_=device;tables_=tables;
        // Root: b0 $Globals (the VS reads its position scale and offsets, the PS its weights), t0..t7 and s0..s7 (one table each).
        D3D12_ROOT_PARAMETER parameters[3]{};
        parameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;parameters[0].ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
        const D3D12_DESCRIPTOR_RANGE textures{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,Inputs,0,0,0},samplers{D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,Inputs,0,0,0};
        parameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[1].DescriptorTable={1,&textures};parameters[1].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        parameters[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[2].DescriptorTable={1,&samplers};parameters[2].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        const D3D12_ROOT_SIGNATURE_DESC root{3,parameters,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT|
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS|D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS|
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS};
        ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&root,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"post draw root serialization");
        Check(Pipelines().Root(device,blob->GetBufferPointer(),blob->GetBufferSize(),root_.ReleaseAndGetAddressOf()),"post draw root");
        D3D12_DESCRIPTOR_HEAP_DESC heap{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,tables*Inputs,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
        Check(CountedHeap(device)->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&srv_)),"post draw SRV heap");
        heap={D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,sampler_sets_max*Inputs,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
        Check(CountedHeap(device)->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&samplers_)),"post draw sampler heap");
        srv_stride_=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        sampler_stride_=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        for(uint32_t t=0;t<tables;++t)for(uint32_t s=0;s<Inputs;++s)SetInput(t,s,nullptr,DXGI_FORMAT_R8G8B8A8_UNORM);
        D3D12_HEAP_PROPERTIES upload{};upload.Type=D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=sizeof(vertex_words);buffer.Height=1;
        buffer.DepthOrArraySize=buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Check(CountedResource(device)->CreateCommittedResource(&upload,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&vertices_)),"post draw vertices");
        void* memory{};const D3D12_RANGE none{};Check(vertices_->Map(0,&none,&memory),"post draw vertices map");
        std::memcpy(memory,vertex_words,sizeof(vertex_words));vertices_->Unmap(0,nullptr);
        return true;
    }catch(const std::exception& e){reason=e.what();Close();return false;}
}
uint32_t PostDraws::Pipeline(const std::vector<uint8_t>& vertex,const std::vector<uint8_t>& pixel,DXGI_FORMAT target_format,uint32_t texcoords,uint8_t write_mask){
    if(!IsOpen() || vertex.empty() || pixel.empty() || target_format==DXGI_FORMAT_UNKNOWN || texcoords<1 || texcoords>4)throw std::logic_error("post draw pipeline inputs");
    for(size_t i=0;i<keys_.size();++i)if(keys_[i].format==target_format && keys_[i].texcoords==texcoords && keys_[i].write_mask==write_mask && keys_[i].vertex==vertex && keys_[i].pixel==pixel)return uint32_t(i);
    // POSITION then TEXCOORD0..n-1, two floats each (stride 8 + 8 n; LS-3: vs_2D_Tex2D3 and vs_2D_Tex2D4 take 3 and 4).
    const D3D12_INPUT_ELEMENT_DESC layout[5]{{"POSITION",0,DXGI_FORMAT_R32G32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
        {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,8,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},{"TEXCOORD",1,DXGI_FORMAT_R32G32_FLOAT,0,16,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
        {"TEXCOORD",2,DXGI_FORMAT_R32G32_FLOAT,0,24,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},{"TEXCOORD",3,DXGI_FORMAT_R32G32_FLOAT,0,32,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}};
    // YEBIS-NATIVE 5.2, render state 0 (as the tone map): opaque, RGBA written; solid, cull none, depth clip off; no depth or stencil.
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};pso.pRootSignature=root_.Get();pso.InputLayout={layout,1u+texcoords};
    pso.VS={vertex.data(),vertex.size()};pso.PS={pixel.data(),pixel.size()};
    auto& target=pso.BlendState.RenderTarget[0];target.LogicOp=D3D12_LOGIC_OP_NOOP;target.RenderTargetWriteMask=write_mask;
    target.SrcBlend=target.SrcBlendAlpha=D3D12_BLEND_ONE;target.DestBlend=target.DestBlendAlpha=D3D12_BLEND_ZERO;target.BlendOp=target.BlendOpAlpha=D3D12_BLEND_OP_ADD;
    pso.SampleMask=UINT_MAX;pso.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;pso.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;
    pso.RasterizerState.FrontCounterClockwise=TRUE;pso.RasterizerState.DepthClipEnable=FALSE;
    pso.DepthStencilState.DepthEnable=FALSE;pso.DepthStencilState.StencilEnable=FALSE;
    pso.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;pso.NumRenderTargets=1;pso.RTVFormats[0]=target_format;pso.SampleDesc.Count=1;
    ComPtr<ID3D12PipelineState> state;Check(Pipelines().Graphics(device_.Get(),pso,state.ReleaseAndGetAddressOf()),"post draw pipeline");
    pipelines_.push_back(state);keys_.push_back({vertex,pixel,target_format,texcoords,write_mask});return uint32_t(pipelines_.size()-1);
}
void PostDraws::RenewTables(){
    if(!srv_)throw std::logic_error("post draw tables before Open");
    ComPtr<ID3D12DescriptorHeap> fresh;const D3D12_DESCRIPTOR_HEAP_DESC heap{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,tables_*Inputs,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
    Check(CountedHeap(device_.Get())->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&fresh)),"post draw SRV heap");
    RetireAfterCurrentRecording(HoldObject(srv_.Get()));srv_=fresh;
    for(uint32_t t=0;t<tables_;++t)for(uint32_t s=0;s<Inputs;++s)SetInput(t,s,nullptr,DXGI_FORMAT_R8G8B8A8_UNORM);
}
void PostDraws::SetInput(uint32_t table,uint32_t slot,ID3D12Resource* resource,DXGI_FORMAT view_format){
    if(!srv_ || table>=tables_ || slot>=Inputs)throw std::logic_error("post draw input slot");
    D3D12_SHADER_RESOURCE_VIEW_DESC view{};view.Format=view_format;view.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
    view.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;view.Texture2D.MipLevels=UINT(-1);   // every level (LS-3: the film grain's mips)
    auto at=srv_->GetCPUDescriptorHandleForHeapStart();at.ptr+=SIZE_T(srv_stride_)*(table*Inputs+slot);
    device_->CreateShaderResourceView(resource,&view,at);
}
uint32_t PostDraws::SamplerTable(const PostSamplers& set){
    if(!samplers_)throw std::logic_error("post draw samplers before Open");
    for(size_t i=0;i<sampler_sets_.size();++i)if(sampler_sets_[i]==set)return uint32_t(i);
    if(sampler_sets_.size()>=sampler_sets_max)throw std::runtime_error("post draw sampler sets");
    const uint32_t index=uint32_t(sampler_sets_.size());
    for(uint32_t s=0;s<Inputs;++s){D3D12_SAMPLER_DESC d{};d.Filter=set[s].filter;d.AddressU=d.AddressV=set[s].address;d.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        d.MaxAnisotropy=1;d.ComparisonFunc=D3D12_COMPARISON_FUNC_NEVER;d.MinLOD=0;d.MaxLOD=set[s].max_lod;   // BorderColor 0: transparent black
        auto at=samplers_->GetCPUDescriptorHandleForHeapStart();at.ptr+=SIZE_T(sampler_stride_)*(index*Inputs+s);device_->CreateSampler(&d,at);}
    sampler_sets_.push_back(set);return index;
}
void PostDraws::Begin(ID3D12GraphicsCommandList* list) const {
    if(!IsOpen() || !list)throw std::logic_error("post draws before Open");
    ID3D12DescriptorHeap* heaps[]{srv_.Get(),samplers_.Get()};list->SetDescriptorHeaps(2,heaps);list->SetGraphicsRootSignature(root_.Get());
}
void PostDraws::Draw(ID3D12GraphicsCommandList* list,uint32_t pipeline,uint32_t table,uint32_t samplers,D3D12_GPU_VIRTUAL_ADDRESS globals,
    D3D12_CPU_DESCRIPTOR_HANDLE target,uint32_t width,uint32_t height,const D3D12_VERTEX_BUFFER_VIEW* vertices,bool strip) const {
    if(pipeline>=pipelines_.size() || table>=tables_ || samplers>=sampler_sets_.size() || !globals || globals%256 || !target.ptr || !width || !height ||
       (keys_[pipeline].texcoords>1)!=(vertices!=nullptr) || (strip && vertices))throw std::logic_error("post draw arguments");
    const D3D12_VERTEX_BUFFER_VIEW own{vertices_->GetGPUVirtualAddress()+(strip?list_bytes:0),strip?strip_bytes:list_bytes,16};list->IASetVertexBuffers(0,1,vertices?vertices:&own);
    list->IASetPrimitiveTopology(strip?D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    list->SetPipelineState(pipelines_[pipeline].Get());list->SetGraphicsRootConstantBufferView(0,globals);
    auto srv=srv_->GetGPUDescriptorHandleForHeapStart();srv.ptr+=UINT64(srv_stride_)*table*Inputs;list->SetGraphicsRootDescriptorTable(1,srv);
    auto set=samplers_->GetGPUDescriptorHandleForHeapStart();set.ptr+=UINT64(sampler_stride_)*samplers*Inputs;list->SetGraphicsRootDescriptorTable(2,set);
    list->OMSetRenderTargets(1,&target,FALSE,nullptr);
    const D3D12_VIEWPORT viewport{0,0,float(width),float(height),0,1};const D3D12_RECT scissor{0,0,LONG(width),LONG(height)};
    list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
    list->DrawInstanced(strip?4:3,1,0,0);
}
void PostDraws::Close() noexcept {
    pipelines_.clear();keys_.clear();sampler_sets_.clear();root_.Reset();srv_.Reset();samplers_.Reset();vertices_.Reset();device_.Reset();tables_=0;
}
void PostDraws::QuarantineToProcessExit() noexcept {
    for(auto& p:pipelines_)(void)p.Detach();(void)root_.Detach();(void)srv_.Detach();(void)samplers_.Detach();(void)vertices_.Detach();(void)device_.Detach();
}
}
