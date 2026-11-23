# I-0079 - V2 : saccades et rythme d'images en mouvement (D-0195)

Capability: C3, wave 2. Opened 2026-10-21 02:32 Paris from the I-0078 checkpoint (STATE « Next action », validated by Thomas
on 2026-10-21 02:25 Paris, « Je valide »), in a fresh thread.

## Question

While the player moves, the native renderer presents frames over 34 ms that the original D3D11 renderer does not (I-0078
RESULT: Firelink walking 136 frames over 34 ms in 80 s, sprinting 116, the worst 66-69 ms; standing and turning the camera
smooth; Thomas sees no stutter in DX11). What does each slow frame spend its interval on, which of those causes are the
native backend's, and how does Elden Ring keep each of them off the frame (D-0195), so that walking and sprinting present at
the original's rhythm?

## Scope

Included: the Firelink walk and sprint (Run-Motion's paths), measured natively and in the original on the same save and
path; every slow frame classed by cause (a pipeline or shader built on the frame path, texture or buffer uploads and
admissions, model admissions entering the view, the engine thread waiting on the render worker or the GPU, Present and its
pacing, the loading threads); the corrections of the native causes, after Elden Ring's treatment of the same cause; a
precompile at start if on-the-fly pipeline builds are a cause (a minimal screen, D-0172's progress bar later).

Excluded: the engine thread's steady cost toward 120 frames/s (the third collect tranche, next workstream); the game's own
simulation hitches that the original shows too (measured, not corrected).

## Inputs

- I-0078 RESULT (the motion runs i78-motion-01..08, the original as reference i78-original-*), K-0461.
- I-0072 (the pipeline cache, K-0455), I-0074 (texture loading off the engine thread, K-0457), I-0070/I-0071 (persistence).
- Tools: `investigations/I-0078-engine-collect-120-b/Run-Motion.ps1`, `Run-OriginalProbe.ps1`, the in-process sampler
  (`sample-profile.flag`, `sample-profile-start.txt`, `native/frame/Get-SampleProfile.ps1 -SlowFrames`), the journal rows
  `native_slow_frame`, `native_frame_times`, `game_frame_times` (the same engine point under both renderers).
- PresentMon (NVIDIA FrameView SDK's `PresentMon_x64.exe`, 1.9) is on the PC but its ETW session needs elevation, which this
  thread does not have; the frame intervals are taken in process at the same engine point for both renderers instead.

## Hypotheses

- H1: on-the-fly pipeline builds cause slow frames while moving despite the cache. First reading (i78-motion-07/08, the
  rows over 50 ms): `pso_created` 0 in every slow frame after the load: likely refuted, to confirm over 34 ms.
- H2: the slow frames' time is outside the native phases (i78-motion-07/08: a 50-66 ms frame holds collect 4-6, prepare 2,
  record 1.4, GPU wait 0.1, present 0.2-4 ms), spent by the engine thread in the game's own step or waiting on something the
  native backend holds (a lock, a loader join, a resource admission on the engine thread).
- H3: the same engine thread work exists in the original but costs less there (the original's main thread 4.7 ms a frame):
  the cause is a native admission path met by the game's streaming while moving.

## Expected evidence

The engine thread's samples inside the slow frames against those of the ordinary frames of the same run (the sampler's frame
marks), the slow rows' cause fields, and the original's frame intervals on the same path (`game_frame_times`, the present
pacer probe). Before/after measures on the same save, path and build mode (measure mode, no debug layer).

## Exit conditions

- Success: walking and sprinting at Firelink present no more frames over 34 ms than the original on the same path (or each
  remaining one named as the game's own), 0 validation error, the same pictures.
- Refuted: the slow frames come from the game's own work, equal under both renderers.
- Insufficient information: the sampler cannot attribute the slow frames' time.
- Stagnation: no new discriminating evidence after three cycles.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`.

## Safety and human interaction

Live runs under D-0116/D-0117, each announced in the thread at least 5 minutes ahead with the PC clock read just before;
offline, hash-checked restore, nothing installed in the game folder; zone saves of Shebang characters (Eta at Firelink), never
Alpha.
