# I-0141 - L'éclairage des cinématiques et du joueur en RT/PT (ROADMAP chantier 7 étape 7)

Capability: C3

## Question

ROADMAP workstream 7 step 7: where the traced lighting of a cutscene, and of the player in play, departs from the original's
picture, what the original does there, and what the native's one traced sun, its per-draw light sets and its fog should do
instead, tranche by tranche.

## Scope

Included, in this order (each a tranche merged alone):

- Tranche 1: the traced sun of a cutscene whose ShadowParam record names no light set (Shadow Light Id -1: 32000010,
  39000020), the coordinator's recommendation of 2026-11-14 while Thomas was away (his choice B prolonged: light 0 of the
  light set of the scene's characters), to be confirmed by him.
- Then, as read by the E pilot and the game tour (I-0140 RESULT « Next », I-0139): the cutscenes' fog (31000010's wyverns,
  40000040, 45000030's cyan veil: the map's fog applied as a full veil), 31000010's valley (the original's per-pixel light set
  300 and cascades that stop at 10 to 60 m), the player lit warmer and darker than the original's (`i140-e11`), the ending's
  body over-lit by the hand's flame, the Road of Sacrifices' moss lost in the lighting's warm tint.
- Found by tranche 1 (RESULT « Read for the next tranches »): the fade in time of a zone's draw-parameter files where
  the original binds them with a transition time (the native switches at once), and the cutscene act that overrides the
  in-game light and fog records (`IngameDrawParam`), to observe first; the E pilot's 51000000 and 51000010 (the Dreg Heap).

Excluded:

- The emissive surfaces as light sources and the traced volumetric fog (ROADMAP workstream 12).
- The effect families and material fallbacks (the E pilot's, I-0140).

## Inputs

- Canonical claims: K-0494 to K-0496 (the cutscenes' files and banks); I-0140 RESULT « Tranche 7 » (the view's ShadowParam
  record, its keys, F90F0's choice of the shadow sun, the cutscene file's binding), « Thomas's choice for the rest », « Next ».
- Artifacts and sources: the runs `i140-e16-*` and `i140-e17-*` (native and original, their journals and pictures), the
  tour's sheets (`evidence/I-0139/artifacts/sheets-t3d/`), `native/world/light_producers.{h,cpp}`,
  `native/frame/game_bootstrap/10_options_and_passes.inc`, `05_selected_scene.inc`, `07_scene_lighting.inc`.
- Tools: `investigations/I-0139-game-tour/Run-I139Cutscenes.ps1` and `Make-I139Pairs.ps1`; GhidraMCP for what the original
  does (read-only); WORKFLOW « Tools ».

## Hypotheses

- H1 (tranche 1): in a cutscene the key of the view's ShadowParam, {2AABBVVVV, the cut's id}, is also the light set its
  characters carry, so light 0 of that set is the direction the cut's characters are lit from; taking it for the traced
  sun where the record names none lights them as the original's picture shows them.
- H2: the horizontal shadow sun of such records is what the original uses and its picture shows characters lit all the
  same because its cascades do not reach them or hold nothing between them and that sun; then no single direction
  reproduces the original and the picture stays a choice (Thomas's).

## Expected evidence

The journals' `native_shadow_state` rows (the keys, the record's keys, the sun's source and direction) beside
`native_model_light_key` and `native_light_set` (which light set each drawn model carries, its light 0); pairs of pictures,
native before and after against the original's at the same shots of 39000020 and 32000010, measured (mean level of the
characters' regions, not judged by eye alone); no change in play (the same keys and sun as `i140-e16`'s rows).

## Exit conditions

- Success: a tranche's rule is read or observed, built with its tests, witnessed against the original at its cutscenes
  without a regression in play, reviewed and merged.
- Refuted: the witness shows the rule wrong for a cutscene (its picture further from the original's): withdrawn, the
  original's behaviour read further.
- Insufficient information: the original's choice cannot be read or observed: reported with what is missing.
- Stagnation: no new discriminating evidence after 2 cycles.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`; renew a bounded budget automatically within unchanged authorized scope and existing protected boundaries.

## Safety and human interaction

Live runs under WORKFLOW « Live game rule » (announced, offline, restored by hash; the OLED in a 2560x1440 window while it is
on). Tranche 1 applies a recommendation Thomas has not confirmed: it is said as such wherever it is recorded, and his answer
can withdraw it. No irreversible change.
