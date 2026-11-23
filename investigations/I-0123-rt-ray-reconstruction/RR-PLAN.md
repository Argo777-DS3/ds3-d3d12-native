# I-0123 RR-PLAN: Ray Reconstruction and the penumbrae

2026-11-03, the R pilot (RT-PT-PLAN §5 and §8 R5, D-0237; after I-0122 R4b). Statuses: OBSERVED (read in Streamline's headers, its
`sl.dlss_d` plugin's source, its guides, or the native's code), INFERRED, TO CHECK. Sources: Streamline 2.14.16
(`third-party/streamline-2.14.16`): `include/sl_dlss_d.h`, `source/plugins/sl.dlss_d/dlss_dEntry.cpp`,
`docs/ProgrammingGuideDLSS_RR.md` (« PG »), `docs/DLSS-RR Integration Guide.pdf` (« IG », its August 2026 edition, pages as
printed).

## 1. What DLSS-RR is, for this engine

DLSS Ray Reconstruction replaces DLSS Super Resolution at the same place in the frame: it upscales (or, in DLAA, keeps the size) and
denoises in one pass, from the noisy colour, the depth, the motion vectors and guides (PG §3.0, §4.1; OBSERVED). DS3's frame keeps its
shape: the lit HDR after the forward rows and the effects, the velocity, then DLSS (I-0111), then YEBIS, the glow, the UI and the
gamma at the output size. Only the call and its inputs change.

ER has no analogue (D-0195, D-0236): Elden Ring ships no DLSS (connaissance générale) and its ray tracing is not a model. What the
native does better than a usual integration: the guides come from the G-buffer the native already writes, in one compute pass, with no
second geometry pass.

| Input | Where it comes from | Status |
|---|---|---|
| Colour (`kBufferTypeScalingInputColor`) | the lit HDR, as DLSS today | OBSERVED (I-0111) |
| Depth, motion vectors, jitter, matrices | as DLSS today; at least 32 jitter phases (IG §3.6: « at least 32 » for RR; the module uses 8 for DLAA) | OBSERVED; the phase count to raise on RR frames |
| Diffuse albedo (`kBufferTypeAlbedo`) | GB2 (the albedo, read through its sRGB view: linear); the sky (0.5, 0.5, 0.5) (IG §3.4.1) | INFERRED: the composite multiplies the diffuse accumulation by it |
| Specular albedo (`kBufferTypeSpecularAlbedo`) | EnvBRDFApprox2(F0, α, N·V) (PG §4.2.1, IG appendix) with F0 = GB3², α = r²; the sky 0 | OBSERVED (the formula); INFERRED (DS3's F0 and the approximation's fit to DS3's Blinn-Phong) |
| Normal and roughness (`kBufferTypeNormalRoughness`, packed) | GB0 × 2 − 1 (R16G16B16A16_FLOAT), a world normal (the native's light pass shades world positions with it: `rt_lights.hlsl`), in an RGBA16F target, w the roughness r = (2 / (2^(13 g) + 2))^¼ from GB1.x (R3-PLAN §2's equivalence); the sky (0, 0, 0, 0) | OBSERVED (world space); INFERRED (r) |
| Colour before transparency (`kBufferTypeColorBeforeTransparency`) | a copy of the lit HDR at the start of the forward pass (before any forward row, the water's included, and before the effects): the same format, as IG §3.4.11 asks | OBSERVED (the point in `scene_pass_lighting.cpp`); read by presets E and F (IG §3.4.11; F the 310.x default, `sl_dlss_d.h`); its benefit not measured |
| Exposure | none: IG §3.7 « ignore » DLSS's exposure sections for RR; the lit HDR is linear, not pre-exposed | OBSERVED |
| Specular hit distance or specular motion vectors | none until R6 (the reflection rays); RR then reads the reflections' distance (`kBufferTypeSpecularHitDistance`, with `worldToCameraView`, `cameraViewToWorld`) | OBSERVED (optional, IG §3.4.9) |
| Diffuse hit distance | the plugin reads `kBufferTypeDiffuseHitDistance` (dlss_dEntry.cpp:581): GI's rays (R7) | OBSERVED (in the plugin; not in the guides' text) |

The coordinator's brief named « the rays' hit distances »: DLSS-RR has no input for a shadow ray's distance (IG lists none; the
plugin reads only the specular and diffuse ones), so this workstream gives none; R6 and R7 will.

## 2. Streamline

- `sl.dlss_d.dll` and `nvngx_dlssd.dll` beside the module (`Build-GameBootstrap.ps1`'s list; the launchers copy every `sl.*` and
  `nvngx_*` the build puts there), `kFeatureDLSS_RR` loaded with DLSS, Reflex and PCL; `slIsFeatureSupported(kFeatureDLSS_RR)` on
  the adapter. A refusal leaves DLSS Super Resolution (OBSERVED: separate plugins, separate viewport contexts).
