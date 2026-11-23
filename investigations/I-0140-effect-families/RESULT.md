# I-0140 « Les familles d'effets et de draws pas encore natives » - result

## Conclusion

Active (2026-11-13, the E pilot; ROADMAP workstream 5). Opened on `claude/stoic-jennings-ws5` from `main` `ddc221de`, continued by
the fresh E thread on `claude/cranky-albattani-d5c1e3`. The census of the journals' effect counters is done (below). **Tranche 1**
(the cutscenes' faults the game tour met, I-0139 tranche 3): Friede's intro and the High Wall's 30000000 no longer crash natively (the
GX parameter registry's group ids, never written natively, now written as the original's), and the player's face in cutscenes is
shaded with its hair and skin colour (a disp item's FaceGen owner, colour and vertex patch taken in every view, as D3DA20 does);
witnessed (Friede's intro on `build-e3`, before the face change, in `i140-e1`; 30000000, Vordt's intro and the ending on
`build-e4` in `i140-e2`; neither build has I-0138 tranche 4 part 4a, merged in after). **Tranche 2**: pass 0x21 (the DLC's opaque effects:
the burning debris' flames) drawn natively, before the 0x29 rows with the depth written, the effect depth copy and the layer's half
depth taken after it. **Tranche 3**: the ending's embers: a cutscene's `MaterialParam` rows live in the model item's
« MaterialParamModifier » component, which the draw makes its parameter owner; the native takes it so, and the cinder glow shows on the
cutscene player's trousers (`i140-e8`, K-0494). Tranche 2b (the depth copy back before 0x21) was tried and withdrawn. **Tranche 4**:
pass 0x21's effects drawn among the 0x21 rows, before the 0x22 rows, the water and the TAA base, as the sort key orders them
(`i140-e10`; tranche 2's final review's M2 and its L3 answered on such frames). **Tranche 5** (the fresh E thread,
`claude/confident-lumiere-672539`): the GX registry's 20 other group slots written with the original's ids (the withdraw 4A060, reachable
natively, read them at -1: READ, not observed); 31000010's valley and wyverns, 32000010's dragon and the ending's body read
as drawn, their defects the lighting's and the fog's (`i140-e12`, causes INFERRED), handed to R. **Tranche 6**: the GX00 Decal materials take the original's class 8 (0x10 forward) and the engine-6 G-buffer
row, every G-buffer row its material's blend mode as 140200840 chooses it (`i140-e14`, `i140-e15`: no fault, no regression seen). **Tranche 7**: the traced sun's shadow source read as the original's (the view's ShadowParam keys and weight, the
Shadow Light Id's light set, the -1 Angle; 88A10, DAF40, F90F0, 18C590) and a cutscene's own GPARAM file bound with the map's rotation
as CDEE50 does (natively skipped: 31000010's sun a yaw of 156 degrees off); witnessed `i140-e16`, `i140-e17` (the keys and suns the original's). **Tranche 8** (the fresh E thread,
`claude/beautiful-ellis-78a5ea`): the effects' fog, the engine's fog bank (never given a file natively) answered from the native
banks, I-0141 tranche 3's overrides included; the effects' cbFog the original's to the digit in play (`i140-e23`), and in 32000000
the same ranges and endpoints word by word in both modes (`i140-e27`); 32000000's veiled shot is
lost with it (whole-frame YAVG 75.0 against the original's 142.9, 143.2 natively before; OBSERVED; its cause INFERRED, two
readings, R's). Witnessed up to `build-e47`; the final tree (`build-e51`) not run. **Tranche 9**: the credits' « MOVIE » plate (a regression of I-0137 tranche 3): a movie proxy without a
frame imports its inner texture, SYSTEX_BLACK, as the original's; the film seen again (`i140-e28`). **Tranche 10**: the light shafts 10003 natively,
with the scene control's two half-size targets, their clears, copies, viewports and scissor; seen in 30000010 and 38000020
(`i140-e31`, on `7504c672d`; the final tree's later code changes, the outer cache's seeding and a row, not run before the merge). **Tranche 12**: gap 23,
the fast travel's fade under the TAA: the fading rows draw a fade mask the TAA reads (no history and the pixel's own sample over the
mask's 3x3), the hard dots of native `no-taa` (tranches 35 and 41's stand-in for the original) back under the TAA (`i140-e60`,
`i140-e62`: the cause the resolve, not the jitter or the mip bias;
the own sample everywhere would make the fog crawl, +72 %; the fix witnessed against native `no-taa` in `i140-e64` on `build-e66`'s
broader gate; the merged gate witnessed on main's launcher build in `i140-e70`: the dots, the mask on 79 and 65 frames of the two
travels, 42 rows a frame); unchanged under DLSS/DLAA (Thomas,
2026-11-15 23:31: « D'accord, ça me va »). **Tranche 13**: the spells' audit: a test tool attunes any spell by script (STUDY-ATTUNE);
every sorcery, pyromancy and miracle tried once natively then in the original (`i140-e76`, `i140-e80`): 81 of 106 cast, 79 drawn
alike, none seen differing, 2 not compared, 25 not cast in either renderer (Intelligence, a weapon or a shield, a held cast); Father
Ariandel's dark limbs read by PIX (`i140-e71`): the lighting's, not a material (R's). **Tranches 14 to 16** (a second test tool,
F1, equips a hand and adds a SpEffect): the 25 spells left cast on copies of Thomas's Alpha, so **106 of 106 cast, 104 drawn alike,
none seen differing** (56 of them by a Sonnet triage of the sheets alone, INFERRED), Chameleon and Firestorm not compared; the eight
resins alike on a dagger; poison, toxic, frost and curse alike, bleed not shown by a direct add; the Moonlight Greatsword's art
and wave drawn natively, a glare streak and a green-lit floor seen natively only (INFERRED R's); the 118 melee weapon arts used
once in each renderer (four batches): none differing by a luma screen of every cast (a lead, not a judgement: 12 of them read by
eye), Yhorm's Great Machete's blade darker natively (« Tranche 16's arts batches »). **Tranche 17** (F1 equips arrows, bolts and a
quick-slot goods, a held art with an attack inside it): the shields', catalysts', torches' and Stance arts and the goods' own use
alike in both renderers (one read cast a family at least, the rest by a luma screen) but **two differences**: Havel's
Greatshield's dust burst drawn again natively on the art's second use, the Crucifix of the Mad King's violet swirl missing
natively; the hits' flashes on a wall brighter natively (INFERRED R's); the thrown goods alike but the rope firebombs' blast (the
smoke over the fire sooner natively, a lead); the bows' arts and the arrows and bolts not reached (one hand, no target)
(« Tranche 17's runs »). **Tranche 18** ran those leads again: Havel's dust and the Rope Black Firebomb alike, the Crucifix's
swirl missing natively in both native runs but drawn by the original in only 2 of its 3 (a lead). **Tranche 19**: the Crucifix's
violet swirl over the body drawn natively (clearly in 1 stance of 6, perhaps 2; how often not measured; that part closed: the
native can draw it), the original's white ring at the attack not judged, the Rope Firebomb's late swell of flames in 4 of 4 original casts and 2 of 4 native ones (a lead), Farron's
and Splitleaf's arts alike, the Corpse-grubs' maggots (SpEffect 5100): the status's logic runs in both (its gauge, its removal by
a torch), the worms' draw not judged; a bow drawn in two hands (its art not reached); F1's `n` and `p` put a status on, and
probe, the nearest NPC: poison took effect in both (toxic's INFERRED), not in view; no infused swing hit. **Tranche 20**: F1's `f`
turns the player toward the nearest character (its heading kept by the game): statuses and swings took HP from her in both
renderers within 3 to 6 HP, and a second character lost 135 HP after a bleed row in both (its burst INFERRED; their look not
judged), the Chaos and Lightning hits and an arrow's hit alike, the bow's art still not reached; the Rope Firebomb's late swell
drawn natively in tranche 20's three throws under every counting variant tried (the native can draw it: the lead closed), the
tallies over both tranches depending on the variant. **Tranche 21**: the maggots closed by Thomas's eye. **Tranche 22**: F1's `h`
keeps a target alive; the Crucifix's white ring drawn natively, the Fire, Dark, Blessed, Poison and Blood hits alike (the same
damage in both), by the pilot's eye, Thomas's eye left; the bows' arts, the bolts on a target and the statuses' look on an enemy closed by Thomas in his own filmed play. **Tranche 23**:
the embered state closed by Thomas (his character had died); Crystal and Deep on Blacksmith Andre with the same damage in both,
Crystal alike by the pilot's eye, Deep alike in kind with the native's violet brighter (open, cause not read); frostbite by a
weapon not reached (the plan's swings too far apart); the plan's last swing killed Andre in both runs (saves restored). **Tranche 24**: step 5
closed by Thomas's eye on tranches 22 and 23's crops; pass 0x20 never met since pass 0x21 is drawn and no forward blend refused since
its fix (the journals); step 1 keeps 11000 (where it goes not settled) and 10003's brightness for Thomas; the characters' blood paint
not witnessed (none seen in either mode on a struck Andre: step 2 open); a film's frames found on the effects' clock (D-0299), the
HP bar on screen 1 to 44 ms later natively than in the original after the same creation (median 26, the clock mapping's error not measured); step 5's strips still for Thomas's eye (D-0181).
**Tranche 25**: the Firelink wall's mist (2026-11-11's lower contrast) alike to the original at the same effect age, drawn only under
`ffx-observe` (the module of `0860cbc8c`); in play natively the arch has no mist by Thomas's choice (I-0113 gap 24).
**Tranche 26**: Fire Orb's magenta rim at 1920x1080 of the same order in both modes on the effects' clock (peaks 107 and 21
natively, 52 in the original, 60 frames/s), 2560x1440 not compared; the Quit Game portrait close, the face warmer natively; sheets
for Thomas's eye; gap 3's path left.
**Tranche 27**: 11000 native by its mechanism (m45's window spots in the light list, identity INFERRED); Delta's portrait as
Alpha's; quits from the Dreg Heap and Firelink, the five visible Load Game portraits there in both modes (the recorded Ringed City
quit not replayed); Thomas's character's armour black in the portrait natively, the cause INFERRED (no menu-view region pass), the original's
read (STUDY-MENU-ENV-REGIONS), to restore (D-0301, tranche 28); the struck player's paint not settled (orange marks in the original's first shots, the moments
not matched).
**Tranche 28** (D-0301): the menu views draw their environment regions natively for the first time (the region pass restored for them); natively D0C2F0's key gate failed (the engine's
env-map bank empty, the native's gparam banks answer it now), so the menu region was never added; Thomas's character's Quit Game
pane 41.51 natively against 41.23 (30.54 before), the armour blue-silver again (OBSERVED, MEASURED, one view).
**Tranche 29** (a witness, no fix): the engine's env-map bank by call site in both modes at Firelink (OBSERVED, one path, two minutes):
natively only NativeEnvMapCube (2672 calls, the default entry) and D0C2F0's gate (11, the engine's bank false, the native banks'
answer true) read it; of the original's 11 game sites, the 10 of 1A86C0, 75B30, 675B0, 4E2A0, 770C0 and D0C990 are never called natively;
D0D060 and D0CA20 belong to the developer's model viewer (INFERRED), 6A490 is a debug window; 40DD0 (cutscenes) and the sea waves
not met on the path.
**Tranche 30** (the menu views left, both modes): the Face Detail's close-up now takes its own gparam id 10 (D65410's re-key
transcribed): the skin's upper quartile and p90 now the original's, its median (+4 to +5), mean (+8) and lower quartile (+22 to
+26) higher natively (MEASURED, the shots outside D-0299: timed from the pilot's clicks); what is left in the Player views
(creation +5 %, Status body +11 %, the Face Detail's darks) is their missing self-shadow (INFERRED), for tranche 31 by a traced ray
(D-0303). The Load Game list's third visible row warmer and darker natively in every list (MEASURED), not the region IBL (INFERRED: the witness carries no list slot),
the title's tint (tranche 32); 40000040 does not exercise 40DD0 in either mode; gap 3 not reproduced after 480 s at the Ringed City; the region
registry closed in play (D-0298's test flag) and the game went on without regions, travel and quit included (OBSERVED, one run).
**Tranche 31** (D-0303): the Player menu views (the creation's four 784x784 views, its Face Detail, the Status body) draw the
character's own shadow by a traced ray toward their ShadowParam sun, in a traced scene and caches of their own, each view of a
frame with its own material table and top level (up to four), the rest of their lighting raster; the Face and Prof views
unshadowed. At the same moments: the Status body 37.70 natively against 35.86 original (39.77 without; step 1's code, `build-a`: the Status page not run with the merged code), the creation's pane 34.8
against 33.9 (35.6 without), the Face Detail's skin lower quartile 76 to 88 against 76 to 84 (99 to 109 without) (MEASURED); the
shadow where the original's falls, its edge hard where the original's is soft (OBSERVED; soft since tranche 35). With the frame
graph's executor the mask is declared, passed and read on the same frames (READ for every frame; OBSERVED on traced frames only).
Left: the hair's forward rows, the creation's view pairs sharing one table, the held and neutral paths under a failure
or the rays off (counted since tranche 34: 0 in three Status reopenings), RankingBody.
**Tranches 32 to 34**: the Load Game list's « third row » was a band of the screen (y about 400 to 610 at 1920x1080) where the
title's tint layer, a multiply kept at half strength under a page, coloured the page itself (portraits, panels, the background):
the tint fades out under a page since tranche 32 (the band's rows within 1.6 % of the original in every channel, the faces within
1.5 %, MEASURED). The title's ring, embers and smoke (its screen layer) redden rows 2 to 5 by R +0.6 to 3 % (MEASURED, tranche 34's final build); tranche 33
took them off under a page, and Thomas chose (2026-11-21 23:07, « option 3 ») to keep them over it at 0.3: tranche 34 reverted
tranche 33 (his ideal, option 2, the layer under the page, needs the movies' draw order read). Tranche 34 also took the streaks
out of the title's smoke and made the rules beside REKINDLED smooth (stills for his eye), and counted the menu self-shadow's held
and neutral paths: 0 in three Status reopenings under the executor (OBSERVED).
**Tranche 35**: the Player views' penumbra by the original's own screen-space blur (1D7850's passes, its 12 taps, Gaussian and depth
weights, the record's Blur Count and Radius) on the traced term: the neck's hard edge soft as the original's, the pixels off the original
by more than 12 levels 3.5 to 4.0 % before, 2.1 to 2.8 % after on shots 1 to 3 (MEASURED; shot 0 and the Status body
the other way). Step 2's paint seen in both modes on a struck player
kept alive by F4: dark marks on the back natively and in the original (OBSERVED; natively far more of the back covered, a lead),
the fights not at the same moments, so not compared (D-0299).
**Tranche 36** (documentation only): the Status body's distance after tranche 35 is the views' geometry, which differs between
runs (MEASURED; pose and placement not separated); compensated per tile, the blurred body is as near as the hard runs (MEASURED):
the blur's effect not shown either way; the Face Detail realigned, the blurred further on three of four shots (0.04 to 0.40),
so tranche 35's neck percentages are a zero-shift measure across runs (its softness OBSERVED stands); a same-run comparison next;
its taps in the view's own pixels (INFERRED).
**Tranche 37**: hard and blurred in one run (a test file toggled between eight Status openings): at equal page moments the
blurred body is nearer the original's in all four comparisons; in the two on-time ones by 0.43 to 0.67 against a same-state
spread of 0.00 to 0.20 (MEASURED), in the two late ones by less than or about their own spread: the blur stays. The runner's
shots land 0.6 to 1.2 s after their target (MEASURED) and the body's pose seems to follow that moment in both modes (INFERRED), so
shots at equal moments come before any reading of the native's pose.
**Tranche 38**: the Status shots at their target tick (Pilot-Game `-AtTick`): all 64 copies 0 to 1 ms after it. At equal page moments
every blurred shot is nearer the original than every hard one, and the two modes repeat alike (MEASURED). The body's residual: the
native +2 levels brighter and +35 % in gradient (MEASURED; the page's panel equal), its edges stepped where the original's are
smooth (OBSERVED; the original's mechanism not read, a RenderDoc capture next); tranche 37's pose reading not borne out on the page's clock, untested on the views' own (the switch key's); the portrait's picture 2 px left of and 0.75 px
below the original's inside the same frame (MEASURED; tranche 43: it comes with the native's collects after the game unlists the view).

**Tranche 39**: the original anti-aliases its menu views with its own DLAA (GXAntiAliasingFilter 1B9790, mode 3) on their own
single-sample targets after their tone map: READ twice in its code (the menu pipelines keep the per-view AA byte, FB890 enqueues the
filter for every listed view) and MEASURED in play up to its programmes' lookups (`aa-observe.flag`: the Status body's 560x768 on
every frame of the page's visit, the views hidden before the switch key included; the portrait's 512x256 once at the page's
opening; the gates passing; the draw itself INFERRED); AA-NATIVE-READY's « menu views never reach it » was wrong. The native's
extra detail grows with the original's local brightness (gradient x1.07 in the darks to x1.59 in the brightest, MEASURED), that is
with contrast (INFERRED), as an edge filter's correction does; but the [20, 40) bin (69 % of the crop, gradients about 2 levels,
under DLAA's threshold 0.1) is 1.23 to 1.25 too: that the DLAA explains the whole gap is INFERRED and partly contradicted, the
direct test (the original with its Antialiasing option off) not run. Thomas (D-0305): the menu views take our own anti-aliasing, the TAA by default, DLSS or DLAA by the
option; DS3's filter stays removed.

**Tranche 41** (D-0306): the menu views' TAA reconstructs with a tighter Gaussian (8 instead of 2.29) and restarts a history from an
unjittered frame taken as its own samples: the Status body at about 0.78 of the original's DLAA instead of 0.72 (MEASURED, a
calibration run and the witness), a small added shimmer (MEASURED on films, its cause INFERRED). The portrait is resolved about six
times per opening (its reset and five jittered frames, MEASURED by the rows' counter) and then kept; its face about 0.65 to 0.67 of
DLAA instead of 0.61. **Tranche 42**: an even supersample of those frames tried and reverted, the face's gain inside the noise
between runs (INFERRED: the face's distance is this AA's nature, not an unconverged history); the portrait's offset from the
original (about 2 px) and its wobble between openings (up to 0.75 px) remain open. **Tranche 43**: the 2 px come with the native's
collects after the game unlists the view (MEASURED: kept at its first collect, a test tool, it sits within 0.5 px of the original;
the model's pose INFERRED), the native's documented departure from the original's draw while listed; that first collect lacks the
environment cube (MEASURED): the fix, the listing rule with named conditions, is the next tranche's. The dark bin's half not the DLAA's lies where the local level differs between the runs (MEASURED), the cloth's pose
(INFERRED; open until a comparison at the same pose).

## The census of the effect counters (2026-11-13)

A `measure-tools` subagent over the tour's journals (`i139-*`: 93 native, 89 original) and the native journals written since 2026-11-08
(682); `evidence/I-0140/census-effects.md` and its script `census-effects.py` beside it. OBSERVED:

- `native_ffx_skipped` (each new kind of skipped command, logged by default since I-0113 tranche 23): **no row** in 775 native
  journals, the tour's 55 bonfires and 25 bosses included. So the skipped renderer classes (10003 the light shafts, 11000 the spot effect
  lights) and entity kinds were not met in any of them.
- `native_effect_stage` (its last row a journal): no skipped draw, no missing texture, no pipeline failure anywhere; `other_passes`
  (classic FFX draws of passes 0x20 and 0x21, captured and not drawn) 0.29 % of the effect draws in the tour, 0.16 % in the other runs,
  and **only in the DLC's areas**: Friede's fight (up to 47,723 in a run), Ariandel's chapel, Earthen Peak Ruins, the Ringed City (the
  Shared Grave, the Mausoleum Lookout, the Inner Wall, Filianore's church), the Demon Prince, Halflight; the night endurances
  (`full-1109-night`, `full-1112-night`) through the same areas.
- `native_light_entries_unavailable` « no_selected_keys » in every journal (93 and 509): not an effect.

Reading (I-0113 STUDY-EFFECT-PASSES-20-21, the study thread, 2026-11-06): DS3's sub-pass chooser 1402997E0 sends kind-1 effect entities
to 0x21, where 140299E40's mode 1 makes the commit 140264230 bind blend `B+0x08` (off) and depth `B+0x1E0` (LESS_EQUAL, written): opaque
effects drawn with the forward opaque rows, before 0x29; 0x20 is not reached through that chooser. The native's plan for them (read in
the code, not written): `EffectStage::Record` takes the passes `{0x29,0x2D,0x31}` (`scene_pass_frame.cpp`), forces every effect
pipeline's depth write mask to zero (`effect_stage.cpp`), and its `before_pass` for any pass other than 0x29 draws every remaining 0x29
row (`rows_until(UINT64_MAX)`); drawing 0x21 means a first pass 0x21 that keeps the bank's depth write (its pipeline key told apart) and
draws no 0x29 row before it. Which effects they are (what shows in Friede's fight) is not known: a witness there with `ffx-record.flag`
would split 0x20 from 0x21 and name them.

## The faults from the game tour's cutscenes (I-0139 tranche 3, 2026-11-13)

**Friede's intro (cutscene 45000000) crashes natively** (`i139-t3b-c31-friede-nat`, and `i139-t3-c31-friede-nat`; the original plays
it). OBSERVED (the crash report): an access violation at `FUN_140097600+0x1E` < `140038430` < `140CC8180+0x22C` (the cutscene's
`IngameDrawParam` act, REMO-CUTSCENES-NATIVE's slot 9) < the cutscene's update tasks; `r8` (the index) 0x5E326E10. READ (GhidraMCP):
1400380A0 (one family's gparam bank set by the act) takes `lVar = [[0x144961B50]+0x4788]` and `idx = *(int*)(lVar+0x80)` when that is
not -1, **else an uninitialised local**, then 140097600 writes `lVar+0x490+idx*4` and `lVar+0x428+idx*4`; 140031A20 (the light sets)
does the same with `+0x88`; 14003C900, 140040DD0 and 140045CA0 call 140097600 too. INFERRED: natively `+0x80` stays -1 (the
uninitialised index is the faulting value), set in the original by something the native does not run (the display initialisation
B5E00's callees are not run on the native scene root: `01_state.inc`).

READ (the fresh E thread, 2026-11-13; GhidraMCP and headless scans on `ghidra-mirror-i0068` by two `ghidra-research` subagents, their
scripts and outputs in `evidence/I-0140/ghidra-gx-groups/`; 48B80's call order and the readers 380A0, 387D0, 97600, 97740, 970A0, 973C0,
14EB50, 1E1960 and 1DF890 read by the pilot): `[root+0x4788]` is the GX parameter registry, `GXRoot(+0x41C0)+0x5C8`, a 0x4F8 object
built by 936E0 and reset by 93CE0, which empties its group registry `+0x28` and sets its 26 group slots `+0x30..+0x94` to -1 (`+0x490`
to -1, `+0x428` to 0). 48B80 then calls 26 GPU initializers, each registering its group with 1F7090, whose id is the registry index,
so the call order: LightSet 0 `+0x88`, Fog 1 **`+0x80`**, LightScattering 2 `+0x84`, Env Map 3 `+0x8C`, SSAO 4 `+0x50`, Setting Info 5
`+0x30`, Tone Map 6 `+0x34`, Bloom 7 `+0x3C`, LightShaft 8 `+0x54`, Dof 9 `+0x38`, Anti-Alias 10 `+0x4C`, MotionBlur 11 `+0x48`,
FeedBackBlur 12 `+0x44`, ColorGrading 13 `+0x40`, ShadowParam 14 `+0x58`, WaterInteraction 15 `+0x74`, Dominant Light 16 `+0x78`,
AOFields 17 `+0x7C`, AOFields Model 18 `+0x94`, SeaWave 19 `+0x6C`, Snow 20 `+0x70`, Snow Model 21 `+0x90`, Glow 22 `+0x5C`,
ColorParaffin 23 `+0x60`, Lens Simulation 24 `+0x64`, Cloud Shadow 25 `+0x68`; each slot written by its initializer (no other writer
found among the 173 functions that load the registry, `writes.txt`). The native builds the registry and calls 93CE0 (`20_startup_phases.inc`) but not those initializers, so every slot stayed -1,
and the readers that fall back to an uninitialised local (380A0 and 387D0 Fog, 31A20 LightSet, 3C900 LightScattering, 40DD0 Env Map,
45CA0 Snow Model, among 36 readers of the six slots `+0x80..+0x94`) wrote `+0x428/+0x490` with it: Friede's crash, and wherever the
local was small, a silent write inside or near the registry.

**The fix** (`20_startup_phases.inc`, after 93CE0): the six slots `+0x80..+0x94`, whose 36 readers were listed, written with the
original's ids; the registry left empty (its groups' fields and editors are the initializers' work); the other 20 slots left at -1, as
before, until their readers are read (the step review's M1). READ (the pilot, after the step review): 1F7090's callers are exactly the
26 initializers (and five data rows), its id is the registry's size before its push; 964F0's callers are only five of them; 6CAD0 only
stores the id in a bank (`+0x1C`) and allocates its entries; 151810 indexes the `+0x218` vector, bounded. So with the ids set, the readers
write `+0x428/+0x490[id]` and insert keys in the maps `+0x2E0/+0x330` (97600, 97740, 96560, 973C0) as the original does, find nothing in
`+0x290/+0x2E0/+0x330/+0x3D8` to act on, and index the vectors `+0x198+0x80` and `+0x218` (1E1960, 1E1C20, 151810), which the
initializers' callees fill (1DF890), so empty natively and bounded; a reader that fell back to 0 already met that state with id 0.
INFERRED: that no reader of the six reaches the registry otherwise (their second-level callees scanned for the strides, not read in
full); 18EE30's writes to `+0x70/+0x74` taken as a bank's, as 134150's and 191D90's are (the step review's L3). A row
`native_gx_param_group_ids` (`slots` 6). Witnessed: Friede's intro played natively with no fault (`i140-e1`, below).

**The player's face white and flat in cutscenes** (Vordt's intro 30000030 and the ending 41000000, `i139-t3b-c01-vordt-nat`,
`i139-t3b-c30-ending-nat`; the hair black; the embers on the body missing in the ending). OBSERVED in the journals: the face parts'
specular textures (`FG_A_01xx_s`, `FC_M_000x_s`) on the white fallback (`native_texture_white_fallback`, as in play), and
`native_asm_part_missing_binding` / `_missing_inputs` rows (241 to 342 a run). In Vordt's run (`startup.jsonl`): the cutscene's
characters are drawn through the ChrAsm parts (`native_asm_part_draw_sources` during it), whose FaceGen texture is the face composed
for the part's owner `part+0x48` (`09_model_admission.inc`, `NativeComposedFace`); ten faces initialised and nine composed in the run,
one composed at the cutscene's start. So the face exists; which owner the cutscene's player part names, and why the lookup gives the
white texture, was not known; the row `native_face_unbound` (sparse; behind `face-unbound-observe.flag` since the final review, on by default in the witnesses below) was written to name it.

**The witness `i140-e1` (2026-11-13 19:10-19:25, the ASUS, offline, build `build-e3`, `Run-I139Cutscenes -NoReference`, log
`evidence/I-0140/runs-e1.log`, sheets `evidence/I-0140/sheets-e1/`)**: Friede's intro 45000000 **played natively with no fault**
(exit 0, `witness_cutscene` handed, `native_gx_param_group_ids` 6), Vordt's intro and the ending too. In the three, after the
cutscene's row, `native_face_unbound` « no_owner » for `FG_A_0100` (the player's face) under entity table `2951FF8` (a disp entity's
item, SprjFD4ModelItem), `draw_colour` false (its rows at counts 10,000 to 16,000 only: the bindings without an owner shared one
counter with `FC_M_0112`, a hair with no owner, as in play, which reached its first rows before the cutscene); the ChrAsm parts' rows
only at load (« not_composed » before the first composition). READ (`decompiled/DS3-1152-F-00D3DA20.c`, 282-306): D3DA20 takes for **every item** it
draws the FaceGen vertex patch `[item+758]`, the FaceGen texture `[item+770]+0x30` once `+0x91`, and copies `item+0x780..0x78C` to the
group's `+0x40`; the native took the three in the menu views only (`09_model_admission.inc`, 1169 and 1177). **The fix**: the three
taken in every view.

**The witness `i140-e2` (2026-11-13 20:19-20:34, the ASUS, offline, build `build-e4`, log `evidence/I-0140/runs-e2.log`, sheets
`evidence/I-0140/sheets-e2/`: the e1 row, the e2 row, the original's t3b row)**: Vordt's intro, the ending and the High Wall's
30000000 (which the tour saw crash on `main` at the same 97600+0x1E, `i139-t3c-c30000000-nat`) all exit 0 with no fault. The player's
face item now names its FaceGen owner: under `2951FF8`, `native_face_unbound` only « not_composed » rows at counts 1 and 2 per
owner (six owners in 30000000) and no « no_owner » row for `FG_A_0100`; the row repeats every 2,000 counts, so each owner was left
uncomposed fewer than 2,000 times (a short white face at a new owner's start not ruled out). By eye on the sheets: Vordt's face shaded,
the hair and the skin their colour, the face lighter and pinker than the original's; in the ending the face and hair back.
**The map pieces** (the change takes `item+0x780` as the draw colour of every disp item, map pieces included, as D3DA20 copies it to
the group's `+0x40` for each): `sheets-e2/arrive-e1-e2.png`, the arrival views at the High Wall and the Kiln before and after
(e1, e2), the same by eye, YAVG 55.6 / 54.9 and 41.0 / 41.0 (ffmpeg signalstats on the game's 1280x720; the bonfire's fire and the
character's pose differ between the runs); and the first `native_face_stream` row of the three e2 journals comes after
`witness_cutscene` (e.g. line 52571 against 51067), so no map piece of those areas carries a FaceGen patch `[item+758]`. Left (OBSERVED on the sheets): the body much brighter than the original's once the flame takes in the
ending (the traced light of the scene's fire, not read), and the embers on the body and the trousers still missing.

The embers: not read yet (an effect, a decal, a material parameter: the `MaterialParam` act, `140CC0FB0` → `140C3AE30`, a virtual per
model, is a candidate).

**Merged** `82cd0d19` (2026-11-13 21:24); the launcher rebuilt; start check `start-82cd0d199` passed (21:42-21:43, 340 world frames,
restored); the disk rotation (D-0260) run after it: 5.8 GB of 21 build folders older than 48 h deleted, 337 GB free.

## Tranche 2: the passes 0x21 (merged with it: the embers' readings so far)

**The embers (2026-11-13 evening).** OBSERVED: the ending's MQB (`remo_s41_00_0000`, extracted by a Sonnet subagent with BinderTool
from Data1, `evidence/I-0140/remo/`: strings, properties and the scripts; its s30_00_0030 dump equal to I-0069's) holds 27 effect
records: `sfxId` 741000 22 times (slots 1 to 6 in each cut), 741001, 741002, 841030 (twice), 1011, every `dmypolyId` -1; no material,
burn or decal property name; the Remo act classes as `Custom<n>_<i>` tokens (192, 208, 8, 9, 236, 96, 160, 214, 128), not mapped to
their acts. The effect census of the ending (`i140-e3`, native, `effect-census`; `i140-e4`, the original, with `ffx-draw-observe`,
needed for the original's census, which `i140-e3`'s original run lacked): no window of the cutscene holds a 0x2D draw of another bank
than the default in either, so the embers are not among the census's non-default 0x2D draws; what draws them in the original is not
known. Next for them: a capture of the original's frame (RenderDoc, WORKFLOW « Tools ») at the ember view, its draws beside a native
PIX capture.

**The passes 0x20/0x21.** OBSERVED on the tour's pictures (`sheets-0x21/z61-arrive-zoom.png`, `i139-t1-z61-sister-friede`, the
arrival at the Sister Friede bonfire, 10,293 classic draws of 0x20/0x21 captured and not drawn natively): the burning debris and the
fires on the snow show their flames in the original, natively only their glow. A trial behind `effect-pass21.flag`: pass 0x21's
classic draws drawn first in the effect stage, before the 0x29 rows (which its `before_pass` does not draw), with the bank's depth
write kept (pipeline key kind 3), as STUDY-EFFECT-PASSES-20-21 reads mode 1; `ffx-record.flag`'s pass witness names the blend and
depth entries of the 0x20 and 0x21 draws.

**The witness `i140-e5` (2026-11-13 23:20-23:31, the ASUS, offline, `build-e8`, `Run-I139Tour` at `z61-sister-friede=923,445`, nothing
kept, log `evidence/I-0140/runs-e5.log`, sheets `evidence/I-0140/sheets-0x21/e5-*`: the rows e5a, e5b, the original)**: all three exit
0 with no fault. Without the flag (`i140-e5a`, `ffx-record`): `other_passes` 6,726, and every draw the witness counted
(5,512 of them at its last row) is « 21/draw/8/rt1/d1E0 »: pass 0x21 only (no 0x20), blend `+0x08` (off), depth `+0x1E0`, as the study read
mode 1. With it (`i140-e5b`): `other_passes` 0, no pipeline failure, no skipped draw. By eye on `e5-debris-zoom.png` (the debris
right of the player at the arrival): the flames on the snow beside the debris absent in e5a, present in e5b and in the original;
`e5-sweep-2-1.png`: the fires under the far tables show in e5a too (the 0x29 fires), e5b adds flame tongues there (one still each of
animated flames: weaker than the debris), no other change seen. A region mean does not show it (YAVG 176.2,
178.0, 175.3 over the 260x110 crop at (780, 500) of the game's 1280x720, `sheets-0x21/yavg-debris-crop.txt`: the snow dominates). **The fix**: pass 0x21 drawn by default, the flag removed (`424ad5a3`).

Its step review (fresh, on `e03ae881..424ad5a3`): no high finding. M1: the effect depth copy (t3) and the 0x31 layer's half depth were
taken before Encode, so before 0x21's draws wrote the depth (the original's sub 0x28 runs after its sub 0x21): a 0x31 particle behind
the burning debris passed the layer's test against a depth without them. Answered (`3f131c62`): both are taken after 0x21's draws, at
the first later pass's `before_pass` (before the 0x29 rows, as tranche 15's copy), or after Encode when no later pass draws (the plan
key's copies kept), and before Encode as before when the frame has no 0x21 draw. L1: 0x21 in `EffectReadsDepth` (its draws reading t3,
none seen, read the frame's previous copy within the frame). L2: 0x21's GPU particles keep the depth write too (key kind 4), the
witness names their depth. L3 (0x21's geometry after the TAA base, its history weight lowered by the reactive rule) and the same root's
water copies: left, recorded. With the final review's M2: 0x21 is drawn after the water block and the TAA base, where the original's
sub 0x21 comes before 0x22, 0x24 and 0x27 (INFERRED from the key order): an opaque 0x21 effect behind or under a water surface fails
its depth test natively and is missing from the refraction (Next). L4: the comments. L5: `Run-I139Cutscenes` refuses
`-NoNative -NoReference` together and a malformed flag list before the first launch; its comment says `-EffectFlags` replaces
`test-no-ai`.

The step review of that answer (`424ad5a3..3f131c62`): H1, frames with 0x21 draws declared the graph's passes in another order without
a bit of the plan key, so the executor (D-0171) would throw « the frame's passes differ from its plan » when frames with and without
0x21 alternate (the i140-e5 build predates it). Answered (`c2ed5dca`, `35ba9a15`): bit 61 of the key for a 0x21 frame with depth work,
the re-declaration `effects_after_depth` and the rebind only with that work; L2, the preparation marked done at its end, a retry after
a failure inside it throwing (outside the effects' catch) instead of redoing it (the next review's L-a: a pass declared twice would
have been cached as a plan). Its next review (`3f131c62..c2ed5dca`): no high or medium finding, the two lows answered there.

**The witness `i140-e6` of the final form (2026-11-14 00:38-00:46, the ASUS, offline, `build-e13` = `75eeca51`, `Run-I139Tour
-NoReference -NoKeep`, `ffx-record`, log `evidence/I-0140/runs-e6.log`)**: at the Sister Friede bonfire, exit 0, no fault, no
`graph_barrier_mismatch`, no `native_render_job_failure`; `graph_shadow_summary` `effects_after_depth` 3,657 of 8,401 frames (the 0x21
frames with the depth work), `effect_depth_copy` 8,389; the effect stage `other_passes` 0, no skipped draw, no missing texture, no
pipeline failure; its arrival still (`sheets-0x21/e6-debris-zoom.png`: the rows e5a, e6, the original) shows the flames on the snow
beside the debris (one still of animated flames, smaller than e5b's). At Firelink (`z01-firelink`, no 0x21 draw): exit 0, no fault,
no mismatch, `effects_after_depth` absent. Not observed: the depth copy's own effect (a 0x31 particle behind the debris).

**Tranche 2 merged** `b8b76da0` (2026-11-14 01:03); the launcher rebuilt; start check `start-b8b76da05` passed (01:20-01:21, 374
world frames, restored); the disk rotation not run (another session's compiler ran).

## Tranche 2b: a reversal tried and withdrawn

Read the night of 2026-11-14: WATER-TESS-WAVE1-READY 1.4's OBSERVED GPU order (the Firelink listing and `i63-live-13/m37-s4`: the
readable depth's copy and two `DepthScale2x2` draws, then the copy C2, then « the post-water forward rows and the effects ») was taken
to put the 0x21 group after the downsample, and tranche 2's deferral was removed (`db507c16`). Its step review (fresh, on
`20f4f9c0..475ea89d`) found that reading unsupported: that Firelink listing has no 0x21 draw and no pass numbers, the rows after C2
bind the frame copy at t45 (sub 0x29, under whose header C2 is recorded), and the 0x1F/0x21/0x22 split there is INFERRED; the VERIFIED
key (PASS-CHUNKS-STUDY points 1 and 3: `group<<29 | pass<<22 | sub<<16 | order`, the tables stable-sorted ascending) and STUDY-PASS-31
§1 (the depth copy and the downsample are sub 0x28) put sub 0x21 before 0x24, 0x27 and 0x28. Reverted (`7c42bb68`): tranche 2's form
as merged stands. The final review's M2 (0x21 drawn after the water and the TAA base, where sub 0x21 precedes the water) stands too,
in Next. Not merged; nothing of it reached `main`.

## Tranche 3: the ending's embers (the Remo MaterialParam rows)

**A lead (2026-11-14, static).** OBSERVED in the native ending's journal (`i140-e2-c30-ending-nat`, `native_asm_part_draw_sources`):
the player's trousers (`FC_M_0000_Underwear`, family `SAT_Equip`) sample `SYSTEX_DummyBurn_m` at t5 and `SYSTEX_DummyBurn_em` at t6.
READ (a Sonnet subagent on the existing disassembly `evidence/I-0119/artifacts/census/asm/SAT_Equip__SAT_Equip_Gbuf.ppo.asm`, copied
to `evidence/I-0140/sat-equip/`): the Gbuf programme writes an ember glow into its sixth target (o5) as t6(u, v) x `cbMatDynParam`
+0x270 (`SAT_Equip_snp_0_float_4_0`) x `cbMtdParam` +448 (`float_1`) x a rim term x (1 - the albedo's luminance x `float_6`), u from
t5 and a scrolling time, v from the vertex's TEXCOORD2.x (a per-vertex burn progress) by `float_9` clamped to `float_11..float_12`;
no branch: with `float_4_0` at 0 the glow is 0. `FC_BlendRate` (+0xC0, the Burn MTDs' burn) is unused there. The native writes
`float_4_0` from the authored generated parameters, moved toward a parameter-animation row when the draw's table holds a universal row
or one for the material's index (`09_model_admission.inc` 1109, « the embered armour's cinder glow », witnessed in play on TestAnim).
INFERRED then, answered by `i140-e8` below: the ending's embers are that glow, the cutscene driving `float_4_0` through a row the
native did not see on the cutscene's items.

**The cause (2026-11-14, static; a `ghidra-research` subagent, its scripts and outputs `evidence/I-0140/ghidra-materialparam/`; the
pilot re-read the deciding bytes with GhidraMCP).** The Remo `MaterialParam` act (slot 9 `140CC8C10`) edits, through `140CC0FB0`
and the item's component registry (`[A+0x10]` the `SprjFD4ModelItem`, `(item+0x18)->vt+0x30` = `140D29380`: `mov rcx,[rcx+0x38]`,
i.e. `[item+0x50]`), the « MaterialParamModifier » component X (vtable `294FF98`, made by `140D29220`), whose modifier X+8 holds its
own table (X+0x10): kind-0 rows keyed by the parameter name's CRC, universal when the act names no material (`140D2A6F0`, the row
layout the native already reads). `140CC0720` writes one such universal row, « CinderIntensityScale » = 1 (READ: the string at
`142898A98`). At the draw, D3DA20 sets the descriptor's parameter owner to `[item+0x6E8]`, then lets the item's components fill the
descriptor (`(item+0x18)->vt+0x60`): X's vt+0x10 is `140D2A790` (READ: `lea rax,[rcx+8]; mov [rdx+0x20],rax`), so with X the owner
is X+8. The native read `[item+0x6E8]` only: it never saw the cutscene's rows, and the cinder glow (`SAT_Equip_snp_0_float_4_0`)
stayed at its authored value. **The fix** (`475ea89d`, `09_model_admission.inc`): a disp item's owner is X+8 when `[item+0x50]`
holds a component of vtable `294FF98`. Witnessed below (`i140-e8`).
INFERRED, from the subagent: the native's `material_index` of a row (+0x78) is the material name's CRC (`1B0AB0`), as the gate
compares it with the material's +0x60.

Its step review (fresh, on `20f4f9c0..475ea89d`): the transcription is correct and safe for disp items (every read through `Read`,
the vtable checked, the items without the component unchanged, the record's reuse comparing rows). MEDIUM: the ending's journal shows
the trousers' `SAT_Equip` draws as ASM parts (`native_asm_part_draw_sources`, after the cutscene's row), whose owner is
`[entity+0xB18]+0x10*i` (`D01C30`, READ), not this path; the cutscene's player faces are disp items. So which path draws the
trousers in the ending is open: the witness row `native_material_param_component` (`light-observe.flag`, the model and the rows of
the first items whose owner the component gives) answers it with the ending's run. Lows: the deciding bytes saved in
`evidence/I-0140/ghidra-materialparam/pilot-readings.md`; only the slot `[item+0x50]` is consulted, not every component's vt+0x10
(another component writing the descriptor's +0x20 not looked for); the names `component`, `component_vtable`.

**The witness `i140-e7` (2026-11-14 02:16-02:21, the ASUS, offline, `build-e16`, the ending natively with `light-observe`, log
`evidence/I-0140/runs-e7.log`, sheet `evidence/I-0140/sheets-e7/c30-ending.png`: rows e2, e7, the original)**: exit 0, no fault.
`native_material_param_component` from the cutscene's start: the cutscene's character items carry the component (`FG_A_0100`,
`FG_A_1602`, `FG_A_1502`, `FG_A_2012`, `FG_A_7002`, `FC_M_0000`, one row each), so the fix takes their owner; but the embers are
were judged absent by eye on the reduced sheet that night (corrected below: the trousers' crop shows them); the body was as
bright; and the generated parameters' witness had spent its eight rows in play before the cutscene. Not settled: which of the cutscene's parts draw the trousers and with what `float_4_0`, nor whether the descriptor's
+0xA0 texture choice (D3DA20: `[model]+0x358/+0x360` when `item+0x6F2`, which the native takes for the face only) carries the burn
texture (the native binds `SYSTEX_DummyBurn_*`). The two witnesses were widened (`887f7d5c`: the animated parameters per model,
material and value; the component's items with their +0xA0 choice) for the next run.

READ (a `ghidra-research` subagent, `evidence/I-0140/ghidra-d3da20-a0/`): D3DA20's descriptor +0xA0 is a per-index texture provider
(draw record +0x1D0 → `GXFlverModelDC+0x1B0` → 140140680, whose binders 1401ADEB0/1401AD990 ask it for each material texture entry's
key before the material's own): `[model+0x358]`/`[+0x360]` are two views of the model's own FLVER textures (the second with « _l »
variants), chosen when `item+0x6F2` (fixed at the part's construction from a descriptor table's bit, statically all zero), and the
FaceGen set; no burn texture there, so not the embers' path. The `ModelDrawParam` act (140CC9540 → 140D3BEA0, consumer 140CC5D90)
writes (bank, row) pairs with a blend and a fade into a cutscene part's slots `part+0x38..+0xAC` (four slots of six words, dirty bit
0x44), never reset by the act at the scene's end (no reset write found): a lead for the tour's « player wrong after a scene », and
perhaps for the embers. READ next (a `ghidra-research` subagent; its outputs kept in `evidence/I-0140/ghidra-modeldrawparam/` are four offset scans, its decompile
readings not kept, so READ by the subagent): those six words are
GPARAM keys, `{area key, record id}` current and previous with a blend weight and duration, four a part (key 0 the light set, key 1
the fog, key 2 the Snow Model rate, key 3 a reflection-like repository); the part update 140C4D240 forwards keys 0 to 2 to its draw
entity's draw-state group (+0xF8, +0x110, +0x70), which the main view's record (57A20 → 137D60 → 137910) turns into the per-draw cbLight
and cbFog records (the native's `NativeLightKey`, D-0180 C4) and the snow rate. So ModelDrawParam picks the cutscene characters' light
set and fog by the cutscene's GPARAM (area 2AABBVVVV) records, with no reset at the scene's end (the act writes only when a key is set).
INFERRED: the ending's body brightness follows the cutscene's light-set records, which the native does not read (the R pilot's
finding: `Banks::File` looks for `mAA_BB_VVVV`, its fix on its branch); the embers stay unexplained by it.

**The witness `i140-e8` (2026-11-14 03:50-03:55, the ASUS, offline, `build-e18` = this branch with `main` `d365e26f` merged in: R's
cutscene GPARAM bank and I-0138 tranche 4's parts 4b and 4c; `light-observe`; log `evidence/I-0140/runs-e8.log`, sheet
`evidence/I-0140/sheets-e8/c30-ending.png`: rows e7, e8, the original)**: exit 0, no fault. From the cutscene's row, the component's
items are the whole cutscene player (`LG_M_0000`, `BD_M_0000`, `AM_M_0000`, `HD_M_0000`, `HR_A_0101`, `FC_M_0000`, the FaceGen parts),
one MaterialParam row in each component's table, their descriptor +0xA0 choice unused (`use` 0); `native_generated_animated` for `FC_M_0000`'s materials #07#, #13#, #18#,
#19# at offset 624 (`float_4_0`) with the word 1, the universal row reached (four materials, INFERRED the Underwear meshes; no ASM-part row of
`FC_M_0000_Underwear` after the cutscene's row in e8, so the trousers come by the disp items there; e2 had such rows after it, the
path then not recorded per row: the step review's MEDIUM answered within that limit). The trousers' crop of the first view
(`sheets-e8/trousers-e2-e7-e8-orig.png`, 260x420 at (0, 300) of the game's 1280x720, `sheets-e8/regions.txt`): by eye, the ember edges
absent in e2, faint in e7 (the fix without R's bank) and present in e8, as the original's (no region mean measures them: SATAVG 10.2,
11.4, 8.5, 5.1); YAVG 52.6, 56.5, 76.7, 82.6 (the trousers lit by the
scene's light set in e8, R's bank). The body region of the second view (300x400 at (420, 200)): YAVG 214.8 (e2), 214.8 (e7), 197.2
(e8), 164.8 (the original): R's bank lowers it, still brighter than the original's. INFERRED: the embers came through the component's owner (the
word 1 witnessed, the Gbuf programme's formula; e7 shows them without R's bank, though e2's build also lacks tranche 2 and I-0138
part 4a), the light of the scene through R's bank; the body's remaining brightness is open.

**Tranche 3 merged** `d0d7dc96e` (2026-11-14 04:24); the launcher rebuilt; start check `start-d0d7dc96e` passed (04:40, 357
world frames, restored); the disk rotation run (16 build folders, 5.2 GB).

## Tranche 4: pass 0x21 at its key place (the final review's M2)

The original draws the forward rows and the effects by its sort key, whose subs come in the order 0x1F, 0x21, 0x22, then the water's
0x24 (WatDpt) and 0x27 (surface), 0x28 (the layer's half depth) and 0x29 (VERIFIED key, PASS-CHUNKS-STUDY; that the effects of sub
0x21 sit in that order with the rows is INFERRED from the key, as for 0x29). Tranche 2 drew pass 0x21's effects inside the effect
block, after the 0x22 rows, the water and the TAA base. **The change** (`cc791f3d`, `scene_pass_frame.cpp`): on a frame with 0x21
effect draws and its rows sorted (`pass21_order`, plan key bit 62), the 0x1F rows are drawn, then the effect stage starts: the 0x21
rows are drawn among pass 0x21's effects by key (`before_draw`, as the 0x29 rows are), and the forward block's rest (`forward_tail`:
the 0x22 rows, the water with its copies, the TAA base) runs once at the effects' first later pass, or after them; the depth copy and
the half depth after it, as before. Every other frame records and declares what it did (the block's code shared, `forward_tail(0)`).
`SortNativeForwardRows` now gives the ends of the 0x1F and 0x21 rows (`forward_1f`, `forward_21`).

Its step review (fresh, on `bb3f945a..cc791f3d`): no high finding; criteria 1 to 6 held (same commands and passes off such frames;
one pass sequence whether the tail runs in the hook or after Encode; no row lost or drawn twice; rebinding; the copies' places; the
row ends). M (a row failing before or inside the tail left its passes undeclared, the executing plan then throwing, the row's failure
taken for the effects'): answered (`forward_tail(from, &row_failure)` keeps the failure, draws nothing after it, still makes its copies
and declares its passes). L1 (the effect scans' stage 3 and the effect-measure time take the 0x21/0x22 rows and the water on such
frames) and L2 (on a water frame no copy precedes 0x21: a 0x21 statement sampling t4 reads the copy's last content, none seen
sampling it, not checked: no witness reads 0x21's textures): recorded in the code as policies.

**The witness `i140-e10` (2026-11-14 05:38-05:45, the ASUS, offline, `build-e21` = `1e38546e`, `Run-I139Tour -NoReference -NoKeep`,
`ffx-record`, log `evidence/I-0140/runs-e10.log`)**: at the Sister Friede bonfire and at Shared Grave, exit 0, no fault, no
`graph_barrier_mismatch`, no `native_render_job_failure`, restored. `graph_shadow_summary`: the new order (`forward_1f`,
`effects_after_rows`) on 3,667 of 8,401 frames at Sister Friede and 3,035 at Shared Grave, where the water's passes run on 7,772 (so
at least 2,406 frames with both); the effect stage's totals with no skipped draw, no missing texture, no pipeline failure, no
`other_passes`, `last_failure` empty. The arrivals (`sheets-e9-e10/e10-arrivals-e6-e10-orig.png`: columns e6, e10, the original) by
eye on one still each: the debris' flames as in e6 at Sister Friede; Shared Grave as the tour's `i139-t1-z71-shared-grave-nat`
(the sheet's first column there: e6 did not run it). Not observed: a picture where a 0x21 effect sits behind or under the water
(the case the order changes), and a 0x21 forward row drawn in the hook (no counter separates them; its final review's L5).

**Tranche 4 merged** `a1a4138ab` (2026-11-14 06:25); the launcher rebuilt; start check `start-a1a4138ab` passed (06:41, 341
world frames, restored); the disk rotation refused (another session's compiler ran).

### The tour's cutscene defects and the player's red-orange, as read before tranche 5

The tour's cutscene defects (relayed 2026-11-14 from I-0139 tranche 3, `sheets-t3c/31000010.png`, `32000010.png`). **Seen again on
`main` `d0d7dc96e`** (`i140-e9`, 2026-11-14 05:33-05:38, the launcher's build, 31000010 natively, `light-observe`,
`face-unbound-observe`, exit 0, restored; `sheets-e9-e10/e9-c31000010-t3c-e9-orig.png`: columns the tour, e9, the original): the
flat beige standard-bearer and the white sky are gone (the figure dark with its embers, as the original's: tranche 3 and R's
cutscene bank, INFERRED: no run separates them); left: the far valley a flat grey band (the original's castle and forest lit
there) and the white wyverns at 30 s almost invisible (faint translucent shapes). 32000010 not seen again. **The player « wrong
after a scene » is not left by the scene** (OBSERVED on the tour's pictures `i139-t3c-c31000000-nat`, `-c31000010-nat`,
`-c33000000-nat`, `sheets-e9-e10/c31_33_before_after.png`): `stay.png`, 2 s before each scene, already shows the red-orange skin
and dark trousers that `cutscene-90.png` shows after it, the original pale in both, as the tour saw out of any scene: so not a
state the scene leaves (not ModelDrawParam's keys). Its cause is INFERRED only: the traced lighting's (D-0262, R's; handed to R
through the coordinator, 2026-11-14 05:10) is a reading, and I-0113 tranche 34 measured the same « saturated orange skin » in the
character creation's view with and without the rays (`i66-native-i113-t34-face-nort`, `no-rt-scene`) and called it the raster's
(E's). To settle: a world witness with `no-rt-scene` against one with the rays, at High Wall or the Road of Sacrifices (the
integrity review's M). That flag no longer exists (the raster world removed in I-0132 R11e); in its place **`i140-e11`**
(2026-11-14 06:42-06:45, the ASUS, offline, the launcher's build of `main` `a1a4138ab`, the Road of Sacrifices bonfire,
`world-pictures`, `gbuffer-pictures`, exit 0, restored; `sheets-e11/e11-display-hdr-gb2.png`: the presented picture, the lit HDR,
GB2; `sheets-e11/regions.txt`, by Convert-GBuffer.py): on the player's trousers (two 36x80 and 35x80 boxes at (592, 470) and (655,
470) of 1280x720, frame 3000) the material colour GB2 is a neutral pale grey (0.478, 0.455, 0.445 and 0.483, 0.462, 0.451), the
lit HDR red-dominant (0.077, 0.035, 0.035 and 0.109, 0.040, 0.040), the body backlit by the bonfire. So the warm, dark trousers
come after the G-buffer, in the lighting (the trousers are not an SSS material), not from their material (OBSERVED at that
bonfire, one frame; the skin's SSS and the outdoor red-orange of the tour's stills not separated). That supports the lighting
reading (R's); I-0113 tranche 34's measure stays the creation's menu view, raster.

## Tranche 5: the 20 other GX group slots; 31000010 and 32000010 read

**The 20 other slots** (the previous tranche's « Next » 3). READ (the pilot, GhidraMCP, 2026-11-14): the 80 readers of the 20 slots
(`evidence/I-0140/ghidra-gx-slots20/slot_table.tsv`) are four a slot: the bind (A) and its apply (C), reached only through 49D00
(49EB0, with no direct caller, is its tail), which `BindNativeGparamScene` replaces natively; the release (D), on the initializers'
group objects (through 49450, the children's release, INFERRED; whether it runs natively not read; its id indexes only the
bounded, empty `+0x198+0x80` and `+0x218`); and the withdraw (B), inside `GparamFile_Withdraw` 4A060, reachable natively (READ, not
observed): its callers D26800 (the retire), D26DD0 (the unapply) and CDCCD0 test the file's GX handle `+0x70` (never set
natively), but D27040 and D26E40 do not: they take an area out of the holder's area list (written by the genuine D26AB0 and D26C00)
and call 4A060; D27040 is called by the area controller CE6350's layer-1 file switch (`evidence/I-0066/queries/ae/q17/140ce6350.c`
line 60; not its unbinding, which tests `+0x70`) and by LoadMapOverrideGparam 619BD0, D26E40 by CE6230. In `i140-e12`'s 31000010
journal no such file switch shows (each area one MapOverride binding, none after). With a slot at -1 each B (Lens
Simulation's among them: 140185280, which 4A060's tail 4A210 reaches, Ghidra printing it inside 4A060) took a stale register or
stack word as the group id and looked it up in the registry's map `+0x2E0` (96B70 for Lens Simulation), which the six groups written in
tranche 1 fill: a stale word equal to one of their ids could clear that group's (area, bank) entry (INFERRED, never observed). The
readings: `evidence/I-0140/ghidra-gx-slots20/pilot-readings.md`. Limit (the step review's L3): the slot scans cover the functions that
hold a `0x4788` displacement; a reader reaching the registry as `GXRoot+0x5C8` or as an argument is not covered (INFERRED: none, as for
tranche 1). **The fix** (`67887543a`, `20_startup_phases.inc`): the 26 slots written with the original's ids; the 20 new ones then
find no node, no bind having run natively for those groups (in the original the withdraw finds its bind's node);
`native_gx_param_group_ids` should say `slots` 26, the refusal `native_gx_param_group_slot_not_reset` now checking 26 slots (a start and
a travel on the final build before the merge; the D27040 path itself not witnessed). **The witness `i140-e13`** (2026-11-14
08:35-08:39, the ASUS, offline, `build-e26`, `Run-I139Tour -NoReference -NoKeep` to the Road of Sacrifices, `test-no-ai,light-observe`,
log `evidence/I-0140/runs-e13.log`): exit 0, no fault, restored; `native_gx_param_group_ids` `slots` 26, no
`native_gx_param_group_slot_not_reset`; eight `native_gparam_scene_binding` rows (the travel's bindings).

**Tranche 5 merged** `0b162a28b` (2026-11-14 09:04); the launcher rebuilt; start check `start-0b162a28b` passed (09:19, 373
world frames, restored); the disk rotation refused (another session's compiler and Ghidra ran).

**The witnesses** (`68c7a5ac6`): `pictures-from-cutscene.flag` (the world, G-buffer and control-view pictures counted only from the
witness cutscene's hand-over) and `native_character_fade` (`light-observe` in `i140-e12`, its own `character-fade-observe.flag` since
the step review's L4: the character models' admissions by the opacity of their first intent, a row each 600 of a name). Test tools only.

**The witness `i140-e12` (2026-11-14 07:41-07:56, the ASUS, offline, `build-e24` = `68c7a5ac6`, `Run-I139Cutscenes -NoReference`,
flags `test-no-ai,world-pictures,gbuffer-pictures,pictures-from-cutscene,rt-view,light-observe`, log `evidence/I-0140/runs-e12.log`,
sheets `evidence/I-0140/sheets-e12/`)**: 31000010, 32000010 and 41000000 natively, each exit 0, no fault, restored; each wrote its
world, G-buffer and control-view pictures in the cutscene (16, 14 and 16). OBSERVED:

- 31000010's far valley and white wyverns (the tour's « flat grey » and « almost invisible ») are drawn: the control view's classes
  agree with the raster depth over the valley (10500) and over the wyverns (12000), and the traced scene shows the castle, the forest
  and the wyverns with their materials (`c31-rtview-*.png`); their G-buffer holds their textured albedo and normals
  (`c31-10500-*.png`, `c31-12000-*.png`: the presented picture, the lit HDR, GB2; GB0, GB4, the sun's mask). At 10500 the traced
  sun's mask puts the whole valley, the parapet and the player in shadow (only the far mountains and the sky lit), the lit HDR of the
  valley near black; at 12000 the mask lights most of the scene and the wyverns, and the lit HDR is a dim, even, green-grey haze in
  which the white wyverns barely stand out (INFERRED: the fog's weight, not their light).
- 32000010's dragon (black at 5 s): its G-buffer holds its textured albedo and its normals, its lit HDR is black, and the traced sun's
  mask has it in shadow (`c32-9300-*.png`; the integrity review measured 0.65 of the dragon's region in shadow); the original
  (`i139-t3c-c32000010-orig`, `c32-c31-e12-vs-orig.png`'s right column) shows it lit, by eye.
- 41000000, the ending (`world-11700`, the shot of the 20 s still: `c41-11700-*.png`): over the body region of e8's measure (300x400
  at (420, 200) of 1280x720, `Convert-GBuffer.py`), GB2 neutral (0.502, 0.447, 0.396), the lit HDR red-dominant (3.64, 0.64, 0.28);
  the presented still's YAVG 194.0 (ffmpeg signalstats, the same crop of `cutscene-20.png`), against e8's 197.2 and the original's
  164.8 (`i139-t3b-c30-ending-orig`). INFERRED: the flame in the hand (a traced local light) over-lights the body; the cinder glow
  (tranche 3) not separated in that HDR.
- The rays take one sun, the engine's shadow state's (`native_shadow_state` `sun` (-0.772, 0.485, -0.411), the light set
  `[131000000, 0]`), while the per-draw light keys in 31000010 are `[131000000, 300]` for the valley's pieces (its light 0 (0.982,
  0.104, -0.156), 1.2) and `[231000010, 110]`, `[231000010, 20]` for the cutscene's characters ((0.539, 0.485, 0.689), 0.4).
- (« dark », « near black », « haze »: by eye on the stills, except the means given.)
- `native_character_fade`: c2021 zero-fade (track 0, geometry weight 0, suppressed) but 27 admissions at partial opacity (about 9
  frames of the three perched entities near serial 12915: `native_character_candidate` admitted at lines 64396-64398, zero_fade again at
  64418-64420), never at full opacity; the flying wyverns, drawn opaque in the G-buffer, come through another path (not named).

INFERRED: the original lights each draw group by its own light set (a vista's or a cutscene's authored sun), the native's traced
lighting by one physical sun, so the valley (10500) and the dragon lie in that sun's shadow; the wyverns (12000), lit, are lost in the
fog. That is ROADMAP workstream 7 step 7's « the
cutscenes' kind-11 lighting rules » (R) and touches D-0262; handed to R through the coordinator and put to Thomas (2026-11-14 08:15).
How the original's deferred pass picks a light set per pixel is not read. The green, washed hue of 31000010 at 20-30 s is plausibly
the cutscene's ColorGrading LUT the native refuses (STUDY-UNREAD-SETTINGS item 1, R's backlog), INFERRED.

## Tranche 6: the Decal materials (ROADMAP workstream 5 step 2)

The GX00 item's « Decal » (material `+0xD0`, parsed natively since I-0100 tranche 6) was left without its class: a Decal material
(the eyebrows `P[ChrCustom_Hair]_al`, the map overlays `M[ARSN]_e`, `_l_e`, `_al`, `_l_al`, `_tr`, `_e_tr`, `_em_Glow`) kept its
non-Decal class natively, so it was drawn as an opaque G-buffer surface (blend mode 0) or by the forward rows (any other mode,
class 2). READ (a `ghidra-research` subagent, `evidence/I-0140/ghidra-decal-class8/`; the pilot re-read 140200840 and the bank's
alpha words at 143D7F280):

- 142130 gives a Decal class 0x10 when its MTD's `+0x110` (forward) is set, else 8, before its other tests (so never 2).
- 57A20's main rows are made pass by pass: `{5,3,5}` (or the split pair), then `{6,3,8}`; the scene sorts the recorded rows
  by its key with the sub-pass above the distance, so every engine-5 row is drawn before any engine-6 row.
- 140850 sets no state of its own for class 8: a class-8 G-buffer row takes what a class-1 row takes. Its blend is the material's
  blend parameter's (140200840, through 140850's upload 2E5C0): mode m, the table 14473E5C0 row 0 `{0,0,1,2,3,4}`, 0 the bank's
  `+0x08` (no blend), any other index plus 0xF (5A120 sets args+0x200 bit 16 for the Gbuf pass), the bank's entries 16 to 19
  (1400D21F0): SRC_ALPHA/INV_SRC_ALPHA, DEST_COLOR/ZERO, SRC_ALPHA/ONE, the same reverse-subtracted, the alpha ONE/ZERO added,
  every target written RGB only. Its depth is 140850's pair: `+0x860` (written) or, with the MTD's `+0x111`, `+0x868` (not written).
- No programme or GX technique depends on the class or on `+0xD0` in 140680/1407D0.

**The change** (`fb7562181`, `eb6851409`): the admission classes a Decal 8 or 0x10 (`09_model_admission.inc`), so the engine-6 row
(which the native already made, class mask 8) draws it after every engine-5 row, a forward Decal moving from the 0x1F row to 0x29
as in the original; every G-buffer row (engine passes 5 to 7) takes the bank's blend for its mode through the whole table
(`NativeEffectBankBlend`, the observed bank) and no depth write for an MTD `+0x111` (`13b_prepare_model_frame.inc`); the Decal rows
take the inactive parameters of class 1 and 4 (a transparent Decal, class 2 before, gains them). A non-Decal material with a nonzero mode is class 2 (forward), so in practice
only the Decal rows blend in the G-buffer. The rays keep their former classes (a Decal never casts; explicit policy). Not modelled:
the per-instance blend override found by name CRC; the G-buffer rows' sort by the pass key within an engine pass (two blended Decals
over one another may composite in another order); `+0x81` (g_GX_bDisableDepthWrite) still drops a G-buffer row's depth write
(the native's explicit policy: 140850's pair does not read it on this path).

Its step review (fresh, on `0af9e316f..fb7562181`): no high finding; the transcription matches the code (the class rule, the table
and its offset, the bank's entries, the depth pair), the engine-6 rows come after every engine-5 row (rows made pass by pass, chunks
joined in order). M1 (a new mutable static failing the thread-affinity gate) and L3 (the bank transcribed twice): the bank's own
entries through `NativeEffectBankBlend`, the whole table, no static; M2 (the Decal rows losing the inactive FC_Wet and
FC_DamagedNormal writes, an incomplete cbMatDynParam refusing the row): class 8 kept with 1 and 4; L1 (no key sort within a pass),
L2 (`+0x81`), L5 (modes above 5): stated above; L4 (stale comments): rewritten.

Its final review (fresh, xhigh, on `main...dd4b70597`): no high finding; the transcription faithful, no regression for non-Decal
materials by construction (a transparent non-Decal is class 2, never in the G-buffer masks), no new static, the gates passing. M1
(the rule of 140200840 transcribed twice): one rule, `NativeMaterialBlendToken` (kind 0xFF, row 0, bit 16), the table read flat as
the original reads it (`64f97d975`). M2 (unseen, the witness not recorded): the witnesses below before the merge. L1 (a class-8
Decal on a fading 0x29 row or a phantom row may carry `+0x111`): `+0x111` takes the unwritten pair in the forward and effect depth
rules. L2 (the `decal` field now equal to `current_special`): removed. L3 (the pair « written » for a Decal without `+0x111` rests on
args+0x188 staying -1 in preparation 3: INFERRED; 140200840's own depth push overwritten by 140850), L4 (the inactive parameters'
sentence), L5 (I-0100 tranche 6 step 4's « decal state » and NPC-FAMILIES-STATIC's class 1/4 for `_tr_Decal` superseded by this
reading for GX00 Decal materials), L6 (the per-instance blend override's source, args+0x208 = `[args+0x1D8]+8`, the table the native
reads as `parameter_animations`: a lead, not modelled): recorded here.

**The witness `i140-e14`** (2026-11-14 10:25-10:37, the ASUS, offline, `Run-I139Tour` at Firelink Shrine: `build-e28` = `eb6851409`
and the original, then `build-e26` (before) `-NoReference`; log `evidence/I-0140/runs-e14.log`, sheets `evidence/I-0140/sheets-e14/`):
the three exit 0, no fault, restored. The travel from Firelink to Firelink left the Travel page open, so every still shows the page;
the world behind it is the same before and after (PSNR 37.4, 44.3, 48.6 dB over the bottom 180 rows of `world-4800`, `-6000`,
`-6900-display`). `native_pass_rows` (frame 9361): 78 rows of the skinned map objects (tag 1001) moved from engine pass 5 to 6, one of
them from 7 to 6, and one other row (tag -1) from 5 to 6 (80 rows at pass 6 after, none before); `content_draws` 490 both; no
`native_selected_matdyn_unavailable` or constant row, no `native_model_draw_prepare_failed`. Which objects those are, and how they
look, not seen: a second witness on a travel that arrives.

**The witness `i140-e15`** (2026-11-14 10:52-11:00, the ASUS, offline, `Run-I139Tour` to the Road of Sacrifices: `build-e29` =
`64f97d975`, the final code, with `world-pictures` and `gbuffer-pictures`, then the original; before: `i140-e13`, `build-e26`, the same
row, with `light-observe` only (another capture configuration); log `evidence/I-0140/runs-e15.log`, sheets `evidence/I-0140/sheets-e15/`:
before, after, the original): both exit 0, no fault, restored. `native_pass_rows` at the arrival (frames 8521-8641, the same eye in
both; the integrity review's reading, its script `sheets-e15/rows2.py`): about 182 rows of skinned map objects (tag 1001) moved
from engine pass 5 to 6 (`[5,1001]` 220 to 38, `[6,1001]` 0 to 185) and three forward 0x29 rows (tag -1) to pass 6 (`[41,-1]` 3 to 0,
`[6,-1]` 0 to 3): these blend in the G-buffer now; at the run's last camera (frames 9061-9241) one row from 7 to 6. In-zone
`content_draws` 490 and 568 in both; no `native_selected_matdyn_unavailable`, no `native_model_draw_prepare_failed`. The stills before
and after the same by eye, PSNR 38.6 to 40.5 dB over the game's 1280x720 (`arrive`, `sweep-1-1` to `sweep-4-1`, ffmpeg psnr): no
regression seen, and no visible gain in these views. The original's rocks carry a green moss the native's lack in both (not read). So the tranche is the original's state, witnessed without fault where it applies; its look on the Decal overlays
themselves (the Firelink objects' 78 rows, the eyebrows) is not shown by these views.

**Tranche 6 merged** `51df978b8` (2026-11-14 11:27, after `main` `a298e3fc1`, R's HDR10, merged in); the launcher rebuilt (exit 0,
its tests); start check `start-51df978b8` passed on the OLED in a 2560x1440 window (11:42, 265 world frames, restored).

Its integrity review (on `main...2d3df79b3`): no high finding; nothing weakened, no scene-specific case, no work cut. M (« one row moved,
nothing else » at Road of Sacrifices, from a frame the runs do not have): corrected above from the arrival's frames. Lows: the other
capture configuration of the « before » run (stated), Firelink's two other rows (corrected), the in-zone `content_draws` (corrected),
the Evidence section (extended).

## Tranche 7: the traced sun's shadow source (the view's ShadowParam)

R's readings (« Next » 2 below) re-read by the pilot on the listings (GhidraMCP, 2026-11-14; `decompiled/DS3-1152-F-000F90F0`,
`-00088A10`, `-001DAF40`, `-0018C590` and the disassembly where the C differs). READ:

- 88A10 (the view context's ShadowParam record R): with the context's byte `+0x3ED0` it returns the record `+0x90` as it is; else it
  looks up the keys A `+0x3FA8` and B `+0x3FB0` in the ShadowParam bank (`[GXRS+0x4670]`, 18C590) and blends them at the weight
  `+0x3FB8` into `+0x90` (DAF40). 843C0 constructs `+0x90` with 839F0's defaults.
- 18C590: `{-1, -1}` reads `{1, 0}`; a key the bank lacks reads `{1, 0}`; a hit writes the key's area into the record's `+0x20`
  (the record's `+0x24` is its Shadow Light Id); a miss of `{1, 0}` too returns the bank's built-in record.
- DAF40, from its listing (the decompiler's C loses the `+0x8C` kept on the whole-record branches): `|t| <= eps` (`[143D7ECE8]`):
  A copied whole (1400841B0, 0x94 bytes), `|t - 1| <= eps`: B whole, else the lerps `(b - a) t + a` of +0x00, +0x04, +0x34..+0x48,
  +0x54, +0x64, +0x6C, +0x74..+0x80, +0x88 and B's +0x50, +0x70, +0x71, +0x84, +0x90 (+0x08, +0x0C, +0x4C, +0x58..+0x60, +0x68 keep
  the context record's). Every branch: `+0x20/+0x24` A's area and Shadow Light Id, `+0x28/+0x2C` B's (A's or B's twice when one is
  taken whole), `+0x30` t, `+0x8C` (Shadow Color) the context record's own (so 839F0's 0xFF000000 unless another writer: not
  read), `+0x10` the direction of the blended Angle (140148E30's rows).
- F90F0 (the sun's shadow direction, the vector its four cascade calls 1B6020 receive): A's area or id -1 → R `+0x10`; else B's
  -1 → light 0 of the light set A (140032770 on `[GXRS+0x41E0]`, the node's `+0x40` = `Banks::Sun`'s `+0x00`); both valid → the
  Angle of light 0 (node `+0x70` plus `+0xA0`) of A and B interpolated by 1400F8FD0 (theta on [-pi/2, pi/2], phi on [-pi, pi], at
  R's `+0x30`), its direction turned by 1400FEA40 (XMQuaternionSlerpV's code: the dot, sign, ATan2 and Sin) of the two records'
  rotations (`+0x2D0`).

**The change** (`native/world/light_producers.cpp`, `10_options_and_passes.inc`): `CollectNativeShadowState` reads the keys
`+0x3FA8`, `+0x3FB0` and the weight `+0x3FB8` (with `+0x3ED0`, the context's `+0x90` as is) in place of the MotionBlur layer's key
`+0x4008` with id 0; `Banks::Shadow` takes 18C590's key rule, mapbase only under a map area (R's reading of GparamAddExtraId_140D26AB0, I-0138 `tranche-6-ds3-reads/REPORT.md`: a cutscene key without the group reads default's `{1, 0}`), `BlendShadowRecord`
is DAF40 into a record kept per context (`NativeSceneContextCpu`, 839F0's defaults at first), `ShadowSunDirection` is F90F0's choice;
`Banks::Sun` writes its rotation at `+0x290` for the blend. The scene words (FC_ShadowColor among them), the sizes and the clamps
come from that record. Not modelled: light 0's node `+0xA0..+0xA4` (an angle offset the native record leaves at 0: its writer not
read); the slerp's Sin and ATan2 are the Windows SDK's DirectXMath, not checked bit for bit against 1400FE900 and 1400FEDC0. Expected in
play (INFERRED until the witness): the keys `{area, 0}` and mapbase's Shadow Light Id 0 give the same sun as before; the change is the cutscenes' keys (FC_ShadowColor's `+0x8C`, never taken from the file, is mapbase's 0xFF000000 anyway, and the traced frames force row 29: no change in play).

**The witness rows** (test tools; `Run-I139Cutscenes` passes `-GraphicsLevels` to Run-AlphaCast, for the OLED's 2560x1440 window, a runner edit with no other change): `native_shadow_state` names the keys, the record's keys and weight, its Angle, `shadow_colour`
and `sun_source` (`angle`, `light_set`, `blend`, `missing`), a row at each change; in the original (`shadow-observe.flag`)
`reference_light_recording` adds the same words of the context, `reference_shadow_source` a row at each change of them, and
(`cascade-observe.flag`) `reference_shadow_sun` F90F0's direction itself at each change.

**Its step review** (fresh, on `6fd3379ce..383d3fc8c`): no high or medium finding; DAF40, F8FD0, F90F0's choice and 18C590's rule
match the listings, the keys are written natively (D52B70 calls D25450 then 888B0: A = B, weight 0, so in play the blend path of A
with itself, the light set's light 0 up to rounding), no race or lock order introduced, the reference rows pass-through and
bounded. Lows answered (`f3c4c5412`): L1 a NaN weight lerps as COMISS/JC do; L2 the comments (the shadow frame's `area`, a dead
field, removed); L3 one lookup and one light-set build when B's key is A's (the play case); L5 area 0 is no bank's. L4 (a lead, not
modelled): E00270 (from DFD270) writes a stage's own sun direction into the context's `+0x90` (839F0's defaults, its direction,
A `{0, -1}`) when `+0x3ED0` is clear, then 88AB0 (which only clears the cascade bytes `+0x2D7D..`); the native runs neither, and
keeps its own copy of the record (the native context is zeroed, so the copy starts from 839F0's defaults).

**The witness `i140-e16`** (2026-11-14 13:10-13:49, the OLED in a 2560x1440 window, offline, `build-e33` = `f3c4c5412`,
`Run-I139Cutscenes` natively then in the original, flags `test-no-ai,world-pictures,gbuffer-pictures,pictures-from-cutscene,
light-observe,shadow-observe,cascade-observe`; log `evidence/I-0140/runs-e16.log`, pairs `evidence/I-0140/sheets-e16/`): 31000010,
32000010, 39000020, 40000040, the eight runs exit 0, no fault, restored. OBSERVED (`native_shadow_state` against
`reference_shadow_source` and `reference_shadow_sun`, F90F0's own vector):

- The keys are the same in both at every step: the title `{-1, -1}` (R's keys `{1, -1}`: the Angle, the sun (0.592, 0.682, 0.430)
  in both); Firelink `{140000000, 0}` (0.747, 0.545, -0.381) in both; the High Wall `{131000000, 0}` (-0.406, 0.906, 0.116) then
  (-0.772, 0.485, -0.411) in both; the Lothric Castle `{132000000, 0}` (0.705, 0.643, 0.299) in both. So in play the native's traced
  sun is the original's shadow sun (the same direction as before this tranche: R's « same in play » holds). The weight 0 in play.
- 32000010: keys `{232000010, 10..70}` (one per cut), each record's Shadow Light Id -1, so the Angle (0, 0, 1) in both (R's « -1 »).
- 31000010: keys A `{231000010, 22}`, B `{231000010, -1}` at a weight near 1 (0.9976 in the original, 0.9994 natively at their first
  rows), then `{231000010, 22}` twice. The original's F90F0 sun is the map's (-0.772, 0.485, -0.411), the native's (0.539, 0.485, 0.689):
  the same angles (set 22's light 0, (-0.506, 0.663), equal to the map set's), a yaw of about 156 degrees (m31's rotation; 118 degrees between the two vectors). B's tag differs (the
  original 231000010, the native 1: id -1 is no bank row natively; B's id -1 sends both suns to A; but B's record weighs about 0.99 in the blended fields (natively default's: `resolution_class` 0, splits 1.007 to 2.088 in e17), and the original's bank answered `{231000010, -1}` under its own area although the file holds ShadowParam ids 0 and 22 only and 18C590's miss writes tag 1: an open contradiction (the reading of 18C590 or of the bank's contents incomplete), a lead for every B key with id -1 in a cutscene's fades).
- The cause, READ (`MapGparamStep_140CDEE50`; the original's rows `reference_gparam_file_bind` s31_00_0010 kind 11 bank 13 weight 0
  and `reference_gparam_bind` area 231000010, both with m31's transform): once the cutscene's resources are ready the step binds
  the cutscene's own file with the map's transform (where CE1DB0 lists the cutscene id: 31000000, 31000010, 32000000, 32000020,
  33000020-33000042, 34010010, 51010010), but inside a block gated on the MapOverride_forRemo cap's handle `+0x70`, never set
  natively: no `native_gparam_scene_binding_source` row for 231000010 in `i140-e16-c31000010-nat`, so `Banks::Sun` rotated the
  cutscene's sets by m31's MapOffset yaw (none) instead. Every cutscene light set read natively had the same defect (the per-draw
  keys `[231000010, 20/24]` too: `native_light_set` (0.539, 0.485, 0.689)).
- 39000020: Siegward dark natively, lit (metal, a warm key) in the original (`sheets-e16/c39000020-10.png`, `-20.png`, by eye);
  INFERRED the same defect (its sets rotated by m39's MapOffset yaw natively, by its cap's transform in the original). 40000040: the
  tour's uniform background no longer shows on this build (`c40000040-10.png`, `-20.png`, by eye; the fog lighter in the original).

**The fix** (`91e46ccfe`, `18_resource_admission.inc`, `22_setup.inc`): `NativeRemoGparamStep` wraps CDEE50: when the genuine step
has taken its ready branch (`+0x100` changed on `build-e34`; the step counter `+0x4C` since the review's L1), it does the gated block's first part as the original (the map's transform
copied when CE1DB0 lists the id, then the cap's binding at weight 0 through `BindNativeGparamFileFrom`, caller CDF131, the original row's), and
`ApplyNativeGparamAreaBindings` takes kind 11 bindings as an area's layers like kind 7, under their own area only. Every cutscene CE1DB0 does not list now keeps its cap's own `+0x80` (the identity in the original's rows for s40_00_0040 and s32_00_0010) instead of the map's MapOffset yaw by name: Firelink's cutscenes (m40, 28 degrees) turn their light sets against the earlier native picture, as the original's (witnessed for m39's 39000020 only). MapOverride_forRemo's own 49D00 binding (a
copy of `empty`, weight 0, no record) is left.

**The witness `i140-e17`** (2026-11-14 14:20-14:30, the OLED in a 2560x1440 window, offline, `build-e34` = `91e46ccfe`, natively
only, the same flags; log `runs-e17.log`, triptychs before, after and the original in `sheets-e17/`): 31000010 and 39000020 exit 0,
no fault, restored. OBSERVED: `native_gparam_scene_binding_source` s31_00_0010 (area 231000010, kind 11, bank 13, weight 0) with
m31's transform, the original's row word for word, and s39_00_0020 (area 239000020) with the identity (39000020 is not in CE1DB0's
list); 31000010's shadow sun now (-0.7722, 0.4848, -0.4106), the original's; in play every key and sun as in `i140-e16` (the native rows compared at their ends per key: a row a change of key or source, so a sun moving within one key, as the original's (0.565, 0.5, 0.657) to (0.101, 0.743, 0.661) at 39000020's arrival, is seen at its ends only; the original's one value (-0.247, 0.839, -0.485) at 31000010's first cutscene frame has no native match, not read). The
binding's logged caller was CDF031 on that build (a typo, the original's is CDF131: corrected after it, the comment's only use).
By eye: 31000010's first shots cooler and lit nearer the original's (`c31000010-10.png`); 39000020 unchanged, Siegward still dark
where the original lights him (`c39000020-20.png`): its sets carry no rotation in either, its ShadowParam's Shadow Light Id is -1 (the
Angle (0, 0, 1), a horizontal sun, in both), and his G-buffer is textured (`c39000020-9300-gb.png`, the presented, GB2 and lit HDR
pictures): so the tranche-7 « flat Siegward » is no longer flat but dark, the lighting's (the original lights his draws per pixel by
their light sets, the native's rays by the one shadow sun: Thomas's choice B, R's), not a material or texture fallback (E's).
40000040's uniform background: not seen on `i140-e16`'s build (by eye, its cause not read: a note, not a result).

**The fix's step review** (fresh, on the native diff `538f59de5..306f6ae3a`): no high or medium finding; the wrapper matches CDEE50's
listing (the offsets, the order: no cap no binding, no map the binding without the copy; weight 0; the bank the cap's `+0x7C`,
D26A60 a tail jump to D275F0, so the caller CDF131). Lows answered (`da65ef628`): L1 the ready branch detected by the step counter
`+0x4C` (CDF970 adds 1 there alone), not by a new `+0x100` (a reused Res object could hand back the same cap); L2 CE1930's 8-byte
slot and its returned pointer passed to CE1DB0, as CDF0EB-CDF0F3 do; L3 a kind-11 binding is a layer only under its own area; L4 the
comments and the row's `live_layers`; L5 CDEE50, CE1930 and CE1DB0 pinned with their siblings. After `main` (R's tranche 6) merged
in, `Banks::Shadow` takes R's `MapBaseAnswers` (one mapbase rule, map areas from 100,000,000: area 1's default no longer reaches
mapbase, as before R's rule either).

**The integrity review** (on `main...1e69c7f16`): no high finding; the numbers hold against the journals (the suns, keys and
weights, the bindings, all 710 original `reference_light_recording` rows with the Shadow Color 0xFF000000 and `+0x3ED0` 0). M: the
merged tree (the review lows `da65ef628`, `MapBaseAnswers`, R's tranche 6) is not the witnessed `build-e34`: answered by a
re-witness of 31000010 on the final build before the merge (`i140-e18`, below). Lows answered above: the sampled in-play
comparison and the original's unmatched first-frame value, the yaw, 40000040 a note.

**The final review** (fresh, xhigh, on `main...1e69c7f16`): no high finding; DAF40, F8FD0, F90F0, 88A10, 18C590 and CDEE50 checked
against their listings; every retail file with its own ShadowParam (m32_00_0100, m33_00_0000/0010, m36_00_0003, m51_00_0200) and
mapbase have Shadow Light Id 0 at id 0, so play is unchanged beyond the three zones seen. M1 (the merged tree unwitnessed; a
vt+0x30 rewriting `+0x4C` would drop the binding silently): the ready branch by either of its writes (`+0x4C` or `+0x100`), and
the re-witness `i140-e18` on the final build. Lows answered (`d5acbf456` and this record): L1 the `+0x3ED0` branch keeps the
context's copy (natively nothing writes the engine's `+0x90`) with a row; L2 recorded above as an open contradiction; L3
INFERRED: 841B0 copies the whole 0x94 bytes (its listing not saved; read by the pilot through GhidraMCP, its 0x94 bytes field by
field) and the bank's built-in record is 839F0's defaults (not read); L4 the change rows per context, capped at 4,000; L5 the
`+0xA0` offset said 0 in the header; L6 `ShadowFieldsFromBanks` (the light producers' replay path, no caller, `g_banks` unset) left
as it was, a cleanup for workstream 16; L7 a row `native_remo_gparam_unbound` when a cutscene file stays unbound; L8 the
identity transform of the unlisted cutscenes and « own area only » stated above, FC_ShadowColor no change in play; L9 no unit test
pins `BlendShadowRecord`, `AngleLerp` or `Banks::Shadow`'s rules (the witnesses stand for them; a test is a follow-up).

**The re-witness `i140-e18`** (2026-11-14 15:45-15:55, the OLED in a 2560x1440 window, offline, `build-e37` = `d5acbf456`, the
final code, natively; log `runs-e18.log`): 31000010 and 40000040 exit 0, no fault, restored. OBSERVED: the binding rows
s31_00_0010 (area 231000010, kind 11, bank 13, weight 0, m31's transform) and s40_00_0040 (area 240000040, the identity), both
with the caller 13496625 (CDF131), the original's rows; no `native_remo_gparam_unbound` or `native_shadow_fixed_record` row;
31000010's shadow sun (-0.7722, 0.4848, -0.4106) from its light set 22, the original's; Firelink and the High Wall in play as in
`i140-e16`. 40000040 keeps Firelink's keys `{140000000, 0}` through the scene (no cutscene ShadowParam key).

Thomas's choice for the rest (2026-11-14, put to him in the thread with these facts; he asleep, the coordinator applied the
recommendation under his rule, to be confirmed at his waking): option 1, in a cutscene whose ShadowParam names no light set (-1:
32000010, 39000020) the traced sun takes light 0 of the light set of the scene's characters; R's (ROADMAP workstream 7 step 7).

**Tranche 7 merged** `d1fbc6f51` (2026-11-14 16:00, after `main` `787a01b1a`, R's tranche 6, merged in); the launcher rebuilt (exit 0,
its tests); start check `start-d1fbc6f51` passed on the OLED in a 2560x1440 window (16:12, restored); the disk rotation refused (R's
run held the game), left to the next checkpoint.

## The GPU particles' SH (STUDY-NOT-YET-NATIVE item 8)

READ (GhidraMCP, 2026-11-14): the draw `GpuParticleSortAndDraw_1402A7E70` writes its PS b11 `cbCubeEnvSH1` itself before each draw: 48
bytes, the system's SH `S+0x2A20..+0x2A4C` times the rows at 1439F24B0 and its light's colour `[[S+0x2B80]+8]+0x110..+0x118`, through
the update wrapper 1920300 into the buffer at `[[GXRS]+0x4DD0]+0x2C0`, which the native names only during the effects' execution (the
settings shadow, `effect_draw.cpp`), and its `Update` hook keeps those bytes. So the study's « a zero SH » holds only if `S+0x2A20` is
zero natively (its writer not read). A witness row `native_gpu_particle_sh` (`ffx-record.flag`, the first 8 images and every 600th)
tells it on the next run (`build-e38`).

## The Road of Sacrifices' green moss (after tranche 7)

The original's rocks there carry a green moss the native's lack (`i140-e15`, by eye). Read first as the surface « Deposit » layer
(I-0105 tranche 10: natively the names `m33_00_Deposit_*_0000` are not found and the root's `snow_*` bound). **The witness `i140-e19`**
(2026-11-14 16:26-16:30, the OLED in a 2560x1440 window, offline, `build-e37`, from the kept save at the Road of Sacrifices bonfire
`eta-i139-z16-road-of-sacrifices-20261112-204340`, natively with `world-pictures,gbuffer-pictures,light-observe`, the original with
`ffx-draw-observe`; log `runs-e19.log`, sheets `sheets-e19/`): both exit 0, no fault, restored. OBSERVED: the original's own deposit
record there is the built-in `m22_00#0` with null words (`reference_snow_blend`, callers 23F79 and 10466B), so it binds the root's
textures as the native does (`m33_9999.tpf` is in no archive either: `Recover-GameDataByName` found m30's alone): not the cause. The
native G-buffer holds the moss: over the rock wall (500x600 at (1900, 300) of 2560x1440, frame 3600, `Convert-GBuffer.py`) GB2 is a
green-grey (0.345, 0.326, 0.252), the grass and moss visible (`sheets-e19/gb-3600.png`, `world-3600-gb2.rgba8.png`), while the lit
HDR is red-dominant (0.032, 0.019, 0.009: R/G 1.70 against the albedo's 1.06). So the moss is drawn and lost in the lighting's warm
tint (the player's red-orange of `i140-e11`, the same), the lighting's: R's (ROADMAP workstream 7 step 7), not a material (E's).
By eye: the rock wall near black natively in the first sweep (`sweep-1-1.png`), lit in the original.

## The Painted World's cyan veil (45000030, 45000031; the tour, after tranche 7)

The tour (`i139-t3c-c45000030-nat`, 2026-11-14 16:49, the launcher's build of `d1fbc6f51`; sheet `evidence/I-0139/artifacts/sheets-t3d/45000030.png`)
shows the first shots natively under a uniform light cyan veil, the original's dark. OBSERVED: the veil's mean over a 400x300 crop of
`cutscene-15.png` (84, 119, 134) against the original's (30, 31, 38) (ffmpeg, scaled to one pixel); the map's fog record `{145000000, 0}`
(`native_fog_set`: colour 0.43, 0.49, 0.55, start 12, end 100) its hue; through the cutscene every light and fog key the engine gives
the draws is the map's (`native_light_entries`: all 145000000), as the original's per-draw keys would be (not logged in that run); the
cutscene file `s45_00_0030` bound (kind 11, the identity, as CE1DB0 does not list it). INFERRED: the native applies that fog more
strongly than the original (a full veil from 12 m), the fog's model, R's (ROADMAP workstream 7 step 7, workstream 12), the family of
31000010's wyverns lost in the haze; not a material. Handed to the coordinator for R (2026-11-14 17:15).

## 51000000's angels and the GPU particles' SH (the witness `i140-e20`)

The tour (`sheets-t3d/51000000.png`, the launcher's build of `d1fbc6f51`) showed the Dreg Heap's angels absent natively at 15 s and
60 s and a near black picture at 30 s. **The witness `i140-e20`** (2026-11-14 18:54-18:59, the OLED in a 2560x1440 window, offline,
`build-e38` = `c5304b91c`, natively only, `Run-I139Cutscenes` with `test-no-ai,ffx-record,character-fade-observe,world-pictures,
gbuffer-pictures,pictures-from-cutscene`; log `runs-e20.log`, pairs `sheets-e20/`): exit 0, no fault, restored. OBSERVED:
`native_character_fade` admits the angels' model c6250 at full opacity all through (views 5 and 0x45, `full` 3,001 at its last row,
none refused or partial), c2160 the same; the stills show the angels at 30 s and 60 s (`c51-30.png`, `c51-60.png`: native left, the
tour's original right), but at other moments of the scene than the original's shots of the same second (at 60 s natively the angels
in the sky, the original's on the cliff with the city). So the angels are drawn; the tour's « absent » is a shot taken at another
moment (the shots count seconds from the hand-over; INFERRED: the native scene starts later after it, as in 31000010's
`sheets-e17/c31000010-30.png`; not measured). The 30 s picture dim orange-brown natively, a light blue haze in the original: the
lighting's and the fog's (R's, as 45000030). The GPU particles' SH (`native_gpu_particle_sh`, the bonfire's embers, caller 2A83C9):
every image twelve ones, not zero: STUDY-NOT-YET-NATIVE item 8's « zero SH » does not hold on this build (whether the original's
are ones too: not read).

## Gap 16's 608 quads (the witness `i140-e21`)

**The witness `i140-e21`** (2026-11-14 23:09-23:13, the OLED in a 2560x1440 window, offline, `build-e39` = `5e45dd38d`, natively
only, `Run-I113FireOrb -Runs near-native -Casts 3 -LookSweep '1,2300,2'`, Alpha's all-pyromancies copy restored by hash; log
`runs-e21.log`): exit 0, no fault, 10,787 native frames, restored; three orbs burst near the dead tree (`cast-*.png`, `casts.mp4`).
OBSERVED (`native_ffx_radial_draw`, 33 rows: the first 32 draws of class 3D7E8C0 and the 500th; [Get-RadialDraws.py](Get-RadialDraws.py)
and a check of each vertex): every 608 draw is a 4-vertex strip (topology 5, stride 32, not indexed, nothing invalid) whose
vertices are already clip-space positions (x, y, z, w; w 3.9 to 4.1 for the first 32, 10.7 for the 500th, z/w 0.96 to 0.985) followed by their own screen coordinates
(u = 0.5 + 0.5 x/w, v = 0.5 - 0.5 y/w on all four vertices of all 33 rows, within 0.002): a square of 447 to 467 pixels round the
burst for the first 32 draws, 1,063 pixels for the 500th. VS b0 (512 bytes) holds a view-projection and a view matrix, neither
zero; its last row holds the translation, so the row-vector product (v x M) is its form, and so applied to those positions it puts
the quad off screen (x about 17,000; the column product would give on-screen rectangles): the 608 VS passes the position through
(INFERRED; its DXBC not read). So the quads are not degenerate and cover the burst; with I-0113 tranche 31's copy boxes equal in both modes (median width
575 natively against 549 at 1920x1080), the quad's reach is not where the original's « farther » blur comes from (INFERRED; the
original's own quads not logged). The GPU particles' SH: twelve ones again (`native_gpu_particle_sh`, the bonfire's embers).

## The tour's 32000000 and 32000010 (the witness `i140-e22`)

The tour (I-0139 tranche 3, `i139-t3m-c32000000-nat`, on `main` `d1fbc6f51`) showed at 32000000's 50 s the kneeling player's head
a flat blue-grey silhouette, and at 32000010's 40 s the smoke over the temple dark grey where the original's is pale. **The witness
`i140-e22`** (2026-11-14 23:13-23:22, the OLED in a 2560x1440 window, offline, `build-e39`, natively only, `Run-I139Cutscenes` with
`test-no-ai,ffx-record,world-pictures,gbuffer-pictures,pictures-from-cutscene`, Eta's copy restored by hash; log `runs-e22.log`):
both exit 0, no fault, restored. OBSERVED (the stills, by eye): 32000000's close-up comes at 55 s in this run (at 50 s the bell);
the head is again a flat blue-grey mass with only the hair strands' outline, the skin flat pale grey-white, and **the character is
see-through at its lower right edge** (the cracked stones show through the torso, `cutscene-55.png`); the bell at 50 s is a
flat dark grey disc. The original's same shot (`i139-t3c-c32000000-orig/cutscene-50.png`): an opaque player with dark hair. So the
close-up player is drawn partly transparent and flat natively (a fade's or a translucent row's look, INFERRED); whether the
original fades it there (the camera's near fade) and dithers it, or does not fade it, is not read. 32000010: natively the temple
shot comes at 40 s with darker smoke and a darker frame overall; the original's at 40 s shows the same smoke lighter under a white
haze over the whole frame, and its scene ends about 15 s earlier (it is back in play at 50 s, the native at 60 s). So the smoke is
drawn in both; its darkness goes with the frame's (the lighting's and the fog's, R's, as 45000030's veil; INFERRED).

## Tranche 8: the effects' fog (the engine's fog bank answered from the native banks)

**What the effects read** (READ, GhidraMCP on DarkSoulsIII.exe 1.15.2, the pilot, 2026-11-14; PS 2AFA by `fxc /dumpbin`):
`GXFfxDrawContext_Slot27_CommitDraw` 264230 asks the engine's fog bank for the draw's fog record, `38E70([GXRS+0x41E8], key)` at
1402661F4, and 14014E900 turns its +0x10 block into the draw context's +0x880, the cbFog block PS b6 reads; 38E70's other callers
are 263A60 (the GPU particles' draw context), 5A120 (the model effects' pass descriptor, skipped natively), 380A0 (the cutscene act's
fog override, from CC8180) and 137910. Natively that bank never receives a file (07_scene_lighting's note: D272C0, D275F0 and 49D00
are the native's own admission and bindings; the native's world shading builds its records from `Banks` instead), so 38E70 returns
its built-in record for every key it is asked (save the nodes the cutscene act's 380A0 writes into it during a scene: I-0141).
PS 2AFA's output is clamped at 0 (`max r0, 0` before `o0`); its alpha (vertex alpha x texture x
`g_vColorScale.w` x the soft fade x, with `g_ps_FogMultMode`, (1 - distance fog)(1 - height fog)) is not clamped above, so over the
INV_SRC_ALPHA blend of bank 0x10 an alpha above 1 gives dest x (1 - a) < 0: that is where the negatives come from (INFERRED from
the programme and the blend), and the fog lowers that alpha and pulls the colour toward the fog's.

**Observed** (the flame witness `ffx_flame_witness`, both modes, the draw context's +0x880 block, existing journals at Archdragon
Peak): natively every row's fog block is the built-in one, (1, 0, 0, 0 | 0, 0, 0, 0 | 1, 0, 0, 0 | 1, 0, 0, y | 0, ...): no fog
(`i113-t26-orb1440-near-native` 52 rows, `i113-t30-orbscan-near-native` 54, `i140-e21-near-native` all rows); in the original 21 of 69 and
16 of 66 rows carry Archdragon Peak's fog (colour 0.596, 0.709, 0.907, density -0.0132, the squares of `native_fog_set`'s
132000000 colour 0.772, 0.842, 0.952) and 5 and 4 Firelink's (140000000), the rest the built-in block (`i113-t26-orb1440-near-original`,
`i113-t30-orbscan-near-original`). So the native's effects draw without the map's fog where the original's draw with it. The magenta
count of `i140-e21`'s film (`t33/hue640.py`, 640x360 at 5 frames/s; output `evidence/I-0140/hue-e21-e23.txt`): 10 pixels in all,
peak 7, over three casts (tranche 35's 73
to 121 were two casts on an older post chain; the original's films of tranche 26 are no longer kept).

**The fix** (`07_scene_lighting.inc` `NativeEngineFogLookup`, hooked on 38E70 unless `no-effect-fog-banks.flag`): for the fog
bank `[GXRS+0x41E8]` and a key the native rule resolves (`NativeResidentGparamKey`, the rule NativeGparamRecords now shares:
{-1, -1} as {1, 0}; an area of 1 or more with an id other than -1), the record built from `Banks::Fog` in the lookup layout
(+0x10), as the native's own collect builds it; any other bank, the engine's own answer (for the fog bank: see « With I-0141
tranche 3 merged » below). `native_effect_fog_record` logs the first 16 keys built (a process-wide count) (the act's 380A0 asks its sources
A and B so too: in `i140-e24` the {232000000, ...} keys are those sources), `native_effect_fog_engine_node` the first 16 engine
nodes passed over.
Its reach: every caller of 38E70 on that bank: the billboards (264230), the GPU particles (263A60), 380A0 and 5A120 (skipped
natively); 137910 asks the bank `[[root+0x41F8]+0x98]+0x28`, the same bank in the original (`i140-e27-c32000000-orig`: 27 `effect_fog_key` rows from caller 137B7B, inside 137910;
natively none);
no caller is known to write through a returned record (380A0 read by R in I-0141 tranche 3; 263A60's body not read). The order, the banks before the
engine's node, is R's note (2026-11-15 01:00; at that time, before I-0141 tranche 4): natively the node the act writes at layer 11
of the engine's bank was never withdrawn (4A060 gated on the cap's handle), so the engine's node first (the step review's L2, `build-e41`/`build-e42`) would keep a
ceremony's fog on its keys after the scene, while `Banks` drops an act's override (I-0141 tranche 3, `Banks::Overridden`, in `main`
since `a2448ae21`). The light bank (32770, which 264230 also calls four times) is left as it is: the effects' light is not compared yet.
Builds: `build-e40` (the first form, witnessed below), `build-e41` and `build-e42` (the engine's node first, not run),
`build-e43` (the banks first, before I-0141 tranche 3). **With I-0141 tranche 3 merged** (`main` `a2448ae21`, `build-e44`): a key
the act overrides (`Banks::Overridden(1, ...)`, {area, -1} included) is answered from the banks too (NativeGparamRecords the same,
merged with R's rule), and a node of the engine's fog bank was never returned (the `build-e44` form: natively that bank holds only the act's 380A0
nodes, which then stayed after the scene; superseded by the final review's M1 below, an overridden key now takes the act's own
node, and by I-0141 tranche 4, which withdraws the nodes): any other node is passed over for the bank's built-in record, as
every key had before; logged `native_effect_fog_engine_node`. R removes the act's engine nodes in I-0141 tranche 4 (the world's side); the pass-over then
meets none. **The final review of this form** (fresh Opus, xhigh, `main...173ac65e8`): H1, the order of merges: with 380A0's
sources now real, the node left after the ceremony carries the ceremony's fog, which NativeGparamRecords' fog fallback (R's
world side, the engine lookup) would serve after the scene; R removes those nodes in I-0141 tranche 4, so tranche 8 merges after
it (agreed with R, 2026-11-15 03:15). M1: an overridden key is blended at each act update, so its records would fill the store;
an overridden key now takes the act's own engine node of the key (380A0 writes it at each update from the banks' sources;
R's code publishes the override after that write; its range remover 387D0 runs before the override's erasure, so for that window
the override has no node), a fresh record in an eight-slot per-thread ring then (its « used at once » read for 264230 only); the store keeps resident keys only. M2: the
combined witness `i140-e25` (below). L1: the flag in both groups of Run-StartupTrace's pattern. L2, L3: this section and STATE
corrected. L4 (a commit naming a build still running), L5 (one act update's window; two `Banks` calls under two locks in R's
`Build`, R's), L6 (`BindingGeneration`'s lock per lookup) and L7 (the catch path without the pass-over) recorded.

**The witnesses of tranche 8** (2026-11-15 00:36-00:49, the OLED in a 2560x1440 window, offline, `build-e40` = `35e113c7e`, the
first form (the banks first, as the form here; the store and lock answers of the step review not in it); natively only;
logs `runs-e23.log`, `runs-e24.log`, sheets `sheets-e24/`): `i140-e23` (Run-I113FireOrb `-Runs near-native -Casts 3`, Alpha's copy)
and `i140-e24` (Run-I139Cutscenes, 32000010 then 32000000, Eta's copy, with `character-fade-observe`): all exit 0, no fault, restored.
OBSERVED: `native_effect_fog_record` answers every key from the banks ({-1, -1}, 140000000/0, 132000000/0 and /60 in play; in
32000000 the cutscene file's keys 232000000/0, 20, 35, 200, 350 and 1020); the flame witness's fog blocks now carry Archdragon
Peak's fog with the original's words to the last digit ((0.2, 0.0083333, 0.010176, -0.013175 | 0.59572, 0.70939, 0.90652, 0.2 |
...), 7 of 49 rows, and Firelink's 5, the rest the built-in block; 12 of 49 rows fogged against the original's 26 of 69 and 20 of 66,
a difference of sampling (the witness's first 24 rows and every 5000th) not read further). Fire Orb's film:
magenta 6 pixels (peak 6) against `i140-e21`'s 10 (peak 7) on `build-e39`: both small, a difference the bursts' own variation can
make (not a measure of the fix). 32000000 (the integrity review's F1 and F2, checked by the pilot on the stills; the sheet
`sheets-e24/c32000000-before-after-original.jpg` first set two different shots side by side, the native scene's timing
drifting between runs, and the pilot wrongly told Thomas the flat head was gone, corrected in the thread at 00:58): on the same
shot (`i140-e22` 55 s, `i140-e24` 50 s: the fist on the stone, the knee's ember) the flat blue-grey head and the pale
see-through body **persist**: not the effects' fog; the player's one main-walk admission was at full opacity
(`native_character_fade` c0000); cause open. The next shot (the kneeling player, the arches behind: `i140-e22` 60 s, `i140-e24`
55 s) had natively before this fix a grey haze over the whole frame as the original's (`i139-t3c-c32000000-orig` 50 s; YAVG of the
whole frame 143.2 before, 62.2 with the fix): **the fix loses that haze** (a regression on this shot, OBSERVED). INFERRED: the haze
is an effect that kept its full alpha without fog and that the map's fog now fades, where the original's draws read the act's
override of the map's keys (I-0141's second observation); to see with I-0141 tranche 3 merged before tranche 8 merges.
32000010's temple at
40 s (`sheets-e24/c32000010-before-after-original.jpg`): the smoke region (1300x600 at (200, 80)) YAVG 75.8 before, 93.6 now
(ffmpeg signalstats; the two shots matched by eye, also by the integrity review); the original's whiter haze still lighter.

**32000000's haze, the witnesses `i140-e25` to `i140-e27`** (2026-11-15; natively only on the OLED for `i140-e25` (`build-e44`,
I-0141 tranche 3 merged: 32000000 then Fire Orb, exit 0, restored); `i140-e26` (`build-e45`, both modes, its first row budget
spent at Firelink: nothing on the scene); `i140-e27` (04:18-04:27, the ASUS in a 1280x720 window for both modes, offline,
`build-e46` = `f1e3cb36c`, `Run-I139Cutscenes` with `test-no-ai,effect-fog-observe`, Eta's copy restored; log `runs-e27.log`,
sheet `sheets-e27/`): all exit 0, no fault). OBSERVED (ffmpeg signalstats on the game window: the native stills cropped 2560x1440 at (8, 31), the original's 1280x720 at
(8, 31)): with I-0141 tranche 3 the haze shot's whole-frame YAVG is 75.0
(`i140-e25` 55 s) against 62.2 without it, the original's 142.9 and the native's 143.2 before tranche 8. `effect_fog_key` in both
modes: the billboards' caller (2661F9, in 264230) asks above all the key {132000000, -1}, which the act overrides; its answers
take 341 distinct fog blocks natively and 389 in the original, with the same ranges word by word (colour 0 to 0.806, density 0 to
-0.833, start 0 to 383.3, end 0.1 to 100), from the built-in values (0, 0, 0, 1, 0, 100, ...: natively the built-in record, in the
original a node holding those values) to the cutscene file's {232000000, 20} (0.806, 0.854, 1, 0.155, 120, 0.1, -0.833); the
blend steps differ only by when each mode sampled them; the GPU particles (263AF4) and the act's sources (3867E, 3868D) the same
keys and words in both, up to denormal residue (word 10 about -4.5e-30 in the original's override records, 0 natively). So the effects get the original's fog in the cutscene too. INFERRED: the original's grey veil over that
shot is not the effects' (their fog is the same in both modes) but the world's (the dense cutscene fog, start 0.1 m, on the
world's pixels: R's, I-0141), and the native's veil before tranche 8 was an effect drawn without fog in front of the camera,
standing in for it by accident; tranche 8 removes that accident and shows the world fog's lack. The close-up (`i140-e27` 45 s,
both modes, `sheets-e27/c32000000-native-left-original-right.jpg`, top row): the original's arm and knee are as pale and flat as
the native's in that shot (the head out of frame in both); so the tour's « flat head » may be largely the original's look under
that fog, to read again on a shot that shows the head in both modes.

**The step review** (fresh Opus, high, `1940c865b..35e113c7e`): H1 (the store's 4096 records filled per thread and per bank
publication, then every effect back to no fog with no row) answered: the records are shared by content (equal fog words, one
record; their number bounded by the files' distinct fog records; never freed, so a returned record outlives any use), one row
`native_effect_fog_store_full` at the bound. M1 (a new `Banks` at a freed one's address) and M2 (`gparam_lock` on every effect
lookup): each thread keys its index by the banks publication (`native_gparam_publishes`, read without the lock, the lock only when
it changed) and holds the banks by a weak pointer. L1: the function in `Test-ThreadAffinity.ps1`'s scope, its five statics
reviewed in the allow-list (the scan had matched a comment's « NativeEngineFogLookup (below) »; reworded). L2: the engine's own
node first, then withdrawn on R's note (above). L3: the flag spares the hook's install. L4 (the native collect's `NativeGparamRecords::Fog` building the same
records) left: two readers of `Banks::Fog`, not a bridge. L5: this section's status and reach rewritten.

## Tranche 9: the credits' « MOVIE » plate (a regression of I-0137 tranche 3)

**The cause** (OBSERVED in the journals; the page's side INFERRED): the tour's `i139-t5-flag20-nat` (the credits by flag 20, on
`main` `d1fbc6f51`) shows a « MOVIE » plate over the whole film though the native player ran the film (13,002 frames in
`movie_witness`, `native_movie_close` drawn 13,001). Its
journal: one `native_ui_asset_unavailable` (the movie proxy, class 29A4968) five rows before `native_movie_open`, then no UI import
at all (0 `native_ui_asset_image_imported` rows after it). The October witness `i66-native-2065-credits`: the same order, but the
early import took the SYSTEX_BLACK stand-in (`native_ui_offscreen_placeholder`, five rows before the open: its class not printed,
the movie proxy by its place, INFERRED), and after the open 26,032 imports followed (26,030 of the 1920x1080 target). I-0137 tranche 3 (2026-11-12, `6dc18c431`, `79cf21d7e`) made that early import
answer 0 after a witness at the character creation only; with no image the page's movie screen has none to replace and keeps its
default, the plate (INFERRED from the October correlation: the per-update import's caller not traced; E48980, the lookup by
name, caches a non-zero image only). Not the Graphics tab nor the shader page (Thomas's question, 2026-11-15 03:46).
**The original** (READ, GhidraMCP, 2026-11-15): `CSMovieGxTexture`'s constructor EADB80 sets its inner texture `+0x10` to
`[[144792E58]+0x60]`, the texture D4A2D0 registers under « SYSTEX_BLACK » (its name read at 142955578); its vt+0x20 (EADCD0,
VERIFIED in its bytes by the pilot, GhidraMCP; MENUS-MAP §9 names the same forwarding at the SRV slot +0x10) answers the frame's
texture `[+0x18]+8` while a frame is set and jumps to the inner texture's vt+0x20 otherwise; E49320 builds the UI image from the holder's vt+0x20 texture. So before the first frame the original imports
SYSTEX_BLACK: the October stand-in was the original's answer, not an invention. The menu offscreen proxy (`CSOffscreenGxTexture`,
D6C0F0, vt+0x20 D6C250) forwards the same way.
**The fix** (`20_startup_phases.inc`, `NativeUiTextureImport`): a movie proxy without its native target imports its inner
texture (the inner texture's native record, or the native SYSTEX_BLACK common image when it is that entry's texture), a row
`native_ui_movie_proxy_inner`. The menu proxy is left as I-0137 tranche 4 made it (its 0 and the next lookup reach its own target
natively; a black image cached by name would freeze it). Build `build-e47`. **The witnesses** (2026-11-15 05:44-05:55, the ASUS in a 1280x720 window,
offline, natively only, `build-e47` = `29a3d9492`; logs `runs-e28.log`, `runs-e29.log`, sheet `sheets-e28/`): `i140-e28` (the
tour's Run-I139Credits, flag 20, Eta's copy) and `i66-native-i140-e29-charmake` (Run-PilotPair CharMake: a New Game after the
opening movie to the character creation, Alpha's copy): both exit 0, no fault, restored. OBSERVED: in `i140-e28`
`native_ui_movie_proxy_inner` (inner SYSTEX_BLACK, resident) six rows before `native_movie_open`, then 14,544 UI imports after the open
(14,804 in the run; counted by row on `build-e47`: from `c8d00fa4a` on, the row is bounded and the count is its `n` field); the stills at 10, 60 and 240 s show the DARK SOULS III title and the credits' names where the tour's run showed the plate
(`sheets-e28/credits-after-10-60-240-before-240.jpg`). In the creation run two such rows (after the opening movie), the creation
screen drawn (`c2.jpg`). So the fix is VERIFIED on both paths. The proxy family's other classes (Middle, DummyBlink, BaseProxy) not read; the level of
the inner pointer (a GX texture or its holder) is settled only for the SYSTEX_BLACK case (compared by address).

## Tranche 10: the light shafts 10003 (their own targets)

**Read** ([STUDY-LIGHTSHAFT-10003](STUDY-LIGHTSHAFT-10003.md), GhidraMCP, 2026-11-15): 271B40 draws the shaft's mask into the
first of the scene control's two half-size R8G8B8A8 targets (`sc+0x238`, `sc+0x240`, made by 24F2C0 at view attach), blurs it in
a ping-pong between the two (copy, then draw sampling the other, `[data+0x70]` times; the result always back in the first), binds
the pass's target again and composites the first target onto the scene inside a scissor rectangle of the shaft's screen bounds.
Natively 24F2C0 never runs, both slots are null, and the class was skipped and counted (`native_ffx_skipped` 3D65A88, pass 2D, in
the tour's 30000010 and 38000020). I-0068 FFX-FAMILIES-PHASE2 §6.2's « no reader » of `+0x238/+0x240` is withdrawn (271B40
reads them through a pointer it loads first).
**Code** (`effect_draw`, `21_effects.inc`, `EffectStage`): the two targets as native objects of 24F2C0's shape, named at
`sc+0x238/+0x240` for each execution only (null again before it returns); the scene's view in the outer context's render-target
cache (`+0x10/+0x18`), which 271B40 saves and binds again; the inner wrappers OMSetRenderTargets 1920F40, RSSetViewports 1920CD0
and RSSetScissorRects 1920CF0 hooked (no family drawn before called them: the trap counters of every I-0140 witness run are
empty); ClearRenderTargetView and CopyResource on the targets handed on as two new statements (`ClearTarget`, `CopyTarget`); the
class allowed by default (`no-ffx-lightshafts.flag` opts out), run only where the command's scene control is the one whose slots
name ours. The stage makes the two textures at half the render size when a frame uses them (retired through the frame slot when
the size changes), draws into them with one target and no depth, clears and copies them, binds their shader views (the new
texture kind `Target`), takes the record's viewport for them and its scissor where the raster entry enables it (bank `+0x210`),
and leaves them in PIXEL_SHADER_RESOURCE at the frame's end. Witness rows: `native_ffx_light_shaft` (each command's draws, clears
and copies), the stage's `target_draws`, `target_clears`, `target_copies`, the frame counters' `target_statements` and
`target_drops`. Not reproduced: PS s1 stays the stage's static anisotropic wrap sampler (the bank's `+0x2D0` linear wrap in the
original); the scene view's description is R16G16B16A16_FLOAT by assumption (only its size is read). Build `build-e52`.
**Step review** (fresh Opus, high, `6636c5432..41d91ae4b`): no High. Medium M1: the GPU particles' indirect draws kept the
viewport and scissor a classic draw had set (the shaft's scissor, or the half-size viewport when its composite is dropped):
fixed, they set the pass's again. Medium M2: the shaft's target, viewport and scissor stayed in the context's state for the later
commands (271B40 never resets its scissor rectangle; an early return after its first bind would leave target 1 bound): fixed,
restored after each light-shaft command. Low L1 (the opt-out flag still seeded the outer cache): fixed; L2 (the owner skips
counted only in witness runs): counted in `native_ffx_light_shaft`; L3 (`target_count` unused, the scene's clear named foreign):
fixed; its other L3 items (redundant tests, the row formatted in every run, rate-limited by design) left. Its unverified Medium
(t3, `[dc+0x998]`, natively the depth texture object the pass context's `+0x4C8` gives every family) left to the witness. Fixes
`1a57d60a6`, build `build-e53`.
**The first witness** (2026-11-15 10:46-10:54, the ASUS in a 1280x720 window, offline, natively only, `build-e52`; log
`runs-e30.log`): `i140-e30-c30000010-nat` and `i140-e30-c38000020-nat` both closed on 0xC0000005 at `1400C4F10+0xEC` under
271B40 (OBSERVED, the crash reports), restored by hash. C4F10 builds a GX state block on the outer context and reads the 2D
virtual extent `[[144961AB0]+0x88]+0xF8/+0xFC`; `[144961AB0]` is null natively and C4F10 is outside the effect module's
redirected display sites (READ, its disassembly). Fixed (`288ec2083`): on the native outer context C4F10's body is made natively,
statement for statement, the extent the scene's size (271B40 overwrites the block before reading it). The second step review
(fresh Opus, high, `41d91ae4b..1a57d60a6`): no High; M1 (an early return of 271B40 leaves target 1 and the half-size viewport in
the GX contexts' own caches, which 263C70, 264230 and 375DF0 read): fixed, those caches saved and restored around each
light-shaft command; M2 (the owner skips not written when no shaft ran): fixed, a row of their own; L1 (an all-zero viewport or
scissor after a GPU statement never set): fixed; L2 (the comment): rewritten. The third step review (`1a57d60a6..288ec2083`): no
High; M1 (the outer save ran past its 8 scissors into the depth-stencil state, layout, topology and vertex buffer the shaft binds,
leaving the outer and inner caches disagreeing): fixed, the save ends at `+0x20F`; L1 (a heap allocation a shaft command): on the
stack; L2 (C4F10's image constants read at each call) and L3 (two rows at a first owner skip) left, rare. `7504c672d`, build
`build-e55`. The fourth (`288ec2083..7504c672d`): no High, no Medium; two Low on the restore's comment, rewritten (`283c6b85c`,
comments only).
**The second witness** (2026-11-15 11:55-12:05, the ASUS in a 1280x720 window, offline, natively only, `build-e55` = `7504c672d`,
with `test-no-ai` and `ffx-record.flag` (its witness counters; its observer hooks only read);
log `runs-e31.log`; sheets `sheets-e31/`, each row the tour's native run before the tranche (`i139-t3m-*`, the OLED), this run, the
tour's original (`i139-t3c-*-orig`)): both cutscenes exit 0, no fault, restored by hash. OBSERVED in the journals: in 30000010 each
light-shaft command makes 2 draws and 2 clears (no blur: its count 0), in 38000020 3 draws, 2 clears and 2 copies (one blur
step, then the odd count's copy back), all in pass 0x2D, the targets 640x360; the stage drew them (`target_draws` 1,224 and
5,060; `target_clears` 2,448 and 5,060; `target_copies` 0 and 5,060; `target_drops` 0, `skipped` 0, no pipeline failure). OBSERVED on the stills: at
30000010's 30 s the rose window now has the original's cyan glow rising over the kneeling figure (`c30000010-30-rose.png`: before,
a flat white window; the original, the same cyan glow); at 38000020's 50 s the doorway at the top of the stairs holds a pale haze as
in the original (`c38000020-50-door.png`), brighter natively (its region's mean luma, ffmpeg signalstats on the game window
`crop=1280:720:8:31` then `crop=110:170:600:150`, 85.7 against 58.9; the whole window 51.2
against 39.8, so mostly the frame's exposure, INFERRED; the shots are not at the same instant). So the class is drawn natively
with the original's passes: VERIFIED for these two scenes, its brightness against the original left to Thomas's eye.

## Tranche 12: gap 23, the fast travel's fade under the TAA (a fade mask)

Branch `claude/e-i140-gap23` (the fresh E thread, 2026-11-15 evening), from `main` `6c9037dc6`. Gap 23 (I-0113 RESULT, gap 23): in a
fast travel the native body fades as a soft veil under the TAA where the original, without anti-aliasing, breaks into hard dots;
I-0113 tranches 39 and 41 had the TAA's reactive weighting take up to 0.9 of the history's weight where the 0x29 rows and the effects
change the pixel, with no gain on film.

- **The control (`i140-e60`, 2026-11-15 21:10-21:34, the ASUS window, offline, Alpha's all-pyromancies copy, `Run-I113Warp`, the
  second warp filmed without loss, new `-VideoLossless`; build `build-e61` with four trial switches, since removed).** Five native
  runs: the default TAA, `taa-no-jitter` (the TAA on an unjittered camera), `taa-no-mip-bias` (the materials' own mip bias), a trial
  taking the whole weight and the pixel's own sample where the reactive difference holds, and `no-taa`; all exit 0, no fault,
  restored. The hair at 15 frames/s through the fade (`evidence/I-0140/e60/hair-notaa-taa-full.png`; the five films' heads,
  `fade-notaa-taa-nojitter-nomipbias-full.png`): without the TAA the hair breaks into dark dots (I-0113 tranche 41's finding); with the
  default TAA, without the jitter or without the mip bias, a soft veil; with the whole weight and the own sample, dots as without the
  TAA (OBSERVED by eye on lossless frames). So neither the jitter nor the mip bias makes the veil: the resolve does.
- **The form (`i140-e62`, 22:36-22:55, the same protocol, both travels filmed without loss, new `-VideoTravel` on `Run-I113Warp`;
  `build-e62`).** Four native runs, all exit 0, restored: the default TAA, the whole weight with the own sample (`full`), the whole
  weight with the 3x3 reconstruction kept (`recon`), `no-taa`. The fade (`e62/hair-notaa-taa-recon-full.png`): `recon` keeps the
  veil, `full` shows the dots (OBSERVED by eye). Both parts are needed: the reconstruction (a Gaussian over the 3x3 at the pixel's
  unjittered centre) averages a dissolve's single-pixel dots even with no history (`recon`), and the own sample with the 0.9 cap's
  last tenth of history kept the veil too (I-0113 tranche 41's `taa-reactive-raw` trial, on crf-16 films). The crawl
  (`e62/analyse.sh`: the mean absolute luma difference of consecutive frames over the upper half of the window, the camera still,
  the last 8 s of the arrival at Archdragon Peak and its cloud bank, `arrive-taa.png`): default 0.735, `full` 1.267 (+72 %), `recon`
  0.788 (+7 %), `no-taa` 0.230 (no jitter; MEASURED, one run each). The own sample everywhere the reactive difference holds would make
  the fog and the effects crawl with the jitter (tranche 41 found the weight taken over 38 to 52 % of that view).
- **The fix: a fade mask, written by the dissolving rows.** A model's row dissolving (engine pass 0x29 of a category-0x29 model,
  the world's view, not a depth pre-pass, not an effect's model particle, not a phantom: the final review's H1 and M1, below) also
  makes a fade-mask pipeline (`model_draw.h` `fade_mask`, `kFadeMaskPipeline`: its own programmes and
  rasterizer into one R8_UNORM target, the depth tested LESS_EQUAL and not written, no stencil, a ZERO/ZERO blend writing 0 where a
  pixel is drawn), and marks the mask on a frame only while its model's fade value (the group's +0xDC, mulCol.w) moves
  (`NativeFadeMoving`, `fade_mask_draws`: a dissolve, not a translucency that lasts). On a TAA frame with the reactive base and such a
  row, the renderer clears the mask to 1 and draws those rows again into it once the forward and effect rows are drawn (`EncodeTaaFadeMask`, a frame-graph pass, plan key bit 20, the
  mask persistent in PIXEL_SHADER_RESOURCE|NON_PIXEL_SHADER_RESOURCE between frames). The TAA (`taa_shader.h`, t5, `fade`) takes,
  over the 3x3 of a marked pixel, the history's whole weight and the pixel's own sample: there the output is the current frame, the
  original's dots; elsewhere nothing changes (the reactive form of tranches 39 and 41 for the fog, the effects and the partial
  states that last). The dissolve's
  holes are not drawn by the rows (their programmes discard them), so they are reached through the 3x3, as tranche 41's difference.
  ER: not read for this (its TAA's mask inputs are not in ER-TAA-STUDY); FSR 2 has the same idea as its « transparency and
  composition » mask written by the transparent draws (general knowledge). Unit test T14 (`taa_test.cpp`): a marked dot is its own
  sample (1), a hole beside it keeps no history (0), an unmarked mask leaves the frame bit for bit, one pixel from a single marked
  one is its own sample and two pixels from it resolves bit for bit as without the mask, a mask without the base is refused.
- **The witness (`i140-e64`, 2026-11-16 00:36-00:45, the same protocol, `build-e66` = `d19156ef3`'s native tree, both reviews' code
  fixes in).** Two native runs, exit 0, no fault, restored: the default TAA with the fade mask, then with `taa-reactive-view`. The
  fade (`evidence/I-0140/e64/hair-notaa-taa-mask-view.png`: rows `no-taa` and the former default from `i140-e62`, the fix, its view):
  under the TAA the hair now breaks into the dots `no-taa` shows, where the former default kept the veil; the view paints the whole
  head red from the fade's first frames, but the view paints the reactive weight too (the whole crop red from its third frame): it
  shows no more than the old form's view; no journal row on that build counts the mask (its counters came later), so the dots on the
  lossless film are the fix's only witness (OBSERVED by eye). The crawl over Archdragon Peak's cloud bank: 0.709 against the former
  default's 0.735 (MEASURED, one run each, no spread known; on that build the bonfire's coiled sword, an effect's model particle, was
  a marked row too, below the cropped upper half: INFERRED). `native_taa_fade_mask_off` never written; `native_taa_frames` as before.
- What it costs and leaves: the dissolving rows are drawn twice while they dissolve (their own pass and the mask's), and their
  fade-mask pipelines compile at the first dissolve of a session (not in the precompile list before it); the mask is drawn after
  every forward and effect row, so a nearer surface drawn after a dissolving row stops the marking only where that surface writes
  depth. A model's first prepare with a moving fade is not marked (no earlier value to compare). Under DLSS nothing changes (DLSS 310.9 takes no such
  input, STUDY-REACTIVE-MASK): Thomas's condition (« si ça n'impacte pas le DLAA », 2026-11-08) kept, and the soft fade under DLSS
  accepted by him on 2026-11-15 at 23:31 (« D'accord, ça me va », after « je veux juste que le DLSS y soit parfaitement intégré
  comme les autres jeux »: a dithered fade under DLSS or a TAA is soft in other games too, general knowledge).
- **The step reviews** (fresh Opus, high; D-0283's brief). The first (`6c9037dc6..70a2276c6`): no High. M1, the mask gated on the
  base being ready rather than copied: on a frame whose forward rows were left unsorted the TAA would have been handed the mask
  without the base and refused it, a renderer fault: fixed (the mask only where the base is copied, `8dddaa957`). L1, a reset frame or
  an off-screen reprojection gave a marked pixel the reconstruction: fixed. L2, the fade-mask pipelines made for every fading row
  whatever the anti-aliasing: fixed (only where the frame runs the TAA with its reactive weighting). L4, T14's unmarked mask compared
  within 1e-3 and no refusal case: fixed (exact; the mask without the base refused). Left: L3 (a fading row whose programme also
  writes SV_Target1, the glow, gets a one-target mask pipeline; its second output is discarded, a debug-layer warning possible under
  `debug-layer.flag`, not seen: no such fading row is known), L5 (the mask pass declares the depth ReadWrite in DEPTH_WRITE, as the
  forward rows' passes do, though it only reads it). The second (`70a2276c6..8dddaa957`): no High. M1, a draw kept across prepares
  kept a stale `fade_mask` when the anti-aliasing changed (the kept-draw epoch did not hold it): fixed (`d19156ef3`, the decision in
  `NativeKeptEpoch`). L2, the mask's graph use and its binding gated differently (never apart today): fixed. L3, T14's refusal not
  told by its reason, the stage not used after it: fixed. Left: L1 (after a `native_taa_reactive_off` latch the fade-mask pipelines
  are still made, never drawn: wasted pipelines only, no picture changed).
  The third (`8dddaa957..d19156ef3`): no High, no Medium; L1 (T14 claimed the refusal counts no frame without checking it) and L2
  (the frame after the refusal a reset frame only) fixed in the test (`396a2b905`, the counters before and after, a still frame that
  resolves from the history); this last test change is covered by the final review.
- **The final review** (fresh Opus, xhigh, `main...57430d9b4`, `main` = `644902ff7`): « do not merge as it stands ». H1, every
  effect's model particle in pass 0x29 (category 0x29 for its whole life: the bonfire's coiled sword `s11600`, in `i140-e64`'s own
  journal) drew into the mask, so the effects lost the TAA at every bonfire: fixed (`2e2d5c31b`, no effect). M1, the category-0x29
  states that last (a phantom's whole presence, Hidden Body, the camera's approach) drew into it too: fixed (no phantom; first a
  level, the group's fade value under 0.95, which its step review's H1 refuted: Hidden Body's value descends to about 0.4 and holds
  there, I-0113 tranche 10d's reference rows, its 0.975 only the value on entry; then, `662d5e1fe`, a row marks the mask only on a
  frame its model's fade value moved since its last prepare). M2, T14's « pixel away » was 0 in every input: replaced
  by one and two pixels from a single marked one. L1 (the cost bullet stale), L2 (« as Thomas accepted » over-read: his words
  quoted now), L4 (STATE, STUDY-ALL-GAPS, the index), L5 (Evidence, Uncertainty, the shader's citations): rewritten. L3 (no counter
  of the mask's scope): `native_taa_fade_mask` (its first frame and rows) and `fade_frames`, `fade_rows` in `native_taa_frames`. L6:
  the mask released with the base when the TAA stops; `lit` removed (implied); the per-frame scan of the forward rows since replaced
  by `fade_mask_draws`. Its code fixes get a step review and a new witness after the quiet window.
  Their step reviews: the first (`57430d9b4..2e2d5c31b`) gave the H1 above (the level refuted) and M1 (the comments said « a
  character's row »: any model's dissolve marks it, an illusory wall's too; reworded), L1 (a moving fade's first frames, 1 to 0.95,
  unmarked: gone with the level), L2 (the first row's count a running total: now that frame's rows). The second (`2e2d5c31b..662d5e1fe`):
  no High or Medium; L1 (`NativeFadeMoving`'s statics outside the thread-affinity check: listed and reviewed in
  `scripts/thread-affinity-allow.json`), L2 (the map cleared wholesale, an old value compared after an absence: entries unread for
  600 serials not compared and pruned), L5 (a comment) and L6 (these records) fixed; L3 (a model's first moving frame unmarked) and
  L4 (a fade the game steps every other frame, or a scene shown again without a newer collect, would mark the mask on alternate
  frames) recorded in « Uncertainty », L4 for the next witness to count (`native_taa_fade_mask`, `fade_frames`).
- **The second final review** (fresh Opus, xhigh, `644902ff7...8a1aa4c09`): no High open (its one High, `0c8aa1402` never compiled,
  `build-e68` failing on C4459, was answered by `8a1aa4c09` and `build-e69`, 179 unit tests passed). M1, gap 23 recorded « fixed »
  on a gate never run live: the Conclusion, Next, Uncertainty, I-0113's row and STUDY-REACTIVE-MASK say now what ran (`build-e66`'s
  broader gate, against native `no-taa`) and the witness to take. L1, L2 (stale wording): fixed. Left, recorded: L3 (a held
  translucency's rows, rebuilt each frame for their pose, resolve their mask pipeline each time, outside the programme set's cache:
  a key string and a lock a row, no picture changed), L4 (a refused mask pipeline drops its colour row with it, as any refused
  pipeline of a row does), L5 (no test of the reset branch, of the mask pipeline's states or of NativeFadeMoving's rules), L6 (the
  glow rows, as before), L8 (ER's TempAA not read for a per-object no-history mechanism, D-0195: an open check for a later TAA step).
  L7: `main` merged again before the merge.
- **The integrity review** (`main...67cc31bc8`): no High; no test, gate or reader weakened, nothing keyed to one scene, no work cut.
  M1 (ROADMAP, STUDY-ALL-GAPS, STATE and STUDY-REACTIVE-MASK said « fixed » without the merged gate's missing witness): each says it
  now. L1 (« no fading row » on the cloud bank, false for `build-e66`; « unchanged within the runs » with one run each), L4 (the view
  presented as the mask's witness) and L5 (the Conclusion's « the original's dots », judged against native `no-taa`): reworded. L2
  (the readers' outputs and the sheets' commands not kept): `e62/crawl-and-sheets.txt`. L3 (« the history's last tenth » called
  OBSERVED from a crf-16 trial): INFERRED in STUDY-ALL-GAPS.

## Tranche 13: the spells' audit (ROADMAP workstream 5 step 5), its tool

Branch `claude/unruffled-nobel-ed8b36`. FINISH-LINE's spells, infusions and statuses « uncertain » (STUDY-ALL-GAPS row 19): every
sorcery, pyromancy and miracle cast once natively beside the original. Attuning 106 spells by the bonfire's menu is not scriptable
reliably, so a test tool does it (STUDY-ATTUNE: the static read of the attuned spells, the subagent's and the pilot's).

- **The tool** (`04_updates_and_test_tools.inc` `ApplyTestAttune`, test tools only): F11 takes the next id of `test-attune.txt`
  beside the module, looks it up with the setter's own lookup `140E32960` (no Magic row: refused, a row says so), attunes it in slot
  0 with the game's setter `140590490`, selects it with `140590FC0`, fills the FP (+0xE4 from +0xE8, INFERRED, range-checked) and
  writes a `test_attune` row (`attuned` true when slot 0 reads back the id and is selected). `Run-AlphaCast -AttuneEach` holds F11
  before each cast (again once when the journal has no row for that cast) and refuses, before the launch, a missing tool file, more
  than 12 casts without it and `-SpellsPerCatalyst` with it. `Run-I140Spells` runs a family (its list `attune-<family>.txt`, the
  106 goods of I-0113's `test-give-spells.txt` split by id: sorceries 38 from the staff, pyromancies 30 from the flame, miracles 38
  from the talisman, Right arrow 1 and 2 from the flame) natively then in the original on Alpha's all-spells copy at Firelink,
  filmed and shot at 400, 900, 1,600 and 3,000 ms, and counts the spells attuned; `Make-I140SpellSheets` puts each spell's
  native shots above the original's.
- **The step reviews** (fresh Opus, high; D-0283's brief). The first (`ac6869c0e~1..7ec4dd4bd`): no High. M1, an id without a Magic
  row was written and reported a success (the setter writes any id): fixed, looked up first and refused. L1 (the -Casts check after
  the launch), L2 (-AttuneEach without its files, or with -SpellsPerCatalyst), L3 (a lost F11 shifts the list; the series did not
  count), L4 (the native runs under the debug layer), L6 (`done` without the list's size): fixed. L5 (the family split by id range,
  not by the Magic param's category): left, a cast from the wrong catalyst shows in the films. The second
  (`7ec4dd4bd..7d54dddcf`): no High. M1, the count read `set`, which the setter returns true for every slot 0: now `attuned` (slot
  0 holds the id and is selected), counted. L1 (the lookup's comment), L2 (no latch for the key: the runner presses again once when
  row k is missing), L3 (the mismatch's wording, no line without a journal), L4 (`opened`; `done` only for a list read), L5 (the
  build folder computed twice): fixed. The third (`7d54dddcf..3f9272e77`): no High. M1, the count could read N of N while
  a cast cast the previous spell (two lost presses realigned by the next retry): the runner now writes after each press the
  journal's last attune row (« attune k row=k' attuned=… ») and the series checks cast by cast. L1 (the lookup's comment, its
  field names) fixed in the comment and names only (`build-e75` built before it: the same code). L2 (the count did not tell refusals
  from lost presses): answered by M1's per-cast check. `build-e75` (`1d24135a2`'s native tree; 178 PASS lines) is the tranche's
  build; `build-e72` (edited while it built) and `build-e73` (stopped) not used.

- **The second pass's tool, since removed** (the final review's M1: the game set the values back, `i140-e80`; STUDY-ATTUNE « The stats »: Intelligence and Faith are PlayerGameData +0x218 and +0x21C, read live
  by the cast gate `140640890`): the list's « int=N » / « fth=N » words make the tool raise them upward at each press (a
  `test_attune_stats` row a press: before, read back, asked, needed, written); `Run-I140Spells` checks every list before the first
  run and every stats row against the list. Its step reviews: the fourth (`e1e0271a6..7597714d5`): no High or Medium; L1 (one row,
  no read-back), L2 (a sign accepted), L3 (lines against words), L4, L5 (comments): fixed. The fifth (`4779e2014..b906e99b3`):
  **H1, a « // » comment placed before the code on one line swallowed the stat words' assignment, their `continue` and a brace**
  (the build failed; `build-e78` stopped): fixed; L1 to L5 (a row when no write is needed, the asked values checked, the parser's
  digits, the refusal's stage, the read-back's result): fixed. The sixth (`b906e99b3..a8e4e4529`): no High or Medium (braces counted,
  no code in a comment); L1 to L5 (the asked stats from the list and each once, words of at most 31 characters, every stats row
  checked, comments, every list checked before the first run): fixed in the scripts and comments (`02ab0509c`, `8a6dd8b28`).
  `build-e79` (`a8e4e4529`'s code, 178 PASS lines) is the second pass's build; `build-e77` and `build-e78` stopped, not used.

### Tranche 13's runs (`i140-e76`, 2026-11-16 06:32-07:40)

- **The trial** (`i140-e76t-pyromancies-nat`, 06:32-06:35, the ASUS window, offline, Alpha's all-spells copy restored by hash,
  `build-e75` = `1d24135a2`'s native tree): Fireball, Fire Orb, Firestorm attuned and cast in turn (`attuned` true, FP 181 of 181
  filled; the shots show the three spells, `evidence/I-0140/e76/trial-sheet.png`).
- **The audit** (`Run-I140Spells -Prefix i140-e76`, six runs, 06:35-07:40, `evidence/I-0140/e76/audit.log`): pyromancies (30),
  sorceries (38, the staff), miracles (38, the talisman), each natively then in the original; every run exit 0, no fault, restored;
  in every run every cast had its own spell (the runner's rows, `attuned` 106 of 106 natively and in the original). The native
  journals: no `native_ffx_skipped` row (no effect class skipped), no magenta, the usual `native_asm_part_missing_*` rows.
- **Cast or not** (OBSERVED in the journals, the integrity review's F1): each press's row reads the FP before the fill, so press
  k+1 at its maximum means cast k spent nothing: not cast (the series now counts it; the last cast of a run has no next press and
  rests on its pictures). In both renderers alike, 26 of the first pass's 106 spent no FP: 18 sorceries (1305000 to 1307000,
  1310000 to 1313000, 1330000, 1331000, 1340000, 1370000, 1372000, 1603000, 1604000, 1613000, 1620000, 1651000, 1740000), Fire
  Surge (2405000, a held cast: one click), Carthus Flame Arc (2435000), Homeward (3521000, its travel), Force (3530000, the cast
  after Homeward's warp), Lightning Blade (3590000), Darkmoon Blade (3591000), Dark Blade (3640000) and Blessed Weapon (3760000):
  the weapon buffs with a flame, a staff or a talisman in the right hand. The « every cast had its own spell » check proves the
  attunement and the selection, not the cast.
- **Read** (OBSERVED by eye): the 106 sheets (`evidence/I-0140/i140-e76-sheets/`, the shots at 0.4, 0.9, 1.6 and 3 s, native
  above) triaged by three Sonnet subagents (`triage-*.tsv`, 19 marked differs or unsure); the pilot re-read 30 casts on film
  strips aligned on each cast (every 300 ms over 3.6 s of `casts.mp4`, `Make-I140SpellStrip.py` with step 300 and span 3600,
  `evidence/I-0140/i140-e76-strips/`): those 19, 5 of the no-effect ones and the 6 the triage marked alike with a difference in its
  note (Bountiful Sunlight, Caressing Tears, Homing Soulmass, Wrath of the Gods, Crystal Hail, Old Moonlight). The sheets'
  differences were the cast's phase (the click lands 0.5 to 1 s before the cast); every cast re-read was alike (Lightning Storm's
  column, Sunlight Spear, White Corona's rings, Lifehunt Scythe, Projected Heal's glyph ring, Sacred Oath, Great Heal, Great
  Lightning Spear, Dorhys' Gnawing, Lightning Arrow, Replenishment, Soul Greatsword, Snap Freeze, Chaos Bed Vestiges, Fireball,
  Flame Fan and the six); **not compared:** Chameleon's object and Firestorm's pillars, random draws (each renderer drew its own).
  So of the 80 spells cast in the first pass, **78 drawn alike, none seen differing**: 22 by the pilot's eye on the strips (256x144 a
  frame: colour, shape and timing; the other 8 strips were casts not made or not compared), the other 56 by the Sonnet triage of the
  sheets alone (INFERRED, not the pilot's eye).

### The second pass (`i140-e80`, 2026-11-16 09:08-09:26)

- **The runs** (`Run-I140Spells -Prefix i140-e80 -Families sorceries2,miracles2`, `build-e79`, the ASUS window, offline, restored
  by hash; `evidence/I-0140/e80/audit2.log`): four runs, exit 0, no fault; 18 and 2 spells attuned in each renderer; a tool's raise
  of Intelligence and Faith to 99 (since removed: below) read back 99 after each press.
- **Read** (the film strips `evidence/I-0140/i140-e80-strips/`, a Sonnet triage `triage.tsv`, and the FP, OBSERVED): Force
  (3530000) cast (FP spent) and drawn alike; Homeward not cast; **the 18 sorceries still not cast in either renderer** (no FP spent).
- **Why:** every stats row read Intelligence **25 before each press** (OBSERVED in the journal): the game had set +0x218 back
  between presses (an effective value recomputed from a base the tool did not write: INFERRED, the writer not read), so the raise
  never reached a cast; the tool's raise and the series' check of it, a check that could not fail on its own terms (the final
  review's M1), are removed. The Magic param (`evidence/I-0140/e80/magic-param.tsv`, `Get-ParamRows -Param Magic`, READ): 11 of
  the 18 need more than 25 Intelligence (row +0x1E: 27 to 50; the 20 sorceries cast need 8 to 25); 5 carry row +0x2D bit 2
  (1310000 to 1313000, 1340000) and 2 bit 3 (1330000, 1331000), which the cast gate tests against the hands' weapon rows (STUDY-
  ATTUNE « The stats »); the four other spells with bit 2 (2435000, 3590000, 3591000, 3640000) were not cast either: a weapon
  (or, for bit 3, a shield) in hand is what they need, INFERRED.
- So the audit ends at **81 of 106 spells cast, 79 drawn alike (78 and Force), none seen differing, 2 cast but not compared**
  (Chameleon, Firestorm); **25 not cast** in either renderer: 11 high-Intelligence sorceries, 11 needing a weapon or a shield in
  hand (the 7 sorceries with the +0x2D bits, Carthus Flame Arc, Lightning Blade, Darkmoon Blade, Dark Blade) and Blessed Weapon,
  Fire Surge (a held cast), Homeward (a travel, nothing to draw beyond its banner). Left for a later tranche, on a copy of
  Thomas's current Alpha save (Thomas, 2026-11-16 09:46).

## Tranche 14: the 25 spells left, on copies of Thomas's Alpha

Branch `claude/vigorous-yalow-855296`. Thomas (2026-11-16 09:46): the spells left on a copy of his current Alpha, restored by hash.

- **The saves, read before any run** (OBSERVED: the slot decrypted with the public save key, as `Test-SaveSteamId.ps1`; the stats'
  order checked on Alpha's all-spells copy, whose Intelligence 25 the cast gate read live in `i140-e80`, and by the soul level, the
  sum of the nine less 89). Thomas's live save (`%APPDATA%/DarkSoulsIII/…/DS30000.sl2`, last written 2026-11-11 08:19) has the
  bytes of `thomas-alpha-cape-20261111-124806` (SHA-256 2713CD6B…): Alpha, level 144, Vigor 14, Attunement 50, Endurance 12,
  Strength 21, Dexterity 14, **Intelligence 50, Faith 50**, Luck 11, Vitality 11, FP 117 of 326; the 25 spells need at most 50 and 30
  (the Magic rows' +0x1E, +0x1F). Its hands (the slot's equipment, names from the engUS weapon FMG): right Heretic's Staff,
  Pyromancy Flame, Sunlight Talisman (current), left Carthus Shield, Fists, Fists (current): **no melee weapon**. His copy of
  2026-10-31 (`thomas-alpha-20261031-0851`, the same stats) holds a Claymore in the right hand and a Talisman and a Sorcerer's
  Staff (current) in the left: the weapon buffs are cast from it, from the left hand. Reader: `Read-SaveSlot.py`, its output `evidence/I-0140/e84/saves.txt`
  (the all-spells copy's FP 181 matches tranche 13's trial, a second check of the offsets).
- **The tool** (`Run-AlphaCast -CastPlan`): keys before a cast (a catalyst or a hand switched inside one series) and a held
  cast for one cast; `Run-I140Spells` families `thomas` (16: the 11 high-Intelligence sorceries and the two shields from the
  staff with the Carthus Shield in the left hand, Fire Surge held 4 s from the flame, Homeward from the
  talisman, then Homeward again so that its F11 press reads the FP the first one spent) and `thomas-weapon` (14: the five sorcery buffs, the four miracle buffs twice, a Left arrow before each block of four, Blessed Weapon again for the same reason).
- **The cast gate's hands** (READ, GhidraMCP, 2026-11-16 11:20, the pilot): `140640890`'s caller for the menu `1406D35B0` passes
  the right hand's current weapon (`140868C50(ChrAsm, -1)`: slot current x 2 + 1, the save's R slots) and the left hand's (-2: x 2,
  the L slots); a Magic row with +0x2D bit 2 needs the right hand's EquipParamWeapon row +0x102 bit 7, one with bit 3 the left
  hand's +0x101 bit 5 (`140E41670`, the weapon lookup). Read in the rows (`Get-ParamRows -Param EquipParamWeapon`,
  `evidence/I-0140/e84/weapon-param.tsv`): the Claymore 6020000 and 6021300 +0x102 C8 (bit 7 set); the staffs, flame and talismans
  10 to 12 (clear); the Carthus Shield 21070000 +0x101 27 (bit 5 set). So the buffs need a weapon in the right hand and the
  catalyst in the left, the shields a shield in the left hand: **Carthus Flame Arc** needs a flame in the left hand and a weapon in
  the right, which no copy holds (`saves.txt`; the 129 save copies under `.ds3-i0039-backups` read by `Read-SaveSlot.py`, 105 holding an Alpha, `e84/copies-hands.txt`: no flame in a left hand): left for a test tool that
  equips a hand (with the infusions and resins, which need the same).
- **Step review** (fresh Opus, high; `92cab714e..ab559177c`): H1, Carthus Flame Arc in hands that cannot pass its gate: confirmed
  by the read above, the spell taken out of `thomas` (24 spells in this tranche). M1, the copy's hash checked after the test files
  were written (a refusal blocked every later run): checked first. L1 (`-First` with a plan), L2 (the plan's time without F11), L3
  (the last cast not judged by FP: Homeward and Blessed Weapon cast once more), L4 (the save's FP was the current one: now « 117
  of 326 »), L5 (the names file's parsing): fixed.
- **Second step review** (fresh Opus, high; `ab559177c..3d170ae77`): no High or Medium. L1 (one block of four miracle buffs
  spent without the talisman, the Fists question open): left, the cost of a hedge (about a minute a run; `plan-6-1.png` settles it). L2
  (Homeward's FP read by a press during its warp): Homeward is judged by its warp on the film; the series now names a cast whose
  next press wrote no FP row. L3 (the copies' search not recorded): `e84/copies-hands.txt`. L4 (an unreadable copy threw): refused.
### Tranche 14's runs (`i140-e84`, 2026-11-16 11:51-12:15)

- **The runs** (`Run-I140Spells -Prefix i140-e84 -Families thomas,thomas-weapon`, `build-e84` = `build-e82` copied, module SHA-256
  76DCCDE0…, main's native code; the ASUS window, offline, `evidence/I-0140/e84/audit3.log`): four runs, exit 0, no fault, every
  restoration ok, Thomas's live save at its hash after the series (2713CD6B…). Every cast had its own spell (the runner's rows),
  16 of 16 and 14 of 14 attuned in both renderers; the plan's shots show the hands (`plan-1-2.png`: the Carthus Shield in the left
  hand, the Heretic's Staff in the right).
- **Cast or not** (OBSERVED, the FP of the next press, alike in both renderers): `thomas` casts 1 to 14 spent FP (the 11
  high-Intelligence sorceries, Magic Shield, Great Magic Shield, Fire Surge held); Homeward (15) spent none: its cast opens the
  game's « Select destination » prompt (Shrine Bonfire, Bonfire last rested at; `cast-15-1600.png`, both renderers), which the
  series did not confirm. `thomas-weapon` casts 1 to 9 spent FP (the five sorcery buffs from the staff, the four miracle buffs
  from the talisman after one Left arrow), 10 to 13 none: **the Left arrow skips the Fists slot** (L2 staff, L1 talisman, L2 staff
  again), the first block held the talisman.
- **Read** (OBSERVED by the pilot's eye, film strips every 600 ms over 3.6 s from each click, 256x144 a frame,
  `evidence/I-0140/i140-e84-strips6/`, and every 300 ms in `i140-e84-strips/`): the 23 casts drawn alike in shape, colour and
  timing: Homing Crystal Soulmass, Soul Spear, Crystal Soul Spear, White Dragon Breath, Farron Hail, Affinity, Great Soul Dregs,
  Dark Edge, Soul Stream, Twisted Wall of Light, Pestilent Mist, Magic Shield, Great Magic Shield, Fire Surge; Magic Weapon, Great
  Magic Weapon, Crystal Magic Weapon, Frozen Weapon, Hidden Weapon; Lightning Blade, Darkmoon Blade, Dark Blade, Blessed Weapon.
  The differences seen were the cast's phase (Crystal Soul Spear's and Great Soul Dregs' flash one frame apart). A brighter glow
  on the character natively during Lightning Blade, seen by eye, is not borne out by the pictures (`signalstats`, the character's
  crop 300x400 at 500,250: 74.8, 60.0, 78.6 natively against 76.7, 61.5, 73.9 at 0.4, 0.9 and 3 s).
- So the spells' audit stands at **104 of 106 cast, 102 drawn alike, none seen differing, 2 cast but not compared** (Chameleon,
  Firestorm); **left: Carthus Flame Arc** (a flame in the left hand, a weapon in the right: an equip tool) **and Homeward** (its
  prompt confirmed: a key after the click).

## Tranche 15: a hand and a SpEffect by a test tool (the buffs, the statuses, Carthus Flame Arc, Homeward)

Same branch. STUDY-EQUIP-SPEFFECT (a `ghidra-research` read, the calls re-read by the pilot in GhidraMCP before the code).

- **The tool** (`04_updates_and_test_tools.inc` `ApplyTestEffect`, test tools only): F1 takes the next word of `test-effect.txt`
  beside the module: `s<id>` a SpEffect on the player through `140888F20(ChrIns, id, 1.0)` (the equipment effects' own add),
  refused without a SpEffectParam row; `L<id>`, `R<id>` a weapon (its item id, an inventory entry's +4) into the left or the right hand's current
  slot by the equipment menu's own sequence (`140B064E0`: the inventory index found by its value, its earlier slot cleared,
  `140584FF0`, `1405859A0`, `1408C0930`), refused when not owned; one `test_effect` row a press (a hand's slot read back).
- **The series** (`Run-AlphaCast -CastPlan` gains `a<vk>`, a key after a cast's shots): `thomas-tools` (the current copy: the
  Dagger into the right hand and the flame into the left, Carthus Flame Arc twice, the Sunlight Talisman into the left hand,
  Homeward last with E on its prompt; Thomas 2026-11-16 11:20: « une arme en main droite (prendre une dague…) et la pyro en main
  gauche ») and `thomas-buffs` (the Dagger, F4, then one effect a swing: the eight resins' SpEffects 2100 to 2190, the statuses'
  candidates 103, 104, 102, 101). Homeward's arrival to be compared with a fast travel's fade (Thomas, 11:21: « Logiquement
  Homeward aura le même effet qu'un voyage rapide mais oui à vérifier »).
- **Next, the weapon arts with an effect of their own** (Thomas, 11:23: « certaines armes qui ont des effets uniques en weapon art,
  par exemple la plus connue la Moonlight Greatsword »): each such weapon given (F7) if not owned, put in the right hand (F1), its
  art used once natively and in the original; the list from EquipParamWeapon (the art's id) and Thomas's examples. Read
  (`Get-ParamRows`, the pilot): the art's id is EquipParamWeapon +0x194 (SwordArtsParam's 185 row ids; the Moonlight
  Greatsword 6320000 has 303, the straight swords 200, the Dagger 100); 152 arts belong to one or two base weapons
  (`evidence/I-0140/e84/arts-unique.tsv`, one weapon each, staffs, bows and shields included). Tranche 16 first: the Moonlight
  Greatsword (`thomas-arts`), which settles the keys; then the 118 melee ones (`arts-melee.txt`) in four batches of 30.
- **Step review** (fresh Opus, high; `8095f4560..4dba3c646`, the calls checked in the disassembly): no High. M1, the hand path
  left out the menu's gate `140B06930(slot)` and its last call `1406C1DD0(slot)` (a hand raises a flag at
  `[[player+0x1F90]+0x20]+0x50`): both called now, the menu's record `140765220(0x47, index)` named as left out. M2, the Dagger's
  inventory value not shown: the copy's slot holds the value 1000000 (`e84/cape-inventory-check.txt`, with the flame and the
  Sunlight Talisman). M3, a lost or refused F1 word went unseen and shifted the later ones: a word waits without a player, the
  runner presses F1 again when the journal gained no row and names each press's row, the series checks every word applied in
  order. L1 (the sheets' families), L2 (the FP line for the swings), L3 (the study's stale sentence and `1408C0930`'s argument):
  fixed. `build-e85` failed (C4459, a local shadowing a global), `build-e86` built the first version (178 PASS lines).
- **Second step review** (`4dba3c646..490efcdfa`): no High. M1, the words' check proved the tool's order, not which cast's press
  took each word: the series now checks press by press (the w-th F1 press of the plan must log word w; a press without its word
  names the casts after it). L1 (the menu's calls the tool leaves out, named in the comment), L2 (a gate's refusal spent the word:
  it waits now), L4 (the swings' FP line): fixed. L3 (the swings' sheets named by the filler spell): the strips are named by the
  effect instead. `thomas-tools` reordered: Homeward last (its warp would have fallen before the flame's press). `build-e87`
  (the first fixes, 178 PASS lines), `build-e88` (these).
- **The first runs** (`i140-e88`, 13:13-13:19, `build-e88`, `evidence/I-0140/e84/audit4.log`): `thomas-tools` natively and in the
  original, exit 0, no fault, restored (Thomas's live save at its hash after). Every F1 press took its own word (the press-by-press
  check), but every hand word was refused: `index` -1 (OBSERVED in the rows). The tool matched the item id against an entry's
  +0, which holds the entry's handle (0x8080xxxx for a weapon, what 140A323E0 returns and ChrAsm +0x24 holds; the save's slot
  lays an entry out as handle, id, count): the id is at +4 (fixed, `88d266292`, `build-e89`). The four later runs were stopped
  before their launch (a marker file beside the module, which the series refuses; removed after), so no run was spent on a known
  failure.
- **Third step review** (`490efcdfa..e11975498`, with tranche 16's runner changes): no High or Medium. L1 (a global -CastHoldMs
  dropped on a b1 cast: refused now), L2 (two confirms for one item given: one an item), L3 (the strong attacks judged by FP:
  judged on the film, said in the summary), L4 (the presses' shortfall named; the arts' sheets named by the filler spell: the
  strips are named by the move): fixed in `9bf08ce00`.

### Tranches 15 and 16's runs (`i140-e89`, 2026-11-16 14:17-14:43)

- **The runs** (`Run-I140Spells -Prefix i140-e89 -Families thomas-tools,thomas-buffs,thomas-arts`, `build-e89` = `88d266292`'s
  native code, 178 PASS lines; the ASUS window, offline, `evidence/I-0140/e84/audit5.log`): six runs, exit 0, no fault, every
  restoration ok, Thomas's live save at its hash after. Every F1 press took its own word and every word applied (17 rows: the
  hands read back, the SpEffects added), in both renderers.
- **Carthus Flame Arc** (the Dagger in the right hand, the flame in the left, Thomas's advice): cast (FP spent) in both, the
  flame's burst and the burning dagger alike (strips `i140-e89-strips/tools-1.png`, `tools-2.png`). **The spells' audit: 105 of 106
  cast** (Homeward below), 103 drawn alike, none differing, Chameleon and Firestorm not compared.
- **Homeward** (the Sunlight Talisman in the left hand, E on the « Select destination » prompt): cast in both, the golden ring
  then the fade to the loading screen alike (`tools-3-long.png`); the whole picture's luma (`signalstats`, 10 a second from 5 s
  after the click): the ring to 56 natively and 53 to 54 in the original, the fade over about 0.5 s to 17 and 16, the loading
  screen 21 and 22 (OBSERVED). Thomas's « le même effet qu'un voyage rapide »: the same fade natively as in the original; the
  fast travel's own fade is gap 23's (tranche 12). So **106 of 106 spells cast, 104 drawn alike, none seen differing, 2 cast but not
  compared** (Chameleon, Firestorm).
- **The resins** (SpEffects 2100 to 2190 on the player, the Dagger swung; cropped strips `i140-e89-crops/buffs-*.png`, 440x440
  around the character): Charcoal Pine Resin and its Bundle (fire), Gold Pine Resin and its Bundle (lightning), Rotten Pine
  Resin (the purple mist), Human Pine Resin (dark), Carthus Rouge (blood), Pale Pine Resin (magic): each buff drawn on the
  dagger in both, alike in colour, shape and timing (OBSERVED by the pilot's eye at 220x220 a frame). The character itself is
  darker and warmer natively (the traced lighting without the fill lights, D-0262: R's, not the buffs').
- **The statuses' candidates** 103, 104, 102, 101: added (rows applied) but nothing shown in either renderer, no aura, no HUD
  icon (OBSERVED): cure rows (amount -99999, STUDY-EQUIP-SPEFFECT « B2 »); the statuses' own rows next (`thomas-status`:
  22019, 24019, 20019, 21019, 33). The weapon arts' batches `arts-1` to `arts-4` (118 weapons, 30 a run, the 23 the copy lacks
  given first, `arts-give.txt`) are ready.
- **Fourth step review** (`e11975498..4e6879f74`): H1, 69 of the 118 weapons need more Strength or Dexterity than the copy's 21
  and 14 (EquipParamWeapon +0xEE, +0xEF), and an art is refused below its weapon's requirements (the reviewer's knowledge of the
  game, not checked here): the batches now run on a new copy, `thomas-alpha-cape-stats99-20261116-1500` (SHA-256 500C0479…,
  `New-StatsCopy.py`: the source copy only read, Strength, Dexterity, Intelligence and Faith 99, the level the sum less 89, the
  slot encrypted again and its MD5 renewed, every entry's MD5 checked by `Test-SaveSteamId.ps1`; not Thomas's save, which stays
  untouched). M1 (the exact ids: `arts-give.txt` was taken from the copy's exact weapon values, so a weapon held only at +N or
  infused is given; the timing: about 21 s a cast, about 800 s a run, the film's bound 660 s against about 600 s used): said
  in the comment. L1 (the FP lines name the weapon for the batches), L2 (the statuses' FP caveat), L5 (more than 120 weapons
  refused), L7 (the crop strip's intermediates in a folder of their own): fixed. L3 (the statuses' sheets by the filler): the
  crop strips are named by the row. L4 (one E an item, seen with one item): batch 1's `given.png` is read before the others.
  L6 (the crop strip repeats the strip script): kept, a separate tool.
- **Fifth step review** (`4e6879f74..3b1ad953b`): no High; the copy checked byte by byte (only USER_DATA000 differs, the five
  intended ints, every MD5 valid, the source unchanged). M1 (the MD5s after the target slot not checked first: two passes now),
  L1 (the length assumed: refused otherwise), L2 (the first slot holding the name: one slot, the name NUL-ended), L3 (the
  comment's « beside »), L4 (the 120 guard stopped every family: it now drops the arts batches only): fixed; the copy made again
  by the new script has the same SHA-256 (500C0479…). M2: USER_DATA010's profile summary still says level 144 and holds a
  stale image of the slot (not updated, said in the script): whether the game minds is seen on the first batch's load.
- **The Moonlight Greatsword** (given by F7, put in the right hand by F1, the left hand bare): its art Moonlight Vortex (Shift
  and the right button, FP spent) and the charged strong attack's wave (Shift and the left button held 4 s) both came in both
  renderers at the same time; the vortex alike in brightness (the crop's luma peaks at 85 natively and 84 in the original over
  about 0.8 s, OBSERVED, `signalstats`; the wave not measured: its frames show the original's blade and character greener and
  brighter); **seen differing in look** (`i140-e89-crops/mlgs-native-left-original-right.png`):
  natively a horizontal glare streak across the vortex and the floor lit green around the character, the original's glow
  rounder with no streak (one frame of phase between the two pictures). The streak is the post chain's glare, the floor's
  green the effect's light (both R's: workstream 11's glare, the effects' light), INFERRED, not yet A/B'd. So Shift with the
  right button is the art and Shift with the left the strong attack on this keyboard map (OBSERVED by the FP and the film).

### The statuses (`i140-e89-thomas-status`, 2026-11-16 14:58-15:05)

- Two runs (`build-e89`, the current copy, `evidence/I-0140/e84/audit6.log`): exit 0, no fault, restored, Thomas's save at its
  hash after; every F1 word applied in order. Read on the cropped strips (`i140-e89-crops/status-*.png`, OBSERVED by the pilot's
  eye): **poison** 20019 (the purple aura and its HUD bar) and **toxic** 21019 (the darker aura, a second bar) alike in both;
  **frost** 24019 its HUD bar in both, its aura faint (a few white specks) in both; **curse** 33 the petrified, spiked body and
  « YOU DIED » in both, alike (the invincibility test tool does not stop the curse); **bleed** 22019 nothing in either (its row
  lasts 0.1 s; that its blood burst comes from the gauge's fill, which the direct add skips, is INFERRED: STUDY-EQUIP-SPEFFECT
  « B2 »). The
  character natively darker, as in the resins' strips (the lighting's).

### Tranche 16's arts batches (`i140-e89-arts-*`, 2026-11-16 from 15:06)

- **The runs** (`Run-I140Spells -Families arts-1,...`, `build-e89`, the stats-99 copy, `evidence/I-0140/e84/audit7.log` and
  `audit8.log`): `arts-1` and `arts-2` (15:06-15:58), then, after R's merge, `arts-3` and `arts-4` (16:38-17:27), each natively
  then in the original: eight runs, exit 0, no fault, restored, Thomas's save at its hash after; the copy loaded in all eight
  (USER_DATA010's stale level did not stop it); every F1 word applied in order (118 a renderer); the weapons the copy lacked
  given (10, 2, 6, 5: `test_give_done`). The pause: a test file held open beside the module, so the series refused `arts-3` and
  `arts-4`; released and removed after.
- **FP** (OBSERVED by the next press, which judges casts 1 to 29 of a batch, the last one not): spent alike in both renderers by
  19 of `arts-1`'s 29, 26 of `arts-2`'s 29, 29 of `arts-3`'s 29, 23 of `arts-4`'s 27; none spent, in either, by 17: `arts-1` 5,
  10 to 13, 16 to 18, 21, 26; `arts-2` 4, 22, 24; `arts-4` 6, 7, 10, 25. Read on strips for five of them (`arts-1` 5, 10 to 13):
  the same move in both renderers, an idle stance after the click (the art not used: why not read) or Valorheart's guard.
- **The pictures** (`Get-I140CastLuma.py`: the luma of a 640x480 crop around the character, 10 a second over 4 s from each click,
  `e84/arts-1-luma.tsv` to `arts-4-luma.tsv`, OBSERVED): 112 of the 118 peaks within 10 % of the original's; six beyond it,
  `arts-3` 2 (Yhorm's Great Machete, 14 % lower), 23 (Dragonslayer Spear, 11 % higher), 30 (Winged Knight Halberd, 12 %),
  `arts-4` 13 (Dark Hand, 10 %), 15 (Claw, 13 %), 21 (Sellsword Twinblades, 11 %); the native means 5 % darker to 10 % brighter
  (the automatic exposure, D-0280), as large as the screen's 10 %: the screen is a lead for which strips to read, not a
  judgement, and 106 of the 118 arts were not read by eye. Read on cropped strips: the six beyond 10 %: alike but Yhorm's
  Great Machete, whose blade is dark natively and silver-white in the original (the metal's lighting: R's, INFERRED); the
  Dragonslayer Spear's lightning and the Halberd's fire a frame apart; Dark Hand and Claw alike. Read by eye on cropped strips (`i140-e89-crops/arts1-*.png`): Wolnir's Holy Sword's dome (`arts-1` 14), Lorian's
  Greatsword's flame (29) and the sparks of cast 10: alike in shape, colour and timing (casts 16 and 21's strips made, not read).
  The luma figures this RESULT cites for Lightning Blade, Homeward and the Moonlight Vortex: `e84/luma-checks.txt`.

## Tranche 17: the shields', catalysts', torches' and bows' arts, arrows and bolts, the Stance arts, the goods' use

Branch `claude/peaceful-poitras-c27a29` (the fresh E thread, from `main` `0a003878c`). Step 5's left list (ROADMAP), the parts a
test tool on the player alone can reach, the goods' own use (a quick-item slot) included; the statuses on NPCs, the infusions on a
hit and bleed's burst need a target and come next.

- **The static read** (GhidraMCP, the pilot, 2026-11-16 18:20-18:35, READ): `140868490` maps a hand's code to an equipment slot:
  -2 and -1 the hands (ChrAsm +0xC, +0x10: current x 2, +1 for the right), -6 and -5 an arrow (+0x14, +0x18: current x 2 + 6), -4
  and -3 a bolt (+0x1C, +0x20: current x 2 + 7); -5 and -3 are the right hand's (READ by the step review, below). The menu
  `140B064E0` runs one equip sequence for every slot below 0x16 (its clear differs: the protectors skip it, `1406AAA50`'s
  0x10000000; the accessories take a plain clear; only slots to 0xB the +0xE2 test) (`1405826A0`: 0x16 to 0x25 are the quick items, through `140582550`'s
  index and `140585960`, EquipGameData +0x268 + 8 x index); its gate `140B06930` tests the player's state (`1408BD8F0`) for every slot, and
  more only for the protectors (0xC to 0x10) and slot 0x15; `1406C1DD0` raises the player's flag for every slot but the accessories (0x11 to 0x15).
- **The tools** (`04_updates_and_test_tools.inc`, test tools only): F1 gains `A<id>` and `B<id>`, an arrow or a bolt into the
  right hand's current arrow or bolt slot by the same sequence as a hand; F7 a quantity, `w<id>x<n>` (1 to 99, the award buffer's
  own field; ammunition is spent by the shot). `Pilot-Game -Action stance` (Shift and the right button held, the left clicked
  after -Ms, then released) and `Run-AlphaCast -CastPlan` `s<ms>`: a Stance art spends its FP only on the attack made inside it
  (OBSERVED below; the bows' arts were not reached), which tranche 16's plain click never made (17 arts, the strips showing an
  idle stance). Shift stays held through the left click, so the attack inside is the strong one by the keyboard defaults
  (INFERRED); the light one is not audited. The film's length now counts a plan's presses.
- **The families** (`arts-t17.txt`, read by `Run-I140Spells`, the stats-99 copy): `arts-shields` (the 12 shields with an art of
  their own and the Carthus Shield's parry, each in the left hand beside the Dagger, then in the right hand with the left bare:
  which hand's art the game takes beside a weapon is not read), `arts-catalysts` (the 11 catalysts with an art of their own, the
  Sorcerer's Staff and the Talisman for the common ones, the Torch and the Follower Torch), `arts-bows` (8 bows and 2
  crossbows, each with its ammunition, the art as a stance with a shot at 700 ms), `arrows` (the 8 arrows from the Longbow, the 4
  greatarrows from the Dragonslayer Greatbow, the 7 bolts from the Light Crossbow, one shot each), `arts-stance` (tranche 16's 17,
  Storm Ruler's charge held 3 s).
- **The goods** (a `ghidra-research` read, 2026-11-16 18:40, the calls the tool makes re-read by the pilot in GhidraMCP): F1's
  `Q<id>` puts a goods into quick slot 0 by the menu's quick-item branch and selects it (EquipGameData +0x250 +0x68, written by
  `14058F4B0`, the next occupied slot after its argument); the game's use input (R) reads the selected slot through
  `140583DA0` (re-read by the pilot, below; the request path from the input to the animation not found). `CastPlan` `u<vk>`
  makes a cast a key's tap. Families `goods-thrown` (17: knives, firebombs, urns, pies, charms, Prism Stone, the two dragon
  stones last) and `goods-use` (29: the resins by their own use, Purging Stone, Repair Powder, blessings, blossoms, mosses,
  pellets, coins, Siegbräu, two souls, Ember, Homeward Bone last with E on its prompt), the Dagger in the right hand.
- **Step review** (fresh Opus, high; `db7b9277b..55e207f17`, the game's functions checked in its own disassembly): no High. M1, the
  stance could leave the left button or Shift held when stopped: every release now in its `finally`. M2, malformed lines of
  `arts-t17.txt` (a stance's ms, give words, an id at or above the module's bound, words after a cast) failed only inside a run:
  refused before the first run, a `#` comment cut first. M3, the « not cast » line would name every arrow shot (no FP spent): the
  arrows and goods families say it does not apply. L1, an arrow or bolt slot holding ammunition takes the menu's extra
  `1405859A0` between the clear and the equip: called. L2, a stance's shots and `video_ms` were timed from its end, about a second
  after the attack: timed from its left click (`clicked=` from `Pilot-Game`). L3, a stance with the global `-CastHoldMs` or
  `-Button right`: refused. L4, `A` and `B` did not check the item's kind: refused outside the arrows' and bolts' id ranges. L5,
  the right hand's -5 and -3: READ by the reviewer at `1408BB0A7`-`1408BB149` (-6 and -4 followed by `140868C50(-2)`, -5 and -3 by
  `(-1)`), the comment cites it. L6, F7's `x` took a sign: refused.
- **Second step review** (fresh Opus, high; `55e207f17..33f79d866`, without GhidraMCP: the quick branch's calls against the binary
  not checked by it): no High. M1, Homeward Bone last for its warp would stop at its « Select destination » prompt (as Homeward
  did, tranche 15): a cast line may end with `after <vk>`, Homeward Bone's with E. M2, `equipped` true might not mean the use
  reads the item: READ by the pilot in the disassembly, `140583DA0` takes EquipGameData +0x310 when not negative, else
  `14058ED60`, which passes +0x250 +0x68 (the selection the tool writes) to `14058A6F0` for the slot's handle; the row now also
  needs +0x310 negative and logs it. L1, the quick branch's left-outs named in the comment. L2, a Q word could put a non-goods
  entry in the quick slot: refused unless the value's nibble is 4. L3, the dragon stones before Homeward Bone (a form may block
  the next use): moved to the end of `goods-thrown`. L4, goods with a use left out silently: Hidden Blessing, Budding Green
  Blossom, Blooming Purple Moss Clump, Purging Stone, Repair Powder, the Twinkling stones, Siegbräu, Rusted Gold Coin and a
  second soul added (29 and 19 casts); the rest named with its reason in the file's head. L5, a key cast with the global hold or
  right button: refused. L6, the row's `read_back` for Q is the selection: said, slot 0's index and the override logged.
- **Third step review** (fresh Opus, high; `33f79d866..241f1b4da`, the comments' addresses checked in the binary by
  `llvm-objdump`): no High. M1, a stance returns about a second after its click, so its 400 and 900 ms shots were taken late
  under those names: a shot more than 150 ms late is skipped and said (the film holds that moment; in the runs every stance
  returned about 1,040 ms after its click, so its 400 ms shot was skipped and its 900 ms one, about 140 ms late, kept under its
  name: a stance's sheets are not read by their names' times, its film strips are). L2, a stance without its click time: said. L3, four dragon forms in a row: the Twinkling
  stones left out (the same forms, INFERRED), named in the file's head. L4, A and B ids outside the ammunition ranges: refused
  before the run too. L1 (the menu's extra `1405859A0` decided from the slot's id read before the clear, and by +0xE2 rather
  than an id range: rows 98000 to 99100, 409900, 419900 also carry 0xD or 0xE), L5 (a comment's order) and L6 (a sign before an
  id in F1 and F7, a left release never pressed): in the module, answered after the runs (no build while R's run held the PC;
  the runs' module `build-t17d` is `241f1b4da`'s code).
- **A loss** (the pilot's error, 2026-11-16 18:32, the empty file's time): the first build of this tranche was asked into `build-e90`, which tranche 16
  had used; the build refused the folder, but its log redirection had already overwritten `build-e90.log`, tranche 16's log of
  that build (178 PASS lines, cited below in « Tranches 14 to 16's reviews at the merge »). The build folder `build-e90` is
  untouched; the log's content is lost. This tranche's builds are named `build-t17*`.

### Tranche 17's runs (`i140-t17` 19:31-21:43, `i140-t17b` 21:43-22:25, `i140-t17c` from 22:47, 2026-11-16)

- **The runs** (`Run-I140Spells -Prefix i140-t17`, `build-t17d` = `241f1b4da`'s module, a cache build without the tests, as `build-t17e`; the ASUS
  window, offline, the stats-99 copy restored by hash, `evidence/I-0140/t17/audit1.log`): seven families, each natively then in
  the original, fourteen runs, exit 0, no fault, every restoration ok. Every F1 word applied in order in both renderers for the
  five families of arts and ammunition (39, 15, 13, 22, 17 words: the hands, arrows and bolts read back). Read with the luma screen
  (`Get-I140CastLuma.py`, `t17/*-luma.tsv`) and cropped film strips from 1 s before each click (`t17/crops/`), OBSERVED by the
  pilot's eye (frames of 320x240 in the single shield strips, 240x180 in Havel's, the Crucifix's and the arrows', 160x120 in
  the montages of the catalysts, bows, Stance arts and goods).
- **The shields** (26 casts): FP alike in both (ten casts spent none in either: 2 to 6, 9, 10, 19, 20, 25; the parries, Spiked
  Shield's two casts, the Giant Door Shield's two, the Carthus Shield's left one). 25 of 26 within 10 % on the
  screen; read: the Bonewheel's spin, the Dragonhead Greatshield's white burst, the Ghru Rotshield, the Dragonhead Shield's fire,
  Havel's Greatshield in the left hand (the dust, then the stone skin): alike. **Seen differing: Havel's Greatshield in the right
  hand** (cast 14, its Stone Flesh still on from cast 13): natively its dust burst is drawn again 1.5 to 2.4 s after the click,
  in the original none, though the art spent its 16 FP in both (the next press's FP, 310 of 326 in both)
  (`crops/havel-14-native-top.png`, native above). Why: not read (an effect the original kills or never spawns on the art's
  second use, which the native draws: INFERRED, a lead). Which hand the game takes the art from beside the Dagger: each left-hand
  cast spent its shield's art cost (`e84/swordarts-param.tsv` +0x0E: Pierce 14, Moaning 10, Havel 16, Bonewheel 14, Dragonhead
  Greatshield 16, Ghru 12, Dragonhead Shield 12) and drew its own art (the dust, the burst, the fire), but cast 1 (Warrior's Round
  Shield, its art 60 without a cost) spent 5, the Dagger's Quickstep (art 100): that cast audited the Dagger's art, the shield's
  own only in cast 2 (INFERRED from the costs, the final review's M4).
- **The catalysts and torches** (15): FP alike (5 spent none in both: Saint's Talisman, Sunlight Talisman, Talisman, Parting Flame,
  Torch); all within 10 %; read (10 casts): the staffs' blue light, Storyteller's purple mist, Murky Longstaff's purple glow,
  Preacher's Right Arm's crystal burst, the White Hair Talisman's and the Pyromancy Flame's bursts, the Sacred Chime's golden
  light, the Follower Torch's fire breath: alike.
- **The bows and crossbows** (10): the eight bows spent no FP in either: held in one hand, a bow is swung as a club (strips:
  `crops/bows-4-9.png`), its art not reached (two hands needed, INFERRED); the Repeating Crossbow's art fired, alike.
- **The arrows and bolts** (19): every arrow and bolt equipped and read back, but no ammunition effect reached: the bows were
  swung as above (Fire Arrow's faint orange light on the floor in both), and the Light Crossbow's bolts showed no flight, flash or
  impact in the crop in either renderer (the crop cut around the character, 240x180 a frame: a bolt leaves it at once; a
  target in view, and a wider frame, needed: INFERRED). So the arrows, the bows' arts and the bolts are left, with
  two hands and a target.
- **The Stance arts** (17, Shift and the right button held, the left clicked 800 ms after it): 14 of the 16 judged spent FP in both
  (tranche 16's plain click: none), Farron Greatsword and Splitleaf Greatsword none in either; 12 within 10 % on the peak (five
  beyond: Farron +54 %, Rapier +10.5 %, Frayed Blade +13 %, Crucifix +27 %, Drang Twinspears +31 %; Friede's mean +24 %, its
  peak within); read: Lothric's Holy Sword's star glare, the Twin Princes' fire, Storm Ruler, Friede's ice, the Rapier, the Drang
  Twinspears (the sparks and the camera a phase apart; `crops/stance-11-17.png`, made after the final review): alike. Farron
  Greatsword and Splitleaf Greatsword spent no FP in either renderer even so: their arts not shown used (left). Farron Greatsword's and the
  Frayed Blade's hit on the pillar flashes larger and brighter natively (the effects' light, INFERRED: R's, as Moonlight
  Vortex's green ground). **Seen differing: the Crucifix of the Mad King** (cast 15): after the stance's attack the original draws
  a violet swirl around the character's feet and hand for about 2 s; natively none, the red sparks and the grey blur alike
  (`crops/crucifix-15-native-top.png`, native above). A missing effect natively: a lead for the next tranche.
- **The goods, first pass** (`i140-t17`, `build-t17d`): every `Q` word refused (`equipped` false, `earlier` -2): the tool tested
  the category on `140A323E0`'s value, which is the inventory entry's handle (0xB0xxxxxx for a goods), so the uses took
  whatever the quick slot held; not read. Fixed (`60be6dff4`: the item value's nibble from `14058C950`, READ), `build-t17e`.
- **The goods, second pass** (`i140-t17b`, `build-t17e`, 21:43-22:25, `t17/audit2.log`): four runs, exit 0, no fault, restored;
  every Q word put its goods in slot 0, selected (30 and 18 rows, `equipped` true). `goods-use` (29): 29 of 29 within 10 % on the
  screen; 11 read (`crops/goods-use-*.png` and Homeward Bone's film), the other 18 by the screen alone: the Charcoal Pine Bundle's fire on the Dagger, the Rotten Pine Resin
  and Carthus Rouge, Divine Blessing's gold, Green Blossom's green, the Rime-blue Moss Clump, the Black Bug Pellet's ring and
  violet aura, the Rusted Gold Coin, the Soul of a Great Champion's flash, Ember's flames and « EMBER RESTORED », Homeward Bone's
  prompt, warp and arrival at Firelink Shrine: alike. **`goods-thrown` not judged**: the copy already held 12 of its goods, some at
  their maximum, so the three given of each went partly to a bag on the ground, which the give's later E presses opened: its
  list stayed over the picture for the whole run in both renderers (the uses still happened: the quick slot's counts fell). Its
  gives are now only the 5 goods the copy lacks, one each (`goods-use`'s 2 likewise) (the slot's inventory read: entries {0xB0xxxxxx, 0x40000000 | id,
  count}). The series could not be stopped between its runs from outside (the stop file twice missed its window), so
  `goods-use` ran with its earlier gives; its runs were not spoiled (no bag).
- **The thrown goods, third pass** (`i140-t17c`, `build-t17f` = `cdd7c3538`'s module but for two comments, 22:47-23:03,
  `t17/audit3.log`): two runs, exit 0, no fault, restored; 18 F1 words applied, no bag (the 5 gives). 16 of 17 within 10 % on the
  screen; read (`crops/goods-thrown-*.png`, 160x120 a frame, and `crops/rope-firebombs-4-5.png`, 320x200 every 200 ms): the
  knives, the Lightning Urn, the Stalk Dung Pie, the Alluring Skull, the Prism Stone and the dragon stones (no form seen in
  either) alike. **Seen differing, not read: the rope firebombs' blast** (casts 4 and 5, Rope Black Firebomb 14 % below the
  original's peak): natively the brown smoke covers the flames from about 0.4 s after the blast and the fire looks shorter; in
  the original the flames stay bright over the smoke a frame or two longer (a phase of 200 ms not excluded; the smoke's opacity
  or the order of the smoke and the fire natively: INFERRED, a lead).

## Tranche 18: the three leads of tranche 17, each moment captured in both renderers

Branch `claude/e-i140-t18` (from `main` `efcd82590`). Families `lead-crucifix`, `lead-havel`, `lead-rope` (`arts-t17.txt`): each
moment cast again, natively and in the original, with F9 pressed at the effect (`capture <ms>`, `Run-AlphaCast -CaptureAt`): the
effects' draws listed by `ffx_ps_image` in both renderers (blend, colour scale, fog, soft) and natively by `native_ffx_draw_dump`
(`-NativeFlags ffx-record`: textures by FFX id, programmes).

- **A defect of tranche 17 in `main`** (found here, 2026-11-16 23:45): the fourth step review's L3 fix put a `#` comment in the
  middle of `Run-I140Spells.ps1`'s line 81, which swallowed its end (`}; $q.pending+=$w; continue }`): the script no longer
  parsed (every family refused at load). Fixed here (the comment moved to the line's end; the three scripts parse with no error).
  The merged review did not run the script after that fix.
- **The runs** (`i140-t18` natively with `ffx-record`, 2026-11-16 23:55 to 2026-11-17 00:12, `t18/audit1.log`; the original's
  three again with `-ReferenceFlags ffx-draw-observe`, `i140-t18b`, 00:12-00:20, `t18/audit2.log`: the first pass's original runs
  logged no F9 rows, their slot-27 hook being behind that flag in the original, 22_setup.inc; the runner gains `-ReferenceFlags`
  and a `-NotBefore` after midnight, which the first pass read as past and started 6 minutes before its announced time); `build-t17h`;
  the ASUS window, offline, the stats-99 copy restored by hash: nine runs, exit 0, no fault, every word applied. The F9 rows grouped
  by kind (blend, colour scale, colour type, fog, pre-alpha, frame copy, two textures, soft) in `t18/f9-kinds.txt`; strips 320x240
  a frame every 400 ms in `t18/crops/` (native above).
- **Havel's Greatshield: not shown differing.** On the art's second use both original runs of this tranche draw the dust burst
  too (`crops/havel-2-native-top.png`, the `i140-t18b` original below; the final review's 200 ms strips of all three runs,
  `crops/havel-200ms-final-review.png`); the F9 rows hold a kind both draw (blend 1, colour scale 0.3, alpha 0.8: 32 draws natively,
  80 in the original, the dust's INFERRED: the rows carry no effect id; per frame about 12 and 38, a phase not excluded). Tranche
  17's original run did not draw it once (why: not read); the lead is closed.
- **The Rope Black Firebomb: not shown differing.** Its blast, fire and smoke in phase at 100 ms a frame in the native run and both
  original runs (`crops/rope-g302-100ms-final-review.png`, the final review's strip; mine at 400 ms, `crops/rope-native-top.png`,
  could not settle tranche 17's 200 ms reading); the crop's luma peak 99 natively against 87 and 113 in the two original runs (the
  integrity review, YAVG every 50 ms): the original's spread covers the native. The Rope Firebomb (g299), tranche 17's other
  cast, was not run again: left open.
- **The Crucifix of the Mad King: a lead, not confirmed.** The `i140-t18b` original draws the violet swirl around the feet and the
  hand after the stance's attack; the native none, a pale edge glow instead (`crops/crucifix-native-top.png`). But the first
  original run of this tranche (`i140-t18`, without F9 rows) draws no swirl either (the integrity review's strip,
  `crops/crucifix-first-original-no-swirl-integrity-review.png`): the original drew it in 2 of its 3 runs (tranches 17 and 18), the
  native in neither of its 2 (OBSERVED). The F9 rows: the `i140-t18b` original draws eight kinds the native never draws in its
  300 (22 draws: blend 1 with colour scale 0 and alpha 1 or 3.8, some with two textures; blend 1 with 0.65 and alpha 1, pre-alpha
  0; blend 2 with alphas 0.05, 2.5, 5 and 7.5; a frame-copy draw with two textures and soft 0.6), and the native draws 71 of blend 2,
  colour scale 0.65, alpha 1 where the original draws 7 (other captures 4 to 6): a native surplus beside its edge glow. The F9 press
  lands 0.5 to 1 s after its asked time, unrecorded, so the lists are not aligned in time. Three readings stay open (INFERRED): the
  swirl's emitters not spawned natively, dropped before the draw, or drawn with other parameters (the surplus); and the original's
  own swirl intermittent (what decides it, not read). The native draw dump of that frame (`native_ffx_draw_dump`, 192 rows at
  frame 4796, its programmes beside the journal) is not read yet.
- The shots of a captured cast: a stance's 400 to 1,600 ms shots skipped (late), another cast's taken after F9 under their nominal
  names (`cast-2-400.png` of Havel shows about 2.3 s): read the films, not those shots. The colour scales in `f9-kinds.txt` are
  rounded to 0.1, which splits a drifting kind in two (the rope's 8.4 and 8.5); its script is not kept. The module: `build-t17h`
  (`cdd7c3538`'s code, module E6D6A749…). The first pass started 6 minutes before its announced time, the slot already given by the
  coordinator (said in the thread at once). In every strip the character is near-black natively after the effect (the lighting's,
  R's, INFERRED).
- **Step review** (fresh Opus, high; `efcd82590..db776daa7`, the scripts): no High. M1, `-NotBefore` moved a time only forward:
  now the HH:mm nearest the clock, within 12 h either side. M2, « capture » claimed F9 at its time, which lands 0.5 to 1 s later,
  unrecorded: said in the comment and above (the Crucifix's F9 reading weakened to a pointer). L1, a capture without its flags
  recorded nothing: refused. L2 (a captured cast's shots taken late: said in the comment), L3 (flags checked only in the run),
  L4 (a one-cast family's « not judged » line): left.
- **Final review** (fresh Opus, xhigh; `main...fa31afbec`): no High. M1, the -NotBefore fix started at once a time more than 12 h
  ahead (yesterday's): a nearest time more than 10 minutes past is now refused, never started early. M2, a capture's flags checked
  only per run: checked with the lists, before any run. M3, stale tranche 17 sentences (STATE's open gaps, the
  Conclusion), no tranche 18 Evidence or Uncertainty: written. M4, « the rope firebombs » closed on one of the two and on 400 ms
  strips: the Rope Black Firebomb on the review's 100 ms strip, the Rope Firebomb left open. M5, the Crucifix's reading ignored the
  native surplus of one kind and the native draw dump already taken: both said, three readings kept. L1 to L8: the captured casts'
  shots, the eight kinds listed (and 0.05), Havel « not shown differing », the rounding, the build, the early start, the near-black
  character, the order of `after` and `capture` (said here: `capture` last).
- **Integrity review** (`main...fa31afbec`): no weakened check. High: the Crucifix « confirmed » while the first original run of
  this tranche drew no swirl either (`i140-t18-lead-crucifix-orig`): now a lead, the original 2 of 3, the native 0 of 2. Medium:
  the rope closed on 400 ms strips (above). Low: the eight kinds, the dust's kind INFERRED, the crops' runs named, the parse
  check's log (the PowerShell parser, 0 errors, by both reviews).
- **Their fixes' step review** (`fa31afbec..0bdcfa2a2`): no High. M1, the checks ran after the -NotBefore wait (a bad call
  refused only when its slot came): the wait now follows the lists' and the captures' checks. L1, the per-run capture refusal left
  unreachable: removed. L2, the refusal's message: « more than 10 minutes past (or more than 12 h ahead) ». L3, a capture family
  without its flags now refuses the whole series: said in the comment.
- **The merge** (2026-11-17): `main` unchanged since `efcd82590`; `Test-Project -MergeGate` passed on `41e3dbb1c`; merged
  `b7df7840a`; the launcher rebuilt (`evidence/I-0140/build-launcher-b7df7840a.log`, 178 PASS lines); start check
  `start-b7df7840a` passed, restored (00:51). No disk rotation: run an hour after the last.

## Tranche 19: step 5's left list (the Crucifix's swirl, the Rope Firebomb, Farron and Splitleaf, the maggots, a bow in two hands, the Fire Keeper as a target)

Branch `claude/e-i140-t19` (from `main` `a50d08053`).

- **The Crucifix of the Mad King, read** (the pilot, 2026-11-17 00:55-01:05, READ in the params with the DS3 paramdef of DSMapStudio
  1.11.1's Paramdex, offsets checked against `e84/weapon-param.tsv`'s +0x194, the art's id, = 1409): EquipParamWeapon 9260000 has no hit or
  resident SpEffect, behaviour variation 3814 and art 1409 (type 34, 17 FP on the strong attack only). BehaviorParam_PC 103814740
  makes bullet 100092640 (5 s, following the owner, FFX 442331, limit group 201, its hit bullet 100092641: 0.8 s, FFX 442332, whose
  hit bullet 100092642 carries FFX 442333, radius 3); 103814745 makes 100092643 (0.03 s, the same limit group 201); 103814720 makes
  100092620 (seven shots, FFX 442312 and 442313, which no bank holds). The three FFX are in `frpg_sfxbnd_dlc2_effect` only; their
  textures (`Read-Fxr.ps1`): 442331 11657, 50065, 2012, 1001, 11636 and 11664, 11627 and 11664 (two-texture billboards), 3002 and
  11627 LocalYaw (flat on the ground), 11414, and a RadialBlur; 442332 11401, 11403, 11414, 2012, a RadialBlur; 442333 3002 and
  11627 LocalYaw, 11619, 11630, 12002, 11403, 1000, 1001, a two-texture billboard, a RadialBlur. So the violet swirl about the feet
  is these effects' LocalYaw billboards (INFERRED from the orientation and the colour). What triggers 740 and 745 inside the stance
  (the animation's events) is not read.
- **Tranche 18's native draw dump, read**: `native_ffx_draw_dump` names an FFX texture as id x 4 + kind; at the F9 frame (4796) the
  native drew, beyond the frames 1800 and 2400, 36 draws of texture 11633 (`ps 2AFA`) and 34 of 11401 (`ps 2B14`, colour scale 1),
  both common to many effects (11633 in 496 FFX files, 11401 in 77); none of 442331's or 442333's own textures. The dlc2 bank was
  loaded natively (its records 442331 to 442333 retired at the end, `native_sfx_cpu_record_retired`), no draw skipped and no
  texture missing (`native_effect_stage`); the native's first radial blur draws of the run (`native_ffx_radial_draw` n 1 to 32,
  2D/5 196 in that window's `native_ffx_execute`) came after the cast, before F9. So at F9 the native drew 11401 (one of 442332's
  textures, never drawn at frames 1800 and 2400: the final review's M2) and none of 442331's or 442333's own textures; whether those
  had been drawn and ended (F9 lands 0.5 to 1 s late) was not settled by that run.
- **The Corpse-grubs' maggots, read**: AtkParam_Npc rows 1220020 to 1220160 (model c1220) and 2060010 to 2060100 (c2060) carry
  SpEffect 5100; 6090055 carries 5105. 5100: 80 s, its cycle 5101 every 0.06 s (bleed buildup 1, stateInfo 6, the layout of 5106 and 22019: the final review's L2), stateInfo 261, vfx 261 = FFX 4050, 4051, 4053
  full body (commoneffects); 5105: 40 s, its cycle 5106 every 0.3 s (bleed 10), the same state, vfx 42900 = FFX 41000 to 41003 of
  the m45 bank (Ariandel). c2060 is placed five times in m35 (around -320, -211, -700: the graveyard), once in m39 (206100), seven
  times in m33_01 (`msb-enemies.tsv`): the Corpse-grub, INFERRED from Thomas's places. The fire's removal of the maggots: not read
  (stateInfo 261's handling in the game).
- **The tools** (`Run-I140Spells.ps1`, `arts-t17.txt`): families `lead-crucifix2`, `lead-rope2`, `arts-farron`, `bows2`, `maggots`,
  `keeper-infusions`, `keeper-status`, `lead-rope3`; a cast kind `grip` (E held with the left click: the two-hand toggle of the
  right weapon, OBSERVED below: the bow then drawn and loosed); `arts-t17.txt`'s family names may hold digits (the first launch,
  `t19/audit1.log`, refused `lead-crucifix2` at load: no game started); the keeper families walk to the Fire Keeper as
  DECALS-PAINT-READY did (`-HitKeeper -AimAt -LockOn`). F1 gains `n<id>` (`04_updates_and_test_tools.inc`, test tools only): the
  SpEffect add on the nearest other living character within 25 m (F6's walk of the character sets, nearest instead of strongest),
  its distance in the row.
- **The runs** (`i140-t19`, 2026-11-17 01:14-01:48, `build-t17h`, the ASUS window, offline, the stats-99 copy restored by hash,
  `t19/audit2.log`): ten runs, exit 0, no fault, every restoration ok, every F1 word applied in order. Strips in `t19/crops/`
  (native above unless named otherwise).
- **The Crucifix of the Mad King: the violet swirl can be drawn natively; the original's white ring not judged.** Six stances (800,
  800, 1600, 1600, 2500, 2500 ms before the attack), casts 1 to 5 spending 17 FP in both renderers (the art's R2 cost: the
  stance's left click, Shift held, takes the R2 slot; cast 6, the last, has no FP row). The violet light and particles at the hand and about the
  character after the attack, by the pilot's eye at 280x210 a frame: natively over the body clearly in cast 5 (77.2 to 79.6 s of
  its film), at the hand in cast 2, while casts 1, 4 and 6 show the violet staff head and the pale edge glow tranche 18 called
  « none »; in the original in casts 1, 2 and 6, a white ring at the stance's attack in 3 and 4 (`crucifix2-800-1600a.png`,
  `crucifix2-1600b-2500.png`, `crucifix2-native-only.png`). So the native can draw the swirl (OBSERVED), the original misses it
  too; how often each draws it is not measured (the integrity review's M2; no F9 in these runs, no texture-level evidence of
  442331's or 442333's own textures natively). The stance's length does not decide it (cast 5 is a 2500 ms stance, the
  original's cast 1 an 800 ms one). What does decide it is
  not read (a hit on the pillar, INFERRED). The white ring of the original's casts 3 and 4: natively the camera then faced a
  wall at a short range (a bright glow fills the frame at that moment), not judged: left, with a framing that shows the character. In every cast after the first the two
  characters face different ways (each run's drift from the same save), so a cast's framing differs between the renderers.
- **The Corpse-grubs' maggots (5100 by F1 on the player, then the Torch in the left hand): the status's logic alike, the worms'
  draw not judged.** The status's gauge (the HUD bar under the character, the bleed gauge as 5101's row reads; its length counted
  each second on the film, `t19/gauge.py` and `gauge.txt`, the integrity review's recount kept) grows from the add at the
  same rate in both (6 pixels a second, natively 4 to 180, in the original 2 to 178) and falls from the second the Torch is put
  in hand (natively from 47 s, in the original from 46 s, both 120 at the film's end): the Torch's resident SpEffect 401
  cycles 402 every 2 s, stateInfo 262 (READ), and the maggots' state 261 stops (INFERRED from the gauge). The worms themselves
  (FFX 4050 to 4053 carry FFX model nodes 605, models 4000 to 4002, READ in the FXR) are too small in this framing to judge in
  either renderer (`maggots-zoom-native-brightened-top.png`: white specks on the original's helmet, the native character too
  dark to say, dark curled shapes on its arms; the lighting's, R's): not judged, Thomas's eye or a closer camera. The gauge and
  its fall are game logic and interface, the same code in both renderers: only the worms' FFX is the renderer's, so the
  maggots stay open. The Ariandel maggots (5105, FFX of the m45
  bank) were not tried (no m45 bank at Firelink).
- **The Rope Firebomb (g299, twice): a lead.** The fire's pixels (R above 200, G above 110, B below 140, the whole window at
  320x180 every 50 ms from 0.6 s after the use): my first reading at 20 frames/s (its numbers not kept, and not reproduced by the
  integrity review's recount) is replaced by the final review's 60 frames/s tables (`t19/fire60.txt`, `fire60b.txt`): in cast 1
  the original swells (2,194 rising to 3,251) and the native does not (2,861, then falling); in cast 2 both swell (the original
  2,047 to 2,752, the native 845 to 1,270); the native's fire gone 33 to 67 ms sooner (OBSERVED; the bomb landed on the other side
  of the character natively, so a part of the fire may be hidden by the body: INFERRED). Tranche 17's reading of the same goods (the smoke over the fire sooner natively) agrees; the Rope
  Black Firebomb (g302) was alike in tranche 18. Recounted by the final review (below).
- **Farron Greatsword and Splitleaf Greatsword: alike as far as reached.** Farron's art (SwordArtsParam 401: no FP in any slot,
  type 22 as the Moonlight Greatsword's, which changes the attacks after it, READ) raised the sword in both, no effect in either
  (`farron-1-3-4.png`); no attack followed the raise, so only the raise is judged; tranche 17's « no FP spent » was its cost. Splitleaf's stance spent
  20 FP (its R2 cost) at 1600 ms in both (tranche 17's 800 ms click came before the stance was ready, INFERRED; the 2500 ms cast,
  the last, has no FP row), its leap, white arc, sparks and dust alike.
- **A bow in two hands (the Longbow): the grip works, the art not reached.** After `grip`, the shots drew and loosed the bow in
  both renderers (`bows2-zoom-nat123-orig123.png`, brightened; tranche 17's one-handed bows were swung): so E with the left
  click toggles two hands (OBSERVED). The bow's art (1901: 9 FP on R1 and R2, READ) spent no FP in either with the stance input;
  the arrows' flight and impact not compared (natively the camera faced a wall at a short range). Left.
- **Step review** (fresh Opus, high; `a50d08053..258ab791f`): H1, the keeper families relied on DECALS-PAINT-READY's walk, which
  never hit the Fire Keeper (Native2138-2141: she stands elsewhere at each load), and nothing checked a hit: F1's new `p` probe
  logs the nearest living character's HP, maximum and position, one before each swing and one before a last swing, so a hit is
  an HP drop between two probes; the comments no longer claim the aim. M1, `n` logged a found flag and a distance only, and took
  any character with HP above 0: it now takes F6's bounds (maximum 20 to 20,000, HP not above it) and logs the character's HP,
  maximum and position (no entity id is read; a character not drawn is not told apart, said in the comment). M2, the keeper-status
  comment named the Fire Keeper as the target: INFERRED now. L1 (a comment moved to the wrong line), L2 (the FP line for the new
  families), L4 (FFX 4050, 4051 and 4053, not 4052): fixed. L3, the walk duplicated: F6, `n` and `p` share `VisitTestCharacters`.
  The first build of the fixes failed (C2059: `near`, a `windows.h` macro; clangd had passed it), renamed; `build-t19c`.
- **The second series** (`i140-t19b`, 2026-11-17 02:06-02:33, `build-t19c` = `fb54fa7e0`'s module, a cache build without the
  tests; `-NativeFlags ffx-record -ReferenceFlags ffx-draw-observe`; the ASUS window, offline, the stats-99 copy restored by hash,
  `t19/audit3.log`): six runs, exit 0, no fault, every restoration ok, every F1 word taken in order (the P words listed as
  « not applied » by the runner's check: `build-t19c`'s p row carried neither `id` nor `applied`, the second step review's M1,
  fixed after the runs).
- **The statuses on an NPC (keeper-status): taken, not seen.** In both renderers the first `N` (poison 20019) went to the same
  character, 4.6 m away at (260.97, -55.17, 600.59), HP 307 of 307 (the Fire Keeper's place, INFERRED: the walk's end is 2 m
  from it), whose HP was 250 natively and 253 in the original at the next press (the poison working, OBSERVED in the rows); the
  next `N` (toxic 21019) took her again, and from the third the nearest was another character (HP 712, 13 m away), the first no
  longer living (the toxic's, INFERRED: no row follows the toxic on her); the same sequence in both. Neither film shows her: the camera faced away (strips
  `kstatus-*.png`), so the statuses' look on an NPC is not judged. The frost (24019) and first bleed (22019) rows went to the
  712-HP character, still 712 at the next row in both (no row follows the second bleed): only poison showed an effect in the rows (toxic's INFERRED from her absence).
- **The infused swings (keeper-infusions): no hit.** The P1 probes held the nearest character's HP unchanged between every pair
  in both renderers (307, then 273 or 712 when the nearest changed), while the player walked farther with each swing (4.6 to 12 m
  natively): no swing hit anyone, so the infusions' hits are not reached. A tool that turns the player toward the probed
  character (its yaw written, the offset not read) is the next step for both.
- **The Rope Firebomb (lead-rope3): a lead.** Recounted by the final review at 60 frames/s (`t19/fire60.py`, `fire60b.py` and
  their tables `fire60.txt`, `fire60b.txt`; the same thresholds): the original's late swell 0.25 to 0.45 s after the blast in 4
  of 4 casts, the native's in 2 of 4 (`i140-t19` cast 2 and `i140-t19b` cast 2), and the native's fire gone 33 to 67 ms sooner in
  3 of 4 (OBSERVED; my 20 frames/s reading of these casts, « split both ways », was wrong: the original swells in `i140-t19b`
  cast 2 too). Confounded by where the bomb fell and the camera (INFERRED); tranche 17's reading agrees. The F9 rows (300 in each renderer, the first 300 of the frame after the
  press, which lands 0.5 to 1 s after its asked time) hold the same kinds in both (blend, colour scale, alpha, fog, pre-alpha,
  frame copy, two textures, soft): 245 and 238 of the main kind, the others within a few draws, the fire's fading kinds a little
  further along natively (alpha 3.1 against 4.2, a phase; INFERRED). The native dump at that frame (`native_ffx_draw_dump`, frame
  5764, 114 draws) holds the fire's and smoke's draws in passes 0x29 and 0x31 (textures 11004, 11630, 11631, 11790, 11426, 11402,
  11627, 11626, 19004, 50065, none at frames 1800 and 2400).
- **Second step review** (fresh Opus, high; `7f0f7b7fa..fb54fa7e0`): no High. M1, the runner's word check needs `id` and `applied`
  in every row, which the p row lacked (every probe listed as not applied): written (`applied` true when a character was found).
  L1, the FP line told the reader to ignore bows2's art: bows2's cast 3 is judged by it now. L2, keeper-status's line comments
  named the Fire Keeper and a hit: « the nearest character ». L3, a drop between probes is a hit only for the same position and
  without a status taking HP meanwhile: said in both comments. L5, the p row's buffer: 320 bytes. L6, « p<any »: « p<id », the id
  only logged. L4 (F6 now logs a row, 0 looked, when the player's position cannot be read, and refuses a NaN distance): kept, said
  here.

## Tranche 43: the portrait's 2 px come with its collects after the game unlists it

Branch `claude/e-i140-t43` (from `main` `397381d97`, 2026-11-22, worktree `focused-shaw-ca3bb4`). Scripts and outputs in
`evidence/I-0140/t43/` (`body-shift.py` (its header and first line still name `portrait-shift.py` and a 3x2 grid; it uses 3x4),
`portrait-shift.py`, `kernel.py` and their `-out.txt`; `ghidra-portrait-path.md`, the static reading's report condensed;
`build-a` and its log, the test tool's module, a cache build from `t41/cache`; `run-a.log`).

- **The body without AA is aligned** (`body-shift.py`, t42's search on tranche 37's body crop): the native's Status body against
  the original at equal page moments (+12 s, +16 s) at (+0.25, -0.25) px in 14 of 16 shots against `i140-t38-thomas-orig` and (0,
  0 to -0.25) against `i140-t40-thomas-orig-aaoff`, without AA (`i140-t38-thomas-nat`); with the merged TAA (`i140-t41g`) at (0 to
  +0.25, -0.5 to -1.0) (MEASURED): the 2 px are the portrait's own; the TAA's 0.25 to 0.75 px vertical on the body is left open.
- **The original's camera path, read** (a `ghidra-research` subagent on the GUI, its report condensed in `ghidra-portrait-path.md`):
  the projection is a symmetric perspective from fov and aspect (14004B070), the portrait renders into a 512x256 target with a
  viewport at (0, 0), the Face/Prof camera an orbit around a fixed target built once (140D698C0, 140D6B8D0) (READ); its eye by a
  hand transcription (0.578, 1.627, -1.431) equals the native's logged pose (INFERRED). The half-pixel word +0x68 read as zero
  there is not reconciled with MENU-OFFSCREEN-MODELS (584.0f), nor the aspect fallback with the body's logged aspect 0; the page's
  own draw of the texture is not read.
- **The listing rule is already read** ([MENU-OFFSCREEN-MODELS](../I-0066-independent-native-startup/MENU-OFFSCREEN-MODELS.md),
  « 1169 »; the comment above `native_offscreen_armed`): Init_Play (D677D0) lists a view and the next step (D676D0) unlists it on
  its first call, the original drawing it during that listed frame; the native keeps a holder armed until its renderer's
  destructor (D66280), a departure kept for a first frame before the first pose or the textures. In `i140-t43a`'s journal the
  portrait is unlisted by D676D0 (caller 14055150) right after its first collect, the body only at the page's close; in `i140-t41g`
  the native collects the portrait twice more after that unlist, until its model's release (MEASURED, the journals). The
  original's 512x256 AA ran 8 times over 8 openings, the body's 9561 (`i140-t40-thomas-orig-aaoff`, MEASURED); its listing rows
  are not logged in the original's mode.
- **The witness** (`i140-t43a`, 12:43-12:50, tranche 41's conditions, `build-a` = `b4118b722` with a test tool,
  `menu-first-collect.flag`: every menu view kept at its first complete collect of a listing; exit 0, 0 fault, restoration ok;
  openings 1 to 7 page-confirmed, 8 not): the portrait's whole-crop offset from the original (0 to -0.5, 0 to +0.25) px in openings
  1 to 7 against both originals; with the merged code at kernel 8 (`i140-t41d`'s openings 3 and 7) -1.25 to -1.5, tranche 42's
  reverted build (`i140-t42a`) -1.75 to -2.5, without AA (`i140-t38-thomas-nat`) -1.75 to -2.0 (MEASURED, `portrait-shift-out.txt`).
  Its content is not closer: the whole crop's residual at best 1.56 to 2.41 against 1.28 to 1.58 without AA, the face region 6.3 to
  9.7 against 4.2 to 5.4, and its regions' shifts not one rigid shift (opening 2: r01 (-1.0, -0.75), r20 (-0.75, +0.75)). The
  first collect has no environment cube: its IBL rows `memo 7, bound 0` in every opening of `i140-t43a`, `i140-t41g` and
  `i140-t38-thomas-nat`, the later collects `term 3, bound 4` (MEASURED, the journals); the portrait's YAVG 47.6 against 48.0 to
  48.3 for the native's later collects and 48.2 to 48.5 for the originals (the final review's signalstats). Its face, one
  unjittered own sample, 1.13 to 1.16 of the original's DLAA; the body frozen at its first collect 1.42 to 1.45, opening 5's body
  0.78 (its first collect partial, `native_offscreen_view_partial` nonfinite 1, so not in the tool's `drawn`) (`kernel-a-out.txt`).
  Between openings the frozen portrait still moves 0 to 0.5 px.
- **Reading**: the 2 px come with the native's collects after the game unlists the view (MEASURED by the test); the model's pose
  rather than the camera (INFERRED: the logged camera pose is constant over the portrait's first three collects, `i140-t41g`,
  `i140-t42a`, one opening by the rows' cap, 4 digits).
- **Outcome**: the test tool reverted (`7500d9963`, its module kept in `build-a`); no code merged. The fix is the next tranche's:
  the original's rule, a view drawn while it is listed, with the native policy's reasons as named conditions (a first collect
  before the first pose, textures not resident, the region memo and so the IBL cube unknown at the first collect, as measured
  here); if the frozen view is then resolved over several frames, its pose and skin data must outlive the model's release by
  D66280 (about five frames after the unlist) and its face is expected back near 0.67 of DLAA (tranches 41 and 42), the single
  sample's 1.14 being aliasing. The character creation's views and the Load Game portraits are witnessed with it.
- **Reviews at the merge** (fresh, `main...7949bd56a`): final, H1 (the fix not following from the evidence: the listing rule
  already read, the first collect without the cube, the lifetime past D66280, the face expected back near 0.67) and M1 to M3 (a
  residual from another script, `i140-t42a` named the merged code, MEASURED on the pose) answered by this section's rewrite; L1 to
  L8 (the static reading's limits and report, opening 5, the stale candidates, STATE's row, the body's vertical under the TAA, an
  unsourced 100 ms, the cache build, wording) answered above. Integrity, no High; its M1, M2 and L1 to L3 the same, answered. The
  revert exact, no net code.

## Tranche 42: the briefly drawn views' even supersample, tried and reverted

Branch `claude/e-i140-t42` (from `main` `f97392054`, 2026-11-22, worktree `focused-shaw-ca3bb4`). Scripts and outputs in
`evidence/I-0140/t42/` (`build-a` and its log, `run-a.log`, `rec-a.log`, five films `film-status*.mkv`, `kernel-a-out.txt` by
tranche 41's `kernel.py`, `portrait-shift.py` with `portrait-shift-out.txt`).

- **The idea** (tranche 41's « Left »: the portrait resolved about six times per opening, then kept): after a view's own reset, its
  next seven frames add their own sample with the history's weight n / (n + 1), clipped to the 3x3's min-max box alone, so a view
  drawn for a few frames holds the even average of its phases (a supersample about the pixel's centre) instead of the
  reconstruction's first blends. The code (`f18db3c76`: `TaaParams::warm`, the shader's warm branch, `MenuTaa::cycle`, test T16)
  built with its tests (`build-a`).
- **The witness** (`i140-t42a`, 12:02-12:08, tranche 41's conditions, `build-a`; exit 0, 0 fault, restoration ok; openings 1 to 5
  page-confirmed; opening 6's page row not seen in 15 s, its shots not for a comparison, and the series stopped there, tranche
  40's flake; `run-a.log`). The warm rows (`I-0066/artifacts/i140-t42a-thomas-nat-taa/startup.jsonl`, capped at 56 rows by the
  code) show the body warm 1 to 7 and the portrait 1 to 5 in openings 1 to 4, both to 5 in opening 5 (the cap) (MEASURED). On
  openings 1 to 5: the face 0.640 to 0.719 of the original's DLAA (mean 0.671) against tranche 41's witnesses at kernel 8,
  `i140-t41g` 0.618 to 0.673 (mean 0.644) and `i140-t41f` (mean 0.666): the gain about the spread between runs; the body 0.764 to
  0.808 (mean 0.779, `i140-t41g` 0.783); the portrait's whole-crop offset from the original -1.75 to -2.5 px against tranche 41's
  kernel-8 openings of the calibration run `i140-t41d` (-1.50 and -1.25; its other openings at kernels 2.29, 6 and 12), the native
  without AA -1.75 to -2.0: further from the original, not closer (MEASURED, `kernel-a-out.txt`, `portrait-shift-out.txt`; the
  script is tranche 41's with `NAT` set to `i140-t42a`, its first line still naming `t41`). The five films were not analysed.
- **The opening-to-opening wobble** (MEASURED, the journal and `portrait-shift-out.txt`): openings 2 and 5 drew the same jitter
  phases and differ by 0.25 px, openings 1 and 3 have the same mean phase and differ by 0.25 px, and the spread (0.75 px) did not
  shrink under phase-centred averages: it is not the jitter's phases (INFERRED; tranche 41's reading weakened), or the search's
  0.25 px grid is its floor. Open with the 2 px.
- **Reading** (INFERRED): an even supersample over the phases is still a box about one pixel wide, displayed upscaled; the face's
  distance to the DLAA is what this AA is, not an unconverged history, and the 2 px offset does not come from the TAA.
- **Its step review** (fresh Opus, high, `f97392054..f18db3c76`): no High; M1 (a repeated presentation inside the cycle blends the
  same phase again and pulls the history to it), M2 (the reactive and fade branches override the warm weight and sample), M3 (T16
  does not tell the min-max box from the variance clip), L1 to L5 (one warm value tested, a test that cannot fail, the row's
  requested warm, the header's wording, the cycle length in three places). M1 and M2 did not act in the witness (`repeats` 0 in
  every row; the menu views have no reactive or fade mask, `RenderNativeOffscreen` sets no `base`), so it measured the path as
  designed.
- **Outcome**: reverted (`9c2bcd26c`): a gain inside the noise for a new path and three Medium findings; the review's findings fall
  with the code. The menu views' TAA stays tranche 41's (D-0306).
- **Reviews at the merge** (fresh, `main...27c79797c`): final, no High; M1 (opening 6 not page-confirmed used as valid, the warm
  rows' cap) and M2 (the Conclusion stale) fixed above; L1 to L9 (the witness's time, STATE's time and run, tranche 41's offsets
  from the calibration run, the wobble, the review's Mediums inactive in the witness, the script's first line, the index row, the
  films) fixed above or in STATE and the index. Integrity, no High or Medium; its L1 to L3 (the offsets' baseline, opening 6, the
  time) the same, fixed. The revert exact, no net code.

## Tranche 41: the menu views' TAA, a tighter reconstruction (D-0306); the dark bin the cloth's pose, the portrait still once shown

Branch `claude/e-i140-t41` (from `main` `91450e28d`, 2026-11-22, worktree `focused-shaw-ca3bb4`). Thomas (09:02 Paris, after his
option 2): « Fais ce qui te semble le plus juste concernant le TAA ». Scripts and outputs in `evidence/I-0140/t41/`
(`series-t41.ps1`, tranche 40's `series-t40b.ps1` on this worktree, `-Kernels` writing `menu-taa-kernel.txt` into the build folder;
`kernel.py`, `film.py`, `film-lag.py`, `record-body.ps1`, `darkbin.py`, `portrait-shift.py` and their `-out.txt`, `sim.py` (its
0.56 printed when run, no output kept); `run-a.log` to `run-c.log`; `build-a.log` to `build-f.log`:
`build-a` refused T15's printf (a lost backslash), `build-b` built while a source was edited (discarded), `build-c` the first run's
module, `build-d` the second's, `build-e` the second review's fixes, `build-f` the witness's, `build-g` the final tree's).

- **The cause, offline** (a simulation of the static TAA, `sim.py`): the 3x3 Gaussian exp(-2.29 d^2) at the
  unjittered centre filters even an unjittered frame (0.10 on each side, 0.01 on the diagonals) and the jittered history averages
  the reconstruction over the eight phases: the simulated body at 0.56 of its no-AA gradient, the measured 0.55 (tranche 40).
- **The code** (`f5211c39b`, `b08bc2c70`, `ed8300f24`): TaaParams `kernel` (the Gaussian's coefficient; 2.29 the main view's,
  unchanged) and `own_reset` (a reset frame drawn unjittered outputs the pixel's own sample); the menu collect draws a view's reset
  frame unjittered: its first collect, a cut, or a collect after one not queued for drawing (`native_offscreen_queued`, the
  presentation's record of the serial it queued for each holder) or one that stopped before its TAA; the renderer resolves a
  repeated presentation of an own-sample frame again as one; the kernel chosen once per listing of a holder (`listed_kernels`), the
  constant `kMenuTaaKernel` 8 or, as a test tool, `menu-taa-kernel.txt`'s values in turn by view extent; the journal row
  `native_menu_taa_frames` carries `kernel` (-1 on DLAA frames) and `own`. T15 (taa_test): the own sample on an unjittered reset, none
  on a jittered one, a non-reset frame unchanged, kernel 8 its own reconstruction with steeper edges.
- **Calibration 1** (`i140-t41c`, 2026-11-22 10:21-10:24 Paris, the coordinator's slot, the ASUS window 1920x1080, offline, the copy
  of Thomas's character restored by hash, `build-c`; exit 0, 0 fault, restoration ok; kernels 2.29, 3, 4, 5, 6 by opening; the run
  stopped after opening 5, its page row not seen, tranche 40's flake). The reset rows: `own` true and jitter (0, 0) on the body's
  560x768 and the portrait's 512x256 (MEASURED). `kernel.py` (tranche 38's crops and mask, against the original's DLAA
  `i140-t38-thomas-orig` at equal page moments): the body 0.711, 0.735, 0.761, 0.758, 0.785 (the last opening's shots not
  page-confirmed); the face 0.62 to 0.65 whatever the kernel, the own reset included (tranche 40's 0.61 without it, across runs);
  the panel 0.999 (MEASURED). The portrait is not one frame (below, the witness): its reset is followed by about five jittered
  frames before it is kept, so the own sample weighs about 0.9^5, 0.59, of the picture before the clip (arithmetic on the feedback;
  the clip's share not computed).
- **Calibration 2** (`i140-t41d`, 10:31-10:37, the same conditions, `build-d`; exit 0, 0 fault, restoration ok; kernels 2.29, 6,
  8, 12 twice; every reset row `own` true, jitter (0, 0)). The body against DLAA: 2.29 0.720 and 0.723; 6 0.787 and 0.781; 8 0.789
  and 0.775; 12 0.773 and 0.801; the face 0.590 and 0.628; 0.623 and 0.623; 0.643 and 0.691; 0.679 and 0.679 (MEASURED). The films
  (`record-body.ps1`: ffmpeg gdigrab of a 640x720 region around the body, lossless, 60 frames/s, 1.8 s from 10 s after each page
  row; `film.py`, `film-lag.py`): the body's mean frame-to-frame change 0.275 levels at 2.29, 0.34 to 0.38 at 6 to 12, growing
  almost linearly with the lag (1.85 to 2.30 levels at 8 frames), so mostly the body's own motion (its idle, the cloth; INFERRED); per unit of
  gradient the lag-1 change is 17 to 22 % above 2.29's at 8 (both on odd openings, the self-shadow blurred as in play), about as much
  at 6 and 12 (even openings, filmed with the test toggle's hard self-shadow), the lag-8 change 0 to 16 % (MEASURED): a small added
  shimmer from the jitter (INFERRED), not compared with the original's DLAA (no film of it).
- **The choice** (D-0306): kernel 8, where the body's gain stops and the portrait's nearly; the body at about 0.78 of DLAA
  instead of 0.72. The rest of the gap (about 0.2) is not the filter's: the accumulation's supersampling against DLAA's own
  sharpness (INFERRED); a menu-own sharpening was Thomas's « no » (option 1); his slider applies. The kernel applies to every tabled
  menu view (the creation's, Face Detail's, the equipment's): only the Status body and portrait were measured. The gradient ratio
  rewards a tighter filter even where it adds aliasing; most of the gain survives the films' averaging over time (their mean frame's
  gradient +6 % against the stills' +8 %, MEASURED), but no picture sheet was made: a metric, not a visual claim.
- **The witness** (`i140-t41f`, 11:07-11:12, the same conditions, `build-f` (`801ff2b50`'s tree, before the final review's uid key), no kernel file; exit 0, 0 fault,
  restoration ok; `kernel-f-out.txt`): every reset row kernel 8, `own` true, jitter (0, 0), one per view and opening; the body
  0.778 to 0.794 of DLAA (mean 0.785), the face 0.606 to 0.699 (mean 0.666), the panel 0.999 (MEASURED). The films of the portrait
  (`record-body.ps1 -X 720 -Y 135 -Size 1170x720 -Name filmf`; `film.py` with `FILM=1170x720:40,40,420,270`, the portrait's crop
  420x270 at (760, 175) of the client, `film-f-portrait-out.txt`): over 1.8 s from 10 s after the page row, no change at all frame
  to frame on the portrait's lit pixels in the eight openings (MEASURED): the picture
  shown is not a TAA output moving with the jitter. The rows' counter (`native_menu_taa_frames`, all menu views' resolves) grows by
  five more than the frame serials after each opening's portrait reset (`i140-t41f`: 1124 at serial 6907, 1200 at 6978, the same at
  every opening; `i140-t41d` alike), then by the body's alone (MEASURED): those five the portrait's (INFERRED from the reset's extent
  and the rows' view count), so the portrait resolved about six times per opening (its reset and five jittered frames) and then
  kept, as the original's AA call is made once (tranche 39). So tranche 40's « one frame per opening » was
  wrong, and the own reset covers only the first of the six. A briefly drawn view's picture depends on its opening's jitter phases
  (INFERRED from the per-opening offsets below): what such a view should take (unjittered own samples until a full cycle, say) is
  the next tranche's. No run of this tranche used DLAA (every reset row `dlaa` false) and no repeated presentation ran (`repeats`
  0): the DLAA's unjittered reset input and the repeated own path are read in the code only.
- **The dark bin's half not the DLAA's** (« Next » 11; `darkbin.py` over tranche 40's native and AA-off original switch shots of
  the first two openings): the [20, 40) bin's native / original-AA-off gradient 1.115 overall; over its pixels whose local level (a
  9x9 mean) matches within 5 % the ratio is 1.015, and the other 8 % (the cape's lower edge placed elsewhere, the cloth's folds, by
  40x40 block) hold 91 % of the excess (within 10 %: 1.018 and 86 %; within 20 %: 1.053 and 55 %) (MEASURED; `darkbin-zoom.png`).
  Sorting by local level sets aside any difference that changes the local mean, pose or shading alike: that it is the cloth's pose
  is the pilot's reading of the zoom (INFERRED); the item stays open until a comparison at the same pose.
- **The portrait's 2 px** (« Next » 11): natively the portrait's first collect comes 140 ms after the Status page row (tranche 40's
  `i140-t40k` IBL rows: collects at +140, +156, +203 ms), the original's portrait AA call at +125 ms (`i140-t40-thomas-orig-aaoff`'s
  512x256 `reference_aa_filter` row) (MEASURED). `portrait-shift.py` (a `measure-tools` subagent, tranche 38's `compare.py` search on
  its six regions and the whole crop): in every run of both modes (`i140-t41d`, `i140-t38-thomas-nat`, `i140-t38-thomas-orig`,
  `i140-t40-thomas-orig-aaoff`) the portrait's crop is the same to the pixel at +12 s and +16 s; against the original's opening 1 and
  its same opening the native's shifts are nearly the same (the original's openings not compared among themselves); tranche 38's
  native without AA at (-1.75 to -2.0, +0.5 to +0.75) px against the original (the whole crop), this run's TAA from (-1.25, +0.25)
  to (-2.25, +0.75) by opening on the whole crop (the face region to -2.5) (MEASURED). So the picture does not move once shown in
  either mode (natively: resolved six times, then kept, above), and our TAA moves it by up to 0.75 px from one opening to the next. That a frame's lead of the
  head's idle at the first draw makes the 2 px stays a candidate (INFERRED, not tested), and so does the camera or the target.
- **The final code's witness** (`i140-t41g`, 11:36-11:42, the same conditions, `build-g`, `8c8074b0e`'s tree, the merged code;
  exit 0, 0 fault, restoration ok; `kernel-g-out.txt`): every reset row kernel 8, `own` true, jitter (0, 0); `resets` 16 at the
  run's end, two per opening (MEASURED), so the uid key makes no spurious reset; the body 0.769 to 0.796 of DLAA (mean 0.783), the
  face 0.618 to 0.673 (mean 0.644).
- **Step reviews** (fresh Opus, high). The first (`91450e28d..f5211c39b`): no High; M1 (a collect published but never drawn left the
  next collect jittered while the renderer restarted the history) fixed: `native_offscreen_queued`; M2 (a repeated presentation of
  an own-sample frame resolved as a reconstruction) fixed: `last_own`; L1 (comments claiming a measured tighter constant while it
  was 2.29, D-0306 unrecorded) fixed by the measure and D-0306; L2 (DLAA rows reporting a kernel) fixed: -1; L3 (the calibration's
  counter outside the collect's lock review) fixed: in the collect, allow-listed; L4 (T15's non-reset case tests Record's gate, not
  the shader's) left: the gate is in Record by design; L5 (the main view bit for bit not shown: the cbuffer's 2.29 against the
  literal, the DXBC not compared) left: the same arithmetic, said here. The second (`f5211c39b..b08bc2c70`): no High; M1 (a collect
  stopped at the camera or Busy kept the previous published serial, so the next was jittered) fixed: the serial taken and cleared
  at the loop's start; L1 (the renderer's own resets: DLAA refused, the switch from DLAA, a new history target, a gap the collect
  could not see, start from a jittered frame) left: one frame, then the history converges, said in the code; L2 (the comments'
  claim) fixed with the constant; L3 (the kernel chosen again on a model rebinding or a TAA off within one listing) fixed:
  `listed_kernels`, erased only once unlisted. The third (`b08bc2c70..ed8300f24`): no High, no Medium; L1 to L4 (comments stating an
  inference as measured, the reset's « after a cut or a gap » too broad, « one frame » for a history that fades, the switch to DLAA
  left out) fixed in the comments; L5 (the published serial's note, `motion_seen`'s allow-list note) fixed. The fourth, on the final
  review's code fixes (`6b1d0100c..8c8074b0e`): no High, no Medium; L1 (a refused coefficient of the test file silently shifts the
  rotation) left: the rows log the kernel each opening took; L2 (the cap's reason ignores the perceptual weight, which at 30 still
  floors a far phase whose 3x3 is brighter than about 34 exposed) left: a test tool's bound, the default 8 far from it; its note (a
  presentation copying a scene before the next collect reads the queue resets a view that is drawn) left: an extra unjittered own
  frame, never a skipped reset, as before the change.
- **Final review** (fresh Opus, xhigh; `main...801ff2b50`): no High. M1 (the portrait's frame count is in the rows, the records
  said one frame or not counted, tranche 40's premise left standing) fixed in the records: six resolves per opening, tranche 40's
  lines corrected, the briefly drawn views the next tranche's. M2 (the dark bin closed on an inference) fixed: INFERRED, left open.
  L1 (the queued check keyed on a serial that can repeat) fixed: the scene's uid (`8c8074b0e`). L2 (the test file accepting
  coefficients the resolve cannot weigh) fixed: at most 30. L3 (records: « two runs », the films' run, IN-FORCE's unqualified
  reset, an unmarked inference, the portrait's ranges, `sim.py`'s output, « Next » dropping candidates) fixed. L4 (no DLAA run, the
  repeated own path never ran) said above. L5 (a comment) fixed. Built with its tests: `build-g` (186 PASS).
- **Integrity review** (fresh, `main...a0b128cab`): no High. M1 (the witness said « the final tree » on `build-f`, the uid key not
  run in play) fixed: `build-f` named as such, the final code's witness `i140-t41g` run. L1 (four of the eight portrait films
  analysed, the crop unrecorded) fixed: all eight, the crop in the output's first line. L2 (the extra resolves' attribution to the
  portrait) fixed: INFERRED. L3 (the films of kernels 6 and 12 with the test toggle's hard self-shadow) said above. L4 (the kernel
  measured on one page, applied to every tabled view) said above. L5 (the gradient metric rewards aliasing too) said above. Its
  checks held: no test weakened (186 PASS in every build, T1 to T14 unchanged), no special case, the numbers recomputed.

## Tranche 40: the DLAA makes three quarters of the body's gap; the menu views' own TAA (D-0305)

Branch `claude/e-i140-t40` (from `main` `26686e9e5`, 2026-11-22, worktree `jolly-franklin-66a5b2`). Thomas's D-0305: the menu views
take our anti-aliasing. Scripts and outputs in `evidence/I-0140/t40/` (`series-t40a.ps1` and `run-a.log`, `series-t40b.ps1` and
`run-b.log`; `aaoff.py`, `taa.py` and their `-out.txt`, `dlaa-out.txt` headed with their command; `build-a.log` to `build-k.log`,
the cache builds `build-b` to `build-k`: `build-a` refused a shadowed local (C4456, seen on the build's console, its log empty), `build-b`'s test a camera without a serial, `build-h` a
comment that swallowed a statement; `build-e` the first witness runs' module, `build-k` the second's (`f957d24f5`), `build-l` the merged code's).

- **The cause's direct test** (« Next » 11's first step; the run 2026-11-22 06:44-06:49, the coordinator's slot after R's, the ASUS
  window 1920x1080, offline, the copy of Thomas's character restored by hash, no measure; `t39/build-a`): the original alone with its
  Antialiasing option OFF, tranche 38's arguments otherwise (eight openings, the shots on the page's row clock), `aa-observe.flag`
  (`i140-t40-thomas-orig-aaoff`, exit 0, 0 fault, restoration ok). The rows (MEASURED, `startup.jsonl`): 1B9790 is still called on
  the same three targets (the world's 1920x1080, the body's 560x768 every frame of the visits, the portrait's 512x256 once per
  opening: 8 calls), Enable 1 and mode 3 as before, but the quality byte 0 and its gate false (keys `g0`, tranche 39's `g1`): the
  filter does not draw. Run-StartupTrace's flag pattern was edited in the worktree while this run was going (`no-menu-taa`, below):
  the script was already loaded; a departure from the rule, said here.
- **The shots at equal page moments** (`aaoff.py`, MEASURED; tranche 38's crops and mask: the body 560x620 at (1290, 190), the face
  220x200 at (950, 220), the pixels whose AA-on original gray > 20; the UI's panel as a control). Before the switch key (`-1`, `-2`)
  the three runs agree (no view shown: body 33.5, the panel 12.36 to 12.38 in every shot). On the 16 switch shots, the body's mean
  gradient is 12.74 natively (12.46 to 12.92), 9.70 in the original with its DLAA, 12.00 with it off (11.76 to 12.06): the DLAA
  makes 75 % of the gap (native / AA on 1.31, AA off / AA on 1.24, native / AA off 1.06). The face: 6.24 natively, 5.70 and 6.35:
  the DLAA makes all of it (native / AA off 0.98). By the AA-on original's local brightness (the body, the first two openings'
  switch shots): the bright bins' native / AA off 1.00 (from 90 up: the whole gap the DLAA's), the dark ones not: [0, 20) 1.05 to
  1.06, [20, 40) (230 000 px, the cloth) 1.10 to 1.13, [40, 60) 1.09 to 1.12, [60, 90) 1.05 to 1.06. So tranche 39's contradiction
  is real: about half the dark bin's +23 to +25 % is the DLAA's (AA off / on 1.11), the other half not (INFERRED from the measure:
  the cause of the rest not read; candidates: the native's own shading of the dark cloth, step 2's paint).
- **The menu views' TAA** (`7203d8409`, `a36ece7de`, D-0305): a tabled view's camera jitters in its own pixels when the option's
  byte reads AA on (TAA or DLSS on the world) and `no-menu-taa.flag` is absent (Halton (2, 3) over 8 phases, `upscaler::Jitter` of
  the selection's serial); its camera motion from its own previous camera (`upscaler::MenuViewMotion`, the world's velocity block's
  rule: the jittered current, the unjittered previous, the jitter folded, a cut by YB-4b.5's rule), no object motion; in
  RenderNativeOffscreen the velocity target cleared neutral (the resolve's camera path on every pixel), the resolve, then a TaaStage
  of the view's own (by its target: views of one extent share a context, not a history) into the context's output, which the
  composition tone-maps instead of the lit HDR (binding +2); the perceptual exposure 5 x the tone's scale (the tabled tone
  x / (x + 0.2) of x = c scale). The history restarts on a view's first frame, a cut, a frame it was not drawn, a scene drawn again
  with the last serial. Any failure latches the menu TAA off (`native_menu_taa_latched_off`), the view composed as before.
- **The menu views' DLAA** (`4e24448d8`): when the option runs DLSS on the world (the collect's `NativeDlssActive`), DLAA on the view
  in place of its TaaStage, on a Streamline viewport of its own (2 and up; 0 the world's, 1 NIS's), its own options (DLAA, its extent,
  DLSS's own exposure estimate, no Ray Reconstruction), the TAA's inputs in the TAA's states, the unjittered lens and its
  clip-to-previous-clip from the same history as the motion. A viewport is retired 600 frames before its resources are freed (a
  view gone, its inputs or the context's output re-created) and its number reused only then. A refused evaluation latches only the
  menu views' DLSS off (`native_menu_dlss_latched_off`; DlssReady and the world's DLSS untouched, `test-dlss-refuse.flag` counting
  the world's alone) and the frame resolves by the TAA (its history restarted). Thomas's sharpening on the DLAA output as on the
  world's DLSS (0 by default). The DLAA rides on the TAA's decision: a menu TAA latched off has no DLAA either (the step review's L6).
- **Step reviews** (fresh Opus, high). The TAA's (`26686e9e5..a36ece7de`): no High; M1 (an untabled view composed through the
  jittered depth's mask over a background the TAA never saw) fixed: the menu TAA on tabled views only, their tone read before the
  camera; M2 (`motion_seen` not reviewed by Test-ThreadAffinity) fixed; L1 (the frame number read before the list's Begin) fixed; L2 (a
  caught menu failure leaving the world's velocity objects half made) fixed: every object checked; L3 (a redundant depth barrier pair
  between the resolve and the TAA) left: the world's order, two barriers at 560x768; L4 (a scene drawn again with its serial reusing a
  stale motion) fixed: a reset; L5 (a closed menu's stages kept until the next menu frame or Close) left: bounded (two 560x768 RGBA16F
  histories a view). The DLAA's (`a36ece7de..4e24448d8`): no High; M1 (no sharpening on DLAA) fixed; M2 (a repeated serial evaluated
  again on the same frame token) fixed: the TAA then; L1 (the renderer's latch unread) fixed; L2 (the sweeps after the latch's return)
  fixed; L3 (a re-created output at the same address keeping its viewport) fixed: keyed by the context's creations; L4 (no free
  viewport silent) fixed: a row once; L5 (a stale comment) fixed; L6 noted above. Both answered in `294598ad7` and `0db898547`.
  The fixes' (`4e24448d8..0db898547`): no High; M1 (the sweeps only on a TAA frame) fixed: on every menu view's frame; M2 (the
  repeat keyed on the presentation serial, new each presentation) fixed: on the selection version; L1 (a creation count per context,
  reused after an eviction) fixed: renderer-wide; L2 (a viewport new to the view not resetting) fixed (`5a803f9a4`, `build-i`). Their
  review (`ebdadf5a0..5a803f9a4`): no High; M1 (a repeated version's TAA frame cleared the DLAA's mark, so the next version's DLAA
  reset each time) fixed: the mark kept on a repeat; L1, L2 (comments) fixed (`4d9328918`, `build-j`, tests passed). That fix's
  review (`f2511a8d9..4d9328918`): no High or Medium, criteria held; L1 (the mark's comment naming no history) fixed, comments only.
- **The witness runs** (2026-11-22 07:25-07:35, the coordinator's slot, the ASUS window 1920x1080, offline, the copy of Thomas's
  character restored by hash, no measure; `t40/build-e` from `294598ad7`, a cache build: the sharpening and L1-L5 of the DLAA review
  came after, in `build-f`, not run): tranche 38's native arguments, the option at its default (`i140-t40-thomas-nat-taa`), then
  `dlss-dlaa.flag` (`i140-t40-thomas-nat-dlaa`; the world in DLAA with Ray Reconstruction); 0 fault, restoration ok both. The first
  stopped after four openings (« a pass without its row », the runner's page-row wait), so 8 switch shots; the second ran eight. The
  rows (MEASURED, `startup.jsonl`): `native_dlss_option` `menu_taa` true; `native_menu_taa_frames` from the body's first frame, a reset
  at each opening only (the TAA run: 8 resets over 5400 frames, the portrait one frame per opening (tranche 41: about six, then kept); the DLAA run: 9000 frames all
  DLAA's, viewports 2 to 4, 16 resets); no latch row in either.
- **Compared at equal page moments** (`taa.py`, `taa-out.txt`, `dlaa-out.txt`, MEASURED; aaoff.py's crops, mask and bins; the switch
  shots). The body's gradient: the original's DLAA 9.70, our DLAA 9.73 (9.45 to 9.97; DLAA / original 1.00, every brightness bin 0.98 to
  1.07 but the brightest [130, 256) 1.25), our TAA 7.05 (6.75 to 7.39; 0.73, the bins 0.65 to 0.88 above the darkest). The face (the
  portrait, drawn once by opening: a single reset frame; tranche 41: about six resolves, then kept): the original 5.70, our DLAA 4.55 (0.80), our TAA 3.48 (0.61), the native
  without AA 6.21. The panel unchanged (12.36 to 12.38). So our DLAA gives the original's sharpness on the body; our TAA is softer: its
  reconstruction of the current frame (a 3x3 Blackman-Harris fit at the unjittered centre, `taa.hlsl`) filters even its reset frame,
  the face's case (INFERRED from the shader and the reset frame's measure; the world's TAA is built alike, R's « static detail »
  item, I-0140 t25). Thomas's sharpening applies to both and is 0 by default.
- **Thomas's choice (2026-11-22 08:02 Paris, in the thread: « Option 2 »):** the menu views' TAA left as is, his sharpening applying to
  them; asked before « Doux ou flou? », answered with `t40/sheet-body.png` and `sheet-face.png` (the original's DLAA, our TAA, our
  DLAA, the native without AA, the first opening's switch shot): the portrait's face blurred by the TAA's reconstruction on its single
  frame, the body softened, our DLAA close to the original's.
- **The final review** (fresh Opus, xhigh, `main...f2511a8d9`): no High. M1 (in DLAA, each Status opening made two new DLSS features:
  the opening frame 74 to 136 ms, MEASURED from `native_slow_frame` in `i140-t40-thomas-nat-dlaa`, cause INFERRED) fixed in
  `f957d24f5`: a retiring view's viewport is parked and the next view on the same context's inputs adopts it (or takes it from a view
  idle since the last frame), the oldest parked one's number taken when none is free; parked ones freed past four, after 36000 frames
  or once their context is gone. M2 (the records: the build list, the fixes' review missing, « witnessed » for code after `build-e`)
  fixed: the reviews above, the builds named, the merged code witnessed again (below). L1 (a repeated selection version shown as a
  reset frame) fixed: resolved with no motion (the collect's still matrix; DLSS's clip-to-previous the identity), its history kept, by
  the TAA and the DLAA, counted in the row (`repeats`). L2 (a closed menu's histories kept until a menu view renders) fixed: the main
  and no-world frames sweep. L3 (a stale comment), L4 (the viewport and the bins misquoted), L7 (TOOLS' DLSS row) fixed. L5 (STATE's
  rows naming the merge before it) left: they are written for the merge they precede. L6 (a constant velocity clear; the DLSS constants
  written beside the world's; a format branch reachable once) left: a clear and two barriers at 560x768, and a shared helper would
  touch the world's DLSS path for no behaviour change. Its fixes' step reviews: the sixth (`646a1b009..f957d24f5`; M1 the no-world
  frames did not sweep, L1 a dead context's parked viewport kept, L2 comments) and the seventh (`f957d24f5..dc5da3c3d`; L1 the cap
  counted dead parked ones) fixed in `dc5da3c3d` and `43f396097`; the eighth (`dc5da3c3d..43f396097`): no High or Medium, its two
  Lows on the records (`build-k` named the merged code's; these reviews unrecorded at its reading) answered here.
- **The fixed code witnessed** (2026-11-22 08:48-08:59 Paris, the coordinator's slot, as above; `t40/build-k` = `f957d24f5`, tests
  passed; `series-t40b.ps1 -Prefix i140-t40k`, `run-c.log`): `i140-t40k-thomas-nat-taa` and `-dlaa`, exit 0, 0 fault, restoration ok,
  eight openings each, no latch row. MEASURED (`opening.py`, `opening-out.txt`): in DLAA every opening on viewport 2; the first opening's
  frame 104.0 ms (the features made), the seven others no slow-frame row (before: all eight 73.7 to 136.3 ms); the TAA's first 36.7 ms
  (not comparable with tranche 38's native: these runs had the D3D12 debug layer on, its none, and its first
  opening was under the 34 ms row threshold; no frame cost of the menu AA is drawn from them), the others none; 0 repeated presentations over 9000 menu frames in each run. `dc5da3c3d` and
  `43f396097` (the no-world sweep, the parked viewports' frees) came after and were not run; `build-l` (`43f396097`, tests passed) their
  build. The latch paths (a menu TAA or DLAA refused) are written and reviewed, not witnessed: no test tool reaches them (the
  refusal flag counts the world's evaluations) and no run has a latch row.
- **DLSS Quality on the world** (Thomas, 2026-11-22 09:02 Paris: « Teste aussi DLSS Qualité »; 09:27-09:33, the coordinator's slot, as
  above; `t40/build-m` = `a01e680ae`, the final tree with `main` merged, tests passed; `series-t40b.ps1 -Only quality -Prefix i140-t40m`,
  `run-d.log`): `i140-t40m-thomas-nat-quality`, exit 0, 0 fault, restoration ok; `native_dlss_mode` quality. The menu views take DLAA
  under it as under the world's DLAA (MEASURED: 9000 menu frames all DLAA's, viewport 2 at every opening, 16 resets, 0 repeats, no
  latch row; the first opening's frame 98.6 ms, the seven others no slow-frame row, `opening-out.txt` method). Their sharpness at
  equal page moments (`taa.py`, `quality-out.txt`, the 16 switch shots): the body 9.76 (9.53 to 9.98; 1.01 of the original's 9.70),
  the face 4.68 (0.82), as the DLAA run's 9.73 and 4.55 (`dlaa-out.txt`). This run also witnesses `dc5da3c3d` and `43f396097`.
- **The integrity review** (fresh Opus, `main...69600342f`): no High. M1 (the native witnesses with the debug layer on, tranche 38's
  native without it: the « alike » unsupported) fixed above; M2 (STATE and a heading naming the merged code witnessed; `build-l`
  unfinished at its reading) fixed: `build-l` finished, STATE names `build-k` as the run's; L1 (`build-a`'s refusal without evidence)
  said where it was seen; L2 (the latch paths not witnessed) said. Its recount of the body and face on the `i140-t40k` shots: TAA 6.98
  and 3.50, DLAA 9.67 and 4.69 (as the first runs').
- **The merge** (2026-11-22): `6d9762f2f` into `main`; the launcher rebuilt from it (tests passed, `t40/launcher-rebuild.log`); its start check passed (09:44, `t40/startcheck.log`); the disk rotation from the main checkout: 568 files, 0.3 GB (one build folder older than 48 h).

## Tranche 39: the original anti-aliases its menu views (DLAA), the native not; D-0305

Branch `claude/e-i140-t39` (from `main` `474b05e7f`, 2026-11-22, worktree `fervent-chaum-630710`). Tranche 38's next step: what
smooths the original's menu view picture. RenderDoc is not used: its layer does not start the original (I-0141, « RenderDoc's
layer, first try »; cause not read). Scripts and outputs in `evidence/I-0140/t39/` (`edge.py`, `resample.py`, `bands.py` and their
`-out.txt` headed with their command; `series-t39.ps1`, `build-a.log`, `run-a.log` (the first try, refused before the game:
the flag not yet admitted by Run-StartupTrace) and `run-b.log`; the run `i140-t39-thomas-orig`).

- **On tranche 38's shots, no run** (the eight switch shots of each mode, the body crop 560x620 at (1290, 190), gray).
  - Steps' widths: strong monotonic steps (>= 60 levels over 8 px) along rows and columns are about as wide in both modes (mean
    3.22 natively, 3.33 in the original, pixels inside 10-90 %, the openings' means averaged from `edge-out.txt`), their 50 % crossings spread alike over the pixel (both about
    20/26/32/21 % by quarter): the measure is dominated by texture ramps and does not isolate the silhouettes (MEASURED,
    `edge-out.txt`); no conclusion drawn from it.
  - Round trips (area down by f, bilinear up, mean |difference|): the original loses less at every f (f = 1.25: 1.83 against 2.49;
    1.5: 2.17 against 2.92; 2: 2.66 against 3.51; 3: 3.49 against 4.46), the page's panel equal (3.11, 3.75, 5.07-5.08, 6.67), so the
    finest band (f = 1.25) keeps 73 % of the native's and coarser bands 79 to 87 % (MEASURED, `resample-out.txt`). A picture drawn
    1.25 times smaller or more and enlarged would keep almost none of the finest band: not that (INFERRED).
  - By region: the native's gradient over the original's grows with the original's local brightness (box 9x9): 1.07 in the darkest
    bin (below 20), 1.23-1.25 (20-40), 1.28-1.33 (40-90), 1.39 (90-130), 1.58-1.59 (130 and above), alike in a blurred and a hard
    shot; by colour saturation 1.22 to 1.36 (MEASURED, `bands-out.txt`). The local level difference (native - original, +0.2 to
    +5.8) gives a tone slope near 1 (about 0.91 to 1.18 between bins), too small for the 1.3 to 1.6 (INFERRED, arithmetic on the
    bins): the extra detail grows with contrast, as an edge-directed anti-aliasing filter's correction does (INFERRED). Against it:
    the [20, 40) bin holds 69 % of the crop with gradients of about 2 levels, under the Status body's DlaaThreshold 0.1 (gparam id
    11), yet its ratio is 1.23 to 1.25 (MEASURED): part of the gap is not explained by an edge filter (the final review's M2).
- **The original's menu views, read** (ghidra-research, GhidraMCP on the 1.15.2 image, 2026-11-22; re-read by a fresh Opus
  ghidra-research the same day, below):
  the menu views' pipelines are built at the holder's own extent (560x768 for the Status body, 512x256 for the portraits; the
  factory 1400C3410 takes the descriptor's extent when nonzero), one sample (bit 21 of the descriptor flags clear in both
  descriptors), the colour target at +0x1E0 handed to the UI proxy with no copy (140D64B50), READ. After the geometry the base
  scene's slot 4 (1402059F0) reaches FB890 (through 1400FCE30, pass 0x45) as for the world (INFERRED: the composite's dispatch
  for a menu view not traced); there, without YEBIS bit 10, the GX tone map, then, with the view byte +0x3EBA (1 from the
  pipeline's constructor 1400843C0, cleared by 140085540 only when the descriptor's bit 4 is clear, set in both), the anti-alias
  gparam record of the view's keys into the filter and 1B9670's enqueue of 1B9790 on +0x1E0 (READ); the gparam ids 10 to 15 have
  Enable 1, AntiAliasType 4 (DLAA) (READ, `evidence/I-0067/queries/status-light/default-gparam.tsv`). AA-NATIVE-READY §6's « menu
  views never reach it » (INFERRED there) is contradicted by this reading. Natively the menu views have no anti-aliasing: D-0278
  removed DS3's filter and neither DLSS nor the TAA resolves them.
- **The re-read** (fresh Opus, adversarial, the same image): the seven claims hold, none wrong. The dispatch for a menu view READ:
  the scene list runner 1400BFEF0 draws every listed composite (no main-view or index test; the views first, priority 100), the
  composite 1402046B0 calls each child, the wrapper's slot 8 (1400C1AE0) jumps to the base scene's slot 4; FB890, 1B9670 and 1B9790
  hold no view test. Precisions: +0x50 is the pipeline's copy of the descriptor's flags; slot 4 has two global gates
  ([144961AB0+0x3E1] == 0, the bytes +0x41B0/+0x41B1 of [GXRS+0x52E8] not both set); the filter is recorded in pass context 0x46;
  its enqueue does not depend on Enable (the run does). 1B9790's further conditions: mode < 4, the two programmes present, the target
  present. Its copy (14002C320) goes through a scratch target made at the display's extent (INFERRED: the object's identity), point
  sampled: exact when the display is at least the view's size (the ASUS window 1920x1080 is), the body point-decimated on a display
  under 768 px high. The gparam entries of the menu views (area 1: Status body 11, Status faces 12 and 13) give Enable 1 and DLAA,
  id 13 with DlaaThreshold 0.34 and DlaaRamda 2 (INFERRED: the area-to-file mapping is MENUS-MAP's). Not reached: a full-image scan
  for other writers of +0x3EBA and filter+8 (the headless mirror is gone; `t39/ghidra/dispscan.py`, a raw scan of the executable on
  disk, ran on the packed image, its matches noise and its output deleted by the subagent: no result either way).
- **The witness** (`af1ff2278`, `71315fe6d`, `f56e3b055`, `a6178f69a`): `aa-observe.flag`, Reference runs alone (`ReferenceAaFilterObserved`,
  04c_scene_witnesses.inc): 1B9790's calls by target size, mode, Enable, its first gate (`gate`) and the further conditions it can
  read without calling into the game (`ready`: mode < 4, the two programme objects, the target's object), a failed read named and
  never guessed; rows `reference_aa_filter` on the GetTickCount64 clock, totals at most every 2 s. Admitted by Run-StartupTrace's
  flag pattern and stale-flag list (`368ca9a91`, the run's first try having been refused there).
- **The run** (2026-11-22 05:56-05:59, the coordinator's slot after R's, the ASUS window 1920x1080, offline, the copy of Thomas's
  character restored by hash, no measure; `t39/build-a` from `a6178f69a`, a cache build: a witness run, not a merge's build): the
  original alone, the Status page opened twice (`i140-t39-thomas-orig`, 0 fault, restoration ok). The rows (MEASURED,
  `startup.jsonl`): 1B9790 runs with mode 3 (DLAA), Enable 1, the quality byte 1, the factor 1, its gate and `ready` true, on three
  targets: the world's 1920x1080 every frame from the first load; the Status body's 560x768 from 125 ms after the page's row
  (tick 160330187 against 160330062) on every frame of the page's visit (985 calls in the first visit's 16.5 s, about 60 a
  second, the 8 s before the switch key shows the views included: `status-1.png` at row + 4 s has no view; none between the
  visits); the portrait's 512x256 once per opening (one call each: the first opening's at the body's first, call 3168 beside 3167; the
  second's within the 2 s after its page row, by the totals). No failed read,
  no other key. So the original's menu body reaches its own DLAA after its tone map up to the programmes' lookups every frame of
  the visit (MEASURED), drawn (INFERRED), as the code reads; the portrait once, at the page's opening, about 8 s before it shows
  (when its view is listed for a frame: D676D0's unlisting, `16_offscreen_and_movies.inc`'s account).
- **Thomas's decision (D-0305, 2026-11-22 05:46 Paris, in the thread):** « Ce sera le TAA par défaut pour nous / DLSS / DLAA si
  c'est activé dans les options »: the menu views take the native's own anti-aliasing chain (the TAA by default, DLSS or DLAA by
  the option), none with the option off (Thomas, 06:04: « Et si l'anti aliasing est désactivé, on met rien du coup »), DS3's
  filter staying removed (D-0278). AA-NATIVE-READY §6 corrected.
- **Not done.** The menu views' TAA and DLSS (D-0305: a code tranche; per view a jitter, a history and the camera's motion, the
  object motion of the idle pose to be decided by the comparison); the portrait's 2 px (its camera or target); a native run with
  the switch key's logged moment (logged here in the original: started at row + 8.1 s, returned at + 9.1 to 9.7 s, `run-b.log`);
  the direct test of the DLAA as the cause (the original with Antialiasing off); step 2's paint; the held and neutral paths.
- **Step reviews** (fresh Opus, high). `3bc407dea..af1ff2278`: no High; M1 (a failed read of the quality object or byte reported as
  read) fixed; M2 (« gate » said « draws » though the function tests more after it) fixed: `ready`, the comment says what is not
  seen; M3 (no totals before 3600 calls, the tail lost) fixed: every 2 s of the clock; L1 (a NaN factor runs in the original) fixed;
  L2 (placeholders written as read) fixed: null; L3 (a null +0x10 object as a failed read) fixed; L4 (the totals' call against its
  counts) fixed: counted under the lock; L5 (keys unbounded) fixed: 64; L6 (allocation in a noexcept detour) left: the file's
  witnesses' pattern, out of memory only; L7 (the statics not in the thread-affinity list) left: that check covers the native
  renderer's worker and prepare functions, not detours of the original's code (`71315fe6d`). Its fixes' review
  (`af1ff2278..71315fe6d`): no High; M1 (an infinite factor printed `inf`, not JSON) fixed: `%.9g`, quoted when not finite; L1 (a
  failed key without its gate) fixed; L2 (a null target as a failed read) fixed; L3 (the reads not in the function's order) fixed;
  L4 (NaN through !(a <= b)) fixed: explicit; L8 (« a Status visit's totals are written » unshown) fixed: INFERRED in the comment;
  L5 (rows bounded in rate, not in total), L6 (rows of several threads out of call order: sort by `call`), L7 (allocation), L9 (the
  affinity list) left (`f56e3b055`). Its fixes' second review (`71315fe6d..f56e3b055`): H1 (a lost quote before the factor
  field: that tree did not compile) fixed; L1 (the comment's order of reads), L2 (the size read only with a +0x10 object), L3 (a
  value decided before a failed read kept, the header said otherwise) fixed in the comments; L4 (a failed key without ready and the
  mode, `e0` when Enable was not read) fixed: `failed %s e g r m`, -1 when unread; L5 (a null target where the original would fault)
  said in the comment (`a6178f69a`, built and run).
- **Final review** (fresh Opus, xhigh, `main...62a5895b1`): no High. M1 (« every frame while shown »: the body is filtered on every
  frame of the page's visit, the views hidden before the switch key included, the portrait at the page's opening) fixed. M2 (« grows
  with contrast » MEASURED and the DLAA as the whole cause: the dark bin's 1.23 to 1.25 under DLAA's threshold) fixed: relabelled,
  the contradiction written, the direct test first in « Next » 11. M3 (the third step review unrecorded) fixed above. M4 (size
  warnings: ROADMAP, the index, STATE « Next action ») fixed. L1 (`resample.py`'s header) fixed. L2 (AA-NATIVE-READY §9, the line on
  one DLAA draw, the tense) fixed. L3 (LOG D-0305 beyond Thomas's words) fixed: the method line the pilot's, the filter's removal
  « implied ». L4 (the draw under MEASURED) fixed: up to the programmes' lookups. L5 (« Not done » and the switch key; STATE's time
  and statuses; the tone slope 0.91; the step widths averaged) fixed; the « merged » counts reconciled at the merge.

## Tranche 38: the Status shots at their target tick; the body sharper natively, the portrait 2 px left

Branch `claude/e-i140-t38` (from `main` `ad7fa2b40`, 2026-11-22, worktree `fervent-chaum-630710`). Tranche 37's next step: shots at
equal moments before any reading of the pose, then the body's residual and the portrait's lead on such pairs. Scripts, logs and
pictures in `evidence/I-0140/t38/` (`series-t38.ps1`, `run-a.log`, `compare.py` and `compare-out.txt`, `frame.py`, `residual.py`,
`sharp.py`, `sharp2.py` and their `-out.txt` naming their inputs and crops; `chest-nat-orig-x3.png`, `portrait-nat-orig-x2.png`,
`body-nat-orig-diff6.png`, made as `pictures.txt` says).

- **The tool.** `Pilot-Game -Action shot -AtTick n` waits inside the process for GetTickCount64 to reach n (Thread.Sleep(1) under a
  1 ms timer period, a spin over the last 2 ms; a warm-up copy of the whole screen first; a target more than 30 s ahead refused;
  the game's absence ends the wait and takes no picture) and prints `t0` (the tick before the copy), `ready` and `late` (ready
  after n). Run-AlphaCast's `ShotAt` starts it 2.5 s before each Status target (row + 4, 8, 12 s; the switchflip at
  max(row + 16 s, flip + 1 s)); a target not near in 25 s is said and not shot. Without `-AtTick` the shot is as before.
- **The run** (2026-11-22 04:34-04:46, the coordinator's slot after the launcher's rebuild, the ASUS window 1920x1080, offline,
  the copy of Thomas's character restored by hash in both modes, no measure; `t38/build-a`, a copy of `t37/build-a`, whose
  `native/` equals `main`'s): tranche 37's series again (`i140-t38-thomas-nat`, 0 fault, 19 084 native frames;
  `i140-t38-thomas-orig`). All 64 shots began their copy 0 to 1 ms after their target, the process ready 1.8 to 1.9 s before it
  (MEASURED, `run-a.log`); tranche 37's were 0.6 to 1.2 s late.
- **What the shots show.** Shots 1 and 2 (row + 4 and + 8 s) are the page's statistics (no menu view): the native within 0.72 of
  the original (gray mean |d|, 0 % of pixels over 12) in all sixteen; the views appear with the switch key, pressed by Pilot-Game
  after shot 2 and not on the clock: each key's Pilot-Game ended before row + 9.5 s (INFERRED: every switch shot's process was ready 1.83 to
  1.93 s before row + 12 s, MEASURED, and ShotAt starts it only after the key's return and from row + 9.5 s), its moment otherwise not logged in this run (logged since, the final review's M3). So the
  pairs are equal on the page's clock, not on the views' own (a limit).
- **At equal page moments the blurred self-shadow is nearer the original in every pairing (MEASURED).** Body (tranche 37's crop),
  each native shot against the eight original shots of its kind at one best shift: at the switch blurred 3.28 to 4.02 (means
  3.39 to 3.51), hard 3.79 to 4.57 (means 3.91 to 4.18); at the switchflip blurred 3.27 to 3.91 (means 3.41 to 3.55), hard 3.78
  to 4.59 (means 3.88 to 4.11); against each original shot every blurred shot is nearer than every hard one. The spread between
  openings of one state at zero shift (the first opening against the others, `compare.py`): the original 0.15 to 2.73 (its opening 4 the furthest), the native 0.55 to 3.00 in either
  state: the two modes repeat alike at equal page moments (MEASURED). Tranche 37's reading (« the body's pose seems to
  follow that moment in both modes ») is not borne out on the page's clock (untested on the views' own, the switch key's): at equal page moments the original's spread
  (up to 2.73) is not smaller than its on-time spread there (0.19 to 1.42); what varies is perhaps the switch key's moment
  (above; INFERRED, untested).
- **The body's residual (switch, blurred against the original, MEASURED).** Mean |d| over the body's pixels (original gray > 20)
  4.52 raw, 2.70 and 2.40 after a Gaussian blur of 2 and 4 px on both, of which a signed +2.0 levels (the native brighter, the
  middle third +2.9, the top +1.7, the bottom +1.1 to 1.4; a hard shot alike, +1.9): a brightening, unequal by height, plus
  detail; the original against its own shots 0.18 (opening 2) to 2.95 (opening 4, its furthest) raw (`residual-out.txt`). The detail: the gradient (|dx| +
  |dy|, sharpness or texture contrast) over the body 13.3 to 13.6 natively against 9.7 to 10.0 in the original (+35 %, MEASURED),
  the portrait view's face +9 % (6.24 against 5.72), the page's own panel equal (34.28 against 34.34, the control). By eye at x3 (`chest-nat-orig-x3.png`) the native's edges stepped
  (the medallion's rim, the buckle) and its textures crisper, the original's smooth (OBSERVED): the original's menu view is anti-aliased or
  filtered by something the native does not do (INFERRED; the mechanism not read). Not the materials' mip bias: the menu views
  keep the source sampler bank, the TAA's and DLSS's bias going to the main view only (READ, `NativeMaterialBank`,
  `native/frame/game_bootstrap/10_options_and_passes.inc`). AA-NATIVE-READY §6 says menu views never reach
  DS3's filter (INFERRED there): a RenderDoc capture of the original's Status page (the body view's passes, its target's size
  and samples) is the next step, then Thomas's choice (the original's way, or a modern one such as rendering these small views
  at twice the size), D-0278 not covering menu views as written.
- **The portrait's lead (MEASURED).** At the switch, the portrait's frame (the page's UI) at zero shift in both modes (left, top
  and right edges best at (0, 0), |d| 0.18 to 0.62, `frame-out.txt`) while the face inside it is best at (-2.0, +0.75) px (|d|
  10.27 at zero, 2.91 at it): the native's picture of the view 2 px left of and 0.75 px below the original's inside the same
  frame, in all six regions (-1.75 to -2.25, +0.5 to +1.5) and identical in the native's openings 2 to 4 (`compare-out.txt`).
  So not the composition of the page but the view's own picture: its camera or its target's placement (not read). The view
  is 512x256 with a vertical fov of 0.1745 rad, 10° (the native's `native_offscreen_view_selected` row): about 1 mm a pixel at the
  head, so 2 px is about 2 mm of the eye, the target or the head between the modes (INFERRED, arithmetic on the row).
- **Not done.** The step 2 paint, the held and neutral paths, the switch key on the clock.
- **Step reviews** (fresh Opus, high). `ad7fa2b40..550d39f27`: no High; M1 (`Start-Sleep -Milliseconds 1` slept about 15 ms
  under the 1 ms period, so `late` up to 13 ms on time) fixed: `Thread.Sleep(1)`, `ready` and `late` apart; M2 (Pilot-Game's
  wait unbounded) fixed: a target over 30 s ahead refused, the game's absence ends the wait, ShotAt says a target not near in
  25 s; L1 (a game closed during the wait shot as a status page) fixed: « gone », no picture; L2 (`t0` before the first copy's
  set-up) fixed: a warm-up copy; L3 (units, the failure's text) fixed (`a41b5f2e9`). Its fixes' review (`550d39f27..a41b5f2e9`):
  no High or Medium; L1 (`Max(0, …)` on Int32) fixed; L2 (the child's colour codes in the run line) fixed; L3 (the loop's comment
  on `late`) fixed; L4 (the full-size bitmap's set-up inside the timed copy) fixed: the warm-up copies the whole screen into it
  (`2bb28ad18`, `bcd9ef1b2`).
- **Final review** (fresh Opus, xhigh, `main...e4e85fa3a`): no High. M1 (the RESULT's « Next » item 11 still tranche 38's own
  work) fixed. M2 (Pilot-CharMakeWitness's `-OnClock` shots, the creation and Face Detail, still started after their target)
  fixed: the same `-AtTick` start 2.5 s ahead. M3 (the switch key's moment not on the clock, « about 1 s » without evidence) fixed:
  the key's ticks logged from now, the bullet above rewritten from the log. L1 (the presence check able to fall in the last ms)
  fixed: never in the last 300 ms (`2c42a0c19`). L2 (tranche 37's pose reading not reconciled, its quote) fixed above. L3 (the
  residual without its floor, « uniform ») fixed. L4 (provenance: `sharp2.py`'s crops, the residual's inputs, the pictures)
  fixed (`sharp2-out.txt`, `residual-out.txt` headed, `pictures.txt`). L5 (statuses) fixed. L6 (STATE and the index ahead of the
  merge, the « Updated » time) answered at the merge. L7 (the step reviews not in the RESULT) fixed here. L8 (STUDY-ALL-GAPS row
  18) fixed.
  Its code fixes' step review (`e4e85fa3a..2c42a0c19`): no High or Medium; L1 (the Witness's line dropping Pilot-Game's error
  text) fixed: « no row » apart from « no capture tick » with the text, as Run-AlphaCast; L2 (« never in the last 300 ms » true of
  the check's start only) fixed in the comment; L3 (no guard on a target too far ahead in the Witness) left: unreachable with
  today's ranges (at most about 57.5 s, inside its 60 s cap). These runner changes (`2c42a0c19` and after) are not yet run.
- **Integrity review** (fresh Opus, `main...ead865691`): no High or Medium. L1 (« at equal moments » and « not borne out » in the
  summaries without the page-clock qualifier) fixed in the Conclusion, STATE and the index; L2 (« ended before row + 9.5 s »
  reasoned, tagged MEASURED) fixed: INFERRED; L3 (the two modes' spreads taken alike but unnamed: the first opening against the
  others) fixed: the method named.

## Tranche 37: the Player views' blur, hard and blurred in one run

Branch `claude/e-i140-t37` (from `main` `9cd05b83e`, 2026-11-22, worktree `awesome-banach-dee1a2`). Tranche 36's next step: hard and
blurred compared in one run, the spread between openings of one state the noise floor.

- **The tool (`5ab4d991e`).** `menu-shadow-blur-toggle.flag`, a test tool read once: CollectNativeMenuSelfShadow then looks for
  `menu-shadow-hard.now` beside the module at most every 250 ms; while the file is there the Player views' blur passes are 0 (the
  traced mask as tranches 31 and 34 drew it), with a row `native_menu_shadow_blur_test` {hard, tick} at each change. Without the flag
  nothing is read. Its two statics (`looked`, `hard`) added to `scripts/thread-affinity-allow.json`: written only under
  `native_offscreen_collect_lock` (the function's one caller). `Run-AlphaCast -StatusBlurToggle` (with `-StatusViewTimes` now up to 8): blurred on openings 1, 3, 5, 7, hard on
  2, 4, 6, 8, each opening's switch shot followed, 3 s after flipping the file, by `<stem>-switchflip.png` in the other state; the
  file removed after the last opening and on any error. Scripts in `evidence/I-0140/t37/` (`series-t37.ps1`, `pairs.py`); the
  run carried the observer flag `light-observe` in both modes, as tranches 31 to 35.
- **Step review** (fresh Opus, high; `9cd05b83e..5ab4d991e`): no High or Medium. L1 (the flipped state left in force after the
  last opening, so later steps of the run would draw hard) fixed: the file removed after the openings. L2 (a failed delete still
  named « blurred ») fixed: the state named from the file after the action, a state not reached stops the run. L3 (a killed run's
  flag and file not removed at the next start) fixed: both in Run-StartupTrace's stale list. L4 (no row for the starting state, so a
  blurred first opening cannot tell the toggle on from the flag not taken) left: the module was building; the first hard opening's
  row and the runner's line per opening say it (`2d226b5be`).
  Its fixes' review (`5ab4d991e..2d226b5be`): no High or Medium; L1 (the comments still « removed at the end ») and L2 (the original's
  lines named a self-shadow state the file does not give it) fixed; L3 (`no-menu-shadow.flag` not on the stale list, earlier) fixed (`986c3c18d`).
- **The run** (2026-11-22 03:35-03:47, the coordinator's slot after R's, the ASUS window 1920x1080, offline, the copy of Thomas's
  character restored by hash in both modes, no measure, `t37/build-a` of `5ab4d991e`, 186 PASS; `t37/run-a.log`). Native
  (`i140-t37-thomas-nat`, 0 fault): eight openings, the module's row at each file change within 16 to 211 ms of the runner's
  (`native_menu_shadow_blur_test`), the menu rows' `blur` [6, 8] on blurred openings and [0, 0] on hard ones, the renderer 10 429
  traced frames, 4 770 blurred, none refused, held and neutral 0; opening 8's page row not seen (its shots timed from the wait's
  end, left out). The original (`i140-t37-thomas-orig`): the same eight openings and shots (the file does nothing there).
- **The shots' moments.** Each shot's capture tick minus the page's row tick (`t37/run-a.log`), openings 1 to 7: native switch
  12 658, 12 891, 12 873, 12 647, 12 793, 13 190, 12 650 ms; original switch 12 601, 12 596, 12 601, 12 587, 12 860, 12 627,
  12 597; native switchflip 16 357, 16 620, 16 569, 16 328, 16 678, 17 359, 16 334; original switchflip 16 251, 16 214, 16 233,
  16 219, 16 693, 16 243, 16 228. Each shot spawns Pilot-Game only after its target tick, so it lands 0.6 to 1.2 s after it, by a
  few hundred ms more or less (the final review's M1: the switchflip shot also waited 3 s from the flip, not the row clock; now
  16 s after the row, at least 1 s after the flip; not yet run).
- **The body's pose seems to follow the capture moment in both modes (INFERRED from measured distances and ticks; whether the
  native moves more per ms not separated).**
  The body (560x620 at (1290, 190), gray, zero shift, `t37/pairs-out.txt`, `pairs2-out.txt`): the original's switch shots taken
  within 40 ms of each other (openings 1 to 4, 6, 7) are 0.19 to 1.42 apart, its one late shot (5, about 250 ms later) 4.7 to 4.9
  from them; the native's 1, 4, 7 (within 11 ms) 0.82 to 2.05 apart, and from 1 its distance rises with the moment's gap: 4.79
  (135 ms, opening 5), 5.94 (215 ms, 3), 6.73 (233 ms, 2), 8.81 (532 ms, 6), though 2 and 6 are hard against a blurred 1 (the
  state alone about 2, as 4 from 1) and 2 and 3, 18 ms apart, are 3.40 apart. The body moves between the switch and switchflip shots
  in both modes (the original's same opening 2.97 to 4.06: the idle motion). So the native's pose groups are most likely the
  runner's capture jitter, as the original's opening 5 is; tranche 36's « geometry between runs » may be the same (not reread). The residual of the
  on-time native shots against the original (3.26 to 3.41, 20 to 70 ms apart, where the original's own on-time shots are within
  1.42) is not separated between that motion, shading and placement.
- **At equal moments the blurred state is nearer the original (MEASURED in the on-time comparisons).** Mean |d| against the original's shots of the same
  page moment (D-0299: switch with switch, switchflip with switchflip), each with its best shift within 3.75 px, in four
  comparisons: the on-time shots at the switch (against the original's openings 1 to 4, 6, 7), blurred 3.26 to 3.41 (1 and 7,
  12 658 and 12 650 ms) against hard 3.75 to 3.84 (4, 12 647 ms); at the switchflip blurred 3.60 to 3.90 (4f, 16 328 ms) against
  hard 4.05 to 4.45 (1f, 7f, 16 357 and 16 334 ms); the late shots at the switch against the original's opening 5 (12 860 ms),
  blurred 3.62 and 3.71 (3, 5: 12 873 and 12 793 ms) against hard 3.96 (2, 12 891 ms); at the switchflip against o5f (16 693 ms)
  blurred 3.74 (2f, 16 620 ms) against hard 4.03 and 4.44 (5f, 3f: 16 678 and 16 569 ms). Opening 6 (13 190 and 17 359 ms) is left
  out (no original shot near its moments; both its states about 7). In the two on-time comparisons the blurred shot is nearer by
  0.43 to 0.67 against each original shot, where two shots of one state taken within 25 ms differ by 0.00 to 0.20 in this measure
  (1 and 7; 1f and 7f): beyond the spread. In the late ones the margin is 0.25 to 0.34 at the switch (one hard shot, its spread not
  measured; the two blurred 0.09 apart) and 0.29 at the switchflip, where the two hard shots, 109 ms apart, differ by 0.41: within
  the spread. The direction is the same in all four. The hard shot is as near the original's moment as the blurred ones in the
  on-time switch (opening 4 against 1 and 7) and nearer in the late switchflip (5f against 2f), nearer than one of the two blurred
  shots in the late switch (2 against 5, not 3), and further in the on-time switchflip. By eye at x2 (`t37/chest-n7blur-n4hard-o1-x2.png`: blurred, hard, the original) the hard shadow under
  the cloak is darker and sharper, the blurred one nearer the original's. The blur stays as tranche 35 drew it; its pass count and
  radius unchanged (D-0268).
- **The journal's states.** The module writes a row at each change of the file only, not for the starting state (the step
  review's L4, left): a blurred first opening is told by the runner's line and the menu rows' `blur` field; enough for a test tool.
- **Next.** Shots at equal moments before any reading of the pose: the capture taken at its target tick (the shot process started
  ahead and waiting for the tick, or pairs chosen by capture tick within a stated tolerance); then the on-time residual (3.3)
  against the original, and the portrait's 2 to 3 px lead (tranche 36), on such pairs.
- **Final review** (fresh Opus, xhigh, `main...f90ac76f3`): H1 (« the native's pose less repeatable at the same page time », worded
  MEASURED and made the next lead, while the log's capture moments explain the pose groups in both modes) fixed: the claim and
  every summary rewritten (above; the commit message of `f90ac76f3` keeps the first wording). M1 (the switchflip shot timed from the
  flip, not the row clock) fixed in Run-AlphaCast, not yet run (its captures should land about 0.3 s later than this run's switchflip shots). L1 (« six groups »: four) fixed. L2 (« 3 and 5 » not one pose; the noise floor's
  pairs) fixed: 1 and 7, 1f and 7f. L3 (two statuses for one claim) fixed with H1. L4 (`pairs.py` edited after its output) fixed:
  rerun on the stored script (the same numbers). L5 (no time budget for eight openings) fixed: with `-StatusView`, `-Seconds` of
  about 110 + 27 per opening or more (31 with the toggle), from the logs; the fix's step review (`f90ac76f3..10ab01151`): no High or
  Medium; L1 (the first bound, 150 + 32, refused runs that finish) fixed so; L2 (M1's fix not run) said. L6 (STATE's « 1 to 36 »,
  the index's « (36, 37) ») fixed. L7 (no row for the starting state) said above.
- **Integrity review** (fresh Opus, `main...10ab01151`): no High. M1 (the noise floor, taken from on-time pairs, applied to the late
  comparisons, where the hard shots spread by 0.41 against a 0.29 margin) fixed: two comparisons beyond the spread, the late ones
  not. L1 (the pose following the capture moment worded MEASURED) fixed: INFERRED. L2 (« three of the four » not true of the late
  switch) fixed. L3 (the runner's fixes not run; opening 6 left out unsaid) fixed. L4 (the allow-list's two entries and the
  observer flag unnamed) fixed in the tool's bullet.

## Tranche 36: the Status body's distance after tranche 35 is the views' geometry between runs, not shown to be the blur

Branch `claude/e-i140-t36` (from `main` `d8de5c91c`, 2026-11-22; documentation only, no run). Tranche 35 left the Status body
further from the original after the blur, its cause not established (pose, placement or blur). Scripts and pictures in
`evidence/I-0140/t36/` (the pilot's `subpix2.py`, a quarter-pixel shift search within 2 px on a crop given by `CROP`, `face.py`,
`signed.py` and `torso.png`, `dsheet.png`; the final reviews' `edges*.py`, `region_shift.py`, `tiles.py`, `tiles2.py`, `ui.py`,
`portrait_regions.py` and its `.txt`, `within.py`, `within2.py`).

- **The menu views' geometry differs between runs (MEASURED); pose and placement not separated.** The Status page's portrait,
  unshadowed (D-0303), moves as a whole: the same shift in its six regions (`portrait_regions.txt`): tranche 34 against 31
  (+1, 0 to -0.75) px; the native portrait 1.75 to 3 px left of and 0.5 to 1.5 px below the original's (one low-contrast
  region of tranche 35 the other way vertically; a lead, below). The body:
  tranche 35 and tranche 31's control without the self-shadow (`no-menu-shadow`) share one geometry (|d| 1.48 on the portrait at
  shift 0; the body's best shift the same per region), tranches 31 and 34 another, the original's body with them; its gauntlet's
  lower edge (median over columns, three thresholds, `edges*.py`) 1.5 to 2.4 px higher in tranche 35 and in the control than in
  the original, tranches 31 and 34 within 0.3 px: that the shadowless control moves with tranche 35 is what says geometry, not
  shading. Per region (`region_shift.py`) tranche 35 against 34 is a common shift of about (-1, +0.75) px plus the arm's own
  (helmet (-1, 0.75), knee (-0.75, 1.0), gauntlet (-0.25, 2.0)). Within one run the page repeats only in part: tranche 34's three
  reopenings, 1 and 3 at |d| 0.81, reopening 2 at 2.14 and 2.27 from them, not removed by a shift (2.11) and on edges (the
  gradient 24.7 where it differs, 5.66 on average, `within2.py`): unexplained (MEASURED). The page's UI is the same across native
  runs (`subpix2.py`, 700x700 at (100, 150): |d| 0.09; 0.61 against the original).
- **At zero shift the blurred body is further; with the geometry compensated it is as near as the hard runs (MEASURED).** Against
  the original at zero shift: tranche 34 (hard) 4.13, tranche 31 (hard) 4.57, tranche 35 (blurred) 6.35, the control 8.03
  (`subpix2.py`, the body's 560x620 crop at (1290, 190)): this pairs unequal geometries, which D-0299 does not compare. Per 40x40
  tile with its own best shift (82 tiles, `tiles.py 1290 190 560 620`): within 3 px tranche 35 7.67, tranche 34 7.85, tranche 31
  8.72, the control 10.54; within 2 px (`tiles2.py`) 8.47, 8.00, 8.85, 11.02: the order depends on the window, the missing shadow
  still seen. The Face Detail (640x600 at (1150, 300), `face.py`, against tranche 31e's original shots), at zero shift hard
  (tranche 31e) 2.44, 3.29, 3.13, 3.88 and blurred (tranche 35) 4.35, 4.20, 3.66, 3.93; after a quarter-pixel realignment
  (`subpix2.py`) blurred against hard 3.09/2.69, 2.61/2.57, 3.01/2.73, 2.59/2.62: the blurred still further on three shots by
  0.04 to 0.40, shot 0 most. So tranche 35's « Status body the other way » was the views' geometry; whether the blur brings the
  views nearer the original is not shown either way. My first reading (« the blur, not the pose ») was wrong and is withdrawn (the
  final review's H1).
- **The taps' unit, a static read (INFERRED).** 1D7850 divides each tap by W, H = the pass camera's `+0x5C`, `+0x60`
  (`[pass+0x418]`); the main view's camera (CED670) holds the screen size S there. A `ghidra-research` read (GhidraMCP,
  2026-11-22): the holder's camera (D64C30 builds `holder+0x28` by 4AFF0; ED790 writes the constants 1280 x 720 there) is recorded
  by 103510 -> 138540 -> BEEE0 with the pipeline's `+0x20`, `+0x24` (the scene descriptor's w, h from C3410: the holder's 560 x 768
  or 784 x 784) as an override when both are nonzero, the inner size `cam+0x7C`, `+0x80` instead for a tiled camera
  (`+0x70` above 1; READ in `decompiled/DS3-1152-F-000BEEE0.c` 90-101); at replay 22AC60 copies the record into the
  context's `+0x680` and points `+0x418` at it, the subagent's disassembly mapping the record's fields back to the camera's layout
  (BEEE0's record holds the aspect at `+0x5C`, W, H at `+0x60`, `+0x64`); 22AC60 and ED790 are not exported, 138540's second
  camera record (type 6, no override) not followed, and that 1D7850's context is that replay context is not read end to end. So
  W, H the view's size, as tranche 35 drew them, is INFERRED; a live read at 1D7850 during a Status draw
  (`*(rdx+0x418)+0x5C/+0x60`, expected 560, 768 on the untiled branch) closes it.
- **Next.** First a comparison in one run: several Status reopenings per state, hard and blurred alternating (a test flag
  toggled between reopenings), the spread within a state (2.1 to 2.3 between reopenings here) the noise floor; then, only if they
  differ beyond it, the original's own mask (RenderDoc) and the live read above. Neither pass count nor radius is changed on a
  picture fit (D-0268). A lead: the native's unshadowed Status portrait 1.75 to 3 px left of and 0.5 to 1.5 px below the
  original's in the three native runs (`portrait_regions.txt`, one low-contrast region the other way vertically), perhaps a
  placement departure, not read.
- **Reviews.** Final review (fresh Opus, xhigh, `main...6a61c97fa`): H1 (the cause read as the blur while the pictures show a
  geometry difference) fixed by the rewrite; M1 (22AC60's mapping unexported, worded READ) fixed: INFERRED; M2 (the Conclusion and
  STATE left contradicting) fixed; L1 (the map's texel count without a source) the sentence removed; L2 wording; L3 the scripts
  named; L4 main merged in; L5 the RESULT's Next. Second final review (`main...b676e9923`): no High; M1 (« a different pose, not a
  placement » beyond the evidence, the portrait a uniform shift) fixed: pose and placement not separated; M2 (« between the two
  hard ones » against its own 3 px numbers) fixed: the order depends on the window; M3 (« within one run it repeats » overstated)
  fixed: reopening 2's residual given, the next step with several reopenings per state; L1 (the Next list's order) fixed; L2 to
  L6 fixed in the text; L7 the integrity review rerun on the final tree; L8 the portrait's lead recorded. Integrity review on the
  final tree (`main...2088745d3`): no High; M1 (the portrait's lead written with its direction reversed) fixed; L1 (a range)
  fixed; L2 (the Face Detail's realigned result missing from the summaries, tranche 35's neck percentages a zero-shift measure
  across runs) fixed in the Conclusion and STATE. First integrity review
  (`main...bb0f72130`): no High or Medium; L1 (the means without a script) and L2 (the UI crop) traced; L3 and L4 superseded by the
  rewrite.

## Tranche 35: the Player menu views' penumbra by the original's blur; step 2's paint with F4

Branch `claude/e-i140-t35` (from `main` `d36068760`, 2026-11-22, worktree `exciting-panini-b7e96d`). Tranche 31 left the Player
views' traced self-shadow with a hard, stepped edge where the original's is soft (`t31/face-ctl-nat-orig.png`).

- **Read: the original blurs the menu views' mask on screen.** The menu views' mask (mode 1, the single-map executor 1D6550)
  draws a filter programme chosen by the record's filter mode (`+0x40`, `+0x48` or `+0x38`, `decompiled/DS3-1152-F-001D6550.c`
  314-322; which one the menus take, and its PCF width, OPEN: SUN-SHADOWS-C4-STATIC-PREP §1, §6), then calls the
  `GXScreenSpaceShadow_Blur` passes (1D7850, 1D6550 line 433; `decompiled/DS3-1152-F-001D7850.c`; STATIC-PREP §3.2): the pass
  count is the ShadowParam record's Blur Count (`+0x84`) plus the quality row's blur addition (`+0x3C`; +2 at MAX in the native's
  model of 1B5720), alternating between two targets, the first write into the second one
  when the count is odd so that the last lands in the mask; each pass's cbBlurParam (0xD0 bytes) holds `g_vDepthComputeParam` and 12
  taps {x / W * s, y / H * s, G_k}, s = the float at `[[GXRS + 0x5368] + 0x128]` (1 where read) times the record's Blur Radius
  (`+0x88`) (READ). The six passes at MAX are the native's count (`blur [6, 8]` in the journal), the original's own count at MAX
  not observed. The pixel programme (its DXBC disassembled by fxc): the centre at weight 1, each tap G_k * exp(-100 (z - z0)² /
  z0²) on the linear depth, `o.w` the weighted mean of the shadow term, `o.xyz` the centre's packed depth, ClampPoint sampling
  (READ). The menu records (default.gparam): ids 10 and 12 to 15 Blur Count 4, Blur Radius 4; id 11 Blur Radius 8 (READ): 6 passes at
  MAX. G_k = exp(-r² / 2) / sqrt(2 pi) is INFERRED (its two helpers unnamed in the decompile). The filter programme's own width is not reproduced.
- **The code (`82839ffda`, the step review's answers `45a0d4d07`).** `rt_shadow.hlsl`'s `blur`: the same 12 taps scaled by s, the
  same weights on the depth plane's view depth, the centre at weight 1; the intermediate targets' xyz white on the G-buffer and black
  off it (`neutral` 2), so that a tap off the G-buffer weighs 0 as the original's packed depth 1 does; the last pass's xyz the white
  the menu lighting reads. The collect reads the record's count and radius and the GXRS factor (checked reads; none, a factor of
  1); the renderer runs the trace into the target the parity names, then the passes (each its own graph pass `self_shadow_blur`,
  the pass count in the plan's key), a failed pass leaving the mask white; the blur target and its views allocated with the mask's
  (its slots all or none; a failed blur target leaves the mask alone); the blur's pipeline made on its first use (a failure stays the menu's). The journal: `native_menu_self_shadow`'s
  `blur` [passes, scale], the renderer row's `blurred`. The unit test (`rt_scene_test` SunShadow): two passes against the formula
  computed on the bytes of each pass's source, one surface pixel moved to another depth (a depth weight between 0 and 1).
- **Step reviews** (fresh Opus, high). `d36068760..82839ffda`: no High. M1 (the blur's pipeline made in the main view's
  `Encode`: its failure would have turned the game's traced sun off for the session) fixed: made on the blur's first use; L1 (the
  test met only depth weights 0 and 1, one pass) fixed: a pixel at another depth (weights 0.37 and 0.44) and two chained passes;
  L2 (an infinite scale let through at the renderer) fixed; L3 (the collect's row not repeated on a blur change) fixed; L4 (the
  shader's header) fixed. `82839ffda..45a0d4d07`: no High or Medium; L1 (the test's header) and L2 (the shader's header: the main
  view's defaults unqualified) fixed (`0677c8433`). Builds `t35/build-a` and `build-b` (`-Cache`): 178 PASS; the thread-affinity
  checker PASS (rerun on the final tree, `t35/thread-affinity.log`).
- **The run** (2026-11-22 00:58-01:16, the coordinator's slot after R's, the ASUS window 1920x1080, offline, every save copy
  restored by hash, no measure, `t35/build-a`, `t35/series-t35.ps1`, `t35/run-a.log`).
  - **Face Detail** (`i66-native-i140-t35-face`, the shots on the Appearance page's row clock as tranche 31's `i140-t31e`, whose
    original's pictures serve: D-0299, the original unchanged): the journal's views `key [1,11]` width 784, `blur [6, 8]`; the
    renderer 16795 traced frames, 16795 blurred, none refused, no failure. The neck's hard diagonal edge under the jaw is now soft
    as the original's (`t35/neck-t31-t35-orig-x2.png`: tranche 31, now, the original; OBSERVED). On the neck (320x220 at (1270,
    620)) against the original, shots 1 to 3: pixels off by more than 12 levels 3.5 to 4.0 %
    before, 2.1 to 2.8 % now (shot 0 the other way: 7.6 % before, 8.3 % now; |d| 4.20 and 4.81; the pose moves between every shot); mean difference per shot 2.19, 1.99, 2.51 before and 2.29, 1.74, 2.14 now (shot 1 worse); the luma gradient 2.79 to 2.94 before, 2.74 to
    2.86 now, the original's 2.56 to 2.71 (MEASURED, `neckdiff.py`). The skin's statistics (tranche 30's crop and `t30/skin-stats.py`) unchanged within 1
    (mean 118.4 to 124.3; the original 117.3 to 122.4).
  - **Status body** (`i140-t35-thomas-nat`): `key [1,11]` width 560, `blur [6, 8]`, 962 traced, 962 blurred; the body's mean 37.49
    (tranche 31 37.70, tranche 34 37.36, the original 35.86), its luma gradient 10.09 (10.38 in tranche 31, the original 7.61)
    (MEASURED); against the original's picture the body is further off than before: |d| 6.03 and 14.3 % of pixels off by more
    than 12 levels (tranche 31 4.40 and 10.7 %, tranche 34 3.96 and 9.3 %), still about 5.7 after a 2 px realignment
    (MEASURED, `body.py`, the integrity review); whether the pose, the placement or the blur is the cause is not established.
  - **Step 2, the struck player's paint: seen in both modes (OBSERVED).** Eta's m40 save teleported beside the two grave wardens
    (tranche 27's place), F4 at once (`test_invincible` on, the HP refilled each frame: 669 at most; struck at 301 and 309 before
    the press in the two runs), 90 s of hits, a shot every 5 s. Natively the paint ran (`native_paint_drain` 49 requests, 49
    painted; `native_paint_frame` 31 draws; targets of 256 and 512). Dark marks on the struck back in both modes:
    natively at 90 s much of the back covered (`t35/paint-back-nat90-orig70.png`, left); in the original sparse patches low on the back at 75 s
    (`t35/paint-orig-75-80-back.png`, left; at 55 s a black streak over the back, a weak witness); at 70 s its upper back visible
    and nearly unmarked (`t35/paint-back-nat90-orig70.png`, right), at 80 s hidden. The coverage's difference is a lead, not a result. The two fights do not follow one another (the hits' moments and
    places differ: `t35/paint-sheet-nat.png`, `paint-sheet-orig.png`), so the blotches' colour and extent are not compared
    (D-0299); natively they read browner and more contrasted on a lighter frame, the original's darker and redder in a darker one
    (by eye, not measured). The Quit Game portraits show no paint in either mode (`t35/paint-portrait-nat-orig.png`).
- **Evidence**: `evidence/I-0140/t35/` (`series-t35.ps1`, `run-a.log`, `neckdiff.py`, `body.py`, the pictures named above; the
  modules `build-a` (run) and `build-b` to `build-d` (the reviews' fixes, not run), the logs `evidence/I-0140/t35-build-a.log`
  to `-d.log`, `t35/thread-affinity.log` (the checker on the final tree), the object cache `evidence/I-0140/t34-cache`); the runs `evidence/I-0066/artifacts/i66-native-i140-t35-face`,
  `i140-t35-thomas-nat`, `i140-t35-paint-nat` and `-orig`. The runs used `build-a` (`82839ffda`); the merged code adds the step
  reviews' fixes (`build-b`, `build-c`, `build-d`: 178 PASS each), which change no drawing.
- **Final review** (fresh Opus, xhigh, `main...6cb8ecfa4`): no High. M1 (STUDY-ALL-GAPS still said the paint never witnessed)
  fixed: its line and row 25. M2 (the test could not catch a wrong or ignored scale or a wrong depth denominator) fixed in part
  (`202f86308`): the second pass at scale 1.5 (the taps at floor(centre + tap * scale)), the moved pixel beside the shadow's edge;
  a denominator of z² for z0² stays within one step at these depths (left). L1 (the Read bullet cited the main view's PCF; six
  passes the native's count) fixed. L2 (STATE and the index said merged before the merge) fixed. L3 (the module run, the
  affinity log, the skin script, shot 1's neck worse) fixed in the text. L4 (the blur target made for a context whose record
  gives no blur; the cap at 15 silent) left: menu views only, at their own size (784 or 560), no record seen above 8 passes.
  L5 (four redundant fills in six passes, as the original draws) left. L6 (the paint worded beyond its picture) fixed.
  The test fix's step review (`6cb8ecfa4..202f86308`): no High or Medium (an ignored scale off by 31 steps, the depth weight
  forced to 1 or 0 by 17 and 38: caught); L1, L2 (comments) fixed (`fc3082ee3`). Integrity review (`main...f47d727f7`): no High.
  M1 (the Status body's pixel difference computed and left out, worse than before) fixed: given above, its cause not established.
  L1 (the neck's shot 0 left out) fixed: given. L2 (the original's 70 s back visible and nearly unmarked; 55 s weak) fixed. L3
  (build-d's PASS count written before it finished: true at 01:40) noted. L4 (the affinity PASS without a log) fixed: rerun, kept.
- **Merged** `2cac4cf17` (2026-11-22 01:48); the launcher rebuilt (`evidence/I-0140/t35-launcher-build.log`, 178 PASS); start
  check passed, restored (01:52; `evidence/I-0140/t35-startcheck.log`); disk rotation: nothing older than 48 h.

## Tranche 34: the title per Thomas (option 3, the smoke, the rules); the menu self-shadow's held frames and neutral renames counted

Branch `claude/e-i140-t34` (from `main` `1bcf33c6f`, 2026-11-21; continued from 23:10 in worktree `adoring-chatterjee-361aca`). Tranche
31's L7, kept open by tranche 32's final review (M1): no journal row showed a Player view's held frames or its light views renamed to
the neutral, so the Status reopen run could not say whether those paths ran. With it, Thomas's answer to tranche 33 (below).

- **The title (`49aa2aa00`).** Option 3: the screen layer's strength `clamp(t/2) * presence` again (tranche 33's `(1 - dimq)`
  removed): the ring, embers and smoke over a page at 0.3, as before tranche 33; the tint stays off under a page (tranche 32); the
  layer dropped only while its strength is 0. The smoke: the cloud's density takes 2, 4 and 8 cycles (the 16-cycle octave, 8 texels a
  cycle, and the rim, the density's gradient, which lit every wisp's border, drew the streaks seen on
  `i140-t33-thomas-nat/title-after-notices.png` with its levels raised x5); the rim is now a band at the cloud's outer edge (the
  density blurred over 9 texels near 0.14, uneven by a 5-cycle noise). Computed offline on a copy of the code (`t34/clouds-offline.py`,
  standard library): the two images' mean coverage 0.20 and 0.20 against 0.23 and 0.17 before (before the border fade below: 0.19 and 0.17); `t34/cloud-old-new.png` (one bank,
  body and rim composed, before and after). The rules: one quad a side sampling a 256x32 image (a 2 % fade-in, then (1 - u)^0.85 to
  0 along; a Gaussian 1.5 units wide at half height across, in 8 units), centred where the 1.6-unit bars were; they were five plain
  quads at alpha 1 to 0.2, 2.4 px tall at 1080 lines (steps and hard rows: `t34/rule-t33-x4.png`).

- **The code (`7273b103b`).** Two relaxed counters in the renderer, `menu_shadow_held_` (BindSelfShadowViews takes the frame as
  held, the view having a mask) and `menu_shadow_neutral_` (a successful rename of the view's light views to the neutral), printed
  as `held` and `neutral` in `native_menu_self_shadow_renderer` (its buffer 448 bytes).
- **Step review** (fresh Opus, high; `1bcf33c6f..7273b103b`), to answer before the build that runs: no High. M1, the row prints only
  on traced walks 1 to 4 and every 600th, while `held` and `neutral` move mostly when tracing has stopped (a failure, rays off, the
  counts after the last 600th walk lost): print the row also when either changed since the last row. L1, a held frame counted
  even when the rename back to the mask is refused (rare): count after the try or say so. L2, both count events, not frames or
  views (several recordings of one context a frame; a view renamed at each reopening counts each time): reword the comments. L3,
  on a held frame the mask is cleared white (EncodeNativeSelfShadow), so `held` means « unshadowed, its descriptors kept », not a
  stale shadow shown: say it where the row is read.
- **The counters' fixes (`49aa2aa00`).** M1: the row also written when `held` or `neutral` changed since the last row, at most once
  a second, on a collect with no traced walk (`walk` 0; while views are traced, the periodic rows alone). L1: a held recording
  counted only once its views name the mask (not on a refused rename). L2: the comments say both count events. L3 (a held frame's
  mask cleared white: `held` means « unshadowed, its descriptors kept ») is how the run's rows are read below.
- **Step review of `49aa2aa00`** (fresh Opus, high; `868a539b5..49aa2aa00`, the title and the counters): no High or Medium. L1, the
  moved row also fired once a second while views were traced (an untraced recording of a traced view's size holds its mask): the
  comparison now only on a collect with no traced walk. L2, the rule's first texel at 10 % of its peak (a small hard inner end):
  its first texel now 0. L3, its mean along 0.53, not 0.54: corrected. L4, the block's three new statics missing from
  `scripts/thread-affinity-allow.json` (the checker reads only a declaration's first name): added. All in `f1d53c7a1`.
- **Step review of `f1d53c7a1`** (fresh Opus, high; `49aa2aa00..f1d53c7a1`): no High or Medium; the three criteria held. L1, the three
  allow-list entries invisible to the checker (it reported them STALE: a later `-Update` would have dropped them): each static now
  its own declaration (`Test-ThreadAffinity` reports no STALE row for this function); the module unchanged in behaviour, so the run
  took `build-c`.
- **The run (`build-c` of `f1d53c7a1`, 178 PASS; 23:32 to 23:37, `i140-t34-thomas-nat`, native only, the ASUS window 1920x1080,
  Thomas's character's copy restored by hash, 0 fault).** The Status page opened, left and reopened three times (each with its movie
  row, its shots +4.6, 8.6 and 12.6 s), then Quit Game and the Load Game list. The renderer's rows (OBSERVED): walks 1 to 4, 600,
  1200, 1800 and 2400, traced 0 to 2399 (one or two walks behind), past_slots, trace_refused, `held` and `neutral` 0 in every row, and no
  row written after tracing stopped (so neither count moved then). So in these three reopenings no recording held a mask untraced
  and no view's views were renamed to the neutral: every collect of a Player view traced it (traced about equal to walks), and a
  closed page's views are not recorded again (INFERRED from the code: the rename runs only in a recording). Tranche 31's L7 is
  answered for this path: the held and neutral paths did not run here; they stay for a failure or the rays off, not witnessed.
  The title (OBSERVED on the stills, `t34/`): `rules-t33-t34-x4.png` (the rules before and after, x4: the five steps and hard rows
  gone), `smoke-t33-t34-x5.png` (the title's lower half, levels x5: the wisps' streaks gone; the banks' straight cut, the final review's M1), `title-t34.png`. The
  Load Game list (`loadlist-t32-t34.png`): the ring, embers and smoke back over the page as in tranche 32 (MEASURED, the panels
  910x130 at (600, 188 + 153 k), mean RGB of the first shots: rows 1 to 3 and 5 within 0.6 % of tranche 32's in every channel, row 4 R +0.8 % and G +0.7 %
  (the embers' and smoke's places differ), so rows 2 to 5 again R +1 to 3 % against the original `i140-t30b-thomas-orig`, as Thomas chose).
- **Reviews at the merge.** Final review (fresh Opus, xhigh; `main...86d8ff08a`): no High. M1, the new rim reached the cloud
  image's border (the density still 0.11 there, the band up to 0.49), so every bank's orange band ended on its quad's straight edge,
  seen at levels x5 in this run's `title.png` and in `smoke-t33-t34-x5.png` (which the first wording of the run's bullet read as
  « softer »): both images now faded over their 16 border texels by a smoothstep, after the rim is computed (border texels under
  0.002, means 0.19 and 0.17; `clouds-offline.py` now prints them), versions 3 (`7f2beb578`), the stills shot again (below). M2,
  the RESULT's head still told tranches 32 and 33 as current: rewritten (« Tranches 32 to 34 »). L1, the tint's comment still
  « provisionally »: it states Thomas's answer. L2, STATE's rows and its « Updated » line: the R parts taken from main at the merges,
  the line rewritten at the checkpoint. L3, the index row said 34 merged before the merge: left, true once merged. L4, row 4 left
  out of the comparison with tranche 32 and the walks' lag (two at 600 to 1800): both said above. L5, the counts after the last
  600th walk still unwritten when held and neutral do not move: a row on the first collect with no traced walk after one.
  The fix's step review (fresh Opus, high; `bc5b971bb..7f2beb578`): no High or Medium. L1, the square fade leaves a soft straight
  band where a bank reaches its border (the rim's 0.2 contour 5 to 6 texels in over 36 texels of the right side; low alpha, no hard
  cut): left, to judge on the stills; a round window if it shows. L2, the comment's figures not reproducible from the stored script:
  the script now applies the fade. L3, the `ended` row not rate-limited (a flip between traced and untraced collects each frame
  would write a row each time; no such flip found in the code): left.
- **The second run (`build-e` of `7f2beb578`, 178 PASS; 00:02 to 00:08, `i140-t34e-thomas-nat`, the same series and protocol, 0
  fault, restored by hash).** The smoke (`t34/smoke-t33-t34e-x5.png`, levels x5, tranche 33 over this run; `title-t34e.png`): no
  wisp streaks and no straight band at the banks' edges seen (OBSERVED, one still; the fix review's L1 band not seen there). The
  renderer's rows: as in the first run (held and neutral 0), plus a row at each of the three Status visits' ends (`walk` 0:
  traced 960, 1925 and 2893).
  Its Load Game list (the same panels, first shots, against `i140-t30b-thomas-orig`): R +1.7, +3.0, +0.6, +1.2 and +1.9 % on rows 1
  to 5 (MEASURED by the integrity review; the second shots +0.3 to 3 % on rows 2 to 5).
- **Integrity review** (`main...7614a0569`, with this section): no High or Medium; the judge untouched but for the allow-list's
  four entries (stated above), no special case, no work cut, the numbers reproduced. L1, the Load Game figure of the merged code was
  the first build's: restated from the second run (above, STATE). L2, `held` and `neutral` never seen above 0: their increments are
  checked by reading the code only, until a forced case (a failure, the rays off) shows a count; « the paths did not run » rests on
  that. L3, the first coverage figures were before the border fade: said.
- **Merged** `d89de1e87` (2026-11-22 00:13); the launcher rebuilt (`evidence/I-0140/t34-launcher-build.log`, 178 PASS); start
  check passed, restored (00:21; `evidence/I-0140/t34-startcheck.log`); disk rotation: nothing older than 48 h.
- **Thomas's eye.** The stills (`t34/title-t34e.png`, `smoke-t33-t34e-x5.png`, `rules-t33-t34-x4.png`) accepted (« Bon travail »,
  2026-11-22 02:40); option 2 later.

## Tranche 33: the title's ring and embers off under a page

Branch `claude/e-i140-t33` (from `main` `1a135adec`, 2026-11-21). Tranche 32's final review (its M2) found the title's second layer
over the Load Game page: the screen layer (the ring, the sword sign, the embers and the smoke, a screen blend after DS3's whole UI),
kept under a page at `presence` 0.3 (Thomas's look, I-0132 R12e), reddening rows 1 and 2 (x(1.03, 1.015, 1.00)) with the ring's arc
across them. Thomas was asked (thread, 21:32): 1 both layers off under a page (recommended), 2 both drawn under the page (the
movies' draw order to read first), 3 the tint off and the embers kept; the coordinator: option 1 applied until he answers (21:36).

- **The code (`b093c181b`, comments `b88376dbe`).** The screen layer's strength `clamp(t/2) * presence * (1 - dimq)` (was
  `clamp(t/2) * presence`): with no page the same, frame for frame, box or not, once a page's hold is over; under a fully eased
  page 0 and no layer set (the pass draws nothing; the journal's `screen_frames` 0 of 600 under the list, 600 of 600 in tranche 32's
  run); after a page closes the layer stays off for the 1.5 s the page's dim is held (`kCoverHoldMs`: no rise in the Load Game
  list's confirm-to-loading gap), as the word already did and the tint does since tranche 32, then eases back (before, the ring
  and embers stayed at 0.3 then: the final review's M1); `title_fx::Build` still runs every frame, so the embers and smoke keep moving underneath. The fade is the
  product of `presence`'s ease to 0.3 and the page's (`dimq`, 0.2 s), a little faster than the tint's on opening, a little slower
  on closing (said in the comments).
- **Step review** (fresh Opus, high; `1a135adec..b093c181b`): no High or Medium. L1, the « dimmed to 0.3 » comment stale: it now
  says the 0.3 only shapes the screen layer's fade; L2, « with the page's ease » not exact (two eases multiplied): said; L4, the
  comment read as Thomas's request: « asked in the thread », and this record (`b88376dbe`). L3, a frame built and dropped each
  frame under a page (about eight quads at alpha 0, the image list shared; the embers' and smoke's loops already skip their
  draws): left, small, the simulation must run.
- **The run (`build-a` of `4fa783409`, `main` `72c9c6a67` merged; 22:04 to 22:07, `i140-t33-thomas-nat`, native only, Thomas's
  character's copy restored by hash, 0 fault).** One list, one run, against tranche 30's original `i140-t30b-thomas-orig` (MEASURED,
  both shots, 1500 px bands of 80 rows from x 200, the panels 910x130 at (600, 188 + 153 k), the faces 120x120 at (458, 200 + 153 k)):
  every band from y 400 to 1040 within 0.11 %, rows 2 to 5's panels byte for byte the original's and their faces within 0.7 % in
  every channel (the bottom's R x1.05 to x1.15 gone); row 1, the selected row, x0.98 to 1.03 in R with opposite signs in the two
  shots, its highlight pulsing in both games (its Y 52.6 and 53.8 in the original's two shots, 52.0 and 54.7 natively; OBSERVED);
  what row 1 keeps beyond the pulse is not explained (INFERRED: the pulse's phase; its play time also differs, 14:27:35 here
  against 14:28:33, this run reaching the list without the Status page tranche 30's and 32's runs opened); the ring gone
  (`loadlist-top-rows-t32-t33-orig.png`, contrast raised: tranche 32, 33, original).
- **Reviews at the merge.** Final review (fresh Opus, xhigh; `main...f5fac5d5e`): no High. M1, the title bare for 1.5 s after a
  page closes (the page's dim held), the records claiming « the same with no page »: said above and in the comment, and put to
  Thomas (the word was already off then; option 1 makes the whole title come back together). L1, the 0.3 also shapes the word's
  return after a page: the comment says so. L2, row 1 left out of the Conclusion and the measure credited to this tranche in the
  comment: both corrected. L3, STATE's « Last finished » and « Next action » stale or missing option 3: rewritten. L4, the word's
  overlay frame still drawn under a page (28 quads at alpha 0 a frame): left, for a later tranche.
  Integrity review (`main...6f65ba9f7`, with this section): no High or Medium; the judge untouched, no special case, the numbers
  reproduced. Low: row 1's R bound (0.98 to 1.03) and B bound (to 1.01), « within 0.6 % » exact only at 0.7 %, and row 1's play
  time and path (no Status page) unnamed: all corrected above.
- **The merge.** `f7c50133b` (22:25), the launcher rebuilt (178 PASS), the start check `start-f7c50133b` passed (restored); the
  disk rotation: nothing older than 48 h, 480 GB free. Tranche 32's: `1a135adec`, the same.
- **Thomas's answer (2026-11-21 23:07).** « Option 2 dans l'idéal mais aujourd'hui option 3 », perhaps to come back to: the tint off
  under a page (tranche 32 kept), the ring, embers and smoke back over the page at `presence` 0.3 as before this tranche (its code
  to revert). With it, two title effects to rework: the smoke with fewer streaks, and the two rules beside REKINDLED pixelated, to
  make smooth.
- **Left.** The revert (option 3), the smoke's streaks, the rules' smoothing. The word's frame dropped while nothing of it shows (L4).

## Tranche 32: the Load Game list's « third row » was the title's tint under a page

Branch `claude/e-i140-t32` (from `main` `1f87a0f02`, 2026-11-21). Tranche 30 left the Load Game list's third visible row warmer
and darker natively in every list (slots 2 and 5, so a ring of 3 was a lead) and tranche 31 a Status reopen run under the frame
graph's executor (its L7).

- **The cause (MEASURED on tranche 30's shots, `i140-t30b-thomas-{nat,orig}`, `i140-t30b-ringed-*`, `i140-t30-ringed-*`,
  `i140-t27b-delta-*`).** Not a row, not a portrait: a horizontal band of the screen, y about 400 to 610 at 1920x1080, the same
  place in every list. It covers the lower half of the second visible row and the third row whole, and tints everything there:
  the portraits (G x0.95 to 0.97, B x0.90 to 0.94), the panels (the third row's x(1.00, 0.92, 0.86); the second's x(1.03, 0.98,
  0.94)) and the background's « D » outside the list (x(0.98, 0.95, 0.90)); outside the band G and B within 2 % (R below). The faces are the same 128x128 views on every
  row (their key {1, 14}), so their rows' outcomes alike (tranche 30) fit. The band's place and colour are the title's tint layer
  (`16b_rekindled_overlay.inc` `BuildTitleScreen`: a multiply of warm quads over the logo, drawn after DS3's whole UI,
  `scene_pass.cpp` `RecordOverlay(... tint_layer_ ...)`), which Thomas kept at half strength under a page (2026-11-12 08:01):
  over a page it coloured the page itself (INFERRED from the band's place and colour; OBSERVED gone with the layer, below).
  Thomas was asked (thread, 21:00): option 1 (applied, reversible) the tint fades out under a page as the word does, the pages
  and portraits in the original's colours, the logo's edges still visible behind a page (« D », « III ») losing their 5 to 10 %
  of blue on a grey near 32/255; option 2 the tint drawn under the page (between the logo and the page), which needs the movies'
  draw order read in the decompilation first. Message boxes unchanged (the tint full under them, Thomas's 08:01).
- **The code (`0a1cc9b0b`).** `k = clamp(t/2) * (1 - dimq) * (1 - coverq)` (was `(1 - 0.5 dimq)`); the burn off under a box
  (`1 - boxq`) and through k under a page; no layer at all when k is 0 (`SetTintLayer(nullptr)`: the pass draws nothing, its
  textures kept). The frame graph's path (`scene_pass_frame.cpp`) draws no tint layer (the world's frames), so nothing there.
  `Run-AlphaCast -StatusViewTimes n` (1 to 4) opens the Status page n times (`status`, `status2`... shots).
- **Step review** (fresh Opus, high; `1f87a0f02..0a1cc9b0b`): no High. M1, a reopening without its movie row was timed from its
  click after the 15 s wait, so its three shots came back to back: now timed from the wait's end, said not confirmed open and not
  for a comparison, and such a pass ends the reopenings (L1: its Esc could close the menu instead); L2 the shot lines name their
  origin; L3 `-StatusViewTimes` without `-StatusView` throws; L4 the header limited to the first opening (`3fc66486d`). L5, the
  comment replaces Thomas's half strength on a question, not his answer: left until he answers (the comment then names it).
- **The run (`build-a`: compiled from the working tree at 21:02 whose native code was committed as `0a1cc9b0b` at 21:03, the later
  native changes comments only; 21:10 to 21:15, `i140-t32-thomas-nat`, native only, Thomas's character's copy restored by hash, 0
  fault).** One list (Thomas's character's), one run: the Load Game list after Quit Game against tranche 30's original
  `i140-t30b-thomas-orig` (MEASURED, `loadlist-after-quit-1.png` of both, 1500 px wide bands of 80 rows from x 200): y 400 to 640
  now x(1.006 to 1.014, 1.003 to 1.008, 0.999 to 1.002) against x(1.003 to 1.013, 0.92 to 0.96, 0.85 to 0.92) before (the second
  shots: R up to 1.016); the faces (120x120 at x 458, y 200 + 153 k) of the five rows within 0.7 % in every channel on the first
  shots (the integrity review's own crops: row 1, the selected row under the ring, R 0.99 to 1.014 over both shots; row 5 B
  1.008) (rows 2 and 3 were G x0.95 to 0.97, B x0.90 to 0.94); `loadlist-band-before-after-orig.png` (before, after, original). Left as it was, the same in both builds (the final
  review's M2, MEASURED): rows 1 and 2's panels redder, x(1.026 to 1.030, 1.014 to 1.017, 1.00), with an orange ring arc and
  streaks across them that the original has not, and the screen's bottom (y 880 to 1040, R x1.05 to x1.15, B within 2 %): the
  title's screen layer (the ring, embers and smoke, a screen blend after DS3's UI), kept under a page at the title's presence
  (Thomas's look, I-0132 R12e; INFERRED from the ring's shape). Put to Thomas with the tint (option 2 would cover both).
- **The Status page reopened under the executor (tranche 31's L7, in part; OBSERVED).** Two openings, each with its movie row (+4.66, 8.66,
  12.66 s and +4.66, 8.65, 12.73 s); the body box 540x600 at (1300, 200) Y 37.36 and 37.39 at the two `switch` shots (tranche 31's
  native 37.70, original 35.86; the first full run of the merged code on that page); the renderer's rows: views traced, none past
  the pairs, none refused, no failure; `native_graph_shadow` executor on, its summaries with no untracked transition. The held and
  neutral frames are not shown to have run (the final review's M1): the renderer's row counts no held frame and no neutral
  rewrite, the graph's 1927 `self_shadow` passes against about 1927 traced walks leave about none held, and the reopening's first
  collect was traced (INFERRED); left: a counter of both, then a run.
- **Reviews at the merge.** Final review (fresh Opus, xhigh; `main...e58d2b94e`): no High. M1, L7 closed without the held and
  neutral frames seen: reworded above, the item kept open. M2, the title's screen layer still reddens rows 1 and 2: measured and
  recorded above, put to Thomas. M3, a look Thomas chose (the tint half under a page, 08:01) changed before his answer: the
  Conclusion says the change is provisional; merged as a reversible default the coordinator applies until he answers (21:01). L1, a
  first opening without its row did not end the reopenings: it does now (`-StatusViewTimes` above 1 only). L2, the band's place in
  the code comment and the row of the panel figures: corrected. L3, STATE's « step 5 closed » and 8d's lost detail: restored. L4,
  tranche 30's region-IBL tally (09_model_admission, 16_offscreen_and_movies) kept: it witnesses the region IBL's outcomes per
  view, which remain true and cheap (its rows behind `light-observe.flag`); tranche 30's « cause still not found » now points here.
  The fixes' step review (fresh Opus, high; `e58d2b94e..21ebc50ab`, the code): no High or Medium. L1, the comment's band to
  640 px (the comparison bands' end): the measured 610, the comment rewrapped; L2, the block's comment names a reopening's line;
  L3, no stop line once the game is gone. All three fixed. Integrity review (`main...21ebc50ab`, with this section): no High or
  Medium; the judge untouched, no special case, no work cut, the numbers reproduced. Low: the « within » figures exceeded in its own
  crops (the second shots, row 1 under the ring) and the after-measure one list, one run: both said above and in the Conclusion;
  the build's commit not provable from its log (built from the working tree 74 s before `0a1cc9b0b`): said above.
- **Left.** Thomas's answer (option 1 or 2, the screen layer with it; L5). The held and neutral frames' counter and run (L7). D-0303's leftovers (the penumbra, the hair's forward rows, the creation's view
  pairs sharing a table) and RankingBody.

## Tranche 31: the Player menu views' self-shadow by a traced ray (D-0303)

Branch `claude/e-i140-t31` (from `main` `dd5866ea1`, 2026-11-21). Thomas's « Choix 2 » (12:22): the character's own shadow on
itself in the Player menu views (the creation's ChrMakePreview and its Face Detail close-up, the Status body; RankingBody if met)
by a traced ray toward the view's ShadowParam {1, id} Angle (Shadow Light Id −1), close to the original, no shadow map; the Face and
Prof views unshadowed; the rest of their lighting raster (D-0301).

- **What the code does (step 1, `0b47deb38`).** A view is a Player one when D64C30's pipeline descriptor (argument 5) has flags
  (+8) bit 0 set (the Player family's 0x400852201F; the Face and Prof families' 0x8422014 have none; READ, D65DD0 builds it):
  `NativeOffscreenSceneGraph` keeps it on the view's proxy. With the rays on, the collect gathers the view's admitted candidates
  (`rays_also`, the three admission sites) into a traced scene of its own (`CollectNativeMenuRtScene`: the storage BLAS of the
  static parts, the skinned ones through the rays' skinning and materials, deduplicated by model and pose; the sun casters' mask
  unless suppressed), in caches of its own (skinned instances and material shards) evicted by the menu's own walk, so the main
  view's walk and caches are untouched. Its shadow state (`CollectNativeMenuShadowState`): the bank's ShadowParam record of the
  view's key, blended as 88A10 does with A = B, the sun from `ShadowSunDirection` (the record's Angle), the scene words for
  FC_ShadowColor, gated by the Shadow Quality row's shadows-on byte. The prepare gives such a frame the traced sun's blocks (13b).
  The renderer (`RenderNativeOffscreen`) builds the view's top level and material rows on the frame's list
  (`EncodeMenuRtScene`, its own `RtMaterialTable` and `RtTlas`; one traced view a frame, a second one counted and left
  unshadowed), traces the sun term into a mask at the view's extent (`EncodeNativeSelfShadow`: RtSunShadow's `fill`/`trace` by the
  G-buffer stencil bit 0x08, xyz white by the new `neutral` constant) that every light pass block's t32 names, and draws the
  shadowed sun (b8 from the view's own shadow scene). Any failure turns the menu self-shadow off for the session
  (`FailMenuShadow`), the view drawn unshadowed; `no-menu-shadow.flag` (a test tool) leaves every view as before.
- **The forward rows.** The face is a G-buffer row (`GXFlver_FgFaceSSSIbl_Bloody`, I-0119 GBUF-FAMILIES), so the deferred mask
  covers it; tranche 30's « the face is a forward row » was wrong. Only the hair and similar forward rows (0x1F, 0x29) stay
  unshadowed natively (the original draws them with `FwdSdw`/`FwdSSdw` by its forward window 0x710, 5A120); left (step 2 not
  made: the hair alike by eye in the Face Detail's three pictures, below).
- **The Status body, same moment (D-0299; `build-a`, 2026-11-21).** `status-switch` at the Status page's row + 12 598 ms (original,
  `i140-t31-thomas-orig`), + 12 615 ms (native, `i140-t31-thomas-nat`) and + 12 827 ms (the native control `no-menu-shadow`,
  `i140-t31-ctl-thomas-nat`); the body box 540x600 at (1300, 200): YAVG 35.86 original, 37.70 native, 39.77 control (tranche 30b's
  native 39.72): the traced self-shadow takes about 2.07 of the 3.91 the native was brighter (the control's pose differs, below; OBSERVED; `body-ctl-nat-orig.png`, control,
  native, original). The shadow falls where the original's does (the cape's left side, the arm's inside: seen on the picture; the control's character stands turned a little, its idle
  pose not the same at that moment).
- **The creation, first series (`build-a`).** c2..c4 at the creation's row + 10.6, 20.6, 30.6 s (native) and + 10.9, 20.7, 30.7 s
  (original): the pane 780x760 at (1076, 180) Y 35.59 natively (all three) against 33.93 to 33.96, the same as tranche 30b's
  native without the shadow (35.60 to 35.70): no change (OBSERVED; `creation-nat-orig.png`). The cause, from the rows: the creation
  records four Player views at 784x784 a frame (two of 22 candidates, two of 14; one camera), and step 1 traced one view a frame,
  the others left unshadowed (READ: `EncodeMenuRtScene`'s one-a-frame rule; which of the four the pane shows is not known). The
  `native_menu_self_shadow` rows, keyed by gparam id and width, flipped between the views' two candidate sets and spent their 64
  rows; now keyed by the view's holder.
- **The Face Detail, four series not reached.** No Appearance page row in 15 s, the face shots skipped as the pilot says.
  The creation's command list (`04_010_ChrMake_CommandList`) first advanced at + 34.4, + 55.0 and + 109.6 s of the creation's row
  (native; the original + 34.7 and + 54.5 s), each time 3.5 to 4 s after the pilot's title bar click and press, which it waited
  for 0, 20 and 75 s: the page opens on an input, not after a delay (OBSERVED), and that press is spent opening it. Then a
  second press at the same point (206, 410), at the page's row + 2 s, was not taken (`i140-t31d`, both modes; its control run timed out before New Game, `run-d-ctl.log`); the cursor moved
  to (222, 410), as tranche 30's runs did, it was, the Appearance page (`04_060_ChrMake_ChildCommandList`) 3.5 s later framing
  the face with no further press (the pilot, `Pilot-CharMakeWitness.ps1`, so now).
- **The Face Detail (`build-f`, `1d6906661` with `main` `9732c7521` merged, 2026-11-21 19:43).** face-0..3 at the Appearance
  page's row + 14.6, 20.6, 26.6, 32.6 s in the three runs (within 0.06 s; `i140-t31e`): the skin (640x640 at (1150, 200),
  tranche 30's `skin-stats.py`) natively mean 118.7 to 125.4, p10 64 to 69, lower quartile 76 to 88, median 130 to 139; original
  117.3 to 122.4, 64 to 68, 76 to 84, 127 to 134; the control (no-menu-shadow) 125.7 to 130.4, 73 to 76, 99 to 109, 132 to 138
  (tranche 30b's native the same): the darks now the original's, the mean within 1.3 to 3.0 (MEASURED). The shadow falls on the
  shoulder and the neck under the jaw as the original's, but with a hard, stepped edge where the original's is soft
  (`face-ctl-nat-orig.png`, control, native, original; OBSERVED): the ray is sharp (no penumbra: the menu views have no Ray
  Reconstruction); left. The renderer's rows: 4 views traced a frame, none past the pairs, none refused, no failure, no
  `graph_barrier_mismatch`. The original's runs (`i140-t31d`, `i140-t31e`) closed on its known close fault (0xC0000005) after
  their shots, as `build-d`'s.
- **Step 1's review answered (`00bbd030f`) and every Player view traced (`1551dec92`).** The menu walk's programme failures in
  records of their own; the menu caches emptied once no collect traces a view (now 2.5 s after the last that did) and at a reset, and
  each menu table swept on the main frames no view took it; a context's t32 the mask on its traced frames only (the neutral white
  otherwise, no pass); a view traced only with the shadowed sun's programme; the self-shadow depth view released when the depth is
  made again; the renderer's failure and counts in `native_menu_self_shadow_renderer`; the menu collect in the thread-affinity
  checker's scope. Then up to four views a frame traced, each with a material table and top level of its own (`kMenuRtSlots`), a
  fifth counted and left unshadowed; `native_menu_self_shadow`'s change test by holder and its no_storage by kind.
- **Step reviews.** Step 1 (`cf155e452..186be5858`, fresh Opus, high): no High; M1 (the menu walk's programme failures in the main
  walk's records, their `until` in another walk's numbers), M2 (the menu caches and table never emptied once the menus close),
  L1 to L7 (L1 stale comments, L2 the renderer's failure and counts never written, L3 t32 the mask on untraced frames, L4 a mask traced without the
  shadowed sun's programme, L5 the depth view kept across a depth's remake, L6 the menu collect outside the thread-affinity
  checker, L7 the retry policy unsaid): all fixed in `00bbd030f` (L7 said in the comment). Its fixes (`186be5858..00bbd030f`):
  H1, one traced Player view a frame where the creation records four, the comment's « no Player view shares a frame » unsupported
  (fixed by `1551dec92`, already written); M1, a context whose views alternate traced and untraced remade its shader-visible heap
  twice a frame; L1 the caches emptied at one skipped collect; L2 the menu table never swept in the creation (no main view) and a
  view with no instance taking the frame's pair; L3 the collect building a traced scene the renderer cannot use without the
  programme; L4 `ibl_rows_left` and the thread_local unreviewed; L5 a heap failure faulting the renderer: all fixed in
  `c75acf8a8` (M1 by holding the mask 120 frames after its last trace, cleared white; L4's thread_local reviewed at its
  declaration, the checker reading function bodies only).
- **The creation, all four views traced (`build-d`, `c75acf8a8`, 2026-11-21 15:54 and 16:14).** c2..c4 at the creation's row
  + 10.6, 20.6, 30.6 s in every run (the native, the original and the control within 0.4 s): the pane 780x760 at (1076, 180) Y
  34.79 to 34.81 natively, 33.93 to 33.94 original, 35.59 for the control (`no-menu-shadow`, as tranche 30b's 35.60 to 35.70): the
  traced self-shadow takes 0.78 of the 1.65 the native was brighter, as on the Status body (OBSERVED; series `i140-t31b`, and
  `i140-t31c` the same to 0.02; `creation-ctl-nat-orig-b.png`, control, native, original: the shield arm's inside and the legs'
  shaded as the original's). Its rows: each of the four views traced on every frame (`native_menu_self_shadow_renderer`
  `traced` 16 795 at walk 4 200, none past the four pairs, none refused, no failure); the 22-candidate views make 1 placed and
  8 skinned instances, the 14-candidate ones 7 skinned; in both the 8 (or 7) other candidates have no storage for want of a
  receiver at the model's +0xE8 (`no_storage_kinds` [8, 0, 0, 0]), as many as the skinned parts (that they are each part's
  second draw is INFERRED, not read). The original's run closed on DS3's own fault at a window close during the creation
  (D12759 through the body-scale witness, I-0137, which the Reference mode keeps); its shots were taken before.
- **The last step review** (`00bbd030f..c75acf8a8`, fresh Opus, high): no High, no Medium. L1, the caches' clear counted in
  collects (about 0.8 s at 144 frames a second, shorter than an unplaced view's 2 s skip); L2, a refused t32 rewrite at a hold's
  end leaving t32 on a stale mask the view's graph had not declared; L3, two allow-list reasons out of date; L4, the renderer row's
  `second` and `cleared` not saying what they count; L5, comments the code no longer supports; L6, the creation's two pairs of
  views (the same candidates) each building its own table and top level (a lead: to share one pair between views of one candidate
  set and camera); L7, the never-taken tables swept at start. L1 to L5 and L7 fixed in `ecff63be8` (L1 by time, 2.5 s; L2 the
  mirror's slot 10 put back and a still-named mask cleared white, t32 and the graph reading one `SelfMaskBound`; L4 the fields
  `past_slots` and `trace_refused`); L6 left (a menu cost, not measured).
- **The reviews of those fixes.** `69c2e1656..ecff63be8`: H1, with the frame graph's executor on (D-0171) a plan keyed by its
  declarations and its passes compared to the plan: declaring the mask on « its views name it » while the pass ran only on traced
  and held frames, a hold run out (or a menu reopened untraced) ran a frame without the cached plan's `self_shadow` pass, a
  logic error faulting the renderer; M1 the same through the refused rewrite's returns; L1 to L4 (the mirror left disagreeing on
  a remade mirror, two comments, a pair given back swept as taken): fixed in `ebafd46df` (the pass on every declared frame). Its
  review (`ecff63be8..ebafd46df`): M1, one key for frames whose lighting reads the mask and frames whose does not (a hold run out
  whose rewrite to the neutral succeeds), so a cached plan could leave the mask a render target while the lighting samples it;
  L1 `SelfMaskBound`'s comment, L2 two comments, L3 a refused rewrite retried every frame: fixed in `43c39bc27`
  (`BindSelfShadowViews`: the t32 rewrite before the graph is declared, the frame's trace and the bound image deciding the
  declaration, the pass and t32 alike). Its review (`ecff63be8..43c39bc27`): no High, no Medium; L1 the mask kept and cleared
  every frame after any failure, L2 comments, L3 the traced frame written before a refused rewrite: fixed or said in `1d6906661`
  (nothing held once the menu self-shadow failed; a refused rewrite not tried again on its mirror), left to the final review.
- **Optimisation pass.** The traced scene, its caches and the trace run only while a Player menu view is drawn; play frames
  add the tests that skip them and, after a menu closes, the empty table encodes of `TickMenuRtMaterials` until its slots' idle
  sweep (up to 256 per slot); the menu cost not measured, the identical pairs' own tables (L6) left below. A view context's mask,
  its RTV and depth view live as long as the context (the G-buffer's keep-alive rule, I-0120 E1), the four top levels and table
  rows until the renderer closes.
- **Left.** The penumbra (a soft edge as the original's shadow map gives: a few jittered rays and a filter, or the shadow map's
  own softness read); the hair's forward rows unshadowed (step 2, not needed by the pictures so far); the creation's identical
  view pairs sharing one table and top level (a menu cost); a Status open, leave and reopen run under the executor (the held and
  neutral frames, READ only); RankingBody not met.
- **Reviews at the merge.** The final review (`main...1614561a8`, fresh Opus, xhigh): no High, no Medium. L1 the destructor's
  quarantine missing the mask and its RTV; L2 nothing released renderer-side at a menu's close (the mask, its depth slot, the top
  levels); L3 a depth slot released against the last submitted frame while the open list may still read it (two views of one
  extent with different stencil clears in one frame); L4 the Face Detail's id-10 sun not in a journal row; L5 the pilot's header
  stale; L6 records (the merge claimed before it, the Status body's build not said, the executor sentence unlabelled, the
  original's close faults in t31d and t31e not said, the optimisation bullet); L7 the held and neutral frames never run live; L8 the
  menu state not reset at `Close`; L9 the menu collect and encode copying the main walk's. L1, L5 and L8 fixed in `36c78162d`; L2,
  L6 said here and in the Conclusion (the merge's hash written at the checkpoint); L3 left (not met in the menus: one stencil clear
  per extent); L4 left (key 10 passed by the code, its sun not journalled); L7 left (above); L9 left (one helper for the main and
  menu walks, a refactor of its own). The integrity review (`main...76930e72a`, fresh Opus): no High, no Medium; L1 the Status
  page not run with the merged code, L2 the Status body's share confounded by the control's pose, L3 STATE's merge ahead of the
  fact, L4 three figures off the journals (55.0 s, 0.06 s, 1.3 to 3.0), L5 t31d's failed control run not said, L6 the series b to
  e's measures with no saved command, L7 the 20 thread-affinity entries not said (`CollectNativeOffscreenScenes`,
  `CollectNativeMenuSelfShadow`, a `CollectNativeYebisInputs` line: their reasons hold, the checker 0 missing). L1, L2, L4, L5, L7
  said here; L3 at the checkpoint; L6 left (every figure reproduced by the review with the same tools and crops). The fixes' step
  reviews (`1614561a8..36c78162d`, then `..e53b1aa0d`): no High, no Medium; the ready flag cleared at `Close` and never set again
  on a reopen (fixed, `e53b1aa0d`: it stays with the pipeline it reports), the pilot's comments (fixed, `3a2ebcd2f`, `3a2eb487f`);
  the lighting's device objects kept across a `Close` (older than this tranche; the renderer is not reopened in production) left.
- **The merge.** `6d372ccbe` (2026-11-21 20:38): the launcher rebuilt (178 PASS, 0 FAIL), the start check passed (20:47, ready in
  3.8 s, restored); the disk rotation found nothing older than 48 h (482 GB free).

## Tranche 30: the menu views left after tranche 28 compared in both modes; gap 3's Ringed City quit

Branch `claude/e-i140-t30` (from `main` `8ba5ded10`, 2026-11-21). The views left by tranche 28 (D-0301), each in both modes on the
ASUS window 1920x1080, offline, `light-observe` in both, save copies restored by hash; the module `evidence/I-0066/artifacts/build-t30`
(tranche 29's final build: its native code equals `main`'s). Runner and tools in `evidence/I-0140/t30/` (`series-t30.ps1`,
`Get-CropStats.ps1`, `skin-stats.py`); the measures in `crop-stats.txt`, `loadlist-face-stats.txt`, `skin-stats.txt`, `face-grid.txt`.

- **The Equipment page has no 3D view** (MENUS-MAP: in game only the Status screen draws the character); nothing to compare.
- **The first slot (10:27 to 10:54, `run.log`; six runs, every restoration ok, no native fault).** `i140-t30-thomas-{nat,orig}`
  (Thomas's character's copy, Continue: the Quit Game portrait, then the Status page with `-StatusView`'s first form, then
  `-QuitToLoadList`), `i140-t30-ringed-{nat,orig}` (Eta at Ringed City Streets, `eta-r11c-ringed`, then `-QuitToLoadList`),
  `i66-{native,reference}-i140-t30-face` (Run-PilotPair CharMake -FaceDetail -DeleteProfile on a copy of Thomas's save). The
  original's face run left by 0xC0000005 at the close during the creation (its known close fault, the menus sheet M10).
- **Status page (OBSERVED, not compared).** It opens in both modes (`02_070_Status` row in both); it shows « Check character
  overview » with no 3D model: the body view waits for the page's « Y :Switch », whose key was not pressed. Natively the 560x768 and
  512x256 views drew (`native_menu_env_regions`: drawn 2, and 1 or 2), and the original's `reference_region_select` for the menu
  pair {1, 2} selected 1 or 2 menu keys among 31 or 32 regions: the selection agrees. `-StatusView`'s two Esc after the page opened
  the menu again (Esc closes the whole menu: `i140-t30-thomas-nat/status-closed.png`), so `-QuitToLoadList` found no title in both
  thomas runs (« no title row after the quit »): the step review's H1 (below); Thomas's character's Load Game list not seen.
- **Quit Game portrait (MEASURED).** The pane 510x250 at (987, 437): YAVG 41.51 natively against 41.23, SATAVG 2.97 against 2.96,
  as in tranche 28.
- **Load Game portraits (MEASURED; INFERRED where said).** Natively the ten 128x128 views drew 10 regions (`drawn` 10), the original
  selects 10 menu keys for the pair {1, 2}. The face of each visible row (48x48 at x 496, y 230 + 153k): rows 0, 3 and 4 alike; the
  third visible row warmer and darker natively in every list: Zeta (`i140-t30-ringed`) Y 142.8 SAT 31.4 against 150.8 and 25.6,
  the same 4 s later (not a fade); in tranche 27's lists before the region pass, Gamma in both (`i140-t27b-delta`, `-thomas`: 140.6
  and 140.9, SAT 32.3 and 32.5, against 149.4 and 25.7). Row 1 mildly (148.3 / 26.9 against 149.2 / 25.4). Every view takes the
  same key {1, 14} (`native_offscreen_scene_cpu`); a different character on that row each time, so the view's place, not its
  profile (INFERRED). Older than tranche 28; cause not found (a lead below).
- **Gap 3 (OBSERVED, not its recorded case).** Quit Game from the Ringed City then the Load Game list: the five visible portraits
  present in both modes. The 480 s stay did not happen (`-StaySeconds` acts only after `-TravelPress`): each run lasted about 3 min.
- **Character creation (MEASURED).** The four 784x784 views (natively `drawn` 14, the original 14 regions). The pane 780x760 at
  (1076, 180): Y 35.6 to 35.7 natively against 33.9 to 34.1, SAT 1.37 to 1.39 against 1.28 to 1.30 (about 5 % brighter); by eye
  the native's armour has brighter highlights and deeper darks, the original's flatter (`creation-c3-zoom-native-left-original-right.png`).
- **Face Detail (MEASURED; the shots timed from the pilot's clicks, the pose the same by eye).** The pane +1.6 to +2.9 Y natively.
  The skin pixels of a 640x640 crop at (1150, 200) (R > G > B, R > 60, R − B > 25, as I-0113 tranche 34): mean Y equal (117.0 to
  122.8 against 117.2 to 121.4), the spread lower natively (sd 31.3 to 31.8 against 38.7 to 39.8), chroma equal. Where (8x8 cells of
  80 px, `face-grid.txt`): the face's cells 13 to 28 Y darker natively, the lower left shoulder 46 to 77 brighter; by eye the
  original's shoulder in a shadow the native lacks, and its face lit more (`facedetail-0-zoom-native-left-original-right.png`).
  The original's Player views (ChrMakePreview, StatusBody) draw a shadow of the character on itself (MENUS-MAP §48: an atlas,
  ShadowParam {1, id}'s Angle and cascades); natively the menu views were never shadowed (`16_offscreen_and_movies.inc`: « no
  shadows »; I-0131's R10-PLAN row 1, « never offscreen »). The shoulder is that missing shadow (INFERRED); the face's lesser light
  is not explained by it (a shadow only darkens the original).
- **The close-up's own key (READ; the fix `8acdc7c78`, seen live in the second slot, below).** The creation's four 784x784 views are built with MenuOffscrRendParam
  row 0's gparam id 11 (natively `native_offscreen_scene_cpu` 784 / 11, twice); the Face Detail switches them to the second preset,
  row 1, whose id is 10 (MENUS-MAP §21: the close-up's light, fog, env and tone; §48: its ShadowParam). In the original the switch
  FUN_140D6ABD0 calls D68260(view, row 1's +0x2C), which tail-jumps to D65410(holder = [view+0xB8], id), the re-key of the holder's
  composite scene +0x48 to {D27770(1), id} (88AF0); natively +0x48 is nil (NativeOffscreenSceneGraph), so D65410 wrote nothing and
  the close-up stayed lit by id 11: the face's lesser and differently placed light (INFERRED, the live check in the second slot).
  `NativeOffscreenRekey` now takes D65410 natively: the id goes to the view's proxy, which the collect reads each frame (light
  blocks, background, tone); its row `native_offscreen_view_rekey` (from, to, width).
- **The second slot (11:28 to about 12:25, `run-b.log`, `series-t30b.ps1`; the module `build-t30b` of `8acdc7c78`).**
  `i140-t30b-thomas-{nat,orig}` (Quit Game portrait, `-StatusView`, `-QuitToLoadList`), `i66-{native,reference}-i140-t30b-face`,
  `i140-t30b-cut-{nat,orig}` (Firelink's 40000040 by `witness-cutscene.txt`), `i140-t30b-ringed-{nat,orig}` (Eta, Firelink's
  bonfire, Travel to Ringed City Streets, 480 s, Quit Game, the Load Game list). Every restoration ok; the original's face run left
  by its known close fault (0xC0000005), after its shots.
- **Face Detail with the re-key (MEASURED, `skin-stats-t30b.txt`, `facedetail-t30b-0-native-left-original-right.png`).** Natively
  the four 784x784 views re-keyed from 11 to 10 (`native_offscreen_view_rekey`, four rows, `found`), and the close-up's tone row
  names key {1, 10}. The skin's highlights now the original's: upper quartile 153 to 155 against 153 to 155, p90 163 to 165
  against 163 to 164 (before: 138 to 148 and 157 to 161 natively); the median 132 to 137 against 127 to 133 (+4 to +5) and the mean
  125.2 to 129.8 against 117.1 to 121.3 (about +8), where before the re-key the mean was equal and the median 6 below (the final
  review's M1). The difference left is in the darks: lower quartile 98 to 107 against 76 to 81 (+22 to +26), the spread 32 to 34
  against 39 to 40; by eye the original's hair shadow
  on the forehead and the head's on the left shoulder, absent natively. The face's lesser light was the missing re-key (OBSERVED);
  the rest, the surplus in the mid-tones and darks included, is consistent with the Player views' missing shadow (INFERRED, a lead
  below). These shots are outside D-0299 (timed from the pilot's clicks and fixed waits, no clock value named; the final review's
  M2), as the creation's: tranche 31's comparison times them from the page's own row, as `-StatusView` does.
- **Status page (MEASURED).** `-StatusView` opened the page (row `02_070_Status`; shots at row + 4.6, 8.6 and 12.6 s by the capture
  ticks), the switch F showed the face and body views in both modes, one Esc closed the menu. The body view (540x600 at (1300, 200)
  of `status-switch.png`): Y 39.7 natively against 35.9, SAT 1.84 against 1.57 (about 11 % brighter); by eye the native's armour
  with brighter highlights and deeper contrast, the original's flatter, as at the creation (+5 %). The same Player family and the
  same missing self-shadow (INFERRED). The pairs on the module's tick (`run-b.log`): 44, 9 and 59 ms apart from the page's row
  (the body measure on the 59 ms pair, a pose at rest on a page; no tolerance was set beforehand: these are the offsets accepted).
- **Thomas's character's Load Game list (MEASURED, `t30b-thomas-stats.txt`).** Alpha's row (Thomas's character) now equal (Y 110.0
  against 108.9; tranche 27's 100.6 against 109.0 came before tranche 28's region pass); Gamma, on the third visible row, warmer and
  darker again (143.4 / SAT 31.6 against 149.4 / 25.7), the other rows within 1.6 Y. In both lists (Alpha to Epsilon here, Delta to
  Theta at Ringed City) the third visible row, slots 2 and 5: a view's place, not a profile (the 10 views have the same key {1, 14}).
  A view's region IBL is chosen (09_model_admission's NativeMenuRegionIbl) before the admission notes the model's classes
  (NoteNativeRegionClasses), so a view's first collect takes no cube (« memo »). The witness `native_offscreen_view_ibl`
  (`fc7ea266b`: each view's first three collects, its region-IBL outcomes by count; `i140-t30c-ibl-nat`, the third slot) answers
  no: every one of the ten 128x128 views is collected at least three frames running (about 16 ms apart, each view two frames after
  the one before), its first collect « memo » for every model, the second and third with the cube bound for 4 models and the class
  term refusing the others (3 of 7, 5 of 9 on Alpha's armour); the same outcome on every view but one with 4 models (2 bound).
  The region IBL is not what differs on the third row (INFERRED: the rows carry the order of first admission, not the list's slot,
  so which holder is the third row is inferred; holders 2 to 6 and 8 to 10 log the same outcomes, the final review's L4). Cause still not found here; found in
  tranche 32: the title's tint layer over the page.
- **The third slot (12:32 to 12:45, `run-c.log`, `series-t30c.ps1`, native only; the module `build-t30c` of `c9e2fa330`).**
  `i140-t30c-ibl-nat` (Thomas's character's copy, Quit Game, the Load Game list) and `i140-t30c-close-nat` (below); both
  restored, no fault.
- **The region registry closed in play (D-0298, Thomas's yes 2026-11-21; `c9e2fa330`, `test-region-close.flag`; OBSERVED).** Eta
  at Firelink: the registry closed 60 s after the HP read (`test_region_close` start and closed in the same millisecond, 12:37:57;
  `native_env_region_cpu_retired`), then the travel to Ringed City Streets (a map loaded with the registry closed: 8
  `native_env_region_cpu_request_answered_0` « closed », the rows' cap: which callers is not logged), 120 s there, Quit Game, the
  Load Game list with its portraits, the game's own exit (exit 0, no fault). The traced main view showed no step at the close
  (`pre-1-<n>.png`, 50 shots 12:37:43 to 12:39:03: the centred 1600x700 crop's YAVG 46.5 to 47.0 before and after,
  `t30/close-sweep-stats.txt`; an earlier « 44.1 to 44.5 » had no measure behind it, the integrity review's L1): a traced view takes no
  region pass (D-0262). Witnessed in play for the first time: the close itself and requests answered 0 after it; the other paths
  written for it (65320 answered 0, 652D0's lookups given the closed manager's default record) have no row and are not shown to
  have run; the exit's Close then found the registry gone (no second `native_env_region_cpu_retired`).
- **The act's blend 40DD0 in a cutscene (OBSERVED).** 40000040 played in both modes (`witness_cutscene` handed); the bank's
  readers by site (`env_bank_sites`): natively 416A0 at module+216F3A and 41990 at D0C8ED only, in the original 416A0 at 1A8786/8798,
  4E2EC/2FE, 67A64, 75B6D/7F and 25C47D, 41990 at 77114/713E and D0C8ED/C9C5. 40DD0's sites (the return addresses 41232, 41241, 41482, 41491, the
  rows' convention; the calls at 4122D, 4123C, 4147D, 4148C) in neither mode: this cutscene does not exercise the lead; tranche 29's lead stays open for an act that blends (not met).
- **Gap 3 with the stay (OBSERVED, not reproduced).** Eta arrived at Ringed City Streets (`arrive.png` 11:57:33), stayed 480 s
  (`stay.png` 12:05:36), quit: natively the five visible portraits present (`i140-t30b-ringed-nat/loadlist-after-quit-1.png`),
  the rows as in the first slot (the third, Zeta, 142.9 / SAT 31.5 against 150.8 / 25.6; the others within 1.2 Y). The recorded
  case followed a much longer session; 8 minutes do not bring it back. Left open, with the endurance runs.
- **`-StatusView`'s step review (fresh Opus, high, `8ba5ded10..857054a04`).** H1: two Esc after the Status page open the menu again
  (and `-MenuTour` had the same two Esc a page, so its Status and System pages were never opened after the first: its
  `tour-status.png`); M1: the printed offsets were scheduled waits from the poll, not capture times, the row's own tick unused;
  L1: no row and the shots still taken from the timeout; L2: no exit check in the wait and before each shot; L3: the icon at 388
  where LIVE-RUN-NOTES says 394. Answered in `2134e75be`: one Esc (and one a page in `-MenuTour`), the shots scheduled from the
  row's tick and Pilot-Game's shot line naming its capture tick (`tick=`, GetTickCount64 right after the screen copy), no row no
  shot, exit checks, 394, and `-StatusSwitchVk` (F by default, a guess) before a third shot.
- **Second step review (`857054a04..2134e75be`).** No High. M1: the switch key, untested, before the closing Esc, with nothing
  checking the menu closed; Lows: no exit check before `status-closed.png` (L1), an unbounded tick wait (L2), a game exit reported as
  « row not seen » (L3), any line naming the page counted, not only first-advance rows (L4), `-MenuTour`'s 323/388/518 against the
  notes' 329/394/525 (L5), the key unvalidated (L6), the printed offsets past 4/8/12 s by Pilot-Game's start (L7, said in the
  comment). Answered in `eede34b5d`: the switch key pressed back after its shot, a new `02_000_IngameTop` row within 3 s of the
  closing Esc answered by one more Esc, capped waits stopping on exit, first-advance rows only, the notes' icons, the key validated.
- **Third step review (`2134e75be..8acdc7c78`, the re-key and the script).** No High. M1: the reopen check relies on a row the
  module writes once per new movie address (a set capped at 256, never cleared), so a reused address, a full set or a row after
  3 s goes unseen; the comment did not say so. Lows: the re-key's comment on the target's clear value (L1: the UI's first import
  reads the proxy's current id; a view not tabled is cleared with the new background, a debug-layer notice) and on 88AF0's object
  and ShadowParam (L2); `-MenuTour` without exit checks (L3); `-StatusSwitchVk` open to mouse and Windows keys (L4). What held: D65410's
  only caller D68260 (from D6ABD0), its first bytes safe to patch, the native-only table, one leaf lock. Answered in `3dbdadb40`
  (comments and script; the module of `8acdc7c78` unchanged): the blind spots said, a letter key or 0, `-MenuTour`'s exit checks, the
  re-key's comment corrected. The witness keyed by address (M1's better fix) left: a native change for one script's check.
- **Fourth step review (`8e7f7c16d..c9e2fa330`, the IBL witness and the region close).** No High, no Medium. Lows: the witness's
  rows logged before the unplaced gate, so a collect in a view's non-finite wait counts zeros and a first collect only « memo »
  (L1); `ibl_seen` never forgetting a holder, a reused address getting no rows (L2); the five counts not a partition of `models`,
  refusals after the IBL choice counted (L3); the close's once-guard a plain static (L4); the close's thread inferred, not
  checked (L5); the manager's lock +0x2A0 left reset mid-play, safe only if every engine path taking it is a gated hook (L6,
  residual, unverified); the menu views keeping their {1, 10} cube after the close (L7); a close finding no registry logging
  « closed » (L8). Answered in `b51f33b3d`: the row gains the collect's non-finite count, unlisted holders forgotten with a
  running order and a 384-row cap, the counts' meaning in the comment, an atomic exchange, the start row naming the registry's
  presence and wrappers, the comment saying the menu views keep the cube; its own step review below. L1 and the third slot: its
  rows came before this fix, but the views' second and third collects had 4 models bound, so they were collects with placed
  models; the conclusion above stands. L5 left INFERRED (the start row names the thread; ENGINE-STEP-THREAD-MAP's lane 0). L6
  left a residual risk: the run went through a travel, 120 s of play, the Load Game list and the exit with no fault, which
  shows no unhooked caller met in that path, not that none exists (the Ghidra read of +0x2A0's takers not done: a test tool).
- **Fifth step review (`8ac8b1035..b51f33b3d`, those answers).** No High. M1: the witness's forgetting ran inside the views'
  loop, after its skips, so with no view listed (the Load Game list closed) nothing was forgotten and a holder reopened at the same
  address kept its old order and count. Lows: the witness on in every native session and its bookkeeping going on after its rows
  are spent (L1); the forgetting repeated per view (L2); the close's first tick a plain static beside the atomic guard (L3); the
  start row's wrappers read before the close, a request in flight still able to add one (L4); the thread claim citing a map that
  does not name 8DD2E0 (L5). Answered in `3430e9605`: the forgetting with the other per-holder tables before the views, every
  collect; nothing kept or done once the 384 rows are spent; the first tick an atomic compare-exchange (no early pass on a tick
  from another thread); the field named `wrappers_before`; the comment citing LEGACY-ROOT-INVENTORY-903 (8DF920, step 0x0E) and the
  map's lane 0. L1's flag left: a witness capped at 384 rows, as the module's other first-rows witnesses.
- **Sixth step review (`15a8d8b70..3430e9605`).** No High. M1: the witness keyed by the holder's address alone, so a holder re-armed
  at the same address before any collect sees it unlisted inherits the old order and count (the same in the older `waits` and
  `drawn`). Lows: the tally's reset and counts going on after the rows are spent (L1); `collect` and `holder_order` counting
  admitted collects, not listed ones (L2); « row 70 » a line of the thread map, not its frame table's DrawStep row 79 (L3);
  CloseNativeEnvRegions without a guard against a second concurrent close, the module's Close safe only because it comes later
  (L4, the tranche 28 code). Answered in the next commit: the witness keyed by holder and model (another model is a new holder),
  the comments saying admitted collects, first admission and the tally's cost, row 79. Left: `waits` and `drawn` by address
  (tranche 19's, not this tranche's idea); L4 (the run's close and its closed row in the same millisecond, the exit minutes later;
  a guard in the close is tranche 28's code to change in its own step). These code changes are covered by the final review.
- **Left for tranche 31 (D-0303, Thomas « Choix 2 », 12:22: the Player views' self-shadow by a traced ray).** What the code has
  today (a read of `native/` for this, file:line in the tranche's plan to come): no traced scene for a menu view (the rays' walk is
  the main view's, `12_scene_collect.inc` `rays_walk`; the frame's top level is built once, after the menu views are recorded,
  `scene_pass_frame.cpp` `EncodeRtScene`; `rt_tlas_`, the material table and rows, the mask target `mask_` and `rt_main_views_` are
  single members of the renderer); the menu views' lighting runs with no mask and no shadowed sun (`scene_pass_views.cpp`
  `RenderNativeOffscreen`: `EncodeNativeLighting` with `mask` and `sun` null, t32 the neutral white); forward rows (the face, the
  hair) are unshadowed natively in every view (`content/model_pass.cpp`: pass 0 `Fwd`, the original's `FwdSdw`/`FwdSSdw` not
  taken). So the tranche needs: a traced scene of the view's own models (their storage BLAS and skinned instances), its own top
  level and material rows, built before the view's lighting; a mask at the view's extent traced toward the ShadowParam direction
  {1, id} (MENUS-MAP §48: d = (cos X sin Y, −sin X, cos X cos Y) from its Angle) and read by the shadowed sun's pass; and the
  forward rows' shadow (the face is a forward row), by the original's screen-space variant `FwdSSdw` reading the same mask if its
  bindings allow. Compared with the original at the same moment (D-0299): the Status body's YAVG above, and the Face Detail's skin
  quartiles and the creation's pane measured again with shots timed from the page's own row (above they are outside D-0299).
- **Reviews at the merge.** The final review (fresh Opus, xhigh, `main...8b046144a`): no High. M1: the Face Detail's skin said
  « equal » / « matches » in this section, STATE and D-0303's premise, where only the upper quartile and p90 match (the median +4
  to +5, the mean +8, the lower quartile +22 to +26 natively): reworded in the four places. M2: the creation and Face Detail shots
  outside D-0299 (fixed waits after clicks, no clock value) and no tolerance stated for the Status pairs: both said above, the
  re-timed comparison left to tranche 31. M3: the witnesses' last three commits never built or run (the journals show
  `c9e2fa330`'s forms: no `nonfinite` field, a start row with `thread` only): the head built with its unit tests before the merge
  (its build line below, at the merge); the witnessed forms are `c9e2fa330`'s. Lows: stale « Next » 5, « to see live » and the switch key « unwitnessed » (L1,
  corrected); 40DD0's sites named by their calls where the rows key return addresses (L2, both named); the IBL witness on in every
  native session (L3: its rows now with `light-observe.flag` only, `f411997ae`); the third row's holder inferred, not carried (L4,
  labelled INFERRED); the region flag silent in Reference (L5: a refusal row, `f411997ae`). What held: the re-key against D65410's
  disassembly (its only caller D68260 from D6ABD0; +0x48 nil natively), the witness under the collect's lock, the close's guard
  and the launcher's refusal of `test-*` flags, the scripts' bounded waits, D-0303's records. The step review of those fixes
  (`8b046144a..f411997ae`): no High; M1 the flag silent in the original-probe mode, its hook not installed there; L1 a locked
  exchange each schedule call after the row: both answered in `8a7f5f0ae` (refused at setup as `test-death.flag`, the reviewer's
  own fix; a load before the exchange). The integrity review (`main...e1d6dae9e`): no weakened test, no one-scene case, no work
  cut in a measure, the sampled numbers matching. M1: the Conclusion's « not the region IBL (OBSERVED) » where the body says
  INFERRED (corrected, and STATE); M2: the head's build not recorded (the build line below); L1: the close's YAVG figure not
  reproducible (measured again and saved, above); L2: older records citing `-MenuTour`'s Status and System pages, never opened
  after the first (I-0139 `sheets/menus.md` M42 and its icon line, MENUS-NATIVE-GAPS's System row: marked); « 1.5 Y » was 1.6.
- **The final tree's build (`80a93f89e`, main `d3d5104c9` merged in; `build-t30d`, complete, `-Jobs 4`, `t30/build-t30d.log`).**
  Exit 0, 178 PASS lines, no failure; not run live (the start check after the merge is its first launch).
- **Merged** into `main` as `438717113` (2026-11-21 14:15); the launcher rebuilt (178 PASS, `t30/launcher-rebuild-438717113.log`);
  the start check passed (`t30/start-check-438717113.log`: passed, restored, no refusal); the disk rotation deleted nothing (no build
  folder older than 48 h, no raw frames older than 7 days; 489.5 GB free).

## Tranche 29: the env-map bank's callers, the engine's answers by call site (a witness, no fix)

Branch `claude/e-i140-t29` (from `main` `407d17330`, 2026-11-21), merged `d43a542b8` (10:00, main `6f60aed41` merged in first), the
launcher rebuilt (`build-launcher-d43a542b8.log`, 178 PASS lines), the start check passed (`start-d43a542b8`, 10:09 to 10:10,
restored); the disk rotation's dry run found nothing older than 48 h (its `-Apply` not run: deletions are Thomas's here). Thomas's yes (06:56, « Oui tu peux faire l'inventaire »). The
static list [ENV-BANK-CALLERS](ENV-BANK-CALLERS.md) (a Sonnet subagent's, unverified) read again by the pilot, then each caller's
answer witnessed in both modes.

- **Read again (READ, GhidraMCP and the executable's bytes).** 41990 is a two-level lookup with no fallback; 416A0 reads {-1, -1}
  as {1, 0}, returns the key's entry, else {1, 0}'s, else the bank's default +0x1A8 (id 0, scale 1.0, the name « EnvMap_», no
  cube). D0D060 and D0CA20, the region removal and add that tranche 28's final review left as a lead, are steps of the
  developer's model viewer (`ModelViewerInsStep_forSkybox`: `STEP_Init_forReleseEnvMap`, `STEP_Wait_forLoadResource`), so never
  in play (INFERRED from the classes). 6A490 is a debug window of the region manager (reached from a registered callback: the
  list's « no callers » was wrong). 1BF180 and 1C10C0 belong to `GXSeaWaveRenderObj` (the SeaWave group, whose `Enable Draw Wave`
  is 0 in every shipped file: STUDY-UNREAD-SETTINGS, not re-read). 40DD0 is the cutscene act's blend into the bank; natively it
  runs pass-through (NativeRemoBankWrite<3>), so with no file in the bank it would blend the default entries (INFERRED).
- **The witness (code; `light-observe.flag`, both modes).** 416A0 hooked in both modes, 41990 in the Reference (natively the native
  list installs `NativeEnvBankHasObserved` with the flag, `NativeEnvBankHas` without); per return site and reader, in one global
  table of atomics: the calls, those on the bank GXRS+0x42A8 itself, 41990's true answers from the engine's bank (natively also
  `given_yes`, the answer the game received), 416A0's answers (exact, {1, 0}'s, the default), the {-1, -1} keys, the scale range,
  the cubes, natively the native banks' hits, the first six keys; a row `env_bank_sites` at each site's first call and every 20 s
  (60 s after 30 rows, 200 at most) with the bank's address and its areas and ids. Its first form (`61c624be0`) kept calls in
  per-thread slots: the first run's native journal (`i140-t29-witness-{nat,orig}`, `build-t29a`, 08:47 to 08:52) had one row (the step review's M1, below), so that run proves no absence; the
  form run here is `7d1b78220`'s (`build-t29c`, 178 PASS lines; `build-t29b`, `48b01e841`'s, was never run).
- **The runs (OBSERVED; `i140-t29c-witness-{nat,orig}`, 09:13 to 09:18, Thomas's character's copy, Continue at Firelink Shrine,
  about 85 s in play each (about 2 min 10 s with the title), the Quit Game portrait; exit 0 both, restored by hash).** Natively 4 rows, in the original 16;
  neither table full, rows not exhausted, every call on the GXRS bank.

  | Site (reader) | Caller | Original | Natively |
  |---|---|---|---|
  | 1A8786, 1A8798 (416A0) | 1A86C0, the model arguments' commit | 667977 calls each: 41207 exact, 626770 {1, 0}'s; scale 1 to 1.5, a cube on every one; keys {0, -1}, {140000000, 405/401/202}, {1, 10} | never called |
  | 75B6D, 75B7F (416A0) | 75B30, the forward IBL cube bind | 93534 each: 54944 exact, 38590 {1, 0}'s; scale 1 to 1.5 | never called |
  | 67A64 (416A0) | 675B0, EnvMapRegion_Execute | 60195: 60167 exact, 28 {1, 0}'s; area 140000000's ids | never called |
  | 4E2EC, 4E2FE (416A0) | 4E2A0, a group's IBL keys | about 37246 each, all {1, 0}'s for the key {0, -1}, scale 1.5 | never called |
  | 77114, 7713E (41990) | 770C0 | 19548 each, never true (key {0, -1}): its REM fallback [param+0x178] (READ by the final review) | never called |
  | D0C8ED (41990) | D0C2F0, the menu region add | 11, all true ({1, 10}) | 11: the engine's bank false on all, the game given true on all (the native banks, D-0301) |
  | D0C9C5 (41990) | D0C990, its release | 11, all true | never called (D0C990 replaced) |
  | module+21677A (416A0) | NativeEnvMapCube | not called | 2672, all the default entry (scale 1, no cube); the native banks hold every key |
  | module+25BACD (416A0) | ObserveForwardIblBind (a Reference witness) | 12, exact | not hooked |

  The engine's bank in the original: 7 areas (1: 8 ids, 15, -1, 0, 10 to 14; 140000000: 39 ids; 2, 3, 5, 6, 200000000), keys
  +0xD8 and +0xE0 {0, -1}; natively empty (0 areas), the same keys. No call to either reader from D0D060, 6A490, 1BF180,
  1C10C0 or 40DD0 in either mode on this path (no cutscene, no sea); D0CA20 calls neither reader (READ): natively its admission is
  absent from tranche 28's rows (`set_region` false on all 47), in the original not witnessed.
- **Reading (INFERRED).** On this path the native leaves to the engine's bank only its own NativeEnvMapCube (which names the
  key's own cube when 416A0 gives the default; its scale 1.0 is read by no draw here: the region snapshot's cube scale has no
  reader, and the menu view's world regions are refused by their masks) and D0C2F0's gate, which tranche 28 answers from the native banks. Every
  other reader of the bank is the original's raster or model path, which the native does not run here; their scales (1.5 for
  the groups' own keys {0, -1}) are reproduced by NativeEnvMapScale (tranche 14 of I-0113). 770C0's two false answers are the
  original's too, so its REM fallback is the original's choice. Left unseen: 40DD0 in a cutscene (natively its blended node would
  hold the default's name and cube, which NativeEnvMapCube reads as the default; to witness with I-0141's acts), 1A86C0 from a
  context the native does not own (the effect model renderer's, none in these 85 s), and the sea waves (inert by data).
- **Step reviews** (fresh Opus, high, three). **First (`407d17330..61c624be0`, fresh Opus).** M1: calls held in a thread's own slots were reported only when that thread
  reached 256 calls or 32 sites, so a sparse thread's calls (and a thread's last ones) never reached a row; a site absent from the
  rows proved nothing. Confirmed by the first run (`i140-t29-witness-nat`: one row only). Lows: 0 printed where a count does not
  apply and non-finite scales able to break the JSON (L1); {-1, -1} counted exact and the native banks asked the raw key (L2);
  anything outside the game image labelled « module » (L3); the row not naming its bank (L4); a per-call flag test natively and
  4.3 KB of TLS (L5); comments counting keys for calls (L6); exceptions possible in a noexcept Note (L7); new statics outside the
  thread-affinity allow-list (L8); small races (L9). Answered in `48b01e841`: one global table of atomics, rows from any call,
  nulls, minus_one, site kinds bounded by each module's SizeOfImage, the bank's address, the observed native 41990 chosen at
  install time, try blocks; L8 below.
- **Second (`61c624be0..48b01e841`).** M1: the kind « other » (bit 63) was shifted out with the reader's bit, so an address outside
  both modules read as a game RVA; M2: rows only at 20 s intervals and 60 at most, so a site first called in a run's last 20 s,
  or after row 60, was in no row; M3: natively 41990's « yes » was the answer the game received, which inside D0C2F0's scope adds
  the native banks' (NativeEnvBankAnswer), not the engine bank's. Lows: area_count 0 when unreadable (L1), a range able to print
  inf under the memory model (L2), no per-site sign of which table was passed (L3), the TLS cache outside the allow-list on a
  prepare path (L4). Answered in `7d1b78220`: the kind in bits 60-61, a row at each site's first call, rows every 20 s then 60 s
  (200 at most, the last marked), yes the engine's and given_yes the native's answer, area_count null, the range checked finite
  with release/acquire, on_gxrs_bank per site.
- **Third (`48b01e841..7d1b78220`).** No High, no Medium. Lows: new-site rows share the 200-row budget with interval rows (L1) and a
  full table near the end is in no row (L2): left, a run of this tranche lasts minutes (at most about 25 sites seen, rows far
  below 200); the last row's mark found by its flag, not its place, and absent from a failure row (L3): left, no failure row in
  the runs; on_gxrs_bank counted as « not the bank » when GXRS is unreadable (L4) and an unreadable key recorded as {-1, -1}
  (L5): left, neither met (on_gxrs_bank equals calls and minus_one is 0 on every site of both runs); the header comment's cadence wording (L6): fixed after the build, a comment
  only; the TLS cache outside the allow-list (L7, the second review's L4, the first's L8): left, `Test-ThreadAffinity` scans the
  listed render-worker and prepare functions only and an entry for Note would read STALE; the cache is NativeEngineFogAnswer's
  pattern (a weak_ptr per thread, the publication serial read before the lock) and runs only with light-observe.flag; the GXRS
  bank read again on each call (L8): left, two guarded reads under the flag only.
- **Reviews at the merge.** Final review (fresh Opus, xhigh, `main...c205d296e`): no High, no code defect. Medium: ENV-BANK-CALLERS'
  « Witnessed » listed D0CA20 as not called though it calls neither reader (the witness cannot see it), gave 770C0 a native answer
  it never had, and put D0D060's « never in play » (INFERRED) inside an OBSERVED paragraph: rewritten there and here (D0CA20 natively
  absent from tranche 28's admission rows, `set_region` false on all 47; in the original not witnessed). Low fixed: 40DD0's native
  blend labelled INFERRED (L3); the index row's « at Firelink » (L4); STATE and the index said « merged » ahead of the merge (L5:
  left, the merge makes it true); ENV-BANK-CALLERS' « 24 call sites » (23 by the reviewer's byte scan), 77110 (7710F), D0D060's « to witness »
  and 6A490's « no callers » (L6); `evidence/I-0140/t29-cache` named (L7); the cadence comment (L8, a comment only, not rebuilt).
  Low left: the key {-1, -1} equals the empty-slot mark, so a 41990 site asked only {-1, -1} would list no key, and an unreadable
  key, cube or scale leaves no count of its own (L1, L2): no such site or read in either run (every site lists its keys,
  minus_one 0); the witness on light-observe beside the others and its extra 41990 lookup per 416A0 call (L9): flag-only, kept
  for the cutscene witness left.
- **Integrity review** (fresh Opus, `main...e3762068a`): no High, no Medium; no check weakened, no special case, nothing cut,
  the figures sampled held (the counts, the bank's areas, the « module+ » symbols, 23 call sites, `set_region`). Low fixed: « about
  two minutes in play » was about 85 s (2 min 10 s with the title), and the runner's header said five minutes (L1); the « Low
  fixed » L5 above was only true at the merge (L2: said so); 770C0's REM fallback in ENV-BANK-CALLERS' OBSERVED paragraph now READ
  (by the final review) (L3); D0D060's « D0CA20 never met in about 370 native runs », uncited, dropped (L4).

## Tranche 28: the menu views' environment regions (D-0301), Thomas's character's portrait as the original's

Branch `claude/e-i140-t28` (from `main` `061c619f8`, 2026-11-21), merged `8825582e4` (08:05), the launcher rebuilt (179 PASS), the start check passed (`start-8825582e4`, 08:14 to 08:15: 46 regions admitted, 4 wrappers retired, no refusal; the module's close of the registry was not reached); disk rotation: nothing older than 48 h. The env-map bank's other callers, Thomas's next inventory: [ENV-BANK-CALLERS](ENV-BANK-CALLERS.md). ROADMAP workstream 5 step 4's portrait; D-0301 (Thomas, 03:44:
« remets le portrait original », the menu views keep their raster lighting and take the original's region pass).

- **The pass restored for the menu views (code).** An offscreen (menu) view draws its environment regions, natively for the first
  time (the legacy producer ran for the main view only), in place of a cleared SpecularAcc (an untraced main view still clears): the GXEnvMapRegion programmes, root, PSOs and box back in `scene_pass_lighting.cpp` (EncodeNativeEnvRegions,
  64 regions at most, `env_views`/`env_cube_views` in the view heap), produced by `ProduceNativeMenuEnvRegions`
  (`12_scene_collect.inc`) with the original's selection 67070 (STATIC, re-read): the manager GXRS+0x42B0 while on, the handles as
  69940 resolves them, the masks against the menu views' pair {1, 2}, the fade distance (+0xC8 or the manager's +0x2C0), the
  bucket rule (+0x2DA); the frustum test not applied (a box off screen rasterizes nothing). The cube: an area-1 key's GILM<id> as
  the menu forward rows' (NativeMenuEnvCube), its scale NativeEnvMapScale (default.gparam's row 10: 1.0); the CB as the legacy
  producer's (K-0424). The main view is untouched (traced, no region pass). Witness row `native_menu_env_regions` per view extent.
- **The portrait measure (Thomas's yes, 04:52).** `Run-AlphaCast.ps1 -QuitPortrait` writes, after each Quit Game shot, the pane's
  YAVG, UAVG, VAVG and SATAVG (ffmpeg `signalstats` on the 510x250 crop at 987,437 of the 1920x1080 window; another size or no
  ffprobe: « not measured »).
- **First run: nothing drawn (OBSERVED).** `i140-t28-thomas-nat` (`build-t28b`, 05:13-05:17): the Quit Game view (512x256) listed 30
  regions, all the world's {0x11, 1}; no menu region; the pane's YAVG 30.54 as before (the original's 41.23: `i140-t28b-witness-orig` and `i140-t28d-witness-orig`, `t28/run-b.log`,
  `run-d.log`; tranche 27's run measured another crop).
- **The cause (OBSERVED, `i140-t28d-witness-{nat,orig}`, light-observe, 05:42-05:47).** D0C2F0 runs natively as in the original,
  11 calls (10 at once early, before the world's regions are listed, one at the Quit Game click), GILM0010 registered both ways; its
  key gate 41990({1, 10}) on the engine's env-map bank GXRS+0x42A8 fails on all 11 natively (key_known 0) and passes on all 11 in
  the original: natively that bank holds only its default entry, the native reading the gparam rows from its own banks
  (NativeEnvMapScale, NativeEnvMapCube). The manager was also off natively at the first 10 (the native region registry is made by
  its first request); in the original it is on at all 11. An earlier reading (« the native never calls D0C2F0 », `i140-t28b`) was
  wrong: the native mode had no hook there; `1d186d4bf` installed it, and 67070's in plain Reference (it was in the original-probe
  block only, so `i140-t28b-witness-orig` had no selection rows). The original's selection (`reference_region_select`): the menu
  pair {1, 2} sees 31 regions, the 30 world ones and {1, 10} masks {0x11, 2}, box ±30 at the origin.
- **The fix (code).** `NativeMenuRegionAdd` (D0C2F0, `16_offscreen_and_movies.inc`): D0C2F0 runs, its key gate answered for that
  call alone from the native banks (`NativeEnvBankHas` on 41990: the engine's answer, else whether the key's area file has the id in
  its « EnvMap:ColorScale » row); the native region registry's 65D60 admits D0C2F0's call (its return site 140D0C908, the registry
  made by it when first) and answers 0 to a menu call out of scope or after its close; its 65320 answers 0 to D0C2F0's null handle.
  The release: the renderer destructor's D0C990 has the same gate, then tail-calls 664D0; natively `NativeMenuRegionRelease` sends a
  handle the registry holds to 664D0 (the registry's lock not held across it, the game's own lock order). A first native run on the
  fix without the registry's admission refused at the title (`i140-t28e`, `native_env_region_cpu_request_scope`, restored), as the
  step review had found.
- **The result (OBSERVED, MEASURED; `i140-t28h-witness-nat`, `build-t28h` = `4ae628812`'s code, light-observe, the debug layer
  off, 06:26-06:29).** Exit 0, no fault. The 11 adds pass
  (key_from_banks 1, a handle each), all 11 released at their views' teardown (held 1, released 1, one thread); the
  Quit Game view lists 31 regions and draws one, {1, 10} GILM0010 « bound », rate 1. The pane: YAVG 41.51 natively against 41.23 in
  the original (30.54 before), SATAVG 2.97 against 2.96, UAVG 128.38 and VAVG 128.51 in both
  (`t28/thomas-portrait-native-before-after-original.png`: the native before, after, the original; the armour's metal blue-silver
  again). One character, one view (Quit Game); the equipment and creation views share the constructor, not compared.
- **Step reviews** (fresh Opus, high), five. The producer (`061c619f8..3d7e80afc`, `..cbd3fea4b`): M1 (the witness items overflowed)
  fixed, truncated; M2 (one witness for all views) fixed, per extent; M3 (the portrait crop on any shot size) fixed, 1920x1080 only;
  L1 (the record's own cube with +0x2B8), L2 (span and rate in the row), L4 (the statics in the affinity scope), L6, L7 fixed; L3 (the
  manager read twice per call) left, small; L5 (the main view's heap about 6198 descriptors) left, inside its limit. Second: M1 (the
  outcome by fields), L1 to L5 fixed. The native region (`576c7702d..5836ede52`): H1 (65D60's replacement refuses D0C2F0's call)
  fixed, then OBSERVED as predicted in `i140-t28e`; M1 (key_known mixed the banks' answer) fixed; M2, L1 fixed; L2 (the thread-locals
  outside the affinity check) left: thread-local; L3, L4 (41990's equivalence unread, its callees) left: D0C2F0 calls no 416A0
  after it (the next review). `..bd75612d2`: H1 (the region never released: D0C990's gate) fixed; L1 to L3 fixed; L4 left as above;
  L5 fixed. `..595f7c044`: M1 (the registry's lock held across 664D0, the inverse of the game's own order) fixed; M2 (the release
  unseen) answered by `i140-t28h`; L1 to L5 fixed. The request path's order (the registry's lock, then 65D60's +0x2A0) was still 664D0's inverse: fixed at the
  final review (« Reviews at the merge »); only the release rows carried a thread (the engine thread's, i140-t28h).
- **Reviews at the merge.** Two final reviews (fresh Opus, xhigh, `main...677bd73d4`: E7's, its report
  `t28/final-review-677bd73d4.md`, and this thread's; neither saw the other), nothing high in either. Medium: the lock order
  (both): the request held the registry's lock across 65D60 and the close across 66640, both taking the manager's +0x2A0, while
  664D0 takes +0x2A0 then the registry's lock: fixed (`272666d03`), neither held across the game's call; its step review (fresh
  Opus, high) found the unlocked window let the close free the manager under a request in flight (Medium): the requests' 65D60
  counted in `env_region_cpu_calls`, which the close waits for before 66640 (`9c38eb479`); a second step review found two more
  windows (Medium): the caller's 65320 right after a request in flight, and the game's own 664D0 calls (D09F00's loop) not
  counted: 65320 answered 0 after the close for the three callers' return sites (140D0A8C1, D0C928, D0CFD5; none uses the
  answer), 664D0 hooked (ReleaseNativeEnvRegionCpu) to count every call on the scene root's manager and answer 0 after the
  close, as 664D0 itself with the manager off (READ: `XOR AL,AL` at 1400664E3); requests after the close answer 0 for all three
  callers, not only the menu's (its L1); a third step review (Medium): the game's own 66640 (D0D060, and 65B70 in 65690, the
  manager's destructor) neither counted nor gated, the manager's freed vectors left pointing after the close, and 652D0 refused
  after it: 66640 hooked as 664D0 (counted, a no-op after the close; the close calls its trampoline), the vectors' triples and
  +0x58, +0x2C8 zeroed after their frees (65690 and 66640 skip an empty vector; the lock's 17CC360 is `XOR EAX,EAX; RET`, 17CC330
  resets its table: READ), 652D0 after the close answers the closed manager's +0x80, as its own answer for a stale wrapper;
  664D0 pinned in the build's byte pins beside 66640 (`build-t28m`, 179 PASS); a fourth step review (Medium): 652D0 could still
  refuse while the close's own 66640 retires the wrappers: the closed manager kept from the close's start (`build-t28n`); its
  Low: 65B70 (READ) frees +0x58 and +0x2C8 as blocks when non-zero after its 66640, so the zeroing also removes a double free the
  destructor would have done after the close (+0x60, the count, is read only with +0x58); the trampoline's dead fallback replaced
  by a refusal; D-0302's title, ROADMAP line and IN-FORCE wording narrowed to Thomas's words (AMD's FSR, Intel's XeSS),
  who tests them open (a fifth step review: XeSS not limited to upscaling, the « Where » and « until » readings labelled or
  dropped; 66640 and 664D0 call no 652D0, READ: their calls are the lock, 652A0 through slot 0, the heap's free, memmove and
  memset, so the close's +0x80 answers reach no clear; the two comments rewritten). These windows exist only at the module's close with a region call on another thread:
  none was met in a run, none of the fixes ran live. Medium (E7's): 65D60's third caller D0CA20 (from 936700, once its two
  textures and gparam are loaded; never met) was refused (the game closed): admitted (its return sites 140D0CFB2 and, its null
  handle at 65320, 140D0CFD5; READ in the disassembly), a row field `set_region`; its region is released only by 66640, in the
  original too (D0CA20 keeps no handle), and its pair D0D060 (from 936660, no static caller) gates its removal on 41990({1, 0}),
  which the native bank likely fails (INFERRED): the next tranche's inventory. Medium (this thread's): the records said the menu
  views drew their regions « again »: they never did natively (the legacy producer ran for the main view only): said « for the
  first time » in RESULT, STATE, ROADMAP, the index; STUDY-ALL-GAPS row 18 rewritten. Low fixed: the first run's build and time,
  the original's 41.23 source, the left views (Status, Face Detail, the ten Load Game portraits, which now take the pass), the
  admitted row capped at 256 with its thread, `menu_region_add`'s thread, the menu witness's every-600th row capped at 64, the
  manager turned off at the close before its vectors are freed, stale comments (09's manager, 04c's 40 rows per pair, the close's
  thread), `EncodeNativeEnvRegions` in Test-ThreadAffinity's scope, `thread-affinity-allow.json`'s BOM removed, Run-AlphaCast's dead
  else. Low left: 69940's handle walk now four times and the selection without a unit test (a pure function needs a snapshot type:
  a later tranche); 41990 hooked process-wide, its callees unread (D0C2F0 calls no 416A0 after it); every native run with the debug
  layer off; the cube descriptors (about 6144) in every view context's heap, inside its limit; +0x2B9/+0x2BB (67070's targets 6, 7,
  the diffuse variant) and +0x2DA's 0x3A state not read; NativeEnvBankHas answering from the published gparam banks (a renderer
  built before them lacks its region, no row without light-observe); PortraitStats accepts any 1920x1080 shot (a borderless one
  shifts the pane) and runs before the Esc presses. Integrity review (fresh Opus, `main...272666d03`): nothing high; Medium: the
  merged code and the final review's fixes ran in no live run: said here (`build-t28i` to `build-t28o` were never run;
  the start check after the merge is the first run of the final tree); Low: the measure under light-observe and the debug layer
  off, said in « The result »; the affinity allow-list's three new statics (`calls`, `extents`, `lock` of
  ProduceNativeMenuEnvRegions, each with its reason) and the two functions added to the check's scope (stricter). The figures
  were recomputed from the stored shots by the reviewer (30.5447, 41.2347, 41.5073).
- **Left.** The menu forward rows' IBL keeps its constant key {1, 10} (I-0113 tranche 35) though the manager now holds that region:
  reading 4E2A0's choice from it is open. To compare in both modes: the equipment, Status, Face Detail and character creation views,
  and the ten 128x128 Load Game portraits (they share D65DD0, so they take the region pass now, each drawing the ten views' identical
  regions while all ten exist; no t28 run opened that list). Measured: one character, one view (Quit Game).
## Tranche 27: 11000 in the light list, the portraits of Delta and Thomas's character, gap 3's path (steps 1 and 4)

Branch `claude/e-i140-t27` (from `main` `0bdbe83d5`, 2026-11-21). ROADMAP workstream 5 step 1's 11000 and step 4's portraits and gap 3.

- **The runs** (`evidence/I-0140/t27/series-t27.ps1`, 02:18-02:40, the coordinator's slot announced in the thread; `run.log`):
  `build-t27`, main's code; the ASUS window 1920x1080, offline, every save copy restored by hash; natively the rays and no debug
  layer. Three sets, each native then original: Delta (`shebang-zones-delta-m50`, `-Continue`, `-QuitPortrait -QuitToLoadList`),
  a copy of Thomas's character (`thomas-alpha-cape-20261111-124806`, the same) and Friede's arena (Eta's m40 save, the travel to the
  Ariandel Chapel, the player teleported to the arena's MSB player start c0000_0001, -239.94 27.38 -230.08, the camera turned in
  eight steps, `test-no-ai`). All six exit 0, no fault.
- **11000 (step 1): the effect spot lights native by their mechanism; m45's two in the light list (OBSERVED in the journal; that
  they are the FXRs' INFERRED from place).** The FXR spot light takes the map's spot path (I-0103 CENSUS, EFFECT-LIGHTS §2; the
  same vtable 3D4E8E8), native since I-0106; at Friede's arena `native_light_visibility` (the local shadows' selection) holds two
  spot lights (spot 1) at -251.9 35.7 -229.9 and -256.0 40.8 -230.0, beside the MSB regions
  « boss area window A, B » (SFX 845210 and 845211, the only 11000 placements of m45, as `i140-e71` found; their spheres, radius 1,
  are 2.3 and 5.9 m from region B, the nearer of the two for both: a loose match), kept in 16 of 16 rows;
  the spots reach the rays' list (`native_point_lights`: spots 12, spot items 12, faded 0; CollectNativePointLights' spot branch,
  `RtSpotLightOf`; that these are the two is matched by the counts only). Nothing ties them to the FXR (no owner read, no run
  without the SFX); m33's and `commoneffects`' 11000 not seen. The eight steps' sheet
  (`t27/friede-sweep-native-left-original-right.png`) is alike by the pilot's eye but for the character: facing the wall his back
  and trousers are lit red natively, blue-grey in the original (`t27/friede-back-native-left-original-right.png`), the family of
  STUDY-ALL-GAPS row 4 (characters under D-0262's lighting; a light 2.2 m from him, radius 6, kept, or the bounce: cause not read),
  relayed to R. A witness naming each spot item's world centre was written and withdrawn: `native_light_visibility` already logs each
  light's pointer and sphere (the step review's M1).
- **Delta's portrait (step 4): as Alpha's (MEASURED).** `t27/portrait-yuv.txt` (the crops of tranche 26): face 120.3 natively against
  125.7, U 111.1 against 112.4, V 144.0 against 142.8; hair 38.2 against 38.1 (I-0113: 15.7 against 39.0); the pane 31.7 against
  32.2; the background 9.9 in both. To the digit Alpha's numbers of tranche 26: the same default face.
- **Thomas's character's portrait: the armour's metal black natively, blue-silver in the original (OBSERVED, MEASURED;
  `t27/thomas-portrait-zoom-native-left-original-right.png`).** Face crop 53.7 against 66.4, the helmet's top 60.3 against 108.5, the
  pane 32.2 against 42.0. The menu's forward IBL (I-0113 tranche 35) binds GILM0010 (`native_menu_ibl` « bound »), so the forward
  rows are not the cause. The deferred specular ambient is the likely one (INFERRED, no A/B run): natively a view that does not
  trace clears SpecularAcc (I-0131 R10a,
  `scene_pass_lighting.cpp`, « the menu views make no region »; before R10a the region pass ran for the main view only). Read
  statically (`ghidra-research`, GhidraMCP): the original's menu views do make a region. The offscreen views' base constructor
  D65DD0 calls D0C2F0, which looks up « GILM0010 » (area 1, id 10; `default.gparam`'s Env Map row 10, scale 1.0) and adds a region
  to the manager GXRS+0x42B0 (65D60), box ±30 round the origin, masks {0x11, 2}; the menu pipeline's flags 0x8422014 (D64C30) set
  bit 27, so 85540 sets +0x3ECF and 5C0E0 runs for its models (STATIC). A second read: the deferred region pass (67070's selection)
  runs for the menu views and filters by the view's mask pair, {1, 2} there (D64C30 sets the composite's +0xE0), so a menu view
  draws its own region and none of the world's {0x11, 1} (STATIC; the frustum's plane math INFERRED;
  [STUDY-MENU-ENV-REGIONS](STUDY-MENU-ENV-REGIONS.md)). Thomas, 2026-11-21 03:44: « remets le portrait original » (D-0301): the
  menu views keep their raster lighting and take the original's region pass; tranche 28's, whose first step is a witness (the
  study re-read; the native manager's state and the menu region when D0C2F0 runs: the manager was off at the Face Detail's
  creation in I-0113 tranche 35). A lead for the forward rows: 4E2A0 writes the group's scales +0x140/+0x144 from the
  chosen entries' +0xC (1.0 for {1, 10}, STATIC by this read), where the native takes the group's own keys' (1.5).
- **Gap 3's path (step 4): replayed, the portraits all there in both modes (OBSERVED).** In the first series `-QuitToLoadList`
  pressed Load Game on the title's notices (after Quit Game the title asks PRESS ANY BUTTON and shows the start's two offline notices
  again; `loadlist-after-quit-*.png`: « Unable to connect »). Fixed (three step reviews, below): the notices' base taken 8 s after
  the title's new page row (60 s at most; none: nothing pressed), each notice pressed until its `menu_window_close` row, as at the
  start. The second series (`series-t27.ps1 -Prefix i140-t27b`, 03:42-04:02, the coordinator's slot, `t27/run-b.log`): in all four
  runs (Delta from the Dreg Heap, the gap's own character, and Thomas's character from Firelink, each native and original)
  « title rows 1 new », both notices closed, the list opened; the five visible slots each show their portrait in both modes, Delta's
  included (`t27/gap3-loadlist-native-left-original-right.png`, the native left). Gap 3: not reproduced after these quits; the
  recorded case followed a quit from the Ringed City after a long session (I-0113 RESULT « Gap 3 »), not replayed. Limits of the
  switch: the title's row is the movie's first advance (one a movie pointer, 256 a process: a missed row skips the list, safely);
  the four quit clicks are timed, not gated on the menu (Pilot-QuitAndKeepSave's).
- **Step 2, the struck player's paint: a possible difference, not settled (the pairs not at the same moment).** Eta's m40 save, teleported beside two MSB c1430
  enemies on the way to Firelink (209.5 -57 603 and 211 -56.5 601.1; grave wardens with swords, not hollows), the AI on: Eta
  (level 7) was killed within about 10 s in both modes and came back at Firelink's bonfire; natively the paint ran
  (`native_paint_drain` 23 requests, 23 painted, 5 targets of 256 or 512). Seen (`t27/paint-native-left-original-right.png`,
  `paint-back-15-30-native-original.png`): orange marks on the original's chest and arm in the first shots (hit sparks or the paint,
  not told apart), none on either body after the respawn (the red on the back is the bonfire's flame). Next: a stronger
  character or F4's invincibility, the camera on the struck side, at the hits' moments (D-0299).
- **Step reviews** (fresh Opus, high): `614b4eedf..58181b235` (the notices): M1 (the base after a fixed 12 s, a late logo counted
  as a notice) fixed: the title's row awaited; L1 (the header), L2 (the count unsaid per notice) fixed. `..1ffac8d80`: L1 (presses in
  the world when no title row came) fixed: the rest skipped, « game gone » said; L2 (the header's order, « PRESS ANY BUTTON again »)
  fixed. The tranche's first step review (`0bdbe83d5..7ea79b545`): M1/L2 (the spot witness redundant) withdrawn; L4 (`-QuitToLoadList`
  with `-KeepSaveLabel`) refused; L5 (the title's rows) said.

- **Reviews at the merge.** Final review (fresh Opus, xhigh, `main...ee2457792`): nothing high. M1 (gap 3 closed on quits that are
  not the recorded Ringed City one) fixed: kept open in I-0113, STUDY-ALL-GAPS, ROADMAP, STATE and the index, I-0113's stale line
  fixed. M2 (11000's wording unlabelled or uneven; the workstream's own STUDY-NOT-YET-NATIVE not updated; « kept » is the shadows'
  selection) fixed: one wording, both copies, `native_point_lights` cited, CENSUS's skipped bullet dated. M3 (the armour's cause
  stated as fact, D-0301 on an unre-read study) fixed: INFERRED in RESULT, row 18 and the LOG; tranche 28 opens with a witness.
  L1 (STATE's placeholder hash) fixed: the hash at the checkpoint. L2 (the paint's wording) fixed. L3 (the title row deduplicated
  by movie pointer) and L4 (the quit clicks not gated) recorded under the switch's limits. L5 (the evidence, `build-t27b`) fixed.
  L6 (the study: another view's eye, the native manager off at creation, the scales' conflict) fixed. L7 (D-0262's line) fixed.
  Integrity review (fresh Opus, `main...4c244c772`): no test, gate or special case touched, no cut in a measure; M1 (11000 on a
  loose place match: the distances, the counts-only tie, m33 and `commoneffects` unseen, « none absent » as reasoning) fixed in
  the words above; L1 (the paint called null while the stills differ) and L2 (five of eight slots seen) fixed; L3 (Friede's sweep by
  eye) stated as such.
- Merged `8cf1340e5` (2026-11-21 04:27); the launcher rebuilt (`evidence/I-0140/build-launcher-8cf1340e5.log`, 178 PASS lines);
  start check `start-8cf1340e5` passed, restored (04:39; `evidence/I-0140/startcheck-8cf1340e5.log`); the disk rotation not run
  (another thread's compiler was running, `Clear-DiskCache` refuses then): left to the next merge.

## Tranche 26: gap 16 and the Quit Game portrait on today's module (step 4)

Branch `claude/e-i140-t26` (from `main` `2e379583f` with tranche 25, 2026-11-20). ROADMAP workstream 5 step 4: gap 16 (Fire Orb's
magenta and cyan rim at Archdragon Peak, STUDY-ALL-GAPS row 17; its last count, 6 to 10 pixels over three casts on a 640x360 film,
had no original beside it) and the menus' portrait cases (row 18).

- **The runs** (`evidence/I-0140/t26/series-t26.ps1`, 12:21-12:32, the coordinator's slot announced in the thread):
  `Run-I113FireOrb -Runs near-native,near-original -Casts 4 -LookSweep '1,2300,2' -QuitPortrait` (its new switch, tranche 25),
  Alpha's all-pyromancies copy restored by hash, the ASUS window 1920x1080, offline; `build-t26`, the launcher's module of
  `134d59be4` (R's SFX stop table in); natively the rays (level « rt ») and the default DLAA (`native_dlss_mode`), depth of field and motion blur off in both modes'
  GraphicsConfig (natively `native_yebis_dof` « no level drawn », `native_yebis_motionblur` « motion blur off »); `ffx-record`
  natively, `ffx-draw-observe` in the original. Both exit 0, no fault, restored (`t26/run.log`).
- **Gap 16: the magenta rim, short and intermittent, in both modes and of the same order at 1920x1080 (OBSERVED, measured on
  the effects' clock, D-0299).** Each cast creates 524020 (twice), 524022 and 524023 in both modes; at 524022's creation the arm
  is at the release and no explosion shows, which starts just after 524023's (+433 to +530 ms natively, +450 to +517 in the
  original, the final reviewer's sheet): 524023 is the burst (INFERRED from the frames). `Get-EffectFrames.py --fxr 524023`
  puts both films on its creation (`t26/frames523-native/`, `frames523-original/`, `frames.tsv`), and `Get-I113HueCount.py` counts
  the saturated magenta and cyan of every film frame (60 frames/s) from 100 ms before it for 1.2 s on a 700x500 crop round the
  burst (`t26/huecount60-524023.txt`): natively a peak of 0, 107, 21 and 0 pixels a cast (sums 0, 221, 106, 0; 7 and 8 frames
  above zero), the original 7, 1, 2 and 52 (sums 7, 1, 3, 264; its cast 4 11 frames above zero), all between +250 and +470 ms;
  no cyan in either. A first count at 20 frames/s on 524022 (`t26/huecount.txt`) skipped the native's 107 frame and the
  original's small ones: superseded. Seen: each mode's strongest frame in that window
  (`t26/mag-c2-900-c4-800-native-left-original-right.png`, native left: casts 2 and 4 of each mode at 524022's +900 and +800 ms)
  shows violet and magenta patches at the fireball's rim against the bright sky in both. Earlier counts are not comparable:
  I-0113 tranche 35's 73 to 121 pixels and `i140-e21`/`e23`'s 6 to 10 are whole films at 640x360 and 5 frames/s (`t33/hue640.py`,
  which gives 0 magenta in both of today's films, re-run by the integrity reviewer, its output not saved); I-0113 tranche 27's same crop at 10 frames/s gave magenta peaks of 104 to 726 natively
  against 0 to 9. So the strong rim Thomas saw on 2026-10-31 is not on today's module at 1920x1080 and what is left is of the
  original's own order (INFERRED); **2560x1440, where I-0113 tranche 26 found it strongest (240 to 1,981 natively against 0), not
  compared since the fixes.** What removed the residual after I-0113 tranche 35 is not established (a candidate: tranche 8's
  effects' fog, which lowers the 2AFA billboards' alpha). Row 17: alike at 1920x1080 by the pilot's measure, the sheet for
  Thomas's eye, 2560x1440 left.
- **The Quit Game portrait: the same framing, hair and background; the face a little darker, warmer and more contrasted
  natively (OBSERVED, measured).** `quit-load-*` (Firelink) and `quit-arrive-*` (Archdragon Peak) in both modes, Alpha
  (`t26/quit-load-arrive-native-left-original-right.png`; the pane 1:1 `t26/portrait-zoom-native-left-original-right.png`):
  on the 1920x1080 shots (`t26/portrait-yuv.txt`, its crops and commands `portrait-yuv.sh`) the face's 80x80 mean luma 120.3
  natively against 125.7, U 111.1 against 112.4, V 144.0 against 142.8 (warmer natively, the direction of Thomas's « face
  warmer » of 2026-11-08); the hair's 40-row mean 38.2 against 38.1 (I-0113 tranche 35's Alpha hair, 41.2 natively against 46.8,
  « to check », now within 0.1); the background 9.9 in both, pixel for pixel; each mode's four shots equal (the portrait is a
  still). The hair's dither against smooth edges is not measured at this size; Delta's portrait (I-0113: hair 15.7 against 39.0)
  and Thomas's own character not run; gap 3 (a Load Game portrait missing after a quit from a zone) not replayed. Row 18: the
  Quit Game portrait close for Alpha, the warmer face and the hair's edge for Thomas's eye on the 1:1 sheet; Delta, his character
  and gap 3's path left.
- **Reviews at the merge.** Final review (fresh Opus, xhigh, `main...HEAD` at `e2af6ecea`): nothing high. M1 (the peaks from a
  20 frames/s sampling; « the same size ») answered: recounted at 60 frames/s on 524023, restated above. M2 (gap 16 still open in
  STATE, I-0113's table, « Next » and STUDY's open list) fixed. M3 (row 17's « before tranche 33's fix » and incomparable metrics)
  fixed. M4 (the 2560x1440 limit dropped from the closures) carried into row 17, ROADMAP and Thomas's list. M5 (the portrait's
  closure past its numbers) answered above. L1 (524022 not the burst), L2 (unequal moments side by side, worded as each mode's
  peak), L3 (ROADMAP's size), L4 (`portrait-yuv.sh`), L5 (the 1:1 sheet), L6 (the residual's removal not established) fixed.
  Integrity review (fresh Opus, `main...HEAD` at `73ca99d46`): nothing high; the change of anchor and rate judged stated and not
  result-led (the native's peak rose from 55 to 107). M1 (I-0113 row 16's 1920x1080 counts under « open at 2560x1440 »), M2
  (ROADMAP's « 56 by a triage » dropped while trimming), M3 (the index's « portrait alike »), L1 (Thomas's character dropped from
  ROADMAP and STATE; « darker » in I-0113 row 20), L2 (STATE's and row 17's closure wording), L3 (hue640's output; the Evidence
  list), L4 (the blur settings) fixed.

## Tranche 25: the Firelink mist's contrast measured again (step 4)

Branch `claude/e-i140-t25` (from `main` `f4b6ecb88`, 2026-11-20), merged through `claude/e-i140-t26`. STUDY-ALL-GAPS row 23
(« the mist's own lower contrast », I-0113 tranche 39, 2026-11-11: the arch's mist spread 24-33 natively without the TAA and
23-32 with it, against 31-48; `evidence/I-0113/t37/crops-t39.txt`).

- **The runs** (`evidence/I-0140/t25/series-t25.ps1` 11:45-11:53 and `series-t25b.ps1` 11:59-12:05, the coordinator's slots
  announced in the thread; I-0113 tranche 39's protocol: Eta's m40 save outside Firelink's west entrance, the camera on the arch,
  a still every 4 s for 24 s and the hold filmed; the ASUS window 1920x1080, offline, restored by hash; `build-t24`, the
  launcher's module of `0860cbc8c`): five runs, all exit 0, no fault, restored (`t25/run.log`, `run-b.log`). R's SFX stop table
  (I-0141 tranche 13, merged at `134d59be4` after these runs) is not in that module: the mist is a loop never stopped (INFERRED,
  not read), so not covered here.
- **Natively the arch shows no mist at all in play (OBSERVED; `t25/sheets/mist-taa-notaa-orig-hold3.png`), by Thomas's
  choice.** The walls' SFX 840080 and 4 are not created natively (`ffx_create`: 0 against 1 each in the original, every other id
  within one) and `firelink_wall_dropped` names both: I-0113 gap 24's cut (Thomas, 2026-11-11 23:18 and 23:53, « Coupé par
  défaut » then « off forcé », `04_updates_and_test_tools.inc` 168). Tranche 39's measure was taken before that cut.
- **The mist itself, drawn natively with `ffx-observe.flag` (whose hook on the same create function makes the wall's own hook
  refused, so 840080 and 4 are made as in the original): alike by the measure at the same age of the effect (OBSERVED, D-0299;
  `t25/paired-t25.py`, `paired-t25.txt`).** The crop of the mist inside the arch (160x200 at 925,150) read from each film at
  the wall effect 840080's ages 60 to 85 s, every second (its `ffx_create` row and `hold.clock.json`; 26, 26 and 25 frames):
  mean luma 100.9 natively with the TAA, 99.9 without it, 99.6 in the original; mean luma spread 38.8, 39.6 and 40.9; mean Sobel
  gradient 26.8, 29.4 and 27.6. The mist brightens and darkens on a cycle of about 3 s, which the TAA run's table shows about
  1 s off the original's at equal age (the final reviewer's reading of the same table, not checked further): creation pairs the
  moment, not the loop's phase exactly. The five stills, by `crops-t39.sh` (`t25/crops-t25.txt`, the mean luma
  `t25/luma-t25.txt`, made by `luma-t25.sh`): Sobel 24.7-28.7, 27.1-30.4, 25.3-29.6; spread 31-49, 38-51, 32-50; luma 91-114,
  86-116, 83-113; secondary: the stills are 4 s apart on the runner's clock and the mist cycles in about 3 s, so they are not
  paired (D-0299), a range against a range.
- **The native's static detail moved since 2026-11-11: the stone and the foliage softer under the TAA, the hair sharper (OBSERVED,
  measured; not this row's).** The
  original's control crops are unchanged since tranche 39 (stone 57.6-58.5 against 57.8-58.3, hair 36.7-37.7 against
  37.1-37.5, foliage 55.4-56.1 against 55.5-56.3), so the view is the same; the native's moved: the stone 49.3-49.7 with the
  TAA against 54.0-54.4 then, the foliage 54.8-55.7 against 59.1-60.0, the hair 30.2-31.6 against 26.6-27.0, and without the
  TAA the stone 62.4-62.7 against 63.9-64.2, the foliage 72.3-73.0 against 69.1-69.7, the hair 39.8-41.7 against 34.9-36.1. The stone's TAA to no-TAA ratio fell from
  0.85 to 0.79. Which change between `build-t38e` and `build-t24` did it is not read; handed to the TAA's owner (the
  coordinator, for R; in STATE's R row).
- So the lower contrast of 2026-11-11 is not reproduced on the Firelink wall's mist (which change removed it, not read), and in
  play that mist is not drawn natively: row 23 closed by the pilot's measure; the sheets stay for Thomas's eye (D-0181). No other
  mist (a boss fog gate) was looked at.
- `Run-I113FireOrb.ps1` gains `-QuitPortrait` (in this branch, for tranche 26's run): a pass-through to Run-AlphaCast's existing
  switch, off by default (the Quit Game portrait after the load and after the arrival); no measurement changed, no tranche 25 run
  used it.
- **Reviews at the merge.** Final review (fresh Opus, xhigh, `main...HEAD` at `6ee88de87`): nothing high. M1 (D-0299 named as a
  limit where the wall's creation could pair the films) answered: the paired table above (`paired-t25.py`, the reviewer's script,
  kept). M2 (the stone « as in tranche 39 » when the native's controls moved) answered: the controls' bullet. M3 (« today's
  module » stale once R's stop table merged) answered: the module named, the stop table not covered. M4 (the mist still in
  STUDY-ALL-GAPS' open list and I-0113's handover) fixed. M5 (the merge's STATE and README conflicts, R's rows older) fixed: main's
  R rows, this branch's E parts. L1 (tranche 39's spreads) to L9 (the summary's length, `series-t25b.ps1`'s header) fixed, L5
  answered by keeping the sheets for Thomas's eye, L6 by « the Firelink wall's mist », L8 (STATE's size) at the merge. Integrity
  review (fresh Opus, on `main...HEAD` at `665bdc3de`): H1 (the runner's new switch unstated) recorded above; M1 (Thomas's eye
  promised, carried nowhere) fixed: the sheets in STATE's Thomas list and ROADMAP; L1 (the caveats dropped in I-0113 and row 23),
  L2 (the stills' D-0299 caveat), L3 (the controls' moves both ways, the foliage without the TAA, R's row), L4 (`luma-t25.sh`)
  fixed.

## Tranche 24: step 5 closed by Thomas; pass 0x20 and the refused forward blends read in the journals; the paint not witnessed

Branch `claude/e-i140-t24` (from `main` `6fda5cb23`, 2026-11-20, a fresh E thread).

- **Step 5 closed by Thomas.** Shown tranches 22 and 23's crops (`t22/crops/crucifix3-strips.png`, `inf3-*.png`,
  `t23/crops/andre-cast3.png`, `andre-deep-strips.png`; the Deep hit's brighter native violet named as the open point), Thomas
  (2026-11-20 09:36): « Pour moi c'est ok ». So the Crucifix's ring, the five infused hits, Crystal and Deep's violet are closed by
  his eye (D-0268); step 5's audit has no item left for E (R's: the Moonlight glare streak, Yhorm's machete, the hits' flashes).
- **Pass 0x20 never met since pass 0x21 is drawn (OBSERVED in the journals; STUDY-ALL-GAPS row 22).** `native_effect_stage`'s
  `other_passes` counts the classic FFX draws of a pass the effect stage does not take: 0x20 and 0x21 until `424ad5a31`
  (2026-11-13 23:32, pass 0x21 drawn), 0x20 alone since (`effect_stage.cpp` 197). Its last value is 0 in all 412 journals of the
  store written since 2026-11-14 that carry the row, the night endurance `full-1116-night` included, which walks the DLC's areas
  where the same counter stood at 77,805 in `full-1112-night` (before `424ad5a31`); Friede's fight (`i139-t2e-b22-friede-nat`,
  35,919 before it) was all 0x21 by tranche 2's own reading. With the study's static reading (1402997E0 sends the kind-1
  entities to 0x21; 0x20 is not reached through it), pass 0x20 is closed as never met: nothing to draw without a case.
- **The refused forward blends (STUDY-ALL-GAPS row 26): none since I-0113 tranche 10b (OBSERVED in the journals).**
  `native_forward_blend_unsupported` (`13b_prepare_model_frame.inc` 525, at most 12 rows a process) is in 119 journals of the
  store, the last at 2026-11-01 03:14 (`i115-al-n60`), before `6120bb56a` (2026-11-01 05:31, the ordinary rows' blend by the
  original's rule); none of the 2,550 journals written since 2026-11-01 after it holds one. Closed as never met since: the
  refusal path stays in the code (`13b` 524-525), so a material it would refuse has not appeared in these journals' coverage.
- **Step 1's other items**: the GPU particles' SH is twelve ones, not zero (`i140-e20`, `i140-e21`, above); the spot effect lights
  11000 are, by I-0103's census, the map's spot path, so a light rather than a draw (INFERRED there): where they go (workstream
  12's sources, whose steps do not name them yet) is not settled, so they stay step 1's open item with 10003's brightness for
  Thomas's eye (STUDY-ALL-GAPS row 21).
- **Seen in passing (a lead for R, not E's):** Andre's hair and beard grey natively where the original's are white
  (`i140-t23-andre-infusions-nat` and `-orig`, `cast-10-3000.png`; unpaired frames, the pose differs: not a measure). The hair is a
  forward row (`SAT_HairIbl2_Fwd_[A]`, `04b_pass_witnesses.inc` 646), and a forward row's cbLight loses lights 1 to 3 on a traced
  frame (`12_scene_collect.inc` 720, D-0262), as the crystals of gap 17; that this is the cause is INFERRED, not read. Relayed to
  the coordinator for R.
- **A film on the game's clock (D-0299), scripts only.** Run-AlphaCast's recorder writes beside its film ffmpeg's report
  (`<film>.ffmpeg.txt`, FFREPORT at level 32: gdigrab's input « start: », its first frame in seconds since 1970: 65 ms natively and 51 ms
  in the original after the clock pair's UTC in this tranche's two films) and `<film>.clock.json` (the QPC in ms and UTC read together), and
  [Get-EffectFrames.py](Get-EffectFrames.py) puts each `ffx_create` row (the module's QPC ms, both modes) in the film and extracts
  its frames at chosen offsets: the native and the original paired by an effect's own start and its rank, the creation's whole caller
  chain written (the module's RVAs above the 24F0C4 wrapper, « 0 » a frame outside the module). Inside these films each id has one
  chain (FXR 193's E6DD65 > 7CB8B2 > 7CBF7E natively, with two frames outside the module between, and E6DD65 > 7CB8B2 > 7CBF7E >
  879489 > 8906FD in the original); the module captures six frames, so the native chain stops above the frames that tell two
  sources of the same path apart (the original's 879489, 8779EC, 87B0D5 for 193, 201001, 320): a pair from two sources of the
  same path is not seen natively (its step review's H1; a deeper capture, 21_effects.inc 373, is a later step's). The rows' `t` is a
  direction (a unit vector on every row), not a place: the reader no longer writes it (the final review's M1). Its limits: the
  picture lags the creation (measured below), the offsets are wall-clock ms, the module logs 6000 creations a session. Plan `andre-paint` (`arts-t17.txt`): seven Dagger swings on Blacksmith Andre, `H1` before each, `I1` once.
- **Step review** (fresh Opus, high; `6cc2b4fd2..877f20411`), before the run: no High. M1 a stray ffmpeg report (`C`, a FFREPORT
  path cut at its colon by the pilot's first test) committed at the root: removed. M2 an empty answer silent: the reader counts the
  rows (before and inside the film), warns at the cap and exits non-zero without a row or a frame. L1 rows past the film's end:
  dropped (its length from the report's last progress line), a frame named only when written. L2 the clock's write could throw
  inside the recorder: wrapped. L3 the audio started after the clock's write: the clock read after it. L4 a pair by rank alone:
  the position and the first caller written (both empty of meaning: the final review's M1, below). L5 the limits above: in the reader's text and its table's header. Fixed in
  `2a6044408`.
- **The run** (`i140-t24`, 10:30-10:41, the coordinator's slot announced in the thread; the ASUS window, offline; the launcher's
  module of `0860cbc8c` (no native change since) copied to `evidence/I-0140/build-t24`; natively `ffx-record`, in the original
  `ffx-draw-observe`; the stats-99 copy restored by hash; `t24/run.log`): both exit 0, no fault, restored; the 20 F1 words applied
  in order in each; swings 1 to 6 each took 63 HP from Andre in both (the `p` rows), the seventh seen on his HP bar. `ffx_create` rows: 497 natively, 487 in the
  original, under the cap; each swing creates FXR 193 once in both (with 221400, 401000, 420 at the same moment), seven in each
  film. Frames at its creation plus 0, 100, 300, 1000, 3000 and 8000 ms (`t24/frames-nat`, `t24/frames-orig`, each `frames.tsv`).
- **The picture's lag, measured on a cue tied to the hit (the final review's M3, its step review's H2; `t24/lag-check.py`,
  `t24/lag-check.tsv`).** Andre's pose is no reference (his hammering is not locked to the player's hit: at 1000 ms after hit 7
  his hammer is up natively, down in the original, `k7-300-1000.png`). His HP bar is: hidden before each hit, shown with « 63 »
  after it. Its red row is 1 to 2 px tall in the yuv420 film, so a pixel counts when R > 80 and R exceeds G and B by 35, and the
  cue is the first frame whose longest red run above him reaches 40 px (the bar 72 to 84, 4 to 18 before it), every frame of the
  films from 150 ms before each FXR 193 creation, each frame at its own time in the film (the seek point plus showinfo's
  pts_time; the films miss frames: 59 and 56 gaps, 43 and 52 of 33 ms, 16 and 4 of 50 ms, none just before a cue). A first count (R > 120, G and B < 70, a change of 3 px) missed the
  bar on three hits and caught it late on the others, and a second counted frames from an assumed start: both replaced. The
  bar's first frame after hits 1 to 7: +25, +26, +29, +37, +55, +37, +45 ms natively; +10, +11, +28, +11, +11, +10, +10 in the
  original (OBSERVED, to one frame). So the bar comes 1 to 44 ms later natively (median 26, 7 of 7 hits). Each film's clock
  mapping is one constant in all its values, its error not measured (the original's 10 ms is short for render, present and
  capture, so the mapping may put creations late); a repeated frame just before the cue in each film (original hit 3 at +11,
  native hit 5 at +39) sets the range's ends, about 15 to 35 ms without them; and the bar is UI, the effects' own picture not
  measured. Measured once (one window, one build). At 3000 ms both men stand idle in the same place
  (`body-k1-k7-3000.png`), which says little.
- **The struck character's paint: not witnessed (none visible in either renderer, OBSERVED by eye; ROADMAP workstream 5 step 2
  stays open).** After seven slashes Andre's chest, arms and face show no blood mark in the original nor natively at 3000 ms after
  hit 7 (`body-k1-k7-3000.png`, hit 1 above for the same pose). Natively the paint runs: `native_paint_drain` 28 requests drained
  (four a hit), 21 painted, 7 refused by the receiver's gate (whether the original's gate refuses the same, not read);
  `native_paint_frame` 12 draws, on the hits' frames only; three targets (256, 256 and 512). Nothing in these rows names the
  receiver, so that the painted targets are Andre's (and not the player's) is not established, and the native frame is much
  darker than the original's, which hides dark red on skin. A null result, not « alike »: a broken native paint would give the same
  films (the final review's H1). Next: a case where the original visibly shows the paint first (an enemy bleeding under heavy hits,
  the player struck, the camera close, a larger window), then the native at the same moment; or the receiver's owner in the
  paint rows and the targets' content read.
- **The blood spurt: alike in kind, natively denser to the eye and at 100 ms by the count (an open lead).** Pink filaments burst
  from the hit in both (`k2-k6-100.png`, `k7-300-1000.png`), natively thicker and more opaque. The pixels with R > 90 and R > G + 40
  in a 560x520 crop round Andre (`t24/red-count.py`, `t24/red-count.tsv`): at 100 ms 2,864 to 9,300 natively against 1,608 to
  2,246 (hit 6's 21,711 in the original an outlier, not read), so 4,000 to 7,600 more on five of seven hits; at 300 ms 1,086 to
  1,587 against 518 to 1,447; at 1000 ms (the spurt gone, the skin alone) 1,002 to 1,966 against 414 to 687, the skin's share
  about 650 to 700. So at 300 ms the excess is mostly the skin's (warmer natively, the lighting's, R's), at 100 ms mostly not; the
  native's HP bar about 26 ms later (above), if its effects lag alike, makes its spurt younger at the same offset: part of it, perhaps.
  Open, not read further: the spurt's particles natively against the original's at the same moment.
- **Andre's hair grey natively, now at paired moments** (`body-k1-k7-3000.png`: the same pose, white strands in the original, a
  grey, softer mass natively): the lead given to R above, with its pictures.

## Tranche 23: the embered state on the clothes (Thomas's impression; Crystal and Deep)

Branch `claude/e-i140-t23` (from `main` `38b039f97`). Thomas (2026-11-17 09:16-09:18, relayed): « J'ai l'impression que les effets
de braise manquent en natif sur les vêtements », « Normalement ça fait des traces oranges sur les vêtements », « Oui c'est le statut
embrasé du jeu ».

- **Read before any run.** Tranche 17b's `goods-use` (`i140-t17b-goods-use-*`, `build-t17e` = `60be6dff4`'s module) used an Ember:
  after it (`attune-29.png`, the character from behind at Firelink Shrine) the orange traces show on the cloak natively as in the
  original (`t23/t17b-attune-29-body.png`, OBSERVED by the pilot's eye: a handful of glowing spots in each, not counted). Since
  `60be6dff4` the module's changes are the test tools and the effects' witnesses only (`git diff --stat 60be6dff4 main -- native`:
  `effect_draw`, `04_updates_and_test_tools.inc`, `04b_pass_witnesses.inc`, `21_effects.inc`, `22_setup.inc`), none on the
  admission's parameter animation (`09_model_admission.inc`, `float_4_0`) or the glow stage; `21_effects.inc`'s `EntityOffered`
  (now returning `uint8_t`) and `CreateEffectObserved` are hooked only under `ffx-record.flag` (`22_setup.inc`), off the glow path. In Thomas's filmed play
  (`i140-t22-thomas-nat`) the character was never embered: the health bar's icon is the hollow one in every frame showing the HUD, taken every
  15 s (`t23/thomas-nat-hpicon.png`; about a quarter of the 40 cells show the equipment menu, one is black), so no trace was due
  there. INFERRED: no regression. Thomas (09:45): « Ah oui c'est vrai mon perso est mort donc
  a perdu ses braises, my bad. Tout est ok alors, pas d'écart »: closed by Thomas, no gap.
- **The witnesses prepared** (`ember2`, not run, closed by Thomas: an Ember from the quick slot, then the character from behind, from
  the front and from behind again) and `andre-infusions` (Thomas, 09:46: « Gel aussi avec la straight sword d'Irithyll mais faut
  plutôt taper André, la firekeeper meurt trop vite »): Crystal, Deep and the Irithyll Straight Sword's frost on Blacksmith Andre,
  the player placed by `-AimAt` 1.5 m from his standing point toward the bonfire, `P1` before each swing, `H1` before each new weapon but only
  the first Irithyll swing, and F4 before each new weapon. F4 toggles invincibility (`04_updates_and_test_tools.inc` ~935), it is
  not a refill: on, off, on, off, so the player was unprotected during the Deep and the last three Irithyll swings (a plan defect,
  the final review's M3; for a rerun, `H1` before every swing and `I1` once at the start, as maggots2 and crucifix3). No step
  review before the run (WORKFLOW asks one; it would likely have caught it). The launch: the coordinator's slot (R done at 10:26),
  announced to Thomas at 10:26, and Thomas's written rules (10:03, two `allow` lines for the runners), which let auto mode launch it.
- **The run** (`i140-t23`, 10:26-10:42, `build-t22-final` (`d87e86ef1`'s module; the native changes since are comments), the ASUS
  window, offline, the stats-99 copy restored by hash, `t23/audit1.log`): two runs, exit 0, no fault, restored; 30 F1 words applied
  in order in each. The nearest character was Andre in both (HP 645, at 230.81, -56.38, 596.68; the first probes at 1.14 m natively and 1.10 m in
  the original, then 0.85 to 0.87 m).
- **Every judged swing hit him, with the same damage in both** (the probes after each swing, `t23/audit1.log`; cast 11 not probed): the Dagger 63 twice,
  Crystal 110 twice, Deep 52 twice, the Irithyll Straight Sword 142 four times (OBSERVED in the rows).
- **Crystal: alike by the pilot's eye** (`t23/crops/andre-cast3.png`, 400, 900 and 1600 ms, native above): the blue crystal
  sparks over him at 400 ms in both. **Deep: alike in kind, the native's violet brighter** (`andre-cast5.png`,
  `andre-deep-strips.png`, 100 ms a frame from 0.2 s, native in rows 1 and 3): red streaks, then violet wisps at 0.6 to 0.9 s in
  both, the native's violet whiter and brighter (an impression by eye, not measured; that it is the hits' brighter flash, R's
  item, is INFERRED, not read, so it stays on E's list). Thomas's eye left on both.
- **Frost: not reached.** Four Irithyll swings at 142 each and no extra drop: frostbite did not proc on Andre (INFERRED from the
  equal damage; DS3 shows no buildup bar on an enemy). The swings were about 23 s apart (09:32:18 to 09:33:54 UTC), long enough
  for the buildup to decay (INFERRED, not read): this plan cannot reach it; a rerun needs quick consecutive swings, or Andre's
  frost resistance and the sword's buildup read first. The sword's own frost mist is drawn alike at 1600 ms (`andre-cast11.png`;
  the original's 900 ms frame shows Andre, not the blade). Left.
- **Andre killed in both runs.** With `H1` only before the first Irithyll swing he had 645 - 4 x 142 = 77 HP before cast 11,
  another 142 swing; cast 11's 3000 ms shots show no Andre at his anvil and his line « Beshrew your heart... » in both renderers
  (OBSERVED, `i140-t23-andre-infusions-nat` and `-orig`, `cast-11-3000.png`; his death INFERRED from the line and his absence).
  Both runs played on the stats-99 copy, restored by hash after each: Thomas's save keeps him.

### Tranche 24's reviews at the merge

Merged `6044b13ce` (2026-11-20 11:37; records and scripts only, so no launcher rebuild nor start check); the disk rotation
(D-0260) after R's run: 78 build folders older than 48 h, 19.8 GB deleted, 521.5 GB free.


The final review (fresh Opus, xhigh, `main...4087ad045`): **H1** step 2 closed on a null result (no paint seen in either mode):
answered, step 2 open, the paint « not witnessed » in RESULT, ROADMAP, STUDY-ALL-GAPS row 25, STATE and the index. **M1** the
reader's caller and position meaningless (the wrapper's return, a direction): the position dropped, the callers above the wrapper.
**M2** records contradicting ROADMAP's « Done » on steps 1, 2 and 5: step 1 keeps 11000 and 10003's brightness, step 2 the
overlay view, « Next » 3, 5 and 10 and STUDY-ALL-GAPS' summary agree. **M3** « the clock holds » not shown by its sheets: the lag
measured on the HP bar. **L1** « denser » against its counts: reworded (and again after the integrity review). **L2** the
evidence line and the Conclusion: added. **L3** STATE's rows: rewritten (its D-0230 date is not E's row: left). **L4** row 25's
binding and row 26's line: corrected; the comment at `05_selected_scene.inc` 285 is stale, native code outside this tranche: left.
**L5** the reader's robustness: `--out` and `--fxr` required, negative offsets refused, failed frames reported, the epoch check.
**L6** the stray `C` file enters `main`'s history (added in `877f20411`, removed in `2a6044408`): left, local history only.
**L7** the index row: corrected. Fixed in `d4f1f30a1`.

The step review of those fixes (fresh Opus, high, `4087ad045..d4f1f30a1`): **H1** the callers cut to three hide the frames that
tell sources apart: the whole chain written with its « 0 » frames, the depth's limit stated (above). **H2** the first lag count
measured a threshold artefact: the test replaced and the lag re-measured (7 of 7 hits). **M1** the
lag includes each film's clock mapping: said. **L1** frames past the film's end counted; **L2** an absent id named; **L3** older
PNGs kept beside a new `frames.tsv`: said on exit; **L4** a cue at the creation reachable (the search from the first frame);
**L5** the module's comment calls `t` the translation row: stale, native code, left; **L6** the epoch bound 1 s.

The step review of those (fresh Opus, high, `d4f1f30a1..c3f45e05e`): no High. **M1** the lag counted in frames from an assumed
start: each frame's own time read, the values above (native 25 to 55 ms, original 10 to 28, median difference 26). **M2** the
difference not separable from the films' mapping errors: said. **L1** `--list` cut to eight creations: each id's distinct
chains with their counts. **L2** the epoch bound refusing a slow ffmpeg start: 60 s, the gap printed. **L3** a UI cue
generalised to the picture: « the HP bar ». **L4** the native's repeated frames in its values: said.

Its own step review (fresh Opus, high, `c3f45e05e..86b6a5c17`): no High. **M1** the repeated frames in both films, setting the
range's ends: said, as **M2** the gaps' sizes. **L1** the reader's text on `--list`: corrected. **L2** lag-check reading every id
of `frames.tsv`: it keeps FXR 193's rows. **L3** its ffmpeg without `CREATE_NO_WINDOW`: given. Its output unchanged.

The integrity review (fresh Opus, `main...d4f1f30a1`): no High. **M-1** step 5 closed past D-0181 (Thomas's eye on the strips
dropped from the left items): restored in « Next » 5 and ROADMAP step 5 with the 56 triage verdicts, the two casts not compared
and the two arts not used. **M-2** the spurt's 100 ms count left out and « no denser spurt » past the data: the 100 ms column
reported, an open lead. **L-1** the chain wording: corrected (above). **L-2** the lag's dropped hits: the new count finds all
seven. **L-3** one scene's lag written into every output: the header no longer carries it, TOOLS says « measured once ». **L-4**
six swings judged by a row: said. **L-5** the gdigrab offset's pointer: the two films' 65 and 51 ms cited; Thomas's 09:36 words
are the thread's. Row 26's « are drawn »: reworded to « never met since ».

### Tranche 23's reviews at the merge

- **Final review** (fresh Opus, xhigh; `main...1482e4b05`): no High. M1 the merge of `main` rolled STATE's R entry back to
  I-0141 tranche 10: fixed (main's sentence put back). M2 the summaries called Deep alike and ROADMAP's left list dropped frost:
  fixed (STATE, ROADMAP, the index and the Conclusion say Deep alike in kind with a brighter native violet, open; frostbite in
  ROADMAP's left list; R's attribution INFERRED). M3 the plan killed Andre and F4 is a toggle: recorded above; the plan's comment
  corrected, the plan kept as run. L1 frost's reason: recorded (the swings' spacing). L2 STATE over its size, stale counts
  (« 13 to 22 »), « the seven infused hits » for two sets, the closed embers under « left »: fixed. L3 the asked question beside
  its answer and « every frame »: fixed. L4 `ember2` kept, marked not run and closed by Thomas. L5 no step review before the run,
  the launch's slot: recorded. L6 the first probes' distance, the frost mist's frames, the lock-on comment: fixed.
- **Integrity review** (fresh Opus; the same range with this section): no High. M Deep folded into « alike » in STATE, the index
  and ROADMAP: fixed (as M2). L1 « whiter and brighter » kept as an impression, not measured. L2 « every frame » with the HUD: fixed.
  L3 `EntityOffered`'s change named, off the glow path: fixed. L4 « every judged swing » and the first probes: fixed. Outside its
  categories, STATE's R entry: fixed (as M1).
- **The merge**: `547b2db09` (2026-11-17 10:58, during R's run without a measure, the coordinator's word); scripts and records
  only, so no launcher rebuild and no start check (as tranche 21).
- **NVIDIA FLIP on the Andre pairs** (Thomas's yes, 11:24; `flip.exe`, the original as reference, `-vc 0.7 0.53 1920`, the game
  area cropped; `t23/flip/`: `flip.csv`, `maps/`, `sheets/`): the 44 pairs' mean FLIP lies between 0.11 and 0.17 in every cast,
  with or without an effect, so the mean does not single out the hits. The maps light up Andre's whole body, the wall's bricks
  and the candles: the two runs are not in the same animation phase (Andre's pose differs, `andre-close-2-3000.png`) and the
  lighting differs (the rays against the raster), so on live pairs FLIP measures pose and lighting, not the effects (OBSERVED in
  the maps). It needs aligned frames: a cutscene's same moment, or a still scene. Seen in passing: Andre's hair and beard less
  white natively (`andre-close-2-3000.png`; the pose and light angle differ, so not a finding).

## Tranche 22: step 5's left list after the pause (a target that lives, the statuses from beside, bolts, the bow's art, the Crucifix again)

Branch `claude/e-i140-t22` (from `main` `f36e4445a`; Thomas lifted the pause at 06:54, the weekly limit still at 90 %).

- **The tools** (`04_updates_and_test_tools.inc`, test tools only): F1's `h<id>` writes the nearest living other character's HP
  (the `p` probe's character, its data module's `+0xD8`) back to its maximum (`+0xDC`) once, its row the probe's with the HP
  before; a character at 0 HP is not in `VisitTestCharacters`, so none is raised. The runner takes `H<id>`. Families
  (`arts-t17.txt`): `crucifix3` (the stance, meant outdoors by a teleport that did not hold, below), `keeper-status3` (the
  statuses after the walk's facing, F1 and a camera turn of an eighth of a turn, no lock), `keeper-infusions3` (two Dagger
  swings to close in, then Fire, Dark, Blessed, Poison and Blood twice each, `P1` then `H1` before each swing), `bolts2` (six
  bolts at the nearest character, no lock of their own), `bows4` (the Longbow's art at 1500 and 2500 ms, each followed by a
  cast, so the FP line judges it; no step review of its own, the final review covered it).
- **Step review** (fresh Opus, high; `f36e4445a..a86e509f3`), before the run: no High. M1, keeper-status3's second bleed had no
  refill (her 307 HP below a poison's and two bleeds' 327): `H1` before every status. M2, a Blood swing whose burst kills her
  would leave Blessed to another character: Blood last, Poison before it. M3, `M1 M1` assumed the first click locks on (it only
  resets the camera when she is out of view): keeper-status3 makes no middle click, the walk's own facing, F1, then `T500`. L1,
  the look runs after a cast's F1 presses whatever the line's order: cast 1 a swing only, the statuses after the turn. L2, after
  an applied `h` the next probe compares with the maximum: said in the runner's and the module's comments. L3, L4 (comments):
  fixed. The first launch was stopped before its start time (07:10) and the fixed plans launched for 07:20.
- **The run** (`i140-t22`, 07:20-08:13, `build-t22a` = `a86e509f3`'s module, a cache build without the tests; the step review's
  fixes are scripts and a comment; the ASUS window, offline, the stats-99 copy restored by hash, `t22/audit1.log`): ten runs, exit
  0, no fault, every restoration ok, every F1 word applied in order. It ran 8 minutes past its slot (08:05).
- **The Crucifix of the Mad King: the white ring alike by the pilot's eye; Thomas's eye left (`t22/crops`).** The teleport was applied and then released at the
  start position (`witness_teleport` then `witness_teleport_released` at 266.83, -53.01, 590.12 in `startup.jsonl`): the five
  stances ran in Firelink Shrine, as in tranche 19, casts 1 to 4 spending FP in both. In the strips (`t22/crops/crucifix3-strips.png`,
  from each stance's click, 250 ms a frame, 192 pixels wide, native above each original) the round translucent shockwave about
  the character comes about 1 s after the click in the original's casts 1 to 3 (4 and 5 not made out at that size, the camera diving, then a
  wall) and natively in casts 2, 3 and 5 (OBSERVED by the
  pilot's eye; casts 1 and 4 natively show the violet burst and its radial streaks, the ring not made out at that size), with
  the violet swirl at the feet in both; the final review, on its own frames, saw the ring natively in all five. So the
  original's white ring, tranche 19's lead, is drawn natively too, by the pilot's eye (D-0181: Thomas's eye left). Cast 5,
  the last, is not judged by the FP line. The teleport (as tranche 21's maggots2) never held: removed
  from the runner.
- **The infused hits: Fire, Dark, Blessed, Poison and Blood alike by the pilot's eye; Thomas's eye left (`t22/crops`).** Every
  judged swing hit her in both renderers (the first Dagger swing missed; the last Blood swing is not judged, no probe after it), with
  the same damage (the `f` and `p` rows after each `h` refill: Fire 93 twice, Dark 199 twice, Blessed 196 twice, Poison and
  Blood 152 each time, in both; `t22/audit1.log`). Strips (`t22/crops/inf3-<infusion>.png`, 200 ms a frame from 0.2 s after the
  swing, native above): Fire's flames, sparks and smoke, Dark's violet burst, Blessed's, Poison's and Blood's red spurts, alike
  by the pilot's eye at 256 pixels a frame. Natively the characters are darker (the traced lighting, R's) and some impacts flash
  brighter (Poison's first swing; the hits' brighter flash, R's item). No bleed burst was seen (the Blood swings did not
  fill her gauge, INFERRED from the equal damage). Crystal (2010500) and Deep (2010900) were never swung at a target: left.
  The keeper walk's nearest character differs from load to load (here and below): a target fixed by the walk is not assumed.
- **The statuses on an NPC: framed in the original, not natively.** The keeper walk's nearest character differed: in the
  original the Fire Keeper (HP 307, 3.57 m), in the picture from cast 2, beside the player after the turn (`t22/crops/status3-look-c4.png`,
  `status3-c5c6.png`, `status3-orig-<status>.png`); natively a character of HP 239 at 9.49 m, not made out in the picture
  (the « Talk » prompt at 3.09 m). Every status was added to its character in both (the rows' HP: natively 239 to 182 after the
  poison, 230 to 150 after the frost, 230 to 38 and 227 to 35 after the bleeds; in the original 307 to 173, 295 to 129, 298 to
  21, 295 to 18, with her Dagger's hits). Their look on an NPC is not compared: not judged.
- **The bolts (bolts2): not reached.** The nearest character was the one of HP 239 at 9.5 m in both; its HP never changed
  (every probe 239), and the film shows the player facing an arch with no bolt seen (`t22/crops/bolts2-*.png`). Not judged.
- **The Longbow's art (bows4): not cast.** The stances of 1500 and 2500 ms, each followed by a cast, spent no FP in either
  renderer. Thomas (08:26, relayed): « je peux m'en occuper moi même »: left to his own play, with the bolts and the statuses on
  an NPC: after the three items listed to him (08:34), Thomas (08:34): « Tu lances tes enregistrements et je fais l'action »:
  a run filmed while he plays (below).
- **Thomas's own play: the bows' art, the bolts and the statuses on an enemy closed by Thomas's eye.** `run-thomas.ps1`
  (`t22/thomas.log`; `i140-t22-thomas-nat`, `build-t22a`, the ASUS window, offline, the stats-99 copy restored by hash): F7 gave
  the Longbow, the Light Crossbow, Standard, Lightning and Exploding Bolts and Blood and Poison Longswords, then 600 s filmed
  (`hold.mp4`, 08:48-08:58) with no input from the runner. In the film (`t22/thomas/`, a frame every 4 s): the equipment menus,
  the crossbow and the bolts, Firelink Shrine and the hollows below it, fights. Thomas (08:59): « Tu peux fermer le jeu », then
  (09:00) « Pour moi tout est bon »: the three items closed by Thomas (D-0268), natively, against his knowledge of the original.
  The native game ran to the runner's 900 s bound (exit 0xD3126612, the controller's stop, no fault, restored); the original
  was closed by the pilot during its load at Thomas's word (restored, `restoration.json` ok, his save's hash checked after), the
  runner then stopped and the test files removed by hand.

## Tranche 21: the maggots filmed as Thomas asked (scripts only)

Branch `claude/e-i140-t21` (from `main` `50a022a46`). Thomas (2026-11-17 05:22-05:27): « refilmer en 1080p, caméra normale,
personnage vu de l'avant puis arrière », « au moment où le personnage s'équipe d'une torche », « juste des screen », « dans un endroit
éclairé, dehors », « S (quand il est de dos) et Z (quand il est de face), sans toucher à la caméra ».

- **The tools** (scripts only, no native change): the runner's words `I1` (F4), `S<id>` (F1's SpEffect on the player), `T<dx>` (a
  camera turn, Run-AlphaCast's new plan item `l<dx>`, not used in the end), the cast kinds `face` and `away` (a tap of S or Z), its
  `-GraphicsLevels` passed to Run-AlphaCast; the family `maggots2` meant by the Cemetery of Ash's bonfire (the teleport was released at the start: it ran in Firelink
  Shrine, tranche 22's final review).
- **The run** (`i140-t21`, 05:36-05:53, `build-t20b`, `ScreenMode=FULLSCREEN` at 1920x1080 on the ASUS, offline, the stats-99 copy
  restored by hash, `t21/audit1.log`): natively the four casts at 05:38-05:39 (the maggots added, every F1 word applied); the
  game then ran to the run's 900 s bound (exit 0xD3126612, the controller's stop, no fault, restored): Run-AlphaCast's close
  click, made for a window's title bar, falls inside the picture in full screen (the step review's High; said in the runner's
  comment), and Thomas was using the game meanwhile (« c'est moi ça, pas un bug », 05:57). The original run did not take the
  teleport and its game was gone at once (exit 0, restored), Thomas's doing as well (same message): no original pictures.
- **Thomas's own check** (05:40-05:44, watching the run and in his own play): « les maggots et leur disparition » seen, then
  « C'est bon les maggots sont de la bonne couleur, j'ai été vérifié moi même »: the maggots closed by Thomas (D-0268). The black
  grass he saw at Cleansing Chapel (the Cathedral of the Deep) goes to workstreams 12 and 13 (Thomas, 05:55: « tout sera
  amélioré aux chantiers 12 et 13 »).
- **Step review** (fresh Opus, high; `50a022a46..c78639b3a`): High, the « 12-minute close » recorded as a native defect was the
  runner's bound (above, rewritten; Thomas's own use besides). Medium, three comments on the teleport's line: each back on its own.

## Tranche 20: the player turned toward a target (the statuses on an NPC, the infused hits, a bow), the Rope Firebomb again

Branch `claude/e-i140-t20` (from `main` `c1d70cc8d`).

- **The tool** (`04_updates_and_test_tools.inc`, test tools only): F1's `f<id>` writes the player's heading, the physics module's
  `+0x74` (OBSERVED in I-0114 series 9: it flips by π at each leg's reversal), toward the nearest living other character of
  `VisitTestCharacters`, held 1.5 s, and logs the heading 0.25 s after the hold (`test_face_released`). Its convention, facing
  (-sin h, -cos h) in x and z, so h = atan2(player x - target x, player z - target z), is OBSERVED in `i114-cam-f60`'s rows (a
  steady walk's atan2(-dx, -dz) equals the heading, 0.001 rad at the 90th percentile; the step review's recount: headings near
  0 and π only, so another direction was the form's extrapolation). The runner's `M1` word is a middle click (Run-AlphaCast's plan
  key 4); the families with `M1` click once, after their first `F1`, and leave out the walk's own lock-on (a second click would
  release a lock).
- **The runs** (`i140-t20`, 2026-11-17 03:38-04:14, `build-t20b` = `a35c0a0cf`'s module, a cache build without the tests, of the
  step review's fixes; the ASUS window, offline, the stats-99 copy restored by hash, `t20/audit1.log`): eight runs, exit 0, no
  fault, every restoration ok, every F1 word applied in order. Every `f` row was kept by the game: each `test_face_released`
  heading equals the one written to the fourth decimal (0.0029, 0.0028, 0.0025, 1.9656, 1.9657 natively; the same rows in the
  original), and the player then walked toward the target with each swing (3.57, 2.27, 1.47 m).
- **The statuses on an NPC (keeper-status2): taken in both within 3 to 6 HP; their look not judged.** An `n` row's HP is read
  before its add. The rows in order (native, original): the character at the Fire Keeper's place (INFERRED; HP 307) at 307 when
  poison was added; cast 1's swing (a miss, INFERRED from keeper-infusions2's miss at the same 3.57 m); 250 and 250 at the next
  `f`; 238 and 241 when frost was added; cast 2's swing; 69 and 72 at the next `f`; 57 and 63 when bleed was added; cast 3's
  swing; then no longer in the list (no longer living, INFERRED). The next `N` rows went to a character of HP 239 at 10.25 m,
  239 when bleed was added and 104 at the next row in both renderers (cast 4's swing, thrown at 10.25 m, could not reach it,
  INFERRED):
  a 135 HP drop OBSERVED in the rows, its being bleed's burst INFERRED, not in view (`t20/audit1.log`). The film (`crops/kstatus2-1-3.png`, the whole window) shows the
  player facing her and the game's « Talk » prompt, but she is dark and mostly behind the player: the statuses' look on her is
  not judged.
- **The infused hits (keeper-infusions2): Chaos and Lightning alike.** The probes (P1 before each swing): the Fire Longsword
  missed (3.57 m, HP unchanged), the Chaos Longsword hit (307 to 108 in both), the Lightning one hit and she left the list;
  then the nearest was a character of HP 239 whom the probes found unchanged from 10.89 m down to 3.59 m (the last swing, after
  the last probe, unjudged). The strips (`crops/kinf2-chaos-lightning.png`, 200 ms a
  frame, native above): the Chaos hit's fire burst and smoke, the Lightning hit's yellow arcs on her, alike in both (OBSERVED
  by the pilot's eye at 214x128 a frame). The other infusions (Fire, Dark, Poison, Blood, Blessed) not reached on a target.
- **The bow on a target (bows3): an arrow's hit alike, the art not reached.** Two hands by `grip`, turned toward her: the
  Standard Arrow hit her in both (the blood spurt and her HP bar, `crops/bows3-2.png`); she then left the list. The Longbow's
  art by Shift and the right button spent no FP in either (cast 3); the stance of 1500 ms (cast 5, the last) has no FP row.
- **The Rope Firebomb (lead-rope4): not shown differing; the lead is closed.** Three uses a renderer, the final review's
  counting (`t20/fire60-rope4.py`, `fire60-rope4.txt`: 60 frames/s, the same thresholds and window): a « swell » read as the
  largest count of frames +15 to +28 after the blast (the first count above 500; 0.25 to 0.47 s) above the smallest of frames
  +3 to +14, the rule coded in `t20/swell-rule.py` (its table `swell-rule.txt`) and run over all 14 casts of tranches 19 and 20
  (the final review's H1: tranche 19's tallies came from other readings): natively a swell in 6 of 7 casts (one, `i140-t19b`
  cast 1, by 2 pixels: 1,765 over 1,763), in the original in 3 of 7. That rule is one of 16 variants of a sweep (the blast as the
  first count above 200, 500 or 1,000 or as the first peak; four window pairs; `t20/swell-sweep.py`, `swell-sweep.txt`, the
  integrity review's H1): with the blast at the first peak, tranche 19's casts read the original 4 of 4 or 2 of 4 and the
  native 0 or 1 of 4 (tranche 19's reading reproduced), with a threshold the original 2 of 4 and the native 2 or 3 of 4; the
  coded rule (a late maximum over an early minimum) misses a local late rise on a falling curve, which tranche 19 counted
  (`i140-t19b` original cast 2: 1,018 rising to 1,268 scores no). In every variant tranche 20's three throws read native 3 of 3,
  original 1 of 3. So the native can draw the late swell (OBSERVED in tranche 20's throws): the lead « missing natively » is
  closed on that; how often each renderer draws it is not measured (the tallies over both tranches depend on the variant);
  the native's fire as long or longer in this run (34, 34, 36 counts above 500 against 20, 32, 32, counted in `fire60-rope4.txt`).
- **The heading's direction** at 1.92 and 1.97 rad (off the axis i114-cam-f60's rows cover): the player walked toward the
  character it was turned to (10.89, 9.06, 7.23, 5.41, 3.59 m in keeper-infusions2 at 1.92 rad; 10.25 to 8.95 m in keeper-status2
  at 1.97), OBSERVED in the rows; at 1.63 rad (bows3) only the release row (the heading kept).
- **Step review** (fresh Opus, high; `c1d70cc8d..76c8b7e8c`, the heading's convention recomputed by it from `i114-cam-f60`): no
  High. M1, the middle click toggles a lock (the walk's own lock-on, then an `M1` before each swing, would release it every other
  cast): `M1` on each family's first `F1` only, the walk's lock-on left out for them. L1, `nan` in the rows (not JSON): 0 or null.
  L2, the release row 2.0 s after the press raced the runner's next key: 0.25 s after the hold (1.75 s). L3, the convention's
  evidence covers headings near 0 and π only: said (the release rows above then held at 1.97 rad too). L4, L5 (comments, bows3's
  FP line): fixed. L6 (a new `f` during a pending release row erases it; the plans space them 4 s apart): left, said here.

## The merged gate's witness (`i140-e70`, 2026-11-16)

The fresh E thread (branch `claude/unruffled-nobel-ed8b36`), from `main` `ffd132c95`. One native run, 04:20-04:25 (Paris), the ASUS
window, offline, Alpha's all-pyromancies copy, `Run-I113Warp -VideoLossless -VideoTravel` on the launcher's build of `main`
(`e5aa7374d`, rebuilt 03:54; module SHA-256 `0AD712F6…913E60`): exit 0, no fault, restored by hash. No measure.

- **The mask's scope (OBSERVED, the journal):** `native_taa_fade_mask` first at serial 7594 with 42 rows; `native_taa_frames` at
  3,600 frames `fade_frames` 0, at 7,200 frames 79 (3,318 rows), at 10,800 frames 144 (6,048 rows): the first travel marked 79
  frames and the second 65, 42 rows each (INFERRED from the counters at 3,600-frame marks) (the character's meshes, INFERRED), nothing between them (the bonfires' coiled swords and the
  cloud bank unmarked); no `native_taa_fade_mask_off`.
- **The fade (OBSERVED by eye on the lossless frames):** `evidence/I-0140/e70/hair-e64rows-e70main.png`, `i140-e64`'s sheet (native
  `no-taa`, the former default, the fix on `build-e66`, its view) with `i140-e70`'s row last: under the TAA the hair breaks into the
  dots native `no-taa` shows, as on `build-e66`.
- **The crawl over Archdragon Peak's cloud bank:** 0.737 over 477 frames (e62's command; the former default 0.735, `i140-e64`
  0.709; one run each, no spread known: no change shown).
- So gap 23's merged gate works in a fast travel; left, Thomas's eye in play (D-0181). Readings: `evidence/I-0140/e70/readings.txt`.

## Father Ariandel's limbs dark natively (the tour's relay; `i140-e71`, 2026-11-16)

The tour (I-0139 RESULT « Sister Friede by her own event », `sheets-s6/friede-ariandel.jpg`) saw Father Ariandel's limbs, pale flesh
with ember cracks in the original, drawn as dark flaps without their glow natively, and relayed it as « a missing emissive,
INFERRED, not read ». Read here:

- **The material (STATIC, `evidence/I-0140/e71/c6010-meshes.tsv`, a Sonnet subagent's SoulsFormats read of c6010.flver in memory;
  I-0063's `textures.tsv`):** 361 bones, 24 meshes, all Dynamic; the body, fur, hair and cloth materials are `C[ARSN]_e`
  (`GXFlver_ColDifSpcBumpIbl_Bloody`, alpha-tested at 128: `_e` is the alpha test, not an emissive) or `C[ARSN]`, with diffuse,
  specular, shininess and normal textures and no emissive texture; 11 cloth meshes (`C[ARSN]_e_Cloth`, two streams); 8 meshes use
  16-bit bone indices (up to 295). So no emissive map is missing: the original's glow is not a texture of the model.
- **The run (`i140-e71-ariandel-nat`, 04:33-04:38, the ASUS window, offline, Eta's save copy restored by hash, `build-e69`, the
  tour's place: the travel to Sister Friede, the boss revived, the teleport facing Father Ariandel in his chair, F8: four PIX
  captures, `pix-capture.flag`, `pix-f8.txt` « 30 6 »).** The game closed at the end on an access violation inside
  `WinPixGpuCapturer.dll` (one crash dump; the known fault of a PIX run's exit, LIVE-RUN-NOTES), after the four captures were written.
- **The capture `pix-f8-12044` (pixtool; `evidence/I-0140/e71/pix/`), OBSERVED:** the G-buffer after the last opaque draw (global
  id 196) holds the limbs' true shape and material: gnarled pale arms and hands in the normals (RT0) and the material colour (RT2,
  about 0.45 grey at pixel (440, 490) of the 1280x720 target, on the left arm, against the snow's 0.75 at (470, 600): ffmpeg's
  one-pixel crop of the saved PNGs, 16-bit values over 65535); the lit HDR before the post chain (id 384) draws the same
  limbs, with their detail, but dark (about 0.13 at the same pixel, read the same way); the final picture shows them as dark, flat shapes, where the
  original shows them pale and lit and the bowl dark (`ariandel-albedo-hdr-final-orig.png`: the material colour, the lit HDR, the
  final picture, the original's `i139-t3f` frame). The bowl is the reverse: bright gold natively, dark in the original.
- **The lights near him:** the map's two window spot lights (MSB regions `GD_SFXspot` « boss area window A, B », SFX 845210 and
  845211, the only 11000 effect spot lights of m45, `msb.tsv`, `fxr.tsv`) at (-262.0, 40.8, -230.0) and (-256.9, 38.7, -229.7), and
  `native_light_visibility` keeps two spot lights there natively (spheres at -251.9 35.7 -229.9 and -256.0 40.8 -230.0, `kept` 1;
  that they are those two is INFERRED from their places): the native light list holds them.
- So the limbs are not a geometry, skinning, cloth or material fault: they are drawn and lit, but dark. The darkness is the
  lighting's, the family of STUDY-ALL-GAPS row 4 (characters darker or warmer than the original under D-0262's lighting; the player
  beside him is darker too); the orange rim on the original's right arm is a light this run did not identify (INFERRED: a light, not
  the model). Not E's: relayed to R (workstream 7 and 12) with this evidence. Not run: the original's own frame in RenderDoc.

## Two items of STUDY-NARROW-WITNESS (I-0137, D-0290), read in the journals (2026-11-15)

- **Row 6, I-0121 D2** (an effect statement whose FFX texture or SYSTEX_WHITE is not resident is dropped for that frame,
  `21_effects.inc` `Resolve`): its counter `unresolved_textures` (in `native_ffx_execute`'s summary, written with `ffx-record`) is
  0 in every one of its 212 rows over the 15 I-0140 runs that carry it (Firelink `i140-e6`, Sister Friede and the Shared Grave
  `i140-e5*`, `i140-e10`, Archdragon Peak with Fire Orb `i140-e21`, `i140-e23`, `i140-e25`, the cutscenes 32000000, 32000010 and
  51000000 `i140-e20`, `i140-e22`, `i140-e24`, `i140-e25`; OBSERVED, grep of the journals). So no effect was dropped there for a
  texture not resident: the « a few particle draws missing or untextured » of STUDY-NOT-YET-NATIVE row 8 is not this drop at those
  places (INFERRED for the places not run).
- **Row 8, I-0137 tranche 7** (a world frame with no UI packet presented without the overlay): its rows (`native_ui_overlay_absent`,
  6 to 10 a run) are in every cutscene run of the tour (`i139-t3m-*`), and the tour's comparison of those cutscenes
  (`evidence/I-0139/artifacts/cutscenes-compare-main.md`, rows 30000040, 34010000, 37000030, 45000000) found the same subtitle
  lines natively and in the original at the matched moments, the subtitles drawn natively (I-0139 RESULT); OBSERVED by the tour,
  read here. So the cutscene half of the inference holds where a subtitle shows. Friede's second phase (2026-11-15, tranche 11,
  from runs already taken, no new run): the tour's `i139-v2-b22-friede-nat` (`build-tour2`, I-0137 tranche 7 merged; its step 10,
  30 s, after the transition cutscene 45000010) and `i139-t2e-b22-friede-orig` (its step 11, 30 s) both show the HUD back with the
  boss bar « Father Ariandel and Friede » at the same place and size (`evidence/I-0140/sheets-friede/phase-bar-30.png`, the game
  windows cropped; the contact sheets beside it), OBSERVED. So the overlay comes back after a boss's transition cutscene as in the
  original: row 8 holds at both of its places (INFERRED for the places not run).

## Next

1. The ending's body still brighter in its second view (tranche 3): `i140-e12` shows its material neutral and its lit HDR red-dominant
   (the hand's flame, INFERRED), so the lighting's, R's (ROADMAP workstream 7 step 7).
2. The cutscenes' traced sun (R, ROADMAP workstream 7 step 7): tranche 7 made the native's shadow source the original's (the keys,
   the Shadow Light Id, the cutscene file's binding and rotation; `i140-e16`, `i140-e17`). Left: option 1 (above, « Thomas's choice
   for the rest »: where the ShadowParam names no light set, light 0 of the characters' set), the valley of 31000010 (the original's
   per-pixel set 300 and cascades that stop at 10 to 60 m, INFERRED) and the wyverns in the fog; 31000010's shots run later natively
   than in the original (`sheets-e17/c31000010-30.png`: another shot at 30 s, not read). The player « wrong after a scene »
   (`i140-e11`) is R's too.
3. A view of a Decal overlay itself (Firelink's skinned objects, a character's eyebrows) for Thomas's eye (tranche 6). The Road of
   Sacrifices' moss: drawn, lost in the lighting's tint (above), R's.
4. Leads of tranche 7, not modelled: E00270 (from DFD270) writing a stage's own sun into the context's ShadowParam record; light 0's
   node `+0xA0` angle offset (its writer unread); MapOverride_forRemo's own binding (no record).
5. ROADMAP workstream 5's other steps. The blood's fade is native since I-0105 tranche 4 (the type 1 erase and the type 2 clear;
   its item here was stale); the paint seen in both modes on a struck player kept alive by F4 (tranche 35), its colour and extent
   not compared (the fights not at the same moments: a scripted hit at a fixed moment would allow it). The menu views' regions (tranche 28): compared in tranche 30
   (the equipment page has no 3D view); left: the menu forward rows' constant key {1, 10} against the manager's region. The env-map bank (tranche 29): 40DD0 in a cutscene (I-0141's acts) and 1A86C0 from a context the native does not own, not yet met. The single-model NPC phantoms: D07140 (entity slot 24) writes the draw state holder's `+0x94`,
   `+0x95`, `+0x97` and `+0x828` through D40080 and D3FE90, genuine code that runs natively too (READ, GhidraMCP); no visible case
   is known, so no work without a witness. Gap 16: the 608 quads read (not degenerate, `i140-e21`) and the effects' fog
   given (tranche 8); at 1920x1080 its rim of the original's order on the effects' clock (tranche 26): left, Thomas's eye and a 2560x1440
   pair. The burn MTD (the
   burn look's reader exists since I-0105 tranche 3): its four objects (`o002487`, `o002488` with `M[burn]_e_cloth`, `o962426`
   with `M[burn]`, `o962486`; I-0063's scan, `flver.tsv`) are declared in the model lists of `m32_00` and `m38_00` but placed by no
   part of any MSB (`msb.tsv`: no part row names them; OBSERVED in the scan, 2026-11-15), so no burning object can be met in a
   zone (a Remo cutscene or an event spawning them not read): left without a witness, like the phantoms. Gap 17 (tranche 11,
   2026-11-15 14:02-14:06, the ASUS in a 1280x720 window, offline, `build-e58`, Thomas's character copy of 2026-10-31 08:51 casting
   Crystal Magic Weapon at Firelink, natively then in the original; `runs-e32.log`, `evidence/I-0140/run-e32.ps1`; sheet
   `sheets-e32/crystal-nat-top-orig-bottom.png`): both exit 0, restored. The crystals' saturated pixels (`crystal-hue.py`: blue above
   red by 60 and green by 40, in the window's `crop=300:330:480:180` around the blade) at 400, 900, 1,600 and 3,000 ms: blue-to-red
   1.88, 1.81, 1.88, 1.83 natively against 1.84, 1.80, 1.71, 1.70; green-to-red 1.22 to 1.24 against 1.20 to 1.22; their blue
   154 to 175 against 161 to 178 (OBSERVED; the poses not at the same instant). So the hue Thomas saw paler on 2026-10-31 is now
   within a few per cent, a little darker natively; but the whole window is brighter natively (mean luma 63.6 and 67.1 against
   48.0 and 49.8, the automatic exposure, D-0280), so against the frame the crystals are about 30 % darker natively (Thomas,
   2026-11-15 19:05: « plus sombre, mais est-ce normal ? »). READ: on a traced frame the native zeroes lights 1 to 3 (the fill
   lights) of every forward row's cbLight set but the cutscenes' own (`12_scene_collect.inc`, CollectNativeForwardLights, D-0262), and
   the crystals are forward rows (pass 0x29): they lose the fill light the original gives them, by D-0262; that this alone makes
   the 30 % is INFERRED (no A/B: no flag keeps the fill lights). Offered to Thomas (19:15): a test flag and an A/B, or an exception
   for the effects as D-0279's for the cutscenes. Thomas (19:09): « Ok ça marche, de toute façon les effets seront eux mêmes des
   sources lumineuses donc ça redeviendra comme avant. C'est bon pour moi »: gap 17 closed (the effects as light sources are
   workstream 12's). Gap 23 fixed under the TAA (tranche 12), its merged gate witnessed (`i140-e70`, « The merged gate's witness »),
   validated by Thomas in play (2026-11-17 05:47, « J'ai fait ke voyage en TAA, validé »). The spells' and arts' audit: tranches 13 to 23 (below; the Crucifix's violet swirl, both rope
   firebombs, Havel's dust, Farron's and Splitleaf's arts, the Crucifix's white ring, the Chaos and Lightning hits (tranche 20), the Fire,
   Dark, Blessed, Poison and Blood hits (tranche 22) and Crystal (tranche 23) alike by the pilot's eye, Deep alike in kind with the
   native's violet brighter (tranche 23)); the bows' arts, the bolts on a target and the statuses' look on an enemy closed
   by Thomas in his own play (tranche 22, 09:00, « Pour moi tout est bon »), the embers on the clothes closed by Thomas (tranche
   23: his character had died, no gap); tranches 22 and 23's crops (the Crucifix's ring, the five infused hits, Crystal, the Deep hit's brighter violet) closed by
   Thomas's eye (tranche 24, 2026-11-20 09:36); frostbite by a weapon closed by Thomas (11:24, « D'accord pour le gel », on the pilot's proposal; closed by
   default at 11:05 while he was away: the frost status's look already alike by a SpEffect, tranche 15; four Irithyll swings 23 s apart did not
   proc it; a rerun would need quick consecutive swings, `H1` before each, `I1` once); the maggots closed by Thomas's eye (tranche 21); the Ariandel maggots (5105, the m45 bank: at Ariandel);
   Yhorm's Great Machete's dark blade and the hits' brighter flash (R's). Left before workstream 5 closes (D-0181): Thomas's eye on
   tranches 13 to 21's strips, where 56 spells were judged alike by a Sonnet triage of the sheets alone (INFERRED), Chameleon and
   Firestorm not compared and two shield or Stance arts not used. The blood spurt denser natively at 100 ms (tranche 24), open.
6. The effects' light (32770, which 264230 calls four times on the same empty engine bank): taken by R (2026-11-15, the smoke of
   32000000 at 28 to 35 s whiter and denser natively), from this branch.
7. Tranche 8's merge with I-0141 tranches 3 to 5: done in this branch (the overridden keys from the banks; the act's engine nodes
   removed at the withdrawal by R's tranche 4, the pass-over a guard since).
8. 32000000's flat, see-through close-up player (the tour's defect, still there, `i140-e24` 50 s): the original's close-up is as
   pale (`i140-e27` 45 s); a shot showing the head in both modes first, then, if they differ, pictures timed to it. The haze shot's
   grey veil: INFERRED, two readings (the world's fog, or the flash's afterglow out of phase: R's, 2026-11-15), R's (I-0141).
9. The credits' « MOVIE » plate: fixed in tranche 9 and witnessed (`i140-e28`, `i140-e29`); the rebind once planned here is not
   needed (the per-update imports follow the open, as in October).
   The proxy family's other classes (Middle, DummyBlink, BaseProxy): in every journal kept under `evidence/I-0066/artifacts`
   (2026-11-15 grep), `native_ui_asset_unavailable` names only the movie proxy's class (10 rows): no other proxy ever reached the
   import without a native record, so nothing to change without a case.

10. The light shafts 10003 (class 3D65A88): native since tranche 10 (above), witnessed in 30000010 and 38000020; left: Thomas's
   eye on their brightness (`sheets-e31/`).
11. The menu views' anti-aliasing (D-0305): done in tranche 40 (our TAA, DLAA when the option runs DLSS); the TAA's reconstruction
   tighter on them (D-0306, tranche 41: the body at about 0.78 of DLAA); an even supersample for the briefly drawn views tried
   and reverted (tranche 42: the face inside the noise). Left: the portrait's 2 px, with the native's collects after the game
   unlists the view (tranche 43): the original's listing rule with the native policy's reasons as named conditions, the first
   collect's missing cube among them (tranche 43's « Outcome »); the wobble between openings (0.75 px, 0.5 px frozen; not the
   jitter's phases, tranche 42); the body's 0.25 to 0.75 px vertical under the TAA (tranche 43); the dark bin's half not the DLAA's (the cloth's pose, INFERRED; a comparison at the
   same pose).

## Reviews

**Tranches 8 and 9 at the merge.** Tranche 8's step review (fresh Opus, high, `1940c865b..35e113c7e`; H1 the store, M1, M2, L1 to
L5), its first final review (`main...9234959f9`; M1 a two-shot sheet, F1 to F5 of the integrity review), its second
(`main...173ac65e8`; H1 the order of merges, M1 the store, M2 the witness) and tranche 9's step review (`fd6443425..29a3d9492`; M1
Next 9, M2 the labels, L1 to L4) are answered in their sections above. The final review of both on the final tree (fresh Opus,
xhigh, `main...5de82d99d`, `build-e49`): no High; M1 (the thread-affinity scope on the wrapper, not on NativeEngineFogAnswer and
LogEffectFogKey) fixed and re-keyed; M2 (the movie proxy after its close: A2's case reversed, unstated) answered: the original's
STEP_Finish_Play EAF670 publishes a null frame (EADE50 clears +0x18), so after the close it forwards to SYSTEX_BLACK too (READ),
said in the code and here; M3 (the Conclusion, Next, Evidence, Uncertainty and STATE stale) rewritten; L1 (a comment saying the
act's nodes stay) and L2 (R's withdrawal probe through the detour) fixed; L5 (the witness map before its bound) and L7 (the
per-update import row unbounded) bounded; L3, L4 and L8 corrected in the text; L6 (no native witness rows with the A/B flag),
L9 (the per-lookup lock, the duplicated SYSTEX_BLACK scan, the answered witnesses kept behind `ffx-record`) and L10 (R's STATE
rows) left: recorded. The step review of those fixes (`5de82d99d..c8d00fa4a`): M1 (the bounded import row breaking MENUS-MAP
§52's count) fixed (a row for each new texture generation, an `n` field); M2 (the `build-e44` form's sentences read as current)
marked superseded; L1 (word 7) and L2 (the veil told three ways) written; L3 and L4 fixed; L5 (the comment's « without a frame »)
reworded. The integrity review (`ad15b002a...7a36f6170`): no High; M1 (the Conclusion silent on the lost veil) written; its
lows (the YAVG crops, the 640x360 count, the final tree unrun, « as before », 137910's bank, word 10's residue, the row count's
basis) written above.

- Tranche 1's step review (fresh, read-only, on `5132d393..374cbfa0`): no high finding. M1 (the readers of the 20 other slots and
  964F0's callers not read): answered by writing only the six slots whose 36 readers were listed, the others left at -1, and by reading
  964F0's and 1F7090's callers, 6CAD0 and 151810 (above). L1 (1F7090's callers and body not in the evidence): read (26 callers, the id
  the registry's size before its push). L2 (the comment said the readers « find nothing », but 97600 and 96560 write): reworded. L3
  (18EE30's writes to `+0x70/+0x74`): recorded as INFERRED. L4 (the face row's cap of 64 (owner, model) pairs could hide the player's
  row after a tour): counted first, by owner (by owner and model, 1,024 pairs, after the final review). The C4456 warning of the first form fixed (`ae03aee3`).
- Tranche 1's final review (fresh, xhigh, on `main...14ac452a`, the final tree with I-0138 tranche 4 part 4a): no high finding.
  M1 (nothing recorded about the map pieces, whose draw colour the change now takes from the item): the arrival views and the
  `native_face_stream` rows recorded above. L1 (`native_face_unbound` on by default, against « diagnostics only behind their flag »;
  the bindings without an owner sharing one counter): behind `face-unbound-observe.flag`, counted by owner and model. L2 (« its first
  two frames », « from the cutscene's start », « shaded as the original's » claimed more than the rows and sheets show): reworded.
  L3 (the readings the GX fix's safety rests on not in the evidence store): `evidence/I-0140/ghidra-gx-groups/pilot-readings.md`
  (1F7090's callers and body, 48B80's calls, 964F0's and 93CE0's callers: no runtime reset, 6CAD0, 151810, 14EB50, 1E1960), and
  `gparam-index/` listed. L4 (comments saying « menu parts », the row `native_menu_face_stream` firing in cutscenes): reworded, the
  row renamed `native_face_stream` (`MENU-OFFSCREEN-MODELS.md` keeps the old name as its history). L5 (status.json, the investigations
  index): updated. L6 (479a1d8a and 3ef31491 had no step review): this final review covers them. L7 (the merged tree « not
  compiled »): read mid-build; `build-e5` of `14ac452a` completed (exit 0, its unit tests, no compiler error), and `build-e6` of the
  answered tree.
- Tranche 1's integrity review (on `main...14ac452a`): no high or medium finding; no test, gate, reader or runner touched, no case keyed
  to a scene, the runs' `module_sha256` the builds named. Its lows: the face row's sampling (reworded with the final review's L2), the
  builds each witness ran on (below), « by eye » kept as such, no brightness claim on it.
- The step review of the final review's code answers (`14ac452a..c695b88b`): no high or medium finding. Its lows: the runner could
  neither set nor clear `face-unbound-observe.flag` (added to `Run-StartupTrace.ps1`'s pattern and stale-flag list); the cap of 256
  (owner, model) pairs could fill before a cutscene's owner (1,024); two comments still saying « menu part » (`05_selected_scene.inc`,
  `16_offscreen_and_movies.inc`, reworded); this record's « by default » (corrected).

- Tranche 2's final review (fresh, xhigh, on `main...fde3a37c`): no high finding; frames without 0x21 draws behave as before (the same
  commands, passes and key). M1 (the final form never run): its witness first in Next, before the merge. M2 (0x21 drawn after the water
  and the TAA base): recorded above and in Next. Lows: the retried preparation rethrows its own failure (an `exception_ptr`: the
  failure's cause kept); « none seen » for 0x21 reading t3 reworded « not checked »; `effect_stage.h`'s pipeline kinds, ROADMAP's step 1,
  this record's Conclusion and the sweep sheet's claim corrected; `depth_work` removed (an alias); the diagnostics on bit-61 frames
  (a row run counted for a preparation alone, the depth copy inside effect-measure's interval, the scan's kind-0 point before 0x21's
  draws) noted here, not changed.
- Tranche 2's integrity review (on `main...0c511cd2`): no high or medium finding; no test, gate or reader weakened, no case keyed to a
  scene, the trial flag gone. Its lows answered: the witness's counted share stated, the region means' file named, the final form's
  still cited.
- Tranche 3's step review (`20f4f9c0..475ea89d`, with tranche 2b's): above, in « Tranche 3 » (its MEDIUM on the ASM parts answered by
  `i140-e8` within its limit) and « Tranche 2b » (its HIGH withdrew the reversal).
- Tranche 3's integrity review (on `main...e59c5a8d`): no high finding. M1 (the embers credited to the fix on a tree that also carries
  R's bank, while e7 had been judged without them) and L1 (the e7 judgement): answered by the trousers' crop (e7 shows them) and the
  attribution marked INFERRED; L2: the region means added; L3: the ModelDrawParam chain's status (READ by the subagent, its readings not
  kept); L5: « one MaterialParam row in each component's table ».
- Tranche 3's final review (fresh, xhigh, on `main...51660483`): no high finding; the fix correct, safe, a mechanism, no regression
  by construction. M1 (the summary sections stale): the Conclusion, « The cause », Reviews, Evidence and `status.json` updated. L1
  (the +0xA0 fields printing « (unowned) »): removed. L2 (the witnesses' counters): the component's row once per item on the main
  walks, the animated row per (model, material) for four values. L3: wording (by eye, INFERRED). L4 (only `[item+0x50]` consulted):
  `[item+0x48]`'s component (ctor `140D2A7A0`, vtable `1429500E0`) has a no-op +0x10 (`140D28D70`: `ret`, READ); the 0xA8-byte
  components (key `14294FB10`, ctor encrypted) not read (K-0494's limit). L5 (a torn read of the component's table while the act edits
  it would lose the authored values for a frame): not established that they run at once; no `native_selected_matdyn_unavailable` row in
  e7 or e8. L6: K-0494 added. The witness widening `887f7d5c` had no step review of its own: this review covers it.
- Tranche 4's step review: above, in « Tranche 4 ». Its integrity review (on `main...57c8abd2`): no high finding; M (the player's
  cause given as observed) and its lows (the Shared Grave baseline, the 31000010 attribution) answered in « Next » 4 (now « The tour's
cutscene defects and the player's red-orange, as read before tranche 5 ») and ROADMAP
  workstream 7 step 7 (the cause INFERRED, I-0113 tranche 34's contrary measure, a `no-rt-scene` world witness to settle it). Its final
  review (fresh, xhigh, on `main...6cb100b5`): no high or medium finding; criteria 1 to 5 met. L1 (a copy's failure inside the tail
  taken by Encode's catch): kept as the frame's (`try` around the tail); L2, L3: the comments; L4 (`rows21_until` repeats
  `rows_until`): left; L5: « Not observed » widened; L6 to L9: this record's Conclusion, Reviews, Evidence, `status.json`, the sheets'
  columns, e9's start, STATE at the checkpoint. Built after it: `build-e22`.
- Tranche 5's step review (fresh, on `a1f087169..67887543a`): no high finding; criteria (a) to (e) met (the 26-slot table and its
  static_assert, no cost without the flags, the latch and the witness's statics, the runner, nothing else changed). M1 (31000010 was
  said to have written no G-buffer picture: it wrote 16, a listing cut short) and L1 (« zero-fade throughout ») corrected above, the
  c31 G-buffer sheets made; M2 (the slots' outcome stated before any run): worded as expected, a start and an area change on the
  final build before the merge; L2 (Lens Simulation's reader is 140185280 through 4A060's tail, the fifth caller CDCCD0, the readings
  not saved, « cleared » for a possible effect): corrected, `pilot-readings.md` saved; L3 (the scans' limit): stated; L4 (the witness's
  lock and name read on every admission of every `light-observe` run, lowercase only): its own flag, `C` accepted. Optimisation pass:
  the additions are one startup table, a latch read only under its flag, and two flag-gated witnesses.
- Tranche 5's integrity review (on `main...cdbac67c0`): no high finding; nothing weakened, no scene-specific case, no work cut, the
  numbers checked against the journals. Its M (the ending's body handed to R without a reading; « 4A060 runs natively » a static
  reading) answered: the 41000000 reading added, « reachable, not observed ». Its lows: the runner's edit (two flag names added to its
  pattern and its stale-flag list, nothing else), the picture count (14 for 32000010), ROADMAP and STATE worded as INFERRED, the
  original's run named and « by eye » said.
- Tranche 5's final review (fresh, xhigh, on `main...cdbac67c0`): no high finding; the slot table matches `slot_table.tsv` and 48B80's
  order, the startup refusal unchanged for an engine that resets the 26 slots, no cost or behaviour without the flags. M1 (causes
  written as fact, the ending's body unread and missing from ROADMAP): STATE, ROADMAP step 7 and Next reworded, the reading added.
  M2 (the slots' run promised, not done): the start and travel on the final build before the merge, recorded below. L1 (CE6350's
  file switch, not its unbinding, calls D27040; the area lists the genuine D26AB0/D26C00's; « reachable »; « as in the original »)
  and L2 (D's route 49450) corrected in the code's comment, this record and `pilot-readings.md`; its lead (a native D27040 leaves the
  matching `gparam_scene_bindings` entry, kind 8, unread by the kind-7 apply) recorded, not read. L3, L4 (ROADMAP and the Conclusion
  aligned; the moved « Next » 4 pointed to), L5 (STATE's duplicated sentence removed), L6 (the flag without a cutscene takes no
  picture: said in the comment), L7 (Evidence and Uncertainty).
- Tranche 10's final review (fresh, xhigh, on `main...ef40b7dfd`): no High. M1 (records outside the tranche still said 10003
  skipped or unread: K-0485, STUDY-ALL-GAPS row 21 and its count, the CENSUS row, STUDY-NOT-YET-NATIVE row 6, FFX-FAMILIES-PHASE2
  §6.2): all updated, the tranche's departures added to row 21. L1 (the outer render-target cache seeded for every family): fixed,
  seeded only during a light-shaft command; L2 (a family's dropping bind counted only with ffx-record): told once a class and kind
  in every run; L5 (the provider's trap table 4 slots wide): as wide as the others, the unused tokens left; L7 (the stage counts
  misquoted), L9 (Evidence, Uncertainty, status), L10 (STATE), L11 (the study's witness lines), L12 (D-0292's chronology, the usage
  tool named): corrected. Left, recorded: L3 (a scissor dropped in 0x31's layer: no shaft seen there; an opaque composite there
  would cover the layer), L4 (a failed target creation retried each frame, told in `last_failure`), L6 (no unit test for the pure
  helpers), L8 (38000020's still shows the haze but does not isolate the shaft: no same-build A/B with `no-ffx-lightshafts.flag`).
  Fixes `6fb5d82db` (code) and `3b84254e2` (records); build `build-e57` (tests passed). Its step review (the fifth): no High, no
  Medium; L1 (an allocation in the noexcept render-target hook): a fixed table; L2 (the thread not said), L3 (« makes » for « may
  make », the study's row wording), L4 (a comment): corrected, `e249acbc8`; a sixth step review on that change.
- Tranche 10's integrity review (on `main...3b84254e2`): no High. M1 (the merged code is not the witnessed code: `6fb5d82db` and
  `e249acbc8` came after `build-e55`): said in the Conclusion and the Uncertainty; M2 (a build and a step review cited before they
  existed): recorded once done (above); L3 (30000010's « before » tile another build and screen, not the same instant), L4 (the
  witness carried `ffx-record.flag`), L5 (the luma crops not given): added to the tranche's account. No weakened test, no
  scene-specific case, no work cut during a run found; its numbers held against the journals and pictures.

### Tranche 13's reviews at the merge

- **The final review** (fresh Opus, xhigh, `main...5035c51ec`, `main` = `6b512216b`): no High. M1, the Intelligence/Faith raise did
  not hold and its check could not fail: removed (the code, the rows, the list words, the series' check; the finding kept above).
  M2, buffs carry into the next cast (Dark Blade) and Chameleon's object is random: Dark Blade found not cast (below), Chameleon and
  Firestorm « not compared ». M3, stale or contradictory lines (STATE's gap 23 and spells, the Conclusion, Next 5, Evidence,
  Uncertainty, STUDY-ATTUNE): rewritten. L1 (wrong counts), L2 (the +0x2D reasoning), L8 (an INFERRED labelled OBSERVED): fixed in
  the records; L3 (the check by index: now by id), L4 (the tool undocumented: USAGE, TOOLS), L5 (no time budget: refused before the
  launch), L6 (the strip maker: step 300, no window, no intermediates), L7 (the param repository read before the lookup): fixed.
  Their step review (`5035c51ec..097fe1c3c`): no High or Medium; L2 (ids above the module's bound), L3 (the budget from the shots
  and the hold), L4 (intermediates left on a failure): fixed; L1 (the repository refusal's row lacks `listed` and `row`, so the
  checks name its cast « another spell »; the total still flags it): left, the refusal unreachable in the world where the schedule
  runs.
- **The integrity review** (`main...097fe1c3c`): F1 (High), four spells counted cast were not (no FP spent: Carthus Flame Arc,
  Lightning Blade, Darkmoon Blade, Dark Blade), and the cast-by-cast check proved the attunement, not the cast: the counts
  rewritten (81 cast, 79 alike, 2 not compared, 25 not cast), the « four cast » argument dropped, the series now counts the casts by
  their FP. F2 (Medium), « alike » rested on the Sonnet triage for most spells, six of whose notes named a difference: those six
  re-read on strips by the pilot (alike), the split stated (23 by the pilot, 56 by the triage). F3 (the spacing range), F4 (the
  Ariandel values' pixels), F5 (STUDY-ATTUNE's « every cast »): fixed. No test, gate or reader weakened; nothing keyed to one
  scene; no work cut.
- The final tree (`097fe1c3c`'s native code, the same in `48d6e53fd`): `build-e82`, 178 PASS lines, no failure.
- Merged `4ac039096` (2026-11-16 10:20); the launcher rebuilt from it (`evidence/I-0140/launcher-rebuild-4ac039096.log`: exit 0,
  178 PASS lines), start check `start-4ac039096` passed (10:29, restored; `evidence/I-0140/startcheck-4ac039096.log`).
  The disk rotation (D-0260, `Clear-DiskCache.ps1 -Apply`, 10:40): 3 build folders older than 48 h and 110 raw-frame folders older
  than 7 days, 1.8 GB; 212.5 GB free on C:.

### Tranches 14 to 16's reviews at the merge

- **Final review** (fresh Opus, xhigh; `main...59dc74a4a`, the game calls checked against the disassembly): the native tool and
  the scripts correct. H1, the weapon arts recorded as done while two batches had run: the four batches then run and recorded
  (« Tranche 16's arts batches »). M1, the Conclusion, Next, Uncertainty, Evidence, the gaps' row 19 and `status.json` still
  tranche 13's: rewritten. M2, INFERRED results stated as facts (none differing, bleed's cause, the glare's owner, the wave's
  brightness): labelled, the brightness limited to the vortex. M3, step 5's left list incomplete (projectiles, the goods' own
  use, statuses on NPCs, shield infusions, torches): listed. M4, the RESULT committed with a lone CR (the whole file rewritten in
  the diff): removed, renormalised (+169 lines). L1 (other transient refusals spend a word: left, the press-by-press check names
  it, said in Uncertainty), L2 (the equip sequence's threads not read: said in Uncertainty), L3 (the F1 retry's time: in the
  budget), L4 (stale texts: fixed), L5 (the tour's open divergence: kept in its ROADMAP line), L6 (the glare streak: named under
  workstream 11 in step 5's line), L7 (Pilot-Game's repeated Vk2 code: left, three short copies), L8 (the merged tree's build:
  `build-e90`, then `build-e91` after main's second merge, 178 PASS lines each; `build-e90.log` lost since, tranche 17's « A loss »).
- **Integrity review** (`main...c7c8101a7`): no weakened check, no scene-specific case, no work cut during a measure. F1, « the
  118 arts run » while 60 had: the 118 then run and the texts say so. F2, the arts that spent no FP not judged: 17 named, five read,
  the cause open. F3, « about 280 equips »: 248 equips and 97 adds counted in the journals. F4, « every batch »: all eight runs
  loaded. F5, the luma screen's 10 % is as large as the exposure's offset: said, and the strips read counted. F6, the FP counts
  over the judged casts (29 a batch): recounted. F7, luma figures without an output file: `e84/luma-checks.txt`. F8, the triage
  caveat outside the Conclusion: carried into ROADMAP, the index and row 19.
- **The merge**: `afa629210` (2026-11-16 18:05); the launcher rebuilt from it (`evidence/I-0140/build-launcher-afa629210.log`, 178
  PASS lines); the start check `start-afa629210` passed, restored (18:17). The disk rotation (D-0260): 668 files, 1.2 GB (one build folder
  older than 48 h, raw frames older than 7 days), 205 GB free.
### Tranche 17's reviews at the merge

- **Final review** (fresh Opus, xhigh; `main...209a99065`, the menu's calls checked in the binary by `llvm-objdump`): no High.
  M1, `L` and `R` put an arrow or a bolt into a hand: refused (400000 to 404999). M2, `Q` took any goods, spells included: refused
  from 1000000, the departure (key items) named. M3, « alike but two » overreached for the Stance arts (Rapier and Drang Twinspears
  unread, the brighter flashes dropped from ROADMAP): both read (alike, a phase apart), the flashes carried. M4, cast 1 (Warrior's
  Round Shield on the left) spent the Dagger's Quickstep cost: said, the claim narrowed. M5, counts and statements contradicted by
  the logs (14 of 16 Stance arts spent FP, the 900 ms shots kept, 17 goods-thrown casts and two stones, the runs' times, the gives
  5 and 2, the shields' no-FP casts): corrected. M6, the Conclusion, Evidence, Uncertainty, `status.json` (budget 16 of 16, now 17
  of 20), STUDY-ALL-GAPS row 19 and STATE not updated: done. L1 (goods matched only by 0x40000000 | id), L2 (dead code), L3 (the
  other 0xD and 0xE rows), L4 (the menu's clears differ by slot kind), L5 (a left release only when down), L6 (an untimed stance
  marked on its cast line and counted), L7 (a comment's order), L9 (the ammunition the copy holds no longer given), L10
  (`arts-t17.txt` read only for its families), L11 (the bolts' « target » labelled INFERRED, the bows' art claim removed), L12
  (the strong follow-up said), L13 (the loss's time, the pointer at the 14-16 reviews), L14 (F7's `a` an accessory): fixed.
- **Its fixes' step review** (fresh Opus, high; `209a99065..319566629`): no High or Medium. L1, the quick slot's selection
  (`14058F4B0`) is the tool's, not the menu's: named as a departure. L2, « neither into a hand » claimed more than the code's
  ranges: the comment says the player's ranges. L3, the runner's check did not apply the module's new refusals: it does.
- **Integrity review** (`main...cdd7c3538`): no weakened check, no scene-specific case, no work cut during a measure; the
  numbers checked against the logs and tables held. M-a, the Conclusion said the thrown goods judged before their third pass
  was read: read since (« The thrown goods, third pass »). M-b, STUDY-ALL-GAPS row 19 and the left lists dropped the thrown goods
  and Farron's and Splitleaf's arts (no FP spent): carried. M-c, the Rapier's « read » had no saved strip: saved
  (`crops/stance-11-17.png`). L-a, « 320x240 a frame » overstated most reads: the resolution given per strip. L-b, « every use
  drawn » for goods-use: 11 read, 18 by the screen alone. L-c, `arts-t17.txt` said the uses stopped under the bag: corrected.
- **The merge** (2026-11-16): `main` unchanged since `0a003878c`, so not merged into the branch; `Test-Project -MergeGate` passed on
  `6bc8b2879`; merged `757af5aca`; the launcher rebuilt from it (`evidence/I-0140/build-launcher-757af5aca.log`, 178 PASS lines);
  start check `start-757af5aca` passed, restored (23:24); `Clear-DiskCache -Apply`: 12 build folders older than 48 h and 1,080
  raw frames deleted, 12.8 GB, 193 GB free.

### Tranche 19's reviews at the merge

- **Third step review** (fresh Opus, high; `cffa8f9a5..c93672683`): no High or Medium. L1, the keeper families' header still named
  the Fire Keeper and a hit as fact: « toward -AimAt, where she is expected », « on a hit ». L2, « the same position » would refuse
  the probe pairs where a hit made her stagger: « the same character (its maximum, a position within about 1 m) ». L3, the id
  range: « below 0x0FFFFFFF ». All three comment-only, fixed.
- **Final review** (fresh Opus, xhigh; `main...0b9737ac2`, the complete build `build-t19-final`, 178 PASS lines): H1, the Rope
  Firebomb closed on a misreading: recounted at 60 frames/s (its scripts and tables kept in `t19/`), the original's late swell in
  4 of 4 casts, the native's in 2 of 4: reopened, a lead in RESULT, ROADMAP, STATE and the index. M1, the maggots called alike
  and closed on their gauge and its fall, which are game logic and interface: « the status's logic alike, the worms' draw not
  judged », left open. M2, the Crucifix closed with the original's white ring unjudged, and « at F9 no Crucifix texture » while
  11401 (442332's) was drawn: the violet swirl closed, the ring left, the F9 sentence corrected. L1, FP claims beyond the rows
  (the last casts) and the stance's R2 cost: corrected. L2, 5101 a bleed buildup, the gauge the bleed gauge: corrected. L3, the
  art id's offset is EquipParamWeapon's: corrected. L4, the `s` rows now print a target's empty fields (distance -1, HP 0): left
  (the runner reads only `id` and `applied`; said here); the comment's id range fixed. L5, the `n` and `p` rows carry no identity
  but HP and position (the ChrIns address would tell two characters apart within a run): left for the next tranche's facing
  tool, which needs it. L6, frost and bleed showed nothing on the 712-HP character: recorded. L7, `grip`'s stale « INFERRED »:
  « OBSERVED in i140-t19 ». L8, counts and dumps not kept: the fire counts kept; the param dumps are regenerated by
  `Get-ParamRows.ps1` from the game's Data0.bdt at its pinned hash (not kept, as tranche 16's were kept: a choice, said). L9,
  the maggots plan names its casts by the filler spell in the FP line: left (the line says FP does not apply to it).
- **Integrity review** (`main...6dce13a4f`, with this section): no check weakened, no special case, nothing cut. High: STATE's
  « Updated » line still called the rope firebombs closed and the maggots alike: rewritten. M1, the first series' rope paragraph
  kept a contradicted « missing natively » and numbers not reproduced: replaced by the 60 frames/s tables. M2, the Crucifix's
  « 3 of 6 against 3 of 6 » counted the edge glow tranche 18 called none: now « clearly in 1 of 6, perhaps 2; how often not
  measured », the closure kept on « the native can draw it ». L1, toxic's effect INFERRED, not seen: said. L2, the gauge counts
  held on a recount; its script and table kept (`t19/gauge.py`, `gauge.txt`). L3, the p row's `applied` fix has not run in a
  game yet (only unit-tested in `build-t19-final`): said here. L4, Farron's art judged on its raise only: said.
- **The merge** (2026-11-17): `main` unchanged since `a50d08053`; `Test-Project -MergeGate` passed on `cfa6d7597`; merged
  `66b14a459`; the launcher rebuilt (`evidence/I-0140/build-launcher-66b14a459.log`, 179 PASS lines); start check
  `start-66b14a459` passed, restored (03:16); the disk rotation (D-0260): 12.7 GB of 9 build folders and 1,467 raw frames
  deleted, 197.9 GB free on C:.  The post-review edits were records and comments only (no rebuild of the reviewed tree).

### Tranche 20's reviews at the merge

- **Integrity review** (`main...b492d085a`): no check weakened, no special case, nothing cut. High: the swell's rule was one of
  16 variants, not said, and the closure and « 6 of 7 against 3 of 7 » depended on it: the sweep filed (`t20/swell-sweep.py`),
  its spread stated, the closure rested on tranche 20's native 3 of 3, which holds in every variant, the cross-tranche tally
  dropped as a result. M1, the coded rule does not reproduce tranche 19's reading of tranche 19's casts: said, with the case.
  L1 (1.92, not 1.63 rad, for the approach distances), L2 (the fire's lengths: the direct counts), L3 (« a target's hits alike »
  in STATE and the index: the Chaos, Lightning and one arrow hit), L4 (cast 4's swing INFERRED): fixed.
- **Final review** (fresh Opus, xhigh; `main...de087a940`, `build-t20-final` 179 PASS lines): H1, « the Rope Firebomb's swell in 5
  of 7 throws in each » added tallies of two readings: the rule coded (`t20/swell-rule.py`) and run over all 14 casts, natively
  6 of 7, the original 3 of 7; the closure kept on « the native does not lack it », the throws' variation INFERRED. M1, the
  keeper-status2 rows read before each add were attributed to the wrong events and « the same HP » overstated: the rows given
  in order, « within 3 to 6 HP », the character's identity INFERRED. M2, a 135 HP drop after the second bleed row in both
  renderers left out: reported (OBSERVED in the rows, the burst INFERRED, not in view), the left item narrowed to its look. L1
  (the release row's 0.5 s in two comments), L2 (the direction off-axis: the approach distances cited, not the release rows), L3
  (bows3's FP line), L4 (keeper-infusions2's distances), L5 (« tranches 13 to 19 », STATE's open gaps and pilot branch rows),
  L7 (`k4` in `-CastPlan`'s help), L8 (arts-t17's « before each swing »): fixed. L6 (the walk's lock-on left out by a name list,
  the family names repeated in five lists, the nearest-character lambda thrice): left, a cleanup for when the families settle.

- **The merge** (2026-11-17): `main` unchanged since `c1d70cc8d`; `Test-Project -MergeGate` passed on `894e8a7c0`; merged
  `e95df269d`; the launcher rebuilt (`evidence/I-0140/build-launcher-e95df269d.log`, 179 PASS lines); start check
  `start-e95df269d` passed, restored (05:04). Work paused after it at 89 % of the weekly limit (Thomas, 2026-11-17 04:45).

### Tranche 22's reviews at the merge

- **Integrity review** (`main...70e8c9709`): no High; no check weakened, no special case (`h` runs only with the test flag), nothing
  cut, the numbers sampled held. M1, ROADMAP, STATE, the index and the Conclusion said « alike » without « by the pilot's eye »
  and left out Crystal and Deep: both added. L1, « every one spending FP » for five stances, four judged: « casts 1 to 4 ». L2,
  the ring « in every original cast »: the original's casts 4 and 5 not made out at the strip's size: said. Fixed.
- **Final review** (fresh Opus, xhigh; `main...d87e86ef1`, `build-t22-final` 179 PASS lines): no High. M1, the hand-off to
  Thomas rested on a message read out of order: based on his 08:34 reply to the three items listed. M2, Crystal and Deep never
  swung at a target: listed as left. M3, the teleport of `crucifix3` and `maggots2` was released at the start every time: removed
  from the runner, tranches 21's and 22's text and the families' comments corrected. M4, « closed » for the pilot's eye alone:
  « alike by the pilot's eye; Thomas's eye left ». M5, the keeper walk's nearest character varies from load to load: recorded,
  no fix (a target fixed by the walk is not assumed). L1 (the tools bullet: what ran), L2 (the native ring in 5 of 5 on its
  frames), L3 (cast 5's FP, the first Dagger swing's miss, the last Blood swing not judged), L4 (« compared with the maximum »:
  the reader's rule, in the runner's and the module's comments; a comment-only change of the module, not rebuilt), L5
  (arts-t17's « 135 of her 307 HP »), L6 (« tranches 13 to 20 » in STATE and the RESULT), L7 (STATE's « Updated » line ahead of
  its commit), L8 (« out of view », « beside once the camera turned »): fixed. L9 (bows4 had no step review): said in the tools
  bullet, the final review covered it.

- **The merge** (2026-11-17): `main` unchanged since `f36e4445a`; `Test-Project -MergeGate` passed on `e5b2bdf2a`; merged
  `f01033e1b`, then `ecdc3cb5b` (Thomas's weapon-art keys in LIVE-RUN-NOTES, the embered state in the left list); the launcher
  rebuilt (`evidence/I-0140/build-launcher-ecdc3cb5b.log`, 179 PASS lines); start check `start-ecdc3cb5b` passed, restored
  (09:37); `Clear-DiskCache -Apply`: nothing older than 48 h to delete.

## Evidence
- Tranche 29: `evidence/I-0140/t29/` (`series-t29.ps1`, `run-a.log`, `run-c.log`, `build-t29a.log` to `build-t29c.log`, 178 PASS
  lines each); the runs `evidence/I-0066/artifacts/i140-t29-witness-{nat,orig}` (`build-t29a`, the first form) and
  `i140-t29c-witness-{nat,orig}` (`build-t29c`); the modules `evidence/I-0140/build-t29a`, `evidence/I-0066/artifacts/build-t29b`
  (never run), `build-t29c` and `build-t29-final` (the final tree with main `6f60aed41` merged, `c4cdbb0a7`, a `-Cache` build,
  178 PASS lines, `t29/build-t29-final.log`; not run live), the object caches `evidence/I-0140/t29-cache` (`build-t29a`'s) and `evidence/I-0066/artifacts/t29-cache`.
- Tranche 28: `evidence/I-0140/t28/` (`series-t28.ps1`, `run.log`, `run-b.log`, `run-d.log`, `run-e.log`, `run-h.log`,
  `thomas-portrait-native-before-after-original.png`); the runs `evidence/I-0066/artifacts/i140-t28-thomas-nat`,
  `i140-t28b-witness-{nat,orig}`, `i140-t28d-witness-{nat,orig}`, `i140-t28e-witness-nat` (the refusal) and `i140-t28h-witness-nat`;
  the modules `evidence/I-0140/build-t28` to `build-t28h` (`build-t28*.log`, 178 PASS lines each; the runs used `build-t28b` to
  `build-t28h`), `build-t28i` (the final tree with main merged, a `-Cache` build, never run live: the merged I-0141 code and this
  tranche's together ran in no run) and `build-t28j`, `build-t28k` and `build-t28l` (the final review's fixes, then each step review's: `272666d03`'s,
  `9c38eb479`'s code, the third `e534fce21`'s; `t28/build-t28j.log` to `build-t28l.log`, 178 PASS lines each), `build-t28m` to
  `build-t28o` (the third and fourth step reviews' fixes with main `407670f32` merged, then the final tree `b4a1311c6`; 179 PASS
  lines each, the merged I-0141 test included), none run live; the final review's report from E7 `t28/final-review-677bd73d4.md`.
- Tranche 27: `evidence/I-0140/t27/` (`series-t27.ps1`, `run.log`, `run-b.log`, `portrait-yuv.txt`, the sheets); the runs
  `evidence/I-0066/artifacts/i140-t27-{delta,thomas,friede}-{nat,orig}` and `i140-t27b-{delta,thomas,paint}-{nat,orig}`; the module
  `evidence/I-0140/build-t27` (main's code: a first full build, `build-t27.log`, edited mid-build when the witness was withdrawn,
  then rebuilt, `build-t27b.log`, module SHA-256 217FA43B…, the one both series ran).
- Tranche 26: `evidence/I-0140/t26/` (`series-t26.ps1`, `run.log`, `frames-*/`, `frames523-*/`, `huecount.txt`, `huecount60-524023.txt`, `portrait-yuv.sh`, `portrait-yuv.txt`, the sheets); the
  runs `evidence/I-0066/artifacts/i140-t26-orb-near-native` and `-original`; the module `evidence/I-0140/build-t26`.
- Tranche 25: `evidence/I-0140/t25/` (`series-t25.ps1`, `series-t25b.ps1`, their logs, `crops-t39.sh`, `sheets/`); the runs
  `evidence/I-0066/artifacts/i140-t25-mist-nat`, `-notaa`, `-orig`, `-obs`, `-obs-notaa`.
- Tranche 24: `evidence/I-0140/t24/` (`run.log`, `frames-nat/`, `frames-orig/` with their `frames.tsv`, `sheets/`, `red-count.py`
  and `.tsv`, `lag-check.py` and `.tsv`); the runs `evidence/I-0066/artifacts/i140-t24-andre-paint-nat` and `-orig` (each with
  `casts.clock.json` and `casts.ffmpeg.txt`); the module `evidence/I-0140/build-t24` (the launcher's of `0860cbc8c`).
- Tranche 23: `evidence/I-0140/t23/` (`audit1.log`, `crops/`, the embered state's stills); the runs `evidence/I-0066/artifacts/i140-t23-*`.
- Tranche 22: `evidence/I-0140/t22/` (`audit1.log`, `crops/`); the runs `evidence/I-0066/artifacts/i140-t22-*`; the build
  `build-t22a` with its log; the merge build `build-t22-final` (179 PASS lines) with its log.
- Tranches 14 to 16: `evidence/I-0140/e84/` (`saves.txt`, `copies-hands.txt`, `cape-inventory-check.txt`, the param dumps
  `weapon-param.tsv`, `goods-param.tsv`, `speffect-param.tsv`, `swordarts-param.tsv`, `arts-unique.tsv`, the series' logs
  `audit3.log` to `audit8.log`, `arts-1-luma.tsv` to `arts-4-luma.tsv`, `luma-checks.txt`); strips `i140-e84-strips/`,
  `i140-e84-strips6/`, `i140-e89-strips/`, `i140-e89-crops/`; runs `evidence/I-0066/artifacts/i140-e84-*`, `i140-e88-*`,
  `i140-e89-*`; builds `build-e84` (a copy of `build-e82`) to `build-e91` with their logs; the save copies
  `.ds3-i0039-backups/thomas-alpha-cape-20261111-124806` (read only) and `thomas-alpha-cape-stats99-20261116-1500` (its
  `record.json`).

- Tranche 7: `evidence/I-0140/runs-e16.log`, `runs-e17.log`, `runs-e18.log`, `sheets-e16/`, `sheets-e17/`; the runs `evidence/I-0066/artifacts/i140-e16-*`,
  `i140-e17-*`; the builds `build-e31` to `build-e36` (`build-e33` the e16 witness, `build-e34` the e17 one, `build-e37` the final tree, the e18 witness).
- Tranche 6: `evidence/I-0140/ghidra-decal-class8/`, `runs-e14.log`, `runs-e15.log`, `sheets-e14/`, `sheets-e15/`; the runs
  `evidence/I-0066/artifacts/i140-e13-*` (before), `i140-e14-*`, `i140-e14b-*`, `i140-e15-*`; the builds `build-e27` (`fb7562181`),
  `build-e28` (`eb6851409`), `build-e29` (`64f97d975`, the final code; the build logs carry no commit, the order by their times).
- Tranche 5: `evidence/I-0140/runs-e12.log`, `sheets-e12/`, `ghidra-gx-slots20/pilot-readings.md`; the runs
  `evidence/I-0066/artifacts/i140-e12-*`; the builds `evidence/I-0140/build-e24` (the witnesses), `build-e25` (the slots),
  `build-e26` (`cdbac67c0`, the final code but for comments).
- `evidence/I-0140/census-effects.md`, `census-effects.py`; the tour's runs under `evidence/I-0066/artifacts/i139-*`.
- Tranche 4: `evidence/I-0140/runs-e9.log`, `runs-e10.log`, `sheets-e9-e10/`; the runs `evidence/I-0066/artifacts/i140-e9-*`,
  `i140-e10-*`; the builds `evidence/I-0140/build-e20`, `build-e21`, `build-e22`.
- Tranche 3: `evidence/I-0140/runs-e7.log`, `runs-e8.log`, `sheets-e7/`, `sheets-e8/` (`trousers-e2-e7-e8-orig.png`, `regions.txt`),
  `remo/` (the ending's MQB dumps), `sat-equip/`, `ghidra-materialparam/` (`pilot-readings.md`), `ghidra-d3da20-a0/`,
  `ghidra-modeldrawparam/`, `ghidra-gx-slots20/`; the runs `evidence/I-0066/artifacts/i140-e7-*`, `i140-e8-*`; the builds
  `evidence/I-0140/build-e16`, `build-e18`, `build-e19`.
- `evidence/I-0140/gparam-index/` (the first reading of the index, by the previous E thread), `ghidra-gx-groups/` (the GX group
  scans, `pilot-readings.md`), `runs-e1.log`, `runs-e2.log`, `sheets-e1/`, `sheets-e2/`; the runs
  `evidence/I-0066/artifacts/i140-e1-*`, `i140-e2-*`; the builds `evidence/I-0140/build-e3`, `build-e4`.

- Tranches 8 and 9 and their witnesses: `runs-e21.log` to `runs-e29.log`, `sheets-e24/`, `sheets-e27/`, `sheets-e28/`,
  `hue-e21-e23.txt` (the magenta counts); the runs `evidence/I-0066/artifacts/i140-e21-near-native`, `i140-e22-*` to `i140-e28-*`,
  `i66-native-i140-e29-charmake`; the builds `evidence/I-0140/build-e39` to `build-e49` and their logs (`build-e48` built while
  the merge of `main` changed its sources: not used); `build-e50` (`c8d00fa4a`) and `build-e51` (`7a36f6170`, the final tree's code).
- Tranche 10: `evidence/I-0140/ghidra-10003/271B40.c`, [STUDY-LIGHTSHAFT-10003](STUDY-LIGHTSHAFT-10003.md); the runs
  `evidence/I-0066/artifacts/i140-e30-*` (the faults, their crash reports) and `i140-e31-*`, `runs-e30.log`, `runs-e31.log`,
  `sheets-e31/`; the builds `evidence/I-0140/build-e52` to `build-e57` and their logs (`build-e55` = `7504c672d`, the witnessed code;
  `build-e56` the tree after `main`'s merge before the final review's fixes; `build-e57` = `6fb5d82db`; `build-e58` = `7b6d47f83`,
  the final tree with `main` `25f3f309b` merged, its unit tests passed).
- Tranche 10's merge: `64a93c38c`; the launcher rebuilt from it (`launcher-rebuild-64a93c38c.log`, tests passed), start check
  `start-64a93c38c` passed on the ASUS (13:40, restored; `startcheck-64a93c38c.log`); the disk rotation found nothing to remove.
- Tranche 12 (gap 23): the series and readers in `evidence/I-0140/e60/`, `e62/`, `e64/` (`series-*.ps1`, `sheets.sh`, `analyse.sh`,
  the sheets `hair-notaa-taa-full.png`, `fade-notaa-taa-nojitter-nomipbias-full.png`, `hair-notaa-taa-recon-full.png`,
  `arrive-*.png`, `hair-notaa-taa-mask-view.png`); the runs `i140-e60-warp-{taa,nojitter,nomipbias,full,notaa}-native`,
  `i140-e62-warp-{taa,full,recon,notaa}-native`, `i140-e64-warp-{mask,view}-native` under `evidence/I-0066/artifacts/`, their films
  lossless (`travel2.mp4`, `travel.mp4`: 0.4 to 2.3 GB each; e60's and e62's deleted after the merge, e64's kept);
  builds `build-e61` (the trials), `build-e62`, `build-e66` (`d19156ef3`, the tree `i140-e64` ran; removed, its module's hash in the
  runs' records), `build-e67` (unit tests), `build-e68` (failed: C4459, a parameter hiding a global), `build-e69` (the final tree).
  The readers' printed outputs (the crawl numbers, the head's Sobel) and the hair sheets' commands are in `e62/crawl-and-sheets.txt`.
- Tranche 12's merge: `e5aa7374d`; the launcher rebuilt from it (`launcher-rebuild-e5aa7374d.log`, 179 tests passed), start check
  passed on the ASUS (04:12, restored; `startcheck-e5aa7374d.log`); the e60 and e62 lossless films deleted after the merge (D-0142),
  e64's kept; the disk rotation not run (another thread's build was running).
- Father Ariandel: `evidence/I-0066/artifacts/i140-e71-ariandel-nat` (journal, crash dump), `evidence/I-0140/e71/` (the series
  script and log, `c6010-meshes.tsv` and its script, `pix/` the screenshot, event list and G-buffer and HDR targets read from
  `build-e69/pix-f8-12044.wpix`, the sheet `ariandel-albedo-hdr-final-orig.png`); the four captures (7.1 GB) in `build-e69/`.
- Tranche 20: `evidence/I-0140/t20/` (`audit1.log`, `crops/`, `fire60-rope4.py` and `fire60-rope4.txt`), the runs
  `evidence/I-0066/artifacts/i140-t20-*`, the builds `build-t20a`, `build-t20b` with their logs.
- Tranche 19: `evidence/I-0140/t19/` (`audit1.log`, the refused first launch; `audit2.log`, `i140-t19`; `audit3.log`, `i140-t19b`;
  `crops/`), the runs `evidence/I-0066/artifacts/i140-t19-*` and `i140-t19b-*`, the builds `build-t19a`, `build-t19b` (failed),
  `build-t19c` with their logs; the params read with the DS3 paramdefs of DSMapStudio 1.11.1's Paramdex (dumps by
  `Get-ParamRows.ps1`, not kept: BehaviorParam_PC, Bullet, SpEffectVfxParam, AtkParam_Npc).
- Tranche 18: `evidence/I-0140/t18/` (`audit1.log`, `audit2.log`, `f9-kinds.txt`, `crops/`, three of them the reviews' strips);
  runs `evidence/I-0066/artifacts/i140-t18-lead-*` and `i140-t18b-lead-*`; build `build-t17h`.
- Tranche 17: `evidence/I-0140/t17/` (the series' logs `audit1.log` to `audit3.log`, the luma tables `*-luma.tsv` and their
  words, the strips `crops/`); runs `evidence/I-0066/artifacts/i140-t17-*`, `i140-t17b-*`, `i140-t17c-*`; builds `build-t17a`
  to `build-t17g` with their logs (the object cache `cache-t17`); `build-e90.log` is empty (« A loss »).
- Tranche 13: `evidence/I-0140/e76/` (`trial.log`, `trial-sheet.png`, `audit.log`), `evidence/I-0140/e80/` (`audit2.log`,
  `magic-param.tsv`), the sheets `i140-e76-sheets/` (with the triages' TSVs), the strips `i140-e76-strips/` and `i140-e80-strips/`;
  the run folders `evidence/I-0066/artifacts/i140-e76t-*`, `i140-e76-*`, `i140-e80-*` (journals, `casts.mp4`, shots); the builds
  `build-e75` (the first pass) and `build-e79` (the second); `build-e72`, `e73`, `e74`, `e77`, `e78`, `e81` not used (their
  `NOT-USED.txt`).
- The merged gate's witness: `evidence/I-0066/artifacts/i140-e70-warp-main-native` (journal, the two lossless films kept), the
  series script and log `evidence/I-0140/e70/series-e70-g23.*`, `readings.txt`, `hair-e64rows-e70main.png`, `arrive-main.png`.

## Uncertainty

- Tranche 20: one run a family and renderer; the Fire Keeper dies within three or four hits, so few hits a run; « a swell » in
  the Rope Firebomb's counts a crude criterion (a later maximum above an earlier minimum) on whole-window counts; the heading's
  direction OBSERVED near 0 and π (i114-cam-f60) and at 1.92 and 1.97 rad by the approach distances; the swell's rule is a crude
  one on whole-window counts; the keeper character's identity INFERRED from its place (no entity id).
- Tranche 19: six Crucifix stances, four Rope Firebomb uses, two Farron and two Splitleaf casts, one maggots run a renderer;
  the characters face different ways from the second cast on (each run's drift), so framings differ; the fire counts over the
  whole window mix the fire's size with where the bomb fell; the maggots' removal read from the gauge, not from the worms;
  the fire and gauge counts first taken at 20 frames/s, the fire's recounted at 60 by the final review (kept); the NPC that took
  the statuses named by its place and HP only (no entity id); `grip`'s key the keyboard default as the pilot
  knows it (the bow drawn after it: OBSERVED).
- Tranche 18: one cast a lead and renderer each pass; the original's own variation (Havel's dust, the Crucifix's swirl, the
  rope's peak) shows that one run a renderer cannot decide an effect that varies; the F9 rows taken at an unrecorded moment
  0.5 to 1 s after their asked time.

- Tranche 17: one cast an art or goods and renderer; « alike » by the pilot's eye on crop strips (160x120 to 320x240 a frame) for the casts read, the
  others by the luma screen alone (a lead, not a judgement); which hand's art the game takes beside a weapon read only from the FP
  costs (the shields' casts); the stance's attack the strong one (Shift held, INFERRED); the two differences' causes not read;
  the F1 tool's quick-slot path runs, like its hand path, from the character manager's schedule hook, not the menu's step (no
  fault over the 18 runs); the bows' two-handing and a target for the ammunition not tried.

- Tranche 13: one cast per spell and renderer; « alike » judged by the pilot's eye at 256x144 a frame on film strips for 23 casts,
  by a Sonnet triage of the 480x270 sheets alone for the other 56; fine detail, a short flicker between two frames 300 ms apart, and sound not judged; Thomas's eye not yet
  on the sheets (D-0181); the 2 not compared left (the 25 not cast: tranches 14 to 16); the last cast of each run not judged by FP; the FP offset +0xE4 INFERRED (the fill witnessed:
  filled to 181 at each press, from as low as 81).
- Tranches 14 to 16: one cast a spell, resin, status or art and renderer; « alike » by the pilot's eye on strips of 256x144 or
  220x220 frames (the arts: the per-cast luma table first, then the strips it points to); the save's offsets OBSERVED on three
  copies, not read in the game's code; the statuses' rows and bleed's cause INFERRED (STUDY-EQUIP-SPEFFECT « B2 », the subagent's
  read, not re-read by the pilot); the stats-99 copy's USER_DATA010 summary still says level 144 (the game loaded it in all
  eight batch runs); the F1 tool runs the menu's equip sequence from the character manager's schedule hook, not the menu's step: which
  threads touch ChrAsm then is not read (no fault over 248 equips and 97 adds in the `i140-e89` runs); a hand word whose slot or inventory cannot be read, or an
  s word without a repository or module, is spent and shifts the later words (the series' press-by-press check names it).
- Tranches 8 and 9: a key the native rule does not resolve ({area, -1} not overridden, {0, x}) gets the bank's built-in record;
  the original's nodes for {area, -1} equal those values at areas 132, 140 and 141 (`i140-e26`, `i140-e27`) but for word 7
  (+0x2C: 0 in the original's nodes, the integer 1 in the native records; whether 14E900 reads it not read), other maps unread,
  and for {0, x} the original would answer {1, 0} (never met by a witness). The pass-over of an engine node never ran
  (`native_effect_fog_engine_node` 0 in every run). The cause of 32000000's lost veil is INFERRED (the world's, or the flash's
  afterglow out of phase: R's reading, 2026-11-15). The proxy family's classes other than the movie's and the menu's not read.
- The census reads counters only: an effect drawn wrong (not skipped) does not show in it.
- Tranche 5: the withdraw path of the 20 slots is reachable natively by the call graph, never observed; the slot scans see only the
  functions holding a `0x4788` displacement; the path that draws 31000010's flying wyverns is not named; the lighting causes of the
  cutscene defects are INFERRED (how the original's deferred pass picks a light set per pixel is not read).
- Tranche 10: neither scene's picture isolates the shaft (the « before » tiles are the tour's runs, another build, the OLED, not the
  same instant; no A/B with `no-ffx-lightshafts.flag` in one build), only the journals show it drawn; the witnessed build is
  `7504c672d`, the merged code's later changes (the outer cache seeded only during a shaft command, a row) not run before the merge; the shafts' brightness against the original is left
  to Thomas's eye; the 0x31 layer's scissor, the stage's s1 sampler and a failed target creation are not witnessed.
- Tranche 12: one run per variant (no run-to-run spread for the crawl numbers, 0.709 against 0.735); the dots judged by eye on
  lossless frames against the native `no-taa` film (tranches 35 and 41's proxy for the original, no original in the e64 sheet);
  the merged gate witnessed once (`i140-e70`, one run: the mask on 79 and 65 frames of the two travels, every one with 42 rows; that
  each travel's fade is marked on every one of its frames, not on alternate ones, is INFERRED from the steady 42 rows); no fading row writing SV_Target1 (the glow) is known, whose second output the one-target mask pipeline discards; Hidden
  Body's descent and rise and the camera's approach mark the mask while they move (a few frames each, not witnessed). A model's first
  frame of a moving fade is not marked; a fade stepped every other frame would mark the mask on alternate frames (not seen).
