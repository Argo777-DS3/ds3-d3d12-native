# I-0068 part AS: where the bloom of emissive materials comes from, what native lacks, and the correction by family (STATIC)

Request (coordinator, 2026-10-11): after NPC-W347 (EmiIblGlow is read by the lighting as emissive but does not bloom natively), study statically the glow
chain of the emissive: where the bloom comes from in the original, what native lacks, and the general correction by family. Builds on
[EMISSIVE-GLOW-NATIVE](EMISSIVE-GLOW-NATIVE.md) (parts AA, AB, AC: the observed chain, programmes, taps, scales) and
[PRODUCERS-UNION-SNOW-GLOW](PRODUCERS-UNION-SNOW-GLOW.md) §3 (the gparam mapping); this part adds the per-pixel weight rule by shader family, the
bank-wide census, the native state at `b5b20a63`, and the ordered correction. Nothing was launched; native code was read, not changed.

Labels: STATIC (DXBC of the cached programmes, executable tables, native source), OBSERVED (I-0063 captures, cited), INFERRED, OPEN.
Evidence (outside Git): `evidence/I-0068/queries/glow/` (`glow-outputs.tsv` from `Scan-GlowOutputs.ps1` (the `none` class there is the `[A]` variants, whose whole `o5` is written in one `movc`), `builtin-material-parameters.tsv` from `I0068Table.java`).

## 0. Answer first

1. **The bloom is not YEBIS and not the material's own pass: it is the engine's `GXGlowFilter`, fed by the alpha of the G-buffer emissive target GB5.** Ten draws per
   frame after YEBIS and the anti-aliasing: `EmissiveToGlowPS` (GB5 → source, `rgb × a`), four `Sample13Weight` blurs, five additive `GlowCompositeSinglePS` onto
   the final target (EMISSIVE-GLOW §1–§2, OBSERVED in all five witness zones and m40).
2. **The weight is chosen per shader family, in the pixel programme (new, STATIC).** A G-buffer programme writes `o5.xyz = emissive texture × FC_EmissiveColor` and
   `o5.w =` the glow weight: for the **Glow families** `o5.w = FC_GlowScale.w` (`cbMtdParam` word `cb3[9].w`, the MTD parameter `g_GlowScale`; 56 families), for the
   plain **Emi families** `o5.w` is computed from a register; read in full for `EmiIbl_Bloody` it is 0 unless the alpha-coverage flag `cb3[5].z` is set (the other plain Emi families have the same shape, INFERRED): **they light but do not bloom**, in the original too (§2).
3. **Native has the sources but no filter.** The G-buffer already writes GB5 (RGBA16F, cleared to 0) and the light composite already reads it as t8
   (`scene_pass.cpp:29,1018`). Missing: the ten-draw filter, its parameters, the second target of forward rows, and the effect system's glow twins (§3).
4. **Correction by family** (§4): G-buffer Glow families need only the filter; forward Glow families need the filter and a second render target on forward rows;
   FFX `…GlowD` need the filter and the effects phase; plain Emi families need nothing. Eight ordered steps with witnesses (§5).
5. **Witness expectations change**: of the W3/W4/W7 models only `c2150` and `c5270` (`EmiIblGlow`, `g_GlowScale` (1, 1, 1, 1)) bloom; `c1090` and `c5110` (`EmiIbl`, GX03 5.0)
   are bright but **do not bloom in the original** (NPC-W347 §3 is corrected accordingly).

## 1. The chain and its parameters (recap and what is new)

Recap, EMISSIVE-GLOW §1, §2, §8: `EmissiveToGlowPS` (GB5 → 1280 × 720 R11G11B10), `Sample13Weight` ×4 (¼, ⅛, 1/16, 1/32), `GlowCompositeSinglePS` ×4 up the levels
(blend SRC_ALPHA / ONE, alpha 1) and ×1 onto the final target. Taps: `w = exp(−½(x² + y²)) / 2π`, normalised; three tap sets (2D, horizontal, vertical, §8.2).
Scales: `g_GlowScaleOffset.x = (param+0x14) × f` for extraction and level composites, `(param+0x20) × f` for the final composite, `f = clamp([[GXRS+0x5368]+0x80], 0, 1)`.

