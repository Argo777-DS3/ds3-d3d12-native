# I-0009 - Visible 165 FPS locomotion prototype

Capability: C4

Canonical correction name: **LTF — Locomotion Timing Fix**. Every reference in
this contract to the accepted correction means LTF, not TRTF.

## Question

Can the already confirmed I-0007 time-normalized distance comparison be turned
into a disabled-by-default, explicitly activated and safely reversible prototype
that produces a reproducible, visible sprint/roll improvement during ordinary
play at 165 FPS without changing the accepted LTF law?

## Scope

Included:

- Reuse the exact I-0007 comparison point `005C73D8`, the previous broad-update
  delta, and the normalization `raw * ((1/60) / elapsed)`.
- Preserve the accepted opcode, player/owner, consecutive-index, position-pair,
  finite-delta and exact-build guards; fail closed on any mismatch.
- Package a 165-FPS-only prototype that is inert on disk and after injection
  until an explicit activation command.
- Provide explicit activation, status and stop controls; stopping must disable
  the correction, hooks and timing override and verify restoration.
- Demonstrate one reproducible uncorrected/corrected sprint and roll comparison
  in the same exact-build session, with visible operator assessment and bounded
  telemetry sufficient to prove which state was active.
- Preserve relevant non-regressions: cadence, no guard failure/overflow, clean
  shutdown, exact timing/debug restoration, and the already accepted animation
  timing boundary remaining untouched.

Excluded:

- Reopening I-0007 or I-0008, repeating their accepted acquisitions, changing
  the correction formula, action-specific gains or speculative new causal hooks.
- 240/unlimited FPS, distribution, installer work, online play, roll i-frames,
  broad C4 completion or claims outside the demonstrated sprint/roll path.
- Persistent game-file changes or automatic launch/injection.

## Inputs

- Canonical claims: K-0037 through K-0039 and K-0044 through K-0053, with the
  limits recorded in the I-0007 and I-0008 results.
- Exact build: Steam AppID 374320 / BuildID 10167187 / executable 1.15.2.0,
  SHA-256 `EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C`.
- Accepted implementation inputs: I-0007 `distance_normalization.hpp`, debug
  register restoration, causal comparison guards, timing override restoration,
  sprint V3 and unchanged-formula roll evidence.
- Tools: native synthetic tests, exact-build byte validation, bounded telemetry,
  and one announced offline live-game before/after session only after all offline
  gates pass.

## Hypotheses

- H1: the accepted comparison-only correction can remain active across ordinary
  sprint and roll play and visibly removes the 165-FPS half-distance sprint
  anomaly while preserving the accepted roll-motion behavior.
- H2: the accepted trial-scoped mechanism cannot safely remain active across a
  playable interval because its guard/lifetime/restoration contract fails;
  packaging must stop without a live demonstration.
- H3: the correction activates and restores safely but does not produce a clear
  operator-visible difference in the bounded before/after protocol.

## Expected evidence

The prototype build record must bind source and binary hashes to the exact game
build and accepted upstream source. Native tests must exercise inactive startup,
activation/deactivation, normalization, guard rejection, bounded telemetry and
restoration state. Live evidence must record exact requested/actual 165 cadence,
explicit OFF/ON transitions, correction/guard counters, operator-visible sprint
and roll observations, and a COMPLETE record proving hooks/debug state/timing
restoration. H1 requires both the visible comparison and technical state proof;
telemetry or operator report alone is insufficient.

## Exit conditions

- Success: deliver the inert-on-disk prototype, pass offline safety tests, and
  preserve one exact-build 165-FPS live session with a tangible OFF/ON sprint
  difference, normal corrected roll motion, no relevant regression, and exact
  stop/restoration proof.
- Refuted: a deterministic offline or live result shows the unchanged accepted
  correction cannot meet the playable lifetime or safety contract; disable it,
  preserve the discriminator and close without local patch stacking.
- Insufficient information: offline gates pass but a required human-visible
  maneuver or exact-build runtime state cannot be obtained; leave everything
  disabled and record the minimal intervention needed.
- Stagnation: after two implementation or validation cycles fail for the same
  unexplained reason, stop patching and open an RCA boundary rather than adding
  another local fix.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`;
  renew a bounded budget automatically only within this unchanged scope and the
  existing protected boundaries.

## Initial nine-cycle plan

1. Reconcile the clean-main checkpoint and bind only accepted I-0007/I-0008
   inputs; define this contract and exact non-goals.
2. Extract a minimal playable mechanism from the accepted comparison handler,
   removing acquisition-only lifetime and unbounded trace storage.
3. Implement explicit OFF/ON/status/stop control with OFF startup and fail-closed
   guard behavior.
4. Build and run native normalization, guard, control-state and restoration
   tests; record source/binary provenance.
5. Validate exact-build bytes/configuration and produce the reversible operator
   protocol; do not launch the game yet.
6. Announce and execute one offline exact-build 165-FPS session; verify READY in
   OFF state before any maneuver.
7. Capture an uncorrected sprint/roll witness, switch ON in place, and capture
   the corrected witnesses without changing formula or target.
8. Stop and prove timing/debug/hook restoration before interpretation; analyze
   counters, cadence and visible reports.
9. Close or record the minimal genuinely human intervention still required;
   update canonical state, manifest and result, validate, and checkpoint.

## Safety and human interaction

No live action occurs before offline gates pass. The eventual DS3 launch/test
will be announced in advance with a short protocol. It is offline and
operator-controlled: Thomas performs the sprint/roll inputs and reports the
visible comparison. The prototype never edits game files, starts disabled,
supports explicit stop, and must restore the captured timing/debug state. Any
guard failure disables correction and requests shutdown rather than guessing.

