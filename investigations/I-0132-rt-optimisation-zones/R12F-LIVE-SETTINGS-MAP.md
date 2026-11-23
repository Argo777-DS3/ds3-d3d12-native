# R12f: where each setting is read today (a map for the live settings)

Written 2026-11-11 (Paris) by the thread that first opened R12f, before Thomas moved it to E (09:25: « il devient E, relance un fil
pour reprendre R »). It hands R12f to the fresh R thread: no live-settings code was written on this branch. The table below is a
**Sonnet worker's static map** (an Explore agent over `native/`, file:line from `main` `d6c41c06`), not re-read by a pilot: non-canonical
until checked (WORKFLOW « Subagents »). Abbreviations: `01_` = `native/frame/game_bootstrap/01_state.inc`, `02_` = `02_frame_lifecycle`,
`02b_` = `02b_rekindled_graphics.inc`, `15_` = `15_cpu_service_and_input.inc`, `20_` = startup phases, `22_` = `22_setup.inc`.

## Common facts

- Every start reader (`NativeTaaRequested`, `NativeSharpen`, `NativeRrRequested`, `NativeRtPtRequested`, `NativeRtBouncesRequested`,
  `NativeDlssMode`/`Chosen`, `NativeReflexMode`, `NativeFrameGenerationMultiplier`, `NativeRequestedRate`) is a function-local
  `static const`, fixed at its first call; test flags first. `NativeSettings()` (01_:765) is the file read once.
- The live copy `NativeSettingsCurrent()` / `EditNativeSettings` (02b_:20-40) is read only by the page's rows today.
- `StepRekindledGraphics()` runs first in `BeginFrame` on the engine thread, before `FollowNativeDlssOption` (15_:906),
  `FollowNativeReflex` (15_:927), `FollowNativeRenderExtent` (02_:650) and the display followers: the place to apply a change.
- `WithPresentationIdle` (02_:613) quiesces the render worker around a presentation or extent change (used at 02_:632, 669, 714, 739, 774).

## Per setting

| Setting | Read today | Already per frame | What is fixed at start |
|---|---|---|---|
| level (rt/pt) | `NativeRtPtRequested` once at 20_:229 (inside `rt_usable`), stored in `renderer.rt_pt` (atomic, scene_pass.h:333) | the renderer reads `rt_pt` each frame (scene_pass_lighting.cpp:656, 721; scene_pass_frame.cpp:1241, 1494, 1511) | nothing sized by it; PT needs RR ready |
| bounces | `NativeRtBouncesRequested` once at 20_:229 → `renderer.rt_bounces` (atomic) | read each frame (scene_pass_lighting.cpp:659, scene_pass_frame.cpp:1495) | nothing |
| dlss mode | `NativeDlssMode`/`Chosen` (01_:818, 834, static) | `FollowNativeDlssOption` sets `native_dlss_active` each frame; `FollowNativeRenderExtent` re-keys the extent on mode and RR (02_:653-669); Streamline re-sends `slDLSSSetOptions`/`slDLSSDSetOptions` on a mode, size or RR change (streamline_host.cpp:278-307) | DLSS, RR, Reflex, PCL loaded at Streamline's start whatever the mode; 12_scene_collect:168 reads `NativeSettings().taa` and `NativeDlssChosen()` (DS3's own AA filter) |
| rr | `NativeRrRequested` once at 20_:249 → `renderer.rr`, `native_rr_session` | `native_rr_active` each frame (15_:~921); the renderer reads `rr` (scene_pass_frame.cpp:933, 1420) | `LatchRrOff` (scene_pass_frame.cpp:1604) one-way with `RrDisable()` |
| reflex | `NativeReflexMode` (01_:838) from `FollowNativeReflex` each frame | `SetReflexMode` re-sends only on a change; forced On while frame generation runs | nothing |
| frame_generation | `NativeFrameGenerationMultiplier` at 20_:135 into `SetUpscaler` | `PresentPaced` calls `SetFrameGeneration(on)` each present (scene_pass.cpp:185-193); the multiplier is an atomic re-read at each off→on (streamline_host.cpp:408-429) | DLSS-G loaded only when requested at start (scene_pass.cpp:285-292, streamline_host.cpp:142 `fg_requested`, `slSetTagForFrame`); `frame_generation_` and `presenter_.waitable=false` decided at `Start`; `SetUpscaler`/`Start` throw after start (scene_pass.h:175); latch `fg_failed_`/`FgDisable` |
| taa | `NativeTaaRequested` (01_:778, static) inside `FollowNativeDlssOption` each frame | `native_taa_active` each frame; jitter (01_:858), mip bias (10_options_and_passes:61-62), `scene->taa` (13b:1506) | latch `taa_failed_` (scene_pass_frame.cpp:88); 12_scene_collect:168's static |
| sharpen | `NativeSharpen` (01_:781, static) into `frame->sharpen` each prepare (13b:1506) | the TAA and the DLSS sharpen read `input.sharpen` each frame (scene_pass_frame.cpp:966, 1003) | latch `dlss_sharpen_failed_` |
| frame_rate | `NativeRequestedRate` (15_:381, static) written into the clock each `BeginFrame` (+0x354, +0x358) | the value written each frame | `SetPresentUnpaced(rate > 0)` at 20_:97 (throws after open); the I-0114 corrections hooked at 22_:801 only when the start rate is above 0 |
| vsync | `NativeSettings().vsync` once at 20_:96 | the present interval chosen each present (scene_pass.cpp:177, 179) | `SetPresentSync` (frame_pacer.h:29, a plain bool) and `SetPresentTearing` throw after start; `DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING` only at the chain's creation (backend.cpp:899-904, kept by ResizeBuffers at 935) |
| borderless | `NativeBorderlessOption` (02_:687, static) at start | the page's Screen Mode row already switches it live (02b_:133-155 → `ApplyNativeDisplayRequest`, `renderer.ApplyDisplayMode` under `WithPresentationIdle`) | the launcher's `borderless.flag` |

## The hard ones (the worker's reading)

1. Frame generation off → on when it was not loaded at start: DLSS-G, the chain without its waitable object and the pacer are decided
   at `Start` (R12-PLAN's « DLSS-G loaded at start »).
2. V-sync off: the chain needs `ALLOW_TEARING` from its creation, and the pacer's sync settable after start.
3. The frame rate: the corrections' hooks (22_:801) and `present_unpaced_` depend on the start rate. Since I-0113 tranche 36 (gap 27,
   E's branch `claude/eager-morse-14500d`), the corrections' k is `logic_rate::FrameK(dt)`: 1 at and under 60 frames/s, so hooks
   installed at every start run the original's call whenever DS3's mean frame time is not shorter than 1/60 s (within 1e-4); its frames
   a little shorter at the 60 cap would still be corrected slightly, so a live rate keeps an explicit gate (the requested rate 0: k
   held at 1).
4. The statics at 12_scene_collect:168 and 01_:778; the one-way latches if a re-enable after a failure is wanted.
