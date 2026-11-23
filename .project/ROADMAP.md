# Roadmap: everything left, phase by phase, workstream by workstream

The general plan Thomas accepted on 2026-11-12 (D-0273, D-0274): five phases, twenty-one numbered workstreams, in the advised
order. A finished step keeps one line here; its account is in its investigation's RESULT. The longer text of 2026-11-15 is in
[archive/2026-11-15/ROADMAP.md](archive/2026-11-15/ROADMAP.md) (D-0291), the earlier ones in tag `archive/pre-roadmap-2026-11-12`
([ARCHIVE.json](ARCHIVE.json)). Current position: [STATE](STATE.md); programme contract: the [native
mandate](NATIVE-D3D12-RESET-MANDATE.md); method: [WORKFLOW](WORKFLOW.md); RT/PT design (historical): [RT-PT-PLAN](RT-PT-PLAN.md);
D-0228's list (historical): [FINISH-LINE](FINISH-LINE.md). Sizes (S, M, L, XL) are estimates; « (plan) » marks a step the plan
added and Thomas accepted with it.

## Capabilities

- **C1**, reproducible analysis baseline: acquired 2026-09-19 (DS3 1.15.2.0, K-0001/K-0002; [decompiled/README](../decompiled/README.md)).
- **C2**, the original D3D11 renderer's map: a read-only reference, recovered only for a named native decision.
- **C3**, the native D3D12 backend, « Dark Souls III: Rekindled »: **current**, phases 0 to 2. First milestone accepted with
  reserves on 2026-10-19 (D-0194). Final acceptance: the complete game and its DLC with the original D3D11 renderer unavailable.
- **C5**, the release: phase 3.

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
| **1. The finish line (D-0228)** | The whole game natively with no fault and no missing effect, checked zone by zone; the compatibility probes | 4 to 10 | R, E, robustness, the game's tour |
| **2. The modern picture** | YEBIS replaced, HDR10, physical light sources, traced volumetric fog, remade textures | 11 to 13 | Fable 5.2 high (12; 11's tranches 1 to 6, done), Opus (11's tranches 7 and 8, D-0281), Opus and Sonnet (13) |
| **3. The release (C5)** | Resolutions, languages, controls, optimisation, final review, publication, `rekindled.dll`; NVIDIA cards first, AMD's FSR and Intel's XeSS after (D-0274, D-0302) | 14 to 18 | Opus |
| **4. Game additions** | ER-like animations and gameplay, ER's effects, game options, wishes | 19 to 21 | Fable to confirm (19) |

Phases 0 and 1 follow each other. Phase 2 starts with its study during phase 1 (read-only, no PC). Phase 3 waits for phase 2
for the optimisation and the final review; its compatibility probes come in phase 1 (workstream 10). Phase 4 holds studies
only before the release, unless Thomas decides otherwise. **The public release comes after phase 2** (Thomas, D-0273); the
installer with it, in workstream 18 (D-0300).

## Phase 0: close what is open

### 1. The full control every three days (D-0230)

- Every three days (D-0230); earlier accounts in I-0132's and I-0139's RESULTs. The last 2026-11-21 (I-0141 RESULT): the gate
  passed, no fault; next due 2026-11-24.

### 2. Finish R12 and close I-0132 (pilot R): closed 2026-11-21

- I-0132 complete, its leftovers in workstreams 7 and 8; its account in
  [archive/2026-11-21/ROADMAP-workstream-2.md](archive/2026-11-21/ROADMAP-workstream-2.md).

### 3. Open gaps: tranche 41, then close I-0113 (pilot E)

- Done 2026-11-12: I-0113 complete, the final table of the 27 gaps at the head of its
  [RESULT](../investigations/I-0113-open-gaps/RESULT.md); open and carried to workstream 5: gaps 16, 17, 23.
- (plan) One session of Thomas in play on the « fixed, never validated » gaps: still to hold, its list after the RESULT's table.

