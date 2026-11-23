# I-0011 live result

## Outcome

I-0011 is complete as a reversible exact-build test tool. Two sessions loaded
from clean DS3 processes and ended with exact timing, hook and debug-register
restoration. The runtime exposed live numeric cadence, Native, explicit
no-write Unlimited rejection, measured FPS and the unchanged locomotion
correction without a restart inside either session.

Thomas explicitly reclassified the overlay as test-only infrastructure and
accepted its remaining visual/input limitations. No third overlay patch is
scheduled.

## Second session

Session `20260920T144258Z-pid24832` exercised numeric targets 127, 120, 240,
146, 165 and 360, two Native transitions, three unsupported Unlimited attempts
and correction ON/OFF. All nine timing rows succeeded, including the final
exact native `30/0 -> 30/0` restore. The session ended `complete` with 101,834
updates, 21,478 corrected comparisons and zero guard failures.

The commit-on-release repair is supported live: the second session produced
only six numeric transitions corresponding to completed selections, versus 213
intermediate numeric writes in the first session.

The supplied screenshot records target 165, measured 165.0 FPS, correction ON
and the overlay displayed over the DS3 System menu.

## Operator observation

Thomas reported that the correction worked across every player animation he
observed, explicitly including light sprint and light roll, at the exercised
framerates. He also reported no perceptible enemy slowdown. He interprets this
as confirmation that the temporal branch has been corrected independently of
framerate.

This is retained as a strong `OBSERVED` operator result. It broadens the visible
support beyond the earlier normal sprint/forward-roll witnesses, but it is not
promoted to exhaustive proof for every player animation, enemy update,
simulation/collision, camera, effect or online consumer. Those systems were not
individually instrumented or quantitatively compared in I-0011, so whole-C4
completion remains open.

## Accepted overlay limitations

- The panel still visibly jitters.
- Opening it during gameplay removes character control but does not expose a
  usable pointer; a DS3 menu is still needed for mouse interaction.
- The screenshot and operator report confirm that the controls remain usable
  through that menu path.

The first attempted repair proved that transferring foreground/focus to a
same-process tool window is insufficient to release DS3's pointer state. The
remaining jitter also survived position-change gating. Because this overlay is
not the intended product UI, Thomas directed that these imperfections are not
a blocker. A future product overlay should use a separately scoped input/render
integration instead of extending this test window by trial and error.

## Boundary

Genuine Unlimited remains unavailable through the recovered timing pair and is
not simulated by a high numeric cap. Attack/jump, roll invulnerability/damage,
enemy timing invariance, broader temporal systems, packaging and distribution
remain unqualified unless separately investigated.

