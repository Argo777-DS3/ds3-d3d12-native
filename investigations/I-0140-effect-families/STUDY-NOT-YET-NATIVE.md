# What is not yet native: an inventory for I-0113 tranche 10 (study thread, static, 2026-11-01)

Copied into I-0140 on 2026-11-13 (WORKFLOW « Canonical sources »); the original stays in I-0113. Several rows changed since (the raster
lighting removed, tranches 11 to 41 of I-0113): each row is re-read before use.

Thomas (2026-11-01 03:09-03:10): « que l'étude passe en revue tout le code et la décompilation pour voir ce qui n'est pas encore
branché sur notre natif », as a work queue for the gaps pilot while the R pilot works on the framerate. STATIC only: nothing
launched, no build. Framerate, unlock, timing, pacing, Reflex and DLSS-mode items are left out (the R thread's UNLOCK-READY §3).

**Method.** Four read-only sweeps (Sonnet agents): (A) `native/frame/game_bootstrap/*.inc`, (B) the rest of `native/`, (C) the
3,288 decompiled functions against `native/`, (D) the open items of the existing inventories (STATE, FINISH-LINE, I-0103 CENSUS,
MENUS-NATIVE-GAPS, WAVE1-LAST-EFFECTS, I-0113 RESULT and its studies, the I-01xx RESULTs). Their raw tables (about 300 rows with
file:line) are in the study thread's scratchpad, not in Git. The rows below are the ones that matter for tranche 10; **« read »**
marks a row whose source line the study thread re-read itself, the others come from the sweeps' reading of the code comments
(comment-based: a few comments are stale, so check the code before acting).

## 1. Visible gaps first: the work queue for tranche 10

Ordered by how likely they show in play and how many things one fix covers.

| # | System | What is not native | Where | Status | Visible consequence, link to known gaps |
|---|---|---|---|---|---|
| 1 | lighting/IBL | **Forward rows' IBL**: t30/t31 bound to a 1x1 black cube, « Forward pass explicit open policy (1114) » | `01_state.inc:342`, `08_model_programmes.inc:1119-1141` | read | No environment reflection on every forward `…Ibl` material: phantoms, fading characters, FFX model particles (the crystals' hue, gap 17), glass/water. Already STATE's next action. Pilot's note (tranche 10): the particles take the black common cube in the original too where the REM is null (RESULT); the open part is a world object's region cubes. |
| 2 | models/translucency | **The forward state rule**: the native uses a fixed policy (fading 0x29 rows forced alpha and depth written; phantoms 0x21/0x22 opaque) where the original chooses per draw (1A86C0's kind blend `[[GXRS+0x4DD0]+0x2F0 + 8 × state+0x84]`, else `GXFlverMaterialParamBlendMode` 140200840, then Begin's depth). **Forward materials with an MTD blend mode other than 0 or 2 are skipped** (`native_forward_blend_unsupported`, not drawn) | `13b_prepare_model_frame.inc:414-438` | read | Fading characters (gaps 11 and 19: Hidden Body's hair, Chameleon's flicker, Iudex Gundyr at his death) and any additive/multiply forward material missing. Tranche 9 already ported the rule for E03 particles; the same code path can serve ordinary rows. [STUDY-TRANSLUCENT-CHAMELEON](../I-0113-open-gaps/STUDY-TRANSLUCENT-CHAMELEON.md) |
| 3 | models/fade | **The fade category is chosen at admission** (0x21 opaque or 0x29 translucent from the group's `+0x84/+0x85/+0x87` and `+0xDC` against 1.0), not per draw | `09_model_admission.inc` (the category), RESULT gap 11 | sweep | The alternation of Chameleon (gap 11's lead: a value at the scalar's edge flips it). |
| 4 | effects | **Effect passes 0x20 and 0x21 are recorded and never drawn** (`++stats_.other_passes`, « captured, not drawn »); the E03 model particles of passes 0x2D, 0x20 and 0x31 are counted, not drawn | `renderer/effect_stage.cpp:173`, `scene_pass_frame.cpp:642` (`{0x29,0x2D,0x31}`), `12_scene_collect.inc:499-502` | read | Effects submitted in those passes are missing. Their count is already in the native's statistics (`other_passes`, `NoteNativeEffectCandidates`): one native run at a spell-heavy spot tells whether gaps 9, 14 or 12 are in them. |
| 5 | effects | **Effects drawn after every forward row and with read-only depth** (no depth-writing effect draw; 0x31 sees the copy without 0x2D's output) | `scene_pass_frame.cpp:605-608`, I-0103 CENSUS line 129 | read | Effect/translucent ordering and depth-writing effects differ (FINISH-LINE line 7, deferred from I-0104). |
| 6 | effects | **Renderer classes outside `allowed[]` are skipped**: none left (the light shafts 10003 native since I-0140 tranche 10; the spot effect lights 11000 a light, not a draw: the map's spot path, native by its mechanism, m45's two in the light list, identity INFERRED, I-0140 tranche 27) | `frame/effect_draw.cpp` (Execute's `allowed[]`), I-0103 CENSUS | read | None known absent (the spots' identity INFERRED). |
| 7 | effects | The open spell gaps: 9 lightning filaments black, 13/16 pyromancy explosions (a Normal 608 over a non-finite frame copy, INFERRED), 12 Flame Fan cut, 14 Lightning Arrow's bow; and the census's unknowns (miracles incomplete, sorceries, pyromancies, Dark spells, infusions, status effects uncertain) | I-0113 RESULT, [STUDY-GAPS-16-13](../I-0113-open-gaps/STUDY-GAPS-16-13.md), [STUDY-GAP-12](../I-0113-open-gaps/STUDY-GAP-12.md), FINISH-LINE | docs | As listed in STATE's next action. |
| 8 | effects | GPU particles read a zero environment SH (the scene root's device member null; only `cbCubeEnvSH1` shadowed during Execute); classic FFX draws with a geometry shader, a foreign buffer or a sampler outside the bank are dropped; the effect root has t0..t4 only, a missing texture is a null view | `effect_draw.cpp:424-437`, `21_effects.inc:646-650`, `effect_stage.cpp:100-111, 303-310` | sweep | GPU particles lit differently; a few particle draws missing or untextured (the native counts them: `other_samplers`, `foreign_buffers`, `missing_textures`). |
| 9 | water | **No water interaction map** (t57 = SYSTEX_GRAY « no interaction map »); t54 black until the reflection view exists; the reflection view skips decals and some characters | `08_model_programmes.inc:1031-1040`, `12_scene_collect.inc:1002, 1014` | sweep | No ripples or wake where a character walks in water; gap 18 (white patches at the Cemetery of Ash) is separate. The missing reflections were dropped as raster residuals. |
| 10 | lighting | **Projected-mask spot lights 0x69/0x70 are not drawn** (« no tile path »), shadowed 0x68/0x6F only when a face is in the atlas; line lights (type 107) not produced; shadowed lights through a virtual texture not produced | `07_scene_lighting.inc:94-97`, `world/light_producers.cpp:143, 206` | read (0x69/0x70, 107) | Gobo/cookie spots and tube lights missing where the maps use them (which maps: not counted). |
| 11 | lighting | Local-light visibility masks of 222FF0/FCE30 not reproduced; menu offscreen views take the main view's light, fog and IBL, no point lights or shadows (the comment may be stale since 1169) | `07_scene_lighting.inc:214-217`, `16_offscreen_and_movies.inc:331` | read (masks) | Extra lights the engine drops; menu portrait lighting (gap 20, the System menu portrait black in borderless play, and gap 3 are UI-side). |
| 12 | post/YEBIS | **The tone map is refused** (« the sRGB composition stays ») under auto exposure, a ToneType other than 2 (the value scale, the contrast, the field-of-view vignette and an exposure or gamma of 0 served since I-0113 tranche 20); **the grading LUT is refused** for a record not from the area's own map file (cutscenes, mapbase, default) | `13b_prepare_model_frame.inc` (the tone map's block, 1343-1373 at tranche 20; the grading after it), `12_scene_collect.inc:104-118` | read | Zones or cutscenes whose records use those words lose the native tone map or grading. Which areas: the native logs `native_map_override_table` (MapOverride_0001 and _0003 set AutoExposure) and `native_yebis_tonemap`; one run per zone family tells. |
| 13 | post/YEBIS | Glare only shape 1 (tinted shape refused); DoF only front blur (flags 2); shafts, motion blur and colour V only in the witnessed configurations; auto focus refused | `world/yebis_*.cpp`, `13b:1290-1433` | sweep | Only where a zone's record uses them. |
| 14 | decals | **Characters' blood/damage decal provider has no native owner**: t41/t42 the neutral common decal (« explicit open policy ») | `05_selected_scene.inc:285-286` | read | Blood or damage marks on characters may be missing (partly covered by the blood-provider patch through paint views: to check in play). |
| 15 | decals | GX00 Decal materials keep their non-Decal class (eyebrows, `M[ARSN]_e/_tr/_al` overlays); paint cube and height variants consumed without a draw; decal request families other than kinds 0 and 1 rejected | I-0103 CENSUS, `12_scene_collect.inc:741-755`, `03_engine_cpu_owners.inc:303-313` | read (families) | Map overlays and eyebrows wrong (FINISH-LINE line 7); some blood projections missing. |
| 16 | UI/characters | **Done in tranche 16** (RESULT « Tranche 16 »): the face image's mark layer drawn natively (`ComposeNativeFaceLocked`, `TextureIngress::AdmitComposedFace`). Open: its place at window sizes other than 1280x720 (the view size taken as the output size, INFERRED), and a mark whose texture is not resident, skipped where the original draws its colour alone (1423781F0) | `03_engine_cpu_owners.inc` (`ComposeNativeFaceLocked`) | read | Was: face marks missing in character creation and on the face texture they compose. |
| 17 | models | Admission refusals the original draws: `zero_fade` characters (m33 c1370, c1441, c2271 in a Fable review), m31 objects « never drawn natively »; categories other than 2/0x21/0x29, the fast and non-gated pose paths, a second LOD0 face set, unusual textures are refused | `09_model_admission.inc:176-215`, `content/model_pass.cpp:50`, `content/model_pose.h:39`, `game_scene_geometry.cpp:25-69` | sweep | Objects or characters missing. Whether these still happen is not stated in the comments: the native's refusal rows (`native_admission_*`, `popin_native_refused`) at the witness zones answer it. |
| 18 | models | Burn MTDs' parameter list has no native reader; SSS `g_SSSStrength_UseCAParam` not read; cloth: ChrAsm parts without an auxiliary palette, a non-neutral cloth cancel not posed; white fallback for default-bound texture names | I-0103 CENSUS, `07_scene_lighting.inc:360`, `13b:265-271`, `content/model_pose_binding.h:15-17`, `08_model_programmes.inc:1050-1062` | sweep | Burning objects, some skins, some capes, a white surface now and then (each witnessed case fixed one by one, as the Handmaid's chair). |
| 19 | effects | **Silent no-ops of the native effect context** (added 2026-11-07, [STUDY-SILENT-CALLS](../I-0113-open-gaps/STUDY-SILENT-CALLS.md)): the per-rectangle scene copies `260440` (607 distortion, 608 radial blur: gap 22, E's tranche 23) and `262A70` (the trails 606/10012 with a distortion property) read the GX context's bound-target cache, never written natively, and return; the skipped renderer classes and entity kinds are counted only with `ffx-record.flag` | `effect_draw.cpp:418-480`, STUDY-SILENT-CALLS §1 | read | Gap 22's jumps (OBSERVED, E); distortion trails refracting a stale copy (INFERRED); a skipped class invisible in an ordinary run. |

## 2. Behaviour risks (the game closes where the original continues)

| # | What | Where | Status | Note |
|---|---|---|---|---|
| R1 | **The native UI ends the process on any unsupported case** (`Fail` → `std::terminate`): offscreen composition, colour-clear commands, filter kinds 4 and 5, text layer kinds other than 0-7, 10, 11, instanced layouts; 486 of the 1,316 UI selectors have no programme | `frame/game_ui_session.cpp:103, 217-219, 299, 693`, `game_ui_mesh_owner.cpp:246`, `primitive_program_catalog.inc` | read (Fail) | A menu or HUD element never seen natively may close the game: FINISH-LINE line 8's crash tour of the unseen zones, plus the menus listed in MENUS-NATIVE-GAPS, would surface them. |
| R2 | Unowned GPU-side methods of the scene context, view and model storage are `Unsupported*` stubs that `Refuse` (exit) if the engine ever calls them | `03_engine_cpu_owners.inc:465-566`, `14_scene_step_and_storage.inc:191`, `18_resource_admission.inc:362, 404, 642` | read (465) | Same remedy: the crash tour; FINISH-LINE line 1's « skip and continue » rule could extend to them. |

## 3. Not native by design, or dropped (no work for tranche 10)

- By design: FMOD audio (native wraps and counts only), Bink decoding (the library's planes, native presentation), DirectInput,
  the engine's device-absent branches (`01_state.inc:165-172`: « ~150 sites », in fact 18 functions that test the member and about 140 that use it untested, STUDY-SILENT-CALLS §2), occlusion culling and GPU occlusion queries (cost only), effect
  culling (cost only), FXAA (DLAA/DLSS stand in), the R0 ray-tracing base (the RT/PT plan, not this list).
- Dropped by Thomas or a decision (FINISH-LINE « À abandonner », D-0235): tessellation (`native-tess.flag`, off by default: GX04/GX90
  stay flat), the Settlement wheels, the m30/m40/m31 lighting residuals, the water reflection's sun shadow and missing characters,
  the title's blue shadow, the glow witness, the pop-in leftovers.
- Stale comments found by the sweeps (the code has the feature): « no point/spot lights yet » (`renderer/model_frame.h:18`),
  « while the native has no shadows » (`content/model_pass.cpp:48`), the decal pass « until its encoder exists »
  (`01_state.inc:101-104`), the probe lighting omission (`17_render_step.inc:98-102`), the dynamic mesh buffer omission
  (`17_render_step.inc:178-186`). Worth a cleanup pass (FINISH-LINE line 2 bis), not a fix.

## 4. The decompilation

- 3,288 functions exported in `decompiled/`; **743 are named in `native/`** (about 325 of them only in comments), **2,545 are not**
  (1,532 named in a document, 1,013 nowhere). Not being named does not mean not native: the native reimplements whole systems
  without naming each function (lighting, YEBIS, menus), and most of the unnamed ones are studies of systems that run in the
  original by design (game logic, streaming, input, cutscene control) or one-off probes.
- The groups by system (unnamed in native): characters and skinning 183, cutscenes/REMO 176, menus and Scaleform 169, effects 142,
  materials and MTD 139, fonts 109, YEBIS 107, engine threads 105, phantoms and signs 83, fade plates and loading 79, display 77,
  culling and LOD 59, tone map and SSS 52, streaming 51, ending credits 47, input 43, water 31, shadows 30, timing 19 (the R thread's).
- The documents mark 23 of them « not transcribed », « not run natively » or « no native ». Still open after checking: the
  **single-model NPC phantoms** (`D07140` → `D3FE90` / `D40080`, PHANTOMS-NATIVE « Open »: an NPC phantom that is a single model is
  not covered; visible only for such NPCs) and the effect-attached FFX `2447C0`. Stale: the wind block (`C4D330`, `D3D280`,
  `D35F80`), native by default since FW-9 (`18_resource_admission.inc:507-516`); the effect light host (`2B2410`, `266FC0`), the
  effect lights being native (STATE).

## 5. Suggested order for tranche 10

1. Row 1 (forward IBL), already STATE's next action; then row 2 with row 3 (one transcription of the original's forward state
   rule for every forward model row, which closes gaps 11 and 19 if the study's lead holds).
2. One native run at the spell spots with the statistics rows already in the module (row 4's `other_passes`, row 8's counters,
   row 12's `native_map_override_table` / `native_yebis_tonemap`, row 17's refusal rows): it turns rows 4, 8, 12 and 17 from
   « possible » into a measured list before any code.
3. The spell gaps (row 7) in the order STATE gives; row 9 (water interaction) and row 16 (the face marks) are self-contained.
4. Rows R1 and R2 belong with FINISH-LINE line 8 (the crash tour of the unseen zones).
