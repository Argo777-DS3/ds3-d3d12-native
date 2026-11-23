# I-0105 - Les effets manquants

Capability: C3. FINISH-LINE line 5 ([FINISH-LINE](../../.project/FINISH-LINE.md)), opened 2026-10-28 by the pilot (Opus 5.5)
after I-0104 (the native effect context, [DESIGN](../I-0104-native-effect-method/DESIGN.md) §3 names what this line writes in
it). Order: I-0103 [CENSUS](../I-0103-effects-census/CENSUS.md) §7 point 3. Decisions: D-0190 (native effect contracts),
D-0195 (Elden Ring's architecture first, without its known defects), D-0227 (full production: short tranches merged once written
and reviewed, no live run before a merge), D-0229 (the one-minute start check after each merge).

## Question

For each effect family the native skips or does not produce (CENSUS §3 and §5), what does the original engine do to draw it, how
does Elden Ring do the same job (D-0195), and which native code draws it in the base I-0104 built, so that the family appears as
the original's?

## Scope

Included, by tranche, in the census's order (each tranche merged on its own once written and reviewed):

1. **E03 model particles (FXR 605, renderer class `3D65F88`, draw `27EA70`)**: 752 FXRs in every zone; the likely Estus Flask in
   the hand while drinking (Thomas, 2026-10-28 08:42), parts of spells, the Ringed City's grass (EFFECTS-E03). The original draws
   each particle's FLVER model through the engine's model draw (`29ADF0` → `2BA3A0` → `2C0130` → `636F0` → `63CB0`) on the pass
   context's model context `+0x460`, which is null natively (Native2035's fault). Natively: through the native model renderer
   (I-0068 [EFFECTS-E03](../I-0068-static-yebis-ffx-lights-spec/EFFECTS-E03-MODEL-PARTICLES-NATIVE.md) §2 M1-M6), as Elden Ring
   does (to read first, `ER-E03.md`).
2. **The trails 606 (`3D7E380`) and 10012 (`3D660B8`), both through the draw context's slots and the GX wrappers (I-0105 RESULT)**:
   326 FXRs; a skipped view at the High Wall (Native1968).
3. **The burning objects' look** (no native reader of the Burn MTDs' parameters, I-0101), **the blood's fade**, **the surface
   snow** (the per-frame block's rows +0x60..+0xAF never produced; checked first), **the light shafts 10003** (`3D65A88`; m30, m33,
   m38), each after reading what the original does.

The spells, infusions and status effects (Thomas, 2026-10-28 18:15-18:22: the miracles' effects incomplete; the sorceries',
pyromancies', Dark spells' (Black Flame included), every weapon and shield infusion's and the status effects' uncertain) are
counted unfinished: each tranche says which of them it can reach, none is called finished without being seen.

Excluded:

- The key order of the effects with the forward rows, the depth-writing effect draws, the copy before each sampling pass, the
  bindings resolved once at the bind: FINISH-LINE line 7 (I-0104 RESULT), unless Thomas moves them.
- Effect spot lights 11000: line 6. Families never used by the game's data (10002, 10008, 10200, 10302, 10600, 10013, 10014) and
  those that draw nothing (10300, 10301): nothing to write (CENSUS §7 point 4).
- Live runs before a merge (D-0227); after each merge, the launcher's rebuild and the start check (D-0229). A witness run is taken
  only when a fix cannot be written without it, said to Thomas first.

## Inputs

- CENSUS (K-0485) §2-§7; FFX-DRAW (I-0066) for the E03 witnesses (Reference2037: at Firelink the only E03 draw is `s11600`, pass
  0x29, LOD 0, with bones; Native2035 faulted with `ffx-model-particles.flag`); EFFECTS-E03 (I-0068 part K) for the engine chain.
- Native code: `native/frame/effect_draw.*`, `native/frame/game_bootstrap/21_effects.inc` (the native effect context), the model
  path (`05_selected_scene.inc`, `08_model_programmes.inc`, `09_model_admission.inc`, `12_scene_collect.inc`,
  `13b_prepare_model_frame.inc`, `renderer/model_draw.*`).
- Tools: Thomas's GhidraMCP on the DS3 copy (read-only); headless Ghidra on the ER mirrors (`ghidra-research`, re-read by the
  pilot where a design depends on it); clangd; the MSVC build and its unit tests.

## Hypotheses

- H1 (E03): the model particles need no new rendering machinery, only a native entry: each `29ADF0` call on the native pass
  context hands the native model path one submission (model, LOD, material table, the 400-byte state, the world-space bone rows,
  the engine pass), drawn as a forward row of that pass.
- H2 (E03): Elden Ring's model appearance reaches the same mesh loop as its world models (no effect-only model renderer).
- H3 (606 and 10012): their renderers draw through the GX statements the native context already takes, or a few more; 606 needs
  the pass context's slots 0x20-0xD8 (DESIGN §3).

## Expected evidence

Static: the DS3 and ER functions at named addresses, decompiled, with what the pilot read marked. Code: unit tests for the pure
parts (the submission's record, the bone palette's space), clangd, the build and its unit tests, `Test-Project`, a step review
and a fresh Opus at xhigh on the final tree; after each merge the start check (the Firelink path: its only E03 draw is `s11600`,
so the check names whether E03 draws are made and none faults).

## Exit conditions

- Success: each family of the scope drawn natively (or shown to need nothing), each tranche reviewed and merged with its start
  check passed.
- Refuted: H1 refuted if the native model path cannot take a submission without an entity (then the design says which seam).
- Insufficient information: a family whose original behaviour the decompiler cannot settle is written as open, with a witness
  run proposed to Thomas.
- Stagnation: no new discriminating evidence after two cycles on one question; after two failed local fixes, an RCA.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`.

## Safety and human interaction

No live run before a merge. The start check after each merge is announced in the thread (start and end times, « PC libre »).
Thomas's saves are never written; Alpha only from a copy if a witness run is ever needed (Thomas, 2026-10-28 22:55: « Il peut
prendre Alpha » for the spell tests, each run restored by hash).
