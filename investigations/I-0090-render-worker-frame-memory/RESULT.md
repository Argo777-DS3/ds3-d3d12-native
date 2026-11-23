# I-0090 result: the render worker's frame memory

## Conclusion

Complete (2026-10-22, K-0471): the render worker is no longer the frame's limit. Unlocked at m45 its job falls from 8.07 to 6.55 ms a frame (-19 %; busy 97 to 83 %, its heap frees 10 to 2 %, its retirement 1.03 ms to 0), the frame 119 to 125 frames/s (p50 8.33 to 7.93, p99 9.66 to 9.11) and now bound by the engine thread (DS3's work 3.8 ms, the collect 4.1 ms); the Firelink walk 169 to 175 frames/s (p99 7.75 to 6.87). The target of 1.5 ms of the worker is met at m45 (1.52 ms); the frame's gain is bounded by the engine thread, as BOTTLENECKS predicted.

Two changes, made after Elden Ring's per-frame memory (I-0086 ER-STUDY §5: persistent material and pipeline state, the per-frame
draw packet rebuilt into a frame arena, no general heap call on the draw path) without its defects (per-entity draw data rebuilt
with no change test; the arena's spin lock):

- **Step 1, the draw's persistent state.** A main-view row rebuilt because its model changed (an animated pose, an engine
  animation block, a volatile model's new mesh set: 252 rows a frame at m45, 346 on the Firelink walk) passes its kept draw to
  `PrepareNativeModelDraw` as a base; the new draw takes the base's prepared `State` (pipeline, root signature, descriptor
  range with its per-slot table copies, sampler tables, the C4-2a encode record) when every state input is the same (programme
  set and sampler bank, geometry, fixed-function state, targets, SRVs, samplers, table copies, the constant bindings' registers,
  kinds and payload sizes); only its constant payloads are new. The last holder of a state returns its descriptor range.
  A frame never holds two draws of one state (a second encode of a state in one frame is refused loudly). Shadow:
  `prepare-shadow.flag` prepares every Nth adoption in full and compares. A/B: `no-state-reuse.flag`.
- **Step 2, the frame's holders off the render worker.** The frame ring's retirement (after the slot's fence) swaps the slot's
  holders (the frame input with its rebuilt draws, payloads and lists, the UI build, the effect frame) to a release thread owned
  by the ring, which destroys them; the render worker only swaps two vectors. Four batches waiting are released inline; WaitIdle
  and Close flush. A/B: `inline-retire.flag`.
- **A latent defect fixed**: `OwnedNativeModelDraw::Describe` returned a `GBufferDraw` whose table span pointed at a local copy
  of the tables (dangling once Describe returned); the unit test's first draw read a stale handle under the debug layer once the
  stack layout changed. The per-copy tables are now made once in the state.

## Measurements (series-1, build i90b, one process a run, measure mode without the debug layer, unlocked on the tearing chain)

Medians over the in-game windows (`Get-I90Summary.ps1`); base = both steps off (`no-state-reuse` and `inline-retire`), s1 = step 1,
s12 = both. The engine thread's stamps: DS3's own work before the collect (to update), the collect (update), the post's wait for the
render worker (end of frame). The worker's job and the slot release per frame from `native_frame_audit`. The last column is per frame from the run's last 600-prepare row of `native_persistent_draws`: the rows rebuilt because their model changed, and the rebuilt rows (for any reason: model, row or a refused frame block) that took their kept draw's state.

| Run | frames/s | frame p50 / p99 ms | present p50 / p99 | DS3 / collect / end of frame | worker job ms | slot release (worker) | release thread | rebuilt rows / state reused |
|---|---|---|---|---|---|---|---|---|
| i90-01 m45 base | 119.0 | 8.33 / 9.66 | 8.32 / 9.62 | 3.83 / 4.18 / 0.39 | 8.07 | 1.03 | 0 | 256 / 0 |
| i90-02 m45 s1 | 124.2 | 8.02 / 9.12 | 8.01 / 9.35 | 3.82 / 4.16 / 0.06 | 7.49 | 0.77 | 0 | 252 / 253 |
| i90-03 m45 s12 | 125.3 | 7.93 / 9.11 | 7.95 / 9.47 | 3.80 / 4.13 / 0.05 | 6.55 | 0.00 | 0.92 | 253 / 254 |
| i90-04 walk base | 168.9 | 5.67 / 7.75 | 5.70 / 7.69 | 2.99 / 2.59 / 0.46 | 5.09 | 0.52 | 0 | 363 / 0 |
| i90-05 walk s1 | 171.5 | 5.55 / 7.54 | 5.54 / 7.59 | 3.04 / 2.44 / 0.08 | 4.84 | 0.49 | 0 | 361 / 359 |
| i90-06 walk s12 | 175.1 | 5.62 / 6.87 | 5.66 / 7.25 | 2.98 / 2.50 / 0.04 | 4.35 | 0.00 | 0.66 | 353 / 350 |

Observations:
1. Step 1: the rebuilt rows take their kept draw's state (253 a frame at m45 against 252 rows rebuilt for a model change,
   350-359 against 353-361 on the walk: almost every rebuild only changes constant payloads). The prepare's draw phases (programmes, validation, pipeline,
   samplers, descriptor range, SRV copies: 1.90 ms of CPU a frame over the prepare's threads at m45) fall to 0; the main view's
   prepare 3.17 to 2.70 ms; the completion-retired descriptor ranges pending at their peak 30 to 2.