New this part:

- **The values.** The Glow group resolves through the view key to `mapbase.gparam` id 0 for the world (PRODUCERS-UNION §3.3): Enable 1, BlurSampleScale 1, Blur
  RgbScale 1.5, Blur AlphaScale 1, New Glow Enable 1 (mode B), NewGlow Final ColorScale 0.5; `remobase` id 0: 1, 2, 2, 1, 1, 0.5 (cut scenes).
  Natively **no gparam file reaches the engine's banks** (`game_bootstrap.cpp:2972…`: the light and fog records are built from the admitted files by
  `NativeGparamRecords`, qualified against the engine); the filter fields at `pipeline+0x2E80` (`+0x08` scale, `+0x0C` RgbScale, `+0x10` AlphaScale, `+0x14` New Glow,
  `+0x18` Final, `+0x1D` Enable) are written by the original's per-frame `FB890` and are never filled. The Glow record must come the same way: `Banks::Glow(key)` from
  the admitted gparam files with the engine's key rule (`{−1, −1}` → `{1, 0}`; absent (area, id) → `{1, 0}`) and the key blend.
- **The factor** `[[GXRS+0x5368]+0x80]`: `FUN_127010` (called by the native root constructor) builds the object as 18 slots of `{int kind, qword, float value, byte flag}` at `+0x40`,
  stride `0x18`, value 1.0 and flag 1; `+0x80` is the value of slot 2 (`0x40 + 2 × 0x18 + 0x10`), so **`f = 1` natively** until an options rewrite changes it (QUALITY-OBJECT-NATIVE). Read it each frame.
