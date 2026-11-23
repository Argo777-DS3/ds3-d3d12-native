# I-0139 sheet: endings, credits and the DLC arrivals (rows F01..F07, D01..D08)

Written by a Sonnet 5.5 worker from SHEETS-BRIEF; non-canonical until the pilot checks it.

**Covers.** The endings of the game and the credits (F), the arrivals in Ashes of Ariandel and The Ringed City (D).

**Sources.**
- EMEVD dumps read for this sheet, `evidence/I-0139/artifacts/static/endings-dlc/event_{m35_00,m40_00,m41_00,m45_00,m50_00,m51_00,m51_01,common,common_func}*.txt`. They were made with `investigations/I-0067-static-menus-subsystems-map/Get-EmevdDump.ps1` from the game bytes another worker extracted under `C:/Users/User/Tools/DS3-Decompilation/gamedata-i0138/bosses/`, read only. This worker extracted no new bytes.
- `investigations/I-0069-static-menu-audio-map/ENDING-CREDITS-NATIVE.md` (called ECN), `REMO-CUTSCENES-NATIVE.md`, and `evidence/I-0069/queries/credits/` and `remo/`.
- sl2-analyzer `db_ds3/endings.json` (CURATED). `lord_cinders.json` holds only the four Lords' kill bits: no ending data. `questlines.json` holds item bits only, no ending conditions.
- I-0132 RESULT « R11c and R11b », R11-PLAN §12, `evidence/I-0066/artifacts/i132-r11c0-eta-tabs/go-12..14.png` (Eta's Travel tabs, read by eye), `.project/LIVE-RUN-NOTES.md`, `investigations/I-0089-test-tools/USAGE.md`, and `investigations/I-0089-test-tools/boss-flags.json` (flag 14100800 = Soul of Cinder, 15000800 = Dreg Heap map boss, 15100800/15100850 = m51_00, 15110800 = Slave Knight Gael; its confidence is « inferred », not read from the game).

**How to read the « reach » column.**
- **Test launcher** = `Launch-DS3-DX12-Test.ps1 -Shebang` (or `-Bosses`), F4 invincible, F5 revive, F6 kill the strongest enemy within 25 m.
- **Eta** holds every bonfire (R11-PLAN §12). Travel from Eta: `Run-AlphaCast -Continue -BonfirePages -TravelPress "<tabX>,191;<rowX>,<rowY>"`; rows start at y 280, 33 px apart, row 1 at x 400.
- **`witness-flag.txt`** beside the module holds `id value`. It sets or clears one event flag through the game's own setter (`ApplyWitnessFlag`, `native/frame/game_bootstrap/04_updates_and_test_tools.inc`; ECN §5 R5; `Run-AlphaCast -SetFlag "20 1" -Casts 0`). One id per file, and the file is removed once read.
- **Not a cutscene player.** A forced flag skips the cutscene, because the EMEVD sets the flag after its Remo cutscene. No native run of an ending cutscene is recorded (REMO-CUTSCENES-NATIVE verdict 3: no native run has played a cutscene).
- **Test characters only.** Run only on Eta, Gamma or the other Shebang characters, or on a copy of Alpha (D-0116 hash-checked restore, offline, announced).

**What stayed OPEN.**
- Which of the four ending flags (20 to 23) belongs to which named ending.
- The exact conditions (Fire Keeper's eyes, Untended Graves, Firekeeper summon sign, Yuria's questline) are not in the EMEVD read. They live in ESD talk scripts and in item and flag tests of other events, which were not read.
- The real Cathedral to Ariandel trigger position and the Dreg Heap entrance's prerequisite flags.

## Table 1: endings and credits (F)

All four ending names are CURATED (`endings.json`: save-record offset 1375, bits 7, 6, 5, 4 for Link, Usurpation, End of Fire, Unkindled). The EMEVD structure is STATIC and the same for every row (the Kiln, `m41_00_00_00`):

- **Event 14100100** (dump lines 36 to 108): waits for the Soul of Cinder's death (flag 14101100). The interactable entity 4101100 offers an action prompt (text 9341, button id 10012020). Then Remo cutscene **41000060** plays, flag **22** is set, **41000070** plays, flag **23** is set (110, 6400, 9922, 9923 alongside).
- **Event 14100101** (lines 109 to 151): the same wait. Entity 4101952, enabled at the boss's death by event 14100800, offers a prompt (text 9342, id 10012021). **41000000** plays and flag **20** is set (with 9920); then **41000050** and **41000051** play and flag **21** is set (with 9921).
- Flags 20 to 23 are consumed by `FUN_1408F5930` (ECN §1): kind 1 for flags 20 and 22, kind 2 for 21 and 23. Then `EndingStep` plays the credits.
- Which branch is which named ending is OPEN. INFERRED from structure only: two distinct interactables (a first-flame asset and a second entity), each with two cutscene stages.

| Row | Name | Map | What to look at | How to reach it with the test launcher | Source and status |
|---|---|---|---|---|---|
| F01 | Ending « To Link the First Flame » (Link the Fire; the default) | m41_00 Kiln of the First Flame | The Remo cutscene chain, fade plates 3/4, then the credits. Conditions: none known beyond the Soul of Cinder's death (flag 14101100); named flag conditions OPEN. | Load an Eta-based save. Travel tab 863 (Kiln), row 1 (INFERRED; row names not read). Revive the Soul of Cinder with `test-revive.txt` holding `14100800` (or `-Bosses`), enter the arena, F4, F6 (0 HP). Walk to the prompt (entity 4101100 or 4101952; which is Link is OPEN) and press E. To skip the cutscene and go straight to the credits: `witness-flag.txt` = `20 1` (or `22 1`). | Name and bit: CURATED. Events and cutscene ids: STATIC (`event_m41_00_00_00.txt`). Branch-to-ending mapping: OPEN. Credits by forced flag: RECORD (ECN §5 R5, Native2065). |
| F02 | Ending « The Usurpation of Fire » | m41_00 | Its own cutscene chain. Conditions given in the brief and OPEN here: Fire Keeper's eyes, the Untended Graves (m40_00), the Firekeeper's summon sign, Yuria's questline (`questlines.json` lists Yuria's item bits only). | As F01. To skip a branch: `witness-flag.txt` = `21 1` or `23 1`. The conditions can be set only once their flag ids are read (smallest tool: an ESD reader for Yuria and the Fire Keeper). | Name and bit: CURATED. Which flag: OPEN. Conditions: OPEN (brief only). |
| F03 | Ending « The End of Fire » | m41_00 | The ending reached by not linking. Its variant is the next row. | Same Kiln arrival; which of the four flags it ends on: OPEN. | CURATED name and bit; mapping and trigger OPEN. The brief's « end of fire and its variant » is read as F03 and F04. |
| F04 | Ending « Unkindled Ending » (variant) | m41_00 | A fourth sl2 ending bit (4). Its cutscene is not identified. | As F03. | CURATED name and bit only. |
| F05 | The credits | not a map: movie page over the title flow | The Bink movie `fdp_endroll_ww.bik` (movie id 3, INFERRED from the table `{2,3,3,3}`; 13,002 frames at 30000/1001 fps = 433.8 s), drawn by the native Bink player. Native pages matched Reference2064 at 10, 30, 60, 120, 240 and 420 s. **2026-11-15 on main `d1fbc6f51` (I-0139 RESULT « Tranche 5 »): natively a « MOVIE » placeholder plate at every shot from 10 to 420 s, the original the credits: a regression, filed to workstream 16.** | **Forced flag only, no ending played.** `witness-flag.txt` = `20 1` in a loaded world (`-SetFlag "20 1" -Casts 0`), run at least 520 s. It exercises the real chain: `FUN_1408F5930`, then `EndingStep` (title-flow state 6). The skip key closes it. | STATIC (code, ECN §0, §1). Native chain: RECORD (ECN §5 R5: Native2065, 2026-10-18, exit 0, faults 0). Movie id and file: INFERRED. Fades around it: none drawn natively (ECN §6). |
| F06 | After the credits: the « Begin journey 2 in current state? » (NG+) dialog | title flow | The dialog seen at 480 and 540 s, identical in native and Reference. `DAT_14489F200` selects C510 or C570 (the play-through counter); which runs is OPEN (ECN row CR5). | As F05, wait 480 to 540 s. | RECORD (ECN §5 Native2065); C510 or C570 and the files written: OPEN. |
| F07 | Flag 30 (state 8 of the title-flow step) | m40_00 Firelink, event 14005480 | `m40_00` event 14005480 sets flag 30; the title-flow state 8 behind it was not read. | `witness-flag.txt` = `30 1` (never run). | STATIC for the flag; the effect is OPEN (ECN §9 item 1). |

