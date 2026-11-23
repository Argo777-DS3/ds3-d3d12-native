#include "auto_exposure.h"
#include "backend.h"
// I-0132 R12h step 2: the kernels built by fxc (shaders/auto_exposure.hlsl).
#include "auto_exposure_histogram.dxbc.h"
#include "auto_exposure_adapt.dxbc.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace ds3native {
using Microsoft::WRL::ComPtr;
namespace {
void Check(HRESULT status,const char* what){if(FAILED(status))throw std::runtime_error(what);}
struct Params {uint32_t size[2];float log2_min,bin_width,low,high,target,dt,speed_brighter,speed_darker,transition;uint32_t reset;float min_log2,max_log2,strength;uint32_t pad;};
static_assert(sizeof(Params)==64);
void Transition(ID3D12GraphicsCommandList* list,ID3D12Resource* resource,D3D12_RESOURCE_STATES from,D3D12_RESOURCE_STATES to){
    D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition.pResource=resource;
    b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;b.Transition.StateBefore=from;b.Transition.StateAfter=to;list->ResourceBarrier(1,&b);}
void UavBarrier(ID3D12GraphicsCommandList* list,ID3D12Resource* resource){
    D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;b.UAV.pResource=resource;list->ResourceBarrier(1,&b);}
ComPtr<ID3D12Resource> Buffer(ID3D12Device* device,uint64_t bytes,D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state,D3D12_RESOURCE_FLAGS flags,const char* what){
    D3D12_HEAP_PROPERTIES heap{};heap.Type=type;D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=bytes;d.Height=1;
    d.DepthOrArraySize=1;d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;d.Flags=flags;ComPtr<ID3D12Resource> r;
    Check(CountedResource(device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&d,state,nullptr,IID_PPV_ARGS(&r)),what);return r;}
}
bool AutoExposureStage::Open(ID3D12Device* device,std::string& reason){
    Close();
    try{
        if(!device)throw std::invalid_argument("auto exposure device");
        device_=device;
        // Root: b0 the tone-map pass's parameter block (root CBV), b1 the parameters (16 constants), u0 the bins and u1 the state (root UAVs),
        // a table of t0 the scene and u2 the factor.
        const D3D12_DESCRIPTOR_RANGE ranges[2]{{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,2,0,1}};
        D3D12_ROOT_PARAMETER p[5]{};
        p[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;p[0].Descriptor={0,0};
        p[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;p[1].Constants={1,0,16};
        p[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;p[2].Descriptor={0,0};
        p[3].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;p[3].Descriptor={1,0};
        p[4].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[4].DescriptorTable={2,ranges};
        for(auto& parameter:p)parameter.ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
        const D3D12_ROOT_SIGNATURE_DESC root{5,p,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
        ComPtr<ID3DBlob> blob,error;
        Check(D3D12SerializeRootSignature(&root,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"auto exposure root serialization");
        Check(Pipelines().Root(device,blob->GetBufferPointer(),blob->GetBufferSize(),root_.ReleaseAndGetAddressOf()),"auto exposure root");
        D3D12_COMPUTE_PIPELINE_STATE_DESC d{};d.pRootSignature=root_.Get();
        d.CS={auto_exposure_histogram_dxbc,sizeof(auto_exposure_histogram_dxbc)};Check(Pipelines().Compute(device,d,pso_histogram_.ReleaseAndGetAddressOf()),"auto exposure histogram pipeline");
        d.CS={auto_exposure_adapt_dxbc,sizeof(auto_exposure_adapt_dxbc)};Check(Pipelines().Compute(device,d,pso_adapt_.ReleaseAndGetAddressOf()),"auto exposure adapt pipeline");
        const D3D12_DESCRIPTOR_HEAP_DESC views{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,2*max_frame_slots,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
        Check(CountedHeap(device)->CreateDescriptorHeap(&views,IID_PPV_ARGS(&views_)),"auto exposure views");
        view_stride_=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        // Committed buffers are zeroed: the bins start empty and the state uninitialised (its mark 0).
        histogram_=Buffer(device,4*bins,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,"auto exposure bins");
        state_=Buffer(device,1024,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,"auto exposure state");
        histogram_->SetName(L"native auto exposure bins");state_->SetName(L"native auto exposure state");
        D3D12_HEAP_PROPERTIES defaults{};defaults.Type=D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC texture{};texture.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;texture.Width=1;texture.Height=1;texture.DepthOrArraySize=1;
        texture.MipLevels=1;texture.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;texture.SampleDesc.Count=1;texture.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        Check(CountedResource(device)->CreateCommittedResource(&defaults,D3D12_HEAP_FLAG_NONE,&texture,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&factor_)),"auto exposure factor");
        factor_->SetName(L"native auto exposure factor");
        for(auto& r:readbacks_)r=Buffer(device,256,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_FLAG_NONE,"auto exposure readback");
        return true;
    }catch(const std::exception& e){reason=e.what();Close();return false;}
}
void AutoExposureStage::Record(ID3D12GraphicsCommandList* list,ID3D12Resource* scene,uint32_t slot,const PostToneParams& tone,const AutoExposureParams& params,
    const AutoExposureConstantSink& constants){
    if(!IsOpen() || !list || !scene || !constants || slot>=max_frame_slots)throw std::logic_error("auto exposure pass before the stage");
    const auto desc=scene->GetDesc();
    if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D || desc.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT)throw std::invalid_argument("auto exposure scene");
    // The reading of the slot written eight frames ago (its copy long complete), then the slot reused below.
    auto& readback=readbacks_[readback_frames_++%readbacks_.size()];
    if(readback_frames_>readbacks_.size()){void* p{};const D3D12_RANGE range{0,32};
        if(SUCCEEDED(readback->Map(0,&range,&p))){float f[7]{};std::memcpy(f,p,sizeof f);const D3D12_RANGE none{};readback->Unmap(0,&none);
            uint32_t pixels{},mark{};std::memcpy(&pixels,&f[3],4);std::memcpy(&mark,&f[4],4);
            reading_={f[0],f[1],f[2],f[5],f[6],pixels,mark==0xA5E0u};}}
    // dt: the CPU's time between two Records, at most 0.25 s (a stall does not jump the adaptation).
    const auto now=std::chrono::steady_clock::now();
    float dt=timed_?std::chrono::duration<float>(now-last_).count():0.0f;if(params.fixed_dt>=0.0f)dt=params.fixed_dt;dt=std::clamp(dt,0.0f,0.25f);last_=now;timed_=true;
    if(!recorded_){Transition(list,histogram_.Get(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        Transition(list,state_.Get(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);}
    else Transition(list,factor_.Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    // This slot's views (its previous list completed before the slot records again).
    auto cpu=views_->GetCPUDescriptorHandleForHeapStart();cpu.ptr+=SIZE_T(view_stride_)*2*slot;
    auto gpu=views_->GetGPUDescriptorHandleForHeapStart();gpu.ptr+=UINT64(view_stride_)*2*slot;
    D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
    v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;v.Texture2D.MipLevels=1;device_->CreateShaderResourceView(scene,&v,cpu);
    D3D12_UNORDERED_ACCESS_VIEW_DESC u{};u.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;u.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
    cpu.ptr+=view_stride_;device_->CreateUnorderedAccessView(factor_.Get(),nullptr,&u,cpu);
    static_assert(sizeof(PostToneParams)<=256,"the tone-map block in the 256 bytes asked below");   // the final review's L1
    const auto memory=constants(256);   // the block's bytes (224 since I-0141 tranche 11) in a constant buffer's 256
    if(!memory.cpu || !memory.gpu || memory.gpu%256)throw std::runtime_error("auto exposure constants");
    std::memset(memory.cpu,0,256);std::memcpy(memory.cpu,&tone,sizeof tone);   // zeroed here: a test's own allocator may not
    Params c{};c.size[0]=uint32_t(desc.Width);c.size[1]=desc.Height;c.log2_min=log2_min;c.bin_width=bin_width;c.low=params.low;c.high=params.high;
    c.target=params.target;c.dt=dt;c.speed_brighter=params.speed_brighter;c.speed_darker=params.speed_darker;c.transition=params.transition;
    c.reset=params.reset?1u:0u;c.min_log2=params.min_log2;c.max_log2=params.max_log2;c.strength=params.strength;
    ID3D12DescriptorHeap* heaps[]{views_.Get()};list->SetDescriptorHeaps(1,heaps);list->SetComputeRootSignature(root_.Get());
    list->SetComputeRootConstantBufferView(0,memory.gpu);list->SetComputeRoot32BitConstants(1,16,&c,0);
    list->SetComputeRootUnorderedAccessView(2,histogram_->GetGPUVirtualAddress());list->SetComputeRootUnorderedAccessView(3,state_->GetGPUVirtualAddress());
    list->SetComputeRootDescriptorTable(4,gpu);
    list->SetPipelineState(pso_histogram_.Get());list->Dispatch((c.size[0]+15)/16,(c.size[1]+15)/16,1);
    UavBarrier(list,histogram_.Get());
    list->SetPipelineState(pso_adapt_.Get());list->Dispatch(1,1,1);
    UavBarrier(list,state_.Get());UavBarrier(list,factor_.Get());UavBarrier(list,histogram_.Get());   // the step review's L2: the bins' clear before any later histogram
    Transition(list,factor_.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    Transition(list,state_.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
    list->CopyBufferRegion(readback.Get(),0,state_.Get(),512,32);
    Transition(list,state_.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    recorded_=true;++frames_;
}
bool AutoExposureStage::CopyDlssExposure(ID3D12GraphicsCommandList* list,ID3D12Resource* exposure){
    if(!recorded_ || !list || !exposure)return false;
    Transition(list,state_.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
    D3D12_TEXTURE_COPY_LOCATION from{},to{};from.pResource=state_.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    from.PlacedFootprint.Offset=0;from.PlacedFootprint.Footprint={DXGI_FORMAT_R32_FLOAT,1,1,1,D3D12_TEXTURE_DATA_PITCH_ALIGNMENT};
    to.pResource=exposure;to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;to.SubresourceIndex=0;
    list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
    Transition(list,state_.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    return true;
}
AutoExposureReading AutoExposureStage::LastReading() const {return reading_;}
void AutoExposureStage::Close() noexcept {
    root_.Reset();pso_histogram_.Reset();pso_adapt_.Reset();views_.Reset();histogram_.Reset();state_.Reset();factor_.Reset();
    for(auto& r:readbacks_)r.Reset();device_.Reset();readback_frames_=0;timed_=false;recorded_=false;reading_={};
}
void AutoExposureStage::QuarantineToProcessExit() noexcept {
    (void)root_.Detach();(void)pso_histogram_.Detach();(void)pso_adapt_.Detach();(void)views_.Detach();(void)histogram_.Detach();
    (void)state_.Detach();(void)factor_.Detach();for(auto& r:readbacks_)(void)r.Detach();(void)device_.Detach();
}
}
