# I-0140 tranche 10: the light shafts 10003, read (2026-11-15)

Class `3D65A88` (FFX 10003, `GXVfxParticleAppearanceLightShaft`), its draw slot 8 `271B40` (decompiled whole in
`evidence/I-0140/ghidra-10003/271B40.c`, 1,726 lines), read by the E pilot with GhidraMCP (Thomas's GUI, DS3 1.15.2) together with
the functions it calls. READ means read in the decompiled code (Ghidra's C, the wrappers' bodies checked one by one); the runs are
in the RESULT (« Tranche 10 »: `i140-e30` faulted in 1400C4F10, `i140-e31` drew the shafts).

## Where its targets come from

- 299E40 (the FFX pass command) builds the draw context and stores at its `+0x10` a pointer to the command data's `+0x58`, whose
  first word is the data's `+0x10`: the scene control (the same object 299E40 tests at `+0x5B9`, `+0xD0`, `+0xE0`). 271B40 reads
  `S = *[dc+0x10]` and takes `S+0x238` and `S+0x240` (READ).
- 24F2C0 (the scene control's view attach) makes them: two objects of vtable `143D1D638` (0xA0 bytes; `+0x70` an embedded object of
  vtable `143D1D610`), each given a render-target resource by 2BD50 at **half the view's width and height (integer division)**, format
  **0x1C, R8G8B8A8_UNORM**, one mip, one slice (READ). I-0068 FFX-FAMILIES-PHASE2 §6.2 had found no reader of `+0x238/+0x240` in its
  static search; 271B40 reads them through `[[dc+0x10]]+0x238` (a pointer it loads first), which that search did not follow. The
  claim there (« no reader ») is withdrawn for these two.
- Natively 24F2C0 never runs: `SceneControl` zeroes `sc+0x218..+0x248`, so both are null.

## The object shape 271B40 relies on

- `vt+0x20`: the resource (what CopyResource 1920260 takes; it reads `+0x18` of it, the D3D11 resource).
- `vt+0x40`, `vt+0x48` with a mip: the width and height (used for the clear's viewport).
- `+0x48`: the shader view.
- `+0x70`: an object whose `vt+8` returns the render-target view.
- A view's `+8` is its resource; SetRenderTargetsWrapper 5F00 reads `GXResource_GetKind(res+0x18)` (19281C0) and, kind 3,
  `GXTexture_GetDescAndSize(res+0x18)` (1924730): with `res+0x18` null, the kind is `*([res+0x20]+0x18)` and the description the
  twelve words at `*([res+0x20]+8)` (READ).

## The statements, in order (READ)

1. Save the outer context's render targets (`outer+0x18`, `outer+0x58`) and viewports (2B970); unbind PS t0..t5 (6100).
2. Clear both targets to (0, 0, 0, 0) (1920200), bind the first (5F00), viewport at its size (6080).
3. The mask: programme `(appearance+0xA4)+5` of the shader set `[sc+0x28]+0x168` (entries of 0x38 bytes: `[0]` layout, `[1]` VS,
   `[5]` PS), PS s9 = bank `+0x2E8`, s1 = bank `+0x2D0`, raster `+0x208`, blend `+0x08` (opaque), depth `+0x1D8` (off), t3 the draw
   context's depth `[dc+0x998]+0x48`, t0 the appearance's texture `+0x20`; the draw context's constant rows written (`dc+0x5B0..`,
   `+0x6B0..+0x6D8`) and uploaded (vt+0xA8, 2638E0, 2637B0); a 4-vertex strip (topology 5) of 0x50-byte vertices from the ring.
4. The blur, `[renderer data+0x70]` times (when above 0): programme `+0xA8`, blend `+0x10` (alpha), a full quad (±1) with
   `1/(n+1)` weights in its vertices; each step unbinds, copies the target it will draw into from the other (CopyResource), binds it
   and samples the other: step 0 copies 1 into 2 and draws 2 from 1, step 1 copies 2 into 1 and draws 1 from 2, and so on; an odd
   count ends with a copy of 2 into 1. The result is always in the first target.
5. Bind the saved targets and viewports again; when the saved view is known (`outer+0x18` and its `+8` non-null) a scissor rectangle
   from the shaft's screen bounds times the scene's size (2B8E0) and raster `+0x210` (no cull, scissor on).
6. The composite: programme `+0xAC`, t0 the first target, t1 the appearance's texture `+0x28` (when set), blend `(appearance+0x30)`
   (0: bank `+0x10`, else `+0x08 + (n-1) x 8`), depth `+0x1D8`, the constants again, a 4-vertex strip; then depth `+0x1E8`, the saved
   viewports and raster `+0x208` back.

The outer wrappers call the inner GX wrappers themselves (5F00 → 1920F40 OMSetRenderTargets, 6080 → 1920CD0 RSSetViewports, 2B8E0
→ 1920CF0 RSSetScissorRects, 6040 → 1920CA0, 6100 → 1920DA0); 1400C6940 clears its dirty mask before testing it and commits
nothing in 1.15.2 (READ: its first statement).

## Native design (tranche 10)

- Recording (`effect_draw`): the two targets as native objects of the shape above, named at `sc+0x238/+0x240` for the execution
  only; a scene view in the outer cache `+0x10/+0x18` (the pass's own target, as the original's pass binds it); the inner wrappers
  1920F40, 1920CD0 and 1920CF0 hooked (the target, the first viewport and scissor rectangle kept in the context's state);
  ClearRenderTargetView and CopyResource on the targets handed on as statements (`ClearTarget`, `CopyTarget`); the class allowed by
  default (`no-ffx-lightshafts.flag` opts out), run only where the command's scene control is the one whose slots name ours.
- Drawing (`EffectStage`): two R8G8B8A8_UNORM textures at the frame's half size, their views, the draws into them (one target, no
  depth), their clears and copies, their shader views (`NativeEffectTextureKind::Target`), the record's viewport for them and its
  scissor where the raster entry enables it, the targets back to PIXEL_SHADER_RESOURCE at the frame's end.
- Not reproduced: PS s1 is the stage's static anisotropic wrap sampler, not the bank's linear wrap `+0x2D0` (the stage's s1 is
  fixed for every effect, INFERRED close for a dust texture); the depth view bound with the scene targets is the stage's own.
- 1400C4F10 (a GX state block 271B40 builds first) reads the GX display record `[[144961AB0]+0x88]`, null natively: on the native
  outer context its body is made natively, the 2D virtual extent the scene's size (the block is overwritten before any read).
- Witness: `native_ffx_light_shaft` (each command's draws, clears and copies; the first 8 and every 600th, in every run), the
  stage's `target_draws`, `target_clears`, `target_copies`, `native_ffx_light_shaft_owner_skipped` (in every run) when a
  command's scene control differs, `native_ffx_render_target_drop` when another family binds no view or one not ours.