2. Step 2: the worker's slot release (its retirement) 1.03 to 0 ms at m45 (0.52 to 0 on the walk); the destruction, 0.92 and
   0.66 ms a frame, runs on the release thread.
3. Together the render worker's job falls 8.07 to 6.55 ms at m45 (-1.52 ms, -19 %) and 5.09 to 4.35 on the walk; the post's wait
   for the worker 0.39 to 0.05 ms. The sample profile (i90-07 against I-0088's i88-20, m45, 30,000 samples a thread): the
   worker busy 97 % to 83 %, `RtlFreeHeap` 10.0 % to 2.0 %, `RtlAllocateHeap` 9.2 % to 7.3 %, `FrameRing::Retire` 12.3 % to absent,
   `PrepareNativeModelDraw` 8.6 % to 2.2 % (the reflection view's rows, rebuilt each frame).
4. The frame is now bound by the engine thread, as BOTTLENECKS predicted: m45 125 frames/s (p50 7.93, p99 9.11 against the
   base's 8.33 and 9.66 measured the same evening), DS3's work 3.8 ms and the collect 4.1 ms; the walk 169 to 175 frames/s, its p99
   7.75 to 6.87 ms. The evening's base m45 (119) is below I-0088's 125 at the same code with the steps off: the PC measured slower
   this evening; the A/B within the series is the comparison.
5. Correction to BOTTLENECKS: its "offscreen views, the reflection 1.2 ms" was mostly the frame ring's retirement, run inside
   `RenderNativeOffscreen` when the reflection view opened the frame's list first (`FrameListSlot` -> `Begin` -> `Retire`); with the
   retirement gone the reflection's render is 1.9 % of the worker (about 0.15 ms) and its prepare 4.0 % (0.3 ms, its 36 rows rebuilt in full each frame). Item 4's expected gain on the worker is that, not 1.2 ms.
6. Pictures: m45 base against s12 mean difference 0.14 (0-255, 160x90 cells, no cell over 120), the walk 0.43-1.14 (the walk's
   timing); the same scene as I-0088's i88-12 (0.16).
7. Guards on the Firelink walk with both steps (i90-08, `prepare-shadow` and `record-revalidation`): the prepare shadow 0 mismatch
   and the model draw shadow 0 (1,503,751 adoptions in the run, every 30th, about 50,000, prepared in full and compared); serial walk check 0 in
   64, pass rows 0 in 64, engine bytes 0 differences over 5,760 checks; no fault in any run.

## Reviews

Step review of step 1 (Opus): no High; M1 (two draws of one state in one frame would now fault: unreachable, noted as the
contract above), lows applied (the shadow's global-heap predicate, a failing shadow prepare keeps the adoption, a deterministic
same-frame test, the unused counter removed). Step review of step 2 and the Describe fix (Opus): no High; M1 the release thread not flushed before the device closes on a faulted close (fixed: `SceneRenderer::Close` flushes first, the batches' tickets have passed); L1 the inline-release test proved nothing (it now counts the recording thread's destructions); L2 the cap allows five frames' holders with the one being destroyed (documented); L3 the WaitNext comment (made exact); L4 the release thread is not joined at process exit (the render worker has the same exposure; an orderly close joins it). Final review (Opus, the whole diff): no High, no Medium; lows applied in the documents (the shadow coverage counted over the whole run, the last column's meaning, the series header); two comment-only lows in `native/` left for the next workstream that touches those files (`backend.h` and `frame_ring_test.cpp` call deferred release "off by default": true of the class, the game turns it on unless `inline-retire.flag`), as is the absence of a mode row for the two flags (a run's configuration shows in `state_reused` and `release_thread_ms`): changing `native/` now would void the gate's record. Destructors on the release thread take only leaf locks (the descriptor, placed, staging and upload pools, the draw cache's lock, `returned_ranges_lock`, the primitive caches' lock), and no caller of `Flush` holds one.

## Evidence

- Runs `evidence/I-0066/artifacts/i90-01..10-*` (`Run-I90Series.ps1`; series logs `evidence/I-0090/artifacts/series-1.log`,
  `series-2.log`), builds `evidence/I-0090/artifacts/i90b-build` (the measurements) and `i90c-build` (the reviewed tree: the debug
  layer and menu runs i90-09 and i90-10, 0 validation error, no fault, the menu and Status screen drawn); the profile report
  `i90-07-prof-s12/sample-profile-report.txt` against `i88-20-prof-main`; the gate `i90-final` (REPLAY-GATE.json, log
  `evidence/I-0090/artifacts/gate-i90-final.log`): PASS, frames over 34 ms at the frame end 13, over 25 ms 99.5, held 17.5, game
  steps over 34 ms 1.5, pictures 0.52 and 0.53, memory leg -6 MB a visit.

## Uncertainty / unresolved questions

- One run per configuration in one evening series, one PC (RTX, Ryzen 7 5800X3D), a 1280x720 window; the evening's base was slower
  than I-0088's same configuration (119 against 125 frames/s at m45), so the A/B within the series is the comparison; Archdragon
  Peak not measured.
- The release thread's destruction (0.92 ms a frame at m45) is moved off the worker, not removed: the per-frame objects are still
  heap objects (the rebuilt rows' inputs and payloads, the frame input's lists, the volatile models' mesh sets). A true frame arena
  for them (ER's linear allocator) would remove it; it is not needed while the engine thread bounds the frame.
- The gate's memory leg (i90-final-z1) reported the pre-existing exit heap fault `0xC0000374` at its quit (exit code 0, no dump;
  I-0082 §5: raised on a worker thread while the process ends, also in the original); I-0088's gate did not report it. The ring's
  release thread is not joined at process exit (review L4, as the render worker); whether it now raises the same exit fault more
  often is not known: tonight's endurance (`exit-dump.flag` off) and the next gates watch it.
- The reflection view still rebuilds its 36 rows in full each frame (`PrepareNativeModelDraw` 2.2 % of the worker): kept draws for
  the offscreen views are item 4's.

## Artifacts produced or modified

- `native/renderer/model_draw.*` (State, Adopt, SameStateInputs, the Describe fix), `native/renderer/backend.*` (the ring's
  release thread), `native/renderer/scene_pass.h`, `native/frame/game_bootstrap/02, 13, 20`, the unit tests
  (`model_draw_test.cpp`, `frame_ring_test.cpp`), the runner's flags (`Run-StartupTrace.ps1`).
- `investigations/I-0090-render-worker-frame-memory/` (CONTRACT, RESULT, `Run-I90Series.ps1`, `Get-I90Summary.ps1`).

## Suggested next step

The frame is bound by the engine thread: BOTTLENECKS item 2, the collect's storage (the admitted models' and pass rows' places value-initialised and rebuilt every frame, the admission's per-frame heap: the collect 4.1 ms at m45, admission 24 % of the engine thread, `std::construct_at` 7.9 %), then re-rank. Item 4 (the reflection view) is smaller than BOTTLENECKS said (observation 5).