## Phase 1: the finish line (D-0228)

Workstreams 4 to 8 are independent and run at the same time, one game launched at a time.

### 4. Robustness: « skip the object and go on » everywhere

- **Closed 2026-11-13** with [I-0137](../investigations/I-0137-robustness/RESULT.md) (RESULT « Closing »); its left items
  (step 1's selectors without programmes, the refused packet, the filter pass without programmes, the instanced layouts, the
  text layer kinds, the HAL's methods; none met since 2026-10-20) carried to workstream 16.
- Done with it (I-0137 tranches 1 to 4): I-0120's A1 (the controller accepted only the default Steam path), its priority 4
  and priority 5 (D-0273), each fixed or left with its reason. Never checked on a real launch: A1 with the game in a second
  Steam library (I-0137 RESULT « Not checked »), before the installer.

### 5. The effect and draw families not yet native (pilot E, [I-0140](../investigations/I-0140-effect-families/RESULT.md))

- Only what shows with the rays (D-0250). Steps, by family, from
  [STUDY-NOT-YET-NATIVE](../investigations/I-0113-open-gaps/STUDY-NOT-YET-NATIVE.md):
  1. Pass 0x21 drawn, 0x20 never met since (the journals; I-0140 tranches 2 and 24); the light shafts 10003 native (tranche 10),
     their brightness to Thomas's eye; the GPU particles' SH ones; the spot effect lights 11000 native by the map's spot path (m45's
     two in the light list, identity INFERRED; m33 unseen; tranche 27). Done but for his eye on 10003.
  2. Decals: the decal pass (wave 1's D1) and the GX00 class native (I-0140 tranche 6; a view of an overlay itself for Thomas's
     eye); the characters' blood and damage paint (D2) drawn natively since I-0105, seen in both modes on a struck player kept alive
     by F4 (tranche 35: dark blotches on the back); colour and extent not compared (the fights not at the same moments).
  3. Single-model NPC phantoms; the burn MTD parameters.
  4. Gap 16 at 2560x1440 (blocked: the OLED unplugged); gap 3 (not reproduced, t30); menu views' regions (D-0301, t28),
     compared (t30; the Load Game « third row », t32 to t34); the Player views' self-shadow (t31, t35, t37, t38); the original's own DLAA on its menu views (t39); D-0305: our TAA on them, DLAA when the option runs DLSS (t40); D-0306: their TAA's reconstruction tighter, the body at about 0.78 of DLAA (t41); the portrait 2 px left (t38). Sheets for Thomas: gap 16,
     the portrait (26), the mist (25).
  5. Cast natively beside the original (I-0140 tranches 13 to 24): every spell, resin, status, art, goods, infusion and the maggots
     alike by the pilot's eye (56 by a sheets triage) or closed by Thomas's; Chameleon and Firestorm not compared, two shield or Stance arts not used. Left: Thomas's eye on tranches
     13 to 21's strips (D-0181); the blood spurt denser natively at 100 ms (tranche 24). R's: the Moonlight glare streak, Yhorm's
     machete darker, the hits' flashes brighter.
- Not here: YEBIS's refusals (cutscenes, exposure, glare, depth of field) go to workstream 11.
- Pilot: E, Opus 5.5 high; `ghidra-research` for targeted decompilation. Size L.

### 6. The tour of the whole game (plan, [I-0139](../investigations/I-0139-game-tour/RESULT.md)): closed 2026-11-15

