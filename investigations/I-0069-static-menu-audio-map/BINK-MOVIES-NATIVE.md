# Bink movies: general native note (I-0069, Task 10)

STATIC (Ghidra mirror of `DarkSoulsIII.exe` 1.15.2.0 and the offline game data). Nothing was launched.
Status words: **STATIC** read in code or data, **INFERRED** deduced, **OPEN** not known statically.

**Relation to existing work.** The ready-to-code spec for the Bink player already exists and is on `main`: [I-0068 part J, BINK-NATIVE-READY.md](../I-0068-static-yebis-ffx-lights-spec/BINK-NATIVE-READY.md)
(ordered CALL / TRANSCRIBE / NATIVE list, plane upload, constant block, PSO and sampler, audio, title integration, checks K1-K8, risks). This note is the **general overview** the coordinator asked for:
which images are Bink and which are not, the whole path from file to screen, the audio path, what the native build does today, and the checklist with witnesses. It was written **independently** (a second static
reading of the same functions) and the two agree on every GPU-side value; the points where this reading adds or corrects something are listed in section 6. For code-level detail, follow part J.

Evidence outside Git: `evidence/I-0069/queries/bink/` (shader disassembly `vs.asm`, `ps-3plane.asm`, `ps-4plane.asm` and the DXBC blobs, bytes of the vtables `143A9CDF8` / `143A9CD08`, of the pointer cells
`1446CAD40` / `1446CB4C8` and of the float constants `143D7FEF0`, `143D7EE10`, `143D7EE7C`, `143D7F0D8`, the movtae strings `movtae-strings.txt`, and the archive index lines for the movie, logo and movtae files).
File headers of the two movies: `evidence/I-0068/queries/bink/movie-headers.tsv`.

## 0. Verdict

1. **Only two video files exist, and neither is a logo nor a boss cinematic.** The install holds `movie/fdp_opening.bik` (285,696,952 bytes, 1920x1080, 4750 frames, 24 fps, 5 audio tracks, no alpha) and
   `movie/fdp_endroll_ww.bik` (647,246,828 bytes, 13002 frames, 30000/1001 fps) (STATIC, `movie-headers.tsv`). The table `other/movtae.movtae` (a DCX TAE whose strings are `FDP_Opening.mp4`,
   `FDP_EndRoll_JP.mp4`, `FDP_EndRoll_WW.mp4`, STATIC) feeds the name that `FUN_140EACA10` formats into `movie:/%s.bik` (`1429A45F8`); `FDP_EndRoll_JP` has no file in this install. New Game requests movie id 1 (MENUS-MAP 35),
   which is the opening (INFERRED from the table order and the only non-credits entry; part J §0 reaches the same reading; ENDING-CREDITS-NATIVE section 2: the credits request id 2 or 3 through `FUN_140EACC20`; 1 = opening, 2 = JP end roll, 3 = WW end roll is INFERRED from the string order, the TAE key lookup was not read).
2. **Startup logos are not Bink (witness BW3).** The archive holds the Scaleform pages `05_900_logo_fromsoft.gfx`, `05_901_logo_bne.gfx`, `05_902_logo_esrb.gfx`, `05_903_warn_illegalcopy.gfx`,
   `05_905_logo_copyright.gfx` (STATIC, archive index); they are drawn by the Scaleform path like any menu page. That they are the startup logos is INFERRED from the names.
3. **A boss cinematic is not Bink (witness BW4).** No boss video file exists in `movie/`. The in-engine cutscene machinery is the "Remo" family (`SprjRemo*` classes, `RemoBase` startup file in MENUS-MAP);
   that boss introductions run through it is INFERRED from the absence of any other movie source.
4. **(Superseded: the native Bink player landed in `6d857eef`, 2026-10-11, after this note; see I-0066 `BINK-NATIVE.md` and [ENDING-CREDITS-NATIVE.md](ENDING-CREDITS-NATIVE.md) section 6. The text below describes the state this note was written against.)** **The native build omits the Bink player completely.** `NativeMoviePlayerFactory` replaces the factory `FUN_141ADE670` (`game_bootstrap.cpp:6464`, hook `{0x1ADE670, ...}` at 9360) and returns no player, which is the
   original's own "no player" path in `FUN_140EAF7E0`; log row `native_movie_player_omitted {factory: 141ADE670, caller: EAF7E0, movie_drawn: false}`. New Game continues to character creation without the
   opening, the movie texture proxy `CSMovieGxTexture` imports `SYSTEX_BLACK` (`game_bootstrap.cpp` ~8603), and **no movie sound plays**. Cause (Native1171): the original passes the old GX device `*[144961AB0]`,
   which is null natively; the player init `FUN_141ADED50` and the step both dereference it (fault `EAF8F6`).
