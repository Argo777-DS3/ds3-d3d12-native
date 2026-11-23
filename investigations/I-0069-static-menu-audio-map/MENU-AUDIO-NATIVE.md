# Menu audio as a native checklist (I-0069)

STATIC (Ghidra mirror of `DarkSoulsIII.exe` 1.15.2.0) + game data read offline (`sound/*.fev` of Data1, the installed `sound/` folder) + the earlier live logs of I-0066.
Nothing was launched. Status words: **STATIC** = read in code or data, **INFERRED** = deduced, **OPEN** = not known statically.
Baseline: [MENUS-MAP.md](../I-0067-static-menus-subsystems-map/MENUS-MAP.md) sections 3 and 28 (request API, ids, "no rendering dependence", INFERRED banks). This note
confirms the banks, adds the engine, the configuration and the volume path, and fixes what the native build does.
Strings and tables quoted from the executable (configuration text, bus names, the category letter table, call-site counts) were read in the Ghidra mirror and are not in the repository exports. Evidence (outside Git): `evidence/I-0069/queries/menu-audio/` (`fev-event-names.txt`, `install-sound-dir.txt`, `native-sound-event-requests-argo8.jsonl`).

## 0. Verdict

1. **Menu audio is FMOD Ex plus the FMOD Designer event system, wrapped by the FromSoftware "MO" sound manager; nothing in it touches the graphics device.** Scaleform has no
   audio (`GFx_*SoundLoader: Audio library is not set.` strings, and the menu movies carry no sound tags, MENUS-MAP 78).
2. **The native build already plays it.** `NativeMainServices` runs the genuine `FUN_140814B50` / `FUN_1408153B0` audio initialization
   (`game_bootstrap.cpp:8813-8814`; the original `FUN_140EDF140` does the same), the menu logic issues the same requests as the reference (below), and Thomas heard the
   title music and the menu effects natively (I-0066 witness 1175, title music confirmed again at Native75). **Nothing audio-related is deferred or absent in
   `game_bootstrap.cpp`**; the deferred items next to it are visual (the debug font and the SFX visuals, `native_optional_visual_deferred`, `sfx_visuals_present:false`).
3. What audio *does* depend on is not D3D11 but liveness: the menu logic must run (it issues the requests), the per-frame sound update must keep being scheduled, and the settings
   object must be loaded (volumes). A native frame loop that stalls (for example a graphics wait that never returns) silences the game without any audio defect.

## 1. The engine and its configuration (STATIC)

| Item | Where | Content |
|---|---|---|
| Libraries | imports | `fmodex64.dll`, `fmod_event64.dll` (`FMOD_EventSystem_Create` is imported by name), `binkw64.dll` for movies |
| Wrapper | `..\..\Source\FMOD\MOFmodSoundManager.cpp`, `MOFmodSoundBank.cpp` | the FromSoftware manager; `FUN_141A37830` (8,330 bytes) is its initializer: `FMOD_Memory_Initialize` with the game allocator, FMOD `setCallback`, four **custom file callbacks** (`FUN_141A59B10/59A20/59EF0/59FB0`) so FMOD reads through the game's virtual file system, event system creation, loading of the mixer `.fev` |
| Audio service | `FUN_140814B50` (singleton `_DAT_14477D6C8`), `FUN_1408153B0(service)` | creates the 0x1D8-byte audio object (`FUN_141A6FCD0`) and sets its memory pools through `FUN_141A70C40` (ten pools of 0xC0000, one of 0x60000, one of 0x100010) and a 0x894000 setting (`FUN_141A70D80`); the exact role of these pools was not read |
| Compiled-in configuration text | `142829EFF..14282A0xx` | `CueMaxNum= 400`, `BankSetMaxNum= 66`, `EmitterMaxNum= 400`, `MessageBuffSize= 262144`, `SoundOfSpeed= 340.29`, `FmodSoundHomePath= fmod:`, `FmodEventFilePathNum= 1`, `FmodEventFilePath_0= ${FmodSoundHomePath}/fdp_mixer.fev`, `FmodDebugLogLevel= 0`, `FmodStreamBuffSize= 196608`, `MainThreadUpdateTime= 33333`, `UseFmodLoader= FALSE`, `FmodRPCSettingPath= .../fdp_multiRPC.rpc`, `FmodMixerDataPath= .../fdp_multiMix.mix`, `FmodMultiChannnelDataPath= .../fdp_multich.mch`, `IsDivideFEV= FALSE`, `FmodSPURS= 1`, `UseMSP= true`, `UseHDR= false`, `UseITLimiter= true`, `FmodThreadCoreID= 3`, `ChannelPoolNum= 48`, `FmodMaxXMANum= 48`, thread heap/stack sizes |
| Sound bus names | wide strings `14282A6xx` | `Default`, `Ghost.SE`, `Music`, `Voice`, `old SOUND`, `RPC`, `HDR`, `ITL`, `InGameMaster`, `Bgm`, `Se` and the debug format `curVol[%0.2f] tarVol[%0.2f] remainTime[%0.2f]<%sVol` (a volume debug display; no code reference found) |

