<!-- Archive copy of `.project/ROADMAP.md` as it stood on main before the documentation diet of 2026-11-15 (D-0291, Thomas: « Je valide d'avance »). Text verbatim; only relative links are rewritten to resolve from this folder. Never an active instruction: the live file is `.project/ROADMAP.md`. -->

# Roadmap: everything left, phase by phase, workstream by workstream

The general plan Thomas asked for and accepted on 2026-11-12 (D-0273, D-0274): five phases, twenty-one numbered workstreams,
in the advised order. It replaces the capability roadmap and its waves; the earlier text (the history of the orders, what
wave 2 has done, the performance narrative) is in tag `archive/pre-roadmap-2026-11-12` ([ARCHIVE.json](../../ARCHIVE.json)).
Current position and next action belong to [STATE](../../STATE.md); the programme contract to the [native
mandate](../../NATIVE-D3D12-RESET-MANDATE.md); the method to [WORKFLOW](../../WORKFLOW.md); the RT/PT design to
[RT-PT-PLAN](../../RT-PT-PLAN.md). [FINISH-LINE](../../FINISH-LINE.md) is the history of D-0228's list. Sizes (S, M, L, XL) are the
plan's estimates, not measures. « (plan) » marks a workstream or a step the plan added and Thomas accepted with it, as
opposed to one an earlier record already held.

## Capabilities

- **C1**, reproducible analysis baseline: acquired 2026-09-19 (DS3 1.15.2.0, K-0001/K-0002; [decompiled/README](../../../decompiled/README.md)).
- **C2**, the original D3D11 renderer's map: a read-only reference, recovered only for a named native decision.
- **C3**, the native D3D12 backend, « Dark Souls III: Rekindled »: **current**. Phases 0 to 2 below. First milestone accepted
  with reserves on 2026-10-19 (D-0194). Final acceptance: the complete game and its DLC rendered and presented with the
  original D3D11 renderer unavailable.
- **C5**, the release: phase 3 below.

## Rules that govern every workstream

- No D3D11 bridge, wrapper, translation or fallback (D-0096); Elden Ring's architecture, improved, without its known defects
  (D-0195); families and engine mechanisms, never one asset (D-0150, D-0161); resolution and presentation are inputs
  (D-0151); every supported language (D-0153); existing native components reused (D-0154).
- The lighting is traced: no raster lighting, no lighting cheat, no fallback without DXR (D-0235, D-0237, D-0251, D-0262);
  judged by its truth, not by the original's pixels (D-0249); the open-gaps work corrects only what stays visible with the
  rays (D-0250).
- Full production: no gate before a merge, a one-minute start check after it, the full control every three days (D-0227,
  D-0229, D-0230). Thomas's eye is the judge of a look (D-0268).

## The phases

| Phase | What it delivers | Workstreams | Pilots |
|---|---|---|---|
| **0. Close what is open** | R12 finished, I-0132 and I-0113 closed, the full control read | 1 to 3 | R, E, the control's thread |
| **1. The finish line (D-0228)** | The whole game natively with no fault and no missing effect, checked zone by zone; a first installer | 4 to 10 | R, E, robustness, the game's tour |
| **2. The modern picture** | YEBIS replaced, HDR10, physical light sources, traced volumetric fog, remade textures | 11 to 13 | Fable 5.2 high (12; 11's tranches 1 to 6, done), Opus (11's tranches 7 and 8, D-0281), Opus and Sonnet (13) |
| **3. The release (C5)** | Resolutions, languages, controls, optimisation, final review, publication, `rekindled.dll`; NVIDIA cards only (D-0274) | 14 to 18 | Opus |
| **4. Game additions** | ER-like animations and gameplay, ER's effects, game options, wishes | 19 to 21 | Fable to confirm (19) |

Phases 0 and 1 follow each other. Phase 2 starts with its study during phase 1 (read-only, no PC). Phase 3 waits for phase 2
for the optimisation and the final review; its compatibility probes come in phase 1 (workstream 10). Phase 4 holds studies
only before the release, unless Thomas decides otherwise. **The public release comes after phase 2; a private installer
from phase 1** (Thomas, D-0273).

## Phase 0: close what is open

### 1. The full control of 2026-11-12 (D-0230)

- Steps: read the measure's result (ended 04:53); a regression becomes the concerned pilot's next workstream; the gate's
  reference retaken with the rays' lighting and its thresholds reviewed (the old reference predates D-0253: the gate failed
  on the standstill pictures alone on 2026-11-09 and again against it on 2026-11-12; the control's thread reports a new
  reference taken that night, its calibration passing: its record is the control's result); the memory leg read again.
- Done 2026-11-12 (I-0132 RESULT « The full control of 2026-11-12 »): no fault, no regression by the gate's thresholds (frames over
  2.04 P up but within); the memory leg 4.5 MB a visit over 8 visits; the reference retaken (darker than 2026-11-09's, cause unknown),
  approved by Thomas at 05:35.
- PC: the whole PC for a measure. Pilot: the control's thread (Opus), numbers by `measure-tools`. Size S.

### 2. Finish R12 and close I-0132 (pilot R)

- Steps:
  1. Title screen R2: merged `ea7d22e0` (2026-11-12: the embers, the tinted and burnt logo, the ring and sword, the title kept and blurred
     under a box; I-0132 RESULT « R12e »); its remainder (RESULT « R12e », « The remainder »): the final review's Lows, the bonfires'
     crackle at the title (the game's s444444441, INFERRED: Thomas's ear), New Game's opening film covering the title; seen: French,
     1920x1080, 16:10 (1680x1050), the Options page from the title, the return to the title from play; the crackle validated by
     Thomas's ear (2026-11-12 18:33); the title page's close key DS3's own (the original's too). Done.
  2. The shader compilation screen as a full page before the title, **shown only when a compilation really happens**
     (D-0273), its list holding what still compiles at a first frame (the overlay's shader; the TAA, sharpening and NIS
     stages to check). Never run: a cold driver cache with SER (about 24 s).
  3. R12f's open points: seen 2026-11-12 (I-0132 RESULT « ROADMAP workstream 2, steps 3 and 4 »): frame generation switched in
     exclusive full screen, a later launch reading the page's file, the return to 60 after an unlocked start (applied; its cadence
     not judged at 35 frames/s), Esc on the title's page (the original's too); 2026-11-13 (RESULT « The measures left »): the
     tearing-capable chain in exclusive full screen with V-sync on (every present synced, no tearing flag), the 60 frames/s play
     through the installed corrections the original's logic within the mechanics rows' resolution (under a compile's
     load). Done.
  4. R12g's open points: the mouse on the scrolled page, the French help lines, the vignette at Firelink (corners 18 % darker):
     seen 2026-11-12. **The GI quality row: measured in motion 2026-11-13, no gain resolved over the turn's common range, the row removed**
     (D-0273; RESULT « The measures left »; Thomas: « ça me va »), the GI's paths a test tool. Frame generation stays off under the
     in-game menus (D-0273). Done.
  5. Chromatic aberration: read the original's tone-map programme before any settings row (D-0266 names DS3's; none is added
     that YEBIS does not have). Read 2026-11-12 (I-0138 STUDY §2.3): never drawn in the world (its enable 0 in every world file,
     none in the original's capture at the High Wall), enabled in three cutscene files only. Thomas (D-0276): reproduced
     there; the row comes with the Rekindled colorimetry's own aberration (workstream 11, tranche 7). Done for this workstream.
  6. The cloth's own velocity: merged `5274abeb` (I-0134, complete). Done.
- Waits for Thomas: his eye on R11c's sheets and on the embers.
- Pilot: R, Opus 5.5 high. Size M. Closed when I-0132 is `complete`, its leftovers carried by workstreams 7 and 8. Steps 1 and 3 to 6
  done (2026-11-13); step 2 is the second R12 pilot's (R12h), I-0132 closes with it.

### 3. Open gaps: tranche 41, then close I-0113 (pilot E)

- Steps: tranche 41 (three Firelink runs walking out of the west entrance right after a warp; the view from inside the
  shrine; gap 23's films with the `no-taa` control; the reactive weighting at large coverage; the disk rotation); the final
  table of the 27 gaps; what stays open goes to workstream 5.
- Done 2026-11-12, but for the disk rotation (stopped again by a running compiler; STATE « Next action »): tranche 41 merged
  `e305469c`; I-0113 complete, the final table of the 27 gaps at the head of its
  [RESULT](../../../investigations/I-0113-open-gaps/RESULT.md): validated by Thomas: 2, 5, 19, 20, 21 (the hair); fixed, never
  validated by him: 1, 4, 9, 12, 14, 22, 25, 27, with 26's camera turn and 11's hair; superseded (fixed under the raster
  lighting, removed since: what shows now is the rays'): 6, 8; not a native defect or not reproduced: 3, 13, 15, 26 (the
  frozen inputs); closed by decision: 18 (RT reflections), 24 (walls removed), 11 for Chameleon (left as the original);
  dropped: 7 (a small measured brightness gap of FINISH-LINE's abandoned list, its cause never found; no look under the
  rays was taken); open, to workstream 5: 16, 17, 23. Gap 10 never existed.
- (plan) One session of Thomas in play on the « fixed, never validated » gaps: still to hold, its list after the RESULT's table.
- Pilot: E, Opus 5.5 high. Size S.

## Phase 1: the finish line (D-0228)

Workstreams 4 to 8 are independent and run at the same time, one game launched at a time.

### 4. Robustness: « skip the object and go on » everywhere

- Steps:
  1. The interface: 486 of 1,316 UI selectors have no programme and an unsupported case ends in `std::terminate`: the draw
     skipped and a journal row instead.
  2. The `Unsupported*` stubs that refuse; I-0101's unsorted stops (asset loaders, movies, cloth, the walk's bounds,
     degraded filters). Done in I-0137 tranche 6 (RESULT « Tranche 6 »): none met since 2026-11-06 (a census of 4,693 journals); a map
     piece's runaway child list in the main walk and a Bink frame out of contract skipped and counted; the stubs (the module's
     interfaces), the loaders, the cloth, the movie catches, the filters and an unreadable MTD (a skip would be a stand-in) kept with
     their reasons.
  3. I-0120's priority 4 (the skip rule, the stale water Fresnel, the frozen lighting set, the refusal at a resize, the black
     stand-in for a movie), then priority 5 (go-ahead: D-0273).
  4. **A1: the controller accepts only the default Steam path**: blocks any release, fixed before the installer.
  5. The exit faults (I-0137 tranche 4): the `FUN_140E602F0` fault with a menu open and the « Select profile to delete » list are
     one fault, the module's (a stopped Close now keeps its thread, witnessed); the character creation's close (DS3's own,
     two guards in turn left the next reader of the released data faulting; the RCA, I-0137 tranche 8: DS3's quit request releases the
     c0000 resources while the creation's menu models keep updating, so three readers are gated on the quit; two clean witnesses); `DarkSoulsIII.exe+0xF14C52`: I-0124's path is DS3's own and fixed natively; the other path, through
     the thread loop 141B5F770 (I-0113's g24i), is DS3's online RPC thread (`FdpSv::RPCSystemImpl`), stopped natively at the quit
     (I-0137 tranche 6; its stop « signalled » at the quits read since).
- **Closed 2026-11-13** with I-0137 (tranches 1 to 8, RESULT « Closing »): step 1's left items (the selector without programmes,
  INFERRED unreached; the refused packet; the filter pass without programmes; the instanced layouts, the text layer kinds, the HAL's
  methods), none met since 2026-10-20, carried to workstream 16.
- PC: start checks only. Pilot: a « robustness » thread, Opus 5.5 high, mechanical work to Sonnet. Size M.

### 5. The effect and draw families not yet native (after I-0113; a new, bounded investigation)

- Only what shows with the rays (D-0250). Steps, by family, from
  [STUDY-NOT-YET-NATIVE](../../../investigations/I-0113-open-gaps/STUDY-NOT-YET-NATIVE.md):
  1. Effect passes 0x20 and 0x21 recorded and never drawn (0x21 drawn since I-0140 tranche 2; 0x20 not reached); the skipped renderer classes 10003 (light shafts) and 11000 (spot
     effect lights); the GPU particles' zero SH.
  2. Decals: the characters' blood and damage, the GX00 class (eyebrows, overlays), the decal pass (FINISH-LINE line 7).
  3. Single-model NPC phantoms; the burn MTD parameters.
  4. Gaps 16, 17 and 23, the mist's lower contrast, the menus' portrait cases.
  5. A systematic audit of the families never looked at (FINISH-LINE: spells, infusions and status effects « uncertain »):
     every sorcery, pyromancy, miracle, infusion, buff, status and projectile cast once natively beside the original.
- Not here: YEBIS's refusals (cutscenes, exposure, glare, depth of field): YEBIS goes in workstream 11.
- Pilot: E, Opus 5.5 high; `ghidra-research` for targeted decompilation. Size L.

### 6. The tour of the whole game (plan)

- Answers FINISH-LINE's « not validated »: nobody plays the whole game natively; the credits were seen by a forced flag only.
- Steps: a closed list (every zone and bonfire, every boss with its entry, phases and death, the events, the cutscenes, the
  NPCs and merchants, every menu, the endings and credits, the arrivals in both DLC); each row visited through the test
  launcher (warp, flags) with three questions: no fault, no skipped family in the journal, the picture judged beside the
  original; each defect goes to workstream 4, 5 or 7.
- It is today's form of the automatic native-against-original comparison on a replayed path and of the review by bug class
  (approved 2026-10-26, never done).
- Limit: a tour by warp is not a real playthrough; Thomas's own stays the final judge.
- PC: the largest consumer of runs (ASUS, at night). Pilot: its own thread, Opus 5.5 high, sheets and journals by Sonnet. Size L.

### 7. RT/PT leftovers after R12 (pilot R)

- Steps, in this order:
  1. The DLC arrivals: Ariandel holds 334 frames black, the Ringed City times out; at Ariandel the game selects 700 objects
     where the native admits 7 (cause not established). Not revisited: the Dreg Heap, the High Wall's Vordt bonfire and the
     Pit of Hollows under the new arrival rule.
  2. FINISH-LINE line 8 ter (D-0255): the change-driven collect and rays' walk, not started.
  3. Families without a material in the rays (I-0119, EmiGlow, the skinned family `o000250`).
  4. Never looked at in the zones' tour: Ray Reconstruction's local-light noise (the ReSTIR DI condition), the lit cumulus,
     a size off 16:9, NRD 0.8 darker.
  5. Frame generation: screen-mode changes with it on (I-0131), its value on the OLED.
  6. R11b's optimisation leftovers; the interior PT cost never measured; SER measured on one GPU, without a unit test.
  7. The player lit warmer and darker natively (a red-orange body, dark trousers) where the original keeps its cold grey
     ambient: seen by the game tour in the cutscenes 31000000, 31000010 and 33000000, the same before and after them, and
     out of cutscenes (Halfway Fortress, Iudex Gundyr), so not the Remo ModelDrawParam act; its cause, the player's RT/PT
     lighting (D-0262), is the E pilot's reading only, against I-0113 tranche 34's measure of the same orange skin in the
     character creation with and without the rays (the raster's); `no-rt-scene` gone (R11e), a G-buffer witness instead
     (`i140-e11`): the trousers' material colour neutral pale, their lit HDR red-dominant, so the lighting's (I-0140 RESULT
     « The tour's cutscene defects and the player's red-orange, as read before tranche 5 », 2026-11-14; one bonfire frame, the skin's SSS not separated). With it, the cutscenes' kind-11 lighting rules left INFERRED by I-0138 part 4b (the MapOffset
     rotation, the Snow prefix; I-0138 RESULT « Parts 4b and 4c: what they leave »).
     And the tour's cutscene defects I-0140 tranche 5 read (`i140-e12`, causes INFERRED): 31000010's valley and wyverns and
     32000010's dragon are drawn (the raster and the rays agree, their G-buffer textured); the valley and the dragon lie in the one
     traced sun's shadow, the wyverns, lit, are lost in the fog, where the original's draw groups carry their own light sets (the
     valley's `[131000000, 300]`, the scene's `[231000010, 110]`). I-0140 tranche 7 made the traced sun the original's shadow
     source (the view's ShadowParam, its Shadow Light Id; a cutscene's own file bound with the map's rotation, `i140-e16` to
     `i140-e18`); for R, [I-0141](../../../investigations/I-0141-cutscene-lighting/RESULT.md): tranche 1 (where the ShadowParam names no light
     set, as in 32000010 and 39000020's Siegward, light 0 of the cut's set when it is on, and a cutscene's own lights 1 to 3
     kept: Thomas's provisional choice, D-0279); tranche 2's first observation (2026-11-14): 32000010 compared in the zone's
     true state (the King alive again, the bell rung) is at the original's level; in the original the cutscene act
     overrides the map's light and fog records by the cutscene file's during a scene, and natively too, but into engine
     banks the native renderer does not read; tranche 3 (2026-11-15): those overrides given to `Banks`, witnessed in the ceremony
     scene (the map's close pieces in the ceremony's cool light, the overrides withdrawn with the scene); left: the last cut's veil and the haze in play in Archdragon
     Peak's cloud-sea state, the fade of a zone's files, the valley's per-pixel set 300, the settings the native never
     reads (I-0140's STUDY-UNREAD-SETTINGS; Thomas, 2026-11-14 22:30: « Corrigez les quand même »). The
     ending's body (41000000) brighter than the original's (YAVG 194 against 165): its material neutral, its lit HDR red-dominant,
     the hand's flame (a traced local light) over-lighting it.
- Emissive surfaces as light sources (lava, crystals) belong to workstream 12.
- Pilot: R, Opus 5.5 high. Size L. After workstream 2.

### 8. Cadence and the unlocked frame rate: the leftovers

- Steps ([I-0114](../../../investigations/I-0114-unlocked-framerate/RESULT.md) left them to « a cadence workstream »): the follow
  camera's drift at 240; the witnesses never taken (stairs, lock-on, ledges, ramps, the jump's takeoff under 60, the 30
  frames/s events); UNLOCK-READY §5's physics, effects, post and interface rows; the OLED's 280 Hz clock never paced, Reflex's
  latency never measured; the 60 frames/s path through the installed corrections compared with the original's; gap 27 (k
  held at 1 under 60: an exception to D-0244's « one logic at every rate », recorded as such by D-0273 and looked at again
  here).
- PC: the OLED on and Thomas present, measure runs (the whole PC). Pilot: Opus 5.5 high, numbers by `measure-tools`. Size M.

### 9. I-0127, the effects' batching: closed without step C

- Thomas, D-0273: step C (ExecuteIndirect) is not done; the measured stages cost 0.13 to 0.18 ms, under the study's « nothing
  to win » line. Closed (I-0113 tranche 41's checkpoint, 2026-11-12).

### 10. The first installer and the compatibility probes (D-0267)

- Steps: the production build without development hooks, flags or test launchers; the installer (the renderer and the
  settings page only, over the player's own game, the original executable required, no texture, game fixes as options); **no
  NVIDIA DLL is distributed** (Thomas, D-0274): the player supplies them, so this workstream designs how the module starts
  and runs without them and how the player adds them, and lists what the module itself compiles in of NVIDIA's (NRD, SHaRC,
  RTXDI, D-0252) for workstream 18's licence review; the Agility SDK's redistribution; (plan) the probes on Thomas's PC: a
  run under WARP, GPU-based validation, a video-memory budget limited to 4 then 6 GB.
- After workstream 2 (D-0267) and workstream 4's A1. Pilot: Opus 5.5 high. Size M.

## Phase 2: the modern picture

Thomas's order (D-0264, [RT-PT-PLAN](../../RT-PT-PLAN.md)): the post-processing chain first, the physical sources after it.

### 11. The modern post-processing chain in YEBIS's place (D-0262)

- **Its study and its plan** ([I-0138](../../../investigations/I-0138-modern-post-chain/RESULT.md), step 1, done 2026-11-12, read-only): the
  [STUDY](../../../investigations/I-0138-modern-post-chain/STUDY.md) (what YEBIS does in DS3 by area and in the cutscenes, what Elden Ring does instead, the native
  chain as it stands), its annexes ([CENSUS-WORLD](../../../investigations/I-0138-modern-post-chain/CENSUS-WORLD.md),
  [CENSUS-CUTSCENES](../../../investigations/I-0138-modern-post-chain/CENSUS-CUTSCENES.md), [ER-POST-CHAIN](../../../investigations/I-0138-modern-post-chain/ER-POST-CHAIN.md),
  [ER-TONEMAP-HDR](../../../investigations/I-0138-modern-post-chain/ER-TONEMAP-HDR.md), [ER-VALUES](../../../investigations/I-0138-modern-post-chain/ER-VALUES.md)) and the
  [PLAN](../../../investigations/I-0138-modern-post-chain/PLAN.md) Thomas answered (D-0276). Every tranche below starts from them.
- The chain (PLAN §4), in Elden Ring's order with DS3's values: DLSS or the TAA, depth of field, motion blur, the emissive
  glow, the exposure, the bloom with the light shafts, one tone-map pass (curve, colour matrix, vignette, 3D LUT), the
  output transform, the UI. Two colorimetries: the original's, remade (D-0178, D-0273), and « Rekindled ».
- Tranches (PLAN §5), each merged alone with before and after sheets for Thomas's eye:
  1. The base and the original colorimetry: a pass of our own in place of YEBIS's tone map and grading programmes,
     compared bit for bit with the original's captured draws; nothing changes on screen
     ([TRANCHE-1-READY](../../../investigations/I-0138-modern-post-chain/TRANCHE-1-READY.md)). Size M. Built 2026-11-13 (I-0138
     RESULT « Tranche 1 »): our programmes and parameter block, in the unit tests 0 differing bytes against the original's
     captured draws at m37, m31, m40 and m30; two draws kept where a LUT is on (the card's 8-bit write is part of the picture;
     one draw with HDR10); the old path behind `post-tone-old.flag` until tranche 8. Merged `cf0af0f3`; its live comparison
     (both paths drawn from one frame's inputs, `post-tone-verify.flag`): 96 witness frames of 96 equal at four areas and
     three sizes, with the TAA and DLSS. Done.
  2. The bloom, with the light shafts rewritten inside it (no bridge to the old pass, D-0276). Size M. Its sheet:
     [TRANCHE-2-READY](../../../investigations/I-0138-modern-post-chain/TRANCHE-2-READY.md) (remade exactly with our programmes, as
     tranche 1; the plan's first form, a new pyramid judged on sheets, kept as its §6). Built 2026-11-13 (I-0138 RESULT
     « Tranche 2 »): the glare family's programmes of our own (`post_glare.hlsl`), in the unit tests 0 differing texels
     against the original's captured draws (228 surfaces at 9 zones) and equal to the library's at four other sizes; a
     defect of `main` fixed with it (no bloom, shafts, depth of field or motion blur from about 1,562 wide in the
     eight-level areas without a luminance gamma: an unpinned technique); the old path behind `post-glare-old.flag`, the
     witness check `post-glare-verify.flag`. Merged `a651528c`; its live comparison: 96 witness pairs of 96 equal at
     four areas and three sizes, with the TAA and DLSS; stills before, after and of the original at 1920x1080 for
     Thomas's eye (`evidence/I-0138/artifacts/tranche-2-stills/`). Done.
  3. The exposure: the automatic exposure rebased on the new pass (one target for the whole game, D-0263, D-0264);
     Elden Ring's bounded adaptation shown beside our half strength. Size S. Built 2026-11-13 (I-0138 RESULT
     « Tranche 3 »): the automatic exposure reads our tone-map block, the same factor bit for bit; merged `020685cb`;
     a test flag, `auto-exposure-er-bounds.flag`, and a sheet of five views beside our half strength. Done: Thomas keeps the
     half strength (2026-11-13, D-0276's entry; the flag removed; an exposure after Elden Ring's judged again at tranche 6).
  4. The depth of field and the motion blur, with the cutscenes' lens values and the Reinhard curve of two cutscene
     records. Size L. In three parts, each merged alone
     ([TRANCHE-4-READY](../../../investigations/I-0138-modern-post-chain/TRANCHE-4-READY.md)). Part 4a built 2026-11-13 (I-0138
     RESULT « Tranche 4, part 4a »): the depth of field's and the motion blur's programmes of our own (53 more in
     `post_glare.hlsl`), on the card the same bytes as the library's programmes at the captures and away from them (the
     composite's forty counts, the blur's seven tap counts), the depth of field's also the original's captured draws'
     (the motion blur within its test's unchanged rule); the motion blur's plan serves 2 to 8 taps where it
     served 5 alone (a latent refusal: every world record gives 5). Merged `080479eab`; live, 52 motion blur, 52
     glare and 52 tone-map witness pairs equal (no depth-of-field pair: no level drawn at the witness views). Parts 4b and 4c merged together 2026-11-14
     (I-0138 RESULT « Tranche 4, part 4b », « part 4c »): the cutscenes' draw-parameter files, never read natively (every
     cutscene key fell to `default`), read for every group; the depth of field under holder flags 0 (an edge quality of -1
     takes the creation flags; the composites `CompositeBlurredLevels_T1..T8` ours, equal to the library's); tone type 3
     drawn with the library's Reinhard curve; the original captured in Vordt's intro and the opening scene (its Reinhard
     words and tone map reproduced bit for bit). Done.
  5. HDR10: the float output with the blue-noise dither (Thomas, 2026-10-30), the 10-bit PQ output, two settings
     (maximum luminance, paper white), the UI in HDR, the emissive glow before the tone map. On the OLED with Thomas.
     Size L. Its sheet: [TRANCHE-5-READY](../../../investigations/I-0138-modern-post-chain/TRANCHE-5-READY.md). Built
     (I-0138 RESULT « Tranche 5 »): the HDR10 chain behind the row « HDR output » with its peak and its paper white,
     live; the picture in float with the game's UI drawn in place, the original colorimetry's highlights continued
     above a knee, the dither; witnessed on the ASUS with the chain forced. Left: the session on the OLED with Thomas
     (the knee, the glow, the paper white; the resize, full-screen and frame-generation paths on a real HDR10 chain);
     **the emissive glow is not moved before the tone map** (D-0276 asked it here: in float it adds without a clamp
     at the original's place; Thomas, 2026-11-14: he judges it on the OLED first; the move, if he keeps it, goes with
     item 6's candidate after Elden Ring). On the OLED since (RESULT « Tranche 5 on the OLED »): seven runs on a real
     HDR10 chain without a fault; his judgement by eye still to hold. **Tranche 5b** (his requests of 2026-11-14, RESULT
     « Tranche 5b »): the three HDR rows on the Graphics tab, the peak and the paper white as sliders, the game's
     brightness without effect and locked on an HDR10 chain. **Tranche 5c** (his request of 2026-11-14, D-0278, RESULT
     « Tranche 5c »): the game's own anti-aliasing filter removed (it was the TAA's and DLSS's fallback on an SDR chain). **Tranche 5d**
     (his request of 2026-11-14 20:00, RESULT « Tranche 5d »): the HDR output's two sub-rows hidden while it is off, their
     separator a quarter shorter. **Tranche 5e** (his decision of 2026-11-14 22:16, D-0280, RESULT « Tranche 5e »): the
     automatic exposure no longer darkens under the zone's own exposure.
  6. The Rekindled colorimetry: candidates on sheets, AgX and ACES among them (Thomas, 2026-11-13, relayed by the
     coordinator), one after Elden Ring's shipped curve and settings,
     its slight chromatic aberration kept (D-0276), without its LUT images. Size M. Built and merged `1def62857`
     (RESULT « Tranche 6 »): the three candidates behind test flags, SDR and HDR10; sheets at five zones with Thomas;
     **his pick open** (and his A/B by the keys on the OLED); « Elden Ring » as he means it is its vivid art direction
     (2026-11-14), which may ask more than the candidate.
  7. The settings rows proposed in R12 and not chosen: bloom, light shafts, film grain, field of view, the
     colorimetry, the chromatic aberration (the HDR output's rows: done, tranches 5 and 5b). Size M.
  8. YEBIS and its refusals removed. Size S.
- Open, each lifted in the tranche that needs it (STUDY §6, PLAN §6): what glare shape 3 and the custom glare draw in
  the original; which records each cut of a cutscene selects.
- Waits for Thomas: his eye at every tranche. Implementation after workstream 2, in a fresh thread (D-0215).
- Pilot: **Fable 5.2 high** (Thomas, 2026-11-10) for tranches 1 to 6; tranche 4's parts 4b and 4c by Opus 5.5 high, Fable
  back for tranches 5 and 6 (Thomas, 2026-11-13, D-0276's entry); **tranches 7 and 8 by Opus 5.5 high** (Thomas,
  2026-11-14, D-0281), Fable again only if his pick of the Rekindled colorimetry asks more than the three candidates.
  Size XL.

### 12. Physical light sources and the traced volumetric fog

- The end of the lighting cheat (D-0262): the fill lights and the baked ambient go once the sources are reworked.
- Steps: the sources' families named from the collect, the sky and emissive marks; flames as real sources, emissive
  triangles as sources (the Smouldering Lake's lava, crystals); physical terms in place of DS3's x2; the traced volumetric
  fog lit by the real lights, the Firelink veil's source; new sources only where a visible object is one; the fill lights
  and the baked ambient removed; the zones' tour again.
- After workstream 11 and workstream 7's step 3. Light painted in the textures stays workstream 13's.
- Pilot: **Fable 5.2 high** (Thomas, 2026-11-10), R in support. Size XL.

### 13. T1, the textures

- Thomas's own copy de-lit and upscaled; he publishes them himself, outside the installer.
- Steps: the replacement folder read by the renderer; the tool chain (chaiNNer, Real-ESRGAN, texconv) and a per-zone
  procedure; one pilot zone judged by Thomas; the rest of the game in batches; the real texture-quality row and the
  anisotropy apart ([wish](../../wishes/TEXTURE-QUALITY.md)); DirectStorage only if the loads ask for it.
- The de-lighting is judged after workstream 12; the tooling may be prepared before. The upscale holds the GPU: never during
  a measure. Pilot: Opus 5.5 high for the chain, Sonnet 5.5 for the batches. Size L.

## Phase 3: the release (C5)

### 14. Hardware and configurations

- NVIDIA cards only: nothing is done for AMD and Intel (Thomas, D-0274: he has no such hardware), so no FSR, XeSS or generic
  denoiser and no testers of those cards. A small card is stood for by workstream 10's limited video-memory budget.
- Steps: resolutions (4K, odd sizes, ultrawide, 16:10), two monitors, Resizable BAR; every language of the game; keyboard and
  mouse; the Xbox and PS5 button icons. Pilot: Opus 5.5 high. Size M.

### 15. The second optimisation review

- The aim of about 120 frames/s (Thomas noted « 20fps en PT 4 rebonds »), after phase 2, which changes the costs. It holds
  the engine thread's leftovers of the earlier roadmap (the frame step under the native renderer, DS3's spinning frame
  limiter replaced by a timed wait, the render worker's frame at m45) and, from I-0113, the TAA's reactive forms' GPU cost
  and the Firelink walls' follow effect asking the game again once a frame. PC: measures. Size M.

### 16. The complete review and the cleanup

- A fresh code review of all `native/` (as I-0120); the reviews' « recorded, left » points; the development flags and tools;
  the worktrees and branches (deletions are Thomas's to run); **a short release checklist that replaces the table of the 13
  obligations** in STATE, which stays there until the checklist is written. Size M.

### 17. Interface finishing

- The start logos on black as ER's, the help texts in every language, what the game's tour found in the menus. Size S.

### 18. Publication

- **A complete review of the licences and of the security before anything goes public** (Thomas, D-0274: NVIDIA's code
  compiled into the module, the SDKs, the installer, what the repository holds); Thomas's decisions (where, when, in what
  form); the final installer; **last of all, `game_bootstrap.dll` renamed `rekindled.dll`**. The GitHub copy stays light and
  private and Thomas updates it himself (D-0274): no thread pushes, no decompiled code or game file goes on line, no public
  repository is planned.

## Phase 4: game additions

Nothing here is on the way to the release unless Thomas says so; studies may run before.

### 19. ER-like animations and gameplay ([I-0130](../../../investigations/I-0130-animation-study/RESULT.md))

- Steps: a run playing an edited c1100 clip (loose-file loading in the native module or Mod Engine 2: Thomas's decision);
  the standing-jump prototype; then one by one: jump on A/X, jump attacks, power stance, combos, guard counter, crouch,
  posture; the full decrypted Ghidra project when the gameplay needs it (D-0272); ER's clips need extraction tools (Thomas's
  decision).
- Pilot: Fable 5.2 high, to be confirmed by Thomas (D-0198). Size XL.

### 20. Effects and game options

- ER's blood and some of its effects (D-0233, feasibility not studied); the draw-distance option; the study of the off-screen
  enemies' update; the original's LOD defects (low priority).

### 21. Wishes

- [The NPC « Claude »](../../wishes/NPC-CLAUDE.md), [Thomas's new pyromancy](../../wishes/NEW-PYROMANCY.md),
  [the texture-quality row](../../wishes/TEXTURE-QUALITY.md). No thread without Thomas's go.

## What runs at the same time

| Lane | Now | Next | PC |
|---|---|---|---|
| R (RT/PT) | 2 | 7, then support of 12 | short runs |
| E (open gaps) | 4 (3 done 2026-11-12) | 5 | short runs |
| Robustness | 4, carried by E | 16 | almost nothing |
| The game's tour | 6 | feeds 4, 5, 7 | many runs, at night |
| Cadence | 8 when Thomas and the OLED are there | 15 | measures, the whole PC |
| Fable | 11: tranches 1 to 6 done (I-0138; tranche 4's parts 4b and 4c by an Opus thread) | 12 (11's tranches 7 and 8 go to Opus, D-0281) | short runs; the OLED and Thomas for HDR10 |
| Study (S) | the animation clip | as Thomas says | nothing |
| Installer | 10 once R12 closes | 18 | start checks |

The real limits: one game launched at a time (the ASUS's run queue is the bottleneck, above all with workstream 6), the whole
PC quiet during a measure, the main checkout exclusive for each merge, the weekly limit (D-0248). About five lanes at once is
the useful maximum.

## Models

- Opus 5.5 at high effort pilots every workstream by default (D-0245); final review at xhigh, `integrity-reviewer` before
  each merge (D-0268). Sonnet 5.5 does the mechanical work: targeted decompilation, measures, the tour's sheets, the texture
  batches.
- Fable 5.2 at high effort: workstream 12, and workstream 11's tranches 1 to 6, done (Thomas, 2026-11-10; D-0281,
  2026-11-14: the rest of workstream 11 and workstream 7's remainder by Opus 5.5 high); workstream 19 to confirm. Anything else case by
  case, on Thomas's word each time (D-0198).

## Open decisions

None on 2026-11-12: D-0273 and D-0274 settled those the plan had raised, D-0276 those of workstream 11's plan (the
check of workstream 2 step 5 is answered there: DS3 draws no chromatic aberration in the world).

## Abandoned or outside this plan

- By FINISH-LINE: the measuring apparatus of before D-0227, the pop-in's leftovers unless a defect is seen, the small
  luminance residuals, the formal qualification of the 13 obligations and the open witnesses list.
- By decisions: any raster path with a traced equivalent (D-0251), the GPU-driven raster (D-0255), RTX Mega Geometry
  (D-0238), the top level on async compute (3 % at most), the settings page of our own (D-0271), the Firelink walls' row,
  the two frame-rate sliders of the earlier roadmap (R12c's value row stays).
- Not planned: AMD and Intel cards, FSR, XeSS and a generic denoiser (D-0274); a public repository (D-0274); a PT under NRD
  as a goal; a frame-generation multiplier above x2 (Thomas's card offers x2 only).
