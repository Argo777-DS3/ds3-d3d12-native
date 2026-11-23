# I-0068 part E: native D3D12/DXGI specification for display modes, fullscreen, resize, swap chain and screenshots

Request (coordinator, 2026-10-10): the last D3D11 paths outside GX without a specification
(MENUS-MAP §34 lists A/B, §38):

- screenshots;
- DXGI display modes: enumeration, fullscreen, resolution change, resize and swap chain.

Also: list any other D3D11 site outside GX that no specification covers.

Static reading (private read-only database copy). Reused: K-0004–K-0009 (I-0002–I-0004: device,
swap-chain object, Present and SyncInterval). The I-0066 branch's native presenter is read for
the gap list (`native/renderer/backend.cpp`, `FramePresenter`). Labels: STATIC, INFERRED.

## 1. Answer first

- DS3 has **three screen modes**, held by the window object `CSWindowImp`:
  - windowed;
  - borderless full screen (the window resized to the desktop);
  - **exclusive full screen** (`IDXGISwapChain::SetFullscreenState(TRUE, output)`).
- It has **two resolution lists**:
  - a fixed 16:9 list for windowed mode;
  - the output's DXGI display modes for full screen.
- **Mode changes are deferred:** the settings code queues a request on GX's per-swap-chain
  record, and GX applies it at the start of the next frame (§4.3). The order of operations is
  `ResizeTarget` / `SetFullscreenState` / `ResizeTarget`, then `ResizeBuffers` on `WM_SIZE`,
  then the listeners.
- The swap chain has a **gamma pass**, which the shipped game always enables (§9):
  - an intermediate render target;
  - a 1025-entry ×3 R32_FLOAT curve;
  - drawn into the back buffer just before `Present`;
  - the curve is the **brightness option**: `x^(2.2 / (2.2 + 0.5·(v − 5)/5))`, v = 0..10.
- The native presenter on the I-0066 branch covers creation, resize and present (flip-discard,
  2 buffers, R8G8B8A8). It **does not yet** cover:
  - the mode list from the native adapter;
  - exclusive full screen;
  - deferred mode application;
  - the listener notifications;
  - the gamma pass;
  - occlusion handling.
- **Screenshots** are a debug path (MENUS-MAP §38): one readback plus WIC, optional.

## 2. Objects (STATIC)

| Object | Where | Role |
|---|---|---|
| GX display object `[4961AB0]` | `+0x10` device record (`ID3D11Device` `+0x0`, immediate context `+0x8`, K-0004); `+0x88..+0x90` array of **0x148-byte swap-chain records** (one per window; `FUN_14000C240` scans it); `+0xB18` resize listeners; `+0xB50` "mode change pending"; `+0x8B0` (`param_1[0x16]`) adapter/output list; `+0xB60` (`[0x16C]`) a GX object paused around mode changes; `+0x163*8` display-change listeners | owner of device, swap chains, modes |
| Swap-chain state | 0x3050 bytes (K-0006): `+0x0` `IDXGISwapChain*`, `+0x8` device record, `+0x10` back-buffer RTV wrapper, `+0x18` heap, `+0x20` gamma LUT SRV, `+0x28` intermediate RTV, `+0x30` back-buffer RTV for the gamma pass, `+0x38..+0x3043` 1025 RGB float curve, `+0x3044` "window association off" flag, `+0x3048` swap-effect enum | created by `FUN_14192C220` |
| Swap-chain record (in GX, 0x148 bytes) | `+0x0` swap-chain state, `+0x8` window object, `+0x10..+0x6F` cached `DXGI_SWAP_CHAIN_DESC` (from `GetDesc`, `FUN_14192C780`), `+0x118` id, `+0x11C` current full-screen flag, `+0x12C` request pending, `+0x12D` request: windowed, `+0x130/+0x134` requested width/height, `+0x138` requested output index, `+0x140` caller data | request/apply state |
| `NS_SPRJ::CSWindowImp` | `+0x8` HWND; `+0x38` window/settings object (`+0xB8` window mode, `+0xBC` screen mode: 0 windowed, 1–2 full screen); `+0x40` exclusive flag; `+0x50` desktop size; `+0x58` current client size; `+0x60` size-timer flag; `+0x68`/`+0x70` windowed/full-screen resolution; `+0x78` requested mode (1 = full screen); `+0xB0` mode-list cache | game-side window and settings |

