# I-0113: the effect passes 0x20 and 0x21 (STUDY-NOT-YET-NATIVE row 4; static, study thread)

Study thread S, 2026-11-06 (Paris), read-only, for the open-gaps pilot E. Thomas's Ghidra GUI on `DarkSoulsIII.exe` 1.15.2
(GhidraMCP); the decompiled lines are those already cited in [STUDY-T17-LAYER-ALPHA](STUDY-T17-LAYER-ALPHA.md) §1-§3.

## What the original does (VERIFIED in code)

- **The sub-pass chooser `1402997E0` never picks 0x20.** By the entity's kind `param_3`: 1 → 0x21 (0x31 only with the debug
  byte `DAT_144962375`, and back to 0x21 when « near »); 2, 4 → 0x29; 3 → 0x29 (0x31 when near or the debug byte); 5 → 0x2D;
  6 → 0x31 (`DAT_14473DAD6`, default 1) else 0x29; any other → 0x29 (0x31 when near or the debug byte); and 0x23 for every kind
  when `1402993F0` holds (STUDY-WATER-INTERACTION: the pass that draws into the water interaction map). An FFX entity reaches
  engine pass 0x20 only through another recorder (not found; none seen in the journals below).
- **In 0x21 the classic FFX draws are opaque and write depth:** `140299E40` sets the context's mode 1 for pass 0x21, and the commit
  `140264230` then binds blend `B+0x08` (off) and depth `B+0x1E0` (LESS_EQUAL, written), whatever the effect's blend type. In
  0x20 (mode 0) the commit passes a null blend and a null depth state to the GX setters (`141920ED0`, `141920F00`); whether the
  GX context then binds D3D11's defaults (no blending; depth test LESS, written) or keeps the previous states is not read (TO CHECK).
- So kind 1 is the « opaque effect » kind: its draws go with the opaque forward rows of 0x21, before the translucent 0x29 rows
  (INFERRED from the key order; which effects are kind 1 is not identified statically: the kind comes from the entity's
  caller, `1402997E0` has no direct caller, it is reached through a table).

## What the native sees (OBSERVED in the journals, RESULT tranche 11)

In the 236 native journals read in tranche 11, the classic FFX draws of 0x20 and 0x21 (`other_passes`) are 0 everywhere; with
`ffx-record.flag` the commands recorded are of 0x29, 0x2D and 0x31 only, except m31 (`i113-m31c-native`: 106 of 0x21, the E03 model
particles tranche 3 draws), and `native_effect_models`' `other_pass` (E03 of 0x2D, 0x20, 0x31 counted, not drawn) is 1 at m31 only.

## For E

Row 4 holds no gap seen in play so far. What a transcription needs if a kind-1 effect appears (the native's
`other_passes` or `NoteNativeEffectCandidates` would count it): draw it in 0x21's place with `B+0x08` and `B+0x1E0` (as the
mode-1 rule above), into the forward HDR target with the depth written, before the 0x29 rows. A zone never witnessed (m33, m34,
m35, m38, m39, m41, m51 and the DLC's) is where one could show; the cheapest check is the `other_passes` count already in every
`native_effect_stage` row of a zone tour.
