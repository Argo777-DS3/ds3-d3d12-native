# STUDY-NARROW-WITNESS: the removals, replacements and fidelity fixes validated on one witness or none

The study thread, 2026-11-15 (05:18 to 05:35 Paris), read-only, on the coordinator's request after the E pilot found the credits'
« MOVIE » plate (I-0139 tranche 5, `i139-t5-flag20-nat`; I-0140 RESULT « Next » 9). Thomas's question: are there other cases like
it, in the general sense. No code changed, no run made, no Ghidra session (a live run was in progress). The menu proxies (Status
page, portraits) are the E pilot's and are only cited here.

Sources: every record from I-0066 to I-0141 (a Sonnet sweep of the RESULTs, the two STUDY-NOT-YET-NATIVE, I-0120's REVIEW,
I-0121, decisions/LOG.md; each item below re-read by the pilot at the cited line before it was kept), the native code at `main`
`90c7a6a8b`, and a census of every `startup.jsonl` written since 2026-11-12 00:00 (707 journals, 2026-11-12 00:25 to 2026-11-15
05:21, 27.9 million rows; native and original mixed, 194 of them original or Reference; its script
`census.py` was a scratch file, the numbers are kept here).

## 0. Answer first

- **The A2 mechanism.** I-0137 read the original's import 140E49320 (« returns 0 when the manager makes no image ») and its
  caller 140E48980 (« a 0 is asked again at the next lookup »), and took both as the original's answer for a movie proxy without
  its frame. Two readings were missing: the proxy's own slot answers its inner texture (`[[144792E58]+0x60]`, MENUS-MAP §9), so
  the original never takes the 0 path there; and the credits page asks once, so the native's 0 is never asked again. The witness
  (the character creation) met the case only at a movie's close, where nothing showed it.
- **The census says the A2 case is the only skip of the kind met in play since the 12th.** Of every `NoteNativeSkip` reason, the
  journals hold only `nonfinite_placement` (61 rows, 25 journals; the original uploads such rows, I-0113 gap 19) and
  `ui_import_asset_unavailable` (9 rows, 9 journals). The 9 `native_ui_asset_unavailable` rows are all the movie proxy class
  (`texture_table_rva` 0x29A4968): the credits (`i139-t5-flag20-nat`), every character creation of I-0137's witnesses
  (`i66-native-i137-t4-charmake` to `-t8d-charmake`) and a New Game (`i66-native-r12e-r1-newgame`). So E's fix must be witnessed
  at the credits, the creation after the opening movie and a New Game, not at the credits alone (the NG+ dialog after the film
  was the same in both modes, I-0139 tranche 5).
- **No other record shows a regression traced to a stand-in or fallback removal.** The cases below are of the same shape (the
  native shows nothing where the original shows something, on a reading or a witness that did not cover the case), none met in
  the 707 journals; § 1 lists them with the witness that would settle each.
- **Thomas's own removals** (D-0262 « no fallback », D-0254 and D-0262's fill lights and baked ambient, D-0278's anti-aliasing
  filter, gap 24's Firelink walls) are choices with known, filed costs, not hidden regressions: § 3.
- **One judge to re-check**: the Reference mode's body-scale witness forwarded two of four arguments from 2026-10-10 to 2026-11-13
  09:00 (I-0137 tranche 7); originals with posed characters run in that window are suspect as judges: § 4.

## 1. Same shape as A2: the native shows nothing where the original shows something (to test)

