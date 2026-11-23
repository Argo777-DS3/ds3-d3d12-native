# I-0088 follow-up: what limits the frame at 125 frames/s (Thomas, 2026-10-22)

Thomas (08:36 and 08:54, relayed by the coordinator): after I-0088, a complete profile at m45 and on the Firelink walk,
unlocked on the tearing chain and without the debug layer, to find what limits the frame now (the game's logic, the render
worker, the GPU submission or the GPU itself), the bottlenecks ranked by expected gain, then one workstream per bottleneck,
one at a time.

Runs, `main` 0b97380d's native tree (the gate's build `evidence/I-0083/gates/build-a572e9f1d727`): `i88-20-prof-main` (m45,
the sampler after 1,200 world frames, 30,000 samples a thread), `i88-21-walkprof-main` (the Firelink walk three times, the
sampler from world frame 7,000, 75,000 samples a thread); with the stamps and pacing rows of `i88-12-m45-tp2` and
`i88-13-walk-tp2` (the same code before the final review's two small fixes). Reports: `sample-profile-report.txt` beside each
trace. Percentages are inclusive shares of a thread's samples; milliseconds are those shares of the measured frame (m45 7.92 ms,
walk 6.17 ms). The p50 / p99 per station are over the 120-frame windows' means (the frame's own p50 / p99 are in RESULT).

## Where the frame goes

| Station | m45 (p50 / p99) | Firelink walk (p50 / p99) | Reading |
|---|---|---|---|
| Frame (engine thread interval) | 7.92 / 9.10 ms, 125 frames/s | 6.17 / 7.28 ms, 157 frames/s | |
| DS3's own work before the collect (game logic, its steps) | 3.72 / 5.02 ms | 3.19 / 4.98 ms | DS3's timeline; ours only through the observers (P9) |
| Native collect (engine thread) | 3.95 / 5.04 ms | 2.48 / 4.54 ms | admission 22 %, pass rows 10 / 5.5 %, reflection 7 / 12.5 %, two value-initialised vectors 4.8 + 4.4 % |
| Effects update (our transcription, DS3's SFX step, engine thread) | 5.4 % (0.42 ms) | 14.9 % (0.92 ms) | one thread; the walk meets many more effects |
| End of frame (the post waiting for the render worker) | 0.30 / 0.69 ms | 0.73 / 1.00 ms | the engine thread waits for the worker |
| Render worker: prepare | 3.11 / 3.37 ms | 2.02 / 3.32 ms | `PrepareNativeModelFrame` 42-44 % of the worker |
| Render worker: record (models) | 2.38 / 2.66 ms | 1.28 / 2.04 ms | `OwnedNativeModelDraw::Encode` 19 / 12 % |
| Render worker: offscreen views (the water's reflection, INFERRED: the only view of the frame job's list in play) | 14.9 % (1.2 ms) | 19.6 % (1.2 ms) | `RenderNativeOffscreen` |
| Render worker: frame ring retirement | 12.3 % (1.0 ms) | 15.3 % (0.9 ms) | `FrameRing::Retire` in `FrameListSlot`: the previous frame's byte vectors and draw inputs destroyed |
| Render worker: heap (allocate + free, all callers) | 19 % (1.5 ms) | 20 % (1.2 ms) | `RtlAllocateHeap` 9.2-9.4 %, `RtlFreeHeap` 10.0-10.5 % |
| Render worker: the driver's recording and submission | 11.7 % (0.9 ms) | 9.7 % (0.6 ms) | `nvwgf2umx` |
| Render worker: idle | 3.2 % | 8.2 % | busy 97 % at m45, 92 % on the walk |
| Render worker: UI packet build, present | 0.39 + 0.19 ms | 0.38 + 0.18 ms | |
| GPU | never waited for | never waited for | the frame slot wait (frame N-2's fence) 0.00-0.01 ms a frame, `gpu_wait_ms` 0.07: the GPU is not the limit at 1280x720 |

Conclusions:
1. The render worker is the limit: busy 97 % at m45 and 92 % on the walk, and the engine thread waits for it at the post
   (0.3 ms m45, 0.7 ms walk). The engine thread is close behind (m45: 3.7 DS3 + 4.25 native, walk 3.2 + 3.2): lifting the
   worker alone moves the limit to the engine thread at about 130 frames/s at m45.
2. The GPU is not a limit at this resolution (its fence is always passed when the worker reuses a frame slot); to be measured
   again at 1440p and 4K (D-0170) before any GPU work.
3. A fifth of the worker's time is the general heap: the per-frame byte vectors and draw inputs made in prepare and destroyed a
   frame later in the ring's retirement. ER does not do this (I-0086 ER-STUDY §5: a linear frame arena reset per frame,
   per-thread bump chunks, no general heap call on the draw path).

## Bottlenecks ranked by expected gain (one workstream each, in this order)

| # | Workstream | Where | Expected gain | ER reference |
|---|---|---|---|---|
| 1 | **The render worker's frame memory**: the prepared frame's constant blocks, byte vectors and draw inputs from a frame arena reset when the frame slot comes back (no per-object heap allocation and no per-object destruction at retirement), the draw inputs kept by model where unchanged | worker | 1.5-2.5 ms of the worker (heap 1.5 ms, retirement 1.0 ms overlap); the frame then bound by the engine thread, m45 about 130 frames/s | ER-STUDY §5: GXLinearAllocator (72 MiB, reset per scene render), per-context chunked bump allocators; its spin lock not copied |
| 2 | **The collect's storage**: the admitted models' and pass rows' places neither value-initialised nor rebuilt every frame (recycled scene storage or places constructed by the workers), the per-frame heap of the admission | engine | 0.7-1.0 ms of the engine thread (vectors 0.73 ms, heap 0.4 ms) | ER-STUDY §5 and §8 (P3) |
| 3 | **The effects' update in parallel** (our transcription's per-effect and per-list work on the pool, as DS3's own `GXFfxUpdateJob` and ER's per-effect tickets, barriers on events, not ER's `Sleep(0)` poll) | engine | 0.3 ms m45, 0.6-0.7 ms on the walk | ER-STUDY §4 (P4) |
| 4 | **The reflection view** (its collect 0.56-0.8 ms on the engine thread and its render 1.2 ms on the worker): kept by content between frames when the camera and the water plane do not move, its walk beside the main one | both | up to 1 ms of the worker, 0.5 ms of the engine thread | none studied (ER's planar reflection to read first) |
| 5 | DS3's own steps under the native renderer (the observers on the update path, P9) and its limiter spin at the cap (P7) | engine | up to 0.5 ms; P7 no frame gain | ER-STUDY §6, §7 |

The GPU side (bindless binding, more queues, async compute) waits until a measurement at higher resolutions shows the GPU in the
path (ROADMAP: once the CPU is no longer the limit).

## Proposal

The next workstream is item 1, « Mémoire de trame du fil de rendu »: measure the worker's allocations per frame by caller
(their count and bytes, a diagnostic behind a flag), then move the prepared frame's per-frame objects onto a frame arena owned by
the frame slot (reset when the slot's fence has passed, which the ring already waits for), keep the stable draw inputs by model,
and measure the worker's busy share, the frame at m45 and on the walk, p50 and p99, with the same guards, pictures and gate as
I-0088. Then items 2 to 5, one workstream each, re-ranked by a new profile after each.
