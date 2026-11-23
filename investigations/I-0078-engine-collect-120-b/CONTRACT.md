# I-0078 - V2: the engine thread's collect toward 120 frames/s, second tranche (D-0195)

Capability: C3, wave 2. Opened 2026-10-20 21:48 Paris from the I-0077 checkpoint (STATE « Next action », I-0077 RESULT
« open »), in a fresh thread.

## Question

Of the engine thread's native work per frame at m45 (6.65 ms: collect 4.81, UI prepare 1.35, end of frame 0.49; I-0077 run
i74-native-70), which parts still repeat for unchanged entities or wait in series on the engine thread, and how much of it can
be removed by keeping per-entity data as Elden Ring keeps its persistent scene data (ER-STUDY I-0071 A, I-0077 §3: the entity
Draw reads its own fields and emits into frame memory; materials, shader choices and holders persist below the draw and are
invalidated by events), so that the engine thread's frame comes closer to the 8.3 ms of 120 frames/s?

## Known inputs

- I-0077 RESULT and its profile i74-native-71 (the engine thread and every pool worker sampled): the admission 2.1 ms on the
  engine thread (its own share of the pool's work about 1.6 ms, the serial default construction of the models 0.38 ms), each
  reused model about 6 µs of CPU (guarded reads about a quarter, then the record check, the instance records, the record and
  carry lookups, the global pose and storage locks, allocations); the pass rows 1.1 ms (the serial construction of the rows
  0.34 ms); the engine thread waiting on the pool's tails about 0.5 ms; the UI prepare 1.35 ms (the engine's Scaleform display
  about 0.4 ms, our 64 submissions about 0.8 ms); the effects 0.55 + 0.5 ms.
- ER: I-0071 ER-STUDY A (no prepared draw kept per instance; persistent scene, material blocks, shader choice tables, holders),
  I-0077 ER-STUDY (layer jobs, entity Draw jobs by ticket, per-thread records joined and sorted).

## Hypotheses

- H1: the serial parts left on the engine thread (the default construction of the admitted models and of the pass rows, the
  pool's last chunks) can overlap the pool's work or shrink without changing the scene.
- H2: a reused model's admission repeats work whose inputs did not change (the same engine blocks read field by field many
  times, a lookup and a lock per store, counters derived per mesh); one read per engine block and one per-entity lookup cut
  its CPU cost substantially.
- H3: the pass rows of a model whose row inputs are unchanged need not be evaluated again (ER: the per-entity data persist,
  only the per-frame emission remains).

## Discriminating evidence

m45 (`m45split`) before and after in measure mode, no debug layer, the same save and pose; the admission sections
(`admission-marks.flag`); the sample profile (`m45prof`); the guards on the Firelink walk (`check`: record revalidation, the
pass-row and walk checks, 0 mismatch); a D3D12 validation run (`m45debug`); pictures at m45 against main.

## Scope and exit

In: the main collect's admission, its placement, the pass rows, the pool scheduling; the UI production and the effects'
update are measured and moved only if the profile and ER give a clean seam. Out: the game's own update, the render worker,
the GPU. Success: a measured shorter engine-thread frame at m45, 0 guard mismatches, 0 validation messages, the same pictures;
reviewed (D-0176) and merged. Refutation: no measurable gain beyond run-to-run noise. Budget: 12 cycles.

## Safety and human interaction

Live runs under D-0117 with the D-0116 conditions (`Run-I74Series.ps1`: offline, hash-checked restore, test saves only), each
series announced in the thread at least 5 minutes before (PC clock), no other heavy work during a measurement.
