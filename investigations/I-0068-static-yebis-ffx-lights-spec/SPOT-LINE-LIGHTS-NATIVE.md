# I-0068 part U: spot, shadowed and line lights, ready to code

Request (coordinator, 2026-10-10): chain on the coverage matrix's families without a spec. This
note covers row 22 (spot and shadowed point/spot lights) and row 23 (line lights).

Labels: STATIC, OBSERVED, INFERRED.

**Evidence:**

- I-0063's `drawn-families.tsv` (which light programmes each zone draws);
- the host-era producers already on `main` (`native/world/`), proved against live frames in
  I-0045, I-0047, I-0049 and I-0063;
- the decompiles exported here.

## 0. Native status (`main`)

| Part | Native | Proof |
|---|---|---|
| point lights (items from `EFF50`/`212810`, fade `1F1C60`, tile binning `234E40`, `GXLightAcc_DPointLightN`/`LegacyDPointLightN`) | **DONE** (Native1054–1055) | `native/frame/game_bootstrap.cpp:2404`, `:2460` |
| spot lights (class `3D4E8E8`) | **ABSENT**: counted and skipped | `native/frame/game_bootstrap.cpp:2436` (`++spots; continue`) |
| shadowed point and spot lights | **ABSENT** (no local shadow maps) | `native/frame/game_bootstrap.cpp:2412` |
| line lights | **ABSENT** | — |
| host-era spot producers (reusable code) | `cbLightAccLightParam` and `cbLightAccShadowInfo` for spot batches, the organise buffer records, hull binning | `native/world/light_producers.cpp:96`, `:122`, `:188`; `native/world/live_host.cpp:1459`; `native/world/light_tiles.h:29` |

## 1. What the original draws (OBSERVED, I-0063 inventory)

| Programme | Zones (frames drawn) |
|---|---|
| `GXLightAcc_DSpotLight1` | m31, m37 |
| `GXLightAcc_DSpotLight2..4` | m37 |
| `GXLightAcc_DSpotLightShadowed1` | m31, m37 |
| `GXLightAcc_DPointLightShadowed1` | m31, m37, m40 (m40 listing: in the point-light sequence) |
| `GXLightAcc_DPointLightShadowed2` | m37 |
| `GXLightAcc_DLineLight1..4`, `SpotLightMask*` | **never drawn** in any retained capture |

**The host's light classes** (`live_host.cpp:1490`), by item type:

| Type | Class | Batch size | Writer |
|---|---|---|---|
| 0x65 | 0, point | 12 | 1A1DA0 |
| 0x66 | 1, shadowed point | 4 | — |
| 0x6C | 2, legacy point | 12 | 1A1DA0 |
| 0x6D | 3, shadowed legacy | — | — |
| 0x67 / 0x6E | 4, spot | **4** | **1A6690** |
| 0x68 / 0x6F | 5, shadowed spot | 4 | 1A6690 |
| 0x6B | 6, line | — | — |

Class 3 and the line class were "not met" on the host frames.

The host reproduced m31 and m37 spot lighting within the K-0426 count (I-0063). The
programmes and constants are therefore known; what production native lacks is the **item
producer** and the **draw**.

## 2. The spot chain (STATIC)

```
FCE30 (scene task) → 103510 → 10A220: for each kept light (+0xB0 set, +0xA0 & 1):
  spot class 3D4E8E8, vtable +0x30 = F1010(light, recorder, ...):
    transform = light.vt+0x28; fade = 1F1C60(light, recorder+0x130, transform, light+0x1E4)   // same fade as points
    if fade > 0: item = frame alloc 0x280 bytes (recorder+0x4B0)+0x38; 2136B0(item, light, recorder, fade)
  111820 sorts the items; 1A0080 writes the organise buffers (spot records via 214710);
  tiles: 1A1DA0 per 128×128 tile; spot items binned by their hull (214310 plane set);
  spot batches: 1A6690(pass, view, items, programme table by batch size), called by 1A1DA0's tile loop
```

**`2136B0`, the spot item (0x280 bytes, vtable `3D61ED0`).** It is 3005 bytes of math: the
cone's view-space frame, its projection, the bounding points and the fade. Production native
must **transcribe it** as it transcribed `212810` for points (Native1055). The decompile is
exported.

The fields its consumers read (all STATIC, cross-checked with the host producers):

| Item field | Consumer | Use |
|---|---|---|
| `+0x1C` type | 1A0080, binning | 0x67/0x6E plain, 0x68/0x6F shadowed |
| `+0x20`, `+0x24` | 1A6690 | near and far depth of the item; the batch keeps min/max for its depth bounds |
| `+0x30..+0x60`, `+0xE4/+0xE8`, `+0x60..+0xA0`, `+0x1C9` | `2144C0` (vtable +0x10) → `cbLightAccLightParam` 8 rows; `214710` → 128-byte organise record | position/direction/colour rows, cone, matrix; `+0x1C9` clears the specular weight |
| `+0xA0` (matrix), `+0x278` (texture) | `2144C0` sink slots 3/4 | projected mask texture (`SpotLightMask*`; not witnessed) |
| `+0xE0`, `+0x1C0` (shadow map), `+0x264`, `+0x268..+0x274` (atlas rect) | `2144C0`, `cbLightAccShadowInfo` | shadowed spots: shadow index, map, and atlas rectangle `{dx, −dy/2, x0/W + dx, y0/H + dy/2}` |
| `+0xF0` (points), `+0xF8` (count ≤ 8), `+0x100` (matrix) | `214310`, host binning | the hull the tile binning tests (not the `+0x70` sphere, which can be negative) |

