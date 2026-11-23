# I-0076 ER study: display modes, swapchain and resize in Elden Ring 2.7.1 (2026-10-20)

Elden Ring 2.7.1 (`eldenring_2.7.1.0_25080141_1A354710.exe`), static only: read-only headless Ghidra 12.1.3 at below-normal
priority on the analysed copy `C:/Users/User/Tools/DS3-Decompilation/er-mirror-i0072-b` (`-readOnly -noanalysis`), no ER launch,
no DS3 claim anywhere in this file. The raw C/ASM exports and the small Java scripts stay in the session scratchpad
(`w76/out`, `w76/scripts`), not in the repository. Addresses are ER absolute VAs. Status: OBSERVED (read in the decompile or the
assembly), INFERRED (a reading of the code). DXGI and D3D12 calls are named by their vtable slot, counted from the interface
layouts and confirmed against the arguments in the assembly. A claim about the *absence* of an import rests on the import
scan of section 9. What static reading could not settle is listed in section 8.

## 0. The short answer

- **Three modes, one setting.** `ScreenMode` in `GraphicsConfig.xml`: `WINDOW` = 0, `FULLSCREEN` = 1, `BORDERLESS` = 2
  (string table 142C01320, strings 142C013D8 / 142C013E8 / 142C01400). Each mode has its own saved resolution
  (`Resolution-WindowScreenWidth/Height`, `-FullScreenWidth/Height`, `-BorderlessScreenWidth/Height`).
- **Exclusive full screen is real DXGI exclusive full screen**: the swapchain is created *windowed*, and mode 1 calls
  `IDXGISwapChain::SetFullscreenState(TRUE, output)` and `ResizeTarget`. ER does **not** restyle the window for mode 1.
- **Borderless is a windowed swapchain** in a `WS_POPUP` window moved over the monitor rectangle with `SetWindowPos`; the
  rectangle comes from `IDXGIOutput::GetDesc().DesktopCoordinates` (`GetSystemMetrics` only as a fallback). The swapchain is
  monitor-sized and the chosen resolution becomes an internal render scale.
- **Swapchain**: `IDXGIFactory::CreateSwapChain` (not `CreateSwapChainForHwnd`), `DXGI_SWAP_EFFECT_FLIP_DISCARD` (4),
  `Windowed = TRUE`, flags `ALLOW_MODE_SWITCH` (2) only, `MakeWindowAssociation(hwnd, 3)` = `NO_WINDOW_CHANGES | NO_ALT_ENTER`.
  No tearing flag, no waitable object. Alt+Enter is handled by the game itself.
- **Resize**: wait for the direct queue to be idle (signal + spin), release everything that references the back buffers,
  `ResizeBuffers`, then rebuild the render-size targets. The swapchain size and the internal render size are different
  numbers (aspect fit plus a float scale), so ER does render at a size different from the window.

## 1. The modes and the setting (question 1)

- **Persistence** (OBSERVED): the file name is the wide string `GraphicsConfig.xml` (142C01200). The key pointers sit in the
  table at 143B4D060-143B4D098: `Resolution-WindowScreenWidth` 142C01438, `Resolution-WindowScreenHeight` 142C01478,
  `Resolution-FullScreenWidth` 142C014B8, `Resolution-FullScreenHeight` 142C014F0, `Resolution-BorderlessScreenWidth` 142C01530,
  `Resolution-BorderlessScreenHeight` 142C01580, `ScreenMode` 142C015C8, then `Auto-detectBestRenderingSettings`
  (142C015E0), `FrameRateLimit` (142C01628) and the quality keys (`QualitySetting`, `TextureQuality`, `Antialiasing`, `SSAO`,
  `DepthOfField`, `ShadowQuality`, ...). Save is 140E954E0 (reads the in-memory config, maps the int through the table at
  142C01320 to the strings `WINDOW`/`FULLSCREEN`/`BORDERLESS`); load is 140E95420 -> 140E96480. The wrapper 140E92E50 builds the
  file name and calls the save; its two direct callers are 14067C3C0 and 14067C790 (the save-game paths), so the file is also
  rewritten whenever the game saves.
