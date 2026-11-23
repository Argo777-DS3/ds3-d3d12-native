# I-0068 part AH: every open live check, grouped by save, zone and position

Request (coordinator, 2026-10-10): one file with every live check still open in the I-0068
notes, grouped by save, zone and position, so the pilot can verify several in one launch. For
each check: what to read and the expected value. Aim: fewer live tests.

## How to read this file

**Kind:**

- **NOW**: settles a static open point. It is readable today on the original renderer
  (Reference, pass-through observers) and/or on today's native.
- **ACC**: acceptance after the named step is coded. Read it in the first run after that step.

**"Orig" / "Nat":** read on the original renderer, on native, or both at the same spot.

**IDs** keep their source note, so the definition stays where it is:

- G·AUDIT = NATIVE-GATES-AUDIT §4;
- G·GLOW = EMISSIVE-GLOW §6;
- G·E07 = EFFECTS-E07 §7;
- R·EL = EFFECT-LIGHTS §5;
- R·W = WATER-REFLECTION-INTERACTION-E08 §1.3;
- R3·CHR = CHR-FADE-EXPARAM-REFLECTION §3;
- F·ATT = FFX-ATTACHED §4.2;
- F·PH = PHANTOMS-SIGNS-FOGWALLS §4;
- F·CHR = CHR-FADE §1;
- P·E01 = EFFECTS-PHASE2 §9;
- P·PH = PHANTOMS §4;
- S·SH = SHADOWS §checks;
- S·PH = PHANTOMS §4;
- L·SP = SPOT-LINE-LIGHTS;
- L·E05 = EFFECTS-E05-E04 §5;
- D·DEC = DECALS;
- D·FWD = FORWARD-DEPTH;
- H = YEBIS-HAL-READY §8;
- K = BINK-NATIVE-READY;
- M = VELOCITY-MOTIONBLUR §5;
- A = SSAO;
- W = WATER-NATIVE;
- E = EFFECTS-E03;
- T = FFX-ATTACHED §4.1;
- V = REMAINING-FAMILIES §3;
- X = CHR-FADE §2;
- Z = EFFECTS-PHASE2 §9.

**Conditions:** every run is read-only, in the D-0116 conditions.

**Settings:**

- **MotionBlur ENABLE** only for the M-rows (Thomas allowed it);
- brightness changes only for H3;
- everything else at Thomas's settings (REMAINING-FAMILIES §2).

## 1. Argo's save: High Wall (`m30_00`)

**Positions:**

- MENUS-MAP §69 gives Argo's start as (30.7, −24.1, 54.9);
- §59 estimates Argo near `c1105_0011` (−32.7, −0.1, 83.2);
- use the save's real position. From (30.7, −24.1, 54.9), **sign 2** (25.0, −25.5, 52.3) and
  **Vordt's fog** (27.7, −25.5, 44.9) are within 10 m.

### 1.1 At the start, no movement (one Reference launch + one native launch)

| ID | Kind | Orig/Nat | Read | Expected |
|---|---|---|---|---|
| G1·AUDIT | NOW | both | `+0x220`/`+0x224` of the map-part models whose meshes classify `0x100` (extend the 1101 witness at `game_bootstrap.cpp:3875` to `asm_part < 0`); include the torch bowl `o304863` (`_em`) | Orig: `+0x224` bit 15 set (INFERRED). Nat: `+0x224` = 0 (confirms PRODUCERS-UNION §1) |
| G4·AUDIT | NOW | Orig | `pipeline+0x2E88..+0x2E9D` (glow filter: BlurSampleScale, RgbScale, AlphaScale, NewGlow, Final, Enable); `[[GXRS+0x5368]+0x80]`; keys `pipeline+0x4028/+0x4090` | 1, 1.5, 1, 1, 0.5, 1 (`mapbase` id 0); factor > 0; key = the view key |
| gparam ids | NOW | Orig | the gparam files registered at load (the `49D00` registration: area and id per file) | `default`, `mapbase` (as (1, 0), INFERRED), `m30_00_*`, `mapoverride_NNNN`, `eclipse`: record the list |
| P4·PH | NOW | Orig | on the player's hair forward draw: break or observe `142430` `out[1]` | the plain `SAT_HairIbl2_Fwd[_[A]]` / `…Bloody_Fwd_[A]` and `DC+0x425` = 0 (MENUS-MAP §75 withdrew the earlier `_Phantom_` expectation; part AL) |
| D2·FWD | ACC (forward depth) | both | depth at a hair texel of Argo | the hair surface depth, not the background's |
| M3 | NOW (MotionBlur ENABLE) | Orig | a frame with an NPC or cloth/hair moving: the `Velo` rows (cloth VS with b2/b7, I-0067 VELOCITY-SKINNING) | rows present; record the VS per family |

### 1.2 Toward Vordt's fog and sign 2 (same launch; walk ≤ 30 m)

| ID | Kind | Orig/Nat | Read | Expected |
|---|---|---|---|---|
| F1·PH | NOW | Orig | counter on the object SFX creation for entity 3001800 (`2006[4]` → object route; FFX-ATTACHED §1.2/§1.3 counters) | one effect at dummy 101 of `o000401_2000` (27.7, −25.5, 44.9), id from `20005820` (3 INFERRED) |
| S1·PH | NOW (Ember needed) | Orig | counter on `853800` with record `+0x23` set; `7CB820` called from `853800` | one call per visible sign at the sign transform. Sign 2 needs flag 9500 (unknown); sign 1 (6.7, −6.3, 87.1) needs 13005392 |
| S2·PH | ACC (effects 1–3, 2.1) | Nat | `sign+0x10` | non-null; E01 draws at the sign |
| P1·PH, P2·PH, P3·PH | NOW (summon) / ACC | Orig, then Nat | summon the NPC of the sign (sign 1: `c0000_0007`, NpcParam 27920; sign 2: `c0000_0008`, 27320): list of its draws, group `+0x87`/`+0xDC`, b2 rows 26–30 | list **0x21** (fade-in: 0x29), `+0x87` set, `+0xDC` < 1 then 1; Edge = (0.5, 0.5, 0.5, 0.5) (PhantomParam 80) |

