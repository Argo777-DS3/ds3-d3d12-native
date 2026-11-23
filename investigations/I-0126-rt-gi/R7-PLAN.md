# R7, « L'illumination globale tracée » : the plan

2026-11-06, the R pilot (RT-PT-PLAN §8 R7; after I-0125 R6). Statuses: OBSERVED (read in the native's code or a cited primary
source), INFERRED, TO CHECK. The rules it follows: D-0237 and D-0249 (a traced picture is judged by its truth, not the original's
pixels; a datum misread is a defect), D-0247 (the baked ambient goes at R7; the maps' Omni fill lights tagged for Thomas), D-0254 (the light sets' directional fill lights go on the GI frames), D-0251 (a raster
path with a traced equivalent is removed, not kept behind a flag), D-0197 (no stand-in), the RT and PT levels of RT-PT-PLAN §6. The
study thread's reports in this folder carry the detail: [STUDY-R7-GI](STUDY-R7-GI.md) (the synthesis), [STUDY-R7-DS3-AMBIENT]
(STUDY-R7-DS3-AMBIENT.md), [STUDY-R7-ER-GI](STUDY-R7-ER-GI.md).

## 0. In short

DS3 lights its surfaces' diffuse ambient from baked data: the lightmaps (about 97 % of m30's and 89 % of m40's map pieces), the
instance's SH rows (characters, props), the decals' own SH, all written into GB4 by the G-buffer programmes, then `GB4 × SSAO × mask`
in `GXLightDeferredAO` (pass 0). R7 replaces that term by a traced diffuse bounce: one cosine-distributed ray a pixel from the
G-buffer surface, its hit lit as R6's reflection hits are (the sun with its shadow ray, the frame's local lights, now the hit's
emission), its further bounces read from a world-space radiance cache (SHaRC) at the RT level, the first bounce's paths reused by
ReSTIR GI at the PT level. Ray Reconstruction denoises it with the guides it already has. On the frames the GI serves, pass 0 gets
no baked ambient and the SSAO chain is not run (the shadow mask stays); by D-0251 that raster code then goes.

## 1. What changes in the frame (OBSERVED: `native/renderer/scene_pass_lighting.cpp`; the fill lights' removal amended by D-0257: kept by default)

| Today | With R7 (a GI frame) |
|---|---|
| `EncodeNativeShadowMask` (`:355`): the SSAO chain (`ao_.Record`) then the mask t32 (the sun shadow, GB1.z) | the mask alone, t33 the neutral white (`native_light_neutral_[0]`): t32.x = 1, so the directional lights' `1 + w (AO − 1)` and the local lights' `t33.x` are neutral |
| pass 0 `GXLightDeferredAO` reads t6 = GB4 | t6 bound to the neutral black (`native_light_neutral_[1]`) in pass 0's view range: no baked diffuse ambient, the decals' GB4 included (STUDY-R7-GI §4c); the specular ambient (t7, R6b) unchanged |
| pass 2 `GXLightCmbAlbedo` → `light_targets[2]` (R16G16B16A16F) | then **the GI pass**: additive full-screen into `light_targets[2]`, `GB2 × (1 − F0) × E(x)`, before the SSS (the skin keeps its blur of the whole diffuse) and the composite; RR then denoises the lit HDR as today |
| R6's `HitRadiance`: `albedo × (RtAmbientOf + sun + local) + specular` | the emission once FC_EmissiveColor is read for the rays (after R7a, before R7b's dark interiors); `RtAmbientOf` replaced by the cache's radiance at R7b |

