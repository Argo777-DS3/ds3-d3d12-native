# I-0010 live result

## Outcome

The bounded target-240-request validation passed at an actual measured cadence
of approximately 238 FPS under Thomas's D-0019 acceptance. In one clean
exact-build offline process, Thomas performed the same straight sprint and
single forward roll first with correction OFF, then with the exact unchanged
I-0009 correction ON.

- `OFF` at 237.774704 measured FPS: Thomas reported that sprint and forward-roll
  travel were too short.
- `ON` at 238.185471 measured FPS: Thomas reported that both travelled distances
  were correct and similar to 60 FPS, and explicitly validated the result.

The target request remained 240 throughout; this result is qualified at the
actual measured approximately-238 cadence and must not be described as actual
240 FPS. Thomas attributes the small request shortfall to hardware performance;
the limiting component was not instrumented.

Thomas also reported that attack travel was too short OFF and correct ON, and
that jump appeared correct ON. Attack and jump were outside the announced
stimulus and did not receive the same controlled baseline/repetition, so they
remain unqualified operator observations.

## Accepted live session

Session: `sessions/20260920T134137Z-pid29232`

- Exact executable SHA-256:
  `EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C`.
- Immediately before injection, Windows reported the active 2560x1440 display
  path at 279 Hz.
- Injection entered `STANDBY timing=0 correction=0`; `prepare` then changed only
  central timing `30/0 -> 240/1` with correction OFF.
- The pre-maneuver cadence checkpoint was 237.616806 FPS, above the revised
  229.0-FPS floor, with zero guard failures and central-delta mismatches.
- The OFF checkpoint contained 1,146 guarded comparisons and zero corrections.
- From correction enable to the ON checkpoint, 1,636 additional guarded
  comparisons occurred and all 1,636 received the unchanged normalized operand.
  Of those, 1,542 changed a native reduction decision; zero normalized
  comparisons retained the reduction.
- All 45,410 central `+0x268` checks matched the broad-update delta. The
  observation-only player animation sentinel changed 4,541 times; neither
  I-0008 path was patched.
- There were zero guard failures. At cleanup, arms, disarms, recurrence exits
  and commits were balanced at 45,409 each.
- Explicit `disable` preceded `stop`. `COMPLETE` records correction off, hooks
  disabled, debug registers restored and timing restored exactly `240/1 -> 30/0`.

## First cadence-only precheck

The earlier session `20260920T133430Z-pid28388` stopped before any maneuver under
the original 235-FPS floor. It measured 229.002670 FPS at that checkpoint and
231.680191 FPS cumulatively, never enabled correction, and restored all state
exactly. D-0019 subsequently accepted >=229 FPS and required the clean restart
used for the accepted session. This precheck is preserved as environment and
cleanup evidence, not a correction failure or locomotion result.

## Boundaries

- Qualified: visible straight sprint and single forward-roll travel at an
  actual approximately-238-FPS cadence, on the exact build, with explicit
  same-process OFF/ON comparison and exact cleanup.
- Not qualified: actual 240 FPS, attack, jump, roll invulnerability/damage,
  other actions or roll variants, terrain/state coverage, whole-C4 correctness,
  unlimited FPS, persistence, packaging or distribution.
- The result remains a reversible prototype validation. I-0009 is unchanged,
  and no game file was modified.
