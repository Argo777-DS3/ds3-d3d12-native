# Fade plates as a native checklist (I-0069, Task 12)

STATIC (Ghidra mirror of `DarkSoulsIII.exe` 1.15.2.0 and `native/frame/game_bootstrap.cpp`). Nothing was launched.
Status words: **STATIC** read in code or data, **INFERRED** deduced, **OPEN** not known statically.
Baseline: [MENUS-MAP.md](../I-0067-static-menus-subsystems-map/MENUS-MAP.md) section 6 ("Fade plates: how they are drawn"), sections 33 and 38 (frame order, Scaleform), 35 (title fades), and [REMO-CUTSCENES-NATIVE.md](REMO-CUTSCENES-NATIVE.md) (the cutscene plates).
Evidence outside Git: `evidence/I-0069/queries/fade/call-sites.txt` (every call site of the plate API with its argument set-up).

## 0. Verdict

1. **There are seven plates but the screen gets one quad.** `SprFD4FadeSystem` composes the seven plate colours, in plate order, into **one** RGBA colour (`system+0x20..0x2C`) and `FUN_140D0E790` records **one full-screen rectangle** for it. This corrects MENUS-MAP section 6, which says each visible plate is drawn.
2. **Update and recording are CPU code that the native build does not replace; the replay is.** The per-frame chain `FUN_140D54F80` -> `FUN_140DFDC50` -> `SprjFadeImp` slot 1 `FUN_140D0FE60` (update the plates, compose, record the quad into the Ez 2D list `[RendManImp+0x40]`) is not hooked.
   The replay of that list belongs to `FUN_140DFD270`, which sits under `FUN_140D55190`, and **`NativeRenderStep` replaces `FUN_140D55190`** (`game_bootstrap.cpp:6822`, hook at 9480). So the plates update and are recorded, and **nothing draws them**.
3. **The wait on the fade (`PrologueWait` of a cutscene, the map-move step, many dialogs) is pure CPU state** (`plate+0x38`, the remaining fade time). It does not need a GPU, so the native build does not stall on it as long as the update chain runs; the visible fades are missing.
4. **Plate 6 (`InGameChroma`) is not a quad.** Only its red component goes to the stage as a scalar (`FUN_140D0FE60` -> `FUN_140E004F0` stores one float at `stage+0x4FC` and sets `stage+0x4F8`), and the scene update `FUN_140E006A0` consumes it. **`UpdateNativeSceneCpu` replaces `E006A0`** (hook at line 9403) and reproduces that step (`game_bootstrap.cpp:5430-5441`: clamp to 1, tint from `0x39F2490`, write `context+0x3928..0x3950`, clear the stage fields); the gap is downstream: no native code reads `context+0x3928..`.
5. Two bodies are **encrypted at rest** (Arxan) and could not be read: `FUN_14075EAD0`, the gate that suppresses the draw, and `FUN_140330F50`, the easing curve of the fade. Both are listed as open points with a read-only loaded-code row.

## 1. The objects (STATIC)

| Object | Layout |
|---|---|
| `SprjFadeImp` (global `[144791A38]`, vtable `14294A500`; ctor `FUN_140D0F6E0`) | `+8` the `SprFD4FadeSystem`, `+0x10 + 8*i` plate `i` (0 to 6); slot 1 `FUN_140D0FE60` is the per-frame call |
| `SprFD4FadeSystem` (vtable `14294A250`) | `+8` map plate index -> plate; `+0x20..0x2C` composite RGBA; slot 1 `FUN_140D0E910` update and compose; slot 2 `FUN_140D0E790` record; slot 3 `FUN_140D0E4B0` insert; `FUN_140D0E8F0` read composite |
| `SprFD4FadePlate` (0x78 bytes, vtable `142949D70`; ctor `FUN_140D0D110`) | `+0x10..0x1C` current RGBA, `+0x20..0x2C` target RGBA, `+0x30` easing object (vtable `1427457D8`, `+8` = duration in seconds), `+0x38` remaining time, `+0x40` flags |

**Plate indices and names** (pointer table `14463E0C8`; `FUN_140D0F6E0` builds them in this order): 0 `Title`, 1 `MapIn`, 2 `InGame`, 3 `Remo`, 4 `InRemo`, 5 `Ending`, 6 `InGameChroma`.

