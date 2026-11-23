# I-0105 tranche 1: how DS3 and Elden Ring draw the model particles (E03, FXR 605)

2026-10-28, static. DS3 1.15.2 read by the pilot (Opus 5.5) in Thomas's GhidraMCP (the copy `ghidra-mcp-ds3`, read-only) and the
exported `decompiled/` files. Elden Ring 2.7.1 read by a `ghidra-research` worker (Sonnet) with headless Ghidra on the mirrors
`er-mirror-i0070-a` and `-i0072-a` ([Invoke-ErGhidra.ps1](../I-0104-native-effect-method/Invoke-ErGhidra.ps1),
[I0104Vt.java](../I-0104-native-effect-method/I0104Vt.java), [I0105Callers.java](I0105Callers.java)); outputs under
`evidence/I-0105/er/`. The pilot re-read the two facts the design rests on (marked « checked »).

## 0. Answer first

- **DS3 draws each model particle through the engine's model draw**, the mesh and material loop of the world's models
  (`140063CB0`), on the pass context's model context `+0x460`. Natively that context is null: with the class allowed, Native2035
  faulted in `5A120`'s `149CB0`, the scene constants' upload. Nothing in the chain is effect-specific below `29ADF0`.
- **Elden Ring does the same** (H2 supported): its `GXFfxAppearanceModel` draws each particle through `GXFfxFlverModelRenderer`, a
  model control, an adapter and `141AAA240`, the equivalent of DS3's `636F0`, then the per-mesh loop `141AAB2F0` (checked: the two
  callers of `141AAA240` in `callers1.txt`; `141AAA240` calls `141AAB2F0` in `model-core.txt`); that its other caller `141C41DE0`
  is a world model path is INFERRED (its owner class not read). The skeleton is a CPU array of world-space 3x4 bone rows, 0x30 bytes each.
- **The native follows it**: the model particle is not an effect statement but a model draw. Its inputs (the model, the LOD, the
  400-byte draw state, the bone rows) go to the native model path as a candidate of the main walk, drawn as a forward row of its
  pass. What ER does per particle and the native does not copy: a full draw state and three constant uploads per particle per mesh
  (ER-side redundancy, from the decompilation); the native keeps the model's prepared state between frames (records, I-0090) and
  makes only the per-frame blocks. What it keeps as ER: one draw and one palette upload per particle per mesh, no instancing (a
  known cost, not measured).

## 1. DS3 1.15.2, the chain (read by the pilot)