Ranked by the chance of a visible regression (this study's judgement).

| # | Change (record) | What the original does (status) | Met since 11-12 | Witness that settles it |
|---|---|---|---|---|
| 1 | A2, the movie proxy without its frame answers 0 (I-0137 tranche 3) | the proxy forwards to its inner texture (MENUS-MAP §9, STATIC) | **9 rows**: credits, every creation, a New Game | E's plan (I-0140 « Next » 9): the credits by flag 20 natively, a creation and a New Game, stills or a film at the movie's open and close |
| 2 | A2, the menu proxy before its target or after its view retired answers 0 (I-0137 tranche 4; witness `i137-t4-status`, where the case did not occur) | « Stop » removes the scene, « the proxy keeps the last image » (MENUS-MAP, the Load Game portraits, STATIC) | no row (`native_ui_offscreen_placeholder` and `_asset_unavailable` none for the class 14295DA78) | **E's** (the menu proxies); the same « follow the holder » rule as row 1 |
| 3 | The other proxy classes of the family (`SprjMiddleGxTexture`, `SprjDummyBlinkGxTexture`, `GXBS::GXTextureBaseProxy`, MENUS-MAP §9): `NativeUiTextureImport` finds no resident record and answers 0 | each forwards to its inner texture (INFERRED from the family's shared slot 0) | none (only 0x29A4968 in the rows) | none needed now; E's fix of row 1 written for the proxy family rather than the movie class, and the row's `texture_table_rva` watched |
| 4 | A UI image binds the record it imported, not its holder (`UiImageIngress::Import`; E's finding): any texture imported before it is resident stays absent for a page that imports once | the original's image wraps the holder and samples it at draw time (E's reading, to confirm in 140E478F0) | only row 1's class | part of E's plan (the rebind) |
| 5 | The face image's mark layer skipped when its texture is not resident (I-0113 tranche 16; STUDY-NOT-YET-NATIVE row 16) | 1423781F0 draws the mark's colour alone (READ, recorded open) | no `face_mark_texture_not_resident` skip | a character creation with a mark chosen right after the creation opens (when its texture may still be loading), native and original stills |
| 6 | I-0121 D2: an effect statement whose FFX texture (or SYSTEX_WHITE for the GX solid texture) is not resident is dropped for that frame (`21_effects.inc:801`) | the effect's textures arrive with its resource (INFERRED, not read): the original has no « not resident » case | **unknown**: the counter (`unresolved_textures`, `dropped`) is logged only with the effects witness flag | one run in an effects-rich place (Firelink's bonfire, a spell series) with that flag: a count that stays above 0 after the first seconds is an effect drawn by the original and never by the native (STUDY-NOT-YET-NATIVE row 8's « a few particle draws missing or untextured » may be it) |
| 7 | The UI's never-witnessed families: a display of offscreen composition or with an authored colour clear opens as an invisible one (I-0137 tranche 5); a blend mode, primitive state or mesh kind never seen is skipped (I-0101 item 4) | the original draws them | no `native_ui_skipped` row | none until a row appears; the row names the family (before, the game closed there) |
| 8 | The world frame with no UI packet presented without the overlay (I-0137 tranche 7) | INFERRED: the original draws that frame with nothing from its UI | **1,205 rows, 132 journals**, all in cutscene and boss runs (`no_movie` 514, `movies_without_packet` 691) | a subtitled cutscene and Friede's second phase natively and in the original, stills of the subtitles and the boss bar: equal means the inference holds. **Done** (I-0140 RESULT « Two items of STUDY-NARROW-WITNESS »): the subtitles by the tour, the boss bar on the tour's Friede runs, equal in both |
| 9 | G6: an image past the UI set's 1,024 answers 0 (I-0137 tranche 2) | no such bound in the original (INFERRED) | no `ui_import_set_full` | none; the row is the witness |
| 10 | S4 (RT): a material whose diffuse view the bindless heap refused is a record without material, a miss for the rays, not white (I-0137 tranche 4) | the original has no rays; its raster draws the material | not counted by a row | none until the heap is near full (workstream 15) |
| 11 | C6: a composed effect group's member linked after its composition is not looked for again (I-0137 tranche 4) | the original finds the member when it draws | not counted | an effect seen whole in the original and partial natively would point here |

Safe after reading (the original's behaviour READ, the case not met): A3 (14002B260 and 1400AC110 leave the wrapper empty;
`native_encoded_texture_failure` 0 rows), B6, the movie planes' skip (the output keeps its last image), the map piece's runaway
child list (`disp_children_bound`, not in the rows).

## 2. The native's « 0 / skip » rows met in play, checked against the original

| Row | Rows, journals | Read | Verdict |
|---|---|---|---|
| `native_body_scale_update_skipped` | 96, 10 (cutscene runs) | every row `quit` true (e.g. `i140-e27-c32000000-nat`, lines 58743 to 58790 of 61262, before `native_exit_sound_stop`) | safe: the gate acts only after DS3's quit request (I-0137 tranche 8) |
| `native_menu_model_update_skipped` | 3, 3 | `quit_requested` | safe, the same |
| `native_asm_part_missing_binding` / `_missing_inputs` | 89,369 each, 498 | the parts' model has no storage, no mesh, no material (`storage` 0, `meshes` 0): an empty slot | safe: nothing to draw in the original either |
| `native_selected_palette_unavailable` | 20,702, 489 (sampled 1 in 300) | character skin, hair, face and eye parts with no object pose on slot 5 (Anor Londo's run: lines 22434 to 39780 of 57913) | a delay while the pose is made (D-0197), long-standing (1028, 1070); 21_effects.inc:1011 records one form where the original « draws on the b5 it had » (a model particle's Dynamic mesh with an empty list): to watch, not shown visible |
| `native_selected_texture_unavailable` | 1,584, 99 | all `reason` 1, the bindings not ready (16 rows a run at most) | a delay (D-0197) |
| `native_texture_white_fallback` | 41,651, 498 | the loader's SYSTEX_DUMMY for a texture bound with no data: `_s` shininess, `SYSTEX_Dummy*`, FaceGen | the original's own default (I-0113's Anor Londo reading); the faces' white `_s` is I-0140's open question, not a removal |
| `native_disp_refused` | 7,671, 39 | `light-observe.flag` diagnostics of the original's own map piece gates (reason 3, the visibility words) | safe: the original's gate |
| `native_offscreen_hair_refused` | 5,820, 97 (I-0138 tab runs) | the menu views' hair | **E's** (the menu views) |
| `firelink_wall_dropped` | 1,480, 395 | Thomas's « off forcé » (gap 24) | intended |

## 3. Removals Thomas decided (known costs, already filed)

- D-0262 « no fallback » (I-0132 R11e): an arrival held black up to 5 s; I-0139 tranche 1 counted 12 of 55 arrivals released by the
  timeout, Ariandel 334 black frames (ROADMAP workstream 7).
- D-0254 and D-0262 (fill lights 1 to 3, the baked ambient): Siegward black or grey in 39000020 (I-0141 RESULT, the provisional
  D-0279), the outdoors darker (I-0139); workstreams 7 and 12.
- D-0278 (I-0138 tranche 5c): DS3's anti-aliasing filter removed, the fallback when the TAA or DLSS resolves no frame on an SDR
  chain; « not witnessed live » (I-0138 RESULT). To test once: an SDR run with the upscaler made to fail, the picture unresolved
  rather than black.
- Gap 24 (I-0113 tranches 38 and 40): Firelink's two loading walls; « a crossing of the west entrance right after a warp never
  done ».

## 4. Fidelity fixes on a narrow witness (the original read, the picture checked once)

Lower risk (each a transcription of a read function), listed so the next run in that place looks for it: I-0113 tranche 14 (the
main view's masks as constants, Anor Londo only), tranche 15 (the forward rows' key order, the menu views not compared), tranche 17
(pass 0x31 into a half-size layer, one bonfire at 1280x720 without DLSS), gap 17 (the effect context's states: the crystal and the
flask only, Lightning Spear not seen again), I-0108 tranche 1 (depth of field at quality 0 and 1, one zone), I-0138 tranche 6 (a
cutscene key without its group reads `default`, 17 to 48 of 49 cutscene files: VERIFIED in the executable, « not witnessed yet in
a cutscene »). Two rest on an inference rather than a reading, and are the likelier to be wrong: **I-0113 gap 9** (the row
inheritance removed after an INFERRED cause; three casts at Firelink) and **I-0115** (« the more opaque of the two views draws »,
the decompile not re-read).

**The judge**: the Reference mode's `ObserveBodyScaleUpdate` (slot 7, hooked unconditionally in that mode since `2d67d08a`,
2026-10-10) forwarded (this, matrices) of (this, matrices, count, first) until `63b113d9` (2026-11-13 08:57): a possible
out-of-bounds write on every effective body-scale update (I-0137 tranche 7, « suspect until taken again or argued harmless »).
Every original run of that window with a posed character (the creation's preview, the cutscenes' player actors: I-0139's first
cutscene comparisons on `build-tour5/6` among them) is suspect as a judge. The tour's re-run on main (`c64ba7f07`, 2026-11-14) is
after the fix: verdicts resting only on the earlier originals are re-read against it before anyone acts on them.

## 5. The lesson, for the rule

A reading that « the original returns nothing » in a case must read what the original's arguments hold in that case (here the
proxy's slot), and a « 0 is asked again » must find the caller that asks again in every page met. And a stand-in may be the
original's own default (SYSTEX_BLACK behind the movie proxy, SYSTEX_DUMMY behind a texture with no data): D-0197 forbids a stand-in
the native invents, not the original's. Adopted by Thomas (2026-11-15 05:42, « Oui vas y ») as D-0290, in WORKFLOW « Method rules »: before a stand-in
is removed or a refusal becomes a skip that draws nothing, the original's answer for the same input is read, and the case is
witnessed in every place the journals show it met (here, the `native_ui_offscreen_placeholder` rows named the credits).

## 6. Limits

- The census counts rows; capped rows (many at 4 or 16 a run) undercount, and the effects' drop counter is behind a flag (§ 1 row 6).
- The sweep read I-0105, I-0117, I-0119, I-0122, I-0128 and I-0132's R11d and R12 only by grep.
- Nothing here was observed in a run; the « original » columns are the records' readings (STATIC or INFERRED as marked).