**Plate flags** (`+0x40`): bits 0 and 2 freeze the plate (`FUN_140D0D510` skips its update), bits 1 and 3 hide it (`FUN_140D0D3F0` then reports the zero colour). The setters `FUN_140D0FDF0(fade, plate, bool)` and `FUN_140D0FDC0(fade, plate, bool)` write bits 0 and 1; the writers of bits 2 and 3 were not found.

**API** (all take the singleton and a plate index):

| Function | Role |
|---|---|
| `FUN_140D0FD40(fade, plate, colour*)` | set the colour **instantly** (easing object with duration 0 -> `FUN_140D0D490`: current = target = colour, remaining = 0) |
| `FUN_140D0FE20(fade, plate, colour*, ease*)` | set a **timed** fade: target = colour, remaining = `ease+8` seconds (instant when the duration is 0 or less) |
| `FUN_140D0FB70(fade, out*, plate)` / `FUN_140D0FC20` | read the plate's current / target colour (zero for a missing plate) |
| `FUN_140D0FCB0(fade, plate)` | **fade finished**: `remaining <= 0` (true for a missing plate) |
| `FUN_140D0FC00(fade, out*)` | read the composite colour |
| `FUN_140D11530`, `FUN_140D116B0` | fade actions that animate plate 2 |

Two constants are used everywhere: **opaque black** `(0, 0, 0, 1)` = `FUN_1417A3830` and **transparent** `(0, 0, 0, 0)` = `FUN_1417A3730` (`14469DD70`, `14469DCF0`). The usual fade duration is the literal `0.5` s (`143D7EDB4`), the cutscene `40000020` uses `3.0` s (`0x40400000`).

## 2. Update and composition (STATIC)

- **Plate update** `FUN_140D0D510(plate, dt)`: if the plate is neither paused nor in bits 0/2 and `remaining > 0`: when `remaining <= dt`, current = target and remaining = 0; otherwise `current += (target - current) * f` with `f` from the easing evaluator `FUN_140330F50` (encrypted) applied to the plate's easing object, and `remaining -= dt`.
- **Composition** `FUN_140D0E910`: update every plate, then fold them in plate order with the colour `FUN_140D0D3F0` returns (the current colour, or zero when hidden): for each plate `c += (plate.rgb - c) * a`, `T *= (1 - a)`; the result is `alpha = clamp01(1 - T)` and `rgb = c / alpha` (straight, non-premultiplied) when `alpha > 0`, each component clamped to 0..1.
  Plate 0 is the bottom and plate 6 the top. Plate 6 is in the map too; it only contributes if its alpha is above 0 (INFERRED to stay 0 in practice: the game sets it to transparent in `FUN_14046EAE0`).
- **Per-frame call** `FUN_140D0FE60`: (1) system slot 1 (update and compose); (2) unless the gate `FUN_14075EAD0` is true, system slot 2 `FUN_140D0E790(system, list = [RendManImp+0x40], 0x780, 0x438)`; (3) plate 6: `FUN_140E004F0([RendManImp+0x10], red component)` after the hide flags apply. `FUN_140DFDC50` calls the scene update `E006A0` (the consumer of plate 6) before this call, so the tint lags by one frame.

## 3. Submission (STATIC / INFERRED)

`FUN_140D0E790` (when the composite alpha `system+0x2C > 0`): on the list's current layer set blend index 7 and clear the second state, pack the composite colour (`FUN_1402D94F0` to bytes, reordered to the layer's ARGB, `FUN_14030E510` writes it as four floats into the layer state `+0x20..0x2C`
and `+0x30..0x3C`), then record one rectangle with `FUN_14232FDF0` from the vector `(w/2, h/2, 0, 0)` = `(960, 540, 0, 0)` computed from the virtual canvas `0x780 x 0x438` (`(w+2) * 0.5 - 1`, constants `143D7EDB4` = 0.5 and `143D7F4C0` = (-1, -1, 0, 0)); that it covers the whole canvas depends on how the Ez rectangle call reads the vector (INFERRED).

