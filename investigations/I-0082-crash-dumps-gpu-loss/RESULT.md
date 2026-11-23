# I-0082 result: automatic minidumps and GPU loss

Complete (2026-10-21). Every native fault now leaves a minidump and a report that names the faulting function from the
module's PDB, written by the controller from outside the game; a removed device and a GPU hang are classified and reported
instead of closing the game silently; in play the player gets a message. The D3D12 debug layer is now asked for, never required:
the module starts on a PC without the Windows Graphics Tools. Elden Ring has none of this (I-0080 ER-STUDY 7: no minidump, no
device-removed handling, infinite waits); its tolerance of a missing debug layer is the one part copied.

## What was built ([DESIGN](DESIGN.md))

- **Debug layer opt-in** (I-0080 lead G1): `debug-layer.flag` or `gpu-validation.flag` next to the module; a missing layer is
  logged (`native_debug_layer` with `requested`, `enabled`, `gpu_validation`, `dred`) and the device opens without it.
  `Run-StartupTrace.ps1` keeps its meaning (the layer unless `-NoDebugLayer`; `-GpuValidation -NoDebugLayer` refused); the
  launcher places no debug flag. Under the layer the pipeline library is `native-pipelines.v2.debug.library` (I-0081
  observation 6: the layer refuses to store a name a library written without it holds).
- **Crash record** (Control block version 3) and the **controller as dump writer**: `MiniDumpWriteDump` out of process (thread
  info, full memory info, the memory the stacks point to, `game_bootstrap.dll`'s data segments; about 4.3 MB), then a `.txt`
  report with the faulting thread's stack named at that moment (`crash_dump.cpp`, `crashes/<journal>-crash<N>.dmp/.txt` beside
  the journal). The module writes the dump itself when no controller takes the request within 3 s.
- **Requests**: the vectored handler for fatal-class exceptions (a small-frame path for a stack overflow; handled faults inside
  the module's own guarded calls excluded); `ExitNative`, the single fault exit (the five copies of the `native_failure` catch
  became `NativeFailure`; a refusal carries the failure text it follows); `SIGABRT` for a `std::terminate` on any thread.
- **GPU loss**: DRED auto-breadcrumbs and page faults forced on before the device (unless `no-dred.flag`); at the fault exit the
  removal reason and the DRED rows (`native_device_removed`, `native_dred_list`, `native_dred_page_fault`); the 5 s completion
  deadline is reported as a GPU hang (kind 5). The queue, frame-slot, chunk and ingress lists are named for the breadcrumbs.
- **Player message** (play mode only): a French message box after the game closed on a native fault exit or of a recorded
  exception; the launcher keeps the last five dumps and copies the PDB beside the module.
- **Proof flags** (announced test runs only): `test-crash.flag`, `test-device-removed.flag`, `test-gpu-hang.flag` at the
  1,200th presented frame; [Run-FaultSeries.ps1](Run-FaultSeries.ps1) runs them.

## Observations

1. **Unit controls** (`crash_dump_test.exe`, in the build): a dump of a parked thread and of a thread inside its exception
   filter, both `MDMP`, never overwritten; the stack report names `ParkedThread` and `RaiseInFaultingThread` with file and line;
   a removed device reports its reason and DRED rows on WARP and the hardware.
2. **Live fault runs** (2026-10-21 13:33-13:38 and 14:03, build-2 then build-3, runs `i82-plain`, `i82-debug`, `i82-crash`,
   `i82-removed`, `i82-hang`, `i82b-hang`): the plain run starts without the layer (`requested:false`) and the debug run with
   it; the crash gives kind 1, `0xC0000005`, the report's first frame `StepNativeFaultTests` (02_frame_lifecycle.inc:556) above
   `FinishNativeFrame`, `Clear`, `EndFrame` and the game's own frames as `FUN_140D553C0` and so on; the removed device gives kind
   4, `DXGI_ERROR_DEVICE_REMOVED`, reason « native_render_worker_faulted: native render worker: native startup present: HRESULT
   887A0005 »; the hang gives kind 5 after the 5 s deadline, reason « native completion timeout ». Every dump was written by the
   controller.
3. **DRED records nothing for an application's removal.** After `ID3D12Device5::RemoveDevice`, with a list pending behind a queue
   wait, `GetAutoBreadcrumbsOutput` lists no node and DRED 1.2's device state is 0 (unknown), on WARP and on the RTX alike (unit
   control, and the live `i82-removed` / `i82-hang`). DRED captures GPU faults (a Windows TDR, a page fault); the first design's
   « remove the hung device to freeze the breadcrumbs » was therefore dropped: a hang Windows sees removes the device itself
   (TDR after 2 s, before our 5 s deadline) with its breadcrumbs, and the deadline catches the stalls it does not see, where DRED
   has nothing. The breadcrumbs of a real TDR were not produced here (no system TDR on Thomas's PC).
4. **DRED's cost is not measurable on the walk.** Four interleaved Firelink walks (measure mode, no debug layer, `i82-w1` to
   `w4`): frames over 17/25/34 ms 879/69/9 and 965/58/9 with DRED, 970/55/9 and 890/55/7 without; mean frame 16.8 ms in all
   four (V-sync at 60 Hz), held two refreshes 36 and 31 against 29 and 30. DRED stays on by default.
5. **The pre-existing exit heap fault** (`0xC0000374` at every quit, also in the Reference, WAVE1-CLOSURE): it is raised on a
   worker thread while the main thread ends the process, before the loader's shutdown flag: the first walks dumped it (`i82-w1`
   to `w4`, exit code 0, the faulting thread already gone when the controller tried to open it). A heap corruption now waits 3 s
   before its request: at a quit the process ends first (`i82-w5-exit`: the row `native_exit_phase_exception`, no dump, exit 0);
   in play the dump is taken. `i82-w1-dred`'s dump is kept for the audit (I-0080 lead D2).

## Uncertainty

- The breadcrumbs of a real GPU fault and the page-fault rows are unwitnessed (no TDR provoked on the shared PC).
- A heap corruption during play is dumped 3 s late (the context is kept in the exception record; other threads move on).
- A `__fastfail` (`0xC0000409`) outside the module's CRT bypasses every handler: the controller reports the exit code only.

## Reviews

Step review (Opus 5.5): ten findings, all applied (SIGABRT instead of a per-thread terminate handler, a small frame for a stack
overflow, the module's own guarded calls excluded, the fallback's state static and its writer bounded, truncating copies, an
explicit symbol path and the PDB beside the play module, `WAIT_FAILED` not a hang, the runner's refusal, dbghelp only where
used, the raw stack rows replaced by the report). Final review: see below.

## Artifacts

Runs under `evidence/I-0066/artifacts/i82-*` and `i82b-hang` (journals, controller rows, `crashes/*.txt` reports); the tested
module `evidence/I-0082/artifacts/build-3` (build-1 and build-2 removed); series logs `evidence/I-0082/artifacts/fault-series-1.log`,
`dred-ab-1.log`, `verify-1.log`. The dumps were removed at close (D-0142) except `i82-w1-dred/crashes/startup-crash1.dmp` (the
exit heap fault, for the audit's lead D2).
