#include "overlay_pass.h"
#include "backend.h"
// I-0132 R12h step 2: the overlay's programmes built by fxc (shaders/overlay.hlsl; R12a's final review L4: no compilation at a first frame).
#include "overlay_vs.dxbc.h"
#include "overlay_ps.dxbc.h"
#include "overlay_psscreen.dxbc.h"
#include <cfloat>
#include <cstring>
#include <stdexcept>

namespace ds3native {
using Microsoft::WRL::ComPtr;
namespace {
void Check(HRESULT status,const char* what){if(FAILED(status))throw std::runtime_error(what);}
// The quads as the shader reads them (with R12h's fire mode: a second image's slot, the mode, its rectangle normalised): the rectangle
// in clip space, the texture rectangle normalised, the colour, the image's slot in
// the block (-1 a plain colour) and whether it is a coverage image (its red channel the alpha).
struct GpuQuad {float rect[4];float uv[4];float colour[4];int32_t image;uint32_t coverage;int32_t image2;uint32_t mode;float uv2[4];};   // coverage: 0 colour, 1 coverage, 2 colour from an sRGB view; mode 1 a fire (R12h)
static_assert(sizeof(GpuQuad)==80);
DXGI_FORMAT TextureFormat(overlay::Format f){return f==overlay::Format::Coverage8?DXGI_FORMAT_R8_UNORM:f==overlay::Format::Rgba8?DXGI_FORMAT_R8G8B8A8_UNORM:DXGI_FORMAT_BC7_UNORM;}
}
struct OverlayPass::Block {
    uint32_t index{};std::vector<ComPtr<ID3D12Resource>> resources;
};
OverlayPass::OverlayPass():free_lock_(std::make_shared<std::mutex>()),free_(std::make_shared<std::vector<uint32_t>>()),
    generation_shared_(std::make_shared<std::atomic<uint64_t>>()),generation_(*generation_shared_){}
OverlayPass::~OverlayPass(){Close();}
bool OverlayPass::Open(ID3D12Device* device,DXGI_FORMAT target,std::string& reason,Blend mode){
    const bool screen=mode!=Blend::Alpha;   // both premultiplied (PSScreen)
    Close();
    try{
        if(!device)throw std::invalid_argument("overlay device");
        device_=device;
        // Root: t0 the quads (a root SRV), t1..t16 the block's images (a table), s0 linear clamp, s1 linear repeating (R12h's fire noise).
        D3D12_DESCRIPTOR_RANGE range{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,uint32_t(overlay::Frame::kImages),1,0,0};
        D3D12_ROOT_PARAMETER parameters[2]{};
        parameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;parameters[0].Descriptor={0,0};parameters[0].ShaderVisibility=D3D12_SHADER_VISIBILITY_VERTEX;
        parameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[1].DescriptorTable={1,&range};parameters[1].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        D3D12_STATIC_SAMPLER_DESC samplers[2]{};auto& sampler=samplers[0];sampler.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;sampler.MaxAnisotropy=1;
        sampler.ComparisonFunc=D3D12_COMPARISON_FUNC_NEVER;sampler.MinLOD=0;sampler.MaxLOD=FLT_MAX;sampler.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        samplers[1]=sampler;samplers[1].AddressU=samplers[1].AddressV=samplers[1].AddressW=D3D12_TEXTURE_ADDRESS_MODE_WRAP;samplers[1].ShaderRegister=1;   // R12h: s1 repeating
        const D3D12_ROOT_SIGNATURE_DESC root{2,parameters,2,samplers,D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS|
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS|D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS};
        ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&root,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"overlay root serialization");
        Check(Pipelines().Root(device,blob->GetBufferPointer(),blob->GetBufferSize(),root_.ReleaseAndGetAddressOf()),"overlay root");
        vs_={overlay_vs_dxbc,sizeof(overlay_vs_dxbc)};ps_=screen?D3D12_SHADER_BYTECODE{overlay_psscreen_dxbc,sizeof(overlay_psscreen_dxbc)}:D3D12_SHADER_BYTECODE{overlay_ps_dxbc,sizeof(overlay_ps_dxbc)};mode_=mode;MakePipeline(target,pipeline_,"overlay pipeline");
        D3D12_DESCRIPTOR_HEAP_DESC heap{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,kBlocks*uint32_t(overlay::Frame::kImages),D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
        Check(CountedHeap(device)->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&heap_)),"overlay heap");
        stride_=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        {std::lock_guard guard(*free_lock_);free_->clear();for(uint32_t i=kBlocks;i-->0;)free_->push_back(i);}
        return true;
    }catch(const std::exception& e){reason=e.what();Close();return false;}
}
void OverlayPass::MakePipeline(DXGI_FORMAT target,ComPtr<ID3D12PipelineState>& out,const char* what){
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};pso.pRootSignature=root_.Get();
    pso.VS=vs_;pso.PS=ps_;
    auto& blend=pso.BlendState.RenderTarget[0];blend.BlendEnable=TRUE;
    blend.SrcBlend=D3D12_BLEND_SRC_ALPHA;blend.DestBlend=D3D12_BLEND_INV_SRC_ALPHA;blend.BlendOp=D3D12_BLEND_OP_ADD;
    blend.SrcBlendAlpha=D3D12_BLEND_ONE;blend.DestBlendAlpha=D3D12_BLEND_INV_SRC_ALPHA;blend.BlendOpAlpha=D3D12_BLEND_OP_ADD;
    if(mode_==Blend::Multiply){blend.SrcBlend=D3D12_BLEND_DEST_COLOR;blend.DestBlend=D3D12_BLEND_INV_SRC_ALPHA;blend.SrcBlendAlpha=D3D12_BLEND_ZERO;blend.DestBlendAlpha=D3D12_BLEND_ONE;}
    else if(mode_==Blend::Screen){blend.SrcBlend=D3D12_BLEND_INV_DEST_COLOR;blend.DestBlend=D3D12_BLEND_ONE;blend.SrcBlendAlpha=D3D12_BLEND_ZERO;blend.DestBlendAlpha=D3D12_BLEND_ONE;}
    blend.LogicOp=D3D12_LOGIC_OP_NOOP;blend.RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;
    pso.SampleMask=UINT_MAX;pso.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;pso.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;
    pso.RasterizerState.DepthClipEnable=FALSE;pso.DepthStencilState.DepthEnable=FALSE;pso.DepthStencilState.StencilEnable=FALSE;
    pso.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;pso.NumRenderTargets=1;pso.RTVFormats[0]=target;pso.SampleDesc.Count=1;
    Check(Pipelines().Graphics(device_.Get(),pso,out.ReleaseAndGetAddressOf()),what);
}
void OverlayPass::Set(std::shared_ptr<const overlay::Frame> frame){
    if(frame && !overlay::Valid(*frame)){++refused_;frame.reset();}
    std::lock_guard guard(lock_);pending_=std::move(frame);}