- **Where it is replayed** (MENUS-MAP 33, OBSERVED order): `FUN_140DFD270` draws the 2D layers `+0x30`/`+0x38`, then Scaleform, then **layer `+0x40`** (the plates, unless `RendManImp+0x90`), then screen-capture hook 2 and the later layers, then the LDR -> back-buffer copy. The plates therefore cover the interface and come **after tone mapping and post** (INFERRED: the interface is in the 8-bit LDR target, MENUS-MAP 33; the plates are replayed after it, into the same stage). The layer `+0x40` is shared with menu rectangles (`FUN_140A3B720`/`A3B990`), so it is not plates-only. Screen-capture hook 1 sits between layer `+0x38` and Scaleform.
- **Shader and blend:** the GUI shader styles (`FUN_140D10940`, `GXGuiShader_Color` pixel programme) and layer blend index 7. `FUN_1400D21F0` builds three groups of five entries (bases 0, 5, 10); each group is {off (1,0), (4,5), (8,0), (4,1), (4,1 with op 2)} with the GX enum 0 ZERO, 1 ONE, 4 SRC_ALPHA, 5 INV_SRC_ALPHA, 8 DEST_COLOR (zero-based, MENUS-MAP 5814), and group 1 (entries 5 to 9) has write mask 7 (RGB only). By **direct indexing, entry 7 is (8, 0) = DEST_COLOR / ZERO, a multiply**, while `SRC_ALPHA / INV_SRC_ALPHA` is entry 6. A multiply cannot fade a picture to black with a straight alpha, so the layer value is probably shifted by one in the replay (`FUN_14237D5D0`, not read): **the mapping from the layer value to the table is OPEN** (row FL5). Treat "straight-alpha blend, RGB mask" as the intent, not as read.
- **Other readers of the composite:** the Scaleform page `FadeScreen` (slot 2 `FUN_140B54D80`) reads the composite (`FUN_140D0FC00`), packs it to ARGB and writes it to its window (`FUN_140E65190`): the loading-screen fade is mirrored into the movie (CPU/UI path, unaffected by the GX replay).

## 4. Who drives the plates (STATIC; `call-sites.txt`)

32 sites set a colour, 19 start a timed fade, 5 read a plate, 13 test "finished", 3 read the composite.

| Plate | Driver (function) | Role |
|---|---|---|
| 0 `Title` | `FUN_14075E360` (clears it only when `[144763258]+0x804` is non-zero and the cached flag `1447632B0` is 0; called by `TexStabilizeWait`, the cutscene exit and others), `FUN_1407641B0`, `FUN_140909F90`, `FUN_1406F0890`, `FUN_14071D000`; the option dialogs `FrpgMenuDlgPlayableTitle` (`FUN_1406E4DA0`, `1406E50A0`), `FrpgMenuDlgEnvOption` (`FUN_14071DF70`), `FrpgMenuDlgKeyOption` (`FUN_140721890`), `FrpgMenuDlgPcOption` (`FUN_1407273A0`) | title and options fades (the dialogs start and wait on it with `FUN_140D0FE20` / `FUN_140D0FCB0`) |
| 2 `InGame` | `MoveMapStep` states `BeginInit` `FUN_1408F15F0`, `TexStabilizeWait` `FUN_1408F24C0`, `MoveMap` `FUN_1408F2CA0`; `FUN_1408E9290`, `1408E9B20`; `FUN_1408F5D20` (starts a timed fade, tests "finished", and sets a plate whose index is a variable); `FUN_140499150`, `140499320`; `FUN_140D11530`, `140D116B0` | map change: set opaque black at the start, clear and fade in at the end |
| 3 `Remo`, 4 `InRemo` | the `SprjRemoMan` steps and the `FadeInOut` act (REMO-CUTSCENES-NATIVE) | cutscene enter and exit |
| 5 `Ending` | `FUN_1408E87D0` (only plate 5), `1408E9B20`, `1408F15F0`, `1408F24C0` | set to transparent at the map-move steps; the opaque use was not found in the sites read (OPEN) |
| 6 `InGameChroma` | `FUN_14046EAE0` (clear), `FUN_1408F36F0` (`MoveMapStep` Finish), `FUN_1408FAA20` | chroma tint; `1408FAA20` fades it to white `(1,1,1)` when a player state holds and sets the paused and hidden flags (`FUN_140D0FDF0/FDC0`) from the Remo "full cutscene" bit |