**`1A6690`, the spot batch draw** (STATIC).

**Its caller.** The tile loop of `1A1DA0` (lines 945–975 of its decompile) draws in order:

1. the point batches (`1A5B50`, `1A55B0`);
2. **`1A6690` twice**:
   - the plain spots, with the programme table `pass+0x2C0` (`+0x118` in the other mode);
   - the shadowed spots, with the table `+0x2E0` (`+0x138`);
3. `1A60F0`, with the table `+0x188` (`+0x168`). INFERRED: the line lights.

**Inside `1A6690`**, over the tile's items, in groups of **up to 4**:

1. **Programmes:** the PS is `table[n−1]`, indexed by the group size `n`: `DSpotLightN` or
   `DSpotLightShadowedN`. The VS is `pass+0x10`, the input layout `pass+8`.
   `pass+0x358[n−1]` and `pass+0x3B8[n−1]` go to the constant sink (INFERRED: the per-size
   constant layouts).
2. **Constants:** each item's `vt+0x10` (`2144C0`) appends its rows into the sink, which
   `1A7470` uploads.
3. **Depth-stencil:** GX state `[[GXRS]+0x2A0]+0x1E8`, stencil ref 0xFF.
4. **Depth bounds** = the group's min near / max far, projected by the view (`141920F30`).
   - INFERRED: the vendor depth-bounds test.
   - D3D12: `OMSetDepthBounds`, when `D3D12_FEATURE_D3D12_OPTIONS2.DepthBoundsTestSupported`;
     otherwise skip it. It is an optimisation only, because the programmes test the cone.
5. PS **b4** = `[pass+0x348]`.
6. **Draw:** `7F820([GXRS]+0x4DD8, view+0x460, 0)`. `1A1DA0` reads the same `+0x4DD8`
   object, so INFERRED: the tile quad of the point batches.

**Shadowed point lights** (`DPointLightShadowedN`, class 1) use the point path with batches of
4 and a shadow map per item. **Shadowed lights of both kinds** need the local-light shadow
atlas (4096² R16 tiles) of SHADOWS-NATIVE §1.5.

## 3. Ready-to-code order

| # | Step | Native |
|---|---|---|
| 1 | **Spot items** | in `CollectNativePointLights`, replace `++spots; continue` (`:2436`) by the `F1010` path: fade (`1F1C60`, already native), then a native transcription of `2136B0` into a 0x280-byte item |
| 2 | **Organise records** | spot records into the spot buffer (the 128-byte layout, `light_producers.cpp:200`); upload order line, spot, point (`pc+0x460/+0x458/+0x450`) |
| 3 | **Binning** | the hull test (`+0xF0/+0xF8/+0x100`, `light_tiles` in `native/world`), merged into the existing `234E40` tile lists |
| 4 | **Draws** | per tile, after the point batches: groups of ≤ 4 spots → `GXLightAcc_DSpotLightN`; `cbLightAccLightParam` rows from `SpotLights` (`light_producers.cpp:101`); the same tile quad, stencil and blend as the point batches; depth bounds when supported |
| 5 | **Shadowed** (after SHADOWS-NATIVE §1.5) | `DSpotLightShadowedN` / `DPointLightShadowedN`, batches of 4, the atlas rectangle row and `cbLightAccShadowInfo` (`light_producers.cpp:122`) |
| 6 | **Forward** | the spot buffer `g_SpotLightBuffer` (t62) and the light indices also feed the forward programmes. Fill them from the same records (COVERAGE-MATRIX row 28: currently empty) |

## 4. Line lights (row 23)

- **Never drawn in any retained capture**; no witness zone exercises them.
- The organise record is STATIC (`1A0080`, type 107): 80 bytes =
  {item `+0x20..+0x60`, `+0xC0`, `+0xC4`, `[143D7EE7C]`, `+0xA0`}. The host leaves it
  unproduced (`light_producers.cpp:205`).
- The programmes `GXLightAcc_DLineLight1..4` exist. INFERRED: their batch draw is `1A60F0`,
  the third light call of the `1A1DA0` tile loop.
- **Specify the line-light item producer only once a zone that draws them is found.** Until
  then, the native line buffer stays empty, as the original's is in every witness frame.

## 5. Checks

| # | Check | Pass |
|---|---|---|
| L1 | m31, m37 spot count | the native spot items per frame equal the original's (host logs / a fresh capture), same types |
| L2 | items | each transcribed item's consumer fields (§2 table) bit-identical to the engine's `2136B0` output for the same light and camera (a read-only witness of the frame item) |
| L3 | draws | per tile, the same batch sizes (`DSpotLight1..4`) as the original |
| L4 | image | the lit HDR at m31/m37 within the K-0426 count, as the host achieved |
| L5 | shadowed | after the shadow atlas: `DSpotLightShadowed1` and `DPointLightShadowed1/2` at m31, m37, m40 |

## 6. Risks

1. **`2136B0` is long** (3005 bytes, SIMD-heavy). Transcribe it with L2 as the gate. Without
   the old recorder, any field read from `recorder+…` must come from the native view, as for
   `212810`.
2. **The programme tables.** `pass+0x2C0/+0x2E0` (and the other mode's `+0x118/+0x138`) are
   the plain/shadowed spot tables by position in `1A1DA0`. The mode flag is the tile group's
   `+0x20` byte. Read the pass constructor that fills them, together with `+0x358/+0x3B8`,
   before coding the selection.
3. **Depth bounds** are optional; without them the draws stay correct but slower.
