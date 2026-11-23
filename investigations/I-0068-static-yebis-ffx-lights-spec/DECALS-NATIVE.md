# I-0068 part S: deferred decals (gxdecal), ready to code

Request (coordinator, 2026-10-10): after the coverage matrix, attack the first family with no
specification, in ready-to-code format. That family is the deferred decals, `gxdecal`
(COVERAGE-MATRIX row 29). They are box-projected decals: blood, dirt, stains and footprints,
drawn into the G-buffer after the opaque pass.

Labels: STATIC, OBSERVED, INFERRED.

**Evidence** (`evidence/I-0068/queries/decal/`):

- `decal-draws-all.tsv`: every 7-target draw in the 123 retained captures, from
  `Get-DecalDraws.ps1`;
- `decal-draws-i63-live-10.tsv`: the five witness zones (no decal draws);
- the Ghidra query indexes.

**Decompiles in `decompiled/`:** 51AA0, 19B690, 198910, 50720, 51320, 52790, 527F0, 52840,
52890 and 5A120.

**Programme interfaces:** reflected from the game's own DXBC (`fxc /dumpbin` on the I-0063
programme cache, outside the repository; only the facts are quoted here).

## 0. Native status (`main`, code read)

| Part | Native | Proof |
|---|---|---|
| CPU decal manager (`GXDecalManager`, vtable `3D20460`): registry, handles, requests `51BF0`, validation `52B60`, synchronous world query `C1FB0`, retirement | **DONE** (CPU only) | `native/frame/game_bootstrap.cpp:810` (construction), `:912` (requests), `:887` (query), `:851` (retirement) |
| programme and constant-description selection (`51320`: 47 programmes, 14 descriptions) | **DONE** (CPU lookups only, no GPU buffers) | `native/frame/game_bootstrap.cpp:980` |
| GPU resources of the manager (`50720`: cube buffers, three constant buffers, 16 + 8 states) | **ABSENT** (slots `+0x280..+0x3D7` zeroed on purpose) | `native/frame/game_bootstrap.cpp:836` |
| scene consumers `19B690` (submit) and `198910` (draw) | **ABSENT**: both hooked to `UnownedNativeDecalSceneConsumer`, which refuses | `native/frame/game_bootstrap.cpp:977`, `:8522`, `:8523` |
| G-buffer stencil bit 0 (the "decal receiver" mark) | **wrong for characters**: native writes `9` (bit 0 set) for every mesh | `native/frame/game_bootstrap.cpp:4211` |

**So the CPU side exists and the rendering is absent.** The specification follows.

## 1. The original pass (OBSERVED)

**Where.** The five witness zones (m31, m37, m45, m50, m51) draw no decal in their retained
captures (`decal-draws-i63-live-10.tsv`, 0 rows). Decals appear in Firelink-session captures:

- I-0055 `stream1`: 21 draws in three frames;
- I-0064 `argo-*`: 6–15 draws;
- I-0065 `argo-*`: 2–18 draws;
- the m40 listing (I-0053): 51 draws per frame.

Every drawn decal uses the same programme pair, **`Decal_DifSpcAm`** (named in the m40 listing).

**Order** (m40 listing, I-0055 events): it comes after the whole G-buffer pass and before the
depth downsample, SSAO, env-map regions and lighting.