## Table 2: the DLC arrivals (D)

Ariandel is DLC1 (m45), the Dreg Heap is m50_00 and the Ringed City is m51_00 and m51_01 (DLC2). Known native problems, from I-0132 RESULT « R11c and R11b » and ROADMAP workstream 7 step 1:
- Ariandel holds its 5 s black arrival with the map not yet in the draw lists: 334 frames black on the new rule. DS3's culler selected 700 objects while the native made 7 model intents; the cause is not established. Four `nonfinite_placement` skips.
- The Ringed City's arrival times out (6.7 and 6.8 s). Its map pieces reach the draw lists only seconds after the first world frames (map instances 0, then 2,027); INFERRED that the original shows its fade there (not filmed).
- The Dreg Heap was seen before but not revisited.

Travel tabs (pilot units; first row at (400, 280), read from `go-12..14.png`):

| Tab x | Zone | Rows |
|---|---|---|
| 923 | The Painted World of Ariandel | Snowfield, Rope Bridge Cave, Corvian Settlement, Snowy Mountain Pass, Ariandel Chapel, Sister Friede, Depths of the Painting, Champion's Gravetender |
| 983 | The Dreg Heap | The Dreg Heap, Earthen Peak Ruins, Within the Earthen Peak Ruins, The Demon Prince |
| 1043 | The Ringed City | Mausoleum Lookout, Ringed Inner Wall, Ringed City Streets, Shared Grave, Church of Filianore, Darkeater Midir, Filianore's Rest, Slave Knight Gael |

