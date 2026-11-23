# T27: where the original's diffuse ambient of a deferred skinned character comes from, and the cheapest zeroing hook

Static reading by a Sonnet subagent for the R pilot (2026-11-23), non-canonical until the pilot rereads what a design rests on.
READ = seen in code or capture text this session; INFERRED = my reading of the shape; OPEN = not reached.
Sources: GhidraMCP on Thomas's GUI (DarkSoulsIII.exe 1.15.2, sha256 ef5e07c5...): `decompile_function_by_address` on 149CB0,
14AB20, 1496D0, 137910, 5A120, 5B3D0, 5CEC0, 1AC840, 1400616E0, 140061950, 1495F0, 14BA90, 32A80, 675B0, 416A0, 3FEB0, 40DD0, D9EE0;
`get_function_xrefs`; `read_memory` 1439F24B0. Programme text: `evidence/I-0119/artifacts/census/asm/*Gbuf*.ppo.asm` (676 G-buffer PS) and
`evidence/I-0125/artifacts/dxbc/*` (sun, DeferredAO, EnvMapRegion). No script was run (no headless Ghidra, no build). No file other than this one touched.

## 0. In short

1. The deferred character's diffuse ambient does NOT come from cbLight. The Ibl G-buffer programmes (SkinSSSIbl, SAT_Equip*, Dragon_FDP,
   HairIbl2, C_ARSN*, 486 of 676) write `o4.xyz = (1 - F0) * max(0, (N,1) . aSh[3])` with `aSh` = `cbInstanceData` (cb4) rows +8..+10 (byte
   +0x80..+0xAF of each 0x110-byte instance record). READ (asm below). `FC_AmbientSH` (cbLight rows 12-14) is read by 12 Gbuf programmes only:
   `Fujir_[ARSN]_OpacityBlend` and the Water family (WaterBumpTri*, WaterInteraction, Water, + `_[A]` twins); none is a character body. READ (grep).
2. The instance record is built per draw group by `5B3D0` (`BuildVSModelParams`), not by 149CB0/14AB20/1496D0. READ.
3. 137910's rows 12-15 are read by no deferred pass: the sun pass reads cb5 rows 0-11, 16, 17 only; DeferredAO has no constant buffer. READ.
4. Cheapest hook: a post-call hook on `5B3D0` that zeroes `param_8 + 0x80 .. +0xAF` (0x30 bytes). Section 5.

## 1. What the character's G-buffer programme reads (READ)

`gxflvershader__GXFlver_SkinSSSIbl_Bloody_Gbuf.ppo.asm` (cbuffers cb1 TextureTilingScale, cb2 MatDynParam, cb3 MtdParam, cb4 InstanceData, cb7
ModelParam; no cbLight, no cbPerFrame), tail:
```
dp4 r2.x, r0.xyzw, cb4[r3.y + 8].xyzw   ; r0 = (N, 1)
dp4 r2.y, r0.xyzw, cb4[r3.y + 9].xyzw
dp4 r2.z, r0.xyzw, cb4[r3.y + 10].xyzw
max r0.xyz, r2.xyzx, l(0)
mul o4.xyz, r1.xyzx, r0.xyzx            ; r1 = 1 - F0
```
`Dragon_FDP__Dragon_FDP_Gbuf.ppo.asm` lines 308-312: the same four lines with `cb4[r1.y + 8..10]`; its cb9 (cbPerFrame) is read at line 255 only
(`cb9[11].x`, not ambient). `o1.z = cb4[+11].x` (ShLightMask.x, the GB1.z sun mask, not ambient). The struct declares `aSh[3]` at
offset 128 and `ShLightMask` at 176 of `PerInstanceData` (0x110 bytes: VC_InstanceData[2] = 532 bytes in the dump). 138 Gbuf PS sample a
lightmap `t26` instead (map pieces, family A of `I-0126-rt-gi/STUDY-R7-DS3-AMBIENT.md`); a few `*_nolight`, `Veill_NoLight`, `Eclipse_cloth`,
`ColShZFeather` read neither (no ambient in GB4).
Downstream (READ, `dxbc/GXLightDeferredAO_ppo.asm` 44-50): `o0 = t6.rgb * t32.x * t32.y`, `o1 = t7.rgb * t32.x * t32.y`; no cb at all.

## 2. Who fills the instance SH rows (READ in 5B3D0 `14005B3D0`, signature of the decompile)

