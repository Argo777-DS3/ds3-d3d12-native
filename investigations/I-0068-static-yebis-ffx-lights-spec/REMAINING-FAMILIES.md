# I-0068 part V: the remaining families without a spec (probes, unused passes, material variants)

Request (coordinator, 2026-10-10): after decals, water and lights, chain on the remaining
families without a spec in the coverage matrix:

- character light probes;
- velocity / motion blur;
- SSR, AO fields, cloud shadows;
- material variants.

Labels: STATIC, OBSERVED, INFERRED.

**Evidence:**

- I-0063's inventory (`drawn-families.tsv`, `family-status.tsv`) over all its runs;
- the 123 retained captures;
- Thomas's `GraphicsConfig.xml` (read-only);
- the DXBC reflection of the programmes (I-0063 programme cache).

## 1. Character light probes (matrix row 25): already specified

The coverage matrix listed this row as "no spec". **That was wrong:** I-0067
**TONEMAP-SKIN §10** ("Per-instance SH") specifies the whole path, STATIC and CPU only:

- the `1AC840`/`61950` blend toward a target SH;
- the probe pick `14001D650` over `G+0x46E8` (nearest probe in the 27 cells, lowest `+0x1A`);
- the `.btpb` volumes rotated by the map;
- the light-set rows `1495F0` (the hemisphere formula);
- the SH basis constants `143D7F940`.

**Native status:**

- `NativeObjectProbeLightBlend` keeps only the clock (`native/frame/game_bootstrap.cpp:5866`).
- The instance-SH producer already consumes `animation+0x130/+0x16C` when the block is owned
  (`native/frame/game_bootstrap.cpp:2205`). So the work is TONEMAP-SKIN §10's "native
  consequence" (3 steps).
- Ready to code from that section. No further static work is needed here.

## 2. Passes never drawn with the current settings (rows 26 and 39)

**OBSERVED.** None of these pixel programmes is drawn in any I-0063 run (m31, m37, m45, m50,
m51, the m40 streams), nor in the m40 named listing:

- `GXSsr`, `GXLightDeferredAOWithSsr` (SSR);
- `GXAOFields`, `GXAOFields_Sphere`, `GXAOFields_3DTex`, `GXAOFields_Combine_BilateralUpScale`;
- `GXLightAcc_DDirLightShadowed1_Cloud` (cloud shadows);
- `MotionBlur*`, `LensSimu*_MotionBlur*` / `*Velocity*`, and the material `_Velo` passes.

The `GXSsr.vpo` line in `drawn-families.tsv` is the full-screen VS it shares with
`GXScreenSpaceShadow`, not SSR. The frame uses `GXLightDeferredAO` and `DDirLightShadowed1`.

**Thomas's settings** (`%APPDATA%/DarkSoulsIII/GraphicsConfig.xml`):

| Setting | Value |
|---|---|
| `MotionBlur` | **DISABLE** |
| `ReflectionQuality` | MAX |
| `ShadowQuality` | MAX |
| `SSAO` | HIGH |
| `WaterSurfaceQuality` | HIGH |

SSR and AO fields are therefore not drawn even at MAX reflection quality in these zones. The
renderer selects them elsewhere: a map parameter or a per-area switch, not read here.

**STATIC owners** (programme lookups, where to start if one becomes visible):

| Feature | Lookup | Notes |
|---|---|---|
| SSR | `1457B0` (`u"GXSsr"`, plus a 0x140-byte constant buffer) | |
| AO with SSR | `1010A0` (`u"GXLightDeferredAOWithSsr"`) | |
| cloud shadow | `19E090` (`u"GXLightAcc_DDirLightShadowed1_Cloud"`) | in the cascaded-shadow object family of SHADOWS-NATIVE |
| AO fields | `0A12C0` (`u"GXAOFields"`) | RTTI `GXSR::GXAOFieldsRenderer`, `GXAOFieldsFileData` (baked per-map AO volumes) |
| velocity | material pass **1 `Velo`**, reached by row preparation **5** (`5A120`: preparation 5 → `+0x1C = 1`) | its post side is the `gxposteffect` `MotionBlur*`/`LensSimu*` family |

