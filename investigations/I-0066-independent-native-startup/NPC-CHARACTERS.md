# Non-player characters natively (workstream started 2026-10-10, branch `claude/npc-enemies`)

Specification: I-0067 [MENUS-MAP](../I-0067-static-menus-subsystems-map/MENUS-MAP.md) sections 54, 55, 57, 58.
Witness place: Argo (Thomas's save, slot 0), High Wall, the chained corpse `c1105_0011` in front of her
(`i66-reference-summon-1181/summon-1.jpg` against `i66-native-summon-1181/summon-1.jpg`).

## Native1183: where characters stop

Witnesses (`game_bootstrap.cpp`, bounded): `native_character_early` (every SprjModelDrawEntity the native
selection visits: model data name +2B8, pose bone count E+A80+70, E+C3C, category, bounds E+B80, reason) and
`native_character_candidate` (admission outcome in AdmitNativeModelCandidate, once per change). The Load Game pilot
now re-presses the row before each OK (`Pilot-LoadGameThomas.ps1`).

- `i66-native-npc5-1183`, `i66-native-npc6-1183`: the dynamic world layer receives 260 SprjModelDrawEntity inserts
  (`world_layer_insert_kind`, caller 885584), but the native selection (C1FB0 over the world's layers with the
  native visitor) visits only five, all characters: c1000 (2 bones; bounds then missing_inputs), two c1100 (118
  bones; refused by bounds; E+C3C 10 or 32714, bit 0 off), c2240 (93; bounds), c1290 (155; reaches admission, then
  zero_fade). No other SprjModelDrawEntity (no object, not the corpse) is ever visited. Their bounds change over
  time (live).
- Open (I-0067 question sent): how the dynamic layer chooses the entities it hands a visitor, and which update the
  native frame skips.
- I-0067 answers (MENUS-MAP 62): c1105 has its own FLVER (120 bones); E+C3C bit 0 is "drawn" (D074A0 from C4AF10,
  game-side masking: a refusal there is correct); bounds E+B80 and the culler registration (14002D800 -> [E+18]
  vt+30) are written by D08190 at the head of D07710 when BA8 bit 2 (set by D076A0); zero_fade comes from the LOD
  controller E+F0 (D1CCE0, FD4 steps 0x4F/0x50).
- `native_character_survey` (`i66-native-npc7-1183`): 16 character draw entities inserted natively at the High Wall
  (c1000, c1090, six c1100, three c1280, c1282, c1290, c2240, c3250, c5270), all with live bounds; BA8 = 3.
- `draw_entity_population` (both modes, 60 and 100 s after the first insert; `i66-reference-population-1183`,
  `i66-native-population-1183`): the same 16 entities with the same drawn/hidden bits in both modes; no entity has a
  c1105 model data name in either mode. So the population is identical; natively only some of the visible ones are
  ever handed to the selection visitor (two c1100 out of six). Open: the dynamic layer's traversal (question sent).
- I-0067 (MENUS-MAP 62): the "corpse" in front of Argo is an object (nearest: o001050_3208 at 1.9 m, o001090_3019,
  the torch bowl o304863_8030, o001080_3547); c1105_0011 is not instantiated in this save. Proposed cause, the
  visitor's category byte (native 0x01, original 0x02 G-buffer / 0x04 forward): measured not the cause here (every
  character's E+20 carries bit 0; widening the native mask to 0x07, kept, changed 323 -> 324 selected objects,
  `i66-native-mask-1183`, `i66-native-e20-1183`).
- Objects (`native_character_candidate` extended to o-models, `native_object_early` for SprjModelDispEntity holding
  o-models; `i66-native-objects-1183`, `i66-native-objects2-1183`): admitted objects exist (o001080, o001090,
  o004660, o3025xx ...), many are refused by zero_fade, and many disp entities stop at the "live" check
  (D03050 -> 64F000(entity+39C) -> 619800) or at bounds; o001050 (7 instances) and the bowl o304863 stop at bounds.
  Open: where Argo's camera is (to tell a correct cull from a wrong one), and the "live" words natively.
- Decal receiver bit (I-0068 DECALS-NATIVE step 1): the native G-buffer stencil reference is 9 for every mesh, the
  original 8/12 on characters (bit 0 = decal receiver, kept only for map/object parts without disable_decal). No native
  pass reads bit 0 yet (no decal pass; the env-map regions test bit 3), so the fix belongs with the decal pass, where
  its rule (args +18C bit 2, kinds 3-4, disable_decal) can be checked against D1.
- Camera (`native_cull_frame`, `i66-native-cull-1183`): the native scene camera, the LOD manager camera and the
  culler planes agree: Argo's camera is near (30.7, -24.1, 54.9). o001050_3208 (-34.5, -0.15, 82.6) is about 70 m
  away, so its bounds refusal is a correct cull: it is not the object in front of Argo.