- `slDLSSDSetOptions` (mode, output extent, HDR, packed roughness, `worldToCameraView` and `cameraViewToWorld` for later, the default
  preset in every mode: IG §3.13 recommends Default; the 310.x default is preset F, `sl_dlss_d.h`), sent every RR frame (the camera's rows; the plugin
  rebuilds its instance only on a mode, size, preset or roughness change, dlss_dEntry.cpp:347). `slDLSSDGetOptimalSettings` gives the render extent in a scaled mode (the plugin creates NGX at its own
  optimal size, dlss_dEntry.cpp:470, so the frame must render at it, not at Super Resolution's).
- `slEvaluateFeature(kFeatureDLSS_RR)` with the tags: colour, output, depth, motion, the three guides, the colour before
  transparency. The same constants as DLSS (`slSetConstants`).
- Which frames: every frame DLSS would evaluate while the rays are on for the session (`rt-scene.flag`, the device's DXR, the
  self-test) and the adapter offers RR, unless the player turned it off.
- **A player option** (Thomas, 2026-11-03, D-0247: « ça doit rester une option en jeu comme les autres jeux »; RT-PT-PLAN §6: On
  by default in ray tracing, Off allowed, required in path tracing): until the options menu (R10), `rr-mode.txt` beside the module
  (one word, « off ») as `dlss-mode.txt`, read at start; `no-rr.flag` for the test runs. The menu (R10) switches it at run time (a
  switch restarts the history, as a mode change does). A frame does not switch between the two (each switch would restart a
  history); a frame without a top level goes through RR all the same (nothing noisy in it). An evaluation refused latches RR off and
  the next frame runs Super Resolution (the latch I-0111 has, one level down), then DLSS's own latch as before.

## 3. The penumbrae (RR-b)

- The sun (`rt_shadow.hlsl`): the ray toward a point of the sun's disc, uniform in the cone of half-angle θ around the direction the
  frame gives; θ 1° as first written (the physical sun's 0.27°, connaissance générale, gave too thin a penumbra to show; the original's
  cascades blur wider), 2° since Thomas's choice on the pictures (2026-11-04, I-0122 R4c). One sample a pixel a frame, from a hash of (pixel, frame serial) (IG §3.5: white noise,
  independent across pixels and frames, a good hash such as xxhash32 or PCG; no screen-space pattern).
- A shadowed local light (`rt_lights.hlsl`'s RtLightSeen): the ray toward a point of a sphere of radius ρ around the light, uniform
  on the sphere's disc seen from the surface; ρ not a datum of DS3's lights (they are points): 10 % of the light's range, at most 0.5 m
  (Thomas's choice, 2026-11-04; first 5 % and 0.25 m), the near-plane cut kept.
- Only on frames RR evaluates (a constant in each pass's block: θ = ρ = 0 otherwise): a frame without RR keeps today's sharp
  shadow, never noise on screen.

## 4. Tranches

| Tranche | Content | Seen in |
|---|---|---|
| RR-a | Streamline's RR feature, the player's option (`rr-mode.txt`); the guide pass (`rr_guides.hlsl`, one full-screen pixel pass inside the lighting after the composite, into three targets the size of the G-buffer); the colour before transparency (the lit HDR as the composite leaves it: before every forward row, the opaque 0x1F rows included, which RR may then take for transparency: TO CHECK); RR in place of DLSS on traced frames, `no-rr.flag`; the jitter's phases raised to 32 at least on RR frames; the guides checked by the unit test (RR's own debug view needs NVIDIA's development DLL, IG §8.1) | Runs at the witness zones, RR against `no-rr` on the same path; nothing noisy yet: H1 |
| RR-b | The penumbrae (§3) on RR frames | The same frames, sharp against soft; Thomas's eye: H2 |

The atlas's removal (I-0122) passes in front of either as soon as Thomas says yes.

## 5. Risks

- RR on a mostly raster frame (H1): RR is trained for path-traced input; on noise-free raster lighting it may soften textures or
  smear the effects. Measured by the RR / `no-rr` pairs; the colour-before-transparency guide is the first answer for particles.
- The roughness equivalence (INFERRED) and the specular albedo from a GGX fit for a Blinn-Phong engine: a wrong guide shows as a
  blurred or a boiling specular; the guides' pictures and RR's own debug view (IG §8.1: the development DLL only, CTRL+ALT+F12) are
  the checks.
- Forward rows over a G-buffer surface (water, the 0x29 rows): their pixels carry the guides of the surface behind (IG §3.4.1 allows
  blending toward the transparent surface; not done in RR-a).
- Memory: three render-size targets (R11G11B10 twice, RGBA16F) and one HDR copy, about 50 MB at 1920x1080 by the formats, plus RR's own
  (`DLSSDState`, not read).
