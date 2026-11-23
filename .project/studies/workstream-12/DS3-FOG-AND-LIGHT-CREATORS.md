# DS3 fog, light scattering, volumetrics and light creators (static reading)

**Non-canonical until the study thread re-reads it.** Date 2026-11-22. Sonnet 5.5 subagent, read-only, for workstream 12.
Binary: DarkSoulsIII.exe 1.15.2, sha256 `ef5e07c5...498c` (checked with `get_program_info`; the file `game/DarkSoulsIII.exe` of the
main checkout has the same hash and was used for the raw scans). Tags: OBSERVED (read in code, strings or data, address given),
INFERRED (reasoning, says on what). Nothing was renamed, commented or run; no headless Ghidra; no overlay (encrypted functions are
named as such, none was needed).

## Answers first

1. **Fog** is one CPU record, 0x60 bytes (6 rows, `FC_FogParamArray[set]`), built by `BuildCbFogRecord_14014e900` from the 11-parameter
   `Fog` GPARAM group (Color, heightColor, Dist, heightDist, DistOffset, HeightOffset, heightFogDensity, heightFogIntencity, ToUpper,
   Alpha, Contrast): distance fog `d = sat(E.x - E.x * 2^(-(dist_xz - DistOffset)/Dist))` toward the squared fog colour, then a linear
   height ramp toward the squared height colour; no sun-direction or inscatter term (OBSERVED).
2. The same record feeds every consumer that reads `cbFog` (b6): `GXLightComposite` (deferred, set picked per pixel by GBuffer3.w),
   the `*_Fwd` forward programmes, sky domes, FFX particles; there is **no** separate depth-fog post pass, no fog volume, no sky-fog
   programme in the shader bundles (OBSERVED by programme names). Other fog-like things: underwater fog (`_WatDpt`), YEBIS screen-space
   light shafts, effect light-shaft programmes `GXFfxLightShaft*`.
3. **`LightScattering`** (15 parameters: light X/Y rotation, distance %, light colour, RGB %, scattering direction coefficient,
   Rayleigh, Mie, blend %, ground diffuse colour and %) is a Preetham/Hoffman-style parameter set, **but no function consumes it**:
   its bank is only inserted, erased and looked up (`14003C900`, `14003D080`, `14003D710`), its class `GXLightScattering` has one virtual
   (the destructor), and **all 149 value ids in all 168 dumped GPARAM files hold the default zeros** (Rayleigh 0, Mie 0, g 0). Dead data.
4. `Ls` is the same parameter list under an older group name, present only in `s_template`; the engine registers 26 groups and `Ls`
   is not one of them (INFERRED: ignored).
5. **Volumetrics**: no volume-texture fog, no froxel, no light volume in any shipped shader. A `GXGiVolume` (voxel GI) + 3 `GXRsmScene`
   (reflective shadow maps) + `GXVoxelRenderer` + `GXFlverModelRendererDrawImplVoxel` exist as live-created C++ objects, but the bundles
   contain **no programme** named Rsm, Voxel, GiVolume or Voxelization, so they cannot draw (INFERRED dead pipeline; one enable byte not traced).
6. `GXAOFields` (+ `GXAOFieldsRenderer`, programmes `GXAOFields_3DTex/_Sphere/_Combine_BilateralUpScale`, 69 `*_Aof_Dol` flver variants)
   is real code created in `LightManagerSetup_1400ff630`, but its GPARAM groups hold data only in `mapbase` (I-0140), so effectively inert.
7. **Light creators**: exactly three light classes enter `[[GXRS]+0x41D0]`: point `GXPointLight` (vtable 0x143D4E808, 0x1B0), spot
   `GXSpotLight` (0x143D4E8E8, 0x330), line `GXLineLight` (0x143D4E888, 0x1D0); base `GXLightBase` 0x143D4E9A8. One constructor each
   (vtable `lea` scan, no other site); no area, ambient or capsule class, no debug light creator.
8. Creator families: map BTL bank loader `140018490` (point, spot), FFX point host `1402B2410` -> `140266FC0`, FFX spot host `1402B4080` ->
   `14024ED10` -> `140018380(.,0)`, FFX line host `1402B5BA0` -> `1402670C0` -> `1400F0340`. Held torches, lanterns, body glows, bullets and
   Remo lights are **all FFX hosts** (the effect owner is not stored on the light); only map lights carry a name string at `+0x30`.
9. Point light fields: position xyz `+0x150`, radius = w `+0x15C`, diffuse rgb `+0x70` (squared in the item) x brightness `+0x7C`,
   specular rgb `+0x80` x `+0x8C`, legacy falloff `+0xAC` (item type 0x6C instead of 0x65), shadow `+0xB1` (item shadow byte also needs
   `+0x94 > 0`), flicker object at `+0x170` (words `+0x184/+0x188/+0x18C`, output `+0x19C`), type word `+0xA8` (1 point, 2 spot, 3 line).
