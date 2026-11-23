# I-0109 - Le socle DX12 moderne (R0): result

Status: complete (2026-10-30): tranche 1 merged into `main` (`4b99b292`), the start check `start-4b99b292` passed; branch
`claude/quizzical-hofstadter-051ee4`, second pilot (the RT pilot since 2026-10-30 05:34).

## The inbox runtime on Thomas's PC (2026-10-30 04:10, outside the game)

[device-caps-probe.cpp](device-caps-probe.cpp), built with the module's MSVC and run alone (no game), output
`evidence/I-0109/device-caps-probe-2026-10-30.txt`. OBSERVED:

| Item | Value |
|---|---|
| Adapter | NVIDIA GeForce RTX 4070 Ti SUPER, 16 061 MB, driver UMD 32.0.16.1714 |
| Runtime | `d3d12.dll` and `D3D12Core.dll` 10.0.26100.9549 (inbox, Windows 11 10.0.26200) |
| `D3D12CreateDevice` at 12_2 | S_OK; highest feature level 12_2 |
| Highest shader model | 6.8 (6.9 asked, 6.8 answered) |
| Resource binding / heap tier | 3 / 2 (full bindless) |
| Raytracing tier | 0xC (`D3D12_RAYTRACING_TIER_1_2`: DXR 1.2) |
| Mesh shaders / sampler feedback | tier 1 (10) / 0.9 (90) |
| Enhanced barriers | supported (`OPTIONS12`) |
| `ID3D12Device5`, `ID3D12Device10` | available |
| `D3D12GetInterface(CLSID_D3D12SDKConfiguration)` → `ID3D12SDKConfiguration1` | S_OK (the Agility SDK's in-process route exists, unused) |

Consequences (INFERRED): R0 needs no Agility SDK; DXR 1.2's opacity micromaps are an API feature available now, while shader
execution reordering needs SM 6.9 (an Agility SDK) later. The Windows SDK 10.0.28000 carries dxc 1.9.2602 (targets up to
`lib_6_9`) with `dxil.dll`.

## Elden Ring

`eldenring.exe` 2.7.1 exports no `D3D12SDKVersion` and ships no `D3D12` folder (OBSERVED, export table read): its ray tracing
runs on the inbox runtime. The static reading (`ghidra-research`, headless on the mirrors; [REPORT](../../evidence/I-0109/er-rt-study/REPORT.md),
its device creation and imports re-read by the pilot), read under D-0236 for the RT's place in the engine:

- The device: `D3D12CreateDevice` at 12_0 asking `ID3D12Device5` directly (`FUN_141ea2730`), the inbox runtime (imports:
  `D3D12CreateDevice`, `D3D12GetDebugInterface`, `D3D12SerializeRootSignature` only). The RT gated by OPTIONS5 and, a defect,
  forced off on any « GeForce GTX » adapter name.
- The place in the engine: a ray-tracing system (`GXRT::GXRayTracingSystem`) owning an RT world (`GXRtWorld`) where the models
  register their BLAS instances world-wide, apart from the camera's draw lists; a per-frame executor builds the TLAS whole and
  dispatches one ray generation per effect (AO, sun shadow) in a stage after the G-buffer and before the deferred shadow and AO
  (`RAYTRACING_POSTGEOM`, INFERRED from the stage table); results denoised and faded by distance into the raster.
- Its workings (not a model, D-0236): DXR state objects and `DispatchRays`, no inline ray query seen; static BLAS built once,
  skinned BLAS by compute skinning and refit; the TLAS capped at 2,000 instances; RT shaders as DXIL libraries compiled offline.
- For R0 (INFERRED): the place matches the plan's (an RT world registered apart from the camera's lists, RT-PT-PLAN §3.2), and the
  device route is the one tranche 1 takes (inbox runtime, offline DXIL); 12_2 here instead of ER's 12_0.

## Tranche 1 (branch): the device, dxc, the self-test and the bindless heap

1. **The device** (`DeviceQueues::Open`): created once (the level asked is a minimum: one device per adapter), its highest
   level read (`D3D12_FEATURE_FEATURE_LEVELS`); `device5` held (null on a runtime without it); `DeviceCapabilities` read once (shader model asked from `D3D_HIGHEST_SHADER_MODEL` down, binding tier,
   raytracing tier, mesh shader tier, enhanced barriers). The start row `native_device_caps`. The pipeline library needs no new
   key: a new module already discards it (`PipelineStore::Environment::module`). Unit check in `crash_dump_test` (12_2's
   guarantees read: SM 6.5, DXR 1.1, binding tier 3, mesh shaders, `device5`).
2. **dxc**: `native/renderer/shaders/rt_selftest.hlsl` compiled by the build (`dxc -T cs_6_6`, the Windows SDK's on PATH after
   vcvars) into `rt_selftest.dxil.h` in the build folder; its root signature is inside the DXIL.
3. **The self-test** (`RunRaytracingSelfTest`, `raytracing_base.cpp`): one triangle's BLAS and TLAS, one inline ray
   (`RayQuery`) in the DXIL compute shader, the TLAS and the result reached through the bindless heap
   (`ResourceDescriptorHeap`), read back: the hit at t=1 expected. It runs once at start after the device opens, before the
   frames; it never refuses (a failure names its step in the row `native_rt_selftest`); `no-rt-selftest.flag` skips it.
