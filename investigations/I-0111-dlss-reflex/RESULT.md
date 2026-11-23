# I-0111 - DLSS et Reflex (R1): result

## Conclusion

Complete (closed 2026-11-09 with Thomas's yes, « Closed » below); its history: Active (2026-10-30). DLSS is on by default (Thomas, 2026-10-31 00:45): Streamline is loaded at every start (`no-dlss.flag` to
refuse it) and a frame runs DLSS whenever the game's own Antialiasing option is on, in the mode named in `dlss-mode.txt` (one
word), NVIDIA's DLAA otherwise. Tranche 1 (DLAA through Streamline 2.14.16) merged `63fdc8a9`; tranche 3 (the scaled modes)
merged; tranche 4 step 1 (the reactive mask) merged `64b8e01d`, step 2 (the game's option, the mode file) merged `b21d5b9a`, start
check `start-b21d5b9a` passed; Thomas's first play after it (2026-10-31 07:14) ran without DLSS: the launchers copied the module but not Streamline's DLLs (`native_streamline` refused, « sl.interposer.dll absent »; the start check runs from the build folder, which has them); fixed in the launchers (`ffd4b4d8`): the 07:25 play ran DLAA (`native_streamline` ready, `native_dlss_option` active). Thomas compared DLAA and Performance in play (08:03): no difference he can see, the darkening included. Open: in exclusive full screen with Performance (09:01 play), the quit through the title screen ended the module on `native_output_resize_failed` (ResizeBuffers through Streamline's proxy: 887A0001). Reproduced (`i111-fs-quit-1`, 09:26-09:29, exclusive full screen on the ASUS, DLSS Performance: three Alt-Tabs resized fine; at the close ResizeBuffers answered E_ACCESSDENIED through Streamline's proxy, then the chain refused GetBuffer, DXGI_ERROR_INVALID_CALL, and the follow refused). Fix (branch `claude/r1-quit-fullscreen`, final review answered): when the chain refuses its buffers after a failed resize, stand-in textures of its size take their place, every frame follows until a resize succeeds; I-0101's premise that the buffers survive a failed ResizeBuffers is refuted with Streamline's proxy. Verified: `i111-fs-quit-2` (09:56-09:59, the first version) and `i111-fs-quit-3` (10:22-10:26, after the final review), exclusive full screen with DLSS Performance: the Alt-Tabs resized, the close's ResizeBuffers answered E_ACCESSDENIED, the stand-ins took over (`"stand_in":true`, the follow's rows limited by its failing count, 7 rows) and the game left by itself, exit 0, no refusal, restored. Merged `674d3988`; the launcher rebuilt on `6505dbb7` (with I-0113 tranche 7), start check `start-6505dbb7` passed (Streamline ready, DLSS active by the option). Beyond ER's FUN_141E9F100, which takes the buffers again (D-0195: an addition, named). Residual: the full-screen toggle, the display mode change and the output resize still refuse on the same answers (I-0101 L4). Tranche 2, the modern cadence with Reflex, moved into I-0114 « Le framerate débloqué » (D-0242,
Thomas 2026-10-30 23:39): there the frame-start wait helps only once the logic is unlocked. One visible residual left open: the
frame 4 to 7 % darker as the render extent shrinks, from the SSAO mask (below), left for the traced GI unless Thomas sees it in
play.

**The NVIDIA App's DLSS override stays on** (Thomas, 2026-10-30 08:14: « je garde l'override, d'ailleurs la plupart des gens
utilisent l'override »): witness runs keep it and record the preset it chose (in Streamline's `sl.log`: `Override Reported:
ModelPreset, Value: N`, 12 = preset L in `sl_dlss.h`). `i111-dlss-2` ran preset L. The project's `nvngx_dlss.dll` is 310.9.16,
the version the override loaded too (DLSS 4.5, presets L and M); without the override NVIDIA's defaults are K for DLAA, Quality and
Balanced, M for Performance, L for Ultra Performance. Thomas (08:17): « Laisse le réglage Nvidia par défaut, les gens utiliseront
l'override au pire »: the module sets no preset (`DLSSPreset::eDefault`).

## Plan

