# Remo engine cutscenes as a native checklist (I-0069, Task 11)

STATIC (Ghidra mirror of `DarkSoulsIII.exe` 1.15.2.0, the offline game data and `native/frame/game_bootstrap.cpp`). Nothing was launched.
Status words: **STATIC** read in code or data, **INFERRED** deduced, **OPEN** not known statically.
Baseline notes: [BINK-MOVIES-NATIVE.md](BINK-MOVIES-NATIVE.md) (the only video files are two Bink movies; boss cinematics are not video), [SUBTITLES-DIALOGUE-NATIVE.md](../I-0067-static-menus-subsystems-map/SUBTITLES-DIALOGUE-NATIVE.md) (caption queue),
[MENUS-MAP.md](../I-0067-static-menus-subsystems-map/MENUS-MAP.md) section 6 (fade plates) and 27 (gparam areas), [EFFECT-LIGHTS.md](../I-0068-static-yebis-ffx-lights-spec/EFFECT-LIGHTS.md) (effect-hide mask).
Evidence outside Git: `evidence/I-0069/queries/remo/` (EMEVD cutscene calls, remobnd census, MQB strings of the two witnesses, class slot tables, string, cross-reference and singleton-reader searches).

## 0. Verdict

1. **A "Remo" cutscene is a data-driven timeline played by engine code, not a video.** The data are `remo/sAA_BB_VVVV.remobnd.dcx` bundles (a BND4 holding one `.mqb` timeline plus `.hkx` animations per cut). 72 bundles were opened (68 in `Data1`, 4 in `DLC1`; `s45_00_0090` is in the index but not in the census, reason not recorded); the
   `DLC2` bundles (areas 50 and 51, path `remo_dlc2:/`) have no dictionary names and were not opened. The engine classes are `SprjRemoImp` (singleton), `SprjRemoMan`, `SprjRemoRes`, `SprjRemoIns`, `SprjRemoStep` and 17 "act" classes (section 3).
2. **The cutscene CPU logic is genuine game code that the native build does not replace.** No hook of `native/frame/game_bootstrap.cpp` replaces a Remo function. Three places in the native source read Remo state: the comment at line 146, the pass-through observer `ObserveWorldUpdateGate` (logs `remo154`), and `UpdateNativeSceneCpu`
   (line 5332, replaces `E006A0`), which reads `[4791248]+8` (`+0x154`, holder at `+0x158`), logs `native_drawparam_holder` and applies `stage+0x508` through `D52B70` (the scene parameter holder that the manager sets for a cutscene, see section 5). The remaining consequences are on the GPU and presentation side (section 6).
3. **No native run in the project notes has played a cutscene** (no note mentions one; the observers would log `remo154 != 0`). Everything below about native behaviour is therefore a static prediction, with live rows to settle it (section 7).
4. **Correction to the brief:** the cutscene subtitle does not use caption slot 0. `FUN_140CCBC90` pushes `{text id, fade, kind 1}`, which writes **slot 1** (shared with talk messages); slot 0 is reserved for Bink movie subtitles (SUBTITLES-DIALOGUE-NATIVE sections 2 and 3). The same function also starts a sound on the Env sound system (type 7, second field of the row): INFERRED the voice line.

## 1. The data (STATIC)

- **Id and file name.** A cutscene id is eight digits `AABBVVVV`, for example `30000030` -> `s30_00_0030`. The name is built by `FUN_140CCDA20` -> `FUN_140CE1C90` (whose fallback format is `sAA_BB_VVVV` when `FUN_140CE1B10` fails). The path builder `FUN_140E9D3E0` takes `area = (id / 1000000) % 100`:
  `45` -> `remo_dlc1:/%s.remobnd`, `50` or `51` -> `remo_dlc2:/%s.remobnd`, anything else -> `remo:/%s.remobnd` (`14299F970`, `14299F910`, `14299F940`); the roots are `data1:/remo`, `game_dlc1:/remo`, `game_dlc2:/remo`. The sound companion path is `sndremo:/%s` (`FUN_140CE2010`).
