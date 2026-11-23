# I-0068 part BB: the whole YEBIS chain as a D3D12 design, in the frame plan with the AO and glow passes (static)

Request (coordinator, 2026-10-11): a D3D12 design of the whole YEBIS chain (exposure, glare and bloom, depth of field, motion blur, tone map, anti-aliasing), starting from [YEBIS-CHAIN-CHECKLIST](YEBIS-CHAIN-CHECKLIST.md) (P1-P24), looking at what native already does against what is missing, fitting the frame plan of phases A to G with the AO ([SSAO-D3D12-DESIGN-NATIVE](SSAO-D3D12-DESIGN-NATIVE.md)) and glow ([GLOW-CHAIN-NATIVE](GLOW-CHAIN-NATIVE.md)) passes, inspired by ER (D-0157); witnesses in the five zones and live rows. Nothing was launched, native code was not changed.

Labels: **STATIC**, **OBSERVED** (the retained I-0053 / I-0063 captures as cited by the notes), **INFERRED**, **OPEN**. Sources are the notes named in the checklist (YEBIS-NATIVE, YEBIS-HAL-READY, YEBIS-TECHNIQUES, TONEMAP-SKIN, DISPLAY-SWAPCHAIN-NATIVE, VELOCITY-MOTIONBLUR-NATIVE and the I-0067 motion-blur note) plus the native sources at `cda2c1a7` (`renderer/scene_pass.cpp`, `frame/game_bootstrap.cpp`). This note adds the execution model; it does not redo the stage arithmetic.

## 0. Answer first

1. **Keep the engine's library; change how its draws are executed.** The checklist's route stands (call the YEBIS library on a native `IGPUDevice`; no stage is re-implemented in shaders). D-0157 changes what the native device *is*: today's design (YEBIS-HAL-READY §5, §6) is a D3D11-shaped immediate device that issues a barrier, a PSO, a descriptor copy and a draw at every `Draw` call. The design here makes the device a **recorder**: each library draw becomes a typed `YebisDrawRecord`, the frame's records form an immutable `YebisFrame`, and a native planner turns it into batched barriers, aliased transient targets, bulk descriptor tables and one encode. The library's CPU code and its draw order are unchanged.
2. **Native has none of the chain.** The in-game path ends with a plain sRGB encode of the lit HDR (`composition_pso_`, `scene_pass.cpp:355-381`): no exposure, tone map, glare, depth of field, light shafts, grading, motion blur, anti-aliasing, glow or gamma pass. What native already has and the chain reuses: the lit HDR and GB5, the readable depth rule, the velocity targets and their motion-vector resolve (Native1186-1192), the quality object (Native1255), the gparam banks and the tone-map key witness (`native_tonemap_inputs`), and the UI and presentation path (§2).
3. **The frame is 11 plan groups after the lit HDR**, 36 to 48 library draws plus 15 GX draws per frame (§3). With the record list, the **library draws of one frame cost one encode and one batched set of barriers**, the temporal resources are plan resources with cross-frame edges, and the transient pyramids alias (§4).
4. **Three design improvements over the D3D11 shapes**, each checked against the original's data: the LDR target becomes a ping-pong pair (no `PointSampleColor1` copy before the anti-aliasing and glow); the scene copy of the library input folds into the plan as one explicit pass with its clamp; the persistent glare, afterimage and DoF-table resources are declared with cross-frame edges instead of conventions (§4.2).
5. **AO and glow slot into the same plan** with the same binding model: AO before the lighting (A1-A5 of the AO note), glow after the anti-aliasing (§3, §4.4); the shadow-mask, AO, YEBIS, anti-aliasing, glow and gamma passes share one post-pass root signature family.
6. **Order and witnesses**: fourteen steps, each with its check on the five zones (m31, m37, m45, m50, m51), and live rows YB1 to YB16 (§6).

## 1. What the chain is and what native has (STATIC + the code at `cda2c1a7`)

