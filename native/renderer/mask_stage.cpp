#include "mask_stage.h"
#include "backend.h"
#include <stdexcept>

namespace ds3native {
using Microsoft::WRL::ComPtr;
namespace {
void Check(HRESULT status,const char* what){if(FAILED(status))throw std::runtime_error(what);}
}
bool MaskStage::Open(ID3D12Device* device,uint32_t width,uint32_t height,std::string& reason){
    Close();
    try{
        if(!device || !width || !height)throw std::invalid_argument("mask stage inputs");
        device_=device;width_=width;height_=height;
        const D3D12_DESCRIPTOR_HEAP_DESC heap{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,1,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
        Check(CountedHeap(device)->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&rtv_)),"mask RTV heap");
        MakeTarget();return true;
    }catch(const std::exception& e){reason=e.what();Close();return false;}
}
void MaskStage::MakeTarget(){
    D3D12_HEAP_PROPERTIES properties{};properties.Type=D3D12_HEAP_TYPE_DEFAULT;
    // D-0180: the mask is cleared to white (the shadow term before the cascaded shadow), its optimised clear value.
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=width_;desc.Height=height_;desc.DepthOrArraySize=desc.MipLevels=1;
    desc.Format=ColorFormat();desc.SampleDesc.Count=1;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;mask_.Reset();
    D3D12_CLEAR_VALUE clear{};clear.Format=ColorFormat();for(auto& c:clear.Color)c=1.0f;
    Check(CountedResource(device_.Get())->CreateCommittedResource(&properties,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,&clear,IID_PPV_ARGS(&mask_)),"shadow mask");
    device_->CreateRenderTargetView(mask_.Get(),nullptr,rtv_->GetCPUDescriptorHandleForHeapStart());
    ++target_generation_;
}
void MaskStage::Resize(uint32_t width,uint32_t height){
    if(!IsOpen() || !width || !height)throw std::invalid_argument("mask stage resize");
    width_=width;height_=height;MakeTarget();
}
uint64_t MaskStage::TargetBytes() const noexcept {
    uint64_t bytes{};
    if(mask_){const auto desc=mask_->GetDesc();bytes+=desc.Width*desc.Height*4u;}
    return bytes;}
void MaskStage::Close() noexcept {mask_.Reset();rtv_.Reset();device_.Reset();width_=height_=0;}
void MaskStage::QuarantineToProcessExit() noexcept {
    for(auto* object:{static_cast<IUnknown*>(rtv_.Detach()),static_cast<IUnknown*>(mask_.Detach()),static_cast<IUnknown*>(device_.Detach())})(void)object;
}
}
