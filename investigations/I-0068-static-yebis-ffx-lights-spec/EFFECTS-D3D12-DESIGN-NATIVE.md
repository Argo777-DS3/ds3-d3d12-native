# I-0068 part AV: the native D3D12 design of the FFX effect system (particles, ribbons, distortion, effect lights, foliage wind)

Request (coordinator, 2026-10-11): prepare the effects workstream. Starting from [EFFECTS-ENTRY-DOOR-NATIVE](EFFECTS-ENTRY-DOOR-NATIVE.md), the glow chain ([GLOW-CHAIN-NATIVE](GLOW-CHAIN-NATIVE.md), the FFX twins) and the summon signs ([EFFECT-FAMILIES-SIGNS-NATIVE](EFFECT-FAMILIES-SIGNS-NATIVE.md)), write a native D3D12 design of the FFX effect system: particles, ribbons, distortion, effect lights, foliage wind. Take inspiration from Elden Ring for the architecture (D-0157). Ordered steps, witnesses on the five effect zones (m37, m31, m51, m50, m45), live rows. Static only: nothing launched, no native code edited.

Labels: STATIC (decompilation, DXBC reflection, MSB and params of the retained game data), OBSERVED (earlier captures, cited), INFERRED, OPEN. Evidence: `evidence/I-0068/queries/effects/root-signature-superset.txt` (register usage of the 328 FFX programmes, from `ffx-program-layouts.tsv`).

## 0. Answer first

1. **Keep the engine's effect CPU code, replace only what talks to the GX device.** The creation door, the SfxIns objects, the FXR property evaluation, the emitters, the vertex generation and the wind and light hosts are engine CPU code (ENTRY-DOOR §0, PHASE1 rules CALL / TRANSCRIBE / NATIVE). That part of the plan is settled and unchanged.
2. **D-0157 changes the rest: do not copy GX's immediate-mode draw.** In the original, a sorted pass command executes a callback (`FUN_140299E40`) that builds a draw context and, per draw, calls slot 27 (`FUN_140264230`) to choose the programme, states and constants, then issues the GX statements. The native design splits this in two: a **record** phase (CPU, the same engine code: vertices to a frame arena, one typed `EffectDrawRecord` per draw) and an **encode** phase (native D3D12, one `PassKind::Effect` per engine pass of the frame plan). Every count of the live witnesses (68 patch-list draws at m37, ...) becomes checkable on the records, without a GPU.
3. **One root signature, one PSO cache, one arena.** The 328 non-compute FFX programmes use only: VS b0, b1, b9; DS b0; GS b9; PS b0, b1, b6, b9, b11; textures t0..t7; samplers s0..s9 (measured, §2.2). A single graphics root signature covers all of them; PSOs are keyed by the resolved state tuple and prewarmed; the 2 MB ring becomes a per-frame arena retired by fence.
4. **Wind and lights do not wait for any draw.** The foliage wind (FOLIAGE-NATIVE F2) needs only the door and the per-frame library update (steps N1 to N3); the effect lights need the same plus the existing point-light consumer. They are the first visible results and the best early witnesses.
5. **Elden Ring is an architecture reference only.** The ER folder holds no study of its effect system (a search for FXR, FFX, particle, VFX and SFX finds nothing outside generic text). What it does document, and what the design uses, is its D3D12 submission structure (`DS3-ER-RENDERER-BOUNDARY-MAP`): typed operation records built above the native encoder, resource barriers finalised **before** native encoding, queue `ExecuteCommandLists` + `Signal`, and bookkeeping retired after the fence. Nothing about ER's effect behaviour is used as DS3 evidence.

## 1. What stays the engine's, and what becomes native

