# R11-PLAN: the optimisation and the zones' tour in PT

The R pilot, 2026-11-09 (Paris), from main `be57dca2`. No study thread report for R11 (none filed); ER has no path tracing to
copy (D-0236), so the method is the measure, then NVIDIA's public practice (said « connaissance générale » where it is).

## 1. Where the time goes (what is known)

R8c's table (I-0128 RESULT « R8c », `677c2c2e`, the ASUS, 1920x1080 DLAA with RR, milliseconds, OBSERVED):

| View | frame | GI | reflections | cache | DLSS-RR |
|---|---|---|---|---|---|
| Firelink, RT | 13.8 | 4.1 | 2.6 | 1.1 | 3.3 |
| Firelink, PT B = 2 | 29.5 | 11.2 | 11.1 | 1.1 | 3.4 |
| High Wall, PT B = 2 | 31.1 | 13.0 | 11.1 | 1.2 | 3.3 |

Since then R9 added the traced probes (`rt_probe`) and R9d's fill shadow (`rt_fill_shadow`, two rays a pixel, on by default since
D-0259), neither timed on the merged tree; R10b's frame generation (off by default) not timed. A path vertex past the first costs
about 7 ms for each of the GI and the reflections at 1920x1080 (INFERRED: its trace and its material take most of it). The traced
passes are full-screen pixel shaders with inline ray queries (`rt_reflect.hlsl`, ps_6_6); the runtime is the inbox one (shader
model 6.8, no Agility SDK at R11's start: no shader execution reordering, which needs 6.9 and a ray generation shader; the Agility
SDK 1.620.1 allowed and downloaded on 2026-11-09, §5).

## 2. R11a: the measure and the changes that keep the estimator

1. **The measure** (one slot, the PC entirely free: it measures; said to Thomas as WORKFLOW asks): `rt-gpu-measure.flag` on the
   current tree, Firelink's yard, the High Wall and Anor Londo's hall (the interior, by its teleport), the RT and PT levels, DLAA
   and DLSS Quality, 1920x1080, 1280x720 and one window off 16:9; the probes', the fill shadow's and the water's zones read too.
2. **Russian roulette past the first vertex** (PathFrom, the PT level): after a vertex's lobe is drawn, the path goes on with the
   probability p = min(1, max(through)) and its throughput divided by p, else it ends with nothing (unbiased; standard practice,
   connaissance générale). With DS3's albedos (about 0.2 to 0.5) most second vertices end before their trace (H1). Written
   behind `rt-roulette.flag` (opt-in, `0c8f501d`); the GI's mean checked against the flag off (Compare-GiMean.py, the still views
   of R8b), the displayed noise shown to Thomas; then the default (its flag turned into `no-rt-roulette.flag`, the test tool) or
   dropped. A floor on q (0.1 to 0.25, still unbiased) if the stills show the dark paths' large weights (the step review's L3).
3. **R9d's fill shadow folded into the GI pass** (I-0129 « Uncertainty »): the pixel's surface, its normal, its set and its
   geometric normal made once; the two outputs keep their own targets and blends (the GI added into CmbAlbedo, the visibility
   multiplied into the mask's red channel) if the pass order allows it, else the shared reconstruction factored only. Measured;
   dropped if it saves nothing (H2). Read 2026-11-09 (`scene_pass_lighting.cpp`): the order does not allow it: the fill shadow
   writes the mask before GXLightDeferredAO (pass 0) reads it, the GI adds into CmbAlbedo after pass 2; what the two share is a
   few G-buffer loads and the surface's reconstruction. So not folded; the fill shadow's own zone is measured (§2.1) and, if it
   weighs, its two rays are the lever (one ray under RR), not a fold.
4. **The optimisation pass** over the traced passes: work done twice (the G-buffer read by several passes, the regions' coverage
   loop), the 30 skipped walks' rule, the CPU vectors of R8c's grid made each frame (I-0128 « R8c » L7).

R11a merges as a tranche once written and reviewed (D-0227); its measure is a diagnostic for the choice of §3, not a gate. Item 4 was
not done in R11a: it moves to R11b (the final review's M2), with I-0128 « R8c » L7.

## 3. R11b: one structural optimisation, chosen from R11a's numbers

Candidates, to rank by R11a's figures (each its own flag, measured the same way):

| Candidate | What it saves | What it changes |
|---|---|---|
| A. One path a pixel at the PT level for the diffuse and the specular (the lobe drawn by weight, RR given the one signal) | about half the PT's traces | the noise's split between the two signals; RR's inputs (to read in its guide) |
| B. The RT level's reflections at half resolution on rough surfaces (RT-PT-PLAN §6's own RT level) | most of the RT level's reflections | the reflections' sharpness on rough surfaces |
| C. The traced passes as compute shaders on 8x8 tiles | the quads' helper lanes, better coherence | nothing in the picture |
| D. Shader execution reordering (DXR pipeline, shader model 6.9) | the paths' divergence (NVIDIA's figures, connaissance générale) | the Agility SDK (allowed and downloaded 2026-11-09, §5), a large rewrite |
| E. Frame generation's tags `eValidUntilPresent` (I-0131 « Final review » L3) | three full-size copies a frame | to try on the OLED |

