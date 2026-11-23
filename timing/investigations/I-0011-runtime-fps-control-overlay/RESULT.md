# Investigation result

## Conclusion

The correction controlled and validated here is **LTF — Locomotion Timing
Fix**. It is distinct from the later I-0013 TRTF.

Complete at cycle 12/12 on dedicated branch
`codex/i-0011-runtime-fps-control-overlay`. The complete offline gate passes for
one inert, exact-build runtime DLL and external emergency controller. It exposes
a compact HOME-toggle windowed overlay, a 30–360 integer slider, presets
30/60/90/120/144/165/240, QPC-measured actual FPS, target/native state,
LTF ON/OFF at the broad-update boundary, and exact stop
restoration.

The LTF implementation includes the accepted I-0010 formula/guard and
debug-restoration headers directly. There is no cadence profile, multiplier
table, per-action gain, Present patch or RCA-only animation instrumentation.

Genuine Unlimited has a bounded partial outcome: the recovered timing pair has
proved finite numeric (`target/1`) and native restoration (`30/0`) states but no
proved limiter-off representation. The visible Unlimited control returns
`unsupported` without writing state; it is not simulated with a high cap.

Both live sessions stopped cleanly with exact `30/0`, hook and debug-register
restoration and zero guard failures. The repaired slider produced six completed
numeric transitions rather than the first session's 213 intermediate writes.
The attempted window-focus/placement repair did not eliminate panel jitter or
make the pointer usable during ordinary gameplay; a DS3 menu remains required.
Thomas explicitly accepted those limitations because the overlay is a test
tool rather than the product UI, so no third UI patch is scheduled.

Thomas reported correct behavior for every player animation he observed across
the exercised framerates, explicitly including light sprint and light roll,
with no perceptible enemy slowdown. This is strong bounded operator support for
the broader temporal-branch theory, not exhaustive proof of every temporal
consumer or whole-C4 correctness.

## Evidence

- [I-0011 contract](CONTRACT.md).
- [Build validation](../../evidence/I-0011/prototype/BUILD-VALIDATION.json),
  [exact-build validation](../../evidence/I-0011/prototype/EXACT-BUILD-VALIDATION.json)
  and [ten-run restoration stress](../../evidence/I-0011/prototype/NATIVE-RESTORATION-STRESS.txt).
- [Formula/guard identity audit](../../evidence/I-0011/prototype/DERIVATION-VALIDATION.json).
- [Unlimited architecture assessment](../../evidence/I-0011/prototype/UNLIMITED-ASSESSMENT.md).
- [Prototype guide](../../evidence/I-0011/prototype/README.md),
  [overlay mockup](../../evidence/I-0011/prototype/OVERLAY-MOCKUP.svg) and
  [revised live protocol](../../evidence/I-0011/prototype/LIVE-PROTOCOL.md).
- [First live partial result](../../evidence/I-0011/LIVE-PARTIAL-RESULT.md) and
  raw session `20260920T142829Z-pid27872`.
- [Final live result](../../evidence/I-0011/LIVE-RESULT.md), supplied screenshot
  and raw session `20260920T144258Z-pid24832`.
- [I-0011 manifest](../../evidence/I-0011/manifest.json).

## Uncertainty / unresolved questions

- Overlay jitter and menu-required pointer interaction remain accepted test-tool
  limitations; product-quality input/render integration is unresolved.
- The operator's broad player-animation and enemy observations are qualitative;
  exhaustive subsystem invariance remains unproved.
- Unlimited remains unavailable through the recovered intervention. A distinct
  limiter-off engine mechanism, if one exists, is unproved and was not invented.

## Artifacts produced or modified

- Dependency-free Win32/GDI overlay runtime, controller, pure state machine and
  native test suite under `evidence/I-0011/prototype/`.
- Release DLL/controller, strict build provenance, exact-build and derivation
  validators, ten-run restoration evidence, overlay mockup, architecture
  assessment, operator guide and live protocol.
- Completed investigation status, Evidence Store manifest/index and paused
  resume state.

## Suggested next step

Checkpoint and integrate only on Thomas's direction. Any future product overlay
or whole-C4 claim requires a separately scoped investigation; do not continue
patching this test window.
