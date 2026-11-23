# I-0104 - Méthode native des effets

Capability: C3. FINISH-LINE line 4 ([FINISH-LINE](../../.project/FINISH-LINE.md)), opened 2026-10-28 by the pilot (Opus 5.5)
after I-0103, whose [CENSUS](../I-0103-effects-census/CENSUS.md) §7-§8 orders this line. Decisions: D-0190 (native effect
contracts replace the FFX statement replay), D-0195 (Elden Ring's architecture first, without its known defects), D-0227 (full
production: short tranches merged once written and reviewed, no live run before a merge).

## Question

How does Elden Ring 2.7.1 take its effects from the engine to D3D12, and which native effect base follows from it for DS3, so
that the families the native already draws (E01 603, 604, 600 first; then 607, 608, 602, 601 and the GPU particles 10000) are
produced by the engine and drawn by our D3D12 without replaying the GX/D3D11 statements captured by `effect_draw`?

## Scope

Included:

- **Tranche 1, static**: Elden Ring's effect rendering read in the decompiler (read-only, a copy of the analysed project): the
  record of the effect scene entity and its per-node callback, the renderers' draw slot, where vertices and constants are
  written, the GX draw API the renderers call and how ER's D3D12 layer turns it into pipelines, root bindings and draws, the
  textures (the frame copy for distortion, the depth), the order of the effects against the forward rows, the GPU particles
  (compute, counters, indirect draws, resource ownership and barriers), and the lifetime of every resource. Each ER defect met
  is named and not copied. Output: `ER-EFFECTS-STUDY.md`, then `DESIGN.md` (the native contract for DS3: what the engine
  produces, what the backend owns, the E01 correctness points of CENSUS §5 written into it).
- **Tranches 2 to 4** ([DESIGN](DESIGN.md) §2, which replaced this contract's first plan after the ER reading): tranche 2 the
  native effect context, one path for every classic family (E01 603, 604, 600 and, on the same path, 602, 607, 608, 601), the
  string witnesses gated, the scene renderer's dead stage observer removed (CENSUS §8); tranche 3 the E01 correctness points (the
  key order with the forward rows, the depth-writing draws, the copy before each sampling pass); tranche 4 the GPU particles
  (10000) in the context, and the producers' capture layer (`world/light_producers`, `world/effect_producers`; settled in RESULT) kept for its
  formulas or removed (CENSUS §8). No GX fallback is left (D-0190).

Excluded:

- The families the native skips (E03 605, the trails 606 and 10012, the light shafts 10003): line 5, written directly in the
  base this line builds. Effect spot lights 11000: line 6.
- The effects' update (the CPU simulation and its threading, I-0092): unchanged unless the base needs a seam there.
- Live runs before a merge (D-0227); after each merge, the launcher's rebuild and the one-minute start check (D-0229).

## Inputs

- Canonical claims and notes: I-0103 [CENSUS](../I-0103-effects-census/CENSUS.md) (K-0485), [FFX-DRAW](../I-0066-independent-native-startup/FFX-DRAW.md)
  (the E01/E02 classes, E07, its « Open »), I-0068's [EFFECTS-D3D12-DESIGN-NATIVE](../I-0068-static-yebis-ffx-lights-spec/EFFECTS-D3D12-DESIGN-NATIVE.md)
  (written before ER was read for effects), [EFFECTS-E07](../I-0068-static-yebis-ffx-lights-spec/EFFECTS-E07-GPU-PARTICLES-NATIVE.md),
  I-0086 [ER-STUDY](../I-0086-er-engine-thread-study/ER-STUDY.md) §4 (ER's effect update and its one scene entity),
  I-0072 [ER-PIPELINE-STUDY](../I-0072-shader-pipeline-cache/ER-PIPELINE-STUDY.md) (ER's GX device context, state tracker and
  pipeline cache).
- Native code: `native/frame/effect_draw.*`, `native/frame/effect_system.*`, `native/frame/game_bootstrap/21_effects.inc`,
  `native/renderer/effect_stage.*`.
- Tools: headless Ghidra 12.1.3 read-only on the ER mirrors (`er-mirror-i0070-a`, `er-mirror-i0072-a/-b`) and the DS3 mirror;
  Thomas's GhidraMCP on the DS3 copy (read-only); clangd and the MSVC build with its unit tests.

## Hypotheses

- H1: ER keeps DS3's shape (one effect scene entity, per-node callbacks recorded at record time, renderers that build their
  vertices on the CPU) and changes only the layer below: the renderers call a GX draw API whose D3D12 implementation tracks
  state, builds pipelines from pooled ids and binds through one global root signature. Then the native base is that layer,
  implemented natively for the effect renderers, instead of the D3D11 wrappers hooked today.
- H2: ER's effect renderers produce typed draw packets (programme, states, constants, vertex range) consumed by the backend
  without a state machine. Then the base copies that packet.
- H3: ER sorts its effects with the forward rows by key and copies the scene colour once per sampling pass; DS3's original
  does the same in its passes (FFX-DRAW « Open »), and the native order is the defect.

## Expected evidence

ER functions at named addresses, decompiled, from scripts that can be run again (kept in the investigation), with what Opus
checked marked; every design claim that rests on ER is checked by Opus in the decompiler before code depends on it. For DS3,
the native records before and after each family's move: the same draws, programmes, states, constants and textures for the same
frame (a deterministic comparison in the unit tests or a recorded frame), then the build, its unit tests and `Test-Project`.

## Exit conditions

- Success: the study and design written; E01, then 607, 608, 602, 601 and 10000 drawn through the native base with no statement
  replay left for them; each tranche reviewed (a fresh Opus at xhigh on the final tree) and merged into `main` with the
  launcher's rebuild and the start check.
- Refuted: H1 and H2 refuted if ER's effects reach D3D12 another way; the study then says which, and the design follows it.
- Insufficient information: an ER mechanism the decompiler cannot settle is written as open, with the DS3 choice and why.
- Stagnation: no new discriminating evidence after two cycles on one question; after two failed local fixes, an RCA.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`.

## Safety and human interaction

Ghidra is read-only (no rename, no comment, no saved change) on the mirrors. No live run before a merge (D-0227); a measure or a
live witness only if a fix cannot be written without it, said to Thomas first and run under the Live game rule. No build or
test while an earlier pilot's claude.exe still runs in a worktree of the repository (the coordinator's handover, 2026-10-28).