The `fmod:` root is the install `sound/` folder: `fdp_mixer.fev`, `fdp_multirpc.rpc`, `fdp_multimix.mix`, `fdp_multich.mch` and the streamed banks `fdp_smain.fsb` (6,464,320 bytes) and
`fdp_smain_dlc1.fsb` (314,400) are **loose files**; `fdp_main` and the DLC banks, the per-character banks `fdp_cNNNN` and the per-map banks `fdp_rm/sm/vm/xmNN` are in the archives
(Data1 and Data5, `data1/sound/*.fev/.fsb/.itl`); the install folder also holds loose `fdp_rm/sm/vm/xmNN.fsb`. **INFERRED:** which of them FMOD reads through which callback (`sound:` against `fmod:`) was not traced.

## 2. Banks and event names (STATIC, from the `.fev` files)

Resident bank set (data table `1446554A8..1446554D0`): `fdp_main_dlc1` / `sound:/fdp_main_dlc1`, `fdp_main_dlc2`, `fdp_main`. Streaming bank `fdp_smain` (+ `_dlc1`).
Event name = category letter (table `142786578`: `a c f o p s m v x b g y z e`, index = category) followed by nine digits (`FUN_1404AD2D0`).

| Bank | Events | Menu relevant |
|---|---|---|
| `fdp_main.fev` (1.8 MB, `.fsb` 23.2 MB in Data1) | 2,726 names: `c` 643, `e` 153, `f` 61, `g` 586, `o` 43, `s` 503, `v` 9, `z` 728 | **`f` = menu sound effects**: `f000000000, 15, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1010, 1020, 1030, 2000, 2001, 10000, 30000, 30100, 40000..40700, 44441..44450, 70000, 70010, 1044441..1044450, 2044441..2044450` and **`f999999999`**, which exists as an event (the sentinel id of the menu code, `FUN_14076BA90` reserves it a channel; that the event is silent was not read from the bank) |
| `fdp_smain.fev` (12 KB, streamed from `fdp_smain.fsb`) | 24 names: `m000000001`, `m000000010`, `v019690225`, `v444444444..448`, `v999999999`, two `c` and thirteen `s` events; groups `master`, `music`, `Ghost`, `Voice`, `Default` | **`m000000001` = title music**, `m000000010` the other menu music |
| `fdp_smain_dlc1.fev` | `v444444444`, `v444444450..453` | voice |
| `fdp_main_dlc1/2.fev` | `c g s z` only (character, generic, SE) | none |

Category 11 (`y`, ids `y300000000`, `y400000000` seen in both reference and native logs) has **no event in these banks** (`y` is not among the letters): **OPEN**, probably a bank of
the character or system families.

## 3. Triggers and the MenuMan sound object (STATIC)

The sound object `[144763868]` (MENUS-MAP 3): 57 SE channel handles at `+0x14 + 8*i` (the player picks channel `FUN_14076BA90`), an interrupt flag at `+0x1EC`, BGM handle `+0x1DC` with
the current BGM id at `+0x10`, a voice handle `+0x1E4`, the last SE id at `+4`.

| Function | Effect |
|---|---|
| `FUN_14076B930(id)` | stop the channel if `+0x1EC`, then play category 2 `f<id>` on the channel (181 old-dialog call sites) |
| `FUN_14076B8B0(ch, id)` | same on a given channel (Scaleform path, `FUN_140A67BD0` at the end of the `NewMenuSystem` update) |
| `FUN_14076B920(id)` | category 6 `m<id>`: if the id differs from `+0x10`, or the handle is not playing, stop the old BGM and start the new one |
| `FUN_14076B9F0()` | stop the BGM, clear `+0x10` |
| `FUN_14076B960(id)` | stops the voice handle `+0x1E4`, plays category 7 `v<id>` on it, stores the id at `+4` |