## 3. Device and swap-chain creation (STATIC)

### 3.1 Device `FUN_14192D650` (K-0004)

- **Adapter:**
  - adapter 0 of the GX factory when an output wrapper exists (`FUN_141930E00`);
  - otherwise the default adapter;
  - driver type from `FUN_1419299D0` (0 → HARDWARE … 4 → WARP).
- **Flags:**
  - `param+0x40` → SINGLETHREADED (1);
  - `param+0x4` → DEBUG (2);
  - `param+0x41` → SWITCH_TO_REF (4);
  - `param+0x42` → PREVENT_INTERNAL_THREADING_OPTIMIZATIONS (8);
  - one of them only, in that priority.
- **Feature levels:** list {11_0, 10_1, 10_0, 9_3, 9_2, 9_1}, starting at the configured level
  (`FUN_141929A30`).
- **Debug:** `ID3D11InfoQueue::PushStorageFilter` with the configured deny list (`param+0x20`,
  count `+0x28`).
- **Then:**
  - `IDXGIDevice` → `GetAdapter` → `FUN_141933B90` (outputs enumerated) → `FUN_141930CA0`
    (adapter/output record, stored in the GX object);
  - `FUN_141934200` and **`FUN_141933200`**: the present-pass resources. Its VS and PS come
    from **embedded DXBC** at `1446BD950` (size `[1446BDC08]`) and `1446BDC10` (size
    `[1446BE030]`); `D3DGetBlobPart` keeps the VS input signature. A linear-clamp sampler,
    depth off, raster cull none.

### 3.2 Swap chain `FUN_14192C220` (from `FUN_142138EA0`, K-0005/K-0006)

1. `IDXGIDevice` → parent factory (`GetParent(IID_IDXGIFactory)`).
2. **`DXGI_SWAP_CHAIN_DESC`** from the GX descriptor:

   | Field | Value |
   |---|---|
   | BufferDesc | width, height, refresh num/den, format (`FUN_1419294A0`), scanline order, scaling |
   | SampleDesc | count, quality |
   | BufferUsage | `FUN_141929980`, plus SHADER_INPUT-class bit `0x20` when the gamma pass is on |
   | BufferCount | `+0x28` |
   | OutputWindow | `+0x30` |
   | **Windowed** | **always TRUE at creation** |
   | SwapEffect | `FUN_1419299C0(+0x44)` |

3. `IDXGIFactory::CreateSwapChain` (`vt+0x50`).
4. Unless descriptor `+0x38` (0 in the shipped game, `FUN_140006C00`): **`MakeWindowAssociation(hwnd, NO_WINDOW_CHANGES | NO_ALT_ENTER)`**
   (3). DXGI never changes modes by itself; the game does.
5. **Gamma pass** (descriptor `+0x39`, **1 in the shipped game**: `FUN_140006C00` writes `0x0100` to bytes `+0x38/+0x39`):
   - a 1025-entry curve `+0x38` initialised to identity (`i/1024` for R, G, B);
   - a 1D texture **1025 × array 3, R32_FLOAT** (`FUN_141925600`) + SRV (`+0x20`), filled by
     `FUN_14192D040`;
   - an **intermediate render target** with the descriptor's size and format (`FUN_14192D1C0`
     → `+0x28`);
   - the back-buffer RTV at `+0x30`.
   - Without the gamma pass, the back-buffer RTV is `+0x10` (`FUN_14192D360`:
     `GetBuffer(0, ID3D11Texture2D)` + RTV).

### 3.3 Present `FUN_14192CF00` (K-0005, K-0009)

1. With the gamma pass: `FUN_141933480` draws the intermediate through the curve into the
   back buffer (`IASetPrimitiveTopology`, the §3.1 shaders).
