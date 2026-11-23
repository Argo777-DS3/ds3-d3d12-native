# I-0067 tranche 2: tone map chain and skin lighting (static)

Request (coordinator, for the I-0066 pilot, 2026-10-10): the native character image is
too flat (no tone map) and bare hands are darker than in the original. Static reading of
DarkSoulsIII.exe 1.15.2.0, same tools and conventions as [MENUS-MAP](MENUS-MAP.md).
Existing claims reused: K-0407 (GX pass order), K-0410/K-0412 (post chain issued by the
YEBIS 2 middleware, tone curve ≈ (2x)^(1/2.2) on one map), K-0433 (the owned host replays
that chain exactly from captures, with its temporal state).

## 1. Tone map and post-processing

### Where the parameters come from (STATIC)

Per scene, the GX scene render preparation `FUN_1400FB890` (called from
`FUN_1400FCE30`, reached from the `GXSg*SceneImpl` slot 4 draws) interpolates each
graphics-parameter bank between two entries, A and B, and applies the result to the
YEBIS settings unless the debug override `[ctx+0x2270]` is set:

```
A = bank(repo[+0x4618], scene+0x3FE0); B = bank(repo[+0x4618], scene+0x4048)  // FUN_1401638C0
A+0xDC = B+0xDC = Exposure (current value, see auto exposure)
t = lerp(A+8, B+8, scene+0x40AC)                                               // FUN_140218BA0
FUN_1400E42A0(t)       // ToneMap bank  -> YEBIS tone/exposure globals
glare bank (repo[+0x4620], scene+0x3FF0/0x4058, weight +0x40B4) -> FUN_1400E4460 (glare/bloom globals)
a third bank before it -> FUN_1400E4070 (writes 144962004)
```

These banks are the gparam ToneMap entries (`ToneMapA/B:Bank:GXParamId`,
`ToneMapA/B:ID:GXParamId` strings; INFERRED: A and B are the two gparam sets being blended
and the weight is the blend factor).

### ToneMap block layout to YEBIS globals (STATIC, `FUN_1400E42A0`; names from the YEBIS registration `FUN_1400E8210`)

| dword | global | name |
|---|---|---|
| 0 | `144962008` | Exposure (written only when AutoExposure is off) |
| 1 | `14496200C` | Gamma |
| 2 | `144962010` | (unnamed, next to Gamma) |
| 3 | `144962014` | ToneMappingFactor |
| 4 | `144962018` | FilmicContrast |
| 5, 6, 7 | `144962034/38/3C` | ColorH, ColorS, ColorV |
| 8 | `1449621B8` | Contrast ("Contrust") |
| 9 | `144961FC0` | AutoExposure enable |
| 10 | `144961FE4` | DelayFrame |
| 11 | `144961FC4` | MiddleGray |
| 12 | `144961FC8` | ToneInfluencedByGlare |
| 13, 14 | `144961FCC/FD0` | MinExposure, MaxExposure |
| 15–18 | `144961FD4..FE0` | Exposure metering width, height, offset X, offset Y |
| 19–21 | `144961FE8..FF0` | AdaptationSensitivity, AdaptationScale, AdaptationSpeedLimit |
| 22–24 | `144961FF4..FFC` | DarkAdaptationSensitivity, DarkAdaptationScale, DarkAdaptationSpeedLimit |
| 25 | `144962000` | fAdaptationScale / fDarkAdaptationScale |
| 26, 27 | `144962040/44` | Vignette power, VignetteFovDependence |
| 28, 29 | `14496201C/20` | (unnamed) |
| 30–33 | `144962024..30` | ToneLateralDispersion[0..1], ToneUniformDispersion[0..1] |
| 34 | `1449621C0` | (unnamed) |

The name-to-global table of `FUN_1400E8210` (131 entries: tone map, exposure,
adaptation, vignette, chromatic aberration, glare, bloom, lens, light shafts, DoF) is kept as
`evidence/I-0067/queries/yebis-vars.tsv`. The glare/bloom bank `FUN_1400E4460` fills
`144962048..1449622BC` (GlareThreshold `144962050`, GlareSamplingThreshold `144962274`, and
others).

### Auto exposure (STATIC)

With AutoExposure on, the exposure is computed by YEBIS on the GPU (metering window,
MiddleGray, Min/Max, light and dark adaptation speeds and scales above; the 640x540
adaptation target and the 2048x2 table read back next frame in K-0433). Each frame the YEBIS
filter `FUN_1400E5110` (reached from `GXBS::GXYebis2Filter` slot 0 `FUN_1400E4CE0`) calls
`FUN_1400E69A0`, which reads the adapted exposure back from the library
(`144962008 = FUN_1416CA160(yebis, 0)`), so the Exposure global always holds the current
value; `FUN_1400FB890` then copies it into both bank entries (`+0xDC`).

### The chain itself

`GXBS::GXYebis2Filter` (`143D4CC78`) drives the YEBIS library (`CPostEffect`,
`PPFX::CRenderGlare`, functions `1416Cxxxx`/`14192xxxx`). The library's own shader table
lists every technique by name, programme and index (for example
`tech_TonemapHDR_Dither_Exposure_Gamma[_LINEAR|_LOG|_REINHARD|...]`,
`tech_TemporalAntialias_Tonemap`, glare, DoF), so the shader bytecode is in the
executable. Parameters it consumes include `fParam_TonemapMaxMappingLuminance`,
`m44_PreTonemapColorTransformMatrix`, `m44_PreTonemapGlareColorTransformMatrix` (K-0412).
A second, GX-native tone map (`GXBS::GXToneMapFilter`, programmes `ToneMap_*PS.ppo`,
debug "GS Presents - ToneMap") also exists; K-0410/K-0433 show the YEBIS chain running on
m40, and which filter runs in other cases is not established.

### What the native backend needs (INFERRED)

The post chain is not engine logic: it is a fixed middleware pipeline whose inputs are the
scene HDR and the YEBIS globals above. The native backend can reuse the host-qualified
replay of K-0433 (same programmes and constants, with its temporal state) driven by these
globals, or reimplement the selected technique. The per-frame inputs are: the interpolated
ToneMap block, the glare/bloom block, the auto-exposure state carried between frames
(exposure feedback, the 640x540 adaptation target, the 2048x2 table), and the selected
tone-map technique (`ToneType`).

## 2. Why bare skin is lit differently from armour

### Skin is its own material family with a screen-space SSS stage (STATIC)