- **Bundle layout** (`remobnd-contents-witnesses.txt`): `.../remo/<name>/<name>.mqb` (the timeline) and `cutNNNN/aNNNN.hkx` (Havok animations, usually one per cut; `s45_00_0000` has 7 for 6 cuts; the MQB names them `a0000.HKT`). Sizes of the MQB range from 1.6 KB (a bare fade) to 487 KB (`s45_00_0010`, 12 cuts).
- **EMEVD triggers.** Bank `2002` instructions `[3]`, `[4]`, `[9]`, `[10]`, `[11]` and `[12]` carry the cutscene id as their first argument (95 instructions in 14 maps and `common`, 41 distinct non-zero ids, `emevd-bank2002-all.txt`); the usual shape is `2002[4] id,0,x,area,10000` then `2002[4] id,10,...` then `2002[3] id,2,10000`
  (`x` such as `3002810` looks like an entity or region id: INFERRED). Calls whose id comes from an event parameter add more: `m33_00 ev=13300412` passes id 0, filled from `2000[0]` instructions with `33000020..33000043`.
- **Census of the 72 bundles** (`remobnd-census.txt`, property names found in the MQB schema strings, number of bundles containing each): `dmypolyId` 57 (shared by effect and sound tracks; `sfxId` 52, `slotNo` 53), 50 switch scene parameter groups (`begin/end <Light|Fog|Scattering|Envmap|Shadow|DofGlareQuality|ToneMap|Dof|Bloom|ColorGrading|LightShaft|FeedbackBlur|MotionBlur|Antialias|Ssao|DominantLightMap> Bank/Id`),
  48 set model draw parameters (`forChr`, `forObj`, `forMap`, `fadeCol`), 37 play sound effects (`seType`, `seId`), 32 fade (`beginFadeCol`, `endFadeCol`), 26 carry light-like properties (`DiffuseColor`, `SepcularColor`, `Radius`, `ShadingModel`; `DOF` 13, `SSAO` 3, `Spot` 2), 21 carry subtitles (`talkParamId`),
  18 fade a draw colour (`beginAlpha`), 9 rumble (`rumbleId`), 8 edit a material parameter (`materialName`, `paramName`, `blendRate`), 2 spawn decals (`decalId`). Character resources are named `cNNNN_NNNN`, dummy objects `dNNNN_NNNN`, map objects `oNNNNNN_NNNN`, map hit toggles `hMM_NN_NN_NN_NNNNNN`, cameras `camNNNN`.
- **Witnesses (data).**
  - *Base-game boss introduction*: `30000030` = `remo/s30_00_0030.remobnd.dcx` (67 KB MQB, 3 cuts 0010, 0020, 0060 with one camera each, resources `c0000_0000` (player), `c2240_0000`, `d0000_0000..0005`, `d1000_0000`, a map hit toggle `h30_00_00_00_995020`, effect and `HU-` tracks), played by event `13005810` of `m30_00`
    (entity `3000800`, flags `13000800`, `13000801`, ids `3002810`, `3002811`; `emevd-m30_00-event-13005810.txt`). That it is the area boss introduction is INFERRED (entity `3000800` with defeat flag `13000800`); no boss name was read in the data.
  - *DLC boss introduction*: `45000000` = `DLC1` `remo/s45_00_0000.remobnd.dcx` (128 KB MQB, 6 cuts, resources `c0000`, `c6010_0001`, `c6020_0000`, objects `o459020_0000/8000`, dummies `d0000/d0001/d0003`, cameras `cam0010..0050`, a `DoFocus` track), played by event `14505810` of `m45_00` (entities `4500800`, `4500801`, `4500802`, flag `14500800`).
  - *Cutscene with subtitles*: `30000040` (`s30_00_0040`, `talkParamId`, 8 cuts, event `13005830`). *Bare fade*: `s37_00_0000` (2.1 KB, one `beginFadeCol` track).

## 2. The lifecycle (STATIC)

