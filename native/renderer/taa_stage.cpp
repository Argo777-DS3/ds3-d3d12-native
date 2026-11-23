#include "taa_stage.h"
#include "backend.h"
// I-0132 R12h step 2: the TAA's kernels built by fxc (shaders/taa.hlsl).
#include "taa_resolve.dxbc.h"
#include "taa_sharpen.dxbc.h"
#include "taa_mirror.dxbc.h"
#include <cstring>
#include <stdexcept>

namespace ds3native {
using Microsoft::WRL::ComPtr;
namespace {
void Check(HRESULT status,const char* what){if(FAILED(status))throw std::runtime_error(what);}
constexpr uint32_t kTable=8;   // t0..t5, u0..u1
struct Constants {uint32_t size[2];float inv_size[2];float jitter[2];float exposure,feedback;uint32_t reset;float clip_gamma,sharpen;uint32_t reactive,fade;float kernel;uint32_t own_reset;};
static_assert(sizeof(Constants)==60);
void Transition(ID3D12GraphicsCommandList* list,ID3D12Resource* resource,D3D12_RESOURCE_STATES from,D3D12_RESOURCE_STATES to){
    D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition.pResource=resource;
    b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;b.Transition.StateBefore=from;b.Transition.StateAfter=to;list->ResourceBarrier(1,&b);}
}
static_assert(TaaStage::slots==max_frame_slots);
bool TaaStage::Open(ID3D12Device* device,std::string& reason){
    Close();
    try{
        if(!device)throw std::invalid_argument("taa device");
        device_=device;
        // Root: b0 the constants (15 words; I-0140 tranche 41: the kernel and own_reset), a table of t0..t5 and u0..u1 (I-0113 tranche 39: t4 the reactive base; I-0140: t5 the fade
        // mask); s0 a static linear clamp sampler (the history's taps).
        const D3D12_DESCRIPTOR_RANGE ranges[2]{{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,6,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,2,0,0,6}};
        D3D12_ROOT_PARAMETER p[2]{};
        p[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;p[0].Constants={0,0,sizeof(Constants)/4};
        p[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[1].DescriptorTable={2,ranges};
        for(auto& parameter:p)parameter.ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
        D3D12_STATIC_SAMPLER_DESC sampler{};sampler.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;sampler.MaxLOD=D3D12_FLOAT32_MAX;
        sampler.ShaderRegister=0;sampler.ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
        const D3D12_ROOT_SIGNATURE_DESC root{2,p,1,&sampler,D3D12_ROOT_SIGNATURE_FLAG_NONE};
        ComPtr<ID3DBlob> blob,error;
        Check(D3D12SerializeRootSignature(&root,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"taa root serialization");
        Check(Pipelines().Root(device,blob->GetBufferPointer(),blob->GetBufferSize(),root_.ReleaseAndGetAddressOf()),"taa root");
        // The step review's M2: IEEE strictness (fxc /Gis, the build's row), so the shader's non-finite guards are not folded away.
        D3D12_COMPUTE_PIPELINE_STATE_DESC d{};d.pRootSignature=root_.Get();d.CS={taa_resolve_dxbc,sizeof(taa_resolve_dxbc)};
        Check(Pipelines().Compute(device,d,pipeline_.ReleaseAndGetAddressOf()),"taa pipeline");
        d.CS={taa_sharpen_dxbc,sizeof(taa_sharpen_dxbc)};Check(Pipelines().Compute(device,d,sharpen_.ReleaseAndGetAddressOf()),"taa sharpen pipeline");
        // I-0132 R12c: the copy before the sharpening of DLSS's output.
        d.CS={taa_mirror_dxbc,sizeof(taa_mirror_dxbc)};Check(Pipelines().Compute(device,d,mirror_.ReleaseAndGetAddressOf()),"taa mirror pipeline");
        // R12h step 2's step review H1 (a comment swallowed the Mirror's creation): the stage opens only with its three pipelines.
        if(!pipeline_ || !sharpen_ || !mirror_)throw std::runtime_error("taa: a pipeline missing after Open");
        // Each slot: the resolve's eight views (a table), then the sharpening's (its t3 the history just written, its u0 the output);
        // then, after every slot's sixteen, RecordSharpen's sixteen a slot (the mirror's eight, the sharpening's eight).
        const D3D12_DESCRIPTOR_HEAP_DESC views{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,4*kTable*slots,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
        Check(CountedHeap(device)->CreateDescriptorHeap(&views,IID_PPV_ARGS(&views_)),"taa views");
        view_stride_=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        return true;
    }catch(const std::exception& e){reason=e.what();Close();return false;}
}
void TaaStage::MakeHistory(uint32_t width,uint32_t height,DXGI_FORMAT format){
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC image{};image.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;image.Width=width;image.Height=height;image.DepthOrArraySize=1;
    image.MipLevels=1;image.Format=format;image.SampleDesc.Count=1;image.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    for(auto& history:history_){
        Check(CountedResource(device_.Get())->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&image,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,
            IID_PPV_ARGS(history.ReleaseAndGetAddressOf())),"taa history");
        history->SetName(L"native TAA history");}
    written_=0;history_valid_=false;
}
bool TaaStage::Prepare(ID3D12Resource* colour,ID3D12Resource* depth,ID3D12Resource* motion,ID3D12Resource* output,std::string& reason){
    prepared_=nullptr;
    try{
        if(!IsOpen() || !colour || !depth || !motion || !output)throw std::invalid_argument("taa: the stage or an input missing");
        const auto c=colour->GetDesc(),o=output->GetDesc(),m=motion->GetDesc(),z=depth->GetDesc();
        if(c.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D || c.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT)throw std::invalid_argument("taa: the colour's format");
        if(z.Format!=DXGI_FORMAT_R32G8X24_TYPELESS)throw std::invalid_argument("taa: the depth's format (the scene's typeless depth)");
        if(o.Width!=c.Width || o.Height!=c.Height || o.Format!=c.Format || m.Width!=c.Width || m.Height!=c.Height || z.Width!=c.Width || z.Height!=c.Height)
            throw std::invalid_argument("taa: the inputs' extents or the output's format");
        if(!history_[0] || history_[0]->GetDesc().Width!=c.Width || history_[0]->GetDesc().Height!=c.Height)MakeHistory(uint32_t(c.Width),c.Height,c.Format);
        prepared_=colour;return true;
    }catch(const std::exception& e){reason=e.what();for(auto& h:history_)h.Reset();history_valid_=false;return false;}
}
bool TaaStage::Record(ID3D12GraphicsCommandList* list,ID3D12Resource* colour,ID3D12Resource* depth,ID3D12Resource* motion,ID3D12Resource* output,
    const TaaParams& params,uint32_t slot){
    if(!IsOpen() || !list || !colour || !depth || !motion || !output || slot>=slots || prepared_!=colour || !history_[0])throw std::logic_error("taa pass before its Prepare");
    prepared_=nullptr;
    const auto c=colour->GetDesc(),m=motion->GetDesc();
    if(params.base){const auto b=params.base->GetDesc();if(b.Width!=c.Width || b.Height!=c.Height || b.Format!=c.Format)throw std::logic_error("taa: the reactive base's extent or format");}
    if(params.fade_mask){const auto f=params.fade_mask->GetDesc();if(!params.base || f.Width!=c.Width || f.Height!=c.Height || f.Format!=kTaaFadeMaskFormat)
        throw std::logic_error("taa: the fade mask without the base, or its extent or format");}   // I-0140
    const uint32_t width=uint32_t(c.Width),height=c.Height;
    const bool reset=params.reset || !history_valid_;
    const uint32_t read=written_,write=1-written_;
    // This slot's views (its previous list completed before the slot records again).
    auto cpu=views_->GetCPUDescriptorHandleForHeapStart();cpu.ptr+=SIZE_T(view_stride_)*2*kTable*slot;
    auto gpu=views_->GetGPUDescriptorHandleForHeapStart();gpu.ptr+=UINT64(view_stride_)*2*kTable*slot;
    const auto next=[&]{auto at=cpu;cpu.ptr+=view_stride_;return at;};
    D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;v.Texture2D.MipLevels=1;
    v.Format=c.Format;device_->CreateShaderResourceView(colour,&v,next());
    v.Format=DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;device_->CreateShaderResourceView(depth,&v,next());
    v.Format=m.Format;device_->CreateShaderResourceView(motion,&v,next());
    v.Format=c.Format;device_->CreateShaderResourceView(history_[read].Get(),&v,next());
    device_->CreateShaderResourceView(params.base,&v,next());   // t4: the reactive base, or a null view (read only with `reactive`)
    v.Format=kTaaFadeMaskFormat;device_->CreateShaderResourceView(params.fade_mask,&v,next());   // t5 (I-0140): the fade mask, or a null view (read only with `fade`)
    D3D12_UNORDERED_ACCESS_VIEW_DESC u{};u.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;u.Format=c.Format;
    device_->CreateUnorderedAccessView(output,nullptr,&u,next());device_->CreateUnorderedAccessView(history_[write].Get(),nullptr,&u,next());
    const bool sharpened=params.sharpen>=1e-3f;   // the second step review's L-c: a tiny value is none (the shader's test agrees)
    if(sharpened){   // the sharpening's table: t0..t2 as the resolve's, t3 the history just written, u0 the output, u1 the same history (unused)
        v.Format=c.Format;device_->CreateShaderResourceView(colour,&v,next());
        v.Format=DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;device_->CreateShaderResourceView(depth,&v,next());
        v.Format=m.Format;device_->CreateShaderResourceView(motion,&v,next());
        v.Format=c.Format;device_->CreateShaderResourceView(history_[write].Get(),&v,next());device_->CreateShaderResourceView(nullptr,&v,next());
        v.Format=kTaaFadeMaskFormat;device_->CreateShaderResourceView(nullptr,&v,next());
        device_->CreateUnorderedAccessView(output,nullptr,&u,next());device_->CreateUnorderedAccessView(history_[read].Get(),nullptr,&u,next());}
    Constants k{};k.size[0]=width;k.size[1]=height;k.inv_size[0]=1.0f/float(width);k.inv_size[1]=1.0f/float(height);
    k.jitter[0]=params.jitter_x;k.jitter[1]=params.jitter_y;k.exposure=params.exposure>0.0f?params.exposure:1.0f;k.feedback=params.feedback;
    k.reset=reset?1u:0u;k.clip_gamma=params.clip_gamma;k.sharpen=sharpened?(params.sharpen<1.0f?params.sharpen:1.0f):0.0f;k.reactive=params.base?(params.reactive_view?2u:1u):0u;k.fade=params.base && params.fade_mask?1u:0u;
    k.kernel=params.kernel>0.0f?params.kernel:2.29f;k.own_reset=reset && params.own_reset && params.jitter_x==0.0f && params.jitter_y==0.0f?1u:0u;   // I-0140 tranche 41
    Transition(list,history_[read].Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    ID3D12DescriptorHeap* heaps[]{views_.Get()};list->SetDescriptorHeaps(1,heaps);list->SetComputeRootSignature(root_.Get());
    list->SetComputeRoot32BitConstants(0,sizeof(Constants)/4,&k,0);list->SetComputeRootDescriptorTable(1,gpu);
    list->SetPipelineState(pipeline_.Get());list->Dispatch((width+7)/8,(height+7)/8,1);
    Transition(list,history_[read].Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    if(sharpened){   // the history just written read by the sharpening into the output, then back to unordered access
        Transition(list,history_[write].Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        auto table=gpu;table.ptr+=UINT64(view_stride_)*kTable;list->SetComputeRootDescriptorTable(1,table);
        list->SetPipelineState(sharpen_.Get());list->Dispatch((width+7)/8,(height+7)/8,1);
        Transition(list,history_[write].Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);}
    else{D3D12_RESOURCE_BARRIER written{};written.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;written.UAV.pResource=history_[write].Get();list->ResourceBarrier(1,&written);}
    written_=write;history_valid_=true;++frames_;if(reset)++resets_;if(params.base)++reactive_frames_;if(params.base && params.reactive_view && !sharpened && !reset)++view_frames_;
    return reset;
}
void TaaStage::RecordSharpen(ID3D12GraphicsCommandList* list,ID3D12Resource* target,float exposure,float sharpen,uint32_t slot){
    if(!IsOpen() || !mirror_ || !list || !target || slot>=slots)throw std::logic_error("taa sharpening: the stage or its target missing");
    const auto t=target->GetDesc();
    if(t.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D || t.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT || !(t.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
        throw std::invalid_argument("taa sharpening: the target's format or its unordered access");
    const uint32_t width=uint32_t(t.Width),height=t.Height;
    if(!scratch_ || scratch_->GetDesc().Width!=t.Width || scratch_->GetDesc().Height!=t.Height){
        D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC image{};image.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;image.Width=width;image.Height=height;image.DepthOrArraySize=1;
        image.MipLevels=1;image.Format=t.Format;image.SampleDesc.Count=1;image.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        Check(CountedResource(device_.Get())->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&image,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,nullptr,
            IID_PPV_ARGS(scratch_.ReleaseAndGetAddressOf())),"taa sharpening scratch");
        scratch_->SetName(L"native DLSS sharpening scratch");}
    auto cpu=views_->GetCPUDescriptorHandleForHeapStart();cpu.ptr+=SIZE_T(view_stride_)*2*kTable*(slots+slot);
    auto gpu=views_->GetGPUDescriptorHandleForHeapStart();gpu.ptr+=UINT64(view_stride_)*2*kTable*(slots+slot);
    const auto next=[&]{auto at=cpu;cpu.ptr+=view_stride_;return at;};
    D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;v.Texture2D.MipLevels=1;v.Format=t.Format;
    D3D12_UNORDERED_ACCESS_VIEW_DESC u{};u.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;u.Format=t.Format;
    // The mirror's table: t0..t5 null (not read), u0 the scratch, u1 the target (read). The sharpening's: t0..t3 the scratch (t3 read),
    // t4 and t5 null, u0 the target (written), u1 the target (not read).
    for(int i=0;i<6;++i)device_->CreateShaderResourceView(nullptr,&v,next());
    device_->CreateUnorderedAccessView(scratch_.Get(),nullptr,&u,next());device_->CreateUnorderedAccessView(target,nullptr,&u,next());
    for(int i=0;i<4;++i)device_->CreateShaderResourceView(scratch_.Get(),&v,next());
    device_->CreateShaderResourceView(nullptr,&v,next());device_->CreateShaderResourceView(nullptr,&v,next());
    device_->CreateUnorderedAccessView(target,nullptr,&u,next());device_->CreateUnorderedAccessView(target,nullptr,&u,next());
    Constants k{};k.size[0]=width;k.size[1]=height;k.inv_size[0]=1.0f/float(width);k.inv_size[1]=1.0f/float(height);
    k.exposure=exposure>0.0f?exposure:1.0f;k.sharpen=sharpen<1.0f?sharpen:1.0f;
    D3D12_RESOURCE_BARRIER written{};written.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;written.UAV.pResource=target;list->ResourceBarrier(1,&written);   // DLSS's writes first
    Transition(list,scratch_.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    ID3D12DescriptorHeap* heaps[]{views_.Get()};list->SetDescriptorHeaps(1,heaps);list->SetComputeRootSignature(root_.Get());
    list->SetComputeRoot32BitConstants(0,sizeof(Constants)/4,&k,0);list->SetComputeRootDescriptorTable(1,gpu);
    list->SetPipelineState(mirror_.Get());list->Dispatch((width+7)/8,(height+7)/8,1);
    Transition(list,scratch_.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    auto table=gpu;table.ptr+=UINT64(view_stride_)*kTable;list->SetComputeRootDescriptorTable(1,table);
    list->SetPipelineState(sharpen_.Get());list->Dispatch((width+7)/8,(height+7)/8,1);
    list->ResourceBarrier(1,&written);   // the sharpened target before its next reader
    ++sharpen_frames_;
}
void TaaStage::Close() noexcept {
    prepared_=nullptr;root_.Reset();pipeline_.Reset();sharpen_.Reset();mirror_.Reset();scratch_.Reset();views_.Reset();for(auto& h:history_)h.Reset();device_.Reset();written_=0;history_valid_=false;
}
void TaaStage::QuarantineToProcessExit() noexcept {
    (void)root_.Detach();(void)pipeline_.Detach();(void)sharpen_.Detach();(void)mirror_.Detach();(void)scratch_.Detach();(void)views_.Detach();for(auto& h:history_)(void)h.Detach();(void)device_.Detach();
}
}
