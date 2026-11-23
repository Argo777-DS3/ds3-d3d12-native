# I-0111 - DLSS et Reflex (R1)

Capability: C3 · [RT-PT-PLAN](../../.project/RT-PT-PLAN.md) §5 and §8, workstream R1 (D-0235, D-0237) · the RT pilot, opened
2026-10-30 after R0 ([I-0109](../I-0109-modern-dx12-base/RESULT.md)) merged.

## Question

What does the native module need so that NVIDIA DLSS (Super Resolution and NVIDIA's DLAA) and Reflex run inside Dark Souls III
through Streamline 2.14.16 in manual-hooking mode (D-0165), with DS3's own data (projection, depth, motion vectors, lit HDR), and
can it be put in place without changing the default frame?

## Scope

Included, in tranches, each merged once written and reviewed (D-0227):

1. **NVIDIA DLAA through Streamline** (render size = output size, no extent split): the Streamline host (signed DLLs loaded from
   the module's folder, `slInit` with manual hooking, `slSetD3DDevice`, the factory upgraded before the swap chain so its
   `Present` reaches `presentCommon`, `slShutdown` before the device); the camera jitter (Halton (2, 3), one injection point in
   the scene camera producer; the velocity matrices kept unjittered: the object rows' current-frame shift on both matrices, the
   resolve's current-to-previous matrix folding the jitter, UPSCALER-DLSS-READY §3.2-3.3); the velocity pass and its object rows
   forced on while DLSS runs; the evaluation between the velocity (slot 12) and the YEBIS passes on the lit HDR; the history reset
   (the velocity cut rule, the first world frame, a mode change); DS3's own anti-alias filter (FXAA / DS3's « DLAA », not
   NVIDIA's) off while DLSS runs. Opt-in by `dlss.flag` while it is new.
2. **Reflex** (Off, On, On + Boost) and the PC latency markers on the game and render threads; the exposure input (the tone
   map's exposure, or auto-exposure).
3. **The scaled modes** (Quality, Balanced, Performance, Ultra Performance): a render extent distinct from the output extent
   (every size-dependent pass and constant, D-0151), the render size from `slDLSSGetOptimalSettings`, the material samplers' mip
   bias, a depth at the output size for the YEBIS stages that read one.
4. **The reactive mask** (planned as a stencil bit 0x40 from the blended draws, resolved to `R8`; written from the lit HDR's
   colours before and after the blended rows, RESULT tranche 4) and the motion of the effects, the water and
   the forward rows; the mode in the game's options; DLSS on by default once Thomas accepts it in play.

Excluded:

- Ray Reconstruction (R5 and after, once R3 has the roughness), frame generation (R10), Mega Geometry (D-0238).
- The effects pilot's files (`effect_stage.cpp`, `native/effects/`, the FXR runners) while I-0105 runs, beyond the jitter their
  matrices may need (said in the RESULT if so).
- Anything installed in the game folder: the Streamline DLLs travel with the module's build folder.

## Inputs

- The DLSS sheet: [UPSCALER-DLSS-READY](../I-0066-independent-native-startup/UPSCALER-DLSS-READY.md) (2026-10-11; its line
  references predate the split of `game_bootstrap.cpp` into `game_bootstrap/*.inc`, its « direct NGX » proposal replaced by D-0165's
  Streamline manual mode).
- Streamline 2.14.16: `C:/Users/User/Tools/DS3-Decompilation/third-party/streamline-2.14.16` (`include/`, `bin/x64/`, `docs/
  ProgrammingGuide*.md`), its x64 DLLs signed by NVIDIA (WORKFLOW « Tools »).
- The native code: `native/renderer/backend.cpp` (device, factory, swap chain), `scene_pass.cpp` (`PresentPaced`),
  `scene_pass_frame.cpp` (the frame's passes), `scene_pass_velocity.cpp`, `velocity.h`, `scene_view.cpp`, `world/producers.cpp`
  (`ProduceSceneCamera`), `frame/scene_ingress.cpp`, `frame/game_bootstrap/12_scene_collect.inc` (`CollectNativeVelocityInputs`).

## Hypotheses

- H1: Streamline's manual-hooking mode runs inside DS3's process with the native device and a proxied swap chain only (no device
  or command-list proxy); the game folder's DXGI proxy does not intercept it.
- H2: DS3's motion vectors (previous minus current, render pixels, unjittered) are DLSS's convention with `mvecScale = (1/W, 1/H)`;
  the jitter's sign is DLSS's when the projection is shifted by `+2 jx / W, -2 jy / H` (UPSCALER-DLSS-READY rows UL3, UL4: checked
  live, a sign flipped if the picture says so).
- H3: DLAA changes nothing in the default frame when `dlss.flag` is absent (no Streamline DLL loaded, the projection words
  `-0.0` as today).

## Expected evidence

The build with its unit tests (the jitter words, the reprojection algebra, the Halton table); the start row naming Streamline's
state and DLSS's support; a short announced witness run with `dlss.flag` (Firelink, still camera and a pan: no crawl, no ghost,
the debug layer clean); the one-minute start check after each merge (D-0229).

## Exit conditions

- Success: the four tranches merged, DLSS (DLAA and the scaled modes) and Reflex running inside DS3 on Thomas's PC, reviewed (step
  reviews, final review at xhigh), DLSS on by default once Thomas has accepted it in play.
- Refuted: Streamline cannot run inside DS3's process without a device proxy or a file in the game folder: stop, record, and ask
  Thomas (direct NGX and NVAPI would need their SDKs).
- Insufficient information: a sign or a state Streamline does not document and the picture cannot settle.
- Stagnation: two failed local fixes of the same defect open an RCA.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`.

## Safety and human interaction

Live runs only under WORKFLOW « Live game rule » (announced, offline, hash-checked restore, the ASUS alone when Thomas is away);
the RT pilot has priority on the ASUS and the launcher rebuilds unless the open-gaps pilot has a fix ready. Nothing installed in
the game folder. Installing a program on Thomas's PC or changing an NVIDIA setting stays his decision.