| Layer | Original | Native |
|---|---|---|
| Requests (objects, characters, events, signs, FXR child spawns) | `SprjSfxImp` door `E6DCD0/E6DA30/E6DBC0` | **engine code, unchanged** once `+0x58` is published and the `7CB820` deferral is deleted (ENTRY-DOOR D1 D2) |
| Per-frame update | `E6E750` → SceneCtrl slot 8 `24D000`: instances, emitters, hosts (point-light host fills `[[GXRS]+0x41D0]+0x10`) | engine code; the GX-touching statements transcribed (PHASE1 §5) |
| Draw submission | scene graph → SceneCtrl draw `14024E790` → library draw → `GXFfxDrawPass` slot 4 records sorted pass commands → callback `140299E40` at pass execution | **split**: record job per engine pass (CPU, engine renderers) + native encode (new) |
| Programme choice, states, constants | draw-context slot 27 `140264230`, per draw | the same function computes the values (CALL/TRANSCRIBE); results stored in the record instead of bound |
| Vertex generation | builders `385F70` (patch lists), `396F30` (triangle list), `39E900` (`Draw(4)`) writing a 2 MB GPU ring | the builders write into the **frame arena** (map/unmap native); topology and strides from the PSO key |
| GPU particles | `2A6A30` simulation, `2A7E70` draw, append/consume buffers | transcribed to compute + `ExecuteIndirect` (E07 §5) |
| Frame copy | pipeline `+0x1108` copy at the start of pass 0x29 | one copy per frame, only when a record asks for it (§2.4) |

## 2. The design (D-0157)

### 2.1 Record then encode

`EffectDrawRecord` (native struct, one per original draw), filled where the builder is called:

| Field | Source |
|---|---|
| engine pass id | the pass the sorted command was recorded for: 0x21, 0x29, 0x2D, 0x20, 0x31, 0x39, 0x45 (never 0x3A; PHASE2 §3) |
| sort key | the library's own order (kept: it is the engine's depth and layer sort) |
| programme set | VS/HS/DS/GS/PS indices into the 158-entry shader set per stage (slot 27) |
| resolved states | blend type (via the `141ACABF0` table, SIGNS §3: `Unk0` = additive), depth/raster bank ids, topology kind, render-target set (1, or 2 for the glow twins) |
| geometry | arena offset, size, stride (from the input layout), index offset and format when indexed, draw count |
| constants | arena offsets of the VS/DS/PS common images, `cbFog`, light and SH buffers, `gcbSceneParam` where used |
| textures | up to 8 SRV keys (`t0..t7`) and sampler bank ids |
| flags | `needs_frame_copy`, `needs_depth`, `is_glow_twin`, GPU-particle system id |

The record list per frame is the **immutable input** of `PassKind::Effect` passes in the existing native `FramePlan` (`native/renderer/frame_plan.h`: explicit semantic order, versioned resources, dependency checks). New plan resources: the frame arena, the frame copy, the shared depth view, the glow target. The plan compile already checks read-after-write on them; the ER lesson (barriers finalised before encoding) is therefore the plan's job, not the effect code's.

Gains: (1) record counts are comparable with the retained measurements without a GPU (the 68, 5 and 6 of §6); (2) `needs_frame_copy` is known before encoding, so the copy disappears for frames without distortion; (3) the record job can run on a worker thread while the previous frame encodes; (4) a state tuple never seen before is known one frame ahead, so its PSO can be compiled off thread.

### 2.2 One root signature, a PSO cache and an arena

Measured over the 328 non-compute programmes (`root-signature-superset.txt`):

| Stage | Constant buffers (slot: buffer, reflected size) | Textures / samplers |
|---|---|---|
| VS | b0 `VSCommonBuffer` 448; b1 `fcbVolumeRenderParam` 224 (4 programmes); b9 `gcbSceneParam` 448 (one); structured t0..t1 for the GPU-particle VS | none besides particle structured buffers |
| HS | none | none |
| DS | b0 `DSCommonBuffer` 448 | none |
| GS | b9 `gcbSceneParam` 448 (the GPU-particle spark expansion) | none |
| PS | b0 `PSCommonBuffer` 560; b1 `PSLightBuffer` 400 (44), or `fcbVolumeRenderParam`/`fcbFluidParam`; b6 `cbFog` 224; b9 `gcbSceneParam` 448; b11 `cbCubeEnvSH1` 48 | t0..t7; s0..s9 (s9 = linear wrap observed on the patch-list draws, PHASE2 §6.1) |