- **The window manager** (OBSERVED): class `CS::CSWindowImp` (RTTI `.?AVCSWindowImp@CS@@`, COL 143373490), vtable 142C011E8:
  slot 0 140E8F9D0 (destructor), **slot 1 140E8FBE0 = start-up**, **slot 2 140E92690 = per-frame tick**. One global instance,
  `_DAT_14458F910` (every use goes through a null-check helper). Fields that matter: +0x08 HWND, +0x20/+0x24 window position,
  +0x28/+0x2C current size, +0x38 the display-state object (below), +0x48 a byte that turns mode 1 into mode 2 (a launch
  option, see section 8), +0x49 "in size/move loop", +0x4C "toggle requested" (Alt+Enter), +0x58/+0x5C last applied client
  size, +0x65 "WM_SIZE seen", +0x66 monitor (output) index; +0x6C a 0x60-byte **applied** config copy (window resolution
  +0x6C, full-screen resolution +0x74, borderless resolution +0x7C, mode +0x84); +0xD0 the same struct as **staging** config
  (window +0xD0, full screen +0xD8, borderless +0xE0, mode +0xE8). 140E94510 copies staging to applied; 140E952A0 returns a
  bit mask of which fields differ (bit 0 window resolution, bit 1 full-screen resolution, bit 2 borderless resolution,
  bit 3 mode, then the quality fields).
- **The display-state object** at WM+0x38 (OBSERVED): constructor 140E8A6D0 (0xE8 bytes, vtable 142C00608). +0xA8 current
  mode, +0xAC target mode, +0xB0/+0xB4 stored window position, +0xB8 "apply requested", +0xB9 "enabled", +0xBC a frame
  cooldown, +0xC0..+0xE3 the request (9 dwords, below), default refresh +0xC8/+0xCC = 60/1, or 120/1 when the flag read from
  the string `CS.WIN64.is120FpsTest` (142BF52C8, cached by 140E5F0F0) is set. The settings facade `_DAT_14458D578` that the
  window code calls everywhere (140E75990, 140E75A60, 140E758D0, 140E75B40, 140E75AC0, 140E75AB0, 140E75AF0, 140E75A90 ...) is
  made of stubs in this build: each is `ret` or returns its own argument (`MOV EAX,EDX ; RET`), so the saved value *is* the
  field read from the window manager.
- **The options-menu side** (OBSERVED for the setters, INFERRED for the menu role): the screen-mode item writes the staging
  mode with 14078BE90 / 140E925D0 (`if (WM+0x48 && mode != 0) mode = 2; WM+0xE8 = mode`), the resolution item with 140E925A0
  (`mode 0 -> WM+0xD0`, `1 -> WM+0xD8`, `2 -> WM+0xE0`, value taken from the resolution list entry), the reset-to-defaults
  14078BD40 calls both. These sit in the menu function cluster 14078B6C0-14078BFA0. **There is no separate apply call**: the
  tick below compares staging with applied on the next frame and applies the difference.
- **Start-up** (OBSERVED, 140E8FBE0): `ImmDisableIME(0)`; read the launch option into +0x48; read the monitor index into
  +0x66 and clamp it to the number of outputs of adapter 0 (140E90C30: `CreateDXGIFactory`, `EnumAdapters(0)`, `EnumOutputs(i)`
  until failure); load `GraphicsConfig.xml` into the staging struct (140E95420); mode = staging +0xE8 (1 becomes 2 when +0x48
  is set); build the resolution lists (140E93910 -> 140E98BE0) and fit the saved resolution to them; for mode 0 the saved
  window position is reset to 0 when it lies outside the virtual screen (`GetSystemMetrics` 0x4C-0x4F); apply the mode with
  140E93180; `RegisterClassExW` (style 3, `lpfnWndProc` = 140E92FA0); `AdjustWindowRect` with the style of the mode;
  `CreateWindowExW(0, ...)` at the saved position; `ShowWindow` (SW_SHOWNORMAL, or SW_MAXIMIZE when the settings facade says
  so, which the stub never does), `UpdateWindow`, `SetForegroundWindow`, `SetActiveWindow`, `GetClientRect`.
- **Window styles** (OBSERVED, 140E8FA40 / 140E8FAD0): mode 0 = `0xCA0000` (CAPTION | SYSMENU | MINIMIZEBOX), or `0xCF0000`
  (adds THICKFRAME | MAXIMIZEBOX) when a boolean option (obfuscated getter 1455AD2EB, read as "resizable window") is true;
  mode 1 = `0x82000000` (POPUP | CLIPCHILDREN); mode 2 = `0x80000000` (POPUP). The restyle mask is `0x82CA0000` (`0x82CF0000`).

## 2. How a mode or resolution change is applied (the chain)

OBSERVED unless marked.

