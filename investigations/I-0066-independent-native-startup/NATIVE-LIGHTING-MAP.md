# Native deferred lighting: map of the withholding-era host chain (1047)

Status: planning map for the next I-0066 family, after the first native world
draw (Native1045, G-buffer material colour only). Sources: `native/world/live_host.cpp`
(`compose::Composer`), `native/world/light_producers.*`, `light_tiles.*`,
`native/world/world_adapter.cpp` (`writer_specs`), ledger K-0407, K-0421..K-0429,
investigations I-0044..I-0050, I-0063, I-0064. Compiled from a read-only survey;
line references are to those files at commit `32909301`.

## What the old host did

The host did not author lighting passes: it replayed captured D3D11 events with the
game's own DXBC (template frame A for fixed passes, current frame B for states,
bindings and variable passes, aligned by programme name), and recomputed natively
only the constant blocks and the light buffers. The window runs from the first
`DepthScale2x2` to the last `GXLightComposite`.

A native implementation therefore keeps the producers and programmes and replaces
the capture-only parts (event order, bindings, viewport/scissor, render-target ids,
pooled shadow blocks, resource sizes, snapshot pairing) with an owned pass schedule
fed by live engine CPU state.

## Passes in order (K-0407)

| # | Pass (programme) | Kind | Constant families (writer RVA) |
|---|---|---|---|
| 0 | all | – | `cbSceneParam` 13AA60 |
| 1 | `DepthScale2x2*` downsample | draw | `cbSampleOffset` 1CB9D0 |
| 2 | SSAO, SSAO2 reprojection | draw | `cbSSAOParam` 1B33D0/1B38D0, kernel 1B33D0, `cbSSAOParam2` 1B26E0 |
| 3 | screen-space sun shadow + blur | draw | `cbPerScene` 1D6D20, `cbBlurParam` 1D7850, `cbCompositeShadowMask` 10CC20 |
| 4 | `GXEnvMapRegion*` per box | draw | `CB_ENVMAP_REGION` 675B0 |
| 5 | `GXLightDeferredAO` | draw | `cbLightAccParam` 10ED10 |
| 6 | sun `GXLightAcc_DDirLightShadowed1` | draw | `cbLightAccParam` 19F640, `cbLight`/`cbFog` 137910 (builders 1496D0/14A2D0/14E900), `cbLightAccVtxParam` 19F4D0/19FB20 |
| 7 | `GXLightOrganize` | compute | `cbLightAccParam` 1A0080; line/spot/point light buffers |
| 8 | per-tile point/spot `GXLightAcc_*N` (128×128 scissor) | draw | `cbLightAccVtxParam`, `cbLightAccLightParam`, `cbLightAccShadowInfo`, `cbLightAccParam` 1A1DA0; tiling 234E40 |
| 9 | `GXLightCmbAlbedo` | draw | none identified |
| 10 | SSS | draw | `cbSSS` 1141B0 |
| 11 | `GXLightComposite` → lit HDR | draw | none identified |

## Inputs

All producer inputs are engine CPU memory (`writer_specs`, `.rdata`, game-data
banks), but they were captured at the writer's entry. The native port must reach
the same argument objects outside the calls: the pass object and its view
`[pass+0x418]`, the 1A0080 light list, 137910's entry list, the shadow state, the
SSS profile vector `[0x1449622F8]`, the camera `[G+0x4DE8]`. Native owners already
built: root layer `4961B50` (D-0160), FX world and light registry (956), scene
context, camera inputs.

## Suggested native order

1. Sun + ambient first (passes 5, 6, 11, 9) with `cbSceneParam`/`cbLightAccParam`/
   `cbLight`/`cbFog`: the smallest set giving a lit image.
2. Point/spot lights (7, 8) from the native light registry with the tile binning
   transcription (`light_tiles`).
3. SSAO, sun shadow, env-map regions, SSS.

Uncertain: which pass reads 1072C0, 1D8050, 764D0; whether `GXLightCmbAlbedo` and
`GXLightComposite` have engine-written constants; exact per-pass SRV/RTV identities
(capture-only so far). Absent from `decompiled/`: 1A6690, 1A60F0, 13A580, 67070,
88A10, 1B5720, 1B6B80, 23A0C0, 234E40, 2442D0, 244520, 2144C0, 214710, 214310,
10C7E0, 113D80, 149CB0, 14AB20, 148E30, 149040, 66FA0, 65D60, 102A80, 22BF70,
1A7470, 1A71B0, 18AF40, 839F0.

