#pragma once
// I-0111 (R1, D-0165): NVIDIA Streamline 2.14.16 in manual-hooking mode, the module's only route to DLSS. One instance per
// process. The interposer and its plugins are loaded from the module's folder (never the game folder) after their NVIDIA
// signature is checked; loaded at every start unless `no-dlss.flag` (I-0111 tranche 4 step 2; each frame's DLSS then follows the game's
// Antialiasing option). NGX itself may still run a newer DLSS
// model the NVIDIA App's DLSS override placed under ProgramData (i111-dlss-2's sl.log: « Feature dlss override enabled »).
// Manual hooking: the device and the command lists stay native; the DXGI factory is upgraded before the swap chain is made, so that the swap chain's Present
// reaches Streamline's per-frame bookkeeping (presentCommon); slShutdown runs before the device is released.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "scene_view.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <array>
#include <cstdint>
#include <string>

namespace ds3native::streamline {
// The interposer from `folder` (its signature checked), then slInit with manual hooking, DLSS requested, no over-the-air update,
// no console, its log in `folder`. False with a reason (nothing stays loaded). `sink` receives Streamline's warnings and errors.
// I-0131 R10b (R10-PLAN §3): `frame_generation` also loads DLSS-G (sl.dlss_g) with the frame-based tags it needs (the global tags;
// DLSS's own tags stay local to its evaluation, ProgrammingGuide §2.12); the chain made while it is loaded is its chain (§4).
bool Initialise(const std::wstring& folder,void (*sink)(const char* row),std::string& reason,bool frame_generation=false) noexcept;
bool Initialised() noexcept;
// slSetD3DDevice and whether DLSS runs on the adapter (slIsFeatureSupported with its LUID).
bool SetDevice(ID3D12Device* device,const LUID& adapter,std::string& reason) noexcept;
// The factory the swap chain is made with: Streamline's proxy of `factory` (a new reference in `proxy`).
bool UpgradeFactory(IDXGIFactory4* factory,IDXGIFactory4** proxy,std::string& reason) noexcept;
// Initialised, the device set and DLSS supported: the frame may jitter and evaluate (read on the game and render threads).
bool DlssReady() noexcept;
// DLSS off for the session (the renderer's own refusal, such as its output's allocation): DlssReady false from now on.
void Disable() noexcept;
// I-0123 (RR-PLAN §2): DLSS Ray Reconstruction (sl.dlss_d, loaded with DLSS): DlssReady and the adapter offers it, not latched off.
// RrDisable latches it off for the session (its own refusal; DLSS Super Resolution stays).
bool RrReady() noexcept;
void RrDisable() noexcept;
// I-0140 tranche 40 (D-0305): the menu views' DLAA, each view on a viewport of its own (2 and up: 0 is the world's, 1 NIS's) with its
// own options. MenuDlssReady: DlssReady and the menu views' DLSS not latched off (a refused menu evaluation latches only them off;
// the world's DLSS stays). FreeMenuDlss: one menu viewport's resources released (no list with its evaluation in flight); Shutdown
// releases every one.
bool MenuDlssReady() noexcept;
void FreeMenuDlss(uint32_t viewport) noexcept;
// I-0111 tranche 3: the DLSS modes the module offers (NVIDIA's DLAA and the scaled modes; NVIDIA's default preset for each).
enum class DlssMode : uint8_t {Dlaa,Quality,Balanced,Performance,UltraPerformance};
const char* DlssModeName(DlssMode mode) noexcept;
// The render extent DLSS asks for `mode` at an output extent (slDLSSGetOptimalSettings; the output extent itself for DLAA). False
// with a reason when the feature cannot answer yet (its functions exist once it has the device and a swap chain) or answers nothing.
// I-0123: `rr`, Ray Reconstruction's (slDLSSDGetOptimalSettings: its plugin creates NGX at its own optimal size).
bool DlssRenderExtent(DlssMode mode,uint32_t output_width,uint32_t output_height,uint32_t& width,uint32_t& height,std::string& reason,bool rr=false) noexcept;
// I-0111 tranche 2 (Reflex, D-0240: a player option, never the frame's only clock; CADENCE-REFLEX-READY §5): Off, On, On + Boost.
// Reflex and PCL load with DLSS (and only where DLSS runs: an adapter with Reflex but not DLSS gets neither, step review L7); once the device is set, ReflexReady says whether Reflex's functions answered (the driver's own say is ReflexLowLatencyAvailable; PCL's markers are
// sent whatever the mode, as Streamline asks). SetReflexMode sends the options (at least once, even Off), again only on a change.
enum class ReflexMode : uint8_t {Off,On,Boost};
const char* ReflexModeName(ReflexMode mode) noexcept;
bool ReflexReady() noexcept;
bool SetReflexMode(ReflexMode mode,std::string& reason) noexcept;
// slReflexGetState's lowLatencyAvailable, read when the options were last accepted: 1, 0, or -1 when Reflex is not ready or did not
// answer (the driver's own say on low latency).
int ReflexLowLatencyAvailable() noexcept;
// Reflex's sleep before frame `frame` starts (the engine thread; it returns at once with Reflex Off).
void ReflexSleep(uint64_t frame) noexcept;
// One PCL marker of frame `frame` (the job serial; one index space with DLSS's frame token). Thread-safe.
enum class PclMarker : uint8_t {SimulationStart,SimulationEnd,RenderSubmitStart,RenderSubmitEnd,PresentStart,PresentEnd};
void Marker(PclMarker marker,uint64_t frame) noexcept;
// The DLSS feature's resources released (the GPU idle), then slShutdown; the interposer stays loaded (its proxies may still be
// referenced by a released swap chain's last owners until process exit).
void Shutdown() noexcept;

// One frame's DLSS evaluation (render size = output size for DLAA, smaller in a scaled mode). Matrices row-vector (v' = v M) as
// the engine's, unjittered; the jitter in render pixels; each resource's state is the one it is in when the evaluation runs.
struct DlssFrame {
    uint32_t frame{};                    // the frame index (Streamline's frame token)
    uint32_t viewport{};                 // I-0140 tranche 40: 0 the world's; 2 and up a menu view's (DLAA, its own exposure estimate, no RR)
    DlssMode mode{DlssMode::Dlaa};
    ID3D12Resource *color{},*depth{},*motion{},*output{};
    D3D12_RESOURCE_STATES color_state{},depth_state{},motion_state{},output_state{};
    // I-0111 tranche 2 (D-0242): DS3's exposure (1x1 R32_FLOAT, the tone map's multiplier of the linear HDR), tagged as DLSS's
    // exposure with auto-exposure off; null: DLSS's auto-exposure.
    ID3D12Resource* exposure{};D3D12_RESOURCE_STATES exposure_state{};
    uint32_t render_width{},render_height{},output_width{},output_height{};
    float jitter_x{},jitter_y{};
    bool reset{};
    Matrix4 view_to_clip{},clip_to_view{},clip_to_previous_clip{},previous_clip_to_clip{};
    std::array<float,3> position{},up{},right{},forward{};
    float near_plane{},far_plane{},vertical_fov{},aspect{};
    // I-0123 (RR-PLAN §1): Ray Reconstruction in place of DLSS (no exposure: RR reads none, its guide §3.7); its guides at the render
    // extent: the diffuse and specular albedo, the world normal with the roughness in w (packed), the colour before the transparent
    // rows (optional: null); I-0125 R6c: the specular hit distance (optional: null); all in `guide_state`.
    bool rr{};
    ID3D12Resource *albedo{},*specular_albedo{},*normal_roughness{},*before_transparency{},*specular_hit_distance{};
    D3D12_RESOURCE_STATES guide_state{};
    Matrix4 world_to_view{},view_to_world{};   // row-vector as the engine's (RR's specular hit distance, R6)
};
// Options (when the mode or a size changed), constants, tags and slEvaluateFeature on `list`. Streamline binds its own heaps, root
// signature and pipeline and restores none (manual hooking): every pass after it binds its own. False with a reason. I-0123: a
// refused Ray Reconstruction latches only RR off (RrReady false from then on), DLSS stays ready.
bool EvaluateDlss(ID3D12GraphicsCommandList* list,const DlssFrame& frame,std::string& reason) noexcept;

// I-0131 R10b (R10-PLAN §3; ProgrammingGuideDLSS_G): DLSS frame generation, requested at Initialise. FgLoaded: requested, the plugin
// loaded and the adapter supported (from SetDevice on; the chain is then made without the frame-latency object, §12.1). FgReady:
// loaded, not latched off and DLSS ready (its functions are asked at the first SetFrameGeneration). FgDisable latches it off for the session (its options set Off once more).
bool FgLoaded() noexcept;
bool FgReady() noexcept;
void FgDisable() noexcept;
// I-0132 R12f (the settings live; ProgrammingGuideDLSS_G §18): DLSS-G requested at Initialise and supported on the adapter
// (FgSupported), then loaded or unloaded while no chain exists (SetFrameGenerationLoaded): the chain made after it is Streamline's
// proxy with DLSS-G, or the native one without it (§12.1: the frame-latency object then ours again). False with a reason.
bool FgSupported() noexcept;
// I-0132 R12f (Thomas, 2026-11-11 03:53): NVIDIA Image Scaling's sharpen-only mode (Streamline's sl.nis) on the tone-mapped 8-bit colour:
// `input` (in `input_state`) sharpened into `output` (unordered access, in `output_state`), both width x height; `sharpness` 0 to 1.
// False with a reason; a refusal latches NIS off for the session (NisReady false).
bool NisReady() noexcept;
void NisDisable() noexcept;
void NisFreeResources() noexcept;   // before the tagged targets are made again
bool EvaluateNisSharpen(ID3D12GraphicsCommandList* list,uint64_t frame,ID3D12Resource* input,uint32_t input_state,ID3D12Resource* output,uint32_t output_state,
    uint32_t width,uint32_t height,float sharpness,std::string& reason) noexcept;
bool SetFrameGenerationLoaded(bool loaded,std::string& reason) noexcept;
// The frame's DLSS-G inputs (§5): the depth and the motion vectors of DLSS's evaluation at the render extent, with the same frame
// token (its constants, EvaluateDlss's), and the HUD-less colour at the back buffer's size; each in the state it is in on `list`,
// tagged eOnlyValidNow (Streamline copies them on `list`: the next frame reuses the targets). False with a reason.
bool TagFrameGenerationDepth(ID3D12GraphicsCommandList* list,uint32_t frame,ID3D12Resource* depth,D3D12_RESOURCE_STATES depth_state,
    ID3D12Resource* motion,D3D12_RESOURCE_STATES motion_state,uint32_t render_width,uint32_t render_height,std::string& reason) noexcept;
bool TagFrameGenerationHudless(ID3D12GraphicsCommandList* list,uint32_t frame,ID3D12Resource* hudless,D3D12_RESOURCE_STATES state,
    uint32_t width,uint32_t height,std::string& reason) noexcept;
// The mode for the next Present, on the presenting thread before it (§6.0): on for a frame tagged above, off otherwise (§6.4: the
// resources retained). Sent only on a change. False with a reason (a refusal latches frame generation off).
bool SetFrameGeneration(bool on,std::string& reason) noexcept;
// The multiplier asked (2 to 4, Thomas 2026-11-08: x3 and x4 for the RTX 50's multi frame generation, §6.2); each On sends
// min(multiplier - 1, numFramesToGenerateMax) generated frames (the adapter's maximum read from slDLSSGGetState, 1 on an RTX 40).
void SetFrameGenerationMultiplier(uint32_t multiplier) noexcept;
// Once latched off and its Off gone through a Present: DLSS-G's resources freed (§6.4: a long deactivation; eRetainResourcesWhenOff
// keeps them otherwise). On the presenting thread, after a Present. The step review's L5.
void ReleaseFrameGeneration() noexcept;
// Reflex ready and its mode sent On or Boost (DLSS-G requires it, §8: the step review's M2).
bool ReflexOn() noexcept;
// slDLSSGGetState (no VRAM estimate): the frames presented per frame since the last call, the status bits, the multiplier's maximum,
// whether V-Sync is supported with it. False when not ready or refused.
struct FgState {uint32_t presented{},status{},generate{},generate_max{},min_size{};bool vsync{};};   // generate: the frames sent to generate   // min_size: minWidthOrHeight (§14.3)
bool FrameGenerationState(FgState& state) noexcept;
}
