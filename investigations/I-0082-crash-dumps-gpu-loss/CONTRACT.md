# I-0082 - Minidumps automatiques et pertes du GPU

Capability: C3

## Question

Can every native crash and every GPU loss of the native renderer leave a minidump, a readable report and, during play, a
message to the player, instead of closing the game silently; and can the module start without the Windows Graphics Tools
(the D3D12 debug layer off unless asked)? Thomas's order D-0200 (stability tools, « je valide les 4 »), with the two holes
[I-0080 ER-STUDY](../I-0080-er-dx12-pitfalls/ER-STUDY.md) section 7 found (leads C2 and G1).

## Scope

Included:

- G1: the debug layer opt-in (`debug-layer.flag`, or `gpu-validation.flag`), tolerated when absent; the runners keep their
  meaning (`Run-StartupTrace.ps1` places the flag unless `-NoDebugLayer`); a pipeline library of its own under the debug
  layer (I-0081 observation 6: the layer refuses `StorePipeline` of a name already in a library written without it).
- C2: the native fault exits (refusals, native failures), the fatal exceptions, a removed device and a GPU hang (the 5 s
  completion timeout) each request a minidump; the controller writes it out of process with a symbolized report of the
  faulting thread; DRED breadcrumbs and page fault data in the journal when the device is lost; a hang removes the device to
  obtain them; a message box to the player in play mode.
- Test-only fault injection behind flags (a crash, a removed device, a GPU hang), to prove each path.
- The cost of DRED in play (A/B on the Firelink walk, measure mode, no debug layer).
- Elden Ring's treatment (D-0195), from I-0080 section 7, and what the native backend does better.

Excluded:

- Recovering from a lost device (recreating the device and every resource mid-game): the report and a clean exit only.
- The process-exit heap fault 0xC0000374 (pre-existing, also in the Reference): logged, dumped only on request
  (`exit-dump.flag`), its diagnosis left to the audit (I-0080 lead D2).
- The other audit leads (A1-F1, G2-G3) and the replayed path (D-0200's next workstreams).

## Inputs

- [I-0080 ER-STUDY](../I-0080-er-dx12-pitfalls/ER-STUDY.md) sections 3 and 7 (ER: no minidump, no device-removed handling, debug
  by a configuration byte with a missing layer tolerated; ours: debug on unless `no-debug-layer.flag`, a hang ends in a silent
  `ExitNative`).
- Code: `native/renderer/backend.cpp` (`DeviceQueues::Open`, `WaitOn`), `native/frame/game_bootstrap_ctl.cpp`,
  `native/frame/game_bootstrap/15_cpu_service_and_input.inc` (`Fault`), `02_frame_lifecycle.inc` (`ExitNative`, `Refuse`),
  `20_startup_phases.inc` (the debug-layer flag), `tools/Launch-DS3-DX12.ps1`.
- The old `world/live_host.cpp` DRED reader (I-0050), a diagnostic host, not production.

## Design (to be validated)

See [DESIGN.md](DESIGN.md).

## Expected evidence

Unit tests (the dump writer and the report on a thread of the test process; the removal report on a WARP device removed by
`RemoveDevice`); three live runs at the title screen with the injection flags, each leaving a dump, a report and journal rows
of the right kind; a run without any flag and without the debug layer starting normally; the DRED A/B walk.

## Exit conditions

- Success: each injected fault yields a dump that opens (MDMP signature, the faulting thread present), a report naming the
  faulting function from the module's PDB, the journal rows, and the play-mode message; the module starts without
  `debug-layer.flag`; DRED's cost measured and the default chosen from it.
- Refuted: a path cannot produce a dump from out of process (recorded, with the in-process fallback's evidence).
- Insufficient information: recorded with the missing instrument.
- Stagnation: no new discriminating evidence after 3 cycles.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`.

## Safety and human interaction

Live runs under D-0117/D-0116 through `Run-StartupTrace.ps1` (offline, hash-checked restore), announced 5 minutes ahead with
the PC clock; the injected faults touch only the test process (no system TDR: `ID3D12Device5::RemoveDevice` and a queue wait
are confined to the game's device). Alpha untouched. Dumps are disposable cache (D-0142): kept until the cycle that read them
closes.