**Ready-to-code decision:**

- The native renderer draws **nothing** for these features, which is what the original does
  in every witness.
- Each one becomes a spec task only when a capture shows it drawn. For motion blur, Thomas
  would have to enable the setting.
- Native must not invent them: a neutral SSR or AO-field input would change the image the
  original produces.

## 3. Material variants (rows 4, 7, 11, 12): no new pass

DXBC (`GXFlver_ColDifSpcPomMulMaskIbl_Gbuf`, `Interior_Gbuf`, `Crystal_Gbuf`): these are
**ordinary G-buffer programmes**. They read the same blocks as M01:

- b1 `cbTextureTilingScale`;
- b2 `cbMatDynParam`;
- b3 `cbMtdParam`;
- b4 `cbInstanceData` (with `aSh`);
- b8 `cbSceneParam`.

They differ by family-specific inputs:

| Family | Witness (I-0063) | Specific inputs | Native path |
|---|---|---|---|
| M03 POM (`ColDifSpcPom*DOL`, `ColDifSpcPomMulMaskIbl`) | m31, m37 | b2 `FC_PomHeightScale`, `FC_PomMin/MaxSamples`, `FC_Pom2HeightScale`, `FC_BlendMaskParam`; t14 `g_BlendMaskTexture` | the generic `cbMatDynParam` producer from MTD values (`native/content/model_parameters.cpp:17`) |
| M10 interior windows (`Interior`) | m37 | composed (SPX `snp`) values at b3 +624..+656; t2–t4 | the composed-group path, `ProduceMtdParams` by RDEF name (`native/frame/game_bootstrap.cpp` cbMtdParam fallback) |
| M06 snow on surfaces (`*_SnowLt`) | m31 (VS variants `…DOL_SnowLt_*`) | a **VS** variant of the family; snow inputs in `cbPerFrame` (`FC_SnowDir`, `FC_SnowTilingScale`, …) | the native `cbPerFrame` producer (`native/frame/game_bootstrap.cpp:2074`) and the family name lookup |
| M11 crystal (`Crystal`, `RecursiveCrystal`) | none of the five zones. I-0064 RESULT notes the adult `c3100` "has crystal pass41", outside the G-buffer snapshot (INFERRED: engine pass 41 = 0x29, forward) | composed values at b3 +624..+648; a **cube** texture t1 | the composed-group path; INFERRED forward row 0x29 |

**Ready to code:**

- **Nothing new** for these families: they go through the existing G-buffer and forward rows.
- The task is to confirm, on native runs at m31 and m37, that they are **drawn and not
  gated**: look for no `native_capability_gate` and no `native_asm_part_class_gate` refusal
  for the families above.
- Then compare their G-buffer targets against the originals:
  - (V1) POM at m31/m37;
  - (V2) `Interior` at m37;
  - (V3) the `_SnowLt` VS at m31.
- Crystal needs a witness with `c3100`.

**Correction (part AC, NATIVE-GATES-AUDIT §2):** "nothing new" holds for POM only.

- **`Interior` and the Emi map families:** class `0x100` has no native row for map parts.
- **`_SnowLt`:**
  - the variant is in the **PS**, not the VS;
  - three snow words have no native producer.
- **Snp values of `Interior` and `Crystal`:** they sit in **b2** `cbMatDynParam` +624.., not
  b3. They come from the SPX generated parameters.

## 4. Coverage after this part

All the families the matrix listed without a spec are now covered:

- decals (S);
- water (T);
- spot, shadowed and line lights (U);
- character probes (TONEMAP-SKIN §10);
- the unused passes (decided: none drawn);
- material variants (no new pass).

**Still without a spec:**

- the planar water reflection view and the water interaction simulation (D07/E09; WATER-NATIVE
  risks 3–4);
- E08 effect light shafts (FFX-FAMILIES §3 table only);
- the line-light item producer (no witness).