4. **The bindless heap** (`BindlessHeap`, `BindlessIndices`): one shader-visible heap of 2^18 slots for the RT path, slots
   reused only after the fence passes the ticket of their last reader; unit test `raytracing_base_test` (the slots' rule, the
   self-test on the hardware with the debug layer, and on WARP, reported).

### The step review (fresh Opus, 2026-10-30) and its answers

No high finding. Answered:

- M1: build-2 tested a source whose scratch and result buffers were still created in UAV (the debug layer's four « Ignoring
  InitialState » warnings); the committed COMMON states are now built and tested (build-3).
- M2: the self-test's objects live in one holder: released once the list completed, retired after its ticket when the wait
  fails, kept to process exit with their bindless slots when the submission itself fails (no ticket proves completion).
- M3: `raytracing_base_test` fails on any debug-layer warning or error, as the stage suites do.
- L1: `feature_level` is the device's highest (`D3D12_FEATURE_FEATURE_LEVELS`), not the level asked; `crash_dump_test` relaxed.
- L2: an incremental build (`-ChangedUnits`) keeps the `dxc` lines (and, since the final review's M2, compiles
  `raytracing_base` with them, the unit that embeds their headers).
- L3: `BindlessIndices` keeps each slot's state and refuses a double release (unit-tested).
- L4: `BindlessHeap::Open` returns its HRESULT; the self-test names the step `bindless_heap`.
- L5 (left, recorded): the self-test runs on the game's device and queue at every start (76 ms on the hardware and 15 ms on
  WARP with the debug layer in build-3's unit test); its time in the game is read at the start check; caching a pass per adapter
  and driver if it costs.

### The final review (fresh Opus xhigh, 2026-10-30) and its answers

No high code finding. H1 (the merge order) is answered by Thomas: « Fusionne après la revue » (2026-10-30 04:48), so this branch
merges before I-0105's close. Answered (build-4):

- M1: `device5` is released by `Close` and detached by `QuarantineToProcessExit` with the device (it held the device past its
  close, so a reopen got the old device).
- M2: an incremental build compiles `raytracing_base` with the `dxc` lines.
- M3: `raytracing_base_test` holds the queue behind an unsignalled fence: the self-test reports `wait`, retires its objects after
  its ticket and keeps both slots pending; once the fence is signalled, both come back.
- L1: the keep-to-exit holder is made before the submission (no allocation once the list may run); a failed `RetireAfter` keeps
  the objects to exit. L2: the stages `submit` and `wait`. L4: one device creation (the 12_2-then-11_0 pair was the same device).
- L5: `BindlessHeap::Cpu` refuses a slot outside the heap; the comments say the order the slots come back in. L6: the start
  check reports `native_device_caps` and `native_rt_selftest` and fails when the self-test ran and failed; the row's `t` is never
  NaN. L7: the timing above. L9: the test's header. L10: `dxc --version` in the build log.
- Left, recorded: L3 (a self-test wait that times out leaves `gpu_hang_detected` set for the run's crash record); `Allocate`
  trusts its caller's completed value (L5).

### What tranche 1 leaves of the contract (L8)

The bindless heap's per-frame rings (R2, with their first frame user); `ID3D12Device10` and the command list 7 (enhanced
barriers, when a pass uses them); DXIL debug information for PIX (the build strips it; `-Zi -Qembed_debug` when a capture needs
it); an `rt_available` state from the self-test's result (R2 reads it; today the row is diagnostic only). No ER reading of its
descriptor heaps: the bindless heap follows the RT plan (D-0236).
- The step review of these answers (fresh Opus, 2026-10-30): no high or medium. Answered (build-5): the failed-wait test lets
  the queue go on every exit and checks exactly one retirement and both slots back; it skips (and says so) where the self-test
  does not run; the feature level defaults to 11_0 should its query fail; the start check's header names the new condition.

## The start check after the merge (`start-4b99b292`, 2026-10-30 05:40-05:41, the ASUS)

`scripts/Invoke-StartCheck.ps1 -Commit 4b99b292` on the launcher's module rebuilt from `main` (107 PASS): the world drawn (381
frames), closed by its close box and gone by itself, no refusal, failure or crash, `restoration ok`; OBSERVED in
`evidence/I-0109/start-4b99b292.json`: `native_device_caps` feature level 12_2, SM 6.8, raytracing tier 1.2, binding tier 3,
mesh shaders, enhanced barriers; `native_rt_selftest` ran and passed inside DS3 (status 1, t = 1.0000) in 157.53 ms at start,
without the debug layer (twice the unit test's 78 ms: INFERRED the pipeline's first compile in the game's process; L5 stays
recorded, a cache per adapter and driver if it matters).

## Closing

R0 is done: the device, dxc, the bindless heap and DXR proven inside DS3, Elden Ring's RT read (D-0236). What the contract
leaves goes to the workstreams that first use it (the list above). Next: R1, DLSS and Reflex through Streamline 2.14.16 (in
`Tools/DS3-Decompilation/third-party`), its inputs listed in [RT-PT-PLAN](../../.project/RT-PT-PLAN.md) §5; then R2 and R5
(D-0237).
