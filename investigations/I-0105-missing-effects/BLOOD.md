# I-0105: how the original fades and washes painted blood (static)

2026-10-29, a `ghidra-research` worker (Sonnet), headless Ghidra on `ghidra-mirror-i0066` (read-only); outputs
`evidence/I-0105/blood/` (decompiled functions, caller lists, the PntCol programmes disassembled). Read by the pilot, not yet
re-checked in the decompiler where marked INFERRED. Context: [DECALS-PAINT-READY](../I-0066-independent-native-startup/DECALS-PAINT-READY.md)
§1; the native drains paint queue type 0 only (blood painted), so blood never leaves.

## Answer

Blood has no time-based fade. It leaves in three ways, all through the decal manager's paint queue (0x30-byte entries at
manager+0x20: receiver, handle, +0x18 type, p0..p3 floats at +0x1C..+0x28, +0x2C an erase byte):

1. **Clear all, type 2** (mask 0xFFFF; 5F430 → the target's clear 4CD70: diffuse and specular (0,0,0,1), height (1,1,1,1), depth
   1.0): queued by SprjAsmModelDrawEntity slot 46 (D00E60, per part) and SprjModelDrawEntity slot 30 (D071B0) when a chr's flag
   is set by ChrIns vt+0x688: a bonfire event about 1.33 s later (SprjLuaEventConditionBonfire 45DD90, the player) and the enemy
   group reset (8D13D0 under 8DDA50); INFERRED: resting at a bonfire, a death reload.
2. **Uniform decaying erase, type 1** (`{0, v, 1, 0, 1}`, about 1 s, v falling to 0): ChrIns vt+0x690 after a SpEffect is removed
   whose SpEffectParam +0x302 no remaining effect shares (INFERRED: a wet or material layer).
3. **Wash sweep, type 1** (`{0, 0, s-T, T, 1}`, assembled models: the player and replay ghosts): PlayerIns' update while a wet
   effect's progress s is above zero (INFERRED: rain or water).

Type 1 draws the erase pass 1AAF90: the receiver's meshes again with shader pass 10 (VS `<family>_PntCol[_Skin]`: the second UV set
as position, depth 0, COLOR1 through; PS `GXFlver_Common_PntCol`), into the paint target's diffuse and specular with its depth,
no clear; PS b10 (cbPaintParam) +176 `FC_PaintEraseColor = (0, 0, 0, 1 - p1)`, +192 `FC_PaintFadeParam = (p2 + p3, p2, p3 ? 1/p3 : 0)`;
per pixel: discard where COLOR1.z > p2 + p3, else alpha (1 - sat((z - p2)/p3)) (1 - p1). Its blend (read by the pilot in
GhidraMCP, 2026-10-29; no capture needed): the draw state takes `[[GXRS+0x4DD0]+0x320]` last (OMSetBlendState through 15220),
made in the device member's init 140072380 (`param_1 + 100`) with every render target SRC_ALPHA / INV_SRC_ALPHA ADD and alpha
INV_DEST_ALPHA / ONE ADD, all channels (the GX blend and operation enums are D3D11's minus one: converters 141929DB0 and
141929E80, used by 14192F2C0 before CreateBlendState; the functions and the descriptors' data exported to `evidence/I-0105/blood/states/`
by headless Ghidra): each texel lerps toward the neutral (0,0,0,1) by the pixel's alpha. Depth
`[[GX+0x2A0]+0x1E0]` (1400D21F0: on, written, LESS_EQUAL; the VS's depth 0 always passes), raster `+0x208` (solid, no culling).
5F430 hands 1AAF90 `{p2, p3, p1}`, and `p0` selects the target's surface (0 from both producers). Type 1 without its byte (the
bloodmask blit 5F620) has no producer in the shipped code.

## Native addition (I-0105 tranche 4, [RESULT](RESULT.md))

- Type 2 (mask 0xFFFF): drained as the handle-0 clear the native already makes.
- Type 1 (byte set): one erase job per command: the receiver's G-buffer rows drawn with shader pass 10's programmes into the paint
  target, PS b10 from p1..p3, the blend above.
