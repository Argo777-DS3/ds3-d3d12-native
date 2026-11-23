# I-0082 design: minidumps, GPU loss and the debug layer

Rule D-0195: Elden Ring's mechanism first, without its known defects. ER's side comes from
[I-0080 ER-STUDY](../I-0080-er-dx12-pitfalls/ER-STUDY.md) section 7 (ER-DEC, the debug byte checked by Opus there, the rest a
worker's reading) and section 3 (the waits); no new decompilation was needed, since ER has no mechanism for most of this.

## 1. What ER does, and its defects

| Job | ER 2.7.1 (I-0080) | ER's defect | Native design |
|---|---|---|---|
| Debug layer | enabled only when a configuration byte is set; a missing layer tolerated (null check) | none (copied) | opt-in `debug-layer.flag`, a missing layer tolerated and logged |
| Crash capture | no minidump (dbghelp only in Havok's stack tracer), CRT filter, an assertion dialog | nothing to analyse after a crash | a minidump at every native fault, written out of process by the controller, with a symbolized report |
| Device removed | never asked (`GetDeviceRemovedReason` absent), Present's result dropped, HRESULTs mapped to codes | a TDR ends in a hang or a crash, no reason | the reason read, DRED breadcrumbs and page fault logged, a dump, a message |
| GPU hang | every fence wait `INFINITE`; the frame-lag spin's hang message off | the game freezes forever | the 5 s completion deadline (existing) becomes a reported hang (kind 5), the dump and the message; a hang Windows detects (TDR) comes as a removed device with DRED's breadcrumbs |
| Message to the player | none | silent exit | a message box after the game closed (play mode) |

## 2. The debug layer (lead G1)

`20_startup_phases.inc`: `debug_layer` is true only with `debug-layer.flag` or `gpu-validation.flag` next to the module.
`DeviceQueues::Open(warp, debug)`: a failed `D3D12GetDebugInterface` no longer throws; the device opens without the layer
(no DXGI debug factory, no InfoQueue callback) and `DebugLayerActive()` says so; the start row `native_debug_layer` gives
`requested` and `enabled`. The runner keeps its meaning (`Run-StartupTrace.ps1` writes `debug-layer.flag` unless
`-NoDebugLayer`, and removes a stale `no-debug-layer.flag`); the launcher places no debug flag. Under the layer, the pipeline
library is `native-pipelines.v2.debug.library` (and its list), so a validation run never stores into a library written
without the layer (I-0081 observation 6).

## 3. The crash record and the dump (lead C2)

**Control block version 3** (`game_bootstrap_shared.h`): a `Crash` record, filled by the module and served by the controller,
the one process that survives the game: state (0 idle, 1 requested, 2 taken, 3 written, 4 failed), dumps written this run,
kind, code, thread, the address of the `EXCEPTION_POINTERS` in the game (0 when none), the device-removed HRESULT, a reason
(256 chars) and the dump path the controller chose.

**Kinds**: 1 exception (a fatal-class exception seen first-chance by the vectored handler: access violation, illegal or
privileged instruction, stack overflow, integer division by zero, in-page error, heap corruption, non-continuable), 2 native
failure (`0xD3126607`, the caught exception's text), 3 refusal (`0xD3126606`), 4 device removed, 5 GPU hang.

**Module side** (`RequestCrashDump`, heap-free, one request at a time, at most 4 a run): fills the record, sets state 1,
waits up to 3 s for the controller to take it (state 2), then up to 120 s for the dump; when no controller answers, it
writes the dump itself from a fresh thread with the same writer (the in-process fallback, report without symbols).
`ExitNative(code, reason)` is the single fault exit: before it ends the process it classifies the fault from the device
(`GetDeviceRemovedReason`, the hang flag set by `WaitOn`), logs the removal report, requests the dump, then exits as before.
The five copies of the `native_failure` catch become one `NativeFailure(what)`.

**Exit-phase exceptions** (withdrawn 2026-10-29, I-0099 STUDY step 4: DS3's exit heap fault fixed, K-0484; every fatal-class exception is dumped at once and `exit-dump.flag` is gone): once the loader reports the process shutting down (`RtlDllShutdownInProgress`), a fatal-class
exception is logged (`native_exit_phase_exception`) but not dumped unless `exit-dump.flag` is present: the heap fault
0xC0000374 at every quit is pre-existing and also in the Reference (WAVE1-CLOSURE), not a native crash. It comes before that
flag, on a worker thread while the main thread exits (RESULT observation 5), so a heap corruption waits 3 s before its request:
at a quit the process ends during the wait.

**Controller side** (`game_bootstrap_ctl.cpp`, its wait loop already polls the process every 50 ms): on state 1 it takes the
request, writes `<journal folder>/crashes/<journal name>-crash<N>.dmp` with `MiniDumpWriteDump` from out of process
(`ClientPointers` true: the exception record stays in the game's memory; thread info, unloaded modules, full memory info,
indirectly referenced memory, and the data segments of `game_bootstrap.dll` only), then a `.txt` report: kind, code, reason,
device-removed reason, exception record, and the faulting thread's stack walked from its exception context (or from the
suspended thread) and named from the module's PDB at that moment (the PDB of a later build would no longer match the dump).
Writing from out of process keeps the dump possible when the game's heap is corrupt or its stack exhausted.

**Message**: in play mode (no time bound), after the game exited, the controller shows one message box when the run ended
on a native fault exit (kinds 2-5) or on a recorded exception with an abnormal exit code: what happened in French words and
the folder of the dump. Bounded test runs print the record in the controller's final JSON row instead (no window).

## 4. GPU loss (lead C2)

- **DRED** (`DeviceQueues::Open`, before the device): auto-breadcrumbs and page-fault reporting forced on through
  `ID3D12DeviceRemovedExtendedDataSettings` (part of the runtime, no Graphics Tools needed), unless `no-dred.flag`. Its cost is
  measured on the Firelink walk (A/B) before the default is kept.
- **Removal report** (`renderer/device_removal.cpp`): the HRESULT, then per breadcrumb node not completed (at most 16): list and
  queue names, operations completed and recorded, and the operations around the last completed one by name; the page fault
  address with the existing and recently freed allocations near it (type and name). JSON rows in the journal and in the report.
- **Names**: the queue, the frame-slot lists, the ingress lists and the recording-pool chunk lists get `SetName`, so the
  breadcrumbs say which list stopped.
- **Hang**: `WaitOn`'s deadline sets the hang flag before it throws; `ExitNative` reports kind 5 and dumps. (First design:
  remove the hung device on purpose to freeze the breadcrumbs. Dropped: DRED records GPU faults only, nothing after
  `RemoveDevice`, measured on WARP and the hardware, RESULT observation 3. A hang Windows detects removes the device itself.)

## 5. Proof (fault injection, behind flags, read once at start)

At the 1,200th presented frame (the title screen, no pilot needed), on the engine thread: `test-crash.flag` writes through a
null pointer (kind 1); `test-device-removed.flag` calls `RemoveDevice` (kind 4 at the next submission or wait);
`test-gpu-hang.flag` makes the queue wait on a fence nobody signals (kind 5 at the 5 s deadline). None touches another process
or the system (no `dxcap -forcetdr`).
