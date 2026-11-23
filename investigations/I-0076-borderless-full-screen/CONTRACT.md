# I-0076 - V2: full screen and the borderless window through the native presentation (D-0170, D-0195)

Capability: C3, wave 2. Opened 2026-10-20 13:30 Paris from the I-0075 checkpoint (STATE « Next action »), in the I-0074/I-0075
thread while the fresh thread waits for Thomas's authorisation (coordinator, 13:24).

## Question

How do DS3's three screen modes (windowed, borderless full screen, exclusive full screen) and a resolution change from the
graphics options reach the native D3D12 presentation, after Elden Ring's display-mode architecture, so that applying full
screen no longer crashes and every mode and size is honoured?

## Known inputs

- WAVE1-CLOSURE §2.3 (Thomas, 2026-10-19 16:36): Apply full screen crashes, access violation at DarkSoulsIII+0xB544 (reading
  +0x88 of a null object).
- I-0068 [DISPLAY-SWAPCHAIN-NATIVE](../I-0068-static-yebis-ffx-lights-spec/DISPLAY-SWAPCHAIN-NATIVE.md): the original's GX
  display object `[4961AB0]` owns the swap chains, the deferred mode requests (`FUN_14000C240` queue, `FUN_140009EA0` apply:
  ResizeTarget / SetFullscreenState / ResizeTarget), the toggle `FUN_14000B590` and the resize `FUN_14000B700`; the game side
  is `CSWindowImp` (`FUN_140EF5590` per frame, window procedure `FUN_140EF4D10`, modes at its settings object +0xB8/+0xBC:
  0 windowed, 1 exclusive full screen, 2 borderless).
- STATIC (this investigation, `evidence/I-0076/artifacts/ghidra-{a,b,c}`): natively `[4961AB0]` is null; every caller of
  `FUN_14000B540` (GetFullscreenState of the display's swap chains, `+0x88`) tests it first except `FUN_140EF1D00`, the
  full-screen retry of `CSWindowImp`, which runs when a mode change is pending: that is the crash. The mode request
  (`FUN_140EF54A0` -> `FUN_14000C240`) and the resize (`FUN_140EF50D0` -> `FUN_14000B700`) are skipped natively, so neither
  a mode nor a resolution change reaches the native presentation (the output extent is the engine's size S read at start).
- The native presenter: flip-discard swap chain at S (DXGI stretches it into the client), `MakeWindowAssociation(NO_ALT_ENTER)`,
  a `SceneRenderer::Resize` that recreates the size-dependent targets.

## Hypotheses

- H1: the native presentation can take the GX display's role for the modes: answer `FUN_14000B540` and perform
  `FUN_14000B590` and the deferred request natively, so the engine's own window code (style, placement, retry, focus) runs
  unchanged.
- H2: a resolution change is the engine's size S changing (`FUN_140EDAA60` on WM_SIZE); the native output extent, the
  presenter and the size-dependent targets follow it at a frame boundary.

## Discriminating evidence

The ER study (`ER-STUDY.md`, branch `claude/i0076-er-display-modes`); live runs (announced) switching windowed, borderless and
full screen and the resolution from the graphics options, with no fault, the picture filling the screen at the selected size,
and 0 D3D12 validation messages.

## Exit conditions

Success: the three modes and a resolution change applied from the options without a fault, pictures at the new size, reviewed
and merged. Refuted or insufficient: recorded. Budget: 10 cycles.
