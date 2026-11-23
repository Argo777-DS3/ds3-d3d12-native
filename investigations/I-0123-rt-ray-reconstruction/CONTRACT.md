# I-0123 - DS3 DX12 Vague 2: R5's other half, Ray Reconstruction and the traced shadows' penumbrae (La reconstruction des rayons)

Capability: C3

## Question

Can the native run NVIDIA's DLSS Ray Reconstruction (Streamline's `sl.dlss_d`, D-0165) in place of DLSS Super Resolution on the
traced frames, fed the guides DS3's G-buffer gives (diffuse and specular albedo, normals and roughness from R3's reading of the
gloss channel), so that the traced shadows can turn soft (one stochastic ray a light a pixel toward an area light: the sun's disc,
a local light's sphere) and be denoised by it, without a visible loss where nothing is noisy (RT-PT-PLAN §5, §8 R5; D-0237)?

## Scope

Included (the tranches of [RR-PLAN](RR-PLAN.md)):

- RR-a: Streamline's DLSS-RR feature loaded beside DLSS (its signed DLLs `sl.dlss_d.dll`, `nvngx_dlssd.dll`); a guide pass from the
  G-buffer (diffuse albedo, specular albedo, world normal with the roughness packed); the colour before the transparent rows; one
  evaluation of DLSS-RR in place of DLSS on the frames the rays run on (`rt-scene.flag`), in the mode the game's option and
  `dlss-mode.txt` give; `no-rr.flag` keeps DLSS Super Resolution (the comparison's other side).
- RR-b: the penumbrae: the sun's ray toward a point of its disc, a shadowed local light's toward a point of its sphere, a new
  sample a pixel a frame, only on the frames DLSS-RR denoises (a frame without it keeps the sharp ray: no noise reaches the screen).
- The pictures shown to Thomas (RR against Super Resolution, sharp against soft), since the look is his to judge (D-0181, D-0237).

Excluded:

- The specular hit distance and the specular motion vectors: DLSS-RR's only hit-distance input is the reflection rays' (R6), which
  do not exist yet; a shadow ray's distance has no input (RR-PLAN §2). The local atlas's removal (I-0122, on Thomas's yes), ReSTIR
  DI (R4c, after this), GI (R7), the transparent rows' own guides (R9), frame generation (R10).

## Inputs

- Streamline 2.14.16 (`C:/Users/User/Tools/DS3-Decompilation/third-party/streamline-2.14.16`: `include/sl_dlss_d.h`,
  `docs/ProgrammingGuideDLSS_RR.md`, `docs/DLSS-RR Integration Guide.pdf`), the module's host `native/renderer/streamline_host.{h,cpp}`
  (I-0111), the DLSS evaluation in `scene_pass_frame.cpp`.
- The G-buffer as the native reads it (I-0119 R3b, `rt_view.hlsl`; I-0122 `rt_lights.hlsl`): GB0 the world normal × 0.5 + 0.5,
  GB1.x the gloss g (the exponent 2^(13 g)), GB2 the albedo through its sRGB view, GB3 √F0; R3-PLAN §2 the roughness equivalence.
- The traced sun (`rt_shadow.hlsl`, I-0118) and local lights (`rt_lights.hlsl`, I-0122 R4b).
- ER has no analogue (D-0195, D-0236): Elden Ring has no DLSS and its ray tracing is not a model (connaissance générale for the first).

## Hypotheses

- H1: on frames whose lighting is mostly raster and noise-free, DLSS-RR's picture stays as sharp as Super Resolution's (no
  smearing of the textures or the effects), given correct guides.
- H2: DLSS-RR turns one stochastic shadow ray a light a pixel into a stable soft shadow at 60 frames/s.

## Expected evidence

Runs at the witness zones (Firelink at three sizes, the High Wall, Anor Londo, Ariandel), each frame captured twice (RR and
`no-rr` in separate runs on the same path), the pictures side by side and FLIP's mean between them; the guides' own pictures
(albedos, normal, roughness) checked against the G-buffer; then the soft shadows against the sharp ones on the same frames.

## Exit conditions

- Success: DLSS-RR runs on traced frames at every witness zone, its picture judged by Thomas, the penumbrae in, the default with
  the rays.
- Refuted: DLSS-RR smears what raster lighting draws (H1 false) beyond what a guide can fix: then RR only where a pass is
  stochastic, or a classic denoiser on the shadow term (a decision for Thomas).
- Insufficient information: the pictures cannot tell RR's loss from DLSS's own (then FLIP against a native frame without DLSS).
- Stagnation: no new discriminating evidence after 3 cycles.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`; renew a bounded budget automatically within unchanged authorized scope and existing protected boundaries.

## Safety and human interaction

Live runs announced (WORKFLOW « Live game rule »), offline, test saves restored by hash, the ASUS alone while Thomas is away (the
OLED off). New NVIDIA DLLs from the already downloaded and checked Streamline package (signed by NVIDIA, checked by the interposer
before loading; Thomas's yes of 2026-10-30 04:51 covers the package). No irreversible change.
