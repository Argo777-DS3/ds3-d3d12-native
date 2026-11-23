# Investigation result

## Conclusion

The menu audio path is FMOD Ex plus the FMOD Designer event system behind the FromSoftware "MO" sound manager; it has no dependence on the D3D11 renderer. The native build keeps
the genuine audio initialization and already plays the title music and the menu effects (I-0066 witnesses 1175 and Native75); the request sequence at the title is the same in the
reference and native logs. Nothing audio-related is deferred or absent in `game_bootstrap.cpp`; the native build can lose audio only if the menu logic or the sound step stops running.
See [MENU-AUDIO-NATIVE.md](MENU-AUDIO-NATIVE.md) (checklist A1-A7, live rows AU1-AU6).

## Evidence

- `evidence/I-0069/queries/menu-audio/` (event names of the `.fev` files, install `sound/` listing, native request rows); `decompiled/` exports listed in the note (section 8).
- Banks: `f` menu effects in `fdp_main.fev` (61 events), title music `m000000001` in `fdp_smain.fev`; configuration text at `142829EFF`.

## Uncertainty / unresolved questions

- Volume order (music, SE, voice) and defaults; the bank of the `y` events; the start path of `m000000010`; the mute-flag trigger; which loader serves each bank (movie audio and video: see below).

## Artifacts produced or modified

- `MENU-AUDIO-NATIVE.md`, `BINK-MOVIES-NATIVE.md`, `CONTRACT.md`, exports in `decompiled/`.

## Suggested next step

Run live rows AU1-AU3 in a reference and a native session (read-only) to settle the volume order and the Load Game music continuity.

## Bink movies (iteration 2, 2026-10-11)

General native note in [BINK-MOVIES-NATIVE.md](BINK-MOVIES-NATIVE.md) (path from file to screen, audio, native status, checklist B1-B8, witnesses BW1-BW4, rows BL1-BL5). The code-level ready-to-code spec is [I-0068 part J](../I-0068-static-yebis-ffx-lights-spec/BINK-NATIVE-READY.md); this pass is an independent second reading that agrees with it on every GPU-side value.

- Only two video files exist (`fdp_opening.bik`, `fdp_endroll_ww.bik`); the startup logos are Scaleform pages and boss cinematics are in-engine (Remo), so neither needs Bink.
- Video: CPU decode into `R8` planes, one full-target quad with an embedded VS and the 3-plane PS (the alpha PS is never bound) into an `R8G8B8A8_UNORM` texture bound into the `CSMovieGxTexture` proxy that `MovieScreen` samples; the YCbCr matrix comes from `HBINK+0xD0`.
- Audio: XAudio2 inside Bink with a DirectSound fallback, not FMOD (this corrects `MENU-AUDIO-NATIVE.md`, fixed there).
- Native: `NativeMoviePlayerFactory` omits the player (`native_movie_player_omitted`): no opening picture and no movie sound.
## Remo engine cutscenes (iteration 3, 2026-10-11)

General native note in [REMO-CUTSCENES-NATIVE.md](REMO-CUTSCENES-NATIVE.md) (data, lifecycle, the 17 acts, camera, readers of the "cutscene running" flag, native status R1-R14, witnesses RW1-RW4, rows RL1-RL9).

- A cutscene is a data-driven timeline (`remo/sAA_BB_VVVV.remobnd.dcx` = one MQB timeline plus one Havok animation per cut) played by genuine CPU code (`SprjRemoImp` -> `Man` -> `Res` -> `Ins`); 72 bundles were read, 41 ids are referenced by EMEVD bank 2002 in 14 maps.
- The native build hooks nothing of it; no recorded native run has played a cutscene. The GPU-side needs are the scene parameter groups and post consumers, cutscene lights, the fade plates (no native drawer found) and the camera route (consumer not located).
- Correction to the brief: Remo subtitles use caption slot 1 (kind 1), slot 0 is the Bink movie subtitle.