- Display groups (`native_object_early` with DrawGroups E+39C, the owner's DispGroups [E+70]+8.., the global
  StageRend+514; `i66-native-groups-1183`): within 10 m of the camera, o302590 (seven instances) and o004660 stop at
  the "live" test because their DispGroups owner holds zeros (DrawGroups word 2 = 0x200); two other instances whose
  owner holds 0BA00000 / 103D7D00 / 84302FCA ... pass (walked, admitted). So natively some DispGroups owners are never
  written. Open: which blocks 61AA60 writes natively (I-0067 MENUS-MAP 63 chain: player hit P+1AC8, FieldArea copy,
  606BC0, 621730 -> 61AA60).
- Connect condition (`draw_entity_population` with X = [14489C7A8]; `i66-reference-connect-1183`,
  `i66-native-connect-1183`): X+0x44 = 2 and X+0x5A = 0 in both modes, so 607570's connect is true in both; the
  zero DispGroups owners are not explained by connect. X is built by EBF410 (+0x44 = 2) and rewritten each frame by
  its FD4 task EBF820 (X+0x6C forced value, else [X+8] vt+0x20, provider EC0640); +0x5A by EBFB00 (8F0600 0,
  628F10/ADBE40 1, BCF100 0) (MENUS-MAP 63).

## Handoff checkpoint (2026-10-10, to the thread "Chantier PNJ et ennemis")

Next controls, same frame, both modes (MENUS-MAP 63): (ii) hit = FA+0x60 (or +0x68) and hit+0x5B; (iii) the block b
with [[FA+8]+0x12C0+0x438*b+0x3E8] == [E+0x70] of a zero-group object (o302590 near (30.7, -24.1, 54.9)), b against
hit+0x5B, and the map id of [FA+0x10]+0x20+0x70*b; (iv) the connect-collision list hit+0xE0: [C+0xB8]+4..+7 and
C+0x6C..+0x8B, a C matching b with bit 0x200 in word 2; (v) with connect true and such a C, [block_b+0x3E8]+8 must be
non-zero, else re-read the 606BC0 call. Player P = 8D9D70([14477FDB8]) ([[14477FEC8]+0x150] or [14477FDB8]+0x80),
current hit P+0x1AC8 (setter 88F870), FieldArea copy 8FAE00 (needs [14477FDB8]+0x90 == 0).
Also queued for this workstream: the in-game character parts' tint (MENUS-MAP 60: native world parts of
SprjAsmModelDrawEntity read the shared group E+0x280, never written by the original, so FC_SkinMatchColor is 0;
state+0x280 = E+0xBD0+0x10*[desc_i+0x48], plus CFDE70's other overrides +0xD0..+0xDC, +0xC0, +0x84/+0x87, +0x98;
fixes the pale brows) and double-sided hair (HR_A_0106 face sets CullBackfaces=False; native always CULL_BACK).

## Native1184: DispGroups witnessed, character and skinned-object draws admitted

Witnesses (`game_bootstrap.cpp`, both modes): `field_groups_compute` (hook 606BC0: FieldArea, hit, connect, hit block
and groups, block map records, the connect list) and `field_block_groups` (hook 61AA60: groups written per block,
owner, block index); `native_object_early` now carries the owner address and its block index.

- `i66-reference-fieldgroups-1184`, `i66-native-fieldgroups-1184`: identical chain in both modes. The first calls have
  hit = 0 (no ground contact yet: groups zero); then hit block 1 with groups 0BA00000/103D7D00/84302FCA/CF00081A/
  00000001/0/0/04010000, written to block 1 (block 2: word 5 04000000, word 7 80000000). Every nearby object's owner
  is block 1's source. The 1183 "live" refusals were these pre-contact frames (the witness logs once per change):
  after the write, o302590 and the others are walked. DispGroups are not a native defect.
- The object missing in front of Argo is the chained corpse with the lantern (o302590 skinned, character materials
  C[ARSN]_e, families GXFlver_ColDifSpcBumpIbl_Bloody[_Cloth]). Outcome tally (pseudo parts 1000 = SprjModelDrawEntity,
  1001 = skinned disp items, in `native_asm_part_outcomes`): every refused draw failed on textures, t41/t42
  `g_DecalDiffuseTexture` unbound. Cause: the neutral decal (absent decal for t41/t42) was given only to the player's
  ASM parts and Common-decal models; characters whose decal provider is a GXBloodyMaterialPatch (3D41DF0) and map
  object items without provider (choice None) had nothing. Fixed by family: both now take the neutral decal.
  `i66-native-decal-1184`: characters (1000) 11/11 drawn; `i66-native-corpse-1184`: the chained corpse drawn natively
  (`ingame-1.jpg` against `i66-reference-fieldgroups-1184/ingame-1.jpg`).
- Palette padding: the skinned-VS palette was padded to 256 rows by a fixed size, and a pose with more rows (o302590:
  282 bones) overran the padded copy. The padding is now max(256, pose rows).
