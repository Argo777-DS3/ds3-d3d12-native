# Preparation study: the anti pop-in workstream (whole game)

> Note (2026-10-26, on its merge into `main`): where this study says the gate runs « on both screens », that was D-0209; since
> D-0218 the gate runs on the ASUS (the OLED added when justified), its memory leg with `-Memory` when a branch touches the memory,
> the loadings or the streaming (the pop-in workstream and I-0099's fix do). The text below is the study as written.

Read-only study (2026-10-25 07:57-08:10 Paris, branch `claude/popin-plan-study` from `claude/i97-tranche5` at `19f0d983`),
written while I-0097 tranche 5 runs its gate: no game, no build, no Ghidra; only the code in `native/` and journals already in
the Evidence Store were read (at idle priority). It sits beside I-0097 because the defects were seen in its tranche 5 runs; the
pop-in workstream is its own (ROADMAP « Pop-in workstream », not tranche 5) and takes its own number when it opens. Statuses: OBSERVED (read
in a journal or in the code here), INFERRED, GENERAL (general knowledge). Nothing here is promoted to the ledger.

A copy for Thomas is in the project files at `etudes/pop-in/POPIN-STUDY.md`.

Inputs: STATE and ROADMAP « Pop-in workstream » on `claude/i97-tranche5` (Thomas, 2026-10-25 06:50-07:05), WORKFLOW « Standing
rules » (No pop-in), D-0150, D-0161, D-0174, D-0181, D-0197, D-0211, D-0213; I-0070, I-0071 (ER-STUDY and RESULT), I-0097 RESULT
(E2, G2, G4); the pop-in rule study [`POPIN-RULE-STUDY-2026-10-24.md`](../I-0100-popin/POPIN-RULE-STUDY-2026-10-24.md); the study thread « Comprendre l'écran de chargement gris
et violet » (2026-10-25 06:21-06:34); runs `i97-t5-noai-settlement-{n,n2,o,o2}`, `i97-t5c-settlement-{1,2}`,
`i70-native-16-m32-enemies-main`.

## 0. The rule and the scope

- **Scope: the whole game** (Thomas, 2026-10-25 07:59, « on règle le pop-in pour l'ensemble du jeu, pas juste le village des
  morts-vivants », relayed by the coordinator). It is already the repository's rule: native production is written per family
  or engine mechanism, never for one asset (D-0161); coverage goes family by family with witnesses (D-0150); a visual workstream
  closes with side-by-side pictures at every witness zone (D-0181). The Undead Settlement is the observed case, not the target.
- **No stand-in** (D-0197, D-0211, WORKFLOW Standing rules): nothing drawn with a substitute texture, mesh or invented LOD; a
  model not ready appears later, **as the last resort only**. The native part (texture decodes and uploads, model storages,
  GPARAM, pipelines) is prepared ahead, during the loading screen or from the cache of DS3's earlier loads (G4). DS3's own
  loading policy (FLVER CPU loads, FXR, map parts) is never forced to preload. D-0213: the preparation may add up to 1 s a load.
- **The reference is the original game** on the same replayed path: the native must show no more pop-in and no more flicker
  than the original, measured, not judged by eye alone.

## 1. What Thomas saw (2026-10-25, Undead Settlement, native only)

1. Objects popping in along the path (`i97-t5-noai-settlement-*`, ASUS, pictures every 2 s in `evidence/I-0097/captures/`).
2. Distant enemies appearing at once, and worse, vanishing and coming back quickly (a flicker the original does not show).
3. After a death, now and then, a grey loading page with violet outlines; at 08:56 a photo shows it at a path's first load too, the band bright magenta (2.3).
Thomas thinks 1 and 2 are linked; the pop-in makes no large stutter.

## 2. What the journals and the code say

### 2.1 During the walk the native loads almost nothing (OBSERVED)

`i97-t5-noai-settlement-n` (native, no AI, the whole Settlement path):
- the 520 model storages were all admitted before the first world frame (`native_model_storage_admitted` 1 to 520 at lines
  16747-24603, the first world frame at line 25174); the storage queue's longest wait is 0.0 ms in every census row after
  frame 2490 (`native_model_registry`);
- texture creations: 2,272 before the first world frame, 269 in its first 145 frames, 44 over the rest of the path;
- the texture content cache (G4) over the run: 30 hits, 2,555 misses (`native_texture_kept`, frame 12060): DS3 does not reload
  map parts on this path as it does at Firelink (TRANCHE5-STUDY §1);
- no draw row refused for a missing texture, constant or palette after the first frames (`native_selected_draw_preparation`
  every 600 frames: `missing_textures` 0, `unsupported_palettes` 0); `missing_geometry` 0 and `zero_fade` 0 or 2 in every
  `native_main_walk_outcomes` sample (one every 600 collects).

So the pop-in seen **while walking** is not our content arriving late (D-0197's delay): nothing is pending then. It must come
from **which objects the native decides to draw in a given frame** (fades, LOD, distance cull) compared with the original's
emit, or from objects refused for a reason the samples miss. Two gaps to explain in T0: 1,623 rows prepared against 1,494
drawn at serial 2511 with every refusal counter at 0; and the samples are every 600 collects, too coarse for a transient.

### 2.2 Distant characters: the native's admission flips (OBSERVED at Archdragon Peak, cause open)

`i70-native-16-m32-enemies-main` (`light-observe.flag`, 12,152 frames): one c2160 entity's admission outcome changed 9,510
times between `admitted` and `zero_fade`; four o391000 entities 9,562 times each (`native_character_candidate`, logged on each
change of outcome). I-0070 read it as « the native follows the engine's fade weights »; whether the original's weights flip the
same way was never measured (I-0070 RESULT, I-0071 « Open »). DS3's load balancer stays at level 0 in both renderers (I-0071
point 1), so it is not the cause.

Code paths that can turn a drawn character into `zero_fade` for one frame (`native/frame/game_bootstrap/09_model_admission.inc`):
- the main walk uses the pass-5 geometry weight for categories 0x02/0x04 and the 0x45 track weight only for category 0x01
  alone (lines 92-108); the suppress bytes E+0xA5D / item+0x71A, which DS3's probe pick sets for the entities it lights, zero
  the pass-5 selection (line 101) and the model is refused (line 118);
- the fade words are read once per collect; the LOD controller step D1CCE0 is their only writer (04_reference_witnesses.inc
  comment above `ObserveLodStep`): a read between DS3's frames, or a weight computed for another view's camera, would give a
  one-frame zero;
- the witness counts every walk (main, shadow, reflection) under one key per entity, so part of the flips may be the shadow walk
  disagreeing with the main walk, not a visible flicker: the instrument must be per view.

DS3's own weights cannot flip every frame (OBSERVED in the decompile, `decompiled/DS3-1152-F-00D1CCE0.c`, MENUS-MAP 62 §3):
the LOD controller step compares the camera distance (times the manager's scale) with the LodParam distance plus or minus a
margin that depends on the last side (+0x92 for the weight +0x7C, +0x93 for the geometry weight +0x80: hysteresis), then moves
each weight one step a frame toward 1 or 0 and clamps it (a timed fade; the first frame is instant); a LOD change also
cross-fades on a timer (+0x78). So a weight that is 1 on one frame and 0 on the next is not DS3's normal output: the flips
point at what the native reads, or at another path to `zero_fade` (the suppress bytes, the view rule, the walks mixed in one
witness), INFERRED; T0 tells which.

The pilot's lead (a distant character drawn only on the frames its pose updates) is **not supported by the code**: the native
pose owner keeps its last snapshot between DS3's updates (`05_selected_scene.inc` lines 80-83: `owner.current` is replaced
only by a new update) and the admission takes it every frame (`09_model_admission.inc` line 203). A pose never updated does
refuse the part (`native_asm_part_pose_stale`, 10 samples in the Settlement run, `updates` 0), which matters at the first
frames after a load, not in steady play.

### 2.3 After a load: the world drawn before it is complete (OBSERVED), the violet outlines unexplained

In the four Settlement runs the first world frame after every load has 84 drawn objects (`native_model_draw_submitted`
`content_draws`): the Continue from the title in all four (serials 1311-1520), and in `i97-t5c-settlement-1`/`-2`, where Beta
dies, the reloads after the deaths too (serials 7106 and 11299, 7001 and 12054). After the Continue the journal's next sample,
10 to 41 frames later, has 1,508-1,561; after the first death of each run, 80-95 frames later, 1,556-1,557. The 84-object
frames carry 37 other draws, the loading page's UI elements the study thread counted. In `-n`, the 269 texture creations of those frames are what was missing (2.1): under D-0197 a mesh whose texture is not resident is not drawn
(`13_model_frame.inc` line 1489). The study thread found the same at the two native deaths of 05:57 and 06:01, with the loading
page's 37 UI elements still on screen: if DS3's fade opens over that frame, the sky and fog show through, grey. The original
seeds every texture holder with SYSTEX_DUMMY until its texture arrives (I-0071 point 2, OBSERVED in DS3's code), so in those
frames the original either draws with the dummy (a stand-in we do not copy) or has nothing visible under its page: **not
measured**.

**Thomas's photo** (2026-10-25 08:56 PC time, the pilot's path bases with `test-no-ai`, project file
`captures/defauts/ecran-chargement-magenta.jpg`; OBSERVED on the photo): it happens **without a death too**, at a path's first
load. The whole window is a uniform light grey, the « Now loading » bonfire mark included, and the item card (« Leather
Shield ») is drawn whole and readable, framed by a thick, ragged band of bright magenta that follows the card's torn-ink edge,
not a thin text shadow. Read in the code (OBSERVED): no magenta debug or clear colour exists in `native/`; the UI filters'
temporary targets are cleared to transparent black at their first use (`primitive_pass.cpp` lines 303-305), and the back
buffer's clear is a dark blue-black (`backend.cpp` line 1123). The only magenta is DS3's own system texture SYSTEX_MAGENTA
(`content/model_common_textures.cpp` lines 94-97, 0xFFFF00FF), one of the 1x1 SYSTEX_* textures the native creates as DS3 does.
Hypotheses, to separate by measure (INFERRED):
- **G (the grey)**: the loading page's full-screen dark backdrop is not drawn on those frames, so the world's first frames
  (fog and sky with 84 objects) show through; with it, black hides everything else.
- **M (the magenta)**: the card's ragged edge layer is drawn with a placeholder instead of its own image (a texture not yet
  imported on those frames: a system texture such as SYSTEX_MAGENTA, or DS3's error/dummy holder, multiplied by the edge's
  mask), or its image carries magenta in its transparent texels (a colour key) that the backdrop normally hides. In both cases
  the band appears only where the backdrop is missing or the image is not ready, which fits « intermittent ».