| Stage | Code | Content |
|---|---|---|
| Request | EMEVD handlers `FUN_140473CF0`, `140474280`, `1404743B0`, `1404744F0`, `140474640`, `1404747A0`, `140474E90` (variants with an entity or two entities) | build a `SprjRemoRequest` (`FUN_140CDB200`; id, mode, attachment list at `+0x230`: `PlayerWarp`, `PlayerWarpByPoint`, `LuaEventManRemoCb`, `SetupCeremonyParam`), then `SprjRemoImp::Request` `FUN_140CE2810` -> `SprjRemoMan::Request` `FUN_140CD8A50`, which allocates a 0x140-byte `SprjRemoIns` (`FUN_140CD4360`) and queues it at `Man+0xD0` |
| Singleton and queries | `SprjRemoImp` (0x40 bytes at `[144791248]`, `FUN_140CE3EB0`); `Man` is `Imp+8` | `FUN_140CE2790` = **`Man+0x154` bit 0**; `2730` = bit 1, `27A0` = bit 5, `2700` = bit 6, `27B0` = bit 7 (1 when no Man); `FUN_140CE2750` = "is cutscene id N playing"; `FUN_140CE27D0` = stop request by id (INFERRED: sets `Ins+0xCC` bit 1 through `FUN_140CD5A70`); `FUN_140CE2830` = finish request (sets `Man+0x154 \|= 0x100` or finishes the Ins) |
| Manager steps | `SprjRemoMan` (`FUN_142687BE0`): Init `140CD9660`, RegistWait `140CD9950`, Prologue `140CD9680`, PrologueWait `140CD9810`, Update `140CD9BF0`, Epilog `140CD8D00`, EpilogWait `140CD90C0`, FinishWait `140CD95B0`, Finish `140CD9420` | RegistWait clears the flag bits, resets the camera fields and, unless `Man+0x150` equals `0x1D928F2` (31009010), sets **fade plate 3** (`FUN_140D0FD40(fade, 3, ...)`, `FUN_140D0FE20`); Prologue and PrologueWait set bit 0 again; PrologueWait polls plate 3 (`FUN_140D0FCB0(fade, 3)`) and proceeds when it returns non-zero, then starts the Ins (`FUN_140CD4C40/5930/5960`); every Update advances the FD4 remo manager (`FUN_14235CD30`, `[1449649A0]`) and the current Ins (vtable `+0xA8`) |
| Resource steps | `SprjRemoRes` (`FUN_142687F70`): Load, LoadRemobnd, LoadExtraResource, LoadMapTpf, ResourceIsReady, Unload | `LoadRemobnd` requests two file caps from the file manager `[1447559C8]`: the remobnd (`FUN_140CE1F80` -> `FUN_1405060B0`) and a companion (`FUN_140A38650` -> `FUN_140504F50`); `LoadExtraResource` loads the chr and object resources of the cut list and a sound stream (`[14479A448]`, `FUN_140E7F160`) and uses the readiness check `FUN_140CE0EB0` (init `CDE5B0:282`, wait `CDEE50:89`); `LoadMapTpf` makes the map pieces resident (`FUN_140CDFC10`: `FUN_140605600/610`, `FUN_1406068A0(.., 300, 0x1C2)`); `ResourceIsReady` is `FUN_140CDF5A0` (not exported) |
| Instance steps | `SprjRemoIns` (`FUN_1426878B0`): Init `140CD5C10`, Play_Init `140CD5C80`, Play_Wait `140CD5D80`, Finish `140CD5B10`, Finish_Wait `140CD5BD0`, DummyForWarp_* | Play_Init requests the cutscene sound stream (`FUN_140E804E0`, handle at `Ins+0xD8`); **Play_Wait** each update: `FUN_14235D6C0` copies a 64-byte matrix from the Env (`FUN_140CCDA00`) to the FD4 remo player (`+0x2A0`) and a derived matrix to `+0x2E0`, `FUN_14235D8E0(dt)` advances time, and while the player is not past its end (`FUN_14235D760` false) the Env sub-systems are updated (`FUN_140CD0550` sfx system, `FUN_140CD3170` sound system, `FUN_140CCE980` setting block, `FUN_140CCDF90`); the 64-byte transform result (`FUN_14235D710`) is stored at `Ins+0xE0`; at the end `Ins+0x12C \|= 2` and the stop result is computed (`FUN_140CD63E0`) |
| Frame task | `SprjRemoStep` (FD4 task, vtable `1429432D8`, steps Init, Update, Finish) | its body was not read (OPEN); INFERRED to tick the manager once per frame |