### 1.3 Argo's torch (−31.32, 2.01, 85.24) (same launch)

| ID | Kind | Orig/Nat | Read | Expected |
|---|---|---|---|---|
| T1 | NOW | Orig | **replaced by T1·a/b/c (LIVE-REDUCTION-AM §4):** counter on `14065A010` (`ObjModelSfxSlot` update) and the arguments of `140E6EB70` at the torch; caller RVA at `7CB820` for the id-300 and id-320 requests; the model-sound call `1404AD240` | torch: effect 800023 on dummy 200, one handle; id-300/320 callers among `52EC30`, `5292A0`, `46E600`, `97CFA0` |
| T2 | NOW | both | the request transform | ≈ (−31.32, 2.45, 85.38) ± rotation |
| T4 | NOW | Orig | slot `+0x2C` bit 5 and `7CC880` per frame | slot: per-frame calls; one-shot: none |
| T3, T5, T6 | ACC (effects 1–2) | Nat | `handle+0x10`; a registry light within 0.5 m of T2; the image | non-null; one warm light; flame sprites and pillar light |
| G5·GLOW | ACC (model union + glow) | Nat | the bowl's GB5 and halo | GB5 written, glow visible |

### 1.4 Vordt's arena (28, −10, 158) (fight active)

| ID | Kind | Orig/Nat | Read | Expected |
|---|---|---|---|---|
| F2·PH | NOW | Orig | object SFX for 3001890 (`o000401_4000`) and 3001780 (`o000400_0000`) | effects at dummy 101, ids 3 and 2 |
| S4·SH | ACC (shadows) | Nat | the player's ground shadow; hood shadow in the Status/Equipment Player views | as the original (MENUS-MAP §48) |

### 1.5 W2: the missing foliage (witness teleport (24.3, −10.3, 96.1); part AP, FOLIAGE-NATIVE §10)

| ID | Kind | Orig/Nat | Read | Expected |
|---|---|---|---|---|
| FOL1 | NOW | Nat | replace the 12-sample caps of `native_selected_constant_unavailable` / `native_vmodel_param_unproduced` (`game_bootstrap.cpp:5007`, `:4867`) by a per-frame histogram keyed by (family, resource, first unproduced word) | grass families only; words 112..127 and 1168..1199 always; 128..1167 only while `anim+0x620` is 0 |
| FOL2 | NOW | both | for the first three grass items in view: `[item+0x760]`, `[item+0x690]`, the two ints at `[item+0x280]`, `[item+0x70+0x630]` | Orig: handle ≠ −1, flag 1, counts 0..8, `anim+0x630` 0 (INFERRED); Nat: record |
| FOL3 | ACC (F1) | Nat | `native_pass_rows` world rows and `content_draws` at the four poses of FOLIAGE-NATIVE §10 | +3 / +9 / +25 / +17 grass meshes (± display groups), no grass refusal; image: litter and masses present, static |
| FOL4 | NOW | both | for `m009002_2004`, `_2006`, `_2010`: `[entity+0x388]`, the live-test and `classify` results, `[item+0x71A]`, `E+0x170`; and the admission outcome of **map-piece** items (extend `game_bootstrap.cpp:3339` to names starting with `m`) | Orig: drawn; Nat: the first failing gate of FOLIAGE-NATIVE §9 |
| FOL5 | NOW | Orig | the 57A20 rows for `m009002`, `m001761`, `m001706` | rows present (classes 1 and 4) |

### 1.6 The High Wall NPC signs through the effects system (part AQ, EFFECT-FAMILIES-SIGNS-NATIVE §5.2; needs the effect system, so ACC unless a counter on the original is taken)

