# Investigation result

## Conclusion

Complete (2026-11-08): tranche 1 (R9a, the forward rows; R9b, the effects) merged `5983d6bd`; R9c (no DS3 option cuts an input of the
rays) merged `5ad1a76f`; R9d (the fill lights' traced shadow) merged `7ca576d4`, on by default with DS3's AO weights since I-0131
(Thomas's option 2, D-0259). Tranche 1: DS3's one probe a forward model is traced on
the rays' frames (its ambient on the GI frames, its sun's mask with the traced sun), through the constants DS3's own programmes read
(OBSERVED in the records for four characters: the traced ambient of DS3's order, warmer); the lit effects' probe query 1DB60 answers
from the same probes by cell (the detour's writes INFERRED: logged by `rt-probe-observe.flag` since the final review, not yet seen in a
run); about 73 µs a frame measured for about 12 probes at 64 rays (before the final review's 128). Thomas's look in play is to come.

## Study A (2026-11-08)

- **The forward rows** (OBSERVED, `fxc /dumpbin` in `evidence/I-0129/dxbc/`): `GXFlver_ColDifSpcBumpIbl_Fwd` and
  `SAT_HairIbl2_Fwd_[A]` take their ambient from the instance record's three `aSh` rows, `max(dp4((n, 1), row), 0)` a channel, and
  the sun (the light set's first directional light, `FC_DirLightCol_0` and `Spc_0`) times `ShLightMask.x` (the record's +0xB0); the
  fill lights carry no mask. The native draws `Fwd`, never `FwdSdw` (the cascaded shadow map lookup): its forward rows take the sun
  unshadowed. The deferred `*_Gbuf` programmes write the same `ShLightMask.x` into GB1.z (`ColDifSpcBumpIbl_Gbuf`,
  `SkinSSSIbl_Bloody_Gbuf`: `mov o1.z, cb4[… + 11].x`), the deferred pixels' sun mask under the traced shadow (STUDY-R7-DS3-AMBIENT
  §1.1).
- **The effects** (study A's Ghidra report, `evidence/I-0129/study-a/NOTES.txt`; 1DB60 re-read by the pilot): the classic draws'
  `PSCommonBuffer` (264230) and the GPU particles' `cbCubeEnvSH1` rows (2A4EB0) both come from DS3's probe query **1DB60** at the
  effect instance's world translation, three SH rows of the same form and a float4 mask that the bump programmes take on the sun
  (`g_vLightMask.x`, cb0[13].x, OBSERVED in `GXFfxsoftBumpTexture`); the directional colours are the light set's, without a
  position. 1AC840 (the instances' probe) is not on the effects' path.
- So both consumers read one probe at their position: R9 traces that probe (R9-PLAN §2) instead of changing DS3's programmes.

## R9a (2026-11-08): the traced probes, the forward rows

- `native/renderer/rt_probes.h` (the CPU side, pure; `native/frame/rt_probes_test.cpp` passes): the requests (an object's box, its
  light set), the results (the sums of L and L ω over the valid rays, the sun's visible rays), the rows `mean(L) + 2 mean(L ω) · n`
  (exact for a uniform sky and a hemisphere in the test), the states by key (the first answer whole, then a blend a frame; the frame's
  bits: the ambient only after a GI frame's probes, the sun only after a traced sun's; none after 500 ms without a delivery; the
  unseen dropped after 120 deliveries), the values quantized to a 10-bit mantissa so a still light keeps the same record bytes.
- `rt_reflect.hlsl`'s `probe` (cs_6_6, one group of 64 a request, 128 since the final review): a uniform direction from a uniform point of the box moved to
  where it leaves the box, TraceSurface and HitRadiance as the GI's ray (the cache at the hit), a miss the forward sky or Unmet, a
  back face not counted (`RtSurface.back`, new); one sun ray a thread from another point of the box. `RtReflections::EncodeProbes`
  (after the cache, `GpuZone::RtProbe` for `rt-gpu-measure.flag`), its results copied into the frame slot's readback, delivered by
  the slot's completion.
- The bootstrap: one request a model that draws forward rows this collect and has its candidate's world box (`NativeRtProbeRequests`),
  keyed by its draw group and model, so a character's deferred parts keep their record (their GB1.z stays DS3's under the traced
  shadow); `ApplyNativeRtProbe` writes the rows and multiplies `ShLightMask.x` in that model's record. `no-rt-probes.flag` keeps
  DS3's probes; the `native_rt_probes` row (on, why not, frames, states, deliveries).

## R9a's first slot (2026-11-08 07:58-08:09, the ASUS, no measure but `probegpu`; `evidence/I-0129/artifacts/runs-r9a1.log`)

- Build `build-r9a1` (R9a and R9b as committed `e3ba8457`). Runs `i129-r9a1-*` and `i129-r9a1b-*` (Run-R7aGi, 1280x720, Shebang
  saves restored by hash): Firelink's interior (`m40chair`) and Anor Londo's hall (`m37hall`), `gi` (the traced probes) and `noprobe`
  (no-rt-probes.flag; its first two runs refused by Run-StartupTrace's pattern, which named the flag in one of its two groups:
  fixed, run again as `i129-r9a1b-*`), then `probegpu` at Firelink's yard with `-Measure`. Six launches, no fault, every save restored.
- **The probes run in the game (OBSERVED)**: `native_rt_probes` on, a delivery every traced frame (1,074 of 1,075 at Firelink), 12 to
  32 states (the forward models and the effects' cells about the camera).
- **Their cost (OBSERVED, `native_rt_gpu`, 1280x720, the RT level, 600-frame windows)**: `rt_probe` 73 and 74 µs a frame on average
  (117 and 143 µs at most), about 1 % of the frame's 6.7 ms, for about 12 probes at Firelink's yard.
- **The pictures**: the first shot after the teleport (`walk-1.png`; the later shots lost the window's focus), `r9a1-sheet.png`:
  nothing broken or black, the hair and the body alike in both modes by eye; the character's idle pose differs between runs, so a
  crop's brightness does not discriminate (the hair 32.1 against 30.5 of 255 at Firelink: not interpreted). The values themselves
  need the records: `rt-probe-observe.flag` (added after this slot) logs each probed model's baked and traced rows.

## R9a's step review (fresh Opus, high, `295ef113b..e3ba8457`) and its answers

- **H1**: a model that draws forward and G-buffer rows shares one record, whose `ShLightMask.x` the G-buffer programmes write into
  GB1.z (the deferred sun's mask under the traced shadow): the sun would have been masked twice there. Answered: such a model (any
  row of engine pass 5 to 7) asks no sun rays (`kRtProbeRequestNoSun`, the request's flag the probe reads), so its mask stays DS3's;
  water models (rows 0x24, 0x27) ask nothing (their lighting is R11's).
- **M1**: a shadowed probe ray now counts the deferred sun's floor (`lerp(1 − FC_ShadowColor, 1, 0)`, its luminance), as HitLight's
  `seen`. **M2**: the states give their average only once it moved by 3 % (rows) or 0.03 (sun) from what they gave (`RtProbeSettle`,
  tested), the blend then 0.1 a frame (the final review's M4 below: not enough against the rays' noise). **M3**: the probes' heap slots released at
  the renderer's close (`ReleaseProbeViews`). **M4**: 2A4EB0 calls 1DB60 as a statement and reads only its outputs (OBSERVED, its
  decompile); 2ADC80 and 2AE5A0 INFERRED alike.
- **L1** the states cleared at a scene reset; **L2** the effects' detour idle once the probes failed; **L3** the raw view's offset
  checked against D3D12's 16 bytes. **L4** (past 1,024 forward models the tail never probed): not answered, a probe set of 12 to 32
  seen (recorded).

## R9a's second slot (2026-11-08 08:22-08:25, the ASUS, no measure; `runs-r9a2.log`)

- Build `build-r9a2` (the step review answered, `7f429d05`), `rt-probe-observe.flag` (Run-R7aGi's `probeobs`), Firelink's interior and
  Anor Londo's hall; two launches, no fault, restored by hash. Each probed model's last record (`native_rt_probe_record`: its band 0,
  the ambient a surface takes whatever its normal, baked against traced, linear):
  - **The characters** (Shebang's parts, `FG_A_*`, `HR_A_*`, `FC_*`): Firelink 0.013/0.014/0.020 baked against 0.020/0.017/0.014
    traced; a second character 0.008/0.009/0.011 against 0.010/0.007/0.006; a third (`HR_A_0011`, `FC_M_0112`, darker in its corner)
    0.003/0.003/0.004 against 0.015/0.012/0.011. Anor Londo's hall 0.031/0.060/0.090 against 0.054/0.070/0.073. So the traced ambient
    is of DS3's order (the same magnitude, within a factor 1 to 1.7, but one dark corner's fivefold), warmer than the baked probes' blue
    (the candles and the fires the rays see; DS3's probes are the zone's sky hemisphere): OBSERVED for these 4 characters, INFERRED
    beyond them.
  - **The sun's mask**: inside (Firelink's interior) DS3's own mask was 0 and stays 0; in the hall 0.715 stays 0.715 (the probe's
    share times it, the share 1 there or the models of mixed rows asking none); not yet seen where the traced share is below 1 and
    DS3's mask 1 (an outdoor shade).
  - **Mixed models** (map pieces `m40_00_00_00_0012**`, `m37_00_00_00_0100**`, with G-buffer and forward rows): no sun rays (H1), their
    rows baked 0 (no bank, no animation: ProduceInstanceRecord's zero) and traced 0.13 to 0.17 at Firelink, 0.016 to 0.04 in the hall:
    their forward rows take an ambient DS3 gave them none of, if their forward programme reads `aSh` (a lightmapped `*DOL_Fwd`
    does not: TO CHECK which). Not seen as a change in the pictures of the first slot.
- The effects' cells answer through 1DB60 (states 13 and 32 counted with the models), their values not logged by this flag.

## The final review (fresh Opus, xhigh, `main...651d0961`) and its answers

- **H1** (a stored sun share brought the double mask back: a model that once asked sun rays kept its share after it gained G-buffer
  rows, and a state answered after its key was no longer asked): a frame that traced the sun clears the share of a key it gave no sun
  rays; a state answers only while its key was delivered within the last 8 deliveries (`kRtProbeRecent`); tested.
- **M1** the effects' mask times DS3's own (as the forward rows'), no longer replaced. **M2** a cell not answered yet takes a face
  neighbour's answer (a moving effect). **M3** boxes at most 8 m a side, 4 m for a model with G-buffer rows (a room-sized map piece's
  box saw outside its room: the mixed map pieces' 0.13 to 0.17 above), else DS3's rows. **M4** the hysteresis lost to the rays' noise
  (the band-1 estimate's spread about 0.14 L a frame at 64 rays): 128 rays a probe and a blend of 0.05 (the spread after it about
  0.016 L, INFERRED); the records' stability under it not measured (the claim withdrawn). **M5** the detour's writes logged under
  `rt-probe-observe.flag` (`native_rt_probe_effect`), its effect INFERRED until seen.
- **L1** the frame's bits decided before the uploads; **L2** an epoch: the answers of requests made before a load's Clear dropped;
  **L3** (every admitted model looks its key up; a character's parts trace the same box): recorded, not changed; **L4** (the readback
  held twice): kept, the slot's rule; **L5** the records corrected (this section, the conclusion, the index row, status).

- The fixes' step review (fresh Opus, high, `651d0961..0def9cc0`): no High; M1 the index row and status claimed more than the runs
  (128 rays and the fixes unrun then): the row corrected (73 µs measured at 64 rays), the merged tree run by the start check; L2 a dead
  check folded; L3 the C++ ray count dropped (the shader's define the one source); L4 comments; L5 the detour's log gives the key that
  answered and whether a neighbour did, sampled by cell; L7 the epoch required; L8 the test checks kRtProbeRecent's edge. Left: L6 (7
  locked lookups an effect query on a frame that traced nothing), L9 (the detour's sampling statics outside the thread-affinity scan:
  a mutex and plain counters under it, run on DS3's effect threads).

## Tranche 1 merged; R9c opened (2026-11-08)

- **Merged** `5983d6bd` (09:18); the launcher rebuilt from `main`, its unit tests passed; start check `start-5983d6bd` passed
  (09:35-09:36, restored).
- **R9c** (Thomas, 2026-11-08 08:55: « idem pour le jitter et tout ce dont ont besoin le DLSS et RR puis RT/PT »): the motion
  vectors and the jitter already follow only the upscaler and the GI (I-0111: `CollectNativeVelocityInputs`, `ApplyUpscalerJitter`;
  OBSERVED). An audit of the inputs DS3's graphics options gate (a Sonnet reading, the three below re-read by the pilot, OBSERVED):
  Shadow Quality's on byte (+0x28) turns the shadow frame's `on` off, and with it the traced sun and the hits' sun mask (the cascades
  are not drawn under the traced sun: the sun then unshadowed); Lighting Quality (+0x44) leaves the local lights unmarked as shadowed,
  so the RT level's rays trace them unshadowed; Reflection Quality's DISABLE (+0x6C) binds the water's black default instead of its
  traced reflection. Antialiasing decides DLSS and so RR, by design (DLSS in the AA's place).
- **The sheet and the first lowest-options run** (09:36-09:50, the ASUS, no measure, `runs-r9sheet.log`, `Run-R9Sheet.ps1`, build
  `build-r9c1`, the code before R9c's reviews): raster (`no-rt-scene`), RT and PT at most (4 vertices), DLAA 1920x1080, Firelink's
  yard, the High Wall, Anor Londo's hall, the first shot after the teleport (`rt-sheet-raster-rt-ptmax.png`, shown to Thomas). Mean luma
  (signalstats, of 255): Firelink 50.8 / 58.1 / 59.3, the High Wall 53.3 / 59.7 / 59.2, the hall 37.2 / 46.0 / 46.5: the rays lift the
  shade (the GI's bounce), the PT level at most about the RT level's on these views (sun-lit, few lights). By eye: the hall's metal
  reflections (the pillar, the door) only traced; Firelink's arches lit by the bounce. The lowest options (`i129-low-*`, Lighting
  LOW): its byte +0x44 is 0 there (`native_shadow_state` « lighting »: 0), so the gate mattered; with r9c1 the local shadow items were
  found (`native_local_shadow` hits 1 to 5) and the water plane published at Reflection DISABLE (OBSERVED in its startup.jsonl).
- **R9c's reviews** (step: fresh Opus, high, `f6bede0a..34107145`; final: xhigh, `main...029df738`): answered: the shadow frame's
  `on` set once the collect decided the traced sun (no cascade drawn because of it when the rays fail or skip), logged
  (`native_rt_sun_option_forced`), the sizes' fallback removed (every Shadow row gives at least 128); the local lights' gate follows
  the collect's `local_traced`; Lighting Quality's fade scale (+0x40: 0.5 at DISABLE and LOW) at least HIGH's 1.0 with the rays'
  lights; **the water's DISABLE kept** (the step review found the water material's FC_WaterFresnel words zeroed by the same option,
  kept in the admission's record for up to 512 collects: forcing the reflection left a dark water whenever the trace stopped;
  Reflection DISABLE stays the player's « no reflection » until R12's menu replaces DS3's options).
- **The final tree's run** (10:25-10:28, the ASUS, no measure, `runs-r9c4.log`, `Run-R9cLow.ps1`, build `build-r9c4`: R9c with
  main's tranche 32 merged), Firelink's interior at DS3's lowest options (Shadow DISABLE, Lighting LOW, Reflection DISABLE, ...),
  OBSERVED in the journals: Shadow Quality DISABLE writes 0 (`native_shadow_state` « shadows_on »: 0); with the rays, the frame forced
  on (`native_rt_sun_option_forced`, its 20 rows) and the traced sun drawn every frame (the graph's `rt_sun_shadow` 1,201 of 1,201);
  Lighting LOW writes 0 and the local shadow items still found (`native_local_shadow` hits 2 to 4); with `no-rt-scene.flag` (the
  rays off) nothing forced (no row, no `rt_sun_shadow` pass, local hits 0: DS3's option behaviour). The failed and skipped cases: by
  reading only (the force sits after `sun_traced`, which carries them). Two launches, no fault, restored by hash.
- **Merged** `5ad1a76f` (10:29, main's tranche 32 merged in before it); the launcher rebuilt from `main`, its unit tests passed; start
  check `start-5ad1a76f` passed (10:41-10:42, restored). Next: R9d (R9-PLAN §4), the fill lights' traced shadow, a trial.

## R9d (2026-11-08): the fill lights' traced shadow, a trial behind `rt-fill-shadow.flag`

- **The code** (R9-PLAN §4; `6d8d9ea0`, `7c08d3cc`, `4ab0559a`, `336383d9`, then the reviews' `a41b09e3`, `b5c3839f`, `888ee14c`): on the GI frames with the mask, `rt_reflect.hlsl`'s
  `fill_shadow` multiplies the mask's AO channel (t32.x, white since D-0251) by each pixel's visibility toward its light set's fill
  lights, blended ZERO / SRC_COLOR into the red channel only, after the mask's composite and before the lighting's passes
  (`EncodeRtFillShadow`, graph pass `rt_fill_shadow`, plan key bit 55, GPU zone `rt_fill_shadow`); each set's FC_SsaoWeight (row 17)
  keeps DS3's weights for lights 1 to 3 and takes 0 for light 0 on those frames (since `b5c3839f`; before it, and with
  `rt-fill-shadow-all.flag` since, (0, 1, 1, 1)), so DS3's directional programme shadows the fill lights as its data weights their AO
  and leaves the traced sun alone. The visibility: two rays a pixel, each picking a fill light in proportion to its diffuse term there
  times its row-17 weight (DirLights' forms: light 1 Lambert;
  lights 2 and 3 only with their bLocalLight flag, wrapped), light 1's ray in a 45° cone about its direction, a wrap light's a cosine
  direction about N + L, against what the eye sees (RtMaskViewBits, the GI's geometry). The `native_rt_gi` rows carry its frames,
  missed frames and failure; `native_light_fill` each set's fill rows; Run-R7aGi's modes `gifs`, `gifsall`, `ptmaxfs`, `gifsgpu`.
- **Step review** (fresh Opus, high, `e06e0720..6d8d9ea0`): no High or Medium. Answered: L1 the PSO's format MaskStage's; L2 the frames
  without a top level counted (`fill_shadow_missed`); L3 Get-R8Gpu's zones (`rt_probe`, `rt_fill_shadow`) and the cost mode `gifsgpu`;
  L4 the mask back to PIXEL_SHADER_RESOURCE on any exception.
- **Three slots** (2026-11-08, the ASUS, no measure, offline, restored by hash, no fault in 33 launches; `Run-R9dSheet*.ps1`,
  `runs-r9d1.log` to `runs-r9d3.log`; DLAA 1920x1080, Firelink's yard, the High Wall, Anor Londo's hall, Firelink's lower corridor
  `m40low`): the first (11:49-12:04, `build-r9d2`, light 1 only, the sun casters' mask) and the second (12:42-12:59, `build-r9d3`,
  lights 1 to 3) left the mask's red channel about white (its mean 255 at Firelink, 244 to 255 elsewhere, from the runs'
  `world-*-mask.rgba`); `native_light_fill` showed Firelink's sets with a cool fill from above (set 1: light 1 (0.18, 0.94, 0.29),
  colour (0.41, 0.57, 0.79); its row 16 printed as floats then, read as integers since the fourth slot: [1, 0, 0, 0], light 2's wrap fill
  active, the final review's L1), so the rays met nothing they could see: Firelink's map pieces are INFERRED not to
  be sun casters (DS3's category bit 0x02). The third (13:27-13:35, `build-r9d4`, the view mask) shadows the fill: the mask's red
  mean 38 at Firelink (the shrine's roof and arches), 176 at the High Wall, 228 to 241 in the hall (OBSERVED in the masks).
- **The sheet** (`rt-sheet-r9d-fill-shadow.png`, Make-R9dSheet.ps1: the flag-off runs `i129-r9d2-*`, the flag's `i129-r9d3-*`, the first
  shot after the teleport, the character's pose differs between runs), mean luma (signalstats YAVG of 255, the client area) without
  then with the shadow: Firelink 58.0 → 54.3 (PT at most 59.2 → 55.3), the corridor 23.0 → 20.5, the High Wall 59.4 → 56.1 (59.0 → 56.2),
  the hall 44.9 → 45.2 (46.2 → 45.4: the pose's, not interpreted). By eye: in the corridor the character loses the cool fill and keeps
  the candles' warm light (D-0257's « no fill » look, now only where the fill is occluded); the hollows and the stairs' undersides
  darker elsewhere. Shown to Thomas (13:40) for his judgement; the flag stays off by default.

- **The fixes' step review** (fresh Opus, high, `80e1cd3f..336383d9`): no High. Answered: M1 light 1's rays weighted by their cosine
  to the shading normal (none below it: Lambert's), no longer mirrored; M2 the geometric normal from the depth's neighbours (a light-1
  ray under it dropped, a wrap ray mirrored above it, the origin lifted by the shadow rays' offset: a normal map no longer occludes
  its own receiver); M3 recorded: rays of 1e5 m against the view mask make a sky-like visibility, so an interior's fill goes (the
  dark corridor is this estimator's expected answer, read against the flag-off runs, not a proof that the fill is right); L1 the
  `native_light_fill` row kept whatever the flag (at native_light_entries' rate); L2 row 16 logged as integers; L3 the pick never
  takes a light of weight 0. Left: L4 (a negative fill colour weighs 0 but row 17 still scales it: none seen in the sets logged), L5
  (the pass recomputes the GI pass's surface: fold it into `gi` if the trial is kept).
- **The fourth slot** (14:15-14:23, `build-r9d5`, the fixes' step review answered; `Run-R9dSheet4.ps1`, `runs-r9d4.log`; 7 launches, no
  fault, restored by hash): the mask's red mean 47 at Firelink (38 before the fix: the normal maps' self-occlusion gone), 185 at the
  High Wall (176), 234 to 243 in the hall. The sheet `rt-sheet-r9d-fill-shadow-2.png` (the flag-off runs `i129-r9d2-*`, the flag's
  `i129-r9d4-*`), mean luma without then with: Firelink 58.0 → 54.5 (PT at most 59.2 → 55.4), the corridor 23.0 → 20.6, the High Wall
  59.4 → 56.8 (59.0 → 56.9), the hall 44.9 → 45.4 (the pose's). Shown to Thomas (14:30), in place of the first sheet.
- **E's scene scale** (I-0113 tranche 34, relayed 2026-11-08 13:45): DS3's pass row scale (+0x110, SH Color Scale: 0.5 at the High
  Wall, 0.01 at m30_01) scales the baked instance rows. The traced probe does not carry it (R's answer): on the GI frames it replaces
  the baked rows with traced radiance in the units of the deferred GI pass, which replaces GB4 with no scene scale (D-0249); scaling
  the forward rows alone would light a character and the wall behind it differently under the same light.

- **The final review** (fresh Opus, xhigh, `main...6f906a15`): no High. M1 (the forced (0, 1, 1, 1) overrode DS3's own FC_SsaoWeight,
  which exempts light 1 in Firelink's set 1 (0.75, 0, 1, 1) and Anor Londo's hall's sets (1, 0, 1, 1), OBSERVED in `native_light_fill`
  of `i129-r9d4-*`; STUDY-R7 §1.1's « light 1 fully AO-weighted everywhere » marked contradicted): by default the trial keeps DS3's
  weights for lights 1 to 3 and zeroes light 0's, the rays' choice weighted by them; `rt-fill-shadow-all.flag` the full variant (the
  first two sheets' form). L1 the RESULT's « no bLocalLight flag » corrected; L2 light 1's ray weight normalised; L3 the sheets'
  flag-off columns came from another build (main's tranche 33 merged between): the fifth slot runs every column from one build; L4
  the graph's consistency (0 `graph_barrier_mismatch`, the pass on 1,201 of 1,201 frames in the fourth slot's runs) cited here, the
  failure path and NRD's frames with the flag (two rays' raw noise) reviewed in code, not run; L5 the redundant gates removed; L6
  this record, status, STATE, the CONTRACT's scope. Its fix's step review (`6f906a15..b5c3839f`): H1 (an offscreen view, recorded
  before the world frame, saw the last world frame's `rt_fill_shadow_now_`: the pass on another view's targets) fixed by a gate in the
  lighting itself (`fill_shadow`: the main view with the mask); L1 light 1's weight over the cone's mean cosine (its mean 1 in an
  unclipped cone, 1.003 to 1.026 clipped: the next review's Monte Carlo); L2 row 17 unclamped in the pick; L3, L4 comments and a
  memcpy. That fix's review (`b5c3839f..888ee14c`): nothing found. Build `build-r9d7`, its unit tests passed.
- **The fifth slot** (15:01-15:38, `build-r9d6`, `Run-R9dSheet5.ps1`, `runs-r9d5.log`): 7 of 18 runs made (Firelink's yard and
  corridor, `gi`, `gifs`, `gifsall`; the High Wall's `gi`), no fault, restored by hash; the 11 others refused by Run-StartupTrace's
  disk guard (C: at 909 MB free: this branch's raw run frames, 13 GB, and two builds), before any backup or launch. The raw frames
  of the R9d runs removed but the masks (D-0142). The rest in a sixth slot (`Run-R9dSheet6.ps1`). The pilot's camera input was refused
  in three of the seven (`m40-gifs`, `m40-gifsall`, `m30-gi`: the window lost the focus to the Claude app); the sheet uses the first
  shot after the teleport, taken before any input.
- **The sixth slot** (15:57-16:10, `build-r9d7` = `888ee14c`, the H1 fix; `runs-r9d6.log`; 11 launches, no fault, restored by hash):
  the High Wall's `gifs` and `gifsall`, the hall's three modes, the PT level at most without and with the flag. Builds: `build-r9d6`
  = `b5c3839f` (the fifth slot), `build-r9d7` = `888ee14c`; between them light 1's ray weight over the cone's mean cosine (a
  difference only where light 1 is chosen: Firelink's sets give it weight 0 by default, so only the `-all` columns at Firelink and
  in its corridor come from the earlier shader, said on the sheet's record). `build-r9d8` = `c8d68322` (main merged): its unit tests
  passed. The graph: `fill_shadow_frames` equal to the GI frames, 0 `graph_barrier_mismatch` in the r9d4 to r9d6 runs.
- **The default form's effect** (OBSERVED, the mask's red mean of `world-*-mask.rgba`, flag off / DS3's weights / `-all`): Firelink's
  yard 255 / about 246 / about 47; its corridor 255 / 246 / 47; Anor Londo's hall 255 / 248 to 250 / 234 to 243; the High Wall 255 /
  185 / 186. Every Firelink set logged gives light 1 (the cool fill from above, colour (0.40, 0.57, 0.79)) the AO weight 0, so DS3's
  data leaves it unshadowed; only light 2's wrap fill is. Mean luma of the first shot (YAVG of 255) flag off / default / `-all`:
  Firelink 57.9 / 57.5 / 54.7, the corridor 23.1 / 21.7 / 20.5, the High Wall 59.9 / 58.2 / 56.4, the hall 44.9 / 45.1 / 44.4 (the
  pose's); the PT level at most off / default: Firelink 59.4 / 58.6, the High Wall 59.6 / 57.2, the hall 46.1 / 45.7. So the default,
  DS3's data, changes little at Firelink and in the hall; the first two sheets Thomas saw were the `-all` form. The sheet
  `rt-sheet-r9d-fill-shadow-final.png` (five columns: off, default, `-all`, PT at most off and default) puts the choice to Thomas.

- **Merged** `7ca576d4` (16:16), the trial off by default; the launcher rebuilt from `main`, its unit tests passed; start check
  `start-7ca576d4` passed (16:29-16:30, restored). The final sheet and the choice (off, DS3's weights, `-all`) posted to Thomas (16:20).

## Uncertainty / unresolved questions

- R9d: Thomas chose DS3's AO weights (2026-11-08 17:51, D-0259), on by default since I-0131 (`no-rt-fill-shadow.flag`); `-all`
  (every fill light shadowed) retried with the light sources' rework in R11; its cost not measured (`gifsgpu`); one visibility for lights 1 to 3
  (exact for their AO-weighted luminance, a tint where their colours differ, and their specular scaled by the diffuse's V); NRD's frames
  keep the rays' noise; the specular ambient darkened by the same channel; the pass recomputes the GI pass's surface (to fold into the
  GI pass with R11's optimisation); its cost not measured yet.

- The fill lights (Thomas, 2026-11-08 08:42): the forward rows keep them direct (cbLight's lights 1 to 3), and the probes' rays light
  their hits with them too (HitLight's DirLights), so their bounce adds to the direct fill: the same partial double count as the
  deferred surfaces' under the GI (D-0257), which goes with their traced replacement.
- The mixed map pieces' forward rows: which programmes read the traced rows (Ibl) and which ignore them (DOL); the effects' cells'
  values (not logged); the sun's share where DS3's mask is 1 (an outdoor shade); Thomas's look in play.
- Not done: the E03 model particles (605, the Estus Flask in the hand: their own FXR light or no box), the lightmapped forward rows,
  the classic bump effects' sun (its mask is the FXR's own after 1DB60, R9-PLAN §1.2), the water (R11); the step review's L4 (more
  than 1,024 forward models a frame).

## Artifacts produced or modified

- `evidence/I-0129/dxbc/` (the disassemblies), `evidence/I-0129/study-a/` (study A's scripts and outputs).
- R9d: `evidence/I-0129/artifacts/Run-R9dSheet.ps1` to `Run-R9dSheet6.ps1`, `Make-R9dSheet.ps1`, `runs-r9d1.log` to `runs-r9d6.log`,
  the sheets `rt-sheet-r9d-fill-shadow.png`, `-2.png` (the `-all` form) and `rt-sheet-r9d-fill-shadow-final.png`; the runs `evidence/I-0066/artifacts/i129-r9d1-*` to `i129-r9d6-*`
  (their journals, shots and masks); the builds `evidence/I-0129/build-r9d7` and `build-r9d8`.
