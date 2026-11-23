# I-0113 tranche 7: the constants the native leaves zero (census)

Thomas, 2026-10-31 05:28 (« on fait ça »; his question at 05:26: list every parameter of our renderer that falls to zero): list every
shader constant the native leaves zero where the original fills it, the class of tranche 6's defect (Anor Londo's POM wall: its
GX02 item unread, b2 +0x40..+0x5F zero, K-0487). §1 was written by the tranche 6 thread; §2 to §4 by the tranche 7 thread
(2026-10-31 05:40-06:40), static: the decompiled original, the native's code, the game's FLVERs and programmes read offline.

## 1. The material GX items: 141070 against the native's readers

141070 (`decompiled/DS3-1152-F-00141070.c`), the GX item parser, does not run natively. The native reads the items in four places:
`18_resource_admission.inc` (« C2 », into the same material fields), `content/texture_tiling.cpp` (`ReadTextureScrollRecords`: the
scroll items), `content/model_parameters.cpp` (`ReadModelDamageParameters`: GX80-82; GXMD) and `07_scene_lighting.inc` (GX15, the
SSS profile). Tags are little-endian (0x32305847 = « GX02 »). « Material » offsets are 141070's `param_1`; 22E930 reads them as its
`param_3` = material +0xD0 (so material +0xDD is 22E930's flag +0x0D).

| Item | 141070 writes | Feeds (22E930 unless said) | Native |
|---|---|---|---|
| GX00 | 14022B7C0 on +0xD0 (Ex Param: Decal, DisableCasting, SlotNo, CullingMode) | the admission's class and no-cast | read |
| GX01 | +0xF8, flag +0xDC | b2 +0x30 FC_BlendMaskParam (22E930's flag +0x0C, or +0x19 from GX13) | read |
| GX02 | +0x110, +0x114, +0x120, float +0x118, +0x11C; flag +0xDD | b2 +0x40..+0x5F (POM) | read (tranche 6) |
| GX03 | +0x104, flag +0xDE | FC_EmissiveColor | read |
| GX04 | tessellation +0x250..+0x278, +0xDF | the deformation choice | read |
| GX05 | +0x108, flag +0xE0 | FC_TranslucencyScale | read |
| GX06 | +0x10C, flag +0xE1 | b2 +0xA0 FC_SnowMaskParam | read |
| GX07 | 14022C180 (water) | b2 +0x110..+0x17F | read |
| GX08 | 14022C420: +0x1AC, +0x1B0 (clamped), flag +0xE5 | b2 +0xB0 FC_FROZEN_THRESHOLD | not read; **no material carries it**, no programme reads +0xB0 |
| GX09 | +0x200..+0x20C, flag +0xE3 | FC_ParallaxParam | read |
| GX12 | +0x124, +0x128, +0x134, float +0x12C, +0x130; flag +0xE8 | b2 +0x70 (POM second layer) | read (tranche 6) |
| GX13 | +0xF8..+0x100, flag +0xE9 | b2 +0x30, +0xC0 FC_BlendRate (burn) | read |
| GX14 | 22BD90 | FC_DynamicDiffuseColor (sky) | read |
| GX15 | 14022BF70 on +0xD0 (the SSS link) | b2 +0x190 FC_SSS_ID | read (`NativeSssProfileOf`, 07_scene_lighting) |
| GX16 | 14022C2B0: the phantom block (+0x22, +0x70/+0x71, +0x80..+0xCF) | b2 +0x1A0..+0x1EF | not read; **no material carries it** (the native's phantom policy stands) |
| GX17 | +0x410, flag +0xF3 | b2 +0x1F0 VC_VelocityOffset | not read; **no material carries it**, no programme reads +0x1F0 |
| GX20..GX34, GX38 | one bit of the mask +0x284 and an entry {type, 8 bytes}: per-texture-slot UV scroll speeds (WHEEL-STUDY §2) | the texture scroll | read (`ReadTextureScrollRecords`) |
| GX35 | +0x3C8, flag +0xEB | b2 +0x90 FC_EnvMapDolBlendRate | not read; 58 materials carry it (`M[ARSN]_l_env`, mostly the High Wall), **no programme reads +0x90** |
| GX36 | +0x3D0..+0x3DC, flag +0xEC | b2 +0xD0 FC_FlowMapUvTiling | **read since tranche 7** |
| GX37 | +0x3E0..+0x3EC, flag +0xED | b2 +0xE0 FC_FlowMapIntencity | **read since tranche 7** |
| GX39 | +0x3F0..+0x3FC = data word 0 four times, flag +0xE6 | b2 +0xF0 FC_SoftVolume | **read since tranche 7** (the block existed since I-0105, its flag was never set) |
| GX40 | +0x400..+0x40C = data word 0 four times, flag +0xE7 | b2 +0x100 FC_PointCloudRate | **read since tranche 7**; no material carries it |
| GX80, GX81, GX82 | +0x4A0 and flag +0xF4; 14022BB90, 14022BC90 (+0x3D4/+0x3D5, colours, the MTD providers, flags +0xF5/+0xF6) | b2 +0x200..+0x22F FC_DamagedNormal* | read (`ReadModelDamageParameters`) |
| GX90 | tessellation (as GX04) and +0xE4 | the deformation choice | read |
| GXMD | 14022C4B0 | the generator values (+0x1A) | read (model_parameters) |

After the items, 141070 also acts on the MTD's flag word (gxMtd +0x88) and the resource's flags: bit 27 or resource flag 2 →
+0xF7 (b2 +0x230..+0x26F, the wet block: the native's `SelectModelWetActivation`); bit 11 → 14022C0D0 (the SSS link, the same
registry); bit 9 → +0xE5 (b2 +0xB0, read by no programme); bit 0 → +0x108 = 0, +0xE0 = 1 (b2 +0x10 = 0; natively a clear flag +0x10 writes 0 there even over a generator value, where 22E930 keeps it: latent, the authored
generators start at register 0x27 or above); bit 26 or resource flag 1 → the phantom block's defaults
(the native's phantom policy, P3).

## 2. Which items DS3's materials carry (OBSERVED, every FLVER)

`Get-I113GxCensus.ps1` (2026-10-31 05:42-05:54, read-only, the game not started): every archive entry of Data1-Data5, DLC1 and DLC2
whose dictionary name is a map, character, object, parts or effect binder, and every unnamed entry that is a DCX binder holding
FLVERs (the DLC pieces), read in memory with the pinned tools of `Recover-GameDataByName.ps1` and SoulsFormats 1.0.1: **11,770
FLVERs, 49,574 materials**, no FLVER unreadable (169 DCX entries are not BND4 binders: that run did not tell named from unnamed ones apart, the script now counts them apart). Data0 is not read: the pinned BinderTool
has no key for it (it holds no model binder; not checked further). The table: `evidence/I-0113/zero-constants/gx-census.tsv` (one
row per material, the items with their version and first data words; in that run the columns are separated by the two characters « `t », the script now writes tabs).

The items 141070 parses and the native did not, with their weight (materials counted once per archive, so an effect present in a
base archive and a DLC counts twice):

| Item | Materials | Data | Who reads the word it feeds (§3) | Effect natively before tranche 7 |
|---|---|---|---|---|
| GX35 | 58 (`M[ARSN]_l_env` 54, `M[ARSN]_l_e_env` 4; m30 33, the DLC pieces 20, m34 3, m41 2) | 1.0 | no programme | none |
| GX36 | 4 (`c5110[FlowDisplacement]` of c1320 « Crystalball », c1321, c3140, c3141) | (1, 1) or (20, 10) | no programme | none |
| GX37 | the same 4 | (0.93, 0.5), (1.33, 0.9), (0.35, 0.15) | 10 PS of the FlowDisplacement family | the PS read words nothing produced: the draw refused (`native_selected_constant_unavailable`), not witnessed |
| GX39 | 55 effect materials (`S[A]_add_Soft` 36, `S[ARSN]_em_Soft` 7, …); 51 carry 0 | 0.3 for the m50 effects s09540 and s09541 (Data1 and DLC2) | 80 PS (`*ZFeather*`) | FC_SoftVolume 0 for those two effects (the others are 0 in the data) |

GX08, GX16, GX17 and GX40 are carried by no material. So of the items 141070 parses, the native now reads every one a material
carries and a programme reads; GX35 stays unread (no programme reads +0x90: reading it would change nothing).

## 3. Which programmes read those words (OBSERVED, RDEF)

The RDEF of the 11,641 programmes cached by I-0063 (`C:/Users/User/.codex/I0063-cache/programs`, the game's own; the reader is
I-0106's `tools/rdef.py` logic): cbMatDynParam's variable at each offset, how many programmes declare it and how many use it.
Pixel programmes (4,196, 2,913 with cbMatDynParam):

| b2 offset | Variable | Used by |
|---|---|---|
| +0x10 | FC_TranslucencyScale | 508 |
| +0x90 | FC_EnvMapDolBlendRate | 0 |
| +0xA0 | FC_SnowMaskParam | 72 |
| +0xB0 | FC_FROZEN_THRESHOLD | 0 |
| +0xD0 | FC_FlowMapUvTiling | 0 |
| +0xE0 | FC_FlowMapIntencity | 10 (FlowDisplacement) |
| +0xF0 | FC_SoftVolume | 80 (the ZFeather families) |
| +0x100 | FC_PointCloudRate | 12 (`GXFlver_ColShZFeather`) |
| +0x1F0 | VC_VelocityOffset | 0 |
| +0x200, +0x210, +0x220 | FC_DamagedNormal* | 352, 352, 64 |
| +0x230 | FC_WetDiffuseColor | 1,020 |

No vertex programme (1,368 with the buffer) uses any of these; the domain programmes declare another layout (`g_DC_Water*`).

## 4. The native's own zero fills (static)

Where the native has no producer for a word a programme reads, it refuses the draw (`native_selected_constant_unavailable`): such
a word is never silently zero. The silent zeros are the places the native writes 0 on purpose, listed from its code
(2026-10-31):

| Buffer | Words | Native rule | Matches the original? |
|---|---|---|---|
| b2 cbMatDynParam | the 22E930 blocks with their flag clear (+0x20, +0x30, +0x40..+0x5F, +0x60, +0x70, +0xA0, +0xC0, +0xD0, +0xE0, +0xF0, +0x100, +0x180, +0x190) | 22E930 leaves them; the per-material buffer's creation zero | yes once every flag is set as 141070 sets it (§1, §2); +0x10 is zeroed even over a generator value (§1, latent) |
| b2 | +0x200..+0x22F, +0x230..+0x26F when damage or wet is disabled (`ProduceOrdinaryInactiveModelParameters`) | the creation zero | to compare live |
| b2 | +0x1A0..+0x1EF, the phantom block's defaults | explicit policy (P3) | witnessed in I-0067 |
| b4 cbInstanceData | +0x54..+0x5C, +0x60 without a light set, +0x68/+0x6C, +0x104..+0x10C, and aSh/ShLightMask +0x80..+0x10F for draw types other than 0, 3, 4 (`05_selected_scene.inc`) | 5B3D0 leaves them; the instance array's creation zero | **to compare live**: the original's instance records are a per-frame ring, so a word 5B3D0 leaves may hold an earlier draw's value there (the wheel's b4 in gap 7 is this question) |
| b7 cbModelParam | the parallax-corrected cubemap box words +0x80..+0x19F when both boxes are disabled (`13b_prepare_model_frame.inc`) | explicit open policy | the programme does not use them then |
| lighting, velocity, offscreen views | words no programme reads (`light_producers.cpp`, `velocity.h`, `16_offscreen_and_movies.inc`) | zero | not read |

## 5. The live census (2026-10-31 10:01-10:09, build `t7a`)

`Run-I113ZeroCensus.ps1 -Zones m40,m31,m37` (the Shebang zone saves, about 80 s a run, offline, restored by hash; the build with
GX36, GX37, GX39 and GX40 read): natively `native_zero_census` (zero-census.flag), in the original `reference_zero_census` (with
decal-observe.flag: the mesh loop 63CB0, the PS's DXBC checksum, the PS b2 and b3 bytes uploaded inside the loop, which every row used;
b7 is bound outside the loop, so none) and `reference_zero_instance` (light-observe.flag, 5B3D0's record). `Compare-I113ZeroCensus.py`
matches a native draw to the original's by (model, PS checksum, index count), the « full key », else by (model, PS) alone, the
« fallback », which may pair two meshes of a model and is not evidence for a word; each class is counted apart
(`evidence/I-0113/zero-constants/diff-m40.tsv`, `-m31.tsv`, `-m37.tsv`; the runs `evidence/I-0066/artifacts/i113-t7-census-*`):

| Zone | Native draws | Full key | Fallback | Unmatched |
|---|---|---|---|---|
| Firelink Shrine (m40) | 410 | 175 | 22 | 213 |
| Undead Settlement (m31) | 634 | 261 | 289 | 84 |
| Anor Londo (m37) | 479 | 312 | 100 | 67 |

- **On the 748 full-key draws, no PS b2 or b3 word the programme reads is zero natively and nonzero in the original** (OBSERVED; b2 is
  bound on most of them, b3 on nearly all). The tranche 6 class does not remain among those draws; it says nothing of the unmatched
  ones.
- The one full-key difference: b2 FC_SSS_ID on every player skin and face draw (18 a zone), native 1 and original 0 (the SSS profile
  index; the native registry's choice, 07_scene_lighting): to read.
- The fallback differences (b2 FC_BlendMaskParam on `MulMaskDOL` pieces of m31 and m37, FC_PomHeightScale and FC_Pom2HeightScale on
  m31 pieces, b3 FC_AlphaRef on one hair mesh) compare draws of different index counts: not evidence.
- Unmatched: 364 native draws, of models the original's census did not see in its run (mostly Firelink map pieces of engine pass 5)
  or that the original drew with another programme (the player's hair and face in other passes); a draw whose PS the original's
  CreatePixelShader hook had not recorded is dropped from its census uncounted (review L5).
- **b4 not compared**: the native row's b4 payload is not the instance record the draw reads (the instances are planned into the
  instance block table after this point, `PlannedNativeMeshBlocks`) and the programme indexes b4 dynamically, so every word reads as
  used; the original's record is 5B3D0's. The b4 rows of the diff are not evidence. Gap 7's b4 question stays open: a native capture
  of the planned instance rows is needed.
- b7 not compared (the original binds it outside the mesh loop). No draw of the materials whose items tranche 7 added (c1320, c1321,
  c3140, c3141, s09540, s09541) was in these runs: the new reads are not witnessed live.

Some original words are a buffer's earlier content (tranche 6: b2 +0x10..+0x3F of the POM wall, INFERRED), so a difference is read
against the programme's RDEF before it is called a defect.
