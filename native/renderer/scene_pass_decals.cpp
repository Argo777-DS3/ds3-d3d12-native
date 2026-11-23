// The scene renderer's decals: their targets and sampled copies, the decal pass, the water depth copy and the character
// paint targets (scene_pass_internal.h has the shared helpers).
#include "scene_pass_internal.h"

namespace ds3native {
// DECALS-WAVE1 D1 (see scene_pass.h): the decals' targets and sampled copies.
const std::array<DXGI_FORMAT,7>& SceneRenderer::DecalTargetFormats() noexcept {
    static constexpr std::array<DXGI_FORMAT,7> formats{DXGI_FORMAT_R11G11B10_FLOAT,color_formats[1],color_formats[2],color_formats[3],
        color_formats[4],color_formats[5],color_formats[0]};
    return formats;}
SceneRenderer::DecalSourceSet SceneRenderer::DecalSources() {
    RequireThread(false,"native decal sources on the render thread");
    if(!open_ || !gpu_.device || !presenter_.width || !presenter_.height)return {};
    // I-0111: at the render extent, as the G-buffer they copy.
    const auto fits=[&](const ComPtr<ID3D12Resource>& r){if(!r)return false;const auto d=r->GetDesc();return d.Width==RenderWidth() && d.Height==RenderHeight();};
    if(fits(decal_sources_.gb0) && fits(decal_sources_.gb1) && fits(decal_sources_.depth))return decal_sources_;
    // A size change: the earlier copies stay alive in their holders until their draws complete.
    const auto heap=Heap(D3D12_HEAP_TYPE_DEFAULT);DecalSourceSet made;
    const auto create=[&](DXGI_FORMAT format,D3D12_RESOURCE_FLAGS flags,ComPtr<ID3D12Resource>& out,const char* what){
        const auto image=Image(RenderWidth(),RenderHeight(),1,format,flags);
        Check(CountedResource(gpu_.device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&image,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,nullptr,IID_PPV_ARGS(&out)),what);};
    create(color_formats[0],D3D12_RESOURCE_FLAG_NONE,made.gb0,"native decal GB0 copy");
    create(color_formats[1],D3D12_RESOURCE_FLAG_NONE,made.gb1,"native decal GB1 copy");
    // The depth copy keeps the depth's two planes (a depth-stencil resource, as CopyResource needs), read through its R32 plane.
    create(DXGI_FORMAT_R32G8X24_TYPELESS,D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,made.depth,"native decal depth copy");
    decal_sources_=made;return made;
}
// 51AA0's order: the copies (GB0, GB1, depth; the original's +0x1110, +0x1118, +0xFD0), then each decal with the seven targets
// and the depth read-only (stencil reference 1 against the G-buffer's receiver bit 0). Leaves the G-buffer as render targets
// and the depth in DEPTH_WRITE, as the lighting expects.
void SceneRenderer::EncodeNativeDecals(ID3D12GraphicsCommandList* list,const NativeModelFrameInput& input,NativeModelConstantUploads& uploads,
    const D3D12_VIEWPORT& viewport,const D3D12_RECT& scissor) {
    auto* graph=active_graph_shadow;auto* gb0=view_->colors[0].Get();auto* gb1=view_->colors[1].Get();auto* depth=view_->depth.Get();
    ID3D12Resource* copies[3]{input.decal_gb0_copy.Get(),input.decal_gb1_copy.Get(),input.decal_depth_copy.Get()};
    ID3D12Resource* sources[3]{gb0,gb1,depth};
    const auto rt=D3D12_RESOURCE_STATE_RENDER_TARGET,copy_source=D3D12_RESOURCE_STATE_COPY_SOURCE,copy_dest=D3D12_RESOURCE_STATE_COPY_DEST,
        psr=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,depth_read=D3D12_RESOURCE_STATE_DEPTH_READ|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    Transition(list,gb0,rt,copy_source);Transition(list,gb1,rt,copy_source);Transition(list,depth,D3D12_RESOURCE_STATE_DEPTH_WRITE,copy_source);
    for(auto* copy:copies)Transition(list,copy,psr,copy_dest);
    if(graph){graph->Pass("decal_copies",PassKind::Material);   // after the transitions: a use names the state the pass runs in
        for(unsigned i=0;i<3;++i){graph->Use(sources[i],ResourceAccess::Read,copy_source);graph->Use(copies[i],ResourceAccess::Write,copy_dest);}}
    for(unsigned i=0;i<3;++i)list->CopyResource(copies[i],sources[i]);
    for(auto* copy:copies)Transition(list,copy,copy_dest,psr);
    Transition(list,gb0,copy_source,rt);Transition(list,gb1,copy_source,rt);Transition(list,depth,copy_source,D3D12_RESOURCE_STATE_DEPTH_WRITE);
    DepthPlaneTransition(list,depth,D3D12_RESOURCE_STATE_DEPTH_WRITE,depth_read);
    if(graph){graph->Pass("decals",PassKind::Material);graph->Use(view_->env_target.Get(),ResourceAccess::Write,rt);
        for(unsigned i=0;i<6;++i)graph->Use(view_->colors[i].Get(),ResourceAccess::ReadWrite,rt);   // blended
        graph->Use(depth,ResourceAccess::Read,depth_read,{0,1});graph->Use(depth,ResourceAccess::ReadWrite,D3D12_RESOURCE_STATE_DEPTH_WRITE,{1,1});
        for(auto* copy:copies)graph->Use(copy,ResourceAccess::Read,psr);}
    const std::array<D3D12_CPU_DESCRIPTOR_HANDLE,7> targets{CPU(view_->light_rtvs.Get(),rtv_stride_,7),CPU(view_->rtvs.Get(),rtv_stride_,1),
        CPU(view_->rtvs.Get(),rtv_stride_,2),CPU(view_->rtvs.Get(),rtv_stride_,3),CPU(view_->rtvs.Get(),rtv_stride_,4),CPU(view_->rtvs.Get(),rtv_stride_,5),
        CPU(view_->rtvs.Get(),rtv_stride_,0)};
    const auto read_only=CPU(view_->dsv.Get(),gpu_.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV),1);   // READ_ONLY_DEPTH
    {GBufferBindState state;for(const auto& draw:input.decal_draws)draw->Encode(list,targets,read_only,viewport,scissor,NativeModelFrameAddresses{},uploads,&state);}
    DepthPlaneTransition(list,depth,depth_read,D3D12_RESOURCE_STATE_DEPTH_WRITE);
}
ComPtr<ID3D12Resource> SceneRenderer::WaterDepthCopy() {
    RequireThread(false,"native water depth copy on the render thread");
    if(!open_ || !gpu_.device || !presenter_.width || !presenter_.height)return {};
    const std::lock_guard hold(water_depth_lock_);
    if(water_depth_copy_){const auto d=water_depth_copy_->GetDesc();if(d.Width==RenderWidth() && d.Height==RenderHeight())return water_depth_copy_;}   // I-0111: the depth's extent
    // A size change: the earlier copy stays alive in its holders until their draws complete.
    const auto heap=Heap(D3D12_HEAP_TYPE_DEFAULT);const auto image=Image(RenderWidth(),RenderHeight(),1,DXGI_FORMAT_R32G8X24_TYPELESS,D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    ComPtr<ID3D12Resource> made;
    Check(CountedResource(gpu_.device.Get())->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&image,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,nullptr,IID_PPV_ARGS(&made)),
        "native water depth copy");
    water_depth_copy_=made;return made;
}
std::shared_ptr<NativePaintTarget> SceneRenderer::MakePaintTarget(uint32_t size) {
    RequireThread(false,"native paint target on the render thread");
    if(!open_ || !gpu_.device)return {};
    if(size<16 || size>4096)throw std::invalid_argument("native paint target size");
    auto target=std::make_shared<NativePaintTarget>();target->size=size;
    const auto heap=Heap(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_DESCRIPTOR_HEAP_DESC desc{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,2,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
    Check(CountedHeap(gpu_.device)->CreateDescriptorHeap(&desc,IID_PPV_ARGS(&target->rtvs)),"native paint target views");
    const auto stride=gpu_.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    for(uint32_t i=0;i<2;++i){
        const auto image=Image(size,size,1,paint_formats[i],D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        D3D12_CLEAR_VALUE clear{};clear.Format=paint_formats[i];clear.Color[3]=1.0f;   // 4CD70: (0,0,0,1)
        auto& resource=i?target->specular:target->diffuse;
        Check(CountedResource(gpu_.device.Get())->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&image,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,&clear,
            IID_PPV_ARGS(&resource)),i?"native paint specular target":"native paint diffuse target");
        target->rtv[i]=CPU(target->rtvs.Get(),stride,i);
        D3D12_RENDER_TARGET_VIEW_DESC view{};view.Format=paint_formats[i];view.ViewDimension=D3D12_RTV_DIMENSION_TEXTURE2D;
        gpu_.device->CreateRenderTargetView(resource.Get(),&view,target->rtv[i]);}
    {// D-0193 rung 2b: the projector depth map (R16 typeless: D16 for the depth pass, R16 for the paint's t32).
        const auto image=Image(paint_occlusion_edge,paint_occlusion_edge,1,DXGI_FORMAT_R16_TYPELESS,D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
        D3D12_CLEAR_VALUE clear{};clear.Format=DXGI_FORMAT_D16_UNORM;clear.DepthStencil.Depth=1.0f;
        Check(CountedResource(gpu_.device.Get())->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&image,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,&clear,
            IID_PPV_ARGS(&target->occlusion)),"native paint occlusion map");
        D3D12_DESCRIPTOR_HEAP_DESC dsvs{D3D12_DESCRIPTOR_HEAP_TYPE_DSV,1,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
        Check(CountedHeap(gpu_.device)->CreateDescriptorHeap(&dsvs,IID_PPV_ARGS(&target->dsvs)),"native paint occlusion view");
        target->dsv=target->dsvs->GetCPUDescriptorHandleForHeapStart();
        D3D12_DEPTH_STENCIL_VIEW_DESC view{};view.Format=DXGI_FORMAT_D16_UNORM;view.ViewDimension=D3D12_DSV_DIMENSION_TEXTURE2D;
        gpu_.device->CreateDepthStencilView(target->occlusion.Get(),&view,target->dsv);}
    return target;
}
// DECALS-PAINT-READY rung 2: each job's target from PIXEL_SHADER_RESOURCE to RENDER_TARGET and back: cleared to (0,0,0,1) at its
// first job or a handle-0 command (4CD70), then (1AAA70's colour pass, pass 8) the depth cleared to 1.0 (1419201A0) and the
// struck character's meshes drawn at the target's size with the projector's VS b0 and a cbSceneParam whose VC_ClipPlane is
// (0,0,0,1) (1AB370's 0x2E0 upload, [1439F2500]). Before the G-buffer, so the G-buffer samples this frame's paint (the original
// paints after its G-buffer: one frame later). The targets are not frame-graph resources (their own transitions). I-0105: an
// erase job's draws (1AAF90, pass 10) take the same target, depth and viewport without the projector depth map; their programmes
// read none of the frame blocks.
void SceneRenderer::EncodeNativePaint(ID3D12GraphicsCommandList* list,const NativeModelFrameInput& input,NativeModelConstantUploads& uploads,const NativeModelFrameAddresses& frame) {
    const auto psr=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,rt=D3D12_RESOURCE_STATE_RENDER_TARGET;
    for(const auto& job:input.paints){const auto* target=job.target.get();
        if(!target || !target->diffuse || !target->specular || (target->cleared && !job.clear && job.draws.empty()))continue;
        Transition(list,target->diffuse.Get(),psr,rt);Transition(list,target->specular.Get(),psr,rt);
        if(!target->cleared || job.clear){const float neutral[4]{0,0,0,1};
            for(const auto& view:target->rtv)list->ClearRenderTargetView(view,neutral,0,nullptr);target->cleared=true;}
        // D-0193 rung 2b: the projector depth map, cleared to 1.0, then the struck character's depth programmes (its Dpt caster rows)
        // with VS b0 = 256 bytes (the identity at +0, F at +64: the Dpt VS's VC_View_MatrixViewProj); t32 of the paint draws below.
        if(!job.erase && !job.draws.empty() && target->occlusion){const auto dsr=D3D12_RESOURCE_STATE_DEPTH_WRITE;Transition(list,target->occlusion.Get(),psr,dsr);
            list->ClearDepthStencilView(target->dsv,D3D12_CLEAR_FLAG_DEPTH,1.0f,0,0,nullptr);
            if(!job.depth_draws.empty()){auto addresses=frame;
                {const auto range=frame_uploads_.Allocate(gpu_.device.Get(),256);std::memset(range.cpu,0,256);std::memcpy(range.cpu+64,job.view.data(),64);
                    for(int i=0;i<4;++i){const float one=1.0f;std::memcpy(range.cpu+20*i,&one,4);}addresses[FrameVsView]=range.gpu;}   // +0 the identity, as the original's (Reference2362 paint_oracle pass 2)
                {const auto range=frame_uploads_.Allocate(gpu_.device.Get(),1024);std::memset(range.cpu,0,1024);const float clip[4]{0,0,0,1};std::memcpy(range.cpu+64,clip,16);addresses[FrameVsScene]=range.gpu;}
                const D3D12_VIEWPORT viewport{0,0,float(paint_occlusion_edge),float(paint_occlusion_edge),0,1};const D3D12_RECT rect{0,0,LONG(paint_occlusion_edge),LONG(paint_occlusion_edge)};
                GBufferBindState state;for(const auto& draw:job.depth_draws)draw->Encode(list,{},target->dsv,viewport,rect,addresses,uploads,&state);}
            Transition(list,target->occlusion.Get(),dsr,psr);}
        if(!job.draws.empty()){
            auto& depth=paint_depths_[target->size];
            if(!depth.resource){const auto heap=Heap(D3D12_HEAP_TYPE_DEFAULT);
                const auto image=Image(target->size,target->size,1,DXGI_FORMAT_D32_FLOAT,D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL|D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);
                D3D12_CLEAR_VALUE clear{};clear.Format=DXGI_FORMAT_D32_FLOAT;clear.DepthStencil.Depth=1.0f;
                Check(CountedResource(gpu_.device.Get())->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&image,D3D12_RESOURCE_STATE_DEPTH_WRITE,&clear,
                    IID_PPV_ARGS(&depth.resource)),"native paint depth");
                D3D12_DESCRIPTOR_HEAP_DESC desc{D3D12_DESCRIPTOR_HEAP_TYPE_DSV,1,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
                Check(CountedHeap(gpu_.device)->CreateDescriptorHeap(&desc,IID_PPV_ARGS(&depth.dsv)),"native paint depth view");
                D3D12_DEPTH_STENCIL_VIEW_DESC view{};view.Format=DXGI_FORMAT_D32_FLOAT;view.ViewDimension=D3D12_DSV_DIMENSION_TEXTURE2D;
                gpu_.device->CreateDepthStencilView(depth.resource.Get(),&view,depth.dsv->GetCPUDescriptorHandleForHeapStart());
                depth.edge=target->size;target_bytes_key_.clear();}
            const auto dsv=depth.dsv->GetCPUDescriptorHandleForHeapStart();
            list->ClearDepthStencilView(dsv,D3D12_CLEAR_FLAG_DEPTH,1.0f,0,0,nullptr);
            auto addresses=frame;   // D-0193: the frame's blocks under the projector's VS b0 and clip plane
            {const auto range=frame_uploads_.Allocate(gpu_.device.Get(),256);std::memset(range.cpu,0,256);std::memcpy(range.cpu,job.view.data(),64);addresses[FrameVsView]=range.gpu;}
            {const auto range=frame_uploads_.Allocate(gpu_.device.Get(),1024);std::memset(range.cpu,0,1024);const float clip[4]{0,0,0,1};std::memcpy(range.cpu+64,clip,16);addresses[FrameVsScene]=range.gpu;}
            const D3D12_VIEWPORT viewport{0,0,float(target->size),float(target->size),0,1};const D3D12_RECT rect{0,0,LONG(target->size),LONG(target->size)};
            GBufferBindState state;for(const auto& draw:job.draws)draw->Encode(list,target->rtv,dsv,viewport,rect,addresses,uploads,&state);
            }
        Transition(list,target->diffuse.Get(),rt,psr);Transition(list,target->specular.Get(),rt,psr);}
}
}
