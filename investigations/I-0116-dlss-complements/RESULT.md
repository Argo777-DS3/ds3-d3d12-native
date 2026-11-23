# I-0116 - Les compléments de DLSS: result

## Conclusion

Active (opened 2026-11-01 06:15 by the RT pilot in the I-0114 thread, branch `claude/i0115-dlss-complements`: named before the
number was found taken, I-0115 being the Silver Knight study's; continued 07:20 on `claude/eager-kirch-362820`). Tranche 1, the
effects' jitter: written, three step reviews and the final review answered, witnessed in series 1-3 (0 refused, outside,
disagreeing or unmatched; the bare draws one clip-space programme, E05's radial blur) and series 4 (the final tree, build-8,
on the OLED: the GPU particles' view-projections and the distortions' world-view-projections shifted); merged `05125a3a`,
the launcher rebuilt, start check `start-05125a3a` passed (on the OLED). Tranche 2: the reactive mask I-0111 gave DLSS made no
difference to the presented pictures under the running model (series 5), so it is removed (series 6); the effects' motion is
not written (the guide gives current models no input for them). Tranche 3: nothing needed (read). Complete: merged `07bd231d`, the launcher rebuilt, start check `start-07bd231d` passed; with the merge of
tranches 2 and 3 (see the investigations index); open: moving effects' ghosting under DLSS not witnessed, preset K not
observed (Uncertainty).

## Plan

| Tranche | Content | State |
|---|---|---|
| 1 | The effects' jitter: the projections their programmes rasterise with shifted like the scene camera's while DLSS jitters | merged `05125a3a`, start check passed |
| 2 | The effects' motion: written or left to the reactive mask, by DLSS's guide and a capture | the mask found not read (series 5): removed; the effects' motion not written (the guide gives current models no input for them; their omission is its known weak case) |
| 3 | The YEBIS motion blur's camera velocity un-jittered, only if cheap (YEBIS to be replaced) | not needed (read): the error is of second order and zero with a still camera; left to YEBIS's replacement |

## Tranche 1

- **The question's facts** (static): the FFX programmes rasterise with their captured blocks' matrices, the game's, unjittered
  (I-0111's known limit). `evidence/I-0068/queries/ffx/ffx-program-layouts.tsv` (every stage) and
  `evidence/I-0068/queries/upscaler-ready/projection-vars.tsv`: used 64-byte matrices `g_projectionMatrix` (VSCommonBuffer
  +128, 12 vertex programmes), `g_ds_projectionMatrix` (DSCommonBuffer +128, 23 tessellated domain programmes), `m_gViewProjMatrix`
  (gcbSceneParam +208, used by the GPU particles' 4 geometry programmes and `VS_VFX_OptStandardParticle`; the 13 other programmes
  that declare it, the pixel ones among them, do not use it), `g_worldViewProjectionMatrix` (+192, 2),
  `g_ds_viewProjectionMatrix` (+0, 1), `m_fWorldViewProjection` (+96, 2: the volume passes, skipped natively); the compute sort's
  `m_ProjMatrix` / `m_ViewProjMatrix` and the inverses are not rasterising.
- **The code** (`21_effects.inc`: `EffectProjectionSites`, `EffectFrameBuilder::Finish`; `upscaler.h`: `ShiftProjectionWords`):
  each draw's programmes reflected once (by registry generation and stage, under a lock) for those matrices in a cbuffer bound
  at a slot the effect context gives (VS b0, DS b0, GS b9, PS b0, b1, b6, b9, b11); while the scene camera is jittered (DLSS),
  each site's block in the frame's arena copy takes the camera's NDC shift (clip.xy += ndc · clip.w), once a block. A pure
  projection's layout is told by its w vector (0, 0, ±1, 0); a world-to-clip one's first by its declared class (column_major:
  transposed), since the second step review against the scene camera (`upscaler::WorldClipLayout`, below). Nothing changes
  without the jitter. Unit tests (`upscaler_test`): the shifted FC_MatrixProj equals
  `ProduceSceneCamera`'s jittered words; the row-by-row layout gives the transpose; an orthographic block is refused untouched;
  a turned, moved view-projection is not told by the projection's pattern and, by its declared layout, equals V times the
  jittered projection within 1e-5.
- **Step review** (`0e646297..cc481eec`): H1 the tessellated draws missed (`g_ds_projectionMatrix`), H2 the GPU particles'
  view-projections refused on every frame (the pattern test only fits a pure projection), M1 a family could drop out unseen,
  L1 repeated per-frame reflection lookups, L2 compute statements reading a shifted block (harmless by layout today), L3 the
  counters' thread. Answered (`8b8baed3`): H1-H2 (every stage's rasterising names, the declared layout for world-to-clip
  matrices, the view-projection test), M1 (`bare`: draws with no site counted, their vertex programme named once by
  `native_ffx_jitter_bare` in build-4, not yet run); L1-L3 recorded.
