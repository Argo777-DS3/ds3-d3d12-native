# I-0113 gap 17: who sets the draw item's +0x1A9 for an E03 model particle (study thread, static, 2026-10-31)

Written by the study thread for the gaps pilot to copy into I-0113. STATIC only (decompiles and their assembly in `decompiled/`,
GhidraMCP), nothing launched. Question (the pilot, 2026-10-31): the draw item Q's `+0x1A9` sets bit 20 of `S+0x200` in `5A120`,
which turns `_Fwd` into `_Fwd_[A]`; who writes it for an FXR 605 model particle (the crystals `s04020`..`s04022`), and from which
value.

## 1. The reader (VERIFIED)

- `14005A120` (`decompiled/DS3-1152-F-0005A120.c:157-165`, assembly `14005A1A8`, `14005A1C7`): Q = its third argument (R8, then
  R12). `Q+0x1A9 != 0` → `14013B450(context, Q+0x1A6)`, which binds pixel slot 0x26 (t38) to the texture
  `[[GXRS+0x4DE8]+0x8D0 + min(Q+0x1A6, 1) × 0x70]` vt+0x10 (cached in context `+0x12B8`), and `S+0x200 |= 0x100000`; else bit 20
  cleared.

## 2. The ordinary models' writer (VERIFIED): `14005C0E0`, the recording of a model's draw item

- `decompiled/DS3-1152-F-0005C0E0.c:98-115` and `:365`: with X = the descriptor's first word (`*param_7`, the entity's fade group
  that the native's `ReadModelFadeStateAt` reads), `Q+0x1A9` = `X+0x89 && X+0x124 < 1.0` when the row is `param_4 == 1` and
  `*(param_3+0x10) == 2`, else `X+0x88 && X+0x124 < 1.0` (1.0 = `[143D7EE10]`, the native's `category_scalar`). `Q+0x1A6` and
  `+0x1A7` come from the descriptor's bytes `+0x0B`, `+0x0D`.
- The native's `fade_dither = fade.fade_enabled && fade.opacity < category_scalar` (`10_options_and_passes.inc:294`) is this
  formula for the ordinary branch (`+0x88`); the `+0x89` branch (`param_4 == 1`, kind 2) is not modelled natively.

## 3. The E03 model particles' path: **nobody writes it** (VERIFIED from the assembly)

- An E03 draw does not go through 5C0E0: `14029ADF0` (the model particle's draw, called by the renderer of class 3D65F88) builds
  its draw item **on its own stack** and calls `140059470(pass, args, &descriptor, &Q)`, which calls `5A120(pass, args, Q)`
  (`decompiled/DS3-1152-F-00059470.c:161`).
- 29ADF0's frame: `RBP = RSP + 0x100` (prologue `14029ADF7`, `14029ADFF`); Q = `RSP+0x50` (`14029B0D7..B0E4`: `Q+0` = 0,
  `Q+4` = 0x187, `Q+8` = 0xFF); `Q+0x10..+0x19F` = a 400-byte copy of the block at `RBP+0x150` (`14029B0EC..`); then
  `Q+0x1A0` = -1 (`RBP+0xF0`), `Q+0x1A4..+0x1A7` = 0 (`RBP+0xF4`, a dword), `Q+0x1A8` = 0 (`RBP+0xF8`, a byte),
  `Q+0x1B0`, `+0x1C0`, `+0x1D8..`. **No instruction of 29ADF0 writes `RBP+0xF9` (= `Q+0x1A9`).** 59470 writes `Q+0x1A4`, `+0x1A5`,
  `+0x1B8`, `+0x1C8` and not `+0x1A9` (`00059470.c:154-157`; no `+0x1A9` operand in its assembly).
- So for an E03 particle **`Q+0x1A9` is the stack's leftover byte**: whatever an earlier call on the same thread left at that
  address. `Q+0x1A6` is 0 (in the zeroed dword), so when the leftover is nonzero t38 gets the first of the two textures.
- Consequence (INFERRED): in the original, a model particle takes `_Fwd_[A]` when the leftover byte is nonzero and `_Fwd`
  otherwise; the value depends on the call sequence before each 29ADF0 call (the renderer's other calls, the previous
  particle's draw), so it can differ from one call site or pass to the next. That fits tranche 8's census, where the
  original draws each crystal with both `_Fwd` and `_Fwd_[A]` (F546E123F52F4F41, the only one that reads `cbMatDynParam` b2:
  FC_DynamicDiffuseColor (1.5, 1.5, 1.5, 1), FC_EmissiveColor, FC_isDitherAlpha 0).
- Natively the E03 particle's fade comes from the draw state's `+0x88` / `+0x124` (`NativeEffectModelFade`,
  `09_model_admission.inc:57-66`), i.e. 5C0E0's formula applied to a path where the original applies none: with `+0x88` clear
  or the opacity at 1, never `_[A]`. That matches the native's « `_Fwd` only ».

## 4. What to decide and check

- The original's result here is undefined behaviour, so the native has to pick the observed one. The useful observation is in
  tranche 8's census journals (`evidence/I-0113/gap17/`, the original's run): **per frame and per crystal, how many draws and
  with which PS**.
  - Two draws a frame (one `_Fwd`, one `_Fwd_[A]`, e.g. at two call sites or passes): reproduce both rows natively, the second
    with pixel variant 1.
  - One draw a frame whose PS alternates: take the dominant one (by count), most likely `_[A]` given the violet, opaque look.
- Whatever the choice, the native rule belongs to the E03 path only (the `ModelSubmission` of class 3D65F88), not to
  `fade_dither` of the ordinary models, which is right as 5C0E0 writes it.
- The witness for a live check: the native census row of `s04020`..`s04022` with the chosen variant, beside the original's
  picture at the Firelink bonfire (`crystal-native-norows-original.png`'s setting).