10. There is **no** source-radius/size/length field on a point light, no per-light specular scale beyond the specular brightness, and no
    scattering/volumetric scale anywhere in the light classes; the spot has cone words `+0x1D0..+0x1DC`, the line light a segment
    (`+0x150` start, `+0x160` vector, `+0x180` end) and a radius `+0x170`.

## 1. Fog: parameters to shader constants

### 1.1 Builder and record layout (OBSERVED)

`BuildCbFogRecord_14014e900(out, set, view)` (decompiled; the native transcription is `native/world/light_producers.cpp` `FogRecord`,
which matches line by line). Callers (MCP `get_function_calls`): `140038d60` (x2), `140137910` (x2, the cbLight/cbFog entry builder),
`14005a120` (forward draw context), `140263a60` (GPU particle draw context), `140264230` (FFX draw context slot 27).

The set record (`param_2`, floats `p[i]`) is the bank record the GPARAM binder fills. Its field order is given by the registry rows of
`FUN_14014eb50` (11 params, id 1..11 at record+0x10 +0x00/+0x30/+0x10/+0x40/+0x44/+0x48/+0x14/+0x18/+0x1C/+0x20/+0x24) and by the
dump `evidence/I-0066/queries/gparam-all/param_drawparam_default.tsv` (group `Fog`, parameter names):

| p offset | GPARAM `Fog` parameter | type | default (default.gparam) |
|---|---|---|---|
| +0x00 (p[0..3]) | `Color` (rgba) | 4 floats | (0,0,0,1) |
| +0x30 (p[12..15]) | `heightColor` (rgba) | 4 floats | (0,0,0,1) |
| +0x10 (p[4]) | `Dist` | float | 0 |
| +0x40 (p[16]) | `heightDist` | float | 0 |
| +0x44 (p[17]) | `DistOffset` | float | 0 |
| +0x48 (p[18]) | `HeightOffset` | float | 0 |
| +0x14 (p[5]) | `heightFogDensity` | float | 100 |
| +0x18 (p[6]) | `heightFogIntencity` | float | 0.01 |
| +0x1C (p[7]) | `ToUpper` | bool | 1 |
| +0x20 (p[8]) | `Alpha` | float | 1 |
| +0x24 (p[9]) | `Contrast` | float | 0 |
| +0x4C..+0x54 (p[19..21]) | map position (set by the area binding, not GPARAM) | 3 floats | |

(Across the 168 dumped files: 728 value rows per Fog parameter; `ToUpper` 444 x 0 and 284 x 1; 71 distinct `Dist`; `heightFogDensity` 329 x 100.
One stray parameter `heightFogBase` exists only in `s_template` and is not among the 11 registered.)

Decompiled excerpt (key lines of `14014e900`):

```c
fVar7 = (_DAT_143d7ed70 < p[5]) ? p[5] : 0.1;           /* max(heightFogDensity, 0.1) */
if (*(char*)(p+7) == 0) fVar7 = -fVar7;                  /* ToUpper == 0 : sign flip */
fVar7 = 1.0 / fVar7;
if (DAT_144961abd != 0) fVar7 = p[6];                    /* global byte: heightFogIntencity instead */
fVar9 = (p[4] <= 0) ? 0 : 1.0 / p[4];                    /* 1 / Dist */
fVar6 = pow(2.0, (0 - view[0x34]) * fVar7);              /* 2^(-camera_y * f7) */
out[0]=p[3]*p[8]; out[1]=fVar9; out[2]=fVar6*fVar9; out[3]=fVar7;      /* row 0 Param */
out[4..6]=p[0..2]^2; out[7]=p[9];                                       /* row 1 Col: colour squared, Contrast */
out[8]=p[3]*p[8]; out[9]=p[17]; out[10]=fVar9; out[11]=fVar7;           /* row 2 ExtParam */
out[12]=p[15]*p[8]; out[13]=p[20]+p[18]; out[14]=(1/p[16])*fVar7; out[15]=view[0x34];  /* row 3 HeightParam */
out[16..18]=p[12..14]^2; out[19]=p[9]; out[20]=1 (uint)                 /* row 4 HeightCol, row 5 fogType */
```

`DAT_144961abd` is set to 1 by `D573B0` for the whole session (FOG-BYTE-4961ABD.md); so in play `f7 = heightFogIntencity` (`p[6]`), not
`1/density`. The 14E5D0 function (`FUN_14014e5d0`, OBSERVED) blends two records field by field, `out = a + (b - a) * t` for every float word,
and for `t >= 1` copies `b` (the area-transition fade; called from the bank "set key" function `1400380A0` when its blend argument is > 0).

### 1.2 The per-pixel formula (OBSERVED, `GXLightComposite_ppo.asm`, copy in `ds3-code/`)

Fog is applied in the composite from `cbFog` (b6, `FC_FogParamArray[2]`, 6 rows of 16 bytes a set). Set index = GBuffer3.w x 255 (`imul r1.y,r1.y,6`).
Only rows 1..4 are read; row 0 (`Param`) and row 5 (`fogType`) are unused by the composite:

