# Wave 1, the FFX effect draws (E01 first): implementation log

Workstream « Dessin des effets FFX », decided by Thomas (2026-10-17 06:48 Paris) after « Effets manquants de la Vague 1 »:
the bonfire flames, torches, embers, smoke and spells need the effect draw side. Branch `claude/ffx-draw` from `main`
46523524. Design inputs (static, I-0068): [EFFECTS-D3D12-DESIGN-NATIVE](../I-0068-static-yebis-ffx-lights-spec/EFFECTS-D3D12-DESIGN-NATIVE.md)
§3 (steps N5-N14), [EFFECTS-PHASE2-E01-E02-NATIVE](../I-0068-static-yebis-ffx-lights-spec/EFFECTS-PHASE2-E01-E02-NATIVE.md)
(the machinery, contracts, PSOs, builders, checks P1-P6 and Z1-Z3), [EFFECTS-IMPLEMENTATION-CHECKLIST](../I-0068-static-yebis-ffx-lights-spec/EFFECTS-IMPLEMENTATION-CHECKLIST.md)
§3, [FFX-INPUT-LAYOUTS](../I-0068-static-yebis-ffx-lights-spec/FFX-INPUT-LAYOUTS.md), FFX-WAVE1-READY F7.

## 0. Starting point (read in the tree at 46523524)

- Wave 1 runs the effect system natively (`native/frame/effect_system.cpp`, FW-2/FW-3): the scene control built and
  published (`SprjSfxImp+0x58`), the door open, the per-frame slot 8 update transcribed, effect lights and the foliage wind
  fed. Its GX statements are replaced by a device token (a reserved no-access page): buffers, views and programmes created on
  it are native records (`CreateBuffer`, `CreateView`, `CreateProgram`); **the shader set FUN_140291520 is already built**
  on the token (sc+0xE0, [sc+0x28]+0x168) with native programme records keeping the bytecode.
- Not built: the GPU ring (sc+0xC0/+0xC8/+0xD0), the attachment of the SFX layer to the view's scene, the pass points, the
  draw pass and the per-draw context. The view attach (24F2C0) stores the view only.
- Nothing of the effects is drawn natively: in every side-by-side the bonfire, torches and lanterns are missing.

## 1. Plan (the design's N5-N9, E01 first)

