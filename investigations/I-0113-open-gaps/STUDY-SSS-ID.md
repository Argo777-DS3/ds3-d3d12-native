# I-0113 tranche 7's one full-key difference: FC_SSS_ID 1 natively, 0 in the original (study thread, static, 2026-10-31)

Written by the study thread for the gaps pilot to copy into I-0113. STATIC only, nothing launched. The difference: b2
FC_SSS_ID (+0x190) on every player skin and face draw, native 1, original 0 (ZERO-CONSTANTS-CENSUS §3).

## 1. What the word is in the original (VERIFIED)

- `14022E930` (`decompiled/DS3-1152-F-0022E930.c:906-922`): when `material+0x21` (an SSS material) is set, the profile is
  `material+0x368` (the linked instance) if the `g_SSSWidth` parameter's slot 6 (`+0x18`, `g_SSSStrength_UseCAParam`) holds and
  the instance exists, else the parameter's base profile `[material+0x370]+0x10`; **FC_SSS_ID = float(profile+0x30)**,
  +0x194..+0x19F zero.
- `profile+0x30` is rewritten every frame by `1401141B0` (`decompiled/DS3-1152-F-001141B0.c`), which has two branches on the
  byte `DAT_14473D39D`:
  - zero: every active profile (`+0x35`) in vector order, slot = its position (the default profile 113F80's at 0);
  - **nonzero: only the profiles marked used this frame (`+0x36`, cleared as they are taken), equal ones merged (`112C90`:
    equal width and strength share a slot), slots given compactly from 0 in vector order; the cbSSS records written in
    that order; the count at `obj+0x42`.**
- `DAT_14473D39D` = **1** in `.data` (read at 14473D39D: `01`), and its only reference is that read in 1141B0
  (`get_xrefs_to`): the compact branch always runs.
- So in the original the slot numbers only the profiles drawn: the default profile 0 is used by no material, and the player's
  skin profile, the first used one in vector order, gets **slot 0, and cbSSS record 0 holds it**. (The writer of `+0x36` was
  not read: INFERRED, the draw of a material that references the profile. FC_SSS_ID written by 22E930 takes the slot of
  1141B0's latest run.)

## 2. Natively (VERIFIED, `native/frame/game_bootstrap/07_scene_lighting.inc:350-425`)

- The native registry keeps creation order with 113D80's default profile always at entry 0 (`NativeSssDefault`), so the first
  skin profile is entry 1; `NativeSssProfileOf` returns that index, written as FC_SSS_ID (`09_model_admission.inc:827`).
- `ProduceNativeSssInputs` uploads **every** entry at its index (`profiles.data()+i*0x120`): record 1 is the skin profile.

## 3. Conclusion and the one check left

- INFERRED from 1 and 2: **1 against 0 is a numbering difference, not a defect**: each side's FC_SSS_ID indexes its own
  cbSSS array, and both point at the skin profile's record (original compact slot 0, native entry 1). What would be a
  defect is a different record behind the index.
- The check: compare the record **at the FC_SSS_ID slot** in both modes (b1 of the SSS blur, 0x120 bytes: the scaled width
  `width × 1/tan(fov/2)` then the 17 kernel taps). The native witness `native_sss_inputs` logs record 0 only (the default):
  log record [FC_SSS_ID] instead (or all entries once), and on the original read the cbSSS upload of 1141B0
  (`FUN_141920300` with `[obj+0x38]`, `obj` = `[[GXRS+0x4DE8]+0x18]`) at the same place. Equal: close the difference as
  numbering. Different: the strength source (`g_SSSStrength_UseCAParam`, still open natively: « not read ») or the width
  (112C60 along the link) is the next read.
- If the census should stop reporting it, compare FC_SSS_ID through the record it names, not as a raw word.