`E(x)`: one ray a pixel, cosine-distributed about the G-buffer's shading normal (GB0), from the pixel's world position offset as
R6's rays are (`frame.offset`); its hit `HitRadiance`; its miss R6d's `SkyRadiance`, else the light set's hemisphere rows
(`(U + D)/2 + (U − D)/2 · N.y`, sets buffer rows 12 to 14: INFERRED from the rows' formula, STUDY-R7-DS3-AMBIENT §3.2). The GI's
hit distance goes to RR's diffuse guide if Streamline's RR takes one (TO CHECK in the RR guide: today only the specular hit
distance is written).

**Also on a GI frame** (as written, R7a): the light sets' lights 1 to 3 (DS3's unshadowed directional fill lights) zeroed in the
deferred passes' cbLight upload, so for the pixel and the rays' hits (D-0254); a ray that misses takes the forward sky, else the
region cubes at their sharpest level, else the hemisphere rows along it; the bounce legs are not fogged (the composite fogs the pixel
for the eye; the sky a ray meets is still fogged from the surface, R6d's rule: to settle at R7b); a GI frame whose rays' scene has
no top level or material table this frame adds nothing (its baked ambient, SSAO and fill lights already left out: counted, never
seen in the witness runs); the GI needs the traced sun still on for the session (its hits' sun shadowed). The forward rows (the
hair, translucent pieces, the water, the lit effect models) keep their own cbLight, fill lights included, with their baked ambient
until R9.

**Not R7's:** the forward rows' ambient (the hair, the translucent pieces, the water: they keep their SH or lightmap until R9; a GI
frame mixes both, listed for R9), the specular ambient (R6), the emissive textures as sampled emitters (later, RT-PT-PLAN §8 lead).

## 2. Elden Ring (D-0195, D-0236) and the frame challenge

ER 1.09's ray tracing traces occlusion only for its ambient (`GXRayGen_ao`, a temporal feedback and an à-trous filter, a distance
fade into its raster AO; STUDY-R7-ER-GI §2), no radiance bounce, no cache. There is no GI to copy; what R7 takes from it is the place
(a traced ambient stage after the G-buffer and before the deferred ambient composition) and, for frames without RR, its denoiser's
shape (§5 option A). ER's known defects not copied: its raster AO kept under the traced one (D-0251 removes ours), its TLAS cap, its
vendor gate, its 12_0 device.

Sought: the diffuse light a surface receives from the scene, traced. Engine invariant: the G-buffer (GB0 to GB3, GB5), the
direct light passes, the composite and its fog, the forward rows. Redesigned: the ambient's source (baked → traced). Unknown: none
blocking; three checks are listed in §6.

## 3. The radiance cache (RT level): SHaRC

NVIDIA's Spatially Hashed Radiance Cache (`https://github.com/NVIDIA-RTX/SHARC`, v1.6.5.0 of 2026-02-28, the « NVIDIA RTX SDKs
LICENSE »): downloaded with Thomas's yes (2026-11-06 17:18, « Prends SHARC »), the tag's source archive `sharc-v1.6.5.0.zip`
(10,562,988 bytes, SHA-256 823A54F6…1422, its images included), kept in `C:/Users/User/Tools/DS3-Decompilation/third-party/
sharc-1.6.5.0-download/`; its headers, licence, README, changelog and integration guide extracted to `third-party/sharc-1.6.5.0/`
(outside the repository, as Streamline). Shader-only HLSL headers (`include/SharcCommon.h`, `HashGridCommon.h`, `SharcTypes.h`;
`SharcGlsl.h` for GLSL), three passes (an update from a share of the pixels' paths, `SharcUpdateHit`/`SharcUpdateMiss`; a compute resolve
of every entry; the query `SharcGetCachedRadiance` at a path's vertex), DXIL shader model 6.2 or newer with 16-bit types, its
buffers cleared at load, 64-bit atomics optional. It fits the native's inline ray queries (the passes are ordinary shaders around a
path loop). The licence asks for NVIDIA's notice (« This software contains source code provided by NVIDIA Corporation. ») in what is
distributed and forbids putting the SDK under an open-source licence: the project's GitHub copy is private now and will be public
later (Thomas, 2026-11-06 17:18), so SHaRC's headers never enter the repository or the copy (the build reads them from
`third-party/`, as Streamline's), and the notice goes with the distributed module.

Its seed and its cold start: a cell the cache has not filled answers with the hit's baked ambient (`RtAmbientOf`, within 5 % of the
raster's GB4 on 90-100 % of the pixels per zone, R6a), faded out as the cell fills, never added once the cell answers (else the
bounce counts twice). The lightmaps' colour holds no direct sun (their alpha is the sun's mask: STUDY-R7-GI §4b, INFERRED strong), so
the seed does not count the traced sun twice.

The cache also serves R6's reflection hits (their ambient, `RtAmbientOf` today) and, at R9, the effects and transparents.

