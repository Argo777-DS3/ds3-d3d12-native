# R2, « La scène en rayons » : the plan prepared ahead

Study thread, 2026-10-30 (Thomas's « Oui go », 09:45 Paris, relayed): R2 of [RT-PT-PLAN](../../.project/RT-PT-PLAN.md) §8 prepared
ahead, read-only, for the pilot who opens it after R1, the cadence and the unlocked frame rate (D-0240). Inputs: this
investigation's [RESULT](RESULT.md) (DS3's geometry, §2-§4), R0's base ([I-0109](../I-0109-modern-dx12-base/RESULT.md)), D-0236
(Elden Ring's RT observed for its place in the engine only), D-0237 (R5 right after R2), D-0238 (standard DXR, measured). Labels
as in RESULT. The pilot copies this file into R2's investigation in its first commit (WORKFLOW « Canonical sources »).

## 0. In short

- **Goal of R2**: every surface the game has loaded around the player is in a DXR scene each frame (static map pieces, objects,
  characters skinned, cloth, physics objects, alpha-tested foliage), checked by a traced view against the raster on the same
  camera, with the costs D-0238 asks measured. Nothing visible changes in normal play (behind a flag until R5 uses it).
- **What R0 gives**: a 12_2 device with DXR 1.2 and `device5`, dxc building DXIL at build time, inline ray queries proven inside
  DS3 (`native_rt_selftest`), a bindless heap of 2^18 slots (its per-frame rings left to R2).
- **What DS3 gives** (RESULT §2): one GPU buffer per model with per-mesh slices, float3 positions at byte 0, triangle lists, R16
  or R32 indices, the LOD-0 face set the native already selects, a 3x4 row-major placement per draw (the DXR instance layout),
  CPU skinning palettes with the previous pose, generation counters on cloth and physics streams, engine categories per walk
  (main view, sun casters 0x02, local casters 0x04, reflection 0x10).
- **Six tranches** (§3): T0 the BLAS registry at admission; T1 the loaded-world instances and the TLAS; T2 skinned and deformed
  geometry; T3 the alpha-tested geometry; T4 the traced control view and its comparison with the raster depth; T5 the measures
  and the Mega Geometry verdict (D-0238). R5 then adds the sun's shadow rays on this scene.
- **Thomas's choice** (§5, 2026-10-30 15:41): the rays see everything the engine has loaded.

## 1. Where the RT world sits (the architecture)

- **An RT world owned by the renderer, apart from the camera's lists** (the place Elden Ring gives it, D-0236: `GXRtWorld`, models
  registering their BLAS instances world-wide; its workings, the 2,000-instance TLAS cap and the DispatchRays path, not copied).
  DS3 places 4,057 map pieces at Firelink alone (RESULT §2.2): the TLAS takes every instance, no cap below the scene.
- **Inline ray queries** (`RayQuery` in compute), as R0's self-test: R5's shadows, R6's reflections and the control view are
  compute passes that read the G-buffer and trace; no state objects or shader tables (a DispatchRays path only if a later
  workstream proves it needs one).
- **In the frame graph**: a `rt_scene` node after the collect (CPU: instance list) and before the G-buffer's consumers (GPU: BLAS
  builds and refits, then the TLAS build, on the direct queue first; the async compute queue a later optimisation, measured).
  The node is one declaration and one bit of the key, as `light_organize` (RT-PT-PLAN §2).