`Man::Update` (`FUN_140CD9BF0`) copies from the current Ins to `Man+0x100..0x13F` (16 floats), `+0x140`, `+0x144`, `+0x148`, writes `+0x14C = 0`, and republishes `+0x154` bits 0 to 7 (bit 0 also covers the fade-in because Prologue sets it before the Ins plays); it also drives **plate 3** itself: `FUN_140D0FD40(fade, 3, ...)` on every update and, when bit 1 and `0x100` are set, a timed fade
(`FUN_140D0FB70`, `FUN_140D0FE20`) that is the exit and skip fade. Bit 7 mirrors `Env+0x8F4` (`FUN_140CD58D0`), bits 5 and 6 are Ins-active-and-Env conditions (`FUN_140CD5A10`, `FUN_140CD59E0`).

## 3. The acts: what each one drives (STATIC unless marked)

All 17 act classes share one 22-slot vtable layout (`SprjRemoAct`; INFERRED roles: slot 8 begin, slot 9 per-update apply, slot 11 end; several acts keep slots 9 and 11 empty and do their work in slot 7 or 8). The object they drive is the `SprjRemoEnv` of the Ins (`Ins+0xC0`, reached through the accessors `FUN_140CCDAE0..140CCDB70`:
request `+0x18`, resource block `+0x260`, sfx system `+0x590`, sound system `+0x598`, setting `+0x5A0`, scene-draw-param set `+0x5A8`, model-draw-param sets `+0x750`, `+0x7D8`, `+0x860`, seamless data `+0x8E8..0x8F4`).

| Act (function that does the work) | What it does | Native-side dependency |
|---|---|---|
| `Anim` (slot 8 `140CC2AD0` 77 KB, slot 9 `140CC5F60`; not read in detail) | INFERRED: binds a character, object or camera resource to its Havok animation (`cut....hkx`) | CPU pose; the pose reaches the native model pose paths (`UpdateNativeModelPose*` hooks) like any character; camera: section 4 |
| `Dmyobj` (slot 8 `140CC73C0` creates a 0xA0-byte object, slot 9 `140CC74E0` copies the track matrix into it) | INFERRED the carrier of the dynamic light, DOF and SSAO objects (the census properties `DiffuseColor`, `Radius`, `DOF`, `SSAO`) | light and post consumers (R8, R9) |
| `Sfx` (slot 9 `140CCB3B0`) | spawns an effect in the Env sfx system: `FUN_140CD0480(sys, +0x248, id, entity)` when `+0x224` is negative, else `FUN_140CD03B0(sys, slot)` (a stop was not seen) | native effect path (`DeferNativeSfxSceneRequest`, `NativeSfxViewAttach`) |
| `Decal` (slot 8 `140CC7110`; slots 9 and 11 are empty returns) | INFERRED decal request through `thunk_FUN_1453C54C0` with a row from `FUN_140E2A330` and the track transform | native decal registry (`RequestNativeDecalCpu`) |
| `SceneDrawParam` (slot 9 `140CCA850`) | 12 entries (stride `0x130`) per update: gparam group switches by (bank, id) with begin and end blend (`FUN_140D52AA0`) | gparam banks: areas `9` `MapOverride_forRemo` and `10` `RemoBase` (MENUS-MAP 27) must resolve (R7) |
| `IngameDrawParam` (slot 9 `140CC8180`) | writes the in-game draw parameter blocks of the renderer manager (`[144961B50]+0x41E0..`, five families through `FUN_1400320B0`, `1400387D0`, `14003D080`, `140040690`, `140045560`) | the same producers, native consumers of the post groups |
| `ModelDrawParam` (slot 9 `140CC9540`) | per character, object and map model draw parameters (`FUN_140D3BEA0`, flags `+0x191`, `+0x1D9`, `+0x221`) | model draw state in the native model preparation |
| `MaterialParam` (slot 9 `140CC8C10`) | edits a named material parameter with a blend rate (`FUN_140CC0FB0`) | native material parameter producers |
| `DrawAlpha` (slot 9 `140CC7740`) | applies a colour (RGBA floats unpacked from `FUN_1402D94F0`) to the entity draw colour (`FUN_140CC1080`) | instance colour/alpha in the native model instance data |
| `FadeInOut` (slot 9 `140CC7BD0`; slots 8 and 11 `140CC7B90`, `140CC7BB0`) | `Setting+0` counter +1 at begin, -1 at end; each update sets **fade plate 4** (`FUN_140D0FD40(fade, 4, colour)`) | fade plates (R10) |
| `Subtitle` (slot 9 `140CCBC90`, blank `140CCBC60`) | text id from a TalkParam row, fade by phase, pushed as `{id, fade, kind 1}` every update (`FUN_140765060`); also starts a sound on the Env sound system (type 7) | caption window, slot 1 (R11); sound (R12) |
| `Se` (slot 8 `140CCAE10`, slot 9 `140CCAEE0`) | plays a sound effect by (`seType`, `seId`) through the Env sound system at a dummy polygon (`FUN_140CD30A0` / `FUN_140CD2F00`) | FMOD path |
| `StreamSound` (slot 4 `140CCB8B0`, slot 8 `140CCB8A0`; slots 9 and 11 empty) | obtains a stream handle (`FUN_140E804E0`, stored at `+0x200`); slot 8 sets its state byte to 2 (INFERRED: play) | FMOD path |
| `BossBgm` (slots 7 and 8 `140CC6EA0`, `140CC6E80` -> `140CC6F00`) | starts a boss music through the map sound manager `[144752F80]+0x30` (`FUN_1404DA1D0` with the id at `+0x194`, or `FUN_1404DA170` for the default) | FMOD path |
| `StopMapBgm` (slot 8 `140CCB5F0` -> `140CCB620`) | stops the map music (`FUN_1404DA0A0`) and records it (`FUN_140CCDC20`) | FMOD path |
| `Rumble` (slot 8 `140CC9A10`; slots 9 and 11 empty) | pad rumble by id (`FUN_1407C8200`, `[144769268]`) | none (input side) |
| `SeamlessFinish` (slot 9 `140CCB130`) | stores its parameter at `Env+0x8F0` and calls `FUN_140CCDC00(env, 1)`; INFERRED "ends the cutscene without a closing fade" | none on the GPU |