- **Mode B and the blur scale**: BlurSampleScale 1 with mode B means the taps are used at one texel of the **target** level (CORRECTED, part BJ: the viewport that `cbFilterParam` reads is the render target's, GLOW-WAVE1-READY §1.3), i.e. 4 source texels for the first blur and 2 for the others.

## 2. The weight rule, by shader family (STATIC, DXBC; `glow-outputs.tsv`)

**G-buffer programmes (`o5`).** For the two witness families, the instructions are:

| Family | `o5.xyz` | `o5.w` |
|---|---|---|
| `GXFlver_ColDifSpcBumpEmiIbl_Bloody_GBuf` | `sample(t6, uv) × cb2[2].xyz` = `g_EmissiveTexture × FC_EmissiveColor` | `r4.w` from `movc r4.xyw, (cb3[5].z ≠ 0), alpha, (0, 1, 0)`: **0** unless the flag selects the alpha |
| `GXFlver_ColDifSpcBumpEmiIblGlow_Bloody_GBuf` | the same | `movc o5.w, (cb3[5].z ≠ 0), alpha, cb3[9].w`: **`FC_GlowScale.w`** |

`cb3[9]` is `FC_GlowScale@144`, produced by the MTD parameter `g_GlowScale` (builtin parameter 55: kind 1, vector index 9, so the generic numeric route; `builtin-material-parameters.tsv`). `cb3[5].z` is
the third word of `FC_AlphaRef`; the MTD `g_AlphaRef` is an `Int2` (x, y), so the flag is INFERRED zero for these materials.

**Bank-wide census** (non-phantom programmes with two or more outputs, 1,311 in the cache):

| Target | Programmes | Rule |
|---|---|---|
| G-buffer o5 (6 or 7 outputs) | 212 programmes = 106 families x plain and alpha-tested `[A]` (the `[A]` variants write `movc o5.xyzw, discard, 0, r4`: the same value, zeroed where the alpha test fails). Plain programmes: `o5.w` = `cb3[9].w` for **56 families**, computed from a register for 50 | Glow families bloom: `EmiIblGlow[_Cloth|_ZFeather|_Bloody]`, `EmiMulIblGlow`, `EmiMulIblGhost`, `EmiDOLGlow`, `EmiEnvDOLGlow`, `EmiMulMask…Glow`, `EmiGlow`, `SSSEmiIblGlow`, `C_ARSN_cinder[_em]`, `C[ARSN][cinder]`, `Fur_FDP`, `Dragon_FDP`, `BurnCloth`, `RecursiveCrystal`, `SAT_Equip`, `SAT_Equip_Df/SSS/Veill/em`, `SATTest_Burn`, `FlowmapVertexOffset_em`, `Veill_Curtain`, `c5280LastBoss_em`, `C_ARSN_em`; the plain `Emi…` families and the map families (`MultiBlend2_et1_em…`, `nolight`) compute `o5.w` from their own alpha |
| forward o1 (2 outputs) | 348, of which 276 `o1.xyz = r × cb3[9].xyz` (`FC_GlowScale.xyz`), 24 `mad`, 24 `o1 = 0` | forward Glow families write the glow colour; `o1.w` is a mask × alpha |
| FFX o1 | 44 `…GlowD` | `o1 = r0 × cb0[22]` (`g_psGlowScale`), colour and weight |

So **the sources of bloom are three, as EMISSIVE-GLOW §3 said, but the G-buffer one is selected by family weight, not by the mere presence of an emissive texture.**

## 3. What native has and lacks (STATIC, native at `b5b20a63`)

| Item | Original | Native today | Gap |
|---|---|---|---|
| GB5 | RGBA16F, written by 6-output G-buffer programmes | `color_formats[5]` RGBA16F, six targets, cleared to 0; the 6th output of Emi/EmiGlow programmes lands there (R7 of NPC-W347) | none |
| GB5 consumers | light composite t8; glow filter t0 | light composite t8 (`scene_pass.cpp:1018`) | glow filter |
| forward second target | rows bind [HDR, GB5]; RT1 follows RT0's blend (IndependentBlendEnable 0, EMISSIVE-GLOW §8.1); write mask 0 for one-output programmes | forward rows bind one target (`game_bootstrap.cpp:4867`, `render_target_count=1`); a 2-output Glow forward programme has no matching RT1 | second target; or the programme is refused/loses o1 |
| glow filter | `GXGlowFilter` 10 draws, after YEBIS + DLAA, onto the final LDR | none; the final display is a plain composition (`scene_pass.cpp:464`) | whole filter |
| filter parameters | gparam Glow group via the view key, blended by `FB890` | none (no Glow record, no gate read) | `Banks::Glow` + gate |
| factor | `[[GXRS+0x5368]+0x80]` | object built by `FUN_127010`, value 1 | read it |
| FFX glow twins | `…GlowD` second target | effect system absent | effects phase |
| tone mapping order | glow added to the final LDR after AA | no YEBIS yet | compose onto the native final colour now (risk 3 of EMISSIVE-GLOW), keep after YEBIS/AA later |

No material-level refusal stands in the way: the `FC_GlowScale` word is produced by the generic numeric route, the Glow programmes resolve (NPC-W347 R1), and the six-target G-buffer is in place.

## 4. Correction by family

| Family class | Examples (witness) | Needs | Notes |
|---|---|---|---|
| **G-buffer Glow** (56 families) | `EmiIblGlow_Bloody` (`c2150` W3, `c5270` W4), `SSSEmiIblGlow`, cinder, Fur, Dragon, SAT_Equip, ghosts | the filter (steps 3–8) | the weight `FC_GlowScale.w` is already produced; nothing per material |
| **G-buffer plain Emi** | `EmiIbl_Bloody` (`c1090`, `c5110`), `EmiMulIbl`, `EmiDOL` | nothing | lit by the composite (t8); no bloom in the original |
| **Forward Glow** (276 programmes) | forward `…EmiIblGlow_*_Fwd`, `SAT_Equip*_Fwd`, `Veill_Curtain`, cinder `_Fwd` | step 1 (RT1 = GB5, RT0's blend, write mask 0 for one-output programmes) + the filter | the veil `Veill_NoLight_Fwd` has one output: no bloom |
| **FFX glow twins** (44) | `GXFfx…GlowD` (flames, auras, spells) | the effects phase (E01/E02 builders) + the filter | `o1 = r0 × g_psGlowScale` |
| **Glow filter** | all | steps 3–8 | one implementation serves all families |

## 5. Ordered native steps and witnesses

| # | Step | Witness |
|---|---|---|
| 1 | forward rows bind [HDR, GB5]: RT1 gets RT0's blend descriptor and write mask; one-output programmes write mask 0 on RT1 | a forward Glow draw: GB5 changes where only the forward row draws |
| 2 | `Banks::Glow(key)`: the Glow group of the view's admitted gparam files (MapBase id 0, RemoBase id 0 for cut scenes) with the engine's key rule and blend; read `[[GXRS+0x5368]+0x80]` each frame | the six values of §1 for the High Wall and m40; `f = 1` |
| 3 | glow resources: one 1280 × 720 R11G11B10 source and four levels (320 × 180 … 40 × 23, rounded up) | sizes |
| 4 | extraction `EmissiveToGlowPS` (GB5 → source), blur `Sample13Weight` ×4 (taps of EMISSIVE-GLOW §8.2, scale BlurSampleScale), composite ×5 (SRC_ALPHA / ONE; `(RgbScale × f)` ×4 then `(Final × f)`) | structure G1: 1 + 4 + 5 draws |
| 5 | placement after the native final colour (later after YEBIS/AA, before UI and gamma) | order (settled in part BD, MASTER-PLAN §6 S4: YEBIS, AA, glow, UI, gamma; constructor member order and captures) |
| 6 | gate: `Enable` and `f > 0`, otherwise skip all ten draws | G6 |
| 7 | image witnesses | `c2150` at Firelink (`m40_00` (309.3, −35.4, 617), TestAnim save): a halo; `c1090` at `m30_01` next to it: bright, **no halo**; the High Wall torch bowls (`M[ARSN]_em`, MENUS-MAP §54) once the map families are admitted |
| 8 | FFX twins with the effects phase | flames and the sign glows (EFFECT-FAMILIES-SIGNS-NATIVE: 4 of 5 sign billboards set `bloom`) |

## 6. Live rows (none taken)

| ID | Read | Expected |
|---|---|---|
| GL1 | the six Glow values at the High Wall and m40 (the `GXGlowFilter` fields `pipeline+0x2E80` in the original) | 1, 1, 1.5, 1, 1, 0.5 (INFERRED from `mapbase` id 0) |
| GL2 | `[[GXRS+0x5368]+0x80]` | 1.0 |
| GL3 | GB5 alpha over `c2150` (EmiIblGlow) and `c1090` (EmiIbl) in the original | `c2150` ≈ 1 over emissive texels, `c1090` 0 |
| GL4 | the blend descriptor of a two-target forward draw | RT1 = RT0 (EMISSIVE-GLOW §8.1) |

## 7. Corrections to earlier notes

- NPC-W347-NATIVE §3 ("glow extraction … EmiIbl, EmiIblGlow") and the NW5 image row: only the Glow families bloom; `EmiIbl` is lit-only.
- EMISSIVE-GLOW-NATIVE §3 ("G-buffer emissive materials write GB5"): they write GB5.rgb; the **alpha is the family's glow weight**, 0 for the plain Emi families.

## Exports and evidence

- Tools (this part): `Scan-GlowOutputs.ps1`; `I0068Table.java` extended (stride and integer count).
- Evidence (outside Git): `evidence/I-0068/queries/glow/glow-outputs.tsv`, `builtin-material-parameters.tsv`, `multi-output-ps.tsv`, `gbuf-o5-classes.tsv`.