- Done (433 reference runs, D-0284); its rows not reached are workstream 16's; played again at the project's end. Its text of
  2026-11-16 in [archive/2026-11-16/ROADMAP-workstream-6.md](archive/2026-11-16/ROADMAP-workstream-6.md).

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
  6. R11b's optimisation leftovers; the interior PT cost never measured; SER measured on one GPU, without a unit test; on an empty
     pipeline store the native passes' pipelines made at their first use, outside the precompile page, and a cold driver cache
     with SER (about 24 s) never run, and NIS's pipeline at its first sharpened frame (Streamline's) not timed (I-0132 R12h step 2).
  7. The cutscenes' lighting, [I-0141](../investigations/I-0141-cutscene-lighting/RESULT.md) (tranches 1 to 28; D-0279, D-0282,
     D-0293, D-0294, D-0297). Left: 32000010 brighter natively (t28: the sun at the traced hits; the sun mask); 32000020 darker natively; the valley's set 300; the
     Road of Sacrifices darker and the ending's body (41000000) brighter (workstream 12's light, INFERRED); m32's 8 models in whole windows
     of tranche 18's state, the margin's « no pixel » (INFERRED); I-0138 t10: 34010010 lighter, 41000060 cut 0 darker.
- Emissive surfaces as light sources (lava, crystals) belong to workstream 12.
- Pilot: R, Opus 5.5 high. Size L. After workstream 2.

### 8. Cadence and the unlocked frame rate: the leftovers

- Steps ([I-0114](../investigations/I-0114-unlocked-framerate/RESULT.md) left them to « a cadence workstream »): the follow
  camera's drift at 240; the witnesses never taken (stairs, lock-on, ledges, ramps, the jump's takeoff under 60, the 30
  frames/s events); UNLOCK-READY §5's physics, effects, post and interface rows; the OLED's 280 Hz clock never paced, Reflex's
  latency never measured; the 60 frames/s path through the installed corrections compared with the original's; gap 27 (k
  held at 1 under 60: an exception to D-0244's « one logic at every rate », recorded as such by D-0273 and looked at again
  here).
- PC: the OLED on and Thomas present, measure runs (the whole PC). Pilot: Opus 5.5 high, numbers by `measure-tools`. Size M.

### 9. I-0127, the effects' batching: closed without step C

- Thomas, D-0273: step C (ExecuteIndirect) is not done; the measured stages cost 0.13 to 0.18 ms. Closed 2026-11-12.

### 10. The compatibility probes (D-0300)

- The probes on Thomas's PC: a run under WARP, GPU-based validation, a video-memory budget limited to 4 then 6 GB. The production
  build and the installer moved to workstream 18 (D-0300).
- After workstream 2 and workstream 4's A1. Pilot: Opus 5.5 high. Size S.

## Phase 2: the modern picture

Thomas's order (D-0264): the post-processing chain first, the physical sources after it.

### 11. The modern post-processing chain in YEBIS's place (D-0262, [I-0138](../investigations/I-0138-modern-post-chain/RESULT.md))

- Every tranche starts from I-0138's [STUDY](../investigations/I-0138-modern-post-chain/STUDY.md), its annexes and the
  [PLAN](../investigations/I-0138-modern-post-chain/PLAN.md) Thomas answered (D-0276).
- The chain (PLAN §4), in Elden Ring's order with DS3's values: DLSS or the TAA, depth of field, motion blur, the emissive
  glow, the exposure, the bloom with the light shafts, one tone-map pass (curve, colour matrix, vignette, 3D LUT), the
  output transform, the UI. Two colorimetries: the original's, remade (D-0178, D-0273), and « Rekindled ».
- Tranches (PLAN §5), each merged alone with before and after sheets for Thomas's eye; the account of each is its section of
  I-0138's RESULT:
  1. to 7. Done (tranche 5's left: Thomas's judgement by eye on the OLED, and the emissive glow is not moved before the tone map
     until he judges it there); their lines of 2026-11-16 in [archive/2026-11-16/ROADMAP-workstream-11.md](archive/2026-11-16/ROADMAP-workstream-11.md).
  8. YEBIS removed (2026-11-16); 8b to 10 done 2026-11-21 and 22 (D-0299, D-0290).
     Left: the bloom's strength (t10); the original's lit shape-3 frame in play (m32_00's slot-1 collisions
     not located).