| Step | Function | What it does |
|---|---|---|
| Renderer class `3D65F88` slot 8 | `27EA70(appearance, drawCtx, fxr, matrix)` | per particle: evaluates the FXR properties (position, colour, scale), calls the per-instance `GXFfxFlverModelRenderer` `[this+0x48]` (vtable `143D6A298`) slot +0x28 (`29B2C0`, the world matrix and the instance's bone rows) and slot +0x20 (`29ADF0`); the return address after the latter is `2822A6`, Native2035's fault stack |
| Model renderer draw | `29ADF0(renderer, passCtx, fxr)` | the 400-byte draw state (`4DF20` defaults, the FXR fields `+0x10..+0x98`), `1A8580(args, passCtx+0x750, passCtx+0x460, state)`, `59470(passCtx, args, ..)`, `1A86C0(args)`, `2BA3A0([renderer+0x28], args, 0)` |
| Arguments | `1A8580` → `236290` | `args+8` = `passCtx+0x460` (the context's address), `+0x18` LOD 0, `+0x30`/`+0x38` all bits, `+0x40` 0xFFFF, `+0x44` 0xFF, `+0x120..+0x17F` identity object rows (current and previous), `+0x180` 4, `+0x1A8` null; the state copied at `+0x240` |
| Scene constants | `59470` → `5A120` → `149CB0` | GX uploads through `[passCtx+0x460]+8` (null natively: the fault); its light block (rows, blend vector, the same formula as 5B3D0's instance SH) from the FXR's light that 29ADF0 writes at `args+0x3E0..+0x424` after 1A8580 (`+0x424` = 1), else from the state's banks |
| Commit | `1A86C0` | GX binds through `[args+8]` |
| Bones | `2BA3A0(instance, args, 0)` | with `args+0x48` clear (236290 clears it, nothing sets it): the adapter's vt+0x10 with the instance's bone array `[instance+0x70]` (the asm, `1402BA431`..`43F`); set, a temporary array of the rows times the instance world |
| Adapter | `2C0130` → `636F0(table, model, args, bones)` | returns when `model+0x2B4` is clear; clamps `args+0x18` to `model+0x1E4 - 1`; material table `args+0x1A8` or `model+0x1F0`; `63CB0` |
| Mesh loop | `63CB0` | every mesh with `+0x10` bit 0 at the LOD (`model+0xB4`), whose material passes the open masks (`args+0x30..+0x44`); `64610` draws it with shader pass `args+0x20` = 0 (Fwd) |

So a model particle is drawn as a forward row of the pass its command runs in, every material class allowed, with shader pass 0:
the native forward row 0x29 of category 0x29 (class mask 0xFFFF). At Firelink the only E03 draw is `s11600`, pass 0x29, LOD 0,
with bones (FFX-DRAW, Reference2037).

## 2. Elden Ring 2.7.1 (the worker's report, the two facts above checked)

| Step | ER | DS3 |
|---|---|---|
| Appearance slot 8, per particle | `GXFFX::GXFfxAppearanceModel` (vtable `14306C170`) `141CDD7B0` | `27EA70` |
| Renderer world matrix | `GXFfxFlverModelRenderer` (vtable `14306C6D8`) vt+0x28 `141CF0D80` | `29B2C0` |
| Renderer draw | vt+0x20 `141CF07A0(renderer, passCtx, params)` | `29ADF0` |
| Draw state | `141C2A100(state, passCtx+0x8F0, passCtx+0x570, params)` (`GXSR::GXFlverModelDC`) | `4DF20`, `1A8580` |
| Scene constants | `141AB7D80(passCtx, state, ..)`, also called by the world's model draw items (`141AB6570`, slot 6 of `GXFlverModelDrawItemBase`) | `59470` |
| Model control | `141D473A0(renderer+0x30, state, 0)` (`GXFfxModelCtrl`) | `2BA3A0` |
| Adapter | `GXFfxFlverModelDataAdapter` slot 2 `141D701E0` | `2C0130` |
| Model draw | `141AAA240(helper, model, state, bones)`; callers `141D701E0` and `141C41DE0` (a model path that uploads its own matrices; « world » INFERRED, its owner class not read) | `636F0` |
| Mesh loop | `141AAB2F0`, then `141AAC2B0` per mesh (index buffer, topology, the state commit and the draw on the D3D12 device context, as I-0104 §1) | `63CB0`, `64610` |

- The bones: `141CF0D80` composes the model control's local pose (`ctrl+0xB8`) with the world matrix into `ctrl+0x80` (3x4 rows,
  0x30 bytes); `141AAA240` multiplies them by the model's inverse-bind array (`model+0x2F8`) and binds the result at VS slot 5 from
  frame-transient memory.
- The context: the effect callback's pass context is the same object the world model draws use (`passCtx+0x570`, the D3D12 device
  context); DS3's `+0x460` against `+0x450` is the same split by another offset.
- ER's defects on this path (from the decompilation): per particle and per mesh a full draw state, three constant uploads, the bone
  array computed up to twice and uploaded, one draw each, no instancing; the pipeline built at draw time when missing (I-0072's
  mechanism, inherited from the model path, not re-read here). The native does not copy them: its model records keep the prepared
  meshes and pipelines between frames, and its pipeline cache (I-0072) covers these programmes like the world's.

## 3. What the native does (tranche 1's code)

- `effect_draw`: on the native pass context, `59470` and `1A86C0` return without their GX uploads and `636F0` hands its inputs to
  the caller (`ModelSubmission`): the model, the clamped LOD, the material table override, the state at `args+0x240`, the bone rows
  (the model's `+0x1C` of them) and the command's pass and key. The class `3D65F88` runs by default (`no-ffx-models.flag` opts out).
- `RecordNativeEffects`: each submission's pose made from its bone rows as `636F0` receives them, full rows not composed with cloth
  gate rows (`ComposeNativeModelPose`, shared with the engine's pose updates; INFERRED: `29B2C0`, its `2B9DE0` and the per-mesh bone
  binding behind `63CB0`, the executor `143D42010`'s slots 0 and 1, are not read), kept in the scene (`effect_models`).
- Not carried, not read further: 29ADF0's `args+0x188` (FXR[0]: -1 or 0) and bit 21 of `args+0x200` (FXR+0x34), and the bits 5A120
  sets in `args+0x18C` inside the skipped 59470 (63CB0 tests bit 8).
- The main collect joins the effect record before the main walk's admission and adds each submission of pass 0x29 with the model's
  own material table as a candidate (kind 3): its group the state, its pose the particle's, its placement the identity (the args'
  object rows), its fade a full track at the asked LOD with the state's opacity and fade bytes, no fade owner (no parameter
  collection: generated values stand), no animation object (236290's preparation bytes clear), category 0x29 (the forward row 0x29
  alone, class mask 0xFFFF). The rest is the world's admission, pass preparation, forward lights and records.
- Not drawn yet, counted (`native_effect_models`): submissions of the passes 0x21, 0x2D, 0x20 and 0x31 (the native has no model rows
  in them) and those with their own material table.
