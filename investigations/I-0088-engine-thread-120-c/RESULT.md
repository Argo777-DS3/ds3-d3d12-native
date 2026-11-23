# I-0088 result: the engine thread toward 120 frames/s, third tranche

## Conclusion

Complete (2026-10-22, K-0470): 120 frames/s is reached at m45 and on the Firelink walk, measured unlocked (the game's rate
raised, Present(0) on a chain that may tear): m45 106 to **125 frames/s** (frame p50 9.28 to 7.92 ms, p99 10.76 to 9.10), the
walk at **157 frames/s** (p50 6.17, p99 7.28). At the 60 cap the engine thread's native work at m45 falls from 5.58 to 3.87 ms.
Two changes of I-0086's plan (ER-STUDY §9), made after Elden Ring's architecture without its defects:

- **P1, the UI display off the engine thread.** ER runs its UI task beside the scene render and plays the Display closure on
  its render thread; DS3 queued its per-movie display on GX's command queue. Ours: the movies' selection under the manager's
  lock and DS3's `E52EE0` per movie, with the native HAL behind it, run on a worker of their own posted at the collect's entry
  and joined by the render step (and always by EndFrame): no DS3 step runs during the DrawStep row, so no movie is advanced or
  destroyed meanwhile; only the movies' own locks are taken (not ER's global Scaleform lock held across the draw). The UI
  stamp falls from 1.27-1.39 ms to 0. `no-ui-worker.flag` keeps the former path (A/B).
- **P2a, the effect record beside the walk.** The main view's effect record (`RecordNativeEffects`, 0.4-0.5 ms) on a side
  worker posted at the set-up and joined before the scene's publication (a per-group join, not ER's pool-wide one): the m45
  collect 4.46 to 3.97 ms unlocked, 4.11 to 3.79 capped. `no-collect-side.flag` records inline (A/B).
- **P0, the measurement**: `original-unlock.flag` in a native run raises the game's rate (timing object +0x354/+0x358) and
  presents at once on a chain created with `DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING` when the factory supports it; without the
  tearing chain a window on the 60 Hz desktop held flip discard near 120 frames/s (the render worker waited about 1 ms a frame on
  the frame-latency object). Measurement only: the stale-flag sweep of the runner removes it after a killed run.

Not done in this tranche (measured, recorded for the next): the collect's value-initialised vectors (the admitted models'
places and the pass rows' places, 4.9 % and 4.7 % of the engine thread unlocked, about 0.4 ms each, `memset` 5.3 %), the
render worker as the next limit at m45 (the post waits 4.5 % of the engine thread), DS3's limiter spin at the cap (P7, no frame
gain), the other collect parts beside the walk (the lights and the reflection share process-wide words: `UseRenderSize`, the
reflection flag, the light keys), P3 to P6 and P9.

## Measurements (measure mode, no debug layer, medians over the in-game windows, `Get-I88Summary.ps1`)

