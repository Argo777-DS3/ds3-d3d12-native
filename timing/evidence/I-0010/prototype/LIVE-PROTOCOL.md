# I-0010 bounded target-240 live before/after protocol

## Preconditions

- Announce the exact test and short input sequence before launching DS3.
- Use Steam offline mode and the same unobstructed, safe test area used for the
  accepted I-0007 locomotion witnesses.
- Confirm `BUILD-VALIDATION.json` and `EXACT-BUILD-VALIDATION.json` pass and the
  disk config remains `target_fps=240`, `start_enabled=0`.
- Confirm the primary display still reports at least 240-Hz mode headroom at
  2560x1440. If it does not, do not inject.
- Thomas controls sprint/roll input. Automation does not synthesize gameplay
  input and does not start the test unexpectedly.

## Sequence

1. Thomas launches the exact-build game offline, loads the prepared character,
   stands still on a clear straight route, and reports `prêt`.
2. Technical owner resolves the single game PID and runs `inject`. Require
   `state=STANDBY timing=0 correction=0`; injection alone changes nothing.
3. Run `prepare`. Require `state=OBSERVE timing=1 correction=0`, actual cadence
   stable between 229.0 and 245.0 FPS after at least five seconds, zero
   central-delta mismatch and zero guard failure. If this cadence gate fails,
   skip all gameplay witnesses, run `stop`, and classify the result as
   insufficient information. This revised floor was explicitly accepted by
   Thomas after the first clean precheck; always report that the request is 240
   and the actual measured cadence is approximately 229 or higher.
4. **OFF witness:** Thomas performs one straight sprint for about four seconds,
   releases, then one single forward roll after stamina recovery. He reports the
   visible sprint distance/speed anomaly and any roll anomaly. Run `status` and
   preserve the OFF counters.
5. Run `enable`. Require `state=CORRECT timing=1 correction=1`. Wait one second.
6. **ON witness:** on the same clear route, Thomas repeats one straight sprint
   for about four seconds, releases, then one single forward roll after stamina
   recovery. He reports the tangible difference and any new anomaly. Run
   `status`; require corrections, relevant native reductions corrected, balanced
   arms/disarms, zero guard/central mismatch and continuing animation samples.
7. Run `disable`; require `state=OBSERVE correction=0`. Then run `stop`.
8. Require `COMPLETE`, `debug_restored=1`, `timing_restored=1`, timing restored
   to its exact captured original value (expected 30/0), and no failed marker.
   Thomas may then close the game.

## Acceptance and stop rules

Accept only if technical state and Thomas's visible report agree: OFF reproduces
the high-cadence sprint amplitude defect, ON visibly removes it, corrected roll
motion remains normal, the I-0008 controls stay valid, and cleanup is exact.
Any guard failure, central-delta mismatch, crash, unexpected input, incorrect
state, missing comparison/correction activity or failed restoration ends the
trial. Do not reinject or improvise a second correction in the same process.
