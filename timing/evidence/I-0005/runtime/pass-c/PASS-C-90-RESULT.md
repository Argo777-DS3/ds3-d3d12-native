# I-0005 Pass C — 90 FPS matched result

## Decision

**PASS for the ordered 90 FPS gate.** The target cadence was reached, all
qualified semantic chains remained valid, the bounded sentinels showed no
reproducible divergence, sampler cost remained small, and the timing state was
restored exactly before hook shutdown. The next permitted condition is 120 FPS.

## Session

Primary session: `20260919T113730Z-pid23528`

- Capture span: 136.7791608 s.
- Semantic samples: 12,311.
- Broad-update rate: 89.999090 Hz; median and p95 delta `0.0111111002` s.
- Present rate: 89.940792 Hz; 12,311 interval-0 and 17 transition/startup
  interval-1 calls.
- Position, animation, and controller-presence validity: 100%.
- Direct semantic-sampler cost: 13.2600 us mean and 340.8 us maximum, equal to
  0.11934% of a 90-FPS frame on average.
- Raw output rate: 227,911.36 bytes/s.
- Override transition: `30/0 -> 90/1`; exact restore: `90/1 -> 30/0`, both
  success `1`.

## Matched semantic result

- Idle and camera-input markers retained exactly zero player displacement and
  zero player-facing change.
- The obstacle marker reached zero last-second displacement; the maximum
  last-second obstacle plateau across 60/90 remains 0.00561 unit.
- The representative animation produced exactly the same six animation IDs and
  five candidate duration values at 60 and 90; play speed remained `1`.
- The median animation-clock increment divided by broad delta was
  `10.00000657` at 90 versus `10.00000108` at 60. The ratio matched `10` within
  `0.01` for 1,089/1,112 forward pairs at 90 and 901/916 at 60.
- Sprint path speed was 4.393 units/s at 90 versus 4.685 at 60; roll path speed
  was 3.159 versus 3.215. These human-repeated path measurements are diagnostic,
  not deterministic equality claims, and do not cross a divergence gate.

No camera transform was measured. Physical scan-out was not measured. Absence
of divergence in the bounded sentinels does not prove universal gameplay
independence.

## Evidence

- Raw session: `sessions/20260919T113730Z-pid23528/`
- Derived session metrics: `analysis/20260919T113730Z-pid23528/`
- Cross-condition tables: `comparison/condition-summary.csv` and
  `comparison/marker-comparison.csv`
- Machine-readable gate summary: `comparison/comparison-summary.json`