```
d  = sat( E.x - E.x * 2^( -(len_xz(p - cam) - E.y) * E.z ) )        E = row2: (Alpha*Color.a, DistOffset, 1/Dist, f7)
c1 = lerp( c, FogCol.rgb * (1 + FogCol.w * (L - 1)), d )             FogCol = row1: (Color.rgb^2, Contrast); L = dot(c, (0.29891, 0.58661, 0.11448))
h  = sat( sat( (p.y - cam.y + H.w - H.y) * H.z ) * H.x )             H = row3: (Alpha*heightColor.a, mapPos.y + HeightOffset, f7/heightDist, cam.y)
out = lerp( c1, HeightCol.rgb * (1 + HeightCol.w * (L - 1)), h )     HeightCol = row4: (heightColor.rgb^2, Contrast)
```

(`exp` in DXBC is base 2.) The horizontal distance is built from view-space position rotated by `g_CameraMatrix` rows 0 and 2.
`RtForwardFog` in `native/renderer/shaders/rt_material.hlsli:425` is this formula. Height fog is a clamped linear ramp, not an exponential
integral; row 0's `2^(-h f7)/Dist` term (camera-height attenuated density) is built but the composite does not read it. INFERRED: row 0 is read by other
shaders (sky or effect programmes); not verified (shader bytecode of those programmes was not disassembled here).

### 1.3 Is there more than one fog mechanism? (OBSERVED unless tagged)

| Mechanism | Consumers | Evidence |
|---|---|---|
| `cbFog` distance + height fog | `GXLightComposite` (deferred), all `*_Fwd` programmes (`Col_Fwd`, `Fujir_Nolight_*_Fwd`, `MultiBlend*_Fwd`, `WaterBump*_Fwd`, the sky domes), FFX particle programmes (`PS_VFX_*` soft variants read b6), via draw-context setups `14005A120`, `140263A60`, `140264230` | I-0125 dxbc (`STUDY-SKY-PROGRAMMES.md` section 4), `I-0068/EFFECTS-E07-GPU-PARTICLES-NATIVE.md:168`, callers of `14014e900` |
| Underwater fog of water | `WaterBumpTri_*_WatDpt` writes `rgb = FC_WaterFogParam1.rgb`, `a = mask*(1 - 1/(1 + max(0, sceneDepth - waterDepth)*FC_WaterFogParam1.w))` | `I-0068/WATER-NATIVE.md` 2.2 (from the shader listing) |
| Screen-space light shafts (YEBIS `LightShaft` group, 15 params: ShaftLength, ColorScale, AngleX/Y, NoiseMask, DiffractionRing...) | YEBIS post (`YebisLightShaft*` functions, `141750620`), sky mask + 4 radial blurs | `I-0138/STUDY.md` rows 56-59, GPARAM `LightShaft` param list |
| Effect light shafts | FFX programmes `GXFfxLightShaft`, `GXFfxLightShaftDust`, `...Procedual(Mask)`, `...Texture`; `PS_VFX_RayCast*` | `I-0068/queries/fingerprints/programs.tsv` (gxffxshader) |
| Depth-fog post pass, fog volume, sky fog programme | **none**: the 96 `gxrenderershader` and 128 `gxposteffect` programme names contain nothing with Fog/Scatter/Volume | same table, full name lists read |

## 2. `LightScattering` and `Ls`

- OBSERVED group layout (every dump, e.g. `param_drawparam_default.tsv`): Japanese parameter names with the units in the name:
  `光源X軸回転角度[deg]`, `光源Y軸回転角度[deg]` (light X/Y rotation angle), `距離倍率[%]` (distance multiplier, default 100), `光源色R/G/B` (light colour, 0),
  `光源RGB倍率[%]` (light RGB multiplier, 100), `散乱方向係数` (scattering direction coefficient, float 0; the Henyey-Greenstein g slot),
  `レイリー散乱係数` (Rayleigh coefficient, float 0), `ミー散乱係数` (Mie coefficient, float 0), `ブレンド係数[%]` (blend, 0),
  `地上の乱反射光色R/G/B` (ground diffuse-bounce colour, 0), `地上の乱反射光RGB倍率[%]` (its multiplier, 100). The `Ls` group (only `s_template`) has the same list.
- OBSERVED data: in the 168 dumped files all 149 `LightScattering` value ids carry exactly the defaults above; no id has a non-zero Rayleigh, Mie,
  direction coefficient, colour or angle (`cat param_drawparam_*.tsv | awk ...` over group `LightScattering`).
- OBSERVED code: bank `[GXRS+0x41F0]`, registry slot `+0x84` (I-0140). Class `GXLightScattering` (vtable `0x143D52F78`, RTTI) has one vtable
  slot, `14014FA70` (the destructor); the vtable address is `lea`-loaded only at `14003AEEF`, `14003B03E`, `14003C03E`, `14003CC89` (binder
  `14003BC70` and set-key `14003C900`) and the destructor `14014FA76` (raw scan `ds3-code/lea_vtables.txt`). `FUN_14003c900` (decompiled):
  allocates a 0x88-byte `GXLightScattering` per (area, id), copies ten dwords `+8..+0x2c` (the 15 parameters packed as 12 shorts + 3 floats),
  registers the editor object with `FUN_1400957d0`; its only caller is `140CC8180` (the cutscene act, I-0141). `FUN_14003d080` (decompiled)
  erases a key range; `14003d710` is the lookup, called only from `14003c900`.