- After workstreams 12 and 13 (Thomas, 2026-11-16 09:56 and 09:58, « Et si on les réécrit ? », « Cool »): our own grading LUTs by DS3
  area for the Rekindled colorimetry, over its global base (the vivid variant): generated by a script from a few settings an area
  (temperature, tint, saturation, contrast, tinted shadows and highlights) that mark each area's dominant colour (Thomas, 10:06:
  « le bleu froid pour Irithyll ou encore le violet pour le bastion de Farron », examples), into 16x256 strips (the Rekindled entries, which draw no LUT today, take one first), judged by
  Thomas on before/after sheets from the tour's references, in batches; ER's LUTs a style reference only, none of their values
  (D-0274). The tool may be prepared before. Size M.
- After workstream 13 (Thomas, 2026-11-16 05:55 and 05:57): on an HDR10 chain, a gentle widening of the colours from Rec. 709
  toward BT.2020 inside the « Rekindled » colorimetry (its vivid colours after Elden Ring), not an option of its own, after what
  Elden Ring's HDR mode does (read in its Ghidra project, read-only); it makes more sense once the textures are de-lit. Size S.
- Open, each lifted in the tranche that needs it (STUDY §6, PLAN §6): what the custom glare draws in the original (shape 3
  read and drawn in 8f and 8g); which records each cut of a cutscene selects beyond 32000010 and 32000020 (8g).
- Pilot: tranches 1 to 6 Fable 5.2 high (Thomas, 2026-11-10; 4b and 4c by Opus); **from 7, Opus 5.5 high** (D-0281), Fable
  again only if his pick of the Rekindled colorimetry asks more than the three candidates.
  Size XL.

### 12. Physical light sources and the traced volumetric fog

- The end of the lighting cheat (D-0262): the fill lights and the baked ambient go once the sources are reworked.
- Steps: the sources' families named from the collect, the sky and emissive marks; flames as real sources, emissive
  triangles as sources (the Smouldering Lake's lava, crystals); physical terms for the local lights (DS3's own strength since D-0293); the traced volumetric
  fog lit by the real lights, the Firelink veil's source; new sources only where a visible object is one; the fill lights
  and the baked ambient removed family by family once the sky is in (D-0307); the zones' tour again.
- First (R, 2026-11-16, on Thomas's « L'original est toujours plus beau » at Firelink): the
  original's look there is its authored ambient (cool blue-grey on the back walls) and its fill lights on the characters (the player
  pale and modelled), both removed by D-0262; the traced bounces alone give neither. Proposed as the workstream's first step: each
  area's ambient (DS3's environment probes) as a real source of the traced lighting, the sky and ambient light the rays receive,
  shadowed and bounced, for the characters (Father Ariandel, the player too warm: STUDY-ALL-GAPS rows 2 and 4) and the dark
  interiors. Thomas, 11:17 and 11:22: no hurry, « Chaque chose en son temps ».
- Waiting here (Thomas, 11:24, « Oui »): Father Ariandel's limbs dark natively (I-0140 RESULT « Father Ariandel », `i140-e71`): the
  lighting's, OBSERVED (the G-buffer right, the lit HDR dark); that the traced ambient alone fixes it INFERRED (the characters' fill
  lights and an unidentified orange rim in the original): reopened with the sources made, a witness on him.
- After workstream 11 and workstream 7's step 3. Light painted in the textures stays workstream 13's.
- The study ahead (S, 2026-11-22): [STUDY](studies/workstream-12/STUDY.md); its four choices, D-0307.
- Pilot: **Fable 5.2 high** (Thomas, 2026-11-10; D-0281), R in support. Size XL.

### 13. T1, the textures

- Thomas's own copy de-lit and upscaled; he publishes them himself, outside the installer.
- Steps: the replacement folder read by the renderer; the tool chain (chaiNNer, Real-ESRGAN, texconv) and a per-zone
  procedure; one pilot zone judged by Thomas; the rest of the game in batches; the real texture-quality row and the
  anisotropy apart ([wish](wishes/TEXTURE-QUALITY.md)); DirectStorage only if the loads ask for it.
