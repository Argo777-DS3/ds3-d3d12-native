# R4, « Les lumières pour les rayons » : the plan

2026-11-03, the R pilot (RT-PT-PLAN §8 R4, after R3 and I-0117 T7; R5b with it). Statuses: OBSERVED (read in a programme's
disassembly or the native's code), INFERRED, TO CHECK. The disassemblies are in `evidence/I-0122/artifacts/dxbc/` (`fxc
/dumpbin` of the I-0063 programme cache; outside Git).

## 0. In short

DS3 lights a frame with the sun (its own programme, its shadow traced since I-0118), the local lights (points, legacy points,
spots, the effects' lights among them) drawn as tile batches of the `GXLightAcc_D*Light*` programmes, the baked ambient (GB4, the
lightmaps) and the env-map regions. R4 takes the local lights: one world-space list on the GPU, built from the lights the native
already selects, and a native pass that shades them with DS3's own formulas (read in the DXBC) into the same accumulation
targets, on with the rays. First at parity with the original (R4a, a numeric check), then with their shadows traced instead of
the local atlas (R4b, the plan's R5b), then sampled when every light casts (R4c, ReSTIR DI, behind Ray Reconstruction). The same
list and formulas are what R6 to R8 shade a ray's hit with: that is why the list is in world space and holds every light the
frame lights with, not only the camera's.

## 1. What DS3 does (OBSERVED)

**The list.** The engine's light container `[[GXRS]+0x41D0]` holds the active bank's points and spots and the effects' lights
(I-0110 §5.2, EFFECT-LIGHTS row 609/11000). `CollectNativePointLights` keeps each enabled one of the main view's category (+0xB0,
+0xA0 & 1) whose distance fade (1F1C60, the SettingInfo blend) is positive: the points with their colours already
`c² × k × scale × fade`, the spots through the genuine item builder 2136B0. No frustum cull: the 128-pixel tile binning
(`ds3tiles::Bin`, 234E40) assigns them to the screen. `CollectNativeLocalShadows` takes the shadowed ones (at most 48, the
container's capacity).

**The programmes.** Per pixel, P the view-space position from the depth, E = P / |P| (camera to point), N = GB0 × 2 − 1 in view
space (cb0[4..6]), g = GB1.x, t = GB1.y, F0 = GB3², ao = t33.x; L = (Lpos − P) / d, d² = |Lpos − P|²; w = DiffColor.w (the
gloss clamp); n = 2^(13 g), n' = max(1, n w / (0.01 n + w)).

| Programme | Diffuse | Specular |
|---|---|---|
| `DPointLightN` (N ≤ 12, the same term summed) | t = 0: sat(N·L) Diff × fd; else max(N·L + 10t, 0) / (1 + 20t) Diff × fd + T; T = sat(L·E)^(32 TL.y) × (max(1 − E·N, 0) / 2)² × TL.x × Diff × 10t × fd; fd = max(1/d² − R⁻², 0) | sat(N·H)^n' × (n' + 2) / 8 × Spec × sat(N·L)^0.1 × fs × F0; H = normalize(L − E); fs = 1 − sat(d² R⁻²) |
| `LegacyDPointLightN` | as the point with fd = fs = (1 − sqrt(sat(d² R⁻²)))², the translucency T times it | sat(R_E·L)^(n'/4) × (n' + 2) / 8 × Spec × f × F0, R_E = reflect(E, N) (no N·L^0.1) |
| `DSpotLightN` (N ≤ 4) | light-space q = M·P (`ViewToLightSpaceMatrix`), lit only where q.z > 0; cone c = max(1 − |q.xy / q.w|², 0); `ShadingMode` > 0: the point's terms with fd × c and fs × c; else the legacy's lobe and both falloffs sat(1 − q.w / R)² × c² | as its diffuse's mode |

Both outputs × (diffuse: 1 − F0 / 2; specular: F0 as above) × ao, added (blend ONE/ONE) into the R11G11B10 pair `light_targets`
0 (diffuse) and 1 (specular). The shadowed variants (`DPointLightShadowedN`: the row grows `ProjParam`, `ShadowCenterPos` and
six faces' `ViewportScaleOffset`) take a visibility v = 1 + ProjParam.w × (s − 1), s the mean of four comparison taps (s12) of the
local atlas around the point's cube face: ProjParam.w is a shadow strength. v multiplies the wrapped or Lambert diffuse and the
specular, **not the translucency** T (OBSERVED in `DPointLightShadowed1`; `DSpotLightShadowedN` the same form with
`ShadowParam.x` as the strength and one atlas rectangle). R4b keeps that form with
the ray's visibility in place of s. The sun's programme differs (Schlick, a visibility, 1 − F0: I-0119 « The Fresnel »), and stays.

**Elden Ring (D-0195, D-0236).** ER's ray tracing (1.09) traces the sun's shadows and AO (ER-PIPELINE-STUDY §8); its local lights
stay raster. ER has no traced local light to copy: its place in the engine is the one used here (the container is the engine's
own, the native reads it as ER's renderer would). What the native does better: the lights the camera does not see stay in the list
for the rays.

## 2. The frame challenge (WORKFLOW, anti-tunnel)

- Sought: a native shading of DS3's local lights from a world-space list, because R6 to R8 shade hits the G-buffer never saw, and
  R5b needs the local lights' shading native to replace the atlas's term with a ray's.
- Engine invariant: the light items and their colours (recovered, kept); redesigned: the tile batches, the programmes' view-space
  constants, the atlas (the backend's mechanics).
- Smaller experiment first: R4a changes nothing on screen (parity), and its check discriminates a transcription error before R4b
  builds on it.

## 3. The tranches

| # | What | Seen | Merge |
|---|---|---|---|
| R4a | The one list (`RtLight` records, world space) and the native local lighting pass from it, on with the rays, in place of the unshadowed point, legacy and spot batches; `rt-lights-check.flag` draws both into separate targets and counts | Nothing changes (parity) | Its own |
| R4b | (R5b) The shadowed lights in the native pass, one traced ray a light a pixel (the local casters' mask RtMaskLocal, the engine's category 0x04 that the atlas's faces draw, rather than RtMaskSun's as first written; the alpha test as the sun's); with Thomas's yes on the picture, the local atlas, its faces and casters no longer made on traced frames | The local shadows traced (Anor Londo, Irithyll, Firelink's spots) | Its own |
| R4c | ReSTIR DI for the path-traced level (every light casts, sampled), after Ray Reconstruction (R5's other half); the list then serves R6's hits | The PT level's direct light | Its own |

## 4. R4a in detail

- **The list** (`NativeRtLights`, built at collect beside the batches, from `scene.point_lights` and `scene.spot_lights`): a
  record of 128 bytes a light: kind (point, legacy point, spot), world position and R⁻² (the point's 1/r², the spot's range R),
  diffuse colour and w, specular colour, TL.x, TL.y, the spot's mode and its world-to-light rows (the item's view-to-light matrix
  times the camera's world-to-view: rows taken as they are applied, Mᵀ × V), whether shadowed. The view-space rows are kept only for the
  original programmes (the shadowed batches, the check, the forward grid).
- **The tiles.** The pass reads, per 128-pixel tile, the list's indices the binning gave it (one buffer of offsets and indices,
  the batches' order), so each pixel sums exactly the original's lights.
- **The pass** (`rt_lights.hlsl`, dxc ps_6_6, one full-screen triangle, blend ONE/ONE into targets 0 and 1 as the programmes):
  P rebuilt in world space from the depth and the camera (as `rt_shadow.hlsl`), N from GB0, g, t from GB1, F0 from GB3, ao from
  t33.x; the formulas of §1 in `rt_lights.hlsli` (one function a kind, the shared reader R6 calls later). On with the rays
  (`rt-scene.flag` and a passing self-test), as the traced sun; the shadowed groups keep the original programmes and the atlas
  until R4b.
- **The fallback**: the unshadowed batches are still made; a frame whose pass cannot run draws them (step review M1).
- **The check** (`rt-lights-check.flag`, a test tool): the original batches drawn into a cleared scratch pair, the native pass
  into a second, both read back; `Compare-RtLights.py` counts offline by target the pixels within 1, 2, 4 and 8 steps of 255
  (relative above 1) and the largest error; the frame keeps the native pass.
- **Tests** (`rt_scene_test.cpp`): the world rows against the view rows (a light's q from the world position equals the item's
  from the view position), the record's layout by `static_assert` and the pass against the point programme's terms.
- **The run**: one announced slot, the High Wall, Firelink, m37 and m45, at 1280x720, 1920x1080 and one arbitrary window (the
  three sizes, WORKFLOW), `rt-scene` with `rt-lights-check`.

## 5. The fill lights and the baked ambient (decided: kept until R7, Thomas 2026-11-03, D-0247)

OBSERVED (I-0110 §5.1, §6): most map lights are cold, unshadowed fill lights named `Omni*`, placed beside the lightmaps to fake
the bounce; 89 to 97 % of the map meshes carry the baked ambient (GB4). Both cheat what the traced GI (R7) will compute.

Recommendation: keep both exactly as the original through R4 to R6: R4 lights the fill lights as DS3 does (unshadowed, its
formulas), GB4 stays. Removing them before GI exists would leave interiors black. At R7 (GI), the baked ambient goes (D-0237:
removed once the traced GI is accepted) and the fill lights are tagged (their names by matching the bank's positions, I-0110 §5)
so that the lighting pass can dim or remove them zone by zone with Thomas's eye. At the PT level (R4c, R8) a fill light casts like
any light. Nothing in R4a depends on the choice.

## 6. Risks

- The R11G11B10 targets quantise after each blended batch in the original, once in the native pass: small differences, measured
  by the check (the tolerance set from the first run).
- A light the native selects the original does not (CollectNativeLocalShadows' « open » masks, I-0106): unchanged by R4a (the same
  items), to keep in mind when the list serves the rays.
- The spot's view-to-light rows: 2136B0 builds them from the main view's camera; their world form must be the light's own, not
  the camera's (the test above).

## 7. R4c in detail (2026-11-04, the R pilot)

**The frame challenge.** Sought: the path-traced level's direct light, where every light of the list casts (D-0247: « at the PT
level a fill light casts like any light »), at the cost of a few rays a pixel whatever the number of lights. Engine invariant: the
list and DS3's formulas (§1, unchanged); redesigned: which lights a pixel traces. ER has no analogue (its local lights stay raster,
§1): the method is ReSTIR DI's (Bitterli et al., SIGGRAPH 2020; NVIDIA's RTXDI; connaissance générale), written here on the native
list, not RTXDI's code (a new download is Thomas's); its first stage kept (below). Smaller experiment first: none smaller answers « does every light casting look
right » than the pass itself, behind its option.

**The level.** `rt-level.txt` beside the module (one word, `rt` the default or `pt`; the player's option until R10's menu, as
`rr-mode.txt`) or `rt-pt.flag` (the test runs). The PT direct light runs only on a frame Ray Reconstruction denoises with a top level
(RT-PT-PLAN §6: RR mandatory in PT); any other frame shades the RT level (R4a and R4b, unchanged).

**What a PT frame does** (one draw of the full-screen triangle, `pt`, in place of R4b's `shade`, in the same pass and targets): per
pixel, the candidates are the lights its tile holds (the binning: a light outside the tile lights none of its pixels). The target
p̂(light) = luminance of (diffuse × (1 − F0 / 2) + specular × F0), the unshadowed terms of §1 without the translucency; one light chosen
in proportion to p̂ over all the tile's candidates (resampled importance sampling with every candidate, ReSTIR's first stage), one ray
toward it (`RtLightSeen`, the visibility v, 1 or 0), its terms × Σp̂ / p̂(y) × v: an unbiased estimate of every light's shadowed terms,
one ray a pixel a frame, for Ray Reconstruction to denoise. Added: the translucency of every tile light summed unshadowed (the
`Shadowed` programmes' form, §1). A light with no near plane (the unshadowed points): the ray stops short of it by its sphere's radius,
at least 0.1 m (a lamp's own housing; INFERRED, to be seen).

**No reuse (ReSTIR's spatial and temporal stages), as written and reviewed** (2026-11-04): the first version reused four neighbours'
reservoirs with Bitterli's 1 / Z on the unshadowed target, which darkened every lit pixel beside a shadow (a shadowed neighbour's
reservoir counted) and carried the loss from frame to frame (step review H1); the second kept the temporal reuse alone, whose empty
reservoirs lost their M exactly on the histories that came out dark, a bright overshoot for some 40 frames after every restart and along
every disocclusion (its review H1'), and whose fix either lags a moving caster's shadow for up to 20 frames (the reused sample not traced
again) or darkens the penumbrae (traced again). With every tile light already a candidate, the reuse would only share visibility; its
unbiased forms need a ray from each reused domain. Left out until a picture shows a noise that Ray Reconstruction does not remove.

**Every light casts whole** (decided in the writing, 2026-11-04): the `Shadowed` programmes' strength (ProjParam.w, 0.8 at the
bonfires) is a raster's partial shadow; at the PT level v is 1 or 0.

**Seen**: the PT level beside the RT level at the four zones (Firelink at the three sizes), pictures for Thomas; no parity measure
(the PT level is a different picture by design, D-0237). **Tests**: on the hardware device 256 frames of one tile with five lights, all
hidden from half the pixels, whose mean approaches the terms with every light casting.

**Not in R4c**: the lights the camera's tiles do not hold (R6's hits need a world-space structure: with R6), the emissive surfaces
(RT-PT-PLAN §8 « Piste à évaluer »), the sun (its own traced ray, I-0118).
