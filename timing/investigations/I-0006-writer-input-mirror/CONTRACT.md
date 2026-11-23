# I-0006 - Final-writer input mirror at 60 and 165 FPS

Capability: C4

## Question

Among the confirmed final writer's inputs, which is the first value whose
target-165 behavior explains the approximately `0.5` loss of sprint amplitude?

## Scope

Included:

- One unchanged deterministic target-60/target-165 mirror at the confirmed
  pre-commit observation point, RVA `009DC559`.
- Only qualified mobile commits from the accepted sprint stimulus and equal
  accumulated-delta windows.
- The capture schema, validity gates, float32/MXCSR reconstruction, ratio
  factorization, and ordered discrimination rule defined by the canonical
  writer-algebra bridge.
- Offline instrumentation build, synthetic validation, bounded operator-run
  preparation, acquisition review, and comparison.

Excluded:

- Exploratory hook discovery, new engine-region searches, and repetition of
  the completed final-writer discovery.
- Semantic naming of the neutral candidate source or its object chain.
- Semantic Pass 2, roll, correction, permanent patching, and 240 FPS.

## Inputs

- Canonical claims: K-0032 and K-0033.
- Governing decision: D-0009.
- Capture and interpretation SSOT:
  `evidence/I-0005/runtime/pass-c/PASS-C-SPRINT-WRITER-ALGEBRA-BRIDGE.md`.
- Accepted sprint stimulus and temporal-delta capture from the existing Pass C
  instrument; exact build identity from I-0001.
- Tools: observation-only external DLL instrumentation, hardware execute
  breakpoint, offline bit-exact reconstruction and comparison analyzers.

## Hypotheses

- H1: source selection or inhibition frequency is the first categorical
  divergence.
- H2: the effective neutral candidate offset is already reduced at target 165
  and explains the final approximately `0.5` ratio.
- H3: the candidate offset remains equivalent while the consumed `+0x1C0`
  scalar explains the approximately `0.5` ratio.
- H4: both numeric inputs differ and their measured product explains the final
  ratio.
- H5: neither input explains the final ratio; alignment/capture validity or the
  local sequential float32 calculation diverges and must be resolved from the
  captured bits and MXCSR before any wider work.

## Expected evidence

Two exact-build sessions produced by the same instrument and operator protocol,
one at target 60 and one at target 165. Every accepted row must satisfy the
pointer, context/update, inhibition, no-overflow, bitwise reconstruction, and
post-update commit gates routed to the canonical bridge. The comparison must
report, for each target, candidate offset, scalar, and final output, then apply
the bridge's categorical and numeric decision order with the existing 2%
cross-condition tolerance on both fixed-axis projection and path-norm sums.
Raw sessions, analyzer outputs, hashes, build identity, instrument identity,
and exact restoration evidence must be preserved in the I-0006 Evidence Store.

## Exit conditions

- Success: one validity-clean mirror identifies the first categorical or
  numeric divergence, or proves that only the local calculation/alignment
  remains as the bounded cause.
- Refuted: the mirror proves that none of the confirmed inputs or the captured
  local calculation can produce the measured final ratio under valid pairing.
- Insufficient information: any validity gate fails, the target cadence or
  sprint stimulus is not accepted, or one point cannot be paired
  deterministically.
- Stagnation: no new discriminating evidence after two analysis/revision
  cycles; stop and replan without widening the hook surface.
- Budget exhausted: stop when `iterations_used == iterations_max`.

## Safety and human interaction

The instrument is read-only apart from the already validated reversible timing
override, writes no game-directory file, and must restore the exact original
timing and debug-register state. DS3 must not be launched automatically.
Immediately before either live launch, tell Thomas in one sentence that the run
will distinguish a candidate reduction from a scalar reduction or a divergence
introduced by the local calculation, state the corresponding conclusions, and
wait for his operator action/approval. Thomas performs the established F4 then
AZERTY `Z+Space` stimulus. Any restoration failure, unexpected inhibition,
invalid pointer chain, mismatch, overflow, or unmatched update stops the run.
