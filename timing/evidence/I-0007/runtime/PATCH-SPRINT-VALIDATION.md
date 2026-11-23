# Causal prototype sprint validation — measured pass, late-window limitation

## Final sprint checkpoint

**PASS within the requested bounded sprint gate.** V3 session
`20260919T185120Z-pid3804` retained the unchanged correction until input release.
Thomas explicitly confirmed the normal movement persisted to the end of stamina.
The comparable measured interval has 495 updates, 494 exact decisions/commits,
no reductions, scalar1, valid input/position/animation and no missing update.
Relative to the patched60 control: distance/delta +0.639177%, distance/wall-time
+0.293584%, animation/delta -0.460101%, all within the existing tolerances.

601 post_measure rows cover a further 3.67245 delta-seconds through stamina
depletion and input release. All captured tail scalars remain1 and all 1,260
warmup/measure/tail windows' commits/arithmetic checks pass. Half-second sprint
tail rates remain approximately5.916–6.280 until the native animation changes
after depletion; the approximately3-unit bug does not return during sprint.
The V2 late-window objection is resolved by continued application, not a new
movement formula. Exact timing165/1 -> 30/0 and debug-state restoration pass.

Raw trace SHA-256:
`0265596F5E7CC33AA814AE0750E3D4B9F3EDF4A17BB2A373A99F0AE8582A31A6`.
[Final V3 analysis](analysis/20260919T185120Z-pid3804/causal-patch-summary.json),
[tail](analysis/20260919T185120Z-pid3804/patch-tail-trial-1.json),
[raw session](sessions/20260919T185120Z-pid3804/).
Next: [roll with identical correction](ROLL-SAME-PATCH-PROTOCOL.md).
This is not whole-game, universal terrain, hitch-transient or distribution proof.
The earlier chronological notes below remain evidence of the resolved limitation.

## Target 60 control

Session `20260919T184047Z-pid3804`, trial 1, shared-readable V2 DLL SHA-256
`6F15BAA80026379083A2D8461328973DAC629DE88F6142419EB8F3D3C41354BE`.
The session is now stopped: exact timing 165/1 -> 30/0 and debug restoration
passed. Raw hashes in the final causal-patch summary are immutable.

- 181 measured updates / 180 inter-update commits; median delta 0.0166666005.
- All 180 measured comparisons preserve the native decision (no reductions),
  and all measured resulting source scalars are exactly 1.
- Every measured commit is the expected writer, bit-exact with its register;
  previous/current/next position pairings and patched arithmetic all pass.
- No missing measured broad index, overflow, patch rejection or extra writer.
- Input, position and animation validity are 100%; sprint ID 300020210.
- Net distance per accumulated delta 5.881385061 versus accepted D-0005
  reference 5.894055679: ratio 0.997850272, error -0.214973%.
- Net distance per wall second ratio 0.997812842 (-0.218716%).
- Sampled path ratio 1.004590614 (+0.459061%).
- Animation progress per delta ratio 1.000000009, same ID.

The 60 control is accepted for progression to 165 under the existing bounded
D-0005 reference (its manually differing starts/facing remain a deviation;
the original strict reproducibility flag remains false). Thomas reported the
trial finished, without an accompanying anomaly report; this is not an explicit
visual non-regression attestation. Final acceptance still requires 165 result,
operator-visible assessment and exact cleanup. Roll remains untested.

## Reproduction and evidence status

Use [causal analyzer](analyze_causal_patch.mjs) plus unchanged I-0005 locomotion
and cadence analyzers. The small trial output and arithmetic checks are in
[session analysis](analysis/20260919T184047Z-pid3804/); reference comparison in
[comparison](analysis/patch-60-vs-accepted-60/locomotion-comparison.json).
These are interim files and will be recomputed after trial 2/stop. Raw session
files are still growing. Hashes emitted by the legacy cadence analyzer at this
stage are snapshots only, not final immutable capture identities.

## Next

After the 60 control passed, switch the existing process to 165 with the V2
control. Verify CONTROL/override readback and actual delta, then use the same
F4/Z+Space trial. Do not stop or reinject between cadences. Per-trial target
must be established from delta/override history, not initial session-info=60.
Compare trial 2 directly with trial 1 and the accepted original reference.

## Target 165 measured result and explicit operator objection

Trial 2: 489 measured broad updates, 488 qualified commits, median delta
0.00606059982. All positions, inputs and animation samples valid; same sprint ID.
Every decision and commit reconstructs exactly, no missing update or overflow.
Distance/delta 5.885030922 versus patched 60's 5.881385061: +0.061990%.
Distance/wall-time difference -0.285552%; path/delta +0.262497%; animation/delta
+0.758478% (within existing 1% animation tolerance).

One normalized reduction occurs at broad index 20793 following a 0.019402368
delta sequence: corrected test 0.917071, scalar 1 -> 0.8 -> 0.96 -> 1.152 -> 1.
This is the exact existing recurrence, including its native one-step overshoot;
it is disclosed rather than treating the scalar as constant. No new recurrence
gain or clamp is introduced. General hitch/transient invariance is not proved.

Thomas explicitly reported normal sprint for much of the action, then the bug
returning near the end of stamina. Therefore **do not accept the whole sprint
or advance to roll solely from the three-second mean**.

Reinspection of the already captured semantic stream shows that 0.5–1.5 seconds
after measure_end, path/delta falls to 2.969646 / 2.958875 while Z+Space remains
held, sprint ID unchanged and stamina still 34 down to 19. This precedes actual
stamina exhaustion. In V2, measure_end sets trial=0: the correction stops being
armed although target165 remains active. Thus the late observation occurs
outside the correction's application lifetime, not inside the accepted window.
It supports the lifetime explanation, not proof of full-action correction.
Reproducer: [tail analyzer](analyze_patch_tail.mjs), output
[trial-2 tail](analysis/20260919T184047Z-pid3804/patch-tail-trial-2.json).

## Required focused follow-up

V3 retains the identical correction/observation handler and time-normalization
formula, but keeps capture/correction active after measure_end while input is
held. `post_measure` records persist until `trial_end` on release. At 10 extra
seconds, the entire probe stops/restores instead of silently leaving target165
without correction. Native lifetime/shared-read/operand/debug tests pass.
Validate only this newly exposed tail at 165 before roll; do not repeat the RCA
or silently dismiss Thomas's observation. The standard first three seconds
remain comparable with the accepted 60 result. Full sprint/roll acceptance pending.
