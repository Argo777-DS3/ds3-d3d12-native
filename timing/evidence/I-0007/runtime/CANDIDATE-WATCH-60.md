# Selected-candidate writer discovery at target 60

Session: `20260919T170034Z-pid9748`. Result: accepted for writer localization;
not a causal fix or a new acceptance baseline.

## Evidence

One complete sprint trial has 181 broad-update measurement snapshots with
100% valid input, position and animation fields, animation ID `300020210`.
The armed transition covers 180 consecutive intervals, paired with the next
broad-update snapshot using the canonical I-0006 N-to-N+1 rule. Full-session
update/Present rates are 59.925666/59.925660 Hz, median delta 0.0166666005 s.
Timing restoration is exactly `60/1 -> 30/0`; COMPLETE reports hooks disabled
and debug registers restored. All three installed hooks reported MH_OK.

All 540 watch hits share one thread and one candidate address, have valid code
bytes and no overflow. Every interval has exactly the same ordered writes:

1. `00DDA89E`, inside `00DDA870`: `MOVAPS [RAX+0x70],XMM0`, copy to nested
   candidate. Post-instruction RIP is `00DDA8A2`.
2. `01669D6A`, inside Ghidra function `016693F0`: `MOVAPS [RDI+0x70],XMM0`.
   The preceding `ADDPS` at `01669D4E` adds the existing candidate to a vector.
   The vector is held in XMM6 at the observed hit; post-RIP is `01669D6E`.
3. The same `00DDA89E` copy again. It preserves the middle write's first and
   third lanes in all 180 intervals, while the second lane can change.

The middle writer reconstructs bit-exactly on all four lanes as
`candidate_after = f32(candidate_before + XMM6)` for 180/180 intervals.
Both addressing modes resolve to the watched nested candidate for all hits.
The final copied XYZ matches the next broad-update position bit-exactly for
180/180 intervals. These checks are reproduced by
[`analyze_candidate_watch.mjs`](analyze_candidate_watch.mjs), with outputs in
[`analysis/20260919T170034Z-pid9748`](analysis/20260919T170034Z-pid9748).

Runtime unwind metadata starts at `016695A9` for the middle writer, while the
static containing function starts at `016693F0`. The latter includes that
secondary prologue; this is not evidence of an opaque or separate producer.
Raw session metadata retains the inherited I-0006 schema label. The I-0007
condition, filenames and unchanged manifest binary hashes identify the probe;
raw evidence is preserved as emitted.

## Interpretation and next step

The persistent-candidate producer is now observed directly. Its local add is
correct at 60; the first wrong value and cadence-dependent formula remain
unproven. XMM6 is loaded from a local result after `02064FF0` in this static
function, so it is the next input to compare rather than patching the add.
First perform the announced unchanged-probe 165 mirror, then reconstruct the
smallest upstream calculation discriminated by the pair. No patch applied.

The route changes facing by approximately 0.218 radians. This does not
invalidate writer identity or exact same-update arithmetic; scalar travel
comparisons must retain the canonical projection and anchor caveats and must
not promote this single run to a new sprint acceptance baseline.