## Fade plates (iteration 4, 2026-10-11)

General native note in [FADE-PLATES-NATIVE.md](FADE-PLATES-NATIVE.md) (objects and API, update and composition, submission, the full driver census, the map-move and cutscene sequences with the `PrologueWait` link, native gaps F1-F9, witnesses FW1-FW5, rows FL1-FL7).

- Seven plates are composed into **one** colour and one rectangle (this corrects MENUS-MAP section 6).
- The CPU update and recording chain is not hooked; the replay (`DFD270` under `D55190`) is replaced by `NativeRenderStep`, so the plates update but are not drawn; `PrologueWait` and the other waits depend only on CPU state.
- Plate 6 (`InGameChroma`) is consumed by `E006A0`, which the native scene update replaces: no consumer. The draw gate `14075EAD0` and the easing curve `140330F50` are encrypted at rest (loaded-code row FL6).

## Scaleform filter passes (iteration 5, 2026-10-11)

[SCALEFORM-FILTER-PASSES.md](SCALEFORM-FILTER-PASSES.md) complements (does not rewrite) I-0067 SCALEFORM-FILTERS-NATIVE for the pilot's steps F2 to F7: the programme behind each filter key (VS 507, PS 1269..1274 for everything the menus use), the DXBC math and constant-buffer layouts, the uniform-slot to field map, the states and targets of the pass loop, worked pass timelines, and the census of menu pages by parameter set with witnesses PW1..PW7 and rows PL1..PL6.

## Ending credits and native fades (iteration 6, 2026-10-11)

[ENDING-CREDITS-NATIVE.md](ENDING-CREDITS-NATIVE.md): how the original reaches the credits, the movie and states used, test routes without an end-of-game save (R1 to R5, rows CR1 to CR6), and the native D3D12 fade design that completes FADE-PLATES-NATIVE F2 to F5 (ER method, D-0157; rows FD1 to FD7).

- The credits are a Bink movie (INFERRED `fdp_endroll_ww.bik`, movie id 3 from the table `{2,3,3,3}`, 433.8 s) started by state 0 of the FD4 step `EndingStep` through `FUN_140BEA570`. No fade-plate call site is in the credits code.
- The way in (STATIC): the ending events of map `m41_00` (cutscenes `41000000`, `41000050/51`, `41000060`, `41000070`) set event flags 20 to 23; the world step reads them in `FUN_1408F5930`, sets the request byte `[14475AC00+0xAE8]`, and the title-flow step constructs `EndingStep` in its state 6 (`FUN_14090ADB0`). Flag 30 leads to another state (not read).
- Test routes: R1 calls `FUN_140BEA570` through a witness file (read-only, guard mandatory: the call crashes without the menu-job owner), R5 sets event flag 20 in a loaded world to run the whole chain (needs the D-0116 restore). Neither was run.
- Native: the native Bink player (`6d857eef`, full pace in Native1252) already plays the credits movie; the fade quad is the missing piece. Design: composite read from the genuine CPU fade system, one full-target triangle with 16 bytes of root constants, created at open, recorded at the end of the UI category in the two live recorders, no per-frame resource to retire.
- Review: an independent Opus 5.5 review found the first version wrong on the trigger (it was reachable through LEA-based references that the `w:` queries miss), on the identity of the singleton, on the R1 safety claim and on several status labels; all were fixed in this version.
- Open: the dialogue that starts the `m41_00` events, the value of `DAT_14489F200` after the credits and what the world exit writes (CR5), the page opened by the credits callable, the language-group stub (encrypted at rest).
- Budget: iteration 6 of 6 is used; `iterations_max` is extended to 8 under the WORKFLOW rule. Next bounded plan: the live rows (CR1 to CR6, FD1 to FD7 and the earlier AU/BL/RL/FL/PL rows) when the pilot runs them, and a loaded-code row for the encrypted stubs.