## Native implementation (1049–1050)

Programme resource roles, read from the DXBC (offline `fxc /dumpbin` of the I-0063
programme cache; licensed bytes stay outside the repository): t0 GBuffer0 (normal,
`g_LightPrepassAttrTexture`), t1 depth, t3 GBuffer1, t4 GBuffer2 (albedo), t5
GBuffer3 (specular, `.w` × 255 = light/fog set index), t6 irradiance (GBuffer4,
seeded by the Gbuf programmes with ambient SH or lightmap), t7 specular
accumulation, t8 emissive (GBuffer5), t17 `gLitTextureFlat` (light-count debug
view only), t32 screen-space shadow/AO mask; s7 point clamp, s8 linear wrap.
`GXLightAcc` (VS) takes POSITION float4 + TEXCOORD float2 and `cbLightAccVtxParam.x`
as z. `GXLightDeferredAO` = (t6, t7) × t32.x·t32.y; `GXLightAcc_DDirLight*` adds the
sun pair; `GXLightCmbAlbedo` = t4 × t6; `GXLightComposite` = t6 + t7 + t8, then
fog by `cbFog` set, alpha from t4.

Native chain (Native1050, lit image, 0 debug errors): DeferredAO → DDirLight1
(additive) → CmbAlbedo → Composite into owned targets, shown by the plain sRGB
composition. Programmes from the engine's loaded repositories (VS `4792D58`,
PS `4792BF8`). `cbLightAccParam` from the camera (inverse projection, camera
pose, projection, screen size). `cbLight`/`cbFog` by the engine's own CPU
builders `1496D0`/`14A2D0`/`14E900`/`14E5D0` over lookups `32770`/`38E70` in the
banks `[[root+41F8+98]+20/+28]`, as `137910` without its upload; the view argument
is the camera output record (pose +0, lens +50, W +5C, H +60).

Entries: `[root+41F8]+88/+90` stay empty natively (count 0, Native1049). The old
renderer's `24740` fills them with `137EF0`, deduplicating eight per-object source
lists (`+8..+40`, counts `+48..`, reset per frame by `23790`); objects keep
`{list, index}` at state `+F0` (`5C0E0`), read back by `5B3D0` as instance envData
for state types 6/7 only (0 otherwise, so deferred pixels use set 0). The native
equivalent runs over the selected models in draw order with the group key block
{`+F8` light a, `+100` light b, `+108` light t, `+110` fog a, `+118` fog b, `+120` fog t}
(the 0x30 entry and `5A120`'s `+338..` layout). The fog offsets are inferred from
that layout; the 1050 witness did not log (sampling), to be confirmed.

Explicit open policy at 1050: neutral t32 (white), no environment specular
(black t7), unshadowed `DDirLight1` (the engine selects `DDirLightShadowed1[_Cloud]`),
no point/spot lights, no tone map/bloom (plain sRGB of the lit HDR).
Next: tone map chain, screen-space shadow (cascades), SSAO, env-map regions,
point/spot lights (`GXLightOrganize` + per-tile passes).

## Tone map chain (read 1050, not implemented)

The scene context's tone map pass object (`context+0x3060`, class vtable
`143D5DE88`: execute `1C48D0`, destructor `1C3480`; constructed by `1C36D0` from
`85540`, which loads its programmes by name from the table `14473E3A8`: 2 VS,
24 PS, 2 CS) is recorded by `1C4730` (from `FB890`) with a parameter block copied
from the pass object `+0x20..+0x8F` and a computed `+0x40` word. `1C48D0` builds
`cbToneMap` (176 bytes) from that block: inverse scene scale, `g_ToneMapScaler`
(+4..+7), `g_ToneMapParam` (+9, +A), `g_ToneMapSceneLumScale` = powf(.., p0) / p1,
`g_AdaptParam` {+C, +20·+D, +20·+E}, `g_AdaptCenterWeight` {+F, W/H·+F},
`g_BrightPassThreshold` (+13), `g_GlareLuminance` (+12), `g_BloomBoostColor`
(constant `1439F24B0`, +15), `g_vBloomFinalColor` ([pass+6C8]·+14 / Σ+17^k),
`g_vBloomScaleParam` (+17); then sub-passes `1C69B0` (luminance chain),
`1C7410`, `1C6600` (bloom, when enabled), `1C66A0`, `1C8700` (LUT quads).
`ToneMap_TabledToneMapLUTPS` = LUT3D(table(x/(x+0.2)) · scaler) with
x = scene · SceneLumScale + glare · GlareLuminance; the table comes from
`WriteFilmic/SensitometricToneMappingTablePS`, the 16³ colour-grading LUT from
the `*LUTCS` programmes. Inputs still to trace: who fills the pass block (GPARAM
ToneMap A/B keys `context+3FE0/+4048`), the adaptation state, and the LUT sources.