- Raw scan of the image for any code reading `[reg + 0x41F0]` (`ds3-code/scan_disp.py ... 41F0`, `hits_41F0.txt`): three hits only:
  `140CC81C3` (inside `140CC8180`), `140D27508` and `141BDCBCB`. The two others sit in code Ghidra has no function for: `140D27508` is a jump
  stub `mov rcx,[GXRS]; mov r8d,[rdx+0x7C]; mov edx,[r10+0x64]; mov rcx,[rcx+0x41F0]; jmp 14003D600`, one of a run of identical stubs for
  `+0x41E8`, `+0x41F0`, `+0x42A8` (the same banks the cutscene act writes); `141BDCBCB` is `mov rcx,[rcx+0x41F0]` followed by an ordinary
  function prologue (frame 0x398) - not resolved (see Not reached).
- Conclusion (INFERRED on the above): the bank is a store written by the area binder and the cutscene act and never read by a renderer; and
  even if read, the shipped values are zero. There is no CPU-side formula to give (no coefficient builder found), and no shader constant for it
  in any programme name list. If a native workstream wants Preetham/Hoffman sky scattering it has to author the values.

## 3. Anything volumetric

| Item | Class / RTTI vtable | Created at (lea of vtable) | Registration / creator called | Shader programmes | Status |
|---|---|---|---|---|---|
| Voxel GI volume `GXGiVolume` (0x640 bytes, `GXRS+0x14F0`) and renderer `GXGiVolumeRenderer` (0x1E0) | `0x143D50C38`, `0x143D50178` | ctor `14011F850` (lea `14011F8A1`), renderer init `140116980` (lea `140116990`) | `FUN_1400898D0` (called by `FUN_14008D7F0`; that function is registered as a GX step by the large GX init `140089960`, `lea 14008D0E7`, OBSERVED) allocates both and sets `GXRS+0x4145 = 1` | **none** (no GXGi*, Rsm, Voxel programme in any bundle) | created live; passes need `GiVolume+0x10` (ctor sets 0) and `+0x3E0/+0x3E1/+0x3EC/+0x3ED` (ctor sets `+0x3E0 = 1`, `+0x3EC = 1`, `+0x3E1 = 0`, `+0x3ED = 0`); the byte that turns it on was not traced |
| Reflective shadow map `GXRsmScene` (3 objects, `GXRS+0x14F8..`) | `0x143D5EC40` | ctor `1401DC790`, created 3x in `FUN_1400896C0` with size 0x200 x 0x200 | called per frame from `GXSgShadowSceneImpl_Slot4_1402057d0` -> `FUN_1400FA600` when `GXRS+0x4145 && GXRS+0x14F8` (OBSERVED in the decompile) | none | live code path, no programme to draw |
| `GXVoxelRenderer` | `0x143D62620` | `140116B9C` (inside the GI renderer init) | member of the GI renderer | none | dead with it |
| `GXFlverModelRendererDrawImplVoxel` | `0x143D41D28` | `140057386`, `14005D281`, `14005E093` | flver draw-impl variant | pass-name table at `14473CE60` (suffixes `Fwd`, `Dpt`, `Velo`, `FwdSdw`, `DptA`, `Gbuf`, `FwdSSdw`, `Pnt`, `Gbuf2`, `PntCol`, `PntCube`, `WatDpt`, `SO`, `Aof_Dol`, `Aof`, `FwdDbg`, `Voxelization`, `GBufDbg`, `FwdSdwDbg`, read from memory at `143D488F0`) names `Voxelization` as a draw-pass suffix; **zero** programmes carry it | dead (no programme) |
| `GXAOFields`, `GXAOFieldsRenderer`, `GXAOFieldsFileData` | `0x143D47368`, `0x143D47380`, `0x143D47390` | renderer created in `LightManagerSetup_1400ff630` (lea `1400FF7EB`); also `140061CCC` | live creator; programmes `GXAOFields_3DTex`, `_Sphere`, `_Combine_BilateralUpScale`, `GXAOFields.vpo` and 69 `*_Aof_Dol` flver variants exist | data only in `mapbase` (I-0140 items 7 and 9) | code live, data inert (the gating byte of its use not read) |
| `GXCloudShadow` / `GXCloudShadowParam` | `0x143D530D8` | `140048962`, `140048B01` | cloud-shadow bank; programme `GXLightAcc_DDirLightShadowed1_Cloud` | no file carries the `Cloud Shadow` group (I-0140) | inert data |
| Fog volume, froxel, light volume | no class | RTTI list (10,701 classes) contains no Froxel/FogVolume/LightVolume/Atmosphere; only `GXViewVolume`, `GXTestVolume`, `GXSphereVolume`, `GXPointLightViewVolume`, `GXShadowMapTestVolume` (culling volumes) | | | none |
| `IGPUVolumeTexture@PPFX` | `0x142B674A8` | interface of the PPFX (post-FX library) layer, 20 slots | not followed | | not reached |

