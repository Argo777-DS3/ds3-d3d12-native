# I-0100 result: no pop-in (Pas de pop-in)

Active. Each step is recorded here when it closes; the plan is the [CONTRACT](CONTRACT.md) and
[POPIN-STUDY](../I-0097-stutter/POPIN-STUDY.md) §5-§6.

## Step 1: the pipeline library stamped with what made it (C5)

1. **The defect** (I-0097 RESULT tranche 5 item 6, POPIN-STUDY 2.3 and §3 C5): the native title and loading page drawn on a
   light grey with hard coloured halos on every native OLED run of the morning of 2026-10-25, black again without the build folder's
   pipeline library (`i97-t5g-nolib-1`) and with a library rebuilt from empty (`i97-t5w-main`); the bad library and its list kept
   at `evidence/I-0097/pipeline-library-archive-20261025-0945`. The library had no stamp of what made it: only the checksum that
   ties it to its list (I-0072), and it had accumulated entries over many module builds in the same folder.
2. **The correction** (`966128cf`): the list's header (format 2) records the environment both files were made in: the module
   image holding the cache (its PE time stamp, a hash of the image under `/Brepro`, its size, checksum and PDB signature), the
   adapter's PCI identity (vendor, device, subsystem, revision), the user-mode driver's version and the loaded `d3d12.dll` and
   `D3D12Core.dll` images. At the start another module, adapter, driver or runtime discards the library and keeps the list,
   whose records name themselves (a description, its programmes and its root by their bytes), so the precompile builds the
   library again while the game boots, as games rebuild their pipeline caches after an update (Thomas, 2026-10-26 01:27: « refaire
   une pré-compilation à chaque changement de build, comme les jeux modernes »); a list of the former format keeps its records too (final review L2), its library discarded. Elden Ring
   checks its library against the adapter only (ER-PIPELINE-STUDY, `backend.h`); the native adds the module, the driver and the
   runtime. The row `native_pipeline_library` gives the discard reasons (`discarded`: 1 format, 2 module, 4 adapter, 8 driver,
   16 runtime) and the environment. Every new module build is a new module identity, so each build starts a fresh library in
   its folder and in the launcher's (the validated « remise à zéro à chaque build »), without a deletion in the build script.
3. **Controls**: `pipeline_cache_test` T6: the same environment keeps both files; another module, driver, runtime or adapter
   discards the library and keeps the list, the reasons adding up; after a new build the list's record is precompiled into a
   new library, which the next start of that build loads; the hardware driver's version is read. The first form passed on warp and hardware (`evidence/I-0100/build-c5.log`), the final one too (`evidence/I-0100/build-t1f.log`).
4. **Step review** (a fresh Opus at high, D-0198): no high finding. M1, the environment's DXGI factory taken through a `dxgi.dll`
   import, which would bind to a proxy in the game's folder: now the system DXGI as the device's (`SystemFactory`), and the build
   refuses a `dxgi.dll` import. L1 the driver's version required on hardware; L3 (keep the list on a change) taken, for every
   change after the witness below; L4 the witness runner stops its capture. Not done: L2 (a list of the former format crafted
   in the control; the path is one comparison, read in review); L5 (zeros in the row with `no-pipeline-library.flag`, as the
   row says nothing was checked).