- Bare skin uses `GXFlver_SkinSSSIbl_Bloody` (the player's body meshes, I-0066 Native1094);
  the face uses `GXFlver_FgFaceSSSIbl_Bloody`; other SSS programmes are
  `GXFlver_SSSIbl_Bloody[_Cloth]` and `GXFlver_SSSEmiIblGlow_Bloody[_Cloth]`. Armour uses the
  non-SSS families.
- SSS pixels are tagged in the GBuffer (stencil class 12 "SSS skin", I-0066 `gx_replay.cpp`;
  material word `FC_SSS_ID`, Native1093) and lit through the "Irradiance(SSS)" path: after
  deferred lighting, the composite scene (`GXSgCompositeScene` slot 11, `FUN_140024740`) runs
  the SSS stage `FUN_1401141B0`. When enabled (`[this+0x44]` and `[this+0x45]`) it rebuilds,
  for every active profile of the `GXSssProfileManager` vector `[1449622F8]`, a 0x120-byte
  record (17-tap separable kernel from `FUN_1401130F0`, already reproduced natively in
  `light_producers.cpp`, plus the profile width scaled by a projection term computed from
  `[camera+0x50]`) and uploads it (`FUN_141920300`). The programmes are `GXSssBlurX`,
  `GXSssBlurY` and `GXSss`, loaded by `FUN_140114040` at GX rendering-system setup.
- Material side: `GXFlverMaterialParamSss` (`143D60A38`) carries `g_SSSStrength` and the
  switch `g_SSSStrength_UseCAParam` ("Use Ex Param - SSS Strength": the strength comes from
  the character's extra parameters); the material resolver `FUN_140D40790` looks up
  `g_SSSDiffuse` and `g_SSSPower`. Profiles have Strength R/G/B, Width and ProfileId
  (gparam "[SSS]").
- Skin colour: `SkinMatchColor` (the draw entity `+0x280` read as `FC_SkinMatchColor`,
  I-0066 Native1099) is a per-character input separate from the albedo texture.

### Native consequence (INFERRED, to verify on a frame)

Nothing in `native/renderer` runs the SSS blur stage (only the profile and kernel producers
and the material `FC_SSS_ID` word exist). Pixels of the SSS class therefore keep only the
part of their lighting that the albedo combine applies directly; the scattered diffuse that
`GXSssBlurX/Y` and `GXSss` add back is missing, which darkens exactly the bare-skin family
(hands, face) and not armour. Secondary candidates for the same symptom: `FC_SkinMatchColor`
still zero, `g_SSSStrength` from the character extra parameters not produced, or the
missing tone map lowering mid-tones overall (that affects armour too, so it does not explain
the difference alone).

Discriminating check for the pilot: on a reference frame, compare the SSS-class pixels
before and after the `GXSssBlurX/Y` and `GXSss` draws, which sit between
`GXLightCmbAlbedo` and `GXLightComposite` in the original pass list (K-0407).

## 3. The SSS stage, executable detail (STATIC)

### Objects

- SSS renderer object: `[[GXRenderingSystem + 0x4DE8] + 0x18]` (`GXRenderingSystem` =
  `[144961B50]`). Fields: `+0x00` PS `GXSssBlurX`, `+0x08` PS `GXSssBlurY`, `+0x10` VS
  `GXSss` (fetched with the shader repository's slot `+0x30`, the vertex-shader getter, while
  the two PS use slot `+0x20`), `+0x18` input layout (2 elements, layout descriptor
  `14473CA40`), `+0x20` constant buffer holding the profiles, `+0x30` depth-stencil state,
  `+0x38` CPU copy of the profile records, `+0x40`/`+0x42` profile counts, `+0x44` enabled,
  `+0x45` programmes loaded. Created at GX rendering-system setup by `FUN_140114040`
  (from `FUN_1400220A0` and `GXRenderingSystem` slot 1 `FUN_140023080`).
- Profiles: `GXSssProfileManager` vector `[1449622F8]` (begin/end at `+0`/`+8`, lock at
  `+0x20`).

### Per-frame constant update (`FUN_1401141B0`, from the composite scene `FUN_140024740`)

For each active profile (`profile+0x35` set; at most 0xE2 entries), in order, with its
index stored in `profile+0x30`:

```
record (0x120 bytes) in [obj+0x38] + i*0x120:
  +0x00 float  width_scaled = width * (1 / f([camera+0x50] * 0.5))   // width = profile+0x2C, or FUN_140112C60() when profile+0x38 != 0
  +0x10 float4 kernel[17]                                              // FUN_1401130F0, copied from *profile (0x110 bytes)
```

(`f` is `thunk_FUN_141FD59E0`, INFERRED `tanf`; `[camera+0x50]` INFERRED the vertical field
of view.) The whole array is uploaded to the constant buffer `[obj+0x20]` with
`FUN_141920300` (UpdateSubresource-style), only when at least one profile is active.

### Command recording and order

`FUN_140101700(list, scene)` records the stage when `scene+0x3EC6` and
`[[GXRenderingSystem+0x5360]+0x75]` are set. The view is `scene+0x7E8` when `scene+0x3ECC`
and `[scene+0x30] > 1`, else `scene+0x1E0`. It records three sorted commands (sort keys at
`list+0x120..+0x12C`):

1. key (0x17, 3): `FUN_1400BF730` → executor slot `+0x10` with the view's colour targets
   `[view+0x5D0]` and depth `[view+0x5E0]` (INFERRED: bind the lighting targets).
2. key (0x19, 7): the SSS draw callback `FUN_14010CBB0(ctx, scene)` →
   `FUN_140114640(sssObj, ctx, irr = [scene+0x798], irr, depth = [scene+0x2C0])`.
3. key (0x19, 0xFFFF): executor slot `+0x18` (INFERRED: end of the group).

`FUN_140101700` is called by the scene recorder `FUN_140103510` between
`FUN_1401094F0` and `FUN_1401018A0`; the final order comes from the sort keys. The live
capture of K-0407 places the SSS draws after `GXLightCmbAlbedo` and before
`GXLightComposite`.

### The two draws (`FUN_140114640`)

Skipped unless `DAT_144961BF7 == 0`, `obj+0x44`, `obj+0x45`, all inputs present and the
profile vector exists.

```
tmp = pool([GXRS+0x4DD8]).acquire({ w = irr.width, h = irr.height, format code 0xA, 1, 1 })   // FUN_140080510
save state (FUN_1400C5CB0/5D70/5E30)
OMSetBlendState([[144961AB0]+0x2A0]+8, ...)                                     // FUN_140015220
OMSetDepthStencilState(obj+0x30 (or [[144961AB0]+0x2A0]+0x1D8 when DAT_144961BF6), stencil ref 4)   // FUN_140005EA0
RSSetState([[144961AB0]+0x2A0]+0x208)                                           // FUN_140006040 (context vtable +0x158)
PS constant buffer b1 = [obj+0x20] (profiles); b8 = [[GXRS+0x4DD0]+0x2A8] (frame constants)   // FUN_140020250
-- draw 1: horizontal blur into tmp
OMSetRenderTargets(tmp RTV, depth view of [scene+0x2C0])                         // FUN_1400C5B00
ClearRenderTargetView(tmp, 0,0,0,0)                                              // FUN_141920200 (vtable +0x190)
PS t36 = irr SRV; IASetInputLayout(obj+0x18); PS = GXSssBlurX; VS = GXSss; GS/HS/DS = null
full-screen quad from ctx+0x460                                                  // FUN_14007F820
PS t36 = null
-- draw 2: vertical blur back into the irradiance target
OMSetRenderTargets(irr RTV, same depth)
PS t36 = tmp SRV ([tmp+0x48]); PS = GXSssBlurY; same depth-stencil state, ref 4
full-screen quad; PS t36 = null
restore state (FUN_1400C5E60/5DB0/5D10); release tmp (FUN_140080770)
```

So the stage reads and rewrites the diffuse irradiance target only where the stencil test
with reference 4 passes (the SSS class), using a cleared temporary of the same size.
Open: the stencil comparison and masks inside the depth-stencil state `obj+0x30`, the
blend state contents, and the exact meaning of format code 0xA (INFERRED
R16G16B16A16_FLOAT if the engine uses DXGI numbering); they are readable from the
created state objects or a capture.

## 4. SSS GPU states, decoded (STATIC)

GX state descriptors use D3D11 field order with enums shifted by one (verified on the
converters `FUN_14192AC30` comparison and `FUN_14192ACB0` stencil op: GX value = D3D11
value - 1, e.g. GX 2 = EQUAL, GX 7 = ALWAYS for comparisons; GX 0 = KEEP, GX 2 = REPLACE for
stencil ops). Depth-stencil descriptor layout (`FUN_14192F760`): `+0` DepthEnable,
`+4` DepthWriteMask, `+8` DepthFunc, `+0xC` StencilEnable, `+0xD` StencilReadMask,
`+0xE` StencilWriteMask, `+0x10..+0x1C` front Fail/DepthFail/Pass/Func, `+0x20..+0x2C` back.

The SSS object's two depth-stencil states are created by **`FUN_140114A50`**, starting
from the GX common depth-stencil state `[[144961AB0]+0x2A0]+0x1D8` (depth test off, depth
write off, depth func ALWAYS, stencil off, created by the GX common-state builder
`FUN_1400D21F0`):

| State | Use | Depth | Stencil |
|---|---|---|---|
| `obj+0x30` | the two SSS blur draws (stencil reference 4) | test off, write off | enable, read mask 0x04, write mask 0x00, front/back: Fail KEEP, DepthFail KEEP, Pass KEEP, Func **EQUAL** |
| `obj+0x28` | marking (not used by the blur draws) | test off, write off | enable, read mask 0x00, write mask 0x04, front/back: Fail KEEP, DepthFail KEEP, Pass **REPLACE**, Func ALWAYS |

So the blur passes run only where stencil bit 2 (value 4) is set, i.e. the SSS class.

Blend: `[[144961AB0]+0x2A0]+8` is the first GX common blend state (`FUN_1400D21F0`, loop
entry 0): AlphaToCoverage off, blending off (Src ONE, Dest ZERO, Op ADD for colour and
alpha), write mask RGBA. Both draws therefore overwrite their target where the stencil
passes.

Rasterizer: `[[144961AB0]+0x2A0]+0x208` is the first GX common rasterizer state (index 0 of
the block at `+0x208`): GX fill 0 and cull 0 (INFERRED SOLID and NONE, converters not
read), FrontCounterClockwise set, depth bias 0, depth clip on, scissor off, multisample
off, antialiased lines off. (Odd entries of the block enable scissor; entries 2/3 and 4/5
change the cull mode; the second group of 12, at `+0x268`, enables multisampling.)

Common-state table layout (`FUN_1400D21F0`, object `[[144961AB0]+0x2A0]`, 0x6D0 bytes):
blend states `+0x08..+0x1D0`, depth-stencil states `+0x1D8..+0x200` (6 entries),
rasterizer states `+0x208..+0x2C8` (2 groups of 12), samplers `+0x2D0..+0x340`.

## 5. YEBIS post chain: executable order and where its constants come from

Order and targets (OBSERVED, I-0054 cycle 1, one m40 frame, 51 post events): after the
scene HDR, `LutCopy2DTo3DCs` (16³ LUT), `PointSampleColorClamp1` (HDR → R16G16B16A16F
copy), 35 YEBIS draws (luminance, glare pyramid, eye adaptation into a 640x540 R11G11B10
target read back next frame, depth of field using depth) ending in the 8-bit LDR target
with the 3D LUT; `CustomDofLutCopy` (compute, writes the 2048x2 R32F table the next
frame's first YEBIS draw reads); then `PointSampleColor1`, `DLAA`, `EmissiveToGlowPS`,
four `Sample13Weight`, five `GlowCompositeSinglePS` (the last into the LDR target).
`evidence/I-0054/RESULT-NOTES.md` has the resource identities.

Engine side (STATIC): `GXYebis2Filter` slot 0 `FUN_1400E4CE0` → `FUN_1400E5110` prepares
the frame inputs (among them the `0x144961FBC` term and view data at
`[view+0x380/+0x388]`, `[view+0x450]`) and calls `FUN_1400E69A0`, which configures the
YEBIS object `[[144961AB0]+0xB80]` through about 80 calls of the library API
(`FUN_1416C9xxx`..`FUN_1416CBxxx`; the roles of the individual API functions are not
identified),
reads back the adapted exposure (`FUN_1416CA160` → `144962008`) and a second value
(`FUN_1416CA2E0` → `144962120`), and ends with `FUN_1416C9310(yebis, 0x10000000, …)` (INFERRED: the render call). INFERRED: the values
it passes come from the YEBIS globals of section 1 and from the view.

The library builds each draw's constant buffer itself (its shaders name
`fParam_TonemapMaxMappingLuminance`, `m44_PreTonemapColorTransformMatrix`, …). Recovering
that arithmetic statically means decompiling the middleware; the cheaper route for the
native backend is the host route already qualified in K-0433 (same programmes, constants
per draw from a capture), then identifying which constant-buffer fields follow which YEBIS
global (exposure, gamma, tone-mapping factor, glare) by varying them, since the shaders'
reflection names the fields.

## 6. YEBIS API calls of `FUN_1400E69A0` and the globals that feed them (STATIC + INFERRED)

Key facts (STATIC):

- The third argument of `FUN_1400E5110`/`FUN_1400E69A0` is the YEBIS globals block itself:
  `param_3 = 144961F70` (checked: `param_3[0x14]` = `144961FC0` AutoExposure,
  `param_3[0x1B/0x1C]` = metering offset X/Y, `param_3[0x20..0x24]` = the adaptation
  parameters). The block is 0x370 bytes, `144961F70..1449622DF`, and can also be loaded
  from a settings file by `FUN_1400E39B0`.
- All the API functions (`FUN_1416C9xxx..FUN_1416CBxxx`) write into the YEBIS context
  `[1448E5E38]` (`CPostEffect`); eleven are named by their own diagnostics
  (`..\src\Gpu\GPUPostEffect.cpp`, "PPF", Oct 14 2014): `InitializeContinuousEffects`
  (`9270`), `InitializeAfterimage` (`92B0`), `BeginPostEffectScene` (`9310`, the last call),
  `SetRenderSceneLuminanceScale` (`9430`), `SetGlareParameters` (`9670`),
  `SetFeedbackEnable` (`A3B0`), `SetColorGradingLutEnable` (`A570`),
  `SetChromaticAberrationEnable` (`A870`), `SetAntialiasEnable` (`A940`),
  `SetMotionBlurEnable` (`AA20`), `SetDepthOfFieldEdgeQuality` (`BFE0`).
- **The two read-backs**: `FUN_1416CA160` returns the adapted exposure
  (`[[1448E5E38]+0x1F0] + i*0x68 + 0x54`) → `144962008` "Exposure"; `FUN_1416CA2E0` returns
  `[[1448E5E38]+0x1F8] + i*0x48 + 0x34` → `144962120` **"DofFocusDistance"** (the
  depth-of-field focus distance computed by the library).

Global → call mapping (INFERRED: globals read just before each call; float arguments passed
in registers are not always visible in the decompilation). Full list, 80 calls:
`evidence/I-0067/queries/yebis-api-calls.txt`.

| Call | Globals feeding it | Likely role |
|---|---|---|
| `FUN_1416C9450`, `FUN_1416C9490` | camera term (tan of half angle) | projection / field of view |
| `FUN_1416C95B0`, `FUN_1416C95F0` | current view matrix (`[view+0x210]`) and previous one | motion vectors / temporal |
| `FUN_1416CB200` | depth texture and its size | depth input |
| `FUN_1416CA110` | `144961FC0` AutoExposure | auto exposure on/off |
| `FUN_1416CA230` | `144961FDC/FE0` metering offsets (and width/height) | metering window |
| `FUN_1416CA210` | `144961FF0..144962000` adaptation speed limits, dark adaptation sensitivity/scale, scale ratio | eye adaptation |
| `FUN_1416CA1D0`, `FUN_1416CA1B0` | (MiddleGray, Min/Max exposure; INFERRED from the argument checks `0 < x < 1` and `min <= max`) | exposure key and range |
| `FUN_1416CA450` | `144962014` ToneMappingFactor (INFERRED: also Gamma and FilmicContrast through register arguments) | tone curve |
| `FUN_1416CA8B0` | `144962020..28` lateral dispersion | chromatic aberration |
| `FUN_1416CBB90/BBF0/BC30/BC90` | `144962034/38/3C` ColorH/S/V, `1449621B8` Contrast, `1449621C0` | colour adjustment (each only when the value differs from neutral) |
| `FUN_1416C9670` SetGlareParameters | `1449621B4` RemapFactor | glare |
| `FUN_1416C96B0` | `144962278..1449622BC` custom bloom (glare/bloom luminance and dispersion, ghost, star, afterimage) | custom glare shape |
| `FUN_1416C97F0`, `FUN_1416C9830` | `1449620B8..CC` light-shaft ratio/attenuation/noise, `1449620E0/E4` diffraction ring | light shafts |
| `FUN_1416CB4A0`, `FUN_1416C98D0`, `FUN_1416CBFE0` | `14496211C..144`, DofAdaptiveAperture factor/base FOV | depth of field |
| `FUN_1416CAA20`, `AAA0`, `AA60`, `AB40`, `AB90`, `AAE0` | `14496214C..1449622C0` | motion blur |
| `FUN_1416CA5D0`, `FUN_1416CA570` | `144962228` | colour-grading LUT |
| `FUN_1416CA3B0` | `144962178` | feedback (afterimage) |
| `FUN_1416CA940` | `144962190` | anti-aliasing |

For the native tone map this means: exposure, gamma, tone-mapping factor, filmic contrast
and the adaptation set (section 1 table) reach YEBIS through `FUN_1416CA110`,
`FUN_1416CA1B0/1D0`, `FUN_1416CA210`, `FUN_1416CA230` and `FUN_1416CA450`, and the library
returns the adapted exposure each frame.

## 7. SSS stage activation conditions (STATIC)

For the two SSS draws to run, **all** of the following must hold
(`GXRS` = `[144961B50]`, `obj` = `[[GXRS+0x4DE8]+0x18]`):

| # | Condition | Who sets it, when |
|---|---|---|
| 1 | `obj` exists | `FUN_1400FF630` (light-manager setup, from GX rendering-system init `FUN_1400220A0`, itself from `FUN_1400B5E00` ← `SprjGraphicsImp` `FUN_140D573B0`) allocates 0x48 zeroed bytes and stores them at `[[GXRS+0x4DE8]+0x18]` |
| 2 | `obj+0x44 = 1` | `FUN_140114A50` (called right after the allocation): creates the two depth-stencil states, the 0xFF60-byte constant buffer `+0x20` (0xE3 profiles x 0x120) and its CPU copy `+0x38`, creates the profile manager if absent (`FUN_140113F80`), then sets `+0x44` |
| 3 | `obj+0x45 = 1` | `FUN_140114040(obj, shaderRepository)` at the end, **only if** the shader repository returns the pixel shaders `GXSssBlurX` and `GXSssBlurY` (repository slot `+0x20`) and the vertex shader `GXSss` (slot `+0x30`); it also creates the input layout `+0x18`. Called from `FUN_1400220A0` (after `FUN_1401457B0` with the same repository) and from `GXRenderingSystem` slot 1 `FUN_140023080` (shader reload) |
| 4 | profile manager `[1449622F8]` non-null, with active profiles | `FUN_140113F80` creates it (reserves 0xE3 entries) and adds the **default profile 0** (`FUN_140113D80(mgr, 0)`: strengths R/G/B 0.48/0.41/0.28, width 0.012, `+0x34 = +0x35 = 1`, so active); more profiles come from SSS materials (`FUN_140113B10`, called from `GXFlverMaterialParamSss` slot 20 `FUN_140201CA0` and from `FUN_1402306C0`) |
| 5 | `DAT_144961BF7 == 0` | debug switch (registered by the SSS debug page `FUN_140114C80`); 0 by default |
| 6 | the command is recorded: `scene+0x3EC6 != 0` | set to 1 by the scene-context setup `FUN_140085540` (debug label "Enable SSS", `FUN_14008D170`) |
| 7 | the command is recorded: `[[GXRS+0x5360]+0x75] != 0` | the current quality-settings object `[GXRS+0x5360]` (0x78 bytes, created in `FUN_1400220A0` at `14002229C`) gets its word `+0x75` from the quality table `[GXRS+0x5358]`: entry `(level + 0xBF) * 3` with `level = [GXRS+0x534C]` (the twelfth quality setting, INFERRED shader/lighting quality); also refreshed by `FUN_140023790` when settings change |
| 8 | irradiance `[scene+0x798]` and depth `[scene+0x2C0]` non-null | scene targets |

Per-frame constants (section 3) are rebuilt only when conditions 2 and 3 hold.

Discriminating reads for the native run (read-only): `[[[144961B50]+0x4DE8]+0x18]` and its
bytes `+0x44`, `+0x45`; `[1449622F8]` and its vector size; `[[144961B50]+0x5360]+0x75`
and `[[144961B50]+0x534C]`; `scene+0x3EC6`; `DAT_144961BF7`. A zero `+0x45` points to the
shader repository (the three SSS programmes not found); a zero `+0x75` points to the
quality settings; a null `obj` means the GX light-manager setup did not run.

## 8. How SSS profiles are built and how a material picks its profile (STATIC + open)

### Profile object (0x40 bytes, `GXSssProfileManager` vector `[1449622F8]`)

| Offset | Field |
|---|---|
| `+0x20/+0x24/+0x28` | strength R, G, B |
| `+0x2C` | width (radius); for a linked profile the width of the end of the link chain (`FUN_140112C60`) |
| `+0x30` | index: vector position at creation (`FUN_140113D80` stores the vector size), then the cbSSS slot assigned this frame (`FUN_1401141B0` writes the slot counter) |
| `+0x34` | set at creation and when linked (role not read) |
| `+0x35` | active (set at creation, cleared when its material parameter is destroyed, `GXFlverMaterialParamSss` slot 0) |
| `+0x36` | used this frame (read by `FUN_1401141B0`) |
| `+0x38` | link to a base profile (0 for a base) |

The only profile constructor path is **`FUN_140113D80(manager, link)`** (profile ctor
`FUN_140115540`): defaults strength 0.48/0.41/0.28 (`0x3EF5C28F/0x3ED1EB85/0x3E8F5C29`),
width 0.012 (`0x3C449BA6`), `+0x34 = +0x35 = 1`, `+0x38 = link`, appended to the vector,
`+0x30` = its position. Callers: the manager setup (`FUN_140113F80`, default profile 0, no
link) and the FLVER material path below. (`FUN_140113B10` is only the debug page of a
profile: Width, Strength R/G/B, ProfileId.)

### Per material (FLVER material load, `GXFlverMaterial` slot 11 `FUN_140141070`)

The FLVER material item loop dispatches on GX item tags (compared as four-character codes, for example `GX00`, `GX02`, `GX20`, `GX24`, `GX90`). For SSS materials:

- the material's MTD parameter list (`[mtd+0x90..+0x98]`) is searched for the parameter named
  **`g_SSSWidth`**; this parameter object is a `GXFlverMaterialParamSss`
  (vtable `143D60A38`): `+0x10` = its **base profile** (one per MTD), `+0x18` = "use the
  material's own strength" switch (`g_SSSStrength_UseCAParam`, returned by slot 6
  `FUN_140201990`), `+0x19` = shown in the debug page;
- with per-material SSS data in the FLVER material (the GX item handled by
  **`FUN_14022BF70`**): a **linked instance profile** `FUN_140113D80(manager, base)` gets the
  strength from that item's floats `+0x0C/+0x10/+0x14` and the base's width; the material
  stores it at `+0x368`;
- without it (**`FUN_14022C0D0`**): `+0x368 = 0` (the material uses the base);
- in both cases `material+0x370` = the `g_SSSWidth` parameter, `material+0x21 = 1` (SSS
  material) and `material+0x418 = max(material+0x418, 0x19)`.

### The material's SSS id (shader word `FC_SSS_ID`, `FUN_14022E930` at `14023002A`)

When `material+0x21` is set: `profile = (param->slot6() /* +0x18 */ && material+0x368) ?
material+0x368 : param+0x10`; `FC_SSS_ID = { float(profile+0x30), 0, 0, 0 }` at constant
offset `+0x190`. So the shader indexes the cbSSS array by the profile's `+0x30`, which
`FUN_1401141B0` rewrites to the profile's slot in this frame's upload.

### The kernel

`FUN_1401130F0(profile)` builds the 17 taps from the profile's strength (`+0x20..+0x28`)
with the five-Gaussian skin profile `FUN_140112E60`/`FUN_140112D80` (falloff colour at
`144745FF0`); the native `SssKernel`/`SssProfile` in `native/world/light_producers.cpp`
already reproduce it, and the slot width is `width * 1/tan(fov/2)` (section 3).

### Open (encrypted)

How the base profile of a `GXFlverMaterialParamSss` gets its values from the MTD is in code
encrypted at rest right after its slot 0 (`1402019E0..140201C1F`; the class's slots 3..19
are generic parameter slots). I-0045 game-data comparisons (native `Banks::SssBase`) show
that base profile k takes the MTD's `g_SSSStrength` and `g_SSSWidth`, in material-bank
order, after the default profile 0; reading the loaded bytes of RVAs `0x2019E0`..`0x201C20`
would confirm the conversion.

## 9. `FC_SkinMatchColor` and the MTD parameter names (STATIC)

- The only writer of `FC_SkinMatchColor` is the debug page `FUN_14004E3C0`; the pilot measured
  it at zero in the original game too, so it does not explain the darker hands.
- The MTD parameter names that the FLVER material reads are registered by `FUN_140D40790`
  (74 names, including `g_SSSDiffuse`, `g_SSSPower`, `g_IsUnifyTOD_Skin`, `g_DiffuseMapColor`
  and `g_DiffuseMapColorPower`). The list is in `evidence/I-0067/queries/mtd-params.txt`.

## 10. Per-instance SH (`aSh`, cbInstanceData `+0x80..+0xAF`): where it comes from (STATIC)

`FUN_1401AC840` builds `aSh` from the anim block `p = anim+0xF0`. The 0x30 bytes it uses are
`q+0x130..+0x15F` with `q = p - 0xF0` (three float4 rows, one per colour channel: `.xyz` the
L1 vector, `.w` the L0 term), plus `q+0x16C`. **All of this is CPU work.** There are two
sources: a baked map probe, or the ambient hemisphere of the light set.

### Writer: `FUN_140061950(q, record, smooth)`

- It stores `q+0x178 = record` (that is `p+0x88`) and copies the record's first 0x1C bytes to
  `q+0x180` (`p+0x90`): 13 SH shorts plus the short at `+0x1A`.
- **The whole SH update is gated by `[GXRS+0x4DD0] != 0`**, the same gate as in `FUN_1401AC840`.
  If that pointer is null natively, `q+0x130..` never changes and `aSh` stays zero, whatever
  the record.
- With a record, the shorts `s0..s12` become the rows with SH basis constants: `k1 = 0x39FA2A2C`
  (0.488603/1024) and `k0 = 0x39906EC1` (0.282095/1024), from `.rdata 143D7F940`.
  - Row R = `(s9 k1, s3 k1, s6 k1, s0 k0)`.
  - Row G = `(s10 k1, s4 k1, s7 k1, s1 k0)`.
  - Row B = `(s11 k1, s5 k1, s8 k1, s2 k0)`.
  - The target of `q+0x16C` is `s12/1024`.
  - The file order is therefore L0 RGB, then L1 y, z, x per RGB.
- Without a record:
  - if `q+0x161` is set, the rows are copied from the light-set record `FUN_140032A80([GXRS+0x41E0], q+0x1A0) + 0x290..+0x2BF`;
  - otherwise the rows are zero;
  - in both cases the target of `q+0x16C` is 1.0.
- On first use (`q+0x160 == 0`), `q+0x130..` is initialised from the same light-set rows, or
  zero.
- Each call blends `q+0x130..+0x15F` and `q+0x16C` toward the target. The factor comes from
  the fade clock `q+0x164/+0x168` (step `.rdata 143D7F040` = 1000.0). `smooth = 0` resets
  the clock.
- Callers: `FUN_140D3FFB0`, `FUN_140D3E240` (on a position change or the RendManImp flags
  `[+0x10]+0x570/+0x571`), `FUN_140D3E1F0`, `FUN_140D3EC50` and `FUN_140D40010`.

### Which probe: `FUN_14001D650(G+0x46E8, pos, bbox, out)`, CPU only

- The manager `G+0x46E8` is built by `FUN_14001D160` in G's constructor. It holds one probe
  volume per map.
- The query first keeps the volumes whose AABB (`+0x90..+0xAC`) overlaps the object box, and
  sorts them by the distance from the clamped point.
- For each volume, `FUN_1400F47D0` walks the volume's grids (AABB `+0x60..+0x7C`, cell size
  `+0x80..+0x88`, cell counts `ushort +0x90..+0x94`). It searches the 27 cells around the
  object (offset table `14473D404`, cell lookup `FUN_1400F3690`).
