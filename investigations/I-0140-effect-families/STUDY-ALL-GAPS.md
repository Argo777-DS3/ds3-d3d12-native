# STUDY-ALL-GAPS: every known or suspected departure of the native picture from the original's

Study thread, 2026-11-14 22:35 to 23:40 (Paris), read-only (no build, no run, no game launched; the PC shared with R, E and the
tour). It widens [STUDY-UNREAD-SETTINGS](STUDY-UNREAD-SETTINGS.md) (the settings never read, 11 items) to the whole inventory
Thomas meant: every odd difference of the native picture, whatever its cause.

Sources, read on 2026-11-14 at about 23:00: `main` `6e6226939`; the R branch `claude/quizzical-chebyshev-e5cf9c` (`26e36c247`:
I-0138 tranches 5d and 5e, D-0280, I-0141 tranche 2's preparation); the E branch `claude/beautiful-ellis-78a5ea` (`1940c865b`:
I-0140 « The Road of Sacrifices' green moss », « The Painted World's cyan veil », « 51000000's angels », « Next »); the tour branch
`claude/kind-antonelli-18b0e1` (`c64ba7f07`: I-0139 tranche 3, with « The 24 scenes again natively on `build-tour10` », read
at 22:45 on the coordinator's pointer); I-0113 RESULT's final table; I-0103 CENSUS; ROADMAP; I-0132 R11-PLAN §6; the tour's
`cutscenes-compare.md`, `yavg-t3d.tsv` and two of its `sheets-t3m/`. The coordinator's note of 22:34 (relayed, not yet in a
record) gives tonight's facts marked « coordinator » below.

Statuses as in the [ledger](../../knowledge/LEDGER.md): OBSERVED (a picture, journal or measure), VERIFIED (code or disassembly
read), INFERRED (a reading not witnessed). Owners: **R** (RT/PT, lighting, fog, exposure, post: ROADMAP 7, 11, 12), **E** (effects,
draws, natively missing families, cutscene draws: ROADMAP 5), **T** (the tour, ROADMAP 6), **Thomas** (a decision or his eye).

## 0. Answer first

Most of what looks « odd » natively comes from four causes, not from fifty defects:

1. **The traced lighting without the original's ambient and fills** (D-0262): the level and colour of nearly every view
   (outdoors darker, the player a dark silhouette under the sky, fire-lit interiors warmer and brighter, the player's red-orange
   skin and dark trousers, the moss lost, flat skins in cutscenes). A decision, waiting for workstream 12 (the light sources).
2. **The automatic exposure that darkened the bright zones** (D-0264's half strength toward one target): 32000010's dusk,
   Archdragon Peak at 60 % of the original's level, very likely part of the tour's « outdoors darker ». Corrected by D-0280
   (tranche 5e written on R's branch, not built or merged).
3. **The map's fog applied in full natively** (coordinator; E's reading of 45000030): the cyan, grey or haze veils of 45000030/31,
   51000000/10, 32000010's last cuts, 31000010's wyverns lost in the haze, 40000040's uniform fog. With it two engine mechanisms
   the native does not follow: the cutscene act that overrides the in-game light and fog records, and the 3 s cross-fade of a
   zone's parameter files.
4. **The flames and local lights too strong** (INFERRED: the local lights x2 kept by D-0264, the flames as traced lights): the
   ending's body over-lit by the hand's flame, Yhorm's hall, the fire-lit interiors at 1.4 to 1.7 times the original's level (the
   candles of 38000000/10 read fixed on main by the tour tonight).

The characters' skin dark in cutscenes, which the tour reads on main tonight in 30000000, 30009000, 31000000, 31000010,
41000060, 41000070 and 45000000 (near-black or grey-blue where the original's is pale or tan), is filed by the tour with
tranche 1's light level: cause 1 seen on a face (INFERRED; those runs predate D-0279's kept lights).

Fixed since the older tour builds (the tour's re-run on main `d1fbc6f51`, tonight): the embers, the skin shading in most scenes,
the boss's specular, 32000010's dragon scales, the mountains, the stairs, the candles, 37000020's window light and fog.

Outside those four, the open list is short: the DLC arrivals (black frames, timeouts), gap 16 at 2560x1440 (1920x1080 alike by measure, I-0140 tranche 26; 17 closed by Thomas, 23 fixed under the TAA since, I-0140 tranche 12),
the characters' blood paint seen in both modes but not at one moment (step 2, tranche 35), the menus'
portrait cases, and the RT leftovers never looked at. Several tour verdicts are stale (old builds, unpaired shots): § 4.

## 1. The table, ranked by visible impact

« Seen » is where the gap shows; « Cause » its status; « State »: open, in progress, fixed (merged), written (on a branch,
not merged), decision, to re-check. A row's cluster (A to F) is § 2's.

### High: visible in most views or most cutscenes

| # | Gap | Seen | Cause (status) | State | Owner | Cl. |
|---|---|---|---|---|---|---|
| 1 | Outdoors darker than the original (mean luma 0.56 to 0.74 of it under the open sky: Archdragon Peak 0.57, Depths of the Painting 0.56, Cemetery of Ash 0.63, Road of Sacrifices 0.65) | the tour's 55 bonfires (I-0139 « Tranche 1 »), JPEG screen captures | two causes mixed: no sky ambient on the scene (D-0262, VERIFIED in the shaders) and the automatic exposure darkening a bright zone (row 3, OBSERVED at Archdragon Peak: 60 % → 84 % of the original without it) | to re-measure after D-0280 | R (7 step 4, 12) | A, B |
| 2 | The character a dark silhouette under the open sky; in cutscenes on main a dark, near-black or grey-blue skin where the original's is pale (30000000, 30009000, 31000000, 31000010, 41000060, 41000070, 45000000) | the tour's tranche 1 and its re-run on main (`c64ba7f07`) | the sky's light on him is missing: no ambient but the traced bounces (D-0262; INFERRED for the cutscenes) | decision; workstream 12 | R, Thomas | A |
| 3 | The automatic exposure darkens the zones the game makes bright (by 1 to 1.75 stops: 32000010's end « at dusk », Archdragon Peak in play 62 against 103 of 255) | I-0141 « 32000010 at paired instants », the fourth witness (`i141-t1d`) | OBSERVED: D-0264's half strength toward one game-wide target, below the zone's own exposure | **written**: D-0280 (Thomas 22:16), I-0138 tranche 5e on R's branch (`a2ffd0942`), to build, witness, merge | R (11) | B |
| 4 | The player red-orange (skin) with dark trousers where the original keeps a cold grey ambient, in play and before and after cutscenes; the Road of Sacrifices' moss lost the same way; Father Ariandel's limbs dark and flat (the tour's relay: their G-buffer pale and whole, their lit HDR dark, I-0140 « Father Ariandel ») | every zone with a warm light; `i140-e11`, `i140-e19`, `i140-e71` | OBSERVED: the G-buffer's material colour neutral pale (trousers) and green-grey (moss), the lit HDR red-dominant: the lighting's, not a material (D-0262) | decision; workstream 12 | R | A |
| 5 | Fire-lit interiors brighter and warmer (Shared Grave 1.70, Distant Manor 1.67, Abandoned Tomb 1.64, Corvian Settlement 1.64, Great Belfry 1.59, Church of Yorshka 1.58) | the tour's tranche 1 | INFERRED: the flames' traced light without the cool ambient that balances it, and the local lights x2 (D-0264) | open; workstream 12 | R | A, D |
| 6 | Cutscene veils: a uniform light cyan fog over 45000030/31's first views (luma 98-105 against 23-42), a haze in 51000000 at 30 s and in 51000010, a grey veil over 32000010's arena at paired instants even without the automatic exposure, 31000010's wyverns lost in the haze, 40000040's uniform fog behind the player | the tour (`sheets-t3d`, `sheets-t3c`), I-0141's fourth witness, `i140-e12`, `i140-e20` | the map's fog record applied in full natively (coordinator, 2026-11-14; E: `{145000000, 0}` start 12 m, the hue matches); candidate co-cause: the cutscene act's override of the fog records (row 9) | open; R's next | R (7 step 7, 12) | C |
| 7 | The arrivals in the DLC: all 7 of Ariandel's rows, 4 of the Ringed City's, the Dreg Heap's Z66 and Z68 and the base game's Abyss Watchers released by the timeout with the map's instances at 0; Ariandel 334 frames black; at Abyss Watchers the room past the bonfire's arch black natively, lit in the original | I-0139 tranche 1; ROADMAP 7 step 1 | not established (700 objects selected by the game, 7 admitted natively at Ariandel) | open | R (7 step 1) | — |
| 8 | Cutscenes « brighter and flatter » in dark scenes (most of the 49 compared), the dark original's contrast lost | `cutscenes-compare.md`, `yavg-t3d.tsv` (medians 1.14 to 1.89 in the Painted World's scenes) | INFERRED: rows 2, 5 and 6 together (no ambient, strong local lights, the fog veil); the cutscenes' own lights 1 to 3 kept since D-0279 add to it (Yhorm brighter) | open; to re-read on paired moments after D-0280 | R | A, C, D |

### Medium: visible in some zones, cutscenes or transitions

| # | Gap | Seen | Cause (status) | State | Owner | Cl. |
|---|---|---|---|---|---|---|
| 9 | A cutscene that overrides the in-game light and fog records (the Remo act `IngameDrawParam`, slot 9, on the `MapOverride_forRemo` layer) keeps the map's own natively | none isolated; first suspect for row 6's remaining veil | VERIFIED in the decompile (I-0141 « Read for the next tranches » 2): the native's banks hold no file, the writes land where nothing reads; whether a shipped scene uses it: being observed (R's `light-observe` hooks on 31A20, 380A0, 320B0, 387D0, `cbc2a4a58`) | in progress | R (I-0141 tranche 2) | C |
| 10 | A zone's parameter files switch at once natively where the original cross-fades them (3 s at 32000010's start): a pop of the sun, the fog and the post settings at a boss phase or a storm | 32000010 (OBSERVED in the original's `reference_sun_block`: six steps 8.5 to 30) | VERIFIED: `Banks::SetAreaLayers` ignores the binding's weight; the original's layer table (STUDY-LAYER-FADE, a worker's reading to re-read) | open | R (I-0141 next) | C |
| 11 | Distant scenery in cutscenes: 31000010's distant mountains greyer, their snow less textured (on main); its far valley a grey band where the original's castle and forest are lit (`i140-e12`, before tranche 7) | the tour on main (`c64ba7f07`), `i140-e12` | INFERRED (E, tranche 5): drawn and textured, lying in the one traced sun's shadow where the original lights them with their own draw groups' light sets (`[131000000, 300]`) and its cascades stop at 10 to 60 m; the mountains and 32000010's dragon read fixed on main since (tranche 7's sun rotation, INFERRED) | open, smaller | R (7 step 7) | E |
| 12 | Flames and fire light too strong: the ending's body brighter (YAVG 194 against 165; the hand's flame); 30000010's chapel glow (old build; not checkable on main) | I-0140 « Next » 1, `cutscenes-compare.md` | INFERRED: the traced local light of a flame and the x2 of D-0264 | open (the candles read fixed on main) | R (7 step 7, 12) | D |
| 13 | The player's head a flat blue-grey silhouette, only the hair strands' outline, no face or hair texture (32000000 at 50 s, the kneeling close-up) | the tour on main, its full-size still checked by its pilot (`c64ba7f07`) | not read; the family of the white faces I-0140 tranche 1 fixed (a FaceGen owner or texture missing in one view, INFERRED) or the lighting's (A) | **new**, open | E (5) | F |
| 13b | The smoke over the temple dark grey-black where the original's is pale white fog (32000010 at 40 s, matched moments) | the tour on main | not read; an effect lit by the traced light without the ambient (A) or the fog (C), INFERRED | **new**, open | R or E | A, C |
| 14 | The cutscene's grading LUT and a cutscene key whose file lacks a group (GlareQuality, DoF quality, MotionBlur ratio from `default` and not `mapbase`) | 26 of 49 cutscene files enable a LUT; 17, 46, 48 lack Setting Info, MotionBlur, Anti-Alias | VERIFIED and served (I-0138 tranche 6, K-0495, K-0496, merged `1def62857`) | **fixed, not witnessed** in a cutscene | R (11) | — |
| 15 | Mist and glowing haze missing: 34010000's white mist and bloom at 50 s (old build; not checkable on main, no matched view) | `cutscenes-compare.md` | not established: a timing artefact (§ 4), the light shafts' effect family 10003 (row 21) or the bloom; 37000020/21's window light and fog read fixed on main | to re-check at a matched moment | T | C |
| 15b | The tour's uncertain cases on main, to look at again at matched moments: 30000000 (the white wisp at 30 s; a flat grey slab at 50 s), 34010000 (smeared specular at 50 s; the knight's armour orange at 5 s), 35000050 (the white orb on the hand), 38000020 (the skull's red or blue tint), 39000010 (the hair silver-grey; the dragon brown), 45000000 (Friede's eye glow green, the original's golden) | the tour (`c64ba7f07`), UNSURE by its worker | — | to re-check | T | — |
| 16 | The fast travel's body fade soft (a double image) under the TAA where the original dissolves in hard dots (gap 23) | Thomas, 2026-11-08 | OBSERVED: the TAA's resolve averages the dissolve, not the jitter or the mip bias; its 3x3 reconstruction OBSERVED, the history's last tenth INFERRED (I-0140 tranche 12) | fixed under the TAA by a fade mask (I-0140 tranche 12; the merged gate witnessed in `i140-e70`); soft under DLSS/DLAA (no such input); Thomas's eye | E (5 step 4) | F |
| 17 | Fire Orb's explosion ringed with magenta and cyan (gap 16: after I-0113 tranche 33's fix, tranche 35's 73 to 121 magenta pixels a whole film at 640x360 and 5 frames/s, none in the original) | Archdragon Peak | at 1920x1080 the rim of the same order in both modes on the effects' clock, every frame (peaks 107 and 21 natively, 52 in the original; I-0140 tranche 26; the earlier metric not comparable); what removed the residual not established | alike at 1920x1080 by the pilot's measure (four casts a mode, DLAA), Thomas's eye pending; open at 2560x1440, not compared since the fixes | E | F |
| 18 | The menus' portrait cases: the Quit Game portrait's hair, face and blacker background (Thomas, 2026-11-08); the Dreg Heap character's missing Load Game portrait (not reproduced in 20 openings) | menus | Alpha's Quit Game portrait: background and hair's mean alike, the face a little darker and warmer natively (I-0140 tranche 26); Delta's as Alpha's; Thomas's character's armour black natively, blue-silver as the original's since the menu views' region pass (D-0301, I-0140 tranche 28: its Quit Game pane YAVG 41.51 natively against 41.23, 30.54 before, OBSERVED, MEASURED); quits from the Dreg Heap and Firelink, every portrait there (I-0140 tranche 27) | open: the equipment, Status, Face Detail, creation and Load Game views with the region pass to compare (tranche 28's left); the Status body's picture sharper natively with stepped edges, the original's smooth (I-0140 tranche 38: gradient +35 %, MEASURED; tranche 39: the original's own DLAA on its menu views, READ and MEASURED; D-0305: our TAA or DLSS on them next) and the Status portrait's picture 2 px left of the original's in the same frame (tranche 38, MEASURED); gap 3's Ringed City quit; the warmer face and the hair's edge for Thomas's eye (1:1 sheet) | E (5 step 4) | — |
| 19 | Spells, infusions, buffs, statuses and projectiles never audited one by one (miracles incomplete; sorceries, pyromancies, Dark spells, Black Flame, every infusion and status uncertain) | FINISH-LINE, Thomas 2026-10-28 | the spells 106 of 106 cast, 104 drawn alike (56 by a Sonnet triage, INFERRED), none seen differing, 2 not compared (I-0140 tranches 13 to 16); the eight resins alike; poison, toxic, frost, curse alike; the 118 melee weapon arts used, none differing by a luma screen, Yhorm's Great Machete darker natively; the shields', catalysts', torches' and Stance arts (but two not used) and the goods' use alike but the Crucifix of the Mad King's violet swirl missing natively, intermittent in the original (I-0140 tranches 17 and 18) | the leads drawn alike or closed by Thomas's eye (tranches 19 to 24, the last 2026-11-20); left: Thomas's eye on the strips (D-0181), the blood spurt denser natively at 100 ms (tranche 24); Yhorm's machete and the hits' flashes R's | E (5 step 5) | F |

### Low: a family, a corner, a setting or an effect seldom met

| # | Gap | Cause (status) | State | Owner | Cl. |
|---|---|---|---|---|---|
| 20 | Crystal Magic Weapon's crystals a little paler (gap 17) | measured with the rays (I-0140 tranche 11, `i140-e32`): the crystals' saturated pixels B/R 1.81 to 1.88 natively, 1.70 to 1.84 in the original, G/R 1.22 to 1.24 against 1.20 to 1.22, a little darker natively | closed (Thomas, 2026-11-15: the darker crystals D-0262's fill lights; the effects as light sources, workstream 12) | E (5 step 4) | F |
| 21 | The spot effect lights 11000 (m33, m45, `commoneffects`): the map's spot path, native by its mechanism (m45's two window spots in the light list at Friede's arena, I-0140 tranche 27; identity INFERRED from place); the light shafts 10003 (m30, m33, m38) native since I-0140 tranche 10 (`i140-e31`), their departures: PS s1 the stage's anisotropic wrap (the bank's linear wrap in the original), the scene view's description assumed R16G16B16A16_FLOAT (only its size read), C4F10's 2D virtual extent the scene's size (the block overwritten before any read), a scissor dropped in pass 0x31's half-size layer (no shaft seen there) | m45's two spots OBSERVED in the light list, their identity INFERRED (I-0140 RESULT « Tranche 27 »); 10003 VERIFIED drawn (I-0140 RESULT « Tranche 10 ») | 11000 native; 10003 its brightness to Thomas's eye | E (5 step 1) | F |
| 22 | Effect pass 0x20 recorded, never drawn | VERIFIED; never met in 412 journals since pass 0x21 is drawn, the DLC's night endurance included (I-0140 tranche 24) | **closed**, no case | E (5 step 1) | F |
| 23 | The mist's own lower contrast (Firelink: spread 24-33 natively without the TAA, 23-32 with it, against 31-48, 2026-11-11) | OBSERVED then; on the Firelink wall's mist drawn under `ffx-observe` (the module of `0860cbc8c`), alike at the same effect age (luma, spread, gradient; I-0140 tranche 25); in play the arch has no mist natively by Thomas's choice (I-0113 gap 24) | **closed** by the pilot's measure on the Firelink wall's mist only (no other mist looked at), the sheets for Thomas's eye | E (5 step 4) | F |
| 24 | Firelink's slight blue veil (the sky seen by the GI rays through the open roof) | OBSERVED, left as it is by Thomas (I-0126) | decision; its source in workstream 12 | R (12) | C |
| 25 | The characters' blood and damage paint: t41/t42 the receiver's paint target for a bloody provider (`08_model_programmes.inc` 1208, I-0105), the neutral decal before its first paint | VERIFIED in the code; the paint runs natively (`native_paint_drain`), its receiver not named in the rows; on Andre struck seven times no paint seen in either mode (I-0140 tranche 24); on a struck player kept alive by F4, dark marks on the back in both modes (tranche 35, OBSERVED), natively far more of the back covered, the fights not at the same moments (D-0299) | open: a scripted hit at a fixed moment to compare colour and extent | E (5 step 2) | F |
| 26 | Forward materials whose MTD blend mode is neither 0 nor 2 refused (`13b_prepare_model_frame.inc` 525; tranche 6 changed the G-buffer rows only) | VERIFIED in the code until I-0113 tranche 10b; no refusal in 2,550 journals since (I-0140 tranche 24) | **closed** | E | F |
| 27 | No water interaction map (`g_WaterInteractTexture` grey, `08_model_programmes.inc` 1090): no ripple where a character walks | VERIFIED; but no shipped MTD selects an Interaction family (STUDY-UNREAD-SETTINGS item 9) | likely not a gap; to confirm | E | — |
| 28 | The burn MTDs' parameters (burning objects): read natively since I-0105 tranche 3; their four objects placed by no MSB part (I-0140 RESULT « Next » 5, I-0063's scan) | VERIFIED reader (I-0105); no placement OBSERVED in the scan | no witness possible (a Remo cutscene or an event spawning them not read) | E (5 step 3) | F |
| 29 | The translucent NPC phantom; one summoned-phantom residual; single-model NPC phantoms | WAVE1-LAST-EFFECTS §4, §8; the single-model path runs natively (READ), no visible case | open, no witness | E (5 step 3) | F |
| 30 | GX00 Decal overlays (eyebrows, map overlays) | class and blend fixed (tranche 6, `51df978b8`) | **fixed**, a view for Thomas's eye left | E | F |
| 31 | Families without a material in the rays (EmiGlow, the skinned `o000250`, `Col`, `Veill_Curtain`, the eyes, BloodBlend, Sand, the Dreg Heap's pink family, ...) | I-0119; seen family by family | open | R (7 step 3) | A |
| 32 | Never looked at in the zones' tour: Ray Reconstruction's local-light noise, the lit cumulus, a size off 16:9, NRD 0.8 steps darker than RR | R11-PLAN §6 | open | R (7 step 4) | — |
| 33 | Emissive surfaces not light sources (the Smouldering Lake's lava, crystals) | by design until workstream 12 | planned | R (12) | A |
| 34 | `eclipse.gparam` never served (area 7 has no file) | VERIFIED; when the engine keys area 7 unread | open, likely unmet | R | C |
| 35 | Projected-mask spot lights 0x69/0x70 and line lights (type 107) | the raster comment (`07_scene_lighting.inc` 91) says « no tile path »; whether R4a's one world list (I-0122) carries them: not read in this study | to re-check | R | A |
| 36 | The cutscenes' chromatic aberration (four records of three files) | not drawn natively yet | planned: I-0138 tranche 7 (D-0276) | R (11) | — |

### Not the picture, listed so nothing is lost

The follow camera's drift at 240 and the cadence witnesses never taken (ROADMAP 8); gap 27's k held at 1 under 60 (an exception to
D-0244); the Twin Princes, Wolnir, the Nameless King, the Gravetender and the Wyvern never reached by the tour's tools; the Fire
Keeper's dialogue and the bonfire menu not reached (I-0139 tranche 4); the menu tests in other languages and resolutions (ROADMAP 14).

## 2. Shared causes and duplicates

- **A, the ambient (D-0262).** Rows 1 (in part), 2, 4, 5 (in part), 8, 13b (INFERRED), 31, 33, 35 and STUDY-UNREAD-SETTINGS item 4 are one
  decision seen from different places. Filed several times under different names: the tour's « level departs both ways » (I-0139
  tranche 1, ROADMAP 7 step 4 or 12), ROADMAP 7 step 7's « player lit warmer and darker », the tour's « player wrong after a scene »
  (withdrawn 10:30, the same thing), E's « moss » (`i140-e19`), STUDY-UNREAD-SETTINGS item 4. One owner, one cure: workstream 12.
  Thomas's own words are on record (memory 2026-11-09: no visible gain over the raster; the light sources first).
- **B, the exposure.** Rows 3 and part of 1. The tour's « outdoors darker » numbers predate D-0280: they mix A and B, so they need one
  re-read after tranche 5e before anyone files A's share.
- **C, the fog and the parameter layers.** Rows 6, 9, 10, 13b (perhaps), 15 (perhaps), 24, 34. 45000030's cyan veil, 51000000's haze, 51000010,
  32000010's remaining veil, 31000010's wyverns and 40000040's fog are one family (E said so for 45000030 and 31000010; the
  coordinator for the four first). Rows 9 and 10 are the two engine mechanisms that shape a cutscene's fog over time; the native
  follows neither, so the fog fix should come with them, not as a per-scene value.
- **D, the flames.** Rows 5, 8 (in part) and 12: one INFERRED cause (the flames' traced light, the x2), to read on one interior
  (Shared Grave or a candle scene) before a change.
- **E, the traced sun in cutscenes.** Row 11, with D-0279's provisional rule B (the cutscene's own lights kept) and E's tranche 7
  (the sun's source the original's). The same question as row 2 seen from the scene's far side.
- **F, the effects and draws.** Rows 13, 16, 17, 19 to 23, 25 to 30: E's workstream 5, each its own cause.

Duplicates to merge in the records: ROADMAP 7 step 7 holds rows 2, 4, 6 (31000010's wyverns), 11 and 12 in one paragraph; I-0140
« Next » 1 and 2 repeat rows 11 and 12; the tour's tranche-1 filing and ROADMAP 7 step 4 both hold row 1.

## 3. Fixed or settled tonight (in the inventory as such)

- The game's own anti-aliasing filter removed (I-0138 tranche 5c, D-0278) and the cutscenes' variant B (a cutscene's own lights 1 to
  3 kept, D-0279; Siegward lit in 39000020) merged `ccde3618a`; main `6e6226939`.
- The automatic exposure's floor at the zone's own exposure: D-0280, written on R's branch (row 3).
- 51000000's angels: drawn natively (`i140-e20`: c6250 admitted at full opacity); the tour's « absent » was a shot of another moment.
- Earlier the same day, already in `main`: the white faces in cutscenes and the IngameDrawParam fault (I-0140 tranche 1), the ending's
  and the cutscene player's embers (tranche 3), the cutscene's sun source and rotation (tranche 7), the boss transition cutscenes'
  fault (I-0137 tranche 7). The tour's re-run of 24 scenes on main reads most of the older builds' cutscene defects fixed (§ 0) and
  30000030, 30000040, 35000000/01, 37000020/21/30, 38000000/10 clean beyond the exposure.

## 4. Stale or unsafe verdicts (re-read before acting)

1. **Shots at equal seconds are not the same moment**: the native's scene starts later after the hand-over (about 2 s in 32000010,
   about 10 s in 37000020 tonight, OBSERVED on the sheets; INFERRED elsewhere, not measured). Every « missing » in a cutscene of
   `cutscenes-compare.md` (the knight in 33000000, the stairs in 38000020, the fallen knight in 39000010, the white frame in 32000000,
   the mist in 34010000 and 37000020) may be a moment, not a missing object. Whether the late start is itself a defect (a longer
   black at the scene's start) is open: the cutscene load natively against the original's, measured once.
2. `cutscenes-compare.md`'s first 21 scenes were judged on `build-tour5/6` (before I-0138 part 4b, the faces and the embers): its
   rows are superseded by the tour's re-read on main (`cutscenes-compare-main.md`, `c64ba7f07`). That re-run's `build-tour10`
   predates D-0279 and D-0280, and its originals are the ASUS's windowed captures (its luma table marked not valid by the tour).
3. The tour's tranche-1 luma ratios are JPEG screen captures (a level, not a colour), before D-0280.
4. STUDY-NOT-YET-NATIVE (2026-11-01) is partly stale: the GPU particles' « zero SH » does not hold (`i140-e20`: ones), the forward IBL's
   black cube matters less since the rays light the forward rows (R9), the blood's fade is native (I-0105).

## 5. What would settle the open rows (cheapest first)

1. After tranche 5e merges: re-measure the tour's outdoor views and 32000010 (rows 1, 3, 8): what is left is A's share.
2. The tour's uncertain cases (row 15b) and 34010000 at a matched moment; 32000000's flat head (row 13) witnessed by E with the face
   observers of I-0140 tranche 1.
3. R's `light-observe` rows on the cutscene act (row 9) in 45000030 and 32000010, then the fog rule with the layer fade (row 10).
4. One interior (Shared Grave) and one candle scene (38000000) read for the flames' share of the light (rows 5, 12) before workstream
   12 decides the x2.
5. E's gap 16 witness (`build-e39`), then the spells' audit (row 19), which will also meet rows 21, 22, 25, 28.

## 6. Limits

- No new measure was taken: the numbers are the records'. The cutscene verdicts on main are the tour's (a Sonnet worker's by eye on
  reduced sheets, one still checked by its pilot), not this study's.
- The coordinator's « fog applied in full » is relayed, not yet in a record; E's own reading of 45000030 is INFERRED.
- The owners follow ROADMAP; the pilots decide.
