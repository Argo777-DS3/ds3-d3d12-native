# Velocity source mirror: one targeted 165 capture

Completed: [accepted result and next upstream boundary](VELOCITY-MIRROR-165.md).
The steps below preserve the acquisition protocol, not a request to repeat it.

## Why a new observation is needed

The [increment mirror](INCREMENT-MIRROR-165.md) closes the downstream input
boundary. Existing captures omit owner `+0xE0`, orientation operands and the
branch flags that choose/replace the velocity in `009DEE60`. Those values
cannot be uniquely recovered from the final vector. Reuse the old 60/165
outputs; no automatic new target-60 control.

## Minimal discriminating surface

Isolated `velocity-instrumentation` fork, same three scheduler/broad-update/
Present hooks, same selected-thread `A1429C -> DE50EC` window. Replace the old
observations with four hardware slots:

1. Execute `009DEF77` before `012BCFB0`: source owner `+0xE0`, orientation at
   `RBP-0x39`, time descriptor at RSI. Require `RBX=selected owner`,
   `R8=owner+0xE0`, `RDX=RBP-0x39`, `RCX=RBP-0x49`.
2. Execute `009DEF7C`: same owner, capture transformed output at `RBP-0x49`.
3. Execute `009DF3E6`: same owner, capture local result at `RBP-0x69` before
   the A8 setter. Retain `RBP-0x49` to detect optional replacement/clamp.
4. Write-only, four-byte watch of selected owner `+0xE0`; records actual
   post-write instruction and full source vector if a write occurs in-window.

All hits retain GPRs/XMMs, code bytes, update identity, source `+0xE0..FF`,
the bounded owner slice `+0x70..1FF` covering operands/flags read by this
caller, and additive vector `+0x2E0..2EF`. On execute hits, `input_bytes`
contains source vector at offset 0, orientation at 16, time descriptor at 32
(delta/inverse at offsets 40/44). `output_bytes` contains transformed local
vector at 0 after stage 1 and the final velocity at 16 only at stage 3.
Unused bytes are zero, not evidence of actual values. The inherited
`nested_bytes` column now holds owner `+0xE0..FF`, as identified by this schema.

Limits: eight selected hits/window, 4,096 total; pointer/read mismatch or
overflow stops. RF resumes execution traps; DR6 distinguishes pre-instruction
execution traps from post-write traps. All six debug-state fields are saved,
restored and read back. All four slots are restored, including original DR3.
No locomotion values, final writer or game files are patched.

## Offline qualification

MSVC `/W4 /WX` build passes. Shared DR7 helper test passes three execution
traps plus one data-write trap, correct RF resumption, expected arithmetic and
exact debug-state restoration. `analyze_velocity_mirror.mjs --self-test`
passes float32 identity/yaw reconstruction and truncated-byte rejection.
These do not constitute live DS3 validation.

- DLL SHA-256: `DDE468F93CDD7E6D466A006AF331C039AA10742AE7D0319EA50B6C78A773F442`
- Injector SHA-256: `9471CC71B1914202A893CA9CB592DD1472AD7FE9C46366B277203614187C1063`

## Operator steps

1. Before Thomas launches, announce: this run separates a reduced source from
   a reduction created by its transformation or subsequent selection; an
   already reduced source means trace its writer, otherwise isolate the local
   operation. No new target-60 run unless a precise ambiguity requires it.
2. Thomas launches DS3 offline, active display at 280 Hz, same sprint anchor,
   says ready without pressing F4. Verify exact game/probe hashes and no older
   loaded probe. Run `Set-VelocityMirrorTarget.ps1 -Target 165`, inject the
   matching `velocity-instrumentation/bin/I0007VelocityMirror.dll` with its
   `I0007VelocityMirrorInjector.exe`; require READY and correct schema/condition.
3. F4, then six seconds of AZERTY Z+Space, neutral controller, no camera input.
   Thomas says done and keeps DS3 open until the stop/restoration check.
4. Stop matching probe, run `Reset-VelocityMirrorConfig.ps1`, require COMPLETE
   and exact timing restoration. Analyze triples, reads, same-owner identity,
   code, overflow, cadence and full stimulus. Keep all raw evidence.
5. Thomas closes DS3. Reconstruct the smallest active formula, continue upstream
   if the source is already wrong; no compensation at the proven transmitters.

All three I-0007 configurations remain disabled at checkpoint. Causal patch,
sprint validation at 60/165 and unchanged-patch roll validation remain pending.
