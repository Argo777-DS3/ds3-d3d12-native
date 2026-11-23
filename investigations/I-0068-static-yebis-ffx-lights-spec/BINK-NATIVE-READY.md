# I-0068 part J: native Bink video player, ready to code

Request (coordinator, 2026-10-10): the same "ready to code" format for the Bink player. Natively, the
opening video is skipped today by a bypass. The note covers:

- the ordered CALL / TRANSCRIBE / NATIVE list (Bink's CPU decoding is kept; only the textures and
  the draw become native);
- the YUV plane formats and their upload;
- the conversion shader and its PSO;
- the integration with the title sequence, and what replaces the current bypass;
- the audio path;
- measurable checks (first frame, duration, natural end, skip by key as in the original);
- the risks.

Sources: MENUS-MAP §35/§37/§40 (I-0067, read only, not modified), plus this pass's static
reading. Labels: STATIC, OBSERVED, INFERRED.

Evidence: `evidence/I-0068/queries/bink/`:

- `b1..b6-index.txt` (decompile indexes);
- `shaders/` (DXBC disassembly of the three shaders);
- `movie-headers.tsv` (`Get-BinkMovieHeaders.ps1`).

## 0. What is skipped today, and what the game asks for (STATIC + OBSERVED)

- **The bypass:** the pilot detours `FUN_141ADE670` (the only player factory) to
  `NativeMoviePlayerFactory`, which returns 0.
  - Without a player, `CSMovieIns` takes its own "no player" path in `STEP_Init_Setup`
    `FUN_140EAF7E0` (`+0x148/+0x149 = 0`, `+0x14A = 0`, state 1).
  - `MovieScreen` closes on the next update and the title sequence continues to character
    creation (MENUS-MAP §35, §40).
  - The native UI shows `SYSTEX_BLACK` for the movie proxy (vtable `1429A4968`).
  - The cause (Native1171): the original passes the old GX device `*[144961AB0]`, which is null
    natively, and fails at `EAF8F6`.
- **The movie** (STATIC `FUN_140EACA10`): New Game (`FUN_140AE9070` → `FUN_140BEA6B0` →
  `FUN_140EACC80`) requests **movie id 1** with volume scale 1.0 and the track mode
  `FUN_141A2B6E0() < 3`. The name comes from the movtae resource, formatted as
  `"movie:/%s.bik"` (`1429A45F8`).
  - The game ships two movies (OBSERVED, `movie-headers.tsv`, read only):

    | File | Signature | Frames | Size | Rate | Audio tracks | Alpha | Duration |
    |---|---|---|---|---|---|---|---|
    | `fdp_opening.bik` | `BIKi` (Bink 1) | 4750 | 1920×1080 | 24/1 | 5 | no (flags 0) | **197.917 s** |
    | `fdp_endroll_ww.bik` | `BIKi` | 13002 | 1920×1080 | 30000/1001 | 5 | no | 433.833 s |

  - INFERRED: id 1 is `fdp_opening`. The other requests are `FUN_140EACBD0` (id 0) and
    `FUN_140EACC20` (an id from `FUN_140EA5500`), which fit the end roll.
  - The library is `binkw64.dll` 1.995j (OBSERVED file version), already imported by the
    executable.

## 1. Ordered list

### 1.1 Game side: unchanged except two steps and the publish (CALL unless marked)