1. **Tick 140E92690** (every frame, WM slot 2). If the close flag `DAT_14458F925` is set it only runs the engine's end-of-tick
   (1426707C0) and returns. Otherwise, in this order: (a) mask = 140E952A0(applied, staging); (b) Alt+Enter toggle (+0x4C: if the
   current mode is not 0 the staging mode becomes 0, otherwise 1, or 2 when +0x48 is set); (c) compare the staging mode with the
   current mode of the display-state object (`140E8B310` = +0xA8, -1 meaning "none yet") and set "mode changed"; (d) copy staging
   to applied (140E94510); (e) branch on the obfuscated boolean 144D5A319 (section 8): **false**: if the current mode is 1, the
   device exists and DXGI is no longer in exclusive full screen (`1419EC180`), force "apply mode 0"; **true** (the only branch that
   applies resolution changes; INFERRED to be the retail value): "resolution changed" = the mask bit of the *target* mode's
   resolution (bit 0 for mode 0, 1 for mode 1, 2 for mode 2), and when both the current and the target mode are 1 and DXGI is no
   longer exclusive the window is minimised (`ShowWindow(hwnd, SW_MINIMIZE)`); (f) when `WM+0x65` is set (WM_SIZE), no resolution
   change is pending and the display-state object has no request in flight (virtual +0x50 < 1): 140E93D60 (user resize, section 5);
   (g) mode changed -> 140E93180(WM, newMode); (h) resolution changed -> 140E92080(WM, size from 140E91060); (i) after either, a
   notify call (144E4E360, not read).
2. **Mode change = 140E93180(WM, mode)**: builds the 9-dword request with 140E90CF0, stores it with 140E8B640
   (`DS+0xAC = mode`, request copied to `DS+0xC0..`, position to `DS+0xB0`, `DS+0xB8 = 1`; only when the virtual at +0x50
   reports no request in flight). **Resolution change = 140E92080(WM, size)**: builds the request, submits it straight to
   the renderer with `1419ECB00(device, 0, &request, 0)`, then `SetForegroundWindow` and `SetActiveWindow`.
3. **The request struct** (140E90CF0): `{width, height, refreshNum, refreshDen, windowed (= mode != 1), outputIndex,
   aspectW, aspectH, scale (float)}`. For modes 0 and 1 width/height are the chosen resolution. For mode 2 width/height are
   the **monitor** size (140E99730) and `scale = max(renderW / monitorW, renderH / monitorH)` of the chosen resolution
   (1.0 otherwise); `aspectW:aspectH` is `0:0`, or `16:9` (`0x900000010`) when the obfuscated boolean 144D5A319 is true.
4. **Display-state update 140E8B740**: while enabled and its cooldown is zero, if `DS+0xB8` is set or the exclusive-full-screen
   state differs from "mode == 1", it queues the request with `1419ECB00` (copies the 36 bytes into the swapchain entry of
   the device, sets the entry flag +0x140 and `device+0xEB8`), calls `SetActiveWindow` and `SetFocus`, clears +0xB8 and sets the
   cooldown to 10; if the window is not in exclusive full screen and `IsIconic`, it calls `ShowWindow(SW_RESTORE)`.