1. **Three copies** (`CopyResource`, the frame renderer's list at `140104BE0` to `140104C57`):
   - GBuffer0 (normals, RGBA16F) → `+0x1110`, the **"LightPrepassAttr"** copy, read at t13;
   - GBuffer1 (RGBA8) → `+0x1118`, read at t14 by the blood programme;
   - main depth (`R32G8X24`) → `+0xFD0`, the **readable depth**, read at t4. This is the same
     copy as FORWARD-DEPTH-NATIVE's.
2. **Decal draws**, one per decal, in the manager's list order:
   - topology triangle strip; `DrawIndexed(14)`; R16 index buffer;
   - vertex buffer: stride 12, eight `float3`;
   - **seven render targets**, depth bound read-only, stencil ref **1**.

**Targets** (m40 ids; I-0055 has the same ids):

| RT | Resource | Format | Role |
|---|---|---|---|
| 0 | `12461` | R11G11B10 | env-map regions' SpecularAcc (native `native_env_target_`) |
| 1 | `12444` | RGBA8 | GBuffer1 |
| 2 | `12445` | RGBA8 (typeless) | GBuffer2, albedo |
| 3 | `12446` | RGBA8 | GBuffer3, reflectance (sqrt-encoded) |
| 4 | `12458` | R11G11B10 | GBuffer4, ambient irradiance |
| 5 | `12447` | RGBA16F | GBuffer5, emissive |
| 6 | `12536` | RGBA16F | GBuffer0, normal |

**Observed states** (the capture's D3D11 descriptors):

| State | Value |
|---|---|
| blend, RT0 | SRC_ALPHA / INV_SRC_ALPHA / ADD; alpha ZERO / ONE / ADD; mask **RGB** |
| blend, RT1 | the same, mask **RG** (keeps B and A: A holds the SSS profile index) |
| blend, RT2, mode A (blend 32; 15 of 21 draws in I-0055) | **DEST_COLOR / INV_SRC_ALPHA** (multiply) |
| blend, RT2, mode B (blend 36; 6 draws) | **ONE / INV_SRC_ALPHA** (premultiplied) |
| blend, RT3–RT7 | SRC_ALPHA / INV_SRC_ALPHA, alpha ZERO / ONE, mask RGB |
| depth-stencil | depth on, **no write, LESS_EQUAL**; stencil on, read mask **0x01**, write mask 0x00, both faces **EQUAL** |
| rasterizer | SOLID, **cull FRONT**, FrontCounterClockwise = 1, depth clip on, no bias |
| samplers | s0–s12 = the GX sampler set (`10AB80`); the programme uses s0, s1, s2 (wrap, material), s6 WrapPoint, s7 ClampPoint |
| constant buffers | VS b10; PS b2, b3, b7, b10, b11 (b4, b8, b9 are the frame's bindings and unused) |

**The receivers (stencil bit 0).** The G-buffer draws use stencil refs **9** (map pieces), **8**
and **12** (SSS skin). This was counted on 6-target draws in I-0055 and in i63 m31. The decal
test (EQUAL 1, mask 1) keeps only the bit-0 pixels.

The rule, STATIC (`140850`, lines 229–243 of the decompile), starts from `1` and clears it unless
all three hold:

- draw-args `+0x18C` bit 2 is set. `5A120` sets it for draw-entity kinds **3 and 4** only;
  INFERRED: map and object parts, never characters (kind 0);
- the material's built-in object exists;
- its byte `+0x80` is zero. INFERRED: `g_GX_bDisableDecal`, MTD built-in kind 19, next to
  kind 20 `+0x81`. Native already parses it as `policy.disable_decal`
  (`native/content/model_parameters.cpp:422`).

The rest of the reference: `|8` for the G-buffer pass, then `|4` (SSS) or `|2`.

## 2. The static flow

```
frame renderer (140103510 / 1400FCE30)
  └ copies (above), then, if manager+0x3D8 and pass != 0x39:
    51AA0(manager, list, depth=+0xFD0, attr=+0x1110, gb1=+0x1118)      // GXDecalManager::Draw
      gated by [[144961B50]+0x5360]+0x68 (decal option); stores attr/gb1 at manager+0x3E0/+0x3E8;
      batch = {depth, anyEmissive}  (anyEmissive = some decal has +0x80 != 0 and +0xFC > 0)
      for each decal object in manager+0x68..+0x70 (vtable 3D5B8D8), order index 8, 9, ...:
        decal.vt+0x28 = 19B690(decal, list, manager+0x3DC, batch, order)
          view test of the decal box (list+0x130 vt+0x58), then a render-list command
          kind 3 {decal, 0, batch}; list+0x128 = pass (+0x3DC, or 9 for blend-weight decals)
    command → 198910(_, manager, view, 0, decal, batch)                   // the draw
      skip if decal+0x270 (its material object) is null, or no diffuse (+0x190; +0x1B0 for blend-weight)
      programme/description by variant (§2.1); constants (§2.2); 18 textures (§2.3);
      [optional stencil pre-pass, off by default: manager+0x511 = 0 (constructor 502E0)]
      blend   = [[144961B50]+0x4DE8]+0x888/+0x890/+0x898 by decal+0x208 (0/1/2)
      program + cbModelParam layout; material CB manager+0x280+desc*8
      depth   = 52790(manager, cameraInside ? 0 : 2, prepass) → manager+0x318[...]; stencil ref 1 (0x20 with pre-pass)
      52890(manager, view, cameraInside ? 2 : 1): VB +0x308 (stride 12), IB +0x310 (R16),
            strip, raster mode, DrawIndexed(14)
```

### 2.1 Programme selection (198910, STATIC)

The names table is at `14473C860`, 47 entries. The constant-description table is at
`14473C7F0`, 14 entries. Inside each family, the four entries are indexed by
`Am + 2·E`:

- `Am`: a mask texture `+0x1C0` is present;
- `E`: the batch's `anyEmissive`.

| Condition (decal object) | Programmes [base, Am, E, EAm] | Description |
|---|---|---|
| `+0x26C` set (blend-weight decal) | `Decal_BlendWeight` (30) | 9 |
| else `+0x25B` (POM) | `DifPom` 5, `DifPomAm` 19, `DifPomE` 12, `DifPomEAm` 26 | 5 |
| else `+0x258` (blood) | `Decal_Blood_DifSpcBump` (29) | 8 |
| else no bump (`+0x25A` = 0), no specular (`+0x198` null) | `Dif` 1, 15, 8, 22 | 1 |
| else no bump, specular | **`DifSpc` 2, `DifSpcAm` 16**, 9, 23 | 2 |
| else bump, no specular, parallax `+0x210` ≈ 0 | `DifBump` 3, 17, 10, 24 | 3 |
| else bump, no specular, parallax ≠ 0 | `DifBumpPrx` 4, 18, 11, 25 | 4 |
| else bump and specular | `DifSpcBump` 6, 20, 13, 27 | 6 |
| `+0x248` = 1 (cubic), then the same four bump/specular cases | `Cubic_Dif` 31/39/35/43, `Cubic_DifSpc` 32/40/36/44, `Cubic_DifBump` 33/41/37/45, `Cubic_DifSpcBump` 34/42/38/46 | 10–13 |

Notes:

- Index 0 is `Decal_PrePass`, description 0, used by the stencil pre-pass.
- `DifSpcPom` (7 and its variants) is in the table but no branch selects it.
- Box decals use the VS `Decal_*.vpo`; cubic decals use `Decal_Cubic_*.vpo`.
- The `Am` programmes have no VS of their own.

### 2.2 Constants (198910, STATIC; layouts from the DXBC)

**VS b10 `cbMatrixWorld`** (80 bytes, buffer `manager+0x2F8`):

- `VC_DecalTransform`: row-major 4×4, the unit cube to clip space;
- `VC_WidthScale` = (`+0x244`, `+0x244`, 0, 0): the far-end widening of perspective decals.

The VS widens x and z by `lerp(1, WidthScale, (y+1)/2)` before the transform.

**PS b10 `cbDecalParam`** (416 bytes, buffer `+0x2F0`):

| Offset | Field | Source |
|---|---|---|
| 0 | `FC_ProjToDecalSpace` (row-major) | clip `(ndc.xy, depth, 1)` → decal space; the PS divides by w and keeps [0,1]³ |
| 64 | `g_InvProjMatrix` | inverse of the view's projection (view `+0x50`) |
| 128 | `g_CameraMatrix` | the view's camera world (view `+0xD0..+0x10C`) |
| 192 | `unused_FC_ScreenSize` | (w, h, 1/w, 1/h) from `[view+0x418]+0x5C/+0x60` |
| 208 | `FC_DecalWorldRotation` (3×4) | the decal axes; row 1 (offset 224) is the projection axis, used by the normal fade |
| 256 | `FC_EyeInDecalSpace` | unused |
| 272 | `FC_ParallaxScale` | x = `+0x210`; y = 1 if a bump texture `+0x1A8`, else 0 (the normal output's alpha); z = `+0x214` if a specular texture `+0x198`, else 0 (the opacity factor) |
| 288 | `FC_DecalSoakThreshold` | `+0x250`, `+0x254` (blood) |
| 304 | `FC_CubeCenterOffset` | constants `1439F2490` / `1439F2498` |
| 320 | `FC_DiffuseScale` | `+0x218..+0x220` × material `+0xD0..+0xD8`; w = lifetime fade `19B880` (1 − min(1, `+0x414`/`+0x418`)) × material `+0xDC` |
| 336 | `FC_ReflectanceScale`, `FC_ShininessScale` | `+0x224..+0x230` |
| 352 | `FC_LightInDecalSpace` | 0 |
| 368 | `FC_EmissiveIntensity` | `+0x234`, animated by the curve `+0x1B8`/`+0x130` when present |
| 384 | `FC_NearFade_Start`, `ReciRange`, `FC_FarFade_Start`, `ReciRange` | manager `+0x40C`, `+0x410`, `+0x414`, `+0x418` divided by the decal depth. The far start falls back to `len − +0x418` when `+0x414` < 0. Defaults (`502E0`): 0, 0.25, −1, 0.5 |
| 400 | `FC_NormalThreshold_Start`, `ReciRange` | `c1`, `1/(c2 − c1)`. `c1` and `c2` come from the manager angles `+0x41C` (π) and `+0x420` (0) through `141778230`; INFERRED: a cosine (the decompiler lost its XMM arguments) |

**The other PS buffers:**

- **PS b2** (80 bytes, buffer `+0x300`): `+0x25C..+0x268` and constants. It is used by the POM
  programmes (`cbMatDynParam`).
- **PS b3**: the frame's global block `[[144961B50]+0x4DD0]+0x280`.
- **PS b7 `cbModelParam`** (`FC_DifColMul` …) and **PS b11 `cbCubeEnvSH1`** (3 SH rows):
  `59470` produces them. It runs over a draw state that `1A8580` builds from the decal's
  material object `+0x270`. This is the engine's generic model-parameter routine, and it binds
  b11 itself (`141920E20(…, 0xB, 1)`). The SH rows come from the env-map entries' SH at
  `+0x290..+0x2B8`, blended.

### 2.3 Textures (198910 → `140006100(ctx, 0, 18, list)`)

| Slot | Source | Programme name |
|---|---|---|
| t0 | `+0x190` (`+0x1B0` for blend-weight) | `g_DiffuseTexture` (cubic: `g_CubeDiffuseTexture`) |
| t1 | `+0x198` | `g_SpecularTexture` |
| t2 | `+0x1A0` | — |
| t3 | `+0x1A8` | `g_BumpmapTexture` (observed an 8×8 BC7 flat default when the decal has none) |
| t4 | `batch.depth` (readable depth copy) | `g_DepthTexture` |
| t5–t12 | `+0x1C8..+0x200` | blood sets 2 and 3: `g_DiffuseTexture2/3`, `g_SpecularTexture2/3`, `g_BumpmapTexture2` |
| t13 | `manager+0x3E0` (GBuffer0 copy) | `g_LightPrepassAttrTexture` |
| t14 | `manager+0x3E8` (GBuffer1 copy) | `g_GBuffer1Texture` (blood soak level) |
| t15 | `+0x1B0` | — |
| t16 | `+0x1B8`, or the GX default emissive when `E` is set | `g_EmissiveTexture` / `g_BumpmapTexture3` (blood) |
| t17 | `+0x1C0`, or the GX default mask `**[[144961AB0]+0x270]` | `g_MaskTexture` |

### 2.4 What each programme writes (DXBC, STATIC)

**`Decal_DifSpcAm`** (the observed one) declares **only five outputs, o0–o4**:

- **Clip:** decal-space position from t4 and `FC_ProjToDecalSpace`; discard outside [0,1]³.
- **Normal fade:** from t13's normal against the projection axis.
- **Near/far fade:** along decal y.
- **Opacity:** `a = fade × diffuse.a × DiffuseScale.w × mask.r × DifColMul.w × ParallaxScale.z`;
  discard when `a ≤ 0`.
- o0 = (0,0,0,a): SpecularAcc × (1 − a). INFERRED: a no-op here, because SpecularAcc is cleared
  before the decals and filled by the env-map regions afterwards.
- o1 = (shininess = bump.b × ShininessScale, 0, ·, a), written as R and G only.
- o2 = (a × diffuse.rgb × DiffuseScale.rgb, a): premultiplied albedo.
- o3 = (sqrt(specular × ReflectanceScale), a).
- o4 = ((1 − Fresnel-weighted reflectance) × max(SH1(n), 0), a): the decal's ambient
  irradiance.

**Bump variants** (`DifSpcBump`) add:

- **o5** = (0,0,0,a) on emissive;
- **o6** = (encoded normal, a × ParallaxScale.y) on GBuffer0.

**`Decal_Blood_DifSpcBump`** picks texture set 1, 2 or 3 by counting `GBuffer1.r` (the
surface's shininess) against `FC_DecalSoakThreshold.x/.y`. Rough surfaces soak.

**Consequence for D3D12:** the original binds seven targets for a five-output programme with
write mask RGB. D3D11 leaves undefined what lands in RT5/RT6 then. The native PSO must set
**`RenderTargetWriteMask = 0` for every target the programme's output signature does not
declare**, which matches the drivers' observed behaviour (INFERRED). The output count comes
from the reflected signature.

### 2.5 Geometry and states the native must own (50720, STATIC)

**Cube buffers:**

- **Vertex buffer** (`+0x308`, 96 bytes, from `143D204A0`), eight `float3`:
  (−1,1,1) (1,1,1) (−1,1,−1) (1,1,−1) (−1,−1,1) (1,−1,1) (−1,−1,−1) (1,−1,−1).
- **Index buffer** (`+0x310`, 28 bytes, R16, from `143D20480`), the strip:
  0,1,2,3,7,1,5,0,4,2,6,7,4,5.

**Constant buffers:** 416 (`+0x2F0`), 80 (`+0x300`) and 80 (`+0x2F8`) bytes.

**Depth-stencil states** (`+0x318`, `[4][2][2]` = 16 states plus `+0x398`):

- Each takes a base GX depth state (`[GXRS[0x54]+0x1D8]`, entry 0 or 2) and overrides the
  stencil fields.
- The draw path uses index `uVar1 + ((+0x50F == 0) + (3 − prepass)·2)·2`.
- **Observed: the outside-camera, no-pre-pass state** (§1).
- **Not observed:**
  - the camera-inside variant (base state 0);
  - the pre-pass states (ref 0x20).

**Blend states:**

- `+0x3A0..+0x3C8`: six, for blend-weight decals by `+0x208`;
- `+0x3D0`: the pre-pass;
- `+0x398`: eight-target.

The main decal blends are GX table entries `0x888/0x890/0x898` (§2). Two of them are observed
(§1).

## 3. Ready-to-code order (D3D12)

| # | Step | Native |
|---|---|---|
| 1 | **Fix the receiver bit** | G-buffer stencil reference: start at 1; clear it unless the draw entity is a map/object part (args `+0x18C` bit 2, kinds 3–4) and `!policy.disable_decal`; then `|8`, `|4`/`|2` as today. Characters get 8 / 12 as in the original. `native/frame/game_bootstrap.cpp:4211` |
| 2 | **Resources** | the GBuffer0 and GBuffer1 copies (same formats); the readable depth copy, shared with FORWARD-DEPTH-NATIVE; the cube VB/IB (constants above); three upload rings (416/80/80 bytes per decal) |
| 3 | **Copies** after the G-buffer | GB0 → attr copy, GB1 → GB1 copy, depth → readable depth (`CopyResource`, as the original) |
| 4 | **Collect decals** | the registered CPU objects in `manager+0x68..+0x70` (the native registry already owns this manager); skip when the decal option `[[144961B50]+0x5360]+0x68` is off, the material `+0x270` is null, or the diffuse is null; compute `anyEmissive` over the list |
| 5 | **Cull** | the 8 transformed cube corners against the view frustum (19B690's test); keep the list order |
| 6 | **Per decal** | choose the programme and description (§2.1); fill b10 VS / b10 PS / b2 (§2.2); produce b7 and b11 through the same routine as the meshes' `cbModelParam` (`59470` over a `1A8580` state from `+0x270`); bind the 18 textures (§2.3) |
| 7 | **PSO** | the original DXBC pair; seven RTVs [SpecularAcc, GB1, GB2, GB3, GB4, GB5, GB0]; blend from the decal mode `+0x208`; write mask 0 on undeclared outputs; depth read-only with stencil read 0x01 EQUAL, ref 1; raster: outside camera = observed (cull FRONT, FrontCCW), inside = mode 2 |
| 8 | **Draw** | strip, `DrawIndexed(14)` |
| 9 | **Then** | depth downsample / SSAO / env-map regions / lighting, unchanged |

**Camera inside the volume.** 198910 sets a flag when the camera lies inside the decal box. It
then uses depth state 0 and raster mode 2. Until those two are read (risk 2), treat the inside
case as INFERRED: no depth test, opposite faces.

## 4. Checks

| # | Check | Pass |
|---|---|---|
| D1 | receivers | the native G-buffer stencil matches the original per pixel: 9 on map, 8/12 on characters (I-0055 or a new Firelink capture) |
| D2 | structure | Firelink: the same number of decal draws as the original at the same camera; programme `Decal_DifSpcAm`; seven targets in the §1 order; stencil ref 1 |
| D3 | constants | b10 VS, b10 PS, b7, b11 bit-identical to the engine's for the same decal and frame. The retained captures hold no constant bytes: needs a new capture |
| D4 | images | GBuffer1/2/3/4 after the decal pass against the original's, at Firelink, still camera: mean absolute difference ≤ 2/255 inside decal pixels; no change on characters |
| D5 | lit HDR | the I-0063 lit-HDR comparison at m40 improves where decals sit; the five zones (no decals) stay identical |
| D6 | blood | after a hit on a wall, a `Decal_Blood_DifSpcBump` draw appears with three texture sets and the soak thresholds; rough surfaces take set 3 |

## 5. Risks and open items

1. **Request families.** The native CPU path accepts only kind 0 with the callback table
   `3D20450` and predicate `50120`. Other families refuse (`native_decal_cpu_query_family_not_owned`).
   Which families create the Firelink decals and the hit blood is not established. If they
   are refused, nothing reaches the list.
2. **States not observed:**
   - the camera-inside depth state (GX base 0) and raster mode 2;
   - the third blend mode (`0x898`, or whichever of the three is unobserved);
   - the mapping of modes 0/1/2 to blends A and B.

   Read them in `50720`'s base states and the GX table creator (`14007E130` writes
   `0x888..0x898`), or capture a decal of each mode.
3. **RT0 = SpecularAcc.** The decal multiplies it by (1 − a) before the env-map regions. With
   the native clear order (cleared, then regions), this is invisible. Keep the binding anyway:
   it is the original contract.
4. **`141778230`** (normal-threshold angles) is taken as a cosine (INFERRED). Check against the
   engine's `cbDecalParam` (D3).
5. **Pre-pass** (`manager+0x511`, off by default): its stencil states (ref 0x20) and the
   `Decal_PrePass` draw are not specified. No capture shows them.
6. **Stencil bit 0 condition:** entity kinds 3 and 4 are INFERRED to be map and object parts.
   Check with D1.

## Exports

**Decompiles** to `decompiled/` (`DS3-1152-F-<RVA>`):

- `00051AA0`: the manager's draw;
- `0019B690`: submit;
- `00198910`: per-decal draw;
- `00050720`: GPU resources and states;
- `00051320`: programme lookup;
- `00052790`, `000527F0`, `00052840`: depth-stencil selection;
- `00052890`: cube draw;
- `0005A120`: the receiver bit.

## Kind-1 requests owned natively (I-0066, 2026-10-13; closes risk 1's Firelink stop)

Observation. At Firelink the native game ended about 40 s in on `native_decal_cpu_query_family_not_owned` (Native1508-1512,
1534, 1540, 1547-1553): a request of kind 1 from `140051BDB`, the return address inside `FUN_140051BC0`, a thin wrapper that
calls the request entry (`vt+8`, `51BF0`) with the callback table `3D20450` (the same default predicate `50120` as kind 0).
`51BF0` (decompiled 2026-10-13) creates kind 0 through `19B200` and kind 1 inline: a 0x140-byte object, vtable `3D5B978`,
`+0x18..+0x12F` cleared, `+0x130`/`+0x138` zero, then `vt+0x10` = `19BA40`, which validates the descriptor's floats and copies
its 0x118 bytes to `+0x18` and the handle to `+0x138` (pure CPU, no GX access); on success the same list insertion, bucket and
synchronous world query as kind 0. The other slots of `3D5B978` are `19B0E0`, `19BD20` and `19B0A0..19B0D0`, each a bare
`RET 0`, and `19BA10` the destructor: a kind-1 request never reaches a scene consumer.

Change. `RequestNativeDecalCpu` owns kind 1 through the same scoped original call: the result contract accepts vtable
`3D5B978` for kind 1 (no `+0x140` prepared description; the intent records `kind = 1`), and the registry retirement accepts
those objects with `+0x130 = 0`. Native1558 (Thomas's save, Firelink, 200 s): 25 requests completed, no refusal, 9,701 frames,
0 debug errors, clean close. What the kind-1 requests are for in the game is not established (their draws, if any, are not
in the scene slots).

Review (fresh Opus, 2026-10-13): nothing blocking; checked against 51860, 52A80, 19B1B0, 19B5F0, 50130 and 19BDB0. That the
Firelink requests of Native1558 included kind 1 is **inferred** (the completion row had no kind then; the refusal of Native1508
came right after handle 65540 and 1558 completes 65541 at the same point); the completion row now carries `kind` and the
registry retirement row a `kind1_requests` count. For a future native decal draw: kind-1 objects still count in 51AA0's order
index, its emissive test reads `+0x80`/`+0xFC` on every object (descriptor bytes `+0x68`/`+0xE4` for kind 1), and their intent
has no prepared description; an encoder filters on `intent.kind` but keeps counting them (the flow of section 2 assumes only
`3D5B8D8` objects).
