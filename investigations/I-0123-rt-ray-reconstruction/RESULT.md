# I-0123 - DS3 DX12 Vague 2: R5's other half, Ray Reconstruction and the traced shadows' penumbrae (La reconstruction des rayons): result

## Conclusion

Complete (opened 2026-11-03 by the R pilot on branch `claude/intelligent-villani-341aa7`, after I-0122 R4b; plan [RR-PLAN](RR-PLAN.md)); merged
`4a1ac742`, start check `start-4a1ac742` passed (2026-11-03 21:08).
RR-a written, reviewed (step review) and run at four zones: Ray Reconstruction runs on every traced DLSS frame, never latched off,
and its picture is close to DLSS Super Resolution's where nothing is noisy (H1 not contradicted by the pilot's eye; Thomas's verdict
pending). RR-b
(the penumbrae) written and run: no grain visible, the penumbrae subtle at the radii chosen (a look for Thomas).

## RR-a, Ray Reconstruction on the traced frames (2026-11-03)

- **What it does** (OBSERVED in the code): RR-PLAN §1 and §2. Streamline's `sl.dlss_d` loaded with DLSS (its DLLs `sl.dlss_d.dll`,
  `nvngx_dlssd.dll` copied by the build); on a main-view DLSS frame with the rays on (`rt-scene.flag`) and the player's option
  (`rr-mode.txt` not « off », no `no-rr.flag`; D-0247), the lighting draws the guides after the composite (`rr_guides.hlsl`: the
  diffuse albedo GB2, the specular albedo EnvBRDFApprox2(GB3², r², N·V), the world normal GB0 with r = (2 / (2^(13 g) + 2))^¼ in w; the
  sky's values) and copies the lit HDR as the colour before transparency; the evaluation tags them with `kFeatureDLSS_RR` in place of
  DLSS (no exposure: RR reads none), RR's options every RR frame (packed roughness, the camera's world-to-view rows), RR's own optimal
  render extent in a scaled mode, 32 jitter phases at least. An RR refusal latches only RR off; a feature change restarts the history.
- **Unit test** (`rt_scene_test.cpp` RrGuidePass, build-rra2): the three guides against the formulas computed in the test, GB0 in the
  production format (R16G16B16A16_FLOAT), the sky pixel: worst relative error 1.9 % (R11G11B10).
- **Runs** `rr-a-*` (build-rra, `b18398b7`, 2026-11-03 18:56-19:13 Paris, the ASUS `\\.\DISPLAY1` alone, the OLED off, offline,
  restored by hash; [Run-RrZoneTour.ps1](Run-RrZoneTour.ps1), rt-scene; 14 runs, no fault): each zone twice, RR and `no-rr`: Firelink at
  1280x720, 1920x1080 and 1531x861 (clamped by the screen to 1456x849: the visible game 1440x810), Firelink at 1920x1080 in DLSS Quality
  (render 1280x720), the High Wall, Anor Londo and Ariandel at 1280x720. RR evaluated on every RR run (`native_rr` ready, its first
  `native_dlss_frames` row with `rr_frames` 1), no `native_rr_latched_off` row; no `native_local_shadow_draws` row in any run (the local
  atlas not made, I-0122).
- **Pictures** (`evidence/I-0123/artifacts/pictures-rr-a`, the game area of `walk-1` and `walk-2`; `walk-3` was taken after the game
  closed, the Claude app in it: not used). FLIP mean RR against `no-rr` (flip.exe, LDR): Firelink 0.037 to 0.051, DLSS Quality 0.053 and
  0.064, the High Wall 0.081 and 0.085, Anor Londo 0.051 and 0.046, Ariandel 0.026 and 0.021. Two launches each: the fire, the fog and
  the particles differ between them, and no same-configuration pair was run to give the floor, so these numbers do not measure RR alone
  (insufficient information for a number; the pictures carry the judgement). Seen (the pilot's eye, zoomed x3): no smearing; the High
  Wall's stones and stairs a little sharper with RR; in DLSS Quality, Firelink's wall carving a little softer with RR; an orange
  streak across the hair, first taken for a guide defect, is the bonfire's embers (the same streaks on the `no-rr` side at 1920x1080;
  the embers differ between launches). Shown to Thomas 2026-11-03.
- **Step review** (fresh Opus, high, `062adc54..b18398b7`): nothing high. Answered (`16e9958d`): M1 the shadowed lights vanished on a
  traced frame without a light list (a light whose record cannot be made now gets an inert record, counted `rt_inert`, instead of
  dropping the list; a traced frame without a list keeps an empty atlas, the batches then lighting the shadowed lights unshadowed); L1 a
  switch between RR and DLSS restarts the history; L2 a guide failure evaluates DLSS that frame; L4 RR's options every RR frame (the
  camera's rows); L5 RR's state taken with DLSS's at the frame's start; L6 the atlas kept with `rt-lights-check.flag`; L7 the test's GB0
  format; L8 RR-PLAN. Watched in the runs, not seen: L3 (a non-optimal render extent refused by NGX).

## RR-b, the penumbrae (2026-11-03)

