# I-0068 parts BC, BD and BE (workstream K): one ordered plan for the D3D12 designs (phases A to G and the seven families), the five disagreements settled, and the collect step

Request (coordinator, 2026-10-11): consolidate the designs of effects, glow, shadows, tessellation, SSAO, YEBIS and `cacheAsBitmap` with phases A to G into **one ordered plan**: dependencies between workstreams, recommended order after phases A to C, shared resources (post root signature, arena, FramePlan), common witnesses and risks. **No fiche is rewritten**: this note only orders and links. Every number below is quoted from the linked note; where two notes disagreed, §6 (part BD) settles them statically. Static work (nothing launched, no native code changed).

Labels: **STATIC**, **OBSERVED**, **INFERRED**, **OPEN** as in the linked notes. Decision labels in this note are **PROPOSED**: they belong to Thomas and the principal technical owner.

## 0. Answer first

1. **Three layers, in this order.** (a) *Infrastructure*, the phases A to G of the [D3D12 review](../I-0066-independent-native-startup/D3D12-DESIGN-REVIEW.md); (b) *shared pass machinery*, which the family designs all assume and which no note owns yet: one **post root signature family**, the **frame arena** with its block list, and the **FramePlan draw graph** with states and aliasing; (c) the *families*, each reduced to the slices that need only (a)+(b).
2. **Phases A to C are enough to start every family slice that draws nothing or draws one fullscreen pass** (§4, wave 1). Frames in flight (D) and the plan with states (E) are needed before the families that put many passes in the frame (shadows, effects, the whole YEBIS chain). So: **A–C, then wave 1 in parallel with D–E, then waves 2 to 5 on top of E** (§4).
3. **The shared pass machinery is a deliverable of its own**, placed between phase C and the first multi-pass family: the post root signature, the arena block list and the draw-graph extension of `FramePlan` (§3). Without it each family would build a private version, which is what D-0157 asks to avoid.
4. **One frame order** carries all families (§2); it is NATIVE-PASS-ORDER plus the groups of the seven designs. It does not move a single original pass.
5. **Common witnesses**: the five zones m31, m37, m45, m50, m51 and the four non-regression runs of the review (Native1191, Native1254, Native1262, Status/portraits); one **frame-graph digest row per wave** (§5).
6. **Largest risks** (§7): the post pass family growing past its caps (128 plan passes, root signature shape), temporal resources and frames in flight, aliasing errors that stay silent, and the number of unmeasured costs (the whole post chain has no measured baseline yet).
7. **The frame is dominated by the native `collect` step** (104 ms of 166, phase A): workstream **K** ([COLLECT-NATIVE-REPRISES](COLLECT-NATIVE-REPRISES.md), part BE) is placed in parallel with phases B and C because those phases do not touch it, and because every later family assumes a cheap selection (§4).

## 1. The documents, one line each

| Part | Note | Subject | Native today | Slices that need only A–C | Slices that need E |
|---|---|---|---|---|---|
| review | [D3D12-DESIGN-REVIEW](../I-0066-independent-native-startup/D3D12-DESIGN-REVIEW.md), [A–C](../I-0066-independent-native-startup/D3D12-PHASES-A-C-PREP.md), [D–G](../I-0066-independent-native-startup/D3D12-PHASES-D-G-PREP.md) | R1–R16, phases A–G, commit orders, measures | fixture-only plan, waits per frame | all of A–C | D, E, F, G |
| AV | [EFFECTS-D3D12-DESIGN-NATIVE](EFFECTS-D3D12-DESIGN-NATIVE.md) | FFX record/encode, one root signature, arena, GPU particles | none (door and wind not published) | N1–N4: door, lights, wind (no draw) | N5–N11: records, encode, distortion, model particles, GPU particles |
| AS | [GLOW-CHAIN-NATIVE](GLOW-CHAIN-NATIVE.md), [EMISSIVE-GLOW-NATIVE](EMISSIVE-GLOW-NATIVE.md) | forward `[HDR, GB5]`, glow chain, family weights | GB5 written, chain absent | steps 1–3, 6 | 4–5 as plan passes, 8 with the effects |
| AU, AW | [SHADOWS-NATIVE](SHADOWS-NATIVE.md) §6–§13, [TESSSCENE-SSMASK-NATIVE](TESSSCENE-SSMASK-NATIVE.md) | atlas, cascades, mask, `ShadowFrame`, `cbSceneParam`/`cbTessScene` | absent | steps 1–5 (parameters, atlas, casters, blocks) | steps 6–10 (mask, sun, forward receive, local lights) |
| AT, AW | [TESS-SHADOW-FORWARD-NATIVE](TESS-SHADOW-FORWARD-NATIVE.md) | veil `_PN`, `_FT_D`, forward shadows by family | flat `Fwd` | GX04 parse, `_PN` selection, HS/DS PSO | forward receive (needs shadow mask) |
| BA | [SSAO-D3D12-DESIGN-NATIVE](SSAO-D3D12-DESIGN-NATIVE.md), [SSAO-NATIVE](SSAO-NATIVE.md) | AO chain, gate, parameters, `t33` | constants only | S1–S2 (parameters, resources, PSOs) | S3–S11 (passes, history, aliasing) |
| BB | [YEBIS-D3D12-DESIGN-NATIVE](YEBIS-D3D12-DESIGN-NATIVE.md), [YEBIS-CHAIN-CHECKLIST](YEBIS-CHAIN-CHECKLIST.md), [YEBIS-HAL-READY](YEBIS-HAL-READY.md) | recorder HAL, Y0–Y10, 13a/13b, 15 | sRGB encode only | B1 (tone map first, with the whole library running), B2 | B3–B14 |
| AZ | [SCALEFORM-CACHE-COLORMATRIX](SCALEFORM-CACHE-COLORMATRIX.md), [SCALEFORM-FILTERS-NATIVE](../I-0067-static-menus-subsystems-map/SCALEFORM-FILTERS-NATIVE.md) | cached quads (F8a/F8b), colour matrix (F9) | filters other than the empty set refused | F8a, F9 (UI primitive pass only) | F8b persistence (needs tokens, phase B) |
| map | [NATIVE-PASS-ORDER](NATIVE-PASS-ORDER.md), [LIVE-CHECKS-CONSOLIDATED](LIVE-CHECKS-CONSOLIDATED.md) | the frame in GPU order; every open live row | — | — | — |