- In each cell it keeps the probe with the **lowest short at `+0x1A`**, then the nearest by
  position (`float3 +0x1C`).
- Across volumes, the record with the lowest `+0x1A` wins, then the nearest.
- There is no device and no GPU access on this path. The pilot's question was whether the
  map probes are sampled: yes, but as a nearest-probe pick, with no interpolation between
  probes. The only smoothing is the time blend.

### Where the probes come from (STATIC, plus one INFERRED link)

- **Registration:** `FUN_140651A60` and `FUN_140653170` call
  `FUN_14001D2D0(G+0x46E8, map id, data = [[map+8]+0x40]+0x90, map transform [[map+8]+0x28]+0x5C0, 8)`.
  `FUN_140652070` and `FUN_140651A60` unregister through `FUN_14001D450`.
- **Volume build:**
  - When the file header version exceeds 2, `FUN_14001D2D0` builds a 0xD0-byte volume:
    `FUN_1400F3960`, then `FUN_1400F3AA0(volume, data, transform)`.
  - `FUN_140003FB0(data, transform)` runs once per file. It relocates the group offsets
    (relative to header `+0x3C`; group stride 0x48 before version 3, 0x98 from version 3).
  - It also **rotates every probe's nine L1 shorts (`+6..+0x17`) by the map rotation, in
    place**.