- **What it does** (OBSERVED in the code): on a frame RR denoises (`rr_now_`), the traced sun's ray goes toward a direction uniform in
  the cone of 1° half-angle around the sun (`kRtSunAngularRadius`; the physical sun 0.27°, the original's blurred cascades wider:
  INFERRED, a look) and a shadowed local light's ray toward a point uniform on the disc of radius min(5 % of its range, 0.25 m) facing
  the surface (`kRtLightRadiusShare`; DS3's lights are points: INFERRED); one sample a pixel a frame, white noise from PCG's 3D hash of
  the pixel, the frame's serial and a salt a light (RR's guide §3.5). A frame without RR keeps the sharp ray.
- **Unit tests** (build-rrb): the sharp path's tests pass unchanged (SunShadow, LocalLights); the soft sampling itself has no unit case
  (the runs show it).
- **Runs** `rr-b-*` (build-rrb, `17522c39`, 2026-11-03 19:30-19:37 Paris, the ASUS alone, offline, restored by hash; Firelink at three
  sizes, the High Wall, Anor Londo, Ariandel; 6 runs, no fault, RR on every run, no latch). Pictures against the sharp `rr-a` runs
  (`evidence/I-0123/artifacts/pictures-rr-b`): no grain visible anywhere (RR denoises one ray a pixel); the penumbrae hard to tell at
  these radii (the High Wall's stair shadow on the wall a little softer). Shown to Thomas 2026-11-03 19:40, the radius offered wider.
- **Step review** (fresh Opus, high, `16e9958d..17522c39`): nothing high. Answered (build-rrb2, the unit tests unchanged: the local
  lights' worst error 0.0251 as before): M1 the rt-lights-check pairs kept sharp (the atlas they are compared with); L1 a point's near
  cut stays its own cube around its position when the ray aims at a point of its sphere (a slab test on the view's axes; a spot's plane
  was already exact); L2 the sun's seed the frame's serial, not the view's version; L3 the seed a uint in both layouts, not float bits;
  L5 the disc sampled only for a light that casts a ray, its range renamed; L6 the comments and RR-PLAN §3 (1°). Kept: L4 (a frame whose
  guides fail shows its sampled shadows once through DLSS, then RR is off for the session: one frame, the failure itself logged); L7
  no unit case of the soft sampling (its maths reviewed; the runs show no grain).

## Final review (2026-11-03)

Fresh Opus, xhigh, `main...8d58485e` (with the I-0122 change of D-0247): nothing high; « mergeable after M2 ». Its exit-criteria table
found RR evaluated wherever the guides were drawn (the graph summaries' `rr_guides` count equal to `dlss`'s), no barrier mismatch in
the 20 runs, the layouts identical, the guides as RR's guide writes them. Answered (build-rrb3; with the fixes' step review's L-A: an
aim on a spot's sphere in front of its near plane traces the whole segment instead of returning seen): M1 no empty local atlas made for
a view without a face, and a refused local atlas allocation no longer faults the renderer (as the sun's); M2 STATE's rows; L2 the inert
count's row only when it changes (its unshadowed record taken back, below); L3 the game thread's RR
state requires the renderer's own flag too; L5 the colour before transparency kept (read by preset F, the 310.x default: `sl_dlss_d.h`);
L6 RR-PLAN, this RESULT, I-0122's RESULT, the index and status; L8 the frame's RR choice computed once. Kept: L1 (one to three frames
without the shadowed lights when a frame prepared without the atlas cannot run the native pass: I-0122 RESULT); L3's frames in flight at
a latch (prepared for RR, evaluated by DLSS: one or two); L4 the RR-only latch never witnessed (no test flag; its code reviewed twice) and
RR's instance kept after a latch until shutdown; L7 the guide test inside the DXR-gated hardware test (the module needs DXR for RR).
The fixes' step review (`8d58485e..1ad2ac14`): nothing high. Answered (build-rrb4): M1 L2's change taken back: a light whose shadow
words a record cannot take is inert again (its shadowed batch still holds it; an unshadowed record would light it twice on a frame with
an atlas and no top level; its strength not finite, the batch's own output is not either); L1 the inert count's row on the main view's
collect only (the reflection's collect reset it); L2 a refused local atlas's row the first then every 600th, its comment corrected (a
frame without a list then draws no shadowed light). Not reviewed again: a return to the reviewed `16e9958d` behaviour and two row gates.

## Evidence

- Streamline 2.14.16's RR guide and plugin source, read for RR-PLAN (§1, §2).

## Uncertainty / unresolved questions

- H1: not contradicted in the pictures of four zones (the pilot's eye); Thomas's verdict pending. The forward rows (water, hair) carry the guides of the surface behind (RR-PLAN §5): no defect seen from it yet.
- The roughness equivalence and the specular albedo's fit for DS3's Blinn-Phong (INFERRED, RR-PLAN §5).

## Suggested next step

Thomas's verdict on RR's look and on the penumbrae's radius (offered wider); then R4c (ReSTIR DI, I-0122) behind RR; the forward
rows' guides (water, hair) if a defect shows.