Neighbours not redesigned here, which sit in the same frame: decals ([DECALS-NATIVE](DECALS-NATIVE.md)), water ([WATER-NATIVE](WATER-NATIVE.md)), spot/line lights ([SPOT-LINE-LIGHTS-NATIVE](SPOT-LINE-LIGHTS-NATIVE.md)), velocity ([VELOCITY-MOTIONBLUR-NATIVE](VELOCITY-MOTIONBLUR-NATIVE.md), [YEBIS-MOTIONBLUR-NATIVE](../I-0067-static-menus-subsystems-map/YEBIS-MOTIONBLUR-NATIVE.md)), display ([DISPLAY-SWAPCHAIN-NATIVE](DISPLAY-SWAPCHAIN-NATIVE.md)).

## 2. The single frame, with every group

Order = [NATIVE-PASS-ORDER](NATIVE-PASS-ORDER.md) §1 (unchanged). Added by a design: **bold**. Owner = the note that fixes the group.

| # | Group | Reads | Writes | Owner |
|---|---|---|---|---|
| 0 | CPU before the frame: gather, light items, FX update, **`ShadowFrame`, `AoView`, the per-view `cbSceneParam` image, `cbTessScene`, `YebisFrame` records (library CPU)** | engine state | arena blocks | AU, BA, BB; R13 |
| 1 | **Shadow maps**: atlas, four cascades, tiles | casters | atlas | SHADOWS §9 |
| 3 | G-buffer | models | GB0–GB5, depth | native |
| 4 | copies | depth, GB0/GB1 | `DepthRead` | shared (DECALS §1) |
| 6 | **AO A1–A5**; **mask**, blur, composite | `DepthRead`, GB0, atlas, `AoHistory[prev]` | `AoHistory[cur]`, **t33**, **t32** | BA, AU |
| 7–8 | env regions, lighting (shadowed sun, point/spot tiles) | t32, t33, GB | lit HDR | native, SPOT |
| 9 | forward rows `[HDR, GB5]`, **tessellated veil**, **forward receive (`FwdSdw`/`FwdSSdw`)** | t32 or atlas, `cbTessScene` | HDR, GB5 | AS, AT, AU |
| 9b | **`FrameCopy`** (HDR, once, only if water or a distortion/radial-blur record exists; S3) | HDR | `FrameCopy` | S3 |
| 10 | water | `FrameCopy` | HDR | WATER |
| 11 | **effects** (arena records, E01–E07, glow twins) | `DepthRead`, `FrameCopy` | HDR, GB5 | AV |
| 11b | velocity | | velocity, motion vectors | native |
| 12 | **YEBIS Y0–Y10** (LUT, scene input, motion blur, DoF, glare, afterimage, shafts, tone map, grading, DoF table) | lit HDR, `DepthRead`, velocity, `GlareFinal[prev]` | `Ldr[A]`, `GlareFinal[cur]`, `DofTable[cur]` | BB |
| 13 | **anti-aliasing 13a, glow 13b** | `Ldr[A]`, GB5 | `Ldr[B]` | BB, AS |
| 14 | UI: Scaleform with **cached quads/filters**, menu views | `Ldr[B]` | `Ldr[B]` | AZ |
| 15 | **gamma and present** | `Ldr[B]` | back buffer | BB, DISPLAY §9 |

Order constraints that cross notes (all STATIC from the notes): AO runs after the depth copy and before the lighting (BA §4.3); the mask composite writes t32 after AO's `t33` is available or white (SSAO-NATIVE step 8); effect lights are created in the CPU phase 0 and consumed by the lighting (AV §2.7); glow twins and the filter must be in the same frame as the extraction (AV N8); glow is placed after the anti-aliasing and before the UI (GLOW §5 step 5, BB §3).

## 3. Shared resources

### 3.1 Root signature families (PROPOSED: four, no fifth)

