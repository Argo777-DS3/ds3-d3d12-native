# I-0095 result: the stutter study (read-only)

## Conclusion

Complete (2026-10-23, study; branch `claude/nervous-boyd-c6406a`, not merged). The details, tables and the ranked plan are in
[STUDY.md](STUDY.md); the protocol for the stutter workstream's pilot is its §8.

1. **Noise** (H1 supported). Over the 16 gate measure runs since I-0090, frames over 25 ms 93.8 on average, sd 6.6; over 34 ms
   12.8, sd 2.0. The pair means 87.5, 94 and 99.5 are within noise (no build since I-0090 differs, F(6,9) = 0.64); about 9 runs per build
   are needed to see a change of 10 frames over 25 ms, 4 for 20, 11 for 3 frames over 34 ms. The frames over 17 ms are at the original's level since I-0090 (932-1010 against 945):
   the gap is the 25 ms band and above. The 0.1 % low is 27 frames/s natively (36.8 ms, exact) against about 30.6 in the
   original; the 1 % low about 36 against about 50 (estimates inside the bounds 29-40 and 40-59); the worst frame 57-60 ms against 34.5.
2. **The frames over 34 ms are one per cycle, at the area crossing** (H2 supported): 10-13 a run at the same phase of every walk
   and sprint cycle, 34.6-38.3 ms, with the native collect at 6.2-8.0 ms against its walk mean of 3.0. The original has the same
   crossing at the same phase at 34.0-34.5 ms, every other cycle over the threshold. So most of the « 13 against 3 » is the same
   one-refresh-late frame measured a few milliseconds longer natively. Native-only: 0-2 frames of 56-60 ms a run (cause
   unknown) and a rare 30 ms render-worker prepare at the camera turn.
3. **The frames over 25 ms gather in the reload windows**: the walk crosses the same boundary back and forth, DS3 unloads and
   reloads the same part each cycle, and the native backend rebuilds about 630 materials, 630 layouts and 650 textures a cycle.
   The band follows the reload (correlations 0.83-0.88 with the storage, layout, texture and ingress rows) and not the native
   phases; at the shared engine point the original has 15 game steps over 25 ms, the native 63-87: the native excess is in the
   reload, most likely the engine thread joining DS3 loading tasks that now carry the native creations (INFERRED).
4. **The held count is biased low** (H3 not supported): it samples a frame only when DXGI's PresentCount moved by exactly one,
   which misses 66-90 frames of 120 in the reload windows; the Present intervals have 107-123 frames over 25 ms a run. What the
   player sees of the 25-34 ms band is therefore not known; it must be measured before it is optimised.
5. **Traversal stutter and ER**: no pipeline is created in any slow frame of the path; the crossing's native cost is model
   admission and programme choice for the entering models. ER's useful ideas are publishing a resource only when complete and
   never joining it on the engine thread, idle-priority loading and budgeted releases; ER's streaming of map blocks and models was
   never studied (gap).
6. **Agility SDK**: no stutter gain expected (the slow frames are CPU frames, GPU wait 0.06 ms); keep it for ray tracing and work
   graphs. GPU upload heaps and enhanced barriers may already be in this Windows build's runtime (a feature check, not yet run).

## Ranked plan (STUDY §7)

0. Measure what reaches the screen (unbiased held count, per-frame dump, PresentMon on both renderers).
1. The reload windows (the 25-34 band, the 1 % low): measure the engine thread's join and the native work inside the joined
   tasks, then move the native creation out of them after ER (publish when complete, D-0197 for a late texture).
2. The crossing frame's native collect: a first admission not joined by the engine thread: frames over 34 ms toward the original's level.
3. The 56-60 ms frames (profile at 45 ms first).
4. The test path's 10 s period against the 600-prepare kept-draw expiry (an A/B).
5. First pipelines on a new PC (precompile per zone or late draw). 6. Agility SDK: none for stutter.

## Evidence

- Gate runs `evidence/I-0066/artifacts/i83-cal-m1..m4`, `i83-ref-original`, `i9[0-4]-final-*`, `i94-cal-m1..m4`; profile
  `i81-07-walk-prof-b`; code `native/renderer/scene_pass.cpp:866-872`, `native/frame/game_bootstrap/13_model_frame.inc:241,1644-1655`,
  `08_model_programmes.inc:229-262`, `18_resource_admission.inc`.

## Uncertainty / unresolved questions

- Every expected gain is inferred; nothing was built or run.
- The original's display holds are not measured; per-frame times exist for neither renderer (windows of 120 only).
- One path (Firelink), one PC (Ryzen 7 5800X3D, RTX, 60 Hz).

## Artifacts produced or modified

- `investigations/I-0095-stutter-study/` (CONTRACT, STUDY, RESULT, status).
- `evidence/I-0095/frames-analysis/`: the study worker's report and tables, derived from the gate runs (no capture, no live run); new evidence, to include in the next external backup.
