# I-0110 - Étude RT : géométrie et lumières: result

Status: complete (2026-10-30), study thread, its open points in §9 left to R2 and R4, branch `claude/heuristic-bhaskara-faf567`. Labels: OBSERVED (read in the data
or the code), STATIC (read in the game's code), INFERRED (a reading not yet checked). Evidence: `evidence/I-0110/` (tables of
names, counts and values only; the recovered game files stay in `C:/Users/User/Tools/DS3-Decompilation/gamedata-i0110`, outside
Git, report `evidence/I-0110/recover-report.json`).

## 0. In short: what changes the plan

1. **Mega Geometry is not needed for DS3's geometry** (§4): standard DXR is expected to carry it (INFERRED, measured in R2).
   Thomas agreed (D-0238, 05:25).
2. **DS3's map lighting is mostly baked** (§6): 97 % of m30's map-piece meshes and 89 % of m40's use the `*DOL` and
   `*_lightmap` shaders (OBSERVED), INFERRED the lightmapped family. Once the traced light replaces the raster, the lightmaps' bounce, sky occlusion and baked
   shadows go with it, so R7 (traced GI) carries most of the look, not R4. The lighting pass of D-0237 starts from the light-bank lights
   below (2,408 points and spots in 29 banks over the whole game), which were placed beside the lightmaps, not instead of them.
3. **The full list of a zone's map lights already exists in the engine** (§5): the light container the native reads holds the
   active bank's point and spot lights for the whole zone, not only what the camera sees (m40: 38 points and 6 spots in the
   container, exactly the bank's 38 and 6). R4 needs no new off-screen light walk; it needs to bypass the per-frame cull of the
   item builder.
4. **Off-screen geometry has no list yet** (§3, confirmed): the native only knows what its frustum walks admit. R2's « loaded
   world » list is real work, as the plan says.
5. **Emissive surfaces are few in the maps** (§7): 6 map-piece meshes at m40, none at m30. The visible fire light of torches is an
   FFX effect light, not an emissive texture. Making emissive textures true emitters is cheap but changes little; the effect
   lights (raster effects, D-0237) must join the traced light list.

## 1. Method

- The native code on `main` (`4b99b292`), read with a Sonnet subagent and the claims a design relies on checked at the source
  (cited below).
- Game data recovered read-only with I-0068's `Recover-NamedGameData.ps1` (pinned BinderTool, dictionary and game hash): 1,021 of
  1,527 names (the misses are guessed DLC names): every BTL (29), BTAB (23) and BTPB (22) light bank of the base game and both
  DLC, the 942 map pieces of m30_00 and m40_00, both MSB, `c1105` (an enemy) and the armour parts `BD_M_1000`, `HD_M_1000`.
- Read with SoulsFormats 1.0.1: [tools/Get-BtlCensus.ps1](tools/Get-BtlCensus.ps1) (`btl-census.tsv`, one row a light),
  [tools/Get-FlverCensus.ps1](tools/Get-FlverCensus.ps1) (`flver-m30.tsv`, `flver-m40.tsv`, `flver-chr.tsv`, `flver-parts.tsv`,
  one row a mesh) and [tools/Get-MsbCensus.ps1](tools/Get-MsbCensus.ps1) (`msb-m30.tsv`, `msb-m40.tsv`, one row a placed part);
  MTD parameters from I-0068's `all-mtd.tsv`. The light-bank loader read with GhidraMCP (§5.1).
