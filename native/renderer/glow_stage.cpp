#include "glow_stage.h"
#include "backend.h"
#include "glow_math.h"
#include <cfloat>
#include <cstring>
#include <stdexcept>

namespace ds3native {
using Microsoft::WRL::ComPtr;
namespace {
void Check(HRESULT status,const char* what){if(FAILED(status))throw std::runtime_error(what);}
// GLOW-WAVE1-READY 1.2: the quad of FUN_14007ECB0 (post+0xB58, stride 0x18): a clip-space strip, (x, y, z, w, u, v).
constexpr float quad_vertices[24]{-1,1,1,1,0,0, 1,1,1,1,1,0, -1,-1,1,1,0,1, 1,-1,1,1,1,1};
enum Block : uint32_t {Taps,Filter0,Filter1,Filter2,Filter3,GlowRgb,GlowFinal,Zero,BlockCount};
}
// One pipeline of the stage: the screen quad's vertex programme (POSITION float4 at 0, TEXCOORD float2 at 16) and `pixel`.
void GlowStage::MakePipeline(const GlowBlobs& blobs,const std::vector<uint8_t>& pixel,DXGI_FORMAT format,bool additive,ComPtr<ID3D12PipelineState>& out,const char* what){
    const D3D12_INPUT_ELEMENT_DESC layout[2]{{"POSITION",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
        {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,16,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}};
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};pso.pRootSignature=root_.Get();pso.InputLayout={layout,2};
    pso.VS={blobs.vertex.data(),blobs.vertex.size()};pso.PS={pixel.data(),pixel.size()};
    auto& target=pso.BlendState.RenderTarget[0];target.LogicOp=D3D12_LOGIC_OP_NOOP;target.RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;
    // EMISSIVE-GLOW 1 (observed state 17): the composites add, colour SRC_ALPHA + ONE, alpha ONE + ZERO.
    target.BlendEnable=additive;target.SrcBlend=additive?D3D12_BLEND_SRC_ALPHA:D3D12_BLEND_ONE;target.DestBlend=additive?D3D12_BLEND_ONE:D3D12_BLEND_ZERO;
    target.BlendOp=D3D12_BLEND_OP_ADD;target.SrcBlendAlpha=D3D12_BLEND_ONE;target.DestBlendAlpha=D3D12_BLEND_ZERO;target.BlendOpAlpha=D3D12_BLEND_OP_ADD;
    pso.SampleMask=UINT_MAX;pso.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;pso.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;
    pso.RasterizerState.DepthClipEnable=TRUE;pso.DepthStencilState.DepthEnable=FALSE;pso.DepthStencilState.StencilEnable=FALSE;
    pso.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;pso.NumRenderTargets=1;pso.RTVFormats[0]=format;pso.SampleDesc.Count=1;
    Check(Pipelines().Graphics(device_.Get(),pso,out.ReleaseAndGetAddressOf()),what);
}
bool GlowStage::FloatFinal(const GlowBlobs& blobs,std::string& reason){
    if(final_float_)return true;
    try{if(!IsOpen())throw std::logic_error("glow float final before the stage");
        MakePipeline(blobs,blobs.composite,FloatFinalFormat(),true,final_float_,"glow final composite pipeline (float)");return true;}
    catch(const std::exception& e){reason=e.what();final_float_.Reset();return false;}
}
bool GlowStage::Open(ID3D12Device* device,const GlowBlobs& blobs,DXGI_FORMAT final_format,uint32_t width,uint32_t height,std::string& reason){
    Close();
    try{
        if(!device || blobs.vertex.empty() || blobs.extract.empty() || blobs.blur.empty() || blobs.composite.empty() ||
           final_format==DXGI_FORMAT_UNKNOWN || !width || !height)throw std::invalid_argument("glow stage inputs");
        device_=device;width_=width;height_=height;
        // Root: b0 cbFilterParam, b1 cbSampleOffset, b2 cbGlowParam (pixel root CBVs), t0 the source (one-descriptor table),
        // s3 SS_ClampLinear as a static sampler (trilinear, clamp, the full LOD range; the shaders sample level 0).
        D3D12_ROOT_PARAMETER parameters[4]{};
        for(UINT i=0;i<3;++i){parameters[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;parameters[i].Descriptor.ShaderRegister=i;
            parameters[i].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;}
        const D3D12_DESCRIPTOR_RANGE source{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,0,0,0};
        parameters[3].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[3].DescriptorTable={1,&source};
        parameters[3].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        D3D12_STATIC_SAMPLER_DESC sampler{};sampler.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;sampler.MaxAnisotropy=1;
        sampler.ComparisonFunc=D3D12_COMPARISON_FUNC_NEVER;sampler.MinLOD=-FLT_MAX;sampler.MaxLOD=FLT_MAX;sampler.ShaderRegister=3;
        sampler.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        const D3D12_ROOT_SIGNATURE_DESC root{4,parameters,1,&sampler,D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT|
            D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS|D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS|
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS|D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS};
        ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&root,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"glow root serialization");
        Check(Pipelines().Root(device,blob->GetBufferPointer(),blob->GetBufferSize(),root_.ReleaseAndGetAddressOf()),"glow root");
        MakePipeline(blobs,blobs.extract,TargetFormat(),false,extract_,"glow extraction pipeline");
        MakePipeline(blobs,blobs.blur,TargetFormat(),false,blur_,"glow blur pipeline");
        MakePipeline(blobs,blobs.composite,TargetFormat(),true,level_,"glow level composite pipeline");
        MakePipeline(blobs,blobs.composite,final_format,true,final_,"glow final composite pipeline");
        D3D12_DESCRIPTOR_HEAP_DESC heap{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,5,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
        Check(CountedHeap(device)->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&rtv_)),"glow RTV heap");
        heap={D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,6,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};   // GB5, S, L0..L3
        Check(CountedHeap(device)->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&srv_)),"glow SRV heap");
        rtv_stride_=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        srv_stride_=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=sizeof(quad_vertices);buffer.Height=1;
        buffer.DepthOrArraySize=buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        D3D12_HEAP_PROPERTIES upload{};upload.Type=D3D12_HEAP_TYPE_UPLOAD;
        Check(CountedResource(device)->CreateCommittedResource(&upload,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&quad_)),"glow quad");
        void* memory{};const D3D12_RANGE none{};Check(quad_->Map(0,&none,&memory),"glow quad map");
        std::memcpy(memory,quad_vertices,sizeof(quad_vertices));quad_->Unmap(0,nullptr);
        MakeTargets();return true;
    }catch(const std::exception& e){reason=e.what();Close();return false;}
}
void GlowStage::MakeTargets(){
    D3D12_HEAP_PROPERTIES properties{};properties.Type=D3D12_HEAP_TYPE_DEFAULT;
    for(uint32_t i=0;i<targets_.size();++i){
        const auto size=i?glow::LevelSize(width_,height_,i-1):glow::Size{width_,height_};
        D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=size.width;desc.Height=size.height;
        desc.DepthOrArraySize=desc.MipLevels=1;desc.Format=TargetFormat();desc.SampleDesc.Count=1;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        targets_[i].Reset();   // never cleared: every pass writes every texel of its target, so no clear value
        Check(CountedResource(device_.Get())->CreateCommittedResource(&properties,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,
            IID_PPV_ARGS(&targets_[i])),"glow target");
        auto rtv=rtv_->GetCPUDescriptorHandleForHeapStart();rtv.ptr+=SIZE_T(rtv_stride_)*i;device_->CreateRenderTargetView(targets_[i].Get(),nullptr,rtv);
        auto srv=srv_->GetCPUDescriptorHandleForHeapStart();srv.ptr+=SIZE_T(srv_stride_)*(i+1);device_->CreateShaderResourceView(targets_[i].Get(),nullptr,srv);
    }
    gb5_view_=nullptr;   // the caller's GB5 is recreated with its targets too: its view is written again at the next frame
}
void GlowStage::Resize(uint32_t width,uint32_t height){
    if(!IsOpen() || !width || !height)throw std::invalid_argument("glow stage resize");
    width_=width;height_=height;MakeTargets();
}
std::array<ID3D12Resource*,5> GlowStage::Targets() const noexcept {
    return {targets_[0].Get(),targets_[1].Get(),targets_[2].Get(),targets_[3].Get(),targets_[4].Get()};}
