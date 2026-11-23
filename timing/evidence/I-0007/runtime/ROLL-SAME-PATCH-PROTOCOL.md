# Single-roll validation with the accepted sprint correction

## Additional acceptance gate: invulnerability (D-0012)

Thomas requires unchanged i-frames. This instrument does NOT capture the
actual invulnerability state or damage-acceptance/rejection decision. Its
distance/animation/commit checks qualify only motion; even a <=2% match cannot
close the complete roll gameplay gate. Establish the real engine representation
and consumer before comparing reference60 active windows, action-relative
boundaries, elapsed duration and any proved logical-frame count at165.
Do not equate rendered frames with logical invulnerability ticks or infer
unchanged protection from the animation ID/clock. No frame number is assumed.
Reuse the current motion capture; plan a separately announced minimal direct
invulnerability observation after qualifying its provenance, not blind combat
repetitions or a roll-specific movement patch.

## No roll-specific correction

Sprint is accepted within its bounded gate; see [validation](PATCH-SPRINT-VALIDATION.md).
The roll instrument reuses the exact same correction/observation handler,
same sites, same guards and the shared normalization header. Handler SHA-256
(LF-normalized): `65E5A0DEC6D9D3E2BC5C514DAFD0077938F243B20E0C75650CF46545FB98D7DA`.
Equality was checked against V3 source. No multiplier, scalar, threshold,
time pairing or branch is adapted for roll. Roll is not a separate RCA yet.

Only the acquisition automaton changes: F4 starts a bounded five-delta-second
window, independent of held/released keys, so the same correction remains active
through an impulse action and its settling. All input/animation/position rows,
comparison/recurrence/commit observations remain recorded. Separate DLL and
control event names permit injection after the stopped V3 without relaunch.

Build passes W4/WX; shared correction's native context/restoration tests already
pass. Roll analyzer self-test covers impulse count and displacement. Live roll
sentinel qualification and comparison remain pending, not implied by these tests.

- DLL `I0007RollPatch.dll`: `4AEBCC181C2920AA1E5A6FFE4EFD4AA4048C50487A44975E0689886C9E0C38F7`
- Injector: `4BBFF4C687BDDC138D9EC005F4AE496C212C62ED06ED38A4A03378D4CA034863`

## Acquisition and decision

Verify previous COMPLETE and exact timing/debug restoration. Start with
`Set-RollPatchTarget.ps1 -Target 60`; inject matching roll DLL into the existing
process, require READY/action=single_forward_roll, deadline900 and target readback.
Keep all old Evidence Stores untouched. Announce before operator action.

Thomas starts at rest on the same unobstructed route with full stamina. F4,
then ONE forward roll (Z plus a brief Space press/release), release all keys
once the roll starts; no continuing walk/sprint, camera or controller input.
Wait six seconds after F4 before reporting done. Record whether the whole roll
looked normal. Do not close DS3 between targets.

Flush with `I0007RollPatchInjector.exe flush <pid>` after trial_end. Run both
`analyze_causal_patch.mjs` and `analyze_roll_capture.mjs` into the new session's
I-0007 analysis folder. Inspect one Space impulse, neutral controller, initial/
final settling, animation segments, displacement and all interval/commit checks.
Do not assign a roll animation ID merely from an old free marker; the announced
single action, captured transition and operator report must qualify it.

If 60 capture qualifies, announce and switch live to165 with the same DLL/formula,
repeat ONE roll with the same stimulus. Compare full-window and action-specific
distance/animation; distinguish incidental walk from the roll. If needed, use
matched animation-phase interpolation with explicit boundary uncertainty, not
arbitrary sample exclusion. Require no half-amplitude loss, matched action and
distance within the existing 2% gate before claiming roll correction.
If capture cannot discriminate, report insufficient evidence; this is not
automatically a failed correction. If qualified roll remains incorrect, only
then open the separate roll RCA authorized by the mandate.

Stop using matching injector; verify COMPLETE/exact original30/0 and debug
restoration, reset disk config. Hash raw logs only after stop. The instrument is
not a persistent/distributable patch; its application lasts the announced window.