| # | Function | Role | Mode |
|---|---|---|---|
| 1 | `FUN_140AE9070` → `FUN_140BEA6B0` | New Game: request movie 1 (`FUN_140EACC80` → `FUN_140EACA10` → `FUN_140EAC950` → `CSMovieIns` `FUN_140EAF570`, `+0x148 = 1`), then the `MovieScreen` menu job | CALL |
| 2 | `CSMovieIns` state 1 `FUN_140EAFC00` | wait for the request | CALL |
| 3 | state 2 **`STEP_Init_Setup` `FUN_140EAF7E0`** | sets up the Bink allocator (`FUN_141ADE5B0` globals), **factory `FUN_141ADE670`**, player init `vt+8({device, 4 threads, flag 0})`, open `vt+0x18(name, {tracks, count, IO 0, volume, loop +0x10C})` | **TRANSCRIBE**: identical except the init descriptor's device word = the **native device token**, not `*[144961AB0]`. The factory is CALL again: the bypass hook is removed. |
| 4 | states 3/4 `FUN_140EAFC50` / `FUN_140EAF720` | wait for open (`vt+0x20` = player state: 2 open, 0 failed), `vt+0x48`; then play `vt+0x30` | CALL |
| 5 | state 6 **`STEP_Wait_Play` `FUN_140EAFAC0`** | pause sync `vt+0x50`, update `vt+0x48`, **render `vt+0x68(player, *(*[[144961AB0]+0x60])+8, &srv)`**, publish `FUN_140EADE50(proxy +0xB8, srv)`, movtae events `FUN_140EB0890`, end test `vt+0x40` → next state | **TRANSCRIBE**: identical except the context argument = the native recording token (§3), and the publish = §1.4 |
| 6 | state 7 `STEP_Finish_Play` `FUN_140EAF670` | publish null, `vt+0x48`, close `vt+0x28`, release `vt+0x10`, free, `+0x149 = 1`, state 1 | CALL (its publish call reaches §1.4) |
| 7 | `CSMovieImp` update `FUN_140EACD60`, `MovieScreen` update `FUN_140B54F90` | clear the current movie; close `MovieScreen` (result 2) once `FUN_140EAC940` is false; skip input | CALL |

### 1.2 Player `Movie_FMOD_Bink_CG2::MVMoviePlayerImplBink` (vtable `143A9CD08`, 18 slots, object 0x98)

Fields (STATIC):

- `+0x08` / `+0x10`: allocators;
- `+0x18`: state (3 opening, 2 open, 0 closed);
- `+0x1C` / `+0x20`: width / height;
- `+0x28`: device (from init);
- `+0x30` / `+0x38` / `+0x40`: output texture / SRV / RTV;
- `+0x48`: `HBINK`;
- `+0x50` / `+0x58`: async decode thread count / ids (4 threads, `BinkStartAsyncThread`);
- `+0x60`: Bink memory hooks;
- `+0x68`: graphics object;
- `+0x70`: sound-system block;
- `+0x90`: play state (0 stopped, 1 playing, 2 starting);
- `+0x91`: initialised; `+0x92`: first frame drawn; `+0x93`: no-graphics flag; `+0x94`: loop.

| Slot | Function | Role | Mode |
|---|---|---|---|
| +0x00 | `FUN_141ADE7F0` | destructor | CALL |
| +0x08 | `FUN_141ADED50` | init: `BinkSetMemory` hooks, 4 async threads, **sound system** (§5), graphics object `0x220` (`FUN_141ADFE30`) and its init `vt+8(alloc, alloc, device)`; `+0x28 = device` | CALL (the device is the native token; the graphics init it calls is native, §1.3) |
| +0x10 | `FUN_141ADE940` | release: graphics `vt+0x10`, graphics destructor, stop async threads, `+0x91 = 0` | CALL |
| +0x18 | `FUN_141ADF230` | **open**: path, `BinkSetSoundTrack` / `BinkSetIOSize` / `BinkOpen` (flags `0x400` or `0x100`, `| 0x4000` with sound, `| 0x1000000` with IO size), `BinkSetSpeakerVolumes` per track (volume × 32768), graphics `vt+0x18` (planes), then the **output texture** (`FUN_1419256D0`: w×h, 1 mip, `R8G8B8A8_UNORM`, bind SRV \| RTV) with SRV and RTV, state 2 | **TRANSCRIBE**: every Bink call and the path helpers stay CALL; the output texture and its views are **NATIVE** (§2.3) |
| +0x20 | `FUN_141ADEAE0` | state | CALL |
| +0x28 | `FUN_141ADE850` | **close**: release the RTV/SRV/texture GX objects, graphics `vt+0x20` (free planes), `BinkClose`, state 0 | **TRANSCRIBE**: native output retired after the fence; the rest CALL-equivalent |
| +0x30 | `FUN_141ADF710` | play: `+0x90 = 2` | CALL |
| +0x38 | `FUN_141ADF740` | stop: wait async, `BinkGoto(0)`, `+0x92 = 0`, `+0x90 = 0` | CALL |
| +0x40 | `FUN_141ADEEC0` | stopped: `+0x90 == 0` | CALL |
| +0x48 | `FUN_141ADFB00` | **natural end**: playing, not looping, and `HBINK FrameNum(+0xC) == Frames(+8)` → `vt+0x38` | CALL |
| +0x50 | `FUN_141ADF220` | `BinkPause` | CALL |
| +0x58 / +0x60 | `FUN_141ADECB0` / `FUN_141ADEA70` | width / height | CALL |
| +0x68 | `FUN_141ADEAF0` | **render** (§3) | **TRANSCRIBE**: no GX state save/restore (`FUN_141ADF7C0` / `FUN_141ADEED0` dropped); graphics slots native; `*out` = native output token |
| +0x70 | `FUN_141ADEA30` | time = `FrameNum / (FrameRate / FrameRateDiv)` (used by the movtae events) | CALL |
| +0x78 | `FUN_141ADEA80` | empty | CALL |
| +0x80 / +0x88 | `FUN_141ADF7A0` / `FUN_141ADF200` | `BinkControlBackgroundIO(1 / 2)` | CALL |

