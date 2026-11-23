# I-0011 first live session — partial result

Superseded for closure by [LIVE-RESULT.md](LIVE-RESULT.md). This record remains
the analysis of the first session and its demonstrated defects.

Session: `20260920T142829Z-pid27872`

## Observed result

The exact-build runtime loaded in `STANDBY`, and Thomas confirmed that the HOME
overlay appeared. After the requested target-60 functional step, Thomas
reported that everything worked, with two usability defects: the overlay moved
slightly and DS3 had to be placed on a menu before its controls could be used.
Thomas then used `Restore & Stop` and closed DS3.

This is a positive bounded functional observation, not completion of the live
matrix. There is no target-by-target operator report for 120, 165 and 240, and
no controlled claim for attack, jump, roll invulnerability or broader C4.

## Runtime evidence

- The session ended with `complete`; timing, correction, hooks and debug
  registers were reported restored.
- The final transition restored the exact timing pair from `360/1` to native
  `30/0` with a successful result.
- All 3,883 guarded comparisons were corrected while the fix was active;
  guard failures were zero.
- Correction switched OFF once and ON twice without a reported transition
  failure.
- All 215 timing transition rows succeeded: one Native request, 213 numeric
  writes and one final restore.
- The numeric writes covered 82 distinct targets from 56 through 360. This is
  evidence of a slider-input defect, not 82 intentional target validations:
  the initial implementation committed every mouse-move value while dragging.

## Demonstrated defects and bounded repair

The first implementation repositioned the overlay every 16 ms, even when the
game window had not moved, and made the panel non-activating. Those choices
explain the reported visual jitter and the need for a game menu to release the
pointer. The slider also applied every intermediate drag value.

The repaired revision:

- activates the overlay when HOME opens it and returns focus to DS3 when it is
  closed;
- treats either the game or overlay as valid foreground ownership;
- changes overlay position only when the game client origin changes and limits
  status repaints to 10 Hz;
- previews slider movement and commits exactly once on button release.

The repaired revision passes the strict build, exact-build/signature checks,
ten-run restoration suite and project audit. It has not yet been injected; a
fresh-process minimum repeat is required before the UI defects can be closed.
