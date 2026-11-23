# I-0078 result: the engine thread's collect toward 120 frames/s, second tranche

Complete (2026-10-21). The engine thread's native work (collect, UI prepare, end of frame) at m45 6.66 to 5.52 ms, walking at
Firelink 6.50 to 5.37 ms, sprinting 6.60 to 5.53 ms; the original's main thread measured as the reference (4.7 ms a frame at m45, 159
frames/s unlocked). Runs `evidence/I-0066/artifacts/i74-native-73..95-*` (the I-0074 series runner: `m45split`, `walk`,
`check`, `m45debug`, `m45prof`), `i78-motion-01..08-*` (Run-Motion.ps1: walk, sprint with Space held, camera turn at
Firelink), `i78-menu-01` (Run-MenuCheck.ps1: the in-game menu and Status under the debug layer), `i78-original-01..07`
(Run-OriginalProbe.ps1); builds `evidence/I-0078/artifacts/i78{a..f}-build` (a: main with `admission-marks.flag`).

## Steps

1. The engine thread's serial parts (H1 confirmed): the admitted models' places and the pass rows' places default-constructed
   by the collecting thread while the workers take the first chunks (0.38 and 0.34 ms after the pool before, i74-native-71);
   guided chunk sizes; the scene, view and planned tables carried between collects without a copy (`NativeCarriedTable`).
2. A reused model's admission (H2 confirmed in part), from the admission sections (run 75: record check 1.95, material union
   1.65, instances 1.16, reuse plans 1.11 ms of CPU a collect): each material's 0x4F8 record bytes compared once a collect for
   every record using it (`NativeMaterialRecordBytes`); the record keeps its engine bytes and its material union; the pose and
   storage registries under shared locks; one read of the draw group (`NativeReadWindow`); flat instance tables; the
   per-thread planned set open-addressed; counters added once a model. The admission's CPU fell little (model section 7.3 to
   7.1 ms): its cost is mostly memory traffic over 1,700 models' engine and native data; the wall time fell with step 1.