- **Series 1** (2026-11-01 06:58-07:04, the ASUS, no measure; build-3 = `8b8baed3` with `main` merged; `series-1.log`): the
  jumps path at 60 with DLAA (`i116-jit-jump-n60`): over 3,600 frames 208,523 projections shifted (about 58 a frame), 0 refused,
  0 outside their block, 0 disagreeing with their class, 50 draws without a site. The Firelink run (`i116-jit-fl-n60`) ran
  without DLSS (`native_streamline`: « sl.interposer.dll absent beside the module »: the build folder's copy not yet complete
  when it started), so no row. Not yet seen on a picture against the original.
- **Series 2** (2026-11-01 07:36-07:41, the ASUS, no measure; build-4 = `203e33c7`; `series-2.log`): the Firelink run with DLAA
  (`i116-bare-fl-n60`, `native_streamline` ready): over 10,800 frames 917,331 projections shifted, 0 refused, 0 outside, 0
  disagreeing, 184 draws without a site, all one vertex programme (`native_ffx_jitter_bare`: pass 45, 3,860 bytes, used
  `g_nearFar`, `g_vs_fade*`, `g_vs_softSpriteParam`): `GXFfxSoftRadialBlur`, E05 (the only programme with exactly that set,
  `ffx-program-layouts.tsv`), no matrix, its position already in clip space (EFFECTS-E05-E04-NATIVE: INFERRED); a screen-space
  blur of the jittered scene copy, left unshifted (its quad's sub-pixel place does not show through a blur; INFERRED). The jumps
  run (`i116-bare-jump-n60`) stopped at its launch check (no presenter rate read after the settle), its one bare row the same.
- **Second step review** (`8b8baed3` and `203e33c7`'s row): no High. M-a: the world-to-clip blocks (the GPU particles'
  `m_gViewProjMatrix`) shifted by their declared class alone, which the pure projections' agreement cannot check (no pure
  projection in those programmes) and whose wrong guess would move every GPU particle by far more than a pixel. M-b: a draw
  counted sited by a site in any stage (its pixel programme's included), the bare row naming the vertex programme only. M-c:
  the tests checked the arithmetic, not the step's claims (once a block, outside, untouched without the jitter). L-a the rows
  always on (kept: capped production rows, as `native_dlss_frames`), L-b `sprintf_s` aborting on a long row, L-c
  `VS_VFX_OptStandardParticle` at VS b9 not mapped (not drawn natively; recorded). Answered (`573c2c94`, build-5: unit tests
  pass, `upscaler` 37 checks): `upscaler::WorldClipLayout` tells a world-to-clip block's layout against the scene camera's own
  unjittered view-projection (a view-projection by the camera's w column, a world-view-projection by M·VP⁻¹ being affine; a
  block in neither layout or both is left and counted `unmatched`, so is any world-to-clip block without the camera's
  matrices); a draw is sited by its last geometry stage (GS, else DS, else VS), that programme named; the site shift is the
  pure `upscaler::ShiftSite`, tested on an arena (a block two draws share shifted once and equal to the producer's jittered
  words, a site beyond its block or the arena refused, a world-to-clip block without the camera left, the declared class
  counted against the camera's layout, another view's projection and an orthographic block refused); the bare row a string.
  Untouched without the jitter: `Finish` copies the arena only, as before (by reading; the test cannot reach the builder).
- **Series 3** (2026-11-01 07:58-08:04, the ASUS, no measure; build-5 = `573c2c94`; `series-3.log`, no fault, restored by hash):
  the jumps path (`i116-b5-jump-n60`) 209,546 projections shifted over 3,600 frames, the Firelink path (`i116-b5-fl-n60`)
  962,969 over 10,800; both 0 refused, 0 outside, 0 disagreeing, 0 unmatched; the bare draws (49 and 190) the same
  `GXFfxSoftRadialBlur` vertex programme, now named as the last geometry stage. The GPU particles drew in that run
  (`native_effect_stage`'s `gpu` counts) and their `m_gViewProjMatrix` blocks are world-to-clip sites: none unmatched and none
  disagreeing, so their declared class was the scene camera's layout (OBSERVED; the counters are not split by kind). Not
  yet seen on a picture against the original.
- **Optimisation pass** (read): the reflection once a programme and stage (cached), the camera's inverse once a frame, one
  layout test and at most one shift a block; the view-projection is built again from the camera for this test beside the
  velocity collect's (a 4x4 product, kept: the two run at different points of the frame). Nothing removed.
- **Third step review** (`573c2c94`): no High. M1 the world test's comment claimed more than it checks (another view with a
  perspective lens passes); M2 the counters not split by kind, so series 3 could not show the world-to-clip path ran; L1 the
  tolerance's scale, L2 `outside` counted per draw, L3 two untested branches, L4 the view-projection built again.
- **Final review** (fresh, xhigh, `main...6dd029c6`): no High. M1 the RESULT's conclusion and plan row and STATE stale; L1 the
  refused, outside and unmatched sites not named; L2 a block already jittered shifted again (not possible today: the effect
  system's camera never carries the jitter); L3 the sites copied under a lock up to four times a draw, the camera's matrix built
  without the jitter; L4 facts no longer true (`m_gViewProjMatrix` used by four geometry programmes and
  `VS_VFX_OptStandardParticle`, not 12 pixel ones); L5 E03 left unread.
- **Answered** (`a4322b6a`, `6f2e21b7`, build-8: unit tests pass, `upscaler` 44 checks): a view-projection told by Q = VP⁻¹·M's
  x, y and w columns being the identity's (another near or far plane allowed, another view or field of view refused; a jitter
  of the game's own is under the tolerance far from the origin, not told); a world-view-projection by its perspective w (its z
  column a times its w column over the object's axes, only the right layout shows it; its view not checked); a pure projection
  already off-centre left and counted unmatched; the counters by kind and each once a block; the first view-projection and
  world-view-projection told, and the first four programmes with a site left, named (`native_ffx_jitter_world`,
  `native_ffx_jitter_left`); the sites by reference; the camera's matrix only under the jitter; E03 recorded (Uncertainty);
  the RESULT's facts corrected. A fourth step review (`6dd029c6..a4322b6a`, float32 simulations of 6,000 cameras and world
  transforms up to 6 km: no wrong layout accepted): no High; M1 the view-projection test's tolerance under a camera one tick
  away (read in series 4: none unmatched), L1 a camera-attached block on the view axis held by both layouts and left (counted
  and named), L2-L4 two tests and two comments (`6f2e21b7`).
- **Series 4** (2026-11-01 08:45-08:49, the OLED at 280 Hz, Thomas's choice, no measure; build-8 = `6f2e21b7`'s code;
  `series-4.log`, no fault, restored by hash): the Firelink path with DLAA (`i116-b8-fl-oled`), over 10,800 frames 683,836 pure
  projections, 171,473 view-projections (the GPU particles' `m_gViewProjMatrix`, pass 41, first told in layout 1 as declared)
  and 74,686 world-view-projections (the distortions' `g_worldViewProjectionMatrix`, pass 45, layout 1) shifted; 0 refused,
  outside or disagreeing; 96 unmatched, all `GXFfxdistortion`'s (`native_ffx_jitter_left`: 0.13 % of its blocks, left
  unjittered; INFERRED the both-layouts case of a block on the view axis); the bare draws the same E05 radial blur. OBSERVED:
  the view-projection test held through the path's turns (0 unmatched of that kind).

## Tranche 2

- **DLSS's guide** (the SDK's `DLSS Programming Guide.pdf`, 310.9, March 2026, read as text): §3.15 « Biasing the Current
  Frame », the buffer behind Streamline's `kBufferTypeBiasCurrentColorHint` (I-0111 tranche 4's reactive mask): « has not been
  introduced for use with latest models and should not be used », only preset F reading it; §3.12 preset F « will be deprecated
  in the next SDK cycle and should not be used », K the default for DLAA, Quality and Balanced, L for Ultra Performance, M for
  Performance. The guide's only other mentions of particles: a known weak case of DLSS's feature tracking (small particles,
  « objects with missing motion vectors (many particle effects) »), whose remedy was that bias buffer; a transparency layer
  (`kBufferTypeTransparencyLayer`) exists for Ray Reconstruction only (Streamline's `ProgrammingGuideDLSS_RR.md`).
- **The model running** (`evidence/I-0116/build-5/sl.log`, `build-9/sl.log`): the native asks NVIDIA's default preset
  (`streamline_host.cpp`, Thomas 2026-10-30); NGX loads DLSS 310.9.16 from `C:/ProgramData/NVIDIA/NGX/models` and reports « DRS
  Overridden Preset L » for every mode: Thomas's NVIDIA App override (Thomas, 2026-11-01 08:27: « j'ai forcé le preset L »).