| Family | Used by | Shape (from the notes) | Settled by |
|---|---|---|---|
| **model** | G-buffer, forward, casters (Dpt), tessellated forward rows | R7: ranges sized on use; HS/DS blocks `cbPNTriangles`, `cbTessScene` added for `_PN` rows | R7; AT §3; AW §1 |
| **effects** | E01–E07 draws | root CBVs VS b0,b1,b9; DS b0; GS b9; PS b0,b1,b6,b9,b11; SRV table t0..t7; sampler table s0..s9 (measured over 328 programmes) | AV §2.2 |
| **post** | shadow mask passes, composite, **AO**, **YEBIS draws**, anti-aliasing, glow, gamma, the GX menu tone-map filter | root CBVs b0, b1, b2, b5, b8; SRV table t0..t33; sampler table s0..s15 | §6 S1 (two descriptor sources: the GX bank rows and the YEBIS 16 states) |
| **UI** | primitive pass, filter passes, cached quads | existing (`primitive_pass.*`) | AZ §5 |

The effects family stays apart (different register set, GS/DS stages); the review already says family root signatures stay per family where register sets differ.

### 3.2 Arena (per frame slot, fence-retired, R2)

Block list quoted from the notes; sizes to be measured in phase A (§5, row C1):

| Block | Size / count | Note |
|---|---|---|
| model constants, point-light ring, primitive pass | existing producers | R2a/R2b |
| per-view `cbSceneParam` (960 B), `cbPerScene`, `cbBlurParam`, `cbTessScene` (0xB4 B) | about 2 KB per view | SHADOWS §11–§13; AW |
| effect records and vertices | budget 2 MB per frame (original P5) | AV §2.2 |
| AO blocks | 48 + 256 + 288 B | BA §4.4 |
| YEBIS globals + library dynamic vertices | `$Globals` up to 4096 B per draw, 36 to 48 draws | BB §4.4 |
| glow blocks | 10 draws | AS |
| UI geometry and filter blocks | existing | R2 |