- **Root signature**: root CBVs (GPU virtual addresses into the arena) with per-stage visibility, one SRV descriptor table (t0..t7), one sampler table. Slot discovery per programme disappears; unused registers cost nothing.
- **PSO cache** keyed by {VS, HS, DS, GS, PS, input layout id, blend type, depth bank, raster bank, topology type, RT formats and count}. Prewarm the eight E01 programmes first (frequency ranking, PHASE2 §6); compile unseen tuples asynchronously and skip a draw (counted, logged) rather than hitching, only in a diagnostic mode; the reference mode waits.
- **Frame arena** replaces the single 2 MB ring: one arena per frame in flight (three), sized from the measured peak (the original's budget is 2 MB per frame, P5), constants and vertices in the same arena, retired when the frame's fence signals (the ER deferred-bookkeeping pattern). The per-draw `UpdateSubresource` of holder buffers vanishes: constants are written once into the arena and bound by address.

### 2.3 Scene inputs as explicit views

- **Depth**: soft particles and the near-camera fades read scene depth. In D3D12 the forward depth can be bound as a read-only DSV and an SRV of the same resource at once (DEPTH_READ | PIXEL_SHADER_RESOURCE), so the effect pass needs **no depth copy**. This is a state the plan encodes; D3D11 forced a copy or an unbound slot (P0.2 asked for a "readable depth": this is how).
- **Colour**: effects blend into the forward HDR target; the glow twins also write GB5 (second render target, blend of RT1 equal to RT0, P0.3 of the checklist, GLOW-CHAIN §3 row `GlowD`).

### 2.4 Frame copy only when needed

Distortion (E04) and radial blur (E05) sample a copy of the HDR frame (programmes 57–63 and 139–145, twins 70, 138, 150–155; E05-E04 §1). Original: unconditional copy at the start of pass 0x29. Native: the record phase sets `needs_frame_copy` on those records; the encode phase issues **one** copy (placed resource, transition RENDER_TARGET → COPY_SOURCE → RENDER_TARGET) only when at least one such record exists in the frame. OBSERVED: at m40 `GXFfxdistortionBump` pairs appear in about 47 of 1,800 frames, so most frames skip the copy. The reference mode (always copy) stays available for the comparison rows (FX5).

Part BD (D3D12-MASTER-PLAN §6 S3): this copy is the **same object** the water surface reads as t55 (`pipeline+0x1108`, taken at the start of forward pass 0x29); one `FrameCopy` service serves both, recorded once per frame after the opaque forward rows only if a consumer exists. The YEBIS input copy is another thing (a clamped draw of the final lit HDR).

### 2.5 GPU particles on explicit indirect arguments

E07 already specifies the translation (E07 §5): ping-pong A/B buffers, UAV counters written for non-negative initial counts, `ExecuteIndirect` for the dispatch and the draw, UAV barriers between emit, update, sort and draw. Design additions under D-0157: the counter and argument buffers become plan resources (so barriers are derived, not hand-placed); the per-system state `S` is a record in the same list (system id, capacity, programme set); an **async compute queue** is possible for the update and sort of the two Snowfield systems (2,048 and 8,192 records) but is deferred: the first version runs them on the direct queue, in order, to keep the comparison with the original exact.

### 2.6 Wind: one frame table from the effect system, items keep the engine chain

Original (FOLIAGE-NATIVE §5): a Grass item registers a 0x180-byte node in the FX node manager `[1449623A0]` (`item+0x760`); each frame `FUN_140D3E950` evaluates the node into a local force object (`FUN_140245F90`), copies up to 8 area winds and 8 point winds into `anim+0x210` and sets `anim+0x620`; the draw state copies that block into `cbvModelParam +0x80`; the vertex shader sums the winds, weighted by the second vertex colour. Native today produces the block as zero (F1, "no wind").

Design: **run the engine chain unchanged** (steps N2, N3 then F2), because the node evaluation depends on the area-wind volumes the engine keeps (**part BM correction: it does not read the node box, it keeps a persistent per-node fade state, so an exact per-region cache is not available**, [FFX-WAVE1-READY](../I-0066-independent-native-startup/FFX-WAVE1-READY.md) §1.2). Two native additions only: (a) the `cbvModelParam` producer reads the block the engine wrote, as it does for the other words; (b) a **per-frame wind table** (area wind id → the evaluated record) used purely as a cache and as a witness, so an item that shares its area winds with another in the same frame (same WindSFX region) is not evaluated twice if the evaluation is proved position independent for area winds (OPEN, INFERRED; row FX8 reads both counts). Wind has no draw dependency: it follows the door.

### 2.7 Effect lights: the existing consumer, one ordering constraint

Effect lights need no new draw (PHASE1 §1): the point-light host fills `[[GXRS]+0x41D0]+0x10` during the library update and native `CollectNativePointLights` reads it. The constraint is ordering: the effect update (`E6E750`, SceneCtrl slot 8) must finish before the light-tile build of the same frame, or effect lights lag a frame. In the plan this is a CPU dependency "effect update → light list" (the list is a plan resource written by the update and read by the lighting pass). Witnesses: 163 entries at the m40 bonfire (59 map + 104 effect, E-D6); per-zone E06 weights below.

### 2.8 Glow twins and signs ride the same machinery

The 44 `…GlowD` programmes write `o1 = r0 × g_psGlowScale` (GLOW-CHAIN §3); in the design they are a render-target-set variant of the same PSO key (two targets), no separate path. The summon signs are E01 + E04 + E06 only (SIGNS §0): no new code, they validate the E01 and E04 paths and the door (E-D9: `7CB820` id 30001, flags 8).

## 3. Ordered native steps, with witnesses

Numbering N1.. follows the checklist's preconditions then adds the design's own layers. Each step lists its check (pass before the next) and the first-choice zone.

| # | Step | Check | Witness zone (why) |
|---|---|---|---|
| N0 | preconditions (checklist §0): forward depth and forward targets [HDR, GB5]; readable depth as DSV+SRV; the model union; NPC bodies | the plan compiles with the new resources | any |
| N1 | publish the effect system `+0x58` (PHASE1 §2–4), view attach, per-frame slot 8 (checklist 1.1–1.2, ENTRY-DOOR D1, D4) | R1, R2, R4; E-D1 | m30 (torch), m40 |
| N2 | delete the `7CB820` deferral; the door runs (ENTRY-DOOR D2, D3, D5, D6) | E-D2 histogram, E-D3 ids ⊆ zone table, E-D4 pending list empty | **m31** (839010 ×485: best statistics), m30, m40 |
| N3 | effect lights through the door and the ordering constraint of §2.7 | E-D6 (163 at m40); native registry count per zone equals the original's | m40; then **m37, m50** (E06 170 each), **m51** (290) |
| N4 | **foliage wind F2**: item chain runs, `item+0x760` valid, `anim+0x620` = 1, counts 1..8 | FX8; the grass sways at the original frequency | **m31** (19 WindSFX regions: 800080-800082), m45 (8: 845010), m50 (6: 850801/2), m51 (3), m37 (1: 800081), m30 (3: 800081) |
| N5 | **record layer, dry**: arena, root signature, state resolution, `EffectDrawRecord` list; **no encode** | FX1 records per pass equal the original callback counts; FX2 builder counts; FX3 arena peak ≤ 2 MB | **m37** (≈68 patch-list draws per frame, strides 112/36/160 ≈ 40/27/1), m31 (5), m51 (6) |
| N6 | encode E01 (tessellated billboards): PSOs from PHASE2 §6, patch-list 1 control point, `needs_depth` read-only DSV | P1–P3, P5; the image: flames, embers, magic over the scene | m37, m31, m40; High Wall torch (T6) |
| N7 | E02 ribbons and trails (triangle list, `tessellateTracer*`): same machinery as E01 | E02 draw counts (m45 132, m37 126, m51 124 action weights); a weapon swing trail | m45, m37, m51; action needed for swing |
| N8 | glow twins as a two-target PSO variant; the filter of GLOW-CHAIN step 8 | FX6: RT1 blend = RT0; GB5 alpha where twins draw | m40, m31 (flames), signs |
| N9 | frame copy on demand + E04 distortion + E05 radial blur (non-tessellated builders) | FX5; L1–L7 of E05-E04 §5 | **m37** (E04 54), **m51** (E05 134, E04 22), **m50** (E05 58) |
| N10 | E03 model particles through the native FLVER path (forward variant, depth test on, write off) | E1–E5 of E03 §5; the m51 grass (34 draws per frame) | **m51** (286), m45 (172) |
| N11 | E07 GPU particles: resources, simulation, sort, `ExecuteIndirect` | G1–G7 of E07 §7 | **m45** (two systems 2,048 + 8,192 records, live counts ≈ 1,710 and 5,130), m40 (sparks) |
| N12 | sign validation: E01 + E04 + E06 per visible sign | E-D9; SG rows of LIVE-CHECKS §1.6 | m30 with an Ember (Argo's save) |
| N13 | exit gate: back-buffer gaps attributed to effects fall to the non-effect residue; 600 frames per witness with the debug layer clean | checklist §9 | all five zones |
| N14 | (optional, after N13) PSO prewarm from the observed tuples, async compute for E07, ~~wind-table cache~~ (dropped, part BM) | FX4, FX7 | m45, m31 |

Steps N5 to N9 are independent of N10 and N11 once N5 exists (checklist §9); N4 is independent of every draw step.

## 4. The five effect zones: what each one witnesses

(Counts: FFX-FAMILIES-PHASE2 §5 zone table = FXR action occurrences in the zone's effect bundles; ENTRY-DOOR §4 = idle SfxIns upper bounds and WindSFX regions; the live draw counts are OBSERVED in the I-0063 `i63-live-10` series.)

| Zone | E01 | E02 | E03 | E04 | E05 | E06 | E07 | Idle SfxIns ids (upper bounds) | WindSFX | Role in the design |
|---|---|---|---|---|---|---|---|---|---|---|
| **m31** Undead Settlement | 564 (+4066) | 42 | 84 | 24 | 36 | 64 | 10 | 839010 ×485, 831021 ×65, 831022 ×44, 800061 ×18 | 19 (800080-800082) | door statistics (N2), **wind** (N4), E01 record count 5/frame (N5) |
| **m37** Irithyll | 1490 | 126 | 106 | 54 | 112 | 170 | 14 | 800044 ×44, 800023 ×12 | 1 (800081) | E01 record count ≈ 68 patch-list draws (N5/N6), E04 (N9), E06 |
| **m51** Ringed City | 2986 (+4664) | 124 | 286 | 22 | 134 | 290 | 14 | 851070 ×124, 839010 ×73, 800020 ×45 | 3 (850800-850802) | E03 model particles and the grass (N10), E05 (N9), most lights (N3), 6 patch draws |
| **m50** Dreg Heap | 2066 (+4664) | 46 | 78 | 12 | 58 | 170 | 4 | 850000 ×6, 850080, 850140 (weak witness) | 6 (850801, 850802) | lights and radial blur; use another zone for the door |
| **m45** Ariandel | 2346 (+4435) | 132 | 172 | 10 | 82 | 132 | 32 | 845220 ×40, 845205 ×30, 845070 ×26 | 8 (845010) | **E07 GPU particles** (the falling flakes), E03, trails |

## 5. Risks (INFERRED unless stated)

1. **Record/encode ordering**: the original executes the callbacks in sorted order at pass execution; if a renderer reads state that a *previous* callback of the same pass changed (draw-context state), moving the callbacks into a record job must preserve the call order. Mitigation: the record job runs the callbacks in the library's sorted order on one thread; FX1 and FX2 compare the sequence.
2. **Shared depth as DSV and SRV**: legal only while depth writes are off for the effect pass (they are, except opaque kinds, PHASE2 §6.1); an opaque effect that writes depth needs the copy path. The record's `needs_depth_write` flag selects it.
3. **PSO count**: the tuple space is larger than the observed set; the cache must not grow without bound (cap and log). FX4 measures the real tuple count.
4. **Wind position dependence** (§2.6) is not proved; the cache is optional and off by default.
5. **Unclassified `7CB750` family** (eight callers, ENTRY-DOOR risk 5): effects with an explicit matrix may add programme tuples unseen in §4.
6. **ER**: the comparison covers architecture only; any claim that ER's effects behave like DS3's would be unsupported.

## 6. Live rows (none taken)

| ID | Orig/Nat | Read | Expected |
|---|---|---|---|
| FX1 | Orig, then Nat | callbacks `140299E40` per engine pass (and native records per pass), sequence of programme indices | equal sets and order; no record for pass 0x3A; 0x39 only for glow-capable entities |
| FX2 | Orig, then Nat | draw count per builder `385F70` / `396F30` / `39E900` per frame at m37, m31, m51 | ≈ 68 / 5 / 6 patch-list draws; strides 112 (≈ 40), 36 (≈ 27), 160 (1) at m37 |
| FX3 | Orig | peak ring bytes per frame over 600 frames, all five zones | ≤ 2 MB (sizes the arena) |
| FX4 | Orig | distinct {programme set, blend type, depth bank, raster bank, RT set, topology} tuples per zone | a small set (sizes the PSO cache; prewarm list) |
| FX5 | Orig | frames with at least one draw of programmes 57–63 / 139–145 | m40 about 47 of 1,800; the frame copy is skipped natively in all others |
| FX6 | Orig | on a `…GlowD` draw: render-target count, blend of RT1, `g_psGlowScale` | 2 targets, RT1 blend = RT0, value per family (GLOW-CHAIN §6) |
| FX7 | Orig | E07 live counts, dispatch groups | m45 ≈ 1,710 and 5,130 records; ≈ 1,770–1,780 groups |
| FX8 | Orig | in a windy zone (m31 first): `item+0x760`, `item+0x690`, `[anim+0x210]` and `+0x214` for 20 Grass items, and the area winds found by `FUN_140245F90`; are two items of one WindSFX region given equal area winds? | `item+0x760` ≠ −1, `+0x690` = 1, counts 1..8; the second answers §2.6 |
| FX9 | Orig, then Nat | `[[GXRS]+0x41D0]+0x10` entries in the effect lights at m37, m50, m51 | equal natively; E06 weights as the scale |
| FX10 | Orig, then Nat | SfxIns active and pending lists, created ids, per zone (E-D3, E-D4) | ids ⊆ ENTRY-DOOR §4; pending 0 after each update |
| FX11 | Nat | position of `E6E750` against the light-tile build in the frame | update first |
| FX12 | Nat | debug layer over 600 frames with the depth DSV+SRV and the on-demand frame copy | clean |
| FX13 | Orig | library instance count against SfxMan handle count | instances > handles when parents spawn children |

## Exports

Reads this note builds on: EFFECTS-ENTRY-DOOR-NATIVE, EFFECTS-IMPLEMENTATION-CHECKLIST, EFFECTS-PHASE1-NATIVE, EFFECTS-PHASE2-E01-E02-NATIVE, EFFECTS-E03-MODEL-PARTICLES-NATIVE, EFFECTS-E05-E04-NATIVE, EFFECTS-E07-GPU-PARTICLES-NATIVE, FFX-FAMILIES-PHASE2, FFX-ATTACHED-NATIVE, EFFECT-LIGHTS, GLOW-CHAIN-NATIVE, EFFECT-FAMILIES-SIGNS-NATIVE, FOLIAGE-NATIVE; native `renderer/frame_plan.h`; ER read-only: `ER Decompilation/evidence/I-0008/DS3-ER-RENDERER-BOUNDARY-MAP.md`. Data: `evidence/I-0068/queries/ffx/ffx-program-layouts.tsv`, `evidence/I-0068/queries/effects/root-signature-superset.txt`.
