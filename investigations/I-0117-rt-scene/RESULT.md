# I-0117 - DS3 DX12 Vague 2: R2 of the RT/PT plan, the scene in rays (La scène en rayons): result

## Conclusion

Complete (closed 2026-11-09 with Thomas's yes, « Closed » below); its history: Active (opened 2026-11-01 13:30 by the RT pilot on branch `claude/zealous-antonelli-08ec08`, after I-0114's seventh tranche merged
`5ae7905c`; handed to the fresh R2 thread, branch `claude/dreamy-bohr-fbce0e`, at 14:05). Tranche T0 written, step-reviewed, run at
Firelink with the flag on and off (no regression of the grading or the glow), merged `37ba1db7`. T1 (the loaded world's
instances and the top level) merged `72a6b0c7`. T2 (the skinned and deformed geometry) merged `6b57a5d6`. T3 (the alpha-tested
geometry, with T2's carried points and one skinned instance a pose) merged `12d82522`. T4 (the control view, `rt-view.flag`)
merged `98fa3a4b` as the tool (start check passed), its exit not met (Firelink 99.6 %, the High Wall 95.4 %, m45 not run): carried into T5.
T5a (merged `e4d8220b`; the rays' bits by material class, the sky and the overlays out of the view's and the casters' rays, the programme sets the
raster had not built, the faces' second stream) is merged as the tool: T4's exit is met by the number at Firelink (99.77 %) and m45
(99.89 %), not at the High Wall (95.3 %, its remainder INFERRED the wind's grass in its rest pose); not every disagreement is
explained (RESULT « T5 »; at the High Wall 99.71 % outside the pixels an alpha test met, `i117-t5f`). T5b (the walk on the pool;
the structures' GPU time by PIX, 0.92 ms at m45: no Mega Geometry case) merged `a8b03f3a` with I-0118's R5a; T5c (the alpha
entries' shards, the skinned firsts on the pool): the walk on the engine's thread from 2.80 to 1.20 ms at m45, 1.31 to 0.78 at the
High Wall (RESULT « T5b », « T5c »). R2's measures are done; its open points (the High Wall's grass, the characters' outline) go
with R5's judgement on sight. T6 (the wind's grass, RESULT « T6 ») merged `4277541c`: the High Wall's grass 11.7 to 7.0 steps of
albedo, the control view 93.5 to 96.5 % (T4's 99 % still not met there). T7 (the fault under PIX, RESULT « T7 »): the debug layer
off with `pix-capture.flag`, the calls it reported at a capture INFERRED the capturer's; a run takes its four captures.

## Plan

[R2-PLAN](R2-PLAN.md) (copied from I-0110 in this investigation's first commit), six tranches: T0 the BLAS registry, T1 the
loaded-world instances and the TLAS, T2 skinned and deformed geometry, T3 alpha-tested geometry, T4 the traced control view and its
depth check against the raster, T5 the measures and the Mega Geometry verdict (D-0238).

| Tranche | State |
|---|---|
| T0 | merged `37ba1db7` (the launcher rebuilt, start check `start-37ba1db7` passed) |
| T1 | merged `72a6b0c7` (the launcher rebuilt, start check `start-72a6b0c7` passed) |
| T2 | written, step-reviewed, run at Firelink (`i117-t2a-on`), final review answered and its fixes step-reviewed; merged `6b57a5d6` (the launcher rebuilt, start check `start-6b57a5d6` passed) |
| T3 | written on `claude/optimistic-jones-542fc2`, step reviews, run at Firelink and the High Wall (`i117-t3a`, `i117-t3b`), final review answered (build-22); merged `12d82522` (the launcher rebuilt, start check `start-12d82522` passed, on the OLED) |
| T4 | written on `claude/stoic-jackson-721bb5`, step review, runs at Firelink and the High Wall (`i117-t4b-on`, `i117-t4b-m30-on`), final review answered; merged `98fa3a4b` as the tool (the launcher rebuilt, its unit tests passed, start check `start-98fa3a4b` passed, on the OLED); R2-PLAN's T4 exit (99 % at three zones) not met and carried into T5 |
| T5 | T5a on `claude/practical-bhabha-347690` (2026-11-02 05:25-09:00): written, two step reviews, runs `i117-t5a` to `t5e` (build-27 to build-33), final review answered (build-35), its fixes step-reviewed (build-36); merged `e4d8220b` as the tool (the launcher rebuilt, its unit tests passed, start check `start-e4d8220b` passed, on the ASUS; T4's exit met at Firelink and m45, not at the High Wall). T5b, the measures: next, in a fresh thread; T5b on `claude/zen-benz-ce4cac` (2026-11-02 10:05-12:30): the walk on the pool, step and final reviews, runs `i117-t5g` to `t5j`, merged with I-0118 R5a; T5c (the shards, the skinned firsts on the pool), final review answered |
| T6 | the wind's grass (R2-PLAN T2's rest pose replaced), on `claude/vigorous-kepler-99a438` from 2026-11-03 08:20: merged `4277541c` (step, fix and final reviews answered), runs `i117-t6a` (no grass moved: UNORM8) and `i117-t6b` (the High Wall's grass 11.7 → 7.0 steps, the control view 93.5 → 96.5 %) |
| T7 | the fault under PIX, on `claude/happy-yonath-7a436a` from 2026-11-03 11:57: the debug layer off under PIX; two step reviews answered; runs `i117-t7`, `t7b` (closed at the first capture), `t7c` (the four captures taken); merged `1ad14b19` (the launcher rebuilt, start check `start-1ad14b19` passed) |

## T0, the BLAS registry (2026-11-01, written)

- **One BLAS a model storage, one geometry a mesh slice** (`native/renderer/rt_scene.{h,cpp}`, `RtBlasRegistry`): a deviation from
  R2-PLAN's « one BLAS per mesh slice », chosen because a DS3 model's meshes share one placement (one TLAS instance a model, fewer
  instances); a ray query's `GeometryIndex()` is the slice's index (an ineligible slice is kept as an empty geometry so the indices
  hold). `PREFER_FAST_TRACE | ALLOW_COMPACTION`, opaque geometries (T3 marks the alpha-tested ones).
- **Where**: the model's geometry job on the storage queue (`14_scene_step_and_storage.inc`, after its meshes are published) records
  the build on its own ingress list, after the buffer's upload list on the same queue; the storage buffer is left in
  `VERTEX_AND_CONSTANT_BUFFER | INDEX_BUFFER | NON_PIXEL_SHADER_RESOURCE` when the flag is on (unchanged without it).
- **Compaction**: the build writes its compacted size into a slot of one shared UAV buffer, copied to a persistently mapped readback
  in the same list; each list leaves the UAV buffer in `UNORDERED_ACCESS`. Once the build's list completed (a completion token kept
  by the recording, released when that submission completed), the frame owner (`02_frame_lifecycle.inc`, beside the streamed
  memory trim, each frame) records the compacting copies on one ingress list, swaps the structure and keeps the large one until
  that list completed. The structures and the scratch are placed in the pooled buffer heaps (`CreateStreamed`), each kept by the
  list that writes it (a storage retired before its list is submitted retires against an earlier ticket).
- **Release**: the storage holds its BLAS (`NativeModelStorage::blas`), so the storage's retirement rule (kept until the submitted
  work completed) covers it; the shutdown drops them after the drain.
- **Flag**: `rt-scene.flag` (accepted by `Run-StartupTrace.ps1`), on only when the device offers DXR 1.1 and the start self-test
  did not fail (`native_rt_scene` row); a failed build names the model (`native_rt_blas_failed`) and leaves it out of the rays.
- **Rows**: `native_rt_blas` every 600 frames (models, geometries, skipped, triangles, built and compacted MB, ready, failed, pending,
  the postbuild slots free). `skipped` stays 0 in the game (`ValidContent` refuses a model with such a slice before its BLAS);
  `compacted_mb` counts the structures' 256-byte sizes, not the 64 KB placed ranges small ones take.
- **Unit test** (`native/frame/rt_scene_test.cpp`): the slices a BLAS takes; on the hardware device with the debug layer, one
  model's BLAS from a storage-like buffer (a quad of R16 indices, a triangle of R32, one ineligible slice) built, compacted and ready,
  its counters, no debug-layer message. A first version's completion token was a null `shared_ptr`, which `IngressRecording::Keep`
  drops at once (the test caught it: « built once its list completed »); the debug layer then named the empty geometry's index
format (now `UNKNOWN` with no index buffer) and the postbuild buffer's initial state (now `COMMON`, promoted by the build). Passes:
2,816 bytes built, 1,280 compacted.

- **Step review** (fresh Opus, high, on `f66e74ed..45aeb9bb`): nothing high. Answered (`f454c6a6`, build-3): M1 the compaction
  ran inside the 120-frame window (now each frame); M2 a model destroyed before its reaping, or a build failing before its commit,
  kept its postbuild slot (now given back: the pending entries carry their slot, a guard covers the throw paths; `slots_free` in the
  row); L1 the scratch committed on the storage queue (a paging-fence wait a model, I-0079: now placed); L2 the reaping's erase in a
  loop (one pass); L3 one compacted structure that cannot be placed failed the whole batch (now placed first, the rest left built
  for a later Advance); L4 the test's second case (two builds in one submission, one model destroyed before it, both slots back) and
  whether the debug layer validated (« active » on this PC); L5-L7 comments. Before it, the BLAS and its compacted copy were not
  held by the lists that write them (`45aeb9bb`).
- **Run at Firelink** (`Run-I117T0.ps1`, build-3, Eta's m40 zone save, the ASUS, 14:42-14:45; `i117-t0b-on` with `rt-scene.flag`,
  `i117-t0b-off` without, both with `world-pictures.flag`, both under the debug layer with no error; restored by hash): no fault, no
  refusal. With the flag, the row stable from frame 1860: 419 models admitted since the start (the title's 253 included; the
  counters are cumulative, never decremented: 397 storages live at Firelink from frame 2490, per the final review), 1,256
  geometries, 4,914,095 triangles, 329.5 MB built, 143.1 MB compacted (0.43), every one ready, none failed, none pending. The
  placed memory at frame 3060 (`native_streamed_memory`): 999.6 MB live with the flag against 864.5 without (+135 MB, the live
  BLAS), the buffer pool's heaps 576 MB against 320 (+256 MB, held by the transient built structures and scratch ranges). The grading (I-0113 gap 4's oracle over the dark pixels, frames 1800, 3000, 3600): trilinear within 1.12, 0.90, 0.88 with
  the flag and 1.12, 0.89, 0.89 without (nearest 4.3 to 4.7 in both); the glow's equal right-neighbour share 0.147 and 0.070 with,
  0.135 and 0.098 without (the banded state was 0.79): R2-PLAN §4's risk not met. A first pair (`i117-t0-on`/`-off`, 14:30-14:36)
  never left the title (the pilot's screen variable lost its backslashes in Bash): at the title 253 models, 48.9 MB built, 23.7
  compacted, none failed.
- **Optimisation pass** (read): no duplicate path; the builds reuse the storage's buffer and ingress list order, the compacted
  sizes one shared buffer; nothing runs without the flag.

Not done in T0 (R2-PLAN §3): the GPU build time (timestamps), the frame time with the flag on and off (no measure before a merge,
D-0227), the High Wall, the 64 KB placement granularity of small structures (a sub-allocator if T5 shows the memory weighs). The
Firelink zone's 397 live models are models, not the 4,057 map piece instances I-0110 §2.2 counts (T1 counts those). The winding
convention R2-PLAN §2 asked to check on one mesh is not checked. One BLAS a model (not a slice) removes R2-PLAN §2's per-slice
two-sided route: `TRIANGLE_CULL_DISABLE` is an instance flag and DXR geometries carry no culling, so a model mixing `cull_class` 0
and 1 needs a split BLAS or non-opaque geometry; T4 decides (its primary rays may hit back faces the raster culls).
- **Final review** (fresh Opus, xhigh, `main...d80f8b9a`): nothing blocking; the tested code is HEAD's (build-3's module SHA in
  both runs). Carried into T1, before any BLAS address is read: M1 the row's counters cumulative (live models and placed MB to add,
  decremented when a BLAS goes); M2 the compaction placing and recording one structure a model on the engine thread with no bound
  (a cap a frame, or the work on the storage queue); L1 `no-rt-selftest.flag` leaves the scene on (it should switch it off); L2 a
  persistent Advance failure logged and retried every frame (rate-limit, back off); L3 the flag read twice a model (record per
  storage whether its buffer was left readable); L4 a replaced `held->blas` not retired against the frames, and `structure`
  written on the engine thread while T1's TLAS reads `Address()` (publish it atomically, retire a replaced BLAS by the frames); L6
  the skinned models' bind-pose BLAS not to be instanced (T2); L7 a scratch range and an ingress list a model (a scratch ring or
  a transient pool); L9 the release untested (the placed bytes before and after a storage goes). RESULT's figures corrected
  (M1, L8).

## T1, the loaded world's instances and the top level (2026-11-01, written)

- **The walk** (`CollectNativeRtInstances`, `12_scene_collect.inc`): each main-view collect walks the world again with the engine's
  traverser, as the shadow walks, on a culler copy whose planes and world box hold a box 1,000 km a side (no entity is ended by
  them) and the walk's own frustum the same box (so the main view's pop-in gates and margins, tallies and witnesses leave it
  alone); its categories are the main view's low byte with 0x02, 0x04 and 0x10; the view's display-group bits are kept, so
  « everything loaded » is what the original's own walks can show from where the player stands. The entities' gates are
  AdmitNativeSelectedObject's own; their candidates are gathered (`deferred`), never admitted. Each candidate becomes an
  `RtInstance` (its model storage's BLAS, read once a storage, held by the frame; its 3x4 placement as the DXR transform; its id;
  its mask) unless it is an assembled character's part or has a pose of more than one row (T2), not finite, or has no storage or
  BLAS. Masks by the original's views, as the main walk's rule (AdmitNativeModelCandidate): from the entity's category byte
  (+0x20, map pieces and draw entities alike), the view bit for 0x01 (the 0x45 view, no suppress test) or 0x02/0x04 when not
  suppressed (the pass 5..8 view), 2 for 0x02 when not suppressed (the sun's cascades, passes 5..8), 4 for 0x04 (the local lights'
  faces), 8 for 0x10 (the reflection), 0x80 when none; the suppress byte the probe pick writes (item +0x71A, a draw entity's
  +0xA5D) counts for passes 5..8 only. An entity visit that fails during this walk (a child list's bound, the disp service, an
  exception) skips the walk for that frame instead of closing the game.
- **The top level** (`RtTlas`, `rt_scene.cpp`; `SceneRenderer::EncodeRtScene`, `scene_pass_frame.cpp`): the instance records in
  the frame's upload arena, the build (`PREFER_FAST_BUILD`) at the start of the main view's list after the paint, a null UAV barrier
  before it (the ingress lists' BLAS builds and copies share the frame list's ExecuteCommandLists call) and after it; the structure and its scratch grown by half again, the former retired after the current recording; a device failure
  turns the rays off for the session and is named in the `native_rt_blas` row (`tlas_failed`), the raster going on. Nothing reads
  the top level yet (T4).
- **T0's carried findings**: live models and placed MB in the row, decremented when a BLAS goes (M1); at most 32 compactions a
  frame (M2); `no-rt-selftest.flag` leaves the scene off (L1); the Advance failure row rate-limited, 60 frames between attempts
  (L2); whether a storage's buffer was left readable recorded with it at its upload (L3); the BLAS structure behind its own lock,
  read as a reference the frame holds, the compacted one published only once its copy list is committed (L4, T1 step review M1);
  the release tested (L9).
- **Unit test**: a top level of two instances of the test BLAS (one moved 10 along x and masked as a sun caster), built twice on
  one list, and five rays through it in a DXIL probe (`rt_probe.hlsl`): the instance id (24 bits), the slice (`GeometryIndex()`),
  the distance and the mask as expected, the moved instance's model built in the top level's own submission (a smoke test: the
  debug layer does not see hazards across lists); the gone models' placed bytes released; no debug-layer message (build-8).
- **Step review** (fresh Opus, high, `c7fcac34..c37f1aaa`): nothing high. Answered (`899af32e`, build-5): M1 the compacted BLAS
  published before its copy list was committed (a frame list sent in between would trace an unwritten structure); M2 the view bit
  taken from the walk's union (a reflection-only piece would be seen by the camera); L1 the scratch reused without a barrier (the
  test's two builds); L2 the suppressed items (this answer was wrong: see the final review, H1); L3 the walk counted in the main view's tallies and witnesses
  (the unsupported count, the cloth-set row, the disp census); L4 a persistent skip logged every collect; L5 one structure read and
  one lock an instance; L6 the top level's failure not named. Not done: the static cache (L5, below), the fade weight of a
  dissolving entity in the masks (L2's second half), a zone travel with the flag on (L6).
- **Final review** (fresh Opus, xhigh, `c7fcac34...eab26e1c`): blocking, H1: the suppressed items were taken out as « never drawn »,
  but the original tests that byte only for passes 5..8 and the main walk draws them through the 0x45 view (the m40 candles and
  player, Native2258); the claim was plausible, never checked, and wrong. Answered (`72c04da6`, build-6): the items kept, their sun
  bit cleared (H1); a UAV barrier before the top level's build (M1); the main view's always-on totals left alone (L1); the walk's
  gate pointer cleared on unwinding, an unbalanced traversal or an exception skipping the walk (L2); its candidates kept between
  walks (L3); no walk once the top level failed (L4). Its step review (`eab26e1c..72c04da6`) answered in `af30a5a8` (build-8): an entity
  visit failing inside the walk still closed the game (M-a), a draw entity's suppress byte and categories unread (M-b, L-e), the
  call site's exception text (L-a); that step's own review (`dc025f4e..af30a5a8`): nothing blocking; carried into T2: one
  entity's failure skips the whole frame's rays instead of that entity alone (FINISH-LINE line 1's « skip the object »: trim the
  candidates back, name the entity, go on), the exception text printed unbounded and unescaped, the new collect-thread locals not
  in the thread-affinity allow-list. Recorded, not done: R2-PLAN §3 T1's count against the MSB (M2) moves to T4, whose depth check
  is the direct test of a missing object; the top level sits inside the G-buffer pass's scope with no frame-graph node or key bit
  of its own (R2-PLAN §1), and there is one top level, not one a frame slot (one direct queue: a later frame's build runs after
  the previous frame's reads; to revisit when a traced pass reads it on another queue) (L6); `rt_scene_address_` and
  `RtTlas::Resource()` wait for T4 (L7); the skip counters (`NoteNativeSkip`) still count the rays' walk (step review L-d).
- **Runs at Firelink** (Eta's m40 zone save, the ASUS, restored by hash, no fault): `i117-t1a-on` (build-4, 18:57-18:59),
  `i117-t1b-on` (build-5, 19:17-19:19) and `i117-t1c-on` (build-8, the final tree, 20:25-20:27). In `t1c`, each walk from call 600
  on: 544 candidates, 243 instances (all map pieces; 5 of them suppressed, their sun bit cleared), 296 skinned for T2 (235 map
  items with a pose of more than one row, 56 assembled parts, 5 objects), 5 with no storage; the walk 0.23 ms on average (0.55 at
  most). In `t1b` (build-5, the suppressed items wrongly left out: 238 instances, 208 left out, 93 skinned) by gate, 483 map pieces admitted and 3 ended by their flags, 5 objects admitted and 1 ended, 4 assembled characters
  admitted and 4 ended by their state (the same in `t1c`); the walk 0.28 ms on average (0.65 at most) on the collecting thread; the top level 0.06 MB
  with its scratch; at the exit the live BLAS fall to 0 (the release seen). The walk visits about 500 entities where the m40 MSB
  places 4,057 map pieces: the gates end almost none, so the traverser itself keeps to the loaded, displayed part of the map (the
  display groups, INFERRED; T4's depth check will show whether anything the raster draws is missing; Thomas's « Tout » read as
  everything the engine can show from where the player stands). At 0.23 to 0.28 ms the static cache
  of R2-PLAN §3 T1 is not needed at Firelink (measured again at m45 in T5).

## T2, the skinned and deformed geometry (2026-11-01, merged `6b57a5d6`)

- **T1's carried points** (`1ab99b60`): in the rays' walk an entity whose visit fails (its children's bound, an exception) is
  skipped alone, the candidates it had gathered taken back and the entity named (`NoteNativeSkip`), the walk going on (only a
  missing disp service still skips the whole walk); the exception's text bounded (160 characters) and escaped, the row's buffer
  sized for its bounded fields (a `sprintf_s` overflow ends the process); the call site in its own function
  (`CollectNativeRtScene`), both functions and their locals in `scripts/thread-affinity-allow.json`. The BLAS advance failure
  row's reason bounded alike.
- **How DS3 places a skinned vertex** (OBSERVED: the 6,685 vertex programmes of the I-0063 programme cache disassembled,
  `D3DDisassemble`; `evidence/I-0117/artifacts/t2-vs-census`: the scanner, its table `skin-vs.tsv`, the counts by shape
  `shapes.txt`, three programmes' listings): the non-`_Skin` programmes read one palette row, `NORMAL.w + matricesData.x` (rigid: the map items and
  objects with bones); the `_Skin` programmes the four `BLENDINDICES` rows weighted by `BLENDWEIGHT` over its sum (at least
  0.001); the ones that read VS b12 blend the cancel rows alike and apply them first, `p = (Σ ŵ·b5[i]) · (Σ ŵ·b12[i]) · v`; the
  palette's output is in the world (the camera position is subtracted after it). By shape: 2,044 rigid with the base, 2,022
  blended with the base, 878 rigid and 488 blended instanced (`_Ins`, refused natively), 171 blended reading b12 (38 of them
  instanced); about 500 others read another lane or pattern (grass, `_Pnt`, rigid with b12): not reproduced, and their slices
  left out (below). The `_cloth` programmes take `VC_World` (cbvModelParam) instead of the palette when `VC_isEnableCloth` is set.
- **The compute skinning** (`native/renderer/shaders/rt_skin.hlsl`, `RtSkinner` in `rt_scene.{h,cpp}`): one thread a vertex, one
  slice a group row; per skinned instance its own positions buffer (float3) and BLAS (`ALLOW_UPDATE | PREFER_FAST_TRACE`, one
  geometry a storage slice: `GeometryIndex()` still the slice; the storage's indices), written on the main view's list before the
  top level when the pose's generation changed (or a physics stream's), then refitted in place (`PERFORM_UPDATE`), rebuilt every
  120 refits; an instance of the top level with the identity transform; one scratch for the frame's builds, each its own range.
- **What the collect gives** (`NativeRtSkinnedOf`, `12_scene_collect.inc`): the raster's own inputs, read where the raster reads
  them: the engine model's meshes (`ReadCurrentModelInputsAt`), the native pose's palettes (`model_poses`, current, auxiliary,
  cancel), the owned vertex layouts (`model_layouts`); a mesh taken when its flags hold bit 0, its LOD mask LOD 0 and its
  material's capability words meet the model's (63CB0's tests); its G-buffer vertex programme's shape from the layout's kept
  programme set (`NativeRtShapeOf`: the palette indexed by `NORMAL.w` or by the four `BLENDINDICES` with `BLENDWEIGHT`, b12 read
  or not), a slice left out unless the shape is one reproduced (rigid without b12, blended) or while no set is kept (a model of
  that layout the raster never admitted); the binding by `SelectOrdinaryModelPoseBinding` with the raster's fallback; the cancel
  when the programme reads b12 and the binding has it or a cloth pose retains it (the 1120 rule), an auxiliary binding without it
  refused as the raster refuses it. As the raster, the whole model stays out when its capability words, a mesh's layout or
  material cannot be read, its placement is not finite, or it has physics streams but none for a mesh; a pose row not finite holds
  the instance's last positions. A model object met again in a walk is kept once, its masks joined. The instances are kept walk to
  walk by model object (the engine-wide pose generation tells a new pose); new slices make a new one; one not reached for two walks
  is dropped. Rows: `native_rt_skin` beside `native_rt_walk`.
- **T2b, the physics streams**: a mesh the engine rewrites (`UpdateNativePhysicalModelStreams`, bound by its patch index as the
  admission binds it) is read from that stream's buffer (one dispatch a source), its stream generation in the instance's; the
  stream's upload version held by the frame (a pooled upload buffer goes back to its pool with its version's last holder).
- **T2c, the simulated cloth**: a cloth mesh with a published simulated copy (the admission's 1184 rule: model space, slot 0) is
  read from that copy; with its material's cloth enabled (+0x22: `VC_isEnableCloth`, the admission's rule) it is placed by the
  instance's world row (the placement, as the `_cloth` programmes take `VC_World`), else by the palette as any slot 0; the copy's
  generation and the placement in the instance's generation. Not done: the wind's grass stays in its rest pose (R2-PLAN §3 T2,
  judged on sight in R5); the refits' GPU time (R2-PLAN's row) is left to T5's measure.
- **Unit test** (`rt_scene_test.cpp`): the slices' records (an excluded slice keeps its place, a misaligned element or a slice past
  its source excluded, a stream's slice at its own offset); on the hardware device a rigid quad (its bone in `NORMAL.w`, the cancel
  applied first), an excluded slice and a blended triangle (two bones at half weight) built, then refitted on a new pose, then read
  from a stream buffer, then placed as a cloth copy by the world row, each time traced by rays through a top level: the distances
  follow the rows, `GeometryIndex()` the slice; the auxiliary rows, a rebuild after `kRebuildAfter` refits, a held instance (traced
  at its last place); an unchanged pose neither skinned nor refitted. The debug layer named the test's own
  source buffers left in `COPY_DEST` (its upload list and the skinning's in one submission: nothing decays between them); they are
  now left readable as the game's storage upload leaves them.
- **Step review** (fresh Opus, high, `6c52f73e..90d55955`): nothing high; barriers, refit geometry, lifetime, threads, the shader's
  formulas and the row buffers found sound. Answered (build-12): M1 the cancel applied whatever the programme reads (now only when
  the mesh's G-buffer vertex programme reads VS b12, from its layout's kept programme set or the set built as the admission builds
  it, `NativeRtReadsCancel`; unknown: the slice left out); M2 a model object met twice in one walk would have two builds of one BLAS
  without a barrier (now the second is skipped and counted, `duplicates`); L1 the raster's whole-model gates kept (capability
  words unreadable, a physics stream missing for a mesh: the model out, as the raster); L2 a refused placement no longer leaves an
  instance out for good (tried again after 120 frames); L4 a slice past a dispatch's 65,535 groups excluded; L5 the advance
  failure's reason escaped; L6 the test covers the auxiliary rows and the rebuild after `kRebuildAfter` refits. Kept as
  deviations, for T4: the BLAS take the LOD-0 face set and the LOD-0 meshes at every distance (the storage keeps that face set
  only), the menu faces' stream patch is not read; L3 (the slice table rebuilt each walk, the rows uploaded beside the raster's)
  for T5's measure.
- **Final review** (fresh Opus, xhigh, `main...b7a98521`): nothing blocking, nothing high. Answered (build-13): M1 the simulated
  cloth placed by the world row whatever its material (now only with the material's cloth enabled, else by the palette from the
  copy, and the binding required first); M2 the raster's whole-model gates partly kept (now the placement's finiteness, a missing
  layout or material leave the model out; a pose row not finite holds the instance: a refit may not change which triangles are
  active); M3 the programme shapes not reproduced still skinned (now only the reproduced shapes, read from the kept programme set);
  M4 the census without evidence (now `evidence/I-0117/artifacts/t2-vs-census`); L1 a duplicate's mask lost (now joined; those
  with another pose or placement counted); L2 an auxiliary binding without cancel on a b12 programme (now left out, as the raster);
  L3 the per-owner pose generation (now the engine-wide snapshot generation); L4 programme sets built on the scene thread (now
  only the kept ones read: a model the raster never admitted waits for it); L5, L7, L8 this RESULT's wording; L6 an unused output
  removed. Its fixes' step review (`b7a98521..7464cb82`): nothing high or medium, the rules found to match the raster's code;
  carried into the next tranche: the hold scans every row of the palettes bound, not the rows the slices use (a permanently
  non-finite unused row would keep an instance out or frozen: narrow it, and read the `held` counter in the next run); the
  layout gate runs after the mesh tests (the raster's runs for every mesh of the selected LODs first); the material gate is a
  failed read, not the raster's registry miss (unreachable in practice: layouts leave with their material); the shape and the
  finiteness recomputed each walk (keep them under the programme set's identity and the snapshot's generation; T5's measure);
  a model object whose first candidate fails drops its later duplicates.
- **Run at Firelink** (`i117-t2a-on`, build-12 = `4ff04d39`, Eta's m40 zone save, the ASUS, 22:08-22:10, `rt-scene.flag`, the debug
  layer on and silent, without GPU-based validation: the states of buffers read through root descriptors not checked; restored by
  hash): no fault, no skipped object row. Each walk from call 600: 257 skinned candidates (217 map items, 36 assembled parts, 4
  objects) counted once their storage was found (`skinned` now counts after that read, and `suppressed` includes the skinned: 44 no
  storage against T1's 5, 20 to 26 suppressed against 5), of which 201 duplicates of a model object already met in that walk (step
  review M2 was real; that they are one geometry is INFERRED: build-13 counts those with another pose or placement), 19 with no
  slice, 37 skinned instances,
  all traced (none left out); 145 to 149 slices skinned, 103 excluded by the raster's mesh tests, 49 to 53 not supported (layout,
  binding or programme unknown: not broken down yet), 12 to 16 from a simulated cloth copy, none from a physics stream; about 9
  refits and 0.07 builds a frame (93,000 vertices skinned a frame), 40 MB of positions and structures with the scratch. The walk
  0.48 to 0.59 ms on average (T1's 0.23: the skinned candidates' reads), 23 ms once at the first walks (the programme sets built,
  `NativeRtReadsCancel`), about 1 ms at most later. The static instances unchanged (243). Not seen: whether the static map pieces
  have such duplicates too (coincident instances, a cost, no error: T4/T5), and the pose against the raster (T4's depth check).
- **Optimisation pass** (read): one compute dispatch a vertex source a changed instance, nothing written for an unchanged pose (the
  static map items with bones are skinned once), one scratch for the frame's builds, the pose rows held by reference (no copy
  on the CPU); the slice table rebuilt each walk (step review L3) left to T5's measure; nothing runs without the flag.

## T3, the alpha-tested geometry (2026-11-01 to 11-02, branch `claude/optimistic-jones-542fc2`)

- **T2's carried points** (the last step review's five low points): the hold reads only the palette rows the instance's slices
  index (`NativeRtRowsOf`, from the storage's bytes once an entry: a rigid slice's NORMAL.w, a blended slice's four
  BLENDINDICES; a slice read from a physics stream or a cloth copy every row of its roles), once a pose snapshot; the rigid
  shader path reads its one row (it blended row 0 three more times at weight 0: a row 0 not finite made a rigid slice's
  positions not finite where the programme's are not); the raster's whole-model gates run first for every mesh of LOD 0 (the
  material and the layout in their registries, a layout of another material or type, a missing physics stream), before any
  mesh's own test; the material gate is the registry's (an unreadable pose policy leaves that slice unsupported, as the
  raster's palette is then unbound); the programme shape is read once a programme set (`Programs::rt_shape`); a model object
  whose first candidate stayed out is tried again by a later duplicate that reads other inputs (pose, capability source,
  group, cloth set, source and instance fields). Unit test: a rigid slice whose palettes' row 0 is not finite is placed by its
  own row (t = 11). Step review (fresh Opus, high, `00e5cb69..d597c6d3`): nothing high or medium, the five points found to match
  the raster's code. Answered: L1 an entry read and written back under two locks could outlive the shutdown's close (now the close
  marks the map closed and nothing is written back after it, the alpha sets alike); L2 two new thread-locals (now locals); L3 a
  duplicate equal to a candidate already left out joined its mask to another's instance (now the failed inputs are kept and such a
  duplicate joins nothing); L4 the rows sorted from every vertex's indices (now one bit a row, `RtSkinRowsOf`, moved to
  `rt_scene.cpp`); L5 the rows untested (now `SkinRows` in the unit test); L6 STATE's row.
