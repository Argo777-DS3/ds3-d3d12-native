# I-0068 part D: effect drawing (phase 2), family by family E01–E10

Request (coordinator, 2026-10-10): detail phase 2 of [FFX-SCENECTRL-NATIVE](FFX-SCENECTRL-NATIVE.md)
per appearance family E01–E10:

- the programmes of the shader set `FUN_140291520`;
- constant layouts, blend/depth/raster states, vertex and texture inputs;
- targets (half resolution and recomposition included);
- the four direct D3D11 sites one by one, with their D3D12 equivalents;
- the families ordered by frequency in a starting zone.

Sources:

- STATIC: decompiled GX FFX code (private read-only database copy);
- STATIC (shader bytes): full DXBC reflection of the 327 programmes of
  `gxffxshader.shaderbnd` (I-0063's programme cache, `Get-ProgramLayouts.ps1` →
  `evidence/I-0068/queries/ffx/ffx-program-layouts.tsv`);
- OBSERVED: I-0063 inventories (zone weights, programmes drawn live in the host era).

## 1. Order of work (starting zone m40_00, Cemetery of Ash / Firelink Shrine)

The weights come from I-0063 `inventory/zone-families.tsv`: FXR action occurrences in the zone's
effect bundles. They count placements, not screen time (OBSERVED; their use as a priority is
INFERRED).

| Rank | Family | m40 weight | Whole game | Drawn live in I-0063 | Start here because |
|---|---|---|---|---|---|
| 1 | **E01 billboards and sprites** (600/603/604) | 638 | 28,229 | yes, 8 `GXFfxtessellate*` programmes | most of every zone's effects: fire, embers, smoke, sparks, magic |
| 2 | **E03 model particles** (605) | 128 | 3,157 | not witnessed | reuses the native model path (FLVER materials) |
| 3 | **E02 lines and trails** (601/602/606/10012) | 66 | 1,804 | yes (trails) | weapon trails, line effects; same machinery as E01 |
| 4 | E06 effect lights (609/11000) | 40 | 2,914 | — | **phase 1** (no draw) |
| 5 | **E07 GPU particles** (10000/10001/10008/10009) | 30 | 270 | yes (compute + `PS_VFX_SingleTextureCheapGlowLP`) | compute simulation + GS expansion; separate machinery |
| 6 | **E05 radial blur** (608) | 26 | 1,701 | yes | needs the scene-copy input |
| 7 | **E04 distortion** (607) | 20 | 527 | yes (`GXFfxdistortionBump`) | needs the scene-copy input |
| 8 | E08 effect light shafts (10003) | 0 | 8 | not witnessed | only m30, m33, m38 bundles |
| — | E09 water interaction (10013), E10 lens flares (10014) | 0 | 0 | — | absent from every zone's effect bundles (I-0063 family list stops at E08) |

**Recommendation:**

1. E01 then E02: one machinery, §3.
2. E03.
3. E07.
4. E05 and E04 together (scene copy).
5. E08 last.

## 2. Shared machinery of the classic appearances (E01, E02, E04, E05, E08) (STATIC)

### 2.1 Programme choice

GX draw context `GXFfxDrawContext` slot 27 **`FUN_140264230`** (the "apply" of every classic
effect draw):

- **The draw kind** `ctx+0x964` is set by the appearance's draw-context slot (13–20, e.g.
  `FUN_140260830` writes its argument there), together with the FXR property evaluation (colour,
  scale, texture scroll, soft-sprite and fade parameters into the constant staging
  `ctx+0x2C0..+0x6FF`).
- **Programme index:**
  - a switch on the kind (0..0x2F, `140264230` lines ~280–1100) and on the flags (soft
    `ctx+0xA94` = depth test against the scene depth, glow `ctx+0xA95`, lit `bVar29`) picks an
    **index into the shader-set table** of `FUN_140291520`;
  - **the "GlowD" variant is index + 75** (0x4B). Example: kind 0 → 2 (`GXFfxtexture`), or 0x4D
    (`GXFfxtextureGlowD`).
  - The table, with VS and PS names per index, is
    `evidence/I-0068/queries/ffx/ffx-shader-set.tsv` (`Get-ShaderSetTable.ps1`). Summary:

