# I-0095 study: the replayed path's slow frames, the traversal stutter, and a ranked plan

Read-only study (2026-10-23, Opus 5.5), from the measurements already in `evidence/`: no build, no live run, no Ghidra, nothing
changed in `native/`. For the stutter workstream's pilot (after the memory workstream, I-0096). Statuses follow the ledger:
OBSERVED (read in the cited rows), INFERRED (reasoned from them, not tested), PUBLIC (published documentation or public
knowledge, not verified on this PC).

## 1. What the counts measure (read this first)

- **frame end > 25 / > 34 ms** (`native_frame_times`, the gate's two counts): the interval between two native frame ends on
  the engine side, CPU time. The original's counterpart is its present pacer D5CB0 (`native_original_frames`). At 60 Hz a frame
  of 25-34 ms is one refresh late if nothing absorbs it; one over 34 ms is one refresh late when it is 34-38 ms (the crossing
  frames, timing jitter around two refresh periods) and two or more from about 50 ms.
- **held** (`native_present_refreshes`): frames the display actually kept two refreshes or more (DXGI `SyncRefreshCount`
  between consecutive presented frames). It is meant to be what reaches the screen, but it samples irregular frames badly (§4). **The original has no held count**: its D3D11 swap
  chain is not instrumented, so the gate cannot compare what the player sees between the two renderers.
- **game step** (`game_frame_times`): the interval at DS3's world update gate, the same engine point in both renderers. DS3
  steps its world with the measured interval (the broad-update delta of the timing project, `timing/TERMINOLOGY.md`), so an
  uneven game step can show as uneven motion even when the display misses no refresh (INFERRED; not measured as such).

## 2. Noise (H1) and lows

OBSERVED (`scripts/Get-ReplayMeasure.ps1` on every gate measure run, path totals, 126 s after the trims, about 7,560 frames):

| Runs | frame end >17 / >25 / >34 | game step >17 / >25 / >34 | held (biased, §4) |
|---|---|---|---|
| i83-cal-m1..m4 (2026-10-21) | 1445-1638 / 74-97 / 12-16 | 1063-1080 / 67-81 / 0-4 | 33-51 |
| i90-final to i94-cal, 16 runs (2026-10-22 to 23) | 932-1010 / 79-104 / 9-18 | 1005-1082 / 63-87 / 0-4 | 11-21 |
| i83-ref-original | 945 / 14 / 3 | 780 / 15 / 2 | not measured |

Over the 16 runs since I-0090 (the stutter not targeted by any of them): frames over 25 ms mean 93.8, sd 6.6; over 34 ms mean
12.8, sd 2.0. The gate pairs 87.5, 94 (i94-final), 99.5 differ by 1 to 2 sd: **noise** (H1 supported). Two-run means cannot
separate a change under about 14-16 frames over 25 ms (the within-pair sd is 7.1; a one-way analysis of variance over the
seven builds since I-0090 gives F(6,9) = 0.64 for the frames over 25 ms, 0.34 over 34 ms: no build differs from the others).
Runs per build for a 95 % two-sided test at 80 % power, with the within-pair sd: a change of 10 frames over 25 ms needs about 9,
of 20 about 4; a change of 3 frames over 34 ms about 11 (about 25 if the count were Poisson). The frames over 34 ms vary less than
a Poisson count would (variance over mean 0.30): they are nearly deterministic, one per crossing (§3). Against noise, two
changes since I-0083 are real: the frames over 17 ms (1,533 to 974 on average) and the held count (43 to 15, Welch t = -6);
the frames over 25 ms (84 to 93.8) did not move significantly (t = 1.9). Worker tables: `evidence/I-0095/frames-analysis/`
(`REPORT.md`, `a_stats.txt`, `a_power.txt`).

Two things the totals show that the gate's summary hides:
- **The frames over 17 ms are at the original's level since I-0090**: 932-1010 against the original's 945 (they were 1445-1638 at
  i83). The gap is entirely in the 25 ms band and above.
- **At the shared engine point the gap is larger in the 25-34 band**: game steps over 25 ms 63-87 against the original's 15.

Lows (bounds from the window counts and the per-frame `native_slow_frame` / `original_slow_frame` rows; no per-frame dump exists
for the frames under 34 ms, §8):

| | 0.1 % low (the 8th worst of about 7,560 frames) | 1 % low (the 76th worst) | worst |
|---|---|---|---|
| native (i94-final-m1; the other i9x runs alike) | 36.8 ms, 27 frames/s (exact: 13-14 frames over 34 ms listed) | between 25 and 34 ms: 29 to 40 frames/s (94 frames over 25 ms) | 57-60 ms (class B) when present, else 38 ms |
| original (i83-ref-original) | between 25 and 34 ms: 29 to 40 frames/s (3 over 34, 14 over 25); the crossing frames at 33-34 ms suggest about 30 | between 17 and 25 ms: 40 to 59 frames/s (14 over 25, 945 over 17) | 34.5 ms |

Pooled over the 20 native runs (study worker, `b_summary.txt` in this session's scratch, the band's unseen frames imputed at the
median of the seen ones, an estimate): 1 % low 27.7 ms (36 frames/s), 0.1 % low 36.6 ms (27.3 frames/s, exact); the original
20.1 ms (50 frames/s, estimate within 17-25) and 32.7 ms (30.6 frames/s).

So the 0.1 % low is about 27 against about 30 frames/s, a small gap made by the crossing frame; the 1 % low is where the native
falls clearly behind (36 against 50 frames/s, the 25-34 band), and the worst frame where class B shows.

## 3. The frames over 34 ms (H2): one per cycle, at the crossing, a few ms longer than the original's

OBSERVED (`native_slow_frame` rows inside the motion segments, first 2 s trimmed; runs i83-cal-m1, i94-cal-m1, i94-cal-m2,
i94-final-m1, i94-final-m2):

| Class | Where in the path | Native interval | Native phases | Per run |
|---|---|---|---|---|
| A. The crossing frame | walk: 5.8-5.97 s into each 10 s cycle; sprint: 6.8-7.3 s into each cycle | 34.6-38.3 ms | collect 6.2-8.0 ms (the walk windows' mean is 2.5-3.4 ms), prepare about 1 ms (once in three a render-worker prepare of 9-18 ms beside it), record 1.2 ms, GPU wait 0.06 ms | 10-13 |
| B. Long outliers | walk, no fixed phase (5.45 s, 36.6 s in i83-cal-m1; 30.3 s, 61.3 s in i94-final-m1) | 56-60 ms | collect 3-4 ms, prepare 1.3 ms, record 1.4-2.1 ms: about 50 ms outside the native phases | 0-2 |
| C. Turn prepare | camera turn | 34.8 ms | prepare 30.6 ms (i94-final-m1) | 0-1 |
| D. The first steps after the standstill (inside the gate's 2 s trim, so not counted) | 0.4 and 0.8 s after the walk starts | 42.7-45.1 ms | collect 28.6-30.4 ms | up to 2 (i94-cal-m1, i94-final-m1, i94-final-m2) |

Over all 20 native runs (study worker, 261 frames over 34 ms in the motion, its count untrimmed, so D included): 240 with the collect over 5 ms, classes A and D (mean 37.2 ms, collect
8.5 ms), 17 outside the native phases (class B, mean 49.8 ms), 1 of class C, 3 with a programme analysed or a texture admitted
(34.6-36.5 ms); walk phases: 147 in the 5-6 s bin, 17 in the 0-1 s bin (class D), the rest scattered.

The original on the same path (i83-ref-original, `original_slow_frame`): three frames over 34 ms in the motion, 34.0, 34.5
and 34.5 ms, at 5.9 s, 25.9 s and 45.9 s of the walk: **the same crossing, at the same phase, every other cycle, just over the
threshold** (the other cycles at or just under 34 ms, by its 14 frames over 25 ms). No frame of class B in the original.

Reading (INFERRED from the rows above and I-0081 obs. 2):

- Class A is the area crossing I-0081 found (NPC models initialised, GPARAM banks and environment regions admitted). It costs
  the original about 34 ms too: it is DS3's own work, about two refreshes long in both renderers. The native frame is 2.5-4 ms
  longer, and the native collect is 4-5 ms over its walk mean in exactly those frames (the models entering the view:
  `AcquireNativeModelPrograms`, I-0081 obs. 2). So the gate's « 13 against 3 » frames over 34 ms is mostly the same
  one-refresh-late frame measured at 36-38 ms natively and at 33.5-34.5 ms in the original, on either side of the threshold.
  On screen both should show one held refresh per crossing (the original's held count is not measured: §8 protocol).
- Class B is the native-only stutter that a player would notice most (three refreshes or more; 17 in 20 runs, under one a run), not
  explained by the native phases: a wait on the engine thread (DS3's task join on a loading task, the I-0079 mechanism) or DS3's
  own step slowed. Its cause is not measured; the I-0081 profile (i81-07) mixed it with class A. The original is not free of it
  either: its sprint run i79-18 had one frame of 57.7 ms (I-0079 RESULT table); on the gate path (i83-ref-original) none.
- Class C is a render-worker prepare at the turn: rare.

## 4. The 17-34 ms band: the reload windows of each cycle, and a biased held count (H3)

OBSERVED (per 120-frame window over the walk, i94-final-m1 and i94-cal-m2; the same shape in the other runs read):

- The frames over 17 and 25 ms are not spread evenly: in every 10 s cycle the two windows before the crossing (phase 2-6 s)
  carry 21-52 frames over 17 ms and 2-8 over 25 ms each, the others 1-11 and 0-2. Those same two windows hold, in the journal
  between their timed rows, about 630 `native_model_material_cpu` and as many `native_model_layout_cpu` rows and about 650
  `native_encoded_texture_resident` rows per cycle (i94-final-m1): the walk crosses the same area boundary back and forth, and
  **DS3 unloads and reloads the same map part every cycle**; the native backend rebuilds its materials, layouts and textures
  each time (I-0085 made layouts leave with their material, so a reload rebuilds them). The window holding the crossing frame
  carries the 8 `native_model_stream_initialized` rows of the cycle (the character streams I-0081 saw initialised there).
- Over all 20 native runs (worker, `d_summary.txt`): the band per walk window averages 2.9, 4.7 and 3.0 frames for windows
  ending at phase 3-4, 4-5 and 5-6 s, against 0.03-0.57 elsewhere; the top fifth of the windows hold 735 of the 1,013 band
  frames. Across the walk windows without a frame over 34 ms, the band correlates with the model storage admissions (0.88), the
  layout builds (0.88), the zone retirements (0.85), the game steps over 25 ms (0.87) and the ingress lists a frame (0.83), and
  not with the native phases (collect -0.20, prepare -0.01, record -0.12). Correlations, not causes: the band follows the reload,
  and the engine thread's own native phases do not grow in it. In the crossing frame itself the timed native phases sum to about
  10 ms of 36.5: most of that frame is outside them (DS3's step, its join, the pacer).
- No authored material or effect is admitted during the motion (`native_material_cpu_admitted` and
  `native_effect_cpu_admitted`: 0 in every motion window of i94-final-m1), so the global programme generation
  (`program_registry_generation`, bumped only by those) does not move: the crossing's programme lookups are for new layouts,
  not a global invalidation (a hypothesis checked and rejected here).
- **The held count is biased low where stutter happens** (`native/renderer/scene_pass.cpp:866-872`): a sample is kept only when
  DXGI `PresentCount` advanced by exactly one since the previous Present call. In the reload windows only 66-90 of 120 frames are
  sampled (i94-cal-m2), and over the motion about 7,100-7,400 of about 7,500 presented frames (i94-cal-m1..m4, i94-final-m1/m2).
  Meanwhile the intervals between Present calls on the render worker (`native_present_times`) have **107-123 frames over 25 ms**
  per run, more than the engine side's 85-104 (same runs). The Present call returns in 0.17-0.31 ms on average and the GPU wait
  is 0.06 ms (`native_present_pacing`, walk windows of i94-final-m1): Present does not block, so the queue holds no frame ahead
  and a Present one refresh late most likely shows as a repeated refresh (INFERRED from those two numbers). H3 is therefore **not supported**: the
  held count (13-17 per run) cannot be read as « most slow frames never reach the screen »; it may miss most of them. Over
  the 16 runs since I-0090, 58-62 % of the windows with a frame over 25 ms show no held frame (18 % at i83), and the per-window
  correlation of held with frames over 34 ms is 0.38 (study worker, `d_summary.txt`).
- Consequence for D-0207: the i94-cal recalibration lowered the held threshold from 62 to 22 on a count with this bias. The
  drop from i83 (53 in i83-cal-m1) to i94 (13-17) happened at a similar sampled fraction (7,279 against 7,084-7,397), so it is
  probably real in the sampled frames, but neither number is the display's truth.

Profile of the slow frames (i81-07, I-0081, 13 frames over 34 ms, 203 engine-thread samples; the newest slow-frame profile,
taken before I-0088 to I-0094): the native collect 28 % (model admission 16 %, `AcquireNativeModelPrograms` 8 %, the wait for
the admission pool's deferred candidates 8 %), DS3's critical section at 0x14179945F entered from its task join 11 % (contended
with the loading threads, INFERRED), the pacer's QPC reads 14 %, heap allocation 7 %.

## 5. The traversal stutter

In short (Thomas counts the traversal stutter in the stutter, 2026-10-23): on this path the traversal stutter is the whole story. Every class above is met while moving into, or back out of, an area: the crossing frame (A), the reload windows around it (§4), the first steps (D). No pipeline compile and no GPU limit is involved; the cost is CPU work on the engine thread (the native admission of entering models joined by the engine thread, DS3's own crossing step) and CPU contention from the reload (DS3 and the native rebuild of the same part every cycle).

### 5.1 What the native backend does while moving (from the code and earlier results)

- Textures and static buffers: created on DS3's loading threads as placed resources in pooled heaps (I-0079, after ER's placed
  arm), staged through pooled upload chunks, recorded on the queue's ingress context and executed on the **direct** queue
  ahead of the frame (about 8.7 ingress lists a frame at the title, `native_present_pacing.ingress_lists_per_frame`); no copy
  queue (no `D3D12_COMMAND_LIST_TYPE_COPY` in `native/`). GPU wait in the walk windows 0.06 ms: the GPU is not the limit on
  this path at 60 Hz (OBSERVED, i94-final-m1 pacing rows).
- Static buffers created outside the storage registry's lock, layouts behind a shared lock and an index (I-0081).
- Pipelines: no pipeline state created in any slow frame of the path (`pso_created` 0 in every class-A/B row read here; I-0079
  H1 refuted); the cache (I-0072) precompiles at boot. A pipeline met for the first time on another PC still compiles in its
  first frame (STATE obligation 6): not visible on this path, a real risk elsewhere.
- Models entering the view: their first admission (layout, programme choice `AcquireNativeModelPrograms`, texture plan) runs on the admission pool inside the collect, which the engine thread joins;
  a warm on the loading thread was tried and removed in I-0081 (worse counts, reason not isolated).
- A streamed resource not ready delays its model's appearance (D-0197), never a stand-in.

### 5.2 Elden Ring (documents only: the earlier ER studies, not re-read in ER's code)

Coverage gap first: neither the DS3 project's ER studies nor the read-only ER project (I-0001 to I-0008: submission, lifetime,
renderer boundary) studied ER's map blocks, FLVER model loading, collisions or the trigger of an area crossing. What is known
covers textures and the generic job and file system:

- Loading threads: `CSFileImp`'s pools `LoadProcess_S` (one thread) and `LoadProcess_M` (three to five), at idle priority;
  overlapped `ReadFile`; DCX decoded in steps of at most 0x80000 bytes (I-0073 ER-STUDY §1).
- The engine thread never waits for a texture: the TPF processor posts one task and polls its event with a zero timeout
  (14023B5A0); each texture is decoded, created and uploaded on a worker and published whole (I-0073 ER-STUDY). A texture not
  yet there draws its fallback holder (`SYSTEX_DUMMY`, or the `_l` package: INFERRED), then the full texture swaps in; ER has no
  mip streaming (I-0071 ER-STUDY).
- Creation: placed up to 8 MiB in pooled 32-256 MiB heaps, first fit, no budget; committed above (I-0079 ER-STUDY). Upload: a
  committed UPLOAD buffer, a new COPY list and fence per creation, the creating worker waiting INFINITE (I-0073 ER-STUDY).
- Releases: frame-delayed buckets with a per-call work budget (ER-DEEPENING-897).

ER's stutter sources not to copy (D-0195): a pipeline miss compiled on the recording thread under the cache mutex
(I-0072 ER-PIPELINE-STUDY); one COPY submission, fence and CPU wait per created resource; committed creation over 8 MiB; heap
pools without a budget; D3D11-style barrier round trips and per-descriptor copies; busy waits (I-0080 ER-STUDY); whole-size
texture arrivals. PUBLIC, not verified: Digital Foundry measured spikes up to about 250 ms on ER 1.02 when new areas or enemies
appear (quoted in I-0080 ER-STUDY).

What the native backend already took or improved: the placed arm with best fit and smaller heaps, pooled 16 MB staging with no
CPU wait (better than ER), creations off the engine thread's locks, one pipeline store precompiled at boot (0 pipelines
compiled on a warm walk, 156 on a cold PC, I-0084 PITFALLS-AUDIT), the video memory budget read (beyond ER).

What ER does that the native backend does not, each INFERRED, none measured:

1. **The engine thread does not join texture creation.** DS3's engine still joins its TPF fragments (140D4E100, the lock
   144793D28; I-0073 ER-STUDY §6, I-0079 RESULT « open »); ER polls. Would remove an engine wait proportional to a TPF's
   creation at the crossing; D-0197 already says how a late texture shows (the model appears a few frames late). Risk: DS3's
   structures are not thread-safe and the in-place read relies on the join (I-0074).
2. **Loading work throttled** (idle-priority loading threads, small decode steps). Could reduce the preemption of the engine
   and render threads at crossings and is the candidate for class B (§3). Risk: slower arrival; I-0081's warm got worse from
   contention, so only an A/B decides.
3. **Releases budgeted per frame.** The kept-model expiry and the spare heap's destruction still run on frame threads
   (I-0079 RESULT L7). Small, low risk.
4. **A copy queue** (I-0084 lead F1, deferred): only useful if GPU time at the crossing is the limit; the GPU wait is 0.06 ms on
   this path, so no gain expected here.

## 6. Agility SDK (PUBLIC unless said; nothing verified on this PC)

- **What it is**: Microsoft ships `D3D12Core.dll` beside the application, so the application gets a newer D3D12 runtime than
  Windows' own. Normal selection needs the executable to export `D3D12SDKVersion` and `D3D12SDKPath`; DarkSoulsIII.exe does
  not (ROADMAP, to check). A module loaded into another process selects it with `ID3D12SDKConfiguration1::CreateDeviceFactory`
  (documented for this case; whether it needs Windows' developer mode on a retail PC must be checked before relying on it).
- **GPU upload heaps** (`D3D12_HEAP_TYPE_GPU_UPLOAD`, Resizable BAR): the CPU writes straight into video memory, removing the
  staging copy. For the stutter on this path the expected gain is **none measurable**: the slow frames are CPU frames (GPU wait
  0.06 ms in the walk windows, OBSERVED), the staging copy is already off the engine thread and without CPU wait, and textures
  in such heaps would need a linear layout or `WriteToSubresource`. Its place is the per-frame constants (ROADMAP) and possibly a
  small CPU saving in the record, not the stutter.
- **Enhanced barriers**: cheaper barrier translation and finer synchronisation. The record is about 1.2 ms a frame in total and
  is not longer in the slow frames (OBSERVED rows of §3), so the expected stutter gain is **none**. Their value is for later
  passes (ray tracing, async compute) and code clarity.
- **Possibly no SDK needed**: both features may already be in the system runtime of this Windows 11 (build 26200). A two-line
  check (`CheckFeatureSupport` `D3D12_OPTIONS12.EnhancedBarriersSupported`, `D3D12_OPTIONS16.GPUUploadHeapSupported`) in a
  unit control would answer it without shipping anything. To run once a build is allowed.
- Verdict: **not a stutter lever**. Keep it on the ROADMAP for the RT and work-graph phase as Thomas placed it; the stutter
  plan below does not depend on it.

## 7. Ranked plan (by expected gain on what the player sees; every gain INFERRED until measured)

Baseline per gate run (132 s of motion, i94): frames over 25 ms about 93, over 34 ms about 13 (class A 10-13, B 0-2, C 0-1),
the worst 57-60 ms; the original 14 and 3, the worst 34.5 ms (i83-ref-original). Lows in §2.

| Rank | Item | What it removes | Expected effect | Confidence | Cost, risk |
|---|---|---|---|---|---|
| 0 | **Measure what reaches the screen** (prerequisite, §8): the held count made unbiased, per-frame intervals written for the motion, PresentMon on both renderers | nothing; it decides whether the 25-34 band is visible and gives true lows | no frame gained; prevents chasing a metric | high | small native diagnostic change; PresentMon needs elevation (Thomas) |
| 1 | **The reload windows** (the 25-34 band: about 80 of the 94 frames over 25 ms a run, three quarters of them in the windows at phase 2-6 s of each cycle; it sets the 1 % low): DS3 reloads the same part every cycle (about 630 materials, 630 layouts, 650 textures and 80-140 model storages a cycle), and the native backend does its creations and uploads inside DS3's loading tasks, some of which the engine thread joins (DS3's TPF fragments, 140D4E100, I-0074; I-0079 cause 1 saw the engine thread waiting in that join while a loading thread ran `AdmitEncodedTextureFamily`, before I-0079 made each creation cheaper; ER polls instead and publishes a texture only when it is complete). The band follows the reload (correlations 0.83-0.88) and not the native phases; the original, doing the same reload, has 15 game steps over 25 ms against the native's 63-87 | the engine thread's waits on loading tasks that now carry native work, and the contention of the native creations with the engine thread | if the native excess at the shared engine point is removed: game steps over 25 ms about 75 to 15-30, frames over 25 ms about 93 to 25-45, the 1 % low about 36 to 45-50 frames/s | low to medium: the correlation is strong, the mechanism is inferred | measure first (per window: the engine thread's time in DS3's join, the native time inside the joined tasks, `CreatePlacedResource` and heap calls); then, after ER, the native creation and upload moved out of the joined task (the task hands the decoded data over, a native worker creates and publishes it, a model whose texture is late appears a few frames late: D-0197); then lighter or lower-priority native loading work; a retention cache for parts unloaded seconds ago only as a last resort (it costs memory, against the memory workstream: Thomas's trade-off). Risk: DS3 structures are not thread-safe and the in-place TPF read relies on the join (I-0074) |
| 2 | **The crossing frame's native collect** (class A): the collect admits every candidate of the view on the admission pool and the engine thread joins the pool (`AdmitNativeDeferredCandidates`, `09_model_admission.inc:1111`, `pool.Run`); a model met for the first time (the 8 character streams and their parts entering at the crossing) pays its whole first admission there (layout, programme choice, texture plan). Proposed: a first admission started on the pool without the join, its model drawn from the first collect that finds it ready (a few frames late, as D-0197 allows for a texture), known models unchanged | the collect's +4.3 ms in the crossing frame (7.3 against a walk mean of 3.0 ms); probably also class D (collect 29-30 ms in the first steps after a standstill, cause not read) | crossing frame 37 to about 33 ms, the original's 34; frames over 34 ms about 13 to 3-6, i.e. the original's level, both sitting on the threshold; the display still one refresh late at each crossing, as in the original | medium (the cost is measured; that it can be spread without a visible late model is not) | medium; DS3 thread-safety untouched (the admission already runs on the pool); a character appearing a few frames late extends D-0197 from textures to models: Thomas's call |
| 3 | **Class B, the 56-60 ms frames** (0-2 a run, native only on the gate path) | the worst frames: three refreshes held | the worst frame 60 to about 38 ms; 1-2 fewer frames over 34 ms | low (cause unknown) | a slow-frame profile at 45 ms first; leads: the kept-draw expiry every 600 prepares (two of the four class-B frames read came right after its row in i94-final-m1), a wait in DS3's task join on a loading task |
| 4 | **The test path's period** equals the kept-draw expiry (`native_kept_draw_unused` 600 prepares = 10 s = one walk cycle) and the `%600` diagnostics | a possible artefact either way | none or some; tells whether the class-C prepare spikes (9-18 ms in one crossing in three, 30 ms once at the turn) are the expiry dropping rows that come back | medium that it matters for class C, low for A | an A/B with the expiry at 1,800 prepares (a constant, diagnostic build) or a path cycle of 13 s |
| 5 | **First pipeline on a new PC** (not on this path): 156 pipelines, 181 ms on a cold PC (I-0084) | first-visit hitches for players | none on Thomas's warm PC; large on a first run | high that it exists | precompile per zone, or compile off the frame and draw the model late (D-0197's rule) |
| 6 | Agility SDK: GPU upload heaps (ReBAR), enhanced barriers; a copy queue | GPU-side copy and barrier cost | **none expected on stutter**: the slow frames are CPU frames, GPU wait 0.06 ms | high | keep for RT and work graphs (ROADMAP) |

Not a lever on its own: a deeper presentation queue (D-0187 keeps two frames in flight): with the engine paced by DS3's 60
limiter the CPU does not run ahead, so a third frame would add latency without a frame to absorb the late one (INFERRED).

What « at the original's level » means honestly: after items 2 and 3 the native path would have the original's number of
frames over 34 ms and the same one-refresh hitch at each crossing; the band 25-34 ms (item 1) is the larger gap, and nobody
knows yet how much of it the player sees (item 0).

## 8. Measurement protocol (for the pilot; nothing run by this study)

1. **Instruments first** (one small diagnostic change, A/B-neutral):
   - an unbiased display count per window: missed refreshes = (SyncRefreshCount_end - SyncRefreshCount_start) -
     (PresentCount_end - PresentCount_start) from `GetFrameStatistics`, read once per window, plus the windows' presented count;
   - every frame's engine interval, present interval and game step written for the motion segments (a binary or CSV dump, about
     7,600 frames a run), so that the 1 % and 0.1 % lows and the band's catch-up pattern (a long game step followed by a short
     one: uneven motion even without a held refresh, since DS3 steps with the measured interval) are computed, not bounded;
   - PresentMon (NVIDIA FrameView's copy is on the PC, I-0079) on the original and the native for the same path: `MsBetweenDisplayChange`
     is the only measure the two renderers share at the screen. Needs an elevated session: Thomas's decision.
2. **Runs** (announced under D-0116, offline, hash-checked restore): the gate path, 10 measure runs per build (about 9 are needed to see 10 frames over 25 ms, §2), builds alternated
   in one series (A B A B ...), the first 2 s of each segment trimmed as now; the original 4 runs in the same series with its probe.
   About 24 runs of 5-6 minutes: a night series (D-0204's nights without play), not a merge gate.
3. **Statistics**: per build the mean and sd of each count over the 10 runs; a difference is called only when it exceeds 2 standard
   errors of the difference (sd * sqrt(2/10) = 0.45 sd); for the frames over 34 ms, which are nearly deterministic, the per-run values compared directly (not a Poisson test, §2).
   Lows from the per-frame dump: 1 % low = the 99th percentile frame time over all frames of the 10 runs, 0.1 % low the 99.9th, with
   the run-to-run range of each run's own value.
4. **Classification of every frame over 25 ms** (not only over 34): its phase in the 10 s cycle, its collect, prepare, record,
   GPU wait, and the journal rows between its neighbours (as §3 and §4 did by hand), and the same for the original's slow frames.
5. **Variants to separate causes**: the path with a 13 s cycle (or the kept-draw expiry at 1,800, item 4); a slow-frame profile
   at 45 ms (class B); a profile of the reload windows with the loading threads and the engine thread's time in DS3's join (item 1).
