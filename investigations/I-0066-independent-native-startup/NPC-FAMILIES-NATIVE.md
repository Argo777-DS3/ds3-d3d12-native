# NPC families natively (workstream started 2026-10-11 03:35, branch `claude/npc-families-c1-c5`)

Specification: I-0068 [NPC-FAMILIES-STATIC](../I-0068-static-yebis-ffx-lights-spec/NPC-FAMILIES-STATIC.md) §7 (static
changes C1-C5, then the live blocks W2-W7). Started in the velocity thread while the coordinator opens its own thread.

## C1-C3 (commit d50f3b5c)

- **C1** model union (`+0x220` classes, `+0x224` policy flags) for every admitted model, not only ASM and menu parts
  (`game_bootstrap.cpp`, after the mesh binding). The pose owners' auxiliary policy (`model_policy_flags`, bit 14) keeps its
  part-only scope: widening it would change the palette route of cloth NPCs and needs its own witness.
- **C2** `BuildModelMaterial` parses GX03 (`+0xDE`/`+0x104`), GX05 (`+0xE0`/`+0x108`) and GX09 (`+0xE3`/`+0x200..+0x20C`) from
  the FLVER GX records into the genuine material object, as 141070 would; the 22E930 producers then write
  `FC_EmissiveColor`, `FC_TranslucencyScale` and `FC_ParallaxParam`.
- **C3** vertex programmes may sample material textures (`SelectModelShaderTextures`, Vertex and Pixel stages; the model draw
  already has per-stage SRV tables) and the vertex stage gets `cbPerFrame` (`FC_GlobalTime`).

## Native1193-1195 (`i66-native-npcfam-1193`..`-1195`, Argo at the High Wall start, camera turns)

- No fault, no debug error, configuration restored; close exit 0xD3126606 (the known env-region refusal).
- `native_pass_rows` (1195, every 60th frame: prepared rows by engine pass and owner kind, and the unions per kind): with map
  pieces carrying class `0x100` in view (8 of 295), **8 engine-7 rows for map pieces** appear (`[7,-1,8]`), none before C1
  (they were filtered at MaterialClass, gate 21, in 1192); skinned map objects (1001) now stop at ModelClassMask (gate 18)
  instead of MaterialClass (1192 -> 1193), the original's order. One SprjModelDrawEntity in view (11 rows, 11 velocity rows);
  no class-0x100 character was in view at this start, so the 37 characters of §2.1 are not witnessed yet.
- No new constant refusal (only the pre-existing DOL `cbTexHdrScaleParam` and grass `cbvModelParam` rows).

## Open

- C5 coded (commit after 3e6d762e): forward rows of a draw whose group byte `+0x87` is set take `<family>_Phantom_Fwd[_[A]]`
  (recipes 12-13, pseudo pass 0x100) when the family has it; unit-tested, not witnessed (needs a phantom; its b2 PhantomParam
  rows 26-30 are not produced natively yet, PHANTOMS-SIGNS-FOGWALLS §1.2).
- C4 (scene-colour copy at t45, RecursiveCrystal): needs a renderer-owned HDR copy bound as a scene slot and the copy point
  (W5 live read); not started.
- The live blocks W2-W7 of NPC-FAMILIES-STATIC §7 (positions away from the High Wall start; Firelink needs the TestAnim
  save). Load Game pilot: the native input latch (I-0067 NATIVE-INPUT-LATCH) explains the lost presses; until it lands,
  hold the press until `ui_event_pointer` shows buttons=1.

## C4 and Native1196-1197

- **C4** the scene-colour copy (`pipeline+0x1108`): `SceneRenderer::SceneColourCopy` owns an HDR-format copy at the
  presentation size; the main view copies the lit HDR into it at the start of the forward pass (FFX-FAMILIES-PHASE2 §8: the
  frame copy is made at forward pass 0x29), before the 0x1F and 0x29 rows; the render thread publishes it as a scene texture
  (`native_scene_copy`) and the selection binds it as slot 45 for any mesh whose own material textures do not name slot 45.
  Explicit policy until block W5 reads the original's copy point (this frame before the forward rows, or the previous one).
  The note above on C4 is superseded.
- Native1196: the Load Game confirmation was lost (5 tries). The pilot now holds the row and OK presses 1.2 s (native Load
  Game updates take 0.67-0.84 s and sample the input once, I-0067 NATIVE-INPUT-LATCH); Native1197 loaded on the first try.
- Native1197 (non-regression of C1-C5 at the High Wall): the copy is published once, 303 model frames, 0 debug errors,
  0 faults, configuration restored. No crystal and no phantom in view: C4 and C5 wait for blocks W5 and a summon.

## Block 0 and the geometry refusals (Native1198-1201, Reference1198-1199)