| Tranche | Content | State |
|---|---|---|
| 1 | Streamline host (manual hooking), camera jitter with unjittered motion vectors, the velocity forced on, DLSS evaluated on the lit HDR at render = output size (NVIDIA's DLAA), the history reset, DS3's AA filter off; `dlss.flag` | in progress |
| 2 | Reflex (Off, On, On + Boost) and the latency markers, an option that can stay off (Thomas, 2026-10-30 08:21: « Reflex doit rester une option »); its sleep coordinated with the cadence study's single wait; the exposure input. With it, the cadence by the modern loop (D-0240, [CADENCE-REFLEX-READY](CADENCE-REFLEX-READY.md)) | step 1, the display clock behind `display-clock.flag`; the rest written inside I-0114 by D-0242 (branch `claude/blissful-hugle-47bbb9`, 2026-10-31): Reflex by `reflex-mode.txt` (Off by default), PCL's markers, DS3's exposure to DLSS; the record in [I-0114 RESULT](../I-0114-unlocked-framerate/RESULT.md) « DS3's exposure to DLSS and Reflex » |
| 3 | The scaled modes: render extent distinct from the output extent, optimal render sizes, mip bias, output-size depth for the YEBIS stages | done (branch `claude/jovial-greider-284602`) |
| 4 | The reactive mask, the motion of effects, water and forward rows; the mode in the game's options; DLSS on by default once accepted | step 1, the reactive mask, merged 2026-10-30 (branch `claude/exciting-visvesvaraya-0537da`); step 2, the mode by the game's Antialiasing option (Thomas 23:41), merged `b21d5b9a` 2026-10-31, start check `start-b21d5b9a` passed |

## Tranche 1: NVIDIA DLAA through Streamline (written 2026-10-30)

What the code does (the design of UPSCALER-DLSS-READY, adapted to Streamline's manual hooking, D-0165):

1. **Streamline host** (`native/renderer/streamline_host.{h,cpp}`): `sl.interposer.dll` loaded dynamically from the module's
   folder after `sl::security::verifyEmbeddedSignature` (the module never links `sl.interposer.lib`: without the DLL or its
   signature DLSS is only off); `slInit` with `eUseManualHooking | eDisableCLStateTracking | eDisableDebugText` (the default flags'
   over-the-air update and downloaded plugins of Streamline are left out; NGX itself still takes the NVIDIA App's DLSS override when
   Thomas has it on: final review M2 below), DLSS requested, an
   engine type « custom » with the project's own GUID (no NVIDIA application id), its log beside the module and its warnings as
   `native_streamline_message` rows; `slSetD3DDevice` with the native device and `slIsFeatureSupported` on its adapter's LUID;
   the DXGI factory upgraded to Streamline's proxy just for `CreateSwapChainForHwnd`, so the chain's `Present` reaches
   `presentCommon` (manual hooking's one requirement); the device and the command lists stay native. At close: the proxy chain
   released, `slFreeResources`, `slShutdown`, then the device (the order without Streamline unchanged). Any refusal (missing or
   unsigned DLL, `slInit`, the device, the factory) leaves DLSS off and names its reason in the start row `native_streamline`.
2. **Jitter** (`native/renderer/upscaler.h`): Halton (2, 3) from index 1 minus 0.5, `ceil(8 (output / render)^2)` phases (8 for
   DLAA), indexed by the camera serial. One injection point: `SceneViewInput.jitter_x/y` on the main world camera only (both
   producers of the native scene camera call `ApplyUpscalerJitter` while `DlssReady()`), forwarded by `ProduceCameraConstants` to
   the camera producer's projection words (`m20 = 2 jx / W`, `m21 = -2 jy / H`; zero keeps the engine's `-0.0` words), so
   `FC_MatrixInvProj`, `FC_MatrixInvViewProj`, `VC_MatrixViewProj`, the view constants of every model draw (from byte 256) and the
   decals' projection carry it. `BuildSceneView` stays unjittered (culling, the velocity's matrices); the reflection's camera clears it.
3. **Unjittered motion vectors** (`CollectNativeVelocityInputs`): the object rows' b0 block takes the current frame's NDC shift on
   both matrices (it cancels in their difference); the resolve inverts the jittered current matrix, reprojects with the
   unjittered previous one and adds the jitter back to its output uv (UPSCALER-DLSS-READY §3.3); the history keeps the
   unjittered matrix. While DLSS is ready the velocity pass and its object rows run whatever the motion blur option says. The
   unjittered `clip_to_previous_clip` travels with the velocity input for Streamline's constants.
4. **Evaluation** (`RenderModels`, after the velocity, before the YEBIS passes): the lit HDR, the depth (both planes) and the
   motion vectors in compute-readable states, a `dlss_output_` texture (the lit HDR's format, unordered access) as the output,
   Streamline told each state; `slDLSSSetOptions` (DLAA, the output extent, HDR colour, auto-exposure in this tranche) when the
   extent changes; `slSetConstants` (the unjittered projection and its inverse, current-to-previous clip and its inverse, the
   jitter, `mvecScale = (1/W, 1/H)`, the camera's frame, depth not inverted, camera motion included); the tags travel with
   `slEvaluateFeature`; the output copied back into the lit HDR. The frame graph declares the passes `dlss` and `dlss_copy`
   (plan key bit 40). DS3's own anti-alias filter is skipped while DLSS runs. A refused evaluation leaves the frame as it was and
   latches DLSS off for the session (the game thread stops the jitter at its next frame).
5. **History reset**: the velocity's cut rule (first frame, a view change, a camera cut), the first DLSS frame and a new output
   extent.
6. **Build**: `Build-GameBootstrap.ps1` compiles `streamline_host.cpp` with Streamline's headers and copies `sl.interposer.dll`,
   `sl.common.dll`, `sl.dlss.dll`, `nvngx_dlss.dll` and `NvLowLatencyVk.dll` (imported by `sl.common.dll`) beside the module;
   `upscaler_test` (T-U2, T-U3, T-U5, T-U7, T-U13) runs with the unit tests; `Run-StartupTrace.ps1` accepts `dlss` as an effect flag.

Known limits of this tranche (the next ones' work): the effects (FFX) rasterise with the game's own unjittered matrices and have
no motion vectors (the reactive mask, tranche 4); the SSAO reprojection and the YEBIS motion blur's camera velocity use the game's
unjittered constants (a sub-pixel error, the SSAO to be removed by the traced GI, D-0237); auto-exposure until tranche 2.

## Tranche 3: what the render extent apart from the output extent touches (first inventory, 2026-10-30, read only)

Read in the code at `faff4d62`, for the thread that takes the tranche (UPSCALER-DLSS-READY §3.1 is the design; its line references
predate the split of `game_bootstrap.cpp`):

1. **One size today, written into the engine.** `output_width/height` (`01_state.inc:347`, the presentation size, resized in
   `02_frame_lifecycle.inc:525-539`) is written into the engine's own records: the camera output `+0x5C/+0x60`
   (`14_scene_step_and_storage.inc:95`, `03_engine_cpu_owners.inc:716`), the GX contexts `+0x20/+0x24` and `+0x3F3C/+0x40`
   (`03_engine_cpu_owners.inc:500-515`), each scene view `+0xAC/+0xB0` (`:592`, `02_frame_lifecycle.inc:536`). The engine's own
   CPU-side constants (the effects' screen sizes, the YEBIS plans) therefore follow it: which of them must take the render size is
   the tranche's first question (an effect's soft-particle depth lookup divides `SV_Position` by its screen size).
2. **The camera's `width/height` serves both sizes.** Render-sized uses: the light grid's groups (`12_scene_collect.inc:378`), the
   forward rows' grid (`13b_prepare_model_frame.inc:12`), the decals' screen size (`13_model_frame_inputs.inc:245`), the AO view
   (`12_scene_collect.inc:304`), the velocity's jitter words; output-sized uses: the YEBIS glare, DoF and motion blur plans
   (`13b_prepare_model_frame.inc:1234`, `:1272`, `:1325`), the vignette, the glow (`05_selected_scene.inc:592`). The prepare refuses a
   camera that is not the output size (`13b_prepare_model_frame.inc:25`), as `RenderModels` does (`scene_pass_frame.cpp:134`).
3. **The renderer's 59 reads of `presenter_.width/height`** (`scene_pass.cpp` 9, `scene_pass_frame.cpp` 32 with the world frame's
   viewport and scissor, `scene_pass_velocity.cpp` 10, `scene_pass_decals.cpp` 6, `scene_pass_lighting.cpp` 2): the G-buffer,
   depth, lighting, forward, water and effect copies, velocity, decal and readback targets take the render size; the composition,
   UI (its depth `PrimitiveDepthTarget`), display, glow and AA the output size.
4. **After DLSS** the YEBIS chain reads DLSS's output-sized result: read `dlss_output_` as the chain's source (also the final review's
   L3, the full copy back into the lit HDR); the stages that read a depth (DoF's factor, Alpha_R, the motion blur) need one at the
   output size (§3.7) or stay off in a scaled mode until it exists.
5. **The modes' render sizes** come from `slDLSSGetOptimalSettings` (to fetch with `slGetFeatureFunction` once the device is set,
   as `slDLSSSetOptions`), the material samplers' mip bias from §3.8, the jitter's phases from `upscaler::JitterPhases(render,
   output)` (already written for it). NVIDIA's default presets stay (Thomas, 08:17).

### Tranche 3: written and witnessed (2026-10-30)

What the code does now (on top of the plan below, all four steps written; the default frame and DLAA keep their extent):

- **Renderer.** `SetRenderExtent(width, height, mode)` between frames (the worker quiesced, the frames in flight waited for, the
  world's targets remade); a `Resize` to another presentation extent returns the world to it, the same extent (a full-screen
  toggle) keeps it. At the render extent: the G-buffer, depth, lighting, velocity and its readbacks, AO, sun mask, the scene colour
  copy, the decal and water copies. DLSS's output (the presentation's extent, render target and unordered access, `RENDER_TARGET`
  between frames) is the post chain's source, its composition view at binding +2; the tone map, glare, composition, AA, glow, UI and
  display at the presentation's extent. **§3.7**: after DLSS, an output-size D32S8 depth drawn from the render depth (each output
  pixel takes the render texel under its point in the jittered image, `SV_Depth`), read by Alpha_R, the DoF and the motion blur;
  the motion blur's half-size object velocity at half the presentation's extent (the render-size velocity's 2x2 block under it).
- **Streamline.** The mode in the options (`DLAA`, `MaxQuality`, `Balanced`, `MaxPerformance`, `UltraPerformance`; NVIDIA's default
  preset for each); `DlssRenderExtent` asks `slDLSSGetOptimalSettings`.
- **Game side.** `dlss-quality.flag`, `dlss-balanced.flag`, `dlss-performance.flag`, `dlss-ultra-performance.flag`;
  `FollowNativeRenderExtent` at the engine frame's start; the engine's records keep the output extent, the main camera the
  native passes see takes the render extent with the output's aspect made explicit (FC_ScreenSize, the decals', the light grid's,
  the AO's and the effects' sizes follow it), its jitter `ceil(8 (output / render)^2)` phases; the YEBIS plans, the vignette, the
  glow, DS3's AA and the sun mask's blur taps at the output extent (`NativePostExtent`). **§3.8**: while DLSS runs, the main view's
  material samplers (banks 12 and 13) take `log2(render / output) - 1` through a derived sampler bank (-1 at DLAA).
- **A latent bug found by the scaled extents**: the velocity and the atlas-dump depth readbacks mapped `RowPitch x height`, past the
  buffer when the width is not a multiple of the pitch (853, 742): `i111-q-1` and `i111-b-1` refused at start; fixed (`ea7f4746`).

Witness runs (announced, the ASUS at 1920x1080 60 Hz, the game windowed at 1280x720, the replayed Firelink path with Eta, debug layer
on, no measure; `evidence/I-0066/artifacts/<run>`, the series logs `evidence/I-0111/artifacts/t3-series-{1,2,3}.log`):

| Run | Build | Mode | Render extent | Frames | Outcome |
|---|---|---|---|---|---|
| `i111-q-1`, `i111-b-1` | t3-build-2 | Quality, Balanced | 853x480, 742x418 | ~1,550 | refused: the readback map (fixed) |
| `i111-p-1` | t3-build-2 | Performance | 640x360 | 12,393 | clean |
| `i111-p-2`, `i111-a-2`, `i111-q-2`, `i111-b-2` | t3-build-3 | Performance, DLAA, Quality, Balanced | 640x360, 1280x720, 853x480, 742x418 | 12,279-12,511 each | clean |
| `i111-p-3` | t3-build-4 | Performance | 640x360 | 12,459 | clean |
| `i111-q-4`, `i111-u-4` | t3-build-5 | Quality, Ultra Performance | 853x480, 427x240 | 12,374, 12,465 | clean |
| `i111-ra-5`, `i111-rq-5` | t3-build-6 | DLAA, Quality, with `test-dlss-refuse.flag` | 1280x720, 853x480 | 12,453, 12,315 | latched off at the 600th evaluation (`native_dlss_latched_off`), ran on to the end; at Quality the extent returned to 1280x720 (`native_render_extent` « output ») |

In each clean run: `native_render_extent` names the scaled mode and the optimal size (none at DLAA, by design), `native_dlss_frames`
counts every frame (no `native_dlss_latched_off`, no `native_dlss_unresolved`), the graph shadow found no mismatch, the debug layer
stayed silent, the restoration held. Streamline's `sl.log` (overwritten each run, kept for the last run of each build) shows the
NVIDIA App's override choosing preset L (`ModelPreset, Value: 12`) at Performance (`t3-build-2`, `t3-build-4`) and Balanced
(`t3-build-3`).
OBSERVED (pictures `evidence/I-0111/artifacts/t3-zoom-dlaa-vs-{q,b,p}.png`, `t3-end-dlaa-vs-{q,b,p}.png`): the scaled modes fill the
output, framed as DLAA, sharp at Quality, softer at Performance.

**Open: the frame darker as the render extent shrinks** (mean of the game area against DLAA: -4 % Quality, -5 % Balanced, -7 %
Performance). The lossless dumps of `i111-p-2` and `i111-a-2` (frame 1800) place it before DLSS: the lit HDR -4 %, the shadow and
AO mask t32 219.6 against 235.3; the HDR's other inputs (GB5) equal. The SSAO programme scales its kernel by
`clamp(cb2[10].x / z, cb2[10].y, cb2[10].z)` in AO-layer pixels with only the base following the height: the bounds now follow the
render extent's share (`6d69ad85`), and the sun mask's blur taps the output's; `i111-p-3` shows no change (the mask 218.8), so the
cause is elsewhere in the AO chain (the 2x2 minimum depth downsample, the bilateral upscale or the history: INFERRED candidates, not
read). The SSAO is to be replaced by the traced GI (D-0237); left open for that, or for a reading of the AO chain if Thomas finds it
visible in play.
Thomas (2026-10-31 08:04: « si c'est un comportement anormal l'assombrissement alors faut le traiter »): taken up. The masks' channels
(`world-1800-mask.rgba` of `i111-a-2`, `i111-p-2`, `i111-p-3`, read 2026-10-31): x (the AO) 235.2 at DLAA, 219.6 and 218.8 at
Performance; y 255.0, z 0.3 and w 255.0 in all three. OBSERVED: the whole difference is the SSAO term, the other mask terms equal.
Next: the same frame without DLSS at the render extent (the output at 640x360) against 1280x720, to tell DS3's own SSAO's
resolution dependence (then the same in the original) from a DLSS-path defect.
Measured (2026-10-31 11:48-12:11, the quit fix's build, `no-dlss`, `world-pictures`; `i111-ao-native1280` 1280x720 window and
`i111-ao-native800` 800x450, DS3's smallest window, 640x360 asked; the small windows lost the foreground to the Claude app so
their replay input was refused, but frames 1500-2400 come before the path's first input in every run: the same spawn view):
the mask's x (the SSAO) is 232.7 at 1280x720 and 225.7 at 800x450 without DLSS (-3.0 % at 0.625 of the extent), against 235.2
at DLAA and 219.6 at Performance (render 640x360, -6.6 % at 0.5). OBSERVED: the darkening follows the extent the SSAO runs at,
without DLSS as with it: the SSAO's own resolution dependence (DS3's programmes and constants, ported), not a defect of the DLSS
path. INFERRED: the original darkens the same at a lower resolution (not run). Decided (Thomas, 2026-10-31 12:24: « D'accord, on laisse tel quel donc »): left as it is, to the traced GI that replaces the
SSAO (D-0237).

**Step review** (a fresh Opus at high effort, on `5fddfda4..614b927c`; no high finding): M1 a frame at a scaled extent that DLSS
does not resolve (a latch, no velocity or lighting yet) drew its smaller colour 1:1 in the corner: such a frame now shows no world
(the target cleared, the glow off), and the prepare refuses the main view while a scaled extent waits to return after a latch; L1 a
refused evaluation at DLAA shows its lit HDR (copied into the output), in a scaled mode the previous output for that one frame; L2 a
refused optimal-settings query is asked again every 600th frame, not each frame under Streamline's lock; L5 the output-size depth
is made at the first scaled frame (none at DLAA); L6 the AO bounds' comment says INFERRED; L7 `Close` resets the render extent
and the mode. Kept: L3 the mip bias biases bank 12, which the material table also gives s5 (10AAD0: s0..s3, s5 and s10 share one
sampler in the original), and leaves s8/s9 (banks 1 and 4) at 0 until row UL9 says what they sample (§3.8's « s5 stays at 0 » is
not separable without a per-slot override); L4 an output resize in a scaled mode remakes the world's targets twice (an event, not
per frame); L8 the new statics are under a mutex or on the engine thread.

**Final review** (a fresh Opus at xhigh, on the final tree with `main` merged, `0818970e`): H1 the step review's L1 answer (a refused
evaluation at DLAA copying the lit HDR into the output) declared a pass only on a refusal, outside the frame graph's plan key: the
executor (on by default, D-0171) would throw and fault the session; removed, a refused frame shows the previous output for that one
frame at every extent, and `test-dlss-refuse.flag` (the 600th evaluation refused once) witnessed the latch at DLAA and Quality
(`i111-ra-5`, `i111-rq-5`). M1 the AO darkening: an exit criterion not met, left open with its numbers (above). M2 the latch, a
refused evaluation and Ultra Performance had no live evidence: now witnessed (`i111-ra-5`, `i111-rq-5`, `i111-u-4`); a resize or a
full-screen toggle in a scaled mode is still not run live. L1 a resize returns the mode to DLAA too; L2 the output-size depth's
pipeline is made with it, not while recording; L3 the post generation computed after it; L4 comments; L5 an unreachable check
removed; L6 the `sl.log` and DLAA claims corrected (above); L7 this section, the conclusion, evidence and artifacts. Kept: L8 no new
unit test for the tranche's mappings (the output-depth texel, the velocity-half ratio, `NativePostExtent`), witnessed live instead;
L9 the mip bias on s5 (step review L3). The step review of these answers (a fresh Opus at high effort, `0818970e..fcc000fa`): no high
or medium finding; its low one, a refusal at a new output's first evaluation shows a black world for that frame (the output is a new
resource), said in the comment; the resize and the pipeline's failure branch have no live run.

