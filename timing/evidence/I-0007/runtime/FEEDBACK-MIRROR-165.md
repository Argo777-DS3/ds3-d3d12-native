# Fixed-distance feedback: live causal qualification

## Observation

Session `20260919T181015Z-pid20840`, same process after the builder probe's
verified clean stop, captured 988 hits in 494 selected sprint intervals.
Every `005C73D8` comparison reconstructs exactly as float32 `30 * distance`;
every `005C7435` scalar result matches the reconstructed recurrence.
All 494 tested values were below 1 (range 0.469206–0.649815), all selected
the reduction branch, and all left source scalar `+25C` at exactly 0.5.

The getter equals broad pre-position N and the old vector equals broad
pre-position N-1 in all 494 cases. The governing elapsed time is therefore
delta N-1, not the current timing-module value (which differs on 8/494 rows).
Normalizing by `(1/60) / delta[N-1]` would avoid all 494 false reductions.
This last statement is a replay counterfactual, not an observed patch result.

Raw: [session](sessions/20260919T181015Z-pid20840/).
Checks and hashes: [summary](analysis/20260919T181015Z-pid20840/feedback-summary.json).
Reproducer: [analyzer](analyze_feedback_mirror.mjs).
Raw `scale-feedback-mirror.csv` SHA-256:
`6C12BDD5D77DA3D276A087A0F0CBE507063351455157B88B6E28773385F4C763`.
The probe stopped cleanly, restored debug state and exact timing `165/1 -> 30/0`.

## Causal interpretation

Combined with [builder proof](BUILDER-MIRROR-165.md) and its downstream
canonical links, the chain is now reconstructed:

1. At 165, each variable-time update naturally travels a shorter distance.
2. `005C72B0` compares that distance times a fixed 30 against 1, without time.
3. Its reduction recurrence holds builder `+25C` at 0.5 during sprint.
4. `005CA707` multiplies a reference-compatible source vector by that scalar;
   this is the first proved amplitude-creating operation.
5. Copy, rotation, inverse-delta conversion, candidate integration and final
   writer propagate the reduced amplitude; they do not create it.

Exact scalar producer, gates and constants are in
[source-scale exports](../static/source-scale/). Reduction uses `s*0.8` while
`s>0.5`, otherwise 0.5; recovery uses `s*1.2` while `s<1`, otherwise 1.
These are conditional updates, not strict clamps: one-step overshoot is possible.
No root-motion, collision or physics identity is assigned.

## Correction and remaining proof

Replace only the decision operand with `raw * ((1/60) / verified_elapsed)`.
This continuously expresses distance in reference-60 time units (equivalently
distance divided by twice elapsed time). It does not multiply locomotion output,
write the scalar or change its gains. The reference interval preserves the
original comparison at exactly 1/60; real jitter still requires a 60 validation.

The [reversible prototype protocol](CAUSAL-PATCH-PROTOCOL.md) specifies live
identity/time-pair checks and failure restoration. Cause and proposed formula
are established; sprint patch effectiveness, non-regression and roll remain
unvalidated. General cadence-invariance of the feedback transient is not claimed.