5. **The movie audio is not FMOD.** Bink plays through its own **XAudio2** instance, or **DirectSound** as a fallback, despite the class namespace `Movie_FMOD_Bink_CG2` (section 3). This corrects the earlier
   statement in `MENU-AUDIO-NATIVE.md` (fixed there).

## 1. The path from file to screen (STATIC)

| Stage | Code | Content |
|---|---|---|
| Request | New Game `FUN_140AE9070` -> `FUN_140BEA6B0` -> `FUN_140EACC80` -> `FUN_140EACA10` -> `CSMovieIns` `FUN_140EAF570` | movie id 1, volume 1.0, track mode `FUN_141A2B6E0() < 3`; then a menu job opens the Scaleform page `MovieScreen` (`01_920_movie.gfx`, update `FUN_140B54F90`) |
| Movie step | `CSMovieIns`, FD4 step `FUN_142692DA0`: state 2 `FUN_140EAF7E0`, 3/4 `FUN_140EAFC50` / `FUN_140EAF720`, 6 `FUN_140EAFAC0`, 7 `FUN_140EAF670` | state 2: Bink allocator globals, a **120 MB** buffer (`0x7800000`), factory `FUN_141ADE670`, player init `vt+8({device, 4 threads, 0})`, open `vt+0x18(name, ...)` |
| Player init | `FUN_141ADED50` | Bink memory hooks, four `BinkStartAsyncThread`, the **sound system** (section 3), the 0x220-byte graphics object (`FUN_141ADFE30`, vtable `143A9CDF8`) and its slot `+0x08` with the device |
| Open | `FUN_141ADF230` | `BinkSetSoundTrack`, `BinkSetIOSize`, `BinkOpen(path, flags)`, `BinkSetSpeakerVolumes`, graphics `vt+0x18` (planes), then the **output texture**: `R8G8B8A8_UNORM`, movie size, bind flags `0x28` (render target + shader resource), its SRV (`player+0x38`) and RTV (`player+0x40`) |
| Per frame | playback step `FUN_140EAFAC0` -> player `vt+0x68` = `FUN_141ADEAF0` | `BinkWait`; if a frame is due: `BinkDoFrameAsyncWait(1000)`, on the first frame graphics `+0x28` and `BinkDoFrameAsyncMulti`, graphics `+0x30` (wait, mark the buffer ready), the `BinkShouldSkip` catch-up loop, `BinkNextFrame`, `+0x28`, `BinkDoFrameAsyncMulti`, then graphics **`+0x38` = `FUN_141ADFF10`** (upload and draw into the RTV); `*out` = the SRV once the first frame exists |
| Publish | `FUN_140EADE50(CSMovieGxTexture, srv)` | type check (`FUN_1419281C0 == 3`), copies the view description and size, stores the SRV at `proxy+0x18` |
| Scaleform | `MovieScreen` | samples the proxy by name as `img://` (MENUS-MAP 9); the draw into the output texture precedes it in the same frame (INFERRED order, immediate context) |
| End | `FUN_141ADFB00` (natural end), `FUN_140EACCD0` (skip), `FUN_140EAF670` | `MovieScreen` closes when `CSMovieImp+0x48` is 0 (`FUN_140EAC940`), then character creation opens |

