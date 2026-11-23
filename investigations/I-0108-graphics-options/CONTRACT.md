# I-0108 - Les réglages graphiques

Capability: C3 · FINISH-LINE line 8 (« le tour de chaque réglage graphique, puis la correction de ce qu'il trouve ») · second
pilot, opened 2026-10-30 on Thomas's requests of 2026-10-29 22:09 (the water's reflections) and 23:14 (every graphics setting at
every level).

## Question

Does the native renderer follow each graphics setting of Dark Souls III at each of its levels as the original does, and where it
does not, what does it take to make it?

## Scope

Included:

- The inventory of the 11 keyed settings and their 27 quality-object fields ([INVENTORY](INVENTORY.md)), checked statically and
  live at the Crucifixion Woods lake (series `opt-lv-*`).
- Fixes in the order of their weight: (1) the depth of field refused at the qualities 0 and 1 (« programmes »: absent at
  Crucifixion Woods at every setting); (2) the water reflection following ReflectionQuality (`+0x6D` lights, `+0x70` target
  divisor, `+0x6E` the ordinary parts at MAX) and the characters in it; (3) the mirrored view's sun shadow, if Thomas takes it back
  from FINISH-LINE's « à abandonner ».

Excluded:

- The effects that WaterSurfaceQuality (`+0x74`) and EffectsQuality gate: the effects pilot's files (noted for it).
- The missing textures (FINISH-LINE line 7), the FXAA filters unless a zone uses them, other zones' witness runs beyond those a fix
  needs.

## Inputs

- MENUS-MAP §77 (the quality table per level), QUALITY-OBJECT-NATIVE, the options sheet (stale where INVENTORY says so),
  WATER-TESS-WAVE1-READY 3.3, CHR-FADE-EXPARAM-REFLECTION §3, W10-NATIVE-PASSES (the DoF).
- Evidence: `evidence/I-0066/artifacts/opt-lv-*` and `opt-lv-compare/`.
- Tools: GhidraMCP, the zone-walk runner with `-GraphicsLevels`, ffmpeg sheets (FLIP does not discriminate at the lake: moving
  crabs and foliage).

## Exit conditions

- Success: each fix seen at the lake against the original at the levels it changes, merged; INVENTORY updated.
- Insufficient information: a field whose reader cannot be found is recorded as such, not guessed.
- Budget: 8 cycles, then replan.
