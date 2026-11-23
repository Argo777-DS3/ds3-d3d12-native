# I-0072: Elden Ring's pipeline-state architecture (D-0195 study)

Static reading of Elden Ring 2.7.1 (`eldenring_2.7.1.0_25080141_1A354710.exe`), headless Ghidra read-only on the copy
`C:/Users/User/Tools/DS3-Decompilation/er-mirror-i0072-a` of the analysed project. Study A (pipeline creation, key, cache,
ownership) was carried by a Sonnet 5.5 worker on 2026-10-20; its C/ASM exports are kept with the study's scratch (not in Git).
Opus checked against the exports the four points the design relies on (marked **checked**); the rest is the worker's reading,
non-canonical until a design needs it.

## Observed

1. **Sites.** The seven functions that pass `IID_ID3D12PipelineState` (the GUID occurs once, `0x142c09c80`): the RHI graphics
   path `0x141ea95d0` (`ID3D12Device2::CreatePipelineState`, stream description, +0x178; the main renderer's), the classic
   graphics `0x141ea72c0` (+0x50, one RHI helper), compute `0x141ea8360` (+0x58), the library loads `0x141ea8650` (compute),
   `0x141ea8770` (graphics, no caller found), `0x141ea89d0` (`ID3D12PipelineLibrary1::LoadPipeline`), and a separate UI path
   `0x140eb9110` (own hash map, own device and list).
2. **One pipeline cache per GX device** (`*(GxDevice+0x30)`, 0x648 bytes, ctor `0x141aff870`), reached from the
   `GXBS::GXDeviceContext` draw and dispatch methods (`0x141acaf20`, `0x141acad20`) through the context's state tracker
   (`0x1419e3700` builds the key only when the tracked state is dirty).
3. **Key.** Graphics: 128 bits built from small pooled ids (the five stage programmes' ids, the pooled input-layout,
   rasterizer, blend and depth-stencil ids, a topology class, and a CRC32 of the render-target count, the eight RTV formats and
   the DSV format); compute: the programme id. The library name is a second identity: a 64-bit hash of a normalised 0x150-byte
   description (`0x141c5a1d0`), printed in decimal.
4. **Two maps** (MSVC `std::map`): a main map read without lock, and a pending map behind the cache mutex (cache+0x258);
   a frame step (`0x141b011a0`) merges pending into main, bumps a generation and evicts entries unused for a configured number
   of generations (a scan budget per frame).
5. **A miss is synchronous on the recording thread, under the cache mutex (checked, `0x141b01ae0`):** the mutex is taken at
   entry, the pending map searched again, then the library is asked by name (`0x141c5a8c0`, LoadPipeline) and, when it does not
   hold the pipeline, `0x141e89920` -> `CreatePipelineState` compiles it; a created pipeline is stored in the library when the
   store flag (+0x640) is set, and inserted in the pending map; a failure is kept as a negative entry (-1) and the draw is
   skipped (no fallback pipeline, no asynchronous creation). This is ER's compilation stutter (D-0157 does not copy it).
6. **The pipeline library on disk:** created at renderer creation from a blob (`0x141ea85a0`, `CreatePipelineLibrary` +0x160),
   written back (`0x1419eca00` -> `0x141c5a960`) as `'GPL\0'`, version 3, the blob, then the adapter's name (checked on load:
   another adapter means an empty library).
7. **The precompile list ("PSC", checked, `0x1419fe870`):** a file of magic `0x435350`, version below 3, holding graphics
   records (0xD8 bytes) and compute records (0x18 bytes) whose shader references are names looked up in the loaded shader
   registry (`_DAT_1447f3750`, FNV-1a 64); a missing shader logs « Failed preload GraphicsPipelineState shader not found. »
   Each record builds the same 400-byte stream and calls `0x141b01610` (graphics) or `0x141b014d0` (compute): load or create
   through the library, then the handle is released at once (checked): the pass warms the library and the driver's cache, it
   does not fill the in-memory maps. It runs as a step of `CSGraphicsStep::PipelineStateCacheFragment`, with a lock and a
   cancel flag per record. A capture mode writes the descriptions met in play as JSON under
   `capture:/PipelineStateCache/` (the tooling that builds the list).
8. **Root signatures:** two global ones (graphics, compute) created at device creation (`0x141e9fd30`) and used by every
   pipeline that names none; custom ones only for RHI helpers, ray tracing and the UI renderer.

## Study B: programmes, loading and the boot precompile (Sonnet 5.5 worker, copy `er-mirror-i0072-b`)

9. **Programmes come from the shader archives and loading creates no pipeline.** `.shaderbnd` (`GXShader`, `GXFlverShader`,
   `GXFfxShader`, `GXPostEffect`, ...) and `.shaderbdlebnd` archives (BND4/DCX) are turned on completion into per-stage
   objects (`GXBS::GXVertexShader`/`PixelShader`, device-side `GXBS::GXCGShader`: a copy of the bytes, an id from the pipeline
   manager, the name, a 64-bit hash) and indexed by name in the repository the precompile list reads (`_DAT_1447f3750`).
   Pipeline creation has two entry points only: the draw/dispatch-time miss and the precompile list.
10. **The boot step (checked, `0x140b1f5a0`):** `CSGraphicsStep::STEP_Initialize_forPipelineStateCache` waits until the
    shader and material archive handles report loaded (state byte +0x88 == 4, a dozen of them), then queues
    `PipelineStateCacheFragment` on the job system (`EzWorkPool_HIGHEST_Fixed_Cluster0`); `BeginDrawStep` polls it each frame
    and `STEP_Finish` waits for it, then saves the library. The list is a shipped file, `shader:/PipelineStateCache.dat`,
    authored offline from the capture JSON (a developer switch).
11. **The library file (checked, `0x140b1e460`):** `%TEMP%\FS<hash of "ELDEN RING">.plf`, the `'GPL'` container: header
    (version, a build id checked within bounds, sizes), the serialised library, then the adapter description
    (`IDXGIAdapter::GetDesc`); any mismatch ignores the file and starts an empty library. No `CachedPSO`, no
    `GetCachedBlob`, no D3D12 shader-cache session.

## Inferred

- The recording thread is a render worker (per-context trackers, the lock-free main map, a render-thread check before the
  library save); no dedicated compile thread was found.
- The PSC step runs at loading (a graphics-step fragment with a cancel flag); its thread and whether it holds the loading
  screen are not established.

## Open

The PSC and GPL file names (they come in the init parameters), the eviction table's values, the ray-tracing pipelines.