- **F (the filters, the strongest)**: Thomas adds (08:59) that on the same launch the title menu's text was very pixelated
  and its selection shadow blue, just before the grey page. That launch is `i97-t5b-ariandel-oled-n1` (native, the OLED,
  window 1280x720, launch check 08:56:35; its `start.png` shows the game a few seconds after the photo). Its journal's filter
  plans (`native_ui_filter_plan`, OBSERVED) hold a blue glow (colour FF405CFF = 64, 92, 255, strength 1.5, 3 passes, the
  colour SCALEFORM-FILTERS-NATIVE L5 lists for the HUD) and a **purple glow FF9F2E80 = 159, 46, 128**, strength 0.75, 2 passes,
  blur 50. A glow drawn without its blur, at full strength, gives exactly a hard band of its colour around the shape: purple
  that looks magenta on light grey around the card, blue under the menu's selection. The original itself draws a filtered
  subtree « with the pass reached » when a filter target cannot be made (SCALEFORM-FILTERS-NATIVE step 5, which the native
  copies), and a filter target or its blur at a wrong size would also explain the pixelated text: one cause for the menu and
  the page, set at the launch, which fits « intermittent ». The run's sampled filter rows (every 300 frames) show no refused
  target, too coarse to rule it out.
- **The title menu too** (Thomas's photos of 09:04, `captures/defauts/menu-titre-gris-halo-bleu.jpg` and
  `menu-titre-reseau-gris.jpg`, run `i97-t5b-ariandel-oled-n2`, native, the OLED; OBSERVED on the photos): the title's
  background is the same uniform light grey; the logo shows dark dotted outlines and scattered green pixels; the small texts
  look pixelated; « Continue » sits on a hard dark-blue ellipse; the « Unable to connect » box is drawn black and correct with
  ragged edges. Read together, the simplest single cause is G at every screen without a world: **the black under the UI is
  missing and a light grey is there instead**, and everything DS3 authored to blend on black (soft shadows under texts, the
  logo's dark rim, the selection's blue glow, the card's purple glow) shows as hard dark or coloured shapes on grey. F stays
  possible but is no longer needed to explain the photos.
- **A regression?** (Thomas, 09:06; OBSERVED): not of this morning's code. The native code of the good run
  `i97-t5-noai-settlement-n2` (06:48, the ASUS: its pictures every 2 s show a black title and a normal loading card,
  `evidence/I-0097/captures/i97-t5-noai-settlement-n2` 017-023) and of the bad runs (08:56 and 09:04, the OLED, `main`
  `fa0c0a71`) differ by one comment and the test-no-ai flag's code (`git diff d73dd4c4 fa0c0a71 -- native`); the UI code
  (`game_ui_session.cpp`, `primitive_pass.cpp`, `game_ui_shaders.cpp`) last changed on 2026-10-22 (`ddd9a25a`); the only
  recent change on the OLED's path is the pacing rule (D-0212, 2026-10-24: Present(1) at 280 Hz), which does not change what is
  drawn (INFERRED). What differs between good and bad is the screen (the ASUS composed, the OLED in Independent Flip at 280 Hz)
  and the PC's state (a « 60 » frame-rate counter at the OLED's top right at 08:56: the NVIDIA overlay, Thomas 09:10). The original on the
  same OLED at 09:00 was normal (Thomas). No earlier native title picture on the OLED exists (runs keep in-game pictures only,
  and a GDI capture on the OLED can be stale, I-0097 RESULT tranche 5 item 3), so « since when on the OLED » is unknown.
  Nsight (Thomas's lead, 09:09; OBSERVED at 09:10): the pilot profiled a 7 s test under `nsys` at 08:51 (its report
  `t1.nsys-rep` in the pilot's scratchpad; Thomas accepted an administrator prompt this morning), five minutes before the first
  bad run; at 09:10 no Nsight process runs, no Nsight trace session is listed by `logman query -ets` (only NVIDIA's NVTOPPS
  sessions), and the running DS3 (started 09:11:49) loads no Nsight module (Steam's overlay and D3D12Core only). The NVIDIA
  overlay processes have run since 06:20, so through the good 06:48 run too; the FrameView SDK service (`FvSvc`) runs, its start
  not known. So nothing of Nsight is resident now, but a state it left between 08:51 and the bad runs is not excluded.
  NVIDIA settings (Thomas, 09:14: no game filter, no RTX HDR; OBSERVED at 09:15): the driver profile database
  (`C:/ProgramData/NVIDIA Corporation/Drs/nvdrsdb0.bin`, `nvdrsdb1.bin`, `nvdrssel.bin`) was last written at 05:21, before the
  good 06:48 run, so no driver profile setting changed between the good and the bad runs.
  **Treated as a regression** (Thomas, 09:15: never seen before natively on his OLED). What changed (OBSERVED): his last
  launcher sessions are 2026-10-19 16:36-16:41 (`~/.ds3-play-backups`) and the test launcher's 2026-10-22 08:20
  (`~/.ds3-test-backups`); the two-screen setup with the OLED primary dates from 2026-10-24 05:04 (STATE); the PC was last
  started on 2026-10-24 06:20. Since 2026-10-22 08:20, `main` merged in `native/`: I-0090 to I-0096, I-0097 tranches 1 to 4,
  G6, I-0098 and the step 2 tooling (`git log --first-parent main -- native`); the UI code did not change (last 2026-10-22
  06:34). The screen path changed with I-0098 (10-24 09:05), tranche 3 (10-24 14:02: paced then synced, the screen chosen at
  launch, the window placed before its swap chain) and tranche 4 (10-25 01:44: one pacing rule, Present(1) at 280 Hz). Launch:
  the pilot's runs carry `measure.flag`, `test-no-ai.flag` and, on the OLED, PresentMon (an event-trace reader); the launcher
  carries `measure.flag` only. Every good native run seen today is on the ASUS, where the window is moved from the OLED at the
  title; every bad one stays on the OLED from the start (`native_display_screen` absent in both: no launch placement).
  Suspects, ranked (INFERRED), each with a short test:
  1. **The window kept on the OLED from the start**: a startup state of the presentation or the UI's targets that a move or
     resize repairs (I-0076's follow path runs on a move). Test (5 min): one native launch on the OLED, a photo of the title,
     then the window dragged to the ASUS and back, a second photo.
  2. **A state left by the `nsys` profile of 08:51** (an administrator trace, five minutes before the first bad run). Test:
     Thomas says whether the titles of the native launches of 09:11 (`i97-t5b-ariandel-oled-n3`) and 09:15
     (`i97-t5b-settlement-oled-n1`) were grey; then, if needed, one launch after a restart (his decision).
  Narrowed (Thomas, 09:19: clean on the OLED in the tests he watched before 04:00, i.e. the gate `i97-t5-final` 03:33-04:05):
  `native/` differs between the gate's build (`1b9c91b6`, 03:27) and the bad runs' `main` (`fa0c0a71`) only by the test-no-ai
  code (`04_reference_witnesses.inc`, `git diff 1b9c91b6 fa0c0a71 -- native`), so a code regression after 04:00 is unlikely;
  inside the window the NVIDIA driver profile database was written at 05:21 (with `NvDriverDiagnostics/state.json` at 05:20),
  the `nsys` profile ran at 08:51, and the runs gained `test-no-ai.flag` (also in the good ASUS run) and PresentMon on the OLED.
  The NVIDIA write of 05:21 becomes the first suspect for an OLED-only change (a setting that acts on that screen only, such as
  G-SYNC's windowed mode, would spare the ASUS), Thomas's restart at 09:19 the test of the Nsight one.
  After the restart (Thomas, 09:30): his launcher, with the same `main` module (built 08:41), shows no defect on the OLED.
  The exact difference with the pilot's runs (OBSERVED, `%LOCALAPPDATA%/DS3-DX12/play-20261025-092356.jsonl` and
  `-092703.jsonl` against `i97-t5b-ariandel-oled-n2/startup.jsonl`): same window (1280x720, windowed), same UI viewport,
  same chain (3 buffers, waitable, interval 1); flags `measure.flag` (Thomas; `vsync-off.flag` at 09:27) against
  `measure.flag` and `test-no-ai.flag` with PresentMon reading (the pilot); Thomas's save against the Shebang test save; and
  **the pipeline library**: Thomas's first start loaded none (`file_loaded: false`, every pipeline compiled fresh), his second
  14 records; the pilot's build folder loads a 5.1 MB library of 288 records with a 2.5 MB precompile list
  (`evidence/I-0066/artifacts/game-bootstrap-build/native-pipelines.v2.library(.psc)`, written by every run of that folder,
  last at 09:18), whose pipelines are created on worker threads while the game boots (I-0072, `20_startup_phases.inc`
  lines 159-176). A wrong or half-built pipeline taken at the title from that list (a blend or format state) would give exactly
  « everything blended on black shows on grey », only in the folder with the big list, now and then (a race), INFERRED.
  First elimination, no build needed: the pilot's n3 run repeated with `no-pipeline-library.flag` (the cache in memory only,
  no load, no precompile; the existing A/B flag); clean twice where the same run without it is grey: the library or its
  precompile is the cause. Then, in that order: without `test-no-ai.flag`, without PresentMon.
  The game's shader quality (Thomas, 09:41; OBSERVED): a library entry's key is the whole description, each shader named by
  the hash of its bytecode (`backend.cpp` line 1589, `HashBytes` of `pShaderBytecode`) and the root by its blob's name, so a
  variant compiled under another quality setting has another name and cannot be taken for this one; and both launches read the
  same `GraphicsConfig.xml` (Thomas's, last written 2026-10-22 08:21: WINDOW 1280x720, `ShadeQuality` MAX, every quality key
  as he set it); the pilot's runner rewrites only the screen mode and the window size, to the same values
  (`Run-StartupTrace.ps1` lines 193-205). A library filled under one setting and read under another is therefore ruled out; a
  bad entry for other reasons (a driver-side blob, the precompile race) is not, and the test above stays the first.
  3. **A screen-path change since 10-22** (tranche 3 or 4). Test: the `main` build of 10-24 09:05 (`06747de0`, before tranche 3)
     launched on the OLED, a photo of the title; if normal, the same with tranche 3 (`29a828ae`), then tranche 4 (`8d0c105a`).
  Discriminating check, about 10 minutes after the pilot's tests: the same `main` build launched twice on each screen, the
  title shown 10 s each, now (Nsight closed since 08:51), then with the « 60 » overlay closed, the title read from the native
  back buffer (not GDI) or from a photo; if the OLED is still grey with Nsight gone, Nsight is not the cause; if it is normal,
  one run right after an `nsys` profile tells whether Nsight leaves it. Stopping a service or changing an NVIDIA setting stays
  Thomas's (WORKFLOW Standing rules).
Either way it is a stand-in on screen (D-0197) and part of this workstream, not a cosmetic aside. Note: the « all normal »
Thomas reported at 09:00 was the next run, `i97-t5b-ariandel-oled-o1`, which is the **original** (`mode: reference`): it
says nothing yet about how often the native shows it.

Not a cause (OBSERVED in the code): occlusion. The native creates no occlusion queries
(`20_startup_phases.inc` line 587) and its walk culls by frustum and bounds only (`09_model_admission.inc` line 1072): it can
draw more than the original, never less.

## 3. Probable causes, ranked

| # | Cause | Explains | Status |
|---|---|---|---|
| C1 | The native's per-frame draw decision differs from the original's emit (fade and track weights, suppress bytes, the view a weight belongs to): objects and characters admitted on some frames and not on others, or later than the original | walking pop-in (1), flicker (2), « at once » (2) | INFERRED from 2.1 and 2.2; one mechanism for both, as Thomas thinks |
| C2 | After a load, DS3 shows the world while part of its content (269 textures at the Settlement) is still being created; D-0197 hides those objects | pop-in at the fade-in, the grey page (3) | OBSERVED natively; the original's frames not measured |
| C3 | The UI's filters drawn without their blur at a bad launch (2.3 F), the loading page without its backdrop (G), a placeholder or colour-keyed image (M) | grey page, magenta band, pixelated menu text and blue selection (3), also at a first load | INFERRED; no magenta colour in the native code (OBSERVED) |
| C4 | Late native content in play (E2's storage queue, a texture not resident) | a pop-in where DS3 streams (Firelink, Lothric Castle, Archdragon Peak) | not seen at the Settlement (2.1); to measure where DS3 streams |
| C5 | A wrong pipeline from the build folder's pipeline library (2.3: the pilot's first run without the library showed a black title, 09:45) | the grey and magenta UI; possibly a world pop-in or flicker only if the wrong pipelines are the dithered fade variants an object uses far away | the UI: first test positive (one run); then with a fresh library the title is clean but the pop-in is still there (Thomas, 10:06): **C5 explains the UI, not the pop-in**; C1 (the draw decision) stays the first lead for the world |

**Which objects pop** (Thomas, 10:24, at the Undead Settlement): the small wooden fences and the bushes. Small map pieces
and vegetation have the shortest LodParam distances, so on a walk they cross their fade range all the time, close to the
player; nothing is loaded natively during that walk (2.1). This favours **C1, the fade-in drawn as a pop**: DS3 moves their
weight from 0 to 1 over a timed fade with a margin (2.2), and the original dissolves them in with its dither; if the native
draws them at full opacity as soon as the weight leaves 0 (or only once it reaches 1), they pop. It does not favour C4
(nothing pending at the Settlement) nor C5 (the pop-in stays with a fresh library). The native has the fade path
(`05_selected_scene.inc` line 864: the intent's opacity; `10_options_and_passes.inc` line 275: the dither variant when
`fade_enabled` and the opacity is under the category's scalar; `13_model_frame.inc` lines 1233 and 1558: `FC_DitherEnable`,
`ditherFadeAlpha`; `09_model_admission.inc` line 588: `ProduceDitherAlpha`), so the question is whether these map pieces reach
it (the weight read for the map-object path, D04B90's +0x130 against +0x170, its `fade_enabled`). **First check, at the start
of T0, without a build**: the Settlement path once in each renderer with `light-observe.flag`, whose `lod_step` witness
already runs in both modes (`04_reference_witnesses.inc`, `ObserveLodStep`: map pieces' rows once per model and weight band,
up to 400): name the fence and bush models and compare their weight ramps between the renderers; with pictures at 10 a second
on one short approach to a fence, the original's dissolve against the native's appearance. If the weights ramp alike and the
native still pops, the T0 journal's weight, opacity and dither bit per entity tells which step drops the ramp.

What the pipeline library can and cannot do (OBSERVED in `renderer/backend.cpp` lines 1579-1631): a pipeline **not ready** never
removes a draw: a frame that needs a name another thread is building waits for it (line 1597), otherwise loads it from the
library or compiles it itself, so « not ready » costs a stall, never an object appearing late. A precompiled pipeline is built
from its decoded description and kept only if that description encodes to the same name (line 1659), so the precompile cannot
substitute another description. What remains is a **wrong pipeline for the right description** returned by the library's
driver blob (`LoadGraphicsPipeline`), INFERRED from the title test: it would be wrong at every use, not on alternate frames, so
it cannot make the distant enemies' flicker by itself; it could make objects invisible or wrong at a distance if the bad
entries are the dithered fade variants (`fade_dither`, `model_pass.cpp`), which would look like pop-in at a fixed distance.
**Is the library rebuilt on an update?** (Thomas, 09:49: in games every update forces it; OBSERVED in `backend.cpp`
lines 1532-1563 and 1666-1693): its only stamp is the checksum of the library's own bytes, written in the list, which ties the
two files together; nothing records the module's build, the GPU or the driver. A new module is covered indirectly: every
entry's name holds its shaders' bytecode hashes, so changed shaders get new entries and the old ones age out after `max_idle`
starts. The driver and the GPU are left to D3D12 (`CreatePipelineLibrary` refuses a library from another driver or adapter,
GENERAL). The NVIDIA driver did not change today (32.0.16.1714, dated 2026-09-17, OBSERVED at 09:50), so a driver-version
stamp would not have caught this case; the build folder's library has accumulated entries over many module builds since
I-0072 (the pilot's builds overwrite `game_bootstrap.dll` in the same folder and keep the library). Correction for C5 (to
code in the workstream, not now): (1) a header stamp on the list and the library: the module's build identity (its file hash),
the adapter (vendor and device id), the driver's user-mode version and the D3D12 runtime; any difference discards both and the
start rebuilds them, as games do after an update; (2) the build script removes the folder's library when it writes a new module;
(3) the bad file kept aside as evidence (not deleted) before anything is rebuilt, and the title witness (2.3 M4) run once with it
and once with a fresh library, to name which entries were wrong if it reproduces.
Shortest test: the Settlement path natively twice with `no-pipeline-library.flag` against twice without it, with the
side-by-side sheet (CADENCE-STUDY C-a) and Thomas's eye; then a fresh library (the folder's file moved aside) to tell a bad
file from the mechanism.

## 4. Measures to confirm (tranche T0 builds them, T1 runs them)

M1. **Per-entity visibility journal, both renderers** (`popin-observe.flag`, diagnostics behind their flag): one row per
change, per view, with the frame serial, entity, model name, camera distance and the reason.
- Native: after the main walk's merge, the main view's admitted entities, and for each refusal its code (`zero_fade`,
  `missing_inputs`, `missing_geometry`, incomplete row by reason).
- Original (Reference): a pass-through hook on DS3's main-view emits (D06E10 for draw entities, D04B90 for map objects, MENUS-MAP
  62) logging the entities emitted for pass 5, so the original's own decision is recorded on the same path.
- With it, the fade words each renderer read (`ObserveLodStep` already runs pass-through in both modes) for the entities that
  change.
- Reader `Get-PopinJournal.ps1`: per entity and renderer, the first distance at which it is drawn, its on/off changes a
  minute, its share of frames drawn; matched by model name and placement between runs.

M2. **Readiness after a load**: the frame DS3 starts the world, the frame the loading page is gone (its UI elements and the
fade), the first « complete » frame (no row refused for readiness), and the texture creations and holder inners still on
SYSTEX_DUMMY at the first world frame in both renderers.

M3. **Pictures**: `Capture-ScreenSeries.ps1` at 10 a second around each load's end, and at the path's points every 2 s,
native beside the original (the existing pairing: same path, no AI, the index offset by the motion mark).

M4. **The grey and the magenta** (2.3): a journal of the loading page's UI draws on every frame it is up (behind the same
flag): each element's image identity, whether it is resident, imported this frame or a system texture (SYSTEX_*), and whether
the full-screen backdrop was drawn; with pictures at 10 a second over five path starts and five death reloads (no death is
needed: Thomas saw it at a path's first load). Offline, read-only: the loading card's images in DS3's menu archive, to see
whether their transparent texels are magenta. For F: every filtered primitive of the title menu and the loading page, on every
frame (not one in 300): its passes run, whether a pass fell back (« the pass reached »), its targets' sizes against the screen
and the UI's viewport; and a picture of the title menu at each native launch, so a bad launch is caught from its first screen.

## 5. Tranches

Each tranche: dry check of every announced run (WORKFLOW), a light Opus review per step, the full review at the merge (D-0198),
the gate on both screens before merging anything that changes `native/` (D-0202, D-0209).

- **T0, instruments** (no behaviour change): M1, M2 and the per-view outcome; the readers. Done when a control run with the
  flag off matches `main` and one run per renderer at the Settlement gives the table of 2.2 per view.
  **T0 ready to code** (2026-10-25 18:50 Paris, read in `native/`): the hooks it needs already exist, behind flags, in both
  renderers.
  - Original: the map pieces' emit `ObserveDispEmit` (D04B90, the pass id at `pass+0x124`, `04_reference_witnesses.inc`
    line 1508; installed with `light-observe.flag` or `cascade-observe.flag`, `22_setup.inc` lines 345-353) and the draw
    entities' emit `ObserveDrawEntityEmit` (D079C0, the pass id at `a2+0x124`, line 1925, installed with the character
    updates' hooks, `22_setup.inc` line 329); the frame boundary of a reference run is the original's pacer D5CB0, hooked by
    the original probe (`22_setup.inc` line 199). T0 adds, under one `popin-observe.flag` (reference and native runs, nothing
    without it): the set of entities emitted for the main view's geometry pass (5) in the frame, compared at D5CB0 with the
    previous frame's, one row per change {frame, on/off, entity, model name (a map piece's first child model, a draw entity's
    binding+8), its distance to the camera of the LOD manager (+0x80, as `ObserveLodStep` reads it), the weight +0x7C/+0x80}.
  - Native: the same row from the main walk's merge (where `native_main_walk_outcomes` is written, `12_scene_collect.inc`
    line 963), per admitted entity, plus each refusal's code (zero_fade, missing_inputs, missing_geometry, an incomplete row
    by reason) and the fade intent's opacity and dither bit (`10_options_and_passes.inc` line 275).
  - Reader `Get-PopinJournal.ps1`: per entity and renderer, its first drawn distance, its on/off changes a minute, the frames
    drawn with a weight in (0,1) with and without dither; entities matched across runs by model name and placement rounded to
    0.5 m. Rows on change only, so a run's journal stays small.
  - Cost: one flag and about 150 lines over existing hooks, a step review; it changes `native/`, so the gate before the merge.
