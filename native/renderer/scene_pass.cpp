// The scene renderer (scene_pass.h), its first part: the composition pipelines, the targets, the presentation
// and its pacing, the model targets, the startup frame, the primitives, resize, display modes and close (the other parts are
// scene_pass_lighting, _velocity, _decals, _views and _frame; scene_pass_internal.h has the shared helpers).
#include "scene_pass_internal.h"
#include "streamline_host.h"
#include "../world/menu_tonemap.h"
// I-0132 R12h step 2: the programmes built by fxc (shaders/material_composition.hlsl, shaders/offscreen_composition.hlsl).
#include "material_composition_vs.dxbc.h"
#include "material_composition_ps.dxbc.h"
#include "offscreen_composition_vs.dxbc.h"
#include "offscreen_composition_ps.dxbc.h"
#include "offscreen_composition_backgroundps.dxbc.h"

namespace ds3native {
namespace {
// cD8 review: QPC ticks and milliseconds since them (present_call_ms).
LARGE_INTEGER Ticks() noexcept {LARGE_INTEGER now{};QueryPerformanceCounter(&now);return now;}
double MsSince(const LARGE_INTEGER& before) noexcept {LARGE_INTEGER frequency{};QueryPerformanceFrequency(&frequency);
    return 1000.0*double(Ticks().QuadPart-before.QuadPart)/double(frequency.QuadPart);}
}
void SceneRenderer::MaterialColorCompositionPipeline() {
    // Native composition policy for the first owned pass: display its material
    // color. Lighting/postprocessing ownership will extend this native stage.
    D3D12_DESCRIPTOR_RANGE composition_range{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,0,0,0};
    D3D12_ROOT_PARAMETER composition_parameter{};composition_parameter.ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    composition_parameter.DescriptorTable={1,&composition_range};composition_parameter.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_ROOT_SIGNATURE_DESC desc{1,&composition_parameter,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
    ComPtr<ID3DBlob> blob,error;
    Check(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"composition root serialization");
    Check(ds3native::Pipelines().Root(gpu_.device.Get(),blob->GetBufferPointer(),blob->GetBufferSize(),composition_root_.ReleaseAndGetAddressOf()),"composition root");
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};Blend(pso);pso.pRootSignature=composition_root_.Get();pso.VS={material_composition_vs_dxbc,sizeof(material_composition_vs_dxbc)};
    pso.PS={material_composition_ps_dxbc,sizeof(material_composition_ps_dxbc)};pso.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;
    pso.NumRenderTargets=1;pso.RTVFormats[0]=DXGI_FORMAT_R8G8B8A8_UNORM;
    Check(ds3native::Pipelines().Graphics(gpu_.device.Get(),pso,composition_pso_.ReleaseAndGetAddressOf()),"native composition pipeline");
}
void SceneRenderer::OffscreenCompositionPipeline() {
    // Menu views (1169): the lit HDR drawn at depth 1 with GREATER against the scene depth (cleared to 1, not written), so only
    // pixels the scene covered replace the view's background clear. I-0113 tranche 27: encoded by the view's GX tone map
    // (NativeOffscreenTone, root constants b0) when it is tabled, by the sRGB encode otherwise. The table is TabledToneMapPS's:
    // the writer's 1024 texel centres, quantized to 8 bits (the INFERRED UNORM format), read with linear filtering and clamp
    // (SS_ClampLinear), so each output channel lerps two evaluated entries (world/menu_tonemap.cpp, its CPU reference).
    D3D12_DESCRIPTOR_RANGE range{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,0,0,0};
    D3D12_ROOT_PARAMETER parameters[2]{};parameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    parameters[0].DescriptorTable={1,&range};parameters[0].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
    parameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;parameters[1].Constants={0,0,8};parameters[1].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
    const D3D12_ROOT_SIGNATURE_DESC root_desc{2,parameters,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
    ComPtr<ID3DBlob> blob,error;
    Check(D3D12SerializeRootSignature(&root_desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"offscreen composition root serialization");
    Check(ds3native::Pipelines().Root(gpu_.device.Get(),blob->GetBufferPointer(),blob->GetBufferSize(),offscreen_composition_root_.ReleaseAndGetAddressOf()),"offscreen composition root");
    // The programmes: shaders/offscreen_composition.hlsl (menu_tonemap_test reads the same built bytes).
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};Blend(pso);pso.pRootSignature=offscreen_composition_root_.Get();pso.VS={offscreen_composition_vs_dxbc,sizeof(offscreen_composition_vs_dxbc)};
    pso.PS={offscreen_composition_ps_dxbc,sizeof(offscreen_composition_ps_dxbc)};pso.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;
    pso.DepthStencilState.DepthEnable=TRUE;pso.DepthStencilState.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ZERO;
    pso.DepthStencilState.DepthFunc=D3D12_COMPARISON_FUNC_GREATER;pso.DSVFormat=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    pso.NumRenderTargets=1;pso.RTVFormats[0]=DXGI_FORMAT_R8G8B8A8_UNORM;
    Check(ds3native::Pipelines().Graphics(gpu_.device.Get(),pso,offscreen_composition_pso_.ReleaseAndGetAddressOf()),"native offscreen composition pipeline");
    // I-0113 tranche 29: the tabled views' composition over every pixel (the original's tone map at key 0x39: no depth or stencil),
    // and their background quad (key 0x1A, depth bank +0x1E8: LESS_EQUAL, no write; the quad at depth 1, so it covers the pixels
    // the scene left at the clear depth) into the lit HDR.
    pso.DepthStencilState.DepthEnable=FALSE;pso.DepthStencilState.DepthFunc=D3D12_COMPARISON_FUNC_ALWAYS;
    Check(ds3native::Pipelines().Graphics(gpu_.device.Get(),pso,offscreen_composition_full_pso_.ReleaseAndGetAddressOf()),"native offscreen full composition pipeline");
    pso.PS={offscreen_composition_backgroundps_dxbc,sizeof(offscreen_composition_backgroundps_dxbc)};pso.RTVFormats[0]=ForwardTargetFormat();
    pso.DepthStencilState.DepthEnable=TRUE;pso.DepthStencilState.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ZERO;pso.DepthStencilState.DepthFunc=D3D12_COMPARISON_FUNC_LESS_EQUAL;
    Check(ds3native::Pipelines().Graphics(gpu_.device.Get(),pso,offscreen_background_pso_.ReleaseAndGetAddressOf()),"native offscreen background pipeline");
}
void SceneRenderer::Targets(uint32_t width,uint32_t height) {
    if(view_->target_generation==UINT64_MAX)throw std::overflow_error("native target generation exhausted");
    auto device=gpu_.device.Get();const auto heap=Heap(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_DESCRIPTOR_HEAP_DESC desc{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,6u,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
    Check(CountedHeap(device)->CreateDescriptorHeap(&desc,IID_PPV_ARGS(&view_->rtvs)),"scene MRT views");rtv_stride_=device->GetDescriptorHandleIncrementSize(desc.Type);
    for(unsigned i=0;i<6;++i) {
        const auto image=Image(width,height,1,color_formats[i],D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        D3D12_CLEAR_VALUE clear{};clear.Format=color_formats[i];
        Check(CountedResource(device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&image,D3D12_RESOURCE_STATE_RENDER_TARGET,&clear,IID_PPV_ARGS(&view_->colors[i])),"private Gbuffer");
        device->CreateRenderTargetView(view_->colors[i].Get(),nullptr,CPU(view_->rtvs.Get(),rtv_stride_,i));
    }
    // Slot 1: read-only depth (environment volumes, 1137 SSS stencil test).
    desc={D3D12_DESCRIPTOR_HEAP_TYPE_DSV,2u,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};Check(CountedHeap(device)->CreateDescriptorHeap(&desc,IID_PPV_ARGS(&view_->dsv)),"scene DSV");
    DepthTarget(width,height,0);
    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=color_formats[2];srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;
    device->CreateShaderResourceView(view_->colors[2].Get(),&srv,CPU(view_->bindings.Get(),binding_stride_,composition_binding));
    ++view_->target_generation;   // I-0120 E5: no readback buffer a context any more (made at every generation, never read)
}
// Phase E E0 (D3D12-PHASE-E-FRAME-GRAPH-SHEET 7): the allocation bytes (GetResourceAllocationInfo) of the committed render
// and depth targets, for the main view (its context, the velocity and UI
// depth targets) and for the menu-view contexts, recomputed when one of their generations changes; the readback buffers
// are left out, and so is the scene copy (assigned by the owner thread, review: the frame job holds its own reference).
// Recording thread; the audit row reads the atomics.
void SceneRenderer::NoteTargetBytes() {
    std::vector<uint64_t> key{main_view_.target_generation,main_view_.light_generation,velocity_generation_,uint64_t(bool(primitive_depth_))};
    for(const auto& [extent,set]:offscreen_sets_){key.push_back(extent.first);key.push_back(extent.second);key.push_back(set.target_generation);key.push_back(set.light_generation);}
    if(key==target_bytes_key_)return;
    target_bytes_key_=std::move(key);auto* device=gpu_.device.Get();uint64_t resources{},alignment{};std::vector<ID3D12Pageable*> targets;
    const auto bytes=[&](ID3D12Resource* resource)->uint64_t{if(!resource)return 0;++resources;targets.push_back(resource);const auto desc=resource->GetDesc();
        const auto info=device->GetResourceAllocationInfo(0,1,&desc);alignment=std::max<uint64_t>(alignment,info.Alignment);return info.SizeInBytes;};
    const auto context=[&](const ViewContext& c){uint64_t sum{};for(const auto& color:c.colors)sum+=bytes(color.Get());
        sum+=bytes(c.depth.Get())+bytes(c.env_target.Get())+bytes(c.sss_temp.Get());for(const auto& light:c.light_targets)sum+=bytes(light.Get());return sum;};
    uint64_t main=context(main_view_)+
        bytes(velocity_target_.Get())+bytes(motion_vectors_.Get())+bytes(velocity_half_.Get())+bytes(primitive_depth_.Get());
    uint64_t views{};for(const auto& [extent,set]:offscreen_sets_)views+=context(set);
    gpu_audit.target_bytes_main.store(main,std::memory_order_relaxed);gpu_audit.target_bytes_views.store(views,std::memory_order_relaxed);
    gpu_audit.target_resources.store(resources,std::memory_order_relaxed);gpu_audit.target_extents.store(1+offscreen_sets_.size(),std::memory_order_relaxed);
    gpu_audit.target_alignment.store(alignment,std::memory_order_relaxed);
    // I-0084 (lead D1): over the budget the video memory manager demotes the lowest residency priority first; the frame's
    // targets, written and read every frame, are raised so it takes streamed textures first (ER has the wrappers, I-0080 ER-STUDY
    // 4, with no caller found). Reapplied with each new generation of targets.
    Microsoft::WRL::ComPtr<ID3D12Device1> residency;
    gpu_audit.target_priority_raised.store(0,std::memory_order_relaxed);   // final review 4: no stale count on a failure
    if(!targets.empty() && SUCCEEDED(gpu_.device.As(&residency))){const std::vector<D3D12_RESIDENCY_PRIORITY> high(targets.size(),D3D12_RESIDENCY_PRIORITY_HIGH);
        if(SUCCEEDED(residency->SetResidencyPriority(static_cast<UINT>(targets.size()),targets.data(),high.data())))
            gpu_audit.target_priority_raised.store(targets.size(),std::memory_order_relaxed);}
}
void SceneRenderer::DepthTarget(uint32_t width,uint32_t height,uint8_t initial_stencil) {
    // Typeless: the lighting and the effects read it as a view (I-0120 E6: the D32S8 form, before any model target, was never made).
    const auto image=Image(width,height,1,DXGI_FORMAT_R32G8X24_TYPELESS,D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
    D3D12_CLEAR_VALUE clear{};clear.Format=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;clear.DepthStencil.Depth=1;clear.DepthStencil.Stencil=initial_stencil;
    Microsoft::WRL::ComPtr<ID3D12Resource> replacement;
    Check(CountedResource(gpu_.device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&image,D3D12_RESOURCE_STATE_DEPTH_WRITE,&clear,IID_PPV_ARGS(&replacement)),"private scene depth");
    D3D12_DEPTH_STENCIL_VIEW_DESC view{};view.Format=clear.Format;view.ViewDimension=D3D12_DSV_DIMENSION_TEXTURE2D;
    gpu_.device->CreateDepthStencilView(replacement.Get(),&view,view_->dsv->GetCPUDescriptorHandleForHeapStart());
    view.Flags=D3D12_DSV_FLAG_READ_ONLY_DEPTH;
    gpu_.device->CreateDepthStencilView(replacement.Get(),&view,CPU(view_->dsv.Get(),gpu_.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV),1));
    view_->depth=std::move(replacement);view_->clear_stencil=initial_stencil;
}
uint32_t SceneRenderer::Diagnostics() {
    uint32_t errors{};
    if(gpu_.diagnostics) {
        const auto count=gpu_.diagnostics->GetNumStoredMessagesAllowedByRetrievalFilter();
        for(UINT64 i=0;i<count;++i) {SIZE_T length{};Check(gpu_.diagnostics->GetMessage(i,nullptr,&length),"scene diagnostic extent");
            std::vector<uint8_t> bytes(length);auto message=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());Check(gpu_.diagnostics->GetMessage(i,message,&length),"scene diagnostic message");
            // 1126: a forward programme whose PS also writes SV_Target1 while the forward pass binds one target
            // (CreateGraphicsPipelineState warning RENDERTARGETVIEW_NOT_SET): the write is discarded, as under D3D11.
            if(message->ID==D3D12_MESSAGE_ID_CREATEGRAPHICSPIPELINESTATE_RENDERTARGETVIEW_NOT_SET && message->Severity==D3D12_MESSAGE_SEVERITY_WARNING)continue;
            if(AgilityCapabilityNotice(message->Severity,message->pDescription))continue;   // I-0132 R11f (backend.h: why it is not a fault)
            if(FrameGenerationFenceNotice(message->Severity,message->pDescription))continue;   // I-0132 R12f (backend.h)
            if(message->Severity<=D3D12_MESSAGE_SEVERITY_WARNING){
                if(!errors){last_diagnostic_.assign(message->pDescription?message->pDescription:"",std::min<SIZE_T>(message->DescriptionByteLength,300));
                    for(auto& ch:last_diagnostic_)if(ch=='"' || ch=='\\' || ch<32)ch='\'';}
                ++errors;}}
        gpu_.diagnostics->ClearStoredMessages();
    }
    return errors;
}
// R14 cR4 (with cD9): pace (interval 1, the game's cadence), then Present; the Present call alone is present_call_ms, and
// the interval between consecutive presents is kept for native_present_times.
// I-0131 R10b: DLSS-G off (before a chain change, §12) and latched off after a refusal, its row once.
// The final review's M1: the Off takes effect at the next Present (§6.0), so when the last Present had it On one Present of the chain's
// current buffer (the worker quiesced, its last frame) carries it before the chain changes (§12: Present back on this thread), then the
// queue drained again.
void SceneRenderer::FrameGenerationOff(){if(!frame_generation_)return;std::string reason;(void)streamline::SetFrameGeneration(false,reason);
    if(!fg_presented_on_ || !presenter_.swapchain)return;fg_presented_on_=false;
    try{(void)presenter_.swapchain->Present(0,0);(void)gpu_.Drain(WaitResize);}catch(...){}}
void SceneRenderer::FailFrameGeneration(uint64_t serial,const std::string& reason){
    if(fg_failed_.exchange(true))return;streamline::FgDisable();std::string ignored;(void)streamline::SetFrameGeneration(false,ignored);
    if(!upscaler_sink_)return;std::string why=reason.substr(0,200);for(auto& c:why)if(c=='"' || c=='\\' || c<32)c=' ';char row[320]{};
    sprintf_s(row,"{\"event\":\"native_frame_generation_latched_off\",\"serial\":%llu,\"reason\":\"%s\"}\n",static_cast<unsigned long long>(serial),why.c_str());upscaler_sink_(row);}
HRESULT SceneRenderer::PresentPaced(){
    cadence::Mark(cadence::PaceStart);   // I-0097 tranche 5 (cadence-trace.flag)
    // Review: the refresh rate followed (a window moved to another display, a changed desktop mode) every 120th present.
    if(!(++presents_%120) && present_window_){const auto refresh=RefreshRateOf(present_window_);if(refresh!=refresh_hz_){refresh_hz_=refresh;const double period=RefreshPeriodOf(present_window_);pacer_.SetRefreshPeriod(period>0?period:refresh?1.0/refresh:0);pacer_.Reset();}}
    // I-0111 tranche 2: where the engine's frame-start wait is engaged it is the clock: Present(1) at once, no pacer.
    const bool sync=pacer_.Sync();   // I-0132 R12f: the V-sync live, read once for this Present
    const UINT interval=display_interval_job_?display_interval_job_:present_unpaced_.load(std::memory_order_relaxed)?(sync?1u:0u):present_pacing_?pacer_.Pace(1):0;
    // I-0088 P0: an interval-0 present may tear on a chain made for it, in a window only (an exclusive full screen refuses the flag).
    // I-0132 R12f: only without V-sync (the chain allows tearing whenever paced, so the V-sync can change live; a synced Present(0), on
    // a screen slower than the cadence, never tears), or with original-unlock.flag's measure.
    UINT flags{};if(!interval && presenter_.tearing && (!sync || present_tear_synced_)){BOOL fullscreen{};if(SUCCEEDED(presenter_.swapchain->GetFullscreenState(&fullscreen,nullptr)) && !fullscreen)flags=DXGI_PRESENT_ALLOW_TEARING;}
    // I-0131 R10b (ProgrammingGuideDLSS_G §6.0, §6.4): DLSS-G's mode for this Present, on its thread: on for a world frame whose
    // depth, motion and HUD-less colour were tagged, off otherwise (start-up frames, menus, loads, a frame without DLSS).
    // The step review's M2: only with Reflex on (§8) and a presentation at least DLSS-G's minimum size (§14.3: off below it, not
    // latched; the final review's L1); its status read after its first On present and every 120th after it: a status other than OK
    // twice in a row latches it off.
    if(frame_generation_){bool on=fg_wanted_.load(std::memory_order_relaxed) && !fg_menu_hold.load(std::memory_order_relaxed) && job_serial_ && fg_hudless_serial_==job_serial_ && !fg_failed_.load(std::memory_order_relaxed) && streamline::ReflexOn() &&
            std::min(presenter_.width,presenter_.height)>=fg_min_size_ &&
            !presenter_.stand_in;   // I-0137 (E3): the chain may be of another size than the frame's tagged resources, and such a frame does not reach the screen
        std::string reason;
        const auto on_seen=fg_on_presents_.load(std::memory_order_relaxed);
        if(on && on_seen%120==1){streamline::FgState state{};
            if(streamline::FrameGenerationState(state)){fg_min_size_=state.min_size;
                if(state.status!=0 && ++fg_bad_status_>=2){char why[96]{};sprintf_s(why,"status %u",state.status);FailFrameGeneration(job_serial_,why);on=false;}
                else if(!state.status)fg_bad_status_=0;
                if(std::min(presenter_.width,presenter_.height)<fg_min_size_)on=false;}}
        if(!streamline::SetFrameGeneration(on,reason))FailFrameGeneration(job_serial_,reason);
        fg_presented_on_=on && !fg_failed_.load(std::memory_order_relaxed);
        const auto n=fg_presents_.fetch_add(1,std::memory_order_relaxed)+1;const auto on_n=on?fg_on_presents_.fetch_add(1,std::memory_order_relaxed)+1:on_seen;
        if(upscaler_sink_ && (n==1 || (on && on_n==1) || n%600==0)){streamline::FgState state{};const bool read=streamline::FrameGenerationState(state);char row[320]{};
            sprintf_s(row,"{\"event\":\"native_frame_generation\",\"presents\":%llu,\"on_presents\":%llu,\"on\":%s,\"state\":%s,\"presented\":%u,\"status\":%u,\"generate\":%u,\"generate_max\":%u,\"min_size\":%u,\"vsync\":%s,\"fence_notices\":%llu}\n",
                static_cast<unsigned long long>(n),static_cast<unsigned long long>(on_n),on?"true":"false",read?"true":"false",state.presented,state.status,state.generate,state.generate_max,state.min_size,state.vsync?"true":"false",static_cast<unsigned long long>(frame_generation_fence_notices.load(std::memory_order_relaxed)));upscaler_sink_(row);}}
    // I-0111 tranche 2: PCL's submit-end and present markers of the world frame (none for start-up frames).
    if(job_serial_){streamline::Marker(streamline::PclMarker::RenderSubmitEnd,job_serial_);streamline::Marker(streamline::PclMarker::PresentStart,job_serial_);}
    cadence::Mark(cadence::PresentStart);
    const auto before=Ticks();const HRESULT status=presenter_.swapchain->Present(interval,flags);present_call_ms_.fetch_add(MsSince(before));cadence::Mark(cadence::PresentEnd);
    if(job_serial_)streamline::Marker(streamline::PclMarker::PresentEnd,job_serial_);
    if(fg_failed_.load(std::memory_order_relaxed))streamline::ReleaseFrameGeneration();   // I-0131 R10b (the step review's L5): its Off went through
    LARGE_INTEGER now{},frequency{};QueryPerformanceCounter(&now);QueryPerformanceFrequency(&frequency);
    // What the display did (the statistics of the last frame it showed; I-0097: DisplayRefreshes).
    DXGI_FRAME_STATISTICS statistics{};const bool shown=SUCCEEDED(presenter_.swapchain->GetFrameStatistics(&statistics));
    {const std::lock_guard hold(present_times_lock_);
        if(last_present_ && present_intervals_.size()<4096)present_intervals_.push_back(float(1000.0*double(now.QuadPart-last_present_)/double(frequency.QuadPart)));
        last_present_=now.QuadPart;display_refreshes_.Add(shown,statistics.PresentCount,statistics.PresentRefreshCount);}
    return status;
}
// I-0101 (STUDY §3.2-3.3): DS3 FUN_14192CF00 and ER FUN_141E9F630 drop every Present failure; ER completes the frame only on
// success (its defect: the frame's fence never signalled otherwise). Here a lost device keeps the refusal (its DRED rows); any
// other failure completes the frame normally, its fence signalled and its slot released by the caller, and marks the
// presentation lost once per episode with the discriminator of STUDY §3.5 (H1 a full-screen transition not followed by
// ResizeBuffers, H2 a toggle racing the Present, H3 a window no longer in front). True when the failure was admitted.
bool SceneRenderer::AdmitPresent(HRESULT status,const char* what){
    if(SUCCEEDED(status)){presents_ok_.fetch_add(1,std::memory_order_relaxed);return false;}
    if(IsDeviceLoss(status))
        Check(status,what);
    present_failures_.fetch_add(1,std::memory_order_relaxed);
    // Review M1 (I-0101), I-0111 tranche 2: the count this frame's wait took (the worker's, or the engine's display clock's) is not
    // given back; whichever side waits next skips one.
    if(std::exchange(latency_taken_,false) || display_interval_job_)latency_debt_.fetch_add(1,std::memory_order_relaxed);
    if(presentation_lost_.load(std::memory_order_acquire))return true;
    PresentationLoss loss{};loss.status=status;
    BOOL fullscreen{};loss.fullscreen=SUCCEEDED(presenter_.swapchain->GetFullscreenState(&fullscreen,nullptr)) && fullscreen;
    DXGI_SWAP_CHAIN_DESC desc{};if(SUCCEEDED(presenter_.swapchain->GetDesc(&desc))){loss.windowed=desc.Windowed;loss.width=desc.BufferDesc.Width;loss.height=desc.BufferDesc.Height;}
    if(present_window_){loss.iconic=IsIconic(present_window_);loss.foreground=GetForegroundWindow()==present_window_;
        RECT client{};if(GetClientRect(present_window_,&client)){loss.client_width=uint32_t(client.right);loss.client_height=uint32_t(client.bottom);}}
    {const std::lock_guard hold(present_failure_lock_);present_failure_=loss;}
    presentation_lost_.store(true,std::memory_order_release);
    return true;
}
uint32_t SceneRenderer::DecideDisplayClock(float limiter_period){
    RequireThread(false,"native display clock off the engine thread");
    display_interval_engine_=0;
    if(!display_clock_requested_ || !open_ || faulted_){display_was_engaged_=false;return 0;}
    if(!(display_checks_++%120) && present_window_){const double period=RefreshPeriodOf(present_window_);if(period>0)display_period_=period;
        else if(const auto hz=RefreshRateOf(present_window_))display_period_=1.0/hz;else display_period_=0;}
    HWND window=presenter_.presented;
    // Step review L2: DS3's limiter at 1/60 s (within 1 %), not its 1/30 s mode; or (I-0114) faster with the unlocked rate.
    const bool sixty=std::isfinite(limiter_period) && limiter_period>0 && std::abs(double(limiter_period)*60.0-1.0)<0.01;
    const bool unlocked=std::isfinite(limiter_period) && limiter_period>0 && double(limiter_period)*60.0<0.99;
    if(!presenter_.latency || !pacer_.Sync() || !(sixty || unlocked) || !window || !IsWindowVisible(window) || IsIconic(window)){display_was_engaged_=false;return 0;}
    // I-0114 (step review M1): unlocked, the producer runs at DS3's requested rate: one refresh a frame whenever that period is not
    // longer than the display's refresh (the display then paces it, at its own rate); otherwise DS3's limiter is the cap and the
    // clock stays off. Locked at 60: the refreshes that fit 1/60 s.
    display_interval_engine_=unlocked?(display_period_>0 && double(limiter_period)<=display_period_*1.01?1u:0u):DisplayClock::Fits(display_period_);
    if(!display_interval_engine_)display_was_engaged_=false;
    return display_interval_engine_;
}
DisplayClock::Outcome SceneRenderer::WaitDisplayReady(int64_t deadline){
    RequireThread(false,"native display clock off the engine thread");
    ++display_clock_counts_.waits;
    if(!display_interval_engine_ || !open_ || faulted_ || !presenter_.latency)return {};
    cadence::Mark(cadence::DisplayWaitStart);
    LARGE_INTEGER frequency{};QueryPerformanceFrequency(&frequency);
    if(!display_clock_){HANDLE object=presenter_.latency;   // the chain's object: replaced only with the chain (Start, on this thread)
        display_clock_.emplace(DisplayClock::Source{[]{LARGE_INTEGER t{};QueryPerformanceCounter(&t);return int64_t(t.QuadPart);},
            [object](uint32_t ms){const auto r=WaitForSingleObjectEx(object,ms,FALSE);   // not alertable: each wait takes one count
                return r==WAIT_OBJECT_0?DisplayClock::Release::Signalled:r==WAIT_TIMEOUT?DisplayClock::Release::Timeout:DisplayClock::Release::Failed;}});}
    // Second review M3: the first frame after the clock was off only probes (the present it would pair with may never be taken).
    const bool probe=!std::exchange(display_was_engaged_,true);
    const auto o=display_clock_->Wait(deadline,TakeLatencyDebt(),probe);
    cadence::Mark(cadence::DisplayWaitEnd);
    auto& c=display_clock_counts_;++c.engaged;c.released+=o.released;c.stored+=o.stored;c.timeouts+=o.timeout;c.skipped+=o.skipped;
    c.probed+=o.probed;c.owed_taken+=o.owed_taken;
    c.waited_ms+=1000.0*double(o.end-o.start)/double(frequency.QuadPart);
    if(o.released)c.lead_ms+=1000.0*double(o.lead)/double(frequency.QuadPart);   // second review L2: over the frames started on a take
    return o;
}
std::vector<float> SceneRenderer::TakePresentIntervals(){const std::lock_guard hold(present_times_lock_);std::vector<float> taken;taken.swap(present_intervals_);return taken;}
DisplayRefreshes::Counts SceneRenderer::TakeDisplayRefreshes(){auto spare=DisplayRefreshes::SpareHolds(display_refreshes_.KeepsHolds());const std::lock_guard hold(present_times_lock_);   // I-0098 step review: reserved outside the lock
    return display_refreshes_.Take(std::move(spare));}
void SceneRenderer::Start(HWND window,bool warp,bool debug,uint32_t width,uint32_t height) {
    if(open_ || gpu_.device || faulted_ || !window || !width || !height || width>16384 || height>16384) throw std::invalid_argument("native startup admission");
    try {
        {const std::lock_guard hold(channel_lock_);completed_.clear();}   // cD6: a previous incarnation's results
        // I-0111 (R1): Streamline before the chain (manual hooking), given the device once it exists; any refusal leaves DLSS off.
        upscaler_state_=upscaler_folder_.empty()?"not requested":"";dlss_failed_=false;dlss_frames_=0;dlss_unresolved_=0;dlss_last_serial_=0;
        if(!upscaler_folder_.empty()){std::wstring folder=upscaler_folder_;while(!folder.empty() && (folder.back()==L'\\' || folder.back()==L'/'))folder.pop_back();
            std::string reason;if(!streamline::Initialise(folder,upscaler_sink_,reason,frame_generation_requested_>=2 || frame_generation_offer_))upscaler_state_="refused: "+reason;}
        gpu_.Open(warp,debug);SetRetirementQueue(&gpu_);
        if(streamline::Initialised()){std::string reason;
            if(!streamline::SetDevice(gpu_.device.Get(),gpu_.device->GetAdapterLuid(),reason)){upscaler_state_="refused: "+reason;streamline::Shutdown();}
            // I-0132 R12f: offered, not asked: unloaded before the chain is made (the native chain, our frame-latency object).
            else if(frame_generation_requested_<2 && streamline::FgSupported() && !streamline::SetFrameGenerationLoaded(false,reason) && upscaler_sink_){
                for(auto& c:reason)if(c=='"' || c=='\\' || c<32)c=' ';char row[256]{};
                sprintf_s(row,"{\"event\":\"native_frame_generation_unload_refused\",\"reason\":\"%.200s\"}\n",reason.c_str());upscaler_sink_(row);}}
        frames_.Open(gpu_.device.Get(),frame_slots_);
        presenter_.count=present_buffers_;presenter_.waitable=present_waitable_;presenter_.max_latency=frame_slots_;   // cD8
        // I-0131 R10b (ProgrammingGuideDLSS_G §12.1): with DLSS-G loaded the chain is made without the frame-latency object, which
        // Streamline's pacer then owns; the frame starts at Reflex's sleep (forced on with it), the waits on the object skipped.
        frame_generation_=streamline::FgLoaded();if(frame_generation_){presenter_.waitable=false;streamline::SetFrameGenerationMultiplier((std::max)(frame_generation_requested_,2u));}
        // I-0111 tranche 2 (second review M2): latency 1 with the display clock only on a screen it applies to (the worker waits elsewhere).
        if(display_clock_requested_ && DisplayClock::Fits(RefreshPeriodOf(window)))presenter_.max_latency=1;
        display_clock_.reset();display_was_engaged_=false;
        presenter_.tearing=present_tearing_;   // I-0088 P0
        ProbeHdrOutput(window,width,height);   // I-0138 tranche 5
        presenter_.Open(gpu_,window,width,height);
        frame_generation_=frame_generation_ && streamline::FgLoaded();   // the step review's L1: a refused proxy chain ended Streamline
        fg_wanted_=frame_generation_ && frame_generation_requested_>=2;frame_generation_loaded_now=streamline::FgLoaded();   // I-0132 R12f (its step review's M1): a refused unload stays Off
        if(!presenter_.upgrade_failure.empty())upscaler_state_="refused: "+presenter_.upgrade_failure;
        else if(presenter_.upgraded)upscaler_state_="ready";
        refresh_hz_=RefreshRateOf(window);{const double period=RefreshPeriodOf(window);const uint32_t rate=refresh_hz_;pacer_.SetRefreshPeriod(period>0?period:rate?1.0/rate:0);}present_window_=window;presents_=0;pacer_.Reset();{const std::lock_guard hold(present_times_lock_);present_intervals_.clear();display_refreshes_.Reset();last_present_=0;}   // cR4; final review 3: the nominal rate when the display path gives no period
        exclusive_=false;presentation_lost_=false;present_failures_=0;follow_ok_seen_=presents_ok_.load();failing_follows_=0;latency_debt_=0;latency_taken_=false;   // I-0101: a new chain starts windowed
        open_=true;
    } catch(...) {faulted_=true;throw;}
}
// I-0138 tranche 5: what the chain is asked at its next making or resize, from the request and the window's screen as it is now.
void SceneRenderer::ProbeHdrOutput(HWND window,uint32_t width,uint32_t height) {
    hdr_refusal_.clear();
    if(hdr_request_.mode==hdr::Mode::Off){presenter_.hdr_wanted=presenter_.hdr_refused=false;hdr_factory_.Reset();return;}
    hdr_monitor_=MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST);
    {const auto probed=ProbeHdrDisplay(window,hdr_factory_.ReleaseAndGetAddressOf());
        // A screen not found (the window hidden or going away: seen at the end of i138-t5c-cycle) keeps the last answer.
        if(probed.found || !hdr_display_.found){
            if(probed.found!=hdr_display_.found || probed.hdr!=hdr_display_.hdr || probed.color_space!=hdr_display_.color_space)presenter_.hdr_refused=false;   // another answer: the chain may be asked again
            hdr_display_=probed;}}
    hdr_peak_=hdr::Peak(hdr_request_,hdr_display_);
    bool wanted=hdr_request_.mode==hdr::Mode::Force || hdr_display_.hdr;
    if(wanted && !display_blobs_){wanted=false;hdr_refusal_="no display source (the frame's 8-bit picture is the display stage's)";}
    if(wanted){   // the two stages and the picture a 10-bit chain's frames cannot do without, before the chain is asked (no frame in flight here)
        std::string reason;
        if(!display_.IsOpen() && !display_failed_){
            if(display_.Open(gpu_.device.Get(),*display_blobs_,width,height,reason))display_open_=true;else{display_failure_=reason;display_failed_=true;}}
        if(display_failed_){wanted=false;hdr_refusal_="display stage: "+display_failure_;}
        else if(!output_.IsOpen() && !output_.Open(gpu_.device.Get(),reason)){wanted=false;hdr_refusal_="output stage: "+reason;}
        else if(!output_.Targets(width,height,reason,false)){wanted=false;hdr_refusal_="output picture: "+reason;}}   // the float picture every frame of the chain ends in
    presenter_.hdr_wanted=wanted;if(!wanted)presenter_.hdr_refused=false;   // a later request asks the chain again
}
bool SceneRenderer::HdrOutputStale() const noexcept {
    if(!open_ || hdr_request_.mode==hdr::Mode::Off || !presenter_.presented || !IsWindow(presenter_.presented))return false;
    return MonitorFromWindow(presenter_.presented,MONITOR_DEFAULTTONEAREST)!=hdr_monitor_ || (hdr_factory_ && !hdr_factory_->IsCurrent());
}
bool SceneRenderer::SetHdrOutputLive(const hdr::Request& request) {
    if(!open_ || faulted_)throw std::invalid_argument("scene HDR output");
    if(frames_.Recording())throw std::logic_error("native HDR output inside an open frame list");
    hdr_request_=request;ProbeHdrOutput(presenter_.presented,presenter_.width,presenter_.height);
    if((presenter_.hdr_wanted && !presenter_.hdr_refused)==presenter_.Hdr10Format())return false;   // the chain already is what it is asked
    hdr_probe_fresh_=true;   // this probe is the resize's
    Resize(presenter_.width,presenter_.height);return true;
}
FrameReceipt SceneRenderer::RenderStartup(uint64_t serial) {
    return RenderStartupFrame(serial,nullptr,false);
}
FrameReceipt SceneRenderer::RenderStartupPrimitives(uint64_t serial,uint64_t generation,bool initialize_stencil) {
    const auto* packet=primitives_?primitives_->Packet():nullptr;
    if(!packet || !generation || packet->generation!=generation)throw std::invalid_argument("native startup primitive generation");
    return RenderStartupFrame(serial,primitives_.get(),initialize_stencil,primitives_);
}
const std::array<DXGI_FORMAT,6>& SceneRenderer::ModelTargetFormats() noexcept {return color_formats;}
void SceneRenderer::NativeModelTargets() {
    if(view_->model_targets)return;
    // I-0084: once per device (Close resets it), not again for each view context and resize, which recompiled its HLSL and
    // replaced a pipeline the frames in flight could still hold.
    if(!composition_pso_)MaterialColorCompositionPipeline();
    // +1: the lit HDR source of the composition when native lighting runs.
    // +2: DLSS's output, the composition's source on a DLSS frame (I-0111).
    D3D12_DESCRIPTOR_HEAP_DESC heap{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,composition_binding+3,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
    Check(CountedHeap(gpu_.device)->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&view_->bindings)),"native model composition view heap");
    binding_stride_=gpu_.device->GetDescriptorHandleIncrementSize(heap.Type);
    Targets(ViewWidth(),ViewHeight());view_->model_targets=true;
}
FrameReceipt SceneRenderer::RenderStartupFrame(uint64_t serial,const PrimitivePass* ui,bool initialize_stencil,std::shared_ptr<const void> ui_owner) {
    if(!open_ || faulted_ || !serial || serial<=startup_serial_) throw std::invalid_argument("native startup request admission");
    const auto* primitive=ui?ui->Packet():nullptr;
    if(ui && !primitive)throw std::invalid_argument("native startup primitive build");
    if(primitive && (primitive->serial!=serial || primitive->width!=presenter_.width || primitive->height!=presenter_.height))
        throw std::invalid_argument("native startup primitive frame/target");
    if(primitive)try{PrimitiveDepthTarget(presenter_.width,presenter_.height);}catch(...){faulted_=true;throw;}   // cR2: the frame owns the UI depth (X1, D-12)
    if(primitive && !initialize_stencil && !(primitive_stencil_initialized_ && primitive_stencil_generation_==primitive->generation))
        throw std::invalid_argument("native startup primitive stencil");
    try {
        // cD3: continues the list a movie or menu view of this frame opened (title, Load Game, character creation).
        auto& slot=FrameListSlot(WaitStartup);const auto list=slot.list.Get();
        TickMenuRtMaterials(list);   // I-0140 t31 (the step review of the fixes, L2): the menu tables swept on the frames with no world
        SweepMenuAa(nullptr);   // I-0140 tranche 40 (its sixth step review's M1): the menu views' AA too
        if(primitive){slot.kept.push_back(HoldResource(primitive_depth_.Get()));if(ui_owner)slot.kept.push_back(ui_owner);}   // D-12; cR2: the build
        const auto index=presenter_.swapchain->GetCurrentBackBufferIndex();const auto buffer=presenter_.buffers[index].Get();
        // W3: logos, title, menus and loading go through the gamma pass too (the original's every frame does); without the
        // stage, straight into the back buffer as before.
        const bool display=DisplayReady(),hdr_out=HdrOutputReady(display);
        if(!display)Transition(list,buffer,D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_RENDER_TARGET);
        // I-0138 tranche 5: on an HDR10 chain the frame is drawn on the output stage's float picture (the UI's and the overlay's
        // pipelines are that format's there), from black as the 8-bit picture.
        if(hdr_out)slot.kept.push_back(HoldResource(output_.Scene()));
        constexpr float clear[4]={0,0,0,1};const auto target=hdr_out?output_.SceneTarget():display?display_.Target():CPU(presenter_.rtv.Get(),presenter_.stride,index);
        list->ClearRenderTargetView(target,clear,0,nullptr);
        if(primitive) {
            const auto stencil=primitive_dsv_->GetCPUDescriptorHandleForHeapStart();
            if(initialize_stencil)list->ClearDepthStencilView(stencil,D3D12_CLEAR_FLAG_DEPTH|D3D12_CLEAR_FLAG_STENCIL,1,0,0,nullptr);
            if(ui->FloatTarget()==hdr_out)ui->Encode(list,target,stencil,true);
            else ui_format_skips_.fetch_add(1,std::memory_order_relaxed);   // a build made for the other chain format: not drawn this frame
        }
        // I-0132 R12e: the tint layer (the logo's tint and burn, a multiply), then the screen layer (the title's embers and smoke), over
        // the game's UI and before our overlay.
        if(!overlay_failed_)RecordOverlay(list,target,slot,tint_layer_.Take(presenter_.width,presenter_.height),tint_layer_,hdr_out);
        if(!overlay_failed_)RecordOverlay(list,target,slot,screen_layer_.Take(presenter_.width,presenter_.height),screen_layer_,hdr_out);
        RecordOverlay(list,target,slot,OverlaySnapshot(),overlay_,hdr_out);   // I-0132 R12a: our pages above the game's UI, before the gamma pass (no frame graph here)
        if(display){if(!hdr_out)if(const auto staging=display_.RecordUploadIfDirty(list))slot.kept.push_back(HoldResource(staging.Get()));   // the curve is the brightness pass's alone
            auto* const picture=hdr_out?output_.Scene():display_.Intermediate();
            Transition(list,picture,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            Transition(list,buffer,D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_RENDER_TARGET);
            const D3D12_VIEWPORT viewport{0,0,static_cast<float>(presenter_.width),static_cast<float>(presenter_.height),0,1};
            const D3D12_RECT scissor{0,0,static_cast<LONG>(presenter_.width),static_cast<LONG>(presenter_.height)};
            if(hdr_out)RecordHdrScene(list,serial,CPU(presenter_.rtv.Get(),presenter_.stride,index),viewport,scissor);   // I-0138 tranche 5
            else display_.RecordPass(list,CPU(presenter_.rtv.Get(),presenter_.stride,index),viewport,scissor);
            Transition(list,picture,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);}
        Transition(list,buffer,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PRESENT);
        Check(list->Close(),"startup recorder close");FrameRing::RequireHeld(slot);FrameReceipt result{};result.serial=serial;
        if(primitive) {result.content_incarnation=primitive->content_incarnation;result.primitive_draw_count=static_cast<uint32_t>(primitive->draws.size());}
        // The arena ranges of the movie and the menu views recorded into this list retire with it.
        result.fence=gpu_.Execute(list);primitive_uploads_pending_=false;frame_uploads_.Retire(gpu_,result.fence);if(frames_.Slots()==1)gpu_.Complete(result.fence,WaitStartup);
        if(primitive){primitive_stencil_initialized_=true;primitive_stencil_generation_=primitive->generation;}
        result.present_status=PresentPaced();result.present_lost=AdmitPresent(result.present_status,"native startup present");
        result.presentation_fence=frames_.Slots()==1?gpu_.Drain(WaitStartup):gpu_.Signal();   // cD6
        // cD4: no witness readback on startup frames; diagnostics at the release.
        slot.completions.push_back([this,serial,ticket=result.presentation_fence]{CompleteFrame(serial,ticket,SlotReadbacks{},false,{},false,{});});
        frames_.End(gpu_,slot,result.presentation_fence,WaitStartup);gpu_.CollectCompleted();
        TakeReleased(result,"native startup diagnostic error: ");
        startup_serial_=serial;PublishSerials();return result;
    } catch(...) {faulted_=true;throw;}
}
bool SceneRenderer::ReadStartupView(uint32_t& width,uint32_t& height) const noexcept {
    if(!open_ || faulted_ || !startup_serial_ || !gpu_.device || !gpu_.queue ||
        !presenter_.swapchain || !presenter_.rtv || FAILED(gpu_.device->GetDeviceRemovedReason()))return false;
    DXGI_SWAP_CHAIN_DESC1 swap{};
    if(FAILED(presenter_.swapchain->GetDesc1(&swap)) || swap.Width!=presenter_.width ||
        swap.Height!=presenter_.height || swap.BufferCount!=presenter_.buffers.size())return false;
    for(const auto& buffer:presenter_.buffers) {
        if(!buffer)return false;const auto desc=buffer->GetDesc();
        if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D || desc.Width!=swap.Width ||
            desc.Height!=swap.Height || desc.Format!=swap.Format)return false;
    }
    width=swap.Width;height=swap.Height;return true;
}
// cD3 (P1, D-12): the UI depth-stencil target is persistent per extent (a frame that draws the UI clears it, and the slot
// that records it holds it); a target of another extent is retired by ticket and replaced.
void SceneRenderer::PrimitiveDepthTarget(uint32_t width,uint32_t height) {
    if(primitive_depth_ && primitive_dsv_){const auto desc=primitive_depth_->GetDesc();if(desc.Width==width && desc.Height==height)return;}
    if(primitive_depth_ || primitive_dsv_){
        struct Held {ComPtr<ID3D12Resource> depth;ComPtr<ID3D12DescriptorHeap> dsv;};
        RetireWhenSubmittedWorkCompletes(std::make_shared<Held>(Held{std::move(primitive_depth_),std::move(primitive_dsv_)}));}
    const auto image=Image(width,height,1,DXGI_FORMAT_R32G8X24_TYPELESS,D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    const auto heap=Heap(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_CLEAR_VALUE clear{};clear.Format=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;clear.DepthStencil.Depth=1;clear.DepthStencil.Stencil=0;
    Check(CountedResource(gpu_.device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&image,D3D12_RESOURCE_STATE_DEPTH_WRITE,&clear,IID_PPV_ARGS(&primitive_depth_)),"primitive target allocation");
    D3D12_DESCRIPTOR_HEAP_DESC desc{D3D12_DESCRIPTOR_HEAP_TYPE_DSV,1,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
    Check(CountedHeap(gpu_.device)->CreateDescriptorHeap(&desc,IID_PPV_ARGS(&primitive_dsv_)),"primitive target view heap");
    D3D12_DEPTH_STENCIL_VIEW_DESC view{};view.Format=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;view.ViewDimension=D3D12_DSV_DIMENSION_TEXTURE2D;
    gpu_.device->CreateDepthStencilView(primitive_depth_.Get(),&view,primitive_dsv_->GetCPUDescriptorHandleForHeapStart());primitive_stencil_initialized_=false;
}
void SceneRenderer::RetirePrimitives() {
    // Kept until the queue completes what was submitted until now: the frames that drew the build and its own uploads
    // (which ride the submission of the frame that admitted it). The depth target stays (cD3, P1).
    // cD3: a build whose uploads were not submitted by a frame (an admission without its frame, outside Clear()) has them
    // submitted first, so the ticket below covers them. R13 cR3: with the render worker this owner-thread submission can
    // race the worker's frame submission; both go through DeviceQueues' submit mutex (cR1), the ingress lists still precede
    // every later list, and the build retired here is held by the job and slot that draw it (L2).
    if(primitive_uploads_pending_){gpu_.SubmitIngress();primitive_uploads_pending_=false;}
    if(primitives_)RetireWhenSubmittedWorkCompletes(std::shared_ptr<void>(std::move(primitives_)));
}
std::shared_ptr<const PrimitivePass> SceneRenderer::AdmitPrimitiveWork(const frame::PrimitiveWorkPacket& input) {
    return AdmitPrimitiveWork(frame::PrimitiveWorkPacket(input));
}
std::shared_ptr<const PrimitivePass> SceneRenderer::AdmitPrimitiveWork(frame::PrimitiveWorkPacket&& input) {
    // I-0078: on the engine thread inline, or on the render worker for a job's own packet (ExecuteFrame); with a worker, never
    // on the engine thread (the worker owns the build state then).
    RequireThread(render_thread_.load(std::memory_order_acquire)!=0,"native UI admission off the thread that owns the UI build");
    const auto* previous=primitives_?primitives_->Packet():nullptr;
    if(!open_ || faulted_ || input.width!=presenter_.width || input.height!=presenter_.height ||
       input.generation<=primitive_generation_ || input.texture_generation<primitive_texture_generation_ || input.serial<=mirror_startup_serial_.load(std::memory_order_acquire) ||   // X7
       (previous && (input.generation<=previous->generation || input.texture_generation<previous->texture_generation || input.content_incarnation<previous->content_incarnation)))
        throw std::invalid_argument("primitive admission identity");
    frame::PrimitiveWorkSource source{input.content_incarnation,input.generation,input.texture_generation,input.serial,input.width,input.height,
        std::move(input.programs),std::move(input.buffers),std::move(input.textures),std::move(input.draws),std::move(input.targets)};
    LARGE_INTEGER t_frequency{},t0{},t1{},t2{},t3{},t4{},t5{};QueryPerformanceFrequency(&t_frequency);QueryPerformanceCounter(&t0);
    frame::PrimitiveWorkPacket validated;
    if(frame::ProducePrimitiveWork(std::move(source),validated).status!=frame::PrimitiveWorkStatus::Ready)throw std::invalid_argument("complete native primitive CPU admission");
    QueryPerformanceCounter(&t1);
    try {
        // B6 (R3): the previous build retires by ticket (no Drain); B3a: the uploads are recorded on the ingress context.
        // cD3: they are not submitted alone; the frame that draws the build submits them ahead of its list, in the same
        // call (an admission is always followed by its frame in Clear()). The UI depth target persists (P1).
        RetirePrimitives();QueryPerformanceCounter(&t2);
        auto recording=gpu_.RecordIngress();
        auto built=std::make_shared<PrimitivePass>();built->Build(gpu_.device.Get(),recording.List(),std::move(validated),presenter_.Hdr10Format());QueryPerformanceCounter(&t3);   // I-0138 tranche 5: `float_target`, an HDR10 chain's frames draw the UI on the float picture
        (void)recording.Commit();primitive_uploads_pending_=true;primitives_=built;   // cR2: the depth target and the stencil state belong to the frame (X1)
        QueryPerformanceCounter(&t4);
        // cD4 (R13 X6): no Diagnostics() here; the build's CPU-time messages are read at the release of its frame.
        QueryPerformanceCounter(&t5);
        const auto ms=[&](const LARGE_INTEGER& a,const LARGE_INTEGER& b){return static_cast<float>(1000.0*double(b.QuadPart-a.QuadPart)/double(t_frequency.QuadPart));};
        primitive_admission_ms={ms(t0,t1),ms(t1,t2),ms(t2,t3),ms(t3,t4),ms(t4,t5)};
        primitive_generation_=input.generation;primitive_texture_generation_=input.texture_generation;return built;
    } catch(...) {faulted_=true;throw;}
}
void SceneRenderer::WithdrawPrimitiveWork(uint64_t generation) {
    const auto* packet=primitives_?primitives_->Packet():nullptr;
    if(!open_ || faulted_ || !packet || packet->generation!=generation)throw std::invalid_argument("primitive withdrawal identity");
    try {RetirePrimitives();primitive_stencil_initialized_=false;}
    catch(...) {faulted_=true;throw;}
}
void SceneRenderer::Resize(uint32_t width,uint32_t height) {
    if(!open_ || faulted_ || !width || !height || width>16384 || height>16384) throw std::invalid_argument("scene resize");
    if(primitives_ && primitives_->Packet() && (width!=primitives_->Packet()->width || height!=primitives_->Packet()->height))throw std::invalid_argument("primitive extent requires withdrawal and readmission");
    // cD10: every slot waited for and released (their completions read the old extent's readbacks) before the drain.
    try {if(frames_.Recording())throw std::logic_error("native resize inside an open frame list");frames_.WaitIdle(gpu_,WaitResize);
        gpu_.Drain(WaitResize);FrameGenerationOff();   // I-0131 R10b (ProgrammingGuideDLSS_G §12): DLSS-G off before the chain changes
        // I-0111: another presentation extent returns the world to it, at DLAA (the engine thread sets the mode's render extent again); the
        // same extent (a full-screen toggle, a screen mode) keeps the render extent the frames in preparation were made for.
        if(width!=presenter_.width || height!=presenter_.height){render_width_=render_height_=0;dlss_mode_=streamline::DlssMode::Dlaa;}   // final review L1
        if(hdr_probe_fresh_)hdr_probe_fresh_=false;else ProbeHdrOutput(presenter_.presented,width,height);   // I-0138 tranche 5: the screen the window is on now
        presenter_.Resize(gpu_,width,height);
        if(display_.IsOpen())display_.Resize(width,height);if(glow_.IsOpen())glow_.Resize(width,height);if(post_tone_.IsOpen())post_tone_.Resize(width,height);if(mask_.IsOpen()){mask_.Resize(RenderWidth(),RenderHeight());mask_target_bytes_=mask_.TargetBytes();}if(main_view_.model_targets){for(auto& color:main_view_.colors) color.Reset();main_view_.depth.Reset();Targets(RenderWidth(),RenderHeight());}}
    catch(...) {faulted_=true;throw;}
}
void SceneRenderer::SetRenderExtent(uint32_t width,uint32_t height,streamline::DlssMode mode) {
    if(!open_ || faulted_)throw std::invalid_argument("scene render extent");
    if(width==presenter_.width && height==presenter_.height)width=height=0;
    if((width==0)!=(height==0) || width>presenter_.width || height>presenter_.height)throw std::invalid_argument("scene render extent");
    if(frames_.Recording())throw std::logic_error("native render extent inside an open frame list");
    dlss_mode_=mode;   // read by the next frame's evaluation (the worker quiesced)
    if(width==render_width_ && height==render_height_)return;
    try {frames_.WaitIdle(gpu_,WaitResize);
        gpu_.Drain(WaitResize);render_width_=width;render_height_=height;
        if(mask_.IsOpen()){mask_.Resize(RenderWidth(),RenderHeight());mask_target_bytes_=mask_.TargetBytes();}
        if(main_view_.model_targets){auto* const previous=view_;view_=&main_view_;
            for(auto& color:main_view_.colors) color.Reset();main_view_.depth.Reset();
            Targets(RenderWidth(),RenderHeight());view_=previous;}}
    catch(...) {faulted_=true;throw;}
}
void SceneRenderer::ResizePresentation(uint32_t width,uint32_t height) {
    if(!open_ || faulted_)throw std::invalid_argument("scene presentation resize");
    if(primitives_ && primitives_->Packet() && (primitives_->Packet()->width!=width || primitives_->Packet()->height!=height)){
        try{RetirePrimitives();primitive_stencil_initialized_=false;}catch(...){faulted_=true;throw;}}
    Resize(width,height);
}
bool SceneRenderer::PresentationFullscreen() const {
    if(!open_ || !presenter_.swapchain)return false;
    BOOL fullscreen{};return SUCCEEDED(presenter_.swapchain->GetFullscreenState(&fullscreen,nullptr)) && fullscreen;
}
bool SceneRenderer::PresentationNeedsFollow() const {
    if(!open_ || faulted_ || !presenter_.swapchain)return false;
    return presentation_lost_.load(std::memory_order_acquire) || (exclusive_ && !PresentationFullscreen()) || presenter_.stand_in;   // I-0111: stand-ins follow until a resize succeeds
}
std::optional<SceneRenderer::PresentationLoss> SceneRenderer::FollowPresentation() {
    if(!open_ || faulted_)throw std::invalid_argument("scene presentation follow");
    if(frames_.Recording())throw std::logic_error("native presentation follow inside an open frame list");
    BOOL fullscreen{};Check(presenter_.swapchain->GetFullscreenState(&fullscreen,nullptr),"native full-screen state");
    const bool lost=presentation_lost_.exchange(false,std::memory_order_acq_rel);
    if(!lost && !(exclusive_ && !fullscreen) && !presenter_.stand_in){exclusive_=fullscreen;return std::nullopt;}
    PresentationLoss loss{};if(lost){const std::lock_guard hold(present_failure_lock_);loss=present_failure_;}
    loss.failures=present_failures_.exchange(0,std::memory_order_relaxed);loss.was_exclusive=exclusive_;
    const auto ok=presents_ok_.load(std::memory_order_relaxed);
    // Final review M1: stand-ins count as failing.
    failing_follows_=(lost && ok==follow_ok_seen_) || presenter_.stand_in?failing_follows_+1:0;follow_ok_seen_=ok;loss.failing=failing_follows_;
    // ER FUN_141E9F100: the buffers at the chain's own size (a flip chain presents again only after ResizeBuffers once DXGI
    // has taken it out of exclusive full screen); a new client size follows at the next frame (WM_SIZE, S). Review M2: a Present
    // that keeps failing after it is resized again at the 60th follow, not at every frame.
    // Final review L3: the exclusive state DXGI left always resizes; a chain still exclusive takes the same limit.
    loss.resized=(loss.was_exclusive && !fullscreen) || failing_follows_<3 || failing_follows_%60==0;
    // At the chain's own size only the presenter changes (the stages keep their extent). A failed ResizeBuffers is not fatal
    // unless the device is lost (i101-fs-alttab-1: E_ACCESSDENIED after a focus loss): the buffers as they were, or stand-ins when
    // the chain refuses them (I-0111, i111-fs-quit-1), and the next follow resizes again.
    if(loss.resized)try{frames_.WaitIdle(gpu_,WaitResize);(void)gpu_.Drain(WaitResize);FrameGenerationOff();loss.resize_status=presenter_.TryResizeBuffers(gpu_);
        if(IsDeviceLoss(loss.resize_status))Check(loss.resize_status,"native presentation follow resize");}
    catch(...){faulted_=true;throw;}
    loss.stand_in=presenter_.stand_in;   // final review L2: on every follow
    BOOL now{};exclusive_=SUCCEEDED(presenter_.swapchain->GetFullscreenState(&now,nullptr)) && now;loss.fullscreen=exclusive_;
    return loss;
}
bool SceneRenderer::TogglePresentationFullscreen() {
    if(!open_ || faulted_)throw std::invalid_argument("scene full-screen toggle");
    if(frames_.Recording())throw std::logic_error("native full-screen toggle inside an open frame list");
    frames_.WaitIdle(gpu_,WaitResize);(void)gpu_.Drain(WaitResize);FrameGenerationOff();
    BOOL fullscreen{};Check(presenter_.swapchain->GetFullscreenState(&fullscreen,nullptr),"native full-screen state");
    const auto switched=presenter_.swapchain->SetFullscreenState(!fullscreen,nullptr);
    if(switched==DXGI_ERROR_NOT_CURRENTLY_AVAILABLE || switched==DXGI_STATUS_MODE_CHANGE_IN_PROGRESS)return false;   // the engine retries
    Check(switched,"native full-screen toggle");
    Resize(presenter_.width,presenter_.height);   // the buffers after the switch (FUN_14000B700(GX, 0, 0))
    exclusive_=PresentationFullscreen();   // I-0101
    return true;
}
void SceneRenderer::ApplyDisplayMode(bool fullscreen,bool borderless,uint32_t width,uint32_t height) {
    if(!open_ || faulted_ || !width || !height || width>16384 || height>16384)throw std::invalid_argument("scene display mode");
    if(fullscreen)borderless=false;
    if(frames_.Recording())throw std::logic_error("native display mode inside an open frame list");
    frames_.WaitIdle(gpu_,WaitResize);(void)gpu_.Drain(WaitResize);FrameGenerationOff();
    auto* chain=presenter_.swapchain.Get();BOOL current{};Check(chain->GetFullscreenState(&current,nullptr),"native full-screen state");
    DXGI_MODE_DESC mode{};mode.Width=width;mode.Height=height;mode.Format=presenter_.format;   // I-0138 tranche 5: the chain's
    ComPtr<IDXGIOutput> output;
    if(fullscreen && SUCCEEDED(chain->GetContainingOutput(&output)) && output){mode.RefreshRate={60,1};DXGI_MODE_DESC closest{};
        if(SUCCEEDED(output->FindClosestMatchingMode(&mode,&closest,nullptr)))mode=closest;}
    if(!fullscreen && current)Check(chain->ResizeTarget(&mode),"native display target (leaving full screen)");
    if(bool(current)!=fullscreen){const auto changed=chain->SetFullscreenState(fullscreen,fullscreen?output.Get():nullptr);
        // Review L1: DXGI cannot switch now (the window not in front, a switch in progress): the state stays, and CSWindowImp's
        // retry FUN_140EF1D00 (it compares FUN_14000B540 with its target mode) toggles later, as for the original.
        if(changed==DXGI_ERROR_NOT_CURRENTLY_AVAILABLE || changed==DXGI_STATUS_MODE_CHANGE_IN_PROGRESS)return;
        Check(changed,"native full-screen state change");}
    if(borderless){DXGI_SWAP_CHAIN_DESC desc{};Check(chain->GetDesc(&desc),"native borderless window");
        // The style is the engine's (CSWindowImp restyles for its mode 2: a popup); the presentation places the window over the
        // monitor it is on (review L1: the monitor's rectangle when DXGI has no containing output, a minimised window).
        RECT r{};DXGI_OUTPUT_DESC placed{};MONITORINFO monitor{sizeof(MONITORINFO)};
        if(SUCCEEDED(chain->GetContainingOutput(&output)) && output && SUCCEEDED(output->GetDesc(&placed)))r=placed.DesktopCoordinates;
        else if(GetMonitorInfoW(MonitorFromWindow(desc.OutputWindow,MONITOR_DEFAULTTONEAREST),&monitor))r=monitor.rcMonitor;
        else throw std::runtime_error("native borderless output");
        if(!SetWindowPos(desc.OutputWindow,nullptr,r.left,r.top,r.right-r.left,r.bottom-r.top,SWP_NOZORDER|SWP_FRAMECHANGED))
            throw std::runtime_error("native borderless placement");}
    else Check(chain->ResizeTarget(&mode),"native display target");
    Resize(presenter_.width,presenter_.height);   // the buffers after the change; a new size follows at the next frame (WM_SIZE, S)
    exclusive_=PresentationFullscreen();   // I-0101
}
bool SceneRenderer::FrameGenerationOffered() const noexcept {
    return open_ && !faulted_ && presenter_.upgraded && streamline::FgSupported() && streamline::DlssReady();}   // the step review's M3: DLSS ready
bool SceneRenderer::SetFrameGenerationLive(uint32_t multiplier,std::string& reason) {
    if(!open_ || faulted_)throw std::invalid_argument("scene frame generation");
    if(frames_.Recording())throw std::logic_error("native frame generation inside an open frame list");
    const bool want=multiplier>=2;
    // The step review's M1: what the player wants is kept apart from the plugin's state, so a refused unload leaves DLSS-G Off.
    if(!want)fg_wanted_=false;
    if(!FrameGenerationOffered()){reason="frame generation not offered";return !want;}
    if(want && fg_failed_.load(std::memory_order_relaxed)){reason="frame generation latched off";return false;}   // the final review's L5: first
    if(want==frame_generation_.load(std::memory_order_relaxed)){fg_wanted_=want;if(want)streamline::SetFrameGenerationMultiplier(multiplier);return true;}
    try{
        frames_.WaitIdle(gpu_,WaitResize);(void)gpu_.Drain(WaitResize);FrameGenerationOff();   // §12: DLSS-G off before the chain changes
        // DXGI releases no chain in exclusive full screen (I-0076 review M3): out of it first, its output kept to go back.
        BOOL fullscreen{};ComPtr<IDXGIOutput> output;
        Check(presenter_.swapchain->GetFullscreenState(&fullscreen,&output),"native full-screen state");
        // The final review's M2: DXGI unable to leave it now (as ApplyDisplayMode and the toggle meet it): nothing released yet, the switch
        // refused as busy and tried again at the next frame.
        if(fullscreen){const auto left=presenter_.swapchain->SetFullscreenState(FALSE,nullptr);
            if(left==DXGI_ERROR_NOT_CURRENTLY_AVAILABLE || left==DXGI_STATUS_MODE_CHANGE_IN_PROGRESS){reason="busy: the full screen not left now";return false;}
            Check(left,"native full-screen state (frame generation)");}
        const HWND window=presenter_.presented;const uint32_t width=presenter_.width,height=presenter_.height;
        const auto release=[&]{presenter_.buffers.clear();presenter_.rtv.Reset();presenter_.swapchain.Reset();
            if(presenter_.latency){CloseHandle(presenter_.latency);presenter_.latency=nullptr;}};
        release();
        display_clock_.reset();display_was_engaged_=false;latency_debt_=0;latency_taken_=false;
        (void)gpu_.Drain(WaitResize);   // §18: no work pending on the queue while the feature loads or unloads
        bool switched=streamline::SetFrameGenerationLoaded(want,reason);   // refused: the chain made again as it was
        const auto open=[&]{
            presenter_.count=present_buffers_;presenter_.max_latency=frame_slots_;presenter_.waitable=streamline::FgLoaded()?false:present_waitable_;   // §12.1
            if(display_clock_requested_ && DisplayClock::Fits(RefreshPeriodOf(window)))presenter_.max_latency=1;
            presenter_.tearing=present_tearing_;presenter_.Open(gpu_,window,width,height);};
        open();
        // The step review's M4: a proxy chain refused ends Streamline (Open), the chain made without our frame-latency object: made again
        // with it (DLSS is off for the session then, as at a start whose proxy is refused).
        if(!presenter_.upgraded && !presenter_.waitable && present_waitable_){
            if(!presenter_.upgrade_failure.empty()){reason="proxy chain refused: "+presenter_.upgrade_failure;upscaler_state_="refused: "+reason;}switched=false;
            (void)gpu_.Drain(WaitResize);release();open();}
        frame_generation_=streamline::FgLoaded() && presenter_.upgraded;   // the step review's M3
        frame_generation_loaded_now=streamline::FgLoaded();   // backend.h's FrameGenerationFenceNotice
        fg_wanted_=want && frame_generation_.load(std::memory_order_relaxed);
        if(fg_wanted_)streamline::SetFrameGenerationMultiplier(multiplier);
        fg_presented_on_=false;fg_bad_status_=0;
        if(!presenter_.upgrade_failure.empty())upscaler_state_="refused: "+presenter_.upgrade_failure;
        // The step review's M5: back to exclusive full screen at the mode ApplyDisplayMode chooses (the nearest to 60 Hz at this size).
        if(fullscreen && output){DXGI_MODE_DESC mode{};mode.Width=width;mode.Height=height;mode.Format=presenter_.format;mode.RefreshRate={60,1};
            DXGI_MODE_DESC closest{};if(SUCCEEDED(output->FindClosestMatchingMode(&mode,&closest,nullptr)))mode=closest;
            const auto changed=presenter_.swapchain->SetFullscreenState(TRUE,output.Get());
            if(SUCCEEDED(changed)){(void)presenter_.swapchain->ResizeTarget(&mode);Resize(width,height);}}   // not available now: windowed, the engine's retry toggles later (ApplyDisplayMode's review L1)
        pacer_.Reset();presentation_lost_=false;present_failures_=0;follow_ok_seen_=presents_ok_.load();failing_follows_=0;
        exclusive_=PresentationFullscreen();
        return switched && frame_generation_.load(std::memory_order_relaxed)==want;}
    catch(...){faulted_=true;throw;}
}
void SceneRenderer::Close() {
    frame_generation_loaded_now=false;   // I-0132 R12f (the final review's L13): backend.h's filter disarmed with the renderer
    // I-0111 step review L3: a Start that failed after Streamline's initialisation ends it here too.
    if(!gpu_.device) {streamline::Shutdown();gpu_.Close();frame_uploads_.QuarantineToProcessExit();open_=faulted_=false;last_serial_=startup_serial_=0;PublishSerials();return;}
    // cD6 review: the frames still in flight are waited for and released first, so their release-time work (diagnostics,
    // witness files) runs; their results stay in the channel for the caller (TakeCompleted after Close).
    if(!faulted_ && !frames_.Recording()){try{frames_.WaitIdle(gpu_,WaitClose);}catch(...){}}
    frames_.Flush();   // I-0090 review: the retired holders gone before the device closes, also after a fault (their tickets passed)
    // I-0111 (R1): with Streamline, its proxy chain is released (out of exclusive full screen first), then DLSS's resources and
    // slShutdown, all before the device; without it the order below is unchanged.
    bool streamline_drained=false;
    if(streamline::Initialised()){
        FrameGenerationOff();   // I-0131 R10b (the step review's M1): DLSS-G off before the full screen is left and the chain released (§12)
        if(faulted_ || frames_.Recording()){try{(void)gpu_.Drain(WaitClose);streamline_drained=true;}catch(...){}}   // final review L6: no DLSS work in flight when NGX frees its resources (T5c's L5: marked done once it returned)
        if(presenter_.swapchain){BOOL fullscreen{};if(SUCCEEDED(presenter_.swapchain->GetFullscreenState(&fullscreen,nullptr)) && fullscreen)(void)presenter_.swapchain->SetFullscreenState(FALSE,nullptr);}
        presenter_.buffers={};presenter_.swapchain.Reset();streamline::Shutdown();}
    nis_in_.Reset();nis_out_.Reset();   // I-0132 R12f
    dlss_output_.Reset();render_width_=render_height_=0;dlss_mode_=streamline::DlssMode::Dlaa;   // step review L7
    dlss_exposure_.Reset();dlss_exposure_failed_=false;
    // I-0118 final review L6: after a fault (or a frame still recording) the queue is drained before the rays' pipelines and
    // structures go (their lists may still run them).
    // T5c (the fixes' step review L2): not again when Streamline's close above drained.
    if((faulted_ || frames_.Recording()) && !streamline_drained){try{(void)gpu_.Drain(WaitClose);}catch(...){}}
    rt_materials_.Close(rt_heap_.load(std::memory_order_acquire),gpu_.LastSubmitted());rt_material_table_=rt_material_rows_=0;   // R3a: the views released after the drain
    rt_tlas_.Close();rt_skinner_.Close();rt_skinned_instances_.clear();rt_failed_=false;rt_failure_.clear();rt_tlas_bytes_=0;   // I-0117 (R2 T1, T2): after the queue's idle
    // I-0140 t31: the menu views' table, top level and self-shadow depth views, after the drain.
    for(auto& table:menu_rt_materials_)table.Close(rt_heap_.load(std::memory_order_acquire),gpu_.LastSubmitted());
    for(auto& tlas:menu_rt_tlas_)tlas.Close();menu_rt_skinned_.clear();
    menu_rt_scene_=menu_rt_table_=menu_rt_rows_=0;menu_rt_frame_=UINT64_MAX;menu_rt_used_=0;for(auto& [extent,set]:offscreen_sets_){(void)extent;ReleaseSelfShadowViews(set);}
    // The final review's L8: the menu state starts again with the device (its failure as rt_failed_'s). menu_shadow_ready_ stays with
    // the shadowed sun pipeline it reports, which Close keeps (the lighting programmes resolve once a process).
    menu_rt_ticked_=UINT64_MAX;menu_rt_idle_.fill(kMenuRtIdleTicks);
    menu_shadow_failed_.store(false,std::memory_order_release);menu_shadow_failure_.clear();
    CloseRtView();   // T4: its slots released after the drain
    CloseRtSun();    // R5: its slot and pipelines
    CloseRtLights(); // I-0122 R4a: its slot, pipeline and check images
    CloseRr();       // I-0123 RR-a: the guides' pipeline and targets
    CloseRtDenoise();   // I-0126 R7c: NRD's instance, pipelines, textures and slots
    CloseRtGiMean();   // I-0126 R7d: the bias check's target and slot
    CloseRtMainViews();   // R4b: the views the three share
    depth_out_.Reset();depth_out_dsv_.Reset();depth_out_views_.Reset();depth_out_root_.Reset();depth_out_pso_.Reset();depth_out_view_generation_=UINT64_MAX;
    gpu_.Close();WithdrawRetirementQueue(&gpu_);frame_uploads_.Reset();
    frames_.Close();main_view_.model_targets=false;
    primitive_uploads_pending_=false;primitives_.reset();primitive_depth_.Reset();primitive_dsv_.Reset();primitive_stencil_initialized_=false;primitive_generation_=primitive_texture_generation_=0;
    for(auto& color:main_view_.colors) color.Reset();
    // I-0076 review M3: DXGI does not release a swap chain in exclusive full screen; it leaves it first.
    if(presenter_.swapchain){BOOL fullscreen{};if(SUCCEEDED(presenter_.swapchain->GetFullscreenState(&fullscreen,nullptr)) && fullscreen)(void)presenter_.swapchain->SetFullscreenState(FALSE,nullptr);}
    presenter_.buffers={};presenter_.rtv.Reset();presenter_.swapchain.Reset();if(presenter_.latency){CloseHandle(presenter_.latency);presenter_.latency=nullptr;}
    main_view_.depth.Reset();
    effect_depth_copy_.Reset();{const std::lock_guard hold(water_depth_lock_);water_depth_copy_.Reset();}   // I-0113 tranche 15 step review M1: the closed device's copies
    effect_layer_={};effect_layer_failure_.clear();   // I-0113 tranche 17 step review H1: the layer's objects belong to the closed device
    main_view_.rtvs.Reset();main_view_.dsv.Reset();main_view_.bindings.Reset();offscreen_sets_.clear();
    composition_pso_.Reset();offscreen_composition_pso_.Reset();offscreen_background_pso_.Reset();offscreen_composition_full_pso_.Reset();offscreen_composition_root_.Reset();composition_root_.Reset();open_=faulted_=false;last_serial_=startup_serial_=0;PublishSerials();
    display_.Close();display_open_=false;display_failed_=false;display_failure_.clear();   // W3 (the blobs stay for a reopened device)
    output_.Close();hdr_factory_.Reset();hdr_monitor_=nullptr;hdr_refusal_.clear();hdr_scene_failed_=false;hdr_scene_failure_.clear();ui_format_skips_=0;hdr_probe_fresh_=false;hdr_display_={};hdr_peak_=0;presenter_.hdr_wanted=presenter_.hdr10=false;presenter_.format=DXGI_FORMAT_R8G8B8A8_UNORM;presenter_.hdr_failure.clear();   // I-0138 tranche 5
    overlay_.Close();screen_layer_.Close();tint_layer_.Close();overlay_failed_=screen_layer_failed_=false;   // I-0132 R12a, R12e
    {std::lock_guard guard(overlay_failure_lock_);overlay_failure_.clear();}   // the engine thread reads it (the final review's Low)
    glow_.Close();glow_failed_=false;glow_failure_.clear();   // W6
    taa_.Close();taa_failed_=false;dlss_sharpen_failed_=false;taa_last_serial_=0;   // I-0132 R11j
    menu_taa_.clear();menu_taa_failed_=false;menu_dlss_failed_=false;menu_dlss_sharpen_failed_=false;menu_dlss_full_told_=false;menu_dlss_retired_.clear();menu_dlss_parked_.clear();   // I-0140 tranche 40 (their resources the closed device's; the contexts go with offscreen_sets_)
    taa_base_.Reset();taa_base_failed_=false;   // I-0113 tranche 39
    taa_fade_mask_.Reset();taa_fade_mask_rtv_.Reset();taa_fade_mask_failed_=false;   // I-0140
    effects_.Close();effects_failure_.clear();   // FFX-DRAW N6 (review)
    {const std::lock_guard hold(effects_lock_);effect_queries_.Reset();effect_query_readback_.Reset();effect_query_frequency_=0;effects_window_={};effect_scan_={};}effect_query_failed_=false;   // I-0127 step review M1: the timestamps belong to the closed device
    for(auto& pair:effect_scan_readback_)for(auto& b:pair)b.Reset();effect_scan_bytes_=0;effect_scan_failed_=false;   // I-0113 tranche 29: the scans' buffers belong to the closed device too
    draw_scan_root_.Reset();draw_scan_pso_.Reset();draw_scan_views_.Reset();draw_scan_counts_.Reset();for(auto& b:draw_scan_readback_)b.Reset();   // tranche 30 (the final review's L1)
    draw_scan_failed_=false;draw_scan_target_=nullptr;draw_scan_points_.clear();
    rt_gpu_.Reset();   // I-0128 R8a: the same for the traced passes' timer
    post_tone_.Close();tonemap_failed_=false;tonemap_failure_.clear();   // W10 TM-2; I-0138 tranche 1
    auto_exposure_.Close();auto_exposure_failed_=false;   // I-0132 R11h
    {const std::lock_guard hold(auto_exposure_lock_);auto_exposure_failure_.clear();auto_exposure_reading_={};}
    grading_failed_=false;grading_failure_.clear();   // W10 GR (our pass's LUT side closes with post_tone_)
    glare_family_.Close();glare_family_failed_=false;   // W10 GL-3
    dof_stage_.Close();dof_failed_=false;dof_failure_.clear();   // W10 DF-4
    motionblur_stage_.Close();motionblur_failed_=false;motionblur_failure_.clear();   // W10 MB-4
    mask_.Close();mask_failed_=false;mask_failure_.clear();mask_target_bytes_=0;   // I-0126 R7c (W8 AO-7 before)
    // I-0107: the light grid of this incarnation (a reopened device makes them again).
    light_grid_={};light_grid_frame_={};light_grid_ready_=0;organize_root_.Reset();organize_pso_.Reset();organize_programme_=nullptr;
    organize_views_.Reset();organize_views_depth_=nullptr;organize_views_grid_=0;organize_views_generation_=UINT64_MAX;organize_failed_=false;
    {const std::lock_guard hold(channel_lock_);organize_failure_.clear();}
    // 1186 velocity: device objects of this incarnation.
    velocity_target_.Reset();motion_vectors_.Reset();for(auto& readbacks:slot_readbacks_)readbacks={};
    velocity_rtvs_.Reset();velocity_views_.Reset();velocity_root_.Reset();velocity_resolve_pso_.Reset();
    velocity_half_.Reset();velocity_half_root_.Reset();velocity_half_pso_.Reset();   // W10 MB-4
    velocity_depth_.Reset();velocity_dsv_.Reset();velocity_occlusion_root_.Reset();velocity_occlusion_pso_.Reset();   // I-0136 (its final review's M1)
    velocity_generation_=UINT64_MAX;scene_colour_copy_.Reset();
    movie_root_.Reset();movie_pso_.Reset();movie_quad_.Reset();for(auto& plane:movie_planes_)plane.Reset();movie_views_.Reset();
    movie_plane_sizes_={};movie_rtv_.Reset();{const std::lock_guard hold(channel_lock_);velocity_request_.clear();}last_completed_={};released_errors_=0;released_diagnostic_.clear();
    main_view_=ViewContext{};   // cD2 review: every per-view field of the main view, lighting heaps and generations included (a reopened device recreates them)
}
SceneRenderer::~SceneRenderer() {
    try {Close();} catch(...) {
        // A failed wait is not completion. Preserve prepared draws and their
        // CPU/COM leases until process exit, like the existing GPU quarantine.
        frames_.QuarantineToProcessExit();   // cD1: every slot's list, allocator, draws and held frame inputs
        if(primitives_){primitives_->QuarantineToProcessExit();(void)new std::shared_ptr<PrimitivePass>(std::move(primitives_));}(void)primitive_depth_.Detach();(void)primitive_dsv_.Detach();
        for(auto& color:main_view_.colors) (void)color.Detach();
        for(auto& buffer:presenter_.buffers) (void)buffer.Detach();(void)presenter_.rtv.Detach();(void)presenter_.swapchain.Detach();
        (void)main_view_.depth.Detach();
        display_.QuarantineToProcessExit();   // W3
        output_.QuarantineToProcessExit();   // I-0138 tranche 5
        overlay_.QuarantineToProcessExit();screen_layer_.QuarantineToProcessExit();tint_layer_.QuarantineToProcessExit();   // I-0132 R12a, R12e
        glow_.QuarantineToProcessExit();   // W6
        taa_.QuarantineToProcessExit();   // I-0132 R11j
        for(auto& [target,entry]:menu_taa_){(void)target;if(entry.stage){entry.stage->QuarantineToProcessExit();(void)entry.stage.release();}}   // I-0140 tranche 40
        effects_.QuarantineToProcessExit();   // FFX-DRAW N6 (review)
        post_tone_.QuarantineToProcessExit();   // W10 TM-2; I-0138 tranche 1
        auto_exposure_.QuarantineToProcessExit();   // I-0132 R11h
        glare_family_.QuarantineToProcessExit();dof_stage_.QuarantineToProcessExit();motionblur_stage_.QuarantineToProcessExit();   // W10 GL-3, DF-4, MB-4
        mask_.QuarantineToProcessExit();   // I-0126 R7c
        (void)main_view_.rtvs.Detach();(void)main_view_.dsv.Detach();(void)main_view_.bindings.Detach();(void)composition_pso_.Detach();(void)offscreen_composition_pso_.Detach();(void)offscreen_background_pso_.Detach();(void)offscreen_composition_full_pso_.Detach();
        for(auto& [extent,set]:offscreen_sets_){(void)extent;   // review 1180: the menu view target sets too
            (void)set.bindings.Detach();(void)set.rtvs.Detach();(void)set.dsv.Detach();(void)set.light_views.Detach();(void)set.light_views_cpu.Detach();(void)set.light_rtvs.Detach();
            for(auto& color:set.colors)(void)color.Detach();for(auto& light:set.light_targets)(void)light.Detach();
            (void)set.depth.Detach();(void)set.env_target.Detach();(void)set.sss_temp.Detach();(void)set.output_rtv.Detach();
            (void)set.self_mask.Detach();(void)set.self_mask_rtv.Detach();(void)set.self_blur.Detach();(void)set.self_blur_rtv.Detach();
            (void)set.taa_output.Detach();(void)set.taa_velocity.Detach();(void)set.taa_motion.Detach();(void)set.taa_rtvs.Detach();(void)set.taa_views.Detach();}   // I-0140 t31's final review L1, t35
        // cD1 (phase D sheet R7): the members the quarantine missed before the ring (movie, lighting, environment, SSS,
        // velocity, scene-colour copy), any of which a list in flight may reference.
        const auto leak=[](auto& resource){(void)resource.Detach();};
        leak(movie_root_);leak(movie_pso_);leak(movie_quad_);for(auto& plane:movie_planes_)leak(plane);leak(movie_views_);leak(movie_rtv_);for(auto& readbacks:slot_readbacks_){leak(readbacks.backbuffer);leak(readbacks.velocity);leak(readbacks.motion);}
        leak(native_light_root_);for(auto& pso:native_light_psos_)leak(pso);for(auto& target:main_view_.light_targets)leak(target);
        for(auto& neutral:native_light_neutral_)leak(neutral);leak(native_light_quad_);leak(native_light_list_);leak(main_view_.light_views);leak(main_view_.light_views_cpu);leak(main_view_.light_rtvs);
        for(auto& [programme,pso]:native_point_psos_){(void)programme;leak(pso);}
        leak(native_sss_root_);for(auto& pso:native_sss_psos_)leak(pso);leak(main_view_.sss_temp);
        leak(native_env_root_);for(auto& pso:native_env_psos_)leak(pso);leak(main_view_.env_target);leak(native_env_box_);
        leak(velocity_target_);leak(motion_vectors_);leak(velocity_rtvs_);leak(velocity_views_);
        leak(velocity_root_);leak(velocity_resolve_pso_);leak(scene_colour_copy_);leak(velocity_half_);leak(velocity_half_root_);leak(velocity_half_pso_);
        for(auto* set:{&light_grid_,&light_grid_frame_}){leak(set->indices);leak(set->lookup);leak(set->work);leak(set->points);leak(set->spots);leak(set->lines);}   // I-0107
        leak(organize_root_);leak(organize_pso_);leak(organize_views_);
        leak(taa_base_);   // I-0113 tranche 39
        leak(taa_fade_mask_);(void)taa_fade_mask_rtv_.Detach();   // I-0140
        leak(effect_depth_copy_);   // I-0113 tranche 17 final review L4: the effects' depth copy and the 0x31 layer's objects
        {auto& l=effect_layer_;leak(l.color);leak(l.depth);leak(l.rtv);leak(l.dsv);leak(l.table);leak(l.root);leak(l.downsample);leak(l.composite);leak(l.bilateral);}
        (void)composition_root_.Detach();(void)offscreen_composition_root_.Detach();frame_uploads_.QuarantineToProcessExit();(void)native_light_zeros_.Detach();gpu_.QuarantineToProcessExit();
    }
}
}
namespace ds3native {
EffectMeasureWindow SceneRenderer::TakeEffectMeasure() {
    EffectMeasureWindow out;{const std::lock_guard hold(effects_lock_);std::swap(out,effects_window_);out.gpu_frequency=effect_query_frequency_;}
    LARGE_INTEGER q{};QueryPerformanceFrequency(&q);out.cpu_frequency=uint64_t(q.QuadPart);return out;}
EffectScanWindow SceneRenderer::TakeEffectScan() {EffectScanWindow out;const std::lock_guard hold(effects_lock_);std::swap(out,effect_scan_);return out;}
std::string SceneRenderer::EffectSummary() const {
    const std::lock_guard hold(effects_lock_);
    char row[700]{};sprintf_s(row,"{\"draws\":%llu,\"skipped\":%llu,\"pipelines\":%llu,\"pipeline_failures\":%llu,\"textures\":%llu,\"missing_textures\":%llu,\"other_passes\":%llu,\"gpu\":[%llu,%llu,%llu,%llu,%llu,%llu,%llu],\"args\":[%u,%u,%u,%u,%u,%u,%u,%u],\"last_failure\":\"%.200s\"}",
        static_cast<unsigned long long>(effects_stats_.draws),static_cast<unsigned long long>(effects_stats_.skipped),static_cast<unsigned long long>(effects_stats_.pipelines),
        static_cast<unsigned long long>(effects_stats_.pipeline_failures),static_cast<unsigned long long>(effects_stats_.textures),
        static_cast<unsigned long long>(effects_stats_.missing_textures),static_cast<unsigned long long>(effects_stats_.other_passes),static_cast<unsigned long long>(effects_stats_.gpu_statements),
        static_cast<unsigned long long>(effects_stats_.dispatches),static_cast<unsigned long long>(effects_stats_.indirect_dispatches),static_cast<unsigned long long>(effects_stats_.copies),
        static_cast<unsigned long long>(effects_stats_.clears),static_cast<unsigned long long>(effects_stats_.particle_draws),static_cast<unsigned long long>(effects_stats_.gpu_skipped),
        effects_stats_.args[0],effects_stats_.args[1],effects_stats_.args[2],effects_stats_.args[3],effects_stats_.args[4],effects_stats_.args[5],effects_stats_.args[6],effects_stats_.args[7],effects_last_failure_.c_str());
    std::string out(row);out.pop_back();out+=",\"row_runs\":"+std::to_string(effects_stats_.row_runs)+",\"copy_rects\":"+std::to_string(effects_stats_.copy_rects)+",\"target_draws\":"+std::to_string(effects_stats_.target_draws)+",\"target_clears\":"+std::to_string(effects_stats_.target_clears)+",\"target_copies\":"+std::to_string(effects_stats_.target_copies)+",\"counter_writes\":"+std::to_string(effects_stats_.counter_writes)+",\"counters\":[";for(size_t i=0;i<8;++i)out+=(i?",":"")+std::to_string(effects_stats_.counters[i]);out+="],\"particles\":[";   // E07 witness: the first two particles of the drawn buffer
    for(size_t i=0;i<effects_stats_.particles.size();++i){char cell[24]{};sprintf_s(cell,"%s%.5g",i?",":"",effects_stats_.particles[i]);out+=cell;}
    return out+"]}";
}
}