Witnesses added: `reference_character_row_words` (57A20 hook, Reference: the model returned by the second virtual of `param_9` called with 0,
once per character model, its `+220`/`+224`), `native_character_engine7_row` (pseudo part 1000 at the engine-7 decision, the
native union), `native_pass_rows` now carries the camera `eye` and `axis_z`, `native_character_geometry_missing` (a character
whose storage has no joined meshes, with the `CopyModelGeometry` refusal), and the refusal codes are split (16 more than 32
meshes, 17 vertex buffers, 18 index buffers, 19 a second LOD-0 face set, 9 with topology in bits 8-15 and index format in
bits 16-23). Pilot: `-Forward n` (walk forward without coming back). Runs: Argo (slot 0) 1198 pair and 1201; TestAnim
(slot 1, Cemetery of Ash) 1199 pair and 1200. No fault, configuration restored each time.

- **L0.1, original words** (Reference1198/1199): `c1282` and `c1100` `+220` = 5, `+224` = `0x0C000804`; `c1000` 0 /
  `0x10000000`; `c1430` 5 / `0x0C000804`; **`c3100` 3 / `0x1C208004`** (bit 15 set: the crystal lizard has the engine-7 row,
  not listed in NPC-FAMILIES-STATIC §7 L0.1; bit 21 set). Bits 26-27 (spx-name bits) are set on every character: the native
  union does not produce them (explicit, NPC-FAMILIES-STATIC §8 "other 140F90 words").
- **L0.1, native union**: `c3250` (the only character admitted at the High Wall start) 5 / `0x2804` (1201). No common model
  with the Reference list yet: same-model comparison still open.
- **Geometry refusals (the real blocker for W5 and part of W1/W2):** the original draws `c3100` and `c1100`; natively both are
  `missing_geometry`. `c3100` (1200): face set index format **`0x2A` (R32_UINT)**, topology 4: the native model path only
  accepts 16-bit indices (`game_scene_geometry.cpp` CopyOnce, `MeshSlice`, `model_geometry.h`, `model_draw.cpp`
  ValidateGeometry, `content.cpp` ValidContent all assume R16). `c1100` (1201): **more than 32 meshes** (CopyOnce bound;
  `ValidContent` has the same 32-mesh cap). Next: carry the index width per mesh slice (R16/R32) through the join, the
  content validation, the binding and the draw validation, and raise the mesh bound to the FLVER's count; then rerun W5 (C4
  scene copy at t45 on the crystal) and W1/W2.
- C4 is still not exercised by a crystal draw: `c3100` never reaches selection.

## Native1202-1217 and Reference1206-1215 (thread change 2026-10-11 05:00, branch `claude/eager-dewdney-b8c9b1` from 00fd4300 + main)

- **Index width per mesh slice.** 1FA030 writes the face set view format 0x39 (R16_UINT) for a 16-bit FLVER face set and
  0x2A (R32_UINT) otherwise, and the view start (+8) is the face set's data offset divided by that width. `MeshSlice` now
  carries `index_width` (2 or 4) through `CopyOnce`, `ValidContent`, `BindModelGeometry`, `ValidateGeometry` and the two
  older single-mesh paths; the mesh bound is `kMaximumContentMeshes` (1024) instead of 32. Native1202: `c3100` joined and
  selected (rows `[5,1000,4]`, `[41,1000,3]`, union 3 / `0x208004`); Native1204: `c1100` joined (union 5 / `0x2004`),
  pseudo part 1000 67 prepared, 67 drawn.
