# I-0113 gaps 16 and 13: the pyromancy bursts' false colours (study thread, static, 2026-10-31)

Written by the study thread for the gaps pilot to copy into I-0113. STATIC only: nothing launched, no build. Labels as in
`knowledge/LEDGER.md`.

## 1. The spells' effects (VERIFIED, params and FXR read)

- Magic id = goods id: Fire Orb 2402000, Profaned Flame 2455000. Fire Orb: projectile FXR 524022, hit FXR 524023. Profaned
  Flame: cast 524550 / 524551, bullets 12455100 → FXR 524553 and 12455200 → FXR 524552 (Bullet +0x04 projectile FXR,
  +0x08 hit FXR, +0x68 next bullet). None of the six has a 605 model particle.
- Textures (s01000, s01001, s03000, s10031, s11403, s11619..s11640, s50065): all BC7 but s03000 (DXT1); the format is not the cause.
- Draw actions (blend modes: 2 Normal, 4 Add, 7 Unk7):

| FXR | 608 RadialBlur (mask 50065) | 603 BillboardEx |
|---|---|---|
| 524022 Fire Orb projectile | one Add | all Add |
| 524023 Fire Orb hit | one Add, **one Normal** (node 10) | Add, and **11630 Normal** (octagonal) |
| 524551 Profaned cast | **one Normal** | Add |
| 524552 Profaned burst | one Add, **one Normal** | Add, **11640 Normal**, 1000 **Unk7** |
| 524553 Profaned bullet | one Add | Add, 1001 **Unk7** |

  Both bursts with false colours carry a **608 RadialBlur in blend Normal**; it is white (colour key (1, 1, 1), alpha ramp,
  `alphaMultiplier` 2), width 6.25 to 10, blur radius 0.0625 to 0.075.

## 2. How the original draws a 608 (VERIFIED, static)

- Builder `14039E900` (class 3D7E8C0, Draw(4) strips of pass 0x2D, already run natively): draw-context vtable `143D65688`
  slots 19 (`262530`, setup with the action at `[[node+8]+0x18]+0x240` and blend `[node+0x30]` into `ctx+0x964`), 11, then
  **slot 8 `260410(ctx, 0)`**: a null texture, so **t0 = `ctx+0xA70`, the frame copy**, and `ctx+0x9BA` = 1 (the clamped
  sampler `+0x2E8`); then slot 5 `2603E0`: **t1 = the mask** (`[node+0x28]`, s50065); then slot 12, 0xD0, 0xD8 (slot 27 picks
  the programme) per strip.
- Slot 19 sets `g_color` (PS +0x00) from the particle's packed RGBA8 (so each channel ≤ 1), `g_vColorScale` (PS +0x90,
  `ctx+0x550..+0x55C`) from the action's own properties (not multiplied by the exposure factor `ctx+0xAA8`, unlike slot 13
  `260830`), `g_screenCenter` / `g_radialDistance` (PS +0x70..+0x78). Slot 27 (`264230:132`) sets `g_ps_ColorBlendType`
  (PS +0x17C, `ctx+0x63C`) from the action's F2 fields.
- The programme `GXFfxradialBlur.ppo` (ps_5_0, SHA-256 `E7094993FEC9B52F`, disassembled with fxc from the I-0063 cache):
  1. discards where the mask's alpha rounds to 0;
  2. takes 16 taps of t0 from the pixel toward `g_screenCenter` (step `(center − uv) × g_radialDistance / 16`, clamped by
     `g_ps_RadialBlurSamplingLengthMax`, uv saturated) and keeps **sqrt(mean(tap²))** per channel;
  3. multiplies by the mask's alpha; when `g_ps_ColorBlendType` == 2 an overlay with `g_color × g_vColorScale`, otherwise a
     product with it;
  4. **`max(…, 0)`** per channel; alpha = mask.a × g_color.a × scale.a × the vertex's TEXCOORD2.x, zero under
     `g_ps_softSpriteParam.y`.