**As written (R7b, 2026-11-07).** What a cell holds: the ambient of its surface (its diffuse indirect light over its albedo), not
SHaRC's usual outgoing radiance, so a query takes `RtAmbientOf`'s place at any hit and the hit keeps its own direct light exactly
(its traced sun shadow, its local lights, no voxel blur on them). Made with SHaRC's own calls: the material demodulation on (the
albedo, floored at 0.02), each path vertex's direct light given to the vertices before it (`SharcUpdateMiss`) and never to its own
cell, which `SharcUpdateHit` inserts with no light (its sample counted); the throughput a leg carries the vertex's albedo × (1 − F0).
The update: one path a 5 × 5 block (a draw over the reduced grid, a random pixel of the block each frame: the guide's 4 %), from the
pixel's surface along a cosine direction, up to three cells, the last hit giving its direct light and its ambient (the cache, else
the seed) without a cell; SHaRC's resampling ends a path at a filled cell; a miss the forward sky, else the cubes or the
hemisphere at the vertex. The grid: SHaRC's logarithmic levels, scale 50 (4 cm within 2 m, 16 cm at 10 m), the cells keyed by the
hit's geometric normal (a wall's two sides apart); 2^22 entries (160 MiB, made on the first GI frame; a small stand-by buffer bound before), cleared again after a 100 m camera jump, the resolve's window
10 frames, a cell evicted after 64 frames without a sample; the seed lerped out by the cell's samples (full at 4). A vertex gives its
specular toward the vertex before it too (light that vertex receives, view-free); one whose next leg would cross its horizon ends
the path without a cell. At a reflection's hit the cell answers only as far as the path's footprint covers it (SHaRC's guide: the
cone over the path and the lobe's 2 t sqrt(a² / 2 (1 − a²))), and as far as the segment to the hit spans a cell (a corner, a
contact: the guide keeps tracing there; a ramp over one cell here), the seed otherwise (a mirror, the water, a short segment). Queried at the reflections', the water's and the GI's hits on
the GI frames (the cache is updated and resolved only there, before the reflections). `no-rt-cache.flag` (a diagnostic: R7a's
seed at the hits) and `rt-gi-no-sky.flag` (a diagnostic: the GI's misses bring nothing; the misses' share by difference).

## 4. The tranches

| # | Content | Seen |
|---|---|---|
| R7a | The GI pass (§1) at the RT level: one bounce, its hit lit by `HitRadiance` (the hit's emission after R7a: its FC_EmissiveColor, cbMatDynParam +0x20 from the material's GX03 item, is not read for the rays yet, and the emissive texture alone would light the scene with a guessed colour), the further bounces the hit's baked ambient (`RtAmbientOf`, the cache's seed, standing in until R7b); pass 0 without GB4 and the SSAO's channel neutral on its frames; RR denoising it; a GI frame needs the rays' scene, the local lights' pass (its list) and RR (§5) | Firelink, the High Wall, the Undead Settlement, an interior (the Cathedral or the catacombs: thin walls); mean luma by signalstats against today's baked ambient, pictures to Thomas |
| R7b | SHaRC (§3, « As written »): its update (its own paths from 4 % of the pixels, up to three cells), its resolve, its query at the GI hits and at R6's reflection hits in place of `RtAmbientOf`; the seed's fade | the same views; a dark interior lit by its fires (the bonfire, the candles: the emission) |
| R7c | The frames without RR: the GI denoised by NVIDIA's NRD (§5, Thomas's choice), so the GI serves every traced frame; then D-0251's removal: the SSAO stage (`ao_stage`, its passes, its history) and pass 0's baked-ambient input on the traced frames; the control view's GB4 comparison kept on its own frames (a copy of GB4, or the view's flag); `no-rt-gi.flag` | the start check; one run per zone at 1280x720, 1920x1080 and an arbitrary window (the three sizes, WORKFLOW) |
| R7d | The PT level: ReSTIR GI on the first bounce's paths (temporal then spatial reuse, its Jacobian and visibility checked before it stays: R4-PLAN §7's lesson, written twice there and left out); else recorded as left to R8 with why | the PT level against the RT level on the same views |

Each tranche: a step review (fresh Opus, high), the optimisation pass, the final review before its merge (fresh Opus, xhigh), the
launcher rebuilt and the start check after it.

## 5. Thomas's choices (asked 2026-11-06 17:10, answered 17:18 and 17:21)

