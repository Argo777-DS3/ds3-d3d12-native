# Source builder mirror: discriminate three stages together

Completed: [accepted result and scalar provenance](BUILDER-MIRROR-165.md).
The acquisition instructions below are retained for provenance, not repetition.

## Boundary and missing proof

The [velocity result](VELOCITY-MIRROR-165.md) qualifies the already-reduced
source copy. Old captures contain neither the source object's virtual target,
the intermediate source vectors nor its local scaling operands. Reading only
the final vector cannot recover which earlier stage created it.

Isolated `builder-instrumentation` uses the same three base hooks and the same
selected-thread `A1429C -> DE50EC` interval. Four hardware execution slots:

| Pre-instruction RVA | Observation |
|---|---|
| `005CA642` | Before virtual `+D8`, output at source-object `+1D0` |
| `005CA648` | Virtual output, before the next call's argument setup |
| `005CA65E` | After direct `009C8E80`, before later local scaling |
| `009DB9C1` | Final source, before the load/copy into selected owner `+E0` |

Filter RDI against the selected transform. For each hit, reconstruct source
object as `RBP-1D0`, verify its `+8 -> player+1F90 -> modules+68` chain ends at
that selected owner, and validate first/final call arguments. The instrument
does not assume the observed source object's absolute address survives a run.

`source-builder-mirror.csv` retains GPR/XMMs and:

- `input_bytes`: source-object `+1D0..1FF` (48 bytes), remainder zero padding;
- `output_bytes`: selected transform `+E0..10F`, which is still the old source
  before the final copy; not mislabeled as the virtual call's return value;
- `nested_bytes`: selected transform `+E0..FF` (inherited neutral container);
- `builder_bytes`: source object `+0..26F`, covering vtable, source and gates;
- `builder_25c_bytes`: `+25C..26B`; `XMM6.low` holds caller delta;
- builder/vtable/virtual-target addresses and 256 live code bytes at that
  target and at `009C8E80`. Bytes alone do not prove full function decoding.

Eight selected hits per window, 4096 total maximum; invalid reads or pointer
checks stop the capture. RF resumes all four execution traps. All six debug
fields, including DR3, are saved, restored and read back. No movement or game
files are changed. Target-FPS timing override remains reversible as before.

## Offline qualification and identity

MSVC `/W4 /WX` passes. Shared four-execute-slot test passes one hit per slot,
correct arithmetic/RF resumption and exact debug-state restoration. Analyzer
tests cover stage pairing, scalar discrimination, missing/invalid reads,
wrong owner/order and truncated bytes. Live DS3 qualification is still pending.

- DLL SHA-256: `3E7DDEB09B245EF531390786609405F282540244DC8A613795BF12467F386390`
- Injector SHA-256: `A8D0C01A351ECC2AD783809B42BD6B5E59F38AE3F7210F0448D9230485321764`

## Operator protocol

1. Announce before launch: this run distinguishes an already-reduced producer
   output from a later reduction; if reduced at producer exit, trace its live
   target, otherwise identify the first following reducing stage. This is
   one 165 capture, not an automatic new 60/165 pair.
2. Thomas launches offline at the same sprint anchor, active display 280 Hz,
   and says ready without F4. Verify fresh game process/exact hashes/no old
   probe. Set `Set-BuilderMirrorTarget.ps1 -Target 165`; inject the matching
   `I0007BuilderMirror.dll` with `I0007BuilderMirrorInjector.exe` and require
   READY, schema `I-0007-source-builder-mirror-v1`, condition
   `i-0007-builder-mirror-165`.
3. F4 then AZERTY Z+Space for six seconds; neutral controller, no camera.
   Thomas says done, leaving the game open for stop/restoration.
4. Stop with matching injector; reset `Reset-BuilderMirrorConfig.ps1` and
   require COMPLETE/exact timing restoration. Analyze with
   `analyze_builder_mirror.mjs <session> <analysis>`, plus existing locomotion
   and cadence analyzers. Preserve raw data even if a gate fails.
5. Only then ask Thomas to close DS3. Continue reconstruction before another
   acquisition; a new target-60 observation requires a precise missing proof.

All four I-0007 configurations are disabled at the prepared checkpoint.
No causal patch; eventual sprint 60/165 and unchanged-patch roll validation
are still required, regardless of how many RCA captures remain.