- The characters first (Thomas, 2026-11-16 05:58 and 05:59: « on peut même reprendre les textures de peau, cheveux, visages etc
  d'ER », « on peut commencer par là »): the skin, hair and faces after Elden Ring, its shaders and material settings read in its
  Ghidra project (read-only) and remade as ours; its texture files only as local trials, nothing distributed (D-0279). Beside
  workstream 12's dark characters (STUDY-ALL-GAPS rows 2 and 4), which are the lighting's.
- The de-lighting is judged after workstream 12; the tooling may be prepared before. The upscale holds the GPU: never during
  a measure. Pilot: Opus 5.5 high for the chain, Sonnet 5.5 for the batches. Size L.

## Phase 3: the release (C5)

### 14. Hardware and configurations

- NVIDIA first; then AMD's FSR 4.1 upscaling (RX 9000; RX 7000/6000 through Adrenalin 26.10.1, to check), FSR 3.1 upscaling
  and frame generation; Intel's XeSS possible (D-0302; « last » the pilot's reading); who tests them open; the rays on AMD not
  decided. A small card is stood for by workstream 10's limited video-memory budget.
- Steps: resolutions (4K, odd sizes, ultrawide, 16:10), two monitors, Resizable BAR; every language of the game; keyboard and
  mouse; the Xbox and PS5 button icons. Pilot: Opus 5.5 high. Size M.

### 15. The second optimisation review

- The aim of about 120 frames/s (Thomas noted « 20fps en PT 4 rebonds »), after phase 2, which changes the costs. It holds
  the engine thread's leftovers (the frame step under the native renderer, DS3's spinning frame limiter replaced by a timed
  wait, the render worker's frame at m45) and, from I-0113, the TAA's reactive forms' GPU cost and the Firelink walls'
  follow effect asking the game again once a frame; from I-0138 tranche 9, the TAA's mip bias 0 under the jitter, its clip's test,
  the stone's 0.85 to 0.79 of 2026-11-11. PC: measures. Size M.
- Added 2026-11-21 (D-0304): the native's extra work, eleven items in [OPTIMISATION-SURVEY](OPTIMISATION-SURVEY.md) (a Hi-Z
  occlusion cull; the rays' scene sorted, coarser models, fewer rays and a better denoiser). The methods are the pilot's; no cull
  takes out of the RT/PT scene what a ray can see. Workstreams 12 and 13 do not wait for it; if the frame rate falls too
  low to test them, D-0255's CPU lever comes forward.

### 16. The complete review and the cleanup

- A fresh code review of all `native/` (as I-0120); the reviews' « recorded, left » points; I-0137's left items; I-0139's rows not reached (its RESULT « Suggested next step »)
  and the movie and NPC dialogue audio the tour did not check; the
  development flags and tools; the worktrees and branches (deletions are Thomas's to run); **a short release checklist that
  replaces the table of the 13 obligations** in STATE, which stays there until the checklist is written. Size M.

### 17. Interface finishing

- The start logos on black as ER's, the help texts in every language, what the game's tour found in the menus. Size S.

### 18. Publication

- First (D-0300, from workstream 10): the production build without development hooks, flags or test launchers; the installer
  (D-0267: the renderer and the settings page only, over the player's own game, the original executable required, no texture,
  game fixes as options); no NVIDIA DLL distributed (D-0274): how the module starts and runs without them, how the player adds
  them, what the module compiles in of NVIDIA's (NRD, SHaRC, RTXDI, D-0252); the Agility SDK's redistribution.
- **A complete review of the licences and of the security before anything goes public** (Thomas, D-0274: NVIDIA's code
  compiled into the module, the SDKs, the installer, what the repository holds); Thomas's decisions (where, when, in what
  form); the final installer; **last of all, `game_bootstrap.dll` renamed `rekindled.dll`**. The GitHub copy stays light and
  private and Thomas updates it himself (D-0274): no thread pushes, no decompiled code or game file goes on line, no public
  repository is planned.

