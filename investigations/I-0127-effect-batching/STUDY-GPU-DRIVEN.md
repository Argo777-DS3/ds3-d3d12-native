# GPU-driven rendering for the world's geometry: what it would give the native (study thread S, 2026-11-07)

Thomas (2026-11-07, thread « Les draws d'effets regroupés »): « Les jeux modernes font quoi? » then « Oui demande une étude ».
After I-0127 (the effects' recording 0.13 to 0.18 ms, steps A and B taking 10 % of it; step C waiting for his choice), the
question moves to the world's geometry (map pieces, objects, characters): GPU culling, `ExecuteIndirect`, perhaps mesh shaders.
Three questions: what modern engines do, what the native does today, what it would really gain. **STATIC**: primary sources read
(listed in §5), the native's records (RESULTs, BOTTLENECKS; their journals for the October runs), the native tree at `main`
`a58a4f56`; then R's GPU measure of the same afternoon (I-0128 R8a) and two profiled runs of S (§4).

## 0. Answer first

- **Modern engines** move the « what to draw » decision to the GPU: a persistent scene on the GPU, culling of instances and of
  small mesh clusters (64 triangles) by frustum and by a depth pyramid from the last frame and the current one (two-phase
  occlusion), then a handful of indirect draws. The published gains are large in **draw calls and CPU**, small in **GPU geometry
  time**: Assassin's Creed Unity, « 1 - 2 orders of magnitude less drawcalls », but « Only small overall gain: <10% of geometry
  rendering » on the GPU (Haar, SIGGRAPH 2015). They all rest on one condition the native does not have: **the materials reached
  without a binding per draw** (virtual texturing at RedLynx, « No need to batch by textures! »; bindless; Nanite's own pass and
  material shading).
- **The native today, rays on (measured 2026-11-07, §4):** at 1920x1080 with DLAA and RR the **GPU** limits the frame: about
  17 ms a frame on the GPU at Firelink and the High Wall, of which the rays, their cache, denoising and DLSS take about 15 ms and
  everything raster (G-buffer, forward rows, effects, YEBIS, UI) about 1.5 to 2 ms (R's `native_rt_gpu`). At 1280x720 with DLSS
  the **engine thread** limits it: m45 10.8 ms a frame (91 frames/s unlocked), the CPU never waiting for the GPU (`gpu_wait_ms`
  0.09); on that thread the native collect takes 6.0 ms, **2.0 ms of it the rays' instance walk** (3,419 instances re-read each
  frame), DS3's own work about 4.6 ms; the render worker is 72 % busy (record 4.9 ms, its driver share 1.3 ms).
- **What it would really gain:** on the GPU, GPU culling can only act on the raster's 1.5 to 2 ms (AC Unity: < 10 % of the
  geometry time), i.e. a few tenths of a millisecond at best, while the rays' 15 ms are untouched (the TLAS keeps every
  instance). On the CPU, indirect draws shorten the render worker, which is not the limit; they do not touch the engine
  thread's 10.6 ms. **Mesh shaders**: no, for now (they need every geometry programme rewritten and
  give their gain on dense meshes; DS3's are 2016-era). **The real lever** the GPU-driven idea points at is its other half, **the
  persistent scene**: a collect (and a rays' walk) that follows the engine's changes instead of re-reading the scene each frame
  (§3, option 1). On the GPU the lever is the rays' own cost (the GI 5.5 ms, the reflections 4 ms, DLSS with RR 3.3 ms at
  1080p), R's domain, not the raster.

## 1. What modern engines do (primary sources, §5)

