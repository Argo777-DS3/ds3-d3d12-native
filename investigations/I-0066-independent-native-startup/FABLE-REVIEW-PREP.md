# Wave 1 final control review (Fable 5.1, high effort): the brief

Thomas (2026-10-18 18:43-18:47 Paris, D-0188): finish wave 1 earlier; the final control review corrects the remaining gaps in one
pass; it also gives optimisation axes (it does not run the optimisation workstream). The pre-review position is `main` after the
merge of the thread « Sang sur persos, eau, siège » (STATE "Current workstream").

## 1. What the review covers

- Every gap below, each with its evidence; the reviewer diagnoses and corrects, then witnesses side by side with the original
  (`Run-ZoneWalk.ps1`, `Run-LiveWitness.ps1`, `Run-AlphaCast.ps1`, the protocol of D-0116/D-0117).
- The code of wave 1 as merged (`native/`), for defects a side-by-side does not show.
- Optimisation axes (D-0167, D-0169): where the frame and the collect spend their time (the m45 levers of STATE, the sampling
  profiler `sample-profile.flag` and `Get-SampleProfile.ps1`), ranked, without implementing them.

## 2. Open gaps handed to the review

| Gap | Evidence | What is known |
|---|---|---|
| Native scene darker and duller than the original (Crucifixion Woods) | Native2161 / Reference2160 (reflections off), Thomas 2026-10-18 18:43 | Thomas leaves it to the review with the external hypotheses (exposure, tone map, fog veil, bloom, gamma/sRGB), none checked. |
| Water planar reflection (WT-6/WT-7) | Native2159 / Reference2158 | The dominant difference of the lake with the reflection option on ([WATER-TESS-WAVE1-READY](WATER-TESS-WAVE1-READY.md)); m33 water inventory in `evidence/I-0068/queries/water-ready/m33/` (13 `GXFlver_WaterBumpTri` pieces at two heights, -279 and -261.6, the collisions' `ReflectPlaneHeight`). WT-5 (FwdSdw) deferred. |
| The large cross near the Crucifixion Woods bonfire and a small object in the lake | Native2157 / Reference2156 | Missing natively; not diagnosed (`native_selected_object_unsupported` rows for GXVisbTester 3D4A4E8 and CSSgEzDraw 295B690 seen, link unknown). |
| The Shrine Handmaid's chair, white natively, black in the original | Native2170 / Reference2171 (same teleport and camera), Native2172 | The chair is object `o404854` (hiding the model by name removes it, Native2170), placed at (251.50, -55.29, 604.80), material `M[ARSN]_e`, admitted through the character-candidate path (2 bones). Natively its G-buffer row (`GXFlver_ColDifSpcBumpIbl`, recipe 1) binds the authored textures: `m40_00_chair_01_a` (1024², BC1 sRGB), `Reflectance_055_055_055_r` (16², the 0.55 reflectance), `m40_00_chair_01_n` (512², BC7), the dither; PS b4 holds the placement rows and the colour words (1, 1, 1, 1). Next: the original's draw of the same row (its SRVs and PS constants), then the gap between the two. Separate finding (Native2164): many `_s.tif` shininess textures fall back to SYSTEX_WHITE, as the loader's default holder does in the original (`native_texture_bind` counts absent binds in both modes); not the chair's cause (its `_s` is `SYSTEX_DummyShininess`). |
| Character paint D2 not witnessed live | Native2138-2141 | Coded ([DECALS-PAINT-READY](DECALS-PAINT-READY.md) §7); waits for a struck NPC (Thomas strikes the Fire Keeper himself; the Reference oracle of 1AAA70 with `decal-observe.flag`). Burn marks not seen in the original. |
| Loading screen edges, hitches, embers on Yhorm's skull, Irithyll | STATE (Thomas, 2026-10-18) | The game shows at the loading screen's edges (1920x1080 windowed); hitches up to 1.3 s; the Firelink throne embers odd; Irithyll not compared. |
| Earlier residuals | STATE | The phantom sign's whiter ghost near Vordt (Reference/Native2071-2072), E03 (m51 grass), the sorcery and miracle families, the m31 crosses and cloth, the m37 chapel, the phantom face interior. |

## 3. Witness tooling added for the review

`Run-ZoneWalk.ps1`: `-Teleport "x y z hold"`, `-Look "dx,dy;dx,dy"` (1,500 pixels turn the camera once at the default
sensitivity), `-KeyPath`, `-HideTextures`. `hide-texture.txt` next to the module (read every 2 s, main view only): a texture path
fragment hides the rows naming it; `#name` hides a model by name; `@x y z r` names the models placed within r (`native_model_near`);
`!name` describes a model's draws (`native_named_draw_sources`). `chr-positions.flag`: the characters' placements
(`native_chr_position`) and the player's position. Firelink NPCs (Native2165): Fire Keeper c1400 (260.97, -55.17, 600.59), Shrine
Handmaid c3200 (251.53, -54.56, 604.69).

The full preparation with each gap's sources and witnesses (649 lines, written on branch `claude/fable-review-prep` on
2026-10-18 and brought into `main` by the repository audit on 2026-10-25) is [FABLE-REVIEW-PREP-SOURCES](FABLE-REVIEW-PREP-SOURCES.md).
