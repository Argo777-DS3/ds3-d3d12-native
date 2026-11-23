# Pilot work plan: one ordered plan for the D3D12 follow-ups (K merge, phase D, prepare reduction, render thread, frame graph, options, wave 1)

Request (coordinator, 2026-10-11 18:29 UTC): one ordered plan for the pilot that merges the commit orders of the static sheets, their cross-amendments and the file-conflict map, with a recommendation for every decision. The decisions below are **technical choices of the pilot and the principal technical owner**, not decisions of Thomas; only a change of objective or of the sequencing he fixed would be his (section 7 lists the one place where the plan touches it). Static work: nothing was built, run or measured here; the "state" column reads the repository as of this merge.

Sources (all in `investigations/I-0066-independent-native-startup/` unless noted): [D3D12-PHASE-D-IMPLEMENTATION-SHEET](D3D12-PHASE-D-IMPLEMENTATION-SHEET.md) and [-COMPLEMENT](D3D12-PHASE-D-COMPLEMENT.md); [PREPARE-REDUCTION](PREPARE-REDUCTION-IMPLEMENTATION-SHEET.md); [D3D12-R13-R14-RENDER-THREAD-PACING-SHEET](D3D12-R13-R14-RENDER-THREAD-PACING-SHEET.md) and [D3D12-PHASE-F-SHARED-STATE-SHEET](D3D12-PHASE-F-SHARED-STATE-SHEET.md); [D3D12-PHASE-E-FRAME-GRAPH-SHEET](D3D12-PHASE-E-FRAME-GRAPH-SHEET.md); [D3D12-PHASE-G-GRAPHICS-OPTIONS-SHEET](D3D12-PHASE-G-GRAPHICS-OPTIONS-SHEET.md); [WAVE1-PREP](WAVE1-PREP.md) with its READY notes; [R8B-ENGINE-READS-STATIC](R8B-ENGINE-READS-STATIC.md); [K-COLLECT-IMPLEMENTATION-SHEET](K-COLLECT-IMPLEMENTATION-SHEET.md); the [master plan](../I-0068-static-yebis-ffx-lights-spec/D3D12-MASTER-PLAN.md); the sequencing of [ROADMAP](../../.project/ROADMAP.md) (Thomas, 2026-10-11, D-0157: "phases D-G, before the effects, YEBIS, shadows and SSAO workstreams"). An independent Opus 5.5 pass reviewed the first version; its findings are applied in this one (section 10 lists the deviations from the sheets that the plan makes on purpose).

## 0. Answer first

