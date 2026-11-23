# Accumulator activation observer — I-0007 cycles32–33

Observation-only derivative of the preserved velocity mirror. No recurrence,
distance, source-vector or final-writer patch. Only the existing reversible
timing60/1 override is enabled for acquisition. Four execution observations:

| Slot | RVA | Purpose |
|---|---|---|
| DR0 | 009DF1B1 | Qualified owner+2D0 call, input state and command delta |
| DR1 | 009DF1B6 | Returned accumulator before downstream reset |
| DR2 | 009DF1F1 | Immediate consumer addition, before clearing activation flag |
| DR3 | 009DC04F | Fallback multiply, live broadcast factor and caller |

Observe only the existing selected owner/thread and bounded dispatcher interval.
Fallback RCX-2D0 must match the owner; its stack return at RSP+28 must be009DF1B6.
The other sites use RBX. Check exact instruction bytes before arming. All four
slots use execution/RF; no debug watch writes memory. Existing failure/overflow
and exact debug-state restoration remain. Configuration rejects non60 targets
and measurement beyond12s; one trial maximum, automatic stop at measure end.

Raw schema `I-0007-accumulator-observer-v1`, `accumulator-observer.csv`.
`owner_70_bytes` covers owner+70 through+2EF. `additive_2e0_bytes` is the four
accumulator lanes. Input bytes: timing16 at0, pointed-object32 at16, disk-global
live factor/cutoff at48/52, command delta at56, local outgoing vector at64,
fallback caller at80. Pointer identity is available in the owner snapshot.
`output_bytes[0:16]` is the local outgoing vector at each site. Other lanes are
reserved zero. GPR/XMM, code windows, ordering and broad input/position remain.
The analyzer pairs before/fallback/after/consumer, verifies the exact float32
fallback, downstream reset and addition, and refuses a zero-only bug claim.
It does not claim forwarded delta equals time between successive calls.

## Distinct acquisition question

Does natural movement activate a nonzero accumulator, followed by the0.65
fallback reaching the outgoing vector? Old1976 snapshots are all zero; no
accepted sprint/roll/candidate capture is repeated. A slope/flat transition is
an activation-search stimulus, not yet a proved way to activate the branch.
No165 comparison unless a nonzero fallback is first qualified at60.

## Announced operator protocol

Fresh exact-build offline process, no other probe. Thomas identified a nearby
walkable slope joining flat ground without a jump/drop. Window observation
shows the character on inclined rocks; no hostile activity visible. Thomas
chooses a safe route onto flat ground. Input automation remains unqualified.
Once READY is verified: press F4 once, stay still1 second, walk from the slope
onto the flat within roughly8 seconds, release all movement and stay still to
the12-second capture end. No sprint, roll, jump or forced collision is needed.
Automatic stop must produce COMPLETE; verify timing/debug restoration and
reset config. Do not reinject in that process. A zero-only result is insufficient
information, not permission to repeat the same route or apply a damping patch.

## Validation and resume

`build.ps1`, `test_write_watch.ps1`, `node analyze.mjs --self-test`.
Native test exercises all four execute traps and exact debug restoration.
Analyzer tests exercise nonzero decay, threshold clearing all lanes, full
consumer pairing, missing branch and corrupt-output rejection.
`BUILD-VALIDATION.json` records immutable source/binary/test identities.
For a stopped session: `node analyze.mjs <session-directory> <analysis-directory>`.
No live validation is implied by these offline tests.
