# The ending credits, a save-free test route, and the native fades in D3D12 (I-0069, Task 14)

STATIC (Ghidra mirror of `DarkSoulsIII.exe` 1.15.2.0, EMEVD and movie headers read offline, and `native/` as of this branch). Nothing was launched.
Status words: **STATIC** read in code or data, **INFERRED** deduced, **OPEN** not known statically.
Baseline: [BINK-MOVIES-NATIVE.md](BINK-MOVIES-NATIVE.md) (the movie path), [FADE-PLATES-NATIVE.md](FADE-PLATES-NATIVE.md) (the plates; this note completes its F2/F3/F4/F5 with the D3D12 design),
[REMO-CUTSCENES-NATIVE.md](REMO-CUTSCENES-NATIVE.md), I-0067 MENUS-MAP section 35. ER method: `evidence/I-0066/ER-ENGINE-D3D12-METHOD-710.md` and `ER-ENGINE-D3D12-DEEP-STUDY-716.md` (read-only reference; D-0157).
Evidence outside Git: `evidence/I-0069/queries/credits/` (references, callers and LEA uses read, the movie-id table, the EMEVD census, the ending events, the functor vtables, the language stub).
A first version of this note said the start of the credits was not found and named the wrong objects; an independent review corrected both (the review is recorded in `RESULT.md`). Reference queries with `w:` miss `LEA` operands; the `o:` queries in `lea-references-o-query.txt` do not.

## 0. Verdict

1. **The credits are a Bink movie, nothing else.** The original starts them from one place: state 0 of the FD4 step `EndingStep` calls `FUN_140BEA570`, which requests a movie from `CSMovieImp` and opens a menu job (the movie page). The movie requested is INFERRED to be `movie/fdp_endroll_ww.bik`
   (647,246,828 bytes, 13,002 frames at 30000/1001 fps = **433.8 s**), requested as movie id **3** (the table and the call are STATIC; that id 3 is the WW end roll and that the language index is 1 are INFERRED, section 2). No credits text, scroll data or scene was found in the code read; INFERRED: the roll is in the video.
2. **The way in (STATIC, section 1):** the story ending events set one of the **event flags 20, 21, 22, 23** (the four ending cutscene sets of map `m41_00`); the world step `FUN_1408F5D20` calls `FUN_1408F5930` every update, which reads those flags, clears them, stores the ending kind (1 or 2) and sets the request byte `[14475AC00+0xAE8]`;
   the title-flow step then constructs `EndingStep` in its state 6 (`FUN_14090ADB0` -> `FUN_140F0D160`), and `EndingStep` state 0 starts the movie. (Event flag **30** sets the sibling request byte `+0xAE9`, which leads to state 8 of the same step: not read.)
3. **`EndingStep` has five states** (table `14489F210`). Only state 0 touches the movie. States 2 and 3 build and pump a post-credits menu pipeline (texts through `FUN_140A67390`, `FUN_140A649F0` and `FUN_140A67060`, with the system-message item `0x1061`). **No fade plate call site exists in the credits code read** (the plate census of FADE-PLATES-NATIVE has no site in
   `140BE...`, `140EA...`, `140F0D...`/`140F0E...`; callees reached transitively, such as the menu-manager fan-out of `FUN_140765640`, were not checked: INFERRED). The `1427457D8` object passed to the job pumps is `FD4::FD4Time` (RTTI), a time value, not a fade.
4. **Save-free test routes (section 5):** R1 calls the original `FUN_140BEA570` once on the game's update thread (read-only, INFERRED executable, never run, with a mandatory guard: the call crashes when the menu-job owner or the new-menu flag is missing). R5 sets event flag 20 in memory from a loaded world, which exercises the **whole** real chain (the exit to the title flow, the credits, the return); it is not read-only by itself
   (leaving the world may save), so it needs the D-0116 hash-checked restore.
