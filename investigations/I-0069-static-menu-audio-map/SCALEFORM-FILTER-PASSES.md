# Scaleform filter passes in the original: programmes, uniforms, states, order, and the pages that use them (I-0069, Task 13)

STATIC (Ghidra mirror of `DarkSoulsIII.exe` 1.15.2.0, the DXBC read from the PE, and the offline menu movies). Nothing was launched.
Status words: **STATIC** read in code or data, **INFERRED** deduced, **OPEN** not known statically.

**Relation to existing work.** [SCALEFORM-FILTERS-NATIVE.md](../I-0067-static-menus-subsystems-map/SCALEFORM-FILTERS-NATIVE.md) (I-0067) is the ordered native checklist (F1..F10) and the CPU planner (`FUN_141F75440`, `FUN_141F75180`, `FUN_141F778D0`, the pass loop `FUN_141F7B930`). It is **not rewritten here**.
This note adds what that note lacks, as inputs for the pilot's steps F2 to F7 (branch `claude/ui-filters`): (1) the **programme** behind every filter key (vertex shader, pixel shader, DXBC, constant buffer, textures, the math), (2) the **uniform-slot to constant-buffer-field map**, (3) the **states and targets** of the pass loop, (4) **worked pass timelines**, (5) the **census of menu pages by filter parameter set**, with witnesses.
Evidence outside Git: `evidence/I-0069/queries/scaleform-filters/` (disassembly of VS `371`, `373`, `507`, `508`, `509` and PS `1265` to `1323`, the DXBC blobs, `programme-offsets.tsv`, `uniform-slots-ps.txt`, `uniform-slots-vs.txt`, `menu-filter-pages.txt`).

## 0. Verdict

1. **The menus need six pixel programmes and one vertex shader.** The filter planner produces the keys `0x220000` (small combined blur), `0x200000` (large separable blur) and `0x280000` (final shadow or glow pass), each with a `+1` premultiplied twin; these resolve (recipes in `native/frame/primitive_program_catalog.inc`, available in this build) to **VS `507`** and **PS `1271/1272`, `1269/1270`, `1273/1274`** (section 1). Every shadow and glow of the 102 menu movies has composite-source on, no inner and no knockout (menu census), which selects the base key `0x280000` without variant bits (INFERRED from the planner and from the variant shaders; row PL1 confirms the flag value).
   The other keys (colour matrix `0x180000`, bevel `0x2D0000`, gradient glow and bevel `0x288000`, `0x2C4000`, displacement `0x300000`, and the `0x400`, `0x800`, `0x2000` variants) exist in the bank and are listed for completeness; no menu page reaches them.
2. **All filter programmes are in the pinned programme bank the pilot already decodes** (`DecodeOriginalPrimitiveProgramBank`): same descriptor tables `1446F90C0` (VS) and `1446FC330` (PS), same `uniform` and `semantic` tables. Nothing needs extracting again; section 2 gives the field names so the pilot's `PrimitiveUniformSource` can be checked against them.
3. **The existing note's pass loop is confirmed by the shaders**, and one detail it leaves implicit is now explicit: the blur axis is chosen by the texel-size uniform (`texscale`), because the separable shader samples along `(offset, offset) * texscale` and the planner zeroes one component of `texscale`; the small combined shader loops over both axes with the half extents `fsize.xy` (section 1).
4. **A native filtered primitive needs up to three temporary targets** (the retained source for shadows and glows, and two ping-pong targets), plus the depth-stencil surface when masks are nested inside (section 3, 4).
5. **Where this could not be settled statically:** the colour format of the temporary targets, whether the target bounds include blur padding, and the flag bits the loader gives a menu drop shadow (rows PL1 to PL3).

## 1. The programmes (STATIC; `programme-offsets.tsv`, `uniform-slots-*.txt`)

The key is resolved by `FUN_141F81A90` (shader manager, called through the vtable `+0x10` at the start of `FUN_141F778D0`): `FUN_141F89D40(key)` gives the VS index and `FUN_141F85610(key)` the PS index; the descriptors are `1446F90C0 + 8*VS` and `1446FC330 + 8*PS`, payload at `descriptor+0x20`. The pilot's recipes `{key, record, VS, PS}` agree.

