# Increment input mirror at target 165

Session `20260919T172639Z-pid1696` is accepted for the input boundary, not for
causal patch validation. The existing 60 reference is reused; no new 60 run.

## Observations

- 1,976 hits, 494 consecutive four-hit windows, one selected object and thread.
- Ordered hits: setter post-RIP `00DDA785`, call `01669A1E`, return site
  `01669A23`, feedback store post-RIP `01669FCB`.
- Input vector equals `nested+0xA0` on all 494 calls. Output vector equals
  input bit-for-bit on all four lanes, and displacement equals the float32
  product of output vector and returned time on all 494 calls.
- Record count passed to `02064FF0` is zero on every call. Its record loop and
  `02065FE0` are inactive in this accepted sprint capture.
- Input/output XYZ norm weighted by input time is 2.96223840583, consistent
  with the previous 165 output (~2.96264), versus existing 60 output ~5.92714.
  This comparison is localization, not a new acceptance baseline.
- Setter `00DDA77E` copies `[RDI]` through XMM0 into `[RAX+0xA0]`. All 494
  setter XMM0 values equal the subsequent input bit-for-bit. The feedback
  store `01669FC3` similarly preserves the output in all 494 intervals.
- Every setter has `RAX+0xA0=watched address` and `RDI=RBP-0x69`. This matches
  the local source used by `009DEE60` at its setter call; other known direct
  callers only supply zero vectors. It strongly binds that upstream caller,
  without claiming a captured return address (none was recorded).

Full-session cadence is 164.202159 updates/s and 164.034262 Presents/s, median
delta 0.00606059982 s. The 495 measurement rows have 100% valid position,
animation and input, animation ID `300020210`, no aborted trial. All raw reads
and code bytes are valid, no overflow. Timing restores exactly `165/1 -> 30/0`;
COMPLETE reports hooks disabled and debug registers restored. This binary
also reads back each debug-register restoration. DS3 was subsequently observed
closed. All instrument configurations are disabled. No causal patch applied.

## Interpretation and continued upstream reconstruction

`02064FF0`, its no-record integration path, and both observed setters transmit
an already reduced velocity. Do not patch them. This excludes the need to
reacquire a 60 input mirror merely to establish transmission in the 165 run.

The source caller `009DEE60` obtains a vector from owner `+0xE0` via
`012BCFB0`, then normally multiplies it by the inverse-delta supplied by
`009DC090`. Additional terms and conditional replacement branches exist:

1. `009DEF77` calls `012BCFB0` on owner `+0xE0` and a local four-lane orientation.
   The callee's exact SIMD rotation polynomial is exported; its constant at
   `1446803E0..EC` is four float32 values of 0.5. Orientation normalization in
   the live call is not established yet.
2. Optional owner flag `+0x160` can replace/clamp that transformed vector.
3. `009DF0C7..009DF0F7` loads inverse delta, multiplies the vector and adds
   owner `+0x180..188`; another branch adds masked owner `+0x2E0`.
4. Flag-dependent paths may replace it with owner `+0x120`, clamp/scale that
   alternate vector, or add another delta-scaled term before the setter.

The old register snapshots do not contain owner `+0xE0`, the local orientation,
or these branch flags at their decision points. Therefore the first wrong
input/operation cannot be reconstructed uniquely from the current captures.
The next [velocity source mirror](VELOCITY-PROTOCOL.md) captures those exact
stages in one target-165 run, reusing all established downstream evidence.
It also watches owner `+0xE0` in the same bounded transition; no observed write
would mean the producer lies outside that window, not that the field has no
producer. Continue upstream if its input is already reduced.

## Evidence

- [Analysis](analysis/20260919T172639Z-pid1696/increment-summary.json) and
  [paired values](analysis/20260919T172639Z-pid1696/increment-pairs.json),
  produced by [the analyzer](analyze_increment_mirror.mjs).
- [Raw session identity](sessions/20260919T172639Z-pid1696/session-info.txt).
  Raw mirror SHA-256:
  `84963C6ACB3E72A90CA5C2C72264EFCFEAC94F37D699124A99F376009DDE4797`.
- [Setter references](../static/velocity-producer/setter-xrefs.csv),
  [caller](../static/velocity-producer/DS3-1152-F-009DEE60.c),
  [caller assembly](../static/velocity-producer/DS3-1152-F-009DEE60.asm.txt),
  [rotation assembly](../static/velocity-producer/DS3-1152-F-012BCFB0.asm.txt),
  [exact constant](../static/velocity-producer/rotation-constant.csv).
- Existing I-0005 `009DC090`, `009DC740`, `009DA810` and `00DDA750` exports were
  reused read-only. The `009DA810` owner `+0xE0` store is initialization to zero,
  not a demonstrated active sprint producer. No broad field-user scan performed.