- **C4 exercised.** The scene-copy record had no description, so the selection's shape check (2D: one slice, a known
  format) refused the whole `RecursiveCrystal` pixel stage as Unsupported (Native1202-1203, witness
  `native_texture_shape_refused` added with `native_texture_envelope_refused`). Described as a one-slice 2D target in the
  forward format: `RecursiveCrystal` 3 prepared / 3 drawn (1205), 6/6 at W5 (1214, both lizards). Image W5
  (`i66-native-npcfam-1214` vs `i66-reference-npcfam-1214`, `ingame-teleport.jpg`): the crystal lizard is drawn natively,
  darker and without the original's glow (bloom/GlowFilter belong to the YEBIS and emissive-glow families). The copy point
  (this frame before the forward rows) stays the explicit policy; L5.1 (the original's t45 resource) was not read.
- **Witness teleport (test tooling, both modes).** The pilot writes `x y z [hold seconds]` to `witness-teleport.txt` next to
  the module; on the game thread (WorldChrMan schedule 8DD2E0) the player's physics position
  `[[[WorldChrMan+0x80]+0x1F90]+0x68]+0x80` (the field 9D9650 reads) takes it and keeps it for the hold, events
  `witness_teleport` / `witness_teleport_released`. A single write or an 8 s hold falls through the target's ground
  (1207-1209: y down to -250; the target's collision is not enabled while the player is airborne); holding for the whole
  witness works (1210 onward). MSB positions (`evidence/I-0067/queries/chr/msbenemy.tsv`) are in the same coordinates
  (player start (27.4, -25.5, 53.3) against MSB `c0000_0008` (25, -25.5, 52.3)). Pilot: `Pilot-LoadGameThomas.ps1
  -Teleport x,y,z[,hold] -TeleportWait s`, `Run-PilotPair.ps1` passes both.
- **W2** (next to `c1280_0004`, (24.3, -10.3, 96.1); Native1210-1211, Reference1212): the Lothric knights `c1280` and their
  cloaks are drawn natively. Found outside the NPC inventory: the tree foliage (orange leaves) the original draws there is
  absent natively (a map family, D-0150: to add to the world coverage inventory). L2.1-L2.3 (b2 words, stencil, `cbSSS`)
  were not read on the original.
- **W3** ((-43.8, 17.6, 77.6), pair 1213): a roof; `c2150` and `c1090` were not in view (no row words for them in either
  mode). Not witnessed. Seen there: native sky flat, roof grass absent (the known grass `cbvModelParam` refusal).
- **W6** (Firelink, (262, -55, 603)): Native1215-1216 stopped on a UI shape the native mesh writer did not know (leaf class
  `3AE0EB0`, generate slot `1CFB9C0`, under the same delegate `3AE0C90` as the admitted leaf `3ADF440`/`1C3AB10`); it is now
  admitted the same way (same slot, same CPU interface, output format checked; RVA pinned). Native1217: rows for `c1400`,
  `c1450` (class `0x100`, union `0xE004`: the cinder row 7), `c3190`, `c3200`, `c1430`; families drawn with no failure:
  `C_ARSN_cinder` 2/2, `GXFlver_EyeIbl_Bloody` 2/2, `GXFlver_SSSIbl_Bloody` 2/2, `ColDifSpcBumpEmiIblGlow` 1/1,
  `ColDifSpcBumpTrIbl` 58/58; the Fire Keeper is visible (`ingame-look-4.jpg`). The shrine's map pieces appear a few
  seconds after the teleport natively (streaming); candles and fire are absent (effects family).
- **L0.1, model words.** The original `+0x224` bits 11 and 13 are not the union of the material `+0x24` words: `c1100`,
  `c1282`, `c1430` (cloth materials, no SSS material) have `0x800` and not `0x2000`, and `c3100` (no cloth) has neither.
  57A20 (decompile line 432) tests model bit 11 to call the model's cloth objects (`+0x1F8..+0x200`, vtable `+0x48`): at
  model level bit 11 means "has cloth". The native union (OR of the material policy flags) puts `0x2000` for cloth and
  `0x800` for SSS; native consumes only bits 15 and 12 of it, so no row changes. Bits 0-10, 12, 14-25 agree where both
  modes saw the model (`c3100`); bits 26-28 are not produced natively. The original writer stays unknown (PRODUCERS-UNION
  §1.3).
- **Reload.** A fall into the void (1207-1209) reloads the area, and the native build stops with
  `native_env_region_cpu_generation`, the refusal already known at close: the reload transition is an open family.
- **Not run or not read:** W3 (enemies not in view), W4 (`c5270` arena) and W7 (`c5110`): boss arenas, not attempted; the
  original-side constant reads of every block (b2/b3 words, stencil, t45 resource): this tooling captures no original
  constant buffer. C5 draws nothing yet: the phantom rows (lists 0x21/0x22/0x29) are refused natively (`model_pass.cpp`
  category != 2); I-0067 PHANTOMS-SIGNS-NATIVE (main 1abdc1ae) specifies the rows, constants and witnesses PL1-PL10.

## Pre-merge review (fresh Opus 5.5 agent, high effort, D-0163)

Nothing blocking: the scene-copy barriers (PSR -> COPY_DEST -> PSR, HDR target PSR -> COPY_SOURCE -> RT), the index width in
every MeshSlice consumer, the mesh bound, the UI leaf pairing and pin, the teleport's thread and SEH use and the mandate (no
D3D11) were checked. Fixed: a phantom-flagged forward draw keeps the normal Fwd recipe when the alternate's texture
selection is not ready (explicit policy, witness `native_phantom_forward_fallback`) instead of being dropped; the pilot
reports a teleport only on `"applied":true`; the missing-geometry witness names each storage once (the per-frame name read
is gone); `BindModelGeometry` refuses an index offset not aligned to its width (unit test added). Recorded, not changed: the
Reference-only 57A20 witness does a getter call and a set lookup per row call (witness cost in Reference runs); the scene
copy runs on every lit frame with forward draws even without a t45 consumer, and offscreen menu views bind the main view's
last copy (performance and offscreen fidelity, open); the teleport reader is compiled in for both modes and polls one file
attribute every 15 schedule calls (test tooling; a file next to the module is required). After the fixes: full build with
the control suites and the model-selection test PASS; no live rerun.
