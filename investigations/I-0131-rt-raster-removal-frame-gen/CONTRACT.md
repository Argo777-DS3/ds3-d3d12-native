# I-0131 - Le retrait du raster et la génération d'images

Capability: C3 (RT-PT-PLAN §1, §5 and §8 R10; D-0235, D-0237, D-0249, D-0251, D-0253, D-0258)

## Question

Can the native renderer drop every raster lighting path that has a traced equivalent (the sun's cascades, the local shadow atlas,
the environment cubemaps' pass, DS3's local-light batches on the world view), leaving one fallback without visibility for the frames
the rays cannot serve, and add DLSS frame generation through Streamline's proxy chain, without a fault and without changing the
traced frames' picture?

## Scope

Included:

- R10a: the removal (R10-PLAN §2), the single fallback, the flags that selected the removed paths, the SSAO's leftovers.
- R10b: DLSS-G (R10-PLAN §3): loading, pacing by Reflex, the depth, motion and HUD-less tags, on and off per frame, its setting file.
- Diagnostics rows, unit tests of the pure parts (the setting's parser).

Excluded:

- The menu views' lighting (a studio set outside the rays' scene, R10-PLAN §1 row 9), the forward light grid, the effects' look.
- DS3's directional programme for the sun and the fill lights (shaded by DS3 with the traced visibility; the fill lights stay,
  D-0257; R9d's verdict is Thomas's).
- The settings menu and the title screen (R12, D-0258); the optimisation and the zones' tour (R11).
- The UI alpha for DLSS-G (multi frame generation, x3 and x4 for the RTX 50, added at Thomas's request, 2026-11-08 18:06).

## Inputs

- R10-PLAN (this folder), its inventory; RT-PT-PLAN §1 and §5; I-0118, I-0122, I-0125, I-0126, I-0129 RESULTs.
- Streamline 2.14.16 (`Tools/DS3-Decompilation/third-party/streamline-2.14.16`): `ProgrammingGuideDLSS_G.md`, `ProgrammingGuide.md`,
  `ProgrammingGuideManualHooking.md`, `sl_dlss_g.h`.
- Tools: clangd, `Build-GameBootstrap.ps1 -Out`, `Run-StartupTrace.ps1` and its runners, PresentMon 2.6.0.

## Hypotheses

- H1: every frame the rays cannot serve can be lit by the native passes without visibility (as the 1 to 29 skipped walks already
  are), so no shadow map or cubemap is needed by any frame.
- H2: DLSS-G attaches to the chain the module already makes through Streamline's proxy factory, with Reflex's sleep as the frame
  start, without a change to the device or the command lists.

## Expected evidence

- R10a: the traced frames' journals unchanged (the passes but the removed ones, `graph_barrier_mismatch` 0), their pictures
  equal by mean luma at the witness places; `no-rt-scene` runs without a fault, no cascade or atlas pass, the sun unshadowed.
- R10b: `native_frame_generation` rows with two frames presented per frame, PresentMon's generated frames, no fault across a resize
  and a screen-mode change; with it off the run as before.

## Exit conditions

- Success: both tranches merged, reviewed, the start check passed.
- Refuted: H1 refuted (a frame kind needs a shadow map) or H2 (DLSS-G refuses the proxy chain): recorded, the tranche replanned.
- Insufficient information: the OLED's value of frame generation waits for its screen to be on.
- Stagnation: two failed local fixes on one fault open an RCA.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`.

## Safety and human interaction

Live runs on the ASUS under D-0116/D-0117, each slot asked of the coordinator; offline, saves restored by hash. Thomas's judgement:
the frame generation's look on the OLED; the R9d choice (relayed).