### 1.3 Graphics `MVBinkGraphics` (vtable `143A9CDF8`, 8 slots, object 0x220)

Fields (STATIC):

- `+0x08..+0x18`: allocators and device;
- `+0x20` sampler, `+0x28` depth-stencil, `+0x30` rasterizer, `+0x38` input layout, `+0x40` VS,
  `+0x48` PS opaque, `+0x50` PS alpha, `+0x58` VB, `+0x60` CB;
- **`+0x68` `BINKFRAMEBUFFERS`** (OBSERVED layout from the reads):
  - `+0x68` set count;
  - `+0x6C/+0x70` Y/A buffer width and height;
  - `+0x74/+0x78` cR/cB buffer width and height;
  - `+0x7C` current set;
  - `+0x80 + set·0x40`: four planes `{Buffer +0, Allocate +8, Pitch +0xC}` (Y, cR, cB, A);
- per set at `+0x140 + set·0x48` (3 sets): textures `+0..+0x18`, SRVs `+0x20..+0x38`, ready
  flag `+0x40`;
- `+0x218`: initialised; `+0x219`: planes allocated.

Native keeps the CPU fields (`+0x68..+0x13F`, the ready flags, `+0x218/+0x219`). The GPU
objects live in a native side table keyed by the graphics pointer, and the GX fields stay null.

| Slot | Function | Mode | Native |
|---|---|---|---|
| +0x00 | `FUN_141ADFEE0` destructor | CALL | — |
| +0x08 | `FUN_141AE0830` init | **NATIVE** | root signature + 1 PSO (§4), immutable VB; set `+0x218 = 1` |
| +0x10 | `FUN_141AE0660` release | **NATIVE** | retire them; `+0x218 = 0` |
| +0x18 | `FUN_141AE0BA0` planes | **TRANSCRIBE** | `BinkGetFrameBuffersInfo(hbink, +0x68)`, `BinkAllocateFrameBuffers` (unless `+0x80` is preset), **native plane textures** for each set and present plane (§2.1), `BinkRegisterFrameBuffers`, `+0x219 = 1` |
| +0x20 | `FUN_141AE0570` free planes | **NATIVE** | retire the plane textures; same CPU resets (ready flags 0, `memset(+0x68, 0, 0xD8)`, `+0x219 = 0`) |
| +0x28 | `FUN_141AE0E40` | CALL | empty |
| +0x30 | `FUN_141AE0800` | CALL | `BinkDoFrameAsyncWait(-1)`, mark the current set ready |
| +0x38 | `FUN_141ADFF10` draw | **NATIVE** | §3 |

### 1.4 Publish `FUN_140EADE50(CSMovieGxTexture, srv)` (proxy vtable `1429A4968`)

- **Original:** checks that the SRV's resource is `TEXTURE2D` (`FUN_1419281C0 == 3`), copies the
  SRV description to `+0x20` and the size to `+0x38`, then stores the SRV at `+0x18` (0 clears
  it).
- **Native (TRANSCRIBE):**
  - store the native output token at `+0x18`, the native description at `+0x20` (format 0x1C,
    `TEXTURE2D`), and the size at `+0x38`;
  - record `proxy → native output` in the native movie registry.
- The native UI's image lookup, which today maps vtable `1429A4968` to `SYSTEX_BLACK`, returns
  the registered output instead. It is admitted like the native offscreen targets: one resident
  record, R8G8B8A8, video size. Until the first publish it falls back to `SYSTEX_BLACK`, the
  equivalent of the proxy's default texture.

## 2. Resources and upload

### 2.1 YUV planes (STATIC `FUN_141AE0BA0` / `FUN_141AE0E50`)