- Reviewed by a fresh step reviewer (2026-10-30): the counts recomputed from the tables; its corrections are in this text
  (the BTL census reran with names' tabs escaped, which had shifted two rows).

## 2. Geometry for the BLAS (R2, R5)

### 2.1 What the native already holds (OBSERVED in code)

- **One GPU buffer per FLVER model**, vertices and indices together, in DEFAULT memory, left in
  `VERTEX_AND_CONSTANT_BUFFER | INDEX_BUFFER` (`14_scene_step_and_storage.inc:245-261`); the CPU bytes are kept with the storage
  (`NativeModelStorage::bytes`, `01_state.inc:384`). Keyed by the engine wrapper (`model+0xE8`), retired once submitted GPU work
  completes. A BLAS build reads it as a non-pixel shader resource: one barrier, no copy.
- **Per mesh a slice**: vertex offset, count and stride, index offset and count, material, a second stream, the cull class and
  the index width (`native/renderer/content.h:10-27`, `game_scene_geometry.cpp:76-85`). Indices are slice-local (no base vertex).
- **Only the LOD-0 face set is kept** (the face set whose first word is 0; a second one refuses the model;
  `game_scene_geometry.cpp:62-64`). **Topology must be a triangle list** (field +4 = 4), indices R16 or R32 (`:67-70`).
- **POSITION is float3 at byte 0 of stream 0** in every map piece, character and armour mesh scanned (OBSERVED in the data, §2.2),
  and the native's own validation already assumes it (`content.cpp:39-41`): `DXGI_FORMAT_R32G32B32_FLOAT` straight into
  `D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC`, stride from the slice.
- **The world transform is a row-major 3x4**, the layout of `D3D12_RAYTRACING_INSTANCE_DESC::Transform`
  (`NativeSelectedModel::placement`). **The previous frame's world matrix is not kept** for ordinary models: `cbvModelParam`
  gets the placement twice (`13b_prepare_model_frame.inc:607-619`); only cloth keeps a pair. Motion vectors of moving objects
  and a TLAS's previous transforms (for DLSS and Ray Reconstruction) need it added.
- **Skinning runs in the vertex shader** from a CPU palette (3x4 rows, up to 4,096, current and previous pose ring; bound b5,
  b6, b12 cancel), indices addressing the engine palette directly (no mesh-local remap; `palette_base` 0). R2's compute skinning
  reproduces `pos = b5 · b12 · v` with the four BLENDINDICES (R8G8B8A8_UINT in the data) and four weights (UNORM8).
- **CPU-rewritten geometry**: physics streams (one upload buffer per mesh, `UpdateNativePhysicalModelStreams`) and cloth (whole
  copy re-uploaded each simulated frame, `NativeClothVertexUpload`), both with a generation counter: the refit trigger is there.
- **Wind and grass are vertex-shader animation** (`VC_ForceParam`, `cbvModelParam +0x80`); no instancing of models (every draw
  one instance, `model_draw.cpp:591`); engine-instanced models (`entity+0xA48` records, detected at
  `game_model_selection.cpp:172-176`) are refused at admission (`09_model_admission.inc:204`), so they are absent from the raster and from any future BLAS alike.
- **Tessellated materials** (`3_CONTROL_POINT_PATCHLIST`) draw a displaced surface the BLAS will not have (behind
  `native-tess.flag`, off by default).

### 2.2 What the data contain (OBSERVED)

| | m30_00 (High Wall) | m40_00 (Firelink) | `c1105` | armour `BD`/`HD_M_1000` |
|---|---|---|---|---|
| Model files / meshes | 605 / 3,605 | 333 / 1,296 | 1 / 15 | 2 / 13 |
| Skinned meshes (`Dynamic`) | 0 | 0 | 15 | 13 |
| POSITION | float3 @0, all | float3 @0, all | float3 @0 | float3 @0 |
| Index width (LOD 0) | 16, and 32 for 14 large meshes | 16, and 32 for 88 large meshes | 16 | 16 |
| Strips | none | none | none | none |
| Two-sided meshes (LOD 0 without back-face cull) | 493 (14 %) | 148 (11 %) | 12 of 15 | 11 of 13 |
| Face sets | LOD0 only (2,373), LOD0-2 (1,206), LOD0-1 (26) | LOD0 only (521), LOD0-2 (741), LOD0-1 (34) | LOD0-2 and their motion-blur twins (6) | same (6) |
| Unique LOD-0 triangles | 14.8 M | 16.1 M | 30,700 | 41,700 |
| LOD 1 / LOD 2 triangles | 3.1 M / 1.3 M | 6.2 M / 2.3 M | 18,400 / 6,100 | 25,000 / 8,300 |

- **Instances** (MSB, OBSERVED with SoulsFormats 1.0.1, `msb-m30.tsv`, `msb-m40.tsv`): m30_00 places 2,273 map pieces (605
  models), 2,496 objects (116 models) and 156 enemies; m40_00 places 4,057 map pieces (337 model names, 333 with a recovered
  file), 539 objects and 100 enemies. 1,761 of m40's map-piece instances and 80 of m30's have a scale other than 1 (six
  significant digits). **Placed LOD-0 triangles**: 24.4 M at m30, 24.7 M at m40, 1.6 and 1.5 times the unique ones (the large
  models are placed once; on average a map-piece model is placed 3.8 times at m30, 12.2 at m40). The visible set per frame is far
  smaller (about 1,000 meshes, STATE, I-0095).
- **Part flags a TLAS must honour** (MSB fields as SoulsFormats 1.0.1 names them, not yet confirmed in the engine): a draw-group
  and display-group bit set per part (38 distinct sets at m30, 222 at m40: what the engine shows depends on where the player
  stands), `IsShadowOnly` (8 map pieces and 247 objects at m30: shadow casters the camera never sees, an instance mask for shadow
  rays only), `DrawByReflectCam` false (91 at m30, 157 at m40: hidden from the water's reflection), `DrawOnlyReflectCam` (8 at m30:
  reflection-only stand-ins), `LodParamID` (the LodParam row giving the part's LOD distances; 22/23/24 on most m30 objects).
- **Alpha test** (MTD, OBSERVED): the `_e` materials carry `g_AlphaRef` 128 (`M[ARSN]_l_e`, `_e_grass`, `_e`, `_l_e_env`), 10 for
  `_3m_edg_l`; m30: 324 opaque meshes with a non-zero `g_AlphaRef`, m40: 155. These need an any-hit shader or opacity
  micromaps (DXR 1.2, available, I-0109) with the material's diffuse alpha. The blended materials (`_al`, `_add`, water
  `GXFlver_WaterBumpTri`, `g_BlendMode` 2 or 4: 20 meshes at m30, 12 at m40) stay out of the opaque BLAS (plan §1: transparents stay raster).

### 2.3 Consequences for R2 (INFERRED)

- A BLAS per (model storage, mesh slice), from the LOD-0 face set the native already selects; compacted. Static map pieces never
  rebuild. Map pieces are placed several times: the TLAS instances share one BLAS (scale in the instance transform).
- LOD: the engine cross-fades between LOD variants (`SelectModelFade`, two intents during a fade); the native draws LOD-0 only.
  R2 can do the same (LOD 0 for every instance), costing 15 to 16 M unique triangles a zone; LOD 1 and 2 BLAS add about 30 to 50 %
  if far instances should use them. Decided by R2's measure (§4).
- Characters: 30,700 (`c1105`) and 41,700 (two armour parts) measured; about 100,000 with every armour part, estimated; skinned in compute into a per-instance buffer, then a BLAS refit
  (`PERFORM_UPDATE`) per frame, a rebuild on a LOD or pose-topology change. Cloth and physics streams: refit on their generation.
- Wind-animated grass and foliage (`g_GXFT_WindEffect`): either the static rest pose in the BLAS (the shadow lags the sway), or
  the wind applied in a compute pass with a refit; to choose on sight (Thomas's eye) in R2.

## 3. Off-screen geometry

Confirmed: no list of loaded instances exists natively (`05_selected_scene.inc:166-167`, « no loaded-model scan »). The walks are
the camera frustum (a 1.5 m margin), the sun cascades (mask 0x02), the local lights (0x04) and the water reflection (0x10), each
through the engine's own culler; registries of admitted storages, poses and streams exist but carry no placement. R2 needs a walk
of the loaded world with an infinite (or large-radius) culler through the same engine traverser, honouring the display groups
(§2.2).

## 4. RTX Mega Geometry for DS3 (Thomas, 05:25: « je sais pas si Mega Geometry serait pertinent »)

Verdict: **not needed; R2 in standard DXR, measured** (the coordinator's proposal, confirmed; D-0238).

- What Mega Geometry pays for: millions of dense triangles rebuilt or streamed (clusters of at most 256 triangles, Nanite-like
  scenes, dense vegetation), and very many instances (the partitioned TLAS). DS3 has neither: 15 to 16 M unique static triangles a
  zone that never rebuild, 2,000 to 4,000 placed map pieces plus 500 to 2,500 objects, a few dozen skinned characters of 30,000
  to 42,000 triangles measured (about 100,000 estimated for a fully armoured one). A full TLAS rebuild of about 10,000 instances and a few dozen refits a frame are the standard DXR workload
  every RT game of this size carries (INFERRED, to measure in R2).
- Its costs: the DirectX version is a preview (Agility SDK 1.722.0-preview, SM 6.10, Windows developer mode); the other route is
  NVIDIA's NVAPI extension; both NVIDIA-specific. Thomas keeps the original geometry (no HD models), which removes its main payoff.
- Where it could still help: BLAS memory (15 to 16 M triangles a zone, plus a neighbouring zone while streaming, is of the order
  of 1 GB compacted, INFERRED) and LOD switching. R2 measures the per-frame build and refit time, the BLAS memory and the LOD
  switches on the witness zones; the acceleration-structure layer keeps one interface so another backend can join if one of them
  weighs.

## 5. The maps' light sources (R4, the lighting pass)

### 5.1 The light banks (OBSERVED, `btl-census.tsv`)

2,431 lights in 29 banks (`map/mXX_XX_00_00/mXX_XX_00_00_000N.btl.dcx`): base game 2,172 lights in 24 banks, DLC1 127 (m45 122, the arena m47 5), DLC2
132 (m50 41, m51_00 87, m51_01 4); the arenas hold 1 to 5 each. Per light (SoulsFormats 1.0.1 names; the unnamed words' meaning INFERRED from
their values): name, position, rotation, diffuse colour and brightness, specular colour and brightness, radius, cone angle, width;
`Unk18` the type (0 point: 2,174; 1 spot: 234; 2 directional: 23, at most one a bank: m35 0001, m38, both m47 banks and m40
0001 have none, m32 0001 is empty); `Unk27` true on 44 lights (23 directional, 12 spots, 9 points): INFERRED the
shadow-casting flag (I-0106 reads the engine's own flag; to match). Typical values: radius 4 to
12 m, diffuse brightness 0.5 to 3.

| Zone | Lights (bank 0000) | Points | Spots | Shadowed (`Unk27`) | Bank 0001 |
|---|---|---|---|---|---|
| m30_00 High Wall | 185 | 154 | 30 | 5 | 133 |
| m30_01 Lothric Castle | 116 | 94 | 21 | 2 | 92 |
| m31_00 Undead Settlement | 161 | 154 | 6 | 2 | none |
| m37_00 Irithyll, Anor Londo | 524 | 478 | 45 | 3 | none |
| m39_00 Irithyll Dungeon | 264 | 250 | 13 | 5 | 39 |
| m40_00 Firelink | 45 | 38 | 6 | 3 | 41 |
| m45_00 Ariandel | 122 | 111 | 10 | 1 | none |
| m51_00 Ringed City | 87 | 78 | 8 | 1 | none |

- **The directional light of each bank is not the game's sun** in the native (the sun comes from the area's GPARAM,
  NATIVE-LIGHTING-MAP « Sun source »); INFERRED the bake's key light, kept in the bank. m39's bank 0001 holds a directional
  « KeyLight_ID1 » of brightness 50.
- **Two banks for some zones** (0000 and 0001: m30_00, m30_01, m32 (its 0001 empty), m33, m34_01 (both 82 lights), m35, m39,
  m40). At m40, 0001 drops two of the opening's spots (`Spot_冒頭_001` and `_004`, « opening »; it keeps 002, 003, 006 and 007)
  and the directional light, and adds two « 動的闇 » (« dynamic dark ») points: INFERRED a second lighting state of the zone; which bank
  the engine loads when is open (§9).
- **Names say the role**: `Omni*`, `動的_*` (« dynamic »), `Spot_*`, `KeyLight`, `Direct*`. Many points are cold fill lights placed
  beside the lightmaps (the m30 torch: only fill lights of colour (197, 227, 255) near it, MENUS-MAP §54). These are the lights the
  lighting pass of D-0237 will retune.

**The loader** (STATIC, `FUN_140018490`, GhidraMCP 2026-10-30): it walks the bank's 0xC8-byte records into the container
`[[GXRS]+0x41D0]` (its first argument), placing each by the area's world transform (the area object's `+0x5C0..+0x5EC`). Type
(record `+0x18`) 0 builds a point (`FUN_1400EFCB0`, 0x1B0 bytes), 1 a spot (`FUN_140018380`, then `FUN_1400F10C0` for its cone);
**type 2 (directional) builds nothing**: the bank's key light is never a runtime light. Record to light, by offset: `+0x00..+0x0C`
→ `+0xBC..+0xC8` (the four words SoulsFormats leaves unnamed); `+0x1D..+0x1F` diffuse RGB bytes /255 and `+0x20` diffuse
brightness → `+0x70..+0x7C`; `+0x24..+0x26` specular RGB and `+0x28` brightness → `+0x80..+0x8C`; **`+0x27` → `+0xB1`** (the
44 « shadow » lights, 21 of them points or spots; INFERRED the shadow request, which the item's `+0x1C8` gates in the shadow slot `FUN_140214350`);
`+0x38..+0x40` position; `+0x58` radius (the point's sphere w); `+0x74..+0x7C` → `+0x184..+0x18C` (point) or `+0x200..+0x208`
(spot), INFERRED the flicker words; `+0x98`, `+0x9C` → `+0x98`, `+0x9C`; byte `+0xA0` → `+0xAC` (the « legacy » point model when
1, NATIVE-LIGHTING-MAP); byte `+0xA1` sets bit 0x10 of the category word `+0x60`; `+0xA2`/`+0xA3` → `+0xB4`/`+0xB2`; `+0xA4` →
`+0x90` (20.0 by default in old banks); `+0xC4` → `+0xB8` (the light's own distance the native's fade reads); a spot with
byte `+0xA8` set gets `FUN_1400F1440(+0xB0)` (INFERRED its projected texture). Two callers: the area's load (`FUN_140931F30`,
its resource `+0x1F0`) and `FUN_140652EB0`, which builds a bank name from an ID (`FUN_140651450`): INFERRED the way the second
bank (0001) or a light-bank change reaches the container. Either way the container is the runtime truth R4 reads.

### 5.2 The engine's runtime list (OBSERVED in earlier native witnesses, matched here)

The native already reads the engine's light container `[[root]+0x41D0]` (NATIVE-LIGHTING-MAP « Point/spot lights », the point
class vtable `0x143D4E808`, the spot class `0x143D4E8E8`, items built by their virtual `+0x30`). At m40 it held 44 lights
(Native1051-1055): 38 points and 6 spots, **exactly m40's bank 0000 without its directional light**. So the container holds the
whole active bank of the loaded zone, independent of the camera; the per-frame culling happens in the item builder (`+0x30` returns
null for a culled light). R4 reads the container, takes the lights' world data from the objects (as the native already does for
the tiles) and skips that cull. The fade by the SettingInfo blend and the colour formula (`c²·k·scale·fade`) are already
transcribed (NATIVE-LIGHTING-MAP « Native point lights »).

### 5.3 Other light sources

- **The sun and sky**: GPARAM (the area's own binding, native since 2026-10-13).
- **FFX effect lights** (torch and bonfire flames, spells): point lights carried by effects (I-0068 EFFECT-LIGHTS, the native's
  effect light registry). The warm torch light of m30 is one of them (MENUS-MAP §54). They must join R4's list; under D-0237 the
  effects stay raster but their lights become traced.
- **Light probes** (BTPB, 22 banks, 23 MB) and **lightmap atlases** (BTAB, 23 banks): baked, for characters and objects, and for
  the map pieces (§6). Both disappear under RT; their data can serve as a look reference.

## 6. Baked lighting: what the traced light replaces

| Map-piece shader family (MTD) | m30 meshes | m40 meshes |
|---|---|---|
| `GXFlver_ColDifSpcBumpDOL` (`M[ARSN]_l`, `_l_e`, `_l_m`, `_l_env`, `_l_al`) and the other `*DOL` shaders (grass, snow) | 3,186 | 1,071 |
| `MultiBlend*_lightmap` (`_2m_l`, `_3m_l`, `_4m_l`) | 323 | 88 |
| without lightmap (`Ibl`, `Col`, `Grass`, water, NoLight, no MTD) | 96 | 137 |

INFERRED (shader names, earlier native work on « the lightmap delivery », t26): `DOL` is the lightmapped family. 97 % of m30's
map-piece meshes and 89 % of m40's carry DS3's baked light, of which the traced picture keeps nothing. This is the main reason a
traced DS3 will look different (D-0237's « le baked est triché »), and why R7 (GI) is where the look returns, with the lighting
pass tuning lights only once GI is on. Suggested order for the lighting pass: judge zones after R7, not after R4 alone.

## 7. Emissive surfaces

- G-buffer emissive: `GXFlver_ColDifSpcBumpEmiIbl*` and `*EmiDOL*` (`g_EmissiveTexture` t6, `FC_EmissiveColor`), written to
  GB5 and added in the composite (EMISSIVE-GLOW-NATIVE §3). Forward glow: `*EmiIblGlow_Fwd` (dropped natively so far).
- In the map pieces they are rare: m40 6 meshes (`M[ARSN]_em_Glow`, `_l_em_Glow`), m30 none (OBSERVED). The torch bowls
  (`o304863`, `M[ARSN]_em`) are objects; eyes, lamps and auras are characters, parts and effects.
- Consequence: under RT the G-buffer's emissive is already exact for what the camera sees; for reflections and bounces the
  material table of R3 needs the emissive texture and colour. Making them ReSTIR emitters (Thomas's lead, D-0237) costs little
  (few triangles) and changes little; the effect lights (§5.3) matter more for the warm light of fires.

## 8. Uncertainty

- The BTL fields: where each lands in the light object is read in the engine's loader (§5.1, STATIC); what the engine does with
  `+0xB1` (the shadow request), the flicker-like words and `+0x98`/`+0x9C` is INFERRED from their values.
- The MSB part flags are SoulsFormats 1.0.1's names (its `IsShadowOnly` reads true on every enemy, a sign the names do not fit
  every part type).
- Two scans are of m30_00 and m40_00 only; the triangle counts of other zones are not measured.
- The BLAS memory figure is an estimate, not a measure.

## 9. Next static steps (open)

1. The BTL loader is read (§5.1); left: the link from `+0xB1` to the item's shadow request `+0x1C8` (in `FUN_1402136B0`), the flicker words' use, and
   which bank (0000 or 0001) the second loader `FUN_140652EB0` takes when (a map state, the « LightingQuality » option whose
   string exists, or an event).
2. The LodParam table: the LOD distances per part, and whether far LODs or impostor pieces exist that a traced reflection would
   show instead of LOD 0.
3. The group masks: the native's main walk already applies the engine's own test, the view's display-group words
   (`view+0xE0`) against the entity's world mask (`E+0x70`), its category byte (`E+0x20`) and the `D03430` visibility words
   (`09_model_admission.inc`, read by the native subagent). Proposal for R2 (INFERRED): the loaded-world walk keeps exactly these
   tests and drops only the frustum, so the rays see what the game would show from the player's place (a part hidden by its
   groups, an interior seen from outside for instance, stays out of the rays as it stays out of the picture). Left: whether a
   shadow-only or reflection-only part (§2.2) is told apart by these bits or by another field.
4. The engine-instanced models (`entity+0xA48`): **never met** (OBSERVED): in the 1,367 recorded native runs whose log has the
   main walk's outcome rows (`native_main_walk_outcomes`, sampled), `instance_streams` is 0 every time. Not a gap for the rays
   in the zones played so far. The plan's « l'herbe instanciée de m51 refusée par le natif » (RT-PT-PLAN §2) has no witness in
   the code or the logs read here (the native's subagent found no m51 refusal): to check on an m51 run before R2 counts it.

## 10. Side reading for I-0105 tranche 10: the High Wall's snow record textures (2026-10-30)

Asked by the coordinator to help the effects pilot (the five textures t47..t51 the original takes from the blended `Snow`
record at the High Wall; the record's words are non-null there, null in the integrated record at Firelink, `i105-snowtex-ref-2`).
Data-first, the game not started:

- `map/m30/m30_9999.tpf.dcx` (OBSERVED, recovered read-only, `recover-systex-report.json`) holds exactly ten textures:
  `m30_00_Deposit_A_0000`, `_M_`, `_N_`, `_R_`, `_S_0000` and the same five for `m30_01`. The m30 map packs
  (`m30_0000..0003.tpfbhd`) hold no snow-named texture; `other/systex.tpf` holds `snow_a`, `snow_h`, `snow_m`, `snow_n`, `snow_r`,
  `snow_s` (512x512), the root's fallbacks.
- The executable holds the wide strings `%sDeposit_a_%04d` (`0x143D517E8`), `%sDeposit_m_%04d` (`0x143D51870`) with `_n_`,
  `_r_`, `_s_` beside them, `maptpf:/%s/%s_9999.tpf` (`0x1427D60F8`) and the record's default `m22_00` (`0x143D4EF10`).
  INFERRED: the record's textures are named from its area prefix (`+0x28`, « m30_00 ») + `Deposit_<a|m|n|r|s>_` + the GPARAM
  `Snow / Texture:ID` as `%04d` (0 everywhere, so `_0000`); the integrated record keeps `m22_00`, which has no texture.
- The two m30 snow materials (pieces 003970 and 008111) bind only `g_SnowHeightTexture` = `Other/SysTex/snow_h` in their FLVER
  parameters: t47..t51 come from the record or the root, as the pilot read.
- Confirmed by the effects pilot's GhidraMCP reading (STATIC, its thread « Dessiner la neige sur les surfaces »): the writer `131EA0`,
  called each frame through `12F350` from `499C0` and retried while a word is null, looks up `%sDeposit_<x>_%04d` from the
  record's `+0x28` string and its Texture:ID in the repository `[root+0x47C8]`; words `+0x00` a, `+0x08` n, `+0x10` r, `+0x18` s,
  `+0x20` m. The native code is the pilot's (I-0105 tranche 10).

## 11. R2 prepared ahead

The plan for R2 (2026-10-30, Thomas's « Oui go » at 09:45): [R2-PLAN](R2-PLAN.md): the RT world apart from the camera's
lists, inline ray queries, six tranches (the BLAS registry at admission, the loaded-world instances and the TLAS, skinning and
refits, the alpha test, a traced control view checked against the raster depth, the measures of D-0238), one choice for Thomas
(the radius of the loaded world in the rays).