- **File format (INFERRED):** the data are the map's baked probe files `.btpb`
  (`map:/%s/%s_%04d.btpb`, `map_dlc1:`, `map_dlc2:`; the RTTI class `NS_SPRJ::BtpbFileCap`).
  The link from `[map+0x40]+0x90` to `BtpbFileCap` is not proved statically.

### The light-set rows `+0x290..+0x2BF`: `FUN_1401495F0(record, record+0x290)`, CPU only

The bank `[GXRS+0x41E0]` is the gparam light-set map: a two-level hash on (area, id), with
fallback keys {1, 0}, then the default record at `map+0x700`. `FUN_140030D80` creates one
0x350-byte record per (area, id):

1. `FUN_140149460` constructs the record.
2. `FUN_14014B3A0` sets its defaults.
3. The record is stamped with its key at `+0x2E0/+0x2E4`, and `+0x2D0` receives the
   rotation of the file's own transform (`FUN_1400362B0`), identity for every shipped file
   (correction in MENUS-MAP section 45).
4. `FUN_14014BA90` derives the four light directions and then calls `FUN_1401495F0`.

`FUN_1401495F0` writes the rows from the hemisphere colours:

```
U = rgb(+0x220)^2 * (+0x22C)      upper hemisphere colour, squared, times intensity
D = rgb(+0x230)^2 * (+0x23C)      lower hemisphere
a = (|xyz(+0x260)|, 1)            axis; default .rdata 1439F24E0 = (0, 1, 0)
row_c = ((U_c - D_c)/2 * a.x, (U_c - D_c)/2 * a.y, (U_c - D_c)/2 * a.z, (U_c + D_c)/2)
```

- In the pilot's `Banks::Sun` frame (`r = record+0x40`), `+0x220` is `r+0x1E0` (LightSet
  parameter 20), `+0x230` is `r+0x1F0` (parameter 21) and `+0x260` is `r+0x220` (`.rdata 39F24E0`).