**Seen on the way, not DLSS's**: the posterised dark areas (« banding ») of `i111-plain-1` are in the native frame, intermittent from
run to run: the lossless back-buffer dumps of `i111-a-2` have 27 to 52 distinct dark colours where `i111-p-2` has about 1,950, same
build and the same YEBIS rows, the lit HDR smooth in both (so after the HDR, in the post chain); answered to the open-gaps thread.

### Tranche 3 plan (2026-10-30, the thread that took the tranche over from the RT pilot's WIP `7f52182a`)

Opt-in, each step leaving the default frame (no `dlss.flag`) and DLAA's render extent as before:

1. **The render extent apart** (the renderer): `SceneRenderer::SetRenderExtent(width, height, mode)`; the world's targets, depth,
   lighting, velocity, AO, sun mask, scene copy, decal and water copies, the velocity readbacks at the render extent; DLSS's output
   at the presentation's extent (render target and unordered access, `RENDER_TARGET` between frames) is the post chain's source
   (no copy back into the lit HDR: final review L3), its composition view at binding +2; the post passes' viewports at the
   presentation's extent. **The game side**: `dlss-quality.flag`, `dlss-balanced.flag`, `dlss-performance.flag` or
   `dlss-ultra-performance.flag` choose the mode (the game's options in tranche 4); the render size from
   `slDLSSGetOptimalSettings`; `FollowNativeRenderExtent` at the engine frame's start (after the output follows S) sets it, the worker
   quiesced; the engine's own records keep the output extent, the main camera handed to the native passes takes the render extent
   with the output's aspect made explicit, its jitter `ceil(8 (output / render)^2)` phases; the YEBIS plans, the vignette, the glow
   and DS3's AA at the output extent (`NativePostExtent`), the effects' viewport at the render extent. Alpha_R, DoF and the motion
   blur stay off in a scaled mode until step 2.
2. **The depth at the output size** (§3.7) for Alpha_R, DoF and the motion blur: the render depth's nearest texel at each output
   pixel's jittered position, drawn as an output-size depth; the motion blur's half-size object velocity from the render-size target.
3. **The mip bias** (§3.8): while DLSS runs, the main view's material samplers (banks 12 and 13 of 10AAD0's table: s0..s3, s5, s10,
   s11) take `log2(render / output) - 1` through a derived sampler bank; the reflection and the menu views keep the source bank.
