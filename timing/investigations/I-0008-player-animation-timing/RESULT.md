# I-0008 result - Player animation temporal path

## Conclusion

Complete at cycle 12/12 with bounded success. The investigation binds the
qualified player animation clock to the exact `SprjChrTimeActModule` ring
writer, reconstructs the native `01284D70` timing formula, and captures the
actual target-165 generator/control input-to-output chain. On the observed
stationary-player mode-1 path, the native argument is the same variable delta
as central timing `+0x268`, never fixed `1/60`, and all 495 results reach the
ring bit-for-bit.

The retained 60/90/120/165 evidence supports a ten-update ring refresh cadence.
At target 165, 3,429 of 3,476 stable intervals match the preceding ten measured
deltas within `1e-5`; each of the remaining 47 is closer to that variable sum
than to `10 * (1/60)`. Hidden fixed-60 substitution is therefore refuted on the
captured path and unsupported by every retained target-165 residual. This is a
bounded timing-path result, not whole-animation or whole-C4 acceptance.

## Evidence and results

- [Contract and cumulative plan](CONTRACT.md).
- [Owner, exact writer and retained refresh contract](../../evidence/I-0008/CLOCK-CONTRACT.md).
- [Initial live capture and native formula](../../evidence/I-0008/LIVE-AND-FORMULA.md).
- [Target-165 generator/control result](../../evidence/I-0008/CONTROL-LIVE-RESULT.md).
- [Announced protocol and restoration gates](../../evidence/I-0008/CONTROL-OBSERVER-PROTOCOL.md).
- [Artifact locations, identities and hashes](../../evidence/I-0008/manifest.json).

Cycles 1-6 identify `modules+0x10` as a `SprjChrTimeActModule`, bind `+0x24`
and `+0x2C` to record-zero time/duration in a ten-record ring, locate writer
`009B4F8D/009B4F97`, and show that `+0xC0` advances modulo ten. All 7,371
retained stable ring intervals span ten broad updates.

Cycles 7-9 record 180 exact player ring insertions, identify the bounded native
formula at `01284D70`, and reproduce 51,840 finite native cases bit-for-bit.
That target-60 capture alone could not distinguish variable delta from fixed
`1/60`, so it did not close the input question.

Cycles 10-12 build and validate the four-site discriminator, then execute one
announced three-second offline target-165 capture. It records 4,149 events and
retains 495 unambiguous generator/control/ring chains at 165.010080145 Hz. All
chains share one player/module identity, mode 1, flags 0, speed 1 and duration
2. Generator XMM2 and control XMM1 equal central `+0x268` exactly, using
float32 deltas `0x3BC697FF` (492) and `0x3BC6DEBD` (3); both differ from
`1/60`. Native output replays and ring values match in 495/495 chains,
including one wrap. Timing restores `165/1 -> 30/0`, saved debug state is
reread exactly, the process later closes, and no live instrument remains.

## Remaining uncertainty

The 47 stable target-165 residuals span 16 clusters and several animation and
marker partitions. Their retained rows lack live control identity and mode, so
their precise transition, selection or alternate-branch cause is unknown. The
single live capture does not cover every animation mode, transition, identity,
actor or framerate. No animation patch is proposed.

## Artifacts produced

The Evidence Store contains exact-build static exports; retained cross-target
audits; native formula tests; disabled observer/controller sources and binaries;
the raw target-165 capture, session and restoration records; deterministic
analysis; and the joined residual summary. The manifest records their hashes
and provenance. Heavy artifacts remain routed from this branch and need not be
regenerated.

## Suggested next step

Do not repeat I-0008 to test fixed-60 substitution. A future investigation may
target transition or alternate-mode identity only if that uncertainty becomes
material to a concrete animation defect. I-0008 remains unmerged into `main`.