- Other callers of `FUN_1401495F0` recompute the rows whenever a record is edited or blended:
  `FUN_140264230`, `FUN_1402A4EB0`, `FUN_1402AE5A0`, `FUN_140147ED0`, `FUN_140148770`,
  `FUN_140148F50` and `FUN_14014B3A0`.
- No GX manager is involved: the rows depend only on the gparam bank.

### Native consequence (INFERRED)

`aSh` is reproducible natively without any GPU stage:

1. Check `[GXRS+0x4DD0]` in the native run.
2. Rebuild the light-set rows with the formula above from the banks the pilot already builds.
3. Load the map's `.btpb` probes, rotated by the map yaw, and apply the nearest-probe pick and
   the time blend.

## 11. GXEnvMapRegion: where each region's IBL cube comes from (STATIC)

The pilot already implements the region pass: selection `FUN_140067070`, constant block
`CB_ENVMAP_REGION` and executor `FUN_1400675B0`. This section answers only one question:
where the cube comes from. In `675B0`, the cube bound at t30 is
`FUN_1400416A0([GXRS+0x42A8], record)+0x40`, together with the scale `+0xC`.

### The bank `[GXRS+0x42A8]`: gparam EnvMap entries

- `FUN_140CC8180` applies the gparam banks of the current map:
  - `+0x41E0` light sets;
  - `+0x41E8` fog;
  - `+0x41F0`;
  - `+0x42A8` EnvMap (`GXEnvMapParamManager`, GXBS);
  - `+0x4600`.
- `FUN_14003FEB0` creates one 0xA8-byte entry per gparam id of the EnvMap group. It sets
  `+0x8 = 0`, `+0xC = 1.0`, and `+0x10` = the prefix. The prefix is `"EnvMap_"` by default
  (`.rdata 143D42BD8`), otherwise the gparam string (debug label `EnvMap:Prefix`), or that
  string followed by `"GILM"` when `FUN_140095220` is true. The function also sets
  `+0x40 = 0` (released) and `+0x90/+0x94` = key.
- `FUN_140040DD0` handles gparam blends: it builds the entry as a copy or blend of two entries
  (`+0xC` blended linearly; `+0x8`, `+0x10` and `+0x40` taken from the dominant one).

### Name to cube: `FUN_14006CED0(entry, _, repo)`

```
if entry+0x98 != entry+0x8: entry+0x98 = entry+0x8; release entry+0x40
if entry+0x40 == 0:
    name = swprintf(L"%s%04d", entry+0x10, entry+0x8)        // e.g. "EnvMap_0304"
    entry+0x40 = repo->Get(name)  // vtable slot 1, refcount++
```

- `FUN_1400499C0`, called from `FUN_140023410`, runs this every frame:
  - on the default entry (bank `+0x1A8`);
  - then on each entry, with `repo = [bank+0x250]`.
- If `repo` is null, or does not know the name, `+0x40` stays null.
- **The repository is `NS_SPRJ::SprjGxResourceRepository_forEnvmap`** (vtables
  `1429551C0`/`1429551D8`, constructor `FUN_140D49D00`).
  - The `SprjGraphicsImp` constructor `FUN_140D55DB0` creates it as its fourth repository
    (`this+0x20`). It creates six repositories in all (`+0x08..+0x30`; the base class
    `SprjGxResourceRepository` is built by `FUN_140D49570`).
  - Its slot 1 (`140D49DB0`, a jump to `FUN_140D49B10`) is `Get(name)`. It looks the name
    up in the texture resource manager `[144792F58]` (`FUN_140D4ED20`) and returns
    `resource+0x60`, the GX texture. When `this+0x10` is set, the name is replaced by
    `SYSTEX_DummyLightmap`.
  - Which code stores the repository at `bank+0x250` is still unknown.
    - The last call of the `SprjGraphicsImp` constructor, `FUN_140D57EB0`, was a candidate.
      Its loaded bytes (Native1160) show only debug GUI registration (MENUS-MAP section 12).
    - The bank constructor `FUN_14003F720` writes 0 there.
    - No other direct store exists.

### Fallback when `+0x40` is null (`675B0`)

1. The first fallback is `[[GXRS+0x4DD0]+0x178]`, the global REM cube.
2. If that is also null, the GX common texture `[[144961AB0]+0x270]+0x70` is used.

The global cubes are set by name through the same repository:

- `FUN_140041B30(bank, name)` sets `+0x178`, the global REM cube.
- `FUN_140041B50(bank, name)` sets `+0x180`, the global IEM cube. When that is a cube,
  `FUN_1400D9EE0` derives `[GXRS+0x4DD0]+0x190..+0x1E8`, the `render190` block that
  `cbInstanceData` reads. Without it, those rows take the constant `.rdata 1439F2500`.

`SprjFlverDrawSystem` passes the names:

- **Slot 17**, `FUN_140D1EB80(texIem, texRem, texC, flag)`, takes texture resources:
  - texIem goes to `41B50`;
  - texRem goes to `41B30`;
  - texC goes to the encrypted `41B70`.
- **Slot 18**, `FUN_140D1E7E0`, passes `ModelViewer_Default_iem` and `ModelViewer_Default_rem`,
  each looked up with `FUN_140D4ED20([144792F58], name)`; texture resource `+0x60` is the GX
  texture.
- **Slots 19/20** set `snow_a/n/r/s` and `dust_a/n/r/s` into `GXRS+0x4800..+0x4818`.

`FUN_140078680`, an alternative path, releases every entry's `+0x40` (`FUN_140041610`). It
then resets the globals through `FUN_1400742B0` (suffix `"_rem"`) and the encrypted
`FUN_140075CB0` with the repository `[bank+0x250]`.

## 12. The name-to-texture repositories of `SprjGraphicsImp` (STATIC, one INFERRED link)

The `SprjGraphicsImp` constructor `FUN_140D55DB0` creates six repositories that turn a texture
name into a GX texture. They all share one mechanism, and the env map one is only one member
of the family.

| `SprjGraphicsImp` field | Class (vtable) | Constructor |
|---|---|---|
| `+0x08` | `SprjGxResourceRepository` (`142955030`) | `FUN_140D49570` |
| `+0x10` | `SprjGxResourceRepository_forPostProcess` (`1429553D0`) | `FUN_140D49F40` |
| `+0x18` | `SprjGxResourceRepository_forDecal` (`142955110`) | `FUN_140D49C50` |
| `+0x20` | `SprjGxResourceRepository_forEnvmap` (`1429551C0`) | `FUN_140D49D00` |
| `+0x28` | `SprjGxResourceRepository_forLightmap` (`142955320`) | `FUN_140D49E80` |
| `+0x30` | `SprjGxResourceRepository_forGparam` (`142955270`) | `FUN_140D49DC0` |

- **Shared interface:** the primary interface is `GXBS::IGXTextureRepository`. In every class,
  its slot 1 `Get(name)` is `FUN_140D49B10`, either directly or through a one-jump stub:
  `resource = FUN_140D4ED20([144792F58], name)` and `return resource+0x60`.
  - `[144792F58]` is the texture resource manager.
  - When `this+0x10` is set, the name is replaced by `SYSTEX_DummyLightmap`.
- **Secondary interface:** the subobject at `+8` is `GXBS::IGXResourceRepository`. Its slots
  2..7 forward to two other managers, `[144792748]` (`FUN_140D45020`) and `[144792838]`
  (`FUN_140D45970`), with a name built by `FUN_140D48760`. Its slot 1 returns null.
- **Registration:** the place that hands the repositories to their GX consumers is unknown.
  The constructor's last call `FUN_140D57EB0` only registers debug GUI widgets (loaded
  bytes, MENUS-MAP section 12). The only consumer visible statically is the env map one,
  `[GXRS+0x42A8]+0x250` (section 11).

### Native consequence (INFERRED)

Any GX system that resolves a texture by name through one of these repositories gets null
natively if the repositories are not created and registered. The six systems at risk are
post-process, decals, env maps, lightmaps, gparam textures and the base repository. Because
`Get` reduces to a name lookup in the texture resource manager, the native backend can supply
the same binding for all six at once.

## 15. The YEBIS shaders themselves: technique table, bytecode, reflection and tone-map formulas (STATIC)

### The embedded shader set

- The executable holds 3534 DXBC blobs. Two read-only tools list and disassemble them:
  - `Get-DxbcReflection.ps1` gives the offset, SHA-256, used constants and bindings of each
    blob;
  - `Get-DxbcDisassembly.ps1` disassembles a blob with the system `d3dcompiler_47`.
- **YEBIS shaders** are the 2551 consecutive blobs from file offset `0x2CC3B48`: 37 vertex
  shaders and the rest pixel shaders.
- **Technique table:** 2555 text entries `tech_<name>,vs_<name>,<i>,ps_<name>,<j>` (from file
  offset `0x2BD9D43`). `i` and `j` are indices into that blob list; all 2555 pairs check out
  (VS at `i`, PS at `j`).
- **The full mapping** is in `evidence/I-0067/queries/yebis-techniques.tsv`: technique, VS and
  PS names, indices, PS file offset, both SHA-256 and the used `$Globals` fields.
  - The pilot can identify any captured YEBIS draw by hashing its bound bytecode.
- **Families (counts):**
  - DepthOfField 832, TonemapHDR 707, Mad* blur and downsample 400+, BrightPass 54;
  - Bokeh*, MotionBlur*, TemporalAntialias (including `_Tonemap`), AlchemySsao,
    LensDistortion, LateralChromaticAberration, GlareGhost, MadColorGradingLutT1;
  - AnalyzeIntensity, MadLumGamma, MaxColorLumGamma.
- **One constant buffer:** every YEBIS shader uses a single `$Globals` buffer (4096 bytes, b0)
  with a fixed layout. Notable fields:

