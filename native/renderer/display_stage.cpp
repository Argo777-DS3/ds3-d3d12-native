#include "display_stage.h"
#include "backend.h"
#include <cfloat>
#include <cstring>
#include <stdexcept>

namespace ds3native {
using Microsoft::WRL::ComPtr;
namespace {
void Check(HRESULT status,const char* what){if(FAILED(status))throw std::runtime_error(what);}
D3D12_RESOURCE_BARRIER Barrier(ID3D12Resource* resource,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after){
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition={resource,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};return barrier;}
D3D12_RESOURCE_DESC CurveDesc(){
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE1D;desc.Width=DisplayCurveEntries;desc.Height=1;
    desc.DepthOrArraySize=3;desc.MipLevels=1;desc.Format=DXGI_FORMAT_R32_FLOAT;desc.SampleDesc.Count=1;return desc;}
}
bool DisplayStage::Open(ID3D12Device* device,const DisplayBlobs& blobs,uint32_t width,uint32_t height,std::string& reason){
    Close();
    try{
        if(!device || blobs.vertex.empty() || blobs.pixel.empty() || !width || !height)throw std::invalid_argument("display stage inputs");
        device_=device;width_=width;height_=height;
        // The shader samples the curve linearly: R32_FLOAT must be filterable (GAMMA-COLOUR-PATH-READY 3.1).
        D3D12_FEATURE_DATA_FORMAT_SUPPORT support{DXGI_FORMAT_R32_FLOAT};
        Check(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT,&support,sizeof(support)),"display curve format query");
        if(!(support.Support1&D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE))throw std::runtime_error("display curve format not filterable");
        // Root: t0 the intermediate, t1 the curve (one pixel table), s0 linear clamp with the original's LOD range.
        D3D12_DESCRIPTOR_RANGE range{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,2,0,0,0};
        D3D12_ROOT_PARAMETER parameter{};parameter.ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        parameter.DescriptorTable={1,&range};parameter.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        D3D12_STATIC_SAMPLER_DESC sampler{};sampler.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;sampler.MaxAnisotropy=1;
        sampler.ComparisonFunc=D3D12_COMPARISON_FUNC_NEVER;sampler.MinLOD=-FLT_MAX;sampler.MaxLOD=FLT_MAX;sampler.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        const D3D12_ROOT_SIGNATURE_DESC root{1,&parameter,1,&sampler,D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS|
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS|D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS|D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS};
        ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&root,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"display root serialization");
        Check(Pipelines().Root(device,blob->GetBufferPointer(),blob->GetBufferSize(),root_.ReleaseAndGetAddressOf()),"display root");
        // The original's state (FUN_141933480): a 4-vertex strip without input layout, blend off, depth and stencil off,
        // solid, no culling, no depth clip, one R8G8B8A8_UNORM target.
        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};pso.pRootSignature=root_.Get();
        pso.VS={blobs.vertex.data(),blobs.vertex.size()};pso.PS={blobs.pixel.data(),blobs.pixel.size()};
        pso.BlendState.RenderTarget[0].SrcBlend=pso.BlendState.RenderTarget[0].SrcBlendAlpha=D3D12_BLEND_ONE;
        pso.BlendState.RenderTarget[0].DestBlend=pso.BlendState.RenderTarget[0].DestBlendAlpha=D3D12_BLEND_ZERO;
        pso.BlendState.RenderTarget[0].BlendOp=pso.BlendState.RenderTarget[0].BlendOpAlpha=D3D12_BLEND_OP_ADD;
        pso.BlendState.RenderTarget[0].LogicOp=D3D12_LOGIC_OP_NOOP;pso.BlendState.RenderTarget[0].RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;
        pso.SampleMask=UINT_MAX;pso.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;pso.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;
        pso.RasterizerState.DepthClipEnable=FALSE;pso.DepthStencilState.DepthEnable=FALSE;pso.DepthStencilState.StencilEnable=FALSE;
        pso.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;pso.NumRenderTargets=1;pso.RTVFormats[0]=Format();pso.SampleDesc.Count=1;
        Check(Pipelines().Graphics(device,pso,pipeline_.ReleaseAndGetAddressOf()),"display pipeline");
        D3D12_DESCRIPTOR_HEAP_DESC heap{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,2,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
        Check(CountedHeap(device)->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&srv_)),"display SRV heap");
        heap={D3D12_DESCRIPTOR_HEAP_TYPE_RTV,2,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};   // I-0131 R10b: the second, the HUD-less colour's
        Check(CountedHeap(device)->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&rtv_)),"display RTV heap");
        srv_stride_=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        const auto desc=CurveDesc();D3D12_HEAP_PROPERTIES properties{};properties.Type=D3D12_HEAP_TYPE_DEFAULT;
        Check(CountedResource(device)->CreateCommittedResource(&properties,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&curve_)),"display curve texture");
        D3D12_SHADER_RESOURCE_VIEW_DESC view{};view.Format=DXGI_FORMAT_R32_FLOAT;view.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
        view.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;view.Texture1DArray.MipLevels=1;view.Texture1DArray.ArraySize=3;
        auto handle=srv_->GetCPUDescriptorHandleForHeapStart();handle.ptr+=srv_stride_;device->CreateShaderResourceView(curve_.Get(),&view,handle);
        // The identity curve (brightness 5) until a frame carries another one.
        pending_.assign(DisplayCurveBytes,0);
        if(!GenerateDisplayCurve(DisplayCurveIdentity,std::span<uint8_t,DisplayCurveBytes>(pending_.data(),DisplayCurveBytes)))throw std::logic_error("display identity curve");
        Targets();return true;
    }catch(const std::exception& e){reason=e.what();Close();return false;}
}
void DisplayStage::Targets(){
    // The original's intermediate; the clear value matches the startup clear (a mismatch is a debug warning, which this
    // renderer counts as an error).
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=width_;desc.Height=height_;
    desc.DepthOrArraySize=desc.MipLevels=1;desc.Format=Format();desc.SampleDesc.Count=1;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    D3D12_CLEAR_VALUE clear{Format(),{0,0,0,1}};D3D12_HEAP_PROPERTIES properties{};properties.Type=D3D12_HEAP_TYPE_DEFAULT;
    intermediate_.Reset();hudless_.Reset();   // I-0131 R10b: the HUD-less colour made again at the new extent on its next use
    Check(CountedResource(device_.Get())->CreateCommittedResource(&properties,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_RENDER_TARGET,&clear,IID_PPV_ARGS(&intermediate_)),"display intermediate");
    device_->CreateRenderTargetView(intermediate_.Get(),nullptr,rtv_->GetCPUDescriptorHandleForHeapStart());
    device_->CreateShaderResourceView(intermediate_.Get(),nullptr,srv_->GetCPUDescriptorHandleForHeapStart());++intermediate_generation_;
}
void DisplayStage::Resize(uint32_t width,uint32_t height){
    if(!IsOpen() || !width || !height)throw std::invalid_argument("display stage resize");
    width_=width;height_=height;Targets();
}
void DisplayStage::SetCurve(const DisplayCurve& curve){
    if(!curve || curve==last_)return;
    if(curve->size()!=DisplayCurveBytes)throw std::invalid_argument("display curve extent");
    last_=curve;
    if(*curve!=(pending_.empty()?uploaded_:pending_))pending_=*curve;
}
D3D12_CPU_DESCRIPTOR_HANDLE DisplayStage::Target() const noexcept {return rtv_?rtv_->GetCPUDescriptorHandleForHeapStart():D3D12_CPU_DESCRIPTOR_HANDLE{};}
ID3D12Resource* DisplayStage::Hudless(DXGI_FORMAT format){
    if(!IsOpen())return hudless_.Get();
    if(hudless_){if(hudless_->GetDesc().Format==format)return hudless_.Get();
        RetireAfterCurrentRecording(HoldObject(hudless_.Get()));hudless_.Reset();}   // the chain changed its format between frames
    try{D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=width_;desc.Height=height_;
        desc.DepthOrArraySize=desc.MipLevels=1;desc.Format=format;desc.SampleDesc.Count=1;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        D3D12_HEAP_PROPERTIES properties{};properties.Type=D3D12_HEAP_TYPE_DEFAULT;
        Check(CountedResource(device_.Get())->CreateCommittedResource(&properties,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,IID_PPV_ARGS(&hudless_)),"display HUD-less colour");
        device_->CreateRenderTargetView(hudless_.Get(),nullptr,HudlessTarget());
    }catch(...){hudless_.Reset();}
    return hudless_.Get();
}
D3D12_CPU_DESCRIPTOR_HANDLE DisplayStage::HudlessTarget() const noexcept {
    if(!rtv_ || !device_)return {};auto handle=rtv_->GetCPUDescriptorHandleForHeapStart();
    handle.ptr+=device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);return handle;}