`5B3D0(uint k, longlong, longlong view, longlong dc, longlong group(param_5), int* pass(param_6), uint, uint64* record(param_8),
longlong, uint64* , char)`. Callers: `5CEC0` at 14005D0B1 (record = its stack array `alStack_228[34]` = 0x110 bytes, uploaded by
`UpdateSubresource` right after the call, READ) and `5D5C0` at 14005D93F (array of instances, INFERRED same). Block taken only when
`*param_6 == 0 || *param_6 - 3 < 2` (pass kinds 0, 3, 4; the same `uVar2` 5A120 switches on). Source of the six qwords stored at
`record[0x10..0x15]` (= bytes +0x80..+0xAF), in order of test:
- `[[GXRS+0x4DD0]+0x13E8] != 0`: rows = `[GXRS+0x4DD0]+0x190..+0x1BF` (a global override; the IEM-derived block of `D9EE0`). ShLightMask then
  collapses to 1 (fVar46..49 = 1), so this switch is NOT neutral for the sun mask. Its writer was not found (OPEN).
- else the model's animation block `q = [group+0x68]` when `q != 0 && q != -0xF0 && [q+0x170] != 0`: rows = `q+0x130..+0x15F`, strength `q+0x16C`.
  `q+0x130` is written each update by `1400616E0` (a copy of `1AC840`, `p = q+0xF0`) and once by `140061950` (new probe record): the
  `.btpb` probe's 13 shorts (SH basis `143D7F940`), or the light-set record rows `+0x290..+0x2BF` of key `q+0x1A0`, blended by clock
  `q+0x164/+0x168`. Gated by `[GXRS+0x4DD0] != 0`.
- else the light-set records of the group's two keys `group+0xF8`, `group+0x100` (32A80 on `[root+0x41E0]`), rows `+0x290..+0x2BF` each,
  blended by `group+0x108`.
Then each row: x, y, z times `1439F24B0 = (1,1,1,1)` (READ, memory), **w times the pass context's `+0x110`, `+0x114`, `+0x118`**
(`param_3+0x110 + 4r`). Bytes +0xB0..+0xBF (ShLightMask = (s-1)*f41+1, f41 from record `+0x2C0`) follow; they gate the sun, do not touch them.
Which of the three sources 32000010's King and dragon take: OPEN (runtime). All three end in the same 0x30 bytes.

## 3. The light-set record and cbLight rows 12-15 (READ)

`32A80(bank=[root+0x41E0], key*)` returns the per-(area,id) record (0x350 bytes, TONEMAP-SKIN §10, size not re-read; default at bank+0x700).
`149CB0` (forward, one record), `14AB20` (forward blend of two, same fields lerped by t) and `1496D0` (deferred; its `param_2` is the
record + 0x40) build the same 0x120 entry; offsets in the record (149CB0's stack layout, entry = cbLight FC_DirLight[n]):

| cbLight row (entry byte) | content | record offsets (READ) |
|---|---|---|
| 12-14 (+0xC0..+0xEF) AmbientSH | row c = `((U_c - D_c)/2 * a.xyz, (U_c + D_c)/2 * a.w)`, U = rgb(+0x220)^2 * (+0x22C), D = rgb(+0x230)^2 * (+0x23C), a = (+0x260..+0x26C & mask) + `1439F2500` | +0x220..+0x23C, +0x260..+0x26C |
| 15 (+0xF0) AmbientReflection | xyz = rgb(+0x270)^2 * (+0x27C), w = +0x280 | +0x270..+0x280 |
| 16 (+0x100) bLocalLight | x = (+0x28D != 0), y = (+0x28E != 0) | gates lights 2 and 3 in the sun pass |
| 17 (+0x110) SsaoWeight | +0x90, +0x100, +0x170, +0x1E0 | |
Record rows `+0x290..+0x2BF` = `1495F0(record, record+0x290)`: the same formula as rows 12-14, cached once at record creation
(`14BA90`) and recomputed by `1495F0`'s other callers (264230, 2A4EB0, 2AE5A0, 147ED0, 148770, 148F50, 14B3A0). cbLight rows 12-14 do NOT read
`+0x290`; they recompute from `+0x220..`. So: zeroing record `+0x22C` and `+0x23C` zeros cbLight rows 12-14 (every term carries the
intensity), `+0x27C` and `+0x280` zeros row 15; the instance SH needs `+0x290..+0x2BF` zeroed instead (or the 5B3D0 output, below).
The 0x2A0-byte copy of the tranche-27 hooks covers every byte 149CB0 and 14AB20 read (max `+0x28E`; I re-read both: nothing past `+0x281`
except the three flag bytes `+0x28C..+0x28E`). READ.

## 4. Rows 12-15 and the deferred passes (Q2)

`GXLightAcc_DDirLightShadowed1_ppo.asm`: cb5 index `r0.w = set*18`; used rows +0..+11, +16 (`.x` gates light 2, `.y` light 3), +17. No
+12..+15. READ. `GXLightComposite` reads cbFog (cb6), `GXLightCmbAlbedo` no cbLight, `GXLightDeferredAO` no cb. So zeroing rows 12-15 of
137910's entries changes nothing in the deferred image (only the G-buffer seeding of the 12 programmes in section 0 point 1 reads them,
through the forward-style cbLight of 5A120, kind 0, `case 0` at 14005A267 / 14005A2BA). Programmes reading cb5 rows 12-15: Fujir_ARSN
OpacityBlend Gbuf (rows 12-14 -> `o4`, `cb5[15].w * cb3[2].z`), Water* Gbuf and `WaterBumpTriInteractionFloatObj_Fwd`; ColDifSpcBumpIbl_Fwd
declares them `[unused]` (STUDY-R7-DS3-AMBIENT §2.3, OBSERVED there, not re-disassembled by me; no other `_Fwd` text is in the census, OPEN).

