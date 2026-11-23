# I-0005 Pass C — 120 FPS matched result

## Decision

**PASS for the ordered 120 FPS gate.** Requested cadence was reached, all
qualified chains remained valid, bounded idle/camera/collision sentinels passed,
the animation clock retained its delta-scaled contract, probe cost remained
small, and exact timing-state restoration succeeded. The next permitted
condition is 165 FPS.

## Session

Primary session: `20260919T114725Z-pid27100`

- Capture span: 181.886 s.
- Semantic samples: 21,821.
- Broad-update rate: 119.969425 Hz; median and p95 delta `0.00833330024` s.
- Present rate: 119.886160 Hz; 21,821 interval-0 and 16 transition/startup
  interval-1 calls.
- Position, animation, and controller-presence validity: 100%.
- Direct semantic-sampler cost: 13.1911 us mean and 290.4 us maximum, equal to
  0.15829% of a 120-FPS frame on average.
- Raw output rate: 318,853.33 bytes/s.
- Override transition: `30/0 -> 120/1`; exact restore: `120/1 -> 30/0`, both
  success `1`.

## Matched semantic result

- Idle ended at its exact starting position and facing with no active controller
  input. One axis alternated within a maximum 0.0010223-unit band; this is below
  the explicit 0.01-unit stability tolerance and does not accumulate.
- Camera-input retained exactly zero player displacement and facing change.
- The obstacle marker retained a zero last-second displacement plateau.
- The median animation-clock increment divided by broad delta was
  `10.00002971`; 1,997/2,030 forward pairs matched ratio 10 within 0.01.
- The representative animation retained the established `137033000` through
  `137034000` chain and the same duration values. Operator timing did not include
  the final `137034010` transition, so exact sequence equality is not claimed.

Human-repeated path length and segment duration remain diagnostic rather than
deterministic equality tests. No camera transform or physical scan-out is
measured, and these bounded sentinels do not prove universal gameplay
independence.

## Evidence

- Raw session: `sessions/20260919T114725Z-pid27100/`
- Derived metrics: `analysis/20260919T114725Z-pid27100/`
- Cross-condition tables: `comparison/condition-summary.csv` and
  `comparison/marker-comparison.csv`
- Machine-readable gate summary: `comparison/comparison-summary.json`
