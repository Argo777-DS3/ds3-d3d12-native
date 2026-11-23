# I-0126 result: L'illumination globale tracée (R7)

## Conclusion

Active since 2026-11-06 (Paris), the R pilot. The plan: [R7-PLAN](R7-PLAN.md), with Thomas's choices of 2026-11-06 (D-0252: RR the
optimum, NVIDIA's NRD without it, SHaRC the cache). R7a (the first bounce at the RT level, the fill lights removed on its frames,
D-0254) merged `24a23297`: luma +0 to +5 steps over the baked ambient, the relief kept, the Firelink veil left as it is (Thomas,
2026-11-07 01:37: « Ça marche ») until R7b measures the misses' share. R7b (SHaRC as the hits' ambient, the hits' emission)
merged `f0b6aea4` (start check `start-f0b6aea4` passed): written, reviewed and seen (the cells answer; their ambient within the launches' spread of
the baked one; the interiors brighter than the baked ambient; the Firelink veil not the GI's sky) (below). R7c (NVIDIA's NRD
denoising the GI on every traced frame without RR, DLSS off included; the SSAO removed, D-0251): written, reviewed and seen (NRD on
every GI frame of its runs, its GI 0.3 to 1.2 steps under RR's), merged `f03c4dfb`. R7d (ReSTIR GI at the PT level, on NVIDIA's RTXDI reservoirs, Thomas's choice A): RTXDI's resampling measured biased (+5.5 %), rewritten natively and unbiased over the picture (within 0.1 %), with a local residual of 1 to 2 % in some blocks (the same in two launches), and its frames less steady under RR than the one ray's: left to R8, opt-in (`rt-restir.flag`); on the way, R7c's frozen seed on NRD's frames and R6c's barrier mismatch fixed (below).

## R7a: the first bounce (2026-11-06)

- **Written** (`e1cdc2b6`, its step review's fixes `7e92cd0e`): on the main view's frames RR denoises with the local lights' pass
  and the rays' scene (RtGiFrame, plan key bit 51), pass 0 reads a neutral black for GB4 and the neutral white t33 (the SSAO stage
  not recorded); rt_reflect.hlsl's `gi` (RtReflections::EncodeGi) adds GB2 × (1 − F0) × L into light_targets[2] after
  GXLightCmbAlbedo, L one cosine ray's TraceRadiance (R6's hit lighting, not fogged since `07316a22`; the forward sky on a miss), else the region
  cubes, else the light set's hemisphere along the ray; `no-rt-gi.flag` keeps the baked ambient (R7a's comparison until R7c).
- **Step review** (fresh Opus, high, `19c5282e..e1cdc2b6`): H1, the `rt_gi` graph pass declared only on frames with a top level
  under one plan key (a plan mismatch would fault the renderer): declared on every GI frame. M1, the hemisphere rows used as a
  radiance (biased): U along the axis's side of the ray, D on the other, whose cosine mean is the rows' formula. M2, frames without
  a material table counted as GI frames: counted missed. M3, another view's lighting targets clearing the main view's mask key: kept.
  M4, the hit's emission the plan named: deferred (FC_EmissiveColor not read for the rays; R7-PLAN §1 and §4 amended). L1 GB0's
  normal unflipped, L2 the GI's own random salt, L3 a comment. Fine as reviewed: the estimator, GB2's linear view, F0, the set
  index, the origin, the pipeline, the views, the states and the restored bindings, the failure latch.
- **Seen** (`build-r7a2`, [Run-R7aGi.ps1](Run-R7aGi.ps1), the ASUS, offline, restored by hash, no fault; 23:12-23:22 and
  23:25-23:29): the GI ran (`native_rt_gi` frames 640 to 1,079 a run, none missed), RR on. The first series' GI runs after the
  first baked one ran without the GI (`$flags` is `$Flags` in PowerShell: the baked run's `no-rt-gi` stayed in the later runs'
  flags; LIVE-RUN-NOTES' known pitfall), so m30, m31 and m37 were run again (`i126-r7a2-*-gi`). Mean luma (signalstats YAVG, the
  window's 1280x720 crop), baked / GI: Firelink 50.2 / 58.7 and 49.2 / 56.3; the High Wall 53.3 / 60.0 and 71.6 / 77.5; the
  Undead Settlement 42.3 / 49.8 and 31.6 / 39.5; Anor Londo 112.4 / 114.8 and 90.7 / 93.2 (the third views lost: the Claude app in
  front). By eye (the pictures `evidence/I-0126/artifacts/pictures-r7a1/*-baked-top-gi-bottom.png`, sent to Thomas): flatter, a
  grey-blue veil over the ground, the contact shading of the cobbles and corners lost. Not accepted as a true light (D-0249).
- **Candidates** (INFERRED, not checked): DS3's eye fog applied along every bounce leg (RtForwardFog in TraceRadiance: the fog's
  colour added to the ambient of every surface whose rays meet distant geometry: the veil); the region cubes at their sharpest level
  for the misses; the hit's baked ambient on top of its direct light (the second bounce approximated by the baked one); no emission.