**Answered:** 1 is **A** (Thomas, 17:21: « la GI doit être présente même quand l'option RR est sur off. Et une fois la GI en place,
son équivalent raster est supprimé »): the native denoiser for the frames without RR goes into R7c, and the baked ambient and the
SSAO are removed once the GI serves every traced frame (D-0251). 2 is SHaRC (17:18). The denoiser of option A (17:22-17:27, the
pilot's question how other games do it, Thomas: « Va pour NRD comme solution de base, et le RR comme solution optimale »): **NVIDIA's
NRD** (ReBLUR or ReLAX, as the RTX Remix remasters and Cyberpunk 2077 without RR: general knowledge), not a native one; downloaded
at R7c's start, outside the repository as SHaRC and Streamline, NVIDIA's notice with the distributed module.

1. **The GI without Ray Reconstruction** (RR is a player option at the RT level, D-0247). One ray a pixel is noise without a denoiser.
   - **A (recommended): a native denoiser for those frames** — a temporal accumulation along the motion vectors with a history
     reset on disocclusion, then a few à-trous passes guided by the depth and normals (ER's AO denoiser's shape, §2); one more piece
     of R7c. The baked ambient and the SSAO then go entirely (D-0251).
   - B: with RR off, no GI: the baked ambient and the SSAO stay for that option only (a raster path kept beside its traced
     equivalent, against D-0251's rule).
   - C: RR becomes required at the RT level too (the « Off » option removed from RT-PT-PLAN §6).
2. **SHaRC** (§3): **decided** (Thomas, 2026-11-06 17:18: « Prends SHARC »), downloaded, outside the repository.

R7a needs neither answer (it runs only with RR and has no cache).

## 6. Checks before the code depends on them

- That no G-buffer pixel carries stencil bit 0x10 (one stencil read of a frame): else GB1.z masks the ambient on those pixels, and
  the GI must take that mask (STUDY-R7-GI §4c, VERIFIED statically, TO CHECK live).
- That the lightmaps hold no direct sun (a lightmapped surface in full sun against one in its map's shadow, their GB4 compared:
  STUDY-R7-GI §4b) before the seed is used.
- Whether Streamline's RR takes a diffuse hit distance (its guide), else the GI's noise is denoised from the albedo and normals
  alone.
- Measure, not by eye (2026-11-04): every brightness claim from signalstats on the same crop; a zone much darker or brighter than
  the baked ambient is an anomaly to explain (D-0249 judges a true light, not equality).

## 7. R7c as designed (2026-11-07, the R pilot)

**NRD, its source and its build** (OBSERVED: the clone and its CMake, `Source/InstanceImpl.h`, `Shaders/Shaders.cfg`). NVIDIA's NRD
v4.17.3 (`https://github.com/NVIDIA-RTX/NRD`, tag of 2026-04-30, chosen over v4.18.0 of 2026-10-30 as the older; « NVIDIA RTX SDKs
LICENSE », SHaRC's), cloned to `C:/Users/User/Tools/DS3-Decompilation/third-party/nrd-4.17.3-download/NRD` (9.6 MB), with the two
source archives its CMake pins: MathLib v11 (`mathlib-v11.zip`, 94,527 bytes, SHA-256 4A077210…0A28A06) and ShaderMake `18f5a344`
(`shadermake-18f5a344.zip`, 38,124 bytes, SHA-256 35DE2547…10623B6), both MIT. NRD makes no graphics call: it describes its
pipelines, its texture pools and each frame's dispatches (resources, constants, grid), and the engine records them; its shaders
need not be embedded, each pipeline then names its programme and permutation (`shaderIdentifier`, « for custom integrations »). So
the module's build compiles NRD itself, as it reads SHaRC: the library's C++ (`Source/`, static, no embedded shaders) with MathLib's
and ShaderMake's headers, and the programmes REBLUR_DIFFUSE uses with the Windows SDK's dxc, hash-pinned, never in the repository
(D-0252); no CMake, no ShaderMake executable. CMake's own build was refused by Claude Code's guard (it downloads and runs code):
asked to Thomas (2026-11-07 06:10).

**The frame** (a GI frame RR does not denoise; RR's frames unchanged). The denoiser: REBLUR_DIFFUSE (one diffuse signal with its hit
distance; RELAX is NRD's choice for ReSTIR's signals, R7d's question). After GXLightCmbAlbedo as today:

| Pass | What it does |
|---|---|
| `gi_signal` | the GI's ray as `gi`'s, writing NRD's inputs instead of adding: the radiance demodulated (L, before GB2 × (1 − F0)) with its hit distance normalized (`REBLUR_FrontEnd_PackRadianceAndNormHitDist`); the normal and roughness (`NRD_FrontEnd_PackNormalAndRoughness`, GB0 and GB1's gloss); the view Z; the motion (below) |
| NRD | its dispatches (classify, pre-pass, temporal accumulation, history fix, blur, post-blur, stabilization) at the render size, its pools the stage's own (UAV barriers, out of the frame graph, as the cache's) |
| `gi_compose` | GB2 × (1 − F0) × the denoised radiance added into CmbAlbedo, where `gi` adds today |

The motion NRD needs is the G-buffer surface's, at the lighting, where the velocity is not drawn yet (it follows the forward and
effect rows, NATIVE-PASS-ORDER 11b). The Velo rows move before the lighting (after the decals, every frame: one place); the resolve
stays where it is (its camera motion from the final depth, the forward rows' and the water's included, for DLSS and the motion blur);
`gi_signal` decodes the G-buffer surface's motion with the resolve's formula. What moves: a Velo row behind a forward row that writes
depth (a phantom, the water surface) now writes its object's motion there (INFERRED small: the forward rows are thin or static).
NRD's camera: the unjittered projection and the pose of DLSS's evaluation, the previous frame's kept by the renderer; its history
restarts on the velocity's reset, a size change, and after a frame without the NRD GI (RR's frames, a missed frame).