2. **`Present(syncInterval, 0)`**. The interval comes from `FUN_1400D5E30`: either the request,
   or manual QPC pacing then 0 (K-0009).
3. `DXGI_STATUS_OCCLUDED` (`0x087A0001`) is accepted silently. Other errors go to
   `FUN_14192A4D0`.

### 3.4 Resize `FUN_14192C910` (via `FUN_1421391F0`, K-0007)

1. Release the back-buffer RTV (`+0x10`), the intermediate (`+0x28`) and its RTV (`+0x30`). The
   intermediate's size and format are kept.
2. **`ResizeBuffers(count, w, h, format, flags)`**:
   - format through `FUN_1419294A0`;
   - flags from the swap-effect field (`FUN_1419299C0(+0x3048)`).
3. Recreate the RTV, or the intermediate and the back-buffer RTV.

## 4. Display modes, full screen, resolution changes (STATIC)

### 4.1 The two lists (`FUN_140EF9090(list, hwnd, fullscreen)`)

**Windowed (`fullscreen = 0`):** the fixed table at `144662D70` (12 sizes).

- The values are 800×450, 1024×576, 1152×648, 1280×720, 1440×810, 1600×900, 1680×945,
  1920×1080, 2048×1152, 2560×1440, 3200×1800, 3840×2160.
- The first is always kept. The next ones are kept while they fit in the desktop window rect
  (`GetDesktopWindow`/`GetWindowRect`).

**Full screen (`fullscreen = 1`):**

- with the GX object: **`FUN_14000C040(GX, output 0, list)`**:
  - output record → **`IDXGIOutput::GetDisplayModeList(R8G8B8A8_UNORM, 0)`** (`FUN_141932EE0`,
    count then array of 28-byte `DXGI_MODE_DESC`, converted to the GX enum);
  - width × height deduplicated.
- before GX exists: **`FUN_140EF9580`**:
  - `CreateDXGIFactory` → `EnumAdapters(0)` → `EnumOutputs(0)` →
    `GetDisplayModeList(R8G8B8A8_UNORM, 0)`;
  - every 28-byte mode kept (`FUN_140EF9D80`).
- Then: sizes below the first table entry (800×450) removed, duplicates removed. An empty list
  falls back to 800×450.

### 4.2 The game side: `CSWindowImp` (slot 2 `FUN_140EF5590`, per frame; window procedure `FUN_140EF4D10`)

- **Settings changed** (`FUN_140EF7380` compares the requested settings with `+0xB0`):
  - rebuild both lists (`FUN_140EF9090(…, 1)` and `(…, 0)`);
  - validate the requested resolution against them (`FUN_140EF6990`).
- **Screen-mode change** (`+0x78`, exclusive flag `+0x40`): `FUN_140EF1C50` sets the
  window/settings mode (0, 1 or 2) and `FUN_140EF61A0` runs. That function is encrypted
  at rest; read decrypted in I-0133 (RESULT §4): it only registers `WINDOW` debug-menu items when the debug menu exists, not
  the window's style or placement (the earlier INFERRED role was wrong).
- **Resolution or mode change:** `FUN_140EF54A0(window, resolution)`, which calls
  **`FUN_14000C240(GX, swap chain 0, windowed = (mode ≠ 1), output, &size, data)`**. That only
  **queues** the request (record `+0x12C..+0x140`, `GX+0xB50 = 1`).
  - Then `SetForegroundWindow`/`SetActiveWindow`.
