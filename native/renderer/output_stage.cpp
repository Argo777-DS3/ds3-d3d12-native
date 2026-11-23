#include "output_stage.h"
#include "backend.h"
#include "blue_noise.h"
#include <cstring>
#include <stdexcept>
#include "post_output_vs.dxbc.h"         // fxc output (Build-GameBootstrap.ps1): const BYTE post_output_vs_dxbc[],
#include "post_output_copy_ps.dxbc.h"    // post_output_copy_ps_dxbc[], post_output_scene_ps_dxbc[]
#include "post_output_scene_ps.dxbc.h"

namespace ds3native {
using Microsoft::WRL::ComPtr;
namespace {
void Check(HRESULT status,const char* what){if(FAILED(status))throw std::runtime_error(what);}
D3D12_RESOURCE_BARRIER Transition(ID3D12Resource* resource,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after){
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;barrier.Transition={resource,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};return barrier;}
// Descriptors, in threes (the root's one table t0..t2). The picture heap: the 8-bit picture, null, the noise (RecordCopy). The
// scene heap: null, the float picture, the noise (RecordScene).
constexpr uint32_t kViews=3;
constexpr UINT kNoisePitch=(kBlueNoiseEdge+D3D12_TEXTURE_DATA_PITCH_ALIGNMENT-1)/D3D12_TEXTURE_DATA_PITCH_ALIGNMENT*D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
}
bool OutputStage::Open(ID3D12Device* device,std::string& reason){
    Close();
    try{
        if(!device)throw std::invalid_argument("output stage inputs");
        device_=device;
        // Root: b0 the parameter block, one table t0..t2 (read by Load: no sampler).
        D3D12_ROOT_PARAMETER parameters[2]{};
        parameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;parameters[0].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        const D3D12_DESCRIPTOR_RANGE textures{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,3,0,0,0};
        parameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[1].DescriptorTable={1,&textures};
        parameters[1].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        const D3D12_ROOT_SIGNATURE_DESC root{2,parameters,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS|D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS|
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS|D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS};
        ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&root,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"output root serialization");
        Check(Pipelines().Root(device,blob->GetBufferPointer(),blob->GetBufferSize(),root_.ReleaseAndGetAddressOf()),"output root");
        const auto make=[&](const BYTE* pixel,size_t bytes,DXGI_FORMAT format,ComPtr<ID3D12PipelineState>& out,const char* what){
            D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};pso.pRootSignature=root_.Get();
            pso.VS={post_output_vs_dxbc,sizeof post_output_vs_dxbc};pso.PS={pixel,bytes};
            auto& target=pso.BlendState.RenderTarget[0];target.LogicOp=D3D12_LOGIC_OP_NOOP;target.RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;
            target.SrcBlend=target.SrcBlendAlpha=D3D12_BLEND_ONE;target.DestBlend=target.DestBlendAlpha=D3D12_BLEND_ZERO;target.BlendOp=target.BlendOpAlpha=D3D12_BLEND_OP_ADD;
            pso.SampleMask=UINT_MAX;pso.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;pso.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;
            pso.RasterizerState.DepthClipEnable=FALSE;pso.DepthStencilState.DepthEnable=FALSE;pso.DepthStencilState.StencilEnable=FALSE;
            pso.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;pso.NumRenderTargets=1;pso.RTVFormats[0]=format;pso.SampleDesc.Count=1;
            Check(Pipelines().Graphics(device,pso,out.ReleaseAndGetAddressOf()),what);};
        make(post_output_copy_ps_dxbc,sizeof post_output_copy_ps_dxbc,SceneFormat(),copy_pso_,"output pipeline (the 8-bit picture copied)");
        make(post_output_scene_ps_dxbc,sizeof post_output_scene_ps_dxbc,Format(),scene_pso_,"output pipeline (scene)");
        stride_=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        const D3D12_DESCRIPTOR_HEAP_DESC heap{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,1,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
        Check(CountedHeap(device)->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&rtv_)),"output RTV heap");
        // The blue noise (R8_UNORM, 64x64), filled from an upload buffer by the first Record.
        D3D12_HEAP_PROPERTIES defaults{};defaults.Type=D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC texture{};texture.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;texture.Width=texture.Height=kBlueNoiseEdge;
        texture.DepthOrArraySize=texture.MipLevels=1;texture.Format=DXGI_FORMAT_R8_UNORM;texture.SampleDesc.Count=1;
        Check(CountedResource(device)->CreateCommittedResource(&defaults,D3D12_HEAP_FLAG_NONE,&texture,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&noise_)),"output noise");
        D3D12_HEAP_PROPERTIES upload{};upload.Type=D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=UINT64(kNoisePitch)*kBlueNoiseEdge;buffer.Height=1;
        buffer.DepthOrArraySize=buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Check(CountedResource(device)->CreateCommittedResource(&upload,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&noise_upload_)),"output noise upload");
        void* memory{};const D3D12_RANGE none{};Check(noise_upload_->Map(0,&none,&memory),"output noise map");
        for(uint32_t row=0;row<kBlueNoiseEdge;++row)std::memcpy(static_cast<uint8_t*>(memory)+size_t(row)*kNoisePitch,kBlueNoise.data()+size_t(row)*kBlueNoiseEdge,kBlueNoiseEdge);
        noise_upload_->Unmap(0,nullptr);
        return true;
    }catch(const std::exception& e){reason=e.what();Close();return false;}
}
bool OutputStage::Targets(uint32_t width,uint32_t height,std::string& reason,bool recording){
    if(scene_ && width==width_ && height==height_)return true;
    try{
        if(!IsOpen() || !width || !height)throw std::invalid_argument("output stage targets");
        ComPtr<ID3D12Resource> scene;D3D12_HEAP_PROPERTIES defaults{};defaults.Type=D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=width;desc.Height=height;desc.DepthOrArraySize=desc.MipLevels=1;
        desc.SampleDesc.Count=1;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        desc.Format=SceneFormat();D3D12_CLEAR_VALUE black{SceneFormat(),{0,0,0,1}};
        Check(CountedResource(device_.Get())->CreateCommittedResource(&defaults,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_RENDER_TARGET,&black,IID_PPV_ARGS(&scene)),"output scene");
        scene->SetName(L"hdr scene");
        // Retired by ticket: a frame in flight may still read it.
        if(scene_){if(recording)RetireAfterCurrentRecording(HoldObject(scene_.Get()));else RetireWhenSubmittedWorkCompletes(HoldObject(scene_.Get()));}
        scene_=scene;width_=width;height_=height;++targets_generation_;
        device_->CreateRenderTargetView(scene_.Get(),nullptr,SceneTarget());
        return true;
    }catch(const std::exception& e){reason=e.what();return false;}
}
D3D12_CPU_DESCRIPTOR_HANDLE OutputStage::SceneTarget() const noexcept {return rtv_?rtv_->GetCPUDescriptorHandleForHeapStart():D3D12_CPU_DESCRIPTOR_HANDLE{};}
// Each heap made afresh when what it shows changed, the former heap retired by ticket (the frames in flight that read it keep it).
namespace {
D3D12_SHADER_RESOURCE_VIEW_DESC View(DXGI_FORMAT format){
    D3D12_SHADER_RESOURCE_VIEW_DESC view{};view.Format=format;view.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;view.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;view.Texture2D.MipLevels=1;return view;}
}
void OutputStage::PictureViews(ID3D12Resource* picture,uint64_t picture_generation){
    if(picture_views_ && picture==picture_view_ && picture_generation==picture_view_generation_)return;
    ComPtr<ID3D12DescriptorHeap> fresh;const D3D12_DESCRIPTOR_HEAP_DESC heap{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,kViews,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
    Check(CountedHeap(device_.Get())->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&fresh)),"output picture view heap");
    const auto at=[&](uint32_t i){auto h=fresh->GetCPUDescriptorHandleForHeapStart();h.ptr+=SIZE_T(stride_)*i;return h;};
    auto view=View(PictureFormat());device_->CreateShaderResourceView(picture,&view,at(0));
    view=View(SceneFormat());device_->CreateShaderResourceView(nullptr,&view,at(1));   // a null view (reads 0)
    view=View(DXGI_FORMAT_R8_UNORM);device_->CreateShaderResourceView(noise_.Get(),&view,at(2));
    if(picture_views_)RetireAfterCurrentRecording(HoldObject(picture_views_.Get()));
    picture_views_=fresh;picture_view_=picture;picture_view_generation_=picture_generation;
}
void OutputStage::SceneViews(){
    if(scene_views_ && targets_generation_==targets_view_generation_)return;
    ComPtr<ID3D12DescriptorHeap> fresh;const D3D12_DESCRIPTOR_HEAP_DESC heap{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,kViews,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
    Check(CountedHeap(device_.Get())->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&fresh)),"output scene view heap");
    const auto at=[&](uint32_t i){auto h=fresh->GetCPUDescriptorHandleForHeapStart();h.ptr+=SIZE_T(stride_)*i;return h;};
    auto view=View(PictureFormat());device_->CreateShaderResourceView(nullptr,&view,at(0));   // a null view
    view=View(SceneFormat());device_->CreateShaderResourceView(scene_.Get(),&view,at(1));
    view=View(DXGI_FORMAT_R8_UNORM);device_->CreateShaderResourceView(noise_.Get(),&view,at(2));
    if(scene_views_)RetireAfterCurrentRecording(HoldObject(scene_views_.Get()));
    scene_views_=fresh;targets_view_generation_=targets_generation_;
}
void OutputStage::Draw(ID3D12GraphicsCommandList* list,ID3D12PipelineState* pso,ID3D12DescriptorHeap* views,D3D12_CPU_DESCRIPTOR_HANDLE target_rtv,D3D12_GPU_VIRTUAL_ADDRESS constants,
    const D3D12_VIEWPORT& viewport,const D3D12_RECT& scissor){
    if(!noise_ready_){
        D3D12_TEXTURE_COPY_LOCATION to{};to.pResource=noise_.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        D3D12_TEXTURE_COPY_LOCATION from{};from.pResource=noise_upload_.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        from.PlacedFootprint.Footprint={DXGI_FORMAT_R8_UNORM,kBlueNoiseEdge,kBlueNoiseEdge,1,kNoisePitch};
        list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        const auto barrier=Transition(noise_.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);list->ResourceBarrier(1,&barrier);noise_ready_=true;}
    ID3D12DescriptorHeap* heaps[]{views};list->SetDescriptorHeaps(1,heaps);
    list->SetGraphicsRootSignature(root_.Get());list->SetPipelineState(pso);
    if(constants)list->SetGraphicsRootConstantBufferView(0,constants);   // the copy's programme reads no constant
    list->SetGraphicsRootDescriptorTable(1,views->GetGPUDescriptorHandleForHeapStart());
    list->IASetVertexBuffers(0,0,nullptr);list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    list->OMSetRenderTargets(1,&target_rtv,FALSE,nullptr);
    list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
    list->DrawInstanced(3,1,0,0);
}
void OutputStage::RecordCopy(ID3D12GraphicsCommandList* list,ID3D12Resource* picture,uint64_t picture_generation,const D3D12_VIEWPORT& viewport,const D3D12_RECT& scissor){
    if(!IsOpen() || !list || !picture || !scene_)throw std::logic_error("output copy before the stage's picture");
    PictureViews(picture,picture_generation);Draw(list,copy_pso_.Get(),picture_views_.Get(),SceneTarget(),0,viewport,scissor);++copy_passes_;
}
void OutputStage::RecordScene(ID3D12GraphicsCommandList* list,D3D12_CPU_DESCRIPTOR_HANDLE target_rtv,const hdr::OutputParams& params,
    const OutputConstantSink& constants,const D3D12_VIEWPORT& viewport,const D3D12_RECT& scissor){
    if(!IsOpen() || !list || !scene_ || !target_rtv.ptr || !constants)throw std::logic_error("output pass before the stage's picture");
    const auto memory=constants(256);
    if(!memory.cpu || !memory.gpu || memory.gpu%256)throw std::runtime_error("output constants");
    std::memcpy(memory.cpu,&params,sizeof params);
    SceneViews();Draw(list,scene_pso_.Get(),scene_views_.Get(),target_rtv,memory.gpu,viewport,scissor);++scene_passes_;
}
void OutputStage::Close() noexcept {
    root_.Reset();copy_pso_.Reset();scene_pso_.Reset();picture_views_.Reset();scene_views_.Reset();rtv_.Reset();scene_.Reset();noise_.Reset();noise_upload_.Reset();device_.Reset();
    picture_view_=nullptr;picture_view_generation_=targets_generation_=targets_view_generation_=0;width_=height_=stride_=0;noise_ready_=false;
}
void OutputStage::QuarantineToProcessExit() noexcept {
    (void)root_.Detach();(void)copy_pso_.Detach();(void)scene_pso_.Detach();(void)picture_views_.Detach();(void)scene_views_.Detach();(void)rtv_.Detach();(void)scene_.Detach();
    (void)noise_.Detach();(void)noise_upload_.Detach();(void)device_.Detach();
}
}
