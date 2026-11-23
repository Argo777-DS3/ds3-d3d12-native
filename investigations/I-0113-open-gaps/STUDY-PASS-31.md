# I-0113: pass 0x31 at half size in the original (static, tranche 16, for tranche 17)

Read by the `ghidra-research` subagent (headless Ghidra 12.1.3 on `ghidra-mirror-i0068`, read-only, 2026-11-02 evening) for the
gaps pilot; its exports are in `evidence/I-0113/t16/pass31/` (`d1/` to `d26/` by function, `find_*.txt`, `fieldmap_89960.txt`).
Non-canonical until the pilot re-reads the sources a transcription rests on. The live facts it explains (OBSERVED,
`reference_effect_depth`, `i113-t16-c-original` at 1280x720): while the 0x31 effects draw, the target is 640x360 and the depth a
separate 640x360 D32_FLOAT_S8X24 texture bound read-only (DSV flags 3) and sampled at t3, never a copy destination.

## 1. The depth copies and the half-size depth (VERIFIED in code)

`140103510` (the scene pass-setup recorder) records, in key order:
- **Sub 0x28, order 7**: when `[settings+0x4158]`, a type-4 entry `1400BEA50` → `141920260` = CopyResource(dst
  `[[settings+0xFD0]+0x40]`, the full-size depth copy t3; src the main depth `[[R13+0xE0]+0x40]`); then, when `+0x4158`,
  `+0x4159` and the filter `+0x3A20` are set, `1401CB840(settings+0x3958, …, sub 0x28, order 7, src [settings+0xFD0])`: the depth
  downsample (`+0x415B` swaps the source for `1400202F0(settings)+0x20`). So the copy 0x29 reads is made before 0x29.
- **Sub 0x2A, order 7**: a second CopyResource entry when `+0x415A` (forced to 1 at 140DFF456): the copy before 0x2D.
- `140102790` once a frame: the subs 0x2F and 0x32 below.

The filter at `settings+0x3958` (built by `1401CB6B0`, called at 140085540:336; vtable 143D5E070): a group of (w>>1, h>>1)
with depth format code 0x14 (DXGI 20, D32_FLOAT_S8X24_UINT), its texture at `settings+0x3980`, and a (w>>2, h>>2) group; VS
`ScreenSpaceQuadShader.vpo`, PS `DepthScale2x2OutputDepth.ppo`. Its draw (`1401CB920` → `1401CB9D0` twice: half from the copy,
quarter from the half): the source SRV at t1, cb1 the tap offsets 0.5/size, a depth-only target (writable DSV), cleared to 1.0,
depth bank `+0x1E0` (LESS_EQUAL, write). The 2x2 reduction is the minimum of four taps writing oDepth (an earlier DXBC study,
`SSAO-WAVE1-READY.md`). The half depth is not refreshed after sub 0x28: the 0x31 particles test against the depth as of just
before 0x29.

## 2. The half-size target, its binding and clear (VERIFIED)

- Created in 140085540 (140086AC5..DBA) when the descriptor's `+0x30` bit 8 is set (`Graphics.IsEnableScaleRender`, default 1
  INFERRED: its reader is in packed code): `1400DCB80(settings+0xF70, desc)` with (w>>1, h>>1), colour code 10 (DXGI 10,
  R16G16B16A16_FLOAT, INFERRED from the depth codes equal to DXGI values), name `EsramShared_Top`, its depth the filter's half
  depth (`+0x3980` adopted, not created). Then `settings+0x3EC3 = 1`.
- `140102790` (gated by `+0x3EC3`) records sub 0x2F: order 1 `1400BE8E0` = ClearRenderTargetView to (0, 0, 0, 1); order 3
  `1400BF730` binds the half RTV `[settings+0xFA0]` with the half depth's read-only DSV `[settings+0xFB8]` (`1400CCCF0` makes a
  writable and a read-only DSV per slice, flags 3); order 5 `1400BF4C0(ctx, 0xF8, 8)` snapshots the state. The 0x31 key finds
  no block of its own and falls back to the 0x2F snapshot: the half target stays bound for every 0x31 draw.
- t3 at 0x31: `140299E40`'s switch on the engine pass (cases 0x30 and 0x31) replaces the depth SRV by the half depth's
  `[[ctx+0x4D0]+0x28]+0x48` (ctx+0x4D0 = the filter, set at 140142A50); the frame copy is not replaced.

## 3. The composite (VERIFIED code; programme names not found)

Sub 0x32: order 3 `1400BE9F0` (the target's end/restore); order 7 `140102A80` with a 0x40-byte block: byte 0 = bilateral when
`[settings+0x415C]` (`Graphics.IsEnableBilateralUpscale`, default 0) or the Effect param's `Force HQ UpScale`
(`[[GXRS+0x5360]+0x4B]`, default not read); `+8` the main HDR RTV, `+0x10` the half target's SRV, `+0x20` the half depth's,
`+0x2C/+0x30` the full size, `+0x34/+0x38` the half size. The draw into the full-size HDR: blend bank `+0x1D0` (ONE /
SRC_ALPHA colour, ZERO / ONE alpha: result = layer.rgb + scene.rgb × layer.a, per FFX-DRAW.md's bank dump), depth bank
`+0x1D8` (off), raster `+0x208` (solid, no cull), samplers banks `+0x2E0` (point clamp) and `+0x2E8`; the plain PS
`[[lib+0xB98]+0x10]` (t0 the half colour; INFERRED a plain sample: the same slot is the copy draw's PS), the bilateral PS
`[[lib+0xBE0]+0x10]` (t0, t1, t6 the half depth, a cb of 1/half and 1/full sizes and the camera's near and far), `lib` =
`[GXRS+0x4DD8]`. With the clear's alpha 1, the layer is premultiplied colour over accumulated transmittance (INFERRED).

## 4. Which effects take 0x31 (VERIFIED reads, defaults partly)

`1402997E0` chooses each effect entity's sub-pass by its kind `param_3`: 5 → 0x2D; 2, 4 → 0x29; 1 → 0x21, or 0x31 when
`DAT_144962375`, but 0x21 again when « near »; 3 and others → 0x29, or 0x31 when `DAT_144962375` or « near »; 6 → 0x31 when
`DAT_14473DAD6` (default 1), else 0x29, then case 3's overrides. `DAT_144962375` defaults to 0; both are FFX debug settings
(« Force-Render BlendBuffer », « Function-Scale Render », `14024F9A0`). « near »: the entity's view-space z at most
|size × (2.0 − env[0x158]) × cam[0x64]|. No effects-quality dependency in these reads. The kinds' meanings are not identified.

## 5. Not reached

The composite's PS names and maths (read the hashes in a capture); bank `+0x1D0` read live; colour code 10 beyond the stored
Format; `ctx vt+0x10` exactly `140005F00`; the `Force HQ UpScale` default; the kinds' meanings.