5. **Native today:** since the native Bink player landed (`6d857eef`, 2026-10-11, after BINK-MOVIES-NATIVE was written) and was brought to full pace (Native1252: the opening in 197.875 s, 4749 frames drawn, 0 skipped), the credits movie is decoded and drawn into the movie proxy like the opening. What is missing for a faithful sequence is the fade quad (section 7).
6. **The D3D12 fade design (section 7):** read the composite colour of the genuine CPU fade system, record **one full-target triangle with 16 bytes of root constants** at the end of the UI category of the native frame (after the Scaleform primitives, before the read-back and the transition to present), in the two recorders the live frame uses, with a pipeline created at open for the swap-chain format.
   ER gives the method (one backend owns recording and presentation; ordered records; recording owns used data until completion), not a fade: the ER documents contain none.

## 1. How the original reaches the credits (STATIC unless marked)

| Step | Code | Content |
|---|---|---|
| Ending events | EMEVD map `m41_00_00_00`, events `14100100` and `14100101` (`emevd-m41_00-events-...txt`) | after a dialogue-triggered start (`2007[10] 10012020/10012021`, INFERRED from the family) each plays Remo cutscenes (`41000060` then `41000070`; `41000000`, then `41000050` and `41000051`) and then sets `2003[2]` flags **22**, **23** (event `14100100`) and **20**, **21** (event `14100101`), together with 110, 6400 and 9920 to 9923. `m40_00_00_00` event `14005480` sets flag **30**. Which ending each cutscene set is, and the map's player-facing name, are not stated in this repo's notes |
| Reading the flags | `FUN_1408F5930` (called every update by the world step `FUN_1408F5D20` at `1408F5E04`) | reads event flags 20, 21, 22, 23 and 30 through `FUN_1404C5350(_DAT_144752F68, id)` (INFERRED event-flag reader: the setter `FUN_1404C5E20` splits the id with `% 1000` and bit tests). If any of 20 to 23 is set, `[param+0x1ED]` is zero and `FUN_14078C480(_DAT_144763F38)` is false: `FUN_140628090` sets `[14475AC00+0xAE8] = 1`, `[1447572B8+0x7C]` becomes 1 (flags 20, 22) or 2 (flags 21, 23), `FUN_140628960(0x3D1486)`, `FUN_140628BE0(1)`, `FUN_140628670(1)`, `FUN_140765E30(4000)`, `FUN_140628020`. Flag 30 sets `+0xAE9` through `FUN_1406280A0`. Then the five flags are cleared |
| The title-flow step | state table built by `FUN_140909AC0` (11 entries: `FUN_14090B170`, `A820`, `AA40`, `AA10`, `B6F0`, `B000`, `ADB0`, `AEB0`, `B650`, `B570`, `AFB0`); INFERRED to be the step that owns the title flow (it also sets plate 0 in `FUN_140909F90`) | state 5 `FUN_14090B000` waits for its child (`[[step+0x90]+0x20]+0x30 == 0`), then: `FUN_1406273A0()` (`+0xAE8`) set -> `FUN_140625C00` clears it and the state becomes **6**; `FUN_1406273B0()` (`+0xAE9`) -> 8; otherwise 1 |
| State 6 | `FUN_14090ADB0` | allocates 0x90 bytes, constructs **`EndingStep`** with `FUN_140F0D160` (vtable `1429B9D58` then `1429B9D88`, `+0x20 = 14489F210`, `+0x80 = 5`), wraps it with `FUN_140908120(task, 7, step)`, stores it at `step+0x80`, state 7 |
| State 7 | `FUN_14090AEB0` | waits for the child state `[child+0x10] == 3` (done), then reads `DAT_14489F200` (section 2) and calls `FUN_14090C510` or `FUN_14090C570`, which set the state to 4 |
| `EndingStep` | `Step<EndingStep>`; its own vtable `1429B9D88` (RTTI `EndingStep`), `Step<EndingStep>` at `1429B9D58`; the singleton cell `_DAT_14489F278` points to `14489F2F0`, whose vtable `1429B9E88` is the runtime-class descriptor `DLRF::DLRuntimeClassImpl<EndingStep>` (reviewed), not the step. `FUN_140F0D100` is a kind-of test (caller `FUN_140F0E5E0`), `FUN_142695F20` a static initialiser, `FUN_14178C990` the generic runtime-class registration (136 callers) | state at `step+0x10`, wait counter at `step+0x80` (5 frames at start); the state table of 5 entries of 16 bytes at `14489F210` is built by `FUN_142695E40` |
| State 0 | `FUN_140F0E3A0` | writes `DAT_14489F200 = 0`; while `[step+0x80] > 0` decrements it and returns; then calls **`FUN_140BEA570(out32)`** and goes to state 1 |
| Request | `FUN_140BEA570` | checks `_DAT_14489C308` (`CSMovieImp`), calls **`FUN_140EACC20(CSMovieImp)`**, then builds a callable of vtable `142916B18` and hands it to `FUN_140BF1520` (INFERRED name: the menu-job creator) with the 32-byte out buffer. Its only code caller is state 0 |
| Movie request | `FUN_140EACC20` | `FUN_140EACA10(imp, FUN_140EA5500(), volume (143D7EE10), 0, 1, FUN_141A2B6E0() < 3)`; `FUN_140EA5500` returns `[144658708 + idx*4]` with `idx = _DAT_14479BA04` (above 3 it makes an assertion call and still indexes the table) |
| Movie open | `FUN_140EACA10` -> `FUN_140EAC950` -> `CSMovieIns` (`FUN_140EAF570`) | formats `movie:/%s.bik` (`1429A45F8`); the name is resolved from the `movtae` table by a key lookup (BINK-MOVIES-NATIVE section 1); see there for the steps |
| State 1 | `FUN_140F0E3E0` | waits until `FUN_140BEA4A0()` is false (`CSMovieImp+0x48 == 0`: no movie instance), then state 2; `DAT_14489D5B8 != 0` jumps to state 4 |
| State 2 | `FUN_140F0DAD0` | builds the post-credits pipeline of menu jobs: dialogue texts 450001 and 450000 of the category `FDP_Dialogues` (`1428B62E8`) through `FUN_140A67390` / `FUN_140A649F0`, the system-message item `0x1061` through `FUN_140A67060`, a menu-manager byte through `FUN_1407642A0`; the block `{100, 2, 0x29}` with `1428B8150` is a per-dialogue parameter block passed to `FUN_140BBC170`, not a wait |
| State 3 | `FUN_140F0DA00` | pumps the job list with the frame time (`FUN_140A3E180`, `FUN_140BB1AA0`: generic job-list tickers), `FUN_140765640(1)` (menu-manager flag `+0x2E8 = 0` and the visibility fan-out), `FUN_140765220(0, 1)` (menu-manager state 0 = 1); `DAT_14489D5B8 != 0` -> state 4 |
| State 4 | `FUN_140F0D9F0` | finish (sets the state to -1) |