uint64_t GlowStage::TargetBytes() const noexcept {
    uint64_t bytes{};for(const auto& target:targets_)if(target){const auto desc=target->GetDesc();bytes+=desc.Width*desc.Height*4;}return bytes;}
void GlowStage::Record(ID3D12GraphicsCommandList* list,ID3D12Resource* gb5,DXGI_FORMAT gb5_format,uint64_t gb5_generation,D3D12_CPU_DESCRIPTOR_HANDLE final_rtv,
    const GlowValues& values,const GlowConstantSink& constants,GlowRecorder& recorder,bool float_final){
    if(!IsOpen() || !list || !gb5 || !final_rtv.ptr || !constants || (float_final && !final_float_))throw std::logic_error("glow pass before the stage");
    if(gb5!=gb5_view_ || gb5_format!=gb5_view_format_ || gb5_generation!=gb5_view_generation_){
        D3D12_SHADER_RESOURCE_VIEW_DESC view{};view.Format=gb5_format;view.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
        view.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;view.Texture2D.MipLevels=1;
        device_->CreateShaderResourceView(gb5,&view,srv_->GetCPUDescriptorHandleForHeapStart());gb5_view_=gb5;gb5_view_format_=gb5_format;gb5_view_generation_=gb5_generation;}
    // The frame's constant blocks (GLOW-WAVE1-READY 3.2), one 256-byte block each.
    const auto memory=constants(ConstantBytes);
    if(!memory.cpu || !memory.gpu || memory.gpu%256)throw std::runtime_error("glow constants");
    auto* bytes=static_cast<uint8_t*>(memory.cpu);std::memset(bytes,0,ConstantBytes);
    glow::TapImage(values.blur_scale,bytes+256*Taps);
    for(uint32_t level=0;level<4;++level){const auto size=glow::LevelSize(width_,height_,level);glow::FilterParamImage(size.width,size.height,bytes+256*(Filter0+level));}
    std::memcpy(bytes+256*GlowRgb,&values.rgb_scale,4);std::memcpy(bytes+256*GlowRgb+4,&values.alpha_bits,4);
    std::memcpy(bytes+256*GlowFinal,&values.final_scale,4);std::memcpy(bytes+256*GlowFinal+4,&values.alpha_bits,4);
    const auto block=[&](Block b){return memory.gpu+256ull*b;};
    ID3D12DescriptorHeap* heaps[]{srv_.Get()};list->SetDescriptorHeaps(1,heaps);list->SetGraphicsRootSignature(root_.Get());
    const D3D12_VERTEX_BUFFER_VIEW vertices{quad_->GetGPUVirtualAddress(),sizeof(quad_vertices),24};
    list->IASetVertexBuffers(0,1,&vertices);list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    const auto rtv=[&](uint32_t i){auto handle=rtv_->GetCPUDescriptorHandleForHeapStart();handle.ptr+=SIZE_T(rtv_stride_)*i;return handle;};
    const auto draw=[&](ID3D12PipelineState* pso,uint32_t source_slot,D3D12_CPU_DESCRIPTOR_HANDLE target,uint32_t w,uint32_t h,Block b0,Block b1,Block b2){
        list->OMSetRenderTargets(1,&target,FALSE,nullptr);list->SetPipelineState(pso);
        // 1400C6220/1400C63D0: the viewport and scissor are the render target's size.
        const D3D12_VIEWPORT viewport{0,0,float(w),float(h),0,1};const D3D12_RECT scissor{0,0,LONG(w),LONG(h)};
        list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
        list->SetGraphicsRootConstantBufferView(0,block(b0));list->SetGraphicsRootConstantBufferView(1,block(b1));list->SetGraphicsRootConstantBufferView(2,block(b2));
        auto table=srv_->GetGPUDescriptorHandleForHeapStart();table.ptr+=UINT64(srv_stride_)*source_slot;list->SetGraphicsRootDescriptorTable(3,table);
        list->DrawInstanced(4,1,0,0);++passes_;};
    const auto rt=D3D12_RESOURCE_STATE_RENDER_TARGET,read=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    ID3D12Resource* const s=targets_[0].Get();
    const auto size=[&](uint32_t i){return i?glow::LevelSize(width_,height_,i-1):glow::Size{width_,height_};};
    // Pass 1: extraction, GB5 -> S.
    recorder.Barrier(list,gb5,rt,read);recorder.Pass("glow_extract",gb5,s,false);
    draw(extract_.Get(),0,rtv(0),width_,height_,Zero,Zero,Zero);
    recorder.Barrier(list,gb5,read,rt);   // nothing else reads GB5 this frame
    // Passes 2-5: the 13-tap blurs at one target texel, S -> L0 -> L1 -> L2 -> L3.
    static constexpr const char* blur_names[4]{"glow_blur0","glow_blur1","glow_blur2","glow_blur3"};
    for(uint32_t level=0;level<4;++level){const auto target=size(level+1);
        recorder.Barrier(list,targets_[level].Get(),rt,read);recorder.Pass(blur_names[level],targets_[level].Get(),targets_[level+1].Get(),false);
        draw(blur_.Get(),level+1,rtv(level+1),target.width,target.height,Block(Filter0+level),Taps,Zero);}
    // Passes 6-9: the additive composites up, L3 -> L2 -> L1 -> L0 -> S (Rgb).
    static constexpr const char* up_names[4]{"glow_up2","glow_up1","glow_up0","glow_source"};
    for(uint32_t i=0;i<4;++i){const uint32_t from=4-i,to=3-i;const auto target=size(to);
        recorder.Barrier(list,targets_[from].Get(),rt,read);recorder.Barrier(list,targets_[to].Get(),read,rt);
        recorder.Pass(up_names[i],targets_[from].Get(),targets_[to].Get(),true);
        draw(level_.Get(),from+1,rtv(to),target.width,target.height,Zero,Zero,GlowRgb);}
    // Pass 10: S onto the final colour (Final).
    recorder.Barrier(list,s,rt,read);recorder.Pass("glow_final",s,nullptr,true);
    draw(float_final?final_float_.Get():final_.Get(),1,final_rtv,width_,height_,Zero,Zero,GlowFinal);
    for(const auto& target:targets_)recorder.Barrier(list,target.Get(),read,rt);   // every glow target back to RENDER_TARGET
    ++frames_;
}
void GlowStage::Close() noexcept {
    root_.Reset();extract_.Reset();blur_.Reset();level_.Reset();final_.Reset();final_float_.Reset();rtv_.Reset();srv_.Reset();quad_.Reset();
    for(auto& target:targets_)target.Reset();device_.Reset();gb5_view_=nullptr;gb5_view_format_=DXGI_FORMAT_UNKNOWN;
    width_=height_=0;
}
void GlowStage::QuarantineToProcessExit() noexcept {
    (void)root_.Detach();(void)extract_.Detach();(void)blur_.Detach();(void)level_.Detach();(void)final_.Detach();(void)final_float_.Detach();(void)rtv_.Detach();
    (void)srv_.Detach();(void)quad_.Detach();for(auto& target:targets_)(void)target.Detach();(void)device_.Detach();
}
}