- **T1, whole-game measurement** (study bases outside the gate, D-0213): two runs per renderer on each replayed path:
  Settlement (Beta m31), Firelink (Eta m40), Ariandel (Epsilon m45), and the paths tranche 5 adds (Archdragon Peak m32 with
  Eta, Lothric Castle m30 with Beta); a load after death (Settlement), a bonfire travel and a Continue from the title per zone.
  Output: per zone, the share of entities each cause explains. Decision: which of T2 to T4 to do, in what order (a decision
  for Thomas only if a cause needs a trade-off).
- **T2a, the draw decision brought back to DS3's (C1)**: the native's main view admits exactly the entities the original's emit
  draws, with the same weights from the same moment of DS3's frame, per family (draw entities, map objects, parts), never per
  model. Includes DS3's own fade-in: a weight between 0 and 1 drawn dithered as the original draws it (the native has the
  dither variant already, `model_pass.cpp` `fade_dither`). This is the comparison base for T2b.
- **T2b, the modern layer on top (§9)**: only once T2a matches the original. Each item is a step with its own A/B against T2a
  and its pictures; each is optional for Thomas, none is a stand-in.
- **T3, the load (C2, D-0211)**: the native part ready before DS3 reveals the world: what DS3 asks during the load is prepared
  within it (the G4 cache, the textures' upload batched in the loading step); while the loading page is up, no world frame with
  a refused row is shown, by holding DS3's loading step on our pending work (at most + 1 s, D-0213) rather than drawing a
  substitute. What DS3 itself asks only after the world starts is the last-resort case (D-0197), counted.
- **T4, the loading page (C3)**: its images (always the same menu archive) prepared at the game's start (D-0211), so no
  element is ever drawn before its own image is resident and the backdrop is drawn from the page's first frame; no placeholder
  texture on any UI draw (D-0197); M4 as the witness, then 20 loads of each kind without a grey or coloured picture.
- **T5, streaming zones (C4)**: if T1 finds late native content where DS3 streams, the same preparation as T3 in play (the
  cache, the storage queue ahead of the first frame that needs it).
- **T6, closing**: every witness zone (Theta m30, Beta m31, Archdragon Peak m32, Iota m33, Gamma m37, Eta m40, Epsilon m45,
  Delta m50, both sky states where they change, D-0181), pictures side by side shown to Thomas; then the merge.

## 6. Success criteria (against the original, same path, two runs each)

1. **Readiness**: 0 rows refused for readiness in any frame shown after the loading page is gone, at every zone and load kind;
   each last-resort delay (D-0197) logged with its cause, expected 0 on the paths.
2. **Appearance**: for every entity both renderers draw on a path, the native's first drawing distance at least the original's
   minus 2 % (its own run-to-run spread to be measured in T1; the threshold set from it if larger); no entity the original draws
   that the native never draws.
3. **Flicker**: per entity, the native's on/off changes in the main view at most the original's (0 for an entity the original
   draws steadily); over a path, the native's total at most 1.1 times the original's.
4. **Loading**: 0 pictures showing the world grey or incomplete under the loading page over 10 death reloads and 10 bonfire
   loads per zone tested; load time + 1 s at most against `main` (`Get-I97LoadTimes.ps1`).
5. **No regression**: the gate PASS on both screens; memory per visit within the gate's leg.
6. **Thomas's eye**: the T6 pictures, and his own play at the Settlement.

## 7. Risks and open points

- The original's emit hook (M1) must be pass-through and cheap: measured with the flag off and on in the original.
- Run-to-run position drift (Thomas: the replayed character's position varies): entities matched by model and placement, the
  distances compared per run pair.
- If the original's weights flip too (unlikely given DS3's timer and hysteresis, 2.2) while its draw looks steady, the
  difference is in when or what the original draws from them (a kept draw list): T2a then copies that behaviour.
- C2's hold of DS3's loading step must not touch its game logic (D-0211): only our own pending GPU work is waited for.

## 8. Decision: the modern method, in two steps (Thomas, 2026-10-25 08:25-08:27 Paris)

Thomas (project chat, relayed by the coordinator): « on fera mieux que ER en supprimant ces défauts de pop in pour se
rapprocher des jeux modernes sous UE5, même si nous on a pas nanite », then « Ok, on fera la méthode moderne alors, vu que
c'est mieux ? ». The order (the coordinator's recommendation, accepted): **first** the native's draw decision brought back to
DS3's (T2a: our gap corrected, the comparison base), **then** the modern layer on top (T2b: fades, margins, preparation ahead),
to do better than DS3 and ER, **without any stand-in** (D-0197, D-0211). The working thread records it in `decisions/LOG.md`
when the workstream opens.