**The GPU work is small.** Bink decodes on four CPU threads into its own `R8` plane buffers (Y full size, Cr and Cb half size, an alpha plane only if the file has one: never for the shipped files). The graphics object
holds, per frame buffer, four `R8_UNORM` textures and SRVs; `FUN_141ADFF10` uploads the buffer marked ready (`UpdateSubresource`), writes a 0x90-byte constant buffer and draws **one full-target triangle-strip
quad** (`Draw(4, 0)`, unit-quad vertex buffer at `143A9CE38`, no blend, no depth, viewport and scissor to the movie size) into the RGBA output with an embedded VS and the 3-plane pixel shader
(`ps_4_0`: `rgb = (Y*cb0[4] + Cr*cb0[1] + Cb*cb0[2] + cb0[3]) * cb0[0]`). The YCbCr matrix and bias (`cb0[1..4]`) are not in the executable: they are 64 bytes copied from **`HBINK+0xD0`** (the Bink handle returned
by `BinkOpen`), the other rows are `(1,1,1,1)`, the quad transform `(2,-2,-1,1)` and the uv-crop rows from the plane sizes (full table in part J §3). The 4-plane alpha shader is created (`+0x50`) but
`FUN_141ADFF10` never binds it, so an alpha plane is never composited.

## 2. What the native build does with each piece

| Piece | Native today | Needed to play the movies |
|---|---|---|
| Factory `FUN_141ADE670` | hooked, returns 0 (omission) | unhooked |
| Steps `FUN_140EAF7E0`, `FUN_140EAFAC0` | run only up to the "no player" path | transcribed without the `[144961AB0]` reads (part J §1.1) |
| Player init / open / close / render | not reached | open/close create the RGBA output with native objects; render passes a native context token (part J §1.2) |
| Graphics object (8 slots) | not created | slots `+0x08/+0x10/+0x18/+0x20/+0x38` native (pipeline, plane textures, upload, draw); `+0x28` is empty, `+0x30` stays original (part J §1.3) |
| Publish `FUN_140EADE50` | not reached; proxy imports `SYSTEX_BLACK` | store a native output token and register it with the UI image lookup (part J §1.4) |
| Bink decode threads, `binkw64.dll` | not started | unchanged CPU code |
| Audio (XAudio2 / DirectSound) | not started | unchanged OS audio, no GPU work |
| Scaleform logo pages and `MovieScreen` | `ObserveUiMovieAdvance` runs for Scaleform movies; `MovieScreen` closes at once because `CSMovieImp+0x48` is 0 | unchanged |

## 3. Movie audio (STATIC)

- **Sound system** (`FUN_141ADED50` -> `FUN_141ADFCF0`): unless `FUN_141A2B6E0() == 2`, `FUN_141ADFD40` runs `CoInitialize`, `CoCreateInstance(XAudio2)`, engine start and `CreateMasteringVoice`, then
  **`BinkSetSoundSystem(BinkOpenXAudio2, xaudio2)`** (import `144DBC07C`); otherwise, or if that fails, **`BinkSetSoundSystem(BinkOpenDirectSound, 0)`** (import `144DBC084`).
- **Tracks and volume** (`FUN_141ADF230`, part J §5): mode `FUN_141A2B6E0() < 3`; mode 0 maps tracks 0..3 to L+R, C, LFE and the rear pair, mode 1 uses track 4 on L+R (INFERRED stereo downmix);
  volume = sound option `[144769290]+0x15C` scaled by `32768.0` (`143A9CD98`).
- No FMOD call exists in the player or graphics functions read. The movie audio is independent of the renderer and of the FMOD sound manager of `MENU-AUDIO-NATIVE.md`; the native build plays none because it creates no player.

## 4. Native checklist

| # | Requirement |
|---|---|
| B1 | **Decision.** Keep the explicit omission until the player work is done; `native_movie_player_omitted` is the regression marker. |
| B2 | Replace the graphics object and the three GX calls inside the player open (`FUN_141ADF230`: texture, SRV, RTV on `player+0x28`) and the close; keep the Bink calls, threads and audio. |
| B3 | Plane textures and upload: `R8_UNORM`, Y full, Cr and Cb half, only for the ready buffer, copied on the game thread inside slot `+0x38` (the decode threads reuse the buffers). Keep `HBINK` untouched: size at `+0/+4`, frame count `+8`, the 16 colour floats at `+0xD0`. |
| B4 | Pipeline: the embedded VS and the 3-plane PS, the 9 x float4 block, one clamped linear sampler (GX filter 6, part J §4), triangle strip, no blend, no depth. Do not wire the alpha PS. |
| B5 | Output: one `R8G8B8A8_UNORM` texture per open, stable until close, bound into the `CSMovieGxTexture` proxy; `SYSTEX_BLACK` before the first frame; `FUN_140EADE50` must be redirected or the native object must satisfy its three readers (`FUN_1419281C0`, `FUN_141928420`, `FUN_141924730`). |
| B6 | Pacing unchanged (`BinkWait`, `BinkShouldSkip`); duration 197.9 s for the opening; `CSMovieImp+0x48` back to 0 at the end; skip by `FUN_140EACCD0`. |
| B7 | Audio untouched: COM initialized on the thread that runs `FUN_141ADED50`; the DirectSound fallback remains valid. |
| B8 | The step and the render must not read `[144961AB0]` (the Native1171 fault site). |