- **Window procedure** `FUN_140EF4D10`:
  - **`WM_SIZE`**:
    - if the swap chain should be full screen but is not (`FUN_14000B540` →
      `GetFullscreenState`), toggle it (`FUN_14000B590`);
    - then, unless inside a size-move loop, `FUN_140EF50D0`: client size (windowed) or the
      selected mode (full screen) → `FUN_140EDAA60(w, h)` (game-side size) and
      **`FUN_14000B700(GX, w, h)`** (GX resize, §4.4).
  - `WM_ENTERSIZEMOVE`/`WM_EXITSIZEMOVE` (0x231/0x232): hold, then apply on exit.
  - `WM_SYSCOMMAND` `SC_MOVE`/`SC_SIZE` and `WM_ENTERMENULOOP`: a 10 ms timer
    (`FUN_140EF4580`) that pauses the game (`FUN_1417DC0C0(…, 1)`) during modal loops.
  - `WM_TIMER` 0x113: `FUN_140ED8E50`.
  - `WM_KEYDOWN` `'P'` (0x50): `FUN_140814280` (INFERRED debug).
  - mouse messages: `SetCursor`.
- **Focus and occlusion** (`FUN_140EF5590` tail): when the window loses focus in exclusive full
  screen, `ShowWindow(SW_MINIMIZE)`. `FUN_140EF1D00` retries the full-screen toggle every 10
  frames while the actual state differs from the requested one, logging after 5 failures.

### 4.3 Deferred application, at the start of each GX frame: `FUN_14000A570` → `FUN_140009EA0`

`FUN_14000A570` also calls `D3DPERF_GetStatus`. When `GX+0xB50` is set:

1. **Drain:** flush GX (`FUN_14000B4E0`), pause the `+0xB60` object, rebuild the display caches
   (`FUN_14000A210`, `FUN_14000A440`), resize the `+0x388/+0x390` pools.
2. For each swap-chain record with a pending request:
   1. **Mode** = `{w, h, 60/1, format 0, …}`. If windowed, `IDXGIOutput::FindClosestMatchingMode`
      on the requested output (`FUN_1419330B0`) supplies the refresh rate.
   2. If leaving full screen: **`ResizeTarget(mode)`** first (`FUN_14192CB30`).
   3. If the full-screen state must change (or the output changed):
      **`SetFullscreenState(fullscreen, output)`** (`FUN_14192CD90`; output only when going full
      screen).
   4. **`ResizeTarget(mode)`**. In full screen, with a valid size, the format is set to
      R8G8B8A8_UNORM (0x1C).
   5. Record the new state (`+0x11C`, output `+0x120`), update the window record
      (`FUN_1400D5C20`), notify the display listeners `GX[0x163]` `vt+0x28(id, fullscreen,
      data)`.
3. `ResizeBuffers` itself follows from the `WM_SIZE` that `ResizeTarget` and the mode switch
   produce (§4.2 → §4.4).

### 4.4 GX resize `FUN_14000B700(GX, w, h)` → `FUN_14000B830`

1. Flush GX.
2. Resize listeners `[GX+0xB18]`: slot 1 (release size-dependent resources), then slot 3
   (recreate). The listeners are the post-process manager, the scene context, the rendering
   system and the FFX scene control (part C).
3. `FUN_140009D20`.
4. Per swap-chain record: **`ResizeBuffers(count, w, h)`** (`FUN_14192C910`) → `GetDesc` cache
   (`FUN_14192C780`) → GX's per-window size `GX+0x198+0x20·id`.
5. `FUN_140008620` (render targets of the new size).
6. Then:
   - the Scaleform viewport (`[144961B38]+0x18` → `FUN_142307D70`, size or the current
     `+0xF8/+0xFC`);
   - the display listeners `GX[0x163]` slot 4 (`+0x20`) and slot 2 (`+0x10`) with the new size.

### 4.5 Full-screen toggle `FUN_14000B590(GX)` (from `WM_SIZE` or the retry)

- Same drain as §4.3 step 1.
- Per swap chain: `GetFullscreenState` → **`SetFullscreenState(!current, null)`**, record the
  new flag, `ShowWindow(SW_SHOW)`.
- Then `FUN_14000B700(GX, 0, 0)`: resize at the current size.

## 5. Native D3D12/DXGI specification (INFERRED design; the order and the data are the original's)