Facts for the volumetric plan: the original has **no** light-volume or fog-volume renderer to replicate; its only "volumetric" looks are the
YEBIS screen-space shafts (12 world areas on), authored shaft billboards in effects, and the unauthored LightScattering group.

## 4. Who creates run-time lights

All run-time lights enter one container, `[[GXRS]+0x41D0]` (vector at `+0x10`, pushed with `FUN_14001AE00`). Evidence that the families below are complete:
the vtable `lea` scan finds a single constructor for each light class (`ds3-code/lea_vtables.txt`: point `1400EFCCA`, line `1400F035A`, spot
`1400F09DB`; the second site of each is the destructor) and `get_function_xrefs` gives all callers of the constructors. The only callers of
the push `14001AE00` are `1400180D0` (map path), `140018490` (loader), `140266FC0` (effect point) and `1402670C0` (effect line).

| Family | Creator chain (OBSERVED) | Class | Distinguishing data readable on the light |
|---|---|---|---|
| Map light bank (BTL, "OmniXXX", "Spot_XXX", "動的_XXX") | loader `GXLightManager_LoadBtlLights_140018490` <- `140931F30` (area load, `+0x207C` call) and `140652EB0` (bank name from an id, `FUN_140651450`). Point: `1400EFCB0` called inline in 18490 (new-format records) or via `1400182F0` (old-format records, call site `140019622`, which then calls `RegisterMapLight_1400180D0`); spot: `140018380(mgr, 1)` then `SpotLight_SetCone_1400f10c0` and `FUN_1400f13d0` | point `0x143D4E808`, spot `0x143D4E8E8` | **name string at `+0x30`** (`FUN_140003c40(light+0x30, name)`, non-empty); the four bank words at `+0xBC/+0xC0/+0xC4/+0xC8`; world matrix at `+0x110..+0x14C`; `+0xB1` from record `+0x27`; `+0x94 = 0.8`, `+0x90 = 20.0` or record `+0xA4`; `+0xAC` from record `+0xA0`; flicker words `+0x184/+0x188/+0x18C` (point) or `+0x200/+0x204/+0x208` (spot) from record `+0x74..+0x7C`; category `+0x60/+0x64` from the caller's mask (`param_5`) |
| FFX point light (effect type 0x261 = 609) | `FfxFactory_PointLightHost_14028f200` -> host ctor `1402B2410` (host class 0x120, vtable `143D6A838`) -> `GXPointLight_CreateAndPush_140266fc0(container+0x110)` -> `GXPointLight_Ctor_1400efcb0`; per frame `1402B2E30` updates it | point `0x143D4E808` | name empty; `+0x60 = owner+0x1FC` (create-info, 0x11 or 0x01; bit 0x10 from the FXR record), `+0x64 = owner+0x200`; `+0xA4 = record+0x60`; `+0xAC = record+8` (legacy) ; `+0xB1 = (record+0x30 != 0)`, `+0x94 = record+0x40`; flicker `+0x184..+0x18C` from record `+0x10..+0x2C` when record `+0xC != 0` else (0,0,1); `+0x1A0 = 1` when record `+0x44 != 0` (second colour at `+0x160`); `+0xB0` toggled each frame by the host's visibility test |
| FFX spot light (type 0x2AF8 = 11000) | `FfxFactory_SpotLightHost_14028f290` -> `1402B4080` (0xF0, vtable `143D6A888`) -> `FUN_14024ED10` -> `GXSpotLight_Create_140018380(mgr, 0)` (second argument 0: no lock) -> `SpotLight_Constructor_1400f09b0`; cone set by `EffectLight_Slot_OnLoadOrChange_1402b49d0` -> `SpotLight_SetCone` | spot `0x143D4E8E8` | name empty; same masks as above; not read in detail (host ctor `1402B4080` not decompiled) |
| FFX line light (type 0x2AFA = 11002) | `FfxFactory_LineLightHost_14028f320` -> `1402B5BA0` (0xD0, vtable `143D6A8D8`) -> `FUN_1402670C0` -> `FUN_1400F0340` (alloc 0x1D0; `GXLight_BaseCtor(this, 3)`; pushed `FUN_14001AE00`) | line `0x143D4E888` | `+0xA8 == 3`; the only family of this class; programmes `GXLightAcc_DLineLight1..4` exist (deferred line-light accumulation) |
| Character light (held torch, lantern, flame, enchant glow), object light (candles, braziers, bonfires), bullet / magic light, cutscene (Remo) light | **no separate creator**: each is an SFX effect whose FXR contains a type 0x261 / 0x2AF8 / 0x2AFA action; owners (I-0068 `EFFECT-LIGHTS.md` 4): `ChrLanternSlot_1408755d0`, `ChrWepSfxSlot`, `ChrBurnSlot`, `WorldSfxManImp`, `ObjMultiSfxSlot`, bullet chain `14097CFA0`, `SprjRemoSfxAct`, all through SfxIns creation `140E6DCD0` -> SceneCtrl `14024EFE0` | as above | **not on the light**. The light has no back-pointer to its host or owner. The host holds it (`host+0x28` = light pointer for the point host; host `+8` = the effect instance, `[[host+8]+0x10]+0x40` = the create-info block with `+0x1F8` category bits and `+0x1F0` the scene control). A native collect has to name the family from the effect side (the FFX instance and its FXR id), not from the container |
| Debug light | none found (no other caller of any light constructor) | | |
| Directional / sun / ambient / area | `type 2` bank rows build nothing (`18490` branches only on 0 and 1); the sun comes from the LightSet record; no area, ambient or capsule light class exists | | |

