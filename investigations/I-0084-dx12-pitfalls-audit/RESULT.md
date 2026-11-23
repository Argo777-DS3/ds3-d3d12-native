# I-0084 result: the D3D12 pitfalls audit

Complete (2026-10-21). Every lead A1-G3 of I-0080 has a verdict with its evidence in [PITFALLS-AUDIT](PITFALLS-AUDIT.md). Four
pitfalls were removed here: the video memory budget ignored (D1), the stages' pipelines outside the cache (A2), a pass compiling
its HLSL again per view (A2b), and the only other adapter this PC has, WARP, unreachable (G3). Of the rest, three are solved with
measurements (B1 at Firelink, C1, G2), three were solved earlier (C2, G1 by I-0082; the waits of C3 on our side), and the
redesigns (bindless E1-E2, a copy queue F1, transient aliasing D3, the precompile screen A1) stay where the ROADMAP places them,
now with their numbers.

## Observations

1. **Inventory** (main 68b392d4, static, every site checked against its callers): 6 pipeline callers of `PipelineStore` and 21
   direct creation sites on the game path; 19 raw `ResourceBarrier` lines (the rest through the frame graph or the helpers it
   overrides); full drains only on events (stencil-clear change, movie planes, lighting programme change, resize, close); one
   busy spin of ours (the pacer's last millisecond on a refresh that is not a multiple of 60 Hz); 20 `SetDescriptorHeaps` sites;
   `QueryVideoMemoryInfo` for a log only, no residency call.
2. **GPU-based validation** (`a84-base-gbv`, main's build): the replayed Firelink path complete, 0 fault; any error or warning
   ends the native run, so 0 of either (H1 holds at Firelink).
3. **Full drains**: 0 in play in every run on the path and over the 19-zone loop of I-0081 (`i81-19-zones-d`); H3 refuted.
4. **Cold pipeline cache** (`a84-base-cold`, no library or list): 156 pipelines compiled on the frame path, 60 by model draws in
   the first three world frames (one of 181.6 ms), none during the motion; the path's counts as warm (frames over 34 ms 13,
   against 12-13). The stages' 56 more were then outside the count: `a84-fix1-m1` (the routed build, a new folder) stores 212,
   `a84-fix1-m2` loads 212 and compiles 2.
5. **The engine thread's waits** (`a84-base-sample`): 31 % of its samples in `RtlQueryPerformanceCounter` inside DS3's own limiter
   (0x140EEDBE9..C65), as in the original (I-0078: 36 %); ER's limiter spins too (I-0080).
6. **Video memory**: peak local use 2.2-2.5 GB at Firelink (1280x720, ultra), 1.8 GB over 19 zones; budget 15.2-15.3 GB on the
   RTX 4070 Ti SUPER. With a ballast leaving 4 GB of headroom above the 0.5 GB used at the device's opening (`a84-fix1-vram4g`): no window over the budget, the path's counts as without
   (frames over 25/34 ms 75/12, held 40; m2: 82/14, 41). Leaving 1.5 GB, less than the game uses (`a84-fix1-vram1500`, then
   `a84-fix2-vram1500` after the step review's hysteresis): 66-67 windows over the budget, no fault, frames over 25 ms 134 then
   105 and held 76 then 62 (the demoted textures read from system memory); the reaction released 60 heaps the first time, 16 the
   second. H2: the reaction holds what it can (the backend's own spare memory is small: one free heap a pool, 64 MB of staging);
   the game's resident set is the game's.
7. **CPU cost of the fixes** (measure mode, the same path): collect 3.3-3.7 ms, prepare 1.7-1.9 ms, record 1.3-1.4 ms in every run,
   main and branch alike (60 frames/s).
8. **WARP** (`a84-fix2-warp`): the device opened on the Microsoft Basic Render Driver (binding tier 3, root signature 1.1), menus
   and the first lit world frame drawn; the next frame passed the 5 s completion deadline (reported as a GPU hang, I-0082). With
   WARP's deadlines (120 s, render worker 600 s), `a84-fix3-warp`: the lit world drawn frame after frame at about 11 s a frame,
   no fault, until the trace's time ran out and the controller ended it (exit 0xD3126612, its normal timeout).

## Conclusion

The native backend avoids each of ER's known D3D12 defects that the audit could measure here (no mutex held while compiling,
no CPU spin of its own, no busy frame-lag wait, no device loss left silent, no AMD library, no vendor-only path, a budget read
and acted on). What remains are the redesigns already on the ROADMAP and the game's own limiter spin, which the engine thread's
third tranche owns.

## Uncertainty

- NVIDIA's driver shader cache stayed warm in the cold run: a new PC compiles slower at the first world frame.
- One RTX PC at 60 Hz; AMD and Intel are not run (WARP stands in for a second implementation); the witness zones under GPU-based
  validation, the zone-travel leak check and a real TDR are the night endurance test's.
- The ballast stands for a small card's budget; a real 4 GB card also has less bandwidth.

## Reviews

Step review (Opus 5.5): no High; six findings applied: WARP's own pipeline library (it would have overwritten the hardware
one), `warp.flag` and the budget file removed as leftovers by the runner, the ballast logged when skipped and released before the
device, the reaction per segment (local heaps, non-local staging) and only down to the excess. Kept (Low): the stages' own
intermediates stay at normal residency priority (the scene's 31 targets, 123 MB, are raised).

Final review (Opus 5.5, the whole branch): one High, applied: the WARP deadline's edit had put a comment before
`renderer.SetRenderThread`, swallowing it, so the render thread's ownership checks were off in builds 3 (the runs `a84-fix3-warp`
only; rendering is not affected by those checks); restored before the gate. Four Low, applied: the ballast held by a leaked
object (nothing at the module's unload), the priority counter reset on a failure, the store's comment limited to the
game_bootstrap path (the fixture paths and the witness `Backend` keep direct creations), the scripts' wording of the ballast.
The replayed path gate (D-0202) then ran on the final tree: `i84-final` PASS (2026-10-21 22:36; the mean of the two measure
runs: frames over 34 ms 13, over 25 ms 90, held 57, game steps over 34 ms 2, against 22, 105, 62 and 6; the validation run
complete without fault; the standstill pictures 0.57 and 0.66 from the reference).

## Artifacts

Runs under `evidence/I-0066/artifacts/a84-*` (journals, replay logs, the sample report of `a84-base-sample`); builds
`evidence/I-0084/artifacts/build-2` and `build-3`; [Run-AuditSeries.ps1](Run-AuditSeries.ps1).