**Then** RtGiFrame drops RR: the GI runs on every traced frame (RR, else NRD); a frame NRD cannot serve (its stage not open, a
failure latched) keeps the baked ambient, the frames without the rays' fallback, and is counted (`nrd_missed`; R7c's final review M1). Then D-0251's removals (§4): the SSAO's passes,
pipelines, programmes and history (the AO stage keeps the mask target), pass 0's GB4 left only on the frames without the rays,
`no-rt-gi.flag`.

## 8. R7d as designed (2026-11-07, the R pilot)

**The frame challenge.** Sought: the PT level's diffuse bounce with less noise for the same rays, each pixel reusing the second
vertices its neighbours and its history found (ReSTIR GI, Ouyang et al., HPG 2021: general knowledge). Engine invariant: the GI's
first ray and its hit's radiance (`GiSample`, `HitRadiance`, the cache); RR the PT level's denoiser (RT-PT-PLAN §6: RR mandatory at
the PT level, so NRD and its RELAX never see a PT frame: STATE's RELAX note answered, nothing to choose there). Redesigned: which sample
a pixel shades. ER has no analogue (§2: its rays trace occlusion only).

**Its source** (Thomas's choice, asked 2026-11-07 09:56): **A** NVIDIA's RTXDI library, its GI part (`Include/Rtxdi/GI/`: the
reservoirs, the temporal and spatial resampling, the Jacobian, the bias correction by rays, the boiling filter; header-only HLSL), at
the commit RTXDI v3.1.0 pins (`NVIDIA-RTX/RTXDI-Library` `f12037fa`, 2026-09-03; the « NVIDIA RTX SDKs LICENSE », SHaRC's and NRD's),
outside the repository and hash-pinned by the build as SHaRC's headers, the native writing only its bridge functions (`RAB_*`: the
G-buffer surface, the rays, the target); **B** written natively (R4-PLAN §7's lesson: visibility in Z); **C** left to R8.

**A PT GI frame** (`rt-level.txt` « pt » or `rt-pt.flag`, a frame RR denoises, the GI's gates): in place of `gi`'s one draw,

| Pass | What it does |
|---|---|
| initial | `gi`'s cosine ray as the initial sample: its hit's position and geometric normal and its radiance toward the pixel (`HitRadiance`; a miss a point at RT_REFLECT_FAR along the ray with the sky's or `Unmet`'s radiance), its source pdf cos θ / π, the target p̂ = luminance(L) × cos θ (Lambert's albedo constant over the pixel) |
| temporal | the previous frame's reservoir at the pixel's reprojection (the camera's motion alone, the depth through the velocity's current-to-previous rows: the Velo rows are not drawn yet on RR's frames; a moving object's history fails the surface tests and is not reused), taken where the previous surface's depth and normal agree (the surfaces kept a frame in a buffer of their own), its M capped at 8, a sample past 30 frames dropped (its radiance is not traced again) |
| spatial | a few neighbours within a radius, the same agreement tests, each neighbour's sample reconnected to the pixel's surface with its Jacobian; the bias correction by rays: Z counts only the neighbours whose surface sees the chosen sample (one ray each; R4c's darkening beside the shadows came from a Z over the unshadowed target) |
| shade | one visibility ray from the pixel to the chosen sample (occluded: nothing), GB2 × (1 − F0) × L cos θ / π × W (the reservoir's weight) added into CmbAlbedo where `gi` adds; RR denoises the lit HDR as today |

**Biases that stay by design, said** (not hidden): a reused sample keeps the radiance it was traced with (a moving light, a cache
still filling: a lag bounded by the age cap; zero on still views); the hit's view-dependent specular reused toward a neighbour (a few
centimetres from the pixel that traced it); RTXDI's ray-traced normalization evaluates each domain's target without that domain's
Jacobian (the step review's M3) and counts a domain by its own sample's Jacobian against the window, not the chosen sample's (the fixes'
review M-1): both INFERRED from the headers; the window [0.5, 2] by default, [0.1, 10] with rt-gi-jacobian-wide.flag, the check run with
both; a bias found there is fixed by the native normalization (the chosen sample's Jacobian toward each domain), written then. The receiver everywhere is
the point the rays leave from (the surface moved toward the eye by the shadow rays' offset; the step review's M2).

**The bias check, before the reuse stays** (R4-PLAN §7's lesson): `rt-gi-mean.flag`, a diagnostic. On a still camera each GI
frame adds its pixel's estimate of L (the demodulated radiance the pixel's GI adds, before GB2 × (1 − F0)) into a float
accumulation, restarted when the camera's rows or the size change, written beside every world picture (`-gi-mean.rgba32f`: the
sums in rgb, the frames in a). Compared on the same still views (Firelink and the High Wall for the sun's shadow edges, an interior),
at least 1,000 frames each, against the PT level's one ray (`gi` itself, the default since R7d's end; ReSTIR with `rt-restir.flag`; the step review's M1: the RT level lights
the hits' local lights unshadowed and all at once, the PT level one of them with its shadow ray, so the RT level's mean is another
integral): two of its launches give the floor (the launches' spread); ReSTIR GI without reuse the tool's own control (the same estimator,
plus a visibility ray, the reservoir's 32-bit LogLuv radiance and the surfaces' snorm normal: equal within the floor, another launch's
random numbers); then the temporal reuse, the
spatial reuse and both. It passes when each region's mean differs from the PT level's one ray's within the floor and no band follows the shadow
edges (the pixels within a few pixels of a strong gradient compared apart from the others). Else the reuse is recorded as left to R8
with why, and the PT level keeps the one-ray GI.

**R7c's open M2 in passing**: the G-buffer depth kept for the temporal test also lets the velocity resolve tell a pixel that a
forward row covered after the Velo rows (its final depth nearer than the G-buffer's): the camera's motion there, for NRD's frames and
the PT frames alike. Taken in R7d if it stays a few lines, else left recorded.

**Seen**: the PT level against the RT level on the same views (the three sizes, WORKFLOW), pictures for Thomas, and the check's
numbers.

**Outcome** (RESULT « R7d »): RTXDI's own temporal and spatial resampling measured biased (+0.3 to +5.5 %), replaced by a native
unbiased one on its reservoirs (the target with the visibility, 1/Z over the domains that could have produced the chosen sample, a
reused miss re-evaluated at its receiver); unbiased over the picture within 0.1 %, a local residual of 1 to 2 % in some blocks; its frames
less steady under RR than the one ray's. Left to R8, opt-in (`rt-restir.flag`); the PT level's GI the one ray.
As built, against the design above: no age cap (a sample-dependent drop), no visibility ray in the shade pass (the visibility is
in the target), the native 1/Z in place of RTXDI's ray-traced correction; RTXDI's reservoirs, packing and combination kept.