std::shared_ptr<const overlay::Frame> OverlayPass::Take(uint32_t width,uint32_t height){
    std::shared_ptr<const overlay::Frame> frame;{std::lock_guard guard(lock_);frame=pending_;}
    if(!frame || frame->quads.empty())return nullptr;
    if(frame->width!=width || frame->height!=height){++skipped_size_;return nullptr;}
    return frame;}
std::shared_ptr<OverlayPass::Block> OverlayPass::TakeBlock(){
    uint32_t index{};
    {std::lock_guard guard(*free_lock_);if(free_->empty())return nullptr;index=free_->back();free_->pop_back();}
    // The final review's L6: a block of an earlier opening never returns its index to the free list of a later one.
    const std::weak_ptr<std::mutex> lock=free_lock_;const std::weak_ptr<std::vector<uint32_t>> free=free_;
    const std::weak_ptr<std::atomic<uint64_t>> generation=generation_shared_;const uint64_t made=generation_.load();
    return std::shared_ptr<Block>(new Block{index,{}},[lock,free,generation,made](Block* block){
        if(const auto l=lock.lock())if(const auto f=free.lock())if(const auto g=generation.lock()){std::lock_guard guard(*l);if(g->load()==made)f->push_back(block->index);}
        delete block;});}
uint32_t OverlayPass::Record(ID3D12GraphicsCommandList* list,D3D12_CPU_DESCRIPTOR_HANDLE target,uint32_t width,uint32_t height,
    const std::shared_ptr<const overlay::Frame>& frame,const Allocate& allocate,std::vector<std::shared_ptr<const void>>& kept,bool float_target){
    if(!IsOpen() || !list || !target.ptr || !frame || frame->quads.empty() || frame->width!=width || frame->height!=height)return 0;
    if(float_target && !pipeline_float_){   // I-0138 tranche 5: an HDR10 chain's float picture, its pipeline made at its first frame
        if(float_failed_)return 0;
        try{MakePipeline(DXGI_FORMAT_R16G16B16A16_FLOAT,pipeline_float_,"overlay pipeline (float target)");}catch(const std::exception& e){pipeline_float_.Reset();float_failure_=e.what();float_failed_=true;return 0;}}
    // The images: a texture made and copied for each new (identity, version).
    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    for(const auto& image:frame->images){
        auto& texture=textures_[image.identity];
        if(image.external){   // R12a's second tranche: a texture the native holds, bound as it is (no upload, no barrier)
            auto* const resource=static_cast<ID3D12Resource*>(image.external);
            if(texture.resource.Get()!=resource){const auto desc=resource->GetDesc();texture={image.version,image.width,image.height,image.format,resource};texture.view_format=desc.Format;}
            continue;}
        if(texture.resource && texture.version==image.version && texture.width==image.width && texture.height==image.height && texture.format==image.format)continue;
        D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=image.width;desc.Height=image.height;
        desc.DepthOrArraySize=desc.MipLevels=1;desc.Format=TextureFormat(image.format);desc.SampleDesc.Count=1;
        D3D12_HEAP_PROPERTIES properties{};properties.Type=D3D12_HEAP_TYPE_DEFAULT;ComPtr<ID3D12Resource> resource;
        Check(CountedResource(device_.Get())->CreateCommittedResource(&properties,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&resource)),"overlay image");
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT rows{};UINT64 total{};
        device_->GetCopyableFootprints(&desc,0,1,0,&footprint,&rows,nullptr,&total);
        D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=total;buffer.Height=1;buffer.DepthOrArraySize=buffer.MipLevels=1;
        buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;D3D12_HEAP_PROPERTIES upload{};upload.Type=D3D12_HEAP_TYPE_UPLOAD;
        ComPtr<ID3D12Resource> staging;
        Check(CountedResource(device_.Get())->CreateCommittedResource(&upload,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&staging)),"overlay staging");
        void* memory{};const D3D12_RANGE none{};Check(staging->Map(0,&none,&memory),"overlay staging map");
        const auto& bytes=*image.bytes;const size_t packed=size_t(overlay::ImageBytes(image.width,1,image.format));   // one row of pixels (or of blocks)
        const size_t source_row=image.format==overlay::Format::Bc7?size_t((image.width+3)/4)*16:packed;
        for(UINT r=0;r<rows;++r)std::memcpy(static_cast<uint8_t*>(memory)+footprint.Offset+size_t(r)*footprint.Footprint.RowPitch,bytes.data()+size_t(r)*source_row,source_row);
        staging->Unmap(0,nullptr);
        D3D12_TEXTURE_COPY_LOCATION from{},to{};from.pResource=staging.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;from.PlacedFootprint=footprint;
        to.pResource=resource.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;to.SubresourceIndex=0;
        list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition={resource.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE};barriers.push_back(barrier);
        kept.push_back(HoldResource(staging.Get()));kept.push_back(HoldResource(resource.Get()));   // the step review's L2
        texture={image.version,image.width,image.height,image.format,resource};++uploads_;upload_bytes_+=bytes.size();}
    if(!barriers.empty())list->ResourceBarrier(UINT(barriers.size()),barriers.data());
    // The block: a new one when the frame's images (or their versions) differ from the current block's.
    std::vector<std::pair<uint64_t,uint64_t>> wanted;wanted.reserve(frame->images.size());
    for(const auto& image:frame->images)wanted.emplace_back(image.identity,image.version);
    if(!block_ || wanted!=block_images_){
        if(auto block=TakeBlock()){
            auto handle=heap_->GetCPUDescriptorHandleForHeapStart();handle.ptr+=size_t(block->index)*overlay::Frame::kImages*stride_;
            for(size_t slot=0;slot<overlay::Frame::kImages;++slot){
                D3D12_SHADER_RESOURCE_VIEW_DESC view{};view.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;view.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                view.Texture2D.MipLevels=1;D3D12_CPU_DESCRIPTOR_HANDLE at{handle.ptr+slot*stride_};
                if(slot<frame->images.size() && frame->images[slot].external){const auto& t=textures_[frame->images[slot].identity];   // L3: an external texture's mips
                    view.Texture2D.MipLevels=t.resource->GetDesc().MipLevels;}
                if(slot<frame->images.size()){const auto& t=textures_[frame->images[slot].identity];view.Format=t.view_format!=DXGI_FORMAT_UNKNOWN?t.view_format:TextureFormat(t.format);
                    device_->CreateShaderResourceView(t.resource.Get(),&view,at);block->resources.push_back(t.resource);}
                else{view.Format=DXGI_FORMAT_R8_UNORM;device_->CreateShaderResourceView(nullptr,&view,at);}}   // a null view: reads 0
            block_=std::move(block);block_images_=std::move(wanted);}
        else if(!block_){++stale_blocks_;return 0;}
        else ++stale_blocks_;   // every block in flight: the previous images this frame (said in the counts)
    }
    // The quads, in clip space, normalised texture coordinates, coverage per image.
    const size_t count=frame->quads.size();const auto [cpu,gpu]=allocate(count*sizeof(GpuQuad));
    auto* out=reinterpret_cast<GpuQuad*>(cpu);
    const float sx=2.f/float(width),sy=2.f/float(height);
    for(size_t i=0;i<count;++i){const auto& q=frame->quads[i];GpuQuad g{};
        g.rect[0]=q.x0*sx-1;g.rect[1]=1-q.y0*sy;g.rect[2]=q.x1*sx-1;g.rect[3]=1-q.y1*sy;
        if(q.image>=0 && size_t(q.image)<frame->images.size()){const auto& image=frame->images[size_t(q.image)];
            g.uv[0]=q.u0/float(image.width);g.uv[1]=q.v0/float(image.height);g.uv[2]=q.u1/float(image.width);g.uv[3]=q.v1/float(image.height);
            const auto& texture=textures_[image.identity];const DXGI_FORMAT f=texture.view_format;
            const bool srgb=f==DXGI_FORMAT_BC7_UNORM_SRGB || f==DXGI_FORMAT_BC1_UNORM_SRGB || f==DXGI_FORMAT_BC2_UNORM_SRGB || f==DXGI_FORMAT_BC3_UNORM_SRGB ||
                f==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB || f==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB || f==DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;   // re-encoded (not witnessed live: both pieces are BC7_UNORM)
            g.image=q.image;g.coverage=image.external?(srgb?2u:0u):image.format==overlay::Format::Coverage8?1u:0u;}
        else g.image=-1;
        g.image2=-1;g.mode=0;   // R12h: a fire's second image, its rectangle normalised by its own size
        if(q.mode==1 && g.image>=0 && q.image2>=0 && size_t(q.image2)<frame->images.size()){const auto& second=frame->images[size_t(q.image2)];
            g.image2=q.image2;g.mode=1;g.uv2[0]=q.u2_0/float(second.width);g.uv2[1]=q.v2_0/float(second.height);g.uv2[2]=q.u2_1/float(second.width);g.uv2[3]=q.v2_1/float(second.height);}
        std::memcpy(g.colour,q.colour.data(),sizeof(g.colour));out[i]=g;}
    const D3D12_VIEWPORT viewport{0,0,float(width),float(height),0,1};const D3D12_RECT scissor{0,0,LONG(width),LONG(height)};
    list->OMSetRenderTargets(1,&target,FALSE,nullptr);
    ID3D12DescriptorHeap* heaps[]{heap_.Get()};list->SetDescriptorHeaps(1,heaps);
    list->SetGraphicsRootSignature(root_.Get());list->SetPipelineState(float_target?pipeline_float_.Get():pipeline_.Get());
    list->SetGraphicsRootShaderResourceView(0,gpu);
    auto table=heap_->GetGPUDescriptorHandleForHeapStart();table.ptr+=uint64_t(block_->index)*overlay::Frame::kImages*stride_;
    list->SetGraphicsRootDescriptorTable(1,table);
    list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
    list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);list->DrawInstanced(UINT(count*6),1,0,0);
    kept.push_back(block_);kept.push_back(frame);
    ++frames_;quads_+=count;return uint32_t(count);
}
OverlayPass::Counts OverlayPass::TakeCounts() noexcept {
    return {frames_.exchange(0),quads_.exchange(0),uploads_.exchange(0),upload_bytes_.exchange(0),refused_.exchange(0),skipped_size_.exchange(0),stale_blocks_.exchange(0)};}
void OverlayPass::Close() noexcept {
    // The final review's M1: the engine thread's latest frame stays (a screen set once is drawn again after a reopen), unless it names
    // an external texture (R12a's second tranche's final review L3: another device's resource after a reopen): then it is dropped.
    {std::lock_guard guard(lock_);if(pending_)for(const auto& image:pending_->images)if(image.external){pending_.reset();break;}}
    {std::lock_guard guard(*free_lock_);++generation_;}   // under the free list's lock, as the deleters compare it
    block_.reset();block_images_.clear();textures_.clear();root_.Reset();pipeline_.Reset();pipeline_float_.Reset();vs_={};ps_={};float_failed_=false;float_failure_.clear();heap_.Reset();device_.Reset();}
void OverlayPass::QuarantineToProcessExit() noexcept {
    (void)new std::shared_ptr<Block>(std::move(block_));
    for(auto& [identity,texture]:textures_)(void)texture.resource.Detach();
    (void)root_.Detach();(void)pipeline_.Detach();(void)pipeline_float_.Detach();(void)heap_.Detach();(void)device_.Detach();}
}
