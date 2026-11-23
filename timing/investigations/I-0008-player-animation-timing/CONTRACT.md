# I-0008 - Player animation temporal path

Capability: C4

## Question

Which exact operation advances the qualified player-animation clock observed in
Pass C and I-0007, which timing value governs that operation, and does retained
60/90/120/165 evidence support the same progression without hidden fixed-cadence
substitution?

## Scope

Included:

- Reuse the exact-build Pass C and I-0007 animation clock, duration, identity,
  play-speed, broad-delta and update records before acquiring anything new.
- Revalidate the player module chain and separate observed field behavior from
  object or gameplay semantics.
- Locate the smallest exact-build writer/dataflow boundary for the qualified
  `modules+0x10 -> +0x24` clock and identify its input timing value when evidence
  permits.
- Compare retained forward increments and deterministic sprint/roll segments by
  target, animation identity, duration, play speed and broad delta.
- If retained evidence cannot bind writer and timing input, design and validate
  offline the smallest read-only observer needed to discriminate the remaining
  hypotheses.

Excluded:

- Reopening I-0007 locomotion correction or repeating accepted sprint/roll
  acquisitions.
- Roll invulnerability, camera, collision, effects, broad animation-graph
  reconstruction, animation naming, asset editing, 240/unlimited and a
  distributable patch.
- Treating human-repeated marker durations as deterministic action equality.

## Inputs

- Canonical claims: K-0001, K-0020, K-0038 and K-0039, with the limits recorded
  in the I-0005 and I-0007 results.
- Exact build: Steam AppID 374320 / BuildID 10167187 / executable 1.15.2.0,
  SHA-256 `EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C`.
- Artifacts: Pass C 60/90/120/165 semantic sessions, deterministic 60/165
  locomotion sessions, canonical read-only Ghidra project and existing exporters.
- Tools: deterministic offline analysis and read-only exact-build Ghidra export;
  reversible live observation only if a later bounded protocol is justified.

## Hypotheses

- H1: the qualified clock is advanced from the same variable broad delta at each
  target, with the observed factor near 10 arising from a stable local scale.
- H2: the field is a downstream proxy whose apparent delta relation hides a
  separate fixed-step or cadence-substitution boundary.
- H3: different animation states select distinct timing inputs or scales; the
  aggregate median conceals a target-dependent branch.

## Expected evidence

Retained-runtime analysis must preserve session identity, target, update index,
animation identity, current/max values, play speed, broad delta, transition and
reset classification. Static evidence must preserve exact instruction bytes,
function entry, field displacement, input registers/dataflow and hashes. H1
requires a writer/input formula consistent with retained cross-target rows; an
aggregate correlation alone is insufficient. H2 or H3 requires a reproducible
alternative input, fixed step, selector or state partition.

## Exit conditions

- Success: bind the qualified clock to an exact writer and timing formula, then
  show retained 60/90/120/165 rows satisfy its bounded invariance gates, or
  demonstrate and causally localize an independent cadence defect.
- Refuted: prove that the qualified field is not an animation-progress clock or
  cannot represent the relevant player-animation timing path; record the exact
  mismatch and choose a better qualified state.
- Insufficient information: retained captures cannot identify object/writer or
  distinguish timing inputs; preserve the gap and a minimal validated observer.
- Stagnation: no new discriminating evidence after two cycles; stop broad field
  scans and replan around a more selective identity or runtime boundary.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`;
  renew a bounded budget automatically only within unchanged scope and protected
  boundaries.

## Initial six-cycle plan

1. Reconcile retained animation fields, sessions and provenance; produce a
   cross-target row-level audit rather than relying on prior medians.
2. Partition increments by identity, duration, play speed and reset/transition;
   test broad-delta and fixed-step models.
3. Recover exact-build writer leads for `+0x24/+0x2C`, bounded by float access
   shape and the known player-module chain.
4. Export and reconstruct only discriminating writer candidates and their timing
   inputs.
5. Join static and retained runtime contracts, or validate a minimal read-only
   observer offline if a live identity remains necessary.
6. Record the bounded conclusion and next C4 gate without broadening scope.

## Safety and human interaction

Cycles 1-4 are offline and do not launch Dark Souls III. Any later live run must
be announced with one discriminating sentence, use disabled-by-default read-only
instrumentation, and prove exact hook/debug/timing restoration. No live run is
scheduled by this contract.

## Tranche 2: cycles7-9, unchanged scope

Cycles1-6 preserve the old audit, statically join the ring owner/writer, explain
refresh cadence and prepare a native-tested read-only observer. Retained samples
lack source identity, ring cursor and callback pairing; they cannot prove the
remaining formula. Renew cumulative budget to9 without resetting used cycles.

7. After Thomas signals the offline idle character is ready, execute the announced
   three-second identity/copy observation; restore and validate before interpretation.
8. Use the captured source identity to constrain its time-field writer/input,
   or record insufficient information if the observation does not discriminate.
9. Join static/runtime contracts and close with explicit correctness limits.

No new framerate target, locomotion comparison, patch or main merge is authorized
by this tranche. Exact [protocol](../../evidence/I-0008/OBSERVER-PROTOCOL.md).

## Tranche 3: cycles 10-12, same question and scope

The cycle-9 insufficient-information result closed its acquisition tranche, not
the broader authorized workstream: the next discriminator is narrow, causal and
already derived from its evidence. Extend the cumulative budget to 12 without
resetting prior cycles.

10. Build and offline-validate a disabled observer for generator `010768C0`,
    control entry/return `01284D70/01284EA8`, and ring writer `009B4F8D`.
11. After an announced operator-ready non-60 run, pair the live generator,
    control fields, actual delta, native output and player ring values in one
    invocation chain; prove restoration before interpretation.
12. Re-evaluate the retained cross-target rows against the bound formula, or
    close the exact residual gap if the new capture still cannot discriminate.

This tranche does not authorize a patch, locomotion reacquisition, 240/unlimited,
roll i-frames, distribution or merge into `main`. A live run remains contingent
on an explicit protocol announcement and Thomas's ready signal.
