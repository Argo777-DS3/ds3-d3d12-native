# I-0132 result: L'optimisation et le tour des zones en PT (R11), then R12

## Conclusion

Opened 2026-11-09 (Paris) by the R pilot from main `be57dca2`; the plan is [R11-PLAN](R11-PLAN.md) (no study thread report for R11).
The six RT-line investigations still active (I-0111, I-0114, I-0117, I-0118, I-0119, I-0125) closed the same day with Thomas's yes,
their open points in R11-PLAN §6 (read by a Sonnet worker, the rows R11 takes re-read by the pilot in each RESULT).

R11a (2026-11-09): Russian roulette past a path's first vertex takes 6.4 ms off the PT level's frame at two vertices (1920x1080
DLAA) and, INFERRED from a pair that mixed two views, about 24 ms at four; the GI's mean unchanged (1.0001) and the displayed frames as
steady; the default since. R9d's fill shadow cannot
share the GI's pass (the pass order) and costs 0.2 to 1.0 ms. The interior (Anor Londo's hall) costs twice Firelink at the RT level. R11g (2026-11-09): the rays' walk on its own worker beside the main walk takes
0.65 to 1.2 ms off the engine thread's collect (−20 to −25 %, Firelink, the High Wall, the hall), kept.
R11h (2026-11-09/10): an automatic exposure (our own pass, the original tone map's t2), one target for the game (0.20, the original's
mean level; D-0263), on by default at half strength (D-0264: each view keeps half its distance to the target, as between the
original's views); the local lights in physical terms built and shown, found ugly by Thomas, kept as test tools (DS3's x2 the default);
DS3 has almost no emissive surface (its fires are effects and point lights).
R11i (2026-11-10): the Agility SDK's runtime by default (the pipeline states its 1.620 debug layer refused cleaned by one rule where the
pipelines are made, no message in 9 debug-layer runs) and ReSTIR GI by default at the PT level (+5.0 to +6.5 ms, as steady as the one
ray at Firelink, a little less at the High Wall); Thomas's defaults D-0265 (a TAA next, R11j). The pipeline store's roots named by their
content (2026-11-10): the launcher's list precompiled 258 of 258 at its next start, none failed or compiled in play. R11j (2026-11-10): a
TAA the default anti-aliasing (ER's TempAA mode of its AA filter on DS3's inputs, 0.10 ms at 1920x1080; as smooth as DLAA on the High
Wall's branches, no trail seen in the turning films), DLSS only with a chosen mode; Thomas's sharpening off by default. R11c (2026-11-10): the tour of the
zones never seen natively (ten zones at eleven bonfires, both DLCs', the High Wall's and the Undead Settlement's water; on main's tree,
before SER) found no fault, no unserved frame and no wrong family; an arrival is now ready only once its walk met the map's pieces. R11b
(2026-11-10): shader execution reordering on the PT level's paths took 5.1 and 9.3 ms off a 1920x1080 frame in steady windows (Firelink 23.9 → 18.8, the
High Wall 27.5 → 18.1; Anor Londo's hall 45.1 → 27.7 in its first, loading window), measured on build-r11b2 before its state fixes, the
GI's mean within 0.2 %; the default. The merged tree's SER and arrival rule were checked on a few views only (below), not toured.
R12 (2026-11-10 to 2026-11-21, ROADMAP workstream 2): one settings file, our rows in the game's Graphics tab and Advanced page, every
setting live, the « Rekindled » title screen and the precompile page (sections by name); its last step (R12h step 2, 2026-11-21):
the native passes' 31 programmes compiled at build time, the same bytecode as at run time, so no HLSL compiles in the module any
more (on an empty pipeline store their pipelines are still made at first use, outside the page: carried). Closed 2026-11-21 with
workstream 2, the CONTRACT's success conditions met for R11 (R11a and R11c merged with their start checks, R11b done, R11d's
rework carried by ROADMAP workstream 12) and R12 run here by its plan (the CONTRACT's amendment); its leftovers carried by ROADMAP
workstreams 7 and 8.

## R11a (2026-11-09)

- **Written** (`0c8f501d`, behind `rt-roulette.flag` for the measure): in `PathFrom` (rt_reflect.hlsl), after a vertex's lobe is
  drawn and its throughput updated, the path goes on with the probability q = min(1, max channel of the throughput), its throughput
  divided by q, else ends with nothing (unbiased); its draw at the vertex's salt + 0x12; never on the reference's paths (C++ side and,
  after the step review's L2, the shader's `cache`); the `native_rt_level` and `native_rt_gpu` rows say it. The runners took the flag
  and Run-R7aGi.ps1 cost, bias and DLSS Quality modes.
- **Not done: the fill shadow folded into the GI pass** (R11-PLAN §2.3, H2): read in `scene_pass_lighting.cpp`, the fill shadow
  writes the mask before GXLightDeferredAO (pass 0) reads it, the GI adds into CmbAlbedo after pass 2; they share a few G-buffer
  loads. Its own cost below (0.2 ms at Firelink, 1.0 ms at the High Wall): no fold.
- **Step review** (fresh Opus, high, `2135640e..0c8f501d`): no High or Medium. L1 the plan named the opposite flag: R11-PLAN fixed
  (`a228dbbd`); L2 the shader trusted the C++ side to keep the reference out: `cache &&` added; L3 no floor on q, the dark paths'
  large weights (under ReSTIR GI the initial sample's weight is its reservoir's target): no floor, the stills below show no extra
  noise, ReSTIR GI being opt-in; L4 the rows say « roulette » from `rt_pt` without RR's readiness (as `level` does): `pt_frames` is
  the authority.
- **The slot** (02:25-02:52 Paris, the ASUS, the PC entirely free, `build-r11a1` = `0c8f501d`, `Run-R11aSlot1.ps1`, `runs-r11a1.log`;
  21 runs `i132-r11a1-*`, no fault, all restored by hash; Run-ZoneWalk -Measure, no debug layer). The camera's looks were refused in
  most runs (the Claude app in the foreground: LIVE-RUN-NOTES), so most runs held the bonfire's view; in `m40-pt4gpu` and
  `m40-pt2gpu` one look went through (its frames 1800 and 2100 show another view): their windows mix two views. The `m37hall` PT runs
  wrote no `native_rt_gpu` row (fewer than the first window's frames in the world, INFERRED): the interior's PT cost not measured.
  Get-R8Gpu `-Windows`, milliseconds, OBSERVED; the window per row (the final review's L4): the 1920x1080 PT rows wrote one window
  (the one ending at frame 1800, which Get-R8Gpu skips by default as holding the load) and use it; the RT rows and the smaller sizes
  wrote two and use the second (the first differs by at most 0.1 ms); so the RT and PT rows are not the same window:

  | Run (1920x1080 DLAA with RR unless said) | frame | GI | reflections | cache | DLSS-RR | fill shadow | probes |
  |---|---|---|---|---|---|---|---|
  | Firelink, RT | 13.5 | 3.7 | 2.4 | 1.1 | 3.4 | 0.17 | 0.08 |
  | Firelink, PT 2 vertices / with the roulette | 30.2 / 23.8 | 11.5 / 8.2 | 11.3 / 8.3 | 1.1 | 3.3 | 0.17 | 0.08 |
  | Firelink, PT 4 vertices / with the roulette | 50.5 / 26.3 | 21.3 / 9.3 | 21.7 / 9.6 | 1.1 | 3.3 | 0.17 | 0.08 |
  | the High Wall, RT | 16.6 | 4.8 | 3.3 | 1.2 | 3.3 | 1.0 | 0.20 |
  | the High Wall, PT 2 / with the roulette | 32.8 / 26.4 | 13.4 / 9.7 | 11.1 / 8.4 | 1.2 | 3.3 | 0.95 | 0.20 |
  | Anor Londo's hall, RT | 26.4 | 9.5 | 8.3 | 2.3 | 3.3 | 0.15 | 0.46 |
  | Firelink DLSS Quality (render 1280x720), RT | 7.4 | 1.7 | 1.0 | 0.6 | 1.8 | 0.09 | 0.07 |
  | Firelink DLSS Quality, PT 2 / with the roulette | 14.7 / 12.1 | 5.1 / 3.7 | 5.0 / 3.7 | 0.6 | 1.8 | 0.09 | 0.08 |
  | Firelink 1280x720 DLAA, RT / PT 2 with the roulette | 6.9 / 11.5 | 1.7 / 3.7 | 1.0 / 3.7 | 0.6 | 1.6 | 0.09 | 0.07 |
  | Firelink, a 1597x899 window (its chain 1440x810, 16:9 again), RT / PT 2 with the roulette | 8.2 / 14.1 | 2.1 / 4.6 | 1.3 / 4.6 | 0.7 | 2.0 | 0.10 | 0.07 |

  The roulette saves about 45 % of the second vertex at two vertices (3.3 ms of the GI's 7.3 and 3.0 of the reflections' 7.2 at
  Firelink, against R8c's one-vertex figures 4.2 and 4.1), less than H1 hoped: INFERRED the pixel shader's warps, which pay a trace as long as one of their pixels goes on. At four
  vertices it takes about 24 ms off (INFERRED: `m40-pt4gpu`'s window mixed the bonfire's view and another, the look having gone
  through in that run; its pair to time again): the PT level at its most then costs 2.5 ms more than two vertices with the roulette,
  3.9 ms less than two without it. The scene's build (`rt_scene`, I-0117's open point) took 0.27 ms at Firelink, 0.40 in the hall and
  0.52 at the High Wall; the water's pass (I-0125's) 0.19 ms at Firelink at 1920x1080 RT, 0.24 at PT 2 (none in view elsewhere). The hall's lights: 531 in
  the list (431 outside the grid), its longest cell 19 (Firelink 163 and 34): its cost is not the lights' loop alone (not separated).
- **The bias check** (the same build, Firelink, the camera still, 1280x720; `pt2mean` and `pt2meanroul`, Compare-GiMean.py at frame
  2700, 1,103 and 896 frames): the whole picture 1.0001 of the paths without the roulette, the 32x32 blocks p5 0.991, median 1.000,
  p95 1.0095 (R8's floor between two launches of one mode: 0.986 to 1.015). **The displayed frames' steadiness** (the mean |difference|
  of the display's luma between frames 300 apart, the same runs, four regions without the bonfire's flame or the character): 0.29 /
  0.29 on the far wall, 0.63 / 0.63 on the floor, 0.42 / 0.42 and 0.53 / 0.54 elsewhere (without / with): the same. Mean luma of
  the bonfire's view at frame 1800 (1920x1080, YAVG of 255): 63.84 without, 63.77 with (pictures `pictures-r11a1/`).
- **The default** (2026-11-09, the pilot's call: the estimator's mean and the frames' steadiness unchanged, R11-PLAN §2.2):
  `no-rt-roulette.flag` the test tool; the runners and Run-R7aGi's `*noroul` modes follow (`8c73f228`, `build-r11a2`: built, its unit
  tests passed; no live run of the final default, D-0227). Not done in R11a: §2.4's optimisation pass, moved to R11b.
- **Final review** (fresh Opus, xhigh, `main...8c73f228`): no High; the code found correct and unbiased (every return path, the draws,
  the bit, the rows' buffers). M1 STATE's next action gave the superseded order and M2 §2.4 left unsaid: rewritten (STATE, R11-PLAN
  §2, this RESULT); L1 `rt-roulette.flag` back in the runner's stale-flag cleanup list; L2 the four-vertex figure marked INFERRED;
  L3 the comparison's row corrected; L4 the window per row said; L5 the index row, status.json, the six RESULTs' first line, the
  index's separators, STATE's RT list, R11-PLAN's ER sentence (D-0236); L6 the scene's and the water's zones above; L7 the ReSTIR
  modes now take the roulette: said in Run-R7aGi.ps1. No code changed by the answers (documentation and a runner's list).

- **Merged** `50edc20a` (2026-11-09 03:20 Paris); the launcher rebuilt from `main`, its unit tests passed; start check `start-50edc20a`
  passed (03:40-03:42, the rays' self-test passed, restored).

## R11d (2026-11-09): no lighting cheat (D-0262)

- **Thomas's turn** (03:01 to 03:48 Paris, D-0262): after R11a's pictures, « je ne vois pas trop ce qu'apporte le RT/PT par rapport
  au raster »; then no lighting cheat any more (the fill lights, the baked ambient, the stand-ins), only the real sources adjusted;
  the light painted in textures left to T1; the frames the rays do not serve magenta in the test runs, and no fallback in the end
  (the arrival held, the walk asynchronous, a latched failure an error message); the Agility SDK allowed.
- **Step 1** (`6e74b4d9`; R9d's removal by a Sonnet worker, checked by the pilot): the sets' fill lights zeroed on every GI frame
  (no flag), R9d's fill shadow pass removed (shader, pipeline kind 11, flags, zone, rows, plan-key bit 55 free); the hits' ambient
  the radiance cache's answer alone (no RtAmbientOf seed, no footprint ramp); a ray meeting neither a surface nor the forward sky
  brings nothing (Unmet and Hemisphere removed) in every pass; every surface's lobe traced at both levels (no roughness cut, no
  regions' coverage); FC_ShadowColor's floor gone (`seen = s` in the hits, 0 for a probe's occluded sun, 1 in the shadowed sun
  pass's b8 row 29). Built `build-r11d1`, unit tests passed. **Its step review** (fresh Opus, high, `50edc20a..6e74b4d9`): no High;
  M1 a frame without a top level lost DS3's cubes along R (the fallback): restored for that case only; M2 the pass still gated on
  DS3's regions: removed; L1 `NativeLightingInput::shadow_colour` dead: removed (its row a witness); L4, L6 stale comments: rewritten
  (a Sonnet worker, comment-only, checked by the pilot's diff of the code lines) (`b4a868ad`). Recorded: the constant fields `cut`,
  `dominant`, `shadow_colour` unread (layout kept); ReSTIR's light-set packing and its similarity test (opt-in path) kept; the
  regions still uploaded for the non-water passes (small); the forward programmes' own FC_ShadowColor (I-0125 read (1, 1, 1, 0.005)
  at its site: no floor there); a traced frame without the GI gives its hits no ambient.
- **Step 2** (`7e63c601`): the sheets' test tools: `rt-local-x2/x4.flag` (the rays' local lights' colours), `rt-emissive-x4/x16.flag`
  (the hits' emission, HitEmission). **Magenta** (`13d2d5d9`): `rt-fail-magenta.flag`, a main-view frame without a traced sun while
  the rays are on cleared magenta after the composite, counted (`fail_magenta_frames` in `native_rt_gi`); Run-R7aGi's default flags
  and the start check take it.
- **The sheets** (04:15-04:55, the ASUS, no measure, `Run-R11dSheets.ps1`, `runs-r11d1.log`; 32 runs `i132-r11d1-*`, no fault, all
  restored; `build-r11a2` for « before », `build-r11d2` for the rest; `Make-R11dSheet.ps1`, `pictures-r11d1/`): the camera's looks and
  the teleports landed at different frames in some runs (the window's focus; the faster raster and RT runs reached a frame number
  earlier), so some tiles show another view (said to Thomas). Mean luma (YAVG of 255) of each run's picture, OBSERVED: Firelink's
  yard (frame 1500, one view) raster 57.2, before 63.8, RT 58.8, PT 58.0, PT local x2 73.6, x4 94.9, x2 + emission x4 73.3, x4 + x16
  94.7; Firelink's corridor (each run's last) before 40.9, PT 43.5, x2 53.6, x4 63.8; Anor Londo's hall before 55.5, PT 54.6, x2 67.8,
  x4 85.2; the High Wall (outdoors) raster 75.5, PT 74.2, x2 75.7, x4 76.8. The emission changes nothing: Firelink's rays hold 8
  slices with an emission of 1,258 (`native_rt_materials`): DS3's candles and fires are effects and point lights, not emissive
  surfaces (INFERRED from that count). Without the cheats the interiors keep their level (the cache's bounce), Firelink's far arch
  goes dark (its fill light gone), outdoors is unchanged (INFERRED, from runs some of which show another view). The sheets ran on
  `build-r11d2` (`7e63c601`), before the step review's answers (the regions' gate still on the specular, `b4a868ad`): their luma is
  that build's. **Thomas chose the local lights x2** (05:11), the default since `08715e4c`; then the sources in physical terms with an
  automatic exposure (R11-PLAN §5). The interior sheets to take again on the final tree before a setting goes to every zone.
- **Final review** (fresh Opus, xhigh, `main...834e04d8`): no High; seven Medium, answered in `08715e4c` (code) and the documents:
  M1 the magenta caught only a frame without a traced sun: it now tests the frame fully served (the traced sun, a top level with its
  tables, the GI's bounce, the native local lights' pass), gated on the rays asked at startup, its causes OR-ed (1, 2, 4, 8); M2 its
  count read from walks the start check never reaches: its own row `native_rt_fail_magenta` as the count changes (32 at most), parsed
  by the start check with the causes; M3 a GI frame without the GI's inputs lost DS3's fill lights and GB4 for nothing: both now only
  with the GI's inputs (else the fallback, magenta in the tests); M5 SHaRC's segment rule (not a cheat) went with the seed: kept, the
  cell's answer ramped to 0 over one cell for a contact (the footprint rule stays gone: a sharp reflection's hit reads its cell, its
  voxel steps accepted); M6 the forward rows' own cbLight kept the fill lights: removed at collect too (the session's GI asked); M7
  the sheets' build and the INFERRED tag: said above. **M4 recorded, not changed**: on a frame RR does not denoise (rr-mode off,
  no-rr, RR not ready: NRD's frames, not the default) every surface's specular is now one undenoised lobe sample (shimmer and cost);
  NRD's specular (REBLUR diffuse and specular) or that option's fate in R12's menu, to decide with Thomas. Lows: L1 the comments
  fixed; L2 the hits still evaluate lights 1 to 3 (zero) and the collect still makes the dominant rows (rt-view's only): left, small;
  L3 a GGX sample under the surface falls back to the mirror (a bias, now on every surface): left, recorded; L4 the documents fixed;
  L5 the counter moved private; L6 RR and frame generation may carry magenta one frame further: said in the tool's comment.
- **Its answers' step review** (fresh Opus, high, `834e04d8..08715e4c`): H1 the magenta row still sat inside the sampled walks: moved
  out, on every walk; M1 its statics in the thread-affinity allow-list; M2 the forward fill lights removed for the menu views too:
  the main view's collect only; M3 Run-R7aGi's `nosky` mode swallowed by a comment: restored; M4 cause 8 on views without any local
  light: only with lights to shade (cause 4 there stays: the GI needs the light list, a gap before R11d, now visible); L1 a GI
  exception frame counted as served: counted missed; L2 comments (`f708f132`, `f85bfe80`, a C4456 build error fixed on the way).
  **M5 recorded**: the x2 reaches the rays' list (the native deferred lights pass and the hits) only; the forward grid's and the
  fallback batches' lights stay DS3's x1 until the sources go physical. **The last step review** (`08715e4c..f85bfe80`): no High or
  Medium; its two Low comments fixed (`988a7e9f`). `build-r11d6` (`f85bfe80`) built, unit tests passed.
- **The final tree's sheets** (05:55-06:14, the ASUS, no measure, `Run-R11dSheets2.ps1`, `runs-r11d2.log`; 15 runs `i132-r11d2-*` on
  `build-r11d5` = `f708f132`, before the last step review's answers (the magenta row's sampling, H1): no fault, all restored;
  `pictures-r11d1/sheet-final-*`): Firelink's corridor, one view in the five tiles, mean luma raster 38.6, RT x2 51.7, PT x2 54.7,
  PT x1 42.4, PT x4 68.0; Firelink's yard (frame 1500) raster 57.3, RT 75.1, PT x2 73.3, x1 58.0, x4 97.1; Anor Londo's hall (the views
  differ) raster 45.9, RT 64.9, PT x2 72.8, x1 69.6, x4 90.0. No magenta in the pictures read (the rows not checkable on that build:
  H1). Sent to Thomas.

- **Merged** `56fe00e5` (2026-11-09 06:15 Paris); the launcher rebuilt from `main`, its unit tests passed; start check `start-56fe00e5`
  passed (06:40-06:42, 293 world frames, 0 magenta frame, restored). The disk rotation (`Clear-DiskCache.ps1 -Apply`): 16 build folders
  older than 48 h, 3.5 GB, 509.5 GB free on C:.

## R11e (2026-11-09): no fallback (D-0262)

The R pilot's fresh thread (branch `claude/goofy-dirac-9766a7`, from main `0777ba17`); the design read in the code first (R11-PLAN
§8: the walks of the arrival are not in any journal, the walks' rows being sampled every 600th).

- **Code** (one step, steps 1 to 5 of R11-PLAN §8): a skipped or failed walk hands its frame the last walk's scene of this load
  (`native_rt_last_scene`, dropped at a scene reset; `reused` in the walk row), the 30-walk rule removed; every main-view frame with
  the rays judged (R11d's causes; `RtJudged`), an unserved frame's world black after the post chain with the UI over it (magenta under
  `rt-fail-magenta.flag`); an arrival (the first walk after a scene reset) held black until the last judged frame is served and the
  scene's instances are steady (2 % over 250 ms and 10 walks), at most 5 s, the released frame restarting the histories (the velocity's
  reset), one `native_rt_arrival` row an arrival and the first 240 walks of the first three journaled (`native_rt_arrival_walk`); a
  traced pass's latched failure, rays unusable at startup, or unserved frames in a row outside a hold (120, then 2 s: the review's L5) close the game through
  `NativeRtFatal` (a `native_rt_fatal` row, the error box on its own thread unless the test runs' flag, then SC_CLOSE to the window,
  DS3's own quit); DS3's fallback removed from the main view (the fill lights and GB4 kept without the GI's inputs, the point batches
  without the native lights' pass, the region cubes along R and the water's cubes in `rt_reflect.hlsl`, `no-rt-scene.flag`,
  `no-rt-selftest.flag`, the runners' `raster` mode); the start check reports the arrivals and fails on a fatal close. Built
  `build-r11e1`, `build-r11e2` (`613a0a24`), unit tests passed.
- **Its step review** (fresh Opus, high, `feadd908..613a0a24`): **H1** a main view without any local light had no light list, so no
  local lights' pass, no reflections and no GI: unserved, black, then the fatal close (or a flicker at a light's fade edge): such a
  view now gets an empty list, served (the lights' pass shades nothing; the reflections and the GI run). **M1** the steadiness needed
  40 walks a second: over max(250 ms, the last 10 walks' span). **M2** the 5 s clock started at the reset, used up by a long load:
  it starts at the arrival's first judged frame, and a held frame ends a run of unserved frames. **M3** the region cubes still uploaded
  (and a failure there now fatal) for nothing: `ProduceNativeEnvRegions` and the cube slots removed, one zeroed row keeps the root's
  t4 bound. **M4** the startup's fatal left the world lit by DS3's passes until the close: the frames judged (black) there too, and
  SC_CLOSE posted again every 10 s, three times. **L2** cause 1 fired wherever the Shadow Quality row gave a size of 0: the traced sun no
  longer gated by that row (R9c's rule), `no-sun-shadow.flag` removed. **L3** served on three walks in a row, past the arrival's fourth
  judged frame. **L4** a scene reset during a walk: its scene neither kept nor reused. **L5** the fatal run in time (2 s), not frames.
  Recorded: **L1** frames without a lighting input are not judged (none seen); **L6** the arrival's statics are the main collect's
  (one thread), outside the checker's scope; **L7** the release's history reset needs the velocity (no denoiser runs without it);
  **L8** a runner without `rt-fail-magenta` gets the error box on a fatal (the run waits for its bound); `Run-I117T0.ps1` still passes
  `no-rt-scene` (historical).
- **The answers' step review** (fresh Opus, high, `613a0a24..9ffa3f20`): no High; **M-A** the clock at the first lit frame left a hold
  whose world is never lit black for ever: 60 s after the reset without a judged frame, the fatal close; **M-B** the 2 s run could
  count a gap without judged frames after one unserved frame: 30 frames too; **L-A** the reset's arrival count in the same lock as
  the last scene; **L-B** the clock past the frames in flight (as the served test); **L-C** a walk counts toward the served streak
  only when a newer frame was judged; **L-D** SC_CLOSE every 30 s after the first three, for as long as the window lives; **L-E** the
  dead env-region types and `env_region_limit` removed, the stale comments rewritten (a Sonnet worker, comment-only, checked).
- **The last answers' step review** (fresh Opus, high, `9ffa3f20..92ccb4ae`) and **the final review** (fresh Opus, xhigh, `main...92ccb4ae`):
  no High; both Medium the same: the 60 s « never lit » rule counted the loading screen (walks without any main-view frame), so a
  60 s load would have closed the game: now 60 s and 600 frames held black without a judgement (`RtHeldUnjudged`, counted where the
  world is cleared), never a loading screen. Lows answered: the unserved run's 2 s measured to its last frame (`RtUnservedLast`); a
  reuse of the last scene stops after 2 s (`native_rt_reuse_stopped`: the frames unserved, the fatal rule then); SC_CLOSE only to a
  window of this process; the stale comments (a Sonnet worker, comment-only, checked); R11-PLAN §8 item 4 brought to the code (no owner
  for the box, the close after it, the 60 s and 30-frame rules, an optional pass's latch fatal by design). Recorded, not changed: a
  main-view frame without a lighting input is not judged outside a hold (none seen; judging it risks a wrong close where the main
  view draws unlit, not witnessed); the hold's bounds and the poll evaluated on walks only (no flow without walks found); the main
  view's point batches still built (for cause 8; R11b's optimisation pass); the regions' zeroed row and t4 left bound (R11b);
  `warp.flag` runs close at startup without DXR 1.1; the per-walk arrival journal kept for R11c's tour.
- **Their answers' step review** (fresh Opus, high, `92ccb4ae..0d5dcc2b`): no High or Medium; L1 the poll reads the run's last tick
  first; L2 the never-lit close keeps the hold (black until the window closes); L4 comments; L3 (4 s of skipped walks now fatal):
  every `native_rt_walk` row of the I-0066 artifacts' runs so far (4,884) has `skipped` empty and every sampled `skip_run_max` 0
  (OBSERVED): no skip run in normal play seen.
- **The witness slot** (07:33-07:42, the ASUS, no measure, `Run-R11eArrival.ps1`, `runs-r11e1.log`; 7 runs `i132-r11e1-*` on
  `build-r11e2`: Firelink, the High Wall, Anor Londo's hall, RT and PT, NRD at Firelink; no fault, all restored; the looks lost to the
  window's focus in five): no fatal close, no unserved frame outside the holds (no `native_rt_fail_magenta` row), OBSERVED. The
  arrival's walks (OBSERVED, `native_rt_arrival_walk`): the walks run through the loading screen without instances and without a lit
  frame (2.1 s at Firelink, 4.0 s at the High Wall, over 5 s at Anor Londo), the instances come in one walk, the first lit frame
  follows; from that first judged frame to the release 0.24 s at the High Wall (11 frames held) and 0.55 s at Firelink with NRD (29
  frames). The reset-started clock (M2) released four of the seven at `timeout`: Anor Londo's two and Firelink's RT before their first
  lit frame, Firelink's PT 0.4 s after it (17 frames held, its served streak begun): the case M2's answer fixes; the shipped rule has
  not run live yet (its first reading: the start check after the merge, a `ready` arrival and no `native_rt_fatal`).

- **Merged** `7a059f76` (2026-11-09 09:38 Paris, after Thomas freed the PC); `build-r11e6` (`d9d8a83a`) built, unit tests passed; the launcher rebuilt
  from `main`, its unit tests passed; start check `start-7a059f76` passed (09:45-09:47: 291 world frames, the arrival `ready` 5.1 s after
  the reset, its walks 130 to 161 past the loading screen, 31 frames held; 0 unserved frame, no fatal close, restored): the shipped arrival
  rule's first live reading. The disk rotation: 3 build folders, 0.7 GB, 505.7 GB free on C:.
- **Thomas** (09:23-09:24, on the noisy reflections without RR): « on avait dit qu'on utilisait un débruiteur intermédiaire de Nvidia »,
  « Oui NRD, qui s'active quand le RR est désactivé puis passe le relais au RR quand ce dernier est activé »: D-0252's NRD was wired to
  the GI alone (the reflections were not noisy before R11d); the specular goes through NRD next.

## NRD's specular (2026-11-09): the reflections denoised when RR is off (D-0252)

The R pilot's fresh thread (branch `claude/mystifying-kilby-04bb53`, from main `67f3105e`). Thomas (09:24): « Oui NRD, qui s'active
quand le RR est désactivé puis passe le relais au RR quand ce dernier est activé ».

- **Code** (`b0f21eaa`): NRD's instance is REBLUR_DIFFUSE_SPECULAR (one denoiser for both signals, its shared passes; NVIDIA's
  recommended form when both are traced, connaissance générale; its programmes the `NRD_SIGNAL=BOTH` permutations in
  Build-GameBootstrap.ps1). On an NRD frame (a GI frame RR does not denoise) one signal pass, `nrd_signal`, takes the reflections'
  slot before GXLightDeferredAO: the GI's cosine ray and the specular's GGX lobe ray (the same draws as `gi` and `trace`:
  `SpecularSample` factored out of `Trace`, which RR's frames keep unchanged), five targets (the diffuse L and its hit distance, the
  normal and the lobe's linear roughness, the view Z, the motion, the specular L before its term and its hit distance normalized at
  the pixel's roughness); NRD's dispatches right after; `spec_compose` writes the specular term × the denoised L into SpecularAcc
  with `trace`'s pipeline state (the stencil bit 0x08, R11G11B10); the GI's compose stays after GXLightCmbAlbedo, skipped and counted
  missed when NRD did not record that frame (`rt_nrd_recorded_`). The velocity target is read in the `env_regions` graph pass (moved
  there from `rt_gi`, transitioned before its declaration and back after, on every path). Failures: the signal's latches the GI (as
  R7c's signal did), NRD's refusal NRD (FailRtNrd), the compose's the reflections; each fatal (I-0132 R11e), the frame unserved for a
  refusal of the signal or NRD (its GI missing); a compose that throws leaves its frame without specular, uncounted, until the fatal
  poll (as on RR's frames: no « reflections » cause bit).
- **What it costs**: the specular's ray is the one `trace` already cast; the signal now covers every pixel with a surface (`trace`
  ran under the stencil bit 0x08 only), and REBLUR's specular half is added (not timed).
- **Step review** (fresh Opus, high, `3c48b525..b0f21eaa`): no High or Medium (the permutations, the views' indices, the barriers and
  the latches checked against NRD's `Reblur_DiffuseSpecular.hpp`). Answered in `d56a6ecd`: L1 an exception of NRD's recording itself
  left the lighting: now NRD's refusal; L2 the horizon factor's base saturated (a NaN from rounding, RR's frames included); L5 a stale
  name. Recorded: L3 on NRD's frames `nrd_frames` counts NRD's recordings and `native_rt_reflect`'s `missed` also counts a refused or
  latched frame (each already fatal or unserved); the GI's path there needs the reflections and the lights' pass too; L4 on NRD's
  frames the `RtReflect` GPU zone holds both rays and NRD's dispatches, `RtGi` the compose only (not comparable with earlier NRD zones).
  `build-r11n1` (`b0f21eaa`) and `build-r11n2` (`d56a6ecd`) built, unit tests passed.
- **The witness slot** (10:30-10:37 Paris, the ASUS, no measure, `Run-R11nNrd.ps1`, `runs-r11n1.log`; 6 runs `i132-r11n1-*`, still
  views at Firelink's bonfire and Anor Londo's hall, 1920x1080 DLAA; no fault, all restored): NRD's frames on `build-r11n1` served,
  OBSERVED (`native_rt_gi`: Firelink 1,074 GI frames, `nrd_frames` 1,074, `nrd_missed` 0; the hall 639 and 639, 0), no
  `native_rt_fatal`, no unserved frame, arrivals `ready`. The pictures (`pictures-r11n1/`, `Make-R11nSheet.ps1`: before, after, RR):
  mean luma within 1.8 of 255 of each other (Firelink 72.5, 72.8, 73.4; the hall 66.0, 64.2, 64.9, its views not identical); the
  floor's high-frequency level (the mean of the picture minus its Gaussian blur, σ 1.5, a 1400x380 band) Firelink 5.55 before, 5.52
  after, 6.21 RR, the hall 2.91, 2.74, 2.86: **on still views the change is within the noise** (DLAA's history averages a still
  camera's raw lobe samples); the shimmer Thomas saw is a motion's, which these runs do not film. What the slot shows: NRD's
  specular runs and serves every frame; its benefit in motion is Thomas's to see in play.
- **Final review** (fresh Opus, xhigh, `main...d56a6ecd`): no High. M1 the NRD path never ran and the start check (RR on) would not
  run it: answered by the slot above, and a start check without RR after the merge. Lows: L1 the GI's diagnostic flags reach the
  specular's rays on NRD's frames (diagnostics only: recorded); L2 the RR-less `trace` draw (kind 0) now runs only on frames judged
  unserved anyway: left to R11b's optimisation pass; L3 this RESULT's sentence on the failures corrected above; L4 the signal
  pipeline's name and L5 an unread GB2 slot fixed (`1d003487`; its step review, fresh Opus, high: nothing found); L6 and L7 this RESULT and
  STATE brought to the tree; L8 an exception inside NRD's recording leaves its pools' tracked states ahead (harmless: NRD stays off).

- **Merged** `f3a49990` (2026-11-09 10:40 Paris); the launcher rebuilt from `main`, its unit tests passed; start check `start-f3a49990`
  passed (11:20-11:21: an arrival `ready` in 3.9 s, 0 unserved frame, no fatal close, restored); the final review's M1: the launcher's
  module once without RR (`i132-r11n2-launcher-m40-nrd`, 11:21-11:22, `Run-R11nStart.ps1`): 1,073 GI frames, `nrd_frames` 1,073,
  `nrd_missed` 0, no fatal close, OBSERVED.

## R11f (2026-11-09): the Agility SDK; the walk and the top level read first

- **Read before writing** (R11-PLAN §8: « measured, kept only if it gains », « measured first, built only if it weighs »), OBSERVED in
  the journals already taken (corrected after the final review's M2): the rays' walk (`native_rt_walk`'s `ms_mean`, `i132-r11e1-*`,
  1920x1080, the debug layer on) costs the engine thread 0.7 to 1.2 ms a frame at Firelink and the High Wall and **3.0 ms in Anor
  Londo's hall** (2.98 in r11e1, 3.01 in r11a1); the `rt_scene` GPU zone, which holds all of `EncodeRtScene` (the top level's build
  among it: an upper bound), 0.27 ms of a 13.5 ms GPU frame at Firelink, 0.50 of 16.6 at the High Wall, 0.40 of 26.4 in the hall (1.5
  to 3 %; `i132-r11a1-*-rtgpu`, -Measure). Firelink's RT frames are paced at 60 Hz (13.5 ms of GPU in a 16.7 ms period), not GPU-bound.
  **The top level on async compute is not built** (it weighs 3 % at most: the plan's rule). **The walk asynchronous is the next
  tranche** (R11g), measured before and after as §8 asks: 3 ms in the hall is worth it wherever the engine thread limits (an uncapped
  rate, frame generation at the OLED's 280 Hz).
- **Code** (`101e78a1`): the device made through the Agility SDK 1.620.1 (`ID3D12SDKConfiguration1::CreateDeviceFactory(620, the
  module's D3D12 folder)`, the module being a guest in DS3's process: no D3D12SDKVersion export), its debug layer, GPU-based validation
  and DRED the factory's configuration; any failure keeps the inbox runtime (`agility_failure` in `native_device_caps`, with the
  runtime's `sdk_version`); `no-agility.flag` the test tool (since `acce49ee`: opt-in, `agility.flag`); WARP keeps the inbox runtime. GPU upload heaps (OPTIONS16) for the upload
  arena's chunks and the dynamic streams' versions where the device offers them (`no-gpu-upload.flag`). The build hard-links the
  package's x64 D3D12Core.dll and d3d12SDKLayers.dll (hash-pinned, signed by Microsoft) into the module's D3D12 folder; the play
  launchers copy it.
- **Step review** (fresh Opus, high, `f3a49990..101e78a1`): no High. Answered in `d073ec7d`: M1 the Agility SDK ships its own debug
  layer, so the layer could be on without the Graphics Tools' dxgidebug.dll and the DXGI debug factory fail: the factory now falls back
  to no flag (I-0082: asked for, never required); M2 `EncodeRtScene` read each row's `first` back from the arena (now video memory over
  PCIe): kept in a local; L2 the DXGI factory made again when the fallback changes the debug layer; L3 WARP named in `agility_failure`;
  L4 a GPU upload buffer starts COMMON (the build's unit tests failed on `build-r11f1` on the debug layer's « Ignoring InitialState »:
  the same finding); L7 no Agility SDK under `pix-capture.flag` (PIX with a factory's device not witnessed). Recorded: L1 the adapter
  probe stays on the inbox runtime; L5 the arena (up to 128 MB pooled) and the versions (64 MB) now count against the video memory
  budget, which trims only textures; L6 one process-wide upload heap type (one device in production; a test with two devices on two
  adapters would mix).
- **Final review** (fresh Opus, xhigh, `main...e31df593`): H1 the Agility path had never run while play would ship it: the witness
  below; M1 GPU upload heaps unexercised on the GPU: the witness and a measure below; M2 the walk's numbers (corrected above, the walk
  the next tranche); Lows answered in `21e042dc` (D3D12GetInterface looked up, not imported; a factory whose configuration throws keeps
  the inbox runtime; an ASCII SDK path, its short form otherwise; the adapter taken again by LUID after the DXGI factory's remake;
  `dxgi_debug` in `native_debug_layer`; stale comments; the runners' `noagility` and `rtgpunoup` modes); recorded: L2 the adapter probe
  on the inbox runtime; L8 the Agility SDK on by default although nothing uses SM 6.9 yet: made opt-in after the witness (below). Its answers' step review (`e31df593..21e042dc`): M1 a `std::string` from `wchar_t` iterators would break /W4
  /WX: narrowed explicitly; L1 the new flags in the runner's stale-flag cleanup (with `fg-on`, `fg-x3`, `fg-x4`); L2 the LUID from
  the adapter loop (`576c0410`). `build-r11f3` (`576c0410`) built, unit tests passed.
- **The witness slot** (18:00-18:25 Paris, the ASUS, `Run-R11fWitness.ps1`, `Run-R11fWitness2.ps1`, `runs-r11f1.log`, `build-r11f3`;
  all restored). OBSERVED: the measure runs (-Measure, no debug layer) and the inbox run opened their device as asked: `sdk_version`
  620, `agility_failure` empty, `gpu_upload_used` true (false under `no-gpu-upload`); `noagility`: 616, the inbox runtime, no fault;
  Streamline `ready`, DLSS ready, frame generation on (`fg`). **Every debug-layer run on the Agility runtime closed at start** (`gi` at
  Firelink and the hall, `nrd`, `fg`: `native_failure` 0xD3126607, « native startup diagnostic error: ID3D12Device::CheckFeatureSupport is
  superseded by ID3D12Device16::CheckFeatureSupport1 »): the Agility SDK's debug layer warns at each classic CheckFeatureSupport and the
  module counts any startup warning a fault. Fixed in `d09e1655`: that warning, by its text (its ID is past the Windows Kits headers'
  list), not counted (the module, Streamline and NGX query the classic way; the runtime answering is the SDK's). A first slot passed
  `-Seconds 40` (under Run-R7aGi's minimum): its debug-layer runs did not run and went into the second.
- **The second witness slot** (18:45-19:03 Paris, `Run-R11fWitness3.ps1`, `runs-r11f2.log`, `build-r11f4` = `d09e1655`; 4 debug-layer
  runs, all restored): the device on the Agility runtime (620, `dxgi_debug` true, frame generation on in `fg`), past the CheckFeatureSupport
  notice, then **every run closed at start on another warning of the Agility SDK's debug layer**, stricter than Windows' own: « The
  DSV format is (D32_FLOAT_S8X24_UINT) but the graphics pipeline state does not use depth or stencil ... DSVFormat should be
  DXGI_FORMAT_UNKNOWN » (a pipeline the inbox layer accepts). Decision (the pilot's, said to Thomas): **the Agility SDK opt-in**
  (`agility.flag`, `acce49ee`; the debug-layer run on the inbox runtime with GPU upload heaps, `i132-r11f1-m40-noagility`, had no fault): nothing uses it yet (the inbox runtime, 616 on this PC, offers the GPU upload heaps too,
  OBSERVED in `noagility`); the pipeline states the 1.620 layer refuses are cleaned when SER (R11b's candidate D) needs the SDK. The
  Agility path stays built, tested without the debug layer (the measure runs), its DLLs beside the module.
- **GPU upload heaps** (one pair each, -Measure, still views, `native_rt_gpu`'s means): Firelink's GPU frame 13.18 ms with, 13.29
  without (within the noise); Anor Londo's hall 24.90 with, 25.69 without (−0.8 ms: `rt_cache` 2.04 / 2.23, `rt_probe` 0.22 / 0.45,
  `rt_scene` 0.33 / 0.40), OBSERVED on one pair, not established; kept on (no cost seen).
- **Merged** `3364cb81` (2026-11-09 19:15 Paris); the launcher rebuilt from `main`, its unit tests passed; start check `start-3364cb81`
  passed (19:45-19:46: 352 world frames, an arrival `ready` in 2.9 s, 0 unserved frame, no fatal close, restored; the inbox runtime,
  GPU upload heaps on). The disk rotation (`Clear-DiskCache.ps1 -Apply`): 25 build folders older than 48 h, 4.9 GB, 503.3 GB free on C:.

## R11g (2026-11-09): the rays' walk beside the main walk

R11-PLAN §8 (« the walk asynchronous, within the frame », measured before and after, kept only if it gains), from main `5ad8e795`,
branch `claude/r11g-rays-walk-aside`.

- **Read first** (OBSERVED in `i132-r11a1-m37hall-rtgpu`): the hall's walk (3.0 ms) is the traversal 1.05 ms (one layer of the two holds
  nearly every entity: serial), the static instances 1.6 ms and the skinned 0.2 ms on the admission pool; it ran after the main walk and
  its admission, in the collect's section 1 (4.2 ms in the hall). The admission pool ran one job at a time: a second caller ran its job
  alone. The traversers only write themselves (I-0077's static reading: `C2630` its traverser's +0x13, `208180` no node job, `06F310`,
  `64F000`, `619800` reads, `D03050` its output; DS3's own pass scenes run their layer walks as parallel tasks on the GX workers,
  COLLECT-ORIGINAL-STATIC §0 point 4): two walks on the same layers at once are INFERRED safe, and the guard below compares both.
- **Code** (`e295000d`, its step review's answers below): the rays' walk posted first to its own worker (`collect_rays_worker`,
  `ds3-native-rays`) and joined after the lights, before the rays' forward blocks, its scene's first reader; never across the engine's
  next update (the join on every way out of the collect). The frame's traced flags (`sun_traced`, `local_traced`, the shadow frame's
  forced `on`) set before it, from the renderer's state as before. `WorkPool::Run` shared by concurrent callers: each worker joins the
  oldest open run once (its fixed index), a run returns when its caller and the workers that joined it have returned; a nested run still
  runs alone. The traversers' engine heap (the view's: their construction with the bounds stack's reserve, their destruction, the guard's
  serial walk) taken under one lock (its thread safety is not known). One difference: a light key the admission registers for the first
  time in a collect is found by the rays from the next one (set 0 for that instance one frame). `no-rays-aside.flag` walks inline after
  the admission as before (the runner's `inline` mode); `native_collect_sections` gains `rays_wait_ms` and `rays_span_ms`.
- **The measure** (20:30-20:47 Paris, the ASUS, `Run-R11gMeasure.ps1`, `runs-r11g1.log`, `build-r11g1` = `e295000d`, -Measure, 1920x1080,
  two rounds in turn; the looks were refused in seven of the twelve measure runs (and the Firelink guard run), the foreground lost to the desktop). Three pairs ran both modes
  alike (Firelink's round b and the High Wall's round a still, the hall's round b with its looks): −0.63, −0.78 and −1.32 ms; the other
  three mixed a still run with a moving one (the final review's L1). The `inline` column is R11g's build with `no-rays-aside.flag` (the
  new pool included): the hall's section 1 there, 4.0-4.2 ms, matches R11a's 4.19 on main (the final review's L3). The collect's time
  on the engine thread (`native_collect_split`'s `collect_ms`, the mean of the last five rows, 600 frames), OBSERVED:

| View | inline (no-rays-aside, rounds a, b) | beside (R11g, rounds a, b) | gain | the walk's own time, inline / beside | the join's wait |
|---|---|---|---|---|---|
| Firelink | 2.74, 2.69 | 2.08, 2.06 | −0.65 ms (−24 %) | 0.75 / 0.92-0.98 | 0.00 |
| the High Wall | 4.01, 4.08 | 3.23, 3.21 | −0.82 ms (−20 %) | 1.03 / 1.25-1.30 | 0.00 |
| Anor Londo's hall | 4.78, 4.79 | 3.67, 3.47 | −1.22 ms (−25 %) | 2.83-2.97 / 3.28-3.36 | 1.06-1.10 |

  The walk itself is 0.2 to 0.4 ms slower beside (the workers shared), the net gain holds in both rounds at the three views; in the hall
  the walk is the collect's longest part (the join waits 1.1 ms). The displayed frames are unchanged at 60 Hz (Firelink and the High Wall
  paced, the hall bound by its GPU): the gain is the engine thread's, for an uncapped rate or frame generation. **Kept** (the plan's rule).
- **The guard** (`guard`, record-revalidation.flag, the debug layer on, Firelink and the hall, on `build-r11g1`): both walks made again
  serially every 64th walk while they overlap, **no mismatch** (a mismatch always writes its row; the rows of checks 1 to 4 fell in the
  hall's arrival, 16 to 18 candidates, and its about 30 checks on the loaded hall, 4,742 instances, wrote none: the final review's L2); no
  fault, the arrivals `ready`, no fatal close. The live runs ran `e295000d`; the answers below were built and unit-tested, and the
  merge's start check runs the final tree (the final review's L4).
- **Step review** (fresh Opus, high, `5ad8e795..e295000d`): no High. Answered (`8bb73966`, `build-r11g2` built, unit tests passed): M1 the worker's wait throws at its 30 s
  deadline with the walk still running, which then wrote its outcome into the gone collect's frame: the outcome now shared with the job,
  and on a failed wait the worker is stopped (it waits for the job without a deadline) before the failure goes on; L1 the rays' guard
  restored the main view's tallies over the main walk's concurrent counts: the main walk's only; L4 the thread-affinity allow list and the
  walk's comments say the rays worker; L5 the effect record's open point (an engine write during Execute, a reader on another thread not
  known) said to extend to the rays' engine calls; L6 the pool's test with three callers of 300 runs each (every item done once).
  Recorded: L2 a bounds stack growing past its 64-level reserve on a worker allocates from that heap outside the lock: never seen (no
  `native_walk_bounds_stack_grown` row in the 1,461 journals of I-0100 on); L3 the oldest-first rule serves the rays' runs first: the
  measure above is net of it.
- **Final review** (fresh Opus, xhigh, `main...0f9ebecd`): no High, no Medium. Answered (`929d36f6`, `build-r11g3` built, unit tests passed): L1 to L4 the wording above; L5 a failed
  wait (the 30 s deadline) goes on at once to the refusal (which closes the process, its row and crash dump written) instead of stopping
  the worker without a deadline (a hang without either), the comments corrected; L6 the second one-frame difference written beside the
  first (a programme set the admission builds in the same collect built again by the rays or deferred: that instance without a material
  for one walk); L7 the heap's comment narrowed to the module's own calls, with DS3's own traverser clones on the GX workers (C1D10,
  COLLECT-ORIGINAL-STATIC §2) as the INFERRED support; L8 the stale « engine thread » wordings, `WalkNativeLayers` in the thread-affinity
  check's scope with its four counters reviewed; L10 the rays worker only with twelve hardware threads and up (as the recording pool;
  measured on sixteen); L11 the pool's test: a failure in one caller's run never reaches another caller's. L9 (STATE) at the checkpoint.
  Their step review (fresh Opus, high, `0f9ebecd..208afca9`): no High, no Medium; L1 the L11 test made to hold A open until B's run
  returned (so a failure kept for the next run to return would reach B), L2 the count of refused looks corrected (built and tested with
  the launcher's rebuild from `main`).
- **Merged** `b9f9dd37` (2026-11-09 21:50 Paris); the launcher rebuilt from `main`, its unit tests passed; start check `start-b9f9dd37`
  passed (22:03-22:04: an arrival `ready` in 3.0 s, no fatal close, restored). The disk rotation (`Clear-DiskCache.ps1 -Apply`): 2 build
  folders older than 48 h, 0.5 GB, 498.6 GB free on C:.

## R11h (2026-11-09): the automatic exposure and the sources in physical terms

R11-PLAN §9, from main `26902421`, branch `claude/r11h-physical-light`.

- **Step 1, the automatic exposure** (`dd58c89f`, `c8ca8174`; opt-in `rt-auto-exposure.flag`): AutoExposureStage
  (`native/renderer/auto_exposure.*`, kernels in `auto_exposure_shader.h`, the CPU mirror `auto_exposure_math.h`, the test
  `auto_exposure_test.cpp`): a 256-bin histogram of log2 luminance of the HDR the tone map reads, one group solving by bisection the
  factor whose predicted displayed grey (the original programme's curve, matrix and gamma from its `$Globals`) over the 10th-90th
  percentile pixels is the target, Unreal's adaptation law (3 and 1 stops a second, exponential within 1.5 stops), snapped on the
  velocity's reset; the factor the original tone map's t2 (its Offset words 752 to 756 set to (1, 0, 1, 0, 1) when it is bound); DLSS's
  exposure the area's exposure x the factor (the previous frame's, copied on the GPU); the frame graph's pass `auto_exposure` (plan key
  bit 55). One target for the whole game (the coordinator's note on Thomas's universal rule: no target by area): the original's
  trimmed mean displayed level at the witness views (Measure-AutoExposureTarget.py).
- **Step 2, the local lights in physical terms** (`6246d065`; test tools `rt-local-physical-10/-50/-200.flag`): RtPhysicalLight, one
  rule for every local light: the intensity at 1 m from DS3's colours (a point as it is, a legacy point and a spot without its
  ShadingMode matched at half range, x R² / 16), times the ratio to the sun (0.1, 0.02, 0.005 against DS3's x1), the falloff
  1 / max(d², 1 cm²) windowed by Frostbite's saturate(1 − (d / r)⁴)² at r = sqrt(luminance / 0.0019) (DS3's torch 8 m, the bonfire
  18 m) instead of DS3's cut, for the diffuse and the specular alike; the screen tiles and the hits' grid take r. LIGHT-UNITS.md: the
  units, the exposure rules, the real values and DS3's measured ones (DS3's local lights 100 to 1,000 times too strong against its
  sun, INFERRED).
- **Step 3, why the emissive lights little** (OBSERVED in the materials' rows): only 8 of Firelink's 1,258 traced slices name an
  emissive texture (all 8 with an emission), 5 of 8,395 in Anor Londo's hall, 16 of 4,708 at the High Wall (R11d's runs
  `i132-r11d2-*-gi`, `i132-r11d1-m30-gi`): DS3's fires, candles and
  bonfires are effects (not in the rays' scene) and point lights, so there is almost no emissive surface to light anything; the few
  there are reach the other surfaces only through a bounce ray that happens to hit them (no light sampling of emissive triangles). So
  R11d's x4 and x16 changed nothing. For R11c (Smouldering Lake's lava, crystals): the emissive triangles as light sources (sampled as
  the local lights are, or ReSTIR's candidates) if the tour finds large emissive surfaces; the EmiGlow family (FC_EmissiveColor x
  FC_GlowScale) keeps no material in the rays (R3c's list) and its count is not known.
- **The slot** (22:37-23:11 Paris, the ASUS, 1920x1080, no measure, `Run-R11hSlot.ps1`, `runs-r11h1.log`; 24 runs `i132-r11h1-*`, no
  fault, all restored by hash): phase 1 the original (Run-ZoneWalk -BootstrapMode Reference, the launcher's module) at Firelink's
  bonfire view (`m40`), the shrine's chair (`m40chair`), the High Wall (`m30`) and Anor Londo's hall (`m37hall`); its runs closed
  after about 75 s, their last shot the desktop (left out). The original's trimmed mean displayed level, OBSERVED
  (Measure-AutoExposureTarget.py): 0.09 to 0.37 by view (the shrine's interior 0.09 and 0.10, Firelink's bonfire 0.17, the High Wall
  0.19 to 0.27, Anor Londo outside 0.37 and the hall 0.14 and 0.23), their mean 0.20: the game's one target
  (`auto-exposure-target.txt` 0.20 in `build-r11h2`). Phase 2 on `build-r11h2` (`6246d065`: step 1 before its step review's answers,
  step 2): the PT level as it is (`ptsrc`), with the automatic exposure (`ptauto`), and with it the local lights in physical terms at
  0.1, 0.02, 0.005 (`ptphys10/50/200`); the sheets `pictures-r11h1/sheet-<view>-1.png` (the first shot of each run, the same moment).
  Mean luma (YAVG of 255) of those shots, OBSERVED: Firelink's bonfire orig 52.9, ptsrc 79.3, ptauto 53.7, phys 49.3 to 50.7; the chair
  orig 33.8, ptsrc 43.7, ptauto 49.8, phys 46.3 to 51.3; the High Wall orig 58.2, ptsrc 55.5, ptauto 47.0, phys 46.9 to 47.3; the hall
  orig 43.1, ptsrc 62.7, ptauto 59.6, phys 56.8 to 57.0. The pass converges on its target within a row (mean 0.2000 for 0.20 at Firelink;
  `native_auto_exposure`); the factor rises with the physical ratio where the local lights light the view (Firelink's bonfire: log2 a
  −0.93 at x2, +0.77 at 0.1, +1.38 at 0.02). Seen on the sheets (INFERRED from the pictures): with the automatic exposure the level is
  the original's where the view is average, the dark interiors come out lighter than the original's (one target); with the local
  lights in physical terms the sky's light dominates by day (the fires and candles no longer warm the walls, their flames turn to
  bright points, the hall lit by its door), little difference between 0.1, 0.02 and 0.005 where daylight enters; a dust effect in
  the hall turns white (the effects are scene-referred). `build-r11h2` logged two `graph_barrier_mismatch` rows a frame with the flag
  (the first step review's H1, answered in `44df7da6`).
- **The step reviews** (fresh Opus, high): the first (`269024219..c8ca8174`): H1 the frame graph's shadow mismatched every frame with
  the flag (the read declared before its hand transition, and PIXEL_SHADER_RESOURCE | NON_PIXEL_SHADER_RESOURCE wider than the
  composition's use): the read in NON_PIXEL_SHADER_RESOURCE alone, transitioned first; H2 the test's `near` (windows.h) broke the
  build; M1 the collect's two statics reviewed (thread affinity); M2 a black frame wound the factor up: the pixels not positive or below
  the range left out, the adaptation held under 1 % of the frame or on the tone map's FLT_MIN exposure; L1 DLSS's exposure guarded; L2
  the bins' UAV barrier; L4 the failure read after its atomic (`44df7da6`). Recorded: L3 the kernels compiled on the first flagged
  frame (a hitch once), L5 the 3584-byte block uploaded twice a frame, L6 the target measured on the encoded colour picture with its
  HUD, glare and grading while the pass predicts the grey of the luminance without them (biased, to calibrate on the native's own
  shots against the original's). The second (`c8ca8174..44df7da6`): H1 a comment in the kernel's raw string swallowed the state's two
  stores (`build-r11h3` only, not the slot's `build-r11h2`); M1 the physical lights' soft-shadow sphere and the hits' stop took the
  window's reach (a size change mixed into the sheets): DS3's range kept as a half float in the kind's bits 16 to 31; L1, L2 the
  physical row on the refused count, atomics; L3 the comments and LIGHT-UNITS; L5 the reach from the brighter colour (`12612bf8`,
  RtPhysicalLight's unit test; `build-r11h4`, unit tests passed).
- **The adaptation's strength** (`08041a2a`): the goal the strength times the solved log2 factor (the rest the area's own exposure).
- **The final review** (fresh Opus, xhigh, `main...a8732b1c`): no High; M1 the unit test never ran the kernels: `auto_exposure_stage_test`
  runs the stage on WARP against the CPU mirror (goal, factor, mean, the factor texel, DLSS's copy, the black frame's hold, a reset on
  a black frame), passed (`build-r11h6`); M2 the merged code had never run with the flag: slot 2 (below); M3 the target 0.20 in code,
  the file override dropped; M4 the single target against D-0262's last line: D-0263; L1 a reset on an empty frame waits for a lit one;
  L3 the globals' size asserted; L4 the 1 % test in float; L7 the documents (`3895f805`, `e709cf3d`). Recorded: L2 the slot's views
  rewritten each frame, the readback mapped each frame, the 3584-byte block uploaded twice (small, D-0227); L5 the
  `native_auto_exposure` row only from the rays' collect (every run has the rays since D-0253); L6 the physical tool's refused lights
  (those without colour, kept at DS3's falloff: on `build-r11h2` 19 to 23 of about 505 in the hall, the specular-only ones then; the
  reach from the brighter colour since `12612bf8`); L8 no unit test of a physical spot's record or reach. Its answers' step review: four
  Lows (the test's debug queue required, a luminance at a bin's centre, the pending reset's clearing checked, an uninitialised state on
  an empty frame kept pending), answered (`50dae922`).
- **Slot 2** (23:45-00:03 Paris, the ASUS, no measure, `Run-R11hSlot2.ps1`, `runs-r11h2.log`, `build-r11h5` = `08041a2a`): 9 runs
  `i132-r11h2-*` at Firelink's bonfire, the chair and the High Wall (full strength `ptauto`, half `pthalf`, half with the local lights at
  1/50 `pthalf50`), no fault, restored; no `graph_barrier_mismatch` row (the first step review's H1 answered), the factor kept and
  converged (`native_auto_exposure`). The three runs of Anor Londo's hall refused at their launch: the pilot changed Run-StartupTrace's
  flag list during the series (the game never started, nothing to restore; WORKFLOW: no script the series calls is edited while it
  runs). Mean luma of the first shots, OBSERVED: Firelink's bonfire orig 52.9, before R11h 79.3, full 53.7, half 65.2, half at 1/50 40.8;
  the chair orig 33.8, before 43.7, full 49.9, half 46.7, half at 1/50 30.0; the High Wall orig 58.2, before 55.5, full 47.7, half 51.0,
  half at 1/50 50.0 (`pictures-r11h1/sheet-half-*.png`).
- **Thomas's choices** (23:55 to 23:59 Paris, D-0264): the automatic exposure the default at half strength (« Je suis ta recommandation
  pour 1 et 2 »); the local lights at 1/50 accepted, then « je trouve les images moches » (the physical row: the fog and the daylight
  veil the lifted interiors, the walls and the player flat without warm light, the flames blown white, the pilot's reading at full
  size) and « Fais ça oui » to the pilot's revised recommendation: DS3's x2 kept, the physical ratios test tools
  (`rt-local-physical-10/-50/-200.flag`); test tools for the exposure: `no-auto-exposure.flag`, `rt-auto-exposure-full.flag`. The light
  sources' physical rework (by family, the flames as real sources, the fog lit by the real lights, new sources only where an object is
  one) a later workstream after R11c and the modern post chain (Thomas 00:01, RT-PT-PLAN §8).
- **The defaults' step review** (`e709cf3d..1d18a0ef`): H1 a comment in the kernel's raw string again swallowed two stores and the
  `if`'s brace (the kernels would not compile: every default run would have latched the pass off); M1 the start check blind to that
  latch; L1 the test's step on the wall clock; L2 stale comments; L3 a dead check: answered (`5f69a57c`: the comment on its own line,
  `auto_exposure_test` refusing any comment after code in that string, the start check failing on `native_auto_exposure_failed` and
  reporting the last reading, `fixed_dt` for the test); `build-r11h11` built, every unit test passed. Its step review: no High or Medium;
  recorded L1 the scan's needle from two literals (merged by /GF under /O2), L2 the other HLSL raw string
  (`scene_pass_velocity.cpp:49`) not scanned (its comment ends its line).
- **Merged** `50953fc6` (2026-11-10 00:31 Paris); the launcher rebuilt from `main`, its unit tests passed (`build-main-50953fc6.log`);
  start check `start-50953fc6` passed (00:33-00:35, 286 world frames, 0 magenta frame, the automatic exposure valid at half strength:
  log2 a −0.47 for a goal of −0.47, no failure; restored).
  The disk rotation (`Clear-DiskCache.ps1 -Apply`): 4 build folders older than 48 h, 1.0 GB, 486.2 GB free on C:.
- **Not done here** (R11-PLAN §9 step 2): the families named from the collect, flags for the sky and the emissive surfaces: deferred to
  that rework.

## R11i (2026-11-10): the Agility SDK by default, ReSTIR GI with the PT; Thomas's defaults (D-0265)

R11-PLAN §10, from main `8f30a86a`, branch `claude/r11i-agility-default`.

- **The pipeline states** (`ebbb1d40`, `e54abb7f`): the 1.620 debug layer's refusal (R11f: « DSVFormat should be DXGI_FORMAT_UNKNOWN when
  depth-stencil is not used ») answered by one rule where every graphics pipeline is made, PipelineStore::Acquire (the module's only
  production CreateGraphicsPipelineState, the step review checked): a depth-stencil format the description does not use (the depth and
  stencil tests off, the pixel shader writing no SV_Depth: its output signature read in the DXBC or DXIL container, OSGN, OSG5 or OSG1,
  by name and by system value; an unreadable container keeps the format) is dropped before the description is named; a list's older
  records migrate at the precompile (compiled under their new name, the old record replaced with its age, counted precompiled). The
  coordinator's second case (a draw whose pipeline declares a depth-stencil view none is bound to) was not met: no such message in any
  run below.
- **The defaults**: the Agility SDK's runtime (`no-agility.flag` the inbox runtime; WARP and PIX keep it, `agility.flag` retired);
  ReSTIR GI at the PT level (`no-rt-restir.flag`), so at its frames only, the PT level running where RR denoises; Run-R7aGi's R8 one-ray
  modes took `no-rt-restir` to keep their meaning.
- **The step review** (fresh Opus, high, `8f30a86a..ebbb1d40`): no High. M1 a draw with the dropped format while a depth-stencil view is
  bound never tried: the unit test now draws one; M2 the test never on the 1.620 layer, never on a DXIL container, the migration
  untested: the hardware profile now opens the Agility runtime from the build's D3D12 folder (sdk 620) and counts warnings at the
  pipeline's creation and the draw, two dxc programmes (ps_6_6, reflection stripped) read, a list edited as an older build wrote it
  migrated over two starts; L2 the migrated record's age kept; L3 the system value read with the name; L6, L7 stale texts and modes
  (`e54abb7f`). Recorded: L4 the rule's check twice for a migrated record (negligible); L5 DeviceQueues' WARP branch for the Agility folder
  kept as a guard (the call site no longer reaches it). `build-r11i2` (`bfc8dff1`): every unit test passed, the pipeline cache's on
  WARP's inbox runtime (616) and the hardware's Agility runtime (620).
- **The slot** (01:36-02:04 Paris, the ASUS, `Run-R11iSlot.ps1`, `runs-r11i1.log`, `build-r11i2`; 18 runs, no fault, all restored by
  hash; a first launch at 01:36 started no game: bash read `*>` as a glob and gave the runner a wrong screen key; in 7 of A's 9 runs
  Pilot-Game refused the looks, « foreground is not DarkSoulsIII (explorer) »: the views reached by their teleport, the walks partial).
  OBSERVED: A, the debug layer on the 1.620 runtime (`i132-r11i1-*`, 1920x1080 and a 1500x844 window): Firelink at the RT level, without
  RR (NRD), with frame generation, at the PT level; Anor Londo's hall; the High Wall at both levels: `sdk_version` 620 (shader model
  6.9), no NATIVE_DIAGNOSTIC, `debug_errors` 0, no failure; `noagility` 616 the same. The PT runs' ReSTIR GI on every GI frame
  (477 of 477 at Firelink, missed 0). B, the stability (Get-Stability.py, `i132-r11i2-*`, 1280x720, one launch each, Y of 255):

  | View, form | consecutive |dY| | per-pixel std, median | 300 apart |
  |---|---|---|---|
  | Firelink, ReSTIR GI (`ptr`) / one ray (`ptnr`) | 0.107 / 0.106 | 0.191 / 0.176 | 0.57 to 0.65 / 0.56 to 0.64 |
  | High Wall, ReSTIR GI / one ray | 0.254 / 0.227 | 0.347 / 0.336 | 2.16 to 2.96 / 1.91 to 2.18 |

  The PT level asked without RR (`ptnrd`): 2,876 NRD frames, 0 PT, 0 ReSTIR (the RT level's). C, the cost (-Measure -Still, the PC
  entirely free, Get-R8Gpu, 1920x1080 DLAA+RR, two vertices, in ms): Firelink frame 23.92 with ReSTIR GI, 18.93 without (GI 11.24 /
  6.10); the High Wall 27.50 / 20.95 (GI 13.74 / 7.20): **ReSTIR GI +5.0 and +6.5 ms**, R8d's +5.2 again. INFERRED: as steady as the one
  ray at Firelink, a little less at the High Wall (its moving grass and fire, one launch, no floor), unbiased (R8d), for 5 to 6.5 ms; said
  to Thomas, his choice applied and kept (02:12: « Non c'est bon »).
- **The final review** (fresh Opus, xhigh, `main...b8592e20`): no High. Answered (documents and runners only, the native code unchanged but
  one comment): M1 R8c's no-grid modes (`pt1gpunog`, `pt2gpunog`, `pt2meannog`) took `no-rt-restir` as their grid partners had; M2
  WORKFLOW's Agility row and this RESULT's uncertainty no longer say opt-in; L2 the `agility` mode and pattern word retired; L3 a duplicate
  cleanup entry; L5 the refused looks (above); L6 « as steady as the one ray » corrected (the comment, D-0265). Recorded: L1 the
  `native_rt_restir` row and the `native_rt_gpu` rows' `restir` say asked, not run (true at the RT level too: read `level` with it);
  L4 the test has no negative control on 620, does not assert the migrated age, and checks no warning on the kept-format pipelines;
  L7 a description with a cached blob would be normalised too (no caller); L8 only the pipeline cache's test opens the 620 runtime;
  L9 the list's migration has no live witness: the launcher's first start after the merge migrates R11h's list (its
  `native_pipeline_list_close` read at the start check).
- **Merged** `48e92b59` (2026-11-10 02:20 Paris); the launcher rebuilt from `main`, its unit tests passed (`build-main-48e92b59.log`, the
  pipeline cache's on 616 and 620); start check `start-48e92b59` passed (02:34-02:35, 338 world frames, `sdk_version` 620, the automatic
  exposure valid, 0 magenta frame, restored). The disk rotation (`Clear-DiskCache.ps1 -Apply`): 7 build folders older than 48 h, 1.7 GB,
  476.3 GB free on C:.
- **The list's migration at that start** (the final review's L9): `pipeline_store` 272 created, 261 precompiled of 272, **11 failed**
  (the earlier start checks 0). Read offline (a harness on `build-r11i2`'s objects with a temporary print in the precompile, not kept;
  the launcher's list after the start): every failure a record compiled but under another name, not migrated (4 compute pipelines among
  them), and two root objects each made from two different blobs (12,976 and 14,288 bytes; 76,036 and 78,704: DXIL programmes carrying
  their root signature, an older build's and the current's; the runtime returns one object for one root, on 616 and 620 alike). The
  store names a root by its object (`root_names_`), so the name follows which blob made the object last: a record of the other alias
  changes name at the precompile (counted failed, listed again under the new name), and a pipeline precompiled under one name can be
  compiled again in play when a frame's request names it by the other. INFERRED: an older defect of the store, not this step's rule
  (R11h's list precompiled 192 of 192 on 620 in the harness, no alias), exposed whenever a programme carrying a root changes; the next
  R tranche (a root named by its content, or the current build's blob preferred).
- **Thomas's defaults** (D-0265, 00:44 to 01:05): DLSS off by default once a TAA exists (the default the TAA and the RT level; NVIDIA's DLAA
  stays the default until then); the PT, RR and frame generation offered only with a DLSS mode; ReSTIR GI with the PT; Reflex and Boost
  off, Reflex's Off greyed while frame generation runs; no hidden option in R12's menu. R11j the TAA (R11-PLAN §10).

## The pipeline store's roots named by their content (2026-11-10)

R11i's last finding (« The list's migration at that start »), from main `09a34f64`, branch `claude/r11-pipeline-root-names`.

- **The defect** (R11i): the store named a root by the bytes of the blob that carried it and mapped each root object back to a name
  (`root_names_`); a DXIL programme carrying its root (the rays' passes) changes with its code, and the runtime returns one object for
  two blobs of one root, so the object's name was the last blob's: a record of the other name changed name at the precompile (11 of
  272 counted failed at `start-48e92b59`) and a pipeline precompiled under one name could be compiled again in play when a stage's
  blob gave the other.
- **Written**: a root is named by its content, the serialised root signature (the container's RTS0 chunk, `RootName`), whatever blob
  carries it (a bare serialised root is itself a container with that one chunk; a blob without it is named by its bytes, as before);
  Root returns one object for one content and keeps this build's blob for the list (an older build's programme no longer stays in it);
  an object keeps its first name (`NameRoot`), and one the runtime would return for two contents is counted (`root_aliases`, expected
  0), never renamed. The list's version 4; a version-3 list is read and renamed at Open: each root by its content, its records with it
  (two alias records made one, the younger age kept), then saved as version 4 (`roots_renamed`, `records_merged`). The counters in the
  `pipeline_store` array (positions 13 to 15) and in `native_pipeline_list_close` (with `precompile_failed`).
- **The unit test** (pipeline_cache_test T8, WARP and the hardware's Agility runtime): two dxc programmes embedding one root
  (`pipeline_cache_root.hlsl`): one object, a pipeline listed with either blob precompiled and then found whichever blob a stage gives,
  in both orders (nothing compiled in play), and a list written as version 3 named both blobs' roots (one record each): renamed at the
  open (2 roots, 1 record merged), precompiled without failure, saved as version 4 with one root, this build's blob.
  `build-r11roots2` (`a4aacd83`), then `build-r11roots3` after the step review: every unit test passed; the runtime returned one object for the two blobs on 616 (WARP) and 620.
- **The step review** (fresh Opus, high, `09a34f64..a4aacd83`): no High or Medium; the four criteria hold on reading (T8 fails on the old
  code: its order loop compiles in play). Answered: L1 the test kept red's blob by order alone: the version-3 roots now green first, so
  only Root's replacement saves red's; L3 a failed root creation erased a listed root: only an entry it made now; L4 stale comments
  (backend.h's naming, Encode, the precompile); L5 the container walk shared (`FindChunk`, PixelShaderWritesDepth's too). Recorded: L2 the
  alias of two contents only counted (`root_aliases`, read at the start check); the list's file keeps its name
  (`native-pipelines.v3.psc`), so a rollback to an older module discards a version-4 list once (one start without precompile).
- **The final review** (fresh Opus, xhigh, `main...3cd41e1d`): no High or Medium; the four criteria hold on reading. Answered: L1 a
  listed blob replaced did not mark the list unsaved, so a root a stage makes after the start's save (the sun shadow's, at its first
  traced frame) kept an older build's programme in the list: Root now marks it (once a root a start); L2 the comments said more than
  was witnessed (the runtime's one object observed for two DXIL programmes, not for a bare root against a programme) and the test's
  header still said one object per blob. Recorded: L3 the precompile always builds from green's blob (the other direction by
  symmetry), `root_aliases` never above 0 in a test, an alias's first name would follow the creation order (counted, not
  prevented); L4 STATE rewritten at the checkpoint. `build-r11roots4` (`5d19a3f0`): every unit test passed.
  Its step review (fresh Opus, high, `3cd41e1d..5d19a3f0`): no High or Medium. Recorded: L1 no test fails without the new mark (each
  replacement in T8 meets another mark); L2 a build giving two blobs of one content to two stages would rewrite the list at each flip
  (bounded by stage creations; no such pair today: each traced family has one programme).
- **Merged** `e193af30` (2026-11-10 03:52 Paris); the launcher rebuilt from `main`, its unit tests passed (`build-main-e193af30.log`); start
  check `start-e193af30` passed (04:09-04:10, 344 world frames, restored). The launcher's list migrated at that start (its
  `pipeline_store` row): 258 precompiled of 258, **0 failed**, 0 compiled by a frame's request, 13 records aged out, 28 roots renamed by
  content, 1 record merged, 0 root aliases (R11i's start: 261 of 272, 11 failed). No `native_pipeline_list_close` row in that run's
  journals (the audit row is the one read). The disk rotation: 5 build folders older than 48 h, 1.2 GB, 475.1 GB free on C:.

## R11j (2026-11-10): the TAA, the default anti-aliasing; DLSS only with a chosen mode (D-0265)

R11-PLAN §11, from main `851ff840`, branch `claude/r11j-taa`; ER's study [ER-TAA-STUDY](ER-TAA-STUDY.md) (a Sonnet `ghidra-research`
worker, headless Ghidra on ER 2.7.1, read-only).

- **ER's mechanism** (the study): the fifth mode of `GXBS::GXAntiAliasingFilter`, TempAA (the class DS3's four spatial modes live in),
  on the HDR scene colour before the tone map; Halton (2, 3) jitter over JitterCount phases (8 by default), removed for the
  reprojection's matrices; a ping-pong history (R11G11B10, INFERRED), DilateMotionVector.ppo, a feedback weight (0.75 the code's default,
  the shipped value in GX parameters, NOT FOUND) in four depth bands; a separate SharpenFilter.ppo, the history left unsharpened; the
  history seeded by a copy of the current colour at its re-initialisation (no camera-cut test found).
- **Written** (`taa_stage.{h,cpp}`, `taa_shader.h`): one compute resolve where DLSS runs (after the velocity, before the YEBIS passes),
  on the lit HDR, the scene depth and the native motion vectors, into DLSS's output target (the post chain's source; both never on one
  frame), the history a pair of RGBA16F targets the stage owns (its barriers inside it, held by the frame slot); the motion at the 3x3's
  nearest depth (ER's dilation inside the pass); the current colour at the pixel's unjittered centre (a Gaussian fit of Blackman-Harris,
  perceptually weighted); Catmull-Rom history clipped to the neighbourhood's YCoCg variance box (1.25 sigma); the history's weight 0.9
  at rest (ER's 0.75 code default; its shipped value unknown); reset on the velocity's cut rule, a new extent and any frame the TAA did not
  resolve before. Thomas's sharpening (04:28 to 04:30, off by default): a contrast-adaptive pass of our own after the resolve (ER's
  SharpenFilter's place, the history unsharpened), 0 to 1 by `sharpen.txt` until R12's bar (`sharpen-50.flag`, `sharpen-100.flag` the
  test runs). The material samplers' mip bias -0.5 on TAA frames (DLAA's -1 halved; ER's not found).
- **The option** (D-0265): DLSS runs only when a mode is chosen (`dlss-mode.txt` naming one, or a test flag, `dlss-dlaa.flag` new);
  « off », no file or an unrecognised word name none (`native_dlss_mode`'s `chosen`); the TAA when the game's Antialiasing option is on
  and no DLSS runs (`no-taa.flag` the test tool; its latch, `native_taa_latched_off`, stops it and the jitter); DS3's own filter where
  neither resolves a frame; the jitter 8 phases at the output extent for the TAA. The runners: Run-R7aGi's modes that need DLSS (the PT
  level, frame generation) take `dlss-dlaa` unless they name a DLSS mode; its new modes `taa`, `taas50`, `taas100`, `dlaa`, `dlssperf`,
  `dsaa`, `noaa`, and `-Video`; the start check reads the TAA's rows (its latch fails the check). Thomas's own play folder names
  `performance` in its `dlss-mode.txt`: his game keeps DLSS Performance.
- **The unit test** (`taa_test`, WARP, the debug layer): a uniform scene kept; a 32x32 checkerboard flipping each frame (a jittered
  edge's aliasing) resolved to a steady grey (its spread 0.207 to 0.014 after 40 frames); a reset and an off-screen reprojection the
  shader's reconstruction of the current colour; the sharpening (a uniform scene kept, the checkerboard's contrast raised, the history
  unsharpened); one +Inf sample bounded (its neighbours not blackened); a depth of another format refused by Prepare.
- **The step review** (fresh Opus, high, `851ff840..6323c47a`): H1 the scene's depth is typeless (R32G8X24, DepthTarget), which the
  stage refused, so every TAA frame would have faulted the renderer: accepted, the test's depth made typeless as the scene's; M1 a
  failed check or history inside Record faulted the session: a Prepare step before any barrier, its failure a latch; M2 a +Inf sample
  turned its 3x3 black and the history kept it: every sample and history tap bounded to [0, 65504], IEEE strictness at the compile, a
  test pixel; L1 the start check read no TAA row: added; L2 no mip bias on TAA frames: -0.5. Recorded: L3 the perceptual weights use the
  area's exposure, not the automatic exposure's factor (weights only); L4 the stage compiles its two programmes at its first frame
  (outside the precompile list, R12's precompile screen gathers the native programmes, D-0266); L5 `fg-mode.txt` or `rt-level.txt pt`
  without a DLSS mode change nothing, silently (D-0265's rules; R12's menu greys them).
- **The second step review** (fresh Opus, high, `6323c47a..ab1e3c1d`): no High; H1, M1, M2 and L2 fixed. Answered: M-a the start check
  passed with the TAA never run: it now requires the TAA's row when the Antialiasing option is on and no DLSS mode is chosen; L-a
  Prepare accepted a typed depth its view cannot read: the typeless only; L-b the test never read a history after a sharpened frame:
  a sharpened reset then a plain frame equals the plain pair; L-c a tiny sharpening value could write the output twice: below 1e-3 none;
  L-d `sharpen.txt` read silently: its value in the `native_taa_frames` row, a UTF-8 mark skipped. The sharpening's highlights:
  the final review's H1 (below).
- **The slot** (the ASUS, the OLED off, offline, every save restored by hash; [Run-R11jSlot.ps1](../../evidence/I-0132/artifacts/Run-R11jSlot.ps1);
  `runs-r11j1.log`, `runs-r11j2.log`): 04:58-05:29 (`build-r11j3`), A the stills at 1920x1080 (`i132-r11j1-*`: Firelink and the High Wall,
  `taa`, `taas50`, `taas100`, `dlaa`, `dsaa`, `noaa`), B the TAA at 1280x720, 1920x1080 and a 1500x844 window and the PT level with
  `dlss-dlaa` (`i132-r11j2-*`), D the films (lost: the looks refused, « foreground is not DarkSoulsIII (explorer) », Thomas then at the
  PC); 05:37-05:50 (`build-r11j4`) C the cost and the films again (no picture: a 1920x1080 window's rectangle, 1936x1100, outside the
  screen for gdigrab); 05:56-06:06 the films at 1600x900 (`i132-r11j6-*`, the looks after the recorder's start, Run-ZoneWalk). 39 runs,
  no fault, no TAA latch, no unresolved jittered frame; the TAA's rows on every TAA run, DLSS's on `dlaa`, `dlssperf` and `ptsrc`.
  OBSERVED: the stills ([sheets](../../evidence/I-0132/artifacts/r11j-sheets/)): the TAA smooths the High Wall's thin branches as DLAA
  does, DS3's filter and no AA leave them jagged; the sharpening at 0.5 and 1 crisper. The films' turns ([sheets](../../evidence/I-0132/artifacts/r11j-films/),
  `scripts/video/videosheet.py` for the turns): no trail behind the bonfire's flames, the water or the banners under the TAA, as under DLAA
  and DLSS Performance, judged on frames 3, 7 and 11 of the first turn (INFERRED from stills: a fast spell or effect not filmed; the
  effects' own motion vectors and the TAA's mask stay unwritten). The cost (`i132-r11j3-*`, -Measure -Still, rt-gpu-measure, the PC
  free): the TAA 0.10 ms at Firelink, 0.11 ms at the High Wall, 1920x1080 (the RT level's frame 12.6 and 14.9 ms there, its reflections
  9.2 and 11.4 ms without RR: NRD's frames, for R11b).
- **The final review** (fresh Opus, xhigh, `main...89287359`): H1 the sharpening clipped its result in tone space and inverted it,
  so a moderate highlight's edge rose to 1e4 / exposure (x2500 at 4 among 1s), into the glare, the motion blur, the DoF and the
  automatic exposure (only with the sharpening on, off by default; the slot's `taas*` stills carry it): the result now kept within its
  cross's range (no overshoot, as ER's SharpenRangeMin/Max bound its own), a test row (a highlight of 4 among 1s stays within [1, 4]) and
  the edge test bounded; M1 the unit test could not fail on the resolve's key mechanisms: added the motion's sign (a square moved 3
  pixels), the clip's colour inverse (a red history under a green frame), a vertical edge under the 8 Halton phases converging to the
  unjittered one; L1 the start check passed on one TAA frame: it now fails beyond two unresolved jittered frames; L3 R11-PLAN said no
  sharpen pass; L4 Get-R8Gpu lacked the `taa` zone; L6 `sharpen.txt` with a French decimal comma read as 0. Recorded: the dilation's
  nearest depth not tested (a depth step); L2 the weights' exposure the area's (above); L5 the DLSS and TAA blocks' shared transitions
  not factored; L7 no row says the PT level or frame generation needs a DLSS mode (R12's menu greys them).
  Its step review (fresh Opus, high, `89287359..3a7d8daf`, with a Python mirror of the shader's math): H1 bounded (the clamp in tone
  space, the inverse tone monotonic; the mirror on T8: [1.000, 2.418] against [0.904, 9.52] before). Answered: H-a the new edge test
  could not pass (its outer pixel's cross touches pure black, where the sharpening's amplitude is 0 by design): the outer side must
  only not rise; M-a the jittered edge's convergence passed with a reversed or ignored jitter (the history averages the phases): a
  reset frame drawn under phase 6's jitter, a vertical and a horizontal edge, must equal the jittered reconstruction mirrored on the CPU;
  L-a the unresolved-frames row only at the first and every 600th frame: written at the third too, so the start check's « beyond two »
  holds; L-b T9 passed with the motion ignored: x 14 must follow the square too; L-c a stray first Record removed; L-e the test's header.
  Recorded: L-d with the sharpening on, a pixel past about 1e4 / exposure leaves the pass at that value (lowered, not raised).
  `build-r11j6` (`106f2cd3`): every unit test passed, the TAA's as the mirror predicted (T4 0.844 to 0.945 inside, 0.044 outside
  unchanged; T8 within [1, 2.418]; T9 0.844; T11 the edge row 1.000 0.998 | 0.292 | 0 0; T12 within 0.0002 and 0.0003).
  Its step review (fresh Opus, high, `3a7d8daf..106f2cd3`): no High or Medium; each wrong jitter (x or y ignored, reversed or swapped)
  fails T12 by 10x its tolerance at least. Recorded (test text only): `current_j`'s comment says any colour (grey ones only), T9's line
  prints x 11 alone, the PASS line does not name T12, T4's outer pixel only checked not to rise.
- **Merged** `1087d877` (2026-11-10 07:55 Paris); the launcher rebuilt from `main`, its unit tests passed (`build-main-1087d877.log`); start
  check `start-1087d877` passed (08:11-08:12, 356 world frames, the TAA on: its row at the first world frame, no latch, no unresolved jittered frame, restored). The disk rotation (`Clear-DiskCache.ps1 -Apply`): 7 build folders
  older than 48 h, 1.7 GB, 453.6 GB free on C:.
- **The integrity review** (D-0268, run after the merge, the rule being new: a fresh Opus, `fa858ad8...claude/r11j-taa` and the roots'
  `09a34f64...e193af30`): no high finding; every change to a judge (the start check stricter, Get-R8Gpu's zone, Run-ZoneWalk's looks,
  the runners' modes, the T4 bound) stated with its reason above. Corrected here: (M) the live runs predate the final tree: the stills'
  `taas50` and `taas100` carry the unbounded sharpening (the final review's H1), so « crisper » holds for that pass; the bounded pass
  and the merged tree were seen live only by the start check below; (M) « no trail » in the films is a visual judgement on three
  frames of each turn, the modes' camera angles not matched, not a measure (no slit-scan or difference against a still control);
  (L) the « 1500x844 window » ran at 1440x810 (its `native_taa_frames` row): the window the screen allowed; (L) since R11j every
  Run-R7aGi mode without a DLSS word (`gi`, `rtgpu`, ...) runs the TAA on NRD where it ran DLAA with RR before: the earlier `rtgpu`
  figures are not comparable; (L) the sheets' tiles: `r11j-sheets/*-aa-crop.png` top row TAA, sharpening 0.5, 1, bottom row DLAA,
  DS3's filter, no AA; `r11j-films/*-turn-sheet.png` rows TAA, DLAA, DLSS Performance, columns three moments of the first turn.

## R11c and R11b (2026-11-10): the zones' tour; shader execution reordering

R11-PLAN §12 and §13, from main `5e793ead`, branch `claude/r11c-zones-tour`; one tranche (the tour's runner fixes, the arrival rule, SER).
The ASUS alone, the OLED off, offline, every save restored by hash; slots 08:24-08:28, 08:34-10:47 and 10:47-13:56 (the coordinator's
leave, Thomas asleep); logs `evidence/I-0132/artifacts/runs-r11c0.log` to `runs-r11c10.log`; sheets `evidence/I-0132/artifacts/r11c-sheets/`.

- **The way there**: Eta (the m40 zone save) holds every bonfire of the game and both DLCs (its Travel page's fourteen tabs,
  `i132-r11c0-eta-tabs`; R11-PLAN §12 lists them), so no progression was needed. [Run-R11cTour.ps1](Run-R11cTour.ps1) travelled to
  ten zones at eleven bonfires (Farron Keep, the Catacombs of Carthus, Smouldering Lake's Old King's Antechamber and Demon Ruins,
  Irithyll Dungeon, the Profaned Capital, the Grand Archives, the Consumed King's Garden (Oceiros), the Kiln, the Ringed City's streets,
  Ariandel's Snowfield; the Dreg Heap, seen before, not visited) and to the High Wall's and the Undead Settlement's other nine bonfires, the RT level with a look sweep, each bonfire kept as
  a hash-recorded test save `eta-r11c-<zone>` (labels and hashes in `runs-r11c1.log` to `runs-r11c3.log`); then from each the PT level
  (`ptsrc`, DLAA) and the original (`orig`) at 1600x900 with three looks (`i132-r11c4-*`, Farron and Carthus `i132-r11c5-*`), all on
  build-r11c1 (main's tree: no SER, the former arrival rule). Not looked for in R11c (R11-PLAN §4's « on the way »): RR's local-light
  noise (ReSTIR DI's condition), the lit cumulus and a size off 16:9; Thomas's judgement of the sheets pending.
- **Faults** (Get-R11cJournal.py, a Sonnet worker's reader, on every run): none. Every run closed by its close box or by the controller's
  stop at its time limit (exit 0xD3126612, I-0099), no
  `native_rt_fatal`, no unserved (magenta) frame, no failed walk or BLAS, no latch. Skipped objects: four `nonfinite_placement` at Ariandel
  and at the Consumed King's Garden, and at the Undead Settlement's Undead Settlement bonfire (`i132-r11c3-us2-travel`) (skipped by design,
  FINISH-LINE line 1). The rows common to every run (`native_offscreen_hair_refused`
  x60, `native_asm_part_missing_*`, `native_selected_object_unsupported` x8) come from the Firelink load before the travel, not from the
  zones. The rays' memory (the last `native_rt_blas`, the PT views): 2.9 to 19.0 M triangles, 85 to 583 MB compacted (the High Wall's water
  view the largest); the travels' journals up to 17.8 M triangles (the High Wall's, the Grand Archives' 17.2 M).
- **The arrivals** (`native_rt_arrival`, the travels): ready after 1.75 to 6.1 s; the Ringed City's timed out (6.7 s, its map's instances 0
  for the hold: the map parts walked only after it, then 2,025 instances); three of twenty were released « ready » on 7 skinned instances,
  their map not yet walked: Ariandel's, the High Wall's Vordt bonfire (`i132-r11c3-hw3-travel`, 2.5 s) and the Pit of Hollows (`-us5-`, 1.75 s). **Fixed** (`12_scene_collect.inc`, NativeRtArrivalStep): ready only once the map's own instances are in the scene
  (`walk.map > 0`: the walk's map pieces, not its static objects or skinned instances; the final review's M2), else the timeout's rule (a
  mechanism, not a zone: D-0161); checked on two travels with the merged tree (below).
- **The pictures** (OBSERVED, the sheets `i132-r11c4-<zone>.png`, the original's row above the PT level's; run pictures are JPEG screen
  captures): at every zone the PT level's frames are close to the original's in their light's placement and level; the interiors lit by
  fires (Carthus, Smouldering Lake, the Demon Ruins, the Profaned Capital) take the fires' warm colour where the original keeps a cool grey
  ambient (the traced light has no ambient but the bounces: D-0262, expected); Ariandel's snow a little greyer and darker under its fog.
  No surface family found wrong by eye (no control view needed); the light painted in the textures stays T1's (R11-PLAN §4).
- **The water** (R11-PLAN §6, I-0125): the Undead Settlement's is in view from the Dilapidated Bridge and the Pit of Hollows bonfires
  (`i132-r11c3-us4-*`, `-us5-*`: `native_rt_water` frames +600 a row after the arrival); the High Wall's from none of its four bonfires,
  so by teleport to the m30 MSB's player start `c0000_0003` (-89.76, -48.58, 425.36): traced (`i132-r11c4-m30water-*`, 406 to 1,422
  frames). A Sonnet worker's MSB read (SoulsFormats): the MSB frame is the teleport frame at m30 (four arrivals within 0.16 m of the
  `c0000_*` parts), bonfires are `o000100`; the 2026-10-30 falls came from teleporting to map pieces' origins.
- **The runners** (changes to tools that judge or drive runs, D-0268): Pilot-Game.ps1 brings the game's window to the foreground before
  an input (AttachThreadInput, then, when Windows refuses it, a click on the game window's own caption, checked to be its; the input itself
  still refused unless the window under it is the game's): the Claude app, then explorer, held the foreground through the first series and
  every look was refused (`i132-r11c1-*` views lost). Run-R7aGi's runs last 150 s by default (60 s closed a zone's views before their
  looks: a zone's load is longer than Firelink's), its `-ExtraSaves`, the `m30water` view and the SER modes; Run-StartupTrace's flags
  take `no-rt-ser`. Both changes were made between runs of a series (the edit atomic, the file replaced whole), against WORKFLOW's « no
  script it calls is edited »: said here; the runs after them are the ones read.
- **R11b, shader execution reordering** (R11-PLAN §13): the PT level's path builds of the reflections (trace_hit, kind 9) and of ReSTIR GI's
  initial sample (gi_temporal) also compiled as `lib_6_9` ray generation shaders (`trace_hit_ser`, `gi_temporal_ser`), one state object
  on the reflections' root, DispatchRays over the extent; TraceSurface reorders each thread by its committed hit's record
  (`dx::MaybeReorderThread`) before its material and light; trace_hit_ser writes a scratch (R11G11B10, R16) that ser_apply copies with
  trace_hit's own pipeline states (its stencil test). The PT GI without ReSTIR (kind 10) keeps its pixel shader (not the default).
  `no-rt-ser.flag` the test tool; `native_rt_ser` its row. **Measured** (`i132-r11b1-*`, -Measure -Still, rt-gpu-measure, 1920x1080 DLAA
  with RR, two vertices, the PC free, build-r11b2): Firelink 23.86 → 18.81 ms a frame (the reflections 5.81 → 3.49, the GI 11.15 → 8.34),
  the High Wall 27.46 → 18.13 ms at the same window, serial 2400 (6.90 → 2.86, 13.68 → 8.16), Anor Londo's hall 45.06 → 27.68 ms in
  both runs' first window (serial 2400, Get-R8Gpu -Skip 0: the load's window, which the reader skips by default; the run without SER, at
  about 22 frames/s, has no second one; the SER run's steady window reads 23.86). The GI's mean (`i132-r11b2-m40-ptr` against `-ptrnoser`,
  Compare-GiMean at frame 3600): 0.9980 of the pixel shaders', blocks p5 0.990, median 1.000, p95 1.011, worst block 0.756; on
  build-r11b3 (`i132-r11b3-m40-ptr` against `-ptrnoser`) 0.9993, worst block 0.937; wider than R8's floor between two launches of one
  ReSTIR-less estimator (I-0128: 1.0001, blocks 0.996 to 1.005 at about 1,800 frames) and no floor between two ReSTIR launches exists, so
  « unchanged » means within 0.2 % on the whole picture, not proven per block. The pictures alike by eye and by their mean luma (YAVG 57.47
  and 57.42 over the client, `i132-r11b2-m40-ptsrc` and `-ptsrcnoser`, walk-3; `r11c-sheets/i132-r11b2-m40-ser.png`). The default, kept;
  the merged tree's barriers (the lighting phase's NON_PIXEL states on SER frames) are not timed.
- **Its step reviews** (fresh Opus, high): `f80c76fe`: H1 the ray generation reads the depth and GB0 to GB3 while they are only in pixel
  shader states; M1 an SER preparation failure inside Encode latched the reflections (fatal); L1 an RGBA16F scratch double-rounded; L2 the
  row counted MakeSer, not dispatches; L3 the scratch kept while SER idles, L4 every pixel traced (the stencil not read in the ray
  generation), L5 kind 10 not reordered, L6 one instance enough for the arrival: recorded. Answered in `116649b4` (M1: prepared before any
  barrier, a failure the pixel shader and SER off; L1 R11G11B10; L2 dispatches), whose review (fresh Opus, high) found H1 not fixed: the
  frame graph's executor suppresses hand-written barriers of tracked resources. Its second answer (`71550dff`, build-r11b4: the two passes
  declared PIXEL | NON_PIXEL, the hand barriers back to PIXEL after each) failed in turn: its review and the live runs (`i132-r11b4-*`,
  1,279 to 1,471 `use_state` rows a run on the SER frames, none on the TAA's) showed the planner keeps a read state's superset (a later
  PIXEL read satisfied without a transition) while the hand barriers went back, so no SER plan was ever cached. **RCA** (two failed local
  fixes, WORKFLOW): the cause is the planner's rule (frame_graph.cpp: a read-only state is brought only when the current one does not
  include it), not the passes; so on a frame with SER the whole lighting phase now holds the depth's plane 0 and GB0 to GB3 in PIXEL |
  NON_PIXEL_SHADER_RESOURCE (set at its start, every lighting pass declaring it: shadow mask, the sun's traced shadow, the cache,
  env_regions, the lights, rt_gi, the SSS, the composite, RR's guides), and light_organize and the control view, which read the depth
  after it, take DEPTH_READ | PIXEL | NON_PIXEL on those frames (build-r11b5 left only light_organize's 4,122 rows). build-r11b6
  (`i132-r11b6-*`: Firelink, the hall and the High Wall at the PT level, Firelink's TAA): no `graph_barrier_mismatch`, SER on (4,342 to
  4,880 dispatches), no fatal row, no unserved frame.
- **Its third step review** (fresh Opus, high, `f559af83`): M1 the control view (rt-view.flag) still declared GB0 to GB3 NON_PIXEL on SER
  frames (the planner holds the superset); L1 rt_view and light_organize read the last lighting's member, not this recording's flag;
  L2 no unwinding of the phase on a throw (as the phase's end transitions before: the frame faults). Answered with the final review's.
- **The final review** (fresh Opus, xhigh, `main...f559af83`): no High. M1 as above; M2 the arrival rule counted any static instance,
  not the map's pieces; M3 Pilot-Game took a window by its title alone (an Explorer window on the game's folder has it) before its caption
  click; M4 the summary lines gave the merged default results measured on earlier builds. Lows: L1 as above; L2 two stale comments; L3
  Run-R7aGi.ps1 left in CP1252 bytes by the branch's edits (the `m40low` comment displaced); L4 « twelve zones » was ten at eleven
  bonfires; L5 two « on the way » items without an outcome; L6 STATE's next action and the logs' list stale; L7 the « same picture »
  evidence a whole-frame mean, the sheet's luma over the whole screen, no unit test of SER; L8 the reader misses a pass turned off; L9
  the state object compiled in play at the first SER frame (a first-PT-frame hitch, outside the precompile list); L10 SER the default on
  any shader model 6.9 device, measured on one GPU (an adapter that does not reorder pays the scratch and the apply draw); L11 each SER
  pass uploads its constants twice. Answered (`4a169175`, build-r11b8): M1 and L1 (GB0 to GB3 and the depth NON_PIXEL | PIXEL in
  rt_view on SER frames, light_organize too, both from `rt_ser_now_`); M2 (`walk.map`, the walk's map pieces, 0 on a reused or skipped
  walk); M3 (only DarkSoulsIII's window, the point checked again before the click); M4, L4, L5, L6 (this RESULT, the index, FINISH-LINE,
  STATE); L2; L3 (rebuilt from main's UTF-8 text); L7 the sheet's luma over the client; L8 the reader's « passes off ». Recorded: L7's
  unit test, L9, L10, L11, the control view's diagnostic only, the skinned positions left NON_PIXEL outside the diff (unchanged, not
  verified). Its step review (fresh Opus, high, `4a169175`): no High; M the place action's wait loop still took a window by title
  (fixed: one `Get-GameWindow` for both, `d8ffc8b0`); L the arrival row without `map` (added); L the sheet's crop at a fixed client origin
  (recorded).
- **build-r11b8's checks** (`runs-r11c10.log`): Firelink with rt-view.flag (40 control-view passes), the High Wall, the hall and a TAA run:
  no `graph_barrier_mismatch`, SER on (4,328 to 4,880 dispatches), no fatal row, no unserved frame. The travels to Ariandel and the Ringed
  City with the new rule: both arrivals timed out (6.8 and 6.7 s, the map's pieces not walked during the hold: their BLAS builds;
  Ariandel's held 334 frames black where build-r11c1's released on skinned instances at 1.9 s). So where the map comes late the arrival
  now holds its 5 s limit (the world black under the UI, the hold's design, R11e) where the former rule showed a world without its map;
  expected at the High Wall's hw3 and the Pit of Hollows too, not re-run. The map's BLAS latency at an arrival is the open point (the BLAS
  build's budget at an arrival, for R11b's next tranche). The control-view run also met `native_rt_skin_unsupported` x32 (o000250, `why:
  shape`), a skinned family the rays do not take, there only (recorded with I-0119's families).
- **The integrity review** (D-0268, fresh Opus, `main...d8ffc8b0` and this RESULT): no High; every runner and reader change stated, no
  test or gate touched, no scene-keyed code, no work cut in the measured pairs (same flags but `no-rt-ser`, same build). Corrected here:
  (M) three of twenty arrivals, not Ariandel's alone, were released on skinned instances; (M) the hall's pair is its first, loading window
  and the High Wall's gain at one window 9.3 ms; (M) the GI's mean is wider than R8's floor (its worst block, the b3 pair added); (L) the
  reader's fault words miss `native_device_removed`, `native_dred*`, `native_display_lost`, `graph_barrier_mismatch`,
  `native_video_memory_over_budget` and the record mismatches (none in any run: the mismatch counts above come from a separate grep) and
  count `native_pass_refusals` (`gates:{}`) as one; (L) the closes, the skips, the triangle range, the YAVG's source, the sheets (no sheet
  file for Farron, Carthus's re-run and the Pit of Hollows: they are in `r11c-tour-overview.jpg`), the skin row: corrected above.
- **Merged** `09e3f927` (2026-11-10 14:14 Paris); the launcher rebuilt from `main`, its unit tests passed (`build-main-09e3f927.log`);
  start check `start-09e3f927` passed (14:28-14:30, 283 world frames, the arrival ready, no unserved frame, restored; the RT level and the
  TAA, so SER is not exercised by it). The disk rotation (`Clear-DiskCache.ps1 -Apply`): 13 build folders older than 48 h, 3.2 GB, 425.5 GB
  free on C:.
  The row's field `dispatches`; the failure string's assignment cannot throw. Recorded: no frame-graph unit test pins the superset rule.

## R11b's second tranche (2026-11-10): the open points and the optimisation pass

R11-PLAN §13 item 4 with R11c's open points, from main `9610612a`, branch `claude/r11b-ser-tranche2` (the fresh thread waiting for
Thomas's card, the coordinator's note: this thread went on).

- **The DLC arrivals' late map** (read in the journals, no change): in `i132-r11b8-ringed-travel` the raster's own view selected 18
  objects during the hold (`native_current_view_selected` sample 5280) and 949 after it (5520), while the native admission went on
  (397 `native_model_storage_admitted` and 1,379 `native_encoded_texture_resident` rows over the hold, 53 and 1,064 between those two
  samples): at the Ringed City DS3's own map pieces reach its draw lists only seconds after the arrival's first world frames, for the
  raster as for the rays (`map` 0 then 2,027); INFERRED that the original shows its fade there (not filmed). **Ariandel differs** (the
  final review's M3, `i132-r11b8-ariandel-travel`): at sample 5280, inside its hold, DS3's culler selected 700 objects while the native
  made 7 model intents: there the lag looks like the native admission's own (not established); left open for a loading workstream.
- **SER's first setup** (the state object, the apply pipeline, the targets) is timed: `make_ms` in `native_rt_ser` (-1 before), 195.0 ms
  at Firelink and 202.8 at the High Wall (`i132-r11b10-*`, build-r11b10, whose rows printed -0.0 before it: the -1 form is
  build-r11b12's, unit-tested, not run live). INFERRED inside the arrival's hold (its frames black anyway): no row names the frame of the
  first make; the dispatches at call 600 put SER's first frame near walk 125 of a hold of walks 124 to 154 at Firelink and near 229 of
  229 to 239 at the High Wall (at most two dispatches a frame), right at the hold's start. Only arrivals are covered: a first PT frame
  in play without an arrival (R12's menu switching the level) would take the hitch on a shown frame. Recorded, not moved to the start.
- **The light grid's scratch** (I-0128 « R8c » L7): the boxes and the cells' counts and fills (about 130 KiB) kept between frames by the
  main view's collect (thread_local, zeroed in place; `words` stays the frame's), a unit test (the same words after a larger list and one
  past the budget, which it checks it passed). Not in the thread-affinity list: Test-ThreadAffinity.ps1 scans the render worker's and the
  preparation's functions only (the three entries first added were stale there: removed, the final review's M2).
- **The main view's point batches** (the final review's M1: built for nothing): with the rays asked the main view never draws them
  (`traced`), so the collect now makes DS3's batches only for a view that draws them (the menu views, or the rays not asked); the tiles of
  the rays' list come from the binning as before; the frame's judgement (cause 8) reads a new `local_lights` flag, set where a batch
  would have been made (a tile on screen with a light), so the judgement is unchanged; the `native_point_light_batches` row says
  `batches_made`. Its step review (fresh Opus, high, `6cdacdd3`): M1 the first form set the flag for any light (cause 8 stricter where no
  tile is on screen or the projection unread): moved into the tile loop; L1 two comments, L2 the row's field: done; its re-check: none.
  Checked live on build-r11b13 (`runs-r11c12.log`: Firelink and the High Wall at the PT level, Firelink's TAA: no mismatch, no fatal row,
  no unserved frame; `make_ms` 210.2 and 197.7); build-r11b14 (the flag's final form) built, every unit test passed, not run live.
- **Merged** `e8cabef5` (2026-11-10 16:15 Paris); the launcher rebuilt from `main`, its unit tests passed (`build-main-e8cabef5.log`);
  start check `start-e8cabef5` passed (16:30-16:31, 371 world frames, the arrival ready in 3.1 s, restored). The disk rotation: 1 build
  folder, 0.2 GB, 421.5 GB free on C:.
- **Not done, with their reason** (the figures read in the code, no run behind them): the PT GI without ReSTIR (kind 10) not reordered
  (the test tool's path, `no-rt-restir.flag`, and the session's one-ray GI after a ReSTIR failure); the regions' zeroed row (416 bytes a
  frame) and the root's t4, dead plumbing since R11e (nothing reads them; kept: removing a root slot changes every reflections programme,
  for another tranche); the RR-less `trace` draw (only on frames judged unserved, black anyway); the stencil not read in the ray
  generation (every pixel traced, the apply's stencil choosing: its share of pixels without bit 0x08 not measured); the scratch kept while
  SER idles (two render-extent targets); each SER pass's constants uploaded twice (two 512-byte blocks); §2.4's « work done twice »
  (the G-buffer read by several passes: each pass's own loads, R11a found the pass order prevents fusing them, R11-PLAN §2.3) and « the 30
  skipped walks' rule » (removed by R11e: a skipped walk reuses the last scene).
- **Checks** (`runs-r11c11.log`, build-r11b10, the ASUS, no measure): Firelink and the High Wall at the PT level and Firelink's TAA: no
  `graph_barrier_mismatch`, no fatal row, no unserved frame, SER on.
- **Its step review** (fresh Opus, high, `main..55339f72`): no High; M1 `make_ms` printed -0.0 before the first setup: -1 now; L1 the
  thread_locals outside the thread-affinity list: added, then removed (above); L2 the comment's 300 KiB (130), the counts zeroed before the budget's return
  (after it now); L3 the row's wording (the whole first setup); L4 no test of the kept scratch: added (build-r11b12: every unit test
  passed, the new one with them).
- **The final review** (fresh Opus, xhigh, `main...19a48d54`): no High; M1 the point batches' reason not true (made only where drawn
  now); M2 the thread-affinity entries dead (removed); M3 Ariandel's cause stated without its journal (open now); M4 two §2.4 items without
  a reason (given); L1 kind 10 also the one-ray GI after a ReSTIR failure; L2 the admission counts' window; L3 the -1 form's build; L4 t4
  dead plumbing (said); L5 the clock read only on the first make; L6 the test checks its budget case; L7 STATE at the checkpoint.
  **The integrity review** (D-0268, fresh Opus): no High; its M1 (the counts) and M2 (the first make's frame INFERRED, arrivals only), L1
  (the -1 not run) and L2 (the figures read in the code) answered above.

## R12b (2026-11-10): one settings file, rekindled-settings.ini

R12's first tranche (RT-PT-PLAN §8 R12, D-0265, D-0266; [STUDY-R12-SETTINGS-PAGE](STUDY-R12-SETTINGS-PAGE.md) §4 and §5.5), from main
`91a676e8`, branch `claude/r12b-settings-model` (begun by the previous R thread, `6647d10e`, continued by the R12 thread).

- **The model** (`native/renderer/rekindled_settings.h`, header-only): the remaster's settings in one file beside the module,
  `rekindled-settings.ini` (« key=value » lines: level, bounces, dlss, rr, reflex, frame_generation, taa, sharpen, frame_rate, vsync,
  borderless), the defaults of D-0265 and D-0266, the grammar (any case, blanks, « ; », « # » and « [section] » lines, a UTF-8 mark,
  CRLF, a decimal comma, a key named twice keeping its last value), each refused value named with its text and keeping the entry's
  earlier value; D-0265's rules as `Available()` (what the page greys and locks, never hides) and `Effective()` (what would run): the PT
  level, RR and frame generation only with a DLSS mode, RR locked On at the PT level, Reflex Off locked while frame generation runs, the
  TAA and its sharpening only without a DLSS mode. Its unit test `rekindled_settings_test` is in the build's control list.
- **The module's readers** (`01_state.inc`, `02_frame_lifecycle.inc`, `15_cpu_service_and_input.inc`, `20_startup_phases.inc`): the
  eight one-word files (`dlss-mode.txt`, `rr-mode.txt`, `rt-level.txt`, `rt-bounces.txt`, `reflex-mode.txt`, `fg-mode.txt`, `frame-rate.txt`,
  `sharpen.txt`) are no longer read; each reader keeps its test flags first, then the file, read once at start; V-sync and Borderless
  also read it (besides `vsync-off.flag` and `borderless.flag`); one `native_settings` row names the file's state (found, bytes, keys
  accepted and refused, unknown lines, values, and what `Effective()` would run). The values given to the renderer are the requested
  ones, as before: Streamline and the renderer still decide what runs; the page (R12c) applies `Effective()`.
- **The play launcher** (`tools/Launch-DS3-DX12.ps1`): `-FrameRate` and `-Reflex` write the file's keys; when the file does not exist
  yet, the old files' words are moved into it once, read as the module read them (an unrecognised word was the default, so nothing is
  written for it), the old files then renamed `*.migrated` (never deleted). Thomas's play folder holds only `dlss-mode.txt`
  « performance »: by the code his game keeps DLSS Performance; the launcher's block was run offline on copies
  ([Test-R12bLauncherSettings.ps1](Test-R12bLauncherSettings.ps1): his folder's case, an empty folder, every old word with a UTF-16
  file, a decimal comma and an exponent, a settings file already there: every case passed), the module's reading of the file in a run
  is to be witnessed after the merge (the start check runs its build folder without the file). The launcher prints the DLSS mode with
  the rate and Reflex, each the last value the module accepts.
- **Its step review** (fresh Opus, high, `91a676e8..6647d10e`): no High, no Medium. L1 the old `sharpen.txt`'s exponent: the
  launcher's migration reads one now; L2 a UTF-16 old file, which the old parsers of rr, rt-level, reflex, fg and sharpen misread, is
  read as its text (said in the launcher; the old launcher wrote ASCII and the play folder holds none); L3 the launcher's DLSS line knew
  no `ultra_performance` alias: added; L4 a refused DLSS word left no trace: the refused values kept (`refused_values`, `RefusedValue`),
  the `native_dlss_mode` row's text the refused word again; L5 no test of a refusal after an accepted value: added; L6 stale comments
  naming the old files: renamed to the file's keys, except two shader comments (`rt_lights.hlsl`, `rt_reflect.hlsl`), left so the
  shaders' bytes do not change for a comment; L7 the launcher's own clamp of the bounces: removed (the module clamps).
- **The final review** (fresh Opus, xhigh, `main...729493cd`): no High. M1 the launcher's migration and the module's file path never
  run, the RESULT stating their outcome: the launcher's block run offline (above), the outcome said by the code, the module's path to be
  witnessed after the merge. L1 a UTF-16 settings file ignored: read now (its zero bytes dropped, as `dlss_mode.h`), tested in both
  byte orders; L2 the launcher printed the last value even when refused: the last value the module accepts; L3 the old files brought
  back by a removed settings file: renamed `*.migrated` once moved; L4 D-0266's chromatic aberration and vignette, and the entries'
  table: with their readers in R12c (R12-PLAN); L5 the row's « effective » not what runs: named « rules », said in the comment; L6
  STATE at the checkpoint; L7 two stale comments (`dlss_mode.h`, the WORKFLOW « Tools » row's « Reflex and frame generation to
  come »): fixed; L8 the fonts' lookup without a lock: R12-PLAN's copy once, on the engine thread.
  Those fixes' own step review (fresh Opus, high, `d204ec93`): no High, no Medium; L1 the harness lacked an existing `.migrated` and
  the last accepted value of `frame_rate` and `reflex`: two cases added (seven pass); L2 the launcher refused a frame rate with leading
  zeros the module reads: accepted; L3 the header's grammar comment: the UTF-16 mark named (a comment only, not rebuilt); L4 a play with
  `-Build` on a module older than R12b no longer finds the renamed `dlss-mode.txt` (DLSS off then): recorded here.
- **The integrity review** (D-0268, fresh Opus): no High, no Medium; L1 « his game keeps DLSS Performance » stated as observed: said
  by the code now (above); L2 build-r12b1 left out: it did not compile (a syntax error in the new test, `rekindled_settings_test.cpp`
  line 25), fixed before build-r12b2; no check was changed to pass.
- **Builds**: build-r12b1 (did not compile, above); build-r12b2 (the first commit: every unit test passed); build-r12b3 (the step
  review's answers: every unit test passed); build-r12b4 (the final review's answers). Not run live (D-0227: the start check after the
  merge).
- **Merged** `19f1cd64` (2026-11-10 19:35 Paris); the launcher rebuilt from `main`, its unit tests passed (`build-main-19f1cd64.log`);
  start check `start-19f1cd64` passed (19:52-19:53, the arrival ready in 3.1 s, closed by its close box, restored). **The module's
  reading of the file witnessed** (the final review's M1; `i132-r12a2-overlay`, 19:59-20:01, R12a's build with a
  `rekindled-settings.ini` of `dlss=dlaa` and `sharpen=0.25` beside its module, removed after): the `native_settings` row `file`
  true, `accepted` dlss and sharpen, nothing refused; `native_dlss_mode` DLAA from the file, chosen.

## R12c's door and our Graphics page's logic (2026-11-10, the second R12 pilot)

Branch `claude/blissful-pare-9fb1e7`, a second pilot beside the R pilot (who draws the page, R12a and 16b); static reading in
[R12C-STATIC](R12C-STATIC.md). No drawing in this tranche: the door, the game-side glue and the API the page calls
(`native/frame/game_bootstrap/02b_rekindled_graphics.inc`, included after 02; `native/frame/graphics_rows.h`).

- **Thomas's choices on the way** (this thread, 2026-11-10 Paris): the Screen Mode order Full screen > Borderless > Windowed (19:23);
  after the first run showed the game's Graphics page has six fixed slots (R12C-STATIC §5), our Graphics page recreated, with
  Borderless, V-sync and the frame rate (20:30); Advanced Settings is Quality Setting's new name, the same button leading to the R
  pilot's page (20:32); the preset kept at the head of the Advanced page (20:37); Auto-detect removed and kept OFF (20:40, his
  2026-10-31 decision, options sheet G §7a). Our Graphics page: Screen Mode, Resolution, V-sync, Frame rate, then the Advanced button.
- **The entry** (once the page registers, `RekindledDoorRegister`, or with `rekindled-door-test.flag`): the game's Graphics tab
  holds two value rows built with the game's own row function (`FUN_140AB1C20`, our three functions at the game's MSVC 2015
  `std::function` ABI): « Graphics settings » (the screen mode and resolution as its value) and the game's « Advanced Settings »
  (the preset as its value); the tab's Defaults falls in the third slot, a button slot, as in the original. A press on either
  (`FUN_140AB4C30`, the value rows' press, detoured for our rows only) opens the page at its section (`RekindledDoorSection`);
  the game's own Advanced Settings page (`FUN_140BE3150`) is left as it is (final review M5: the armed tab no longer offers its
  button, and its detour was never run). While the page is open, every Scaleform menu's input query (`FUN_140A5B930`) answers no.
- **The API for the page** (the game thread): the settings edited live and written atomically (`NativeSettingsCurrent`,
  `EditNativeSettings`: V-sync and the frame rate take effect at the next launch); the screen mode with Borderless
  (`RekindledRead/WriteScreenMode`: live, Full screen to Borderless switched natively as I-0076's start does), the game's resolution
  lists and writes (`RekindledResolutions`, `RekindledRead/WriteResolution`: live); the preset (`RekindledRead/WritePreset`, its eleven
  levels from the game's own table `FUN_140EF7340`); DS3's five rows still in use with their offered values, labels and help ids
  (`kGameRows`, `RekindledRead/WriteGameValues`, the preset then Custom). GraphicsConfig.xml is written by the game's own writer
  (`FUN_140EF5AB0`) once the window applied the request (`StepRekindledDoor`, each engine frame).
- **Tools**: `rekindled-door-test.flag` (the door armed, a request taken, closed 180 frames later), `sound-observe.flag` (the menu
  sounds' names in a native run), `no-rekindled-graphics.flag` (the game's tab and Advanced Settings untouched);
  `Run-AlphaCast -OptionsSteps` (presses and keys after `-OptionsTour`, a screenshot each).
- **Tests**: `graphics_rows_test` (our ids free of the game's, the words in English and French, the Screen Mode values, a borderless
  edit read back), `rekindled_settings_test`'s « an edited file read again » (asked by the R pilot). Every unit test passes
  (`build-r12cb1` did not compile, a string literal broken by a shell edit; `build-r12cb2` failed `graphics_rows_test` on 60 frames a
  second read as a custom rate, fixed; `build-r12cb3` to `build-r12cb9` pass; `build-r12cb10` did not compile, `_get_osfhandle` without
  `<io.h>`, fixed; `build-r12cb11` (the step review's fixes), `build-r12cb12` (main with R12a merged in) and `build-r12cb13` (the final
  review's fixes, main merged again) pass, every unit test of the merged tree included).
- **Runs** (the ASUS, offline, not measuring, Alpha's save restored by hash, each announced and its slot given by the coordinator):
  `r12cb-graphics1` (20:10-20:13, `build-r12cb3`: our V-sync and frame-rate rows on the game's page, the six-slot limit seen);
  `r12cb-door1` (21:05-21:08, `build-r12cb6`: the door opened, taken and closed; the button in a value slot showed the movie's
  placeholder); `r12cb-door2` (21:32-21:36, `build-r12cb7`: one value row, its value right, its press opening the door; a patch meant
  to drop Defaults did not, removed); `r12cb-door3` (21:55-21:58, `build-r12cb8`: the two entries and Defaults as in the original, the
  door's round trip); after the step review, `r12cb-door4` (22:36-22:40, `build-r12cb11`, with `update-observe`: the module started, the
  gate inside the witness, the Advanced entry opening the page at `advanced`); after the final review, `build-r12cb13`: `r12cb-unarmed1`
  (23:21-23:25, no flag: the game's Graphics tab as the original's, five rows and Defaults, its Advanced Settings page opening),
  `r12cb-door5` (23:25-23:30: lost at the title, a click missed while a message reached the Claude app; closed without fault, restored;
  run again) and `r12cb-door6` (23:42-23:46: the Graphics entry opening at `graphics`, every API write once with the values held, the
  screen mode, the resolution among 8 offered, the preset, the five rows, then GraphicsConfig.xml saved by the game; Defaults pressed,
  the game's confirmation shown). Every run but `r12cb-door5` exited 0 with no fault, and every one restored.
- **Step review** (a fresh reviewer, read-only): H1 (two detours on `FUN_140A5B930` and on `FUN_1404AD510` with `update-observe.flag`,
  the module refusing to start) fixed, one detour per address; M1 (a page that never takes the request leaving every menu deaf) an
  untaken request dropped after 120 engine frames; M2 the press's registers (`FUN_140AA7900` reads only `rcx`, R12C-STATIC §7), the four
  forwarded; M3 the layouts recorded in R12C-STATIC §7; M4 (D-0153) the game's own words in the other languages; the Lows answered
  (comments, the settings file flushed and always closed, Windowed keeping the full-screen kind, writes on the game thread only,
  the two flags in the stale-flag cleanup, their own error code).
- **Final review** (a fresh reviewer, read-only, on the tree with main merged): H1 the water row labelled « Reflection quality »
  (721109 for 721110) fixed; M1 a page that took the request then stopped leaving the menus deaf: the open page calls
  `RekindledDoorKeepOpen` each frame it draws, 120 engine frames without it close the door; M2 `no-rekindled-graphics.flag` now leaves
  the door unarmed (its step too); M3 the threads: the menu-side hooks run on the game's menu task (the door4 census) and only read,
  the writes and the resolutions' query on the engine frame's thread, which they check; M4 `rekindled-door-test.flag` now drives each
  write once with the values the game holds (a `test_writes` row); M5 the detour of the game's Advanced Settings page removed (never
  run, the armed tab no longer offers its button); the Lows answered (the resolution only from the game's list, the screen-mode write's
  result, the disk outside the settings lock, the choices' words per value, the unarmed probe removed, the runner's flag, our words only
  when armed, error codes of its own, the documents). Left as they are: the input witness's row logs the game's answer before the gate
  (L2: with `update-observe` it says « eligible » while the door holds the menus); the detours' `void` returns (L3: the game's callers
  are a `std::function<void()>` call and the tab builder's pointer); unpinned hook prologues (L13, as the module's other hooks).
- **Integrity review** (read-only): no weakened test, no special case, no work cut during a measure; its Lows answered here (the
  builds' list, the sounds' moments marked inferred, R12C-STATIC §6's observed Defaults).
- **Open**: the page itself (the R pilot's); the levels the renderer reads following a write of DS3's config (the copy config →
  block is encrypted, INFERRED live: to witness once the page writes); the cursor, value-change and cancel sounds of a page (the run
  logged 1010, 100, 1000, 1020 early, INFERRED from their order the title's screens, and 500 at each tab change: to read with a run that moves through rows).

## R12c (2026-11-11, the R pilot): our rows in the game's Graphics tab and Advanced Settings page (D-0271)

- **The decision** (D-0271, Thomas 2026-11-10 23:02 to 23:06): the game's own pages as the base of ours, after the R pilot read
  them (R12C-STATIC §8): the Advanced Settings page's rows are plain C++, its film has fourteen row slots of one kind, a scroll bar
  and a greyed state; the Graphics tab's six slots take D-0270's five rows in slots of their kind. D-0270's native page and its door
  are dropped; the native overlay serves R12d and R12e.
- **What is written** (`02b_rekindled_graphics.inc`, `graphics_rows.h`): the Graphics tab Screen Mode (our three functions on the
  game's value row: Full screen, Borderless, Windowed), Resolution (the game's own three functions), V-sync (the game's On/Off row on
  a byte of ours), the frame rate (a value row, DS3's 60 and seven more, a custom rate shown), the game's Advanced Settings button
  (its functor, which creates the game's page); the Advanced page the Quality Setting preset (state +0x64), DS3's five rows (the
  game's labels, choice builders and state bytes +0x65, +0x68, +0x69, +0x6C, +0x6E), then the RT rows on bytes of ours, each
  built greyed and locked when D-0265's rules make it unavailable and showing then what runs: at the first build rendering, light
  bounces, DLSS, Ray Reconstruction, frame generation, Reflex, the TAA, its sharpening; at the merge (Thomas's asks below)
  Anti-Aliasing (Off, TAA, NVIDIA DLAA, NVIDIA DLSS), « └ DLSS mode » (only with NVIDIA DLSS), the sharpening (a slider, 0 to 10),
  rendering, « └ Light bounces » (only at the PT level), NVIDIA Ray Reconstruction, NVIDIA Frame Generation, NVIDIA Reflex (always
  offered, last). A row's change callback keeps the preset and the levels together (a preset writes its eleven levels, a level
  that leaves the preset's table makes it Custom, a chosen preset put back after the game's own callback) and calls the game's own callback; the engine thread carries
  the changes into `rekindled-settings.ini` (V-sync, the frame rate, the RT rows: at the next launch, as each help line says) and the
  screen mode into the window (Borderless switched natively, as R12c's door did). Removed: the door, its two Graphics entries, the
  value-row press detour and the menus' input gate. Auto-detect kept OFF.
- **The first run** (`r12g-pages1`, 2026-11-11 00:33-00:39, ASUS, `build-r12g1`, `-OptionsTour -OptionsSteps`): both pages show our
  rows in the game's look (the Graphics tab: Screen Mode « Windowed », Resolution « 1280x720 », V-sync, « Maximum frame rate 60 (Dark
  Souls III) », the Advanced Settings button, Defaults; the Advanced page: Quality Setting « Custom », the five rows, then the eight RT
  rows, Rendering, Light bounces, Ray Reconstruction and Frame generation greyed). The cursor skips a greyed row. V-sync (On to Off),
  Texture (Max to Low) and the TAA (On to Off) were written: the settings file twice, and the Texture level applied live by the game
  (`native_option_change` generation 4, `levels[0]` 4 to 1): the page's change callback carries the state into the config at once.
  Two faults: a greyed row showed its list's first entry (Light bounces « 1 » for 2, Ray Reconstruction « On » for off), and the
  fifteenth row (the page's Defaults, added after our fourteen) did not scroll: the cursor went off the film and the next step ended
  the game in Scaleform (0xC0000005 under `FUN_140A4CA90`, the window's update; dump kept). Fixed (`build-r12g2`): a greyed row's
  list is the one value it shows; the Defaults row is left out of our Advanced page (OpenRestoreDefaultsBox_140AAE950 skipped for
  it), the Graphics tab keeping its own. A value row's right arrow does not step its value (it opens its list on a press, as the
  game's Screen Mode row): the frame rate's list to try with a press.
- **The second run** (`r12g-pages2`, 01:09-01:16, `build-r12g3`, in play at Firelink): no fault; DLSS set to DLAA built the page's rows
  again at once, the cursor kept (`rebuilt_cursor` 8): Rendering, Ray Reconstruction and Frame generation opened, the TAA and its
  sharpening greyed; a row turned greyed by the rebuild lost its words (its slot's GrayoutItem hidden by the enabled row it held).
- **Thomas's asks during the tranche** (this thread, 2026-11-11): the greying live (00:44, « priorité »): the page's rows built again
  at the page's next update when a row that governs others changed (the generic window update FUN_140A4CA90 entered for our page, the
  rows' list emptied as the page's destructor empties it, our rows built in the same slots, bound again with the cursor where it was;
  R12C-STATIC §8.4); one « Anti-aliasing » row above Rendering, TAA then DLSS (00:46, 00:47), Off first (01:28: « Désactivé en
  premier »), the DLSS mode in a row that appears with DLSS; the sharpening kept with DLSS (01:05): its pass now runs on DLSS's output
  too (`TaaStage::RecordSharpen`, inside the DLSS pass: a compute copy of DLSS's output into a scratch, then the TAA's own sharpening
  from it back into the output, which stays in UNORDERED_ACCESS for the frame graph).
- **The step review** (fresh, read-only, on `44ca82a0..66b2dc61`): H1 the preset row wrote the game's Antialiasing level, which the page
  no longer offers and which gates DLSS and the TAA (FollowNativeDlssOption), in the config's encoding where the state holds 3 for On:
  the game's Antialiasing kept On (state +0x66 = 3 at the page's build and after a preset, the window's config +0x20 = 1 each frame);
  M1 a refused V-sync write retried every frame: one write a value; M2 an RT row's value refused by the file's rules while the row
  showed it: the page's settings handed whole to the engine thread; M3 STATE's rows, rewritten at this checkpoint; M4 a run's choices
  left in the build folder's `rekindled-settings.ini`: `Run-StartupTrace.ps1` keeps it aside and puts it back; L1 each RT default its
  own four bytes; L2 the Defaults skip consumed once, the rebuild only for a window of the sub-page's class; L3, L4, L5 (a
  static_assert on the fourteen slots), L6 the dead door-test flag dropped. Outside the range: a comment that swallows code in
  `tools/Launch-DS3-DX12.ps1` (its message only), left for the launcher's next change.
- **The third run** (`r12g-pages3`, 02:00-02:08, `build-r12g6`, the tree with main merged): the Anti-aliasing row from TAA to DLSS
  built the rows again with the DLSS mode row (fourteen rows, on DLAA, only Light bounces greyed); the mode set to Quality and the
  sharpening to 25 % with DLSS (each written, `native_rekindled_graphics` « rt_rows »); back to TAA, the mode row gone (thirteen rows,
  the fourteenth slot empty), the greyed rows with their words (Ray Reconstruction « Off », Frame generation « Off »). The Anti-aliasing
  row's English help was cut at the box's edge (about 95 characters): the four longest help lines shortened, `graphics_rows_test`
  bounding every RT help line to 88 characters in both languages (not witnessed in a run; the French width not seen).
- **The sharpening after DLSS measured** (`r12g-sharpen-on` with `dlss-dlaa,sharpen-100`, `r12g-sharpen-off` with `dlss-dlaa`,
  02:08-02:13, `build-r12g6`, the same view at Firelink): the pass ran (`native_dlss_sharpen_frames` in the first run only); on
  `ingame.png` the mean absolute Laplacian of the luma (`sharpness2.py`, ffmpeg's crop to grey) 24.42 against 17.37 on the floor
  (800, 430, 450x190: +41 %) and 2.62 against 2.07 on the wall (450, 120, 400x160: +26 %), the mean luma 81.81 against 81.33 and 25.09
  against 24.94, the desktop control region (1400, 200, 400x200) identical (0.939, 76.94 both). One capture an arm from two launches:
  single-pair figures, no same-setting repeat to give the runs' own spread with DLSS and RR.
- **The fourth run** (`r12g-pages4`, 02:45-02:52, `build-r12g7`): the sharpening's slider (Thomas, 02:21: « C'est mieux un curseur »,
  the game's slider row, R12C-STATIC §8.5) written at each step (0.1, 0.2, 0.3, then 0.4 with DLSS); the preset row set to Low wrote
  the levels (the rows' reports carry Low's levels; the reading then that the rows showed and wrote back their old values is not
  borne out by the journal, the final review's L2);
  with the Anti-aliasing set Off, the sharpening rebuilt greyed in the slot its slider held showed nothing (the choice row's maker
  hides the slider and never shows its own ComboBox), and the cursor's next step ended the game in Scaleform as at `r12g-pages1`
  (0xC0000005 at `FUN_140E60346`, under `FUN_140A4CA90`; dump kept). Fixed (`build-r12g8`): each row's own widget shown after it is
  made (ComboBox or Slider); a preset or a level that changes the preset builds the rows again; Reflex, always offered, the last row
  (`graphics_rows_test` holds it), the sharpening moved under the anti-aliasing.
- **The fifth run** (`r12g-pages5`, 03:17-03:24, `build-r12g9`, the review's fixes): the preset set to Low wrote its levels, the
  five rows reported those writes as their own changes and so set the preset back to Custom; with the Anti-aliasing Off, the
  sharpening's slot (a slider before) rebuilt as a greyed choice row, its ComboBox now shown, and the cursor's next steps ended the
  game again (the same Scaleform fault at `FUN_140E60346`): a slot whose row changes from the slider to a choice row is not survived.
  So (`build-r12g10`): the sharpening a choice row of eleven steps (Off, 10 % to 100 %, the kind that went from active to greyed and
  back without a fault in `r12g-pages2` and `r12g-pages3`), the slider's code kept behind `kSharpenSlider` (off) until that switch is
  understood (Thomas told); a level change sets Custom only when the levels left the preset's table.
- **The sixth run and a diagnostic** (`r12g-pages6`, 03:37-03:43, `build-r12g10`; `r12g-wrap1`, 03:44-03:50): the same fault without
  the slider (the sharpening an eleven-step choice row), so the slider was not its cause: in `r12g-pages6` at the cursor's step up
  from the first row after two rebuilds; in `r12g-wrap1` the step up from the first row on the page as opened went to the last row
  without a fault, and after one rebuild (Anti-aliasing to DLSS) the fifth step up (into the five DS3 rows) ended the game. The lead:
  the rebuild releases the old rows (FUN_140A90390) while another part of the page still names one, the queued Scaleform commands of
  the window's update (`FUN_140A4CCA0` → `FUN_140E4DDF0`) then reaching a freed row. Written, not yet built (Thomas took the PC at
  03:50): each old row kept alive by one more reference across a rebuild (`rows_kept`), to witness; R12c is not merged until a
  rebuild is proven safe (both withdrawn by the RCA below: `rows_kept` dropped, `r12g-pages7` the rebuild's witness). A chosen
  preset became Custom through the game's own callback (Max too, `r12g-wrap2`; what it compares is not read): the preset put back in
  the page's state when the levels are its table's, and in the window's config by the engine thread (`preset_kept`).
- **The RCA** (Thomas, 04:27: « Lance une RCA Fable 5.2 », effort high; Fable 5.2, read-only, the six dumps and the journals; checked
  by the pilot on the deadlines): every fault of these runs (`r12g-pages1`, `r12g-pages4` to `r12g-pages6`, `r12g-wrap1`, `r12g-wrap2`)
  is the bounded run's stop arriving with a menu open, at its deadline to the second (`created_utc` + `seconds`: 03:43:28 and the fault
  at 03:43:30 in `r12g-pages6`, 04:26:24 and 04:26:25 in `r12g-wrap2`; no fault in `r12g-pages2` and `r12g-pages3`, whose menus had closed
  before their deadlines): the controller sets `stop`, `Close()` withdraws the native Scaleform HAL (`manager+0x980 = 0`,
  `GameUiSession::Close`), and the menu's next frame re-imports the Status face (`SYSTEX_Menu_StatusFace`, RDI 0 in `FUN_140E602F0`, no
  null check). The known close fault of I-0113 (its RESULT, 2026-11-08); INFERRED not reached in a player's session (the controller sets `stop`
  only at a bounded run's deadline, never in play), but I-0113's tranche 31 met the same fault when its runner closed the window
  with the profile list open: a player closing the game with a menu open (Alt+F4) is a plausible path, not witnessed; our step
  lists were longer than the runs. So the earlier readings (the fifteenth row, the slider's slot, a released row) are withdrawn, the live
  rebuild and the slider are on again (their two switches since removed, the final review's L6); the Defaults row stays out (the fifteenth row did not scroll:
  a visual observation of `r12g-pages1` that stands). The runs end their steps with Esc before the deadline. Open: the close fault
  itself (a guard on `FUN_140E602F0` when the HAL is withdrawn) and the menu's per-frame re-import of the face image, for E.
- **The control run** (`r12g-pages7`, 05:00-05:08, `build-r12g14`, the live rebuild and the slider on, the steps ended by Esc before the
  deadline): no fault; the Anti-Aliasing from TAA to DLSS built the rows again (fourteen, « └ DLSS mode » under it), the mode set to
  Quality, the sharpening's slider moved twice (0.1, 0.2), back to TAA (thirteen rows), then Off (the sharpening greyed), the cursor
  down past the last row and up past the first; each change written to the settings file.
- **The last asks and their run** (`r12g-pages8`, 05:55-06:03, `build-r12g17`): NVIDIA DLAA its own Anti-Aliasing choice (Off, TAA,
  NVIDIA DLAA, NVIDIA DLSS; Thomas 05:29), the DLSS mode row Quality to Ultra Performance only with NVIDIA DLSS; « └ Light bounces »
  only at the PT level (Thomas 05:09); « Anti-Aliasing », « Génération d'images NVIDIA » (Thomas 04:30, 02:58). Witnessed: twelve rows on
  TAA, the sharpening greyed with Off and back as a slider with TAA (moved to 0.1), NVIDIA DLAA (twelve rows, nothing greyed), NVIDIA DLSS
  (the mode row, set to Balanced), Path tracing (the bounces row, set to 3; Ray Reconstruction greyed On); no fault. The preset Max was
  kept (`preset_kept`), then turned Custom again by the game's callback after the five rows' reports: the chosen preset is now put back
  after any row's callback while the levels are its table's (written after the run, not yet built: Thomas tested `build-r12g17` himself).
- **The preset kept, its run** (`r12g-pages9`, 06:40-06:46, `build-r12g18`): Max chosen on the preset row, the five rows' reports, then
  the Anti-Aliasing changed twice (NVIDIA DLAA, back to TAA), each change a rebuild: `preset_kept` 4 after each, the page's preset row
  on Max in every picture, no report of the game's Custom on the preset row, no fault, restoration held. Not witnessed: the page
  closed and opened again (the Esc closed the whole menu).
- **The second step review** (fresh, read-only, on `aa57521de..d073806c3`): no High; M1 choosing Custom on the preset row was put
  back to the remembered preset while the levels matched it: the preset row's report now always taken as the player's choice, Custom
  included (`r12g-pages9`: the game's Custom no longer reaches that row); L1 the help lines and comments that still made the PT, frame
  generation and the sharpening wait on « a DLSS mode » (NVIDIA DLAA opens them too) and L2 the bounces row called greyed: rewritten;
  L3 a DLAA case in the rows test's choice loop and its round-trip through the file: added. Built in `build-r12g19`, not run live.
- **The final review** (fresh, read-only, on `main...e226119b`): H1 Path tracing with Ray Reconstruction set Off ran the RT level at the
  next launch (the module read the file's raw rr): `NativeRrRequested` now gives RR at the PT level, as `Effective`; M1 « Off » left
  DS3's own anti-aliasing filter running (its gate the game's Antialiasing option, which the page keeps On): the settings' « Off »
  now turns it off too (`no-taa.flag` is not the page's Off: it keeps DS3's filter), and the forced option is recorded in D-0271; M2 a preset did not refresh the rows (and seen in `r12g-pages4`):
  the rows built again; M3 STATE's rows and R12-PLAN (a NUL byte from a PowerShell escape, its head): rewritten; L1 the page
  recognised by its rows' list too; L4 the V-sync byte read as an atomic; L5 the window update's RDX never read by it (its prologue),
  written; L6 the runner's settings file put back after the settle check; L8 the stale documents. Left as they are, recorded:
  L2 no WARP test of `RecordSharpen` and `Mirror` (the measured runs witness them; a typed UAV load of R16G16B16A16_FLOAT, which every
  DLSS-capable adapter supports); L3 the sharpening outside the DLSS zone of the GPU times, its stage opened at its first frame;
  L7 DLSS offered whatever the adapter (the TAA's or DS3's filter then runs, as the module's DLSS latch already does).
- **The integrity review** (fresh, read-only): no High or Medium; the sampled figures reproduce (24.418 against 17.373, 2.617 against
  2.072); its Lows answered above (the control region's box, single-pair figures, the help lines' width not witnessed) and here: the
  unit tests' changed assertions are the sharpening kept with DLSS (Thomas, 01:05) and the door's ids removed with the door.
- **The final tree's integrity review** (fresh, read-only, `main...3f6cfa113`): no High or Medium; its sampled figures and
  faults reproduce (the six dumps 1.6 to 3 s after their deadlines, one address); its Lows answered: the close fault's « impossible »
  marked INFERRED, `no-taa.flag` set apart from the page's Off, this RESULT committed before the merge.
- **The final review on the final tree** (fresh, read-only, `main...8c1cd3769`): no High. M1 the slider's own game callback (the
  page's vfunc 0x90) never passed through our put-back: the chosen preset now put back at each update of the page too, and the
  preset row's report taken as the player's only with the cursor on it; M2 R12-PLAN's exit (the pages from the title, Resolution
  and Borderless from their rows, a later launch reading the file) not witnessed: moved to R12f's exit (R12-PLAN); M3 STATE's next
  action rewritten after R12-PLAN at the checkpoint. Lows answered: L1 R12C-STATIC's lost « r » and LF endings, L2 stale sentences
  here, L3 the « never hidden » comment and D-0271's later asks, L4 I-0113's tranche 31 cited for the close fault, L5 the DLSS
  sharpening's latch reset on close, L6 the two always-on switches removed, L7 the menu task's wording, L8 the frame rate's
  pending value shown, L10 our page's identity cleared when its address holds another window. Left, recorded: L9 the 32-byte buffer
  given to `FUN_140C20E20` not read (the Defaults row is left out); L11 the launcher's `borderless.flag` and `vsync-off.flag` over
  the page's choice, for R12f. Its step review (fresh, `8c1cd3769..290b44e91`): no High or Medium; its Lows left for R12f, which
  redoes these paths: the frame rate's pending value taken before its write (a brief old value), our page's address stored before
  its rows (a re-entered update would clear its identity; not seen in the code read), the game's Custom shown one frame after the
  slider, the mouse's focus on the preset row not read.
- **The control of the final code** (`r12g-pages10`, 07:30-07:37, `build-r12g20`): Max chosen and kept (`preset_kept` 4), the
  sharpening slider moved to 0.1 then 0.2 with the preset still Max, Custom chosen on the preset row and kept, Max again and kept,
  the texture quality lowered (the preset Custom, the rows built again); no fault, restoration held.
- **A later launch reading the page's file** (Thomas's own play with `build-r12g20`, the play launcher, its journals
  `%LOCALAPPDATA%/DS3-DX12/play-20261111-073907.jsonl` and `play-20261111-080849.jsonl`, a Windows restart between): the second
  launch read `dlss=dlaa` that the page wrote in the first (the first read `dlss=quality`), the rest unchanged (PT, four bounces, Reflex
  on, sharpening 0.2, 144 frames/s, Borderless); the Agility SDK asked and loaded (620) in both. Thomas: « 20fps en PT 4 rebonds et il
  est magnifique » (NVIDIA DLAA at the PT level, the ASUS or OLED not named).
- **Merged** `2a9e0b7b` (08:38 Paris, Test-Project passed in main). The launcher's rebuild from main (08:22-08:34): the module and
  every test but one built and passed; `pipeline cache hardware` FAILED « no native hardware adapter », and the same binary of
  `build-r12g20` that passed at 07:20 failed too: Windows had installed KB5132061 at Thomas's restart (08:04; D3D12Core
  10.0.26100.9818). OBSERVED with a standalone probe: once the Agility SDK's device factory exists, a dxgi.dll loaded after it makes
  `D3D12CreateDevice` on the RTX 4070 Ti Super answer DXGI_ERROR_UNSUPPORTED (the factory's device and the inbox probe alike, with
  or without the debug layer); DXGI loaded first, the device opens (SDK 620). DS3's process imports DXGI, so play was not touched
  (Thomas played `build-r12g20` after the restart, the Agility SDK loaded). Fixed on the branch (R12f, `backend.cpp`: the system
  DXGI loaded before the factory); the probe opens SDK 620 with it. The start check `start-2a9e0b7b` passed (08:45-08:46, the
  launcher's module, restored).
- **After the merge, merged with them** (`build-r12f1`, every test passed (its folder overwritten on 2026-11-11 09:23 by I-0113 tranche 36's first build, by mistake: this build's log and test outputs lost), the pipeline cache's hardware profile included): the
  system DXGI before the Agility factory (above); RR's preset F written in every mode (Thomas, 07:25: « Preset F pour RR »; FG has
  no preset in Streamline 2.14.16, the driver's override alone picks one); the frame rate's pending value cleared only once
  written; the Advanced page's identity stored after its rows. Their step review (fresh, read-only): no High or Medium; L1 RR's
  stale « default preset » comment rewritten; L2 the rebuild path keeps the page's identity while its rows move (cleared, never
  wrong, if an update re-entered; not seen): for R12f.
- **The checkpoint** (`1d65a15a`): the launcher rebuilt from main, every test passed; start check `start-1d65a15a` passed
  (09:15-09:16, restored); the disk rotation (Clear-DiskCache -Apply: 13 build folders older than 48 h, 3.2 GB).
- **Open**: the pages from the title screen's Options (every run went through Esc in play); the frame rate's list (a value row opens
  its list on a press), Resolution and Borderless chosen from their rows, the tab's Defaults row; a later launch reading what the
  page wrote; the shortened help lines seen in a run.

## R12f (2026-11-11, the R pilot): every setting live, NIS

- **The ask** (Thomas, 2026-11-11 06:21 and 06:22: « ça peut pas se faire en direct ? », « c'est pas possible de tout faire en temps
  réel ? »): each row of the pages applied while the game runs. The R pilot's thread that first opened R12f (now E) left a Sonnet worker's
  map of where each setting was read ([R12F-LIVE-SETTINGS-MAP](R12F-LIVE-SETTINGS-MAP.md), not re-read by it); this thread read the paths it
  changed.
- **The first tranche** (`69e4f6dd`): the module's readers (`01_state.inc`, `NativeRequestedRate`) take the settings copy the page edits
  (`NativeSettingsCurrent`, under a short lock; the test flags still first, read once). The renderer already decided the TAA, DLSS and its
  mode, RR, the PT level and the bounces a frame at a time, its histories resetting on a gap (the TAA's, DLSS's, ReSTIR's, NRD's serial
  rules), so a change reaches the next engine frame through the existing followers (`FollowNativeDlssOption`, `FollowNativeReflex`, the
  render extent); `FollowNativeLiveSettings` (BeginFrame) pushes what the start set once: the rays' level, the bounces and RR (only where
  the rays were set up; written on a change only, so the renderer's own RR latch holds), the V-sync and the unlocked rate's presentation.
  The frame rate: DS3's limiter pair put back when the player returns to 60, the I-0114 corrections installed at every native start (k held
  at 1 at DS3's rate). The V-sync: the paced chain made tearing-capable, a Present tearing only without V-sync. DS3's own AA filter gate
  live. The help lines no longer say « at the next launch ». The launcher's `-Borderless` and `-NoVsync` written as the settings' keys when
  the file has none (R12c's final review L11), never as flags any more, and a line printed when the file already holds the key.
- **Its step review** (fresh, read-only, `d6c41c06..69e4f6dd`): H1 the follow camera's laws take their own k from the call's dt, so at
  DS3's 60 with a slow frame they rescaled the camera: they now call the original at DS3's rate (`native_rate_unlocked`), the wall state
  given back in the original's units; M1 the rays' level and bounces read several times in one worker frame: taken once at the main frame's
  start (`rt_pt_now_`, `rt_bounces_now_`); L2 the corrections not installed under `original-unlock.flag` at the rate 0; L3 the rate pinned
  to the start's where the corrections are not installed (the observing flags); L4 the ignored launcher switch printed; L5 the settings
  applied live even when the file cannot be written; L6 the SER row's level live; and, outside the range, the launcher's `elseif` swallowed
  by a comment since R12b (its rate summary): fixed. L1 (the tearing-capable chain in exclusive full screen with V-sync on) is for a run.
- **The second tranche** (`b3ee685d`, frame generation): NVIDIA's guide asks for the swap chain to be made again whenever DLSS-G is
  switched on or off (ProgrammingGuideDLSS_G §18; §12.1: while the plugin is loaded Streamline's pacer owns the frame-latency object, so the
  chain is made without ours). Streamline is now initialised with DLSS-G at every start (unless `no-fg-offer.flag`) and the plugin unloaded
  (`slSetFeatureLoaded`) before the first chain when the player did not ask for it; a switch (`SetFrameGenerationLive`, under
  `WithPresentationIdle`) drains, turns DLSS-G off, leaves exclusive full screen, releases the chain, loads or unloads the plugin, makes the
  chain again at the same size, and goes back to exclusive full screen. Its step review (fresh, `69e4f6dd..b3ee685d`): no High; M1 a
  refused unload left frame generation on with the player's Off: the player's choice kept apart from the plugin's state (`fg_wanted_`);
  M2 a new multiplier sent only at the next On: its mode marked unknown; M3 a switch after DLSS was latched off: offered only with DLSS
  ready, on only with the proxy chain; M4 a refused proxy chain during a switch left no frame-latency object: the chain made again with it;
  M5 the exclusive full screen given back at the mode `ApplyDisplayMode` chooses; L1 the settings applied live before any early return of
  the file's write; L2 `native_rate_unlocked` with k's own rule; L3 a camera first seen at DS3's rate adopted in its units; L4 (every DLSS
  start now asks for the frame-based tagging and two more exports) recorded here.
- **NIS** (Thomas, 2026-11-11 03:53: NVIDIA's NIS in place of our sharpening): Streamline's `sl.nis`, already in the SDK the project holds
  (signed by NVIDIA, now linked beside the module), its sharpen-only mode (NVSharpen) on viewport 1, after the tone map on the 8-bit colour
  (its copy in, its own output, copied back; three passes in the frame graph, the plan key's bit 19), where the TAA or DLSS resolves the
  frame and the sharpening is asked; our own pass where NIS is not offered or latched off. The sharpening's help line names NIS.
- **The runs** (the ASUS, no measure, Run-AlphaCast `-OptionsTour -OptionsSteps`, restored by hash): `r12f-live1` (10:08-10:15,
  `build-r12f-live2`): the V-sync turned off on the page applied at the next frame (`native_settings_live` vsync false) and back with the
  tab's Defaults; the rest missed: the pilot confirmed with Enter, which DS3's menus ignore (E confirms). `r12f-live2` (10:35-10:41,
  `build-r12f-live3`): the V-sync off, 90 frames/s (`frame_rate` 90 live), NVIDIA DLAA (`native_dlss_option` active, the first DLSS frame at
  once), path tracing (`level` pt), three bounces, then frame generation on: the chain made again in 75 ms, Reflex forced On, DLSS-G's
  first On present (status 0); each written to the file and applied without a relaunch. At its next frame the debug layer warned that a
  queue waited on a fence at 0 (« will always be satisfied »), which the module never does (its only `ID3D12CommandQueue::Wait` is
  `test-gpu-hang.flag`'s, at 1: Streamline's queue, INFERRED), and the run ended on it as on any debug-layer warning (dump kept): filtered while DLSS-G is loaded
  (`FrameGenerationFenceNotice`, counted in the frame generation's row), a fault otherwise.
- `r12f-live3` (11:00-11:07, `build-r12f-live4`): the same, then NIS at 0.1 and 0.2 with NVIDIA DLAA (`native_nis` ok, 3,600 frames
  by 11:05), frame generation on, off and on again (91, 91 and 75 ms; Reflex On, Off, On; two frames presented a rendered frame,
  status 0); no fault, restored. The fence warning was filtered (`fence_notices` 2; one notice counted at both its readers, INFERRED from the
  code, since counted once). NIS's frames had four `graph_barrier_mismatch` each (3,023 rows over 758 serials, the rows' own cap): the
  passes declared before their hand transitions (the shadow records the hand state at the declaration); the hand barriers drew the
  frame (no still compared). The declarations reordered with NIS's step review below, then seen clean in `r12f-live5`.
- **NIS's step review** (fresh, `b3ee685d..e7dc57a6`): H1 the NIS targets made again at a new extent without
  `slFreeResources(NIS, viewport 1)` (ProgrammingGuideNIS §3; Streamline caches its views by the resource's address): freed first
  (`NisFreeResources`); M1 the declarations after the transitions (above); L1 Reflex forced while DLSS-G is loaded, wanted or not;
  L2 the upscaler state after a refused proxy chain; L4 the notice counted once; L5 the comment (the module's only queue wait is
  `test-gpu-hang.flag`'s, at 1); L6 comments. Left, recorded: L3 the exclusive full screen given back at the mode nearest 60 Hz, not the
  chain's own (a high-refresh exclusive mode entered by Alt+Enter); L7 a second follow camera's wall state forgotten (the one-object
  limit, already documented).
- **The sub-rows' separator** (Thomas, 05:11 and 05:12): a static study of the film (a Sonnet worker, `film-study/parse.py` in the
  pilot's scratchpad): the left line is an unnamed shape inside an unnamed sprite of the slot's `Widgets/ComboBox/Caption`, which also
  holds the label `Text_0`; a Ghidra study (`ghidra-research`, GhidraMCP): the game sets a clip's position and scale through Scaleform's
  `GetDisplayInfo`/`SetDisplayInfo` (the ObjectInterface's 0xD8 and 0xE0, X at +0x00, XScale at +0x18, flags 0x1 and 0x8). So the two
  « └ » rows' `Caption` is scaled to 75 % and moved right by the quarter it loses, its `Text_0` scaled back and placed so the label stays
  (graphics_rows.h `RowCaption`, unit-tested); every other row gets the film's values again (a slot keeps its clips across a rebuild).
- `r12f-live4` (11:26-11:35, `build-r12f-live5`, Run-AlphaCast's new `-TitleSteps`): R12c's moved exit, the pages from the title: the
  title's System opened the Game Options, its Graphics tab with our five rows and the Advanced Settings page with ours (twelve rows,
  `built open` at the title, OBSERVED on `title-step-6` to `-11`). Esc did not close the page at the title (three presses, the page
  stayed), so the runner's Load Game presses landed on the Reflex row, which cycled Off, On, Boost and was written and applied each time,
  and the in-game steps did not run; no fault, restored. The caption's places read in the game: Caption -456 (the film's), Text_0 -200
  (the film's PlaceObject says -198): the layout now takes the places the game holds (read from a slot not yet changed).
- **The final review** (fresh, xhigh, `main...03df5662`): no High. M1 frame generation switched in exclusive full screen never run
  (left open, below); M2 the return to 60 never run: seen in `r12f-live5`; M3 the RESULT's claims (rewritten here) and STATE (at the
  checkpoint); M4 the collision with E's gap 27 (`FrameK`): main (`ee319129`) merged, the rule combined (k held at 1 at DS3's own rate,
  `FrameK` otherwise, the camera's laws too), step-reviewed with no finding. Lows answered: L1 `gr::SubRow` used, L2
  `GetDisplayInfo`'s answer taken, L3 `%g`, L5 the frame generation's latch checked first, L7 comments, L13 the filter disarmed at
  `Close`, L15 a lambda renamed, L16 `camera-fixed-step.flag` among the observing flags. Left, recorded: L4 (a scripted clip may stop
  following its timeline: the sub-rows' captions looked right in `r12f-live5`'s shots), L6 (`slSetFeatureLoaded` while the precompile
  workers use the device: the device is not proxied under manual hooking, INFERRED low risk), L8 (a dozen short settings copies a frame),
  L9 (NIS's two targets kept once made), L10 (the help line names NIS where our pass is its fallback), L11 (sl.nis loads at every
  Streamline start: a default change), L12, L14 (the live row logs the raw settings).
- **The integrity review** (fresh, `main...03df5662`): no High; M1 NIS's fixes not run (then run in `r12f-live5`, above and below);
  M2 the 60 frames/s path through the installed corrections not measured against the original (open, below); L1 the inverted help-line
  assertion (`graphics_rows_test`: no RT help line says « next launch »), named here; L2 two wordings, rewritten; L3 the mismatch rows'
  cap and no still for NIS, rewritten; L4 `r12f-live4`'s stray presses also changed DS3's preset and levels (235 level rows, the
  graphics config saved 43 times), restored by hash.
- `r12f-live5` (12:25-12:33, `build-r12f-live7`, windowed): the V-sync off, 90 then back to 60 (`native_frame_rate_restored` 30 and 0,
  the pair the game held before the first write), NVIDIA DLAA then NVIDIA DLSS Quality (`native_render_extent` 853x480 in 20.6 ms, back to
  the output with DLAA), NIS on DLSS (0.1, 0.2) and on the TAA (`native_nis` through 14,400 frames), RR off and on, path tracing and back,
  the anti-aliasing Off then TAA; 0 `graph_barrier_mismatch`; the captions read -456 and -200. Then the Resolution row's change
  (1440x810, `native_output_resized`) faulted the render worker at once: « primitive admission identity » (dump kept, restored). The UI
  packet posted at the collect's entry was made at the former extent and the output followed the engine's size before the render step
  (collect, BeginFrame, render step: ENGINE-STEP-THREAD-MAP), so the worker's admission refused it: a fault of the native resize at any
  resolution change made in play, older than R12f (INFERRED: the same path since I-0078's UI worker). Fixed: that packet dropped, the
  frame presented without its UI, the next at the new extent (`TakeNativeUiDisplay`). Its step review (with main's merge): the first
  version prepared the UI again inline, which prepared nothing (the movies' prepared flags already taken): removed, the comment on the
  frame order rewritten.
- `r12f-live6` (12:55-13:01, `build-r12f-live9`, windowed): R12c's moved exit, Resolution and Borderless from their rows: 1440x810, then
  Borderless at 1920x1080, each applied at once (`native_output_resized` four times, `native_ui_display_changed` with the extent changed,
  no fault); the game's own « Keep current settings? » then answered No by the pilot, which put each back. The exclusive full screen
  and frame generation in it were not reached. Restored.
- **The final review on the final tree** (fresh, xhigh, `main...258b49c4`, main `ee319129` merged): no High; the RESULT's runs
  match the journals. M1 « a later launch reading the page's file »: witnessed for R12c's page in Thomas's play (R12c above, his
  journals of 07:39 and 08:08), not again with R12f's file (which now always holds `borderless` and `vsync`): open, below; M2 leaving
  exclusive full screen for a frame generation switch was a hard fault on a transient DXGI refusal: now refused as busy before anything
  is released and tried again at the next frame. L1 the DLSS mode row itself and Reflex in play not changed in a run (the DLSS mode
  through the Anti-aliasing row, Reflex at the title; the same mechanism); L2 the return to 60 after a start at an unlocked rate not
  run (the pair DS3 held is captured at the first unlocked frame); L3, L4 comments (NisFreeResources' invariant; BeginFrame falls after
  the collect: a live change lands on one frame collected under the old settings and rendered under the new); L6 an unused accessor
  removed; L5 STATE at the checkpoint; L7 (from R10b: frame generation stays on while the options pages cover the world, ProgrammingGuideDLSS_G
  asks it off in full-screen menus) for the frame generation's next work.
- **Its step review** (fresh, `258b49c4..68fb879a`): no High; M1 a busy switch retried every frame (a quiesce, a drain and a row each
  frame while DXGI kept refusing): now every 30th frame, its row the first time and every 20th try; L1, L2 comments. Built
  (`build-r12f-final`), every test passed; not run live (the busy path needs DXGI to refuse leaving the full screen).
- **Merged** `c9db5355` (14:00 Paris, main `901f93d4` merged into the branch first; Test-Project passed in main); the launcher rebuilt from
  main (every test passed); start check `start-c9db5355d` passed (14:16-14:17, the ASUS, 346 world frames, restored).
- **Open**: frame generation switched in exclusive full screen, and the tearing-capable chain in exclusive full screen with V-sync on
  (the first step review's L1), never run; the 60 frames/s play through the installed corrections not compared with the original's (no
  measure; k held at 1, each detour the original's call); the sub-rows' separator not looked at by Thomas (its shots
  `evidence/I-0066/artifacts/r12f-live5/options-step-*.png`); a frame generation multiplier changed in place (the RTX 4070 Ti Super offers
  x2 only); NIS against our sharpening on a still; the Esc at the title page (the pilot's: B or Esc did not close it); a
  later launch reading R12f's page file; the return to 60 after a start at an unlocked rate; frame generation off in the menus (L7).

## R12g (2026-11-11, the R pilot): the GI quality, the vignette, our scroll and its bar, frame generation off under the menus

- **The ask** (Thomas, 2026-11-11 06:25 to 06:34, R12-PLAN): the texture quality and the anisotropic filtering (x2 to x16) as two rows, a
  GI quality at the PT level (the paths per pixel, beside the bounces), the vignette On/Off; during the tranche (15:19 to 15:23): the
  vignette the last row, the page's scroll bar, the game's Texture Quality row kept. And from R12f's final review L7, the former R pilot's
  question to Thomas (frame generation off while a menu covers the world, NVIDIA's guide §6.4), unanswered by this checkpoint: its
  recommendation applied (cut automatically while an in-game menu is open, back by itself in play), as the coordinator relayed.
- **The static reading** ([R12G-STATIC](R12G-STATIC.md), three Sonnet `ghidra-research` workers on GhidraMCP): DS3's « Texture Quality »
  sets only the material samplers' filter and anisotropy (OBSERVED for every reader found; the game's debug menu labels the level
  « TextureFilter »; no texture resolution, INFERRED: the mip-parsing job not followed). An anisotropic filtering row first took its place
  (`5a4e02501`, run in `r12g-rows1`: x16, x8, x4 live); told the row was only the filter, Thomas kept the game's row as it is (« remets
  comme avant ») and wished a real texture quality with the anisotropy apart for upscaled textures one day
  ([wish](../../.project/wishes/TEXTURE-QUALITY.md)); the row and its settings key withdrawn (`a3372184`). The options list does not
  scroll past the film's fourteen slots (a row takes the slot of the list's size when made; OBSERVED in code, the conclusion INFERRED); the
  film's `ScrollBarV` thumb is the mask clip `Bar`, placed by the game's own setters (OBSERVED in the disassembly).
- **What is written**:
  - the settings file's `gi_paths` (1 to 4; default 1) and `vignette` (on; D-0266's), the parse, the write and D-0265's rules (the GI
    quality available at the PT level only), unit-tested (`rekindled_settings_test`);
  - « └ GI quality » under the bounces, at the PT level: Low to Ultra, 1 to 4 GI paths a pixel (`RtReflectConstants::gi_paths` in the
    former `cut`'s place, unread since R11d): in `GiTemporal` the paths are the canonical domain's candidates of ReSTIR GI, streamed by RIS,
    each its own random salt (`GiSalt`: the path in bits 20 and up), their M counted in Z, the last frame's cap in frames (times the paths,
    the step review's M2); one path is R8d's form exactly; the one-ray `gi` pass averages them where ReSTIR does not run;
  - « Vignette », the last row: DS3's tone-map vignette drawn only with the row On (the glare's bright pass follows), live; the tone map's row
    names it (`vignette_drawn`);
  - our scroll: the page's rows a list (the preset, DS3's five, our RT rows shown), a window of fourteen built in the slots, moved when the
    game's cursor wraps past the window's edge (`ScrollAfterMove`, built again after the game's update); a rebuild keeps the cursor on its
    row of the list (the step review's M1, `FirstRowKeeping`); the film's scroll bar shown at each update of the page and its thumb placed
    when the window or the list changes (`ShowRekindledScrollBar`, `ScrollThumb`: the game's formula; the list's own scroll fields never
    written, the cursor's moves read them); unit-tested (`graphics_rows_test`);
  - frame generation held off while the NewMenuSystem's root window is open (`[14478DA40]+0xA0`, MENUS-MAP §2): DLSS-G's mode Off at each
    Present (its resources kept), back at the first Present after; a row at each change (`native_frame_generation_menu`).
- **The step review** (fresh, read-only, `cd9aa225..5a4e0250`): no High; the estimator checked (the RIS weights, Z, one path identical,
  the salts). M1 a rebuild kept the slot, not the row, when the window was clamped: fixed; M2 the history's cap in M took the GI quality's
  gain on a still view (4 + 8 samples against 1 + 8): the cap times the paths; L1 (a former file's default anisotropy turning a preset
  Custom) and the anisotropy's L3 went with the row; L6 the new keys in the settings rows: added. Left, recorded: L2 a mouse jump from the
  last row to the first is read as a wrap (the window moves instead of the cursor); L4 the one-ray pass reads the pixel's surface again for
  each extra path (small, only at the PT level without ReSTIR); L5 the menu signal's coverage (the bonfire, shop and dialogue menus) not
  checked beyond the in-game menu.
- **The first run** (`r12g-rows1`, 15:01-15:07, the ASUS, `build-r12g-rows1`, Run-AlphaCast `-OptionsTour -OptionsSteps`, no measure,
  restored): no fault. NVIDIA DLAA built the rows again; path tracing gave fifteen rows, fourteen built (`first` 0, `slots` 14), the bounces
  and the GI quality as sub-rows; the bounces to 3, the GI quality to Medium (`native_settings_live` gi_paths 2), frame generation x2 (its
  chain made again in 96 ms); the cursor down past the fourteenth slot moved the window one row (`scrolled` 0 to 1). Frame generation stayed
  off while the menu was open (`native_frame_generation_menu` held true, `on_presents` 0 through 1,200 presents) and came on once it closed
  (on at present 1,640). A fault that is not R12g's: the switch to path tracing froze the game 12.3 s (`native_slow_frame` serial 15089,
  record 12,277 ms, no pipeline state counted: the PT level's programmes compiled at their first use; DLAA's switch 2.7 s), and one key
  press was lost in it, so the vignette and the wraps were not reached. For R12d (the precompile screen with our native programmes).
- **The second run** (`r12g-rows2`, 15:41-15:48, `build-r12g-rows2` = `a3372184`, the same tools, restored): no fault. Texture Quality the
  game's row again (thirteen rows on DLAA); path tracing (fifteen rows; the freeze 9.7 s, waited out); the GI quality Low to Ultra
  (gi_paths 2, 3, 4 live); the cursor down past the window (`scrolled` 0 to 1, the vignette shown last), the vignette Off (`vignette_drawn`
  false from the next frame), down past the last row back to the first (`scrolled` 1 to 0), up past the first to the last (0 to 1); the
  scroll bar shown with its thumb 14/15 of the track at the top (`scroll_bar` y 0) and at the bottom (y 45), seen on the stills (the gap
  below the thumb at the top window, above it at the bottom one: `options-step-23` to `-27`); then, from the bottom window, Rendering back
  to Ray tracing (thirteen rows, the window clamped to 0) kept the cursor on Rendering (`rebuilt_cursor` 8). Not seen: the GI quality's
  effect on a still (no image compared; the journal's gi_paths and the shader's path are the evidence), the vignette in an area whose
  Vignette Power draws one (the Firelink start's tone map drew it: `vignette_drawn` true then false).
- **The second step review** (fresh, read-only, `5a4e0250..4eb993a5`, with the optimisation pass: the one-ray pass's extra paths from
  the first's receiver): no High; the withdrawal checked (no leftover of the anisotropy row). M1 a rebuild with the same window and list
  left the thumb as the game's bind places it (full length at the top): the thumb placed again after each build; L2 a track not read yet
  never retried: the key kept once placed; L1 the comment's « hidden at each cursor move » wrong (the stills: only the bind hides it):
  rewritten, the bar shown at each update kept as cheap insurance; L3 the history cap's comments; L4 one origin for the GI's rays
  (`GiOrigin`); L5 STATE's next action, a test of the thumb at fourteen rows and none, and a former file's `anisotropy` key ignored.
- **The integrity review** (fresh, read-only, `main...05f1d617`): no High. Its points answered here: (A) the final tree (`4eb993a5`, the
  optimisation pass; `61c056d4`, the second step review's fixes) was not run live (no live run before a merge, D-0227: the start check
  after it); its unit tests in `build-r12g-final` (below); the thumb placed again after a build with the same window, and the one-ray
  pass's averaging (the PT level runs ReSTIR by default: no run took that path in any version) are unobserved. (B) STATE's « both runs
  passed » rewritten: both runs without fault; the first lost a press in the freeze; the GI quality's and the vignette's effect on the
  image not compared; the help lines' fit in the box: the final review's L5 below.
  (C) the run builds were made from the working tree before their commits (`build-r12g-rows1` 14:40, committed `5a4e0250` at 14:54;
  `build-r12g-rows2` 15:27, committed `a3372184` at 15:39, no edit between: the journals carry those commits' rows). The changed
  assertions, stated: `graphics_rows_test`'s last row the vignette, Reflex before it, both offered (Thomas, 15:19); `rekindled_settings_test`'s
  accepted keys 11 to 13 (the two new keys) and its defaults with them. The new test tools: `rt-gi-paths-N.flag` (N 1 to 4) and
  `no-vignette.flag`, not used in the runs.
- **The final review** (fresh, xhigh, `main...HEAD` with main `6f974e10` merged): no High. M1 the GI quality's effect is shown only by the
  setting reaching the renderer (`native_settings_live` gi_paths 2 to 4): no picture, no GPU time per level and no bias check (CONTRACT:
  `Compare-GiMean.py` where the estimator changes its variance), so its effect and its unbiasedness at 2 to 4 paths are INFERRED from the
  code (the reviewer's reading against RTXDI's archive: W = 1/pdf, M = 1, risWeight = p̂·W·M, Z the N candidates, the cap before Z; one path
  R8d's form) and the exit criterion « each row seen changing what it names » open for the GI quality: the check (a PT still at
  `rt-gi-paths-1.flag` against `rt-gi-paths-4.flag` under `rt-gi-mean.flag`, `Compare-GiMean.py`; the GI pass timed with `Get-R8Gpu.ps1`)
  is the first diagnostic after the merge; the vignette's too is the CPU's choice of the tone map's programme. L1 `GiSample`'s and the
  salts' dead defaults removed; L2 the bar shown only after a build (with its thumb); L6 a test message. L5: the stills `r12g-rows2`
  `options-step-20` and `-24` show the GI quality's and the vignette's English help lines inside the box; the French ones not seen.
  Left, recorded: L3 the mouse (a jump from the last visible row to the first read as a wrap; the bar's arrows and thumb, the list's own
  range one page, probably do nothing when clicked: untested); L4 frame generation held off in the in-game menu, which does not pause the
  game (Thomas's answer to the former R pilot's question still pending, in STATE); the menu signal for the bonfire, shop and dialogue
  menus not read. Outside the diff: CONTRACT.md's scope line excludes R12, which runs under I-0132 since R12b (for the next CONTRACT edit).
- **Its fixes' step review** (fresh, `f96edfbd..69ddfcb2`): no High or Medium; every salt as before; the bar shown after the page's
  open, each rebuild and each scroll. Lows: L-a the bar hidden by another caller of the game's draw (the help box, a confirmation box, a
  screen-mode change) would stay hidden until the next build: to look at in the next options run; L-b R12G-STATIC §3 rewritten.
- **Merged** `47ace3ed` (16:35 Paris, main `2e59f89e` merged into the branch first; Test-Project passed in main); the launcher rebuilt from
  main (every test passed); start check `start-47ace3ed9` passed (16:43-16:44, the ASUS, 292 world frames, restored); the disk rotation
  (Clear-DiskCache -Apply: 6 build folders older than 48 h, 1.9 GB).
- **Open**: the GI quality's check (the final review's M1: done below, « The GI quality's check »); Thomas's answer on frame generation held off in the in-game menus; the mouse on
  the scrolled page and its bar; the bar after the help box or a screen-mode change; the French help lines in the box; the PT level's
  10 to 12 s freeze at its first live switch (its programmes compiled then: R12d, fixed there).
- **The GI quality's check** (the final review's M1; 16:54-17:05 Paris, the ASUS, the PC entirely free, `build-r12g-merged` = main's code
  at `47ace3ed`, Run-R7aGi `-Still -Measure` (no debug layer) at Firelink's bonfire, the PT level with NVIDIA DLAA and RR, 1280x720, two
  vertices; runs `i132-r12g-gi-m40-ptone`, `-ptone4`, `-ptg1`, `-ptg4`, no fault, each restored; `evidence/I-0132/artifacts/r12g-gi/`):
  - **the bias** (Compare-GiMean.py at frame 3600, about 1,800 frames each, against the one ray `ptone`; R8d's floor of two `ptone`
    launches 1.0001): ReSTIR GI at one path 1.0010, at four 1.0000 (blocks p5 to p95 0.990 to 1.009, the floor's width); the one ray
    averaging four 1.0003: **no bias at four paths** (`compare-*.txt`);
  - **the cost** (Get-R8Gpu's windows after the first, `gpu-summary.txt`): the GI pass 5.57 ms at one path, 17.58 ms at four (x3.2), the
    frame 10.99 to 23.05 ms; the one ray at four 16.81 ms (its one path not timed): the paths are traced;
  - **the displayed picture** (Get-Stability.py, the pictures 300 frames apart, Y of 255, `stability.txt`): ReSTIR at one path 0.70 to
    0.72, at four 0.68 to 0.75: **no steadier picture measured at four paths** with ReSTIR and RR on a still view; the one ray 0.69 to 0.73,
    averaging four 0.57 to 0.68. Frame-to-frame flicker not measured (no `world-burst` taken) and no moving view; the two stills at
    frame 3600 (`ptg1-vs-ptg4-3600*.png`) not told apart by eye, no figure made from them.
  The tools: Run-R7aGi's new modes `ptg1`, `ptg4`, `ptone4` (Run-StartupTrace's `-EffectFlags` now accepts `rt-gi-paths-1` to `-4` and
  `no-vignette`; additive, no flag's meaning changed); the readers `python -I Compare-GiMean.py <ptone>/world-3600-gi-mean.rgba32f
  <mode>/world-3600-gi-mean.rgba32f --size 1280x720` and `Get-Stability.py <the four runs> --size 1280x720 --apart
  2400,2700,3000,3300,3600` (the integrity review: the first written files held a wrong path's error; made again from the runs, the same
  figures; over 1800 to 3600 ReSTIR at one path reads 0.633 to 0.716, the same conclusion).
  So the GI quality is unbiased and costs about 12 ms of GI a frame at Ultra (1280x720), for no gain these measures show on a still
  view; its gain, if any, would be in motion (where ReSTIR's history is lost) or in flicker: not measured, still to do. Thomas told
  (17:10); asleep, the coordinator applied the recommended option (17:06): the row kept, Low the default, its help line factual (« costly,
  little gain on a still view »; no claim on motion).

## ROADMAP workstream 2, steps 3 and 4 (2026-11-12 evening, the R pilot): R12f's and R12g's open points

Runs only, no code (build `build-r12e-r8` = main with R12e's remainder; the ASUS, no measure, offline, each restored by hash, no
fault; the settings file a run needs written beside the module before it and removed after, Run-StartupTrace keeping its own copy aside).

- **Step 3** (R12f):
  - **Frame generation switched in exclusive full screen** (`r12f-c1-fs`, 18:06-18:11, exclusive full screen 1920x1080, Epsilon's save,
    the Advanced page's NVIDIA Frame Generation Off to x2 then back): `native_frame_generation_live` asked 2, ok, 1,359 ms; then asked 0,
    ok, 1,307 ms (the chain made again, the exclusive full screen left and given back); `on_presents` 0 in between: the in-game menu holds
    frame generation off (R12g, D-0273), so no generated frame was presented; the page's row x2 then Off (`options-step-24`, `-27`).
  - **A later launch reading the page's file**: a file beside the module with `dlss=dlaa`, `frame_rate=unlimited`, `vsync=off`, read at
    the launch (`unlock_fix` rate 1000 source file; `native_settings_live` first: rt, rr, vsync false, frame_rate 1000; the page showing
    NVIDIA DLAA, Unlimited, V-sync Off).
  - **The return to 60 after an unlocked start** (`r12g-c1-fr`, 18:50-18:56, the same start at the PT level, in French): the Graphics
    tab's Maximum frame rate list opened (E) and 60 chosen: `native_settings_live` frame_rate 0 (DS3's 60) applied live; the cadence not
    judged: the PT level at 1920x1080 ran at about 35 frames/s, under 60 either way (`native_present_pacing`).
  - **Esc on the page opened from the title**: the original does not close it either (`r12f-c2-ref`, the Reference renderer, and
    `r12f-c2-nat`, 18:12-18:20: the title's System, Game Options, then Esc, Esc, Backspace, Q: the page stays in both): DS3's own, closed.
  - The tearing-capable chain in exclusive full screen with V-sync on, and the 60 frames/s play through the installed corrections
    against the original's: not run (the second is a measure).
- **Step 4** (R12g):
  - **The French help lines** (`r12g-c1-fr`): every row of the Advanced page walked down in French at the PT level (« Anticrénelage »,
    « Netteté », « Rendu », « └ Rebonds de lumière », « └ Qualité de la GI », « Génération d'images NVIDIA », « NVIDIA Reflex »,
    « Vignettage »), each help line inside the box (`options-step-16` to `-28`); RR greyed at the PT level and skipped by the cursor.
  - **The mouse on the scrolled page**: down past the window (`scrolled` 0 to 1, cursor 13, 15 rows in 14 slots), then a press on NVIDIA
    Frame Generation's line of the scrolled window put the cursor on it (`options-step-29`; with the scroll ignored the press would have
    met the greyed RR row).
  - **The vignette where an area draws one** (`r12g-vig-on-m40-gi` and `r12g-vig-off-m40-gi`, 18:56-18:59, Run-R7aGi m40 `-Still`,
    1280x720, the second with `no-vignette.flag`): `vignette_drawn` true against false; on the same world picture
    (frame 2400, `world-2400-display.rgba`) the corners 18 % darker with it (mean luma 49.6 against 60.3), the side edges 19 %, the
    centre the same (40.1 against 40.3); side by side `evidence/I-0132/artifacts/r12g-vignette-firelink-on-off.png` (on left).
  - **The GI quality's gain in motion** (D-0273: measured in motion, the row removed if there is none): not run; it is a measure (the PC
    entirely free). A design for it: the same replayed camera turn in both GI qualities at a frame rate both hold (a cap at 30), the
    frame-to-frame change of lossless pictures (`world-burst.flag` around the turn) compared between Low and Ultra, the motion's share
    equal in both, the rest the noise; R12g's still view showed no gain.

### The measures left (2026-11-12 evening, the fresh R thread, `claude/sleepy-chebyshev-305b2f`)

- **The tools** (`f4e99bcc`, the step review's answers `a52807d2`; builds `build-gim1` to `-gim3`, 0 warnings, every unit test passed):
  - `world-burst-after-<n>.flag` (n 300, 600, 900, 1200 or 1800): world-burst.flag's 32 lossless pictures from the n-th frame after the
    first world frame instead of serial 2400 (which depends on the title's and the load's lengths), its row `world_burst_window`; the
    follow camera's yaw [+0x144] at each call in the burst (`world_burst_yaw`); without the flag the burst is 2400's as before.
  - `Pilot-Game -Action sweep -X <units> -SweepSeconds <s>`: a camera turn at a constant rate (stopwatch-paced every 2 ms).
  - [Run-GiMotion.ps1](Run-GiMotion.ps1): Firelink's save, the PT level with NVIDIA DLAA and RR, 1280x720, the debug layer off,
    `unlock-30.flag` (every run holds 30 frames/s, one game step a frame, so the same turn a frame in every mode), the burst 900 frames
    after the first world frame and a turn of 5,000 units over 20 s started 240 frames before it from the burst's row; modes `gm1`
    (`rt-gi-paths-1`, the row's Low) and `gm4` (`rt-gi-paths-4`, Ultra), by default gm1, gm4, gm1, gm4 (the repeats give the spread).
  - [Get-GiMotion.py](Get-GiMotion.py): per pair of consecutive burst pictures, the sideways shift that best aligns a band (the
    character's column left out), the mean |dY| left at that shift and its median over tiles; the yaw turned; a run marked INVALID when
    its turn missed the burst (no yaw, a shift at the search bound, a mean shift off the others'). What one shift does not explain also
    holds the orbit's parallax and the perspective: equal in every run with the same turn, so only Low against Ultra is read, against
    the spread of the repeats.
  - The step review (fresh, `1ddb08e1..f4e99bcc` and the recorder `594abfe5`): no High; M1 the new flags not in Run-StartupTrace's
    stale list, M2 the figure's comment and the character in the band, M3 no validity mark: fixed; L1 the sweep's 15 ms ticks, L2 the
    serial read from the camera's thread, L3 a 300th-frame witness inside a moved burst, L5-L7: fixed; L4 (the burst inside the turn)
    checked by M3's marks; L8 (the title film's mux waits inside the run, no audio offset) left: the recorder is a listening aid.
- **The series** (2026-11-13 05:54-06:16 Paris, the ASUS, the PC entirely free, offline, each run restored by hash, no fault; build
  `build-gim3`; log `evidence/I-0132/artifacts/ws2-gi-motion/` for the pictures):
  - **The tearing-capable chain in exclusive full screen with V-sync on** (`i132-ws2-fsv`, Invoke-ReplayRun, exclusive full screen
    1920x1080, the Firelink path, PresentMon): 22,519 of 22,531 presents `Hardware Composed: Independent Flip` (12 `Composed: Flip`
    at the switch), every one SyncInterval 1, PresentFlags 0, AllowsTearing 0; `native_present_times` p50 16.67 ms in every window after the load (p99 16.72
    to 21.19 over frames 5281 to 17281; one window at the load, frame 1321, p99 80.6 ms and a 756 ms frame); exit code `0xD3126612` with the close, as every full-control `-1080fs` run since
    2026-10-28. **Seen: no tearing flag in full screen, the presents synced.** Closes R12f's L1.
  - **The GI quality in motion** (`i132-gim1-1-gm1` to `-4-gm4`, Run-GiMotion, four runs): the camera turned 1.126 to 1.130 rad over
    each burst (a frame's step 0.031 to 0.039 rad), the aligned shift 24.9 to 25.1 px a frame in all four. Over each whole burst (the
    reader's first figures) Low (one path) read 7.152 and 7.043, Ultra (four paths) 6.672 and 6.683: about 6 % steadier, first taken as a
    gain (`3534e2aa`, the help line « a small gain in motion »). **The final review's H1 showed it false**: the bursts start at different
    points of the turn (their first yaw 0.987 and 1.152 rad for Low, 0.930 and 0.917 for Ultra; the watcher's wall-clock start, up to
    7 frames apart), and the scene, not the GI, sets most of the figure (every run's figure above 8 while the yaw is over about 0.85). Over the yaw
    range all four runs cover (0.062 to 0.917 rad; the reader now keys the pairs by the yaw, the final review's M1): **Low 6.980 and
    6.512, Ultra 6.471 and 6.576** (tiles' median 4.100 and 4.042 against 4.095 and 4.200): the qualities' means differ by 3 % in Ultra's favour on the
    aligned figure (6.746 against 6.524) and 2 % in Low's on the tiles (4.071 against 4.148), both inside the spread of Low's own repeats
    (0.47; one Low and one Ultra run 0.51 apart). **No gain resolved in motion** with two runs a quality
    (`ws2-gi-motion/get-gimotion-common.txt`; the final reader, valid runs only and the yaw unwrapped, gives the same figures), none on a
    still view (R12g); an illustration only, not evidence: the two runs' 16th burst frames side by side
    (`ws2-gi-motion/gm1-vs-gm4-burst16-crop.png`, 0.05 rad or about 1.4 frames of the turn apart). **The row is removed** (D-0273, `7f2a5fba`): the page, its words, the settings' `gi_paths` (an unknown line now, dropped by the next write); the
    GI's paths stay a test tool (`rt-gi-paths-N.flag`, Run-R7aGi's `ptg1`, `ptg4`). Its cost was R12g's (the GI pass x3.2 at 1280x720).
    The precision left: two runs a quality; a yaw-aligned burst (the module starting it from the turn) would tighten it, not run.
  - **The 60 frames/s play against the original's logic** (the mechanics path `scripts/replay/firelink-mechanics.path.json`, the ASUS,
    60, V-sync on, Validation, no measure; I-0114's `Get-Mechanics.py`): the first pair (`i132-ws2-mech-fix`, `-nofix`, 06:01-06:07,
    `build-gim3`) logged no row: mechanics-observe's rows needed locomotion-observe, which keeps the corrections out at the rate 0. The
    rows now come with mechanics-observe alone (`3534e2aa`); the pair again on the final tree's build `build-ws2-final3` (07:16-07:22, no
    fault, restored; **not on a free PC**: this branch's `build-ws2-final4` compiled meanwhile, and the presents' p99 passed 20 ms in 33
    and 41 of 61 windows against 16 and 20 in the first pair with the PC free, so the rows were sampled under hitched frames): `i132-ws2-mech-fix2` (mechanics-observe: the corrections installed at 60, k held at 1) and `i132-ws2-mech-nofix2`
    (with locomotion-observe: the corrections not installed, the original's logic, series 21's `i114-mech-n60` form; without it the
    follow camera is not seen, the step review's M1). By event, with the corrections against without: rolls 3.52-3.64 m against
    3.37-3.60 (peaks 10.4-11.1 m/s against 10.6-11.2), stops from the sprint 0.71-0.90 m in 0.44-0.48 s against 0.79-0.83 in 0.43-0.48,
    light attacks' lunges 1.26-1.28 m against 1.27-1.28, backsteps 2.43 m in 0.88-0.89 s against 2.44-2.45 in 0.90, turns settled in
    0.11-0.23 s or 0.49-0.54 s against 0.08-0.21 or 0.50-0.54, the follow camera 0.37-0.39 rad turned in 0.89-0.95 s against 0.37-0.40
    in 0.90-0.93, its recentre 1.41-1.53 rad in 0.37-0.43 s against 1.40-1.46 in 0.36-0.38 (the first recentre 0.425 s against 0.375, more
    than a row's 1/30 s: not explained). **OBSERVED, under the compile's load: the 60 frames/s play through the installed corrections is
    the original's logic within the rows' resolution** (series 21's 2 %, the camera's 5 %) but for that recentre; a pair on a free PC
    would confirm it. Closes R12f's open point. The original game itself cannot log these rows (I-0114: the Reference setup does not reach
    them); the native without the corrections is its logic.

- **Merged** `61fc3f67` (07:39 Paris, main `e20ef20e` merged into the branch first; Test-Project -MergeGate passed on the branch and
  Test-Project in main); the launcher rebuilt from main (0 warnings, every unit test passed); start check `start-61fc3f67d` passed
  (07:55-07:56, the ASUS, 366 world frames, restored); the disk rotation before it (06:20, Clear-DiskCache -Apply: 41 build folders
  older than 48 h, 11.3 GB).
- **Thomas** (2026-11-13 06:57, on the GI row's removal): « Ok donc un seul réglage pr défaut non visible pour la GI, ça me va ».
- **The final review** (fresh, xhigh, `main...f3b0f5a3`; the merge named main `9119c900`, its second parent is `c347b279`, main
  with I-0137 tranche 6): H1 the 6 % (above: withdrawn, the row removed); M1 the reader blind to bursts starting apart (the pairs keyed by
  the yaw, compared over the common range); L1 the resume rows (at the merge); L2 the present times (rewritten); L3 the merged tree
  unbuilt (built: `build-ws2-final2` failed a unit test, rekindled_settings_test's « an edited file read again » counting 13 keys where
  Write now emits 12, fixed `1f38a7fd`; `build-ws2-final3` and `-final4` (`b6db456a`, comments only beyond it) 0 warnings, every unit
  test passed); L4 the sweep went on if the game left the front, the watcher job not removed on a throw
  (fixed); L5 left: `world-burst-after-<n>.flag` without world-burst.flag does nothing and says nothing, and the yaw rows need
  camera_laws (absent under no-unlock-fix, camera-fixed-step or an observing flag at the rate 0).
- **The tests changed with the removal**: graphics_rows_test lost the GI quality row's three assertions (absent below the PT level, a
  sub-row at it with Low by default, paths 1 to 4); rekindled_settings_test lost the `gi_paths` default, parse and refusal checks and
  gained one (a former `gi_paths=4` line read as unknown and dropped by Write); its two round trips count 12 keys, not 13 (Write's
  lines without `gi_paths`).
- **The integrity review** (fresh, `main...0d82113b`): no High; M1 the mechanics pair under the compile (stated above); M2 « no gain
  measured » and « more than any gap » (rewritten: no gain resolved); M3 final4 cited before its tests (recorded since); Lows: the
  mechanics ranges of both runs, the 0.85 rad, the crop as illustration, the reader's file (all rewritten above).
- **The removal's step review** (fresh, `4ee64046..1f38a7fd`): no High; M1 the camera's fields 0 with mechanics-observe alone under
  no-unlock-fix (the original's-logic run takes locomotion-observe; Get-Mechanics notes a run without a camera; the comment names it);
  L1 the sweep's stop line kept; L2 the common range over the valid runs, the yaw unwrapped; L4 the comment on the GI's paths fixed for
  the session: fixed. L3 left, recorded: the `rt_rows` and `greyed` masks of `native_rekindled_graphics` are indexed by RtRow, so since
  `7f2a5fba` Rr, frame generation, Reflex and the vignette are one bit lower (the vignette 256, not R12g's 512).

## R12h (2026-11-12, the second R12 pilot): the precompile page and the game's window black from its creation

R12-PLAN « R12h's design », from main `8062341a`, branch `claude/r12h-precompile-page` (the coordinator's 16b and R12-PLAN merged in
three ways from its drop, no conflict).

- **What**: R12d's corner line (a line and a bar at the bottom right, over the start logos and the title) made a page of its own
  (Thomas, 2026-11-12 02:34-02:35): black over the whole output, « Compilation des shaders » / « Compiling shaders » centred in the
  titles' font, a 440-unit bar, the percentage, one dim line saying the step is long only at a first launch and after a driver update
  (`BuildPrecompileScreen`, 16b). Shown only while the work still runs 3 s and 30 frames after our first Step (the pilot's threshold for
  D-0273's « only when a compilation really happens »: a warm start is over within about 1 s on that clock, `r12d-pt2` 1,028 ms, 0.2
  to 1.3 s in R12h's runs), held at least 1.5 s, faded in over 0.25 s and out over 0.5 s; a page whose progress has not moved for 60 s
  closes with a `stalled` row. R12e's title is built only once the page is over (`Step`).
- **The game's window black** (Thomas, 2026-11-12 06:00, relayed: « le blanc agresse les yeux »): the start check `start-732bb5ed5`'s
  desktop captures (I-0135's) showed the game's window white from 18.25 s to 28.05 s, before any logo. OBSERVED: our `Start` (on the
  window's thread) ran 21.08-28.03 s and the first Present (`native_clear` 1) followed it; nothing painted the client before. Two
  parts, both under `no-black-window.flag`: `Start`'s first step fills the client black and sets the class's background black
  (`native_window_black`); from `Setup`, right after `MH_Initialize` (the module is injected as soon as the process exists), MinHook
  hooks on user32's `RegisterClassExW`/`A`, `RegisterClassW`/`A`, `CreateWindowExW`/`A` and `ShowWindow`: a class whose procedure lies
  in the game's image registered with a black background, the game's visible top-level window filled black; pass-throughs from
  `Start` (never the window procedure, no MinHook freeze at run time). The module links gdi32; Run-StartupTrace accepts
  `no-black-window` in `-EffectFlags` and removes a `no-black-window.flag` a killed run left behind.
- **Builds**: `build-r12h` (the page, every test passed), `build-r12h2` (the 3 s threshold), `build-r12h3` (the step review's Lows),
  `build-r12h4` (`Start`'s fill), `build-r12h5` (the early hooks), `build-r12h6` = `f7fbdb65` (their step review): each 0 warnings,
  every unit test passed (`overlay_text_test`, `font_raster_test` among them); clangd 0 errors at each commit.
- **The runs** (the ASUS, no measure, offline, Eta's save `shebang-zones-eta-m40` loaded by Continue and restored by hash, no fault in any):
  - `r12h-page1fr` (06:31, `build-r12h4`, French, an empty store): `native_precompile_screen` `shown:false`, done at frame 34 in 1,278 ms,
    16 pipelines (the warm-up alone; the driver cache warm, INFERRED from the time); the title and « REKINDLED » after it
    (`evidence/I-0132/artifacts/r12h-title-fr-after.png`). The desktop captures of that run were not written (the script's path not
    quoted), so it was run again.
  - `r12h-page2en` (06:33, `build-r12h4`, English, `precompile-screen-hold.flag`): the page shown 80.5 s (from frame 77; 4,830 frames and 83.6 s since the first Step), its stills at
    48 % and in its fade-out (`r12h-page-en-48.png`, `r12h-page-en-fadeout.png`); the hold kept it over the loaded world (Continue
    pressed under it), the test tool's behaviour.
  - `r12h-page1fr2` (06:35, `build-r12h4`, desktop captures every 0.1 s): white 13.95-16.33 s (2.4 s, all before our `Start` at
    16.30 s), black from 16.44 s (`r12h-window-white-15s.png`, `r12h-window-black-18s.png`).
  - `r12h-page1fr3` (07:34, `build-r12h6`, French, an empty store, captures every 0.1 s): the window appears at 13.96 s already black
    and stays black to the logos, no white frame (`r12h-window-black-at-show.png`); the only bright frames (26.6-29.0 s) are the game's
    Bandai Namco logo (I-0135's). The rows: the seven hooks installed, none refused, no window yet (13.19 s); DS3 registers its class
    « FDPclass » through `RegisterClassExW`, its procedure at RVA 0xEF5BD0 (a JMP into the protected section, 0x144F0691B; whether it reaches FUN_140EF4D10 not read), its brush 6 =
    `COLOR_WINDOW+1` (the white), and shows it with `ShowWindow` (13.77 s); `native_precompile_screen` `shown:false` (985 ms).
- **The step reviews** (fresh, high): the page (`8062341a..8f78c274`): no High or Medium; L1 the title could start under a 0x0 output
  before the page and L2 « 100 % » from 99.5 %, both fixed (`e53ad592`). The black window (`e53ad592..25dea3e4`): no High; M1 the
  registration and showing paths not read (the non-Ex calls bypass the Ex exports, a window made visible by `CreateWindowEx`): every
  path we know of hooked (`ShowWindowAsync` and `SetWindowPos` with `SWP_SHOWWINDOW` not: harmless once the class's background is
  black), the rows to say which one DS3 takes (they did: `RegisterClassExW`, `ShowWindow`); L1 the showing's process check, L2 an
  ANSI class's procedure, L3 a refused hook removed (else `MH_ALL_HOOKS` would retry it and a second refusal stop the start), L4 the
  class row's field, L5 no latch on the first window: fixed (`f7fbdb65`); L6 the game's original brush never deleted: closed by the run,
  the brush is a system colour index (6), not a GDI object.
- **The final review** (fresh, xhigh, `main...f8214068`, main merged in at `28572857`): no High. M1 no French still and R12e's word
  after a shown page never observed (`r12h-page2en`'s hold outlasted the title: Continue pressed under the page): answered by `r12h-page3fr`
  (below). Lows fixed (`build-r12h8`): L1 the
  class row's `main_proc` dropped (0xEF5BD0 read: a JMP into the protected section); L2 the body family read only while the page is
  up (`r12h-page1fr` parsed TsukuMinPro, 110 ms, for a page that never showed); L3 the class's background set black before the
  showing erases it and for the game's windows found invisible at the install; L4 one fill shared by the hooks and `Start` (its fill
  the fallback); L5 the comments on the title's order and on the 3 s clock; L6 the reviews the comments cite named; L8 « every path we
  know of »; L9 the brush note; L11 the record. Left for Thomas: L7 a button pressed under a long page reaches the game (two confirms
  go title, Continue, the world under the page, as `r12h-page-en-fadeout.png` shows under the hold): provisional choice (the coordinator's
  recommendation, applied while Thomas sleeps, 2026-11-12 07:57): input not blocked; Thomas may revisit it. L10: the hooks ran in one run (`r12h-page1fr3`), the A/B flag never run with the final build; the post-merge start
  check is the second run with them. An observation: in windowed mode the window's title bar stays the light theme's white: made dark
  (the coordinator, 07:57) by `DwmSetWindowAttribute` `DWMWA_USE_IMMERSIVE_DARK_MODE` (Windows 11; an older Windows 10 refuses it,
  ignored, its HRESULT in the `native_window_black*` rows) at the showing and at `Start`, under `no-black-window.flag`; the module
  links dwmapi.
- **The integrity review** (fresh, `main...f8214068`): no tampering, every sampled number matches its journal or capture. L1 the
  runner's change unstated, L2 « about 2 s » for a warm start (1,028 ms on the page's clock in `r12d-pt2`, the warm-up thread's own
  2.1 s), L3 the page's frame count: the record fixed above; the 3 s threshold now written as the pilot's choice for D-0273.
- **The merged tree**: `build-r12h7` (main's I-0134 merged in, `f8214068`): 0 warnings, every unit test passed; `build-r12h8`
  (`c2460b3a`, the reviews' Lows) and `build-r12h9` (`a3cddc49`, the dark title bar) the same.
- **`r12h-page3fr`** (08:24-08:27, `build-r12h9`, French, `precompile-screen-hold.flag`, twelve Shift presses at the title keeping it
  until the page is over, then Continue; no fault, restored; another thread's compile ran meanwhile, no measure): the page in French
  (« Compilation des shaders », « 39 % », the note line, inside the 1280x720 window; `evidence/I-0132/artifacts/r12h-page-fr-50-darkbar.png`),
  shown from frame 87 for 79.7 s (`done` at 4,831 frames, 82.7 s since the first Step), its fade-out (`r12h-page-fr-fadeout.png`),
  then `native_rekindled_title` `shown:true` and the title with « REKINDLED » on the next captures (`r12h-title-fr-just-after-page.png`):
  R12e's word after a shown page (the final review's M1). The title bar dark on every capture (`dark_title` 0x00000000 in
  `native_window_black_early` and `native_window_black`); the hooks' rows as in `r12h-page1fr3` (seven, none refused, FDPclass).
- **Open**: no still of a real page (a cold driver cache, the only case that shows it since D-0273, was not made: the NVIDIA cache is
  Thomas's setting); the
  page on a cold first start (how long, its stall close) unmeasured; input under the page: blocked since « R12h's L7 » below.

### R12h's L7 (2026-11-12): no input under the precompile page

Thomas chose (2026-11-12, relayed by the coordinator at 09:56) the keys blocked while the page is up, replacing the provisional
« not blocked ». Started on `claude/r12h-block-input` from main `28c39a9a` (`206c85c1`), carried on `claude/objective-ellis-da4aeb`.

- **What**: while the page is up (`BuildPrecompileScreen`, from its first shown frame through its fade-out), every input the game
  reads is held back as I-0113 gap 26's `input_focus` holds it while the game is away (04): a DirectInput keyboard read holds no key
  (its 256 bytes cleared after a successful poll), a DirectInput mouse read no motion, wheel or button; the menus' mouse object
  (23891E0, NATIVE-INPUT-LATCH's fields) no pressed edge (+8, +0x10, +0x18) and no wheel (+0x28), its held state left as read (no
  false edge from that object; the decide action reads the DirectInput buffer, below for the held inputs); the native latch forces nothing and takes its counts as seen (no press
  replayed after the page); the pad's poll returns false unread (the original's own early return while its window is inactive, its
  update's failure path keeping its bindings off), the pad's poll hooked for it even under `no-pad-focus-fix.flag`. The page sets the
  block at its first shown frame and clears it in its end (`over`, the stall close included) and in `Step`'s failure path (the overlay
  off for the session). On unless `no-precompile-input-block.flag`; a `native_precompile_input` row at the page's end counts the polls
  that held an input back (keyboard, mouse, menus' mouse) and the pad's polls skipped. A warm start, whose page never shows, is
  unchanged.
- **The first step review** (fresh, on `206c85c1`, run by the first session): H1 a mid-line comment in `Step`'s catch swallowed
  `SetScreenLayer` and the next declaration (the build failed, C2065): fixed (`99322173`). M2 a key or button held across the page's
  end reached the game as a new press (its reads under the page said « released »): a key or DirectInput mouse button seen down under
  the page now stays cleared after it until a read finds it released, and the pad's poll, read again, still answers false while one
  of its buttons (+0x248) is down, both for at most 10 s after the end (`kAfterPageMs`; a `native_precompile_input_after` row then
  counts the polls held). L3 a press counted by the window messages' latch under the page could be replayed if the page ended before
  the next poll: no press counted while the page is up (`cab5f72b`). L4 the pad's detour now installed under `no-input-focus-fix` and
  `no-pad-focus-fix` too (its behaviour there unchanged outside the page): stated above.
- **The second step review** (fresh, on `206c85c1..99322173`; its M1 the held inputs, answered by `cab5f72b` above): L1 an overlay
  lost for the session (its open or record failed: no page drawn) kept the input held until the precompile's end or the stall close:
  the block is not set, or is released with a `native_precompile_input` `released:"overlay_lost"` row, once the overlay's own failure
  is kept (`OverlayLost`; the renderer no longer lets the screen layer's open failure overwrite the overlay's message); L2 a refused
  pad hook looked like an unpolled pad: `pad_hooked` in the page's row, `observe_off` only when the observer was on; L3 this record's
  status (below) (`6371827e`).
- **Thomas's layout** (2026-11-12 10:06, « allonger la barre … et les placer en bas de l'écran, toujours centré »): the bar 840 units
  long (was 440), the heading, bar, percentage and note moved to the bottom (baselines 556, 580, 620, 678 of 720), centred; the
  sizes kept (read as: the bar longer, the whole block at the bottom). The page's black background untouched (another session's
  work on it, Thomas 10:20) (`57b27435`). Then (Thomas, 10:45, « raccourcir un peu la taille de la barre, du texte etc »): the bar
  700 units, the heading 29, the percentage 19 and the note 15 units (were 34, 22, 17), baselines 576, 596 (the bar), 630, 680
  (`7fe03d65`); approved on `r12h-l7-layout2`'s still (`evidence/I-0132/artifacts/r12h-l7-layout2-page.png`; Thomas 11:09: « c'est
  bien comme ça », with a contrast under the text for the coming background, its second tranche).
- **Builds**: `build-r12h-input4` (`57b27435`) and `build-r12h-input7` (`7fe03d65`, the final tree, main `8f415270` in): 0 warnings,
  every unit test passed; clangd 0 errors at each commit (`build-r12h-input1` to `-input3` and `-input6` abandoned: the first failed
  on H1, the others stopped for later changes; `-input5` the merged tree before the Lows, the same outcome); their logs
  `evidence/I-0132/build-r12h-input4.log`, `-input5.log`, `-input7.log`; the clangd checks seen in its output, not kept.
- **`r12h-l7-layout2`** (11:06-11:10, the ASUS, no measure, offline, `build-r12h-input7`, the same protocol as below, one Enter under
  the page; exit 0, no fault, restored by hash): the smaller layout; `native_precompile_input` `blocked:true`, `keyboard` 32, `mouse`
  2,624, `menu_mouse` 27 polls held, `pad_polls` 0, `pad_hooked:true` (the final review's L2 field, built and run).
- **`r12h-l7-block1`** (10:25-10:29, the ASUS, no measure, offline, `build-r12h-input4`, French, `precompile-screen-hold.flag`,
  Epsilon's save `shebang-zones-epsilon-m45` (Run-AlphaCast's default) by Continue, restored by hash; exit 0, no fault): the page shown
  for 79.3 s (`done` at 4,831 frames, its end about 10:27:37), at the bottom (`evidence/I-0132/artifacts/r12h-l7-page-layout-1.png`).
  Under it, the runner's first loop (six presses on the first notice's OK at 648,427, 10:26:37-10:27:28) and five keys from a second
  pilot (Enter, Space, Enter, Escape, Enter, 4.8 to 17.1 s after the title's movie; `r12h-l7-block1-keys.txt`) changed nothing:
  `native_precompile_input` `keyboard` 148, `mouse` 2,032, `menu_mouse` 21 polls held, `pad_polls` 0 (no pad on the PC); the first
  offline notice still up after the page (`r12h-l7-title.png`, 10:28:10). The runner's second loop aimed at the second notice's OK
  (y 450, outside the first notice's button) and fell mostly after the page, so it proves nothing either way. The two notices
  (`menu_window_close`, vtable 43029208) closed about 35 s and 43 s after the page's end, by the two `-TitleSteps` presses
  (`title-step-1.png`, `-2.png`), then Continue loaded the save. The A/B, with earlier builds without the L7 code (`r12h-page2en`
  `build-r12h4`, `r12h-page3fr` `build-r12h9`, Eta's save; `no-precompile-input-block.flag` itself never run): the same notices closed
  13 s and 20 s after the title's movie, under the page. The two closes of vtable 43092088 before the title's movie, in every run,
  are not input. `native_precompile_input_after` `held_polls` 0 (nothing held across the end).
- **The final review** (fresh, xhigh, `main...8c426a7e`): no High, no Medium in the code; M1 the record (fixed above, the integrity
  review's same points), M2 HEAD unbuilt then (`build-r12h-input5`, the merged tree `8c426a7e`: 0 warnings, every unit test passed).
  Lows fixed: L1 the block held again when a device reopen brings a lost overlay back; L2 the row's `blocked` says whether it was
  ever set; L3 after the page, a held key or button no longer turns the latch off for the others (a held input sends no new press
  to count); L5 the end's tick stored before the block clears; L6 `AfterPage` idle once its window is over, `PageMenuMouse` void;
  L7 a comment back on its function; L8 `no-precompile-input-block` in Run-StartupTrace's `-EffectFlags` and its stale-flag cleanup;
  L10 the bar within 90 % of an output narrower than 7:6. Left: L6's string copy per page frame (`OverlayLost`, kept: one short
  copy while the page is up).
- **Open**: the pad untested live (no pad on the PC), and its after-page hold reads its digital buttons only (+0x248: a trigger held
  across the end may still arrive as a press, the final review's L4); a key held across the page's end not run (`held_polls` 0 here);
  the window messages' latch not exercised (`native_input_latch_observer` `installed:false` in this run as in R12h's, the final
  review's L9); `no-precompile-input-block.flag` never run; a real page on a cold driver cache still unseen.

### R12h's second tranche (2026-11-12): the precompile page's living background

Thomas (10:36-11:10, relayed by the coordinator and in this thread): his picture behind the page (a kneeling knight, his sword planted
in glowing rocks, a cathedral at the right; made by his own image session), « rendre l'image vivante »: flames where the sword
enters the rocks, embers, ash, dust, mist and fog, the tattered cape moving, wind over the whole scene, a fire's crackle, and the
text and percentage in contrast over it. Branch `claude/objective-ellis-da4aeb`, from main `1ecd0877`.

- **What** (`page_fx` in 16b, before `BuildPrecompileScreen`):
  - **The picture**: `rekindled-precompile-bg.jpg`, then `.png`, beside the module or in `%LOCALAPPDATA%/DS3-DX12` (Thomas's PC only,
    not in the repository: his `Chevalier DS3.png`, 3840x2144, copied there as the `.png`); decoded by WIC on a thread of its own
    from the page's first shown frame (`native_precompile_background` row: 132 to 164 ms in the runs), scaled to the lines the
    output shows of it (1354x756 for the 1280x720 window), faded in over 0.4 s once ready; it covers the output with a push-in of 4 %
    over a minute. Without it, or with `no-precompile-background.flag`, the page stays R12h's black page.
  - **The effects**, placed in the picture's own coordinates: the cape's region cut in tiles of about 14 output pixels sampling the
    picture a little aside (a wave travelling left with the wind, none at the shoulders, the body, the rocks or the picture's edge);
    an animated fire at the sword's foot (four tongues from a 32-frame atlas of value noise rising through a warped tongue, baked once
    on the picture's thread, two frames crossfaded); a flickering glow there and at the ground's other embers; a heat haze above the
    sword (tiles sampled a texel or two aside); embers rising from the fire, the cape and the ground as short streaks; ash and dust;
    nine fog banks in three layers; one gusting wind pushing all of them to the left (as the picture's cape and debris go).
  - **The text's contrast**: the bottom 30 % darkened in bands under the bar and text; the note line lighter (0.66 for 0.50) when
    the picture is drawn; the titles' shadow as before.
  - **The crackle**: a fire's sound synthesised once (8 s, a rumble, a hiss, crackles and a few pops, its seam crossfaded) and looped
    by winmm's `PlaySound` from memory while a drawn page is up; stopped at the page's end, at Step's failure paths, when the overlay
    is lost and at the native close; one try only. `no-precompile-sound.flag` turns it off. Its recipe rendered for Thomas:
    `evidence/I-0132/artifacts/r12h-fx-crackle-16s.wav` (the same algorithm in Python). R's fire sound from the game's own FMOD
    banks, when found, may replace it (the coordinator, 11:45).
  - **Freed** at the page's end: the picture, the fire's atlas and the sound's bytes (the overlay pass keeps its uploaded textures
    until its close, as for every image it has drawn).
  - The module links `windowscodecs`, `ole32` and `winmm`.
- **The step review** (fresh, on `affd1e49..629363a5`): H1 the heat haze, drawn after the fire, hid it: drawn first now. M1 the sound
  could play without a drawn page (an overlay lost): only with a drawn page. M2 a failed `PlaySound` retried every frame: one try.
  M3 the fire's texture made and uploaded every frame: one atlas baked once. M4 the picture and the sound held for the session:
  freed at the page's end. Lows: L1 the height checked against the overlay's 8192, L2 nothing thrown out of the decode thread, L3 the
  picture's fade-in, L4 the note's colour only with the picture, L5 the moving tiles only once the picture is opaque (patches during
  the fades), L6 none at the picture's edge, L8 the sound stopped at the native close, L9 the comments, L10 the dust's wrap
  (`eae813fc`). Left: L7 (the picture decoded for the first frame's output size: a window made larger during the page shows it a
  little soft); L10's duplicated noise and dot of `title_fx` (kept apart while R edits `title_fx`).
- **Thomas's eye** (12:43-13:10): on `r12h-fx-page3`, « l'effet que t'as mis sur la cape, mets-le sur le feu de l'épée aussi […] sinon
  c'est cool »: the picture's own fire and sparks at the sword's foot wave as the cape does, faster and rising (the haze's region
  widened to 0.64-0.77 x 0.48-0.775, tiles of 6 pixels up to 50x80, up to four times the haze's shift on the flames), the animated
  fire fainter over it (`679f78be`); then (13:10) « l'effet un peu trop rapide »: the atlas played at 0.6 and the fire's wave at 0.6
  of its speed, then (13:11-13:12, « l'ondulation surtout », « la même vitesse que pour la cape ») the wave at the cape's speed in time (2 radians a second). Asked (12:10) whether the bonfire's own effect could serve: the game's particle effect needs the world, after the page;
  its flame textures may serve if loaded by then: the next tranche's first question.
- **The final review** (fresh, xhigh, `main...679f78be`): H1 main had moved (R12e's second look, `ea7d22e0`) and the merge conflicted
  in Step's two catches: merged (`0ad37e67`), both main's tint layer reset and the crackle's stop kept, main's `OverlayLost` (tint
  failures ignored) kept. M1 the crackle survived an overlay loss with the input block off: stopped whenever the overlay is lost (it
  goes on while the output is 0x0, the page still up). M2 this record (this bullet and the next). Lows: L1 a decode finishing after
  the release keeps nothing; L3 the native close stops the sound first, before its deferred-close return; L4 the crackle made on the
  picture's thread; L5 the quads reserved for the clamped maximum; L7 the comments; L8 the placed layers (fire, glows, the cape's and
  the fire's waves) only on a picture of Thomas's picture's shape (3840x2144 within 0.5 %), another picture getting the wind's layers
  alone. Left: L2 the picture's texture kept in the overlay pass until its close (about 33 MB at 4K); L5's per-frame tile weights;
  L6 the noise and dot duplicated with `title_fx` (R's rewrite now in main; a later merge of the two); L9 the tiles stop at the
  fade-out's start (a jump of the waves for 0.5 s), the two crossfaded fire frames lose some coverage mid-blend, `PlaySound`
  bypasses DS3's FMOD volume settings.
- **The second step review** (fresh, on `679f78be..0ad37e67`, the merge and the final review's answers): no High or Medium; the merge
  correct, nothing of main lost; no race on the shared picture, atlas and sound. Lows fixed: L1 the embers' fire and cape sources
  only on Thomas's picture; L2 only a failed `PlaySound` stops the retries (a stop when the overlay is lost lets the sound start again
  with the page); L3 Step's catches free the page's picture, atlas and sound too; L4 the crackle made after the picture is published
  and not at all under `no-precompile-sound.flag`.
- **Builds**: `build-r12h-fx1` and `-fx2` refused by MSVC (C4459: locals hiding the module's `image` and `output_width`, which
  clangd did not report), `-fx3` (`8f9a777e`), `-fx5` (`629363a5`), `-fx6` (`eae813fc`), `-fx7` (`8dfefb63`), `-fx9` (`679f78be`, main
  `d689149c` in): 0 warnings, every unit test passed (logs `evidence/I-0132/build-r12h-fx*.log`); `-fx4` and `-fx8` stopped for the
  next change; `-fx10` to `-fx12` stopped for the next ones; `-fx13` (`c617cff9`, the final code with main `ea7d22e0` in): 0 warnings, every unit test passed.
- **The runs** (the ASUS, no measure, offline, `precompile-screen-hold.flag`, French, Epsilon's save by Continue, restored by hash, each
  exit 0 and no fault; `-VideoTitle` films):
  - `r12h-fx-page1` (11:38-11:41, `build-r12h-fx3`): the picture and its effects drawn
    (`evidence/I-0132/artifacts/r12h-fx-page1-a.png`, `r12h-fx-page1-12s.mp4`), `PlaySound` started (`native_precompile_sound` `playing:true`; the films carry no sound); Thomas (11:44): « les effets font
    un peu cheap mais c'est pas mal »; the drawn flames a hard block (`r12h-fx-fire-zoom.png`), the cape's wave barely seen.
  - `r12h-fx-page2` (12:20-12:23, `build-r12h-fx6`): the animated fire still read as a drawing, a hard triangle left of the blade
    (`r12h-fx-page2-fire.png`).
  - `r12h-fx-page3` (12:36-12:39, `build-r12h-fx7`): the fire's licks along the blade with soft edges (`r12h-fx-page3-fire.png`,
    `r12h-fx-page3-a.png`, `r12h-fx-page3-15s.mp4`); input held as in L7 (`keyboard` 28, `mouse` 2,604, `menu_mouse` 27 polls).
  - `r12h-fx-page4` (13:02-13:05, `build-r12h-fx9`): the picture's fire waving (`r12h-fx-page4-15s.mp4`, `r12h-fx-page4-fire-6s.mp4`,
    `r12h-fx-page4-a.png`); background 140 ms, `PlaySound` started, input held (28, 2,628, 27).
  - `r12h-fx-page5` (13:45-13:48, `build-r12h-fx13`, the final tree): the page as merged (`r12h-fx-page5-a.png`, `r12h-fx-page5-15s.mp4`);
    background 135 ms, `PlaySound` started, input held (32, 2,596, 27); R12e's title shown after the page (`native_rekindled_title`).
- **Open**: the final tree run once (`r12h-fx-page5`), the fire's speed not yet seen by Thomas; the page at other resolutions and
  aspects not run (the layout follows the picture's coordinates); the frame time of the page's effects on the engine thread
  unmeasured (up to about 12,800 quads at 4K, the tiles clamped); the page on a cold driver cache still unseen. Next tranche:
  Thomas's music under the page (« Prologue », Yuka Kitamura, his FLAC copied to `%LOCALAPPDATA%/DS3-DX12` as
  `rekindled-precompile-music.flac`, not in the repository; the game's own opening film keeps its narration in every track,
  `fdp_opening.bik`'s five, previews `evidence/I-0132/artifacts/prologue-*.mp3`), mixed with the crackle, and the bonfire's flame
  textures if loaded by the page's time.

### R12h's third tranche (2026-11-12): music under the precompile page, and the fires' sound for R

Thomas (12:46-13:08): a music of the game under the page, the prologue's; the game's opening film `movie/fdp_opening.bik` (3:18, five
Bink audio tracks: stereo, two mono, two stereo; `evidence/I-0132/artifacts/prologue-fdp_opening-ffprobe.txt`) keeps its narration in every track and mix tried (previews
`evidence/I-0132/artifacts/prologue-*.mp3`: without the centre, the rear pair alone, the centre cancelled; Thomas: « on entend encore
la voix »), so Thomas put the soundtrack's « Prologue » (Yuka Kitamura, his FLAC, 3:10, 44.1 kHz, 24-bit) on his PC: copied to
`%LOCALAPPDATA%/DS3-DX12/rekindled-precompile-music.flac`, not in the repository. Branch `claude/objective-ellis-da4aeb` from main
`5e03f6b8`.

- **What** (16b `page_fx`): the page's sound is one stereo 16-bit `waveOut` stream mixed on a thread of its own (four 100 ms
  buffers): the music (`rekindled-precompile-music.flac`, `.mp3` or `.wav` beside the module or in `%LOCALAPPDATA%/DS3-DX12`, decoded
  once to 16-bit PCM by Media Foundation's source reader on the picture's thread at its own rate) faded in over 2 s and looped, the
  crackle (made at the stream's rate) under it; at the page's end both fade out over 1.5 s. The game's title music
  (`FUN_14076B920(id)`, I-0069 MENU-AUDIO-NATIVE §3) is held from the music's decode (a MinHook detour, 15 `HoldTitleBgm`) and let go
  once the page's sound has faded, by `rekindled_overlay::Step` on the engine thread (the thread of the game's own request): one music
  at a time; the game's BGM stop (`FUN_14076B9F0`, detoured too) drops a held id. Without the
  music the crackle plays alone and nothing is held. `no-precompile-music.flag`: the crackle alone. The module links `mfplat`,
  `mfreadwrite`, `mfuuid`.
- **The step review** (fresh, on `5e03f6b8..9ff76a98`): no High or Medium. Lows fixed (`1b4e46fc`): L1 two musics at once (a title
  request during the decode, the overlap of the fade, a restart after a lost overlay): the hold from the decode, let go after the
  fade, no music on a restart; L2 a failed stream lets the title's music go; L3 a failed write ends the mixer (a dead field removed);
  L4 the reader stops on its error and format-change flags; L5 the decode's cleanup on an allocation failure; L6 the length cap at
  the stream's rate; L7 the picture's row times the picture alone; L8 the hold's rows carry their thread; L9 sound-observe's distinct
  mode only under its flag (the Reference's witness keeps its first 120 rows).
- **The fires' sound for R** (the coordinator, 13:57: R needs the FMOD events of the bonfire's crackle): the world's sounds do not go
  through `FUN_1404AD510`: in `r12h-mu-page1` at Firelink it saw five distinct requests, the menus' four and the zone's `y400000000` (the
  `s`, `z` and `c` events of the world do not pass there); the existing `audio-census.flag`
  (FMOD::Event::start, NATIVE-AUDIO-CENSUS.md) names every event started. Idle 30 s at three bonfires: `r12h-fire-census-m40`
  (14:27-14:30 by its journal, Eta at Firelink), `r12h-fire-census-m30` (14:30-14:32, Theta at the High Wall), with I-0066's `i66-native-1574-m31-audio`
  (Beta at the Undead Settlement). The « s » events common to the three: `s444444441` at m30 and m31, `s444444444` at Firelink (m30 also starts
  `s000180000`, at neither other bonfire: set aside); besides them each zone's `y` (`y300000000`, `y310000000`, `y400000000`) and `a`
  families (zone sounds; no `a` event at m40), and m31's `z003020001`; a « z » family at all three with a number per zone: `z006000001` (m30), `z006000002` (m40), `z006000004` (m31); the zones'
  music `m400100000` (Firelink), `m300100003`/`m300100004` (High Wall). INFERRED, not heard: the bonfire's crackle is `s444444441`
  (Firelink's own `s444444444`, why another number not read), the `z006` family a second candidate. Played from the module as the menus play theirs:
  `FUN_1404AD510(&handle, category, id, -1)` (`s` = 5, `z` = 12, `m` = 6 in the table `142786578`). Sent to the coordinator for R.
- **Builds**: `build-r12h-mu1` refused by MSVC (C4459: `opened`), `-mu2` (`9ff76a98`) and `-mu3` (`1b4e46fc`): 0 warnings,
  every unit test passed; `-mu4` refused (a mid-line comment of the final review's L4 fix had swallowed the mix's declaration, C2065;
  clangd reported nothing), `-mu5` (`108900a0`, the final code) and `-mu6` (`1064c4c8`, main `50b89d24` merged in): 0 warnings, every unit test passed (logs
  `evidence/I-0132/build-r12h-mu*.log`).
- **`r12h-mu-page1`** (14:19-14:23 by its journal, `build-r12h-mu2`, Eta at Firelink, `precompile-screen-hold` and `sound-observe`; no fault,
  restored): `native_precompile_music` found, decoded 190.6 s at 44,100 Hz in 313 ms; the stream opened with the music; the title's
  `m1` held (`native_title_bgm` `held:1`) and let go after the page (`released:1`). Not heard (the films carry no sound).
- **`r12h-mu-page2`** (14:47-14:50, `build-r12h-mu3`, the final code, Epsilon by Continue; no fault, restored): the music decoded in
  366 ms, the stream opened with it, `m1` held on thread 37552 (the game's request) and let go on thread 38332 (a menu movie advance
  after the page): not the same thread, no fault seen, the sound object's thread rules not read.
- **`r12h-mu-page3`** (15:18-15:22 by its journal, `build-r12h-mu5`, the final code, Epsilon by Continue; no fault, restored): the
  music decoded in 291 ms and played; `m1` held and let go after the page, both on thread 37868 (the engine thread, `native_start`'s).
- **The final review** (fresh, xhigh, `main...7b4ed949`): no High. M1 a lost overlay let the title's music go while the page's still
  faded: only a mixer that never launched lets it go at once, a launched one at its end. M2 the held request replayed on a menu task
  thread, not the game's engine thread (`r12h-mu-page2`: held on 37552, the overlay's thread, replayed on 38332), with the sound
  object unlocked: replayed from `rekindled_overlay::Step`. Lows: L1 the BGM stop detoured (a Continue right after the page); L2 the
  mixer counted as playing once its thread exists; L3 its buffers sized before the stream opens; L4 the crackle alone at the second
  tranche's level; L5 Media Foundation delay-loaded (absent, no music); L6 a format change on the first sample kept at the same rate
  and layout; L7 this record (`a` at m40, the two L8). Left: L6's 2x vector growth and the `.mp3` and `.wav` paths not run.
- **After the merge** (`9e45d637`): the launcher rebuilt (0 warnings, every unit test passed), start check `start-9e45d6370` passed
  (15:46-15:48, 354 world frames, restored); the disk rotation (`Clear-DiskCache.ps1 -Apply`, 15:50): 1,752 files, 1.6 GB, five build
  folders older than 48 h, 372 GB free.
- **Open**: Thomas's ear on the mix (levels: music 0.8, crackle 0.55 under it); the bonfire's flame textures for the page's fire
  (Thomas, 13:10) looked at in the fourth tranche (below).

### R12h's fourth tranche (2026-11-12): the page's fire with the bonfire's own textures

Thomas (13:10, « tu peux pas récupérer les effets du bonfire pour le feu de l'épée ? »; then 13:10, « ton feu est bien sinon […] un peu trop rapide »). The bonfire's effect is `f000001020`
(WAVE1-LAST-EFFECTS-STATIC-PREP §2.1): a flame body of three scrolling texture layers (FFX textures 11409, 10000, 11409), sparks (2000,
2001), a heat haze (6000). The game's particle system needs the world (after the page); its textures may serve. Branch
`claude/objective-ellis-da4aeb` from main `c5b9fafa`.

- **The probe** (`r12h-ff-probe1`, 16:04-16:08, `build-r12h-ff1`; no fault, restored): the texture ingress holds `s11409` (512x512,
  BC1), `s10000` (128x128, BC1), `s02000` (128x128, BC7), `s02001` (256x256, BC1) and `s06000` (128x128, BC1) while the page is up
  (`native_precompile_bonfire_texture` rows; none of their `_a` variants).
- **The look** (`r12h-ff-look2`, 16:28-16:32, `build-r12h-ff2`, `precompile-bonfire-textures-test.flag`;
  `evidence/I-0132/artifacts/r12h-ff-look2-textures.png`): `s11409` and `s10000` grey tileable flame and cloud noise, `s02000` a smoke
  puff with alpha, `s02001` black and white veins, `s06000` a normal map. Not a sheet of flames: the bonfire's shader multiplies
  scrolling noise through a flame's shape. (`r12h-ff-look1`, 16:25, never started (its key pilot found no game window, `r12h-ff-look1-keys.txt`; the runner's refusal not kept): Run-StartupTrace's second flag group refused the new
  flag; both groups now take it and `no-precompile-input-block`.)
- **What**: the overlay gains a fire mode (`overlay::Quad` `image2`, its texel rectangle and `mode`, defaults leaving every other quad
  as before; `GpuQuad` 80 bytes; a second static sampler repeating): the quad's image alpha is a heat field, the second image's red
  channel breaks it up (heat x (0.25 + 1.5 noise)), the colour from the fire's ramp, the quad's alpha a gain on the output (since
  the step review's M1). The page's four tongues take
  their heat from the second tranche's atlas and the bonfire's `s11409` rising through them (0.22 of the texture a second, a different
  offset per tongue); `s02000` puffs rise from the flames' tips and drift left with the wind. Both textures looked up by name every 30
  frames until found, held until the page ends; without them the fire stays the second tranche's.
- **`r12h-ff-fire1`** (16:48-16:52, `build-r12h-ff4`; no fault, restored): the flames broken up as the bonfire's
  (`r12h-ff-fire1-fire.png`, `r12h-ff-fire1-fire-6s.mp4`); the fire's wave in 6-pixel tiles at four times the haze showed steps on the
  blade's edge: 4-pixel tiles, at most 2.5 times (`a7b35bb2`).
- **The step review** (fresh, on `c5b9fafa..e889f409`): no High. M1 the crossfade weight scaled the heat, so the fire's colour
  and size pulsed at the atlas's frame rate (computed; seen in `r12h-ff-fire2`, 17:09-17:13, `build-r12h-ff5`: a strip at 20 frames a
  second alternates yellow and red cores, `evidence/I-0132/artifacts/r12h-ff-fire2-pulse-20fps.png`; a 40x40 core's mean green at 20
  frames a second alternates 72-134, `r12h-ff-fire2-core-green.txt`): the quad's alpha is now a gain
  on the output, the heat unscaled (a crossfade and the page's fade stay linear). M2 the fire's 4-pixel tiles held only below about
  1540 output pixels (the count clamped): the budget moved from the cape (75 x 75) to the fire (70 x 110). Computed, not run (every
  run used the 1280x720 window, where no clamp binds): the fire's tiles stay 4 pixels to about 1200 lines across and 1430 down (4.8
  pixels wide at 1440 lines), the cape's 14 pixels to about 1330 lines (1600 before); a displacement in a shader would lift both
  limits (not done). Lows: L1 the probe only with the test tool's
  flag; L2 the noise's rectangle from its own size; L3 the overlay pass keeps its own reference to an external texture until it
  closes (stated); L4 Valid's new rules unit-tested (`overlay_text_test`); L5 (a one-frame index shift when the textures first
  appear, all blocks in flight) left (`7da4ecda`).
- **Builds**: `build-r12h-ff1`, `-ff2`, `-ff4`, `-ff5` (`a7b35bb2`): 0 warnings, every unit test passed; `-ff3` refused by MSVC
  (C4456: a local `px` hiding Draw's); `-ff6` (`7da4ecda`): 0 warnings, every unit test passed (`overlay_text_test` 9 checks); `-ff7` (the merged tree, `defa0ed1`): 0 warnings, every unit test passed.
- **`r12h-ff-fire2`** (17:09-17:13, `build-r12h-ff5`: the 4-pixel tiles at 2.5 times): the pulse above. **`r12h-ff-fire3`**
  (17:32-17:36, `build-r12h-ff6`, the final code; no fault, restored): the core's mean green moves smoothly (106 to 145 over the
  first second, no frame-to-frame alternation, `r12h-ff-fire2-core-green.txt`), `r12h-ff-fire3-fire.png`, `r12h-ff-fire3-fire-6s.mp4`;
  whether the steps on the blade's edge are gone judged by eye only on these stills (none seen at 1280x720).
- **`r12h-ff-fire4`** (17:57-18:00, `build-r12h-ff7`, the merged tree; no fault, restored): the page and its fire up to the hold
  (`evidence/I-0066/artifacts/r12h-ff-fire4/notice-2.png`, 77 %), then the title. Merged `de5301e3`.
- **The final review** (fresh, xhigh, `main...5c2ef024`): no High. M1 the bonfire's noise fell through the tongues (v grows
  downward): it rises now (`+t`). M2 main's R12e remainder conflicted in Run-StartupTrace's flag lists: merged as the union (`6749f574`;
  16b merged clean). Lows: L1 the two crossfaded quads per tongue are linear per quad, their union dips a little mid-crossfade (no
  alternation measured in `r12h-ff-fire3`); L2 the comments; L3 the probe removed (it had answered), the look tool's lookups every
  30 frames; L4 the two textures keyed by name like the menu pieces (D-0161's exception for the page's decoration, on Thomas's PC as
  the game installs it): `s02000` drawn only as the BC7 puff seen, `s11409` only at a usable size; L6 two more unit cases (`image2`
  below -1, a plain quad naming a second image); L7 Thomas's ask timed 13:10 (both his messages). Left: L5 (the larger Quad and
  GpuQuad for every overlay user, about 1 MB reserved per page frame).
- **Open**: Thomas's eye on the new fire (before: `r12h-fx-page3-fire.png`; after: `r12h-ff-fire3-fire.png`); the sparks (`s02001`) and the haze's normal map (`s06000`) not used; the bonfire's colour ramp and HDR
  gain (`color3` 2) approximated.

### R12h's fifth tranche (2026-11-12): the game's own fire sound under the page

Thomas (18:17, to the coordinator's question « tu veux que ce soit aussi le vrai son du jeu ? »: « Oui »): the page's fire sounds with
the game's own bonfire event instead of the second tranche's synthesised crackle. Branch `claude/stoic-edison-56c05d` from main
`de5301e3`.

- **Whether FMOD answers under the page** (read in the DS3 mirror, GhidraMCP): the issue `FUN_1404AD510` ends in `FUN_1407CEC40`:
  unless the sound is muted (`[144769280]`, a stored handle copied) it calls `FUN_141A2CFE0`, which takes the sound system's singleton
  from `FUN_141A37810` (`[1449323B8]`, **made on first use**) and answers the invalid pair `[144932300]` unless the singleton's byte
  `+0x698` is set, then `FUN_141A29D00` (nothing for the invalid pair, else a message on the singleton's `+0xD0` queue). So an early
  call cannot fault but could make the singleton before the game does: the module reads `[1449323B8]` and its `+0x698` itself
  (`SoundReady`) and issues nothing until both are set.
- **What** (16b): `title_crackle` (R12e) moved above `page_fx`; the page's drawn frames call `StartFromPage` (every 0.5 s, 40 issues
  at most while the sound system is ready and the issue comes back invalid; rows `native_precompile_fire_sound` `waiting`, `started`
  or `gave_up`); the event so started goes on into the title (the title's switch finds it playing); an overlay lost stops it only if
  the page started it. The synthesised crackle (`MakeCrackle`, its buffers and gain) removed; the `waveOut` mixer plays Thomas's
  music alone (no stream without it). `no-precompile-sound.flag`: no sound under the page; `no-precompile-music.flag`: the fire's
  event alone; `no-title-crackle.flag` (no observer and no `title_crackle_stop`; R12h's `DropTitleBgm` stays) leaves the page's fire event out too: never an event without its
  stop. The fire's level is the game's SE volume; the music stays at 0.8.
- **The step review** (fresh, `de5301e3..46f4efef`): no High. M1 the page's tries went through `Start`, which clears the title's
  `pending` and leaves `failed` set after a refused issue: the title's own start could be lost for that visit; the page now issues
  through `IssueLocked`, touching neither. L1 an overlay loss reset a title start held under the page: `StopFromPage` stops the handle
  alone. L3 `no-title-crackle.flag`'s reach said in the comment. L4 `FUN_1407CEC40`'s path read (above). L2 (the page's event alive at
  the title) left to the run (`c4c7e581`).
- **Builds**: `build-r12h-fs1` (`46f4efef`) and `-fs2` (`c4c7e581`): 0 warnings, every unit test passed (logs
  `evidence/I-0132/build-r12h-fs*.log`). A loopback recorder of this thread's was withdrawn for the R pilot's (`594abfe5`,
  `Run-AlphaCast -AudioRecorder`, with its `-TitleHoldSeconds`, a wait at the title inside `-VideoTitle`'s film, not used
  here; merged into the branch).
- **`r12h-fs-sound1`** (18:59-19:04 by its baseline and restoration, the ASUS, `build-r12h-fs2`, `precompile-screen-hold` and `audio-census`, French, Epsilon's
  save, `-VideoTitle -AudioRecorder`; exit 0, no fault, restored): **the sound system ready at the page's first pictured frame**
  (`native_precompile_fire_sound` started, `tries` 1, `after_ms` 0), `audio_request_name` 5/444444441 from the page's own call
  (`4AD545` in its stack) then `audio_event_name` s444444441 result 0 at the same tick; the music decoded and played; `m1` held and let go after the page; no `stopped` row (Continue did not
  load within the run, as in `r12e-r1-page`). The recording (`evidence/I-0066/artifacts/r12h-fs-sound1/title.wav`, 96.5 s from the
  FromSoftware logo under the page; `title-sound.mp4`; light copy `evidence/I-0132/artifacts/r12h-fs-sound1-page-title-light.mp4`):
  the music throughout at about -15 to -34 dBFS RMS a half second; above 2 kHz short broadband ticks every 0.1 to 0.3 s (about -60 to -70 dBFS
  in the spectrogram's bins, the music's partials there -40 to -55), under the page (40 to 46 s) and at the title (88 to 94 s) alike (`evidence/I-0132/artifacts/r12h-fs-spec-*.png`):
  INFERRED the fire's crackle (the same ticks over two different musics, Thomas's FLAC under the page and the game's m1 at the title; not recorded without the event; the same event at the title validated
  by Thomas's ear at 18:33, STATE), its loudness against the music for Thomas's ear.
- **The integrity review** (`main...e7cf939f`): no High or Medium; Lows answered in this section (the stack's row, the music's
  range, the run's times, `-TitleHoldSeconds` stated).
- **The final review** (fresh, xhigh, on `bce3390f`): no High. M1 the page's event outlived a page ending before the title's music
  (on a quick precompile, about 5 to 7 s of crackle over the publisher logos, neither page nor title drawn): `PageOver` at the page's
  end and in Step's catches stops it unless the title's music is there to take it over (a held start, the menu sound object's music at
  1, or the title's m1 held for the page's music), the title's switch then issuing it again (`8ee450f3`, its own step review below).
  L1 the retry's cause (an invalid pair) not read: said INFERRED (the name is resolved later on the sound thread); the three
  functions' reads stay in the Ghidra project, not exported. L2 the wordings above. L3, L4 (the R pilot's recorder: a failed
  capture call pads silence and exits 0; `-AudioRecorder` unchecked, a killed recorder's empty WAV still muxed) told to the R pilot,
  its tool. L5 (the sound flag read twice) left.
- **The fix's step review** (fresh, `8ee450f3`): no High or Medium; the three keeps all mean the title is there, the title's
  re-issue and adoption paths unchanged, no new shadowing. Left: its L1, `Resume`'s drop branch (the sound object unreadable or its
  music no longer 1 without a switch) clears a held start without stopping a page event kept for it: a narrow edge, for the next
  tranche (`{pending=false; if(from_page) StopLocked();}`).
- **Build** `build-r12h-fs3` (`8ee450f3`, the final code): 0 warnings, every unit test passed.
- **Merged** `bc11fa5d` (2026-11-12 19:37 Paris; Test-Project -MergeGate passed on the branch); the launcher rebuilt from main (0
  warnings, every test passed, `evidence/I-0132/build-launcher-bc11fa5d.log`); start check `start-bc11fa5df` passed (19:49-19:50, the
  ASUS, restored; a warm cache: no page shown, the title's crackle started and stopped at Continue).
  The disk rotation (`Clear-DiskCache.ps1 -Apply`, 19:52): 2,633 files, 2.3 GB, eight build folders older than 48 h, 365.7 GB free.
- **Thomas's ear** (22:16, relayed by the coordinator): « Top le son du menu shaders, ça fonctionne »; the coordinator's note: the
  fire's level under the music validated, nothing to change (which film or build he heard not said).
- **Open**: the page ending before the title (M1's
  stop and the title's fresh start) not yet seen in a run; the event under a page without the
  music not recorded; the sparks' and the haze's textures (fourth tranche) still unused.

### R12h's sixth tranche (2026-11-12): the page in the game's languages

Thomas (20:12, « il peut faire les autres langues aussi s'il les maîtrise […] je préfère l'anglais qu'une langue hallucinée »; 20:24,
« shaders » kept rather than a translated word): the page's two lines in every language a Steam PC build runs in. Branch
`claude/stoic-edison-56c05d` from main `9119c900`.

- **What the page had** (FONTS-LANGUAGES-NATIVE §1-§2): French or English by the ordinal `[14479BA10]`, in the three families the
  overlay read (TsukuOldMinPro, TsukuMinPro, MatisseProN). The game loads each language's own two families (MenuFont_01 and _02); in
  Polish (Polish), Korean (Hangul) and both Chinese (SimplifiedChinese; TraditionalChinese with TsukuMinProSub) none of the three is
  loaded, so the page drew **no words at all** there (INFERRED from the table, no run in those languages).
- **What** (16b): five more font slots (TsukuMinProSub, Polish, Hangul, SimplifiedChinese, TraditionalChinese; folders as the Data1
  index names them), each read only when the game inserts it, so only in its language's runs. `kPageWords`: the title and the note in
  the thirteen selectable ordinals (English, French unchanged; Italian, German, Spanish for `spaES` and `spaAR`, Brazilian Portuguese,
  Polish, Russian, Japanese, Simplified and Traditional Chinese, Korean), each with its two families; « shader » kept where the
  languages' games keep it (a transcription in Russian, Japanese, Korean; 着色器/著色器 in Chinese, the standard term there); the percent
  spaced in French, German and Russian. The language is decided once, when both its families are read (the game loads them for the
  language it chose) and every code of its words is in their chains; otherwise English, also after 10 s with its families unread;
  no words before the decision (no flipping); one row `native_precompile_language`. The title's chain starts with the titles' family
  only when it alone covers the title.
- **The step review** (fresh, `9119c900..85c1aa8e`): no High or Medium; every translation judged correct and natural by the reviewer's reading (no
  native speaker, no string of the game's compared; none to fall back); the mapping and the keys checked against the ordinal table and the Data1 index. Lows: L1 a title half in TsukuMinProSub (a
  126 KB subset) would mix typefaces: the titles' family first only when it covers the whole title; L3 the Italian and Spanish notes
  closer to « long » (`c7ab5c44`). Left: L2, a CJK page's first frames parse an 11.4 MB family on the engine thread (as MatisseProN's
  90-120 ms in English), to see in its witness.
- **Builds**: `build-r12h-ml1` (`85c1aa8e`): 0 warnings, every unit test passed; `-ml2` (`c7ab5c44`): 0 warnings, every unit test passed.
- **The integrity review** (`main...5356dda8`): no High or Medium; Lows answered: Thomas's validation of the fifth tranche's sound
  recorded as relayed (which film or build he heard not said), the translations « judged correct by the reviewer's reading ».
- **The final review** (fresh, xhigh, `main...5356dda8`): no High; M1 main not merged in: merged (`aa6a5106`, main `c347b279`, STATE
  resolved on main's rows). Lows: L1 the Chinese strings translate « shader » (着色器/著色器): kept, Thomas's « Bien » (20:24) came after
  this thread's answer naming them; L2 the chains rebuilt every frame (a body's typeface could change after a late family): made once
  at the decision; L3 English as a fallback drawn in index order and unchecked: through the run's own families first, and no words at
  all if English is not covered either; L4 STATE's rows rewritten; L5 this record; L7 static checks of the slots' identities, the table's
  slots and its ordinals; L8 the row names the first missing code (`577def2e`). Left: L6 `TitleChain`/`BodyChain` kept for the overlay
  test screen alone; L8's note width (a window narrower than about 7:6 may clip the longest notes, as French already could) and
  Spanish's « 49% » (games' usage). The optimisation pass (read): the chains and the coverage now computed once; the new families read
  only where the game loads them.
- **Build** `build-r12h-ml3` (`577def2e`, with main `c347b279`, the final code): 0 warnings, every unit test passed.
- **Merged** `2fd56ec1` (2026-11-13 06:53 Paris; Test-Project -MergeGate passed on the branch); the launcher rebuilt (0 warnings, every
  test passed, `evidence/I-0132/build-launcher-2fd56ec1.log`); start check `start-2fd56ec12` passed (06:58-07:00, the ASUS, restored;
  English, a warm cache: no page shown). Thomas in this thread (06:55): « Tu peux lancer toutes les langues que tu veux ».
  The disk rotation not run then: another thread's game and compiler running; run 2026-11-13 14:55: 7,055 files, 5.2 GB, 19 build
  folders older than 48 h, 331.0 GB free.
- **The witnesses** (2026-11-13 13:57-14:51, the ASUS, the launcher's module from main `2efc16e1`, `precompile-screen-hold`,
  `-TestLanguage` one run per language, no save loaded, each closed at the runner's 150 s and restored by hash, no fault or crash;
  `evidence/I-0066/artifacts/r12h-ml-<language>`; Thomas's go in this thread, 06:55): English, French, German, Spanish, Polish, Russian,
  Japanese, Simplified Chinese, Traditional Chinese, Korean: in each, `native_precompile_language` names the run's own language,
  `families_read` and `covered` true, no missing code, decided 0.1 to 0.2 s after the page showed; `native_overlay_font` reads only
  that language's families (Polish 87.5 ms, Hangul 91.0, SimplifiedChinese 72.8, TraditionalChinese 142.6, TsukuMinProSub 1.3 with
  126 glyphs). The stills at 30 s (`evidence/I-0132/artifacts/r12h-ml/page-*.png`, `page-languages-sheet.png`): every line whole, no
  hole or box, centred; the spaced percent in French, German and Russian. Polish, Korean and both Chinese now have words (none
  before, INFERRED). Not run: Italian, Latin American Spanish, Brazilian Portuguese (the same two families as German and Spanish).
- **Open**: none for the tranche; a native reader's look at the translations if Thomas wants one.

## R12h step 2 (2026-11-21, the R pilot): the native passes' programmes compiled at build time

ROADMAP item 2's step 2 (D-0172's precompiled shaders; D-0273's page shown only when a compilation really happens, its list
holding what still compiles at a first frame), from main `b4708decc`, branch `claude/r-i132-r12h2`.

- **What compiled at run time**: 31 `D3DCompile` entry points at 14 sites, each compiled at its pass's first use (a session compiles
  those of the passes it opens: the draw scan behind its test flag, the FaceGen programmes at a face's composition, the overlay's at
  its first frame, R12a's final review L4). Reported by the R10 thread (2026-11-21, its session's scripts, not filed, so not evidence
  here): about 470 ms at the first world frame, the automatic exposure's Adapt 203 ms and the TAA's Resolve 121 ms of it. The sites (`D3DCompile` in `native/`,
  the tests aside): the TAA (3 entries), the automatic exposure (2), the offscreen composition (3), the material composition (2),
  the effect layer (4), the velocity resolve, half size, occlusion and output-size depth (2 each), the cloth Velo vertex programme
  (1), the effect draw scan (1, a test tool), the overlay (3: VS, PS, PSScreen), the FaceGen composition's base and marks (2 each):
  31. NIS is Streamline's plugin, whose programmes and pipeline are its own (not checked, carried below).
- **What is written**: each C++ string moved byte for byte into `native/renderer/shaders/<name>.hlsl` (14 files; the comments that
  described them moved with them; `auto_exposure.hlsl`'s line 80, a comment inside the moved text, changed since by the step review's
  answer, so that file alone is no longer its string to the byte; `taa_shader.h` and `auto_exposure_shader.h` removed), compiled by fxc in `Build-GameBootstrap.ps1`
  (`$movedPrograms`, with the run-time flags: `/Ges` the strictness, the TAA's `/Gis /O3` too, fxc's default level otherwise) into
  `<name>_<entry>.dxbc.h`; the sites take the arrays (`D3D12_SHADER_BYTECODE`), the overlay keeps two of them in place of its blobs.
  `auto_exposure_test` and `menu_tonemap_test` read the built bytes (stage and shader model by reflection; the Tone block's layout as
  before); the « no comment after code » check on the kernels now runs in the build, on `shaders/auto_exposure.hlsl` (the step review's M1, below). The build refuses a module
  that imports `D3DCompile` (imports.txt): the module now imports `D3DReflect` and `D3DDisassemble` alone from d3dcompiler_47 (the
  game's programmes), where main's launcher module also imported `D3DCompile`.
- **The same bytes** ([Compare-R12h2Programs.py](Compare-R12h2Programs.py): each string at `b4708decc` compiled by `D3DCompile`
  through ctypes with the module's flags, against fxc's output of the moved file and the build's headers): **31 of 31 identical**,
  with System32's `d3dcompiler_47.dll` (`evidence/I-0132/r12h2-compare.txt`, against `build-r12h2-4`); on the final tree
  (`8898c03fd`, the line-80 comment included) against `build-r12h2-6`'s headers, again 31 of 31 with System32's DLL and 31 of 31 with
  the Windows SDK 10.0.28000's own `d3dcompiler_47.dll` beside its fxc (`evidence/I-0132/r12h2-compare-tip.txt`). The pipelines' descriptions, and the store's names made from them, are therefore unchanged:
  a list saved before this step still serves them (INFERRED from the identity, not run on an older list).
- **Builds** (`-Jobs 4`, the cache `evidence/I-0132/cache-r12h2`): `build-r12h2-1` stopped on `auto_exposure_test`'s headers before
  `windows.h` (BYTE); `-2` on the new import check matching the DLL's own upper-case row (now case-sensitive, a function's row);
  `-3` on a comment that swallowed that line's code; `build-r12h2-4` (`df67a01e1`, incremental from the cache: 4 of 476 commands run, so its « 0 warnings » covers those alone): 178 lines starting PASS (as main's
  launcher build), `auto-exposure.txt` and `menu-tonemap.txt` clean; `build-r12h2-5` (`737f94014`, the step review's fixes): the same.
- **The step review** (fresh, high, `b4708decc..df67a01e1`): **H1** the TAA's Mirror pipeline never made: the replacement's comment
  swallowed its creation on the same line (DLSS with Thomas's sharpening above 0 would have lost the sharpening at its first frame,
  latched off; it compiles, the bytes are identical, `taa_test` does not record the sharpening, the sharpening is 0 by default):
  the statement on its own line and `Open` now refuses a stage missing one of its three pipelines. **M1** the « no comment after
  code » check dropped on a reason that does not hold (an HLSL line comment swallows code as the raw string's did, R11h's two H1s):
  kept, now by the build on `shaders/auto_exposure.hlsl` before fxc. L1 `CompileClothVelocity` and model_draw.h's « does not
  compile »: `BuiltClothVelocity`, the comment. L2 « 31 at their first use » counted the entry points, not the calls (the overlay
  compiles two of its three at each of its three instances' Open): named entry points. What held, by the reviewer: no
  `D3DCompile` left outside the tests; each .hlsl the old string exactly; every row's entry, profile and flags those of its call; the
  build graph's `/I"$out"` units and embedding list; the headers at global scope, the bytecode pointing at static arrays.
- **The fix's step review** (fresh, high, `df67a01e1..737f94014`): no High or Medium; no statement hidden in a comment anywhere in
  the step's diff (C++, HLSL, PowerShell, every added line read); the check runs before the commands on the serial, Ninja and cache
  paths and passes the file as it stands. Lows answered: L1 this section's two stale sentences (the dropped check, « calls ») and
  `build-r12h2-5`; L2 two stale comments (auto_exposure.hlsl's « this string », the test row's « compile »); L3 model_draw.h's
  sentence; L4 (nit) `RecordSharpen`'s `!mirror_` test, now unreachable: kept as a guard.
- **The final tree's build** (`build-r12h2-6`, `8898c03fd`, main `8ba5ded10` merged in, complete, no cache): 178 lines starting
  PASS, no C++ warning; fxc's warnings, printed by a complete build only: `post_tone.hlsl`'s nine (main's) and three in the moved
  files (`taa.hlsl` X3577 on `isnan` under `/Gis`, `offscreen_composition.hlsl` X3571 on `pow`, `overlay.hlsl` X4000 in `PS`): the
  same programmes, whose warnings `D3DCompile` returned at run time and nothing read.
- **The runs** (2026-11-21 10:54-10:57, the ASUS, offline, the save restored by hash, `build-r12h2-6`; `Invoke-StartCheck`, both passed,
  no fault): `start-8898c03fd` on the build folder's empty store (R12d's warm-up alone, 16 pipelines; 344 world frames): the first
  world frame 348.0 ms, its recording 310.4 ms with 22 pipelines made; `start-8898c03`, the next start on the list it saved (191
  pipelines precompiled in 194 ms, the page not shown; 340 world frames): the first world frame 83.7 ms, its recording 57.1 ms, no
  pipeline made. Main's two last start checks before this step, on the launcher's list (191 and 200 pipelines, none made at that
  frame): `start-d43a542b8` (the step's base's native tree) 447.2 and 419.6 ms; `start-6f60aed41` (an older tree, before I-0140
  tranche 29's witness code) 1,407.7 and 1,379.9 ms. Against the base's tree the first world frame's recording is about 360 ms
  shorter (419.6 to 57.1 ms, one run each); main's own spread between those two starts (960 ms) is larger than that and not read,
  so `start-6f60aed41` is not counted in this step's gain.
- **The page's list** (D-0273), partly met: no HLSL compiles at a first frame any more (the module imports no `D3DCompile`). What is
  left is the driver's pipeline creation: on a saved list the store's precompile (the native passes' pipelines among them) and
  R12d's warm-up (the rays' passes) run under the page, shown only when that work really lasts (`start-8898c03`: no pipeline made
  at the first world frame); on an empty store the native passes' pipelines are made at their first use, outside the page
  (`start-8898c03fd`: 22 pipelines made at the first world frame, which ones not logged: the native passes' among them is INFERRED
  from the empty store, the game's model pipelines may be too). The TAA's, the sharpening's and the overlay's programmes (ROADMAP item 2's « to
  check ») are among the 31; NIS's are Streamline's.
- **Open, carried to ROADMAP workstream 7** (its item 6): on an empty store (a first launch, a deleted store) the native passes'
  pipelines, now from fixed bytes, are still made at their first use, outside the page (not in R12d's warm-up; the final review's
  M1 proposes making them there through `Pipelines()`, the same descriptions); NIS's pipeline at the first sharpened frame
  (Streamline's, outside our store and the page, not timed); a cold driver cache with SER (about 24 s, the ROADMAP's figure) never
  run: it needs Thomas's NVIDIA cache emptied.
- **Reviews at the merge**: the final review (fresh, xhigh, `main...8898c03fd`): no High. **M1** step 2's list called done while on
  an empty store the native passes' pipelines are still made outside the page: the records say « partly met » (ROADMAP item 2, this
  section), the warm-up carried. Lows: L1 `auto_exposure.hlsl` no longer its string to the byte and the identity run before that
  edit: rerun on the final tree (above), recorded; L2 NIS's pipeline unchecked: carried; L3 the R10 thread's timing repeated as fact
  and « 31 at the first world frame »: marked as reported, 31 entry points at 14 sites (STATE too; the build script's comment already
  names its source, left); L4 the CONTRACT excluding R12: its amendment, the titles aligned (RESULT, README); L5 R12-PLAN's R12h row,
  ROADMAP item 2's line for Thomas (the embers now in STATE's list), `status.json`'s time; L7 the comparison tool's unused class and
  unreleased blobs, fixed, and its SDK DLL named. Kept: L6 (five full-screen vertex programmes compile to the same bytes, two others
  too: one shared file would keep the names; left for workstream 15's optimisation review); `status.json`'s `iterations_used` (3) not
  kept since R11's first tranches, left as it is. The integrity review (`main...c3d473f48`): no High or Medium; nothing weakened (the
  tests' checks kept or moved into the build, the 178 PASS lines those of main's launcher build), no scene case, no cut (the four
  starts' settings rows alike). Its Lows answered here: L1 the gain against the base's tree alone (about 360 ms, one run each), L2
  the 22 pipelines not named by the journal, L3 `build-r12h2-4`'s « 0 warnings » an incremental build's.
- **Merged** `bf252061b` (2026-11-21 11:09 Paris; Test-Project passed in main); the launcher rebuilt from main (178 PASS, no C++
  warning, `evidence/I-0132/build-launcher-bf252061b.log`); start check `start-bf252061b` passed (11:19-11:20, the ASUS, 359 world
  frames, restored; 203 pipelines precompiled, the page not shown, the first world frame's recording 59.6 ms, no pipeline made); the
  disk rotation (D-0260): nothing older than 48 h to delete.

## R12e (2026-11-11, the R pilot): the « Rekindled » title screen

- **What** (R12-PLAN « R12e's design », D-0182's mock-up): « REKINDLED » letter-spaced in ember colour in TsukuOldMinPro under DS3's
  logo, between two rules fading outward; a fade-in, then one glow crossing it; embers rising and a discreet smoke behind the logo (the
  second tranche, a screen layer over DS3's UI in the startup frames). The UI movies' advance hook stamps the title's logo
  (`05_001_Title_Logo`) and the movies that cover it (`05_010_*`, `01_010_*`, `02_*`); a cover fades the word out in 150 ms and back
  in 300 ms, the intro once per visit. Only the titles' family is read at the title (1.0 to 1.6 ms; MatisseProN's or TsukuMinPro's
  91 to 120 ms never spent there, the step review's M1: in `r12e-film3` and `-film4` those parses fell in the precompile screen).
- **The first stills** (`r12e-title1` in English at 1280x720, `r12e-title1fr` in French in a 1920x1061 window, 20:12-20:17, `build-r12e1` =
  `e4e008fa`, no fault, restored): the word under the logo in both (`evidence/I-0132/artifacts/r12e-title-first-en.png`,
  `r12e-title-first-fr.png`), hidden under the Load Game list (`list.png`); `native_rekindled_title` rows at each cover.
- **The step review** (fresh, `main...e4e008fa`): no High. M1 every family parsed at the title (a 100 ms freeze in the logo's fade):
  the titles' family alone. M2 the intro replayed after every cover, a cover cutting the word at once: the intro once per visit, covers
  fade. Lows answered: L1 a tick stored after `now` (fresh), L2 the glow from outside the word to outside it, L3 a smooth clock (QPC),
  L4 the word only in the titles' font, L6 the counters' rows only while a screen can show, L7 a dead name in a comment. L5 kept: the
  halo is the glyph atlas's 3-pixel pad at any size. L8 (layout at 16:10 or 4:3, the title's own Options movie, DS3's fade-outs)
  left for the film (`3c6f3c94`).
- **The film** (`r12e-film2`, 20:46-20:48, `build-r12e3` = `c33d6686`, no fault, restored): the glow crosses the word once, left to
  right, a band of about one letter (`evidence/I-0132/artifacts/r12e-title-glow.mp4`, its strip `r12e-film2/glow-strip.png`); the
  intro plays at the title's first frames, about 14.7 s before the offline notices (`ui_movie_first_advance` ticks): `r12e-film1`
  began after it and `r12e-film2` 2 s into it (the fade-in missed), so -VideoTitle now starts at FromSoftware's logo (`f7c91774`).
- **The second tranche** (embers and smoke, `42853fed`): an underlay before DS3's UI showed nothing (`r12e-film3`, 21:21-21:23,
  `build-r12e5`: a region beside the logo at Y 16, black, at every sample: the title movie read as opaque, INFERRED); they are drawn
  in a screen layer after the UI and before the overlay (light on the black, nothing on the white logo). `r12e-film4` (21:43-21:45,
  `build-r12e6` = `b18bacbe`, no fault, restored): the embers rising and the smoke behind the logo
  (`evidence/I-0132/artifacts/r12e-title-embers.mp4`, `r12e-title-embers.png`); the same region's brightest pixel at 105 to 174 while
  they pass; the counters row `screen_quads` about 143 a frame (72 embers' halo and core, 7 clouds, the faded ones left out).
- **The second step review** (fresh, `3c6f3c94..87c23973`): no High; the blend, the latch, the lifetimes and the PSO keys hold. M1 a
  cloud wrapped while still on the output (a pop at the top): it wraps once wholly off. M2 the field laid out for 16:9: its half width
  the output's (at least 640 units). Lows: L3 -VideoTitle's recorder stopped with -Continue too, its bound 120 s + 8 s a step; L4 the
  screen layer's frames, refusals and skips in the counters row; L5 a failure names its pass; L6 both passes opened at the overlay's
  first frame (the screen layer's compile not at the title); L7 half a code value of dither in the screen layer (the smoke's few codes
  over black); L8 the documents.
- **Firelink's loading walls** (the E pilot's ask once I-0113 tranche 38's `firelink_wall` was in main, merged into the branch at
  `9b3a9e27`): an Advanced page row « Firelink loading walls » / « Murs de chargement de Lige-Feu », On (the original's) or Off, before
  the vignette (kept last), live through the settings (E's reader at the walls' creation: from the next load); `graphics_rows_test`
  passed in `build-r12e8b` (`5c5cb370`; `build-r12e7` refused its first form, a shadowed name, and `build-r12e8` stopped at the
  timing-sensitive `collect_profile_test` under load before reaching it, 7.1 ms for 4). `no-title-embers.flag` leaves the embers out
  (a test switch, off by default; no run used it).
- **The final review** (fresh, xhigh, `main...5c5cb370`): no defect in the code; H1 the final tree never tested nor run (its tests
  passed since in `build-r12e8b`; the runs below). M1 the word came back 0.4 s between the Load Game list's confirm and the loading
  (`r12e-film4`): a cover now holds it off 800 ms after its last advance. M2 the covers a hand-made list: every title page but the
  title's own two (`05_0*`), the message boxes, `02_*` and character creation (`04_*`). M3 the exit's matrix: the runs below. Lows: L1
  the logo's grace 500 ms; L2 the intro once the titles' font is read; L3 the screen layer's own latch (the overlay and the precompile
  screen go on without the embers); L4 no clock read in play; L6 dead code and a stale comment; L7 the numbers above. Kept: L5 (two
  passes, two heaps, an extra heap switch in the startup frames only), L8 checked on the page (below). The integrity review: nothing
  weakened, no scene case, no cut; its findings are the same evidence gaps (the final tree not run; the numbers corrected above).
- **The final tree's runs** (22:53-23:02, `build-r12e9` = `27bfef86`, restored): `r12e-final-fr` (French, 1920x1080, filmed):
  no fault, the word, its glow, the embers and the smoke (`evidence/I-0132/artifacts/r12e-title-final-fr.png`), the screen layer's
  569 frames in 600 with no refusal; `r12e-final-1610` (a 1680x1050 window asked): no fault, but the game made it 1680x945 (16:9), so
  16:10 is still unseen (open); `r12e-final-row` (the Advanced page in play): « Firelink loading walls » on the page, On, above
  « Vignette », all fourteen rows in the window (`options-step-5.png` to `-7`); the run then faulted at 23:02:27 in `FUN_140E602F0`
  (`[manager+0x980]` null), two seconds after its 240 s deadline with the Options open: R12g's RCA's close fault (the bounded run's
  stop withdraws the HAL with a menu open), not R12e's; its steps outlasted the deadline before the row was switched.
- **The last step review** (fresh, `5c5cb370..27bfef86`): no High. M1 the 800 ms hold sized from one gap (0.69 s): 1.5 s now (a
  return from the Load Game list shows the word 1.5 s later). Lows: L2 one grace for the step and the title (500 ms); L3 comments; L4 a
  failure row whenever its text changes; L6 the message boxes' family (`01_01*`, the small one too). Kept: L1 (one tick count and two
  loads a frame in play, no QPC), L5 (the screen layer compiled after a failed overlay open: never drawn), L7 (500 ms of tail when the
  logo stops with no cover).
- **The row switched** (`build-r12e11` = `003456f1`, the merged tree with main's tranche 39; `r12e-final-row2` 23:36-23:41 and
  `r12e-final-row3` 23:48-23:53, no fault, closed by the steps before the deadline, restored): the greyed rows skip the cursor (the
  first run's twelve presses went past the row and switched Texture quality, put back), nine reach it: « Firelink loading walls » Off
  then On, each saved (`native_rekindled_graphics` `rt_rows` 512, `firelink_wall` false then true).
- **The row withdrawn** (Thomas, 2026-11-11 23:53: « Enlève la ligne concernant le mur de Firelink, off forcé »): the three files
  back to main's (no row; the walls' setting stays the settings file's, off by default and forced by the E pilot's tranche 40, which
  owns the walls' hook). The runs above stand as the row's test before its withdrawal.
- **Merged** `40a92818` (2026-11-12 00:10 Paris; Test-Project passed in main); the launcher rebuilt from main (every test passed); start
  check `start-40a928182` passed (00:25-00:26, the ASUS, 346 world frames, restored).
- **The embers' second look** (Thomas, 2026-11-11 23:24 and 2026-11-12 00:08, his animated mock-up 00:07 for the sparks' density,
  the mock-up's orange-edged smoke kept, the video's grey one « laide »): 220 embers (streaks, one in twelve large and blurred), a
  wispy cloud and its rim image, banks at the corners and the sides lit from below, a warm glow along the bottom. `r12e-film5`
  (01:39-01:41, `build-r12e13` = `f114f82c`): sixteen banks over the whole width made an orange wall with fibrous streaks (a 32-cycle
  octave); `r12e-film6` (02:02-02:04, `build-r12e14` = `96aa1e61`): ten banks at the corners and the sides, half the opacity, no fine
  octave (`evidence/I-0132/artifacts/r12e-title-embers2.mp4`, `.png`), sent to Thomas; no fault in either, restored.
- **The night's look** (Thomas, 2026-11-12 02:13 to 02:46, each answered in the thread): the embers' motion turbulent (`r12e-film7`,
  02:32-02:34, `build-r12e15` = `fb79026e`: « vraiment bien là le mouvement », 02:36; the embers then died low, their life 8 to 16 s
  since), then a wind to the right with gusts and trails, embers drawn as embers (a red halo, an irregular flake, a white-yellow hot
  core), about one in four (the small ones) in front of the title (the overlay, alpha blended), the logo tinted by a multiply layer (a third pass), the
  mock-up's ring and sword behind the title, the word's glow every 10 s, the title kept after the button (message boxes no longer
  hide it) and under its pages (dimmed to 0.3; the loading screens and character creation hide it). The Fable 5.2 review (read-only,
  during the night's measure, Thomas 03:40): H1 a comment had swallowed the embers' age and heat (the field would have emptied after
  16 s), restored; M1 a page's dim held 1.5 s; M2 a message box's own state (the word faint, the tint off, the front embers moved behind);
  M3 the smoke recycled at the top (but placed straddling the bottom edge: the final review's M1 below); L3 the ring's seam; L1 the tint's failures named. Not answered: L4 (the tint and the ring
  placed by 16:9 constants, as the word), L5 (the passes' shaders compiled once each, the tint frame rebuilt every title frame).
  `r12e-film8` (05:55-05:57, `build-r12e16` = `4e1d8800`, no fault, restored; `evidence/I-0132/artifacts/r12e-title-night.mp4`,
  `.png`): the tinted logo, the ring and sword, the embers in the wind; under the Load Game list the title dimmed (`list.png`).
- **The burnt ring** (the coordinator, Thomas asleep, 2026-11-12 05:58: r12e-film8's ring too clean and even; `27ab914a`): an uneven
  stroke, charred gaps, ember spots, cracks and sparks in a 384-texel image. `r12e-film9` (06:43-06:45, `build-r12e17` = `27ab914a`,
  no fault, restored; `evidence/I-0132/artifacts/r12e-title-ring.mp4`, `.png`; `notice-2.png` the offline box readable, `list.png`
  the title behind the Load Game list); Thomas's taste on it not yet given.
- **The final review** (fresh, xhigh, `main...4cbfda6a`, main merged in with I-0113 tranche 40) and the integrity review: no High. M1
  a recycled smoke bank straddled the bottom edge (a pop): placed wholly below the output now. M2 our word is drawn over DS3's UI, so
  « kept under its pages » put « REKINDLED » across the Load Game rows and the offline box's OK (`r12e-film9` `list.png`,
  `notice-2.png`): my choice, Thomas asleep (his to change): under a page or a box the word fades out and the logo's tint goes off
  (DS3's own logo stays, dimmed by the page), the embers and the ring stay at 0.3 under a page and 0.6 under a box, all eased over
  0.2 s. Lows answered: L1 a tint record failure named « tint », L6 the cracks' seam at the ring's left point, L9 comments. Kept
  (recorded): L2 the tint's counts not in the row, L4 the images made at the title's first frame (about 20 ms, INFERRED), L5 the
  ring image at 384 texels whatever the output, L7 (moot: the tint off under a page), L8 the three passes' duplicated compiles and
  uploads. The integrity review's Medium (`r12e-film9` unrecorded) and Lows (the dim and box states' wording, « one in four ») answered
  in this section.
- **The answers' step reviews** (`4cbfda6a..02d81843`, then `02d81843..4df88a75`): H1 the cracks' declarations swallowed by the L6
  comment (MSVC refused `build-r12e19`; clangd's check had said nothing), restored; M1 a message box's own eased factor (no jump when
  it closes); M2 the tint fades with a loading screen's cover; L1 the word's curve outside pages as before; L3 no state carried into a
  new visit. `build-r12e20` = `4df88a75`: every test passed. Kept (recorded): a page opening while the title is still fading in after
  a cover lifts the word and the tint for a frame (rare), and the front embers change layer in one frame when a page or a box opens.
- **The final tree's film** (`r12e-film10`, 07:57-07:59, `build-r12e20` = `4df88a75`, no fault, restored): under the offline box
  the word gone, the box readable, the ring and the embers behind it (`notice-2.png`); the Load Game list clean, the embers faint
  around it (`list.png`); side by side in `r12e-film10/check.png`.
- **The morning's look** (Thomas, 2026-11-12 08:01 to 11:32, each answered in the thread; films `r12e-film11` to `-film15`, `build-r12e21`
  to `-r12e30`, no fault, restored): a crimson burn asked (08:01), then the Fable 5.2 comparison with the mock-up (Thomas's idea, 08:02;
  run twice, the second on the crimson film): the logo's burn as fine brown-black stains and cracks (a multiply image), the embers in
  three classes (320 in all, for Thomas's target of 120 to 170 seen, 08:58: about 60 of 200 by eye in `r12e-film13`; the count at 320
  not measured), the smoke brown and fainter (« trop de fumée », 08:55), spread along its
  path at the start and faded in over 4 s (11:30), the glow behind the titles tried then removed (« la coloration des lettres suffit »,
  11:32; « trop vif » 09:45, « sous SOU » 09:48); the logo's tint kept under windows (08:01), REKINDLED at 35 % under a message box
  (option 1, 11:30), our glow, embers and the logo's burn out behind the box (« plus très bien lisible », 09:43). `r12e-film15`
  (12:08-12:10, `build-r12e30` = `7ee5f63a`): `evidence/I-0132/artifacts/r12e-title-final2.mp4` and `-pair.png` (the title; the offline
  box readable, its OK seen through the word).
- **Their step reviews**: H1 the cracks' declarations swallowed by a comment (restored; a comment swallowed code four more times this
  morning, three caught by MSVC or a grep of the diff before building: `.project/LIVE-RUN-NOTES.md` now asks for that grep); M1 the logo's tint eased with a
  cover of its own (no dip when a page closes), M2 the ring fainter behind a box, M3 the burn off under a page (`2026-11-12` 12:11).
  Kept: the dead `first` placement of the smoke, the box's area a fixed rectangle (|x| < 360, y 240 to 460: a taller box shows embers
  at its edges), the big embers drifting off the right edge early.
- **The crackle** (Thomas, 08:54: the bonfires' crackling sound at the title): `r12e-film13`'s about 24 s at Firelink (ticks 92071828 to 92094984) with `audio-census.flag`
  heard Firelink's ambience (`y400000000`), its music (`m400100000`), `z006000015`, `s444444444` and the character's sounds, no fire
  loop identified; named by the shader page's census at two bonfires and played at the title since (« The remainder », below).
- **The final tree and its last reviews**: `build-r12e31` (`f44e9fce`, main's R12h page and E's tranche 41 merged in, the native tree
  of the merge) built and passed every unit test (12:27); it was not filmed: the page's and the boxes' behaviour rest on `r12e-film15`
  (`7ee5f63a`), the reviews and the post-merge start check. The final review (fresh, xhigh, `main...29e20d10`): no High; its Medium
  (this evidence gap) answered here; Lows kept for the next R thread: stale comments in 16b, scene_pass.h/.cpp and 15 (the box state,
  the image counts, « one in four », the streak, the octaves), `PlaceCloud`'s dead `first` branch, the tint's counts not in the row,
  the box's area fixed from the English offline notice (D-0161: a taller or translated box shows embers at its edges), the third
  pass's duplicated compiles and uploads (no `reserve`, the images built at the title's first frame), the tint clearing at about 35 %
  when a loading screen follows a page, an unlocked clear of the failure string in `Close` (older), the page not clearing the tint.
  The integrity review: no High or Medium; its Lows answered above (24 s at Firelink, the ember count an estimate, the comment rule).
- **The blur** (Thomas, 12:25: REKINDLED blurred under a box rather than darkened, back to sharp after): the sharp glyphs out and the
  blurred copy (the shadow atlas, slightly magnified) in, eased with the box; its step review's Medium (the rules no longer dimmed) and
  Lows answered (`d15d51c6`). `r12e-film16` (12:59-13:01, `build-r12e33` = `d15d51c6`, every test passed, no fault, restored):
  `evidence/I-0132/artifacts/r12e-title-blur.mp4`, `r12e-title-blur-box.png` (the OK readable, the word a blur behind it).
- **The remainder** (2026-11-12 afternoon, the R pilot's fresh thread, branch `claude/reverent-yalow-def0de`):
  - **The final review's Lows** (`a0dfa729`): the stale comments (the six images, three octaves, 9 % large, about one small or medium
    ember in eight in front, the flakes and their trail, the box's and the page's shares, the cover's hold); `PlaceCloud`'s dead
    `first` branch removed (every bank placed below the output; its random draws now y before x: the banks' positions differ from the
    earlier films' frame for frame, the step review's L2); the tint layer's counts in the `native_overlay` row (`tint_quads` 3 a frame
    in `r12e-r1-fr1080`); the precompile page clears the tint layer; under a loading screen that follows a page the layers are kept until
    the tint's own ease ends (it cleared at about 35 %); `Close` clears the overlay's failure under its lock. Kept: the box's area a fixed
    rectangle (D-0161: a taller or translated box shows embers at its edges), the three passes' duplicated compiles and uploads, the
    images made at the title's first frame.
  - **The crackle** (`eb1a4463`, `bd973b2a`, `af319bb1`; 16b `title_crackle`): the game's own event **s444444441** (in the resident
    `fdp_main.fev`), the bonfire effect's sound request (the effects' `FUN_140D53940` under the effect node's update, the same caller
    chain at the High Wall's and Firelink's bonfires: `r12h-fire-census-m30`, `-m40`, the shader page's thread; at Firelink FMOD never
    started it, `s444444444` started there instead: the integrity review's Low), played through the
    menus' sound path (`FUN_1404AD510(handle, 5, id, -1)`) after the title music's switch leaves the menu sound object's music at 1, and
    stopped (`FUN_141A29EB0`) after a switch to another music or the music's stop (`FUN_14076B9F0`). The step review (fresh, `main...bd973b2a`):
    M1 `FUN_14076B920` only jumps to `FUN_14076BBA0(object, id)`, also the object's table entry: the observer is on `FUN_14076BBA0`
    itself; L1 a comment, L3 « audible » INFERRED; no High. That s444444441 is the crackle Thomas hears is INFERRED (its caller and its
    zones), and that it sounds at the title, played with no position, unknown: his ear.
  - **With R12h's music** (main merged in at `8ace60fb`, its third tranche: the title music held under the precompile page and let go
    on the engine thread, and its own detour of the music's stop `FUN_14076B9F0`): one detour of the stop, R12h's `DropTitleBgm`, stops
    the crackle through a pointer (`e550146c`; `build-r12e-r5` refused by MSVC: a forward declaration in another namespace than 16b's,
    which clangd did not report; `16dd0580`, `build-r12e-r6` every test passed). The final review (fresh, xhigh, `main...16dd0580`): no
    High; M1 without Thomas's music file (any other PC) the title dialog built under the page would start the game's crackle under the
    page's own: held until the page is over (`Resume` from Step); M2 this record; L1, L2 the crackle installed only with R12h's stop
    detour and its pointer set by the setup (a crackle that could never stop); L3 a failed issue not retried until the music stops or
    changes; L5 comments (`13c0f581`); L7 noted: the cover stamped from the film's first render call, a few frames before its first
    picture. Kept: L4 the box's fixed area (D-0161, above), L6 the documents (this checkpoint). The integrity review (`main...16dd0580`):
    no High; M1 the runs below predate the merge (their `hooks` 2 is the removed stop hook): the merged crackle run in `r12e-r1-newgame`
    (below); L1 Firelink's census (above). The answers' step review (fresh, `16dd0580..13c0f581`): no High or Medium; L-a a failed Step
    under the page would have held the crackle for the session (the catches end the page for it), L-b Resume's start dropped if a Stop
    came since, L-c the game's calls under the lock read for re-entry (none in the mirror; `FUN_1407CE930`'s callees not read), a
    comment (`2fd93825`, `build-r12e-r8`); kept L-d (a crackle started before the page shows keeps playing under it: the page shows
    after 3 s, the title dialog later in `r12e-r1-page` and `r12h-mu-page3`). The optimisation pass (read): one lock and two loads a Step for `Resume`, the
    film's stamp one store a render call; nothing repeated.
  - **The opening film under the title** (E's `i66-native-i137-t4-charmake2`: after New Game `native_rekindled_title` stayed shown about
    200 s while the Bink film played, `native_movie_open` to `native_movie_close`, 4,748 frames; no UI movie names the film and the title's
    logo movie goes on advancing under it): every frame the native Bink player renders now stamps a cover (`b7e716df`); no film plays at
    the title itself in the r12e runs (no `native_movie_open` in `r12e-final-fr` or `r12e-film15`).
  - **The runs** (15:07-15:15, the ASUS, `build-r12e-r3` = `af319bb1`, Epsilon's save by Continue, `audio-census.flag`, no fault, each
    restored; `evidence/I-0066/artifacts/r12e-r1-fr1080`, `r12e-r1-1610`):
    - `r12e-r1-fr1080` (French, exclusive full screen 1920x1080, `-VideoTitle`): the title right in French (`title.mp4`); Continue, then
      Quit Game from the System menu: the logos, then the title again with its intro and « APPUYEZ SUR UNE TOUCHE » (`options-step-7.png`):
      **the return to the title from play seen**. The crackle: `native_title_crackle` started (handle valid), `audio_event_name`
      s444444441 started by FMOD with result 0 beside m000000001, stopped at Continue, started again at the return (`starts` 2).
    - `r12e-r1-1610` (English, exclusive full screen 1680x1050, the ASUS's 16:10 mode): **16:10 seen**: the word, the ring and the logo's
      tint laid out by the height, the embers over the wider field (`title.png`); the Game Options opened from the title's System
      (`title-step-5.png`): **a page other than Load Game seen**: the word faded out, the embers and the ring dimmed. Esc, Backspace and
      Q to close it: inconclusive (the Claude app took the foreground for two of the three keys; the page stayed open, `title-step-8.png`).
  - **New Game** (`i66-native-r12e-r1-newgame`, 16:57-17:07, the ASUS, `build-r12e-r6` = `16dd0580`, Alpha's all-spells copy, a profile
    deleted in the installed copy only, `Run-PilotPair -Pilot CharMake -DeleteProfile`, `audio-census.flag`, no fault, restored by
    hash): `native_title_crackle` `hooks` 1, started at the title (FMOD result 0), **stopped at New Game before the film** (through
    `DropTitleBgm`), `native_movie_open` then at once `native_rekindled_title` shown false covered true: **the opening film covers the
    title**; `native_movie_close` after 4,749 frames, then `04_000_ChrMake_Top`. E's pilot timed out « waiting for the movie screen »
    (it waits for a UI movie name the Bink film never has) after the creation had come: the pilot's, told to E.
  - **Under the page** (`r12e-r1-page`, 17:27-17:31, the ASUS, `build-r12e-r7` = `13c0f581`, `precompile-screen-hold.flag` and
    `no-precompile-music.flag`, the latter added to Run-StartupTrace's effect flags for it (an allow-list entry, no flag's meaning
    changed), Epsilon's save, no fault, restored): the page 80 s (4,831 frames) with its own crackle and no music file; the title's
    m000000001 started under it (R12h holds the music only with the file); **the game's crackle held and started right after the
    page's end** (`native_precompile_screen` done, then `native_title_crackle` started, FMOD result 0). Continue did not load within the
    run (the page's 80 s).
- **Merged** `49a53fe0` (2026-11-12 17:37 Paris; Test-Project -MergeGate passed on the branch, Test-Project in main); the launcher
  rebuilt from main (every test passed, `evidence/I-0132/build-launcher-49a53fe0.log`); start check `start-49a53fe07` passed
  (17:54-17:55, the ASUS, 357 world frames, restored); the disk rotation refused (another thread's compiler running), for the next
  checkpoint.
- **Thomas's ear** (2026-11-12 18:33, « Tout fonctionne », on the new R thread's film `r12e-title-sound1`, `title-sound.mp4`, the title
  with its sound): the crackle heard at the title, validated. The title page's close key: the original does not close it either (Esc,
  Backspace and Q at the title's Game Options page, `r12f-c2-ref` and `r12f-c2-nat`, workstream 2 step 3 below): DS3's own.
- **Open**: the title music under the page without Thomas's file (R12h's choice); the word's French label never longer than its caption; the smoke's dither not pictured; Thomas's taste for
  the embers' size, number and brightness.

## R12d (2026-11-11, the R pilot): the native passes' warm-up and the precompile screen

- **The ask** (R12-PLAN « R12d »; Thomas, 16:00: « Au pire pour le path tracing tu peux demander un retour à l'écran titre », « Si tu
  n'arrives pas à corriger »): the shader precompile screen with its progress bar, our native programmes included; the 10 to 12 s freeze
  at the live switch to path tracing (R12g's runs) its first target. Design: R12-PLAN « R12d's design ».
- **The sweep** (an Explore worker, Sonnet): every native pipeline site; the rays' own (`RtReflections::Make`'s thirteen kinds,
  `RtLocalLights`' two, SER's state object) device-only and lazy, the rest small or the game's programmes (in the store's list after
  one session).
- **What is written**: `RtReflections::Describe` and `RtLocalLights::Describe` (the passes' descriptions, unchanged, in one place);
  `Warm` (each made through the store from them, no member written; SER's state object made by `CreateSerState` and kept under its lock
  for `MakeSer`, which waits for it while it is built and takes it on the same root); `StartNativeWarm` (a below-normal thread after the
  store's precompile, with the rays usable, not under WARP, SER only with `rt_ser`; `no-native-warm.flag` the A/B) and `StopNativeWarm`
  (joined and the warmed SER released before the store and the device close; the thread object never destroyed at unload); the screen
  (`BuildPrecompileScreen`, 16b: « Compiling shaders NN% » in DS3's fonts and a bar at the bottom right while the store's precompile or the
  warm-up runs, after 30 frames, hidden for good when done; `precompile-screen-hold.flag` the stills' test tool); the font observer by
  default (a refused one leaves the screens without DS3's fonts, the bar still drawn, since the second step review; `no-rekindled-overlay.flag`
  the A/B), the fonts read only while a screen needs them.
- **The runs** (the ASUS, no measure, the D3D12 debug layer on as in R12g's runs it is compared with, Run-AlphaCast, an empty store in
  each new build folder, restored by hash; the tools: `no-native-warm`, `precompile-screen-hold` and `no-rekindled-overlay` added to
  Run-StartupTrace's accepted flags):
  - `r12d-pt1` (17:34, `build-r12d1`, before SER's warm-up): the warm-up made 15 of 15 in 15.8 s (a cold driver cache), the screen shown
    14.5 s (745 frames); the switch to path tracing still froze 8.3 s: `native_rt_ser` make_ms 8,300 (SER's state object, outside the
    store). `r12d-screen1`: the 1800-frame hold ended before the title (no still).
  - the step review (fresh, `5b7cf612..9c1f506e`): the descriptions identical token for token, the roots the store's same objects, no
    deadlock; H1 a joinable `std::thread` global destroyed at the module's unload in a play session (never Close) would end the process at
    every quit: never destroyed now (`*new`); M1 the warmed SER held past the device: released by `DropWarmed`, and made only with SER on;
    M2 no warm-up under WARP; L1 the observer's rows the test's only, quiet once the screen is over; L3 a refused observer no longer
    stops the start. Left, recorded: L2 the engine font request now runs by default (English sessions whose screen lasts past frame 120,
    as R12a's test runs did); L4 MakeSer may wait on the below-normal warm thread (bounded by the compile); L6 the cache's update and
    resolve and the probes' pipelines not warmed (they run at the RT level before any PT switch).
  - `r12d-pt2` (18:11, `build-r12d2`): the warm-up made 16 of 16 in 2.1 s (the driver cache warm by then), the screen not shown (done at
    frame 30); the switch to path tracing: `make_ms` 1.3 (INFERRED: the warmed object taken; no row names the hand-off) and **no frame
    over 0.3 s** at the switch (`native_slow_frame`; against R12g's 12,282 and 9,734 ms, both with an empty store; no `no-native-warm`
    A/B run). « A cold driver cache » and « warm by then » are INFERRED from the timings. With SER warmed and a cold driver cache (not
    run), the warm-up would last about 24 s, and a switch to path tracing inside it waits for SER's build (the step review's L4).
    `r12d-screen2`: the hold to the title (4800 frames, 83 s): « Compiling shaders 50 % » and its bar over the title's copyright line
    (`evidence/I-0132/artifacts/r12d-screen-title.png`): moved to the bottom right (`9b30a0c7`).
  - the second step review (fresh, `9c1f506e..9b30a0c7`): no High; M-1 the observer quiet once the screen was over would never see the
    fonts the title loads (R12e needs them): on for the session, without an allocation per insertion (a 128-character lower-case copy),
    the fonts read only while a screen needs them; L-1 the bar drawn without the observer (a refused one leaves the screens without
    DS3's fonts, no longer the overlay off); L-2 the words fixed at the bar's left, the percentage right-aligned; L-3 R12-PLAN; the nit
    (the language read once).
  - `r12d-screen3` (18:43, `build-r12d3` = `b9eed5e2`, the second review's fixes, an empty store: `pipelines` 16): the warm-up 2.15 s, the
    three fonts read during the screen, the hold to the title (`evidence/I-0132/artifacts/r12d-screen-title-final.png`, the hold's
    progress, not a real one): « Compiling shaders » and « 50% » crossed at the fixed 220-unit bar: the bar now as wide as the measured
    words, a gap and « 100% » (`41b3e360`).
- **The final review** (fresh, xhigh, `main...de93635a`): no High. M1 the final layout had no still, French never drawn: the stills
  below. Its Lows answered (`fdb160f7`): L1 the TsukuMinPro request and the menu pieces' lookup only for the overlay test until a screen
  needs them (R12e); L2 Run-StartupTrace's stale-flag list extended with the new flags; L3 the warm-up joined before the submissions' lock;
  L4 the lights' two pipelines warmed before SER's state object, SER's apply pipeline (kind 12) only with SER; L6 the bar never narrows;
  L7 the kind count's comment; L8 the documents (this RESULT, 22_setup's comment, STATE at the checkpoint). Left, recorded: L5 the store
  counts the warm-up's pipelines in `frame_created` (its rows read « frame requests' compilations »: the warm-up's fifteen or sixteen
  included since R12d); R12a's final review L4 stays open: the overlay's own shader is compiled (`D3DCompile`) at its first frame, now
  the precompile screen's first frame when it is shown (not warmed: small, and the screen is drawn while it compiles); L9 the screen at the
  bottom right could cover the HUD's souls counter only if the precompile outlasted the load into the world (not seen).
- **The final stills** (`r12d-screen4` in English, `r12d-screen4fr` in French, 19:10-19:14, `build-r12d4` = `fdb160f7`, no fault,
  restored; the hold's progress): « Compiling shaders 49% » and « Compilation des shaders 49 % » apart from their percentage, the bar as
  wide as the words (`evidence/I-0132/artifacts/r12d-screen-corners.png`); the English run's store empty (16 pipelines, the warm-up's),
  the French one's its list (198).
- **The last step review** (fresh, `b9eed5e2..fdb160f7`): no High or Medium; the counts reach their totals with and without SER, WarmSer's
  lock and wait unchanged, the join outside the submissions' lock safe (the workers stopped, the GPU drained). L1 the bar's widest width held
  in pixels across an output change: in 720-line units now (D-0151); L2 SER's kinds named once (`kSerApplyKind`, `WarmTotal`), `305ead6a`.
  Outside the step, recorded: Run-StartupTrace's stale-flag list lacks thirteen older flags (`borderless`, `no-rr`, ...), for a later cleanup.
- **Merged** `fa1ffd43` (19:35 Paris, main unchanged since `cdd1a5f0`; Test-Project passed in main); the launcher rebuilt from main (every
  test passed); start check `start-fa1ffd431` passed (19:46-19:47, the ASUS, 369 world frames, restored).
- **Open**: the warm-up's cost on a cold driver cache with SER (about 24 s, not run); the overlay's own shader compiled at its first frame
  (R12a's final review L4); the store's `frame_created` counting the warm-up; the screen over the HUD if a precompile outlasted the load.

## R12a (2026-11-10): the native overlay, DS3's menu fonts

R12a's first tranche (R12-PLAN « R12a's design », « The game's files for the overlay »; the menu pieces are its second tranche),
from main `19f1cd64`, branch `claude/r12a-overlay`.

- **The overlay** (`native/renderer/overlay_pass.{h,cpp}`, `overlay_frame.h`): our pages' quads (a screen rectangle, a texture
  rectangle, a colour, an image) drawn after the game's UI and before the gamma pass, into the same target, in the startup frames
  (title, menus, loading) and the world frames; its own root signature, pipeline and HLSL (`D3DCompile`; the study's §3.1 proposed
  draws inside the game's UI packet, R12-PLAN gives the reason for a pass of ours); each image a texture made again when its version
  changes, the frame's images in one block of a shader-visible heap that returns to the free list only when every frame that used it
  has completed (the frame slot's kept list). The engine thread hands the latest frame (kept across a reopen); the recording thread
  takes one snapshot a frame, which the frame graph's plan key (bit 59), the pass's declaration and the draw all use.
- **DS3's fonts** (`font_outline.h`, `font_raster.h`, `overlay_text.h`, header-only, unit-tested by `font_raster_test` and
  `overlay_text_test`): DefineFont3 read from the game's `font.gfx` (the SWF shape records, the advances, the kerning), each glyph
  rasterised at the size needed by exact coverage, its blurred copy for the menus' drop shadow (the radius with the output), a shelf
  atlas per font with copy on write, UTF-8, a font chain (a code the first font lacks drawn from the next). **Their bytes come from the
  game's memory**: the Scaleform repository (`GfxRepositoryImp`, `[image+0x4796CE8]`) keeps each file the game's file requests read
  under its resolved path, `data1:/font/<family>/font.gfx` (OBSERVED in `i132-r12a3-overlay`; the `font:/` form found nothing in
  `i132-r12a2-overlay`). Its insertions run on another thread than the engine's (OBSERVED in `i132-r12a6-overlay`: thread 31280 against
  the engine's 33856) and its lookup takes no lock, so the module never calls the lookup: an observer of the insertions
  (`FUN_140E0EA10`, pass-through) keeps each family's node once inserted, and the engine thread copies its bytes once. The English game
  loads MatisseProN (the body) and TsukuOldMinPro (the titles); MatisseProN lacks ë, î, ï, ù, û, ÿ, œ, æ and their capitals (read
  offline on the game's file), so TsukuMinPro (the European languages' body) is requested once through the game's own file request
  (`FUN_140503D30`, as its menus do; its cap kept for the session). Parsing the two large fonts took 110 and 100 ms on the engine
  thread (`i132-r12a7-overlay-debug`, debug layer on), once a session.
- **The test screen** (`16b_rekindled_overlay.inc`, behind `rekindled-overlay-test.flag`, a test tool until the page): a panel, a
  title in TsukuOldMinPro, lines in the body chain with French accents, guillemets and a long dash, shown 180 frames and hidden 60 in
  turn. Without the flag nothing changes, by reading the code (no frame, no pass declared, no hook installed; no flagless run of this
  branch; the start check after the merge is one). Witnessed live on the ASUS (offline, restored by hash, every run with the runner's
  `rekindled-settings.ini` of `dlss=dlaa` and `sharpen=0.25`, so the overlay was seen on DLAA frames only;
  [Run-R12aOverlayWitness.ps1](Run-R12aOverlayWitness.ps1); `Run-StartupTrace.ps1` accepts the flag):
  `i132-r12a4-overlay` (20:40-20:44): drawn at the title and at Firelink, 2 uploads then none, 7 glyphs missing: the body was then
  drawn in TsukuOldMinPro by that build's fallback (its colon, long dash, guillemets and parentheses missing; MatisseProN, read
  offline since, has them); `i132-r12a5-overlay` (21:13-21:18): the font chain, the three fonts read (TsukuMinPro requested), no glyph
  missing, about 354 quads a frame; `i132-r12a6-overlay` (21:42-21:46, build-r12a6): the same, the insertion and engine threads named;
  `i132-r12a7-overlay-debug` (22:15-22:18, build-r12a7, the merged code): the D3D12 debug layer on, `debug_errors` 0 on 9,992 frames,
  the overlay shown and hidden in turn with no frame graph mismatch, no fault, restored. Stills sent to Thomas
  (`evidence/I-0132/artifacts/i132-r12a4-overlay-shots/r12a-overlay-*.jpg`, `i132-r12a5-overlay-shots/r12a-fallback-016.jpg`).
- **Its step review** (fresh Opus, high, `main...b894289a`): **H1** the overlay pass was not in the frame graph's plan key, so the
  world renderer would fault (`frame's passes differ from its plan`) when the overlay appeared or went between two frames of one key,
  and `HasFrame()` was read twice: one snapshot a frame, bit 59 of the key (seen in both states in `i132-r12a7-overlay-debug`); M1 the
  lookup could run while the game inserted (below, the final review's M2); M2 the parser's in-tag bounds untested: every cut inside the
  tag and three changes of every byte tested (no crash; the bounds hold by the parser's checks, the test has no sanitizer); L1 an
  overlay failure never said: the compiler's message kept and a `native_overlay_state` row; L2 a new texture held only by the pass:
  kept by the frame slot too; L3 a full atlas silent: counted in that row (eviction left to the page); L4 the test screen rebuilt every
  frame (a test tool; the page sets a frame only on a change); L5 the insertion observer kept; L6 the runner hid a failed run: it
  throws now; L7 dead code removed.
- **The final review** (fresh Opus, xhigh, `main...cfe9f4b8`): no High. M1 `Close()` dropped the engine thread's latest frame (a page
  set once would vanish after the overlay's opening): kept now; M2 the unlocked lookup still called for the game's own fonts: never
  called now (the observer's node), the threads observed (above); M3 no debug-layer run and the overlay never toggled:
  `i132-r12a7-overlay-debug`; M4 the plan, the RESULT and R12a's exit disagreed on the menu pieces: R12a in two tranches in R12-PLAN, the
  pieces the second. L1 a failure in the step ended the native frame: contained (one row, the overlay off); L2 the shadow's blur fixed
  at radius 1 and a dead field: the radius with the output, part of a glyph's entry; L3 the fonts parsed whole on the engine thread:
  timed (110 and 100 ms, once), lazy parsing left for the page if it matters; L4 the uploads redo work (a committed texture and staging
  a version, the whole atlas copied, the PSO compiled on the first overlay frame): recorded, the precompile screen (R12d) to include it;
  L5 one family's insertion flag: one node a family; L6 a block of an earlier opening could refill a later free list: a generation; L7
  copy on write on `use_count`: an atlas published when handed out; L8 the RGBA8 and BC7 sizes untested: tested (their uploads for
  the title screen's images, R12a's second tranche: the pieces are external); L9 the documents: R12-PLAN's paths, the `SwfCache` question and the pass's reason updated, the 7 glyphs attributed,
  `status.json`; L10 the test's leftovers removed; L11 the stale-block draw (practically unreachable with 32 blocks): recorded.
- **Those fixes' step review** (fresh Opus, high, `cfe9f4b8..38c751b5`): no High, no Medium; L1 the blocks' generation compared
  outside the free list's lock: compared and changed under it; L2 a node that could not be read failed its family silently for the
  session: a row, and a later node tried; L3 two comments still said the inserting thread unobserved: corrected; L4 the blur's test
  only counted entries: it reads each entry's shadow corner (a 3x3 and a 7x7 box); a nit, the step's catch allocated: a fixed buffer.
- **The integrity review** (D-0268, fresh Opus): no High; its Medium (the 7 glyphs attributed to MatisseProN, the request's reason on
  that reading) answered above with the offline reading of the three fonts' codes; its Lows (« none read past the end » by test,
  DLAA-only witnesses, what was witnessed against what only build-r12a6 and later had) answered in the bullets above.
- **Builds**: build-r12a1 (did not compile: a vexing parse in the new `.inc`), build-r12a2 to build-r12a7 (each passed every unit
  test, each run live above, always with the flag); build-r12a8 (those fixes' review answered).
- **Merged** `98db10cc` (2026-11-10 22:44 Paris); the launcher rebuilt from `main`, its unit tests passed (`build-main-98db10cc.log`);
  start check `start-98db10cc` passed (23:00-23:01, the arrival ready in 3.1 s, closed by its close box, restored). The disk rotation:
  1 build folder, 0.2 GB, 410.0 GB free on C:.
- **Open**: the menu pieces (R12a's second tranche: the panels, the cursor's glow, the rules and the arrows of `01_common.tpf`, from
  the textures the native already holds, by name); the page itself (R12c) on the second R12 pilot's door API
  (`02b_rekindled_graphics.inc`), which will install the insertion observer by default.

## R12a's second tranche (2026-11-10): DS3's menu pieces

From main `22a44394`, branch `claude/r12a-pieces`. Since Thomas's course change (2026-11-11 00:06, R12-PLAN: the settings pages stay
the game's Scaleform pages), the overlay and these pieces serve the precompile screen and the title screen only.

- **The pieces come from the textures DS3's menus loaded** (OBSERVED in `i132-r12p2-pieces` and `i132-r12p3-pieces-debug`):
  `TextureIngress::FindByName` returns the first resident record whose content bears a TPF entry name (`MENU_OptionBase`,
  `MENU_ItemParts` of `01_common.tpf`: 2048x1024 and 1024x512, BC7_UNORM); an overlay image can name such a texture
  (`overlay::Image::external`, its owner the record), bound by an SRV of its own format with no upload and no barrier (the ingress
  leaves its textures readable by pixel shaders). By reading only (no run exercised them): an sRGB view re-encoded in the shader (both
  pieces are BC7_UNORM); a withdrawn record looked up again, its generation the image's version (a new record, a new descriptor block);
  only plain 2D textures, all their mips; none once the ingress closes; a pending frame naming an external texture dropped when the
  overlay closes.
- **The rectangles** (texels, in `16b_rekindled_overlay.inc`): measured on the textures decoded offline
  ([decode.py](study-r12-tools/decode.py) with the study's `bcdec.py`, [piece_boxes.py](study-r12-tools/piece_boxes.py): alpha bounding
  boxes of 4x4 cells, each edge to within 3 texels, band scans and the cursor glow's row profile; its output
  `evidence/I-0132/artifacts/r12a-pieces-measure.txt`; the decoded images, licensed bytes, not kept): the Options pages' panel
  (4,0)-(1216,684) of `MENU_OptionBase`; the orange glow behind a selected row (5,77)-(412,116) (it fades in from row 77 and ends hard
  after row 115) and the thin rule (10,4)-(410,6), white in the texture and tinted gold by eye, of `MENU_ItemParts`. INFERRED: these
  are the pieces the game's Options pages draw (their films' bitmap fills not read); the look is judged on the stills. A texture of
  that name with another size is refused (its row says so). The arrows were not drawn (left to the screens that need them).
- **The test screen** draws them once they are found (a plain panel and line before, behind the flag only; without the flag nothing
  changes, by reading): `i132-r12p2-pieces` (23:35-23:37, build-r12p2, the ASUS, offline, restored; DLAA frames): both textures found,
  the panel, the rule and the glow drawn at Firelink, œ, ï and û from TsukuMinPro; a still sent to Thomas
  (`evidence/I-0132/artifacts/i132-r12p2-pieces-shots/r12a-pieces-014.jpg`; its glow's lower rows, 116-121, were the field's, fixed
  since); `i132-r12p3-pieces-debug` (00:06-00:09, build-r12p3, the D3D12 debug layer on): both pieces found (`used`, generations 303
  and 288), `debug_errors` 0 on 8,233 frames, no frame graph mismatch, no fault, restored.
- **Its step review** (fresh Opus, high, `main...c7d97f85`): no High; M1 an external resource changed under one image key kept the
  old descriptor block: the record's generation is the image's version; L1 a withdrawn record never looked up again: it is; L2
  `FindByName` after the ingress's close: none; L3 arrays, cubes and mips: plain 2D only, all mips; L4 the sRGB list incomplete and
  unwitnessed: completed, said; L5 the rectangles for other sizes: refused with a row; L6 a dead flag (`said`): now a count of the
  piece's rows (a record found again is named again, four times at most); L7 the panel stretched: drawn at its aspect.
- **The integrity review** (D-0268, fresh Opus): no High; its Medium (the rectangles' measurement without evidence) answered by the
  scripts and their output above; its Lows (the sRGB path unwitnessed, the tint « as the films tint it ») answered above.
- **The final review** (fresh Opus, xhigh, `main...9e04d6a5`): no High. M1 the glow's bottom edge cut by the scan's band limits (the
  field's rows drawn): its row profile measured, (5,77)-(412,116); M2 the final code unrun and the RESULT's observed and read mixed:
  `i132-r12p3-pieces-debug` with the debug layer, the read-only parts said as such; no unit test of `FindByName` (recorded: the
  pieces serve the two screens only now); M3 the RGBA8 and BC7 upload paths without a producer and stale comments: kept for the title
  screen's images (R12e), the comments corrected (the first tranche's L8 answer, « their uploads come with the pieces », no longer
  holds: the pieces are external). L1 `FindByName` returns the first match even of another size, and scans every 30 frames: recorded
  (two textures, two names); L2 an external texture kept by the pass after no frame names it: recorded; L3 holders past the shutdown:
  a pending frame naming an external texture dropped at the overlay's close; L4 work per quad (a lookup and format checks): recorded;
  L5 the stand-in panel before the pieces arrive: a test tool behind the flag, never for a screen (D-0197); L6 the measurement tools
  approximate: said above; L7 the documents: R12-PLAN's door section points to the second pilot's own record, the arrows said.
- **Builds**: build-r12p1 (did not compile: a local named `image` hid the module's base), build-r12p2 (run live above), build-r12p3
  (the step and integrity reviews' answers, run live with the debug layer), build-r12p4 (the final review's answers: the glow's
  rectangle, the close; every unit test passed, not run live).
- **Merged** `498ea633` (2026-11-11 00:37 Paris, after main's R12c was merged into the branch and the merged tree built,
  build-r12p5); the launcher rebuilt from `main`, its unit tests passed (`build-main-498ea633.log`); start check `start-498ea633`
  passed (00:59-01:00, the arrival ready in 3.3 s, closed by its close box, restored). The disk rotation refused to run (another
  pilot's game or build running): left to the next checkpoint.

## The full control of 2026-11-09 (D-0230)

Main `f3a49990` (R11e and NRD's specular), the launcher's module, 2026-11-09 11:40-14:03 Paris (Thomas, 11:07: « vous pouvez le faire »),
the ASUS alone, offline, every save restored by hash ([Run-FullControl-1109.ps1](Run-FullControl-1109.ps1), I-0125's with the date and
1280x720 added to the control sizes; log `evidence/I-0132/artifacts/full-1109.log`).

- **The replayed path gate** `full-1109-gate` with its memory leg, the ASUS base: **FAIL on the standstill pictures alone**: both
  measure runs' `start.png` depart from the reference (mean 21.47 for a threshold of 1.64, cells 0.166 for 0.005); every cadence
  metric within its threshold (period 16.67 ms; over 2.04 P 1; over 1.5 P 16; display_late 8.5 for 22; game steps over 2.04 P 1.5).
  The pictures are whole-screen captures: the reference (`evidence/I-0083/reference/firelink-start-EA537D5CE027.png`, 2026-10-21) has the
  Claude app around the game's window, this run the desktop's wallpaper; inside the window the native's light is the rays' since D-0253
  and R11d (mean luma of the whole picture 40.6 against 73.3; `full-1109-gate-pictures.png`). INFERRED: the departure is the reference's
  age and the desktop, not a fault; the reference to take again with the rays' lighting and a covered desktop (the gate's thresholds
  predate the rays' default, D-0253, as STATE already says). The memory leg **inconclusive** (8 loads, 7 title stays after a visit,
  eight needed).
- **The control sizes**: 1920x1080 in full screen, a 1500x844 window, 1280x720 windowed: no fault, restoration held.
- **The night endurance** (`full-1109-night`, 12:28-14:03): 95 minutes, 54 visits and loads, no fault, no crash dump, restored.

## The full control of 2026-11-12 (D-0230)

Main `08a47552` (I-0113 tranche 40 after R12e; the launcher's module rebuilt 01:48 from it), 2026-11-12 02:34-04:53 Paris (the coordinator's
slot), the ASUS alone (the OLED off), offline, every save a Shebang copy restored by hash, Alpha untouched
([Run-FullControl-1112.ps1](Run-FullControl-1112.ps1): the 2026-11-09 runner with the tag as a parameter, run from the branch
`claude/musing-keller-d9f6eb` so that the gate's record lands in its checkout; log `evidence/I-0132/artifacts/full-1112.log`).

- **The replayed path gate** `full-1112-gate` with its memory leg, the ASUS base (`evidence/I-0083/gates/full-1112-gate.json`): **FAIL on the
  standstill pictures alone**, against the 2026-10-21 reference: mean 13.47 and 13.50 for a threshold of 1.64 (cells 0.022 for 0.005).
  Every cadence metric within its threshold: period 16.67 ms; frames over 2.04 P 4.5 (8); over 1.5 P 20.5 (27); display_late 8.5 (22);
  game steps over 2.04 P 2.5 (6). Within the thresholds but up: frames over 2.04 P were 0 and 0, 1 and 1, 0 and 2 in the
  2026-10-28, 11-03 and 11-09 gates, 5 and 4 now (the calibration's would-be rises below say the same). The memory leg 4.5 MB a visit over its 8 visits (threshold 40; on 2026-11-09 inconclusive, one quit
  missed).
- **The reference retaken** (D-0268, stated here; the 2026-11-09 control's conclusion): the reference was the raster's, taken before the
  rays' default (D-0253) and every later lighting change; each gate since failed on it. One more measure run of the gate's build
  (`-m3`), then `Invoke-ReplayGate -Calibrate -NewReference -Evaluate` on m1, m2, m3 and v1 (`full-1112-cal.json`): PASS, a
  self-consistency check of the calibration, not a gate pass (m1 judged against itself, the picture thresholds from m2 and m3); the reference
  is now m1's picture (`evidence/I-0083/reference/firelink-start-4382198A7EE8.png`, tree `32b1e295`), m2 and m3 depart from it by 0.41
  and 0.47. Thresholds: display_late 22 -> 14, picture_mean_diff 1.64 -> 1.5; frames over 2.04 P (8 -> 9) and over 1.5 P (27 -> 30)
  would have risen and were kept (no `-AllowRaise`); nothing else changed. The old and new references side by side:
  `evidence/I-0132/artifacts/full-1112-reference-old-new.png`; with the 2026-11-09 control's between them,
  `evidence/I-0132/artifacts/full-1112-firelink-start-1021-1109-1112.jpg`. Observed: the new picture is darker than the 2026-11-09
  control's (the client area's mean luma 56.6 against 67.6, the old raster reference 49.2; `evidence/I-0132/artifacts/full-1112-luma.txt`),
  the back of the shrine darkest.
  Candidates among the merges between the two (not all of them): R11h (the automatic exposure on at half strength, D-0263), R11i (ReSTIR GI), R11j (the TAA), R12g (the GI
  quality) and tranche 40 (the walls off); which one darkens it is not separated. INFERRED: a visual change of the merged lighting work, not a fault (unproved);
  Thomas's eye on the pair stays the judge (D-0181). The order was the reverse of the gate script's (`-NewReference` after D-0181's
  side-by-side check): the reference was retaken at 03:04, the stills shown to Thomas at 05:20. Until he approves them the new
  reference is **provisional**: from this merge the gate no longer sees the darkening of about 11 luma levels since 2026-11-09, cause
  unknown; if he rejects it, the reference and these thresholds are void (the integrity review's M1). **Approved** by Thomas
  (2026-11-12 05:35, in the control's thread, to the three-way still: « Oui ça me va »): the reference stands.
- **The R pilot's four clangd checks** (about 15 s, 02:36-02:37, the coordinator's account) fell inside m1. m1's counts are the highest of the three but within
  their spread (display_late 9, 8, 7; over 2.04 P 5, 4, 3; over 1.5 P 21, 20, 19); its picture is a standstill render, unaffected
  (m2 and m3 within 0.5 of it). A disturbed m1 can only loosen a calibrated threshold (display_late by one at most), never tighten it
  wrongly: nothing run again.
- **The control sizes**: 1920x1080 in full screen, the 1500x844 window (the game makes it 1440x810, as on 2026-11-06 and 2026-11-09),
  1280x720 windowed: each replay complete, no fault, restored.
- **The night endurance** (`full-1112-night`, 03:18-04:53): 94 minutes, 55 visits and loads, no fault, no crash dump, closed by its
  close box, restored.
- « PC libre » was said to the coordinator at 05:16 (the pilot's own account), 23 minutes after the end: this session was restarted
  during the endurance and its waiter lost.
- **The reviews** (final, Opus xhigh; integrity): no high finding. Answered: the reference provisional until Thomas approves the
  stills (both reviews' M1, here and in STATE); the calibration's PASS named a self-consistency check; the luma figures filed; the
  cadence drift stated; the sources of the clangd and « PC libre » times. The runner's reference step after the control (both
  reviews' L1, L2): it now runs only when every reason of the gate is a picture departure, with at least three measure runs, a
  refused run's `-r` preferred, in its own try so a failure there no longer skips the sizes and the endurance; the 2026-11-12 control
  ran its earlier form (commit `95633af7`), whose step took the reference without that check. The unused gate build of the tree before
  tranche 40 (`evidence/I-0083/gates/build-67ac8f4f4436`) is left to the disk rotation.

## Uncertainty / unresolved questions

- R11c/R11b: the DLC arrivals (Ariandel, the Ringed City) hold their 5 s black, DS3's own map not yet in its draw lists (the original's
  picture there not filmed); SER's first setup takes about 200 ms inside the arrival's hold; SER measured on one GPU (an RTX 4070 Ti Super) and only before its
  state fixes; no unit test of SER; the tour judged on main's tree (no SER), JPEG screen captures, Thomas's judgement pending; RR's
  local-light noise, the lit cumulus and a size off 16:9 not looked at; the Dreg Heap not revisited.
- The interior's PT cost (the hall's PT runs wrote no row); why the hall costs twice Firelink at the RT level (traces, hits' lights
  or materials: not separated by the zones).
- The roulette's noise judged on still views at Firelink only (RR on); under ReSTIR GI (the PT level's default since R11i) not looked at; the four-vertex pair
  to time again on one view.
