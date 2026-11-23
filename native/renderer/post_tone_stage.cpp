#include "post_tone_stage.h"
#include "backend.h"
#include <cstring>
#include <stdexcept>
#include "post_tone_vs.dxbc.h"       // fxc output (Build-GameBootstrap.ps1): const BYTE post_tone_vs_dxbc[], post_tone_ps_dxbc[],
#include "post_tone_ps.dxbc.h"       // post_tone_lut_ps_dxbc[], post_tone_reinhard_ps_dxbc[]
#include "post_tone_lut_ps.dxbc.h"
#include "post_tone_reinhard_ps.dxbc.h"   // I-0138 part 4c: the Reinhard curve's entry
#include "post_tone_hdr_ps.dxbc.h"            // I-0138 tranche 5: the HDR entries
#include "post_tone_hdr_reinhard_ps.dxbc.h"
#include "post_tone_agx_ps.dxbc.h"            // I-0138 tranche 6: the candidate looks' entries
#include "post_tone_aces_ps.dxbc.h"
#include "post_tone_hdr_agx_ps.dxbc.h"
#include "post_tone_hdr_aces_ps.dxbc.h"
#include "post_tone_er_ps.dxbc.h"
#include "post_tone_hdr_er_ps.dxbc.h"
#include "post_tone_lut_copy_cs.dxbc.h"         // I-0138 tranche 8: the strip's copy into the volume, ours