| Indices (glow: +75) | VS / PS programmes |
|---|---|
| 0–2 | `GXFfxtexture` |
| 3–11 | `bumpTexture`, `softTexture`, `softBumpTexture`, `multiTexture`, `softMultiTexture` (+`2Sampler` PS), `colorOnly`, `pointSprite` |
| 12–35 | `GXFfxtessellate{,Bump,Soft,SoftBump}Texture`, PS `…Blend…` for 24–35 |
| 36–40 | `tessellateTracer{,Bump}Texture`, `tessellateLineColorOnly`, `tessellateQuadLineColorOnly`, `tessellatePointSprite` |
| 41–56 | `tessellate{,Soft}MultiTexture` (+`2Sampler`, `Blend…` PS) |
| 57–62 | VS `GXFfxdistortion`, PS `distortion`, `distortionBump`, `distortionRotation`, `distortionTexture`, `distortionBumpTexture` |
| 63 | `GXFfxradialBlur` |
| 64–70 | `bumpTexture_PNT1..3`, `softBumpTexture_PNT1..3` (1–3 point lights), `bumpDistorsionTexture` |
| 71–74 | VS `GXFfxLightShaft`, PS `LightShaftProcedual`, `…ProcedualMask`, `…Texture`, `…Dust` |
| 75–127 | glow (MRT) twins of 0–56 |
| 128–157 | **correction (part L):** the VS/PS arrays hold 158 entries. Ghidra had split off entries 128–157, which include the soft radial blur (139), soft distortions (140–145), soft and distortion tracers (146–153, 156–157) and the soft bump distortion (154–155). See [EFFECTS-E05-E04-NATIVE §0](EFFECTS-E05-E04-NATIVE.md) |

- **Tessellated programmes** add a hull and a domain stage per tessellation pattern: `_quad`,
  `_octed`, `_quincunx` (`GXFfxtessellate*_{quad,octed,quincunx}.hpo/.dpo`, 25 each).

### 2.2 Constants (reflection; the five scene-control buffers of FFX-SCENECTRL-NATIVE §2.2 match by size)

| Buffer | Register | Size | Scene-control member (INFERRED by size) | Content (fields in the TSV) |
|---|---|---|---|---|
| `VSCommonBuffer` | VS b0 | 448 | `+0xE8` (0x1C0) | view/proj/world-view-proj matrices, fog start/end, screen size, near/far, eye, near/far fade (`g_vs_fade*`), soft-sprite params, camera position |
| `DSCommonBuffer` | DS b0 | 448 | `+0xF0` (0x1C0) | the same for tessellated draws (`g_ds_*`, camera up for billboarding) |
| `PSCommonBuffer` | PS b0 | 560 | `+0xF8` (0x230) | `g_color`, light colours, hemisphere, near/far, wave, colour scale, ambient SH (48 B), tracer params, glow scale, texture/colour blend types, soft-sprite params, radial-blur lengths, light-shaft params, pre-alpha multiply |
| `PSLightBuffer` | PS b1 | 400 | `+0x108` (0x190) | `g_PL_CountMax` + `g_vPLArray` (384 B): point lights for the `_PNT1..3` and lit variants |
| `cbFog` | PS b6 | 224 | `+0x100` (0xE0) | the scene fog block (same layout as the native lighting `cbFog`) |

`FUN_140264230` uploads them with `UpdateSubresource`:

- VS from `ctx+0x140`;
- DS from `ctx+0x300` (tessellated only);
- PS from `ctx+0x4C0`;
- PSLight from `ctx+0x6F0` (lit only);
- fog from `ctx+0x880`, built by `FUN_14014E900` from `[GXRS+0x41E8]`.

It then binds them:

- VS b0;
- DS b0;
- PS b0 + b1;
- PS b6.

### 2.3 Textures and samplers

- **SRVs t0–t4** from `ctx+0x968..+0x988`:
  - t0 diffuse / multi 0 / frame buffer;
  - t1 normal / multi 1;
  - t2 blend / multi 2;
  - t3 **scene depth** (`[GXRS+0x4E10]+0x48`, else `ctx+0xA68`; used by every `soft*`
    programme);
  - t4 trace frame buffer.
- Bound `PSSetShaderResources(0, 5)`.
- **Sampler s9** (`g_ClampLinearSampler`) = GX common bank `[[4961AB0]+0x2A0]+0x2C8 + 8i`, or
  `+0x2E8` for pass type 5 when `ctx+0x9BA` is set. Distortion and light-shaft dust also use **s1**
  `g_WrapLinearSampler`.