| Stage (checklist) | Draws (zone) | Library or GX | Native today |
|---|---|---|---|
| P1-P4 driver: init, globals block from the gparam banks, about 80 setter calls | CPU | engine functions `E2B20`, `E42A0`, `E4460`, `E4070`, `E4B50`, `E69A0` | not run (no `CPostEffect`, no HAL) |
| P5 LUT copy (`LutCopy2DTo3DCs`, compute, 16³) | 1 dispatch when grading is enabled | GX | absent |
| P6 scene copy (`PointSampleColorClamp1`) into the library input (RGBA16F) | 1 | GX | absent (the lit HDR is encoded directly) |
| motion blur (before DoF, option) | +2 | library, native velocity as input | velocity targets and resolve exist; no consumer |
| P7b depth of field | 10 at every zone (1 + 2 + 6 + 1) | library | absent |
| P7c glare, bright pass, bloom levels | 11 to 23 | library | absent |
| afterimage | 6 | library | absent |
| light shafts | 6 where enabled | library | absent |
| glare combine and final glare resample | 3 | library | absent |
| P7d tone map | 1 | library (`TonemapHDR_[Dither_][Vignette_]Exposure_[Matrix_]Gamma`) | **replaced by an sRGB encode**, no exposure |
| P7e colour grading | 1 where enabled | library | absent |
| P11 DoF table (`CustomDofLutCopy`, compute) | 1 dispatch | GX | absent |
| P12-P13 after-chain: `PointSampleColor1` copy, anti-aliasing (DLAA) | 2 | GX | absent |
| P14 glow: `EmissiveToGlowPS`, `Sample13Weight` ×4, `GlowCompositeSinglePS` ×5 | 10 | GX | absent (GB5 is written) |
| UI | n | native | exists |
| P16 gamma pass and `Present` | 1 | native, to build | plain presentation |
| menu views without YEBIS (P23) | the GX tone-map filter | GX | the GX tone map for modes 0 and 7 with UseTable (written, seen live in mode 0 (I-0113 tranche 27 after the merge: the Quit Game portrait's face matched, mode 7 not yet seen); I-0113 RESULT « Tranche 27 »), sRGB encode for the other modes |

Per frame: m31 48 library draws, m37 36, m45 36, m50 41, m51 36 (m40 41); glare base 210×177 (m31, m37, m45, m51) or 392×331 (m50, m40); final glare 420×354 or 640×540 (checklist §5). All sizes are derived by the library from the native scene size (D-0151).

## 2. The execution model (D-0157)

### 2.1 Recorder instead of an immediate device

The library calls the device through 77 slots; 29 are native (YEBIS-HAL-READY §2). In the recorder model the native slots do not touch D3D12 at all during the library's CPU pass:

| Slot group | Records |
|---|---|
| render target, depth, viewport, scissor (6, 7, 15-18) | current-state fields of the recorder |
| render-state index, sampler mode (40, 50, and the CALL slots that only store them) | fields of the current record |
| `Clear` (52) | a `Clear` record |
| `DrawPrimitive` / `DrawIndexedPrimitive` (57, 58), after the effect commit (the `CTinyEffect` slots 23/24) | a **`YebisDrawRecord`** |
| creators 60-67 | **virtual resource requests** (size, format, usage, flags, wrapped native resource or not): the handle carries a stable id; the native resource is bound when the plan is compiled |
| vertex and index buffers (68, 69) | ranges of the frame arena filled by the library's own writes |

`YebisDrawRecord` = {technique (VS/PS blob ids), input layout id, render-state index (blend, write mask, scissor), up to 8 RT ids + DSV id, viewport and scissor, SRV ids t0..t16 and sampler modes s0..s15 (the pairs the bound textures request), the `$Globals` bytes (only up to the highest used offset), vertex range and draw arguments, stage tag}. The records are typed operations above the encoder; barriers are *not* part of them.

### 2.2 What the ER documentation contributes

The ER folder holds no study of its post-process stages. What the review documents and this design takes over (architecture only, D-0060): operation records built above the native encoder, barrier states carried as data and issued in groups (ER E8), per-draw work proportional to what changed (E10), and fence-gated reuse of upload space (E4). Nothing about ER's image processing is used as DS3 evidence.

### 2.3 Plan and encode

1. **Record** (owner thread, inside the engine's frame step: the library's CPU code and the gparam reads stay on the engine thread, review R13/R8b): produces the `YebisFrame` and the GX-step records (LUT copy, scene copy, DoF table, anti-aliasing, glow).
2. **Compile**: the planner derives, per resource, the first and last use in the frame, the required state at each draw (RT while written, `PIXEL_SHADER_RESOURCE` while sampled, UAV for the compute steps), the batched barriers between *groups of consecutive draws that do not depend on each other*, the aliasing of transient targets with disjoint lives, the descriptor ranges (one table of up to 17 SRVs and 16 samplers per draw, written into the frame-linear heap in one pass), and the PSOs needed (a miss is created off the render path, review R9).
3. **Encode** (render thread or worker): one command-list section; no per-draw state queries.

Per-draw state that D3D11 re-sent (heaps, root signature) is set once per section. The `FramePlan` extension of review R10 (states per use, subresource ranges, aliasing barriers) is the prerequisite; its pass cap (128) must be raised for a frame that carries about 48 + 15 + 5 + 20 draws as nodes (the plan treats each library draw as a node of a *draw graph* inside a pass group, not as a top-level pass).

## 3. The frame plan, from the G-buffer to the presentation

Groups in execution order; **bold** = added by this note or by the AO/glow notes, normal = existing native work. Numbers follow NATIVE-PASS-ORDER.

| # | Group | Work | Reads | Writes | Notes |
|---|---|---|---|---|---|
| 3 | G-buffer | six targets, depth, stencil | | GB0-GB5, depth | native |
| 4 | copies | readable depth, GB0/GB1 copies | depth, GB | `DepthRead`, copies | shared resource of the AO, forward, effects, DoF |
| 6a | **AO** | A1-A5 of SSAO-D3D12-DESIGN-NATIVE | `DepthRead`, GB0, rotation, `AoHistory[prev]` | `AoHistory[cur]`, **t33** | gate: quality HIGH/MAX |
| 6b | shadow mask + composite | GX passes | t33, depth, atlas | t32 | SHADOWS-NATIVE |
| 7-8 | env regions, lighting | native | t32, t33, GB | lit HDR | native |
| 9-11 | forward, water, effects | native | | lit HDR (+ GB5) | effects phase |
| 11b | velocity | native (Native1186) | | velocity, motion vectors | option MotionBlur |
| **Y0** | **LUT copy** (when grading is enabled) | compute 2D → 3D, 16³ | 2D LUT | `Lut3D` (UAV → SRV) | P5 |
| **Y1** | **scene input** | `PointSampleColorClamp1`: copy and clamp the lit HDR into the library input | lit HDR | `YScene` (RGBA16F) | P6; one explicit pass |
| **Y2** | **motion blur** (option) | library stage, +2 draws | `YScene`, velocity, depth | `YScene` copies | before DoF unless the library moves it |
| **Y3** | **depth of field** | 10 draws: depth factor into the scene alpha, source, aperture, composite | `YScene`, `DepthRead`, `GlareFinal[prev]` (I3), `DofTable[prev]` (I4) | `YScene`, DoF layers (transient) | every zone |
| **Y4** | **glare and bloom** | source, bright pass, bloom levels, combine | `YScene` | pyramid (transient), glare | 11-23 draws |
| **Y5** | **afterimage** | blur pairs and history | `AfterHist[prev]` | `AfterHist[cur]` | persistent ping-pong |
| **Y6** | **light shafts** (where enabled) | mask, 4 radial, 1 | depth, grain | radial pair (transient) | m31, m50 observed |
| **Y7** | **combine and final glare** | `MadLumGammaT<k>`, resample | pyramid, shafts | `GlareFinal[cur]` (I3) | persistent |
| **Y8** | **tone map** | one draw | `YScene`, `GlareFinal[cur]`, exposure, grain | `Ldr[A]` (or a grading input) | the first image |
| **Y9** | **grading** (where enabled) | `MadColorGradingLutT1_S1` | tone-mapped, `Lut3D` | `Ldr[A]` | m31, m40 observed |
| **Y10** | **DoF table** | `CustomDofLutCopy` compute | scene depth | `DofTable[cur]` | read next frame |
| 13a | **anti-aliasing** | DLAA (type 4) | `Ldr[A]` | `Ldr[B]` | replaces copy + DLAA |
| 13b | **glow** | extract, 4 blurs, 5 composites | GB5, `Ldr[B]` | `Ldr[B]` (additive) | GLOW-CHAIN |
| 14 | UI | Scaleform | `Ldr[B]` | `Ldr[B]` | native |
| 15 | **gamma + present** | brightness curve | `Ldr[B]`, curve | back buffer | DISPLAY-SWAPCHAIN §9 |

Menu views (no YEBIS, pipeline bit 10 clear) take the GX tone-map filter then the UI: group Y8 collapses to the GX filter with the Tone Map bank's second block as its inputs, not a fixed exposure (checklist P23; I-0113 RESULT « Tranche 27 »); the same record machinery serves it.

## 4. Resources, lifetimes and barriers

### 4.1 Table

| Resource | Size / format | Lifetime | Declared use |
|---|---|---|---|
| lit HDR | scene size, the native lit format | frame | read by Y1 |
| `YScene` | scene size, RGBA16F | frame | RT in Y1/Y3 (the depth factor goes into its alpha), SRV after |
| DoF layers, bloom pyramid, light-shaft radial pair | derived by the library (m40: 392×331, 392×331, 196×166, 98×83, 49×42, 25×21; DoF 784×294 and 392×147) ; R11G11B10 / RGBA16F | **transient** | alias candidates (disjoint lives, §4.3) |
| `GlareFinal[2]` (I3) | 640×540 or 420×354, same format | **persistent**, written every frame, read by the next frame's depth-factor draw | ping-pong by frame parity, never aliased |
| `AfterHist[2]` (I5) | glare base size | persistent | ping-pong |
| `DofTable[2]` (I4) | 2048×2 R32F | persistent | UAV in Y10, SRV in the next frame's Y3 |
| `Lut3D` | 16³ | persistent while enabled | UAV in Y0, SRV in Y9 |
| grain / dither | 256×256 RGBA8 | persistent (library-created) | SRV |
| `Ldr[A]`, `Ldr[B]` | back-buffer size, R8G8B8A8 | frame | see §4.2 |
| glow targets | the glow chain's sizes (EMISSIVE-GLOW §8) | transient | alias candidates |
| AO targets | SSAO-D3D12-DESIGN-NATIVE §4.2 | per its table | |
| exposure readback (AutoExposure only) | small | per-slot ring | off in every file read; kept for completeness |

### 4.2 Three improvements over the D3D11 shapes (each equivalent by data flow)

1. **`Ldr` ping-pong instead of the `PointSampleColor1` copy.** The original copies the tone-mapped LDR image before the anti-aliasing so that DLAA reads one texture and writes the other. A pair `Ldr[A]`, `Ldr[B]` does the same without the copy draw; glow's additive composites and the UI then draw into `Ldr[B]`. Check: the DLAA result bit-equal on a still frame (row YB7).
2. **The scene copy is a plan pass with its clamp.** `PointSampleColorClamp1` exists because the library's input must be a render target with finite values; as a plan pass, the lit HDR can later be produced directly in that format by the forward pass if the formats allow (an optimisation to measure, not a requirement).
3. **Temporal resources are plan resources with cross-frame edges.** The library reads I3 and I4 at the start of frame N+1 that frame N wrote; today that is a convention of "never alias, keep the state". In the plan the edge is explicit; with frames in flight on one queue it is ordered by the queue, and the resize or reset rule (first frame after a load, no history) is one flag the plan honours (row YB6).

### 4.3 Barriers and aliasing

Barriers come from the compiled draw graph: a resource is transitioned once between its writer group and its first reader group, and the library's ping-pong flips (bloom levels, afterimage, radial pair) are the only per-draw transitions left. Transient targets with disjoint lives share placed storage (review R11); the persistent ones and `Lut3D` never alias. `ResourceHeapTier` (read in phase A with the binding tier) decides whether render-target textures and the 3D LUT may share one heap.

### 4.4 Binding model shared by all post passes

One **post-pass root signature family**: root CBVs b0 (the YEBIS globals block or the pass block), b1, b2, b5, b8 (AO, shadow mask, composite), one SRV table `t0..t33` (the YEBIS draws use `t0..t16`; the AO and the mask passes use `t0..t3`, `t32`, `t33`), a sampler table `s0..s15` with two sources (D3D12-MASTER-PLAN §6 S1): the YEBIS draws copy from the 16 modes of YEBIS-NATIVE §5.2 (created once; ten of them have no row in the engine bank: black border, mirror, linear-mip-point with MaxLOD maximum), the GX passes (AO, mask, anti-aliasing, glow) copy the engine bank rows their binding code names; copied per draw into the frame-linear sampler heap and a small vertex-stage table for the DoF scatter (INFERRED rare). All fullscreen passes then differ only by PSO and the bytes of their blocks; the PSO count is about 30 for YEBIS (technique × blend × mask) plus five AO and a dozen GX (prewarmed at scene setup, review R9).

`$Globals` blocks are written into the frame arena at `round_up(highest used offset, 256)` bytes per draw (at most 4096), the library's dynamic vertices into the same arena; descriptors from the frame-linear heap (review R2, R6).

## 5. Fit with phases A to G

| Phase | What the chain needs | What it gives |
|---|---|---|
| A (R15, measure) | GPU timestamps per group (Y1-Y10, 13a, 13b, 15, AO), counters `yebis_draws`, `yebis_psos_created`, `post_barriers` | the cost table of the whole post chain (the largest unknown of the frame) |
| B (R2, R3, R5, R4) | arena for globals, vertices, the three constant blocks of the AO; ingress for the grain and the 2D LUT sources; token retirement of the surfaces the library releases and of `YebisFrame` | none |
| C (R6, R7, R9, R8a) | frame-linear descriptors (about 50 draws × 17 + 16); prewarmed ~45 PSOs; one post root shape | the shared post root signature is the first multi-pass use of R7 |
| D (R1, R12, R14) | persistent resources with ping-pong and per-slot arena blocks; the exposure readback ring per slot; gamma pass and the waitable swap chain share the last group; menu views and the YEBIS views are plan passes of one list | the temporal edges are valid on one queue; the waits of the original disappear |
| E (R10, R11) | the draw graph, states per use, aliasing for the pyramids, DoF layers, glow and AO transients | the model for every later effect |
| F (R13, R8b) | `YebisFrame` is built on the owner thread (library CPU code and bank reads) and is immutable: encode and present move to the worker | the clearest record/encode split of the frame |
| G (R16) | the two compute steps (`LutCopy2DTo3DCs`, `CustomDofLutCopy`) are tiny; an async queue only if a measurement shows contention | — |

## 6. Ordered steps, witnesses and live rows

| # | Step (checklist ids) | Witness (five zones m31, m37, m45, m50, m51) |
|---|---|---|
| B1 | **Tone-map-only first slice**: native HAL recorder, `CPostEffect` initialisation, the driver (P1-P4), Y1, Y8 only with the rest of the library running (the library cannot be split), the gamma pass | YB1: draw count and order per zone; YB2: `S` fields; image brightness against the original (H2) |
| B2 | PSO prewarm, arena and descriptor integration (phases B, C) | YB10; debug layer clean |
| B3 | Y3 depth of field and Y10 DoF table (persistent I3, I4) | YB5: persistent targets; the 10-draw block at every zone |
| B4 | Y4, Y5, Y7 glare, bloom, afterimage, final glare | draws 11-23 per zone with the sizes of §1; YB1 |
| B5 | Y6 light shafts, Y0/Y9 LUT and grading | m31 (48 draws, shafts, grading) and m50 (shafts), m40 grading; YB1 |
| B6 | `Ldr` ping-pong, 13a anti-aliasing, 13b glow, UI order, 15 gamma and present | YB7, YB8, YB9; one DLAA and ten glow draws per frame |
| B7 | Y2 motion blur with the native velocity (MOTIONBLUR Y1-Y16) | +2 draws; Y13 image witness |
| B8 | reset and resize rules (P9, P24): history flags, re-wrap of surfaces, sizes not multiples of 4 | YB6, YB15 |
| B9 | menu views without YEBIS (P23) through the same records | the Status and portrait views |
| B10 | AO (A1-A5) and its t33 into the lighting | SSAO rows SS1-SS12 |
| B11 | plan states, batched barriers, aliasing | YB11, YB12 |
| B12 | per-slot arena blocks and readback ring for frames in flight | YB11 |
| B13 | worker-thread encode from the immutable `YebisFrame` | YB13 |
| B14 | AutoExposure readback path (synthetic: no shipped file enables it) | YB16 |

Live rows (none taken):

| ID | Orig/Nat | Read | Expected |
|---|---|---|---|
| YB1 | Orig, Nat | the draw list and the render-target graph between `PointSampleColorClamp1` and `CustomDofLutCopy` at each zone | m31 48, m37 36, m45 36, m50 41, m51 36 draws; same order, sizes, formats (R11G11B10 pyramid, RGBA16 DoF and scene, R8G8B8A8 LDR) |
| YB2 | Orig, Nat | the library context `S+0x6E8/+0x6EC/+0x6F0/+0x70C` after the first frame | the zone's exposure (m31 2.2, m37 1.52, m45 2.0, m50 2.0, m51 2.0), gamma 2.2, tone type 2, filmic contrast (0, 0.3, 0.3, 0.1, 0.3) |
| YB3 | Orig, Nat | the last tone-map draw's constants | `c208 = T` of the checklist table, `c202.x = 0.4545` |
| YB4 | Orig | pipeline bit 10, `[144961F70]`, the option factors `[[GXRS+0x5368]+0x68/+0x80/+0x98]` at the quality levels | bit 10 set in the world, clear in menu views; factors 1.0 at the top level |
| YB5 | Orig, Nat | the persistent targets over 5 frames | I3 written each frame and read the next; I4 written by Y10; I5 pair alternates |
| YB6 | Orig, Nat | the first frame after a load or teleport | no history read; same image as the original's first frame |
| YB7 | Orig, Nat | the DLAA output on a still frame with the ping-pong pair | equal to the original's copy-then-DLAA result |
| YB8 | Orig, Nat | glow draws per frame; glow factor 0 | 1 + 4 + 5 draws; none at factor 0 |
| YB9 | Orig, Nat | brightness 0, 5, 10 | the curve of DISPLAY-SWAPCHAIN §9.4 (identity at 5) |
| YB10 | Nat | PSOs created per frame after warm-up | 0 |
| YB11 | Nat | `slots=2` against `slots=1` on a still scene | identical `Ldr[B]` |
| YB12 | Nat | aliased against separate targets | identical image; memory down by the measured amount |
| YB13 | Nat | the record list hash on two frames of a still scene | equal (deterministic recording) |
| YB14 | Nat | GPU time per group (timestamps) | the phase A table of the post chain |
| YB15 | Nat | a size that is not a multiple of 4 or 128 | all library sizes derived, no fault, same graph |
| YB16 | Nat (synthetic) | AutoExposure forced on | the readback is consumed one or more frames late; no stall |

## 7. Risks

1. **The library cannot be split from outside** (checklist §6): the first visible result already runs the whole chain, so the draw graph, the arena and the PSO prewarm must exist with step B1.
2. **Virtual resource requests.** The library's pools create and release targets through the creators; binding them lazily at compile time requires that no creator caller reads the resource before the draw. The wrapped resources (scene input, depth, LUT, LDR) are bound at creation.
3. **Per-draw scissor and sampler modes** are part of the record; a missing full-target scissor clips (the D3D12 scissor is always on).
4. **The `Ldr` pair** changes which texture later passes see; the glow, the UI and the gamma pass must name `Ldr[B]`. YB7 guards it.
5. **Aliasing** of the DoF layers and the pyramid is only valid if the library never re-reads a layer after its last recorded use in the frame (the record list shows it; the planner checks).
6. **Light shafts and grading gates** are not fully predicted by the gparam enables (checklist §4.2, §4.4): YB1 is the witness, not the table.
7. **The `PassKind`/plan caps**: the frame carries up to about 100 draw nodes in the post chain alone; the plan must treat draws as nodes of a graph (review R10).

## 8. Open points

The `PassKind` of the YEBIS groups in `frame_plan.h` (a new `PostProcess` kind); the exposure and option factors at each quality level (YB4); `LutSourceId` to a texture (checklist §7); the exact gate of the grading and light-shaft draws per zone (YB1); whether the forward pass can write the library's input format directly (§4.2 item 2).

## 9. Corrections and settlements after the disagreement study (part BD, STATIC)

Details and evidence in [D3D12-MASTER-PLAN](D3D12-MASTER-PLAN.md) §6.

1. **`144961FB8` and `144961FBC` are the library's GlareQuality and DepthOfFieldQuality, not the scene size.** The registration table names them (`yebis-globals.tsv`: `144961fb8 GlareQuality`, `144961fbc DepthOfFieldQuality`), `FUN_1400E4070` writes them from the Setting Info words 16 and 17, and `FUN_1400E2B20` passes them to `FUN_1416C87D0` as its two trailing arguments (line 177 of the export). YEBIS-NATIVE §4 step 2 and the checklist P1 call them "size": the library takes its target size from the surface it adopts (`Initialize`), and the quality arguments choose the pyramid depth and size through the library's quality table (`14469B4D0`, TONEMAP-SKIN §17). That explains the two glare bases of the checklist (210×177 and 392×331): they follow the zone's GlareQuality. A change of either quality raises the re-initialisation flags (`param_1+0x2278`, `+0x20F8`), so the native must treat a quality change as a library re-init (rows YB6 and YB17).
2. **The effective DoF quality is `min(zone value, quality+0x14)`** (`FUN_1400E4070` lines 131-145; `+0x14` = 0, 0, 2, 3, 5 at DISABLE..MAX). The zone value (7 default, 4 m40, 5 m50) is the maximum, not what runs: at the references' DoF MAX it is 5 in the default zones, 4 at m40, 5 at m50; at HIGH 3. GlareQuality has no level cap in that function.
3. **Samplers**: the YEBIS states are their own 16, not bank rows (ten have no row); §4.4 says it.
4. **Copies**: the YEBIS input (Y1) is a clamped draw of the final lit HDR, unrelated to the water/effect scene copy (one object in the original, `pipeline+0x1108`), which becomes the `FrameCopy` service of the master plan (S3).
5. **Order**: YEBIS, anti-aliasing, glow, UI, gamma (constructor member order AA before glow, and the captures): §3 stands.
6. **Plan constraints** (S2): a ping-pong needs two identities, history is `initialized = true` input of the next frame, and the reset rule is the builder's.

Live rows added: **YB17** (original and native; the DoF draw list at each DoF level at m31: counts, sizes; expected 10 draws at MAX, the library's other counts below) and **YB18** (the two init arguments `[144961FB8]`, `[144961FBC]` after the first frame at m31, m40, m50: Glare and DoF quality equal the zone value capped for DoF).

## Exports

No new export. Inputs read: `YEBIS-CHAIN-CHECKLIST.md`, `YEBIS-NATIVE.md` §4-§6, `YEBIS-HAL-READY.md` §2-§8, `NATIVE-PASS-ORDER.md`, `native/renderer/scene_pass.cpp`, `native/frame/game_bootstrap.cpp`; the review and phase notes in I-0066 (`D3D12-DESIGN-REVIEW.md`, `D3D12-PHASES-A-C-PREP.md`, `D3D12-PHASES-D-G-PREP.md`).