1. **State.** In `main`: phases A to C, workstream K (collect 108 to 27 ms), the quality object, the static sheets. **On the pilot's branch `claude/dreamy-vaughan-fd4276` and not yet merged:** cD0 (audit counters), cP0 (prepare split), cP1/cP3/cP4/cP8a (shared programme sets, pipelines by state words, palette checks, sampler tables by bank), and cP5 in four commits (shared constant ranges, call-local blocks; `663ca039` fixes a quadratic sweep of the range map that had raised the draw-constants slice from 2.9 to 4.9 ms at Native1304; the value after the fix is in the pilot's runs, not in a document yet).
2. **First action, DONE by the pilot on 2026-10-11 (prepare 26.3 to 16.2 ms, PREPARE-REDUCTION section 11): merge the pilot's branch into `main`**, once the non-regression bundle B0 and the prepare gate of section 6 (PW-0 to PW-2 and the part of PW-3 that applies) have run on it, and with the unit tests that need only `model_draw` (T-P3, T-P5, T-P6) added in or before the merge (the branch adds no T-P test today; T-P1, T-P2 and T-P4 need the cP2 extraction and are due with it). Every other lane edits the same functions (section 4) and the branch is already 7 commits and 10 native files ahead.
3. **Six lanes, one critical path.** Lane D (renderer ring) cD1 to cD10; lane P (prepare) cP2, cP6, cP7, cP9; lane T (threads) cF.1 to cF.7 then cR0 to cR6; lane E (frame graph) E0 to E10; lane O (options) O0 to O3; lane W (wave-1 families) W0a to W10 and waves 2 to 5. Using only the prerequisites that the sheets write, **the critical path to F2 is cD1 -> cD2 -> cD3 -> cD4 -> cR2 -> cR3 -> cR5**; cF (all of it, before cR2), cR0, cR1, and the prepare chain cP2 -> cP6 -> cP7 (cR5 needs cP6 and cP7) run in parallel with it. cR2 after cD4 is the plan's own ordering (decision D-12): the sheets leave open who owns the UI depth, and the receipt, diagnostics and read-back channels are designed twice if cR2 comes first.
4. **F1 alone is worth little; F2 is where the engine thread is freed.** Figures below are ceilings derived on the K base (Native1298: frame about 70 ms, collect 27.2, prepare 24.6, 14.5 frames/s), not measurements. F1 (record and present on the worker) hides at most the render slice that R13 section 7 measured on its M4 base (−3.4 % of 148.6 ms, about 5 ms; about 3.7 ms on a post-phase-D base): on the K base that is at most about 5 to 8 %, minus the UI admission build and the job construction that stay on the engine thread. F2 (prepare also on the worker): the engine thread keeps about 4.8 + 27.2 + 6.2 ms plus the rest of `Clear()`, about 40 ms (25 frames/s), **whatever prepare costs** (prepare sheet, line "With F2"); this assumes cP6 and cP7 are done (S1 to S4 move work into collect, and F6 repeats prepare in diagnostic builds). **Neither the prepare reduction nor cP6/cP7 is in `main` yet.** Ship F1 behind its flag as infrastructure and go to F2; do not make F1 a default.
5. **Twelve decisions with a recommendation each (section 7)**, among them: the order of the movie and the menu views against the scene (keep the code's order through cD3, change it in a separate commit); the `CloseNativeModelStreams` lock fix (in cF.4); B5 keeps the phase D sheet's measure-first guard and becomes the worker-side form before cR3; the worker's wait shape stays the R13 sheet's W2 (W1 the fallback); the OptionChange snapshot after cD0.
6. **Live cost is batched into six witness bundles (section 6)** that run at milestone merges, not per commit.

## 1. State of play

| Item | Where | State |
|---|---|---|
| Phases A to C, R15 switch, B-phase memory work | `main` | done (M4: 6.73 fps) |
| Workstream K (P1-P6, R8a, P4b, collect profile, shadow compare) | `main` (merged) | done (collect 27.2 ms, Native1298 14.5 fps, prepare 24.6) |
| Quality object (Q1-Q8), sampler selector, velocity gate | `main` | done (Native1255/1263) |
| Static sheets: D, D complement, prepare reduction, R13/R14, F, E, G options, wave-1 READY notes | `main` | merged, Opus-reviewed |
| cD0 audit counters | pilot branch | committed `6b2da3c5`, not merged |
| cP0 prepare split | main (merged 2026-10-11, PREPARE-REDUCTION section 11) | committed `9052c2bf` |
| cP1/cP3/cP4/cP8a | main (merged 2026-10-11, PREPARE-REDUCTION section 11) | committed `ea2d3386` (`rebuild-prepare.flag`, `prepare-shadow.flag`) |
| cP5 (shared payloads, call-local blocks, the shared upload ranges measured slower and removed, Native1310-1311) | main (merged 2026-10-11, PREPARE-REDUCTION section 11) | committed `930dad37`, `720829ea`, `663ca039`, `639b7a19` |
| everything else | not started | see the catalogue |

## 2. Commit catalogue

IDs are those of the sheets except where marked **(plan id)**: cP8a/cP8b (the sheet has one cP8), cF.4b, cD3b, and O0 to O3 (the options sheet lists its mitigations M-1 to M-6 and the OptionChange mechanism; the plan names the commits). "Files" lists the hot files first, the full list is in section 4. "B" is the witness bundle of section 6. Prerequisites are **the ones the sheets write**; anything the plan adds is marked *plan order*. State: **done** (main), **branch** (pilot, unmerged), **todo**.

### Lane D: renderer ring ([phase D sheet](D3D12-PHASE-D-IMPLEMENTATION-SHEET.md) and [complement](D3D12-PHASE-D-COMPLEMENT.md))

| ID | Content | Prerequisites | Files | Flag | B | State |
|---|---|---|---|---|---|---|
| cD0 | audit counters; **follow-up** (complement): `present_call_ms`, `create_ms`, `release_ms`, `depth_recreations` read 0 in the game path, `DeviceQueues::Close` under `WaitClose` | none | `backend.*`, `model_draw.cpp`, `scene_pass.cpp`, `primitive_pass.cpp`, `game_bootstrap.cpp`, `gpu_texture_upload.cpp`, `game_texture_ingress.cpp`, `model_draw.h`, pacing row | none | B0 | done (+ follow-up todo) |
| cD1 | `FrameSlot`/`FrameRing` with one slot; recorder objects into the slot; complete fault quarantine; `kept` holds the GPU-referenced frame objects (movie target, view targets, environment cubes); chunk-lag fix or sizing for slots + 1 (complement 4.4, in cD1 and cD6); debug assertion on raw resource arguments | cD0 | `backend.*`, `scene_pass.*` | `frames-in-flight.flag` read | B0, B1 | done (D3D12-PHASE-D sheet, implementation log) |
| cD2 | `ViewContext` replaces `SwapTargetSet` (16 swapped members in the phase D sheet; the phase E census counts 18 swaps in `:1335-1342`; the commit moves whatever the function swaps); per-view RTV heap persistent | cD1 | `scene_pass.*` | none | B0 | done (phase D sheet, implementation log) |
| cD3 | one frame list, arena size classes, **UI depth persistent per extent and held by the slot (D-12)**, `Retire` once, `SubmitIngress` removed from the game path, guards at record time; **the list keeps the code's order (movie, views, main, UI)**; the order change is cD3b (D-1) | cD2 | `scene_pass.*`, `backend.*`, `primitive_pass.cpp`, `game_bootstrap.cpp` | none | B0, B1 | done (phase D sheet, implementation log) |
| cD3b (plan id) | order of the movie and the menu views against the scene (D-1) | cD3 | `game_bootstrap.cpp` (`Clear()`), `scene_pass.cpp` | none | B0 | todo (optional) |
| cD4 | per-slot state: env cube descriptors per slot/view, deferred readbacks, receipt split, diagnostics, slot wait before the lock, **lock scope ends before `Execute`**; **G1 to G4 and R6 are implemented in the R13 forms** (`WitnessChannel`/`TakeCompleted`, worker-only `Diagnostics()`, receipt cell, X3/X6/X10), so cR2 does not redo them (D-12) | cD3 | `scene_pass.*`, `game_bootstrap.cpp` | none | B1 | done except the lock scope before `Execute` (waits for cR1) and the X10 cell (cR3) (phase D sheet, implementation log) |
| cD5 | B5 dynamic streams: **measure first** (D-4) | cD3 (and the cD0 number of dynamic streams) | `game_bootstrap.cpp`, `model_geometry.h`, `model_draw.cpp`, `backend.h`, `scene_pass.*` | `legacy-dynamic-streams.flag` | B1 | done in a pooled form (phase D sheet, implementation log; B5w not needed) |
| cD6 | flip to two slots, startup paths included; per-frame `Complete`/`Drain` removed; chunk lag fix (pool release at the slot's own ticket, or pools sized for slots + 1) | cD4, cD5 | `scene_pass.cpp`, `model_draw.cpp` | `frames-in-flight.flag` | B1 | done: two slots under the flag (phase D sheet, implementation log) |
| cD7 | M5 run (`slots=1` against 2, with a temporary buffer-count flag; verdict provisional until RD12) | cD6 | runners, `game_bootstrap.cpp` | `single-frame.flag` | B1 | done: no gain at S1, default stays one slot (provisional until RD12) |
| cD8 / cD9 | swap chain (buffer count, waitable object), `Present(interval, flags)`, tearing, occlusion; **the pacer lives behind this flag** (D-9) | cD7 | `backend.*`, `scene_pass.cpp` | `legacy-present.flag` | B1 | cD8 done under waitable-present.flag; cD9 waits for RP-1 with cR4 (phase D sheet, implementation log) |
| cD10 | shutdown and resize order, one `WaitIdle` | cD6 | `scene_pass.cpp`, `game_bootstrap.cpp`, `game_texture_ingress.cpp` | none | B1 | Resize part done; the close order goes to phase F (D-10) |

### Lane P: prepare ([prepare reduction sheet](PREPARE-REDUCTION-IMPLEMENTATION-SHEET.md))

The sheet's order is "cP0 first; cP1 before cP2/cP3/cP5; cP2 before cP3/cP4/cP6". **The pilot's branch deviates on purpose**: it edited in place (cP3/cP4/cP5 before the cP2 extraction). cP2 then extracts what is stable (after the merge); the unit tests that need the extraction (T-P1, T-P2, T-P4) are due with cP2.

| ID | Content | Prerequisites | Files | Flag | B | State |
|---|---|---|---|---|---|---|
| cP0 | split and counters (main vs offscreen) | K | `game_bootstrap.cpp`, `model_draw.cpp`, pacing row | none | B3 | done |
| cP1 / cP3 / cP4 / cP8a (plan id) | shared programme sets, pipelines by state words, palette-check cache, sampler tables by bank | cP0 (sheet: cP1 before cP3; cP2 before cP3/cP4: deviation above) | `model_draw.{h,cpp}`, `game_bootstrap.cpp` | `rebuild-prepare.flag`, `prepare-shadow.flag` | B3 | done |
| cP5 | shared payloads and ranges (call-local blocks, shared upload ranges measured slower and removed at Native1310-1311: each draw copies the shared payload) | cP1 | `model_draw.cpp`, `game_bootstrap.cpp` | same | B3 | done |
| cP2 | row set-up, resource plan (PR-7), extraction of the producers into a linkable unit (`prepare_cache`) | cP0, cP1 | `prepare_cache.*`, `game_bootstrap.cpp` | same | B3 | todo |
| cP6 | model-level values on the snapshot (= R8b S1-S3: group state, wind, material scalars, E8 cached) | cP2, K | `game_bootstrap.cpp`, `prepare_cache.*` | same | B3 | todo |
| cP7 | forward light and frame lighting off the engine (= S4) | cP6 | `game_bootstrap.cpp`, `prepare_cache.*` | same | B3 | todo |
| cP8b (plan id) | descriptor batching; option B (persistent SRV tables) is a separate sheet, "after phase D" | phase D (the sheet says only that) | `model_draw.cpp` | same | B3 | todo |
| cP9 | M-run, gain table | cP7 | runners | | B3 | todo |

### Lane T: threads ([R13/R14 sheet](D3D12-R13-R14-RENDER-THREAD-PACING-SHEET.md), [phase F sheet](D3D12-PHASE-F-SHARED-STATE-SHEET.md))

The phase F sheet puts the pre-flight commit **cF (cF.1 to cF.7) before cR2**; cF.4, cF.5 and cF.7 condition F1, cF.1 to cF.3 and cF.6 condition F2. The sheet gives no order inside cF.

| ID | Content | Prerequisites | Files | Flag | B | State |
|---|---|---|---|---|---|---|
| cF.4 | lock order document + assertion helper; **`CloseNativeModelStreams` fix** (D-2) | none | `game_bootstrap.cpp`, new header | none | B0 | done except the assertion helper (with cR3) |
| cF.4b (plan id) | the phase F sheet's "preferred follow-up": move the menu-view fallback collect out of `Clear()`; **not a prerequisite of F2** (F sheet section 4, item 14) | cF.4 | `game_bootstrap.cpp` | none | B0 | todo (optional) |
| cF.5 | `Test-ThreadAffinity.ps1` allow-list over a named function list | none | `scripts/` | none | none | done (R13 sheet, implementation log) |
| cF.7 | exit hygiene: worker object and `renderer` leaked or lock-free destructors; injectable exit seam (D-10) | none | `game_bootstrap.cpp`, `scene_pass.cpp` | none | B2 | done (R13 sheet, implementation log) |
| cF.1 / cF.2 / cF.3 / cF.6 | thread-local flag out of prepare, atomic counters, witness file writes out of prepare, leaf-call lighting programmes (F2 only) | none | `game_bootstrap.cpp` | none | B0 | todo |
| cR0 | thread ids rows, `worker_busy_ms`, `queue_wait_ms`, `input_to_present_ms`, atomic counters (X9) | none | `game_bootstrap.cpp`, pacing row | none | B2 | done in part: TM-1, TM-4, TM-6 (R13 sheet, implementation log) |
| cR1 (= cD-F) | `DeviceQueues` hardening: `submit_mutex_` incl. the ingress swap, per-wait events, re-wait loop | none (self-contained) | `backend.*` | none | B2 | done (R13 sheet, implementation log) |
| cR2 | job-safe renderer inline: X1 (UI snapshot object that **refers to** the slot's persistent UI depth, D-12), X2 (scene copy in the job), X7 (`WorkerMirror`); X3/X6/X10 already done by cD4; `FrameJob` built and executed inline | cF (all); *plan order*: cD4 | `scene_pass.*`, `primitive_pass.*`, `game_bootstrap.cpp` | none | B0 | done except X10 (cR3) (R13 sheet, implementation log) |
| cR3 | `RenderWorker` (own unit), depth-1 queue, **wait W2 (R13 recommendation), W1 the fallback** (D-8), fault channel, quiesce, whole-frame job, clock after the post | cF.4, cF.5, cF.7, cR0-cR2, cD1-cD4, and B5w or `legacy-dynamic-streams.flag` (R13 3.4) | `render_worker.*`, `scene_pass.*`, `game_bootstrap.cpp` | `render-thread.flag` | B2 | done (R13 sheet, implementation log; RT-2, RT-4 in part, RT-8, RT-9 open) |
| cR4 | `FramePacer` on the worker | cR3, cD8/cD9 | `frame_pacer.*`, `scene_pass.*` | cD9's flag | B1, B2 | todo |
| cR5 (F2) | prepare on the worker | cR3, cP6, cP7, cF.1-3, cF.6 | `game_bootstrap.cpp` | `prepare-thread.flag` | B2, B3 | todo |
| cR6 | M8 run, defaults | cR5 | runners | | B2 | todo |

### Lane E: frame graph ([phase E sheet](D3D12-PHASE-E-FRAME-GRAPH-SHEET.md))

| ID | Content | Prerequisites | Files | Flag | B | State |
|---|---|---|---|---|---|---|
| E0 | rows: heap tier, allocation info, view extents, barrier calls | none | `game_bootstrap.cpp`, `scene_pass.cpp` | none | B4 | done (phase E sheet 11) |
| E1 | `FramePlan` extension (pure code, tests, port of the retired plan control) | none | `frame_plan.*`, `frame_graph.*` | none | B4 | done |
| E2 | shadow mode (state at each use) | E1 | `scene_pass.cpp`, `primitive_pass.cpp` | `graph-shadow.flag` | B4 | done |
| E3 | pass-by-pass conversion (lighting, forward, velocity, composition, UI, movie, views); reads the order flags of D-1 in `game_bootstrap.cpp` | E2; **views and movie parts: cD2, cD3** (sheet: "E0 to E2 are independent of phases D and F; E3 for the main view can start before phase D") | `scene_pass.cpp`, executor, `game_bootstrap.cpp` | `frame-graph.flag` | B4 | done (D-0171: default) |
| E4 / E5 | batching; declared boundary states | E3 | executor, `scene_pass.cpp` | same | B4 | done |
| E6 | decision gate for aliasing (no code) | E0 | | | | done: E7-E9 not built at 720p (phase E sheet 11) |
| E7 - E9 | placed heaps, aliasing barriers, views share storage | E6 yes, E5, cD3 | `scene_pass.*`, `backend.cpp`, `game_bootstrap.cpp` | `alias-targets.flag` | B4 | optional |
| E10 | M-run | | | | B4 | optional |

### Lane O: options ([options sheet](D3D12-PHASE-G-GRAPHICS-OPTIONS-SHEET.md))

| ID | Content | Prerequisites | Files | Flag | B | State |
|---|---|---|---|---|---|---|
| O0 (plan id; M-1) | **the Shade gate** (skip the SSS profiles when `+0x75` is clear, in prepare at `ProduceNativeSssInputs`). **Not behaviour-neutral**: images change below Shade MEDIUM (the original's behaviour); it adds a fourth direct read of the levels that O2 later removes | none | `game_bootstrap.cpp` | none | B5 | done (options sheet, pilot results; Native1374-1378) |
| O1 (plan id; M-5, M-3) | keep the last valid sampler bank on a transient read failure; a log row when the configured size S differs from `output_width` | none | `game_bootstrap.cpp` | none | B5 | done (options sheet, pilot results) |
| O2 (plan id) | `OptionChange`: one snapshot per frame in the frame input (the options sheet places it in the existing frame entry; the consumers are on the collect side, so no job is needed first) | cD0 merged | `game_bootstrap.cpp`, `scene_pass.*`, `model_draw.cpp` | none | B5 | done (options sheet, O2 results; Native1454-1458) |
| O3 (plan id) | witness prerequisites: `Run-OptionSweep.ps1`, total-PSO and sampler-table counters | cD0 merged | runners, `model_draw.cpp` | | B5 | done except the total-PSO and sampler-table counters; first sweep of every level Native/Reference1374-1378 (options sheet, pilot results) |

### Lane W: wave-1 families ([WAVE1-PREP](WAVE1-PREP.md) 4.2 and the READY notes)

W0a (draw envelope), W0b (`NativeViewEffectsInput`), W1/W2 (Scaleform), W3 (gamma), W4 (effect lights), W5a-f (pure producers), W6 (forward GB5), W7 (tessellation), W8 (AO resources), W9 (shadow atlas), W10 (YEBIS). Their own prerequisites are in WAVE1-PREP and the master plan, which says wave 1 "needs A-C only; … runs in parallel with D-E". Cross-lane constraints: **W0a after the pilot's branch is merged** (same fields of `NativeModelDrawInput` and the pipeline key as cP1/cP3; it also edits `EnvelopeMatches` at `scene_pass.cpp:1362-1366/1551-1562` and the forward target arrays of both paths, MODEL-DRAW-ENVELOPE-READY); **W0a and W3 (composition, presentation, start-up frames, `Resize`: GAMMA-COLOUR-PATH-READY) collide with cD1/cD2/cD3/cD8/cD9/cD10/E3 in `scene_pass`**, so they land after the structural lane D commits or are merged alone and re-anchored (rule (c) of section 5). Waves 2 to 5 of the master plan need E3 to E5 and the single list (cD3). **Sequencing:** ROADMAP (Thomas, D-0157) puts phases D to G before the effects, YEBIS, shadows and SSAO workstreams; the plan therefore does not start the wave-1 slices of those four workstreams in parallel with lane D. Starting them earlier would change the order Thomas fixed and is his call.

## 3. Dependency graph (only the prerequisites of section 2; plan order marked)

```
 main now ---- merge pilot branch (cD0, cP0-cP5) ----+
                                                      |
 independent, any time: O0, O1, O2, O3, cF.5, E0, E1, cR0, cR1, cF.7
                                                      |
 cD1 -> cD2 -> cD3 -> cD4 --(plan order)--> cR2 -> cR3 -> cR4 (also needs cD8/9)
                |      \                      ^     ^
                |       +-> cD5 (B5) ----------+-----+  (B5w or legacy flag)
                |       \-> cD3b (optional)    |
                |                         cF (all, before cR2)
                |
 cD4,cD5 -> cD6 -> cD7 -> cD8/9 ;  cD6 -> cD10
 cP2 -> cP6 -> cP7 -------------------------------------> cR5 (F2, also needs cR3, cF.1-3, cF.6) -> cR6
 E1 -> E2 -> E3 (views/movie: cD2, cD3) -> E4 -> E5 --> waves 2-5 of the master plan
 W0a (after the merge), W0b, W1-W5 (not before D-G for the four workstreams: ROADMAP)
```

**Milestones.** M-A: the pilot's branch in `main` (now). M-B: cD1-cD4, cF and cR1 done (single list, slot-owned state, locks ordered). M-C: cR2 and cR3 behind its flag (F1 infrastructure). M-D: cP6/cP7 + cR5 (F2: the large gain). M-E: slots=2 default (cD6/cD7 verdict). M-F: E3-E5 (graph drives the barriers) which opens waves 2 to 5.

## 4. File-conflict map and the serialisation it forces

Built from the "Files" column of section 2 plus the READY notes of lane W.

| Hot file | Commits that edit it | Rule |
|---|---|---|
| `native/frame/game_bootstrap.cpp` (collect, prepare, `Clear()`, hooks) | cD0, cD3, cD3b, cD4, cD5, cD7, cD10, cP2, cP6, cP7, cF.1-4, cF.6, cF.7, cR0, cR2, cR3, cR5, E0, E3, E7, O0-O2, W0a, W0b, W3-W10 | keep each diff inside named functions; split "move code" from "change behaviour"; merge within a day |
| `native/renderer/scene_pass.{cpp,h}` | cD0, cD1-cD4, cD3b, cD5, cD6, cD8/cD9, cD10, cF.7, cR2-cR4, E0, E2, E3, E5, E7-E9, O2, W0a, W3, W6, W8-W10 | **serialise the structural ones: cD1 -> cD2 -> cD3 -> cD4 -> cR2 -> E3**; W0a and W3 merge alone and re-anchor |
| `native/renderer/model_draw.{h,cpp}` | cD0, cD5, cD6, cP1-cP5, cP8a (branch), cP8b, O3, W0a, W7 (HS/DS) | the pilot's branch merges first; W0a rebases on it; cD5 and cD6 touch only the stream copy and the chunk pools; cP2/cP6/cP7 move code out of it, they do not edit the draw envelope |
| `native/renderer/backend.{h,cpp}` | cD0, cD1, cD3 (arena classes), cD5, cD6, cD8/cD9, cR1, E8 (GBV flag) | cR1 is self-contained and can go first |
| `native/renderer/primitive_pass.cpp` | cD0, cD3, cR2, E2, E3 | after cD3 |
| `native/renderer/gpu_texture_upload.cpp`, `game_texture_ingress.cpp` | cD0, cD10 (ingress) | small |
| `native/renderer/frame_plan.*` | E1 | low risk |
| new files | `render_worker.*`, `frame_pacer.*`, `frame_graph.*`, `prepare_cache.*`, tests | no conflict |

**Collision pairs to watch.**
- **cD3 and cR2 on the UI depth:** X1 of the R13 sheet makes the UI pass an immutable snapshot that owns its own depth target, while cD3 (P1) makes `primitive_depth_` persistent per extent. Decision D-12 settles it: the slot owns the persistent depth, the snapshot refers to it. This is why cR2 follows cD4 in the plan.
- **cD4 and cR2 on the same channels:** receipt (R6 and X10), diagnostics (G4 and X6), read-backs (G1 to G3 and X3). cD4 implements the R13 forms directly.
- **cD2 and cR2** share no member (cR2's are `primitives_`, `scene_colour_copy_`, the witness members, `Diagnostics`, the mirror and the receipt; none is in the swapped target set, phase E section 2); their order is the plan order above, not a member conflict.
- cD3 and E3 (the executor assumes the single list: E3's views and movie parts after cD3); cP2 against the pilot's in-place cP work (cP2 extracts what is stable, after the merge); W0a against cP1/cP3 (same input struct), and W0a/W3 against cD1-cD3/cD8-cD10 (`scene_pass`); cF.4's `CloseNativeModelStreams` edit against anything in the stream hooks.

## 5. Merge order (a linear sequence that keeps every commit buildable; parallel steps in braces)

1. **Merge the pilot's branch** (cD0, cP0, cP1/3/4/8a, cP5) after B0 and the prepare gate, with T-P3/T-P5/T-P6.
2. {cD0 follow-up, O0, O1, O2, cF.5, E0, E1, cR0, cR1, cF.7} (independent, small, any order; O0 and O1 carry witnesses).
3. cF.4, then {cF.1-3, cF.6} (the whole of cF precedes cR2; cF.4b optional).
4. cD1, then cD2, then cD3, then cD4 (each merged alone; rule (c)).
5. {cD5 (after the cD0 stream count), E2}, then {cP2, cP6, cP7} in parallel with step 6.
6. cR2 (inline), then cR3 behind `render-thread.flag` (B2 bundle), then {cD6, cD7}.
7. cR5 (F2) behind `prepare-thread.flag`; cR4 and cD8/cD9 together (one present path).
8. E3 to E5 (the graph drives barriers) while cD6/cD7 are measured; E6 decision; cD3b if D-1 says so; W0a and W3 at their slots once the lane D structural commits are in.
9. cD10, cP8b, cP9, cR6, then E7-E10 only if E6 says yes.

**Rules.** (a) A commit merges into `main` as soon as Test-Project and its unit tests pass and the bundle part it touches has run on it; the flag keeps the old behaviour. Bundles are played at milestones, not per commit (section 6): a commit that waits for its bundle stays on the branch for the shortest possible time. (b) `git merge main` into the working branch before each merge; target at most a few days of drift, and merge as soon as the bundle part has passed rather than waiting for a full bundle. (c) A commit that changes a hot file's structure (cD1, cD2, cD3, cD4, cR2) is merged alone, never bundled, and gets its own B0 pass (the structural commits of lane D only need B0 plus the part of B1 they touch). (d) Live runs follow the D-0116 protocol (announced start time and estimated duration in Paris time, then "PC libre"). (e) The static thread supplies a short "re-anchor" note when a merge moves line numbers the sheets cite (they cite functions first).

## 6. Witness bundles (batch the live runs)

| Bundle | Content | Covers | Rows |
|---|---|---|---|
| B0 | the non-regression set: Native1254 in game (High Wall), title and menus, Status and portraits, **character creation, a Bink movie, an area change**, velocity Native1191, NPC and cloth Native1184/1185, quit Native1262; once with the debug layer (600 frames, 0 errors) and once without for timing; the K digest and `prepare-shadow` mismatch counters; the K witnesses that STATE lists as not yet played (K sheet 5.2) | every structural commit | A-C set (phase D sheet 245) |
| B1 | slot and present set: RD1-RD5, **RD6-RD9 (resize, cadence, shutdown, soak)**, RD10-RD12, **RB5-1/RB5-2 (dynamic streams)**, `slot_wait_ms`, `present_call_ms`, the 3-buffer variant | cD1-cD10, cR4 | phase D sheet + complement |
| B2 | thread set: RT-1..RT-9, **RP-1..RP-3 (pacer)**, RF-1..RF-5, the lock-order helper counters, quit/area change/movie close with a job in flight | cR0-cR6, cF | R13 + F sheets |
| B3 | prepare set: PW-0..PW-4, R8B-1, 2, 3, 4, 6 (**R8B-5 is a reference observation of Thomas's own run only** and is not in the bundle), the split rows on a world zone, a character zone and a menu zone | cP*, cR5 | prepare sheet |
| B4 | graph set: EW-0..EW-5 (shadow mode first) | E* | E sheet |
| B5 | options set: OP-0..OP-7 (restricted sweep) | O* | G sheet |

**Merge gate of step 1** is B0 plus PW-0, PW-1, PW-2 and the part of PW-3 that applies to the branch; PW-4 (the M-run of cP9) and the R8B rows belong to cP6/cP7/cP9.

## 7. Decisions: recommendation for each

All are technical choices for the pilot and the principal technical owner, not decisions of Thomas. "Reversible" means a flag or one commit undoes it.

| Id | Decision | Recommendation | Why | Reversible |
|---|---|---|---|---|
| D-1 | Order of the movie and the menu views against the scene in the single list. The code runs movie then views then the main view (`Clear()`, `game_bootstrap.cpp:725-726`); NATIVE-PASS-ORDER step 14, phase E and the master plan place the views and the Bink in the UI group, after the scene (phase E section 2 keeps it OPEN) | **cD3 keeps the code's order** (no behaviour change in the structural commit); a separate optional commit **cD3b** switches to the original's order (main scene, then views and movie, then UI), judged by B0 portraits and Bink. This amends the phase D sheet 3.2 and cD3 and the phase E lifetimes (views then read the current-frame scene copy, one cross-frame read fewer, shorter transient lifetimes) | the structural commit stays equivalent; the image-changing step is isolated and witnessed | yes (one commit) |
| D-2 | The lock cycle `gpu_submissions` vs `model_stream_lock` | **cF.4: `CloseNativeModelStreams` sets its flag under its lock, releases, then takes `gpu_submissions`**; the fallback collect leaves `Clear()` only as cF.4b (preferred follow-up, not needed for F2) | smallest fix; both orders then form one chain | yes |
| D-3 | OptionChange | **O2 right after cD0 merges**, with O0/O1 first; the snapshot lives in the frame entry that already exists and is carried in the job once cR2 exists | the consumers are on the collect side; no dependency on the job | yes |
| D-4 | B5 (dynamic streams) | **keep the phase D sheet's measure-first guard**: cD5 starts with the cD0 count of dynamic streams per frame; if small, B5a (prepare-time range) is enough in phase D and the worker-side form **B5w** (R13 3.4: the worker allocates the arena range and records the copy from the job's immutable stream versions) is built before cR3, which until then requires `legacy-dynamic-streams.flag`; if the count is large, go straight to B5w, which is valid under every threading variant. B5a is valid again under F2 where prepare and record share the worker (R13 cR5) | the R13 sheet amends phase D 5.2 only for prepare and record on different threads; B5w carries the open copy-state rule of T-D8, which B5a avoids | `legacy-dynamic-streams.flag` |
| D-5 | slots default | **keep `slots=1` as the default until M5** (cD7), judged with the 3-buffer variant | on a post-phase-D base the gain is a few ms (R13: the GPU wait is already removed by cD6) | flag |
| D-6 | aliasing (R11) | **postpone E7-E10 until E0's numbers**; do E1-E5 first | memory is not the constraint on the test adapter (E0 confirms) | n/a |
| D-7 | clock after the post | **adopt the original's order at cR3**, with RT-9 | the original does it (EEE190/EEE160); the witness decides | flag |
| D-8 | worker wait shape | **W2 as in the R13 sheet 8.1** (`MsgWaitForMultipleObjects` with the pump and the proposed 30 s deadline), **W1 (original-faithful: unbounded wait for the replay, then 30 ms slices with the pump) as the fallback if RT-6 shows re-entrancy trouble.** T-R1, T-R3 and RT-6 are written for W2 and stay valid; if W1 is ever chosen, T-R3 loses its deadline and RT-6 changes | the first version of this plan inverted the sheet without saying so; there is no new evidence for it | yes |
| D-9 | pacer flag | **one flag, cD9's `legacy-present.flag`** | avoids two overlapping switches | yes |
| D-10 | process exit | **leak the worker object and the renderer (as RCA879 does for `ui_sessions`)**: no destructor runs at `ExitProcess`, which kills the other threads before static destructors; the lock-free destructor variant is the fallback for builds that must clean up | smallest hazard; matches the phase F exit semantics | yes |
| D-11 | who owns F1 as a default | **F1 stays infrastructure behind `render-thread.flag`; the default flips only with F2** (done: D-0168, 2026-10-12, F1 and F2 by default) | F1 alone hides a few ms at best | flag |
| D-12 | Who owns the UI depth and the receipt/diagnostics/read-back channels (phase D P1/G1-G4/R6 against R13 X1/X3/X6/X10) | **the slot owns the persistent UI depth per extent (cD3); the cR2 snapshot refers to it. cD4 implements the channels in the R13 forms.** cR2 follows cD4 | designing them twice is the one real design conflict between the two sheets (phase E lists it as open) | yes |

**Where the plan touches Thomas's sequencing:** only the lane W point of section 2 (wave-1 slices of the effects, YEBIS, shadows and SSAO workstreams do not start before D-G unless he lifts the ROADMAP order). **Information requested from Thomas (not a decision):** the output resolution he intends to run (720p, 1080p, 1440p or above). It changes the value of aliasing (E6) and the weight of the ceilings in the sheets; no code depends on it.

## 8. Quick wins independent of the order

O1 (keep the last valid sampler bank; a log row when the configured size S differs from `output_width`), O2/O3 (after cD0), cF.5 (allow-list script), E0 (rows), E1 (pure plan code), cR0 (rows), cR1 (queue hardening), cF.7 (exit hygiene), cD0 follow-up counters. Each is small and unblocks a later lane; the pure-code and row commits are behaviour-neutral, cR1 and cF.7 are gated or invisible in normal runs. **O0 is a quick win but not neutral** (it changes images below Shade MEDIUM; B5 witness).

## 9. Risks of the plan, and how to keep it current

1. **The plan is only as fresh as the pilot's branch:** the "state" column is read from the repository at the merge of this note; the pilot updates the state column and the milestone line when a commit merges (one line per commit in `STATE.md`).
2. **Measured outcomes can remove commits:** M5 (no gain: keep `slots=1`), E6 (no aliasing), a prepare split after cP6 that shows F2's worker below the engine thread without cP7, the cD0 stream count (B5 form), the first wave-1 GPU numbers. The plan lists the gates; it does not assume their results.
3. **Hot-file drift** is the main schedule risk (section 4); the merge rules in section 5 are the control.
4. **Wave-1 families are owned by other threads;** this plan only fixes their interface with the ring, the graph and `model_draw`; coordinate W0a and W3 with the pilot before either lands.
5. **Not covered:** the parallel scene pool for the collect (a separate sheet once the K caches are measured), the COPY/compute queues, the encrypted Options-page semantics.

## 10. Deviations from the sheets that the plan makes on purpose, and evidence

- The pilot's in-place order in lane P (cP3/cP4/cP5 before cP2) against the prepare sheet's order; tests due as written in section 2.
- cR2 after cD4 (D-12) where the R13 sheet only requires cD1 to cD4 for cR3 and says nothing about cR2's position.
- cD3 keeps the code's order of movie and views (D-1), with the original's order as the optional cD3b.
- cD4 implements G1-G4/R6 in the R13 forms.
- The merge gate of the pilot's branch is B0 plus part of the prepare gate, not the sheet's whole gate.
- O0 is placed as a quick win with its own witness although it changes images (the options sheet's M-1).

Repository state: `main` at the merge of this note; the pilot branch `claude/dreamy-vaughan-fd4276` log and diffstat (`git log main..claude/dreamy-vaughan-fd4276`: 7 commits; 10 native files; `main` has not moved on `native/` since the merge-base `933a9431`, checked by the reviewer). The commit contents and prerequisites are read from the sheets listed at the top; nothing here was built, run or measured, and no pilot result after Native1298 is quoted except the one commit message.

## Amendments by the engine step and thread map

[ENGINE-STEP-THREAD-MAP](ENGINE-STEP-THREAD-MAP.md) adds to the plan: (1) cR0 carries the passive rows TM-1 to TM-5 of the map (thread-id sets for the model-storage, pose, stream, cloth and retirement hooks, Ez task steps for the other task classes, one captured stack at the frame hooks); (2) cF.5's owner-only list is `Start`, `BeginFrame`, `UpdateNativeSceneCpu`, `NativeRenderStep`, `EndFrame`, `Clear`, `Post`/`Quiesce`, and every other engine hook is "pool-capable or unknown" until TM-1 narrows it; (3) the F2 gate is unchanged and its reason is now observed (the character steps run on up to six threads and rows 80 to 87 of the same frame follow `EndFrame`, so S1 to S4 snapshots are required and the retirement hooks must respect the kept set).

## Amendment by D-0167: the engine-thread budget at S1 and the order toward 60 frames/s (pilot, 2026-10-12)

Thomas's targets (D-0167): 60 frames/s first (16.7 ms a frame), then an unlocked frame rate. Measured at S1 after cR3 (Native1371, High Wall, render worker on, no debug layer, medians of 120-frame windows; `native_collect_split` stamps and `native_prepare_split`): **the engine thread spends about 55 ms a frame (18.1 frames/s)**, the GPU and the worker are not the limit (worker job 2.5 ms, GPU wait 1.2 ms).

| Engine-thread slice | ms | Largest parts |
|---|---|---|
| frame begin to update (engine) | 3.5 | engine |
| collect (`UpdateNativeSceneCpu`) | 26.5 | MeshBlocks 7.0, MeshTextures 5.8, ModelAdmission 5.0, Teardown 2.4, MeshLookups 2.0, MatDyn 1.4, ShaderTex 1.2, Passes 1.2 |
| UI prepare (render step) | 6.6 | Scaleform and the UI frame |
| `Clear()`: prepare | 15.4 | draw constants 2.9, palettes 2.2, other blocks 2.0, samplers/SRVs 1.9, row set-up 1.4, frame blocks 1.1, validation 1.1 |
| `Clear()`: job build | 2.1 | views, movie, UI admission |

Consequences (technical choices of the pilot; ranked by what they remove from the engine thread):

1. **F2 alone does not reach 60 frames/s**: moving prepare to the worker (cR5) leaves about 40 ms (25 frames/s). It stays on the path (it needs R8b S1 to S4, which are also the inputs of point 2).
2. **The largest lever is cross-frame persistence for unchanged models**: C-5 shows 113 of 126 models with unchanged per-frame inputs from one collect to the next. A persistent per-model record (K sheet C7: the admission record, its mesh textures and blocks) and persistent prepared draws (prepare sheet C4) keyed by the snapshot digest would make collect and prepare proportional to the models that changed, not to all models. Both need the R8b snapshot (prepare sheet cP6 = R8b S1 to S3) as their key, with the permanent shadow comparison (rebuild every Nth frame and compare) as the guard.
3. Order adopted: **cP2** (extraction with the `Read` seam, so the snapshot and digest code is testable on the host) -> **cP6** (the snapshot S1 to S3: the key of C4/C7 and the gate of F2) -> **C7 + C4** (persistent records and draws for unchanged models, behind a flag, shadow-compared) -> **cP7** (S4) -> **cR5** (prepare on the worker). Each step is measured at S1 against the 16.7 ms target; the UI prepare (6.6 ms) and the engine's own slices are listed for later, after this chain.
4. **For the unlocked rate**: nothing in this chain may assume a 60 Hz cadence (pacer cR4, clock order RT-9); the locomotion rules come from the LTF-TRTF work as universal rules per mechanism (D-0167).