Every request ends in `FUN_1404AD510(category, id)` -> `FUN_1407CE930` (the sound implementation's request builder; the handle assignment `FUN_141A299F0` only copies the 8-byte handle).

**Observed request sequence at the title, the same skeleton in the reference and the native runs (differences are input-dependent repeats)** (`sound_event_request` rows of I-0066: `i66-reference-ash-1185`, `i66-reference-chrview-1184`,
`i66-native-argo8-1179`): `6:1` (title music) `2:999999999` `2:1010` `2:100` `2:0` `2:1000` then `2:1010 2:100 2:1020` and `11:300000000`/`11:400000000`. The reference ash run has no second `2:0`; the reference chrview run and the native run share `...2:1000 2:0 2:1010 2:100 2:1020 11:300000000` and the native run repeats `11:300000000` once. All the `f` ids exist in
`fdp_main.fev`. Meaning of the ids (cursor 100, decide 200, value change 300, open 1010 or 1000, sentinel 999999999) is **INFERRED** from MENUS-MAP 28.

**Music flow.** Start: `FUN_140AD8900` (TitleTopDialog constructor) and `FUN_140ADA260` call `FUN_14076B920(1)`. Stop: `FUN_140AE8930` (title dialog step) and `FUN_140AE8AB0` call
`FUN_14076B9F0`. The Load Game list is part of the same dialog, so **the title music continues through Load Game until one of those stops runs** (INFERRED; the other music id
`m000000010` has no direct caller of `FUN_14076B920` besides those two, so it is started by a path not read: scripts or the vtable method `FUN_14076BBA0`, **OPEN**).

## 4. Volumes and the per-frame update (STATIC / INFERRED)

The profile settings object `[[1447572B8]+0x58]` (the same object as the language and subtitle settings, `FONTS-LANGUAGES-NATIVE.md`, `SUBTITLES-DIALOGUE-NATIVE.md` 7C) holds the sound options
in bytes `+0xB` (sound type 0, 1 or 2), `+0xC`, `+0xD`, `+0xE` (volumes 0..10). The per-frame update of the sound module `_DAT_144769290`, `FUN_1407CF780(dt)`, reads them:
`+0xC`, `+0xD`, `+0xE` are divided by the constant at `143D7EF40` (a 0..10 scale, **INFERRED** 10.0), clamped to 0..1 and stored as three targets at module `+0x160`, `+0x16C`, `+0x178`
(each target is ramped into `+0x15C / +0x168 / +0x174` with the remaining time at `+0x164 / +0x170 / +0x17C`; two further ramps at `+0x144..+0x14C` and `+0x150..+0x158` exist, the second being a fade that multiplies the bus volumes); `+0xB` is copied to `+0x1C4`. The three volumes are **INFERRED** to be music, SE and voice by the order of the configuration keys
`Game.Option.Sound.SoundType / MusicVol / SeVol / VoiceVol` (`14283C0D0..14283C170`). Mute edges: a state byte `+0x1CD` latches the flags `+0x1C9/+0x1CA`: pause (`FUN_141A2DBD0(0)`) when either flag is set and `+0x1CD == 0`, resume (`FUN_141A2CED0(0)`) when both are clear and `+0x1CD == 1`; `+0x1CA` also zeroes the three volume targets. What these calls do to FMOD (**INFERRED**: pause and resume) and the trigger (for example focus loss) were not read. `+0xB` is copied to `+0x1C4` only for the values 0, 1, 2. `FUN_1407CF780` is called from `FUN_140E80880` and `FUN_140965890` (the first is
**INFERRED** to be the `SprjSoundStep` update, class strings `SprjSoundStep` and `Step<SoundStep>` exist; the call chain to the scheduler was not read).

The option pages write these bytes through the same apply functions as every option (`FUN_140AB4FC0`, `FUN_140BE4520`, `FUN_140BE4DB0`, `FUN_1405A0930`, index 3 to 6 of the page byte array).
An old dialog `FrpgMenuDlgSoundOption` exists (class string) and is not part of the release option page (the Scaleform page is `02_040_OptionSetting`).

## 5. What depends on the renderer (STATIC)

| Item | Depends on D3D11 or the GX frame? |
|---|---|
| FMOD initialization, banks, event requests, volumes | **No** |
| Menu sound requests (old dialogs) | no: they run in their own update; the Scaleform menus issue theirs at the end of the `NewMenuSystem` update, which does not need the renderer but needs that update to run (MENUS-MAP 28) |
| Per-frame sound update | no, but it is a game task: it needs the frame loop to keep running |
| Title music start/stop | no (TitleTopDialog logic) |
| Movie audio | **corrected by [BINK-MOVIES-NATIVE.md](BINK-MOVIES-NATIVE.md) section 3:** the movie audio is mixed by **XAudio2 inside Bink** (`FUN_141ADFD40`: `CoCreateInstance(XAudio2)`, `CreateMasteringVoice`, `BinkSetSoundSystem(BinkOpenXAudio2, ..)`; DirectSound as fallback), not by FMOD, despite the class namespace `Movie_FMOD_Bink_CG2`; the video side draws into an RGBA texture behind `CSMovieGxTexture`. The native build creates no movie player, so it has neither picture nor sound for movies |
| Scaleform menus | silent (no audio library) |

## 6. Native checklist and live rows

| # | Requirement |
|---|---|
| A1 | Keep the genuine audio initialization (`FUN_140814B50` / `FUN_1408153B0`, `FUN_141A37830`) in the native path: do not stub it, do not replace the FMOD file callbacks; the native VFS must serve `sound:` and `fmod:`. |
| A2 | Keep the menu logic and the `NewMenuSystem` update running; the Scaleform sounds are queued there. Do not add a graphics gate in front of the sound step. |
| A3 | Keep the sound step scheduled when the native frame loop waits or refuses a frame: a stalled main loop is the only way the native build can lose menu audio. |
| A4 | The observer `ObserveSoundEvent` (first 120 requests) stays a pass-through; its rows are the regression test (the title sequence above). |
| A5 | Do not treat `f999999999`, `y300000000`, `y400000000` as errors: the first is a real event name (the sentinel), the `y` events are outside the five `.fev` files read. |
| A6 | Volumes come from the settings object: if the profile settings are not loaded, the volumes default (the constructor values of `FUN_1405A0320`, **not read** for these bytes). |
| A7 | Movie audio and video: see [BINK-MOVIES-NATIVE.md](BINK-MOVIES-NATIVE.md) (checklist B1-B8, rows BL1-BL6); the movie audio is XAudio2, outside the FMOD checklist. |

**Static witnesses (AW):** AW1 configuration text (section 1); AW2 `fdp_main.fev` 61 `f` events and `fdp_smain.fev` `m000000001`; AW3 reference and native request sequences identical (section 3);
AW4 `game_bootstrap.cpp:8813-8814` audio initialization; AW5 `FUN_1407CF780` reads settings bytes `+0xB..+0xE`.

**Live rows (AU), read-only:**

| Row | Action | Expected |
|---|---|---|
| AU1 | Native run: log `sound_event_request` for the Load Game list and the pause menu (extend the 120-row cap or filter) | `f100` cursor and `f200` decide rows, no `m` stop until New Game or Continue |
| AU2 | Read `[144763868]+0x10` (BGM id) and `+0x1DC` playing state across Title -> Load Game -> Continue | `1` throughout, stopped by `FUN_14076B9F0` at Continue |
| AU3 | Change the music, SE and voice volume options; read `[144769290]+0x160/+0x16C/+0x178` | the three targets follow the option bytes `+0xC..+0xE` (settles music/SE/voice order) |
| AU4 | Read `[144769290]+0x1C4` and the option byte `+0xB` | the sound type setting |
| AU5 | Native run: confirm that the sound step keeps running while the first game frames load (no silence gap) | continuous music |
| AU6 | Log category 11 requests with their callers | which bank `y3000...` belongs to |

## 7. Open points

1. Bank of the `y` events; the `m000000010` start path; the trigger of the mute flags.
2. The exact volume order (AU3) and the defaults of a new profile for bytes `+0xB..+0xE`.
3. Movie audio/video: done in [BINK-MOVIES-NATIVE.md](BINK-MOVIES-NATIVE.md) (XAudio2, not FMOD).
4. Which loader (`sound:` or `fmod:`) serves each bank in the native VFS.

## 8. Exports

Cited functions are exported to `decompiled/` (`DS3-1152-F-<RVA>`): `FUN_140814B50`, `1408153B0`, `140EDF140`, `141A37830`, `14076B930`, `14076BA90`, `14076B8B0`, `14076B920`,
`14076B9F0`, `14076B960`, `1404AD2D0`, `1404AD510`, `1407CE930`, `1407CF780`, `1407CFEA0`, `140E80880`, `141A299F0`.