| Run | Configuration | frames/s | frame p50 / p99 ms | DS3 before the collect | collect | UI | end of frame | native work |
|---|---|---|---|---|---|---|---|---|
| i88-01 m45, 60 cap | P1, P2a off | 60 | 16.70 / 18.02 | 11.07 (with the limiter's spin) | 4.11 | 1.39 | 0.07 | 5.58 |
| i88-10 m45, 60 cap | P1 + P2a | 60 | 16.66 / 17.67 | 12.79 | 3.79 | 0 | 0.07 | 3.87 |
| i88-14 m45, unlocked, tearing | P1, P2a off | 106 | 9.28 / 10.76 | 3.67 | 4.41 | 1.26 | 0.05 | 5.73 |
| i88-12 m45, unlocked, tearing | P1 + P2a | 125 | 7.92 / 9.10 | 3.72 | 3.94 | 0 | 0.30 | 4.25 |
| i88-13 Firelink walk, unlocked, tearing | P1 + P2a | 157 | 6.17 / 7.28 | 3.19 | 2.46 | 0 | 0.73 | 3.20 |
| i88-03 / i88-08 / i88-07 m45 unlocked, no tearing | off / P1 / P1 + P2a | 103 / 118 / 119 | 9.53 / 8.34 / 8.35 | 3.68 / 3.74 / 3.76 | 4.36 / 4.46 / 3.97 | 1.27 / 0 / 0 | 0.36 / 0.28 / 0.66 | |
| i88-05 / i88-09 walk unlocked, no tearing | off / P1 + P2a | 113 / 120 | 8.68 / 8.73 | 3.26 / 3.28 | 3.97 / 3.56 | 1.14 / 0 | 0.49 / 1.48 | |

Observations:
1. P0 (H1 holds): unlocked, the engine thread bounds the frame (its stamps sum to the frame); DS3's own work before the
   collect is 3.7 ms at m45 and 3.2-3.3 ms on the walk (I-0078's 4.7 ms counted the capped frame).
2. The first P1 build refused at its first world frame (`native_cpu_shader_policy_thread`: the UI shader policy was bound to the
   thread that built it); the policy is now adopted by each frame's preparing thread under the preparation lock.
3. The effect record runs on the side worker (`native_ffx_record_thread` names it, i88-15) with the same work as inline
   (i88-16, 6,000 frames each: 196,963 against 205,224 draws as the particles vary, 0 dropped, 0 unresolved, default textures
   16,534 against 16,531, the recorded 0x2D and 0x31 passes 5,511/16,543 against 5,510/16,540).
4. Guards on the Firelink walk with P1 + P2a (i88-17, `record-revalidation.flag`, prepare shadow): the serial walk check 0
   mismatch in 64, the pass rows' parallel check 0 in 64, the engine bytes check 0 differences over 5,760 checks.
5. D3D12 debug layer: m45 with P1 + P2a (i88-11) 0 errors over 4,926 frames; the in-game menu and the Status screen (I-0078's
   menu check, i88-18) 0 errors over 2,694 frames, both drawn (`menu.jpg`, `status.jpg`).
6. Pictures: m45 with P1 + P2a (i88-10) equal to the build without them (i88-01): HUD, embers, lighting. No rendering code
   changed, so no side by side with the original was made beyond the gate's standstill picture.
7. Profile (i88-19, unlocked, P1 + P2a, 30,000 samples of the engine thread): the collect 48.8 %, the admission 23 %, the pass
   rows 10 %, the reflection 7.4 %, the two vector value-initialisations 4.9 % and 4.7 %, the post's wait on the render worker
   4.5 %; the UI and the effect record no longer appear on the engine thread (they are on the `ui` and `side` threads).

## Reviews

Step review of P0/P1 (Opus): M1 a post after a deferred Close (fixed: no post after a stop request), M2 the HAL's thread word
(it names the session's building thread, a pool thread, never the owner: unchanged branch), M3 the lock order (the engine's UI
locks added at 1b), lows applied. Step review of P2a and the policy fix (Opus): no High; M1 validate the effect path on another
thread (done, observation 3); lows noted. Final review (Opus, the whole diff): no High; M1 E07's settings shadow, written to the
process-wide `[[GXRS]+0x4DD0]` while the effect record executes, now during the walk: the module's own readers
(`NativeDeviceAbsent`, the 61950 and 1AC840 hooks) take the shadow as absent; an engine reader of that member on the pool during
the walk is not known (INFERRED safe: the walk calls the engine's culling and visitor functions; none faulted or differed over the
series and the guards); M2 the new flags in the runner's stale sweep (fixed); lows: documentation made exact, line endings
restored, `TakeNativeUiDisplay`'s return kept for clarity, the per-present full-screen query kept (measurement flag only).

## Evidence

- Runs `evidence/I-0066/artifacts/i88-01..19-*` (Run-I88Series.ps1), builds `evidence/I-0088/artifacts/i88b..d-build`; the
  profile report `i88-19-prof-up2/sample-profile-report.txt`; the gate `i88-final` (REPLAY-GATE.json).

## Uncertainty / unresolved questions

- One run per configuration, one PC (RTX, Ryzen 7 5800X3D), a 1280x720 window; Archdragon Peak not measured.
- The engine's own readers of `[[GXRS]+0x4DD0]` during the walk (final review M1) are inferred, not shown.

## Artifacts produced or modified

- `native/`: P0 (15, 20, scene_pass, backend), P1 (01, 02, 14, 17, 20, render_worker, game_ui_shaders, game_ui_session), P2a
  (12, effect_draw), the render worker's test.
- `investigations/I-0088-engine-thread-120-c/` (CONTRACT, RESULT, Run-I88Series.ps1, Get-I88Summary.ps1); the runner's flags
  (`Run-StartupTrace.ps1`); K-0470.

## Suggested next step

The bottleneck profile Thomas asked for after the merge ([BOTTLENECKS](BOTTLENECKS.md)): the render worker is the limit; the next workstream is its frame memory (item 1), then the collect's storage, the effects' update, the reflection view and DS3's steps, one at a time.