4. Witness runs (announced, the ASUS): Quality, then Balanced and Performance, a still camera and a pan, the NVIDIA App's preset
   read in `sl.log`; pictures against DLAA and against the frame without DLSS.

**Step review** (a fresh Opus at high effort, 2026-10-30, on `98d89df0`; no high finding): M1 a jittered frame DLSS does not resolve
(the velocity dropped when a motion-blur-only word is unread, or no lighting yet): while DLSS runs the velocity no longer needs
those words (Reference1188's length 0.1 and the .rdata epsilon as fallbacks), and such frames are counted in
`native_dlss_unresolved`; M2 DLSS's frames and its latch reached no row: `native_dlss_frames` (the first, then every 3,600) and
`native_dlss_latched_off` with the reason; M3 the scene and view tables are never carried while DLSS runs (the camera block
changes every frame, as with a moving camera): kept, the tables hold the jittered words, the cost is that of a moving camera;
L1 a statement lost in a comment, restored; L2 one native factory reference leaked per start (the proxy takes its own), removed;
L3 a failed start left Streamline initialised, now ended in `Close`; L4 the output's allocation and a chain the proxy refuses now
leave DLSS off instead of faulting; L5 the lit HDR's alpha after DLSS (`alphaUpscalingEnabled` off) is DLSS's: DoF and Alpha_R
rewrite it, no later pass is known to read the HDR's alpha before them (open, UPSCALER-DLSS-READY §7); L6 the clip matrix is made
only while DLSS runs.

**Final review** (a fresh Opus at xhigh, 2026-10-30, on the tree with `main` merged; no high finding):
- M1 a failed allocation of DLSS's output stopped DLSS but left the jitter on: the renderer's refusals now go through
  `LatchDlssOff` (its flag and `streamline::Disable()`, so the game thread stops the jitter; the row `native_dlss_latched_off`).
- M2 « only the signed DLLs beside the module run » was overstated: `build-2/sl.log` of `i111-dlss-2` shows « Feature dlss override
  enabled », a model snippet under `C:/ProgramData/NVIDIA/NGX/models/dlss/versions/20318480` (« APP_NAME = nvapp_override ») and
  « Override Reported: ModelPreset, Value: 12 »: the NVIDIA App's DLSS override (Thomas's setting) chose the model preset of the
  comparison pictures. Corrected here and in the comments; whether witness runs keep his override is his decision.