- **Series 5** (2026-11-01 09:19-09:27, the OLED, no measure; build-9 = `3cf2f38e`, `reactive-full.flag` a diagnostic that clears
  the mask to 1 on every DLSS frame; `series-5.log`, restored by hash): the Firelink path with `world-pictures` twice,
  `i116-rm-normal` (the mask as I-0111 draws it: 0.7 % of the pixels above 0 at frame 1,500) and `i116-rm-full` (the mask 255 on
  every pixel, read back). A model reading the bias would take the current, jittered colour everywhere and lose its temporal
  smoothing. The presented pictures' Laplacian energy (`Get-I116Sharpness.py`): frames 1,800 to 3,300 within 0.3 % (ratios
  0.999-1.003), frame 1,500 5 % lower with the full mask (the view not the same moment); a crop of frame 2,400 side by side shows
  the same edges; the pictures' per-pixel luma difference at frames 1,800-3,300 1.2-1.3 / 255 on average, 0.7-0.9 % of the pixels
  above 16 (the final review's recomputation). OBSERVED: the mask tagged through Streamline made no difference to the presented
  pictures under preset L; INFERRED with the guide: the model does not read it, nor would K (not observed: every run here had
  Thomas's override). No noise floor was taken (two runs of one configuration); `world-N` is the frame serial and the two runs'
  first DLSS frames were serials 1,273 and 1,422, which explains frame 1,500.
- **Decision** (the pilot's, reversible): the mask is removed (WORKFLOW « Optimise, do not stack »): its scene copy, its
  full-screen pass, its target, its plan bit and its tag, and the forward rows' reordering it needed on DLSS frames (the opaque
  depth writers moved ahead of the rows that write no depth: the original's order back). The effects keep the motion of what is
  behind them (no motion vectors of their own: a blended effect's vector would replace the background's seen through it). The
  guide gives current models no input for them: their omission is its known weak case (§3.15, « objects with missing motion
  vectors (many particle effects) » may be erased or trail), its only remedy the bias they no longer read; accepted until Ray
  Reconstruction's transparency layer (R2, R5). Not witnessed: a fast effect's ghosting under DLSS (Uncertainty).

- **Step review** (`main...f03b7343`, the net removal): no High, no Medium; nothing of the mask left (no member, declaration,
  plan bit 42, tag, counter, witness file); Streamline's tags and inputs right (the exposure at index 4, six inputs at most); the
  forward block and the water path line for line those before the mask (`252529d0`). L: the `encode_one` helper the mask added
  is kept as `encode_forward`'s loop (the same encoding, one bind state a call).
- **Series 6** (2026-11-01 09:47-09:51, the OLED, no measure; build-10 = `f03b7343`'s code; `series-6.log`, restored by hash):
  the Firelink path with `world-pictures` (`i116-nomask-fl`): no fault, DLSS evaluated on 10,800 frames, no mask row or file;
  the presented pictures' Laplacian energy against `i116-rm-normal` (with the mask) within 0.3 % at frames 1,800-3,300 (0.6 %
  lower at 1,500, the view not the same moment).

## Tranche 3

- **Read** (`13b_prepare_model_frame.inc`, the motion blur's inputs): the YEBIS motion blur's camera velocity is rebuilt from the
  depth with the camera's unjittered lens (`tan_half`, `aspect`) and the current and previous poses. Under DLSS the depth at a
  pixel is the jittered image's, the surface at the pixel's unjittered place minus the jitter, so the point rebuilt is at most
  half a render pixel beside the right one. With a still camera both poses are equal and the velocity is zero whatever the point
  (no error); with a moving camera the error is the velocity's change over half a pixel, of second order on a continuous surface
  (at a silhouette the jittered depth sample may cross the edge and take the other surface's parallax, so the blur length may
  shimmer there while strafing, in a sub-texel band of the half-size velocity; at DLAA the blur reads the jittered depth itself,
  in the scaled modes `depth_out` picks the texel under the unjittered point) (INFERRED from the
  reading, not measured). The object velocity it reads at half size comes from the velocity target, unjittered since I-0111.
- **Decision**: nothing written; the YEBIS passes are to be replaced (Thomas, 2026-11-01 03:02: « On ne gardera pas YEBIS »), and
  their replacement takes DLSS's own unjittered vectors.

## Evidence

- `evidence/I-0116/build-1` to `build-10` (build-9 the diagnostic, build-10 the removal's tree: unit tests pass), `series-1.log`
  to `series-6.log`, `start-check-05125a3a.log`; the runs `evidence/I-0066/artifacts/i116-jit-*`,
  `i116-bare-*`, `i116-b5-*`, `i116-b8-fl-oled`, `i116-rm-normal`, `i116-rm-full`, `start-05125a3a`, `i116-nomask-fl`; the DLSS guide's text read from `C:/Users/User/Tools/DS3-Decompilation/third-party/streamline-2.14.16/docs/DLSS Programming Guide.pdf`.

## Uncertainty / unresolved questions

- The draws without a projection site are `GXFfxSoftRadialBlur` (E05, clip space): left unshifted (INFERRED harmless).
- A world-view-projection's view is not checked (a perspective lens of another view would pass); none is drawn by an
  effect but the distortions (`GXFfxdistortion`, its soft form: `g_worldViewProjectionMatrix`), INFERRED the scene camera's;
  0.13 % of their blocks left unmatched at Firelink (series 4), the cause not read (the both-layouts case INFERRED).
- E03 model particles are admitted as ordinary scene models (`AppendNativeEffectCandidates`, `12_scene_collect.inc`), so they draw
  with the scene camera's jittered constants (INFERRED from reading, the final review's L5).
- No picture yet: a thin effect (the Estus flame, Lightning Spear) at DLAA against the original, nor a fast effect's ghosting
  under DLSS without motion vectors of its own (the guide's known weak case); a Lightning Spear picture at DLAA against
  `no-dlss` can follow (not a gate, D-0227).
- The mask's removal rests on preset L (Thomas's override) and the guide for K; a user forcing the deprecated preset F would lose
  a mask F reads.
