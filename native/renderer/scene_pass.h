#pragma once
#include "backend.h"
#include "scene_view.h"
#include "frame_plan.h"
#include "frame_graph_shadow.h"
#include "primitive_pass.h"
#include "model_frame.h"
#include "display_stage.h"
#include "output_stage.h"
#include "overlay_pass.h"
#include "mask_stage.h"
#include "work_pool.h"
#include "frame_pacer.h"
#include "streamline_host.h"
#include "rt_denoise.h"
#include "gpu_zones.h"
#include <cmath>
#include <map>
#include <mutex>
#include <optional>
#include <memory>
#include <unordered_map>
#include <atomic>

namespace ds3native {
namespace frame {struct PrimitiveWorkPacket;}
struct SceneTexture {
    uint32_t shader_slot{},width{},height{};
    DXGI_FORMAT format{};
    std::vector<std::vector<uint8_t>> mips;
};
enum class SceneShaderStage : uint8_t { Vertex,Pixel };
struct SceneVertexElement {
    std::string semantic;
    uint32_t semantic_index{},byte_offset{};
    DXGI_FORMAT format{};
};
struct SceneConstantBinding {
    SceneShaderStage stage{};
    uint32_t shader_register{};
    std::vector<uint8_t> bytes;
};

// I-0068 BINK-NATIVE-READY §2-§4: one decoded Bink frame, copied out of Bink's ready plane set on the game thread
// (the async decode threads reuse the buffers), drawn by the game's own conversion programmes into the movie output.
struct NativeMovieFrame {
    uint32_t width{},height{};                    // the video (HBINK +0/+4): viewport and output size
    uint32_t luma_width{},luma_height{},chroma_width{},chroma_height{};   // Bink buffer sizes (+0x6C..+0x78)
    std::vector<uint8_t> luma,chroma_r,chroma_b;  // packed rows (width bytes each), R8_UNORM
    std::array<float,36> constants{};             // the 9 x float4 block of 141ADFF10 (§3)
    std::vector<uint8_t> vertex_shader,pixel_shader;   // the game's DXBC (1446CAAB0, 1446CAD50)
    std::array<float,8> quad{};                   // the unit quad (143A9CE38), triangle strip, stride 8
};
// R13 cR2 (R13/R14 sheet 4.1, X1, X2, X10): one frame as an immutable job, built on the engine thread and recorded by
// ExecuteFrame (inline on the owner thread until cR3 moves it to the render worker). Every pointer keeps what the frame
// reads alive until the job is gone; the renderer's slot keeps the GPU-referenced parts until the frame's ticket.
// I-0113 tranche 27: a menu view's GX tone map (world/menu_tonemap, YEBIS-CHAIN-CHECKLIST P23): with `tabled` (mode 0 or 7 with
// UseTable) the composition draws TabledToneMapPS (x = colour * scale, y = x / (x + 0.2), the mode's 1024-entry 8-bit table at y,
// times scaler); otherwise the sRGB encode as before (the other modes are not ported).
struct NativeOffscreenTone {bool tabled{};uint32_t mode{};float scale{1.0f},contrast{0.5f},gamma{2.2f};std::array<float,3> scaler{1.0f,1.0f,1.0f};};
struct NativeOffscreenViewJob {
    std::shared_ptr<const NativeModelFrameInput> frame;
    Microsoft::WRL::ComPtr<ID3D12Resource> target;std::shared_ptr<const void> target_owner;   // the view's texture record
    std::array<float,4> background{};
    std::wstring pixel_file;
    NativeOffscreenTone tone;   // I-0113 tranche 27
};
class PrimitivePass;
// A failure of the job's movie or of one of its menu views, distinct from a failure of the frame itself (the owner refuses
// each with its own reason, as when they were separate calls).
struct NativeFrameStageError : std::runtime_error {
    enum class Stage : uint8_t {Movie,View} stage;
    NativeFrameStageError(Stage s,const char* message):std::runtime_error(message),stage(s){}
};
struct NativeFrameJob {
    uint64_t serial{};
    std::shared_ptr<const NativeModelFrameInput> models;     // the world frame, complete before it enters the job (null for title, menu, loading, movie frames)
    std::optional<NativeMovieFrame> movie;                   // the latest Bink frame, into `movie_target`
    Microsoft::WRL::ComPtr<ID3D12Resource> movie_target;std::shared_ptr<const void> movie_target_owner;
    std::wstring movie_pixel_file;
    std::vector<NativeOffscreenViewJob> views;               // menu views, recorded in this order before the frame
    // X1: the UI build this frame draws (admitted on the engine thread, immutable afterwards), its generation and
    // whether the frame initializes the UI stencil; null when the frame has no UI.
    std::shared_ptr<const PrimitivePass> ui;uint64_t ui_generation{};bool initialize_ui_stencil{};
    // I-0078 (ER: the render thread records what the frame draws): with the render worker, the UI packet itself, built by the
    // worker at the start of the job (AdmitPrimitiveWork on the render thread) instead of `ui` on the engine thread.
    std::shared_ptr<frame::PrimitiveWorkPacket> ui_packet;
    Microsoft::WRL::ComPtr<ID3D12Resource> scene_copy;       // X2: the scene-colour copy the forward pass writes (t45)
    // I-0107: the light grid the main view's GXLightOrganize writes and its forward rows read (t59..t62), as `scene_copy`.
    struct LightGridSet {Microsoft::WRL::ComPtr<ID3D12Resource> indices,lookup,work,points,spots,lines;uint32_t tiles16{};uint64_t serial{};} light_grid;
    std::wstring velocity_file;   // R13 cR3 (4.1): the 1186 motion-vector picture of this frame (witness), by value
    std::wstring pixel_file;      // W10 TM-2: the presented picture of this world frame (witness, world-pictures.flag), by value
    bool pixel_only{};            // I-0128 R8d (world-burst.flag): that picture alone (no HDR, G-buffer or GI mean beside it)
    std::wstring rt_view_file;    // I-0117 T4: the control view's pictures of this frame (rt-view.flag), the base of their two names
    DisplayCurve display_curve;   // W3: the gamma curve of this frame (null keeps the current one), made on the engine thread
    // I-0111 tranche 2 (display-clock.flag): the engine's frame-start wait engaged for this frame: the Present interval (1, or 2 on
    // a 120 Hz screen); 0 when not. Decided on the engine thread before the job is posted (step review M1), read by the worker.
    uint32_t display_interval{};
};
// Owns native G-buffer execution, material-color composition and presentation.
// No old surface, command list, device or output publication enters this API.
// I-0127 (effect-measure.flag): the effect stage's frames over a window: sums and maxima of EffectFrameMeasure, per-frame
// histograms in 10-microsecond buckets (the last one open), the GPU block's time (its own frames: a slot's timestamps arrive
// at its release).
// I-0113 tranche 29 (effect-scan.flag, gap 16): a frame's float picture scanned on the CPU at the slot's release, by channel R, G, B,
// A: NaN, +-inf and negative (not -0) half floats, the box of the pixels holding one (non-finite, then negative: x0, y0, x1, y1
// exclusive; empty when none). `stage` 0: the scene copy the passes after 0x29 sample, as remade before `pass` (0x2D or 0x31);
// 1: the lit HDR after the effect passes and the 0x31 layer's composite; 2: the scene copy then, holding what tranche 23's
// per-quad copies (CopyRect) wrote into it during the passes, so what a later quad sampled inside its rectangle (step review M2);
// 3: the lit HDR before the effect block (before the 0x29 rows and effects).
// The scan reads the whole picture back every effect frame that samples the copy: a scan run's timings are not representative
// (step review M1).
struct EffectScanRow {
    uint64_t serial{};uint32_t stage{},pass{},width{},height{};std::array<uint32_t,4> nan{},inf{},neg{};std::array<uint32_t,4> bad_box{},neg_box{};
};
// I-0113 tranche 30 (effect-scan-draws.flag, gap 16): which statement of the effect block writes the negatives. On the GPU, after
// each drawing statement of pass 0x29 (kind 1: an effect draw or a GPU-particle draw, `draw` its identity), after the 0x29 rows drawn
// before a statement (kind 2, `key` the statement's) and before and after the block (kinds 0 and 3), the lit HDR's pixels with a
// negative channel are counted (by channel, the pixels, each channel's most negative value, their box, the NaN pixels). A row is
// kept for a point whose negative pixels, or one channel's, grew by more than kEffectDrawScanGrowth since the point before it, or
// whose most negative value of a channel deepened by a quarter and at least 0.05 (the step review's L1: a statement deepening negatives
// already there), and for a baseline that already holds some. The VS and PS bytes ride with the row (dumped beside the journal by
// key and content). A frame's points stop at kEffectDrawScanPoints - 1 (the last kept for a closing kind, 3 or 5), the points refused
// counted. Tranche 31: the scan goes on through pass 0x2D, a point after each of its drawing statements (kind 4) and one after it
// (kind 5), kind 3 then marking the 0x29 pass's end where 0x2D opens.
// With effect-measure.flag the scan's dispatches fall inside the block's two timestamps and its rebinds count as row runs.
struct EffectDrawScanPoint {
    uint32_t kind{};int32_t pass{};uint32_t key{},op{},blend{},depth{},raster{},targets{};std::array<uint8_t,5> textures{};uint64_t vs{},ps{};
    std::shared_ptr<const std::vector<uint8_t>> vs_bytes,ps_bytes;};
struct EffectDrawScanRow {
    uint64_t serial{};uint32_t point{},points{};EffectDrawScanPoint at;uint32_t pixels{},before{},nan{};std::array<uint32_t,4> neg{},box{};std::array<float,3> peak{},peak_before{};};
struct EffectScanWindow {uint64_t frames{},flagged{},skipped{};std::vector<EffectScanRow> rows;   // pictures scanned, flagged, not scanned (no buffer)
    // tranche 30: frames, points read, rows kept or dropped, frames not scanned, points refused at the cap
    uint64_t draw_frames{},draw_points{},draw_flagged{},draw_skipped{},draw_capped{};std::vector<EffectDrawScanRow> draw_rows;};
struct EffectMeasureWindow {
    uint64_t frames{},draws{},draws_max{},statements{},pipeline_changes{},arena_bytes{},arena_max{},cpu_ticks{},cpu_ticks_max{},callback_ticks{};
    uint64_t gpu_frames{},gpu_ticks{},gpu_ticks_max{};uint64_t cpu_frequency{},gpu_frequency{};
    std::array<uint64_t,EffectCallKinds> calls{},repeats{};std::array<uint32_t,128> cpu_histogram{},gpu_histogram{};
};
inline constexpr uint32_t kShareRendererNoGi=1,kShareRendererNoSpecular=2;   // SceneRenderer::share_variant_off (I-0141 tranche 27)
class SceneRenderer {
public:
    SceneRenderer()=default;
    SceneRenderer(const SceneRenderer&)=delete;
    SceneRenderer& operator=(const SceneRenderer&)=delete;
    ~SceneRenderer();
    // Native startup owns the window/device before CPU scene assets arrive.
    // Loading requests are distinct from current engine content requests.
    // D3D12 phase D cD6: the number of frame slots (1 = each frame waited for, as before the ring; 2 or 3 = frames in
    // flight), fixed before Start (the bootstrap: two, one under one-frame-slot.flag).
    void SetFrameSlots(uint32_t slots){if(open_ || !slots || slots>max_frame_slots)throw std::invalid_argument("frame slots");frame_slots_=slots;}
    uint32_t FrameSlots() const noexcept {return frames_.Slots();}
    // Phase E E2 (graph-shadow.flag): the main view's frames are recorded with the frame graph's shadow (frame_graph_shadow.h);
    // the mismatch rows of the shadowed frames are taken by the owner's log (at most 64 kept), with the frame and use counts.
    void SetGraphShadow(bool enabled) noexcept {graph_shadow_enabled_=enabled;}
    // Phase E E3 (the default since D-0171): the shadow plus its executor (frame_graph_shadow.h, Execute): a frame whose structure
    // the shadow already found equal takes the graph's barriers.
    void SetFrameGraph(bool enabled) noexcept {graph_shadow_.SetExecutor(enabled);if(enabled)graph_shadow_enabled_=true;}
    uint64_t GraphExecutedFrames() const noexcept {return graph_executed_frames_.load();}
    uint64_t GraphBarriers() const noexcept {return graph_barriers_.load();}           // E3: transitions and ResourceBarrier calls the graph recorded
    uint64_t GraphBarrierCalls() const noexcept {return graph_barrier_calls_.load();}
    std::vector<std::string> TakeGraphShadowRows();
    uint64_t GraphShadowFrames() const noexcept {return graph_shadow_frames_.load();}
    uint64_t GraphShadowMismatchFrames() const noexcept {return graph_shadow_mismatch_frames_.load();}
    uint64_t GraphShadowUses() const noexcept {return graph_shadow_uses_.load();}   // tracked uses compared since start
    // D3D12 phase D cD8 (R14a-b): back buffers (2 = the former chain, up to 4) and the frame-latency waitable object (its
    // maximum latency is the slot count), fixed before Start (the bootstrap: three and waitable, two under no-waitable-present.flag). WaitFrameSlot waits on it.
    void SetPresentation(uint32_t buffers,bool waitable){if(open_ || buffers<2 || buffers>4)throw std::invalid_argument("presentation");
        present_buffers_=buffers;present_waitable_=waitable;}
    // I-0088 P0 (original-unlock.flag, measurement only): ask for a chain that may tear; PresentTearing says what Start got.
    // I-0132 R12f: the bootstrap asks for it whenever the presentation is paced (the V-sync switched live, SetPresentSyncLive); an
    // interval-0 Present then tears only without V-sync, unless SetPresentTearSynced (original-unlock.flag: it tears with it too).
    void SetPresentTearing(bool requested){if(open_)throw std::logic_error("present tearing after start");present_tearing_=requested;}
    void SetPresentTearSynced(bool tear){if(open_)throw std::logic_error("present tearing after start");present_tear_synced_=tear;}
    // I-0138 tranche 5 (HDR10, step 1; hdr-output.flag and hdr-output-force.flag until the settings rows): before Start, what the
    // output asks for. Start and every resize of the presentation probe the window's screen (ProbeHdrDisplay) and ask the
    // presenter for an HDR10 chain when the mode is Force, or Auto on a screen whose Windows HDR is on; only with the display
    // stage's source (the frame's 8-bit picture is its intermediate). On an HDR10 chain the frame's last pass is OutputStage's
    // in place of the game's brightness pass. HdrOutput says what the chain got (after Start, on its thread).
    void SetHdrOutput(const hdr::Request& request){if(open_)throw std::logic_error("HDR output after start");hdr_request_=request;}
    struct HdrOutputState {hdr::Request request;hdr::Display display;bool hdr10{},format10{};float peak{};std::string failure;};
    bool Hdr10Chain() const noexcept {return presenter_.hdr10;}   // the chain's kind now (the engine thread's read each frame, as HdrOutput's)
    HdrOutputState HdrOutput() const {return {hdr_request_,hdr_display_,presenter_.hdr10,presenter_.Hdr10Format(),hdr_peak_,hdr_refusal_.empty()?presenter_.hdr_failure:hdr_refusal_};}
    uint64_t OutputPasses() const noexcept {return output_.ScenePasses();}
    // Step 3 (the settings' rows): the request changed while the game runs, and the screen followed. HdrOutputStale (the engine
    // thread, each frame, no DXGI call but IsCurrent): the window is on another monitor than at the last probe, or DXGI's
    // factory of that probe is no longer current (Windows' HDR switched, a screen plugged). SetHdrOutputLive (between frames, the
    // worker quiesced, as a resize): the new request and a new probe; the chain is resized, to change its format, only when what
    // it is asked changes (a new paper white or peak is only a number). True when it was resized.
    bool HdrOutputStale() const noexcept;
    bool SetHdrOutputLive(const hdr::Request& request);
    // The chain's format is the one it is asked (false after a resize the system refused: the chain kept its buffers).
    bool HdrOutputSettled() const noexcept {return presenter_.Hdr10Format()==(presenter_.hdr_wanted && !presenter_.hdr_refused);}
    // Every frame of an HDR10 chain ends in the output stage's float picture: a world frame with our tone map draws its scene
    // there (the tone map's HDR entry, the glow), any other frame's 8-bit picture is copied there (OutputStage::RecordCopy), a
    // startup frame starts there from black; the game's UI and our overlay then draw on it in place, and the output pass
    // (RecordScene) writes the back buffer. The tone map's HDR draws, the frames whose 8-bit picture was copied, the frames
    // whose UI build was made for the other chain format and was not drawn (none expected: a build is made with its frame),
    // and why the tone map's HDR entry was given up for the session (its frames then go through the 8-bit picture and the copy).
    uint64_t ToneHdrFrames() const noexcept {return post_tone_.HdrFrames();}
    uint64_t HdrCopyPasses() const noexcept {return output_.CopyPasses();}
    uint64_t UiFormatSkips() const noexcept {return ui_format_skips_.load(std::memory_order_relaxed);}
    bool HdrSceneFailed() const noexcept {return hdr_scene_failed_.load();}
    const std::string& HdrSceneFailure() const noexcept {return hdr_scene_failure_;}   // read once HdrSceneFailed
    // I-0138 tranche 6: a candidate look's pipeline could not be made (the frames keep the record's own curve).
    bool ToneLookFailed() const noexcept {return tone_look_failed_.load();}
    const std::string& ToneLookFailure() const noexcept {return tone_look_failure_;}   // read once ToneLookFailed
    bool PresentTearing() const noexcept {return presenter_.tearing;}
    // I-0098 (frame-dump.flag): the display count also keeps each sample's frames and refreshes (DisplayRefreshes::KeepHolds).
    void SetDisplayHolds(bool keep){if(open_)throw std::logic_error("display holds after start");display_refreshes_.KeepHolds(keep);}
    // I-0111 (R1; at every start unless no-dlss.flag): before Start, the folder of Streamline's DLLs (the module's) and where its warnings go; Start then
    // initialises it, hands it the device and makes the chain through its factory. UpscalerState says what Start got.
    // I-0131 R10b: `frame_generation` (the settings' frame_generation) also loads DLSS-G; once it is loaded the chain is made without the frame-latency
    // object (Streamline's pacer owns it, ProgrammingGuideDLSS_G §12.1): FrameGenerationLoaded(), and FrameGeneration() once the player wants it too.
    // `frame_generation`: the multiplier asked (0 off, 2 to 4: frames presented a rendered frame, capped by the adapter's maximum).
    // I-0132 R12f: `offer` (the settings live) also loads DLSS-G at Streamline's start when it is not asked, then unloads it before the
    // chain is made, so SetFrameGenerationLive can turn it on later (ProgrammingGuideDLSS_G §18).
    void SetUpscaler(std::wstring folder,void (*sink)(const char*),uint32_t frame_generation=0,bool offer=false){if(open_)throw std::logic_error("upscaler after start");
        upscaler_folder_=std::move(folder);upscaler_sink_=sink;frame_generation_requested_=frame_generation;frame_generation_offer_=offer;}
    // After Start: DLSS-G loaded and the player's frame generation on (I-0132 R12f: the plugin's state and the player's apart, FG's step review M1).
    bool FrameGeneration() const noexcept {return frame_generation_.load(std::memory_order_relaxed) && fg_wanted_.load(std::memory_order_relaxed);}
    bool FrameGenerationLoaded() const noexcept {return frame_generation_.load(std::memory_order_relaxed);}   // DLSS-G loaded (Reflex then required, §8)
    // I-0132 R12g (R12f's final review L7, ProgrammingGuideDLSS_G §6.4: DLSS-G off in full-screen menus): set by the engine thread while one of
    // the game's in-game menus is open (FollowNativeLiveSettings), DLSS-G's mode Off at each Present then (its resources kept, §6.4's flag), On
    // again at the first Present after it closes; the player's choice and the chain unchanged.
    std::atomic<bool> fg_menu_hold{};
    // I-0132 R12f (Thomas, 2026-11-11 06:22: « c'est pas possible de tout faire en temps réel ? »): frame generation switched while the
    // game runs, between frames (the engine thread, the worker quiesced: WithPresentationIdle), as NVIDIA's guide asks (§18): the queue
    // drained, DLSS-G off, the chain released (out of exclusive full screen first), DLSS-G loaded or unloaded, the chain made again at the
    // same size (with our frame-latency object only without DLSS-G, §12.1), then the exclusive full screen given back. `multiplier` 0 off,
    // 2 to 4 on; the same state only changes the multiplier. False when frame generation is not offered or Streamline refused the change
    // (the chain then made again as it was). A failure to make the chain throws (the caller's refusal ends the run, as a resize's).
    bool SetFrameGenerationLive(uint32_t multiplier,std::string& reason);
    bool FrameGenerationOffered() const noexcept;
    const std::string& UpscalerState() const noexcept {return upscaler_state_;}   // read after Start, on its thread
    void Start(HWND window,bool warp,bool debug,uint32_t width,uint32_t height);
    // I-0111 tranche 3 (UPSCALER-DLSS-READY §3.1): the world's render extent (the DLSS mode's render size), at most the presentation's,
    // and the mode DLSS evaluates with; the presentation's size (or 0) returns to it. Between frames (the engine thread, the worker
    // quiesced): a new extent waits for the frames in flight and remakes the world's targets, as Resize; a Resize to another
    // presentation extent returns to it (the same extent keeps it: a full-screen toggle).
    void SetRenderExtent(uint32_t width,uint32_t height,streamline::DlssMode mode);
    std::array<uint32_t,2> RenderExtent() const noexcept {return {render_width_?render_width_:presenter_.width,render_height_?render_height_:presenter_.height};}
    // Installed CPU asset ingress shares this native device/queue. The owner
    // must serialize uploads with every render/resize/close (including fence
    // waits); this accessor supplies no legacy graphics object or readiness.
    DeviceQueues& CpuIngressDevice() noexcept {return gpu_;}
    // Actual completed loading presentation, separate from scene readiness.
    bool ReadStartupView(uint32_t& width,uint32_t& height) const noexcept;
    FrameReceipt RenderStartup(uint64_t serial);
    // Draw a genuine admitted primitive transaction before any zone is ready.
    // Stencil initialization is explicit; this entry sets no game readiness.
    FrameReceipt RenderStartupPrimitives(uint64_t serial,uint64_t generation,bool initialize_stencil);
    // R13 cR2: records and submits one frame job (movie, menu views, then the world frame or the UI frame) in the frame's
    // single list; inline on the owner thread until cR3. The receipt is the frame's (X10: with a worker, the previous job's).
    // R13 cR5: `prepared` is the world frame the render worker prepared for this job (the default since cR6), used when the job
    // carries none.
    FrameReceipt ExecuteFrame(const NativeFrameJob& job,std::shared_ptr<const NativeModelFrameInput> prepared={});
    // R13 cR3 (T-R5 affinity, in every build: one load per call): the render worker's thread. Once set, the frame entries
    // (ExecuteFrame, WaitFrameSlot) refuse any other thread and the engine-thread entries (AdmitPrimitiveWork,
    // SceneColourCopy) refuse the worker. 0 (the default) = everything on the owner thread, as before cR3.
    void SetRenderThread(DWORD thread) noexcept {render_thread_.store(thread,std::memory_order_release);}
    // I-0074 point 3c: the G-buffer draws recorded in chunks on the recording pool (on by default; no-record-chunks.flag for A/B).
    void SetRecordChunks(bool enabled) noexcept {record_chunks_disabled_.store(!enabled,std::memory_order_relaxed);}
    struct RecordChunkCounters {uint64_t frames{},chunks{},blocks{};};   // chunked frames, chunks, constant payloads they copied
    RecordChunkCounters RecordChunks() const noexcept {return {record_chunked_frames_.load(),record_chunks_.load(),record_chunk_blocks_.load()};}
    // 1168 menu offscreen views (Status, portraits, character creation): the same G-buffer, lighting and
    // composition as the main view, on a target set of the view's own size, composed into `target` (an
    // R8G8B8A8_UNORM render target left shader-readable). cD3: recorded into the frame's single list, submitted by the frame.
    // pixel_file (development witness): the composed view image as raw RGBA rows (width*4 bytes each).
    // background: the view clear (gparam BG Color); only pixels with scene depth receive the lit composition.
    // tone (I-0113 tranche 27): the view's GX tone map applied by that composition (NativeOffscreenTone).
    void RenderNativeOffscreen(const NativeModelFrameInput& input,ID3D12Resource* target,const std::array<float,4>& background,const std::wstring& pixel_file={},const NativeOffscreenTone& tone={});
    // Bink movie frame (I-0068 BINK-NATIVE-READY): planes uploaded, converted by the game's YCbCr programmes into `target`
    // (R8G8B8A8_UNORM, shader-readable between frames, the movie output the UI samples). cD3: recorded into the frame's single list.
    // pixel_file (development witness): the converted frame as raw RGBA rows.
    void RenderNativeMovie(const NativeMovieFrame& frame,ID3D12Resource* target,const std::wstring& pixel_file={});
    static const std::array<DXGI_FORMAT,6>& ModelTargetFormats() noexcept;
    // The lit HDR image the forward pass draws into (GXLightComposite output).
    static constexpr DXGI_FORMAT ForwardTargetFormat() noexcept {return DXGI_FORMAT_R16G16B16A16_FLOAT;}
    static constexpr DXGI_FORMAT ModelDepthFormat() noexcept {return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;}
    // 1186 velocity family: the target of the Velo rows (the engine's RGBA8 encoding kept on 16 bits per channel at render
    // resolution, explicit policy: Reference1186 shows a half-size R8G8B8A8_UNORM target) and the native motion vectors
    // (previous minus current position in pixels, render resolution, unjittered; upscaler preparation).
    static constexpr DXGI_FORMAT VelocityTargetFormat() noexcept {return DXGI_FORMAT_R16G16B16A16_UNORM;}
    static constexpr DXGI_FORMAT MotionVectorFormat() noexcept {return DXGI_FORMAT_R16G16_FLOAT;}
    // D3D12 phase D cD4 (sheet G1-G4, R6; R13 X3, X6, X10): what needs a frame's GPU results is produced when its slot is
    // released, after its ticket completed (at the end of the frame with one slot, a later frame with more), and handed out
    // through this channel. A completed frame carries its serial and ticket, the sampled back-buffer hash, the debug-layer
    // errors and first message read at its release (Diagnostics() runs there only), and the 1186 velocity statistics as one
    // JSON object (empty when the frame was not sampled).
    struct CompletedFrame {
        uint64_t serial{},ticket{},pixel_hash{};uint32_t changed_pixels{},debug_errors{};bool pixels_sampled{};
        std::string diagnostic,velocity;
    };
    std::vector<CompletedFrame> TakeCompleted();   // the frames completed since the last call, oldest first
    // 1186 witness request: the next sampled frame's motion vectors as an RGBA8 picture (width*4 bytes per row; R/G = 128 +
    // 4 x the motion in pixels, B = 255 on object velocity, A = 255), written when that frame completes.
    void RequestVelocityPicture(std::wstring path);
    // cD4 (sheet R5): the wait for the slot the next frame reuses, called before the caller takes its submission lock (no
    // wait with one slot: the slot was released at the end of the previous frame). Milliseconds waited since the last call.
    void WaitFrameSlot();
    double TakeSlotWaitMs() noexcept {return frames_.TakeWaitMs();}
    double TakeSlotReleaseMs() noexcept {return frames_.TakeReleaseMs();}   // cD6: releases (draws, chunks, completions) since the last call
    // I-0090: the retired slots' holders destroyed on the ring's release thread (inline-retire.flag keeps them on the recording thread).
    void SetDeferredRelease(bool deferred) noexcept {frames_.SetDeferredRelease(deferred);}
    double TakeReleaseThreadMs() noexcept {return frames_.TakeReleaseThreadMs();}
    double TakeWaitableWaitMs() noexcept {return waitable_wait_ms_.exchange(0);}   // cD8: frame-latency waits
    double TakePresentCallMs() noexcept {return present_call_ms_.exchange(0);}   // cD8 review: the Present calls alone (W2)
    // R14 cR4 (with cD9): the original's present pacer on the presenting thread, interval 1 (the game's 60 Hz cadence; the
    // engine settings object of the original's interval, RP-1, is not read yet): paced to 1/60 s, then the pacer's one rule
    // (I-0097 tranche 4, D-0212: the fitting refreshes, Present(1) on other rates, Present(0) on slower screens), or Present(0) without V-sync (SetPresentSync(false): the chain should allow tearing, SetPresentTearing).
    // Off (legacy-present.flag, runner -DisplayFlags legacy-present): Present(0) at once, as before. Fixed before Start.
    void SetPresentPacing(bool enabled){if(open_)throw std::logic_error("present pacing after start");present_pacing_=enabled;}
    // I-0114: with an unlocked rate DS3's limiter (or the display clock) is the cap: no 1/60 s pacing; Present(1) with V-Sync, Present(0) without.
    void SetPresentUnpaced(bool unpaced){if(open_)throw std::logic_error("present unpaced after start");present_unpaced_=unpaced;}
    void SetPresentSync(bool sync){if(open_)throw std::logic_error("present sync after start");pacer_.SetSync(sync);}
    // I-0132 R12f (the settings live): the V-sync and the unlocked rate's presentation changed while the game runs (the engine thread,
    // at a frame's start; the presenting thread takes them at its next Present).
    void SetPresentSyncLive(bool sync) noexcept {pacer_.SetSync(sync);}
    void SetPresentUnpacedLive(bool unpaced) noexcept {present_unpaced_.store(unpaced,std::memory_order_relaxed);}
    uint32_t RefreshHz() const noexcept {return refresh_hz_.load();}
    void SetJobDisplayInterval(uint32_t interval) noexcept {display_interval_job_=interval;}   // the render thread, at a job's start
    // I-0111 tranche 2 (PCL): the serial of the world frame the render thread runs (0 between jobs and for start-up frames), whose
    // submit-end and present markers PresentPaced sends.
    void SetJobSerial(uint64_t serial) noexcept {job_serial_=serial;}
    // I-0111 tranche 2 (the modern cadence, form B, display-clock.flag; CADENCE-REFLEX-READY §2): the frame-start wait on the engine
    // thread. DecideDisplayClock, before a frame's job is posted, engages it only while V-Sync is on, the chain is waitable, the
    // window visible, the screen fits 1/60 s steps (DisplayClock::Fits) and DS3's limiter runs at 1/60 s (`limiter_period`, seconds);
    // its result is the job's `display_interval`, with which the worker skips the frame-latency object (one waiter a frame) and
    // presents at that interval at once (no pacer). WaitDisplayReady, at the end of EndFrame, waits for the frame just decided,
    // `deadline` DS3's limiter's next one in performance-counter ticks. With the flag the chain's maximum latency is 1 when the screen
    // fits at Start (the object then released as the compositor takes the previous present: the display's event; step review H-A,
    // L-E; second review M2: 2 elsewhere, where the worker waits).
    void SetDisplayClock(bool requested){if(open_)throw std::logic_error("display clock after start");display_clock_requested_=requested;}
    bool DisplayClockRequested() const noexcept {return display_clock_requested_;}
    uint32_t DecideDisplayClock(float limiter_period);
    DisplayClock::Outcome WaitDisplayReady(int64_t deadline);
    struct DisplayClockCounts {uint64_t waits{},engaged{},released{},stored{},timeouts{},skipped{},probed{},owed_taken{};double waited_ms{},lead_ms{};};
    DisplayClockCounts TakeDisplayClockCounts() noexcept {auto taken=display_clock_counts_;display_clock_counts_={};return taken;}   // the engine thread's
    std::vector<float> TakePresentIntervals();   // milliseconds between consecutive presents since the previous call
    // I-0097: the display side (IDXGISwapChain::GetFrameStatistics, DisplayRefreshes) since the previous call.
    DisplayRefreshes::Counts TakeDisplayRefreshes();
    uint64_t WaitableTimeouts() const noexcept {return waitable_timeouts_;}
    // D3D12 review R15 (phase A): development witnesses in the in-game frame path (backbuffer readback and hash every 120th
    // frame, velocity readback every 30th). A measurement run clears it; explicit pixel or velocity files still read back.
    bool witness_diagnostics{true};
    // I-0113 (final review L6): GB0..GB4 read back beside each world picture only with gbuffer-pictures.flag (set at startup).
    bool gbuffer_pictures{};
    // C4 (I-0068 NPC-FAMILIES-STATIC §5.2, FFX-FAMILIES-PHASE2 §8): the scene-colour copy (pipeline+0x1108), the lit HDR copied
    // once per frame at the start of the forward pass, read as t45 by the forward programmes (RecursiveCrystal refraction).
    // Created on first request at the presentation size (pixel-shader resource between frames); the main view copies into it
    // from then on. Callers wrap the returned resource as a scene texture.
    Microsoft::WRL::ComPtr<ID3D12Resource> SceneColourCopy();
    // DECALS-WAVE1 D1: the copies the deferred decals sample, created on first request at the presentation size (pixel-shader
    // resources between frames; recreated after a size change, the earlier ones kept by their holders): GB0 (RGBA16F, t13
    // g_LightPrepassAttrTexture), GB1 (RGBA8, t14) and the depth (R32G8X24 typeless, t4 g_DepthTexture through its R32 plane).
    // The frame input names them; the main view copies into them after the G-buffer when it has decal draws.
    struct DecalSourceSet {Microsoft::WRL::ComPtr<ID3D12Resource> gb0,gb1,depth;};
    DecalSourceSet DecalSources();
    // I-0107 (14019EF60's sizes, grow-only with the presentation size; tiles16 = ceil(w / 16) ceil(h / 16)): the light grid of
    // GXLightOrganize, created on first request off the render thread: g_wLightIndices (tiles16 x 1024 x 8 bytes, stride 8),
    // g_lightLookup (tiles16 x 12, stride 12), g_workData (4), the point (1024 x 64), spot (1024 x 128) and line (1024 x 80)
    // light buffers; buffers in COMMON at creation, put in their states by their first frame. Callers wrap the four the forward
    // programmes read (t59..t62) as structured-buffer scene textures.
    NativeFrameJob::LightGridSet LightGridSources();
    // I-0107: the light grid refused (its buffers, root or pipeline; latched, read by the collect and the frame start) and why.
    bool LightGridFailed() const noexcept {return organize_failed_.load(std::memory_order_acquire);}
    bool RtSceneFailed() const noexcept {return rt_failed_.load(std::memory_order_acquire);}   // I-0117 (R2 T1)
    // I-0140 tranche 31 (D-0303): the Player menu views' traced self-shadow failed (off for the session, the views unshadowed), why,
    // and its counts: the views traced, those past a frame's kMenuRtSlots pairs (drawn unshadowed: their context's mask cleared white
    // while held, else t32 the neutral), the traced views whose trace drew nothing (their mask cleared white); the white clears of held
    // frames and of a refused rewrite away from the mask are not counted (the last step review's L4 and its review's L2).
    bool MenuShadowFailed() const noexcept {return menu_shadow_failed_.load(std::memory_order_acquire);}
    // The shadowed sun's programme made (the collect builds a view's traced scene only then: the step review of the fixes, L3).
    bool MenuShadowReady() const noexcept {return menu_shadow_ready_.load(std::memory_order_acquire);}
    std::string MenuShadowFailure() const {return MenuShadowFailed()?menu_shadow_failure_:std::string{};}   // written once, before the flag
    std::array<uint64_t,6> MenuShadowCounts() const noexcept {return {menu_shadow_traced_.load(std::memory_order_relaxed),menu_shadow_past_slots_.load(std::memory_order_relaxed),
        menu_shadow_refused_.load(std::memory_order_relaxed),menu_shadow_held_.load(std::memory_order_relaxed),menu_shadow_neutral_.load(std::memory_order_relaxed),
        menu_shadow_blurred_.load(std::memory_order_relaxed)};}
    std::string RtSceneFailure() const {return RtSceneFailed()?rt_failure_:std::string{};}   // written once, before the flag
    uint64_t RtSceneBytes() const noexcept {return rt_tlas_bytes_.load(std::memory_order_relaxed);}   // the top level and its scratch
    // I-0117 (R2 T3): the bindless heap the rays' passes read (opened by the start self-test, R0, after the renderer's start and
    // before the first frame); its frame owner is the recording thread from then on. Once.
    void SetRtHeap(BindlessHeap* heap) noexcept {rt_heap_.store(heap,std::memory_order_release);}
    // I-0119 R3a: the material table's counters (RtMaterialTable::Totals), as the recording thread last stored them.
    RtMaterialTable::Counters RtMaterialTotals() const noexcept {const std::lock_guard hold(rt_material_lock_);return rt_material_totals_;}
    // I-0117 T2: the skinned instances of the last frame, and since the start their builds, refits and skinned vertices; the
    // instances left out (no resource or palette); the bytes of their positions and structures with the skinning's scratch.
    // I-0117 T4 (rt-view.flag, set at startup with the rays' scene): the control view on every rt_view_every-th main view with a top
    // level, and on the frames whose job names its pictures; the samples' counts (RtViewTotals).
    // A failure turns it off for the session (step review L2, L3: read by the engine thread's rows, its reason kept).
    std::atomic<bool> rt_view{};uint32_t rt_view_every{60};
    // I-0117 R5 (set at startup with the rays' scene): the sun's shadow traced in place of the cascades' mask layer
    // (RtSunShadow) on the main view's frames with a top level; a failure turns it off for the session (the raster's mask again).
    std::atomic<bool> rt_shadow{},rt_shadow_witness{};   // the witness: rt-shadow-witness.flag (RtSunConstants::offset.w)
    std::string RtSunFailure() const {const std::lock_guard hold(rt_view_lock_);return rt_sun_failure_;}   // empty while it works
    // I-0122 R4a (set at startup with the rays' scene): the unshadowed local lights shaded natively from the frame's list (RtLocalLights)
    // in place of their batches on the main view; a frame that cannot draw it, or a failure (off for the session), draws the batches.
    std::atomic<bool> rt_lights{};
    // I-0122 R4c (set at startup with the rays: the settings' level pt or rt-pt.flag): the PT level's direct light (every light of the
    // tile casts, one sampled a pixel) on the main view's frames Ray Reconstruction denoises with a top level; the RT level otherwise.
    std::atomic<bool> rt_pt{};
    // I-0128 R8b (set at startup: the settings' bounces, rt-pt-no-cache.flag): the PT level's traced vertices a path (R8-PLAN §3), and the bias
    // check's reference (8 vertices, no cache).
    std::atomic<uint32_t> rt_bounces{kRtPathVerticesDefault};std::atomic<bool> rt_path_reference{};
    // I-0132 R12g (rt-gi-paths-N.flag, else one: fixed for the session since its settings row went; set at startup): the PT level's GI paths a pixel (rt_scene.h's kRtGiPathsMost).
    std::atomic<uint32_t> rt_gi_paths{kRtGiPathsDefault};
    // I-0128 R8c (set at startup: no-rt-light-grid.flag clears it, the equality check's other half): the hits' local lights from the
    // frame's light grid (RtLightList::grid) where the collect made one.
    std::atomic<bool> rt_light_grid{true};
    // I-0132 R11a (set at startup: no-rt-roulette.flag clears it, a test tool): the PT level's paths past their first vertex go on with
    // the probability of their throughput (Russian roulette, rt_reflect.hlsl's PathFrom), never on the reference's paths; the default
    // since R11a's measure (the GI's mean 1.0001 of the paths without it, the displayed frames as steady, 6 to 24 ms saved at 1920x1080).
    std::atomic<bool> rt_roulette{true};
    // I-0132 R11d (R11-PLAN §5; set at startup, test tools for Thomas's sheets of the real sources' settings, D-0262): the local
    // lights' colours on the rays' list (1, DS3's own, by default since D-0293, 2026-11-16; rt-local-x2.flag 2, rt-local-x4.flag 4;
    // the forward grid's lights stay DS3's own (R11e: the main view draws no batch) until the sources go physical) and the emissive surfaces' emission
    // at the rays' hits (rt-emissive-x4.flag, rt-emissive-x16.flag: kRtReflectEmissiveX4, kRtReflectEmissiveX16).
    std::atomic<float> rt_local_scale{1.f};std::atomic<uint32_t> rt_emissive_bits{};
    // I-0132 R11h (test tools, D-0264): the local lights in physical terms (RtPhysicalLight) at this ratio to the sun, in place of
    // rt_local_scale (rt-local-physical-10.flag 0.1, -50 0.02, -200 0.005); 0 off, the default; set at startup.
    std::atomic<float> rt_local_physical{0.f};
    // I-0132 R11d (set at startup: rt-fail-magenta.flag, the test runs' tool, Thomas 2026-11-09): the main view's frames the rays do
    // not fully serve while they were asked (rt_asked: the rays usable at startup) shown magenta (EncodeNativeLighting), counted with
    // the OR of their causes (1 sun, 2 top level, 4 GI, 8 local lights). R11e (D-0262, no fallback): every such frame judged, without
    // the flag its world black (the UI over it); an arrival's held frames black and counted apart (RtHeldFrames), never magenta.
    std::atomic<bool> rt_fail_magenta{},rt_asked{};
    uint64_t RtFailMagentaFrames() const noexcept {return rt_fail_magenta_frames_.load(std::memory_order_relaxed);}
    uint32_t RtFailMagentaCauses() const noexcept {return rt_fail_magenta_causes_.load(std::memory_order_relaxed);}
    uint64_t RtHeldFrames() const noexcept {return rt_held_frames_.load(std::memory_order_relaxed);}
    // I-0132 R11e: the main view's frames judged so far (bits 8 up) and the last one's causes (bits 0 to 7, 0 served), one word so the
    // collect reads both at once (NativeRtArrivalStep).
    uint64_t RtJudged() const noexcept {return rt_judged_.load(std::memory_order_acquire);}
    // I-0132 R11e step 4: the main view's unserved frames in a row outside an arrival's hold (0 once one is served) and the last one's
    // causes: a run lasting 2 s is a failure no latch names (NRD latched off on frames RR does not denoise, a scene never ready).
    uint32_t RtUnservedRun() const noexcept {return rt_unserved_run_.load(std::memory_order_relaxed);}
    uint64_t RtUnservedSince() const noexcept {return rt_unserved_since_.load(std::memory_order_relaxed);}
    uint64_t RtUnservedLast() const noexcept {return rt_unserved_last_.load(std::memory_order_relaxed);}
    // The final review's M1: the main view's held frames encoded without a judgement (no lighting input), since the start.
    uint64_t RtHeldUnjudged() const noexcept {return rt_held_unjudged_.load(std::memory_order_relaxed);}
    uint32_t RtUnservedRunCauses() const noexcept {return rt_unserved_run_causes_.load(std::memory_order_relaxed);}
    // I-0132 R11e step 4 (D-0262: a latched failure an error message and a clean close): the first latched failure of a traced pass
    // the frames need (the top level, the sun, the local lights, the reflections, the GI, the cache, the water, the probes), named;
    // empty while they run or when the rays were not asked. The optional ones (ReSTIR GI, the control view) and the denoisers' latches
    // (NRD, RR: the frame stays traced) are not in it.
    std::string RtFatalFailure() const {
        if(!rt_asked.load(std::memory_order_relaxed))return {};
        const std::pair<const char*,std::string> named[]{{"top level",RtSceneFailure()},{"sun shadow",RtSunFailure()},{"local lights",RtLightsFailure()},
            {"reflections",RtReflectFailure()},{"global illumination",RtGiFailure()},{"radiance cache",RtCacheFailure()},{"water",RtWaterFailure()},{"probes",RtProbeFailure()}};
        for(const auto& [what,why]:named)if(!why.empty())return std::string(what)+": "+why;
        return {};}
    // I-0125 R6b (set at startup with the rays): the deferred surfaces' specular ambient traced (RtReflections)
    // in place of the region draws on the main view's frames with the local lights' pass; a failure turns it off for the session.
    std::atomic<bool> rt_reflect{};
    uint64_t RtReflectFrames() const noexcept {return rt_reflect_frames_.load(std::memory_order_relaxed);}
    uint64_t RtReflectMissed() const noexcept {return rt_reflect_missed_.load(std::memory_order_relaxed);}   // frames with regions and no lights' pass
    std::string RtReflectFailure() const {const std::lock_guard hold(rt_view_lock_);return rt_reflect_failure_;}   // empty while it works
    // I-0125 R6c (set at startup with the rays and the water's reflection): the water's g_ReflectionTexture traced (RtReflections::
    // EncodeWater) on the main view's lit frames whose input carries rt_water (R6f, D-0251: the mirror view removed); a failure turns it
    // off for the session (the water then samples the black default). RtWaterDrawn: the last target a pass was recorded into (the
    // engine thread marks the reflection written from it).
    std::atomic<bool> rt_water{};
    // I-0126 R7a (R7-PLAN §1; set at startup whenever the rays are usable, D-0251 R7c: no flag): the deferred surfaces'
    // diffuse ambient traced (RtReflections::EncodeGi) in place of pass 0's GB4 on the main view's frames with the local lights' pass
    // that RR denoises or, R7c, NRD (RtNrdFrame); a failure turns it off for the session (I-0132 R11e: the game then closes with a message).
    std::atomic<bool> rt_gi{};
    std::atomic<uint32_t> rt_gi_diagnostic{};   // R7a's diagnostic bits (kRtReflectGiNo*), from their flags at startup
    // I-0141 tranche 27 (test tooling, a bounded run's share variant, witness-variant.txt): kShareRendererNoGi the traced diffuse ambient
    // not composed (EncodeRtGi returns before its encode, the frame still counted served: only the share is taken out, NRD's signal and
    // histories go on), kShareRendererNoSpecular SpecularAcc (t7: the traced reflections) cleared to zero after the reflections.
    std::atomic<uint32_t> share_variant_off{};
    // I-0126 R7d (R7-PLAN §8): the GI's bias check (rt-gi-mean.flag, a diagnostic, set at startup): each GI frame's estimate accumulated
    // (RtGiMeanFrame), written beside every world picture; a failure to make its target turns it off for the session.
    std::atomic<bool> rt_gi_mean{};
    // I-0126 R7d (R7-PLAN §8): ReSTIR GI at the PT level (set at startup; by default since I-0132 R11i, no-rt-restir.flag the PT level's one-ray GI), its
    // resampling stages (kRtRestirTemporal, kRtRestirSpatial; rt-gi-no-temporal.flag and rt-gi-no-spatial.flag, the bias check's
    // diagnostics), its frames and why it is off once it failed. I-0128 R8d: its decorrelation in the same word (kRtRestirPermute, the cap
    // and the share: rt_scene.h).
    std::atomic<bool> rt_restir{};std::atomic<uint32_t> rt_restir_stages{kRtRestirTemporal|kRtRestirSpatial|kRtRestirPermute|102u<<kRtRestirShareShift};
    uint64_t RtRestirFrames() const noexcept {return rt_restir_frames_.load(std::memory_order_relaxed);}
    // I-0132 R11b (R11-PLAN §13; set at startup: no-rt-ser.flag clears it, the test tool): shader execution reordering on the PT level's
    // path programmes (RtReflections::MakeSer), its frames and why it is off once it could not be made (the pixel shaders then).
    std::atomic<bool> rt_ser{true};
    uint64_t RtSerDispatches() const noexcept {return rt_reflect_pass_.SerDispatches();}   // the reordered dispatches, up to two a frame (the step reviews' L2)
    std::string RtSerFailure() const {const std::lock_guard hold(rt_view_lock_);return rt_ser_failure_;}
    std::string RtRestirFailure() const {const std::lock_guard hold(rt_view_lock_);return rt_restir_failure_;}
    uint64_t RtGiFrames() const noexcept {return rt_gi_frames_.load(std::memory_order_relaxed);}
    uint64_t RtGiMissed() const noexcept {return rt_gi_missed_.load(std::memory_order_relaxed);}   // GI frames that added no bounce (no inputs, a ReSTIR, NRD or GI failure)
    std::string RtGiFailure() const {const std::lock_guard hold(rt_view_lock_);return rt_gi_failure_;}   // empty while it works
    // I-0126 R7c: the GI frames NRD denoised (RR off), and why NRD is off once it failed (empty while it works).
    uint64_t RtNrdFrames() const noexcept {return rt_nrd_frames_.load(std::memory_order_relaxed);}
    uint64_t RtNrdMissed() const noexcept {return rt_nrd_missed_.load(std::memory_order_relaxed);}   // traced RR-off frames NRD did not serve
    std::atomic<bool> rt_nrd_off{};   // NRD latched off for the session (the render thread sets it; the collect reads it: the fixes' step review L-d)
    std::string RtNrdFailure() const {const std::lock_guard hold(rt_view_lock_);return rt_nrd_failure_;}
    // I-0126 R7b (R7-PLAN §3): the radiance cache (RtReflections::EncodeCache) on the GI frames, its update and resolve before the
    // reflections, so the frame's hits (the reflections', the water's, the GI's) take their ambient from it; set at startup with the
    // GI unless no-rt-cache.flag (a diagnostic: the hits get no ambient; I-0132 R11d, D-0262: R7a's baked ambient removed). A failure turns
    // the cache off for the session (the GI goes on with no ambient at its hits).
    std::atomic<bool> rt_cache{};
    uint64_t RtCacheFrames() const noexcept {return rt_cache_frames_.load(std::memory_order_relaxed);}
    std::string RtCacheFailure() const {const std::lock_guard hold(rt_view_lock_);return rt_cache_failure_;}   // empty while it works
    std::atomic<bool> rt_sky_witness{};   // I-0125 R6d: rt-sky-witness.flag (kRtReflectSkyWitness), a diagnostic
    // I-0129 R9a (R9-PLAN §2): the traced probes (RtReflections::EncodeProbes) on the main view's traced frames with requests, after the
    // cache (their hits' ambient from it on a GI frame); set at startup with the rays unless no-rt-probes.flag. A failure turns them
    // off for the session (the forward rows and the effects keep DS3's baked probes).
    std::atomic<bool> rt_probes{};
    uint64_t RtProbeFrames() const noexcept {return rt_probe_frames_.load(std::memory_order_relaxed);}
    std::string RtProbeFailure() const {const std::lock_guard hold(rt_view_lock_);return rt_probe_failure_;}   // empty while it works
    uint64_t RtWaterFrames() const noexcept {return rt_water_frames_.load(std::memory_order_relaxed);}
    ID3D12Resource* RtWaterDrawn() const noexcept {return rt_water_drawn_.load(std::memory_order_acquire);}
    std::string RtWaterFailure() const {const std::lock_guard hold(rt_view_lock_);return rt_water_failure_;}   // empty while it works
    uint64_t RtPtFrames() const noexcept {return rt_pt_frames_.load(std::memory_order_relaxed);}   // the frames shaded at the PT level
    bool RtPtRefused() const noexcept {return rt_lights_pass_.PtRefused();}   // its pipeline refused (the fixes' review L-a)
    // I-0123 RR-a (set at startup with the rays' scene unless no-rr.flag): DLSS Ray Reconstruction in place of DLSS on the main view's
    // DLSS frames, with its guides from the G-buffer (RrGuides) and the colour before the transparent rows; a failure to make them,
    // or RR's refusal, turns it off for the session (DLSS Super Resolution from the next frame).
    std::atomic<bool> rr{};
    std::string RtLightsFailure() const {const std::lock_guard hold(rt_view_lock_);return rt_lights_failure_;}   // empty while it works
    uint64_t RtLightsMissed() const noexcept {return rt_lights_missed_.load(std::memory_order_relaxed);}   // frames whose list was not drawn
    RtViewTotals RtViewSamples() const {const std::lock_guard hold(rt_view_lock_);return rt_view_totals_;}
    std::string RtViewFailure() const {const std::lock_guard hold(rt_view_lock_);return rt_view_failure_;}   // empty while it works
    RtSkinCounters RtSkinTotals() const noexcept {return {rt_skin_instances_.load(std::memory_order_relaxed),rt_skin_builds_.load(std::memory_order_relaxed),
        rt_skin_refits_.load(std::memory_order_relaxed),rt_skin_vertices_.load(std::memory_order_relaxed),rt_skin_failed_.load(std::memory_order_relaxed),rt_skin_bytes_.load(std::memory_order_relaxed),
        rt_skin_winds_.load(std::memory_order_relaxed),rt_skin_wind_vertices_.load(std::memory_order_relaxed)};}   // T6: the winds since the start
    std::string LightGridFailure() const;
    // The decals' seven targets: SpecularAcc (R11G11B10), GB1, GB2, GB3, GB4, GB5, GB0 (the original's order, Decal7).
    static const std::array<DXGI_FORMAT,7>& DecalTargetFormats() noexcept;
    // DECALS-PAINT-READY rung 1 (D2): a struck character's paint target, made off the render thread (the collect's): the diffuse
    // R8G8B8A8_UNORM_SRGB and specular R8G8B8A8_UNORM textures (GXPaintRenderTarget 4CFB0's 0x1D and 0x1C), size x size, render
    // targets left PIXEL_SHADER_RESOURCE, their RTVs in the target's own heap. The caller makes the shader views.
    std::shared_ptr<NativePaintTarget> MakePaintTarget(uint32_t size);
    static constexpr std::array<DXGI_FORMAT,2> paint_formats{DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,DXGI_FORMAT_R8G8B8A8_UNORM};
    // WATER-TESS-WAVE1-READY WT-2: the readable depth the water rows sample (t56 g_DepthTexture), the depth's R32G8X24 copy at the
    // presentation size (pixel-shader resource between frames, read through its R32 plane), made on first request off the render
    // thread; the main view copies the depth into it before the WatDpt rows.
    Microsoft::WRL::ComPtr<ID3D12Resource> WaterDepthCopy();
    // W3 (GAMMA-COLOUR-PATH-READY 3.1, 3.5): the original's gamma pass. With a source set before the first frame (the
    // pinned shaders read from the image), the world and startup frames draw into the stage's intermediate and the pass
    // draws it into the back buffer through the frame's curve (NativeFrameJob::display_curve); without one (tests,
    // no-gamma.flag, a failed admission) or when the stage fails to open, the frames draw straight into the back buffer.
    void SetDisplaySource(DisplayBlobs blobs){if(open_)throw std::logic_error("display source after start");display_blobs_=std::move(blobs);}
    bool DisplayOpen() const noexcept {return display_open_.load();}
    uint64_t DisplayUploads() const noexcept {return display_.Uploads();}
    uint64_t DisplayPasses() const noexcept {return display_.Passes();}
    // I-0132 R12a: the native overlay above the game's UI (our pages): the engine thread's latest frame (null: none), drawn by the
    // recording thread after the UI and before the gamma pass in the startup and world frames; its counts taken and reset.
    void SetOverlay(std::shared_ptr<const overlay::Frame> frame){overlay_.Set(std::move(frame));}
    // I-0132 R12e: the screen layer, in the startup frames only, after the game's UI and before the overlay, with a screen blend (the
    // title screen's embers and smoke: an underlay before the UI showed nothing in r12e-film3: the title movie read as opaque); an overlay failure stops it too, its own
    // failure (screen_layer_failed_) only itself.
    void SetScreenLayer(std::shared_ptr<const overlay::Frame> frame){screen_layer_.Set(std::move(frame));}
    // R12e (Thomas, 2026-11-12 02:14): the tint layer, a multiply blend after the game's UI and before the screen layer (DS3's logo tinted
    // orange), in the startup frames only; its failure the screen layer's latch.
    void SetTintLayer(std::shared_ptr<const overlay::Frame> frame){tint_layer_.Set(std::move(frame));}
    OverlayPass::Counts TakeScreenLayerCounts() noexcept {return screen_layer_.TakeCounts();}
    OverlayPass::Counts TakeTintLayerCounts() noexcept {return tint_layer_.TakeCounts();}
    OverlayPass::Counts TakeOverlayCounts() noexcept {return overlay_.TakeCounts();}
    // The last failure of the overlay, the screen layer or the tint layer (« screen » or « tint » named; empty while none), read by the
    // engine thread's row.
    std::string OverlayFailure() const {std::lock_guard guard(overlay_failure_lock_);return overlay_failure_;}
    uint64_t GlowPasses() const noexcept {return glow_.Passes();}
    uint64_t GlowFrames() const noexcept {return glow_.Frames();}
    uint64_t TaaFrames() const noexcept {return taa_.Frames();}   // I-0132 R11j
    uint64_t TaaResets() const noexcept {return taa_.Resets();}
    bool TaaFailed() const noexcept {return taa_failed_.load();}   // latched for the session (the engine thread then stops the jitter)
    // I-0140 tranche 40 (D-0305): the menu views' TAA latched off for the session (the engine thread then stops their jitter).
    bool MenuTaaFailed() const noexcept {return menu_taa_failed_.load();}
    bool GlowFailed() const noexcept {return glow_failed_.load();}
    uint64_t TonemapFrames() const noexcept {return post_tone_.Frames();}   // I-0138 tranche 1: our pass
    // I-0132 R11h: the automatic exposure's frames, its last reading (eight frames late) and, once it failed, why.
    uint64_t AutoExposureFrames() const noexcept {return auto_exposure_.Frames();}
    AutoExposureReading AutoExposure() const {const std::lock_guard hold(auto_exposure_lock_);return auto_exposure_reading_;}
    std::string AutoExposureFailure() const {if(!auto_exposure_failed_.load())return {};const std::lock_guard hold(auto_exposure_lock_);return auto_exposure_failure_;}   // the step review's L4
    bool TonemapFailed() const noexcept {return tonemap_failed_.load();}
    const std::string& TonemapFailure() const noexcept {return tonemap_failure_;}   // read once TonemapFailed() is true
    uint64_t MotionBlurFrames() const noexcept {return motionblur_stage_.Frames();}   // W10 MB-4
    bool MotionBlurFailed() const noexcept {return motionblur_failed_.load();}
    const std::string& MotionBlurFailure() const noexcept {return motionblur_failure_;}   // read once MotionBlurFailed() is true
    uint64_t GradingFrames() const noexcept {return post_tone_.LutFrames();}   // W10 GR; I-0138 tranche 1: our pass's LUT draws
    bool GradingFailed() const noexcept {return grading_failed_.load();}
    const std::string& GradingFailure() const noexcept {return grading_failure_;}   // read once GradingFailed() is true
    uint64_t MaskTargetBytes() const noexcept {return mask_target_bytes_.load();}   // I-0126 R7c (D-0251): 0 while the stage is closed
    bool MaskFailed() const noexcept {return mask_failed_.load();}
    const std::string& MaskFailure() const noexcept {return mask_failure_;}   // read once MaskFailed() is true
    const std::string& GlowFailure() const noexcept {return glow_failure_;}   // read once GlowFailed() is true
    // FFX-DRAW N6: the effect stage's counters (draws, skipped, pipelines made and refused, textures bound and missing) and its last
    // refusal, as one JSON object (any thread).
    std::string EffectSummary() const;
    EffectMeasureWindow TakeEffectMeasure();   // I-0127: the window since the last call, reset
    EffectScanWindow TakeEffectScan();   // I-0113 tranche 29: the scans since the last call (at most kEffectScanRows rows kept), reset
    // I-0128 R8a (rt-gpu-measure.flag, set at startup): the main view's frame and its traced passes timed on the GPU (GpuZoneTimer);
    // TakeRtGpuMeasure the window since the last call, reset.
    std::atomic<bool> rt_gpu_measure{};
    GpuZoneWindow TakeRtGpuMeasure(){return rt_gpu_.Take();}
    bool DisplayFailed() const noexcept {return display_failed_.load();}   // the reason is in DisplayFailure() once this is true
    const std::string& DisplayFailure() const noexcept {return display_failure_;}
    // cR2: returns the admitted build (the frame job of this serial draws it).
    std::shared_ptr<const PrimitivePass> AdmitPrimitiveWork(const frame::PrimitiveWorkPacket& input);
    std::shared_ptr<const PrimitivePass> AdmitPrimitiveWork(frame::PrimitiveWorkPacket&& input);   // the same; the packet's payloads move into the build
    void WithdrawPrimitiveWork(uint64_t generation);
    // Engine thread: an admitted build is current (a world frame without a UI build is then refused, as before cR2).
    bool PrimitiveBuildCurrent() const noexcept {return primitives_ && primitives_->Packet();}
    bool Faulted() const noexcept {return faulted_;}
    void Resize(uint32_t width,uint32_t height);
    // I-0076: the presentation and its size-dependent targets at a new output extent, between frames (nothing recording): the
    // current UI build, made for the old extent, is withdrawn first (the next frame admits its own).
    void ResizePresentation(uint32_t width,uint32_t height);
    // I-0076 (the role of the original's GX display, DS3 FUN_14000B540 / FUN_14000B590 / FUN_140009EA0; ER 1419EF6E0): the
    // swap chain's full-screen state; a full-screen toggle (SetFullscreenState(!current, no output), then the buffers at the
    // current size), false when DXGI cannot switch now; a mode request applied in the original's order: leaving full screen,
    // ResizeTarget first; SetFullscreenState(full screen, the containing output) when the state changes; ResizeTarget to the
    // mode (in full screen the output's closest mode); then the buffers at the current size. Between frames, nothing recording.
    // Borderless (ER 140E8B8E0, DS3 screen mode 2): a windowed swap chain whose window (the engine's popup) is placed over the
    // monitor it is on (the containing output's desktop rectangle); the requested size is not used (the output follows the
    // window's client).
    bool PresentationFullscreen() const;
    // I-0101 (STUDY §3; ER FUN_1419ED670, the device frame start): a failed Present other than a lost device completes its
    // frame (both originals ignore every Present failure) and marks the presentation lost, with what the chain and the window
    // were at that moment. The engine thread asks cheaply whether to follow (that mark, or an exclusive full screen DXGI left by
    // itself, a focus loss), then follows between frames: ResizeBuffers at the chain's own size, as ER does before it presents
    // again, and the exclusive state recorded. Empty when nothing was to follow.
    // `failing`: the follows in a row with no successful Present between them (step review M2: a Present that keeps failing is
    // resized at the first three and every 60th, not every frame); `resized` whether this follow resized.
    struct PresentationLoss {HRESULT status{},resize_status{};uint64_t failures{},failing{};bool was_exclusive{},fullscreen{},windowed{},iconic{},foreground{},resized{},stand_in{};
        uint32_t width{},height{},client_width{},client_height{};};
    bool PresentationNeedsFollow() const;
    std::optional<PresentationLoss> FollowPresentation();
    bool TogglePresentationFullscreen();
    void ApplyDisplayMode(bool fullscreen,bool borderless,uint32_t width,uint32_t height);
    void Close();
private:
    FrameReceipt RenderStartupFrame(uint64_t serial,const PrimitivePass* ui,bool initialize_stencil,std::shared_ptr<const void> ui_owner={});
    // I-0074: `input_owner`, when given, owns `input`: the slot holds it, and through it every draw, instead of a copy of
    // each draw's holder (thousands of reference counts taken and dropped a frame, 5 % of the render worker at m45).
    FrameReceipt RenderModels(const NativeModelFrameInput& input,const PrimitivePass* ui,std::shared_ptr<const void> ui_owner,
        const Microsoft::WRL::ComPtr<ID3D12Resource>& scene_copy,const std::wstring& pixel_file,std::shared_ptr<const void> input_owner={});
    // D3D12 phase D cD3 (sheet 4, cD3): one list per frame. The movie, the menu views and the frame record into the slot
    // that is recording (the first of them begins it); only RenderStartupFrame and the frame job submit it.
    FrameSlot& FrameListSlot(uint8_t site);
    // Witness copies of the movie and menu view targets, written when the slot that recorded them is released (G3).
    // pixel_bytes: bytes per texel of the target (4 for the 8-bit views, 8 for the half-float lit HDR); before: its state.
    // tracked false: a surface the frame graph does not know (a stage's own, with its own barriers): the copy is recorded inside
    // the current pass, without a readback pass of its own.
    void RecordPixels(ID3D12GraphicsCommandList* list,FrameSlot& slot,ID3D12Resource* target,D3D12_RESOURCE_STATES after,
        const std::wstring& path,const char* failure,uint32_t pixel_bytes=4,D3D12_RESOURCE_STATES before=D3D12_RESOURCE_STATE_RENDER_TARGET,bool tracked=true);
    // Per-slot readbacks (G1, G2), created on first use at the presentation extent; a slot that samples holds them.
    // I-0111: the back buffer's at the presentation's extent, the velocity's at the render extent.
    struct SlotReadbacks {Microsoft::WRL::ComPtr<ID3D12Resource> backbuffer,velocity,motion;uint32_t width{},height{},velocity_width{},velocity_height{};
        DXGI_FORMAT backbuffer_format{DXGI_FORMAT_R8G8B8A8_UNORM};   // I-0138 tranche 5: the back buffer's format when the slot sampled (four bytes a pixel either way)
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT velocity_footprint{},motion_footprint{};};   // the footprints of these buffers (review)
    std::array<SlotReadbacks,max_frame_slots> slot_readbacks_;
    // cD4: the release-time work of a frame (its slot's last completion): sampled back-buffer hash and file, velocity
    // statistics and picture, diagnostics; the result goes to the channel.
    void CompleteFrame(uint64_t serial,uint64_t ticket,const SlotReadbacks& readbacks,bool pixels,const std::wstring& pixel_file,
        bool velocity,const std::wstring& velocity_file);
    SlotReadbacks& Readbacks(uint32_t slot,bool velocity);
    void TakeReleased(FrameReceipt& receipt,const char* failure);
    mutable std::mutex channel_lock_;          // the witness channel: requests in, completed frames out (X3)
    std::vector<CompletedFrame> completed_;
    std::wstring velocity_request_;
    // Errors of frames released since the frame functions last checked (they throw after their End), and the completed
    // frame of the last release (the receipt of a frame released at its own End carries its results).
    uint32_t released_errors_{};std::string released_diagnostic_;
    CompletedFrame last_completed_;
    void Targets(uint32_t width,uint32_t height);
    void DepthTarget(uint32_t width,uint32_t height,uint8_t initial_stencil);
    void MaterialColorCompositionPipeline();
    void OffscreenCompositionPipeline();
    void NativeModelTargets();
    void PrimitiveDepthTarget(uint32_t width,uint32_t height);
    uint32_t Diagnostics();
    std::string last_diagnostic_; // 1126: first warning/error text of the last Diagnostics() call
public:
    // 1130 witness: CPU milliseconds of the last AdmitPrimitiveWork steps (produce, drain, build, wait, diagnostics). I-0078: read
    // by the thread that admitted (with the render worker, the worker, which hands them to the frame's outcome).
    std::array<float,5> primitive_admission_ms{};
private:
    DeviceQueues gpu_;
    FramePresenter presenter_;
    FrameRing frames_;   // cD1; cD6: the bootstrap sets two slots by default, one under one-frame-slot.flag (SetFrameSlots)
    uint32_t frame_slots_{1};
    uint32_t present_buffers_{2};bool present_waitable_{},present_tearing_{},present_tear_synced_{};
    // cD8. R13 cR3: atomic (the frame adds on the render worker, the owner thread's pacing row takes them).
    std::atomic<double> waitable_wait_ms_{},present_call_ms_{};std::atomic<uint64_t> waitable_timeouts_{};
    bool present_pacing_{true};std::atomic<bool> present_unpaced_{};std::atomic<uint32_t> refresh_hz_{};FramePacer pacer_;   // cR4: the presenting thread (refresh read by the owner)
    // I-0111 tranche 2: the display clock's request, its engagement (written by the engine thread, read by the worker), the screen's
    // period the engine reads every 120th wait, its counts.
    bool display_clock_requested_{};uint32_t display_interval_engine_{},display_checks_{};double display_period_{};
    uint32_t display_interval_job_{};   // the render thread's: the job being run (RunNativeFrameJob sets it)
    uint64_t job_serial_{};             // the render thread's: the world frame's serial (SetJobSerial)
    std::optional<DisplayClock> display_clock_;bool display_was_engaged_{};   // the engine thread's: its owed counts kept across frames
    DisplayClockCounts display_clock_counts_;
    std::mutex present_times_lock_;std::vector<float> present_intervals_;int64_t last_present_{};
    HWND present_window_{};uint64_t presents_{};DisplayRefreshes display_refreshes_;   // cR4 review, I-0097
    HRESULT PresentPaced();   // cR4: pace, Present, record the interval
    bool AdmitPresent(HRESULT status,const char* what);   // I-0101: a lost device throws; another failure marks the presentation lost
    std::atomic<bool> presentation_lost_{};std::atomic<uint64_t> present_failures_{};std::mutex present_failure_lock_;PresentationLoss present_failure_{};
    bool exclusive_{};   // I-0101: the exclusive state the presentation last set or followed (engine thread)
    std::atomic<uint64_t> presents_ok_{};uint64_t follow_ok_seen_{},failing_follows_{};   // I-0101 review M2 (presents_ok_ on the worker)
    // I-0101 review M1: a frame whose latency wait took a count and whose Present failed gives nothing back to the waitable
    // object; the next frame skips its wait once per such count (render thread only).
    // I-0101 review M1, I-0111 tranche 2 (second review M1): the counts failed Presents never gave, consumed by whichever side waits
    // the object for the next frame (the worker, or the engine's display clock).
    std::atomic<uint32_t> latency_debt_{};bool latency_taken_{};
    bool TakeLatencyDebt() noexcept {uint32_t d=latency_debt_.load(std::memory_order_relaxed);
        while(d && !latency_debt_.compare_exchange_weak(d,d-1,std::memory_order_relaxed)){}return d!=0;}
    Microsoft::WRL::ComPtr<ID3D12RootSignature> composition_root_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> composition_pso_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> offscreen_composition_pso_;   // composition where scene depth < 1
    // I-0113 tranche 29: a tabled view's background quad into its lit HDR where the scene depth is 1 (before the forward rows), and
    // its composition over every pixel (no depth test), as the original's key 0x1A quad and key 0x39 tone map.
    Microsoft::WRL::ComPtr<ID3D12PipelineState> offscreen_background_pso_,offscreen_composition_full_pso_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> offscreen_composition_root_;   // I-0113 tranche 27: its SRV table and the tone's root constants
    // Bink movie conversion: root, pipeline, unit quad and the three R8 planes (recreated when the buffer size changes).
    Microsoft::WRL::ComPtr<ID3D12RootSignature> movie_root_;Microsoft::WRL::ComPtr<ID3D12PipelineState> movie_pso_;
    Microsoft::WRL::ComPtr<ID3D12Resource> movie_quad_;std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,3> movie_planes_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> movie_views_;std::array<uint32_t,4> movie_plane_sizes_{};
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> movie_rtv_;   // the output view, rewritten each frame
    // W5a SW-6a/SW-7c: a D16 depth atlas (resource, its view heap, edge, generation); I-0131 R10a: the paints' occlusion depths only
    // (the cascades' and the local lights' atlases are removed).
    struct DepthAtlas {Microsoft::WRL::ComPtr<ID3D12Resource> resource;Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsv;uint32_t edge{};uint64_t generation{};};
    uint32_t binding_stride_{},rtv_stride_{};
    uint64_t last_serial_{},startup_serial_{};
    std::shared_ptr<PrimitivePass> primitives_;   // cR2 (X1): the current UI build; a frame job holds the one it draws
    Microsoft::WRL::ComPtr<ID3D12Resource> primitive_depth_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> primitive_dsv_;
    bool primitive_stencil_initialized_{};
    uint64_t primitive_stencil_generation_{};   // cR2: the UI build whose stencil the depth target holds
    // cD3: the admitted build's ingress list waits for the submission of its frame. R13 cR3: atomic (the admission sets it
    // on the owner thread, a frame clears it on the worker; a game-path frame submits every committed ingress list anyway).
    std::atomic<bool> primitive_uploads_pending_{};
    uint64_t primitive_generation_{},primitive_texture_generation_{};
    bool open_{};
    std::atomic<bool> faulted_{};   // R13 cR2 (X7): set by the frame (the worker after cR3), read by the admission
    std::atomic<DWORD> render_thread_{};   // R13 cR3: SetRenderThread
    void RequireThread(bool worker,const char* what) const;
    // R13 cR2 (X7): the serials the frame writes, as the engine thread's admission reads them (published at each write).
    std::atomic<uint64_t> mirror_startup_serial_{};
    void PublishSerials() noexcept {mirror_startup_serial_.store(startup_serial_,std::memory_order_release);}
    // 1049 native deferred lighting (NativeLightingInput): genuine programmes,
    // owned targets {diffuse acc, specular acc, albedo-lit, lit HDR}, neutral
    // 1x1 {white, black}, persistent constant blocks (bounded dynamic indexing).
    void NativeLightingResources(const NativeLightingInput& input);
    // I-0131 R10a: DS3's local-light batches, all unshadowed, into the accumulation pair (the frames the native lights pass does not draw).
    void EncodeNativePointLights(ID3D12GraphicsCommandList* list,const NativeLightingInput& input,const D3D12_RECT& screen);
    // mask: the main view's shadow mask input when it is drawn this frame (D-0180): the shadow mask t32 (t33 is always the neutral white).
    void EncodeNativeLighting(ID3D12GraphicsCommandList* list,const NativeLightingInput& input,
        const D3D12_VIEWPORT& viewport,const D3D12_RECT& scissor,const NativeShadowMaskInput* mask=nullptr,const NativeModelFrameInput* sun=nullptr);
    // I-0140 tranche 31 (D-0303): a Player menu view's self-shadow mask (its context's), in place of the mask stage's: with `sun` (the
    // view traced this frame: its top level menu_rt_scene_) the traced sun's two draws into it (cleared white if they fail), its
    // views' t32 naming it (BindSelfShadowViews, before the graph); without `sun`, on a frame whose views name the mask
    // (SelfMaskBound: held, or a refused rewrite away from it; the graph declared it) the mask cleared white; else no pass. Entered as EncodeNativeShadowMask is (the depth plane
    // readable, the stencil plane writable). I-0140 tranche 35: on a traced frame whose `sun` names blur passes (SelfShadowBlurPasses),
    // the trace written into the target the parity names, then the original's blur passes alternating between the blur target and
    // the mask, the last into the mask (each its own graph pass); a pass that fails leaves the mask cleared white, as a failed trace.
    void EncodeNativeSelfShadow(ID3D12GraphicsCommandList* list,const NativeModelFrameInput* sun,const D3D12_VIEWPORT& viewport,const D3D12_RECT& scissor);
    // The blur passes a traced frame takes (0: none, or no blur target or views yet): the frame's, at most kMenuShadowBlurPassesMax.
    uint32_t SelfShadowBlurPasses(const NativeModelFrameInput& sun) const noexcept {
        return view_->self_blur && view_->self_blur_view!=BindlessIndices::none && view_->self_mask_view!=BindlessIndices::none && std::isfinite(sun.self_shadow_blur_scale) && sun.self_shadow_blur_scale>0.f?
            std::min(sun.self_shadow_blur_passes,kMenuShadowBlurPassesMax):0u;}
    void BindSelfShadowViews(bool traced);   // the view's t32 before its graph is declared (self_traced_now_ the frame's trace)
    bool self_traced_now_{};
    uint32_t self_blur_passes_now_{};   // I-0140 t35: the traced frame's blur passes (RenderNativeOffscreen, before its graph)
    // Its view's skinning, material records and top level on the frame's list (false: none this view, a view past the frame's
    // kMenuRtSlots pairs or a failure).
    bool EncodeMenuRtScene(ID3D12GraphicsCommandList* list,const NativeRtSceneInput& input);
    void MakeSelfShadowMask(BindlessHeap& heap);   // the context's mask, its blur target and the three bindless views (throws)
    void FailMenuShadow(const char* why) noexcept;
    // The step review of the fixes (M1): the context's t32 keeps naming its mask for kSelfMaskHold frames after its last trace, the
    // mask cleared white on its untraced frames, so views of one context that are traced and not in one frame, or a short gap, do
    // not rewrite its views (a fresh shader-visible heap each) every frame; past it, or once the menu self-shadow failed, t32 goes back
    // to the neutral white once (a refused rewrite keeps the mask, cleared white, on that mirror).
    static constexpr uint64_t kSelfMaskHold=120;
    // SelfMaskBound: the image last bound into the context's views is its mask; once BindSelfShadowViews ran for the frame, what its
    // published views name (the passes' t32 and the view's graph read it, the last step review's L2).
    bool SelfMaskBound() const noexcept {return view_->self_mask && view_->self_bound_mask==view_->self_mask.Get();}
    bool SelfMaskHeld() const noexcept {return SelfMaskBound() && frames_.Numbered()<view_->self_traced_frame+kSelfMaskHold;}
    void TickMenuRtMaterials(ID3D12GraphicsCommandList* list);   // the menu tables' sweep on a frame no view took them (step review M2)
    // D-0180: the mask (t32) of the mask stage, written with GXRend_CompositeShadowMask; the lighting views name it once per
    // {view heap, view generation, stage target generation} (mask_views_), the neutral white otherwise (t33 always, I-0126 R7c).
    void EncodeNativeShadowMask(ID3D12GraphicsCommandList* list,const NativeShadowMaskInput& mask,const D3D12_VIEWPORT& viewport,const D3D12_RECT& scissor,
        const NativeModelFrameInput* sun=nullptr,bool gi=false);
    void BindNativeMaskViews(bool stage,bool gi);void SyncNativeMaskViews(bool stage,bool gi);   // I-0126 R7a: gi, pass 0's GB4 the neutral black
    // RefreshLightViews copies the mirror's fixed part into a fresh shader-visible heap (the former retired by ticket) and keeps the
    // key of the mask views that named the former heap.
    void RefreshLightViews();
    Microsoft::WRL::ComPtr<ID3D12PipelineState> native_mask_pso_,native_sun_shadowed_pso_;
    D3D12_GPU_VIRTUAL_ADDRESS sun_scene_block_{};   // the shadowed sun pass's b8 (FC_ShadowColor), on the traced sun's frames
    struct MaskViews {ID3D12DescriptorHeap* heap{};uint64_t generation{UINT64_MAX},targets{UINT64_MAX};bool stage{},gi{};} mask_views_;   // R7a: stage, gi the views bound
    // I-0074 point 3c: chunks of at least 128 draws, at most 16 (ER's chunk planner splits a run into near-equal chunks).
    static constexpr size_t record_chunk_draws=128,record_chunks_max=16;
    std::atomic<bool> record_chunks_disabled_{};std::atomic<uint64_t> record_chunked_frames_{},record_chunks_{},record_chunk_blocks_{};
    std::shared_ptr<const NativeLightingPrograms> native_light_programs_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> native_light_root_;
    std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>,4> native_light_psos_;
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,2> native_light_neutral_;
    // B2c: b0 cbLightAcc, b2 cbLight, b3 cbFog from frame_uploads_ per submission; b1 stays a static block of zeros.
    Microsoft::WRL::ComPtr<ID3D12Resource> native_light_zeros_;
    std::array<D3D12_GPU_VIRTUAL_ADDRESS,4> native_light_constants_{};
    // D3D12 review R2 (B2c): ranges of the submission being recorded (lighting, point-light batches, env regions, SSS,
    // velocity), retired by the ticket of the list that reads them.
    UploadArena frame_uploads_;
    // 1054 point-light batches: PSOs by programme; per-batch constants from frame_uploads_.
    std::map<const std::vector<uint8_t>*,Microsoft::WRL::ComPtr<ID3D12PipelineState>> native_point_psos_;
    Microsoft::WRL::ComPtr<ID3D12Resource> native_light_quad_,native_light_list_;
    D3D12_VERTEX_BUFFER_VIEW native_light_quad_view_{};
    // 1137 skin SSS stage: root (b1 cbSSS, b8 cbSceneParam, t1..t3, t36), BlurX/BlurY
    // pipelines, the cleared temporary (format of the irradiance target) and constants.
    void EncodeNativeSss(ID3D12GraphicsCommandList* list,const NativeLightingInput& input);
    Microsoft::WRL::ComPtr<ID3D12RootSignature> native_sss_root_;
    std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>,2> native_sss_psos_;
    // 1147 env-map regions (pass 4; D-0301: an untraced menu view only): root (b3 region, b8 scene, t0/t1/t3/t5, t30), pipelines
    // {base, parallax-corrected} x {outside, inside}, the SpecularAcc target read by DeferredAO as t7, the unit box and per-region constants.
    void EncodeNativeEnvRegions(ID3D12GraphicsCommandList* list,const NativeLightingInput& input);
    // D3D12 phase D cD2 (sheet 4, cD2): the per-view resources (G-buffer and composition bindings, targets, depth,
    // lighting targets and views, environment and SSS targets), one context for the main view and one per menu
    // view extent (1168). The recording functions use the context `view_` points to; a menu view points it at its own
    // context for its recording (no member swap). `output_rtv`: the menu view composition view, kept with its context (P3).
    struct ViewContext {
        uint32_t width{},height{};
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> bindings,rtvs,dsv,light_views,light_rtvs,output_rtv;
        // Optimisation review axis 2: the CPU mirror of light_views' fixed part (light_fixed_views entries): every fixed view is
        // written here and copied into the shader-visible heap, so a change (the mask views) copies into a fresh heap and
        // retires the former one by ticket instead of rewriting it in place behind the frames in flight.
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> light_views_cpu;
        std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,6> colors;
        Microsoft::WRL::ComPtr<ID3D12Resource> depth,env_target,sss_temp;   // I-0120 E5: no readback (never read)
        std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,4> light_targets;
        uint64_t target_generation{},light_generation{UINT64_MAX};
        bool model_targets{};uint8_t clear_stencil{};
        uint64_t env_frame{};uint32_t env_ranges{};   // cD3 (D1): env cube ranges handed out in frame `env_frame`
        uint64_t last_frame{};   // I-0120 E1: the numbered frame a menu view last recorded into this context
        // I-0140 tranche 31 (D-0303): a Player menu view's self-shadow mask at the context's extent (made when a view recorded into
        // the context first has a traced scene and the shadowed sun; its lighting views' t32 on the frames it is traced, the neutral
        // white on the others), its RTV, the bindless view of its depth for the ray (made for `self_depth_generation`, the target
        // generation; released when the depth is made again, the context leaves or the renderer closes), and the key its t32 views
        // were written for (the mirror, its light generation, the image named: the mask or the neutral white).
        Microsoft::WRL::ComPtr<ID3D12Resource> self_mask;Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> self_mask_rtv;
        uint32_t self_depth_view{UINT32_MAX};uint64_t self_depth_generation{UINT64_MAX},self_bound_generation{UINT64_MAX};
        // I-0140 tranche 35: the blur's second target (the mask's twin: format, extent, white clear; its RTV), made with the mask, and
        // the bindless views of the mask and of it that a blur pass reads, made and released with the depth's view.
        Microsoft::WRL::ComPtr<ID3D12Resource> self_blur;Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> self_blur_rtv;
        uint32_t self_mask_view{UINT32_MAX},self_blur_view{UINT32_MAX};
        const void* self_bound_mirror{};const void* self_bound_mask{};
        bool self_bind_refused{};   // a rewrite of these views refused (BindSelfShadowViews: not tried again until they are bound anew)
        uint64_t self_traced_frame{};   // the numbered frame its mask was last traced (the step review of the fixes, M1: SelfMaskHeld)
        // I-0140 tranche 40 (D-0305): a menu view's TAA in this context: its output (the lit HDR's format at the context's extent,
        // unordered access, UNORDERED_ACCESS between recordings; viewed at the composition's binding +2), its camera motion (the
        // velocity target cleared each recording, no object rows, so the resolve's camera path on every pixel; the motion vectors;
        // both RENDER_TARGET between recordings), their RTVs and the resolve's views {t0 velocity, t1 depth}, made for
        // `taa_generation` (the target generation: a new depth rewrites the depth's view after its drain). The histories are the views'
        // own (menu_taa_, by target), not the context's: views of one extent share a context.
        Microsoft::WRL::ComPtr<ID3D12Resource> taa_output,taa_velocity,taa_motion;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> taa_rtvs,taa_views;
        uint64_t taa_generation{UINT64_MAX};
        uint64_t taa_made{};   // the TAA output's creation, renderer-wide (a menu view's DLAA viewport retires on a new one, a re-created context's too)
    };
    ViewContext main_view_;
    void ReleaseSelfShadowViews(ViewContext& context) noexcept;   // I-0140 t31, t35: its self-shadow views' slots, against the last submitted ticket
    ViewContext* view_{&main_view_};
    // The extent of the current view: the presentation for the main view, the context's own for a menu view (the former
    // swap exchanged the presenter's extent with the set's).
    uint32_t ViewWidth() const noexcept {return view_==&main_view_?RenderWidth():view_->width;}
    uint32_t ViewHeight() const noexcept {return view_==&main_view_?RenderHeight():view_->height;}
    // I-0111 tranche 3 (UPSCALER-DLSS-READY §3.1, D-0151): the extent the world renders at, DLSS's render size in a scaled mode;
    // 0 is the presentation's. The world's targets, depth, lighting, velocity, AO, sun mask and copies take it; the composition,
    // YEBIS, glow, AA, UI and display the presentation's.
    uint32_t render_width_{},render_height_{};
    uint32_t RenderWidth() const noexcept {return render_width_?render_width_:presenter_.width;}
    uint32_t RenderHeight() const noexcept {return render_height_?render_height_:presenter_.height;}
    uint64_t dlss_output_generation_{};   // each DLSS output made: the post stages keyed on it renew their views
    streamline::DlssMode dlss_mode_{streamline::DlssMode::Dlaa};
    // B6: moves the UI build into a holder kept until the queue completes what was submitted (cD3: the depth target stays).
    void RetirePrimitives();
    std::map<std::pair<uint32_t,uint32_t>,ViewContext> offscreen_sets_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> native_env_root_;
    std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>,4> native_env_psos_;
    Microsoft::WRL::ComPtr<ID3D12Resource> native_env_box_;
    D3D12_VERTEX_BUFFER_VIEW native_env_vertices_{};
    D3D12_INDEX_BUFFER_VIEW native_env_indices_{};
    // 1186 velocity (main view only): the velocity target and motion vectors (render targets between frames), their RTVs,
    // the resolve's views {t0 velocity, t1 scene depth}, root (b0 constants), pipeline, constants and witness readbacks.
    void NativeVelocityResources();
    // C4-1: each frame block of a prepared view copied once into the slot's upload arena (a null kind gives address 0).
    std::array<D3D12_GPU_VIRTUAL_ADDRESS,FrameBlockKindCount> UploadNativeFrameBlocks(const NativeModelFrameInput& input);
    // I-0126 R7c (R7-PLAN §7): the Velo rows drawn before the lighting (the G-buffer surface's object motion, which NRD's signal pass
    // reads), their resolve after the forward and effect rows (the camera motion of the final depth, for DLSS and the motion blur).
    void EncodeNativeVelocityRows(ID3D12GraphicsCommandList* list,const NativeModelFrameInput& input,const NativeModelFrameAddresses& frame,
        NativeModelConstantUploads& uploads,const D3D12_VIEWPORT& viewport,const D3D12_RECT& scissor);
    void EncodeNativeVelocity(ID3D12GraphicsCommandList* list,const NativeModelFrameInput& input,const D3D12_VIEWPORT& viewport,const D3D12_RECT& scissor,
        const SlotReadbacks* sample);
    std::string ReadNativeVelocity(const SlotReadbacks& readbacks,const std::wstring& picture_file);
    Microsoft::WRL::ComPtr<ID3D12Resource> velocity_target_,motion_vectors_;
    // I-0111: DLSS's output (the lit HDR's format at the presentation's extent, unordered access and render target; RENDER_TARGET
    // between frames), the post chain's source on a DLSS frame, viewed at the composition's binding +2.
    Microsoft::WRL::ComPtr<ID3D12Resource> dlss_output_;
    std::wstring upscaler_folder_;void (*upscaler_sink_)(const char*){};std::string upscaler_state_;
    uint32_t frame_generation_requested_{};std::atomic<bool> frame_generation_{};   // I-0131 R10b: the multiplier asked before Start; DLSS-G loaded (Start; R12f: live)
    bool frame_generation_offer_{};   // I-0132 R12f: DLSS-G at Streamline's start to be switched on later
    std::atomic<bool> fg_wanted_{};   // the player's frame generation (Start, SetFrameGenerationLive)
    // I-0131 R10b: the last frames whose depth and motion, then HUD-less colour, were tagged (the worker's); the presents with
    // DLSS-G on and all presents since Start; a refusal latches it off for the session, its row once.
    uint64_t fg_depth_serial_{},fg_hudless_serial_{};std::atomic<uint64_t> fg_on_presents_{},fg_presents_{};std::atomic<bool> fg_failed_{};
    void FailFrameGeneration(uint64_t serial,const std::string& reason);
    void FrameGenerationOff();
    bool fg_presented_on_{};uint32_t fg_min_size_{},fg_bad_status_{};   // the last Present's mode; DLSS-G's minimum size, its non-OK reads in a row
    std::atomic<uint64_t> dlss_frames_{},dlss_unresolved_{};std::atomic<bool> dlss_failed_{};uint64_t dlss_last_serial_{};
    void NativeDlssOutput();
    void DlssOutputView();
    // I-0111 tranche 2 (D-0242): DS3's exposure for DLSS, a 1x1 R32_FLOAT written each DLSS frame from the tone map's Exposure
    // (NON_PIXEL_SHADER_RESOURCE between frames); without a tone-map input, DLSS's own auto-exposure.
    Microsoft::WRL::ComPtr<ID3D12Resource> dlss_exposure_;bool dlss_exposure_failed_{};
    ID3D12Resource* RecordDlssExposure(ID3D12GraphicsCommandList* list,float exposure,bool automatic=false);   // automatic: I-0132 R11h's, when it has run
    // I-0111 tranche 3 (§3.7): the depth at the presentation's extent in a scaled mode (DEPTH_WRITE between frames, made at the first
    // scaled frame), its pipeline and the scene depth's view it reads.
    Microsoft::WRL::ComPtr<ID3D12Resource> depth_out_;Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> depth_out_dsv_,depth_out_views_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> depth_out_root_;Microsoft::WRL::ComPtr<ID3D12PipelineState> depth_out_pso_;uint64_t depth_out_view_generation_{UINT64_MAX};
    void NativeDepthOut();
    void EncodeDepthOut(ID3D12GraphicsCommandList* list,float jitter_x,float jitter_y);
    // DLSS off for the session (its renderer flag and Streamline's, so the game thread stops the jitter), its row with the reason.
    void LatchDlssOff(uint64_t serial,const std::string& reason);
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> velocity_rtvs_,velocity_views_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> velocity_root_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> velocity_resolve_pso_;
    uint64_t velocity_generation_{UINT64_MAX};
    // I-0136: the Velo rows' own depth (the scene depth's format and extent, DEPTH_WRITE between frames; its DSV, its SRV the
    // velocity views' slot 2) and the occlusion pass (the velocity target's pixels a nearer scene surface hides cleared back).
    Microsoft::WRL::ComPtr<ID3D12Resource> velocity_depth_;Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> velocity_dsv_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> velocity_occlusion_root_;Microsoft::WRL::ComPtr<ID3D12PipelineState> velocity_occlusion_pso_;
    // W10 MB-4: the engine's half-size R8G8B8A8 object velocity (the original's pipeline+0xDF0 group the motion blur reads), reduced from
    // the velocity target (RTV slot 2 of velocity_rtvs_, shader-readable between frames), and its reduction pipeline.
    Microsoft::WRL::ComPtr<ID3D12Resource> velocity_half_;Microsoft::WRL::ComPtr<ID3D12RootSignature> velocity_half_root_;Microsoft::WRL::ComPtr<ID3D12PipelineState> velocity_half_pso_;
    void EncodeVelocityHalf(ID3D12GraphicsCommandList* list);
    Microsoft::WRL::ComPtr<ID3D12Resource> scene_colour_copy_;   // C4
    DecalSourceSet decal_sources_;   // DECALS-WAVE1 D1
    // I-0107: the light grid (LightGridSources, owner thread), this frame's (the job's, set before RenderModels on the
    // recording thread), the set whose buffers were put in their states, and the organize stage's root, pipeline and views.
    NativeFrameJob::LightGridSet light_grid_,light_grid_frame_;uint64_t light_grid_serial_{},light_grid_ready_{};   // ready: the set put in its states
    Microsoft::WRL::ComPtr<ID3D12RootSignature> organize_root_;Microsoft::WRL::ComPtr<ID3D12PipelineState> organize_pso_;
    const std::vector<uint8_t>* organize_programme_{};Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> organize_views_;
    ID3D12Resource* organize_views_depth_{};uint64_t organize_views_grid_{},organize_views_generation_{UINT64_MAX};
    std::atomic<bool> organize_failed_{};std::string organize_failure_;   // the grid, root or pipeline refused: no grid for the session
    // I-0117 (R2 T1): the frame's top level from NativeModelFrameInput::rt_scene, built at the start of the main view's list; a
    // device failure turns it off for the session (rt_failed_), the raster untouched.
    RtTlas rt_tlas_;std::string rt_failure_;std::atomic<bool> rt_failed_{};std::atomic<uint64_t> rt_tlas_bytes_{};D3D12_GPU_VIRTUAL_ADDRESS rt_scene_address_{};
    // T2: the skinned instances' positions and BLAS (RtSkinner), its counters for the rows (the recording thread writes them).
    RtSkinner rt_skinner_;std::vector<RtInstance> rt_skinned_instances_;
    std::atomic<uint64_t> rt_skin_instances_{},rt_skin_builds_{},rt_skin_refits_{},rt_skin_vertices_{},rt_skin_failed_{},rt_skin_bytes_{},rt_skin_winds_{},rt_skin_wind_vertices_{};
    // I-0119 R3a (T3's alpha records before it): the material table (RtMaterialTable) and the views it names in rt_heap_; the
    // frame's table and rows for the traced passes (0: none this frame); the counters for the rows.
    std::atomic<BindlessHeap*> rt_heap_{};RtMaterialTable rt_materials_;D3D12_GPU_VIRTUAL_ADDRESS rt_material_table_{},rt_material_rows_{};
    mutable std::mutex rt_material_lock_;RtMaterialTable::Counters rt_material_totals_{};
    void StoreRtMaterialTotals() noexcept;
    void EncodeRtScene(ID3D12GraphicsCommandList* list,const NativeRtSceneInput& input);
    void TickRtMaterials(ID3D12GraphicsCommandList* list);
    // T4: the control view (rt-view.flag): its pass, its pictures (traced, classes; I-0119 R3b: the material differences) and the
    // counts' buffer at the render extent, their bindless slots (the depth's SRV, the pictures' UAVs, R3b: GB0 to GB3's SRVs) for one
    // target generation; the samples' totals for the row.
    RtView rt_view_;std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,3> rt_view_images_;Microsoft::WRL::ComPtr<ID3D12Resource> rt_view_counts_;
    // The counts' readbacks, one a sample in turn (a sample a frame at most: one is written again only after max_frame_slots later
    // frames, its slot's completion read by then), made once (a committed buffer a sample made a hitch candidate, Thomas 03:39).
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,max_frame_slots+1> rt_view_readbacks_;uint32_t rt_view_readback_{};
    // The pictures' UAVs (traced, classes, materials); the depth and G-buffer views are rt_main_views_.
    std::array<uint32_t,3> rt_view_slots_{BindlessIndices::none,BindlessIndices::none,BindlessIndices::none};uint64_t rt_view_generation_{UINT64_MAX};
    mutable std::mutex rt_view_lock_;RtViewTotals rt_view_totals_;std::string rt_view_failure_;   // the slots' completions, a failure
    void FailRtView(const char* why) noexcept;   // the tool off for the session, its reason kept
    std::wstring rt_view_file_;   // this frame's pictures (the job's), empty on most frames
    bool RtViewFrame(const NativeModelFrameInput& input);   // whether this frame runs the control view (in the plan key), its resources made
    // I-0122 R4b (R4a final review L3): the main view's views the rays' passes read through the bindless heap (the control view, the
    // traced sun, the local lights), one set for a target generation: the depth (R32 of the D32S8), GB0, GB1, GB2 and GB3 in the
    // lighting's formats (GB2's sRGB view gives the albedo linear); I-0125 R6a: GB4 (the ambient the control view compares). Made by MakeRtMainViews (throws on a slot it cannot take; the
    // former set released against the last submitted ticket when the targets change), released at the renderer's close.
    enum RtMainView : uint32_t {rt_depth_view,rt_gb0_view,rt_gb1_view,rt_gb2_view,rt_gb3_view,rt_gb4_view};
    std::array<uint32_t,6> rt_main_views_{BindlessIndices::none,BindlessIndices::none,BindlessIndices::none,BindlessIndices::none,BindlessIndices::none,BindlessIndices::none};
    uint64_t rt_main_views_generation_{UINT64_MAX};
    void MakeRtMainViews(BindlessHeap& heap);
    void CloseRtMainViews() noexcept;
    // R5: the traced sun shadow: its pass (its depth view rt_main_views_'), this frame's choice (in the plan key: bit 44; the blur
    // passes' bits then 0), its failure (under rt_view_lock_).
    RtSunShadow rt_sun_;bool rt_sun_now_{};std::string rt_sun_failure_;
    // I-0140 tranche 31 (D-0303): the Player menu views' traced self-shadow: their own material tables and top levels, one pair a
    // traced view of the frame (a table writes its rows once a list; the creation and its Face Detail record four Player views a
    // frame: up to kMenuRtSlots, `menu_rt_used_` of them taken in the numbered frame `menu_rt_frame_`), this view's scene, table and rows,
    // its failure (menu_shadow_failure_ written once, before the flag) and counts (the bootstrap's native_menu_self_shadow_renderer
    // rows read both). The skinner is the frame's (rt_skinner_). `menu_rt_idle_`: per pair, the main or startup frames recorded since a
    // view last took it, its table swept on the first kMenuRtIdleTicks of them (TickMenuRtMaterials: its records and views released by
    // ticket); a pair never taken starts past them (the last step review's L7).
    static constexpr uint32_t kMenuRtSlots=4;
    std::array<RtMaterialTable,kMenuRtSlots> menu_rt_materials_;std::array<RtTlas,kMenuRtSlots> menu_rt_tlas_;
    uint64_t menu_rt_frame_{UINT64_MAX},menu_rt_ticked_{UINT64_MAX};uint32_t menu_rt_used_{};std::vector<RtInstance> menu_rt_skinned_;
    static constexpr uint32_t kMenuRtIdleTicks=2*RtMaterialTable::kRetireFrames+RtMaterialTable::kSweepFrames;   // past a set's retirement and a sweep
    D3D12_GPU_VIRTUAL_ADDRESS menu_rt_scene_{},menu_rt_table_{},menu_rt_rows_{};
    std::array<uint32_t,kMenuRtSlots> menu_rt_idle_{kMenuRtIdleTicks,kMenuRtIdleTicks,kMenuRtIdleTicks,kMenuRtIdleTicks};
    std::atomic<bool> menu_shadow_failed_{},menu_shadow_ready_{};std::string menu_shadow_failure_;
    std::atomic<uint64_t> menu_shadow_traced_{},menu_shadow_past_slots_{},menu_shadow_refused_{};
    std::atomic<uint64_t> menu_shadow_held_{},menu_shadow_neutral_{};   // I-0140 tranche 34: held recordings, renames to the neutral (events)
    std::atomic<uint64_t> menu_shadow_blurred_{};   // I-0140 tranche 35: traced recordings whose blur passes all ran
    bool RtSunFrame(const NativeModelFrameInput& input,bool possible);
    // I-0122 R4a: the native local lights: its pass (its depth and G-buffer views rt_main_views_'), this frame's choice and camera, the frames with a list and no pass, its failure (under rt_view_lock_); the check's four
    // pictures (the batches' pair, the native pass's pair) and their RTVs, this frame's names.
    RtLocalLights rt_lights_pass_;
    bool rt_lights_now_{};RtLightsConstants rt_lights_camera_{};std::atomic<uint64_t> rt_lights_missed_{};std::string rt_lights_failure_;
    std::atomic<uint64_t> rt_pt_frames_{};   // I-0122 R4c
    // I-0125 R6b: the reflections' pass, its frames, the regions (I-0132 R11e: one zeroed row keeping the root's t4 bound, count 0; the cube
    // slots removed), and the list's upload shared with the local lights' pass (UploadRtLights).
    RtReflections rt_reflect_pass_;std::atomic<uint64_t> rt_reflect_frames_{},rt_reflect_missed_{};
    std::string rt_reflect_failure_;   // under rt_view_lock_
    std::atomic<uint64_t> rt_fail_magenta_frames_{};std::atomic<uint32_t> rt_fail_magenta_causes_{};   // I-0132 R11d
    // I-0132 R11e: the held frames, the judged word (RtJudged), and this frame's state (the render thread's): held by its arrival,
    // its world to show black (held, or unserved without rt_fail_magenta).
    std::atomic<uint64_t> rt_held_frames_{},rt_judged_{};bool rt_hold_now_{},rt_world_black_{};
    std::atomic<uint32_t> rt_unserved_run_{},rt_unserved_run_causes_{};   // R11e: unserved frames in a row outside a hold, the last's causes
    std::atomic<uint64_t> rt_unserved_since_{},rt_unserved_last_{};   // R11e: the ticks (GetTickCount64) of the run's first and last frames, 0 without a run
    std::atomic<uint64_t> rt_held_unjudged_{};bool rt_judged_now_{};   // R11e (the final review's M1): held frames without a judgement; this frame judged
    uint32_t rt_reflect_anisotropy_{1};float rt_reflect_bias_{},rt_reflect_time_{};   // this frame's (NativeRtSceneInput)
    std::array<float,4> rt_reflect_sun_{};uint64_t rt_reflect_frame_{UINT64_MAX};   // the frame's sun (rt_sun); the last frame the regions were uploaded
    D3D12_GPU_VIRTUAL_ADDRESS rt_reflect_regions_{};   // R6c: that frame's regions' buffer (UploadRtRegions), shared by the water's pass
    bool EncodeRtReflections(ID3D12GraphicsCommandList* list,const NativeLightingInput& input);
    // I-0132 R11 (D-0252): on an NRD frame, in EncodeRtReflections' place: NRD's signal pass (the GI's and the specular's), its
    // dispatches and the specular's compose (true when SpecularAcc was written); rt_nrd_recorded_ whether NRD served this frame (the
    // GI's compose, EncodeRtGi, reads it).
    bool EncodeRtNrd(ID3D12GraphicsCommandList* list,const NativeLightingInput& input);
    bool rt_nrd_recorded_{};
    // I-0126 R7a: this frame's GI (RtGiFrame, in the plan key), its pass after GXLightCmbAlbedo (EncodeRtGi), its counts and failure.
    bool rt_gi_now_{};std::atomic<uint64_t> rt_gi_frames_{},rt_gi_missed_{};std::string rt_gi_failure_;   // the failure under rt_view_lock_
    // I-0126 R7d: the frame's serial, the seed of the reflections', the water's, the cache's and the GI's random numbers (FillRtReflectFrame);
    // until R7d it was rr_serial_, which only RR's frames advance: on NRD's frames every pixel drew the same direction each frame.
    uint64_t rt_seed_serial_{};
    // I-0126 R7d (R7-PLAN §8): the bias check's accumulation (R32G32B32A32 at the render extent, in UNORDERED_ACCESS, its UAV slot),
    // this frame's choice and restart, the camera's rows and the frame it last served (a change, a gap or a new extent restarts it).
    Microsoft::WRL::ComPtr<ID3D12Resource> rt_gi_mean_target_;uint32_t rt_gi_mean_slot_{BindlessIndices::none};
    bool rt_gi_mean_now_{},rt_gi_mean_restart_{};std::array<float,16> rt_gi_mean_camera_{};uint64_t rt_gi_mean_frame_{UINT64_MAX};uint32_t rt_gi_mean_estimator_{};
    bool RtGiMeanFrame(FrameSlot& slot);
    GpuZoneTimer<max_frame_slots> rt_gpu_;bool rt_gpu_now_{};uint32_t rt_gpu_slot_{};   // I-0128 R8a: the frame being recorded is timed, its slot
    // The main view's only (the final review's L6; a safety net: every frame's start sets rt_gpu_now_ again, and view_ returns to the main view).
    void GpuZoneBegin(ID3D12GraphicsCommandList* list,GpuZone zone){if(rt_gpu_now_ && view_==&main_view_)rt_gpu_.Begin(list,rt_gpu_slot_,zone);}
    void GpuZoneEnd(ID3D12GraphicsCommandList* list,GpuZone zone){if(rt_gpu_now_ && view_==&main_view_)rt_gpu_.End(list,rt_gpu_slot_,zone);}
    void GpuZoneCancel(GpuZone zone){if(rt_gpu_now_ && view_==&main_view_)rt_gpu_.Cancel(rt_gpu_slot_,zone);}
    // I-0126 R7d: this frame's ReSTIR GI (RtRestirFrame: a PT GI frame RR denoises; in the plan key: bit 54), its history bit, the last
    // ReSTIR frame's serial and eye, this frame's current-to-previous rows (the velocity's, camera only), its frames and failure.
    bool rt_restir_now_{},rt_restir_history_{};uint64_t rt_restir_serial_{UINT64_MAX};std::array<float,4> rt_restir_eye_{},rt_restir_eye_previous_{};
    Matrix4 rt_restir_current_to_previous_{};std::atomic<uint64_t> rt_restir_frames_{};std::string rt_restir_failure_;
    uint64_t rt_restir_used_{UINT64_MAX};static constexpr uint64_t kRtRestirIdleFrames=600;   // the last ReSTIR frame's serial (its buffers' release)
    bool RtRestirFrame(const NativeModelFrameInput& input,FrameSlot& slot);
    // I-0132 R11b: this frame's SER (RtSerFrame: a main-view frame RR denoises at the PT level whose paths go past their first hit).
    bool rt_ser_now_{};std::string rt_ser_failure_;
    std::atomic<int64_t> rt_ser_make_us_{-1};   // I-0132 R11b: the SER setup's first making (state object, apply pipeline, targets; native_rt_ser's make_ms), -1 before
public:
    int64_t RtSerMakeMicroseconds() const noexcept {return rt_ser_make_us_.load(std::memory_order_relaxed);}
private:
    // The step reviews' H1: on a frame with SER the lighting phase holds the depth's plane 0 and GB0 to GB3 in kSerReadState (ray
    // generation reads them), set at its start (EncodeNativeLighting) for its passes' declarations and barriers.
    D3D12_RESOURCE_STATES lit_gb_read_{D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE},lit_depth_read_{D3D12_RESOURCE_STATE_DEPTH_READ|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE};
    D3D12_RESOURCE_STATES GbReadState(ID3D12Resource* resource) const;void SerFailed();
    static constexpr D3D12_RESOURCE_STATES kSerReadState=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    bool RtSerFrame(FrameSlot& slot);
    void CloseRtGiMean() noexcept;
    bool RtGiFrame(const NativeModelFrameInput& input) const;
    bool RtGiPossible(const NativeModelFrameInput& input) const;
    void EncodeRtGi(ID3D12GraphicsCommandList* list,const NativeLightingInput& input,GraphShadow* graph);
    // I-0126 R7c (R7-PLAN §7): the GI on a main-view frame RR does not denoise, through NRD (rt_denoise_, at the render extent; I-0132
    // R11, D-0252: the specular with it, REBLUR_DIFFUSE_SPECULAR): this frame's choice (RtNrdFrame, before RtGiFrame; in the plan key:
    // bit 52), NRD's camera, the velocity's scale and current-to-previous rows for the signal pass, the velocity target's and the
    // denoised radiances' bindless slots (made for their generations), the last
    // serial it served (a gap restarts NRD's history), its frames; a failure turns it off for the session (its reason under rt_view_lock_).
    RtDenoiser rt_denoise_;bool rt_nrd_now_{};RtDenoiseCamera rt_nrd_camera_{};
    std::array<float,4> rt_nrd_velocity_scale_{};Matrix4 rt_nrd_current_to_previous_{};
    uint32_t rt_velocity_slot_{BindlessIndices::none},rt_denoised_slot_{BindlessIndices::none},rt_denoised_spec_slot_{BindlessIndices::none};   // I-0132 R11: the specular's
    uint64_t rt_velocity_slot_generation_{UINT64_MAX},rt_denoised_slot_generation_{UINT64_MAX},rt_denoised_spec_slot_generation_{UINT64_MAX},rt_nrd_serial_{UINT64_MAX},rt_nrd_serial_now_{};
    std::atomic<uint64_t> rt_nrd_frames_{},rt_nrd_missed_{};std::string rt_nrd_failure_;
    bool RtNrdFrame(const NativeModelFrameInput& input,const SceneView& view,FrameSlot& slot);
    void FailRtNrd(const char* why) noexcept;
    void CloseRtDenoise() noexcept;
    // I-0126 R7b: this frame's cache resolved (FillRtReflectFrame's kRtReflectCache), its frames and failure (under rt_view_lock_).
    bool rt_cache_now_{};std::atomic<uint64_t> rt_cache_frames_{};std::string rt_cache_failure_;
    void EncodeRtCache(ID3D12GraphicsCommandList* list,const NativeLightingInput& input,GraphShadow* graph);
    // I-0129 R9a: the probes' pass (after the cache) and this frame's readback, its requests and bits, given to the slot's completions
    // at the frame's end (scene_pass_frame.cpp).
    void EncodeRtProbes(ID3D12GraphicsCommandList* list,const NativeLightingInput& input);
    struct RtProbePending {Microsoft::WRL::ComPtr<ID3D12Resource> readback;std::shared_ptr<const RtProbeRequests> requests;uint32_t count{},bits{};};
    RtProbePending rt_probe_pending_;std::atomic<uint64_t> rt_probe_frames_{};std::string rt_probe_failure_;
    // R6c: the frame's regions and their cubes' slots, uploaded once a frame (the specular ambient's pass and the water's read them).
    D3D12_GPU_VIRTUAL_ADDRESS UploadRtRegions(const NativeLightingInput& input,BindlessHeap& heap);
    // The constants both passes share (the caller's camera rows apart): the counts but the lights', the mode, the cone, the samplers', the
    // sun (I-0132 R11d, D-0262: the roughness cut, the dominant rows and FC_ShadowColor are no longer read by the rays).
    void FillRtReflectFrame(RtReflectConstants& c,const NativeLightingInput& input) const;
    RtReflectConstants RtMainReflectConstants(const NativeLightingInput& input) const;
    // I-0125 R6c: the water's pass this frame (in the plan key: bit 48), its camera, plane and target (held by the frame's slot), the
    // target's RTV (made each frame), the frames it ran and the last target it was recorded into.
    bool rr_hit_written_{};   // the R6b pass wrote RR's specular hit distance this frame (the R6c step review's M1)
    bool rt_water_now_{};RtViewConstants rt_water_camera_{};std::array<float,4> rt_water_plane_{};Microsoft::WRL::ComPtr<ID3D12Resource> rt_water_target_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rt_water_rtvs_;
    std::atomic<ID3D12Resource*> rt_water_drawn_{};std::atomic<uint64_t> rt_water_frames_{};std::string rt_water_failure_;   // the failure under rt_view_lock_
    static constexpr D3D12_RESOURCE_STATES kRtWaterState=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    bool RtWaterFrame(const NativeModelFrameInput& input,FrameSlot& slot,bool lit);
    void EncodeRtWater(ID3D12GraphicsCommandList* list,const NativeLightingInput& input,GraphShadow* graph,const D3D12_VIEWPORT& viewport,const D3D12_RECT& scissor);
    void UploadRtLights(const RtLightList& list);
    bool pixel_only_{};   // I-0128 R8d: this frame's presented picture alone (the job's pixel_only, world-burst.flag)
    bool RtLightsFrame(const NativeModelFrameInput& input,bool lit);
    // I-0123 RR-a: Ray Reconstruction's guide pass, its targets (the diffuse and specular albedo, the normal with the roughness, then
    // the colour before the transparent rows in the lit HDR's format; all at the render extent, readable by pixel and non-pixel
    // shaders between frames) and the guides' RTVs; this frame's choice (in the plan key: bit 47) and camera; the RR frames evaluated.
    // I-0125 R6c: the fifth, the specular hit distance (kRrHitDistanceFormat), written by the reflections' pass (its RTV the fourth).
    RrGuides rr_guides_;std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,5> rr_targets_;Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rr_rtvs_;
    bool rt_pt_now_{};uint32_t rt_bounces_now_{1},rt_gi_paths_now_{1};   // I-0132 R12f: the frame's level and bounces (the worker's, taken at the main frame's start); R12g its GI paths
    bool rr_now_{},dlss_last_rr_{};uint64_t rr_serial_{};RrGuidesConstants rr_camera_{};std::atomic<uint64_t> rr_frames_{};   // dlss_last_rr_: the last evaluated frame's feature
    static constexpr D3D12_RESOURCE_STATES kRrGuideState=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    bool RrFrame(const NativeModelFrameInput& input,FrameSlot& slot);
    // Inside the lighting, after the composite (GB0 to GB3 and the depth readable, the lit HDR shader-readable): the guides, then the
    // lit HDR copied as the colour before transparency.
    void EncodeRrGuides(ID3D12GraphicsCommandList* list,GraphShadow* graph);
    void LatchRrOff(uint64_t serial,const std::string& reason);
    void CloseRr() noexcept;
    void FailRtLights(const char* why) noexcept;
    void CloseRtLights() noexcept;
    // The pass into `targets` (the accumulation pair when null) (t33 the neutral white); the lighting's heap, root and bindings set again after it.
    bool EncodeRtLights(ID3D12GraphicsCommandList* list,const NativeLightingInput& input);
    // The list's and tiles' upload of the current recording (the check frame's second pass reads it again).
    struct RtLightsUpload {const RtLightList* list{};D3D12_GPU_VIRTUAL_ADDRESS lights{},tiles{};bool grid{};} rt_lights_uploaded_;   // reset by each EncodeNativeLighting; R8c: its grid after the records
    void FailRtSun(const char* why) noexcept;
    void CloseRtSun() noexcept;
    void EncodeRtView(ID3D12GraphicsCommandList* list,FrameSlot& slot,const NativeModelFrameInput& input,GraphShadow* graph);
    void CloseRtView() noexcept;
    void OpenNativeLightOrganize(const NativeLightingInput& input);   // the root and pipeline, before the plan key
    // Returns whether the grid was built this frame (the forward rows then read it).
    bool EncodeNativeLightOrganize(ID3D12GraphicsCommandList* list,const NativeLightingInput& input);
    bool NativeLightOrganizeReady(const NativeLightingInput& input) const;   // the frame plan's key holds it
    // DECALS-PAINT-READY rung 2: the paint pass's depth (D32, one per target edge, cleared to 1.0 for each paint; the original's
    // pooled D32S8 +0x38), render thread only.
    std::map<uint32_t,DepthAtlas> paint_depths_;
    Microsoft::WRL::ComPtr<ID3D12Resource> water_depth_copy_;   // WT-2
    Microsoft::WRL::ComPtr<ID3D12Resource> effect_depth_copy_;   // I-0113 tranche 15: the depth the effects sample (render thread)
    // I-0113 tranche 17 (STUDY-PASS-31): pass 0x31's half-size layer (render thread): its colour target (R16G16B16A16, resting in
    // RENDER_TARGET) and depth (D32S8, resting in DEPTH_WRITE), their views (RTV; DSV writable then read-only; a shader-visible
    // table written by the frame: the depth copy for the downsample; the colour, the main depth and the half depth for the
    // composite), the downsample and the plain and bilateral composite pipelines (STUDY-T17-LAYER-ALPHA §5b).
    struct EffectLayer {
        Microsoft::WRL::ComPtr<ID3D12Resource> color,depth;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtv,dsv,table;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> root;Microsoft::WRL::ComPtr<ID3D12PipelineState> downsample,composite,bilateral;
        ID3D12Resource* viewed_copy{};ID3D12Resource* viewed_depth{};uint32_t width{},height{};uint64_t generation{},viewed_generation{};
    } effect_layer_;
    bool EffectLayerReady();   // made at the render extent's half (a size change remakes it); false on a failure (kept until Close, reported as the effects' last failure)
    std::string effect_layer_failure_;
    std::mutex water_depth_lock_;   // I-0105 step review: WaterDepthCopy is called by the parallel admission's workers
    void EncodeNativePaint(ID3D12GraphicsCommandList* list,const NativeModelFrameInput& input,NativeModelConstantUploads& uploads,const NativeModelFrameAddresses& frame);
    void EncodeNativeDecals(ID3D12GraphicsCommandList* list,const NativeModelFrameInput& input,NativeModelConstantUploads& uploads,
        const D3D12_VIEWPORT& viewport,const D3D12_RECT& scissor);
    void NoteTargetBytes();std::vector<uint64_t> target_bytes_key_;   // phase E E0: the generations last measured
    bool graph_shadow_enabled_{};GraphShadow graph_shadow_;   // E2: recording thread
    std::optional<DisplayBlobs> display_blobs_;DisplayStage display_;std::atomic<bool> display_failed_{},display_open_{};std::string display_failure_;   // W3: recording thread (the flags read by the owner)
    bool DisplayReady();   // W3: opens the stage at the presentation extent on first use; false: straight to the back buffer
    // I-0138 tranche 5: the HDR10 output. The request (before Start), the screen's answer and the peak (Start and Resize, the
    // worker quiesced), the stage (recording thread, opened by the first frame of an HDR10 chain).
    hdr::Request hdr_request_;hdr::Display hdr_display_;float hdr_peak_{};OutputStage output_;
    // hdr_display_, presenter_.hdr_wanted and hdr_peak_ from the request and the window's screen. The step review's M1: the chain is
    // asked 10-bit only once the display stage (at the presentation's extent given) and the output stage are open, since no other
    // pass can write a 10-bit back buffer; hdr_refusal_ names why an asked HDR10 chain is not asked of the presenter (L3).
    // A probe that does not find the window's screen (a window hidden or going away) keeps the last answer; a screen whose
    // answer changed lets a chain that had refused the colour space be asked again (the third step review's L2).
    // hdr_probe_fresh_: SetHdrOutputLive's probe serves the Resize it calls (one probe a switch: L3).
    void ProbeHdrOutput(HWND window,uint32_t width,uint32_t height);std::string hdr_refusal_;bool hdr_probe_fresh_{};
    Microsoft::WRL::ComPtr<IDXGIFactory1> hdr_factory_;HMONITOR hdr_monitor_{};   // the last probe's (HdrOutputStale)
    // True on a chain whose buffers are R10G10B10A2 (the stage opened on first use, its float picture at the presentation's
    // extent); such a chain without the display stage, the output stage or that picture has no pass that can write its back
    // buffer: it throws (the frame's refusal).
    bool HdrOutputReady(bool display);
    // The frame's 8-bit picture (the display stage's intermediate, PIXEL_SHADER_RESOURCE) into the float picture.
    void RecordHdrCopy(ID3D12GraphicsCommandList* list,const D3D12_VIEWPORT& viewport,const D3D12_RECT& scissor);
    // The tone map's HDR pipelines for this frame; a failure is kept for the session. RecordHdrScene: the float picture through
    // the output transform into `target`.
    bool HdrToneReady();
    void RecordHdrScene(ID3D12GraphicsCommandList* list,uint64_t serial,D3D12_CPU_DESCRIPTOR_HANDLE target,const D3D12_VIEWPORT& viewport,const D3D12_RECT& scissor);
    std::atomic<bool> hdr_scene_failed_{};std::string hdr_scene_failure_;std::atomic<uint64_t> ui_format_skips_{};
    std::atomic<bool> tone_look_failed_{};std::string tone_look_failure_;   // I-0138 tranche 6 (the failure written before the flag)
    // I-0132 R12a: the overlay, opened on the recording thread at the first frame that has one; a failure turns it off for the
    // session (its reason kept). RecordOverlay draws it into `target` (RENDER_TARGET, the presentation's extent).
    // R12e: the screen layer's own latch, screen_layer_failed_ (the final review's L3).
    OverlayPass overlay_,screen_layer_,tint_layer_;bool overlay_failed_{},screen_layer_failed_{};std::string overlay_failure_;mutable std::mutex overlay_failure_lock_;
    std::shared_ptr<const overlay::Frame> OverlaySnapshot();   // the frame's overlay (null: none), taken once (the step review's H1)
    // `pass`: the overlay, or the screen layer (R12e, its screen blend), each opened at its first frame.
    void RecordOverlay(ID3D12GraphicsCommandList* list,D3D12_CPU_DESCRIPTOR_HANDLE target,FrameSlot& slot,const std::shared_ptr<const overlay::Frame>& frame,OverlayPass& pass,bool float_target);   // float_target: an HDR10 chain's float picture
    // W6 GL-7: the glow stage on the recording thread, opened from the first frame's pinned programmes at the presentation extent
    // (Resize follows it); a failure is kept for the session and the frames have no glow.
    GlowStage glow_;std::atomic<bool> glow_failed_{};std::string glow_failure_;
    // I-0132 R12f: NIS's sharpening (Streamline's sl.nis) on the tone-mapped 8-bit colour: its copy (NON_PIXEL_SHADER_RESOURCE between
    // frames) and its output (UNORDERED_ACCESS between frames), at the presentation's extent, made at its first frame; NisTargets says
    // whether a frame can run it.
    Microsoft::WRL::ComPtr<ID3D12Resource> nis_in_,nis_out_;std::atomic<uint64_t> nis_frames_{};bool NisTargets();
    // I-0132 R11j: the TAA (opened at its first frame; a failure latches it off for the session, its row once), the serial it last
    // resolved (a gap resets its history).
    TaaStage taa_;std::atomic<bool> taa_failed_{};uint64_t taa_last_serial_{};bool TaaReady(uint64_t serial);
    // I-0140 tranche 40 (D-0305): each menu view's TAA, by its target (views of one extent share a context but not a history): its
    // stage (its history pair, opened at its first frame), the numbered frame it last resolved (a gap, the view's first frame or a
    // view at a reused address, resets) and the selection version it last resolved (a repeated version: the same picture, resolved
    // with no motion, its history kept). One left unused 600 frames retires (its viewport parked, menu_dlss_parked_). A failure latches every menu view's TAA off for the session,
    // its row once (native_menu_taa_latched_off); the counts for the periodic row native_menu_taa_frames.
    // Its DLAA (the option's DLSS mode): the view's Streamline viewport (2 and up, 0 none yet), the inputs it was evaluated on (other
    // resources take a new viewport: Streamline keeps views by address), the viewport it last evaluated (a new one resets) and whether
    // the last frame was DLAA's (a switch resets the one taken); a viewport adopted (MenuDlssViewport) or taken away resets; a viewport left for other inputs retires for 600 frames before its resources are freed (no list in flight then) and its number is reused.
    struct MenuTaa {std::unique_ptr<TaaStage> stage;uint64_t last_frame{},last_version{UINT64_MAX};uint32_t dlss_viewport{},dlss_evaluated{};std::array<ID3D12Resource*,4> dlss_inputs{};std::array<uint64_t,2> dlss_made{};bool dlss_last{},last_own{};};   // last_own: I-0140 tranche 41, the last TAA resolve its own samples
    std::unordered_map<ID3D12Resource*,MenuTaa> menu_taa_;std::atomic<bool> menu_taa_failed_{},menu_dlss_failed_{},menu_dlss_sharpen_failed_{},menu_dlss_full_told_{};
    std::vector<std::pair<uint32_t,uint64_t>> menu_dlss_retired_;   // (viewport, the numbered frame it retired)
    std::atomic<uint64_t> menu_taa_frames_{},menu_taa_resets_{},menu_dlss_frames_{},menu_aa_repeats_{};
    // A view's DLAA viewport kept when its entry retires, for the next entry on the same inputs (a menu's next opening: a new target, the
    // context's resources; Streamline's feature not made again, the final review's M1); the four last kept, each at most 36000 frames
    // and while its context lives.
    struct ParkedDlss {uint32_t viewport{};std::array<ID3D12Resource*,4> inputs{};std::array<uint64_t,2> made{};uint64_t since{};};
    std::vector<ParkedDlss> menu_dlss_parked_;
    TaaStage* MenuTaaFor(ID3D12Resource* target,uint64_t serial);   // the view's stage, open; null (latched) on a failure
    void SweepMenuAa(ID3D12Resource* target);   // every menu view's, main and no-world frame, the list open (target null but a view's)
    uint64_t menu_aa_creations_{};   // renderer-wide: a context's TAA output's creation number (ViewContext::taa_made)
    // An entry without one takes a parked viewport on its inputs, else one an entry idle since the last frame holds on them (taken from it),
    // else a free number, else the oldest parked one's; 0: none (the TAA then; its row once). `made`: the context's target generation and its TAA output's renderer-wide creation number
    // (ViewContext::taa_made).
    uint32_t MenuDlssViewport(MenuTaa& entry,const std::array<ID3D12Resource*,4>& inputs,const std::array<uint64_t,2>& made,uint64_t serial);
    void LatchMenuDlssOff(uint64_t serial,const std::string& reason);
    void MenuTaaResources();   // the current context's TAA targets and views (throws: the caller latches)
    void LatchMenuTaaOff(uint64_t serial,const std::string& reason);
    void VelocityPipelines();   // the resolve's root and pipeline (and the main view's heaps), shared by the menu views' TAA
    // I-0113 tranche 39 (STUDY-REACTIVE-MASK): the TAA's reactive base, the lit HDR copied once the 0x1F, 0x21 and 0x22 rows (and the
    // water) are drawn, before the 0x29 rows and the effects, on a TAA frame; at the render extent in the HDR's format,
    // PIXEL_SHADER_RESOURCE|NON_PIXEL_SHADER_RESOURCE between frames; remade at a new extent. A refused creation leaves the TAA without
    // it for the session (native_taa_reactive_off).
    Microsoft::WRL::ComPtr<ID3D12Resource> taa_base_;bool taa_base_failed_{};
    void NativeTaaBase();
    void CopyTaaBase(ID3D12GraphicsCommandList* list);   // from the lit HDR in RENDER_TARGET, left so
    // I-0140 (gap 23): the TAA's fade mask (model_draw.h kFadeMaskFormat) on a TAA frame with the reactive base and rows whose model's
    // fade moved (fade_mask_draws): at the render extent, cleared to 1, 0 where such a row draws (its fade-mask pipeline, after the forward and effect
    // rows); PIXEL_SHADER_RESOURCE|NON_PIXEL_SHADER_RESOURCE between frames; remade at a new extent; a refused creation latches it off
    // for the session (native_taa_fade_mask_off).
    Microsoft::WRL::ComPtr<ID3D12Resource> taa_fade_mask_;Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> taa_fade_mask_rtv_;bool taa_fade_mask_failed_{};
    std::atomic<uint64_t> taa_fade_frames_{},taa_fade_rows_{};   // its final review's L3: the frames the TAA read a mask on, the rows drawn into it (native_taa_frames)
    void NativeTaaFadeMask();
    void EncodeTaaFadeMask(ID3D12GraphicsCommandList* list,const NativeModelFrameInput& input,const NativeModelFrameAddresses& frame,
        NativeModelConstantUploads& uploads,const D3D12_VIEWPORT& viewport,const D3D12_RECT& scissor);
    std::atomic<bool> dlss_sharpen_failed_{};   // I-0132 R12c: the sharpening after DLSS (TaaStage::RecordSharpen), latched off on a failure
    void LatchTaaOff(uint64_t serial,const std::string& reason);
    EffectStage effects_;std::string effects_failure_;   // FFX-DRAW N6 (render thread)
    mutable std::mutex effects_lock_;EffectStage::Stats effects_stats_;std::string effects_last_failure_;   // copies for EffectSummary
    // I-0127 (effect-measure.flag): the frames since the last TakeEffectMeasure (under effects_lock_); the GPU times from two
    // timestamps a slot around the effect block (the stage, its interleaved 0x29 rows, its copies and the 0x31 layer), read at
    // the slot's release.
    EffectMeasureWindow effects_window_;
    Microsoft::WRL::ComPtr<ID3D12QueryHeap> effect_queries_;Microsoft::WRL::ComPtr<ID3D12Resource> effect_query_readback_;uint64_t effect_query_frequency_{};bool effect_query_failed_{};
    // I-0113 tranche 29 (effect-scan.flag): the scans' window (under effects_lock_) and one readback buffer a slot and stage, kept
    // while the picture's footprint is unchanged (a slot's buffer is read by its own completion before the slot records again).
    static constexpr size_t kEffectScanRows=256;EffectScanWindow effect_scan_;
    // A buffer that cannot be made latches effect_scan_failed_ for the device (the step review's L1: no retry a frame).
    std::array<std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,4>,max_frame_slots> effect_scan_readback_{};uint64_t effect_scan_bytes_{};bool effect_scan_failed_{};
    void RecordEffectScan(ID3D12GraphicsCommandList* list,FrameSlot& slot,ID3D12Resource* target,D3D12_RESOURCE_STATES state,uint32_t stage,uint32_t pass,uint64_t serial);
    // I-0113 tranche 30 (effect-scan-draws.flag): the per-statement count (EffectDrawScanRow). Begin clears this slot's counts
    // and writes the HDR's view (false: not scanned this frame, the objects could not be made: latched for the device); Point
    // records one count (the HDR from RENDER_TARGET and back, local barriers; sets its own heap, root and pipeline: the caller
    // binds its state again); End copies the counts into the slot's readback, read at the slot's release.
    static constexpr uint32_t kEffectDrawScanPoints=1024,kEffectDrawScanStride=64,kEffectDrawScanGrowth=16;static constexpr size_t kEffectDrawScanRows=512;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> draw_scan_root_;Microsoft::WRL::ComPtr<ID3D12PipelineState> draw_scan_pso_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> draw_scan_views_;Microsoft::WRL::ComPtr<ID3D12Resource> draw_scan_counts_;
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,max_frame_slots> draw_scan_readback_{};bool draw_scan_failed_{};
    std::vector<EffectDrawScanPoint> draw_scan_points_;ID3D12Resource* draw_scan_target_{};uint32_t draw_scan_width_{},draw_scan_height_{};
    bool BeginEffectDrawScan(ID3D12GraphicsCommandList* list,FrameSlot& slot,ID3D12Resource* hdr);
    void RecordEffectDrawScanPoint(ID3D12GraphicsCommandList* list,uint32_t kind,int32_t pass,uint32_t key,const NativeEffectDraw* draw);
    void EndEffectDrawScan(ID3D12GraphicsCommandList* list,FrameSlot& slot,uint64_t serial);
    bool GlowReady(const NativeGlowInput& input);
    // W10 TM-2; I-0138 tranche 1 (ROADMAP workstream 11): our tone-map pass on the recording thread, opened from the first frame
    // that carries the tone-map input; a failure is kept for the session and the frames keep the sRGB composition (tranche 8: the
    // original's programmes and their stages, TonemapStage and GradingStage, are gone from the module).
    std::atomic<bool> tonemap_failed_{};std::string tonemap_failure_;
    PostToneStage post_tone_;
    // I-0132 R11h: the automatic exposure (on by default, D-0264: the tone-map input's auto_exposure); a failure latches it off for the session
    // (the tone map then runs with the area's own exposure); its last reading and failure under auto_exposure_lock_ for the collect's row.
    AutoExposureStage auto_exposure_;std::atomic<bool> auto_exposure_failed_{};
    mutable std::mutex auto_exposure_lock_;std::string auto_exposure_failure_;AutoExposureReading auto_exposure_reading_{};
    GlareStage glare_family_;std::atomic<bool> glare_family_failed_{};   // W10 GL-3
    bool glare_cut_pending_{};   // I-0138 tranche 8f: a camera cut not yet given to the glare stage (a frame without glare draws keeps it)
    GlareStage dof_stage_;std::atomic<bool> dof_failed_{};std::string dof_failure_;   // W10 DF-4: the depth of field (a second glare stage)
    GlareStage motionblur_stage_;std::atomic<bool> motionblur_failed_{};std::string motionblur_failure_;   // W10 MB-4: the motion blur (a third glare stage)
    // W10 GR: the colour grading, our pass's LUT side, opened from the first frame that carries a LUT; a failure is kept for the
    // session and the frames keep the ungraded tone map.
    std::atomic<bool> grading_failed_{};std::string grading_failure_;
    bool GradingReady(const NativeGradingInput& input);
    bool TonemapReady(const NativeTonemapInput& input);
    // I-0126 R7c (D-0251; W8 AO-7 before): the mask stage on the recording thread, opened from the first frame whose mask input is
    // ready at the render extent (Resize follows it); a failure is kept for the session. The target bytes are published for the owner's row.
    MaskStage mask_;std::atomic<bool> mask_failed_{};std::string mask_failure_;std::atomic<uint64_t> mask_target_bytes_{};
    bool MaskReady();
    void FinishGraphShadow(GraphShadow& graph);   // E2/E3: End, the rows and the counters of one shadowed recording
    void DeclareViewGraph(GraphShadow& graph,bool lit);   // E2: a view context's G-buffer, depth and lighting resources
    bool native_neutral_ready_{};   // E5: the 1x1 neutral images cleared after their creation (then shader-readable for good)
    uint64_t graph_recordings_{};   // E2: the serial of a shadowed movie recording
    std::atomic<uint64_t> graph_shadow_frames_{},graph_shadow_mismatch_frames_{},graph_shadow_uses_{},graph_executed_frames_{},graph_barriers_{},graph_barrier_calls_{};
    std::mutex graph_shadow_lock_;std::vector<std::string> graph_shadow_rows_;
};
}