- **The diagnostics** (2026-11-07 00:26-00:58, `build-r7a4` and `build-r7a5`, the ASUS, no fault; luma as above, Firelink's two views
  then the High Wall's): the GI's legs unfogged (`i126-r7a3`) 58.7, 56.3, 59.5, 77.6: no change (the fog not the cause); the GI's
  hits without the fill lights, their baked ambient, the local lights or all three (`i126-r7a4*`): at most 2 steps darker (the
  bounce itself not the cause); the SSAO kept (`i126-r7a5ssao`) 56.4, 54.8, 58.7, 75.6; the pixel's fill lights off, the sets'
  lights 1 to 3 (`i126-r7a5nofillp`) 54.8, 51.2, 56.4, 73.6 (baked 50.2, 49.2, 53.3, 71.6). By eye (`pictures-r7a5/`, sent to
  Thomas), the fill lights off gives back a directional light (the character's and the cobbles' relief, a warmer light); the SSAO
  kept stays flat. INFERRED: the flatness is DS3's unshadowed fill lights, which the SSAO darkened in the hollows, lighting them
  fully once the SSAO goes; the remaining 2 to 3 steps the traced bounce's own. Asked to Thomas whether the fill lights go on the
  GI frames (D-0251; D-0247's « fill lights » may mean the maps' Omni points).

- **The fill lights removed** (D-0254, Thomas 2026-11-07 01:05; `0d140f73`, `build-r7a6`): on the GI frames the sets' lights 1 to 3
  zeroed in cbLight for the pixel and the hits; the diagnostics they replace removed. Seen (`i126-r7a6-*`, 02:19-02:26, the ASUS,
  no fault; the GI on 640 to 1,072 frames a run, none missed), baked / GI: Firelink 50.3 / 55.0 (its second view lost: the Claude app
  in front); the High Wall 53.7 / 55.7 and 72.8 / 74.3; the Undead Settlement 42.3 / 43.6 and 31.6 / 36.1; Anor Londo 112.4 / 113.7
  and 90.7 / 90.3 (the last two zones against `i126-r7a1`'s baked runs). By eye (`pictures-r7a6/`, sent to Thomas): the relief back
  (the character, the cobbles), a directional and warmer light at the High Wall, the shade a little softer than the baked one;
  Firelink keeps a slight blue veil from the sky through its open roof. Missing: the hits' emission (FC_EmissiveColor for the rays),
  the further bounces (R7b).

## R7a's final review (fresh Opus, xhigh, `main...47691252`) and its answer

- **H1**, the plan's interior with thin walls not run (no zone save in one): asked to Thomas whether it moves to R7b (below).
- **M1**, a latched-off traced sun leaving the GI's hits unshadowed: the GI needs the traced sun on for the session (RtGiFrame).
  **M2**, the forward rows keep their own cbLight and its fill lights: said in D-0254 and R7-PLAN §1 (until R9). **M3**, the plan
  and the CONTRACT behind the code: amended (R7-PLAN §1 « Also on a GI frame »). **M4**, a by-eye judgement as the conclusion:
  reworded. **L6**, the RESULT's stale bits, status.json: fixed. Left: L1 (a GI frame whose rays' scene has no top level loses
  its ambient: counted, never seen), L2 (the sky a GI ray meets still fogged from the surface: R7b's veil check), L3 (a shared
  bindings restore, the position reconstruction copied, the GPU cost unmeasured: no optimisation measured, D-0227), L4 (the two
  diagnostics kept for R7b), L5 (small comments: the missed counter's comment fixed).

## R7b: the radiance cache and the hits' emission (2026-11-07)

- **Written** (`03badfc9`, its step review's fixes `ab609748`; R7-PLAN §3 « As written »): SHaRC 1.6.5's headers read from
  `third-party/` (hash-pinned by the build, never in the repository); the cells hold their surface's ambient (the demodulation on,
  each vertex's direct light given only to the vertices before it), queried by `HitAmbient` at the reflections', the water's and
  the GI's hits in place of `RtAmbientOf`, the seed lerped out by the cell's samples; `cache_update` (one path a 5 × 5 block, three
  cells) and `cache_resolve` before the reflections on the GI frames (`RtReflections::EncodeCache`, graph pass `rt_cache`); the hits'
  emission (`g_EmissiveTexture` × FC_EmissiveColor, read in the collect as the admission writes cbMatDynParam +0x20) added in
  `HitRadiance`; the diagnostics `no-rt-cache.flag` and `rt-gi-no-sky.flag`.
- **Step review** (fresh Opus, high, `763edd9c..03badfc9`): H1, the emission's read overwritten by an older white stand-in
  (`{1,1,1,0}` for every emissive texture, harmless before HitRadiance read it): removed. M1, the update's vertices left out their
  specular toward the vertex before them (light that vertex receives, view-free): given. L1 stale comments; L2 the 160 MiB made
  whenever R6 ran (now a small stand-by buffer until the first GI frame); L3 the capacity written twice (now from C++ in
  `previous.w`); L4 non-finite or negative samples (clamped, finite, at most 256); L5 the fixed point's rounding (1e4: a known
  darkening bias on the dimmest deep bounces, kept and to watch in the interiors); L6 no clear on a warp (now after a 100 m jump).
  Fine as reviewed: SHaRC's call order and weights, the seed blend, the barriers and states, the roots, the constants' layout,
  the graph key, the licence rule.

- **Seen** (2026-11-07 04:05-04:32, the ASUS, offline, restored by hash, no fault; `build-r7b3`, the witness `build-r7b4`; the
  extended [Run-R7aGi.ps1](Run-R7aGi.ps1): modes `seed` (no-rt-cache), `nosky` (rt-gi-no-sky), the interiors `m40chair` and
  `m37hall` by teleport; runs `i126-r7b1-*`, `i126-r7b2-*`, `i126-r7b3w-*`; mean luma by signalstats YAVG on the 1280x720 crop of
  walk-1 and walk-2, a picture with the Claude app in front left out). The cache against the seed at the hits (gi / seed): Firelink
  61.4 / 61.1 and 57.7 / 57.1; the High Wall 63.8 / 64.1 and 80.6 / 79.6; the Undead Settlement 52.5 / 51.8 and 40.3 / 39.5; Anor
  Londo 118.5 / 118.0 and 96.2 / 95.7; Firelink Shrine's interior (by the candles) 38.9 / 38.4; Anor Londo's staircase hall 51.7 /
  51.1: −0.3 to +1.0 steps, within the launches' spread (the pictures' mean absolute difference about 2 steps between any two
  runs). The witness (`rt-cache-witness.flag`: a hit's ambient its cell's fill, red none to green full): green over every hit at
  Firelink and in the hall by eye, so the cells answer (INFERRED from the pictures, not measured by channel); their ambient is close to the lightmaps' (INFERRED: the lightmaps hold
  the indirect light the cache now traces). The interiors against the baked ambient everywhere: 35.9 → 38.9 and 46.6 → 51.7, no
  leak seen through the walls (by eye, `pictures-r7b1/`, sent to Thomas). The GI's misses at Firelink (gi / nosky): 61.44 / 61.43
  and 57.7 / 57.9, inside 38.9 / 39.0: no share; the blue veil is not the GI's sky (its source left: the specular ambient or the
  fog, TO CHECK). The emission: written, not isolated in a view yet. A process fault of the pilot: `Run-StartupTrace.ps1` edited
  (a flag added to its pattern) while the series ran (WORKFLOW forbids it); the runs after it were unaffected (no refusal, no fault).
  Thomas on the sheets (2026-11-07 05:03-05:05 Paris): « Pour l'instant c'est très bien »; the character darker than with the baked
  ambient: « c'était justement incohérent avant, comme s'il avait une lumière naturelle qui émanait de son corps » (the sets' fill
  lights and the instance's SH, D-0254).

## R7b's final review (fresh Opus, xhigh, `main...e481ca97`) and its answer

- No High. **M1**, the cache queried at sharp reflections and the water (cell-sized blocks where the guide says keep tracing): the
  answer weighted by the footprint against the cell (`f7d9ae8b`); not looked at live after the fix. **M2**, no evidence of the
  emission: its records now counted (`native_rt_materials` « emissive »); the plan's « a dark interior lit by its fires » left open:
  DS3's bonfire and candle flames are FFX effects with their point lights (INFERRED), the effects' emission is R9's, the point lights
  already lit the hits. **L1** a cell counted unlit past the horizon: the vertex ends the path; an unread hit after a cell still counts
  it (the GI's own zero, D-0197). The throughput's albedo saturated. **L3** Make calls MakeRoot; **L4** one constants helper; **L7**
  native 16-bit ops in the rays' gate; **L5**, **L6**, **L10** the documents. Left, recorded: **L2** a map change within 100 m keeps
  the old cells for about 10 GI frames; **L8** NVIDIA's notice beside a distributed module (none distributed yet; Streamline's the
  same); **L9** no unit test of EncodeCache (exercised live); **L11** the GPU cost unmeasured (D-0227); **L12** R7a's two diagnostics
  kept (cheap, for R7c's comparisons); **L5** of the step review: the fixed point's rounding bias.
- The fixes' step review (fresh Opus, high, `e481ca97..f7d9ae8b`): M-1, the guide's other condition (a segment shorter than its
  cell): the cell's answer ramped in over one cell of the segment (`7da67564`, a ramp chosen over the guide's hard cut); L-1 a
  comment; L-2 one saturated albedo in the update; L-3 the emissive count with the material; L-4 `native_16bit` in the device row.
  Its own step review (`f7d9ae8b..7da67564`): no High, no Medium; the ramp recorded as a choice, these lines written.

## R7c: NRD for the frames without RR, the SSAO removed (2026-11-07)

- **NRD** (R7-PLAN §7): NVIDIA's NRD 4.17.3 (cloned outside the repository with MathLib v11 and ShaderMake `18f5a344`, hash-pinned)
  compiled by the module's build from its sources (NRD's custom integration: its C++ static, the REBLUR_DIFFUSE programmes by the
  SDK's dxc, mapped by `shaderIdentifier`); no CMake and no ShaderMake executable. Claude Code's guard refused the build of code from
  outside even after Thomas's « Oui compile »: Thomas added an `allow` rule (WORKFLOW « Thomas's PC and tools »). `rt_denoise.cpp`
  records NRD's dispatches with the engine's root, heaps, textures and barriers, every dispatch and its constants checked and
  uploaded before anything is recorded (`9051ca36`, the step review's fixes `548cdb22`). `gi_signal` writes NRD's inputs from the GI's
  ray (`GiSample`, shared with `gi`), `gi_compose` adds GB2 × (1 − F0) × the denoised radiance; the Velo rows drawn before the lighting
  (their resolve after the forward rows); `RtNrdFrame`, plan key bit 52; the `native_rt_gi` row's `nrd_frames`.
- **The SSAO removed** (D-0251, `3caed429`, a Sonnet subagent's edit read by the pilot): its passes, programmes and pins, targets,
  history, plan, constants producers, collect, rows and tests (1,029 lines); the shadow mask t32 kept (`mask_stage`), t33 the neutral
  white on every frame; `no-rt-gi.flag` removed (the frames without the rays keep their baked ambient, without AO).
- **Step review** (fresh Opus, high, `93677ff0..e30a6459`): no High. M1, the velocity target left in PIXEL_SHADER_RESOURCE when the
  signal pass throws: restored. L1 the descriptor ring held by the slot; L2 the constants uploaded before recording; L5 stale
  comments; L6 MaskReady's unused parameter: fixed. Left: L3, the Velo rows before the lighting give DLSS and the motion blur the
  hidden object's motion behind a forward row that writes depth (a phantom, the water): not witnessed (INFERRED small); L4 the first
  NRD frame compiles about 13 compute pipelines (one hitch, then the pipeline cache).
- **Built** `build-r7c1` (`fb1ef176`), its unit tests passed. **Seen** (2026-11-07 08:05-08:20, the ASUS, offline, restored by hash,
  no fault; [Run-R7aGi.ps1](Run-R7aGi.ps1) `-Modes gi,nrd -Sizes 1280x720,1920x1080,1371x771`, runs `i126-r7c1-*`): NRD denoised
  every GI frame of the six RR-off runs (974 to 1,077 a run, `missed` 0, no `nrd_why`), at 1280x720 and 1920x1080. The 1371x771
  window was not taken (the game kept 1280x720; 1440x810 in the second runs, below). Mean luma (signalstats YAVG, the window's
  1280x720 crop of walk-1 and walk-2; the pictures with the Claude app in front left out), RR / NRD: Firelink 55.4 / 54.4 and 51.5 /
  50.8; the High Wall 57.5 / 56.6 and 74.1 / 73.8: NRD 0.3 to 1.2 steps darker, within the launches' spread. By eye
  (`pictures-r7c1/`, sent to Thomas): NRD's GI clean, no noise or blotch seen, a little darker on the candle-lit wall.

## R7c's final review (fresh Opus, xhigh, `main...1467d3c8`) and its answer

- **H1**, no GI on the frames without DLSS when the game's motion blur is off (no velocity input, so neither RR nor NRD, and nothing
  counted): the vectors made every frame while the GI is on and NRD not failed (`f4ac61cc`, the collect's `upscaler` condition).
  **M1**, an RR-off traced frame NRD cannot serve: counted (`nrd_missed`), it keeps the baked ambient (the fallback of the frames
  without the rays; R7-PLAN §7 amended). **M2**, the Velo rows before the lighting on every frame (DLSS, RR and the motion blur given
  a hidden object's motion under a forward row that writes depth): drawn early only on NRD's frames; on NRD's frames the exposure
  stays (the fixes' step review M-a: DLSS with RR off and the motion blur read it there; a fix needs a depth before the forward rows,
  left). **M3**, the local lights' always-white AO term: removed (the slot, the fetch, the constant renamed `pad`). **M4**, coverage:
  the second runs below. L1 NRD's ignored files in the pin; L2 the shared gates first (`RtGiPossible`); L3 the declared velocity read
  on the early return; L4 unused accessors; L5 comments; L6 the settings once; L7 `NVIDIA-NOTICE.txt` beside the module. Left: the 15
  SSAO rotation images still admitted (only the first is read), the original's AO pass observer, the SSAO constants producers without
  a test (L4).
- The fixes' step review (fresh Opus, high, `1467d3c8..f4ac61cc`): no High; M-a above; L-a the declared read on a throw, L-b comments,
  L-c one gate predicate, L-d the vectors stop once NRD failed, L-e a comment (`133370dd`); `rt_scene_test` after the AO term
  (`6375972e`). Built `build-r7c3`, its unit tests passed.
- **Seen again** (2026-11-07 09:09-09:20, the ASUS, offline, restored by hash, no fault; runs `i126-r7c2-*`, `runs-r7c2.log`): with
  Antialiasing OFF and Motion Blur DISABLE (no DLSS: `native_dlss_option` inactive), NRD denoised every GI frame at Firelink, the
  High Wall, the Undead Settlement and an interior (974 to 1,079 a run, `nrd_missed` 0); with RR off at the Undead Settlement and
  the interior, every GI frame too; at 1440x810 (a size the game keeps; 1371x771 was not) RR on and NRD both ran (1,074 and 1,076
  frames). The first AA-off attempt did not start (Run-R7aGi's graphics levels bound by position: fixed). By eye
  (`pictures-r7c2/`, sent to Thomas): the GI clean without DLSS (aliasing, no AA).

## R7d: ReSTIR GI at the PT level (2026-11-07)

- **Its source** (R7-PLAN §8): Thomas chose A (2026-11-07 09:59, « Choix A »): NVIDIA's RTXDI library, the commit RTXDI v3.1.0 pins
  (`NVIDIA-RTX/RTXDI-Library` `f12037fa8e97ebc08e9e3edfd2de528ed1772a4b`, 2026-09-03, the « NVIDIA RTX SDKs LICENSE »), its GitHub
  archive (154,557 bytes, SHA-256 F91DE92D…3F75) in `C:/Users/User/Tools/DS3-Decompilation/third-party/rtxdi-library-f12037fa-download/`,
  extracted for reading to `third-party/rtxdi-library-f12037fa/`; the build checks the archive's hash and extracts its headers into its
  output each build (as NRD's archives). Before the choice the pilot had read three of its headers through GitHub's API to weigh the
  option (deleted after, said to Thomas).
- **A defect of R7c found on the way** (OBSERVED in the code, its effect INFERRED): the rays' seed (`FillRtReflectFrame`'s `c.seed`)
  was `rr_serial_`, which only RR's frames advance (`RrFrame`); on NRD's frames (RR off, DLSS off included) every pixel drew the same
  GI direction, the same local light and the same cache-update pixel every frame, so NRD's temporal accumulation met frozen noise and
  the cache learnt from fixed 4 % of the pixels. R7c's « NRD 0.3 to 1.2 steps darker » may come from it (not checked). Fixed: the seed is
  the frame's serial on every frame (`rt_seed_serial_`; RR's frames unchanged, their serial already).
- **The bias check's tool** (`rt-gi-mean.flag`): on a GI frame each pixel's estimate of L (`gi`, `gi_signal`, `gi_shade`) added into an
  R32G32B32A32 target at the render extent (restarted on a camera, extent or serial change; in the plan key, bit 53), written beside
  every world picture as `-gi-mean.rgba32f`; [Compare-GiMean.py](Compare-GiMean.py) compares two of them (the whole picture's ratio,
  the ratio next to the reference's strong edges against elsewhere, the blocks' spread), checked on synthetic files.
- **Written**: ReSTIR GI as R7-PLAN §8's table (rt_reflect.hlsl's `gi_temporal`, `gi_spatial`, `gi_shade`; RtReflections::EncodeGiRestir;
  RtRestirFrame, plan key bit 54): RTXDI's temporal and spatial resampling with its ray-traced bias correction, the native bridge (the
  surfaces from the depth and GB0, kept a frame in a buffer of their own; the initial sample the GI's own ray; the visibility rays
  on the view mask with the alpha tests), the camera's motion only, no boiling filter, fallback or permutation sampling, a Jacobian
  outside [0.5, 2] not reused; `no-rt-restir.flag` then (the PT level's one ray; `rt-restir.flag` opt-in since the fourth series), `rt-gi-no-temporal.flag`, `rt-gi-no-spatial.flag`
  (the check's stages). Memory: the reservoirs 2 × 32 bytes and the surfaces 2 × 16 bytes a pixel (about 200 MB at 1920x1080, 350 MB
  at 2560x1440).

- **Step review** (fresh Opus, high, `080ba94d7..b9d2556b6`): no High. M1, the check's reference: the RT level lights the hits' local
  lights unshadowed and all at once, the PT level one with its shadow ray (another integral): the reference the PT level's one ray
  (`no-rt-restir.flag`). M2, the receiver: the initial ray leaves from the surface moved toward the eye, the target, the Jacobian and the
  shading used the surface itself (a contact-shaped bias): the rays' origin everywhere. M3, RTXDI's ray-traced normalization evaluates
  each domain's target without its Jacobian (INFERRED from the headers): stated, bounded by reusing only a Jacobian within [0.5, 2]. L1 the
  mean restarted on a frame that drew nothing and on a change of estimator; L2 the surfaces as uint4 (no float path for the packed
  normal); L3 an extent guard before the root UAVs; L4 the plan's motion and biases; L6 the row's buffer 1,024; L7 the buffers released
  when ReSTIR latches off. Left: L5 (the control equal within the LogLuv rounding: said), L8 (RTXDI's 32-pixel radius and thresholds
  fixed whatever the render scale: its defaults, kept).
- **The fixes' step review** (fresh Opus, high, `b9d2556b6..a51c4dbd6`): no High. M-1, the narrowed window not bias-free (RTXDI counts a
  domain by its own sample's Jacobian): both windows kept, [0.5, 2] the default and [0.1, 10] with `rt-gi-jacobian-wide.flag`, left to
  the bias check (INFERRED either way); L-a the plan's pass criterion the PT level's one ray; L-b the control equal within the floor
  (another launch's random numbers, the snorm normal); L-c the ReSTIR buffers bound only by its draws. Also: R6c's `graph_barrier_mismatch`
  row a frame (E's report: `use_state`, `env_regions`, `rr_specular_hit_distance`), RR's hit distance transitioned before the pass declares it.

- **The bias check, first series** (2026-11-07 11:10-11:30, `build-r7d5`, the ASUS, offline, restored by hash, no fault; runs
  `i126-r7d1-*` and `i126-r7d1b-*`, [Run-R7aGi.ps1](Run-R7aGi.ps1) `-Still`, 1280x720; `compare-r7d1.txt`; the means over 600 to 1,000
  frames of the camera still, against the PT level's one ray, `ptone`): the floor (a second `ptone` launch) 1.0002 at Firelink and the
  High Wall (blocks p5 to p95 0.990 to 1.010 and 0.999 to 1.002); ReSTIR without reuse (`pt0`, the tool's control) 1.0002 and 1.0001,
  the same; with RTXDI's resampling, the temporal reuse 1.0029 and 1.0056, the spatial 1.0095 and 1.0236, both 1.0552 and 1.0311 (blocks
  up to 1.28 and 1.37), the wide window 1.0515 and 1.0227: **biased, brighter**, over the whole picture, not along the shadow edges.
  `m40chair`'s teleport did not move the camera (its means Firelink's: 1.0550 the same view). The `graph_barrier_mismatch` rows gone
  (0 in the runs). Conclusion (INFERRED from the headers, the reviews' M3 and M-1): RTXDI's normalization counts a domain by its own
  sample, not the chosen one, so Z falls short. **Rewritten natively** on its reservoirs: the target with the visibility, 1/Z over the
  domains that could have produced the chosen sample (rt_reflect.hlsl's header): RTXDI's temporal and spatial functions no longer used.

- **The rewrite's step review** (fresh Opus, high, `35d4b4b04..dd90c8b09`): no High; the 1/Z form unbiased on still views as derived
  (the Jacobian's direction, M, Z's domains, the chosen domain without a ray, the history's invariant, the reprojection checked). M1, a
  miss's radiance read at its receiver (Unmet: its light set's hemisphere, its regions): the light set in the domain tests (kept in the
  reservoir's misc bits), the regions' few centimetres stated. L1 the visibility rays' margin stated; L2 a candidate of no weight never
  taken; L3 the neighbours read once; L4 the 32-pixel radius kept (its footprint follows the render scale).

- **The bias check, second series** (12:15-12:30, `build-r7d7`, the native resampling; runs `i126-r7d2-*`, `compare-r7d2.txt`): the
  whole picture against `ptone` 0.9996 to 1.0003 at Firelink, 0.9993 to 1.0000 at the High Wall, 0.9990 at the Undead Settlement
  (`pt`, both stages; the temporal and the spatial alone and the wide window the same), the floor 0.9998 and 0.9999: **no bias over the
  picture** (from 1.0552 and 1.0311). Next to the edges at most 0.9962. But some 32x32 blocks far beyond the floor (the High Wall 0.79 to
  1.34). **Third series** (13:00-13:03, `i126-r7d2b-*`, `pt` again; `compare-r7d3.txt`): the blocks' ratios of the two `pt` launches
  correlated 0.993 at the High Wall (0.70 at Firelink), the floor's with `pt` 0.08 and 0.26: **a local bias, not noise**, in the same
  blocks (the High Wall's far tower roof against the sky, Firelink's far right wall: distant surfaces whose rays miss). Cause (INFERRED, the
  rewrite's review M1): a miss's radiance is its receiver's (the forward sky fogged from it, else the region cubes at it or its set's
  hemisphere), reused as is at a neighbour metres away; or the hits' specular toward the receiver that traced them (that review's M1).
  Fix written (`896a13ce`): a reused miss takes q's own radiance along the direction to it (its flag in the reservoir's misc bits); the
  block correlation to run again, with `rt-gi-no-glossy.flag` (the GI hits without their specular, a diagnostic) to tell the two causes.

- **Fourth series** (13:33-13:46, `build-r7d9`, the miss fix and `rt-gi-no-glossy.flag`; runs `i126-r7d4-*`, `i126-r7d4b-*`;
  `compare-r7d4.txt`): against `ptone`, `pt` 0.9993 and 1.0000 at Firelink, 0.9996 twice at the High Wall; the High Wall's blocks now
  0.98 to 1.04 (0.79 to 1.34 before the fix), their rms 0.005 (the floor's 0.001), the two launches' blocks correlated 0.66 (0.42 at
  Firelink, rms 0.009 against the floor's 0.006): the far misses' bias gone, a residual local one of a percent or two left (INFERRED:
  the hits' specular, the cache's segment ramp, the visibility margin, the header's list). Without the hits' specular (`ptng` against
  `ptoneng`) the same rms: the specular not the residual's main part. **Its benefit, measured**: the displayed frames' change between
  two world pictures 300 frames apart (mean |ΔY| of 255, a still camera, after RR): Firelink `ptone` 0.65 to 0.68, `pt` 0.84 to 0.94
  (and without the specular 0.64 to 0.81 against 0.80 to 0.91); the High Wall (grass, trees and fire moving) 2.46 to 2.61 against 2.38 to
  2.88: **ReSTIR's frames less steady under RR, not steadier** (INFERRED: its reused samples correlated in time, which RR's history
  takes for signal). So, as R7-PLAN §4 asks, **ReSTIR GI is left to R8, opt-in** (`rt-restir.flag`), with its bias check and the
  native unbiased resampling ready; the PT level keeps the one-ray GI by default. R8's questions: the guides RR needs for correlated
  samples (or decorrelation: RTXDI's PT has a stage for it), the residual local bias, the GPU cost (unmeasured: about 7 rays a pixel,
  200 to 350 MB).

## R7d's final review (fresh Opus, xhigh, `main...43de99f2`) and its answer

- No High; the exit criteria met (the archive pinned and outside Git, ReSTIR opt-in, no cost when off but two root UAVs bound and
  uniform branches; the evidence's figures as quoted). **M1** STATE's rows: rewritten at the checkpoint. **M2** the same mismatch on NRD's
  frames (`use_state`, `rt_gi`, `velocity`, every R7c NRD run): the velocity target transitioned before the pass declares it, back on
  every path; not seen live yet (the start check runs RR). **M3** the env_regions fix lets the frame graph drive RR's frames again: in
  `i126-r7d4-m40-pt` the graph shadow's executed frames 1,361 of 1,368 (R7c's RR runs 0), its barriers in place of the hand ones on the
  passes added since R6c; seen at the PT level at 1280x720 without a fault; the RT level's RR frames and other sizes not run (the start
  check after the merge reads its counts). **L1** the old flag's name gone; **L2** RTXDI in NVIDIA's notice and the shader's header;
  **L3** R7-PLAN §8's Outcome says what changed from the design; **L4** the stability figures stored (`compare-r7d4-stability.txt`);
  **L6** the conclusion says the residual; **L8** ReSTIR's buffers released after 600 frames without it; **L9** the comments. Left,
  recorded for R8: **L5** the check's frames 597 to 1,004 (295 for one `ptwide`) against the plan's 1,000, no interior (the teleport
  did not move the camera), the edge band at Firelink covering 74 % of the pixels (the reference's noise: « not along the edges » untested
  there); **L7** the GI ray written twice (`gi`'s GiSample and RtRestirInitial); **L10** R7c's M2 (the Velo rows' hidden motion) and the
  plan's « Seen » row (three sizes, pictures) not done, ReSTIR being opt-in; **L11** R7d's evidence listed under « Evidence »; **L12** the seed fix not seen on NRD's frames;
  **L13** RTXDI's adoption recorded in D-0252's line (decisions/LOG). The answers' step review (fresh Opus, high, `43de99f2..926762fd`): no
  High or Medium; L-1 the NRD frames now driven by the graph too, not seen live (an NRD witness run below); L-2 the velocity's state on any
  exception; L-4 the old flag kept in the runner's cleanup list (older builds); L-3 the idle release counted in frames (600), kept.
- **The barrier fixes seen** (14:45-14:49, `build-r7d12`, the ASUS, offline, restored by hash, the debug layer on, no fault, no debug
  message; runs `i126-r7d5-*`, Firelink): RR off (NRD 1,067 frames of 1,067), the RT level with RR, Antialiasing OFF (NRD 1,075 of 1,075):
  0 `graph_barrier_mismatch`, the graph shadow's executed frames 1,222 of 1,229, 1,324 of 1,331 and 1,458 of 1,465 (R7c's: 0). The
  seed fix's effect on NRD's picture not measured.
- **Merged** `55a15049` (2026-11-07 14:55); the launcher rebuilt from `main`, its unit tests passed; start check `start-55a15049`
  passed (15:05-15:06, 315 world frames, 0 `graph_barrier_mismatch`, the graph shadow's executed frames 307 of 315).

## Evidence

- The runs' disposable caches (`evidence/I-0066/artifacts/i126-r7a*`, `i126-r7b*`, `i126-r7c*`: raw frames, pictures, journals): to
  delete (Thomas, 2026-11-07 09:58 Paris: « Oui on efface, plus tard »), keeping the run logs and `pictures-r7*` sheets.
- The builds `evidence/I-0126/artifacts/build-r7a1` to `build-r7a7` with their logs; the run logs `runs-r7a1.log` to `runs-r7a6.log`;
  the journals `evidence/I-0066/artifacts/i126-r7a1-*` to `i126-r7a6-*`; the pictures `pictures-r7a1/`, `pictures-r7a3/`, `pictures-r7a5/`,
  `pictures-r7a6/`.
- R7d: the builds `build-r7d1` to `build-r7d10` (each with its extracted RTXDI headers), the runs `i126-r7d1-*`, `i126-r7d1b-*`,
  `i126-r7d2-*`, `i126-r7d2b-*`, `i126-r7d4-*`, `i126-r7d4b-*` (their gi-mean dumps disposable once the figures are kept), `runs-r7d1.log` to
  `runs-r7d4.log`, `compare-r7d1.txt` to `compare-r7d4.txt` and `compare-r7d4-stability.txt`.
- [Run-R7aGi.ps1](Run-R7aGi.ps1) needs R7a's code and Run-StartupTrace's `no-rt-gi` (branch `claude/relaxed-bose-r7a` until R7a merges).

## Uncertainty / unresolved questions

- R7-PLAN §6's checks (the stencil bit 0x10, the lightmaps' sun, RR's diffuse hit distance) not run yet.
