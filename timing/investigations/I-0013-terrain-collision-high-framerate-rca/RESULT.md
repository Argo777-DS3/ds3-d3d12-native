# Investigation result

## Conclusion

Success, accepted by Thomas under D-0026. This result establishes **TRTF —
Terrain Response Timing Fix**. TRTF is technically and causally distinct from
**LTF — Locomotion Timing Fix**: it operates on a different recurrence, site,
guard set and proof chain. The initial valid 60/240 pair reproduced normal behavior at 60 and
visible hopping at 239.770 Hz. The sampled source/root remains monotone at both
rates, while opposing vertical cache motion per route distance is 5.466 times
larger at 240. The previously isolated vertical override gate is zero in every
row and is therefore refuted as the active cause of this reproduced symptom.
The cycle-16 pair clears base construction. The cycle-18 pair then proves all
901 final outputs bit-exactly equal source XYZ plus carried `+0x250` vertically,
while every comparable prior cache equals the preceding final output. The
amplified signal is therefore carried into the optional local transform by the
two contributions summed in `005C75F0`, not created by later cache transport.
Live provenance now resolves those calls to `005E6580` and `008BC9A0`.
Exact-build reconstruction shows that `005E6580` maintains a bounded per-call
state through either `state + 0.2 * response` or `retention * state`, with no
visible elapsed-time operand. The cycle-20 pair proves carried equals this state
in all 900 rows while the other contribution is zero; its active additive
opposing-distance density is 2.518x larger at 239.597 than 60.002 calls/s.
TRTF's general conversion is `step * dt/dt60` for the additive branch and
`retention^(dt/dt60)` for decay. The final correction-ON control performed zero
writes and preserved 181/181 native60 results bit-for-bit. At 237.084 sampled
calls/s, all 712 retained rows matched the additive formula exactly and Thomas
reported that the hopping disappeared with no observed anomaly. Shutdown
disabled correction, removed the sole hook and restored timing exactly. The
high-level original collision/terrain class identity remains unassigned.

## Evidence

- [`MATRIX-RESULT.md`](../../evidence/I-0013/MATRIX-RESULT.md)
- [`STAGE-BOUNDARY-RESULT.md`](../../evidence/I-0013/STAGE-BOUNDARY-RESULT.md)
- [`cache-writer-state/ASSESSMENT.md`](../../evidence/I-0013/static/cache-writer-state/ASSESSMENT.md)
- [`CACHE-RECURRENCE-DISCRIMINATOR-RESULT.md`](../../evidence/I-0013/CACHE-RECURRENCE-DISCRIMINATOR-RESULT.md)
- [`BASE-TRANSFORM-DISCRIMINATOR-RESULT.md`](../../evidence/I-0013/BASE-TRANSFORM-DISCRIMINATOR-RESULT.md)
- [`AGGREGATOR-OUTPUT-DISCRIMINATOR-RESULT.md`](../../evidence/I-0013/AGGREGATOR-OUTPUT-DISCRIMINATOR-RESULT.md)
- [`CARRIED-PRODUCER-PROVENANCE-RESULT.md`](../../evidence/I-0013/CARRIED-PRODUCER-PROVENANCE-RESULT.md)
- [`PRODUCER-STATE-DISCRIMINATOR-RESULT.md`](../../evidence/I-0013/PRODUCER-STATE-DISCRIMINATOR-RESULT.md)
- [`TIME-LAW-DERIVATION.md`](../../evidence/I-0013/TIME-LAW-DERIVATION.md)
- [`LIVE-TIME-FIX-VALIDATION.md`](../../evidence/I-0013/LIVE-TIME-FIX-VALIDATION.md)

## Uncertainty / unresolved questions

- Which high-level system owns that operation? Collision/terrain ownership is
  not yet established.
- The alternate decay branch, other producer modes, actors, exhaustive terrain,
  online use, packaging and distribution remain unvalidated.

## Artifacts produced or modified

- One-hook source/cache/root/vertical-state observer and deterministic
  analyzers.
- Read-only exact-build decompilation and field-use evidence.
- Native-60-preserving TRTF additive and decay time laws plus a reversible one-hook
  prototype that starts disabled.
- A route-normalized cache-cadence analyzer and the exact live binaries for the
  reproduced 60/240 session.
- An offline-qualified replacement one-hook observer at `009D97B0`, restricted
  to the exact current-player call from `005C8100`, plus its accepted live pair
  and exact cleanup.
- Read-only downstream reconstruction of the carried `+0x250` local-transform
  path and its predicates.
- An offline-qualified one-hook `005C8100` observer that records final output,
  all three known caller returns, prior cache and the carried/override predicate
  state without adding any protected operation or correction.
- Exact live 60-ON and 240-ON producer captures, their deterministic formula
  analyses, operator observations and cleanup log.

## Suggested next step

Integrate this coherent checkpoint into `main` only after explicit authorization.
Do not reopen I-0013 for product packaging or broader C4 work; route those as a
new scoped workstream when Thomas requests it.
