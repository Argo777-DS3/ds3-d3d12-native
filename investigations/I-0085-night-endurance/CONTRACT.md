# I-0085 - Endurance de nuit

Capability: C3 (Wave 2, stability, D-0200 fifth item)

## Question

Over one to two hours of zone travels in one native process (measure mode, no debug layer), does anything grow without bound
or drift: the process's private memory, the local and non-local video memory against their budgets, the streamed heaps and
staging, the CPU descriptors, the pending retirements, the pipelines; do the frame times of the same zone degrade from the
first visit to the last; and does any rare fault (a crash, a GPU hang, a device loss) appear that the short runs never reach?

## Scope

Included:

- A long run: the five Shebang witness zones (Beta m31 Undead Settlement, Gamma m37 Irithyll, Delta m50 Dreg Heap, Epsilon m45
  Ariandel, Zeta m51 Ringed City; Alpha untouched) loaded from the title in a loop for 90 to 120 minutes, each stay with camera
  turns, then a quit to the title; the window closed at the end with `exit-dump.flag` (the pre-existing exit heap fault
  0xC0000374 dumped for its diagnosis, D-0201).
- The tooling it needs: the trace's 900 s bound raised for this run (the controller and `Run-StartupTrace.ps1`), a tour that
  checks each load in the journal (`native_present_pacing` content draws) instead of trusting fixed waits (I-0081's tour missed
  every other zone, i81-19), and a reader that splits the journal by visit and fits the trends.
- I-0084's deferred check: the five witness zones once each under GPU-based validation (H1 beyond Firelink).
- Fixes the runs call for, when they are bounded (a leak with its owner found, a counter missing); a leak or drift whose cause
  needs a redesign is recorded with its numbers and placed.

Excluded:

- The engine thread's third tranche (120 frames/s, DS3's limiter spin), the next workstream.
- A real TDR and other GPUs (I-0084: not on this PC).
- Walking through the zones (combat, deaths and the save's state stay out of an unattended run; camera turns stand for the
  view changes).

## Inputs

- I-0084 RESULT (the counters: private, local and non-local memory with their budgets in `native_present_pacing`; the budget
  reaction rows `native_video_memory_over_budget`), I-0081 RESULT observation 5 (memory level over five zone loads, the tour's
  missed loads), I-0079 (`native_streamed_memory`), I-0082 (minidumps, `exit-dump.flag`), I-0083 (the gate, D-0202).
- Tools: `Run-StartupTrace.ps1`, `Pilot-TitleContinue.ps1`, `Pilot-Game.ps1`, I-0081's `Run-ZoneLoop.ps1` and
  `Pilot-ZoneTour.ps1` (the navigation clicks), `native/frame/Build-GameBootstrap.ps1`, `scripts/Invoke-ReplayGate.ps1`.

## Hypotheses

- H1: the title-state private and local memory have no upward trend over the visits (a slope under 5 MB a visit, against the
  1.57-1.67 GB title level of i81-19).
- H2: the streamed heaps, CPU descriptors, pending retirements and pipelines return to a level at each title visit (no
  monotonic growth after the first loop).
- H3: a zone's frame times (p99, frames over 25 and 34 ms in the stay) on its last visit are within the spread of its first
  visits (no drift).
- H4: no fault in the long run; the witness zones pass under GPU-based validation without an error or warning.

## Expected evidence

The long run's journal and tour log under `evidence/I-0066/artifacts/i85-*`, the reader's table per visit and its fitted
slopes, the validation run's journal, and the exit dump's stack if the heap fault occurs.

## Exit conditions

- Success: the long run complete (90 minutes or more, every zone visited at least eight times), each hypothesis judged with
  its numbers, the bounded fixes landed and checked, step and final Opus reviews applied, the replayed path gate passing on the
  final tree.
- Refuted: a hypothesis contradicted (a leak or drift found), recorded with its owner and either fixed here or placed.
- Insufficient information: the tour cannot keep loading zones unattended (recorded with its screenshots).
- Stagnation: no new discriminating evidence after 3 cycles.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`.

## Safety and human interaction

Live runs under D-0116/D-0117 (offline, protected files restored by hash, nothing installed in the game folder), each announced
in the thread at least 5 minutes before with the PC clock, one live test at a time, the Shebang zone save (Alpha untouched);
measurement runs without the debug layer; Thomas is away until 07:00 and left the PC to the tests (2026-10-21 20:53).
