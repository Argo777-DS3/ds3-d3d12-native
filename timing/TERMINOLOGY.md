# Canonical terminology

Updated: 2026-09-22

This file defines the canonical names of the two accepted temporal corrections.
Use these identifiers in results, checkpoints, contracts, implementation state,
user interfaces and future conversations. Historical raw evidence is not
rewritten; when it is cited, the applicable canonical name must be stated.

## LTF — Locomotion Timing Fix

`LTF` names the locomotion correction established by the I-0007 RCA and carried
unchanged through I-0009–I-0012. It normalizes the accepted locomotion-distance
comparison with the previous broad-update delta:

`normalized = raw * ((1/60) / previous_broad_delta)`

Its bounded causal and validation scope includes sprint and roll-motion. Its
guards, intervention site, evidence and unresolved limits remain those recorded
by I-0007 and its successor validation workstreams.

## TRTF — Terrain Response Timing Fix

`TRTF` names the distinct correction established by the I-0013 terrain-response
RCA. At the resolved producer, the active additive state update is converted to:

`state' = state + (native_state' - state) * dt / dt60`

The separately detected multiplicative branch has the continuous form:

`state' = state * retention^(dt/dt60)`

The accepted live result covers the additive branch for the exact current-player
route and build: native 60 is bit-exact/no-write and high-rate slope/stair
hopping disappears. The decay branch and the other I-0013 exclusions remain
unvalidated.

## Required separation

`LTF` and `TRTF` are technically and causally distinct. They have different
operations, intervention sites, guards, proof chains and validation boundaries.
Do not merge their internal identities, state, telemetry, configuration or
evidence, and do not refer to them as the "old", "new", "previous" or "current"
fix when the intended correction can be named.

A later user-facing control may activate both corrections together. Such an
orchestration layer must continue to expose and preserve `LTF` and `TRTF` as
separate internal corrections.
