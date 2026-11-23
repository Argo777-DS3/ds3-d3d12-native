# I-0072 result: the shader and pipeline cache

Complete (2026-10-20, merged into `main` under D-0129 after the pre-merge review below).

## Conclusion

H1 confirmed. On a cold pipeline library (a first start on this PC, a new build folder, or after a driver change) the first
frames of each scene compile their pipelines on the render worker, ER's synchronous miss: in the main runs the windows that
compiled pipelines hold 29 of the 32 frames over 34 ms of the Firelink walk, 16 of 20 at m45 and 64 of 115 on the Archdragon
Peak walk, and 3 to 5 slow frames of 260-710 ms carry 46-68 compilations each. Thomas's play of 2026-10-19 16:41 compiled about
200 pipelines with the V1 library loaded (one window: 55 pipelines, 24 frames over 34 ms).

H2 confirmed: with the V1 library warm, the frames still loaded every pipeline from the library on the frame path (104-173 a
scene) and compiled 10-13 at Firelink and m45 (descriptions the V1 names did not separate, kept unstored, or met for the first
time).

H3 confirmed: with the cache, after any start that follows a run which met the scene (a warm start, or "reload shaders" that
removes the library and keeps the list), the three scenes compile **no** pipeline on the frame path and load none: every frame
request is a memory hit (103-173 a run). The precompile creates the 188-206 listed pipelines at start on four workers in 28-47
ms from the library, or in 36-502 ms when it must compile them (the driver's own shader cache warm; a driver change compiles
more), long before the first world frame. Pipelines never met on this PC still compile at their first frame (the cold runs
equal main's), and are listed for the next start.

The architecture is Elden Ring's (ER-PIPELINE-STUDY): one cache per device with a memory map, global roots held by the device,
a library on disk checked against what wrote it, and a precompile list replayed at boot (ER's `PipelineStateCache.dat`, shipped,
replayed by `CSGraphicsStep::STEP_Initialize_forPipelineStateCache` once the shader archives are loaded). DS3 differs where ER
stutters or depends on load order: the list is written by the cache itself on this machine (ER's capture tooling always on) and
carries the programme bytes and root blobs, so the precompile starts with the device instead of waiting for the shader
archives; its pipelines stay in memory (ER releases them and still loads at the first draw); a miss never holds the cache lock
while the driver compiles (ER holds its mutex), and one thread builds a given name while the others wait for that name only.

## The change

- `PipelineStore` (backend.h/.cpp): names = a 128-bit hash of the canonical encoding of the whole description (root blob name,
  programme bytes names, every fixed-function field written field by field, the input layout); `Root` (one root object per blob
  and device); `Graphics`/`Compute` (memory hit, else the library's object when it holds the name, else compiled and stored);
  `Precompile(workers)`; `Save` writes the library and `native-pipelines.v2.library.psc` (magic, version, the library's stamp,
  roots, programmes, records, checksum; every name recomputed at load, any mismatch discards the file; a library whose stamp
  differs is replaced by an empty one). The V1 names index (`.names`) and the per-site name hashing are gone; the v1 files are
  deleted at start.
- Sites: the model draws, the effect stage (draw, particle, compute), the point lights and the primitive pass take their roots
  from `Root` and pass their descriptions; `pso_created` counts compilations only.
- Rows: `native_pipeline_library` (list loaded, records, workers), `native_frame_audit.pipeline_cache` [memory hits,
  precompiled, listed, failed, waits, direct, precompile ms, done], `native_pipeline_library_close`.
- Launcher: `Launch-DS3-DX12.ps1 -ReloadShaders` and `Launch-DS3-DX12-ReloadShaders.cmd` (D-0172's "reload shaders": the
  library removed, the list kept, everything recompiled while the game boots).
- Unit controls `pipeline_cache_test` (WARP and hardware, debug layer): one root per blob, memory hits, one build for eight
  racing threads, file round trip and precompile from the library, a damaged list, a foreign library.

## Evidence

Runs `evidence/I-0066/artifacts/i72-native-01..18-*` (measure mode, no debug layer, I-0070's saves and key paths), builds
`evidence/I-0072/artifacts/{main,i72,i72b}-build`, series logs `series-{main,i72,i72b}.log`; summary `Get-PipelineSeries.ps1`.

| Scene: on the frame path, compiled / loaded from the library; frames over 34 ms | main cold | main warm (V1) | cache cold | cache warm | cache reload |
|---|---|---|---|---|---|
| Firelink walk | 157 / 0; 32 | 12 / 140; 56 | 158 / 0; 33 | 0 / 0; 26 | 0 / 0; 29 |
| m45 | 121 / 0; 20 | 18 / 104; 25 | 121 / 0; 17 | 18 (new to the list) / 0; 23 | 0 / 0; 21 |
| Archdragon Peak walk | 173 / 0; 115 | 0 / 173; 66 | 172 / 0; 93 | 0 / 0; 83 | 0 / 0; 92 |

The counts are the library's over the run (every site: model draws, effects, point lights, primitives); with the cache the
precompile's own loads and compilations are subtracted (warm: 188-206 loaded at start, none compiled; reload: 206 compiled at
start). The rows written after the review count the frame requests apart (`pipeline_cache` frame_created, frame_loaded).

The cold series removed the files before each run, so the warm m45 run met 18 pipelines its list (Firelink and Archdragon Peak)
did not hold; the reload runs, with the whole list, compiled none. The frames over 34 ms left are the scenes' other hitches (the
m45 load frame of 440 ms, texture and model admissions, I-0070's residuals): they vary run to run as much as between builds.
Witness run i72-native-10 (debug layer on, warm start): 0 debug-layer errors over 5,187 frames. After the review corrections (build `i72b-build`, the same list): i72-native-17 (Firelink, debug layer on) and 18 (m45, measure), warm: 206 precompiled from the library in 27-29 ms, 0 compiled and 0 loaded by the frame requests (`frame_created`, `frame_loaded`), 0 debug-layer errors over 5,181 frames, m45 17 frames over 34 ms.
Pictures: the start and arrival shots of main against the cache 0.97-1.01 at Firelink and m45; Archdragon Peak 0.94-1.31 in
the sky and fog cells, as between two main runs (0.88-1.13), the scene identical by eye.

## Uncertainty

- The first encounter of a pipeline on a PC still compiles at its first frame (cold columns). Predicting the pass rows'
  descriptions at material admission, or seeding the list from the witness-zone coverage runs, is the next tranche if wanted.
- `native_pipeline_library_close` never appears in the runs (the process ends without the device close, as before this
  workstream): the files are written by the pacing window's save every 600 frames when something changed, so the pipelines met
  in the last seconds before quitting are listed at the next start only.
- A driver update without the driver's own shader cache costs more precompile time than measured here (502 ms the most); the
  boot hides several seconds, not measured beyond that.

## Artifacts produced or modified

Code above; `ER-PIPELINE-STUDY.md`, `Get-PipelineSeries.ps1`, `pipeline_cache_test.cpp`; launcher and its `.cmd`.

## Pre-merge review

Opus 5.5 at high effort, 2026-10-20 on 85f81404: no high finding; nothing neutralised or dead, no D3D11 path, the V1 naming fully
removed. Corrected on the branch: M1 the library and the bytes it was created from are held together (`Library`), so a load
running outside the lock keeps both alive; M2 `Save` keeps only the serialisation and a snapshot under the lock, the checksums,
the list's assembly and the disk outside; M3 the list ages its records (each start writes them one start older, a frame
request resets the age, eight starts unused and the record leaves the list: stale descriptions of earlier builds go); L1 the
programmes are hashed before the lock; L2 a rasterized stream goes direct; L3 `Save` writes only complete records; L4 a stored
name the library fails to load marks the library stale (written without a stamp, rebuilt at the next start); L5 the precompile
guard and the device read under the lock; L7 `Open` keeps what the same device built before it; L8 frame-request counters apart
from the precompile's (`frame_created`, `frame_loaded`); L9 the test's direct path uses another root blob, and the test covers
Close then Open, the frame requests' counters and the ageing. Accepted: L6 (a frame waiting on a below-normal precompile worker,
at boot only), L10 (the v1 files deleted at each start: harmless; a quit during the boot precompile without Close).

Later (2026-10-30, I-0113 tranche 2, D-0241): the driver library is no longer loaded nor stored; the list (format 3,
`native-pipelines.v3.psc`) is compiled at every start. Loaded pipelines sampled their static samplers as point samplers once the I-0109
ray-tracing self-test ran at start (K-0486).
