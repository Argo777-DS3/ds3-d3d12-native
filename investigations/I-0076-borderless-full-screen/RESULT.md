# I-0076 result: full screen, resolutions and the borderless window through the native presentation

Complete (2026-10-20). Runs `evidence/I-0066/artifacts/i74-native-40..52-*` (the I-0074 series runner, scenes `m45full`,
`m45window`, `m45full1440`, `m45cycle`, `m45borderless`, `m45`), builds `evidence/I-0076/artifacts/i76{b..g}-build`; static
reads `evidence/I-0076/artifacts/ghidra-{a..n}`; the ER study [ER-STUDY](ER-STUDY.md). Started in the thread « V2 : textures
hors du fil moteur », finished in a fresh thread from its checkpoint 21fa8f41.

## Conclusion

H1 and H2 confirmed; the crash of WAVE1-CLOSURE 2.3 (Apply full screen, an access violation at DarkSoulsIII+0xB544) is gone.

- **The GX display's role, natively (H1, after ER).** `FUN_14000B540` answers the native swap chain's full-screen state,
  `FUN_14000B590` toggles it, and CSWindowImp's request (`FUN_140EF54A0`, hooked after its own part) is queued and applied at
  the next engine frame by `SceneRenderer::ApplyDisplayMode` in the original's order (DISPLAY-SWAPCHAIN-NATIVE 4.3: leaving
  full screen, `ResizeTarget` first; `SetFullscreenState(full screen, the containing output)`; `ResizeTarget` to the mode,
  in full screen the output's closest mode). Swap chain `ALLOW_MODE_SWITCH`, `MakeWindowAssociation(NO_WINDOW_CHANGES|
  NO_ALT_ENTER)` as the original and ER: the engine's own window code (style, placement, retry, focus) runs unchanged.
- **The output follows the engine's size S (H2, D-0151).** S (image+46601E8, written on WM_SIZE and by the options) becomes the
  output extent at the start of the next engine frame: the render worker quiesced and its last frame finished, the
  presentation and its targets resized, then the scene contexts and views given the new size (`no-output-follow.flag` A/B).
  A movie's display rectangle from the former extent is fitted to the new one the frame after a shrink (D3D11 clipped it).
- **The options' Apply** (STATIC, ghidra-h..k): CSWindowImp is `[14489F118]`; the graphics menu's setters write `+0xC0` full
  screen (`FUN_140B07C40`), `+0xB0` / `+0xB8` the windowed and full-screen resolutions (`FUN_140B07CE0`); CSWindowImp's frame
  step `FUN_140EF5590` compares them with `+0x68`, validates the resolution (`FUN_140EF6990`), sets the mode and requests.
  `display-cycle.flag` (test) writes them as the setters do at fixed frames.
- **Borderless (D-0170), DS3's own mode 2.** `ScreenMode` has two tokens (WINDOW, FULLSCREEN), but CSWindowImp `+0x40` turns
  full screen into screen mode 2 (ghidra-l..n, as ER's WM+0x48): `FUN_140EF5590` and the start-up `FUN_140EF46A0` set the
  display state's target mode to `1 + (+0x40 != 0)` (`FUN_140EF1C50`, display state `+0xBC`); the display state's apply
  `FUN_140EF1DC0` restyles the window as a popup (`FUN_140EF45D0(true, true)` = 0x80000000) at the screen size; the retry
  `FUN_140EF1D00` asks for no exclusive state when the target is 2. The start-up reads `+0x40` from an encrypted launch-option
  getter (`FUN_140ED1080`, unreadable at rest). Natively, `borderless.flag` (the launcher's `-Borderless`,
  `tools/Launch-DS3-DX12-Borderless.cmd`) sets `+0x40` after the start-up (hook of `FUN_140EF46A0`, and at the presentation's
  start) and the target mode 2 for a full-screen configuration; a request in mode 1 with `+0x40` is borderless instead of
  exclusive (the original asks GX for windowed = mode != 1, which with `+0x40` would contradict its own retry); the
  presentation leaves exclusive full screen if needed and places the window (the engine's popup) over the monitor it is on
  (the containing output's desktop rectangle, ER 140E8B8E0). A placement that leaves the window's size unchanged sends no
  WM_SIZE, so the engine's window procedure is given the client size then (run 47: S stayed 1280x720 in a 1920x1080 client).
  The full-screen resolution chosen in the options is not used in borderless: the output is the monitor's size (ER uses it
  as an internal render scale; DS3 has none).

| Run | Build | Result |
|---|---|---|
| 40 m45full (Thomas's config: full screen 1920x1080) | i76b | started in full screen: toggle, output 1280x720 -> 1920x1080; no fault, 0 validation message (9,866 frames) |
| 41 m45window (1600x900) | i76b | rendered at 1600x900; no fault, 0 message |
| 42 m45full1440 (config 2560x1440) | i76c | no fault, 0 message; the game validates the request to the monitor's 1920x1080 |
| 43 m45cycle | i76c | steps 0-2 correct; step 3 (full screen -> window 1280x720) refused the frame after the shrink (a movie's former display rectangle) |
| 44 m45cycle | i76d | the five steps correct (window 1600x900, full screen, full screen, window 1280x720, full screen; style, client, S and output consistent), the rectangle fitted 8 times at the shrink; no fault, 0 message (9,674 frames) |
| 46 m45cycle | i76e | as 44 (borderless code present, flag off) |
| 47 m45borderless (full-screen config, `borderless.flag`) | i76e | no exclusive state at any step, the window a popup (0x94000000) over 0,0-1920,1080, windowed steps restyled by the engine (0x14CA0000); start rendered at S 1280x720 stretched |
| 48 m45borderless | i76f | as 47, the start at 1920x1080 (the client size given to the engine); no fault, 0 message (9,811 frames) |
| 49 / 50 m45, measure mode | i76f / main | 60.0 / 60.0 frames/s, collect 5.78 / 6.03 ms, prepare 2.84 / 2.94, record 1.85 / 1.94 (medians of 75 windows) |
| 51 m45cycle, 52 m45borderless | i76g (review) | every step as 44 and 48 (exclusive: style 0x14000000, borderless: 0x94000000 over 0,0-1920,1080, windowed 0x14CA0000; S, client and output equal); no fault, 0 message (9,645 and 9,798 frames) |

The desktop of this PC is 1920x1080: Thomas's 2560x1440 monitor is unknown to Windows when switched off (Thomas, 2026-10-20
15:41); the behaviour is generic, every size coming from the monitor and the game's validated lists.

Pre-merge review (Opus 5.5, D-0176), applied in build `i76g-build`: no High finding.
- Medium, applied: M1 a presentation change refused inside another (DXGI sends window messages synchronously; a toggle reached
  from the window procedure answers "not switched" and the engine retries, instead of locking `gpu_submissions` twice); M2 the
  scene contexts and views sized after the submission lock (lock order: `scene_view_cpu_lock` before `gpu_submissions`); M3
  exclusive full screen left before the swap chain's release at close.
- Low, applied: L1 a switch DXGI cannot make now (`NOT_CURRENTLY_AVAILABLE`, `MODE_CHANGE_IN_PROGRESS`) left to the engine's
  retry, and the borderless monitor from `MonitorFromWindow` when DXGI has no containing output (minimised); L4 a UI rectangle
  emptied by the fit not drawn; L5, L7 comments. Recorded: L2 the order is the original's (DISPLAY-SWAPCHAIN-NATIVE 4.3), not
  ER's ResizeTarget before entering; L3 a mode change resizes the targets once in `ApplyDisplayMode` (the buffers after the
  DXGI transition) and again when S changes (16-64 ms per change, event-driven, not per frame); L6 `GetFullscreenState` on the
  engine thread relies on DXGI's serialisation; L8 the borderless resolution (above).

## Uncertainty / open

- One monitor at 1920x1080 on this PC: another size, a second monitor, HDR and the 1440p monitor are not witnessed (D-0170's
  universal witnesses: 1440p, 4K, one odd resolution).
- `+0x40`'s retail source: the encrypted getter `FUN_140ED1080` returns the configuration key `SPRJ.WIN64.isFakeFullScreen`
  (read decrypted in I-0133, RESULT §4); the option stays the native flag.
- The remaster's options menu (D-0170) will carry the borderless choice; today it is a launcher entry.