| Original | Native |
|---|---|
| Device creation `FUN_14192D650` | D3D12 device on the same adapter choice (adapter 0 / default / WARP); debug layer instead of the info-queue filter; no D3D11 device |
| Swap chain `FUN_14192C220` (`Windowed=TRUE`, `MakeWindowAssociation(NO_WINDOW_CHANGES \| NO_ALT_ENTER)`) | `IDXGIFactory4::CreateSwapChainForHwnd` on the direct queue; **FLIP_DISCARD** (D3D12 requires a flip model); **`MakeWindowAssociation(hwnd, NO_WINDOW_CHANGES \| NO_ALT_ENTER)`** like the original (today the native passes only `NO_ALT_ENTER`); `BufferCount` ≥ 2; format R8G8B8A8_UNORM; flag `ALLOW_MODE_SWITCH` for exclusive full screen |
| Gamma pass (`+0x20/+0x28/+0x30`, `FUN_141933480`) | always (§9): an intermediate RT of the back-buffer size and format, a 1025×3 R32_FLOAT texture updated from the curve `+0x38`, and a full-screen pass with the same embedded shaders (DXBC at `1446BD950`/`1446BDC10`) into the back buffer before `Present` |
| Present `FUN_14192CF00` (interval from `FUN_1400D5E30`) | `Present(interval, 0)` with the same interval; treat `DXGI_STATUS_OCCLUDED` as success (skip rendering while occluded if desired); exclusive full screen requires interval ≥ 1 or a waitable object |
| Mode lists `FUN_140EF9090` | windowed: the same 12-entry table filtered by the desktop; full screen: `IDXGIOutput::GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0)` on the **output that contains the native window** (`IDXGISwapChain::GetContainingOutput`) or output 0 of the native adapter (`EnumAdapterByLuid(device LUID)`), deduplicated by size, ≥ 800×450; the 28-byte `DXGI_MODE_DESC` records are identical, so `FUN_140EF9D80`/`FUN_14000C040`'s consumers take them unchanged |
| Mode request `FUN_14000C240` | keep the engine's request record (it is CPU); the native frame start does §4.3 instead of `FUN_140009EA0` |
| Deferred application `FUN_140009EA0` | at the start of the native frame, after the GPU is idle (fence): `ResizeTarget` (leaving full screen), `SetFullscreenState(fs, output)`, `ResizeTarget(mode)`; then the **same listener notifications** (`GX[0x163]` slot 5) so the game side sees the change |
| Toggle `FUN_14000B590` | `GetFullscreenState` / `SetFullscreenState(!fs, null)` / `ShowWindow`, then the native resize |
| GX resize `FUN_14000B700`/`B830` | wait for the GPU fence; release every back-buffer reference; `ResizeBuffers(0 or count, w, h, DXGI_FORMAT_UNKNOWN or R8G8B8A8, flags)`; recreate RTVs and the size-dependent native targets (G-buffer, HDR, LDR, post chain, half-resolution FFX targets); call the engine's resize listeners `[GX+0xB18]` slots 1 and 3 when their objects exist natively, the Scaleform viewport, and the display listeners slots 4 and 2 |
| Window procedure, focus, timers | the engine's own (`CSWindowImp`); the native only has to keep `GetFullscreenState` truthful (the engine's retry loop depends on it) |

**Rules (D-0151):** resolution, aspect and mode are inputs. Every size-dependent native resource
is recreated from the new size, and nothing is cached across a resize.

## 6. Screenshots (STATIC, MENUS-MAP §38 refined)

- `FUN_140205BA0(texture, type, path)`:
  - from `FUN_1400C4250` (GX screenshot request; chain `1400C48D0` ← `1400CC440`);
  - the type selects the extension and the D3DX format: 0 bmp, 1 jpg, 3 png, 4 dds, 10, 11,
    12 (other WIC containers), default = the caller's name;
  - **`D3DX11SaveTextureToFileW`**.
- `FUN_140223580` (from `FUN_1400FE640` and `GXSceneContext` `FUN_140120460`): DDS of a GX
  texture after a GX flush (INFERRED: debug capture of environment maps).
- Not reachable in the shipped game's normal flow (MENUS-MAP §9/§38).
- **Native:**
  - `CopyTextureRegion` into a READBACK buffer (row pitch aligned to 256), fence wait;
  - encode with WIC for bmp/jpg/png/tif/gif/wmp, or write a DDS header + rows;
  - off the frame path. Optional: implement only when a debug feature needs it.

