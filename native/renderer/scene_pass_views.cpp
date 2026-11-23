// The scene renderer's offscreen views (the menu views), pixel readbacks and the Bink movie frame
// (scene_pass_internal.h has the shared helpers).
#include "scene_pass_internal.h"
#include "effect_scan.h"
#include "../world/menu_tonemap.h"
#include "effect_draw_scan_cs.dxbc.h"   // I-0132 R12h step 2: built by fxc (shaders/effect_draw_scan.hlsl)

namespace ds3native {
void SceneRenderer::RenderNativeOffscreen(const NativeModelFrameInput& input,ID3D12Resource* target,const std::array<float,4>& background,const std::wstring& pixel_file,const NativeOffscreenTone& tone) {
    if(!open_ || faulted_ || !target || input.draws.empty() || input.draws.size()>131072 ||
       !input.lighting || !input.lighting->programs)
        throw std::invalid_argument("native offscreen view admission");
    const auto desc=target->GetDesc();
    const auto width=static_cast<uint32_t>(desc.Width),height=desc.Height;
    if(desc.Format!=DXGI_FORMAT_R8G8B8A8_UNORM || !width || !height || width>4096 || height>4096 ||
       input.camera.width!=width || input.camera.height!=height)throw std::invalid_argument("native offscreen view target");
    // cD3: an earlier view of this frame may have recorded with this context's depth target (complement 4.1: the game
    // path never changes the stencil clear value, so this never fires).
    if(frames_.Recording() && offscreen_sets_.contains({width,height}) && input.clear_stencil!=offscreen_sets_.at({width,height}).clear_stencil)
        throw std::logic_error("native offscreen depth recreation inside an open frame list");
    // W0a: one envelope check per list (EnvelopeMatches); Forward2 and DepthOnly wait for their passes' graph declarations.
    for(const auto& draw:input.draws)if(!draw || !EnvelopeMatches(draw->Input(),ModelPassEnvelope::GBuffer6,color_formats,ModelDepthFormat()))
        throw std::invalid_argument("native offscreen output formats");
    for(const auto& draw:input.forward_draws)if(!draw || !EnvelopeMatches(draw->Input(),ModelPassEnvelope::Forward1,forward_formats,ModelDepthFormat()))
        throw std::invalid_argument("native offscreen forward formats");
    // I-0120 E1: one context per extent: past kKeptViewContexts, the least
    // recently used of those idle for 600 frames leaves by ticket, so the output-size changes do not keep every former size.
    constexpr size_t kKeptViewContexts=4;const auto numbered=frames_.Numbered();
    while(offscreen_sets_.size()>kKeptViewContexts){auto oldest=offscreen_sets_.end();
        for(auto it=offscreen_sets_.begin();it!=offscreen_sets_.end();++it)
            if(it->first!=std::pair{width,height} && numbered>=it->second.last_frame+600 && (oldest==offscreen_sets_.end() || it->second.last_frame<oldest->second.last_frame))oldest=it;
        if(oldest==offscreen_sets_.end())break;
        ReleaseSelfShadowViews(oldest->second);   // I-0140 t31
        RetireAfterCurrentRecording(std::make_shared<ViewContext>(std::move(oldest->second)));offscreen_sets_.erase(oldest);}
    auto& set=offscreen_sets_[{width,height}];if(!set.width){set.width=width;set.height=height;}
    set.last_frame=numbered;
    // cD2: the view records into its own context; the main view's context is current again on every exit.
    view_=&set;
    struct Restore {SceneRenderer& owner;~Restore(){owner.view_=&owner.main_view_;}} restore{*this};
    try {
        NativeModelTargets();
        if(input.clear_stencil!=view_->clear_stencil){gpu_.Drain(WaitDepthRecreate);gpu_audit.depth_recreations.fetch_add(1,std::memory_order_relaxed);DepthTarget(width,height,input.clear_stencil);++view_->target_generation;
            ReleaseSelfShadowViews(*view_);}   // I-0140 t31 (its step review L5): the former depth's view goes with it
        NativeLightingResources(*input.lighting);
        // cD1: the slot holds the draws, the lighting input (its environment cubes) and the view target until its ticket.
        // cD3: the view records into the frame's single list (opened here when the view comes first), submitted by the frame.
        auto& slot=FrameListSlot(WaitOffscreen);SweepMenuAa(target);   // I-0140 tranche 40
        slot.draws.insert(slot.draws.end(),input.draws.begin(),input.draws.end());slot.draws.insert(slot.draws.end(),input.forward_draws.begin(),input.forward_draws.end());
        slot.kept.push_back(input.lighting);slot.kept.push_back(HoldResource(target));slot.referenced.push_back(target);
        auto* list=slot.list.Get();
        // I-0140 tranche 31 (D-0303): a Player menu view with its traced scene (the collect's: its ShadowParam's sun and its character's
        // instances): the scene held by the slot, the context's mask made before the graph declares it, the view's top level built on
        // the list (EncodeMenuRtScene: none, the view unshadowed this frame); `traced` its mask's trace and the shadowed sun pass, which
        // needs its programme (the step review's L4: without it the mask would be traced and not read).
        bool traced=false;
        if(input.rt_scene && input.shadow_blocks && native_sun_shadowed_pso_ && !menu_shadow_failed_.load(std::memory_order_acquire)){slot.kept.push_back(input.rt_scene);
            if(auto* const heap=rt_heap_.load(std::memory_order_acquire);heap && heap->IsOpen())try{MakeSelfShadowMask(*heap);}catch(const std::exception& error){FailMenuShadow(error.what());}
            traced=view_->self_mask && EncodeMenuRtScene(list,*input.rt_scene);}
        // The view's t32 bound before its graph is declared (the review of the fixes' fixes, M1): `traced` the frame's after it, the mask
        // declared exactly when the lighting reads it.
        BindSelfShadowViews(traced);traced=self_traced_now_;
        self_blur_passes_now_=traced?SelfShadowBlurPasses(input):0u;   // I-0140 t35: before the graph declares the blur target
        // I-0140 tranche 40 (D-0305): the view's TAA this frame (the collect's decision, with the view's camera motion): its stage (by the
        // view's target: its own history), the context's targets and views, the inputs checked, all before the graph declares them; a
        // failure latches every menu view's TAA off (the view composed from its lit HDR as before).
        auto* hdr=view_->light_targets[3].Get();
        TaaStage* const menu_taa=input.taa && input.velocity && hdr && tone.tabled?MenuTaaFor(target,input.serial):nullptr;   // tabled only (the step review's M1; the collect's rule too)
        const bool taa=menu_taa && [&]{
            try{MenuTaaResources();std::string reason;
                if(menu_taa->Prepare(hdr,view_->depth.Get(),view_->taa_motion.Get(),view_->taa_output.Get(),reason))return true;
                LatchMenuTaaOff(input.serial,"prepare: "+reason);}
            catch(const std::exception& e){LatchMenuTaaOff(input.serial,std::string("resources: ")+e.what());}
            return false;}();
        const D3D12_VIEWPORT viewport{0,0,static_cast<float>(width),static_cast<float>(height),0,1};
        const D3D12_RECT scissor{0,0,static_cast<LONG>(width),static_cast<LONG>(height)};
        std::array<D3D12_CPU_DESCRIPTOR_HANDLE,6> targets;
        for(unsigned i=0;i<targets.size();++i)targets[i]=CPU(view_->rtvs.Get(),rtv_stride_,i);
        // Phase E E2/E3: the view's recording under the graph's shadow (and executor), as the main view's.
        GraphShadow* graph{};const auto shader_states=static_cast<D3D12_RESOURCE_STATES>(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        if(graph_shadow_enabled_){graph=&graph_shadow_;graph->Begin(input.serial,list);DeclareViewGraph(*graph,true);
            graph->Declare(target,1,shader_states,FrameLifetime::External,"view_target",true);
            if(traced || SelfMaskBound())graph->Declare(view_->self_mask.Get(),1,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,FrameLifetime::Persistent,"self_mask");   // I-0140 t31
            if(self_blur_passes_now_)graph->Declare(view_->self_blur.Get(),1,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,FrameLifetime::Persistent,"self_blur");   // I-0140 t35
            if(taa){const auto rt=D3D12_RESOURCE_STATE_RENDER_TARGET;   // I-0140 tranche 40 (the history the stage's own, as the main view's)
                graph->Declare(view_->taa_velocity.Get(),1,rt,FrameLifetime::Transient,"menu_velocity");graph->Declare(view_->taa_motion.Get(),1,rt,FrameLifetime::Transient,"menu_motion");
                graph->Declare(view_->taa_output.Get(),1,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,FrameLifetime::Persistent,"menu_taa");}
            const bool sss=native_sss_psos_[0] && !input.lighting->sss_profiles.empty() && !input.lighting->sss_scene.empty();
            // I-0140 tranche 35: the blur's pass count (bits 11 to 14) selects the self-shadow's passes too.
            graph->Execute(uint64_t(1)<<8|uint64_t(1)|uint64_t(sss)<<1|uint64_t(!input.forward_draws.empty() || tone.tabled)<<2|uint64_t(!pixel_file.empty())<<6|uint64_t(traced)<<10|
                uint64_t(self_blur_passes_now_)<<11|uint64_t(taa)<<15);   // I-0140 tranche 40: bit 15 the view's TAA
            active_graph_shadow=graph;}
        struct ShadowScope {~ShadowScope(){active_graph_shadow=nullptr;}} shadow_scope;
        ClearGBuffer(list,targets,view_->dsv->GetCPUDescriptorHandleForHeapStart(),1.f,input.clear_stencil);
        if(graph){graph->Pass("gbuffer",PassKind::Material);for(const auto& color:view_->colors)graph->Use(color.Get(),ResourceAccess::Write,D3D12_RESOURCE_STATE_RENDER_TARGET);
            graph->Use(view_->depth.Get(),ResourceAccess::Write,D3D12_RESOURCE_STATE_DEPTH_WRITE);}
        NoteTargetBytes();   // E0: menu views too (character creation draws no main view)
        const auto frame_blocks=UploadNativeFrameBlocks(input);   // C4-1: once per view, for its three passes
        // C4-1b: every other constant payload copied once per view as its draws are recorded.
        NativeModelConstantUploads uploads([this](uint64_t bytes){const auto range=frame_uploads_.Allocate(gpu_.device.Get(),bytes,UploadArena::alignment,false);
            return NativeModelConstantUploads::Range{range.cpu,range.gpu};},frames_.Numbered(),frames_.CompletedThrough());   // C4-2a
        {GBufferBindState state;for(const auto& draw:input.draws)draw->Encode(list,targets,view_->dsv->GetCPUDescriptorHandleForHeapStart(),viewport,scissor,FrameAddressesOf(frame_blocks,NativeModelPass::Deferred),uploads,&state);}
        // I-0140 t31: the shadowed sun's b8 from the view's own shadow scene (FC_ShadowColor from its ShadowParam's colour, 13b).
        if(traced){const auto range=frame_uploads_.Allocate(gpu_.device.Get(),1024);std::memcpy(range.cpu,input.shadow_scene.data(),960);sun_scene_block_=range.gpu;}
        EncodeNativeLighting(list,*input.lighting,viewport,scissor,nullptr,traced?&input:nullptr);
        // I-0113 tranche 29: a tabled view (the GX tone map) has the original's background quad (menu_tonemap.h BackgroundQuadColour:
        // the BG Color squared, 18/255 -> 1/255) drawn into its lit HDR where the scene depth is 1, before the forward rows, so the
        // hair's forward rows that lie over the background blend onto it as in the original, and the composition then tone-maps
        // every pixel. The other views keep the composition where the scene depth is below 1 over the background clear (explicit policy,
        // the final review's L5: the original draws the quad for every menu view, but their tone map is not ported, so the unsquared
        // clear stays their background). The quad's alpha reaches the HDR only; the composition writes alpha 1 (the final review's L4).
        if(tone.tabled && !offscreen_composition_pso_)OffscreenCompositionPipeline();
        if(!input.forward_draws.empty() || tone.tabled){
            Transition(list,hdr,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);
            const std::array<D3D12_CPU_DESCRIPTOR_HANDLE,1> forward{CPU(view_->light_rtvs.Get(),rtv_stride_,3)};
            if(graph){graph->Pass("forward",PassKind::Material);graph->Use(hdr,ResourceAccess::ReadWrite,D3D12_RESOURCE_STATE_RENDER_TARGET);
                graph->Use(view_->depth.Get(),ResourceAccess::ReadWrite,D3D12_RESOURCE_STATE_DEPTH_WRITE);}
            if(tone.tabled){const auto quad=menu_tone::BackgroundQuadColour(background);
                const auto f=[](float x){uint32_t u{};std::memcpy(&u,&x,4);return u;};
                const uint32_t words[8]{0,0,0,0,f(quad[3]),f(quad[0]),f(quad[1]),f(quad[2])};   // b0's gamma word (alpha) and scaler.rgb (BackgroundPS)
                const auto depth=view_->dsv->GetCPUDescriptorHandleForHeapStart();list->OMSetRenderTargets(1,&forward[0],FALSE,&depth);
                list->SetPipelineState(offscreen_background_pso_.Get());list->SetGraphicsRootSignature(offscreen_composition_root_.Get());
                // The step review's L2: the root's table set too (not read by BackgroundPS; resource binding tier 1 wants every parameter set).
                ID3D12DescriptorHeap* heaps[]{view_->bindings.Get()};list->SetDescriptorHeaps(1,heaps);
                list->SetGraphicsRootDescriptorTable(0,GPU(view_->bindings.Get(),binding_stride_,composition_binding+1));
                list->SetGraphicsRoot32BitConstants(1,8,words,0);list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
                list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);list->DrawInstanced(3,1,0,0);}
            {GBufferBindState state;for(const auto& draw:input.forward_draws)draw->Encode(list,forward,view_->dsv->GetCPUDescriptorHandleForHeapStart(),viewport,scissor,FrameAddressesOf(frame_blocks,NativeModelPass::Forward),uploads,&state);}
            Transition(list,hdr,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }
        // I-0140 tranche 40 (D-0305): the view's TAA on its lit HDR once its forward rows are drawn, before the composition tone-maps it
        // (the main view's order): its camera motion resolved from the depth (the velocity target cleared: the camera path on every
        // pixel), then the stage into the context's output, which the composition reads instead (binding +2). The history restarts on the
        // camera's cut rule and whenever this view did not resolve the previous frame (its opening, a gap, a reused target address).
        ID3D12Resource* composed=hdr;
        if(taa){
            auto* depth=view_->depth.Get();auto* velocity=view_->taa_velocity.Get();auto* motion=view_->taa_motion.Get();auto* out=view_->taa_output.Get();
            for(auto* held:{velocity,motion,out})slot.kept.push_back(HoldResource(held));
            const auto psr=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,npsr=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,rt=D3D12_RESOURCE_STATE_RENDER_TARGET;
            const auto colour_read=psr|npsr,depth_read=D3D12_RESOURCE_STATE_DEPTH_READ|npsr,resolve_depth=D3D12_RESOURCE_STATE_DEPTH_READ|psr;
            // The frame's number read now, the list open (MenuTaaFor's; the step review's L1). A scene presented again without a new
            // collect (its selection version, camera.serial, the last one's) is the same picture under the same jitter: resolved with no
            // motion (the collect's still matrix) and its history kept, by the TAA as by the DLAA (the final review's L1).
            auto& entry=menu_taa_[target];const auto frame_number=frames_.Numbered();
            const bool repeated=entry.last_version==input.camera.serial,gap=entry.last_frame+1!=frame_number,cut=input.velocity->reset && !repeated;
            if(repeated)menu_aa_repeats_.fetch_add(1,std::memory_order_relaxed);
            const auto vectors=CPU(view_->taa_rtvs.Get(),rtv_stride_,0);const float neutral[4]{0.5f,0.5f,1.0f,0.0f};
            if(graph){graph->Pass("menu_velocity",PassKind::Clear);graph->Use(velocity,ResourceAccess::Write,rt);}
            list->ClearRenderTargetView(vectors,neutral,0,nullptr);
            Transition(list,velocity,rt,psr);DepthPlaneTransition(list,depth,D3D12_RESOURCE_STATE_DEPTH_WRITE,resolve_depth);
            if(graph){graph->Pass("menu_velocity_resolve",PassKind::Effect);graph->Use(velocity,ResourceAccess::Read,psr);
                graph->Use(depth,ResourceAccess::Read,resolve_depth,{0,1});graph->Use(motion,ResourceAccess::Write,rt);}
            {const auto constants=frame_uploads_.Allocate(gpu_.device.Get(),256);
                struct {Matrix4 matrix;std::array<float,4> scale,screen;} words{repeated?input.velocity->still_to_previous:input.velocity->current_to_previous,input.velocity->scale,
                    {float(width),float(height),1.0f/float(width),1.0f/float(height)}};std::memcpy(constants.cpu,&words,sizeof(words));
                const auto out_vectors=CPU(view_->taa_rtvs.Get(),rtv_stride_,1);
                ID3D12DescriptorHeap* heaps[]{view_->taa_views.Get()};list->SetDescriptorHeaps(1,heaps);
                list->OMSetRenderTargets(1,&out_vectors,FALSE,nullptr);list->SetPipelineState(velocity_resolve_pso_.Get());list->SetGraphicsRootSignature(velocity_root_.Get());
                list->SetGraphicsRootConstantBufferView(0,constants.gpu);list->SetGraphicsRootDescriptorTable(1,view_->taa_views->GetGPUDescriptorHandleForHeapStart());
                list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
                list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);list->DrawInstanced(3,1,0,0);}
            DepthPlaneTransition(list,depth,resolve_depth,D3D12_RESOURCE_STATE_DEPTH_WRITE);Transition(list,velocity,psr,rt);
            Transition(list,hdr,psr,colour_read);Transition(list,depth,D3D12_RESOURCE_STATE_DEPTH_WRITE,depth_read);Transition(list,motion,rt,npsr);
            if(graph){graph->Pass("menu_taa",PassKind::PostProcess);graph->Use(hdr,ResourceAccess::Read,colour_read);graph->Use(depth,ResourceAccess::Read,depth_read);
                graph->Use(motion,ResourceAccess::Read,npsr);graph->Use(out,ResourceAccess::Write,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);}
            TaaParams p{};p.jitter_x=input.camera.jitter_x;p.jitter_y=input.camera.jitter_y;
            // The perceptual weights at the view's own tone: a tabled view's x / (x + 0.2) of x = c scale is c e / (1 + c e) with
            // e = 5 scale; an untabled view is encoded unscaled.
            p.exposure=tone.tabled && tone.scale>0?5.0f*tone.scale:1.0f;
            p.reset=cut || gap || entry.dlss_last;p.sharpen=input.sharpen;
            p.kernel=input.taa_kernel;p.own_reset=true;   // I-0140 tranche 41 (D-0306): its own sample on an unjittered reset (the collect's), its kernel
            // Its step review's M2: a repeated presentation of a frame resolved as its own samples resolves again as one (the same picture,
            // unjittered), not blended from a reconstruction and clipped.
            if(repeated && entry.last_own && p.jitter_x==0.0f && p.jitter_y==0.0f)p.reset=true;
            // DLAA on the view while the option runs DLSS (the collect's choice, D-0305), on the view's own Streamline viewport, the
            // TAA's inputs in the TAA's states; its history restarts as the TAA's, and on a switch from the TAA. A refusal latches the
            // menu views' DLSS off and this frame resolves by the TAA (its history restarted). No GPU zone: the zones time the main view.
            // Each evaluation on its presentation's own frame token (the DLAA review's M2), a repeated version with no motion. The viewport
            // keyed by its inputs and the context's creations (an address reused after a remake: L3), reused across openings (MenuDlssViewport).
            bool dlaa=false,dlaa_reset=false;
            if(input.dlss && !menu_dlss_failed_ && streamline::MenuDlssReady()){
                const uint32_t viewport_id=MenuDlssViewport(entry,{hdr,depth,motion,out},{view_->target_generation,view_->taa_made},input.serial);
                streamline::DlssFrame f{};f.frame=uint32_t(input.serial);f.viewport=viewport_id;f.mode=streamline::DlssMode::Dlaa;
                f.color=hdr;f.depth=depth;f.motion=motion;f.output=out;
                f.color_state=colour_read;f.depth_state=depth_read;f.motion_state=npsr;f.output_state=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                f.render_width=f.output_width=width;f.render_height=f.output_height=height;f.jitter_x=p.jitter_x;f.jitter_y=p.jitter_y;
                f.reset=cut || gap || !entry.dlss_last || viewport_id!=entry.dlss_evaluated;   // a new viewport too (the fixes' review L2)
                // The unjittered lens (the collect's MenuViewMotion: the same projection), the previous clip from its history.
                VelocityMatrix view_matrix{},projection{},inverse{};const bool lens=VelocityViewAndProjection(input.camera,view_matrix,projection) && VelocityInverse(projection,inverse);
                if(lens){for(unsigned i=0;i<16;++i){f.view_to_clip[i]=float(projection[i]);f.clip_to_view[i]=float(inverse[i]);}
                    const Matrix4 still{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};const Matrix4& clip_to_previous=repeated?still:input.velocity->clip_to_previous_clip;
                    VelocityMatrix to_previous{};for(unsigned i=0;i<16;++i)to_previous[i]=clip_to_previous[i];
                    f.clip_to_previous_clip=clip_to_previous;
                    if(VelocityInverse(to_previous,inverse))for(unsigned i=0;i<16;++i)f.previous_clip_to_clip[i]=float(inverse[i]);}
                const auto& pose=input.camera.camera_pose;
                f.right={pose[0],pose[1],pose[2]};f.up={pose[4],pose[5],pose[6]};f.forward={pose[8],pose[9],pose[10]};f.position={pose[12],pose[13],pose[14]};
                f.near_plane=input.camera.near_plane;f.far_plane=input.camera.far_plane;f.vertical_fov=input.camera.vertical_fov;
                f.aspect=input.camera.explicit_aspect?input.camera.explicit_aspect:float(width)/float(height);
                std::string reason=lens?"":"menu DLSS: no lens";
                dlaa=lens && viewport_id && streamline::EvaluateDlss(list,f,reason);dlaa_reset=f.reset;
                if(!dlaa && viewport_id){LatchMenuDlssOff(input.serial,reason);p.reset=true;}
                // Thomas's sharpening on the DLAA output, in place, as on the world's DLSS (2026-11-11; the DLAA review's M1); a failure
                // latches only the menu views' DLAA sharpening off, its row once.
                if(dlaa && input.sharpen>=1e-3f && !menu_dlss_sharpen_failed_){
                    try{menu_taa->RecordSharpen(list,out,p.exposure,input.sharpen,frames_.CurrentIndex());slot.kept.push_back(HoldResource(menu_taa->SharpenScratch()));}
                    catch(const std::exception& e){if(!menu_dlss_sharpen_failed_.exchange(true) && upscaler_sink_){std::string why=std::string(e.what()).substr(0,200);
                        for(auto& ch:why)if(ch=='"' || ch==static_cast<char>(92) || ch<32)ch=' ';char row[320]{};
                        sprintf_s(row,"{\"event\":\"native_menu_dlss_sharpen_latched_off\",\"serial\":%llu,\"reason\":\"%s\"}%c",static_cast<unsigned long long>(input.serial),why.c_str(),10);upscaler_sink_(row);}}}}
            const bool reset=dlaa?dlaa_reset:menu_taa->Record(list,hdr,depth,motion,out,p,frames_.CurrentIndex());
            if(!dlaa)for(auto* history:menu_taa->History())if(history)slot.kept.push_back(HoldResource(history));
            entry.dlss_last=dlaa;if(dlaa)entry.dlss_evaluated=entry.dlss_viewport;
            const bool own=!dlaa && reset && p.jitter_x==0.0f && p.jitter_y==0.0f;entry.last_own=own;   // as Record's own_reset (taa_stage.cpp)
            if(dlaa)menu_dlss_frames_.fetch_add(1,std::memory_order_relaxed);
            Transition(list,motion,npsr,rt);Transition(list,depth,depth_read,D3D12_RESOURCE_STATE_DEPTH_WRITE);Transition(list,hdr,colour_read,psr);
            Transition(list,out,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,psr);
            entry.last_frame=frame_number;entry.last_version=input.camera.serial;composed=out;
            const auto n=menu_taa_frames_.fetch_add(1,std::memory_order_relaxed)+1;const auto resets=reset?menu_taa_resets_.fetch_add(1,std::memory_order_relaxed)+1:menu_taa_resets_.load(std::memory_order_relaxed);
            if(upscaler_sink_ && (n<=3 || n%600==0 || (reset && resets<=24) || (repeated && menu_aa_repeats_.load(std::memory_order_relaxed)<=3) || (dlaa &&menu_dlss_frames_.load(std::memory_order_relaxed)==1))){char row[384]{};   // the frames count both (dlaa: DLAA's)
                sprintf_s(row,"{\"event\":\"native_menu_taa_frames\",\"frames\":%llu,\"dlaa_frames\":%llu,\"dlaa\":%s,\"resets\":%llu,\"reset\":%s,\"serial\":%llu,\"width\":%u,\"height\":%u,\"views\":%zu,\"viewport\":%u,\"repeats\":%llu,\"jitter\":[%.4f,%.4f],\"kernel\":%.2f,\"own\":%s}\n",
                    static_cast<unsigned long long>(n),static_cast<unsigned long long>(menu_dlss_frames_.load(std::memory_order_relaxed)),dlaa?"true":"false",static_cast<unsigned long long>(resets),reset?"true":"false",
                    static_cast<unsigned long long>(input.serial),width,height,menu_taa_.size(),dlaa?entry.dlss_viewport:0u,static_cast<unsigned long long>(menu_aa_repeats_.load(std::memory_order_relaxed)),p.jitter_x,p.jitter_y,
                    dlaa?-1.0f:p.kernel>0.0f?p.kernel:2.29f,own?"true":"false");upscaler_sink_(row);}}   // kernel -1: DLAA's frame (no TAA reconstruction; its step review's L2)
        // The lit HDR composed into the view's target: over its background clear where the scene depth is below 1 (not tabled), or
        // over every pixel with no clear (a tabled view, tranche 29).
        // P3: the view context keeps its one-slot RTV heap; the view is rewritten each frame (an RTV is read at record time).
        if(!view_->output_rtv){const D3D12_DESCRIPTOR_HEAP_DESC rtv_desc{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,1};
            Check(CountedHeap(gpu_.device)->CreateDescriptorHeap(&rtv_desc,IID_PPV_ARGS(&view_->output_rtv)),"native offscreen composition view");}
        auto* rtv=view_->output_rtv.Get();
        gpu_.device->CreateRenderTargetView(target,nullptr,rtv->GetCPUDescriptorHandleForHeapStart());
        Transition(list,target,shader_states,D3D12_RESOURCE_STATE_RENDER_TARGET);
        // The composition reads the lit HDR and tests the scene depth (the view's DSV) while writing the view target.
        if(graph){graph->Pass("view_composition",PassKind::Composition);graph->Use(composed,ResourceAccess::Read,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            graph->Use(view_->depth.Get(),ResourceAccess::ReadWrite,D3D12_RESOURCE_STATE_DEPTH_WRITE);graph->Use(target,ResourceAccess::Write,D3D12_RESOURCE_STATE_RENDER_TARGET);}
        const auto output=rtv->GetCPUDescriptorHandleForHeapStart();const auto depth=view_->dsv->GetCPUDescriptorHandleForHeapStart();
        if(!tone.tabled)list->ClearRenderTargetView(output,background.data(),0,nullptr);   // a tabled view's composition writes every pixel (L7)
        list->OMSetRenderTargets(1,&output,FALSE,&depth);
        if(!offscreen_composition_pso_)OffscreenCompositionPipeline();
        ID3D12DescriptorHeap* composition_heaps[]{view_->bindings.Get()};list->SetDescriptorHeaps(1,composition_heaps);
        list->SetPipelineState(tone.tabled?offscreen_composition_full_pso_.Get():offscreen_composition_pso_.Get());list->SetGraphicsRootSignature(offscreen_composition_root_.Get());
        list->SetGraphicsRootDescriptorTable(0,GPU(view_->bindings.Get(),binding_stride_,composition_binding+(taa?2u:1u)));   // I-0140 tranche 40
        {const auto f=[](float v){uint32_t u{};std::memcpy(&u,&v,4);return u;};   // I-0113 tranche 27: the view's tone (b0)
         const uint32_t words[8]{tone.tabled?1u:0u,tone.mode,f(tone.scale),f(tone.contrast),f(tone.gamma),f(tone.scaler[0]),f(tone.scaler[1]),f(tone.scaler[2])};
         list->SetGraphicsRoot32BitConstants(1,8,words,0);}
        list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
        list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);list->DrawInstanced(3,1,0,0);
        if(!pixel_file.empty())RecordPixels(list,slot,target,shader_states,pixel_file,"native offscreen pixel evidence write");
        else Transition(list,target,D3D12_RESOURCE_STATE_RENDER_TARGET,shader_states);
        // The set's lit HDR returns to its frame-start state (render target), as after the main composition; the TAA's output to its own.
        Transition(list,hdr,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);
        if(taa)Transition(list,composed,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        if(graph){graph->Pass("view_output",PassKind::Present);graph->Use(target,ResourceAccess::Read,shader_states);active_graph_shadow=nullptr;FinishGraphShadow(*graph);}
    }catch(...){faulted_=true;throw;}   // a recording slot keeps what it holds (released at Close after the drain, or quarantined)
}
// cD3: the frame's single list. The movie and the menu views open it when they come first; the frame (the frame job or
// RenderStartupFrame) continues it, then submits it once.
FrameSlot& SceneRenderer::FrameListSlot(uint8_t site) {
    return frames_.Recording()?frames_.Current():frames_.Begin(gpu_,site);
}
// cD3 (sheet G3): a witness copy of `target` (left RENDER_TARGET by its pass) recorded into the frame list; the file is
// written when the slot is released (cD4: a completion of the slot, which holds the readback buffer).
void SceneRenderer::RecordPixels(ID3D12GraphicsCommandList* list,FrameSlot& slot,ID3D12Resource* target,D3D12_RESOURCE_STATES after,
    const std::wstring& path,const char* failure,uint32_t pixel_bytes,D3D12_RESOURCE_STATES before,bool tracked) {
    // A volume's slices follow each other in the footprint (RowPitch x Height apart): its rows are written slice after slice.
    const auto desc=target->GetDesc();const auto width=static_cast<uint32_t>(desc.Width),
        height=desc.Height*(desc.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE3D?desc.DepthOrArraySize:1u);
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT64 total{};gpu_.device->GetCopyableFootprints(&desc,0,1,0,&footprint,nullptr,nullptr,&total);
    const auto readback=NewBuffer(gpu_.device.Get(),total,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    Transition(list,target,before,D3D12_RESOURCE_STATE_COPY_SOURCE);
    if(auto* graph=tracked?active_graph_shadow:nullptr){graph->Pass("pixel_readback",PassKind::Readback);graph->Use(target,ResourceAccess::Read,D3D12_RESOURCE_STATE_COPY_SOURCE);}
    D3D12_TEXTURE_COPY_LOCATION from{},to{};from.pResource=target;from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    to.pResource=readback.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint=footprint;
    list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
    Transition(list,target,D3D12_RESOURCE_STATE_COPY_SOURCE,after);
    // cD4: the file (raw rows, width*pixel_bytes bytes each) is written at the slot's release; the completion holds the buffer.
    // I-0120 E4: a witness picture that cannot be mapped or written (a full disk) is dropped with a line on the debug output, not
    // thrown from the slot's completion (which faulted the renderer: rendering stopped for a test picture).
    slot.completions.push_back([readback,footprint,width,height,path,failure,pixel_bytes]{
        const D3D12_RANGE range{0,static_cast<SIZE_T>(footprint.Offset+uint64_t(footprint.Footprint.RowPitch)*(height-1)+uint64_t(width)*pixel_bytes)};
        void* memory{};if(FAILED(readback->Map(0,&range,&memory)) || !memory){OutputDebugStringA("witness readback map failed: picture dropped\n");return;}
        std::ofstream file(path,std::ios::binary);
        for(uint32_t row=0;row<height && file;++row)
            file.write(static_cast<const char*>(memory)+footprint.Offset+uint64_t(row)*footprint.Footprint.RowPitch,static_cast<std::streamsize>(width)*pixel_bytes);
        const D3D12_RANGE no_write{};readback->Unmap(0,&no_write);
        if(!file){OutputDebugStringA(failure);OutputDebugStringA(": picture dropped\n");}});
}
// I-0113 tranche 29 (effect-scan.flag): `target` (an R16G16B16A16_FLOAT picture in `state`, left in it) copied into this slot's
// readback buffer of `stage`, scanned at the slot's release (effect_scan.h); a picture with a NaN, an inf or a negative gives a
// row (at most kEffectScanRows a window). Its barriers are local (no graph pass: a diagnostic, as tranche 23's CopyRect). A
// buffer that cannot be made skips the scan.
void SceneRenderer::RecordEffectScan(ID3D12GraphicsCommandList* list,FrameSlot& slot,ID3D12Resource* target,D3D12_RESOURCE_STATES state,uint32_t stage,uint32_t pass,uint64_t serial) {
    const auto desc=target->GetDesc();const uint32_t index=frames_.CurrentIndex();
    const auto skip=[&]{const std::lock_guard hold(effects_lock_);++effect_scan_.skipped;};
    if(effect_scan_failed_ || desc.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT || desc.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D || stage>3 || index>=max_frame_slots){skip();return;}
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT64 total{};gpu_.device->GetCopyableFootprints(&desc,0,1,0,&footprint,nullptr,nullptr,&total);
    if(total!=effect_scan_bytes_){for(auto& pair:effect_scan_readback_)for(auto& b:pair)if(b){slot.kept.push_back(HoldResource(b.Get()));b.Reset();}effect_scan_bytes_=total;}
    auto& readback=effect_scan_readback_[index][stage];
    if(!readback)try{readback=NewBuffer(gpu_.device.Get(),total,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);}catch(const std::exception&){effect_scan_failed_=true;skip();return;}
    D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={target,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,state,D3D12_RESOURCE_STATE_COPY_SOURCE};
    list->ResourceBarrier(1,&b);
    D3D12_TEXTURE_COPY_LOCATION from{},to{};from.pResource=target;from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    to.pResource=readback.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint=footprint;
    list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
    std::swap(b.Transition.StateBefore,b.Transition.StateAfter);list->ResourceBarrier(1,&b);
    slot.kept.push_back(HoldResource(readback.Get()));
    slot.completions.push_back([this,buffer=readback,footprint,stage,pass,serial]{
        const uint32_t width=footprint.Footprint.Width,height=footprint.Footprint.Height;
        const D3D12_RANGE range{0,static_cast<SIZE_T>(footprint.Offset+uint64_t(footprint.Footprint.RowPitch)*(height-1)+uint64_t(width)*8)};
        void* memory{};if(FAILED(buffer->Map(0,&range,&memory))){const std::lock_guard hold(effects_lock_);++effect_scan_.skipped;return;}   // the step review's L6
        const auto counts=effect_scan::Scan(static_cast<const uint8_t*>(memory)+footprint.Offset,footprint.Footprint.RowPitch,width,height);
        const D3D12_RANGE none{};buffer->Unmap(0,&none);
        const std::lock_guard hold(effects_lock_);auto& w=effect_scan_;++w.frames;
        if(!counts.Any())return;
        ++w.flagged;if(w.rows.size()>=kEffectScanRows)return;
        w.rows.push_back(EffectScanRow{serial,stage,pass,width,height,counts.nan,counts.inf,counts.neg,counts.bad_box,counts.neg_box});});
}
// I-0113 tranche 30 (effect-scan-draws.flag, gap 16): the per-statement count of the lit HDR's negatives (EffectDrawScanRow). Each
// point has 64 bytes in the counts buffer: +0..+12 the pixels with a negative R, G, B, A; +16 the pixels with any negative channel;
// +20..+28 the most negative R, G, B as the bits of their magnitude (a non-negative float orders as its bits); +32 and +36
// width - x and height - y maxed (the box's left and top), +40 and +48 x + 1 and y + 1 maxed (its right and bottom), +44 the
// pixels with a NaN. The buffer lives in COMMON between frames (cleared from a zero upload at Begin, copied out at End).
// Its programme: shaders/effect_draw_scan.hlsl.
bool SceneRenderer::BeginEffectDrawScan(ID3D12GraphicsCommandList* list,FrameSlot& slot,ID3D12Resource* hdr) {
    draw_scan_points_.clear();draw_scan_target_=nullptr;
    const uint32_t index=frames_.CurrentIndex();auto* device=gpu_.device.Get();
    const auto skip=[&]{const std::lock_guard hold(effects_lock_);++effect_scan_.draw_skipped;return false;};
    if(draw_scan_failed_ || !hdr || index>=max_frame_slots)return skip();
    const auto desc=hdr->GetDesc();if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D || ForwardTargetFormat()!=DXGI_FORMAT_R16G16B16A16_FLOAT)return skip();
    const uint64_t bytes=uint64_t(kEffectDrawScanPoints)*kEffectDrawScanStride;
    if(!draw_scan_pso_)try{draw_scan_failed_=true;   // latched unless everything is made
        D3D12_DESCRIPTOR_RANGE range{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,0,0,0};D3D12_ROOT_PARAMETER p[3]{};
        p[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[0].DescriptorTable={1,&range};p[0].ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
        p[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;p[1].Descriptor={0,0};p[1].ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
        p[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;p[2].Constants={0,0,3};p[2].ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
        const D3D12_ROOT_SIGNATURE_DESC root{3,p,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
        ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&root,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"effect draw scan root serialization");
        // The final review's L7: a diagnostic's root and pipeline made by the device, not kept in the saved pipeline list.
        Check(device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(draw_scan_root_.ReleaseAndGetAddressOf())),"effect draw scan root");
        D3D12_COMPUTE_PIPELINE_STATE_DESC d{};d.pRootSignature=draw_scan_root_.Get();d.CS={effect_draw_scan_cs_dxbc,sizeof(effect_draw_scan_cs_dxbc)};
        Check(device->CreateComputePipelineState(&d,IID_PPV_ARGS(draw_scan_pso_.ReleaseAndGetAddressOf())),"effect draw scan pipeline");
        const D3D12_DESCRIPTOR_HEAP_DESC views{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,max_frame_slots,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
        Check(CountedHeap(device)->CreateDescriptorHeap(&views,IID_PPV_ARGS(&draw_scan_views_)),"effect draw scan views");
        D3D12_HEAP_PROPERTIES defaults{};defaults.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC b{};b.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;b.Width=bytes;
        b.Height=1;b.DepthOrArraySize=1;b.MipLevels=1;b.SampleDesc.Count=1;b.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;b.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        Check(CountedResource(device)->CreateCommittedResource(&defaults,D3D12_HEAP_FLAG_NONE,&b,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&draw_scan_counts_)),"effect draw scan counts");
        draw_scan_failed_=false;}
    catch(const std::exception&){draw_scan_pso_.Reset();return skip();}
    auto& readback=draw_scan_readback_[index];
    if(!readback)try{readback=NewBuffer(device,bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);}catch(const std::exception&){draw_scan_failed_=true;return skip();}
    UploadArena::Range zero{};try{zero=frame_uploads_.Allocate(device,bytes,UploadArena::alignment,true);}catch(const std::exception&){}   // the step review's L5
    if(!zero.resource)return skip();
    D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Format=ForwardTargetFormat();v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    v.Texture2D.MipLevels=1;auto at=draw_scan_views_->GetCPUDescriptorHandleForHeapStart();at.ptr+=SIZE_T(index)*device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    device->CreateShaderResourceView(hdr,&v,at);   // this slot's view: the slot's earlier list completed before it records again
    D3D12_RESOURCE_BARRIER t{};t.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;t.Transition={draw_scan_counts_.Get(),0,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_DEST};
    list->ResourceBarrier(1,&t);list->CopyBufferRegion(draw_scan_counts_.Get(),0,zero.resource,zero.offset,bytes);
    t.Transition.StateBefore=D3D12_RESOURCE_STATE_COPY_DEST;t.Transition.StateAfter=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;list->ResourceBarrier(1,&t);
    slot.kept.push_back(HoldResource(draw_scan_counts_.Get()));slot.kept.push_back(HoldResource(readback.Get()));slot.kept.push_back(HoldObject(draw_scan_views_.Get()));
    draw_scan_target_=hdr;draw_scan_width_=static_cast<uint32_t>(desc.Width);draw_scan_height_=desc.Height;
    return true;
}
void SceneRenderer::RecordEffectDrawScanPoint(ID3D12GraphicsCommandList* list,uint32_t kind,int32_t pass,uint32_t key,const NativeEffectDraw* draw) {
    if(!draw_scan_target_)return;
    if(draw_scan_points_.size()>=kEffectDrawScanPoints-(kind==3 || kind==5?0:1)){const std::lock_guard hold(effects_lock_);++effect_scan_.draw_capped;return;}   // the step review's L2
    EffectDrawScanPoint p{};p.kind=kind;p.pass=pass;p.key=key;
    if(draw){p.op=uint32_t(draw->op);p.blend=draw->blend;p.depth=draw->depth;p.raster=draw->raster;p.targets=draw->targets;p.vs=draw->vs.key;p.ps=draw->ps.key;
        p.vs_bytes=draw->vs.bytes;p.ps_bytes=draw->ps.bytes;
        for(uint32_t t=0;t<5;++t)p.textures[t]=uint8_t(draw->textures[t].kind);}
    const uint32_t point=static_cast<uint32_t>(draw_scan_points_.size());draw_scan_points_.push_back(std::move(p));
    D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition={draw_scan_target_,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE};list->ResourceBarrier(1,&b);
    ID3D12DescriptorHeap* heaps[]{draw_scan_views_.Get()};list->SetDescriptorHeaps(1,heaps);
    list->SetComputeRootSignature(draw_scan_root_.Get());list->SetPipelineState(draw_scan_pso_.Get());
    auto gpu=draw_scan_views_->GetGPUDescriptorHandleForHeapStart();gpu.ptr+=uint64_t(frames_.CurrentIndex())*gpu_.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    list->SetComputeRootDescriptorTable(0,gpu);list->SetComputeRootUnorderedAccessView(1,draw_scan_counts_->GetGPUVirtualAddress());
    const uint32_t constants[3]{point,draw_scan_width_,draw_scan_height_};list->SetComputeRoot32BitConstants(2,3,constants,0);
    list->Dispatch((draw_scan_width_+7)/8,(draw_scan_height_+7)/8,1);
    std::swap(b.Transition.StateBefore,b.Transition.StateAfter);list->ResourceBarrier(1,&b);   // the dispatch's reads done before the next draw writes
}
void SceneRenderer::EndEffectDrawScan(ID3D12GraphicsCommandList* list,FrameSlot& slot,uint64_t serial) {
    if(!draw_scan_target_)return;draw_scan_target_=nullptr;
    const uint32_t index=frames_.CurrentIndex();auto readback=draw_scan_readback_[index];const uint32_t points=static_cast<uint32_t>(draw_scan_points_.size());
    D3D12_RESOURCE_BARRIER t{};t.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;t.Transition={draw_scan_counts_.Get(),0,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE};
    list->ResourceBarrier(1,&t);
    if(points)list->CopyBufferRegion(readback.Get(),0,draw_scan_counts_.Get(),0,uint64_t(points)*kEffectDrawScanStride);
    t.Transition.StateBefore=D3D12_RESOURCE_STATE_COPY_SOURCE;t.Transition.StateAfter=D3D12_RESOURCE_STATE_COMMON;list->ResourceBarrier(1,&t);
    if(!points)return;
    const uint32_t width=draw_scan_width_,height=draw_scan_height_;
    slot.completions.push_back([this,readback,serial,width,height,described=std::move(draw_scan_points_)]{
        const uint32_t n=static_cast<uint32_t>(described.size());const D3D12_RANGE range{0,SIZE_T(n)*kEffectDrawScanStride};void* memory{};
        if(FAILED(readback->Map(0,&range,&memory))){const std::lock_guard hold(effects_lock_);++effect_scan_.draw_skipped;return;}
        std::vector<uint32_t> words(size_t(n)*kEffectDrawScanStride/4);std::memcpy(words.data(),memory,words.size()*4);
        const D3D12_RANGE none{};readback->Unmap(0,&none);
        const std::lock_guard hold(effects_lock_);auto& w=effect_scan_;++w.draw_frames;w.draw_points+=n;
        uint32_t previous=0;std::array<uint32_t,4> channels_before{};std::array<float,3> peak_before{};
        for(uint32_t i=0;i<n;++i){const uint32_t* c=words.data()+size_t(i)*kEffectDrawScanStride/4;const uint32_t pixels=c[4];
            std::array<float,3> peak{};for(int k=0;k<3;++k)std::memcpy(&peak[k],&c[5+k],4);
            bool grew=pixels>previous+kEffectDrawScanGrowth || (i==0 && pixels);
            for(int k=0;k<4;++k)grew|=c[k]>channels_before[k]+kEffectDrawScanGrowth;
            for(int k=0;k<3;++k)grew|=peak[k]>peak_before[k]*1.25f && peak[k]>peak_before[k]+0.05f;
            if(grew){++w.draw_flagged;
                if(w.draw_rows.size()<kEffectDrawScanRows){EffectDrawScanRow r{};r.serial=serial;r.point=i;r.points=n;r.at=described[i];r.pixels=pixels;r.before=previous;r.nan=c[11];
                    r.neg={c[0],c[1],c[2],c[3]};if(pixels)r.box={c[8]?width-c[8]:0,c[9]?height-c[9]:0,c[10],c[12]};r.peak=peak;r.peak_before=peak_before;
                    w.draw_rows.push_back(std::move(r));}}
            previous=pixels;channels_before={c[0],c[1],c[2],c[3]};peak_before=peak;}});
    draw_scan_points_=std::vector<EffectDrawScanPoint>{};
}
void SceneRenderer::RenderNativeMovie(const NativeMovieFrame& frame,ID3D12Resource* target,const std::wstring& pixel_file) {
    if(!open_ || faulted_ || !target || !frame.width || !frame.height || !frame.luma_width || !frame.luma_height ||
       !frame.chroma_width || !frame.chroma_height || frame.vertex_shader.empty() || frame.pixel_shader.empty() ||
       frame.luma.size()!=size_t(frame.luma_width)*frame.luma_height ||
       frame.chroma_r.size()!=size_t(frame.chroma_width)*frame.chroma_height || frame.chroma_b.size()!=frame.chroma_r.size())
        throw std::invalid_argument("native movie frame admission");
    const auto desc=target->GetDesc();
    if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D || desc.Format!=DXGI_FORMAT_R8G8B8A8_UNORM ||
       desc.Width!=frame.width || desc.Height!=frame.height)throw std::invalid_argument("native movie output target");
    try {
        auto* device=gpu_.device.Get();
        if(!movie_root_){
            // b0 (VS and PS, the 144-byte block), t0..t3 (PS: Y, cR, cB, A), static s0: MIN_MAG_LINEAR_MIP_POINT, clamp
            // (the GX sampler of 141AE0830: filter 6 -> 0x14, address 2 -> CLAMP), LOD 0..FLT_MAX (BINK-NATIVE-READY 4).
            D3D12_DESCRIPTOR_RANGE range{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,4,0,0,0};
            D3D12_ROOT_PARAMETER parameters[2]{};
            parameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;parameters[0].Descriptor={0,0};parameters[0].ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
            parameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[1].DescriptorTable={1,&range};parameters[1].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
            D3D12_STATIC_SAMPLER_DESC sampler{};sampler.Filter=D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
            sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;sampler.MaxLOD=D3D12_FLOAT32_MAX;
            sampler.ComparisonFunc=D3D12_COMPARISON_FUNC_NEVER;sampler.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
            D3D12_ROOT_SIGNATURE_DESC root{2,parameters,1,&sampler,D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT};
            ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&root,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"movie root serialization");
            Check(ds3native::Pipelines().Root(device,blob->GetBufferPointer(),blob->GetBufferSize(),movie_root_.ReleaseAndGetAddressOf()),"movie root creation");
            const D3D12_INPUT_ELEMENT_DESC element{"POSITION",0,DXGI_FORMAT_R32G32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0};
            D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};pso.pRootSignature=movie_root_.Get();
            pso.VS={frame.vertex_shader.data(),frame.vertex_shader.size()};pso.PS={frame.pixel_shader.data(),frame.pixel_shader.size()};
            pso.BlendState.RenderTarget[0].RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;pso.SampleMask=UINT_MAX;
            pso.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;pso.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;pso.RasterizerState.DepthClipEnable=TRUE;
            pso.RasterizerState.MultisampleEnable=TRUE;   // mirrors the GX raster desc (0x10101); no effect at 1 sample
            pso.InputLayout={&element,1};pso.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            pso.NumRenderTargets=1;pso.RTVFormats[0]=DXGI_FORMAT_R8G8B8A8_UNORM;pso.SampleDesc.Count=1;
            Check(ds3native::Pipelines().Graphics(device,pso,movie_pso_.ReleaseAndGetAddressOf()),"movie pipeline");
            movie_quad_=Upload(device,frame.quad.data(),sizeof(frame.quad));
            const D3D12_DESCRIPTOR_HEAP_DESC views{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,4,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
            Check(CountedHeap(device)->CreateDescriptorHeap(&views,IID_PPV_ARGS(&movie_views_)),"movie plane views");
        }
        const auto view_stride=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        const std::array<uint32_t,4> sizes{frame.luma_width,frame.luma_height,frame.chroma_width,frame.chroma_height};
        if(sizes!=movie_plane_sizes_ || !movie_planes_[0]){
            // cD3 review: the planes and their shader-visible views are replaced in place, after the drain of submitted work;
            // no list still recording may hold them (the movie records first in the frame).
            if(frames_.Recording())throw std::logic_error("native movie plane recreation inside an open frame list");
            gpu_.Drain(WaitMoviePlanes);
            for(size_t plane=0;plane<3;++plane){const auto w=plane?frame.chroma_width:frame.luma_width,h=plane?frame.chroma_height:frame.luma_height;
                const auto heap=Heap(D3D12_HEAP_TYPE_DEFAULT);const auto image_desc=Image(w,h,1,DXGI_FORMAT_R8_UNORM);
                Check(CountedResource(device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&image_desc,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,nullptr,
                    IID_PPV_ARGS(&movie_planes_[plane])),"movie plane");
                device->CreateShaderResourceView(movie_planes_[plane].Get(),nullptr,CPU(movie_views_.Get(),view_stride,static_cast<uint32_t>(plane)));}
            // t3 (the alpha plane): never bound by the opaque programme 141ADFF10 draws with; a null view keeps the table complete.
            D3D12_SHADER_RESOURCE_VIEW_DESC none{};none.Format=DXGI_FORMAT_R8_UNORM;none.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
            none.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;none.Texture2D.MipLevels=1;
            device->CreateShaderResourceView(nullptr,&none,CPU(movie_views_.Get(),view_stride,3));
            movie_plane_sizes_=sizes;}
        // cD3 (B2d): the three planes at aligned placed footprints, then the constant block, in one range of this frame's
        // arena (512-byte aligned for the footprints, written whole), retired with the frame's single submission.
        std::array<D3D12_PLACED_SUBRESOURCE_FOOTPRINT,3> footprints{};UINT64 offset{};
        for(size_t plane=0;plane<3;++plane){const auto plane_desc=movie_planes_[plane]->GetDesc();UINT64 bytes{};
            device->GetCopyableFootprints(&plane_desc,0,1,offset,&footprints[plane],nullptr,nullptr,&bytes);
            offset=(footprints[plane].Offset+bytes+D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT-1)&~UINT64(D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT-1);}
        const UINT64 constants_offset=offset,total=offset+256;
        const auto upload=frame_uploads_.Allocate(device,total,D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT,false);
        {const std::array<const std::vector<uint8_t>*,3> sources{&frame.luma,&frame.chroma_r,&frame.chroma_b};
            for(size_t plane=0;plane<3;++plane){auto& footprint=footprints[plane];const auto w=footprint.Footprint.Width;
                for(UINT row=0;row<footprint.Footprint.Height;++row)
                    std::memcpy(upload.cpu+footprint.Offset+UINT64(row)*footprint.Footprint.RowPitch,sources[plane]->data()+size_t(row)*w,w);
                footprint.Offset+=upload.offset;}   // the copy reads the chunk at the range's offset
            std::memset(upload.cpu+constants_offset,0,256);std::memcpy(upload.cpu+constants_offset,frame.constants.data(),sizeof(frame.constants));}
        if(!movie_rtv_){const D3D12_DESCRIPTOR_HEAP_DESC rtv_desc{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,1};   // one slot, rewritten each frame
            Check(CountedHeap(device)->CreateDescriptorHeap(&rtv_desc,IID_PPV_ARGS(&movie_rtv_)),"movie output view");}
        auto* rtv=movie_rtv_.Get();
        device->CreateRenderTargetView(target,nullptr,rtv->GetCPUDescriptorHandleForHeapStart());
        // cD3: the movie opens the frame's single list (it comes first in the frame); the frame submits it.
        auto& slot=FrameListSlot(WaitMovie);slot.kept.push_back(HoldResource(target));slot.referenced.push_back(target);auto* list=slot.list.Get();
        // Phase E E2/E3: the movie's recording under the graph's shadow (and executor): its planes (persistent, shader-readable
        // between frames) and its target (the ingress record, presented to the UI in the shader states).
        const auto movie_states=static_cast<D3D12_RESOURCE_STATES>(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        GraphShadow* graph{};
        if(graph_shadow_enabled_){graph=&graph_shadow_;graph->Begin(++graph_recordings_,list);const char* plane_names[3]{"movie_y","movie_cr","movie_cb"};
            for(size_t plane=0;plane<3;++plane)graph->Declare(movie_planes_[plane].Get(),1,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,FrameLifetime::Persistent,plane_names[plane]);
            graph->Declare(target,1,movie_states,FrameLifetime::External,"movie_target",true);
            graph->Execute(uint64_t(1)<<9|uint64_t(!pixel_file.empty())<<6);active_graph_shadow=graph;
            graph->Pass("movie_upload",PassKind::Upload);}
        struct ShadowScope {~ShadowScope(){active_graph_shadow=nullptr;}} shadow_scope;
        for(size_t plane=0;plane<3;++plane){
            Transition(list,movie_planes_[plane].Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_DEST);
            if(graph)graph->Use(movie_planes_[plane].Get(),ResourceAccess::Write,D3D12_RESOURCE_STATE_COPY_DEST);
            D3D12_TEXTURE_COPY_LOCATION to{},from{};to.pResource=movie_planes_[plane].Get();to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            from.pResource=upload.resource;from.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;from.PlacedFootprint=footprints[plane];
            list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
            Transition(list,movie_planes_[plane].Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);}
        const auto shader_states=static_cast<D3D12_RESOURCE_STATES>(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        Transition(list,target,shader_states,D3D12_RESOURCE_STATE_RENDER_TARGET);
        if(graph){graph->Pass("movie_draw",PassKind::Effect);for(const auto& plane:movie_planes_)graph->Use(plane.Get(),ResourceAccess::Read,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            graph->Use(target,ResourceAccess::Write,D3D12_RESOURCE_STATE_RENDER_TARGET);}
        const auto output=rtv->GetCPUDescriptorHandleForHeapStart();list->OMSetRenderTargets(1,&output,FALSE,nullptr);
        ID3D12DescriptorHeap* heaps[]{movie_views_.Get()};list->SetDescriptorHeaps(1,heaps);
        list->SetPipelineState(movie_pso_.Get());list->SetGraphicsRootSignature(movie_root_.Get());
        list->SetGraphicsRootConstantBufferView(0,upload.gpu+constants_offset);
        list->SetGraphicsRootDescriptorTable(1,GPU(movie_views_.Get(),view_stride,0));
        const D3D12_VIEWPORT viewport{0,0,static_cast<float>(frame.width),static_cast<float>(frame.height),0,1};
        const D3D12_RECT scissor{0,0,static_cast<LONG>(frame.width),static_cast<LONG>(frame.height)};
        list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
        const D3D12_VERTEX_BUFFER_VIEW quad{movie_quad_->GetGPUVirtualAddress(),sizeof(frame.quad),8};
        list->IASetVertexBuffers(0,1,&quad);list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);list->DrawInstanced(4,1,0,0);
        if(!pixel_file.empty())RecordPixels(list,slot,target,shader_states,pixel_file,"native movie pixel evidence write");
        else Transition(list,target,D3D12_RESOURCE_STATE_RENDER_TARGET,shader_states);
        if(graph){graph->Pass("movie_output",PassKind::Present);graph->Use(target,ResourceAccess::Read,movie_states);active_graph_shadow=nullptr;FinishGraphShadow(*graph);}
    }catch(...){faulted_=true;throw;}
}
}
