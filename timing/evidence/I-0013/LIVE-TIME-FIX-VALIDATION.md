# Live producer time-law validation

## Outcome

Session `20260921T071100Z-pid10584` validates the general elapsed-time
substitution on the established route. Thomas reported normal behavior at 60
and confirmed that the familiar high-rate hopping disappeared at 240.

The target-60 correction-ON control retained 181/181 valid producer calls at
59.996096 calls/s. Every row received the exact native float32 delta
`0.0166666005`; all 181 returned the native result bit-for-bit and the prototype
performed zero state writes.

The target-240 correction-ON capture retained 712/712 valid calls at 237.083754
calls/s. Every row used the additive branch and every final state matches
exactly:

`state + (native_state - state) * delta / 0.0166666005`

All 712 corrections were applied, with zero invalid rows or guard failures.
The live step ratio varied continuously from `0.249997` to `0.378218` according
to the supplied delta (mean `0.253081`); there is no requested-FPS table or
240-specific multiplier. Final-state total variation during the capture was
`0.671732` of the native per-call result sequence.

## Cleanup

The stop command disabled correction before removing the sole hook, restored
the original cadence pair exactly, and logged `success=1`. DS3 was then closed
by Thomas. No persistent game file was installed or modified.

## Acceptance boundary

This closes I-0013 for the reproduced current-player slope/stair symptom and
its proven mode-1 additive producer path. The patch also contains the derived
multiplicative decay law, but that branch did not activate in the live route.
Other producer modes, actors, exhaustive terrain, online use, packaging and
distribution remain outside this bounded acceptance. High-level original
collision/terrain class names are not assigned.