5. **Live witness** (2026-10-26 01:26-01:35, the ASUS, the Settlement's turn-only path, pictures every second, runner
   `Run-I100TitleWitness.ps1`; side by side `evidence/I-0100/c5-titre-ancienne-bibliotheque-vs-correction.png`), OBSERVED:
   - `i100-c5-oldlib-main` (`main`'s module, the bad library and list): the title grey, the logo reduced to dotted outlines
     with green and violet pixels (`cap-018` to `cap-041`; Thomas saw it live, 01:26), the library loaded (277 records, 209
     pipelines loaded from it, the 68 other stored names failing to load: a hint about the bad library, final review L7). So the defect is not the OLED's: the files alone make it, on either screen.
   - `i100-c5-oldlib-fix` (this step's first form, the same files): discarded (`discarded` 1, the former format), the title
     black; but the list went with the library and 196 pipelines were compiled at their first use on the frame path
     (`pipeline_cache` frame creations), the cost Thomas pointed at.
   - `i100-c5-oldlist-main` (`main`'s module, the bad list alone, no library): the 277 records precompiled afresh in 108 ms
     while the game boots, none compiled on the frame path, the title black. So the faulty part was the library's driver
     objects, not the descriptions (INFERRED: which entries, and why the driver's stored objects were wrong under names that
     hash the whole description, is not established); keeping the list across a change is safe, hence the rule above.
6. **Not covered** (INFERRED): a library made in the same environment that is wrong for another reason (a driver profile
   changed between two starts of one module, a driver fault in a stored blob) is not caught by the stamp; the launcher's
   `-ReloadShaders` remains the user's remedy.

## Step 2: T0, what each renderer draws (`popin-observe.flag`)

1. **The instrument** (`98099555`, revised after its step review): rows only behind `popin-observe.flag`, in both renderers.
   Native, per main-view prepare: each model drawn (a row other than the shadow and velocity ones drawn) or why not (not ready,
   no main row, refused at admission with its code, absent: not walked), one `popin_native` row per change, with the weights
   the admission read, the fade intent's opacity and dither bit; `popin_first` for each model's first admission in the run and
   its time; `popin_rebuilt` for the admissions that built a record again. Original (Reference with `original-probe.flag`):
   the main-view draws as DS3 hands them on (D3DA20 for a map piece's item, D079C0 for a draw entity, CFDE70 for a character's
   part; pass ids 5 and 0x45), gathered between two world update gates, one `popin_reference` row per change, and one
   `popin_reference_window` row per window (its emit count, first and last emit). Positions: the translation of the matrix
   each draw uses, as the native's candidates read it; distances in metres from the LOD manager's camera. Reader
   `Get-PopinJournal.ps1`; series runner `Run-I100PopinSeries.ps1`.
2. **Step review** (a fresh Opus at high): H1 the original's map-piece rows came from D04B90, which every piece reached by the
   traversal enters, drawn or not (its weight and visibility tests are inside it): now D3DA20, called only for a drawn item;
   M1 DS3's traversal runs on job threads, so a window can cut a frame: the windows' emit counts and times are written and the
   reader leaves out the changes around a short window; M2 the characters' parts (CFDE70) were missing from the original's
   rows; M3 a main view not prepared (a load) left the native's models « drawn »: now they go off; L1 the original keyed by
   (object, model); L2 a scene repeated recognised by its uid; L3 the row collector cannot throw from a destructor; L4
   `PopinNativeFrame`'s statics registered in the thread-affinity list; L5 the reader's frame count; L7 noted in the reader;
   L9 an unrelated line-ending change undone. Found by the pilot in the first runs: the LOD controller's +0x20 is not a
   position (every centre near 0, every distance about 466 m), replaced by the draws' matrices; `popin_fresh` counted every
   record rebuilt (107,121 rows in `i100-t0-settlement-n1`), replaced by `popin_first` and `popin_rebuilt`.
3. **First series** `i100-t0-settlement` (2026-10-26 01:52-02:04, the ASUS, the Settlement path with enemies without AI;
   n1, o1, n2, o2) with the first form: the original's rows not used (H1); the native's, OBSERVED (`Get-PopinJournal.ps1`):
   - 541-542 objects, 12,132-12,602 changes a run; every stop of a drawn model was « absent » (6,036-6,271): no model went
     off for a zero fade, a refusal or a readiness delay on this path;
   - every appearance at the fade intent's opacity 1 without dither (6,331 rows in n1): the native never drew a fade-in here;
   - c1200, c1240, FG_A_0100 and o392550/551/553 build their admission record again at every admission: 21.4 s of admission
     work over the run's 8,134 frames in n1 (about 2.6 ms a frame on the pool), a cost, not a pop-in (to the ROADMAP).
4. **Second series** `i100-t0b-settlement` (2026-10-26 02:24-02:40, the ASUS, the same path; n1, o1, o2, then n3 because n2's
   launch check failed: no presenter row in time), the revised instrument (`c9585ce5`), OBSERVED (`Get-PopinJournal.ps1`, a hold
   of 30 frames, the load's first 300 frames set apart):
   - the instrument holds: positions agree between the renderers to the centimetre for the same objects (o001090 at the load:
     the same 14 instances and distances in n1 and o1); the original's windows are clean (its emits 1.6-1.9 ms after the gate,
     229 a window at the bonfire), no window cut a traversal;
   - the original does not hand a static distant piece to its draw every frame (m31_00_00_00_000103 at 76 m: emitted on 2
     steps of about 15, the camera still): its draws are kept between re-emits, so its own « flicker » rows are its cadence,
     not a visible flicker (9,117 reappearances in o1 even with the hold): the flicker criterion needs the original's kept
     draws, not its emits (open);
   - appearance distance: n1 against o1, 141 objects appear nearer natively (by up to 9.0 m) of the 963 both draw; o1
     against o2, 138 (by up to about 8 m): the native's appearances are within the original's own run-to-run spread (the
     replayed character's drift), no later appearance of an object both draw (but see item 7);
   - the native never drew anything below opacity 1 (every `popin_native` appearance at the fade intent's opacity 1, no
     dither), and no drawn model stopped for a fade, a refusal or a readiness delay (every stop « absent »).

## Step 3: preparing a zone's models at its load, evaluated (no code)

From `popin_first` in `i100-t0b-settlement-n1` and `-n3` (OBSERVED): 971 and about the same models first admitted in a run; after
the load's first 300 frames 405-406 of them, 367-368 ms of admission work on the pool, 298 map pieces (300-311 ms), 96-97
objects (29-34 ms), 11 characters (23-38 ms). They gather in the camera turn (collect serials 8479-8596): 11-13 collects carry
10 ms or more of first admissions (up to 44.8 ms in one collect, s8502 of n3), the turn's heavy frames of I-0097 tranche 5 item
10. Every one was admitted at first sight (models DS3 had already loaded with the zone). So preparing every model of the zone
during its loading screen (D-0211) would remove about 0.37 s of admission work from play, all of the turn's first-admission
peaks, at a cost inside D-0213's + 1 s a load (the 565 models admitted at the load itself took about 0.74 s with the rest of
the load; on the admission pool the extra time is a fraction of that, to measure). INFERRED: worth doing, as T3/T5's universal
preparation; the measure of its load time is its first control.
Also seen: c1200, c1240, FG_A_0100 and o392550/551/553 build their admission record again at every admission (19.4 s of pool
work over n1, `popin_rebuilt`): a cost to look at with the preparation (a record that cannot be kept).

## Step 2 (continued): the draws' opacity

5. **Third pair** `i100-t0c-settlement` (2026-10-26 02:54-03:00, the ASUS, n1 and o1, `aaf2ecf4`: the original's draw opacity read
   in D3DA20 at the group copy's +0x124 that D04B90 scales by a piece's weight, `popin_reference_fade`; the native's fade
   intents, `popin_native_fade`), OBSERVED:
   - the original drew no map piece below opacity 1 on this path (0 `popin_reference_fade` rows); the native neither: its only
     fades were c1240's, a 12-frame dithered ramp (opacity 0.083 to 0.917, dither 1, 66 of the 68 rows; 2 at 0.000). Not validated for fades: no positive control (an object known to fade in the original), the base of the group's +0x124 not logged, and the draw entities' and characters' opacity not read in the original (PopinDrawEmit, PopinPartDraw pass 1): « 0 rows » says no fade was seen by this instrument, not that none happened;
   - every one of the 404-406 models first admitted after the load (n1 of t0b and of t0c) was drawn in its first admission's
     frame: no readiness delay natively;
   - the side-by-side sheet every 4 s (`evidence/I-0100/sheet-t0c-*.jpg`, `New-I97PathSheet.ps1 -Captures`) shows the same
     scene in both renderers at each point of the walk; one difference, a distant hollow on the original's pictures at +24 s and
     +28 s and not on the native's, is not settled (the hollows move between runs; the reader cannot match moving characters).
6. **Objects the original draws and the native never does** (final review H1, OBSERVED): o000499 (3 instances, 125-289 m),
   o319250 (3, 168-177 m) and m31_00_00_00_009400 (2, 303-369 m), map pieces' items drawn by the original in pass 0x45 with
   weights 1/1 from step 7185 (the camera turn) in i100-t0b-settlement-o1, -o2 and i100-t0c-settlement-o1 (20-30 rows a
   run), and never in any row of i100-t0b-settlement-n1, -n3 or i100-t0c-settlement-n1 (neither popin_native nor
   popin_first: the native's main walk never yields them). Outside the original's own spread (o1 against o2: 35 never-drawn
   keys, all moving characters). o000499 is the class Fable's review met at m33 (the Crucifixion Woods cross, bounds never
   written, I-0066 « Optimisation review » note in `09_model_admission.inc`). This fails POPIN-STUDY §6 criterion 2 and is
   T2a's first lead: why the walk never reaches them (layer mask, culler, category).
7. **Conclusion of T0 on the Settlement path** (INFERRED from the observations above): for the objects both renderers draw, the
   native's appearances are within the run-to-run spread, no readiness delay was seen, and no fade was seen in either renderer
   by this instrument (not validated for fades); but some distant objects are drawn by the original and never by the native
   (item 6). Open: those objects (T2a), the load (the world drawn incomplete under the loading page, C2: POPIN-STUDY M2, not
   built in this tranche), distant characters (2.2, Archdragon Peak; the instrument cannot match characters across the
   renderers: the player's parts have other names natively, moving characters change keys), the original's kept draws for a
   flicker criterion, a positive control of the fade measure, and Thomas's eye on the next native runs. The preparation at the
   load (step 3) is justified by the turn's first admissions whatever the pop-in's cause.
8. **Instrument limits** (final review L5): 93 % of the original's item draws carry the « variant above 0 » bit, which D3DA20
   skips when 812DF0 answers (not decompiled), counted as drawn; the reader does not list objects drawn natively and never by
   the original (29 for n1 against o1, almost all characters); popin_first's set is never cleared, so a reload undercounts.
   Views other than the main one and M2 (readiness after a load) are deferred to the tranche that needs them. T0's flag-off
   control is the gate's runs on this tree. `evidence/I-0100/t0b-n1-o1.json` is the first reader's output, superseded.
9. **Final review** (a fresh Opus at xhigh, D-0198): H1 (item 6, the conclusion corrected here, in ROADMAP and STATE), M1 (the
   fade claim narrowed, item 5), M2 (the deferrals, item 8); L1 stale comments fixed; L2 a list of format 1 keeps its records
   (no first start compiling about 200 pipelines on the frame path in every existing folder after the merge); L4 dead counters
   removed, the item hook tests its pass first, the native's on-row no longer reads the owner; L3, L5 recorded (item 8); L6
   figures corrected; L7 (pre-existing: StorePipeline « already exists » for a record aged out while its name stayed in the
   library) bounded by C5's reset at every build. The fixes' step review (a fresh Opus at high): nothing at high or medium; its two lows: the discard bit's comment clarified (format 1 keeps its records), no unit control yet for a format-1 or unknown-version list (read in review only).

## Tranche 1's gate and merge

The replayed path gate on the ASUS with the memory leg (D-0218), `i100-t1b` (2026-10-26 04:19-04:43): **PASS**, frames over
2.04 P 4.5 (threshold 8), over 1.5 P 19 (27), display_late 14.6 (22), game steps over 2.04 P 2 (6), pictures 0.87-0.88 / 0.0001,
memory -2 MB a visit (40); the memory leg's quit raised DS3's exit heap fault 0xC0000374 after its close (I-0099, not a fault of
the gate). A first gate `i100-t1` (03:42-04:11) made every run but stopped at its evaluation on a fault of the script (the memory
leg's value named like the `-Memory` switch, fixed in `4ec8d1af`); its runs judged as an experiment also passed; a gate re-judging
its own runs files no record, so it was run again whole.

## Tranche 2, step 1: Thomas's play, the reference list

Thomas played natively on 2026-10-26 from 05:01 to 05:25 (the OLED, borderless 2560x1440 at 280 Hz, `tools\Launch-DS3-DX12-Popin.cmd`,
`popin-observe` and `crossing-observe`), with `Watch-PopinPlay.ps1` beside him: his journal (`play-20261026-050151.jsonl`,
77 MB), PresentMon from 05:05 (OLED numbers, never mixed with the ASUS base), the minute-to-serial clock, his notes and his
NVIDIA clips (05:07:32, 05:12:00, 05:16:52, 05:20:42), all in `evidence/I-0100/play/20261026-045253/` (the clips stay in his
Videos folder). PresentMon wrote nothing for its first 3 minutes: started after the game and unelevated it could not resolve the
process name, and an orphan trace session of an earlier gate (DS3Replay) was losing the events; it was restarted on the
process id at 05:05 once the orphan session was stopped (`Watch-PopinPlay.ps1` now does both).

The reference list (Thomas's words, OBSERVED by him; the journal's serials from the clock):

| # | Time, zone | What Thomas saw | Family |
|---|---|---|---|
| R1 | 05:09 Undead Settlement (Beta), serials 25200-28681 | the shadows pop in, not the models; distant shadows (05:11) | sun shadow casters |
| R2 | 05:12 Archdragon Peak, 35875-39354 | the man-serpent appears and disappears | distant characters |
| R3 | 05:13 Archdragon Peak, 39415-42996 | a problem with the lever's shadow | shadows |
| R4 | 05:14 Archdragon Peak, 43059-46561 | distant animations at a very, very low rate | distant characters' animation |
| R5 | 05:17 Crucifixion Woods, 53715-55129 | distant shadows: the tree far away, the cross on the character's right | sun shadow casters |
| R6 | 05:17-05:18 Crucifixion Woods | chased through the forest: the vegetation's shadows pop in all along | sun shadow casters |
| R7 | 05:23 Anor Londo | the bonfire's shadow pops in | local light shadows |
| R8 | 05:24 Anor Londo, 78845-82403 | a distant animation very slow, the silver knight (no clip) | distant characters' animation |
| R9 | T0 (step 2 item 6) | o000499, o319250, m31_00_00_00_009400 drawn by the original, never by the native | main view, distant objects |

Not pop-in, to the ROADMAP: Anor Londo's walls grey and smooth (their texture missing, 05:23). Seen fine: the Load Game portraits
(05:06, all present). Thomas found Archdragon Peak and the Crucifixion Woods otherwise « rien de bien choquant ».

From his journal (OBSERVED, `popin_native`): every stop of a drawn model is « absent » (87,636) or zero_fade (105): nothing went
off for a readiness delay. Short gaps (off, then on within 5 frames at 280 Hz) for map pieces at 7-800 m (m31_00_00_00_002050
112 times, m32_00_00_00_004252 99, m37 pieces 20-80 each) and, within 30 frames, for characters: c1200 21, c1390 14 (the
man-serpent's zone: one c1390 absent one frame at 45 m, serials 38438-38439, standing still), c1240 13, c5226 12, c1441 12. The
journal cannot tell which gate removed them (« absent »: not walked), nor anything of the shadows (no caster rows): the tranche's
instruments below add both.

Families, in the order of the next steps: the shadows (R1, R3, R5, R6, R7: five of eight notes), the distant characters' gaps
(R2), their animation rate (R4, R8), the objects never drawn (R9).

## Tranche 2, step 2: the instruments and the first causes

1. **Instruments** (`043254b4` to `f6a580b1`, all behind `popin-observe.flag`; builds `evidence/I-0100/build-t2a`, `-t2b`, `-t2c`):
   `popin_native_gate`, the gate that ends every main-walk entity (map piece, draw entity, assembled character: passed,
   frustum, enable, visibility, category, absent), with a draw entity's bounds E+0xB80 and whether its draw centre is in the
   culler's view; `popin_native_shadow` and `popin_reference_shadow`, the sun casters by entity (native, with their cascades)
   and the original's depth rows of 63CB0 by model; `popin_reference_pass`, the pass contexts the original's main-view draws
   come through (8 ring copies of id 0x45, engine pass 60, and 8 of id 5, engine pass 2: one view, OBSERVED);
   `popin_native_refused`, a model refused at admission that was never drawn; `popin_native_lightmap`, the source of a model's
   lightmaps at each fresh admission; the named lightmap lookup failures again after each scene reset (12 a process without the flag, as before). The controller and
   the launcher play the original renderer without a time bound (`-Original`, `tools/Launch-DS3-Original-Popin.cmd`):
   Thomas's object-by-object comparison (the coordinator's item 3). The reader gives the shadow casters side by side by model.
2. **The Settlement pair** `i100-t2a-settlement-n1`/`-o1` (2026-10-26 05:47-05:53, ASUS, enemies without AI; reader output
   `evidence/I-0100/t2a-n1-o1.json`), OBSERVED: the casters differ. The native casts more and longer (m31_00_00_00_002091 73 %
   of the frames against 10 %, o310520 84 % against 5 %, c1240 96 % against 29 %), casts two models the original never casts
   (c1100, c2021) and misses one it casts (m31_00_00_00_003080); the original's player parts cast 55 % of the frames (11
   starts, in step with the walk in and out of the shelter), the native's 98 %. Most native casters are in cascade 3 only.
   Not yet explained; the original's caster rule is the open T2a question for the sun shadow.
3. **The three distant objects** (R9), OBSERVED in `-n1`: their map pieces pass every gate (o000499, o319250 and
   m31_00_00_00_009400: frustum or passed, never refused by flag, visibility or category); so their items are dropped later
   (the child loop or the admission): `popin_native_refused` (build t2c) tells which.
4. **The distant « shadows » after a load (R1, R5, R6, R7)**, OBSERVED in Thomas's native sessions at Anor Londo (06:15-06:33):
   the defect appears after every death's reload and never after a launch into the zone (Thomas, 06:24, and at the Settlement,
   06:50); in the journal `play-20261026-062239` the DOL lightmap lookups (t26/t27) that bind nothing are 322 of 36,000 (0.9 %)
   before the first death and 5,978 of the next 40,000 after it (about 70 %), each falling back to SYSTEX_WHITE. Near the
   bonfire no map piece casts a sun shadow natively (no `popin_native_shadow` row within 60 m) and the original's casters
   there are other models (m37_00_00_00_001053, 000998, 010070…): the balustrade's shadow on the floor is the floor's lightmap
   (INFERRED, superseded by item 13: the stairs' shadows are sun casters). Thomas's pictures (native 06:25, original 06:29, `evidence/I-0100/play/20261026-062707-original/`): the original
   has the balustrade's and the statues' shadows up to the bonfire, the native after its deaths a lit floor there. The
   cause, INFERRED from the code: a binding's kept choice of holders (`NativeTextureHolders`) stands as long as the registry
   root does when its first candidate has a holder, so after a reload that releases the map's lightmap packages and makes new
   caps, the « _atlas000 » candidate keeps a released cap's holder and resolves to nothing; a nearer LOD with another
   material is looked up afresh, hence « right once near ». A white lightmap is also a stand-in (D-0197). Candidate fix
   `0e89a4dd`: a choice stands only within its scene (the scene resets). To validate: the t2c diagnostic run with a death,
   then the fix's run. The black foliage of the Crucifixion Woods (clip 05:16:52 at 0:07) is probably the same family
   (lighting inputs after a load): to check with the same run.
5. **The man-serpent (R2)**, OBSERVED in Thomas's clip 06:32:11 at 0:24 and his journal `play-20261026-063104`: a c1390 drawn
   at about 10 m is ended by the frustum gate at serial 4892 and drawn again at 5043 (0.5 s); at the cull its draw matrix is
   3.7 m from the camera, at its last drawing 9.9 m: the character moved while its bounds E+0xB80 (what the gate tests) did
   not follow, INFERRED; build t2c writes the box and whether the centre is in the view. Of 55 frustum culls of a drawn
   character in that session, 5 had moved more than 3 m.
6. **The distant animations at a very low rate (R4, R8; Thomas 07:01: « plus l'ennemi est éloigné, plus ses animations sont
   lentes, 2 fps limite »; the original normal, 06:57)**: a lead read in the decompile (INFERRED, to verify): the original's
   main-view draw sets a « drawn this frame » bit in the entity, D079C0 for a draw entity (pass 0x45: E+0xC50 |= 1 <<
   E+0xC51; corrected by the final review: Ghidra indexes a longlong pointer there, the asm 140D07DC3 ORs [RBX+0xC50]) and
   D02A30 for an assembled character (E+0xCB0 |= 1 << E+0xCB1); the native, which does not run those draws, never sets them,
   so DS3 may treat every character as unseen and update the distant ones at its unseen rate. The reader first named,
   891A30, tests [chr+0x50]+0x18A, another byte: who reads +0xC50 is open. Next: the readers and the clear of these bits (headless Ghidra), then the bits set by the native's
   main view for the entities it draws.
7. **The candidate fix of item 4 failed visually** (Thomas, 07:17, build t2d): after his deaths the DOL lookups that bind
   nothing fell back to about 1 % (journal `play-20261026-071308`: 737 of 72,000), so the texture choice per scene stays (it
   removes released holders: the t2c witness had shown, after a death, a lookup failing while a fresh D24490 found the « _atlas000 »
   holder with a resident texture), but the floor stayed wrong: those failures were not the visible cause. Item 4's INFERRED
   cause is withdrawn. What Thomas then established (07:32-07:36, build t2e, `evidence/I-0100/play/20261026-073223/`): three
   distances, after a launch into the zone the farthest objects wrong and the middle and near ones right; after a death the
   middle ones wrong too, the near ones right; the lever of Anor Londo (an object) dark from far, whitish near and in the
   original; the Crucifixion Woods' foliage black from far. So the defect follows the LOD drawn (INFERRED), the death moving
   its threshold one level. In that journal the lightmap pages of one model differ by entity, not by LOD (m37_00_00_00_000752:
   atlas000 to atlas003 on its several entities, one page and constant per entity in the rows).
8. **The sun casters by cascade** (corrected the same morning): `native_shadow_walk` counts the shadow-only entities by cascade (0
   or 1 in the three near cascades, 4 to 12 in the far one), but the draws per cascade are not empty (`native_shadow_draws` in
   `i100-t2a-settlement-n1`: 44, 65, 84 and 104): the near cascades are drawn. The casters are tested against the same
   cascade matrices they are drawn with (`NativeCascadeBoxes`, the cbMatrixWVP of 13_model_frame), so the test and the drawing
   agree. This lead is withdrawn; the casters differ from the original by model (item 2), which stays open.
9. **The drawn bits (item 6), an experiment**: `drawn-bits.flag` (off by default) sets them for the entities the main walk admits;
   the Settlement A/B (`i100-t2f-bits-n1` against `-nobits-n1`, 07:55-08:01, ASUS, no AI) saw the same pose updates (242,167
   against 240,358 by frame 9,600) on a path whose enemies stand near: not discriminating, and for draw entities it wrote the
   wrong byte (+0x18A, item 6), corrected to +0xC50 after the final review; only its assembled characters' half was tested.
10. **The sun cascades are not the far defect** (OBSERVED, `native_shadow_state` in Thomas's journal `play-20261026-073223`): at
    Anor Londo the cascades split at 2, 8 and 15 m and fade from 15.1 m over 50 m, so no sun shadow reaches past 65 m, and the
    record is the same before and after his deaths.
11. **The far-LOD lighting: the instance ambient rows gated by the LOD** (cause OBSERVED, fix `37948cad`). The original's 5B3D0
    writes the instance ambient rows (cbInstanceData +0x80..+0xAF, aSh) for row types 0, 3 and 4 (`*param_6`, the recorder row's
    +0x30 byte: 59220 passes it to 5CEC0, which passes it to 5B3D0); its depth (2) and velocity (5) rows take none. The native
    tested the fade intent's LOD variant instead (an earlier reading, OPTIMISATION-REVIEW 1.4, took the two for one), so every
    model drawn at LOD 1 or 2 had zero ambient: dark from far, the darker the less it is lit otherwise (Thomas's lever, the
    Crucifixion Woods' foliage). The data: at the m33 lake (`i66-reference-2222`) the original's map pieces are rows of type 3
    and its objects 0 or 3, the native's variants 0 to 2; on the Settlement path (2026-10-26 10:20-10:57, ASUS, no AI;
    `i100-t2h-lod-n1`, `i100-t2h-sh-n1`, `i100-t2i-sh-o1`, `i100-t2i-sh-n1`) the original lights 41 object and character models
    (rows not zero), the native with the old gate 14 of them, with the fix 40; recounted by the final review in the colour rows only
    (5B3D0 never writes the type-2 rows, so o000250's « lit » rows are uninitialised words): the original lights 40 models and
    all 40 are lit natively, 39 with an identical 12-word vector and c1240 equal to about 6 significant figures (o001060:
    [0.00763442, -0.0262433, ...] in both); c1200 and o310520 are absent from the original's log, not shown unlit. Both
    witnesses stop at 800 rows: the comparison covers about the path's first 3,700 frames, not the whole path;
    the map pieces' rows are zero in both modes (lightmapped, their anim block owns zero rows). The native's main-view record is
    shared by its passes, so it takes the rows at every LOD; `sh-by-lod.flag` keeps the old gate (A/B, to remove in tranche 3).
    That this is what Thomas saw (the Anor Londo lever, the Crucifixion Woods' foliage) is INFERRED, his check by eye; the
    foliage may be map pieces, whose rows are zero in both renderers and untouched by the fix. The original's 5B3D0 hook now also runs under
    original-probe with light-observe.flag (`reference_instance_rows`).
12. **The three distant objects (R9) are not a defect** (OBSERVED): refused natively at admission (« missing_inputs »), their
    model (item+0x58) without storage (+0xE8) and meshes, from the first refusal to the 100th (`i100-t2j-objects-n1`, frames 8,483
    to 8,670); the original calls their item draw (D3DA20) on the same empty model (`popin_reference_model` in
    `i100-t2k-objects-o1`, 2026-10-26 11:32-11:35: o000499, o319250 and m31_00_00_00_009400 storage false, 0 meshes, the only
    three of 299 models drawn): nothing to draw in either renderer. The T0 list (step 2 item 3) counted a call as a draw.
13. **Central Irithyll and the Anor Londo bonfire, after a launch and after a travel** (OBSERVED, 2026-10-26 11:44-12:57, ASUS, Gamma without AI: the
    test save `shebang-zones-gamma-m37-anorlondo-20261026-123818`, made by travelling from its Central Irithyll save; runs
    `i100-t2l-m37-*`, `i100-t2n-al-*`, `i100-t2o-al-*`, pictures in `evidence/I-0100/t2l-compare`, `t2n-compare`, `t2o-compare`):
    the great stairs at 30-60 m carry the balustrade posts' shadows in the original, not natively, after a launch on the spot
    and after a travel alike, with and without the ambient fix; the sun's cascades are the same in both modes (0.1-2, 2-8, 8-15,
    15-65 m, the m37 record; the original's fits in `i66-ref-1538-m37-bind`). So that difference is the sun casters (item 2; superseded: a local light's, tranche 3 step 1),
    open, not the reload. The ambient fix shows nothing there by eye (no object lit by its ambient at LOD 1-2 in the view).
    Also seen: Central Irithyll's chapel warm brown in the original, cold white natively (both native builds; ROADMAP). The
    original's caster rows need its original-probe hooks, whose window the menu pilot cannot drive (`i100-t2o-al-o2`: the game
    ran out of sight): a replayed path is needed there.
    The PIX GPU capture (`pix-capture.flag`, development only): the module loads PIX 2603.25's WinPixGpuCapturer.dll before the
    device and calls its export `CaptureNextFrame(file, frames)` at presented frames 3060 + 600 n (`i100-t2g-pix4-n1`: four
    captures, hr 0, about 1 GB each in `evidence/I-0100/build-t2g/`; a flags-first signature faulted first, `pix3-n1`). The
    captures are not read yet: pixtool's event list and resources need Windows developer mode (Thomas's setting).
14. **Closing tranche 2**: fixed, the far objects' and characters' ambient at LOD 1-2 (item 11); closed, R9 (item 12). Open
    for tranche 3, from Thomas's list: the sun casters by model (R1, R5, R6, R7: the Settlement pair, the Anor Londo stairs),
    the slow distant animations (R4, R8), the man-serpent and the silver knights culled on screen (R2: the bounds not
    following the character, INFERRED), the LOD threshold nearer after a death (the D1CCE0 bias 0x144791E04, its gate
    0x14463F8A0 and offset 0x144791E00 have no instruction writing them by address, INFERRED constants; the tables
    0x14463F5A0/6A0/7A0 by class and the manager's scale +0x150 to log around a reload), Thomas's check of the fix at his places.
15. **Final review** (a fresh Opus at xhigh, D-0198, on `main...HEAD` at `a6d34771`): H1 the drawn bits wrote E+0x18A for draw
    entities, the original's byte being E+0xC50 (fixed; items 6 and 9 corrected); M1 the gate on this tree (next); M2 the
    named lightmap lookups again after each reset ran without a flag (now with popin-observe.flag); M3 STATE and ROADMAP
    claimed more than the data (rewritten: the comparison's cap, the type-2 rows, the visual link INFERRED; item 11). Lows
    fixed: the lightmap source loop and its map gated and bounded, both depth-row witnesses fed, the variant named, two
    comments, the PIX instrument recorded (item 13, WORKFLOW « Tools »), the stale sentences (R9 in ROADMAP and the index,
    the chapel at Central Irithyll, item 4 superseded), the full hash of the new save, the replay runner's message, the
    watcher's orphan-session stop only while one game runs; the fixes' step review (fresh Opus, high) left one low (the
    cascade depth-row map fed only with cascade-observe.flag now) and a comment. Kept: `sh-by-lod.flag` as the A/B for Thomas's check, to remove in
    tranche 3.
16. **Gate** (2026-10-26 13:40-14:09, the ASUS 1920x1080@60, `-Memory`, `46bbb073`, native tree `32ff8554`): PASS. Period 16.668 ms;
    frames ending over 20.4 ms 7.5 a run (4.5 at tranche 1's gate), over 15 ms 20 (19), display late 11 (14.6), game steps over
    20.4 ms 3.5 (2), all within the base's thresholds; pictures 0.79 and 0.74 against the reference; memory -3 MB a visit. The
    memory leg `i100-t2-z1` ends with the known process-exit heap fault 0xC0000374 (at every quit since wave 1, also in the
    original: WAVE1-CLOSURE, I-0082 RESULT item 5; tranche 1's `i100-t1-z1` and `-t1b-z1` alike).

## Tranche 3: the sun casters, the distant characters (plan)

From tranche 2's item 14, in this order: (1) the sun casters by model against the original's depth rows, first at the Anor
Londo stairs (the test save `shebang-zones-gamma-m37-anorlondo`, the original's `reference_depth_models` in a plain Reference
run with `cascade-observe.flag`), then the Settlement pair; (2) the slow distant animations (who reads the drawn byte E+0xC50,
then `drawn-bits.flag` where Thomas saw them); (3) the characters culled on screen (their bounds); (4) the LOD threshold after a
death; (5) `sh-by-lod.flag` removed once Thomas has checked the ambient fix.

## Tranche 3, step 1: the stairs' shadows, a local light's (INFERRED); the spot lights

1. **The original's casters at the Anor Londo bonfire** (OBSERVED, `i100-t3a-al-o1` and `-n1`, 2026-10-26 14:31-14:37, ASUS, Gamma
   without AI, a launch on the spot, plain Reference with `cascade-observe.flag`): the models of its depth rows (63CB0, state
   type 2, `reference_depth_models`) are the player's parts, `m37_00_00_00_010060`, `010074`, `010070`, `o000250`, `o379030`,
   `o379031`, `o370505`; the native's sun walk casts the player, `000998`, `000999`, `001000`, `001053`, `o000200`, `o000250`,
   `o370505`, `o373016`, never 010060/010074/010070/o379030/o379031, which its main view draws. Their categories in the
   original's emit (`reference_disp_names`, pass 0x45): 010074 bits 5 (0x01 | 0x04), 010060 bits 21 (0x01 | 0x04 | 0x10), without
   the G-buffer category 0x02 that tranche D-0180's rule (SUN-SHADOWS-WAVE1 « The rule ») found the sun casters to carry. The
   category 0x04 is the one C2EC0 gives the passes 9..0x38, the local lights' shadow faces (SUN-SHADOWS-STATIC-PREP); the
   original draws thousands of depth draws a few seconds into the local tiles of its 4096 atlas (`atlas_other` 7,317 to 19,440
   per row, viewports 512 and 256) besides its sun quadrants.
2. **So the stairs' shadows are a local light's** (INFERRED, strongly: the categories, the local tiles, and the pictures: the
   posts' shadows run up the stairs in long stripes away from a low light behind the camera, the bonfire, where the sun's
   would be short). The native has no local-light shadow: its selection exists behind `shadow-local.flag` (SW-7, opt-in), its
   receivers and its face passes were left for later waves (SHADOWS-WAVE1-READY §3 items 4-5; FABLE-REVIEW: « local-light
   shadows opt-in and without consumer »). This is Thomas's R7 (« l'ombre du feu de camp ») and the floor and stairs by the
   Anor Londo bonfire (tranche 2 items 4 and 13); its fix is the local-light shadows themselves, a feature of DS3 the native
   does not render yet.
3. **The map's spot lights are not drawn natively** (OBSERVED in the code and the journals): `CollectNativePointLights` counts
   the spot lights (vtable 0x3D4E8E8) as « open » and skips them (07_scene_lighting: « Spot lights stay open for the drawing »;
   their per-frame volume state is not produced natively, Native1509), the forward rows bind an empty `g_SpotLightBuffer`, and
   no deferred spot programme is drawn: at the Anor Londo bonfire 45 spot lights of 533 (`i100-t3a-al-n1`), at Central
   Irithyll 46 of 622 (`i100-t2l-m37-n1`). The local shadows the original draws there are at its spot tile sizes (viewports 512
   and 256, the spot base tile 0x200 >> level). So the missing light is wider than the shadows: the spots' light itself, very
   likely Central Irithyll's warm chapel (tranche 2 item 13), and their shadows (the stairs). STATE's « deferred lighting
   (point, spot and effect lights) » is corrected: the map's spot lights are not drawn.
4. **What this means for the workstream**: the fix of R7 and of the stairs is a lighting feature, the map's spot lights and
   the local-light shadows (SHADOWS-WAVE1-READY §1.5 and §3, the deferred `DSpotLight*` and `DPointLightShadowed*` /
   `DSpotLightShadowed*` programmes, the 4096 local atlas, the face passes 9..0x38 with the category 0x04 casters), a
   workstream of its own by its size; proposed to Thomas (2026-10-26 14:50). The rest of tranche 3 (the distant animations, the
   characters culled on screen, the LOD threshold after a death) goes on meanwhile.

## Tranche 3, step 2: the distant characters' poses

1. **The measure** (`popin_pose_rates`, at the public pose update 16000 natively and through a pass-through hook in the original,
   with `popin-observe.flag` and `pose-observe.flag`: every two seconds each pose receiver's updates and its distance): at the
   Anor Londo bonfire (Gamma without AI, ASUS, 2026-10-26 15:30-16:06, `i100-t3c-pose-o1`, `-n1`, `-bits-n1`, `i100-t3d-pose-o1`,
   `-n1`, eight camera turns of 45 degrees), OBSERVED (corrected by the final review): the original updates the characters at
   54-82 m at every frame while they are in view (119-122 updates in two seconds) and about 4 times in two seconds (2 Hz)
   while they are out of view, the native's `popin_native` rows of the three c1410 there matching those windows; the native
   without the marks never updated them (the player's 16 parts only); with the marks it updates them as the original, at every
   frame in view and at 2 Hz out of view. It also updates a c2170 at 78 m and another receiver at 72-77 m at full rate that the
   original never updates: the native's admission marks a few characters the original's 0x45 emit does not draw (D02A30 also
   skips parts by their +0x10 and flags), an open divergence of the distant characters' draw decision.
2. **The marks** (OBSERVED, `popin_drawn_bytes`): DS3 keeps an 8-slot ring in each byte, the slot in byte+1 moving one step a
   frame, and clears the slot it enters (the bytes before the native's write hold the other seven slots, 127 to 254, and the
   slot moves four steps every 60 collects): a mark expires as the original's. The original writes three of these rings in pass
   0x45: the map piece item's (D3DA20: item+0x794, slot +0x795; found by the final review), the draw entity's (D079C0:
   E+0xC50, +0xC51) and the assembled character's (D02A30: E+0xCB0, +0xCB1). The native marks the two characters' rings for the
   objects its main walk admitted, by default, `no-drawn-bits.flag` opting out; the readers are not named. The item ring is
   marked only with `item-marks.flag`: natively its slot stays at 0 for most items (`i100-t3e-marks-n1`, 2026-10-26 16:52, the
   bytes before the write mostly 1 at slot 0, a few rings moving), so a mark would never expire there; whether the original
   moves it for those items, and who reads it, is open.
3. **So the slow distant animations (R4, R8) are the missing marks** (INFERRED until Thomas's eye check or a run with AI: the
   runs had no AI, so natively the unmarked characters were frozen, where Thomas saw about 2 frames a second, which is the
   original's own out-of-view rate of 2 Hz: DS3 treated every distant character as unseen). The item's ring may gate other
   decisions of DS3 for objects (unnamed readers); marked now as the original does. Two divergences kept (the step review of
   the fix): the marks follow the native's admission, so an object DS3 asks to draw that the native cannot yet (geometry,
   bindings or streams not ready, codes 2-5) stays unmarked until it can (its pose up to half a second old at its first frame);
   D3DA20's early exit when 812DF0 answers (false in the measured runs: the world draws) is not reproduced.
4. **Closing tranche 3**: done, the marks (step 2); found, the Anor Londo stairs' shadows are a local light's (INFERRED) and the
   map's spot lights are not drawn natively (OBSERVED), proposed to Thomas as a workstream of its own (step 1). Open for a
   later tranche: the sun casters of the Settlement pair (tranche 2 item 2), the characters culled on screen (R2; the marks
   may change them, Thomas's check), the LOD threshold after a death, the removal of `sh-by-lod.flag` after Thomas's check.
5. **Final review and gate**: the final review (a fresh Opus at xhigh) found no code blocker; its M1 (the rates read the wrong
   way round, the ring's clearing), M2 (the item's ring), M3-M4 (STATE, INFERRED) and lows are answered above, the item ring then
   made opt-in by the witness run, each fix step-reviewed. Gate `i100-t3` (2026-10-26 17:10-17:27, the ASUS 1920x1080@60,
   without the memory leg: Thomas, 2026-10-26 17:07, « Oui dis lui », the marks touching neither the memory, the loads nor the
   streaming, D-0218): PASS, period 16.667 ms, frames ending over 20.4 ms 2 a run (7.5 at `i100-t2`), over 15 ms 16, display
   late 8, game steps over 20.4 ms 1, pictures 0.77 and 0.76.

## Tranche 4: DS3's LOD and draw distances, the characters culled on screen (plan)

Opened 2026-10-26 17:30 in a fresh thread (D-0215), from tranche 3 item 4 and Thomas's theory (2026-10-26 17:09: « et si la
distance d'affichage était très basse et surtout fausse avec le natif ? Ça expliquerait pourquoi le jeu considère des ennemis et
des objets non dessinés alors qu'ils sont pourtant dans le champ de vision du joueur »). In this order:

1. **The LOD and draw distances, native against original, at a launch and after a death.** DS3 decides both itself: its LOD step
   D1CCE0 (MENUS-MAP 62 §3) compares `|centre - camera| x scale` (the LOD manager's +0x150) with the LodParam row's distances by
   class, its biases and hysteresis, and the native reads the LOD it chose (the fade intent's variant, `SelectModelFade`). So a
   native difference can only come from its inputs: the manager's scale and camera, a controller's centre, the biases. Seen
   already (OBSERVED, `lod_step` rows of `i100-t2i-sh-n1`/`-o1`, title rows only): the scale 0.756695 in both renderers. The
   instrument `popin_lod` / `popin_lod_scale` (popin-observe.flag, both renderers) and its reader `Get-PopinLod.ps1` give, per
   load, the scale and biases and, by model, the distances at which DS3 moved a LOD up and took an object out of its draw
   distance; then a pair at the Anor Londo bonfire (the test save `shebang-zones-gamma-m37-anorlondo`) with a death in each
   renderer.
2. **The characters culled on screen (R2)**: the native's frustum gate tests the character's bounds E+0xB80, which DS3 updates
   from its pose (D08190, MENUS-MAP 62 §2). Before tranche 3 no character was marked drawn natively, so DS3 posed the distant
   ones at its out-of-view rate (2 Hz) and a moving character's bounds could lag up to half a second, as the man-serpent's cull
   at 06:32 (0.54 s, item 5 of tranche 2). With the marks a drawn character is posed every frame: R2 may be gone (INFERRED);
   checked in Thomas's next journal (frustum culls of a drawn character that moved more than 3 m, 5 of 55 before).
3. The Settlement's sun casters (tranche 2 item 2); 4. `sh-by-lod.flag` removed after Thomas's check of the ambient fix.

Not in this workstream: the spot lights and local-light shadows (D-0221).

## Tranche 4, step 0: Thomas's eye check of tranche 3

1. **The distant enemies** (R4, R8): Thomas in play on `main`'s launcher after tranche 3's merge (2026-10-26 17:53): « les ennemis au
   loin c'est good » (OBSERVED by him). Tranche 3 step 2 item 3's INFERRED link is confirmed by his eye: the slow distant
   animations were the missing drawn marks.
2. **The far LODs without a death** (tranche 2 item 11, the ambient rows at every LOD): « LOD longue distance sans mort: good »
   (Thomas, 2026-10-26 17:54, OBSERVED by him). After a death: his next check.
   Then, OBSERVED by him: the far LODs right at Anor Londo after his deaths too (17:57, « oui après mes morts »), and at the
   Undead Settlement (17:58); the Crucifixion Woods next. So the ambient fix closes what Thomas saw of the far lighting at
   Anor Londo and the Settlement, after a launch and after a death, and the Crucifixion Woods (18:00, « LOD ok au bois »): `sh-by-lod.flag` removed (the ambient rows at every LOD, unconditionally).
3. **The characters vanishing near him** (R2 is not settled by the marks): a silver knight « apparaître et disparaître quand il
   était proche de moi » (Anor Londo, 17:56) and the Settlement's villagers (17:58), « même famille de bug », and the giant crab of the Crucifixion Woods' marsh, briefly (17:59), and a man-serpent at Archdragon Peak (18:02). In his journal
   (`play-20261026-175134.jsonl`, the launcher's popin-observe, OBSERVED): c1410 draw entities ended by the frustum gate at 3 to
   9 m (serials 7102-7182: off, on, off, on, off at 3-4 m; 15831 and 16085 at 8.9 m, about 17:55-17:56), the draw centre inside
   their bounds E+0xB80 (the bounds follow the character now), yet DS3's own classify on the view's culler (6F310, culler +0x160)
   says outside, and the centre is outside the culler's planes too (`in_view` 0). Only 2 frustum culls of a character in that
   journal had the centre inside the culler's view (c1000 with zero bounds). So either the knight was out of the culler's view
   while on Thomas's screen (the culler not the drawn view: late, or another camera), or he saw it at the screen's edge. The gate
   rows now also test the bounds and the centre against the view the native draws (`drawn_box`, `drawn_centre`, with the camera's
   position), to tell the two apart.

## Tranche 4, step 1: DS3's LOD and draw distances are the same in both renderers (Thomas's theory, refuted for the LODs)

1. **The runs** (2026-10-26 18:21-18:28, ASUS, announced, Gamma at the Anor Londo bonfire without AI, `test-death.flag`: the player
   killed 75 s after loading, the respawn at the same bonfire; build `evidence/I-0100/build-t4a`, `e4ac6d15`): `i100-t4a-al-n1`
   (native) and `i100-t4a-al-o1` (the original, plain Reference), both with popin-observe; the death took (`test_death`, HP 669
   to 0) in each, no fault, the files restored by hash. Reader `Get-PopinLod.ps1` (output `evidence/I-0100/t4a-lod.json`).
2. **OBSERVED**: the LOD manager's scale is 0.756695 at the launch and 1 after the death's reload in both renderers (at 92.5 s
   natively, 95.2 s in the original), the biases 0; 4,843 LOD controllers a load. The last state of each controller (its LOD and
   its draw weight), keyed by model and centre: at the launch 1,019 of the 1,028 keys both renderers have are the same, after
   the death 1,022 of 1,028; the few others are keys shared by several instances of one model (centres within a metre: the
   key's limit, not a difference). The LOD moves while the character stood (35 and 9 models) are at the same distances (median
   ratio 1, none beyond 5 %). The LOD mix after the death is farther in both (natively lod0 234, lod1 263, lod2 531 at the
   launch; 202, 147, 680 after).
3. **Conclusion**: DS3 decides the LODs and the draw distances (the geometry weight) itself and with the same inputs in both
   renderers; the native's draw distance is neither lower nor different. The LOD threshold nearer after a death (tranche 2
   item 7) is DS3's own: its scale goes from 0.757 to 1 at a reload, so every distance it compares is 1.32 times longer (in the
   original too). Not a native defect; making it better than DS3 (the scale kept across a reload) would be a remaster change, to
   the ROADMAP if Thomas wants it. Thomas's « non dessinés alors qu'ils sont dans le champ de vision » holds for the characters
   vanishing near him (step 0 item 3), whose cause is not the draw distance: next step.

## Tranche 4, step 2: the characters vanishing near the player (open)

1. **The Anor Londo pair** (`i100-t4a-al-n1`, step 1's run; the gate rows with the drawn view, OBSERVED): every frustum cull of a
   character has its bounds outside the drawn view too (`drawn_box` 0, 67 rows); DS3's culler and the drawn view agree there.
   c2131 at 143 m, standing, goes in and out every 30-240 frames at the view's edge, its bounds changing with its idle animation.
2. **The Settlement pair** `i100-t4b-settlement-n1`/`-o1` (2026-10-26 18:41-18:47, ASUS, the replayed path with its camera turn,
   no AI, build t4a), OBSERVED: natively 223 frustum culls of a draw entity with `drawn_box` 0, 3 with 1, all three a c1240
   1.7-3 m away with the camera inside or at its bounds (a box behind the near plane for DS3, crossing it for the drawn view's:
   the near plane's distance, a few centimetres). The characters' gaps (off, then on again) after the load: natively 113 (11 of
   30 frames or less), in the original's emits 170 (91 short); the original's emits are not its kept draws (step 2 of tranche 1),
   so this does not show the native worse, nor better.
3. **Thomas's sessions** (`play-20261026-175134.jsonl`, the serials put on the PC clock by its `tick` rows: 17:56:00 near serial
   15,390, 18:02:00 near 36,900, 18:03:00 near 40,460): the silver knight at 8.9 m culled at 17:56:07 and 17:56:12 (serials 15831,
   16085), the man-serpents at 23-51 m going off and on at 18:02 (38085-38610), a group of five together for 1.2 s (38538-38609),
   every cull with the draw centre outside the culler's view (`in_view` 0). Whether the culler differed from his screen there
   (his session: the OLED, borderless 2560x1440 at 280 Hz) is what build t4a's `drawn_box` tells: his play with it is the next
   measure (the runs here could not bring a character near a fast-turning camera).
4. **The margin (T2b, D-0220 « a margin against flicker »)**: DS3's frustum test of a character's bones' box can end it while its
   mesh, wider than its bones, may still be on screen (INFERRED), and flips it at the view's edge as its animation moves the box (c2131 at Anor
   Londo; the Settlement's c1240). First form (`40a7602e`, build t4b): the main walk kept a draw entity whose box, grown by the
   larger of 0.5 m and a fifth of its longest side, still crossed the culler's planes. Its run `i100-t4c-settlement-n2` (19:22-19:25,
   ASUS, OBSERVED, against `i100-t4b-settlement-n1` without it): frustum culls of draw entities 226 to 154, 90 kept by the margin,
   the characters' gaps after the load 113 to 86, but the near c1240's short gaps not fewer (8 to 12 under 6 m): they now went
   off as « absent », their tree node ended by DS3's node test before the entity test (6160 at 7.9 m: margin, absent, margin,
   absent on serials 8710-8712). So the margin must hold for the nodes too: second form, the main walk on a copy of the view's
   culler whose planes and world box are pushed out by 1.5 m (as the shadow and reflection walks run on copies), so DS3's own
   node and entity tests keep everything within 1.5 m of the view; `no-cull-margin.flag` walks the view's culler; the gate rows
   say « margin » for a draw entity the view's culler alone would have ended. Not a stand-in: the objects themselves, drawn a
   little beyond the edge. Its run `i100-t4d-settlement-n1` (2026-10-26 19:43-19:46, ASUS, build t4c `ef7c20eb`, OBSERVED, the
   same reader as the first form's: the characters' off-then-on gaps after the load's first 300 frames): 75 gaps, 3 of them of 30
   frames or less (11 in `i100-t4b-settlement-n1` without the margin), the near c1240's short gaps 3 (8); 127 « margin » gate rows.
   Accepted with it (final review L6): the characters within 1.5 m of the view, behind the camera too, are marked drawn, so DS3
   poses them at the full rate and any reader of the drawn rings sees them, a simulation-side change beyond the original.
5. **A start-up fault, first seen** (`i100-t4c-settlement-n1`, 2026-10-26 19:10, build t4b, OBSERVED): 0xC0000005 before the first
   native frame, in `effect_draw.cpp:168` `ClearRtv` passing a context that is not the native one to DS3's own GX clear
   (`clear_rtv_original`, reading address 0x10), from DS3's thread FUN_140EDAAE0 → D55330 → D56810 (dump and report in the run's
   `crashes/`). Code tranche 4 did not touch (the margin runs once the world walks); the first of its kind in the 62 crash
   folders of `evidence/I-0066/artifacts`. Recorded as intermittent; run again once (`-n2`); an RCA if it recurs.
6. **The original does not keep a character's draw when it stops emitting it** (`i100-t4e-settlement-o1`, 2026-10-26 20:03-20:06,
   ASUS, the Settlement path, the original with original-probe, build t4d `ce85a0da`, OBSERVED with `popin_reference_chr`: per game
   step, a character model's emitted draw entities beside the rows the original executed for it at 63CB0): 5,842 windows of
   c1240; in none were its rows executed without an emit; of 36 steps where its emits dropped, its rows dropped in the same step
   35 times. So the original's character draws follow DS3's frustum test frame by frame, as the native's: the edge culls of
   items 1-2 are DS3's own (the original's emits had more short gaps of the near villagers than the native's, item 2). The
   margin of item 4 is a change beyond the original (T2b), not the cause of a native difference.
7. **What the man-serpents, the woods' crab and the villagers did in Thomas's session: DS3's distance fade** (OBSERVED,
   `popin_native_fade` of `play-20261026-175134.jsonl`): at 18:02-18:03 the man-serpent c1390 faded out to 0 over 12 frames
   (0.2 s, dithered) each time it walked past 23.0-23.9 m and back in below 19.8-20.5 m (serials 37928-41239, eight fades out);
   c2271 at 51.7-54.1 m (the Crucifixion Woods, 27611-30443), a c1240 at 24.4 m, the silver knight c1410 at 57-59 m. Those are
   DS3's own draw distances (the LOD controller's weight, D1CCE0 against the LodParam row), and in that session the LOD manager's
   scale was very likely 1 (Thomas had died and travelled; step 1: 0.757 at a launch, 1 after any reload, in both renderers).
   The scale (6EB60, DS3-1152-F-0006EB60): tan(fov/2) / tan(ref/2) of the LOD manager's camera, its fov +0x140 against its
   reference +0x2D4; the drawn vertical field of view is 0.7505 rad all through `i100-t4a-al-n1` (`native_sss_inputs`), so
   0.757 is that fov against a reference of about 0.958 rad (item 8: the reference word is the one a death's reload changes, read
   natively; the original's scale was read, not its word). So after a death DS3 itself draws its characters and LODs 1.32 times
   nearer, in the original too (deaths measured; a travel's reload measured natively since, the LOD scale study item 1): the man-serpent's 23.8 m would be about 31 m at a launch. Keeping the launch scale through the
   session would be a change beyond the original (T2b), Thomas's decision.
8. **Option A, measured** (`lod-launch-scale.flag`, `0ceabba7`, build t4f; opt-in until Thomas decides, both renderers): after
   the step-0x4F task D1DE70 the first reference fov seen is put back and the scale recomputed. `i100-t4f-al-n1` (20:25, without
   it, OBSERVED): the LOD camera's fov 0.750492 and reference 0.959931 at the launch, the reference 0.750492 after the death
   (`popin_lod_scale`), so the scale 1. `i100-t4g-al-n1` (20:45, with it, OBSERVED): the reference put back once after the
   death (`native_lod_launch_scale_kept`: 0.750492 to 0.959931, scale 1 to 0.756695; DS3 did not write it again), the scale
   0.756695 all through, the last LOD mix after the death lod0 224, lod1 243, lod2 561, as at a launch (234, 263, 531 in
   `i100-t4a-al-n1`); no fault. Put to Thomas (20:15) as option A against B (DS3's behaviour).
9. **Step review** (a fresh Opus at high, D-0198, on `main...HEAD` at `e2881846`): no high finding. M1, `test-death.flag` and
   `lod-launch-scale.flag` silent in the original-probe mode (their hooks are installed after its early return): test-death is
   now refused there with a row and a failed setup, the D1DE70 hook installed there too. Lows fixed: L1 the play launcher refuses
   a `test-*` diagnostic name (test-death would kill the player in his own save); L2 the launch reference kept by LOD manager;
   L4 the scale check without a lock on every LOD step (a hash compared atomically); L5 a controller's owner named only when it
   is a draw entity; L6 and L3 written in the margin's comment (the margin is less than 1.5 m near the far corners, where the
   grown world box cuts first; the « margin » rows undercount); L8 a misplaced comment. Not changed: L7 (the 0x300-byte culler
   copy, as the shadow and reflection walks', 6F310 reading only +0x160..+0x23F).
10. **Final review** (a fresh Opus at xhigh, D-0198, on `main...HEAD` at `ac94647f`): no high finding in the code; the gate not
    yet run (next). M1 the shipped margin's run recorded (item 4, `i100-t4d-settlement-n1`); M2 STATE and the investigations
    index rewritten; M3 the silver knight's cull at the view's edge marked INFERRED (Thomas's screen at that moment is not
    known; his play with the drawn-view rows is a next action). Lows: L1 `popin_lod_scale` now prints the manager's width and the
    LOD gate as integers (they were read as floats); L3 a reference equal to the camera's fov is never taken for the launch's; the
    nit not taken (the variant is also read by the 1142 SH samples; the build said so); L6, L7 written above (items 4, 7). Not changed, recorded: L2 the step review's
    fixes are built (t4h) but run only through the gate's default flags, the opt-in option A and popin-observe paths to be run
    again before Thomas plays them; L4 ObserveRowExecute's lock on every row of the original in original-probe with
    popin-observe (a comparison run's own cost, not a gate path); L5 `popin_reference_chr`'s name cache by model address (a
    reused address after a reload may be mislabelled until 4,096 names); L8 the gate without its memory leg (D-0218): the margin
    admits a few more models at the view's edges, which DS3 has already loaded, and changes neither the loads nor the streaming.
11. **The fixes' step review** (a fresh Opus at high, on `e2881846..HEAD`): no high finding. M1, option A keyed by manager would
    leave a manager made anew at a reload (its first reference already the camera's) uncorrected: such a manager now takes the
    launch reference another manager kept, and the kept row names its manager (an address change across a reload is not
    measured: option A's run before Thomas plays it, Next action). L5 the LOD gate word 0x14463F8A0 printed as its raw bits
    (`lod_gate_bits`; its type, int or float, is not established). Not changed, recorded: L1 one scale hash for every manager
    (rows on every step if two managers with different scales step together; not seen); L2 the 17th manager dropped silently;
    L3 the light-observe `lod_step` rows keep their old reading of +0x110 and of a non-draw owner (pre-existing, diagnostics);
    L4 the test-death refusal shares the probe mode's ready code -7 (its row names it).
12. **Gate** `i100-t4` (2026-10-26 22:24-22:41, the ASUS 1920x1080@60, without the memory leg: item 10 L8, D-0218; `55ae2106`):
    PASS. Period 16.667 ms; frames ending over 20.4 ms 2 a run (2 at `i100-t3`), over 15 ms 18 (16), display late 11 (8), game
    steps over 20.4 ms 2 (1), all within the base's thresholds; pictures 0.75 and 0.88 against the reference; no fault.

## The LOD scale study: option A or B (LOD-SCALE-STUDY-PLAN)

Opened 2026-10-26 22:52 in a fresh thread (branch `claude/angry-babbage-fa5d4a`, `main` at `72c9e70f`), on Thomas's three
questions (« Ça aurait quel impact ? L'original fait comment ? Et Elden Ring ? ») and two added during it (2026-10-26 23:34:
« ça peut déclencher l'AI des ennemis d'avoir 55° ? » and the same comparison with Elden Ring). Read-only Ghidra on the
mirrors (`ghidra-mirror-i0066`, `er-mirror-i0070-a`; the reviewer of item 6 used `ghidra-mirror-i0068` and `er-mirror-i0072-a`), scripts `I0100GlobalRefs.java`, `I0100FindFloat.java`, `I0100DispUsers.java`, and a raw scan of the image for RIP-relative operands and stored pointers (`riprefs.py`, `bytesat.py`, `ptr.py`, run with `py -I` on the main checkout's `game/DarkSoulsIII.exe`), which finds code Ghidra left undisassembled; their outputs and the reviewer's scripts are in `evidence/I-0100/lodscale/ghidra/`.

1. **Why DS3 replaces its reference at a reload** (OBSERVED in the code, DS3 1.15.2): the LOD manager is a view of the class
   built by 6E780 (vtable 143D42D58, 17 call sites in 14 functions: StageRend DFEEB0, the LOD manager D1D150, shadow and character
   views). That constructor copies an 8-byte global 14473F6C8, {reference height, reference fov}, into the view's +0x2D0/+0x2D4;
   6EB60 then computes the scale +0x150 = tan(fov/2) / tan(ref/2) (over height / reference height when the latter is not 0).
   The global holds 720 (static data) and the class's default fov 0.959931 rad, 55° (14473F6BC), copied there by a dynamic
   initializer at 142640640 (listed in the image's initializer table at 142741C60, INFERRED to be the CRT's). Every frame two
   writers replace it with the live camera's {height, fov} through 6E770: the LOD manager's step-0x4F task D1DE70 and StageRend's
   per-frame update E006A0. So a view made before any frame (the launch's LOD manager) takes 55°, and a view made after play (the
   LOD manager StageRend makes again at every load, INFERRED from the measured reload) takes the camera's 43° (0.750492): scale 0.757 at a launch, 1 after any
   reload. Measured in both renderers: the original's reference word itself, 0.959931 at the launch and 0.750492 after the
   death (`i100-ls-al-o1`, 2026-10-26 23:25, plain Reference with popin-observe and test-death), and a travel's reload does the
   same as a death's (`i100-ls-tr-n1`, 23:29, native, Central Irithyll to Anor Londo: 0.959931 then 0.750492, scale 0.757 then 1).
2. **Elden Ring 2.7.1** (OBSERVED in the code, never run): the same class (constructor 141A609E0: +0x110/+0x114 1280x720, +0x140
   the default fov, the global 143C266D0 copied into +0x310/+0x314) and the same formula (141A614B0, 141A61AC0), with a guard (a
   reference of 0 or less gives the scale 1). Its global has one writer, 141A610B0, called once, from the initialisation routine
   140B1BA70 (which builds the engine's subsystems; its running once per session is INFERRED), with {height 0, fov 43.0 x
   0.017453292 = 0.75049 rad}. No per-frame writer: in ER every view made after that routine, at a launch and after a reload, has the reference 43° (INFERRED; one made before it would take 0, which the guard also turns into the scale 1), so
   the scale is 1 at the default camera fov all session. ER's own gameplay camera fov is not checked (hypothèse: 43°, the
   constant is also read by 140CCCDD0, a camera setup).
3. **So, intended or forgotten** (interpretation, hypothèse): DS3's design is « the reference is the live camera's fov », the
   mechanism coded on purpose (a global refreshed every frame for the views made later); the 55° at a launch is the class's
   default left in the global before any camera exists, an initialisation-order effect, not a choice of distances. Elden Ring
   keeps the reload's result and makes it constant (43° set once). By FromSoftware's own later engine, the intended state is B
   (scale 1 at 43°); A (scale 0.757 kept) is what DS3 shows only from a launch to the first death or travel, every session.
4. **Option A's measured cost** (`Run-I100LodScaleSeries.ps1`, reader `Get-I100LodScale.ps1`, output
   `evidence/I-0100/lodscale/ls-ab.txt` and `ls-ab.json`): five pairs B then A, native, measure mode without the debug layer
   (checked in each `baseline.json`), build `evidence/I-0100/build-ls` (`72c9e70f`, module EC047971…), the ASUS alone (the only
   screen on), the game's 1280x720 window at 60 Hz, Gamma at the Anor Londo bonfire without AI, test-death, then 20 s still at the
   respawn counted from the world's stop (8-10 s of load, then about 10 s standing) and eight camera turns of 250 units each held 3 s (2026-10-26 23:32 to 2026-10-27 00:43); every run without fault, the
   saves restored by hash, A's row `native_lod_launch_scale_kept` once in each A run. OBSERVED, means of the five pairs over the
   camera turns after the death (B, then A): objects DS3's culler selected 449 and 460 (pair to pair 410-481 and 418-495: the
   same within the pose's timing), content draws 608 and 607, collect 1.142 and 1.142 ms, prepare 0.991 and 0.962 ms, record
   1.008 and 1.004 ms, the GPU wait 0.059 and 0.057 ms, private memory 7,466 and 7,466 MB, local video memory 1,815 and 1,815 MB,
   frames over 25 ms 2.2 and 2.4 a window; the GPU's 3D utilisation (Windows' « GPU
   Engine » counter, pair 5 only) 6.6 % and 6.6 %. The 40 s before the death are the same in A and B (1,296 draws, 665
   objects in every run), as they must be. After the reload 8 model instances (7 models; 16 rows of `native_model_stream_initialized`, two per instance, one per stream table) are initialised in every run, A and B, as at the launch's load: the respawn's own. Standing at the respawn, A draws a few more (+5 to +11 draws when the view is the same), hidden in the turns' spread (586-662). Conclusion: at 60 Hz on this PC, A costs nothing measurable on the CPU and in memory; A changes which LOD
   DS3 chooses (finer meshes, `i100-t4g-al-n1`: the LOD mix after the death as at a launch) and the distance at which DS3 fades
   objects and characters out. Not measured or substituted, against the plan: the display and its frames over 20.4 ms (PresentMon failed, item 5; the engine's frames over 25 ms stand in), the `popin_native` counts, the LOD mix and the first admissions (popin-observe was off in the series; the LOD mix is `i100-t4g-al-n1`'s), Run-Endurance's memory counters (the module's own rows, whose private bytes include video memory on this PC), the replayed path (a scripted camera turn instead); and the only work A adds (hypothèse: the finer meshes' vertices) cannot show with the GPU 6.6 % busy at 1280x720 and 60 Hz. The runner's own polling (the journal read once a second, eight short PowerShell processes per turn series, the GPU sampler) ran inside the windows, the same in A and B. Before A becomes the default, the GPU-bound measure to take: the GPU's 3D share or PresentMon's `msGPUActive` on the OLED at 2560x1440 and 280 Hz.
5. **Instruments' failures, said as they are**: PresentMon left no CSV in the eight runs b1 to a4 (started before the game,
   then at the title; the gate's PresentMon worked at 22:24 the same evening). The difference with the gate's working command is
   `--v1_metrics` (this runner asked for the default v2 metrics to read `MsGPUBusy`) and the process that starts it (here a
   hidden nested PowerShell); the claim « it needs elevation » made in the thread at 23:53 was wrong (hypothèse for the cause,
   checked at the next run, item 8: not the metrics; a bench test at 00:25 on `dwm.exe` and Claude's window, v1 and v2, wrote no CSV either, nothing presenting there, so it decided nothing). The GPU's share came from the « GPU Engine » counter instead (pair 5). The
   runner's script was edited while pair 3 ran (its driver for a3 already written, so the run was unaffected), against WORKFLOW's
   live rule. The ETW sessions the killed PresentMon left were stopped (`logman stop -ets`), the gate's `DS3Replay` left alone.
6. **Who reads the reference and the scale: rendering or gameplay** (Thomas, 2026-10-26 23:34; my census with `I0100DispUsers.java`,
   then a fresh Opus reviewer redoing the references itself, 2026-10-27 00:50-01:10, Thomas's agreement 2026-10-27 00:32, no Fable).
   DS3, verified: the global 14473F6C8 is read only by the constructor 6E780 and written only by 6E770 (from D1DE70 and E006A0) and
   the initializer (its write is in raw bytes Ghidra left undisassembled: `F3 0F 11 05` at 142640648 to 14473F6CC); the scale
   exists only in render views (6EB60's one caller 6E9E0, called by engine GX code, D1DE70 and E006A0; the constructor's 17 call
   sites in 14 functions, all GX, StageRend or the LOD manager); the other float readers of a +0x2D4 (22E930, 2BC5B0, 1752BB0,
   A06F90) are other classes (shader constants, a Havok region; A06F90 reads a float where this class keeps an int, hypothèse).
   The readers of the scale: the LOD step D1CCE0 (LOD index, weights, the motion-blur gate E+0x184, all read by the draw emit
   D079C0) and one the first pass missed, **14066BBB0**: a map object's per-frame update (from 140666C60) that tests the item's
   drawn ring and compares |position - view| x scale with a parameter distance (1.0 of hysteresis) to start a GX update of the
   model (140C4C690 → 140C4C6E0, matrices into the model's GX object and its registration with the GX manager): rendering (a
   dynamic texture or model update, hypothèse from MENUS-MAP's names). One simulation-side effect, indirect: the drawn marks
   follow the draw distance, and DS3 poses a drawn character every frame, an unseen one at 2 Hz (tranche 3), so under A a character between about 24 and 31 m (the man-serpent's case) stays drawn and posed every frame after a reload, as at a launch; the readers of the
   characters' rings are still unnamed (no direct displacement reader; reached indirectly). No reader in AI, activation, think or
   aggro was found, in DS3 or in ER (not exhaustive: readers reaching the scene through other pointers, the rings' readers).
   Elden Ring: the same picture (StageRend `CS::StageRend` 140CCA7E0 holds the view at +0x60, RTTI verified); 1406C3670 is the
   exact equivalent of 14066BBB0 (the StageRend view's scale against a parameter distance); ER's game-code readers of a
   +0x310/+0x314 treat it as a float (another class, a camera, hypothèse). The decisive point for gameplay is not a reader,
   though: **the original runs with A's scale at every launch, until the first death or travel**, so whatever A touches, the
   original already does it for the first part of every session; A only removes the switch at the first reload. And the camera's
   own fov (43°) is the same in A and B: A changes the reference, not the field of view the player sees nor the game's rules.
7. **Recommendation** (for Thomas's choice): **A**. It keeps through the whole session the LOD and draw distances DS3 itself shows
   at every launch (1.32 times farther than after a reload: the man-serpent's fade at about 31 m instead of 23.8 m), so the
   picture no longer changes at the first death; its cost is not measurable at 60 Hz on this PC on the CPU and in memory, the GPU read on one pair only (6.6 % in both; item 4); no gameplay reader was
   found and the original already plays with this scale at each launch (item 6). Against it: it departs from the intended state
   that Elden Ring fixed (B, scale 1 at 43°), it is T2b beyond the original, and its cost on the OLED at 280 Hz is not measured.
   If Thomas chooses A: `lod-launch-scale.flag` becomes the default (an opt-out flag), with the run of its path before he plays it
   (tranche 4 items 10-11) and an OLED measure in his play.
8. **Option A by default** (Thomas, 2026-10-27 02:05, D-0222: « Va pour A, après tout c'est un remaster »): a native run keeps the
   launch scale unless `no-lod-launch-scale.flag`; the Reference mode stays the original unless `lod-launch-scale.flag`
   (`22_setup.inc`). Step review (a fresh Opus): no high finding; M1, the per-manager table capped at 16 would have dropped A
   after 16 reloads in a session, now one launch reference kept once for every manager (the control run below shows a new
   manager after the death); M2, D1DE70 now hooked in every native run, so added to the build's prefix pins (a changed prologue
   refuses the launch with its name); M3, the study runner's b* runs now pass `no-lod-launch-scale`; STATE and the index
   rewritten. Control run `i100-t5-al-n1` (2026-10-27 02:23-02:28, ASUS, build t5 before the M1 fix, popin-observe, test-death,
   no LOD flag, OBSERVED): the launch reference 0.959931 recorded on manager 7FF397122E60, after the death the new manager
   7FF3980017F0 corrected (0.750492 to 0.959931, scale 1 to 0.756695), no fault, saves restored. PresentMon with `--v1_metrics`
   from the same hidden nested process left no CSV again: the cause is that process (or its start at the title), not the metrics
   (item 5 corrected); the gate's PresentMon (Invoke-ReplayRun) is unaffected.
9. **Gate** `i100-t5` (2026-10-27 02:44-03:13, the ASUS 1920x1080@60, with the memory leg: A changes the LODs DS3 asks after a
   reload, D-0218; native tree `25f67687e24a`): PASS. Period 16.668 ms; frames ending over 20.4 ms 1.5 a run (2 at `i100-t4`),
   over 15 ms 18.5 (18), display late 10 (11), game steps over 20.4 ms 1.5 (2); pictures 0.81 and 0.84; memory -23 MB a visit.
   OBSERVED in the memory leg `i100-t5-z1`: the launch reference recorded once, then each of the seven reloads' new LOD managers
   (six distinct addresses) corrected to it (the one-reference fix of item 8 live). The leg ended with the known exit heap fault
   0xC0000374 (DS3's own, I-0099), its dump in the run's `crashes/`.
10. **Final review: the resolution, and option A redone** (a fresh Opus at xhigh on `main...HEAD` at `7143bf99`). H1: 6EB60 also
    divides the scale by height / reference height, and the launch pair is {720, 55°}: items 4 and 8 ran at a 1280x720 client,
    where the heights cancel, so they could not show that at another size a reload changes the height term too, which the
    per-manager patch of item 8 left as the reload wrote it (at 1440 DS3's launch scale is 0.757 x 720/1440 = 0.378, the patch
    would have given 0.757 after a death, B 1). M1: StageRend's own view (+0x60, refreshed by E006A0, read by the map objects' GX
    update 14066BBB0) was not corrected. Answer, Elden Ring's mechanism with DS3's launch value: DS3's reference global keeps its
    launch pair, its writer 6E770 skipped by default in a native run (`KeepLodReference`; the D1DE70 hook and its pin removed,
    6E770 already pinned), so every view of every load (the LOD manager, StageRend's view, the others the class builds) is made
    as at a launch: one rule, no case by resolution (Thomas, 2026-10-27 03:47: « la règle universelle, pas de cas par cas »).
    M2 (STATE after the gate) and the lows (the gate's timed runs never reload, so A's frame cost after a reload rests on the
    720p pairs of item 4; the control run's module overwritten by build t5b: each build now in its own folder) answered here and
    in STATE. Control runs, build `evidence/I-0100/build-t5c` (2026-10-27, ASUS, popin-observe, test-death, no LOD flag,
    OBSERVED): `i100-t5c-al-n1` (03:51, 1280x720 window): `native_lod_reference_kept` {720, 0.959931} against the camera's
    {720, 0.750492}, scale 0.756695, unchanged after the death; `i100-t5c-1080-n1` (03:55, full screen 1920x1080, the Settlement
    path): the camera's height 1080, the reference 720, scale 0.504463, unchanged after the death (B would give 1, A's first
    form 0.757); `i100-t5c-1500-n1` (04:08, a window asked at 1500x844, which DS3 made 1440x810: an arbitrary size): the
    reference 720 against the camera's 810, scale 0.672618 (0.756695 x 720/810), unchanged after the death. Reserve (the coordinator, 03:46): A's GPU cost is measured only at 720 on the ASUS; at 1440 and 280 Hz on the OLED
    its distances are about 2.6 times B's and its cost is not measured, a test to ask of Thomas when he is present.
    Step review of the redone A (a fresh Opus): no high or medium finding; the MinHook relocation of 6E770's two instructions, the
    callers' use of RAX (none) and the native scene update's own call of 6E770 (intercepted too) checked; lows answered (the
    reader counts A's new row, the native caller named in the comments, the index and ROADMAP rows say the height term); not
    done, recorded: StageRend's view and the other views are covered by the reasoning (the global constant, its only reader the
    constructor), not by a row of their own.
11. **Gate** `i100-t5b` (2026-10-27 04:29-04:58, the ASUS 1920x1080@60, with the memory leg; native tree `38ffc62c48c7`, the
    redone A): PASS. Period 16.668 ms; frames ending over 20.4 ms 4 a run (1.5 at `i100-t5`, 2 at `i100-t4`: higher, within the
    base's thresholds), over 15 ms 19.5, display late 12, game steps over 20.4 ms 2.5; pictures 0.71 and 0.75 (0.81 and 0.84 at
    `i100-t5`; the firelink path never reloads, so A equals B there: run-to-run spread); memory -5.8 MB a visit. The memory leg
    `i100-t5b-z1` recorded `native_lod_reference_kept` {720, 0.959931} once; it ended with the known exit heap fault 0xC0000374
    (DS3's own, I-0099). The gate's timed runs never reload: A's frame cost after a reload rests on the 720 pairs of item 4.

## Tranche 5: the Settlement's sun casters, the drawn rings' readers (plan)

Opened 2026-10-27 05:31 on branch `claude/focused-hodgkin-e0fe7c` (STATE « Next action » 2), in this order:

1. **The Settlement's sun casters** (tranche 2 item 2: the native casts more models and longer than the original, mostly in
   cascade 3, casts c1100 and c2021 that the original never casts and misses m31_00_00_00_003080). Known: the original redraws its
   four cascades every frame (SUN-SHADOWS-WAVE1 7, so no cascade cache explains it); its casters reach the cascade passes by a
   route not named (no pass admission, 57A20, D079C0 or D04B90 call for passes 1..4, Reference1791); the rule the native applies
   is the G-buffer category 0x02 inside the cascade boxes (D-0180), with the geometry weight of pass 5. The type-2 rows of 63CB0
   also include the local lights' tiles, so the tranche-2 comparison mixed the sun's casters with the local lights' in the
   original. First step: `caster-route.flag` (Reference, with popin-observe; removed in `0e6028d1`), every depth row's return chain counted by chain
   with its models, on the Settlement path, to read the code that chooses the casters from its callers; then that rule in the
   native.
2. **The drawn rings' readers** (tranche 3 step 2 item 2: the item ring item+0x794, the draw entity's E+0xC50, the assembled
   character's E+0xCB0; their readers are not named, the item ring marked only with `item-marks.flag`): a static search
   (headless Ghidra, read-only), then whether the item ring should be marked by default.

## Tranche 5, step 1: the drawn rings' readers (static)

Read-only headless Ghidra on `ghidra-mirror-i0066` by a Sonnet subagent (2026-10-27 05:10-05:50; its scripts and outputs in
`evidence/I-0100/ring-readers/`, the readers of the item ring re-read by the pilot in `decomp-1.txt`), STATIC:

1. **The characters' rings** (E+0xC50/+0xC51 of a draw entity, E+0xCB0/+0xCB1 of an assembled character) have one reader each, a
   virtual pair of the character: slot 114 (+0x390) tests the current slot's bit (ChrIns/EnemyIns 14088CCC0 on [[chr+0x48]+8],
   PlayerIns/ReplayGhostIns 1408BD7B0 on [[chr+0x20B0]+8]) and slot 115 (+0x398) advances the slot and clears the bit it enters
   (140886820, 1408B9670). Their only caller is the character manager's update-rate step 1408DDFA0 (WorldChrManImp's update task,
   1408DF920): a character not drawn since the previous step takes a longer update period (2, 5 or 30 frames by distance band);
   1408DD2E0 then steps a character only on its due frames. This is the mechanism of tranche 3 step 2 (the distant characters at
   2 Hz out of view), now named.
2. **The item ring** (item+0x794/+0x795) is read only by 14066BBB0, the map object's (ObjIns) update through its
   SprjObjModelIns (+0x290, its item at +0x10): when the object has an animation (+0x2B0) and its item's current slot bit is not
   set, +0x3D8 = 0 and the object does not hand its pose to its model (140C4C690); when set, the object's draw distance (with a
   hysteresis) decides. Its slot is advanced only by 1406667C0, the same object's update (ObjIns items only; a map piece's slot
   stays at 0 in the original too, its bit never read). So natively, without the item marks (opt-in `item-marks.flag` since
   tranche 3), every animated map object stays at +0x3D8 = 0 and is never posed (INFERRED from the code; measured in step 2 item 6). The worry
   that kept the item marks opt-in (a mark that never expires) does not hold: the only items whose bit is read are those whose
   slot DS3 moves.

## Tranche 5, step 2: the original's sun casters, measured

1. **The route** (`i100-t5d-route-o1`, 2026-10-27 05:38-05:41, ASUS, Beta at the Settlement without AI, the replayed path, the
   original with popin-observe and the return chains of 63CB0's depth rows, OBSERVED): every depth row runs at render time from the
   sorted command lists (13EEB0 -> 13F470 type 3 -> 59220 -> 5CEC0, B76D0 -> B6FA0 -> 5D5C0 for the grouped draws),
   the FLVER entries of K-0394; so the chain names no selection, only where to read each caster.
2. **The casters by pass** (`i100-t5e-places-o1`, 06:02-06:05, the same path, `caster-place.flag`: the FLVER entries' pass context
   +0x124, the command's pass from its sort key, OBSERVED): the four sun cascades are the passes 5..8 there (the player's parts in
   6 and 7, at 2-8 m from a third-person camera, and in 5 only 11 % of the steps), 9 and 10 two local lights' faces. Over the
   path's 8,400 steps the original casts few models into its cascades: the player always, m31_00_00_00_002100 always (all four),
   o000250 87 % (cascade 3), c1240 36 % (cascade 3), m31_00_00_00_002095 10 %, 002091 9 %, o310520 4 %, o319002 and o319004 4 %,
   never c1100 nor c2021. The native on the same path (`i100-t5e-marks-n1`, 06:05-06:08, OBSERVED, frames summed over the
   instances): 002095 100 %, 002091 77 %, o310520 87 %, o319002 13 %, o319004 9 %, c1100 12 %, c2021 12 %, c1240 and o000250
   over 100 % (several instances). The instance matrix the instrument first read (5CEC0's third argument) was not a matrix; the
   no place could be read: neither the third argument nor the fourth's +0xA0 holds a world matrix (build t5e), and the instrument keeps the model only since build t5f.
3. **The fit is not the difference** (STATIC and OBSERVED): 1B6020 makes two fits per cascade, the drawing one (FUN_140203FB0)
   and the culling one over the cascade record (1B4830's planes, the pass culler +0x410), with the record's own size, depth range
   and alignment; the original's arguments are the same for both in `i66-ref-1539-m31-bind` (callers 1B6343 and 1B63EA: depth
   range 100, size 2048, align 1, blend 1), so the native's cull by its drawing matrices is the original's.
4. **Hypothesis**: the original casts only the category-0x02 models its main view draws (its casters off the screen are absent,
   on the screen while a turn shows them: o310520, 002091, 002095), where the native also admits shadow-only casters, which its
   main walk did not admit. The experiment `view-casters.flag` (no shadow-only caster) measures it against the original's shares.
5. **The rule, measured** (OBSERVED, the same path, ASUS, reader `Get-CasterPlaces.ps1`, output `evidence/I-0100/t5g-casters.json`):
   without shadow-only casters (`i100-t5f-view-n1`, 06:36) 002091 falls from 0.61 to 0.06 of the frames (original 0.05), o310520
   from 0.77 to 0.08 (0.03), but c1100 and c2021 still cast (0.13, 0.12; never in the original). With the second rule, only the
   models whose pass-5 selection is not empty (the geometry weight, and the suppress bytes the probe pick sets for the entities
   it lights, which D06E10 / D3DA20 test for passes 5..8; a model the main walk admitted through the 0x45 view alone does not
   cast), `i100-t5g-view-n1` (07:00) against `i100-t5g-places-o1` (06:57): c1100 and c2021 0 (0), c1240 0.54 (0.45), the
   player's parts 0.73 (0.74: also off under the shelter, the probe pick's suppress byte, INFERRED), 002091 0.09 (0.07),
   002095 0.11 (0.08), o000250 0.89 (0.89), o310520, o319002, o319004 0.04 (0.04). The rule is the default since build t5g
   (`no-view-casters.flag` gives the former walk). Its cost: the shadow walk 0.069 ms a collect against 0.356 (`native_shadow_casters`,
   `i100-t5h-720-n1` against `i100-t5f-marks-n1`). Run to run, c1240 (the near hollow, pushed by the walk) varies: 0.29 in
   `i100-t5h-720-n1`, 0.54 in `-t5g-view-n1`.
6. **The item marks, measured** (OBSERVED, `popin_object_syncs`, the animated map objects' pose hand-over 14066BBB0): the original
   hands 9.5 % of the calls over (28,091 of 294,600, `i100-t5g-places-o1`); natively without marks none (0 of 287,400,
   `i100-t5g-nomarks-n1`, `no-drawn-bits.flag`), so before this tranche no animated map object was posed natively (INFERRED for
   play: on this path); with the marks by default 15.2 % (`i100-t5h-720-n1`), 14.5 % without the culler margin
   (`i100-t5h-nomargin-n1`): the native hands over about 1.5 times as often as the original, not because of the margin; the cause
   is open (more objects marked than the original's pass-0x45 draws, or the mark's timing against DS3's advance), a CPU cost, not
   a visible defect.
7. **Control runs at three sizes** (build t5g, OBSERVED): 1280x720 (`i100-t5h-720-n1`), full screen 1920x1080 (`-1080-n1`) and a
   1500x844 window (`-1500-n1`): no fault; the two runs with `-GraphicsLevels` end with the controller's exit 0xD3126612 and
   `closed` 1 after about 20,500 frames, as option A's control runs `i100-t5c-1080-n1` and `-1500-n1` did (pre-existing, not this
   change). At 1080 and 1500 002091 and 002095 cast in 0.60-0.73 of the frames (0.09-0.19 at 720) and o000250 in 0.39: the
   draw distances grow with the output height (D-0222). The original at 1920x1080 (`i100-t5i-1080-o1`, 07:55, the same exit):
   002091 0.60 (native 0.60), 002095 0.61 (0.73), o000250 0.38 (0.39), o310520 and o319002/4 0.02 (0.02), c1240 0.35 (0.58),
   the player's parts 0.89 (0.86): the rule holds at 1080 too. The original in a 1500x844 window could not be measured:
   twice the replay stopped on `foreground_lost` (`i100-t5i-1500-o1`, `-o2`), the path not played; insufficient information
   there.
8. **Open divergence**: the player's part FG_A_2012 casts natively in all four cascades (0.73-0.87 of the frames) and never in
   the original, which draws it in its main view (`popin_reference` on at step 1 in `i100-t5g-places-o1`, no
   `reference_caster_places` row). Possibly the instrument: `popin_native_shadow` counts a model's cascades before the per-mesh refusal of the cascade rows (material +0xD1, `10_options_and_passes.inc` line 249, `no_cast_meshes`), so a model whose meshes all refuse casts nothing natively either; to check by its depth draws in the next tranche.
   The rule is measured on the Settlement path only (one zone; the other witness zones are not).
9. **Step review** (a fresh Opus at high, on `main...632556a7`): no high finding. M1 the runner's leftover-flag list lacked
   `no-view-casters.flag` and `caster-place.flag` (a killed run's flag would have reverted the rule silently in the next run):
   added, the retired `item-marks.flag` dropped. M2 FG_A_2012 and the rule's scope (item 8). Lows: the reader's per-instance
   sum replaced by the per-window cascade sum (the fourth argument is not a stable instance; 002100 read 4.0); the
   caster-place comment says original-probe only; the shadow walk's map pieces go no further than their known children under
   the rule (no eligibility calls for children it drops); the plan's instrument name. Not changed, recorded: the three hooked
   RVAs (0x66BBB0, 0x5CEC0, 0x5D5C0) are not pinned at start, as the other diagnostic hooks (D3DA20, CFDE70, D1CCE0, 63CB0).
10. **Gate** `i100-tr5` (2026-10-27 08:39-08:56, the ASUS 1920x1080@60, without the memory leg: the rule and the marks touch
    neither the memory, the loads nor the streaming, D-0218; native tree of `2579ec33`): PASS. Period 16.6685 ms; frames ending
    over 20.4 ms 2 a run (4 at `i100-t5b`), over 15 ms 18.5, display late 7.5, game steps over 20.4 ms 1.5; pictures 0.83 and
    0.91 against the reference; no fault. The control run of the step review's fixes (`i100-t5j-720-n1`, 08:36, build t5h):
    the same casting shares (002091 0.09, o310520 0.04, o000250 0.90; c1240 0.69, run to run), the shadow walk 0.066 ms.
11. **Final review** (a fresh Opus at xhigh, on `main...087c0c4a` and `d99862cd`): no high finding; the hooks right in every mode,
    the gate record matching the committed `native/` tree, the rule a mechanism. M1, the comparison measures different things in
    the two renderers: the original's side counts each window's most-used cascade (a model moving between cascades within a
    window is undercounted: the original's figure is a lower bound) and one draw per grouped call (5D5C0's instance count is not
    read); the native's counts a model's cascade bits before the per-mesh no-cast refusal. So the claim is worded: the same
    models except FG_A_2012, at close shares, 002095 above the original in every native run (0.11-0.19 against 0.08 at 720,
    0.73 against 0.61 at 1080), c1240 varying run to run (one original sample at 1080). The reader's fix (the native's actual
    cascade rows after the no-cast refusal, the original's union of passes 5..8 per step and the grouped count) is the next
    tranche's first step, before the rule is measured on other paths. Lows, recorded for the next tranche, not changed here
    (each would change `native/` after the gate): frames with no sun caster now occur on the gate's path (Firelink, inside the
    shrine: `native_shadow_casters` [0,0,0,0] at calls 9,600-10,800 of `i100-tr5-v1`), where the native then skips the
    screen-space sun shadow and draws the unshadowed light programme, the original choosing its programme by pass +0x710 and not
    by casters (the two pictures with an empty atlas are the same, INFERRED: every sample lit, not shown); assembled characters
    other than the player (phantoms, summons, assembled NPCs) are admitted without a per-entity view test, so one behind the
    camera still casts (not measured); off-screen casters no longer cast (a tower behind the camera no longer shadows the ground
    in view), as the original (the trade-off told to Thomas, 2026-10-27); `popin_object_syncs` may count a call that left
    14066BBB0 early with a stale +0x3D8 (the 1.5 times of item 6 an upper bound); `CasterPlacesWindow` names models up to 600
    steps after their draw, its totals uncapped, its keys hashed; 0x66BBB0 unpinned though hooked natively with popin-observe;
    5D5C0 is the grouped entry (B6FA0, lightmapped rigid draws too, K-0151), not the skinned one (items 1-2 above). Done: the
    stale lines (the opening time, item 2's places, step 1's « to measure », item 10's line), STATE's row rewritten under 300
    words, « posed again » corrected to « posed », CONTRACT's safety line aligned with D-0218. The comment of `NativeViewCasters` (« the same models at the same shares ») is reworded with the next native change, not here (the gate's tree).

## Tranche 6: the casters' measure tightened, the rule on the other paths (plan)

Opened 2026-10-27 09:40 on branch `claude/kind-kowalevski-ef0315` (STATE « Next action » 2), in this order:

1. **The measure** (tranche 5 step 2 item 11, M1): one figure on both sides, per model the share of the path's steps (original)
   or frames (native) in which at least one of its instances is drawn into at least one sun cascade. The original:
   `reference_sun_casters` rows (per 600-step window, per model, the steps it was drawn in any of the passes 5..8, its instances
   and grouped draws counted once; names read at the step after the draw, not 600 steps later). The native: `popin_native_shadow`
   now follows the cascade rows the pass preparation made (after the per-mesh no-cast refusal of material +0xD1 and the row gates),
   written after the preparation, with the admission's cascades as `admitted` and `refused_rows` when every row was refused; the
   reader takes per model the union of its entities' casting intervals. Build t6a (diagnostics only, behind popin-observe).
2. **The rule on the other paths**: the Settlement, Firelink and Ariandel replayed paths, the original against the native with
   the tightened measure (ASUS); FG_A_2012 read by its drawn rows. Archdragon Peak has no replayed path nor witness save yet
   (ROADMAP: a reference run warps a new Shebang character there, D-0174).

## Tranche 6, step 1: the tightened measure on three paths

Series `i100-t6a-<path>-o1/-n1` (2026-10-27 10:02-10:23, ASUS, the default 1280x720 window, build t6a of `fe66b95b`,
`popin-observe;test-no-ai;caster-place`, the original first; no fault, every restoration by hash; readers' outputs
`evidence/I-0100/t6a-casters-<path>.json`). Shares: per model, the steps (original) or frames (native) in which it casts into any
sun cascade (OBSERVED):

1. **Settlement**: the same models as tranche 5. 002091 0.09 (native 0.06), 002095 0.16 (0.28), o000250 0.87 (0.90), o310520 and
   o319002/4 0.04 (0.04), c1240 0.25 (0.40), the player's parts 0.71 (0.49: the native run ended inside the shelter, its last
   1,200 frames without a sun caster, end.png; the windows before match within the path's drift), **FG_A_2012 0 (0.49)**.
2. **FG_A_2012 is not the instrument**: its native cascade rows are drawn (no `refused_rows` on any model of the three runs, the
   per-mesh no-cast refusal never applies to it), and the original makes no depth row for it at all (never in 63CB0's
   `popin_reference_shadow`, never in any pass 5..0x38 of 5CEC0/5D5C0), while FG_A_7002 (the same material name, the eyelashes)
   casts in every cascade. FG_A_2012 is the eyebrows (MENUS-MAP §60, `P[ChrCustom_Hair]_al`, blend 2, forward). The refusal is
   upstream of the rows in the original (step 2).
3. **Firelink**: the native casts characters the original does not: an NPC's parts (`BD_F_9900_L`, `FC_F_0000`, ...) 0.31
   (original 0), a second character (`BD_M_7800_L` with its parts) 0.27 against 0.07 (the original casts it only in the last two
   windows, the path's end); and misses objects the original casts: o400700 0.11 (native 0, refused by its main-view frustum),
   o001510 0.08 (native 0; **the original never emits it in its main view**: no `popin_reference` row), m40_00_00_00_004201 0.10
   (0.02), 004202 0.05 (0), o000250 0.20 (0.12).
4. **Ariandel**: the player's parts 1.00 in both, o000250 0.99 (0.99), but distant objects cast natively far more:
   o456301 at 94 m 0.14 (native 0.89), o457100 at 166 m 0 (0.88), both admitted by the native's main walk throughout.
5. **Conclusion**: tranche 5's rule (the casters are the main walk's models with a pass-5 selection) fits the Settlement only.
   The original casts objects its main view never draws (o001510), so its casters are not a subset of its main view; it refuses
   some parts (FG_A_2012) and characters (inside the Firelink shrine) and distant objects (Ariandel) by tests the native does not
   have. The original's caster selection itself is the next question (step 2: its record-time route, STATIC, then the rule).

## Tranche 6, step 2: the original's caster selection, read and ported

1. **Read (STATIC, a Sonnet subagent with the `ghidra-research` brief on `ghidra-mirror-i0066`, 2026-10-27 10:25-10:52, scripts and
   outputs in `evidence/I-0100/t6-caster-route/`; the culler builder re-read by the pilot, `dec-23c1e0.txt`)**: there is no
   persistent caster list. `GXSgShadowSceneImpl` (vtable 143D4A378) slot 4, 2057D0, calls F90F0, which for each cascade i makes a
   pass context with pass 5 + i (BEEE0 writes +0x124), fits the cascade (1B6020) and runs the scene walker C2EC0, which maps
   passes 5..8 to the category 0x02 and walks the world (204C30 -> 234620) with the pass culler +0x410; each visited entity takes
   its own tests (D06E10: E+0xC3C bit 0, the visibility predicate D03430, the area mask, the weight +0x170 times the fade above 0,
   the suppress byte E+0xA5D for passes 5..8; D04B90/D3DA20 for items: +0x130, +0x71A; D02A30 for assembled characters' parts,
   no pass test), then 57A20 -> 5C0E0 records the depth row (mask 0x107) every frame. For passes 1..4 1B6020 sets 1B4830's
   record as the culler; **for passes 5..8 the culler is sun manager +0x170 + 0x100 i, made by 23C1E0** from the pass context's
   camera (+0x130: pose +0x50, its slice), the sun direction and a length: the camera slice's frustum, its planes facing the sun
   kept and the silhouette edges extruded toward the sun, up to 12 planes (+0x10.., count +0xF0) and a box (+0xD0, +0xE0).
2. **Observed in the original** (`caster-place.flag` with the 23C1E0 hook, `i100-t6b-ariandel-o1` and `-firelink-o1`, 11:18-11:25,
   ASUS): 12 planes for every cascade, length 100 and scale 1.0 (the sun manager's +0x914 + 0x50 i in 1B6020; natively the record's depth range, also 100 at
   these zones: INFERRED the same value), the volume of cascade 3 about 130 by
   120 by 110 m around the slice.
3. **Ported (build t6c, `f4234509`)**: the native fit already runs the genuine 239610 on a native camera record per slice; the
   genuine 23C1E0 now runs on the same record (`Shadow::caster_volumes`), and the shadow walk's per-cascade test (`Mask`) also
   requires the entity's box inside the cascade's volume. Off-screen entities in a volume are admitted for the cascades (as before
   tranche 5); a model the main walk admitted casts only with a pass-5 selection (tranche 5). `no-caster-volume.flag` gives
   tranche 5's rule. The native volume of cascade 3 at Firelink equals the original's to 0.01 m (`native_caster_culler` against
   `reference_caster_culler`: lo (241.40, -80.52, 498.63), hi (404.60, 25.23, 598.87)).
4. **Measured** (`i100-t6c-<path>-n1`, 11:41-11:52, ASUS, against `i100-t6a-<path>-o1`, OBSERVED): **every object and map
   piece at the original's share** on the three paths: Ariandel o456301 0.14 (0.14), o457100 0 (0), o459001 0 (0); Firelink
   o001510 0.09 (0.08), o400700 0.11 (0.11), 004201 0.11 (0.10), 004202 0.06 (0.05), o000250 0.20 (0.20), 000350 0.84 (0.85);
   Settlement 002091 0.09 (0.09), 002095 0.10 (0.16), o000250 0.87 (0.87), o310520 and o319002/4 0.04 (0.04), c1240 0.28 (0.25),
   the player 0.74 (0.71). The Firelink NPC (`BD_F_9900_L` with its parts) no longer casts, as in the original. Left: FG_A_2012
   (the eyebrows, every path), and at Firelink the player (the `7800` set) 0.20 against 0.07 (and c1400 0.15 against 0 in this original run, not a divergence: step 5 item 5): during
   the walk the native casts the player into cascades 0 and 1 for about 83 frames in 600, the original never into cascade 0
   there and into 1..3 only at the path's end (`reference_caster_places` by pass).

## Tranche 6, step 3: the two divergences left

1. **FG_A_2012, the eyebrows** (`caster-place.flag`, the 57A20 hook, `i100-t6d-*-o1` 12:09-12:16, `i100-t6f-firelink-o1` 12:55,
   OBSERVED): the original records 57A20 calls for FG_A_2012 in the sun passes, with the model's row-mask word +0x220 = 0x10, which
   5C0E0's depth-row mask 0x107 refuses; FG_A_7002 (the eyelashes) has +0x220 = 2 and casts. 0x10 is FUN_140142130's class for a
   forward material whose instance byte +0xD0 is set: FG_A_2012's material +0xD0 is 1 in the original, 0 natively at admission
   and at row time (`native_caster_row_inputs`, `i100-t6f-firelink-n1`), so the native classes it 2 (transparent) and casts it.
   The writer of +0xD0 is not run natively (its static search: step 4).
2. **The Firelink character at 30 m** (the `7800` set with FG_A_0100, a static NPC, not the player; the player, the `9900`
   female set, casts at no time on that path in either renderer since t6c): the original records it in passes 6..8 only at
   the path's end (`reference_caster_places` with the recorded passes 0x46..0x48), never during the walk; natively its
   4-metre box around the placement crossed the near cascades' volumes for about 80 frames a cycle. Its own box at +0x40
   (about 1 x 0.8 x 1 m) gives no cascade during the walk and cascades 1..3 at the end, the original's
   (`native_caster_box_masks`, `i100-t6f-firelink-n1`). Build t6g: an assembled character's shadow walk test takes its box at
   +0x40 when it holds one (`NativeEntityBox`: ordered, under 50 m, centred within 5 m of the placement; the first frames hold
   another box), else the 4-metre box. c1400 (a draw entity at 15 m): see step 5 item 5.
3. **Firelink with the character's own box** (build t6i, `i100-t6i-firelink-n1` 13:46 against `i100-t6a-firelink-o1`, OBSERVED):
   the NPC 0.08 (original 0.07); the objects unchanged (o001510 0.09, o400700 0.12, 004201 0.11, 000350 0.83); c1400 0.16 (step 5
   item 5). Settlement and Ariandel (`i100-t6i-*-n1`): unchanged but c1240 0.50 at the Settlement (0.25 in the original, 0.28 in
   t6c: the hollow the walk pushes varies run to run, tranche 5 item 5).

## Tranche 6, step 4: the materials' GX00 item

1. **Read (STATIC, a Sonnet subagent, `evidence/I-0100/t6-special-d0/`)**: the FLVER material instance (`GXFLV::GXFlverMaterial`,
   vtable 143D52878, constructor 140140470) holds at +0xD0 its « Ex Param » block (constructed by 14022B550; the member names
   read from the serializer 1402306C0): +0xD0 « Decal », +0xD1 « [Shadow] DisableCasting », +0xD4 « [PaintDecal] SlotNo »,
   +0xD8 « [Shadow] CullingMode ». The only writer from data is 14022B7C0, the GX00 item of the material's GX list, called by
   141070 (vtable +0x58, the GX item parser) from the resource factory 82220 at the model's load; item version 0x65 sets all four,
   another version Decal and SlotNo only. 141070 then stores 142130's class at +0x18.
2. **Observed** (`i100-t6i-firelink-o1` against `-n1`): FG_A_2012's material in the original +0xD0 = 1, class +0x18 = 0x10, the GX
   tier byte +0x4E8 = 38; natively +0xD0 = 0, +0x18 = 0xFFFF, +0x4E8 = 255: the native material factory (`BuildModelMaterial`,
   in place of 82220) never runs 141070 and parsed GX03, GX05, GX07, GX09, GX14, GX04 and GX90 itself, not GX00. So natively no
   material was ever a Decal and none ever refused its depth rows (D-0180 C2's no-cast read a byte always 0: no `refused_rows`
   in any run of this tranche).
3. **Parsed** (build t6j, `ef786498`): `BuildModelMaterial` parses GX00 as 14022B7C0 does (`i100-t6j-<path>-n1`, 14:06-14:17:
   FG_A_2012 casts 0 on the three paths, as the original; the other shares unchanged; the standstill pictures of the three paths
   unchanged against t6i, `evidence/I-0100/t6j-start-<path>.jpg`). Every DisableCasting mesh now leaves the cascades (none
   among the casters of the three paths: the materials listed below carry version 100, which has no DisableCasting).
4. **The main view kept** (the step review's M1; build t6l, `7c867d3c`): with +0xD0 set, 142130 makes a Decal material's class 8
   (or 0x10 when forward), drawn by the engine-6 G-buffer row and the forward row 0x29 with the decal state of 140850; the native
   engine-6 entry reuses the opaque six-output state, so a Decal map material would have been drawn opaque after the base
   surface. Decal materials on the three paths (`native_material_ex_param`, `i100-t6k-*-n1`, all GX00 version 100):
   `P[ChrCustom_Hair]_al` (the eyebrows), `M[ARSN]_e`, `_l_e`, `_al`, `_l_al`, `_tr`, `_e_tr`, `_em_Glow` (map overlays). So until
   the decal pass exists, a Decal material keeps its non-Decal class in the main view (as before this tranche) and its cascade
   rows are refused, as the cascade mask 0x107 refuses classes 8 and 0x10; the decal pass itself goes to the ROADMAP.
   CullingMode (+0xD8) and SlotNo (+0xD4) are written and not read natively.

## Tranche 6, step 5: controls, reviews, cost

1. **Three sizes** (build t6j, Settlement, ASUS, OBSERVED; the original only at 1920x1080, `i100-t5i-1080-o1`, tranche 5's
   measure): 1280x720 window (`i100-t6k-settlement-n1` against the same day's `i100-t6k-settlement-o1`, the tightened measure:
   every model within 0.04 of the original: 002095 0.14 against 0.10, the others equal), full screen 1920x1080
   (`i100-t6j-1080-n1`, 14:27) and a 1500x844 window that DS3 made 1440x810 (`i100-t6j-1500-n1`, 14:33): 002091 0.60-0.61 (0.60),
   002095 0.69-0.74 (0.61), o000250 0.38 (0.38), o310520 and o319002/4 0.02-0.03 (0.02), the player 0.86-0.87 (0.89), c1240
   0.16-0.18 (0.35, run to run), FG_A_2012 0 (0). Both full-screen and window runs end with the controller's exit 0xD3126612 and
   `closed` 1 after about 20,500 frames, as in tranches 5 and option A (pre-existing). No fault in any run of the tranche.
2. **Step review** (a fresh Opus at high, `main...20775ed6` plus t6j): no high. M1, the GX00 Decal moving map overlays to the
   engine-6 row drawn with the opaque state: answered by build t6l (step 4 item 4). Lows: L1 a 160-byte row buffer near its limit
   (256 now); L2 the character box's validity bounds are the pilot's (50 m, 5 m), not the original's (a counter of the
   fallbacks was added, then removed by the final review: it wrote no row; the Firelink NPC's box was valid after its first frame);
   L3 23C1E0's length is taken as the record's
   depth range (+0x64, 100 at m40 and m45 where the original's argument, the sun manager's +0x914 + 0x50 i, read 100): INFERRED,
   open; L4 the original's sun presence keyed by model pointer while the native merges by name: now by name.
3. **Cost** (measure mode, no diagnostic flag but test-no-ai, the Settlement path at 720, ASUS: `i100-t6-opt-main-n1` with
   `main`'s module against `i100-t6-opt-t6l-n1`, 15:25-15:31, OBSERVED): the shadow walk 0.052 ms a collect (main) against 0.066,
   the collect's sections 1 and 2 (the walks; the pass rows) 1.580 and 0.307 ms against 1.638 and 0.350: about 0.1 ms a collect
   more, within one run's noise; game steps p99 22.2 ms (main, two over 17 ms) against 17.0 (none). No gain claimed, no loss seen.
4. **The protocol**: the series `i100-t6j-*` announced for 14:12 started at 14:06 (the wait's time mistyped), a breach of the
   five-minute notice, said to Thomas; the size runs ended 4 minutes after their announced end.
5. **c1400 is not a divergence** (the final review's M1, checked by the pilot): the six original runs at Firelink give c1400
   0 drawn steps in `i100-t6a-` and `-t6e-firelink-o1` and 1,777 to 3,488 (13-14 sun windows) in `-t6b`, `-t6d`, `-t6f` and
   `-t6i-firelink-o1`, mostly in pass 7 (cascade 2), the native's 0.15-0.16 and cascade; the two runs without it also carry fewer
   `popin_reference` rows for it (a different run state, INFERRED). The comparisons above used `-t6a-` as the reference.
6. **Final review** (a fresh Opus at xhigh, `main...7c867d3c`, then the documentation commits): no high or medium code defect.
   H1, the gate not yet run on the final tree: run after the fixes below (the record in step 6). M1, c1400: item 5; the open item
   removed from STATE, ROADMAP, the index and K-0483, and the diagnostic made to chase it removed. M2, the sizes overstated in
   STATE, the index and K-0483: reworded (720 on three paths; the Settlement also at 1080 and in a window against tranche 5's
   measure of the original; 002095 above the original in every native run). Lows fixed: 23C1E0 pinned at start-up (L1);
   `no-caster-volume.flag` also keeps the 4-metre character box (L2); the box counter removed (L3); the answered diagnostics
   removed (`native_caster_row_inputs`, the materials' bytes in `reference_caster_rows`, `native_caster_entity_boxes`; L4); the
   length worded as INFERRED in the code and above (L5); K-0483 points to K-0453 (L7); GX00's Decal read from one data word as
   14022B7C0 does (nit). Open (L6): GX00's SlotNo (+0xD4) and CullingMode (+0xD8) are now written from data where they were 0;
   no DS3 reader of +0xD4 was searched and no run had combat or paint (test-no-ai). Build t6m.

## Tranche 6, step 6: the gate

**Gate** `i100-tr6` (2026-10-27 16:07-16:24, the ASUS 1920x1080@60, native tree of `8ea9ffc2`, build t6m's code; without the
memory leg: the branch allocates nothing new at a load (the GX00 item writes four fields of an existing material) and its
shadow-only admissions existed before tranche 5, D-0218): PASS. Period 16.668 ms; frames ending over 20.4 ms 2.5 a run (2 at
`i100-tr5`), over 15 ms 19 (18.5), display late 10 (7.5), game steps over 20.4 ms 3.5 (1.5); pictures 0.74 and 0.86 against the
reference (0.83 and 0.91 at `i100-tr5`); no fault. Every figure is under its threshold; the frame figures are a little above
tranche 5's two runs, within the gate's run-to-run spread (no regression claimed nor excluded on two runs). The fixes after the
final review had their own step review (a fresh Opus at high, `7c867d3c..8ea9ffc2`): nothing found.

## Tranche 7: the first admissions at a load, Archdragon Peak's path (plan)

Opened 2026-10-27 16:41 on branch `claude/infallible-lumiere-a99a8b` (STATE « Next action » 2; POPIN-STUDY §5 T3, T1), in this
order:

1. **What a first admission costs** (OBSERVED in `i97-t5k-admit-n1`, `crossing-observe.flag`): the slowest admission of every slow
   collect spends almost all its time in `AcquireNativeModelPrograms` (the split's phase 5, MeshPrograms: 2.6-8.8 of 2.7-9.4 ms in
   the Settlement turn, 53.5 of 55.6 ms at the load's first frame), with no new programme analysed during the turn (`analysed`
   272 from the load on, 8,000 analysis hits in the turn): the cost is building a layout's programme set (its recipes, the
   effect found by name, its plans) for every new material layout of a model DS3 loaded with the zone, at the model's first
   sight. The original resolves a material's shader group when it loads the model (its resource factory 82220, which the
   native's `BuildModelMaterial` / `BuildModelLayout` replace), not at its first draw. Step 1: the build's parts timed (behind
   `crossing-observe.flag`), one native run of the Settlement turn.
2. **The fix, in this order**: (a) the parts that repeat the same work for every layout made cheap where they are pure
   functions of the effect (optimise, do not stack); (b) if the turn still carries first-admission peaks, each layout's programme
   set prepared when DS3 loads it (D-0211: during the loading screen, off the engine thread, at most + 1 s a load, D-0213), as
   the original does at its load. ER: to read before (b) is designed (its material's shader binding at load, I-0071 ER-STUDY).
3. **Measured**: the turn's collects with 3 ms or more of admission and their fresh admissions (`crossing-observe.flag`), the
   load time (`Get-I97LoadTimes.ps1`), at 1280x720, 1920x1080 and an arbitrary window (WORKFLOW).
4. **Archdragon Peak** (D-0213, D-0174): Eta travelled to Archdragon Peak in its own copy of the Shebang save (`Run-AlphaCast
   -BonfirePages -TravelPress`), the copy kept hashed; a reconnaissance run, then its replayed path `archdragon.path.json` (a walk
   and the mouse turn), a study base outside the gate.

## Tranche 7, step 1: what a layout's programme set costs

1. **The measure** (build t7a, `44e279c1`: `native_programs_build`, behind `crossing-observe.flag`, the thread time of every
   `AcquireNativeModelPrograms` that builds a set, by part). Run `i100-t7a-settlement-n1` (2026-10-27 17:00-17:03, ASUS, the
   Settlement path with its camera turn, 1280x720 window, `crossing-observe;test-no-ai`, no fault, the save restored by hash),
   OBSERVED, pool-thread milliseconds summed over the workers:

   | Window | Sets built | Lookup | By name | Layout lock | Parameters | Numeric | Selection | Analyses | Plans |
   |---|---|---|---|---|---|---|---|---|---|
   | the load's first frames (to frame 1441) | 595 | 51.3 | 151.0 | 55.7 | 31.0 | 5.0 | 106.6 | 346.8 | 82.8 |
   | the camera turn (frames 8881-9121) | 653 | 2.0 | 9.3 | 26.5 | 21.3 | 4.4 | 95.8 | 84.6 | 64.6 |

   In the turn: 309 ms for 653 sets (0.47 ms a set, the 0.37 s of tranche 1 step 3), no programme analysed afresh (288 analyses
   in the run, all at the load): the analyses part is 653 x about 40 lookups of a kept analysis, each copying and hashing the
   programme's bytecode. The selection (`SelectModelPrograms`, a linear search of the effect's programme names with a string
   built per entry) and the analyses' lookups are 58 % of the turn's cost and give the same result for every layout of one
   effect, path, layout type, instancing and channel preference; the plans 21 %; the layout's lock is waiting (several
   instances of one model admitted together), not work.

## Tranche 7, step 2: the recipes kept per recipe set

1. **The change** (build t7b, `NativeRecipeSet` in `08_model_programmes.inc`): a layout's flat recipes (the programme selections,
   their interfaces and reads, the loose phantom and Velo pixel programmes) are kept per {the programme registry's generation,
   the effect's generation, the programme path, the layout's type, its instancing, the channel preference}; a layout whose key is
   kept takes them as they are. The interfaces and reads are now the kept analyses' own (shared pointers into
   `AnalyzeNativeProgram`'s entries), not copies per layout. The tessellated recipes (`native-tess.flag`) stay per material.
2. **Measured** (2026-10-27 17:30-17:52, ASUS, the Settlement path, `crossing-observe;test-no-ai`, no fault, every save restored by
   hash; t7a is `main` with step 1's instrument), OBSERVED:

   | Run | Window | Sets built in play | of them from a kept set | Their pool time in play | Collects with 3 ms+ of admission after the load | Longest | Turn frames over 17 / 25 / 34 ms | Turn's worst frame |
   |---|---|---|---|---|---|---|---|---|
   | `i100-t7a-settlement-n1` | 1280x720 | 679 | 0 | 325 ms | 25 | 25.0 ms | 52 / 5 / 2 | 39.3 ms |
   | `i100-t7a-settlement-n2` | 1280x720 | 683 | 0 | 308 ms | 32 | 24.3 ms | 29 / 6 / 1 | 38.0 ms |
   | `i100-t7b-settlement-n1` | 1280x720 | 679 | 663 | 111 ms | 13 | 9.0 ms | 43 / 1 / 0 | 28.7 ms |
   | `i100-t7b-settlement-n2` | 1280x720 | 679 | 663 | 118 ms | 12 | 7.7 ms | 44 / 1 / 0 | 27.4 ms |
   | `i100-t7b-1080-n1` | full screen 1920x1080 | 680 | 663 | 124 ms | 10 | 9.3 ms | 105 / 1 / 0 | 28.4 ms |
   | `i100-t7b-1500-n1` | window 1500x844 | 680 | 664 | 114 ms | 12 | 8.9 ms | 58 / 1 / 0 | 27.9 ms |

   The turn is the path's mouse segment (its motion marks, plus 2 s); the frame counts are the module's `native_frame_times`
   windows of 120 frames that end inside it. At the load the sets' pool time fell from 784-830 ms to 587-617 ms (475-482 of 595
   sets from a kept set). Not measured: t7a at the two other sizes (the 1080 and 1500 runs show the same counts as t7b's 720
   runs, not a before and after); the 1080 and 1500 runs end with the controller's exit 0xD3126612 after about 20,500 frames,
   as in tranches 5 and 6 (pre-existing, the game left running to the run's time).
3. **What remains in the turn** (`i100-t7b-settlement-n1/-n2`, play parts in pool milliseconds): the plans 66, the parameters
   15, the effect by name 9, the layout's lock 2-9, the selection 12 (the 16 sets not kept), the analyses 1: the plans are now
   60 % of a set's build. Next: the block plans kept the same way (build t7c), then the question of a preparation at the load.

## Tranche 7, step 3: the block plans kept; Archdragon Peak's path

1. **The change** (build t7c): `PlanNativeModelBlocks` makes the block plans from the interfaces, reads, effect and authored
   material alone, so they are kept per {recipe set key, authored material's generation} (`NativeBlockPlanKey`) and shared
   (`Programs::block_plans` a shared pointer, read through `PlanOf`); a set with tessellated recipes makes its own.
2. **Measured** (`i100-t7c-settlement-n1`/`-n2`, 2026-10-27 18:06-18:12, ASUS, 1280x720, the same path and flags, no fault,
   saves restored; reader `Get-I100Programmes.ps1`), OBSERVED: the sets' pool time in play 49.0 and 49.4 ms (t7b 111-118, t7a
   308-325), at the load 543-564 ms (t7a 784-830); collects with 3 ms or more of admission after the load 6 and 6, the longest
   4.2 and 4.6 ms (t7a 24-25); the turn's frames over 17 / 25 / 34 ms 54 / 1 / 0 and 63 / 1 / 0, worst 27.3 and 28.6 ms. The
   turn's frame over 25 ms is no longer an admission's (the longest admission 4.6 ms): the cadence class of I-0097 tranche 5.
   Each t7c run also has one walk frame of 57 ms (serials 5472 and 7958, collect 2.2-2.4 ms, prepare 1.4-1.6 ms, present
   5.7 ms, no texture admitted): not the admission; the same class is in `i100-t7b-1080-n1`, `-1500-n1` and `main`'s own
   measure run `i100-t7-opt-main-n1` (final review L2): it predates the caching.
3. **Archdragon Peak** (D-0213, D-0174): Eta travelled natively from Firelink to the Archdragon Peak bonfire, rested and saved by
   Quit Game (`i100-t7-archdragon-save2`, 17:18-17:22): `shebang-zones-eta-m32-archdragon-20261027-172201`, SHA-256
   B202E7934D05649FD22CDE718B878384D0D4F755A7E2709510EF90447FC6519D. The first attempt, in a Reference run, opened High Wall's
   travel tab with the same presses (`i100-t7-archdragon-save`; its copy renamed `i100-t7-misfire-eta-highwall-*`, not
   deleted). The reconnaissance `i100-t7-recon-archdragon-n1` (18:12-18:15, native t7c, no AI, pictures every 2 s in
   `evidence/I-0100/captures/`): the walk stays between the bonfire and the first wall, the two turns show the cliffs and the
   distant peaks, no enemy reached. Adopted as `scripts/replay/archdragon.path.json` (a study base outside the gate).
4. **What Archdragon Peak shows** (that run, OBSERVED; figures from the reader as corrected in step 7): 6,193 sets built over
   the run, about 2,310 of them at the load and 3,837 from the path's start (3,826 from a kept recipe set), in bursts of about
   573 every 600 frames, the walk's cycle: DS3 makes those layouts again as Eta walks back and forth (new layouts, so new sets:
   its own streaming, C4); in play the effect found by name was the largest part. Every collect has 8-12 fresh admissions and about 1,110 candidates whose reuse costs about 32 ms of pool
   time (4.3 ms wall): the zone's steady collect cost, not first sights (outside this tranche). Build t7d keeps the by-name
   answer per group and registry generation.

## Tranche 7, step 4: the by-name answer; the step review; Archdragon Peak before and after

1. **Measured** (2026-10-27 18:32-18:44, ASUS, 1280x720, `crossing-observe;test-no-ai`, no fault, saves restored; reader
   `Get-I100Programmes.ps1` as corrected in step 7: « play » from the path's start), OBSERVED, one run each:

   | Run | Sets in play / from a kept set | Pool time in play (whole run) | Longest admission in play | Walk frames over 25 / 34 ms | Turn frames over 17 / 25 / 34 ms | Turn's worst |
   |---|---|---|---|---|---|---|
   | `i100-t7d-settlement-n1` (t7d) | 684 / 667 | 49.5 ms (603) | 5.2 ms | 1 / 0 | 28 / 1 / 0 | 27.4 ms |
   | `i100-t7d-settlement-n2` (t7d) | 679 / 663 | 48.0 ms (627) | 4.3 ms | 1 / 0 | 58 / 1 / 0 | 29.6 ms |
   | `i100-t7a-archdragon-n1` (`main` + instrument) | 3,826 / 0 | 1,726 ms (3,731) | 15.4 ms | 10 / 5 | 20 / 2 / 1 | 55.0 ms |
   | `i100-t7d-archdragon-n1` (t7d) | 3,826 / 3,817 | 220 ms (1,282) | 6.5 ms | 3 / 0 | 2 / 0 / 0 | 24.3 ms |

   At Archdragon Peak t7d's by-name part was 437 ms (t7a 373): its key, the programme registry's generation, moves with every
   material DS3 streams there, so the kept answer was never taken. Build t7e keys it on the effects' own changes
   (`effects_changes`, under `effect_lock`).
2. **Step review** (a fresh Opus at high, D-0198, on `main...a8cd35d0` native): no high finding. M1, a recipe set made while a
   loose programme (the Velo pixel, a family's phantom) was absent was kept for the whole generation, where FindLoadedProgramme
   asks again after 250 ms (Native2202): such a set is now taken only for 250 ms. M2, two workers building one key at once could
   leave a layout with one worker's recipes and the other's block plans: the plans now live in the recipe set they were made
   from (by authored material), and a worker that lost the race takes the winner's set (t7e; since t7f a newly made set
   replaces any kept one and the layout keeps the set it made, step 5). L1, the by-name answer held its effect
   alive: now weakly. L2, the by-name witness row says whether its answer was kept. L3, the sets' key is now the shaders'
   admissions (`shader_registry_changes`, what FindLoadedProgramme depends on) instead of the whole registry's generation, which
   also keeps Archdragon's sets across its streamed materials. The eviction's brief pause under the lock (L3) is not changed.

## Tranche 7, step 5: the fixes measured, the optimisation pass

1. **Build t7e measured** (2026-10-27 19:03-19:15, ASUS, 1280x720, no fault, saves restored; the reader now leaves out the
   frame window that ends after the turn, where Archdragon's run closes the game: the Settlement figures above move by a few frames
   over 17 ms, none over 25 or 34; recomputed), OBSERVED: `i100-t7e-settlement-n1` play 77.6 ms (631 of 681 sets kept; t7d
   48-49.5 with 663-667), selection 36 ms (t7d 12): M1's 250 ms expiry made most sets again, because most families have no
   phantom programme, so nearly every set had an absent loose programme. `i100-t7e-archdragon-n1`: 1,170 ms, the by-name part
   still 371 ms: its rows show groups no effect owns (`GXFlver_SkinSSSIbl_Bloody`, a composed group) scanned again at every
   set, the cache keeping found answers only. Both answered in build t7f: a set keeps the names of its absent loose programmes and
   asks FindLoadedProgramme for them again at every take (that lookup's own 250 ms rule then applies, as before the cache); the
   by-name answer is kept when no effect owns the group.
2. **Optimisation pass** (measure mode, no diagnostic flag but `test-no-ai`, the Settlement path at 720, ASUS:
   `i100-t7-opt-main-n1` with build t7a, `main` plus the inert instrument, against `i100-t7-opt-t7e-n1`, 19:03-19:09, OBSERVED,
   means of the module's 120-frame windows from frame 1600 to 9600): collect 2.346 against 2.137 ms, prepare 1.674 against
   1.534, record 1.337 against 1.240; frames over 17 / 25 / 34 ms 91 / 10 / 3 against 155 / 11 / 5 (corrected by the final
   review M1: 126 / 6 / 2 left out two windows). The t7e run is not a clean comparison: Beta was reloaded in its walk (from
   frame 7781 no draw, 543 model storages retired and admitted again, `native_thread_census` of frame 7801), and those two
   windows hold that reload's slow frames. One run each: no gain claimed beyond the programme sets' own measure; the gate's
   measure runs judge the final build.

## Tranche 7, step 6: the final build's controls; the preparation at the load not needed

1. **Build t7g** (`0d5694c6`: step 5's fixes and the fixes' step review answered: a fresh Opus at high on `a8cd35d0..91ff783c`
   found no high or medium; its lows taken: the sets' map cleared when its old counts free nothing, one take inlined, the by-name
   row's count -1 when the answer was kept). Controls (2026-10-27 19:42-20:00, ASUS, `crossing-observe;test-no-ai`, no fault,
   saves restored), OBSERVED:

   | Run | Window | Sets built / from a kept set | Pool time | Longest admission | Walk over 25 / 34 ms | Turn over 17 / 25 / 34 ms | Turn's worst |
   |---|---|---|---|---|---|---|---|
   | `i100-t7g-settlement-n1` | 1280x720 | 679 / 678 | 62.9 ms | 3.8 ms | 2 / 1 | 25 / 1 / 0 | 28.2 ms |
   | `i100-t7g-1080-n1` | full screen 1920x1080 | 680 / 679 | 53.4 ms | 5.3 ms | 2 / 1 | 62 / 1 / 0 | 28.5 ms |
   | `i100-t7g-1500-n1` | window 1500x844 (DS3: 1440x810) | 680 / 679 | 54.4 ms | 3.9 ms | 1 / 0 | 34 / 1 / 0 | 29.5 ms |
   | `i100-t7g-archdragon-n1` | 1280x720 | 3,826 / 3,826 | 247 ms (whole run 791) | 6.0 ms | 7 / 1 | 23 / 2 / 1 | 48.5 ms |

   (Rows from the reader as corrected in step 7: « play » from the path's start, one run each.) The by-name part and the lookup
   fell to about 1 ms on the Settlement path; what remains of a take is asking again for the set's absent loose programmes
   (the « selection » part, 29-39 ms on the Settlement path: FindLoadedProgramme's own rescan of an absent name every 250 ms, as
   before the cache) and the material's own parameters (12 ms). Archdragon's turn frame of 48.5 ms (serial 7540: collect 3.1,
   prepare 3.8, record 2.9, present 9.0 ms, no pipeline created, no texture admitted) is not an admission's (its longest
   6.0 ms); t7d's turn had none and t7a's one of 55 ms: t7g's Archdragon turn (23 / 2 / 1) is not better than `main`'s (20 / 2 /
   1) on these single runs.
2. **Conclusion** (INFERRED from steps 1-6, one or two runs per build): the camera turn's first admissions at a load were the
   layouts' programme sets built at first sight; keeping their recipes, analyses, block plans and by-name answers removes the
   Settlement turn's admission peaks (the longest admission of a collect in play 24-25 ms to 3.8-5.3 ms; the turn's frames over
   25 ms 5-6 to 1, the « before » measured at 1280x720 only, the « after » at 1280x720, 1920x1080 and a 1500x844 window) and,
   at Archdragon Peak, the programme sets' time in play from 1.7 s to 0.22-0.25 s (3.7 s to 0.8-1.3 s over the whole run), its
   walk's frames over 25 ms from 10 to 3-7 (single runs; its turn not better). A preparation of every model of a zone during
   its loading screen (D-0211's « prepared ahead », tranche 1 step 3) is not built in this tranche, the pilot's decision
   (D-0223): what a first sight still costs in play is about 50-60 ms of pool time on the Settlement path, most of it asking
   again for absent loose programmes (a pre-existing rescan that a preparation would move to the load, not remove) and the
   material's own parameters; its peaks (under 5.3 ms a collect) no longer reach a frame over 25 ms. ER keeps the shader choice per material, pass and variant as persistent data beside the material
   (I-0071 ER-STUDY A, 141AC6CB0); the native now does the same per recipe set, shared by every material of one effect and
   layout type. The turn's one frame over 25 ms left is the cadence class of I-0097 tranche 5 (« Cadence, méthode DX12
   moderne », D-0219).
3. **Not done in this tranche**: Lothric Castle's path (Theta, D-0213); the load itself (C2, the world drawn incomplete under the
   loading page, POPIN-STUDY M2); Archdragon's steady collect (about 1,110 reused candidates, 4.3 ms of admission wall a
   collect); the gate (next).

## Tranche 7, step 7: the final review

1. **Final review** (a fresh Opus at xhigh, D-0198, on `main...ded7329f`, read-only): no high or medium code defect; the cached
   values equal what `main` computes on every path read (the key covers SelectModelPrograms, FindLoadedProgramme through the
   absent names and the shader count, AnalyzeNativeProgram; the plans' key is a unique generation; `effects_changes` at both
   mutation sites; tessellated recipes outside the sets). Its findings on the record, answered:
   - H1, the reader's fixed frame 1400 counted Archdragon's load as play in some runs and not in others (the « not explained »
     variation of the sets built, 3,847 against 6,147, was that boundary: every run builds about 6,193 sets): « play » now
     starts at the path's motion start (`Get-I100Programmes.ps1`), steps 3, 4 and 6 recomputed: Archdragon's programme sets in
     play 1,726 ms (`main`) against 220-247 ms, over the whole run 3,731 against 791-1,282 ms.
   - M1, the optimisation pass's t7e counts (step 5 item 2 corrected: 155 / 11 / 5, a reload in that run's walk).
   - M2, the summaries' wording (the « before » at 720 only, single runs, INFERRED, the preparation's reasoning): corrected in
     step 6, STATE, the index and ROADMAP; the decision recorded (D-0223).
   - M3, no run tested the caches' equivalence: `recipe-verify.flag` (build t7h) makes every taken recipe set again uncached and
     compares it (`native_recipe_set_mismatch`, `native_recipe_set_verified`); its run is the next step.
   - Lows: L1 a stale comment and step 4's race sentence; L2 step 3's « none in t7a's walks » (the 57 ms class is in `main`'s
     run too); L3 the sets and plans now released after their locks (their memory for a session: every recipe set and MTD pair
     met, not measured: the gate's memory leg and the night endurance read it); L4 the Velo name listed once (`std::find`).
     Not changed: L5 (FindLoadedProgramme's rescan of an absent name every 250 ms, pre-existing: a name index is a later
     optimisation), L6 (parts 0-2 also time calls that end without building: « about 0.08 ms a set » removed), L8 (the load
     time: not measured in this tranche).
2. **Thomas's eye** (2026-10-27 20:20-20:25, the OLED, `main`'s launcher module, tranche 6, borderless 2560x1440, OBSERVED by him,
   relayed): « je ne vois plus les ennemis apparaître et disparaître », « les ombres sont bonnes ». Two refusals of that session
   are in STATE « Open gaps » (a lost exclusive full screen; a model method at Archdragon Peak), kept in the planned order
   (Thomas, 20:09 and 20:16).
3. **The verification** (M3; `recipe-verify.flag`, ASUS, the Settlement and Archdragon paths, `crossing-observe;test-no-ai`, no
   fault, saves restored), OBSERVED: build t7h (`i100-t7h-verify-*`, 21:02-21:08) found the DptA recipe (21) differing in
   4,002 of the first 4,096 takes at Archdragon (16 rows on each path); its cause, read in AnalyzeNativeProgram: two admission
   workers analysing one new programme at once both kept their entry, so one programme had two interface addresses (the values
   alike, INFERRED: the same bytes analysed by the same decoder; `main` copied them by value, so it had the duplicates too,
   unseen). Build t7i (`bd43c480`) keeps the first entry for every caller: `i100-t7i-verify-settlement-n1` 0 mismatches over
   1,230 takes, `i100-t7i-verify-archdragon-n1` 0 over 4,724 (`native_recipe_set_verified` 4,096 / 0), 21:24-21:30. The block
   plans are not compared by this instrument (their key, the recipe set itself and a unique material generation, read by both
   reviews); the tessellated recipes (`native-tess.flag`, off by default) were not run.
4. **The gate without its memory leg** (Thomas, 2026-10-27 21:05, « Courte », on the pilot's proposal: the tranche keeps its
   caches for the session, their keys stable after the start, so tonight's night endurance on the merged `main`, 14 loads, reads
   the memory per visit instead).

## Tranche 7, step 8: the gate

**Gate** `i100-tr7` (2026-10-27 21:36-21:53, the ASUS 1920x1080@60, native tree of `bd43c480`, build t7i's code; without the
memory leg, Thomas's choice, step 7 item 4): PASS. Period 16.667 ms; frames ending over 20.4 ms 0 a run (2.5 at `i100-tr6`),
over 15 ms 18.5 (19), display late 8.5 (10), game steps over 20.4 ms 1 (3.5); pictures 0.65 and 0.66 against the reference (0.74
and 0.86 at `i100-tr6`; the gate's picture threshold met, the standstill pictures differ run to run); no fault.
**The fixes' step review** (a fresh Opus at high on `ded7329f..bd43c480`): no high or medium; select_flat equals the former
loop, the verify path writes only its own copy, the analysis re-scan keeps its predicate, the extract and merge are correct. Its
lows, not changed in this tranche (a native change would need the gate again; for the next tranche): L1 a set replaced by
`insert_or_assign` is still released under the lock (rare: a set remade after its absent programme was found, or two workers on
one key); L2 with both `recipe-verify.flag` and `crossing-observe.flag` the uncached copies add to the build parts' timing and
the analysis hits (never combine the two for timing); L3 a mismatch row cannot yet tell a set's error from a registry that
moved after the take (no mismatch was seen). The Test-Bosses launcher's comment now says `-Bosses` belongs to the .ps1 line.

## Thomas's evening play: three refusals (2026-10-27 20:06-20:42)

Read-only from his journals and crash reports (OBSERVED unless said), the next workstream after I-0100 (D-0224):

A lost exclusive full screen faults the native run (Thomas, 2026-10-27 20:06, `main`'s launcher module on the OLED at 2560x1440, OBSERVED in `%LOCALAPPDATA%/DS3-DX12/play-20261027-200537.jsonl` and its `crashes/` report: Print Screen's capture took the focus, a 722 ms frame, then the startup Present returned 887A0001 DXGI_ERROR_INVALID_CALL and the module refused, `native_render_worker_faulted`; the original survives it; the refused call's cause, INFERRED: a Present right after the chain left full screen, before it is fitted again); the borderless window (`-Borderless`) does not take the exclusive mode. A refusal at Archdragon Peak (Thomas, 2026-10-27 20:13, `main`'s launcher module, `play-20261027-201033.jsonl`, OBSERVED): `native_model_storage_method_not_owned`, caller 0x66755A: a map object's per-frame update (140666C60 → 14066B51C → 1406674FE) asks a model resource of the object by index (its container +0x1F0, method +0x10) and calls the returned object's method +0x38, which the native model prefixes (storage, material, layout tables) do not own; a path not met before (not at Anor Londo, not on the Archdragon path's bonfire area). A refusal at Iudex Gundyr's death (Thomas, 2026-10-27 20:42, the test launcher, `DS3-DX12-Test/test-20261027-203531.jsonl`, OBSERVED): `native_model_pose_nonfinite_current_rows` (05_selected_scene.inc:77, the public pose update 16000): DS3 published bone rows with non-finite values as the boss died (phase 2, a fireball); the original sends them to the GPU and goes on. The three are the next workstream after I-0100 (D-0224). 


## Tranche 7, step 9: after the merge

1. **Night endurance** `i100-tr7-night` (2026-10-27 22:35-23:03, the ASUS, `main` at `02d559ca` from the launcher's build, 30
   minutes, 16 loads; `Get-EnduranceSummary.ps1`), OBSERVED: no fault in play, private memory +6.5 MB a visit (threshold 40),
   frames over 25 ms flat per zone (0.36-0.69 per 1,000), the known exit heap fault 0xC0000374 at the close (I-0099). It stands
   for the gate's memory leg Thomas chose to skip (step 7 item 4). Thomas saw one Load Game portrait missing during it (the Dreg
   Heap character; STATE « Open gaps »).
2. **The gate's calibration on `main`** (D-0207), not done: `i100-tr7-cal` (22:14-22:34) and `i100-tr7-cal2` (23:10-23:30) both
   measured display-late counts of 61 to 134 in two of their four runs (13 to 19 in the others; 8.5 at the gate `i100-tr7` of the
   same native tree at 21:36-21:53), so the script kept every threshold. The first overlapped a short Ghidra scan of the study
   thread; the second ran with the OLED on beside the ASUS and Thomas at the PC (INFERRED: the desktop's activity on the other
   screen, not the module: the frames ending over 20.4 ms were 1-1.5 a run). To redo on a quiet PC, the OLED off.

## Tranche 8: the load (plan)

Opened 2026-10-27 23:31 on branch `claude/kind-mccarthy-f8cfb1` (STATE « Next action » 2; POPIN-STUDY §5 T3, M2, §6 criteria 1
and 4), in this order:

1. **What the existing journals say** (no run): in `i100-t0c-settlement-n1` (the Continue) and `i100-ls-a1` (a launch, then a
   death's reload), the loading page's window (KnowledgeLoadingScreen, vtable 28EF918) closes, then the first world frame has
   84 content draws with the page's 37 UI draws still submitted, and the next one about 1,300-1,560; at the Continue 7 models
   are drawn on the first world frame and 552 on the next (`popin_native`), and DS3 creates 248 more textures after the window
   closed (OBSERVED). Whether any of this is visible (the page still opaque on that frame, the meshes whose textures arrive after
   it) is not measured.
2. **M2, the instrument** (behind `popin-observe.flag`, both renderers): one row per frame from the loading page's start to a few
   seconds after its window closes: the page's state, the main view's rows prepared, drawn and refused for readiness (textures,
   constants), the textures DS3 created since the previous row; the original's emitted entities on the same frames.
3. **Measured in one announced slot** (D-0225): the Settlement path (Beta) with its enemies' AI on, so the Continue and the
   deaths' reloads (as `i97-t5c-settlement-1`/`-2`), both renderers, `Run-I100PopinSeries.ps1` with screen pictures at 10 a
   second (`Capture-ScreenSeries.ps1 -Scale`), read by `Get-I100Load.ps1`; the original-probe mode cannot take
   `test-death.flag`, so the deaths are the path's own; then the fix, if one is needed, with its controls at 1280x720,
   1920x1080 and an arbitrary window, the load times (`Get-I97LoadTimes.ps1`, D-0213's + 1 s a load at most).
4. **The gate's calibration on `main`** (D-0207; tranche 7 step 9: two attempts disturbed): `i100-tr7-cal3` (2026-10-27
   23:45-00:03, the ASUS, the same native tree `94e5166d15d5`, Thomas away), not applied (OBSERVED): its third measure run
   failed its launch check (the presenter's rate not yet journalled at the settle, `launch-check.json` « presenter_hz null »), and
   the three others counted 23, 25.9 and 24.1 display-late frames (8.5 a run at the gate `i100-tr7`, 21:36-21:53); frames ending
   over 20.4 ms 1 a run, period 16.667 ms. Why the display-late count is higher since 22:14 on the same tree is not known (the
   OLED was on, Thomas away; no other process of the project ran). To try again later in the night.

## Tranche 8, step 1: the load measured (M2)

Build t8a (`6bb0769d`: `popin_load_frame`, `popin_page_close`, the original's texture creations counted at 2B260/AC110), series
`i100-t8a` (2026-10-28 00:22-00:35, the ASUS, the Settlement path with the enemies' AI, `popin-observe`; pictures at 10 a second
reduced to half size, `evidence/I-0100/captures/i100-t8a-*`), read with `Get-I100Load.ps1`, OBSERVED:

1. **Native (`i100-t8a-n1`)**: three loads read (the Continue, two deaths' reloads; a fourth close is the quit to the title).
   After the loading page's window is asked to close, no main-view row is refused for a texture not resident in the rows read
   (`missing_textures` 0, the mask of missing constants 0, `not_ready` 0 after the close). The first world frame draws 7 models
   (24 draws: the player), then the whole world: 576 models and 1,479 draws 212 ms after the close at the Continue, 36 ms after
   it at the deaths. At the Continue the frame between the two lasts 177.9 ms (journal line 30072, serial 1463: collect 79.9 ms,
   prepare 83.8 ms, record 93.1 ms, 29 pipelines created, 237 programmes analysed: the zone's first-sight pipelines and
   programmes). DS3 creates 269-283 more textures after the close (within 10 s, 5.9 s for the last load).
2. **Original (`i100-t8a-o1`)**: the first game step after the close emits the whole world (1,741 emits at the Continue, 2 ms
   after it), and DS3 creates 267-269 textures after the close: the late textures are DS3's own, in both renderers.
3. **Pictures** (`evidence/I-0100/t8a-continue-n1.jpg` and `-o1.jpg`, 16 pictures from 0.2 s before the close; n1
   `cap-00441`-`cap-00456`): in both renderers the loading card stays fully opaque for about 0.4 s after the close (n1 `cap-00443`
   +11 ms, `cap-00444` +124 ms, `cap-00445` +208 ms), then fades to black and the world fades in (n1 `cap-00449` about +600 ms,
   `cap-00452` +910 ms). The native's player-only frame and its 178 ms frame fall under the opaque card. At the deaths the card is
   opaque too after the close (n1 `cap-01397`, +177 ms). No picture shows the world grey or incomplete under the page or after
   it.
4. **Conclusion** (INFERRED, observed on three loads of one native run and one original run, one path): the « world drawn
   incomplete under the loading page » of POPIN-STUDY 2.3 is no longer visible natively on this path: its grey was the pipeline
   library (tranche 1, C5), its 84-object frame is the player-only first frame, drawn under DS3's opaque card; no texture
   readiness refusal follows the page here (§6 criterion 1 observed on these loads only; criterion 4 asks for 10 deaths and 10
   bonfire loads per zone and the load time, not measured). Not measured: the other zones' loads and bonfire travels, the Load
   Game portraits. What remains at the Continue is the 178 ms first-sight frame, under the card.
5. The series' remaining runs (`-n2`, `-o2`) ran after Thomas's « full production » (2026-10-28 00:28) and were not read; the
   gate's calibration is no longer tried (Thomas, 00:27).
6. **Final review** (a fresh Opus at xhigh on `main...3b6cb548`, D-0198; it also stands for the step review of `6bb0769d`): no
   high finding; with the flag off nothing changes, with it on the hooks pass through (AC110 and 2B260 called as the native's
   own admissions are). Answered: M1 five native files had been turned to CRLF by the pilot's script, back to LF; M2 STATE
   rewritten for D-0227 and the closing; M3 D-0181 (the side-by-side at every witness zone before a visual workstream's merge)
   kept until Thomas says whether D-0227 covers it, I-0100 closed with T6 owed; M4 items 1, 3 and 4 above corrected (the frames
   fall under the opaque card, not in the black; the 178 ms frame is first-sight pipelines and programmes, not the collect
   alone), the sheets kept; L1 the claims narrowed; L2 the dead « empty scene » trigger removed, the reader bounded at 10 s;
   L3 the field renamed `missing_constant_mask`; L4 a lock on PopinLoadFrame's statics; L8 the other D-0227 leftovers (LOG
   summaries, agents, WORKFLOW, ROADMAP, Stop-Guard's comment, live-run). Not changed: L5 (in plain Reference the « textures »
   field reads 0, the counting hooks being in the original-probe mode only), L6 (duplicate counting, behind the flag), L9
   (status set to complete in the merge commit itself).

## Closing (D-0227)

Thomas's « full production » (2026-10-28 00:28, D-0227) ends the measurement tranches: I-0100 closes after tranche 8 with
POPIN-STUDY §6 met in part (criterion 1 observed on three loads of one run at the Settlement, Thomas's eye on 2026-10-27: « je ne
vois plus les ennemis apparaître et disparaître »), not at every witness zone (criteria 2 to 6 not measured everywhere, T6
not run: D-0181's side-by-side owed unless Thomas says D-0227 covers it). Left in the ROADMAP: Lothric Castle's path, the side-by-side at every witness zone, the distant characters against the
original, the Load Game portraits missing now and then, the title's blue shadow, GX00's +0xD4 and +0xD8 readers, 23C1E0's
length. Tranche 8's native change is diagnostics only (behind `popin-observe.flag`); merged without a gate (D-0227), its final
review and the start check after the merge below.