| `cb0` offset | Field |
|---|---|
| 32/48/64 | `fParam_HDRFormatFactor_LOGRGB`/`RGBALUM`/`REINHARDRGB` |
| 2464 (c154..) | `afRGBA_Modulate[32]` |
| 2976 (c186..) | `afRGBA_Offset[16]` |
| 3232 (c202.x) | `fParam_GammaCorrection` (default 0.4545) |
| 3236 (c202.yz) | `fParam_DitherOffsetScale` |
| 3328 (c208) | `fParam_TonemapMaxMappingLuminance` |
| 3344 | `fParam_BrightPassRemapFactor` |
| 3360 (c210..212) | `m44_ColorTransformMatrix` |
| 3424 | `m44_PreTonemapColorTransformMatrix` |
| 3488 | `m44_PreTonemapGlareColorTransformMatrix` |
| 3552 | `fParam_VignetteSimulate` |
| 3568 | `fParam_VignettePowerOfCosine` |
| 3584 | `am44_TransformMatrix[8]` |

The shaders are generic: exposure, curve and colour arrive as `afRGBA_Modulate`/`Offset`
values and matrices. The library computes them on the CPU from the API calls of section 6;
that CPU arithmetic is not decompiled.

### Auto exposure (programmes)

- `MadLumGammaT1..16` and `MaxColorLumGammaT*` reduce the HDR image, as a weighted sum or a
  maximum over N taps. Each tap weight is `afRGBA_Modulate[k]`. The output keeps the chroma
  and replaces the luminance:
  ```
  L = mean(rgb); out.rgb = rgb / L * pow(clamp(L, 0, 64000), GammaCorrection)
  ```
- `AnalyzeIntensity`:
  ```
  out = (dot(t0, Modulate[0]), …, dot(t0, Modulate[0]) - t1.a)
  ```
  That is the measured luminance and its difference from the previous adapted value (t1).
  The adaptation update and the read-back of section 1 then follow.

### Tone map (`TonemapHDR_*`, final pass)

Inputs:

- t0 is the scene HDR, t1 the glare, t2 the exposure/adaptation texture (sampled at `v3`),
  t3 the dither noise;
- `M = afRGBA_Modulate` (c154..), `O = afRGBA_Offset` (c186..), `C = m44_ColorTransformMatrix`
  rows (c210..212), `g = GammaCorrection` (c202.x);
- `sat(x, s) = mean(x) + s * (x - mean(x))`.

```
e      = t2(v3)
eGlare = saturate(sat(e*O0.x + O0.y, O1.x)) * O0.z + O0.w       // O0 = c186, O1 = c187
eScene = sat(e*O2.x + O2.y, O3.x) * O2.z + O2.w                 // O2 = c188, O3 = c189
glare  = (t1 * eScene + eGlare) * M0.y                          // M0 = c154 (saturated in most curves)
x      = scene' * eScene * M0.x                                 // scene' = decoded t0, below
y      = curve(x)
y      = y + (1 - y) * glare                                    // screen-blend the glare; LINEAR: y = x + glare
c      = saturate(C * float4(y, 1))
out.rgb = pow(c, g) + t3.x * Dither.x + Dither.y;  out.a = saturate(dot(t0, M1))   // M1 = c155
```

`scene'` depends on the scene format:

- linear for `LINEAR*`/`LOG*`/`REINHARD`;
- for `_PREMAP` Reinhard-encoded input:
  `scene' = (s + sqrt(s * (s + H.x) + 1) - 1) * H.w`, with `H = fParam_HDRFormatFactor_REINHARDRGB`
  (c4);
- for `ALPHALUM`: `s.rgb * (s.a² * c3.z + 1) * c3.x`.

`curve` by suffix, with `T = fParam_TonemapMaxMappingLuminance` (c208):

| Suffix | `curve(x)` |
|---|---|
| `LINEAR` | `x` |
| `LINEARSAT` | `clamp(x, 6.1e-5, 1)` |
| `LOG` | `saturate(ln(1 + max(x, 6.1e-5) * T.z) * T.y)` |
| `REINHARD[_PREMAP]` | `saturate(x*T.y * (1 + x*T.y*T.z) / (1 + x*T.y))` (extended Reinhard; `T.z` is 1/white²) |
| `REINHARD_SENSITOMETRIC_PREMAP` | `saturate((1 - exp(-x*T.x)) * (1 - T.y*exp(-x*T.x))²)` |

Name parts:

- `_Exposure` means the t2 term is present;
- `_Matrix` is the colour matrix `C`, INFERRED to carry ColorH/S/V and Contrast
  (`FUN_1416CBB90..BC90`);
- `_Gamma` is the power `g`;
- `_Dither` is t3;
- `_Vignette`/`_VignetteSim` add `fParam_VignettePowerOfCosine`/`fParam_VignetteSimulate`;
- `_PreMatrix` applies `m44_PreTonemap(Glare)ColorTransformMatrix` before the curve.

### Colour grading

`MadColorGradingLutT1`:
```
out.rgb = t16_3D( (t0 * M0).rgb * O0.rgb + O1.rgb )   // O0 = c186, O1 = c187, a 16³ LUT
```
This is the `LutCopy2DTo3DCs` 3D LUT of section 5.

### For the native backend (INFERRED)

1. Identify which technique DS3 runs: hash the PS bound at the last YEBIS draw of an original
   frame and look it up in the table.
2. Take its constants (`$Globals`) from a capture, with the formulas above, and relate them
   to the globals by varying Exposure, ToneMappingFactor, FilmicContrast, Gamma, ColorHSV and
   Contrast.
3. Statically, only the library's CPU constant builder remains open (the `1416C…`/`14192…`
   functions behind `FUN_1416CA450` and the colour calls). The shaders and their reflection
   are now fully known.

## 16. The world tone map end to end: which curve DS3 selects, its constants, exposure (STATIC; INFERRED where marked)

Sections 1, 5, 6 and 15 hold the engine side, the observed pass order and the shader formulas.
This section closes the selection and the constants. A native backend can then reproduce the
family without a capture.

### Inputs actually used by the world (gparam data)

- The world scene interpolates the ToneMap bank of its map key (section 1). Values read with
  `Get-GparamValues.ps1` (dumps in `evidence/I-0067/queries/yebis-world/`):

| File, id | ToneType | Exposure | AutoExposure | Gamma | ToneMappingFactor | FilmicContrast | MiddleGray |
|---|---|---|---|---|---|---|---|
| `m40_00_0000` / `m40_00_0010`, id 0 | **2** | **2.3** | **0** | 2.2 | 32 | **0.15** | 0.02 |
| `default`, id 0 | 2 | 1.52 | 0 | 2.2 | 32 | 0.3 | 0.18 |
| `default`, ids 10..15 | 2 | 2 | 0 | 2.2 | 32 | 0.5 | 0.18 |

- **Auto exposure is off in every set read.** The exposure is the gparam value, blended
  between keys A and B. `FUN_1400E69A0` reads it back from YEBIS only when AutoExposure is
  on (section 1).
- The second half of the group (`Exposure` 4.6, `AvgLuminance`, `ToneMapMode`, `UseTable`,
  `LuminanceBalance`, `Scaler`) feeds the GX-native `GXToneMapFilter`, not YEBIS (section 1).
  Which of the two filters runs per map is still not established statically; K-0410 and
  K-0433 show YEBIS on m40.

### How the parameters reach the library (STATIC, `FUN_1400E69A0` with `R14 = 144961F70`)

| Global | Call | YEBIS context field (`[1448E5E38]`) |
|---|---|---|
| Exposure `144962008` (`+0x98`) | `FUN_1416CA450` arg 2 | `+0x6E8` (when > 1.18e-38) |
| Gamma `14496200C` (`+0x9C`) | `FUN_1416CA450` arg 3 | `+0x6EC` (≥ 0) |
| **ToneType `144962010` (`+0xA0`)** | `FUN_1416CA450` arg 4 | `+0x6F0` |
| ToneMappingFactor `144962014` (`+0xA4`) | `FUN_1416CA450` arg 5 | `+0x6F4`, or `1/x` into `+0x6F8` when `+0x456` |
| (constant −FLT_MAX: "default") | `FUN_1416CA450` arg 6 | `+0x750` |
| **FilmicContrast `144962018` (`+0xA8`)** | `FUN_1416CA6A0` | `+0x70C`, clamped to [0, 1] |

### Technique selection (STATIC)

1. `FUN_1416E8C60` normalises `+0x6F0`:
   - −1 means automatic (2, `0x50005`, `0x50008` or `0x30002` from the format flags `+0x452/454/458`);
   - **the low word is the curve and the high word the scene HDR encoding** (0 linear float,
     3..7 premapped formats).
2. `FUN_1416FDBB0` → `FUN_1416FDAF0` maps the curve to a technique index (table `1448E66F0`):
   0→0, 1→1, **2→10**, 3→2, 4→4, 5→6, 6→8, 7→14, 8→1.
   - When the scene is premapped (`+0xC4F`), even indices take the next odd one (the
     `_PREMAP` variant, `FUN_1417006A0`).
3. The technique suffix is the table `1448E7540` (filled by `FUN_141717660`):
   - 0 `_LINEAR`, 1 `_LINEARSAT`, 2 `_REINHARD`, 3 `_REINHARD_PREMAP`, 4 `_REINHARDLUM`,
     5 `_REINHARDLUM_PREMAP`, 6 `_LOG`, 7 `_LOG_PREMAP`, 8 `_LOGLUM`, 9 `_LOGLUM_PREMAP`;
   - **10 `""` (no suffix)**, 11 `_SENSITOMETRIC_PREMAP`, 12 `_REINHARD_LINEARSAT_PREMAP`,
     13 `_REINHARD_SENSITOMETRIC_PREMAP`, 14 `_LINEARSAT`, 15 `_ALPHALUM_PREMAP`.
   - The prefix parts `Dither`, `Vignette`/`VignetteSim`, `Exposure`, `Matrix` and `Gamma`
     follow the flags gathered by `FUN_1416EB110`:
     - `Gamma` when `+0x6EC != 1`;
     - `Matrix` from `+0x754` (`+0x8D4`) and the HSV/contrast matrix (`FUN_1416F7010`);
     - `PreMatrix` from `+0x794` (`+0x8D5`).
