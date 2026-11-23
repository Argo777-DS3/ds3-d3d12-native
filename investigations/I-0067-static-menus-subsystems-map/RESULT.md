# Investigation result

Complete (first tranche, 10/12 cycles). Static only; no game launch by this workstream.

## Resume checkpoint

| Item | Position |
|---|---|
| Branch | `claude/i0067-static-menus-map` (worktree `.claude/worktrees/modest-austin-69a651`), from main `4bc523d5` |
| Done | [MENUS-MAP](MENUS-MAP.md) sections 1–75 (menus, Load Game portraits and their scene, sound, transitions, capture and movies, effects and effect lights, phantoms, character views: list, attachment, camera, shadow, layer masks, the one-frame Status snapshot, part textures, the FaceGen readiness gate menu part material inputs and the Status view background, lights and gparam values, menu sounds, the offscreen view teardown order, phantom parameters the Load Game-only render work the effect-light filters and the census of every menu offscreen view with native witnesses, character creation flow, the Scaleform 2D layer: D3D11 HAL, states, img://SYSTEX path, fades and direct D3D11 calls, old dialogs, HAL method names, every D3D11 path outside GX, title → creation portrait release and the deferred-deletion task, native Scaleform HAL specification, Bink video player, owners and native specs of the remaining direct D3D11 sites, gparam load path for menu views, creation opening and system-step shutdown order, phantom feature bit and families, face-grid entries, phantom pixel programmes per pass, creation menu labels and NPC phantom rows, in-game map gparam files, keys, crossfades and light orientation, hair and eyebrow materials, programmes, instance data and the face-texture override, gparam angle units and layer stack, SAT hair alpha and alpha reference, scene-camera object defaults, phantom lookup fallback, phantom constants b2 rows 26..30, colours per role and activation, Player-view shadow from the ShadowParam angle, face grid opening and its fifteen views, in-game menu pages, name dispatcher, icon sheets and the profile confirmation, menu input devices, the shared input gate and the hit-test unit, icon sheets end to end and their loading at UI start-up, High Wall corpse and torch by family, native specification of non-player characters and menu hair placeholders, the ChrAsm slot build gate, dynamic cloth, _Bloody_Cloth and the emissive family, the cloth uploader, the last readers of S, High Wall NPC summon signs, eyebrow colour, pass and blend, the in-game HUD, character and object selection on the High Wall: draw enable, bounds, LOD fade, the dynamic tree layer and draw groups, the current DispGroups chain from the player ground collision, face-set culling, the FD4 frame steps and character cloth end to end, the cloth dynamic-buffer object contract and the exact cloth tasks of steps 0x28–0x3A, the material cloth flag e+0x22, reaching the cloth instance and set from each owner, and the skinning and multi-model outliers of the 165 character files, the original GXFlverMaterial vtable and its GX item parser, the GX item helpers and the device, and the enemy material families: ghost, SSS, veil, emissive, translucent, tessellated, the enemies of the witness zones and the smallest family witness set, the ExParam block mat+0x210..+0x24F and the enemy SSS, the SSS stencil bit rule and the Fur and Eye families, tessellation variant selection from GX04, veil, ghost, translucent, additive and flow families, the cloth simulation window 0x2F-0x37, the tessellation constant producers and the previous-frame palette, the _Phantom_ pixel programme check for a normal player, corrected: the phantom set is the alternate mat+0x78 chosen by DC+0x425); [VELOCITY-SKINNING](VELOCITY-SKINNING.md) (velocity, model side: palette ring, keepPrevious owners, b5/b6 binder, rigid objects, what writes no velocity); [NATIVE-READS-AUDIT](NATIVE-READS-AUDIT.md) (native reads of engine structures: width, type, unit); [TONEMAP-SKIN](TONEMAP-SKIN.md) sections 1–18 (tone map, world tone-curve selection and constants, glare/bloom and GX post filters, skin SSS, per-instance SH, env map region cubes, name-to-texture repositories, the SSS of enemies and NPCs) |
| Waiting on the I-0066 pilot | nothing; `0xD658B0` and `0xD57EB0` were read from Native1160 loaded bytes (both are debug GUI registrations, MENUS-MAP section 12) |
| Next static questions | which draw selects the `_Phantom_Fwd` programmes (MENUS-MAP section 13, open); further passes relayed by the coordinator |
| Tools | `Invoke-Map.ps1` + `I0067Map.java` (read-only queries, loaded-byte overlay `L:`, instruction-text scan `g:start:end:regex`), `I0067Rtti.java` (RTTI inventory in about 10 s), `Get-DxbcReflection.ps1` / `Get-DxbcDisassembly.ps1` (embedded shaders), `Get-ParamRows.ps1` (regulation param rows), `Get-GparamValues.ps1` (GPARAM values with ids) |

## Conclusion

1. **Load Game fault (Native1116, `14057F8A3`, `rcx = 0x4E0`)**: the appearance copy into a
   null `CSMenuProfModelRend` save-slot portrait from `14478D680[10]`. The ten 128x128
   portraits are created by the `AppMenu::TitleTopDialog` constructor and released by its
   destructor; the MenuSaveDataSummary job reads them without a null check. They belong to
   the menu offscreen model-render family (Prof, Player, Face), each drawing a character
   into its own offscreen GX texture, which the native backend must provide.
2. **In-game menu (Native1113)**: NewMenuSystem opens `MenuMan.IngameTop` only when MenuMan
   state[8] is 7. The pilot measured state[8] = 0 natively with NewMenuSystem present and no
   root window. The only writer of (8, 7) is the in-game menu input mapper
   `FUN_1408FB6D0`, after the encrypted button test `FUN_1407FDB10(ctx)`. The two
   decrypted gates (`FUN_14075EB60`, `FUN_14080EF40`) pass natively. Remaining owners: the
   button test returning false (input route), the blocked branch, or the mapper not running;
   `[MenuMan+0x2E4]` and the button-test bytes discriminate.
3. **Menu sound**: FMOD (`DLMO::MOFmodSoundManager`); a MenuMan-owned sound object
   `[144763868]` with 57 SE channels and a BGM handle; categories `f` (menu SE), `m`
   (music), `v`. No renderer dependency.
4. **Transitions**: seven named fade plates owned by `RendManImp`, the fake loading screen
   of the draw step, screen capture under `SprjGraphicsImp`, Bink movies into
   `CSMovieGxTexture`, and Scaleform loading/fade/movie screens.
5. **Effects**: SprjSfxImp builds the GXFFX core (manager, scene control, resource manager)
   and the shader/texture/FLVER/sound containers; per-frame `GXFfxDrawContext` and
   `GXFfxDrawPass`; appearance families map to the I-0063 effect families.
6. **Phantoms**: gesture and replay ghosts copy `g_GlowScale`, `g_GhostEdgeColor`,
   `g_GhostEdgeParam`, `g_BlendMode` from the `P_Ghost_FDP_Gesture`/`P_Ghost_FDP_Replay`
   materials into the character; with the forward-pass `P_Ghost_FDP` programmes this is the
   phantom rendering mechanism. The summoned-player phantom source is still open.

## Evidence

- [MENUS-MAP](MENUS-MAP.md) (all addresses, call chains and listings).
- Exported functions in `decompiled/` (`DS3-1152-F-<RVA>`), including the forced listing
  `00B3E1D0.listing.txt` and the decrypted `0075EB60`/`0080EF40` `.loaded1120` files.
- RTTI tables `evidence/I-0067/rtti/classes.tsv` (`5AC0988E…`) and `vslots.tsv`
  (`C8B00F09…`), regenerable.
- Pilot measurements and loaded bytes: `evidence/I-0066/artifacts/menu-gates-loaded-1120/`
  (owned by I-0066).

## Uncertainty / unresolved questions

- Which static owner nulls the portrait slot natively (destroyed dialog, dialog never built)
  remains a runtime question.
- The menu button test is unread until its loaded bytes are acquired.
- Role inferences marked INFERRED in MENUS-MAP (player model render = Status view, fade
  plate drawing, appearance-to-family mapping) are not proved.
- Several functions are encrypted at rest or reached only through protection stubs; their
  callers are missing from the Ghidra database (the `e:` raw scan finds them).

## Artifacts produced or modified

- `I0067Map.java`, `I0067Rtti.java`, `Invoke-Map.ps1`, `MENUS-MAP.md`, `decompiled/` exports.

## Suggested next step

When the pilot supplies the button-test bytes, decompile them with `L:` and close
Native1113's owner; then the summoned-phantom parameter source.

## Pre-merge review

Fresh Fable 5.1 review of `main...claude/i0067-static-menus-map` (2026-10-10): scope clean
(notes, read-only scripts and `decompiled/` exports only; nothing under `native/` or I-0066),
scripts read-only, 14 STATIC claims confirmed against the exports. Three fixes applied in
`e462472e`: the FFX resource-manager creator marked INFERRED; raw query outputs preserved
as `evidence/I-0067/queries/` and the evidence manifest filled with hashes (external backup
`D:/DS3_evidence_backups/I-0067`); missing exports added for the menu sound and MenuMan
setter claims. Notes applied: gate value `DAT_14477D5C0`, setter bound `index < 500`,
`+0x142` owner marked INFERRED, query prefixes documented, `decompiled/README` suffixes.

## Tranche 2 (2026-10-10): tone map and skin lighting

[TONEMAP-SKIN](TONEMAP-SKIN.md). The tone map is the YEBIS 2 middleware chain fed each
frame by the ToneMap and glare graphics-parameter banks, interpolated between two entries
by `FUN_1400FB890` and copied into YEBIS globals (`FUN_1400E42A0`, `FUN_1400E4460`); with
auto exposure, YEBIS computes the exposure on the GPU and `FUN_1400E69A0` reads it back each
frame. Bare skin (`GXFlver_SkinSSSIbl_Bloody`, face `FgFaceSSSIbl`) depends on a
screen-space SSS stage (`FUN_1401141B0`, programmes `GXSssBlurX/Y` and `GXSss`) that the
native renderer does not run (INFERRED main cause of the darker hands; secondary
candidates: `FC_SkinMatchColor`, character SSS strength, missing tone map).

## Tranche 3 (2026-10-10): executable SSS stage and portrait production

The SSS stage (objects, per-frame constants, three sorted commands, two draws with their states) is in [TONEMAP-SKIN section 3](TONEMAP-SKIN.md#3-the-sss-stage-executable-detail-static); how the original game produces the Load Game portraits (private GX scene per portrait, `ChrAsmModelRes`, `MenuOffscrRendParam`, RendManImp offscreen list, nine-state step machine) is in [MENUS-MAP section 7](MENUS-MAP.md#7-how-the-original-game-produces-the-save-slot-portraits-static). Open: the encrypted portrait render callback `FUN_140D658B0` (RVA 0xD658B0).

## Tranche 4 (2026-10-10): per-instance SH

[TONEMAP-SKIN section 10](TONEMAP-SKIN.md#10-per-instance-sh-ash-cbinstancedata-0x800xaf-where-it-comes-from-static).
The per-instance SH is pure CPU work:

- **Probe path:** a nearest baked map probe (`FUN_14001D650`/`FUN_1400F47D0` over the volumes
  of `G+0x46E8`, built from the map's probe files, INFERRED `.btpb`), copied by
  `FUN_140061950`.
- **Fallback:** the hemisphere ambient rows `+0x290..+0x2BF` of the gparam light-set record,
  written by `FUN_1401495F0`.
- **Gate:** both paths are gated by `[GXRS+0x4DD0] != 0`.

Section 9 covers `FC_SkinMatchColor`, which is debug only, and the MTD parameter registry.

## Tranche 5 (2026-10-10): GXEnvMapRegion cube source

[TONEMAP-SKIN section 11](TONEMAP-SKIN.md#11-gxenvmapregion-where-each-regions-ibl-cube-comes-from-static).

- Each region cube comes from the gparam EnvMap bank `[GXRS+0x42A8]`. `FUN_14006CED0` resolves
  it by name: `swprintf("%s%04d", prefix, id)` (for example `EnvMap_0304`), looked up in the
  repository `[bank+0x250]`.
- The fallbacks are the global REM cube `[[GXRS+0x4DD0]+0x178]`, then a GX common texture.
- The repository is `SprjGxResourceRepository_forEnvmap`, created by the `SprjGraphicsImp`
  constructor; the code that stores it into `bank+0x250` is unknown (not `FUN_140D57EB0`).

## Tranche 6 (2026-10-11): phantom writer, graphics settings, Scaleform page inventory

- [MENUS-MAP 76](MENUS-MAP.md): the writer of `state+0x87` (`DC+0x425`) is the PhantomParam id of the model or part
  (`holder+0x97`, set by `FUN_140D07140` for single models and by the part descriptor `+0x44` for assembled models, fed
  by `ChrIns` slot 36 -> `FUN_1409CAF30` -> slot 214 and by the weapon-enchant slot). The host player has none, so
  native `<family>_Fwd` is right for it; summoned phantoms, NPCs with a PhantomParam id, replay ghosts and enchanted
  weapons need the whole alternate shader set. Corrections propagated to I-0068 PHANTOMS-SIGNS-FOGWALLS-NATIVE,
  RESULT, STATE.
- [MENUS-MAP 77](MENUS-MAP.md): `GraphicsConfig.xml` keys and tokens (`DISABLE`/`LOW`/`MEDIUM`/`HIGH`/`MAX`;
  `ENABLE` is not a token), the twelve quality slots, the 0x24C-byte quality table decoded per level
  (`Get-QualityTable.ps1`, `Show-QualityTable.ps1`, `quality-table.tsv`), every reader of the quality object, and the
  fact that the native root builds none of it (no graphics option reaches the native renderer).
- [MENUS-MAP 78](MENUS-MAP.md): Scaleform feature inventory of the 102 menu movies (`Get-GfxFeatures.ps1`,
  `menu-gfx-features.tsv`): blend modes, masks, fills and colour transforms are covered by the native HAL; **filters
  (DropShadow, Glow, Blur, with their parameter sets) and the offscreen cache are the gap**; fonts and languages per
  ordinal.
- [MENUS-NATIVE-GAPS](MENUS-NATIVE-GAPS.md): the menus workstream reconciled with the repository (the three ROADMAP
  items are met natively) and the remaining gaps by capability.
- Corrections applied at the coordinator's request (I-0068 NPC-FAMILIES-STATIC): the translucent `TrIbl` family is
  deferred (class 1/4, MTD blend 0, depth written), not forward (MENUS-MAP 68, 69, 72.4).
- Tools added: `I0067Stores.java`, `I0067VCalls.java`, `I0067FieldUsers.java`, `I0067FuncStrings.java`.

## Tranche 7 (2026-10-11): implementation checklists for filters, quality object, input latch, YEBIS motion blur, phantoms and signs, fonts and languages

- [SCALEFORM-FILTERS-NATIVE](SCALEFORM-FILTERS-NATIVE.md): the Scaleform filter planner and the cache offscreen as an ordered checklist (F1-F10, live rows L1-L6).
- [QUALITY-OBJECT-NATIVE](QUALITY-OBJECT-NATIVE.md): native construction of `[GXRS+0x5360]` from the live level block (Q1-Q8, oracle `Build-QualityObject.ps1`).
- [NATIVE-INPUT-LATCH](NATIVE-INPUT-LATCH.md): why the native Load Game confirmation sometimes fails (one sample per update against 0.67-0.84 s updates) and the latch correction (N1-N8, NI1-NI7).
- [YEBIS-MOTIONBLUR-NATIVE](YEBIS-MOTIONBLUR-NATIVE.md): steps 7-8 of the velocity workstream: the parameter group and its words, the driver calls the native must repeat from `1400E69A0`,
  the init flags (`0xF8EE0000`), the gates, the blur-length planner (n = 5, R = 1, 18 px at default 720p), the YEBIS camera-velocity draw (`MakeCameraVelocityOverwrite`, exact shader
  arithmetic and constants), the blur draw (`DirDepthInnerscreen_SM3_T2_S<n>`), the GX dilation (2 passes, near-identity), targets and formats, frame position, checklist Y1-Y16 and live rows ML1-ML12.
- [PHANTOMS-SIGNS-NATIVE](PHANTOMS-SIGNS-NATIVE.md): phantoms and ground signs as a checklist (P1-P18, PL1-PL10): the PhantomParam/RoleParam decode with the final `cbMatDynParam` rows for all 102 rows
  (`Get-PhantomTable.ps1`), the proof that the alternate shader set differs from the normal set only in the `Fwd`/`FwdSdw` pixel slots, the routing table (list 0x21/0x29/2 by `state+0x84/85/87/DC`),
  the DXBC shape of seven `_Phantom_Fwd` families, the sign id chain (type -> category -> RoleParam `+0x30`) and the structure and colours of the sign effect files (30001..30052).
- [FONTS-LANGUAGES-NATIVE](FONTS-LANGUAGES-NATIVE.md): fonts and languages of the Scaleform interface (F1-F9, FL1-FL8, SL1-SL3): the exact chain Steam language name -> id -> ordinal -> filter (13 of 21 ordinals reachable, the rest falls back to engUS), the per-ordinal folder, font, plus-string and digit tables, the files requested per language (msg, 80_language.tpf with 14 banners of 2048 x 384, two font.gfx loaded as MenuFont_01/02), the glyph cache defaults (1 page, 1024 x 1024, mode 1) and the glyph working sets per language, the saved-profile override (-1 = platform), and a bounded static inventory of subtitle and dialogue text (FMG categories, lookup order, display pages).
- [SUBTITLES-DIALOGUE-NATIVE](SUBTITLES-DIALOGUE-NATIVE.md): how NPC dialogue and subtitles reach the screen: FMG categories 1 and 3, the two-slot queue at `[144763258]+0x1654`, the five producers (cutscene subtitle action, bik movie subtitles, talk-message commands), the Scaleform consumer `CaptionMenuWindow` (page `01_060_Caption`, a 256-frame clip faded by frame number, font `MenuFont_01`), the settings byte that gates it, the legacy FD4 dialog, native checklist G1-G6 and live rows CL1-CL5.
- [FD4-TEXT-GX-NATIVE](FD4-TEXT-GX-NATIVE.md): FD4/Ez dialog text on the GX 2D layer: the record -> replay -> GX command -> submit chain (`FUN_1407F84A0`, `FUN_1407F9B90`, `FUN_14237C7E0`, `FUN_1403A4520`), the debug-font resources (`DbgFont14h`: 17 BC1 pages of 512 x 512, 15,165 glyphs covering Latin, Cyrillic, kana, CJK and Hangul), the two debug-font shaders and their constant block and 24-byte vertex, the layout rules of `FUN_141923340`, and the fact that the pilot defers this pipeline (`native_optional_visual_deferred debug_font`): checklist H1-H8, live rows DT1-DT5.