ComPtr<ID3D12Resource> DisplayStage::RecordUploadIfDirty(ID3D12GraphicsCommandList* list){
    if(pending_.empty() || !IsOpen())return nullptr;
    const auto desc=CurveDesc();D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprints[3]{};UINT rows[3]{};UINT64 row_bytes[3]{},total{};
    device_->GetCopyableFootprints(&desc,0,3,0,footprints,rows,row_bytes,&total);
    for(size_t slice=0;slice<3;++slice)if(rows[slice]!=1 || row_bytes[slice]!=DisplayCurveEntries*sizeof(float))throw std::runtime_error("display curve footprint");
    D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=total;buffer.Height=1;buffer.DepthOrArraySize=buffer.MipLevels=1;
    buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;D3D12_HEAP_PROPERTIES properties{};properties.Type=D3D12_HEAP_TYPE_UPLOAD;
    ComPtr<ID3D12Resource> staging;
    Check(CountedResource(device_.Get())->CreateCommittedResource(&properties,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&staging)),"display curve staging");
    std::array<std::array<float,DisplayCurveEntries>,3> slices{};
    DeinterleaveDisplayCurve(std::span<const uint8_t,DisplayCurveBytes>(pending_.data(),DisplayCurveBytes),slices);
    void* memory{};const D3D12_RANGE none{};Check(staging->Map(0,&none,&memory),"display curve staging map");
    for(size_t slice=0;slice<3;++slice)std::memcpy(static_cast<uint8_t*>(memory)+footprints[slice].Offset,slices[slice].data(),sizeof(slices[slice]));
    staging->Unmap(0,nullptr);
    if(curve_initialised_){const auto to_copy=Barrier(curve_.Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_DEST);list->ResourceBarrier(1,&to_copy);}
    for(UINT slice=0;slice<3;++slice){D3D12_TEXTURE_COPY_LOCATION from{},to{};from.pResource=staging.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        from.PlacedFootprint=footprints[slice];to.pResource=curve_.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;to.SubresourceIndex=slice;
        list->CopyTextureRegion(&to,0,0,0,&from,nullptr);}
    const auto to_read=Barrier(curve_.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);list->ResourceBarrier(1,&to_read);
    curve_initialised_=true;uploaded_=std::move(pending_);pending_.clear();++uploads_;return staging;
}
void DisplayStage::RecordPass(ID3D12GraphicsCommandList* list,D3D12_CPU_DESCRIPTOR_HANDLE target,const D3D12_VIEWPORT& viewport,const D3D12_RECT& scissor) const {
    if(!IsOpen() || !curve_initialised_ || !list || !target.ptr)throw std::logic_error("display pass before the stage and its curve");
    list->OMSetRenderTargets(1,&target,FALSE,nullptr);
    ID3D12DescriptorHeap* heaps[]{srv_.Get()};list->SetDescriptorHeaps(1,heaps);
    list->SetGraphicsRootSignature(root_.Get());list->SetGraphicsRootDescriptorTable(0,srv_->GetGPUDescriptorHandleForHeapStart());
    list->SetPipelineState(pipeline_.Get());list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
    list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);list->DrawInstanced(4,1,0,0);++passes_;
}
void DisplayStage::Close() noexcept {
    root_.Reset();pipeline_.Reset();rtv_.Reset();srv_.Reset();intermediate_.Reset();curve_.Reset();hudless_.Reset();device_.Reset();
    uploaded_.clear();pending_.clear();last_.reset();width_=height_=0;curve_initialised_=false;
}
void DisplayStage::QuarantineToProcessExit() noexcept {
    (void)root_.Detach();(void)pipeline_.Detach();(void)rtv_.Detach();(void)srv_.Detach();(void)intermediate_.Detach();(void)curve_.Detach();(void)hudless_.Detach();(void)device_.Detach();
}
}