4. **So DS3 (ToneType 2, linear float HDR) draws `ps_TonemapHDR_[Dither_][Vignette_]Exposure_[Matrix_]Gamma`
   with no curve suffix.** Its disassembly (`evidence/I-0067/queries/yebis-world/ps_TonemapHDR_Dither_Exposure_Matrix_Gamma.asm`,
   file offset `0x37D3B40`) is the **sensitometric (filmic) curve**:

```
x = max(scene * eScene * M0.x, 6.1e-5)
y = saturate((1 - exp(-x*T.x)) * (1 - T.y*exp(-x*T.x))²)          // T = c208
y = y + (1 - y) * glare;  c = saturate(C * float4(y, 1));  out = pow(c, g) + dither
```

  The rest is as in section 15 (eScene and eGlare from t2 with c186..189, glare t1, matrix
  c210..212, `g` c202.x).

### The curve constants (STATIC, `FUN_141718390`, case 10)

- `T.x = 1 + 2.08914 · FilmicContrast^(1/3)`. The constants are `0x4005B443` and the exponent
  `0x3EAAAAAB`, passed to `FUN_141FD3430`.
- `T.y = lerp` of the 21-entry table `142B60C40` at `FilmicContrast · 20`
  (`FUN_141743210`, clamped to [0, 1]; INFERRED: the same FilmicContrast argument):
  ```
  0, 0.048482, 0.090533, 0.12746, 0.16023, 0.1896, 0.217, 0.24664, 0.27911, 0.31463, 0.35344,
  0.39587, 0.4422, 0.49272, 0.54786, 0.608, 0.67358, 0.74507, 0.823, 0.90795, 1
  ```
- `T.z = T.w = 0`. `ToneMappingFactor` does not enter this curve: `FUN_141724DB0` runs for the
  other indices only.
- **Values:**
  - m40 (FilmicContrast 0.15): `T = (2.1100, 0.12746, 0, 0)`;
  - default id 0 (0.3): `T = (2.3985, 0.217, 0, 0)`;
  - ids 10..15 (0.5): `T = (2.6582, 0.35344, 0, 0)`.
- The other curves' constants come from the same function:
  - Reinhard (2/4): `T = (4/r² − 2, 1/div, factor²/(2·div), 1/r²)`, with `r = factor/div`, `factor` = `+0x6F4` and `div` = `+0x6F8`, after `FUN_1417432C0`'s adjustment.
  - Log (6/8) and linear-sat (14) are not used by DS3's data.

### Exposure and the remaining constants (INFERRED, to confirm on one capture)

- With auto exposure off, the library feeds the fixed exposure (`+0x6E8`, 2.3 on m40) through
  the t2 path. `FUN_1416E3F90` passes `exposure / fVar4` (fVar4 = `+0x698`, or `+0x6E8·+0x69C`
  when `+0x454`) to the draw.
  - The t2 terms of section 15 (c186..189) then reduce to that scale.
  - INFERRED: `eScene` = the gparam exposure.
- `g = c202.x` = 1/Gamma = 0.4545.
- `M0.x`/`M0.y` (c154) are the scene and glare scales. `C` (c210..212) is identity unless
  ColorH/S/V or Contrast differ from neutral (`FUN_1416CBB90..BC90`).
- **One original capture of the final YEBIS draw** gives c154, c186..189, c202 and c208. It is
  enough to check these formulas, including `T` against the values above.

### Pass order and intermediates (OBSERVED in I-0054, section 5; formats from that capture)