**Readers of "finished"** (`FUN_140D0FCB0`): the options and title dialogs (`FUN_14071E9E0`, `140721E50`, `140727CF0`, `1406E4DA0`, `1406F0890`, `14071D000`), the menu input gate (`FUN_140A5B930`: when `FUN_140B3D6C0()` holds, it tests that plate 2 is **finished**, `remaining <= 0`, not that its alpha is 0; MENUS-MAP section 6 says alpha; the observer `ObserveUiInputEligibility` hooks it, `game_bootstrap.cpp:9273`), `FUN_140BA0500`, `FUN_1408F5D20`, and **`SprjRemoMan::PrologueWait` `FUN_140CD9810`**.

### The map-move sequence (STATIC)

`MoveMapStep` (state table `14459F630`; states BeginInit `1408F15F0`, MsbLoad ..., TexStabilizeWait `1408F24C0`, MoveMap `1408F2CA0`, Finish `1408F36F0`):
BeginInit sets plate 2 to opaque black and plate 5 to transparent and calls `FUN_1407CF350(0, ...)` (INFERRED: the map audio fade); TexStabilizeWait does the same and also calls `FUN_14075E360` (which clears plate 0 under the condition above). In MoveMap, while the step timer `step+0x1E0` is above 0 and plate 3 is not fully opaque the timer runs down, plate 2 is held opaque black, and when it expires plate 2 is faded to transparent in `0.5` s.
MoveMap also writes `DAT_1447803D5 = 1` when plate 3's alpha reaches 1 (a cutscene black hold) and reads plate 3 and 2 alpha to decide `[144763F38]+0x11D`.

### The cutscene sequence and `PrologueWait` (STATIC)

`SprjRemoMan::RegistWait` (`FUN_140CD9950`): unless the cutscene id is `31009010`, plate 3 is cleared instantly and then faded to opaque black in 0.5 s (`FUN_140D0FE20`), or set straight to opaque black when `FUN_140CD58F0` is true (a request-side flag, not decoded). `PrologueWait` (`FUN_140CD9810`) polls `FUN_140D0FCB0(fade, 3)` and starts the Ins when the remaining time reaches 0, so the cutscene begins under a black plate. In `Man::Update`, plate 3 is kept opaque black until the "playing" bit (bit 1) is set, then cleared each update; when the finish bit `0x100` is set (skip or end) it is faded back to opaque black in proportion to the missing alpha, and once it is opaque the Ins is marked finished (`FUN_140CD5A60`);
`FadeInOut` acts drive plate 4; `Epilog` clears plate 4 and sets plate 3 to opaque black unless `Man+0x154` bit 7 is set; `EpilogWait` waits for map streaming (`FUN_140606AA0`) and for the elapsed time (`Man+0x14C` reaching `Man+0x148`), then fades plate 3 to **transparent** (3.0 s for `40000020`, 0.5 s otherwise; cutscene `31009010` skips this fade; the ids `37000030`, `39000010`, `39000030`, `35000050` additionally call `FUN_14075E360`); `Finish` clears plates 3 and 4.
**So `PrologueWait` depends on the plate's remaining time only; it advances as soon as the CPU update reduces it to 0.**

## 5. What is missing in the native build

