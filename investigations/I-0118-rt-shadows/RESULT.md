# I-0118 - DS3 DX12 Vague 2: R5 of the RT/PT plan, the traced shadows (Les ombres tracées): result

## Conclusion

Complete (closed 2026-11-09 with Thomas's yes, « Closed » below); its history: Active (opened 2026-11-02 by the RT pilot on branch `claude/zen-benz-ce4cac`, with I-0117 T5b). R5a written, seen live, step and
final reviews answered, merged with T5b (`a8b03f3a`, the launcher rebuilt, start check `start-a8b03f3a` passed): the sun's shadow traced in place of the cascades' mask layer, behind `rt-shadow.flag`;
its picture shown to Thomas (2026-11-02 11:50, the High Wall); its own gate (not the raster layer's), merged `c04f4ad7`. Thomas
accepted the recommendation to remove the cascades (2026-11-02, about 16:00 Paris, relayed), on the condition that the orientation
check of his question on the traced mask's incomplete outlines come first; the check ran at 16:21 and was posted to him at 16:29
(« The orientation witness »: at the view he asked about, two thirds of those pixels face away from the sun, the rest mostly the
raster's soft grey); the traced sun shadow is then the default with the rays and the cascades' casters, atlas and blur taps are no
longer made on its frames (D-0237's first raster removal; « The cascades removed »), merged `cba61237` (the launcher rebuilt,
start check `start-cba61237` passed). R5c, the translucent casters (« R5c »): a transparent slice (class 2) casts in the rays as the
original's cascade row draws it (the hair, FaceGen's eyelashes), reviewed, seen at the High Wall and merged. Open: R5b, the local
lights' shadows, with R4; `m37_00_00_00_001005` under R5c's rule.

## R5a, the sun's shadow traced (2026-11-02)

- **What is traced**: the sun's shadow term only. The original's mask layer (SunShadowStage: GXScreenSpaceCascadedShadow_Soft
  from the cascade atlas, then its blur passes) writes the shadow term in the shadow and AO mask's w, which the composite and the
  shadowed sun pass read; with `rt-shadow.flag` (and `rt-scene.flag`; since « The cascades removed », with `rt-scene.flag` alone) the frame draws instead two full-screen triangles
  (`RtSunShadow`, `rt_shadow.hlsl`, DXIL from dxc at build time): OutputColor's value (0, 0, 1, 1) where the G-buffer stencil bit
  0x08 is clear, and on the G-buffer pixels one ray toward the sun through the frame's top level with the sun casters' mask
  (`RtMaskSun`) and the alpha-tested slices' test (`rt_alpha.hlsli`, the slices' ray bits): w 0 when it meets a caster, else 1;
  xyz (0, 0, 1) (the blur that reads them is not run). The surface is rebuilt from the depth as the control view does (the camera,
  the jitter undone), pulled back toward the eye by 2 cm + 0.2 % of the distance; the ray starts 1 cm on (`RtSunConstants::offset`).
  The sun: the shadow frame's `sun` (the LightSet's light 0 the cascade fit takes; toward the sun: y +0.74 at Firelink). Sharp,
  one ray a pixel, no denoiser (D-0237). The cascades are still drawn (their casters and atlas), no longer read: removed once Thomas
  accepts the traced sun (D-0237). The plan key's bit 44 (the blur passes' bits then 0); the graph's pass `rt_sun_shadow`. A
  failure turns it off for the session (the layer again); no top level this frame, or a camera or sun the frame cannot give: the
  mask cleared lit (white, its optimised clear value) for that frame (the step and final reviews' answers below).
- **Unit test** (`rt_scene_test.cpp`, `SunShadow`): a 4x4 view whose raster surface (view z 3, world z 1) lies behind the test
  quad (z 0), the sun toward (0.5, 0, -1): the columns behind the quad 0, the others 1, the row without the stencil bit and the
  pixel without a surface 1, xyz (0, 0, 1) everywhere (`0011 0011 0011 1111`, build-39 and build-40); a zero or not finite sun
  refused. The test's own clears' performance notes are kept out of the debug layer's store for that test.
- **Runs** (build-40, `Run-I117T0.ps1 -Modes on`, the ASUS `\\.\DISPLAY1` alone (the OLED off since 10:43), offline, restored by
  hash, 11:36-11:42; a first series at 11:17 stayed at the title on the vanished `\\.\DISPLAY2`): `i118-r5b-raster-m40/m30` with
  `rt-scene.flag`, `i118-r5b-traced-m40/m30` with `rt-shadow.flag` too. The graph ran `rt_sun_shadow` in 2,397 frames of 4,801
  (the casters' frames) and no `sun_shadow` or blur. Sheets (`evidence/I-0118/artifacts/r5b-*-sheet-*.png`: the presented picture
  above, the mask's w below, raster left, traced right, from the runs' `world-N.rgba` and `world-N-mask.rgba`):
  - **The High Wall**: the traced term follows the raster's shapes (the character, the statues, the ground's shadows), sharp; the
    presented pictures differ little (the eclipse's sun weighs little against the ambient; the pixels facing away from the sun are
    dark by the light's own angle in both). Differences seen: noise on the grass at the right (the rays' grass in its rest pose:
    R2-PLAN T2's wind), a pillar's shadow at the right edge the raster's cascades leave out, the hair casting nothing (forward
    slices: `uncast`), the statues lit in streaks where the raster's are black (not explained: their skinned rest pose or the
    back faces against the cascades' bias, to read), the raster's soft grey patch of a transparent caster absent (the rays leave the
    transparent class out, T5 L1's policy).
  - **Firelink** (the bonfire): no sun shadow in either mask at this place.
- **The picture for Thomas**: `r5b-m30-sheet-2400.png`, posted in the thread at 11:50 with what is traced and what is not.
- **Not measured**: the pass's GPU time (the PIX run had no `rt-shadow.flag`; next capture), the frame's.
- **Step review** (fresh Opus, high, `b6705477..30a33cd6`): nothing high. Answered (build-41): M1 no top level this frame (the
  first frames after a load, a failed build) bound a null address as the structure's root view, which DXR documents for a heap
  descriptor, not a root one (now the frame clears the mask to OutputColor's value and draws nothing); M2 the unit test could not see
  a missing stencil test or the sun's mask (now row 3 is a surface without the bit, which a trace would shadow, and a second quad
  seen by the camera alone lies on the lit columns' rays); M3 a failure while making the depth's slot released the pipelines
  under the frames in flight (now the slot alone; the pipelines go at the renderer's close, after the drain); L1 a failure turned
  the pass off without a row (now `native_rt_shadow` with `on` and `why` at every 600th walk); L2 a camera or sun a frame cannot give
  turned it off for the session (now that frame only, no shadow); L3 the pipelines through the device's pipeline cache, made once
  for the AO mask's format (another refused). L4, the traced pass running only where the raster's mask layer would: answered
  later by its own gate (« Its own gate » below); the casters and atlas still drawn and unread: their removal waits for Thomas's
  acceptance (D-0237).

- **Final review** (fresh Opus, xhigh, `main...a028491d`, with T5b): nothing high. Answered (build-43): M1 the frame's lit clear
  used (0, 0, 1, 1) against the mask's optimised white, a debug-layer warning that closes a run (build-41's path, never run live):
  now white (w 1 either way, the composite rewriting RGB); L4 the alpha entry made this walk by one chunk kept (another chunk's
  equal work dropped); L5 every kept chunk cleared at the walk's start; L6 the queue drained before the rays' objects close after a
  fault; L7 the bindless heap checked; L1-L3 these records (the « no top level » text, I-0117's conclusion and T1's CPU target at
  m45, the walk's equality stated at the level of totals). Kept: L8 the unit test does not run the alpha branch nor tell the pixel
  without a surface apart, and filters the clear-value warnings of its own clears; L9 the casters and atlas still drawn (D-0237's
  removal); L10 the fault under PIX (I-0117 RESULT « T5b »), open. Its fixes' step review (fresh Opus, high, `a028491d..3110b664`,
  build-43 every unit test passed): nothing high or medium; carried into the next tranche: L1 the first alpha set of a walk kept even
  when it is the deferred or withdrawn one (keep it only when complete), L2 `Close` drains twice after a fault with Streamline
  running (one extra deadline on a hung GPU), L3 the kept chunks cleared at the next walk only (a walk that threw holds its
  structures until then: a scope guard). Optimisation pass (read): one pass a frame (two draws), nothing per frame on the CPU but
  the constants; the casters and atlas still drawn (L9).

- **The pass's GPU time** (PIX, `i118-r5c-pix-m30` with `rt-shadow.flag`, build-44, 13:26-13:27, the capture at frame 3060 replayed
  by pixtool, `evidence/I-0118/artifacts/pix-r5c-3060-ps.csv`; a replay's times, not the live frame's): the fill 37 µs (237,208
  pixels off the G-buffer) and the traced draw 143 µs (684,392 G-buffer pixels, one ray each through the High Wall's 666 static and
  257 skinned instances) at 1280x720; the frame's 14 structure builds 1.13 ms (the top level 0.29 or 0.17 ms). The cascades' casters
  and atlas, still drawn, not counted here. As in I-0117's PIX run the game closed after the capture on the debug layer's
  `EmitRaytracingAccelerationStructurePostbuildInfo` message (twice now, only under PIX: LIVE-RUN-NOTES).
- **The statues' streaks** (the High Wall): their streaked surfaces face away from the sun (the sun ahead of the camera there);
  such a surface is dark by the light's own angle whatever the mask, which is why the presented pictures agree; the raster's
  cascades blacken it whole (its depth map holds the sun-facing side, blurred), the rays only where the statue's thin limbs lie on
  the ray. INFERRED from the view and the pictures, not a defect of the rays.

- **Its own gate** (`c3efd899`, `7df5449a`, build-50; the final review's L4): the traced pass runs on the frames with the mask
  stage, the shadowed sun pass's pipeline and the frame's shadow blocks written (`shadow_blocks`, set where `shadow_scene` and
  `rt_sun` are), no longer only where the raster layer could (its casters, the Soft programme, the blur taps): the PCF4/PCF9
  records and a frame without casters get the traced shadow too (by the code; not run live: build-50's unit tests only, and both
  witness zones' records are Soft: a PCF4/PCF9 place for the next run). The raster stage's second target is declared only when the raster
  layer runs. Step review (fresh Opus, high, `478fdb4e..c3efd899`): nothing high or medium; answered L1 (the gate on the blocks
  written, not on the shadow frame's existence: a failed inverse left them zero) and L3 (comments); L2, the casters drawn and unread
  on traced frames, is the removal that waits for Thomas's judgement (D-0237). Final review (fresh Opus, xhigh, `main...1bf9709b`):
  nothing blocking; answered M1 (these records, STATE, I-0117's L5), L2 (comments), L3 (« not run live »); kept L1 (`Close` after a
  hung GPU: an extra deadline, and the rays' objects closed after failed drains, older than this branch: quarantine them then).

## The orientation witness (Thomas's question on the outlines, 2026-11-02)

- **The question**: on `r5b-m30-sheet-2400.png` the traced mask's outlines of the character and the statues are incomplete where
  the raster's are whole (the lower half most). The hypothesis (« The statues' streaks » above): those pixels face away from the sun.
- **The tool** (`0389814e`, `rt-shadow-witness.flag`, a test tool): the traced pass writes 0.5 in the mask's w where the surface
  faces away from the sun, its normal from the rebuilt positions' screen derivatives (noisy on grass and rubble, unreliable on a
  silhouette's pixels), the ray's result elsewhere.
- **Run `i118-r5d-witness-m30-on`** (build-51, the ASUS `\\.\DISPLAY1` alone, 16:21-16:23, offline, restored by hash; Theta at the
  High Wall, `rt-scene`, `rt-shadow`, the witness; 4,211 frames, no fault; `native_rt_shadow` on with `witness` true).
- **Observed** (the raster run `i118-r5b-raster-m30-on` and the traced run `i118-r5b-traced-m30-on` against the witness run, frame by
  frame, the same walk): of the pixels the raster mask shadows (w < 0.5) and the traced one leaves lit, 66.3 % at frame 1800 and
  65.6 % at 2400 are marked facing away (70.0 and 69.2 % in the lower half); 28.7 % at 3000. The runs drew the same view at 1800
  and 2400 (1.1 and 1.0 % of the presented pixels differ by more than 20 a channel between the raster and the witness run) and a
  slightly different one at 3000 (4.9 %). The map `r5d-m30-missing-2400.png` (red: facing away, blue: the rest) puts the red on the
  character's and the statues' flanks turned from the sun (it is ahead of the camera, the eclipse) and the blue in a rim of one or
  two pixels outside the silhouettes; the raster's w on the blue pixels is soft grey, not a hard shadow (mean 0.35, 6,722 of 8,339
  between 0.25 and 0.5, 42 at 0). At 3000 (`r5d-m30-missing-3000.png`), the character turned side-on: the raster's mask greys his
  whole body and the chair at the right (mean w 0.40 on the 26,425 blue pixels, 23,981 between 0.25 and 0.5) where the rays light
  their sun-facing sides; his shadow on the ground is in both masks.
- **Conclusion**: at the view Thomas asked about (2400), OBSERVED: two thirds of the missing outline pixels face away from the sun
  (dark by the light's own angle whatever the mask; the presented pictures agree), with the caveat that the tool's normal is
  unreliable on a silhouette's own pixels; the rest is the raster's soft grey around the silhouettes, the blur spilling over where
  the traced term is sharp (INFERRED). At 3000, the raster greys sun-facing surfaces the rays leave lit: the cascades shading their
  own casters through their resolution and blur (INFERRED; not a caster missing from the rays, the character's ground shadow being
  traced). Posted to Thomas with the 2400 map and the witness sheet `r5d-m30-witness-sheet-2400.png` (16:29); the 3000 reading with
  the merge.

## The cascades removed on the traced frames (D-0237, 2026-11-02)

- **What changes**: the rays on (`rt-scene.flag`) now trace the sun's shadow by default; no flag keeps the cascades beside them
  (D-0237: nothing kept hidden), so `rt-shadow.flag` is gone (`rt-shadow-witness.flag` stays the test tool). The collect decides
  once per main view (`NativeSelectedScene::sun_traced`: the rays on, `rt_shadow` on, the rays not failed, fewer than 30 rays'
  walks skipped in a row), after the rays' walk, which now runs before the shadow walks (it reads none of this collect's admitted
  models; a programme set only a shadow admission builds is seen one walk later), and then skips the cascades' caster walk
  (`CollectNativeShadowCasters`); the prepare then makes no cascade:
  no atlas edge (the atlas, 8192² D16 at the High Wall, retired), no cascade viewports or view blocks, no blur taps (no Soft
  programme, no samplers); the blocks, the shadow scene and the sun stay (the traced pass's gate and the shadowed sun pass read
  them). The renderer's existing gates follow: no casters, so no `shadow_casters` pass, no `shadow_atlas` declaration, the plan key's
  bit 23 and the blur bits 0. The local faces' casters are unchanged (R5b).
- **Without the rays** (no `rt-scene.flag`, no DXR, a failed self-test) or after a failure that turns the rays or the traced pass
  off for the session, the next collect walks the cascades again: the raster layer as before (D-0237: never left without shadows);
  so do 30 rays' walks in a row that skipped or threw (half a second at 60), until a walk makes instances again. Left lit (the
  mask cleared white, as a frame without a top level): a frame whose walk skipped or threw (fewer than 30 in a row), a frame
  without a top level at its record (a failed build), and the one or two frames already collected when a failure latches on the
  render thread. How often a walk skips in play is not measured yet (`native_rt_walk` rows: the first three walks, every 600th,
  the first eight skips and every 600th skip; since build-57 each row counts the skipped walks since the last row and the longest run reaching into it, from its start;
  a lit frame is a skipped walk while the traced pass is on and fewer than 30 have skipped in a row).
- **Step review** (fresh Opus, high, `4d934cb3..17eff9c7`): nothing high. Answered (build-54): M1 the collect decided before the
  rays' walk and without its result, so a walk that skipped or threw left a frame with neither the traced pass nor the cascades
  (now the rays' walk first, `sun_traced` on its instances); L1 the records naming `rt-shadow.flag` as the switch (the investigations
  index, R5a's paragraph; the CONTRACT keeps its plan's wording). Kept: L2 the cascade fit's four caster volumes still made on traced
  frames (four engine calls a collect; the fit's matrices and words are still needed, and the decision comes after the fit).
  With them, I-0117 T5c's carried L2: every map of the rays' model keys on the mixed hash (L1, the alpha shards a cache line each,
  kept: `alignas` pads and warns, C4324, and a shard spans more than two lines).
- **Final review** (fresh Opus, xhigh, `main...72f9bf0c`, before the step review's fixes): nothing high. Answered (build-55): M1 in a
  traced session the sun atlas (8192² D16, 128 MiB at the High Wall) is first made at the collect after the rays fail, when memory
  may be short, and its refusal faulted the renderer (now `shadow_atlas_failed_`: the refusal leaves the sun unshadowed, the lit
  mask, for the session, with a `native_shadow_atlas_failed` row); M2 these records (the time and condition of Thomas's yes, frame
  3000's share and the silhouette caveat, the rim INFERRED); L1 the prepare's redundant `else` and its block's indentation; L2 the
  witness's derivatives computed on every traced pixel (now only with its flag); L4 the step review's M1; L5 the stale comments
  (`model_frame.h`) and records (the index, `status.json`); L6 the merged tree built (build-55, every unit test). Kept: L3's cascade
  fit volumes and `shadow_per_scene` still made on traced frames (microseconds; the decision comes after the fit), the pop-in
  witness's shadow rows now skipped on traced frames; M1's optional release of the rays' structures after their failure (older
  than this branch: I-0117 « Close »).
- **Its fixes' step review** (fresh Opus, high, `72f9bf0c..3b88186e`): nothing high. Answered (build-56): M1 a sun atlas half made
  (the resource committed, its view's heap refused) stayed without a view, and the next frame asking no atlas would clear it
  through a null heap (now the atlas is kept only whole); M2 the collect deciding on each walk's instances made and retired the
  128 MiB atlas for every skipped walk in a traced session (now a skipped walk leaves its frame lit and only 30 in a row bring the
  cascades back); L1 the walk's independence worded exactly; L2 STATE's sentence; L3 `status.json`'s time and build-54 in the
  evidence. Their step review (fresh Opus, high, `3b88186e..90e20a2c`): nothing high; the counter single-threaded (one main-view
  collect a frame, on the engine's thread), no stale top level on a skipped frame. Answered (build-57): M1 the skips not counted,
  so a sporadic skip (a lit frame each time, a strobe) could not be seen (now `native_rt_walk` carries `skips` and `skip_run_max`);
  L2 the sampling's wording. The counters' own step review (`90e20a2c..51dd5d87`): M1 the new statics added to the thread-affinity
  allow-list (`Test-Project` failed without them); L1, L2 their meaning worded exactly (skipped walks, not lit frames; the run
  counted from its start). Kept: M1's reuse of the last walk's instances for a skipped frame (the skinned
  instances are keyed by walk: to weigh once the skips are counted in play); L1 no hysteresis (30 skips, one good walk, 30 skips
  would make and retire the atlas every 31 frames: not seen); L3 the static outside the thread-affinity check's scope, as the
  function's other statics.
- **Diagnostic run `i118-r5e-default-m30-on`** (build-56, the ASUS alone, 17:41-17:43, offline, restored by hash; Theta at the High
  Wall with `rt-scene.flag` alone, not a merge gate; a first try at 17:30 stayed at the title, the screen's name mangled by the
  shell, its folder deleted): 4,241 frames, no fault; `native_rt_shadow` on without its flag; every `native_shadow_draws` row
  `edge` 0 and `per_cascade` [0,0,0,0] (27 draws, the local lights' casters), no `native_shadow_casters` row (the walk skipped), no
  atlas picture, no `native_shadow_atlas_failed` row, every sampled rays' walk made (`skipped` empty); the mask the same traced
  term as R5a's (`r5e-m30-default-sheet-{2400,3000}.png`: the raster run's picture and mask left, this run's right).
- **PCF4/PCF9**: no world place has them. A sweep of every run since 2026-10-01 (1,882 runs with `native_shadow_state` rows past
  their third call, Sonnet, read-only): every world row has FilterMode 2 (Soft) at m30, m31, m32, m33, m35, m37, m38, m40, m45,
  m50, m51; FilterMode 1 (PCF9) only with area 1, the title and menus' view. With the traced default the programme matters only
  when the rays are off; a lower shadow quality setting may select another record (not tried).
- **The rays' walk skipped in motion** (run `i119-walk-skips-m40`, I-0119's build-18, 2026-11-03 01:55-01:57, the ASUS, offline,
  restored by hash, `rt-scene` and `test-no-ai`; Eta walking from Firelink's bonfire into the shrine, forward, right, back, left,
  a camera turn; 5,505 frames, no fault): `native_rt_walk` `skips` 0 and `skip_run_max` 0 at every row to call 3600, the instances
  243 then 532 as the shrine came in (`ms_mean` 0.60 to 0.91 ms). OBSERVED: no lit frame and no return of the cascades on that
  walk; a longer travel (a zone's streaming, a load) not tried.

## R5c, the translucent casters (2026-11-03)

- **The question** (I-0119 RESULT « Suggested next step », R5a's « the hair casting nothing »): the rays' casters took the
  G-buffer classes only (1, 4, 0x100, a surface writing depth), so the hair's shadow, which the raster's cascades drew, was absent
  from the traced term.
- **Read** (the native's cascade row, `model_pass.cpp` and `10_options_and_passes.inc`, from D-0180 C2's reading of 57A20 and
  140680): the cascade passes 1..4 take class mask 0x107 (1, **2**, 4, 0x100), refuse a Decal, a material's no-cast byte and a water
  or refraction material (its policy's flags 0x30: a kind-0x27 draw, the rows 0x24 and 0x27 alone, WT-1), and
  draw Dpt or DptA whatever the MTD's depth write (the native raster's rows; the original's depth state not re-read); the local faces take the same row (I-0106). The rays' `native_rt_forward_only`
  rows of the runs since I-0117 (the High Wall, Firelink, m31, m37, m40, m45, m50; OBSERVED): the hair `HR_A_*`
  (classes 0x82: forward 0x80 slices, row 0x1F, and transparent class-2 slices, row 0x29) and FaceGen's `FG_A_*` (class 2) have
  category 0x17 (the view, the sun's cascades, the local faces, the reflection); every sky or map piece of class 2 or 0x82 has
  category 1 (the view alone: no sun or local bit), but one, `m37_00_00_00_001005` (class 2, category 3, not identified). So, of the
  forward-only instances (those rows name only instances without a surface slice, the first 32 models a run) at those zones, the
  engine's category byte, not the class, keeps the sky out of the casters; a mixed instance's class-2 slices are not covered by
  those rows (the step review's M2: the `translucent` counter below). The « cascade extent » R5a's comment named limits every
  class alike (the rays already cast at any distance).
- **What changes**: `RtSliceCasts` (rt_scene.h) is the cascade row's rule: a surface or a transparent slice (class 2) casts, unless
  a Decal, its no-cast byte or a water or refraction material; a class-2 slice answers the sun's and the local lights' rays besides RtMaskForward's and the
  reflection's (`RtSliceKeep`), never the view's; its instance keeps the engine's sun and local bits (`RtInstanceMask`). Its alpha
  test is the record's (DptA's t0 alpha, as for the G-buffer slices). Kept as explicit policy: a G-buffer slice drawn without a depth
  write (the High Wall's sky, m45's grime cards) does not cast, though the cascades would draw it (T5 final review L1).
- **The witness** (`native_rt_walk`'s `translucent`: the casters, the sun's or the local lights', with a transparent slice that
  casts; with `rt-view.flag` the first 32 such models named once, `native_rt_translucent_caster`: classes, category, whether it
  has surfaces, its mask).
- **Unit test** (`rt_scene_test.cpp`, `SurfaceMasks`): the casting rule's cases (a surface as the caller finds it, a G-buffer class
  writing depth; class 2 without a surface; not 0x80 nor 8 nor 0x10 without a surface; not a G-buffer slice without a depth write;
  not a Decal, a no-cast byte or a water or refraction material); a transparent caster's ray bits; the hair's instance mask (the view's bit forward, the casters' kept); a
  class-2 caster outside the engine's casters (no caster bit added). `Materials` (since the final review): a ray of the sun's mask
  through the probe meets the transparent caster the view's ray passes over.
- **Build** `r5c-build-1` (the first commit, `26a5993b`): every unit test passed (114).
- **Step review** (fresh Opus, high, `2a898eb6..26a5993b`): nothing high; the bits agree (a hair instance mixed, so non-opaque;
  its 0x80 slices passed over by the sun's query, its class-2 ones reached; no view ray reaches a class-2 slice; one class a
  slice). Answered (`e5fba560`, build-2: every unit test passed, 114): M1 the cascade row's water and refraction refusal (WT-1) missing from the rule, so a class-2 water or
  refraction slice of a casting entity would have thrown a hard shadow the original never draws (now refused, with its unit test
  case); M2 the sky claim generalised beyond the forward-only rows (now worded to them; the `translucent` counter and its named
  rows, the first 32 models, count the casters a transparent slice makes, mixed instances included, by the candidates made: a
  duplicate joining its mask is not counted; to read in a run with `rt-view.flag`); L1 to L3 stale comments and a test's label; L4
  the rule's other classes in the test. Kept as explicit policy, L5: the cascade row refuses the phantom and fading categories
  0x21 and 0x29 (`model_pass.cpp`), the rays only by the entity's category byte (a summon's surfaces cast in them when its byte
  has 0x02, not read; its hair then too): with R5b.
- **Its step review** (fresh Opus, high, `26a5993b..e5fba560`): nothing high or medium; the water refusal the raster's, the counter
  gated by `made` in every path (static, wind, skinned firsts and their replayed duplicates), the formats' 33 specifiers and
  arguments, the initialisers. Answered: L1 the new statics passed the thread-affinity check under the forward-only diagnostic's
  names (renamed, their own reviewed entries); L2 the counter's scope (the candidates made) stated; L3 the code's sky comment
  narrowed; L4 the test's label and these lines; L5 these records.
- **Diagnostic run `i118-r5c-hair-m30-on`** (build-2, `Run-I117T0.ps1 -Zone m30 -Modes on -View`, the ASUS `\\.\DISPLAY1` alone,
  11:00-11:02, offline, restored by hash; Theta at the High Wall, `rt-scene` and `rt-view`; 4,496 frames, no fault; not a merge
  gate). OBSERVED: `native_rt_walk` `translucent` 2 at every sampled walk (657 instances, `uncast` 1, `skips` 0); the two named,
  `HR_A_0101` (classes 0x82, category 0x17, no surface, mask 0x1E: forward, sun, local, reflection) and `FG_A_7002` (class 2, the
  same: the eyelashes, which the original casts, I-0100 RESULT tranche 6); `FG_A_2012` (the eyebrows, a Decal byte) stays out,
  mask 0x18, as in the original; no sky, glass or map piece. Against `i118-r5e-default-m30-on`, `mixed` 37 to 43 and `overlaid`
  4 to 10: INFERRED, instances whose class-2 slices now keep 0x1E beside 0x80 or overlay slices at 0x18 (so mixed and non-opaque),
  their category byte keeping them out of the casters (not counted as `translucent`); not read model by model. The walk's views match the R5a traced run `i118-r5e-default-m30-on` at 2400 (1,609 mask pixels
  newly shadowed, 1,668 newly lit: the walk's small differences) and at its 3300 against this run's 3600 (3.9 % of the presented
  pixels differ by more than 20 a channel); elsewhere the views differ. At 2400 (`r5c-m30-hair-sheet-2400.png`: the cascades, the
  traced term before and after) the sun is ahead of the camera, the head's back faces away from it and its hair is not a G-buffer
  pixel, so nothing differs on the character. At 3600 (`r5c-m30-hair-shadow-pair.png`: before left, after right) the character's
  ground shadow ends in a head that is a little larger, with a hump on its top (1,003 to 1,194 shadowed pixels in the 150x70
  window at x 900, y 615 of the 1280x720 mask): the short hair's shadow (INFERRED from the shape; the views not identical). The hair's shadow on the face
  or the neck not witnessed (no frame with the sun behind the camera on the head).
- **Optimisation pass** (read): one comparison more per LOD-0 slice when its set is made (kept between walks), one test a made
  instance for the counter; the named rows behind `rt-view.flag`; nothing added on the render thread. On the GPU, small work not
  measured: the sun rays that reach a new caster (the hair, non-opaque) run its candidates' record test and alpha test, and the
  instances newly mixed (above) take the candidate loop for the rays their mask lets in.
- **Final review** (fresh Opus, xhigh, `main...e3044af5`): nothing high; the rule traced end to end and against the original
  (I-0100's FG_A casting); the counts at 2400 recomputed. Answered: M1 STATE's rows stale (rewritten at this checkpoint); L1 the
  optimisation's wording and the `mixed` change (above); L3 the shader's comment (a slice of a non-opaque instance); L4 the sun's
  mask never tested on the GPU (now the probe's seventh ray, of the sun's mask, meets the quad cut and the transparent caster behind
  it, while the view's ray passes it over; build-4); L6 the counter's duplicates worded (a wind duplicate is counted before its
  join); L7 the records (the sheet script's path, build-3); L8 the test's redundant bit, FG_A_2012. Their step review (fresh Opus, high,
  `e3044af5..dcb45116`): nothing high or medium, the probe's sizes and offsets checked against `rt_probe.hlsl`, both faults it
  guards seen to fail it; answered L1 (build-4's result), L2 (this conclusion, the index), L3 (a test comment); kept L4 (the sun
  ray's material words not checked: the view's rays' are). Kept: L2 `m37_00_00_00_001005`
  (class 2, category 3, at Irithyll, not identified) now casts at any distance where the original's cascades cut casters at their
  extent: open, to read in the next m37 run with `rt-view.flag` (`native_rt_translucent_caster`); L5 the cascade rule in two copies
  (the raster's row and `RtSliceCasts`): the raster's cascades leave when R5 is complete (D-0237), so no shared helper now.
- **Merged** `80a424a9` (`b3677282`'s tree; the launcher rebuilt from `main`, 114 unit tests passed; start check `start-80a424a9`
  passed, restored).

## Evidence

- R5c: builds `evidence/I-0118/artifacts/r5c-build-1` (the first commit), `r5c-build-2` (the step review's answers), `r5c-build-3`
  (`e3044af5`, the reviewed tree: every unit test passed, 114) and `r5c-build-4` (`7b473dde`, the final review's answers: every
  unit test passed, 114, `PASS rt scene` with the sun's ray), their logs beside (build-1's and build-3's folders deleted, D-0142); the run `evidence/I-0066/artifacts/i118-r5c-hair-m30-on`; its sheets `evidence/I-0118/artifacts/r5c-m30-hair-sheet-{1800,2400,3000}.png`
  (`evidence/I-0118/artifacts/scripts/Make-R5cSheet.ps1`), `r5c-m30-head-{1800,2400,3000}.png`, `r5c-m30-hair-shadow-pair.png`, `r5c-m30-head-shadow.png`,
  `r5c-overview-*.png`.

- Builds `evidence/I-0117/artifacts/build-44` (the PIX run's), `build-49`, `build-50` (the own gate, its unit tests); `build-39` (its unit test's debug-layer notes failed the build; the module ran nothing), `build-40`
  (the runs' module, every unit test passed).
- Runs `evidence/I-0066/artifacts/i118-r5b-{raster,traced}-{m40,m30}-on` (and `i118-r5a-*`, at the title); sheets and differences
  `evidence/I-0118/artifacts/r5b-*`.
- The orientation witness: build `evidence/I-0117/artifacts/build-51`, run `evidence/I-0066/artifacts/i118-r5d-witness-m30-on`,
  `evidence/I-0118/artifacts/r5d-m30-witness-sheet-{1800,2400,3000}.png` (the traced run's and the witness run's pictures above,
  their masks below, the 0.5 pixels red), `r5d-m30-missing-2400.png`; the scripts `evidence/I-0118/artifacts/scripts/` (with the FilterMode sweep, `scan.ps1` and its
  table `filter-mode-sweep.txt`).
- The cascades removed: builds `evidence/I-0117/artifacts/build-52` (the first), `build-53` (main merged in), `build-54` (the step
  review's fixes, a C4324 build failure), `build-55` (the final review's fixes), `build-56` (their step review's), `build-57`
  (the skips counted); `r5d-m30-missing-3000.png`; the run `evidence/I-0066/artifacts/i118-r5e-default-m30-on` and its sheets
  `evidence/I-0118/artifacts/r5e-m30-default-sheet-{2400,3000}.png`.

## Closed (2026-11-09)

Closed 2026-11-09 with Thomas's yes (relayed by the coordinator, 01:55 Paris): the open points below are carried to [I-0132 R11-PLAN §6](../I-0132-rt-optimisation-zones/R11-PLAN.md#6-the-open-points-carried-from-the-investigations-closed-on-2026-11-09), where each has its place.

## Uncertainty / unresolved questions

- The right edge's pillar; the ray offset's acne on other surfaces; the hair and the transparent casters (left out of the rays);
  the term's look against the original (D-0237: Thomas's to judge in play).
- The cascades' removal seen in one diagnostic run only (the High Wall, 1280x720; the rays are off by default, so the start check
  does not draw it); the rays' walk skipped no walk on a short walk at Firelink (`i119-walk-skips-m40`), a long travel not tried.

## Suggested next step

R3 next in the RT/PT plan's order; the local lights' shadows (R5b) with R4's light list (the local lights are shaded by DS3's
own light programmes, which read the local atlas: their traced visibility needs the light shading R4 makes native); I-0117 T5c's
L3 (timed first).
