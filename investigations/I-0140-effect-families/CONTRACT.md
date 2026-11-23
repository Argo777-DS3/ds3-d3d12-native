# I-0140 - Les familles d'effets et de draws pas encore natives

Capability: C3 · ROADMAP workstream 5 (« The effect and draw families not yet native »), the E pilot, opened 2026-11-13 on the
coordinator's order while the fresh E thread waits for Thomas, from `main` `ddc221de`, branch `claude/stoic-jennings-ws5`.

## Question

Which effect and draw families does the native still skip or draw wrong where they show with the rays (D-0250), measured from the
journals before any code, and for each, what structural change (a family, an engine mechanism, never one asset: D-0150, D-0161)
draws it as the original does?

## Scope

Included, in ROADMAP workstream 5's order:

1. Effect passes 0x20 and 0x21 recorded and never drawn; the skipped renderer classes 10003 (light shafts) and 11000 (spot effect
   lights); the GPU particles' zero environment SH.
2. Decals: the characters' blood and damage, the GX00 class (eyebrows, overlays), the decal pass.
3. Single-model NPC phantoms; the burn MTD parameters.
4. Gaps 16 (Fire Orb's magenta rim), 17 (the crystals' hue) and 23 (the fast travel's fade under the TAA), the mist's lower
   contrast, the menus' portrait cases.
5. A systematic audit of the families never looked at: every sorcery, pyromancy, miracle, infusion, buff, status and projectile cast
   once natively beside the original.

Excluded: YEBIS's refusals (cutscenes, exposure, glare, depth of field: workstream 11); the raster lighting the rays replace
(D-0250); RT/PT leftovers (workstream 7).

## Inputs

- The study that guides it: [STUDY-NOT-YET-NATIVE](STUDY-NOT-YET-NATIVE.md) (copied from I-0113, 2026-11-01: several rows have
  changed since, the raster lighting removed among them: each row re-read before use), I-0113's final table and studies (gaps 16, 17,
  23), [I-0103 CENSUS](../I-0103-effects-census/CENSUS.md), FINISH-LINE line 7, I-0139's tour (no skipped family met at 55 bonfires).
- The journals' own counters (`native_ffx_skipped`, `native_effect_stage`'s `other_passes`, missing textures), read across the tour
  and the recent native runs first.
- Tools: GhidraMCP and `ghidra-research`, clangd, `Build-GameBootstrap.ps1 -Out`, the spell pilots (`Run-AlphaCast.ps1`), the
  `step-reviewer`, `final-reviewer` and `integrity-reviewer` subagents; runs through the coordinator's slots.

## Hypotheses

- H1: most of the listed families are not met in ordinary play (the tour met no skipped family); the open work is in the spells and
  the passes 0x20 and 0x21.
- H2: some families are met and silently skipped (counted only behind a flag): the census finds them.

## Expected evidence

A census of the counters over the tour's and the recent runs (which families, how often, where); for each family taken, the original's
mechanism read (GhidraMCP), the native change, its unit test where testable, a witness run beside the original's picture (D-0181,
D-0268: before and after stills; Thomas's eye the judge).

## Exit conditions

- Success: each listed family drawn natively, or recorded as not met in play or left with its reason.
- Refuted: a family the present code already draws is recorded so.
- Insufficient information: a family no run can reach is deferred with what would reach it.
- Stagnation: no new discriminating evidence after 2 cycles on one family: recorded and the next one taken.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`; renew a bounded budget automatically within unchanged authorized scope and existing protected boundaries.

## Safety and human interaction

Witness runs only through the coordinator's slots, announced, offline, restored by hash (the live-run skill). Nothing irreversible.