## 9. How DS3, ER and modern engines hide transitions, and what we take

### 9.1 Verified in the repository

| Behaviour | DS3 (original) | Elden Ring |
|---|---|---|
| Fade weight as an object nears its draw distance | timed: one step a frame toward 0 or 1 (`FUN_140D1CCE0`, decompile in `decompiled/`) | a linear ramp over [start, start + length] of distance, **no timer** (140D44120, checked by Opus, I-0071 ER-STUDY C) |
| Margin against flicker at the threshold | **hysteresis** on the draw distance, per weight (+0x92/+0x93 with param +0x18/+0x20) | **none on the fade** (140D44120); hysteresis only on the border next to the current LOD (140D44440) |
| LOD change | cross-fade on a timer (+0x78) | five borders with margins, or a continuous mode with a blend ratio (140D442D0) |
| Distances scaled by load | `CSLoadBalancerImp`, same shape, stays at level 0 in both renderers (I-0071 point 1) | `CSLoadBalancer`, level 0-20 from a frame-rate-like measure, shrinks every LOD and fade distance (140E0F220) |
| A texture not arrived | holder seeded with SYSTEX_DUMMY (I-0071 point 2) | the same, plus a fallback holder when the primary is not valid, INFERRED the `_l` package (140B7F4A0) |