Opening, for comparison: New Game (`FUN_140AE9070`) calls `FUN_140BEA6B0` -> `FUN_140EACC80` (movie id **1**) and builds the callable of vtable `142916AD8`. The two callables have different virtual slots (`functor-vtables-credits-vs-opening.txt`);
which page each opens was not read (the callable's invoke body is behind the job creator); INFERRED: both open the `MovieScreen` page, whose update `FUN_140B54F90` (below) is shared. `FUN_140EACBD0` (movie id 0) has no code caller in the image.

**The page and the end of the movie.** `FUN_140B54F90` (the movie page's update): `FUN_140B60850(input, page+0x9D9)` is the skip test (the key: command `0x54`, or the five confirm checks when `+0x9D9` is set, I-0068 `BINK-NATIVE-READY.md`); when it is true and the byte `page+0x9D8` is zero, `FUN_140EACCD0` -> `FUN_140EAF630` skips the current movie instance. When `FUN_140EAC940(CSMovieImp)` says no movie
is playing, the page closes itself (`vt+0x50` with state 2). So the credits end by playback end or by skip; skip is allowed unless the page's `+0x9D8` byte is set (what sets it: OPEN).

## 2. The movie id, and the value after the credits

**Movie id.** `FUN_140EA5500` indexes the table `{2, 3, 3, 3}` (`144658708`) with `_DAT_14479BA04`, stored by `FUN_140EA5860` from `FUN_140ED00C0(manager, FUN_140EA6130())`. `FUN_140EA6130` returns the constant 1; `FUN_140ED00C0` is a one-instruction stub at rest (`RET 0xB705`, INFERRED protected code, body unreadable), so the index is not known statically: with 1 the table gives 3.
The movtae strings are `FDP_Opening`, `FDP_EndRoll_JP`, `FDP_EndRoll_WW` in that order, and New Game requests 1, the credits 2 or 3, which is consistent with 1 = opening, 2 = JP end roll, 3 = WW end roll; but `FUN_140EACA10` resolves the name through a key lookup of the TAE (not read), so **the id-to-file mapping is INFERRED**. Only the WW file is installed.

**After the credits.** `DAT_14489F200` is read by `FUN_14090AEB0`: zero selects `FUN_14090C510` (state 4, two menu-manager values), non-zero selects `FUN_14090C570` (it increments a counter at `_DAT_1447572B8+0x78`, clamped to 0..9999, calls several menu-manager and player-state functions, then state 4; INFERRED: a play-through cycle counter).
`EndingStep` state 0 writes 0 (`MOV byte ptr [14489F200], 0` at `140F0E3A6`), and no other writer was found, so **C510 is the default path after the credits and C570 needs a write the reference census did not show** (wider store, or a pointer into the same block): OPEN. Both branches run after an ending; "after-ending" is not a property of C570. Row CR5 settles it.

## 3. What the trigger search covered (STATIC)

- The census with `w:` misses `LEA` operands (reviewer finding); the `o:` queries show the real uses: the object `14489F2F0` is loaded by `FUN_140F0D4F0` (`140F0D512`), the vtable `1429B9D88` by `FUN_140F0D160` (constructor) and `FUN_140F0D210`, and the state function table of the title-flow step by `FUN_140909AC0`. The table `14458B000` (MenuMan request names `Movie_StaffRoll`, `Movie_EndingBad`, `Movie_EndingGood`, `Movie_OldTale`, ...) has no `LEA` or data use either (INFERRED a debug or script name table).
- `144BE3B50` is a `.pdata` table of `RUNTIME_FUNCTION` triples (checked), as are `1449EE4CC` and `144A21F1C`: the "DATA callers" of these functions are unwind entries, not tables.
- The flags 450000 and 450001 are dialogue text ids, not event flags.
- Not searched: the talk scripts (ESD) that may start the EMEVD events of `m41_00` (the dialogue start is INFERRED from `2007[10]`), and the callers of the setters of event flags 20 to 23 other than EMEVD (the setter `FUN_1404C5E20` has 45 call sites, only `FUN_1408F5930` uses these ids; the EMEVD interpreter reaches it with a variable id, INFERRED).

## 4. Can the credits be reached without an end-of-game save? (answer to the brief)

- **By event flag:** yes in principle: event flag 20 (or 21, 22, 23) set in memory in a loaded world triggers the genuine chain (R5). It is not a "read-only" action in the strict sense because the world is left and the game may write the character (OPEN), so it must run under the D-0116 hash-checked restore. Flag 30 leads elsewhere (state 8, not read).
- **By map:** the story events are in map `m41_00` and need the ending conditions; reaching them legitimately needs end-game progress; no map shortcut was found.
- **By calling the request itself:** R1, which touches no flag and no save.

## 5. Test routes (live ones under the D-0116 protocol: announced in chat, offline, hash-checked restore)

| Route | What it does | Needs | Status |
|---|---|---|---|
| **R1 (read-only)** | A witness file `witness-credits.txt` next to the module (as `witness-teleport.txt`, `game_bootstrap.cpp:1825-1853`, read on the game thread, removed once read) makes the module call the original `FUN_140BEA570(out32)` once and log `native_credits_request`. **Guard (reviewed on the disassembly):** `FUN_140BF1520` has no safe fallback: when `FUN_14075EB60()` is false or `FUN_140B3CDA0()` is null it calls the empty callable `[R8+0x18]` (`BEA570` passes a callable whose slot is zero, `140BEA5E2`), i.e. `FUN_141FCC680` (a bad-function-call throw) and `INT3` at `140BF1624`. The witness must test both first and refuse otherwise | `CSMovieImp`, the menu-job owner, the new-menu flag; the game update thread (INFERRED); the existing hooks on it are `ObserveUiInputEligibility` (`{0xA5B930}`, title) and `ObserveChrManagerSchedule` (`{0x8DD2E0}`, world) | INFERRED executable; never run |
| **R5 (whole chain)** | A witness file makes the module set event flag 20 through the game's own setter `FUN_1404C5E20(_DAT_144752F68, 20, 1)` (INFERRED signature from the call sites) in a loaded world; `FUN_1408F5930` then runs the real chain of section 1 and the credits play, followed by the return | any ordinary save, the Load Game pilot, saves restored by hash | **built 2026-10-18**: `witness-flag.txt` ("20 1") read on the game thread (`ApplyWitnessFlag`, WorldChrMan schedule, both modes), setter signature read (`1404C5E20(manager, id, value, 0)`); pilot `Run-AlphaCast.ps1 -SetFlag "20 1" -Casts 0`; Reference2064: `witness_flag` applied, the end roll title on screen 10 s later; **Native2065 (2026-10-18): the same chain natively**, the credits pages identical to Reference2064 at 10, 30, 60, 120, 240, 420 s and the "Begin journey 2 in current state?" dialog at 480 and 540 s as in the original (exit 0, faults 0, restoration by hash ok); the files the game writes and C510/C570 (CR5) remain OPEN |
| R2 | R1 from a world loaded from an existing save | R1 plus the Load Game pilot | as R1 |
| R3 | Movie without the page: `FUN_140EACC20(CSMovieImp)` alone | none | decode, pacing and audio witness only; no picture without the page that samples the proxy |
| R4 | Reference observation of a **real** ending: pass-through observers on `FUN_14090ADB0` (title-flow state 6), `FUN_140F0E3A0` and `FUN_140BEA570` (return address), the plate composite | a real ending; live | confirms the flag chain and settles `DAT_14489F200` |

Practical notes: the roll lasts 433.8 s unless skipped, so the pilot's default 420 s run is too short (use at least 520 s, or press the skip key and check the close); the language group must select movie id 3 (row CR4) because the JP file is absent from this install; while the roll plays, the title or world below it keeps running.

## 6. What the native build does with this today

| Piece | State |
|---|---|
| Movie decode, audio and draw | the native Bink player (`NativeMovieStepInit` / `NativeMovieStepPlay`, `RenderNativeMovieFrame`, `RenderNativeMovie`) draws the movie into the proxy's native target before the UI pass samples it (`game_bootstrap.cpp:6505-6541`, `6779-6838`, `626`); the credits use the same `CSMovieIns` path as the opening. The Bink note's verdict item 4 ("omits the player") is out of date since `6d857eef` (updated there) |
| The page (`MovieScreen`) | genuine CPU menu code, drawn by the native UI pass like any page |
| Pace | the opening played at full pace in Native1252 (197.875 s, 4749 frames drawn, 0 skipped; I-0066 `BINK-NATIVE.md`); the credits movie is 30 fps and 13,002 frames, larger: pace and memory over 7 minutes are OPEN (row CR3) |
| World exit and return | genuine CPU step code (`FUN_1408F5930`, the title-flow steps); native behaviour of leaving a world to the title and of the post-credits pipeline is untested (row CR5/R5); the native "Quit in game" work (1257/1260) is the nearest precedent |
| Fades around the credits | none drawn natively (FADE-PLATES-NATIVE F2); no plate call site is in the credits code itself (INFERRED not driven from there), so the fades seen at the start and the end come from the ending cutscenes (Remo plates 3/4) and the title/load transitions |

## 7. Native fades in D3D12 (complement to FADE-PLATES-NATIVE F2, F3, F4, F5)

### 7.1 What the ER method gives, and what it does not

D-0157 (Thomas): engine architecture may follow the source-checked ER engine-to-D3D12 connection with DS3 data. `ER-ENGINE-D3D12-METHOD-710.md` and `ER-ENGINE-D3D12-DEEP-STUDY-716.md` section 6 give the method: the current engine owner produces the CPU data; one backend turns it into native
records; selection consumes the same current associations; records are processed in order, accumulated draw lists flushed before callback records (`ER-271-F-01AECB00`); the recorder owns what it uses until the submission completes ("do not overwrite transient upload or descriptor bytes still used by the GPU");
presentation belongs to the backend and follows the ordered records; retirement must follow an explicit submission token (the ER delayed manager is frame-count based and **is not proven GPU-safe**; do not copy "wait two frames").
**ER contains no fade**: a search of the `ER Decompilation` investigations, knowledge and decisions folders for "fade" and "full-screen" found nothing, and no ER structure or value is used here (ER is never DS3 evidence).

| ER principle | Applied to the fade |
|---|---|
| The genuine engine owner produces, the backend consumes | `SprFD4FadeSystem` (CPU, unhooked) keeps composing the seven plates; the backend reads **its** composite. No second fade model, no re-implementation of the easing (`140330F50` is encrypted anyway) |
| Pipeline prepared from the current state, descriptors from the current tables | one pipeline whose only key is the target format (the swap-chain format `R8G8B8A8_UNORM`, `backend.cpp:393`), created **at renderer open** (the composition pipelines are created lazily, `scene_pass.cpp:352-353`, `808-811`, and `RenderStartupFrame`, which draws the title fades, can run before any model frame) |
| Ordered records, callback categories flushed after the draw lists | the fade is the **last record of the UI category** of the frame |
| The recorder owns used data until completion | the colour travels as **root constants copied into the command list at record time**: no upload buffer, no descriptor, no per-frame resource, nothing to keep alive or retire |
| One backend owns presentation | the recorders that call `Present` share one helper; no second present, no captured intermediate |
| Explicit submission token | nothing to attach: no resource is created per frame; the existing `gpu_.Execute` / `Complete` / `Drain` order is unchanged |

### 7.2 The native frame at the time of reading (STATIC)

`SceneRenderer::RenderNativeModels` (`scene_pass.cpp:1545`) composes the lit HDR into the **swap-chain buffer** (sRGB-encoded by hand into a UNORM buffer, `composition_pso_`, lines 1614-1625; no MSAA), then draws the Scaleform primitives into that same buffer with `primitives_.Encode` (line 1629),
optionally copies the buffer to the read-back (every 120th frame, lines 1631-1640), executes, waits (`gpu_.Complete`, 1644), presents (1664) and drains. `RenderStartupFrame` (1672) does the same without a scene (clear, primitives at 1687, present at 1694); these two are what `Clear()` reaches (`RenderNativeModels`, `RenderStartupPrimitives` -> startup frame, `RenderStartup`) and the only live `Present` calls (reviewed).
`RenderFrame` (1858) is a pass-list recorder (`PassKind::Ui` with the primitives at 2049, `PassKind::Readback`, present at 2114; with `display_enabled_` the UI goes to an intermediate target and the swap-chain buffer is written by the final display), reached only from `native_service` and `game_frame_host`, which are not in the live module. The buffer is in the render-target state when the primitives end. `Clear()` (`game_bootstrap.cpp:617`) publishes the Bink frame before the call, and `NativeRenderStep` (7236) prepares the UI CPU frame.
The primitive pass blends colour SRC_ALPHA / INV_SRC_ALPHA and alpha ONE / INV_SRC_ALPHA (`primitive_pass.cpp:159-164`). There is no fade quad in any recorder.

### 7.3 Design

**D1. Data (CPU to renderer).** In `NativeRenderStep`, after the genuine CPU update, read the composite through the original accessor `FUN_140D0FC00(fade, out*)` (FADE-PLATES-NATIVE F3; or the floats at `[[144791A38]+8]+0x20..0x2C`) and the gate through the original `FUN_14075EAD0()` (F4; a byte; the body is encrypted at rest but callable). Publish `{rgba, gate_open, serial}` next to `native_ui_frame`, stamped with the same serial; the serial check lives in `Clear()` (`game_bootstrap.cpp:633`,
`"native UI current presentation serial"`) and the renderer's own input check, and the fade stamp gets the same two. That the plate update `FUN_140D54F80` runs before `NativeRenderStep` in the same frame is not shown (at worst a one-frame lag: row FD2 measures it). A non-finite component is a contract violation: refuse, as the codebase does for other CPU inputs. Clamp each component to 0..1.

**D2. Pipeline (once, at renderer open).** A root signature with **four 32-bit root constants** (PS-visible) and no table; the vertex stage is the full-screen triangle of the existing composition shaders (`SV_VertexID`, three vertices); the pixel stage returns the constant colour; blend enabled, colour `SRC_ALPHA / INV_SRC_ALPHA / ADD`, alpha `ONE / INV_SRC_ALPHA` as in the primitive pass (the read-back hash includes the alpha byte, so the alpha factors matter to the rows); no depth, no stencil, no culling, one render target of the swap-chain format.
The existing `Blend()` helper (`scene_pass.cpp:194-205`) is opaque (ONE / ZERO, blending off): the pipeline must set its own blend state, not reuse it unchanged. The composite is **straight** RGBA (FADE-PLATES-NATIVE section 2: `rgb = c / alpha`), which is what these colour factors expect.

**D3. Recording (the helper).** `EncodeFadeOverlay(list, target, viewport, scissor, colour)`: return at once when the gate is closed or `alpha <= 0` (the original appends its rectangle only when the composite alpha is above 0); otherwise set the pipeline and the root signature, `SetGraphicsRoot32BitConstants(0, 4, colour, 0)`, bind the swap-chain RTV with no depth view,
set the viewport and the scissor to the whole target, topology triangle list, `DrawInstanced(3, 1, 0, 0)`. **No state transition is needed**: the buffer is already a render target. Insertion points: in `RenderNativeModels` after the `if(primitive){...}` block and before the read-back (so the read-back and its hash show the fade);
in `RenderStartupFrame` after the primitives and before the transition to present; in `RenderFrame`, if it is ever used, as a pass after the last pass that writes the swap-chain buffer (after the final display when `display_enabled_`), before `PassKind::Readback`. The helper binds the target explicitly in every case.

**D4. Colour and rounding.** The buffer holds sRGB-encoded values (the composition applies the encoding) and the original blends the plates in its 8-bit LDR target, so the fade colour is used **as is, without any gamma conversion**. The original packs the composite to bytes (`FUN_1402D94F0`) before drawing; quantising the four floats to 1/255 steps in the constants reproduces that, the rounding mode being INFERRED (row FD1 measures it).

**D5. Blend index 7.** The layer blend index the original uses for the plates is 7; by direct indexing of the three-by-five table of `FUN_1400D21F0` it would be DEST_COLOR/ZERO, which cannot produce a fade of colour alpha, so the intent is straight alpha (FADE-PLATES-NATIVE row FL5 keeps the open point). The helper keeps one switch (straight alpha by default) so that the Reference measurement FL5 can settle it without touching the pipeline layout.

**D6. What is not done.** (a) The 2D (Ez) list `[RendManImp+0x40]` is not replayed: it is a command stream of the old renderer and its growth without the replay is the F5 risk. If a native replay of that list is added later, the fade quad it contains replaces this helper; the helper then refuses to draw (a single owner per quad). (b) Plate 6
(`InGameChroma`) stays a scalar for the scene consumer (F6). (c) The `FadeScreen` page and the audio fades need nothing (F7, F8). (d) No second render target, no read of the previous frame, no copy: the fade never sampled anything in the original either.

**D7. Cost and failure.** One draw, three vertices, sixteen bytes of constants, zero when the composite is transparent; the frame already waits for the GPU each frame, so nothing is added to the synchronisation; a device or swap-chain resize leaves the pipeline valid (the format does not change). Native work list: N1 data publish in `NativeRenderStep`; N2 root signature and pipeline at open;
N3 helper and its insertion points (two live, one pass); N4 a `native_fade_overlay` log row on each change of the alpha (not per frame) with the serial, the colour and the gate; N5 a unit test in the style of `gbuffer_encoder_test.cpp`: draw the helper on a gradient target and compare with the CPU straight blend.

### 7.4 Checks

| Row | Action | Expected |
|---|---|---|
| FD1 | Unit test (no game): colours black, white, red at alpha 0, 0.25, 0.5, 1 over a gradient `R8G8B8A8` target | per-channel difference at most 1/255 from the CPU straight blend, alpha byte as the UI blend leaves it; alpha 0 leaves every byte; records the rounding |
| FD2 | New Game from the title, native run: `native_fade_overlay` rows against the composite log of FL1 | alpha follows the plate (title fade, the 0.5 s fades, map-move black), at most one frame late; the read-back hash changes while alpha is above 0 and `changed_pixels` is 0 at opaque black |
| FD3 | A mid-fade screenshot (alpha near 0.5) native against Reference | mean luminance close to `(1 - alpha)` of the unfaded frame, the HUD and menus darkened too (the plates cover Scaleform) |
| FD4 | The same frame with the gate forced closed (only if FL6 shows a safe source) | no overlay, composite unchanged |
| FD5 | Credits (R1 or R5) with a plate set by the preceding cutscene | the overlay covers the movie page; no overlay at alpha 0 |
| FD6 | Resize or fullscreen toggle during a fade | pipeline valid, no validation error |
| FD7 | Frame cost with alpha 0 | no extra command recorded (`record_ms` unchanged) |

## 8. Witnesses and live rows (read-only unless marked)

**Witnesses.** CW1 the credits through R1 at the title; CW2 the same over a loaded world (R2); CW3 the whole chain through flag 20 (R5, restore by hash); CW4 a real ending (R4, Thomas's own run).

| Row | Action | Expected |
|---|---|---|
| CR1 | R1: log `native_credits_request`, `native_movie_open` (name `fdp_endroll_ww`), frame rows, close | the movie page opens, 13,002 frames play, `CSMovieImp+0x48` returns to 0 at the end, the page closes |
| CR2 | During CR1 press the skip control | the movie closes at once (`FUN_140EAF630`), the page closes, the audio stops; if the page's `+0x9D8` byte is set the press is ignored (record which) |
| CR3 | Native frame rate, memory and audio sync over the whole roll | no stall; compare with the opening of Native1252 |
| CR4 | Log the index `_DAT_14479BA04` and the value `FUN_140EA5500` returns | 1 and 3 |
| CR5 | R5: log the flag consumption, `FUN_14090ADB0`, `EndingStep` states, `DAT_14489F200` at state 7, which of C510/C570 runs, and the files the game writes | the chain of section 1 in order; the title returns; the written files are listed (settles the read-only question) |
| CR6 | File hashes of the save directory and the protected files before and after CR1/CR2 | identical for R1 (the protocol restores by hash anyway) |

## 9. Open points

1. Which dialogue or event starts the `m41_00` ending events, which ending each flag is, and the effect of flags 30 (state 8) and of the `[1447572B8+0x7C]` kind (1 or 2) later in the flow.
2. The value of `DAT_14489F200` after the credits (section 2) and whether the flow writes files when the world is left (CR5).
3. The page opened by each callable (`142916B18`, `142916AD8`), the TAE key lookup that maps the movie id to a file, and the byte `page+0x9D8`.
4. The thread that may call `FUN_140BEA570` (INFERRED: the step-system thread); row CR1 would show a fault immediately if wrong.
5. The language-group stub `FUN_140ED00C0` (encrypted at rest).
6. The inherited open points of FADE-PLATES-NATIVE: the gate `14075EAD0`, the easing `140330F50`, blend index 7.

## 10. Exports

`decompiled/DS3-1152-F-` + `00BEA570`, `00EA5500`, `00EACC20`, `00EACBD0`, `00EACC80`, `00EACA10`, `00BEA4A0`, `00BEA6B0`, `00BF1520`, `00B54F90`, `00EACCD0`, `00EA5860`, `00F0E3A0`, `00F0E3E0`, `00F0DAD0`, `00F0DA00`, `00F0D100`, `00F0E790`, `00F0D4F0`, `00F0D160`, `00F0E5E0`, `02695E40`, `02695FA0`, `02695F20`, `0178C990`, `0090AEB0`, `0090ADB0`, `0090B000`, `0090B6F0`, `0090AA10`, `0090C510`, `0090C570`, `00909AC0`, `00909F90`, `008F5930`, `004C5E20`, `00628090`, `006280A0`, `006273A0`, `006273B0`, `00625C00`, `00908120`, `00A3E180`, `00BB1AA0`, `00A67390`, `00A649F0`, `00A67060`, `007642A0`, `00765640`, `00765220`.
(Eight of the older exports, among them `00EACC20` and `00EA5860`, carry no `entry=` header line; the function names match.)