5. **Window restyle 140E8B8E0** (target mode in `DS+0xAC`): **for mode 1 it does nothing to the window** (the whole block
   is skipped). For mode 0: style = `(GetWindowLongW(GWL_STYLE) & ~mask) | style(mode)`, `AdjustWindowRect` on `{0,0,w,h}`,
   `SetWindowLongW(hwnd, GWL_STYLE, style)`, `SetWindowPos(hwnd, 0, savedX, savedY, outerW, outerH, 0x24)`. For mode 2: the
   rectangle from 140E99730 (the output's `DesktopCoordinates`), `AdjustWindowRect` with the popup style (a no-op),
   `SetWindowLongW`, `SetWindowPos(hwnd, 0, left, top, right - left, bottom - top, 0x24)`. `0x24` is
   `SWP_NOZORDER | SWP_FRAMECHANGED`. Then the facade stubs, the window icon (`LoadImageW`/`WM_SETICON`, once), and a virtual
   call at +0x20 (140E8BF30, bookkeeping). (INFERRED order: 140E8B740 runs, then 140E8B8E0; the call edge is hidden, section 8.)
6. **The renderer takes the request at the start of the next frame**: `1419E77F0` (per frame) calls `1419ED670` (lost
   exclusive full screen) and then `1419EF6E0` (pending requests, section 3).

## 3. Exclusive full screen and the swapchain (question 2)

- **Creation** (OBSERVED, 141E9EAC0, called from 141E994F0 from the device set-up 1419E9B60 at 1419EA3BF):

  ```
  factory = adapter->GetParent(IID_IDXGIFactory2 /*1430A4AA8*/)         // vtable +0x30
  desc = { Width, Height, RefreshRate{num,den}, Format (0x1C, or 0x18 when HDR),
           ScanlineOrdering, Scaling, SampleDesc{1,0}, BufferUsage, BufferCount (1..15, checked in 141E994F0),
           OutputWindow = hwnd, Windowed = 1, SwapEffect = 4 /*FLIP_DISCARD*/,
           Flags = (info.byte31 & 1) * 2 /*ALLOW_MODE_SWITCH*/ }
  factory->CreateSwapChain(commandQueue, &desc, &swapchain)             // vtable +0x50
  if (info.byte30 == 0) factory->MakeWindowAssociation(hwnd, 3)         // vtable +0x40
  ```

  The device set-up passes `byte31 = 1` (bit 2 added for HDR) and `byte30 = 0`, so the association is always made.
  The command queue is the device's direct queue. Width/height come from `device+0x3C8/+0x3CC`, the refresh from
  `device+0x3F0/+0x3F4` (60/1 unless `device+0x1D1` is set and the denominator is non-zero). Only `CreateSwapChain` is used: there is no call through the `CreateSwapChainForHwnd` slot
  (+0x78) on the factory at the creation site, and no `ALLOW_TEARING` (0x800) or `FRAME_LATENCY_WAITABLE_OBJECT` (0x40) flag
  is ever built (`FUN_141E90F20` returns `(flags & 1) * 2` and nothing else). The interface is the plain `IDXGISwapChain`
  layout; slot 36 (+0x120, `GetCurrentBackBufferIndex`) is called on the same object after every present and resize.
- **Entering and leaving full screen** (OBSERVED, `1419EF6E0`, the body of the per-frame request processor). It first waits
  for the GPU (`141AFC930`: `ID3D12CommandQueue::Signal` (+0x70) of fence value+1; `141AFC9A0`: poll and yield until the fence has
  passed, logging a warning above a time threshold), then for each entry with the pending flag:
  1. refresh = 60/1 unless `device+0x1D1` is set and the request has a denominator;
  2. **if full screen is requested** (`windowed == 0`): `IDXGIOutput::FindClosestMatchingMode` (+0x48, wrapper 141E99400) on the
     target output and the closest width/height/refresh replace the request (INFERRED from the use of its output buffer; the
     wrapper's return value is read as a failure flag);
  3. if the windowed flag changes and full screen is requested: **`ResizeTarget`** (wrapper 141E99790 -> 141E9F2D0, DXGI +0x70)
     with the mode descriptor (format 0, or 0x18 under the HDR condition);
  4. `GetFullscreenState` (wrapper 141E997C0, DXGI +0x58); if the state or the output is not the wanted one:
     **`SetFullscreenState(windowed ? FALSE : TRUE, windowed ? NULL : output)`** (wrapper 141E9F3C0, DXGI +0x50, the output
     taken from the device's output list `device+0x148` at the request's output index);
  5. `ResizeTarget` again (windowed: only when width and height are non-zero);
  6. when the state toggled or the render size changed (and no other resize ran meanwhile, tested with the generation
     counter `device+0xEBC`, which 1419EF2B0 increments): notify the listeners (`1419EC050`, list at `device+0xE78`,
     slot +0x08), release the entry's render targets (`1419EE270`), `ResizeBuffers` (wrapper 141E9F100, section 5), recreate
     the targets (`1419ED9B0`, section 5), notify again (`1419EC0A0`, slot +0x10 with the new internal render size).
- **Not a popup trick**: for mode 1 ER never calls `SetWindowLong`/`SetWindowPos` (section 2, item 5); the window geometry is
  whatever DXGI does. For mode 2 it is exactly the popup trick of section 2, with the swapchain staying windowed.
- **Window association** (OBSERVED): flags 3 = `DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER`. Alt+Enter is taken by the
  game: 140E919B0 (called from the window procedure) matches `WM_SYSKEYDOWN` (0x104), `VK_RETURN`, Alt held (`GetKeyState`
  0x12), and not an auto-repeat (lParam bit 30 clear), sets `WM+0x4C = 1` and swallows the message (the handler is skipped when
  the obfuscated boolean 144D5A319 is true and either the current or the staging mode is 2).
- **Destruction** (OBSERVED, 141E9ED90): vendor display-mode callback off if it was on, then `SetFullscreenState(FALSE, NULL)`
  before releasing the buffers and the swapchain.
- **Present** (OBSERVED, 141E9F630, reached by 141E99800 from 141AFCB60 <- 1419EC620): before presenting it compares the
  swapchain's containing output and full-screen state (`GetFullscreenState`, `GetContainingOutput` +0x78 matched to the
  device's output list by 141E9FA00) with the last ones and calls the vendor display-mode callback `_DAT_144805398(format,
  output, enable)` when they changed; then `Present(syncInterval, 0)` (+0x40, flags **0**), accepts `S_OK` or
  `DXGI_STATUS_OCCLUDED` (0x087A0001), signals the queue's fence (+0x70) and refreshes the back-buffer index. The sync interval is an
  argument decided upstream (section 8).
- **Vendor callback** (OBSERVED structure, INFERRED purpose): the three pointers `_DAT_144805388/390/398` are filled by 141E88E40
  per vendor (index 0 default stubs 141EAFE10/20/30; 1 NVIDIA, detected by vendor id 0x10DE in 141EAFE50; 2 AMD, vendor id
  0x1002 in 141EB0400, AGS import `agsSetDisplayMode` called from 141EB06B0 at 141EB0966). The format test selects
  `R10G10B10A2` (0x18) instead of `R8G8B8A8` (0x1C), so the reading is an HDR / display-mode switch bound to entering and
  leaving full screen. It does not change the swap effect or the flags above.

## 4. The window procedure and the focus rules (question 4)

OBSERVED, window procedure 140E92FA0 (registered by 140E8FBE0), fan-out: engine hooks (142670720), 140E919B0 (Alt+Enter),
1426706A0, 1426705C0, **140E91A70**, 140E91E90, then `DefWindowProcW`.

- `WM_CLOSE` (0x10) when the game is running (`_DAT_143D75CB8 != 0`): sets `DAT_14458F925 = 1`, deletes the `SC_CLOSE` system-menu
  item (0xF060), hides the window (`ShowWindow(SW_HIDE)`), `UpdateWindow`, and calls 140DAB5E0 on a global object (INFERRED: the
  quit request); the tick 140E92690 returns early while that flag is set.
- **`WM_SIZE` (5)** in 140E91A70: stop the 10 ms timer; unless in a size/move loop (`WM+0x49`), `wParam == SIZE_MINIMIZED (1)`
  starts the timer (SetTimer id 1, 10 ms, 140E8FA10), anything else sets `WM+0x65` (resize pending); either way
  `DAT_143D87D56 = (wParam == 1)` through 140DAB820.
- **That flag pauses the simulation clock**: in the per-frame update 140DAC0A0, if `DAT_143D87D56` is set the world's time scale
  (+0x344) is driven to 0.0 through 140DAB6E0, otherwise it is restored (INFERRED reading of the scalar as the time scale; the
  branch is OBSERVED). `WM_TIMER` (0x113) in 140E91A70 calls 140DEB2A0 (a small update that keeps a few systems running while
  minimised or in a modal loop). 140E91E90 additionally swallows `SC_SCREENSAVE` and `SC_MONITORPOWER` and turns its own 50 s timer
  into a zero `SendInput`.
- **`WM_ACTIVATEAPP` (0x1C)**: only the *activation* is handled. When `wParam != 0`, the current and target modes are both
  non-zero, the device exists and DXGI is **not** in exclusive full screen, ER builds a request for the current size
  (140E91060 + 140E90CF0) and queues it with `1419ECB00` (re-enter full screen); the stored size is updated. Deactivation falls to
  `DefWindowProcW`; DXGI and the driver leave exclusive full screen by themselves.
- **After DXGI left exclusive full screen** (Alt+Tab, minimise): `1419ED670` (per frame, from 1419E77F0) finds the entry whose
  `GetFullscreenState` is FALSE while the entry still says "not windowed", waits for the GPU, notifies listeners, releases the
  entry targets, `ResizeBuffers` to the swapchain's current size (re-read with `GetDesc`), marks the entry windowed (+0x12C = 1)
  and recreates the targets. In the same spirit the tick (section 2, item 1) minimises the window (boolean true) or applies
  mode 0 (boolean false), and 140E8BD80 forces the *target mode* to 0 when the current mode is 1 and DXGI is no longer exclusive
  (its caller is not known, section 8).
- `WM_ENTERSIZEMOVE` (0x231) sets `WM+0x49`; `WM_EXITSIZEMOVE` (0x232) clears it, kills the timer and sets `WM+0x65`.
  `WM_ENTERMENULOOP` (0x211), `SC_MOVE` and `SC_SIZE` start the 10 ms timer; `WM_EXITMENULOOP` (0x212) stops it. `WM_SETCURSOR`
  restores the game cursor. `WM_DROPFILES` is forwarded to a handler.
- **Cursor clipping** (OBSERVED): 140E206A0 / 140E20930 call `ClipCursor` with the client rectangle converted by `ClientToScreen`
  while the window has focus and the game wants the cursor held, and `ClipCursor(NULL)` otherwise.
- **No presentation pause by occlusion test**: no `Present(..., DXGI_PRESENT_TEST)` and no occlusion check were found; occluded
  presents are simply accepted (`0x087A0001`). The game "pauses" by the time scale above, not by skipping the frame.

## 5. Resolution change at run time and the render size (question 3)

- **User resizes the window** (OBSERVED, 140E93D60, called by the tick when `WM+0x65` is set and no request is in flight):
  take the client size (`GetClientRect`) when 144D5A319 is false; when it is true, the output size for mode 2 and otherwise the
  fitted resolution-list entry (140E99C20);
  when it differs from the last (`WM+0x58/+0x5C`) store it at `WM+0x28/+0x2C` and call **`1419E8070(device, w, h)`**; for
  mode 2 re-run 140E93180; then `SetForegroundWindow`, `SetActiveWindow`, remember the window rectangle for windowed mode.
- **1419E8070** (OBSERVED): `GetCurrentThreadId` check, then `141AFC930` + `141AFC9A0` (**wait for the GPU first**), then
  **1419EF2B0(device, w, h)**: for each entry, notify the listeners (slot +0x08 then +0x18), release the entry targets
  (1419EE270), call **`ResizeBuffers`** through 141E9F100, re-read the swapchain description (141E9EF70 = `GetDesc`, DXGI +0x60)
  into the entry and into `device+0x3C8/+0x3CC`, then `1419ED9B0(device, -1)` to recreate the targets. Afterwards
  `1419EC110` / `1419EC0A0` publish the new size, and the size they publish is the **internal render size** of entry 0
  (`+0x108/+0x10C`), not the swapchain size (also to the object at `_DAT_1447F33E8`, 141D94500).
- **ResizeBuffers wrapper** (OBSERVED, 141E9F100): releases every per-buffer wrapper first (loop over the entry's buffers), frees
  and reallocates the buffer array when the count changes (0x68 bytes per buffer), then
  `ResizeBuffers(count, width, height, format, flags = (f & 1) * 2)` (DXGI +0x68; the callers pass the entry's current format and
  the entry's buffer count), re-wraps each buffer with 141E9F8A0 (one wrapper per back buffer; the `GetBuffer` call was not read)
  and refreshes the back-buffer index. The flags keep `ALLOW_MODE_SWITCH` on every resize.
- **Swapchain size is not the render size** (OBSERVED, `1419ED9B0`, the target rebuild). Inputs per swapchain index:
  `device+0x3C8/+0x3CC` (swapchain width and height), `+0x3E0/+0x3E4` (aspect constraint, 16:9 or 0:0) and `+0x3EC` (a float
  scale). The internal size is computed as:
  - no constraint: `(W, H)`; with a constraint `a:b`: if `a*H == b*W` then `(W, H)`, if `a*H < b*W` then `(a*H/b, H)` (pillarbox),
    otherwise `(W, b*W/a)` (letterbox);
  - then each dimension is rounded up to a multiple of 4, multiplied by the float scale, truncated and rounded up to a multiple
    of 4 again; the result is stored at entry `+0x108/+0x10C`;
  - the entry also gets the centring offsets `(W - w) / 2`, `(H - h) / 2` as floats at `+0x110/+0x114` (negated when the render
    size is larger) and the render size as floats at `+0x118/+0x11C`, which is the data for the final scale/blit to the back buffer;
  - the targets of that size (colour textures, the depth/auxiliary targets, a `0x1C`-format target) are recreated for every
    buffer; the swapchain back buffers themselves are only re-wrapped.
- **So ER does render at a size different from the window**: in windowed and full-screen modes the scale is 1.0 and the render
  size equals the swapchain size (modulo the aspect fit); **in borderless mode the swapchain is monitor-sized and the saved
  resolution only sets the float scale** (`max(renderW / monitorW, renderH / monitorH)` computed by 140E90CF0). Whether the final
  step is a plain stretch or a filter was not read (section 8).
- **Resolution lists** (OBSERVED, 140E98BE0(list, hwnd, type, output, flag)): type 0 a fixed table (140E992E0); type 1 the
  output's mode list, from `IDXGIOutput::GetDisplayModeList` (+0x40, format 0x1C) either directly (140E99850, before the device
  exists) or through the device (1419E9610), filtered by a minimum size (`_DAT_14458F9F8/9FC`), de-duplicated and sorted
  (140E98920); type 2 only the sizes whose aspect ratio equals the monitor's, scaled from the mode list, all tagged 120000/1000.
  In 1419E9610 the refresh numbers are dropped from the list unless `device+0x1D1` is set.

## 6. Monitor selection and the refresh rate (question 5)

- **Monitor** (OBSERVED): the index is the byte `WM+0x66`, read from a (obfuscated) option getter at start-up (144F0DD42), clamped to
  `count - 1` where `count` is the number of `IDXGIOutput`s of **adapter 0** (140E90C30). The display-mode enumeration uses the same
  adapter 0 (140E99850: `CreateDXGIFactory`, `EnumAdapters(0)`, `EnumOutputs(index)`). The rectangle of the selected output is
  `DXGI_OUTPUT_DESC.DesktopCoordinates` read through the device's output list (140E99730: `FUN_141E93EC0` + `IDXGIOutput::GetDesc`,
  +0x38, wrapper 141E99190), with `{0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)}` as the fallback before the
  device exists. The device-side output list is `_DAT_1448052C0`. The adapter the device itself uses is chosen by 141EAE580 /
  141EAE970 (`CreateDXGIFactory1`, `EnumAdapters1(index)` with the index passed by the caller).
- **Imports that are absent** (OBSERVED, section 9): no `MonitorFromWindow`, `GetMonitorInfoW`, `EnumDisplayMonitors`,
  `EnumDisplaySettings`, `ChangeDisplaySettings*`, no DPI-awareness calls, no `MakeWindowAssociation` import (it is a vtable call).
  All monitor work is DXGI.
- **Refresh rate** (OBSERVED): the request carries `{num, den}` taken from the selected mode-list entry for mode 1 (the pair at
  list +0x08), from the borderless table's 120000/1000 tag for mode 2, and from the display-state default 60/1 (120/1 under
  `CS.WIN64.is120FpsTest`). The renderer, however, uses **60/1 unless `device+0x1D1` is set** (1419EF6E0, and the swapchain
  creation in 1419E9B60), so by default a full-screen request asks for 60 Hz and `FindClosestMatchingMode` returns the nearest
  supported mode. The save file has a `FrameRateLimit` key; it is a limiter, not a mode selector (INFERRED from the name and from
  its position among the quality keys; not traced).

## 7. What this suggests as a design shape (INFERRED, no DS3 claim)

- Keep three concepts apart: a **config** (mode, per-mode resolution, monitor), a **request** (nine numbers), and a **renderer
  entry** that owns the swapchain, the output list and the targets that depend on the size. Only the renderer touches DXGI.
- Apply requests **at one point per frame, at the top of the render frame**, after the queue is idle; do not resize from the
  window procedure. The window procedure only sets flags (size changed, minimised, Alt+Enter) and the main thread acts on them.
- Exclusive full screen needs `ALLOW_MODE_SWITCH`, a windowed creation, `ResizeTarget` before and after `SetFullscreenState`, and the
  output pointer; borderless needs none of that, only a popup style and the monitor rectangle.
- Decouple the **internal render size** (aspect fit and a float scale) from the **swapchain size**; recreate only the former's
  targets and re-wrap the back buffers on resize.
- A re-activation handler and a "DXGI left full screen behind our back" check are both required, because losing focus drops
  exclusive full screen without a message the game owns.

## 8. Not settled statically

- **Obfuscated option getters.** The calls through `_DAT_14458CE68` land on jump thunks (140E5CB10, 140E5E5E0, 140E5F9D0,
  140E5FAF0, 140E5FC60, 140E60340) that continue into control-flow-obfuscated code (1450CA3E3, 144F0DD42, 1452B1472, 1455AD2EB,
  14570B7BC, 144D5A319; their push/ret style hides the final targets such as 14550CEEB and 144DE8390 from the analysis). Their
  roles are read from use only: 1452B1472 -> `WM+0x48` (turns mode 1 into mode 2: a "borderless instead of exclusive" switch);
  14570B7BC -> start-up default mode `1 + (WM+0x48 != 0)`; 144F0DD42 -> the monitor index `WM+0x66`; 1455AD2EB -> the resizable
  window style (0xCF0000) and, in 140E90CF0, the use of the raw size instead of a list entry; 1450CA3E3 -> selects between two
  start-up variants in 140E8FBE0; 144D5A319 -> true selects the resolution-list path (list snapping, the 16:9 constraint in the
  request, the mask-based resolution apply in the tick, the Alt+Enter exception for mode 2), false the free-size path. Their option
  *names* and retail values are not known (the true value of 144D5A319 is INFERRED from the fact that only that branch can apply
  a resolution change).
- **The call edge to 140E8B740 and 140E8B8E0.** Slot 24 (+0xC0) of the display-state vtable 142C00608 is 140E8C040, which is a
  single `JMP 144D8F035` into obfuscated code; the tick calls that virtual (140E92690 at its start). No direct reference to
  140E8B740 or 140E8B8E0 exists. Who sets `DS+0xB9` is therefore also open, as is whether `IsIconic -> SW_RESTORE` in 140E8B740
  runs every frame or only during a mode change.
- **Back-buffer count** (creation info +0x24) and the **sync interval** passed to `Present` come from callers not traced
  (1419E9B60's parameters; 1419EC620's arguments).
- **What exactly happens on Alt+Tab out of exclusive full screen** is derived from the two recovery paths above; their
  interleaving with DXGI's own window changes cannot be proven without a run. In particular the staging mode keeps 1 while the
  current mode is forced to 0, so the tick may ask for full screen again on the following frame; whether that is rejected by
  `SetFullscreenState` while inactive was not followed.
- **The final scale/blit from the render size to the swapchain** (entry +0x110..+0x11C) was not read.
- **DPI awareness** may come from the executable manifest; only the import table was checked.

## 9. Method notes and address index

- Import scan: `ERImp.java` listed the imports with their referencing functions: `CreateDXGIFactory` (user functions 140E99850,
  141EAFE50, 141EB0455, 140E90C30), `CreateDXGIFactory1` (141EAE580, 141EAE970), `SetWindowLongW` (140E8B8E0), `SetWindowLongPtrW`
  (1422ABB00, 1422ABDB0, 1422383C0, 1422384E0: engine-side, not the display path), `RegisterClassExW` / `CreateWindowExW`
  (140E8FBE0, 142670860), `AdjustWindowRect` (140E8B8E0, 140E8FBE0, 142670860), `IsIconic` (140E8B740), `SetWindowPos`
  (140E8B8E0 only), `ShowWindow` (140E8B740, 140E8FBE0, 140E93180, 140E92690, 140E92FA0, 142670860), `ClipCursor` (140E206A0,
  140E20930). None of the others in the search set exists in the import table.
- Key addresses: window manager vtable 142C011E8 (start 140E8FBE0, tick 140E92690); display-state vtable 142C00608; apply mode
  140E93180; apply resolution 140E92080; request builder 140E90CF0; queue 1419ECB00; restyle 140E8B8E0; update 140E8B740;
  window procedure 140E92FA0 / 140E91A70 / 140E919B0 / 140E91E90; user resize 140E93D60; device frame start 1419E77F0; lost
  full screen 1419ED670; request processor 1419EF6E0; resize-all 1419EF2B0 (from 1419E8070); target rebuild 1419ED9B0; GPU wait
  141AFC930 + 141AFC9A0; swapchain create 141E9EAC0; wrappers: Present 141E9F630, ResizeBuffers 141E9F100, ResizeTarget 141E9F2D0 /
  141E99790, SetFullscreenState 141E9F3C0, GetFullscreenState 141E997C0, GetDesc 141E9EF70, FindClosestMatchingMode 141E99400,
  destructor 141E9ED90; output rectangle 140E99730; mode list 140E99850 / 140E98BE0 / 1419E9610; output count 140E90C30;
  config save 140E954E0, load 140E95420 -> 140E96480; vendor callbacks 141E88E40, 141EAC340, 141EB06B0.
