# Investigation result

## Conclusion

This result validates **LTF — Locomotion Timing Fix** at the measured high-rate
condition. LTF remains distinct from the later I-0013 TRTF.

Complete at cycle 9/9. The target-240 request produced an actual measured
approximately-238-FPS accepted session under D-0019. In one clean exact-build
process, Thomas reported sprint and forward-roll travel too short with
correction OFF, then correct and similar to 60 FPS with the exact unchanged
I-0009 correction ON.

From the OFF checkpoint to the ON checkpoint, all 1,636 additional guarded
comparisons were corrected and 1,542 native reduction decisions changed. No
normalized comparison retained the reduction. There were zero guard failures
and zero central-delta mismatches. Cleanup balanced 45,409 hook cycles and
restored correction, hooks, debug registers and timing `240/1 -> 30/0` exactly.

The result is qualified at the actual measured approximately-238 cadence, not
actual 240 FPS. Attack and jump observations remain unqualified. I-0009 remains
closed and unmodified.

## Evidence

- [I-0010 contract](CONTRACT.md).
- [Active-display preflight](../../evidence/I-0010/DISPLAY-PREFLIGHT.md).
- [Exact derivation audit](../../evidence/I-0010/prototype/DERIVATION-VALIDATION.json).
- [Build validation](../../evidence/I-0010/prototype/BUILD-VALIDATION.json).
- [Exact-build validation](../../evidence/I-0010/prototype/EXACT-BUILD-VALIDATION.json).
- [Native test output](../../evidence/I-0010/prototype/native-test-output.txt)
  and [ten-run restoration stress](../../evidence/I-0010/prototype/NATIVE-RESTORATION-STRESS.txt).
- [Bounded live protocol](../../evidence/I-0010/prototype/LIVE-PROTOCOL.md).
- [Live result](../../evidence/I-0010/LIVE-RESULT.md),
  [deterministic summary](../../evidence/I-0010/live-summary.json) and
  [accepted raw session](../../evidence/I-0010/sessions/20260920T134137Z-pid29232/).
- [First cadence-only precheck](../../evidence/I-0010/sessions/20260920T133430Z-pid28388/),
  retained with exact cleanup before D-0019.
- [Completed I-0009 result](../I-0009-visible-165-locomotion-prototype/RESULT.md).

## Uncertainty / unresolved questions

- The limiting component behind requested240 versus measured approximately238
  is not instrumented; Thomas attributes it to hardware performance.
- Actual 240 FPS remains unvalidated. Attack, jump, roll i-frames/damage, other
  actions/states/terrain, whole-C4, unlimited, persistence and distribution
  remain outside this result.

## Artifacts produced or modified

- I-0010 contract, status, evidence manifest and display preflight.
- Disabled-by-default target-240 DLL/controller, source, derivation validator,
  native tests, exact-build validator, guide and live protocol under
  `evidence/I-0010/prototype/`.
- Two stopped/restored raw live sessions: the cadence-only precheck and the
  accepted OFF/ON witness, plus final result and deterministic summary.

## Suggested next step

Keep the prototype stopped and inert. Integration into `main` remains a
separate authorization boundary; do not promote the incidental attack/jump
reports or call this actual 240-FPS validation.