OBSERVED type words: `GXLight_BaseCtor_1400f1a40(this, t)` stores `t` at `+0xA8`: point 1, spot 2, line 3.

## 5. Fields of the light objects (what constructors, loader, item builder and effect host read or write)

Base `GXLight_BaseCtor_1400f1a40` defaults (OBSERVED): `+0x08` refcount 0; `+0x30` name string; `+0x58` byte 1; `+0x60/+0x64` category = `0xFFFFFFEF / 1`;
`+0x70..+0x7C` diffuse and `+0x80..+0x8C` specular = the constant at `1439F24B0`; `+0x90 = +0x94 = +0x98 = +0x9C = 1.0f`; `+0xA0 = 1`; `+0xA4 = 100` (int); `+0xA8` type;
`+0xAC = 0`; `+0xB0 = 1`, `+0xB1 = 0`, `+0xB2 = 1`, `+0xB3 = 0` (the dword at `+0xB0` is `0x10001`); `+0xB4 = 2`; `+0xCC`, `+0xCD`, `+0xCE` = 0.
The loader overwrites `+0x90 = 20.0f` (`0x41A00000`) or the record's `+0xA4`, and `+0x94 = 0.8f` (`0x3F4CCCCD`), only when `+0xCD == 0`
(`+0xCC/+0xCD` look like per-light "driven from elsewhere" locks: every creator and the host update test them before writing; INFERRED, a debug/editor override).
Names of `+0x90`/`+0x94` below are INFERRED from the item builder.

### 5.1 Point light (0x1B0 bytes, `GXPointLight_Ctor_1400efcb0`; item builder virtual `+0x30` = `BuildPointLightFrameItem_140212810`)

