# I-0070 - DS3 DX12 Vague 2: scene data persistence (model records kept across texture arrivals and camera turns)

Capability: C3 (wave 2, first workstream; Thomas 2026-10-19 05:55 Paris, D-0193, D-0194; ROADMAP C3 Wave 2).

## Question

Why does the native collect rebuild models that did not change (the Firelink walk bursts, Native2325: 20 to 80 fresh
admissions a collect, 400-900 texture lookups, frames of 50-70 ms while the world streams in), and can the per-model
collect record (C7-2) be kept across texture, material and programme arrivals with explicit, per-record invalidation, so
that a texture arriving updates what depends on it instead of re-admitting every model?

## Scope

Included:

- the model record (C7-2) and its global epochs (`NativeRecordEpochs`: models, programs, texture registry, texture ingress),
  the per-programme texture choices (K P4b/P4c), the records' sweeps (`FinishNativeModelRecords`);
- the four costs measured apart (the walk, the rebuild of the render data, the material and command preparation, the
  handling of what changed) at the three measurement scenes: m45 Ariandel, Archdragon Peak, the Firelink walk;
- whether one model is rebuilt once per arriving texture, whether the camera-turn hitches standing still share the cause;
- the distant enemies at Archdragon Peak, rechecked by walking after the change.

Excluded: bindless binding, pipeline states, more GPU queues, the worker pool (later wave-2 workstreams); the original's
D3D11 path; the per-entity walk's own cost at m45 beyond what the records remove (measured, not redesigned here).

## Inputs

- Claims and records: WAVE1-CLOSURE §2.4 (Native2325 profile, the burst `sample-profile-burst-report.txt` against `-calm-`),
  OPTIMISATION-REVIEW 8.5, PERSISTENT-RECORDS-DESIGN (C7-2), K-COLLECT-IMPLEMENTATION-SHEET 2.4 (texture choices).
- Code: `native/frame/game_bootstrap/08_model_programmes.inc` (records, choices), `09_model_admission.inc` (admission),
  `12_scene_collect.inc` (collect), `native/frame/game_texture_ingress.*`.
- Tools: `Run-ZoneWalk.ps1`, `Run-AlphaCast.ps1`, `Get-PacingMedians.ps1`, the kprofile counters (`native_collect_split`),
  `native_model_records` rows, the sample profiler.

## Hypotheses

- H1 (static reading, 2026-10-19): every texture admission, residency or retirement (`TextureIngress::Changed`), every
  texture registry mutation, every model storage, material or layout insert or erase (`BumpModelRegistry`) and every shader,
  effect or material admission (`BumpProgramRegistry`) moves a global epoch that every record compares: one arrival anywhere
  invalidates every visible model's record (RecordEpochs) and drops every unvisited record; the next collect re-admits all
  of them. Native2325's outcomes support it: `epochs` grows by 16,000 in the 600 collects of the burst, against 2,125 before.
- H2: the camera-turn hitches come from the same loss (records of models out of view dropped when an epoch moves, or after
  120 collects unseen), so a model coming back into view is admitted afresh.
- H0 (to refute): the bursts are the unavoidable first admission of new models.

## Expected evidence

Before/after on the same saves and key paths (measure mode, no debug layer): per-collect fresh admissions by outcome,
texture lookups, collect ms, frames/s, p99 and frames over 34 ms; the revalidation guard (fresh admission compared mesh by
mesh with the record) run often during the walk must show no mismatch; pictures unchanged.

## Exit conditions

- Success: the bursts' fresh admissions and frame-time spikes reduced at the Firelink walk, m45 collect not degraded
  (lower if the expiries go), no record mismatch, no visual regression, no lifetime or synchronisation fault; merged.
- Refuted: H1 and H2 false (the fresh admissions are new models): record it and redirect to the admission's own cost.
- Insufficient information: the counters cannot separate the outcomes: add the missing counter first.
- Stagnation: no new discriminating evidence after 3 cycles.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`.

## Safety and human interaction

Live runs under D-0116/D-0117 (announced in the thread with Paris start and end times, offline, hash-checked restore,
nothing installed in the game folder), zone saves only (Eta for Firelink and Archdragon Peak, Epsilon for m45), one game at a
time on the shared PC. No irreversible change.
