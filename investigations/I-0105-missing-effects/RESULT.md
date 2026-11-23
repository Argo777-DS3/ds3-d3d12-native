# I-0105 result: the missing effects

FINISH-LINE line 5, opened 2026-10-28, in tranches merged one by one (D-0227). Plan and order: [CONTRACT](CONTRACT.md).

## Conclusion

Complete (2026-10-30, tranche 10 the last; D-0235: I-0105 finishes before the RT workstreams). Tranche 10 merged `776f9fd2` (08:31, Thomas's « Fusionne »); the launcher rebuilt (108 PASS, `evidence/I-0105/launcher-build-11.log`); the start check `start-776f9fd2` (08:39-08:40) passed. Merged, each with its start check:
the E03 model particles (`689dfae0`), the trails 606 and 10012 (`3e61b30d`), the burn look (`64658f38`), the spell runs' witnesses
and the GPU particles' depth size (`3927ab10`), the blood's fade (`d391b839`), Lightning Spear's E03 models admitted (`96407cc2`),
the E03 draws with the effect context's blend and depth (`498d1d50`), the feathered s03000 with its readable depth (`a2a627d8`),
the surface snow's words (`5d332c39`) and its textures t47..t51 (tranche 10, the High Wall's `m30_00_Deposit_*` as the original's).
Lightning Spear, Great Lightning Spear, Lightning Stake and Lightning Arrow drawn as the original's on pictures; Fire Orb, Great
Soul Arrow, Bountiful Light as the original's.

Open, recorded (not chased here): Sunlight Spear and Lightning Storm not witnessed (no character holds them); the GPU particles
against the original elsewhere than Lightning Stake; the light shafts 10003 (no spawner found, skipped and counted; line 8's crash
tour if met); the burn look, the blood's fade and the E03 Estus Flask not seen in play; the snow pieces not yet seen against the
original (no piece in view from the High Wall bonfire) and the snow programme's t34 / t52 stages unbound; 1AC390's paint defaults
(tranche 10's side reading); the spells, infusions and status effects stay counted unfinished (STATE's open gaps).

## Tranche 1: the model particles (E03, FXR 605)

- Read ([ER-E03](ER-E03.md)): DS3 draws each model particle's FLVER through the engine's model draw (`29ADF0` → `2BA3A0` → `2C0130`
  → `636F0` → `63CB0`, the world's mesh loop) on the pass context's model context `+0x460`, null natively (Native2035's fault in
  `149CB0`); Elden Ring reaches the same mesh loop (H2 supported; its other caller a world path, INFERRED). Hence no effect
  statement: a model draw.
- Code: `effect_draw` lets the class `3D65F88` run by default (`no-ffx-models.flag` opts out) and, on the native pass context, skips
  `29ADF0`'s two GX uploads (`59470`, `1A86C0`) and takes `636F0`'s inputs (`ModelSubmission`); `RecordNativeEffects` makes each
  submission's pose from its bone rows (`ComposeNativeModelPose`, now shared with the engine's pose updates); the main collect joins
  the effect record before the main walk's admission and admits each submission of pass 0x29 as a candidate of kind 3 (H1), drawn as
  the forward row 0x29 of category 0x29 (class mask 0xFFFF, shader pass 0, as `63CB0` with `236290`'s open masks). A witness row
  `native_effect_models` (the first three collects with model particles and every 600th) counts them and names the models.
- Not drawn, counted: the model particles of the passes 0x21, 0x2D, 0x20 and 0x31 (no native model rows there) and those with their
  own material table. Not settled by reading: whether the FFX bundles' FLVERs reach the native model storage (EFFECTS-E03 risk M2;
  a refusal shows as `missing_geometry` in the witness row), and whether the Estus Flask in the hand is one of them (CENSUS §6,
  INFERRED). Both are read from the start check's journal after the merge.
- Cost: the effect record is joined before the main admission instead of before the publication (it ran beside the walk and the
  admission; now beside the walk alone). Not measured (D-0227).
- Step review (fresh Opus, high, `c8b5fc7e..669b554f`): nothing blocking. Applied: the particle's own light (29ADF0 writes the FXR's
  light into the arguments after 1A8580, args+0x3E0..+0x424, and 59470 takes it instead of the state's banks: the submission now
  carries it and the instance lighting takes 59470's branch for it, `state_owned`); the identity of a particle (one renderer's bone
  array serves every particle of an appearance: offset by its rank among the frame's draws of that array); the runner's second flag
  group (`no-ffx-models`); the executing context atomic; a failed palette publication skips the particle instead of closing the
  game; the witness counts the admitted particles with forward rows, with their forward light (`ProduceNativeForwardLight` over the
  state's gparam keys; a particle without it draws nothing) and with their own light. Left, recorded: the forward cbLight/cbFog stay
  the state's gparam keys' (5A120's, not read further: whether the FXR keys resolve shows in `lit`); the bone rows read are the
  model's `+0x1C`, the array holds the instance's `+0x90` (equal unless an instance holds fewer); the particles are drawn among the
  world's forward rows 0x29, not at their effect key (the key order with the effects is FINISH-LINE line 7).
- Proof: clangd without diagnostics on the changed units (`effect_draw.cpp`, `game_bootstrap.cpp`, `producers.cpp`); the MSVC
  build and its unit tests pass (106 PASS rows, no failure, `evidence/I-0105/build-4.log` on the final tree, `build-3.log` after the step review, the first tree in
  `build-1.log`); the model preparation control (`Test-GameModelPreparation.ps1`, run into this branch's own folder) passes with its
  new check of the state-owned light; `Test-Project` passes. No unit test drives the model particles' admission (it reads engine
  objects): what reaches the frame is read from the start check's `native_effect_models` row after the merge. No live run before the
  merge (D-0227). The CONTRACT's unit tests of the submission's record and the bone palette's space are not delivered: the reading
  is a copy of the decompiled offsets (a test over a fake memory would check the code against itself), and the palette's space is
  INFERRED (ER-E03 §3); the start check's row and a view of a model particle settle them.
- Final review (fresh Opus, xhigh, `main...HEAD` at `94275fe2`): nothing blocking. Applied: the particle's identity made a pure key
  (top bit set: no engine address, so the drawn-bits marks and the records never meet an entity) and the drawn-bits loop skips the
  model particles; the bone rows taken as full rows, not composed with the cloth gate rows (the original hands them raw to 636F0;
  INFERRED either way, written as such); the particles take no light-set slot (no G-buffer row reads envData); the rows dropped
  once the pose holds them; the palette-publication skip told apart by its name constant, not by text; acquire loads; stale
  comments; STATE and the documents (H2 « supported », the per-particle draw named as a known cost, 29ADF0's unread argument
  writes listed). Left: the bone count (above), the per-particle admission's cost (not measured, D-0227; to look at m51 and m45).
- Step review of those fixes (fresh Opus, high, `94275fe2..eec900a9`): nothing blocking. Applied: `Read` refuses an address above the
  user-mode range at once (a particle's key; a guarded copy would have taken an exception per read); the key made collision-free (the
  array's address / 16 and the rank in the low 16 bits); a stale comment; the witness counts the particles whose model has `+0x1E0`
  set (`full_rows`: the bone rows are taken as full rows, the instance's object rows follow `+0x1E0`, consistent only if it is set).
  Build and unit tests: `evidence/I-0105/build-5.log`, 106 PASS.
- Merged `689dfae0` (2026-10-28 21:58); the launcher's module rebuilt from `main` (106 PASS); the start check `start-689dfae0`
  (22:04-22:05) passed: world drawn (366 frames), closed by its close box, restored, no refusal or failure, 47 effect hooks. Its
  effect stage row as the previous check's: 19 pipelines, no skipped draw, no pipeline failure, no missing texture (18,694 draws).
  No `native_effect_models` row: no model particle was drawn by the engine on that path (Reference2037's `s11600` was at Alpha's
  view, not on the start check's), so the native E03 draw is not yet seen; the 33 `native_selected_palette_unavailable` rows are
  the previous check's own (character parts, unchanged).

## Tranche 2: the trails (606, 10012)

- Read (DS3, the pilot, GhidraMCP; the decompiled functions and the reading of `36F280`/`36F350` in `evidence/I-0105/trails/`):
  `3709B0` (606, class `3D7E380`) and `2891A0` (10012, class `3D660B8`) take the draw context `299E40` builds (`25FB80`:
  `[drawCtx+0x20]` the inner GX context `[outer+8]`, `[drawCtx+0x28]` the ring holder `299E40` takes from the scene control's
  `+0xD0` table, the native holder since `Execute` stores it there), set it up through its slots 0x58, 0xA0, 8, 0xB0, 0x20, 0x28,
  0x60 and 27 (264230), and write their strips through `36F280` (the ring's map, `213B690`: the holder's object vt+8; its result
  tested, and the callers write nothing when it fails) and `36F350` (a topology among GX 1-5, 10, 11, the holder's unmap, then the
  GX topology, vertex buffer and draw wrappers on the inner context: each map unmapped before the next). The draw context's slots
  are engine code that already runs natively; 0xA0 (`262A70`) and 0x20 (a texture setter) are new to the native path: `262A70`'s
  only GX call (a CopySubresourceRegion, not hooked: it would trap and be counted) sits behind `[inner+0x59D]`, zero in the native
  inner context. `2891A0` does not read the device member `[[GXRS]+0x4DD0]` (E07's). H3 supported (static). CENSUS's « no ring »
  and « the pass context's slots » for 606 were wrong (corrected there and in I-0104 DESIGN).
  Elden Ring: its appearances include `...Line`, `...QuadLine` and `...Tracer` (RTTI, I-0104 ER-EFFECTS-STUDY §0); I-0104 read
  the billboards' path to its D3D12 device context, not the trails' (INFERRED the same: one device context under every appearance).
- Code: both classes allowed by default (`no-ffx-trails.flag` opts out); the runner's flag groups.
- What a statement the native does not take becomes: an unhooked GX call traps (counted); the frame builder drops a draw with
  topology 0 or above 41 and counts it; GX 10 and 11 become 1- and 2-point patch lists, drawn with a hull and domain programme or
  refused by the stage (`pipeline_failures`); none closes the game.
- Final review (fresh Opus, xhigh, `main...HEAD`, also the step review): nothing blocking in the code; the « no crash » claim rested
  on functions not exported: exported to `evidence/I-0105/trails/` with the map's tested result (its S1); the documents corrected
  (STATE, the index, CENSUS, I-0104 DESIGN, the CONTRACT, K-0485's dated note, the ring holder's source, the topologies). The start
  check after the merge can show only « no crash on its path »: the per-class counts are logged under `ffx-record.flag` alone; its
  effect stage row is compared with tranche 1's.
- Proof: the MSVC build and its unit tests (106 PASS, `evidence/I-0105/build-6.log`); `Test-Project`.
- Merged `3e61b30d` (2026-10-28 22:40); the launcher's module rebuilt (106 PASS); the start check `start-3e61b30d` (22:50-22:51)
  passed: world drawn (339 frames), closed, restored; its effect stage row as before (19 pipelines, no skipped draw, no failure, no
  missing texture, 17,948 draws) and the same two `native_ffx_trap` rows (slots 8 and 9) as the two checks before: no trail met on
  its path, the trails not yet seen.

## Tranche 3: the burning objects' look

- Read ([BURN](BURN.md)): the burn progress reaches the Burn programmes as `FC_BlendRate` (cbMatDynParam +0xC0), from the material's
  GX13 third word moved toward a kind-7 parameter row the burn update 667430 writes into the object's parameter owner (the owner
  the native reads for it: INFERRED); natively neither GX13, GX01 nor kind-7 rows were read, and the Burn programmes' rows were
  likely refused for the unwritten `FC_BlendRate` (INFERRED: the Burn materials not drawn at all).
- Code: GX13 and GX01 parsed at the material's admission; the +0xC0 block made with the kind-7 row's blend. The parameter rows
  already change the model's record (a fresh admission when they change), so the block follows the burn. Expected change: the Burn
  materials' meshes draw (with GX13's value or 0); every other programme's bytes are unchanged.
- Test tooling: `Run-AlphaCast -SpellsPerCatalyst n` (Thomas's keys: Up arrow the spell, Right arrow the catalyst) for the spell run.
- Final review (fresh Opus, xhigh, `main...HEAD` at `0150a182`, also the step review): nothing blocking. Applied: the change to the
  Burn materials said (they draw now, INFERRED refused before), GX01 parsed too (the +0x30 block's other gate), the outer gate and
  the unavailable-snapshot policy commented, the owner's identity INFERRED, Thomas's permission for Alpha moved to the CONTRACT.
- Proof: the MSVC build and its unit tests (106 PASS, `evidence/I-0105/build-8.log`); clangd (the TU builds under /W4 /WX);
  `Test-Project`. Not seen: no run burns an object (Archdragon Peak); the first run reaching a Burn object checks that it appears.

## The spell runs (2026-10-28 23:45 to 2026-10-29 01:44, the ASUS, Alpha from a save copy)

- `start-64658f38` (tranche 3's start check, 23:45): passed, world drawn, restored.
- `i105-spells-1` (native, `ffx-record.flag`, 12 casts): no fault, restored. `native_effect_models` at every sampled collect:
  2 submissions of `s11600`, both admitted, with forward rows, lit, with their own light, full rows: the E03 particles are drawn
  natively. `s11600` is INFERRED the bonfire's coiled sword (`evidence/I-0105/spells/coiled-sword-before-after-reference.png`:
  native before E03, native with E03, the original; the angles differ). No `skipped_renderer_*` count. The Up and Right arrows
  changed nothing: `Pilot-Game.ps1` sent them without KEYEVENTF_EXTENDEDKEY (read as the keypad's), fixed.
- `i105-spells-2` (the same, keys fixed): Fire Orb (pyromancy flame), Great Soul Arrow (staff), Lightning Spear and Bountiful Light
  (talisman) cast and drawn natively (`spells-2-sheet.png`); 28 effect pipelines (21 in run 1), no skipped draw, no pipeline
  failure, no missing texture. Whether they are drawn as the original's needs the Reference run (next).
- `i105-spells-ref` (the original, `ffx-draw-observe`, the same casts; a first attempt refused `-NoDebugLayer` in Reference mode before
  launching anything): side by side (`spells-native-vs-original.png`): Fire Orb, Great Soul Arrow and Bountiful Light alike;
  **Lightning Spear's large branching bolt missing natively** (the defect Thomas saw on 2026-10-19, not a regression; he confirmed
  2026-10-29 00:35), and its ground glow weaker. Thomas, on the native sheet: « ça me semble pas mal, mais à voir en jeu » (an
  impression on pictures, not a validation). The original's E03 models during the casts: `s11600`, `s10002`, `s03000`; natively
  `s10002` admitted too (collect 10200, five submissions).
- `i105-spells-3` and `-4` (witnesses: the trap callers; the commands run and the draws emitted by class and topology): the
  trapped calls (54,701 per slot, slots 8 and 9) are, at their sampled 1st, 100th and 10,000th calls, the GPU-particle simulation
  `2A6A30` (`2A6FEB`, `2A7000`) asking the pass context's depth texture `S+0x24D8` its width and height (a GXTexture2D's vt+0x40 /
  +0x48, `2AA80` / `2AAD0`), which it stores at `S+0x300` / `+0x304`: natively 0 and 0 since E07 (D-0186), a pre-existing gap,
  fixed in this branch (the native depth and copy texture objects answer the scene's size). Topology-11 draws (GX 2-point patches,
  the tessellated tracer programmes) are emitted natively by 606's class `3D7E380` only (2,560 in the run, the original 2,614 in
  its run), no drop, no pipeline failure; that they are Lightning Spear's bolt is INFERRED (no per-cast witness): drawn but not seen.
- `i105-spells-5` (the first eight tracer draws dumped): indexed, stride 80, the first vertex near Alpha (271, -54, 606; INFERRED
  hers), VS b0's and DS b0's first twelve words equal, PS b0's first eight words zero. The tracer programmes (`GXFfxtessellate[Soft]Tracer[Bump]Texture`,
  I-0063's programme cache): the hull shader reads no constant buffer, the domain shader DSCommonBuffer b0, the pixel shader
  PSCommonBuffer b0, PSLightBuffer b1, cbFog b6, t0 diffuse, t1 normal, t3 depth (`tracer_layouts.tsv`). Open: why the emitted
  tracer patches show nothing (the DSCommonBuffer words the domain shader reads, the PS colour words, the depth test of the soft
  variant); the next step compares the same draws in the original (a tracer witness in Reference mode).
- Final review of the witnesses (fresh Opus, xhigh, `main...HEAD` at `558b0c34`): nothing blocking. Applied: the trap reading
  corrected (above) and the depth's size answered; `Pilot-Game.ps1`'s extended keys in `hold` too, through one helper (without
  SELECT, PRINT, EXECUTE, SNAPSHOT); the tracer dump's row buffer widened; the documents (this conclusion, STATE, the index,
  WORKFLOW: the targeted runs are diagnostics, never a merge gate). Build `evidence/I-0105/build-12.log`.
- Step review of the depth-size fix (fresh Opus, high, `558b0c34..fe1ebbf4`): nothing blocking; applied: the width and height
  shifted by the mip as 2AA80 / 2AAD0 do; build `evidence/I-0105/build-13.log` (106 PASS). Merged `3927ab10` (2026-10-29 02:33);
  the launcher rebuilt (106 PASS); `start-3927ab10` (02:41) passed; `i105-spells-6` (native, 02:43-02:52): no `native_ffx_trap`
  row any more (the depth asks answered), 28 pipelines, no drop, no pipeline failure; the GPU particles' look with a non-zero depth
  size not compared with the original (the next targeted run).

## Lightning Spear's bolt: the tracer's constants against the original (2026-10-29)

- Witness (this branch, `d699e66f`, its step review's fixes after the runs): the native trail dump prints the tracer programmes'
  blocks whole; a Reference witness under `ffx-draw-observe.flag` prints the first 8 GX 11 indexed draws' same blocks (the DS and
  PS constant setters 19209D0 and 1920E20 hooked, each constant buffer's last whole update through 1920300). Step review (fresh
  Opus, high, `641ece6d..d699e66f`): nothing blocking; applied after the runs: each block's captured byte count in both rows (a
  block never captured printed zeros indistinguishable from real zeros), the native row's pass and GX 11 only, the Reference hooks
  pass-through once the 8 rows are written, the topology kept on every call, the base signed, and (for tranche 4) a frame dropped
  hands its paint events back to the next presentation. Left: the bytes are the buffer's last update from any context (L3).
- Runs (the ASUS, Alpha from a save copy, 12 casts, 4 spells a catalyst): `i105-bolt-ref` (Reference, 03:42-03:47) and
  `i105-bolt-native` (native, 03:47-03:52): no fault, restored by hash, 8 rows each (pass 0x29).
- Observations: DS b0 (view, projection, eye, fog, near and far, soft-sprite scale) equal within float rounding; PS b0 equal
  but for the ambient SH's w (0 natively, 1 in the original), three integer words (+440 `g_ps_BlendingMode` 2, +444
  `g_ps_FogMultMode` 1 and +544 `g_ps_bPreAlphaMult` 1 natively, 0 in the original) and cbFog +80 `FC_FogType` (1 natively, 0);
  DS `g_ds_lightDir` (0.707, 0.707, 0) natively against (0.497, 0.743, -0.448). The Reference rows predate the `_bytes` fields
  (their zeros may be uncaptured words) and come from other casts than the native rows (6 and 14 indices against 8 to 28).
- Read: the tracer HS (`GXFfxtessellateSoftTracerTexture.hpo`) sets every factor to 1 (one quad per 2-point patch); the DS
  widens the segment by TEXCOORD4.xyz times TEXCOORD0.w and multiplies COLOR0 by COLOR1 (alpha saturated): a zero width or alpha
  hides the ribbon. The first native vertex holds COLOR0 alpha 0 (INFERRED the trail's tail).
- Conclusion: not a zero constant block. Open, next: the vertices (the width TEXCOORD0.w, the direction TEXCOORD4, the alphas)
  and the states and textures of the same draws in both modes.

## Lightning Spear: the model particle s03000 (2026-10-29, branch `claude/awesome-shirley-c13df9`)

- The original draws the E03 model `s03000` about ten frames a cast (pass 0x29, no material table of its own: `i105-bolt-ref-2`,
  `reference_ffx_model_draws` with its new `materials` field), which the sampled native rows had never caught. A row per model's
  first submission (`native_effect_model_first`) shows it submitted and admitted natively with a forward row and its light
  (`i105-bolt-native-2`), but its family `GXFlver_Col` refused at the prepare for cbInstanceData `+0x30` (and the second
  instance's `+0x140`) unproduced (`native_selected_constant_unavailable`, also `s10002`'s draws).
- Read (GhidraMCP): 29ADF0 hands 59470 a null animation object, and 59470 then leaves its instance block's `+0x30` (and `+0xC0`)
  unwritten (`14005A120`..`5A0F2`: the stores at RBP-0x20 and RBP+0x70 only under the object). Witness (`reference_e03_instance`,
  the block captured at its upload returning at `5A0F2`, `i105-bolt-ref-3`): those words are the stack's, zero in nearly every
  draw (stray bits of earlier frames in the others).
- Code: an E03 draw without an animation object produces both rows zero. `i105-bolt-native-4`: no refusal row any more; the
  first vertical streak at 0.9 s now drawn ([sheet](../../evidence/I-0105/spells/spear-fix-native4-ref3-native2.png): native
  with the fix, original, native before). Not yet the original: the spear's straight bright core at 0.4 s is still missing and
  the vertical bolts are thinner and fewer (the 606 tracer, drawn but unseen, is the next lead).

## Tranche 4 M2: the paint queue under the engine lock (2026-10-29)

- The engine lock `[image+0x4793C48]` is a critical section (table `39F76B0`: +0x10 EnterCriticalSection, its timeout -1;
  +0x18 TryEnter; +0x20 Leave; read from a one-time native row and GhidraMCP): recursive. Its producers (D071B0, D00E60, ...)
  push under it, the original's drain 522B0 takes none itself (it runs inside a render callback, `205AC0`).
- Code: `EngineCpuLock` (shared with the physical model streams' update, which had its own copy) taken before `decal_cpu_lock`
  (the producers reach native hooks while holding it: the engine lock first); each queue is read and compacted under it, the
  taken entries drained after it is released (tried since the final review: a busy lock leaves the queues to the next collect, `native_paint_queue_busy`; the next run that paints reads its `n` against `native_paint_drain`'s: a busy count near the collect count sends M2 back). A compaction that fails now drains nothing (before, its entries were drained and
  left in the queue).
- Step review (fresh Opus, high, `0e4ca6a4..2c88e65d`): nothing blocking. Applied: the lock's Leave read at construction (its
  destructor never throws); the taken list reserved and an entry drained only once its erase is written; the Reference instance
  witness cleared after each draw and its non-finite words printed as null; the model-first witness behind `ffx-record.flag`, by
  data name. L1 (a native path holding `decal_cpu_lock` that would wait on the engine lock: ABBA with the collect) answered
  statically: the lock's users are the nine functions that read its table (51B940, 836E40, DFD6A0, D009E0, D00E60, D00F00,
  D00FF0, D071B0, D07220: draw-entity slots and stream updates; GhidraMCP reads of `[144793C48]`), none reached from the decal
  request (51BF0, its world query callbacks 5F114/5F120) or 32A80; recorded at `EngineCpuLock`.

## Lightning Spear: the tracer's points (2026-10-29)

- `i105-bolt-native-5` (`native_ffx_tracer_record`, the first 12 GX 11 draws): layout POSITION float3 @0, COLOR0 RGBA8 @12, COLOR1
  float4 @16, TEXCOORD0 float4 @32, TEXCOORD1/2 RGBA8 @48/@52, TEXCOORD3 R32 @56, TEXCOORD4 float4 @64 (stride 80); indices
  (0,1),(1,2),... R16; blend `0x20`, depth `0x1E8`, raster `0x208`, two targets; t0 an FFX texture (46472), t3 the depth.
  **Every vertex of every strip at one position** (271.4, -54.45, 606.3, by Alpha's hand), TEXCOORD0.x running along the strip,
  TEXCOORD4 a default direction ((1,0,0) or (0,1,0): 3709B0's fallback for a zero-length segment). Zero-length segments: the
  ribbon has no length, hence nothing seen. The points come from the renderer's records (`[r+0x20]`, 0x38 each, their point
  arrays 0x24 a point), appended each frame by vt+0x20 (373D00 → 1422EF7A0 per particle) and, by the static reading only, rebased
  by vt+0x30 (370620, every point by a matrix).
- `i105-bolt-ref-6` / `i105-bolt-native-6` (`ffx_tracer_history`, both modes): the appends and the first point equal in both modes
  (the hand, 271.45, -54.46, 606.26), no rebase call in either over 1200 appends (370620 not reached during these casts, observed).
- A/B (`i105-bolt-ref-noe03`: the original without its E03 draws, `ffx-skip-e03`; `i105-bolt-native-notrails`: native without
  606 and 10012; [sheet](../../evidence/I-0105/spells/spear-ab-refnoe03-nativenotrails-native4.png)): without its E03 models the
  original loses the spear's straight core at 0.4 s and the vertical bolts at 0.9 s, keeping the crackle the native shows; native
  without the trails looks as with them. Conclusion: the missing parts are E03 models, drawn natively since this tranche but thinner
  and fewer; the tracer's collapsed strips are not what is missing (INFERRED: they collapse in the original too while the spear is
  held; not compared draw for draw).
- Final review (fresh Opus, xhigh, `main...HEAD` at `5100b3da`): nothing blocking. Applied: STATE, this conclusion and the index
  rewritten (M1); the engine lock tried, never waited for, the queues left to the next collect when it is held
  (`native_paint_queue_busy`, L1); the tracer record's packed words printed as null when not finite and its offset kept signed
  (L2, L3); the Reference instance witness's name built once and its decal-path limit written (L4); this section's proof and
  observations (L5); two comments (L6). Left, recorded: a decal registry retired while a collect drains it (P1, pre-existing:
  `RetireNativeDecalCpuRegistry` runs the original's retirement before erasing the registry), the invalid JSON of
  `native_ffx_draw_dump` and `native_aa_inputs` rows (P2, pre-existing). Step review of those fixes (fresh Opus, high, `5100b3da..0f03ee2e`): nothing blocking; the M2 section completed (how to read the busy row).
- Proof: the MSVC build and its unit tests (106 PASS, `evidence/I-0105/build-26.log` on the final tree; `build-23` for the s03000
  fix, `build-25` after the step review); `Test-Project`. No unit test drives the E03 instance rows or the queue (engine memory):
  the runs above are the witnesses.

## Tranche 4: the blood's fade

- Read ([BLOOD](BLOOD.md)): no time-based fade; the paint queue's types 2 (clear all: bonfire rest, enemy reset) and 1 (an erase
  pass with the PntCol programmes: a decaying uniform erase after some SpEffects end, a wash sweep while the player is wet) were
  not drained natively, so painted blood never left. The erase pass's states read statically in GhidraMCP (its blend made in
  140072380: colour SRC_ALPHA / INV_SRC_ALPHA, alpha INV_DEST_ALPHA / ONE, a lerp toward (0,0,0,1)), the functions exported to
  `evidence/I-0105/blood/states/`: the Reference capture the study asked for is not needed.
- Code: `SnapshotNativePaint` drains type 2 with the mask 0xFFFF as the handle-0 clear and type 1 with its erase byte as an erase
  event (PS b10 +176 `(0,0,0,1-p1)`, +192 `(p2+p3, p2, 1/p3)`), both on a target the native already made (counted `no_target`
  otherwise); a masked clear and type 1 without the byte stay counted in `other_types`. The frame prepare gives each event its own
  job (by index; before, a paint row joined the first job with the same target and view) and adds to an erase job each of the
  receiver's G-buffer rows that sample t41 and whose family has a `_PntCol` programme (`NativeEraseProgramSet`, `NativeEraseDraw`: the game's
  programmes, the original's states, the paint target and its depth, no projector depth map). Witness rows: `native_paint_drain`
  (`erases`, `no_target`), `native_paint_frame` (`erase_jobs`, `erase_rows`, `erase_refused`), `native_erase_row_refused`.
- Left, recorded: the target's `+0x54` flags 1AAF90 and the type-2 masked clear set are not written (as the type-0 path; their
  readers not read); the erase covers only the rows of a receiver's models in the main view (the original draws the receiver's
  meshes whatever the view, INFERRED; a model out of view keeps its blood until it is drawn and erased again).