A candidate that changes the picture (A, B) is put to Thomas with its numbers and pictures before it is the default.

## 4. R11c: the zones' tour in PT

- **The zones** never looked at natively (FINISH-LINE line 8): Farron Keep, the Catacombs of Carthus, Smouldering Lake, Irithyll
  Dungeon, the Profaned Capital, the Grand Archives, the Consumed King's Garden, the Kiln of the First Flame, the Ringed City; with
  them the witness zones' unseen parts the plan named: the High Wall's and the Undead Settlement's water
  (`evidence/I-0125/artifacts/water-boxes-m30-m31.txt`: three pieces each, none in view from the zone saves' bonfires).
- **The way there**: Eta's travels from the m40 zone save (LIVE-RUN-NOTES: Eta reached Archdragon Peak that way, its Travel
  page has the Kiln's tab; `Run-AlphaCast -Continue -BonfirePages -TravelPress`, the tab and row checked on `go-1.png`), each zone
  reached kept as a new hash-recorded test save (`-KeepSaveLabel`, WORKFLOW « Test characters' progress »; D-0174's one character
  other than Alpha); a zone Eta has not reached is named, with what it needs. The water by teleport from the m30 and m31 zone saves (the boxes' centres, the
  teleport frame checked first: LIVE-RUN-NOTES' warning on the High Wall's MSB frame).
- **What is taken**: at each bonfire the RT and PT levels' first shot and a turn of looks, the journal's faults, skipped objects and
  `native_rt_*` rows (instances without records, materials missing, lights out of the grid); the original's shot at the same place
  for Thomas's comparison (D-0181; D-0249: not a pixel target). Where a surface looks wrong, the control views (`rt-view`, the
  materials') name its family before any fix (D-0150, D-0161).
- **The light painted in the textures** (RT-PT-PLAN §12 risk 9): left to T1's de-lighting (Thomas, 2026-11-09 03:09: « Laisse pour
  T1 du coup si c'est déjà prévu »); nothing in R11.
- **Looked for on the way**: a local-light noise RR leaves (ReSTIR DI's condition, I-0128); the lit cumulus and a size off 16:9
  (I-0125); the magenta frames and their causes (§5 item 4). (`rt-fill-shadow-all`'s retry lapsed with D-0262: no fill light left.)
- **Faults** met are fixed in the tranche by family, or recorded with the zone if the fix is larger.

## 5. R11d: the light sources' rework, at Thomas's eye

**Rewritten 2026-11-09 (D-0262):** no lighting cheat is simulated or created any more; the traced frames' light comes from the real
sources, whose settings alone are adjusted. So R11d:

1. **Removes the cheats** from the traced frames, their code with them (D-0237's option A): the light sets' fill lights (lights 1 to
   3, the pixel's passes and the hits' DirLights), R9d's fill shadow pass and its flags (D-0259 lapses), and the other stand-ins the
   inventory finds in the rays (to read in the code first: the hits' baked ambient as the cache's seed, the region cubes and the
   sets' hemisphere for an unmet ray, the cubes for the RT level's rough surfaces, FC_ShadowColor's floor under the traced sun);
   each one named to Thomas before it goes.
2. **Sheets of the real sources' settings** at the witness places (Firelink outside and inside, Anor Londo's hall, the High Wall),
   the raster's picture beside them: the local lights' intensity (x1, x2, x4) and reach, the emissive surfaces' intensity, the sky;
   test flags for the sheets, Thomas choosing by eye.
3. **The chosen settings applied by family** (D-0150) to every zone, the default.

**Thomas's choice on the sheets (2026-11-09 05:11):** « Va pour x2, et logiquement en extérieur ça sera moins car le Soleil est là.
Faudrait ajuster en fonction d'IRL quoi, reproduire le même comportement »: the local lights x2 the default now; then the real
behaviour: (a) the local lights in physical terms by family (candle, torch, embers, the bonfire's fire: a realistic intensity each,
the inverse-square falloff without DS3's cut at its range, the sun about 100,000 lux), (b) an automatic exposure from the traced
HDR (the eye's adaptation; DS3's fixed exposure per area, YEBIS's auto exposure never on, K-0448), sheets to Thomas at each step.
The other PT games do the same (Thomas, 05:13; connaissance générale: photometric units, inverse-square falloff, an exposure like a
camera's, the artists keeping a hand on exposure and mood). Sources to read at that step, into a short sheet in this investigation
(real intensities by light family, the falloff, the exposure rule): Lagarde and de Rousiers, « Moving Frostbite to Physically Based
Rendering », SIGGRAPH 2014 course notes; Unreal Engine's documentation on physical light units and auto exposure (Thomas, 05:14:
« Nickel si on a déjà de la documentation dessus »).
Thomas (06:22, on the final sheets): « J'imagine que c'est à cause des textures peintes que c'est très illuminé. Par contre ça n'a plus
la même ambiance, forcément, la map est pas construite pour le PT »: the pilot's answer, the step's design: the automatic exposure's target
by area is the mean level DS3 had in the original, measured on the original's pictures (a value by area, not YEBIS's: YEBIS is replaced after the PT, RT-PT-PLAN §8, Thomas 06:23), the light itself physical; the
painted light stays T1's.
The exposure is our own pass (Thomas 06:27, « j'ai peur que ça soit du travail inutile si tu modifies YEBIS »): a histogram of the
traced HDR's luminance to an adapted exposure value, handed to YEBIS's tone map as one input today and kept as it is by the modern
chain that replaces YEBIS; nothing of YEBIS's tone map or grading is tuned.

The light painted in the textures stays for T1 (Thomas, 03:09).

4. **The frames the rays do not serve shown magenta in the test runs** (Thomas, 04:38 to 04:43: « Oui en magenta, comme ça on voit
   mieux », for the three cases: the first world frames before the rays' scene is built, a skipped walk, a latched failure;
   `rt-fail-magenta.flag`, Run-R7aGi's default and the start check's; the player's launcher keeps DS3's fallback), counted in the journal. Then (Thomas,
   04:43, « Le cas 1 peut être corrigé? »): the arrival frames held until the rays' scene is built (with a time limit), once the
   tour has counted them; then (04:44, « comme ça on n'a plus besoin de repli »): a skipped walk traced with the previous frame's
   top level, a latched failure or an adapter without DXR a clear message and a clean close (the pilot's proposal, Thomas's call),
   and DS3's fallback lighting removed.
   With it (Thomas, 04:47): the rays' walk asynchronous (each frame takes the last finished scene, never waits nor skips), the top
   level's build on an async compute queue to measure, and D3D12's GPU upload heaps for the per-frame uploads if the inbox runtime
   offers them (Resizable BAR is on, Thomas 04:48).
   The Agility SDK allowed (Thomas, 04:48: « Tu peux prendre l'Agility SDK d'ailleurs »): Microsoft's NuGet package
   `Microsoft.Direct3D.D3D12` into `Tools/DS3-Decompilation/third-party` (its signature checked, nothing in the game folder), loaded
   from the module through `ID3D12SDKConfiguration1::CreateDeviceFactory` (never tried inside DS3: RT-PT-PLAN §12 risk 5); it opens
   the upload heaps, shader model 6.9's reordering (SER: R11b's candidate D) and opacity micromaps.

## 6. The open points carried from the investigations closed on 2026-11-09

Thomas's yes (2026-11-09 01:55, relayed): I-0111, I-0114, I-0117, I-0118, I-0119 and I-0125 closed, their open points here. R11 takes
the rows marked R11; the others are listed so nothing is lost.

| From | Open point | Where it goes |
|---|---|---|
| I-0111 | Whether Streamline's interposer resolves `dxgi.dll` to the game folder's proxy (H1) | recorded; no defect seen since |
| I-0114 | The follow camera's 240 drift (which of the nine laws); what moves the walked line at 240 | a cadence workstream, not R11 |
| I-0114 | The OLED's display clock at 280 Hz never paced; Reflex's latency not measured | R12 (frame generation and Reflex in the menu), the OLED with Thomas |
| I-0114 | The stairs, lock-on, ledge fall, ramps, the play launcher's `-FrameRate` not witnessed; the counters' fault (`unlock-observe.flag`) not isolated; UNLOCK-READY §5's physics, effects, post and interface rows; the fifth tranche's four items; the jump's takeoff under 60; the 30 frames/s events | a cadence workstream, not R11 (the jump under 60 kept as the original's, Thomas 2026-11-01) |
| I-0117 | The BLAS memory at the other zones, the build's GPU time | R11a (the scene's zone) and R11c (the memory read at each zone) |
| I-0117 | The High Wall's control view at 96.5 % (T4's 99 %); the characters' outline | R11c, if the tour shows a defect |
| I-0118 | The right edge's pillar, the ray offset's acne on other surfaces; `m37_00_00_00_001005` casting at every distance (D-0250) | R11c |
| I-0118 | The rays' skipped walks on a long travel | R11c (the travels) |
| I-0119 | The families without a material (`Col`, `ColDif`, `ColSh`, `EmiGlow`, `Veill_Curtain`, the eyes, `_mt1`, `_p`, `_em`, unlit, BloodBlend, Sand, the Dreg Heap's pink family; the Mul variants, `DirtMask`, Pom, SnowLt), the wet and blood blocks, the skinned shading normals, m45's other vertex formats | R11c, as the tour meets them (by family) |
| I-0119 | FC_EmissiveColor not read | INFERRED closed by I-0126 R7b (the record's emissive colour, `rt_material.hlsli`): checked at the tour |
| I-0125 | The High Wall's and the Undead Settlement's water; the lit cumulus; a size off 16:9; the GPU cost | R11c, R11a |
| I-0125 | R6-PLAN §1, §4 and §5's TO CHECK items; the families still off | R11c |
| I-0125 | The ASUS gate's measure again (display_late), the endurance tour at the OLED's window size | the full control (D-0230), not R11 |
| I-0128 | NRD's frames 0.8 steps darker than RR's; a still interior other than Firelink's; ReSTIR DI | R11c (the interiors), R11a (the measure at the hall) |
| I-0129 | The fill shadow's cost and its fold into the GI; the water's lighting; the E03 model particles' light; the lightmapped forward rows | R11a, R11c |
| I-0131 | The traced frames' final-tree pictures; frame generation's screen-mode changes | R11c; R12 |

## 7. Order and slots

**Changed 2026-11-09 03:01 to 03:12 (Thomas, after R11a's pictures, D-0262):** R11d (§5, without any lighting cheat) comes right
after R11a's merge, before the tour (R11c), which then looks at the chosen lighting; R11b (with §2.4) after the tour. The order below
was the first one.

R11a's code (§2.2 to §2.4) → its step review → one measuring slot (§2.1, before and after each flag, about 30 minutes, the PC
entirely free) → R11a merged → R11c's tour (one or two slots, no measure, about 40 minutes each) → the pictures to Thomas → R11b and
R11d from his answers and R11a's numbers. Each slot asked of the coordinator.

## 8. R11e: no fallback (the R pilot's fresh thread, 2026-11-09, from main `0777ba17`)

§5 item 4 made concrete from the code (read 2026-11-09: `12_scene_collect.inc` CollectNativeRtScene, `scene_pass_frame.cpp`
EncodeRtScene, `scene_pass_lighting.cpp` EncodeNativeLighting, `20_startup_phases.inc`). What the code does today, OBSERVED in it:
the rays' walk runs on the main view's collect (the engine thread, its chunks on the admission pool); a walk that returns a reason
(`culler`, `culler sign`, `disp service`) or throws gives its frame no scene, and 30 in a row stop the traced sun and lights;
an unserved main-view frame is lit by DS3's passes without visibility (the fallback: the sets' fill lights and GB4 without the
GI's inputs, the point batches without the native lights' pass, the sun without its traced mask), magenta under
`rt-fail-magenta.flag`; a latched failure turns its pass off for the session; an adapter without DXR or a failed self-test runs
the whole game on the fallback; `no-rt-scene.flag` is the « raster » mode of the runners. The first three walks of every start
check since R2 met no candidate (`native_rt_walk` calls 1 to 3), the walks are sampled every 600th: the arrival's profile is not
known (no row covers it).

Tranche R11e (the steps reviewed one by one, merged together):

1. **A skipped walk traced with the last scene**: the last walk's scene (its instances, skinned BLAS held by it) kept until the
   next scene reset; a walk that skips or throws hands that scene to its frame, counted (`reused`); the 30-walk rule removed.
2. **The frame's service on every main-view frame** (not only under the flag): the causes of R11d (1 sun, 2 top level or
   material table, 4 GI, 8 local lights) computed always; an unserved frame shows its world black in the player's build (the UI
   drawn over it), magenta in the test runs (`rt-fail-magenta.flag`), counted with its causes.
3. **The arrival held**: from the first walk after a scene reset, the world shown black (the UI kept: DS3's loading screen and
   fade go on over it) until the scene is ready: frames served on three walks in a row, instances present and their count steady
   (within 2 % over max(250 ms, the last 10 walks' span)), at most 5 s from the arrival's first lit frame (the step review's M1, M2,
   L3); then released (the first released frame restarts DLSS's, RR's and NRD's history:
   the velocity's reset). One row an arrival (`native_rt_arrival`: walks, frames and milliseconds held, ready or timeout, the counts
   at release, the walk of the first instances and of the first served frame), and the arrival's walks journaled (the first 240
   walks of the first three arrivals) so the tour measures the rule. A timeout releases the frames (black where unserved, step 2);
   its frames still unserved then fall under step 4's run of unserved frames.
4. **A latched failure closes the game with a message**: any traced pass's latched failure (the top level, the sun, the local
   lights, the reflections, the GI, the cache, the water, the probes) or rays not usable at startup (no DXR 1.1 or the self-test
   failed), or main-view frames unserved in a row outside a hold for 2 s and 30 frames (what no latch names: NRD latched off on
   frames RR does not denoise, a scene never ready after an arrival's timeout), or an arrival's frames held 60 s and 600 frames without
   a lit frame, sets one fatal reason (a latch of an optional-looking pass, the water, the probes, the cache, is fatal by design: no
   traced pass may silently stop). Then the « Dark Souls III: Rekindled » error box naming the reason on its own thread (no owner: the
   game's window thread never blocked; topmost) and, once it is closed, SC_CLOSE posted to the window (the start check's close: DS3's
   own quit, its saves and threads stopped), again every 10 s then 30 s while the window lives; the world stays black until then. No
   box in the test runs (`rt-fail-magenta.flag`: the row, then the close). Thomas's agreement: 2026-11-09 04:45 (D-0262).
5. **DS3's fallback removed** from the main view: the fill lights and GB4 always off there, the point batches only for the menu
   views, the cubes along R of a frame without a top level, `no-rt-scene.flag` and `no-rt-selftest.flag`'s raster world, the
   runners' « raster » mode (the original game is the comparison, D-0181).

Tranche R11f, after R11e's merge: the rays' walk off the engine thread's critical path, and the Agility SDK.

- **The walk asynchronous, within the frame**: a walk may not run across the engine's next update (it reads the entities, poses and
  wind blocks the update writes, and calls the engine's traversers: a race with freed entities; ER's draw jobs also run inside
  its frame's job graph, connaissance générale on ER from I-0071's study). So « asynchronous » is realised as the walk started
  first and overlapped with the main walk and the passes' preparation on the engine thread, joined before the collect publishes;
  the admission pool runs one job at a time (`work_pool.h`: a busy pool runs the caller's job alone), so the overlap needs its
  own workers; the two walks' traversers on the same layers at once (their marks on the entities) read first. Measured before and
  after (the collect's time, the frame's), kept only if it saves time. The top level's build on an async compute queue: measured
  first (its GPU time in `rt-gpu-measure.flag`'s RtScene zone), built only if it weighs.
- **The Agility SDK 1.620.1**: `D3D12Core.dll` (signature checked by the build script) in a `D3D12` folder beside the module,
  the device made through `D3D12GetInterface(CLSID_D3D12SDKConfiguration)` → `ID3D12SDKConfiguration1::CreateDeviceFactory(620,
  path)` → `ID3D12DeviceFactory::CreateDevice` (its debug layer and DRED through the factory's configuration interfaces), the
  inbox runtime kept when the factory cannot be made (a row says which runs); Streamline is in manual hooking (`slSetD3DDevice`),
  so the factory's device reaches it. Then the GPU upload heaps (`D3D12_HEAP_TYPE_GPU_UPLOAD`, OPTIONS16) for the frame's upload
  arena where the device offers them.

## 9. R11h: the sources in physical terms and the automatic exposure (the R pilot's fresh thread, 2026-11-09, from main `26902421`)

§5's last item (Thomas 05:11 to 06:27), made concrete from the code read on 2026-11-09 (OBSERVED in it): the tone map is the
original's `ps_TonemapHDR_Dither_Exposure_Matrix_Gamma` drawn by `TonemapStage` with the blended Tone Map record's Exposure as a CPU
constant (word 616); its t2, the original's adapted-exposure input, is a null view whose Offset words (744 to 759) make its factor 1
(the disassembly, `evidence/I-0067/queries/yebis-world/ps_TonemapHDR_Dither_Exposure_Matrix_Gamma.asm`: the scene term is
`t0 × (Offset[2].z (Offset[3].x (m − avg m) + avg m) + Offset[2].w) × cb154.x` with `m = t2 × Offset[2].x + Offset[2].y`, and the
glare's term carries the same factor); DLSS reads the exposure from a 1x1 R32_FLOAT copied each frame (RR reads none); the effects'
colour scale is 1.5 / the area's exposure (21_effects.inc, the original's rule), so an effect's light is scene-referred once the
area's exposure stays its own; the glare's bright pass takes the YEBIS globals' exposure.

Tranche R11h (the steps reviewed one by one, merged together; every picture change opt-in until Thomas has seen it, §3's rule):

1. **The automatic exposure** (our own pass, Thomas 06:27; `rt-auto-exposure.flag`, opt-in): after the post passes and before the
   glare and the tone map, a compute pass makes a 256-bin histogram of log2 luminance (Rec. 709 weights) of the HDR the tone map reads,
   then one group solves and adapts on the GPU (no read-back on the frame's path): the bins between the 10th and the 90th percentile
   of the pixels (a trimmed mean, the sky and the darkest corners left out), the exposure factor `a` whose predicted displayed grey
   (the tone map's own curve, matrix and gamma from its `$Globals`, the glare and the grading left out) has the game's one target mean,
   found by bisection; adapted at 3 stops/s towards a brighter scene and 1 stop/s towards a darker one, exponentially within 1.5
   stops (Unreal's law as its documentation states it, its speeds taken as connaissance générale); snapped on a scene reset. `a`
   goes to the tone map as its t2 (Offset[2] = (1, 0, 1, 0), Offset[3].x = 1: the scene's and the glare's factor), the area's own
   exposure word kept (so the glare's bright pass and the effects keep theirs); DLSS's exposure texture takes the area's exposure × `a`
   of the previous frame. The target: one for the whole game (the coordinator's note, 22:28, on Thomas's standing rule « la règle
   universelle, pas de cas par cas », D-0263: no target by area), the original's mean displayed level (Thomas 06:22-06:27), the
   same trimmed mean of the luma of the original's pictures, averaged over the witness views. The original's pictures at the witness places are taken in the slot (Firelink's yard and corridor, the High Wall,
   Anor Londo's hall), the native's with and without the flag beside them; one row `native_auto_exposure` (target, `a`, the trimmed
   mean, the area's exposure) every 120 frames, read back off the frame's path.
2. **The sources in physical terms** (test tools, each a flag, then sheets): a reference sheet `LIGHT-UNITS.md` (the real values by
   family: the sun 100,000 lux, the sky, the moon, a candle about 12.6 lm (1 cd), a torch, a campfire; the conversions lm to cd; the
   inverse-square falloff with Frostbite's window at a radius found from the intensity instead of DS3's range; Frostbite's course
   notes could not be fetched (over 10 MB), their figures are connaissance générale until read); the DS3 values measured (the sun's
   and the sky's in the traced HDR, the local lights' colours by kind and radius from the collect's rows); the families named from what
   the collect can see (a light's owner, its radius and colour) before any rule; then the flags (the local lights at their physical
   ratio to the sun, the sky's, the emissive surfaces') and sheets with the automatic exposure on, the original beside them.
3. **Why the emissive lights little** (R11d's sheets: x4 and x16 changed nothing): read in the code and the material counts first;
   the candles' and fires' visible light are effects (not in the rays' scene) and point lights; an emissive surface is met only by a
   bounce ray that happens to hit it (no light sampling of emissive triangles); recorded with what would change it (the emissive
   triangles as light sources, ReSTIR's candidates) for R11c's lava and crystals.

Thomas chooses on the sheets (the default of each flag); the chosen settings by family to every zone, then R11c.

## 10. R11i: the Agility SDK by default, ReSTIR GI with the PT; then R11j, a TAA (the R pilot's fresh thread, 2026-11-10, from main `8f30a86a`)

Branch `claude/r11i-agility-default`; the defaults Thomas chose on R11h's opt-in list (D-0265).

1. **The pipeline states the Agility SDK 1.620's debug layer refuses** (RESULT « R11f »: « DSVFormat should be DXGI_FORMAT_UNKNOWN when
   depth-stencil is not used »): one rule where every graphics pipeline is made (PipelineStore::Acquire, the precompile's included): a
   depth-stencil format the description does not use (the depth and stencil tests off, the pixel shader writing no SV_Depth, read from
   its output signature) dropped before the description is named; a list's records made before the rule migrate at the precompile.
   Then the debug layer on the 1.620 runtime at several views (Firelink at the RT level, without RR, with frame generation, at the PT
   level; Anor Londo's hall; the High Wall at both levels) until no start closes; any other message it raises fixed or recorded.
2. **The Agility SDK by default**, `no-agility.flag` the test tool (`agility.flag` retired); WARP and PIX keep the inbox runtime.
3. **ReSTIR GI by default at the PT level** (`no-rt-restir.flag`), with a witness: its stability as R8d measured it (Get-Stability.py,
   ReSTIR against the one ray at Firelink and the High Wall), its cost (two vertices, -Measure), and the PT level asked without RR (its
   frames are the RT level's on NRD: the PT runs only where RR denoises).
4. **Not switched here: DLSS off by default.** DS3 has no temporal anti-aliasing (its filter's modes are spatial, AA-NATIVE-READY), and
   without DLSS the PT level does not run: Thomas's answer (00:55, D-0265) is a TAA as the default AA, DLSS the optimum; the default
   stays NVIDIA's DLAA until the TAA lands.

**R11j, the TAA** (next, before R11c: the tour judges the default picture, and R12's menu needs the TAA's On/Off): ER's TAA first
(D-0195: Elden Ring's anti-aliasing is temporal, connaissance générale to check in its 2.7.1 decompilation; its passes, history, rejection and sharpening recovered by static decompilation, read-only),
written natively on what the native already has (the velocity target, the camera's Halton jitter for DLSS, the history targets' frame
graph); AMD's FidelityFX (MIT, FSR's native-AA mode) only if ER's mechanism does not transfer, a download Thomas allows. Its place: the
end of the world view's post chain where DLSS runs now (before the glow and the UI), the same inputs DLSS reads; then DLSS off by default
(the game's Antialiasing option the TAA's On/Off); frame generation stays tied to DLSS, as RR (Thomas, 2026-11-10 01:05).

## 11. R11j: the TAA, the default anti-aliasing; DLSS off by default (the R pilot, 2026-11-10, from main `851ff840`)

Branch `claude/r11j-taa`; D-0265; the study [ER-TAA-STUDY](ER-TAA-STUDY.md) (a Sonnet worker, headless Ghidra on ER 2.7.1, read-only;
its raw outputs in `evidence/I-0132/artifacts/er-taa-study/`).

**ER's mechanism** (the study, OBSERVED unless said): the TAA is the fifth mode of `GXBS::GXAntiAliasingFilter` (TempAA, index 4; the
four others are DS3's spatial ones, so DS3's filter class is the place ER grew it); the main camera jitters by Halton (2, 3) minus 0.5
in pixels over JitterCount phases (8 by default), the jitter removed from the matrices the reprojection uses; the resolve runs on the
HDR scene colour before the tone map, reading the current colour, the depth, the velocity (INFERRED which target), the previous
history and a dilated motion vector (DilateMotionVector.ppo), into the new history of a ping-pong pair, then copied (or sharpened by
SharpenFilter.ppo, the history left unsharpened) into the scene colour; a feedback weight (0.75 the code's default, the shipped value
in GX parameters, NOT FOUND) scaled in four depth bands; the history seeded by a copy of the current colour on its first frame and
after a re-initialisation (no camera-cut test found).

**What DS3 does** (DS3's data, ER's architecture, D-0195): one compute pass `taa` where DLSS runs now (after the velocity, before the
YEBIS passes: the same inputs DLSS reads, the lit HDR, the depth, the native motion vectors, which are unjittered, previous minus
current in pixels), its output the post chain's source (the DLSS output's target, the same extent and format) and the history (a
pair); the motion taken at the 3x3's nearest depth (ER's dilation, inside the pass); the current colour reconstructed at the pixel's
unjittered centre; the history read with Catmull-Rom and clipped to the neighbourhood's YCoCg variance box; the blend weighted by the
tone map's exposure (bright samples weigh as dim ones: no flicker from fireflies). Better than ER (its known TAA defects are blur and
ghosting, connaissance générale): the variance clip and the dilation on every pixel, the history reset on the velocity's cut rule,
on a resolution change and whenever the previous frame was not resolved (ER seeds only at its re-initialisation). A sharpening pass of our own after the resolve, at ER's SharpenFilter's place (the history unsharpened), off by default as ER's (Thomas, 2026-11-10 04:28 to 04:30; R12's bar, D-0266).

**The option** (D-0265): the game's Antialiasing option on: the TAA, unless a DLSS mode is chosen (`dlss-mode.txt` dlaa, quality,
balanced, performance, ultra-performance; absent or « off »: none, the new default); DLSS chosen: DLSS as today (RR, the PT level,
frame generation only with it); the option off: no anti-aliasing. DS3's own spatial filter stays where neither runs (a frame without
velocity or lighting). `dlss-dlaa.flag` for the test runs that need DLSS (the PT level), `no-taa.flag` the test tool. The jitter runs
whenever the TAA or DLSS resolves the frame (8 phases for the TAA).

**Exit**: a unit test of the resolve (a still scene converges to the unjittered picture, a cut resets, an off-screen reprojection takes
the current colour); the TAA the default in the start check; the RT level's frames resolved by the TAA, the DLSS modes as before
with the flag; stills at Firelink and the High Wall, TAA against DLAA and against no AA, shown to Thomas.

## 12. R11c: the zones' tour (the R pilot, 2026-11-10, from main `5e793ead`)

Branch `claude/r11c-zones-tour`. Eta (the m40 zone save, `-Continue`) holds every bonfire of the game and both DLCs (the Travel page's
fourteen tabs, `i132-r11c0-eta-tabs`), so every zone of §4 is one travel away: [Run-R11cTour.ps1](Run-R11cTour.ps1) travels (the RT
level, the arrival's look sweep), keeps the bonfire as a test save (`eta-r11c-<zone>`), then Run-R7aGi's views from it (the RT level
`taa`, the PT level `ptsrc` with DLAA, the original `orig`, 1600x900); [Get-R11cJournal.py](Get-R11cJournal.py) reads each journal's
faults, skips, arrivals, BLAS memory and rows; [Make-R11cSheet.ps1](Make-R11cSheet.ps1) makes one sheet a zone. The tabs (pilot units, y 191,
rows from y 280, 33 apart): 263 Cemetery of Ash, 323 High Wall (row 5 Oceiros: the Consumed King's Garden), 383 Undead Settlement,
443 Road of Sacrifices (row 5 Farron Keep), 503 Cathedral, 563 Catacombs of Carthus (row 4 Old King's Antechamber, row 5 Demon Ruins:
Smouldering Lake), 623 Irithyll, 683 Irithyll Dungeon (row 2 Profaned Capital), 743 Lothric Castle (row 4 Grand Archives), 803
Archdragon Peak, 863 Kiln, 923 Ariandel, 983 Dreg Heap, 1043 Ringed City.

## 13. R11b: shader execution reordering on the PT level's paths, with §2.4 (2026-11-10)

The device reports shader model 6.9 on the Agility SDK (`shader_model 69` in the backend's row since R11i). SER's reorder
(`dx::MaybeReorderThread`) runs only in a ray generation shader, so:

1. **The PT level's path programmes as ray generation shaders** (the `_path` builds: the reflections' kind 9, the GI's kind 10, ReSTIR's
   initial sample in `gi_temporal`), compiled `lib_6_9` into one state object on the reflections' root (the global root), dispatched
   over the extent; each writes its result into a scratch UAV target of its own, then a full-screen draw of the existing pipeline state
   (its stencil test and its additive blend unchanged) applies the scratch, so the picture's composition does not move.
2. **The reorder**: after the first hit and after each path vertex's trace (PathFrom), keyed on the hit's material record (its coherence
   hint), the hit's values kept in the thread's registers (no HitObject needed: the inline query's committed values are read before the
   reorder).
3. `no-rt-ser.flag` the test tool; measured by Run-R7aGi's `ptgpu` against it at Firelink, the High Wall and the hall (-Measure, the PC
   free), the GI's mean checked unchanged (Compare-GiMean.py); kept only if it saves time; the pixel-shader path stays where the device
   has no shader model 6.9 (the same picture, not a lighting stand-in).
4. **§2.4's optimisation pass** with R11e's and NRD's leftovers: the main view's point batches built for nothing, the regions' zeroed
   row and t4, the RR-less `trace` draw on unserved frames, R8c's grid's CPU vectors made each frame (I-0128 « R8c » L7).

Done for items 1 to 3 (RESULT « R11c and R11b »: 5.1 and 9.3 ms off the PT level's 1920x1080 frame at Firelink and the High Wall, the default; the PT GI without
ReSTIR, kind 10, not reordered). Item 4 and the open points: R11b's second tranche (RESULT « R11b's second tranche »: the light grid's scratch kept, SER's first
setup timed; the others read and left, each with its reason).
