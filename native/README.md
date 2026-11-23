# Native D3D12 backend source

C++20 sources of the native backend that replaces DS3's D3D11 renderer
([mandate](../.project/NATIVE-D3D12-RESET-MANDATE.md)). Nothing here is
installed in the game directory by the investigation. Independent in-game
development has been authorized under the mandate. D-0156 restarted the direct
connection design after the deeper Elden Ring study and primary source checks;
current resource selection is described in [the bounded contract732–733](../investigations/I-0066-independent-native-startup/DIRECT-MODEL-INPUTS.md).
Production targets direct engine integration; detached
hosts and capture transports remain diagnostic clients, never required runtime
dependencies (D-0155). See [STATE](../.project/STATE.md) for the current checkpoint. The « Key qualification » links below are evidence of I-0038 and earlier, which the retention policy removed from the external store: they are historical records, not available inputs.

| Module | Responsibility | Key qualification |
|---|---|---|
| [`renderer/`](renderer) | Device/queues, uploads, descriptors, PSOs, fences and presentation (`backend`); versioned resource/pass dependencies (`frame_plan`); scene pass and view; ordered primitive pass; the model's mesh slices (`content.h`); the I-0036 witness `Backend` and its camera intent were removed by I-0102 | common frame plan (historical: `evidence/I-0038/artifacts/native-only-bootstrap/common-frame-plan-qualification.md`), primitive pass (historical: `evidence/I-0038/artifacts/native-only-bootstrap/native-primitive-pass-controls/QUALIFICATION.md`) |
| [`content/`](content) | The guarded in-process reads (`acquire`), DCX/TPF containers, DDS resources and GPU texture upload, the shader programme types (`shader_bank.h`), RDEF shader interfaces, the model's programmes, parameters, poses and textures, the AA, decal and paint constants, GPARAM, texture tiling; the material admission, the CPU texture loader and packages, the material resources, the shader bank reader and the I-0037 content host were removed by I-0102 | startup shaders (historical: `evidence/I-0038/artifacts/native-only-bootstrap/native-startup-shader-qualification.md`), startup resources (historical: `evidence/I-0038/artifacts/native-only-bootstrap/native-startup-resource-qualification.md`), material admission (historical: `evidence/I-0038/artifacts/material-shader-input-primary/NATIVE-MATERIAL-ADMISSION-CONTRACT.md`) |
| [`frame/`](frame) | The game module (`game_bootstrap.cpp` and its `game_bootstrap/*.inc`, built by `Build-GameBootstrap.ps1` with its unit tests): the hooks into the running game, the engine's CPU owners, scene ingress, the UI and its primitive programmes/constants/textures/work, effects; the earlier owned-startup stack (I-0038 to I-0040) was removed by I-0102 | the module's unit tests (`Build-GameBootstrap.ps1`) and the live runs of the investigations index (the I-0038 qualifications of the removed stack are historical) |
| [`world/`](world) | The native producers the module links (constant blocks, lights and light tiles, the YEBIS blocks and passes) and the host's virtual-file input (`host_input.h`); the capture-era tools (live host, world adapter, replays, checks) were removed by I-0102, recoverable from commit `6a3b127a`; the effect producers (`effect_producers`) by I-0104, recoverable from `3855d0de` (their 264230 reading: `evidence/I-0064/EFFECTS.md`) | the module's unit tests |
| [`third_party/zlib/`](third_party/zlib) | zlib (DCX decompression) | [source](third_party/zlib/README.md) |

The development-only modules were removed by I-0102: the `frame/output_adapter` prototype (cycle 26) with `window_host.h` and
`execution_scope`, and `world/`'s capture tools (recoverable from commit `6a3b127a`); the I-0036 `camera/` diagnostic, the I-0037
content host (`content/` adapter, controller, host and their scripts) and the `renderer/` ownership test with its `Build.ps1` and
`Test.ps1` (recoverable from commit `bb88b0d8`). The module keeps the camera writer's two pins in `frame/scene_ingress.cpp`.

## Build

The build script (`frame/Build-GameBootstrap.ps1`, the module and every unit test; the `frame/Test-*.ps1` controls reuse its objects) uses the pinned MSVC toolchain
(`vcvars64.bat` from Visual Studio 18 Insiders) and write outputs under
`evidence/<investigation>/artifacts/`, which is git-ignored. Many I-0038
builds and controls were driven by the scripts of the archived investigation
`investigations/I-0038-actual-game-frame-ownership/` (in Git tag `archive/pre-cleanup-2026-10-13`, not in `main`).
Historical evidence inputs may have been intentionally removed under the
retention policy. Do not restore them automatically to rerun an old
qualification; consult `.project/EVIDENCE-RETENTION.json` and the current
checkpoint first.