- Step review (fresh Opus, high, `0dc8c62c..bd2fb7a7`): nothing blocking; the offsets, the parameter mapping, the programmes'
  declarations, the blend and the paint encoding checked against the decompiled code; the per-event job named a fix (before, a
  paint after a clear in the same collect joined the job ahead of the clear and was wiped). Applied: a clear is not made again on
  a scene presented again (it would wipe the paint drawn after it), a type-1 erase of another surface than 0 is counted, rows
  without an erase programme are counted (`no_erase_programme`); then (the witness's step review) a frame dropped hands its paint
  events back to the next presentation. Build `evidence/I-0105/build-16.log` (106 PASS).
- Final review (fresh Opus, xhigh, `main...HEAD` at `e681374f`): nothing blocking. Applied: the erase rows gated on t41 as the
  paint rows (M1: a family without a `_PntCol` programme asked the shader repository at every erase command and rescanned it
  once its 250 ms miss cache expired; the skipped rows counted, `erase_no_t41`); the erase states' functions exported to the evidence (M3); stale sentences here and in STATE; a comment.
  Left, recorded: M2, the queue is read and compacted under the native `decal_cpu_lock` only, while the type-1 and type-2
  producers push under the engine lock `[0x144793C48]` (a push between the read and the end pointer's write is lost; a
  reallocating push would make the compaction write a freed buffer): not introduced here (the native erased these entries
  since D-0193), not taken blindly (the lock order between the native request path and the engine lock is not settled); next:
  settle statically whether the collect can overlap these producers, then take the engine lock or record why not. The
  type-2 clear and the handle-0 clear, the erase and paint programme lookups and their state blocks stay two copies (L2); the
  erase draws are rebuilt at each erase command (L3); the Reference tracer witness filters by topology, not by class (L4).
- Proof: the MSVC build and its unit tests (106 PASS, `evidence/I-0105/build-19.log` on the final tree, after a step review of the final review's fixes (`e681374f..9a9d5e56`, nothing blocking; the skipped erase rows counted and two sentences corrected)); clangd without diagnostics
  (`game_bootstrap.cpp`, `scene_pass_decals.cpp`). No unit test drives the queue (engine memory); not seen: no run yet paints blood
  on the player and rests at a bonfire or gets wet (the next run that does reads `native_paint_drain`'s `erases` and `clears`).
- Merged `d391b839` (2026-10-29 04:52); the launcher's module rebuilt (106 PASS, `evidence/I-0105/launcher-build-5.log`); the start
  check `start-d391b839` (05:03-05:04) passed: world drawn (342 frames), closed, restored, no refusal or failure; its effect stage
  row as before (19 pipelines, no skipped draw, no failure, no missing texture, 18,483 draws); no paint row on its path.

## Lightning Spear: the E03 models drawn thinner (2026-10-29, after tranche 5)

- `i105-bolt-ref-8` / `i105-bolt-native-8` (build-28: `native_effect_model_tally`, `ffx_model_rows`): natively about the same submissions
  as the original's 636F0 calls (separate runs, within 3.4 %) (`s03000` 116 and 116, `s10002` 632 and 654, `s11600` 23,282 and 23,260), all admitted, with the
  original's bone rows equal to five significant digits (`s03000`, one translation word 0.001 apart; `s10002`'s orientation is drawn at random per cast). The difference is in the
  drawing, not in the count or the placement.
- Read (a `ghidra-research` worker, headless, `evidence/I-0105/e03-state/`; the key lines re-read by the pilot): the draw state's
  words reach the GPU, and the native makes them for an E03 draw by its code (INFERRED from the reading, not witnessed natively): state +0x00..+0x3F (the FXR's four texture transforms,
  fxr+0x20..+0x2C) as VS b11 cbTexScroll (1A86C0, the block copied whole; natively `texture_scroll` from the group, which for a
  particle is the state), state +0xD0 (fxr+0x10) as cbModelParam +0x00 and cbInstanceData +0x70, fxr+0x50..+0x90 as the SH rows
  and the blend vector (PS b11, cbInstanceData +0x80..+0xBF). One word not followed natively: 1A86C0 writes args+0x204 = fxr+0x30
  (state +0x85; 29ADF0 leaves +0x84 at -1, so no blend state is bound from `[[GXRS+0x4DD0]+0x2F0]`, `140015220`); its reader is
  not found in the chain read (INFERRED a blend or programme selector).
- `ffx_model_rows`' `lod`: the Reference prints args+0x18 before 636F0's clamp, the native the clamped value (step review L6): a difference there alone means nothing.
- args+0x204 (a second `ghidra-research` read, `evidence/I-0105/e03-blend/`): no reader on the way to the draw (199 instructions
  with a +0x204 operand scanned, none in the chain; only the commit's write and two copy constructors): write-only (INFERRED from a
  static scan). More useful: a model draw whose args+0x2C4 is -1, as an FFX particle's, binds no blend state at all (1A86C0's
  branch; 140015220 is the OMSetBlendState wrapper): the original's E03 draws keep whatever blend the effect pass left bound
  (INFERRED: an effect's additive or alpha blend), where the native draws them with their material's forward blend.
- Next: a Reference witness of the blend, depth and raster states bound at the E03 draws (the last OMSetBlendState of the context
  before 636F0, with its D3D11 description) against the native rows' pipeline states; then the E03 rows drawn with the effect
  pass's state. Control spells once it holds (Thomas, 2026-10-29: Great Lightning Spear, Sunlight Spear, Lightning Arrow,
  Lightning Stake, Lightning Storm; Alpha's Faith too low for some: a test save « Alpha-faith » made from the copy, his own save
  untouched, Intelligence later).

## Tranche 7: the E03 draws with the effect context's states (2026-10-29 evening)

- Witness (`reference_e03_states`, Reference, the GX state setters 1920ED0 / 1920F00 / 1920CA0 by context; `i105-bolt-ref-11` after
  two runs whose search missed the table): at the E03 draws of `s03000`, `s10002` and `s11600` the context holds the GX device
  table's (`[[0x144961AB0]+0x2A0]`, the native bank) blend 0x20 (SRC_ALPHA, ONE: additive) and depth 0x1E8 (tested LESS_EQUAL, not
  written) in 11 of 12 rows (one `s03000` row another blend, depth 0x1E0), the commit's cached blend (holder +0x480) the same;
  the raster the table's 0x208 (solid, no culling) in 7 rows and another object in 5 (INFERRED by material); all read at
  636F0's entry (a per-mesh bind inside 63CB0 would not be seen; the commit's cached blend agrees for the blend). Natively the particles were the fading forward row: alpha blend
  SRC_ALPHA / INV_SRC_ALPHA, depth written.