### 2.4 States (GX common state bank `[[4961AB0]+0x2A0]`, created by `FUN_1400D21F0`)

**Blend** by the FXR blend type `ctx+0x960`, for pass type `ctx+0x18` 3 or 5:

| Blend type | Bank entry | D3D11 desc (GX enum values of `FUN_1400D21F0`; their D3D meaning INFERRED) |
|---|---|---|
| 0 | `+0x08` | blend off |
| 1 | `+0x10` | SRC_ALPHA / INV_SRC_ALPHA |
| 2 | `+0x20` | SRC_ALPHA / ONE (additive) |
| 3 | `+0x28` | SRC_ALPHA / ONE, op code 2 (INFERRED reverse subtract) |
| 4 | `+0x18` | DEST_COLOR / ZERO (INFERRED multiply) |

- The factors are GX enum values 0/1/4/5/8, converted by `FUN_141925360`. The I-0063 live
  inventory saw exactly ZERO, ONE, SRC_ALPHA, INV_SRC_ALPHA and DEST_COLOR (OBSERVED).
- Glow pass (type 6): MRT variants at `+0x1A0/+0x1A8/+0x1B0/+0x1B8` (target 1 = glow
  buffer).
- Type 1: `+0x08` (opaque).

**Depth-stencil:**

- `+0x1E0` for type 1;
- `+0x1E8` otherwise.
- INFERRED: test on, write off for translucent effects; the bank descriptors give the exact
  fields.

**Rasterizer:** `+0x208`, or `+0x218` when `ctx+0x9B8` (one-shot flag).

**Native:** the bank is already modelled on the I-0066 branch (`selected_observer.cpp` reads
`[[4961AB0]+0x2A0]` +`0x2C8` samplers and `+0x348` descriptors). Take the same descriptors
into the PSO key: blend entry, depth entry, raster entry.

### 2.5 Geometry and topology

- The classic appearances write vertices into the scene control's **2 MB GPU ring**
  (`sc+0xC0`), then draw through one of the three builders of §4:
  - non-tessellated kinds: **triangle list**;
  - tessellated kinds: **1-control-point patch list** (one control point per particle or trail
    segment, expanded by HS/DS).
- **Vertex inputs** (reflection), for example:
  - `GXFfxtexture.vpo`: `POSITION, COLOR0, COLOR1, TEXCOORD0, TEXCOORD3, TEXCOORD4`;
  - `GXFfxtessellateTexture.vpo`: `POSITION, COLOR0, COLOR1, TEXCOORD0..5`;
  - `GXFfxmultiTexture.vpo`: `POSITION, COLOR0..4, TEXCOORD0..2`;
  - `GXFfxdistortion.vpo`: `POSITION, TEXCOORD0`.
  - The exact formats and stride are the input layouts GX creates per programme. The I-0063
    live layouts are standard DXGI formats, slot 0 (OBSERVED).

### 2.6 Targets

- **Passes:** the GX pass ids the scene control declares, `sc+0x30..+0x40` = **0x21, 0x29
  (forward), 0x2D, 0x20, 0x31**, plus the scene-control submissions in passes **0x39** and
  **0x45** (FFX-SCENECTRL-NATIVE §3.2).
- Colour target: the lit HDR target of the forward pass.
- **`…GlowD` programmes write `SV_Target0` + `SV_Target1`** (69 of 168 pixel programmes,
  reflection): target 1 = the glow accumulation buffer that GX's glow filter reads after YEBIS
  (`EmissiveToGlowPS`, TONEMAP-SKIN §17; INFERRED link).
- **Half resolution** (`sc+0x150/+0x1B0` R8G8B8A8 or RGBA16F, `+0x238/+0x240` R8G8B8A8, created
  at view attach): **no reader in the executable** (§6.2, STATIC negative search). The earlier
  INFERRED role (distortion and bump buffers) is withdrawn.

## 3. Family by family

