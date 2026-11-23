# I-0010 - Exact same LTF at 240 FPS

Capability: C4

Canonical correction name: **LTF — Locomotion Timing Fix**. I-0010 validates
the unchanged LTF at a higher measured cadence; it does not define TRTF.

## Question

On the exact supported build, does the already accepted I-0007 comparison-only
LTF, packaged with I-0009's guards and reversible controls
unchanged, remain safe and visibly correct sprint and forward-roll travel when
the actual game cadence reaches 240 FPS?

## Scope

Included:

- Start from completed I-0009 checkpoint `e81316e` without reopening or
  modifying I-0009, I-0008 or I-0007.
- Reuse the exact comparison point `005C73D8`, previous broad-update delta and
  normalization `raw * ((1/60) / elapsed)`.
- Preserve the accepted opcode, player/owner, consecutive-index, position-pair,
  finite-delta and exact-build guards without weakening or extending them.
- Preserve I-0009's `STANDBY`/`OBSERVE`/`CORRECT`/stop control contract,
  bounded counters, I-0008 central-delta and animation sentinels, and exact
  timing/debug/hook restoration.
- Change only the target request and I-0010-specific artifact/session identity
  needed for a disabled-by-default target-240 validation build.
- Establish actual target-240 cadence before accepting any gameplay witness,
  then perform one same-process OFF/ON straight-sprint and single-forward-roll
  comparison under Thomas's control.

Excluded:

- Any new correction formula, threshold, gain, action-specific behavior,
  guard relaxation, new causal hook or downstream compensation.
- Repeating accepted I-0007/I-0008/I-0009 acquisitions or altering their
  records and artifacts.
- Unlimited FPS, roll invulnerability/damage, attack, jump, other actions,
  terrain/state coverage, whole-C4 completion, persistence or distribution.
- Online play, automatic game launch/injection or persistent game-file changes.
- Reintroducing the retired Discussion Interface Agent or any replacement.

## Inputs

- Upstream checkpoint: completed I-0009 commit `e81316e` and K-0054.
- Causal correction: K-0037 through K-0039 and the accepted I-0007 sprint and
  roll-motion evidence.
- Independent sentinels: K-0052 and K-0053, as already bounded by I-0009.
- Exact build: Steam AppID 374320 / BuildID 10167187 / executable 1.15.2.0,
  SHA-256 `EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C`.
- Display preflight: Windows primary `DISPLAY1`, 2560x1440 at 280 Hz, with a
  240-Hz-or-higher mode available at the active resolution.
- Prototype input: I-0009 source, native tests, build/exact-build validators,
  controller behavior and restoration contract; its formula/guard core is an
  immutable comparison baseline for I-0010.

## Hypotheses

- H1: with actual cadence compatible with 240 FPS, the unchanged correction
  safely converts the known OFF locomotion defect into visibly normal sprint
  and forward-roll travel, with zero guard/sentinel failures and exact cleanup.
- H2: the active display has sufficient nominal headroom but DS3 does not
  achieve a stable target-240 cadence in the bounded environment, so gameplay
  acceptance is unavailable and the result is insufficient information.
- H3: at target 240 the unchanged correction or I-0009 lifetime/restoration
  contract fails a guard, sentinel, balance or cleanup gate; the test stops and
  the 240 extension is refuted for this prototype.
- H4: technical gates pass but the OFF/ON witness does not show a clear visible
  improvement; preserve the observation without inventing a different fix.

## Expected evidence

Offline evidence must bind the I-0010 source and binaries to `e81316e`, prove
that the normalization/guard core is byte-identical to I-0009, identify every
intentional target/identity change, pass the existing native safety and
restoration tests, and pass exact-build signature validation. The display
preflight must record the active primary mode and availability of at least
240 Hz at its current resolution.

Live evidence, if authorized after those gates pass, must record requested and
actual cadence, explicit OFF/ON transitions, guarded comparisons, corrections,
native decisions, central-delta/animation sentinel activity, balance counters,
guard failures and exact stop/restoration. H1 requires both actual target-240
cadence and Thomas's same-route visible OFF/ON assessment; neither telemetry
nor operator report is sufficient alone.

## Exit conditions

- Success: one announced exact-build target-240 session reaches compatible
  cadence, shows the expected OFF defect and visibly corrected ON sprint/roll
  motion with the unchanged correction, passes all guards/sentinels, and ends
  with exact timing/debug/hook restoration.
- Refuted: deterministic offline or live evidence shows the unchanged I-0009
  correction/control contract cannot remain safe at target 240; stop, restore,
  preserve the discriminator and do not stack a local fix.
- Insufficient information: target 240 cannot be reached, the human witness is
  unavailable, or required evidence cannot be obtained; leave all runtime
  state disabled/restored and record the minimal missing input.
- Stagnation: after two implementation or validation cycles fail for the same
  unexplained reason, stop patching and open an RCA boundary.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`;
  renew a bounded budget automatically only within this unchanged scope and
  existing protected boundaries.

## Initial seven-cycle plan

1. Recover `e81316e`, create the branch/contract, and prove primary-display
   240-Hz headroom without launching DS3.
2. Derive an I-0010 prototype from I-0009 with only target and artifact identity
   changes; prove the formula/guard core unchanged.
3. Build and run native state, guard, formula, sentinel and restoration tests;
   bind source/binary provenance.
4. Pass exact-build validation and audit the disabled-on-disk configuration.
5. Prepare the bounded target-240 operator protocol and exact stop rules.
6. Announce the protocol and wait for Thomas before any DS3 launch or injection;
   if authorized, execute the single OFF/ON witness and restore immediately.
7. Analyze the preserved session, update only evidence-justified claims/state,
   validate the repository and create a coherent checkpoint.

## Safety and human interaction

The display preflight and all offline work are non-invasive. No live action is
permitted until offline gates pass and the exact short protocol has been
announced to Thomas. Thomas launches the game offline and performs every sprint
and roll input. Injection alone must be inert, correction starts OFF, and any
guard/sentinel/cadence failure ends the test. Stop must restore captured timing,
debug registers and hooks; no game file is modified.

## Authorized cadence amendment

After the first cleanly restored live precheck requested 240 but measured
229.002670 FPS at the original 235-FPS lower gate, Thomas explicitly accepted
229 FPS as sufficiently high for this bounded validation. D-0019 therefore
revises only the empirical cadence gate to `229.0 <= measured_fps <= 245.0`
after at least five seconds. Requested and actual cadence must remain distinct
in every result; success under this amendment is a target-240-requested,
approximately-229-or-higher witness, not proof of actual 240 FPS.

Because the first process was stopped and its DLL is inert, the revised witness
requires one clean DS3 restart. The formula, guards, hooks, sentinels, safety
rules, stop conditions and all other scope boundaries remain unchanged. The
planning budget is extended cumulatively from 7 to 9 cycles for the restarted
live witness and closure.
