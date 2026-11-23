# I-0114 - Le framerate débloqué: result

## Conclusion

Complete (closed 2026-11-09 with Thomas's yes, « Closed » below); its history: Active (opened 2026-10-30 20:45 by the RT pilot, branch `claude/exciting-visvesvaraya-0537da`; continued 2026-10-31 evening on
`claude/blissful-hugle-47bbb9`, then `claude/stoic-lamport-195f2b`). First merge `a3bf719e` (tranches 1 and 2, all behind
`unlock-*.flag`); second `37eb7f05` (the follow camera by method B, D-0243; the mechanics at 120, 240 and Unlimited as at 60;
I-0111 tranche 2's rest by D-0242: DS3's exposure to DLSS, Reflex Off by default, PCL's markers). Third tranche (below): the
terrain response of slopes and stairs (TRTF's site and its two siblings) and three animation and effect ramps corrected by k;
the display clock with an unlocked rate and the corrections (the first final review's M6) and lock-on with a target checked on
the ASUS; the rate and Reflex as play-launcher options (`-FrameRate`, `-Reflex`; 60 and Off by default, Reflex Off Thomas's
choice, 2026-10-31 22:23). Merged `c74de349`, start check passed. Fourth tranche (below, branch `claude/jolly-mayer-094985`):
the jump's takeoff (its held speed back near 60's at 120, 160, 240 and Unlimited on the ASUS and the OLED, at 120 most jumps within 0.5 %, outliers to 2.7 %, above about 1 % under 60's), the effect
slots' and the task evaluation's steps, the bone constraint and weights, the interface's hold and the child solver; the
terrain field's level and the target timer left open; merged `61ec57c7`, the launcher rebuilt, start check `start-61ec57c7` passed. Fifth tranche (below): the lock-on target manager's ranges by the gain law; the
light-bound fade pair, the broadphase and UNLOCK-READY §6's candidates read, four left open; merged `6d4df5d5`, start check passed. Seventh
tranche (below, branch `claude/zealous-antonelli-08ec08`): the jump's takeoff law 4·(1/60 − dt) (its held speed 60's within ±0.7 % at 120 and 240,
one jump at 120 1.5 % under after a hitch), the enemies' AI timer and ring (INFERRED their steering) (`140D71920`, `SprjHkAiCharacter`), the roll and the lock-on strafe against 60
(the roll's distance and the strafe's speed 60's; the strafe's extra 1-2 % its sooner start, option A, D-0244). Under 60 the jump stays the original's (+1.2 % at 30, +2.7 % at 20 in its held speed: no law found), kept so
(Thomas, 2026-11-01 23:25: « si c'est identique à l'original alors on laisse »). The ledge fall not witnessed. Merged `5ae7905c`, start check passed. Eighth tranche (below, branch
`claude/i0114-terrain-feedback`): the terrain field's level at a given place is the same at 60 and 240 (with the enemies
frozen, `test-freeze-ai.flag`, a new test tool: the climb to the last step +1.8 %, inside the 60 runs' range); series 35's « +18 % »
is all in the leg after the turn, where its 240 run walked another line (INFERRED the cause of the difference); nothing to correct
in the terrain law, Havok untouched; merged `00b2012a`, start check passed. Open: what moves
that line at 240 (the moving enemies, or the camera, whose yaw after the walk down reaches 92° at 240 even with the enemies frozen). Next: R2 and R5; Thomas's eye on the unlocked rate in play.

## Plan

| Tranche | Content | State |
|---|---|---|
| 1 | The lever and the clock: the requested rate from a flag (`unlock-30`, `-120`, `-240`, `-unlimited.flag`), `k = dt · 60` read once a frame in `BeginFrame`, the display clock (I-0111 tranche 2) engaged whenever DS3's period is not longer than the display's refresh, no 1/60 s pacing with an unlocked rate; `unlock-observe.flag` (a counting thunk at each site, native at 60 only) and the diagnostic run of UNLOCK-READY §4 | written, step-reviewed, witnessed |
| 2 | Locomotion (`1405C72B0` both sites, `1405DC510`, `1409CF010`), then the terrain response (the `1405E6580` family), turning (`1405D0010`); the follow camera (`140515790`, from tranche 3: the 240 sprint drift) | `1405C72B0`, `1405D0010`, the tracker `14050C7B0` merged `a3bf719e`; the follow camera by method B (D-0243) merged `37eb7f05`; the terrain family written in the third tranche (series 25-26); `1405DC510`, `1409CF010` read in the fourth tranche: not the jump's (an AI move's class, a predicate), left |
| 3 | Animation and transitions, the camera's other modes (lock-on, aim) and target tracking | the ramps `1407CB390`, `1407F5260`, `1409B6D80` written (third tranche); lock-on with a target witnessed (series 25-26); the bone constraint and weights written in the fourth tranche, the target timer `140D71920` read (corrected in the seventh tranche) |
| 4 | Physics (the proxy listeners read first), effects and post, interface, the rest; the jump (series 27) | the jump's takeoff, the effect slots, the task step, the bone constraint and weights, the hold, the child solver written (fourth tranche; the jump witnessed, series 30-31); the proxy listeners read (nothing to do); YEBIS left to its replacement; the eye adaptation: nothing to do (K-0448: never on in play); fifth tranche: the lock-on target manager's ranges written, the fade pair, the broadphase and §6's candidates read |
| 5 | Reflex on the frame-start wait (R1 tranche 2), the rate in the options, the sweep against the original at 60 | Reflex, PCL and DS3's exposure to DLSS merged `37eb7f05`; the mechanics sweep at 30, 120, 240 and Unlimited (series 21-22); the display clock with an unlocked rate (series 25); the rate and Reflex as play-launcher options (third tranche), the menus' sliders with the menus' redesign (ROADMAP) |

## Tranche 1 (2026-10-30)

- **The rate**: `BeginFrame` writes `[timing+0x354]` (the requested rate) and `+0x358` (its enable) each frame when an unlock flag
  is present, as `original-unlock.flag` does for its measurement (« Unlimited »: a requested 1,000, the frame its own bound).
- **k**: `native_logic_k = [timing+0x268] · 60`, forced to 1 within 1e-4; the corrections of tranche 2 on read it.
- **The clock**: `DecideDisplayClock` engages the display clock when DS3's period is not longer than 1/60 s (within 1 %), so with an
  unlocked rate on a fitting screen the object paces the frames (H1).
- **The counters** (`unlock-observe.flag`): 34 sites (UNLOCK-READY §3's, the decompiler pass's four character-code candidates, and
  DS3's game step `8F2CA0` as the frame count); each entry hooked by a 26-byte thunk (`mov rax, counter; lock inc [rax]; mov rax,
  original slot; jmp [rax]`: RAX and the flags only, so any signature passes through); a row `unlock_observe` every ten seconds
  with each site's calls and calls per game step.

**Diagnostic run, first series** (2026-10-30 21:18-21:23, announced, the ASUS, the replayed Firelink path with Eta, no measure;
build `evidence/I-0114/artifacts/build-1`, `series-1.log`):
- `i114-obs-o60` (the original with the counters) did not start: `Invoke-ReplayRun` keeps a Reference run's flags to a list
  (`unlock-observe` added since).
- `i114-obs-n60` (native, 60, the counters; 12,436 frames, clean): calls per game step over 10,746 steps (`8F2CA0`, which DS3's
  dt selector `DD4B60` follows exactly, one a step): locomotion `5C72B0` 4.05, the look-ahead `9CF010` 4.13, the transform lerp
  `9B6D80` 4.05 (the same count as the locomotion: one a character a step, INFERRED from the four characters of the path),
  terrain `5E6580` 1.14 and its siblings `5CA3F0` 2.96 and `5D2280` 2.91, turning `5D0010` 0.99, the ramp `7CB390` 2.05, the
  tracker `50C7B0` 7.07, the camera `2A5D60` 5.00, the effect slots `245F90` 15.7, the task evaluation `A4CDF0` 7.68, the network
  timers `EF2CE0` 0.29, the two 30-call cycles `663300`/`663340` 1.01, and the four character-code candidates as their families;
  never called on this path: the gait `5DC510`, the ramp `7F5260`, the bone constraint and weights, the target timer `D71920`, the
  camera's clone `2AF510`, the hold value, the child solver, the YEBIS step `16F27E0`, the UI virtual, the 4-call delay, the
  `+0x1A4C` owner, the broadphase. OBSERVED: the counts per step; INFERRED: each object's site runs once a step (the counts
  follow the path's characters and items), so k per step is the right factor (to confirm per object where a site's count is not
  a whole number).
- `i114-obs-n120` (native, `unlock-120`, `display-clock`, the counters): the display clock took the clock (`native_display_clock`
  554 and 592 of 600 frames started on the display's release, 14-15 ms waited a frame, DS3's period 8.33 ms: H1 OBSERVED), then the
  process faulted at 1,648 frames (an access violation executing at the game image + 0x40E86BE0, a protected-code thunk's
  return `0x140E8787A` on the stack: the game's own protected code jumping 1 GB off its image, INFERRED an anti-tamper reaction to
  a modified entry, or a fault of the unlocked logic).

**Second series** (21:34-21:43, the same conditions; build `build-2`, with the locomotion correction; `series-2.log`):
`i114-iso-a1` (`unlock-120`, `display-clock`, no counters) clean, 12,340 frames, the display the clock on 92-98 % of the frames
(`native_display_clock`); `i114-iso-b1` (the same with the counters) faulted again at 1,666 frames, the same way: the fault comes
with the counters at an unlocked rate (not at 60: `i114-obs-n60`), not from the unlocked rate itself (OBSERVED on two runs each;
which of the 34 entries is not isolated: the counters stay a tool for 60 frames/s runs). `i114-obs-o60b` (the original with the
counters) ran to its end but logged no counts (`native: false` in its readiness row: the Reference setup path does not reach the
counters' installation; left, the native at 60 being the original's logic). In `i114-iso-a1` the frame rate was the display's 60,
so k stayed about 1: the locomotion correction did not yet run with k ≠ 1.

**Locomotion, first measure** (`locomotion-observe.flag`: the player's position every 15th game-step call with the logic's own
time; read with [Get-LocomotionLegs.py](Get-LocomotionLegs.py), legs = the distance between two stops). Series 3 (22:20-22:31, build `build-4`,
`series-3.log`): `i114-loc-n60` (native, 60, the reference: the native's logic at 60 is the original's) walked 15.39-15.48 m a
leg (16 legs) and sprinted 18.4-23.2 m; `i114-loc-f120` and `-u120` ran at 60 all the same (12,300 frames: the native's 1/60 s
pacer still paced the presents with V-Sync off; removed since with an unlocked rate, `SetPresentUnpaced`). Series 4 (22:48-22:56,
build `build-5`, `series-4.log`, the logic really at 120: 24,325 and 24,385 frames): `i114-loc-f120b` (the correction) and
`i114-loc-u120b` (`no-unlock-fix.flag`) walked 15.51-15.63 m a leg and sprinted 20.4-23.4 m, the path's total 426.8 and 427.1 m
against 418.4 m at 60 (+2.0 %, the last legs' wall-clock inputs), the median speed 3.936 m/s against 3.933. OBSERVED: at 120 the
walk and the sprint on this flat path match 60 within 1 % a leg, with or without the locomotion correction: the site's decay
branch does not weigh on this path at 120 (its distance branch needs the mode 3 or `[+0x260]` above its bound, INFERRED not met
in a plain walk). The correction stays (bit for bit at 60, its law right by the unit test); its effect is to be seen at 240 and on
stops and slow walks (UNLOCK-READY §5).

**Locomotion at 240** (series 5, 23:05-23:13, build `build-5`, `series-5.log`; 38,578 and 39,745 frames, about 190 a second):
`i114-loc-u240` (no correction) walked 15.54-15.65 m a leg but sprinted 12.9-13.4 m (8 sprints) against 18.4-23.2 m at 60: LTF's
defect, OBSERVED here (the sprint's factor decays as a stop when a call's distance falls under 1/30 m). `i114-loc-f240` (the
locomotion correction) walked the same and sprinted 22.6, 23.4, 19.1, 23.1 and 16.2 m (the first five sprints as at 60), then
4.2, 0.4, 8.3 and 7.4 m: the route left 60's (the height 3.7-4 m lower at the end, -59.8 against -55.9: INFERRED the character
turned faster at 240, the turning site 1405D0010 being uncorrected, and its last sprints met another ground). The turning
correction (`1405D0010`: the speed's gain a call `coeff[0]·k`, passed in a copy of the coefficients) written next.

**Series 6** (23:30-23:38, build `build-6`: the locomotion and turning corrections; `series-6.log`): `i114-loc-f120c` (120,
24,373 frames) ends where 60 ends (269.8, -55.6, 611.7 against 269.6, -55.9, 610.2), its legs and sprints as at 60.
`i114-loc-f240b` (240, 36,313 frames) ends where `i114-loc-f240` did without the turning correction (263.2, -59.8, 627.0): the
turning site changes nothing measurable on this path. Aligned on the first motion, the 240 run's position follows 60's within the
walk's phase (1.3-8 m, the legs' alternation) until the sprints, then drifts sideways during them (x 270 → 265 → 262 at 132-144 s
of logic, where 60 keeps x = 270 ± 0.3) and leaves the route: a sideways drift of the sprint's heading at 240, not its length.
INFERRED candidates: the tracker (`50C7B0`, 7 a step), the transform lerp (`9B6D80`), the camera's own follow (not yet in the
list). Not `2A5D60`: read whole (GhidraMCP), it gathers the lights of a view (types 1-6 into a light table) and decays a light
bound's fade `[+0x2860] *= 0.55`: UNLOCK-SITES's « camera » name was a guess, it does not steer the character.

**Series 8** (2026-10-31 01:28-01:36, build `build-8`, `series-8.log`: the study thread's first test, CAMERA-FOLLOW-SITES §4.1 of
I-0112, `no-wall-recovery.flag` clearing the follow camera's automatic wall recovery gate `[0x14455BA10]` each frame; its row
read the game's value 1 before the first clear in both runs). `i114-wr-f240` (240, 38,177 frames) still drifts during the
sprints (x 270 → 264.5 at 135 s of logic) and ends at (263.3, -59.8, 627.0), where `i114-loc-f240b` ended with the recovery on;
`i114-wr-f60` (12,480 frames) ends at (269.9, -55.2, 613.5), as 60 did with it on. Observed: the wall recovery is not the
drift's cause. Next (§4.2): the follow camera's yaw `[+0x144]`, its step `[+0x314]` and its focus `[+0x90]`, read from the
camera object a thunk at `140515790`'s entry keeps, in the position rows of `locomotion-observe.flag`, beside the physics
module's floats at `+0x70` (the character's rotation, INFERRED): whichever leads the drift names the site.

**Series 9** (02:10-02:18, build `build-9`, `series-9.log`; the rows above, plus the lower-body twist's tracker by the study
thread's chain from PlayerCtrl): `i114-cam-f240` (37,331 frames), `i114-cam-f60` (12,451). The physics module's float `+0x74` is
the character's heading (it flips by π at each leg's reversal). The twist's tracker reads target 0, value 0, gain 1, mode 0 all
along the path: not the drift. Observed, at 60: after the replay's stick turn (38-41 s of logic) the camera's yaw never moves
again (−0.024 ± 0.002 from π through 60-100 s) and the character's heading equals it on every leg (−0.024 away, 3.118 toward).
At 240: the heading settles 0.04-0.05 rad off the camera's yaw after the first legs (+0.018 away, −3.124 toward, while the yaw
is −0.035 from π) and stays fixed through each leg; the camera's yaw then turns whenever the character walks toward it
(−0.03 → −0.12 from π over a leg, the focus moving on a line off its axis) and back on the next leg, by up to 0.19 rad in the
walk and 0.58 in the sprints, and the motion follows it (the sprint's velocity points where the camera looks). INFERRED: the
camera follows the character; the error is the character's heading not converging to the input's direction at 240. The
turning site `1405D0010` (GhidraMCP, read whole) applies a turn only while the heading's error is not growing or is above a
bound (`error <= previous error`, or `rate · coeff[2] <= error`); otherwise it stops the turn (flag `out[9]`, speed 0). A
residual error below the bound that grows as the camera turns is then never corrected: a feedback between the heading and the
camera, which 60 never enters because its turns end exactly on the target. Next: the player's turn calls (their error
`out[5]`, the stop flag `out[9]`, the step) at 60 and 240, to see where the first residual comes from.

**Series 10** (02:47-02:59, build `build-10`, `series-10.log`; the player's calls of `1405D0010`, told by their previous state's
heading equal to the physics heading, with the turn detour observing at 60 too): `i114-turn-f240` (37,691 frames), `i114-turn-f60`
(12,464), `i114-turn-f240nofix` (38,954, `no-unlock-fix.flag`). Observed:
- The turn site is not the cause: in all three runs it never stops (`out[9]` 0 on every player call) and ends on its target on
  all but 23 calls (`out[8]` 0); its rate is 12.566 rad/s (720°/s), `coeff[0]` 1, `coeff[2]` 0 throughout. The series 9 reading
  of a growing-error stop is refuted: the heading is always the turn's target.
- The target is what differs. At 60 the heading follows the camera's yaw within a leg (43.5-45.8 s of logic: the yaw −0.011 →
  −0.026 from π, the heading π − 0.007 → π − 0.022). At 240 it does not: the heading stays fixed through the leg (44.2-46.9 s,
  π − 0.005 while the yaw goes −0.009 → −0.033), then moves the other way (47-48 s: the yaw −0.035 → −0.079, the heading
  π + 0.005 → π + 0.016), and the velocity follows the heading. The camera, following the character, turns further: in the long
  sprints (124-128 s) the heading stays at π − 0.045 while the yaw goes −0.035 → +0.33, and the character leaves the route.
- Without the corrections (`nofix`) the heading's offset from the yaw is the same in the walk (0.12-0.21 rad at most), but its
  sprints stay at 13 m (LTF's defect), too short for the drift to build; it ends at (270.1, −55.7, 610.8), near 60's
  (269.9, −55.2, 613.3), by that accident. The corrections are not the drift's cause; they make the sprints long enough to show it.
INFERRED: the turn's target (the current heading plus the wanted delta of `1405C2400`, capped at rate · dt, the study thread's
reading) misses or inverts the camera's rotation at 240: a per-frame term in the wanted direction. Next: the wanted delta
read statically, then logged.

**The wanted delta** (2026-10-31 03:10-03:14, run `i114-loaded-f60`, not a measure): `1405E2810`, encrypted at rest, read
decrypted from the running game (`Read-LoadedCode.ps1`, three identical reads, `evidence/I-0114/artifacts/loaded-5e2810-*.json`,
local only) and decompiled by the study thread (`decompiled/DS3-1152-F-005E2810.loadedi114f60.c`, CAMERA-FOLLOW-SITES §8): no dt,
no per-frame term; in free movement the delta `[manip+0x20]` is `wrap([manip+0x90..] − heading)`, and `[manip+0x90]` (pitch, yaw)
is written by `140510290(CameraMan, chr)` from CameraMan's forward `+0x30..0x38`, once per CameraMan update.

**Series 11** (03:36-03:44, build `build-11`, `series-11.log`; the manipulator's angles and delta, CameraMan's forward, and the
calls of `140510290` and `1405E1B80` by thunks): `i114-aim-f240` (38,006 frames), `i114-aim-f60` (12,494). Observed:
- `140510290` and the manipulator's update run once per game step at both rates (15 of each per 15 steps at 60, 60 per 60 at
  240): not a frequency or a skipped call.
- CameraMan's forward yaw equals the character's heading within 0.001 rad at both rates (outside the 180° reversals), and the
  wanted delta is ≈ 0 (at most 2·10⁻⁴ rad a step at 60 while the heading turns): the heading follows CameraMan's forward.
- What differs is CameraMan's forward against the follow camera's yaw `[+0x144]`: |difference| 0.0015 rad on average at 60
  (0.015 at most) and 0.050-0.085 at 240 (0.17-0.20 at most), through the walk and the sprints.
INFERRED: at 240 the follow camera's yaw `[+0x144]` and the view CameraMan copies part ways; either the view does not follow
the yaw, or the yaw turns on its own while the view (and the character) stays. Next: where CameraMan's forward is written from
(the follow camera's matrix or its yaw) and what else writes `[+0x144]`.

**Series 12** (03:58-04:06, build `build-12`, `series-12.log`; the follow camera's view law, read statically by the study thread
in `140515790`'s end, CAMERA-FOLLOW-SITES §9: the view's position P follows the wanted position `[+0xF0]` (from the yaw `+0x144`)
from the previous one `[+0x100]` by a per-frame weight `w` without dt, zero horizontally while the angle g between them, seen from
the focus, is under `a1 = [+0x1CC]`, ramping to full at `a2 = [+0x1D0]`): `i114-view-f240` (38,051 frames), `i114-view-f60`
(12,478). Observed: `a1` 0.1745 rad (10°), `a2` 0.3491 (20°), `w_h` 0.100, `w_v` 0.030, the recentring timer 0 throughout, at both
rates. At 60, g stays at 0.0016 rad on average (0.016 at most) and never reaches `a1`: the view's position and the wanted one
move together. At 240, g is 0.053-0.079 on average and reaches `a1` on 65 of 1,168 rows: the view sits in the dead band while
the yaw `+0x144` turns, and catches up by 10 % a frame only at its edge. INFERRED: the dead band is the mechanism that holds
the view and the character's heading; what makes g grow at 240 is upstream, in how the wanted position (`+0x144` through the
lazy position `+0xE0`, auto mode, §9 (2)) moves against the view's own position between two frames.

**Series 13** (04:18-04:26, build `build-13`, `series-13.log`; the yaw's trail, CAMERA-FOLLOW-SITES §10): `i114-trail-f240`
(37,178 frames), `i114-trail-f60` (12,477). Observed: the trail's weight W = lerp(`[+0x1B4]`, 1, `[+0x164]`) is 0.3 in the walk at
both rates (`[+0x1B4]` 0.3, `[+0x164]` 0, 1 after an input), the focus gain `[+0x1A8]` 0.1 at both: not the difference. The
directions seen from the focus, in one walking leg toward the camera (43-47 s): at 60 the lazy position E0 turns −0.011 → −0.027
rad and the view's position P follows it 0.004-0.007 behind (−0.007 → −0.021); at 240 E0 turns −0.010 → −0.033 while P stays at
−0.005 the whole leg. F0 equals E0's direction at both rates (step 9). So at 60 P moves horizontally although g (0.004-0.007) is
far below the dead band's 0.1745, and at 240 it does not: the view's horizontal motion at 60 comes from a term the dead band
reading does not account for, and that term is what 240 lacks.
The study thread's reading of the series (CAMERA-FOLLOW-SITES §11), checked on these rows: P is a pure trail at both rates (the
fraction of the gap it closes a frame is 0 in the median at both); it turns at 60 because the focus moves sideways against its
axis (the heading 0.0003-0.0005 rad behind the view while it turns, the row's mean velocity 0.001-0.002 off the axis), and not at
240 (heading − view about 10⁻⁵). INFERRED then: a lag of the heading counted in steps seeds the view's turn at 60.

**Series 14** (04:44-04:52, build `build-14`, `series-14.log`; `view-delay-<n>.flag` gives the heading's target the view's copy of
n steps before): `i114-delay3-f240` and `i114-delay6-f240` end at (264.2, −59.6, 626.4) and (263.3, −59.8, 627.0), where the
uncorrected 240 runs end. Refuted: a lag of the heading's target, given 60's time, does not bring the path back. Repeatability:
the seven 60 runs of series 8-13 end within 0.2 m of each other (269.7-269.9, −55.15, 613.3-613.5); six of the seven 240 runs
end at (263.2-264.2, −59.6 to −59.8, 626.4-627.0), one (`i114-trail-f240`) at (267.4, −55.2, 619.0). Next: the follow camera
stepped at 60 inside a 240 run (`camera-step-4.flag`: `140515790` on one call in four with dt × 4), to tell its own laws from
the rest of the loop.

**Series 15** (05:05-05:13, build `build-15`, `series-15.log`; `camera-step-4.flag`, the corrections on): `i114-cam4-f240` (39,683
frames) and `i114-cam4-f240b` (40,152) end at (269.97, −55.63, 611.64) and (269.97, −55.63, 611.66), 0.3 m from each other and
within 2 m of 60's ends; their sprints are all 20.4-23.4 m, as at 60, and the path's total 427.4 and 428.2 m (60: 420). Observed:
with the follow camera stepped at 60, the 240 path is 60's. The drift's cause is in `140515790`'s own per-call laws (the view's
per-frame trail and weights, the dead band judged a frame, the Euler focus and pitch, §9-§11), not in the character, the turn or
the input. The correction belongs there: each law given its 60 behaviour at any rate (or the camera's logic stepped at a fixed
1/60 with its view interpolated for the display, the cheaper and more general route, to weigh).

**The correction: the follow camera's logic at a fixed 1/60** (`camera_step`, 04; the study thread's map of the object,
CAMERA-FOLLOW-SITES §13: 0x440 bytes, no write outside it but through its embedded fade slot `[0x3D8, 0x430)`). Each call
restores the state of the last tick (S60) over `[0x10, 0x3D8)` and `[0x430, 0x440)`, carrying into it any byte written by others
since its last return; runs one call with dt = 1/60 for each 1/60 owed and saves S60; then the display's call from S60 with the
rest owed, without the collision world and with the fade byte `+0x3D5` held at 0; the heading's target (`140510290`) reads S60's
forward. A dt within 10⁻⁴ of 1/60 is the original's call. Behind `camera-fixed-step.flag` (on by default with the corrections in series 19, back behind the flag by D-0243).
**Series 16** (05:31-05:39, build `build-16`, `series-16.log`): `i114-fixed-f240` (37,646 frames) and `i114-fixed-f240b` (38,379),
the corrections and `camera-fixed-step.flag` on, end at (269.91, −55.64, 611.63) and (269.92, −55.63, 611.64), as the runs with the
camera stepped at 60 (series 15), within 2 m of 60's ends; sprints 20.1-23.4 m, paths 426.6 and 427.1 m. Observed: the
correction brings the 240 path back to 60's, repeatably. Open: the display's smoothness (the view between two ticks is an
extrapolation from S60, not yet measured), the walk's residual 1 % (15.62 m legs against 15.46 at 60, the locomotion's), and
the correction's step review before it becomes the unlock's default.

**Step review** (fresh reviewer, 9aa24353..2b5ce154): M1, the display's call from S60 skipped the collision and ran the per-call
laws in full on each display frame (a staircase at the ticks, the camera off the walls between them); M2, the heading's target
was swapped whatever camera ChrCam followed; L1-L7 (a new object at a freed address, a 1/60 dt dropping S60, bytes instead of
words, comments, the thread assumption, the test rows, silent refusals). **Series 17** (05:54-06:06, build `build-17`, the first
version; the per-600-frame smoothness rows: mean per-frame displacement d1 and second difference d2 of the view's position, its
yaw, the character's position): the view's d2/d1 is 0.011 at 60, 0.008 at 240 uncorrected and 1.25 with the first version
(the character's 0.040-0.048 in all three): M1 confirmed, the first version judders. Answered (894273cc): the display is the
blend of the last two ticks at owed / (1/60) (rows orthonormal again, position and field of view blended; up to one tick,
16.7 ms, behind), no law runs on a display frame; S60 kept through a 1/60 dt; aligned words carried; the destructor
`1405130F0` forgets the object; the heading's target swapped only while `[ChrCam+0x60]` is the kept camera (`1404095F0`, the
copy, read: plain); the refusals named; `seen.dt` and the smoothness gaps in the rows. Then (be21b8e5 on the branch, the blend in
`native/world/view_blend.h` with `view_blend_test`): the blend is written at alpha 0 too (the previous tick), where the first
answer showed the current tick, a one-tick jump at each tick. Next, after Thomas's play session (06:20, the PC his): build 18 and
two 240 replays for the path and the smoothness rows.

**Series 18** (2026-10-31 09:11-09:19, after Thomas's play, build `build-18` = the blend at alpha 0 too; `series-18.log`):
`i114-blend-f240` (38,233 frames) and `i114-blend-f240b` (40,139), the corrections and `camera-fixed-step.flag` on, end at
(269.92, -55.64, 611.62) and (269.93, -55.63, 611.66), the same as series 15-16; sprints 20.4-23.4 m, paths 428.0 and 427.7 m. The
smoothness rows: the view's d2/d1 0.009 in both (60: 0.011, 240 uncorrected: 0.008, the first version 1.25), the yaw's d2
4·10⁻⁷ (60: 2.3·10⁻⁶, 240 uncorrected: 1.5·10⁻⁶), the largest camera second difference 0.002 (the first version 0.044).
Observed: the corrected camera brings the 240 path back to 60's and its view is as smooth as the uncorrected 240's. Next: the
correction on by default with the unlocked rate (not behind its flag), its review, and Thomas's eye on it.

**Series 19** (10:26-10:34, build `build-19`: the camera's fixed step on by default with the corrections, `no-camera-fixed-step.flag`
for the A/B; `series-19.log`): `i114-default-f240` (39,757 frames) and `i114-default-f120` (24,777) end at (269.94, -55.63, 611.65)
and (269.97, -55.63, 611.66); walk legs 15.62-15.65 and 15.57-15.62 m, sprints 20.2-23.4 m, paths 427.3 and 426.9 m; the
`unlock_fix` row names `follow_camera`. Observed: by default, 120 and 240 keep 60's path.

## The follow camera by method B (D-0243; second thread, branch `claude/blissful-hugle-47bbb9`, 2026-10-31 evening)

**The laws** (`camera_laws`, `04_updates_and_test_tools.inc`; read in the full decompile `evidence/I-0112/ctx/full/140515790.c`,
line numbers there). Every per-call law of `140515790` that depends on the frame rate is a gain, rate or divisor field the call
reads; a detour at its entry writes the law's value with k = the call's dt · 60 into the field for the call and puts the field back
after it (keeping what the call itself wrote; at k = 1 the original's call). Nine laws, each with `no-camera-law-<name>.flag`:
focus (`+0x1A8`, k30·g Euler, `logic_rate::Euler30`, its ramp and reset redone on the true values after the call), pitch
(`+0x1B8`: the frame's change of pitch kept by a fraction g·k30, `logic_rate::Fraction30`, 60's steady deviation kept), view (P's
weights a call `+0x1BC` ramped, `+0x1C8`, `+0x1D8`, `+0x1DC`: `Gain`), distance (`+0x1D4`: `Gain`), row (`+0x3D0`: `Gain`), lock
(`+0x288`, `+0x28C`, `+0x300`: `Euler30`), speed (the distance `+0x190` from the focus's displacement a call over `+0x180`:
`PerCall` on the divisor, a law CAMERA-FOLLOW-SITES did not list), recentre (`+0x130` used as a gain a call after a stick input:
`Gain` on the value the call uses), wall (`+0x324`, `+0x334` `PerCall`, `+0x360` `Ratio`, the angle `+0x314` carried in the
current rate's units). Not corrected at first: the wall recovery's snap under one degree a call (a shared constant; patched since
the step review, below); left as it is: the dead band, whose
angle is the gap accumulated between F0 and the last view (CAMERA-FOLLOW-SITES §11 (a)), a state rather than a frame's increment.
Unit tests (`logic_rate_test`): `Euler30` and `Fraction30` keep g at k = 1; n calls at k = 1/n equal one at 60; `Fraction30` keeps
60's steady deviation at 0.25, 0.5, 2 and 0.37. Build `build-21` (all unit tests pass). `camera-fixed-step.flag` keeps the fixed
step as the reference (`camera_step`, unchanged).

**Series 20** (2026-10-31 19:16-19:24, announced, the ASUS, the replayed Firelink path with Eta, V-Sync off, no measure; build
`build-21`, `series-20.log`; the laws on, none switched off): `i114-laws-f240` (38,297 frames) ends at (269.85, -55.66, 611.56)
and `i114-laws-f120` (24,662) at (269.95, -55.63, 611.66), where the fixed-step reference ends (series 16-19: 269.91-269.97,
-55.63/-55.64, 611.62-611.66) and within 2 m of 60's (269.7, -55.15, 613.3); uncorrected, 240 ended at (263.2, -59.8, 627.0).
Walk legs 15.61-15.65 m (240) and 15.56-15.63 m (120), sprints 20.5-23.4 and 20.4-23.4 m, paths 427.6 and 426.8 m (60: 420.5),
UNLOCK-READY §5's 2 % met. The smoothness rows over the path (before the replay's closing camera turn, whose rows show a
second difference twice the first at 60 and uncorrected 240 alike, a property of that input, not of a law): the view's d2/d1
0.0088 at 240 and 0.0064 at 120 (60: 0.011, uncorrected 240: 0.0082, the fixed step 0.0090), the yaw's d2 1.1·10⁻⁶ and
2.2·10⁻⁶ (60: 2.3·10⁻⁶). Observed: by method B the follow camera brings the 240 and 120 paths back to 60's, as smooth as 60,
without the fixed step's tick of latency. Which of the nine laws carries the drift is not isolated (no A/B run yet).
The mouse sweep at the path's end (200 moves of 20 units, after the motion) turns the follow camera by 34.6 rad at 240 and 120
with the laws, as at 60 (34.2 and 34.7 in two runs) and uncorrected 240 (35.5); the fixed step turned it by 11.5 rad at 240 and
16.3 at 120 (`i114-default-*`): it dropped the mouse's input on the calls between its ticks, one more reason for method B.

**The step review** (a fresh reviewer, `3a0f660b..a38c85e4`, with the turning and tracker corrections, the final review's M3): no
high finding. M1 the wall recovery's snap (under one degree a call → 0) gates a contact's onset, so corrected rates without it
needed k² times the 60 frames/s rate to start: answered by pointing the snap's load (`14051897C`, the shared constant
`0x143D7ED40`) at a module float the detour sets (`logic_rate::WallSnapThreshold`: one degree · k once a 1/60 s from a contact's
start, 0 between, one degree at k = 1). L1 the recentre timer decayed after a call that skipped its decay (no character):
restored by result (`RecentreAfter`). L2 `Gain` guarded to [0, 1] (no NaN). L3 the restores as tested laws (`RampedAfter`,
`GiveRecentre`, `RecentreAfter`, the snap's gate), the tests at k = 2 and with a relative tolerance. L4-L6 the approximations
named in the comments (the view gain read after its ramp, the lock gain's factor, the call's k against the frame's). The turning
and tracker corrections: right as written (M3 closed). The test hooks of series 14-15 (`view-delay-<n>`, `camera-step-4`) removed
(the final review's L5). The parameters (`camera_laws_params`, build-23): focus 0.1, pitch 0.3, view 0.1 and 0.03, distance 0.4,
row 0.05, lock 0.06, 0.3 and 0.3, speed 0.3; the wall's rate 1.047 rad/s (one degree a call at 60 exactly), far rate 0.175,
max 4.19 rad/s, decay 0.55: so the snap bites (uncorrected, a contact never starts above 60).

**Series 21** (20:13-20:31, announced, the ASUS, V-Sync off, no measure; build `build-23`, `series-21.log`): the mechanics path
(`scripts/replay/firelink-mechanics.path.json`: rolls while walking, stops after a 2 s sprint, 90° turns from a standstill in a
square, light attacks, backsteps; `mechanics-observe.flag`: the rows every 1/30 s of the logic's time) at 60, 30, 120, 240 and
Unlimited (`i114-mech-n60`, `-f30`, `-f120`, `-f240`, `-fu`: 8,286, 4,295, 17,912, 27,957 and 28,438 frames; Unlimited ran at about
the 240 run's rate on this PC), then the Firelink path at 240 with the patched snap (`i114-snap-f240`). Read by event (speeds from
the rows, not from the keys' times, whose alignment drifts):
- rolls (above 4.6 m/s): 3.35-3.59 m in 0.40-0.45 s at 60, 3.50-3.58 at 120, 3.47-3.54 at 240, 3.42-3.63 Unlimited, 3.44-3.75 at 30
  (the 60 and 30 spreads are the rows' 1/30 s sampling); peaks 10.3-11.2 m/s at every rate;
- the sprint 5.58-5.62 m/s at 60, 5.57-5.74 above; the stop from it (the last row at 5 m/s to under 0.2): 0.70-0.90 m in
  0.42-0.50 s at 60, 0.70-0.90 at 120, 0.70-0.89 at 240, 0.73-0.89 Unlimited (the same two clusters at every rate);
- light attacks' lunges 1.27-1.28 m in 0.33-0.36 s and backsteps 2.42-2.47 m in 0.89-0.92 s at 60, 120, 240 and Unlimited;
- turns of 90° from a standstill: the heading settled in 0.10-0.22 s or 0.48-0.55 s (by the leg) at every rate; the follow
  camera turned 0.38-0.41 rad on the sideways legs at every rate;
- 30 frames/s: the same within its frame's resolution (33 ms), with a few events off (a stop read as 0, an attack 0.74 m, a
  backstep 2.06 m): the 50-80 ms taps fall on two or three frames there; not a law's error by the rest of its data.
- `i114-snap-f240`: ends at (269.95, -55.65, 611.60), the same as series 20; no fault with the patched load.
OBSERVED: rolls, stops, sprints, attacks, backsteps and turns at 120, 240 and Unlimited are 60's within the rows' resolution
(UNLOCK-READY §5's 2 %, the camera's 5 %). Not covered yet: slopes and stairs, lock-on and target switching (no enemy at
Firelink), animation blends by their ramp fields, physics, effects and post, interface (UNLOCK-READY §5's other rows).

## DS3's exposure to DLSS and Reflex (I-0111 tranche 2's rest, inside this workstream by D-0242; 2026-10-31 evening)

- **The exposure**: each DLSS frame writes the tone map's Exposure (the blended Tone Map record's word 0, the multiplier of the
  linear HDR the tone map applies; K-0448: never automatic in play) into a 1x1 R32_FLOAT (`RecordDlssExposure`), tagged as DLSS's
  exposure with `useAutoExposure` off; 1 on a frame without a tone-map input (its composition does not expose the HDR). Always
  given, because sl.dlss fixes its exposure mode when it builds its instance (step review M1).
- **Reflex** (D-0240: a player option): `reflex-mode.txt` beside the module (off, on, boost; Off by default, Thomas's choice to
  make, NVIDIA's checklist asks On), the test flags `reflex-on` and `reflex-boost`. Streamline loads Reflex and PCL with DLSS
  (`sl.reflex.dll`, `sl.pcl.dll` copied by the build); the options sent at least once, Off included; the sleep every frame at the
  frame-start point (the end of `EndFrame`); with Reflex on it is the frame-start wait and the display clock is not engaged
  (the worker then waits the frame-latency object as without the clock); PCL's markers: simulation end at `EndFrame`'s entry and
  start at its end (frame serial + 1), render submit start before `ExecuteFrame`, submit end and present start and end around
  `Present` (the job's serial; one index space with DLSS's frame token). Not done: the latency ping (`ePCLatencyPing`, a window
  subclass), the flash indicator; Reflex and PCL run only where DLSS runs (an adapter with Reflex and no DLSS gets neither).
- **Step review** (`654381dd..113f6a4a`): no high. M1 the exposure's presence fixed at DLSS's creation (answered: always given);
  M2 Reflex on with the display clock left the object unwaited (answered: the clock not engaged with Reflex on); L1 the sleep only
  when on (answered: every frame); L2 the frame tokens from two threads (answered: one lock); L3 the lock each frame (a fast path);
  L4 the flag after a shutdown; L5 the row's « sent » (the driver's `lowLatencyAvailable` added); L6 the simulation start before
  DS3's limiter and no ping (noted); L7 Reflex tied to DLSS (noted). Build `build-26` with the answers (unit tests pass).

**Series 22** (20:56-21:15, announced, the ASUS, no measure; build `build-25`, before the review's answers; `series-22.log`):
- the mechanics path with the recentre (the middle button, lock-on without a target) at 60 and 240 (`i114-mech2-n60`, `-f240`):
  the follow camera settles 0.39 s after the press at 60 (1.38-1.46 rad turned) and 0.36-0.41 s at 240 (1.40-1.52 rad);
- `i114-vsync-f120` (`unlock-120` with V-Sync, no `display-clock.flag`): 60 frames a second on the 60 Hz ASUS, the path ends at
  60's (269.70, -55.16, 613.59);
- `i114-ab-speed-f240` (`no-camera-law-speed`): ends at (269.95, -55.66, 611.58), as with every law: the speed law does not
  carry the drift;
- `i114-reflex-f120` (Reflex On, `unlock-120`, V-Sync) and `i114-reflex-n60` (Reflex On at 60): `native_reflex` « on », sent,
  no Streamline warning or error; both end at 60's (269.91, -55.21, 613.01 and 269.81, -55.20, 613.12); DLSS evaluated every
  world frame with the exposure (`native_dlss_frames`, no refusal).
Not run yet: the display clock with an unlocked rate and the corrections (the final review's M6, `display-clock.flag`), Reflex's
latency read by NVIDIA's tools (the FrameView SDK, Thomas's call), the OLED's high rates (Thomas present).

## Third tranche (2026-10-31 night, branch `claude/stoic-lamport-195f2b`)

**The terrain response** (`unlock_fix`'s terrain detours, 15; GhidraMCP, the three functions and their probes read whole): TRTF's
site `1405E6580` (the timing project's I-0013, the hopping on slopes and stairs at 240) and its siblings `1405D2280`, `1405CA3F0`
update `[this+0x2DC]` per call with no dt: a response added (`x + 0.2·min` of the ground probes' answers, or of the ground query's
answer less a negative vertical speed) or a decay (`x·[+0x378]`, `x·0.6`), clamped. Each call is corrected after it from the value
before it (the step times k, the decay to the power k, `logic_rate`'s terrain laws and their unit tests), its branch told from what
the call left and checked against the original's own expression (the probes `1407D7180`, `1407D7780` and the query `1409A21E0`
seen inside the call only); a call matching none is left as written and counted (`unlock_terrain_mismatch`). `no-terrain-fix.flag`
keeps the original (the A/B); `locomotion-observe.flag` adds the player's `unlock_terrain` rows (value, branch, the total variation
of the field, i.e. the sum of |after − before|, TRTF's hopping measure). Corrected after the call rather than pre-biased as
UNLOCK-READY §3 planned for `1405E6580`: its response's increment comes from probes answered inside the call (the final review).

**The ramps** (pre-biased, UNLOCK-READY §1 method A: the original's own step lands on x + c·k): `1407CB390` (a 60-call fade
`[+0x24]` up or down by `[+0x28]/60`, 2 calls a step at Firelink), `1407F5260` (a character's ragdoll blend `[+0x1E0]` by 1/30 to 1,
kept under 1 so the step is taken; not called on the paths run), `1409B6D80` (a blend factor `[+0x60]` by 0.1 under a state bit,
put back when the call left it as written). Not witnessed live in a way that tells them apart (no run triggers a visible fade).

**Series 23's fault** (2026-10-31 22:26-22:35, build `build-28`, `series-23.log`): every run faulted 1-2 s into the world,
at 60 too with the detours only observing, in Havok's contact sort (`1416374A0`, a quicksort that walks off its array on a NaN key)
under the player's mover step. Bisected (series 24, 22:46-22:53, build `build-29`): the three sites alone fault, the probes and
the query alone and no detour at all run clean. The cause: `1405E6580` and `1405D2280` return their field in XMM0 (their epilogues
load `[rbx+0x2DC]` before `RET`; `1405E6580`'s delegate path passes `1405CA3F0`'s XMM0 through), decompiled as `void`; the detours
were `void` and left the caller a stale XMM0 it carries into the mover. Fixed (`23d89157`): the three return the original's value,
or the corrected field when they wrote it; the ramps' detours forward RAX and do no float work after the call. The two runs left
with uncertain flags (`i114-dc-fu`) never started; the save's hash was checked after each stop.

**Series 25** (23:19-23:47, announced, the ASUS, no measure; build `build-30` = `23d89157`, `series-25.log`) and **26**
(23:48-23:57, `series-26.log`):
- The stairs (`scripts/replay/anorlondo-stairs.path.json`: Gamma at the Anor Londo bonfire, the great stairs walked up and down
  twice, then sprinted; read with [Get-Stairs.py](Get-Stairs.py)): the three first walking legs are the same route in every run
  (31.2-31.4 m, ±7.3 m, 3.9 m/s; within 0.1 m of each other to 26 s). Over those 28 s the player's terrain response varies by
  0.79 a second at 60 (`i114-st2-n60`), 1.51 at 240 uncorrected (`i114-st2-u240`), 0.70 and 0.72 at 240 corrected
  (`i114-st2-f240`, `i114-st3-f240`), 0.75 at 120 (`i114-st2-f120`); the branch classification matched the original on every
  call of every character (0 mismatched). These figures are build-30's (`23d89157`), before the step review's answers: each
  call delegated to `1405CA3F0` was noted twice (the variations above are twice the field's own; their ratios hold) and the
  bounded `1405CA3F0` calls were corrected as steps (since left as the original's unless they re-enter the bounds): the merged
  law's terrain has not run. OBSERVED on build-30: TRTF's defect reproduced (the field's variation about doubled at 240) and
  brought to 60's within 12 %; the field's level is not (its mean |x| over the stairs motion 0.0755 at 60, 0.102-0.123 at 240
  corrected, 0.104 at 120, 0.155 at 240 uncorrected: UNLOCK-READY §5's curve within 2 % not met). Not resolved by these rows: the field's curve sampled every 1/30 s is 0.031 from 60's (30 % of
  its mean) corrected and uncorrected alike (the hop is faster than the rows). Limits: after the third leg the path is not
  repeatable (both 240 runs of series 25 skip 3.2 s of logic in 2-3 game steps on the second descent, the world not stepped;
  `i114-st3-u240` left the stairs at the third leg's start); `i114-st2-f240` stopped 0.8 s early at the top of its third climb,
  `i114-st3-f240` did not (run noise).
- M6, the display clock with an unlocked rate and the corrections (`i114-dc2-f120`, `-f240`, `-fu`: `display-clock.flag`, V-Sync,
  the Firelink path): the clock engaged on every frame and the display released 597-600 of each 600 (about 10-12 ms waited a
  frame on the 60 Hz ASUS); the path is 60's (legs 15.46 m, 425.0-425.1 m, the ends (269.92-269.94, -55.20 to -55.28,
  612.6-613.2)). The logic ran at the display's 60 (k about 1): the high rates under V-Sync need the OLED (Thomas present).
- Lock-on with a target (`scripts/replay/settlement-lockon.path.json`: Beta at the Undead Settlement bonfire with
  `test-no-ai.flag`, a hollow locked, strafes around it, a target switch by the mouse; [Get-Lockon.py](Get-Lockon.py)): at 240
  (`i114-lock-f240`, `i114-lock2-f240`) the follow camera turns 1-3.4 % more than at 60 on the strafes (+1.142/−1.107 rad against
  +1.126 to +1.133/−1.068 to −1.072; the two 60 runs agree within 0.5 %), the switch the same (1.273-1.280 against 1.279-1.285 rad
  in 0.12-0.18 s), and the strafes cover 2-3 % more (5.59-5.83 against 5.54-5.70 m). Within UNLOCK-READY §5's 5 % for the camera;
  the strafe's distance slightly over the locomotion's 2 % (the walk's residual is +1 %): open. Series 25's lock runs lost the
  window's foreground at the middle click that released the lock (both at the same event); the path keeps the lock since.
- `i114-lock-obs-n60` (`unlock-observe.flag` at 60 on the lock path, to count the lock-on's sites) faulted executing at
  image + 0x40E86BE0, the counters' fault of tranche 1 (there with an unlocked rate only): the counters are not a tool for this
  zone. The lock path's file was edited while that run was starting (against WORKFLOW « Live game rule »; its replay lost the
  foreground at its first event, so its input is not read).

**Merged** `c74de349` (2026-11-01 00:45), the launcher rebuilt from it (its unit tests pass), start check `start-c74de349`
passed (01:00, the ASUS).

**Series 27** (2026-11-01 01:06-01:20, announced, the OLED 2560x1440 at 280 Hz with Thomas present, V-Sync, no measure; build
`build-33` = the merged native code; `series-27.log`; Thomas, 00:27: « La distance parcourue lors d'un saut, roulade, sprint est
fausse au dessus de 60fps »):
- The jumps (`scripts/replay/firelink-jumps.path.json`: Eta at Firelink, eight jumps from a sprint, two rolls each way, two
  sprints stopped each way; [Get-Jumps.py](Get-Jumps.py)): over each jump's window (1.55-2.7 s of its half cycle) the player covers
  5.91-6.05 m at 60 (`i114-oled-jump-n60`), 5.65-5.86 at 120, 5.65-5.82 at 240 and 5.65-5.88 Unlimited (`-f120`, `-f240`, `-fu`):
  3-5 % less above 60, on all eight jumps of each run; the window's fastest row is 7.34-7.55 m/s at every rate (a short boost the
  1/30 s rows sample too coarsely to place the loss). OBSERVED: the jump is short above 60, no correction covers it (the gait
  `1405DC510` and the look-ahead `1409CF010`, never called on the earlier paths, are candidates: not read). The sprint-and-stop
  windows cover 11.33-11.43 m against 11.05-11.19 at 60 (+2 %); the roll windows mix walking and do not separate the roll.
- 120 and 240 are paced by DS3's limiter under V-Sync (the display clock not engaged: their periods are longer than 280 Hz's);
  Unlimited ran at about 200 frames/s, under the display's 280, so the clock engaged but never waited (`stored` 600 of 600).
- The Firelink path Unlimited (`i114-oled-fu`): ends at (269.92, -55.66, 611.55), where the corrected 240 ends (2 m from 60's),
  walk legs 15.62-15.65 m (the walk's +1 %), path 427.7 m.

**The rate and Reflex in the options** (until the menus' redesign, ROADMAP's two sliders): the play launcher's `-FrameRate`
(a whole number 20-1000 or Unlimited, written to `frame-rate.txt` beside the module; 60 removes it) and `-Reflex` (Off, On, Boost,
into `reflex-mode.txt`), each kept in the play folder until changed and ignored with `-Original`; the module reads
`frame-rate.txt` after the test flags (`frame_rate_option.h`, `frame_rate_option_test`), its source named in the `unlock_fix` row.
Reflex Off by default (Thomas, 2026-10-31 22:23: « Non off par défaut, c'est une option et dans les autres jeux c'est aussi sur
off par défaut »). Not run live with the file (the parser's unit test only).

**Step review** (a fresh reviewer, `9c3fd5c7..31e4163f`, against the timing project's 1.15.2 listings of `1405E6580`, `1405CA3F0`
and `1407D7180`; GhidraMCP not in its session): no high. Answered (`f38ae04a`, build `build-32`): M1 when the ground query's bounds
bind, `1405CA3F0` restarts from the bound each call, so its result is already the same at any rate: left as the original's
(branch `bounded`); M3 the fade and ragdoll ramps put back when their step did not run, as the blend's (`PreBiasField`); L1 the
two clamps keep a NaN (their COMISS/CMOVBE pairs), one `ClampHalf`; L2 a call delegated to `1405CA3F0` noted once; L5 the counters
of a reference run not refused by a `frame-rate.txt`; L7 the reader's loop. Answered by reading: M4 (`1409B6D80`'s blend uses the
factor inside the branch that steps it, so an unstepped call does not read the bias). Recorded, open: M2 `1405E6580`'s own
response may be a relaxation rather than a step when `[+0x374]` hands the field to its probes (then `1 − 0.8^k`, about 7 % less at
240; the player's calls are delegated to `1405CA3F0`, whose query does not read the field: `1407D7D00` to read); L3 the query's
first return is the one recorded (a mismatch would show a nested call); L4 the two thread-local scopes outside the affinity
allow-list (pushed and popped on the calling thread); L6 the ramps' callers' registers checked by reading the three entries
(no float input beyond the declared ones).

**Final review** (a fresh reviewer at xhigh, `main...b833d583`, against the timing project's listings and the series' journals): no
high; the code can merge. Answered before the merge: M1 the records (series 25-26 are build-30's, with the doubled note and the
bounded calls corrected; the merged terrain law not run; the field's level 35-65 % above 60's at 240: above, and Uncertainty);
M2 the launcher offers a rate above 60 before its mechanics are all corrected (CONTRACT, safety): it prints what is not yet
corrected, and Thomas is asked; the launcher's edges (`-FrameRate`, `-Reflex` ignored with `-Original`; the unlocked rate's test
flags refused in `-DiagnosticFlags`; an empty file read as the default; `0060` written as `60`); the corrected-call count behind
its flag; `1407D7780`'s comment (it writes no answer); the decay factor `[+0x378]` read after the call, where the original reads
it; a bounded call corrected when its corrected result re-enters the bounds (`TerrainSettleBounded`); the query's pointers
checked; the counters' fault in Uncertainty; `iterations_max` 16 (this tranche's close, WORKFLOW « Investigations »). The answers' step
review (`b833d583..ec76f723`): no high; M1 `original-unlock`, `reflex-on` and `reflex-boost` refused in `-DiagnosticFlags` too; L1
the launcher shows the rate and Reflex as the module reads them; L2 the terrain's level in its warning; L3 noted: since
`TerrainSettleBounded` a bounded call that re-enters the bounds is labelled `response` or `decay` in the 60 rows (`bounded` on
build-30 and before), nothing written at 60.

## Fourth tranche (2026-11-01 night, branch `claude/jolly-mayer-094985`)

Thomas, 2026-11-01 01:23 (relayed): `-FrameRate` stays, with its warning.

**The jump** (series 27's 3-5 % short above 60). Read first (GhidraMCP): the gait `1405DC510` (a virtual: an AI move's class from
a move vector, not the player's) and the look-ahead `1409CF010` (a predicate) are not the jump's. The player's move:
`1405CA570` (PlayerCtrl's step) takes the call's displacement from its virtual +0xD8 (`1405E57F0`, scaled by
`[[owner+0xB8]+0x10]·[manip+0x2D0]` into `[ctrl+0x384]`), keeps `[ctrl+0x260]` = |displacement| / dt (the speed `1405C72B0`'s
distance branch compares with 5 m/s) and multiplies the displacement by the locomotion factor `[ctrl+0x25C]` when it is under 1;
the mover (`1409DC090` → `1409DEE60`) sets its Havok proxy's velocity from the displacement and dt while its ground state
`[mover+0x1DA]` (or `[+0x1DC]`) is set, and with both clear and `[+0x1DB]` (a jump) holds the velocity read back from Havok
(`[+0x120]`, the last step's) with gravity added.
- **Series 28** (01:45-01:51, announced, the ASUS, no measure; build `build-34`: `unlock_player_move` rows; `series-28.log`):
  `i114-jmp-n60`, `-f240`, `-u240` (`no-unlock-fix`). The factor `[+0x25C]` stays 1 and the speed `[+0x260]` peaks at 7.4-7.6
  m/s at both rates; the player's 1/30 s positions hold 7.47 m/s through the air at 60 and 6.86 at 240 (uncorrected 240: 3.45,
  the sprint lost, LTF's defect). OBSERVED: the jump's loss is in the held speed, not in the requested move.
- **Series 29** (02:08-02:17, the ASUS; build `build-35`: `jump-observe.flag`, a row every game step from the first jump;
  `series-29.log`): `i114-jstep-n60`, `-f240`. At the takeoff (`[+0x1DA]` cleared under `[+0x1DB]`) the proxy holds the last
  step's velocity: 7.47 at 60 (the takeoff 117 ms after the jump state's start), 6.79-6.93 at 240 (100-105 ms after). Against
  the jump state's start the root motion's ramp at 60 is about two steps ahead of 240's (5.93 → 7.47 m/s within 83 ms at 60,
  5.51 → 7.26 within 100 ms at 240): the state flags lag the animation by about two steps, 33 ms at 60, 8 ms at 240. INFERRED:
  the original's two-step latency places the hold later on the jump's rising root motion at 60; at 120 it predicts 7.05 m/s, the
  speed series 27 saw.
- **The correction** (`unlock_fix::MoverVelocity` on `1409DEE60`, its one caller `1409DC090`; it returns its second argument in
  RAX, no float, read in its epilogue): from the takeoff, for `logic_rate::TakeoffDelay(dt)` = 2·(1/60 − dt) (none at or under
  60), each call is handed `[+0x1DA]` set (the state the original's step is in at 60 then) and the byte put back after it unless
  the call wrote it; per mover, under a lock; `no-takeoff-fix.flag` the A/B; unit test `logic_rate_test` (the delay at 30, 60,
  120, 240, its whole calls).
- **Series 30** (02:30-02:43, the ASUS; build `build-36`; `series-30.log`): `i114-jfix-f240` holds 7.37-7.45 m/s, `i114-jfix-f120`
  7.44-7.46 (one 7.27), against 7.47 at 60 (one 7.14); aligned on the key press, the 240 run's speed profile (the ramp, the hold,
  the landing) lies on 60's. **Series 31** (03:00-03:09, the OLED 2560x1440 at 280 Hz under V-Sync, Thomas present; build-36;
  `series-31.log`): held 7.27-7.46 at 120, 7.34-7.42 at « 240 » (160 frames/s under V-Sync), 7.36-7.44 Unlimited (167), 7.47 at
  60. OBSERVED: at 120 most jumps within 0.5 % of 7.47 m/s, outliers to 7.27 (2.7 % under, both screens); at 160, 240 and
  Unlimited 7.32-7.45, about 1 % under 60's (0.3-2.0 %); uncorrected 6.84-7.05. INFERRED: the outliers come from the root
  motion's phase at the takeoff, as 60's own 7.14; the 1 % at the high rates is open. The 1/30 s windows of `Get-Jumps.py`
  cannot settle the jump's distance to 2 %: the replay's key times move by ±40 ms between runs (60 against 60 too).
- The sprint and the roll (Thomas's question, 03:00): the sprint's steady speed 5.74-5.77 m/s at 60, 120 and 240, the stop from
  5 m/s 0.71-0.91 m at every rate (series 28 and 30); the « +2 % » of the sprint-and-stop windows is the same replay phase (0.2-0.3
  m at that speed); the roll as series 21.

**The terrain field's level** (the final review's lead): the mover's `[+0x1A0..0x1A8]` read in every move row stays 0 at Firelink
(jumps included) and on Anor Londo's stairs (series 29-30): the lead is refuted. Series 30 (`test-no-ai`, the stairs at 60 and 240
on build-36, the merged terrain law): over the four walking legs mean |x| 0.100 at 60 and 0.121 at 240, its variation 0.47 and 0.36
a second; series 29 (enemies on, Gamma killed on the third climb in both runs): 0.168 and 0.219. The level is 20-30 % higher at 240
within a series, while two 60 runs differ by 70 %: not settled, not corrected (the field's reader is a virtual, its caller not found).

**Read and corrected without a witness** (each at k = 1 the original's call; none witnessed):
- the effect slots `140245F90` (two banks of 8, 15.7 calls a game step at Firelink): its one load of the step (`140246240`, the
  shared 1/30) feeds each slot's age and life (`1402459A0`), its fade-in timers (+the step, the fade their ratio to 0.1), its drift
  (velocity × the step) and a smoothing gain min(step, 0.95). Method D: the load pointed at a module float (`rip_float`, now shared
  with the follow camera's wall snap) set to (1/30)·k each frame; the timers and the drift exact, the gain to first order (1.3 % at
  240); `no-effect-fix.flag` the A/B;
- the task evaluation `140C2A7B0` → `140A4CDF0` (a time object of the shared 1/60, its load at `140C2ACB6`): the same method, 1/60·k;
- the bone constraint `140828FE0` (`[+0x1C]` toward `[+0x18]` by the gain `[+0x28]` a call) and the bone weights `141180110` (a
  bone's weight rising by `[+0x70]`, falling by `[+0x6C]`, its pose blended by `[+0x68]`, each a call): the gain law on each gain
  for the call, the bits put back after it;
- the interface's hold `1406DE400` (`[+0x38]` −1.5 a call while an id is held): pre-biased when the call will take its step, so
  its own −1.5 lands on −1.5·k before the virtual it calls next (the step review's L1);
- the child solver `1401FD120` (its dt kept and accumulated; its two callers in `1400A5AA0` pass 1/30): its dt × k.

**Read, nothing to do or left**: the Havok proxy contexts `{1/60, 1/60}` of `141530300` and `141530D80` are their support checks'
simplex input (`141533750`), Havok's own fixed values for classifying support, not an integration (E). The tab-select UI virtual
`140741A30` hands an `Ins_tab_select` instance 1/30 once a tab change (E, INFERRED from its names). The target timer `140D71920`
(`[+0x240]` −1/30 a call, refreshed when negative, and an N-frame ring average of positions): not corrected, it returns both RAX
and XMM0 (its epilogue) and its object is not identified. The YEBIS time step `1416F27E0`: not corrected, the YEBIS passes are to
be replaced after the path tracing (RT-PT-PLAN, D-0175; Thomas, 2026-11-01 03:02: « On ne gardera pas YEBIS »); their replacement
takes the real dt. Still to read: the light-bound fade pair `1402A5D60`/`1402AF510`, the broadphase margin `14100DDE0`, UNLOCK-READY
§6's ten candidates. A second family is not a constant: the original's two-step latencies (the jump's), handled where a mechanic
shows one.

**Other runs**: the Silver Knight Thomas saw vanish and reappear on Anor Londo's stairs (02:17; seen again by him on 2026-11-02 at about 04:03, Paris, during tranche 8's series 36 on the same
path, build `build-t8a`, at 240; and at 04:14, series 37's first run, at 60 with `test-no-ai`: the Silver Knights gone and back
within half a second, the same as in series 36 with the AI on, Thomas: not tied to `test-no-ai`; at 05:44, series 39 with the enemies frozen,
`i114-t8-top-n60b` at 60, the flesh-mass enemies gone and back within half a second the same way) flickers natively at 60 as at 240
(series 30's `popin-observe`: a knight at 2-4 m between « passed » and « margin »/« frustum » from frame to frame); the original's
run on the same path (`i114-st5-o60`) is for the culling's own thread (I-0100's family), proposed to the coordinator. `i114-st4-n60`
left a fault at the game's exit (a game thread in `141A3B670` after the network stop), not during the path; `i114-st5-n60` failed
its launch check (no presenter rate read) and ran again as `i114-st5-n60b`.

**Step review** (a fresh reviewer, `3c9e132e..f3d828a1`): no high. Answered: M1 the handed calls rounded to the nearest
(`logic_rate::TakeoffHand`: a call handed while at least half of it is owed; a delay of microseconds near 60 hands none; the unit
test with a jittered dt, within half a call); M2 the takeoff table holds only the movers in a jump (a slot freed when its jump
ends, so the player's is never evicted by stale movers); L1 the hold pre-biased instead of corrected after the call (its virtual
at `[this+0x20]`+0xB8 may read it); L2 under 60 not corrected, now in Uncertainty; L3 the stale plan and Uncertainty rows; L4
`status.json`'s encoding, `SetStepLoads`' needless branch, the redundant takeoff flag.

**Series 32** (03:35-03:41, the ASUS, no measure; build `build-39` = the answered tree; `series-32.log`): two smoke runs at 240,
clean, every correction installed (`unlock_fix` names `takeoff`, `effects`, `task_step`; the wall snap patched beside them):
`i114-smoke-jump-f240` holds 7.32-7.44 m/s in its jumps; `i114-smoke-fl-f240` walks the Firelink path as the corrected runs
(427.7 m, legs 15.61-15.65 m, sprints 20.5-23.4 m). The effect and task steps ran in them (the effect slots are called 15.7
times a game step at Firelink), their effect not observed; the bone, hold and child-solver detours were installed, not called.

**Final review** (a fresh reviewer at xhigh, `main...6fa2f589`): no high; the code can merge. Answered before the merge: M1 the
jump's claim narrowed (at 120 most jumps within 0.5 %, outliers to 2.7 %; above, about 1 % under 60's: the answers' step
review's M-a); L1-L2 the
hold's and the smoke runs' records; L3 the launcher's warning (the jump corrected above 60 only); L4 a takeoff slot unseen for a
second reused (a mover destroyed in a jump); L5 the patch's timing assumption written at it. Recorded, open: M2 the method-D
steps and the child solver's dt·k assume one call a game step (counted at 60 only: the counters are refused with an unlocked
rate), the bone, hold and child-solver detours never called on the paths run, and the start check runs at 60, so Thomas's play at
120 is their first exercise; L6 the child solver's caller also decays type-3 entries' `[+0x6C]` by `[+0xAC]` a call (not read);
L7 a fall off a ledge holds Havok's velocity too (`[+0xD0]` set, `[+0xA2]` clear) with, INFERRED, the same latency: only jumps are
corrected; L8 the task step has no A/B flag. The answers' step review (`6fa2f589..2c45a0e5`): no high; M-a the jump's claim
narrowed again (above); L-a an empty takeoff slot preferred to a stale one; L-b, L-c the records and the patch's comment; L-d (a
takeoff during frames at exactly k = 1 at an unlocked rate leaves a stale ground state) recorded.

**Merged** `61ec57c7` (2026-11-01 04:05), the launcher rebuilt from it (its unit tests pass), start check `start-61ec57c7`
passed (04:16, the ASUS).

## Fifth tranche (2026-11-01 night, branch `claude/i0114-tranche5`, the same thread: Thomas asleep, no fresh thread before his next message)

Read (GhidraMCP), each with its verdict:
- the light-bound fade pair `1402A5D60` / `1402AF510` (identical clones, a view's light gathering): `[+0x2860]` ×0.55 a call while
  the box of the lights' motion over `[+0x3B30]` exceeds `[+0x285C]`, then −`[+0x3B30]` a call while its flag is set (under 0, the
  flag `[+0x3963]`). INFERRED `[+0x3B30]` a dt (its writer not found): the decrement is in time, the motion box a displacement over
  that dt (so a light must move k-times faster to trip it at 240, as the per-frame distance law would say), the ×0.55 per call;
  not corrected until `[+0x3B30]` is read in a run;
- the broadphase margin `14100DDE0`: an adaptive factor (×1.05 capped at 1 / ×0.953 by a count's surplus or deficit), INFERRED the
  same equilibrium at any rate (if the count it balances does not itself depend on the rate: not read), only its convergence a
  call: nothing done;
- UNLOCK-READY §6's ten candidates (a Sonnet `ghidra-research` triage with GhidraMCP; `1408651E0` checked by me, `1405C7DB0`,
  `1405D1B70`, `1405EA6A0` by the final reviewer's decompiles; the others the triage's only):
  `1408651E0` (INFERRED the lock-on target manager `[0x14477DBE0]`: it walks the targetable characters against `[+0x2920] +
  [+0x2914]` and writes the target's position `[ChrIns+0x1080..0x108C]`; two callers, `1408DF7B0` and a tail jump in `1408DFD60`,
  both passing the frame's dt in XMM1 and an object in R8): three ranges (INFERRED: `[+0x2920]` an acquisition range, `[+0x2924]`,
  `[+0x2928]` lock-loss ranges by target type) `[+0x2920..0x2928]` lerped toward a parameter row's by `[+0x2958]` = 0.1 (its one writer
  `1408637A0`) a call, its timers by dt; its one 60.0 (`140865D8E`) is an angle handed to `141777FB0` with another (a cone, not a
  rate): **corrected**, the gain law on `[+0x2958]` for the call. Its effect shows only while a range moves (a parameter row's
  change), which no path here makes: a witness needs a lock-on range transition at 60 and 240. `1405C9DC0` (a character's movement): a residual
  displacement `[+0x200]` spread over n calls (`[+0x230]` decremented a call, the total conserved, the spread n frames long:
  4 times shorter in time at 240): left, n's writer not found. `140F1DF00` (an IK or aim angle `+0x70`): a gain a call with a 0.5°
  minimum, scaled by min(1, `[*param_2+0xD8]`·30) when the rate is under 0.99, INFERRED dt-aware if that field is the frame's dt:
  left. `1405C2CD0` (a step-height follow: a speed from a per-call displacement over `[+0x30]`): unclear, left. The rest from the
  triage, not checked by the pilot (INFERRED): `1405EA6A0` (a yaw turned by at most a cap a call, the cap from a float argument
  likely the dt) dt-aware; `1405CB200`, `1405C75F0`, `1405C7DB0` dt-aware (their dt passed down); `1405D1B70` no recurrence;
  `1406358D0` a one-shot item-parameter summary (60 a seconds-to-minutes unit). §6's eleventh, `1405E57F0`, is PlayerCtrl's
  virtual +0xD8 read in the fourth tranche (the displacement's scale `[+0x384]`, rewritten each call: no recurrence).
- The merge of `main` (I-0113 tranche 10a) into this branch committed STATE with its conflict markers (`97940cd2`, a resolver
  that missed the file's CRLF ends); resolved in the next commit (`f31fa11f`, the open-gaps pilot's rows from `main`).
- Step review (`main...f31fa11f`): no high; answered M1 (STATE and the Conclusion), L1 (the triage's verdicts marked INFERRED,
  the broadphase's assumption), L2 (`1405E57F0`), L3 (the site named the lock-on target manager, its witness), L4 (the 60.0),
  L5 (the gain read after the k test). Final review (xhigh, `main...2880ab83`): no high, the code can merge; answered M1 (the
  walk's and strafe's residual not settled, with its magnitudes, back in Next), L1 (two callers), L2-L4 (counts, Uncertainty,
  which verdicts were checked), L5 (the launcher's warning), L6 (the names marked INFERRED), L7 (the index row).
- **Series 33** (04:57-05:02, the ASUS, no measure; build `build-44` = the reviewed tree; `series-33.log`): two smoke runs at 240,
  clean, every correction installed: `i114-smoke5-lock-f240` (Beta, the lock path) strafes 5.77-5.82 m with the camera's turns as
  series 25-26 (the lock-on ranges' detour installed; their effect not observed), `i114-smoke5-fl-f240` the Firelink path as the
  corrected runs (426.9 m, legs 15.59-15.64 m).

**Merged** `6d4df5d5` (2026-11-01 05:22), the launcher rebuilt from it (its unit tests pass), start check `start-6d4df5d5`
passed (05:34, the ASUS).
- The walk's +1 % and the lock-on strafe's +2-3 % at 240: not settled. A key read up to a frame later at 60 moves each end of a
  leg by at most about 12.5 ms, where the excess needs 30-45 ms of motion (0.16 m at 3.9 m/s for the walk, 0.11-0.17 m at 3.7 m/s
  for the strafes); a start lagging about three more logic steps than the stop would fit, a lag counted in steps like the jump's
  (the « second family »). The replay's ±40 ms phase keeps these paths from settling it: the start and stop lags are to be read
  from per-step rows.
  **Read from per-step rows** (2026-11-01 05:45, no new run: the `unlock_jump_step` rows of series 29-32 cover the jumps path's
  first roll leg, Z held 2.0 s with a roll inside; the motion's duration from the first step faster than 0.5 m/s to the stop
  needs no alignment with the keys): 1.950 s at 60 (two runs), 1.975 at 120 (two), 1.979-1.984 at 160-240 and Unlimited (five).
  OBSERVED: a held move lasts 25-33 ms longer above 60, the difference of about three steps of each rate between the start's
  lag and the stop's (50 ms at 60, 25 at 120, 17 at 240): a latency counted in steps, the walk's +1 % (0.13 m at 3.9 m/s).
  Not corrected: unlike the jump, where the latency changed the held speed, this one changes only when a move starts against
  its key, the responsiveness an unlocked rate is for; whether to give a held move the original's 60 frames/s duration is
  Thomas's call: kept as it is (Thomas, 2026-11-01 07:06: « Le jeu est plus réactif à haut frame rate c'est ça? Oui c'est le
  but »); confirmed 07:13 after the point was put to him as it is (the start reacts 3 steps after the key, so sooner above
  60; giving a held move 60's duration would mean 60's 50 ms at the start at every rate): « A alors. Mais faut pas que ça fasse
  tricher ni décale le personnage sur une même distance parcourue »: the speed stays 60's (series 4), and the next tranche checks
  that a roll and a sidestep cover 60's distance (not yet measured).
- **Sixth tranche: the run-time reads** (branch `claude/i0114-tranche6`; probes in `008bd1b3`, not merged: diagnostic code that
  answered its question). **Series 34** (05:58-06:02, the ASUS, no measure; build `build-45`; `series-34.log`), the jumps path at
  60 (`i114-read-n60`) and 240 (`i114-read-f240`):
  - the light-bound fade pair (`light-bound-observe.flag`, 25,200 and 78,000 calls): the view's `[+0x3B30]` equals the logic's
    dt in every row (0.016667 at 60, 0.0054-0.0066 at 240): OBSERVED a dt. Its bound `[+0x285C]` is 0 and its flag 0 throughout,
    so the ×0.55 branch (which needs a bound above 2^-23) never runs and the fade stays 1.333: inert at Firelink. The decrement
    is in time; where a view sets a bound, the motion box (the lights' velocity × dt) compares a distance a frame with it, the
    per-frame distance law's case: left, no view seen with a bound;
  - PlayerCtrl's `[+0x230]` (INFERRED `1405C9DC0`'s count) stays −1 in all 1,800 and 5,493 per-step rows (jumps, rolls, walks):
    no spread on this path; the triage's reading not witnessed, left open.

## Seventh tranche (2026-11-01 morning, branch `claude/zealous-antonelli-08ec08`)

Thomas, 2026-11-01 07:06-07:16 (relayed by the coordinator at 10:14): option A (the held moves' sooner start above 60 kept, its
+1-3 % on a held move accepted), but no cheating and no shift of the character over the same distance; one logic for every
rate, under 30 too.

**The jump's law, read again from the per-step rows** (no new run; series 29-31's `unlock_jump_step` rows, the uncorrected
`i114-jstep-f240` and `i114-oled-jump-*` and the corrected `i114-jfix-*`, `i114-oled2-jump-*`):
- the jump state (`[+0x1DB]` and `[+0x1DA]` set together) holds `[+0x1DA]` 116.7 ms at 60 (7 steps; once 100 ms), 108.3 ms at
  120 (twice 100), 99.4-104.7 ms at 240: the takeoff is the first step past about 100 ms of the state, at 60 one step past it;
- the requested root-motion speed `[ctrl+0x260]` against the state's start, fitted to 60's over 40-110 ms (least squares on a
  0.1 ms grid), lags 60's by 24.3-24.5 ms at 120 on all eight jumps and 29-35 ms at 240 and Unlimited (dt 5.6-5.9 ms): 3·(1/60 − dt)
  (25.0 and 32.5-33.3 ms). OBSERVED: the fitted lag; INFERRED: the animation leads the state flags by three steps of each rate (the fit
  here, read with [Get-JumpShift.py](Get-JumpShift.py); the simulation below, with [Get-TakeoffSim.py](Get-TakeoffSim.py));
- the speed the original holds is the last grounded step's (the read-back `[+0x120]`), so the held speed a delay of n·(1/60 − dt)
  would give is the requested speed of the last handed step: simulated on the six runs' rows (`TakeoffHand`'s rounding), n = 2
  (tranche 4's law) gives means 7.36-7.42 m/s at 240 and Unlimited and 7.38-7.42 at 120 (the measured 7.37-7.45 of series 30-31:
  the model holds), n = 3 7.35-7.39, n = 3.5 7.39-7.45, **n = 4 7.47-7.51 at 240 and Unlimited and 7.43-7.45 at 120** (single jumps
  7.36-7.54), n = 4.5 7.50-7.54; 60 holds 7.47 (one jump 7.14). The 1 % under 60's above 120 was the law's n = 2.
- **The law** (`logic_rate::TakeoffDelay`): 4·(1/60 − dt) from the takeoff, the calls inside it handed the ground state as before;
  nothing at or under 60 (below). Unit tests: the delay at 20, 30, 60, 120, 240.

**The target timer** (`140D71920`, GhidraMCP, read whole with `140D73250`): a virtual (no direct caller; its vtable's slot 9 at `14295EBB8`) of
`NS_SPRJ::SprjHkAiCharacter`, the enemies' Havok AI character (its vtable `14295EBB8`, logged by the detour in series 35 at the
Undead Settlement and at Anor Londo, its RTTI read with GhidraMCP), returning a target's position (`[this+0x218]`, its +0x20; its
velocity +0x40) plus an offset (INFERRED its steering goal). Two per-call laws without dt: the offset's timer `[this+0x240]` −1/30 a
call (`[0x143D7ED50]`; under 0 a new offset from `140D71DE0`, in state 2 the offset kept until then) and a ring of the last N
offsets summed and normalised (`140D73250`; N `[this+0x238]` set each call from `[0x144549970]`, 20 in the image: N − 1 = 19 samples, 317 ms at 60 and 79 at 240
; 16-byte samples from `[this+0x30]`, 30 at most before `[this+0x218]`; start `+0x230`, next `+0x234`). It returns its result's pointer in RAX
(its XMM0 a leftover of its last store). Corrected (`unlock_fix::TargetTimer`): the timer pre-biased as the hold (`PreBias`, the bits
put back when the call did not step it); the ring held to one sample a 1/60 s per object (`logic_rate::SixtiethTicks`,
`RingByTicks`: the newest sample is replaced until a call completes its tick, a call completing several first repeats it), at k = 1
untouched. Unit tests: the ticks at 20, 30, 60, 240 and an irregular dt; the ring's window at each completed tick equal to 60's at
240, at an irregular dt and at 30. Its calls are counted in the per-step rows (`tt`): 2,240 in the lock path's 120 s at 240 with
`test-no-ai` (cumulative at the last row: about 16 s of rows, 380 to 2,240), 33,790 on the stairs at 240 with the enemies on; `no-target-timer-fix.flag` the A/B; its calls follow the game steps (series 36, below). Its effect (an enemy's steering
smoothed over 79 ms instead of 317 at 240) is not seen in a run: the enemies stood still or were not watched.

**The ledge fall** (the fourth final review's L7, read): `1409DEE60`'s held branch also serves a fall (`[+0xD0]` set and `[+0xA2]`
clear, `[+0x1DB]` clear): the read-back velocity held with gravity added by dt. Unlike the jump, nothing in it is keyed to the
animation: a walk or a run off a ledge holds its steady speed, the same at every rate; only a fall starting inside a speed ramp (a
roll) would take a step's sample of it. INFERRED, not witnessed: series 35's drop at the bonfire (`witness-teleport.txt` written by
the series) never ran, the run's start removes a stale teleport file before the game starts (« removed stale witness-teleport.txt »).

**Series 35** (2026-11-01 10:54-11:21, announced, the OLED 2560x1440 at 280 Hz under V-Sync, offline, no measure; build `build-46`;
`series-35.ps1`, `series-35.log`; 12 runs, all clean, no fault). « 240 » ran at about 164 frames/s (the median step 6.1 ms), 120 at
120, 30 and 20 at their rates (`frame-rate.txt` 20: the logic's dt 0.050, the logic in real time, not slowed).
- The jump above 60 (`i114-t7-jump-f240`, `-f120`): held 7.44-7.52 m/s at « 240 » (median 7.48) and 7.46-7.47 at 120 (one 7.36),
  60 7.47-7.48 (`i114-t7-jump-n60`). OBSERVED: the held speed is 60's within ±0.7 %, one jump at 120 1.5 % under (7.36, after a 12.9 ms hitch) (tranche 4: about 1 % under at 160-240).
- The jump under 60: the original (`-u30`, `-u20`: `no-takeoff-fix`) holds 7.56 at 30 (+1.2 %) and 7.67 at 20 (+2.7 %), on every jump but the seventh at 20
  (2.79 m/s in the original, 0.91 with the first version: the path's same jump at both, a replay phase). The first version of this tranche gave the takeoff under 60 the velocity of |4·(1/60 − dt)| earlier from the mover's history
  (`-f30`, `-f20`): 6.84-6.91 at 30 (−8 %) and 7.12 at 20 (−4.7 %), further from 60's: removed (`6c201859`). At coarse steps the root
  motion's per-step samples are not 60's shifted in time (the requested speed at 20 reaches 7.67, above any 60 sample, 7.57): the
  back-shift that would give 7.47 is 38 ms at 30 and 18 ms at 20, no single law. The hold also lasts to the first step past the
  landing: 217 ms from the takeoff at 60, 206-208 above, 233 at 30, 250 at 20 (13, 7 and 5 steps). OBSERVED: under 60 the jump
  stays the original's, about 1-3 % long in its held speed; not corrected (open).
- The roll (`Get-Moves.py`, the jumps path's rolls taken in full, about 500 ms between the 4.6 m/s crossings interpolated between
  the steps' middles, the step review's L1): 3.589 m at 60, 3.580 at 120, 3.567-3.573 at « 240 », 3.603-3.604 at 30, 3.652-3.657
  at 20 (`i114-t7-mech-f20`'s six full rolls the same), peaks 11.28-11.30 m/s (10.33 at 20, its 50 ms steps). OBSERVED: a roll covers
  60's distance within 0.6 % from 30 to 240; +1.9 % at 20, where a 50 ms step blurs the crossings (the reader's resolution, not
  separated from the game's).
- The strafe in lock-on (`i114-t7-lock-n60`, `-f240`, `test-no-ai`): the speed is the same at both rates, 3.931-3.932 m/s on
  every strafe; each strafe lasts 1,467 ms at 60 and 1,484-1,489 ms at 240 (+17-22 ms) and covers 5.64-5.76 m against 5.60-5.85 m
  (the first strafe shorter at 240, the three others +1.2-1.6 %). OBSERVED: no faster move over the same time; the extra distance is
  the sooner start (option A), the same as the walk's (Thomas's choice, D-0244).
- The sprint at 20 (`-mech-f20`): 5.744-5.747 m/s (60: 5.74-5.77).
- The stairs (`i114-t7-st-n60`, `-f240`, the enemies on; `Get-Stairs.py` and the three first climbing legs, 28 s): the legs the
  same (31.12-31.36 m, 3.88-3.92 m/s), the vertical jitter while moving 0.0096 m at 60 and 0.0057 at « 240 » over the whole path;
  the terrain field's mean |x| over the three legs ([Get-StairsLegs.py](Get-StairsLegs.py)) 0.106 at 60 and 0.124 at 240 (+18 %; series 30 +20 %, build-30's series 25-26
  +35-65 %), over the whole path 0.095 and 0.081 (the paths part after the third leg); its variation a second 0.375 and 0.323.
  OBSERVED: on the comparable legs the field's level is again higher at 240, while the motion is not hoppier: open (the field's
  reader, a virtual, not found; what the level changes is not known).

**Step review** (a fresh reviewer, `831e44c3..c0a3ebf0`): no high. M1, the target timer's premise (its calls follow the frames) had
no count at 60: answered by counting it at 60 too (its detour alone, the original's call, with `jump-observe.flag`; the count and
the vtable log now only with that flag, L4) and series 36. L1 `Get-Moves.py`'s rolls cut at whole edge steps: now between the 4.6 m/s
crossings interpolated (the roll figures above). L2 the records (the strafe at 60 and 240 only; the lock path's count cumulative).
L3 a test of the ring after a hitch (a call completing three ticks after calls that completed none). L5 the counter's comment moved.
Build `build-48` (its unit tests pass).

**Series 36** (12:07-12:12, announced, the OLED, V-Sync, no measure; `build-48`; `series-36.log`; its `series-36.ps1` overwritten in the eighth tranche): the stairs path
with the enemies on at 60 and « 240 » (`i114-t7-tt-n60`, `-f240`; Gamma killed on the climbs, as in series 29: the first 30 s
used). The target timer's calls over the same 30 s from the first motion: 3,332 at 60 (111 a second, 1.85 a game step) and 10,542 at
« 240 » (351 a second at about 200 frames/s, 1.77 a step), per 10 s 1.63-2.04 and 1.61-1.99 a step. OBSERVED: its calls follow the
game steps, so a per-call constant runs faster with the rate and the correction holds (M1 answered). Next runs with the enemies on:
`test-tools.flag` and F4 at the path's start (Thomas, 2026-11-01 11:10: the life refilled, the AI unchanged; LIVE-RUN-NOTES).
**Final review** (a fresh reviewer at xhigh, `main...5bc09a79`, the target timer and its helper disassembled read-only): no high;
the code can merge. Answered before the merge: M1 the summaries' figures and inferences (the 120 jump's 1.5 % outlier, the 20 frames/s
roll's blur not separated, the steering and the three-step lag marked INFERRED, the seventh jump at 20, the stale plan row; the
fit, the simulation and the three legs' readers committed: [Get-JumpShift.py](Get-JumpShift.py), [Get-TakeoffSim.py](Get-TakeoffSim.py),
[Get-StairsLegs.py](Get-StairsLegs.py)); L1 the ring's bound 30 (its slots end at the target pointer `[+0x218]`); L2 a call left
to the original ends its object's tick in progress (`Forget`); L3 the comments (the cut sentence, the A/B's, « summed », 317 ms);
L4 the ticks checked call by call at 20, 30 and 240; L5 the timestamps; L6 the jump row's buffer 512. Build `build-49` (the final
tree, its unit tests pass). The answers' step review (`5bc09a79..bd39b36c`): no high; answered M1 (a cut
sentence and two stale figures), the summary's wording, `Forget` only at k = 1 (a call with no target pushes no sample, the final
review's disassembly), the readers' encoding, labels and a dead line, the jump row written truncated (`_snprintf_s`); `build-50`.

**Merged** `5ae7905c` (2026-11-01 13:10), the launcher rebuilt from it (its unit tests pass), start check `start-5ae7905c` passed
(13:24, the ASUS).

**Readers**: `jump-observe.flag` now opens its per-step rows at the first motion for 120 s (before: 30 s from the first jump), so a
whole path's rolls and strafes are read per step; [Get-Moves.py](Get-Moves.py) (rolls, held moves and falls from those rows).

## Eighth tranche (2026-11-02 night, branch `claude/i0114-terrain-feedback`)

Thomas, 2026-11-02 03:13 (relayed): « Oui je voulais corriger le +18 % sans mettre à jour Havok ».

**A first fix, withdrawn.** With the mode byte `[ctrl+0x374]` set, `1405E6580` hands the field `[ctrl+0x2DC]` to the foot probes
`1407D7180` / `1407D7780` as their ninth argument, into the ground height `1407D7D00` computes for the foot; their answer is
`solver + their eighth argument` (the mover's `+0x1A4`) (VERIFIED, static: GhidraMCP). INFERRED from it: no closed law scales that
branch's step by k, so a tick was written (`build-t8a`: the original's own step once a 1/60 s of game time above 60). The step review
(below) found it is not on the player's path: every `unlock_terrain` row of every stairs run (series 35 to 39) is site `ca3f0`, the
player's calls delegated to `1405CA3F0` (as the third tranche's step review M2 had recorded), whose ground query `1409A21E0` does not
read the field (the rows note the last call's site one call in 15, so « never » rests on that sample and on the delegation read in
the code). Series 36 to 38 ran `build-t8a`, where the tick is present but outside the player's path; it is removed since `build-t8b`
(main's terrain laws), which series 39 ran.

**Series 36** (03:56-04:05, the OLED, `build-t8a`, the AI on; `series-36-t8.ps1`): Gamma died three times and the series was stopped
in its fourth run (the save restored by hash on all four). Its script replaced the seventh tranche's `series-36.ps1` of the same name
(that script is lost; the shared `series-36.log` holds both series one after the other). **Series 37** (04:13-04:23, `test-no-ai`,
`i114-t8-na-*`) and **series 38** (04:43-05:04, `test-no-ai`, four runs a rate, `i114-t8-l3-*`; `f240c` left the known exit fault in
`141A3B670` after its path): the OLED 2560x1440 at 280 Hz, V-Sync, offline, the PC otherwise idle (series 38's first loading
overlapped the end of another pilot's build), every save restored by hash.

**The level, leg by leg** ([Get-StairsLines.py](Get-StairsLines.py): the three walking legs whole, 0-8.3, 10-18.3 and 20-28.3 s from
the first leg's start that `Get-Stairs.py` prints; 8 runs at 60 and 8 at « 240 » over series 30, 35, 37 and 38; series 36 left out,
its third legs cut short by the attacks, `i114-t8-st-n60a` ending at x 550.8 and `-f240a` at 557.5 against 557.4-559.3 for the runs
kept; the rows every 15 calls of the reader in series 37-39, every 3 in series 30 and 35, a time-uniform sample at both):
- legs 1 (up, from the standstill at the bonfire) and 2 (down): mean |x| 0.139 and 0.069 at 60, 0.140 and 0.071 at 240, the runs
  within 0.010 of those (OBSERVED: the same level at both rates);
- leg 3 (up again, after the turn at the bottom): 0.116-0.177 at 60 (mean 0.138), 0.137-0.181 at 240 (mean 0.161): the whole of the
  « +18 % ». The level follows the line walked: the runs ending leg 3 at z ≥ −1083.24 give 0.116-0.146 at either rate (two at 240,
  0.137 and 0.138), those ending at z ≤ −1083.44 give 0.152-0.181 (one at 60, 0.177); the 240 runs end there 6 times in 8, the 60 runs
  once. The heading after the turn: +2.0° to +4.0° at 60, −2.4° to +3.1° at 240; the camera's yaw at the leg's start 86.5-88.1° at
  60, 87.4-92.1° at 240 (`i114-t8-l3-f240b` 2.8 m off the reference line: the run Thomas saw off the stairs).
OBSERVED: at a given place on the stairs the field's level is the same at 60 and 240; series 35's difference is all in leg 3, whose
240 run ended at z −1083.99 against −1083.14, and the 240 runs walk leg 3 on another line more often. INFERRED: the « +18 % » is
that line, not the terrain law; whether the enemies, which still walk under `test-no-ai` (Thomas, 04:49), or the turn at 240 made
it: series 39 below.

**The enemies frozen** (Thomas, 04:59: « le mieux serait de complètement figer l'IA ennemie »): `test-freeze-ai.flag` sets, besides
`test-no-ai`'s byte, DS3's own debug bytes `14477FEA0` and `14477FEA1` (VERIFIED, static). `14477FEA0` is read in `1405CAB20` (the
move zeroed, as a character's bit 0x80 of `+0x1EE8` does), `1405DF000`, `1405E08F0` and the mover at `1409A8428`; each exempts the
local player (`14088CFF0`: `[WorldChrMan+0x80] == chr`), `1405CAB20` and `1409A8428` also a character for which `14088D1C0` holds
(INFERRED another player-type character, a phantom), and `1405CAB20` still adds its virtual +0x90's vector after the zeroing (the
final review's read). Under `14477FEA1`, `1405C5E70` answers false and the AI controller's step `1405CEA80` is skipped. A grab still
happened (series 39: a leech enemy at the top caught Gamma). Thomas's path to the last step: `anorlondo-stairs-top` (Z held from
the bonfire; 30 s in series 39, 20 s since, Thomas 05:43: the 20 s file not run yet).

**Series 39** (05:38-06:05, announced, the OLED 2560x1440 at 280 Hz, V-Sync, offline, the PC otherwise idle; `build-t8b`, main's
terrain laws and `test-freeze-ai`, its `test_no_ai` row `"freeze":1`; `series-39.ps1`, `series-39.log`; the twelve saves restored by
hash, `baseline.json` measure true):
- the climb to the last step (`i114-t8-top-*`, four runs a rate, [Get-TopClimb.py](Get-TopClimb.py), 0-19 s from the first motion:
  the whole rise, 19.19 m in every run; the main flight 5-13 s, a landing, a short flight to the last step at about 18.5 s): mean |x|
  **0.164, 0.164, 0.162, 0.171 at 60 and 0.167, 0.170, 0.168, 0.169 at 240** (means 0.165 and 0.168, +1.8 %, the 240 runs inside the
  60 runs' range); over the main flight 0.294, 0.293, 0.294, 0.313 and 0.295, 0.305, 0.297, 0.297; every run on the same line (mean
  distance to the first run's 0.01-0.14 m). Means compared, not the CONTRACT's curve against time (UNLOCK-READY §5's 2 %).
- the round trip (`i114-t8-fz-*`, two runs a rate): the heading after the turn +3.2° and +4.0° at 60, +2.9° and +3.2° at 240; leg 3
  ending at z −1082.86 to −1083.28 in all four (near the 60 runs' line); leg 3's mean |x| 0.134 and 0.140 at 60, 0.142 and 0.128 at
  240; legs 1 and 2 as before; the camera's yaw at leg 3's start 86.8° and 87.9° at 60, 92.1° and 87.1° at 240.
OBSERVED: with the enemies frozen, the terrain field's level is the same at 60 and 240 over the whole climb (the climb from a
standstill, the condition legs 1 and 2 already showed equal), and the four round trips end leg 3 near the 60 runs' line. Not settled:
what put series 35's 240 runs on another line. At 240, 6 of 8 runs left the line with the enemies moving and 0 of 2 frozen, but 2 of
7 `test-no-ai` runs kept it too, so two frozen runs do not separate the enemies from chance; and the camera's yaw after the walk down
still reaches 92° at 240 with the enemies frozen (`-fz-f240a`, whose heading after the turn stayed +2.9°). Nothing to correct in the
terrain law; Havok untouched. Open, for the camera's own laws (« Uncertainty »): the yaw after a walk down at 240 and the line after a turn.

**Step reviews** (fresh reviewers): `722e5c75..d4fc35c2`: H1 the tick not on the player's path (every row `ca3f0`): the tick
removed and its « corrected » claims withdrawn; M1 the measurement worded beyond its support, M2 an inference marked OBSERVED, L1-L5
(the reset, a hitch, the map, the tests, a percentage): moot with the tick removed. `722e5c75..5203818c`: no code finding (the tool
inert without its flag, the local player not frozen, the play launcher refuses `test-*` names); H1 the line's cause and « not a rate
defect » beyond the data: the cause left open with the camera's yaw at 240; M1 the study's decision still read as a fix: reworded;
M2 the freeze's limit (a grab under it): written in LIVE-RUN-NOTES and the code; M3 / L2 series 36 left out: written with its two
counter-cases; L1 `-fz-n60b`'s leg 3 (the window's edge: now whole legs) and « near » the line; L3 the 20 s path not yet run.

**Final review** (a fresh reviewer at xhigh, `main...3ea6ad5d`, the figures recomputed from the runs and DS3's bytes read): no high;
the branch can merge after M1. Answered: M1 the play launcher's message still called the stairs' terrain response wrong at 240:
replaced by the open point; M2 the summaries' cause of the « +18 % » marked INFERRED, the observed part given; L1 the study's §1
claims marked superseded, `14088CFF0` named; L2 the freeze's readers, its second exemption and the residual vector written; L3 the
legs' windows whole (`Get-StairsLines.py`, `Get-TopClimb.py` committed; leg 3 0.138 and 0.161, the bins 0.116-0.146 and 0.152-0.181);
L4 the slips (2 of 7, the ends, 0.137, series 36's criterion its cut legs); L5 the builds' provenance and the sampled « never »;
L6 the 20 s path not run; L7 the stale references and `status.json`; L8 « may push »; L9 means, not the curve.

**Merged** `00b2012a` (2026-11-02 06:38), the launcher rebuilt from it (its unit tests pass), start check `start-00b2012a63`
passed (07:07, the ASUS; the first try `start-00b2012a` stayed at the title without the network notice: a lost key, no crash, the
save restored).

**Other observation**: the Silver Knights' flicker seen again by Thomas in series 36, 37 and 39, and the flesh masses' (below,
« Other runs » of the fourth tranche; the culling's own thread).

## Final review of the first merge (fresh reviewer at xhigh, main...branch, 2026-10-31)

The default path is clean (no flag: the module behaves as main). Answered before the merge: H1, the camera flipping between a
« sixty » branch and the blend when dt crossed 1/60 (a judder and lost camera time under V-Sync or a GPU-bound ~60: once a state
is kept every dt steps the ticks, alpha clamped); M1, `unlock-observe` with `locomotion-observe` at 60 aborting setup on the turn
entry (now a named `player_turn_refused`); M4, the tracker's gain law outside [0, 1] (the original's call there); the stale
records (status, index, tranche 1's header, the camera flag). Open, for the next tranches (not blocking a merge behind flags):
M2 the camera's fixed step as method A for that one system: Thomas chose method B (D-0243), the fixed step back behind its flag; M3 the turning and tracker corrections without a
step review or a measured effect; M5 one-frame camera inputs (lock-on, recentre, target switch) at 240 to test; M6 a V-Sync run
with the display clock and the corrections; L1 the blend's cuts (snap on an external write to the outputs), L2 the latency at 2
on a screen that does not fit, L3-L5 refusal reasons, dead paths and leftover test hooks (`no-wall-recovery`, `view-delay-<n>`,
`camera-step-4`), L4 the detours' argument types against the disassembly, L6 the pacing row's `unpaced`, L7 the Evidence and
Uncertainty sections, L8 the corrections' threading assumption, L9 the test rows' fixed buffers.
Since (second tranche): M2 answered by method B (the camera's laws, D-0243); M3 closed by the step review of the camera laws;
L5's test hooks `view-delay-<n>` and `camera-step-4` removed; M5 answered (the recentre, series 22; lock-on with a target and
the target switch, series 25-26); M6 answered on the ASUS (series 25: the display clock with 120, 240 and Unlimited).

## Final review of the second tranche (fresh reviewer at xhigh, main...branch, 2026-10-31 21:40)

No high. Answered before the merge: M1 Reflex's failures now latch (`reflex_given_up`: no Reflex on the adapter once PCL answered,
its functions unanswered after 600 tries, or its options refused), so BeginFrame no longer takes Streamline's lock each frame;
M2 the driver's low-latency flag read once when the options are accepted, not each frame; M3 the recentre's claim narrowed to 240
(the only rate run besides 60); L1 an invalid exposure replaced by 1 (the exposure stays given); L2 the patched snap checked every
3600 calls (`camera_wall_snap_reverted`); L4 the unused field, `reflex_state` cleared at shutdown, the header's comment; L5 this
RESULT (the snap's sentence, the reader of series 21's figures: `Get-Mechanics.py`'s « events » line, Evidence and Uncertainty).
Recorded, not changed: L3 the wall state kept for one camera object (one is seen in play); L6 Reflex on drops the display clock's
deadline rule while the logic is locked at 60 (CADENCE-REFLEX-READY §5.5 kept it: a deviation of this tranche, Reflex then the
only frame-start wait, to measure with the latency tools), and PCL's simulation start sent before DS3's limiter (its wait counted
as simulation at a locked 60); L7 the exposure texture outside the frame graph (one 1x1 copy and its two barriers in one place, the
graph's shadow counts it as untracked without a mismatch: `graph_barrier_mismatch` 0 in `i114-reflex-n60` and `i114-mech2-n60`).
`no-wall-recovery.flag` stays (the wall law's A/B control); the first final review's L5 is answered for the other two hooks.
The answers' step review (`cfc381c8..e32c4c34`): no high; M-a the « events » line's claim narrowed (above, Evidence); L-b the
low-latency read in its own try; L-c a refused options call latches only once PCL answered; L-d the 600-try reason named; L-a (a
later loss of low latency not reported) and L-e (the reader's windows tied to this path) recorded.

## Evidence

- `evidence/I-0114/artifacts/build-1` to `build-32` (`build-27` a build whose sources changed while it compiled: unused; the merged
  tree's last answers after `build-32` were checked with clangd and are built by the launcher's rebuild),
  `series-1.log` to `series-26.log`; the runs under `evidence/I-0066/artifacts/i114-*` (series 20: `i114-laws-*`; 21: `i114-mech-*`,
  `i114-snap-f240`; 22: `i114-mech2-*`, `i114-vsync-f120`, `i114-ab-speed-f240`, `i114-reflex-*`; 23: `i114-st-*`, `i114-dc-*`
  with their crash reports; 24: `i114-bis-*`; 25: `i114-st2-*`, `i114-dc2-*`, `i114-lock-*`; 26: `i114-st3-*`, `i114-lock2-*`; 27: `i114-oled-*`;
  fourth tranche, builds `build-34` to `build-38` and `build-39` (37 and 38 never launched), `series-28.log` to `series-32.log`: 28 `i114-jmp-*`; 29 `i114-jstep-*`,
  `i114-st4-*`; 30 `i114-jfix-*`, `i114-st5-*` (`-o60` the original); 31 `i114-oled2-jump-*`; 32 `i114-smoke-*`); fifth tranche, builds `build-42` to `build-44`, `series-33.log` (`i114-smoke5-*`); the
  §6 triage's report is in this RESULT only (a worker's, its verdicts marked); seventh tranche, builds `build-46` and `build-47`,
  `build-48`, `series-35.ps1`, `series-35.log`, `series-36.log` (`i114-t7-*`; its `series-36.ps1` overwritten by the eighth
  tranche's, lost); eighth tranche, `build-t8a`, `series-36-t8.ps1` (`i114-t8-st-*`, in `series-36.log`), `series-37.ps1`,
  `series-37.log` (`i114-t8-na-*`), `series-38.ps1`, `series-38.log` (`i114-t8-l3-*`), `build-t8b`, `series-39.ps1`,
  `series-39.log` (`i114-t8-top-*`, `i114-t8-fz-*`).
- Readers: [Get-LocomotionLegs.py](Get-LocomotionLegs.py) (legs and ends of the Firelink path), [Get-Mechanics.py](Get-Mechanics.py)
  (the mechanics path by key windows, and by event from the speeds: the « events » line holds the raw events series 21's ranges
  were read from; the first, partial roll, the zero-length stops, the attacks' secondary moves and the fourth, weaker attack were
  set aside by hand, and the sprint speed is read from the rows), [Get-Stairs.py](Get-Stairs.py) (the stairs path's legs and the
  terrain rows over the whole motion; series 25-26's 28-second walking windows were computed by hand with the same rows, from the
  first row faster than 1 m/s after 50 s of logic), [Get-Lockon.py](Get-Lockon.py) (the lock path by key windows), [Get-Jumps.py](Get-Jumps.py) (the jumps path by key windows).

## Closed (2026-11-09)

Closed 2026-11-09 with Thomas's yes (relayed by the coordinator, 01:55 Paris): the open points below are carried to [I-0132 R11-PLAN §6](../I-0132-rt-optimisation-zones/R11-PLAN.md#6-the-open-points-carried-from-the-investigations-closed-on-2026-11-09), where each has its place.

## Uncertainty / unresolved questions

- Which of the camera's nine laws carries the 240 drift is not isolated (the speed law does not: `i114-ab-speed-f240`).
- The jump: its held speed is 60's within ±0.7 % at 120 and « 240 » (seventh tranche, the law 4·(1/60 − dt), its n fitted on
  six runs' rows and checked on two); the hold ends at the first step past the landing (206-208 ms from the takeoff above 60, 217
  at 60), so the airborne stretch is about 10 ms shorter above 60 (INFERRED a 0.5 % shorter jump, not measured whole: the replay's
  keys move by ±40 ms). Under 60 the original's held speed is 1.2 % (30) and 2.7 % (20) above 60's and its landing one step later
  (233, 250 ms): open, no time law fits both rates. The other corrections of the fourth tranche are not witnessed live.
- The display clock with an unlocked rate waited on the 60 Hz ASUS only (the logic at the display's 60); on the OLED at 280 Hz the
  rates run (about 200 frames/s at most) stay under the display's, so it never paced them; Reflex's latency not measured (no
  FrameView SDK on this PC).
- The stairs: on build-30 the terrain field's variation is back to 60's within 12 % but its level is 35-65 % above 60's at 240
  (and 120), its curve not resolved by the 1/30 s rows; the merged law (bounded calls left as the original's) not run; the path
  is repeatable on its first three legs only; the hop itself is not seen on a picture (Thomas's eye). The final review's lead
  (`[mover+0x1A0..0x1A4]`) is refuted: those floats stay 0 on the stairs (fourth tranche); on build-36 the level is 20-30 % higher
  at 240 within a series, while two 60 runs differ by 70 %; series 35: +18 % on the three comparable legs, the vertical jitter lower
  at 240 (the motion not hoppier). Eighth tranche: the same level at both rates at the same place (the climb to the last step with
  the enemies frozen, series 39); series 35's difference was the line walked after the turn; what moves that line at 240 (the
  enemies or the camera, 92° after the walk down at 240 against 86.5-88.1° at 60) is open.
- Lock-on: the strafes' speed is 60's (series 35), their extra 1-2 % the sooner start (option A); the target timer `140D71920`
  (the enemies' AI character) is corrected, its effect on an enemy's steering not seen; the bone constraint and weights are
  corrected (fourth tranche) but not seen live. The ledge fall: not witnessed (INFERRED the same at every rate).
- The ramps are pre-biased by their laws (unit tests) but not witnessed live; the play launcher's `-FrameRate` not run live (it
  prints what is not yet corrected when the rate is not 60: the CONTRACT offers a rate above 60 once its mechanics are corrected,
  and Thomas kept it with its warning, 2026-11-01 01:23).
- The counters' fault (`unlock-observe.flag`, image + 0x40E86BE0) is not isolated to one entry, and the corrections hook some of
  those entries; the corrections ran clean at Firelink, Anor Londo and the Undead Settlement.
- Physics, effects, post and interface rows of UNLOCK-READY §5 are not witnessed: the fourth tranche's effect and task steps ran in
  series 32's two smoke runs, their effect not observed; the bone, hold and child-solver detours installed, never called; the objects behind the effect slots, the task
  evaluation, the child solver and the target timer are not identified.
- Fifth tranche: the lock-on ranges' correction (`1408651E0`) is not witnessed (its effect needs a range's transition); the start
  check runs at 60 and does not install it, so an unlocked play is its exercise after series 33's smoke runs; the triage's
  verdicts not checked by the pilot or a reviewer (`1405CB200`, `1405C75F0`, `1406358D0`, `1405C2CD0`, `140F1DF00`'s semantics)
  are the worker's; open: the fade pair (`[+0x3B30]`), `1405C9DC0`'s n, `140F1DF00`, `1405C2CD0`.
- Under 60 (30 and 20 frames/s, or a drop) the jump's takeoff is not corrected (above); the roll, the sprint and a held move are
  60's at 30 and 20 (series 35), the logic in real time at 20 (dt 0.050).
- 30 frames/s: a few events read off (taps on two or three frames); not attributed to a law.