| ID | Kind | Orig/Nat | Read | Expected |
|---|---|---|---|---|
| SG1 | NOW | Orig | the programme names at the commit (`FUN_140264230`) for each of the five `30002` billboards and the distortion | `GXFfxtessellate*Texture` (soft where `depthBlend`, `…GlowD` where `bloom`); `GXFfxdistortionBump` pair for the distortion |
| SG2 | NOW | Orig | `[GXFfxGraphicsResourceManager+0x118/+0x11C]`, `ctx+0x960` and the bank pointer at the commit for three billboards (Add, Normal, FXR value 0) | decides EFFECT-FAMILIES-SIGNS-NATIVE §3.3; baseline: types 2 / 1 / 2 |
| SG3 | NOW | Orig | effect-light registry count (+1 for the sign's light) at 0.5, 5, 40 and 60 m | decides the LOD-child-0 threshold meaning (§2.3) |
| SG4 | NOW | Orig | SRV formats of the sprites | BC7 for `s11636`, `s50907`, `s50908`; BC1 for `s11004` |
| SG5 | NOW | Orig | distortion inputs: t0 = this frame's copy, t1 = `s06000`, blend state, copies per frame | one copy per frame |
| SG6 | NOW | Orig | the sound id handed to the audio subsystem at creation | 30001 once |
| SG7 | ACC | Nat | pixel witness at the High Wall NPC sign | flat additive glow, rising sparks, soft ripple; no black ring (that is the red variant) |

### 1.7 The enemy witnesses W3, W4, W7 (part AR, NPC-W347-NATIVE §6; reads of the native build; the Reference rows need the 57A20 witness)

| ID | Kind | Orig/Nat | Read | Expected |
|---|---|---|---|---|
| NW1 | NOW | Nat | `native_pass_rows` and `native_character_engine7_row` for `c2150` and `c1090` (alternative placements: `m40_00` (309.3, -35.4, 617); `m30_01` (46.9, 41, 190.3)) | rows of NPC-W347-NATIVE §3.3 (`c2150`: row 5 = 2, row 7 = 2; `c1090`: row 5 = 10, row 7 = 1); `flags224` bit 15 set |
| NW2 | NOW | Nat | `native_capability_gate`, `native_selected_constant_unavailable`, `native_selected_texture_unavailable`, `native_recipe_unresolved`, `native_character_geometry_missing` over the witness | none |
| NW3 | NOW | Nat | `native_cloth_*` and `native_selected_palette_unavailable` for the `c5270` cloth meshes (W4, encounter) | none |
| NW4 | NOW | Nat | `cbPerFrame` row 11 on two consecutive frames of a `FlowmapVertexOffset` draw | differs |
| NW5 | ACC | both | image: emissive parts lit without bloom (and without it in the original for EmiIbl), veil flat with alpha, flowmap parts moving | as NPC-W347-NATIVE §3 |
| NW6 | NOW | Orig | `reference_character_row_words` (model +0x220/+0x224) of the same models | bit 15 set |
| NW7 | NOW | both | is the `c5110` boss (entities 4000800, 4000830) drawn before its fight? | decides the W7 procedure |

### 1.8 The glow chain of the emissive (part AS, GLOW-CHAIN-NATIVE §6)

| ID | Kind | Orig/Nat | Read | Expected |
|---|---|---|---|---|
| GL1 | NOW | Orig | the six Glow values (`GXGlowFilter` fields `pipeline+0x2E80`: +0x08, +0x0C, +0x10, +0x14, +0x18, +0x1D) at the High Wall and at m40 | 1, 1.5, 1, 1, 0.5, Enable 1 (mapbase id 0, INFERRED) |
| GL2 | NOW | both | `[[GXRS+0x5368]+0x80]` | 1.0 |
| GL3 | NOW | Orig | GB5 alpha over `c2150` (EmiIblGlow) and over `c1090` (EmiIbl) | `c2150` about 1 over emissive texels, `c1090` 0 |
| GL4 | NOW | Orig | the blend descriptor of a two-target forward draw | RT1 equal to RT0 |
| GL5 | NOW | Orig | one blurred glow level (L0, 320x180) over a bright patch against the native `glow-verify` readback (GLOW-WAVE1-READY §1.3, taps one TARGET texel apart) | equal within R11G11B10 quantisation |
| GL6 | NOW | Orig | the glow target sizes at a presenter size other than 1280x720 | ceil(W/4..W/32) (or a fixed 1280x720) |
| GL7 | NOW | Orig | DXBC hashes bound for glow passes 1, 2, 6 and the vertex shader | `C7E4B6D0`, `8A661248`, `692AF0ED`, `AE3D609D` (GLOW-WAVE1-READY §2) |
| GL8 | NOW | Nat | the four glow programmes resident in repositories `0x4792BF8`/`0x4792D58` at the first native frame | resolved at the first frame |

### 1.9 Veil tessellation and forward shadows (part AT, TESS-SHADOW-FORWARD-NATIVE §5)

| ID | Kind | Orig/Nat | Read | Expected |
|---|---|---|---|---|
| TS1 | NOW | Orig | the `cbTessScene` block (`[[144961B50]+0x4DD0]+0x2D0`) and `cbPNTriangles` (`+0x2C8`) on a `c5270` veil draw (part AW: the conventions are fixed, only numbers remain, TESSSCENE-SSMASK-NATIVE §4) | absolute-world view-projection (with translation), absolute eye, unit vector toward the camera, `GUIParams2.x` = 1.0, four unit inward planes; factors 7 / 3, fade end 20 for `veil` |
| TS2 | NOW | Orig | `view+0x710` while the world view prepares forward rows (hair, water) | 1 or 3 |
| TS3 | NOW | Orig | programme names of the `c5270` veil draws | `Veill_NoLight_…_PN` with HS/DS |
| TS4 | NOW | Orig | the topology of a veil draw | 3_CONTROL_POINT_PATCHLIST |

### 1.10 World shadow system (part AU, SHADOWS-NATIVE §13)

| ID | Kind | Orig/Nat | Read | Expected |
|---|---|---|---|---|
| SH1 | NOW | Orig | `pipeline+0x3EBC`, `[[GXRS+0x5360]+0x28]`, the variant `iVar28` and the `pipeline+0x2DA8` override for the main view | 1, 1, 0 or 1, 0 |
| SH2 | NOW | Orig | `view+0x710` at each forward row (engine pass, shader pass), still frame, player in shade | 1 for pass 0x29, 3 only inside a screen-space window |
| SH3 | NOW | Orig+Nat | the 960-byte `cbSceneParam` and the `cbPerScene` uploaded for one forward row, against the native producers | equal |
| SH4 | NOW | Orig | the sub-pass arguments at the four window call sites in `FUN_140103510` | SHADOWS-NATIVE §7 table |
| SH5 | NOW | Orig | which of the layer `+8` or `+0x10` objects is bound as t32 for a `FwdSSdw` row (part AW: format and channel closed, TESSSCENE-SSMASK-NATIVE §2) | the blurred target when the blur ran; `R8G8B8A8_UNORM`, channel `w` |
| SH6 | NOW | Orig | `[[GXRS+0x5368]+0x128]` and record `+0x24` (blur scale) | > 0 |

### 1.11 FFX effect system design (part AV, EFFECTS-D3D12-DESIGN-NATIVE §6)

| ID | Kind | Orig/Nat | Read | Expected |
|---|---|---|---|---|
| FX1 | NOW | Orig, then Nat | callbacks `140299E40` per engine pass and the sequence of programme indices | equal sets and order; none for pass 0x3A |
| FX2 | NOW | Orig, then Nat | draw count per builder `385F70` / `396F30` / `39E900` per frame at m37, m31, m51 | about 68 / 5 / 6 patch-list draws; strides 112, 36, 160 at m37 |
| FX3 | NOW | Orig | peak ring bytes per frame over 600 frames, five zones | at most 2 MB |
| FX4 | NOW | Orig | distinct {programme set, blend, depth bank, raster bank, RT set, topology} tuples per zone | a small set (PSO prewarm list) |
| FX5 | NOW | Orig | frames with at least one draw of programmes 57-63 / 139-145 | m40 about 47 of 1,800 |
| FX6 | NOW | Orig | on a `GlowD` draw: render-target count, RT1 blend, `g_psGlowScale` | 2 targets, RT1 = RT0 blend |
| FX7 | NOW | Orig | E07 live counts and dispatch groups at m45 | about 1,710 and 5,130 records; 1,770-1,780 groups |
| FX8 | NOW | Orig | m31: `item+0x760`, `item+0x690`, `[anim+0x210]`, `+0x214` for 20 Grass items; equal area winds within one WindSFX region? | not -1, 1, counts 1..8; answers the cache question |
| FX9 | NOW | Orig, then Nat | `[[GXRS]+0x41D0]+0x10` effect-light entries at m37, m50, m51 | equal natively |
| FX10 | NOW | Orig, then Nat | SfxIns active and pending lists and created ids per zone | ids in the ENTRY-DOOR table; pending 0 |
| FX11 | ACC | Nat | order of `E6E750` against the light-tile build | update first |
| FX12 | ACC | Nat | debug layer over 600 frames with depth DSV+SRV and on-demand frame copy | clean |
| FX13 | NOW | Orig | library instance count against handle count | instances greater than handles |

### 1.12 Disagreements settled in static (part BD, D3D12-MASTER-PLAN §6)

| ID | Kind | Orig/Nat | Read | Expected |
|---|---|---|---|---|
| PL1 | NOW | Orig | the D3D11 sampler states created by the YEBIS device (`FUN_141707CB0` table) and bound at the first YEBIS draw of a frame, at m31 | the 16 modes of YEBIS-NATIVE §5.2 (ten with no engine bank row) |
| PL2 | NOW | Orig | the five samplers set by the AO prologue `FUN_1401B22E0` at the first AO draw | bank rows 0, 1, 3, 4, 6 as `s0..s4` (point wrap, trilinear wrap, point clamp, trilinear clamp, white-border point) |
| FC1 | NOW | Orig, then Nat | `CopyResource` of the HDR target per frame at m37 and Firelink (water and distortion), and in a frame with neither | 1 in the first; 1 in the second too (unconditional copy); the native may skip it |
| FC2 | NOW | Orig | the SRV bound as t45 (effects) and t55 (water) in the same frame | the same resource (`pipeline+0x1108`) |
| QG1 | NOW | Orig, then Nat | at m31, still scene, one setting at a time at DISABLE, LOW, MEDIUM, HIGH, MAX: AO draws, AA draw, DoF draws, motion blur draws, glow and glare draws | AO 5 from HIGH; AA from HIGH; DoF 10 at MAX and the library's counts below; motion blur +2 from LOW; glow and glare constant |
| QG2 | NOW | Orig, then Nat | `quality+0x14` and `[144961FBC]` after the first frame at each DoF level, at m31, m40, m50 | `+0x14` = 0, 0, 2, 3, 5; `[144961FBC]` = min(zone, `+0x14`) (m31 5, m40 4, m50 5 at MAX) |
| YB17 | NOW | Orig, then Nat | the DoF draw list and target sizes at each DoF level at m31 (YEBIS-D3D12-DESIGN-NATIVE §9) | 10 draws at MAX; the library's other lists at lower levels |
| YB18 | NOW | Orig | `[144961FB8]` and `[144961FBC]` at m31, m40, m50 and the glare base size | GlareQuality = zone value; the glare base follows it (210×177 or 392×331) |
| PC1 | ACC | Nat | `plan_passes_peak` over 600 frames per zone with the world and all admitted families | under 128 with grouped passes (D3D12-MASTER-PLAN §6 S2) |

### 1.13 FFX wave-1 effects by family (part BM, FFX-WAVE1-READY §8)

| ID | Kind | Orig/Nat | Read | Expected |
|---|---|---|---|---|
| FF1 | NOW | Nat | first-publish log: `light_registry_enabled` of `native_fx_world_published`, `registry+0x110`, the world handle `[[144962360]+8]` ≠ −1, its area record present (`+0xC0[idx] != −1`) | 1, valid, present |
| FF2 | NOW | Orig | at m31: `[mgr+0x30]` (live nodes), the creators of nodes by caller (`140245870` from `D3C4D0`/`D05C80`, `14066F7A0`, others) and the call count of `14025C700` per frame | nodes = items with a handle + other creators; calls = nodes; tells whether FW-3 alone exercises the inline loop |
| FF3 | ACC | Nat | after FW-3: faults on the token page over 600 frames at m40 and m30; calls of `14025B870` per frame | none; 1 per frame |
| FF4 | NOW | Orig | the type registry: presence of `0x27D8`, `0x283C`, `0x283D`, `0x283E`, `0x2713`, `0x261`, `0x2AF8` after `24B260` | all present |
| FF5 | NOW | Orig, then Nat | counts of `140259F60` and `1402552C0` per 600 frames at m31, against the WindSFX effects created (`E6DCD0` with ids 800080..82) | one registration per WindForce action instance (the registrar is the appearance, `140276300`) |
| FF6 | NOW | Orig, then Nat | the world bucket size (`[[reg+0xA0][world idx]+0x10]`) and the sizes of `node+0x120` for 20 grass items at m31 | bucket = live wind entries; node vectors 0..16 |
| FF7 | NOW | Orig, then Nat | `[anim+0x210]` / `+0x214` (area and point counts) for the same 20 items | 1..8 inside a region, equal natively |
| FF8 | NOW | Orig | two items of one region at m31 whose nodes were created at different times, then two created together: are the first area records equal once the fade ramp has passed? | equal for the second pair; the first pair may differ by the fade state; decides whether a cache is exact |
| FF9 | NOW | Orig, then Nat | WindSFX effect creations per 600 frames at m31 (`E6DCD0` id 800081 callers) and the entry lifetimes | the same cadence; tells who re-creates a finite-fade effect |
| FF10 | NOW | Orig | at a frame with shafts (m31, m50) and one without (m37, m40): the keys A/B of index 9 (`context+0x4020/+0x4088`), the weight `+0x40CC`, the record words 0..27 and `1449620A0..F4` | the globals equal the golden for the record in use; explains `Enable` |
| FF11 | NOW | Orig | `post+0x21E0..0x221F` at the same frames | equals `w4..w7` twice and `w24..w27` twice |
| FF12 | NOW | Orig | `[[GXRS+0x5368]+0x50]` and `[GXRS+0x5360]` at m31 | factor 1.0 expected |
| FF13 | NOW | Orig | library instance count against SfxMan handle count (FX13) | instances greater than handles when parents spawn children |
| FF14 | NOW | Orig | at m31 and m40: the wide strings at `post+0x2100/0x2130/0x2160`, `1449622C8/D0`, the argument block of `FUN_1400E5110` (`+0x358/+0x360` and a copy of the blend output at `+0x128`), the two objects' size queries and format, and the texture bound to `LutCopy2DTo3DCs` t0 | gives the prefix, the file and the 16×256 size; closes §4.2 |
| FF15 | NOW | Orig | `144962228..238` at m31, m40, m50, m37 | equal to the grading table of §4.2 |
| FF16 | NOW | Orig, then Nat | the order of `E6E750` against the light collect (FX11) and `native_point_lights` over 600 frames at m40 | update first; 163 |

### 1.14 Shadows of wave 1: cascades, local lights, filtering (part BN, SHADOWS-WAVE1-READY §8)

| ID | Kind | Orig/Nat | Read | Expected |
|---|---|---|---|---|
| SL1 | NOW | Orig | `pipeline+0x3EBC`, `[[GXRS+0x5360]+0x28]`, the variant `iVar28`, `pipeline+0x2DA8` at m30, m40 (and the five zones) | 1, 1, 0, 0 (assumed) |
| SL2 | NOW | Orig | the ShadowParam record the pipeline holds (`88A10` blend result, 0x94 bytes) at each zone | equal to the `mapbase` golden at all seven |
| SL3 | NOW | Orig | the four cascade rasterizer states (`+0xA38..`): depth bias and slope bias values, atlas format | the bias conversion of §2.5 |
| SL4 | NOW | Orig | a copy of the `GXShadowMap` object (0x130 bytes) and the camera record passed to `140239610` for cascade 0..3 at three poses, and the returned matrices | the input layout and the oracle for T-W3 |
| SL5 | NOW | Orig, then Nat | the 960-byte `cbSceneParam` and `cbPerScene` of one deferred and one forward row at m31 | equal natively |
| SL6 | NOW | Orig | `obj+0xC00/0xC04`, `obj+0xC09`, `obj+0xC1D+i` and `[14496231C+i]` over 600 frames at m31, m37 | S = 8192 at MAX; the keep pattern of §5 |
| SL7 | NOW | Orig | quality rows `+0x28..+0x3F` at the five levels | the table of §0.3 |
| SL8 | NOW | Orig, then Nat | the container: `+0x280`, `+0x284`, `+0x288`, `+0x28C..+0x2B4`, item count `(+0x268 − +0x260)/0x1850`, atlas size, at m37, m45, m31, m40 | 48; edge 4096 at the reference; 2 / 3 / 1 / 0 items |
| SL9 | NOW | Orig | per shadowed light: the frame item (`+0xA4`/`+0x1C8`, `+0x9C`/`+0x260`, `+0x7C`, `+0x10` sort key), `ctx+0x280`, `light+0xB1`/`+0x94`, the level chosen, rectangles, `+0x1C` before and after, point vs spot | thresholds of §1.5; the sources of the scalars; spot confirmed at m31, m37, m45 |
| SL10 | NOW | Orig | `[GXRS+0x4E1A]`, `container+0x288` after `FUN_140023790`, `DAT_144961BF5`, `DAT_14473D39C`, `DAT_14473CE59/5A`, and the gate `pipeline+0x78`, `pipeline+0x3EBF`, `[[GXRS+0x5360]+0x44]` | 1, 1; the local-light mode and gate values |
| SL11 | NOW | Orig | the config block values `Draw.PointLightShadowmapResolution`, `Draw.SpotLightShadowmapResolution`, `Draw.DirLightShadowmapResolution` | 256, 512 (defaults) or the reference values |
| SL12 | NOW | Orig | the programme bound for the world mask draw (`PCF4`/`PCF9`/`Soft`) and the blur draws per zone | `Soft` + blur at the reference quality |
| SL13 | NOW | Orig | the mask target format and size, and whether the layer's `+8` or `+0x10` object is bound | `R8G8B8A8_UNORM`, screen size |
| SL14 | NOW | Orig, then Nat | `view+0x710` for every forward row (engine pass) at m37 (water and ghost), m30 (hair), m51 (hair) | the `ShadowFrame` table |
| SL15 | NOW | Orig | `FilterMode` word the mask layer receives and the blur gate inputs `[[GXRS+0x5368]+0x128]`, record `Blur Radius` | 2; > 0 |
| SL16 | ACC | Nat | debug layer over 600 frames with the atlas, the local atlas and the mask plan resources | clean |

### 1.15 Water, refraction/reflection and tessellation of wave 1 (part BO, WATER-TESS-WAVE1-READY §7)

| ID | Kind | Read | Expected |
|---|---|---|---|
| WL1 | order | the forward window (`view+0x710`) at the `0x27` rows and at the 0x1F rows (SHADOWS SL rows) | 1 at the surface (OBSERVED `FwdSdw`), 3 at the `Col` rows of m40 |
| WL2 | order | the GPU order of a frame with water: which forward rows precede `WatDpt`; the HDR `CopyResource` events (expected 2 per frame, 1 without water) and the depth copy | 0x1F rows first; C1 between `WatDpt` and the surface, C2 after the depth downscale (FC1, corrected) |
| WL3 | state | `view+0x891/0x894/0x8A0..0x8AC` after the gather, the frame fields `+0x3F7C..+0x3FA4` and `R+0x3F7C..0x3FA4` before and after the copy of `FUN_140024740`/`FUN_1400885F0`, and where `FB0E0` runs relative to it, at m37 | `n` = (0,1,0), the plane through y = -241.76; the meaning of `t`; whether frame N or N-1 feeds the view |
| WL4 | camera | the reflection view's `cbSceneParam` camera words at m37 against 1.5 | equal to `M · view · flipY` |
| WL5 | options | `[[GX2+0x5360]+0x6C/0x6D/0x6E/0x74]`, `[GX1+0x3E1]`, `frame+0x3EC7/0x3EC9/0x3ECD` at the five zones | `0x6C` set for HIGH, `0x74` set at Firelink |
| WL6 | targets | GB5 around the water draws (two targets bound, one written) | unchanged by the water (the native binds one) |
| WL7 | provenance | the SRVs at t54..t57 of the surface draw (slot range extended to 64): t54 content at m37 and Firelink, t55 = frame copy (C1), t56 = depth, t57 grey or the map; **the alpha channel of the frame copy at water pixels and elsewhere** | as 1.3; alpha 0 on water pixels, non-zero elsewhere |
| WL8 | block | `cbTessScene` and `cbPNTriangles` on a tessellated draw (m50 tree, `c6060`, or the m30 veil: TS1) | the golden words; `g_f4GUIParams2.x` = 1.0 |
| WL9 | rows | the engine rows of a group-0x39 view (the 0x1E rewrite) and the programme names of the three m37 reflection draws | forward `Fwd` programmes, rows 0x1E/0x29 |
| WL10 | gather | the part mask test of the reflection view: which MSB flag the mask 0x10 selects (a part with the flag cleared inside the frustum must be missing) | `DrawByReflectCam` |
| WL11 | plane | the plane against the collision `ReflectPlaneHeight` at m31, m45, m50 | equal at m31/m37; the per-area rule for m45/m50 |
| WL12 | witness | one capture near the water of m31 (50, -155, -580), m45, m50 (22.4, 0, 6.7), m51 swamp: the water programmes drawn, indices | the indices of 1.1 |
| WL13 | data | the GX04 record of `c6060` `Hair_cloth` (DLC chr archive) | mode 2 and its factors |
| WL14 | draw | patch topology draws at m50 near `m001523` | 3-control-point patch list, GBuf_PN programmes |

### 1.16 The other YEBIS passes of wave 1: depth of field, motion blur, feedback blur, auto exposure, history (part BP, YEBIS-SECOND-TRANCHE-READY §7)

| ID | Kind | Read | Expected |
|---|---|---|---|
| YP1 | key | the keys A / B (`context+0x3FE8, +0x4050, +0x4008, +0x4070, +0x4000, +0x4068`) and weights at the five zones, and the record they resolve to (compare the DoF globals `14496211C`, `144962124` and the MotionBlur globals `14496214C..144962158` with 2.1) | `mapbase` values (DoF Enable 1, ApertuerF 2.0; blur time ratio 0.2) |
| YP2 | globals | the full DoF globals block `1449620F8..14496226C` after `FB890`, at the five zones, with the quality levels MEDIUM and MAX | the table of 2.2 with `mapbase` |
| YP3 | inputs | `context+0x413D/+0x4140`, `DAT_144961BF4`, `post+0x2271/+0x2272`, `[GX2+0x5368]+0xC8` and the DoF quality row `+0x10..0x1C` at Thomas's setting | flag clear, levels 7 / 7 at MAX, factor value |
| YP4 | draws | the DoF 10-draw block at m31 and m37 with the record Enable 1 and (forced) 0 | draw count unchanged (10), sizes follow the levels |
| YP5 | globals | the MotionBlur globals with the option ON at m37 (HIGH): `14496214C = 1`, `144962160`, `1449622C0` | 1, recurrences 2 capped by 5, factor |
| YP6 | transition | the MotionBlur record during a key transition: word 12 (stale or lerped) | stale = the DoF word of the same buffer |
| YP7 | feedback | the feedback globals `144962178..18C` at a zone | `Enable` 0 and the file values in some order (Rate 0.8, FallOff 0.9, Power 4.5): the order settles the layout |
| YP8 | exposure | what selects `MapOverride_0001` / `0003` at all (the loader of the override id, the DLC Ceremony rows) and the exposure global over 120 frames in one | the map and the sequence of `144962008` |
| YP9 | read-back | the library's exposure at `[S+0x1F0] + slot × 0x68 + 0x54` against the frame number after a light change | the latency in frames |
| YP10 | focus | `144962120` with the auto focus enabled (a DoF record with the third `Enable` set) | the library's measurement |
| YP11 | history | the two conditions of 2.6 (`[GX1+0xB80]` byte `+0x3C` and count `+0x30`; the parameter byte `+0x4E0`) at the five zones, over a load and a teleport; `post+0x227C`; the after-image pair content after both | both conditions clear in the world view (INFERRED), the histories survive a teleport |
| YP12 | draws | the draw list with the after-image on at m40 (bloom + after-image composite) over a camera cut | the YEBIS-HAL-READY list |

### 1.17 The temporal upscaler: jitter, vectors, reactive mask, history reset (part BQ, UPSCALER-DLSS-READY §6)

| ID | Kind | Read | Expected |
|---|---|---|---|
| UL1 | native | the stub at Quality, Firelink, still camera | sharp, no shimmer; the 18 phases visible in the debug overlay |
| UL2 | native | camera pan with the stub, vectors dumped | vectors non-zero only with motion, unjittered (still camera: all within 0.01 px) |
| UL3 | native | DLSS adapter, pure camera pan | no ghost trail; if there is one, flip the vector sign (`InMVScaleX/Y = -1`) and record which is right |
| UL4 | native | DLAA, still camera | no pixel crawl; if there is, flip the jitter sign |
| UL5 | native | SSAO with jitter on a still camera | no visible shimmer of the AO (the matrices of 3.3) |
| UL6 | native | exposure feed: bonfire to dark corridor | no pumping at the tone map; fallback `AutoExposure` |
| UL7 | native | DoF and light shafts with `DepthOut` against render-size depth | silhouette stair visible or not; decision on 3.7 |
| UL8 | native | motion blur camera velocity with jitter | none on a still camera |
| UL9 | native | what `s8` and `s9` sample (a frame capture by slot) | the sampler set of 3.8 is final |
| UL10 | native | stencil bits 0x40 and 0x80 across a frame at Firelink, High Wall, the DLC maps | never set by any pass except the mask |
| UL11 | native | reactive mask: fog, fire, water, veils | no ghost trail behind a fire or a veil; no softening of the solid next to it |
| UL12 | native | the command list state after the evaluate call | the re-bind list of 3.9 is complete |
| UL13 | native | warp, load, death, cut-scene key change | one reset per event, no smear in the first frames |
| UL14 | native | resolution change and mode change at runtime | the feature recreated, no leak, no stale history |

## 2. TestAnim's save: Cemetery of Ash and Firelink (`m40_00`)

### 2.1 TestAnim's start (−12.3, −54.9, 467.3)

| ID | Kind | Orig/Nat | Read | Expected |
|---|---|---|---|---|
| crystal | NOW / ACC | Orig, then Nat | `c3100` ×3 (MENUS-MAP §69 W5): its draws | forward 0x29, `RecursiveCrystal_Fwd`, two outputs (HDR + GB5); native after NPC bodies (NATIVE-GATES-AUDIT §2.6) |
| R·EL R0–R10 | NOW | both | the effect-system words of EFFECT-LIGHTS §5 | Orig: R1 non-null … R7 counts; Nat today: R1 = 0 / 0, R7 59 |

### 2.2 Firelink Shrine, bonfire view (about 270, −53, 608; the I-0063 `i63-live-01` view)

| ID | Kind | Orig/Nat | Read | Expected |
|---|---|---|---|---|
| R7·EL | NOW / ACC (effects 1) | both | `[[GXRS]+0x41D0]+0x10` and families | Orig 163 (59 + 104); Nat 59 today, 163 after phase 1 |
| F1·ATT | NOW / ACC | both | bonfire `45DD90` → `7CC4B0` → `7CB8F0` | one instance; E01 fire draws |
| F2·ATT | ACC (effects 2.3) | Nat | the held flame (Pyromancy Flame): two lights within 1 m, the instance follows the hand | as the original |
| F3·ATT, P1–P6·E01 | ACC (effects 3) | Nat | effect draws per frame; E01 programmes, patch lists, blends, ring ≤ 2 MB | the m40 listing's counts |
| L2·E05 | ACC (effects 6) | Nat | `GXFfxdistortionBump` presence over the camera path | about 47 of 1,800 frames |
| G1·GLOW, G3·GLOW, G4·GLOW, G6·GLOW | ACC (glow) | Nat | 1 extraction, 4 blurs, 5 composites; forward rows write GB5 (`rt=…/12447`); halo; factor 0 → no glow | as EMISSIVE-GLOW §6 |
| G6·E07, G7·E07 | ACC (E07) | Nat | `CS_VFX_StandardParticle*`, GlowLP; debug layer 600 frames | present; clean |
| W1–W5 | ACC (water) | Nat | water structure, states, t54–t57, image, depth (WATER-NATIVE §1) | as listed; W3 reads t54 at Firelink once on the original (NOW) |
| D1–D6·DEC | NOW (D3 needs a constant capture) / ACC | Orig, then Nat | stencil per pixel; `Decal_DifSpcAm` count; b10/b7/b11 bytes; GB1–4 images; blood after a hit | DECALS-NATIVE §5 |
| H1–H3 | ACC (YEBIS) | Nat | 41 YEBIS draw calls between `PointSampleColorClamp1` and `CustomDofLutCopy`; luminance; brightness 10 and 0 | YEBIS-HAL-READY §8 |
| M1, M2 | NOW (MotionBlur ENABLE) | Orig | M1: camera rotating, player still; M2: camera still, player running (VELOCITY-MOTIONBLUR §5) | target format/size `pipeline+0xE20`/`+0xDF0`, clear value, `g_mCurToPrevScreen`, `Velo` rows, b0 +64, b6, b7, YEBIS order |
| M4 | NOW (MotionBlur ENABLE) | Orig | the first frame after a bonfire warp | `13BFC0` `obj+0x10` reset, NoMove |
| display | NOW | Orig | DISPLAY-SWAPCHAIN §8 rows: gamma pass `+0x28/+0x20`; brightness words; mode lists after opening the options | as listed |
| A6 | ACC (SSAO) | Nat | first frame after a load | no smeared history |

### 2.3 Firelink, boss `c5110` (142.9, −68.9, 594.6) (only if the save's progression allows)

Covers emissive + vertex offset + cloth (MENUS-MAP §69 W7). Read with M3/M5 if MotionBlur is on.

## 3. The I-0063 zone route (`i63-live-10` saves: m31, m37, m45, m50, m51)

Same views as the retained captures. One launch per zone, all rows of that zone together.

| Zone | IDs (kind) | Read | Expected |
|---|---|---|---|
| **m31** | G2·AUDIT (NOW, Nat) | `native_selected_constant_unavailable` for `*_SnowLt`, slots 9/7/2 | refusals with `FC_Snow*` (NATIVE-GATES-AUDIT §2.3) |
| m31 | G3·AUDIT, V1, V3 (NOW / ACC) | POM prepared meshes and draws; GB0–GB4 of POM and `_SnowLt` vs original | POM drawn now; snow after PRODUCERS-UNION §2 |
| m31 | G2·GLOW, L1/L3/L4·SP, Z3, A1/A3·SSAO, S2·SH (ACC) | GB5 over emissive; spot counts and batches; 5 patch draws per frame; SSAO structure and t33; casters per quadrant | per source note |
| m31 | M5 (NOW, MotionBlur ENABLE) | which classes write velocity | opaque only |
| **m37** | G1·AUDIT (NOW, both) | `+0x224` of `Interior` map parts | Orig bit 15 (INFERRED); Nat 0 |
| m37 | V2 (ACC) | `Interior` GB0–GB5 | after the model union |
| m37 | R1·W, R2·W, R3·CHR (NOW Orig / ACC) | one 640×360 reflection view, three draws (5520, 6108, 789 indices), t54; the native reflection gather returns the same meshes | as WATER-REFLECTION §1.3 |
| m37 | X1, X3 (NOW / ACC) | `ExParam_Wet*` rows in rain or water; zero blocks otherwise | CHR-FADE §2 |
| m37 | Z1, Z2, L6·E05, G2·GLOW, L1–L5·SP, A1/A3 (ACC) | 68 patch draws per frame; strides 112/36/160; E05/E04 kinds; spots | per source note |
| **m45** | G1–G5·E07 (ACC) | two systems 2,048/8,192; dispatches; live counts 1,710/5,130; flakes; no zeroed start | EFFECTS-E07 §7 |
| m45 | E1, E2 (ACC), G4·GLOW, A1/A3 | E03 resources and submissions (172 weight) | per source note |
| **m51** | E1–E5 (ACC) | E03 submissions (286 weight); grass 34 draws per frame | EFFECTS-E03 §5 |
| m51 | Z3, L6·E05, A1/A3 (ACC) | 6 patch draws (stride 112); E05 weight 134 | per source note |
| **m50** | A1/A3·SSAO, S2·SH (ACC) | SSAO structure; casters | per source note |

## 4. Situations outside a fixed spot

| ID | Kind | Where | Read | Expected |
|---|---|---|---|---|
| K1–K8 | ACC (Bink) | New Game, any save slot | the opening video: open, first frame, image at frame 240, duration 197.9 s, natural end, skip, audio, no old device | BINK-NATIVE-READY §checks |
| F1–F3·CHR | ACC (NPC fade) | any enemy kill; respawn; fog wall crossing | `group+0x124` ramp; `ditherFadeAlpha` = `group+0x124 × weight`; player fade unchanged | CHR-FADE §1 |
| X2 | ACC | hit an enemy with a damage tint | `ExParam_DamagedNormal_DamageRate` row | CHR-FADE §2 |
| L1·E05 | ACC | a spell or boss attack with a radial blur (the i63-live-33 situation) | PS index 139, t0 = frame copy, t3 = depth | EFFECTS-E05-E04 §5 |
| D6·DEC | ACC | a hit on a wall | `Decal_Blood_DifSpcBump` draw | DECALS-NATIVE |
| D3·FWD, D4·FWD | ACC | soft particles in front of hair; a portrait close-up (Load Game) | faded at the silhouette; hair sharp in DoF | FORWARD-DEPTH |
| red, purple, yellow phantoms | — | the red NpcParam rows (PHANTOMS §1.2) are not on `m30_00`; yellow/purple are online-only | native-only test with a forced PhantomParam row | PHANTOMS §4 |
| E08, line lights, water interaction | — | no witness yet | — | REMAINING-FAMILIES §2 |

## 5. Minimal launch plan

1. **High Wall, Argo, Reference.** §1.1–1.4 NOW rows: G1, G4, gparam ids, P4, T1, T2, T4,
   F1/F2, S1 with an Ember, and P1–P3 if a sign appears.
2. **High Wall, Argo, native.** G1 (native side), T2. Then each ACC row as its step lands.
3. **Firelink, TestAnim, Reference with MotionBlur ENABLE.** M1, M2, M4, the R·EL rows, W3 (t54),
   D3 (a constant capture), the display rows, crystal at the start.
4. **m37 and m31, Reference and native.** G1/G2/G3, R1–R3, X1, M5.

Everything else is ACC and is read in the first run after the matching implementation step.

## 6. Static points closed in part AJ, and what is left to live only

**Closed statically** (with the note that holds the answer):

| Point | Answer | Note |
|---|---|---|
| gparam set loaded for m40 | resident kinds incl. `MapBase` (kind 6), ten Ceremony-based map files, `MapOverride_0000`; Glow resolves to `MapBase` id 0 | PRODUCERS-UNION-SNOW-GLOW §3.3 |
| sign SFX table | RoleParam index 0x45, row `+0x30`: 30001 white, 30011 red, 30021 gold, … | PHANTOMS §2 |
| velocity scale | `FC_DifColMul = (aspect·0.5·k, −0.5·k, 0, 1)` | VELOCITY-MOTIONBLUR §3 |

**Left to live only** (each had a bounded static search with a negative result):

| ID | Question | Read | Expected | Static search done |
|---|---|---|---|---|
| E184 | writer and value of `E+0x184` (draw-entity per-frame velocity flag, `140D079C0`) | `E+0x184` of the player and an NPC with MotionBlur ENABLE; write watchpoint on it | non-zero when the entity may write velocity | all 36 byte writes to `+0x184` in the executable (listing and raw bytes): none is in the draw-entity module (`140D…`); the GX-side hits belong to other classes |
| DC198 | writer of `GXFlverModelDC+0x198` (previous rows pointer of a draw without a pose owner) | write watchpoint on `DC+0x198` (DC = `RDI` in `64340`) during a rigid velocity draw | a pointer to 0x30-byte rows | every DC vtable slot, and all stores to `+0x198` (qword and 16-byte) in `140060000–140068000`, `1401A8000–1401B2000`, `140230000–140242000`, `140290000–1402C8000`: only the constructor zero and a copy |
| TESS | writer of the view block `+0x1130` (`cbTessScene`: `g_f4x4ViewProjection` @0, `g_f4GUIParams2` @96) | the buffer `[[144961B50]+0x4DD0]+0x2D0`, or the view's `+0x1130`, on the `c5270` veil draw | a view-projection | the whole executable: the only writers are the zeroing initialiser and the two copy functions (`139F40`, `13AA60`); no store or address computation in the GX view module |
| MAPBASE | the area under which `MapBase` is registered | the `49D00` call for kind 6 (area, id) | a map area | — |
| ~~LAYOUT~~ | ~~the input-layout formats of the 8 E01 programmes~~ **CLOSED statically (part AM, FFX-INPUT-LAYOUTS)**: 11 layouts for the 158 VS read from the executable tables (`14473EBC0`, `143D22320`), strides 112 / 36 / 160 confirmed, 0 mismatches against the VS signatures | none | none | the capture holds no layout records (the tables are static) |
| ~~CSDOWN~~ | ~~the dispatch site of CS_VFX_*CounterDownSize*~~ **CLOSED statically (part AM, LIVE-REDUCTION-AM §3)**: every GX compute dispatch goes through `1400C5EB0` / `1400C5F40`; their callers are the four E07 sites (emit, update, init sort, bitonic) plus non-FFX post blocks, none selects a CounterDownSize programme | none (the native encoder still logs any CS outside E07 §3, expected empty) | none | exhaustive caller list of both wrappers |