| Step | Resource |
|---|---|
| Scene HDR copy (`PointSampleColorClamp1`) | full-resolution `R16G16B16A16F` |
| Luminance reduction (`MadLumGamma*`, `AnalyzeIntensity`) | small float targets; eye adaptation into **640×540 `R11G11B10F`** (kept between frames) |
| Glare and bloom pyramid, light shafts, DoF (with depth) | half-resolution and smaller targets (sizes not recorded statically) |
| DoF focus table (`CustomDofLutCopy`, compute) | **2048×2 `R32F`** (read by the next frame's first draw) |
| Colour grading LUT (`LutCopy2DTo3DCs`) | **16³** 3D LUT |
| Tone map (`TonemapHDR_*`, second-to-last YEBIS draw; corrected by I-0068) | an intermediate target |
| Colour grading (`MadColorGradingLutT1`, t0 = tone-mapped image, t16 = 16³ LUT), **final YEBIS draw** | the 8-bit LDR target (`R8G8B8A8`, back-buffer size) |
| After YEBIS (GX) | `PointSampleColor1`, `DLAA`, `EmissiveToGlowPS`, `Sample13Weight` ×4, `GlowCompositeSinglePS` ×5 |

Correction (I-0068, INFERRED from the inputs of the last two YEBIS draws of the I-0053 m40 frame):

- the tone map reads exactly its four inputs (scene, glare, exposure, dither) and writes an
  intermediate;
- the last draw reads that intermediate and the 3D LUT, and writes the LDR target.

The tone-map formulas above are unchanged. The full draw-by-draw order and the glare pyramid
sizes are in [I-0068 YEBIS-NATIVE](../I-0068-static-yebis-ffx-lights-spec/YEBIS-NATIVE.md) §3.2
and §4.1.

Exports: `decompiled/DS3-1152-F-016CA450`, `016CA6A0`, `016E8C60`, `016FDBB0`, `016FDAF0`,
`017006A0`, `016EB110`, `016E3F90`, `01718390`, `01743210`, `017432C0`, `01717660`.

## 17. Glare/bloom pyramid and the GX filters after YEBIS (STATIC; sizes and weights partly open)

### Ownership

- The pipeline instance (`FUN_1400843C0`) owns the GX post filters:
  - **`GXBS::GXAntiAliasingFilter`** (vtable `143D5D408`, slot 0 `FUN_1401B9790`);
  - **`GXBS::GXGlowFilter`** (`143D5D490`, slot 0 `FUN_1401BB810`);
  - **`GXBS::GXToneMapFilter`** (`143D5DE88`, constructor `FUN_1401C2FC0`, **at
    `pipeline+0x3060`**, slot 0 `FUN_1401C48D0`).
- YEBIS (`GXYebis2Filter`, section 1) runs only when pipeline flag bit 10 is set
  (`FUN_1400FB890`: `[144961F70] != 0` and `(flags >> 10) & 1`).
- The world sets bit 10. The menu views do not (MENUS-MAP section 12).

### YEBIS glare/bloom: parameters and where they go (STATIC)

The gparam **Bloom** bank (`[GXRS+0x4620]` keys, section 1) → `FUN_1400E4460` → YEBIS globals.

- `GlareLuminance`, `BloomLuminance`, `GhostLuminance`, `StarLuminance`, `AfterimageLuminance`
  and `AnamorphicFlareLuminance` are multiplied by the global glare factor
  `[[GXRS+0x5368]+0x68]`.
- The names come from the registration table (`yebis-vars.tsv`; strings at `143D4D920..`).

| Global | Name | m40 value |
|---|---|---|
| `14496204C` | GlareLuminance (× factor) | 3 |
| `144962050` | GlareThreshold | 0.2 |
| `144962274` | GlareSamplingThreshold | 0.01 |
| `1449621B4` | RemapFactor | 0.5 |
| `144962048` | glare shape (0 when the factor ≤ 0; 14 = custom) | gparam GlareShape 1 |
| `14496205C/60/64` | GlareStarFilterThreshold, GlareStarLengthFovDependence, GlareStarSoftness | 0.001, 0.5, 0.25 |
| `144962068/6C/70` | GlareBloomGaussianRadiusScale, GlareBloomLuminanceGamma, GlareGhostConcentricDistortion | 2, 1.4, 1 |
| `144962278..2BC` | custom-glare set: Bloom/Ghost/Star/Afterimage luminance, dispersion, lengths, angles | all 0 on m40 |

- **Calls** (`FUN_1400E69A0`):
  - `SetGlareParameters` `FUN_1416C9670(yebis, GlareLuminance, GlareThreshold, 1, RemapFactor)`
    writes context `+0x6A4`, `+0x6A0`, `+0xBBC` and `+0xBC0` (RemapFactor floored at 0.001);
  - then `FUN_1416C9690(shape)` → `+0xBC4` for shapes 0..13;
  - shape 14 uses the custom set through `FUN_1416C96B0`.
- **Pyramid object** `PPFX::CRenderGlare` (vtable `142B63138`; validation `FUN_141748EF0`):
  - base size `+0xB0/+0xB4` (≥ 16 each);
  - **up to 16 levels** (`+0xD0`, level targets `+0x230[]`);
  - a second level set `+0xE4` (≤ 16) and `+0x270[]` (`+0xF0`);
  - fixed targets `+0x238`, `+0x280..+0x2B0`.
- **Values that drive the level count:** `Setting Info` GlareQuality 4 (m40 and default) and
  Bloom GlareNumLevels 6 (m40), 8 (default id 0).
  - INFERRED: one level per halving from the glare base size.
  - The base size, the formats and the per-level weights are computed inside the library and
    were not decoded.

### GX tone map filter with its own bloom (`GXToneMapFilter`, pipeline `+0x3060`)

- **Per frame** `FUN_1400FB890` (unless `pipeline+0x3688`) copies the second Bloom block into
  the filter at `+0x60..+0x7C`. That block is Enable, GlareNumLevels, Glare Luminance (× the
  same glare factor), Glare Threshold, GlareBloom Alpha, GaussianRadiusScale, Sturate Ctrl,
  Smooth.
  - Enable = gparam enable, and the factor must be > 0.
  - Then `FUN_1401C4730(+0x3060, frame)`.
- **Values:**
  - second Bloom block on m40 and default: **Enable 0**, GlareNumLevels 7, Glare Luminance 1,
    Threshold 0.5, Alpha 1, GaussianRadiusScale 1, Smooth 0.8;
  - tone-map part (second half of the Tone Map group): Exposure 0, AvgLuminance 1.8,
    ToneMapMode 0, Contrast 0.5, Gamma 2.2, UseTable 1, Enable AutoExposure 0;
  - on `default` ids 10..15, the menu views: ToneMapMode 7, second Exposure 4.6.
- **Execute `FUN_1401C48D0(filter, frame, param)`:**
  - inverse scale `1 / (f(param[0]) / param[1])`, with `f = FUN_141FD3430` (a power);
  - **bloom level weights normalised by `1 / Σ_{k<N} d^k`**, with `N = param[0x11]` and
    `d = param[0x17]`;
  - the constant block (param `[4..7]`, `[9..10]`, `[0xC..0xE] × [0x20]`, `[0xF]` with the
    aspect ratio, `[0x12..0x15]`) is uploaded with `FUN_141920300`, then the passes are
    recorded.
  - INFERRED mapping: `N` = GlareNumLevels, `d` = Smooth.
- `TONEMAP_MODE` is a GUI enum of the filter (`143D5DE18`); the mode values were not named
  statically.
- **Where it applies:** INFERRED, in pipelines without YEBIS (the menu views). Its exposure is
  the Tone Map bank's second block (MENUS-MAP section 27; not a fixed `+0x3E84..+0x3EA4`, a MotionBlur copy: I-0113 RESULT « Tranche 27 »). With the bloom block disabled in every
  gparam read, only its tone map part matters there.

### Glow (`GXGlowFilter`; observed `EmissiveToGlowPS`, `Sample13Weight` ×4, `GlowCompositeSinglePS` ×5)

`FUN_1401BB810(filter, frame, param)` runs when `param+0x24` (enable) is set and the global
glow factor `[[GXRS+0x5368]+0x80] > 0`. The resources are `[GXRS+0x4DD8]` (the GX common
post object).

1. **Strength:** `s = lerp(param+0x14, 0, 1 − factor)`, passed with `param+0x18` as the first
   constant (`FUN_1421397C0`).
2. **Emissive to glow** into the glow source:
   - from `param[1]`, or the filter's own target when `param+0x1C`;
   - with `FUN_14007F820`.
3. **Mode A** (`param+0x1C == 0`): four blurs alternating between two common targets
   (`[GXRS+0x4DD8]` `+0x840` and `+0x8E0`), each with `FUN_14007F950(…, count = param[2])` and
   directions (2, 2)/(1, 2). Then a composite.
4. **Mode B** (`param+0x1C != 0`): a four-level chain over the filter's own targets
   `filter+0x28..+0x40` (each level from the previous level's `+0x48`), with the same blur.
- These match the observed four `Sample13Weight` and five composites (one composite per level
  plus the final one).
- The **gparam `Glow Filter`** group exists only at `default` id 10:
  - Enable 0, BlurSampleScale 20, Blur RgbScale 3, Blur AlphaScale 1, New Glow Enable 0,
    NewGlow Final ColorScale 1;
  - the world keys (m40 id 0) fall back to `{1, 0}`, which has no Glow entry.
  - INFERRED: the observed world glow is enabled by the frame's emissive content and the
    filter's defaults, not by gparam.

### Anti-aliasing (`GXAntiAliasingFilter`, observed `DLAA`)

`FUN_1401B9790(filter, frame, param)` runs when the filter is enabled (`filter+9`) and
`[[GXRS+0x5360]+8]`, with the AA factor `[[GXRS+0x5368]+0x98] > 0`.

- `mode = param[0] < 4` selects the programme at `filter+0x68 + 8·mode`.
- Constants:
  - target size `(w, h)`, `(1/w, 1/h)`, `(0.5/w, 0.5/h)`, `(2/w, 2/h)`;
  - for **mode 3** the threshold term is `lerp(param[1], 1, 1 − factor)` with `param[2]` and
    `param[3]`; the other modes take `param[1..3]` and scale `param[0]`;
  - mode 3 also fills a table of offset pairs (`1/w`, `1/h` × the constants `143D7F0CC`,
    `143D7EE38`, `143D7F104`, `143D7F0FC`, `143D7F0F0`, `143D7EEC8`).
- **gparam `Anti-Alias`:** m40 has no group, so it falls back to default id 0:
  - Enable 1, **AntiAliasType 4**;
  - DlaaThreshold 0.1, DlaaRamda 1.4, DlaaEpsilon 0.12;
  - FXAA Alpha 1, ReduceMin 1/128, ReduceMul 0.125, SpanMax 8.
  - INFERRED: gparam type 4 is the DLAA mode (filter mode 3) seen in the capture.

### Open and how to close it

Static decoding of the YEBIS library's resource sizes and the filters' constant layouts is long
for little gain. **One original capture of a world frame** (the I-0054 route) gives the
remaining numbers directly:

- the glare target sizes and formats;
- the per-level blur constants (`afParam_TexCoordScaler`, `afRGBA_Modulate`);
- the GX glow and DLAA constant buffers.

The parameter sources above then tie each constant to its gparam field.

Exports: `decompiled/DS3-1152-F-001B9790`, `001BB810`, `001C48D0`, `001C2FC0`, `016C9670`,
`016C9690`, `01748EF0`, `000E4460`.

## 18. The SSS of enemies and NPCs: materials, per-material profiles, what the native needs (game data + STATIC; INFERRED where marked)

Data: the 165 character FLVERs (MENUS-MAP section 66) joined with `AllMaterialBnd`; table
`evidence/I-0067/queries/chr/chrsss.tsv` (chr, material, MTD, spx, `g_SSSWidth`,
`g_SSSStrength`, `g_SSSStrength_UseCAParam`, GX items, `GX15` strengths).

**35 characters, 77 materials:** `c1130`, `c1320`, `c1321`, `c1340`, `c1400`, `c2020`, `c2070`,
`c2090`, `c2120`, `c2130`, `c2131`, `c2132`, `c2140`, `c2170`, `c2180`, `c2280`, `c3080`,
`c3140`, `c3141`, `c3190`, `c3200`, `c3250`, `c5020`, `c5180`, `c5210`, `c5250`, `c6020`,
`c6120`, `c6121`, `c6200`, `c6201`, `c6240`, `c6280`, `c6310`, `c6331`.

**MTDs:**

| MTD | Materials |
|---|---|
| `C[ARSN]_SSS` | 56 |
| `C[ARSN]_SSS_em_Glow` | 8 |
| `C[ARSN]_SSS_e` | 6 |
| `C[ARSN]_SSS_e_Cloth` | 2 |
| `P[ARSN]_SSS` | 2 |
| `C[ARSN]_SSS_Cloth` | 1 |
| `C[ARSN]_SSS_em_e_Glow` | 1 |
| `c3141_C[ARSN]_SSS` | 1 |

The spx are `GXFlver_SSSIbl_Bloody[_Cloth]` and `GXFlver_SSSEmiIblGlow_Bloody`. All of them
share **`g_SSSWidth` = 0.025, `g_SSSStrength` = (1, 1, 1) and
`g_SSSStrength_UseCAParam` = True**.

**Every one of the 77 materials carries a `GX15` item** (section 8: `FUN_14022BF70`). Each
therefore creates, at FLVER load, a **linked instance profile**
`FUN_140113D80(manager, base)`, with the base width (0.025) and the item's strength:

| `GX15` strength (R, G, B) | Materials | Examples |
|---|---|---|
| 1.0, 1.0, 1.0 | 54 | |
| 0.3 | 10 | `c2020`, `c2120`, `c3190`, `c3200`, `c3250`, `c6020`, `c6331` |
| 0.25 | 4 | `c6120`, `c6121` |
| 0.4 | 3 | `c2140` body, `c6310` |
| 0.2 | 2 | `c6200`/`c6201` face |
| 0.2, 0.2, 0.21 | 1 | `c5210` |
| 0.325 | 1 | `c6020` head |
| 0.1 | 1 | `c2140` eyes |
| 1.0, 0.4, 0.4 | 1 | `c2280` |

The material item lists are `GX00 GX05 GX20 GX22 GX24 GX26 GX15` (65 materials), with `GX03`
and `GX32` (9) or `GX04` (3).

Because `UseCAParam` is true and the instance profile exists, section 8's selection gives
**`FC_SSS_ID` = the instance profile's slot**, not the MTD's base.

**What the native needs for the enemies (INFERRED from sections 3, 7 and 8):**
1. **One instance profile per SSS material of a loaded character resource**, linked to its MTD
   base: strength from `GX15`, width from the base (`FUN_140112C60` follows the link). With
   the base alone, the 23 materials with a strength other than 1 would scatter too strongly.
2. The `FC_SSS_ID` word (constant `+0x190`) set to that profile's slot of the frame.
3. The per-frame upload: active profiles only, up to 0xE2; a slot per profile used this frame
   (`+0x36`). The 77 enemy profiles, the bases and the player's fit within that limit.
4. The SSS stage itself (section 2's native consequence): without `GXSssBlurX/Y` and
   `GXSss`, every one of these 35 characters loses its scattered diffuse, as bare skin does.

Witnesses for the enemy SSS (MENUS-MAP section 69): `m30_00` `c2020` at (−14.5, −4.3, 103.4),
strength 0.3; `m40_00` `c3190`/`c3200` near Firelink Shrine, 0.3; `m40_00` `c1400`, 1.0.