- Code: `ModelSubmission` carries the bank offsets of the blend and depth the native effect context had bound when 636F0 ran
  (`effect_draw`), and the forward prepare draws an E03 row with them (`NativeEffectBankBlend`, `NativeEffectBankDepth`, the
  effect stage's own bank entries); the raster stays the material's. The pipeline cache test's files named by the process (two
  builds' tests deleted each other's: build-33's « T1 saved once »).
- `i105-bolt-native-12` ([sheet](../../evidence/I-0105/spells/spear-additive-native12-ref11-native8.png): native with the fix,
  the original, native before): at 0.9 s the vertical bolts thick, bright and branching as the original's; at 0.4 s the
  spear's straight bright core still missing natively (next).
- Final review (fresh Opus, xhigh, `main...HEAD` at `7d73bede`): nothing blocking. Applied: the glow's second target takes RT0's
  blend only when the entry is not independent (M1: an independent entry's RT1 would have been overwritten); an E03 row with a
  bank blend is no longer refused for its material's blend mode (L1); a witness row `native_effect_model_states` (the rows, the
  fallbacks, the offsets) (L2); this section's raster sentence (L3); the Reference witness on its own lock (L4); comments (L5);
  the effect stage's two pipelines through `NativeEffectBankBlend` (L6). Left, recorded: no unit test of the bank entries (L7), the
  test's files left in %TEMP% when a run fails (L10).
- Step review of those fixes (fresh Opus, high, `7d73bede..b2ce0574`): nothing blocking in the code; the new counter added to the
  thread-affinity allow-list (`Test-Project` failed without it). `native_effect_model_states`' fallbacks count the E03 rows without
  a known bank blend or depth, an offset 0 included.
- Proof: build-40 on the final tree (106 PASS; build-38 before the review, build-39 with main merged in); clangd on the changed units. `Test-Project -MergeGate` passes.
- Merged `498d1d50` (2026-10-29 20:11); the launcher's module rebuilt (106 PASS, `evidence/I-0105/launcher-build-8.log`); the start
  check `start-498d1d50` (20:21-20:22) passed: world drawn (377 frames), closed, the network stop signalled, restored.

## Tranche 8: the spear's straight core, the feathered model s03000 (2026-10-29 night)

- Method (Thomas, 2026-10-29: frame by frame rather than screenshots at fixed times): `Run-AlphaCast -Video` records the game's
  window with ffmpeg (gdigrab, 60 frames/s, hidden) from the first cast to the last, each cast's offset printed; `Get-CastFrames.ps1`
  makes one sheet of a cast's frames. A first slot recorded nothing (the variable `$video` was the switch `-Video`: PowerShell
  names are case-blind); its native run's trace job died with the script, the game stayed open and the save was restored by hand
  (`Restore-StartupTrace.ps1`, verified); the recorder's stop no longer ends the script before the restoration is awaited.
