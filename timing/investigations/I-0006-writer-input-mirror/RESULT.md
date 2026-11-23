# Investigation result

## Conclusion

Success. In the unchanged target-60/target-165 mirror at pre-commit RVA
`009DC559`, the effective candidate offset is the first confirmed input that
explains the approximately `0.5` final-amplitude ratio.

The categorical path did not change: every qualified commit selected fallback
source `A8`, and inhibition was zero in both conditions. The scalar was exactly
`1.0` (`0x3F800000`) on every qualified commit. The candidate and reconstructed
final ratios were identical within each metric:

| Quantity | Target 60 | Target 165 | 165 / 60 |
| --- | ---: | ---: | ---: |
| candidate projected rate | `5.9046665583` | `2.9492278091` | `0.4994740651` |
| final projected rate | `5.9046665583` | `2.9492278091` | `0.4994740651` |
| candidate path-norm rate | `5.9856527846` | `3.0049877789` | `0.5020317561` |
| final path-norm rate | `5.9856527846` | `3.0049877789` | `0.5020317561` |
| scalar mean | `1.0` | `1.0` | `1.0` |

Therefore the local scalar and the confirmed packed-float writer calculation
do not introduce the loss: `A = U` in both sessions, while `U` itself is about
half as large per accumulated delta at target 165.

## Evidence

- Target 60: session `20260919T155913Z-pid17172`, 180 qualified mobile
  commits over `2.99998809` accumulated seconds.
- Target 165: session `20260919T160534Z-pid8520`, 494 qualified mobile
  commits over `2.99525811243` accumulated seconds.
- All 180/180 and 494/494 qualified rows reconstructed `XMM6` bit-exactly
  under their captured MXCSR.
- Every reconstructed commit matched the final transform at the next broad
  update's entry snapshot. This deterministic `N -> N+1` observation pairing
  is identical at both targets; update `N` supplies the governing delta.
- Both trials were complete, used valid input/position/animation chains, had no
  mirror overflow, and restored the central timing state exactly (`60/1 ->
  30/0`, `165/1 -> 30/0`).
- The comparison is preserved in
  `evidence/I-0006/runtime/analysis/20260919T155913Z-pid17172-vs-20260919T160534Z-pid8520/writer-mirror-comparison.json`.

## Uncertainty / unresolved questions

- The selected `A8` candidate remains semantically neutral. This result does
  not identify requested/root motion, collision, physics, or any other
  gameplay-system role.
- It does not locate the earlier producer of the candidate-amplitude loss.
- Roll remains unmeasured. No correction or 240-FPS work is authorized.

## Artifacts produced or modified

- Two raw runtime sessions and their derived analyses under `evidence/I-0006`.
- A bit-exact float32/MXCSR reconstructor and ordered cross-condition
  comparator with synthetic validation.
- K-0034 records the bounded confirmed conclusion.

## Suggested next step

Open a separately authorized, bounded workstream only if Thomas chooses to
trace the neutral candidate upstream. Start from the confirmed selected `A8`
candidate boundary; do not reopen the final writer, infer semantics, or begin a
correction from this result alone.