One arena per slot with the **same lifetime rule** (retired by the slot's fence, R2/R3); no family owns a private ring. Per-slot copies matter only once frames in flight exist (D): until then one slot.

### 3.3 FramePlan (the extension every note assumes: R10, R11)

What the notes ask of `native/renderer/frame_plan.h`, gathered:

1. **States per use and subresource ranges**, batched barriers, cross-frame initial states (R10; BB §2.3; BA §4.3).
2. **Aliasing** of transient targets (AO, shadow mask, glow, YEBIS pyramids and DoF layers, filter targets); persistent ones (`AoHistory`, `GlareFinal`, `AfterHist`, `DofTable`, `Lut3D`, shadow cascade caches) never alias (R11; BB §4.3).
3. **Draw graph inside a pass group**: the post chain alone carries about 100 draw nodes; the cap of 128 top-level passes (D–G prep §2.5) becomes 256 with a peak counter (§6 S2) and library draws are nodes of a group, not passes (BB §2.3, §7 risk 7).
4. **A `PostProcess` pass kind** and stable ids for virtual resources (BB §8).
5. **Cross-frame edges** as explicit data for the temporal resources (BB §4.2 item 3; BA §4.5).
6. The same readable-depth resource as DSV and SRV for AO, effects, DoF (DEPTH_READ with PIXEL_SHADER_RESOURCE, no copy; AV §2.3; BA §4.2).

### 3.4 Other shared pieces

- **PSO cache and prewarm** (R9): counts quoted: about 45 for YEBIS and GX steps (30 + 5 AO + ~12), the eight E01 programmes first for effects, mask and glow programmes; one hashed key.
- **Descriptors** (R6): frame-linear heaps, used registers only, samplers created once.
- **Shared inputs**: readable depth (`DepthRead`), GB0/GB1 copies, GB5, the HDR frame copy on demand (distortion, water, DoF/library scene input), velocity targets, `ShadowFrame`, the camera record (feeds `cbTessScene`, `cbSceneParam`, `AoView`).
- **Gamma pass and the swap chain** (R14/D5) are the last group for every path (menu views included).

## 4. Dependencies and the recommended order after A–C

```
A measure (R15, timestamps, counters)
B memory (R3 tokens, R2 arena, R5 ingress, R4 geometry)
C descriptors (R9 PSO, R6 heaps, R7 shapes, R8a draws)
K collect (CPU only, parallel to B and C): C0 timers, C1-C3 produce what rows use, C4-C6 caches, C7 persistent admission, C10 with R13
   |
   +--- wave 1 (needs A–C only; engine-side and single-pass; runs in parallel with D–E)
   |        effects N1–N4 (door, lights, wind)      shadows steps 1–3 and 5 (gates, state, atlas, blocks; casters = wave 3)
   |        glow steps 1–3, 6                        tess: GX04 parse, _PN select, HS/DS PSO, cbPNTriangles
   |        SSAO S1–S2 (AoParams, resources, PSOs)   YEBIS B1 slice 0 (recorder skeleton + gamma + tone map)
   |        Scaleform F8a, F9
   |
   +--- D frames in flight (R1, R12, R14)  -->  E plan with states and aliasing (R10, R11)
   |                                                   |
   |      shared post machinery (post root signature, arena block list, draw-graph FramePlan)  <- gate for waves 2–5
   |
   +--- wave 2 (post machinery): SSAO S3–S8; shadow mask + composite (steps 6–7); gamma/AA scaffolding
   +--- wave 3 (lighting & forward): sun switch (step 7), forward receive (8), local-light tiles (9), tessellation constants (10), veil _PN
   +--- wave 4 (colour chain): YEBIS B2–B9 (tone map complete, DoF, glare, afterimage, shafts, grading, AA, glow 4–8, UI order, menu views)
   +--- wave 5 (effects): N5–N11 (records, encode, ribbons, distortion, model particles, GPU particles); glow twins (step 8)
   +--- wave 6: YEBIS motion blur B7 (after velocity captures), frames-in-flight audit, F8b persistence
   +--- F worker thread (R13/R8b) after waves 2–5 are stable; G on measurement
```

Rationale, one line per wave:

**Wave 1 is prepared in [WAVE1-PREP](../I-0066-independent-native-startup/WAVE1-PREP.md) (part BG)**: files, functions, lines, commit order W0a-W10, measures and witnesses (commit W0a is detailed to implementation level, and Opus-reviewed, in [MODEL-DRAW-ENVELOPE-READY](../I-0066-independent-native-startup/MODEL-DRAW-ENVELOPE-READY.md)). The code reading changed three things in this table: shadow step 4 (casters) moves to wave 3 (six hard-coded shader-pass gates, unrecovered row ids); a shared model-draw envelope commit (W0a) precedes glow RT1, tessellation and the shadow atlas; the GX04 parse must ship with a flat fallback or the veil stops drawing. The glow of wave 1 (G1-G3, G6) is detailed to implementation level in [GLOW-WAVE1-READY](../I-0066-independent-native-startup/GLOW-WAVE1-READY.md) (part BJ, Opus-reviewed). The SSAO parameters of wave 1 are detailed in [SSAO-WAVE1-READY](../I-0066-independent-native-startup/SSAO-WAVE1-READY.md) (part BK, Opus-reviewed). The YEBIS and tone-map trunk is detailed in [YEBIS-WAVE1-READY](../I-0066-independent-native-startup/YEBIS-WAVE1-READY.md) (part BL, Opus-reviewed). The effects of wave 1 (N1-N4, by family, with the wind registry chain and the light-shaft and colour-grading banks YEBIS needs) are detailed in [FFX-WAVE1-READY](../I-0066-independent-native-startup/FFX-WAVE1-READY.md) (part BM, Opus-reviewed). The shadows of wave 1 (cascades, local-light shadows, filtering; the casters stay wave 3) are detailed in [SHADOWS-WAVE1-READY](../I-0066-independent-native-startup/SHADOWS-WAVE1-READY.md) (part BN, Opus-reviewed). Water, refraction/reflection (with the `FrameCopy` position) and tessellation of wave 1 are detailed in [WATER-TESS-WAVE1-READY](../I-0066-independent-native-startup/WATER-TESS-WAVE1-READY.md) (part BO, Opus-reviewed). The other YEBIS passes (depth of field, the YEBIS half of motion blur, feedback blur, auto exposure and focus read-back, the history rules) are detailed in [YEBIS-SECOND-TRANCHE-READY](../I-0066-independent-native-startup/YEBIS-SECOND-TRANCHE-READY.md) (part BP, Opus-reviewed). The temporal upscaler (render and output extents, jitter, motion vectors, reactive mask, history reset, DLSS adapter behind a neutral interface) is detailed in [UPSCALER-DLSS-READY](../I-0066-independent-native-startup/UPSCALER-DLSS-READY.md) (part BQ, Opus-reviewed).

| Wave | Content | Why here | Depends on |
|---|---|---|---|
| K | the native collect step ([COLLECT-NATIVE-REPRISES](COLLECT-NATIVE-REPRISES.md), part BE): C0 timers, C1-C3 produce only what the rows use, C4-C6 version caches, C7 persistent admission record, C9 row cache, C10 overlap; prepared in [COLLECT-K-PREP](../I-0066-independent-native-startup/COLLECT-K-PREP.md), original side in [COLLECT-ORIGINAL-STATIC](../I-0066-independent-native-startup/COLLECT-ORIGINAL-STATIC.md) | collect is 104 ms of the 166 ms frame (phase A) and phases B-C do not touch it; CPU only, so it runs now, in parallel with B and C, and its version stamps are shared with R8a and R8b | none (C0 first); C7 converges with R8a; C10 with R13 |
| 1 | engine-side producers and single fullscreen passes | They need an arena and PSOs, not the plan; they unblock witnesses (the door and wind are the best early effect witnesses, AV §0 item 4); YEBIS tone map and gamma are the largest visible gain (NATIVE-PASS-ORDER rank 3) | A, B (arena), C (PSOs) |
| 2 | AO then mask/composite | `t33`/`t32` are consumed by lighting, forward and points; AO needs only depth and normals; the composite works with a white mask B | wave 1 S1–S2, post root signature |
| 3 | shadows into lighting and forward, tessellation | forward receive needs the mask (SHADOWS step 6) and `ShadowFrame`; tessellation constants come from the same camera record | wave 2, `cbSceneParam` image |
| 4 | the colour chain | needs the lit HDR (waves 2–3) to compare; the whole library is run from B1, so the recorder, arena and PSO prewarm already exist | E (draw graph) |
| 5 | effects | needs forward `[HDR, GB5]`, readable depth, frame copy, effect lights; distortion/radial blur and glow twins use the machinery of waves 2–4 | wave 3, 4 |
| 6 | motion blur, persistence | motion blur needs the velocity captures M1–M5 and the library; F8b needs tokens | wave 4 |
| F, G | worker thread, extra queues | largest contract change, after the families are stable | all |

**Differences from NATIVE-PASS-ORDER §3** (value-first order): that table orders by visible gain with no infrastructure; this plan keeps its ranks 1–6 as waves 1, 2 and 4 and moves effects after shadows/YEBIS (they need the shared post machinery, the frame copy and E). Decals, water, spot lights, NPC fade and Bink stay on their own ranks (they use the model root signature and the existing paths) and can interleave anywhere after wave 1 without touching the post family.

## 5. Common witnesses

### 5.1 The five zones, one set for every wave

| Zone | Used by |
|---|---|
| **m31** Undead Settlement | wind (19 regions), 48 YEBIS draws with shafts and grading, door statistics |
| **m37** Irithyll | 36 YEBIS draws, E01 patch lists (≈68), distortion, ghosts in shade (forward shadows) |
| **m45** Ariandel | GPU particles, 8 wind regions, glare |
| **m50** Dreg Heap | 41 draws with shafts, radial blur, 392×331 glare base |
| **m51** Ringed City | model particles/grass, radial blur, `FwdSSdw` hair, most effect lights |

Plus **m30_00/m40_00** (High Wall, Firelink) for torches, the Argo and TestAnim saves ([LIVE-CHECKS-CONSOLIDATED](LIVE-CHECKS-CONSOLIDATED.md) §1, §2).

### 5.2 Non-regression runs (existing, from the review)

Native1191 (velocity), Native1254 (in-game), Native1262 (quit), Status/portraits views; the debug layer clean; no `Drain` in the ingress, retirement and stream paths from phase B on.

### 5.3 One row per wave, shared form (PROPOSED)

| Row | Read | Expected |
|---|---|---|
| C0 | frame-graph digest: the list of group ids, draw counts and states per group at each zone, written once per run | stable between two runs of a still scene; changes only with a documented step |
| C1 | arena peak per slot and per block (the table of §3.2) | under the budget fixed in phase A |
| C2 | PSOs created after warm-up | 0 |
| C3 | GPU time per group (timestamps; BB row YB14, BA SS11, AV FX4) | the cost table of the whole frame |
| C4 | `slots=2` against `slots=1` on a still scene (BB YB11) | identical image |
| C5 | aliased against separate targets (BB YB12) | identical image |
| C6 | peak number of plan passes (`plan_passes_peak`; §6 S2) | under 128 grouped; recorded for the cap |
| C7 | `CopyResource` count of the HDR target per frame (§6 S3) | 1 with water or distortion, 0 or 1 otherwise |

Family rows stay in their notes: SH1–SH6 and S1–S6 (SHADOWS), TS1–TS3, FX1–FX13, G1–G6, SS1–SS12, YB1–YB16, CB1–CB3/DS1–DS3/LB7/CM1/TX1; the full list is in LIVE-CHECKS-CONSOLIDATED.

## 6. The five disagreements, settled statically (part BD)

Request (coordinator, 2026-10-11): settle in static, as far as the original's code and the native allow, the five disagreements of the first version of this section. Nothing was launched, no native code changed. Evidence (outside Git): `evidence/I-0068/queries/samplers/{c1,f1,f2}` (callers of the sampler-set setter, the decompiled bindings, the pipeline constructor), `evidence/I-0068/queries/ssao/s1/1401b22e0.c`; native sources read: `native/content/sampler_bank.h`, `sampler_inputs.h`, `native/renderer/frame_plan.{h,cpp}`.

| # | Disagreement | Verdict | Status |
|---|---|---|---|
| S1 | sampler model of the post root signature (AO static samplers, YEBIS table) | **one sampler table s0..s15, no static samplers; two descriptor sources** | settled (STATIC) + one live row |
| S2 | the 128-pass cap | **keep group-level passes, draws as nodes, raise the cap to 256 and count the peak** | settled (STATIC) + one live row |
| S3 | a frame copy service | **one demand-driven HDR copy for water and effects (the same object in the original); the YEBIS input and the LDR copy are not copies of it** | settled (STATIC) + one live row |
| S4 | glow order | **AA, then glow, then UI, then gamma: no disagreement** | settled (STATIC member order + OBSERVED order) |
| S5 | quality gates | **one gate table; DoF is capped by `quality+0x14`, which corrects two earlier notes** | settled (STATIC) + live rows |

### S1. Samplers

**The original.** GX passes do not use register names as states: they set a sampler array at the start of the pass from the **GX common bank** `[[144961AB0]+0x2A0]+0x2C8 + 8·i` (16 pointers, the 16 rows of `native/content/sampler_bank.h`). The AO layer prologue `FUN_1401B22E0` loads rows **0, 1, 3, 4, 6** and calls `PSSetSamplers(0, 5, ...)` (`FUN_141920D20`, lines 66-83 of the export): `s0` = row 0 (point, wrap), `s1` = row 1 (trilinear, wrap), `s2` = row 3 (point, clamp), `s3` = row 4 (trilinear, clamp), `s4` = row 6 (point, **white** border). The DXBC names of the AO programmes (`SS_WrapPoint` s0, `SS_ClampPoint` s2, `SS_ClampLinear` s3, `SS_WhiteBorderPoint` s4) match this exactly. Other GX filter functions set prefixes of the same list (`FUN_1401CC840` and `FUN_1401D2210`: rows 0, 1, 3, 4 at s0..s3; `FUN_1401C48E0`: rows 3, 4): the glow programmes' `s3 ClampLinear` is row 4. The scene programmes use another numbering of the same rows (`s6` WrapPoint, `s7` ClampPoint, `s8` WrapLinear, DECALS-NATIVE). The names alone are not authoritative: E01's `s9 g_ClampLinearSampler` is observed bound as WRAP (AK point 3), because the binding code, not the name, picks the row.

**The YEBIS library** does not use the bank. Its 16 modes (YEBIS-NATIVE §5.2: filter 0..3 × address border/clamp/mirror/wrap) compared with the bank rows: exact matches for trilinear wrap (row 1), trilinear clamp (row 4), linear-mip-point MaxLOD 0 wrap (row 2) and clamp (row 5); the two point modes match rows 0 and 3 except MaxLOD (0 against FLT_MAX), which is equal on single-mip textures; the remaining **ten modes have no bank row** (all four black-border modes, since the bank's border rows are white; all four mirror modes; the two linear-mip-point FLT_MAX modes). So the YEBIS states cannot all be bank rows.

**Settlement.** The post family uses **a sampler table s0..s15** (the YEBIS draws need per-texture modes, which static samplers cannot follow), with **two sources**: GX passes copy the bank descriptors their binding code names (AO: rows 0, 1, 3, 4, 6 into s0..s4; the same rule for the mask, glow and AA passes, from the function that records each pass), YEBIS copies from its own 16 states created once. The AO note's static sampler set is replaced (SSAO-D3D12-DESIGN-NATIVE §4.4). The bank rows 12 and 13 change with the Texture quality level (`ApplyEngineSamplerSettings`); rows 0, 1, 3, 4, 6 do not, so the AO and glow sets are constant. Live row **PL1**: the descriptors bound at the first YEBIS draw of a frame in the original (the D3D11 sampler states of the 16 modes) equal the §5.2 table.

### S2. The pass cap

`CompileFramePlan` (`frame_plan.cpp:11`) refuses more than **128 passes** and more than 16384 resources, with `pass.after.size() ≤ 128` and a matrix `depends[p][q]` of P×P booleans plus a transitive closure: the cost is O(P·D·P) with D the dependency count, 2 M operations in the worst case, microseconds in practice (D ≤ 4). The cap is a safety check, not a performance limit. It matters because the plan has no notion of draws: a pass is a set of whole-resource uses.

Count of the groups of §2, from the counts in the linked notes (INFERRED sums): shadow maps 5-6, G-buffer 2, copies 3, decals 2, AO 5, mask chain 7, environment regions 1, lighting 7, forward 2, water 5, effects about 8, velocity 3, YEBIS 11, anti-aliasing 1, glow 3, UI about 8, present 1: **about 75 to 90**. If each environment region (the fixture allows 64), light tile, glow draw, filter pass or YEBIS draw (36 to 48) were its own pass, the total would pass 150. **Settlement:** passes are *groups*; draws, tiles and regions are nodes of a draw list carried by the group, outside `CompileFramePlan` (which keeps validating whole-resource hazards between groups); the cap becomes **256** (one line, no cost) and a counter `plan_passes_peak` records the real number (row C6). The plan can be cached by the digest of row C0 when the structure is unchanged.

**Other plan constraints found while reading, which the designs must respect:** a resource identity is unique in a plan (`ResourceIdentity`), so a ping-pong needs two identities; a pass may name a resource once (`DuplicateUse`), so a group that reads and writes a resource uses `ReadWrite`; a read needs the last writer in its dependencies (`MissingDependency`) and a write needs all earlier readers; `initialized` is the native owner's attestation. Consequences: **cross-frame history** is a resource with `initialized = true` (previous frame) and a separate identity for the current one; the reset rule of YB6/SS7 is a builder rule (when the history is invalid, the reader does not name it and binds the neutral texture), because reading an uninitialised resource is an error. There is no `PostProcess` kind in `PassKind` (`Effect`, `Composition`, `Lighting`... exist): add one, or reuse `Composition` for the colour chain.

### S3. Frame copies

The three "copies" are different things:

1. **The scene copy of water and effects is one object.** The water surface reads it as `t55 g_RefractionTexture = [frame+0x1108]` (WATER-NATIVE, the `CopyResource` HDR `12439` to `12440`), and the FFX distortion and radial blur read it as `t45 g_frameBufferTexture`, whose resource is the "frame copy pipeline `+0x1108`" taken by `FUN_1400BFA70` at the start of forward pass 0x29 (EFFECTS-E05-E04-NATIVE §8, FFX-FAMILIES-PHASE2 §8). Same field, same size and format as the HDR target (R8G8B8A8 when the pipeline LDR flag `+0x74` is set). It is unconditional in the original and read by effects drawn after the water, so it does not contain the water or the effects.
2. **The YEBIS input is a draw, not a copy**: `PointSampleColorClamp1` (YEBIS-CHAIN-CHECKLIST P6) copies the final lit HDR (after water, effects and velocity) into an RGBA16F target while clamping non-finite values, and the library then writes the depth factor into its alpha; its content and time differ from (1).
3. **The LDR copy** `PointSampleColor1` before the anti-aliasing is removed by the `Ldr[A]`/`Ldr[B]` pair (YEBIS-D3D12-DESIGN-NATIVE §4.2).

**Settlement.** A `FrameCopy` service for (1) only: a native resource of the HDR size and format created with the scene targets; the copy is recorded **once per frame, after the opaque forward rows and before the water surface or the first effect that needs it, only if a consumer exists** (a water mesh in the frame, or an effect record flagged `needs_frame_copy`, AV §2.4); the reference mode always copies. The consumers bind it as SRV in the effects family (t45 equivalent) and in the model family (t55 equivalent). (2) and (3) stay what the YEBIS note says. Live row **FC1**: the number of `CopyResource` of the HDR target per frame at a zone with water and effect distortion (m37, Firelink) is 1 in the original; and in a frame with neither water nor distortion, whether the original still copies (it does, unconditionally), which the native may skip.

### S4. Glow order

The pipeline instance constructor `FUN_1400843C0` creates the anti-aliasing filter (`param_1[0x5BF]`, vtable `143D5D408`) before the glow filter (`param_1[0x5D0]`, vtable `143D5D490`), and both before the tone-map filter (`+0x60C`): the member order is AA, glow, tone map. The captures show the same order (TONEMAP-SKIN §16/§17: YEBIS, `CustomDofLutCopy`, `PointSampleColor1`, `DLAA`, `EmissiveToGlowPS`, four `Sample13Weight`, five `GlowCompositeSinglePS`, the last into the LDR target). So **YEBIS, AA, glow, UI, gamma**. The apparent disagreement was between "compose onto the native final colour now" (GLOW §4, a temporary step before YEBIS exists) and the final placement. Settled: before YEBIS, glow composes onto the native final colour; once YEBIS exists, it composes onto `Ldr[B]` after the anti-aliasing. Rows C10 and C11 of the checklist remain the witnesses.

### S5. Quality gates (one table)

Source of the levels: the twelve levels of the config block, the quality object `[GXRS+0x5360]` (built natively since Native1255, QUALITY-OBJECT-NATIVE-LIVE) and the option factors `[GXRS+0x5368]` (built by `FUN_127010`, value 1.0 unless an options rewrite changes it). Levels 0..4 = DISABLE, LOW, MEDIUM, HIGH, MAX (MENUS-MAP quality table). The configuration of the references: `[4,3,3,4,3,4,4,4,3,4,3,4]` (Texture MAX, AA ON, SSAO HIGH, DoF MAX, MotionBlur HIGH, Shadow MAX, ...).

| Stage | Gate | Levels that enable it | Source |
|---|---|---|---|
| AO | `quality+0x0A` and (`UseReProj` or `quality+0x0D`) and the view record enable | HIGH, MAX | `FUN_1401B26E0` (SSAO-D3D12 §2.4) |
| Anti-aliasing | `quality+0x08` and AA factor `[[5368]+0x98] > 0` and `filter+9` and gparam `AntiAliasType` | `+0x08` = 1 at HIGH, MAX | `FUN_1400FB890` lines 404-410; checklist P13 |
| Depth of field | **effective quality = `min(zone Setting Info DepthOfFieldQuality, quality+0x14)`**; `+0x14` = 0 / 0 / 2 / 3 / 5 for DISABLE..MAX | any level with `+0x14 > 0` | `FUN_1400E4070` lines 131-145 (new) |
| Motion blur | `quality+0x20` (on), `+0x21` (object velocity), `+0x24` (cap 1, 1, 5, 8 from LOW) | LOW and up; objects from MEDIUM | `FUN_1400E4B50`; YEBIS-MOTIONBLUR |
| Glare | factor `[[5368]+0x68] > 0` and the gparam shape; **not** a quality level | always (factor 1.0) | `FUN_1400FB890` line 262; `E4460` |
| Glow | factor `[[5368]+0x80] > 0` and the filter `Enable` | always (factor 1.0) | GLOW-CHAIN §1 |
| Light shafts, colour grading, LUT, exposure | zone Setting Info and gparam | not quality-gated | checklist §4 |
| Shadows | `quality+0x28`, size `+0x38` | LOW and up, size 1024..8192 | SHADOWS-NATIVE |

**What this corrects.** (1) The YEBIS checklist (P7b and the zone table) and the design note give the DoF quality as the zone value (7 default, 4 m40, 5 m50). The effective value is the minimum with `quality+0x14`: at the DoF MAX of the references it is **5** in the default-quality zones (m31, m37, m45, m51), 4 at m40 and 5 at m50; at HIGH it is 3 everywhere; at LOW and DISABLE it is 0. The "10 DoF draws at every zone" observation is therefore true **at the captured level**; below it the draw count is not known (the library's behaviour at quality 0 and below the zone value is a live row). (2) The checklist said the light shafts and grading gates "are not predicted by the tables": they are zone-file gates, independent of the quality level, and stay witnessed by YB1. (3) "DoF and glare depend on the zone parameters" becomes: DoF on the minimum of zone and level; glare on the zone only.

Live rows: **QG1** (original and native, a still scene at each level of one setting at a time, m31): AO draws (5 or 0), AA draw (1 or 0), DoF draws (10 at MAX, other counts at the lower levels), motion blur draws (+2 or 0), glow and glare draws constant. **QG2**: `quality+0x14` and the DoF global `144961FBC` after the first frame at each DoF level (expected 0, 0, 2, 3, 5 capped by the zone value).

### Effects on the other sections

§2 gains the `FrameCopy` before groups 10 (water) and 11 (effects); §3.1 keeps four families and one sampler table (S1); §3.3 item 3 takes the settlement of S2; §5.3 gains the rows C6 (peak plan passes, S2) and C7 (`CopyResource` count of the HDR target, S3); §7 risk 6 is replaced by a pointer to S5. Family rows PL1, PL2, FC1, FC2, QG1, QG2, YB17, YB18 and PC1 are in LIVE-CHECKS-CONSOLIDATED §1.12.

## 7. Cross-cutting risks (INFERRED unless a note proves it)

1. **Silent barrier/aliasing errors** when many passes land at once (shadows, AO, YEBIS, glow each add transients). Mitigation: R10's shadow mode first (graph logs, hand barriers still do the work), then pass-by-pass conversion, aliasing last (review phase E).
2. **Temporal resources across frames in flight** (`AoHistory`, `GlareFinal`, `AfterHist`, `DofTable`, shadow cascade caches): valid on one queue, but every resize/teleport/cut must reset all of them together. Mitigation: one `ResetHistory` event in the plan, tested at the five zones (BA SS7, BB YB6).
3. **The post chain has no baseline cost** (BB §5 phase A). If the sum of AO + mask + YEBIS + AA + glow is large, the order of wave 4 may need the async compute of phase G earlier.
4. **Library cannot be partially run** (BB §7 item 1): the first YEBIS slice already exercises the whole chain; schedule wave 1's YEBIS work as an *infrastructure* slice, not as a quick win.
5. **Engine thread contracts** (R13/R8b): `ShadowFrame`, `AoView`, `YebisFrame`, effect records and the per-view blocks must be immutable snapshots built on the owner thread (consistent across all notes; check each at phase F).
6. **Gates**: quality gates are one table (§6 S5); the zone-file gates (shafts, grading, LUT, shadow variant) are not predicted by the tables: rows YB1, SS1, SH1 are witnesses.
7. **Cost of the plan itself**: ten-plus groups in wave 3 and 4 need the digest row C0 and the debug layer on every run to catch order errors early.
8. **Scope drift**: each family note lists unchecked live points (SH4/SH5, TS1a/b, FX8, SS1–SS12, YB4); none blocks a wave, but each blocks the matching exit check.

## 8. Open points

The measured per-block arena budget (§3.2, phase A); the rows PL1, FC1, QG1, QG2 and C6/C7 (§6); the library's DoF behaviour below the zone quality; the wave order of decals/water/spot lights relative to waves 2–4; whether wave 1 may run before D–E on a single worker (it should, by construction); the placement of this plan in the ROADMAP (Thomas / coordinator).

## Exports

None. Inputs: the notes linked in §1; the native sources only through those notes.

## Phase E sheet

Phase E (the plan with states and aliasing, section 3.3) is prepared in [D3D12-PHASE-E-FRAME-GRAPH-SHEET](../I-0066-independent-native-startup/D3D12-PHASE-E-FRAME-GRAPH-SHEET.md) (inventory of the 16 pass slots and of the targets with their lifetimes and alias sets, the `FramePlan` extension, commits E0 to E10, tests and witnesses). Corrections it makes to this plan: the "FrameCopy" of section 2 group 9b is the existing scene colour copy, a persistent cross-frame object, not a transient; the movie and the menu views run before the main view in the code while group 14 places them in the UI (OPEN there).

## Graphics options sheet

The game's graphics options (the twelve quality levels, the display keys and the brightness) are inventoried option by option in [D3D12-PHASE-G-GRAPHICS-OPTIONS-SHEET](../I-0066-independent-native-startup/D3D12-PHASE-G-GRAPHICS-OPTIONS-SHEET.md): what each changes in the engine, what native does today, what it recreates (change classes P/G/R/H). It corrects the SSAO gate of section 6 S5 as SSAO-WAVE1-READY does (the AO pass runs from LOW). "Phase G" in this plan keeps its meaning (queues and API options).
