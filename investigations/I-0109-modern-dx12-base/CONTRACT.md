# I-0109 - Le socle DX12 moderne (R0)

Capability: C3 · [RT-PT-PLAN](../../.project/RT-PT-PLAN.md) §8, chantier R0 (D-0235) · second pilot, opened 2026-10-30 after
FINISH-LINE line 1 was found done (I-0101, I-0099; Thomas, 03:51) and the coordinator relayed « préparation de R0 sans croiser le
pilote effets ».

## Question

What does the native module need so that ray tracing can run inside Dark Souls III's process (device, shader compiler,
descriptors, captures), and can it be put in place with the game unchanged?

## Scope

Included:

- The device: feature level 12_2 when the adapter has it (the 11_0 path kept as the fallback during the transition), the
  interfaces RT needs (`ID3D12Device5`, `ID3D12GraphicsCommandList4`; `ID3D12Device10` and the command list 7 for enhanced
  barriers), one start row naming the capabilities read.
- The shader compiler: dxc (Windows SDK 10.0.28000, 1.9.2602) at build time, HLSL to DXIL, the byte code embedded in the module;
  the existing fxc shaders (`D3DCompile`, SM 5.0) and DS3's own DXBC programmes untouched.
- A ray-tracing self-test at device start: one triangle's BLAS and TLAS, one ray query in a compute shader, read back; a row
  says whether DXR works inside DS3's process. It never refuses: a failure marks RT unavailable.
- The bindless heap of the ray-tracing path: one shader-visible CBV/SRV/UAV heap, a persistent region indexed by stable handles
  and per-frame rings. The raster stages keep their own heaps (removed once their traced replacements are accepted, D-0237).
- Captures: DXIL with debug information in PIX; Nsight Graphics is Thomas's to install (WORKFLOW « Tools »).
- Elden Ring read, never copied for the ray tracing (D-0236): its device creation and its ray tracing (patch 1.09: shadows and AO), what it builds for map pieces
  and characters, and how it feeds its TLAS.

Excluded:

- The Agility SDK: not needed for R0 (the inbox runtime already gives 12_2, DXR 1.2 and SM 6.8 on Thomas's PC, below); it comes
  back when SM 6.9 (shader execution reordering) or Mega Geometry's DirectX preview are needed (R2, R8). Its download is Thomas's
  decision.
- Any visible change, any RT pass in the frame (R2 onward), DLSS (R1).
- The effects pilot's files (`effect_stage.cpp`, `native/effects/`, the FXR runners): not touched while I-0105 runs.

## Inputs

- The study: [RT-PT-PLAN](../../.project/RT-PT-PLAN.md) §2, §3, §8, §12.
- The device code: `native/renderer/backend.cpp` (`DeviceQueues::Open`), `backend.h`.
- The capability probe: [device-caps-probe.cpp](device-caps-probe.cpp), its output `evidence/I-0109/device-caps-probe-2026-10-30.txt`.
- Elden Ring 2.7.1: the Ghidra mirrors `Tools/DS3-Decompilation/er-mirror-i0072-{a,b}` (read-only).

## Hypotheses

- H1: the inbox D3D12 runtime is enough for R0 (no Agility SDK). Supported by the probe (below); to confirm inside the game.
- H2: a 12_2 device changes nothing for the game's existing DXBC programmes and the raster passes (D3D12 accepts DXBC on every
  feature level).
- H3: Elden Ring's ray tracing uses the inbox runtime (no `D3D12SDKVersion` export, no `D3D12` folder beside `eldenring.exe`:
  OBSERVED) and builds its acceleration structures from its own scene lists.

## Expected evidence

The start row with the capabilities and the self-test's result inside DS3; the build and its unit tests; the one-minute start
check after the merge (D-0229) with the world drawn as before.

## Exit conditions

- Success: the module opens a 12_2 device on Thomas's PC, the self-test passes inside DS3, the bindless heap and dxc build
  are in place, the game unchanged; reviewed (step reviews, final review at xhigh).
- Refuted: a 12_2 device or a DXIL pipeline breaks something the 11_0 device did: recorded, the cause read before going on.
- Merge order: under D-0235 I-0105 finishes before the RT workstreams; this branch merges after I-0105's close unless Thomas
  decides otherwise.
- Stagnation: no new discriminating evidence after two cycles: RCA (WORKFLOW).

## Safety and human interaction

No live run before the merge (D-0229); the start check after it, on an ASUS slot through the coordinator (the effects pilot
first). The capability probe runs outside the game. Downloads (the Agility SDK, Nsight Graphics) are Thomas's decision.