## 5. The env-map region draws (Q3, READ in 675B0 `1400675B0` and `GXEnvMapRegion_ppo.asm`)

`o0.xyz = (t30 cube sample) * ... * cb3[9].x * fade`, `o0.w = r0.y * cb3[20].x`; `cb3` = CB_ENVMAP_REGION: `g_envMapColorScale`
+144, `g_envMapFadeDistance` +148, `g_envMapFadeRate` +320. In the executor the block sits on the stack from `fStack_308`: `iStack_278` =
`+144` = `iStack_4c8` = **`[FUN_1400416A0(bank [GXRS+0x42A8], region key) + 0xC]`** (float, 1.0 at creation by 3FEB0 `0x3F800000`, lerped by
40DD0 with the blend weight), or `region[0x37]` (int index 0xDC) when `[this+0x2B8] != 0`; fade distance `region[0x33]`; fade rate
`fStack_1c8 = 1 - saturate(...)`. Called through a vtable only (data xrefs 1444A05B0/1444A05C0/144970F10); args `(this, ctx, regions**, count)`;
the draw loop is `if (count != 0) do { ... DrawIndexed } while (--n)` (READ), so a wrapper passing count 0 draws no region at all.
Which gparam file field fills entry `+0xC` beyond the constant 1.0: OPEN (writers 3FEB0/40DD0 only copy or lerp it).
Forward programmes' own cubes: the instance record words `+0xEC` (envMapColorScale0, group `+0x140`) and `+0xFC` (envMapColorScale1,
group `+0x144`), `+0xDC` the interp ratio (group `+0x13C`), all stored by 5B3D0 (READ), as in `producers.cpp:120`.

## 6. Hook points (Q4)

| Goal | Hook | Offsets | Status |
|---|---|---|---|
| Diffuse ambient of every Ibl draw, deferred and forward | detour `5B3D0`; after the original returns, if `param_8 && (*param_6 == 0 \|\| *param_6 - 3 < 2)`: `memset((uint8_t*)param_8 + 0x80, 0, 0x30)` | record `+0x80..+0xAF`; leaves `+0xB0..+0xBF` (ShLightMask) and cbLight untouched | code READ; effect on GB4 INFERRED (o4 formula READ) |
| Same, per-frame gate | one atomic the hook reads | | |
| Specular IBL of forward Ibl draws | same hook: zero floats `+0xEC`, `+0xFC` of the record | | READ offsets |
| Specular IBL of deferred (t7) | detour `675B0`, pass `count = 0`; or zero `[416A0(...)+0xC]` of each region entry and restore | entry `+0xC` | READ; t7's other writers (decals, GB programmes) not read |
| cbLight rows 12-15 of the forward sets and deferred array | extend `ReferenceShareCopy` with `+0x22C`, `+0x23C`, `+0x27C`, `+0x280`; deferred: zero entry `+0xC0..+0xFF` after 1496D0/14A2D0 | no visible effect on a character (section 4) | READ |
Not enough alone: lightmapped map pieces (138 Gbuf programmes) take `t26 x FC_DominantRgbScale x FC_DominantColorScale` (cbPerFrame +0xD0, +0xCC,
asm offsets READ); the CPU writer of those words was not found (23790 builds another block, READ), so that route is OPEN. Decals write GB4 (OPEN).
Why not the sources: `1400616E0` re-blends `q+0x130` every update and the record rows `+0x290` are recomputed by 8 callers; the 5B3D0 tail
catches all three sources of section 2 at the one place every instance record passes. The `+0x13E8` switch also forces ShLightMask = 1.
Gaps: 5D5C0's use of the same call not read; the four pass kinds 0/3/4's exact names (G-buffer vs forward) not tied; which source the 32000010
bodies use; no run of any hook (static only).
