# I-0068 part F: YEBIS techniques per draw, bloom arithmetic, HAL creator slots

Request (coordinator, 2026-10-10, step 2): close YEBIS-NATIVE §7:

- the technique of each draw (link each PS to its `CTinyEffect` technique);
- the bloom weight arithmetic;
- the names of creator slots 60, 62 and 64–67.

Method:

- STATIC: decompiled selection code and the library's own diagnostics.
- OBSERVED: the retained host-era captures. Their bytecode was removed (retention), but
  `events.jsonl` keeps per draw the VS/PS programme ids, the vertex stride and the bound views,
  and `resources.jsonl` keeps the view sizes and formats.
- A programme id is a creation index within one launch (YEBIS creates programmes lazily). Ids
  therefore say which draws share a VS or PS **inside one capture only**.

Labels: STATIC, OBSERVED, INFERRED.

## 1. Answer first

- **Technique names are computed, not looked up by string at draw time.** `CTextureUtil`
  initialisation (`FUN_1417134E0`) fills handle tables.
  - Name = base name with its trailing `n` replaced by the row number, plus one of 22 HDR
    encoding suffixes (§2). Example: `tech_MadT1_Sn`, row 4, suffix 0 → `tech_MadT1_S4`.
  - Every draw leaf reads `table[row][suffix]`; the row is the tap or texture count.
- **Per draw, the technique follows from the sizes and the stage** (§3–§4):
  - every downsample is `tech_MadT1_S{taps}`, with taps from the size ratio;
  - every bloom upsample and the final glare resample are `tech_MadT1_S4` (2×2 taps forced by a
    0.375-texel bias);
  - the bright pass is `tech_BrightPassT1_[Vignette_]THRESHOLD_LUMINANCE_REMAP_S1`;
  - the bloom level combine is `tech_MadLumGammaT{levels}` (6, or 7 on m31);
  - the glare composite is `tech_MadLumGammaT{components}`;
  - the bloom and DoF blurs are `tech_MadT1_S{n}` with per-channel weights (n odd, from the
    Gaussian table, §5);
  - the tone map is `tech_TonemapHDR_…` (VS `vs_2D_Tex2D5`, 4 textures), and the colour
    grading is `tech_MadColorGradingLutT1_S1`.
- **Corrections to YEBIS-NATIVE §3.2** (OBSERVED in the five witness zones, §4):
  - the 6-input draw is the **bloom level combine** inside `GenerateGlare_Bloom`, not
    `_Composite`;
  - `_Composite` is the later 1- or 2-input draw (bloom + afterimage on m40, bloom + light
    shafts on m31/m50);
  - the bloom levels are base, base, base/2, base/4, base/8, base/16 (m31 adds base/32), not base/2 … base/64.