| Plane | Size | Format | Usage | Present |
|---|---|---|---|---|
| Y | `+0x6C × +0x70` (Bink buffer, at least the video size) | `R8_UNORM` (0x3D) | default, SRV, 1 mip | always |
| cR | `+0x74 × +0x78` (half size) | `R8_UNORM` | default, SRV | always |
| cB | `+0x74 × +0x78` | `R8_UNORM` | default, SRV | always |
| A | Y size | `R8_UNORM` | default, SRV | only when the plane's Allocate flag is set: **never for the shipped movies** (header flags 0) |

- **Sets:** up to 3 (`BINKMAXFRAMEBUFFERS`; the free loop runs over 3). Bink decodes into one
  set while another is drawn.
- **Native:** committed `R8_UNORM` textures in COPY_DEST, one SRV each in a per-set descriptor
  table t0..t3. For an absent A plane, bind a null SRV.

### 2.2 Upload (per drawn frame, `FUN_141ADFF10` step 1)

- **Original:** `UpdateSubresource` of each present plane of the **ready** set `+0x7C` (flag
  `+0x180 + set·0x48`, cleared after the upload), from Bink's CPU buffer (pointer `+0`,
  pitch `+0xC` of the plane record). The whole texture is copied, with no box.
- **Native:**
  - In slot +0x38, on the game thread, `memcpy` each plane row by row into a **dedicated movie
    upload ring**, at a 512-byte aligned offset with a row pitch rounded up to 256
    (`D3D12_TEXTURE_DATA_PITCH_ALIGNMENT`).
  - Record `CopyTextureRegion` (placed footprint `R8_UNORM`, w, h, pitch) for the native frame.
  - Copy at draw time rather than at GPU time: the 4 async decode threads write the other sets
    straight after `BinkDoFrameAsyncMulti`, and Bink reuses this set two frames later.
- **Ring size:**
  - one 1920×1080 frame ≈ 2.07 MB (Y) + 2 × 0.52 MB (cR, cB) = **3.1 MB** with 256-byte pitch
    rounding (INFERRED from the Bink buffer sizes; log `+0x6C..+0x78` at open);
  - size the ring for the frames in flight (about 3 × 3.2 MB). It must not be the 2 MB FFX ring.

### 2.3 Output (STATIC `FUN_141ADF230`)

- One `R8G8B8A8_UNORM` texture of the video size (1920×1080), RT + SRV, **created at open and
  kept until close**.
- Scaleform's `CSTextureImage` binds once, so the resource must stay the same for the whole
  playback (MENUS-MAP §37).
- **Native:**
  - a committed resource, initial state PIXEL_SHADER_RESOURCE, with one RTV and one SRV;
  - clear colour none (the draw covers it);
  - retire it after the fence of the last frame that sampled it (close, §1.2 +0x28).

## 3. Per frame (`FUN_141ADEAF0` → `FUN_141ADFF10`)

Game thread, from `STEP_Wait_Play`, once per game update (STATIC):

1. If `vt+0x40` (stopped): nothing.
2. `BinkWait(hbink)`: non-zero means it is too early, so there is no new frame. The last output
   is still published once a first frame exists (`+0x92`).
3. Otherwise:
   1. `BinkDoFrameAsyncWait(hbink, 1000)`.
   2. **First frame** (`+0x90 == 2`): graphics `+0x28`, `BinkDoFrameAsyncMulti(hbink, ids, 4)`,
      `+0x90 = 1`.
   3. Graphics `+0x30` (wait, mark ready).
   4. **Catch-up loop** while `BinkShouldSkip`: `BinkNextFrame`, `+0x28`,
      `BinkDoFrameAsyncMulti`, `+0x30`.
   5. Then `BinkNextFrame`, `+0x28`, `BinkDoFrameAsyncMulti` (decode of the next frame starts).
   6. **Graphics `+0x38` draw** of the ready set into the output RTV.
   7. `+0x92 = 1` on the first frame.
4. `*out = output SRV` (`+0x38`).

**Draw (NATIVE, recorded for the native frame before the UI pass):**