## 7. D3D11 sites outside GX and their specification status (MENUS-MAP §34 A/B/C + §38 + this pass)

| Site | Status |
|---|---|
| Scaleform HAL | MENUS-MAP §36 |
| YEBIS `PPFX::IGPUDevice` (+ `D3DReflect` in `FUN_1417715B0`) | I-0068 part A (reflection: the native HAL reads DXBC reflection itself) |
| Bink movies, movie texture binding | MENUS-MAP (Bink section, commit `5ae2e97a`) |
| FaceGen vertex patch `FUN_14051B940` and its helpers **`FUN_140D3F7C0`/`FUN_140D3FA10`** | §38 + this pass: per FLVER mesh, a dynamic vertex buffer of `stride × count` for the stream with the position semantic (`1419254F0`, usage dynamic, bind vertex buffer, CPU write), bound in place of that stream at draw time (`FUN_140D3FA10`). Native: a per-instance upload-ring copy of the morphed stream, bound as the VB view of that stream |
| FaceGen texture, view clear, GUI shader binding | MENUS-MAP §38 |
| GX FFX (`36FD70`, `385F70`, `396F30`, `39E900`) | I-0068 part D |
| Display modes, full screen, resize, swap chain, Present, gamma pass | **this note** |
| Screenshots `140205BA0`, `140223580` | this note §6 |
| **`D3DGetBlobPart`** `FUN_141930420` | GX vertex-shader creation (also `FUN_141927650`) keeps the input signature for `CreateInputLayout`. Native: not needed (D3D12 input layouts take no signature); covered by the native programme loading |
| `D3DPERF_GetStatus` (`FUN_14000A570`) | profiler check at GX frame start (the same function runs the deferred mode application); native: none |
| Legacy D3D9 `DLGR` shaders `FUN_1418EE2F0` | unreachable (no D3D9 device) |
| `FSHKPlusDC` | abstract interface, no implementer found (MENUS-MAP §34) |

**Not covered anywhere yet:**

1. ~~The gamma pass shaders~~: disassembled and specified in §9.
2. ~~**`FUN_140EF61A0`**: window style and placement~~: read decrypted in I-0133 (RESULT §4), a debug-menu registration
   (`WINDOW` items), nothing to replace.

## 8. Runtime checks for the pilot (read-only)

| Check | Read | Expectation |
|---|---|---|
| Gamma pass in use | swap-chain state `+0x28`, `+0x20` (original) | non-null (STATIC §9: always enabled) |
| Brightness | `[[1447572B8]+0x58]+0xA` and `[144796298]+0x94` in game | v and (v−5)/5; GX `+0x3F0..+0x404` = 2.2 ×3, 2.2 + 0.5·b ×3 |
| Mode lists | `CSWindowImp+0xB0` cache after opening the graphics options | windowed list = table entries ≤ desktop; full-screen list = the output's modes |
| Request record | GX record `+0x12C/+0x12D/+0x130/+0x134/+0x138` after changing resolution or mode in the options | queued; `GX+0xB50 = 1` until the next frame |
| Toggle consistency | `CSWindowImp+0x38 → +0xBC` against `GetFullscreenState` | equal after the retry loop |

## Exports

`decompiled/DS3-1152-F-<RVA>` for `0192D650`, `0192C220`, `0192C910`, `0192CF00`, `0192D360`,
`01933200`, `01933480`, `01930420`, `00EF9090`, `00EF9580`, `00EF5590`, `00EF4D10`, `00EF50D0`,
`00EF54A0`, `0000A570`, `0000B590`, `0000B700`, `0000B830`, `00009EA0`, `0000C040`, `0000C240`,
`0192CD90`, `0192C780`, `01932EE0`, `00205BA0`, `00223580`, `00D3F7C0`, `00D3FA10`; §9: `0192CBD0`, `0192D040`, `000063D0`, `000064D0`, `0000A7B0`, `0000B9D0`, `000C83D0`, `000C8590`, `00DFDD20`, `00D55190`, `0062C010`, `005A0BE0`, `00DFC850`, `00006C00`.
Queries: `evidence/I-0068/queries/display/`.

