# I-0139 result: the tour of the whole game

## Conclusion

Closed on 2026-11-16 as far as the tools the tour made and the recipes tried reach; the rows left are named in « Suggested next
step » with what each needs, and carried by ROADMAP workstream 16. Every run is a reference (D-0284, [REFERENCE.md](REFERENCE.md));
the closed list is [LIST.md](LIST.md). Over 433 runs the native renderer faulted on six (four at the bosses' transition cutscenes,
fixed by I-0137 tranche 7; Friede's intro 45000000 and 30000000 by id on `build-tour4` and `build-tour5`, the IngameDrawParam family
fixed by I-0140), the original playing each; on `build-tour6` and later no native fault on any row reached. Beside those, native and
original behaved the same wherever both ran, but one divergence after a death left OPEN (D06, below); its other differences are
pictures, filed or relayed:
- Tranche 1, the 55 bonfires R11c had not visited: no fault, no skipped family; the light-level family filed.
- Tranche 2, the bosses: every boss reached by its own arrival, alike; deaths read by their defeat flags where F6 reaches them (the
  Gravetender and his wolf alike; Friede's flag set after two F6 kills, her first two phases not shown). Not reached, alike: the Twin
  Princes' fight after their scene (the player back at the bonfire, souls kept) and the Nameless King's death on foot (the player
  died despite F4).
- Tranche 3, the 59 scenes by id: 58 played natively and in the original (45000090 not); in the game's own request form, 35000050
  by its event (D01), alike; Friede's intro by her event not reached. Relayed: 32000000's flat head, 32000010's dark smoke, the
  characters' dark skin, Father Ariandel's limbs dark without their glow natively.
- Tranche 4: the menus, the bonfire menu's page (every travel run's `bonfire.png`), Ludleth, the Handmaid, Andre and the Fire Keeper
  (her line and her menu) alike; at 17 other NPC places nobody answered in either renderer (OPEN whether they stand there in Eta);
  at Anor Londo and Dragon Barracks the point was not reached (both renderers died after the teleport).
- Tranche 5: the credits' native « MOVIE » plate found here was fixed by I-0140 tranche 9; the NG+ dialog alike. The DLC: D01
  (Ariandel, by Gael's scene) and D04 (the Kiln to the Dreg Heap) by their own events, alike; D03 and D06 not reached; D05 and D07
  by travel (tranche 1), D02 by R11c's visit only, D08's map by travel but not its entrance event 15105500.

## The runner's validation (2026-11-12 12:11-12:19, the ASUS, offline)

One row (Z17 Halfway Fortress) through [Run-I139Tour.ps1](Run-I139Tour.ps1) on `evidence/I-0139/build-tour1` (main `8f415270`, no
native change on this branch at that time), log `evidence/I-0139/artifacts/runs-v1.log`; announced at 12:10 for 12:12 on the coordinator's go (two
minutes ahead, not five: said here).

- Native (`i139-v1-z17-halfway-nat`): no fault, no unserved frame, no failed walk; the arrival ready after 3.6 s; skipped: four
  `nonfinite_placement` objects and the Firelink load's common rows (R11c); the bonfire kept as `eta-i139-z17-halfway-20261112-121502`
  (SHA-256 5A64C906BC895499AF33DDFE86D75E2FA616775219A5A90B87458CC13FAD0342); restored.
- The original (`i139-v1-z17-halfway-orig`): the same travel and look sweep, the same views; its pictures all taken, then an access
  violation at the close at `DarkSoulsIII.exe+0xF14C52` (stack `FUN_141FD0EAC`, `FUN_141A3B670`, `FUN_141A39FC0`, `FUN_1417FD480`):
  the exit fault ROADMAP workstream 4 step 5 already lists as unattributed, seen here in the Reference mode (the original renderer,
  our module's pacer and world-gate hooks only). Restored.
- The pictures (JPEG screen captures, by eye): one view, the native lit by the bonfire's traced light where the original keeps its
  grey ambient; the character's trousers darker natively, present (R11c's « no ambient but the bounces », D-0262).

## The boss runner's first trial (2026-11-12 12:24-12:35, the ASUS, offline)

Iudex Gundyr through [Run-I139Boss.ps1](Run-I139Boss.ps1), log `evidence/I-0139/artifacts/runs-b1.log`, natively then in the original
(`i139-b1-b01-iudex-nat`, `-orig`); both clean (no fault, no crash, restored by hash). The tools worked in both modes: `test_revive`
cleared 14000800, F4 (`test_invincible`, 669 HP), F6 (`test_kill`: 1,054 HP written to 0 among 103 characters). **The fog gate was not
crossed**: the teleport (125.4 -63.9 557.8) put the character inside the fog, then the look sweep turned the camera about 110° to the
wall and E found no prompt; F6 killed the boss in his arena with the player outside. The teleport also pinned the player for its 20 s hold, the E and
the steps inside it. Next trial: a 6 s hold, the steps after it, no sweep after the teleport, a 300 ms
step forward (the character turns to the camera, which faces the fog after the teleport), then E. Pictures by eye: the native warmer
and darker where the original keeps a cool grey ambient (the walls, the trousers), as at Halfway Fortress (D-0262).

## The fog gate, the phase and the death (2026-11-12 12:40-12:46, the ASUS, offline)

The same row natively only (`i139-b2-b01-iudex-nat`, log `runs-b2.log`, clean, restored), the 6 s hold, a 300 ms step, E: the prompt
« Traverse the fog », the crossing, Iudex Gundyr's health bar and his fight under F4 (OBSERVED, `step-2.png` to `step-5-30.png`).
F6 (`test_kill`, 1,054 HP written to 0) did not kill him: his second phase started (the black mass out of his back, `step-7-30.png`),
the bar full again. So F6 twice gives a two-phase boss's phase and then its death; the runner's default steps now press it twice
(F4, a step, E, 30 s, F6, 45 s, F6, 45 s).

## Tranche 1, first half (2026-11-12 20:00-23:39, the ASUS, offline)