| Family | Programmes (shader set) | Inputs | Constants | States | Targets | Notes |
|---|---|---|---|---|---|---|
| **E01 billboards and sprites** | indices 0–35, 41–56 (+glow): `texture`, `soft`, `multiTexture`(2Sampler), `bump`, `colorOnly`, `pointSprite`; tessellated `tessellate*` with `_quad/_octed/_quincunx` HS/DS; `_PNT1..3` lit variants | t0 diffuse or multi 0–2, t1 normal (bump), t3 depth (soft); s9 | VS b0 or DS b0, PS b0, PS b1 (lit), PS b6 fog | blend by FXR type 0–4; translucent depth; raster bank | forward HDR; + glow target with `GlowD` | **OBSERVED** live: 8 tessellate programmes (I-0063). Most frequent. |
| **E02 lines and trails** | 36–40: `tessellateTracer{,Bump}Texture`, `tessellateLineColorOnly`, `tessellateQuadLineColorOnly`; non-tessellated `softTracer` | as E01; tracer inputs `TEXCOORD0..4` | DS b0 with `g_ds_camera*`, PS `g_vTracerParam` | as E01 | forward HDR (+glow) | 10012 (trail) uses the same tracer programmes (INFERRED). |
| **E03 model particles** | **not in the FFX set**: `GXFfxFlverModelRenderer` (SceneCtrl slot 9 `FUN_14024F940`, vtable `143D6A298`) draws FLVER models with their **material programmes** (`gxflvershader`) | the model's FLVER streams and MTD textures | the material blocks of the forward pass | the material's MTD blend mode | forward pass | Native: reuse the native model/material path with the FFX instance transforms. |
| **E04 distortion** | 57–62, 70: `distortion`, `distortionBump`, `distortionRotation`, `distortionTexture`, `distortionBumpTexture`, `bumpDistorsionTexture`; tessellated `tessellateDistortion{,Soft}TracerBumpTexture` | **t0 `g_frameBufferTexture` = a copy of the scene colour**, t1 normal, t2 blend, t3 depth, t4 trace frame buffer; s1 wrap + s9 clamp | VS b0, PS b0 (`g_ps_*`, wave params) | blend off or alpha (writes the refracted colour) | forward HDR, after a scene copy | needs the scene copy before the draw (§2.6). |
| **E05 radial blur** | 63: `GXFfxradialBlur`, `GXFfxSoftRadialBlur` | t0 diffuse (scene copy, INFERRED), t1 normal, t3 depth (soft) | PS `g_screenCenter`, `g_radialDistance`, `g_ps_RadialBlurSamplingLength*`, `g_ps_RadiationBlurConjtrastAdjust` | as E04 | forward HDR | |
| **E06 effect lights** | none | — | — | — | the light list | phase 1 (EFFECT-LIGHTS) |
| **E07 GPU particles** | **VFX set** (`apuStack_13a8/eb8/9c8/4a8` of `FUN_140291520`): `VS_VFX_StandardParticle` (`SV_VertexID`, structured buffers `StructuredInstance` 176 B, `IndexedInstance`), `GS_VFX_StandardParticle` (b9 `gcbSceneParam` 448 B), `PS_VFX_{Single,Soft}Texture[Cheap][Bump][GlowLP]` (s7 point clamp, s8 linear wrap, t0 albedo, t2 normal, t3 depth; b0, b1, b6, b9, b11 `cbCubeEnvSH1`); compute `CS_VFX_StandardParticle*` (emit, update by quality Lowest..Highest, bitonic sort, counter downsize), b0 `ccbSceneParam` (1104 B), b1 `ccbForceFiledParam` (3488 B); spark variants `CS/GS/VS/PS_VFX_Spark*` | append/consume particle buffers (176 / 208 / 336 B elements), UAV counters, sort buffers | as listed | from the appearance (`FUN_1402A7E70` standard, `FUN_1402B01E0`/`FUN_1402B1700` spark: GS, depth and raster set directly) | forward HDR (+ shadow variant `GenerateSdw`) | **OBSERVED** live: `CS_VFX_StandardParticle*`, `…Emit`, `PS_VFX_SingleTextureCheapGlowLP`. Simulation runs in the **update** passes (FFX-SCENECTRL-NATIVE §3.1). |
| **E08 effect light shafts** | 71–74: VS `GXFfxLightShaft`, PS `LightShaftProcedual`, `…ProcedualMask`, `…Texture`, `…Dust` | t0 diffuse, t1 multi 1 (dust), t3 depth; s1, s9 | PS `g_Decay`, `g_DecayWeight`, `g_Dencity`, `g_Exposure`, `g_SampleNum`, `g_LightShaftScreenCenter`, `g_ps_LightShaft*` | appearance `GXVfxParticleAppearanceLightShaft` slot 8 `FUN_140271B40` sets the shaders directly | forward HDR | absent from m40 |
| E09 water interaction, E10 lens flares | no programme in the FFX set | — | — | — | — | absent from the game's effect data (I-0063) |