- World ASM part tint (MENUS-MAP 60): state+0x280 = E+0xBD0+0x10*[descriptor+0x48] (CFDE70's stack-group override).
- Face-set culling (MENUS-MAP 64): MeshSlice carries the selected face set's class (+0x0C). Measured: with 2 = FRONT
  the floors and Argo's armour rendered inside out (`i66-native-chrview-1184`), so 0 = none, 1 = front, 2 = back;
  drawn classes 20 % class 0 (two-sided), 80 % class 2, none of class 1 (`native_cull_classes`).
- The NPC fade and ExParams (CHR-FADE-EXPARAM-REFLECTION) are already wired for SprjModelDrawEntity: the default fade
  layout {0x160, 0x200, 0x170, 0x240} is its LOD controller and group, and parameter_animations = E+0xA40.
- Still open: no enemy in view in both modes yet at the High Wall (the camera turn integrates differently at the
  native frame rate); dynamic cloth (MENUS-MAP 64-65: cloth meshes draw their bind pose × VC_World).

## Native1185: native cloth set and upload; the material cloth flag is never produced natively

Coded (MENUS-MAP 65): `NativeDynamicMeshBufferSet` transcribes 23B7960 (admission: material +0x22, hcl name 23C5070,
POSITION buffer 23B6C50, authored bytes) with the engine's own CPU-only GX buffer object (192B300 misc 0x4000 +
1932410, refcount and destructor kept), real CPU copies from set[0]; `NativeClothVertexUpload` replaces 23D5BC0
steps 3-6 (after the 34F1B0/34F090 duplicate replication) by an immutable native upload keyed by the buffer object;
`NativeClothSetRelease` forgets the versions before 23B7280; SprjModelDrawEntity draws link E+0xA70 -> [+0x38] ->
[+0x38] = set and bind the current copy as stream 0; VC_isEnableCloth = 1 only with a bound copy (else skinned).

- `i66-native-cloth2-1185`: 17 sets created (player and NPC models, 1 to 17 meshes), every mesh rejected on material
  +0x22 = 0; `i66-native-cloth3-1185`: no drawn material has +0x22 set either (SAT_Equip_cloth, SAT_HairIbl2_cloth,
  _Bloody_Cloth included). The flag is written by the original material builder 82220 (140F90 / material vt+0x58 /
  141F80 after 82CC0), which the native BuildModelMaterial replaces with 82CC0 + 1B0AB0 + ordinal only. Writer and
  rule requested from the static thread.
- Cloth closed (Native1185, MENUS-MAP 65-71): material +0x22 is produced in BuildModelMaterial from the MTD
  (g_GX_MaterialType 2 or GXFT_Cloth; +0x24 gets 0x2000 for cloth and 0x800 for SSS, g_GX_MaterialType 3), so 23B7960
  admits the cloth meshes (Argo: 7 of 35, NPC models 1-3); the engine's Havok steps write the CPU copies (measured: the
  copy content changes, `simulated` in `native_cloth_vertex_upload`); 23D5BC0 publishes them natively each simulated
  frame; the link instance -> CI -> set works for all three owners (SprjModelDrawEntity E+0xA70, ChrAsm descriptor
  +0x18, SprjFD4ModelItem +0x750; `native_cloth_set_link` valid). The simulated positions are in the cloth instance
  frame (I+0x50 / +0x90, 4x4 rows, translation last; measured rotated 90 degrees from the part placement), so cloth
  draws take VC_World from it (`i66-native-cloth9-1185`); a copy binds only once the simulation rewrote it.
  64340's cloth route asks for the auxiliary palette, which native ChrAsm poses do not own: explicit open policy, the
  current route (Native1185 parts 2-3 had been refused on palettes). Result: Argo's cape follows the simulation
  (`i66-native-cloth10-1185/ingame-1.jpg`); non-regression `i66-native-final-1185` (every part drawn, 0 faults).
- Investigation witnesses removed at the close (DispGroups hooks, early selection witnesses, population survey, cull
  frame, cloth placement); kept: the character candidate outcome, the pseudo parts 1000/1001 tally, the cloth set link
  and upload rows (bounded).
- Open for later workstreams: enemy witnesses W1-W7 of MENUS-MAP 69 (needs walking and combat), the auxiliary cloth
  palette, the other 140F90 words (+0x18, +0x1C, +0x21, the spx-name bits of +0x24, +0x80), per-material SSS profiles
  for enemies (MENUS-MAP 70-71), the special enemy families (Veil, Ghost, tessellation _PN/_FT, Fur, Eye, Recursive
  crystal, MENUS-MAP 68/71/72), Phantom_Fwd programme choice for forward draws (I-0068 PHANTOMS-SIGNS-FOGWALLS-NATIVE,
  check P4), ShortBoneIndices (seven characters, palettes up to 1770 rows).
- Pre-merge review (fresh agent, 2026-10-11): no blocker; applied: late cloth uploads after close return without
  refusal, cloth stream bound only when its byte size equals the slice's stream 0 exactly, part colour index bounded to
  c < 8, content.h cull-class comment corrected, the character witness map capped. Kept as notes: per-frame witness cost
  of the 1000/1001 tallies, one upload resource per cloth mesh per simulated frame (a reusable ring later), palette-index
  validation skipped for cloth-bound meshes, +0x24 initial value assumed zero by 82CC0. Control run
  `i66-native-review-1185`: every part drawn, cape simulated, 0 faults.