- **How DS3 tests the alpha** (OBSERVED on `GXFlver_ColDifSpcBumpDOL_DptA` and its vertex programme; the census of every DptA and
  Gbuf_[A] programme in `evidence/I-0117/artifacts/t3-alpha-census`): alpha = `g_DiffuseTexture` (t0) `.w` at `UV · FC_TextureTilingScale[0].xy
  + .zw`, the UV the vertex programme's `itof(TEXCOORD0.xy) / 2048 · VC_TexScroll_0.xy + .zw`, times COLOR.w (1 in that family);
  cut when alpha plus the screen dither's (d − 0.5) · FC_AlphaRef.y is below FC_AlphaRef.x, or when alpha times the instance's
  mulCol.w is 0 with a reference, or by the fade dither. The [A] pixel variant is chosen for an alpha-tested template (the policy's
  alpha reference), a fading draw, or with the global variant off (SelectNativeModelPixelVariant); FC_AlphaRef.x is -100 (no cut)
  when g_AlphaRef is 0, else (ref − 0.0001) / 255 (the MTD producer, `world/producers.cpp`).
- **The census** (Sonnet, `t3-alpha-census`: its scanners, `alpha-ps.tsv` SHA-256 16FD19F5…C107 with 660 rows, `alpha-vs.tsv`,
  `shapes.txt`; the pilot checked the DptA case by hand, the census itself two more by hand): the 331 DptA and 329 Gbuf_[A]
  pixel programmes each have one `discard_nz` and read FC_AlphaRef; 647 share the condition above (cut when the alpha plus the
  dither term is below FC_AlphaRef.x, or alpha × mulCol.w is 0 with a reference, or by the fade dither), the 13 others are water,
  a Veill curtain and an older MultiBlend4 compile. The alpha (41 shapes) is `t0 g_DiffuseTexture.w` alone (61 DptA, 22 Gbuf),
  `COLOR1.w × t0.w` (66 DptA, where COLOR1.w is the constant 1 in most, 104 Gbuf, the vertex's), 1 with no texture (62 DptA, 61
  Gbuf), or another texture, a channel other than .w, a blend of t0 and t7 by the vertex colour or t14's mask, the alpha squared
  (34 Gbuf), and others (142 DptA, 142 Gbuf). The common UV is `itof(TEXCOORD0.xy) / 2048 · VC_TexScroll_0.xy + .zw`, then
  `FC_TextureTilingScale[0]` for t0; a few use TEXCOORD0.zw with VC_TexScroll_1 or float UVs.
