# Foliage F1 natively (2026-10-11 08:45, branch `claude/foliage-f1` from main 0eb7b1e5)

Specification: I-0068 [FOLIAGE-NATIVE](../I-0068-static-yebis-ffx-lights-spec/FOLIAGE-NATIVE.md) §7-§8 (F1).

- The vertex `cbvModelParam` producer (`game_bootstrap.cpp`, slot 7) now writes: `+0x70` and `+0x490..+0x4AF` as zero **only
  where every variable of the range is marked unused in the RDEF** (`D3D_SVF_USED` clear: the Grass programmes; POM
  programmes read `VFC_PomLightDir`, whose producer, context `+0x1290`, is still absent, so they stay refused); `+0x80..+0x48F`
  from `[anim+0x210]` when `[anim+0x620]` is set, otherwise zero (wind counts 0, rest pose: the item constructor's state);
  `+0x4C0..+0x4EF` zero when `[anim+0x630]` is clear or the animation object is absent.
- Native1234 at W2 ((24.3, -10.3, 96.1), held), against Reference1212 and Native1211: the Grass litter and leaf masses are
  drawn (static); `native_vmodel_param_unproduced` 0 (before: the Grass rows refused), 0 debug errors, no fault.
- Open: wind (F2, effects phase), the tree canopy `m009002` (FOLIAGE-NATIVE §9), FOL reads.

## Pre-merge review (fresh Opus 5.5 agent, high effort)

Nothing blocking (flag parsing, overlap test, no overlapping writes, no D3D11). Fixed: the `+0x620` and `+0x630` flags are read
separately (a failed `+0x630` read no longer discards a valid block; it leaves `+0x4C0` unproduced as before), unreadable
animation flags are logged (`native_vmodel_animation_unreadable`, bounded), the block buffer is a stack array, the header
comment is updated. Recorded: non-Grass programmes that read `VC_ForceParam` with the block flag clear are now drawn with
zero wind (the constructor clears `anim+0x210`, so INFERRED equal to the original's state; whether 5B3D0 zeroes its stack
block when it skips the copy was not read); the 5 `Eclipse_grass` programmes are not witnessed. Full build with the control
suites after the fixes; no live rerun.
