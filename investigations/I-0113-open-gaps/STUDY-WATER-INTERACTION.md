# STUDY-WATER-INTERACTION: the water interaction (ripples and wakes) in DarkSoulsIII.exe 1.15.2

2026-11-06, Sonnet 5.5 (static decompilation for the open-gaps pilot E). Static, read-only. Sources: GhidraMCP on the DS3 copy
(`DarkSoulsIII.exe` sha256 `ef5e07c5…`, image base `0x140000000`), headless Ghidra 12.1.3 on the mirror `ghidra-mirror-i0066`
(`-readOnly -noanalysis`, hidden, idle priority), raw scans of the image (`game/DarkSoulsIII.exe`, same sha256), `fxc /dumpbin` of the
I-0063 cache programmes, the I-0063 scan tables and the game's GPARAMs (I-0063 cache). Nothing was launched, nothing was changed.

Made by the study thread's `ghidra-research` subagent; non-canonical until the pilot re-reads what a change rests on. Re-checked
by the study thread: `14027BDF0` compares the action's `+0x240` with `0x2968` (10600) in its bytes; `fxr.tsv` (148,615 rows) has no
action 10600; no MTD of `mtd-policy-bits.tsv` carries bit 17; all five water `_Fwd` programmes declare t57 in
`program-resources.tsv`, the three Interaction ones t58 too.

Tags: **VERIFIED** = read in code or data (address and excerpt given); **INFERRED** = deduced (the basis is named); **OPEN** = not reached.

Exports (under `evidence/I-0113/water-interaction/`, "EV/" below): `decomp-core.txt` (the manager, the two pass classes, the
frame recorder `103510`), `decomp-support.txt`, `decomp-57a20.txt`, `decomp-frame-init.txt` (every one is `I0099Decompile` output),
`fwd_*.asm`, `GXWaterInteraction*.asm` (fxc), `gparam-*.tsv`, and the small Python scripts and raw outputs listed in section 8.

## 0. Answers first

1. **Two separate mechanisms share the name "water interaction". Do not merge them.**
   - **A. The simulation** (`GXSR::GXWaterInteractionManager`, programmes `GXWaterInteractionHeightMap` / `…NormalMap`): a damped 2-D
     wave equation on a height texture, one finite-difference step per frame, fed by "stamps" (filled discs) and read back as a
     **normal map at t58**, bound **only** to the `…Interaction` water families. **It is dead code in the shipped data**: its two
     inputs never occur (section 2.4). The manager is nevertheless *enabled* by the GPARAMs of m33 (Road of Sacrifices) and runs idle there.
   - **B. The screen-space map `t57`** (`g_WaterInteractTexture`, the 640x360 `R16G16_FLOAT` map cleared to (0.502, 0.502)): **a
     different object**, owned by the scene context (`ctx+0xFE0`), **not written by the manager**. It is read by **every** water
     programme (`Water`, `WaterBumpTri`, `WaterInteraction`, `WaterBumpTriInteraction`, all four `_Fwd` read t57 with the same three
     taps). The only writers found are FFX draw entities routed to the engine pass **0x23** (section 3.3). Whether any shipped
     FXR carries the routing flag is **OPEN** (needs the FXRs; method in section 6).
2. **The manager** lives at `GXSceneContext + 0x2F80` (embedded, constructor `1400A4750`, called from the context constructor
   `1400843C0:FUN_1400a4750(param_1 + 0x5f0, …)`). It records **one command in engine pass 0x21** (main view 0x45 only), which
   executes in pass-key order **before** the t57 clear (pass 0x23) and the water draws (0x24 / 0x27).
