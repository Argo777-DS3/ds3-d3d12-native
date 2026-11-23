# I-0011 bounded live protocol

Status: historical protocol, concluded after two cleanly restored sessions.
The remaining jitter and menu-required mouse path were accepted by Thomas for
this test-only overlay. Do not repeat this matrix without a new investigation.

## Preconditions

- Offline mode; exact Steam BuildID 10167187 / executable 1.15.2.0.
- Windowed mode on the validated high-refresh display.
- `BUILD-VALIDATION.json`, `DERIVATION-VALIDATION.json` and
  `EXACT-BUILD-VALIDATION.json` pass immediately before injection.
- No earlier I-0009/I-0010 DLL remains loaded; start from a clean DS3 process.
- Thomas controls every gameplay input and visible assessment.

## Stop conditions

Stop immediately for a failed/ambiguous state, signature or guard failure,
timing drift, missing overlay, input capture outside the overlay, crash, severe
instability, debug restoration error or timing restoration error.

## Repaired-build minimum repeat

1. Inject once. Confirm `STANDBY`, target `Native`, fix `OFF`, overlay hidden.
2. During ordinary gameplay, press HOME without first opening a DS3 menu.
   Confirm the pointer can operate the overlay directly. Close it with HOME and
   confirm focus/input returns to DS3, then reopen it.
3. Move/resize the game window and Alt-Tab away/back. Confirm the panel remains
   stable rather than visibly jittering, follows a real window move, hides on
   external focus loss and returns aligned on refocus.
4. Drag the slider once to 120 and release. Confirm the preview follows the
   pointer and the selected target changes only on release. The after-action
   log must contain one numeric transition for this drag.

## Remaining functional matrix

5. With the fix ON, select 60, 120, 165 and 240 through presets/slider. At each
   setting compare selected target with measured actual FPS and briefly assess
   sprint plus one forward roll.
6. From 240 select Unlimited. Confirm the UI reports it unavailable, does not
   relabel the state, and leaves the 240 timing pair unchanged. Select 165, then
   attempt Unlimited again with the same no-write result. This is the bounded
   H2 discriminator; it is not an Unlimited success test.
7. At 165, switch the locomotion fix OFF at the safe update boundary and assess
   sprint plus one forward roll. Switch it ON again without restart/reinjection
   and repeat the same two actions.
8. Select Native, then one numeric preset again to exercise exact restoration
   and numeric re-entry in the same process.
9. Click `Restore & Stop` (or use the external `stop` control). Confirm COMPLETE
   and exact timing, debug-register and hook restoration.

Attack and jump may be noticed but are not part of the controlled result. Do
not assess roll i-frames/damage or claim broader C4 correctness.
