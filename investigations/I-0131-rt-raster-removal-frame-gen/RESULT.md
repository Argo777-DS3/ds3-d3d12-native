# Investigation result

## Conclusion

R10 (2026-11-08, the R pilot; [R10-PLAN](R10-PLAN.md)): R10a removed the raster lighting that still came back whenever the rays did
not serve a frame (the sun's cascades, the local shadow atlas, the cubemaps' pass, the shadowed batches; one fallback without
visibility left), the traced frames unchanged (the second slot); R10b added DLSS frame generation (`fg-mode.txt`: on/x2, x3, x4, capped
by the adapter; off by default), two frames presented a frame on the ASUS; R9d's fill shadow the default (D-0259). The step reviews
and the final review answered (« Final review »); merged with main's I-0113 tranche 35.

## The inventory (2026-11-08)

A Sonnet reading of every raster lighting path, the rows the plan rests on re-read by the pilot (R10-PLAN §1). OBSERVED: the sun's
cascades and the local shadow atlas were not removed by I-0118 and I-0123: both still came back on every frame the rays did not serve
(`sun_traced` / `local_traced` false: the rays off, a latched failure, 30 skipped walks), as did the environment cubemaps' pass and
DS3's local-light batches; the menu views are lit by DS3's programmes alone; DS3's directional programme shades the sun with the
traced visibility on every frame.

## R10b: DLSS frame generation (2026-11-08)