R11c travelled to « ariandel » (Snowfield) and « ringed » (the Ringed City's streets, row 3 at y 346 INFERRED); the tour's exact press strings are not in the repository logs.

| Row | Name | Map | What to look at | How to reach it with the test launcher | Source and status |
|---|---|---|---|---|---|
| D01 | Entrance into Ashes of Ariandel (from the base game) | m35_00 Cathedral of the Deep, event 13505100 | The event waits for the player in region 3500725 (`3[3] 257,10000,3500725`), then plays Remo cutscene **35000050**, or **35000051** by a branch, with a map move to region **4502100** in area 45 (`2002[4] 35000050,0,4502100,45,10000`). It then sets flags 14500001 and 73500304. | Real entrance replay: not reachable with today's tools. Needs the position of region 3500725 and the prerequisite flags (OPEN). Smallest tool: an MSB region reader for m35_00 (SoulsFormats, as `Get-MsbGparamSlots.ps1`; m35's MSB was not extracted) plus `-Teleport x y z hold` to it. Fallback: D02. | STATIC (`event_m35_00_00_00.txt` lines 471 to 495; `evidence/I-0069/queries/remo/emevd-cutscene-calls.txt`). Position, prerequisites, the 35000051 branch: OPEN. |
| D02 | Arrival in the Painted World of Ariandel: first bonfire « Snowfield » | m45_00 (DLC1) | A snowfield under fog with the bonfire in a white valley (Travel preview, `go-12.png`). Problems: the 5 s black arrival, 334 black frames, culler 700 vs native 7. | Eta, `-TravelPress "923,191;400,280"`; `-LookSweep "4,-300,2"` as the tour did; 150 s for the views. Test saves `eta-r11c-ariandel` exist (sha256 in `runs-r11c1.log`, `runs-r11c3.log`). | Tab and row: RECORD (`go-12.png`, R11-PLAN §12). Arrival problems: RECORD (I-0132 RESULT). Flags: OPEN. |
| D03 | Ariandel's exit to the Dreg Heap | m45_00, event 14505160 | The event waits for flags 14500161 (common event 9020, `1,9020,14500161,14500162,14500162,6952,45`) and 9322 (set in m45's boss-death handling next to flag 14500800, line 1961). It enables entity 4501101 with an action prompt (text 9344) and warps with `2003[14] 50,5002110`: map area 50, region 5002110 = the Dreg Heap. | Not reachable as a real route (needs the DLC1 boss progress). Skip the route with D05. Flags one id per file: `witness-flag.txt` = `9322 1`, then `14500161 1` (never run). | STATIC (`event_m45_00_00_00.txt` lines 482 to 509; `event_common.txt` line 139). That this is the in-game exit of the DLC1 ending is INFERRED. |
| D04 | The Dreg Heap's entrance from the Kiln | m41_00, event 14105510 (common event 9020 `1,9020,14100511,14100512,14100512,6952,41`) | In the Kiln, a prompt on entity 4101102 (text 9344) triggers `2003[14] 50,5002110` (the Dreg Heap) after flag 14100511. The meaning of 14100511, 14100512 and 6952 is OPEN. | Real route: kill the Soul of Cinder (F01's steps), then the prompt (which flag unlocks it: OPEN). Fallback: D05. | STATIC (`event_m41_00_00_00.txt` lines 161 to 182; `event_common.txt` line 138). Prerequisite flags: OPEN. |
| D05 | Arrival in the Dreg Heap: first bonfire « The Dreg Heap » | m50_00 (DLC2) | A ruined dump with a pale arch (Travel preview, `go-13.png`); DS3's own map. Not revisited since R11c (STATE also lists the Dreg Heap character's missing Load Game portrait, not reproduced). | Eta, `-TravelPress "983,191;400,280"`. | Tab and rows: RECORD (`go-13.png`). RT state: not measured (I-0132 « seen before, not visited »). |
| D06 | The Ringed City's entrance from the Dreg Heap | m50_00, event 15005100 | After the map's boss (flag 15000800, « Demon Prince »: boss-flags.json, unknown confidence) and an action prompt on entity 5001100 (text 9380), Remo cutscene **51000000** plays with a map move to region **5102110**, area 51. | Real route: revive the boss (`test-revive.txt` `15000800`, as for the Iudex in LIVE-RUN-NOTES), F6, then the prompt. The warp is the event; a flag cannot replace it. Fallback: D07. | STATIC (`event_m50_00_00_00.txt` lines 241 to 254). The prompt's position: OPEN. |
| D07 | Arrival in The Ringed City: first bonfire « Mausoleum Lookout » | m51_00 (DLC2) | A golden-yellow view of the Ringed City's tower and the lookout bonfire (`go-14.png`). Streets are row 3. | Eta, `-TravelPress "1043,191;400,280"` (Mausoleum Lookout); `"1043,191;400,346"` for Ringed City Streets. The tour's Ringed travel timed out at 6.7 s (0 map instances during the hold, 2,025 after). | Tab and rows: RECORD (`go-14.png`). Arrival problem: RECORD (I-0132 RESULT, R11b's second tranche). |
| D08 | The Ringed City's second part (Slave Knight Gael's map) | m51_00 to m51_01, event 15105500 | After the m51_00 boss (flag 15100500) and a prompt on entity 5100890 (text 9390): cutscene **51010000**, move to region **5112110**, area 307 (m51_01). Flag 15110001 set. | Eta, Travel tab 1043, row 8 « Slave Knight Gael » (400, 511). | STATIC (`event_m51_00_00_00.txt` lines 1280 to 1295). Row position INFERRED from the 33 px spacing; first-frame state not measured. |

## Gaps (what the tools cannot reach today, and the smallest tool)

1. **The endings' named mapping and conditions.** A reader for the ESD talk scripts (the Fire Keeper, Yuria, the Kiln's two interactables) plus the flag tests in the common events, to name each branch and its flags. Only then can the preconditions be set with `witness-flag.txt` (one id per file; a small loop script would write them in turn).
2. **A cutscene player.** A forced flag skips the Remo cutscene; replaying 41000060, 41000070, 41000000, 41000050 and 41000051 natively needs a witness that calls the game's cutscene start (not built, REMO-CUTSCENES-NATIVE §7). Today a real ending is reachable only by killing the Soul of Cinder and pressing the prompt.
3. **The real DLC entrances.** The positions of m35_00 region 3500725 and of the m41 and m50 prompt entities are not extracted (m35_00's MSB is not in `gamedata-i0138/bosses`). Smallest tool: a SoulsFormats region and asset reader plus `-Teleport`. Travel (tabs 923, 983, 1043) reaches the arrivals without it.
4. **The Kiln's travel row and the Dreg Heap's prerequisite flags** (D04) were not read.
5. **The credits' fades and the post-credits C510 or C570 branch** stay OPEN (ECN §9).