**Vertex shader `507`** (1356 bytes of DXBC, `vs_5_0`) is shared by the blur, shadow, glow, bevel and gradient keys: one `vfuniforms` constant buffer of 144 float4 (`cb0[144]`, indexed by the vertex's instance index, 6 float4 per instance), input `v0` (quad corner) and `v1.x` (instance index).
For instance `i`: `o0 = cb0[6i+0]`, `o1 = cb0[6i+1]` (colour-transform add and multiply), `o3.x = dp4(v0, cb0[6i+2])`, `o3.y = dp4(v0, cb0[6i+3])` (position from the 8-float matrix), `o2.x = dp4(v0, cb0[6i+4])`, `o2.y = dp4(v0, cb0[6i+5])` (texture coordinates from the 8-float source rectangle).
Semantic map (`vs507` semantic table): semantic `0x4` (cxform add) -> float4 offset 0, `0x5` (cxform multiply) -> 1, `0xD` (ortho matrix, 8 floats) -> 2, `0x15` (rect to UV, 8 floats) -> 4. The colour matrix and displacement keys use VS `371`/`373` (`semantic 0xD -> 0`, `0x15 -> 2`; no cxform in the VS).
All passes draw the HAL's quad: 6 indices, one instance.

**Pixel shaders** (all `ps_5_0`; constant buffer `Constants` `cb0`; sampler `s0`..; every blur, shadow and glow output ends with the colour transform (PS 1265 and 1321 do not): `rgb *= v1.xyz`, `a`, `rgba *= v1.w`, `o0 = saturate(v0 * rgba.w + rgba)` with `v0` = cxform add and `v1` = cxform multiply; the `+1` twins write premultiplied colour: `o0.rgb = rgb * a`):

| Key | PS (plain / +1) | Constant buffer | Textures and samplers | What it computes |
|---|---|---|---|---|
| `0x220000` small combined blur | `1271` / `1272` | `fsize` (+0), `texscale` (+16) | `t0 tex`, `s0` | box filter over `x in [-fsize.x, fsize.x]`, `y in [-fsize.y, fsize.y]` (step 1): `sum += tex(v2 + (x, y) * texscale.xy)`; result `sum * fsize.w` (weight), then the colour transform |
| `0x200000` large separable blur | `1269` / `1270` | same | same | one-axis box filter: `x in [-fsize.x, fsize.x]`, `sum += tex(v2 + x * texscale.xy)`; the axis is selected by zeroing one component of `texscale` |
| `0x280000` final shadow or glow (base) | `1273` / `1274` | `fsize` (+0), `offset` (+16), `scolor` (+32), `srctexscale` (+48), `texscale` (+64) | `t0 srctex` `s0`, `t1 tex` `s1` | blur of **alpha only** from `tex` sampled at `v2 + (x + offset.x, y + offset.y) * texscale.xy` over the 2D box; `s = saturate(sum * fsize.w * fsize.z)` (`fsize.z` = strength); `shadow = scolor * s * scolor.a`; result **`shadow * (1 - src.a) + src`** with `src = srctex(v2 * srctexscale.xy)`: the shadow is composed **behind** the source |
| `+0x400` | `1277` | same | same | shadow composed **over** the source (INFERRED name "on top"): `scolor * k + src * (1 - k)` with `k = s * scolor.a` |
| `+0x800` | `1275` | same | same | **inner** variant (INFERRED name): `s' = saturate(src.a * strength - blur)`; shadow mixed into the source and masked by `src.a` |
| `+0x2000` | `1279` | same | same | **knockout** (INFERRED name): `shadow * (1 - src.a) + (0, 0, 0, 1e-4 * src.a)`, i.e. the shadow masked by the source, the source colour dropped |
| `+0x2400` | `1283` | same | same | **hide object** (INFERRED name): `scolor * k + (1 - k) * (0, 0, 0, 1e-4 * src.a)`, the shadow alone, no source mask |
| `+0x2800` | `1281` | same | same | inner variant with the source colour dropped |
| `0x288000`, `0x2C4000` (kinds 4, 5) | `1285`..`1296`, `1309`..`1320` | `fsize`, `offset`, `srctexscale`, `texscale` | `t0 gradtex`, `t1 srctex`, `t2 tex` | the same blur with a colour **ramp** texture (gradient glow and gradient bevel) |
| `0x2D0000` (kind 3, bevel) | `1297`..`1308` | adds `scolor2` (+48) | `srctex`, `tex` | bevel (two colours) |
| `0x180000` (kind 8; `0x180020` exists in the bank but the planner does not produce it) | `1265`..`1268` | `cxadd` (+0), `cxmul` (+16, 64 B) | `t0 tex` | colour matrix (VS `371`/`373`) |
| `0x300000..` (kind 10) | `1321`..`1323` | `compx`, `compy`, `mapScale`, `scale` | `t0 maptex`, `t1 tex` | displacement map |

**Menu programme set:** VS `507`; PS `1271`, `1272`, `1269`, `1270`, `1273`, `1274` (the `+1` twins are selected when `HAL+0x214` bit 0 is set, which `FUN_141BD9D90` does when the mapped blend is 3 or 4; in the pass loop the planner runs after `FUN_141BD9D90(HAL, 0x12, 1)`, so the bit reflects FilterBlend, not the primitive's blend, and the keys of all passes are fixed before the top-of-stack blend is applied: INFERRED that the twins are unreachable from `FUN_141F7B930`; the cached path `FUN_141F7A590` plans under the current blend and could reach them).
The shadow base key `0x280000` is used for both DropShadow and Glow (planner modes 1 and 2); only `scolor` and `offset` differ (a glow has a zero offset and the glow colour).

## 2. Uniform slots to constant-buffer fields (STATIC; `uniform-slots-ps.txt`)

`FUN_141F778D0` writes the HAL shader-interface slots below; the programme's uniform table maps each slot to a register of `cb0` (register * 16 = byte offset) and the texture or sampler register:

| HAL slot | PS field (`1271`, `1269`, `1273`) | Content (from `FUN_141F778D0`, `FUN_141F75180`) |
|---|---|---|
| `0x8` | `fsize` (float4) | `{offX, offY, strength, weightScale}`: half extents in texels times the view scale `s`, strength (1.0 except on the last pass), `1 / ceil(2*off+1)` or the product for the 2D case. Separable keys: the half extent is always in `fsize.x` and `fsize.y` is clamped to `1e-12`. On the **last pass with `i != 0`** (any key except `0x200000`) the planner writes `fsize = {1e-12, (by-1)*0.5*s.y, strength, 1/ceil(2*offy+1)}`: a vertical-only blur in the final shadow pass (`FUN_141F75180` lines 42-48) |
| `0x16` | `texscale` | `{1 / texW, 1 / texH}` of the source, one component zeroed for the separable keys |
| `0x14` | `t0 tex` or `t1 tex` | the source texture, sampler index 3 |
| `0xE` | `offset` | shadow offset `field * -0.05 * view scale` per axis (shader adds it to the loop coordinate) |
| `0x10` | `scolor` | unpacked colour (`FUN_141D12410`) |
| `0x12` / `0x13` | `t0 srctex` / `srctexscale` | the retained original source and `{1 / (srcW * texscale.x), 1 / (srcH * texscale.y)}` = `texW / srcW`, `texH / srcH` (1.0 for same-size targets; `FUN_141F778D0` lines 155-158; the shader multiplies the UV by it). Shadow and glow only. SCALEFORM-FILTERS-NATIVE section 3 calls the companion `0x12` "its texel size", which is wrong for `0x13` |
| `0x9` | `t0 gradtex` | colour ramp texture (kinds 4 and 5) |
| `0x11` | `scolor2` | second colour (bevel) |
| VS semantics `0xD`, `0x15`, `0x4`, `0x5` (uniform slot `0x18`) | `vfuniforms` | matrix (8), rect to UV (8), cxform add (4), cxform multiply (4) at float4 offsets 2, 4, 0, 1 of instance 0 |

Notes: the idle axis is encoded in `texscale`, not in `fsize` (confirmed by the shaders: `fsize.y` is not read by the separable programmes). The **sampler index is 3** for `tex`, `srctex` and `gradtex` (the displacement key passes 2, linear/wrap, when `filter+0x30 == 0`, `FUN_141F778D0` lines 230-233; that the byte reaches manager slot 3 is INFERRED, `FUN_141F81EB0` only forwards it to the texture `vt+0x68`): the texture manager creates its four samplers at index `filter * 2 + address` (`FUN_141F82AA0`: filter `0` point or `0x15` linear, address `1` wrap or `3` clamp, MaxLOD unbounded, LOD bias `-0.75`): 0 point/wrap, 1 point/clamp, 2 linear/wrap, so index 3 = **linear, clamp**.

## 3. States and targets of the pass loop (STATIC / INFERRED where marked)

| State | Value | Source |
|---|---|---|
| Blend during the filter loop | **blend 18, `FilterBlend`**: colour ADD `SRC_ALPHA / INV_SRC_ALPHA`, alpha ADD `ONE / INV_SRC_ALPHA` | `FUN_141BD9D90(HAL, 0x12, 1)` at the loop start; table in MENUS-MAP 33 |
| Blend of the last pass | the primitive's own blend at the top of the stack (`FUN_141BDEE90`, default normal), restored afterwards | existing note step 5 |
| Depth-stencil | mode **1** (disabled: depth off, stencil off, colour on) for every pass | `applyDepthStencilMode(1)` |
| Rasterizer | mode 0 (solid, cull none, depth clip on) | `+0x2F8` |
| Samplers | index 3 (linear, clamp) for `tex`, `srctex`, `gradtex` | section 2 |
| Geometry | the HAL quad vertex array (`vt+0x300`), 6 indices (`vt+0x340(6, 1)`), VS `507` | existing note |
| Temporary colour target | size `{w, h}` of the filtered subtree (child bounds), INFERRED 1 mip and bind flags **SRV + RTV (`0x28`)** (`FUN_141F83DC0` uses `0x28` when texture flag `+0x46 & 0x400` is set and the mips from `+0x44`; the target comes from manager `vt+0x30`, `FUN_141F73750`, not read in full), cleared to transparent at every push; format taken from the texture manager (`manager+8 -> +4`), INFERRED `R8G8B8A8_UNORM` (the interface target format) | `FUN_141F73750`, `FUN_141F83DC0` |
| Temporary depth-stencil | `D24_UNORM_S8_UINT` (DXGI `0x2D`), same size, bind flag `0x40`, created only when the HAL asks for it (masks inside the filtered tree) | `FUN_141F83B50` |
| Targets per filtered primitive | kind 0 (blur): the subtree target plus one ping-pong target (2; with `P = 1` only the subtree target); kinds 1 and 2 with `P >= 3`: the subtree target is **retained as `srctex`** and two further targets alternate (3); `P = 1`: the subtree target is both `tex` and `srctex` (1) | `FUN_141F7B930` |
| Viewport and matrix | every `PushRenderTarget` switches the viewport rectangle and the projection block (`HAL+0x1144..`, `HAL[0x4B]`); `PopRenderTarget(4)` restores them | existing note 4b |

## 4. Worked pass timelines (STATIC with the programme table above)

Notation: `T0` = subtree target, `T1`/`T2` = temporary targets, `P` = number of passes, parent = the target below on the stack.

**A. DropShadow 2x2, passes 3, composite (51 placements; Inventory, Status, Equip pages).** Field blur 40 twips: small. `P = 3`, keys `[0x220000, 0x220000, 0x280000]`.
1. Push: create `T0` (w x h), clear transparent, draw the subtree into it.
2. Pass 0: `T0 -> T1`, PS `1271`, `fsize = {0.5, 0.5, 1, 0.25}` at view scale 1 (at scale 1.5 the half extent is 0.75, still two taps per axis, and the weight 1/9) (the pixel size is `ceil(0.05 * 40) = 2`, so the half extent is `(2-1)*0.5 = 0.5` and the weight `1 / (ceil(2*0.5+1))^2 = 1/4`), `texscale = {1/w, 1/h}`; the shader takes the taps at `-0.5` and `+0.5` on each axis.
3. Pass 1: `T1 -> T2`, PS `1271`, same block.
4. Pass 2 (last): into the parent with blend = top of stack: PS `1273`, `tex = T2`, `srctex = T0`, `fsize = {1e-12, 0.5, 1, 0.5}` (the last pass of a combined key runs a vertical-only 2-tap blur, weight 1/2), `offset = (cos, sin) * dist * -0.05 * scale`, `scolor` = shadow colour, `fsize.z = strength`.
5. Release `T1`, `T2`; `T0` follows the primitive.

**B. DropShadow 7x7, passes 3 (24 placements; Esc-menu pages, bonfire, character-creation top).** Blur 140 twips: large (`140 * 140 = 19600 >= 12800`). `P = 6`, keys `[0x200000 x5, 0x280000]`, passes alternate horizontal and vertical (even = horizontal, odd = vertical) between `T1` and `T2` starting from `T0`; the last pass is PS `1273` with `tex` = the last blurred target and `srctex = T0`.

**C. Glow 5x5, passes 3, strength 1.5, colour (64, 92, 255) (35 placements, the HUD).** Small, `P = 3`, `[0x220000, 0x220000, 0x280000]`; last pass `scolor` = the glow colour, `offset = 0`, `fsize.z = 1.5`.

**D. Blur radius r, passes n (90 placements, HUD banners and animations).** Kind 0: `P = max(n, 1)`, every key `0x220000` (all radii are at most 3 px, small), PS `1271`; the last pass composes into the parent with the cxform. A **0 x 0 blur** (15 placements) reduces to `fsize = {0, 0, ...}`: one tap, an identity copy through the cxform and the target switch (it still costs a target).

**E. Single-pass shadows (dist 1 to 10; blur 1x1 to 5x5 p1; 53 placements in all, MENUS-MAP 78).** `P = 1`: only the last pass; PS `1273` with `tex = srctex = T0`, one target.

## 5. Which pages use which filters (STATIC; `menu-filter-pages.txt`, `filter-sets-by-page.txt`, built from `menu-gfx-features.tsv`)

53 of the 102 movies carry filters (279 placements). All drop shadows are opaque black, strength 1 (one exception), no inner, no knockout, composite on, angle 0.8 rad except where noted. "Chain" = the programme chain of section 1 (`A` = PS 1271, `B` = PS 1269, `C` = PS 1273; VS 507 throughout).

| Parameter set | Placements | Chain | Pages (xN = placements when more than one) |
|---|---|---|---|
| Blur (radius 0 to 3 px, p1 to p3) | 90 | `A` per pass (last pass composites) | `01_000_fe` only |
| DropShadow 2x2, dist 2, p3 | 51 | `A, A, C` | 25 pages: `02_010_equiptop` x2, `02_011_equip` x2, `02_020_inventory` x2, `02_050..054`, `02_055_detailstatus_weapon1` x3, `02_057`, `02_058_detailstatus_player_2p` x5, `02_060_kick`, `02_070_status` x4, `03_001_shop` x2, `03_010_levelup` x2, `03_020_spell`, `03_030_comparison_equip_weapon1` x3, `03_032` x2, `03_033` x2, `03_040_enhancedweapon`, `03_041_enhancedweaponselect` x2, `03_043_enhancedweapon_status` x3, `03_050_itembox` x2, `04_040_chrmake_multislider`, `04_060_chrmake_childcommandlist` |
| Glow (64, 92, 255), 5x5, strength 1.5, p3 | 35 | `A, A, C` | `01_000_fe` x35 (the HUD) |
| DropShadow 7x7, dist 2, p3 | 24 | `B, B, B, B, B, C` | 24 pages: `01_031_bloodmessage_top`, `01_040_gesturetop`, `02_010_equiptop`, `02_011_equip`, `02_020_inventory`, `02_040_optionsetting`, `02_070_status`, `02_080_warp`, `02_090_ranking`, `02_200_coliseum`, `03_000_shoptop`, `03_001_shop`, `03_002_bonfire`, `03_010_levelup`, `03_020_spell`, `03_040_enhancedweapon`, `03_050_itembox`, `04_000_chrmake_top`, `04_010_chrmake_commandlist`, `05_010_profileselect`, `win_02_040_optionsetting`, `win_02_042_pc_graphicsetting`, `win_02_043_pc_keyconfiguration`, `win_04_000_chrmake_top` |
| DropShadow 5x5, dist 3, p1 | 20 | `C` | `01_000_fe` x5, `02_000_ingametop` x2, `02_903_nowloading2` x13 |
| DropShadow 5x5, dist 5, p1 | 11 | `C` | `01_034`, `01_035`, `02_030_inventory_commandlist`, `02_031_numselect`, `02_032_sortmenu`, `02_033_iconhelp` x2, `02_040_optionsetting`, `02_200_coliseum`, `win_02_040_optionsetting`, `win_02_045_pc_commandlist` |
| Glow black 3x3, p1 | 6 | `C` | `01_000_fe` x4, `02_000_ingametop` x2 |
| DropShadow 5x5, dist 1, p1 | 6 | `C` | `02_090_ranking` x6 |
| DropShadow 3x3, dist 2, p3 | 4 | `A, A, C` | `02_020_inventory`, `03_001_shop`, `03_040_enhancedweapon`, `03_041_enhancedweaponselect` |
| DropShadow 3x3 dist 2 p1; 2x2 dist 2 p1; 3x3 dist 3 p1; 5x5 dist 2 p1; 4x4 dist 2 p1 | 3, 3, 2, 2, 2 | `C` | `01_000_fe`, `02_000_ingametop`, `02_031_numselect`, `02_032_sortmenu`, `01_001_fe_soul` |
| DropShadow 5x5, angle 0.2, dist 10, p1 | 2 | `C` | `02_080_warp`, `04_010_chrmake_commandlist` |
| DropShadow 5x5, dist 2, p3 | 2 | `A, A, C` | **`05_000_title`** x2 |
| Glow, red (the Flash default colour), blur 0, strength 0 (no glow) | 3 | `C` with `fsize.z = 0` | `01_000_fe` |
| Glow ramp (seven colours, radius 0.7 to 4.3 px, strength 0.2 to 1.3, p1..p2) | 7 | `A (, A), C` | `01_000_fe` |
| One each: DropShadow 1x1 p1 (two, angles 0 and 3.1), 5x5 p3 dist 3, 5x5 p2 dist 3, 5x5 angle 0.5 dist 5 strength 0.75 p3; Glow grey (204, 204, 204) 5x5 p1 | 1 each | `C` or `A, A, C` | `01_000_fe`, `02_903_nowloading2`, `01_060_caption` (p2: `A, C`), `01_040_gesturetop` (grey glow) |
| cacheAsBitmap | 2 | `0x101` textured quad, no filter programme | `04_030_chrmake_colorselect` (and its `win_` twin) |
### Witnesses (page, what to look at, expected chain)

| # | Page and moment | Why | Chain |
|---|---|---|---|
| PW1 | **Title** (`05_000_title`) menu text | the first filtered page the pilot reaches; 2 shadows, 5x5, p3, small | `A, A, C` |
| PW2 | **HUD in game** (`01_000_fe`, High Wall) | the 35 blue glows (p3, strength 1.5), the 5x5 shadows and the banner blur ramps: the most complete filter page | `A, A, C` and `A` |
| PW3 | **Inventory / Equip list text** (`02_010`, `02_020`) | 2x2 p3 (51) and 7x7 p3 (24) in one movie: both the small and the large branch | `A, A, C` and `B x5, C` |
| PW4 | **Esc menu / Options** (`02_040`) | 5x5 dist 5 p1 plus 7x7 p3: the single-pass and the large branch | `C` and `B x5, C` |
| PW5 | **Now loading** (`02_903_nowloading2`) | 13 single-pass shadows on one page: pass count and target pooling under load | `C` |
| PW6 | **Caption** (`01_060_caption`) | p2: the two-pass small case `[0x220000, 0x280000]` | `A, C` |
| PW7 | **Blur ramp**, HUD banner fade-in (YOU DIED) | animated radius 0 to 3 px, p1..p3, identity at radius 0 | `A` |

## 6. Cross-check of the existing note and open points

- **Confirmed by the shaders:** the key families, the `+1` premultiplied twin, the axis selection through `texscale`, the use of `srctex` only in the final pass of kinds 1 to 5, the meaning of `fsize` (`{offX, offY, strength, weightScale}`) and of the uniforms `0xE`, `0x10`, `0x12`, `0x13`, `0x14`, `0x16`.
- **Contradicted in the existing note:** its row L3 says the Title shadows are "2 px"; the data give 5x5 blur, distance 2 (two placements, p3), so only the distance is 2 px (witness PW1 here). Its "texel size" for uniform `0x12` and the glow-ramp count of MENUS-MAP 78 ("eight ... one grey", which double-counts: the ramp has seven colours) are also corrected above.
- **Added:** the identification of the variant bits (section 1), the single vertex shader and its instance layout, the sampler index mapping, the depth-stencil format `D24S8`, the bind flags of the temporary colour target, the target count per filter kind, the page census.
- **Open points.** (1) The colour format of the temporary targets (manager field, INFERRED `R8G8B8A8_UNORM`). (2) Whether `PushFilters` pads the target for the blur extent: the rectangle comes from the child entry (`1446E9BA4` table) and no padding was read; a blur that grows beyond the child bounds would be clipped, as in the original. (3) The flag bits the SWF loader gives a plain drop shadow (`filter+0x18`); the section 1 reading (base key `0x280000` for the menus) depends on them. (4) The `+0x1000` bit appears only for kinds with the ramp textures. (5) The blur threshold `12800` twips^2 is the only branch on size; sizes at the boundary (`7x7` at 140 twips vs `5x5` at 100) are both present in the menus.

### Live and capture rows (read-only)

| Row | Action | Expected |
|---|---|---|
| PL1 | Break once on `FUN_141F75440` for a Title drop shadow and a HUD glow: read `filter+0x18` and the returned keys | keys `0x220000, 0x220000, 0x280000` (flags with no variant bit set) |
| PL2 | Break on `FUN_141F81A90` for those keys: log the VS and PS indices | VS 507; PS 1271, 1273 |
| PL3 | Read the temporary colour target's format and size at the first Title shadow | format of the interface target; size equal to the child bounds |
| PL4 | Native: one Title frame with F2..F7 enabled, compare the shadow to the original | black soft shadow offset down-right by about 1.4 px (dist 2 at 0.8 rad), no halo |
| PL5 | Native: PW3 page, check both branches | small and large chains give the same softness class |
| PL6 | HUD frame: count targets created per frame and their sizes | three for glows with shadows (3 targets), pooled |

## 7. Exports

`decompiled/DS3-1152-F-` + `01F81A90` (programme selection by key), `01F82AA0` (texture manager and the four samplers), `01F83DC0` (colour surface creation), `01F83B50` (depth-stencil surface), `01F83640` (builds the `FUN_141F82300` object that `01F83B50` reads: most likely the depth-stencil surface creator, PLAUSIBLE), `01F82300`, `01F79450`; the others (`01F81A90`, `01F82AA0`, `01F7BD30`, `01F7BFF0`, `01BDCBD0`, `01F79D40`) were already exported by I-0067 commits; only `01F83640`, `01F82300` and `01F79450` are new here. The filter functions `01F75440`, `01F75180`, `01F778D0`, `01F7B930`, `01F7A590`, `01F7AC90` are exported (SCALEFORM-FILTERS-NATIVE lists them; `01F75440` and `01F7AC90` were committed with the fade-plates pass).