| Step | Original D3D11 (`FUN_141ADFF10`) | D3D12 |
|---|---|---|
| planes | `UpdateSubresource` ×3 (×4 with A) | `CopyTextureRegion` ×3 from the ring (§2.2); barriers COPY_DEST → PIXEL_SHADER_RESOURCE (and back before the next copy) |
| constants | `UpdateSubresource` of the 144-byte CB | 144 bytes (256 allocated) from the upload ring → root CBV b0 |
| state | `RSSetState`, `OMSetDepthStencilState(+0x28, 0)`, `OMSetBlendState(null, 0xFFFFFFFF)`, viewport and scissor `(0, 0, w, h)` | PSO (§4), `RSSetViewports` / `RSSetScissorRects (0, 0, w, h)` |
| bind | `PS/VSSetConstantBuffers(0)`, `PSSetSamplers(0, 1)`, `PSSetShaderResources(0, 4, set +0x160)`, VS `+0x40`, PS **opaque** `+0x48` | root signature; SRV table of the set; static sampler |
| target | `OMSetRenderTargets(1, output RTV, null)` | output PIXEL_SHADER_RESOURCE → RENDER_TARGET, `OMSetRenderTargets` |
| geometry | `IASetPrimitiveTopology(TRIANGLESTRIP)`, input layout, VB `+0x58` stride 8, `Draw(4)` | `IASetPrimitiveTopology(TRIANGLESTRIP)`, VB view (32 bytes, stride 8), `DrawInstanced(4, 1, 0, 0)` |
| after | (GX state restored) | output RENDER_TARGET → PIXEL_SHADER_RESOURCE, before the UI pass samples it |

- One record per game update at most (the draw runs once per `Render`).
- If the native frame consumes slower than the game updates, keep only the latest record. The
  output is overwritten anyway; the plane ring slot of a dropped record is simply reused.

**Constant block, 144 bytes** (STATIC from `FUN_141ADFF10`; values at `143D7FEF0`, `143D7EE10`,
`143D7EE7C`, `143D7F0D8`). With `Ytex = (+0x6C, +0x70)`, `Ctex = (+0x74, +0x78)` and video
`(w, h)`:

| Register | Content | Read by |
|---|---|---|
| c0 | `(1, 1, 1, 1)` colour multiplier | PS |
| c1 | `HBINK+0xD0..+0xDF`: the cR column of Bink's colour-space matrix | PS × cR |
| c2 | `HBINK+0xE0..+0xEF`: the cB column | PS × cB |
| c3 | `HBINK+0xF0..+0xFF`: the bias | PS |
| c4 | `HBINK+0x100..+0x10F`: the Y column | PS × Y |
| c5 | `(2, −2, −1, 1)`: position = `xy·c5.xy + c5.zw` | VS |
| c6 | `(w/Ytex.w, 0, (w/2)/Ctex.w, 0)` | VS (UV from x) |
| c7 | `(0, h/Ytex.h, 0, (h/2)/Ctex.h)` | VS (UV from y) |
| c8 | `(0, 0, 0, 0)` | VS (UV offset) |

- Copy the 64 bytes at `HBINK+0xD0` unchanged each frame, as the original does.
- The roles of c1..c4 are now STATIC (shader disassembly, §4). That replaces MENUS-MAP §37's
  INFERRED "colour-space matrix".

## 4. Shaders and PSO (STATIC, `shaders/*.asm`)

- **VS** (`1446CAAB0`, 656 B, `vs_4_0`):
  - input `POSITION float2`;
  - `o0 = (v·c5.xy + c5.zw, 0, 1)`;
  - `o1 = v.x·c6 + v.y·c7 + c8`: xy = luma UV, zw = chroma UV.
- **PS opaque** (`1446CAD50`, 896 B, `ps_4_0`, the one the draw binds):
  - `Y = t0(uv.xy)`, `cR = t1(uv.zw)`, `cB = t2(uv.zw)`, red channels, one sampler s0;
  - `rgba = (Y·c4 + cR·c1 + cB·c2 + c3) · c0`.
- **PS alpha** (`1446CB0D0`, 1012 B):
  - the same RGB, with `a = t3(uv.xy).x`;
  - created but never bound by `FUN_141ADFF10`. Not needed for the shipped movies (no alpha
    plane).
- **Native:**
  - take the three blobs from the image at those addresses (file offsets `0x46C8AB0`,
    `0x46C8D50`, `0x46C90D0` in `.data`), as for the Scaleform bank;
  - build one PSO, with an optional second for alpha.
- **PSO:**