| Engine / talk | What moves to the GPU | Measured | What it needs |
|---|---|---|---|
| **Assassin's Creed Unity** (Haar, Ubisoft, SIGGRAPH 2015) | CPU: a coarse quadtree culling and a draw-call hash that merges draws by material and render state. GPU: instance culling (frustum, occlusion), cluster expansion and culling (frustum, occlusion, baked triangle backface), index buffer compaction, then `MultiDrawIndexedInstancedIndirect` | « 1 - 2 orders of magnitude less drawcalls », CPU about 75 % of the previous AC with about 10 times the objects; GPU « 20 - 40% triangles culled », « Only small overall gain: <10% of geometry rendering », 30 to 80 % of the shadow triangles culled | meshes cut into fixed 64-vertex strips, vertices fetched by hand in the vertex shader from one shared buffer, instance data in a GPU ring (persistent for static instances); an occlusion depth from the 300 best occluders (about 0.6 ms on PS4) and the last frame's reprojected depth |
| **RedLynx** (Aaltonen, the same talk) | the whole viewport in **two indirect draws**; two-phase occlusion culling against a depth pyramid built from the depth buffer itself; virtual shadow maps culled the same way | 250,000 moving objects: GPU 2.3 ms, **CPU 0.2 ms** on one console core (Xbox One, 1080p) | **virtual texturing** (« All texture data is available at once, using just a single texture binding »), a deferred UV G-buffer |
| **Nanite** (Epic, UE5 documentation) | its own pass that « completely bypasses traditional draw calls »: cluster culling, LOD selection and rasterisation on the GPU, materials shaded afterwards | « Frame budgets are no longer constrained by polycounts, draw calls » (no CPU numbers on the page) | its own mesh format built offline; opaque and masked materials only (no translucency), morph targets unsupported, world position offset clamped, no forward rendering or MSAA; DirectX 12 SM6; ray tracing on a fallback mesh by default |
| **Mesh shaders** (D3D12 specification) | the vertex stage replaced by compute-like groups that emit meshlets; the amplification stage culls meshlets before they are expanded | none in the specification; game-level gains not found in a primary source | « Inputs to the Mesh shader don't have IA enabled », and a mesh pipeline « must not have any other shader types attached (VS, GS, HS, DS) »: every geometry programme rewritten; up to 256 vertices and primitives a group |
| **`ExecuteIndirect`** (D3D12 documentation) | the draws' arguments written by the GPU, their count from a buffer | - | per command only root constants, root CBV/SRV/UAV and the vertex and index views change; **not a descriptor table**: textures must be reachable without one (bindless) |

Common line: the CPU stops issuing one draw per object; the GPU culls what it does not need. The CPU gain comes from fewer
draws; the GPU gain from culling, mostly hidden triangles and shadow casters.

## 2. What the native does today (records; numbers predate the rays unless said)

- **Who decides what is drawn:** the engine itself. The native has no culler of its own: it runs DS3's own frustum tests (`6F310`,
  `D08190`) on a copy of the view's culler pushed out by 1.5 m (`kNativeCullMargin`); occlusion culling and GPU occlusion queries are
  not native by design (STUDY-NOT-YET-NATIVE §3); LOD and fades are DS3's (I-0100).
- **How it draws:** one `DrawIndexedInstanced` per engine draw (the engine's own instance counts), the prepared state kept between
  frames (I-0090: a row rebuilt only when its model changes, about 252 a frame at m45 and 346 on the Firelink walk), recorded in
  chunks of at least 128 draws on a 3-worker pool into one `ExecuteCommandLists` (I-0074). No indirect draw and no merging of world
  draws. At m45 the collect admits 1,686 models into 2,975 pass rows (I-0091). The sun's cascades are gone on traced frames (I-0118;
  27 local-light caster draws at the High Wall), the water's mirror view too (D-0251). The rays' scene: m45 3,479 static and 139
  skinned instances (I-0117).
- **CPU, by thread** (I-0088 BOTTLENECKS, runs `i88-20-prof-main` m45 and `i88-21-walkprof-main` the Firelink walk, 2026-10-22,
  1280x720, unlocked; then I-0090 to I-0093): the frame 7.92 / 6.17 ms (125 / 157 frames/s); **the engine thread** DS3's own work
  3.72 / 3.19 ms, the native collect 3.95 / 2.48 ms (3.39 ms at m45 after I-0093, 144 frames/s); **the render worker** prepare 3.11 /
  2.02, record 2.38 / 1.28, the driver 0.9 / 0.6 ms (its job 6.55 ms at m45 after I-0090, beside the engine thread). After I-0090 the
  frame is bound by the engine thread. The original game's main thread at m45 is 4.7 ms, its whole draw step (the D3D11 submission
  and its GX workers) 2.2 ms (I-0078). Since the rays: the rays' walk adds 1.20 ms to the engine thread at m45 (0.78 High Wall,
  0.64 Firelink; I-0117 `i117-t5l`).