**Other VFX appearances** (not an I-0063 family, rarely used):

- **fluid** (`GXVfxFluidAppearance` slot 8 `FUN_1402701C0`):
  - `PS_VFX_Advect/AdvectBfecc/AdvectVel/Divergence/Jacobi/Project/Vorticity/Confinement/
    Gaussian/GridSlid/GridAddVelocity/DrawTexture/DrawBox/DrawWhite`, b0 `fcbFluidParam`
    (160 B);
  - grid passes through `FUN_14028F890..FUN_140291190`.
- **volume rendering** (`FUN_1402BC5B0`): `PS_VFX_RayCast*`, `RayData*`, `EdgeDetect`, b1
  `fcbVolumeRenderParam` (224 B).
- **"wandering vision"** compute: `CS/GS/PS_VFX_WanderingVision*`.

Implement them only on a witness.

## 4. The four direct D3D11 sites, one by one (STATIC)

| Site | What it does | D3D11 | D3D12 equivalent |
|---|---|---|---|
| **`FUN_14036FD70`** (`GXFfxGraphicsResourceManager` slot 1) → `FUN_14036F9E0` | creates the GPU side of an FFX resource when an FXR resource is resolved. It allocates a CPU record of `n²·0x780 + 0x78` bytes from the heap, initialises it from constant tables (`143D7F320`…), and creates the GX textures and buffers the effect needs (names `GXFfxPostBumpTexture`, `GXFfxPostTexture`, `GXFfxBumpTexture`, `GXFfxTexture`, `GXFfxSelectedStateBufferTexture`). | GX buffer/texture helpers on `[4961AB0]` (`CreateBuffer`/`CreateTexture2D` + SRV) | committed (or pooled placed) resources + SRV descriptors in the native heap; initial data through the upload ring and a copy; release through the frame fence (resource manager slot 2 `FUN_14028F6E0`). The CPU record stays the engine's. |
| **`FUN_140385F70`** (table `143D7E6C0`, 24 KB) | **tessellated** geometry builder: writes one control point per particle or trail segment into the ring | `IASetPrimitiveTopology(GX 10 → D3D 0x21 = 1_CONTROL_POINT_PATCHLIST)` (`FUN_141929BA0` mapping), `Draw(n)`; indexed variant `IASetIndexBuffer(R16_UINT)` + `DrawIndexed` | PSO with `PrimitiveTopologyType = PATCH`, HS/DS = the `_quad`/`_octed`/`_quincunx` programmes; `IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST)`; VB view on the ring; `DrawInstanced(n, 1, start, 0)` / `DrawIndexedInstanced` |
| **`FUN_140396F30`** (table `143D7E840`, 12 KB) | **non-tessellated** builder (camera-facing quads as triangles) | `IASetPrimitiveTopology(GX 4 → TRIANGLELIST)`, `Draw(n)` | PSO `TRIANGLE`; `DrawInstanced(n, 1, start, 0)` |
| **`FUN_14039E900`** (table `143D7E900`, 7 KB) | draws through an interface (`param_2` slots `+0x28..+0xD8`) | `Draw(4)` (`FUN_141921370`) | `DrawInstanced(4, 1, 0, 0)`; topology from the PSO. INFERRED: a 4-vertex strip per item (point sprite or shaft quad); confirm the topology on the first witness. |

## 5. Native implementation notes (INFERRED design)

1. **One root signature for classic FFX:**
   - CBV b0 (VS), b0 (DS), b0 (PS), b1 (PS), b6 (PS) as root CBVs from the upload ring;
   - SRV table t0–t4;
   - sampler s1 and s9 from the GX bank (static samplers are possible: clamp linear and wrap
     linear do not vary per draw).
2. **PSO key:**
   - shader-set index (+glow) and the HS/DS pattern;
   - input layout of the VS;
   - blend entry (5 types × glow MRT), depth entry (`+0x1E0`/`+0x1E8`), raster entry;
   - RT formats (HDR, + glow);
   - topology type (PATCH or TRIANGLE).