## Point/spot lights: recorded chain (read 1050)

`FCE30` (scene draw task) walks the light container `[[root]+0x41D0]` (the
`+0x10` member of the `+0x41C0` lighting object; vector `+0x10..+0x18`), keeping
lights with `+0xB0` set and `+0xA0 & category` non-zero through a local visitor
whose vector (`+0x20..+0x28`) becomes `103510`'s `param_2[7]`. `103510` passes
it to `10A220`, which for each candidate calls the light's virtual `+0x30`
(recorder, settings = the SettingInfo A/B blend of `[root+0x4610]` records
`+0x68..+0x74` by `context+0x3FD8/+0x4040`, ratio `+0x40A8`, and the shadow flag
of `[root+0x5360]+0x40`) to obtain its frame item (null: culled), sorts the
items (`111820`, key `1400FF3B0`), then records `1A0080` (organize buffers, items
by type: point 101/102/108/109, spot 103-106/110-113, line 107; its tail calls `1A36B0`, the organize dispatch, I-0107), `1A0AE0`
(the spot-light mask pre-pass `GXLightAcc_SpotLightMask*`, corrected by I-0107), and, per tile
group of the grid `[14473D544]` × `[14473D548]` (tile size `[[root+4DE8]+8]+0x4C4/
+0x4C8`), `1A1DA0` (point-light draws, binning `234E40`). The light objects'
frame update (virtual `+0x10`, from `499C0`) is not run natively (499C0 omitted).
Open: the light classes' `+0x30` item construction (it allocates from the
recorder's frame memory) and its CPU inputs.

## Native point lights (1051–1055)

Witnesses (Native1051–1053, in game at m40): the container `[[root]+0x41D0]` holds
44 map lights natively (`+0xB0` 1, `+0xA0` 1): point class vtable `3D4E808`
(`+0x10` `EFDA0`, `+0x18` `EFF10`, `+0x30` `EFF50`) and spot class `3D4E8E8`
(`+0x30` `F1010` → `2136B0`). The per-object light keys are `group+0xF8/+0x100`
(t `+0x108`) and the fog keys `group+0x110/+0x118` (t `+0x120`), all
`{140000000, id}` at m40 (confirms the 1050 inference). The scene context's
draw-param keys (ToneMap `+0x3FE0/+0x4048` = `{140000000, 0}`) are applied in game
through the stage's `+0x500` holder by the native `E006A0` update.

Native1055 (0 debug errors): 38 point lights → items transcribed from `EFF50`/
`212810` (view-space position and `1/r²`, colours `c²·k·scale·fade`, specular,
`+0x98/+0x9C`, sphere, near z; legacy when `+0xAC == 1`), the `1F1C60` fade with
the SettingInfo blend (`15E920` on `[root+0x4610]`: {10, 10, 130, 1} at m40) — its
plane taken as view depth (inferred) —, `234E40` binning (I-0047) into 60 tiles of
128×128, 60 batches of `GXLightAcc_DPointLightN`/`LegacyDPointLightN` added into
the diffuse/specular pair. Open: the 6 spot lights (`2136B0` items, hull binning
already transcribed), shadowed lights, the `111820` item order, SSAO (t33 white).

## Sun source: the area's own gparam binding (Reference1537-1539, Native1540-1542, 2026-10-13)

Observation. `reference_gparam_bind` (FUN_140049D00 observed in the original, `shadow-observe.flag`) and
`native_gparam_scene_binding_source` (the native BindNativeGparamScene) record every binding of an area key:

| Area | Original (caller, bank, weight, transform) | Native |
|---|---|---|
| m37 Irithyll (137000000) | D26BEA bank 4 w1 identity; D26D3A bank 10 w1 identity; **D2765B (D275F0) bank 5 w0, rows {-0.9999, 0, 0.0140, 423.2}, {0, 1, 0, -241.8}, {-0.0140, 0, -0.9999, -925.1}** | bank 4 `MapBase`, bank 10 `MapOverride_0000`, identity; no bank 5 |
| m38 (138000000) | same pattern, bank 5 through D2765B with its own resource | bank 4/10 only |
| m31 Undead Settlement (131000000) | banks 4, 10 identity; **D2765B bank 5 w0, rows {-0.9135, 0, -0.4067, 50}, {0, 1, 0, -155}, {0.4067, 0, -0.9135, -580}** | bank 4/10 only |
| m40 Firelink (140000000) | not reached (the title pilot did not continue in Reference1537) | bank 4/10 only |