| # | Piece | Status | Needed |
|---|---|---|---|
| F1 | Update chain (`D54F80` -> `DFDC50` -> `D0FE60`) | not hooked; runs genuinely (INFERRED: no hook in the table for these addresses) | keep it unhooked; confirm it runs every frame (row FL1) |
| F2 | **Draw of the composite quad** (design: [ENDING-CREDITS-NATIVE.md](ENDING-CREDITS-NATIVE.md) section 7: root-constant triangle recorded at the end of the UI category, ER method) | **answered 2026-10-18: no quad to draw, as the original.** Probe `fade-observe.flag` (plates and composite every 16 ms) at a Load Game into the world: Reference2066 and Native2067 give the same sequence (plate 2 opaque black during the load, the composite equal to it, then the 0.5 s fade to 0; the other plates 0): the update chain F1 runs natively. Native2068 (the row now logged before the gate): the gate is **true** for every frame with an alpha above 0, so `D0FE60` records no rectangle and the native overlay draws nothing, as it should. The gate's loaded code (Native2068, `loaded-gate.json`, read-only) is byte for byte the same body as `FUN_14075EB60`, the cached NewMenuSystem flag: a once-computed byte `[1447632B0]` (init bit `[1447632B4]`) from `FUN_14080EF40`. So with the new menu system the plate quad is the old menu system's path (`FrpgMenuDlgFadePlate`, MENUS-MAP 278), and the visible fade is the Scaleform fade window (F7), which runs in both modes (the same movie set, `01_900_Black` twice, Reference2066 / Native2068). The gate is also true in the original: Reference2073 (`Run-LanguageWitness.ps1 -ReadFadeGate`, `fade-gate.json`, read-only) reads `[1447632B0] = 1` with its init bit `[1447632B4] = 1` in the world. **The overlay was removed at the workstream's review** (2026-10-18): with the gate a constant of the PC build's configuration it
never drew, and its pipeline (built lazily inside frame recording) had never run; `NativeFadeComposite`, `EncodeFadeOverlay`, the job's
`fade` and plan-key bit 8 are gone. If a build ever reported the gate false, the old-system plate quad would be the work item: one
full-target straight-alpha triangle after the UI, its pipeline built at initialisation and covered by a headless test. The probe
`fade-observe.flag` stays. Earlier: in Native2065 (title, load, the whole credits chain) the composite read natively stayed 0 (one `native_fade_overlay` row, rgba 0, gate false), so no overlay was drawn and the draw is unproven; whether the original composes a fade there is unknown: the probe `fade-observe.flag` (plates 0..6 and the composite every 16 ms, rows FL1/FL2) is built, not yet run in either mode (`NativeFadeComposite` reads the composite and the gate on the engine thread into `NativeFrameJob::fade`, quantised to 1/255; `SceneRenderer::EncodeFadeOverlay` draws one root-constant triangle, straight alpha, after the UI and before the gamma pass in both live recorders; plan-key bit 8; row `native_fade_overlay` per alpha change); before: the replay `DFD270` is bypassed by `NativeRenderStep` | one full-target quad after the interface and before the back-buffer copy; colour = composite (straight RGBA), `SRC_ALPHA / INV_SRC_ALPHA`, no depth, skipped when alpha is 0 or the gate `FUN_14075EAD0` is true |
| F3 | Source of the colour | reachable without the Ez list: `[[144791A38]+8]+0x20..0x2C` (floats) after the update | read it in the native frame after the CPU update; or record from the list (needs the Ez replay) |
| F4 | Gate `FUN_14075EAD0` | **read 2026-10-18** (loaded code, Native2068): the cached NewMenuSystem flag, the same body as `FUN_14075EB60` (byte `[1447632B0]`, computed once by `FUN_14080EF40`); true natively during every fade, and 1 in the original (Reference2073); encrypted at rest; callable | call the original at the draw (it returns a byte), or read its source flag once with the loaded-code route (row FL6) |
| F5 | Ez list growth | `D0E790` appends one rectangle per frame while the composite alpha is above 0; the list is reset by the replay path (`FUN_140DFD270`/`FUN_1423306F0`), not shown to run natively | confirm the 2D list is reset or bounded without the replay (row FL4); an unbounded list would be a latent fault during a long fade |
| F6 | Plate 6 `InGameChroma` | the CPU step is reproduced: `UpdateNativeSceneCpu` reads `stage+0x4F8/0x4FC`, clamps to 1, takes the tint from `0x39F2490` and writes `context+0x3928..0x3950` (`game_bootstrap.cpp:5430-5441`); **no native code reads `context+0x3928..`** | a native consumer of that block in the colour-grading/post pass (post workstream, I-0068) |
| F7 | `FadeScreen` Scaleform page | UI path, reads the composite | none; check the loading screen colour (FL3) |
| F8 | Audio fade of map changes (`FUN_1407CF350`) | CPU/FMOD | none |
| F9 | Capture hooks around layer `+0x40` | screen-capture hook 2 comes after the plates in the original | none for gameplay; note for screenshots |

## 6. Witnesses and live rows (read-only)