So on fades DS3 is already better than ER (timer and hysteresis), and ER's defects that cause visible pops are a fade without
timer or margin and draw distances that shrink when the frame rate drops (INFERRED from the code, I-0071 ER-STUDY C, which
ranks them as candidate causes of ER's own vanishing distant characters). We copy neither defect: the load balancer stays at
level 0 and the native keeps DS3's timed, margined fades.

### 9.2 General knowledge (not checked here; sources in [`POPIN-RULE-STUDY-2026-10-24.md`](../I-0100-popin/POPIN-RULE-STUDY-2026-10-24.md))

- Unreal Engine without Nanite hides transitions with dithered LOD transitions (`AllowDitheredLODTransition`), distance-cull
  fades, HLOD for far geometry, and prepares pipelines ahead (PSO precaching, « skip the draw until the PSO is ready », which
  is D-0197's rule). GENERAL.
- Modern engines decouple a distant character's animation rate from its drawing (it is drawn every frame from its last pose,
  or an interpolated one). GENERAL.

### 9.3 The modern layer (T2b), candidates in order, each A/B against T2a

1. **Every appearance dithered, never a pop**: what D-0197's last resort delays (a model whose content arrives late) and any
   object entering with a weight jump appears through a short dithered fade, timed in seconds rather than frames, with the
   same dither DS3 uses for its own fades. No substitute is drawn: before it is ready the object is absent, then it fades in.
2. **A margin and a timer wherever DS3 has none**: if T1 finds a family whose drawing switches without DS3's fade (an entity
   category, a view rule, the suppress bytes), it gets the same hysteresis and timer as the LOD controller step.
3. **Distant characters drawn every frame from their last pose**: already the native's rule (2.2); kept, and checked by the T0
   journal at the distances where DS3 slows their update.
4. **Preparation ahead** (D-0211): T3 and T5, so that item 1 almost never fires.
5. **Not in this workstream**: invented LODs, impostors or HLOD built from DS3's data (a stand-in, and changing the data); the
   longer draw distance (ROADMAP, after the preparation ahead); mip streaming (a blurred texture is a stand-in).

Criterion for T2b, in addition to §6: per entity, no frame where it goes from absent to fully drawn, or back, without a fade
(the T0 journal's weight column), on every path; the pictures at 10 a second around a load show no object popping.