- **GPU:** before the rays the GPU was never waited for (slot wait 0.00-0.01 ms). With the rays: §4 (R's whole-frame GPU measure).

## 3. What it would really gain

| Option | What changes | Gain, probably | Cost and risk |
|---|---|---|---|
| **1. A persistent scene, collected by its changes** (the CPU half of GPU-driven) | the native keeps every admitted instance (its rows, bounds, matrix slot, its TLAS instance) across frames and the collect and the rays' walk read only what the engine changed (moved, spawned, removed, LOD or fade changed), instead of re-reading the scene each frame | the collect's 6.0 ms at m45 (the rays' walk 2.0 ms of it: 3,419 instances, 182 of them skinned, the rest map pieces and objects that mostly do not move; `native_rt_walk`) is the largest native cost on the thread that limits the frame at 720p (§4; INFERRED: how much of it is a re-read of unchanged data is not measured; I-0090's rows already persist, the walk over the engine's lists does not) | medium: the engine's change signals must be found (its own dirty bits, the matrix writers); a missed change is a frozen object or a stale ray instance. ER's architecture keeps its scene data persistent too (I-0070/I-0071, D-0195); the rays' part is R's |
| **2. GPU culling + `ExecuteIndirect` for the G-buffer** | instance and cluster culling on the GPU, one indirect draw per pipeline | the render worker's record (2.4 ms) and driver time (0.9 ms) shrink, but that thread is not the limit; GPU: the hidden triangles of the G-buffer (AC: < 10 % of the geometry time) | high: DS3's programmes read their textures through tables bound per draw; an indirect run cannot change a table (§1), so it needs bindless materials, i.e. the G-buffer programmes transcribed (the rays' material table of R3 is a start, but D-0249 keeps the raster G-buffer faithful to the original's programmes) |
| **3. GPU occlusion culling only** (two-phase, the depth pyramid) | the engine's CPU selection kept, hidden instances dropped on the GPU before their draws (with predication or indirect counts) | GPU: overdraw and vertex work in dense interiors (Anor Londo, the Grand Archives); nothing for the rays (the TLAS keeps every instance) | medium: popping if the pyramid is wrong (D-0197, no pop-in); worth it only if the GPU limits the frame |
| **4. Mesh shaders / a Nanite-like format** | meshlets, an offline mesh rebuild | none at DS3's mesh density for the frame; a base for HD assets later | very high: every geometry programme rewritten, skinning, cloth, wind; RTX Mega Geometry was already set aside unless measured (D-0238) |

**Recommendation** (followed by Thomas, D-0255): no GPU-driven raster workstream: the measure (§4) leaves it a few tenths of a millisecond of GPU at best and
nothing on the thread that limits the CPU. The workstream worth opening is option 1, a persistent, change-driven collect and rays'
walk (CPU work, no change of programmes or pictures), for the 720p and unlocked frame rates and the 120-240 frames/s Thomas may
choose (I-0114); at 1080p and above the GPU's rays are the limit, R's line. Option 3 only if a dense interior shows the raster
G-buffer large on the GPU; option 4 belongs with the HD textures and assets after the finish line.

## 4. The measures (2026-11-07)

- **GPU, whole frame** (R's I-0128 R8a runs `i128-r8a2-m40-rtgpu*` and `-m30-rtgpu-1920x1080`, 16:20-16:55, the ASUS, DLAA, RR on, no
  debug layer, `native_rt_gpu` rows every 600 frames, the RT level; read by S): at 1920x1080 the main view's frame **16.7 to 17.3 ms**
  on the GPU (Firelink m40 and the High Wall m30), of which the GI 5.4 to 5.8, the reflections 3.8 to 4.3, DLSS with RR 3.3, the
  cache 1.3 to 1.4, the scene build 0.3 to 0.5, the water 0.3 (m40), the sun and the lights 0.1 to 0.2 each: about 15 ms; the rest,
  about **1.5 to 2 ms**, is everything raster. At 1280x720 (m40) the frame 8.8 to 9.0 ms, the raster's rest about 1.3 to 1.5 ms; at
  1440x810 10.9 ms. The PT levels cost more (16.7 ms at PT 1 bounce, m30).
- **CPU, by thread** (S's runs `s-gpud-m45-prof`, Epsilon standing at the m45 bonfire, and `s-gpud-walk-prof`, Eta on the Firelink walk,
  17:45-17:52, the ASUS, 1280x720 with DLSS, rays on, `unlock-unlimited` and `vsync-off`, measure mode, no debug layer, the sampler;
  `baseline.json` measure true, debug layer off; one run each, no spread between runs; the walk's path lost one key (a hold over the
  pilot's 5,000 ms limit), so part of it did not run; read by a Sonnet agent from `sample-profile-report.txt` and the pacing rows as
  I-0088 BOTTLENECKS did, checked against one pacing row by hand):

| Station | m45 now | m45 2026-10-22 (no rays) | Firelink walk now | walk 2026-10-22 |
|---|---|---|---|---|
| Frame p50 / p99 | 10.83 / 12.67 ms (91 frames/s) | 7.92 / 9.10 | 9.24 / 10.35 ms (108 frames/s) | 6.17 / 7.28 |
| Native collect (engine thread) | 6.04 ms (`collect_ms` 6.25) | 3.95 | 3.41 ms (2.77) | 2.48 |
| of it, the rays' walk (`CollectNativeRtScene`) | 2.03 ms (`native_rt_walk` 2.07, 3,419 instances) | - | 1.10 ms (514 instances) | - |
| DS3's own work and the rest of the engine thread | about 4.6 ms | 3.72 | about 3.4 ms | 3.19 |
| Render worker: prepare / record | 2.42 / 4.88 ms (of the record, the rays' encoding 1.62, DLSS 0.30) | 3.11 / 2.38 | 1.46 / 3.17 | 2.02 / 1.28 |
| Worker busy; its driver share | about 72 %; 1.27 ms | 97 %; 0.9 | about 56 %; 1.41 | 92 %; 0.6 |
| The CPU waiting for the GPU (`gpu_wait_ms`, slot wait) | 0.09; 0.00 | 0.07; 0.00 | 0.07; 0.00 | 0.07; 0.00 |

- **Reading:** at 1280x720 the frame is the engine thread's (it never waits for the GPU, the worker has room); at 1920x1080 with DLAA
  the GPU's 17 ms are above both threads. The raster share a GPU culling could reduce is 1.5 to 2 ms of GPU; the native costs on the
  engine thread that a change-driven scene could reduce are the collect's 4 ms and the rays' walk's 2 ms at m45 (not all of it
  avoidable: the moving instances, the engine's own lists). The comparison with October is not like for like (no rays then).

## 5. Sources read

- Haar, Aaltonen, « GPU-Driven Rendering Pipelines », SIGGRAPH 2015 Advances in Real-Time Rendering:
  https://advances.realtimerendering.com/s2015/aaltonenhaar_siggraph2015_combined_final_footer_220dpi.pdf (text extracted from the
  slides; the quotes are its « Results », « Benchmark » and « GPU-Driven Rendering with VT » slides).
- Epic, « Nanite Virtualized Geometry » (UE 5.8 documentation):
  https://dev.epicgames.com/documentation/en-us/unreal-engine/nanite-virtualized-geometry-in-unreal-engine. Karis's SIGGRAPH 2021
  slides (16 MB) were not read: their size refused the fetch.
- Microsoft, D3D12 Mesh Shader specification v0.86: https://microsoft.github.io/DirectX-Specs/d3d/MeshShader.html (partly through a
  summary of its second half).
- Microsoft, « Indirect drawing » and `ExecuteIndirect`: https://learn.microsoft.com/en-us/windows/win32/direct3d12/indirect-drawing,
  https://learn.microsoft.com/en-us/windows/desktop/api/d3d12/nf-d3d12-id3d12graphicscommandlist-executeindirect.
- Not found in a primary source: Frostbite's GDC 2016 cluster culling numbers, any game's measured mesh-shader gain.
- The native: I-0074, I-0078, I-0088 BOTTLENECKS, I-0090, I-0091, I-0093, I-0100, I-0106, I-0117, I-0118, I-0127 RESULTs;
  `09_model_admission.inc`, `05_selected_scene.inc`, `renderer/gbuffer_encoder.h`, `renderer/effect_stage.cpp`.