3. **Shipped settings** (GPARAM group "WaterInteraction Filter"): `Enable=1, ScreenSpace=1, Create 20 m, Delete 40 m, MaxResolution
   1024, TexelSize 0.02 m, ScreenSpace Max Resolution 1024` in **m33_00_0000, m33_00_0010 and the ten s33_00_00xx files** (12 files; values read in m33_00_0000, m33_00_0010 and s33_00_0000, the other nine from the scan's enable columns);
   `Enable=0` in m30_00_0000/0010/0200 (High Wall; Create 80, Delete 120, 2048). 168 other files have the group with no value.
4. **No MTD selects an Interaction family** and **no MTD sets bit 17 (`GXFT_WaterInteraction`)**: so on the shipped data the manager has
   no handle, hence nothing to simulate, and nothing reads t58.
5. **The plain water families do read t57** (`WaterBumpTri_Fwd` lines 448-451): the interaction "only goes through t57" is
   correct for the shipped water. With the neutral map (3 x 0.50196 x 0.664062 = 1.0000) the shader's perturbation is exactly zero,
   so binding the grey default is exact **until something draws into the map** (section 4.3).

## 1. The manager

### 1.1 Where it lives (VERIFIED)

`1400843C0` (the `GXSceneContext` constructor, vtable `143D466B8`): `FUN_1400a4750(param_1 + 0x5f0, param_2)`, i.e. the manager is the
member at **context + 0x2F80** (0x5F0 x 8). Other code addresses it as `ctx + 0x2f80`:
`103510` (`FUN_1400a4c50(lVar4 + 0x2f80, …)`, `FUN_1400a6470(lVar4 + 0x2f80, …)`), `140087fb0` (`FUN_1400a5aa0(param_1 + 0x2f80)`),
`140e00150` (`FUN_1400a5050(lVar1 + 0x2f80, …)`), `140085540` (`FUN_1400a49b0(param_1 + 0x2f80, &settings)`).

Vtable `143D47638` (RTTI `GXSR::GXWaterInteractionManager`): slot 0 `1400A6440` (command callback, below), slot 1 `1400A48D0`
(deleting destructor). RTTI siblings: handle `143D47650` (`GXWaterInteractionManager::GXWaterInteractionHandle`, 3 slots
`1400A44F0`, `1400A4550` = `GetAreaTransform`, `1400A45B0`), material patch `143D47670` (4 slots; `1400A45E0` = apply), height map
`GXSR::GXWaterHeightMap` `143D5FFE0`, normal map `GXSR::GXWaterNormalMap` `143D60060`, FFX appearance `GXFFX::GXWaterInteractionAppearance`
`143D65DC8`, parameter class `GXBS::GXWaterInteractionParam` `143D62388` and its manager `GXBS::GXWaterInteractionParamManager` (the GParam
reader that feeds the settings). `EV/vtable-string-refs.txt` lists every code site that loads these vtables and the debug strings.

Manager fields (VERIFIED from `1400A4750`, `1400A4C50`, `1400A5AA0`; names INFERRED from use):

| Offset | Content |
|---|---|
| `+0x08` | device / allocator |
| `+0x10 / +0x18 / +0x20` | vector A: the world areas (begin, end, cap); `+0x28` allocator |
| `+0x30 / +0x38 / +0x40` | vector B: the handle entries (0xB0 bytes each); `+0x48` allocator |
| `+0x50` | the **screen-space area** (height map + normal map + patch, 0x20-byte record) |
| `+0x58` | the **default area** (a 2x2 neutral height/normal pair, created by `1400A49B0`) |
| `+0x68` u8 / `+0x69` u8 | **Enable** / **Screen Space Enable** |
| `+0x6C` / `+0x70` | Create Distance / Delete Distance (m) |
| `+0x74` / `+0x78` / `+0x7C` | MaxResolution / ScreenSpace TexelSize (m) / ScreenSpace Max Resolution |
| `+0x80` u8 | editor lock (when set, `Update` does not overwrite `+0x68..+0x7C`) |
| `+0x84` | count of live (non-default) areas; `+0xC0` bytes, `+0xC4` MB used |

### 1.2 Construction and defaults (VERIFIED)

- **Object defaults** (`1400A4750:…`): Enable 0, ScreenSpace 1 (`*(u16*)(this+0x68) = 0x100`), Create `0x41A00000` = **20.0**, Delete
  `0x41C80000` = **25.0**, MaxResolution `0x200` = **512**, TexelSize `0x3CA3D70A` = **0.02**, ScreenSpace Max Resolution `0x800` = **2048**.
- **Scene-context creation** (`140085540:866-881`, view flag bit 16 of `ctx+0x50`, only when `param_5 == 0`): settings seed `{u16 0x0100,
  800.0 (143D7F034), 1000.0 (143D7F040), 0x800, 0.02 (143D7ED44), 0x800}` passed to `1400A49B0`; `ctx+0x3EC8 = 1`. `1400A49B0` creates
  the **default area**: a `GXWaterHeightMap` and `GXWaterNormalMap` of **2 x 2** texels with area 1.0 x 1.0 and stores the settings into `+0x68..+0x7C`.
- **Per frame** the real settings come from the GPARAM blend (see 1.4), not from these seeds.

### 1.3 Targets (VERIFIED code; DXGI names INFERRED from the capture)

`GXWaterHeightMap` constructor `1401FC960`, init `1401FCB20(hm, device, W, H, areaW, areaH, texel)`: **three** render textures
(`hm+0x10`, `hm+0xD0`, `hm+0xD8`), each `FUN_1400DC6A0(rt, W, H, 0x22)`; the same format code `0x22` is the format of the 640x360 t57 map,
which the captures show as `R16G16_FLOAT`, so `0x22` = `R16G16_FLOAT` (INFERRED). A constant buffer of **0x60 bytes** (`hm+0x28`) and the
pixel programme looked up by name (`1401FD760`, registry `GX2+0x4DD0`, key `"GXWaterInteractionHeightMap"` at `143D5FFE8`).
`GXWaterNormalMap` init `1401FE8E0(nm, device, W, H, areaW, areaH)`: **one** render texture `FUN_1400DC6A0(rt, W, H, 0x1C)` (`0x1C` = `R8G8B8A8_UNORM`,
INFERRED), a 0x10-byte constant buffer, programme `"GXWaterInteractionNormalMap"` (`143D60020`). Memory is accumulated at `manager+0xC0`.

- **Height states: 3**, ping-pong rotated once per frame by `1401FD120(hm, dt)`, called from `1400A5AA0` with `dt = 143D7ED50 = 0.033333` (1/30, fixed):

  ```
  1401FD120:  if (hm+0x30 == 3) { t8 = hm+0xD8; t5 = hm+0xD0; hm+0xD0 = hm+0x10; hm+0xD8 = t5; hm+0x10 = t8; }
  ```
  So after the rotation `+0xD0` is the newest state (read as **t0**), `+0xD8` the previous one (read as **t7**), `+0x10` is the oldest and
  becomes the next output. `1401FD120` also writes `hm+0x60 = dt` (`g_deltaTime`), `hm+0x64 += dt` (`g_totalTime`) and clamps the stability
  parameters (`hm+0x6C` wave speed, `hm+0x84` FEM dt) so that `4 - 8c >= 0` (c = speed^2 femDt^2 / dx^2).
- **Heightmap parameter defaults** (`1401FC960`): `+0x30` = 3 (state/"interaction type"), `+0x6C` waveSpeed 3.5, `+0x70` waveLength 0.5,
  `+0x74` waveAmp 0.1, `+0x78` circularAge 2.0, `+0x7C` circularWaveNum 2.0, `+0x80` femAttenuation 5.0, `+0x84` femDt 0.003, `+0x88` dx =
  texel (set by init), `+0x8C` waveK 1.0, `+0xF8/+0xFC` 64.0.
- **Sizes.** Screen-space mode: one area, `W = H = ScreenSpace Max Resolution` (2048 default, **1024 at m33**), area = `W x texel`
  (40.96 m default; **20.48 m at m33**) created by `1400A4C50` and re-created when `+0x7C` differs from the current texture width.
  World mode (ScreenSpace off): per handle, `res = nextpow2(min(MaxResolution, max(boxX, boxZ) / texel))` capped at 2048 (`1400A5AA0` state 1,
  `FUN_1400A67C0(mgr, res, res, size, size)`), with LOD by distance (section 1.5).

### 1.4 The settings feed (VERIFIED)

`103510:1795-1835` (main view `iVar26 == 0x45` only):

```
if (ctx[0x3ec8] == 0) { S = {dword ctx[0x2fe8], …}; ((byte*)&S)[0] = 0; Update(ctx+0x2f80, &S); }       // manager forced disabled
else { S = blend(GXBS param A = ctx+0x40f8, param B = ctx+0x40e8, weight ctx[0x4108]);                    // 140225350
       Update(ctx+0x2f80, &S);  if (ctx[0x2fe8] /*Enable*/) FUN_1400a6470(ctx+0x2f80, cmdlist, ctx); }   // records the pass
```

`S` has 7 dwords (`140225350` blends them): `[0]` bytes Enable, ScreenSpace; `[1]` Create; `[2]` Delete; `[3]` MaxResolution; `[4]` TexelSize;
`[5]` ScreenSpace Max Resolution; `[6]` unused here. `1400A4C50` copies `S[0..5]` into `+0x68..+0x7C` unless `+0x80`. Enable off or
ScreenSpace off tears the screen-space area down (`+0x50` released, `FUN_1400A66C0`).

**Shipped values** (VERIFIED, `EV/gparam-m33_00_0000.tsv`, `-m33_00_0010.tsv`, `-m30_00_0000.tsv`; scan `evidence/I-0063/artifacts/scan/gparam.tsv`):

| Files | Enable | ScreenSpace | Create | Delete | MaxRes | TexelSize | SS MaxRes |
|---|---|---|---|---|---|---|---|
| m33_00_0000, m33_00_0010, s33_00_0000/0020/0021/0022/0030/0031/0032/0040/0041/0042 (12 files; values read in m33_00_0000, m33_00_0010, s33_00_0000, the other nine from the scan's enable columns only) | 1 | 1 | 20 | 40 | 1024 | 0.02 | 1024 |
| m30_00_0000/0010/0200 (the scan; values read for m30_00_0000) | 0 | 1 | 80 | 120 | 2048 | 0.02 | 2048 |
| 168 other files (e.g. m37_00_0510) | the group has no value | | | | | | |

The default GPARAM has no such group (`EV/gparam-default.tsv`). What the manager does with "no value" (the parameter manager's own
defaults) is OPEN; it does not matter without inputs.

### 1.5 The per-frame update and the passes (VERIFIED)

`FUN_1400A6470(mgr, cmdlist, ctx)` (`EV/decomp-core.txt`, the function at the line `=== 1400a5e30`'s caller `1400a6440`/`14010ab80`) records **one
command**: `ctx` recorder fields `+0x420 = 0x101`, `+0x423 = 0`, `+0x128 = 0x21` (**engine pass 0x21**), `+0x12C = 0xFFFF` (mask), only when
`+0x124 == 0x45` (main view). The command object is `{callback = manager vtable slot 0 (1400A6440), arg1 = manager, arg2 = ctx + 0x3EE0}`.
At execution `1400A6440(rcx = –, rdx = commandContext, r8 = {manager, ctx+0x3EE0})` loads `rdx+0x450` and `rdx+0x418`, calls
`1400A5E30(manager, rdx+0x450, rdx+0x418, ctx+0x3EE0)` and then tail-jumps to `14010AB80` (INFERRED from the merged decompile: it re-writes 13 sampler slots from `[GX1+0x2A0]+0x2C8..0x340`, i.e. restores the standard samplers).

**Sort slot.** The recorder key is built in `1400BF4C0`: `key = ((view & 0x7F | grp << 7) << 6 | pass & 0x3F) << 16 | mask`
(`decomp-core.txt`, `=== 1400bf4c0` is in `decomp-support.txt`). Commands run in ascending key order, so within the main view the manager
runs at **pass 0x21, mask 0xFFFF**: after passes <= 0x20, after the FFX type-1 draws of mask 0 (`1402997E0` stores mask 0 for pass 0x21), and
**before** pass 0x23 (the t57 clear, 3.3), pass 0x24 (`WatDpt` rows and the t57 bind, mask 3) and 0x27 (the surface). (Pass numbers of the
water rows: `57A20` rows `{0x24, prep 6}` and `{0x27, prep 0}`, `decomp-57a20.txt:678-684`.)

**Inside `1400A5E30`** (the whole frame work; `decomp-core.txt` `=== 1400a5e30`):

1. for every entry (handle) in vector B: in screen-space mode set `entry.state = 0`; otherwise compute the camera distance `d` to the entry's
   box and set the state: 1 create (`entry.area == default && d < Create`), 2 release (`d > Delete`), 3 resize (LOD: `d > 0.75 Delete` -> res >> 3,
   `> 0.5` -> >> 2, `> 0.25` -> >> 1, else full; only when the target size differs);
2. if the entry's area has a pending-clear flag (`+0x101` set, `+0x100` clear) clear the three states via `1401FE350` (fullscreen draw of a constant);
3. for each **pending stamp** of the entry (vector at `entry+0x98`, 32-byte records `{pos.xyzw, p3, p4, flag}`) call `1401FDB00(hm, ctx, pos, p3, p4, screenSpace)`;
4. world mode: `1401FD280(hm, ctx, {0,0})` then `1401FEB10(nm, ctx, hm+0x10)` for that area;
5. screen-space mode, after the loop: the camera is snapped to whole meters in x and z (`(float)(int)v`, `|v| < 2^23`), the scroll is the difference between the
   snapped current camera (`cmd+0x418` `+0x30/+0x38`) and the snapped previous one (`ctx+0x3EE0` `+0x30/+0x38`), then `1401FD280(area hm, ctx, scroll)`,
   `hm+0x104/+0x108 = (scroll.x / dx, -scroll.z / dx)`, `1401FEB10(area nm, ctx, hm+0x10)`.

`1400A5AA0` (called by `140087FB0` at end of frame when `ctx+0x3EC8`): frees dead areas, rotates the three textures (`1401FD120`, once per area,
the default area excluded), re-points every entry at the screen-space area (screen-space mode) or applies the pending state change, then swaps the stamp lists
(`FUN_1400A75E0` is a `std::vector::swap`): stamps added in frame N are drawn in frame N+1.

**The clear to (0.502, 0.502)** is *not* part of the manager: it is `1401018A0` (section 3.3) on the t57 map.

## 2. The inputs

### 2.1 Handles (VERIFIED)

`GXWaterInteractionHandle` is `{vtable 143D47650, manager}` (0x10 bytes, created by `1400A5050`). The **entry** it denotes (0xB0 bytes, vector B) holds:
`+0x00` handle, `+0x08` area, `+0x10` state (0 none, 1 create, 2 release, 3 resize), `+0x20/+0x30` box centre/half-extent vec4 pair derived from the box,
`+0x40/+0x50` AABB min/max (vec4, w ignored), `+0x60..+0x8F` the model matrix (3 vec4), `+0x90` resolution, `+0x98/+0xA0` the two stamp lists,
`+0xA8/+0xAC` the resize target and ratio. `1400A5050(mgr, aabb(2xvec4), matrix(3xvec4))` returns the handle (0 when `mgr+0x58` is null);
`1400A5970(mgr, handle, aabb, matrix)` updates box and matrix each frame; `1400A5320(mgr, handle)` destroys; `1400A5A40(mgr, handle, speed)` writes
`speed` into the handle's current area (`hm+0x6C`, the wave speed); `1400A5770(mgr, out12, handle, cam)` (via handle slot 1) returns the area-to-world transform
(3 vec4) the draw system uses.

### 2.2 Who creates a handle (VERIFIED)

Only `140E00150` (`FUN_1400a5050(scene+0x2f80, FUN_14006f810(aabb), FUN_14025f9d0(matrix))`, then `1400A5A40`), whose only caller is `140D3C4D0`, the constructor of
**`NS_SPRJ::SprjFD4ModelItem`** (vtable `142951FF8`), `140D3C4D0:…`:

```
if (lVar13 != 0 && FUN_140061b00(model))              // some material of the model has bit 17 (0x20000 = GXFT_WaterInteraction)
    h = FUN_140e00150(scene, FUN_140664120(model) /* AABB at model+0x3D0 */, &matrix, FUN_140061b90(model) /* speed */);
    item[0xED] = h;  item[0xD3] = h;  *(u8*)(item+0xD4) = 1;
```
`FUN_140061b00` walks the model's material list (`model+0x1F8..0x200`) testing `material+0x24 >> 0x11 & 1`; `FUN_140061b90` returns the maximum of
`*(float*)(materialParam+0x84)` (the `g_WaterInteractionSpeedParam`, debug name "InteractionWaterSpeed") over those materials. The item updates the handle
every frame in `140D3E950` (`FUN_1400a5970`) and destroys it in `140D3CC00` (`FUN_1400a5320`).
So **a handle is a water mesh** (an area of water), never a footstep. **Bit 17 is set by 0 MTDs** (`MTD-FLAG-BITS` sheet; I re-checked
`mtd.tsv`: no MTD name or SPX contains "Interaction").

### 2.3 Who creates a stamp (VERIFIED)

Only the FFX appearance `GXFFX::GXWaterInteractionAppearance` (vtable `143D65DC8`), slot 8 `FUN_14027BD30(self, ctx, drawCtx, params)`:

```
if (*(int*)(params+0x70) != 0) {
   m = FUN_14025f9d0(buf, drawCtx+0x60);                        // the effect's world matrix
   mgr = *(ctx+0x9B0);  if (mgr && mgr->Enable (+0x68)) {
      pos = (m[0xC], m[0x1C], m[0x2C], 1.0 /*1439F250C*/);        // the translation column
      u = *(float**)(scene+0x260);
      AddWave(mgr, &pos, u[1], u[0]);                              // FUN_1400a5430
```
The action type is **10600 = 0x2968** (`14027BDF0` accepts only `*(short*)(action+0x240) == 0x2968`; creator `27BB70`, I-0103 `CENSUS.md:75`).
`AddWave(mgr, pos, p3, p4)` (`1400A5430`) iterates the **entries whose area is not the default area** and, if the stamp's box (`pos +- (p3 * 0.5 + p4)`) meets the
entry box (`FUN_1417B6630`), appends `{pos', p3, p4, 1}` (pos' = pos in world space in screen-space mode; pos transformed by the entry matrix otherwise) to
`entry+0xA0`. With no entry there is nothing to receive it.

The two floats `u[0]`, `u[1]` come from `*(*(appearance+0x10)+0x18)+0x260`, the effect's per-draw parameters; they are **p4 = u[0]** (the stamp
radius in metres: `1401FDB00` divides it by the area size and multiplies by the texture size) and **p3 = u[1]** (the stamp amplitude: the disc's vertex colour).
Which FXR field they are is OPEN.

### 2.4 Are the inputs ever present? (VERIFIED on data, INFERRED to "dead")

- FXR action **10600** has **0 rows** in the whole-game FXR scan (`evidence/I-0063/artifacts/scan/fxr.tsv`: 4,390 distinct FXRs, no 10600; I-0103 `RESULT.md:14`, `CENSUS.md:160`:
  "never used by the game"). The I-0063 map's "E09 water interaction 10013" label is wrong: 10013 does not exist in the registry (`FFX-SCENECTRL-NATIVE.md:104`), the
  appearance is type 10600.
- Bit 17: 0 MTDs. Therefore **no handle exists on the shipped data, so `AddWave` (which needs entries) can never record a stamp, and no material applies the t58 patch.**
- INFERRED: the manager (at m33 enabled, screen-space, 1024^2 x 3 + 1024^2 normal, stepped every frame) simulates a flat field, and its normal map is never sampled.

### 2.5 Position to map coordinates (VERIFIED)

- Screen-space area: centred on the **camera x/z snapped to whole metres**; side = `SSMaxRes x texel` (20.48 m at m33). A stamp at world `p` gives, inside `1401FDB00`,
  `px = (p.x / areaW) * W + W/2`, `py = -(p.z / areaH) * H + H/2` after the camera offset (see `matrix` built by `FUN_14004BCA0` in `1400A5E30`), radius in pixels
  `= p4 / area * W` (the smaller of the x/y values), centre value `p3`. The older texture (t7) receives the same disc displaced by `(hm+0x104, hm+0x108)`.
- The map scrolls: the cbuffer carries the whole-metre camera step in texels (`g_vecTranslate`, section 3.1).
- `g_ClickPos` (cb word 0..2) is `hm+0x120..0x128`, never written (0): the analytic cases 1 and 2 of the programme are unused; `g_interactionType` is always 3 (FEM).

## 3. The two passes

### 3.1 Height-map pass `1401FD280(hm, ctxWrapper, float scroll[2])` (VERIFIED)

One **fullscreen draw** (no per-handle draw): `FUN_14007F820(shaderMgr, ctx, 1)` = vertex buffer `shaderMgr+0xB58` (stride 0x18), layout `shaderMgr+0xB60`, topology 5 (strip), **4 vertices**.

| Item | Value |
|---|---|
| Render target | `hm+0x10` (its RTV `+0x30`), size = the texture size |
| SRV | **t0 = rt(hm+0xD0).SRV(+0x28)**, **t7 = rt(hm+0xD8).SRV(+0x28)** |
| Sampler | slot 0 = `[GX1+0x2A0]+0x2E8` (`GX1` = `[144961AB0]`) |
| Vertex shader | `shaderMgr[GX2+0x4DD8] + 0xC18 -> +0x10` (the common fullscreen VS) |
| Pixel shader | `hm+0x18` = `GXWaterInteractionHeightMap` |
| Raster / depth-stencil / blend | bank `+0x08` / `+0x1D8` (stencil ref 0xFF) / `+0x208` of `[GX1+0x2A0]` (no blend, colour write all, no depth) |
| Constant buffer b0 | 0x60 bytes at `hm+0x28`, filled from `hm` as below |

cbuffer `cbWaterHeightMap` (fxc `EV/GXWaterInteractionHeightMap.asm`) and its source (`decomp-core.txt` `=== 1401fd280`):

| cb offset | Name | Value written |
|---|---|---|
| 0 (x y z) | `g_ClickPos` | `hm+0x120..0x12B` (always 0), w = 0 |
| 16, 20 | `g_heightMapWidth/Height` | `hm+0x50`, `hm+0x54` (pixels as floats) |
| 24, 28 | `g_areaWidth/Height` | `hm+0x58`, `hm+0x5C` (metres) |
| 32 | `g_deltaTime` (unused by the programme) | `hm+0x60` = 1/30 |
| 36 | `g_totalTime` | `hm+0x64` (+= 1/30 per frame) |
| 40 | `g_interactionType` | `(float)(u8)hm[0x30]` = 3 |
| 44 | `g_waveSpeed` | `hm+0x6C` (clamped by `1401FD120`) |
| 48, 52 | `g_waveLength`, `g_waveAmp` | `hm+0x70`, `hm+0x74` |
| 56, 60 | `g_circularAge`, `g_circularWaveNum` | `hm+0x78`, `hm+0x7C` |
| 64, 68 | `g_femAttenuation`, `g_femDt` | `hm+0x80`, `hm+0x84` (clamped by `1401FD120`) |
| 72, 76 | `g_femDx`, `g_waveK` | `hm+0x88` (= texel), `hm+0x8C` |
| 80, 84 | `g_vecTranslate.xy` | `(scroll.x / dx, -scroll.y / dx)` (the second operand has its sign bit flipped with `143D801C0`) |
| 88, 92 | `g_vecTranslate.zw` | `hm+0x104`, `hm+0x108` (the previous frame's scroll) |

The programme (case 3, the only one used): with `invW = 1/max(W, 0.001)`, `c = waveSpeed^2 * femDt^2 / max(femDx^2, 0.001)`, `a = femAtt * femDt`,
`h0(p) = t0(p + vecTranslate.xy)`, `hOld = t7(uv + vecTranslate.zw)`:

```
out = min(1,  (2c/(a+2)) * (h0(N) + h0(S) + h0(E) + h0(W))  +  ((4 - 8c)/(a+2)) * h0(C)  +  ((a-2)/(a+2)) * hOld );   // 0 where uv is within one texel of the border
o0.xyz = out;  o0.w = 1
```
(`GXWaterInteractionHeightMap.asm` lines 128-170; the height is signed around 0, **no 0.5 offset**.)

### 3.2 Stamp pass `1401FDB00` and `1401FD7B0` (VERIFIED)

Per pending stamp: shader set type 3 of the 2-D helper (`GX1+0x3C0`, `FUN_1400DB4A0(…, 3)`), raster `+0x08`, depth `+0x1E0` (no depth buffer), blend `+0x208`; a 32-gon
**triangle strip of 50 vertices** (8 floats each: position in NDC from pixel coordinates, `x*2/Wrt - 1`, `y*-2/Hrt + 1`; and a constant vec4 `(p3, p3, p3, 1.0)`), radius in pixels.
It is drawn **twice**: into `rt(hm+0xD0)`, then (after `FUN_1400C5C60` and a render-target switch) into `rt(hm+0xD8)` with the centre displaced by `(hm+0x104, hm+0x108)` in screen-space mode.
(Guard, `1401FDB00` first block: in screen-space mode the stamp is skipped when `0.5 * max(areaW, areaH) - length(pos) < radius + 1.0`, `pos` being the stamp's x/z in area space.)

### 3.3 Normal-map pass `1401FEB10(nm, ctxWrapper, heightRT)` (VERIFIED)

Fullscreen draw (`FUN_14007F820(…, 0)`, 4 vertices), RT = `nm+8` (RGBA8), **t3 = heightRT.SRV** (`hm+0x10`, the state just written), sampler slot 0 = bank `+0x2E8`,
PS `nm+0x10`, VS the common fullscreen VS, raster `+8`, depth `+0x1D8`/0xFF, blend `+0x208`. Constant buffer `cbWaterNormalMap` (16 bytes at `nm+0x18`):
`{g_normalMapWidth = nm+0x20, g_normalMapHeight = nm+0x24, g_normalMapAreaWidth = nm+0x28, (g_normalMapAreaHeight = nm+0x2C, unused)}`.
The programme (`GXWaterInteractionNormalMap.asm`) reads the four neighbours of t3 (+- one texel), forms `dh/dx`, `dh/dz` divided by `2 * areaW / W`, builds the normal of the
height field (Y up; `(0, 1, 0)` when `areaW == 0`) and writes `n * 0.5 + 0.5`, alpha 1. Neutral output `(0.5, 1.0, 0.5, 1.0)`.
OPEN (low impact): the code binds sampler *slot 0* while the programme declares `s2` (`SS_BumpMap`); the sampler in force is whatever sits in slot 2.

## 4. The t57 map and its consumers

### 4.1 Creation (VERIFIED)

`140085540:859-863` (the scene-context create, flag bit 15 of `ctx+0x50`): `FUN_1400DD0B0(ctx + 0xFE0, W >> 1, H >> 1, 0x22)`; `ctx+0x3EC7 = 1`. The RT object (RTTI
`GXBS::GXRenderToTexture`, vtable `143D4BAA0`) at `ctx+0xFE0` has its SRV at `ctx+0x1008` and RTV at `ctx+0x1010`. It is **half the output size** (640x360 at 1280x720),
format `0x22` (`R16G16_FLOAT`, INFERRED). It is independent of the manager.

### 4.2 Clear, targets and bind (VERIFIED)

- **Clear and write window = engine pass 0x23**, recorded by `FUN_1401018A0(…, cmdlist, ctx)`, called in `103510:2123-2128` when `ctx+0x1D0` (the reflection object) exists with
  `R+0x3F7C` armed, `ctx+0x3EC7` is set and the view is not 0x3A:
  - mask 3: `FUN_1400BF730(rec, [ctx+0x1010], [ctx+0x1020])` binds the map as the render target; mask 5: a reserved record of 0xF8 bytes; mask 1: the clear
    `FUN_1400BE8E0` with the colour bytes `0x80 0x80 0x80 0x00` / 255 = **(0.50196, 0.50196, 0.50196, 0)**; mask 0xFFFF: unbind, then `FUN_1400BEDB0(slot 0x39, [ctx+0x1008])`.
- **Bind for the water draws** (`103510:1540-1556`): in pass 0x24, mask 3: `FUN_1400BEDB0(slot 0x39, srv)` where `srv = [ctx+0x1008]` if `ctx+0x3EC7` and `[[GX2+0x5360]+0x74] != 0`, else the default texture index 2
  (grey `0xFF808080`). Slot 0x39 = 57 = **t57**. The binder is the frame recorder itself, not `1401BF180` (`WATER-TESS-WAVE1-READY` 1.3 already says so).
- Note (VERIFIED code, consequence INFERRED): if the option and `3EC7` hold but the reflection object is not armed, pass 0x23 is not recorded and t57 is bound to an
  **uncleared** map. Clear it every frame natively.

### 4.3 What writes it (VERIFIED code; data OPEN)

The only writer path found is the FFX draw recorder `FUN_1402997E0(sceneCtrl, entity, kind, …)`:
`uVar12 = 0x23; if (FUN_1402993F0(entity) == 0) uVar12 = <normal pass for kind: 0x21/0x29/0x2D/0x31>;` and `if (pass ctx +0x425 != 0 && [[GX2+0x5360]+0x74] != 0 || !flagged) record`.
`FUN_1402993F0(entity)` is true when the effect's root property list (kind 2000, more than 2 entries) holds the record tagged **`0x2904` (action 10500)** whose int at **record +0x18** is
non-zero, or (when it is zero) the action instance carries a non-zero value in its property slot (`action+0x260[idx+1]`, only if `action+0x254 >= 0x1C`). The same record's `+0x00/+0x04` is the
light-distance `{enabled, maxDist}` (`1402B2E30`) and `+0x20` is a reflection-view flag (`140299500`, group 0x39). So **an FXR whose root 10500 block has field 6 non-zero draws its sprites
into the t57 map in pass 0x23** (INFERRED reading of the flag: "interacts with water"). Whether any shipped FXR sets it is **OPEN** (section 6).
The draw callback is the standard FFX draw (`140299E40`); the target and clear are those of 4.2.

### 4.4 What the water programmes do with it (VERIFIED, fxc)

All four `_Fwd` programmes (`EV/fwd_Water.asm` 432-435, `fwd_WaterBumpTri.asm` 448-451, `fwd_WaterInteraction.asm` 438-441, `fwd_WaterBumpTriInteraction.asm` 454-457) sample t57 with sampler **s9**
at the pixel's screen UV `uv = SV_Position.xy * FC_ScreenSize.zw` and at `uv + (2 px, 0)`, `uv + (0, 2 px)`, reading `.x` only: `h0, h1, h2`.

```
gx = h0 - h1;  gy = h0 - h2;
m  = (h0 + h1 + h2) * 0.664062 - 1.0;   w = (1 - |m|)^2;           // weight of the bump; 0 at the neutral map (3 * 0.50196 * 0.664062 = 1.0000)
d  = gx * normalize(dFdx(-V)) + gy * normalize(dFdy(-V)); d = normalize(d + (0, 1, 0));   // V = the view vector r4
N.xz = N.xz * w + 4.0 * d.xz;  N = normalize(N)
```
(`fwd_WaterBumpTri.asm:448-483`; `w` is `r3.w` after line 478.) At the neutral value `gx = gy = 0`, `m = 0`, the result is the unmodified bump normal: **binding the grey default is bit-exact**
until a pass-0x23 draw exists.

The two Interaction families also read **t58** (`g_WaterNormalTexture`) at the vertex-interpolated area UV `v7.zw` (BumpTri variant) / `v7.xy`, `*2 - 1`, normalised and blended by
the vertex weight `v9.x`: `N = N_bump * (1 - v9.x) + N_t58 * v9.x` (`fwd_WaterBumpTriInteraction.asm:490-498`), then rotated by the view matrix (`cb8[5..7]`).
t58 is bound by the **material patch** `GXWaterInteractionMaterialPatch.apply` (`1400A45E0`): PS stage slot 0x3A = the area's normal-map SRV (`patch+0x10 -> +8 -> +0x28`), vertex/domain stage slot 0x3A = the area's
`hm+0x10` RT SRV (for the `_FT` tessellated variants); the default texture when the area has none. The patch is the third member of the area record (`area+0x10`, vtable `143D47670`).

### 4.5 Family selection and the material condition (VERIFIED / INFERRED)

The only reader of the material flag is `FUN_140061B00` (`material+0x24 >> 0x11 & 1`, creation of a handle, 2.2). The Interaction programme families are selected by the **MTD's SPX name**
(`GXFlver_WaterInteraction`, `GXFlver_WaterBumpTriInteraction[FloatObj]`); no MTD in `mtd.tsv` (512 MTDs) names them. I found no code that converts a plain water material into an Interaction one at run time
(the patch only binds t58 and the speed; INFERRED that the variant must already be in the MTD). Hence the shipped game never draws an Interaction family.

## 5. A minimal Reference witness for E

Target: **m33 (Road of Sacrifices, the swamp pieces `000070..000081` with `Water_DL` and `000012` `Water_3normaltet`, programme `WaterBumpTri`)**, a character wading in the water. m33 is
the only zone whose GPARAM enables the manager, and it has 13 water pieces (`evidence/I-0063/artifacts/scan/flver.tsv`, `STUDY-WATER-FAMILIES.md` 1.2). Hook in the original (D3D11) process,
log once per frame unless said otherwise:

| # | Address | What | Arguments to log |
|---|---|---|---|
| H1 | `1400A4C50` | settings applied | `rcx` manager; `rdx` -> 6 dwords; afterwards `mgr+0x68..+0x7C`, `mgr+0x50`, `mgr+0x58`, vector sizes `mgr+0x18-0x10`, `mgr+0x38-0x30` (/8) |
| H2 | `1400A6470` | the pass is recorded | `rdx` cmdlist, `r8` ctx (`ctx+0x3EC7/3EC8`, `ctx+0x2FE8`) |
| H3 | `1400A5E30` | the frame work | `rcx` manager; `r8` current view (`+0x30,+0x38` camera), `r9` previous (`ctx+0x3EE0`, `+0x30,+0x38`) |
| H4 | `1401FD280` | height step | `rcx` hm: dump `hm+0x30`, `+0x50..+0x8C`, `+0x104/+0x108`, `+0x10/+0xD0/+0xD8` (RT objects, SRV `+0x28`, RTV `+0x30`); `r8` -> 2 floats (scroll) |
| H5 | `1401FDB00` | stamp | `rcx` hm; `r8` -> pos(4 floats); `xmm3` p3; `[rsp+0x28]` p4 (float); `[rsp+0x30]` screen-space flag |
| H6 | `1401FEB10` | normal step | `rcx` nm: `+0x20..+0x2C`; `r8` height RT |
| H7 | `1400A5430` | **a stamp request** | `rdx` -> pos(4), `xmm2` p3, `xmm3` p4; plus the caller (`14027BD30`) |
| H8 | `1400A5050` / `1400A5A40` | **a handle is created** | AABB (2 vec4), matrix (3 vec4), speed `xmm2` |
| H9 | `1401018A0` | pass 0x23 recorded | `r8` ctx: `ctx+0x1008/0x1010/0x3EC7`, `[ctx+0x1D0]+0x3F7C` |
| H10 | `1402993F0` (return value) and `1402997E0` | **which FFX entities go to pass 0x23** | `rcx` entity; the effect id (`entity->vt[0x28]()`...) and the FXR id; draw count |
| H11 | `1400BEDB0` with `edx == 0x39` | the t57 bind | `r8` SRV pointer (compare with `ctx+0x1008`) |

Captures (D3D11 side, no hooks): the RT of pass 0x23 (640x360 `R16G16_FLOAT`) before and after its draws (`R` channel; neutral 0.50196), and the number of draws between the clear and the next
water draw; if H7/H8 never fire (expected) and H10 fires, the only live path is B. A run with the character in the m33 water that logs **H1, H4, H7, H8, H10** answers every open point
below in one session. Also useful: the same hooks at m37 (no GPARAM value for the group) to see whether H10 fires with the manager off.

## 6. What I could not reach (OPEN)

1. **Whether any shipped FXR sets the pass-0x23 flag** (root 10500, record field 6 at `+0x18`; field 8 at `+0x20` is the reflection flag). The FXR bundles are not on disk in
   extracted form (`I0063-cache` has none, `game/` holds only the exe); `Recover-GameDataByName.ps1` (pattern `^data\d/sfx/frpg_sfxbnd_`) plus SoulsFormats BND4 and `FxrDump.cs`
   (fields of the 10500 action) would settle it. Until then, "t57 stays neutral" is the only witnessed behaviour (123 captures), not a proof of absence.
2. The meaning of the two floats of the 10600 appearance (`u[0]`, `u[1]`) and what the FFX draw callback does inside pass 0x23 (blend, programme): not decoded, because no FXR uses it.
3. The GPARAM "WaterInteraction Filter" values for the 168 files where the group has no value, and the parameter manager's own defaults.
4. How the Interaction variant of a programme would be chosen at run time (no MTD, so unobservable); the exact contents of the transform returned by handle slot 1 (`1400A5770`).
5. `[[GX2+0x5360]+0x74]` (the graphics option gating t57): I did not trace which menu option it is (`+0x6C` is the reflection option per `WATER-REFLECTION-INTERACTION-E08.md`).
6. The 4-argument stack layout of `1401FDB00` was read from the disassembly (`mov r8, rbx; movss xmm3, [rbx+0x10]; movss xmm0, [rbx+0x14] -> [rsp+0x20]`, `1400A6210`); the decompiler printed it wrongly.

## 7. Notes for the native transcription (E)

- The t57 map: a half-resolution `R16G16_FLOAT` target cleared to `0x808080` (= 0.50196) **in pass 0x23 every frame the reflection object is armed**, bound at t57 in pass 0x24; when
  disabled, grey default. Today's native (neutral map) is exact for the shipped witnesses. Anything beyond requires the FXR flag evidence of section 6.1.
- The manager needs no native code for the shipped game: no handle, no stamp, no reader. If transcribed for completeness, the contract is sections 1.5, 3.1-3.3 and the formulas above; it
  costs 3 x res^2 x 4 bytes + res^2 x 4 bytes per area (1024: 16 MB) every frame at m33 for no visible effect.
- Elden Ring was not consulted.

## 8. Scripts and raw outputs (rerunnable)

| File (`EV/`) | How it was made |
|---|---|
| `decomp-core.txt`, `decomp-support.txt`, `decomp-57a20.txt`, `decomp-frame-init.txt` | `pwsh -NoProfile -File run_decomp.ps1 -Out <file> -Addrs "<hex VA ...>"` = `Invoke-Ghidra.ps1` + `investigations/I-0099-exit-heap-fault/I0099Decompile.java` on `ghidra-mirror-i0066` (idle priority). Address lists: core `140103510 1400a4c50 … 140087fb0`; support `1402997e0 1402993f0 … 140e00150`; 57a20 `140057a20 1400575c0 14005a120 1401018a0 14010af10 1400bedb0 1400be8e0 1400bf730 14020e570`; frame-init `140085540 140085220 140088d90` |
| `vtable-string-refs.txt`, `vtable-riprefs.txt` | `py -I riprefs_fast.py game/DarkSoulsIII.exe 143d47638 143d47650 …` (fast variant of `I-0100-popin/riprefs.py`; `vtable-riprefs.txt` is the slow original's output) |
| `mgr-rdata-strings.txt`, `gxbs-param-manager-strings.txt` | `py -I u16strings.py game/DarkSoulsIII.exe 143d47680 143d48900` and `143d5ad00 143d5b170` (UTF-16 strings of the material-parameter table and of the GXBS parameter manager) |
| `disp-*.txt`, `r16g16-creators-raw.txt`, `t57-binders-raw.txt` | `find_disp32.py`, `find_movedx.py` (candidate sites; `t57-binders-raw.txt` is empty: no `mov edx,0x39 ; call 6100/57140/ff050` pattern exists, the binder is `1400BEDB0` through the recorder) |
| `fwd_*.asm`, `GXWaterInteraction*.asm` | `fxc /nologo /dumpbin gxflvershader__GXFlver_<family>_Fwd.ppo` and `gxrenderershader__GXWaterInteraction{HeightMap,NormalMap}.ppo` from `C:/Users/User/.codex/I0063-cache/programs` |
| `gparam-*.tsv` | `pwsh -File investigations/I-0067-static-menus-subsystems-map/Get-GparamValues.ps1 -Path C:/Users/User/.codex/I0063-cache/files/param_drawparam_<map>.gparam.dcx -Out <tsv>` |
| GhidraMCP readings | `get_xrefs_to`, `decompile_function_by_address`, `read_memory` on the GUI database; constants: `143D7EE7C` = 2.0, `143D7EDB4` = 0.5, `143D7EE10` = 1.0, `143D7ED50` = 0.033333, `143D7ED9C` = 0.25, `143D7EDE8` = 0.75, `143D7F0C0` = -0.5, `143D7F0D8` = -2.0, `143D7EF88` = 32.0, `143D7EF2C` = 8.0, `143D7ED90` = pi/16, `143D7EFF4` = 255.0, `143D7F034` = 800.0, `143D7F040` = 1000.0, `143D7ED44` = 0.02, `143D7F390` = (-1, 1, 0, 0) |

Note: the Ghidra GUI database has no data references to the vtables and strings (analysis is partial): the raw scans above are the reproducible source of the "who loads it" claims.