- The frame copy in the original (FFX-FAMILIES-PHASE2 §8, FFX-DRAW): pipeline `+0x1108`, R16G16B16A16_FLOAT, taken in pass
  0x29; natively the copy token maps to `NativeEffectTextureKind::SceneCopy` and `EffectCopyPass` remakes the copy before
  0x2D, so the 608s read the HDR with the 0x29 effects in it (Native2027, the original's equivalent INFERRED there).

## 3. What the programme can and cannot produce (INFERRED from the code above)

Pilot's note (I-0113 tranche 29, `effect-scan.flag`): no NaN or inf was found in what the 608s sample at Fire Orb's bursts; negative halves (most in blue) were, written inside the effect block (RESULT « Tranche 29 »). The negative branch below would then apply (INFERRED: not tested by removing the negatives).

- With finite inputs each output channel is a non-negative function of the same channel of the scene under the disc: the
  programme cannot invent a hue the scene lacks, and with a white colour key the overlay saturates toward white, not toward
  magenta or green.
- A **NaN** in a channel of the sampled copy survives the taps and the product, and the last `max(NaN, 0)` returns 0 (D3D's
  min/max return the non-NaN operand): **that channel is killed over the whole blurred disc**. Red killed gives cyan, green
  killed gives magenta, red and blue killed give green: exactly the palette of gaps 16 and 13. A `+inf` gives white (product
  branch) or 0 (overlay branch with a white key: `inf × 0`). A **negative** value in the float HDR (a subtractive blend) comes
  out positive through the squares.
- Blend Normal makes the 608 **replace** the scene inside its mask, so a killed channel is seen as a coloured disc (13's green
  sphere) or ring (16's bands at the fireball's rim); under the Add 608s the same 0 adds nothing and stays invisible. That
  explains why only the Normal 608s show it.
- **Lead (INFERRED, not observed): non-finite or negative values in the native lit HDR at the burst**, written by one of the
  0x29 draws of the hit (the soft 603s with `depthBlend` 1, the octagonal 11630 / 11640 Normal billboards, the Unk7 billboard
  of 524552), which the Normal radial blur spreads over its disc. The 608 itself, given finite inputs and finite constants,
  cannot. A wrong `g_vColorScale` or `g_ps_ColorBlendType` natively (a constant left zero, tranche 7's class) would change the
  brightness, not the hue, with this white key.

## 4. Checks that decide it (for the pilot's runs)

1. **The lit HDR at the burst** (`world-pictures`, the `-hdr.rgba16f` dump of `scene_pass_frame.cpp`): count NaN, ±inf and
   negative values per channel in the explosion's area on the frame of the ring or the sphere. Present: the lead holds and
   the source is a draw before 0x2D.
2. **A/B `no-ffx-effect-copy.flag`** (native): the 608s then read the forward pass's copy, without the 0x29 effects. The
   colours gone: the bad values come from the hit's 0x29 effects (then hide them by class or texture to find the one);
   unchanged: from the forward rows or the 608's own inputs.
3. **The witness `ffx_distortion_witness`** (slot 27, both modes) already catches every draw whose t0 is the frame copy,
   the 608s included: compare `color`, `colour_scale`, `color_type` and `aa8` between the original and the native at the
   burst. Equal: the constants are cleared and check 1 carries the answer.

Static sources: `decompiled/DS3-1152-F-00260410.c`, `-00262530.c`, `-00264230.c`, `-00260830.c`; the 39E900 decompile and the
FXR dumps (`Read-Fxr.ps1`) in the study thread's scratchpad; `native/frame/effect_draw.cpp` (allowed classes, copy token),
`native/frame/game_bootstrap/21_effects.inc:561` (copy token → SceneCopy), `native/renderer/scene_pass_frame.cpp`
(`EffectCopyPass`, the copy before 0x2D), `native/renderer/effect_stage.cpp:303-313` (t0..t4, the sampler bank).