- Witness (`native_selected_texture_unavailable`, family GXFlver_ColZFeather, with `native_selected_texture_unbound` slot 56;
  `i105-bolt-native-14`; the new `native_effect_model_texture_missing` came after it and names the first texture of a refused
  selection, not the unbound one): `s03000`'s material
  GXFlver_ColZFeather was refused for its PS t56 g_DepthTexture, which natively only water materials received. The original binds
  the readable depth copy (the frame's `+0xFD0`) as t56 among the common rows of the passes after it (FUN_140103510, with t54/t55).
- Code: a programme whose pixel shader reads t56 g_DepthTexture gets the readable depth as a scene slot outside the water policy;
  without water, a forward row sampling it has the depth copied at the start of the forward pass (`forward_depth_frame`, in the
  plan key). `i105-video-native-2`: no texture refusal left; `s03000`'s next refusal its PS b2 cbMatDynParam +0xF0 FC_SoftVolume,
  which 22E930 writes from the material's dynamic object (flag +0x16: +0xF0 = 16 bytes from +0x320; flag +0x17: +0x100 from
  +0x330), not produced natively: now produced with the other flagged blocks (a clear flag: the buffer's creation zero).
- `i105-video-ref-2` / `i105-video-native-2` (cast 9, Lightning Spear; [frames at 0.80 and 0.87 s](../../evidence/I-0105/spells/spear-video-ref2-native2-800-870ms.png)):
  natively the spear's bolts and branches drawn, the dense straight core of the original still missing (before the +0xF0 fix).
- `i105-video-native-3` (build-43, both fixes; [sheet](../../evidence/I-0105/spells/spear-video-ref2-native3-native2.png): cast 9 at
  0.45, 0.80 and 0.87 s, the original, native with the fixes, native before): the dense straight core drawn natively as the
  original's; no `s03000` refusal left (texture, constant); no fault, restored. Seen on pictures, not yet validated in play. The
  instants are approximate: `video_ms` counts from ffmpeg's start, not its first frame (a few tenths of a second, per run).
  Every main-view frame of these runs at Firelink was a water frame (`graph_shadow_summary`): the core was drawn through the
  water branch's depth copy; `forward_depth_frame` (no water) has no witness yet (a cast where no water is in view, next full
  control).
