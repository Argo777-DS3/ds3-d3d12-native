# I-0119 - DS3 DX12 Vague 2: R3 of the RT/PT plan, the materials for the rays (Les matériaux pour les rayons)

Capability: C3

## Question

Can a ray that hits any surface of I-0117's scene know that surface's material as DS3's own G-buffer programmes would write it
(albedo, reflectance F0, gloss, shading normal, emissive, alpha), without the G-buffer, so that reflections and bounces (R6, R7)
get the right colours (RT-PT-PLAN §8 R3)?

## Scope

Included (the tranches of [R3-PLAN](R3-PLAN.md)):

- The gloss channel identified in DS3's programmes and turned into a roughness the traced lighting and Ray Reconstruction can use.
- A material record for every LOD-0 slice of the rays' instances (not only the alpha-tested ones: T3's record is its seed),
  from the slice's G-buffer programme interface, its textures (resolved as the raster resolves them), its MTD values and its
  vertex layout.
- The material control view: the camera's primary rays shaded by the records, compared numerically with the G-buffer the raster
  wrote at the same pixels (GB0 normal, GB1 gloss, GB2 albedo, GB3 sqrt F0), by programme family.

Excluded:

- The lights (R4), reflections (R6), GI (R7), the denoiser's guides (with Ray Reconstruction); the lightmaps (DOL, the baked
  light: lighting, not material, which the traced light replaces); the transparent and forward rows' materials (R9).

## Inputs

- I-0117's scene and records (`RtAlphaSet`, `RtAlphaTable`, `rt_alpha.hlsli`, the control view `rt_view.hlsl`).
- The G-buffer programmes (the I-0063 programme cache, 676 `*Gbuf*.ppo`), the light accumulation programmes (`GXLightAcc_*`),
  their disassembly (d3dcompiler_47, `evidence/I-0068/queries/variants/Disassemble-DxbcFiles.ps1`).
- The native's programme interfaces, texture holders, MTD parameters and vertex layouts (`NativeModelLayout`,
  `ProduceMtdParams`, `ProduceModelTextureTiling`).

## Hypotheses

- H1: most G-buffer families follow one shape by texture name (albedo t0 × FC_DifMapMulCol, F0 the specular texture, gloss the
  bump map's b × (g_SpecularPower / 256)², normal the bump map's rg in the vertex frame), so one record shape covers most pixels.
- H2: the families off that shape (two layers, blend masks, decals on characters, parallax, snow) show in the control view's
  numeric comparison and are few by pixels.

## Expected evidence

The programmes' formulas read in their disassembly (OBSERVED); the control view's agreement per channel and family at Firelink,
the High Wall and m45 (runs announced); the families that disagree listed with their pixel share and explained.

## Exit conditions

- Success: the covered families' traced material agrees with the G-buffer on the agreeing-depth pixels within a tolerance set from
  the first measure; every remaining disagreement explained or listed as a later tranche.
- Refuted: the records cannot reproduce the G-buffer channels without each family's programme.
- Insufficient information: the comparison cannot tell a material error from the mip or the vertex frame (then a finer witness).
- Stagnation: no new discriminating evidence after 3 cycles.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`; renew a bounded budget automatically within unchanged authorized scope and existing protected boundaries.

## Safety and human interaction

Live runs announced (WORKFLOW « Live game rule »), offline, test saves restored by hash; the ASUS (`\\.\DISPLAY1` while the OLED
is off), the OLED only when Thomas is there and says so. No irreversible change.
