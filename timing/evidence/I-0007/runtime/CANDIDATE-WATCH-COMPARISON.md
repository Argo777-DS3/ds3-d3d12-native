# Candidate watch: accepted 60/165 localization pair

## Observation

Target 165 session `20260919T170924Z-pid8088` uses the exact same watch DLL and
injector hashes as [target 60](CANDIDATE-WATCH-60.md). It captures 1,482 hits
over 494 consecutive intervals, with the same three-write sequence at each
interval, one candidate address and thread, no overflow and valid code bytes.
All 494 four-lane additions and subsequent next-update XYZ endpoints match
bit-exactly. The final copy preserves the first and third lanes in all cases.

Cadence is 164.607511 updates/s and 164.426613 Presents/s; median delta is
0.00606059982 s. All 495 measurement rows have valid input, position and
animation, with animation ID `300020210`. Timing restores exactly
`165/1 -> 30/0`; COMPLETE reports hooks disabled and debug registers restored.
The stopped process was subsequently observed closed. No causal patch applied.

## Newly narrowed calculation

The middle writer's XMM6 is loaded at `01669A29` from the result of the call
to `02064FF0` at `01669A1E`. The same result supplies XMM10 (scalar time)
at `01669A23` and XMM13 (a vector) at `01669A2D`; neither XMM6 nor XMM13 is
changed on the path to the observed write.

For every one of 180 target-60 and 494 target-165 intervals:

`XMM6 = f32(XMM13 * XMM10)` on all four lanes, followed by the verified
`candidate_after = f32(candidate_before + XMM6)`.

Thus XMM13 has the mathematical role of a velocity in this local calculation;
this is not a gameplay-system label. XMM8 (remaining scalar time) is zero at
every observed middle write. The returned time matches broad delta bit-exactly
on 180/180 and 479/494 intervals, respectively. The other 15 target-165 rows
differ by at most 7.1004 microseconds, with time/delta in
`[0.998829803, 1.001171567]`. This bounded discrepancy is retained, not called
exact equality; it does not explain a factor of approximately one half.

| Quantity | Target 60 | Target 165 | 165/60 |
|---|---:|---:|---:|
| XMM6 XYZ path norm per accumulated delta | 5.92714464 | 2.96264352 | 0.49984330 |
| XMM13 XYZ norm, delta-weighted | 5.92714464 | 2.96264324 | 0.49984325 |

Single-run paths and facing differ (reference facing change 0.218 radian;
165 change 0.04335 radian). These captures establish exact local dataflow and
the reduced vector, not a replacement for the D-0005 acceptance baseline or
patch acceptance. First-three-memory-lane XYZ norms avoid inherited axis-label
ambiguities. Animation continuity is only bounded by the existing sentinels.

## Interpretation and next discriminator

The local addition and time multiplication transmit an already reduced vector.
They are not a causal patch target. It remains unproved whether `02064FF0`
receives the reduced vector or creates the loss internally.

Its caller copies `nested+0xA0` to input context `+0x10` at `016698D7/016698E9`,
remaining time to context `+0x40`, then dispatches `01650F00` before calling
`02064FF0`. That callee initializes output vector `+0x10` from input `+0x10`,
and iterates records from input `+0x48`, count `+0x50`, potentially changing
the vector through `02065FE0`. Its integration at `020652DF..020652EA` is
`out.displacement += step_time * out.vector`. No first causal formula is
established yet.

The next minimum observation captures the exact call inputs/results and the
actual writer of `nested+0xA0` within the qualified transition. It replaces,
rather than repeats, the now-complete candidate watch. See
[increment producer protocol](INCREMENT-PROTOCOL.md).

## Reproduction and evidence

- [Exact comparison analyzer](compare_candidate_watch.mjs).
- [Comparison output](analysis/candidate-watch-60-vs-165.json).
- [Target-165 analysis](analysis/20260919T170924Z-pid8088/candidate-watch-summary.json).
- [Target-165 raw session](sessions/20260919T170924Z-pid8088/session-info.txt).
- [Selected callee decompilation](../static/increment-producer/DS3-1152-F-02064FF0.c)
  and [assembly](../static/increment-producer/DS3-1152-F-02064FF0.asm.txt), exported
  read-only from the exact I-0002 Ghidra project using I0002ExportSelected.
- Raw watch SHA-256:
  `F0B5D46498E7273B2ADE2BE623CBB2D7F8AC4F656622B615192E4F895E45E296`.
  All other raw hashes are in the session's `analysis-summary.json`.