28 bonfire rows (`tranche1-half1.txt`, Z01-Z42), each natively then in the original, on `build-tour1` (main `8f415270`; main moved
to `9119c900` during the day: the tour's tree is the morning's), log `evidence/I-0139/artifacts/runs-t1a.log`; journals read by a
Sonnet worker (`journals-t1a.md`, checked by the pilot on Z23's rows); pair sheets `pairs-t1/<row>.png` (the native's arrival and
four looks above the original's); mean luma `yavg-t1a.tsv` (`tools/yavg.py`, JPEG screen captures: a level, not a colour).

- **Faults**: none. 56 runs exited cleanly (no crash, no fault, no dump), 56 restorations by hash, 28 bonfires kept as
  `eta-i139-<row>` test saves; every original's run had `test_no_ai`. No unserved (magenta) frame, no failed walk or BLAS, no pass
  off.
- **Skipped families**: none beyond the Firelink load's rows common to every run; `nonfinite_placement` objects (skipped by design)
  at Z02, Z19, Z24, Z26, Z33 (4 each) and Z30 (1).
- **Arrivals**: ready after 3.2 to 9.8 s (Irithyll and Anor Londo the slowest, 5.8 to 9.8 s, with High Lord Wolnir's 7.3 s); **Z23 Abyss Watchers timed out**
  (6.8 s, the map's instances 0 at the release), the rule R11c added (D-0161) releasing it as for the Ringed City.
- **Pictures** (by eye and by mean luma, native/original): the same views, no surface family missing or wrong by eye; the native
  darker outdoors under the sky (Cemetery of Ash 0.63, Road of Sacrifices 0.65, Iudex Gundyr 0.74), brighter and warmer in the
  bonfire-lit interiors (Distant Manor 1.67, Abandoned Tomb 1.64, Church of Yorshka 1.58, Water Reserve 1.43), where the original
  keeps a cool grey ambient (R11c's observation; D-0262: no ambient but the traced bounces). **At Z23 the room beyond the
  bonfire's arch is black natively where the original shows it lit** (`arrive.png`; the row whose arrival timed out): a candidate
  defect on JPEG screen captures only: the run's lossless world pictures stop at frame 4,500, before the travel (Firelink).
- Z01 (Firelink Shrine) is Eta's own bonfire: the travel page greys it, so no travel happened; Firelink is seen at every run's load.

## Tranche 1, second half and the last two rows (2026-11-13 00:08-03:38, the ASUS, offline)

27 rows Z45-Z75 (`tranche1-half2.txt`; started at 00:08, not 23:45: the session restarted at the first half's end; Z74 and Z75 after
it, Thomas's go for a later end), logs `runs-t1b.log` and `runs-t1c.log`, journals `journals-t1b.md` (a Sonnet worker, checked by the
pilot on Z62), pairs `pairs-t1/`, luma `yavg-t1b.tsv`. Z70 is not on Eta's page (the zones sheet).

- **Faults**: none natively (27 runs). The original closed on an access violation at `DarkSoulsIII.exe+0xF14C52` once more, after its
  pictures (`i139-t1-z74-filianore-s-rest-orig`, the stack of the validation's): 2 of the 57 original runs of tranche 1 and its validation.
- **Skipped families**: none beyond the common rows; `nonfinite_placement` x4 at Z59, Z68, Z71.
- **Arrivals**: the base game's all ready (2.6 to 6.8 s); **every Ariandel row timed out** (Z57-Z63, 6.75-6.81 s, the map's
  instances 0), the Dreg Heap's Z66 and Z68 and the Ringed City's Z69 and Z71 too; the other DLC rows ready (3.3 to 6.4 s).
- **Pictures**: the same views; no family missing by eye. The native's level against the original's (mean luma): darker under the
  open sky (Archdragon Peak 0.57, Depths of the Painting 0.56, the Champion's Gravetender 0.57, Earthen Peak Ruins 0.68), where the
  **character is a dark silhouette natively** (Depths of the Painting, Archdragon Peak, Cemetery of Ash: the sky's light missing on
  him); brighter and warmer in the fire-lit interiors (Shared Grave 1.70, Corvian Settlement 1.64, Great Belfry 1.59).

## Tranche 1: the verdict and the defects filed

55 bonfires visited natively and in the original (with R11c's 20, all 75 of Eta's page, Z01 by the load); no native fault, no skipped
family, no unserved frame. Filed (to the coordinator, 2026-11-13):

- **ROADMAP workstream 7, step 1 (the arrivals)**: 12 of 55 arrivals released by the timeout with the map's instances at 0 (all seven
  of Ariandel's, four of the Ringed City's DLC, Abyss Watchers in the base game), and at Abyss Watchers the room past the bonfire's
  arch black natively, lit in the original (a candidate on screen captures).
- **ROADMAP workstream 7 (step 4) or 12 (the light)**: the native's level departs from the original's both ways, outdoors darker
  (0.56 to 0.74 under the open sky) with the character a dark silhouette, interiors lit by fires brighter and warmer (1.4 to 1.7);
  the RT level (`native_rt_level` `rt`, one bounce), the default.
- **Not filed** (Thomas, 2026-11-13 03:47, pointing at I-0124): the exit fault at `+0xF14C52` in the original's runs is DS3's own
  exit-order defect (its FMOD sound thread outliving the static heap), which I-0124 fixed by stopping the sound at the quit, a hook
  installed in the native mode only (`22_setup.inc`, inside `Mode::NativeBootstrap`); the Reference mode runs without it, so its two
  faults in 58 are the original's, not ours (main's ROADMAP workstream 4 has since attributed both F14C52 paths).

## Tranche 2, the bosses' first night (2026-11-13 03:38-05:47, the ASUS, offline)

[Run-I139Boss.ps1](Run-I139Boss.ps1) over `tranche2-night1.txt` (25 rows; stopped by its `-StopAt` before the Nameless King), log
`runs-t2a.log`: 11 bosses natively then in the original, 22 restorations by hash. F6 (`test_kill`, HP written to 0), the same in
both modes: Iudex 1,054 then 1,037 (his phase, then his death), Friede 4,863 (her first phase only: she has three, Thomas),
Dancer 5,111 then 121, the Dragonslayer Armour 1,510 then 1,178, Vordt's run 579 and 190 (other characters: where the teleport put
him is to read). The pictures, the phases and the deaths are not yet read row by row.

- **A native fault, twice**: at Friede (`i139-t2-b22-friede-nat`) and at the Abyss Watchers (`i139-t2-b05-watchers-nat`), seconds
  after the first F6 started the boss's next phase, the native closed on a refusal (0xD3126606, `native_render_job_failure`
  « current native model UI overlay », `02_frame_lifecycle.inc` 564/575: a world frame with no UI packet while the UI build is
  current, thrown on the render worker); the original went through the same moments. Filed to ROADMAP workstream 4 (E), the
  coordinator told at 05:50. **The moment is a boss cutscene** (Thomas, 05:52): the original's picture 10 s after the F6 at the
  Abyss Watchers (`i139-t2-b05-watchers-orig/step-6-10.png`) has no HUD and films the corpses, the phase-2 cutscene; Friede's phase has
  one too (45000010, 12 cuts). INFERRED: a Remo cutscene hides the interface, the game sends no UI packet while the native's UI build
  stays current, and the worker throws; if so, every in-game cutscene closes the native (no native run had played one,
  REMO-CUTSCENES-NATIVE), told to the coordinator at 05:55.
- **The prediction, written before the remaining bosses run** (Thomas, 2026-11-13 05:54): only Friede and the Abyss Watchers have a
  phase-transition cutscene among the bosses run; among the fourteen left, the Nameless King, the Twin Princes and Gael have one. If
  the fault is the cutscene's, those three close the native at their transition and the others do not.
- The morning's runs (2026-11-13 07:01-08:57, `runs-t2b.log` to `runs-t2d.log`, the series paused three times for R and E): the
  Nameless King does not appear on a revive alone (the Great Belfry's bell flag 13200445, m32 event 13205860: run again with it); the
  Greatwood reached by a second teleport near the tree after the fog (Thomas's way: cross the fog, then teleport on the boss), F6 5,405
  HP; Wolnir never reached (F6 found no one; Thomas: the chalice teleports the player); the Old Demon King 5,301, Oceiros 8,087 HP.
  **The Twin Princes: the player died after the teleport in both modes** (a fall INFERRED: the second point, the boss's MSB position; « YOU
  DIED » and the loading screen after it drawn natively, two menu rows first seen); their transition cutscene was not reached, to run
  again without the second point. No fault in these 12 runs.
- The last runs (09:12-11:51, `runs-t2e.log`, 22 restorations by hash): the Wyvern (F6 only on 589 and 522 HP characters: it dies
  by a plunge attack), Champion Gundyr (4,956 then 1,037: INFERRED: the second F6 took a 1,037 HP character, Iudex's second form's HP, so maybe not Champion Gundyr's phase), the Soul of Cinder (1,222, to read), the Champion's
  Gravetender (no one within 25 m natively), the Demon Prince (2,323, 2,323, 1,900, 808), Halflight (2,006), Midir (888, to read),
  Gael, Friede again, the Twin Princes without the second point and the Nameless King with the bell flag (both: F6 found no one).
- **The prediction's outcome**: Gael's native run closed on the same fault (`i139-t2e-b26-gael-nat`, 0xD3126607, « current native
  model UI overlay ») seconds after his first F6, and Friede's again (`i139-t2e-b22-friede-nat`); the originals went through.
  Over tranche 2 the fault fell at every reached boss with a phase-transition cutscene (the Abyss Watchers, Friede twice, Gael) and
  at no other (20 bosses reached or tried without it); the Twin Princes and the Nameless King were not reached, so untested.
- The unreached bosses read on their pictures (a Sonnet worker, `evidence/I-0139/artifacts/unreached-bosses.md`; the pilot checked
  the journal of `i139-t2e-b16-princes-nat`: the teleport applied, then the player was back at the bonfire, a fall INFERRED): new arrivals in
  `retry-unreached.txt` (the Twin Princes at their arena's player start `c0000_0004`, Vordt at his `c0000_0004`, the Gravetender's
  second point at `c0000_0013` in place of a start region 19 m below, Wolnir at the chalice inferred at 355.2 -268.4 -724.0); the
  Nameless King (region 3202855 with the bell flag) and the Wyvern (a plunge) left for later.
- **E's fix checked** (2026-11-13 12:33-12:55, `build-tour2`: this branch with main `63b113d9` merged, I-0137 tranche 7; log
  `runs-v2.log`, natively only): the Abyss Watchers, Friede and Gael go through their transition cutscenes with no fault, three
  restorations; their fights go on after them (F6: the Watchers 1,548, 712, 362; Friede 4,863 then 7,150; Gael 5,584 then 15,037).
  The first Remo cutscenes played natively (OBSERVED on the screen captures, `sheets-v2/watchers-gael-transitions.png`: the Watchers'
  risen corpse in flames, Gael's with its subtitle).
- The unreached bosses again (13:10-13:56, `build-tour2`, `runs-t2f.log`, 8 restorations, no fault): **Vordt reached** from his
  arena's player start (F6 1,328 HP, both modes); the Twin Princes (their arena's start), the Gravetender (`c0000_0013`) and Wolnir
  (the inferred chalice) still not (F6 found no one): they need their events read (the Princes' and Wolnir's arrivals) or a piloted
  approach.
**Tranche 2's verdict**: 25 bosses tried natively and in the original, 64 runs (70 with the trials), no fault in the original's, one native fault family
(the boss transition cutscenes, filed to ROADMAP workstream 4, with E's D126B0 detour as the second candidate cause); not reached by
the tools: Wolnir (the chalice), the Twin Princes, the Nameless King, the Gravetender, the Wyvern (a plunge); Vordt reached on 2026-11-13
at 13:10.
- A second candidate (E, 2026-11-13 08:08, relayed): main since E's tranche 4 declared the detour of D126B0 with two arguments
  instead of four, which touches the player's Remo cutscene actors; a cutscene fault on `build-tour1` may come from it as well as
  from the UI overlay's throw. The two are told apart by the journal's failure row (the throw's message) against an access violation.
- The other nine bosses' native runs closed cleanly; the original's runs all clean.

## Tranche 2, Wolnir and the Twin Princes by their arrivals (2026-11-15 11:19-11:50, the ASUS, offline)

The arrivals read from the event scripts (`evidence/I-0139/artifacts/static/bosses/arrivals-wolnir-princes.md`, a Sonnet worker, its
key lines checked by the pilot in the m38 and m34_01 dumps): Wolnir by the object 3801810 (`o381053_7000`, 360.1 -273.9 -702.9;
event 13805809: the action button, text 9322, then cutscene 38000000 and the warp into the arena region 3802802), not by the fog
3801800 tried before; the Twin Princes by their fog 3411830 (event 13415833), the fight started by flag 13415835 (event 13415831:
cutscene 34010000, then the bosses enabled). Runs `i139-t2g-b08-wolnir-nat/orig` and `i139-t2g-b16-princes-nat/orig`
(`Run-I139Boss`, rows `tranche2-wolnir.txt` and `tranche2-princes.txt` with `-ExtraFlags` clearing 13800801, 13805805, 13801800, and
13410831, 13415835, 13415836), `build-tour11` (main `61b905572`), log `runs-b3.log`, sheets `sheets-t2g/`: exit 0, no fault, restored.
OBSERVED: Wolnir reached in both renderers (E at the object, the arena, « High Lord Wolnir »'s health bar; F6 wrote 0 to 15,041 HP
in both, natively the player then back at the return point); the Twin Princes' fog crossed and their scene 34010000 playing in
both (« Oh dear, another dogged contender »); F6 natively killed a 5,799 HP character, in the original found none within 25 m
during the shots (the scene's length against the step times: the fight's phases not reached; INFERRED). Left: the Nameless King,
the Gravetender, the Wyvern's plunge; the Princes' fight with later F6 rounds.

## Tranche 2, the Nameless King, the Gravetender and the Wyvern by their fogs (2026-11-15 12:06-12:53, the ASUS, offline)

Arrivals read from the event scripts (`evidence/I-0139/artifacts/static/bosses/arrivals-namelessking-gravetender-wyvern.md`, a Sonnet
worker): each boss behind its fog (the Nameless King 3201850 with the bell flags 13200440/445/446 set; the Gravetender 4501860; the
Wyvern 3201800), the second point near the fight. Runs `i139-t2h-b18-namelessking`, `b21-gravetender`, `b17-wyvern`, native and
original (`Run-I139Boss`, rows `tranche2-namelessking.txt`, `tranche2-gravetender.txt`, `tranche2-wyvern.txt`), `build-tour11`, log
`runs-b4.log`, sheet `sheets-t2g/t2h.png`: exit 0, no fault, restored; F6's targets the same in both renderers (4,577 then 589 HP at
the Nameless King, 4,193 at the Gravetender, 7,873 then 589 twice at the Wyvern). OBSERVED alike in both: the Nameless King's
storm fight and his phase-change scene 32000010 (the King on foot), then the player back at the Archdragon Peak bonfire; the
Gravetender's health bar, the wolf entering after F6; the Wyvern killed (the souls up from 6,473,870 to about 6,545,370 in both).
Whether the King's second phase and the wolf died (their defeat flags) is not read; no native-only difference seen (the player's
light aside).

## The second phases and deaths by their flags, Friede by her event, the Fire Keeper (2026-11-15 19:47-21:02, the ASUS, offline)

The tool: `witness-flag-watch.txt` (`native/frame/game_bootstrap/04_updates_and_test_tools.inc`, ApplyWitnessFlagWatch, both modes):
up to 16 event flags sampled every 60th schedule call by the game's own lookup (the getter 1404C5350's block 1404C7F00 and bit,
OBSERVED in the disassembly), a row at the first sample and at each change, -1 where no block holds the flag; Run-I139Boss
`-WatchFlags` prints them. Two step reviews (below). [Run-I139Series6.ps1](Run-I139Series6.ps1), `build-tour12` (native `a52d24e47`),
log `runs-s6.log`, sheets `sheets-s6/`: ten runs, exit 0, no fault, restored. OBSERVED, alike in both renderers unless said:
- **The Twin Princes** (`i139-t2i-b16-princes`): the scene 34010000 played after the fog (a teleport, then T), then the player back
  at the Grand Archives' bonfire; F6 found nobody within 25 m in the five rounds the journal logs; the fight flag 13415835 and 13415830 set with 13410831
  at the scene's end, cleared again some 5 s later; the souls unchanged (6,473,870), so not a death (the final review's L3; why the
  player is back at the bonfire OPEN); 13410830 (the defeat) never set. The fight after the scene is not reached by this method (the arena's player start loses the player); the same in both.
- **The Nameless King** (`i139-t2i-b18-namelessking`): the King on his dragon, F6 4,577 then 589 HP twice in both, then the player died despite
  F4 (INFERRED from the souls, 6,473,870 to 1,500 at the bonfire, both, and the respawn; no « YOU DIED » shot) and reloaded (natively a loading screen at the moment the original
  shows the « Archdragon Peak » banner: the timing); 13205852 set
  then cleared (the phase scene's flag, INFERRED), 13200850 (the defeat) never set in either: his second phase's death not reached.
- **The Champion's Gravetender** (`i139-t2i-b21-gravetender`): F6 4,193, 2,791, then 491 HP three times natively; 4,193, 2,791,
  1,795, then 491 three times in the original (the F6 rounds met different characters); 14505861 and 14505865, 14505863, and the defeat
  flag 14500860 with 14500861 set in both (calls 13,800 natively, 13,980 in the original): **the Gravetender and his wolf die
  alike** (the outcome; the rounds' targets differ as above).
- **Sister Friede by her own event** (`i139-t3f-b22-friede-intro`, m45 event 14505810: 14500800 and 14500801 cleared, the talk flag
  74500300 set, E at Father Ariandel, entity 4500803): the intro 45000000 did not play in either (74500300 cleared by the game at the
  map's start, call 5,160: the event's talk condition not met; the game's own request form still not reached); the fight started
  anyway: F6 7,150 HP (Father Ariandel in his chair, no boss bar before it: `step-5-45.png`), then Blackflame Friede's bar and F6
  6,864 HP, and **the defeat flag 14500800 set in both** (calls 16,440 and 16,560). Her first two phases were not shown (the final
  review's M1): the flag set after two F6 kills on a revived arena, alike. **A native difference**: Father Ariandel's limbs, pale flesh with ember cracks in the
  original, are drawn as dark flaps without their glow natively, the whole run long (`sheets-s6/friede-ariandel.jpg`); relayed to
  E (workstream 5, the characters' dark skin family with a missing emissive: INFERRED, not read).
- **The Fire Keeper** (`i139-t4k-keeper`, `-TalkKeeper` on the m45 save, Alpha's copy): natively she stood beside the player, her
  first line subtitled, then her menu (Level Up, Give Eyes of a Fire Keeper, Give Fire Keeper Soul, Talk, Leave;
  `sheets-s6/keeper-6.jpg`, `keeper-20.jpg`); in the original she stood elsewhere (her place is random each load), no talk; run again below (alike).
- Journals: the rows common to every run (`native_asm_part_missing_*`, studied benign in I-0113, and the Firelink load's); at the
  Fire Keeper 60 `native_offscreen_hair_refused` rows (the menus' hair family, E's); no other refusal, no unserved frame, no
  `native_rt_fail_magenta`.

## The DLC entrances by their own events, the Fire Keeper in the original (2026-11-15 21:51-22:35, the ASUS, offline)

[Run-I139Dlc.ps1](Run-I139Dlc.ps1) (recipes in `evidence/I-0139/artifacts/static/endings-dlc/dlc-entrances.md`, a Sonnet worker; the
four events' lines checked by the pilot in the m35, m41, m45 and m50 dumps) and `Run-I139Series6 -Part keeper -KeeperModes
Reference -KeeperTag b`, `build-tour12`, log `runs-d.log` and `runs-s6.log`, sheets `sheets-s6/d0*.jpg`, `keeper-orig-b.jpg`: nine
runs, exit 0, no fault, restored. OBSERVED, alike in both renderers unless said:
- **D01, the Cathedral of the Deep into Ariandel** (`i139-t5d-d01-ariandel-entry`; 1623 set before the travel and cleared by the game
  at the map's load (calls 5,100 and 5,160), 73500339 set after it, the Cleansing Chapel's bonfire within m35 event 13505100's 10 m of
  Gael): **the event played the entrance scene 35000050 (1623 off; on, it plays 35000051) in the
  game's own request form** (2002[4] with the move to region 4502100: Gael on the floor, « Just a moment, then... », the painting
  scrap, « And then, the Ashes were two. »; natively the journal names `s35_00_0050` in `native_gparam_scene_binding_source` and
  `native_remo_override` 235000050, the original by its pictures) and the player arrived in Ariandel's first cave, in both renderers
  alike, the native player warmer and more orange (the light-level family; the teleport file not taken: the scene held it). The first of the scenes « in the game's own form » witnessed.
- **D04, the Kiln into the Dreg Heap** (`i139-t5d-d04-kiln-dregheap`; 14100511 set): the prompt « Travel to the Dreg Heap » at the
  object 4101102, E, and the warp to the Dreg Heap's first room in both, the native player warmer (the light-level family);
  14100512 then 14100510 set by the event (the flag tool).
- **D03, Ariandel's exit** (`i139-t5d-d03-ariandel-exit`; 9322, 14500800 and 14500161 set): the event armed (14500162 set by it, call
  5,220), but no prompt at the teleport point 2 m from the object 4501101, E did nothing, 14500160 never set, in both: **not
  reached** (the prompt's place or reach, OPEN).
- **D06, the Dreg Heap into the Ringed City** (`i139-t5d-d06-ringed-entry`): natively the Travel click took the High Wall tab
  (`go-1.png`; not run at its place natively here, run again below), in the original the Demon Prince's bonfire, then the teleport to the object 5001100 fell (no floor there)
  and the player reloaded: **not reached** in either (the goods 2156 the event tests also OPEN).
- **The Fire Keeper in the original** (`i139-t4kb-keeper-orig`): she stood beside the player this time; the same first line and
  the same menu as natively. The row is closed alike; natively her robe and the player warmer and browner (the light-level family).

## The final review's recipes with the existing tools (2026-11-15 23:40 to 2026-11-16 00:25, the ASUS, offline)

The final review's H1 named three rows reachable without a new tool. `Run-I139Series6 -Part friede` (prefix `i139-t3g`: the talk flag
74500300 set after the travel by `-AfterFlags`, then T to 1.5 m from Father Ariandel and E) and `Run-I139Dlc -Only` (prefix
`i139-t5e`: D06 with the goods 2156 given by F7, the point 1.5 m short of the object; D03 at 0.9 m on either side of the object in two
runs), `build-tour12`, logs `runs-s6.log` and `runs-d.log`, sheets `sheets-s6/friede-intro-g.jpg`, `d06e.jpg`, `d03b.jpg`: eight runs,
exit 0, no fault, restored. OBSERVED, alike in both renderers:
- **Friede's intro**: 74500300 held this time (set at calls 7,680 and 7,800, not cleared again), and at the arena's start a « Talk »
  prompt at Father Ariandel; after T the camera inside the bowl, E twice, no intro (14500801, which the intro event sets, stayed 0);
  then the same two F6 kills and 14500800 as before. **Not reached**: action 9373's reach, place or another condition (the talk
  script, unread) OPEN.
- **D06**: the goods 2156 given (`test_give_done`), the travel to the Demon Prince's bonfire right natively this time (the first try's
  High Wall tab was the click, not the place), the teleport point fell again (« YOU DIED », drawn natively), back at the bonfire:
  **not reached** (the object's floor unread). **A divergence, OPEN**: after the death natively the souls at 0 with the bloodstain's
  « Recover lost souls » and a green glow under the player, in the original 6,459,700 then 6,473,870 (the integrity review's M1; its
  cause unread, not counted alike).
- **D03**: from 0.9 m on either side of the object 4501101 no prompt, E did nothing, 14500160 never set (the event armed again,
  14500162 at about call 5,280): **not reached** (the prompt's object or its activation, OPEN).

## Tranche 4, the menus' first runs (2026-11-13 17:27-17:45, the ASUS, offline)

[Run-I139Menus.ps1](Run-I139Menus.ps1) on `build-tour3` (main `5fa7c0bd` merged), log `runs-t4a.log`, sheets `sheets-t4/` (the native's row
above the original's): six runs, all clean and restored. The System tabs (Game Options, Camera Options, Network, the others), the
Inventory and the Equipment pages and its lists: the same pages natively and in the original, no missing piece by eye (M rows of the
menus sheet now seen natively under the current main). The tours' clicks opened Equipment where they meant Status, in both modes (the
icons' x, the menus sheet's caveat). Not reached: the Fire Keeper's dialogue (she stands at a random spot; the walk ended at a wall in
both modes) and the bonfire menu (the rest prompt shown, the menu not opened).

## Tranche 3, the cutscene witnesses by id (2026-11-13 17:45-18:13, the ASUS, offline)

The tool `witness-cutscene.txt` (this branch, `04_updates_and_test_tools.inc` ApplyWitnessCutscene: the game's own handler 140474280 of
the EMEVD cutscene instruction, the player its entity, as 2002[3]; its disassembly `evidence/I-0139/artifacts/static/
handler-140474280-disasm.txt`, the static reading `remo-request.md`; a step review, three lows applied) and
[Run-I139Cutscenes.ps1](Run-I139Cutscenes.ps1) on `build-tour4`, log `runs-t3b.log`, sheets `sheets-t3/` (the native's row above the
original's). The first try by teleport to the triggers (16:19-16:42, `runs-t3a.log`) had shown no scene: Vordt's camera turned to the
ground in both modes, Friede's teleport undone by the game.

- **The tool works in both modes**: `witness_cutscene` « handed » for the three ids in the six runs, the scenes played (OBSERVED on
  the pictures).
- **C01, Vordt's intro `30000030`**: played natively with no fault; **the player's face drawn flat white natively** (no shading, the
  hair black) where the original shades it (`sheets-t3/vordt10.png`); the background a little sharper natively (the YEBIS study's
  depth of field refused, not measured here).
- **C30, the ending scene `41000000`**: played natively with no fault; **the face blown to white again** through the scene, the body
  brighter, and the embers burning on the character in the original not seen natively (`sheets-t3/t3b-ending.png`).
- **C31, Friede's intro `45000000`: the native closed on an access violation** in the scene's IngameDrawParam act (DarkSoulsIII.exe
  +0x9761E, stack 140097600 < 140038430 < 140CC8180, the act's slot 9, < the Ins' Play_Wait 140CD5D80; `i139-t3b-c31-friede-nat/
  crashes`); the original played it. Taken by E in I-0140 (its RESULT « The faults from the game tour's cutscenes »; the
  coordinator, 18:25; new native faults go to ROADMAP workstream 16 since workstream 4's close).
- **The request's form** (the final review's L2): the tool plays each scene as EMEVD 2002[3] with mode 0 from the bonfire the
  character travelled to; the game plays Friede's intro with 2002[4] (modes 0, 10, 2, a warp to region 4502810) and the ending with
  [3] mode 2. The original survived the same request, so the native diverges; whether a real playthrough meets the C31 fault or the
  white faces stays OPEN until a witness in the game's own form.
- The faces in the cutscenes (and the missing embers) go to ROADMAP workstream 5 (a family drawn wrong under the scenes' own light
  acts: INFERRED cause, the Remo lights or the face's material, not read); the depth of field to workstream 11 (the YEBIS study).

## Tranche 3, the cutscenes by id (from 2026-11-13 19:25, the ASUS, offline, paused for the other pilots' slots)

[Run-I139Cutscenes.ps1](Run-I139Cutscenes.ps1) over `tranche3-all.txt` (59 ids with a bonfire in their map), log `runs-t3c.log`; the
first six on `build-tour5` (main `d1f0d340`), the rest on `build-tour6` (main `e03ae881`, E's fixes of the IngameDrawParam fault and
the faces merged). A Sonnet worker's comparison of the first 21 (`cutscenes-compare.md`, sheets `sheets-t3c/<id>.png`, by eye on
reduced JPEG sheets; the pilot checked 31000010 and 32000010):

- **Faults**: one, 30000000 natively on `build-tour5` (the IngameDrawParam fault of Friede's intro, the family E fixed in I-0140);
  none on `build-tour6` so far. Every scene played in both modes (HUD gone, camera cuts), the subtitles drawn natively.
- **Pictures**: natively brighter and flatter in dark scenes throughout (the light's level, R's); and, **in several scenes, models
  drawn flat or unlit natively** (OBSERVED on 31000010: the distant mountains flat grey and untextured, the flag-bearer a flat tan
  silhouette, the white wyverns at 30 s missing; on 32000010: the dragon a black silhouette, black smoke over the temple where the
  original shows the player in the mist; the worker's list adds 30000000, 30000010, 30000030/40, 30009000, 31000000, 32000000,
  34010000, to read). **Corrected (2026-11-14 10:30, the pilot on a zoom, `sheets-t3c/31000010-player-zoom.png`): the player after a scene is not
  wrong**: the trousers are there natively, tinted red-orange like the skin by the orange sky, where the original keeps them pale;
  the same tint shows in play with no scene before it (41000010's run, where no scene played). It is the light's colour on the
  character (the traced light without the original's neutral ambient: the light's level filed with tranche 1), not a draw state left
  by a scene, as first written.
- **On main's builds** (from 2026-11-14 07:00: `build-tour7` main `a1a4138a` with R's 4b and 4c, the cutscenes' image settings read;
  `build-tour8` main `0b162a28`, `build-tour9` main `51df978b`; the coordinator's proposal, Thomas's yes): 46 of 59 scenes run by
  10:14, no fault; the worker's reading of the nine run on main (`cutscenes-compare.md`, « build: main »): Siegward flat grey-white
  without his shield's texture in 39000020, the background a uniform fog behind the player in 40000040, the flame on the player
  smaller in 40000020/21, the player flat and dark in 41000000/41000050 (the sky's tint: see the correction above); the 21 scenes
  with a defect on the older build (`tranche3-redo.txt`) to run again on main.
- **The OLED, 2026-11-14 12:17-19:19** (the only screen, the game in a 2560x1440 window, WORKFLOW « Live game rule »; the travel
  clicks hold at that size): 41000051 to 45000000 on `build-tour9`, then 45000010 to 51010005 on `build-tour10` (main `d1fbc6f51`
  with E's light-set rotation fix merged into the branch, `4de8945a`); 58 of 59 scenes run by 19:19 and 51010010 at 19:45-19:55
  (59 of 59), every run exit 0, no fault,
  every hand-over `handed`, every save restored. Sheets `sheets-t3d/` (`Make-I139CutsceneSheets.ps1`: native shots 5/15/30/60 above
  the original's), luma `yavg-t3d.tsv` (`tools/yavg_cutscene.py`). **Shots at equal seconds are not the same moment**: the native
  scene starts later after the hand-over (E's witness `i140-e20`, I-0140 RESULT « 51000000's angels »; INFERRED, not measured), so a
  thing « absent » at a second may be elsewhere in the scene: the 51000000 angels first read as absent are drawn (withdrawn
  19:05); a reading compares moments, not seconds. OBSERVED on `build-tour10`: in 45000030 and 45000031 (the Painted World's red
  flower field) the native's first views, the same pose and camera as the original's, under a uniform light cyan fog veil (luma
  98-105 natively against 23-42), routed by E to R's lighting and fog with 51000000's 30 s view; 45000090 does not play in either
  renderer (the player stays in Ariandel's snow with the HUD; not reached by this method), and that still view measures 91 natively
  against 149 (0.61), the zone's light level filed with tranche 1. On `build-tour9`, before E's fix: the player's light wrong in
  41000060 (median 0.59), 41000070 and 45000000; the three again natively on `build-tour10` (`tranche3-redo2.txt`, prefix
  `i139-t3n`). The Ringed City's last three, read on 2026-11-15 after the final review's M2 (`sheets-t3d/51010000.png`, `51010005`,
  `51010010`, `yavg-t3d.tsv`'s last rows): the same moments natively and in the original at 15, 30 and 60 s; the player dark at
  51010000's 5 s and 51010010's 60 s (the characters' light, as below); at 51010000's 5 s the sky also darker natively (the whole
  frame's luma 40 against 72).
- **The journals of tranches 3 to 5** (2026-11-15, after the final review's M3; every native `startup.jsonl` of `i139-t3c`, `t3m`,
  `t3n`, `t4*`, `t5`): beyond the rows common to every run since tranche 1 (`native_pass_refusals` gates 18/21/23,
  `native_selected_object_unsupported`, `native_asm_part_missing_*`): `native_ffx_skipped` renderer class `3D65A88` in the 2D pass
  once each in 30000010 and 38000020 (both builds): the light shafts' class (10003), its first witness in a journal (I-0105 RESULT
  had none), filed to workstream 5 through the coordinator; `native_body_scale_update_skipped` in 40000020/21 and 51010000 with
  `quit: true` (after DS3's quit, I-0137 tranche 8, by design); `nonfinite_placement` in 30009000 (by design); in the credits run one
  `native_object_skipped` `ui_import_asset_unavailable` (a lead for the « MOVIE » plate, not read). Also, found by the integrity
  review: `native_selected_texture_unavailable` (16 rows a run, `MultiBlend4_et1_snp_0_Texture2D_8`, the `MultiBlend4_et1_lightmap`
  family) in 25 runs, 32000000 and 32000010 among them (in 11 of tranche 1's 55 natives: not a common row; not read);
  `native_offscreen_hair_refused` (60 rows) in the menu and options runs; `native_menu_model_update_skipped` in the options run;
  `native_model_pose_nonfinite` (model o990000) beside 30009000's `nonfinite_placement`.
- **The 23 scenes again natively on `build-tour10`** (24 runs: 45000000 in both lists) (2026-11-14 19:55-22:35, the OLED; 21 of `tranche3-redo.txt` prefix `i139-t3m`,
  3 of `tranche3-redo2.txt` prefix `i139-t3n`): every run exit 0, no fault, `handed`, restored. Read against the earlier originals
  (`i139-t3c-*-orig`; those of 20 of the 21 on the ASUS, a 1280x720 window on the 1920x1080 desktop, so `yavg-t3m.tsv` is marked not
  valid: the wallpaper is in the originals' luma; those of 41000060, 41000070 and 45000000 on the OLED, so `yavg-t3m.tsv`'s 45000000 rows are valid) by a Sonnet worker matching moments, not seconds (`cutscenes-compare-main.md`, sheets `sheets-t3m/`,
  `sheets-t3n/`; reduced sheets plus full-size stills where named), the pilot checking 32000000's 50 s still. Most of the older
  build's defects read FIXED (the embers on the player, skin shading, the boss's specular, the dragon's scales, the mountains, the
  stairs, candles, window light and fog); clean beyond exposure: 30000040, 35000000/01, 37000020/21/30, 38000000/10; 30000030
  UNSURE (the original's dense fog at 20 s, none natively: timing?); 31000010's wyverns, missing on the older build, UNSURE (views
  not matched).
  **Still or new, scene by scene** (the moment, then what differs; UNSURE where the worker was):
  - 32000000, 50 s (the player kneeling close-up, full-size still checked by the pilot): the player's **head a flat blue-grey
    silhouette** with only the hair strands' outline, no face or hair texture; the skin flat grey-white.
  - 32000010, 40 s (the temple, matched to the original's 40 s): the smoke over the temple **dark grey-black** where the original's
    is pale white fog; the king's armour dark at 20 s (UNSURE).
  - The player's or a character's skin dark where the original's is pale (the character's light, filed with tranche 1's light
    level): 30000000 at 5 s (near black with red embers, the original tan), 30009000 at 5 s (grey-blue), 31000000 (the caged
    prisoner at 15 s, the body in the water at 40 s, dark brown), 31000010 at 5 s (the flag-bearer a dark silhouette; the
    foreground arm dark red), 41000060 at 5 and 10 s (a near-black silhouette; the summoned figure in the ring faint at 15 s),
    41000070 at 30 and 40 s (the kneeling player black; the fallen knight's face dark at 5 and 10 s), 45000000 at every second
    (tan to dark).
  - 30000000: the white wisp at 30 s not seen; at 50 s a flat grey untextured slab right of the player (UNSURE).
  - 31000010 at 5 s: the distant mountains greyer, their snow less textured than the original's.
  - UNSURE, to look again at matched moments: 34010000 (smeared specular on metal at 50 s; the kneeling knight's armour orange at
    5 s), 35000050 (the white orb on the hand at the original's 40 s unmatched), 38000020 (the skull without its red or blue tint at
    30-40 s), 39000010 (the player's hair silver-grey at 5-10 s; the dragon brown), 45000000 (Friede's eye glow green, the original's
    golden). NOT CHECKABLE (no matched view): 30000010 and 34010000 after 50 s.
- Filed: the models drawn flat or unlit in scenes to ROADMAP workstream 5 and E's I-0140 (the coordinator, 2026-11-14 02:20; the
  « player left wrong » withdrawn at 10:30).

## Tranche 4, the Firelink NPCs (2026-11-14 23:24-23:57, the OLED in a 2560x1440 window, offline)

The standing points (sheets/npcs-merchants.md « Firelink standing points », STATIC) visited by `Run-I139Npc.ps1` (one process per
mode and nine NPCs: Eta's travel, then per NPC Run-AlphaCast's `T<k>` with `-Teleport2` holding the nine points, the player 1.2 m off
along x, held 3 s, then E), `build-tour10`; runs `i139-t4-fl1-nat` (the trial) and `i139-t4b-fl1/fl2-nat/orig`, log `runs-t4n.log`,
sheets `sheets-t4/fl1.png`, `fl2.png` (per NPC: native after the teleport, native 5 s after E, then the original's two); every run
exit 0, no fault, restored; every teleport `applied` (journal). OBSERVED:
- **The method's faults, not the game's**: Eta stands at Firelink, so its travel to Firelink Shrine leaves the Travel list open
  (travel to the bonfire one is at): the trial's E then confirmed it, and in `fl2` the list stayed up through the first five NPCs
  in both renderers. The trial's second E opened the Handmaid's « Purchase item », which Esc does not close (LIVE-RUN-NOTES: some
  pages ignore Esc), so the trial talks only (`-PerNpc 'w2;69,0;w5'`). The player died after several teleports in both renderers
  (Ludleth's point, then repeatedly in `fl2`): a point 1.2 m off along x can be over a drop. Next: travel to Cemetery of Ash
  (row y 313, the same map) so the list closes, and an offset toward the floor the NPC stands on.
- **The NPCs reached**: the Shrine Handmaid (her menu Purchase Item, Sell Item, Talk, Leave) in both renderers alike; Blacksmith
  Andre reached alike (his « Talk » prompt in both; his menu not seen: the Handmaid's menu carried over the teleport); at Yuria's, Yoel's, Sirris's and Leonhard's points nobody in either renderer (absent in Eta, or the
  point not where they stand: OPEN). No native defect seen beyond the player's light (warmer and brighter natively at Andre's
  forge, the light-level family filed with tranche 1).
- **Again with the method fixed** (2026-11-15 00:50-01:14, runs `i139-t4c-fl1/fl2-nat/orig`, the travel to Cemetery of Ash, F4 first,
  `-Toward` the Firelink bonfire `270.8 -56.3 607.3`, talk only; sheets `sheets-t4/c-fl1.png`, `c-fl2.png`): exit 0, no fault, restored,
  no death. The Travel list closed; **Ludleth's « Examine » prompt and his menu (Begin Transposition, Leave) in both renderers
  alike**; at the Fire Keeper's point no prompt in either. A talk menu left open stays up across the next teleport (Ludleth's over
  the Handmaid's point), and from the Handmaid's point on the camera is left at a wild angle (a close-up of the player's face, then
  straight down or up at the ground for all of `fl2`) in both renderers alike, so `fl2`'s nine NPCs are not seen. Natively against
  the original: the same prompts, menus and views at every point; the differences are the player's and the walls' light (warmer
  and browner natively, bluer in the original: the light-level family). Next for the method: close the talk menu before the next
  teleport (its « Close » key, unread) and recentre the camera after each teleport.
- **`fl2` again** (2026-11-15 01:15-01:31, runs `i139-t4d-fl2-nat/orig`, `-PerNpc 'w2;81,0;w1;69,0;w4;38,0;69,0;w2'`: Q to recentre,
  E, Up to the menu's last row « Leave », E; sheet `sheets-t4/d-fl2.png`): exit 0, no fault, restored. The camera still looks straight
  down at the ground from the first point on, in both renderers alike (Q did not recentre it), so the nine NPCs stay unseen; the
  same views natively and in the original, the light's tint aside. The camera's angle after the travel and teleport is not
  understood (OPEN); the views agree between the renderers, which is the tour's question.
- **The places outside Firelink, natively** (2026-11-15 03:52-04:13, the OLED, runs `i139-t4p-<place>-nat`, `-Offset 0.8 -Lift 1.0`,
  Q, E, Up, E per NPC; log `runs-t4p.log`, sheet `sheets-t4/p-oled.png`): Prison Tower, Abyss Watchers, Irithyll, Rosaria's Bed
  Chamber, Aldrich; exit 0, no fault, restored. The camera behind the player at every point but the Prison Tower (a close-up of
  the head against a wall); no talk prompt or menu at any point natively; no original run there, so whether Yorshka, Hawkwood,
  Sirris, Leonhard and Patches stand there in Eta is OPEN (natively not drawn, or not there, or the point wrong); Rosaria's body on
  her bed in view, no prompt from that point. Paused for the other pilots; the OLED then turned off: the 14 others on the ASUS, natively and in the
  original.
- **Five more places on the ASUS** (2026-11-15 04:47-05:26, runs `i139-t4q-<place>-nat/orig`, 1920x1080, the same steps; sheet
  `sheets-t4/q-asus1.png`): Crystal Sage, Halfway Fortress, Church of Yorshka, Anor Londo, Shared Grave; exit 0, no fault,
  restored. No talk prompt or menu at any point in either renderer, the same in both (whether Orbeck, Anri, Horace and Patches
  stand there in Eta, or the points are wrong: OPEN); at Anor Londo the point was not reached in either (both died after the
  teleport, back at the bonfire with « Recover lost souls »): the Old Woman of Londor not reached. Natively the same places and poses as the original; the light differs (warm sunlit at the Crystal Sage point
  natively, cooler in the original; the Shared Grave brighter natively: the automatic exposure brightens dark views by design,
  D-0280).
- **The last nine places and the five OLED places again, on the ASUS** (2026-11-15 06:27-08:18, runs `i139-t4q-<place>` for Distant
  Manor, Cleansing Chapel, Ariandel Chapel, Earthen Peak Ruins, Ringed Inner Wall, the Dreg Heap, Dragon Barracks, Old Wolf of
  Farron, Water Reserve, and `i139-t4r-<place>` for the five places first run natively only on the OLED, each native then original;
  sheets `sheets-t4/q-asus2.png`, `r-asus.png`): 28 runs, exit 0, no fault, restored. No talk prompt or menu at any NPC or altar
  point in either renderer, the same views in both (the light aside: warmer natively); at the Dreg Heap the point holds an item, «
  Old Woman's Ashes » picked up by E in both renderers (the copy of Eta restored after); at Dragon Barracks (the Altar of Sunlight)
  the point was not reached (both died after the teleport); at Earthen Peak Ruins the camera looked up at the debris in both; at
  Rosaria's Bed Chamber her body in view, no prompt, in both. Whether the NPCs stand there in Eta or the points are wrong stays
  OPEN; no native-only behaviour at any of the 19 places.
- **Tranche 4 so far**: no native fault, no native-only behaviour at an NPC; in Eta at Firelink, Ludleth, the Handmaid and Andre
  answer, the others' points were empty or unseen. The places outside Firelink that Eta's travel reaches: `tranche4-places.txt`
  (19 rows, all run: above and below).

## Tranche 5, the credits by the forced flag (2026-11-15 01:54-02:18, the OLED in a 2560x1440 window, offline)

`Run-I139Credits.ps1` (Eta continued, `witness-flag.txt` « 20 1 », shots 10 to 600 s), `build-tour10` (main `d1fbc6f51`); runs
`i139-t5-flag20-nat/orig`, log `runs-t5.log`, sheet `sheets-t5/credits.png` (native left of the original at each second): exit 0, no
fault, restored. OBSERVED:
- **Natively the credits movie is not shown**: at 10, 60, 120, 240 and 420 s the whole window is a blue plate with a pink
  hand-written « MOVIE » (full-size `flag-240.png`), where the original shows the DARK SOULS III title and the credits' names.
  The native player ran the whole film: `native_movie_open` 1920x1080, 13,002 frames, `movie_witness` progress to frame 12,960,
  `native_movie_close` drawn 13,001, dropped 0, `published` true, the proxy bound (`native_movie_target` resident); every
  `native_movie_frame` row says `pixels: false` (that flag is the pixel-dump switch, so INFERRED only: not a proof the frames
  were not written). INFERRED: the plate is a placeholder texture the movie's surface shows when the decoded frames do not reach
  it; the code has no such picture (no « MOVIE » drawing in `native/`). ECN (I-0069) had the native pages matching the original
  at 10 to 420 s: a regression since, build unknown. The original at 10 s shows the DARK SOULS III title, at 30 s the
  « Programmers » page; natively the plate at both.
- After the film, the « Begin journey 2 in current state? » dialog (YES / NO) at 450 to 600 s, the same natively and in the
  original.
- Filed: the credits' native movie plate to ROADMAP workstream 16 (new native faults) through the coordinator (2026-11-15 02:25).

## The full control of 2026-11-16 (D-0230), run by the tour's thread

Thomas (2026-11-15 19:12, through the coordinator): the tour runs the full control once it is done. Main `644902ff7` (this close
merged), the launcher's module rebuilt from it (unit tests passed, log `evidence/I-0139/build-main-644902ff7.log`, module SHA-256
765C013E48E2...), the start check `start-644902ff7` passed (01:00-01:02, restored). Then `Run-FullControl-1112.ps1 -Tag full-1116
-NewReference` from this checkout, in a hidden detached process (chain log `evidence/I-0139/artifacts/control-chain-1116.log`, log
`full-1116.log`), 2026-11-16 01:02-03:25 Paris, the ASUS alone (the OLED off, Thomas's word 00:34), the other pilots quiet (the
coordinator), offline, every save restored by hash:
- **The replayed path gate** `full-1116-gate` with its memory leg (`evidence/I-0083/gates/full-1116-gate.json`): **FAIL on the
  standstill pictures alone** against the 2026-11-12 reference (mean 11.18 and 11.23 for a threshold of 1.5, cells 0.0116 for
  0.005). Every cadence metric within its threshold: period 16.67 ms; frames over 2.04 P 2 (threshold 8); over 1.5 P 16.5 (27);
  display_late 6 (14); game steps over 2.04 P 0 (6). Memory 10 MB a visit (threshold 40).
- **The reference retaken** (D-0268, stated here): only picture departures, so the runner's reference step ran: m3, then
  `Invoke-ReplayGate -Calibrate -NewReference -Evaluate` on m1 to m3 and v1 (`full-1116-cal.json`): PASS, a self-consistency check
  (m2 and m3 depart from m1 by 0.48 and 0.44); the reference is now m1's picture (`evidence/I-0083/reference/firelink-start-
  9E2B6ABF84BC.png`, tree `4a06937d10e0`). Thresholds lowered by the calibration: frames over 2.04 P 8 -> 5, display_late 14 -> 12;
  the others kept. The new picture is brighter than the 2026-11-12 one (the client area's mean luma 66.5 against 56.6, back to the
  2026-11-09 control's 67.6); the pair `evidence/I-0139/artifacts/full-1116-reference-old-new.jpg`. The cause: D-0280 (below,
  « Why the gate's picture brightened »). **Approved** by Thomas (2026-11-16 03:57, in the tour's thread: « Pour
  l'instant oui », the decisive lever being after workstream 12 and the delit textures): the reference stands.
- **The control sizes**: 1920x1080 in full screen (the controller's 0xD3126612 and `closed` 1, as the full-screen runs since
  I-0100), the 1500x844 window, 1280x720 windowed: each replay complete, no fault, restored.
- **The night endurance** `full-1116-night` (01:52-03:25): 92 minutes of tour, 54 visits and loads, no fault, no crash dump, restored.

## The gate's reference retaken after D-0293 and D-0294 (2026-11-16 10:43-11:11, the ASUS, offline)

D-0293 (the local lights at DS3's own strength) and D-0294 (the exposure's target 0.12) change the gate's standstill picture (the
coordinator, 09:07). `Invoke-ReplayGate -Tag ref-1116b -Calibrate -CalibrationRuns 3 -NewReference -Screens 1920x1080@60` from this
checkout on main `92cab714e` (log `evidence/I-0139/artifacts/ref-1116b.log`, `evidence/I-0083/gates/ref-1116b*.json`), then one run
of the original (`ref-1116b-original`, Kind Reference); every replay complete and restored. Calibration PASS (three measure runs and
the validation): the reference is now `evidence/I-0083/reference/firelink-start-56F3497A02DB.png`; thresholds (D-0268, stated):
frames over 2.04 P 5 -> 4 (lowered), display_late would have risen 12 -> 14 and was kept (no `-AllowRaise`), the others kept. The
client area's mean luma: 66.5 for the reference of 01:37, **48.5 now, the original 47.4** (`evidence/I-0139/artifacts/
ref-1116b-old-new-original.jpg`, old, new, original). **Approved** by Thomas as the gate's reference (2026-11-16 11:15: « Mais
oui ok ça me va »), with his remark « L'original est toujours plus beau » (the original's bluish ambient on the player and the
walls: the baked ambient and fill lights D-0262 removed; passed to R for workstream 12 by the coordinator).

## Why the gate's picture brightened (2026-11-16 05:02-06:28, the ASUS, offline)

Thomas (2026-11-16 03:59: « Quand même bizarre que l'image se soit éclaircie »; 04:00: the large suspect merges only, narrowed step
by step). [Run-I139Bisect.ps1](Run-I139Bisect.ps1): one replayed-path run (Kind Measure, as the gate's) per build of main made from
a detached worktree at each merge (`evidence/I-0139/bisect-<commit>`), the standstill picture's client-area mean luma (ffmpeg
signalstats); logs `bisect-runs.log`, `bisect-runs2.log`; eight runs, every replay complete and restored. OBSERVED:

| main | what it holds | luma |
|---|---|---|
| `465e41560` | the 2026-11-12 reference's native tree | 56.7 (the reference 56.6: not the PC or the screen) |
| `63b113d95` | before I-0138 tranche 1 | 56.5 |
| `cf0af0f3b` | I-0138 tranche 1 (our tone map) | 56.4 (not the tone map) |
| `d1fbc6f51` | 2026-11-14 15:55 | 56.3 |
| `c9cc6bcf8` | I-0138 tranches 5d and 5e (D-0280), I-0141 tranche 2's observation | 66.4 |
| `ad15b002a`, `9e6d8167f` | later | 66.4, 66.5 |
| `644902ff7` | the full control's | 66.5 |

The jump falls between `d1fbc6f51` and `c9cc6bcf8` (three merges; `76f77782d` and `ccde3618a` built, not run: the coordinator's
call once the journals answered). The journals say why: during the standstill (frames 120 to 3,000) the automatic exposure's factor
is log2 -0.46 on `465e41560` and `d1fbc6f51` (the view's mean 0.23 above the target 0.20: darkened to about 0.73) and 0.00 on
`c9cc6bcf8` (`native_auto_exposure`), D-0280's lower bound of 1 (« never darkens under the zone's own exposure ») merged in
`c9cc6bcf8`. CONFIRMED by the bisection's interval and the factor at the picture's moment; the earlier « about 2 % » compared the
path's last frames, not the picture's (withdrawn). The brightening is D-0280's intended effect, not a defect; Thomas lets the threads
review that rule, R takes it (the coordinator, 05:33).

## Evidence

- `evidence/I-0139/artifacts/static/` (the sheets' dumps), `evidence/I-0139/artifacts/runs-*.log`, the runs under
  `evidence/I-0066/artifacts/i139-*`.

## Uncertainty / unresolved questions

- The sheets' OPEN items (LIST « What the sheets could not settle »).

## Artifacts produced or modified

- The native test tool `witness-cutscene.txt` (`native/frame/game_bootstrap/04_updates_and_test_tools.inc`, ApplyWitnessCutscene);
  Run-AlphaCast's `-SetFlag` with several flags, `-KeySteps` (with T), `-Teleport2` and `-Cutscene`; `boss-flags.json` corrected (LIST
  « Corrections »); the tour's runners and readers in this folder.

## Reviews

- The cutscene tool's step review (2026-11-13 17:10): no high or medium; three lows applied (the id of a DS3 map and the EMEVD modes
  only, nothing requested unless the file is removed, the runner's pattern the same).
- The integrity review on the branch (18:18): nothing that makes a result look better; one medium (the Conclusion and Artifacts
  stale) and lows (counts, two readings) answered in this file (`ce75f8ae2`).
- The final review (18:30): no high; M1 (main merged first, `b258d277`), M2 (the Friede fault's owner: I-0140, workstream 16), M3
  (the index, this file's next step, status.json) answered; lows: L1 (the two other singletons the request asserts checked, the file
  waiting), L2 (the request's form stated above), L3 (`-KeySteps` durations to Pilot-Game's range, waits of any length, `-Teleport2`
  with its hold, a T without it logged), L4 (the cutscene runner's pattern the tool's) applied; L5 (the bosses sheet's old keys) a note
  at the sheet's head; L6 (one file check a tool every 15th call) left: four cached-path checks, as the existing witness tools do; L7
  (D-0217 is WORKFLOW's « a series runner written once per need ») kept; L8 this section, and the merge on the coordinator's go
  (2026-11-13 18:14: a tranche of tools, D-0129).

### Reviews at the second merge (2026-11-15, range `main ad15b002a...`)

- **The final review** (Opus 5.5 xhigh, on `7bf1de77e`): H1 (REFERENCE said no run records its module and inferred the build from
  file times, wrong for three menu runs; every baseline holds `module_sha256`) fixed: the build by the baseline's hash, the
  inferred columns dropped, the menus on `build-tour3`; M1 (REFERENCE stale against the TSV) fixed, regenerated; M2 (the Ringed
  City's last three without a picture verdict) fixed, read; M3 (no journal pass for tranches 3 to 5) fixed, the light shafts'
  class `3D65A88` relayed for workstream 5; M4 (« not there in Eta » unsupported) reworded OPEN, the `t4p` originals to run; M5
  (Conclusion, next step, status.json stale) rewritten; M6 (D-0284 and the WORKFLOW row promised more than the records hold)
  reworded to what REFERENCE holds, and Run-AlphaCast writes its parameters (`alphacast-args.json`); lows L1 to L9 applied (the
  NPC runner's own flag only, `-Toward` checked, the credits' time bound, the luma and sheet crops without the desktop, ffmpeg
  failures reported, the reference's readers, F05's row, the WORKFLOW row's runners and times); L7 (a resumed prefix may mix
  builds) recorded in « The runners' resume and pause ».
- **The step review of those answers** (Opus 5.5 high, `7bf1de77e..106164213`): no high; M1 and the integrity review's first
  medium the same (the Conclusion's « ten places alike ») reworded; M2 (the original's 30 s is the Programmers page, not the
  title) corrected; M3 (the luma tool silent on ffmpeg failures) fixed, ERR and exit 1; M4 (the crop chosen by width alone)
  answered: the two known capture sizes only, any other refused; lows: L1 (the OLED crop kept 8 columns of desktop) fixed, from
  8,31; L2 (the arguments without the defaults, a byte-order mark under Windows PowerShell, written at the end) the defaults
  recorded and no mark, the end kept (the run folder is Run-StartupTrace's; noted); L3 (the boss runner's test files) fixed the
  same way, I-0089's Run-TestTools left to its owner; L4 (23 scenes in 24 runs; 20 of 21 originals on the ASUS) corrected; L5
  (`build-tour4`'s commit not stated) said in D-0284; D-0284's wording narrowed after Thomas's « Oui » to what the records hold,
  his words quoted unchanged; L6 (the filing) reworded as relayed, not yet in ROADMAP; L7, L8 corrected; L9 (the sheet script's
  exit code, the credits' fixed 900 s) fixed.
- **The integrity review** (on `106164213`): no high; nothing weakened, no special case, no work cut during a measure; the run
  facts, builds and counts match. Mediums: the Conclusion's « ten places alike » (reworded) and the journals' list incomplete
  (`native_selected_texture_unavailable` of the `MultiBlend4_et1_lightmap` family in 25 runs, the menus' hair and model rows, a
  non-finite pose: added); lows: 45000090 not played, 23 scenes, the filing, Andre's menu not seen, « not there » OPEN, both died
  at Anor Londo, 51010000's darker sky, 30000030 and the wyverns UNSURE, the runners' resume and pause stated: all applied.
- **The step review of the second answers** (Opus 5.5 high, `106164213..b8a1efc32`): no high; M1 (the sheet tool checked the width
  only, so a 1920x1200 or 2560x1600 capture passed « ok ») fixed: each input's size read, any other than 1920x1080 or 2560x1440
  refused by name, exit 1; lows: L1 (the boss runner noted its flag before creating it) fixed; L2 (an old sheet left beside a
  failure) fixed, removed first; L3 (the sheet tool's stale header) fixed; L4 (the luma tool reads each shot twice) left, harmless
  at this size; L5 (« sky » from a whole-frame luma) reworded; L6 (Anor Londo among the « agree nobody answered ») reworded.

### Step reviews of the flag tool (2026-11-15, the fresh thread)

- **First** (Opus 5.5 high, `69f82da1c..266039313`): no high; M1 (the getter returns 0 for a flag no block holds, so a guessed id or
  an absent block read as « cleared ») fixed: the block lookup 1404C7F00 called, -1 written where no block or word is readable, the
  comment narrowed; L1 (a file that cannot be removed re-read every 15th call) fixed, read once; L2 (the flag rows under the test
  rows' cap, the death's change dropped first) fixed, every flag row printed, the test rows at 6 again; L3 (the sampling unstated)
  stated; L4 (the 512-row cap silent) a « capped » row.
- **Second** (Opus 5.5 high, `266039313..a52d24e47`): no high or medium; L1 (-1 also for unreadable words) said in the runner; L2
  (the setter's own `witness_flag` rows not printed) printed; L3 (Series6 without try/finally, no END on a throw) fixed; L4 (the
  keeper part did not refuse another pilot's test files) fixed (`26e226348`); the native comment's « no block » sentence left, true as
  written. Its open note (whether Friede's action reaches 6.8 m) stays open: the talk flag was cleared before E, so the reach was
  not tested.

### Reviews at the close (2026-11-15 and 16, range `main 6c9037dc6...`)

- **The final review** (Opus 5.5 xhigh, on `d6644f1e4`): the flag reader matches the game's getter; the records did not hold. H1 (the
  close used the « insufficient information » exit for rows today's tools could reach, and its list was incomplete): the three
  recipes run with the existing tools (block 3 above: Friede's talk flag after the travel, D06 with the goods given, D03 at two
  points), none reached, alike; the list rewritten with what each row needs, 45000090, Anor Londo, Dragon Barracks and D08's
  entrance added. The close itself stays the pilot's under D-0129, told to the coordinator; the rows go to workstream 16. M1
  (Friede's three phases unsupported) reworded: the flag after two F6 kills, her first two phases not shown. M2 (the « MOVIE » plate
  fixed by I-0140 tranche 9; « filed » for a relayed defect) reworded in the Conclusion and ROADMAP. M3 (the NPC places, D02, D08)
  reworded. M4 (the audio and the rows not carried) added to ROADMAP workstream 16 and STATE. Lows: L1, L2 (Run-I139Dlc's
  -NotBefore, its refusals) fixed; L3 (the Princes' souls kept, the Nameless King's death) and L4 (the keeper's hair rows; the DLC
  journals only `native_asm_part_missing_*`), L5, L6, L7 (1623 cleared by the game; 35000050 for 1623 off) answered in the sections;
  L8 (native D06 not at its place) run again in block 3; L9 (the tool without an exception guard, a fifth file poll without
  `test-tools.flag`) left: the path is the game's own read-only lookup, and the other witness tools poll the same way; L10 (430 runs,
  the branch « merged » in STATE before the merge) corrected, the STATE row true once merged.
- **The step reviews of block 3's runners** (Opus 5.5 high, `d6644f1e4..a2f16cbdf`, then `..ecce9af0c`): no high; M1 (D03's second
  teleport after a possible warp) two runs; M2 (Friede's E 6.8 m away) a point 1.5 m from Father Ariandel; lows (the files made
  recorded once made and removed in both finallys, test-give.txt refused, the give rows printed, -AfterFlags validated) fixed
  (`80ed7cc57`); D01's teleport after its scene noted, not run again.

- **The integrity review** (on `be6e75a10`): H1 (the Conclusion's, ROADMAP's and the index's « no native fault » against the six
  faulted native runs of the tour, two after I-0137's fix) reworded with the six runs and their fixes; M1 (D06's state after the death
  differs) an OPEN divergence, not alike; L1 (five F6 rounds logged at the Princes), L2 (an empty flag sentence at D06, dropped), L3
  (the reader's run-name pattern, stated in REFERENCE.md), L4 (the Nameless King's death INFERRED from the souls), L5 (the native
  player warmer at D01 and D04) and D01's journal rows answered. No weakened test, no special case, no work cut during a measure.

## The merge (2026-11-13)

Merged into main `d1f0d340` on Thomas's go (18:49, in the thread; the coordinator's at 18:40), the launcher's module rebuilt from it,
the start check `start-d1f0d340` passed (19:04-19:07, restored, no refusal or failure). The tour goes on on this branch.

## The runners' resume and pause

`Run-I139Cutscenes.ps1` and `Run-I139Npc.ps1` stop between two rows when `evidence/I-0139/stop-tour.txt` exists (the coordinator's
pauses for the other pilots), and skip a run whose `restoration.json` exists, so the same command resumes a series; a faulted run is
therefore never redone under the same prefix (a new prefix redoes it), and a resumed prefix may span several builds (each run's
build is in REFERENCE.tsv by its module hash).

## Suggested next step

The rows not reached, for ROADMAP workstream 16 or a real playthrough, each with what it needs: the Twin Princes' fight after their
scene and the Nameless King's death on foot (why the player returns to the bonfire, souls kept or lost; a piloted fight); Friede's
first two phases and her intro by her event (action 9373's place and reach, the talk script); D03's and D06's prompts (the objects'
floors and activation from the MSB parts; D06 with the goods 2156 given); D08's entrance event 15105500 (flag 15100500 and the prompt
on entity 5100890); 45000090; the Old Woman of Londor at Anor Londo and the Altar of Sunlight at Dragon Barracks (a teleport point
that holds); the NPCs' presence in Eta; the movie and NPC dialogue audio, not checked by the tour. At the project's end, the whole
tour again from [REFERENCE.md](REFERENCE.md) (D-0284), on one screen and one build.