The attachments (`SprjRemoAttachment*`: `PlayerWarp`, `PlayerWarpByPoint`, `LuaEventManRemoCb`, `SetupCeremonyParam`) are CPU actions at start and end: warp the player to the cutscene position, call the Lua event manager back, set ceremony parameters.

## 4. Camera, characters and objects, map (STATIC / INFERRED)

- **Camera.** The FD4 remo player produces a 64-byte transform each update (`FUN_14235D710`, from the player block at `+0x320`), stored in the Ins and copied to `Man+0x100` with the lens value at `Man+0x140`. The consumer that turns it into the scene camera was **not located**:
  the readers of the Imp singleton (section 5) do not read the matrix, and `Man+0x100` has no other reader through the singleton (OPEN, row RL3). The native scene camera (`ProduceNativeSceneCamera`, hook `0xCED670` at its `E0081C` call site) copies the camera the game computed (`ingress::CopySceneView`, log `native_scene_camera_cpu_produced`
  with `genuine_current_pose`), so a Remo camera would be followed without a Remo-specific change **provided the game routes it into that camera**.
- **Characters and objects.** The MQB names the entities (`c0000_0000` is the player, `cNNNN_NNNN` NPC and boss models, `dNNNN` dummies, `oNNNNNN` map objects); `Anim` drives their pose from the cut animation; `ModelDrawParam` and `DrawAlpha` change how they are drawn.
- **World update during a cutscene.** The caller of the world gate, `MoveMapStep` `FUN_1408F2CA0`, does the freezing: when bit 0 is set and bit 5 is clear it writes `step+0x270` (`running`) = 0; when bit 1 is also set it clears `step+0x271`, sets `step+0x70` = 3 and, if bit 7 is clear, calls `FUN_140E6E720` with an effect-hide mask
  (the `SprjSfxImp+0x148` mask, EFFECT-LIGHTS.md). `FUN_1408FA3E0` (hooked by the observer) then reads bit 0 to choose mode 1 or 2 when `running` is false, which registers the reduced task set (LEGACY-ROOT-INVENTORY-903; native comment at line 143).
