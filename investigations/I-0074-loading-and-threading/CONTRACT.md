# I-0074 - V2 loading and threading after Elden Ring (D-0195)

Capability: C3, wave 2 (ROADMAP « Loading and threading »). Opened 2026-10-20 06:17 Paris from the I-0071 checkpoint (STATE
« Next action » (a) and (b)); the ER study it relies on is I-0073 (branch
`claude/youthful-khorana-5caa1f`, in progress in its own thread).

## Question

What holds DS3's engine thread at the frames where models and textures arrive (the hitches left after I-0071: Firelink walk
frames of 55-150 ms whose native collect is 4-5 ms), and how is that work moved off it, and the render worker's prepare and
recording split into chunks on workers (I-0071 point 3c), after Elden Ring's architecture?

## Known inputs

- OBSERVED (counters `native_program_analysis`, every I-0070/I-0071 run, 2026-10-20): the encoded texture decodes never run
  on the engine thread (`texture_decodes_owner` 0 of 2,242-4,614 a run) and cost 350-455 ms a whole run. The I-0070 RESULT's
  wording « texture admissions decoded on the engine thread » is therefore not supported; STATE's (a) is reframed as the
  question above.
- OBSERVED (i70-native-47, `native_slow_frame`): frames of 59-151 ms with collect 0.7-5.1 ms, prepare 2.3-4.8, render 0.9-2.3,
  13-42 textures admitted in the interval: the time is spent on the engine thread outside the native collect.
- Each encoded texture admission copies its whole TPF container through `ReadProcessMemory` (15.5 GB a Firelink walk, 6 MB
  an admission) and duplicates the decoded image into the record (`content`); uploads go on the direct queue through ingress
  lists (B3a), with no copy queue.
- I-0071: prepare 5.4-5.6 ms and recording 4.2-4.7 ms at m45 on the render worker, the longer path for 120 frames/s.

## Hypotheses

- H1: the engine thread waits at the slow frames for the loader threads (an engine sync on tasks whose native texture or
  model admission is slower than the original's D3D11 creation).
- H2: it contends a lock or the process heap with the loader threads' admissions.
- H3: it runs native work itself at arrivals outside the collect (CPU geometry joins, gparam publishes, the non-encoded
  texture admissions, journal rows).
- H4 (point 3c): the prepare and the recording split into chunks recorded on workers into their own lists, executed in
  chunk order, shorten the render worker's path by about the worker count, the pictures unchanged.

## Discriminating evidence

A sample profile of the engine thread restricted to the slow frames (a frame mark in the samples), with the loader threads
sampled too, on the Firelink walk (I-0070 key path) in measure mode; per-thread admission times. For 3c: the m45 prepare and
recording before and after, pictures against `main`, the guards.

## Scope

In: the native texture and model admission path, the render worker's prepare and recording, the sampler's diagnostics, live
runs under D-0116/D-0117 (announced 5 minutes ahead). Out: the effect contracts (D-0190), bindless binding, the gameplay.

## Exit conditions

- Success: the arrival hitches' cause named with evidence and removed or reduced (frames over 34 ms on the Firelink and
  Archdragon Peak walks against `main`), point 3c measured, no guard mismatch, pictures equal, reviewed and merged.
- Refuted / insufficient information: recorded with the evidence that could not discriminate.
- Stagnation: no new discriminating evidence after 3 cycles. Budget: 10 cycles.