1. **N5 record layer, dry** (no encode): at the native forward pass points (0x21, 0x29, 0x2D, 0x20, 0x31; 0x39, 0x45; never
   0x3A), call the scene control draw `FUN_14024E790` with a native pass context and a native `GXFfxDrawPass` whose slot 4
   records `{key, entity, data}` (keeping `FUN_1402997E0`'s tests); execute the records in key order through
   `FUN_140299E40` (draw context `FUN_14025FB80`, matrices `FUN_1402600A0`, the renderer `entity vt+8`). The GX statements
   reached on the token are captured instead of executed: the ring map/unmap (`FUN_14213B690/6A0`, a native per-frame
   arena), the topology, vertex/index buffer binds, `Draw`/`DrawIndexed` of the three builders, and slot 27
   (`FUN_140264230`)'s programme, state, constant and texture binds. Each draw becomes one `EffectDrawRecord`. Checks:
   FX1/FX2 counts against the original (m37 ≈ 68 patch-list draws per frame, m31 5, m51 6), the arena peak ≤ 2 MB.
2. **N6 encode E01**: one root signature, a PSO cache keyed by the resolved state tuple (§6 of PHASE2, observed states §6.1),
   1-control-point patch lists with the hull/domain pairs, depth read-only (DSV + SRV), RT [HDR, GB5] for the GlowD twins.
   Witnesses: the m40 bonfire, the High Wall torches, m37 (P1-P6, Z1-Z3), side by side.
3. N7 E02 trails, N8 glow twins, N9 frame copy + E04/E05, then E03, E07 (the design's order).

## 2. Log

### 2026-10-17, N5 reading (static, decompiled exports)

- **The recording goes through the GX pass context's command stream, not through a slot the native could replace alone.**
  `FUN_14024E790(sc, passCtx)` (the scene-control draw) calls the library draw `[sc+0x18] vt+0x20` with a `GXFfxDrawPass`
  on its stack (`{vt 143D6A1F8, passCtx, sc, ids sc+0x30..+0x40, flag}`); per entity the pass's slot 4 `FUN_1402997E0` runs
  its tests (masks `entity+0x1F8 & ~sc+0x14C`, `passCtx+0x4B8/+0x4BC`, the environment fade `[[GXRS]+0x5360]` and
  `[[GXRS]+0x5368]+0x158/+0x170/+0x188`, `FUN_14029AA10(passCtx+0x418 camera, entity)`, the near test through
  `FUN_14004BCA0`), chooses the engine pass (kind 1 0x21, 2/4 0x29, 3/6/default 0x29 or 0x31, 5 0x2D) and records:
  `FUN_14020E570([passCtx+0x4B0]+0x38, 0x80, 4)` (the 0x58-byte data block), the key `FUN_1400BF330(passCtx, .., distance)`
  or `FUN_1400BF3F0`, reversed when `passCtx+0x426+pass` is 1, stored at `passCtx+0x12C`, `+0x128` the pass, then
  `FUN_1400BF4C0(passCtx, 4, 0x18)` = `{FUN_140299E40, entity, data}` and `FUN_1400BF2D0(passCtx)` (commit). The multi mode
  (sc+0x590) records the same way with `FUN_1400BF210/270`.
- `FUN_1400BF4C0` allocates in the sort list `[passCtx+0x4B0]` (`FUN_140198680`, 0x18-byte entries at `+0x70`, count `+0x7C`,
  capacity `+0x78`; key words `+0x120/+0x124/+0x128/+0x12C`, flags `+0x421..+0x423`).
- **Native route chosen for N5**: a native pass context (a zeroed block with the fields the slot reads: `+0x124` pass id,
  `+0x410` a native frustum object whose `vt+0x60` answers from the native view's culling, `+0x418` the engine camera the
  native view already reads, `+0x4B8/+0x4BC = {1, 1}`, `+0x4C0..+0x4E0` tokens for the frame copy and depth, `+0x425` the LDR
  flag 0, `+0x426..` the reversal table of the original's pass contexts) and hooks on `FUN_1400BF4C0`, `FUN_1400BF2D0`,
  `FUN_1400BF330`, `FUN_1400BF3F0`, `FUN_14020E570` that act only when the context is the native one (any other context runs
  the original): each `{callback, entity, data, key, pass}` goes to a native list. The records are then executed in key
  order through `FUN_140299E40` with the GX statements of the draw context, slot 27 and the builders captured (next read:
  `FUN_140299E40`, `FUN_14025FB80`, slot 27 `FUN_140264230`'s GX calls, the builders `FUN_140385F70/396F30/39E900`).

### 2026-10-17, the original's draw side measured (Reference1925-1927, `ffx-draw-observe.flag`)

New Reference probe (`ffx_draw_observe`): 24E790 with its pass context's fields, the commands BF4C0 records inside it, the
callbacks 299E40 executed and, inside a callback, the GX topology/Draw/DrawIndexed calls, per pass.

- **The scene-control draw runs once per frame, for GX pass 0x45 only** (Reference1927 m37: 6,000 calls, all pass 0x45);
  every effect command is recorded there (about 64 a frame at the Gamma save's view), and the command data's engine pass
  (+0x08) dispatches the executions: about 59 a frame at 0x29, 3 at 0x2D, 2 at 0x31.
- Draws inside the callbacks: 0x29 about 57 a frame with GX topology 10 (the 1-control-point patch list of E01, the
  tessellated builder) and about 1 with topology 1; 0x2D about 3.5 with topology 4 (triangle list); 0x31 about 2 indexed
  with topology 10. Same order as the design's Z1 (≈ 68 at m37 in the I-0063 capture, another camera).
- The pass context at the call (pass 0x45, both zones): frustum vtable 143D42D58, the camera +0x418 (+0x30 a position,
  +0x50 0.75 aspect, +0x54 0.08 near, +0x58 10000 far at the title; live values to read in game), the sort row +0x250..
  +0x25C (the camera's z row and distance), masks +0x4B8/+0x4BC {1, 1}, flags +0x420..+0x425 zero at entry, the reversal
  table +0x426: passes 1F, 29, 2D, 31, 34 set, +0x4C0..+0x4F0 seven pointers (frame copy, depth, pipeline objects; +0x4E8
  is 1), +0x58C 1.
- Consequence for N5: one native pass context with pass 0x45 a frame; the commands executed per engine pass in key order
  (0x29 inside the forward rows, 0x2D, 0x31 after them; NATIVE-PASS-ORDER for their place).

### 2026-10-17, N5a: native recording wired, why nothing is recorded yet (Native1928-1935, Reference1936)

- Native (`ffx-record.flag`): one native pass context a frame (`RecordNativeEffects`, the main view's collect) with the fields
  above; the camera +0x418 is the GX camera record CED670 fills for the main view (`native_scene_camera_object`: +0x00 the
  world matrix, +0x20 forward, +0x30 position; the view context's +0x210 is empty natively, Native1929); hooks BF4C0, BF2D0,
  20E570 (and a witness on 2997E0) act on that context only. 24E790 runs (sc+0x20 = 1, +0x5BE = 0, +0x590 = 0) and the
  library draw 142155520 walks its list: 136 groups, 1,002 listed objects, of five types; only vtable 143A9C150 draws
  (vt+0x38 = 141AD6440: entity +0xB0 bit 0, then 141ADB0C0 over its node chain, 141ADDE80 per node, which needs node
  +0x70 (a count), +0x58 (the renderer, its vt+0x48 the kind passed to pass slot 6, vt+0x50 to slot 4) and +0x68, then the
  pass's slots 6, 5 (frustum on node+0x30) and 4).
- **The gap**: at the same Gamma m37 view, 663 entities active in both modes, but their nodes' +0x70 is set in 661 and
  +0x58 in 285 in the original (283 with all three: the drawn effects), against 30 and 8 natively (0 with all three). The
  effect library natively never gives its nodes a renderer nor a count, so the draw pass is offered nothing. Next read: who
  writes node+0x58 and +0x70 (the renderer creation from the FXR appearance and the shader set, and what the native
  transcription of the update or the resource manager slot 1 leaves out).

### 2026-10-17, why the drawable nodes have no renderer (Reference1937/1939 against Native1938/1940)

- The GX effect type registry [1449651D0] is the same in both modes (22 types, the same constructors).
- The renderers (node +0x58) by vtable, same view: original 143D7E5C0 ×202, 143D7E440 ×72, 143D7E680 ×3, 143D7E800 ×6,
  143D65878, 143D65CC8; native only 143D7E800 ×6 and the two 143D65… (no 5C0, 440, 680). Their constructors: 5C0 by
  `FUN_14037D260` (registered for type 0x25B by `FUN_14036DD10`), 680 by `FUN_140385DF0` (the class of the tessellated E01
  builder 385F70), 440 by `FUN_140375C70`, 800 by `FUN_140396DA0` (the triangle builder's).
- `FUN_14037D260` (0x48 bytes, vtables 143D7E560 then 5C0): `FUN_140385920` (its vertex block) then **`FUN_140381DE0`**, whose
  first statements take the renderer's textures from the effect's resource provider (`param_3 vt+0x38(id, 0)` into +0x28,
  `vt+0x38(id, 1)` into +0x30, `vt+0x30(id, 2)` into +0x3C) and **fail when the first texture is null**. Natively the FFX
  textures do not exist (the resource manager's slot 1, FFX-FAMILIES-PHASE2 §4 row 1, was left out in wave 1), so every
  textured renderer fails and is destroyed; the six 800 renderers (untextured kinds) are made.
- Next: the FFX GPU resources (slot 1 of the resource manager [sc+0x28]: the effects' TPF textures as native textures the
  provider's vt+0x38 returns), then the recording gets its nodes.

### 2026-10-17, the recording matches the original (Reference1941-1950, Native1942-1952)

- **FFX textures.** The provider's vt+0x38 is `GXFfxGraphicsResourceManager` slot 7 `FUN_14028F7B0` → `FUN_1402B7110`: the
  name `s%05d` (first `s%05d_a` for kinds 0 and 1) is looked up in the texture handler `[rm+0x158]` (`FUN_14024C390`: `+8` the
  `SprjFfxGxTextureRepository`, `+0x10` a second repository, null here), the entry is a `SprjGxTexture` whose vt+0x10 forwards
  to its `GXTexture2D` `[entry+0x10]`, whose vt+0x10 `FUN_14002AA60` returns `[+0x48]`, its `CGShaderResourceView`. Natively
  the entry and the `GXTexture2D` exist and TextureIngress owns the latter, but `[+0x48]` is null (the native admission never
  creates the old view), so every textured renderer constructor (`381DE0`) failed. Native slot 7 (`NativeTexture`): the same
  lookup, and in place of the old view a native token naming the `GXTexture2D` (vtable of traps, `+0x10` 'NFXT', `+0x18` the
  `GXTexture2D`; `TokenReceiver` for the draw capture). Native1945: the renderers by vtable equal the original's (5C0 ×202,
  440 ×72, 680 ×3, 800 ×6), no trap called.
- **The particle simulation was never run natively.** Node `+0x70` is the live particle count written by `FUN_141ADD510`
  (emission through the emitter's vt+8, ageing, compaction). Reference1950: per frame it runs as the update list's items
  (`item vt+8 = FUN_141ADC100` → `1ADD510(item-0x88, [item-8], dt [data+0x18], emitter [data+0x10])`) executed by
  `GXFfxUpdateJob` slot 1 `FUN_140296FF0` on GX's job queue (the chain `24CFA3`/`B5F20`, a job thread); at creation it also
  runs once inline (E6DCD0 → 24F0C4 → 24EED1 → 1AB159F → 1AB12C4). Wave 1 counted the update and proceed lists without
  executing them (EFFECTS-PHASE1 §5 step 7, which took them for draw-side work: **wrong for the update list**). At m37 the
  update list holds only `1ADC100` items and the proceed list is empty. `effect_system::Update` now runs the `1ADC100` items in
  list order (`Engine::run_cpu_items`, behind `ffx-record.flag` while in development; every other kind counted by
  `ItemKinds`). Native1951: nodes `[663, 661, 285, 663, 283]`, the original's exactly.
- **Culling.** The pass context's `+0x410` is a `GXViewVolume` (vtable 143D42D58): native one built by `6E780` and set each
  frame from the camera record by `6E9E0` (as the native light visibility already does), in place of the accept-all stub.
- Recorded per frame, same view: native 69 (0x29 ≈ 63, 0x2D ≈ 3, 0x31 ≈ 2.4), original ≈ 60 (0x29 ≈ 55, 0x2D ≈ 2.6, 0x31 ≈ 2.1).
  INFERRED: the remaining gap is the camera (the native camera record and the original's view are not the same frame);
  to be checked with the executed draws.
- Next (N5b): execute the records in key order through `FUN_140299E40` with the GX statements of the draw context, slot 27
  and the builders captured (`EffectDrawRecord`), the token resolved to its native texture.

### 2026-10-17, N5b: the commands executed natively, every GX statement captured (Reference1953, Native1954-1955)

- **The original's execution context** (Reference1953, `reference_ffx_execute_context`): the commands run with the recording
  pass context itself (`+0x124` 0x45, `+0x128` the engine pass, `+0x12C` the key), its `+0x450` a deferred GX context
  (`1921AC0` answers 1) whose `+0x21F8` is the context index (0, 2, 7 seen), the ring `sc+0xD0[index]`. The original's update
  lists run on the multi-threaded branch (`_GXFfxEntityContextUpdateJob`), the job bodies `296FF0`/`296D80` never run.
- **The draw side's GX statements** all go through the GX context wrappers, first argument the inner context `[outer+8]`
  (D3D11 slot in brackets): `1920300` UpdateSubresource (0x180), `19203F0` input layout (0x88), `1920420` topology (0xC0),
  `1920460` index buffer (0x98), `19204C0` vertex buffers (0x90, through `56F30`), `1920660` VS constants (0x38), `19206E0` VS
  (0x58), `19208A0` HS (0x1E0), `19209D0` DS constants (0x210), `1920A50` DS (0x200), `1920C00` GS (0xB8), `1920CA0` raster
  (0x158), `1920D20` PS samplers (0x50), `1920DA0` PS resources (0x40), `1920E20` PS constants (0x80), `1920EA0` PS (0x48),
  `1920ED0` blend (0x118), `1920F00` depth (0x120), `1921370` Draw (0x68), `1921390` DrawIndexed (0x60), `1921AC0` GetType
  (0x380). The ring is `holder → object → vtable {_, map (this, &ptr, ctx, bytes) → 0, unmap (this, &buffer, &offset, ctx)}`.
  Slot 27 reads the effect display's state bank `+0x2A0` and default-texture provider `+0x270` (sites already redirected).
- **Native (`effect_draw`, `ffx-execute.flag`)**: native outer/inner contexts (state caches; the inner's `+0` a trap object),
  selector answered "deferred" as in the original, the ring a 2 MB CPU arena in `sc+0xD0[0]`, the bank and provider native
  tokens in the effect display, the 21 wrappers hooked (on the native inner context they record into `EffectDrawRecord`; any
  other context reaches the original). The commands run in engine-pass order (0x21, 0x29, 0x2D, 0x20, 0x31), then key order.
  Only the E01/E02 renderer classes run (`3D7E5C0`, `3D7E440`, `3D7E680`, `3D7E800`); the GXVfx appearance (`3D65878`, E07
  GPU particles) faulted in `25D590` on its null GPU buffer (Native1954) and is skipped and counted.
- Native1955 (m37, 600-frame windows): no fault, no trap; per frame ≈ 66 draws, 0x29 patch list (topology 10) 60.7, 0x2D
  triangle list (4) 3.8, 0x31 indexed patch list 2.3; original (Reference1953) 53.5 / 3.3 / 2.2 (+ 0.9 point-list draws at 0x29,
  the skipped GXVfx). Every bound constant buffer is a native effect buffer, every UpdateSubresource resolved (≈ 264 a frame),
  the PS resources 68 FFX texture tokens a frame (the other slots null), sampler s9 a bank state; ring peak 206 KB.
- Next (N6): encode E01 in D3D12: the programmes from the native shader set records, the input layouts (FFX-INPUT-LAYOUTS), one
  root signature, PSOs keyed by the captured state, the arena uploaded per frame, the textures through TextureIngress, at the
  forward pass point (NATIVE-PASS-ORDER).

### 2026-10-17, N6 inputs: the state bank and the programmes (Reference1956, Native1957-1960)

- **The GX common state bank** `[[4961AB0]+0x2A0]` (`FUN_1400D21F0`) as its D3D11 objects describe themselves (Reference1956,
  `reference_state_bank`, every slot +0x08..+0x2F8): blend +0x08..+0x1D0 (the FFX types 0 +0x08 off, 1 +0x10 SRC_ALPHA /
  INV_SRC_ALPHA, 4 +0x18 DEST_COLOR / ONE, 2 +0x20 SRC_ALPHA / ONE, 3 +0x28 SRC_ALPHA / ONE REV_SUBTRACT, alpha ONE / ZERO ADD,
  mask 15; +0x30.. the mask-7 copies, +0x60.. alpha ZERO/ZERO, +0x80.. independent, +0xD0.. alpha-to-coverage; +0x198 off with
  mask 0, then +0x1A0 ONE/ZERO (alpha ZERO/ZERO), +0x1A8 SRC_ALPHA/INV_SRC_ALPHA (alpha ZERO/INV_SRC_ALPHA), +0x1B0
  DEST_COLOR/ZERO (alpha ZERO/INV_SRC_ALPHA), +0x1B8 SRC_ALPHA/ONE (alpha ZERO/ONE), +0x1C0 ONE/SRC_ALPHA, +0x1C8 ONE/ZERO
  REV_SUBTRACT, +0x1D0 ONE/SRC_ALPHA (alpha ZERO/ONE)); depth +0x1D8..+0x200 (as FORWARD-DEPTH-NATIVE 1.1: +0x1E0 LESS_EQUAL
  write, +0x1E8 LESS_EQUAL no write); raster +0x208..+0x2C0 (+0x208 solid cull none, +0x218 solid cull back, +0x228 cull front,
  the odd ones with scissor, +0x238.. wireframe, +0x268.. multisample); samplers +0x2C8..+0x2F8 (+0x2C8 point wrap, +0x2D0
  linear wrap, +0x2D8 linear wrap max LOD 0, +0x2E0..+0x2F0 the clamp versions, +0x2F8 point border white). The native PSOs
  take these descriptors by bank offset (the capture records the offset in the state token).
- **The programmes** (Native1958 probe): the shader set stores each stage as the GX shader object's handle (slot 2 of
  GXVertexShader `6D570`, GXPixelShader `6D560`, GXHullShader `6D550`, GXDomainShader `6D530`, GXGeometryShader `6D540`:
  `[+0x10]`), null natively, while the native registry keeps the bytecode under the object itself. Its first word (the "input
  layout") is the native record of `1925210`, which is `CreateInputLayout` (device, out, the element table from `14473EBC0`,
  its count, the VS object): the native record keeps the element table's first `count` bytes only (mislabelled "programme";
  the layouts are static per VS index, FFX-INPUT-LAYOUTS). Native (`ffx-execute.flag`): the five getters hooked; inside the
  shader set build (`effect_draw::ProgramScope`) a null handle of a registered object returns that object's programme token.
  Native1960: every draw's VS and PS resolve, HS and DS on the 37,684 patch draws (null on the triangle draws), GS null.

### 2026-10-17, N6: E01 drawn natively (Native1961-1966, Reference1965-1966)

- **Engine side** (`effect_draw`, `BuildNativeEffectFrame`, `ffx-execute.flag`): each captured draw becomes a renderer
  `NativeEffectDraw`: the programmes from the native registry (the tokens' shader objects), the pixel programme's target count
  by reflection (SV_Target1: the glow twins also write GB5), the input layout decoded from the image's GX element table (the
  `1925210` record now keeps its source address), the bank offsets of the state tokens, the constant blocks materialized at the
  draw (VS b0, DS b0, PS b0, b1, b6: the bytes of each buffer's last UpdateSubresource), the vertex and index offsets into the
  frame's ring arena, the textures t0..t4. The pass context's frame copy (+0x4C0) and depth (+0x4C8) are native texture objects
  whose view (+0x48) is a copy or depth token: the original then binds them itself (the distortion programmes' t0, the soft
  programmes' t3); without them, the 0x2D distortion draws sampled a null frame copy and covered half the screen in black
  (Native1963). The frame rides the selected scene into the frame input (`NativeModelFrameInput::effects`).
- **Renderer** (`renderer/effect_stage`): one root signature (VS b0, DS b0, PS b0/b1/b6 root views, PS t0..t4 table, PS s9 table,
  s1 static anisotropic wrap), PSOs keyed by programmes, layout, bank entries, topology type and target count (the bank's
  descriptors as Reference1956 dumped them), a shader-visible view ring per frame slot, the arena uploaded once a frame. Drawn
  after the forward rows, into the lit HDR and GB5, the depth read-only (DSV slot 1, its depth plane in DEPTH_READ |
  PIXEL_SHADER_RESOURCE for t3), passes 0x29, 0x2D, 0x31 in that order.
- Native1964-1966: no fault, no refused pipeline, no missing texture; ≈ 67 draws a frame at m37 (11 PSOs), 14 PSOs at m40. Side
  by side (D-0181 shots, same moment): the bonfire flame of m37 matches the original's; m40 (Lige-Feu) draws its bonfire, the core
  INFERRED a little whiter.
- Open (by order): the falling snow of m37 (INFERRED: the GXVfx appearances, E07, still skipped); the default texture (counted,
  bound null); the draws whose depth entry writes (+0x1E0, blend type 0) test without writing (the depth view is read-only);
  the effects drawn after all forward rows instead of sorted with them by key; the order of 0x2D and 0x31 against the passes
  between them (NATIVE-PASS-ORDER).

### 2026-10-17, step review and default (Native1970-1971)

- Opus review of N5-N6 (read-only): applied: the frame-graph plan key has an effect bit and the copy follows effect-only frames;
  the vertex and index buffers must be the ring's or a native effect buffer (the triangle builder 396F30 binds a static quad
  created with data: its bytes are appended to the frame's arena); EffectStage has Close and quarantine (the owner's close, a
  reopen prepares again) and prepares atomically; the item walk of the update list runs only with the draw side, with null
  checks; the display's +0x270/+0x2A0 are set only with the draw side; partial or unreadable constant updates drop the draws
  that bind the buffer (a never-updated block is zero, as created); a GS or a non-bank s9 drops the draw; the depth and copy
  views follow the target generation; line topologies with adjacency; the PSO key carries the target formats.
- Checked (Native1970 witness rows): the update (`native_effect_update`) and the recording (`native_ffx_record_thread`) run on the
  same thread.
- **On by default** with the effect system; `no-ffx-draw.flag` opts out; `ffx-record.flag` now only adds the witnesses (slot 4
  and node update counts, the record and execute rows). Native1971 (m37, no flag but the witnesses): ≈ 61 draws a frame, none
  dropped, 11 PSOs, no fault. D-0185.
- Open (unchanged): E07 (the GXVfx appearances: the m37 snow), the default texture, depth-writing draws, the key order with the
  forward rows, the INFERRED whiter m40 core, a faint rectangle at the distortion quads (m37 right of the flame, m31 lower right).

### 2026-10-17, witnesses and final review (Native1968-1972, Reference1968-1972)

- Side by side (D-0181 shots): m30 (Theta, the High Wall): the bonfire and the tower torch drawn as the original's
  (Native/Reference1968); m31: no effect difference visible (the wave-1 residuals only; a faint rectangle at the lower right,
  INFERRED a distortion quad, open); Thomas's Argo save before Vordt's arena (Run-PilotPair, default flags): the wall torch drawn
  as the original's, no fault in either run (Native/Reference1972).
- Final review (Opus, read-only): nothing blocking. Applied: the update row's unbounded kinds no longer go through a fixed buffer;
  a released effect object's captured constant bytes are forgotten (`effect_draw::Forget`, drained at the next Execute); a bound
  block never updated takes its creation bytes, else zero (INFERRED: the original's buffers are created without data, so their
  first content is undefined under D3D11; the native upload is zero-filled); an absent copy or depth view is remade when its
  resource returns; stale flag comments. Left: one target generation for both views (the copy reset covers the observed case).

## 3. Next: E07, the GPU particles (plan, 2026-10-17)

E01 is merged (main 4f76b5ea). The GXVfx appearances (`3D65878` standard, `3D65CC8`; the m37 snow, m40 bonfire sparks, the Ariandel
snow) are still skipped: their draw slot (`2695E0`) runs the simulation (`2A6A30`, in the flag-0x10 submission) and the draw
(`2A7E70`) on GPU resources created by `2A49F0` (EFFECTS-E07-GPU-PARTICLES-NATIVE). Plan, same method as E01 (D-0185):

1. **Resources.** `2A49F0` creates its buffers and views on the effect device: they are already native records (`CreateBuffer`,
   `CreateView` kinds 2 and 3). Make each a persistent D3D12 resource the renderer owns (structured 176-byte particle buffers
   with UAV and counter, the 16-byte argument and counter buffers with their initial data), keyed by the record's handle and
   retired on release. Witness: Native1954's fault (25D590 reading `[p1+8]`, the global constant holder `[144962360]+0x18`) must
   find its holder: the global GPU-particle constant buffers (`gcbSceneParam`, `ccbSceneParam`, `ccbForceFiledParam`,
   `ccbEmitionSeed`) are created where natively nothing creates them yet: read their creation first.
2. **Capture.** The compute and indirect statements the two functions reach (CS programme, CS constants/SRVs/UAVs with initial
   counts, Dispatch, DispatchIndirect, ClearUnorderedAccessViewUint, CopyResource, VS SRVs, GS constants, DrawInstancedIndirect):
   their GX wrappers and helpers (`1400C5EB0`, `1400C5F40`, `1400C61B0`, `1400BE7D0`, `19211E0`/`1921160`/`1921260`/`19212F0`/
   `1920200..` per the exports list) hooked as in E01, recorded in order into one per-frame command stream (compute and graphics
   interleaved: the simulation precedes the draw).
3. **Encode.** Replay the stream in D3D12 in order: compute PSOs, UAV counters written for each initial count other than −1,
   ExecuteIndirect for the indirect dispatch and draw, UAV barriers, the argument copies; the graphics draws with the E01 stage's
   bank states, topology POINT, VS + GS + PS.
4. **Pass hook-up.** The flag-0x10 submission (the simulation) runs once a frame before the draws: check whether the native
   pass-0x45 recording already offers the GXVfx entities with flag 0x10 (the original also calls 24E790 at pass 0x39).
5. Witnesses: m37 snow, m40 sparks, m45 Ariandel snow (G1-G7 of the E07 note), side by side.

### 2026-10-17, E07 first steps (ffx-gpu.flag, in development; Native1974-1975)

- `PublishNativeFxWorld` builds 248E30's GPU parts on the effect device when `ffx-gpu.flag` is set: the constant holder
  `world+0x18` (`25D220`, seven buffers) and `+0x20` (`25E960`, two): Native1974 `constant_buffers` 9 (all native records). The
  one-context random seed buffer is not yet made (`19254F0` refused my description: `random_buffer` 0; to read 248E30's exact
  description words before retrying).
- With the GXVfx appearances allowed (`effect_draw::Engine::gpu_particles`), the draw `2A7E70` first faulted in the GS constant
  wrapper `1920B80` (Native1974, the handle read through `[+0x18]`): the compute, copy, clear, VS/GS/CS binding and indirect
  statements are now hooked as well (`1920200` ClearRenderTargetView, `1920220` ClearUnorderedAccessViewUint, `1920260`
  CopyResource, `1920B80` GS constants, `19205E0` VS resources, `1921160` CS resources, `1921260` CS UAVs, `19212F0` CS,
  `19211E0` CS constants, `1921320` Dispatch; `1400C5F40` DispatchIndirect and `1400C61B0` DrawInstancedIndirect on the outer
  context), counted only.
- Native1975 then faults at `2A8318` reading `[[GXRS]+0x4DD0]+0x2C0` (cbCubeEnvSH1): the scene root's device member `+0x4DD0`
  stays null natively by design (other native readers rely on it, world_adapter.cpp). E07's draw needs a different route for
  that block than publishing `+0x4DD0` (a transcription of `2A7E70`'s use of it, as EFFECTS-E07 §6 #6 asks): next step of E07.
- Native1977-1981 (m37, `ffx-gpu.flag`): the GXVfx appearance now runs through its simulation and draw without fault. Needed:
  for the execution only, the scene root's `+0x4DD0` names a native settings shadow whose `+0x2C0` is a 48-byte native effect
  buffer (cbCubeEnvSH1; null again before Execute returns, so the native readers of `+0x4DD0` never see it); the pass context's
  frame copy and depth objects answer `vt+0x10` with their view as a GXTexture2D does (the system's `S+0x24D8` depth texture
  is the pass context's); the effect display's `+0x288` (the null-view set the simulation unbinds with) is a block of zeros;
  the native vtables now hold the trap functions (they held the trap table itself: a call through them executed data).
  Per frame at m37: one emit dispatch now and then, one ClearUnorderedAccessViewUint, one DispatchIndirect, two CopyResource,
  one DrawInstancedIndirect (the snow system), its GS and VS resources; all their buffers and views are native records (kinds
  1-3). Not yet resolved: the CS programmes (the GXComputeShader getter `6D520` is not among the shader set's tokens: the VFX
  programmes come from elsewhere), the ordered stream itself (counted only), and the D3D12 side (persistent resources for the
  records, compute PSOs, UAV counters, ExecuteIndirect).
- Native1982-1983: with the GXComputeShader getter (`6D520`) among the shader set's tokens, every CS, VS, GS and PS the GPU
  particles bind resolves (no getter is called outside the set's scope but once, Native1982). The objects they reach (Native1983,
  `native_ffx_gpu_object`, m37's snow system): constant buffers 448 (gcbSceneParam), 1104 (ccbSceneParam), 3488
  (ccbForceFiledParam), 752 (ccbEmitionSeed); the particle buffers 22,528 bytes (capacity 128 × 176, bind 136 = SRV|UAV, misc 32
  structured, stride 176) with UAVs `{format 0, dimension 1, first 0, count 128, flags 0}` and SRVs `{0, 1, 0, 128, 2, …}`; the
  16-byte argument buffers (bind 8? misc 8 = indirect, stride 4) and the counter buffer (bind 128, misc 32, stride 16) with a
  UAV of format 42 (R32_UINT) count 4; the 1024-byte random seed buffer (bind 128, structured, stride 1024) and its SRV. The
  UAV descriptions carry no append/counter flag (flags 0): INFERRED that GX keeps the D3D11 flag elsewhere; to read in the
  view creation (`1925B10`) or witness in the original before the D3D12 UAVs are made.
- Next (E07 encode): an ordered GPU-particle stream from the capture (compute: CS, CBs b0/b1/b5 bytes, SRVs, UAVs with initial
  counts, Dispatch or DispatchIndirect; CopyResource; ClearUnorderedAccessViewUint; the indirect draw with VS t0/t1, GS b9,
  PS states), persistent D3D12 buffers and views per effect record (created from these descriptions, initial bytes uploaded,
  counter resources for the append UAVs), ExecuteIndirect for both indirect statements, run before the effect draws.
- **No append or counter UAVs** (superseded below, "E07 encode and the first visible blockers": the programmes do append and consume) (Reference1987, `reference_gx_uav`, every structured or counter buffer UAV GX creates at m37, the
  D3D11 views' own descriptions): `Buffer.Flags` is 0 for all of them (capacities 128 ×24, 256, 512, 1024, 2048, 4096, ...; the
  R32_UINT counters of 4). EFFECTS-E07 §2's "flag 2 (APPEND)" and "flag 4 (COUNTER)" do not hold in the live views: the
  particle programmes count through their explicit counter buffer, and the initial counts of CSSetUnorderedAccessViews are
  ignored by D3D11 for such views. D3D12: plain UAVs, no counter resources, no counter writes.

### 2026-10-17, E07 encode and the first visible blockers (Native1988-2000, `ffx-gpu.flag`)

- Native1988-1992 (commits `5f0a9446`, `f4c2f117`, `9a302595`): the capture records the ordered GPU-particle stream; the frame
  carries it with the effect records it reaches; `EffectStage` encodes it (persistent buffers per record, compute and particle
  roots, ExecuteIndirect, copies, clears). No fault; nothing visible.
- Native1994: the default texture token resolves to `SYSTEX_WHITE`; still no snow (args `[52,1,1,0]`).
- Native1995-1996 (witnesses `native_ffx_particle_draw`, readback of the drawn particles): GS b9 `gcbSceneParam` is right (its
  view-projection puts the camera at the origin within 0.2). The first particle's fields after its position were mostly zero,
  and PS b0 held a NaN at `+0x1E0`.
- **Correction of Reference1987's reading: the particle programmes append and consume** (`CS_VFX_StandardParticle` declares u0
  `consume`, u1 `append`: `imm_atomic_consume` / `imm_atomic_alloc`; EFFECTS-E07 §3.1's initial counts are 0 on the first
  emission and 0 for the update's append buffer). The D3D11 views carry `Flags 0`, yet the original works, so the hidden
  counters exist (INFERRED: the driver's). Without a D3D12 counter, every alloc wrote one slot. Natively (Native1997-1998), each
  structured UAV view gets a 4-byte counter (pooled, 4096-byte placement, zero when assigned, freed on release), and the bind's
  initial counts other than -1 (captured at `1921260`, applied once, as D3D11 does at the bind) are written into it before the
  dispatch (`counter_writes` about 2 a frame). The particles are distinct now.
- **PS b0 NaN**: `263C70` and `264230` divide a PSCommonBuffer row by the outer context's viewport (`+0xC8` count, `+0xCC`
  D3D11_VIEWPORTs), which was zero natively. The outer context now reports the scene viewport (the native output size). This
  also changes E01's PS b0 at `+0x1E0..+0x1E7`, from NaN to 0 at m37. **E01 has not been compared side by side since this fix.**
- **WeightColor zero**: the update CS reads its keyframe curves from t0 `csInputTex` = `[S+0x2A10]+0x48` (and t4 `[S+0x37D0]+0x48`
  or the default). `2A6A30` reads that GXTexture2D's `+0x48` directly (no `vt+0x10`), which is null natively (Native1999:
  `S+0x2A10` is a GXTexture2D, vtable `3D1D528`; `S+0x37D0` is null at m37). Natively (Native2000), the hook of `2A6A30` (with
  `ffx-gpu.flag`) lends a registry-owned GXTexture2D its FFX texture token at `+0x48` for the call only. The particles now carry
  a colour (WeightColor not zero; 3,001 lends, 0 unowned). Native2000's picture (frame 2100) still shows no snow, and only 3-4
  particles are alive (args) where Native1994 had 52-67 before the counters and the original shows dozens of flakes. Next: the
  alive count (lifetimes, emission count, the consume/append balance across the swap), then the draw itself (PS b0 rows, the
  depth test), then side by side with the original and E01 checked again after the viewport fix.

### 2026-10-17, E07: the alive count answered, the m37 system is the bonfire embers (Reference2001, Native2002-2005)

- Read (2A6A30 decompiled with `I0066GxReach.java`): emit binds u0 random seed, u1 particle buffer [current] (append), u2 counter,
  initial counts all −1, all 0 on the system's first emission; update clears the counter to {0,1,1,1}, binds u0 [current]
  (consume, −1), u1 [previous] (append, 0), u2 counter (0), DispatchIndirect, copies the counter into the draw and dispatch
  arguments; then current and previous swap. The native hidden counters (Native1997) follow this exactly.
- Witness `ffx_simulation_witness` (both modes: Reference with `ffx-draw-observe.flag`, Native with `ffx-record.flag`; per system
  S: simulating calls, emissions S+0x3968, capacity, active, the CPU images of ccbSceneParam S+0x40, its emit info S+0xA0,
  ccbEmitionSeed S+0x1230 and its longevity S+0x1430, the force-field sources): **at m37 the original has one GXVfx standard
  particle system** (capacity 128), emitting on 4.3% of frames, at most 3 particles, longevity 0.667 (Reference2001: 384 emissions,
  754 particles over 9,000 frames); **the native one is the same** (Native2002: 360 emissions, 703 particles over 7,800 frames, the
  same emitter position, seed and longevity words). Its live count is 2-5 in both: the "3-4 alive" of Native2000 was right.
- The particle record decoded (VS reflection: halves packed in pairs): longevity 0.79/0.667, scale 0.0100 × 0.0091, WeightColor
  (1.94, 0.78, 0.076, 1): **orange embers of about 1 cm at the bonfire**, not snow. m40 (Native2005) has the same system at its
  bonfire (args 5, counters 2 and 5). The node kinds of both modes are equal (Reference/Native2001-2002: one `3D65878`, one
  `3D65CC8` = GXForceFieldWindAreaAppearance, no draw); the classic draw counts per frame are equal (29/10 ≈ 60, 2D/4 ≈ 3.8,
  31/10 ≈ 2, 29/1 = 1). **The m37 falling snow is not a GPU particle system** (EFFECTS-E07's attribution, INFERRED, is refuted
  for m37): it is among the classic draws, drawn natively but not visible (side by side, Native/Reference2003): open, next.
- The default texture: no classic draw binds it at m37 or m40 (`native_ffx_default_uses` empty, Native2004-2005; only E07's CS
  defaults use it). `ffx-default-null.flag` (A/B witness) binds it null; Native2004 (null) and Native2003 (white) equal for E01.
- The outer-context viewport (PS b0 +0x1E0, 263C70: a draw parameter in pixels divided by the bound viewport's width and height):
  zero at m37 because the parameter is zero; the faint rectangle at the m37 distortion quads predates it (Native1965), open.
- Side by side (D-0181 shots, same moment): m37 (Native/Reference2003) and m40 (Native/Reference2005): the bonfires as the
  original's; no new difference. Frame rate unchanged with E07 (m37 54 frames/s with or without, debug layer; m40 60).
- **E07 on by default** (`no-ffx-gpu.flag` opts out, D-0186). Build: the texture-ingress test link now takes `effect_stage.obj`.
- Zone witnesses with E07 on by default (Reference/Native2006 m45, 2007 m31, 2008 m50, `ffx-draw-observe` / `ffx-record`): each
  zone has one GXVfx system, the bonfire embers, with the original's numbers (m45 393/9,000 against 295/3,000 frames natively
  at 25 frames/s with the debug layer: the same rate per second; m31 380 against 352, m50 376 against 358); the node kinds
  equal (m45: one `3D65878`, two wind areas); no fault. Side by side: no E07 difference; the wave-1 residuals (m31 crosses, cloth,
  wheels) unchanged. m45 measurement (no debug layer): 59.0 frames/s with E07, 59.1 without (Native2009-2010).
- The faint rectangle at the distortion quads is plain at m45 (left of the bonfire); `ffx-viewport-zero.flag` (A/B witness:
  the outer viewport cache left zero, as before b816d510) gives the same rectangle (Native2011 against 2012): it is E01's open
  point, not the viewport change.
- Open, in order: the m37 snow (a classic draw drawn but not visible), the distortion quads' rectangle (INFERRED: the frame copy
  they sample differs from the target they draw on), depth-writing draws, the key order with the forward rows, E02 and the other
  families.
- Pre-merge review (Opus, read-only): nothing blocking. Applied: the stage takes the records (releases, creation bytes) before
  any early return, and a frame the scene pass does not encode keeps them for the next (`EffectStage::Retain`); the creation
  bytes stay with the stage until release, so a reopen recreates the buffers; `Close` and the quarantine now cover the E07
  device objects; `PrepareGpu` builds into locals and publishes last; the readback witnesses, `native_ffx_gpu_object` and the
  getter's outside-scope lock run under `ffx-record.flag` only; the view descriptions are copied at their exact sizes (20/24);
  a structured clear covers the view's elements only. Left (unobserved): a compute read of the scene copy, an SRV and a UAV of
  one buffer in one dispatch, a frame dropped before the render worker sees it (its releases are then lost). Native2013
  (defaults) and 2014 (`ffx-record`) at m37: no fault, the statements encoded, the embers' numbers unchanged.

### 2026-10-17, the m37 snow drawn (Native2015-2021, Reference2017-2021)

- Per-draw witness (`native_ffx_draw_dump`, `ffx-record.flag`: every classic draw of frames 1800 and 2400, its programmes, bank
  states, textures by FFX id, first vertex, VS/DS/PS b0 heads): the snow is about thirty pass-0x29 draws of 89 one-control-point
  patches each (stride 36, POSITION with the size in w, two COLOR), programmes GXFfxtessellatePointSprite (VS/HS/DS/PS, PS 6,752
  bytes), FFX texture 11441, spread over the courtyard (Native2015).
- Its VS sizes each flake by `g_screenSize.y` and `g_recScreenSize` (VSCommonBuffer +0x130..+0x13F): natively (0, 0, inf, inf), so
  every flake had no size (Native2016); in the original (1280, 720, 1/1280, 1/720) (Reference2018, `ffx_vs_common`: the image the
  GX UpdateSubresource wrapper 1920300 receives from slot 27 at 266308, VS block = draw context +0x140).
- Slot 27 does not write them: `263330` (the draw context's vt+0xC0) stores w, h, 1/w, 1/h at +0x270..+0x27C, called by `375DF0`
  at 3760D4 with the first viewport of the cache at `[dc+0x20]+0x258` (count `+0x4D8`) (a hardware write watch in Reference2019
  gave the writing instruction 263363; the watch itself made both runs fault and was removed). `[dc+0x20]` is the inner GX context
  in both modes (Reference/Native2020, `ffx_draw_context`); its cache held one viewport in the original and none natively. The
  slot-27 parameter table (`ffx_slot27_witness`) is equal in both modes (Reference/Native2017).
- Natively the inner context now reports the scene viewport too (+0x4D8 one, +0x258 the D3D11 viewport), as the outer one does
  (+0xC8/+0xCC). Side by side at m37 (Native/Reference2021): the snowflakes drawn natively as in the original; no fault; 54.5
  frames/s with the witnesses.

### 2026-10-17, the distortion quads' rectangle (Reference2022, Native2023-2030)

- Read (the programmes, `Get-DxbcDisassembly.ps1` on the I-0063 cache): `GXFfxdistortion.vpo` writes the screen position of each
  vertex scaled by `g_fbTexCoef` (VS +0x160); `GXFfxdistortionBump.ppo` samples t0 (the frame copy) at that position plus a normal-map
  offset (at most 0.01 × `g_waveStrength`), multiplies by `g_color` and `g_vColorScale`, and writes alpha `g_color.w`. Slot 23
  (263300) sets `g_fbTexCoef` = (1, 0) when the builder 396F30 has no texture of its own (t0 = the frame copy, slot 8 with null),
  else (0, 1) (the quad's own texcoord).
- Witness `ffx_distortion_witness` (slot 27, both modes, draws whose t0 is the frame copy): the original (Reference2022) and the
  native (Native2023) agree: FXR blend type 0 (bank +0x08, no blending), `g_fbTexCoef` (1, 0), `g_color` (1, 1, 1, a), colour scale
  (1, 1, 1, 1), colour blend type 0, the near/far and wave words. So the quad writes the copy, unblended, over the target.
- The lit HDR (`world-pictures`, Native2026): the quad is in the HDR before YEBIS (its alpha 0.71 or 0.24 marks it); the copy taken
  after the forward rows instead of before (Native2025, A/B) changes nothing.
- **Cause**: the copy natively is taken before the 0x29 effects, so the quad, drawn after them (0x2D), replaces the bonfire's
  flames and embers inside it by the scene without them. With the copy remade between 0x29 and 0x2D (Native2027) the rectangle is
  gone and the fire shows through the quad as in the original. INFERRED: the original's copy for pass 0x2D holds the 0x29 effects
  (its single copy, FFX-FAMILIES-PHASE2 §8, is placed by the pass 0x29 stream; where exactly is not read and not needed: D-0157).
- **Native design**: the forward pass keeps its copy (t45 of the forward rows); the effect stage remakes the copy before the first
  pass after 0x29 that samples it (0x2D, else 0x31), only when such a draw exists (`EffectCopyPass`; one `CopyResource`, its own
  frame-graph passes `effect_copy` / `effects_after_copy`, plan-key bits 31-32). On by default; `no-ffx-effect-copy.flag` keeps
  the forward copy only (A/B).
- Side by side with the defaults (D-0181 shots): m45 (Native2028 / Reference2006), m37 (Native2029 / Reference2021), m31
  (Native2030 / Reference2007): no rectangle at the distortion quads; no fault; the wave-1 residuals (m31 crosses, cloth, wheels)
  unchanged.
- Step review (Opus, read-only): barriers, loop, descriptors and witness correct. Applied: the effect pass's depth transition now
  precedes its graph declaration (the shadow recorded DEPTH_WRITE against the plan's read state on every effect frame since N6, so
  no effect frame ever ran its plan), and the copy passes the plan key promises are declared even when `Encode` stops early. Open
  (INFERRED design): when both 0x2D and 0x31 sample the copy, 0x31 sees it without the 0x2D output.

### 2026-10-17, spells: Alpha's Fire Orb at Firelink Shrine (Reference2031-2032, Native2033-2036)

- Pilot `Run-AlphaCast.ps1` (one process: the trace job dies with the shell that starts it; Reference2031, driven by hand from a
  second shell, needed `Restore-StartupTrace.ps1`, ok): a copy of the Shebang save (the m45 zone save), Load Game, the list scrolled
  to Alpha (level 68, Firelink Shrine, Fire Orb attuned, pyromancy flame in the right hand), four left clicks (the cast) with
  screenshots after each. The two offline notices sit at the same places in both modes (OK at 427, then 450).
- The original's passes during the casts (Reference2032, `ffx-draw-observe`): 29/10, 29/1, 2D/4 as at the zones, plus **29/4
  indexed** (about one a frame at Firelink, with or without casts) and **2D/5 strips** (the casts). Natively (Native2034) both were
  skipped: the renderer classes 3D7E8C0 (slot 8 = 39E900, the `Draw(4)` builder) and 3D7E740 (392560, 16 commands) were outside the
  allowed list, and 3D65F88 (27EA70, two commands a frame) too. The orb itself (E01 and E07) was drawn as the original's.
- 3D7E8C0 and 3D7E740 now run as the other builders (Native2036: 2D/5 drawn during the casts, about 200 per 600 frames; no fault,
  no skipped draw, no pipeline failure, 21 PSOs). Side by side (`pair-cast-*.jpg`, Native2036 / Reference2032): the cast flash and
  the orb as the original's (the shots are not at the same instant of the animation).
- 3D65F88 (27EA70, the indexed lists) under `ffx-model-particles.flag`: Native2035 faulted after the load (reading [0+8] at
  14A1EE, from 2822A6 in the class's renderer through 29A451): open, next.
- The step review's depth fix holds: no `use_state` mismatch in Native2036 (2042 in Native2028), no debug-layer message.

### 2026-10-18, E03 at Firelink, and the whiter bonfire core (Reference2037-2039, Native2040-2041)

- E03 witness (`reference_ffx_model_draws`, Reference with `ffx-draw-observe`: 1400636F0 per model data name): at Firelink the only
  E03 draw is **s11600**, once a frame, pass 0x29, LOD 0, with bones (Reference2037). Skipping it in the original
  (`ffx-skip-e03.flag`, Reference2038) changes nothing visible at Alpha's view: E03 stays open (EFFECTS-E03-MODEL-PARTICLES-NATIVE,
  a native FLVER submission at 1400636F0; its main witness is m51's instanced grass).
- The whiter bonfire core (every bonfire natively, the INFERRED "whiter m40 core" of N6): witness `ffx_flame_witness` (slot 27,
  additive draws of pass type 3): the colour scale `g_vColorScale` (PS +0x90) is 0.652 / 1.957 in the original against 1.5 / 4.5
  natively, everything else equal (glow scale, blend types, soft words, the b6 fog block but its w words 0 / 0.01).
- Read: slot 13 and its siblings multiply the colour by the draw context's +0xAA8, which 299E40 sets to 1.5 / data+0x54, and
  data+0x54 is 140089140(pipeline): `[[GXRS+0x4DD8]+0xCB8]+0x21B0` (a float in XMM0) when the view pipeline's +0x50 bit 10 is
  set, else 1.0. The original's value is the world view's Tone Map exposure (2.3 at Firelink, 2.0 at m45, the native feed's
  `native_yebis_tonemap` exposure at those zones); natively the pass context's pipeline block is zeroed, so 1.0: every effect was
  drawn exposure times too bright.
- Native: 140089140 is detoured; for the native pass context's pipeline it answers the native exposure (`native_effect_exposure`:
  the YEBIS feed's blended Tone Map word 0 when the view's bit 10 gate is open), any other pipeline goes to the original.
  Native2041: colour scale 0.652 as the original's; the Firelink flames orange and detailed as the original's
  (`core-before-after-reference.png`: Native2040, Native2041, Reference2039).
- Note: the b6 fog block differs only at +0x0C and +0x2C (0.01 natively, 0 in the original), words the flame programme does not
  read (cb6[0].w, FC_ExtFogParam.w).
- Side by side with the defaults (Native2042 m45 / Reference2006, Native2043 m37 / Reference2021, Native2044 m31 / Reference2007):
  the bonfires orange and detailed as the original's, no fault.
- Step review (Opus, read-only): nothing blocking. Applied: the exposure is cleared at the start of each feed (no carried value on
  an early return) and given only when our tone map multiplies the scene by it (its pass on, no auto exposure); the stale-flag
  cleanup of `Run-StartupTrace.ps1` covers the new flags; the E03 witness's pass is -1 outside a draw callback. INFERRED: the
  feed's gate (the view context's +0x50 bit 10 and [144961F70]) stands for 140089140's pipeline bit 10.

### 2026-10-18, closing the workstream (Native2046-2048)

- Spells: Alpha's attunement (Reference2045): Great Soul Arrow, Fire Orb and two miracles; only Fire Orb casts without changing
  equipment (the pyromancy flame), and the attune menu did not close under the pilot's Esc, Backspace, Q or F: the sorcery and
  miracle families stay to witness with an equipment pilot.
- Census of the FFX renderer classes (the vtables sharing slot 11 = 267B20, slot 8 = the draw): allowed now 3D7E5C0, 3D7E440,
  3D7E680, 3D7E800, 3D7E8C0, 3D7E740, 3D7E4A0 (378F50, the same ring and slot 27; not yet witnessed), the GXVfx 3D65878 and
  3D65CC8 (E07), 3D65F88 (E03) under its flag; not allowed (never met at the witness zones, counted as skipped when met): 3D7E380
  (3709B0, no ring), the GXVfx 3D659C8, 3D65A88, 3D65AF8, 3D65D38, 3D65DC8, 3D65E38, 3D65EA8, 3D660B8.
- Final side by side with the defaults: m40 (Native2046 / Reference2005), m50 (Native2047 / Reference2008: the eclipse ring as the
  original's), m30 (Native2048 / Reference1968: the bonfire and the tower torch as the original's; the m30 lighting residual is the
  lightmap delivery, not an effect); m45, m37, m31 above (Native2042-2044). No fault.
- Open, carried to the wave-1 residuals: E03 (a native FLVER submission at 1400636F0; m51 grass), the sorcery and miracle families,
  the key order with the forward rows, the depth-writing passes 0x20/0x21 (never met), the classes listed above, the copy seen by
  0x31 when 0x2D also samples it.
- Final review (Opus, read-only, `git diff 9b10c485`): nothing blocking. Applied: a renderer whose vtable reads 0 is skipped (the
  allowed list holds 0 for a flag off); 3D7E4A0 stays on, carried by Native2046-2049 without fault, its class not yet met; stale
  comments and hook counts; the E03 witness prints the pass as a number. Noted: an executor `logic_error` raised inside `Encode`'s
  callback is stored as the stage failure (it surfaces at the next graph pass).