| Offset | Meaning | Evidence |
|---|---|---|
| `+0x150..+0x158` | position xyz (loader: from the bank, effect: the effect's world matrix + fractal-noise jitter, host `1402B2E30`) | ctor: `+0x150` from the position vector; item `+0x20..0x28` = view-space position |
| `+0x15C` | **radius** (w lane); `1/r^2` stored in item `+0x2C`; item box `z +- r` | `fVar43 = +0x15C^2`, `divps(1,..)` |
| `+0x160..+0x168` (+`+0x1A0` byte) | second colour/position row (copied from the position at construction); used instead of `+0x150` for item `+0x40..` when `+0x1A0 != 0` | ctor `param_1[0x2c]=param_1[0x2a]`; item builder branch on `+0x1A0` |
| `+0x70..+0x78` rgb, `+0x7C` brightness | **diffuse**: item colour = `rgb^2 * brightness * flicker(+0x19C) * fade` (the item's `.w` = `+0x90`) | item builder; loader: bytes `/255` from record `+0x1D..+0x1F`, brightness record `+0x20` |
| `+0x80..+0x88` rgb, `+0x8C` brightness | **specular** colour: item = `rgb^2 * brightness * flicker * fade`, plus an item scalar mixing luminance weights (`143D7F5F0..`) | item builder; loader record `+0x24..+0x28` |
| `+0x90` (float 20.0 from loader) | carried into item `+0x3C` (diffuse `.w`): INFERRED specular power / falloff exponent | item builder `*(item+0x3C) = *(light+0x90)` |
| `+0x94` (0.8 from loader, effect: record `+0x40` x curve) | **shadow strength**; item byte `+0xA4` (shadow on) = `(+0xB1 != 0) && (+0x94 > 0)` | `if (*(light+0xb1)==0 || +0x94 <= 0) uVar26 = 0` |
| `+0x98`, `+0x9C` | two scalars copied to item `+0x90/+0x94` (loader: record `+0x98/+0x9C`; effect: curves x owner scales); meaning not established | item builder |
| `+0xA4` (int, 100) | screen-size LOD metric: item `+0x10 = int(r * K * (float)+0xA4 / max(depth - r, eps))` | item builder |
| `+0xAC` | **legacy point model** flag: `== 1` gives item type `0x6C` (legacy programmes `GXLightAcc_LegacyDPointLight*`), else `0x65` | `if (*(light+0xac)==1) item[0x1c] = 0x6c` |
| `+0xB0` | enabled / visible (effect host toggles it per frame) | `1402B2E30` writes `+0xB0 = cVar16` |
| `+0xB1` | **shadow flag** (loader: record `+0x27`; effect: `record+0x30 > 0`) | loader, effect ctor |
| `+0xB2`, `+0xB4` | item `+0x14` (byte) and `+0x18` (int); loader record `+0xA3`/`+0xA2` | item builder, loader |
| `+0xB3` | effect only: record `+0x5C != 0` | `1402B2410` |
| `+0xB8` | the light's own distance the native's fade reads (loader record `+0xC4`) | I-0110 5.1 |
| `+0x60 / +0x64` | category masks (loader: caller's mask; effect: create-info `+0x1FC/+0x200`; bit `0x10` = FXR-controlled) | loader, `1402B2410`, `1402B2E30` |
| `+0xBC..+0xC8` | four bank words (record `+0x00..+0x0C`, unnamed); `+0x110..+0x14C` world matrix (map only) | loader |
| `+0x170` object (RNG `+0x174..+0x180`), `+0x184/+0x188/+0x18C`, `+0x190/+0x194/+0x198`, output `+0x19C` | **flicker**: `Light_ComputeFlickerOutput_1402124a0(obj)`: if both range words (obj `+0x14`, `+0x18`) are 0 the output is 1.0; else every cycle a new duration `T = lerp(range) ` from an xorshift128 (`>> 0xb`, `<< 0xb`, `>> 8`), output `= lerp(obj+0x1C, obj+0x20, ((T - |t - T|)/T))`; ctor defaults `+0x18C = 1.0`, `+0x190 = 1.0`, `+0x194 = -1`, `+0x19C = 1.0` | `1400efcb0`, `1402124a0`; the mapping `+0x184/+0x188` = period range, `+0x18C` = minimum multiplier, `+0x190` = maximum is INFERRED from the lerp |

No field for a **source radius, size, length or soft-shadow width** on the point light; no per-light specular scale beyond the specular brightness
(`+0x8C`); nothing volumetric or scattering related (no field read by the item builder or written by any creator).

### 5.2 Spot light (0x330 bytes, `SpotLight_Constructor_1400f09b0`, `SpotLight_SetCone_1400f10c0`)

Shares the base fields above (diffuse/specular at the same offsets, `+0xAC`, `+0xB1`, `+0x98/+0x9C`, `+0x90/+0x94` set by the loader the same way).
Own words: `+0x110..+0x14C` world matrix; `+0x150..+0x18C` projection rows (loader writes sixteen floats from the transformed placement, `+0x230..+0x26C` a copy);
cone `+0x1D0..+0x1DC` set by `SetCone(light, a, b, c, d)` (`+0x1D0 = a` or `b * K`, `+0x1D4 = b`, `+0x1D8 = c`, `+0x1DC = d`; defaults
from constants `143D7ED28`, `143D7EDBC`; when `b, c, d` are not all positive, `+0xB0 = 0` and the angles fall back to 1.0 and 0.017453292 = 1 degree); derived
by `SetCone`: aspect `+0x1E0 = tan(+0x1DC / 2) / tan(+0x1D8 / 2)`, extents `+0x1E4`, `+0x1E8`, perspective matrix `+0x190..+0x1CC` through `BuildPerspectiveProjection_14004ba40`,
eight cone corners at `+0x220` (inline buffer `+0x270`, count `+0x228`, world copy `+0x230`). INFERRED: `+0x1D0/+0x1D4` near/far, `+0x1D8/+0x1DC` vertical/horizontal field of view in radians.
Flicker words for a spot sit at `+0x200..+0x208` (loader) with the output at `+0x218` (I-0106 step 1); the spot's projected texture handle at `+0x328` (loader: record `+0xA8` flag, `FUN_1400F1440`).
The loader writes the same shadow flag `+0xB1` and legacy flag `+0xAC`. Again no area-size, no per-light specular scale, no volumetric scale.
(The spot item builder `GXSpotLight_BuildFrameItem_1402136b0` and the effect spot host constructor `1402B4080` were **not** read here.)

### 5.3 Line light (0x1D0 bytes, `FUN_1400F0340`)

`+0xA8 = 3`; `+0x150..+0x15C` start point (xyz masked, w lane forced), `+0x160..+0x16C` second vector (the segment), `+0x170` a float from the construction argument (**length/radius**, INFERRED
radius of the capsule), `+0x180..+0x18C` end point `= start + segment * constants (143D2600..)`; flicker object at `+0x190` (RNG at `+0x194`), `+0x1AC = 1.0`, `+0x1B0 = 1.0`, `+0x1B4 = -1.0`, `+0x1BC = 1.0` output;
`+0xB1 = 0` unless `+0xCD != 0`. The effect host `1402B5BA0` sets `+0x60 = owner+0x1FC`, `+0xB1 = 0`, sets bit `0x10` from the FXR record, and reads curve values for colour and the two
segment ends (host `+0x48..+0x80`). This is the only class with a physical **extent** in the original.

## Not reached

- The byte(s) that switch the GI volume / RSM passes on (`GXGiVolume+0x10`, `+0x3E1`, `+0x3ED`); the writer of `GXRS+0x4145`'s consumers beyond the two read sites; so "never draws" rests on the missing programmes (INFERRED), not on a traced gate.
- `1400FAE10` (GI volume pass body), `14011E8F0`, `14011EFD0`, `14011F790` not read; the AOFields renderer's gate in `LightManagerSetup_1400ff630` (4,359 bytes) not read.
- The two unresolved code sites for `[reg+0x41F0]`: the stub target `14003D600/14003D610` (a function with `this[+0x1D0]` map iteration, no Ghidra function) and `141BDCBCB` (frame 0x398, no Ghidra function; possibly an obfuscation stub tail). The raw scan covers only code present unencrypted in the file: functions encrypted at rest are invisible to it, so "no other reader" for LightScattering is INFERRED, strengthened by the all-default data and the missing shader constants.
- Shader constant names of the sky, particle and water programmes were not extracted (only the I-0125 dxbc sample, `GXLightComposite`, `Col_Fwd`, water listing were available); the claim "no scattering constant" is by programme names, not by bytecode reflection of all 8,400 programmes.
- Spot item builder `1402136b0`, effect spot host `1402B4080`, FXR light record layouts beyond the words quoted, meaning of `+0x98/+0x9C`, the exact semantics of the `+0x90` 20.0 and of the second colour row `+0x160`.
- Family attribution from a light object alone (character, object, bullet, Remo): not possible from the fields read; needs the FFX host/instance side (`host+0x28`, `[[host+8]+0x10]+0x40`), not read further.
- `IGPUVolumeTexture` (PPFX) not followed.

## MCP readings and scripts

GhidraMCP (Thomas's GUI, DS3 copy): `get_program_info` (sha256 `ef5e07c5...`). `decompile_function_by_address`: `14014e900`, `1400380a0`, `14014eb50`, `14014e5d0`,
`14003c900`, `14003d080`, `1400efcb0`, `1400f1a40`, `1400f0340`, `1400f09b0`, `140018380`, `1400f10c0`, `1400180d0`, `140018490`, `1400182f0`, `140266fc0`,
`1402670c0`, `1402b2410`, `1402b2e30`, `1402b5ba0`, `140212810`, `1402124a0`, `14024ed10`, `140652eb0`, `1400898d0`, `1400896c0`, `14008d7f0`, `1402057d0`, `1400fa600`,
`140120a70`, `140023790`, `14011f850`. `get_function_xrefs` / `get_function_calls`: `14014e900`, `1400efcb0`, `140018380`, `1400f09b0`, `140266fc0`, `1400182f0`, `140018490`,
`14024ed10`, `1402b5ba0`, `1400f0340`, `1402670c0`, `14001ae00`, `1400180d0`, `140120240`, `1401dc790`, `140116980`, `1401dd490`, `1400898d0`, `1400896c0`, `14008d7f0`,
`14003d710`, `14003c900`, `14003bc70`, dtors `1400a2680`, `140229080`, `1400a0bf0`, `1401a99d0`. `get_function_by_address`: `140d27508`, `141bdcbcb` (not functions), `14003d5ff`,
`14008d0e7`, `1400fa68b`, `140120b84`, `140205839`, `140023ecb`, `1400ff7eb`. `read_memory`: `143d1c8c8`, `14473ce60`, `143d488f0` (pass-name table), `140d274e0`, `141bdcba0`, `14003d600`, `14003d080`.
`disassemble_function`: `140089960` (offsets 2930-3000), `1400ff630` (offsets 150-195). `list_strings` (only UTF-16 strings are defined: ASCII GPARAM names are not in the list).

Raw scans (py -I, read-only, outputs in `C:/Users/User/Tools/DS3-Decompilation/study-ws12/ds3-code/`): `scan_lea.py <exe> <vtable VAs>` -> `lea_vtables.txt`;
`scan_disp.py <exe> 41F0` -> `hits_41F0.txt`, `... 4145 / 14F0 / 14F8` -> `disp_gi.txt`; `scan_target.py <exe> 14008d7f0 1400898d0 1400896c0` -> `refs_gi_init.txt`. Exe: `game/DarkSoulsIII.exe` of the main checkout (same sha256). Data read:
`evidence/I-0067/rtti/classes.tsv` + `vslots.tsv` (RTTI of 10,701 classes), `evidence/I-0066/queries/gparam-all/*.tsv` (168 GPARAM dumps), `evidence/I-0068/queries/fingerprints/programs.tsv`
(programme names of the shader bundles), `evidence/I-0125/artifacts/dxbc/GXLightComposite_ppo.asm` (copy in `ds3-code/`). Prior studies cited: I-0140 `STUDY-UNREAD-SETTINGS.md` and `RESULT.md`, I-0110 RESULT 5, I-0106 RESULT, I-0068 `EFFECT-LIGHTS.md`, `FOG-BYTE-4961ABD.md`.
