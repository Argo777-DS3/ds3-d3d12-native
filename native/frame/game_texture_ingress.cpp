#include "game_texture_ingress.h"
#include "../content/gpu_texture_upload.h"
// I-0132 R12h step 2: the FaceGen composition's programmes built by fxc (shaders/facegen_composition.hlsl, facegen_mark.hlsl).
#include "facegen_composition_vs.dxbc.h"
#include "facegen_composition_ps.dxbc.h"
#include "facegen_mark_vs.dxbc.h"
#include "facegen_mark_ps.dxbc.h"
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstring>
#include <stdexcept>
namespace ds3native::bootstrap {
namespace {
constexpr uint64_t Limit=128ull*1024ull*1024ull;
void Require(bool ok,const char* label){if(!ok)throw std::runtime_error(label);}
void Check(HRESULT result,const char* label){Require(SUCCEEDED(result),label);}
bool Read(uintptr_t address,void* output,size_t size) {
    SIZE_T read{};return address && ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void*>(address),output,size,&read) && read==size;
}
struct SourceSubresource {uintptr_t bytes;uint32_t row_pitch,slice_pitch;};
static_assert(sizeof(SourceSubresource)==16);
// B3a: a COM object kept by an ingress list until the submission carrying it completes.
template<class T> std::shared_ptr<void> Held(const Microsoft::WRL::ComPtr<T>& object){return std::make_shared<Microsoft::WRL::ComPtr<T>>(object);}
}
TextureIngress::Admission::Admission(TextureIngress& o):owner(o){
    std::lock_guard<std::shared_mutex> hold(owner.mutex_);Require(!owner.closed_,"native texture admission closed");++owner.admitting_;
}
TextureIngress::Admission::~Admission(){
    std::lock_guard<std::shared_mutex> hold(owner.mutex_);--owner.admitting_;owner.idle_.notify_all();   // under the lock: Close may return at once
}
std::shared_ptr<const NativeTextureRecord> TextureIngress::Admit(uintptr_t receiver,TextureDescription description,uintptr_t source) {
    Require(receiver && source && description.width && description.height && description.width<=16384 && description.height<=16384 && description.slices==1 && !description.cube,"native texture asset description");
    uint32_t maximum=1;for(auto dimension=std::max(description.width,description.height);dimension>1;dimension>>=1)++maximum;
    if(!description.mips)description.mips=maximum;
    Require(description.mips && description.mips<=maximum,"native texture mip extent");
    ds3content::CpuTextureResource owned{};owned.name="engine-asset";owned.width=description.width;owned.height=description.height;
    owned.mip_levels=description.mips;owned.array_slices=1;owned.format=description.format;
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=description.width;desc.Height=description.height;
    desc.DepthOrArraySize=1;desc.MipLevels=static_cast<UINT16>(description.mips);desc.Format=description.format;desc.SampleDesc.Count=1;
    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(description.mips);std::vector<UINT> rows(description.mips);
    std::vector<UINT64> row_bytes(description.mips);UINT64 total{};
    const Admission admission(*this);   // B3a: serialized with Close by the admission count, not the frame lock
    {Require(gpu_.device!=nullptr,"native texture device unavailable");
        gpu_.device->GetCopyableFootprints(&desc,0,description.mips,0,footprints.data(),rows.data(),row_bytes.data(),&total);}
    Require(total && total<=Limit,"native texture upload bound");uint64_t dense_total{};
    for(UINT mip=0;mip<description.mips;++mip) {
        SourceSubresource input{};
        Require(Read(source+SIZE_T(mip)*sizeof(input),&input,sizeof(input)) && input.bytes && input.row_pitch && rows[mip] && row_bytes[mip] && row_bytes[mip]<=input.row_pitch && footprints[mip].Footprint.Depth==1,"native texture source footprint");
        const auto dense=row_bytes[mip]*rows[mip];Require(dense<=Limit-dense_total,"native texture content bound");dense_total+=dense;
        ds3content::CpuTextureSubresource sub{std::max(1u,description.width>>mip),std::max(1u,description.height>>mip),static_cast<uint32_t>(row_bytes[mip]),rows[mip],{}};
        sub.bytes.resize(static_cast<size_t>(dense));
        for(UINT row=0;row<rows[mip];++row) {const auto offset=uint64_t(row)*input.row_pitch;
            Require(offset<=UINTPTR_MAX-input.bytes && Read(input.bytes+static_cast<uintptr_t>(offset),sub.bytes.data()+static_cast<size_t>(row)*sub.row_bytes,sub.row_bytes),"native texture source row unreadable");}
        owned.subresources.push_back(std::move(sub));
    }
    return AdmitOwned(receiver,std::move(owned));   // I-0074: moved, not copied
}
std::shared_ptr<const NativeTextureRecord> TextureIngress::AdmitOwned(uintptr_t receiver,const ds3content::CpuTextureResource& source) {
    return AdmitOwned(receiver,ds3content::CpuTextureResource(source));
}
std::shared_ptr<const NativeTextureRecord> TextureIngress::AdmitOwned(uintptr_t receiver,ds3content::CpuTextureResource&& moved) {
    return AdmitOwnedKept(receiver,std::move(moved),nullptr);
}
std::shared_ptr<const NativeTextureRecord> TextureIngress::AdmitOwned(uintptr_t receiver,ds3content::CpuTextureResource&& moved,const ContentKey& keep) {
    return AdmitOwnedKept(receiver,std::move(moved),&keep);
}
std::shared_ptr<const NativeTextureRecord> TextureIngress::AdmitOwnedKept(uintptr_t receiver,ds3content::CpuTextureResource&& moved,const ContentKey* keep) {
    Require(receiver && ds3content::ValidCpuTextureResource(moved),"native texture owned CPU resource");
    auto candidate=std::make_shared<NativeTextureRecord>();
    // I-0096: the record keeps the image's description (name, extent, format, per-subresource shape, float metadata), not its
    // bytes: they are staged by the upload below and then freed here, as D3D11 frees the initial data of a texture DS3
    // creates (the native backend used to hold every texture's pixels twice, about 0.9 GB more system memory in m31).
    {ds3content::CpuTextureResource described;described.name=moved.name;described.tpf_format_tag=moved.tpf_format_tag;
        described.width=moved.width;described.height=moved.height;described.mip_levels=moved.mip_levels;described.array_slices=moved.array_slices;
        described.format=moved.format;described.cube=moved.cube;described.float_metadata=moved.float_metadata;
        described.subresources.reserve(moved.subresources.size());
        for(const auto& sub:moved.subresources)described.subresources.push_back({sub.width,sub.height,sub.row_bytes,sub.rows,{}});
        candidate->content=std::make_shared<const ds3content::CpuTextureResource>(std::move(described));}
    const ds3content::CpuTextureResource source=std::move(moved);   // freed when this admission returns
    candidate->description={source.width,source.height,source.mip_levels,source.format,source.array_slices,source.cube};
    const Admission admission(*this);
    {std::lock_guard<std::shared_mutex> hold(mutex_);candidate->generation=++generation_;Install(receiver,candidate);}
    Require(gpu_.device && gpu_.queue,"native texture device unavailable");
    // B3a: recorded on the ingress context; the copy precedes every later submission of the queue (no wait here).
    auto recording=gpu_.RecordIngress();
    auto uploaded=ds3content::RecordNativeTextureUpload(gpu_,recording.List(),source);
    candidate->resource=std::move(uploaded.resource);candidate->content_bytes=uploaded.content_bytes;
    candidate->view=gpu_.AllocateCpuDescriptor();   // B3b: pooled CPU descriptor
    const auto view=ds3content::NativeTextureView(source);
    gpu_.device->CreateShaderResourceView(candidate->resource.Get(),&view,candidate->view.handle);
    recording.Keep(Held(candidate->resource));recording.Keep(std::move(uploaded.upload));
    candidate->upload_fence=recording.Commit();
    // D-0211: kept under its key, with its placed size (what it holds of the heaps), unless the same content is kept already.
    uint64_t placed{};
    if(keep){const auto desc=candidate->resource->GetDesc();placed=gpu_.device->GetResourceAllocationInfo(0,1,&desc).SizeInBytes;}
    {std::lock_guard<std::shared_mutex> hold(mutex_);const auto found=records_.find(receiver);
        Require(!closed_ && found!=records_.end() && found->second==candidate,"native texture incarnation withdrawn");candidate->resident=true;
        if(keep){const std::lock_guard kept(kept_lock_);
            if(!kept_closed_ && !kept_.contains(*keep)){
                kept_.emplace(*keep,KeptEntry{{candidate->description,candidate->content,candidate->resource,candidate->content_bytes,candidate->upload_fence,placed},1,kept_retired_.end()});
                kept_placed_bytes_+=placed;candidate->kept_key=std::make_shared<const ContentKey>(*keep);}}
        Changed();}
    FlushKept();   // a content its Install released (final review 7)
    return candidate;
}
std::shared_ptr<const NativeTextureRecord> TextureIngress::AdmitComposedFace(uintptr_t receiver,const NativeTextureRecord& base,uint32_t width,uint32_t height,
    const FaceMarkLayer* mark) {
    Require(receiver && width && height && width<=4096 && height<=4096 && base.resident && base.resource && base.view &&
        !base.description.cube && base.description.slices==1,"native FaceGen composition input");
    Require(!mark || (mark->texture && mark->texture->resident && mark->texture->resource && mark->texture->view && !mark->texture->description.cube &&
        mark->texture->description.slices==1),
        "native FaceGen mark input");
    auto candidate=std::make_shared<NativeTextureRecord>();
    // 835820 creates the target through 2BCE0/2BD50: one mip, DXGI 0x1C, bind
    // flags 0x28 (shader resource and render target); 2D410 keeps 0x1C, so the
    // resource and both views are R8G8B8A8_UNORM. The SRGB base view returns
    // linear values, which the target stores as such.
    candidate->description={width,height,1,DXGI_FORMAT_R8G8B8A8_UNORM,1,false};
    const Admission admission(*this);
    {std::lock_guard<std::shared_mutex> hold(mutex_);candidate->generation=++generation_;Install(receiver,candidate);}
    Require(gpu_.device && gpu_.queue,"native texture device unavailable");
    auto* device=gpu_.device.Get();
    std::unique_lock<std::mutex> face_pipeline(face_lock_);
    if(!face_pso_){
        // Base layer of 836E40 -> 232FD10 -> 2378 9E0: one quad over the whole
        // viewport (centre 256,256, extent 512, texcoords 0..1), sampled base
        // image times colour0 (1,1,1,1). Explicit open policy: linear clamp
        // sampling (the drawer's sampler [root+2A0]+2D0 is not decoded).
        D3D12_DESCRIPTOR_RANGE range{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,0,0,0};
        D3D12_ROOT_PARAMETER parameter{};parameter.ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        parameter.DescriptorTable={1,&range};parameter.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        D3D12_STATIC_SAMPLER_DESC sampler{};sampler.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.MaxAnisotropy=1;sampler.ComparisonFunc=D3D12_COMPARISON_FUNC_NEVER;sampler.MaxLOD=D3D12_FLOAT32_MAX;
        sampler.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        D3D12_ROOT_SIGNATURE_DESC desc{1,&parameter,1,&sampler,D3D12_ROOT_SIGNATURE_FLAG_NONE};
        Microsoft::WRL::ComPtr<ID3DBlob> blob,error;
        Check(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"native FaceGen root serialization");
        Check(Pipelines().Root(device,blob->GetBufferPointer(),blob->GetBufferSize(),face_root_.ReleaseAndGetAddressOf()),"native FaceGen root");
        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};pso.pRootSignature=face_root_.Get();
        pso.VS={facegen_composition_vs_dxbc,sizeof(facegen_composition_vs_dxbc)};pso.PS={facegen_composition_ps_dxbc,sizeof(facegen_composition_ps_dxbc)};
        pso.BlendState.RenderTarget[0].RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;pso.SampleMask=UINT_MAX;
        pso.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;pso.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;pso.RasterizerState.DepthClipEnable=TRUE;
        pso.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;pso.NumRenderTargets=1;pso.RTVFormats[0]=DXGI_FORMAT_R8G8B8A8_UNORM;
        pso.SampleDesc.Count=1;
        Check(Pipelines().Graphics(device,pso,face_pso_.ReleaseAndGetAddressOf()),"native FaceGen pipeline");
    }
    if(mark && !face_mark_pso_){
        // The mark layer (I-0113 tranche 16; static, headless on ghidra-mirror-i0068, evidence/I-0113/t16/run1): 142379C90 draws a
        // strip of four vertices with GXSimple_TransformSingle2DTexture_V / GXSimple_Single2DTexture_F (entry 5 of 1400DAC20's
        // table), the vertex colour (1,1,1,1), the colour 836E40 set (14030E490) as the pixel constant b0, clamped and rounded to
        // bytes (1400DB640); state mode 7 (142397950): blend SRC_ALPHA / INV_SRC_ALPHA add, alpha ONE / ZERO add, no depth, no
        // cull; the sampler [root+2A0]+2D0 linear with wrap. The programmes' bytecode is not in the executable: texture times
        // colour is INFERRED from their names.
        D3D12_DESCRIPTOR_RANGE range{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,0,0,0};
        D3D12_ROOT_PARAMETER parameters[2]{};parameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        parameters[0].DescriptorTable={1,&range};parameters[0].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        parameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;parameters[1].Constants={0,0,12};parameters[1].ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
        D3D12_STATIC_SAMPLER_DESC sampler{};sampler.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.MaxAnisotropy=1;sampler.ComparisonFunc=D3D12_COMPARISON_FUNC_NEVER;sampler.MaxLOD=D3D12_FLOAT32_MAX;
        sampler.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        D3D12_ROOT_SIGNATURE_DESC desc{2,parameters,1,&sampler,D3D12_ROOT_SIGNATURE_FLAG_NONE};
        Microsoft::WRL::ComPtr<ID3DBlob> blob,error;
        Check(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"native FaceGen mark root serialization");
        Check(Pipelines().Root(device,blob->GetBufferPointer(),blob->GetBufferSize(),face_mark_root_.ReleaseAndGetAddressOf()),"native FaceGen mark root");
        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};pso.pRootSignature=face_mark_root_.Get();
        pso.VS={facegen_mark_vs_dxbc,sizeof(facegen_mark_vs_dxbc)};pso.PS={facegen_mark_ps_dxbc,sizeof(facegen_mark_ps_dxbc)};
        auto& blend=pso.BlendState.RenderTarget[0];blend.BlendEnable=TRUE;blend.SrcBlend=D3D12_BLEND_SRC_ALPHA;blend.DestBlend=D3D12_BLEND_INV_SRC_ALPHA;
        blend.BlendOp=D3D12_BLEND_OP_ADD;blend.SrcBlendAlpha=D3D12_BLEND_ONE;blend.DestBlendAlpha=D3D12_BLEND_ZERO;blend.BlendOpAlpha=D3D12_BLEND_OP_ADD;
        blend.LogicOp=D3D12_LOGIC_OP_NOOP;blend.RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;pso.SampleMask=UINT_MAX;
        pso.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;pso.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;pso.RasterizerState.DepthClipEnable=TRUE;
        pso.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;pso.NumRenderTargets=1;pso.RTVFormats[0]=DXGI_FORMAT_R8G8B8A8_UNORM;
        pso.SampleDesc.Count=1;
        Check(Pipelines().Graphics(device,pso,face_mark_pso_.ReleaseAndGetAddressOf()),"native FaceGen mark pipeline");
    }
    const Microsoft::WRL::ComPtr<ID3D12RootSignature> face_root=face_root_;const Microsoft::WRL::ComPtr<ID3D12PipelineState> face_pso=face_pso_;
    const Microsoft::WRL::ComPtr<ID3D12RootSignature> mark_root=face_mark_root_;const Microsoft::WRL::ComPtr<ID3D12PipelineState> mark_pso=face_mark_pso_;
    face_pipeline.unlock();
    D3D12_HEAP_PROPERTIES heap{D3D12_HEAP_TYPE_DEFAULT};
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=width;desc.Height=height;
    desc.DepthOrArraySize=1;desc.MipLevels=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;
    desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    D3D12_CLEAR_VALUE clear{DXGI_FORMAT_R8G8B8A8_UNORM,{0,0,0,0}};
    Check(CountedResource(device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_RENDER_TARGET,&clear,IID_PPV_ARGS(&candidate->resource)),"native FaceGen target");
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtv,visible;
    D3D12_DESCRIPTOR_HEAP_DESC rtv_desc{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,1};
    Check(CountedHeap(device)->CreateDescriptorHeap(&rtv_desc,IID_PPV_ARGS(&rtv)),"native FaceGen target view");
    device->CreateRenderTargetView(candidate->resource.Get(),nullptr,rtv->GetCPUDescriptorHandleForHeapStart());
    D3D12_DESCRIPTOR_HEAP_DESC visible_desc{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,mark?2u:1u,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE};
    Check(CountedHeap(device)->CreateDescriptorHeap(&visible_desc,IID_PPV_ARGS(&visible)),"native FaceGen base table");
    device->CopyDescriptorsSimple(1,visible->GetCPUDescriptorHandleForHeapStart(),base.view.handle,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    const UINT increment=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    if(mark){D3D12_CPU_DESCRIPTOR_HANDLE second=visible->GetCPUDescriptorHandleForHeapStart();second.ptr+=increment;
        device->CopyDescriptorsSimple(1,second,mark->texture->view.handle,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);}
    candidate->view=gpu_.AllocateCpuDescriptor();
    device->CreateShaderResourceView(candidate->resource.Get(),nullptr,candidate->view.handle);
    // B3a: on the ingress context, after the base image's upload (commit order), before any later submission.
    auto recording=gpu_.RecordIngress();auto* list=recording.List();list->SetPipelineState(face_pso.Get());
    const auto target=rtv->GetCPUDescriptorHandleForHeapStart();const float zero[4]{};
    list->ClearRenderTargetView(target,zero,0,nullptr);list->OMSetRenderTargets(1,&target,FALSE,nullptr);
    const D3D12_VIEWPORT viewport{0,0,float(width),float(height),0,1};const D3D12_RECT scissor{0,0,LONG(width),LONG(height)};
    list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
    ID3D12DescriptorHeap* tables[]{visible.Get()};list->SetDescriptorHeaps(1,tables);
    list->SetGraphicsRootSignature(face_root.Get());list->SetGraphicsRootDescriptorTable(0,visible->GetGPUDescriptorHandleForHeapStart());
    list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);list->DrawInstanced(3,1,0,0);
    if(mark){   // the mark layer over the base, in the same viewport
        list->SetPipelineState(mark_pso.Get());list->SetGraphicsRootSignature(mark_root.Get());
        D3D12_GPU_DESCRIPTOR_HANDLE second=visible->GetGPUDescriptorHandleForHeapStart();second.ptr+=increment;list->SetGraphicsRootDescriptorTable(0,second);
        std::array<float,12> constants{};std::copy(mark->corners.begin(),mark->corners.end(),constants.begin());
        for(size_t i=0;i<4;++i)constants[8+i]=std::nearbyint(std::clamp(mark->colour[i],0.0f,1.0f)*255.0f)/255.0f;   // 1400DB640's bytes
        list->SetGraphicsRoot32BitConstants(1,12,constants.data(),0);
        list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);list->DrawInstanced(4,1,0,0);
        for(auto held:{Held(mark->texture->resource),Held(mark_root),Held(mark_pso)})recording.Keep(std::move(held));}
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition={candidate->resource.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_RENDER_TARGET,
        static_cast<D3D12_RESOURCE_STATES>(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)};
    list->ResourceBarrier(1,&barrier);
    for(auto held:{Held(candidate->resource),Held(base.resource),Held(rtv),Held(visible),Held(face_root),Held(face_pso)})recording.Keep(std::move(held));
    candidate->upload_fence=recording.Commit();
    candidate->content_bytes=uint64_t(width)*height*4;
    {std::lock_guard<std::shared_mutex> hold(mutex_);const auto found=records_.find(receiver);
        Require(!closed_ && found!=records_.end() && found->second==candidate,"native FaceGen incarnation withdrawn");candidate->resident=true;Changed();}
    return candidate;
}
std::shared_ptr<const NativeTextureRecord> TextureIngress::AdmitOffscreenTarget(uintptr_t receiver,uint32_t width,uint32_t height,const std::array<float,4>& clear) {
    Require(receiver && width && height && width<=4096 && height<=4096,"native offscreen target description");
    auto candidate=std::make_shared<NativeTextureRecord>();
    candidate->description={width,height,1,DXGI_FORMAT_R8G8B8A8_UNORM,1,false};
    auto content=std::make_shared<ds3content::CpuTextureResource>();
    content->name="native-menu-offscreen-target";content->width=width;content->height=height;content->mip_levels=1;content->array_slices=1;
    content->format=DXGI_FORMAT_R8G8B8A8_UNORM;
    // I-0096 (step review 5): the description only, as an admitted texture's; a render-target record is bound as is, its bytes
    // were never read (up to 64 MB of zeros a target before).
    content->subresources.push_back({width,height,width*4,height,{}});
    candidate->content=std::move(content);candidate->render_target=true;
    const Admission admission(*this);
    {std::lock_guard<std::shared_mutex> hold(mutex_);candidate->generation=++generation_;Install(receiver,candidate);}
    Require(gpu_.device && gpu_.queue,"native texture device unavailable");
    auto* device=gpu_.device.Get();
    D3D12_HEAP_PROPERTIES heap{D3D12_HEAP_TYPE_DEFAULT};
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=width;desc.Height=height;
    desc.DepthOrArraySize=1;desc.MipLevels=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;
    desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    D3D12_CLEAR_VALUE value{DXGI_FORMAT_R8G8B8A8_UNORM,{clear[0],clear[1],clear[2],clear[3]}};
    Check(CountedResource(device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_RENDER_TARGET,&value,IID_PPV_ARGS(&candidate->resource)),"native offscreen target");
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtv;
    D3D12_DESCRIPTOR_HEAP_DESC rtv_desc{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,1};
    Check(CountedHeap(device)->CreateDescriptorHeap(&rtv_desc,IID_PPV_ARGS(&rtv)),"native offscreen target view");
    device->CreateRenderTargetView(candidate->resource.Get(),nullptr,rtv->GetCPUDescriptorHandleForHeapStart());
    candidate->view=gpu_.AllocateCpuDescriptor();
    device->CreateShaderResourceView(candidate->resource.Get(),nullptr,candidate->view.handle);
    // B3a: the clear on the ingress context, ahead of the first submission that draws or samples the target.
    auto recording=gpu_.RecordIngress();auto* list=recording.List();
    list->ClearRenderTargetView(rtv->GetCPUDescriptorHandleForHeapStart(),clear.data(),0,nullptr);
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition={candidate->resource.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_RENDER_TARGET,
        static_cast<D3D12_RESOURCE_STATES>(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)};
    list->ResourceBarrier(1,&barrier);
    recording.Keep(Held(candidate->resource));recording.Keep(Held(rtv));
    candidate->upload_fence=recording.Commit();
    candidate->content_bytes=uint64_t(width)*height*4;
    {std::lock_guard<std::shared_mutex> hold(mutex_);const auto found=records_.find(receiver);
        Require(!closed_ && found!=records_.end() && found->second==candidate,"native offscreen incarnation withdrawn");candidate->resident=true;Changed();}
    return candidate;
}
std::shared_ptr<const NativeTextureRecord> TextureIngress::AdmitStructuredZero(uintptr_t receiver,uint32_t stride,uint32_t elements) {
    Require(receiver && stride && stride<=2048 && !(stride%4) && elements && elements<=65536,"native structured buffer description");
    auto candidate=std::make_shared<NativeTextureRecord>();
    candidate->description={elements,1,1,DXGI_FORMAT_UNKNOWN,1,false};
    const Admission admission(*this);
    {std::lock_guard<std::shared_mutex> hold(mutex_);candidate->generation=++generation_;Install(receiver,candidate);}
    Require(gpu_.device,"native texture device unavailable");
    // Upload-heap buffer in GENERIC_READ, written once with zeros: shader readable
    // without transitions; no copy queue work.
    const UINT64 bytes=UINT64(stride)*elements;
    D3D12_HEAP_PROPERTIES heap{D3D12_HEAP_TYPE_UPLOAD};
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=bytes;desc.Height=1;
    desc.DepthOrArraySize=1;desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    Check(CountedResource(gpu_.device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&candidate->resource)),"native structured buffer");
    void* memory{};const D3D12_RANGE empty{};Check(candidate->resource->Map(0,&empty,&memory),"native structured buffer map");
    std::memset(memory,0,static_cast<size_t>(bytes));const D3D12_RANGE written{0,static_cast<SIZE_T>(bytes)};candidate->resource->Unmap(0,&written);
    candidate->view=gpu_.AllocateCpuDescriptor();
    D3D12_SHADER_RESOURCE_VIEW_DESC view{};view.Format=DXGI_FORMAT_UNKNOWN;view.ViewDimension=D3D12_SRV_DIMENSION_BUFFER;
    view.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;view.Buffer.NumElements=elements;view.Buffer.StructureByteStride=stride;
    gpu_.device->CreateShaderResourceView(candidate->resource.Get(),&view,candidate->view.handle);
    candidate->content_bytes=bytes;
    {std::lock_guard<std::shared_mutex> hold(mutex_);const auto found=records_.find(receiver);
        Require(!closed_ && found!=records_.end() && found->second==candidate,"native structured buffer incarnation withdrawn");candidate->resident=true;Changed();}
    return candidate;
}
std::shared_ptr<const NativeTextureRecord> TextureIngress::Acquire(uintptr_t receiver) const {
    std::shared_lock<std::shared_mutex> hold(mutex_);const auto found=records_.find(receiver);
    return !closed_ && found!=records_.end() && found->second->resident?found->second:nullptr;
}
void TextureIngress::Retire(uintptr_t receiver) {
    {std::lock_guard<std::shared_mutex> hold(mutex_);const auto found=records_.find(receiver);
        if(found!=records_.end()){found->second->withdrawn.store(true,std::memory_order_release);withdrawn_.push_back(found->second);
            ReleaseKeptUser(*found->second);records_.erase(found);Withdrawn();Changed();}}
    FlushKept();
}
// D-0211: a record leaving the receivers (retired, admitted again) gives back its use of a kept content; the last one puts it
// at the head of the retired list, and the retired contents beyond the limit are released, the least recently retired first.
void TextureIngress::ReleaseKeptUser(const NativeTextureRecord& record) {
    if(!record.kept_key)return;
    const std::lock_guard hold(kept_lock_);const auto found=kept_.find(*record.kept_key);
    if(found==kept_.end() || !found->second.users)return;
    if(!--found->second.users){kept_retired_.push_front(found->first);found->second.retired=kept_retired_.begin();kept_retired_bytes_+=found->second.content.placed_bytes;}
    if(kept_retired_bytes_>kept_limit)EvictKept(kept_retired_bytes_-kept_limit);
}
void TextureIngress::EvictKept(uint64_t bytes) {
    uint64_t released{};
    while(released<bytes && !kept_retired_.empty()){const auto found=kept_.find(kept_retired_.back());kept_retired_.pop_back();
        if(found==kept_.end())continue;
        const auto placed=found->second.content.placed_bytes;released+=placed;kept_retired_bytes_-=placed;kept_placed_bytes_-=placed;
        kept_released_.push_back(std::move(found->second.content));kept_.erase(found);}
}
void TextureIngress::FlushKept() {
    std::vector<KeptContent> released;{const std::lock_guard hold(kept_lock_);released.swap(kept_released_);}
}
std::optional<TextureIngress::KeptContent> TextureIngress::FindKept(const ContentKey& key) {
    const std::lock_guard hold(kept_lock_);const auto found=kept_.find(key);
    if(found==kept_.end()){++kept_misses_;return std::nullopt;}
    ++kept_hits_;return found->second.content;
}
std::shared_ptr<const NativeTextureRecord> TextureIngress::AdmitKept(uintptr_t receiver,const ContentKey& key,const KeptContent& kept) {
    Require(receiver && kept.resource && kept.content,"native kept texture content");
    auto candidate=std::make_shared<NativeTextureRecord>();
    candidate->description=kept.description;candidate->content=kept.content;candidate->content_bytes=kept.content_bytes;
    candidate->resource=kept.resource;candidate->upload_fence=kept.upload_fence;   // uploaded before an earlier submission
    candidate->kept_key=std::make_shared<const ContentKey>(key);
    const Admission admission(*this);
    candidate->view=gpu_.AllocateCpuDescriptor();   // a view of its own (B3b: pooled CPU descriptor)
    const auto view=ds3content::NativeTextureView(*kept.content);
    gpu_.device->CreateShaderResourceView(candidate->resource.Get(),&view,candidate->view.handle);
    candidate->resident=true;   // complete before it is published
    {std::lock_guard<std::shared_mutex> hold(mutex_);
        {const std::lock_guard hold_kept(kept_lock_);auto found=kept_.find(key);
            // Released since FindKept (a budget trim): kept again from this record's copy.
            if(found==kept_.end() && !kept_closed_){found=kept_.emplace(key,KeptEntry{kept,0,kept_retired_.end()}).first;kept_placed_bytes_+=kept.placed_bytes;}
            if(found!=kept_.end()){if(!found->second.users++ && found->second.retired!=kept_retired_.end()){
                    kept_retired_.erase(found->second.retired);found->second.retired=kept_retired_.end();kept_retired_bytes_-=found->second.content.placed_bytes;}}
            else candidate->kept_key.reset();}
        candidate->generation=++generation_;Install(receiver,candidate);}
    FlushKept();
    return candidate;
}
uint64_t TextureIngress::TrimKept(uint64_t bytes) {
    uint64_t before{},after{};
    {const std::lock_guard hold(kept_lock_);before=kept_placed_bytes_;EvictKept(bytes);after=kept_placed_bytes_;}
    FlushKept();return before-after;
}
TextureIngress::KeptCensus TextureIngress::KeptCount() {
    const std::lock_guard hold(kept_lock_);return {kept_hits_,kept_misses_,kept_.size(),kept_retired_.size(),kept_retired_bytes_,kept_placed_bytes_};
}
void TextureIngress::Close() {
    {const std::lock_guard hold(kept_lock_);kept_closed_=true;}   // D-0211: nothing is kept from now on
    {std::unique_lock<std::shared_mutex> hold(mutex_);closed_=true;
        for(const auto& [receiver,record]:records_){(void)receiver;record->withdrawn.store(true,std::memory_order_release);}Withdrawn();Changed();idle_.wait(hold,[this]{return !admitting_;});}
    std::lock_guard<std::mutex> submission(submissions_);gpu_.SubmitIngress();gpu_.Drain(WaitClose);
    {std::lock_guard<std::shared_mutex> hold(mutex_);records_.clear();Changed();}
    {const std::lock_guard hold(kept_lock_);for(auto& [key,entry]:kept_){(void)key;kept_released_.push_back(std::move(entry.content));}
        kept_.clear();kept_retired_.clear();kept_retired_bytes_=kept_placed_bytes_=0;}
    FlushKept();   // every kept content, after the records and the drain
}
}
