# Investigation result

## Conclusion

This result validates **LTF — Locomotion Timing Fix**. LTF is technically and
causally distinct from the later I-0013 TRTF.

Complete at cycle 9/9. The disabled-by-default prototype passed its offline
gates and the announced exact-build live OFF/ON witness. In one continuous
target-165 process, Thomas reported the uncorrected sprint as almost stationary
and roll distance as much too short; after explicit correction enable, he
reported both travelled distances as similar to 60 FPS and considered the
defect corrected.

LTF remains the accepted I-0007 low-XMM0 comparison-only formula and
guards. During the ON witness, all 1,242 additional guarded comparisons were
corrected and 227 native reduction decisions changed, with zero guard failures.
I-0008 contributed an independent central `+0x268` delta check (51,287 checks,
zero mismatches) and an observation-only player animation
`modules+0x10 -> +0x24` sentinel. Shutdown restored `165/1 -> 30/0`, the saved
debug registers and all hooks. There is no animation patch, new causal hook,
per-action gain or 240/unlimited mode.

## Evidence

- [I-0007 result](../I-0007-locomotion-165-rca-fix/RESULT.md).
- [Accepted sprint validation](../../evidence/I-0007/runtime/PATCH-SPRINT-VALIDATION.md).
- [Accepted roll-motion validation](../../evidence/I-0007/runtime/ROLL-VALIDATION.md).
- [I-0008 result](../I-0008-player-animation-timing/RESULT.md).
- [Build validation](../../evidence/I-0009/prototype/BUILD-VALIDATION.json):
  release source/binary hashes, accepted upstream hashes, MinHook identity and
  native test result.
- [Exact-build validation](../../evidence/I-0009/prototype/EXACT-BUILD-VALIDATION.json):
  full executable hash and four exact native signatures pass.
- [Native output](../../evidence/I-0009/prototype/native-test-output.txt):
  state controls, guard/formula, live low-XMM0 correction, RF resume, commit
  watch and exact debug-register restoration pass. The same executable passed
  ten additional consecutive restoration runs at the cycle-5 checkpoint.
- [Prototype guide](../../evidence/I-0009/prototype/README.md) and
  [bounded live protocol](../../evidence/I-0009/prototype/LIVE-PROTOCOL.md).
- [Live result](../../evidence/I-0009/LIVE-RESULT.md),
  [deterministic summary](../../evidence/I-0009/analysis/live-summary.json) and
  [raw session](../../evidence/I-0009/sessions/20260920T130445Z-pid6400/).

## Uncertainty / unresolved questions

- The visible witness is operator-assessed rather than a new metric trajectory;
  the accepted I-0007 captures remain the quantitative support.
- Thomas observed that attack and jump also appeared corrected, but those
  actions were outside the announced protocol and remain unqualified.
- Roll invulnerability, damage behavior, other actions/states/terrain,
  whole-C4 correctness, 240/unlimited, persistence and distribution remain
  outside this result.

## Artifacts produced or modified

- Disabled-by-default DLL and controller in `evidence/I-0009/prototype/bin/`.
- Minimal source, native tests, build/exact-build validators, live controller,
  operator protocol and provenance records under `evidence/I-0009/prototype/`.
- Investigation contract, completed status, live evidence manifest and resume
  state.

## Suggested next step

Keep the prototype stopped and inert. Integration into `main` requires Thomas's
authorization; after integration, select the next bounded C4 target without
promoting the unqualified attack/jump observation.