- **HAL creator slots** (STATIC, the slots' own diagnostics):
  - 60 `CreateTexture` (wrap an existing resource);
  - 61 `CreateTexture`;
  - 62 `CreateVolumeTexture` (wrap);
  - 63 `CreateVolumeTexture` (**stub, returns null**);
  - 64 `CreateRenderTarget`;
  - 65 `CreateDepthStencilSurface`;
  - 66 `CreateSurface` (wrap);
  - 67 `CreateSurface`.
  - YEBIS-NATIVE had 64/65 swapped and called 63 a 3D-texture creator.

## 2. Handle tables (STATIC, `FUN_1417134E0`, table `evidence/I-0068/queries/yebis/techniques/handle-tables.tsv`)

- **`FUN_141716BF0(this, offset, base, rows)`:**
  - for row r = 1..rows, name = `sprintf("%s%d", base minus "n", r)` + suffix[s], s = 0..21;
  - handle = effect `vt+0x58` (find by name), checked by `vt+0xC0`;
  - stored at `this + offset + 0x58·(r−1) + 4·s`; a missing technique stores 0.
- **Suffixes** (`1448E7490`):

  | s | suffix |
  |---|---|
  | 0 | none |
  | 1–6 | `_REINHARDRGB_RGB`, `_REINHARDRGB_RGBFAST`, `_RGB_REINHARDRGB`, `_REINHARDRGBLUM_RGB`, `_REINHARDRGBLUM_RGBFAST`, `_RGB_REINHARDRGBLUM` |
  | 7–12 | the same six for `LOGRGB` / `LOGRGBLUM` |
  | 13–15 | `_RGBALUM_RGB`, `_RGBALUM_RGBFAST`, `_RGB_RGBALUM` |
  | 16–20 | `_REINHARDRGB_REINHARDRGB`, `_REINHARDRGBLUM_REINHARDRGBLUM`, `_LOGRGB_LOGRGB`, `_LOGRGBLUM_LOGRGBLUM`, `_RGBALUM_RGBALUM` |
  | 21 | `_ALPHA_RGB` |

  - DS3's HDR targets are R16G16B16A16 and R11G11B10 with plain RGB, so every observed draw
    uses s = 0 (INFERRED from the formats: no encoding is needed).
- **Leaves read** `leaf_this + X` with `leaf_this = init_this − 0x38`:

  | Table | Init offset | Leaf |
  |---|---|---|
  | `tech_MadT1_Sn` (32 rows) | `0x11C` | `FUN_14171EF10`, `FUN_14171EAE0`, `FUN_14172A660`, `FUN_14171F1F0` (`0xFC`) |
  | `tech_MadScalarWeightT1_Sn` | `0xC1C` | `FUN_14171F1F0`/`FUN_14171EAE0` (`0xBFC`), when all channel weights are equal |
  | `tech_MadT2_Sn` | `0x221C` | `FUN_14172A660` (`0x21FC`), when an additive second source is given |
  | `tech_TexCoordOffsetMadTn` | `0x432C` | `FUN_14171E600` (`0x430C`) |
  | `tech_MadLumGammaTn` | `0x4E2C` | `FUN_141720630` (`0x4E0C`) |
  | `tech_BrightPassT1_*_Sn` (9 entries, `0x6C14..0x6ED4`) | index `v + 3·(mode−1)` | `FUN_141722480` |
  | `tech_TexCoordMadT1_Sn` | `0x1A20C` | `FUN_14171E8E0` (`0x1A240`) |
  | `tech_MadColorGradingLutT1_Sn` | `0x69AC` | colour grading |

  - Bright-pass index: v = 0 none, 1 Vignette, 2 VignetteSimulate; mode 1 THRESHOLD,
    2 THRESHOLD_LUMINANCE, 3 THRESHOLD_LUMINANCE_REMAP.
- **Fallback:** when `table[r][s]` is 0, the leaves step to the nearest available row. Weights
  are rescaled by r/r′ in `FUN_14172A660`.

## 3. Selection rules per stage (STATIC)

| Stage | Code | Technique |
|---|---|---|
| downsample / resample | `FUN_14172A660` → `FUN_1416D6570` | `MadT1_S{nx·ny}`. Per axis: `n = ceil(0.5·src/dst)` (bilinear), minus 1 when `(n−1)·2·dst ≥ src − (2n−2)` and dst ≥ 8. A bias > 0 (`param_12`) forces nx = ny = 2 at ±bias/size. 2:1 → **S1**; 1280×720 → 784×294 → (1, 2) → **S2** |
| glare source | `FUN_14174BED0` | the `+0x220` chain of downsamples while narrower (mode 0), then the bright pass `FUN_14174B830` |
| bright pass | `FUN_14174B830` → `FUN_141736FE0` → `FUN_141722480` | mode = `[1448E6790][ctx+0xBBC]`, table `{1, 3}`. `SetGlareParameters(…, 1, RemapFactor)` → mode 3, stays 3 unless remap = 1. Vignette when `+0x160` is set. Taps 1. → `BrightPassT1_[Vignette_]THRESHOLD_LUMINANCE_REMAP_S1` |
| bloom blur (per level, two passes H then V) | `FUN_14174C160` → `FUN_141740D00` → `FUN_14173FDF0` | per-channel radii (§5), so the per-channel leaf `FUN_14171EAE0` → `MadT1_S{n}` (§5 gives n). Special implementations for k = 2/4/6 (`FUN_14173DB50`/`E6B0`/`F250`) |
| bloom upsample | `FUN_14174C160` tail, level record `+0x28`, `FUN_14172A660` with bias 0.375 | `MadT1_S4` |
| bloom combine | `FUN_14174C9E0` → `FUN_141735980` → `FUN_141720630` | `MadLumGammaT{levels}` with per-level RGBA weights (§5) |
| glare composite | `FUN_1417511C0` → `FUN_141720630` | `MadLumGammaT{k}`. k counts: the bloom result `+0x238`, light shafts `+0x250` and star `+0x258` (with their luminances), ghost `+0x260`, the current afterimage `+0x270[+0x20C]`, anamorphic `+0x2B0` |
| glare order | `FUN_1417538E0` | source, bloom, (light shafts), star `FUN_14174D000`, ghost `FUN_14174E390`, afterimage `FUN_141750000` (**not** light shafts, as YEBIS-NATIVE §3.1 said), anamorphic `FUN_141750F70`, composite |

## 4. The five witness zones (OBSERVED, `i63-live-10` frame 60 of stream 3; `evidence/I-0068/queries/yebis/techniques/yebis-draws-i63-live-10.tsv`)

The draws per frame are m31 48, m37 36, m45 36, m50 41 and m51 36. Every zone shares the first
ten draws (DoF). The glare part differs.

| # | Draw (sizes) | VS (stride) | Technique | Basis |
|---|---|---|---|---|
| 1 | depth factor: 1280×720 RGBA16 ← depth R24G8, final glare of the previous frame, DoF table 2048×2 R32F | `Tex2D1` (16) | `DepthOfField_BlurFactorAlphaFromDepthTextureTypeCastLutT1_S1_<channel>` (table `0x147F4`) | 3 textures, `Tex2D1`; channel INFERRED `RVALUE` from R24G8 |
| 2 | DoF source 1280×720 → 784×294 | `OffsetTex2D2` | **`MadT1_S2`** | taps (1, 2); its PS is the one `TexCoordMadT1_S2` uses later in m31/m50 (same blob, STATIC in the technique table) |
| 3 | 784×294 → 392×147 | `OffsetTex2D1` | **`MadT1_S1`** | same PS as the glare downsamples |
| 4 | DoF aperture: same-size pass ×3 (784, 784, 392) | `Tex2D1` | **`DepthOfField_FrontBlurBlendFactor`** (§8.1) | `FUN_14175C7E0` → `FUN_141727350`; `Tex2D1`, 1 texture |
| 5–6 | DoF blurs: one at 784 (PS A), two with PS B (784, 392) | two distinct VS | `MadT1_S{n}` per pass (INFERRED: same leaves as §3) | PS A equals the bloom blur PS in m37 |
| 7 | DoF composite 1280×720 ← scene + 784, 784, 392 layers | `Tex2D2` (24) | **`DepthOfField_CompositeBlurLevelsFrontBlur[_USNORM]_TS0_3`** (§8.2) | front-blur mode (`+0x140` = 2), 3 front layers |
| 8 | glare source 1280 → 640 (→ 320 when the base is 210×177) | `OffsetTex2D1` | `MadT1_S1` | §3 |
| 9 | bright pass → glare base (210×177, or 392×331 on m50) | `Tex2D1` on m37; `Tex2D2` on m31, m45, m50, m51 | `BrightPassT1_THRESHOLD_LUMINANCE_REMAP_S1` on m37; `BrightPassT1_Vignette[Simulate]_THRESHOLD_LUMINANCE_REMAP_S1` elsewhere | mode 3 (§3); the vignette variants are the only `Tex2D2` bright passes |
| 10 | bloom: L0 and L1 one blur pair each at base size; each further level a downsample `MadT1_S1` + one blur pair; levels of 53 px width or less also a ×2 upsample (98 and more: none) | blurs: one VS for both pairs on m31, m45, m50, m51 (the `OffsetTex2D8` class), two VS on m37 | blurs `MadT1_S{n}`, upsamples `MadT1_S4` | §3; a VS shared by two distinct blur PS means n ≥ 8 for both (INFERRED: `MadT1_S≥8` all use `OffsetTex2D8`) |
| 11 | bloom combine: one input per level (6 levels: base, base, /2, /4, /8, /16; m31 has 7 down to 7×6) → base | `Tex2D1` | **`MadLumGammaT6`** (m31: `MadLumGammaT7`) | `FUN_14174C9E0`; input count OBSERVED |
| 12 | m31, m50 only: light shafts: `Tex2D3` mask (base RGBA16 + 256×256 RGBA8), `TexCoordMadT1_S4` ×4 (radial), `TexCoordMadT1_S2` | `Tex2D3`, `Tex2D4`, `Tex2D2` | `LightShaftMask_SM3` (only `Tex2D3` 2-texture candidate that fits), then `TexCoordMadT1_S4`, `TexCoordMadT1_S2` | PS identity: the radial passes reuse the PS of `MadT1_S4`/`S2`, exactly the PS shared by `TexCoordMadT1_S4`/`S2` |
| 13 | glare composite → base | `Tex2D1` | `MadLumGammaT1` (m37, m45, m51); `MadLumGammaT2` (m31, m50: bloom + light shafts) | 1 or 2 inputs |
| 14 | final glare: base → 420×354 (or 640×540 on m50 and m40) | `OffsetTex2D4` | `MadT1_S4` | same PS and VS as the bloom upsamples |
| 15 | tone map → LDR | `Tex2D5` (48) | `TonemapHDR_[Dither_][Vignette_]Exposure_[Matrix_]Gamma` (TONEMAP-SKIN §15/§16) | 4 slots + `Tex2D5` |
| 16 | m31 (and m40): colour grading → R8G8B8A8 ← tone-mapped + LUT | `OffsetTex2D1` | `MadColorGradingLutT1_S1` | table `0x69AC`, 2 textures |

- On m31 and m50, an extra `OffsetTex2D1` pass (1280×720 RGBA16 ← depth) precedes the glare (closed in §8.4: `Alpha_<channel>`, the depth mask written into the scene alpha;
  it was given here as OPEN).
- **m40** (YEBIS-NATIVE §3.2, no programme ids) maps onto this structure:
  - 310–311 glare source (`MadT1_S1`, then the bright pass to 392×331);
  - 312–329 bloom (18 draws; m50 has the same 18 at the same base size);
  - **330 bloom combine (`MadLumGammaT6`)**;
  - 331–336 afterimage (`FUN_141750000`: `MadOffsetTn` family per its diagnostic);
  - **337 composite `MadLumGammaT2`** (bloom + afterimage);
  - 338 final glare resample `MadT1_S4` (17158; 640×540 in K-0410);
  - 339 tone map; 340 grading.

## 5. Bloom arithmetic (STATIC; `FUN_1417538E0`, `FUN_14174C160`, `FUN_141751DE0`, `FUN_14174C9E0`, `FUN_14173FDF0`)

Fields of `CRenderGlare` and where they come from:

- `+0x2E4` radius scale = `GlareBloomGaussianRadiusScale`, through `FUN_1416CB720` from
  `144962068` (2 on m40);
- `+0x2E0` sampling threshold = `GlareSamplingThreshold`, through `FUN_1416CB740` from
  `144962274` (0.01);
- `+0x2B8` levels;
- `+0xD4` levels kept at base resolution (INFERRED 2 from the observed absence of downsamples
  for L0 and L1);
- `+0x1B0` luminance gamma γ;
- `+0x118` glare luminance; `+0x150` the shape record (`+0x8` bloom luminance, `+0x10`
  tint amount);
- `+0xEE` alternate-constant flag.

The formulas:

```
radius base     R   = 2^(1 − min(D, N)) · C · s          C = 2.2 (1.8 when +0xEE), s = +0x2E4, D = +0xD4, N = levels
level radius    r_i = rec_i[+8] · R · √0.75 (levels ≥ D)
per channel     (r_i, r_i·540/615, r_i·465/615)            wavelengths 615/540/465 nm (constants 142B63170/68/6C)
taps            k   = first j ≥ 2 with g(j) < t, g(j) = exp(−j²/(2σ²))/(√(2π)·σ), σ = max channel radius, t = +0x2E0
                n   = 2·floor(k/2) + 1 (≤ 13); k = 2, 4, 6 use dedicated multi-pass blurs
pass weights    Gaussian per channel (FUN_1416DAA20 / FUN_1416DC220) × √w, applied in each of the two passes (total w)
luminance L     = 0.035 · Lglare(+0x118) · shape.bloom(+8) · shape(+0xC) · F(+0x114)
                  · (γ ≠ 1 ? N′ / (γ^(N′/2) + N′ − γ^(1/2)) : 1)          N′ = max(N − 2, 1)
ratio           q   = min(L^(1/N), 2)                                                  (+0x2DC)
level weight    w_i = L / q^i, tinted from level D−1+shape[+0x14] onward by the 3-colour cycle at 1448E8F70,
                  lerped by shape[+0x10], renormalised so that R + G + B = const (143D7EEB8) · w_i
combine         MadLumGammaT{N}: Σ_i w_i · level_i, then the luminance gamma +0x1B8
```

- The `√` split is visible in `FUN_14173FDF0` (`fVar19 = sqrt(param_6)` multiplies both
  passes).
- The `pow`/`exp` arguments lost by the decompiler were read from the assembly
  (`14174C211`, `14174C309..331`, `141751E81..E9A`, `14174CBDB`).
- **Native:** none of this is needed under the HAL route (the library still computes it). For
  a reimplementation, these are the inputs. The level record values `rec_i[+8]` come from
  `CRenderGlare::CreateSystemObjects`.

## 6. Retention note and corrections made in this pass

- `evidence/I-0068/queries/yebis/yebis-method-strings.tsv` had every VA 0x200 too low (file
  offset + 0x140001800). It is rewritten with +0x140001A00, and all 302 rows were checked
  against the executable.
- YEBIS-NATIVE corrected in place:
  - §3.1: `FUN_141750000` is `_Afterimage`;
  - §3.2: the composite and combine rows, and the m40 rows 330 and 337–338;
  - §4.1: the level sizes;
  - §5.3: slots 60–67;
  - §5.4: the vertex size is 16–48 bytes, set by the technique's VS.

## 7. Open (superseded by §8: items 1 and 3 closed, item 2 reduced to one numeric factor)

1. The DoF aperture pre-pass (row 4) and the DoF composite variant (row 7): `CDepthOfField`
   sub-functions not decoded.
2. Exact n of each blur, per zone: needs the zone's `GlareBloomGaussianRadiusScale` and the
   level records. The constraint set of §4 row 10 is OBSERVED.
3. The light-shaft depth pass on m31/m50.

## Exports

`decompiled/DS3-1152-F-<RVA>`: `017134E0`, `01716BF0`, `0172A660`, `016D6570`, `0174BED0`,
`0174B830`, `01736FE0`, `01722480`, `0174C160`, `01740D00`, `0173FDF0`, `0173CA40`,
`0174C9E0`, `01751DE0`, `017511C0`, `01720630`, `017538E0`, `01709AC0`, `01709720`,
`01709C70`, `01709900`, `01709E30`, `01709FE0`, `0170A380`, `0170A1D0`.
Queries: `evidence/I-0068/queries/yebis/techniques/`.

## 8. Closure of §7 (STATIC + OBSERVED; second request of 2026-10-10)

### 8.1 DoF pre-pass: `tech_DepthOfField_FrontBlurBlendFactor`

- **Call chain:** `FUN_14175EA40` (`ApplyDepthOfField_ApertureFilterAllLevels`) runs one
  `FUN_14175C7E0` per layer (record kind < 6; kind 7 → `FUN_14175D200`).
- In front-blur mode (`+0x140` bit 1) and for layers below `+0x1D8`, `FUN_14175C7E0` draws
  `FUN_141727350`, which reads the handles `0x194FC/0x19500/0x19504/0x19508`:
  - `FrontBlurBlendFactor`, `…Dilation`, `…Mask`, `…DilationMask`;
  - the mask variants are chosen when the layer has a mask texture (`FUN_14175C750`);
  - the dilation variants are chosen when `param_12` is set; it is 0 in this call.
- The observed draw (`Tex2D1` VS, 1 texture, on every layer) is
  **`tech_DepthOfField_FrontBlurBlendFactor`**.
- It is followed by the layer's blur (`FUN_14172A660` / `FUN_14173FDF0`).

### 8.2 DoF composite: `tech_DepthOfField_CompositeBlurLevelsFrontBlur[_USNORM]_TS0_3`

- `FUN_14175F850` switches on `+0x140`:
  - < 2 → `FUN_1417263B0` (`CompositeBlurredLevels[_USNORM]_T{n}`);
  - **2 → `FUN_1417266B0`** (`CompositeBlurLevelsFrontBlur[_USNORM]_TS{a}_{b}`, table `0x7D04`);
  - 6/14/22 → `FUN_1417269B0` (masked).
- In mode 2, a = `+0x1DC` and b = `+0x1D8` (front layers); the composite reads 1 + a + b textures.
- **Observed:** all three layers get the front-blur pre-pass, so b = 3. Four textures, so a = 0.
- The USNORM row (`+8` rows) is taken when `+0x154` is set: `FUN_141762930` sets
  `+0x154 = (+0x158 ≠ 0) || +0x1AC`.
  - `+0x158` comes from `DofEdgeQuarity`, which is empty in every witness zone; the default
    is −1.
  - The suffix is therefore not determined statically. Both rows have the same inputs.
- `+0x140` itself is set by the DoF init `FUN_141756870` (INFERRED from the observed structure:
  value 2).

### 8.3 Blur tap count n: the rule, the zone fields, what is still numeric

**The rule** (STATIC, §5 completed):

`
level record   k_i = i − D + 1   (D = +0xD4)
               size_i = base · 2^(−k_i) for k_i > 0, else base
               rec_i[+8] = 2^(k_i) · scale_i                               (FUN_141749790)
radius         σ_i = rec_i[+8] · 2^(1 − min(D, N)) · C · s · (√0.75 when i ≥ min(D, N))
               C = 2.2 (1.8 with +0xEE)
               s = gparam Bloom "Yebis-GlareBloomGaussianRadiusScale"
per channel    (σ_i, σ_i·540/615, σ_i·465/615)
table          g(j) = exp(−j² / (2σ²)) / (√(2π)·σ), σ = the red radius               (FUN_1416D6E80; 2π read at 143D7EEFC)
k              first j ≥ 2 with g(j) < t, t = gparam Bloom "Yebis-GlareSamplingThreshold"
n              2·floor(k/2) + 1 (bilinear-capable targets); k = 2/4/6 → multi-pass helpers
`

- **D = 2 is STATIC-consistent with the observed sizes:**
  - for i = 1, k ≤ 0 (both L0 and L1 at base);
  - for i = 2, k = 1 (base/2).
  - It also explains why L0 has its own blur programme while L1… share one: `rec[+8]` is 0.5
    for L0 and 1 from L1 on.

**Zone fields** (`evidence/I-0068/queries/yebis/zone-yebis-params.tsv`, regenerated by
`Get-ZoneYebisParams.ps1` from the zones' gparam; `*` = empty in the zone, default used):

| Zone | GlareBloomGaussianRadiusScale | GlareBloomLuminanceGamma | GlareNumLevels | GlareSamplingThreshold | GlareQuality | DepthOfFieldQuality | Aperture front/back levels |
|---|---|---|---|---|---|---|---|
| m31 | 2 | 1 | 8 (7 drawn) | 0.01 | 4* | 7* | 3*/3* |
| m37 | **0.5** | 1.4 | 6 | 0.01 | 4* | 7* | 3*/3* |
| m40 | 2 | 1.4 | 6 | 0.01 | 4 | 4 | 3*/3* |
| m45 | 2 | 1 | 6 | 0.01 | 4* | 7* | 3*/3* |
| m50 | 2 | 1.4 | 6 | 0.01 | 4 | 5 | 3*/3* |
| m51 | 2 | 1 | 6 | 0.01 | 4* | 7* | 3*/3* |

- **Observed agreement:**
  - only m37 has a radius scale below 2;
  - it is also the only zone whose bloom blurs use two non-`OffsetTex2D8` vertex shaders, i.e.
    n < 8 (§4 row 10);
  - in m31, m45, m50 and m51 both blur programmes share the `OffsetTex2D8` VS (n ≥ 8).
- **Not closed numerically:**
  - With the decoded constants, s = 2 gives σ = 1.1 / 2.2 / 1.9 (L0 / L1 / L2+), hence n = 3 / 7
    (k = 6, multi-pass) / 5. That does not reproduce n ≥ 8.
  - One factor of the chain is still missing; the likely place is the `2^(k)` exponent register
    of `rec[+8]` (`14174AAC5`, INFERRED = k).
  - The exact n per zone stays OPEN. The pilot can read it in one capture per zone through the
    blur programme's VS (`OffsetTex2Dn`) or the `` tap count.

### 8.4 Light-shaft passes (STATIC + OBSERVED; m31, m50)

1. **Depth → scene alpha** (`FUN_1416DF6E0`, the effect-mask alpha source, from
   `EndPostEffectScene_InternalProcess`) → `FUN_141727DA0`:
   - technique `tech_Alpha_{R,G,B,A}` (handles `0x431C..0x4328`, channel = `param_14`);
     `tech_MadAlphaColorT2_*` when a second source is given;
   - **input:** the scene depth (R24G8; channel INFERRED R);
   - **target:** the DoF output scene target (same RTV as the DoF composite, 1280×720 RGBA16).
     Its alpha becomes the shaft mask that the glare source chain carries down.
2. **`tech_LightShaftMask_SM3`** (`FUN_1417372A0` → `FUN_1417290B0`; handle `0x6F68`, falling
   back to `0x6F64` `LightShaftMask`):
   - t0 = the glare source surface (bright-pass output, base size RGBA16, alpha = mask);
   - **t1 = `m_pGPUTextureFilmGrain`** (`CTextureUtil+0x216E8`, 256×256 R8G8B8A8 noise; the
     `CreateTexture` at `141719C3C` logs that name);
   - constants at `+0xDC`;
   - **target:** the light-shaft surface (base size, R11G11B10).
3. **Radial blur:** `TexCoordMadT1_S4` ×4 (ping-pong between two base-size surfaces), then
   `TexCoordMadT1_S2`.
4. **Composite** `MadLumGammaT2` (bloom combine + shafts).

- The gparam LightShaft group's `Enable` (m31 0, others 1) does not match where the passes
  appear (m31 and m50 only).
- INFERRED: the composite's condition (`+0x248/+0x250` present, shape and luminance > 0, sun
  term) decides it, not that flag.