- **Map.** `LoadMapTpf` makes map pieces resident; map hit toggles (`h..._995020`) switch collision and visibility; the gparam area 9 `MapOverride_forRemo` and the texture sets named `(forRemo)` / `HiResTexture(forRemo)` (strings `14297A5D2`, `14297A5E8`) are the cutscene-specific map data.

## 5. Who reads "a cutscene is running" (STATIC)

42 functions read the Imp singleton `[144791248]` (`imp-singleton-readers-w144791248.txt`, a `w:` search); 22 of them call the bit-0 getter `FUN_140CE2790`, the others use sibling getters (`FUN_140CE2730`, `27A0`, `27B0`, `2700`, `2740`, `27C0`, `26E0`, `28E0`) or the request methods. What the functions read do:

- *Request and control (10):* the seven EMEVD request handlers (`140473CF0`, `140474280`, `1404743B0`, `1404744F0`, `140474640`, `1404747A0`, `140474E90`), the pad skip and finish handler `1408FB6D0` (reads the confirm inputs, then `FUN_140CE27D0` or `FUN_140CE2830`), `140904710` (INFERRED: finish request when the
  cutscene list drains) and `14065AB10` (a character state routine that special-cases cutscene id `40000020` (`0x2625A14`) through `FUN_140CE2750`).
- *World and scene state with a rendering effect:* `1408F2CA0` (`MoveMapStep`, section 4: freezes `running`, sets the effect-hide mask); `1408FBD20` (sets the stage scene parameter holder `stage+0x508` from the manager when bit 1 is set, the value that `UpdateNativeSceneCpu` applies); `1408FA7F0` (uses `FUN_140CE26E0`, `Env+0x8E8`, to override the player's
  collision part, which selects the gparam targets, MENUS-MAP around line 4017, "Targets come from the player's collision part"); `140DE5940`, `140DE8330`, `140DE8600`, `140DE8680` (enable or disable a per-camera draw group depending on the flag; the group was not identified: OPEN, row RL5; `140DE5A40` also reads the singleton).
- *Predicates:* `1404708E0` returns bit 0; `140BEA4E0` = "menu open or cutscene running" (8 callers, including the pad handler and the New Game chain); `140527540` sets a flag to `!running`; `140C57AE0` skips its action while running; `140CB4F70` returns false while running; `1408FC2A0` writes a per-frame flag at `[144796298]+0x573`; `1408F2B40` uses `FUN_140CE28E0`.
- *Others (not read in detail):* `140608A00`, `14056C9F0`, `14078D6C0`, `14078F4B0`, `1408F36F0`, `1408F5D20`, `1408FAA20`, `1408FB2E0`, `1408FB610`, `1408FC0A0`, `1408FEC40`, `140CB3CA0`, `140CB52F0`.

The functions are CPU code that runs unchanged natively; the rendering effects are the effect-hide mask, the scene parameter holder, the gparam target override and the draw-group switches.

## 6. What the native build does with each piece