- M3 two planned reset sources were missing: the history restarts whenever the previous world frame was not evaluated (after a
  menu that hid the world, frames without lighting or velocity), besides the cut rule and a new extent.
- L2 unused getters removed (the failure reason goes to the latch row); L5 Streamline's messages limited (the first 32, then one in
  600); L6 the queue drained before Streamline's resources are freed when `Close` skipped its wait; L8 the extents given to
  Streamline taken from the lit HDR itself; L9 the command-list contract said where it holds (every later pass binds its own).
- Kept for later tranches: L1 a neutral upscaler interface over the Streamline host (D-0165), with tranche 3's extents; L3 the full
  copy of the lit HDR each frame (the YEBIS chain reading DLSS's output directly), with tranche 3; L4 DLSS's resources kept after a
  latch until `Close`; L7 the close order, a resize and full screen through the proxy not yet run live (the witnesses end by the
  controller, windowed): the start check after the merge covers the close without the flag only.
- The step review of these answers (a fresh Opus at high effort): no high or medium finding; its two low ones applied (the queue's
  drain in `Close` caught as the wait before it is; warnings and errors of Streamline counted apart, each row carrying its count).
- Optimisation pass (read): the projection words' formula appears in the camera producer and `upscaler::JitterNdc` (the test
  checks they agree); nothing else duplicated.

## Tranche 2: the cadence by the modern loop, then Reflex (2026-10-30, in progress)

The plan is the study thread's [CADENCE-REFLEX-READY](CADENCE-REFLEX-READY.md) (form B of MODERN-CADENCE-STUDY, D-0240; its §8 the
static reading of DS3's limiter: the deadline `[clock+0x28] + [clock+0x18] f`, the three-frame dt). Step 1, the display clock
behind `display-clock.flag`: one wait of the swap chain's frame-latency object a frame on the engine thread, at the end of the
native `EndFrame`, before DS3's limiter; decided in `Clear` before the job is posted and carried by the job (`display_interval`:
the worker then skips the object and presents at that interval at once); engaged only with V-Sync, a waitable chain, a visible
window, a screen fitting 1/60 s steps and DS3's limiter at 1/60 s; nothing written into DS3.

**First form, refuted live.** The first form kept the chain's maximum latency at 2 and waited, after a release, to the display's
grid point nearest DS3's deadline. Step review (a fresh Opus at high effort, `1255f548..f7171f46`): H1 the second object wait of
the study's form could need a present the engine had not made yet (the object counts); M1 the decision published after the job;
M2 the failed-Present debt not moved; L1 to L6. The answers (`81fb5fc5`) and their review (`f7171f46..81fb5fc5`): H-A at latency
2 the object almost never blocks (MODERN-CADENCE-STUDY §5.7), so a count already there was read as a refresh and the frame
lengthened by up to a period; M-A a phase band never corrected on a 60.000 Hz screen; L-A to L-E (the debt's bookkeeping, the
startup `Clear`, the tests, the queue depth). The control runs confirmed H-A before the second review's answer was built
(2026-10-30 19:45-19:53, announced, the ASUS, `settlement-turn` with Beta, enemies without AI, PresentMon and the cadence trace,
measure mode, the PC otherwise idle; build `t2-build-2`, `t2-series-1.log`): `i111-t2-dc-1` and `i111-t2-dc-2` (the flag) waited
16.5-16.7 ms a frame on average after the first 1,800 frames (`native_display_clock`: 554-600 of 600 frames « aligned »), so the
game ran at about 30 frames/s (5,951 and 5,975 frames over the path against 7,551 for `i111-t2-base-1` without the flag). OBSERVED.