- **Two frames in flight** (D-0187): two TLAS buffers and two instance-description rings, one per frame slot; BLAS shared, retired
  by the fence of the last frame that referenced them (the model storage's own rule, `RetireWhenSubmittedWorkCompletes`).

## 2. The data paths (from DS3, OBSERVED unless marked)

| Need | Source in the native | Note |
|---|---|---|
| Vertex positions | the model storage buffer, slice `vertex_offset`, `stride`, `vertex_count` (`content.h:10-27`) | `R32G32B32_FLOAT` at byte 0 (`content.cpp:39-41`); the buffer moves to non-pixel shader resource for the build |
| Indices | same buffer, `index_offset`, `index_count`, `index_width` 2 or 4 | slice-local, no base vertex |
| Which face set | the LOD-0 one already selected (`game_scene_geometry.cpp:62-64`) | LOD 1/2 BLAS only if T5 asks |
| Two-sided | `slice.cull_class` 0 (11-14 % of map meshes) | geometry or instance flag `TRIANGLE_CULL_DISABLE`; the winding convention checked on one mesh in T0 |
| Placement | `NativeSelectedModel::placement` 3x4 row-major | the DXR instance transform as is; the previous one kept for motion (RESULT §2.1: not kept today) |
| Skinned positions | pose palette rows b5 (and b12 cancel), BLENDINDICES `R8G8B8A8_UINT`, BLENDWEIGHT `UNORM8` from the layout | compute skinning reproduces the vertex shader's `b5 · b12 · v` |
| Cloth, physics | `NativeClothVertexUpload`, `UpdateNativePhysicalModelStreams`, their generations | a refit when the generation changes |
| Alpha test | `ModelMaterialPolicy::alpha_ref` (`model_parameters.cpp:483`, the maximum of the MTD's `g_AlphaRef` words), the diffuse texture's alpha | `_e` materials, ref 128 (RESULT §2.2) |
| Categories | the walks' masks: main policy `view+0xE0`, 0x02 sun casters, 0x04 local casters, 0x10 reflection | the TLAS instance mask mirrors them (§3 T1) |
| Loaded world | the engine's world layers through its own traverser with a forged culler, as the shadow walks already do (`CollectNativeShadowCasters`, `12_scene_collect.inc:598-623`: planes `+0x160`, box `+0x220/+0x230`) | a large box around the player instead of the camera's frustum |

## 3. Tranches

Each tranche behind `rt-scene.flag` (off by default), unit tests, a step review; the start check after the merge (D-0229). Live
runs announced (WORKFLOW « Live game rule »), never during the DLSS pilot's slots.

**T0, the BLAS registry (no frame cost).** At each model storage admission (`AdmitModelStorage`, after its slices are published),
one BLAS per mesh slice (`PREFER_FAST_TRACE`, `ALLOW_COMPACTION`), built on the ingress list that already copies the buffer,
then compacted (post-build size query, copy, release of the large one) a few frames later; released with the storage. Rows:
`native_rt_blas` (count, triangles, memory before and after compaction, build ms on the GPU). Exit: every admitted map model of
Firelink and the High Wall has its BLAS; the memory per zone recorded (RESULT §4 estimated about 1 GB for 15-16 M triangles); no
frame-time change measured with the flag on (PresentMon, the ASUS).

**T1, the instances and the TLAS.** A loaded-world walk with the forged culler (a box of the chosen radius, §5), recording per
entity its placement, its storage's BLAS and which categories admit it (one walk with the union mask and the entity's own
category bits, or the existing walks' results merged: the pilot measures both). Static map pieces are cached between frames and
re-walked only on an area load or unload (the engine's world-layer changes); moving entities (characters, objects whose
placement changed) are refreshed each frame from the main walk and a dynamic list. Instance mask bits: 1 camera-visible, 2 sun
caster, 4 local caster, 8 reflection-visible (the MSB's `IsShadowOnly` and `DrawOnlyReflectCam` parts come out as 2-only and
8-only through the engine's own categories, RESULT §2.2, to confirm on these bits). The TLAS rebuilt each frame
(`PREFER_FAST_BUILD`). Rows: `native_rt_tlas` (instances, build ms, walk ms). Exit: at Firelink every MSB map piece of the loaded
areas in the TLAS (the count against `msb-m40.tsv`); the walk and the build under 1 ms CPU and 0.5 ms GPU at Firelink and m45
(INFERRED targets, to set by the pilot from the measure).

**T2, skinned and deformed geometry.** A compute skinning pass per visible-or-near character into a per-instance position
buffer (positions only; normals stay the raster's), then a BLAS update (`ALLOW_UPDATE`, `PERFORM_UPDATE`) every frame and a
rebuild when the mesh set or LOD changes or after N updates (quality drift); cloth and physics streams refit on their
generation; the wind's grass in its rest pose first (RESULT §2.3, judged on sight in R5; since T6 moved by its wind, RESULT « T6 »). Rows: refits a frame, their GPU ms.
Exit: Firelink's NPCs and the player (Alpha) skinned in the rays, the pose matching the raster's on the control view (T4).

**T3, the alpha-tested geometry.** The `_e` materials' geometries marked non-opaque; in the ray query, candidate triangles take
the diffuse texture's alpha at the hit's UV against `alpha_ref` (`CommitNonOpaqueTriangleHit`). It needs a per-geometry record
(UV stream offset and format, the diffuse texture's bindless index, the ref): the seed of R3's material table, kept minimal.
Opacity micromaps (DXR 1.2) only as a later optimisation if T5 shows the alpha test costs. Exit: the High Wall's foliage and
fences cut out in the control view as in the raster.

**T4, the control view (the check of R2).** `rt-view.flag` (a test tool, never in the menu): a compute pass casts the camera's
primary rays through the TLAS and writes, by key: the instance colour, the geometric normal, the diffuse albedo (T3's record),
the hit distance. **The check is numeric**: the traced depth against the raster depth buffer at every pixel, the share of pixels
within a small tolerance, per zone (the pixels where the raster draws transparents and effects excluded): a missing object, a
wrong transform, a skinning error or a missed alpha cut shows as a disagreement. A first sun-shadow ray from the G-buffer's
positions is drawn there as R5's preview. Exit: at least 99 % of opaque pixels agree at Firelink, the High Wall and m45 (a
threshold the pilot sets from the first measure), each disagreement explained. (2026-11-02: T4 merged as the tool with Firelink
at 99.6 % and the High Wall at 95.4 %, m45 not run; this exit moves to T5, with the sky model out of the rays: RESULT « T4 ».)

**T5, the measures and the verdict (D-0238).** On the witness zones (Firelink, the High Wall, the Settlement, m45): BLAS memory
compacted, the per-frame refit and TLAS ms, the walk's CPU ms, LOD switches (none planned while R2 keeps LOD 0), a camera turn
and a zone travel for hitches. Mega Geometry comes back only if one of them weighs; the layer keeps one interface for another
backend. Written in R2's RESULT for R5.

## 4. Risks and how each is caught

| Risk | Caught by |
|---|---|
| An object missing from the rays (a walk or category miss, an unadmitted family) | T4's depth comparison; the T1 count against the MSB |
| BLAS memory beyond the budget (two zones while streaming) | T0's rows; LOD 1 BLAS for far instances or a radius cut (§5) |
| The walk's CPU cost (the engine is CPU-bound, I-0095) | T1's static cache and per-frame dynamic refresh; the measure |
| Skinning mismatch with the raster (palette, cancel, layout) | T4 on characters; the vertex shader's formula reused |
| Alpha test cost | T5; opacity micromaps then |
| A BLAS freed while a frame in flight uses it | the storage's retirement rule (fence of the last referencing frame) |
| Encrypted engine code on the path (the walk's traverser) | the shadow walks already call it natively (`0x234620`) |
| An RT pass disturbing the raster's pipelines: the open-gaps thread (I-0113, relayed 2026-10-30 14:47) traced the dark areas' banding to the driver's pipeline library combined with R0's start self-test: with the library loaded and the self-test on, the grading read its LUT with point sampling (its static sampler lost); with `no-rt-selftest.flag` the picture is smooth; D-0241 removes the library (its merge pending) | before T0's merge, a run with `rt-scene.flag` on and off compared on the grading and the other static-sampler passes in lossless readbacks (not the run's JPEG pictures, LIVE-RUN-NOTES), with and without a pipeline library if one still exists; the RT pipelines created after the raster's, or in their own library |

## 5. The choice for Thomas (made 2026-10-30 15:41: everything loaded)

Thomas (project chat, relayed): « 1. Tout. »: the loaded-world walk of T1 takes everything the engine has loaded, no radius;
a cut (LOD 1 BLAS for far instances, a box) only if T0-T1's measures show memory or walk time weighs, put to him then.

The question as put:

**The radius of the loaded world in the rays.** Everything the engine has loaded (the whole area, the most faithful for
reflections and bounce, the most memory and walk time), or a box around the player (for example 300 m, enough for shadows and
most reflections, cheaper). Recommendation: everything loaded, measured in T0-T1, a box only if the measure asks; it is what
removes « an object missing from a reflection » at the root (RT-PT-PLAN §12 risk 2).

## 6. Not in R2

Materials beyond the alpha test (R3), lights (R4), shadows (R5), reflections (R6), GI (R7), denoising (Ray Reconstruction, with
R3), LOD selection for the rays, opacity micromaps and shader execution reordering (SM 6.9, an Agility SDK later).