3. **GPU particles (E07)** need:
   - a compute root signature (b0 `ccbSceneParam`, b1 `ccbForceFiledParam`, UAVs for
     append/consume buffers and counters, SRVs t0–t4);
   - the update-pass hook of FFX-SCENECTRL-NATIVE §3.1 (simulation before drawing);
   - an indirect or counter-driven draw (`VS_VFX_StandardParticle` reads `SV_VertexID` and
     the instance buffers).
4. **Scene copy for E04/E05:** a copy of the HDR colour, resource state COPY_SOURCE →
   PIXEL_SHADER_RESOURCE, before the first distortion or radial-blur draw of the pass.
5. **Witness order:**
   1. an m40 bonfire or candle (E01);
   2. a weapon swing (E02);
   3. a dropped-item model effect or pyromancy (E03, E07);
   4. a heat-haze or magic distortion (E04, E05).

## Exports and evidence

- `decompiled/DS3-1152-F-00264230` (state and programme apply), `00291520` (shader set),
  `000D21F0` (GX state bank), `01929BA0` (topology map), `00260830` (an FXR
  property slot).
- `evidence/I-0068/queries/ffx/ffx-shader-set.tsv`, `ffx-program-layouts.tsv` (both regenerable:
  `Get-ShaderSetTable.ps1`, `Get-ProgramLayouts.ps1`).

## 6. Static closure of the open points (step 3, 2026-10-10)

### 6.1 Constant layouts (STATIC; `FUN_140264230` = draw-context slot 27, the commit)

- **The CPU image is the constant-buffer image.** The commit uploads each buffer whole:
  `UpdateSubresource` (`FUN_141920300`), no box, from a block of the draw context. So the
  byte layout of each block is the reflection layout of §2.2 (`ffx-program-layouts.tsv`),
  with no translation.

  | Buffer | Source block | GX buffer (`ctx` member) | Bound |
  |---|---|---|---|
  | `VSCommonBuffer` 0x1C0 | `ctx+0x140` | `ctx+0x108` | VS b0 |
  | `DSCommonBuffer` 0x1C0 | `ctx+0x300` (tessellated draws only) | `ctx+0x110` | DS b0 |
  | `PSCommonBuffer` 0x230 | `ctx+0x4C0` | `ctx+0x118` | PS b0 |
  | `PSLightBuffer` 0x190 | `ctx+0x6F0` (lit variants only) | `ctx+0x128` | PS b1 |
  | `cbFog` 0xE0 | `ctx+0x880`, filled by `FUN_14014E900` from `[GXRS+0x41E8]` | `ctx+0x120` | PS b6 (`FUN_141920E20(…, 6, 1)`) |

  - The draw-context constructor `FUN_14025FB80` zeroes the blocks (`0x230`, `0x1C0`,
    `0x190` bytes).
  - The appearance setups (slots 13–20) and the commit fill the fields.
- **The three direct builders have no private constant buffers.**
  - `FUN_140385F70` and `FUN_14039E900` call draw-context slot 27 (`vt+0xD8`) before their
    draws.
  - `FUN_140396F30` reaches it through its helpers (INFERRED, same pattern).
  - Their per-vertex data are written by format-specific writers from their tables
    (`[table + i·8 + 0x10]`) into the scene-control ring.
- **Native:** upload the five context blocks as they are, into the upload ring, and bind root
  CBVs (b0 VS/DS, b0 + b1 PS, b6 PS). The engine code that fills them stays the engine's.
  Nothing has to be laid out field by field.

### 6.2 The half-resolution targets have no reader (STATIC, negative search)

> **Withdrawn for `+0x238/+0x240`** (I-0140 tranche 10, [STUDY-LIGHTSHAFT-10003](../I-0140-effect-families/STUDY-LIGHTSHAFT-10003.md)):
> the light shafts' draw slot `140271B40` reads both through `[[draw context+0x10]]` (the scene control), which the search below
> did not follow; `+0x150/+0x1B0` stay unread as far as is known.

- **Searched:** every instruction with displacement `0x150`, `0x1B0`, `0x220`, `0x228`,
  `0x230`, `0x238` or `0x240` in the FFX code (`0x140240000..0x1402BFFFF`), plus every use
  in functions that load the scene control through `[1449623C0]` or `[144799C98]`.
  `evidence/I-0068/queries/ffx/step3/`.
