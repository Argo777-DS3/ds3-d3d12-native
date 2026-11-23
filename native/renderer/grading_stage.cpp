#include "grading_stage.h"
#include "backend.h"
#include <cstring>
#include <stdexcept>

namespace ds3native {
using Microsoft::WRL::ComPtr;
namespace {
void Check(HRESULT status,const char* what){if(FAILED(status))throw std::runtime_error(what);}
// The original's three vertices (Reference1696 draw 50: stride 16, POSITION then TEXCOORD0, indices 0, 1, 2).
constexpr float vertex_words[12]{-1,1,0,1, -1,-3,0,-1, 3,1,2,1};
// s0 and s1 as the original binds them for the draw (Reference1696, PSGetSamplers: filter 20, address 3 and 3, MaxLOD 0).
D3D12_STATIC_SAMPLER_DESC Sampler(uint32_t reg){
    D3D12_STATIC_SAMPLER_DESC s{};s.Filter=D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;s.AddressU=s.AddressV=s.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    s.MaxAnisotropy=1;s.ComparisonFunc=D3D12_COMPARISON_FUNC_NEVER;s.BorderColor=D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    s.MinLOD=0;s.MaxLOD=0;s.ShaderRegister=reg;s.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;return s;}
D3D12_RESOURCE_BARRIER Transition(ID3D12Resource* resource,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after){
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;barrier.Transition={resource,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};return barrier;}
}
void GradingStage::Globals(uint8_t* out){
    std::memset(out,0,GlobalsBytes);
    const auto put=[&](uint32_t at,float a,float b,float c,float d){const float v[4]{a,b,c,d};std::memcpy(out+at,v,sizeof(v));};
    const float n=float(LutEdge);
    const float screen[2]{1.0f,-1.0f};std::memcpy(out+80,screen,sizeof(screen));
    put(2464,1.0f,1.0f,1.0f,1.0f);
    const float scale=(n-1.0f)/n,offset=1.0f/(2.0f*n);
    put(2976,scale,scale,scale,0.0f);put(2992,offset,offset,offset,0.5f);
}
bool GradingStage::Open(ID3D12Device* device,const GradingBlobs& blobs,DXGI_FORMAT target_format,uint32_t width,uint32_t height,std::string& reason){
    Close();
    try{
        if(!device || blobs.vertex.empty() || blobs.pixel.empty() || blobs.lut_copy.empty() || target_format==DXGI_FORMAT_UNKNOWN || !width || !height)
            throw std::invalid_argument("grading stage inputs");
        device_=device;format_=target_format;width_=width;height_=height;
        // Graphics root: b0 $Globals (the VS its position scale and texture offset, the PS its weights), one table t0 and t16, s0 and s1 static.
        D3D12_ROOT_PARAMETER parameters[2]{};
        parameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;parameters[0].ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
        const D3D12_DESCRIPTOR_RANGE textures[2]{{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,16,0,1}};
        parameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[1].DescriptorTable={2,textures};parameters[1].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        const D3D12_STATIC_SAMPLER_DESC samplers[2]{Sampler(0),Sampler(1)};
        const D3D12_ROOT_SIGNATURE_DESC root{2,parameters,2,samplers,D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT|
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS|D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS|
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS};
        ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&root,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"grading root serialization");
        Check(Pipelines().Root(device,blob->GetBufferPointer(),blob->GetBufferSize(),root_.ReleaseAndGetAddressOf()),"grading root");
        // Compute root (LutCopy2DTo3DCs): one table, t0 the 2D strip and u0 the volume.
        const D3D12_DESCRIPTOR_RANGE copy[2]{{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,1}};
        D3D12_ROOT_PARAMETER compute{};compute.ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;compute.DescriptorTable={2,copy};compute.ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
        const D3D12_ROOT_SIGNATURE_DESC compute_root{1,&compute,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
        blob.Reset();error.Reset();Check(D3D12SerializeRootSignature(&compute_root,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"grading compute root serialization");
        Check(Pipelines().Root(device,blob->GetBufferPointer(),blob->GetBufferSize(),compute_root_.ReleaseAndGetAddressOf()),"grading compute root");
        const D3D12_INPUT_ELEMENT_DESC layout[2]{{"POSITION",0,DXGI_FORMAT_R32G32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
            {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,8,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}};
        // YEBIS-NATIVE 5.2, render state 0 (as the tone map): opaque (the captured blend is off), RGBA written; solid, cull none,
        // depth clip off; no depth or stencil.
        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};pso.pRootSignature=root_.Get();pso.InputLayout={layout,2};
        pso.VS={blobs.vertex.data(),blobs.vertex.size()};pso.PS={blobs.pixel.data(),blobs.pixel.size()};
        auto& target=pso.BlendState.RenderTarget[0];target.LogicOp=D3D12_LOGIC_OP_NOOP;target.RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;
        target.SrcBlend=target.SrcBlendAlpha=D3D12_BLEND_ONE;target.DestBlend=target.DestBlendAlpha=D3D12_BLEND_ZERO;target.BlendOp=target.BlendOpAlpha=D3D12_BLEND_OP_ADD;
        pso.SampleMask=UINT_MAX;pso.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;pso.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;
        pso.RasterizerState.FrontCounterClockwise=TRUE;pso.RasterizerState.DepthClipEnable=FALSE;
        pso.DepthStencilState.DepthEnable=FALSE;pso.DepthStencilState.StencilEnable=FALSE;
        pso.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;pso.NumRenderTargets=1;pso.RTVFormats[0]=target_format;pso.SampleDesc.Count=1;
        Check(Pipelines().Graphics(device,pso,pso_.ReleaseAndGetAddressOf()),"grading pipeline");
        D3D12_COMPUTE_PIPELINE_STATE_DESC cs{};cs.pRootSignature=compute_root_.Get();cs.CS={blobs.lut_copy.data(),blobs.lut_copy.size()};
        Check(Pipelines().Compute(device,cs,compute_pso_.ReleaseAndGetAddressOf()),"grading LUT copy pipeline");
        // Descriptors: 0 the source (t0 of the draw), 1 the volume (t16), 2 the LUT strip (t0 of the copy), 3 the volume (u0).
        D3D12_DESCRIPTOR_HEAP_DESC heap{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,4,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
        Check(CountedHeap(device)->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&srv_)),"grading SRV heap");
        heap={D3D12_DESCRIPTOR_HEAP_TYPE_RTV,1,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
        Check(CountedHeap(device)->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&rtv_)),"grading RTV heap");
        srv_stride_=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        const auto slot=[&](uint32_t i){auto h=srv_->GetCPUDescriptorHandleForHeapStart();h.ptr+=SIZE_T(srv_stride_)*i;return h;};
        D3D12_HEAP_PROPERTIES defaults{};defaults.Type=D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC volume{};volume.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE3D;volume.Width=volume.Height=LutEdge;volume.DepthOrArraySize=LutEdge;
        volume.MipLevels=1;volume.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;volume.SampleDesc.Count=1;volume.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        Check(CountedResource(device)->CreateCommittedResource(&defaults,D3D12_HEAP_FLAG_NONE,&volume,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,nullptr,IID_PPV_ARGS(&volume_)),"grading volume");
        D3D12_SHADER_RESOURCE_VIEW_DESC view{};view.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;view.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE3D;
        view.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;view.Texture3D.MipLevels=1;device->CreateShaderResourceView(volume_.Get(),&view,slot(1));
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};uav.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;uav.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE3D;uav.Texture3D.WSize=LutEdge;
        device->CreateUnorderedAccessView(volume_.Get(),nullptr,&uav,slot(3));
        D3D12_SHADER_RESOURCE_VIEW_DESC null{};null.Format=DXGI_FORMAT_R8G8B8A8_UNORM;null.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
        null.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;null.Texture2D.MipLevels=1;device->CreateShaderResourceView(nullptr,&null,slot(2));
        D3D12_HEAP_PROPERTIES upload{};upload.Type=D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=sizeof(vertex_words);buffer.Height=1;
        buffer.DepthOrArraySize=buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Check(CountedResource(device)->CreateCommittedResource(&upload,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&vertices_)),"grading vertices");
        void* memory{};const D3D12_RANGE none{};Check(vertices_->Map(0,&none,&memory),"grading vertices map");
        std::memcpy(memory,vertex_words,sizeof(vertex_words));vertices_->Unmap(0,nullptr);
        MakeSource();return true;
    }catch(const std::exception& e){reason=e.what();Close();return false;}
}
void GradingStage::MakeSource(){
    D3D12_HEAP_PROPERTIES defaults{};defaults.Type=D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=width_;desc.Height=height_;desc.DepthOrArraySize=desc.MipLevels=1;
    desc.Format=format_;desc.SampleDesc.Count=1;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    source_.Reset();Check(CountedResource(device_.Get())->CreateCommittedResource(&defaults,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,IID_PPV_ARGS(&source_)),"grading source");
    source_rtv_=rtv_->GetCPUDescriptorHandleForHeapStart();device_->CreateRenderTargetView(source_.Get(),nullptr,source_rtv_);
    D3D12_SHADER_RESOURCE_VIEW_DESC view{};view.Format=format_;view.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
    view.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;view.Texture2D.MipLevels=1;device_->CreateShaderResourceView(source_.Get(),&view,srv_->GetCPUDescriptorHandleForHeapStart());
}
void GradingStage::Resize(uint32_t width,uint32_t height){
    if(!IsOpen() || !width || !height)throw std::invalid_argument("grading stage resize");
    width_=width;height_=height;MakeSource();
}
void GradingStage::Record(ID3D12GraphicsCommandList* list,ID3D12Resource* lut,DXGI_FORMAT lut_format,uint64_t lut_generation,
    D3D12_CPU_DESCRIPTOR_HANDLE target_rtv,const GradingConstantSink& constants){
    if(!IsOpen() || !list || !lut || !target_rtv.ptr || !constants)throw std::logic_error("grading pass before the stage");
    const auto desc=lut->GetDesc();
    if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D || desc.Width!=LutEdge || desc.Height!=LutEdge*LutEdge)throw std::invalid_argument("grading LUT is not 16 by 256");
    // I-0120 F4: the strip is a loaded texture record's (mAA_BB_CGradingNNNN), immutable under one (resource, generation): its copy
    // into the volume is made when that pair changes (the volume, made at Open, then holds it), not every graded frame.
    const bool copy=NeedsLutView(lut,lut_generation);
    if(copy){
        // Optimisation review axis 2: the four views into a fresh heap (the source, the volume, this LUT, the volume's UAV), the
        // former heap retired by ticket: a LUT change drains nothing.
        ComPtr<ID3D12DescriptorHeap> fresh;const D3D12_DESCRIPTOR_HEAP_DESC heap{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,4,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
        Check(CountedHeap(device_.Get())->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&fresh)),"grading SRV heap");
        const auto at=[&](uint32_t i){auto h=fresh->GetCPUDescriptorHandleForHeapStart();h.ptr+=SIZE_T(srv_stride_)*i;return h;};
        D3D12_SHADER_RESOURCE_VIEW_DESC source{};source.Format=format_;source.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
        source.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;source.Texture2D.MipLevels=1;device_->CreateShaderResourceView(source_.Get(),&source,at(0));
        D3D12_SHADER_RESOURCE_VIEW_DESC volume{};volume.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;volume.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE3D;
        volume.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;volume.Texture3D.MipLevels=1;device_->CreateShaderResourceView(volume_.Get(),&volume,at(1));
        D3D12_SHADER_RESOURCE_VIEW_DESC view{};view.Format=lut_format;view.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
        view.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;view.Texture2D.MipLevels=1;device_->CreateShaderResourceView(lut,&view,at(2));
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};uav.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;uav.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE3D;uav.Texture3D.WSize=LutEdge;
        device_->CreateUnorderedAccessView(volume_.Get(),nullptr,&uav,at(3));
        if(srv_)RetireAfterCurrentRecording(HoldObject(srv_.Get()));srv_=fresh;lut_view_=lut;lut_view_generation_=lut_generation;}
    const auto gpu=[&](uint32_t i){auto h=srv_->GetGPUDescriptorHandleForHeapStart();h.ptr+=UINT64(srv_stride_)*i;return h;};
    ID3D12DescriptorHeap* heaps[]{srv_.Get()};list->SetDescriptorHeaps(1,heaps);
    // 1. FUN_1400E5110's copy (each frame the grading is on in the original): the strip into the volume, when the strip changed.
    if(copy){
        {auto barrier=Transition(volume_.Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);list->ResourceBarrier(1,&barrier);}
        list->SetComputeRootSignature(compute_root_.Get());list->SetPipelineState(compute_pso_.Get());list->SetComputeRootDescriptorTable(0,gpu(2));
        list->Dispatch(1,1,LutEdge);
        {auto barrier=Transition(volume_.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);list->ResourceBarrier(1,&barrier);}}
    // 2. The grading draw from the tone-mapped image.
    {auto barrier=Transition(source_.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);list->ResourceBarrier(1,&barrier);}
    const auto memory=constants(GlobalsBytes);
    if(!memory.cpu || !memory.gpu || memory.gpu%256)throw std::runtime_error("grading constants");
    Globals(static_cast<uint8_t*>(memory.cpu));
    list->SetGraphicsRootSignature(root_.Get());list->SetPipelineState(pso_.Get());list->SetGraphicsRootConstantBufferView(0,memory.gpu);
    list->SetGraphicsRootDescriptorTable(1,gpu(0));
    const D3D12_VERTEX_BUFFER_VIEW view{vertices_->GetGPUVirtualAddress(),sizeof(vertex_words),16};
    list->IASetVertexBuffers(0,1,&view);list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    list->OMSetRenderTargets(1,&target_rtv,FALSE,nullptr);
    const D3D12_VIEWPORT viewport{0,0,float(width_),float(height_),0,1};const D3D12_RECT scissor{0,0,LONG(width_),LONG(height_)};
    list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
    list->DrawInstanced(3,1,0,0);
    {auto barrier=Transition(source_.Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);list->ResourceBarrier(1,&barrier);}
    ++frames_;
}
void GradingStage::Close() noexcept {
    root_.Reset();compute_root_.Reset();pso_.Reset();compute_pso_.Reset();srv_.Reset();rtv_.Reset();vertices_.Reset();volume_.Reset();source_.Reset();device_.Reset();
    source_rtv_={};lut_view_=nullptr;lut_view_generation_=0;format_=DXGI_FORMAT_UNKNOWN;width_=height_=0;
}
void GradingStage::QuarantineToProcessExit() noexcept {
    (void)root_.Detach();(void)compute_root_.Detach();(void)pso_.Detach();(void)compute_pso_.Detach();(void)srv_.Detach();(void)rtv_.Detach();
    (void)vertices_.Detach();(void)volume_.Detach();(void)source_.Detach();(void)device_.Detach();
}
}