**Witnesses.** FW1 title -> New Game / Load Game into the world (plates 0 and 2; map-move sequence of section 4); FW2 a fog gate or bonfire warp (plate 2 black then 0.5 s fade-in); FW3 a boss cutscene start and end (plates 3 and 4, from REMO-CUTSCENES-NATIVE RW1/RW2); FW4 the options dialog opening (plate 0); FW5 the loading screen (`FadeScreen` mirror).

| Row | Action | Expected |
|---|---|---|
| FL1 | Log, per frame, `plate.current/target/remaining` for plates 0 to 6 and the composite `system+0x20..0x2C` during FW1 and FW2 | in the native run the values evolve exactly as in the Reference (the update chain runs) |
| FL2 | Same run: log the composite alpha and note whether any pixel of the native frame changes with it | alpha reaches 1 during the black hold; today the picture does not change (F2) |
| FL3 | FW5: compare the `FadeScreen` page colour with the composite (`FUN_140B54D80`) | equal |
| FL4 | Log the size of the Ez list `[RendManImp+0x40]` once per second over a 3 s fade | bounded (reset every frame) or growing (F5) |
| FL5 | Reference: log the blend description bound for layer `+0x40` during a fade (index 7 of `FUN_1400D21F0`) | the blend actually bound (direct indexing of entry 7 would be DEST_COLOR / ZERO; intent is straight alpha, to be confirmed) |
| FL6 | Read-only loaded-code route (`Read-LoadedCode.ps1`) for RVAs `0x75EAD0` and `0x330F50` once | the gate's source flag and the easing curve decoded |
| FL7 | FW3: `PrologueWait` release time (frames from `RegistWait` to the Ins start) natively vs the Reference | 0.5 s plus load, no stall |

## 7. Open points

1. `FUN_14075EAD0` (the draw gate) and `FUN_140330F50` (the easing curve): encrypted at rest, call sites read only. `FUN_14075EAD0` is also called by `FUN_140B3EA10` (a MenuMan window builder) and sits next to `FUN_14075EB60`, the cached NewMenuSystem flag (MENUS-MAP 4), so it is probably a sibling cached configuration flag (INFERRED). **Answered for the gate (2026-10-18, F4):** it is the same cached NewMenuSystem flag (identical loaded body, the same byte). The easing curve stays open.
2. The layer-value-to-table mapping of blend index 7 and the exact Ez rectangle parameters (section 3 gives the structure, not the final vertices).
3. The opaque use of plate 5 `Ending`, and the plate flags in bits 2 and 3.
4. Whether plate 6's alpha is ever above 0 (then it would also tint the composite): `FUN_1408FAA20` fades it to `(1, 1, 1, 0)`, so the alpha stays 0 on that path.

## 8. Exports

`decompiled/DS3-1152-F-` + `00D0F6E0`, `00D0F9D0`, `00D0FE60`, `00D0FD40`, `00D0FE20`, `00D0FB70`, `00D0FCB0`, `00D0FC00`, `00D0FC20`, `00D0FDC0`, `00D0FDF0`, `00D0E430`, `00D0E4B0`, `00D0E550`, `00D0E610`, `00D0E910`, `00D0E790`, `00D0E8F0`, `00D0D110`, `00D0D3C0`, `00D0D2C0`, `00D0D510`, `00D0D3F0`, `00D0D440`, `00D0D490`, `00D103F0`, `00D10510`,
`017A3730`, `017A3830`, `0030E510`, `0232FDF0`, `00E004F0`, `00E006A0`, `000D21F0`, the drivers `0046EAE0`, `0075E360`, `00499150`, `00499320`, `006E4DA0`, `006E50A0`, `006F0890`, `0071D000`, `0071DF70`, `00721890`, `007273A0`, `008E87D0`, `008E9290`, `008E9B20`, `008F15F0`, `008F24C0`, `008F36F0`, `008F5D20`, `008FAA20`, `00909F90`, `00BA0500`, `0071E9E0`, `00721E50`, `00727CF0`, `00A5B930`,
`00767AC0`, `00CCE9B0`, `00D11530`, `00D116B0`, `00B54D80`, `00B3EA10`, `00DFDC50`, `00D54F80`, `00D55190`.