## Phase 4: game additions

Nothing here is on the way to the release unless Thomas says so; studies may run before.

### 19. ER-like animations and gameplay ([I-0130](../investigations/I-0130-animation-study/RESULT.md))

- Steps: a run playing an edited c1100 clip (loose-file loading in the native module or Mod Engine 2: Thomas's decision);
  the standing-jump prototype; then one by one: jump on A/X, jump attacks, power stance, combos, guard counter, crouch,
  posture; the full decrypted Ghidra project when the gameplay needs it (D-0272); ER's clips need extraction tools (Thomas's
  decision).
- Pilot: Fable 5.2 high, to be confirmed by Thomas (D-0198). Size XL.

### 20. Effects and game options

- ER's blood and some of its effects (D-0233, feasibility not studied); the draw-distance option; the study of the off-screen
  enemies' update; the original's LOD defects (low priority).

### 21. Wishes

- [The NPC « Claude »](wishes/NPC-CLAUDE.md), [Thomas's new pyromancy](wishes/NEW-PYROMANCY.md),
  [the texture-quality row](wishes/TEXTURE-QUALITY.md), [a dynamic HUD as ER's](wishes/DYNAMIC-HUD.md), [a photo mode that moves the sun](wishes/PHOTO-MODE-SUN.md). No thread without Thomas's go.

## What runs at the same time

| Lane | Now | Next | PC |
|---|---|---|---|
| R (RT/PT) | 7 (step 7, I-0141) | support of 12 | short runs |
| E (open gaps) | 5 (I-0140) | as the coordinator says | short runs |
| The game's tour | 6 closed (I-0139); the full control (D-0230) next | its replay at the project's end | the ASUS at night |
| Cadence | 8 when Thomas and the OLED are there | 15 | measures, the whole PC |
| Fable | 12 after 11 (D-0281) | 19 if Thomas confirms | short runs |
| Study (S) | the animation clip | as Thomas says | nothing |
| Probes | 10 (A1's second-library check) | 18 (the installer, D-0300) | start checks |

The real limits: one game launched at a time (the ASUS's run queue is the bottleneck, above all with workstream 6), the whole
PC quiet during a measure, the main checkout exclusive for each merge, the weekly limit (D-0248). About five lanes at once is
the useful maximum.

## Models

- Opus 5.5 at high effort pilots every workstream by default (D-0245); final review at xhigh, `integrity-reviewer` before
  each merge (D-0268). Sonnet 5.5 does the mechanical work: targeted decompilation, measures, the tour's sheets, the texture
  batches.
- Fable 5.2 at high effort: workstream 12, and workstream 11's tranches 1 to 6, done (Thomas, 2026-11-10; D-0281: the rest of
  workstream 11 and workstream 7's remainder by Opus 5.5 high); workstream 19 to confirm. Anything else case by case, on
  Thomas's word each time (D-0198).

## Open decisions

None: D-0273, D-0274 and D-0276 settled the plan's.

## Abandoned or outside this plan

- By FINISH-LINE: the measuring apparatus of before D-0227, the pop-in's leftovers unless a defect is seen, the small
  luminance residuals, the formal qualification of the 13 obligations and the open witnesses list.
- By decisions: any raster path with a traced equivalent (D-0251), the GPU-driven raster (D-0255, but D-0304's cull), RTX Mega Geometry
  (D-0238), the top level on async compute (3 % at most), the settings page of our own (D-0271), the Firelink walls' row,
  the two frame-rate sliders of the earlier roadmap (R12c's value row stays).
- Not planned: a generic denoiser (D-0274); a public repository (D-0274); a PT under NRD
  as a goal; a frame-generation multiplier above x2 (Thomas's card offers x2 only).