| Field | Value | Source |
|---|---|---|
| input layout | `POSITION`, `R32G32_FLOAT`, offset 0, slot 0 | VS signature, VB stride 8 |
| topology type | TRIANGLE (strip at draw) | `Draw(4)` strip |
| rasterizer | SOLID, cull NONE, FrontCCW false, depth bias 0, **depth clip on**, multisample flag on (1 sample) | GX raster desc through `FUN_141929FF0` (0 → SOLID), `FUN_14192A010` (0 → NONE), bytes `0x10101` |
| depth-stencil | depth off, stencil off | GX desc with the depth byte cleared |
| blend | disabled, write mask RGBA | `OMSetBlendState(null)` |
| RTV | `R8G8B8A8_UNORM`, no DSV, 1 sample | output texture |

- **Root signature:**
  - b0 root CBV (VERTEX + PIXEL);
  - SRV table t0..t3 (PIXEL);
  - **static sampler s0 = `MIN_MAG_LINEAR_MIP_POINT` (0x14), address U/V/W CLAMP, LOD 0..FLT_MAX**
    (GX desc filter 6 → `FUN_14192A040` → 0x14; address 2 → `FUN_14192A120` → 3; MaxLOD
    `0x7F7FC99E`).
  - This closes MENUS-MAP §37's open sampler/raster item.

## 5. Audio: no D3D path (STATIC)

- **Init** (`FUN_141ADED50` → `FUN_141ADFCF0`):
  - unless `FUN_141A2B6E0() == 2`, `FUN_141ADFD40`: `CoInitialize`, `CoCreateInstance(XAudio2)`,
    engine start, mastering voice, then **`BinkSetSoundSystem(BinkOpenXAudio2, xaudio2)`**;
  - otherwise, or on failure, `BinkSetSoundSystem(BinkOpenDirectSound, 0)`.
- The class name says FMOD, but Bink plays its own sound through its own XAudio2 (or
  DirectSound) instance.
- **Tracks** (`FUN_140EAF7E0` → open):
  - mode `+0x110 = (FUN_141A2B6E0() < 3)`;
  - mode 0: tracks {0, 1, 2, 3}, mapped to speakers L+R, C, LFE, and the rear pair
    (`BinkSetSpeakerVolumes`);
  - mode 1: track {4} only, on L+R (INFERRED stereo downmix; the files have 5 tracks);
  - volume = sound option `[144769290]+0x15C` × 1.0 × 32768.
- **Native:** nothing. It is CPU and OS audio, unchanged (CALL), and needs no GPU work.

## 6. Integration with the title sequence: what replaces the bypass

- **Remove:**
  - the detour `{0x1ADE670, NativeMoviePlayerFactory}`, so the original factory runs (CPU
    allocation);
  - the `SYSTEX_BLACK` substitution for vtable `1429A4968`, which becomes the fallback before
    the first frame only.
- **Add the detours:**
  - `FUN_140EAF7E0`, `FUN_140EAFAC0`, `FUN_140EADE50`: game side (§1.1, §1.4);
  - `FUN_141ADF230`, `FUN_141ADE850`, `FUN_141ADEAF0`: player (§1.2);
  - `FUN_141AE0830`, `FUN_141AE0660`, `FUN_141AE0BA0`, `FUN_141AE0570`, `FUN_141ADFF10`:
    graphics (§1.3).
- **The sequence becomes the original's:**
  1. New Game;
  2. `MovieScreen` opens; the movie plays for 4750 frames;
  3. natural end or skip;
  4. `STEP_Finish_Play` (`+0x149 = 1`), back to state 1;
  5. `CSMovieImp` clears `+0x48`;
  6. `MovieScreen` closes with result 2;
  7. the next job of the title sequence opens `04_000_ChrMake_Top` (MENUS-MAP §40).
- **Device and context tokens:**
  - the two steps no longer read `[144961AB0]` (the Native1171 fault site);
  - the device token reaches the player only to be stored at `+0x28` and passed to the native
    graphics init. Nothing dereferences it.
- **Thread:** everything above runs on the game thread, as in the original. The GPU work is
  recorded later by the native frame from the queued record, never from the game thread.

## 7. Checks

