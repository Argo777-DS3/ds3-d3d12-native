# Investigation result

## Conclusion

Phase 1 implementation is successful offline and remains active pending
Thomas's manual test. The compiled ReShade add-on exposes cadence controls plus
independent `LTF — Locomotion Timing Fix` and
`TRTF — Terrain Response Timing Fix` toggles. It starts Native/LTF OFF/TRTF OFF.

LTF directly includes the accepted I-0010/I-0007 formula and debug-restoration
headers. TRTF is a separate ReShade-free core preserving I-0013's accepted
branch order, arithmetic precision, native-reference no-write rule, additive
and decay laws, and reset/clamp passthrough. Their requests, active state,
telemetry, intervention sites and restoration are independent. The shared layer
only orchestrates lifecycle and cadence.

The settings surface remains available through ReShade. A runtime-thread
keyboard fallback supplies the same essential controls independently of
ReShade swap-chain runtime recreation, addressing I-0012's exclusive-fullscreen
host limitation without changing either correction. This is structural offline
qualification; exclusive-fullscreen behavior still requires Thomas's manual
confirmation.

## Evidence

- [Offline validation](../../evidence/I-0014/OFFLINE-VALIDATION.md).
- [Build validation](../../evidence/I-0014/prototype/BUILD-VALIDATION.json).
- [Formula, separation and host validation](../../evidence/I-0014/prototype/DERIVATION-VALIDATION.json).
- [Exact-build/signature validation](../../evidence/I-0014/prototype/EXACT-BUILD-VALIDATION.json).
- [PE/add-on validation](../../evidence/I-0014/prototype/PE-VALIDATION.json).
- [Ten-run lifecycle/restoration stress](../../evidence/I-0014/prototype/NATIVE-RESTORATION-STRESS.txt).
- [Operator guide](../../evidence/I-0014/prototype/README.md).
- [Reversible installation validation](../../evidence/I-0014/INSTALL-VALIDATION.md).
- [Evidence manifest](../../evidence/I-0014/manifest.json).

## Uncertainty / unresolved questions

- No game was launched. The add-on is not yet live-qualified as a combined
  binary; Thomas's first manual test is the next gate.
- Exclusive-fullscreen support is structurally independent of the graphics
  runtime, but actual fullscreen cadence and control behavior remain unobserved.
- The manual distance comparison is deliberately uninstrumented. No conclusion
  about distance, speed or LTF generality is made.
- Existing LTF/TRTF validation limits remain unchanged. This is test tooling,
  not a distribution or whole-C4 result.

## Artifacts produced or modified

- `evidence/I-0014/prototype/`: combined ReShade host, independent correction
  cores, strict build, validators, one-file manager and manual test guide.
- `DS3UnifiedTimingFixTest.addon64`: x64 ReShade API-20 add-on with three exact
  runtime hooks and hash-guarded install/removal.
- I-0014 contract, interim result, evidence manifest and resume checkpoint.

## Suggested next step

Thomas manually checks 60 and 240 with LTF ON/TRTF OFF, then optionally repeats
with LTF ON/TRTF ON, in windowed and exclusive-fullscreen modes. Do not add a
distance probe or alter LTF before that report.