- **Found:** only lifecycle code:
  - constructor `FUN_14024B260`;
  - view attach `FUN_14024F2C0`;
  - resize `FUN_14024F630`;
  - detach `FUN_14024F1D0`;
  - shutdown `FUN_14024C830`;
  - destructor `FUN_14024C3B0`.
- **Conclusion** (INFERRED from the negative search; dynamic offsets are not excluded):
  - DS3 1.15.2 allocates the two half-size `GXRenderToTexture` (`+0x150`, `+0x1B0`) and the
    GX textures (`+0x220..+0x240`) but never renders to them or samples them;
  - the effect pixel programmes take their textures from `ctx+0x968..+0x988` (§2.3), which the
    appearance code fills.
- **Native:** do not allocate them. If a later witness shows a draw that targets a half-size
  view, revisit.
- **Texture defaults** (STATIC, commit):
  - with `ctx+0xAB9` set, t0 becomes GX's default texture `**[[4961AB0]+0x270]` (the first of
    17 solid-colour textures built in `FUN_140006C00`: white, black, grey, …);
  - t3 is the scene depth `[GXRS+0x4E10]+0x48`;
  - t4 (`g_TraceframeBufferTexture`) is the copy of `ctx+0x990`. **Correction (§8):**
    `FUN_140299E40` also writes `ctx+0x990` (a stack store the register search filtered out) with the
    frame copy, so t4 is the frame copy too.

### 6.3 ~~Still open~~

- Closed in §8: E04/E05 get `g_frameBufferTexture` from the pipeline frame copy.

## 7. The five witness zones (step 4, 2026-10-10; STATIC weights + OBSERVED notes)

The witness zones (Thomas, 2026-10-10) are:

- Irithyll `m37_00`;
- Undead Settlement `m31_00`;
- Ringed City `m51_00` (`m51_01` has identical weights);
- Dreg Heap `m50_00`;
- Ariandel `m45_00`.

The weights are I-0063's FXR action occurrences in each zone's effect bundles. Each zone also
loads `shared:commoneffects`; `m45` adds `shared:dlc1`; `m50`/`m51` add `shared:dlc2`.
The shared bundles are shown after "+" (INFERRED: which DLC bundle each map loads). They are
regenerated by `Get-ZoneEffectFamilies.ps1` → `evidence/I-0068/queries/ffx/zone-effect-families.tsv`.

| Zone | Rank 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|
| m31 Undead Settlement | E01 564 (+4066) | E03 84 | E06 64 | E02 42 | E05 36 | E04 24 | E07 10 |
| m37 Irithyll | E01 1490 | **E06 170** | E02 126 | E05 112 | E03 106 | E04 54 | E07 14 |
| m45 Ariandel | E01 2346 (+4435) | E03 172 | E02 132 | E06 132 | E05 82 | E07 32 | E04 10 |
| m50 Dreg Heap | E01 2066 (+4664) | **E06 170** | E03 78 | **E05 58** | E02 46 | E04 12 | E07 4 |
| m51 Ringed City | E01 2986 (+4664) | **E06 290** | E03 286 | **E05 134** | E02 124 | E04 22 | E07 14 |
| m40 (reference, §1) | E01 638 | E03 128 | E02 66 | E06 40 | E07 30 | E05 26 | E04 20 |

- **No new family.**
  - E08 effect light shafts has 0 in all five zones and in their shared bundles; it exists only in
    the m30, m33 and m38 bundles.
  - E09 and E10 have 0 everywhere.
  - The phase-2 family set (E01–E07, plus E08 for m30/m33/m38) is unchanged.
  - The light shafts seen on m31/m50 belong to the YEBIS post chain (YEBIS-TECHNIQUES §4 row 12),
    not FFX E08.
- **Aggregate over the five zones** (zone weights):
  - E01 9452;
  - E06 826 (phase 1, no draw);
  - E03 726; E02 470; E05 422; E04 122; E07 74.