| # | Check | Measure | Pass |
|---|---|---|---|
| K1 | open | log at open: `HBINK` width, height, frames, rate (`+0, +4, +8, +0x14, +0x18`), buffer sizes `+0x6C..+0x78`, set count, sound system result, track mode | 1920, 1080, 4750, 24/1 as the file header; 3 planes, no A |
| K2 | **first frame** | game tick of New Game → tick of the first `+0x38` draw; first publish | first draw on the first `Wait_Play` update after open (same update count as the original, measured with the same pilot) |
| K3 | image | capture of `MovieScreen` at Bink frame 240 (10 s) vs the original at the same `FrameNum` | mean absolute difference ≤ 2/255 per channel (INFERRED tolerance); the 16 floats at `HBINK+0xD0` logged once and equal in both runs |
| K4 | **duration** | wall time first draw → `vt+0x40` true; frames drawn + frames skipped by `BinkShouldSkip` | 197.9 s ± 0.1 s; drawn + skipped = 4749 or 4750 |
| K5 | **natural end** | `FrameNum == Frames` → `vt+0x38`; `Finish_Play`; `MovieScreen` close result 2; character creation opens | same order of events as the original; output and planes retired after the fence (native resource count back to its value before New Game) |
| K6 | **skip by key** | during playback, press the key the original uses (the `MovieScreen` input `FUN_140B60850`: command `0x54`, or the five confirm checks when `+0x9D9` is set; take the same key from a Reference run with the same pilot) | `+0x149 = 1` on that update, `vt+0x38` → `Finish_Play`, `MovieScreen` closed on the next update, as in the Reference run; no draw after the close |
| K7 | audio | sound system result and track mode in the log; audible output | `BinkOpenXAudio2` accepted; mode 1 on a stereo setup |
| K8 | no old device | the movie path never reads `[144961AB0]` | no access violation at `EAF8F6`; the native refusal log stays empty |

## 8. Risks

1. **Old device reads.** `FUN_140EAF7E0` and `FUN_140EAFAC0` both dereference `[144961AB0]`. Both
   must be transcribed before the factory bypass is removed, otherwise Native1171 returns.
2. **Plane race.** Copy the ready set's rows on the game thread inside slot +0x38. Copying later
   from Bink's buffers races the 4 async decode threads.
3. **Upload volume.** About 3.1 MB per drawn frame at 24 fps. Use a dedicated ring sized for
   the frames in flight; the shared 2 MB FFX ring would overflow.
4. **Stable output.** One output resource per open, never recreated mid-playback. Scaleform's
   image binds it once.
5. **Proxy readers.** `CSMovieGxTexture+0x18` now holds a native token. Any CPU path that
   dereferences it as a GX SRV (its slots `FUN_140EADCA0..FUN_140EADE40`) must stay behind the
   native UI's vtable check. Audit those 17 slots before enabling.
6. **Pacing.** Bink paces by real time (`BinkWait`, `BinkShouldSkip`). A slow native frame
   drops displayed frames but keeps the duration. Do not throttle the game update during
   `MovieScreen`.
7. **Audio init on the game thread.** `CoInitialize` and a second XAudio2 instance sit beside
   the game's FMOD. This is the original behaviour, so no change is expected (INFERRED).
8. **Colour.** The output is UNORM and non-sRGB, and goes through the gamma pass afterwards
   like the rest of the interface. Comparing against the original must use the same brightness
   setting.

## Exports

`decompiled/`:

- player `01ADE670`, `01ADE720`, `01ADED50`, `01ADE940`, `01ADE850`, `01ADF710`, `01ADF740`,
  `01ADEEC0`, `01ADFB00`, `01ADEA30`, `01ADEAE0`, `01ADFCF0`, `01ADFD40`, `01ADFB80`,
  `01ADF7C0`, `01ADEED0`;
- graphics `01ADFE30`, `01AE0570`, `01AE0660`, `01AE0800`, `01AE0E40`;
- game side `00EAF7E0`, `00EAFAC0`, `00EAF670`, `00EAFC50`, `00EAF720`, `00EAF630`, `00EAF570`,
  `00EACA10`, `00EAC950`, `00EACC80`, `00EACBD0`, `00EACC20`, `00EACCD0`, `00EACD60`,
  `00B54F90`, `00B60850`, `00BEA6B0`, `00EADE50`, `00EAFD80`, `007CE9D0`;
- GX translation `01925450`, `01925400`, `01925360`, `01927B60`, `0192FA20`, `0192A040`,
  `0192A120`, `0192F860`, `01929FF0`, `0192A010`, `0192F600`, `01929ED0`, `01929EF0`,
  `01920300`;
- already exported for MENUS-MAP §37: `01ADFF10`, `01AE0830`, `01AE0BA0`, `01ADF230`,
  `01AE0E50`, `01ADEAF0`.