- **The rays' alpha test** (`rt_alpha.hlsli`, `RtAlphaTable` in `rt_scene.{h,cpp}`, `NativeRtAlphaOf` in `12_scene_collect.inc`): a
  mesh of LOD 0 whose programme set has the policy's alpha reference and a nonzero g_AlphaRef, and whose DptA pixel programme
  samples t0 alone (besides t38 and t56), is alpha-tested by t0's alpha times the vertex's COLOR1.w where the DptA vertex
  programme reads it (the Grass and Col families), the DptA rule (the shadow rays' of R5); a DptA without a texture cuts nothing;
  any other shape stays opaque and is counted (`alpha` row, not read): R3's material table. Not reproduced for the view rays:
  the Gbuf_[A] programmes' own vertex alpha where their DptA has none, and the squared alpha. Its record holds
  the storage buffer's raw view and the diffuse texture's view (the raster's CPU-only SRV, resolved as the raster's
  `NativeTextureHolders`, copied into the bindless heap), the indices, the TEXCOORD0 element (slot 0 or the second stream; UV16 since the
  final review), the tiling row (`ProduceModelTextureTiling`'s register 1, without the UV animation offsets) and the reference;
  the instance's scroll row is its draw group's first 16 bytes. An instance with an alpha-tested slice is `FORCE_NON_OPAQUE`, its
  records from its `InstanceContributionToHitGroupIndex` on, one a `GeometryIndex()`; a record with neither a texture nor a colour
  is opaque, one with the colour alone takes the colour's alpha (a texture not resident yet: opaque, tried again after 30 walks). The candidate keeps the triangle when the texture's alpha at the
  interpolated UV (the top mip: no derivatives in a ray query) is at least the reference. Left out: the dither terms (no ray
  analogue), the mulCol and fade tests (the fade weight is not in the masks yet, T1), the UV animation offsets.
  The per-frame records go in the frame's upload memory; the views are made when first named and released 120 frames after the
  last frame that named them (or when the texture is gone), against the last submitted ticket; the recording thread is the
  bindless heap's frame owner (`SceneRenderer::SetRtHeap`, after the start self-test). The sets are kept by model object between
  walks (made again when the storage changes, every 600 walks, after 30 while a texture is not resident or a stand-in, when a
  texture was withdrawn, and at a scene reset). Rows: `native_rt_skin`'s
  `alpha` (instances, slices, not resident, not read, sets made) and `alpha_frame` (records, buffer views, texture views).
- **Step review** (fresh Opus, high, `d597c6d3..fe961700`): nothing high. Answered (build-17): M1 the vertex colour COLOR1.w the
  Grass and Col DptA programmes multiply into the alpha was ignored (now read, interpolated, when the DptA vertex programme reads
  it; an unread colour format leaves the slice opaque, counted); M2 the bindless indices differ lane to lane (now
  `NonUniformResourceIndex`); M3 the former zone's textures, buffers and skinned structures held through a load (now the rays'
  kept entries go at the scene reset, and a frame without rays still counts in the table's retirement and sweeps it); M4 the
  `alpha` counts covered only the sets remade that walk (now each set's tallies counted at every walk that meets it); L1 an
  `_l` stand-in texture kept 600 walks (now tried again after 30); L3 an unreadable scroll row (now the identity); L4 a full heap
  turned the rays off (now that slice opaque, counted in `full`); L5 the programme set taken from the first choice (now the
  candidate's prefer_channel, as the admission's). Kept: L2 the tiling's kind-8 and kind-5 inputs (the ordinary row's defaults),
  L6 the records rewritten each frame and the polled revalidation (T5's measure), L7 the formats the unit test does not cover
  (float2 and half2 UVs, the second stream, R32 indices, the colour).
- **Second step review** (fresh Opus, high, `fe961700..94f52cbc`): nothing high. Answered (build-19): M1 the Emi family's DptA
  alpha is the vertex colour alone, with no texture, which the rays left opaque and uncounted (now a colour-only record); L2 the
  colour now required in both the vertex programme's inputs and the pixel programme's; L3 `full` in the row, the totals refreshed
  on frames without rays; L4 the 80-byte records at a 256-byte aligned address (said in `rt_scene.h`: a root SRV or raw reads,
  for T4); L5 the former frame's table address cleared on every frame; L6 this RESULT. Kept: L7 a texture only ever a stand-in
  remade every 30 walks (T5's measure), L8 the colour path untested by the unit test (the test quad has no room for a colour).
- **Runs** (build-17 = `94f52cbc`, `rt-scene.flag`, the ASUS, restored by hash, no fault, no skipped object, `Run-I117T0.ps1`
  with its new `-Zone`): `i117-t3a-on` (Eta at Firelink, 00:40-00:42) and `i117-t3a-m30-on` (Theta at the High Wall of Lothric,
  00:42-00:43). At call 2400, Firelink: 25 alpha instances, 160 alpha-tested slices (1 texture not resident, 95 of a shape not
  reproduced, left opaque), the frame's 177 records naming 24 buffer and 24 texture views; the High Wall: 120 alpha instances
  (666 static instances), 279 alpha-tested slices (none waiting, 25 not reproduced), 629 records, 72 buffer and 60 texture views;
  the walk 0.82 ms on average there (2.1 ms at most). `held` 0 in both (the hold's rows, T2's carried point). The skinned rows
  differ from T2's run (build-12, 37 instances, 145 slices): build-13's final-review rules (only the reproduced programme shapes,
  the cloth by the world row only with its material's cloth enabled) were not run before; here 21 to 28 instances, 81 to 110
  slices, 88 to 127 not supported. And every one of T2's « duplicates » (207 at Firelink, 237 at the High Wall: a model object met
  again in a walk) had another pose or placement: they are not one geometry, and the rays left them out. Now the skinned instances
  are kept by model object and pose (one model posed twice, two instances; `poses` in the row), the duplicates of one pose only
  joined (build-18).
- **Runs again** (build-19 = `f6527d36`, the same protocol, 01:17-01:20, no fault, restored by hash): `i117-t3b-on` (Firelink) and
  `i117-t3b-m30-on` (the High Wall). Every former duplicate has its own pose (`poses` 207 and 237, `duplicates` 0): at Firelink
  229 skinned instances traced (28 before), 533 slices, 64 MB of positions and structures with the scratch (25.8 before), about 10
  refits a frame in play (5,872 over calls 1800 to 2400); at the High Wall 246 instances, 597 slices, 133 MB. That each pose places another copy of the model (the map
  items sharing one model object) is INFERRED from the poses differing; T4's depth check shows it. The alpha instances rise
  with them (Firelink 219, 369 alpha-tested slices; the High Wall 213, 1,282 slices, 1,930 records, 72 buffer and 60 texture
  views, none refused); the walk 0.80 ms on average at Firelink and 1.29 ms at the High Wall (2.3 ms at most).
- **Third step review** (fresh Opus, high, `94f52cbc..f6527d36`): nothing high. M1 the pose rule never run (now run, above); M2 the
  untextured DptA alphas other than the constant 1 (the water's 0, a vertex TEXCOORD2.x: 6 programmes) left out uncounted (now
  every untextured, uncoloured alpha-tested slice is counted, `untextured`, and left opaque); L4 a colour-only slice needed a UV
  (now not, and the shader reads none); L6 the alpha sets by model object only (now by model object and programme channel); L7
  the comments. Kept: L3 the colour's source decided from both programmes' interfaces (the census's vertex expressions show
  one family, FgFaceIbl, whose varying comes from COLOR0: not seen in a run), L5 a cloth slice placed by the world row joined to
  a duplicate of its pose with another placement (none counted in the runs: `duplicates` 0). Build-20.
- **Optimisation pass** (read): the alpha sets made once a model object and channel and kept between walks (the texture
  resolution, the programmes' interfaces and the census rule read then, not each walk); the hold's rows once an entry, their
  finiteness once a pose snapshot; the programme shape once a programme set; one bindless view a buffer or texture for as long as
  frames name it, the records the only per-frame work (151 KB at the High Wall; rewritten each frame although only the scroll
  changes: T5's measure); nothing runs without `rt-scene.flag`.
- **Final review** (fresh Opus, xhigh, `main...57fb1634`): nothing high, nothing blocking. Answered (build-21, unit tests only, not
  run live; the runs were build-17 and build-19): M1 a mesh whose programme set the raster has not built yet (the rays see
  everything loaded) was skipped uncounted for 600 walks (now counted, `unknown`, the set tried again after 30); L2 the float and
  half UV branches had no census support (now only UV16, the shape's; the shader keeps the three); L3 the renderer's list of the
  last skinned instances held their structures and alpha sets through a load (now cleared after the top level's build); L4 the
  table's tick outside the rays' failure path (now inside it); L8 dead and stale text. Recorded, for T4 and T5: M2 R2-PLAN's T3
  exit (« the High Wall's foliage and fences cut out in the control view as in the raster ») moves to T4: nothing reads the
  records before T4's control view, and the cut is shown only on the unit test's quad (UV16, R16 indices); T4 checks on game
  data the t0 alpha, the vertex colour (Grass, Col), the colour-only record (Emi), the R32 indices and the second stream; L1
  `GXFlver_ColDifSpcBumpFlowDisplacement_DptA` samples t0 twice at animated UVs, taken as one t0 sample (one programme, not told
  apart by its interface); L5 the loading screens draw no main view, so the former zone's buffer views wait for the next zone's
  frames (memory only); L6 an instance with one alpha-tested slice is non-opaque whole (the instance flag; the alpha test runs
  for its opaque slices too, which R2-PLAN meant per geometry: T5 measures it, a split BLAS if it weighs); L7 the vertex colour
  decided from both programmes' interfaces, not from the census's expressions (104 DptA programmes take the constant 1: none
  of their vertex programmes shown to read COLOR1.w for another use). Its fixes' step review (`57fb1634..1b9a5a30`): nothing high
  or medium; answered (build-22): the sweep kept after the rays failed, the skinned list cleared on a failure too, `unknown`
  only for a DptA programme missing (an ambiguous, unsupported, invalid or unanalysed one counted as not reproduced again), an
  unreachable branch. Its own step review (`abac31fd..494b812a`): nothing high or medium; carried into T4: a family declared
  alpha-tested with no DptA programme stays `unknown` and is remade every 30 walks (the retry helps only a group composed again
  from loose programmes); the comment said otherwise (corrected, comment only: build-22 is the module).
- **Unit test**: the model's quad alpha-tested on a 4x1 texture (alpha 0, 0, 1, 1): a ray where the alpha is 0 passes the quad and
  hits the triangle behind it (t = 2, slice 1) or nothing, where it is 1 it hits the quad; an instance with no set is opaque; the
  instance records' flags and first record; the views released after 120 frames unnamed.

## T4, the control view (2026-11-02, branch `claude/stoic-jackson-721bb5`)

- **The pass** (`rt_view.hlsl`, `RtView` and `RtViewCameraOf` in `rt_scene.{h,cpp}`, `SceneRenderer::RtViewFrame`/`EncodeRtView`):
  with `rt-view.flag` (a test tool, set at the start with `rt-scene.flag` and a passing self-test), on every 60th main view with a
  top level and on the frames whose job names its pictures, a compute pass before the light grid and the forward rows (the depth as
  the deferred rows left it: the decals and the deferred lighting since write none, and no forward, water, effect or transparent row
  has written it yet) casts one ray a pixel through the
  frame's top level with the view's mask (`RtMaskView`), from the camera as the raster's projection makes it (BuildSceneView's lens,
  the upscaler's jitter undone: `ProduceSceneCamera`'s m20 = 2 jx / W, m21 = -2 jy / H), between the near and far planes, the
  alpha-tested candidates through T3's test (`rt_alpha.hlsli`, its UV and triangle reads now shared with the picture's texel). Each
  pixel is classed against the raster depth (view z from the standard depth, then along the ray): agree (within 2 cm + 1 %), close
  (5 %), nearer, farther, missed (no hit on a raster surface), sky (no surface in either), extra (a hit where the raster has none);
  one atomic a class a wave into a 32-byte buffer, read back at the slot's release (`RtViewSamples`). Pictures every 300th world
  frame (at most 16): `rt-view-<serial>-traced.rgba` (each instance and slice its colour, an alpha-tested slice its diffuse texel)
  and `-classes.rgba` (agree grey by distance, close yellow, nearer blue, farther orange, missed red, extra magenta, sky black),
  R8G8B8A8 at the render extent. Row `native_rt_view` with the walk's rows (every 600th walk): samples, the last sample and the sum
  by class, the agreeing share of the raster's surfaces. The pass is in the frame graph (its pass, the depth's plane read, the two
  pictures declared; plan-key bits 42, the pass, and 43, its pictures' readbacks: on a picture frame that traced nothing the files are
  named `-untraced`); the camera, the depth and the pictures at one extent; a failure turns the tool off for the session (its reason in
  the row's `on` and `why`), never the rays or the raster.
- **Not in this pass**: R2-PLAN's geometric normal and hit-distance outputs (the classes and the traced picture answer the check;
  a geometric normal needs the triangle's positions, fetched per hit: R5 or R6 when a pass needs it), and the sun-shadow preview ray
  (left to R5, which writes it on this scene).
- **Unit test**: `RtViewCameraOf` (the lens, the planes, the jitter in NDC, a refused camera); on the hardware device a camera 2
  before the model's quad and a 4x4 depth whose rows are the quad's depth, 3 % farther, 50 % farther and two empty pixels with one
  nearer: agree 5, close 4, nearer 4, farther 1, extra 2; masked to the sun casters, missed 14 and sky 2; the classes picture's
  extra pixel magenta, an agreeing one grey; an 8x4 view (aspect 2) whose corner sees the quad's top right corner with half a pixel of
  jitter over an empty raster: the six hits exactly in the left three columns of the lower two rows (a flipped axis, swapped lens
  scales or the jitter's sign would move them).
- **Step review** (fresh Opus, high, `5963cf08..3a6c66af`): nothing high. Answered (build-25): M1 the unit test could not see the
  ray's direction or the jitter's sign (the corner view above); L1 the extent of the camera, the depth and the pictures now checked
  equal; L2 `rt_view` atomic (the engine thread's rows read it); L3 a failure's reason kept and written (`on`, `why`); L4 a picture
  frame that traced nothing names its files `-untraced`; L5 this section's place of the pass and bit 43; L6 the new render-worker
  functions (and T1's `EncodeRtScene`) in `Test-ThreadAffinity.ps1`. Kept: the trailing UAV barrier of `RtView::Encode` (it closes
  the pass for any caller), and the pictures held until the close once the tool turned off (memory only, a test tool).
- **Runs** (build-25 = `43a4254a`, `rt-scene.flag` and `rt-view.flag`, the OLED, offline, restored by hash, no fault,
  `Run-I117T0.ps1 -Modes on -View`): `i117-t4a-on` and `i117-t4a-m30-on` (03:30-03:36) left out: the screen's name lost a backslash
  in the shell (the pilot could not place the window: Firelink stayed at the title; at the High Wall Thomas loaded the test
  character by hand, so that run reached the world under his inputs: 94-95 % agreeing, the same picture as below). `i117-t4b-on`
  (Eta at Firelink) and `i117-t4b-m30-on` (Theta at the High Wall), 03:42-03:45, the window placed on `\\.\DISPLAY2`, 1280x720,
  about 37 samples each (`native_rt_view` at call 2400):
  - **Firelink**: 99.63 % of the raster's surface pixels agree (sum: 34,891,367 agree, 43,714 close, 52,012 nearer, 33,680
    farther, 0 missed; 27 extra).
  - **The High Wall**: 95.36 % (24,054,602 agree, 402,834 close, 371,334 nearer, 396,654 farther, 0 missed); 7,952,176 extra (26 %
    of the pixels in the last sample, 24.0 % over the sum: the sky).
  - **No pixel missed** in either zone: no surface of the raster is absent from the rays.
  - **The disagreements, seen on the pictures** (`rt-view-3300-*.png` of both runs): (a) the High Wall's grass (the speckled
    close/nearer/farther mix inside the bushes): the wind's grass in its rest pose in the rays (R2-PLAN §3 T2, judged in R5), the top
    mip and no dither; the bushes themselves are cut out as in the raster (the traced picture shows their texture's blades, not
    their cards: R2-PLAN's T3 exit, seen on game data). (b) A halo over the player's hair (nearer, a few pixels) and a one-pixel
    nearer outline on the character's left edges, in both zones: INFERRED a pose one frame apart from the raster's and the top mip's
    larger alpha coverage, not proved. (c) The sky: the High Wall's sky model is in the top level with the view's mask while the
    raster leaves the depth at 1 there (drawn without a depth write): every « extra » pixel. Firelink's vines (a few speckles) as (a).
  - The walk at the High Wall 666 static and 285 skinned instances; the alpha rows: 1,282 alpha-tested slices, 25 of a shape not
    reproduced, **787 `unknown`** (a family declared alpha-tested with no DptA programme, left opaque; Firelink 52), 1,930 records.
  - Exit (R2-PLAN T4: at least 99 % at Firelink, the High Wall and m45, each disagreement explained): Firelink passes; the High
    Wall is at 95.4 %, its grass explained by the planned rest pose; m45 not run (no hash-checked m45 save in `Run-I117T0.ps1`);
    (b) INFERRED; a close/farther patch on the face under the hair (both zones) not explained. Carried into T5, first: the sky
    model out of the rays (it would cover every sun ray of R5), its family found from its draw; the 787 `unknown` slices; (b) and the
    face checked (the pose's frame); the one-sided slices (the raster culls their back faces, the rays do not: a « nearer » source
    the classes do not tell apart, final review L1); m45. **T4 is merged as the tool, its exit not met** (final review M1); the 99 %
    check moves to T5.
- **Stutter** (Thomas, 03:39, during the t4a High Wall run he drove): not measured. Candidates: the walk (at the High Wall 1.42 to
  1.57 ms mean in the windows after the load, above T1's 1 ms target; 4.8 ms at most), the BLAS builds at the load, the control view (a whole-screen trace every 60th frame and, until build-26,
  a readback buffer made a sample: now a ring of four made once). T5's measure.
- **Final review** (fresh Opus, xhigh, `main...c6052bec`): nothing high, no code defect blocking. Answered (the final tree, built by the launcher's rebuild after the merge): M1 the records
  (this section, the Plan table, STATE, R2-PLAN: T4 merged as the tool, its exit carried into T5); L2 a refused readback map turns
  the tool off instead of faulting the next frame (the picture files' write keeps RecordPixels' rule, as every witness picture);
  L3 build-26 (the ring) built and its unit tests passed; L4 the Evidence below; L5 the sky's share and
  the walk's figures; L6 `CloseRtView`'s drain precondition written; the row's failure text read only with the flag. Carried into
  T5: L1 the back faces (above); the pictures written on every sample though read every fifth (a « pictures » word in the
  constants when T5 measures the pass); the alpha path and a vertical jitter not covered by the hardware test. Its fixes' step review (`c6052bec..510b65fa`, with the ring): nothing high or medium; L1 a sample
  read after a failure no longer added to the totals.

## T5, the check at three zones and the measures (2026-11-02, branch `claude/practical-bhabha-347690`)

- **Plan** (the T4 thread's, the coordinator's relay at 05:24): (1) the sky out of the rays, then T4's 99 % check at Firelink,
  the High Wall and m45; (2) the 787 `unknown` alpha slices; (3) the pose's frame (the character's outline) and the back faces;
  (4) R2-PLAN's T5 measures (the refits', the walk's and the alpha test's cost, the per-instance non-opaque flag), then R5.
- **The sky out of the rays** (first written as below, built as build-27; the rule refined after `i117-t5a`: « Then » below). The sky is map
  geometry of forward-only families (WAVE1-LAST-EFFECTS-STATIC-PREP §3.2 and WAVE1-LAST-EFFECTS §1, OBSERVED at m50 and m30: the
  dome `GXFlver_Col`, class 0x80, at the 0x1F row; the clouds and the ring, class 2, at the 0x29 row), drawn after the G-buffer.
  The rule, by family and not by model (D-0161): an instance's masks follow its LOD-0 slices' material classes as the admission
  makes them (`ClassifyModelMaterial`, read with T3's alpha entry, kept with it): with no slice of a G-buffer class (1, 4, 0x100:
  57A20's engine rows 5 and 7) its view bit becomes the new `RtMaskForward` (0x10: seen by the eye, not a G-buffer surface; the
  control view traces `RtMaskView` alone); with no G-buffer slice that casts (its material's no-cast byte +0xD1 and Decal +0xD0
  clear, the cascade row's refusals of 140680) its sun and local bits go. The cascade row's class mask 0x107 also takes the
  transparent class 2; the rays leave it out (explicit policy: a cascade has an extent, a sun ray has none, so the clouds would
  shadow the world; R5 decides translucent casters). A slice whose programme set is not built leaves the engine's bits
  (`unclassed`). First a mixed instance (G-buffer and forward slices in one model) was kept whole; since build-29 each slice has its
  ray bits in its record (« Then » below). Rows: `native_rt_walk` gains `forward_only`, `uncast`, `unclassed`; with
  `rt-view.flag` the first 32 models made forward-only are named once (`native_rt_forward_only`: classes, category, bits before
  and after). Unit test `SurfaceMasks` (`RtInstanceMask`: not classed, a casting G-buffer slice, the sky's classes, a forward
  caster, G-buffer slices none of which casts, a mixed instance).
- **For the run's diagnosis** (written with the sky's rule, one slot for the three zones): (a) the control view counts, of its
  nearer pixels, those whose hit triangle is clockwise as DXR sees it (`nearer_cw`, the counts' word 7, cyan in the classes
  picture): the raster's back face for an unmirrored instance (its front counter-clockwise), to weigh the one-sided slices (final
  review L1) before choosing how the rays cull them; (b) the `unknown` alpha slices' families named once (`native_rt_alpha_unknown`:
  the family, the reference, the statuses of the Gbuf recipes and of Dpt and DptA). The census (`t3-alpha-census`) has every
  alpha-tested family with both its DptA and Gbuf_[A] programmes (114 each), so the `unknown` families are ones with no
  alpha-tested programme at all: whether the raster then draws them opaque or refuses them (gate 30) decides what the rays do;
  (c) the pose's frame: a skinned instance takes the pose the frame's raster drew the model with (the main view's admission's
  snapshot) when that is its owner's previous version rather than the current one (`pose_raster` in `native_rt_skin`): the
  RT walk runs after the main walk and a pose update between them would put the rays one update ahead (the T4 outline, INFERRED);
  (d) `Run-I117T0.ps1 -Zone m45` (Epsilon's hash-checked m45 save, as `Run-AlphaCast`'s).
- **Step review** (fresh Opus, high, `7f37df5d..2b65bb2d`): nothing high. Answered (build-28): M1 the two statics of the
  forward-only names in `thread-affinity-allow.json` (`Test-ThreadAffinity` would have failed); L1 the walk's new counts made once
  an instance (a duplicate only joins its mask); L2 `pose_raster` counted once the instance is made; L5 the hardware test's quad
  (counter-clockwise from its camera) has no nearer pixel counted clockwise. Kept: L3 the raster's pose is found by model (a model
  object with two poses gets it for its first) and only as the owner's previous version (a ring-1 owner has none): an outline left on
  such a character does not refute the pose's frame; L4 a skinned duplicate drawn through the other programme channel joins its
  mask with the first's classes (negligible).
- **Runs `i117-t5a`** (build-27, `rt-scene.flag` and `rt-view.flag`, the OLED `\\.\DISPLAY2` with Thomas there, 1280x720, offline,
  restored by hash, no fault, 06:22-06:27; `Run-I117T0.ps1 -Modes on -View -Zone m40|m30|m45`; `native_rt_view` at the last walk
  row, sums over the samples):
  - **Firelink** (38 samples): 99.66 % agree (34,901,223; close 49,825, nearer 34,770 of which clockwise 9,600, farther 34,963,
    missed 0, extra 19). Forward-only: three hair models (`HR_A_*`, classes 0x82: the 0x1F row's alpha-tested hair and a transparent
    slice, drawn after the depth the view reads), `m40_00_00_00_009000` (0x82) and `000011` (2).
  - **The High Wall** (36 samples): 95.27 %, the sky still there (extra 8,120,273): the rule took out two transparent pieces
    (`m30_00_00_00_901020`, `901030`) but not the dome, and 165 instances stayed unclassed (787 alpha slices with no programme
    set: the raster had not admitted their models). Nearer 362,198 of which clockwise 190,983 (the grass mostly).
  - **m45** (Epsilon, 22 samples, first run there): 85.93 % (close 2,361,684, nearer 480,371 of which clockwise 30,234, farther
    6,634, missed 2, extra 5). The pictures (`rt-view-3000-*.png`): large irregular cut-out shapes over the walls, one dark
    texel colour, close (yellow) where a wall is right behind them and nearer (blue) where the raster sees farther: grime and
    shadow cards the raster draws in the G-buffer without a depth write (the MTD's `g_GX_bDisableDepthWrite`, which 13b's rows
    take), so the depth the view reads does not hold them; INFERRED from the pictures and 13b, the material not read.
  - `pose_raster` 0 in the three zones: no skinned instance was posed one update apart from the raster (the walk follows the
    main walk without a pose update between them, or the owners are ring 1); the characters' outline (still there: a blue
    one-pixel line on their left edges, and the hair's halo now gone with the hair forward-only) is not that.
  - No `native_rt_alpha_unknown` row: the `unknown` slices are all of models whose programme set is not built, none of a family
    without DptA.
- **Then** (build-29): (a) a surface is a slice of a G-buffer class that writes depth (`!disable_depth_write`): the m45 cards and,
  INFERRED, the High Wall's dome; (b) per slice: each slice's ray bits (`RtSliceKeep`: a surface every ray but the forward ones,
  its casters' bits only when it casts; any other slice the forward and reflection rays) in its record (`RtAlphaRecord::keep`,
  the former padding word), the candidate of a ray with none of them passed over (`RtAlphaAnswers` in `rt_view.hlsl` and
  `rt_probe.hlsl`); an instance whose slices differ (`mixed`) keeps its set for the records and gains `RtMaskForward`; (c) the
  rays build the programme set of a model the raster has not admitted, as the admission does (`AcquireNativeModelPrograms` with
  the mesh's material and channel), at most 16 a walk (`programs_built` in `native_rt_walk`). Unit tests: `RtSliceKeep`, a
  G-buffer class without depth write, a mixed instance; on the hardware the triangle made an overlay passed over by a ray of the
  view's mask and hit by one of every bit.
- **Step review** (fresh Opus, high, `39a4071c..194a1dee`): nothing high; the records' indexing (static and skinned), the opaque
  instances, the full heap and the thread of `AcquireNativeModelPrograms` (the admission already calls it on that thread under 64
  candidates) checked. Answered (build-30): M1 the builds bounded by time too (1 ms a walk, `kNativeRtProgramsAWalk` 16 at most),
  timed (`programs_ms`) and profiled as `MeshPrograms`; L1 `RtMaskForward` added to a mixed instance only when a slice is not a
  surface (`overlays`; a mix of casting and non-casting surfaces, a Decal slice, adds none), counted apart (`overlaid`); L3 the
  admission's gate (the layout's material and type) before a build; L4 the counters' comments; `programs_built` counts the sets
  returned. Kept: L2 the static BLAS takes every slice of the storage, a slice not of LOD 0 keeps the default 0xFF (it answers what
  its instance answers, as before T5); L4 `alpha_instances` now also counts the sets kept for their records; L5 the renderer writes
  every record's `keep` whatever its views (the probe's untextured triangle covers it), the bootstrap's choice to keep a set for
  it not unit-tested; the note that `Test-ThreadAffinity.ps1`'s function pattern may miss a return type with a space
  (`NativeRtAlphaOf`'s statics `unknown_lock`, `unknown_named`, locked) carried to the checkpoint.
- **Runs `i117-t5b`** (build-29, 07:13-07:18) **and `i117-t5c`** (build-31, 07:46-07:51; the same protocol, the OLED, no fault):
  | Zone | agree (t5c, sum) | close | nearer (clockwise) | farther | missed | extra | disagreeing met by an alpha test | agree outside them |
  |---|---|---|---|---|---|---|---|---|
  | Firelink | 99.71 % | 47,511 | 15,929 (9,390) | 33,817 | 0 | 18 | 46,728 of 97,257 | 99.85 % |
  | The High Wall | 95.29 % | 417,440 | 353,073 (199,007) | 358,762 | 49,470 | 6,389 | 1,011,556 of 1,178,745 | 99.30 % |
  | m45 | 99.89 % | 14,734 | 1,518 (995) | 6,659 | 1 | 5 | 4,862 of 22,912 | 99.91 % |
  - **The sky is out**: at the High Wall its 8.1 million pixels are now `sky` (no surface in either), extra 6,389 (from 8,120,273).
    m45's grime cards agree (85.9 % to 99.9 %). The `unknown` alpha slices: 0 at the three zones in t5b (build-29, no time bound;
    787 at the High Wall before); in t5c (build-31, the builds bounded to 1 ms a walk) still 266 at the High Wall after 2,400 walks
    and falling (1,308, 1,067, 399, 266), 47 instances unclassed: the bounded builds converged slowly (a deferred entry waited 30
    walks; final review M1, M2: since build-35 a build the budget alone deferred is tried at the next walk and a failed one is not
    retried for 600 walks), not run since.
  - **The High Wall's remainder**, INFERRED the grass: 86 % of its disagreeing pixels met an alpha-tested candidate (the wind's grass
    in its rest pose, the top mip, no dither: R2-PLAN T2, judged in R5); outside them 99.30 % agree. An upper bound: until build-35
    the counter took any alpha-tested candidate the traversal reported, even one past both surfaces (final review M3; since build-35
    only one no farther than the farther of the two). « Outside the alpha-met pixels » is not R2-PLAN's measure (it leaves out the
    transparents and effects only): the High Wall does not meet T4's exit. Missed 49,470 (a streak in
    the sky's corner, red on `rt-view-3000-classes.png`: a surface the raster writes and the rays do not hold; 9 instances `no_storage`
    at the walk; not identified).
  - **The patch on the characters' heads** (close and farther, T4's « face under the hair »): the face's models `FG_A_*` (families
    `GXFlver_FgFaceSSSIbl_Bloody`, `GXFlver_SkinSSSIbl_Bloody`, `GXFlver_EyeIbl`) are left out of the skinned instances
    (`native_rt_skin_unsupported`, « blend elements »: a BLENDINDICES or BLENDWEIGHT shape `rt_skin.hlsl` does not read); the rays hit
    the head behind them. The hair (`HR_A_*`, classes 0x82, its 0x1F row drawn after the depth the view reads) is forward-only now:
    T4's halo over it is gone.
  - **Back faces**: of the nearer pixels about 60 % at Firelink and m45 and 56 % at the High Wall are on a clockwise triangle;
    nearer is 0.05 % of the surfaces at Firelink, 0.01 % at m45: no culling of one-sided slices in the rays (R5's shadow rays
    take both faces; the raster's cascade rows cull nothing, only the local lights' faces take the face set's class: R5 decides).
  - The walk (t5c, the mean over the row's window, CPU ms on the engine's thread): Firelink 0.76 (the engine's traversal 0.20), the
    High Wall 1.31 (0.40), m45 2.80 (1.70; in t5b 3.65 and 2.22): over T1's 1 ms target at m45, T5's measure.
- **The faces** (build-33, written after `i117-t5d`, Firelink alone, 08:08-08:10, build-32: the rows name the layouts): the
  `FG_A` models' BLENDINDICES (R8G8B8A8_UINT at 12) and BLENDWEIGHT (R8G8B8A8_UNORM at 16) are in slot 1, the mesh's second
  stream; a blended skinned slice now reads them there (`RtSkinSlice::blend_stream`, the record's `blend_offset` and
  `blend_stride`, 64 bytes a record; with a physics stream, which rewrites slot 0 only, the slice stays out); the CPU's row set
  reads the indices there too. Unit tests: the records (a second stream read, refused with a physics stream or past the
  storage) and the rows (indices read in the second stream). Run `i117-t5e` (build-33, Firelink, 08:36-08:38, the OLED, no fault):
  no slice left out for its blend elements; 99.77 % agree (34,939,456; close 36,526 from 47,511, nearer 16,037, farther 28,761 from
  33,817, missed 0, extra 20; met by an alpha test 47,833 of 81,324). The other slices left out at Firelink (20,
  « shape ») are objects (`o0*`) whose programme set the raster had not built when the walk met them, or the `C_ARSN_cinder`
  family's (rigid with a cancel): left as they are.
- **Optimisation pass**: the raster's pose for the skinned instances (`pose_raster`) removed: it never applied in four runs at
  three zones (the walk follows the main walk with no pose update between them), a map built every walk for nothing; the
  characters' one-pixel outline stays INFERRED (not the pose's frame; its cause not found: the faces' absence may explain part of it).

- **Final review** (fresh Opus, xhigh, `main...c3da3c17`, build-34): nothing high; « fix first » for the records. Answered: M1 the
  `unknown` count stated as the t5c run left it (above); M2 a build the walk's budget alone deferred is tried at the next walk
  (`deferred`), a layout whose build returned none not again for 600 walks under the same registries (`native_rt_programs_failed`,
  cleared at a scene reset); M3 `alpha_met` counts an alpha test met no farther than the farther of the two surfaces compared, the
  99.30 % an upper bound; M4 the exit's state written (met by the number at Firelink and m45, not at the High Wall; the grass
  INFERRED; not every disagreement explained: the High Wall's 49,470 missed pixels, the characters' outline, about 50,000 of
  Firelink's disagreeing pixels no alpha test met); L1 the casting rule written as an explicit policy (a G-buffer slice drawn
  without a depth write casts in the raster's cascades, not in the rays: R5); L2 INFERRED in the comment; L3 the programme builds
  summed and their largest a walk since the last row (`programs_built`, `programs_ms`, `programs_ms_max`); L4 `stride2` 4-byte
  aligned; L6 the figures; L7 the records. Carried into T5b: L4 the second stream's GPU branch with no hardware test (closed in T5c: the unit test) and a cloth
  copy with `blend_stream` counted without vertices; L5 a mixed set making its whole instance non-opaque (measure it, or the
  overlay and alpha geometries non-opaque in the BLAS); L8 the rays' builds feeding the raster's programme diagnostics.

- **Run `i117-t5f`** (build-35, the High Wall alone, 09:18-09:20, the OLED, no fault; the coordinator's challenge: is the
  remainder really the grass?): 95.69 % agree (23,974,852; close 393,264, nearer 349,368 of which clockwise 193,095, farther
  323,832, missed 12,581, extra 14,691); the disagreeing pixels an alpha test met, bounded now (M3): 1,009,199 of 1,079,045
  (93.5 %); outside them **99.71 %**: the alpha-tested geometry (the grass first) holds the High Wall's gap, as INFERRED; why the
  alpha test disagrees (the wind's rest pose, the top mip, no dither) is not told apart. The programme sets: 1,045 built in the first
  600 walks, 86.7 ms in all, 8.6 ms in one walk at most (a single build past the 1 ms bound), every instance classed and no
  `unknown` alpha slice by the 600th walk; the walk 1.35 ms mean afterwards.
- **Step review of the final review's fixes** (fresh Opus, high, `c3da3c17..306781d6`): nothing high or medium. Answered (build-36):
  L1 a failed build's record holds its layout weakly (a freed layout whose address a new one takes never matches), L2 the expired
  records and those of freed layouts pruned each walk, L4 the alpha bound's tolerance from the farther surface. Carried into T5b: L3 a
  deferred entry rebuilt whole at every walk while it waits (to read on `alpha_made` and `programs_ms` there).

## T5b, the walk's cost (2026-11-02, branch `claude/zen-benz-ce4cac`)

- **The question** (the T5 thread's handoff): the rays' walk took 2.80 ms on the engine's thread at m45 in `i117-t5c` (0.76 at
  Firelink, 1.31 at the High Wall), 1.70 of them the serial engine traversal (`decal_world_query_original`) with every entity's
  gates and candidates made inline, the rest the instances made one candidate after another.
- **Not the static cache of R2-PLAN T1, the main walk's pool instead** (a deviation, said here): a cache of the static pieces
  between frames must still read each frame what decides whether an entity is drawn (its enable flag, the visibility words the
  display groups set, the category against the disp service, the suppress byte, its placement), which is the traversal's own
  work: a cache keyed on area loads alone would leave an event-enabled piece out of the shadows until its next load. Elden Ring's
  answer (`GXRtWorld`, models registering themselves) needs registration hooks DS3 does not have. The main walk already runs the
  same traversal on the admission pool (I-0077, ER's `GXLayerTraverseJob` and `GXEntityDrawParallelJob`): the rays' walk now does
  too.
- **The change** (`12_scene_collect.inc`): `CollectNativeRtInstances` calls `WalkNativeLayers` (the layers' traversers gather, the
  entities' gates and candidates made in chunks on the pool, joined in the walk's order: the serial walk's list), with a
  `NativeRtWalkShare` that sets the thread-local `native_rt_gates` and `native_rt_walk_failed` on every job (an entity whose visit
  fails is skipped alone on a worker too, never the game closed) and gathers the gates; its traversers are `uncounted` (their
  visits counted for the layers' order, not against the scene's object bound). Then the static candidates are made into instances
  on the pool in chunks of 64 (each chunk its counts, merged by `NativeRtWalkCounts::Add`; the ids given at the join), the skinned
  ones on the engine's thread after them as before; the programme builds' budget (16 and 1 ms a walk) shared by the chunks
  (`NativeRtProgramsBudget`). Rows: `native_rt_walk` gains `static_ms_mean` and `skinned_ms_mean` (build-39).
- **Step review** (fresh Opus, high, `e0fd6d6c..db2f9c3d`): nothing high. Answered (`b6705477`, build-38): M1 a failure in
  `WalkNativeLayers` (now thrown for the rays' walk) leaked the traversers still alive (now destroyed on any unwinding, the
  reflection walk's too); M2 the rays' traversers did not count their visits, so the layers lost their largest-first order (now
  `uncounted`: visits counted, not the scene's bound); L2 nothing compared the rays' parallel list with the serial walk (now
  `record-revalidation.flag`'s guard for the rays' walk too, `native_main_walk_check` with `"walk":"rays"`); L3 the chunks
  allocated each walk (now kept between walks, cleared) and `traverse_ticks`' meaning (now WalkNativeLayers' whole: setup,
  traversal, gates, join). Kept: L1, the programme budget handed out by the chunks' scheduling, not the walk's order: while builds
  are deferred (a zone's first walks) which instances are classed in a walk may differ from the serial walk's, two chunks may build
  one layout's set twice (counted twice) and up to one build a worker may start under the 1 ms bound; it settles within a walk or
  two (the exit's equality holds once nothing is deferred).
- **Runs**: `i117-t5g` (build-37, the ASUS, 10:28-10:33, m45, Firelink, the High Wall, `rt-scene` and `rt-view`, restored by hash)
  showed a defect the unit tests could not: the pool's jobs named the walk's `thread_local` candidates inside their lambdas, each
  worker reading its own empty list, so only the engine thread's chunks made instances (m45 60 of 3,479, Firelink and the High Wall
  0; the control view missed). Fixed in build-38 (the jobs reach the lists through references taken on the engine's thread).
  `i117-t5h` (build-38, 10:50-10:54, the same protocol): the same totals as the serial walk's (m45 3,675 candidates, 3,479 static
  instances and 182 skinned candidates, Firelink 243 and 257, the High Wall 666 and 285, the same gates, `no_storage`; m45's
  `forward_only` 10 and `uncast` 3 as the later serial runs `t5e` and `t5f`, not `t5c`'s 8 and 1); the lists themselves not compared
  (INFERRED equal: no run with `record-revalidation.flag`, whose guard compares the candidates, not the instances or masks), the control view's agreement as T5's (m45 99.97 %, Firelink 99.77 %, the
  High Wall 95.68 %). The walk on the engine's thread (`ms_mean` over the row's window, then `traverse_ms_mean`):

  | Zone | T5 `i117-t5c` (serial) | T5b `i117-t5h` (pool) |
  |---|---|---|
  | m45 | 2.80 (1.70) | 1.68 to 1.79 (0.60 to 0.66) |
  | Firelink | 0.76 (0.20) | 0.68 to 0.71 (0.12) |
  | The High Wall | 1.31 (0.40) | 0.99 to 1.07 (0.17 to 0.20) |

  Both series ran with the debug layer on (`baseline.json`: `debug_layer_disabled` false, `measure` false: these runs' default),
  a whole-PC slot without builds; the walk makes no D3D call, but the figures are not a measure in the D-0209 sense. The
  traversal and gates fell by two thirds; the rest (about 1.1 ms at m45) barely moved: where it goes (the static chunks or the
  skinned candidates on the engine's thread) is what build-39's two new means tell.
- **The shares** (build-40, `i118-r5b-raster-*` and `i117-t5j-pix-m45`, 11:36-11:44, the ASUS, the debug layer on; ms on the engine's
  thread, the means of the row's window): Firelink 0.64 = traversal 0.11 + static 0.08 + skinned 0.44; the High Wall 0.97 = 0.16 +
  0.19 + 0.60; m45 1.65 = 0.58 + 0.70 + 0.35. Off Firelink and the High Wall the skinned candidates (one after another on the
  engine's thread: their inputs, layouts, pose and slices read each walk) weigh most; at m45 the static chunks still take 0.70 ms
  for 3,479 instances on the pool, more than the parallel work should: INFERRED the one lock of the alpha entries
  (`native_rt_alpha_lock`, taken by every instance) serialising the workers, not measured. Next for the cost: the alpha entries'
  lock sharded or read shared, then the skinned candidates' reads kept between walks while their inputs are the same.
- **The GPU's share** (PIX, `i117-t5j-pix-m45`, the capture at frame 3060 replayed by pixtool, `save-event-list` with « TOP to EOP
  Duration », `evidence/I-0118/artifacts/pix-3060-events.csv`; a replay's times, one event after another, not the live frame's
  overlap): ten `BuildRaytracingAccelerationStructure` in the frame, 0.92 ms in all: the top level (3,479 static and 139 skinned
  instances) 0.19 or 0.30 ms (the two largest; which is the top level is not named in the list), the skinned refits 26 to 93 µs
  each; the skinning dispatches 0.15 ms with the frame's other dispatches. The BLAS compaction and the builds at a load were not in
  this frame. R2-PLAN T1's GPU target (0.5 ms for the top level) is met, the refits together about 0.5 ms more; its CPU target
  (the walk and the build under 1 ms at Firelink and m45) is met at Firelink (0.64), not at m45 (1.65 to 1.79, the debug layer on). No Mega Geometry
  case from these (D-0238): the structures' cost is under 1 ms a frame at the heaviest witness zone.
- **A fault under PIX** (`i117-t5j-pix-m45`, frame 3663, right after the second capture): the debug layer named
  `EmitRaytracingAccelerationStructurePostbuildInfo` on a BLAS address « in a heap with no placed resource containing it », and the
  render worker turned the message into a failure (the game closed, a dump, restored by hash). No run without PIX ever showed it (T0
  to T5b, the debug layer on in every one); INFERRED a capture's effect on the placed resources' tracking, not proved. Answered
  by T7 (RESULT « T7 »: the calls are not the module's; the debug layer off under PIX).

## T5c, the walk's cost again (2026-11-02, branch `claude/zen-benz-ce4cac`, after T5b's merge `a8b03f3a`)

- **The alpha entries' lock**: the entries (by model object and channel) in 16 shards, each its lock (`NativeRtAlphaShard`), the
  walk's chunks no longer queue on one mutex; `static_workers` in the walk's row (the threads that took a static chunk). With the
  fixes' step review's carried points: a walk's first alpha set is kept only when complete (not deferred, no texture withdrawn),
  `Close` drains once after a fault, the kept chunks are cleared on any exit of the walk.
- **Runs `i117-t5k`** (build-44, the ASUS, 13:23-13:26, `rt-scene.flag`, the debug layer on as before, the whole PC free; ms on the
  engine's thread, the row's window): m45 1.23 to 1.29 (T5b's 1.65 to 1.79): traversal 0.55 to 0.60, static 0.30 (0.70), skinned
  0.35; the High Wall 0.87 to 0.89 (0.97): 0.16 to 0.17, 0.09, 0.61; `static_workers` 8 (the caller and the pool's seven) in every
  walk. The lock was the static chunks' cost (OBSERVED by the drop). From the serial walk's 2.80 ms at m45 this morning, 1.23 now:
  R2-PLAN T1's 1 ms CPU target still not met at m45; the skinned candidates (one after another on the engine's thread, their inputs
  read each walk) are now the largest share off m45 and the next for the cost.
- **The skinned candidates** (`c688d7ae`, build-45; not run): the first candidate of each model object and pose made on the pool
  (NativeRtSkinnedOf, NativeRtAlphaOf, the mask, each its own counts), the walk's order then replayed on the engine's thread as the
  serial loop (the duplicates' masks joined, a failed first's later candidates with other inputs made there, the ids).
- **Step review** (fresh Opus, high, `47a1b039..c688d7ae`): nothing high; no race, deadlock or reachable Refuse found; the k-th
  first of the pool's phase the k-th met by the replay. Answered (build-46): M1 two poses of one model object share its alpha entry
  (by model object and channel) and its programme builds, so made at once they built it twice and could end with different sets
  (now one job a model object, its poses' firsts in the walk's order); L1 the shard and the maps' buckets read the address's
  constant low bits (now a mixed hash, the shard from its top bits); L2 the comments (the threads that change an entry); L3 this
  record. Kept: a failed first's retry spends the programme budget after every first (not in the walk's order): it settles in a walk.
- **Runs `i117-t5l`** (build-46, the ASUS, 13:58-14:01, `rt-scene.flag` and `rt-view.flag`, the debug layer on, the whole PC free):
  m45 1.20 ms (traversal 0.60, static 0.29, skinned 0.28 from 0.35), the High Wall 0.78 ms (0.16, 0.08, 0.51 from 0.61);
  `static_workers` 8; the same totals as before (m45 3,479 static instances, 182 skinned candidates, 139 skinned instances, 146
  poses, `forward_only` 10, `uncast` 3; the High Wall 666, 285, 257, 237, 5, 3) and the control view's agreement unchanged (m45
  99.97 %, the High Wall 95.66 %). Over the day: m45 2.80 to 1.20 ms on the engine's thread, the High Wall 1.31 to 0.78, Firelink
  0.76 to 0.64 (T5b; not run since). R2-PLAN T1's 1 ms CPU target met at Firelink and the High Wall, not at m45 (its traversal's
  0.60 ms: the engine's tree walked whole for 3,675 entities); m45's 1.20 ms taken as T1's figure there (R2-PLAN T1 lets the pilot
  set the target from the measure; said to Thomas with STATE), the next costs being the GPU's.
- **Final review** (fresh Opus, xhigh, `main...30318845`): nothing high or medium, nothing blocking; the totals of `i117-t5k` and
  `t5l` found identical. Answered (records and comments only): L6 `static_workers` 8 « in every row » (one walk in 600 sampled); the
  lock's share attributed, not observed, by the High Wall's pair without PIX (static 0.19 ms in `i118-r5b-raster-m30`, 0.09 in
  `i117-t5k-m30`; m45's 0.70 came from a run with PIX attached); the agreement within 0.02 point (95.68 to 95.66 %); the High Wall's
  1200 row 0.935 ms; L7 the conclusion, the plan table and the evidence; T5's final review L4 closed by the unit test (build-47: a
  blended triangle from the second stream traced at its half-way row); L8 two comments. Carried into the next tranche (perf, low):
  L1 the shards a copy of `NativeEntityShards` (16, not cache-aligned) and L2 `NativeRtSkinKeyHash` unmixed for the other maps (both done with I-0118's cascades removal, build-54); L3 the
  skinned phase's vectors allocated each walk, and one job a model object capping the gain (time the pre-pass, the pool and the
  replay apart first); L4 the replay's duplicate and retry branch never met live (`duplicates` 0 in both series: equal to the
  serial code by reading); L5 `Close` marks Streamline's drain done before it returns (a drain that throws skips the second; done with I-0118's own gate, build-50).

## T6, the wind's grass (2026-11-03, branch `claude/vigorous-kepler-99a438`)

R2's open point at the High Wall: `GrassSpcBump` 11.7 steps of albedo off in the rays, bias -1.7 (I-0119 « R3c fourth tranche »),
INFERRED the wind the rays did not apply (R2-PLAN T2: « the wind's grass in its rest pose first »). With the traced sun shadow the
default (I-0118), a grass in its rest pose also casts a shadow that does not sway with the raster's grass.

- **The wind read** (OBSERVED, `evidence/I-0117/artifacts/t6-wind-census`): of the 695 G-buffer vertex programmes of the I-0063
  cache, 19 read cbvModelParam's `VC_ForceParam` (`GXFlver_Grass*_GBuf`, `_GBuf_Skin`, `Eclipse_grass_Gbuf`; the `_Ins` ones read
  `VT_aForceParam`, engine instancing, refused at admission, I-0110 §2.1), one body once the registers are normalised: after the
  placement (or the pose), the world position moved in x and z by up to 8 directional and 8 point winds (their force, time, place and
  range, gust noise, direction and attraction rows), each weighted by the vertex's TEXCOORD1 (x a phase, y the bend's weight, z a
  time scale) and scaled by 1 / sqrt(VC_ModelSizeParam.y), then leaned on a sphere (x and z a / sqrt(a² + 1), y lowered) by
  TEXCOORD1.y × VC_ModelSizeParam.y. Its literals read by their bits (`fxc /Lx`). The raster binds the block as the animation block
  [anim+210] when [anim+620] is set, VC_ModelSizeParam the draw group's +0x60 (`13b_prepare_model_frame.inc`).
- **Written** (no ER analogue read: Elden Ring's handling of wind in its BLAS was not studied; the design follows R2-PLAN T2's
  skinning): `rt_skin.hlsl` moves a vertex by the wind after its placement or pose when its record names TEXCOORD1 (float3, half4, or 8-bit UNORM since build-59,
  slot 0), from a wind block written after the instance's world row (a moved position not finite keeps the placed one, which a refit
  can keep); `NativeRtSliceWind` marks a slice whose G-buffer vertex programme reads `VC_ForceParam` (with VC_ModelSizeParam and
  TEXCOORD1.xyz) in its material set; a placed candidate whose set has such a slice, while its wind blows (`NativeRtWindBlockOf`: the
  flag set, a count above 0, a positive size), becomes an instance with its own positions and BLAS (`NativeRtWindedOf`: every slice
  the storage's BLAS takes, placed by the world row, mode 3; kept with the skinned ones by model object and pose), skinned and
  refitted when its block or placement changed (each frame while the wind's time runs); a posed candidate's wind slices are moved
  after their pose. A calm set stays a static instance of the storage's BLAS. Rows: `native_rt_skin`'s `wind` (the walk's instances
  moved, calm, wind slices, not read) and `winds`, `wind_vertices` (the renderer's since the start). Unit test (`rt_scene_test`): four
  quads (TEXCOORD1 as float3, half4, R8G8B8A8_UNORM and B8G8R8A8_UNORM, build-60) under one directional and one point wind, the skinned positions read back equal to a CPU
  transcription of the programme within 2e-3 m, nothing skinned again for the same block, the placed positions without one.
- **Runs** `i117-t6a-{m30,m31,m40}`, `i117-t6a-1080-m30`, `i117-t6a-odd-m30` (build-58, 08:58-09:04, the ASUS, offline, restored by
  hash; 2,940 to 3,023 frames, no fault): **no grass moved** (`wind` [0, 0, 0, 25] at the High Wall, 35 at the Undead Settlement:
  every wind slice unsupported), so the grass unchanged (`GrassSpcBump` 11.66 steps, bias -1.6 at call 1200; the control view
  93.5 %). The cause read in I-0110's FLVER census (`flver-m30.tsv`): the grass's TEXCOORD1 is the FLVER `UV1:Byte4C`, an 8-bit
  UNORM, which the first list (float3, half4) did not take. Now R8G8B8A8_UNORM and B8G8R8A8_UNORM too (build-59), and the first 8
  slices left unsupported named (`native_rt_wind_unsupported`).
- **Step review** (fresh Opus, high, `main...11b11459`): nothing high; the HLSL and the C++ transcription matched the DXBC bit for bit
  (the literals, the signed frac, both loops' masks), the block's layout and the GPU states sound. Answered (build-59): M1 a placed
  instance moved by the wind lost its vertex frame (a skinned instance shades with its triangle's normal): its placement's rows now
  ride in its instance row (`RtInstanceRow::frame`, 128 bytes a row) and rotate its records' NORMAL and TANGENT as a static
  instance's (the Grass programmes rotate the frame by the placement alone); L2 a posed instance's wind slices marked whether the
  wind blows or not (no new BLAS at each calm); L3 a model object met again with another placement keeps that one as a static
  instance (`wind` words 5 and 6 count the duplicates and those); L5 the comments. Kept: M2 the cost, measured in the next runs (the
  instances moved, their vertices, the skinned structures' memory); L1 the material set now read before the skinned candidate's
  own gates (its tallies count the candidates that stay out; its programme builds share the walk's budget), recorded; L4 the placed
  instances moved by the wind counted with the skinned ones (`native_rt_skin` `instances`), not in `native_rt_walk` `instances`.
- **Runs** `i117-t6b-{m30,m31}`, `i117-t6b-1080-m30`, `i117-t6b-odd-m30` (build-59, 09:23-09:28, the ASUS, offline, restored by hash;
  2,913 to 3,000 frames, no fault). OBSERVED, the same calls and sizes as `i117-t6a` (build-58, the rest pose):

  | Run | Wind (moved, calm, slices) | Grass family: albedo error, within 8, bias | Control view agreement |
  |---|---|---|---|
  | m30 1280x720 | 9, 14, 25 | `GrassSpcBump` 11.66 → 6.96, 0.58 → 0.75, -1.6 → -0.8 | 93.50 → 96.51 % |
  | m30 1920x1080 | 9, 14, 25 | 10.53 → 5.55, 0.59 → 0.78, -0.1 → 0.0 | 94.15 → 97.59 % |
  | m30 1366x803 | 9, 14, 25 | 11.66 → 6.65 | 93.47 → 96.47 % |
  | m31 1280x720 | 26, 3, 35 | `Grass` 9.53 → 8.84; `GrassSpcBumpDOL` 8.80 → 7.59 | 97.89 → 98.19 % |

  The grass's normal 23° → 20° off at the High Wall (its vertex frame read again, M1). Cost (OBSERVED, the rows): about 58,000 vertices
  skinned a frame at either zone (`wind_vertices` over the run's frames), the skinned structures 140.5 → 150.1 MB at the High Wall and
  175.9 → 243.5 MB at the Undead Settlement (each moved grass its own positions and an uncompacted, updatable BLAS: 2.6 MB a placement
  there), the walk on the engine's thread 0.94 → 0.99 ms (High Wall) and 0.68 → 0.69 (Undead Settlement); the GPU time of the skinning
  and refits not measured (no PIX in these runs); no view refused by the heap (`full` 0). The calm sets (14 at the High Wall: their
  wind block unset or without a wind counted) keep their rest pose, as the raster draws them. Sheet
  `evidence/I-0117/artifacts/t6-wind-sheet-m30-2400.png` (the presented frame, the traced albedo, the differences before and with
  the wind), shown to Thomas: little to see by eye, the gain is in the numbers. Left at the High Wall: the grass's 7 steps and 20°,
  INFERRED its alpha test at the cone's level and the wind's phase between the walk's read and the raster's frame (not told apart).
- **Step review of the fixes** (fresh Opus, high, `11b11459..2119e6de`): nothing high or medium; the row's layout agreed in C++ and
  every shader, the frame zero for every other instance, the UNORM8 and BGRA decodes D3D's. **Final review** (fresh Opus, xhigh,
  `main...95871368`): nothing high; the transcription re-read instruction by instruction, the RESULT's figures matched the rows.
  Answered (build-60): M1 a moved grass that cannot be skinned (a placement refused, a failed structure) was left out of the rays: it
  is now traced in its rest pose (`RtSkinnedInstance::rest`, the storage's BLAS at its placement, as before T6); M2 STATE's row;
  L2 (and the step review's L2) the `native_rt_wind_unsupported` rows behind `rt-view.flag`, with the family; L4 a posed entry made
  again when it has no rows; L5 the offset's sum in 64 bits; L7 a fourth quad (B8G8R8A8_UNORM) and the rest-pose fallback in the
  unit test, the redundant line removed; L8 the records (R2-PLAN T2, I-0119's next step and its hair); the step review's L1 the
  allow-list's entry, L3 the comment. Recorded, not changed: L1 the material set read before the skinned candidate's gates (its
  tallies and its programme builds count candidates that stay out; `wind_calm` now counted after them); L3 a merged duplicate's
  mask tallies; L6 a grass whose model size is not positive is traced in its rest pose where the raster's NaN drops it (D-0197's
  grey zone: the rays keep what the raster cannot place); L7 the cone, ranged and 8-wind cases and the framed shading not unit-tested
  (the framed shading read by the runs: the grass's normal 23° → 20°); the step review's L6 (which of two placements moves follows
  the walk's order; `wind_differing` was 0 in both series). The answers' step review (fresh Opus, high, `95871368..b1840230`):
  nothing high or medium; the rest instance takes the static path everywhere, no double instance, the BGRA reference sound.
  Answered: L2 the comments. Recorded: L1 a grass traced in its rest pose counted with the left out (`left_out`); L3 the unit test's
  rest instance not checked for a zero frame or its mask, the 64-bit sum not unit-tested.
- **Optimisation pass** (read, the branch's additions): nothing read or computed twice; a calm grass stays on the storage's shared,
  compacted BLAS; a moved one is skinned only when its block or placement changed (each frame while its wind's time runs, which
  the programme's sway needs).

## T7, the fault under PIX (2026-11-03, branch `claude/happy-yonath-7a436a`)

- **The question** (T5b's open point): `i117-t5j-pix-m45-on` closed at frame 3663, three frames after the second PIX capture
  (presented frame 3660), on the debug layer's error from `ID3D12CommandList::EmitRaytracingAccelerationStructurePostbuildInfo`
  (« pSourceAccelerationStructuerData[0] 0x00000011b7a00000 belongs to a Heap ... however there are no Placed Resources fully
  containing the GPU Virtual Address range »; its `crashes/startup-crash1.txt`), which `SceneRenderer::Diagnostics` counts as an
  error and the render worker turns into a failure. LIVE-RUN-NOTES recorded it twice (T5b and R5a).
- **Observed** (the code, and a step review's independent sweep): the module never records that call; its compacted sizes come
  with each build (`pPostbuildInfoDescs` of `BuildRaytracingAccelerationStructure`, `rt_scene.cpp`); Streamline does not wrap the
  device. So that call was PIX's capturer's own, made during a capture. A frame's instance records take their BLAS from
  `RtModelBlas::Structure()` held by the frame's input until the frame completed; the compaction swaps a structure only once its
  copy is committed, the copy list holding the large one; placed ranges go back against the last submitted ticket.
- **First form** (`e0280111`, `70b90f70`, build-t7-2; the step review's L1 to L3 answered): the message counted as the capturer's
  after a capture was asked for, by its ID 1159. **Run `i117-t7-pix-m45-on`** (12:31-12:33, m45, the ASUS, `rt-scene` and
  `pix-capture`, restored by hash): closed at frame 3063 after the first capture (3060) on the same message: it is not reported
  under ID 1159. **Second form** (`76c980db`, build-t7-3): the message known by its call's name. **Run `i117-t7b-pix-m45-on`**
  (12:49-12:50, the same protocol): closed at frame 3063 on the next message, a `CopyRaytracingAccelerationStructure`:
  « SourceAccelerationStructureData 0x00000011ca1c0000 belongs to a Heap ... no Placed Resources fully containing ». The module
  was not compacting then: `native_rt_blas` reads 651 models, 651 ready, `pending` 0 from frame 1860 to 3060 (the module's one
  copy, `RtBlasRegistry::Advance`, needs a model built since; none was). INFERRED both calls the capturer's, saving the
  acceleration structures at its capture (first their sizes, then a serialising copy; the Emit message swallowed by the second
  form's filter, the copy came next). (A first reading credited the copy to the compaction; the final review showed the counters
  contradict it.)
- **Not every capture with the layer failed**: `i117-t5h`'s at 3060 (651 models, the top level) passed, `i117-t5j`'s first
  passed and its second failed, `i117-t5i` took four (253 models, no top level). Two explanations stay open, not discriminated:
  the layer loses track of the module's live placed structures at a capture, or the capturer works on addresses the module
  legally released (the large structures after their compaction: 577 MB built against 246 MB compacted), where the layer is
  right. Either way the calls are not the module's (to tell them apart: the large structures' GPU addresses written at their
  release, compared with a message's).
- **The change, third form** (`9fb0daa5`): a filter message by message would be a sieve: the filter is removed, and the module
  no longer turns the debug layer (nor GPU-based validation) on with `pix-capture.flag` (`20_startup_phases.inc`; the
  `native_debug_layer` row gains `pix_capture`). A capture's own validity is PIX's replay to judge (not done for these: the
  captures were deleted unread, « taken » rests on `hr` 0).
- **Run `i117-t7c-pix-m45-on`** (build-t7-4 = `9fb0daa5`, 13:07-13:10, the same protocol): `native_debug_layer` requested false,
  `pix_capture` true; the four captures taken (frames 3060, 3660, 4260, 4860, `hr` 0, each about 1.6 GB, deleted after the run);
  651 BLAS built and compacted (`ready` 651, `failed` 0); the game played on to frame 8098. It ended at the walk's close on the
  access violation in `WinPixGpuCapturer.dll` under the static destruction of `native_reflection`'s textures at the module's
  detach (ExitProcess), the exit fault I-0100 RESULT tranche 2 already records for capture runs: not T7's, left as it is. Its
  `baseline.json` says `debug_layer_disabled` false (the runner's fix came after it): it is not a validation run.
- **Step review of the third form** (fresh Opus, high, `82cbf8e8..9fb0daa5`): nothing high or medium; the filter's removal
  complete; no lifetime or containment fault in the compaction (hundreds of compactions without PIX and with the layer, no
  message). Answered (`ef5d806d`): L1 a run with `pix-capture` still recorded the debug layer on in its baseline, so a validation
  replay could count it as validated: `Run-StartupTrace.ps1` now runs it as `-NoDebugLayer` (and refuses it with `-GpuValidation`).
- **Final review** (fresh Opus, xhigh, `main...ef5d806d`): nothing high. Answered: M1 the copy credited to the compaction (above,
  and the code comment and LIVE-RUN-NOTES rewritten); M2 a `pix-capture.flag` left by a killed run would drop the layer of a
  validation run unseen: the runner removes it as a stale flag, and the module refuses a layer asked for beside it
  (`debug_layer_with_pix_capture`); M3 STATE's rows rewritten; L1 the spot's reading and the gloss clamp's row in I-0119
  corrected (RESULT « The gloss », « The Fresnel », R3-PLAN); L2 the baseline sentence above; L3 the runner refuses `-Measure`
  with `pix-capture`; L4 WORKFLOW's PIX row, T5b's open item, the conclusion. Kept: L5 the flag read in four places (one global
  would be tidier; the rows show the state) and the layer dropped for every PIX run, broader than the evidence, chosen.

## Evidence

- T7: `evidence/I-0117/artifacts/build-t7-1` to `build-t7-5` (build-t7-2 the `i117-t7` run's, build-t7-3 `i117-t7b`'s, build-t7-4
  `i117-t7c`'s); runs `evidence/I-0066/artifacts/i117-t7-pix-m45-on`, `i117-t7b-pix-m45-on`, `i117-t7c-pix-m45-on` (their log
  `runs-t7.log`; the captures deleted); the launcher's log `launcher-1ad14b19.log`.
- T6: `evidence/I-0117/artifacts/build-58` (`11b11459`, the `i117-t6a` runs'), `build-59` (`2119e6de`, the `i117-t6b` runs'); the
  wind census `t6-wind-census`; runs `evidence/I-0066/artifacts/i117-t6a-*`, `i117-t6b-*` (their log `runs-t6a.log`, `runs-t6b.log`;
  the scripts `scripts/Run-T6ZoneTour.ps1`, `Run-T6Series.ps1`); the sheet `t6-wind-sheet-m30-2400.png`.
- T5c: `evidence/I-0117/artifacts/build-44` (the shards), `build-45`, `build-46` (the skinned firsts on the pool, the runs'),
  `build-47` (the second stream's unit test); runs `evidence/I-0066/artifacts/i117-t5k-{m45,m30}-on`, `i117-t5l-{m45,m30}-on`.
- T5b: `build-37` to `build-43`; runs `i117-t5g-*`, `i117-t5h-*`, `i117-t5j-pix-m45-on`; `evidence/I-0118/artifacts/pix-3060-events.csv`.
- T5: `evidence/I-0117/artifacts/build-27` to `build-35` (build-27 the first run's, build-29 = `194a1dee`, build-31, build-33 the
  faces', build-34 the final review's tree, build-35 its fixes, build-36 their step review's); runs `evidence/I-0066/artifacts/i117-t5a-{m40,m30,m45}-on`,
  `i117-t5b-*`, `i117-t5c-*`, `i117-t5d-m40-on`, `i117-t5e-m40-on`, `i117-t5f-m30-on`, their `rt-view-*.rgba` pictures, `rt-view-3000-*.png` converted.

- `evidence/I-0117/artifacts/build-1`, `build-3`, `build-4`, `build-5` (the module and its unit tests; build-3 = `f454c6a6`, build-5 =
  `899af32e`, build-8 = the final tree).
- Runs `evidence/I-0066/artifacts/i117-t1a-on`, `i117-t1b-on`, `i117-t1c-on` (T1); `i117-t2a-on` (T2, build-12).
- T2: `evidence/I-0117/artifacts/build-11`, `build-12` (= `4ff04d39`), `build-13` (the final tree); the vertex programme census
  `t2-vs-census` (`skin-vs.tsv` SHA-256 F4BF5E6B…1D52).
- Runs `evidence/I-0066/artifacts/i117-t0b-on`, `i117-t0b-off` (and the title-only `i117-t0-on`, `i117-t0-off`).
- T4: `evidence/I-0117/artifacts/build-23` to `build-26` (build-25 = `43a4254a`, the runs' module; the final tree in the launcher's rebuild, start check `start-98fa3a4b`); runs
  `evidence/I-0066/artifacts/i117-t4b-on`, `i117-t4b-m30-on` (the result) and `i117-t4a-on`, `i117-t4a-m30-on` (left out: the screen's
  name, Thomas's inputs), their `rt-view-*.rgba` pictures, `rt-view-3300-*.png` converted.
- T3: `evidence/I-0117/artifacts/build-14` to `build-21` (build-14 = `d597c6d3`, build-17 = `94f52cbc`, build-19 = `f6527d36`,
  build-20 = `e35711d8`, build-21; build-22 the final tree); the alpha census `t3-alpha-census` (`alpha-ps.tsv` SHA-256 16FD19F5…C107);
  runs `evidence/I-0066/artifacts/i117-t3a-on`, `i117-t3a-m30-on` (build-17), `i117-t3b-on`, `i117-t3b-m30-on` (build-19).

## Closed (2026-11-09)

Closed 2026-11-09 with Thomas's yes (relayed by the coordinator, 01:55 Paris): the open points below are carried to [I-0132 R11-PLAN §6](../I-0132-rt-optimisation-zones/R11-PLAN.md#6-the-open-points-carried-from-the-investigations-closed-on-2026-11-09), where each has its place.

## Uncertainty / unresolved questions

- The BLAS memory at the other witness zones, the build's GPU time and any frame-time effect are not measured; one zone, one
  resolution (1280x720).