## 9. Gamma pass: shaders, curve, D3D12 (STATIC; second request of 2026-10-10)

### 9.1 Enabled in the shipped game

`FUN_140006C00` (GX initialisation) fills the 0x58-byte swap-chain descriptor passed to
`FUN_142138EA0`:

- width and height; refresh 60/1; format `0x1C` (R8G8B8A8_UNORM); sample count and quality;
  usage 1; buffer count; HWND;
- bytes `+0x38/+0x39` = `0x00/0x01`: window association **on**, gamma pass **on**;
- `+0x44` = 1 (swap-effect enum).

So every frame goes through the gamma pass: GX renders into the intermediate target `+0x28`,
and the pass writes the back buffer.

### 9.2 Shaders (embedded DXBC, file offsets `0x46BB950` / `0x46BBC10`)

Disassembly: `evidence/I-0068/queries/display/gamma/gamma-{vs,ps}.asm` (`D3DDisassemble`).

**VS** (vs_5_0, input `SV_VertexID` only, no input layout):

`
x = ((id & 1) << 1) − 1 ;  y = (id & ~1) − 1 ;  pos = (x, y, 0, 1)
id 0..3 → (−1,−1) (1,−1) (−1,1) (1,1)   : a full-screen quad as a 4-vertex triangle strip
`

**PS** (ps_5_0):

- resources `t0` = `g_src` (Texture2D float4, the intermediate);
- `t1` = `g_rgbTable` (Texture1DArray float, the curve);
- `s0` = `g_sampler`;
- no constant buffer.

`
c   = g_src.Load(int3(floor(SV_Position.xy), 0))          // exact texel, no filtering
out = ( g_rgbTable.SampleLevel(s0, float2(c.r, 0), 0),
        g_rgbTable.SampleLevel(s0, float2(c.g, 1), 0),
        g_rgbTable.SampleLevel(s0, float2(c.b, 2), 0),
        c.a )
`

- Each channel goes through its own slice.
- The coordinate is the normalised value, so the linear filter reads texel `c·1025 − 0.5`
  while entry i was computed for `x = i/1024`. That is an offset of at most half a texel.
  Reproduce it as is: same texture, same sampler.

### 9.3 Draw state (`FUN_141933480`, created by `FUN_141933200`)

| State | Value |
|---|---|
| input layout | none; topology `TRIANGLESTRIP` (5); `Draw(4, 0)` |
| render target | swap-chain back buffer (`+0x30`), no depth |
| blend | null (disabled, write mask RGBA) |
| depth-stencil | depth off, write mask 0, stencil off (`CreateDepthStencilState`, stencil ref 0xFF) |
| rasterizer | solid, **cull none**, depth clip **off**, scissor off, no MSAA |
| sampler `s0` | `MIN_MAG_MIP_LINEAR` (0x15), address CLAMP ×3, bias 0, anisotropy 1, comparison NEVER, border 0, LOD −FLT_MAX..FLT_MAX |
| viewport / scissor | (0, 0, w, h), depth 0..1 |

Afterwards the GX state cache is restored (`FUN_1419203F0` … `FUN_141920F40`).

### 9.4 The curve

- **Generator** `FUN_1400C83D0` (callback vtable `142979790`, slot 0 `FUN_1400C8590`):

  `
  for i in 0..1024:  x = i / 1024
     curve[i].c = clamp( pow(x, src_c / dst_c), 0, 1 )      for c in R, G, B
  `

- **Inputs**, six floats at GX `+0x3F0..+0x404`: `src` R, G, B, then `dst` R, G, B.
  - GX init sets them to 2.2 (`FUN_1400066F0`).
  - `FUN_14000B9D0` copies new values and raises GX `+0x3E3`.