- **The code** (`9dd7eacf`, the review's answers `4890e468`): `fg-mode.txt` (« on ») or `fg-on.flag` loads `kFeatureDLSS_G` with
  `eUseFrameBasedResourceTagging` (`streamline_host`); with it loaded the chain (Streamline's proxy, as since I-0111) is made without the
  frame-latency object and Reflex is forced On; on a DLSS frame with the gamma pass (`fg_frame`, plan key bit 56) DLSS's depth and
  motion are tagged after its evaluation (`eOnlyValidNow`, its frame token) and the gamma pass is drawn once more into the HUD-less
  target before the UI (`fg_hudless`), tagged the same way; before each Present the mode is On for such a frame with Reflex on, Off
  otherwise (`eRetainResourcesWhenOff`); Off before each chain change and at Close; a refusal, a status other than OK or a presentation
  under DLSS-G's minimum latches it off (`native_frame_generation_latched_off`) and frees its resources. Rows `native_frame_generation`
  (presents, On presents, `numFramesActuallyPresented`, status, minimum size, V-Sync support).
- **Step review** (fresh Opus, high, `c6e750bc..d6e5d690`): no High. M1 Close turned nothing off before leaving the full screen; M2 the
  status unread and Reflex not required; L1 the flag after a refused proxy chain; L2 an early Off counted as a refusal; L3 two calls for
  the HUD-less target; L5 its resources kept after a latch: all answered in `4890e468`. L4 (the Off before a chain change has no Present
  between) to witness; L6 two redundant barriers on frames without UI (suppressed when the graph drives); L7 the motion blur's halving
  (NVIDIA's checklist) deferred: YEBIS stays until its replacement (RT-PT-PLAN §8).
- **The first slot** (17:12-17:21, the ASUS, no measure, `Run-R10bSlot1.ps1`, `runs-r10b1.log`, build `build-r10b1` = `d6e5d690`,
  before the review's answers; 7 launches, no fault, 0 `graph_barrier_mismatch`, restored by hash): with `fg` the rows show
  `presented` 2 (two frames presented a frame), status 0, V-Sync supported, at Firelink and the High Wall at 1920x1080 and Firelink at
  1280x720 and 1600x900 (OBSERVED); `gi` (off) as before. Streamline's warnings (OBSERVED in the journals): `slDLSSGGetState` should
  run on the present thread (it does: the render worker), no backbuffer extent given (it takes the full back buffer: right), a tagged
  resource's alignment overridden, the frame timer reset after a 100 ms frame (the load). The display-mode run did not step: the
  cycle's first step is at frame 3000, after the run's end; again in the next slot with a longer run.

## R10a: the raster lighting removed (2026-11-08)

- **The code** (a Sonnet agent on branch `r10a-removal`, `d55588d0` to `d1cf7065`, merged `3880811e`; 28 files, +230/-1485): the local
  lights' batches always unshadowed (the shadowed batch kind, its views, root parameters 10 and 11 and sampler s12 removed); the
  native lights pass shades unshadowed without a top level (`traced` 1 or 2, the shader's `traced==0` branch gone);
  `rt-lights-check.flag` removed; the local atlas (`CollectNativeLocalCasters`, the faces, the caster rasterizer, `NativeDepthAtlas`)
  removed, `CollectNativeLocalShadows` keeping the selection and the shadow words the rays read; the sun cascades (`CollectNativeShadowCasters`,
  `SunShadowStage`, the cascade fit and volumes, the casters' engine pass 1 and its rows, `no-caster-volume.flag`) removed, the traced
  sun's inputs kept (the direction as `rt_sun`, the scene words with `FC_ShadowColor` in the shadowed sun pass's b8, `on`, the R9c
  force); the environment regions' raster pass (`EncodeNativeEnvRegions`, its programmes and views, `no-rt-reflect.flag`) removed, the
  regions and cubes kept as the rays' data, the specular ambient cleared on a frame that did not trace. A frame `sun_traced` false has its
  mask cleared white (the sun unshadowed), the single fallback.
- **Step review** (fresh Opus, high, `c6e750bc..r10a-removal`): no High. M1 a light was shadowed on its slot-3 hit alone (before, it also
  needed a face DS3's atlas could draw): the face test restored without the atlas (`NativeLocalHitHasFace`, the same finiteness, tile
  and capacity rules; `bad_faces` in `native_local_shadow`); M2 the rays' shadowed lights gated by the 12 removed raster programmes,
  and a record refusing its shadow words made inert: the programmes' lookup removed, such a record left unshadowed (`rt_unshadowed`),
  both in `06091b66`. OBSERVED by the review: `DDirLightShadowed1` reads only `cb8[29]` (+0x1D0, FC_ShadowColor) of b8
  (`evidence/I-0125/artifacts/dxbc/GXLightAcc_DDirLightShadowed1_ppo.asm`), so the cascade words no longer written change nothing read;
  no menu view collects local lights (no change there). Lows: the mask's dead rotation inputs and the stale comments removed in the
  optimisation pass (`181a780d`); left as they are, recorded: the never-taken engine-pass-1 branch of the pass rows
  (`10_options_and_passes.inc:285`), `no_cast_meshes` always 0, ModelDraw's unused `local_rasterizer_set`, the frame plan's test-only
  SSAO resource and env-region pass (`frame_plan.cpp`, checked by `frame_graph_test`), the reference-run observers of the original
  (`cascade-observe`, `shadow-observe`, `caster-place`), bit 25 now equal to bit 44.
- **The second slot** (18:31-18:44, the ASUS, no measure, `Run-R10Slot2.ps1`, `runs-r10a2.log`, build `build-r10a2` = `f144da5a`: R10a
  before its review's answers, R10b with x3/x4; 10 launches, no fault, 0 `graph_barrier_mismatch`, restored by hash; the camera input
  refused in some runs, the window losing the focus to the Claude app: the first shot is taken before any input): with the rays, no
  local light missed and the GI on every lit frame at Firelink's yard (three sizes), the High Wall, Anor Londo's hall and the PT level at
  its most. Mean luma of the first shot (YAVG of 255) against the last R9d runs with the fill shadow (the default since D-0259):
  Firelink 60.84 → 60.90, the High Wall 70.83 → 70.76, the hall 112.89 → 112.84, the PT level at its most (the High Wall) 71.02 → 70.40
  (the PT's noise and the pose): unchanged within the runs' spread (OBSERVED). The fallback (`no-rt-scene`): no fault, no shadow map made,
  Firelink 53.0 and the High Wall 67.1 (the sun unshadowed, the baked ambient). `fgx4`: x4 asked, x2 used on the RTX 4070 Ti Super
  (`generate` 1, `presented` 2). The screen-mode cycle with frame generation did not step again: the runs end at about 2,830 frames, before
  the cycle's first step (frame 3,240).
## Final review (fresh Opus, xhigh, `main...e8e1d373`) and its answers

- No High. M1 DLSS-G's Off before a chain change and at Close took effect only at the next Present (§6.0), which none carried:
  `FrameGenerationOff` now presents the chain's current buffer once when the last Present had it On, then drains (`2bbd8b66`); M2 STATE,
  the index and status.json behind D-0259 and the code: rewritten at this checkpoint. Lows answered in `2bbd8b66`: L1 DLSS-G off below its
  minimum size (no latch), latched only on a non-OK status read twice; L4 the mask without the common images' gate; comments, the
  CONTRACT's exclusion (x3/x4 delivered), R10-PLAN §3, I-0129's stale sentence. Recorded, not changed: L2 the free after a latch could
  come later; L3 the three tags `eOnlyValidNow` (NVIDIA advises `eValidUntilPresent` first: three full-size copies a frame to save, to
  try on the OLED); L5 the 30 skipped walks' rule now only drops R9c's force; L7 the runners still accept removed flags
  (`rt-lights-check`, `no-rt-reflect`, `no-caster-volume`, `rt-fill-shadow`: a run with them tests the default); L8 no unit test of
  `fg-mode.txt`'s parser, no `native_rt_fallback` row; L10 a failed HUD-less target keeps bit 56 for one frame; L11 the OLED's checks.
- Its fixes' step review (fresh Opus, high, `e8e1d373..2bbd8b66` and `181a780d`): no High or Medium. Recorded: L-a the carry presents
  the chain's older buffer (one refresh of an older frame at a resize or mode change with DLSS-G on); L-b a failed carry (an exclusive
  full screen lost, stand-in buffers) unlogged; L-c a « resolution too low » status may count toward the latch; L-d a refused toggle
  retried carries a stale Present each time; L-e Close's drain counted at the resize site; L-f the frame generation's state not reset at
  Start (no reopen path today).
- **The third slot** (19:27-19:31, the ASUS, no measure, `Run-R10Slot3.ps1`, `runs-r10a3.log`, build `build-r10a5` = `2bbd8b66`; 3 launches,
  no fault, 0 `graph_barrier_mismatch`, restored by hash): DLSS-G on the final tree, `presented` 2, status 0, no latch (OBSERVED). The
  screen-mode cycle did not step a third time: Run-R7aGi's `-Stay` did not keep the window (the runs ended at 2,120 to 2,877 frames,
  before frame 3,240). The final merged tree `build-r10a6` (main's tranche 35 in) built with its unit tests.
- **Merged** `6eb9cf4d` (19:47, main's I-0113 tranche 35 merged in before it); the launcher rebuilt from `main`, its unit tests passed;
  start check `start-6eb9cf4d` passed (19:59-20:00, the rays' self-test passed, restored).

## Uncertainty / unresolved questions

- R10b: its value is on the OLED (280 Hz, Independent Flip); the ASUS is composed by Windows, where DLSS-G with V-Sync adds latency
  (ProgrammingGuideDLSS_G §22.6) and 60 Hz gains nothing. No UI alpha (the UI draws into the gamma intermediate): the UI's look in
  generated frames to judge. DLSS-G stays On under the in-game menus drawn over the world (NVIDIA advises Off in full-screen menus);
  the screen-mode changes and resizes with it On not witnessed (the cycle's runs closed before its first step): to witness before
  R12's menu offers it. The final review's recorded Lows (« Final review »).
- R10a: the traced frames' pictures were compared on `f144da5a` (before the face rule's restoration and the programmes' gate); the
  final tree's traced run (`i131-r10a3-m40-gi`) has no fault, not compared by picture.

## Artifacts produced or modified

- `evidence/I-0131/build-r10a6` (the final merged tree; the other builds removed for the disk, their commits named above); the launcher's log `launcher-6eb9cf4d.log`, the start check's `startcheck-6eb9cf4d.log`; `Run-R10Slot2.ps1`, `runs-r10a2.log`, `Run-R10Slot3.ps1`, `runs-r10a3.log`; `evidence/I-0131/artifacts/Run-R10bSlot1.ps1`, `runs-r10b1.log`; the runs
  `evidence/I-0066/artifacts/i131-r10b1-*` and `i131-r10a2-*` (journals and shots, raw frames removed).