| # | Piece | Native today | Needed for a faithful cutscene |
|---|---|---|---|
| R1 | Request, queue, steps, file caps, sound, Lua callbacks (all genuine CPU) | not hooked; runs as in the original (INFERRED, never exercised) | nothing; verify with RL1 |
| R2 | World freeze (`8F2CA0`, gate `8FA3E0`) | `ObserveWorldUpdateGate` pass-through, logs `remo154` | keep pass-through; the log row is the witness that the cutscene started |
| R3 | Data loading: `remo:/`, `remo_dlc1:/`, `remo_dlc2:/`, `sndremo:/`, hkx repository | goes through the genuine VFS if the native build keeps it (INFERRED) | the three roots and the `DLC2` bundles must be reachable; RL1, RL9 |
| R4 | Camera | follows the camera the game computed (`ProduceNativeSceneCamera`) | confirm the Remo camera reaches it (RL3) |
| R5 | Characters and objects | genuine pose update; native model pose and model preparation hooks | boss, NPC and dummy-object models of the cut resident and drawn; map hit toggles |
| R6 | Effects (`Sfx`), decals (`Decal`), effect-hide mask | native sfx records and decal registry exist; the mask is read by the effect system | the cutscene effect slots (`slotNo`), long-lived decals and the hide mask; not exercised |
| R7 | Scene parameters (`SceneDrawParam`, `IngameDrawParam`, gparam areas 9 and 10, holder `stage+0x508`) | `UpdateNativeSceneCpu` applies `stage+0x508` through `D52B70` and logs `native_drawparam_holder`; `Banks::Load` reads every `*.gparam` of the staged directory by file name (`light_producers.cpp:1118`) | `remobase.gparam` and `mapoverride_forremo` must be staged next to `default`; the (area, id) keys written by the acts must resolve; the log row proves it (RL10) |
| R8 | Dynamic lights and DOF objects of a cut (`Dmyobj`) | the light producers cover map and effect lights; no cutscene light source is documented | a cutscene light path (RL6) |
| R9 | Depth of field, SSAO, bloom, colour grading during a cut | the post transition intent is published but not drawn (`native_post_draw_ready:false`) | native post consumers (I-0068 specs) |
| R10 | Fade plates 3 and 4 (cutscene enter and exit, `FadeInOut` act) | **no drawer in `native/`** (no `D0E790`, `D0FE60`, `D0FD40`, `FadeSystem`); contract in MENUS-MAP 6: full-screen quads, blend state 7, 1920x1080 virtual canvas, plate 6 `InGameChroma` tint | draw the plates. **Live check:** plate state only advances inside `FUN_140D0FE60`, which runs from the draw side (`FUN_140DFDC50` <- `FUN_140D54F80`); if the native frame skips it, plate 3 never finishes and `PrologueWait` could block the start of the cutscene (RL8) |
| R11 | Subtitles | caption queue and Scaleform window are genuine CPU and UI (SUBTITLES-DIALOGUE-NATIVE) | slot 1, kind 1: no new work |
| R12 | Sound: `Se`, `StreamSound`, `BossBgm`, `StopMapBgm`, subtitle voice | FMOD path genuine; `ObserveSoundEvent` observer | none (CPU/FMOD); music handover between map and boss BGM to be witnessed (RL7) |
| R13 | HUD and input gating (42 readers) | genuine CPU | none |
| R14 | Video | not involved (the opening and ending are Bink, `BINK-MOVIES-NATIVE.md`) | none |

## 7. Witnesses and live rows (read-only)

**Witnesses.** RW1 base-game boss introduction `30000030` (event `13005810`, `m30_00`); RW2 DLC boss introduction `45000000` (event `14505810`, `m45_00`, `DLC1`); RW3 a cutscene with captions, `30000040`; RW4 a bare fade, `s37_00_0000`.
A pass criterion for each: `remo154` bit 0 set while the cutscene runs and cleared after, the cutscene camera framing visible, the fades visible at both ends, the effect and light tracks visible, the music handover and the caption (RW3) correct.

| Row | Action | Expected |
|---|---|---|
| RL1 | Reference run: trigger RW1 (fog gate of the `m30_00` boss); log `FUN_140CD8A50` (id), the Res step transitions and the two file cap paths | `remo:/s30_00_0030.remobnd` requested; Res reaches `ResourceIsReady`; the same rows under the native build |
| RL2 | Same run: log `ObserveWorldUpdateGate` rows (`remo154`) from request to end | bit 0 set for the cutscene, bits 1, 5, 7 recorded |
| RL3 | Break on the writer of the scene camera (`CED670` call site `E0081C`) and on `Man+0x100` readers during the cutscene | which function copies the cutscene transform into the camera |
| RL4 | Dump the 64 bytes at `Ins+0xE0` and the lens at `Man+0x140` each update | a path matching the `cam0010` keyframes |
| RL5 | Log the draw groups toggled by `140DE8330/8600/8680` when bit 0 changes | the group that is hidden during a cutscene |
| RL6 | List the lights registered while the cutscene runs and their source | cutscene lights present or absent natively |
| RL7 | Sound: log `FUN_140CCB8B0`, `140CC6EA0`, `140CCB5F0` and the FMOD event ids | boss music starts, map music stops |
| RL8 | Fade plates 3 and 4: log colour and alpha per update, and whether `FUN_140D0FE60` runs natively | the same curve natively; `PrologueWait` is released (needs the plate update, R10) |
| RL9 | `DLC2` bundle: read one of `remo_dlc2:/s51_00_0000.remobnd` through the engine | bundle opens (unnamed in the dictionary, so unread statically) |
| RL10 | Log `native_drawparam_holder` rows during the cutscene (`flags`, `area_holder`, tokens) | the holder is present and its tokens match the expected pair |