namespace ds3native {
using Microsoft::WRL::ComPtr;
namespace {
void Check(HRESULT status,const char* what){if(FAILED(status))throw std::runtime_error(what);}
// Descriptors, in fours (the root's one table t0..t3): 0 the scene, 1 the glare, 2 the factor, 3 the dither (the tone map's draw);
// 4 the intermediate, 5 the volume, 6 and 7 null (the LUT's draw); 8 the strip and 9 the volume's UAV (the copy's t0 and u0);
// 10 the scene, 11 the glare, 12 the factor, 13 the volume (the HDR draw: the LUT in the dither's place).
constexpr uint32_t kViews=14,kLutViews=4,kCopyViews=8,kHdrViews=10;
// s0, the glare's sampler as the original binds it for its tone-map draw (i66-reference-1643, PSGetSamplers: trilinear, clamp); s1,
// the tone-mapped image's and the LUT's as for its grading draw (Reference1696: filter 20, linear with point mips, clamp, for
// both); MaxLOD 0.
D3D12_STATIC_SAMPLER_DESC Sampler(uint32_t reg,D3D12_FILTER filter){
    D3D12_STATIC_SAMPLER_DESC s{};s.Filter=filter;s.AddressU=s.AddressV=s.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    s.MaxAnisotropy=1;s.ComparisonFunc=D3D12_COMPARISON_FUNC_NEVER;s.BorderColor=D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    s.MinLOD=0;s.MaxLOD=0;s.ShaderRegister=reg;s.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;return s;}
D3D12_RESOURCE_BARRIER Transition(ID3D12Resource* resource,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after){
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;barrier.Transition={resource,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};return barrier;}
// The original's states for both of its draws (YEBIS-NATIVE 5.2, render state 0): opaque, RGBA written; solid, cull none, front
// counter-clockwise, depth clip off; no depth or stencil. The three vertices come from the vertex programme.
D3D12_GRAPHICS_PIPELINE_STATE_DESC Pipeline(ID3D12RootSignature* root,const BYTE* pixel,size_t pixel_bytes,DXGI_FORMAT target_format){
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};pso.pRootSignature=root;pso.VS={post_tone_vs_dxbc,sizeof post_tone_vs_dxbc};pso.PS={pixel,pixel_bytes};
    auto& target=pso.BlendState.RenderTarget[0];target.LogicOp=D3D12_LOGIC_OP_NOOP;target.RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;
    target.SrcBlend=target.SrcBlendAlpha=D3D12_BLEND_ONE;target.DestBlend=target.DestBlendAlpha=D3D12_BLEND_ZERO;target.BlendOp=target.BlendOpAlpha=D3D12_BLEND_OP_ADD;
    pso.SampleMask=UINT_MAX;pso.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;pso.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;
    pso.RasterizerState.FrontCounterClockwise=TRUE;pso.RasterizerState.DepthClipEnable=FALSE;
    pso.DepthStencilState.DepthEnable=FALSE;pso.DepthStencilState.StencilEnable=FALSE;
    pso.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;pso.NumRenderTargets=1;pso.RTVFormats[0]=target_format;pso.SampleDesc.Count=1;return pso;}
}
bool PostToneStage::Open(ID3D12Device* device,const std::array<uint8_t,64>& dither,DXGI_FORMAT target_format,uint32_t width,uint32_t height,std::string& reason){
    Close();
    try{
        if(!device || target_format==DXGI_FORMAT_UNKNOWN || !width || !height)throw std::invalid_argument("post tone stage inputs");
        device_=device;format_=target_format;width_=width;height_=height;
        // Root (both draws): b0 the parameter block (the vertex programme reads the vignette's coordinates), one table t0..t3, s0 and s1 static.
        D3D12_ROOT_PARAMETER parameters[2]{};
        parameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;parameters[0].ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
        const D3D12_DESCRIPTOR_RANGE textures{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,4,0,0,0};
        parameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[1].DescriptorTable={1,&textures};
        parameters[1].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        const D3D12_STATIC_SAMPLER_DESC samplers[2]{Sampler(0,D3D12_FILTER_MIN_MAG_MIP_LINEAR),Sampler(1,D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT)};
        const D3D12_ROOT_SIGNATURE_DESC root{2,parameters,2,samplers,D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS|
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS|D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS};
        ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&root,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"post tone root serialization");
        Check(Pipelines().Root(device,blob->GetBufferPointer(),blob->GetBufferSize(),root_.ReleaseAndGetAddressOf()),"post tone root");
        Check(Pipelines().Graphics(device,Pipeline(root_.Get(),post_tone_ps_dxbc,sizeof post_tone_ps_dxbc,target_format),pso_.ReleaseAndGetAddressOf()),"post tone pipeline");
        Check(Pipelines().Graphics(device,Pipeline(root_.Get(),post_tone_reinhard_ps_dxbc,sizeof post_tone_reinhard_ps_dxbc,target_format),reinhard_pso_.ReleaseAndGetAddressOf()),"post tone Reinhard pipeline");
        stride_=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        // The 4x4 dither texture, filled from an upload buffer by the first Record.
        D3D12_HEAP_PROPERTIES defaults{};defaults.Type=D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC texture{};texture.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;texture.Width=4;texture.Height=4;
        texture.DepthOrArraySize=texture.MipLevels=1;texture.Format=DXGI_FORMAT_R8G8B8A8_UNORM;texture.SampleDesc.Count=1;
        Check(CountedResource(device)->CreateCommittedResource(&defaults,D3D12_HEAP_FLAG_NONE,&texture,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&dither_)),"post tone dither");
        D3D12_HEAP_PROPERTIES upload{};upload.Type=D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=4*D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;buffer.Height=1;
        buffer.DepthOrArraySize=buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Check(CountedResource(device)->CreateCommittedResource(&upload,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&dither_upload_)),"post tone dither upload");
        void* memory{};const D3D12_RANGE none{};Check(dither_upload_->Map(0,&none,&memory),"post tone dither map");
        for(uint32_t row=0;row<4;++row)std::memcpy(static_cast<uint8_t*>(memory)+row*D3D12_TEXTURE_DATA_PITCH_ALIGNMENT,dither.data()+16*row,16);
        dither_upload_->Unmap(0,nullptr);
        return true;
    }catch(const std::exception& e){reason=e.what();Close();return false;}
}
bool PostToneStage::OpenLut(std::string& reason){
    if(lut_pso_)return true;
    try{
        if(!IsOpen())throw std::invalid_argument("post tone LUT inputs");
        // The copy (cs_lut_copy, in GX's LutCopy2DTo3DCs's place): one table, t0 the 2D strip and u0 the volume.
        const D3D12_DESCRIPTOR_RANGE copy[2]{{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,1}};
        D3D12_ROOT_PARAMETER table{};table.ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;table.DescriptorTable={2,copy};table.ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
        const D3D12_ROOT_SIGNATURE_DESC root{1,&table,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
        ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&root,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"post tone LUT copy root serialization");
        Check(Pipelines().Root(device_.Get(),blob->GetBufferPointer(),blob->GetBufferSize(),copy_root_.ReleaseAndGetAddressOf()),"post tone LUT copy root");
        D3D12_COMPUTE_PIPELINE_STATE_DESC cs{};cs.pRootSignature=copy_root_.Get();cs.CS={post_tone_lut_copy_cs_dxbc,sizeof post_tone_lut_copy_cs_dxbc};
        Check(Pipelines().Compute(device_.Get(),cs,copy_pso_.ReleaseAndGetAddressOf()),"post tone LUT copy pipeline");
        D3D12_HEAP_PROPERTIES defaults{};defaults.Type=D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC volume{};volume.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE3D;volume.Width=volume.Height=LutEdge;volume.DepthOrArraySize=LutEdge;
        volume.MipLevels=1;volume.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;volume.SampleDesc.Count=1;volume.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        Check(CountedResource(device_)->CreateCommittedResource(&defaults,D3D12_HEAP_FLAG_NONE,&volume,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,nullptr,IID_PPV_ARGS(&volume_)),"post tone LUT volume");
        const D3D12_DESCRIPTOR_HEAP_DESC heap{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,1,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
        Check(CountedHeap(device_)->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&rtv_)),"post tone intermediate view heap");
        MakeIntermediate();
        Check(Pipelines().Graphics(device_.Get(),Pipeline(root_.Get(),post_tone_lut_ps_dxbc,sizeof post_tone_lut_ps_dxbc,format_),lut_pso_.ReleaseAndGetAddressOf()),"post tone LUT pipeline");
        return true;
    }catch(const std::exception& e){reason=e.what();copy_root_.Reset();copy_pso_.Reset();lut_pso_.Reset();volume_.Reset();intermediate_.Reset();rtv_.Reset();return false;}
}
bool PostToneStage::OpenHdr(DXGI_FORMAT format,std::string& reason){
    if(hdr_pso_)return true;
    try{
        if(!IsOpen() || format==DXGI_FORMAT_UNKNOWN)throw std::invalid_argument("post tone HDR inputs");
        Check(Pipelines().Graphics(device_.Get(),Pipeline(root_.Get(),post_tone_hdr_ps_dxbc,sizeof post_tone_hdr_ps_dxbc,format),hdr_pso_.ReleaseAndGetAddressOf()),"post tone HDR pipeline");
        Check(Pipelines().Graphics(device_.Get(),Pipeline(root_.Get(),post_tone_hdr_reinhard_ps_dxbc,sizeof post_tone_hdr_reinhard_ps_dxbc,format),hdr_reinhard_pso_.ReleaseAndGetAddressOf()),"post tone HDR Reinhard pipeline");
        hdr_format_=format;
        return true;
    }catch(const std::exception& e){reason=e.what();hdr_pso_.Reset();hdr_reinhard_pso_.Reset();return false;}
}
bool PostToneStage::OpenLook(uint32_t look,bool hdr,std::string& reason){
    if(!look || look>kLooks){reason="post tone look unknown";return false;}
    auto& pso=(hdr?hdr_look_pso_:look_pso_)[look-1];
    if(pso)return true;
    try{
        if(!IsOpen() || (hdr && !hdr_pso_))throw std::invalid_argument("post tone look before its stage");
        struct Entry {const BYTE* bytes;size_t size;};
        const Entry sdr[kLooks]{{post_tone_agx_ps_dxbc,sizeof post_tone_agx_ps_dxbc},{post_tone_aces_ps_dxbc,sizeof post_tone_aces_ps_dxbc},{post_tone_er_ps_dxbc,sizeof post_tone_er_ps_dxbc}};
        const Entry float_scene[kLooks]{{post_tone_hdr_agx_ps_dxbc,sizeof post_tone_hdr_agx_ps_dxbc},{post_tone_hdr_aces_ps_dxbc,sizeof post_tone_hdr_aces_ps_dxbc},
            {post_tone_hdr_er_ps_dxbc,sizeof post_tone_hdr_er_ps_dxbc}};
        const auto& entry=(hdr?float_scene:sdr)[look-1];
        Check(Pipelines().Graphics(device_.Get(),Pipeline(root_.Get(),entry.bytes,entry.size,hdr?hdr_format_:format_),pso.ReleaseAndGetAddressOf()),"post tone look pipeline");
        return true;
    }catch(const std::exception& e){reason=e.what();pso.Reset();return false;}
}
void PostToneStage::MakeIntermediate(){
    D3D12_HEAP_PROPERTIES defaults{};defaults.Type=D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=width_;desc.Height=height_;desc.DepthOrArraySize=desc.MipLevels=1;
    desc.Format=format_;desc.SampleDesc.Count=1;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    intermediate_.Reset();
    Check(CountedResource(device_)->CreateCommittedResource(&defaults,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,IID_PPV_ARGS(&intermediate_)),"post tone intermediate");
    device_->CreateRenderTargetView(intermediate_.Get(),nullptr,rtv_->GetCPUDescriptorHandleForHeapStart());
}
void PostToneStage::Resize(uint32_t width,uint32_t height){
    if(!IsOpen() || !width || !height)throw std::invalid_argument("post tone stage resize");
    width_=width;height_=height;
    if(intermediate_){MakeIntermediate();intermediate_view_=nullptr;}   // its view is written again by the next Record with a LUT
}
void PostToneStage::Record(ID3D12GraphicsCommandList* list,ID3D12Resource* scene,uint64_t scene_generation,D3D12_CPU_DESCRIPTOR_HANDLE target_rtv,
    const PostToneParams& params,const PostToneConstantSink& constants,ID3D12Resource* glare,uint64_t glare_generation,ID3D12Resource* adapted,const PostToneLut* lut,bool hdr){
    if(!IsOpen() || !list || !scene || !target_rtv.ptr || !constants || (hdr && !hdr_pso_))throw std::logic_error("post tone pass before the stage");
    const uint32_t look=(params.flags&kPostToneLookMask)>>kPostToneLookShift;   // I-0138 tranche 6: a candidate's curve in the record's place
    ID3D12PipelineState* const look_pso=look && look<=kLooks?(hdr?hdr_look_pso_:look_pso_)[look-1].Get():nullptr;
    if(look && !look_pso)throw std::logic_error("post tone look before its pipeline");
    ID3D12Resource* strip=lut?lut->strip:nullptr;
    if(lut){
        if(!strip || !lut_pso_)throw std::logic_error("post tone LUT before its side is open");
        const auto desc=strip->GetDesc();
        if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D || desc.Width!=LutEdge || desc.Height!=LutEdge*LutEdge)throw std::invalid_argument("post tone LUT is not 16 by 256");}
    if(!dither_ready_){
        D3D12_TEXTURE_COPY_LOCATION to{};to.pResource=dither_.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        D3D12_TEXTURE_COPY_LOCATION from{};from.pResource=dither_upload_.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        from.PlacedFootprint.Footprint={DXGI_FORMAT_R8G8B8A8_UNORM,4,4,1,D3D12_TEXTURE_DATA_PITCH_ALIGNMENT};
        list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        const auto barrier=Transition(dither_.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);list->ResourceBarrier(1,&barrier);dither_ready_=true;}
    // The strip is a loaded texture record's, immutable under one (resource, generation) (I-0120 F4): its copy into the volume is
    // made when that pair changes, not every frame. A frame without a LUT leaves the volume as it is.
    const bool copy=strip && (strip!=strip_view_ || lut->generation!=strip_view_generation_);
    if(!views_ || copy || (lut && intermediate_.Get()!=intermediate_view_) || scene!=scene_view_ || scene_generation!=scene_view_generation_ ||
       glare!=glare_view_ || glare_generation!=glare_view_generation_ || adapted!=adapted_view_){
        // The views into a fresh heap, the former heap retired by ticket (the frames in flight that read it keep it): a change of
        // the scene, the glare surface or the strip drains nothing.
        ComPtr<ID3D12DescriptorHeap> fresh;const D3D12_DESCRIPTOR_HEAP_DESC heap{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,kViews,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
        Check(CountedHeap(device_.Get())->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&fresh)),"post tone view heap");
        const auto at=[&](uint32_t i){auto h=fresh->GetCPUDescriptorHandleForHeapStart();h.ptr+=SIZE_T(stride_)*i;return h;};
        D3D12_SHADER_RESOURCE_VIEW_DESC view{};view.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;view.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
        view.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;view.Texture2D.MipLevels=1;device_->CreateShaderResourceView(scene,&view,at(0));
        view.Format=DXGI_FORMAT_R11G11B10_FLOAT;device_->CreateShaderResourceView(glare,&view,at(1));   // null without a glare (samples 0)
        view.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;device_->CreateShaderResourceView(adapted,&view,at(2));   // null without a factor (not read)
        device_->CreateShaderResourceView(dither_.Get(),nullptr,at(3));
        view.Format=format_;device_->CreateShaderResourceView(intermediate_.Get(),&view,at(kLutViews));   // null before the LUT's side opens
        D3D12_SHADER_RESOURCE_VIEW_DESC volume{};volume.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;volume.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE3D;
        volume.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;volume.Texture3D.MipLevels=1;device_->CreateShaderResourceView(volume_.Get(),&volume,at(kLutViews+1));
        view.Format=DXGI_FORMAT_R8G8B8A8_UNORM;device_->CreateShaderResourceView(nullptr,&view,at(kLutViews+2));device_->CreateShaderResourceView(nullptr,&view,at(kLutViews+3));
        view.Format=copy?lut->format:DXGI_FORMAT_R8G8B8A8_UNORM;device_->CreateShaderResourceView(copy?strip:nullptr,&view,at(kCopyViews));   // read by this frame's copy only
        // The HDR draw's table (I-0138 tranche 5).
        view.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;device_->CreateShaderResourceView(scene,&view,at(kHdrViews));
        view.Format=DXGI_FORMAT_R11G11B10_FLOAT;device_->CreateShaderResourceView(glare,&view,at(kHdrViews+1));
        view.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;device_->CreateShaderResourceView(adapted,&view,at(kHdrViews+2));
        device_->CreateShaderResourceView(volume_.Get(),&volume,at(kHdrViews+3));   // null before the LUT's side opens (not read without the flag)
        if(volume_){D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};uav.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;uav.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE3D;uav.Texture3D.WSize=LutEdge;
            device_->CreateUnorderedAccessView(volume_.Get(),nullptr,&uav,at(kCopyViews+1));}
        if(views_)RetireAfterCurrentRecording(HoldObject(views_.Get()));views_=fresh;
        scene_view_=scene;scene_view_generation_=scene_generation;glare_view_=glare;glare_view_generation_=glare_generation;adapted_view_=adapted;
        intermediate_view_=intermediate_.Get();}
    const auto gpu=[&](uint32_t i){auto h=views_->GetGPUDescriptorHandleForHeapStart();h.ptr+=UINT64(stride_)*i;return h;};
    ID3D12DescriptorHeap* heaps[]{views_.Get()};list->SetDescriptorHeaps(1,heaps);
    if(copy){
        {const auto barrier=Transition(volume_.Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);list->ResourceBarrier(1,&barrier);}
        list->SetComputeRootSignature(copy_root_.Get());list->SetPipelineState(copy_pso_.Get());list->SetComputeRootDescriptorTable(0,gpu(kCopyViews));
        list->Dispatch(1,1,LutEdge);
        {const auto barrier=Transition(volume_.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);list->ResourceBarrier(1,&barrier);}
        strip_view_=strip;strip_view_generation_=lut->generation;}
    const auto memory=constants(256);
    if(!memory.cpu || !memory.gpu || memory.gpu%256)throw std::runtime_error("post tone constants");
    PostToneParams block=params;
    block.flags=(params.flags&(kPostToneVignette|kPostToneReinhard))|(adapted?kPostToneAdapted:0u)|(hdr && lut?kPostToneHdrLut:0u)|(lut && !hdr?kPostToneGrainLater:0u);   // grain_later: I-0138 tranche 7
    std::memcpy(memory.cpu,&block,sizeof block);
    // The tone map: into the target, or with a LUT into the intermediate.
    // I-0138 tranche 5: the HDR entry draws once into the target, the LUT read in it.
    const auto tone_rtv=lut && !hdr?rtv_->GetCPUDescriptorHandleForHeapStart():target_rtv;const bool reinhard=(params.flags&kPostToneReinhard)!=0;
    list->SetGraphicsRootSignature(root_.Get());list->SetPipelineState(look_pso?look_pso:hdr?(reinhard?hdr_reinhard_pso_.Get():hdr_pso_.Get()):reinhard?reinhard_pso_.Get():pso_.Get());list->SetGraphicsRootConstantBufferView(0,memory.gpu);
    list->SetGraphicsRootDescriptorTable(1,gpu(hdr?kHdrViews:0u));
    list->IASetVertexBuffers(0,0,nullptr);list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    list->OMSetRenderTargets(1,&tone_rtv,FALSE,nullptr);
    const D3D12_VIEWPORT viewport{0,0,float(width_),float(height_),0,1};const D3D12_RECT scissor{0,0,LONG(width_),LONG(height_)};
    list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
    list->DrawInstanced(3,1,0,0);++frames_;lut_block_=lut && !hdr?memory.gpu:0;if(hdr)++hdr_frames_;
}
void PostToneStage::RecordLut(ID3D12GraphicsCommandList* list,D3D12_CPU_DESCRIPTOR_HANDLE target_rtv){
    if(!list || !target_rtv.ptr || !lut_block_ || !lut_pso_ || !views_)throw std::logic_error("post tone LUT draw before its tone map");
    {const auto barrier=Transition(intermediate_.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);list->ResourceBarrier(1,&barrier);}
    ID3D12DescriptorHeap* heaps[]{views_.Get()};list->SetDescriptorHeaps(1,heaps);auto table=views_->GetGPUDescriptorHandleForHeapStart();table.ptr+=UINT64(stride_)*kLutViews;
    list->SetGraphicsRootSignature(root_.Get());list->SetPipelineState(lut_pso_.Get());list->SetGraphicsRootConstantBufferView(0,lut_block_);
    list->SetGraphicsRootDescriptorTable(1,table);
    list->IASetVertexBuffers(0,0,nullptr);list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    list->OMSetRenderTargets(1,&target_rtv,FALSE,nullptr);
    const D3D12_VIEWPORT viewport{0,0,float(width_),float(height_),0,1};const D3D12_RECT scissor{0,0,LONG(width_),LONG(height_)};
    list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
    list->DrawInstanced(3,1,0,0);
    {const auto barrier=Transition(intermediate_.Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);list->ResourceBarrier(1,&barrier);}
    lut_block_=0;++lut_frames_;
}
void PostToneStage::Close() noexcept {
    root_.Reset();copy_root_.Reset();pso_.Reset();reinhard_pso_.Reset();lut_pso_.Reset();copy_pso_.Reset();hdr_pso_.Reset();hdr_reinhard_pso_.Reset();views_.Reset();rtv_.Reset();dither_.Reset();dither_upload_.Reset();
    for(auto& pso:look_pso_)pso.Reset();for(auto& pso:hdr_look_pso_)pso.Reset();hdr_format_=DXGI_FORMAT_UNKNOWN;
    volume_.Reset();intermediate_.Reset();device_.Reset();
    scene_view_=nullptr;scene_view_generation_=0;glare_view_=nullptr;glare_view_generation_=0;adapted_view_=nullptr;strip_view_=nullptr;strip_view_generation_=0;
    intermediate_view_=nullptr;dither_ready_=false;lut_block_=0;format_=DXGI_FORMAT_UNKNOWN;width_=height_=stride_=0;
}
void PostToneStage::QuarantineToProcessExit() noexcept {
    (void)root_.Detach();(void)copy_root_.Detach();(void)pso_.Detach();(void)reinhard_pso_.Detach();(void)lut_pso_.Detach();(void)copy_pso_.Detach();(void)hdr_pso_.Detach();(void)hdr_reinhard_pso_.Detach();(void)views_.Detach();(void)rtv_.Detach();
    for(auto& pso:look_pso_)(void)pso.Detach();for(auto& pso:hdr_look_pso_)(void)pso.Detach();
    (void)dither_.Detach();(void)dither_upload_.Detach();(void)volume_.Detach();(void)intermediate_.Detach();(void)device_.Detach();
}
}