Interpretation. The area's own file (bank 5: the per-map gparam, a state variant where one exists) is bound by FUN_140D275F0,
which the original runs on admission (D272C0) and the native AdmitGparam skips ("no D275F0 auto-apply", comment above
NativeGparamRecords). Its fifth argument carries the map transform (the yaw that orients the area's directional lights and the
translation that places its fog). FUN_140037340 converts it with 1400362B0 into the per-record quaternion (+0x5C..+0x64) of
every ID the file brings for that area. Without that binding the native records read MapBase or the file named after the area,
unrotated: Irithyll's 180 degrees and m31's default are this gap. Not yet checked: the source of the transform D275F0 passes
(the CSGparamResCap +0x80 that CDEE50 fills from the map's +0x5C0 for the ids of FUN_140CE1DB0's list, or another writer for
the other maps), the weight ramp of bank 5 (bound at weight 0), and how 32770 picks between the banks of one area.

Next (general by mechanism): reproduce D275F0's binding natively for every admitted per-map file (area from its metadata,
transform from the same source as the original, at the same moment), feed it to `Banks` (area -> bound file and transform
per bank, MapRows for the 3x4), then compare `native_light_blocks` sun0 with the original at m31, m37, m40 and the Shebang
zones. Reference runs need the title driven by hand or a fixed pilot: Pilot-TitleContinue does not continue in Reference.

### The binding path, read statically (thread « Source du soleil puis W6 », 2026-10-13)

Observation (exports `decompiled/DS3-1152-F-00D272C0`, `00D275F0`, `00D26A60`, `00D26A70`, `00CE6350`, `00CDEE50` with their
listings):

- **One funnel.** `FUN_140D26A60` (`EDX = [cap+0x7C]`, `XMM2 = XMM1`) and `FUN_140D26A70` (`EDX = D27BA0(7)`, weight in `XMM2`)
  both end in `JMP 140D275F0`; `D272C0` calls it at `D2746D`. So the observed caller `D2765B` (the return inside `D275F0`)
  covers the three. `D275F0` returns unless `[cap+0x70] != -1`, `[cap+0x78] == 0` and the bank is not -1; then it stores the
  bank at `+0x7C`, sets `+0x78`, reads the area through `D27670` and calls `49D00(root+0x41C0, handle, area, bank, cap+0x80,
  weight)` (the weight a float, `[RSP+0x28]`).
- **Admission binds only some kinds.** `D272C0` calls `D275F0` for a kind other than 0 when `(kind - 6) & ~7 != 0` or kind
  10, with the weight `[143D7EE10]` (0 for kind 10). The area files are **kind 7** (Native1542: the ten m31 files are kind 7,
  bank 5), so admission never binds them.