3. The UI (after ER's pipeline cut, I-0077 ER-STUDY §4: the render thread records what the frame draws): the UI packet goes
   in the frame job and the render worker builds it at the job's start (`SceneRenderer::ExecuteFrame`); the engine thread keeps
   a mirror of whether a build is current, refreshed from the renderer only while the worker is quiesced.
4. In motion (Thomas, 2026-10-21: walk, sprint, camera turn): while the camera moves no scene table is carried and about 1,400
   scene and view blocks were packed a collect by the thread that held `tables_lock`; they are now packed after it.
5. The admission pool at 11 workers instead of 7 (run 85): collect 4.04 against 4.10 ms for 9.4 against 7.1 ms of CPU; not
   kept.

## Measurements (measure mode, medians; the engine thread's native work = collect + UI prepare + end of frame)

| | main (a) | steps 1-3 (c / e) | with step 4 (f) |
|---|---|---|---|
| m45 standing: collect, UI, end of frame | 4.80, 1.37, 0.49 (run 77) | 3.99, 1.37, 0.07 (run 90) | 4.07, 1.38, 0.07 (run 93) |
| m45: the engine thread's native work | 6.66 ms | 5.43 ms | 5.52 ms |
| Firelink camera turn: collect, end of frame | 2.96, 0.49 (motion-05) | 2.34, 0.06 (motion-06) | |
| Firelink walk: collect, end of frame | 4.74, 0.50 (motion-01) | 4.74, 0.07 (motion-02) | 4.02, 0.07 (motion-07) |
| Firelink sprint: collect, end of frame | 4.82, 0.50 (motion-03) | 4.80, 0.07 (motion-04) | 4.18, 0.08 (motion-08) |

Instrumentation (Thomas's question): the same build without the collect timers (run 80) against with them (run 76): collect
3.97 against 4.03 ms, prepare 2.82 against 2.91 ms, about 1.5 % and 3 %; the launcher runs none of them.

Guards (record revalidation, the serial walk and pass-row checks, the engine-bytes check) on the Firelink walk, runs 81, 87, 91,
94: 0 mismatch. D3D12 validation at m45 (runs 82, 88, 92, 95): 0 error, 0 fault. The in-game menu and the Status
screen with the UI built on the worker under the debug layer (i78-menu-01): drawn, 0 fault. A zone never tested before
(Thomas, 2026-10-21): the Catacombs of Carthus, travelled to from Firelink under the debug layer (i78-carthus2-native): 0
error, 0 fault, the picture at the first bonfire close to the original's (i78-carthus2-reference; the bonfire flame a little
brighter natively); the first attempt reached Irithyll of the Boreal Valley, also drawn as the original (i78-carthus-*).

## The original, as a reference study (Thomas, 2026-10-20, D-0196)

`original-probe.flag` (a reference run without any observation hook; only the present pacer D5CB0 timed): at m45 capped the
presenting thread, which is the process's first (main) thread, works 11.0 ms a frame and waits 5.7 ms in the pacer
(i78-original-01). The pacer's interval forced to 0 leaves 16.67 ms (02, 03): a second limiter, which Thomas's timing project
(now under [timing/](../../timing/IMPORT.md): I-0009, I-0011) identifies: the central timing object [0x14489DD10], requested rate
float +0x354 and enable byte +0x358. Sampled (04, 05): capped, 34 % of the main thread waits in the vertical blank inside
Present and about 36 % spins in the game's limiter (QPC around 0x140EEDBE9..0x140EEDC65); the main thread works about
**4.7 ms** a frame at m45, its whole draw step (0x1423C1DA4, the D3D11 submission with its GX workers) 2.2 ms. Unlocked
(`original-unlock.flag`: 360 requested and interval 0, in memory): **159 frames/s at m45 (6.3 ms), 220 at Firelink walking
(4.6 ms)** (06, 07), in a 1280x720 window.

Against the native engine thread at m45 (run 89, the same spin function): its frame step 0x140EDAC5E 9.5 ms against the
original's 4.3; the draw step 7.3 ms (our collect 4.3 inside) against 2.2; the effects update 0x140E6E8D6 0.38 against 0.13;
the Scaleform update 0x1408F55AB 1.05 against 0.79 ms.

## Pre-merge review (Opus 5.5, D-0176)

No High finding. M1 (the material compare keyed by the frame serial, which repeats if a collect runs twice before `Clear()`):
keyed by the scene uid. M2 (the probe's unlock wrote the game's memory from study code): written after the pacer call, on a
committed writable page only, reference runs only, its source imported (timing/). L1 (the pool's A/B knob in a shared header):
removed after the A/B showed no gain. L2 (a reflection admission zeroed the collecting thread's pending marks): only the pool
workers' are dropped. L3 (the open-addressed set reads a null key as seen): unreachable, every caller checks the programme.
L4 (the pacer's fourth argument): nothing touches it before the call. L5, L6: housekeeping, harmless. The step-4 change
(packing after the lock) came after the review: the entries are made under the lock by the one thread that planned the
programme and read by nobody before the scene's publication (guard run 94).

## Uncertainty / open

- One run per scene and build; the walk and sprint are 4 s out and back near the bonfire.
- The native frame step is about twice the original's whole main thread: the next tranche starts from that comparison (run 89
  against original-05: the draw step 7.3 against 2.2 ms; inside it, our collect 4.3 ms, the UI 1.4 ms, the rest of the game's
  draw step), the pass rows kept per model, the walk pipelined into the admission, the UI production and the effects' update
  off the engine thread; while moving, the scene and view blocks packed per programme every frame (ER binds one scene block
  per pass).
- Sprint above 60 frames/s needs the LTF/TRTF gameplay fixes (timing/ I-0007, I-0013) when the unlocked rate is offered.
