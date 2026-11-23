# I-0115 result: the Silver Knight that flickers (Le Silver Knight qui clignote)

Complete (2026-11-02): merged `f0e8548c` (the launcher rebuilt, unit tests passed; start check `start-f0e8548c` passed). The native
cause found and fixed (section « Step 3 »), validated by Thomas's eye (08:29) and by the rows; the original's side INFERRED (item 7).
Branches `claude/keen-poincare-77b50d` and `claude/objective-visvesvaraya-7fa224`. The plan is the [CONTRACT](CONTRACT.md).

## Step 1: series 30 read (no AI), OBSERVED

The three runs of I-0114's series 30 (2026-11-01 02:34-02:43, the ASUS, `anorlondo-stairs.path.json`, `test-no-ai` and
`popin-observe`; the path walks up the stairs, Z 8 s, and back down backwards, S 8 s, so the knights following the player come
under and behind the camera): `i114-st5-o60` (the original), `i114-st5-n60b` and `i114-st5-f240` (native, `main` 3c9e132e and the
framerate pilot's probes, the margin active).

1. **Natively every cull of a near knight is of a box outside the drawn view.** The gate rows of the c1410 draw entities within
   12 m (`popin_native_gate`, n60b serials 5300-7600): `frustum` (outside the walk's culler grown by 1.5 m) and `margin` (kept by
   the margin) always with `drawn_box` 0, `passed` always with 1; over both runs no draw entity in the drawn view went off
   (frustum or absent with `drawn_box` 1: none, except the run's last prepare where every model goes). The knights' boxes are
   behind or below the camera when cut: at serial 7109 the box's x ends 1.4 m behind the camera walking up +x; at 5532-5535 the
   camera stands 0.8 m above the box's top inside its x-z footprint and the gate flips passed/margin each frame, drawn on both.
   The box follows the draw matrix (its centre within 1-2 m, the weapon's side).
2. **The original cuts its near knights too** (`popin_reference`, its emits): bb8c0 off for one step at 4.42 m (step 3714, its
   rows dropped with it: `popin_reference_chr` 2 → 1 entities, 6 → 3 rows) and for 33 steps at 7.3 m (5481-5514). Natively
   short gaps of the same kind: 7ead0 1 serial at 4.7 m (f240 17608), 86e30 16 serials (f240 20434), ee1e0 5 frames at 11 m (n60b
   7529). The two renderers' knights move on the same path at about the same times (native serial ≈ original step + 1832): the
   long gaps match in place and length (the original's bb8c0 off 4473-4742, the native's e5d70 off 6257-6645).
3. **So series 30 does not show the flicker** Thomas saw (series 29, with AI, without popin-observe): what it records is DS3's
   own culls of knights out of view in both renderers. D07E10 (the draw entity's slot +0x98) returns E+0xB80, which D08190
   recomputes from the bones when E+0xBA8 bit 2 is set: the native tests the same box as the original's walk.
4. **Instrument** (`popin-observe.flag`, both renderers, build `evidence/I-0115/build-1`): `popin_native_near`, each character
   model within 8 m of the camera, on change: drawn or why not (not ready, no main row, absent, far), the ordinary category and
   the fade group's +0x84/+0x85/+0x87, +0xDC (09_model_admission's inputs) and the fade intent's opacity; `popin_reference_near`,
   the original's draw entities of character models emitted within 8 m (PopinDrawEmit, D079C0 with the group D06E10 hands it),
   the same words and the group's +0x124, « absent » when one stops being emitted while near. Reader `Get-NearRows.py`.

## Step 2: the stairs with AI in both renderers, OBSERVED

Series 1 (2026-11-01 03:09-03:14, announced, the ASUS 1920x1080@60, no measure, offline, restored by hash; build
`evidence/I-0115/build-1` = `55ce898e`'s code on `main` 3c9e132e; `scripts/replay/anorlondo-stairs.path.json` with AI, as series 29,
`popin-observe`; log `evidence/I-0115/series-1.log`): `i115-al-o60` (the original, Reference) then `i115-al-n60` (native, 60).
Gamma died in both near the end of the second descent (every character « far » or absent: original step 4187, native serial 5951).
Read with `python -I Get-NearRows.py i115-al-o60 i115-al-n60` and the gate rows.

1. **The same knight is cut at the same place and time in both renderers.** The original: c1410 `89a00` emitted at 7.9 m (step
   3439), stops being emitted at 3.84 m (step 3532, `popin_reference_near` « absent ») and comes back at 3.89 m 243 steps later
   (3775). The native: c1410 `23610` drawn at 7.8 m (serial 5263), kept by the margin at 2.9 m (5309), cut by the walk's frustum
   test at 5.1 m (5352, `drawn_box` 0: outside the drawn view too) and drawn again 165 frames later at 5.0 m (5517, margin; passed
   at 5567). The two clocks align at native serial ≈ original step + 1830 (series 30 gave 1832): the cut starts within 10 frames
   in both, the native's gap is shorter (the margin).
2. **No state change of the knight near the camera**: its fade group stays `k` [255, 0, 0], `dc` 1.000, `o` 1.000 in both, the
   native's category 2 (deferred) on every drawn row. The near camera fades the player instead: his parts go to `k` [255, 1, 0],
   `dc` 0.98 and the native's category 0x29 (41) for a few rows (FG_A_*, HR_A_0101, FC_M_0000), the native's explicit translucent
   policy (I-0113 STUDY-TRANSLUCENT-CHAMELEON §2); the knight is never in that state.
3. **The original's instrument saw only the knights' approaches** (6 rows): it covers draw entities (D079C0), not the player's
   assembled parts (CFDE70), so the player's fade is native-only in these rows.
4. Thomas's eye during the native run: not reported to this thread before it stopped.

## Step 3: the original against the native, filmed, and the cause (2026-11-02)

Runs (announced, the OLED 2560x1440 at 280 Hz, a 1280x720 window, nothing measured (the native runs in measure mode, without the
debug layer), offline, restored by hash; build
`evidence/I-0115/build-2` = `main` 1fc94aef with this investigation's near rows; `test-freeze-ai` and `popin-observe`; the new
[Run-I115Steered.ps1](Run-I115Steered.ps1), steered from a command file: video, shots, menu clicks): `i115-fl-o60` (the original, 07:33)
and `i115-fl-n60` (native, 07:38): `anorlondo-stairs-top` filmed, the return to the last bonfire (the first shortcut of the pause
menu, the Coiled Sword Fragment, then « Bonfire last rested at »), the climb filmed again; `i115-hall-o60` (07:58) and `i115-hall-n60`
(08:01): the new `scripts/replay/anorlondo-stairs-hall.path.json` (Z held 30 s, into the cathedral's hall), then a look left and right.

1. **Thomas's place** (2026-11-02 07:55): the flesh masses on the left, well into the cathedral; during `i115-hall-n60` he wrote « Vu »
   at 08:02:31 and « Revu » at 08:02:40 (native frames about 5340 and 5880).
2. **The LOD scale** (Thomas's hypothesis): 0.756695 at the first load in both renderers; after the return to the bonfire the original's
   goes to 1 (`ref_fov` takes the camera's fov, as after a death in I-0100) and the native's stays 0.757 (option A, D-0222). It moves
   the distances below by 1.32 in the original after a reload; it does not make the flicker (seen at the first load).
3. **The blink, OBSERVED in the rows** ([Get-Blinks.py](Get-Blinks.py)): natively, each already drawn character falls to opacity
   0.083 at one distance and comes back over 11 frames (`popin_native_fade`, the main draw's fade intent, dithered): the Silver
   Knights c1410 at 27.7 m, the flesh masses c2131 at 19.5-20.1 m (five in `i115-hall-n60`, the first at frame 5305, 35 frames
   (0.6 s) before the frame of Thomas's « Vu » message: a timing match, INFERRED the same event), at the first load and after the
   reload alike. The original's side is NOT established: its character draws were recorded at opacity 1 whatever they were
   (`PopinDrawEmit` passed no opacity, step review M1); since `build-4` they carry the fade group's +0x124 and each view's lowest
   (`popin_reference_fade` `o5` / `o45`), to read on a new original run. The distance is the LOD step's `draw_at` (c2131 class 5: 15 / 0.757 = 19.8 m, `popin_lod`),
   where the geometry weight (the LodParam's second distance) starts its ramp: the same frame the shadow rows go to `cast` with
   weights [1.000, 0.083].
4. **Cause** (OBSERVED in the code, `09_model_admission.inc`): for an entity of categories 0x01 and 0x02/0x04 the native's main walk
   used the pass-5 view (the geometry weight) whenever it had an intent, else the 0x45 view (the track weight, 1). Beyond that
   distance the geometry weight is 0 (no pass-5 intent) and the 0x45 view drew the character at 1; at that distance the pass-5 view
   took over at 0.083. The videos at 10 frames a second did not show it (the knights hidden behind the stairs' top at 28 m, the
   hall too dark in a desktop capture); Thomas's eye did.
5. **Fix** (build `evidence/I-0115/build-3`, unit tests passed; `build-4` adds the step review's answers): the more opaque of the
   two views draws such an entity (the 0x45 view only when strictly more opaque); `no-view-weight-fix.flag` gives the former rule.
   INFERRED that this is what the original's screen shows (its two views' draws); to confirm by Thomas's eye with the test launcher
   (`-Build` build-3), by the rows (no blink with the fix) and by the original's `o5` / `o45`. Accepted divergence (step review L2):
   such a model's shadow rows take the same 0x45 intent, so its shadow starts at full strength instead of the 11-frame ramp. The other runs: the hall path ends in a death in both renderers (the ceiling's flesh mass,
   a grab, despite `test-freeze-ai`); `i115-hall-n60` left the known exit fault in `141A3B670` at the close (I-0114), not during the path.
6. **Thomas's test** (2026-11-02 08:22-08:29, the test launcher with `-Build` build-3, Gamma's copy, `-NoAi -PopinObserve`, the
   OLED; journal `%LOCALAPPDATA%/DS3-DX12-Test/test-20261102-082234.jsonl`): « Pour moi c'est validé, je ne vois plus de
   clignotement » (08:29). Its rows agree: Get-Blinks finds no drop of a drawn character where the geometry weight starts (the
   shadows' `cast` rows at [1.000, 0.083], c2131 at 38 m in his session), only five c2131 fading out at their draw distance at
   136 m as he walked away; he came within 2.4 m of a flesh mass and 1.9 m of a knight. (Why the geometry weight started at 38 m
   in his session and 20 m in the runner's: his journal's `popin_lod_scale` is 0.378347 at 2560x1440, `ref_height` 720, against
   0.756695 at 1280x720 in the runner, OBSERVED: 6EB60 divides the scale by height / reference height.)
7. **The original's two views and the fixed native, OBSERVED** (09:06-09:10, the OLED, build `evidence/I-0115/build-4`, the hall path,
   `test-freeze-ai` and `popin-observe`): `i115-view-o60` (the original, its draw entities now recorded with their fade group's
   +0x124 and each view's lowest, `o5` / `o45`) has no `popin_reference_fade` row at all: no flesh mass (down to 5.7 m) or knight
   (down to 26 m) emitted under opacity 1, and every character row's first window came through the 0x45 view alone (`ids` 2).
   That the original draws them at 1 where the native fell to 0.083 stays INFERRED: the new +0x124 read (the near rows' reading
   of D079C0's group, decompile not re-read) recorded no character fade at all in that run, not even a first appearance, so no
   positive case shows that it sees one. `i115-view-n60` (native, the fix): `Get-Blinks.py` finds no blink (none of the 0.083
   drops of `i115-hall-n60`).

## Conclusion so far

The flicker Thomas saw is a native fade fault, not DS3's culling (steps 1-2 recorded the cuts of knights out of view, the same in both
renderers, which steps 1-2's rows could see; the blink is a fade change of a drawn character, which they did not cover beyond 8 m and
did not look for): each character of categories 0x01 and 0x02/0x04 dropped to 8 % and came back in 0.2 s at its LodParam second
distance (OBSERVED natively, step 3). The fix removes it: Thomas's eye and his session's rows (build-3, item 6) and a native run's
rows (build-4, item 7). INFERRED: that the original draws them at 1 there (its rows show no fade and only the 0x45 view, item 7,
but its instrument has no positive case) and that the screen of an entity drawn by both views shows the more opaque one.

## Reviews

- Step review (fresh Opus, high, `1fc94aef...651282d5`): nothing high. M1 (the original's draw entities recorded at opacity 1
  whatever they were) answered: their fade group's +0x124 and each view's lowest (`o5`, `o45`) recorded, the claim kept INFERRED;
  L1 (an unreadable track weight) keeps the pass-5 selection; L2 (the shadow's start without the ramp) accepted and recorded; L3
  (Get-Blinks keyed by place) the native fade rows carry their entity; L4 the slips; L5 the runner's waits bounded (b752b302).
- Final review (fresh Opus, xhigh, `main...460ed437`; a first run lost to a PC restart): nothing high. M1 (the original's side) kept
  INFERRED (item 7: no positive case of the +0x124 read); M2 (Get-Blinks on the original) fade-ins and fade-outs set apart; M3 and
  L6 the stale records and wording; L2 the CONTRACT amended; L3 the 38 m explained; L4 the runner refuses a reused name; L7 the
  evidence manifest's subject; L8 the rule as `PreferMainViewFade` with its unit test (the model preparation control, which also
  needed Streamline's host to link: PASS on `build-5`); L1 (the shadow's ramp) and L5 (the near rows' per-frame cost, diagnostics
  only) left, in « Next » (d5d2f96e, eb5439eb).
- Optimisation pass: the fix adds one 0x45 selection per category-0x01 entity that also has a pass-5 intent (one
  `SelectModelFade`, no allocation); every other addition is behind `popin-observe.flag`. Nothing to remove.

## Next

- Left open: the original's side read in its code (D06E10, which view emits a character of categories 0x01 and 0x02/0x04 and
  with which weight) or a positive case of the +0x124 read; the shadow's start without the geometry weight's ramp for such models
  (step review L2, accepted: a `shadow_fades` selection would restore it); the near rows' per-frame cost in popin-observe runs (final
  review L5, diagnostics only); the player's parts in category 0x29 at the camera's approach (`dc` 0.98, I-0113 gaps 11/19).