**Witnesses.** BW1 *intro*: New Game, `MovieScreen` opens, the opening plays (4750 frames, 197.9 s, 1920x1080) and character creation opens at the end or on skip (today: `native_movie_player_omitted`, no picture, no sound).
BW2 *credits*: `fdp_endroll_ww` (433.8 s), reachable only at the end of the game: a replay needs a save at the end (**OPEN**, not needed for the first milestone). BW3 *startup logos*: Scaleform pages, pass without any Bink work.
BW4 *boss cinematic*: an in-engine Remo sequence, pass without any Bink work; a movie regression must not change it.

**Live rows (BL), with part J's checks K1-K8 as the detailed measures:**

| Row | Action | Expected |
|---|---|---|
| BL1 | Reference run, New Game: log the return of `FUN_141ADE670`, the open path and the sound-system result | non-null player; `movie:/fdp_opening.bik`; XAudio2 or DirectSound accepted |
| BL2 | Log `HBINK` width, height, frames, rate and the 16 floats at `+0xD0` once | 1920, 1080, 4750, 24/1 and the colour block (K1, K3) |
| BL3 | Break on the constant-buffer `FUN_141920300` (size `0x90`) in `FUN_141ADFF10` and dump 9 float4 | rows 1-4 equal the 16 floats of BL2, row 0 `(1,1,1,1)`, row 5 `(2,-2,-1,1)` |
| BL4 | Native run with the player enabled: first draw tick, duration, natural end, skip key (K2, K4-K6) | as the Reference run |
| BL5 | Same run: the process shows a mastering voice during the movie and no `EAF8F6` fault (K7, K8) | movie sound |

## 5. Open points

1. The movie id to file mapping is inferred (id 1 = opening); BL1 settles it. The id and trigger of the ending movie are unread.
2. Whether any other code path draws or pauses the movie through the FMOD sound manager (none found in the functions read).
3. The player's live reaction to the options menu (the volume source is read, the reaction is not).

## 6. Points where this independent reading adds to or corrects part J

- Section 0 points 1 to 3: the explicit statement that the **logos are Scaleform pages** and **boss cinematics are not Bink**, with the evidence (archive index, the two-file `movie/` folder).
- The correction of `MENU-AUDIO-NATIVE.md` (movie audio is XAudio2 or DirectSound, not FMOD).
- Confirmed independently: graphics vtable slots and functions, the 9 x float4 block rows, the PS-A binding only (`+0x48`), the RGBA output creation (`0x1C`, flags `0x28`, SRV `+0x38`, RTV `+0x40`), the publish through `*param_3` of `FUN_141ADEAF0`, the draw state sequence.
- The colour block is read from the Bink handle (`param_2` of `FUN_141ADFF10` is the handle that `BinkOpen` returned, not the frame-buffer info copy at `this+0x68`), as part J states; the labels "Cr / Cb / Y column" are INFERRED from the shader and the Bink plane order, not read in the executable.
- The format numbers (`0x1C`, `0x3D`) assume the GX wrapper enum is DXGI-numbered, the project-wide convention (INFERRED here; part J §4 lists the GX translators for the sampler).

## 7. Exports

`decompiled/DS3-1152-F-` + `01ADFF10`, `01AE0830`, `01AE0BA0`, `01AE0E50`, `01AE0800`, `01AE0660`, `01AE0570`, `01ADFEE0` (new here), `01ADFE30`, `01ADE670`, `01ADE720`, `01ADEAF0`, `01ADF230`, `01ADFD40`, `01ADFCF0`,
`01ADED50`, `01ADE850`, `01ADE940`, `01ADFB80`, `00EAF7E0`, `00EAFAC0`, `00EADE50`, `01925450`, `01920300`, `01921370` (all but `01ADFEE0` were already exported by earlier passes).