## 8. Open points

1. The consumer of the cutscene camera (section 4) and the identity of the draw groups switched by the `140DE8xxx` functions.
2. The `DLC2` bundles (`s50`, `s51`) were not read; the boss names are inferred from entity numbers, not from text.
3. Whether the native build keeps the genuine VFS roots for `remo*:/` (R3) and whether it stages `remobase.gparam` (R7).
4. The meaning of `Man+0x154` bits beyond those named in section 2, the body of `SprjRemoStep`, and the `Anim` act.

## 9. Exports

`decompiled/DS3-1152-F-` + `026878B0`, `02687BE0`, `02687F70`, `00E9D3E0`, `00CE1C90`, `00CE2010`, `00CE1F80`, the Ins steps `00CD5C10`, `00CD5C80`, `00CD5D80`, `00CD5B10`, `00CD5BD0`, `00CD5A90`, `00CD5AE0`, `00CD6140`, the Man steps `00CD9660`, `00CD9950`, `00CD9680`, `00CD9810`, `00CD9BF0`, `00CD8D00`, `00CD90C0`, `00CD95B0`, `00CD9420`, `00CD8A50`,
`00CD8B90`, `00CD89D0`, `00CD8C30`, `00CDA510`, the Res steps `00CDE590`, `00CDF4F0`, `00CDF2E0`, `00CDF480`, `00CDE5B0`, `00CDEE50`, `00CDF230`, `00CDF290`, `00CDF5D0`, `00CDF940`, `00CDF540`, `00CDF970`, `00CDFC10`, `00CE0EB0`, the Imp methods `00CE26E0`, `00CE2700`, `00CE2730`, `00CE2740`, `00CE2750`, `00CE2790`, `00CE27A0`, `00CE27B0`, `00CE27C0`, `00CE27D0`,
`00CE2810`, `00CE2830`, `00CE2950`, `00CE3EB0`, `00CE3E30`, the EMEVD handlers `00473CF0`, `00474280`, `00474E90`, the request builders `00CDB200`, `00CDB600`, `00CDB6A0`, `00CDB430`, `00CDB360`, `00CD4360`, the Env accessors `00CCDAE0` to `00CCDB70` (even slots) and `00CC1350`, the Env sub-systems `00CD2FD0`, `00CD2F00`, `00CD30A0`, `00CD03B0`, `00CD0480`, the act functions `00CCBC90`, `00CCBC60`,
`00CC7B90`, `00CC7BD0`, `00CC7BB0`, `00CCAE10`, `00CCAEE0`, `00CCB3B0`, `00CCB8B0`, `00CCB8A0`, `00CC9A10`, `00CCB130`, `00CC7110`, `00CC73C0`, `00CC74E0`, `00CC7740`, `00CC8180`, `00CC8BB0`, `00CC8C10`, `00CC9540`, `00CCA850`, `00CC5F60`, `00CC6EA0`, `00CCB5F0`, the FD4 player accessors `0235D6C0`, `0235D710`, `0235D750`, `0235D760`, `0235D890`, `0235D8B0`, `00CD5240`, `00CD59D0`, `00CD58D0`,
`00CD5A10`, `00CD59E0`, and the readers `008FB6D0`, `008F2CA0`, `008FBD20`, `008FA7F0`, `008FA3E0`, `008F2B40`, `004708E0`, `00BEA4E0`, `00527540`, `00C57AE0`, `00CB4F70`, `008FC2A0`, `0065AB10`, `00904710`, `00DE5940`, `00DE8330`, `00DE8600`, `00DE8680`. `008FB6D0` is the pad-confirm handler that issues the skip and finish requests.