**Second form** (written after the runs, `t2-build-3`): with the flag the chain's maximum latency is 1, so the object is released
as the compositor takes the previous present (the display's event) and blocks when the engine is ahead; the engine waits it once
a frame and starts the frame on that release; a count already there returns at once and is counted apart (`stored`, no display
event); no timed wait (DS3's limiter spins whatever is left to its own deadline, the `lead` measured); a failed Present's count
is a debt the next wait skips; only `EndFrame`'s `Clear` decides.

**Second form: its reviews.** The step review of the second form (`81fb5fc5..421c66fc`; the steady state's argument holds: one
wait a frame paired with the previous frame's present, frame N+1 starting on the compositor's taking of present N-1 while the engine
work, the worker and the composition deadline fit two refreshes): H1 a timed-out wait left a spare count for good; M1 two debts;
M2 latency 1 on screens the clock never engages; M3 a count lost at a minimise edge; L1 to L4. Answered in `cd519a11`: a timed-out
wait owes its count, taken first at the next frame by a 0 ms probe; one atomic debt for both waiters; latency 1 only when the
screen fits at start; the first frame after the clock was off only probes; the lead averaged over the frames started on a take.

**Second form, measured: neutral on frames, worse on latency, no lock at a logic locked at 60** (2026-10-30 20:30-20:38,
announced, the same conditions as the first series; build `t2-build-4`, `t2-series-2.log`; read with `Get-I97Cadence.ps1 -Segment
all`): `i111-t2-dc-3` and `i111-t2-dc-4` (the flag) ran 7,555 and 7,516 frames against 7,530 for `i111-t2-base-2`: no frame lost.
But the object found a count already there in 75 to 100 % of the frames over most of the path (`native_display_clock` « stored »
580-600 of 600 a row; « released » frames only in stretches, with DS3's limiter then 8-12 ms ahead of them): the engine never runs
ahead of the display while DS3's limiter holds the logic at 60, so a back-pressure wait never blocks (the study's §5.7, now with
the wait on the engine thread at latency 1 too). PresentMon: holds over 1.5 periods 0 and 4 against 1, no drop in any run; the
latency until displayed p50 30.98 and 18.76 ms against 16.25 ms (the presents now leave as soon as recorded, early in the refresh,
and wait for the next blank or two: the pacer of `main` placed them just before it, phase p50 0.88 ms after the blank against
14.21 and 10.65). OBSERVED: at a locked 60, form B does not take the clock and adds 2.5 to 15 ms of latency; DS3's limiter stays
the clock and `main`'s pacer is the better presentation. INFERRED: form B becomes the clock once the logic asks for more frames
than the display shows (the unlocked frame rate, D-0240's method B: the producer then ahead of the display under V-Sync, the
object blocking each frame), as CADENCE-REFLEX-READY §0 point 2 and the study's §6.7 (« survives V2 unchanged ») say.

## Tranche 4: the reactive mask and the mode in the options (2026-10-30, in progress)

The reactive mask (step 1) was removed by [I-0116](../I-0116-dlss-complements/RESULT.md) tranche 2 (2026-11-01): DLSS 310.9's
models other than the deprecated preset F do not read the current-colour bias, and a run with the mask forced to 1 changed
nothing (series 5); the forward rows' DLSS-frame reordering went with it.

Opened by the RT pilot's thread of 2026-10-30 17:05 (branch `claude/exciting-visvesvaraya-0537da`), after Thomas kept D-0240's order
(« On garde l'ordre prévu »): this tranche, then the cadence with Reflex (tranche 2), the unlocked frame rate, then R2.

**The reactive mask, from the colours rather than a stencil bit.** UPSCALER-DLSS-READY §3.4 planned a stencil bit 0x40 written by
every blended class (a stencil variant of the forward, water, veil and effect pipelines) and resolved to R8. Written instead (step 1,
on a DLSS frame only): the lit HDR copied once the opaque rows are drawn (`CopyReactiveBase`), and before DLSS a full-screen draw
writes an R8 mask at the render extent from the largest channel difference between that copy and the final lit HDR, both through
the reversible tone map `c / (1 + max(c))`, scaled by 1, nothing under 0.02, at most 0.9 (FSR 2's « auto-reactive » rule, GENERAL;
the numbers INFERRED, to tune on sight, row UL11). A frame whose forward block did not run (no forward row, water or effect) clears
the mask; a frame with only 0x1F rows draws a mask of zeros. DLSS reads it as its current-colour bias
(`kBufferTypeBiasCurrentColorHint`, « lerp(history, current, bias) » in `sl_core_types.h`, the « DLSS.Input.Bias » input of
`sl.dlss.dll`, read in the DLL's strings); without a mask the tag is left out. Why the colours: no pipeline of the forward rows, the
water or the effects changes (they stay as the open-gaps pilot has them), the mask covers every blended family whatever writes it (a
veil, a fog card, an effect model particle), and nothing reads a stencil bit whose other writers are not all known (UL10). Its cost
on a DLSS frame: one copy of the lit HDR and one R8 full-screen draw at the render extent. Off without DLSS (no copy, no draw, the
forward rows in their order: the default frame unchanged); a failure to make its resources leaves DLSS without it for the session
(`native_dlss_reactive_off`). The frame graph declares `reactive_base` and `reactive_mask` (plan key bit 42).

**What is opaque for the mask** (after the step review's M1 and the final review's M1): the 0x1F rows; the water (WatDpt and the
surface, which writes depth: drawn before the base, so outside the mask, its look under DLSS judged in play, row UL11); then, at
the head of the rows after the water's copies (their own place in the original order 0x1F, 0x24, 0x27, 0x21, 0x22, 0x29), the rows
that are opaque and write depth (the phantom rows 0x21/0x22), in their order, up to the first row that blends and writes depth (a
fading 0x29 row), and only when every crossed row tests depth LESS or LESS_EQUAL and writes none, and the moved row tests the
same way, writes all its colour channels and every target a crossed row writes (GB5's glow); the scan stops at any other row.
Under those conditions, moving an opaque depth writer ahead of a row changes only pixels where, in the original order, it hid that
row from behind. Rows are ordered by pass, so 0x21/0x22 already come before 0x29: what may cross another row is mostly a 0x29
effect-model row that takes its effect bank's depth with writes on and no blend. The other rows stay after the base: the opaque ones that write no depth (an opaque 0x29
material, not fading) are marked reactive (their order against the blended rows matters), counted with the rows moved in
`native_dlss_reactive_rows`; a 0x1F row of an effect model drawn additive is before the base, so outside the mask (kept, recorded).

**Witnessed, first form** (2026-10-30 17:35-17:43, announced, the ASUS, the game windowed at 1280x720, the replayed Firelink path
with Eta, debug layer on, no measure; build `t4-build-2`, series log `t4-series-1.log`): `i111-t4-a-1` (DLAA, 12,320 frames) and
`i111-t4-q-1` (Quality, 853x480, 12,405 frames) clean: DLSS evaluated every frame (`native_dlss_frames` to 7,200, no latch, no
`native_dlss_reactive_off`), the debug layer silent, the graph shadow without a mismatch, the restoration held. The lossless masks
(`world-<n>-reactive.r8` beside each witness frame's lit HDR, eight a run from frame 1,500 to 3,600) mark 0.6 to 0.8 % of the
pixels (0.34 % and 0.03 % at frame 1,500, the path's start), 0.07 % at most above one half, the largest 127 to 226 of 255: OBSERVED on
`evidence/I-0111/artifacts/t4-mask-dlaa-{1800,3000}.png` (the frame, then the mask in red over it darkened), the bonfire's flames
and nothing of the opaque scene. Then `i111-t4-a-2` (build `t4-build-3`, the step review's answer, 18:06-18:10, DLAA, 12,301
frames, `t4-series-2.log`): clean likewise. Coverage (final review L2): at Firelink almost every frame takes the water branch (its
water rows), the branch without water ran once a run, and no phantom row was drawn: the move itself is not yet witnessed.

**Witnessed, the final review's answer** (build `t4-build-4`, 18:48-18:55, the same conditions, `t4-series-3.log`): `i111-t4-a-3`
(Firelink, 12,255 frames, the water branch) and `i111-t4-ad-3` (Archdragon Peak's path with Eta, 5,836 frames, no water: the branch
without water) clean, DLSS every frame, the debug layer silent, the graph shadow without a mismatch (21,165 and 4,036 frames),
`native_dlss_reactive_rows` 0 rows moved and 0 opaque rows marked in both (no phantom on either path: the move is still not
witnessed). The move's conditions added after it (`t4-build-5`) only narrow which rows move.

**Step review** (a fresh Opus at high effort, on `9ffa23a1..43dfbf0a`; no high finding; the default frame, the transitions against
the frame graph, the lifetimes, the tag without a mask and the absence of dead code hold; the 0x1F rows are at the front of the
forward list under the chunked preparation too, `10_options_and_passes.inc:448-482`): M1 the opaque rows after the 0x1F ones were
marked (above); L1 a comment said a failure latched DLSS off (it leaves DLSS without the mask: corrected); L2 a frame with only 0x1F
rows copies and draws a mask of zeros (the comment corrected; keying it on the rows would be outside the plan key).

**Final review** (a fresh Opus at xhigh, on the tree with `main` merged, `f538fa6a`; no high finding; criteria 1 to 5 hold): M1 the
moved rows were drawn ahead of the water's copies and surface in the water branch (a phantom's legs in the refraction copy and the
readable depth): the water branch now moves them after the water's copies, at their own place, and the water stays outside the
mask; the move also stops at the first row that blends and writes depth; L1 a row with depth off and the write mask on counted as a
depth writer (now both required); L2 the coverage said above, and the moved rows counted in the row; L3 the mask figures corrected
(above); L4 the water was said to be marked only for its waves: it is now outside the mask; L5 the contract's line updated; L6 one
encode function for the forward rows, the moved rows' flags kept in a member; nits: the counters the render thread's own, the
header's comment. The step review of these answers (a fresh Opus at high effort, `f538fa6a..ff82be8c`; no high or medium finding;
the default frame, the graph's passes, the break rule and the rest unchanged hold): L-a a moved row writing the HDR only could hide
the glow a crossed row wrote into GB5, L-b the claim assumed LESS_EQUAL tests and full colour writes the code did not check: both
now conditions of the move (above); L-c, L-d two stale comments corrected.

**Step 2, the mode in the game's options** (Thomas, 2026-10-30 23:41: « Oui dans l'option anti-aliasing, de toute façon il y aura
une refonte des menus »). Streamline is loaded at every start (its signed DLLs beside the module), unless `no-dlss.flag`; each
frame's DLSS is the game's own Antialiasing option: `FollowNativeDlssOption` (in `BeginFrame`, the engine thread) reads the quality
object's AA byte `[[[0x144961B50]+0x5360]+8]` (AA-NATIVE-READY's gate) and sets `native_dlss_active`; ON: DLSS (DS3's own filter
runs only where DLSS cannot: no NVIDIA card, a refusal, a latch, `no-dlss.flag`), OFF: no anti-aliasing. The frame's decision is
taken once at its collect (`NativeSelectedScene::dlss`) and carried to the prepare on the render worker and to the renderer
(`NativeModelFrameInput::dlss`, `dlss_frame`); the jitter, the render extent (a scaled mode returns to the output when the option
goes off, through the quiesced path), the velocity and the mip bias follow the same value on the engine thread (INFERRED from the
code: the option changed in the menu takes effect at the next frame; no live toggle run yet). The mode: `dlss-mode.txt` beside the
module (`dlss_mode.h`, unit-tested: one whole word, `dlaa`, `quality`, `balanced`, `performance` or `ultra-performance`, any case,
the UTF-8 and UTF-16 marks ignored; anything else keeps DLAA), the test flags first, NVIDIA's DLAA by default; read at start and
named in one row `native_dlss_mode` (mode, source, text). Witnessed (2026-10-31 00:03-00:18, announced, the ASUS, the replayed Firelink path
with Eta, debug layer on, no measure; build `t4b-build-1` of the RT branch, before the final review's answers; `t4b-series-1.log`):
`i111-t4b-on` (windowed 1280x720, the option on) DLSS on every world frame (`native_dlss_frames` to 7,200 in a run of 12,224 frames
with its title and loading); `i111-t4b-fs` (exclusive full screen 1920x1080 through
Streamline's proxy chain, the first such run) DLSS on the whole path (21,815 frames), the graph shadow without a mismatch, no
debug-layer message; the runner could not close a full-screen game (it clicks a window's close box) and ended it at its budget,
the restoration held; `i111-t4b-off` (`Antialiasing=OFF`) the option read off after the load (`native_dlss_option` false, aa
byte 0), no DLSS frame, DS3's own filter off as the option asks. Not witnessed: a live toggle in the menu, a scaled mode chosen by
the file, the option on without DLSS.

**Final review** (a fresh Opus at xhigh, on `main...claude/r1-t4-aa-option`, the step alone on `main`; no high finding): M1 the
flag written by the engine thread was read by the prepare on the render worker (a race at a toggle: a blank or unresolved frame):
the decision now travels with the frame's scene, the flag atomic; M2 the RESULT and STATE out of step with the branch (committed
with the merge); M3 DLSS on by default before Thomas's acceptance in play: asked to Thomas, who answered (2026-10-31 00:45, relayed): « Et pour ta question dessus, oui, fusionne comme ça » (merged on by default, the option ON: his play its acceptance; the open-gaps pilot told that a comparison against the original passes `no-dlss`); L1 stale comments (`01_state.inc`,
`streamline_host.h`, `scene_pass.h`, `20_startup_phases.inc`, the build script, WORKFLOW « Tools »: rewritten); L2 the mode file's
parsing (`dlss_mode.h` with `dlss_mode_test`, the row naming its source); L3 the option's row read the byte only when DLSS was ready
(now both); L4 the witnesses' coverage (said above); L5 D-0242 cites the RT branch's section; L6 the « no anti-aliasing » wording
(corrected above). The step review of these answers (a fresh Opus at high effort, `2312b6fb..d0a8e9fb`; no high or medium finding;
the order within a frame holds: `BeginFrame` sets the option, the camera's jitter, then the collect's decision, the prepare and the
renderer read the scene's value; the AA filter runs when the option is on and DLSS cannot): L-a the engine thread's reads re-read
Streamline's readiness mid-frame (a latch between the jitter and the collect): they now take only the `BeginFrame` value; L-b the
option's row only on a change of the decision: now at the first frame and at each change of the byte, its read or Streamline's
state; L-c a comment; L-d the parser accepted a second word: refused now, tested; L-e kept (the marks dropped anywhere; no UTF-16
big-endian test).

**The motion of the effects, the water and the forward rows.** Unchanged: the effects and the blended rows write no motion vector
(the camera's motion comes from the depth under them); the mask is what keeps them from ghosting. The water surface writes depth, so
it moves with the camera; the motion of its waves is not in the vectors nor in the mask (row UL11, in play).

## Evidence

- Builds `evidence/I-0111/artifacts/build-1` and `build-2` (unit tests passed: `upscaler_test` 18 checks, the resolve within
  0.004 px of the unjittered motion, the rows within 1e-10 px).
- Witness runs (announced, the ASUS at 1920x1080 60 Hz, the game windowed at the save's 1280x720, the replayed Firelink path with
  Eta, debug layer on, no measure; `evidence/I-0066/artifacts/<run>`, `evidence/I-0111/artifacts/witness-{1,2}.log`):
  - `i111-dlss-1` (build-1, `dlss.flag`, 06:37): clean (no fault, restoration held), but DLSS refused at start:
    `slGetFeatureFunction(slDLSSSetOptions)` asked right after `slInit` answers `not_initialised` (Streamline: « 'kFeatureDLSS' has
    not been initialized yet »); the refusal path left the frame as before (no jitter). OBSERVED: the interposer loads and
    `slInit` succeeds inside DS3's process (H1, first half).
  - `i111-plain-1` (build-1, no flag, 06:41): clean; the comparison pictures' left half.
  - `i111-dlss-2` (build-2, `dlss.flag`, 07:10): `native_streamline` « ready », `native_dlss_frames` 1 at serial 1405, then 3,600
    and 7,200 (no latch, no `native_dlss_unresolved` row), the debug layer silent, no fault, restoration held. OBSERVED: Streamline
    in manual hooking with the native device and the proxied chain runs DLSS inside DS3 (H1).
- Pictures `evidence/I-0111/artifacts/i111-pair-{start,end}.png` and `i111-zoom-{start,end}.png` (left without DLSS, right with):
  edges smoothed; the dark areas' flat bands with red and blue blotches of the left half are intermittent from run to run, DLSS or
  not (tranche 3 above). The jitter's and the vectors' signs (H2) need a look in motion: not yet seen.
- Tranche 3: builds `evidence/I-0111/artifacts/t3-build-{1..6}` (unit tests passed each time, 108 PASS rows), the series logs
  `t3-series-{1..5}.log`, the pictures `t3-zoom-dlaa-vs-{q,b,p}.png`, `t3-end-dlaa-vs-{q,b,p}.png`, `t3-zoom-p2-vs-b2.png`; the runs
  `evidence/I-0066/artifacts/i111-{q,p,b}-1`, `i111-{p,a,q,b}-2`, `i111-p-3`, `i111-{q,u}-4`, `i111-r{a,q}-5` (the `-2` and `-3`
  Performance and DLAA runs with `world-pictures`: the lit HDR, GB5, mask and back-buffer dumps).

## Closed (2026-11-09)

Closed 2026-11-09 with Thomas's yes (relayed by the coordinator, 01:55 Paris): the open points below are carried to [I-0132 R11-PLAN §6](../I-0132-rt-optimisation-zones/R11-PLAN.md#6-the-open-points-carried-from-the-investigations-closed-on-2026-11-09), where each has its place.

## Uncertainty / unresolved questions

- The jitter's and the motion vectors' sign conventions as DLSS reads them (UPSCALER-DLSS-READY UL3, UL4): settled live.
- Whether Streamline's interposer, loaded inside DS3, resolves `dxgi.dll` to the game folder's proxy (H1).

## Artifacts produced or modified

- `native/renderer/streamline_host.{h,cpp}`, `native/renderer/upscaler.h`, `native/frame/upscaler_test.cpp`; changes in the camera
  producer, `scene_view`, the velocity collect, the backend's presenter, the scene renderer's start, close and world frame; the
  build script; `scripts/New-CompileCommands.ps1`; `Run-StartupTrace.ps1` (`dlss` flag).
- Tranche 3: `native/renderer/scene_pass{.h,.cpp,_frame.cpp,_velocity.cpp,_decals.cpp}`, `streamline_host.{h,cpp}`,
  `native/world/light_producers.{h,cpp}` (the AO bounds), `native/frame/game_bootstrap/{01,02,05,10,12,13b,15,21}*.inc`;
  `Run-StartupTrace.ps1` (the mode flags and `test-dlss-refuse`).
- Evidence: `evidence/I-0111/artifacts/build-{1,2,3}`, `witness-{1,2}.log`, `t3-*`, the comparison pictures; the runs under
  `evidence/I-0066/artifacts/i111-*`.