- **The area controller binds them.** `FUN_140CE6350` keeps ten caps looked up by id (`D25BA0`, `area << 16 | variant`) and,
  for each of its five layers whose index changed, calls `D26A70(cap, layer, weight)` with weight 0 on a fresh selection
  (crossfades go through `D26DD0`, gated on a valid handle too). That is the m31 state variant (I-0063's `m31_00_0010`).
- **The transform is written by the map step.** `FUN_140CDEE50` (the map's gparam step, `[144790FF0]`) creates the map's
  kind-9 file (`D25D60` into `+0x100`); only when its handle is valid (`CMP [RAX+0x70], -1` at `CDF0A1`) does it copy the
  map's 3x4 (`[[+0xE8]+0x80]+0x5C0..+0x5EF`) into the area file's cap (`[[+0xF0]+0x80]+0x80..+0xAF`), when `CE1DB0` accepts
  the id `CE1930` derives from `+0xC0`, call `D26A60(cap, 0)`, and bind the kind-9 file on `D27BA0(9)` for area `+0xD4`
  with the identity (`1439F9720`) and weight 0.
- **Correction to MENUS-MAP 43** (and TONEMAP-SKIN 10 through it): the `CSGparamResCap` `+0x80..+0xAF` is not written only by
  its constructor; `CDEE50` (outside the scanned gparam module) writes the map's transform there.

Interpretation. Natively `D26A70`/`D26A60` reach `D275F0` and stop on the handle (every native file keeps `+0x70 = -1`), so
the area file is never bound. The first attempt (Native1532-1534) took the transform from a different moment and found
the identity.

**Implemented and measured (Native1543-1545, 1547; Reference1546).** `D275F0` is hooked by `BindNativeGparamFile`: the
original with "admitted natively" in place of the handle test (a file not admitted yet is skipped, as the original skips one
not loaded yet), then 49D00's bookkeeping through `RecordNativeGparamBinding` (row `native_gparam_scene_binding_source`, with
kind and weight). `AdmitGparam` applies `D272C0`'s admission rule. `Banks::BindArea` receives, per area, the latest kind-7
binding (file key and transform); every lookup of that area reads the bound file, and `Sun` rotates by `MapRows`, the
`1400362B0` decomposition `MapYaw` already used, **on the transposed rows** (Native1543, rows as stored: the m37 sun 1.6
degrees past the original, twice the rows' 0.8-degree offset from a half turn; `MapYaw` now passes its rows in the stored
form, bit-identical). Reference observer `reference_gparam_file_bind` (`D275F0`, `shadow-observe.flag`).

| Zone | Original (Reference cascade-fit light) | Native before (Native1528/1529) | Native now | Bound file and transform (native = original) |
|---|---|---|---|---|
| m37 Irithyll (Gamma) | (-0.368736, 0.5, -0.783603), Reference1523 | (0.379641, 0.5, 0.778378) | (-0.368736, 0.5, -0.783603), Native1544 | `m37_00_0000`, yaw ~179.2 |
| m31 Village (Beta) | (-0.772244, 0.484809, -0.410609), Reference1522 | default (0.497261, 0.743145, -0.447736) | (-0.772243, 0.484809, -0.410609), Native1545 | `m31_00_0010` (the state variant), yaw ~204 |
| m40 Firelink (Thomas's save) | (0.7472608, 0.5446394, -0.3807485), Reference1546 | — | (0.747261, 0.544639, -0.380748), Native1547 | `m40_00_0000`, yaw 28 (rows bit-equal to Reference1546) |

The native values are printed to six digits (equal to the original at that precision; the m31 x differs in the sixth digit of
the print only). Reference1546 also shows the admission binds (kinds 1-5 at weight 1, RemoBase kind 10 at weight 0, caller
`D27472`) that `AdmitGparam` now reproduces (same names, kinds, banks, weights in Native1543). Each native run: 0 debug errors,
no new refusal (1547's closing `native_decal_cpu_query_family_not_owned` is the known decal gap, also in 1534 and 1540).

Writer of the transform. No native code writes the file's `+0x80`: it already holds the map's rows when `CE6350` binds the
file, so the writer is engine CPU code that runs natively unchanged. `CDEE50`'s gated block (above) is one static writer; a
wrapper that replayed it never ran in these zones (no call at all) and was removed. Open: the fog record's map position
(`Banks::Fog` still reads MapOffset, absent natively; the translation of the bound rows is the candidate, unmeasured), the
crossfade weights (`D26DD0`, gated on the handle) and unbinding on a variant change.

**Review fixes (fresh Opus review, 2026-10-13; no blocking finding).** (1) Unbinding: `CE6350` unbinds the caps its layers
leave through `D26DD0`, gated inline on the handle, so natively a cap stayed bound and refused its next binding (a variant
A -> B -> A would have kept B). `CE6350` is now wrapped by `NativeAreaControllerStep`: after each step, a natively bound cap of
its ten slots that no layer selects is unbound as `D26DD0` does (`+0x7C = -1`, `+0x78 = 0`, binding removed, row
`native_gparam_file_unbinding`). Implemented, not yet exercised live (no variant change in the measured runs; Native1548 m31,
Native1549 m40: no unbinding row, the sun unchanged and equal, 0 debug errors). (2) Several live kind-7 bindings on one area
(the controller's five layers): the latest is taken, a rule **not validated** (every measured area had one); row
`native_gparam_area_layers` when it happens. (3) `Banks::SetAreaBindings` swaps every area's binding under one lock and `Sun`
reads the bound file and its rows in one locked call. (4) Retire erases bindings by receiver. (5) Admission rows carry the
caller `D27472`. Pins added for `D26A60`, `D26A70`, `CE6350`. The transpose in `MapRows` is a measured choice (above), not read
in `37340` or `1400362B0`.
