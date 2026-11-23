# I-0092 - Mise à jour des effets en parallèle

Capability: C3 (Wave 2, performance: the engine thread, bottleneck 3 of [I-0088 BOTTLENECKS](../I-0088-engine-thread-120-c/BOTTLENECKS.md))

## Question

After I-0091 the frame is bound by the engine thread (m45 unlocked 137 frames/s, 7.26 ms: DS3's work 3.7 ms, the collect
3.6 ms). The effects' update, our transcription of the scene control's slot 8 (`effect_system::Update`, in DS3's SFX step on the
engine thread), is 6.1 % of the engine thread at m45 (`i91-18-prof-s123`, about 0.44 ms; BOTTLENECKS: 0.92 ms on the Firelink
walk), all on one thread: the update list's particle simulation items (2.8 %), the library update through the manager's slot 3
(2.3 %), the node-manager refills (0.7 %). Which parts can run on the pool, as DS3 and Elden Ring run them, and how much of the
engine thread and of the frame does that give back, unlocked, standing and in motion?

## DS3 and ER (read for this contract, DS3 1.15.2 decompiled by Opus)

- DS3's single-threaded branch of slot 8 (`FUN_14024D000`, `DAT_144962376 == 0`, the branch the native backend takes) does not
  run its two work lists inline: each is a ranged GX job (`FUN_1400B6480`: the pass, start 0, the item count, a range of
  `min(count, 10)`), submitted to the display's job queue `+0x2C8` and run by the GX workers with the caller helping
  (`FUN_1400D1C40` wakes the workers, `FUN_1400D1D30` takes jobs until the queue is empty); the update list's body
  `FUN_140296FF0` calls each item's vt+8 over its range. The node manager's update `FUN_14025B870` hands its nodes to the same
  kind of ranged job (ranges of 10). INFERRED (review M1: the generic job's run slot vt+0x18, which may run a job on the caller when the queue takes it, was not read): DS3 runs them concurrently in ranges of 10; FFX-DRAW §2 saw `296FF0` run on a GX job thread, and the original's own path (the multi-threaded branch) gives each GX worker its own 0x2DB0 update context (`FUN_14024AA60`, one per worker of `[display+0x2D0]`), so its effects' updates run on several workers at once.
- The particle item (`FUN_141ADC100` -> `FUN_141ADD510`) reads only its own node (item - 0x88), its owner ([item - 8]), dt and the
  emitter, not the pass context it is given; its random generator (`FUN_141ACADB0` -> `FUN_141ACABC0` -> `FUN_141ACB000`) is a
  per-thread block found through a thread slot (allocated on a thread's first use), a xorshift (`FUN_140249630`).
- ER (I-0086 ER-STUDY §4): the update spread over the GX pool by ticket (per effect, then the lists), three barriers; task 0
  polls each barrier with `Sleep(0)` (a defect not copied: ours joins on the pool's condition variable, per group).

## Scope

Included, each measured before and after:

1. The node refills and the update list's particle items on the admission pool (free during DS3's SFX step: the collect uses it
   later in the DrawStep row), by tickets of DS3's own range (10), the engine thread helping, joined before the passes are
   destroyed; the other items counted as before. The per-item kind counts (a diagnostic, a map lookup an item each frame) only
   at the frame whose row is logged. A/B: `serial-effects.flag`.
2. The engine thread re-profiled; the library update (DS3's slot 3, inside the manager) and the next bottleneck named.

Excluded: DS3's multi-threaded branch (`_GXFfxEntityContextUpdateJob`, per-worker contexts of the GX display), the proceed list
(empty in the witnessed zones), the effects record (I-0088 already beside the walk), the render side.

## Hypotheses

- H1: the particle items and the node refills scale over the pool's eight threads: their 3.5 % of the engine thread at m45
  falls under 1 %, the frame gains about 0.2 ms at m45 and more on the walk.
- H2: the pictures (effects included) stay equal within the run-to-run difference; the particles' random draws come from each
  thread's own generator, as in the original.

## Expected evidence

The engine thread's stamps and the frame and present p50/p99 (`Get-I91Summary.ps1`), unlocked at m45 and on the Firelink walk,
measure mode without the debug layer; the sampler before and after (`effect_system::Update`'s share); `native_effect_update`'s
counts (nodes, items run) equal between the builds; the pictures (m45, the walk, a bonfire's flames); the debug layer; the gate.

## Exit conditions

Success: the effects' update measurably down on the engine thread, the frame faster at m45 and on the walk, no visual difference,
validation 0, the gate. Refuted: no measurable gain, not kept. Stagnation after 3 cycles; budget 6.

## Safety and human interaction

Live runs under D-0116/D-0117, announced 5 minutes before with the PC clock, offline, the Shebang zone saves (Alpha untouched);
builds hidden (D-0199). Thomas (2026-10-22 21:59): one thread at a time while the weekly limit is at 89 %, this thread continues.
