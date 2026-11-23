# I-0088 - Fil moteur vers 120 images/s, tranche 3

Capability: C3 (Wave 2, performance: the engine thread toward 120 frames/s, D-0200's order done, ROADMAP)

## Question

The native work on the engine thread at m45 is about 5.5 ms a frame (collect 4.07, UI prepare 1.38, end of frame 0.07,
I-0078 run 93), beside DS3's own update (about 4.7 ms): at 120 frames/s the whole frame has 8.3 ms, so the native share must
fall to about 3.6 ms or less. Which of the I-0086 plan's changes (ER-STUDY §9: P0 to P9), made after Elden Ring's architecture
without its defects (D-0195, I-0080), bring it there, measured unlocked, standing and in motion?

## Scope

Included, in the plan's order: P0, the engine thread measured unlocked (the game's rate raised and Present(0), as
`original-unlock.flag` does for the original); P1, the UI's display (DS3's `E52EE0` per movie and our HAL behind it) off the
engine thread (done on its own worker beside the collect), as ER runs its UI task beside the scene render and plays its Display
closure on the render thread, and DS3 queued the display on GX's command queue; P2 with P3, the
collect's independent parts as concurrent tasks with per-group joins, and the per-frame memory without the general heap; P7,
DS3's limiter without its spin; then P4, P9, P5 and P6 only on measurement. Each change measured before and after (p50 and
p99, standing at m45 and in motion at Firelink, measure mode without the debug layer, the sampler where it decides), its
picture compared, the guards and the debug layer run.

Excluded: DS3's own update (its timeline is fixed in its image; P9 only removes our observers' cost on it); the gameplay
fixes an unlocked rate needs (LTF/TRTF, timing/); the GPU side (bindless, more queues: once the CPU is no longer the limit).

## Inputs

I-0086 ER-STUDY (`investigations/I-0086-er-engine-thread-study/ER-STUDY.md` on the study branch `claude/er-engine-thread-study`, 243bf355, not
merged: its §3 for the UI, §5 for the frame memory, §8 for ER's defects, §9 for the plan); I-0077 and I-0078 (the collect's
first two tranches, the original measured as the reference); I-0079 and I-0081 (the stutter tools); I-0084 (the limiter spin);
MENUS-MAP for DS3's UI path; `decompiled/DS3-1152-F-00E52EE0.c` and `-00E5B940.c` (the per-movie display and its selection).

## Hypotheses

- H1: unlocked, the native engine thread's frame is bounded by DS3's update plus our native work, not by the render worker
  or the GPU, at m45 and on the Firelink walk (P0 decides it).
- H2: the UI's display can run off the engine thread with the movies' own locks (DS3's `E52EE0` takes them) if the worker's
  UI part ends before DS3 next touches the movies' lifetime; the engine thread's UI stamp then falls under 0.1 ms with the
  same pictures.
- H3: the collect's engine-only serial parts (set-up, effects record, lights, menu views, the shadow-caster walk) overlap the
  walk and admission chain when run as tasks with per-group joins, and the per-frame heap traffic goes with recycled storage.

## Expected evidence

The engine thread's stamps (`native_collect_split` stamps, `Get-CollectSplit.ps1`), frames/s and present intervals (p50,
p99, `native_frame_times`, `native_present_times`) unlocked and capped; the sampler (`sample-profile.flag`,
`Get-SampleProfile.ps1`) before and after; the guards (`record-revalidation.flag`) and the debug layer; the pictures side by
side with the original; the replayed path gate with its memory leg at the merge (D-0202, D-0204).

## Exit conditions

- Success: the engine thread's native work at m45 at or under 3.6 ms (or the frame at 120 frames/s unlocked at m45 and on the
  Firelink walk), with no visual regression, guards at 0 mismatch, 0 validation error; Opus step and final reviews; the gate.
- Partial: a measured gain short of the target; the remaining items replanned in RESULT.
- Refuted: a change that measures no gain is not kept (Optimise, do not stack).
- Stagnation: no new discriminating evidence after 3 cycles.
- Budget exhausted: close the tranche and replan.

## Safety and human interaction

Live runs under D-0116/D-0117, announced 5 minutes before with the PC clock, offline, the Shebang zone saves (Alpha
untouched), measure mode without the debug layer for measurements; one live test at a time on the shared PC (the coordinator
relays the other threads' slots); builds into this thread's own folder, hidden (D-0199).