- **Order for the draws** (supersedes §1's m40 order for the witness zones):
  1. **E01**;
  2. **E03**;
  3. **E02**;
  4. **E05**;
  5. **E04**;
  6. **E07**.
  - E05 and E04 share the scene-copy input, so they are done together.
  - E06 (phase 1) is ranked 2nd on m37, m50 and m51. The effect lights of FFX-SCENECTRL-NATIVE
    phase 1 are worth most there.
- **Exception, Ariandel:**
  - weights count placements, not screen time;
  - I-0063's live notes on m45 name the missing **falling snow (A02/E07 GPU particles)** as the
    dominant visible gap (`evidence/I-0063/LIVE-RUNS.md`, i63-live-20 and later).
  - For the m45 witness, E07 comes before E05/E04.

## 8. The frame copy behind `g_frameBufferTexture` (E04, E05; STATIC; second request of 2026-10-10)

**What it is:**

- Pipeline member **`+0x1108`** (the pipeline instance of `FUN_1400843C0`): a GX texture
  (0xA0-byte object, vtables `143D52B48` / `143D1D638`).
- It is created in **`FUN_140085540`** (pipeline target creation, also the resize path):
  - size = the view size (`param_2[0..1]`);
  - **format `R16G16B16A16_FLOAT`** (10), or **`R8G8B8A8_UNORM`** (0x1C) when the pipeline's LDR
    flag `+0x74` is set;
  - 1 mip, 1 sample.
- It has the format of the lit HDR target (R16G16B16A16 in the retained captures).

**When it is filled** (per frame, `FUN_140103510`):

1. Pass 0x20: the pair (frame copy, view target) is registered in the GX command context
   (`FUN_1400BEED0` → `ctx+0x740/+0x748`).
2. **Pass 0x29 (forward):**
   - `FUN_1400BFA70(cmd, +0x1108, [viewTarget+0x30]+8)` copies the **lit HDR target**
     (`pipeline+0x240`, or `+0x660` with `DAT_144961BA0`; for pass id 0x3A the
     `[pipeline+0x2DA0]+0x1040` target);
   - the copy is a resolve when the source is multisampled, else a plain copy;
   - then `FUN_1400BEDB0(slot 0x2D, SRV)` binds the copy as **GX global pixel SRV slot 45** for the
     forward programmes.
- Earlier passes (around passes 0x1A–0x0A) bind `+0x1108` at slot 0x37. INFERRED: those passes see
  the previous frame's copy.

**How effects get it:**

1. `FUN_140204C30` (`GXSgDrawScene` context build):
   - `passCtx+0x4C0 = pipeline+0x1108`;
   - `+0x4C8 = pipeline+0xFD0` (depth);
   - `+0x4D0 = pipeline+0x3958`; `+0x4D8 = +0x1110`; `+0x4F0 = pipeline`;
   - `+0x4E0 = pipeline+0x2F80` when flag bit 16 of `+0x50` is set.
2. `FUN_1402997E0` (effect draw-pass visitor) copies them into the draw command (`cmd[4..9]`).
3. `FUN_140299E40` builds the draw context:
   - **`ctx+0xA70` = frame copy SRV** (`[cmd[4]]+0x48`);
   - `ctx+0x990` = the same (t4);
   - `ctx+0xA68` = depth SRV (t3 fallback);
   - for appearance kinds 0x30/0x31, `ctx+0xA68` = `[[cmd[6]]+0x28]+0x48`.
4. Draw-context **slot 8 `FUN_140260410(tex)`**:
   - a null texture → **t0 = `ctx+0xA70`** (the frame copy) and flag `+0x9BA`, which also selects
     the alternative sampler `+0x2E8` (§2.3);
   - otherwise t0 = tex.
5. The other texture slots (draw-context vtable `143D65688`): slot 4 t0; 5 t1; 7 t2; 6 t3; slot 9 t3 =
   depth (`[GXRS+0x4E10]+0x48`, else `ctx+0xA68`); slot 10 t0..t2 at once.

**Ordering:**

- Effects drawn in pass 0x29 or later (0x31) read the scene **as lit before the forward
  translucents**.
- Effects in earlier passes (0x20, 0x21) read the previous frame's copy (INFERRED from the order).

**Native:**

- One texture of the native HDR target's size and format, recreated on resize.
- At the start of the native forward pass:
  - transition the HDR target to COPY_SOURCE and the copy to COPY_DEST;
  - `CopyResource` (`ResolveSubresource` if MSAA);
  - transition both back (PIXEL_SHADER_RESOURCE for the copy).
- Expose its SRV:
  - as the effect draw context's frame buffer (t0 when slot 8 receives null, and t4);
  - as GX slot 45 for the GX forward programmes that read it.
- The scene control's half-resolution targets remain unused (§6.2).