# I-0138 tranche 7: the settings rows, a first sheet

2026-11-15, written by the R thread (Opus 5.5) while it waited for a live slot on I-0141; nothing here is built. Sources: [PLAN](PLAN.md)
§5 (tranche 7) and §7, ROADMAP workstream 11 item 7, D-0276, D-0285, the settings model (`native/renderer/rekindled_settings.h`), the
pages' rows (`native/frame/graphics_rows.h`, `game_bootstrap/02b_rekindled_graphics.inc`), the Graphics tab's added slots
(`native/frame/options_film.h`), the candidate looks (`native/renderer/tone_looks.h`, `04_updates_and_test_tools.inc`
NativeToneLookRequested), ER's LUTs read (`evidence/I-0138/artifacts/er-luts/REPORT.md`, RESULT « ER's LUTs, read »). Labels:
**verified** (read in the code), **proposed**, **open**.

## 0. Answer first

1. **The rows** (PLAN §5: « bloom, light shafts, grain de film, champ de vision, sortie HDR, le choix de colorimétrie, et l'aberration
   chromatique de la colorimétrie Rekindled »; the HDR output is done, tranche 5b): **Colorimetry** (Original, the default; Rekindled,
   D-0285), its sub-row **Chromatic aberration** (shown while Rekindled is chosen, On by default: Thomas 2026-11-12 12:34 « on garde »),
   **Bloom** (a strength), **Light shafts** (On or Off), **Film grain** (a strength, Off by default: a new effect, PLAN §5), **Field of
   view** (its own study first, PLAN §5: below, §4).
2. **Sliders for the strengths** (Thomas, 2026-11-14 12:05, the memory « numeric settings as sliders »): bloom and grain as the game's
   slider rows by 10 %, the unit in the label; the On/Off rows as choice rows.
3. **Where** (proposed): the rendering rows on the Advanced page, before the Vignette, which stays last (Thomas, 2026-11-11 15:19);
   the field of view on the Graphics tab (a display setting, as the HDR rows). The Advanced page's film has only choice slots: a slider
   there needs the slider kind in its slots (as `options_film.h` did for the Graphics tab), **open** until its film is read.
4. **The Rekindled look** (D-0285): `tone-look-er.flag`'s candidate (its curve, vignette, lateral dispersion, the bloom before the
   curve, no DS3 grey grade) plus a midtone and shadow lift of our own after ER's LUTs (their grey node 8, 136 in, at 149 to 173 over
   the overworld's day, median about 154; black slightly cool, 0, 0, 10 to 21; white kept): proposed a power curve on the display-encoded
   value, y = x^0.8 (136 to 155), and the black's blue raised by about 0.04 fading out by mid-grey; to be judged on sheets beside the
   candidate without it (Fable as a read-only subagent for the colorimetry, Thomas's word of 2026-11-15 09:20).
5. **Test flags retired by the rows**: `tone-look-er.flag` becomes the row (the AgX and ACES candidates and `tone-look-keys` stay test
   tools until tranche 8, D-0285 offering neither); `no-yebis-shafts.flag` stays a test tool beside the Light shafts row.

## 1. The settings model (verified, then proposed)

- `settings::Settings` holds the RT rows, the frame rate, the V-sync, Borderless, the Vignette and the HDR rows; `Parse` and `Write`
  are the file `rekindled-settings.ini`, unit-tested; an unknown key is counted, a refused value keeps the earlier one.
- Proposed keys: `look=original|rekindled`, `aberration=on|off`, `bloom=0..100` (percent, default 100), `shafts=on|off` (default
  on), `grain=0..100` (default 0), `fov=<degrees or a scale>` (after §4). The launcher's migration reads them as it reads the others.

## 2. The rows (verified, then proposed)

- `graphics_rows.h`: `RtRow` lists the Advanced page's rows; `RtShown` hides a sub-row (the DLSS mode, the bounces), `RtAvailable`
  greys one, `RtChoices` its values, `RtWrite` the settings after a choice, `RtGoverns` the rows whose change rebuilds the page; the page
  scrolls over 14 slots (`kPageSlots`), so more rows cost nothing in the film.
- Proposed: `Look`, `Aberration` (a sub-row, `SubRow`), `Bloom`, `Shafts`, `Grain` before `Vignette`; `Look` governs (the aberration's
  row appears with Rekindled). The texts in English and French as the others.

## 3. The readers (verified, then proposed)

- The look: today `NativeToneLookRequested` (04) reads the test flags; proposed: the settings' look when no test flag asks one.
- The bloom: the glare plan's last weight (the bright pass's luminance or the composite's factor) scaled by the strength; 0 draws no
  glare at all (the shafts with it, since they ride the glare plan: to state on the row's help line, or the shafts' own draw kept).
- The shafts: `AppendNativeShafts` not called when off (as `no-yebis-shafts.flag`).
- The grain: a new pass after the tone map (a blue-noise or the executable's film-grain texture, `NativeFilmGrain`, read for the shafts'
  mask), monochrome, its strength the row's; open: its exact form, to show Thomas on sheets.

## 4. The field of view (open)

The game's camera writes its vertical field of view at the camera record's `+0x50` (K-0395); our renderer reads it from there, and the
game culls with its own frustum. A row that only widened our projection would leave the game culling a narrower view (pop-in at the
edges). The setting has to be applied where the game builds its camera (the producer `CED670` or its parameters, the LockCam and
camera params), read in a small Ghidra study before any code: which function sets `+0x50`, whether the cutscenes' own lenses must
be left alone (they should: a cutscene's framing is authored), and the range (DS3's default 43.8 degrees vertical at 16:9, INFERRED).

**Read so far** (GhidraMCP, 2026-11-15, the R thread): the step-0x4F task `LodTask_Step4F_BuildGxCamera_140d1de70` takes the active
camera from the game camera holder `[144789CD8]` (`FUN_140989e10`), copies its words `+0x50..+0x5C` (the field of view first) and its
view matrix, and `WriteSceneCameraOutput_140ced670` writes the GX camera record (`+0x50` the vertical field of view, widened to keep
the horizontal one when the screen is narrower than `[142945248]`'s aspect); the same record then feeds the culler
(`CullerUpdateFromCamera_14006e9e0`) in that task. So a field of view changed at the active camera's `+0x50` before this task reaches
both the culling and our renderer (INFERRED: the other readers of the active camera, e.g. the game's own projection for the UI's
markers and the lock-on, not read). Left: who writes the active camera's `+0x50` (the player's camera and its params; the cutscene's
camera, to leave alone), whether the value is read again elsewhere in a frame. A lead (I-0112 [CAMERA-FOLLOW-SITES](../I-0112-unlocked-framerate-impact/CAMERA-FOLLOW-SITES.md)
§3 D): `FollowCam_Update_140515790` lerps the follow camera's `+0x50` toward the camera parameter row (`140515A9A..140515B90`), so the
player's field of view is the param row's; a scale there would leave the cutscenes' cameras alone.

## 5. The witness

Per PLAN: the new rows in the menu (English and French, the page's scroll), each row's effect on one zone's sheet (Irithyll for
the bloom and the shafts), the Rekindled look with and without the lift, the settings file written and read back; the unit tests
(`graphics_rows_test`, the settings' parse and write).