- **Per-frame value** (`SprjDrawStep` slot 24 `FUN_140D55190` → `FUN_140DFDD20`, on the
  object `[144796298]`, created by `FUN_140DFC850`):

  `
  dst = 2.2 + k · b        k = [+0x98] = 0.5 (constructor)
  b   = [+0x94], then [+0x94] = 0        src = 2.2
  `

- **b** comes from `FUN_14062C010` (in-game update):
  - `b = clamp(f(v), −1, 1)`, with `f = FUN_1405A0BE0`;
  - v = byte `+0xA` of the option block `[[1447572B8]+0x58]`;
  - `f(v) = (v − 5)/5`;
  - INFERRED: v is the game's brightness option, 0..10, default 5.
- **Result:** `exponent = 2.2 / (2.2 + 0.1·(v − 5))`.
  - v = 5: identity.
  - v = 10: exponent 0.815 (brighter).
  - v = 0: exponent 1.294 (darker).
  - When `FUN_14062C010` does not run (outside the game world), b = 0, so identity.
- **Upload path:**
  1. `FUN_14000A7B0` (GX end of frame), on `+0x3E3`, calls `FUN_1400064D0`. That fills
     the 1025×RGB curve into the present object, sets dirty `+0x301C`, and clears `+0x3E3`.
     A deferred variant runs when GX is multithreaded (`+0x80`).
  2. `FUN_1400063D0` (present, before `FUN_1400D5CB0`) calls `FUN_14192CBD0` per swap
     chain when dirty.
  3. `FUN_14192CBD0` copies 0x300C bytes into `+0x38`. It de-interleaves R, G, B into the
     three slices with `UpdateSubresource` (4100 bytes each).
  - The request is raised every in-game frame, so the curve is regenerated (3×1025 `pow`)
    and uploaded every frame.
- This replaces `IDXGIOutput::SetGammaControl` (same 1025-entry RGB layout as
  `DXGI_GAMMA_CONTROL.GammaCurve`), so it also works in windowed mode.

### 9.5 D3D12 equivalent (INFERRED design; data and order are the original's)

- **Root signature:**
  - one descriptor table, SRV t0..t1 (pixel visibility);
  - one static sampler s0 with the §9.3 descriptor;
  - flags: `DENY_VERTEX/HULL/DOMAIN/GEOMETRY_SHADER_ROOT_ACCESS`;
  - no input-assembler layout flag.
- **PSO:**
  - VS and PS: the two embedded DXBC blobs as is (D3D12 accepts SM 5.0 DXBC);
  - empty input layout; primitive type TRIANGLE;
  - one RTV, the swap-chain format (R8G8B8A8_UNORM); DSV UNKNOWN;
  - blend disabled, write mask ALL; rasterizer solid, cull none, `DepthClipEnable = FALSE`;
  - depth-stencil disabled; sample count 1.
- **Resources:**
  - the intermediate RT: swap-chain size and format, recreated on every resize (§3.4). It is
    the target of the native frame's final composite, game and UI.
  - the curve texture: `Texture1DArray` 1025 × 3, R32_FLOAT, default heap. Update it through
    an upload buffer: `CopyTextureRegion` per slice, row pitch 4100 aligned to 4352. Upload
    only when the six inputs change; the curve is a pure function of them (INFERRED: this
    removes the per-frame `pow` and upload without changing the result).
- **Per frame**, after the UI and before `Present`:
  1. Barriers: intermediate `RENDER_TARGET → PIXEL_SHADER_RESOURCE`; back buffer
     `PRESENT → RENDER_TARGET`.
  2. Set the PSO, root signature, descriptor table, viewport and scissor (w×h); topology
     `TRIANGLESTRIP`; `DrawInstanced(4, 1, 0, 0)`.
  3. Barriers back: back buffer → `PRESENT`; intermediate → `RENDER_TARGET`.
- **Engine-side inputs stay the engine's:** `FUN_14062C010`, `FUN_140DFDD20`,
  `FUN_14000B9D0` and the generator are CPU code. The native only replaces `FUN_14192CBD0`
  (upload) and `FUN_141933480` (draw).