- Step review (fresh Opus, high, `53d6a388..HEAD`): nothing blocking. Applied: `WaterDepthCopy` under its own lock (M1: the
  parallel admission's workers now call it for every t56 programme; a first creation or a resize raced); the frame's readable
  taken only from an SRV in the readable's format (L2: a material texture at t56 could have been chosen); the texture witness
  stops reading names once full and is in the thread-affinity scope and allow-list (L4); the recorder stopped after the last cast,
  waited for after a kill, and a missing ffmpeg a warning (L5). Left, recorded: a t56 row outside the main view, or in
  `forward_early` on a water frame, samples the last copied depth (L3: wrong feathering only, no state hazard); materials other
  than the effects that read +0xF0/+0x100 now draw with 22E930's values or zero (L6, as the original).
- Final review (fresh Opus, xhigh, `main...HEAD` at `cfc3d30d`): nothing blocking. Applied: the no-water path's missing witness
  said here and in STATE (M1); the witness citation corrected (L1); the readable search stops at the first match (L3); the texture
  witness capped at 4096 calls (L4); ffmpeg bounded by `-t` if the script is killed, an early exit reported (L7); the video offsets
  said approximate (L8); this proof line (L9); resume-thread's stale « end turns freely » (L10). Left, recorded: on a frame with
  decals and t56 rows but no water the depth is copied twice (L2: one more full-screen copy; the original has one, +0xFD0); the first
  t56 admission of a session rebuilds the kept records once (L5, as water's first appearance); `WaterDepthCopy` reads the size
  outside its lock and `NativeWaterDepthRecord` stores in two steps (L6: a resize's transient); the worktrees' ignored
  `.claude/settings.json` still name the deleted Stop hook (Thomas's settings).
- Step review of those fixes (fresh Opus, high, `cfc3d30d..13730ea2`): nothing blocking. Applied as it proposed: the witness's
  cap became a rate (the first 4096 calls, then one in 600: a row refused every frame from the load no longer silences the casts'
  rows, M1); ffmpeg's bound counts the latest shot (L1); the recorder's report flag initialised and set on an error (L3).
- Proof: build-46 on the final tree (106 PASS); `Test-Project -MergeGate` passes; the lock, the format filter and the witness's
  rate built, not run live (D-0227).
- Merged `a2a627d8` (2026-10-29 23:17, after I-0107's merge; main's graph key bit 38 is I-0107's `organize_frame`, so
  `forward_depth_frame` is bit 39); the launcher's module rebuilt (106 PASS, `evidence/I-0105/launcher-build-9.log`); the start
  check `start-a2a627d8` (23:26-23:27) passed: world drawn (358 frames), closed, the network stop signalled, restored.

## Next: the « Alpha-faith » test save (plan, 2026-10-29)

- Source: the m45 zone save copy Run-AlphaCast uses by default (SHA-256 8FA5E590…9D1C: Alpha at Firelink, the four spells attuned),
  never Thomas's live save; the copy `thomas-alpha-keeper-20261019-1701` (EF0BE67A…7385) holds Alpha before the Fire Keeper but
  from before the spells were equipped.
- One announced run: `Run-AlphaCast -TalkKeeper -HoldSeconds 600 -HoldShotEvery 5 -KeepSaveLabel alpha-faith` (the walk to the
  Fire Keeper, E, then the hold), the level-up menu driven from another shell with `Pilot-Game.ps1` (key, press, shot) and read on
  its pictures: Level Up, Faith (seventh row), raised to cover the control spells (the five spells' Faith requirements read in
  game on their item pages before choosing the value), confirmed; Quit Game keeps the save as a labelled test save with its hash.
  The Fire Keeper stands at a random spot each load: a miss is relaunched.
- Done (`i105-alpha-faith-1`, native, 2026-10-29 23:52 to 30 00:05): Alpha talked to the Fire Keeper, Faith 25 to 75 (Thomas,
  22:53: « 75 en faith [...] on vise large »), level 68 to 118 for 2,694,148 souls (3,150,522 left), the menus closed with the
  command list's Back and Leave; Quit Game kept the save: `C:/Users/User/.ds3-i0039-backups/alpha-faith-20261030-000515/DS30000.sl2`,
  SHA-256 69601F3CDDCD31D72AD0E9D5B40C4C19369371E93B39D0F2B27F2791F4BB010A (checked); Thomas's save restored by hash, no fault.
  Use: `Run-AlphaCast -SaveCopy <that path> -SaveCopySha256 <that hash>`. The level-up menu: Down x7 to Faith, Right per point,
  Down x2 to Confirm, E, Left to YES, E; Esc does nothing there (right click, Back).

## The control spells and the no-water depth copy (2026-10-30 00:13 to 00:38, the ASUS)

- Study copied in: [CONTROL-SPELLS-STUDY](CONTROL-SPELLS-STUDY.md) (where each spell keeps its effects; what the native path will
  do with them). Static prediction refuted by the run below: Lightning Arrow's E03 models are not in another pass nor with their
  own material table.
- `i105-attune-1` (native, Alpha-faith): Alpha placed on the Firelink bonfire (`witness-teleport.txt` "270.5 -54.6 605.5"), rested,
  Attune Spell read on pictures: Attunement 4, four slots. Alpha holds Lightning Spear, Great Lightning Spear, Lightning Stake and
  Lightning Arrow, **not Sunlight Spear nor Lightning Storm** (the whole miracle list read, `a-16-names.png`). Attuned: Great
  Lightning Spear, Lightning Spear, Lightning Stake, Lightning Arrow (`a-22-slots.png`); the list opens scrolled to the slot's
  current spell; the command list's Back, then Leave. Kept: `C:/Users/User/.ds3-i0039-backups/alpha-faith-attuned-20261030-002542/DS30000.sl2`,
  SHA-256 6D433A3B6801740DC92367E9A10633F4E76A1B5D589BC32367905F5712F15F3D (checked); no fault, restored.
- `i105-ctl-native-1` / `i105-ctl-ref-1` (the same in both modes: `-BonfirePages -TravelPress "323,191;400,280"` to the High
  Wall of Lothric bonfire, `-SwitchRight -SwitchRightTimes 2` to the talisman, `-SpellsPerCatalyst 12` (Up before each cast),
  six casts with `-Video`; the sixth, a second Stake, found no FP): Lightning Spear, Lightning Stake, Lightning Arrow, Great
  Lightning Spear, Lightning Spear. No fault in either run, both restored.
- Observations (native): after the travel no water frame (`water_surface` stays 4,799 from frame ~9,700) and 273 more
  `water_depth_copy` uses, in bursts at the casts: the no-water depth copy (`forward_depth_frame`) ran. Every E03 model
  admitted, none refused (`native_effect_model_tally`: `s03000` 461, `s08040` 81, `s08290` 113, `s10002` 2,358, `s11603` 30;
  all pass 0x29, no own materials, with bones, lit); the effect stage: no skipped draw, no pipeline failure, no missing texture
  (35 pipelines); the E03 rows' states the bank's blend 0x20 and depth 0x1E8, no fallback.
- Pictures (ffmpeg, 200 ms apart, the original above, native below:
  [casts 1 to 4](../../evidence/I-0105/spells/control-highwall-ref1-native1-casts1-4.png),
  [Lightning Arrow at 100 ms](../../evidence/I-0105/spells/control-arrow-ref1-native1.png)): Lightning Spear's straight core
  away from water, Great Lightning Spear, Lightning Stake's pillar and GPU particles and Lightning Arrow's bow and bolts drawn as
  the original's, to the timing of the casts (a few tenths of a second apart between runs). Seen on pictures, not in play.
- Not witnessed: Sunlight Spear (the study: Great Lightning Spear's families, only colours differ) and Lightning Storm (the
  study: GPU particles and blend 7, both met by Lightning Stake); a character holding them is needed.
- Tool: `Run-AlphaCast -SwitchRightTimes n` (the Right presses of `-SwitchRight`).

## The light shafts 10003: where they are spawned (static, 2026-10-30)

- The class `3D65A88`'s draw slot (vt+0x40, `271B40`) sets its own render targets (two textures from its context's object
  `[param_2[2]]+0x238/+0x240`), clears them (`1920200`), copies (`1920260`) and draws with the GX wrappers the native context
  hooks, plus `140015220` (the blend commit): a multi-target pass, more than a strip family.
- Its three FXRs (700014 in m30, 833031 in m33, 738204 in m38, `evidence/I-0063/artifacts/scan/fxr.tsv`) are placed by no SFX
  region of the m30, m33 and m38 maps (SoulsFormats MSB3 over the cached MSBs, `FFXID`), by none of the four zones' event
  scripts (`event/m30_00`, `m30_01`, `m33_00`, `m38_00`, recovered with `Recover-GameDataByName.ps1`: no int32 of the three ids)
  and by no other FXR of their bundles (no aligned reference). `skipped_renderer_3D65A88` appears in no evidence journal (the
  witness runs with `ffx-record.flag`, mostly Firelink and the witness zones). INFERRED: spawned by an object or a part model, or
  not used in play; the class stays skipped and counted until line 8's zone runs meet it (a count in `native_ffx_record`).

## Tranche 9: the surface snow (2026-10-30)

- Gap (I-0103 CENSUS §5, « check first »): natively cbPerFrame's snow rows +0x60..+0xAF, cbModelParam's FC_SnowRate +0x40 and
  cbMatDynParam's FC_SnowMaskParam +0xA0 were never produced, so a snow-family draw that reads them is refused (absent, not drawn
  without snow: `PackShaderConstant` Missing; COVERAGE-MATRIX row 7, `evidence/I-0068/queries/snow-family/REPORT.md` Q1), while the `Snow` groups of m30, m31, m38 and m45 (`m30_00_0000`: Scene Snow Rate 0.75, Angle
  (-1.22, 0.63), heights 0.22 / 0.44; m31, m38, m45: rate 1) give the original's snow materials (`*_Snow` MTDs, the SnowLt and
  DeepSnow families: two m30 map pieces, one m38, a few objects; I-0063 scan) their cover. FINISH-LINE: real, small in extent.
- Original (static, GhidraMCP): 23790 builds the rows from 131780's blend of two `Snow` records (12F400 on [root+0x45F8], the view
  context's pair +3FC0/+3FC8, blend +3FD0; a key no bank holds gives the bank's built-in record); 131780 copies a or b at the blend's
  ends (the direction then from a's angles) and otherwise lerps every field and the two angles through 1316D0; FC_SnowDir is
  (cos e sin z, -sin e, cos e cos z, 0). It stores the scene snow rate at [root+0x4E70]; 1A86C0 writes FC_SnowRate =
  lerp(46300([root+0x4600], state +2B0 / +2B8) +8, state +2C0) times it.
- Code: `ds3prod::ProduceSnow` / `ProduceSnowRate` (producers), `Banks::SnowAt` / `SnowModelAt` (the gparam files' `Snow` and
  `Snow Model` groups, natively the engine banks never receive files: their built-in records stand for a key no layer holds), the
  rows collected with the Dominant rows (12_scene_collect.inc) and written in both cbPerFrame productions, FC_SnowRate after
  `ProduceModelParamPs`; witnesses `native_snow_rows`, `native_snow_rate`, and in Reference (`ffx-draw-observe.flag`)
  `reference_snow_rows`, `reference_snow_rate` (the original's uploaded words). Unit checks in `glow_bank_test` (24 checks).
- Witness (`i105-snow-ref-1` / `i105-snow-native-1`, 2026-10-30 02:45-02:53, the ASUS, alpha-faith-attuned travelled to the High
  Wall of Lothric, both restored, no fault): the original's cbPerFrame rows at Firelink (0, 1, 4.3e-7, 0 | 1, 1, 0, 0 | 0.8 ×3, 1 |
  0.04, 0, 2, 0.02 | 1, 1, 0, 0) and at the High Wall (0.201, 0.9397, 0.2767, 0 | ... | 0.04, 0.22, 4.545, 0.64 | ...); natively the
  same words in both places (`m30_00_0010`'s record at the High Wall, scene rate 0.75): the direction confirms 141FD1A40 = sin,
  141FD1740 = cos. The original's FC_SnowRate: 1 for its first 449,273 cbModelParam updates (Firelink and the way), 0.75 from the first
  High Wall update on, no other value; the native per-draw FC_SnowRate not witnessed (build-50 had no `native_snow_rate`, and its kept
  draws held their first rate: the step review's H1), expected 0.75 = the built-in 1 × 0.75.
- The snow pieces at the High Wall (observed, `i105-snow-native-1`): the `GXFlver_ColDifSpcBumpMulMaskDOL_SnowLt` material was
  prepared and refused at its textures: `native_selected_texture_unbound` slot 47 g_SnowDiffuseTexture (and 16
  `native_selected_texture_unavailable` rows naming slot 0, the witness's cap), no constant row: the pieces stay absent natively. The
  original binds t47..t51 in 103510 from the blended `Snow` record's texture holders (131780's first 0x28 bytes: t47 out+0x00, t48
  +0x10, t49 +0x18, t50 +0x08, t51 +0x20), else the root's defaults [root+0x4800], +0x4810, +0x4818, +0x4808, +0x4820, else the device
  table [[0x144961AB0]+0x270] +0, +0x20, +0x20, +0x30, +0x10 (static, `decompiled/DS3-1152-F-00103510.c` 739-790): not bound natively
  yet. The High Wall sweep pictures are the same natively and in the original
  (`evidence/I-0105/spells/snow-highwall-ref1-native1-sweep.png`): no snow piece in view.
- Step review (fresh Opus, high, `4ebc45b0..36c598f7`): H1 the kept draws kept their first FC_SnowRate (the native's first High
  Wall row was the built-in record, before the gparam layers bound): the kept epoch now carries the scene rate, the built-in rate and
  the gparam banks with their generation; L1 the rate witnesses (native per draw, Reference counts past the cap); L2 the rows
  witness on a change of keys or a settled blend; L3 `ProduceNativeSnowRate` in the thread-affinity scan, its statics reviewed; L4
  1316D0's wraps and the empty groups unit-checked. Builds: build-50 (the witness), build-51 (the fixes), 106 PASS.
- Final review (fresh Opus, xhigh, `main...HEAD` at `510fc3d8`): the code a faithful transcription (23790, 131780, 1316D0 checked
  against the disassembly, 1A86C0, 12F400, 46300), no hazard; the documentation promoted plausible to true. Applied: H1 the third
  word, cbMatDynParam +0xA0 FC_SnowMaskParam {s, 0, -0.5 s, 0} from GX06 (22E930's flag +0x11, s = +0x3C; PRODUCERS-UNION-SNOW-GLOW
  2.3), with GX06 parsed natively (the material builder: +0xE1 = 1, +0x10C = data word 0, as 141070 at 140141837; its +0x4E8 raise
  has no native reader: the buffer grows per block), and this section corrected (the gap was a refusal); M1 the rate's witness
  corrected above; the witnesses formatted only when logged and bounded; test nits. A step review of those fixes found the GX06 item
  not parsed natively (fixed as said) and the snow pieces' texture refusal in the run (the section above). Left, recorded: t47..t51
  not bound (the snow pieces stay refused); the kept draws rebuilt at every
  gparam binding and every frame of a snow-rate transition (coarse epoch, L1); per-draw locks in `ProduceNativeSnowRate` (L2); the
  decal and offscreen cbModelParam paths without FC_SnowRate (0 where the original writes 1, harmless unless read, L3); the
  MapOverride files' Snow records (Scene Snow Rate 0) never consulted (kind-8 layers, INFERRED, L4); no positive SnowAt check on a
  file holding the group (L7). The surface snow stays open on FINISH-LINE line 5 until a snow piece is seen admitted.
- Proof: build-54 on the final tree (106 PASS, `glow_bank_test` 24 checks, 7 of them the snow's); `Test-Project -MergeGate` passes;
  the GX06 parse and the +0xA0 block built, not run live (D-0227).
- Merged `5d332c39` (2026-10-30 03:47); the launcher's module rebuilt (106 PASS, `evidence/I-0105/launcher-build-10.log`); the start
  check `start-5d332c39` (04:05-04:06) passed: world drawn (374 frames), closed, the network stop signalled, restored.
- Next (tranche 10): the snow textures t47..t51 (103510's binds above): a witness of the objects at [root+0x4800..+0x4820] and the
  device table [[0x144961AB0]+0x270] in both modes (their class and texture names), then their native records, so the snow pieces
  draw; then the High Wall pieces seen against the original.

## Tranche 10: the snow textures t47..t51 (2026-10-30)

- Original (static, GhidraMCP, with the I-0110 study thread): 103510 binds, for every draw, t47..t51 from the blended `Snow`
  record's words +0x00, +0x10, +0x18, +0x08, +0x20 (131780: a's words at a blend of 0 within its epsilon, b's otherwise), a null
  one replaced by the root's [root+0x4800], +0x4810, +0x4818, +0x4808, +0x4820, then by GX's default table [[144961AB0]+0x270]
  +0, +0x20, +0x20, +0x30, +0x10 (06C00's entries 0, 4, 4, 6, 2: white, BLACKSPEC, BLACKSPEC, the flat normal, grey). The record's
  words are filled by 131EA0 (each frame from 12F350 through the manager update 499C0, a null word retried) with the repository
  [root+0x47C8] lookups (D49B10: D4ED20's entry+0x60 holder) of `%sDeposit_{a,n,r,s,m}_%04d` from the record's string and its
  `Texture:ID` (0 in all 54 gparam files): at the High Wall of Lothric the record `m30_00_` gives map/m30/m30_9999.tpf's
  `m30_00_Deposit_A/N/R/S/M_0000`; the built-in record's `m22_00` names no texture (Firelink: null words). The root's members are the
  holders of `global_snow_a`, `_n`, `_r`, `_s` (D59E90), then `snow_*` (D1E8A0 at each context publication) or `dust_*` (D1EA10);
  +0x4820 has no known writer.
- Code: `CollectNativeSnowTextures` (12_scene_collect.inc) once per main collect; the record's names made natively
  (`NativeSnowRecordNames`: a key `Banks::SnowAt` finds in a gparam file is named by its area's stem "mAA_BB_" with the group's
  `Texture:ID`, any other key by the engine's built-in record's own string and ID; INFERRED: equal to the original's record at the
  High Wall, but at Ariandel the original keeps the built-in record where the native finds m45_00_0000's group (no texture
  either way); m30_01, whose `m30_01_Deposit_*` exist, is not witnessed), looked up at each collect through the repository D49B10 answers from (the resource singleton's
  +0x30, which D573B0 publishes as [root+0x47C8] in the original; `NativeSnowRecordHolders`); the root's members stored as D59E90
  and D1E8A0 store them (`StoreNativeSurfaceTextures`, `PublishNativeStageTextures`); the defaults the native bank's entries of the
  same indices. A holder is resolved through its inner as the material bindings are; one without a resident record leaves its slot
  unbound (no stand-in, D-0197). A programme reading t47..t51 (`samples_snow`) gets them as scene slots; a model record binding them
  compares them (`NativeRecordEpochs::snow`). GX-DEPENDENCIES C6 and K-0109 now name 103510 as the root members' consumer.
  Witnesses: `native_snow_textures`, `native_snow_record_names`, `native_snow_selection`, `native_surface_store` (both modes) and,
  in Reference with `ffx-draw-observe.flag`, `reference_snow_blend` (each record's name, ID and five words, per caller) and
  `reference_snow_textures`.
- Witnesses (the ASUS, alpha-faith-attuned travelled to the High Wall of Lothric bonfire, offline, restored by hash, no fault in
  any run): `i105-snowtex-native-1` / `-ref-1` (04:33-04:41): the native root empty, the defaults bound (the grey snow);
  `-native-2` (05:01-05:05): the root's `snow_*` bound; `-native-3` / `-ref-2` (05:47-05:55): the original's record words non-null at
  the High Wall; `-native-4` / `-ref-3` (06:23-06:31, build-59): the original's 103510 (caller 10466B) and 23790 blend the records
  `m30_00_`#0 at the High Wall and `m22_00`#0 elsewhere; natively the names `m30_00_Deposit_{a,r,s,n,m}_0000` are found and t47..t51
  are `m30_00_Deposit_A_0000`, `_R_`, `_S_`, `_N_` (512x512) and `_M_0000` (2048x2048); at Firelink both modes fall back to the root
  (`snow_*`, t51 grey). The `GXFlver_ColDifSpcBumpMulMaskDOL_SnowLt` programme is no longer refused at t47: its G-buffer stages are
  Ready (its Gbuf pixel programme reads t47 alone, I-0068's dump), every prepared frame without a missing texture. Four of its other
  pixel stages name t34 g_LightPrepassAttrTexture and t52 g_RandomNoiseTexture ([[root+0x4DD8]+0x778], absent natively): not bound
  (INFERRED passes the native does not draw for this opaque family; such a draw stays refused). The menu views' scenes get no snow
  slots (a snow programme there stays refused). `i105-snowshot-{native,ref}-{highwall,ariandel}` (06:52-07:04, build-61): the teleport
  to the High Wall fence 003970 (I-0110: "-7 8.5 195") landed indoors and the m45 zone save starts indoors: no snow piece in view in
  either mode; at Ariandel the original blends the built-in record `m22_00` (null words) where the native takes m45_00_0000's
  `Snow` group (the same values, a 1e-7 angle), and both bind the root's `snow_*` (no `m45_00_Deposit_*` exists). `-wall` and `-fence` (07:20, 07:58):
  teleports to the pieces 008111 and 003970 (MSB positions, then shifted by the arrival's offset (-44, +67, -58)) left Alpha dead
  (no floor there): the teleport's frame at the High Wall is not known. The pieces seen
  against the original stay open.
- Reviews: the final review (fresh Opus, xhigh, `main...00471da3`) found H1 the record's words in memory order (fixed: t-order),
  M1 the record branch never witnessed (the witnesses above, which found the Deposit textures), L1-L9 (STATE, GX-DEPENDENCIES and
  K-0109, the last value kept past Close, the second store comment, the defaults looked up per collect, the unwitnessed ports, these
  records, the paint defaults, the menu views): fixed or recorded here. Step reviews of the fixes (`00471da3..db09d2a8`: five lows,
  fixed in `b9f1a11c`) and of the record names (`b9f1a11c..23e8fde6`: two mediums, four lows, fixed in `cba6825b`: the area-stem rule, the built-in
  record's own string, the texture manager checked before D49B10). The final review on the final tree (`6585da96`): no high; M1 the
  record rule above (recorded INFERRED, FINISH-LINE scoped to the witnessed zone), M2 this bullet, L1 the native D1EA10 observer
  removed, L2 a comment, L4 K-0109's wording; L3 (five lookups a collect, names that never exist included) and L5 (no unit check of
  the names) left.
- Side reading (not changed, open): 1AC390's paint defaults are [[GX+0x270]] entries 16 or 2 (t3) and 15 or 0 (t4) by the paint
  context's +0x38, not +0x10 = SYSTEX_NORMAL as 12_scene_collect.inc's D2 comment infers (entry 2 is grey).
- Proof: build-62 on the final tree (main `63fdc8a9` merged, 108 PASS); `Test-Project -MergeGate` passes.
