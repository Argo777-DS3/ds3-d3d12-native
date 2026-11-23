// The scene renderer's world frame: the stages opened on first use (gamma, glow, AA, tone map, grading, sun shadow, AO),
// the frame job, RenderModels and the frame graph's declarations (scene_pass_internal.h has the shared helpers).
#include "scene_pass_internal.h"
#include "streamline_host.h"
// I-0132 R12h step 2: the effect layer's programmes built by fxc (shaders/effect_layer.hlsl).
#include "effect_layer_vs.dxbc.h"
#include "effect_layer_downsample.dxbc.h"
#include "effect_layer_composite.dxbc.h"
#include "effect_layer_bilateral.dxbc.h"
#include <chrono>

namespace ds3native {
namespace {
// FFX-DRAW (Native2027): the first effect pass after 0x29 (0x2D distortion, 0x31) with a draw sampling the scene copy; 0: none, or
// the frame opts out (no-ffx-effect-copy.flag). I-0113 tranche 23: kept beside the distortion quads' own copies (CopyRect): the
// original also copies the whole target into the frame copy around pass 0x2D (i113-t23-dist2-original: two full copies a frame,
// one recorded just before the first quad's region copy on another thread; their GPU order not read).
int32_t EffectCopyPass(const NativeEffectFrame& frame) {
    if(!frame.effect_copy)return 0;
    for(const int32_t pass:{0x2D,0x31})
        for(const auto& draw:frame.draws)
            if(draw.pass==pass && (draw.op==NativeEffectOp::Draw || draw.op==NativeEffectOp::DrawIndirect) &&
               std::any_of(draw.textures.begin(),draw.textures.end(),[](const NativeEffectTexture& t){return t.kind==NativeEffectTextureKind::SceneCopy;}))return pass;
    return 0;
}
// I-0113 tranche 15: whether a statement the stage draws (passes 0x21, 0x29, 0x2D, 0x31) reads the scene depth (t3, a GPU particle's
// resources). I-0140: a 0x21 draw reading t3 (not checked: no witness counts 0x21's textures) reads the last copy taken (an earlier
// frame's, or a zeroed one after a resize), this frame's being taken after 0x21's draws.
bool EffectReadsDepth(const NativeEffectFrame& frame) {
    const auto depth=[](const NativeEffectTexture& t){return t.kind==NativeEffectTextureKind::Depth;};
    for(const auto& draw:frame.draws)
        if((draw.pass==0x21 || draw.pass==0x29 || draw.pass==0x2D || draw.pass==0x31) && (std::any_of(draw.textures.begin(),draw.textures.end(),depth) ||
           std::any_of(draw.cs_resources.begin(),draw.cs_resources.end(),depth) || std::any_of(draw.vs_resources.begin(),draw.vs_resources.end(),depth)))return true;
    return false;
}
// I-0113 tranche 17: whether the frame draws in pass 0x31 (its half-size layer is then made, drawn and composited).
bool EffectDrawsLayer(const NativeEffectFrame& frame) {
    return frame.effect_layer && std::any_of(frame.draws.begin(),frame.draws.end(),[](const NativeEffectDraw& d){
        return d.pass==0x31 && (d.op==NativeEffectOp::Draw || d.op==NativeEffectOp::DrawIndirect);});
}
}
// W3: the gamma stage on the recording thread, opened at the presentation extent the first time a frame needs it (Resize
// keeps it at the new extent); a failure is kept for the session and the frames draw straight into the back buffer.
bool SceneRenderer::DisplayReady() {
    if(!display_blobs_ || display_failed_)return false;
    if(display_.IsOpen()){
        if(display_.Width()!=presenter_.width || display_.Height()!=presenter_.height)throw std::logic_error("display stage extent differs from the presentation");
        return true;}
    std::string reason;
    if(display_.Open(gpu_.device.Get(),*display_blobs_,presenter_.width,presenter_.height,reason)){display_open_=true;return true;}
    display_failure_=reason;display_failed_=true;return false;
}
// I-0138 tranche 5: the HDR10 chain's last pass (scene_pass.h).
bool SceneRenderer::HdrOutputReady(bool display) {
    if(!presenter_.Hdr10Format())return false;
    if(!display)throw std::runtime_error("HDR10 chain without the display stage: "+display_failure_);
    std::string reason;
    if(!output_.IsOpen() && !output_.Open(gpu_.device.Get(),reason))throw std::runtime_error("HDR10 output stage: "+reason);
    if(!output_.Targets(presenter_.width,presenter_.height,reason))throw std::runtime_error("HDR10 output picture: "+reason);
    return true;
}
bool SceneRenderer::HdrToneReady() {
    if(hdr_scene_failed_)return false;
    std::string reason;
    if(post_tone_.IsOpen() && post_tone_.OpenHdr(OutputStage::SceneFormat(),reason))return true;
    hdr_scene_failure_=reason.empty()?"the tone-map stage is not open":reason;hdr_scene_failed_=true;return false;
}
void SceneRenderer::RecordHdrScene(ID3D12GraphicsCommandList* list,uint64_t serial,D3D12_CPU_DESCRIPTOR_HANDLE target,const D3D12_VIEWPORT& viewport,const D3D12_RECT& scissor) {
    // The dither's blue noise moves every frame (the same place for the HUD-less colour and the final one of a frame).
    auto params=hdr::BuildOutputParams(hdr_request_.paper_white,hdr_peak_);params.flags=hdr::kOutputDither;
    params.noise_offset[0]=int32_t((serial*23u)&63u);params.noise_offset[1]=int32_t((serial*41u)&63u);
    output_.RecordScene(list,target,params,[this](uint64_t bytes){
        const auto range=frame_uploads_.Allocate(gpu_.device.Get(),bytes);return OutputConstantMemory{range.cpu,range.gpu};},viewport,scissor);
}
void SceneRenderer::RecordHdrCopy(ID3D12GraphicsCommandList* list,const D3D12_VIEWPORT& viewport,const D3D12_RECT& scissor) {
    output_.RecordCopy(list,display_.Intermediate(),display_.IntermediateGeneration(),viewport,scissor);
}
// I-0132 R12a: the overlay after the UI (both frame kinds call it with the UI's target): opened at its first frame; a failure is kept
// for the session and nothing is drawn from then on (the game's own UI is unchanged either way).
std::shared_ptr<const overlay::Frame> SceneRenderer::OverlaySnapshot(){
    return overlay_failed_?nullptr:overlay_.Take(presenter_.width,presenter_.height);}
void SceneRenderer::RecordOverlay(ID3D12GraphicsCommandList* list,D3D12_CPU_DESCRIPTOR_HANDLE target,FrameSlot& slot,const std::shared_ptr<const overlay::Frame>& frame,OverlayPass& pass,bool float_target){
    const bool screen=&pass==&screen_layer_ || &pass==&tint_layer_;   // R12e: the tint layer shares the screen layer's latch
    if(!frame || overlay_failed_ || (screen && screen_layer_failed_))return;   // a pass declared for this frame stays declared even when nothing is drawn (its key bit is the snapshot's)
    std::string reason;
    // I-0132 R12e (its second step review's L5, L6; the final review's L3): the three passes opened at the overlay's first frame (the screen
    // layer's compile not at the title's), a failure naming its pass; the screen layer's failure its own (the overlay and the precompile
    // screen go on without the embers).
    const char* const name=&pass==&tint_layer_?"tint ":screen?"screen ":"";   // the final review's L1
    if(!pass.IsOpen()){
        for(OverlayPass* each:{&overlay_,&screen_layer_,&tint_layer_}){const bool layer=each!=&overlay_;
            const auto mode=each==&tint_layer_?OverlayPass::Blend::Multiply:layer?OverlayPass::Blend::Screen:OverlayPass::Blend::Alpha;
            if(each->IsOpen() || (layer && screen_layer_failed_) || each->Open(gpu_.device.Get(),DisplayStage::Format(),reason,mode))continue;
            std::lock_guard guard(overlay_failure_lock_);   // I-0132 R12h's L7: the overlay's own failure not overwritten by a layer's (OverlayLost, 16b)
            if(!layer || overlay_failure_.empty() || overlay_failure_.rfind("screen ",0)==0 || overlay_failure_.rfind("tint ",0)==0)
                overlay_failure_=std::string(each==&tint_layer_?"tint ":layer?"screen ":"")+"open: "+reason;
            if(layer)screen_layer_failed_=true;else overlay_failed_=true;}
        if(!pass.IsOpen())return;}
    try{(void)pass.Record(list,target,presenter_.width,presenter_.height,frame,[&](uint64_t bytes){
            const auto range=frame_uploads_.Allocate(gpu_.device.Get(),bytes,UploadArena::alignment,false);return std::pair<uint8_t*,D3D12_GPU_VIRTUAL_ADDRESS>{range.cpu,range.gpu};},slot.kept,float_target);
        // I-0138 tranche 5: a pass that could not make its float pipeline draws nothing on an HDR10 chain's frames: named (the
        // overlay's own failure is not overwritten, nothing is latched: the pass goes on on an SDR chain).
        if(float_target && pass.FloatFailed()){std::lock_guard guard(overlay_failure_lock_);
            if(overlay_failure_.empty())overlay_failure_=std::string(name)+"float pipeline (nothing drawn on an HDR10 chain): "+pass.FloatFailure();}}
    catch(const std::exception& e){std::lock_guard guard(overlay_failure_lock_);   // a layer's failure never over the overlay's own (R12h's L7)
        if(!screen || overlay_failure_.empty() || overlay_failure_.rfind("screen ",0)==0 || overlay_failure_.rfind("tint ",0)==0)overlay_failure_=std::string(name)+"record: "+e.what();
        (screen?screen_layer_failed_:overlay_failed_)=true;}
}
// W6 GL-7: the glow stage, opened at the presentation extent from the first frame that carries the pinned programmes, into
// the 8-bit colour target (the gamma intermediate or the back buffer, both R8G8B8A8_UNORM) or, on an HDR10 chain's frame whose
// tone map draws in float, into the output stage's float picture; a failure is kept for the session.
bool SceneRenderer::GlowReady(const NativeGlowInput& input) {
    if(glow_failed_ || !input.programs)return false;
    std::string reason;
    if(glow_.IsOpen()){
        if(glow_.Width()!=presenter_.width || glow_.Height()!=presenter_.height){glow_failure_="glow stage extent differs from the presentation";glow_failed_=true;return false;}   // review: disable, not fault
    }else if(!glow_.Open(gpu_.device.Get(),input.programs->blobs,DisplayStage::Format(),presenter_.width,presenter_.height,reason)){glow_failure_=reason;glow_failed_=true;return false;}
    // I-0138 tranche 5: on an HDR10 chain the final composite may draw on the float picture: its pipeline, made once.
    if(presenter_.Hdr10Format() && !glow_.FloatFinal(input.programs->blobs,reason)){glow_failure_=reason;glow_failed_=true;return false;}
    return true;
}
// I-0132 R11j: the TAA stage, opened at its first frame; a failure (the stage's or its output target's) latches it off for the session,
// its row once (the engine thread stops the jitter from the next frame, TaaFailed).
void SceneRenderer::LatchTaaOff(uint64_t serial,const std::string& reason){
    if(taa_failed_.exchange(true) || !upscaler_sink_)return;
    std::string why=reason.substr(0,200);for(auto& c:why)if(c=='"' || c=='\\' || c<32)c=' ';char row[320]{};
    sprintf_s(row,"{\"event\":\"native_taa_latched_off\",\"serial\":%llu,\"after_frames\":%llu,\"reason\":\"%s\"}\n",static_cast<unsigned long long>(serial),
        static_cast<unsigned long long>(taa_.Frames()),why.c_str());upscaler_sink_(row);
}
// I-0132 R12f: NIS's two targets at the presentation's extent (8-bit, as the display's colour), made again at a new extent (the frames
// that used the former ones hold them); false while Streamline's NIS is not ready or a target cannot be made (NIS then latched off).
bool SceneRenderer::NisTargets(){
    if(!streamline::NisReady())return false;
    const auto fits=[&](ID3D12Resource* r){if(!r)return false;const auto d=r->GetDesc();return d.Width==presenter_.width && d.Height==presenter_.height;};
    if(fits(nis_in_.Get()) && fits(nis_out_.Get()))return true;
    if(nis_in_ || nis_out_)streamline::NisFreeResources();   // its step review's H1
    try{const auto heap=Heap(D3D12_HEAP_TYPE_DEFAULT);ComPtr<ID3D12Resource> in,out;
        const auto plain=Image(presenter_.width,presenter_.height,1,DisplayStage::Format(),D3D12_RESOURCE_FLAG_NONE);
        const auto writable=Image(presenter_.width,presenter_.height,1,DisplayStage::Format(),D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        Check(CountedResource(gpu_.device.Get())->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&plain,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,nullptr,IID_PPV_ARGS(&in)),"nis input");
        Check(CountedResource(gpu_.device.Get())->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&writable,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&out)),"nis output");
        nis_in_=std::move(in);nis_out_=std::move(out);return true;}
    catch(const std::exception&){streamline::NisDisable();return false;}
}
bool SceneRenderer::TaaReady(uint64_t serial){
    if(taa_failed_)return false;
    if(taa_.IsOpen())return true;
    std::string reason;if(taa_.Open(gpu_.device.Get(),reason))return true;
    LatchTaaOff(serial,"open: "+reason);return false;
}
// I-0140 tranche 40 (D-0305): a menu view's TAA stage by its target, opened at its first frame; those left unused 600 frames retire (their
// histories held by the slots that used them, long completed). A failure latches every menu view's TAA off for the session, its row once
// (the engine thread stops their jitter from the next frame, MenuTaaFailed).
void SceneRenderer::LatchMenuTaaOff(uint64_t serial,const std::string& reason){
    if(menu_taa_failed_.exchange(true) || !upscaler_sink_)return;
    std::string why=reason.substr(0,200);for(auto& c:why)if(c=='"' || c=='\\' || c<32)c=' ';char row[320]{};
    sprintf_s(row,"{\"event\":\"native_menu_taa_latched_off\",\"serial\":%llu,\"after_frames\":%llu,\"reason\":\"%s\"}\n",static_cast<unsigned long long>(serial),
        static_cast<unsigned long long>(menu_taa_frames_.load(std::memory_order_relaxed)),why.c_str());upscaler_sink_(row);
}
// An entry unused 600 frames leaves (its DLAA viewport parked for the next entry on its inputs, menu_dlss_parked_), a retired viewport
// is freed 600 frames on (no list of it in flight), a parked one beyond the four last, after 36000 frames or once its context is gone;
// run by every menu view's frame, whatever its AA (the fixes' review M1: a latch, an untabled view or the option off still sweep), and
// by the main and the no-world frames (the final review's L2: a closed menu's histories go).
void SceneRenderer::SweepMenuAa(ID3D12Resource* target){
    if(menu_taa_.empty() && menu_dlss_retired_.empty() && menu_dlss_parked_.empty())return;
    const auto numbered=frames_.Numbered();
    std::erase_if(menu_taa_,[&](const auto& entry){const auto& e=entry.second;const bool idle=entry.first!=target && numbered>=e.last_frame+600;
        if(idle && e.dlss_viewport)menu_dlss_parked_.push_back({e.dlss_viewport,e.dlss_inputs,e.dlss_made,numbered});return idle;});
    std::erase_if(menu_dlss_retired_,[&](const auto& retired){const bool done=numbered>=retired.second+600;if(done)streamline::FreeMenuDlss(retired.first);return done;});
    // A parked viewport whose context is gone or remade can never be adopted again: freed at once (the sixth step review's L1).
    const auto live=[&](const ParkedDlss& parked){for(const auto& [extent,context]:offscreen_sets_){(void)extent;
        if(context.target_generation==parked.made[0] && context.taa_made==parked.made[1])return true;}return false;};
    std::erase_if(menu_dlss_parked_,[&](const auto& parked){const bool done=numbered>=parked.since+36000 || !live(parked);if(done)streamline::FreeMenuDlss(parked.viewport);return done;});
    // The cap after: only adoptable viewports count (the seventh step review's L1).
    while(menu_dlss_parked_.size()>4){streamline::FreeMenuDlss(menu_dlss_parked_.front().viewport);menu_dlss_parked_.erase(menu_dlss_parked_.begin());}
}
TaaStage* SceneRenderer::MenuTaaFor(ID3D12Resource* target,uint64_t serial){
    if(menu_taa_failed_)return nullptr;
    auto& entry=menu_taa_[target];
    if(!entry.stage)entry.stage=std::make_unique<TaaStage>();
    if(entry.stage->IsOpen())return entry.stage.get();
    std::string reason;if(entry.stage->Open(gpu_.device.Get(),reason))return entry.stage.get();
    menu_taa_.erase(target);LatchMenuTaaOff(serial,"open: "+reason);return nullptr;
}
// I-0140 tranche 40 (D-0305): a menu view's Streamline viewport for its DLAA (MenuTaa's account, scene_pass.h).
uint32_t SceneRenderer::MenuDlssViewport(MenuTaa& entry,const std::array<ID3D12Resource*,4>& inputs,const std::array<uint64_t,2>& made,uint64_t serial){
    // A new viewport's first evaluation resets (the view's code).
    if(entry.dlss_viewport && (entry.dlss_inputs!=inputs || entry.dlss_made!=made)){menu_dlss_retired_.emplace_back(entry.dlss_viewport,frames_.Numbered());entry.dlss_viewport=0;}
    if(entry.dlss_viewport)return entry.dlss_viewport;
    // The viewport of an entry on the same inputs: parked, or held by an entry that resolved neither this frame nor the last (a menu
    // opened again within 600 frames; that entry, if drawn again, takes another and resets). Its feature kept (the final review's M1).
    const auto numbered=frames_.Numbered();
    const auto adopt=[&](uint32_t id){entry.dlss_viewport=id;entry.dlss_inputs=inputs;entry.dlss_made=made;return id;};
    for(auto parked=menu_dlss_parked_.begin();parked!=menu_dlss_parked_.end();++parked)
        if(parked->inputs==inputs && parked->made==made){const auto id=parked->viewport;menu_dlss_parked_.erase(parked);return adopt(id);}
    for(auto& [target,other]:menu_taa_){(void)target;
        if(&other!=&entry && other.dlss_viewport && other.dlss_inputs==inputs && other.dlss_made==made && other.last_frame+1<numbered){
            const auto id=other.dlss_viewport;other.dlss_viewport=other.dlss_evaluated=0;return adopt(id);}}
    const auto taken=[&](uint32_t id){for(const auto& [target,other]:menu_taa_){(void)target;if(other.dlss_viewport==id)return true;}
        for(const auto& retired:menu_dlss_retired_)if(retired.first==id)return true;
        for(const auto& parked:menu_dlss_parked_)if(parked.viewport==id)return true;return false;};
    for(uint32_t id=2;id<66;++id)if(!taken(id))return adopt(id);
    // None free: the oldest parked one's feature freed and its number taken (idle 600 frames and more: no list of it in flight).
    if(!menu_dlss_parked_.empty()){const auto id=menu_dlss_parked_.front().viewport;streamline::FreeMenuDlss(id);menu_dlss_parked_.erase(menu_dlss_parked_.begin());return adopt(id);}
    if(!menu_dlss_full_told_.exchange(true) && upscaler_sink_){char row[160]{};   // the DLAA review's L4
        sprintf_s(row,"{\"event\":\"native_menu_dlss_no_viewport\",\"serial\":%llu,\"retired\":%zu}%c",static_cast<unsigned long long>(serial),menu_dlss_retired_.size(),10);upscaler_sink_(row);}
    return 0;
}
void SceneRenderer::LatchMenuDlssOff(uint64_t serial,const std::string& reason){
    if(menu_dlss_failed_.exchange(true) || !upscaler_sink_)return;
    std::string why=reason.substr(0,200);for(auto& c:why)if(c=='"' || c=='\\' || c<32)c=' ';char row[320]{};
    sprintf_s(row,"{\"event\":\"native_menu_dlss_latched_off\",\"serial\":%llu,\"after_frames\":%llu,\"reason\":\"%s\"}\n",static_cast<unsigned long long>(serial),
        static_cast<unsigned long long>(menu_dlss_frames_.load(std::memory_order_relaxed)),why.c_str());upscaler_sink_(row);
}
// W10 TM-2: the tone-map stage, opened at the presentation extent from the first frame that carries the tone-map input, into
// the 8-bit colour target (as the glow); a failure is kept for the session. I-0138 tranche 1: our pass (PostToneStage); tranche 8:
// the original's programmes (TonemapStage) are gone from the module.
bool SceneRenderer::TonemapReady(const NativeTonemapInput& input) {
    if(tonemap_failed_)return false;
    if(post_tone_.IsOpen()){
        if(post_tone_.Width()!=presenter_.width || post_tone_.Height()!=presenter_.height){tonemap_failure_="post tone stage extent differs from the presentation";tonemap_failed_=true;return false;}
        return true;}
    std::string reason;
    if(post_tone_.Open(gpu_.device.Get(),input.dither,DisplayStage::Format(),presenter_.width,presenter_.height,reason))return true;
    tonemap_failure_=reason;tonemap_failed_=true;return false;
}
// W10 GR: the colour grading at the presentation extent. I-0138 tranche 1: our pass's LUT side (after TonemapReady opened the pass);
// tranche 8: its strip copy our own compute (PostToneStage::OpenLut), the original's programmes (GradingStage) gone from the module.
bool SceneRenderer::GradingReady(const NativeGradingInput& input) {
    if(grading_failed_ || !input.lut || !post_tone_.IsOpen())return false;
    if(post_tone_.HasLut())return true;
    std::string reason;
    if(post_tone_.OpenLut(reason))return true;
    grading_failure_=reason;grading_failed_=true;return false;
}
bool SceneRenderer::MaskReady() {   // I-0126 R7c (D-0251): the mask stage alone, no programmes
    if(mask_failed_)return false;
    if(mask_.IsOpen()){
        if(mask_.Width()!=RenderWidth() || mask_.Height()!=RenderHeight()){mask_failure_="mask stage extent differs from the render extent";mask_failed_=true;return false;}
        return true;}
    std::string reason;
    if(mask_.Open(gpu_.device.Get(),RenderWidth(),RenderHeight(),reason)){mask_target_bytes_=mask_.TargetBytes();return true;}
    mask_failure_=reason;mask_failed_=true;return false;
}
// I-0113 tranche 17 (STUDY-PASS-31, static; the live target and depth 640x360 at 1280x720, reference_effect_depth): pass 0x31
// draws into a half-size layer (140085540's ScaleRender group: (w>>1, h>>1), colour code 10, R16G16B16A16_FLOAT INFERRED) whose
// depth is the half depth 1401CB9D0 makes at sub 0x28 from the depth copy t3 (the minimum of 2x2 texels, DepthScale2x2OutputDepth,
// SSAO-WAVE1-READY: here transcribed as loads; for an odd size the last column or row is not read), bound read-only and
// sampled at t3 while 0x31 draws (sub 0x2F clears the layer to (0, 0, 0, 1)); sub 0x32 (140102A80) composites it into the scene,
// colour ONE / SRC_ALPHA, alpha ZERO / ONE (bank +0x1D0), no depth, no cull, with one of two programmes (STUDY-T17-LAYER-ALPHA §5b):
// the plain SampleColor1 (one linear-clamp sample of the layer) or, when the block's byte 0 is set (140102790: settings+0x415C
// or the Effects quality row's « Force HQ UpScale », +0x5360's +0x4B: Effects MAX only; live 1 at Effects MAX, i113-t17-b-original),
// the bilateral SampleBilateralDepthBlur (gxposteffect), transcribed from its DXBC below: four linear taps of the layer one full
// pixel off, each weighted by its half-depth texel's agreement with the full depth (1 / (|z_half / z_full - 1| + 0.01)) and the
// inverse distance of the pixel's fraction, cb1 rows 0..5 as 140102A80 writes them. Explicit policy: settings+0x415C (the
// Graphics.IsEnableBilateralUpscale setting, default 0 INFERRED) is not read; the Effects row's byte decides.
bool SceneRenderer::EffectLayerReady() {
    if(!effect_layer_failure_.empty())return false;
    const uint32_t width=std::max(1u,RenderWidth()>>1),height=std::max(1u,RenderHeight()>>1);
    auto& l=effect_layer_;
    if(l.color && l.width==width && l.height==height)return true;
    try{auto* device=gpu_.device.Get();
        if(!l.root){
            // t0..t2 from the table (the downsample: the depth copy; the composite: the layer's colour, the main depth, the half
            // depth), b0 the bilateral's cb1 rows 0..5 as root constants, s0 point clamp and s1 linear clamp (banks +0x2E0, +0x2E8,
            // reference_layer_composite: filters 0 and 21; the programmes' SS_ClampPoint s2 and SS_ClampLinear s3).
            D3D12_DESCRIPTOR_RANGE range{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,3,0,0,0};D3D12_ROOT_PARAMETER parameters[2]{};
            parameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[0].DescriptorTable={1,&range};parameters[0].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
            parameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;parameters[1].Constants={0,0,24};parameters[1].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
            D3D12_STATIC_SAMPLER_DESC samplers[2]{};
            for(UINT s=0;s<2;++s){auto& sampler=samplers[s];sampler.Filter=s?D3D12_FILTER_MIN_MAG_MIP_LINEAR:D3D12_FILTER_MIN_MAG_MIP_POINT;sampler.ShaderRegister=s;
                sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;sampler.MaxAnisotropy=1;sampler.ComparisonFunc=D3D12_COMPARISON_FUNC_NEVER;
                sampler.MaxLOD=D3D12_FLOAT32_MAX;sampler.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;}
            const D3D12_ROOT_SIGNATURE_DESC desc{2,parameters,2,samplers,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> blob,error;
            Check(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"effect layer root serialization");
            Check(Pipelines().Root(device,blob->GetBufferPointer(),blob->GetBufferSize(),l.root.ReleaseAndGetAddressOf()),"effect layer root");
            D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};pso.pRootSignature=l.root.Get();pso.VS={effect_layer_vs_dxbc,sizeof(effect_layer_vs_dxbc)};
            pso.SampleMask=UINT_MAX;pso.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;pso.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;
            pso.RasterizerState.DepthClipEnable=TRUE;pso.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;pso.SampleDesc.Count=1;
            pso.BlendState.RenderTarget[0].RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;
            // The downsample: depth only, every texel written (ALWAYS with writes; the original clears to 1.0 and tests LESS_EQUAL).
            pso.PS={effect_layer_downsample_dxbc,sizeof(effect_layer_downsample_dxbc)};pso.NumRenderTargets=0;pso.DSVFormat=ModelDepthFormat();
            pso.DepthStencilState.DepthEnable=TRUE;pso.DepthStencilState.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ALL;pso.DepthStencilState.DepthFunc=D3D12_COMPARISON_FUNC_ALWAYS;
            Check(Pipelines().Graphics(device,pso,l.downsample.ReleaseAndGetAddressOf()),"effect layer downsample pipeline");
            // The composite (bank +0x1D0): colour ONE / SRC_ALPHA, alpha ZERO / ONE, into the lit HDR, no depth.
            pso.PS={effect_layer_composite_dxbc,sizeof(effect_layer_composite_dxbc)};pso.NumRenderTargets=1;pso.RTVFormats[0]=ForwardTargetFormat();pso.DSVFormat=DXGI_FORMAT_UNKNOWN;
            pso.DepthStencilState=D3D12_DEPTH_STENCIL_DESC{};
            auto& blend=pso.BlendState.RenderTarget[0];blend.BlendEnable=TRUE;blend.SrcBlend=D3D12_BLEND_ONE;blend.DestBlend=D3D12_BLEND_SRC_ALPHA;blend.BlendOp=D3D12_BLEND_OP_ADD;
            blend.SrcBlendAlpha=D3D12_BLEND_ZERO;blend.DestBlendAlpha=D3D12_BLEND_ONE;blend.BlendOpAlpha=D3D12_BLEND_OP_ADD;blend.LogicOp=D3D12_LOGIC_OP_NOOP;
            Check(Pipelines().Graphics(device,pso,l.composite.ReleaseAndGetAddressOf()),"effect layer composite pipeline");
            pso.PS={effect_layer_bilateral_dxbc,sizeof(effect_layer_bilateral_dxbc)};
            Check(Pipelines().Graphics(device,pso,l.bilateral.ReleaseAndGetAddressOf()),"effect layer bilateral pipeline");
            D3D12_DESCRIPTOR_HEAP_DESC heap{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,1,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
            Check(CountedHeap(device)->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&l.rtv)),"effect layer RTV heap");
            heap={D3D12_DESCRIPTOR_HEAP_TYPE_DSV,2,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
            Check(CountedHeap(device)->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&l.dsv)),"effect layer DSV heap");}   // the SRV table: written by the frame
        // A size change: the former targets held until the recordings that use them complete.
        if(l.color)RetireAfterCurrentRecording(HoldObject(l.color.Get()));if(l.depth)RetireAfterCurrentRecording(HoldObject(l.depth.Get()));
        l.color.Reset();l.depth.Reset();l.viewed_copy=nullptr;
        const auto heap=Heap(D3D12_HEAP_TYPE_DEFAULT);
        auto image=Image(width,height,1,ForwardTargetFormat(),D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);D3D12_CLEAR_VALUE clear{ForwardTargetFormat(),{0,0,0,1}};
        Check(CountedResource(device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&image,D3D12_RESOURCE_STATE_RENDER_TARGET,&clear,IID_PPV_ARGS(&l.color)),"effect layer colour");
        image=Image(width,height,1,DXGI_FORMAT_R32G8X24_TYPELESS,D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);D3D12_CLEAR_VALUE depth_clear{ModelDepthFormat(),{}};depth_clear.DepthStencil={1.0f,0};
        Check(CountedResource(device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&image,D3D12_RESOURCE_STATE_DEPTH_WRITE,&depth_clear,IID_PPV_ARGS(&l.depth)),"effect layer depth");
        device->CreateRenderTargetView(l.color.Get(),nullptr,l.rtv->GetCPUDescriptorHandleForHeapStart());
        const UINT dsv_stride=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        D3D12_DEPTH_STENCIL_VIEW_DESC dsv{ModelDepthFormat(),D3D12_DSV_DIMENSION_TEXTURE2D,D3D12_DSV_FLAG_NONE};
        auto at=l.dsv->GetCPUDescriptorHandleForHeapStart();device->CreateDepthStencilView(l.depth.Get(),&dsv,at);
        dsv.Flags=D3D12_DSV_FLAG_READ_ONLY_DEPTH|D3D12_DSV_FLAG_READ_ONLY_STENCIL;at.ptr+=dsv_stride;device->CreateDepthStencilView(l.depth.Get(),&dsv,at);   // 1400CCCF0's flags 3
        l.width=width;l.height=height;++l.generation;return true;
    }catch(const std::exception& e){effect_layer_failure_=e.what();if(l.table)RetireAfterCurrentRecording(HoldObject(l.table.Get()));l=EffectLayer{};return false;}   // step review L1: the table may be in flight
}
// R13 cR2: one frame job in the frame's single list: the movie, the menu views in order, then the world frame or the UI
// (startup) frame. Inline on the owner thread until cR3.
FrameReceipt SceneRenderer::ExecuteFrame(const NativeFrameJob& job,std::shared_ptr<const NativeModelFrameInput> prepared) {
    RequireThread(true,"native frame job off the render thread");
    // I-0078: the job's UI packet built here, on the render thread, before the frame that draws it (as before on the engine thread).
    if(job.ui && job.ui_packet)throw std::invalid_argument("native frame job UI twice");
    const auto ui=job.ui_packet?AdmitPrimitiveWork(std::move(*job.ui_packet)):job.ui;
    if(job.display_curve && DisplayReady())display_.SetCurve(job.display_curve);   // W3: the frame's gamma curve
    using Stage=NativeFrameStageError::Stage;
    if(job.movie){try{if(!job.movie_target)throw std::invalid_argument("native frame job movie target");
            RenderNativeMovie(*job.movie,job.movie_target.Get(),job.movie_pixel_file);}
        catch(const std::exception& e){throw NativeFrameStageError(Stage::Movie,e.what());}
        catch(...){throw NativeFrameStageError(Stage::Movie,"native movie frame: unknown exception");}}
    for(const auto& view:job.views){try{const auto& frame=view.frame;
            if(!frame || !view.target)throw std::invalid_argument("native frame job view");
            RenderNativeOffscreen(*frame,view.target.Get(),view.background,view.pixel_file,view.tone);}
        catch(const std::exception& e){throw NativeFrameStageError(Stage::View,e.what());}
        catch(...){throw NativeFrameStageError(Stage::View,"native offscreen view: unknown exception");}}
    if(job.models && prepared)throw std::invalid_argument("native frame job prepared twice");
    const auto& models=job.models?job.models:prepared;   // cR5: inline or worker preparation
    if(models){
        // The world frame's own UI fields must agree with the job's (one source of truth: the job).
        if(models->serial!=job.serial || models->primitive_generation!=(ui?job.ui_generation:0) ||
           models->initialize_primitive_stencil!=(ui && job.initialize_ui_stencil))throw std::invalid_argument("native frame job world frame");
        // R13 cR3: the job's own velocity picture, requested on the recording thread so this frame takes it.
        if(!job.velocity_file.empty())RequestVelocityPicture(job.velocity_file);
        light_grid_frame_=job.light_grid;   // I-0107: the grid this frame's organize writes (the job's, made off the render thread)
        rt_view_file_=job.rt_view_file;   // I-0117 T4: the control view's pictures, taken by this frame's pass
        pixel_only_=job.pixel_only;   // I-0128 R8d: a burst's picture, alone
        return RenderModels(*models,ui.get(),ui,job.scene_copy,job.pixel_file,models);}
    if(ui){const auto* packet=ui->Packet();if(!packet || packet->generation!=job.ui_generation)throw std::invalid_argument("native frame job UI generation");
        return RenderStartupFrame(job.serial,ui.get(),job.initialize_ui_stencil,ui);}
    return RenderStartupFrame(job.serial,nullptr,false);
}
FrameReceipt SceneRenderer::RenderModels(const NativeModelFrameInput& input,const PrimitivePass* ui,std::shared_ptr<const void> ui_owner,
    const ComPtr<ID3D12Resource>& scene_copy,const std::wstring& pixel_file,std::shared_ptr<const void> input_owner) {
    SceneView view;
    if(!open_ || faulted_ || input.serial<=std::max(startup_serial_,last_serial_) ||
       input.camera.width!=RenderWidth() || input.camera.height!=RenderHeight() ||   // I-0111: the world at the render extent
       !BuildSceneView(input.camera,view) || input.draws.size()>131072 ||
       // I-0141 tranche 20: no G-buffer draw only for a lit frame of forward draws (the G-buffer pass then records nothing)
       (input.draws.empty() && (input.forward_draws.empty() || !(input.lighting && input.lighting->programs))))
        throw std::invalid_argument("current native model frame");
    // W0a: one envelope check per list (EnvelopeMatches); Forward2 and DepthOnly wait for their passes' graph declarations.
    for(const auto& draw:input.draws)if(!draw || !EnvelopeMatches(draw->Input(),ModelPassEnvelope::GBuffer6,color_formats,ModelDepthFormat()))
        throw std::invalid_argument("native model output formats");
    if(input.forward_draws.size()>131072 || (!input.forward_draws.empty() && !(input.lighting && input.lighting->programs)))
        throw std::invalid_argument("native forward pass requires lighting");
    bool forward_gb5=false;   // W6 GL-6: a Forward2 draw writes GB5 as its second target
    for(const auto& draw:input.forward_draws){
        const bool second=draw && EnvelopeMatches(draw->Input(),ModelPassEnvelope::Forward2,forward2_formats,ModelDepthFormat());
        if(!draw || (!second && !EnvelopeMatches(draw->Input(),ModelPassEnvelope::Forward1,forward_formats,ModelDepthFormat())))
            throw std::invalid_argument("native forward output formats");
        forward_gb5|=second;}
    if(input.velocity_draws.size()>131072 || (!input.velocity_draws.empty() && !input.velocity))
        throw std::invalid_argument("native velocity rows require velocity inputs");
    for(const auto& draw:input.velocity_draws)if(!draw || !EnvelopeMatches(draw->Input(),ModelPassEnvelope::Velocity1,velocity_formats,ModelDepthFormat()))
        throw std::invalid_argument("native velocity output formats");
    for(const auto& job:input.paints)for(const auto& draw:job.draws)if(!draw || !job.target || !EnvelopeMatches(draw->Input(),ModelPassEnvelope::Paint2,paint_formats,DXGI_FORMAT_D32_FLOAT))
        throw std::invalid_argument("native paint output formats");   // D2
    for(const auto& job:input.paints)for(const auto& draw:job.depth_draws)if(!draw || !EnvelopeMatches(draw->Input(),ModelPassEnvelope::DepthOnly,{},DXGI_FORMAT_D16_UNORM))
        throw std::invalid_argument("native paint occlusion formats");   // D-0193 rung 2b
    for(const auto* list:{&input.water_depth_draws,&input.water_draws})for(const auto& draw:*list){   // WT-3/WT-4: the forward envelopes
        const bool second=draw && EnvelopeMatches(draw->Input(),ModelPassEnvelope::Forward2,forward2_formats,ModelDepthFormat());
        if(!draw || (!second && !EnvelopeMatches(draw->Input(),ModelPassEnvelope::Forward1,forward_formats,ModelDepthFormat())))
            throw std::invalid_argument("native water output formats");
        forward_gb5|=second;}
    const auto* primitive=ui?ui->Packet():nullptr;
    if(ui && !primitive)throw std::invalid_argument("native model UI build");
    if(primitive){
        if(input.primitive_generation!=primitive->generation || primitive->serial!=input.serial ||
           primitive->width!=presenter_.width || primitive->height!=presenter_.height)
            throw std::invalid_argument("native model UI overlay mismatch");   // I-0137 tranche 7: renamed, so as not to be taken for the absent overlay (02_frame_lifecycle.inc's own case)
        try{PrimitiveDepthTarget(presenter_.width,presenter_.height);}catch(...){faulted_=true;throw;}   // cR2: the frame owns the UI depth (X1, D-12)
        if(!input.initialize_primitive_stencil && !(primitive_stencil_initialized_ && primitive_stencil_generation_==primitive->generation))
            throw std::invalid_argument("current native model UI stencil");
    }else if(input.primitive_generation || input.initialize_primitive_stencil)
        throw std::invalid_argument("native model UI overlay unavailable");
    LARGE_INTEGER phase_frequency{},phase_start{},phase_closed{},phase_completed{},phase_end{};QueryPerformanceFrequency(&phase_frequency);QueryPerformanceCounter(&phase_start);
    try {
        NativeModelTargets();
        if(input.clear_stencil!=view_->clear_stencil){
            if(view_->target_generation==UINT64_MAX)throw std::overflow_error("native target generation exhausted");
            gpu_.Drain(WaitDepthRecreate);gpu_audit.depth_recreations.fetch_add(1,std::memory_order_relaxed);DepthTarget(RenderWidth(),RenderHeight(),input.clear_stencil);++view_->target_generation;
        }
        const bool lit=input.lighting && input.lighting->programs;
        if(lit)NativeLightingResources(*input.lighting);
        if(input.velocity)NativeVelocityResources();
        // I-0111: DLSS this frame on the lit HDR (at the render extent: equal to the presentation's for NVIDIA's DLAA, smaller in a
        // scaled mode), with the velocity's vectors; its output at the presentation's extent; a failed evaluation latches it off for
        // the session.
        const bool dlss_frame=lit && input.velocity && input.dlss && streamline::DlssReady() && !dlss_failed_ && [&]{
            try{NativeDlssOutput();}catch(const std::exception& e){LatchDlssOff(input.serial,std::string("output: ")+e.what());dlss_output_.Reset();}   // step review L4, final review M1
            return bool(dlss_output_);}();
        // I-0132 R11j: the TAA where DLSS does not run (the frame's decision, NativeSelectedScene::taa), on the same inputs at the
        // presentation's extent (no upscaling), into DLSS's output target (the post chain's source; both never run on one frame).
        const bool taa_frame=!dlss_frame && lit && input.velocity && input.taa && RenderWidth()==presenter_.width && RenderHeight()==presenter_.height &&
            TaaReady(input.serial) && [&]{
                try{NativeDlssOutput();}catch(const std::exception& e){LatchTaaOff(input.serial,std::string("output: ")+e.what());dlss_output_.Reset();}
                if(!dlss_output_)return false;
                // The step review's H1 and M1: the inputs and the history checked and made before any barrier, a failure a latch.
                std::string reason;if(taa_.Prepare(view_->light_targets[3].Get(),view_->depth.Get(),motion_vectors_.Get(),dlss_output_.Get(),reason))return true;
                LatchTaaOff(input.serial,"prepare: "+reason);return false;}();
        // I-0113 tranche 39 (STUDY-REACTIVE-MASK): the TAA's reactive base on a TAA frame (never on DLSS's or RR's: no other path); a
        // refused creation leaves the TAA without it for the session.
        const bool taa_base_ready=taa_frame && input.taa_reactive && !taa_base_failed_ && [&]{
            try{NativeTaaBase();return true;}
            catch(const std::exception& e){taa_base_failed_=true;taa_base_.Reset();
                if(upscaler_sink_){std::string what=e.what();what=what.substr(0,160);for(auto& c:what)if(c=='"' || c=='\\' || c<32)c='\'';
                    char row[256]{};sprintf_s(row,"{\"event\":\"native_taa_reactive_off\",\"serial\":%llu,\"reason\":\"%s\"}\n",static_cast<unsigned long long>(input.serial),what.c_str());upscaler_sink_(row);}
                return false;}}();
        if(!taa_base_ready){taa_base_.Reset();taa_fade_mask_.Reset();}   // its step review's L4: not kept past the TAA (a slot that used it holds it); I-0140's mask alike (its final review's L6)
        // Step review M1/M2: a jittered frame DLSS (or the TAA) does not resolve (no velocity, no lighting yet, a latch) is counted and named.
        if(!dlss_frame && !taa_frame && (input.camera.jitter_x!=0 || input.camera.jitter_y!=0)){const auto n=dlss_unresolved_.fetch_add(1,std::memory_order_relaxed)+1;
            if(upscaler_sink_ && (n==1 || n==3 || n%600==0)){char row[200]{};sprintf_s(row,"{\"event\":\"native_dlss_unresolved\",\"frames\":%llu,\"serial\":%llu,\"lit\":%s,\"velocity\":%s}\n",
                static_cast<unsigned long long>(n),static_cast<unsigned long long>(input.serial),lit?"true":"false",input.velocity?"true":"false");upscaler_sink_(row);}}
        NoteTargetBytes();   // E0: the target sets' allocation bytes, when a generation changed
        // cD1: the slot holds the draws and the lighting input (its environment cubes) until its ticket.
        // cD3: the frame continues the list the movie and the menu views opened (or opens it) and submits it once.
        auto& slot=FrameListSlot(WaitMainFrame);SweepMenuAa(nullptr);   // I-0140 tranche 40 (the final review's L2)
        rr_now_=dlss_frame && RrFrame(input,slot);   // I-0123 RR-a: its targets held by this slot
        // I-0132 R12f (the step review's M1): the rays' level and bounces taken once for the frame (the engine thread changes them live).
        rt_pt_now_=rt_pt.load(std::memory_order_relaxed);rt_bounces_now_=rt_bounces.load(std::memory_order_relaxed);rt_gi_paths_now_=rt_gi_paths.load(std::memory_order_relaxed);
        const bool input_held=bool(input_owner);
        if(input_owner)slot.kept.push_back(std::move(input_owner));   // I-0074: the input, its draws, paint targets and lighting
        else{slot.draws.insert(slot.draws.end(),input.draws.begin(),input.draws.end());slot.draws.insert(slot.draws.end(),input.forward_draws.begin(),input.forward_draws.end());
            slot.draws.insert(slot.draws.end(),input.velocity_draws.begin(),input.velocity_draws.end());
            slot.draws.insert(slot.draws.end(),input.decal_draws.begin(),input.decal_draws.end());   // DECALS-WAVE1 D1
            for(const auto& job:input.paints){slot.draws.insert(slot.draws.end(),job.draws.begin(),job.draws.end());slot.draws.insert(slot.draws.end(),job.depth_draws.begin(),job.depth_draws.end());if(job.target)slot.kept.push_back(job.target);}   // D2
            slot.draws.insert(slot.draws.end(),input.water_depth_draws.begin(),input.water_depth_draws.end());slot.draws.insert(slot.draws.end(),input.water_draws.begin(),input.water_draws.end());}   // WT-3/WT-4
        for(const auto& [edge,depth]:paint_depths_){(void)edge;if(depth.resource)slot.kept.push_back(HoldResource(depth.resource.Get()));}
        for(const auto& copy:{input.decal_gb0_copy,input.decal_gb1_copy,input.decal_depth_copy})if(copy)slot.kept.push_back(HoldResource(copy.Get()));
        if(input.lighting && !input_held)slot.kept.push_back(input.lighting);   // I-0074 review L5: held through the input otherwise
        if(input.rt_scene && !input_held)slot.kept.push_back(input.rt_scene);   // I-0117 (R2 T1): the structures its instances trace
        if(primitive){slot.kept.push_back(HoldResource(primitive_depth_.Get()));if(ui_owner)slot.kept.push_back(ui_owner);}   // D-12; cR2: the build
        auto* list=slot.list.Get();
        // I-0128 R8a (rt-gpu-measure.flag): the main view's frame timed on the GPU, from here to its list's close, and its traced passes.
        rt_gpu_slot_=frames_.CurrentIndex();
        rt_gpu_now_=view_==&main_view_ && rt_gpu_measure.load(std::memory_order_relaxed) && rt_gpu_.Ready(gpu_.device.Get(),gpu_.queue.Get());
        if(rt_gpu_now_){rt_gpu_.Start(rt_gpu_slot_);GpuZoneBegin(list,GpuZone::Frame);}
        std::vector<ID3D12CommandList*> submitted_lists;   // I-0074 point 3c: the lists closed before `list`, in submission order
        const D3D12_VIEWPORT viewport{0,0,static_cast<float>(RenderWidth()),static_cast<float>(RenderHeight()),0,1};   // I-0111: the world's
        const D3D12_VIEWPORT out_viewport{0,0,static_cast<float>(presenter_.width),static_cast<float>(presenter_.height),0,1};   // the post chain's, UI's and display's
        const bool scaled=RenderWidth()!=presenter_.width || RenderHeight()!=presenter_.height;   // a DLSS scaled mode
        const bool display=DisplayReady();   // W3: the frame into the gamma pass's intermediate
        const bool hdr_out=HdrOutputReady(display);   // I-0138 tranche 5: an HDR10 chain's last pass is the output transform
        const auto overlay_frame=OverlaySnapshot();   // I-0132 R12a (the step review's H1): one snapshot for the key, the pass and the draw
        // I-0131 R10b (R10-PLAN §3): a frame DLSS-G interpolates from: a DLSS frame with the gamma pass (its HUD-less colour is that
        // pass drawn before the UI) and frame generation ready; in the plan key (bit 56: the HUD-less pass).
        const bool fg_frame=dlss_frame && display && FrameGeneration() && streamline::FgReady();   // I-0132 R12f: the player's too
        ID3D12Resource* const fg_hudless=fg_frame?display_.Hudless(presenter_.format):nullptr;   // the step review's L3: one target for the declaration and the pass
        // I-0111 (step review M1): a frame at a scaled render extent that DLSS does not resolve (a latch, no velocity or lighting yet)
        // shows no world this frame: its colour is smaller than the presentation (the prepare refuses such frames once DLSS is off).
        const bool world_shown=!scaled || dlss_frame;
        const bool glow=world_shown && lit && input.glow && input.glow->programs && GlowReady(*input.glow);   // W6 GL-7: the ten passes this frame
        if(lit && input.mask && input.mask->ready)(void)MaskReady();   // I-0126 R7c: the mask stage's target
        // I-0111: the post chain's source, the lit HDR or, on a DLSS frame, DLSS's output (the presentation's extent); I-0132 R11j: the
        // TAA's output there too.
        auto* const post_source=dlss_frame || taa_frame?dlss_output_.Get():lit?view_->light_targets[3].Get():nullptr;
        const auto post_desc=post_source?post_source->GetDesc():D3D12_RESOURCE_DESC{};
        // W10 GL-3/LS-1: whether the glare plan runs this frame (its extent the post chain's source's, the stage open), and with it Alpha_R (the plan's
        // pre draws, with the tone map that takes the glare): decided before the frame graph's plan key, which carries alpha_r.
        const auto glare_matches=[&]{if(!lit || !input.tonemap || !input.yebis_glare || !input.yebis_glare->programs || glare_family_failed_)return false;
            return post_desc.Width==input.yebis_glare->plan.width && post_desc.Height==input.yebis_glare->plan.height;};
        const auto open_glare=[&]{if(glare_family_.IsOpen())return true;std::string reason;
            if(glare_family_.Open(gpu_.device.Get(),reason)){return true;}
            glare_family_failed_=true;return false;};
        // I-0111 (UPSCALER-DLSS-READY §3.7): in a scaled mode the stages that read the depth (Alpha_R, DoF, the motion blur) read the
        // depth at the presentation's extent drawn after DLSS (made with DLSS's output); off without it.
        const bool depth_out_frame=dlss_frame && scaled && [&]{
            try{NativeDepthOut();}catch(const std::exception& e){LatchDlssOff(input.serial,std::string("output depth: ")+e.what());depth_out_.Reset();}
            return bool(depth_out_);}();
        ID3D12Resource* const post_depth=depth_out_frame?depth_out_.Get():view_->depth.Get();
        // The generation the post stages key their views on: a new DLSS output or output-size depth (just made above), or the view's targets.
        const uint64_t post_generation=dlss_frame || taa_frame?(view_->target_generation^(dlss_output_generation_<<32)^(uint64_t{1}<<63)):view_->target_generation;
        const bool alpha_r=(!scaled || depth_out_frame) && glare_matches() && !input.yebis_glare->plan.pre.empty() && TonemapReady(*input.tonemap) && open_glare();
        // W10 DF-4: the depth of field (with the glare that reads its output), decided before the plan key too.
        const auto open_dof=[&]{if(dof_stage_.IsOpen())return true;std::string reason;
            if(dof_stage_.Open(gpu_.device.Get(),reason)){return true;}
            dof_failure_=reason;dof_failed_=true;return false;};
        const bool dof=(!scaled || depth_out_frame) && glare_matches() && input.yebis_dof && input.yebis_dof->programs && !dof_failed_ && TonemapReady(*input.tonemap) &&
            [&]{const auto& desc=post_desc;const bool same=desc.Width==input.yebis_dof->plan.width && desc.Height==input.yebis_dof->plan.height;
                return same;}() && open_dof();
        // W10 MB-4: the motion blur (with the glare, after Alpha_R, from the velocity target's half-size reduction), decided before the plan key.
        const auto open_motionblur=[&]{if(motionblur_stage_.IsOpen())return true;std::string reason;
            if(motionblur_stage_.Open(gpu_.device.Get(),reason)){return true;}
            motionblur_failure_=reason;motionblur_failed_=true;return false;};
        const bool motionblur=alpha_r && glare_matches() && input.velocity && velocity_half_ && input.yebis_motionblur && input.yebis_motionblur->programs && !motionblur_failed_ &&
            TonemapReady(*input.tonemap) && [&]{const auto& desc=post_desc;const auto& m=*input.yebis_motionblur;
                const bool same=desc.Width==m.plan.width && desc.Height==m.plan.height && m.object_width==(presenter_.width>>1) && m.object_height==(presenter_.height>>1);
                return same;}() && open_motionblur();
        const bool depth_out_pass=depth_out_frame && (alpha_r || dof || motionblur);   // I-0111: drawn when a stage reads it
        // W10 TM-2: with the tone-map input, our tone-map pass replaces the sRGB composition. W10 GR (review): it and the colour
        // grading after it are decided before the plan key, which carries the grading (its pass is declared only with it).
        const bool tonemap=lit && input.tonemap && post_desc.Width==presenter_.width && post_desc.Height==presenter_.height &&
            TonemapReady(*input.tonemap);
        // I-0138 tranche 5 (step 2): on an HDR10 chain, this frame's scene in float and its UI on a layer (scene_pass.h). In the plan
        // key (bit 62).
        const bool hdr_scene=hdr_out && tonemap && HdrToneReady();
        const bool grading=tonemap && input.yebis_grading && GradingReady(*input.yebis_grading);
        // I-0132 R11h: the automatic exposure before the glare and the tone map (the plan key's bit 55; a failure latches it off from the next frame).
        const bool auto_exposure_frame=tonemap && input.tonemap->auto_exposure && !auto_exposure_failed_;
        // I-0138 tranche 5c (Thomas, 2026-11-14 11:42): the game's own anti-aliasing filter (1B9790, FXAA.vpo and DLAA.ppo on the 8-bit
        // colour) is removed; a frame neither DLSS nor the TAA resolves has no anti-aliasing.
        // I-0132 R12f (Thomas, 2026-11-11 03:53: « NIS » for the sharpening): NVIDIA Image Scaling's sharpening in place of our pass where the
        // TAA or DLSS resolves the frame, after the tone map on the 8-bit colour (before the glow and the UI), in the plan key (bit 19); our
        // own sharpening where NIS is not offered or latched off.
        const bool nis_frame=!hdr_scene && (dlss_frame || taa_frame) && tonemap && display && input.sharpen>=1e-3f && NisTargets();
        if(lit)OpenNativeLightOrganize(*input.lighting);   // I-0107: its root and pipeline (a refusal latches)
        const bool organize_frame=lit && NativeLightOrganizeReady(*input.lighting);   // the light grid built before the forward rows
        const bool mask_frame=lit && input.mask && input.mask->ready && mask_.IsOpen() && !mask_failed_ && native_mask_pso_ &&
            mask_.Width()==RenderWidth() && mask_.Height()==RenderHeight() && input.camera.width==RenderWidth() && input.camera.height==RenderHeight();
        // D-0180 C4: the sun shadow, in the mask stage (its passes in the plan key). I-0118 R5a (its final review L4): the traced sun
        // shadow has its own gate (the mask stage, the shadowed sun's pipeline, the frame's shadow blocks and sun). I-0131 R10a: it is
        // the only sun shadow; a frame it does not serve gets the mask cleared white (the sun unshadowed).
        rt_sun_now_=RtSunFrame(input,mask_frame && native_sun_shadowed_pso_ && input.shadow_blocks);   // step review L1: the blocks written
        const bool sun_shadow=rt_sun_now_;
        rt_lights_now_=RtLightsFrame(input,lit);   // I-0122 R4a: the native local lights
        rt_water_now_=RtWaterFrame(input,slot,lit);   // I-0125 R6c: the water's reflection traced (in the plan key), its target held by this slot
        rt_seed_serial_=input.serial;   // I-0126 R7d: the rays' seed every frame (R7c's NRD frames had RR's last serial: their noise frozen)
        rt_nrd_now_=lit && RtNrdFrame(input,view,slot);   // I-0126 R7c: the GI's NRD on a frame RR does not denoise (in the plan key)
        rt_gi_now_=lit && RtGiFrame(input);   // I-0126 R7a: the traced diffuse ambient (in the plan key)
        rt_nrd_now_=rt_nrd_now_ && rt_gi_now_;
        rt_restir_now_=rt_gi_now_ && !rt_nrd_now_ && RtRestirFrame(input,slot);   // I-0126 R7d: ReSTIR GI on a PT GI frame (in the plan key)
        // Its final review's L8: its buffers released after kRtRestirIdleFrames frames without it (RR or the PT level off; the frames that
        // used them hold them).
        if(rt_restir_now_)rt_restir_used_=input.serial;else if(rt_restir_used_!=UINT64_MAX && input.serial>rt_restir_used_+kRtRestirIdleFrames){rt_reflect_pass_.ReleaseRestir();rt_restir_used_=UINT64_MAX;}
        rt_gi_mean_now_=rt_gi_now_ && RtGiMeanFrame(slot);   // I-0126 R7d: the bias check (rt-gi-mean.flag; in the plan key: bit 53)
        rt_ser_now_=lit && RtSerFrame(slot);   // I-0132 R11b: the path programmes reordered (in the plan key: bit 58, its passes' reads, H1)
        // I-0132 R11d (D-0262): FC_ShadowColor (b8's row 29, +0x1D0, the only row GXLightAcc_DDirLightShadowed1 reads) at 1 on the traced
        // sun's frames: the mask's lerp(1 − FC_ShadowColor, 1, s) is then s, no sun let into the shadows (DS3's faked sky light: the GI's).
        if(sun_shadow){const auto range=frame_uploads_.Allocate(gpu_.device.Get(),1024);std::memcpy(range.cpu,input.shadow_scene.data(),960);
            const float full[3]{1.f,1.f,1.f};std::memcpy(range.cpu+16*29,full,sizeof full);sun_scene_block_=range.gpu;}
        const D3D12_RECT scissor{0,0,static_cast<LONG>(RenderWidth()),static_cast<LONG>(RenderHeight())};
        const D3D12_RECT out_scissor{0,0,static_cast<LONG>(presenter_.width),static_cast<LONG>(presenter_.height)};
        std::array<D3D12_CPU_DESCRIPTOR_HANDLE,6> targets;
        for(unsigned i=0;i<targets.size();++i)targets[i]=CPU(view_->rtvs.Get(),rtv_stride_,i);
        // Phase E E2 (graph-shadow.flag): this frame's resources with their between-frame states, then the graph's passes.
        std::wstring velocity_file;if(input.velocity){const std::lock_guard hold(channel_lock_);velocity_file.swap(velocity_request_);}
        const bool sample_velocity=input.velocity && ((witness_diagnostics && input.serial%30==0) || !velocity_file.empty());
        const bool sample_pixels=!pixel_file.empty() || (witness_diagnostics && input.serial%120==0);
        // Image regression (2026-10-16): with a presented-picture witness, the lit HDR too (R16G16B16A16, before the YEBIS passes),
        // next to it as world-N-hdr.rgba16f, to compare with the original's tone-map input (Reference reference-draw-00-t0.raw).
        const bool sample_hdr=lit && !pixel_file.empty() && !pixel_only_;   // I-0128 R8d: not for a burst's pictures (world-burst.flag)
        // FFX-DRAW (Native2027): the pass before which the scene copy is remade for the effects (0: none).
        const int32_t effect_copy_pass=lit && input.effects && !input.effects->draws.empty() && scene_copy && scene_copy->GetDesc().Width==RenderWidth() &&
            scene_copy->GetDesc().Height==RenderHeight()?EffectCopyPass(*input.effects):0;
        // I-0113 tranche 23: the frame's distortion quads copy their own rectangles (CopyRect statements; no-ffx-effect-copy.flag: none).
        const bool copy_rect_frame=lit && input.effects && input.effects->effect_copy && scene_copy && scene_copy->GetDesc().Width==RenderWidth() &&
            scene_copy->GetDesc().Height==RenderHeight() && std::any_of(input.effects->draws.begin(),input.effects->draws.end(),[](const NativeEffectDraw& d){return d.op==NativeEffectOp::CopyRect;});
        // DECALS-WAVE1 D1: the decals this frame (their copies at the G-buffer size; in the plan key).
        const auto gbuffer=view_->colors[0]?view_->colors[0]->GetDesc():D3D12_RESOURCE_DESC{};   // CopyResource: the copies match the G-buffer
        const auto decal_fits=[&](const ComPtr<ID3D12Resource>& r){if(!r)return false;const auto d=r->GetDesc();return d.Width==gbuffer.Width && d.Height==gbuffer.Height;};
        const bool decal_frame=lit && !input.decal_draws.empty() && view_==&main_view_ && decal_fits(input.decal_gb0_copy) && decal_fits(input.decal_gb1_copy) && decal_fits(input.decal_depth_copy);
        // WATER-TESS-WAVE1-READY WT-2..WT-4: the water rows this frame (the readable depth at the G-buffer size, the frame copy at the
        // presentation size: t56 and t55; in the plan key).
        const bool water_frame=lit && view_==&main_view_ && (!input.water_depth_draws.empty() || !input.water_draws.empty()) && decal_fits(input.water_depth_copy) &&
            scene_copy && scene_copy->GetDesc().Width==RenderWidth() && scene_copy->GetDesc().Height==RenderHeight();
        // I-0105: without water, a forward row that samples the readable depth (t56: the feathered effect models, s03000) has it
        // copied from the depth at the start of the forward pass.
        const bool forward_depth_frame=lit && view_==&main_view_ && !water_frame && !input.forward_draws.empty() && decal_fits(input.water_depth_copy);
        const bool rt_view_frame=RtViewFrame(input);   // I-0117 T4: the control view (its pictures made here, before the plan key)
        const bool rt_view_pictures=rt_view_frame && !rt_view_file_.empty();   // and their readbacks (the plan key's too)
        // I-0113 tranche 15: the effects test the writable depth (the 0x29 rows between them write it; their own write mask zero but
        // pass 0x21's, I-0140),
        // and a statement that samples the depth reads a copy taken before the 0x29 rows (in the plan key). Explicit policy: the
        // original's t3 is [GXRS+0x4E10]+0x48, not read; a copy is what D3D12 allows beside a writable depth.
        // I-0113 tranche 17: pass 0x31 into its half-size layer, whose depth is downsampled from that copy (STUDY-PASS-31).
        const bool layer_frame=lit && view_==&main_view_ && input.effects && EffectDrawsLayer(*input.effects) && EffectLayerReady();
        const bool effect_depth_frame=lit && input.effects && !input.effects->draws.empty() && (EffectReadsDepth(*input.effects) || layer_frame);
        // I-0140: a frame with pass 0x21 draws takes the depth copy and the half depth after them (its passes in another order when it
        // has that depth work: in the plan key, bit 61; its step reviews' H1 and L-b).
        const bool pass21_frame=lit && input.effects && std::any_of(input.effects->draws.begin(),input.effects->draws.end(),[](const NativeEffectDraw& d){return d.pass==0x21;});
        // I-0140 tranche 4 (its final review's M2): on such a frame with its rows in the original's order, pass 0x21's effects are drawn
        // among the 0x21 rows (the sub 0x21, by key), before the 0x22 rows, the water (subs 0x24, 0x27) and the TAA base, as the sort key
        // puts them; the forward block's rest then runs at the effects' first later pass (in the plan key, bit 62).
        const bool pass21_order=pass21_frame && input.forward_keys.size()==input.forward_draws.size() && input.forward_1f<=input.forward_21 &&
            input.forward_21<=std::min(input.forward_early,input.forward_draws.size());
        // I-0113 tranche 39: the base is copied where the forward block runs (its rows, effects or water); a frame without them has
        // nothing drawn over the lit HDR before the TAA, which then resolves without it. Its step review's L3: not on a frame whose rows
        // were left unsorted (native_forward_rows_unsorted: forward_early then counts the 0x1F rows only, the 0x21/0x22 rows would be marked).
        // Its final review's L3: only when something is drawn after it (a row past forward_early or an effect).
        const bool taa_base_copy=taa_base_ready && input.forward_keys.size()==input.forward_draws.size() &&
            (input.forward_early<input.forward_draws.size() || (input.effects && !input.effects->draws.empty()));
        bool taa_base_copied=false;
        // I-0140 (gap 23): the fade mask where the reactive base is copied (its step review's M1: not on a frame whose rows were left
        // unsorted), on a frame with a row whose model's fade moved (fade_mask_draws, 13b).
        const bool taa_fade_mask_pass=taa_base_copy && !taa_fade_mask_failed_ &&
            !input.fade_mask_draws.empty() && [&]{
            try{NativeTaaFadeMask();return true;}
            catch(const std::exception& e){taa_fade_mask_failed_=true;taa_fade_mask_.Reset();
                if(upscaler_sink_){std::string what=e.what();what=what.substr(0,160);for(auto& c:what)if(c=='"' || c=='\\' || c<32)c='\'';
                    char row[256]{};sprintf_s(row,"{\"event\":\"native_taa_fade_mask_off\",\"serial\":%llu,\"reason\":\"%s\"}\n",static_cast<unsigned long long>(input.serial),what.c_str());upscaler_sink_(row);}
                return false;}}();
        if(effect_depth_frame){const auto d=effect_depth_copy_?effect_depth_copy_->GetDesc():D3D12_RESOURCE_DESC{};
            if(d.Width!=RenderWidth() || d.Height!=RenderHeight()){
                if(effect_depth_copy_)slot.kept.push_back(HoldResource(effect_depth_copy_.Get()));   // a size change: the former copy held to its frames' end
                const auto heap=Heap(D3D12_HEAP_TYPE_DEFAULT);const auto image=Image(RenderWidth(),RenderHeight(),1,DXGI_FORMAT_R32G8X24_TYPELESS,D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
                ComPtr<ID3D12Resource> made;Check(CountedResource(gpu_.device.Get())->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&image,
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,nullptr,IID_PPV_ARGS(&made)),"native effect depth copy");
                effect_depth_copy_=made;}}
        GraphShadow* graph{};
        if(graph_shadow_enabled_){graph=&graph_shadow_;graph->Begin(input.serial,list);
            auto* back_buffer=presenter_.buffers[presenter_.swapchain->GetCurrentBackBufferIndex()].Get();
            const auto rt=D3D12_RESOURCE_STATE_RENDER_TARGET;DeclareViewGraph(*graph,lit);
            if(input.velocity){graph->Declare(velocity_target_.Get(),1,rt,FrameLifetime::Transient,"velocity");graph->Declare(motion_vectors_.Get(),1,rt,FrameLifetime::Transient,"motion");
                // I-0136 (its step review's H1): the Velo rows' own depth, both planes, DEPTH_WRITE between frames.
                if(input.velocity->own_depth)graph->Declare(velocity_depth_.Get(),2,D3D12_RESOURCE_STATE_DEPTH_WRITE,FrameLifetime::Transient,"velocity_depth");}
            if(motionblur)graph->Declare(velocity_half_.Get(),1,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,FrameLifetime::Transient,"velocity_half");   // W10 MB-4
            if(scene_copy)graph->Declare(scene_copy.Get(),1,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,FrameLifetime::Persistent,"scene_copy");
            if(decal_frame){graph->Declare(input.decal_gb0_copy.Get(),1,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,FrameLifetime::Persistent,"decal_gb0");   // DECALS-WAVE1 D1
                graph->Declare(input.decal_gb1_copy.Get(),1,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,FrameLifetime::Persistent,"decal_gb1");
                graph->Declare(input.decal_depth_copy.Get(),2,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,FrameLifetime::Persistent,"decal_depth");}
            if(water_frame || forward_depth_frame)graph->Declare(input.water_depth_copy.Get(),2,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,FrameLifetime::Persistent,"water_depth");   // WT-2
            if(effect_depth_frame)graph->Declare(effect_depth_copy_.Get(),2,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                FrameLifetime::Persistent,"effect_depth");   // I-0113 tranche 15
            if(layer_frame){graph->Declare(effect_layer_.color.Get(),1,rt,FrameLifetime::Persistent,"effect_layer");   // I-0113 tranche 17
                graph->Declare(effect_layer_.depth.Get(),2,D3D12_RESOURCE_STATE_DEPTH_WRITE,FrameLifetime::Persistent,"effect_layer_depth");}
            graph->Declare(back_buffer,1,D3D12_RESOURCE_STATE_PRESENT,FrameLifetime::External,"back_buffer");
            if(display){graph->Declare(display_.Intermediate(),1,rt,FrameLifetime::Persistent,"display_intermediate");   // W3: the new pass, shadowed first
                // The curve (three slices) stays shader-readable for the graph: its upload brings it back with raw barriers
                // before the pass (created COPY_DEST, first upload COPY_DEST to PSR), so the graph never transitions it.
                if(!hdr_out)graph->Declare(display_.Curve(),3,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,FrameLifetime::Persistent,"display_curve");   // I-0138 tranche 5 (step review L2): the brightness pass's alone
                if(fg_hudless)graph->Declare(fg_hudless,1,rt,FrameLifetime::Persistent,"fg_hudless");}   // I-0131 R10b
            if(hdr_out)graph->Declare(output_.Scene(),1,rt,FrameLifetime::Persistent,"hdr_scene");   // I-0138 tranche 5
            if(primitive)graph->Declare(primitive_depth_.Get(),2,D3D12_RESOURCE_STATE_DEPTH_WRITE,FrameLifetime::Persistent,"ui_depth");
            if(mask_frame)graph->Declare(mask_.Mask(),1,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,FrameLifetime::Persistent,"shadow_mask");   // D-0180: the mask
            if(nis_frame){graph->Declare(nis_in_.Get(),1,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,FrameLifetime::Persistent,"nis_in");   // I-0132 R12f
                graph->Declare(nis_out_.Get(),1,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,FrameLifetime::Persistent,"nis_out");}
            if(dlss_frame || taa_frame)graph->Declare(dlss_output_.Get(),1,rt,FrameLifetime::Persistent,"dlss_output");   // I-0111; I-0132 R11j: the TAA's output
            if(taa_base_copy)graph->Declare(taa_base_.Get(),1,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                FrameLifetime::Persistent,"taa_base");   // I-0113 tranche 39
            if(taa_fade_mask_pass)graph->Declare(taa_fade_mask_.Get(),1,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                FrameLifetime::Persistent,"taa_fade_mask");   // I-0140
            if(rr_now_){static constexpr const char* names[5]{"rr_albedo","rr_specular_albedo","rr_normal_roughness","rr_before_transparency","rr_specular_hit_distance"};
                for(size_t i=0;i<rr_targets_.size();++i)graph->Declare(rr_targets_[i].Get(),1,kRrGuideState,FrameLifetime::Persistent,names[i]);}   // I-0123
            if(rt_water_now_)graph->Declare(rt_water_target_.Get(),1,kRtWaterState,FrameLifetime::Persistent,"rt_water");   // I-0125 R6c
            if(rt_gi_mean_now_)graph->Declare(rt_gi_mean_target_.Get(),1,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,FrameLifetime::Persistent,"rt_gi_mean");   // I-0126 R7d
            if(rt_view_frame){graph->Declare(rt_view_images_[0].Get(),1,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,FrameLifetime::Persistent,"rt_view_traced");   // I-0117 T4
                graph->Declare(rt_view_images_[1].Get(),1,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,FrameLifetime::Persistent,"rt_view_classes");
                graph->Declare(rt_view_images_[2].Get(),1,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,FrameLifetime::Persistent,"rt_view_materials");}   // I-0119 R3b
            if(depth_out_pass)graph->Declare(depth_out_.Get(),2,D3D12_RESOURCE_STATE_DEPTH_WRITE,FrameLifetime::Persistent,"depth_out");   // I-0111 §3.7
            if(glow){static constexpr const char* names[5]{"glow_source","glow_l0","glow_l1","glow_l2","glow_l3"};const auto glow_targets=glow_.Targets();
                for(size_t i=0;i<glow_targets.size();++i)graph->Declare(glow_targets[i],1,rt,FrameLifetime::Persistent,names[i]);}   // W6: the new passes, shadowed first
            // I-0113 gap 4: the grading's source and volume, read back on the witness frames (their own barriers stay inside the stage).
            if(grading && sample_hdr){graph->Declare(post_tone_.Intermediate(),1,rt,FrameLifetime::Persistent,"grading_source");
                graph->Declare(post_tone_.Volume(),1,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,FrameLifetime::Persistent,"grading_volume");}
            // E3 (the default since D-0171): the frame's structure (what selects its passes) with the declarations is the plan key; a
            // plan the shadow found equal to the hand-written barriers for that key drives this frame's barriers.
            const bool sss=lit && native_sss_psos_[0] && !input.lighting->sss_profiles.empty() && !input.lighting->sss_scene.empty();
            const bool forward_pass=lit && !input.forward_draws.empty();
            const bool effect_frame=lit && input.effects && !input.effects->draws.empty();   // FFX-DRAW N6 (review: in the key)
            const bool copy_made=(forward_pass || effect_frame) && scene_copy && scene_copy->GetDesc().Width==RenderWidth() && scene_copy->GetDesc().Height==RenderHeight();
            graph->Execute(uint64_t(lit)|uint64_t(sss)<<1|uint64_t(forward_pass)<<2|uint64_t(copy_made)<<3|uint64_t(bool(input.velocity))<<4|
                uint64_t(sample_velocity)<<5|uint64_t(sample_pixels)<<6|uint64_t(bool(primitive))<<7|uint64_t(display)<<10|uint64_t(bool(overlay_frame))<<59|
                uint64_t(glow)<<11|uint64_t((forward_pass || water_frame) && forward_gb5)<<12|uint64_t(alpha_r)<<13|uint64_t(dof)<<14|uint64_t(motionblur)<<15|uint64_t(grading)<<16|
                uint64_t(sample_hdr)<<17|uint64_t(mask_frame)<<18|uint64_t(taa_fade_mask_pass)<<20|   // bit 19 NIS's passes (I-0132 R12f); bit 20 the TAA's fade mask (I-0140); bits 21..24, 26..29, 36 and 37: the SSAO plan's, the cascades', the local atlas's and the lights check's, unused (D-0251, I-0131 R10a); bit 34, the game's own AA filter until I-0138 tranche 5c, unused; bit 46, the lights check's until then, the tone map's HDR entry and bit 63 the HDR10 chain (I-0138 tranche 5); bit 55, R9d's fill shadow pass until I-0132 R11d (D-0262), I-0132 R11h's automatic exposure since
                uint64_t(sun_shadow)<<25|uint64_t(effect_frame)<<30|
                uint64_t(effect_copy_pass==0x2D)<<31|uint64_t(effect_copy_pass==0x31)<<32|uint64_t(decal_frame)<<33|uint64_t(nis_frame)<<19|uint64_t(water_frame)<<35|
                uint64_t(organize_frame)<<38|uint64_t(forward_depth_frame)<<39|uint64_t(dlss_frame)<<40|uint64_t(depth_out_pass)<<41|uint64_t(rt_view_frame)<<42|uint64_t(rt_view_pictures)<<43|uint64_t(rt_sun_now_)<<44|uint64_t(effect_depth_frame)<<45|uint64_t(rr_now_)<<47|uint64_t(rt_water_now_)<<48|uint64_t(layer_frame)<<49|uint64_t(layer_frame && input.effects->layer_bilateral)<<50|uint64_t(rt_gi_now_)<<51|uint64_t(rt_nrd_now_)<<52|uint64_t(rt_gi_mean_now_)<<53|uint64_t(rt_restir_now_)<<54|uint64_t(auto_exposure_frame)<<55|uint64_t(fg_frame)<<56|uint64_t(taa_frame)<<57|uint64_t(rt_ser_now_)<<58|uint64_t(taa_base_copy)<<60|uint64_t(pass21_frame && effect_depth_frame)<<61|uint64_t(pass21_order)<<62|uint64_t(hdr_scene)<<46|uint64_t(hdr_out)<<63);   // the ping-pong; C4; DECALS-WAVE1 D1; WT-2
            active_graph_shadow=graph;}
        struct ShadowScope {~ShadowScope(){active_graph_shadow=nullptr;}} shadow_scope;   // also on an exception
        ClearGBuffer(list,targets,view_->dsv->GetCPUDescriptorHandleForHeapStart(),1.f,input.clear_stencil);
        if(graph){graph->Pass("gbuffer",PassKind::Material);for(const auto& color:view_->colors)graph->Use(color.Get(),ResourceAccess::Write,D3D12_RESOURCE_STATE_RENDER_TARGET);
            graph->Use(view_->depth.Get(),ResourceAccess::Write,D3D12_RESOURCE_STATE_DEPTH_WRITE);}
        const auto frame_blocks=UploadNativeFrameBlocks(input);   // C4-1: once per view, for its three passes
        // C4-1b: every other constant payload copied once per view as its draws are recorded.
        NativeModelConstantUploads uploads([this](uint64_t bytes){const auto range=frame_uploads_.Allocate(gpu_.device.Get(),bytes,UploadArena::alignment,false);
            return NativeModelConstantUploads::Range{range.cpu,range.gpu};},frames_.Numbered(),frames_.CompletedThrough());   // C4-2a
        if(view_==&main_view_ && !input.paints.empty())EncodeNativePaint(list,input,uploads,FrameAddressesOf(frame_blocks,NativeModelPass::Deferred));   // D2: the paint before the G-buffer that samples it
        rt_scene_address_=0;rt_material_table_=rt_material_rows_=0;   // T3 second step review L5: no former frame's table
        rt_hold_now_=view_==&main_view_ && input.rt_hold;rt_world_black_=rt_hold_now_;rt_judged_now_=false;   // I-0132 R11e: the arrival held (step 3)
        if(view_==&main_view_ && input.rt_scene){rt_reflect_anisotropy_=input.rt_scene->anisotropy;rt_reflect_bias_=input.rt_scene->texture_bias;rt_reflect_time_=input.rt_scene->time;}   // I-0125 R6b, R6d
        if(view_==&main_view_)rt_reflect_sun_=input.rt_sun;
        if(view_==&main_view_ && input.rt_scene && !rt_failed_){GpuZoneBegin(list,GpuZone::RtScene);EncodeRtScene(list,*input.rt_scene);GpuZoneEnd(list,GpuZone::RtScene);}   // I-0117 (R2 T1)
        else if(view_==&main_view_)TickRtMaterials(list);   // T3: no rays this frame
        // I-0140 tranche 31 (its step review M2): each menu table no view took this frame (they record before the main view) swept on
        // the first kMenuRtIdleTicks such frames, so what the menu caches let go leaves it.
        if(view_==&main_view_)TickMenuRtMaterials(list);
        // I-0074 point 3c (ER 141B3E890 / 141B3D940): `count` draws in chunks of near-equal size recorded on the recording pool,
        // each into its own list of the slot with its own bind state and constant uploads; the list so far is submitted before
        // them and the frame continues in a further list after them, in chunk order (ER's ordered submission). `record(list,
        // uploads, state, begin, end)` records one chunk. False when the draws are too few (or the switch is off): the caller
        // records them on this list. The G-buffer draws (the sun-shadow casters stay under its threshold at the witness
        // scenes, so they are recorded on this list: I-0075 review M2).
        using ChunkRecorder=std::function<void(ID3D12GraphicsCommandList*,NativeModelConstantUploads&,GBufferBindState&,size_t,size_t)>;
        const auto record_chunks=[&,this](size_t count,const ChunkRecorder& record,const char* what)->bool{
            auto& pool=RecordingPool();
            const size_t chunks=record_chunks_disabled_.load(std::memory_order_relaxed) || pool.Workers()<2 || count<2*record_chunk_draws?1:
                std::min<size_t>({count/record_chunk_draws,pool.Workers()*2,record_chunks_max});
            if(chunks<=1)return false;
            // Review L7: a refused close names the debug layer's message, as the frame's last close does (the chunks' closes are
            // checked after the pool returns, on this thread).
            const auto close_or_throw=[&](HRESULT closed,const char* failed){if(SUCCEEDED(closed))return;Diagnostics();
                char why[480]{};sprintf_s(why,"%s (%s): HRESULT %08X (%.360s)",failed,what,static_cast<unsigned>(closed),last_diagnostic_.c_str());throw std::runtime_error(why);};
            close_or_throw(list->Close(),"native model recorder close before the chunks");submitted_lists.push_back(list);
            std::vector<HRESULT> chunk_closes(chunks,S_OK);
            std::vector<ID3D12GraphicsCommandList*> chunk_lists(chunks);
            for(auto& chunk_list:chunk_lists)chunk_list=frames_.ExtraList(gpu_.device.Get(),slot);   // the ring on this thread
            std::mutex arena_lock;std::atomic<size_t> ticket{};
            const auto numbered=frames_.Numbered(),completed=frames_.CompletedThrough();
            const std::function<void(size_t)> job=[&](size_t){
                for(size_t c;(c=ticket.fetch_add(1,std::memory_order_relaxed))<chunks;){
                    auto* chunk_list=chunk_lists[c];
                    // The chunk's payloads from 64 KB blocks of the frame arena, one locked allocation a block (a lock a payload took
                    // 9 % of the render worker at m45, i74-native-18); a payload past a quarter block takes its own.
                    const uint64_t block_bytes=64*1024;NativeModelConstantUploads::Range block{};uint64_t left{};
                    NativeModelConstantUploads chunk_uploads([&](uint64_t bytes){const uint64_t size=(bytes+UploadArena::alignment-1)&~(UploadArena::alignment-1);
                        if(size>left){const std::lock_guard hold(arena_lock);
                            if(size>block_bytes/4){const auto range=frame_uploads_.Allocate(gpu_.device.Get(),size,UploadArena::alignment,false);return NativeModelConstantUploads::Range{range.cpu,range.gpu};}
                            const auto range=frame_uploads_.Allocate(gpu_.device.Get(),block_bytes,UploadArena::alignment,false);block={range.cpu,range.gpu};left=block_bytes;}
                        const NativeModelConstantUploads::Range taken=block;block.cpu+=size;block.gpu+=size;left-=size;return taken;},numbered,completed);
                    GBufferBindState state;
                    // Review L6: a chunk that throws closes its list (the renderer faults, but no open list is left behind).
                    try{record(chunk_list,chunk_uploads,state,count*c/chunks,count*(c+1)/chunks);}catch(...){(void)chunk_list->Close();throw;}
                    chunk_closes[c]=chunk_list->Close();
                    record_chunk_blocks_.fetch_add(chunk_uploads.Blocks(),std::memory_order_relaxed);}};
            pool.Run(job);
            for(const auto closed:chunk_closes)close_or_throw(closed,"native model chunk close");
            submitted_lists.insert(submitted_lists.end(),chunk_lists.begin(),chunk_lists.end());
            list=frames_.ExtraList(gpu_.device.Get(),slot);if(graph)graph->SetList(list);
            record_chunked_frames_.fetch_add(1,std::memory_order_relaxed);record_chunks_.fetch_add(chunks,std::memory_order_relaxed);
            return true;};
        {const auto gbuffer_dsv=view_->dsv->GetCPUDescriptorHandleForHeapStart();const auto deferred=FrameAddressesOf(frame_blocks,NativeModelPass::Deferred);
            const auto gbuffer_draws=[&](ID3D12GraphicsCommandList* into,NativeModelConstantUploads& with,GBufferBindState& state,size_t begin,size_t end){
                for(size_t i=begin;i<end;++i)input.draws[i]->Encode(into,targets,gbuffer_dsv,viewport,scissor,deferred,with,&state);};
            if(!record_chunks(input.draws.size(),gbuffer_draws,"G-buffer")){GBufferBindState state;gbuffer_draws(list,uploads,state,0,input.draws.size());}}   // m45: one bind state a loop
        if(decal_frame)EncodeNativeDecals(list,input,uploads,viewport,scissor);   // DECALS-WAVE1 D1: after the G-buffer, before the lighting (51AA0)
        // I-0126 R7c: on NRD's frames (plan key bit 52) the Velo rows before the lighting (NRD's signal reads the G-buffer surface's
        // motion), their resolve after the forward rows; the other frames draw them where the original does (the final review's M2:
        // there a row behind a forward row that writes depth keeps the camera motion for DLSS, RR and the motion blur; on NRD's
        // frames it does not, the fixes' step review M-a, left: R7c RESULT).
        if(input.velocity && rt_nrd_now_)EncodeNativeVelocityRows(list,input,FrameAddressesOf(frame_blocks,NativeModelPass::Velocity),uploads,viewport,scissor);
        if(lit)EncodeNativeLighting(list,*input.lighting,viewport,scissor,mask_frame?input.mask.get():nullptr,sun_shadow?&input:nullptr);
        // I-0113 witness (world-pictures.flag with gbuffer-pictures.flag, the Handmaid chair): GB0..GB4 as the lighting read them,
        // beside the presented picture.
        if(sample_hdr && gbuffer_pictures){const auto path=std::filesystem::path(pixel_file);constexpr auto rt=D3D12_RESOURCE_STATE_RENDER_TARGET;
            constexpr const wchar_t* gbuffer_files[5]{L"-gb0.rgba16f",L"-gb1.rgba8",L"-gb2.rgba8",L"-gb3.rgba8",L"-gb4.r11g11b10"};
            for(unsigned i=0;i<5;++i)RecordPixels(list,slot,view_->colors[i].Get(),rt,(path.parent_path()/(path.stem().wstring()+gbuffer_files[i])).wstring(),
                "native G-buffer evidence write",i==0?8:4,rt);}
        // D-0180 C4 witness (world-pictures.flag): the mask t32 beside the presented picture, 
        if(sample_hdr && mask_frame){const auto path=std::filesystem::path(pixel_file);
            RecordPixels(list,slot,mask_.Mask(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,(path.parent_path()/(path.stem().wstring()+L"-mask.rgba")).wstring(),
                "native shadow mask evidence write",4,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);}
        // I-0117 T4: the depth as the deferred rows left it (the decals and the deferred lighting since write none; no forward row yet).
        if(rt_view_frame)EncodeRtView(list,slot,input,graph);
        if(organize_frame){if(!EncodeNativeLightOrganize(list,*input.lighting))throw std::logic_error("native light organize after its plan");
            // Step review: the slot holds the grid this list writes and reads (a growth replaces the sets the job and renderer hold).
            for(auto* r:{light_grid_frame_.indices.Get(),light_grid_frame_.lookup.Get(),light_grid_frame_.work.Get(),light_grid_frame_.points.Get(),
                light_grid_frame_.spots.Get(),light_grid_frame_.lines.Get()})slot.kept.push_back(HoldResource(r));}
        const bool effect_pass=lit && input.effects && !input.effects->draws.empty();   // FFX-DRAW N6
        if(input.effects && !effect_pass)effects_.Retain(*input.effects);   // E07 (review): its releases and creation bytes wait for the next encode
        if(lit && (!input.forward_draws.empty() || effect_pass || water_frame)){
            // Forward pass 0x29 after GXLightComposite, into the lit HDR image,
            // depth tested against the G-buffer depth (written by the rows whose state asks it: phantom, 0x1F and 0x29 rows).
            auto* hdr=view_->light_targets[3].Get();
            const std::array<D3D12_CPU_DESCRIPTOR_HANDLE,1> forward{CPU(view_->light_rtvs.Get(),rtv_stride_,3)};
            // W6 GL-6: Forward2 draws also write GB5 (left a render target by the lighting).
            const std::array<D3D12_CPU_DESCRIPTOR_HANDLE,2> forward2{forward[0],targets[5]};
            const auto encode_one=[&](const OwnedNativeModelDraw& draw,GBufferBindState& state){const bool second=draw.Input().shape.envelope==ModelPassEnvelope::Forward2;
                draw.Encode(list,second?std::span<const D3D12_CPU_DESCRIPTOR_HANDLE>(forward2):std::span<const D3D12_CPU_DESCRIPTOR_HANDLE>(forward),
                    view_->dsv->GetCPUDescriptorHandleForHeapStart(),viewport,scissor,FrameAddressesOf(frame_blocks,NativeModelPass::Forward),uploads,&state);};
            const auto encode_forward=[&](std::span<const std::shared_ptr<const OwnedNativeModelDraw>> draws){
                GBufferBindState state;   // m45: one bind state a loop
                for(const auto& draw:draws)encode_one(*draw,state);};
            const auto forward_uses=[&](const char* name){if(!graph)return;graph->Pass(name,PassKind::Material);graph->Use(hdr,ResourceAccess::ReadWrite,D3D12_RESOURCE_STATE_RENDER_TARGET);
                graph->Use(view_->depth.Get(),ResourceAccess::ReadWrite,D3D12_RESOURCE_STATE_DEPTH_WRITE);
                if(forward_gb5)graph->Use(view_->colors[5].Get(),ResourceAccess::ReadWrite,D3D12_RESOURCE_STATE_RENDER_TARGET);
                if(scene_copy)graph->Use(scene_copy.Get(),ResourceAccess::Read,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);   // t45 (review)
                if(water_frame || forward_depth_frame)graph->Use(input.water_depth_copy.Get(),ResourceAccess::Read,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);   // t56
                // I-0125 R6c (step review H1): t54, the traced water's target, read in its home state (the executed plan brings it back there).
                if(rt_water_now_)graph->Use(rt_water_target_.Get(),ResourceAccess::Read,kRtWaterState);};
            // C4: the scene-colour copy (t45) of the lit HDR (FFX-FAMILIES-PHASE2 §8): from `from` (the HDR's state), back to RENDER_TARGET.
            const auto copy=scene_copy;   // cR2 (X2): the job's copy, not the member
            const bool copy_fits=copy && copy->GetDesc().Width==RenderWidth() && copy->GetDesc().Height==RenderHeight();
            bool copy_held=false;   // the optimisation pass of I-0113 tranche 23: held once a frame, however many copies (a quad each)
            const auto copy_scene=[&](D3D12_RESOURCE_STATES from,const char* name){
                if(!copy_held){slot.kept.push_back(HoldResource(copy.Get()));copy_held=true;}   // cD4 (D4): a size change replaces the member while this list may be in flight
                Transition(list,hdr,from,D3D12_RESOURCE_STATE_COPY_SOURCE);
                Transition(list,copy.Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_DEST);
                if(graph){graph->Pass(name,PassKind::Effect);graph->Use(hdr,ResourceAccess::Read,D3D12_RESOURCE_STATE_COPY_SOURCE);
                    graph->Use(copy.Get(),ResourceAccess::Write,D3D12_RESOURCE_STATE_COPY_DEST);}
                list->CopyResource(copy.Get(),hdr);
                Transition(list,copy.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                Transition(list,hdr,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);};
            // WT-2: the readable depth (t56) copied from the depth.
            const auto copy_depth=[&]{auto* depth=view_->depth.Get();auto* readable=input.water_depth_copy.Get();
                slot.kept.push_back(HoldResource(readable));
                Transition(list,depth,D3D12_RESOURCE_STATE_DEPTH_WRITE,D3D12_RESOURCE_STATE_COPY_SOURCE);
                Transition(list,readable,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_DEST);
                if(graph){graph->Pass("water_depth_copy",PassKind::Material);graph->Use(depth,ResourceAccess::Read,D3D12_RESOURCE_STATE_COPY_SOURCE);
                    graph->Use(readable,ResourceAccess::Write,D3D12_RESOURCE_STATE_COPY_DEST);}
                list->CopyResource(readable,depth);
                Transition(list,readable,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                Transition(list,depth,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_DEPTH_WRITE);};
            // I-0113 tranche 15: the forward draws come in the original's order (SortNativeForwardRows): the 0x1F, 0x21 and 0x22 rows,
            // then from forward_sorted the 0x29 rows, which the effects' statements of 0x29 interleave by key (a row first at an equal
            // key, INFERRED); without effects the 0x29 rows follow the others.
            const size_t forward_count=input.forward_draws.size();
            const bool interleave=effect_pass && input.forward_keys.size()==forward_count;
            const size_t rows_end=interleave?std::min(input.forward_sorted,forward_count):forward_count;
            const auto early=std::min<size_t>(input.forward_early,rows_end);
            const auto& encode_forward_rows=encode_forward;
            // The forward block from row `from` (at most `early`): the rows before the water and the 0x29 rows, the water, the TAA base.
            // I-0140 tranche 4: on a frame in pass21_order, the rows before `from` (the 0x1F rows, then the 0x21 rows with pass 0x21's
            // effects among them, by key) are drawn first, and this block runs at the effects' first later pass (or after them).
            // I-0140 tranche 4 (its step review's M): with `failure` given (the tail run among the effects), the inner encode_forward keeps a row's failure there
            // and draws no row after it, but still makes the copies and declares every pass, so the graph's passes stay the plan's.
            const auto forward_tail=[&](size_t from,std::exception_ptr* failure){
                const auto encode_forward=[&](std::span<const std::shared_ptr<const OwnedNativeModelDraw>> draws){
                    if(!failure){encode_forward_rows(draws);return;}
                    if(*failure)return;
                    try{encode_forward_rows(draws);}catch(...){*failure=std::current_exception();}};
                if(water_frame){
                    // WATER-TESS-WAVE1-READY 3.2 (the GPU order of 1.4): the 0x1F rows; the readable depth (t56) copied from the depth;
                    // WatDpt (0x24, its alpha 0 the refraction mask); C1, the frame copy the surface refracts (t55); the surface (0x27,
                    // writing depth); C2, the copy the later forward rows and the effects read (t45, the surface in it); the other rows.
                    forward_uses("forward_early");encode_forward({input.forward_draws.data()+from,early-from});
                    copy_depth();
                    forward_uses("water_depth");encode_forward(input.water_depth_draws);
                    copy_scene(D3D12_RESOURCE_STATE_RENDER_TARGET,"water_copy");
                    forward_uses("water_surface");encode_forward(input.water_draws);
                    copy_scene(D3D12_RESOURCE_STATE_RENDER_TARGET,"scene_copy");
                    if(taa_base_copy){CopyTaaBase(list);slot.kept.push_back(HoldResource(taa_base_.Get()));taa_base_copied=true;}   // I-0113 tranche 39: the water in the base
                    forward_uses("forward");encode_forward({input.forward_draws.data()+early,rows_end-early});
                }else if(taa_base_copy){   // I-0113 tranche 39: the base before the 0x29 rows
                    forward_uses("forward_early");encode_forward({input.forward_draws.data()+from,early-from});
                    CopyTaaBase(list);slot.kept.push_back(HoldResource(taa_base_.Get()));taa_base_copied=true;
                    forward_uses("forward");encode_forward({input.forward_draws.data()+early,rows_end-early});}
                else{forward_uses("forward");encode_forward({input.forward_draws.data()+from,rows_end-from});}};
            if(water_frame)Transition(list,hdr,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);
            else{
                if(forward_depth_frame)copy_depth();   // I-0105
                if(copy_fits)copy_scene(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,"scene_copy");   // C4: at the start of the forward pass
                else Transition(list,hdr,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);}
            if(pass21_order){forward_uses("forward_1f");encode_forward({input.forward_draws.data(),input.forward_1f});}
            else forward_tail(0,nullptr);
            // FFX-DRAW N6: the classic FFX draws and GPU-particle statements of engine passes 0x21 (I-0140), 0x29, 0x2D, 0x31 into the
            // lit HDR and GB5, t4 the scene-colour copy. I-0113 tranche 15: with the 0x29 rows between them by key (FINISH-LINE line 7), the
            // depth writable for both (the effects' write mask zero but 0x21's, whose opaque draws write it), the programmes that sample it
            // (t3, the particles' resources) reading a copy made after the 0x1F, 0x21 and 0x22 rows and pass 0x21's draws.
            if(effect_pass){
                const auto depth_copy_state=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;   // E07: the update programmes read it too
                const bool copy_usable=scene_copy && scene_copy->GetDesc().Width==RenderWidth() && scene_copy->GetDesc().Height==RenderHeight();
                // I-0113 tranche 17 (sub 0x28, 1401CB9D0): the layer's half depth, the minimum of the copy's 2x2 texels, then read-only
                // (DEPTH_READ, sampled at t3) for pass 0x31. The table (the depth copy for the downsample; then the layer's colour, the
                // main depth and the half depth for the bilateral composite) is written into a fresh table when one of them changes.
                auto& layer=effect_layer_;const auto layer_read=D3D12_RESOURCE_STATE_DEPTH_READ|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;   // step review M3: 0x31's GPU statements read it too
                const UINT view_stride=gpu_.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                const auto layer_table=[&](UINT index){auto h=layer.table->GetGPUDescriptorHandleForHeapStart();h.ptr+=uint64_t(index)*view_stride;return h;};
                // I-0140 (its step review's M1): the depth copy (t3) and the layer's half depth (sub 0x28) are taken after pass 0x21's opaque
                // draws, which write the depth (the original's sub 0x21 before its sub 0x28): before Encode when the frame has none, else at
                // the first later pass's before_pass (its draws reading t3 then see the debris), or after Encode when no later pass draws.
                bool effect_depth_prepared=false;std::exception_ptr effect_depth_failure;
                // Its step reviews' L2 and L-a, its final review's L1: marked done at its end; a failure inside it, caught by the effects'
                // catch, is thrown again, itself, at the next call (after Encode, outside that catch) without redoing the work or
                // declaring its passes twice. (effect_depth_frame covers layer_frame.)
                const auto prepare_effect_depth=[&]{if(effect_depth_prepared)return;if(effect_depth_failure)std::rethrow_exception(effect_depth_failure);
                    try{
                    if(effect_depth_frame){auto* depth=view_->depth.Get();auto* copied=effect_depth_copy_.Get();
                        slot.kept.push_back(HoldResource(copied));
                        Transition(list,depth,D3D12_RESOURCE_STATE_DEPTH_WRITE,D3D12_RESOURCE_STATE_COPY_SOURCE);
                        Transition(list,copied,depth_copy_state,D3D12_RESOURCE_STATE_COPY_DEST);
                        if(graph){graph->Pass("effect_depth_copy",PassKind::Effect);graph->Use(depth,ResourceAccess::Read,D3D12_RESOURCE_STATE_COPY_SOURCE);
                            graph->Use(copied,ResourceAccess::Write,D3D12_RESOURCE_STATE_COPY_DEST);}
                        list->CopyResource(copied,depth);
                        Transition(list,copied,D3D12_RESOURCE_STATE_COPY_DEST,depth_copy_state);
                        Transition(list,depth,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_DEPTH_WRITE);}
                    if(layer_frame){
                        if(layer.viewed_copy!=effect_depth_copy_.Get() || layer.viewed_depth!=view_->depth.Get() || layer.viewed_generation!=layer.generation+view_->target_generation){
                            ComPtr<ID3D12DescriptorHeap> fresh;const D3D12_DESCRIPTOR_HEAP_DESC desc{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,4,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
                            Check(CountedHeap(gpu_.device.Get())->CreateDescriptorHeap(&desc,IID_PPV_ARGS(&fresh)),"effect layer table");
                            D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Format=DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
                            v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;v.Texture2D.MipLevels=1;
                            auto at=fresh->GetCPUDescriptorHandleForHeapStart();gpu_.device->CreateShaderResourceView(effect_depth_copy_.Get(),&v,at);
                            v.Format=ForwardTargetFormat();at.ptr+=view_stride;gpu_.device->CreateShaderResourceView(layer.color.Get(),&v,at);
                            v.Format=DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;at.ptr+=view_stride;gpu_.device->CreateShaderResourceView(view_->depth.Get(),&v,at);
                            at.ptr+=view_stride;gpu_.device->CreateShaderResourceView(layer.depth.Get(),&v,at);
                            if(layer.table)RetireAfterCurrentRecording(HoldObject(layer.table.Get()));layer.table=fresh;layer.viewed_copy=effect_depth_copy_.Get();
                            layer.viewed_depth=view_->depth.Get();layer.viewed_generation=layer.generation+view_->target_generation;}
                        slot.kept.push_back(HoldResource(layer.color.Get()));slot.kept.push_back(HoldResource(layer.depth.Get()));
                        if(graph){graph->Pass("effect_layer_depth",PassKind::Effect);graph->Use(effect_depth_copy_.Get(),ResourceAccess::Read,depth_copy_state);
                            graph->Use(layer.depth.Get(),ResourceAccess::Write,D3D12_RESOURCE_STATE_DEPTH_WRITE);}
                        ID3D12DescriptorHeap* heaps[]{layer.table.Get()};list->SetDescriptorHeaps(1,heaps);list->SetGraphicsRootSignature(layer.root.Get());
                        list->SetPipelineState(layer.downsample.Get());const auto dsv=layer.dsv->GetCPUDescriptorHandleForHeapStart();list->OMSetRenderTargets(0,nullptr,FALSE,&dsv);
                        const D3D12_VIEWPORT half{0,0,float(layer.width),float(layer.height),0,1};const D3D12_RECT half_rect{0,0,LONG(layer.width),LONG(layer.height)};
                        list->RSSetViewports(1,&half);list->RSSetScissorRects(1,&half_rect);list->SetGraphicsRootDescriptorTable(0,layer_table(0));
                        list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);list->DrawInstanced(3,1,0,0);
                        Transition(list,layer.depth.Get(),D3D12_RESOURCE_STATE_DEPTH_WRITE,layer_read);}
                    }catch(...){effect_depth_failure=std::current_exception();throw;}
                    effect_depth_prepared=true;};
                if(!pass21_frame)prepare_effect_depth();
                // The effects' uses, the 0x29 rows' among them.
                const auto effect_uses=[&](const char* name){if(!graph)return;graph->Pass(name,PassKind::Effect);graph->Use(hdr,ResourceAccess::ReadWrite,D3D12_RESOURCE_STATE_RENDER_TARGET);
                    graph->Use(view_->colors[5].Get(),ResourceAccess::ReadWrite,D3D12_RESOURCE_STATE_RENDER_TARGET);
                    graph->Use(view_->depth.Get(),ResourceAccess::ReadWrite,D3D12_RESOURCE_STATE_DEPTH_WRITE);
                    if(scene_copy)graph->Use(scene_copy.Get(),ResourceAccess::Read,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                    if(effect_depth_frame)graph->Use(effect_depth_copy_.Get(),ResourceAccess::Read,depth_copy_state);
                    if(water_frame || forward_depth_frame)graph->Use(input.water_depth_copy.Get(),ResourceAccess::Read,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);};   // the rows' t56
                effect_uses("effects");
                // I-0113 tranche 29 (effect-scan.flag): stage 3, the lit HDR before the effects and the 0x29 rows (the lighting, the rays and
                // the other forward rows in it), so a value found later is placed before or inside the effect block.
                if(input.effects->scan && effect_copy_pass)RecordEffectScan(list,slot,hdr,D3D12_RESOURCE_STATE_RENDER_TARGET,3,uint32_t(effect_copy_pass),input.serial);
                // I-0113 tranche 30 (effect-scan-draws.flag): the HDR's negatives counted before the block (kind 0), after each 0x29
                // drawing statement (1) and the 0x29 rows drawn before a statement (2), and once the 0x29 pass and its rows are done (3).
                // Tranche 31: on through pass 0x2D, after each of its drawing statements (4), closed after it (5) at the first later
                // pass (0x31 draws into its layer) or the block's end.
                const bool draw_scanned=input.effects->scan_draws && BeginEffectDrawScan(list,slot,hdr);bool draw_scan_closed=false,draw_scan_2d=false;
                if(draw_scanned)RecordEffectDrawScanPoint(list,0,0x29,0,nullptr);
                const auto close_draw_scan=[&](int32_t pass){if(!draw_scanned || draw_scan_closed || pass==0x29)return false;
                    if(pass==0x2D){if(draw_scan_2d)return false;draw_scan_2d=true;RecordEffectDrawScanPoint(list,3,0x29,0,nullptr);return true;}
                    draw_scan_closed=true;RecordEffectDrawScanPoint(list,draw_scan_2d?5:3,draw_scan_2d?0x2D:0x29,0,nullptr);return true;};
                slot.kept.push_back(input.effects);
                EffectStage::Targets effect_targets{};
                effect_targets.hdr=forward[0];effect_targets.gb5=targets[5];effect_targets.hdr_format=ForwardTargetFormat();effect_targets.gb5_format=color_formats[5];
                effect_targets.dsv=view_->dsv->GetCPUDescriptorHandleForHeapStart();
                effect_targets.depth_format=ModelDepthFormat();effect_targets.depth=effect_depth_frame?effect_depth_copy_.Get():nullptr;effect_targets.depth_view_format=DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
                effect_targets.target_generation=view_->target_generation;
                if(copy_usable){effect_targets.scene_copy=scene_copy.Get();effect_targets.scene_copy_format=ForwardTargetFormat();}
                // I-0113 tranche 17: pass 0x31's targets, the layer's (sub 0x2F: its one RTV and the half depth's read-only view).
                EffectStage::Targets layer_targets{};bool layer_cleared=false;
                if(layer_frame){auto dsv=layer.dsv->GetCPUDescriptorHandleForHeapStart();dsv.ptr+=gpu_.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
                    layer_targets.hdr=layer.rtv->GetCPUDescriptorHandleForHeapStart();layer_targets.hdr_format=ForwardTargetFormat();layer_targets.dsv=dsv;
                    layer_targets.depth_format=ModelDepthFormat();layer_targets.depth=layer.depth.Get();layer_targets.depth_view_format=DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
                    layer_targets.scene_copy=effect_targets.scene_copy;layer_targets.scene_copy_format=effect_targets.scene_copy_format;
                    layer_targets.viewport={0,0,float(layer.width),float(layer.height),0,1};layer_targets.scissor={0,0,LONG(layer.width),LONG(layer.height)};
                    effect_targets.layer=&layer_targets;}
                // The 0x29 rows keyed at or before `limit`, from where the last run stopped.
                // Step review L2: a row's failure is the frame's, as before the interleave (thrown after the effects, never taken
                // for an effects failure by Encode's catch); the rows after it are not drawn.
                size_t next_row=rows_end;std::exception_ptr row_failure;
                const auto rows_until=[&](uint64_t limit){if(next_row>=forward_count || input.forward_keys[next_row]>limit)return false;   // the final review's L4: no state made for nothing
                    GBufferBindState state;bool drew=false;
                    try{while(next_row<forward_count && input.forward_keys[next_row]<=limit){drew=true;encode_one(*input.forward_draws[next_row++],state);}}
                    catch(...){row_failure=std::current_exception();next_row=forward_count;}
                    return drew;};
                // I-0140 tranche 4: on a frame in pass21_order, the 0x21 rows keyed at or before `limit` (from where the last run stopped), and
                // the forward block's rest (forward_tail: the 0x22 rows, the water, the TAA base) once, at the first pass after 0x21 or after
                // Encode; its failure is the frame's, as a row's (thrown after the effects; the 0x29 rows then not drawn).
                // Its step review's L1 and L2: on such a frame the effect scans' stage 3 and the effect-measure timestamps take the 0x21
                // and 0x22 rows and the water inside the block; and on a water frame no copy is made before 0x21, so a 0x21 statement
                // sampling t4 reads the copy's last content (not checked: no witness reads 0x21's textures; an explicit policy until one does).
                size_t next_row21=pass21_order?input.forward_1f:0;bool tail_pending=pass21_order;
                const auto rows21_until=[&](uint64_t limit){if(!pass21_order || next_row21>=input.forward_21 || input.forward_keys[next_row21]>limit)return false;
                    GBufferBindState state;
                    try{while(next_row21<input.forward_21 && input.forward_keys[next_row21]<=limit)encode_one(*input.forward_draws[next_row21++],state);}
                    catch(...){row_failure=std::current_exception();next_row21=input.forward_21;next_row=forward_count;}
                    return true;};
                const auto run_tail=[&]{if(!tail_pending)return false;tail_pending=false;rows21_until(UINT64_MAX);
                    try{forward_tail(input.forward_21,&row_failure);}catch(...){if(!row_failure)row_failure=std::current_exception();}   // its final review's L1: a copy's failure too
                    if(row_failure)next_row=forward_count;
                    effect_uses("effects_after_rows");return true;};
                // FFX-DRAW (Native2027): the distortion (0x2D) and later passes sample the scene with the 0x29 effects in it (their
                // quads otherwise show a rectangle without the fire): the copy is remade before the first of them that samples it,
                // after the last 0x29 rows.
                bool effect_copied=false;   // review: the copy passes the plan key promises are declared even if Encode stops early
                const auto before_pass=[&](int32_t pass){
                    const bool tail=pass!=0x21 && run_tail();   // I-0140 tranche 4: the forward block's rest after pass 0x21
                    // I-0140: the depth copy and the half depth after 0x21's draws (above), before the 0x29 rows (tranche 15's copy).
                    const bool prepared=pass!=0x21 && !effect_depth_prepared && effect_depth_frame;if(prepared){prepare_effect_depth();effect_uses("effects_after_depth");}
                    const bool drew=(pass!=0x29 && pass!=0x21 && rows_until(UINT64_MAX)) || prepared || tail;   // I-0140: 0x21 before the 0x29 rows
                    // I-0113 tranche 17 (sub 0x2F): the layer cleared to (0, 0, 0, 1) before pass 0x31, after the scene copy if it comes now.
                    const auto clear_layer=[&]{if(pass!=0x31 || !layer_frame || layer_cleared)return;layer_cleared=true;
                        if(graph){graph->Pass("effect_layer",PassKind::Effect);graph->Use(layer.color.Get(),ResourceAccess::ReadWrite,D3D12_RESOURCE_STATE_RENDER_TARGET);
                            graph->Use(layer.depth.Get(),ResourceAccess::Read,layer_read);if(scene_copy)graph->Use(scene_copy.Get(),ResourceAccess::Read,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);}
                        const float black[4]{0,0,0,1};list->ClearRenderTargetView(layer.rtv->GetCPUDescriptorHandleForHeapStart(),black,0,nullptr);};
                    if(pass!=effect_copy_pass || effect_copied){clear_layer();return drew;}
                    effect_copied=true;copy_scene(D3D12_RESOURCE_STATE_RENDER_TARGET,"effect_copy");
                    effect_uses("effects_after_copy");
                    // I-0113 tranche 29: after the pass opens (the final review's H1: on an executed frame the graph brings the copy to
                    // PIXEL_SHADER_RESOURCE only there, copy_scene's own transitions being suppressed).
                    if(input.effects->scan)RecordEffectScan(list,slot,copy.Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,0,uint32_t(pass),input.serial);
                    clear_layer();return drew;};
                effect_targets.before_pass=[&](int32_t pass){const bool drew=before_pass(pass);return (pass!=0x21 && close_draw_scan(pass)) || drew;};
                if(interleave)effect_targets.before_draw=[&](int32_t pass,uint32_t key){if(pass==0x21)return rows21_until(key);   // I-0140 tranche 4
                    if(pass!=0x29 || !rows_until(key))return false;
                    if(draw_scanned)RecordEffectDrawScanPoint(list,2,pass,key,nullptr);return true;};
                if(draw_scanned)effect_targets.after_draw=[&](const NativeEffectDraw& d){
                    if((d.pass!=0x29 && d.pass!=0x2D) || draw_scan_closed || (d.op!=NativeEffectOp::Draw && d.op!=NativeEffectOp::DrawIndirect))return false;
                    RecordEffectDrawScanPoint(list,d.pass==0x2D?4:1,d.pass,d.key,&d);return true;};
                // I-0113 tranche 23 (gap 22): a distortion quad's own copy (140260440; the trails' 262A70 the whole target): its box in the
                // target's pixels, the rectangle floored and ceiled (the +0xA88 branch: from (0, 0) to its size), 16 pixels wider on each
                // side and clamped to the target, copied into the scene copy at the same place. Each quad then distorts the ones drawn
                // before it, as the original's (the far mist's cross-fading pairs, gap 22). A rectangle not finite copies nothing.
                // The final review's M3: a copy's barriers are local to it (the HDR RENDER_TARGET -> COPY_SOURCE and the copy
                // PIXEL_SHADER_RESOURCE -> COPY_DEST, then back, one ResourceBarrier call each way), so both resources leave it in the
                // states the effect passes declare: no graph pass of its own, nothing in the plan key, however many quads a frame draws.
                const auto copy_rect=[&](const D3D12_BOX& box){
                    if(box.right<=box.left || box.bottom<=box.top)return;
                    if(!copy_held){slot.kept.push_back(HoldResource(copy.Get()));copy_held=true;}
                    const auto barriers=[&](D3D12_RESOURCE_STATES hdr_from,D3D12_RESOURCE_STATES hdr_to,D3D12_RESOURCE_STATES copy_from,D3D12_RESOURCE_STATES copy_to){
                        D3D12_RESOURCE_BARRIER b[2]{};for(auto& x:b)x.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                        b[0].Transition={hdr,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,hdr_from,hdr_to};b[1].Transition={copy.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,copy_from,copy_to};
                        list->ResourceBarrier(2,b);};
                    barriers(D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_DEST);
                    D3D12_TEXTURE_COPY_LOCATION to{},from{};to.pResource=copy.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                    from.pResource=hdr;from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;list->CopyTextureRegion(&to,box.left,box.top,0,&from,&box);
                    barriers(D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);};
                if(copy_rect_frame)effect_targets.copy_rect=[&](const NativeEffectDraw& d){
                    const int64_t w=RenderWidth(),h=RenderHeight();
                    const auto clamp=[](int64_t v,int64_t n){return v<0?int64_t{0}:v>n?n:v;};
                    int64_t left{},top{},right{},bottom{};
                    if(d.copy_kind==1){right=w;bottom=h;}
                    else if(d.copy_kind==2){right=clamp(int64_t(d.copy_size[0])+16,w);bottom=clamp(int64_t(d.copy_size[1])+16,h);}
                    else if(std::all_of(d.copy_rect.begin(),d.copy_rect.end(),[](float v){return std::isfinite(v) && std::abs(v)<1e6f;})){
                        const auto& r=d.copy_rect;
                        left=clamp(int64_t(std::floor(double(r[0])*w))-16,w);top=clamp(int64_t(std::floor(double(r[1])*h))-16,h);
                        right=clamp(int64_t(std::ceil(double(r[2])*w))+16,w);bottom=clamp(int64_t(std::ceil(double(r[3])*h))+16,h);}
                    copy_rect(D3D12_BOX{UINT(left),UINT(top),0,UINT(std::max(left,right)),UINT(std::max(top,bottom)),1});};
                // I-0127 (effect-measure.flag): the block's GPU time, a timestamp before the stage and one after the 0x31 layer's composite.
                const bool effect_measured=input.effects->measure;const uint32_t query_slot=frames_.CurrentIndex();
                // The final review's L4: a diagnostic never fails the frame; its objects' failure is kept for the device (no retry a frame).
                if(effect_measured && !effect_queries_ && !effect_query_failed_){effect_query_failed_=true;
                    D3D12_QUERY_HEAP_DESC q{D3D12_QUERY_HEAP_TYPE_TIMESTAMP,2*max_frame_slots,0};ComPtr<ID3D12QueryHeap> heap;UINT64 frequency{};
                    if(SUCCEEDED(gpu_.device->CreateQueryHeap(&q,IID_PPV_ARGS(&heap))) && SUCCEEDED(gpu_.queue->GetTimestampFrequency(&frequency)) && frequency)
                        try{effect_query_readback_=NewBuffer(gpu_.device.Get(),16*max_frame_slots,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
                            {const std::lock_guard hold(effects_lock_);effect_query_frequency_=frequency;}   // step review L2: read under the lock
                            effect_queries_=heap;effect_query_failed_=false;}
                        catch(const std::exception&){effect_query_readback_.Reset();}}
                const bool effect_timed=effect_measured && effect_queries_ && query_slot<max_frame_slots;
                if(effect_timed)list->EndQuery(effect_queries_.Get(),D3D12_QUERY_TYPE_TIMESTAMP,2*query_slot);
                // I-0140: pass 0x21 first (the DLC's kind 1 effects: the burning debris' flames at the Sister Friede bonfire, i140-e5),
                // its opaque draws writing the depth the 0x29 rows then test; 0x20 is not reached by the sub-pass chooser.
                static const std::vector<int32_t> passes{0x21,0x29,0x2D,0x31};
                try{effects_.Encode(gpu_.device.Get(),list,*input.effects,passes,effect_targets,viewport,scissor,frames_.CurrentIndex(),
                    [this](uint64_t bytes,bool zero){const auto range=frame_uploads_.Allocate(gpu_.device.Get(),bytes,UploadArena::alignment,zero);return EffectStage::UploadSink::Memory{range.cpu,range.gpu,range.resource,range.offset};});
                    for(auto& retired:effects_.TakeRetired())slot.kept.push_back(std::move(retired));}   // E07: released records' buffers, held to completion
                catch(const std::exception& e){effects_failure_=e.what();}
                run_tail();   // I-0140 tranche 4: no pass after 0x21 drew (the plan key's passes)
                if(!effect_depth_prepared && effect_depth_frame){prepare_effect_depth();effect_uses("effects_after_depth");}   // I-0140: no later pass drew (the plan key's copies)
                rows_until(UINT64_MAX);   // the rows keyed after every 0x29 statement, or all of them when Encode stopped early
                close_draw_scan(0);if(draw_scanned)EndEffectDrawScan(list,slot,input.serial);   // tranche 30 (31: the pass 0 closes it)
                if(effect_copy_pass && !effect_copied)effect_targets.before_pass(effect_copy_pass);
                // Step review M2: the layer's passes the plan key promises, even when Encode stopped before 0x31 (a cleared layer
                // composites as nothing).
                if(layer_frame && !layer_cleared)effect_targets.before_pass(0x31);
                // I-0113 tranche 17 (sub 0x32, 140102A80): the layer into the lit HDR (colour ONE / SRC_ALPHA, alpha ZERO / ONE), when it
                // was cleared for this frame's 0x31 draws; the half depth back to DEPTH_WRITE, the layer back to RENDER_TARGET.
                if(layer_frame){
                    if(layer_cleared){Transition(list,layer.color.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                        // The bilateral reads the main depth (block+0x18: the HDR group's depth, not the copy) and the half depth.
                        const bool bilateral=input.effects->layer_bilateral;auto* main_depth=view_->depth.Get();
                        const auto main_read=D3D12_RESOURCE_STATE_DEPTH_READ|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
                        if(bilateral)Transition(list,main_depth,D3D12_RESOURCE_STATE_DEPTH_WRITE,main_read);
                        if(graph){graph->Pass("effect_layer_composite",PassKind::Effect);graph->Use(layer.color.Get(),ResourceAccess::Read,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                            if(bilateral){graph->Use(main_depth,ResourceAccess::Read,main_read);graph->Use(layer.depth.Get(),ResourceAccess::Read,layer_read);}
                            graph->Use(hdr,ResourceAccess::ReadWrite,D3D12_RESOURCE_STATE_RENDER_TARGET);}
                        ID3D12DescriptorHeap* heaps[]{layer.table.Get()};list->SetDescriptorHeaps(1,heaps);list->SetGraphicsRootSignature(layer.root.Get());
                        list->SetPipelineState(bilateral?layer.bilateral.Get():layer.composite.Get());list->OMSetRenderTargets(1,&forward[0],FALSE,nullptr);
                        list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);list->SetGraphicsRootDescriptorTable(0,layer_table(1));
                        if(bilateral){   // 140102A80's cb1 rows 0..5: the taps (-1,-1), (+1,-1), (-1,+1), (+1,+1) over the half size, the full size, the depth's terms
                            const float hw=float(layer.width),hh=float(layer.height),n=input.camera.near_plane,f=input.camera.far_plane,range=f-n;
                            const float p33=f/range,p43=-f*n/range;   // 14004B070's projection rows [0x28] and [0x38], as scene_view's
                            const float rows[24]{-1/hw,-1/hh,1,1, 1/hw,-1/hh,1,1, -1/hw,1/hh,1,1, 1/hw,1/hh,1,1,
                                float(RenderWidth()),float(RenderHeight()),0,1, p33,p43/range,range,1};
                            list->SetGraphicsRoot32BitConstants(1,24,rows,0);}
                        list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);list->DrawInstanced(3,1,0,0);
                        if(bilateral)Transition(list,main_depth,main_read,D3D12_RESOURCE_STATE_DEPTH_WRITE);
                        Transition(list,layer.color.Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);}
                    Transition(list,layer.depth.Get(),layer_read,D3D12_RESOURCE_STATE_DEPTH_WRITE);}
                if(effect_timed){list->EndQuery(effect_queries_.Get(),D3D12_QUERY_TYPE_TIMESTAMP,2*query_slot+1);
                    list->ResolveQueryData(effect_queries_.Get(),D3D12_QUERY_TYPE_TIMESTAMP,2*query_slot,2,effect_query_readback_.Get(),16ull*query_slot);
                    slot.kept.push_back(HoldObject(effect_queries_.Get()));slot.kept.push_back(HoldResource(effect_query_readback_.Get()));
                    slot.completions.push_back([this,readback=effect_query_readback_,query_slot]{void* p{};const D3D12_RANGE range{16ull*query_slot,16ull*query_slot+16};
                        if(FAILED(readback->Map(0,&range,&p)))return;uint64_t at[2]{};std::memcpy(at,static_cast<const uint8_t*>(p)+16ull*query_slot,16);
                        const D3D12_RANGE none{};readback->Unmap(0,&none);if(at[1]<=at[0])return;
                        const uint64_t ticks=at[1]-at[0];const std::lock_guard hold(effects_lock_);auto& w=effects_window_;
                        ++w.gpu_frames;w.gpu_ticks+=ticks;w.gpu_ticks_max=std::max(w.gpu_ticks_max,ticks);
                        ++w.gpu_histogram[std::min<uint64_t>(ticks*100000/std::max<uint64_t>(effect_query_frequency_,1),127)];});}
                // I-0113 tranche 29 (effect-scan.flag): on a frame whose passes after 0x29 sample the copy, the lit HDR after them and
                // the copy as the per-quad copies left it (both in the states the effect passes declare); after the effect block's end
                // timestamp (the final review's L6; stage 0 runs inside the block, where the copy is made, so effect-measure's GPU time
                // includes its readback when both flags are on).
                if(input.effects->scan && effect_copy_pass){RecordEffectScan(list,slot,hdr,D3D12_RESOURCE_STATE_RENDER_TARGET,1,uint32_t(effect_copy_pass),input.serial);
                    if(copy_rect_frame)RecordEffectScan(list,slot,copy.Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,2,uint32_t(effect_copy_pass),input.serial);}
                {const std::lock_guard hold(effects_lock_);effects_stats_=effects_.Statistics();effects_last_failure_=effects_failure_.empty()?effects_stats_.last_failure:effects_failure_;
                    // I-0127 (step review L3): a frame Record ran for only.
                    if(effect_measured && effects_stats_.measure.recorded){const auto& f=effects_stats_.measure;auto& w=effects_window_;++w.frames;w.draws+=f.draws;w.draws_max=std::max<uint64_t>(w.draws_max,f.draws);
                        w.statements+=f.statements;w.pipeline_changes+=f.pipeline_changes;w.arena_bytes+=f.arena_bytes;w.arena_max=std::max(w.arena_max,f.arena_bytes);
                        w.cpu_ticks+=f.ticks;w.cpu_ticks_max=std::max(w.cpu_ticks_max,f.ticks);w.callback_ticks+=f.callback_ticks;
                        static const uint64_t qpc=[]{LARGE_INTEGER q{};QueryPerformanceFrequency(&q);return uint64_t(q.QuadPart);}();
                        ++w.cpu_histogram[std::min<uint64_t>(f.ticks*100000/std::max<uint64_t>(qpc,1),127)];
                        for(uint32_t k=0;k<EffectCallKinds;++k){w.calls[k]+=f.calls[k];w.repeats[k]+=f.repeats[k];}}
                    if(effects_last_failure_.empty() && !effect_layer_failure_.empty())effects_last_failure_="effect layer: "+effect_layer_failure_;}   // step review L1: 0x31 at full size then
                if(row_failure)std::rethrow_exception(row_failure);
            }
            Transition(list,hdr,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }
        // I-0140 (gap 23): the TAA's fade mask, once the forward and effect rows are drawn (the depth as they left it).
        if(taa_fade_mask_pass){slot.kept.push_back(HoldResource(taa_fade_mask_.Get()));
            EncodeTaaFadeMask(list,input,FrameAddressesOf(frame_blocks,NativeModelPass::Forward),uploads,viewport,scissor);}
        if(sample_hdr){const auto path=std::filesystem::path(pixel_file);
            RecordPixels(list,slot,view_->light_targets[3].Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,(path.parent_path()/(path.stem().wstring()+L"-hdr.rgba16f")).wstring(),
                "native lit HDR evidence write",8,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            // I-0126 R7d: the bias check's sums (rgb) and frames (a) at the render extent, beside the picture.
            if(rt_gi_mean_now_)RecordPixels(list,slot,rt_gi_mean_target_.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,(path.parent_path()/(path.stem().wstring()+L"-gi-mean.rgba32f")).wstring(),
                "native gi mean evidence write",16,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);}
        // 1186 velocity (groups 0x33/0x34) after the forward and effect steps, ahead of the post chain (NATIVE-PASS-ORDER 11b;
        // I-0126 R7c: on NRD's frames its rows drawn before the lighting).
        // cD4 (G1, G2, X3): the witness requests are taken now; the readbacks are this slot's, read at its release.
        const auto slot_index=frames_.CurrentIndex();
        SlotReadbacks* readbacks=sample_pixels || sample_velocity?&Readbacks(slot_index,sample_velocity):nullptr;
        if(readbacks){slot.kept.push_back(HoldResource(readbacks->backbuffer.Get()));
            if(sample_velocity){slot.kept.push_back(HoldResource(readbacks->velocity.Get()));slot.kept.push_back(HoldResource(readbacks->motion.Get()));}}
        if(input.velocity){if(!rt_nrd_now_)EncodeNativeVelocityRows(list,input,FrameAddressesOf(frame_blocks,NativeModelPass::Velocity),uploads,viewport,scissor);
            EncodeNativeVelocity(list,input,viewport,scissor,sample_velocity?readbacks:nullptr);}   // I-0126 R7c: on NRD's frames the rows drawn before the lighting
        // I-0111 (UPSCALER-DLSS-READY §0.4, §3.1): DLSS between the velocity and the YEBIS passes, on the lit HDR (linear, not
        // pre-exposed); its output, at the presentation's extent, is the post chain's source (tone map, glare, composition). Every
        // input in a compute-readable state, the output in unordered access; Streamline is told those states and leaves them so.
        if(dlss_frame){
            auto* hdr=view_->light_targets[3].Get();auto* depth=view_->depth.Get();auto* motion=motion_vectors_.Get();auto* out=dlss_output_.Get();
            slot.kept.push_back(HoldResource(out));
            const auto psr=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,npsr=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            const auto colour_read=psr|npsr,depth_read=D3D12_RESOURCE_STATE_DEPTH_READ|npsr;
            Transition(list,hdr,psr,colour_read);Transition(list,depth,D3D12_RESOURCE_STATE_DEPTH_WRITE,depth_read);
            Transition(list,motion,D3D12_RESOURCE_STATE_RENDER_TARGET,npsr);Transition(list,out,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            if(graph){graph->Pass("dlss",PassKind::PostProcess);graph->Use(hdr,ResourceAccess::Read,colour_read);graph->Use(depth,ResourceAccess::Read,depth_read);
                graph->Use(motion,ResourceAccess::Read,npsr);graph->Use(out,ResourceAccess::Write,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                if(rr_now_)for(const auto& target:rr_targets_)graph->Use(target.Get(),ResourceAccess::Read,kRrGuideState);}   // I-0123: RR's guides
            // D-0242: DS3's exposure (the tone map's) rather than DLSS's own estimate; 1 on a frame without a tone-map input (its
            // composition does not expose the HDR). Always given, as sl.dlss fixes its exposure mode at its instance's creation (step
            // review M1).
            // I-0123: Ray Reconstruction reads no exposure (its guide §3.7): none made on its frames.
            // I-0123 step review L1, L2: Ray Reconstruction when this frame chose it and its guides did not latch it off.
            const bool rr_eval=rr_now_ && rr.load(std::memory_order_relaxed);
            auto* const exposure=rr_eval?nullptr:RecordDlssExposure(list,input.tonemap?input.tonemap->exposure:1.0f,input.tonemap && input.tonemap->auto_exposure);
            if(exposure)slot.kept.push_back(HoldResource(exposure));   // its own copy and barriers (RecordDlssExposure), outside the frame graph
            streamline::DlssFrame f{};f.frame=uint32_t(input.serial);f.mode=dlss_mode_;f.exposure=exposure;f.exposure_state=npsr;
            f.color=hdr;f.depth=depth;f.motion=motion;f.output=out;
            f.color_state=colour_read;f.depth_state=depth_read;f.motion_state=npsr;f.output_state=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            {const auto d=hdr->GetDesc(),o=out->GetDesc();f.render_width=uint32_t(d.Width);f.render_height=d.Height;f.output_width=uint32_t(o.Width);f.output_height=o.Height;}   // final review L8: the resources' own extents
            f.jitter_x=input.camera.jitter_x;f.jitter_y=input.camera.jitter_y;
            // Final review M3: the history restarts on the velocity's cut rule and whenever the previous world frame was not
            // evaluated (the first, after a menu that hid the world, frames without lighting or velocity).
            // I-0123: the history restarts too when the previous evaluated frame ran the other feature.
            f.reset=input.velocity->reset || dlss_last_serial_+1!=input.serial || dlss_last_rr_!=rr_eval;
            // The unjittered camera (BuildSceneView's lens, as the velocity's), row-vector as the engine's; the previous clip from the
            // velocity's history (the cut rule's reset gives the current one).
            f.view_to_clip=view.projection;
            {VelocityMatrix p{},inverse{};for(unsigned i=0;i<16;++i)p[i]=view.projection[i];
                if(VelocityInverse(p,inverse))for(unsigned i=0;i<16;++i)f.clip_to_view[i]=float(inverse[i]);
                for(unsigned i=0;i<16;++i)p[i]=input.velocity->clip_to_previous_clip[i];
                if(VelocityInverse(p,inverse))for(unsigned i=0;i<16;++i)f.previous_clip_to_clip[i]=float(inverse[i]);}
            f.clip_to_previous_clip=input.velocity->clip_to_previous_clip;
            const auto& pose=input.camera.camera_pose;
            f.right={pose[0],pose[1],pose[2]};f.up={pose[4],pose[5],pose[6]};f.forward={pose[8],pose[9],pose[10]};f.position={pose[12],pose[13],pose[14]};
            f.near_plane=input.camera.near_plane;f.far_plane=input.camera.far_plane;f.vertical_fov=input.camera.vertical_fov;
            f.aspect=input.camera.explicit_aspect?input.camera.explicit_aspect:float(input.camera.width)/float(input.camera.height);
            // I-0123 RR-a: Ray Reconstruction with its guides (written by the lighting, EncodeRrGuides) and the camera's world-to-view rows.
            if(rr_eval){f.rr=true;f.albedo=rr_targets_[0].Get();f.specular_albedo=rr_targets_[1].Get();f.normal_roughness=rr_targets_[2].Get();
                f.before_transparency=rr_targets_[3].Get();f.specular_hit_distance=rr_hit_written_?rr_targets_[4].Get():nullptr;f.guide_state=kRrGuideState;   // R6c: the hit distance
                VelocityMatrix p{},inverse{};for(unsigned i=0;i<16;++i){p[i]=pose[i];f.view_to_world[i]=pose[i];}
                if(VelocityInverse(p,inverse))for(unsigned i=0;i<16;++i)f.world_to_view[i]=float(inverse[i]);}
            std::string reason;GpuZoneBegin(list,GpuZone::Dlss);const bool evaluated=streamline::EvaluateDlss(list,f,reason);if(evaluated)GpuZoneEnd(list,GpuZone::Dlss);else GpuZoneCancel(GpuZone::Dlss);
            // I-0132 R12c (Thomas, 2026-11-11 01:05: the sharpening kept with DLSS): Thomas's sharpening on DLSS's output, in place, inside
            // this pass (its target stays in UNORDERED_ACCESS: no new use for the frame graph); the stage opened as for the TAA, a failure
            // latched off with its row.
            if(evaluated && input.sharpen>=1e-3f && !nis_frame && !dlss_sharpen_failed_ && TaaReady(input.serial)){   // I-0132 R12f: NIS's instead when it runs
                try{taa_.RecordSharpen(list,out,input.tonemap?input.tonemap->exposure:1.0f,input.sharpen,slot_index);slot.kept.push_back(HoldResource(taa_.SharpenScratch()));
                    const auto n=taa_.SharpenFrames();
                    if(upscaler_sink_ && (n==1 || n%3600==0)){char row[200]{};sprintf_s(row,"{\"event\":\"native_dlss_sharpen_frames\",\"frames\":%llu,\"serial\":%llu,\"sharpen\":%.3f}\n",
                        static_cast<unsigned long long>(n),static_cast<unsigned long long>(input.serial),double(input.sharpen));upscaler_sink_(row);}}
                catch(const std::exception& e){if(!dlss_sharpen_failed_.exchange(true) && upscaler_sink_){std::string why=std::string(e.what()).substr(0,200);for(auto& ch:why)if(ch=='"' || ch=='\\' || ch<32)ch=' ';
                    char row[320]{};sprintf_s(row,"{\"event\":\"native_dlss_sharpen_latched_off\",\"serial\":%llu,\"reason\":\"%s\"}\n",static_cast<unsigned long long>(input.serial),why.c_str());upscaler_sink_(row);}}}
            // I-0131 R10b: DLSS-G's depth and motion, DLSS's own with its frame token and constants, copied by Streamline on this list
            // in their states here (eOnlyValidNow: the next frame reuses them); the HUD-less colour follows before the UI.
            if(evaluated && fg_frame){std::string fg_reason;
                if(streamline::TagFrameGenerationDepth(list,uint32_t(input.serial),depth,depth_read,motion,npsr,f.render_width,f.render_height,fg_reason))fg_depth_serial_=input.serial;
                else FailFrameGeneration(input.serial,fg_reason);}
            Transition(list,motion,npsr,D3D12_RESOURCE_STATE_RENDER_TARGET);Transition(list,depth,depth_read,D3D12_RESOURCE_STATE_DEPTH_WRITE);
            // I-0111 tranche 3 (final review L3): the post chain reads DLSS's output itself (no copy back into the lit HDR, which is not
            // read again this frame). A refused evaluation leaves this one frame its previous output, or black on a new output's first
            // evaluation (final review H1: a pass only on a refusal would be outside the plan key); DLSS is off from the next.
            Transition(list,hdr,colour_read,D3D12_RESOURCE_STATE_RENDER_TARGET);Transition(list,out,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,psr);
            if(evaluated){dlss_last_serial_=input.serial;dlss_last_rr_=rr_eval;const auto n=dlss_frames_.fetch_add(1,std::memory_order_relaxed)+1;
                const auto rr_n=rr_eval?rr_frames_.fetch_add(1,std::memory_order_relaxed)+1:rr_frames_.load(std::memory_order_relaxed);
                if(upscaler_sink_ && (n==1 || n%3600==0 || (rr_eval && rr_n==1))){char row[240]{};sprintf_s(row,"{\"event\":\"native_dlss_frames\",\"frames\":%llu,\"rr_frames\":%llu,\"serial\":%llu,\"width\":%u,\"height\":%u}\n",
                    static_cast<unsigned long long>(n),static_cast<unsigned long long>(rr_n),static_cast<unsigned long long>(input.serial),f.output_width,f.output_height);upscaler_sink_(row);}}
            else if(rr_eval && streamline::DlssReady())LatchRrOff(input.serial,reason);   // I-0123: RR's refusal leaves DLSS (Streamline latched RR only)
            else LatchDlssOff(input.serial,reason);
            if(depth_out_pass)EncodeDepthOut(list,input.camera.jitter_x,input.camera.jitter_y);}   // §3.7
        // I-0132 R11j (R11-PLAN §11): the TAA where DLSS would run, on the same inputs in the same states (the frame graph's), its output
        // DLSS's target; its history pair the stage's own, held by this slot. The history restarts on the velocity's cut rule and whenever
        // the previous world frame was not resolved by it (the first, after a menu, a frame without lighting or velocity, DLSS's frames).
        if(taa_frame){
            auto* hdr=view_->light_targets[3].Get();auto* depth=view_->depth.Get();auto* motion=motion_vectors_.Get();auto* out=dlss_output_.Get();
            slot.kept.push_back(HoldResource(out));
            const auto psr=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,npsr=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            const auto colour_read=psr|npsr,depth_read=D3D12_RESOURCE_STATE_DEPTH_READ|npsr;
            Transition(list,hdr,psr,colour_read);Transition(list,depth,D3D12_RESOURCE_STATE_DEPTH_WRITE,depth_read);
            Transition(list,motion,D3D12_RESOURCE_STATE_RENDER_TARGET,npsr);Transition(list,out,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            if(graph){graph->Pass("taa",PassKind::PostProcess);graph->Use(hdr,ResourceAccess::Read,colour_read);graph->Use(depth,ResourceAccess::Read,depth_read);
                graph->Use(motion,ResourceAccess::Read,npsr);graph->Use(out,ResourceAccess::Write,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                if(taa_base_copied)graph->Use(taa_base_.Get(),ResourceAccess::Read,colour_read);   // I-0113 tranche 39
                if(taa_fade_mask_pass && taa_base_copied)graph->Use(taa_fade_mask_.Get(),ResourceAccess::Read,colour_read);}   // I-0140 (its second step review's L2: as p.fade_mask)
            TaaParams p{};p.base=taa_base_copied?taa_base_.Get():nullptr;p.reactive_view=input.taa_reactive_view;p.fade_mask=taa_fade_mask_pass && taa_base_copied?taa_fade_mask_.Get():nullptr;p.jitter_x=input.camera.jitter_x;p.jitter_y=input.camera.jitter_y;p.exposure=input.tonemap?input.tonemap->exposure:1.0f;p.kernel=input.taa_kernel;   // I-0138 tranche 9: taa-kernel.txt's (0: the stage's 2.29)
            p.reset=input.velocity->reset || taa_last_serial_+1!=input.serial || input.taa_reset_always;p.own_reset=input.taa_reset_always;p.sharpen=nis_frame?0.0f:input.sharpen;   // I-0132 R12f: NIS's instead when it runs; I-0138 tranche 9: taa-reset-always.flag (a test tool)
            GpuZoneBegin(list,GpuZone::Taa);const bool reset=taa_.Record(list,hdr,depth,motion,out,p,slot_index);GpuZoneEnd(list,GpuZone::Taa);
            // I-0140 (its final review's L3): the frames the mask was read on, its first one told with that frame's rows.
            if(p.fade_mask && taa_fade_frames_.fetch_add(1,std::memory_order_relaxed)==0 && upscaler_sink_){char row[160]{};
                sprintf_s(row,"{\"event\":\"native_taa_fade_mask\",\"first_serial\":%llu,\"rows\":%llu}\n",static_cast<unsigned long long>(input.serial),
                    static_cast<unsigned long long>(input.fade_mask_draws.size()));upscaler_sink_(row);}
            for(auto* history:taa_.History())if(history)slot.kept.push_back(HoldResource(history));
            Transition(list,motion,npsr,D3D12_RESOURCE_STATE_RENDER_TARGET);Transition(list,depth,depth_read,D3D12_RESOURCE_STATE_DEPTH_WRITE);
            Transition(list,hdr,colour_read,D3D12_RESOURCE_STATE_RENDER_TARGET);Transition(list,out,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,psr);
            taa_last_serial_=input.serial;const auto n=taa_.Frames();
            if(upscaler_sink_ && (n==1 || n%3600==0)){char row[384]{};sprintf_s(row,"{\"event\":\"native_taa_frames\",\"frames\":%llu,\"resets\":%llu,\"reactive_frames\":%llu,\"view_frames\":%llu,\"fade_frames\":%llu,\"fade_rows\":%llu,\"reset\":%s,\"serial\":%llu,\"width\":%u,\"height\":%u,\"sharpen\":%.3f}\n",
                static_cast<unsigned long long>(n),static_cast<unsigned long long>(taa_.Resets()),static_cast<unsigned long long>(taa_.ReactiveFrames()),static_cast<unsigned long long>(taa_.ViewFrames()),static_cast<unsigned long long>(taa_fade_frames_.load(std::memory_order_relaxed)),static_cast<unsigned long long>(taa_fade_rows_.load(std::memory_order_relaxed)),reset?"true":"false",static_cast<unsigned long long>(input.serial),presenter_.width,presenter_.height,p.sharpen);upscaler_sink_(row);}}
        // Same native colour composition as the existing first-zone envelope.
        // No captured irradiance, lighting default or original GPU result.
        const auto index=presenter_.swapchain->GetCurrentBackBufferIndex();auto* buffer=presenter_.buffers[index].Get();
        // Lit HDR (left readable by the lighting), DLSS's output on a DLSS frame (I-0111) or, without lighting, material colour.
        auto* source=post_source?post_source:view_->colors[2].Get();
        if(!lit)Transition(list,source,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        // W3: with the gamma pass the composition and the UI draw into its intermediate (left a render target between frames),
        // and the back buffer is written by the pass; without it, straight into the back buffer as before.
        auto* output=display?display_.Intermediate():buffer;
        // I-0138 tranche 5: where the post chain draws the scene (scene_out: the 8-bit picture, or the output stage's float
        // picture when our tone map's HDR entry draws) and where the frame ends, the UI and the overlay drawn in place
        // (final_out: on an HDR10 chain always the float picture, a frame's 8-bit picture being copied into it before the UI).
        auto* const scene_out=hdr_scene?output_.Scene():output;auto* const final_out=hdr_out?output_.Scene():output;
        if(hdr_out)slot.kept.push_back(HoldResource(final_out));
        if(!display)Transition(list,buffer,D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_RENDER_TARGET);
        // W10 DF-4 (W10-NATIVE-PASSES 9): the depth of field: the depth factor into the lit HDR's alpha (the stage's pre draw, from the
        // depth's plane 0 and the DoF table), the source chain, the levels and the composite into the stage's DoF scene, copied back into
        // the lit HDR, which Alpha_R, the glare and the tone map then read as the original reads its DoF output. The three passes are
        // declared whatever happens (a failure keeps the frame's pass list; the copy is skipped and the failure kept for the session).
        if(dof){
            const auto& plan=input.yebis_dof->plan;const auto& programs=*input.yebis_dof->programs;
            const auto depth_read=D3D12_RESOURCE_STATE_DEPTH_READ|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            const auto sink=[this](uint64_t bytes){const auto range=frame_uploads_.Allocate(gpu_.device.Get(),bytes);return GlareConstantMemory{range.cpu,range.gpu};};
            Transition(list,source,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);
            DepthPlaneTransition(list,post_depth,D3D12_RESOURCE_STATE_DEPTH_WRITE,depth_read);
            if(graph){graph->Pass("dof_factor",PassKind::PostProcess);graph->Use(source,ResourceAccess::ReadWrite,D3D12_RESOURCE_STATE_RENDER_TARGET);
                graph->Use(post_depth,ResourceAccess::Read,depth_read,{0,1});}
            std::string reason;
            const bool factor=dof_stage_.RecordPre(list,source,post_depth,post_generation,plan,programs,sink,reason,nullptr,&input.yebis_dof->table);
            DepthPlaneTransition(list,post_depth,depth_read,D3D12_RESOURCE_STATE_DEPTH_WRITE);
            Transition(list,source,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            if(graph){graph->Pass("dof",PassKind::PostProcess);graph->Use(source,ResourceAccess::Read,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);}
            ID3D12Resource* scene_dof=factor?dof_stage_.Record(list,source,post_generation,plan,programs,sink,reason):nullptr;
            if(!scene_dof){dof_failure_=reason;dof_failed_=true;}
            Transition(list,source,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_DEST);
            if(graph){graph->Pass("dof_copy",PassKind::Effect);graph->Use(source,ResourceAccess::Write,D3D12_RESOURCE_STATE_COPY_DEST);}
            if(scene_dof){Transition(list,scene_dof,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);
                list->CopyResource(source,scene_dof);
                Transition(list,scene_dof,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);}
            Transition(list,source,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);}
        // W10 LS-1: with the shafts (or, I-0138 tranche 8c, the motion blur) in the glare plan, Alpha_R writes the depth's mask into
        // the lit HDR's alpha before the glare's source chain and the motion blur read it (the original draws it into the DoF output;
        // natively, without DoF, the lit HDR); a plan without glare draws (a skipped or refused shape) still draws it.
        if(alpha_r){
            const auto depth_read=D3D12_RESOURCE_STATE_DEPTH_READ|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            Transition(list,source,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);
            DepthPlaneTransition(list,post_depth,D3D12_RESOURCE_STATE_DEPTH_WRITE,depth_read);
            if(graph){graph->Pass("alpha_r",PassKind::PostProcess);graph->Use(source,ResourceAccess::ReadWrite,D3D12_RESOURCE_STATE_RENDER_TARGET);
                graph->Use(post_depth,ResourceAccess::Read,depth_read,{0,1});}
            std::string reason;
            if(!glare_family_.RecordPre(list,source,post_depth,post_generation,input.yebis_glare->plan,*input.yebis_glare->programs,[this](uint64_t bytes){
                    const auto range=frame_uploads_.Allocate(gpu_.device.Get(),bytes);return GlareConstantMemory{range.cpu,range.gpu};},reason,input.yebis_glare->film_grain.get())){
                glare_family_failed_=true;}
            DepthPlaneTransition(list,post_depth,depth_read,D3D12_RESOURCE_STATE_DEPTH_WRITE);
            Transition(list,source,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);}
        // W10 MB-4 (W10-NATIVE-PASSES 10): the motion blur after Alpha_R (the original's order: depth of field, Alpha_R, motion blur, glare):
        // the velocity target reduced to the half-size object velocity, the camera velocity into the stage's internal velocity (the depth's
        // plane 0 and the object velocity), the blur passes from the lit HDR, the last pass's surface copied back into the lit HDR, which the
        // glare and the tone map then read. The passes are declared whatever happens; a failure skips the copy and is kept for the session.
        if(motionblur){
            const auto& plan=input.yebis_motionblur->plan;const auto& programs=*input.yebis_motionblur->programs;
            EncodeVelocityHalf(list);
            const auto depth_read=D3D12_RESOURCE_STATE_DEPTH_READ|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            const auto sink=[this](uint64_t bytes){const auto range=frame_uploads_.Allocate(gpu_.device.Get(),bytes);return GlareConstantMemory{range.cpu,range.gpu};};
            DepthPlaneTransition(list,post_depth,D3D12_RESOURCE_STATE_DEPTH_WRITE,depth_read);
            if(graph){graph->Pass("motion_blur",PassKind::PostProcess);graph->Use(source,ResourceAccess::Read,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                graph->Use(post_depth,ResourceAccess::Read,depth_read,{0,1});graph->Use(velocity_half_.Get(),ResourceAccess::Read,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);}
            std::string reason;
            // Review: the blur's depth test reads Alpha_R's alpha; a failed Alpha_R this frame skips the blur (the copy too).
            const bool alpha_written=!glare_family_failed_;
            ID3D12Resource* blurred=alpha_written?motionblur_stage_.Record(list,source,post_generation,plan,programs,sink,reason,nullptr,post_depth,velocity_half_.Get()):nullptr;
            if(!blurred && alpha_written){motionblur_failure_=reason;motionblur_failed_=true;}
            DepthPlaneTransition(list,post_depth,depth_read,D3D12_RESOURCE_STATE_DEPTH_WRITE);
            Transition(list,source,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_DEST);
            if(graph){graph->Pass("motion_blur_copy",PassKind::Effect);graph->Use(source,ResourceAccess::Write,D3D12_RESOURCE_STATE_COPY_DEST);}
            if(blurred){Transition(list,blurred,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);
                list->CopyResource(source,blurred);
                Transition(list,blurred,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);}
            Transition(list,source,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);}
        // I-0132 R11h (R11-PLAN section 9 step 1): the automatic exposure from the HDR the glare and the tone map read, its factor the
        // tone map's t2; snapped at a cut (the velocity's reset: a load, a teleport, a camera cut). A failure latches it off (the area's own exposure then, as no-auto-exposure.flag) and fails the start check.
        ID3D12Resource* adapted=nullptr;
        if(auto_exposure_frame){
            // The step review's H1: a compute read (NON_PIXEL_SHADER_RESOURCE alone, so the composition's PIXEL_SHADER_RESOURCE is an exact
            // transition for the planner), the hand transition before the declaration (the shadow takes the hand state at Use).
            const auto psr=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,read=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            Transition(list,source,psr,read);
            if(graph){graph->Pass("auto_exposure",PassKind::PostProcess);graph->Use(source,ResourceAccess::Read,read);}   // declared whatever happens (the plan key's bit 55)
            try{std::string reason;
                if(!auto_exposure_.IsOpen() && !auto_exposure_.Open(gpu_.device.Get(),reason))throw std::runtime_error(reason);
                auto params=input.tonemap->auto_params;
                params.reset=params.reset || (input.velocity && input.velocity->reset);
                auto_exposure_.Record(list,source,frames_.CurrentIndex(),input.tonemap->post_record,params,[this](uint64_t bytes){   // the record's own block (a look does not move the exposure: model_frame.h)
                    const auto range=frame_uploads_.Allocate(gpu_.device.Get(),bytes);return AutoExposureConstantMemory{range.cpu,range.gpu};});
                adapted=auto_exposure_.Factor();
                const auto reading=auto_exposure_.LastReading();const std::lock_guard hold(auto_exposure_lock_);auto_exposure_reading_=reading;}
            catch(const std::exception& e){auto_exposure_failed_=true;adapted=nullptr;
                const std::lock_guard hold(auto_exposure_lock_);auto_exposure_failure_=e.what();}
            Transition(list,source,read,psr);}
        if(graph){graph->Pass("composition",PassKind::Composition);graph->Use(source,ResourceAccess::Read,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            graph->Use(scene_out,ResourceAccess::Write,D3D12_RESOURCE_STATE_RENDER_TARGET);}
        const auto target=display?display_.Target():CPU(presenter_.rtv.Get(),presenter_.stride,index);
        const auto scene_target=hdr_scene?output_.SceneTarget():target,final_target=hdr_out?output_.SceneTarget():target;   // I-0138 tranche 5
        if(tonemap){
            // W10 GL-3: the glare family first (its plan's draws from the lit HDR), its output the tone map's t1; a failure is kept
            // for the session and the tone map then runs with a black t1, as before the family existed.
            ID3D12Resource* glare_t1=nullptr;
            // I-0138 tranche 8f (its fixes' step review's L6): a camera cut is kept until a frame records the glare, so a cut on a frame
            // without glare draws still clears the afterimage's history (the library clears it on its display record's cut whatever
            // the family draws).
            if(input.velocity && input.velocity->reset)glare_cut_pending_=true;
            // I-0138 tranche 8c: a plan served without draws draws none, t1 black: the library skips the family at shape 0 or a glare
            // luminance not positive; tranche 8f: shapes 1 and 3 served, the others refused (GlareShapeGate).
            if(glare_matches() && !input.yebis_glare->plan.draws.empty()){
                std::string reason;
                if(open_glare()){
                    glare_t1=glare_family_.Record(list,source,post_generation,input.yebis_glare->plan,*input.yebis_glare->programs,[this](uint64_t bytes){
                        const auto range=frame_uploads_.Allocate(gpu_.device.Get(),bytes);return GlareConstantMemory{range.cpu,range.gpu};},reason,
                        input.yebis_glare->film_grain.get(),nullptr,nullptr,glare_cut_pending_);
                    if(!glare_t1){glare_family_failed_=true;}else glare_cut_pending_=false;}}
            // W10 GR: with the grading, the tone map draws into an 8-bit image of its stage and the LUT's draw takes it into the target.
            // I-0138 tranche 1: both draws our pass's (PostToneStage).
            {PostToneLut lut;if(grading)lut={input.yebis_grading->lut,input.yebis_grading->lut_format,input.yebis_grading->lut_generation};
                // I-0138 tranche 5: on a scene frame the HDR entry, one draw into the float scene (the LUT read in it).
                // I-0138 tranche 6: a candidate look's pipeline made at its first frame; a failure is said once and the frames are then
                // drawn with the record's own block (its curve and colour matrix; a neutral look's frame had its LUT left out when it
                // was prepared and stays without it).
                auto post=input.tonemap->post;
                if(const uint32_t look=(post.flags&kPostToneLookMask)>>kPostToneLookShift){std::string reason;
                    if(tone_look_failed_ || !post_tone_.OpenLook(look,hdr_scene,reason)){if(!tone_look_failed_){tone_look_failure_=reason;tone_look_failed_=true;}
                        const auto kept=post;post=input.tonemap->post_record;post.hdr_knee=kept.hdr_knee;post.glare_scale=kept.glare_scale;   // the bloom's row and the film grain kept too (I-0138 tranche 7)
                        for(size_t i=0;i<std::size(post.grain);++i)post.grain[i]=kept.grain[i];post.grain_seed=kept.grain_seed;}}
                post_tone_.Record(list,source,post_generation,scene_target,post,[this](uint64_t bytes){
                    const auto range=frame_uploads_.Allocate(gpu_.device.Get(),bytes);return PostToneConstantMemory{range.cpu,range.gpu};},glare_t1,glare_family_.Generation(),
                    adapted,grading?&lut:nullptr,hdr_scene);
                if(hdr_scene && grading && input.yebis_grading->lut_owner)slot.kept.push_back(input.yebis_grading->lut_owner);}
            if(grading && !hdr_scene){const auto& g=*input.yebis_grading;
                if(graph){graph->Pass("colour_grading",PassKind::PostProcess);graph->Use(output,ResourceAccess::Write,D3D12_RESOURCE_STATE_RENDER_TARGET);}
                post_tone_.RecordLut(list,target);
                if(g.lut_owner)slot.kept.push_back(g.lut_owner);
                // I-0113 gap 4 witness (world-pictures.flag): the tone map's output (the LUT draw's source, 8-bit) and the 16x16x16
                // volume it samples (R32G32B32A32, slice after slice), beside the presented picture.
                if(sample_hdr){const auto path=std::filesystem::path(pixel_file);constexpr auto rt=D3D12_RESOURCE_STATE_RENDER_TARGET,psr=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
                    RecordPixels(list,slot,post_tone_.Intermediate(),rt,(path.parent_path()/(path.stem().wstring()+L"-tonemap.rgba")).wstring(),"native tone-map evidence write",4,rt);
                    RecordPixels(list,slot,post_tone_.Volume(),psr,(path.parent_path()/(path.stem().wstring()+L"-lut.rgba32f")).wstring(),"native grading volume evidence write",16,psr);}}
            list->RSSetViewports(1,&out_viewport);list->RSSetScissorRects(1,&out_scissor);}
        else if(!world_shown){constexpr float black[4]{0,0,0,1};list->ClearRenderTargetView(target,black,0,nullptr);}   // M1
        else{
        list->OMSetRenderTargets(1,&target,FALSE,nullptr);
        ID3D12DescriptorHeap* composition_heaps[]{view_->bindings.Get()};list->SetDescriptorHeaps(1,composition_heaps);
        list->SetPipelineState(composition_pso_.Get());list->SetGraphicsRootSignature(composition_root_.Get());
        list->SetGraphicsRootDescriptorTable(0,GPU(view_->bindings.Get(),binding_stride_,composition_binding+(dlss_frame || taa_frame?2u:lit?1u:0u)));
        list->RSSetViewports(1,&out_viewport);list->RSSetScissorRects(1,&out_scissor);
        list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);list->DrawInstanced(3,1,0,0);}
        Transition(list,source,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);
        if(nis_frame){   // I-0132 R12f: NIS's sharpening on the 8-bit colour (its copy in, its own output, copied back), before the glow and the UI
            constexpr auto rt=D3D12_RESOURCE_STATE_RENDER_TARGET,src=D3D12_RESOURCE_STATE_COPY_SOURCE,dst=D3D12_RESOURCE_STATE_COPY_DEST,
                npsr=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,uav=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            auto* in=nis_in_.Get();auto* out=nis_out_.Get();slot.kept.push_back(HoldResource(in));slot.kept.push_back(HoldResource(out));
            // Each pass's transitions before its declaration (the shadow records the hand state at Use: its step review's M1).
            Transition(list,output,rt,src);Transition(list,in,npsr,dst);
            if(graph){graph->Pass("nis_copy",PassKind::PostProcess);graph->Use(output,ResourceAccess::Read,src);graph->Use(in,ResourceAccess::Write,dst);}
            list->CopyResource(in,output);Transition(list,in,dst,npsr);
            if(graph){graph->Pass("nis",PassKind::PostProcess);graph->Use(in,ResourceAccess::Read,npsr);graph->Use(out,ResourceAccess::Write,uav);}
            std::string reason;const bool sharpened=streamline::EvaluateNisSharpen(list,input.serial,in,uint32_t(npsr),out,uint32_t(uav),presenter_.width,presenter_.height,input.sharpen,reason);
            // A refused evaluation leaves this frame's colour unsharpened (its copy back skipped); NIS is latched off from the next (our
            // sharpening then).
            Transition(list,out,uav,src);Transition(list,output,src,dst);
            if(graph){graph->Pass("nis_back",PassKind::PostProcess);graph->Use(out,ResourceAccess::Read,src);graph->Use(output,ResourceAccess::Write,dst);}
            if(sharpened)list->CopyResource(output,out);
            Transition(list,output,dst,rt);Transition(list,out,src,uav);
            list->RSSetViewports(1,&out_viewport);list->RSSetScissorRects(1,&out_scissor);
            const auto n=sharpened?nis_frames_.fetch_add(1,std::memory_order_relaxed)+1:0;
            if(upscaler_sink_ && (!sharpened || n==1 || n%3600==0)){for(auto& c:reason)if(c=='"' || c=='\\' || c<32)c=' ';char row[320]{};
                sprintf_s(row,"{\"event\":\"native_nis\",\"frames\":%llu,\"serial\":%llu,\"sharpness\":%.3f,\"ok\":%s,\"reason\":\"%.200s\"}\n",
                    static_cast<unsigned long long>(n),static_cast<unsigned long long>(input.serial),double(input.sharpen),sharpened?"true":"false",reason.c_str());upscaler_sink_(row);}}
        if(glow){   // W6 GL-7 (GLOW-WAVE1-READY 5.5): the glow onto the 8-bit colour, after the composition and before the UI
            struct Recorder final:GlowRecorder {
                GraphShadow* graph;ID3D12Resource* output;
                Recorder(GraphShadow* g,ID3D12Resource* o):graph(g),output(o){}
                void Barrier(ID3D12GraphicsCommandList* l,ID3D12Resource* r,D3D12_RESOURCE_STATES from,D3D12_RESOURCE_STATES to) override {Transition(l,r,from,to);}
                void Pass(const char* name,ID3D12Resource* read,ID3D12Resource* write,bool blend) override {
                    if(!graph)return;graph->Pass(name,PassKind::PostProcess);graph->Use(read,ResourceAccess::Read,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                    graph->Use(write?write:output,blend?ResourceAccess::ReadWrite:ResourceAccess::Write,D3D12_RESOURCE_STATE_RENDER_TARGET);}
            } recorder(graph,scene_out);
            const GlowValues values{input.glow->blur_scale,input.glow->rgb_scale,input.glow->final_scale,input.glow->alpha_bits};
            // I-0138 tranche 5: on a scene frame onto the float scene, at the original's place (after the tone map) and in its
            // encoded values, as onto the 8-bit picture but without its clamp.
            glow_.Record(list,view_->colors[5].Get(),color_formats[5],view_->target_generation,scene_target,values,[this](uint64_t bytes){
                const auto range=frame_uploads_.Allocate(gpu_.device.Get(),bytes);return GlowConstants{range.cpu,range.gpu};},recorder,hdr_scene);
            // W6 image witness (world-pictures.flag): GB5 (the glow's input) and S (the extraction plus the composites up) beside
            // the presented picture; both stay render targets.
            if(sample_hdr){const auto path=std::filesystem::path(pixel_file);constexpr auto rt=D3D12_RESOURCE_STATE_RENDER_TARGET;
                RecordPixels(list,slot,view_->colors[5].Get(),rt,(path.parent_path()/(path.stem().wstring()+L"-gb5.rgba16f")).wstring(),"native GB5 evidence write",8,rt);
                RecordPixels(list,slot,glow_.Targets()[0],rt,(path.parent_path()/(path.stem().wstring()+L"-glow.r11g11b10")).wstring(),"native glow evidence write",4,rt);}
        }
        // I-0132 R11e (D-0262, no fallback): an arrival's held frame (counted) or a frame the rays did not serve, without
        // rt-fail-magenta.flag: its world black after the post chain, the UI drawn over it (DS3's loading screen and fade go on), before
        // the HUD-less colour; declared on every main-view frame with the rays so the frame graph's plan keeps one form.
        if(view_==&main_view_ && rt_asked.load(std::memory_order_relaxed)){
            if(graph){graph->Pass("rt_world_black",PassKind::Clear);graph->Use(scene_out,ResourceAccess::ReadWrite,D3D12_RESOURCE_STATE_RENDER_TARGET);}
            if(rt_world_black_){constexpr float black[4]{0,0,0,1};list->ClearRenderTargetView(scene_target,black,0,nullptr);}
            if(rt_hold_now_){rt_held_frames_.fetch_add(1,std::memory_order_relaxed);if(!rt_judged_now_)rt_held_unjudged_.fetch_add(1,std::memory_order_relaxed);}}   // the final review's M1: held without a judgement
        // I-0138 tranche 5: an HDR10 chain's frame whose picture is the 8-bit one (no tone map of ours, or its HDR entry given up):
        // copied into the float picture, where the frame goes on.
        if(hdr_out && !hdr_scene){
            Transition(list,output,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);   // before the declaration, as every pass here (the shadow reads the state at the Use)
            if(graph){graph->Pass("hdr_copy",PassKind::PostProcess);graph->Use(output,ResourceAccess::Read,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                graph->Use(final_out,ResourceAccess::Write,D3D12_RESOURCE_STATE_RENDER_TARGET);}
            RecordHdrCopy(list,out_viewport,out_scissor);
            Transition(list,output,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);}
        // I-0131 R10b (ProgrammingGuideDLSS_G §5.1): the HUD-less colour, the gamma pass drawn into its own target before the UI (the
        // back buffer's colour space and size); tagged once the depth and motion were (fg_depth_serial_), the Present then interpolates.
        if(fg_frame){
            if(!hdr_out)if(const auto staging=display_.RecordUploadIfDirty(list))slot.kept.push_back(HoldResource(staging.Get()));   // the curve before its first pass
            auto* const hudless=fg_hudless;
            Transition(list,final_out,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            if(graph){graph->Pass("fg_hudless",PassKind::PostProcess);graph->Use(final_out,ResourceAccess::Read,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                if(!hdr_out)graph->Use(display_.Curve(),ResourceAccess::Read,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                if(hudless)graph->Use(hudless,ResourceAccess::Write,D3D12_RESOURCE_STATE_RENDER_TARGET);}
            if(hudless && hdr_out)RecordHdrScene(list,input.serial,display_.HudlessTarget(),out_viewport,out_scissor);   // I-0138 tranche 5: the float picture before the UI, in the back buffer's colour space
            else if(hudless)display_.RecordPass(list,display_.HudlessTarget(),out_viewport,out_scissor);
            Transition(list,final_out,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);
            std::string fg_reason;
            if(!hudless)FailFrameGeneration(input.serial,"HUD-less colour target");
            else if(fg_depth_serial_==input.serial){
                if(streamline::TagFrameGenerationHudless(list,uint32_t(input.serial),hudless,D3D12_RESOURCE_STATE_RENDER_TARGET,presenter_.width,presenter_.height,fg_reason))fg_hudless_serial_=input.serial;
                else FailFrameGeneration(input.serial,fg_reason);}}
        if(primitive){
            const auto stencil=primitive_dsv_->GetCPUDescriptorHandleForHeapStart();
            if(input.initialize_primitive_stencil)list->ClearDepthStencilView(stencil,D3D12_CLEAR_FLAG_DEPTH|D3D12_CLEAR_FLAG_STENCIL,1,input.primitive_initial_stencil,0,nullptr);
            if(graph){graph->Pass("ui",PassKind::Ui);graph->Use(final_out,ResourceAccess::ReadWrite,D3D12_RESOURCE_STATE_RENDER_TARGET);
                graph->Use(primitive_depth_.Get(),ResourceAccess::ReadWrite,D3D12_RESOURCE_STATE_DEPTH_WRITE);}   // the primitive pass's own targets stay outside (sheet 6.13)
            if(ui->FloatTarget()==hdr_out)ui->Encode(list,final_target,stencil,true);   // I-0138 tranche 5: in place, on the float picture of an HDR10 chain
            else ui_format_skips_.fetch_add(1,std::memory_order_relaxed);   // a build made for the other chain format: not drawn this frame
        }
        if(overlay_frame){   // I-0132 R12a: our pages above the game's UI, before the gamma pass (the snapshot the plan key has)
            if(graph){graph->Pass("overlay",PassKind::Ui);graph->Use(final_out,ResourceAccess::ReadWrite,D3D12_RESOURCE_STATE_RENDER_TARGET);}
            RecordOverlay(list,final_target,slot,overlay_frame,overlay_,hdr_out);}
        if(display){   // W3: the gamma pass (GAMMA-COLOUR-PATH-READY 3.5), after the UI and before the witness readback
            // I-0113 gap 4 witness (world-pictures.flag): the intermediate the curve reads (graded, the glow and the UI drawn).
            if(sample_hdr){const auto path=std::filesystem::path(pixel_file);constexpr auto rt=D3D12_RESOURCE_STATE_RENDER_TARGET;
                // I-0138 tranche 5: on an HDR10 chain the output pass's source instead: the float picture, the UI drawn.
                if(!hdr_out)RecordPixels(list,slot,output,rt,(path.parent_path()/(path.stem().wstring()+L"-display.rgba")).wstring(),"native display evidence write",4,rt);
                else RecordPixels(list,slot,final_out,rt,(path.parent_path()/(path.stem().wstring()+L"-scene.rgba16f")).wstring(),"native HDR picture evidence write",8,rt);}
            if(!hdr_out)if(const auto staging=display_.RecordUploadIfDirty(list))slot.kept.push_back(HoldResource(staging.Get()));
            Transition(list,final_out,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            Transition(list,buffer,D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_RENDER_TARGET);
            if(graph){graph->Pass("display",PassKind::PostProcess);graph->Use(final_out,ResourceAccess::Read,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                if(!hdr_out)graph->Use(display_.Curve(),ResourceAccess::Read,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);graph->Use(buffer,ResourceAccess::Write,D3D12_RESOURCE_STATE_RENDER_TARGET);}
            if(hdr_out)RecordHdrScene(list,input.serial,CPU(presenter_.rtv.Get(),presenter_.stride,index),out_viewport,out_scissor);   // I-0138 tranche 5
            else display_.RecordPass(list,CPU(presenter_.rtv.Get(),presenter_.stride,index),out_viewport,out_scissor);
            Transition(list,final_out,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);
        }
        // 1122: the backbuffer readback and its CPU hash are a development witness; take them every 120th frame (and
        // whenever a pixel file is requested) instead of every frame. The presented image is unchanged.
        if(sample_pixels){
        Transition(list,buffer,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE);
        const auto image=Image(presenter_.width,presenter_.height,1,presenter_.format);D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};   // I-0138 tranche 5: the chain's format
        gpu_.device->GetCopyableFootprints(&image,0,1,0,&footprint,nullptr,nullptr,nullptr);
        D3D12_TEXTURE_COPY_LOCATION from{},to{};from.pResource=buffer;from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        to.pResource=readbacks->backbuffer.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint=footprint;
        if(graph){graph->Pass("readback",PassKind::Readback);graph->Use(buffer,ResourceAccess::Read,D3D12_RESOURCE_STATE_COPY_SOURCE);}
        list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        Transition(list,buffer,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_PRESENT);
        }else Transition(list,buffer,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PRESENT);
        if(graph){graph->Pass("present",PassKind::Present);graph->Use(buffer,ResourceAccess::Read,D3D12_RESOURCE_STATE_PRESENT);
            active_graph_shadow=nullptr;FinishGraphShadow(*graph);}
        // I-0129 R9a: the probes' answers to their states once the slot's work completed (its readback held until then).
        if(rt_probe_pending_.readback){auto pending=std::move(rt_probe_pending_);rt_probe_pending_={};slot.kept.push_back(HoldResource(pending.readback.Get()));
            slot.completions.push_back([pending=std::move(pending)]{void* p{};const D3D12_RANGE range{0,size_t(pending.count)*sizeof(RtProbeResult)};
                if(FAILED(pending.readback->Map(0,&range,&p)) || !p)return;std::vector<RtProbeResult> results(pending.count);
                std::memcpy(results.data(),p,size_t(pending.count)*sizeof(RtProbeResult));const D3D12_RANGE none{};pending.readback->Unmap(0,&none);
                try{pending.requests->states->Deliver({pending.requests->keys.data(),pending.count},results,pending.bits,GetTickCount64(),pending.requests->epoch);}catch(...){}});}
        // I-0128 R8a: the frame's end, its pairs resolved; read at the slot's release.
        if(rt_gpu_now_){GpuZoneEnd(list,GpuZone::Frame);
            if(const uint32_t mask=rt_gpu_.Resolve(list,rt_gpu_slot_)){slot.kept.push_back(HoldObject(rt_gpu_.Heap()));slot.kept.push_back(HoldResource(rt_gpu_.Readback()));
                slot.completions.push_back([this,readback=Microsoft::WRL::ComPtr<ID3D12Resource>(rt_gpu_.Readback()),at=rt_gpu_slot_,mask,width=RenderWidth(),height=RenderHeight(),pt=rt_pt_now_ && rr_now_]{   // the frame's level (FillRtReflectFrame's rule)
                    rt_gpu_.Complete(readback.Get(),at,mask,width,height,pt);});}
            rt_gpu_now_=false;}
        {const auto closed=list->Close();if(FAILED(closed)){Diagnostics();char why[480]{};sprintf_s(why,"native model recorder close: HRESULT %08X (%.380s)",static_cast<unsigned>(closed),last_diagnostic_.c_str());throw std::runtime_error(why);}}FrameRing::RequireHeld(slot);FrameReceipt receipt{};receipt.serial=input.serial;
        receipt.content_draw_count=static_cast<uint32_t>(input.draws.size()+input.forward_draws.size()+input.velocity_draws.size());
        receipt.primitive_draw_count=primitive?static_cast<uint32_t>(primitive->draws.size()):0;
        QueryPerformanceCounter(&phase_closed);submitted_lists.push_back(list);receipt.fence=gpu_.Execute(std::span<ID3D12CommandList* const>(submitted_lists));primitive_uploads_pending_=false;frame_uploads_.Retire(gpu_,receipt.fence);if(frames_.Slots()==1)gpu_.Complete(receipt.fence,WaitMainFrame);QueryPerformanceCounter(&phase_completed);
        if(primitive){primitive_stencil_initialized_=true;primitive_stencil_generation_=primitive->generation;}
        receipt.present_status=PresentPaced();receipt.present_lost=AdmitPresent(receipt.present_status,"native model present");
        // cD4 (R6): the ticket after Present covers the frame; its results are produced when the slot is released (here with
        // one slot) and arrive through the channel; this frame's receipt carries them when it was released at its own End.
        receipt.presentation_fence=frames_.Slots()==1?gpu_.Drain(WaitMainFrame):gpu_.Signal();   // cD6: the slot ticket, without a wait
        // The completion keeps the buffers it reads (a size change may replace the slot's readbacks before its release).
        slot.completions.push_back([this,serial=input.serial,ticket=receipt.presentation_fence,held=readbacks?*readbacks:SlotReadbacks{},sample_pixels,pixel_file,
            sample_velocity,velocity_file]{CompleteFrame(serial,ticket,held,sample_pixels,pixel_file,sample_velocity,velocity_file);});
        frames_.End(gpu_,slot,receipt.presentation_fence,WaitMainFrame);gpu_.CollectCompleted();
        TakeReleased(receipt,"native model diagnostic error: ");
        QueryPerformanceCounter(&phase_end);const auto ms=[&](const LARGE_INTEGER& a,const LARGE_INTEGER& b){return static_cast<float>(1000.0*double(b.QuadPart-a.QuadPart)/double(phase_frequency.QuadPart));};
        receipt.record_ms=ms(phase_start,phase_closed);receipt.gpu_wait_ms=ms(phase_closed,phase_completed);receipt.present_ms=ms(phase_completed,phase_end);
        last_serial_=input.serial;PublishSerials();return receipt;
    }catch(...){faulted_=true;throw;}
}
// E2/E3: the end of one shadowed recording (the main view, a menu view or the movie): comparison rows and counters.
void SceneRenderer::FinishGraphShadow(GraphShadow& graph) {
    auto rows=graph.End();graph_shadow_frames_.fetch_add(1,std::memory_order_relaxed);graph_shadow_uses_.store(graph.Uses(),std::memory_order_relaxed);
    graph_executed_frames_.store(graph.ExecutedFrames(),std::memory_order_relaxed);
    graph_barriers_.store(graph.GraphBarriers(),std::memory_order_relaxed);graph_barrier_calls_.store(graph.BarrierCalls(),std::memory_order_relaxed);
    if(!rows.empty()){graph_shadow_mismatch_frames_.fetch_add(1,std::memory_order_relaxed);const std::lock_guard hold(graph_shadow_lock_);
        for(auto& row:rows)if(graph_shadow_rows_.size()<64)graph_shadow_rows_.push_back(std::move(row));}
    // Review: the passes the shadowed recordings opened and the hand-written transitions of undeclared resources, every 600th.
    if(graph.Frames()%600==1){auto summary=graph.Summary();const std::lock_guard hold(graph_shadow_lock_);graph_shadow_rows_.push_back(std::move(summary));}
}
void SceneRenderer::DeclareViewGraph(GraphShadow& graph,bool lit) {
    const auto rt=D3D12_RESOURCE_STATE_RENDER_TARGET;const char* gbuffer_names[6]{"gb0","gb1","gb2","gb3","gb4","gb5"};
    for(unsigned i=0;i<6;++i)graph.Declare(view_->colors[i].Get(),1,rt,FrameLifetime::Transient,gbuffer_names[i]);
    graph.Declare(view_->depth.Get(),2,D3D12_RESOURCE_STATE_DEPTH_WRITE,FrameLifetime::Transient,"depth");
    if(lit){const char* light_names[4]{"light0","light1","light2","light3"};
        for(unsigned i=0;i<4;++i)graph.Declare(view_->light_targets[i].Get(),1,rt,FrameLifetime::Transient,light_names[i]);
        // E5: the neutral images start in their creation state (render target) until cleared once, shader-readable after.
        const auto neutral_start=native_neutral_ready_?D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:rt;
        graph.DeclareStarting(native_light_neutral_[0].Get(),neutral_start,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,FrameLifetime::Persistent,"neutral0");
        graph.DeclareStarting(native_light_neutral_[1].Get(),neutral_start,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,FrameLifetime::Persistent,"neutral1");
        graph.Declare(view_->env_target.Get(),1,rt,FrameLifetime::Transient,"env");graph.Declare(view_->sss_temp.Get(),1,rt,FrameLifetime::Transient,"sss_temp");}
}
std::vector<std::string> SceneRenderer::TakeGraphShadowRows() {
    const std::lock_guard hold(graph_shadow_lock_);std::vector<std::string> rows;rows.swap(graph_shadow_rows_);return rows;
}
// I-0117 (R2 T1): the frame's instance records in its upload memory, then the top level over them on the frame's list (before the
// G-buffer; nothing reads it until the traced passes). A device failure turns the rays off for the session and closes the game with a message (I-0132 R11e, NativeRtFatal).
// T3 step review M3: a frame with no rays still counts in the material table's retirement and sweeps it (a load releases the
// former zone's views and records 120 frames on).
// A failure turns the rays off for the session, as EncodeRtScene's (final review L4; I-0132 R11e: the game closes with a message, NativeRtFatal); once they are off
// the sweep still runs, so the views go (its step review's first point), a further failure kept silent.
void SceneRenderer::TickRtMaterials(ID3D12GraphicsCommandList* list) {
    if(auto* const heap=rt_heap_.load(std::memory_order_acquire);heap && heap->IsOpen())
        try{(void)rt_materials_.Encode(gpu_.device.Get(),list,*heap,[](uint64_t){return UploadArena::Range{};},gpu_.LastSubmitted(),gpu_.CompletedValue(),{});StoreRtMaterialTotals();}
        catch(const std::exception& error){if(!rt_failed_.load(std::memory_order_acquire)){rt_failure_=error.what();rt_failed_.store(true,std::memory_order_release);}}
}
void SceneRenderer::StoreRtMaterialTotals() noexcept {
    const auto t=rt_materials_.Totals();const std::lock_guard hold(rt_material_lock_);rt_material_totals_=t;
}
void SceneRenderer::EncodeRtScene(ID3D12GraphicsCommandList* list,const NativeRtSceneInput& input) {
    if((input.instances.empty() && input.skinned.empty()) || !gpu_.device5){TickRtMaterials(list);return;}
    try{
        ComPtr<ID3D12GraphicsCommandList4> list4;Check(list->QueryInterface(IID_PPV_ARGS(&list4)),"rt scene list4");
        // T2: the skinned instances first (their positions and BLAS on this list), each one built an instance of the top level.
        rt_skinned_instances_.clear();RtSkinCounters skin{};
        rt_skinner_.Encode(gpu_,list4.Get(),[&](uint64_t bytes){return frame_uploads_.Allocate(gpu_.device.Get(),bytes,UploadArena::alignment,false);},
            input.skinned,rt_skinned_instances_,skin);
        rt_skin_instances_.store(skin.instances,std::memory_order_relaxed);rt_skin_builds_.fetch_add(skin.builds,std::memory_order_relaxed);
        rt_skin_refits_.fetch_add(skin.refits,std::memory_order_relaxed);rt_skin_vertices_.fetch_add(skin.vertices,std::memory_order_relaxed);
        rt_skin_failed_.store(skin.failed,std::memory_order_relaxed);rt_skin_bytes_.store(skin.bytes+rt_skinner_.Bytes(),std::memory_order_relaxed);
        rt_skin_winds_.fetch_add(skin.winds,std::memory_order_relaxed);rt_skin_wind_vertices_.fetch_add(skin.wind_vertices,std::memory_order_relaxed);   // T6
        const auto count=static_cast<uint32_t>(input.instances.size()+rt_skinned_instances_.size());
        rt_material_table_=rt_material_rows_=0;
        if(!count){rt_scene_address_=0;TickRtMaterials(list);return;}
        // I-0119 R3a: the instances' material records and rows, in the order the instance records number them (the static
        // instances, then the skinned). Views and ranges released here were last named by a frame now submitted (LastSubmitted).
        auto* const heap=rt_heap_.load(std::memory_order_acquire);const bool materials=heap && heap->IsOpen();
        RtMaterialTable::Frame table;
        if(materials){table=rt_materials_.Encode(gpu_.device.Get(),list,*heap,
                [&](uint64_t bytes){return frame_uploads_.Allocate(gpu_.device.Get(),bytes,UploadArena::alignment,false);},gpu_.LastSubmitted(),gpu_.CompletedValue(),
                {std::span<const RtInstance>(input.instances),std::span<const RtInstance>(rt_skinned_instances_)});
            rt_material_table_=table.table;rt_material_rows_=table.table?table.rows:0;StoreRtMaterialTotals();}
        const auto range=frame_uploads_.Allocate(gpu_.device.Get(),uint64_t(count)*sizeof(D3D12_RAYTRACING_INSTANCE_DESC),UploadArena::alignment,false);
        auto* descs=reinterpret_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(range.cpu);
        const std::span<const uint32_t> firsts(table.firsts);   // empty without a table: every instance opaque
        RtFillInstanceDescs(input.instances,descs,firsts.empty()?firsts:firsts.first(input.instances.size()));
        RtFillInstanceDescs(rt_skinned_instances_,descs+input.instances.size(),firsts.empty()?firsts:firsts.subspan(input.instances.size()));
        rt_scene_address_=rt_tlas_.Build(gpu_.device5.Get(),list4.Get(),range.gpu,count);rt_tlas_bytes_.store(rt_tlas_.Bytes(),std::memory_order_relaxed);
        rt_skinned_instances_.clear();   // final review L3: the frame's input holds the skinned BLAS; this list keeps nothing past the build
    }catch(const std::exception& error){rt_scene_address_=0;rt_skinned_instances_.clear();rt_failure_=error.what();rt_failed_.store(true,std::memory_order_release);}
}
// I-0140 tranche 31 (its step review M2): a menu table no view took this frame encoded with no group (no rows written), on the first
// kMenuRtIdleTicks such frames, so its retirement counts and its sweep runs as TickRtMaterials' for the main table; a failure turns
// the menu self-shadow off.
void SceneRenderer::TickMenuRtMaterials(ID3D12GraphicsCommandList* list) {
    // Once a numbered frame, from the main view's frame or the startup frame (character creation draws no main view: the step review
    // of the fixes, L2).
    if(menu_rt_ticked_==frames_.Numbered())return;menu_rt_ticked_=frames_.Numbered();
    auto* const heap=rt_heap_.load(std::memory_order_acquire);if(!heap || !heap->IsOpen())return;
    const uint32_t taken=menu_rt_frame_==frames_.Numbered()?menu_rt_used_:0;
    for(uint32_t slot=taken;slot<kMenuRtSlots;++slot){if(menu_rt_idle_[slot]>=kMenuRtIdleTicks)continue;++menu_rt_idle_[slot];
        try{(void)menu_rt_materials_[slot].Encode(gpu_.device.Get(),list,*heap,[](uint64_t){return UploadArena::Range{};},gpu_.LastSubmitted(),gpu_.CompletedValue(),{});}
        catch(const std::exception& error){FailMenuShadow(error.what());return;}}
}
// I-0140 tranche 31 (D-0303): a Player menu view's traced scene on the frame's list, as EncodeRtScene's (the frame's skinner, its
// scratch shared: each Encode opens with a UAV barrier), into a material table and top level of the menu's own: the frame's next
// free pair (a table writes its rows buffer once a list, its states assume it; the creation and its Face Detail record four Player
// views a frame), past kMenuRtSlots a view left unshadowed and counted. The rays failed (rt_failed_) or the menu's own failure: none. A failure here
// turns the menu views' self-shadow off for the session (FailMenuShadow), never the rays.
bool SceneRenderer::EncodeMenuRtScene(ID3D12GraphicsCommandList* list,const NativeRtSceneInput& input) {
    menu_rt_scene_=menu_rt_table_=menu_rt_rows_=0;
    if(menu_shadow_failed_.load(std::memory_order_acquire) || rt_failed_.load(std::memory_order_acquire) || !gpu_.device5 ||
       (input.instances.empty() && input.skinned.empty()))return false;
    auto* const heap=rt_heap_.load(std::memory_order_acquire);if(!heap || !heap->IsOpen())return false;
    const auto frame=frames_.Numbered();
    if(menu_rt_frame_!=frame){menu_rt_frame_=frame;menu_rt_used_=0;}
    if(menu_rt_used_>=kMenuRtSlots){menu_shadow_past_slots_.fetch_add(1,std::memory_order_relaxed);return false;}
    const uint32_t slot=menu_rt_used_++;
    try{
        ComPtr<ID3D12GraphicsCommandList4> list4;Check(list->QueryInterface(IID_PPV_ARGS(&list4)),"menu rt list4");
        const auto upload=[&](uint64_t bytes){return frame_uploads_.Allocate(gpu_.device.Get(),bytes,UploadArena::alignment,false);};
        menu_rt_skinned_.clear();RtSkinCounters skin{};
        rt_skinner_.Encode(gpu_,list4.Get(),upload,input.skinned,menu_rt_skinned_,skin);
        const auto count=static_cast<uint32_t>(input.instances.size()+menu_rt_skinned_.size());
        if(!count){menu_rt_skinned_.clear();--menu_rt_used_;return false;}   // the pair given back (the step review of the fixes, L2)
        menu_rt_idle_[slot]=0;   // taken: its table swept after its last use (the review of the Lows' fixes, L4: not a pair given back)
        const auto table=menu_rt_materials_[slot].Encode(gpu_.device.Get(),list,*heap,upload,gpu_.LastSubmitted(),gpu_.CompletedValue(),
            {std::span<const RtInstance>(input.instances),std::span<const RtInstance>(menu_rt_skinned_)});
        const auto range=upload(uint64_t(count)*sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
        auto* descs=reinterpret_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(range.cpu);
        const std::span<const uint32_t> firsts(table.firsts);
        RtFillInstanceDescs(input.instances,descs,firsts.empty()?firsts:firsts.first(input.instances.size()));
        RtFillInstanceDescs(menu_rt_skinned_,descs+input.instances.size(),firsts.empty()?firsts:firsts.subspan(input.instances.size()));
        menu_rt_scene_=menu_rt_tlas_[slot].Build(gpu_.device5.Get(),list4.Get(),range.gpu,count);
        menu_rt_table_=table.table;menu_rt_rows_=table.table?table.rows:0;
        menu_rt_skinned_.clear();   // the frame's input holds the skinned BLAS
        return menu_rt_scene_!=0;
    }catch(const std::exception& error){menu_rt_skinned_.clear();menu_rt_scene_=menu_rt_table_=menu_rt_rows_=0;FailMenuShadow(error.what());return false;}
}
// I-0117 (R2 T4): the control view on the main view's every rt_view_every-th frame with a top level (and on the frames whose job
// names its pictures), once its pictures, counts and slots are made for the current targets (the slots released against the last
// submitted ticket when the targets change: no submitted list reads them after it; the pictures themselves held by each frame that
// used them). A failure turns the tool off for the session, never the rays or the raster.
bool SceneRenderer::RtViewFrame(const NativeModelFrameInput& input) {
    if(!rt_view.load(std::memory_order_relaxed) || view_!=&main_view_ || !input.rt_scene || rt_failed_.load(std::memory_order_acquire) || !gpu_.device5 || !view_->depth)return false;
    if(input.serial%rt_view_every!=0 && rt_view_file_.empty())return false;
    auto* const heap=rt_heap_.load(std::memory_order_acquire);if(!heap || !heap->IsOpen())return false;
    // Step review L1: the camera, the depth and the pictures at one extent (the render extent; RenderModels refuses another camera).
    const auto depth=view_->depth->GetDesc();
    if(input.camera.width!=RenderWidth() || input.camera.height!=RenderHeight() || depth.Width!=RenderWidth() || depth.Height!=RenderHeight())return false;
    if(rt_view_generation_==view_->target_generation && rt_view_images_[0] && rt_view_images_[0]->GetDesc().Width==RenderWidth() &&
       rt_view_images_[0]->GetDesc().Height==RenderHeight())return true;
    try{
        for(auto& slot:rt_view_slots_)if(slot!=BindlessIndices::none){heap->Release(slot,gpu_.LastSubmitted());slot=BindlessIndices::none;}
        for(auto& slot:rt_view_slots_){slot=heap->Allocate(gpu_.CompletedValue());if(slot==BindlessIndices::none)throw std::runtime_error("rt view slots");}
        MakeRtMainViews(*heap);
        D3D12_HEAP_PROPERTIES heap_type{};heap_type.Type=D3D12_HEAP_TYPE_DEFAULT;
        const auto desc=Image(RenderWidth(),RenderHeight(),1,DXGI_FORMAT_R8G8B8A8_UNORM,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        for(size_t i=0;i<rt_view_images_.size();++i){ComPtr<ID3D12Resource> image;
            Check(CountedResource(gpu_.device)->CreateCommittedResource(&heap_type,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&image)),"rt view picture");
            D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};uav.Format=desc.Format;uav.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
            gpu_.device->CreateUnorderedAccessView(image.Get(),nullptr,&uav,heap->Cpu(rt_view_slots_[i]));rt_view_images_[i]=std::move(image);}
        if(!rt_view_counts_){auto buffer=Buffer(RtView::kCountBytes);buffer.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            Check(CountedResource(gpu_.device)->CreateCommittedResource(&heap_type,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&rt_view_counts_)),"rt view counts");}
        rt_view_generation_=view_->target_generation;return true;
    }catch(const std::exception& error){CloseRtView();FailRtView(error.what());return false;}
}
// I-0117 R5: the traced sun shadow this frame: its gate (`possible`: the mask stage, the shadowed sun's pipeline, the frame's shadow blocks written), rt_shadow (on with the rays), the main view
// with the rays' scene, the camera and the depth at the render extent; its depth view (R32 of the D32S8) in the bindless heap made
// for the target generation. Without a top level at its record the mask is cleared lit for that frame (EncodeNativeShadowMask).
bool SceneRenderer::RtSunFrame(const NativeModelFrameInput& input,bool possible) {
    if(!possible || !rt_shadow.load(std::memory_order_relaxed) || view_!=&main_view_ || !input.rt_scene || rt_failed_.load(std::memory_order_acquire) || !gpu_.device5 || !view_->depth)return false;
    auto* const heap=rt_heap_.load(std::memory_order_acquire);if(!heap || !heap->IsOpen())return false;
    const auto depth=view_->depth->GetDesc();
    if(input.camera.width!=RenderWidth() || input.camera.height!=RenderHeight() || depth.Width!=RenderWidth() || depth.Height!=RenderHeight())return false;
    try{MakeRtMainViews(*heap);return true;}
    catch(const std::exception& error){FailRtSun(error.what());return false;}   // step review M3: the pipelines stay for the frames in flight
}
// I-0122 R4b (R4a final review L3): the views the rays' passes share (rt_main_views_), made once for the main view's target generation.
// Allocated before the former set is released, so a failure keeps it; the former set released against the last submitted ticket
// (no list submitted after it reads them).
void SceneRenderer::MakeRtMainViews(BindlessHeap& heap) {
    if(rt_main_views_generation_==view_->target_generation && rt_main_views_[rt_depth_view]!=BindlessIndices::none)return;
    std::array<uint32_t,6> made{BindlessIndices::none,BindlessIndices::none,BindlessIndices::none,BindlessIndices::none,BindlessIndices::none,BindlessIndices::none};
    for(auto& s:made){s=heap.Allocate(gpu_.CompletedValue());
        if(s==BindlessIndices::none){for(auto& t:made)if(t!=BindlessIndices::none)heap.Release(t,gpu_.LastSubmitted());throw std::runtime_error("rt main view slots");}}
    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;
    gpu_.device->CreateShaderResourceView(view_->depth.Get(),&srv,heap.Cpu(made[rt_depth_view]));
    for(uint32_t i=0;i<5;++i){D3D12_SHADER_RESOURCE_VIEW_DESC gb{};gb.Format=color_formats[i];gb.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
        gb.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;gb.Texture2D.MipLevels=1;
        gpu_.device->CreateShaderResourceView(view_->colors[i].Get(),&gb,heap.Cpu(made[rt_gb0_view+i]));}
    for(auto& s:rt_main_views_)if(s!=BindlessIndices::none)heap.Release(s,gpu_.LastSubmitted());
    rt_main_views_=made;rt_main_views_generation_=view_->target_generation;
}
// After the queue's drain only: the renderer's close.
void SceneRenderer::CloseRtMainViews() noexcept {
    if(auto* const heap=rt_heap_.load(std::memory_order_acquire))
        for(auto& s:rt_main_views_)if(s!=BindlessIndices::none){try{heap->Release(s,gpu_.LastSubmitted());}catch(...){}}
    rt_main_views_={BindlessIndices::none,BindlessIndices::none,BindlessIndices::none,BindlessIndices::none,BindlessIndices::none,BindlessIndices::none};rt_main_views_generation_=UINT64_MAX;
}
// I-0122 R4a: the native local lights this frame: rt_lights (on with the rays), the frame's list, the main view lit with the camera,
// the depth and the list's tiles at the render extent; the main view's shared views (MakeRtMainViews: the depth, GB0, GB1, GB3).
// A failure to make them turns it off for the session.
bool SceneRenderer::RtLightsFrame(const NativeModelFrameInput& input,bool lit) {
    if(!lit || !input.lighting || !input.lighting->rt_lights || !rt_lights.load(std::memory_order_relaxed) || view_!=&main_view_ || !gpu_.device5 || !view_->depth)return false;
    auto* const heap=rt_heap_.load(std::memory_order_acquire);if(!heap || !heap->IsOpen())return false;
    const auto depth=view_->depth->GetDesc();const auto& list=*input.lighting->rt_lights;
    if(input.camera.width!=RenderWidth() || input.camera.height!=RenderHeight() || depth.Width!=RenderWidth() || depth.Height!=RenderHeight() ||
       list.edge!=128 || list.columns!=(RenderWidth()+127)/128 || list.rows!=(RenderHeight()+127)/128)return false;
    RtViewConstants camera{};if(!RtViewCameraOf(input.camera,camera))return false;
    try{
        MakeRtMainViews(*heap);
    }catch(const std::exception& error){FailRtLights(error.what());return false;}
    auto& c=rt_lights_camera_;c={};c.right=camera.right;c.up=camera.up;c.forward=camera.forward;c.position=camera.position;c.jitter=camera.jitter;
    c.width=camera.width;c.height=camera.height;c.depth=rt_main_views_[rt_depth_view];c.gb0=rt_main_views_[rt_gb0_view];c.gb1=rt_main_views_[rt_gb1_view];
    c.gb3=rt_main_views_[rt_gb3_view];
    c.columns=list.columns;c.tile=list.edge;
    return true;
}
// I-0123 RR-a (RR-PLAN §2): Ray Reconstruction this frame: rr (set at startup), Streamline's RR ready, the main view at the render
// extent with a camera; the main view's shared views (MakeRtMainViews), the guides' targets at the render extent and the lit HDR's
// format (remade on a change; the former ones held by the frames that used them), each held by this frame. A failure to make them
// latches RR off for the session.
bool SceneRenderer::RrFrame(const NativeModelFrameInput& input,FrameSlot& slot) {
    if(!rr.load(std::memory_order_relaxed) || !streamline::RrReady() || view_!=&main_view_ || !view_->depth || !view_->light_targets[3])return false;
    auto* const heap=rt_heap_.load(std::memory_order_acquire);if(!heap || !heap->IsOpen())return false;
    if(input.camera.width!=RenderWidth() || input.camera.height!=RenderHeight())return false;
    RtViewConstants camera{};if(!RtViewCameraOf(input.camera,camera))return false;
    try{
        MakeRtMainViews(*heap);
        const auto hdr_format=view_->light_targets[3]->GetDesc().Format;
        const auto stale=[&]{if(!rr_targets_[3] || !rr_targets_[4])return true;const auto d=rr_targets_[3]->GetDesc();return d.Width!=RenderWidth() || d.Height!=RenderHeight() || d.Format!=hdr_format;};
        if(stale()){
            if(!rr_rtvs_){D3D12_DESCRIPTOR_HEAP_DESC rtvs{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,4,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
                Check(CountedHeap(gpu_.device.Get())->CreateDescriptorHeap(&rtvs,IID_PPV_ARGS(&rr_rtvs_)),"rr guide targets");}
            const auto heap_type=Heap(D3D12_HEAP_TYPE_DEFAULT);
            std::array<ComPtr<ID3D12Resource>,5> made;   // I-0125 R6c: the fifth the specular hit distance, its RTV the fourth
            for(uint32_t i=0;i<5;++i){const auto format=i<3?kRrGuideFormats[i]:i==3?hdr_format:kRrHitDistanceFormat;
                const auto image=Image(RenderWidth(),RenderHeight(),1,format,i!=3?D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET:D3D12_RESOURCE_FLAG_NONE);
                // The hit distance is cleared each RR frame: its clear value given (the debug layer refuses a clear without one).
                const D3D12_CLEAR_VALUE zero{kRrHitDistanceFormat,{0,0,0,0}};
                Check(CountedResource(gpu_.device.Get())->CreateCommittedResource(&heap_type,D3D12_HEAP_FLAG_NONE,&image,kRrGuideState,i==4?&zero:nullptr,IID_PPV_ARGS(&made[i])),"rr guide target");}
            for(uint32_t i=0;i<3;++i)gpu_.device->CreateRenderTargetView(made[i].Get(),nullptr,CPU(rr_rtvs_.Get(),rtv_stride_,i));
            gpu_.device->CreateRenderTargetView(made[4].Get(),nullptr,CPU(rr_rtvs_.Get(),rtv_stride_,3));
            rr_targets_=std::move(made);}
    }catch(const std::exception& error){LatchRrOff(input.serial,std::string("guides: ")+error.what());return false;}
    for(const auto& target:rr_targets_)slot.kept.push_back(HoldResource(target.Get()));
    rr_serial_=input.serial;
    auto& c=rr_camera_;c={};c.right=camera.right;c.up=camera.up;c.forward=camera.forward;c.position=camera.position;c.jitter=camera.jitter;
    c.width=camera.width;c.height=camera.height;c.depth=rt_main_views_[rt_depth_view];c.gb0=rt_main_views_[rt_gb0_view];c.gb1=rt_main_views_[rt_gb1_view];
    c.gb2=rt_main_views_[rt_gb2_view];c.gb3=rt_main_views_[rt_gb3_view];
    return true;
}
// I-0126 R7a (R7-PLAN §1): the traced diffuse ambient this frame: rt_gi (set at startup), the main view's frame that Ray Reconstruction
// denoises (rr_now_, decided before) or, R7c, that NRD denoises (rt_nrd_now_, decided before), with the local lights' pass (rt_lights_now_: its camera and list) and the rays' scene; a size
// in transition or a refused camera has no lights' pass, so no GI (the frame judged unserved, I-0132 R11e: not shown, world black, magenta in the test runs; no SSAO since R7c, D-0251). R7a's final review M1: and
// the traced sun shadow still on for the session (a latched-off traced sun would leave every GI ray's sunward wall unshadowed; a frame
// without a sun shadow at all, rt_sun_now_ false, takes the hits' light 0 unshadowed as R6's reflections do).
bool SceneRenderer::RtGiFrame(const NativeModelFrameInput& input) const {return (rr_now_ || rt_nrd_now_) && RtGiPossible(input);}
// The GI's gates but its denoiser's (RtGiFrame's and RtNrdFrame's: one list, the fixes' step review L-c).
bool SceneRenderer::RtGiPossible(const NativeModelFrameInput& input) const {
    return rt_gi.load(std::memory_order_relaxed) && view_==&main_view_ && rt_lights_now_ && rt_shadow.load(std::memory_order_relaxed) && input.rt_scene &&
        !rt_failed_.load(std::memory_order_acquire) && input.lighting && input.lighting->rt_lights;
}
// I-0126 R7d (R7-PLAN §8): the GI's bias check this frame (rt-gi-mean.flag, on a GI frame): its accumulation at the render extent
// (made on its first frame or a new extent, in UNORDERED_ACCESS, its UAV in the bindless heap; the former one held by the frames that
// used it, its slot released against the last submitted ticket), held by this slot; restarted when the camera's rows (rt_lights_camera_,
// the jitter apart: every jittered frame is a sample of the same pixel), the extent or the frame sequence changes (a frame without
// the GI between two). A failure turns it off for the session.
bool SceneRenderer::RtGiMeanFrame(FrameSlot& slot) {
    if(!rt_gi_mean.load(std::memory_order_relaxed))return false;
    auto* const heap=rt_heap_.load(std::memory_order_acquire);if(!heap || !heap->IsOpen())return false;
    bool made=false;
    try{if(!rt_gi_mean_target_ || rt_gi_mean_target_->GetDesc().Width!=RenderWidth() || rt_gi_mean_target_->GetDesc().Height!=RenderHeight()){
            const auto heap_type=Heap(D3D12_HEAP_TYPE_DEFAULT);
            const auto image=Image(RenderWidth(),RenderHeight(),1,DXGI_FORMAT_R32G32B32A32_FLOAT,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
            ComPtr<ID3D12Resource> target;
            Check(CountedResource(gpu_.device.Get())->CreateCommittedResource(&heap_type,D3D12_HEAP_FLAG_NONE,&image,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&target)),"rt gi mean target");
            const uint32_t fresh=heap->Allocate(gpu_.CompletedValue());if(fresh==BindlessIndices::none)throw std::runtime_error("rt gi mean slot");
            D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};uav.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;uav.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
            gpu_.device->CreateUnorderedAccessView(target.Get(),nullptr,&uav,heap->Cpu(fresh));
            if(rt_gi_mean_slot_!=BindlessIndices::none)heap->Release(rt_gi_mean_slot_,gpu_.LastSubmitted());
            rt_gi_mean_slot_=fresh;rt_gi_mean_target_=std::move(target);made=true;}
    }catch(const std::exception&){rt_gi_mean.store(false,std::memory_order_relaxed);return false;}
    slot.kept.push_back(HoldResource(rt_gi_mean_target_.Get()));
    const auto& c=rt_lights_camera_;std::array<float,16> camera{};
    for(int i=0;i<4;++i){camera[i]=c.right[i];camera[4+i]=c.up[i];camera[8+i]=c.forward[i];camera[12+i]=c.position[i];}
    // The step review's L1: a change of estimator (RR's gi or NRD's nrd_signal, ReSTIR or the one ray) restarts it too.
    const uint32_t estimator=(rt_nrd_now_?1u:0u)|(rt_restir_now_?2u:0u);
    rt_gi_mean_restart_=made || camera!=rt_gi_mean_camera_ || rt_gi_mean_frame_+1!=rt_seed_serial_ || estimator!=rt_gi_mean_estimator_;
    rt_gi_mean_camera_=camera;rt_gi_mean_frame_=rt_seed_serial_;rt_gi_mean_estimator_=estimator;
    return true;
}
// I-0132 R11b (R11-PLAN §13): SER this frame: rt_ser (set at startup), a main-view frame RR denoises at the PT level whose paths go past
// their first hit (RtPathsOf's condition), the rays' heap open; its state object and scratch targets at the render extent
// (RtReflections::MakeSer; held by this slot). A failure turns it off for the session (its reason in the native_rt_ser row): the path
// programmes then run as pixel shaders, the same picture.
bool SceneRenderer::RtSerFrame(FrameSlot& slot) {
    if(!rt_ser.load(std::memory_order_relaxed) || !rr_now_ || view_!=&main_view_ || !rt_pt_now_)return false;
    if(!rt_path_reference.load(std::memory_order_relaxed) && rt_bounces_now_<2)return false;
    auto* const heap=rt_heap_.load(std::memory_order_acquire);if(!heap || !heap->IsOpen())return false;
    try{const bool first=!rt_reflect_pass_.SerMade();const auto t0=first?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};
        rt_reflect_pass_.MakeSer(gpu_.device.Get(),*heap,RenderWidth(),RenderHeight(),gpu_.CompletedValue(),gpu_.LastSubmitted());
        if(first)rt_ser_make_us_.store(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-t0).count(),std::memory_order_relaxed);}
    catch(const std::exception& error){rt_ser.store(false,std::memory_order_relaxed);
        try{const std::lock_guard hold(rt_view_lock_);if(rt_ser_failure_.empty())rt_ser_failure_=*error.what()?error.what():"unknown";}catch(...){}
        return false;}
    rt_reflect_pass_.HoldSer(slot.kept);
    return true;
}
// I-0126 R7d (R7-PLAN §8): ReSTIR GI this frame: rt_restir (set at startup), a GI frame RR denoises at the PT level (rt_pt), the
// rays' heap open; its buffers at the render extent (RtReflections::MakeRestir; held by this slot). Its history: the last ReSTIR
// frame the previous serial, the same buffers, the velocity's rows (the camera's motion) without a cut. A failure turns it off for the
// session (the PT level's one-ray GI then).
bool SceneRenderer::RtRestirFrame(const NativeModelFrameInput& input,FrameSlot& slot) {
    if(!rt_restir.load(std::memory_order_relaxed) || !rr_now_ || !rt_pt_now_)return false;
    auto* const heap=rt_heap_.load(std::memory_order_acquire);if(!heap || !heap->IsOpen())return false;
    bool made=false;
    try{made=rt_reflect_pass_.MakeRestir(gpu_.device.Get(),RenderWidth(),RenderHeight(),RtRestirShareOf(rt_restir_stages.load(std::memory_order_relaxed))!=0);}   // I-0128 R8d: a decorrelated share's third part
    catch(const std::exception& error){rt_restir.store(false,std::memory_order_relaxed);rt_reflect_pass_.ReleaseRestir();
        try{const std::lock_guard hold(rt_view_lock_);if(rt_restir_failure_.empty())rt_restir_failure_=*error.what()?error.what():"unknown";}catch(...){}
        return false;}
    rt_reflect_pass_.HoldRestir(slot.kept);
    rt_restir_history_=!made && rt_restir_serial_+1==input.serial && input.velocity && !input.velocity->reset;
    if(input.velocity)rt_restir_current_to_previous_=input.velocity->current_to_previous;
    rt_restir_eye_previous_=rt_restir_eye_;rt_restir_eye_=rt_lights_camera_.position;rt_restir_serial_=input.serial;
    return true;
}
// After the queue's drain only: the renderer's close.
void SceneRenderer::CloseRtGiMean() noexcept {
    if(auto* const heap=rt_heap_.load(std::memory_order_acquire);heap && rt_gi_mean_slot_!=BindlessIndices::none){try{heap->Release(rt_gi_mean_slot_,gpu_.LastSubmitted());}catch(...){}}
    rt_gi_mean_slot_=BindlessIndices::none;rt_gi_mean_target_.Reset();rt_gi_mean_now_=false;rt_gi_mean_frame_=UINT64_MAX;
}
// I-0125 R6c (R6-PLAN §6): the water's reflection traced this frame: rt_water (set at startup), the main view's lit frame with the
// collect's rt_water (a target of kRtWaterFormat that takes a render target, a finite plane with a normal), the frame's camera; the
// target's RTV made; the target held by this frame's slot. A failure to make the view turns the pass off.
bool SceneRenderer::RtWaterFrame(const NativeModelFrameInput& input,FrameSlot& slot,bool lit) {
    rt_water_target_.Reset();
    if(!lit || !rt_water.load(std::memory_order_relaxed) || view_!=&main_view_ || !input.rt_water || !input.rt_water->target)return false;
    auto* const target=input.rt_water->target.Get();const auto d=target->GetDesc();
    if(d.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D || d.Format!=kRtWaterFormat || !(d.Flags&D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) || !d.Width || !d.Height ||
        d.Width>D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION || d.Height>D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION)return false;
    const auto& plane=input.rt_water->plane;
    if(!std::all_of(plane.begin(),plane.end(),[](float v){return std::isfinite(v);}) || !(plane[0]*plane[0]+plane[1]*plane[1]+plane[2]*plane[2]>0))return false;
    RtViewConstants camera{};if(!RtViewCameraOf(input.camera,camera))return false;
    try{if(!rt_water_rtvs_){D3D12_DESCRIPTOR_HEAP_DESC rtvs{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,1,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
            Check(CountedHeap(gpu_.device.Get())->CreateDescriptorHeap(&rtvs,IID_PPV_ARGS(&rt_water_rtvs_)),"rt water target view");}
        // Made each frame (a pointer equal to a freed target's would keep a stale view); a list reads it when it records it.
        gpu_.device->CreateRenderTargetView(target,nullptr,CPU(rt_water_rtvs_.Get(),rtv_stride_,0));}
    catch(const std::exception& error){rt_water.store(false,std::memory_order_relaxed);rt_water_drawn_.store(nullptr,std::memory_order_release);   // the fixes' review L1
        try{const std::lock_guard hold(rt_view_lock_);if(rt_water_failure_.empty())rt_water_failure_=error.what();}catch(...){}
        return false;}
    slot.kept.push_back(HoldResource(target));
    rt_water_camera_=camera;rt_water_plane_=plane;rt_water_target_=input.rt_water->target;
    return true;
}
// I-0126 R7c (R7-PLAN §7): NRD this frame: the GI on (rt_gi, not latched), the main view's lit frame RR does not denoise, its velocity
// (the Velo rows drawn before the lighting, the target of this view's generation), the camera at the render extent and the bindless
// heap; the denoiser open at the render extent, the velocity target's and the denoised radiances' slots (I-0132 R11: the diffuse's
// and the specular's) made for their generations
// (the former released against the last submitted ticket), NRD's camera (BuildSceneView's unjittered rows, row-vector row-major: a
// column-vector matrix's column-major array) and the signal's velocity constants; NRD's textures held by this slot. Its history
// restarts on the velocity's cut and on any gap in the serials it served. A failure turns NRD off for the session; a traced frame
// without RR that NRD cannot serve adds no GI and no specular and is judged unserved, not shown (I-0132 R11e, D-0262; R7-PLAN §7) and is counted
// (rt_nrd_missed_, the native_rt_gi row's nrd_missed).
bool SceneRenderer::RtNrdFrame(const NativeModelFrameInput& input,const SceneView& view,FrameSlot& slot) {
    // RtGiFrame's own gates first (the final review's L2: NRD is not opened or latched on a frame the GI would not serve anyway).
    if(rr_now_ || !RtGiPossible(input))return false;
    // From here a traced frame the GI is meant for: one NRD cannot serve is counted (the final review's M1; judged unserved, I-0132 R11e).
    if(rt_nrd_off.load(std::memory_order_relaxed) || !input.velocity || velocity_generation_!=view_->target_generation || !velocity_target_ || input.camera.width!=RenderWidth() ||
        input.camera.height!=RenderHeight()){rt_nrd_missed_.fetch_add(1,std::memory_order_relaxed);return false;}
    auto* const heap=rt_heap_.load(std::memory_order_acquire);if(!heap || !heap->IsOpen()){rt_nrd_missed_.fetch_add(1,std::memory_order_relaxed);return false;}
    try{
        std::string reason;
        if(!rt_denoise_.Open(gpu_.device.Get(),RenderWidth(),RenderHeight(),reason)){FailRtNrd(reason.c_str());rt_nrd_missed_.fetch_add(1,std::memory_order_relaxed);return false;}
        MakeRtMainViews(*heap);
        const auto slot_for=[&](uint32_t& made,uint64_t& made_generation,uint64_t generation,ID3D12Resource* resource,DXGI_FORMAT format){
            if(made!=BindlessIndices::none && made_generation==generation)return;
            const uint32_t fresh=heap->Allocate(gpu_.CompletedValue());if(fresh==BindlessIndices::none)throw std::runtime_error("rt nrd slots");
            D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=format;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
            srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;
            gpu_.device->CreateShaderResourceView(resource,&srv,heap->Cpu(fresh));
            if(made!=BindlessIndices::none)heap->Release(made,gpu_.LastSubmitted());
            made=fresh;made_generation=generation;};
        slot_for(rt_velocity_slot_,rt_velocity_slot_generation_,velocity_generation_,velocity_target_.Get(),VelocityTargetFormat());
        slot_for(rt_denoised_slot_,rt_denoised_slot_generation_,rt_denoise_.Generation(),rt_denoise_.Output(RtDenoised::Diffuse),kRtDenoiseOutputFormat);
        slot_for(rt_denoised_spec_slot_,rt_denoised_spec_slot_generation_,rt_denoise_.Generation(),rt_denoise_.Output(RtDenoised::Specular),kRtDenoiseOutputFormat);
    }catch(const std::exception& error){FailRtNrd(error.what());rt_nrd_missed_.fetch_add(1,std::memory_order_relaxed);return false;}
    rt_denoise_.Hold(slot.kept);slot.kept.push_back(HoldResource(velocity_target_.Get()));
    auto& c=rt_nrd_camera_;c.view_to_clip=view.projection;c.world_to_view=view.view;c.jitter={input.camera.jitter_x,input.camera.jitter_y};
    c.restart=input.velocity->reset || rt_nrd_serial_+1!=input.serial;rt_nrd_serial_now_=input.serial;
    rt_nrd_velocity_scale_=input.velocity->scale;rt_nrd_current_to_previous_=input.velocity->current_to_previous;
    return true;
}
void SceneRenderer::FailRtNrd(const char* why) noexcept {
    try{const std::lock_guard hold(rt_view_lock_);if(rt_nrd_failure_.empty())rt_nrd_failure_=why && *why?why:"unknown";}catch(...){}
    rt_nrd_off.store(true,std::memory_order_relaxed);
}
// After the queue's drain only: the renderer's close.
void SceneRenderer::CloseRtDenoise() noexcept {
    if(auto* const heap=rt_heap_.load(std::memory_order_acquire))
        for(auto* s:{&rt_velocity_slot_,&rt_denoised_slot_,&rt_denoised_spec_slot_})if(*s!=BindlessIndices::none){try{heap->Release(*s,gpu_.LastSubmitted());}catch(...){}}
    rt_velocity_slot_=rt_denoised_slot_=rt_denoised_spec_slot_=BindlessIndices::none;
    rt_velocity_slot_generation_=rt_denoised_slot_generation_=rt_denoised_spec_slot_generation_=UINT64_MAX;
    rt_denoise_.Close();rt_nrd_now_=false;rt_nrd_serial_=UINT64_MAX;
}
// RR off for the session (the renderer's flag and Streamline's: the game thread's jitter and render extent follow DLSS again), its row.
void SceneRenderer::LatchRrOff(uint64_t serial,const std::string& reason) {
    rr.store(false,std::memory_order_relaxed);streamline::RrDisable();
    if(!upscaler_sink_)return;
    std::string why=reason.substr(0,200);for(auto& c:why)if(c=='"' || c=='\\' || c<32)c=' ';char row[320]{};
    sprintf_s(row,"{\"event\":\"native_rr_latched_off\",\"serial\":%llu,\"after_frames\":%llu,\"reason\":\"%s\"}\n",static_cast<unsigned long long>(serial),
        static_cast<unsigned long long>(rr_frames_.load(std::memory_order_relaxed)),why.c_str());upscaler_sink_(row);
}
// After the queue's drain only: the renderer's close.
void SceneRenderer::CloseRr() noexcept {rr_guides_.Close();rr_targets_={};rr_rtvs_.Reset();rr_now_=false;}
void SceneRenderer::FailRtLights(const char* why) noexcept {
    try{const std::lock_guard hold(rt_view_lock_);if(rt_lights_failure_.empty())rt_lights_failure_=why && *why?why:"unknown";}catch(...){}
    rt_lights.store(false,std::memory_order_relaxed);
}
// After the queue's drain only: the renderer's close (its pipeline and check images).
void SceneRenderer::CloseRtLights() noexcept {
    rt_lights_pass_.Close();
    // I-0125 R6b: the reflections' pipeline (I-0132 R11e: the cube slots removed with the region cubes' sampling).
    if(auto* const heap=rt_heap_.load(std::memory_order_acquire)){rt_reflect_pass_.ReleaseProbeViews(*heap,gpu_.LastSubmitted());   // I-0129 R9a's step review M3
        rt_reflect_pass_.ReleaseSerViews(*heap,gpu_.LastSubmitted());}   // I-0132 R11b
    rt_reflect_pass_.Close();rt_reflect_frame_=UINT64_MAX;rt_reflect_regions_=0;   // the final review's L1: the frame ring restarts
    rt_water_target_.Reset();rt_water_rtvs_.Reset();rt_water_drawn_.store(nullptr,std::memory_order_release);rt_water_now_=false;rr_hit_written_=false;   // I-0125 R6c
}
void SceneRenderer::FailRtSun(const char* why) noexcept {
    try{const std::lock_guard hold(rt_view_lock_);if(rt_sun_failure_.empty())rt_sun_failure_=why && *why?why:"unknown";}catch(...){}
    rt_shadow.store(false,std::memory_order_relaxed);
}
// After the queue's drain only: the renderer's close (its pipelines).
void SceneRenderer::CloseRtSun() noexcept {rt_sun_.Close();}
void SceneRenderer::FailRtView(const char* why) noexcept {
    try{const std::lock_guard hold(rt_view_lock_);if(rt_view_failure_.empty())rt_view_failure_=why && *why?why:"unknown";}catch(...){}
    rt_view.store(false,std::memory_order_relaxed);
}
void SceneRenderer::EncodeRtView(ID3D12GraphicsCommandList* list,FrameSlot& slot,const NativeModelFrameInput& input,GraphShadow* graph) {
    auto* const depth=view_->depth.Get();auto* const heap=rt_heap_.load(std::memory_order_acquire);
    // I-0132 R11b: on a frame with SER the graph's planner holds the depth's plane 0 from the lighting phase in DEPTH_READ | PIXEL |
    // NON_PIXEL (a read state's superset satisfies this pass's), so this pass takes that state too (else a use_state mismatch every frame).
    // The same for GB0 to GB3 (the final step review's M1), from this recording's SER flag (its L1: never another frame's).
    const auto ser=rt_ser_now_?D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:D3D12_RESOURCE_STATES(0);
    const auto depth_read=D3D12_RESOURCE_STATE_DEPTH_READ|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE|ser;
    DepthPlaneTransition(list,depth,D3D12_RESOURCE_STATE_DEPTH_WRITE,depth_read);
    // R3b: GB0 to GB3 read by the pass (render targets until the forward rows), back after it; final review M1: through the
    // renderer's Transition (recorded by the graph's shadow, the graph's own barriers on a frame it drives), before the pass's uses.
    const auto gb_read=[&](size_t i){return i<4?D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE|ser:D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;};
    const auto gbuffer=[&](bool in){for(size_t i=0;i<5;++i)Transition(list,view_->colors[i].Get(),in?D3D12_RESOURCE_STATE_RENDER_TARGET:gb_read(i),in?gb_read(i):D3D12_RESOURCE_STATE_RENDER_TARGET);};   // I-0125 R6a: GB4 too
    gbuffer(true);
    if(graph){graph->Pass("rt_view",PassKind::Readback);graph->Use(depth,ResourceAccess::Read,depth_read,{0,1});
        for(const auto& image:rt_view_images_)graph->Use(image.Get(),ResourceAccess::Write,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        for(size_t i=0;i<5;++i)graph->Use(view_->colors[i].Get(),ResourceAccess::Read,gb_read(i));}
    for(const auto& image:rt_view_images_)slot.kept.push_back(HoldResource(image.Get()));
    std::wstring file;file.swap(rt_view_file_);
    RtViewConstants constants{};
    // No top level this frame (no instance, or its build failed): the pass and its barriers kept (the plan key's), nothing traced.
    bool traced=false;
    if(rt_scene_address_ && rt_view.load(std::memory_order_relaxed) && RtViewCameraOf(input.camera,constants))try{
        constants.depth=rt_main_views_[rt_depth_view];constants.traced=rt_view_slots_[0];constants.classes=rt_view_slots_[1];constants.differences=rt_view_slots_[2];
        constants.gb0=rt_main_views_[rt_gb0_view];constants.gb1=rt_main_views_[rt_gb1_view];constants.gb2=rt_main_views_[rt_gb2_view];constants.gb3=rt_main_views_[rt_gb3_view];
        if(input.rt_scene){constants.anisotropy=input.rt_scene->anisotropy;constants.texture_bias=input.rt_scene->texture_bias;constants.dominant=input.rt_scene->dominant;}   // R3c: the raster's level
        constants.gb4=rt_main_views_[rt_gb4_view];   // I-0125 R6a
        rt_view_.Encode(gpu_.device.Get(),list,*heap,[&](uint64_t bytes){return frame_uploads_.Allocate(gpu_.device.Get(),bytes,UploadArena::alignment,false);},
            rt_scene_address_,rt_material_table_,rt_material_rows_,constants,rt_view_counts_.Get());
        slot.kept.push_back(HoldResource(rt_view_counts_.Get()));
        D3D12_RESOURCE_BARRIER copy{};copy.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;   // a buffer: back to COMMON at the submission's end
        copy.Transition={rt_view_counts_.Get(),0,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&copy);
        auto& next=rt_view_readbacks_[rt_view_readback_++%rt_view_readbacks_.size()];
        if(!next)next=NewBuffer(gpu_.device.Get(),RtView::kCountBytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
        const auto readback=next;
        list->CopyBufferRegion(readback.Get(),0,rt_view_counts_.Get(),0,RtView::kCountBytes);
        // Final review L2: a refused map turns the tool off, never the renderer (a completion's exception faults the next frame).
        slot.completions.push_back([this,readback,serial=input.serial]{std::vector<uint32_t> words(RtView::kCountBytes/4);void* memory{};const D3D12_RANGE range{0,RtView::kCountBytes};
            if(FAILED(readback->Map(0,&range,&memory))){FailRtView("rt view readback map");return;}std::memcpy(words.data(),memory,RtView::kCountBytes);const D3D12_RANGE none{};readback->Unmap(0,&none);
            const auto c=RtViewCountsOf(words.data());const std::lock_guard hold(rt_view_lock_);if(!rt_view_failure_.empty())return;auto& t=rt_view_totals_;++t.samples;t.serial=serial;t.last=c;
            t.sum.agree+=c.agree;t.sum.close+=c.close;t.sum.nearer+=c.nearer;t.sum.farther+=c.farther;t.sum.missed+=c.missed;t.sum.sky+=c.sky;t.sum.extra+=c.extra;t.sum.nearer_cw+=c.nearer_cw;t.sum.alpha_met+=c.alpha_met;
            for(uint32_t k=0;k<kRtViewFamilies;++k){const uint32_t* w=words.data()+16+kRtViewFamilyWords*k;   // I-0119 R3b: the families' checks, summed
                const auto bias=[](uint32_t x){return static_cast<int64_t>(static_cast<int32_t>(x));};   // R3c: a sample's signed sums
                t.families[k].Add({w[0],w[1],w[2],w[3],w[4],w[5],w[6],w[7],{bias(w[8]),bias(w[9]),bias(w[10])},bias(w[11]),w[12],w[13],w[14],w[15],bias(w[16]),w[17],w[18]});}});   // I-0125 R6a: the ambient
        traced=true;
    }catch(const std::exception& error){FailRtView(error.what());}   // the tool off for the session (the pass and barriers recorded so far stay valid)
    DepthPlaneTransition(list,depth,depth_read,D3D12_RESOURCE_STATE_DEPTH_WRITE);
    gbuffer(false);
    // The readbacks are in the plan key (bit 43): on a frame that traced nothing they still run, their files named untraced (step
    // review L4: the pictures then hold an earlier sample, or nothing yet).
    if(!file.empty()){const auto uav=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;if(!traced)file+=L"-untraced";
        RecordPixels(list,slot,rt_view_images_[0].Get(),uav,file+L"-traced.rgba","native rt view picture write",4,uav);
        RecordPixels(list,slot,rt_view_images_[1].Get(),uav,file+L"-classes.rgba","native rt view picture write",4,uav);
        RecordPixels(list,slot,rt_view_images_[2].Get(),uav,file+L"-materials.rgba","native rt view picture write",4,uav);}
}
// After the queue's drain only (final review L6): its root signature and pipeline are not held by the frames that recorded them;
// the callers are the renderer's close and RtViewFrame's first creation or a target generation's change, both after a drain.
void SceneRenderer::CloseRtView() noexcept {
    if(auto* const heap=rt_heap_.load(std::memory_order_acquire))
        for(auto& slot:rt_view_slots_)if(slot!=BindlessIndices::none){try{heap->Release(slot,gpu_.LastSubmitted());}catch(...){}slot=BindlessIndices::none;}
    rt_view_images_={};rt_view_counts_.Reset();rt_view_readbacks_={};rt_view_.Close();rt_view_generation_=UINT64_MAX;rt_view_file_.clear();
}
}
