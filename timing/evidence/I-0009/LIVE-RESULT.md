# I-0009 live result

## Outcome

The bounded visible milestone passed on the exact supported build at target 165
FPS. In one continuous offline process, Thomas performed the same straight
sprint and single forward-roll witness first with correction `OFF`, then with
correction `ON`.

- `OFF`: Thomas reported that sprint made the character travel almost nowhere
  and that the roll distance was also much too short.
- `ON`: Thomas reported that travelled distance was similar to 60 FPS and
  considered the defect corrected for both sprint and roll.

This is a controlled same-process visible comparison, not a new instrumented
distance measurement. It confirms the requested playable result when combined
with the already accepted quantitative I-0007 sprint and roll validations.

Thomas also noticed that attack and jump appeared corrected. Those actions were
outside the announced stimulus and received no separate baseline or repeat, so
this is retained only as an unqualified observation.

## Runtime evidence

Session: `sessions/20260920T130445Z-pid6400`

- Exact executable SHA-256:
  `EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C`.
- `prepare` changed the central timing request from `30/0` to `165/1`; measured
  cadence stabilized near `164.85` FPS.
- The `OFF` checkpoint had 545 guarded comparison hits and zero corrections.
- From correction enable to the final `ON` checkpoint, 1,242 additional guarded
  comparisons occurred and all 1,242 received the unchanged I-0007 normalized
  operand. Of those, 227 changed a native reduction decision; zero normalized
  comparisons retained that reduction.
- All 51,287 central `+0x268` checks matched the broad update delta. The
  observation-only player animation sentinel changed 5,129 times, showing that
  the sampled animation state remained live; neither I-0008 path was patched.
- There were zero guard failures. At shutdown, arms, disarms, recurrence exits
  and commits were balanced at 51,285 each.
- Explicit `disable` preceded `stop`. The controller restored central timing
  from `165/1` to `30/0`, restored the saved debug registers, disabled the
  hooks, and wrote `COMPLETE`.

The deterministic counter derivation is retained in
`analysis/live-summary.json`; raw controller events and restoration records are
retained unchanged in the session directory.

## Preparation deviation

The first wrapper invocation stopped at a PowerShell interpolation parser error
before the controller or injector ran. The string interpolation was corrected,
then the exact-build-gated injection succeeded. This pre-injection failure did
not alter the game process. The wrapper-only fix is included in the final
checkpoint; the tested DLL and controller binaries are unchanged from the
offline checkpoint.

## Boundaries

- Qualified here: visible straight sprint distance and one forward-roll motion
  at target 165 FPS, on the exact build, with explicit OFF/ON comparison and
  exact runtime cleanup.
- Not qualified: attack, jump, roll invulnerability, damage behavior, other
  actions or roll variants, terrain/state coverage, whole-C4 correctness,
  240/unlimited FPS, persistence, packaging or distribution.
- The result remains a reversible prototype. Injection alone is inert and the
  prototype is stopped after this session.

