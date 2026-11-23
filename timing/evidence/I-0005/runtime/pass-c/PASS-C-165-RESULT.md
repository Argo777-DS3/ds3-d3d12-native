# I-0005 Pass C — 165 FPS matched result at 280 Hz

## Decision

**PASS for the 165 FPS cadence gate and the currently instrumented bounded
sentinels; BLOCKED for the 165 gameplay gate.** With Windows set to the active
2560x1440 280-Hz mode before launch, the unchanged central `165/1` intervention
reached the requested cadence. All qualified chains and bounded semantic gates
passed, probe cost remained small, and the exact original `30.0/0` timing state
was restored.

Immediately after the run, Thomas reported a reproducible locomotion anomaly at
real 165 FPS: animation speed appeared normal while character travel distance
was incorrect. That operator observation is `OBSERVED`, not yet quantified or
causally localized. It blocks 240. The next action is a deterministic locomotion
comparison at 60/90/120/165, not a higher target.

## Session

Primary session: `20260919T121245Z-pid6348`

- Preflight display mode: Windows 2560x1440 at 280 Hz (279 Hz through CIM).
- Capture span: 229.1748934 s.
- Semantic samples: 37,774.
- Broad-update rate: 164.821720 Hz; median and p95 delta
  `0.00606059982` s, approximately `1/165`.
- Present rate: 164.684749 Hz; 37,773 interval-0 and 19
  startup/transition interval-1 calls.
- Position, animation, and controller-presence validity: 100%.
- Direct semantic-sampler cost: 13.0525 us mean and 476.4 us maximum,
  equal to 0.21537% of a 165-FPS frame on average.
- Override transition: `30/0 -> 165/1`; exact restore: `165/1 -> 30/0`,
  both success `1`.

## Matched semantic result

- Idle retained the exact starting position and facing with no active
  controller input.
- Camera input retained exactly zero player displacement and facing change.
- The obstacle marker retained a 0.0019122-unit last-second displacement,
  below the 0.01-unit bounded plateau tolerance.
- The median forward animation-clock increment divided by broad delta was
  `10.000000792`; 3,430/3,572 forward pairs matched ratio 10 within 0.01.
- The representative animation included the established `137033000` through
  `137034010` chain and retained the same duration values observed at lower
  targets.

Human-repeated segment duration, path, displacement, and animation selection
remain diagnostic rather than deterministic equality tests. No camera transform
or physical scan-out is measured, and these bounded sentinels do not prove
universal gameplay independence.

The free/manual movement markers in this session cannot refute the reported
locomotion anomaly because their paths, durations, headings, and action mixes
were not controlled strongly enough for cross-framerate distance equality.

## Cadence RCA update

The earlier central `165/1` request was capped near 120 while Windows exposed a
119-Hz active mode. With no probe or intervention change beyond the preflight
display environment, the retry reached 165 under a 280-Hz active mode. This
strengthens the supported windowed display/driver/compositor ceiling diagnosis;
it does not isolate one external component as the sole owner.

## Evidence

- Raw session: `sessions/20260919T121245Z-pid6348/`
- Derived metrics: `analysis/20260919T121245Z-pid6348/`
- Cross-condition tables: `comparison/condition-summary.csv` and
  `comparison/marker-comparison.csv`
- Machine-readable gate summary: `comparison/comparison-summary.json`
- Earlier failed-cadence diagnosis: `PASS-C-165-RCA.md`
- Locomotion block and acquisition plan: `PASS-C-165-LOCOMOTION-RCA.md`
