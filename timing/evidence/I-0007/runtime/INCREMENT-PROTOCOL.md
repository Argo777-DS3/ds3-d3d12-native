# I-0007 increment producer mirror

## Question

Does `02064FF0` receive the half-amplitude vector, or create it? If the input
is already wrong, identify the active writer of `nested+0xA0` and continue
upstream. Do not patch the correct multiplication/addition.

## Instrument and bounds

The isolated `increment-instrumentation` fork preserves the original watch.
It installs only the same three broad-update, Present and scheduler hooks.
During the qualified `A1429C -> DE50EC` transition on the selected thread:

- DR0 executes before call `01669A1E`, DR1 before return-site `01669A23`.
  Filter both by `RDI+0xA0 == selected nested+0xA0`. Set RF to execute the
  interrupted instruction once without retrapping.
- DR2 watches one four-byte write at selected `nested+0xA0`, reporting the
  post-write instruction, registers, and full four-lane field value. This
  directly follows the newly established upstream dataflow; it is not a
  speculative new region.
- Capture input bytes `[RBP-0x80, +0x60)`, output bytes `[RBP-0x10, +0x30)`
  only after the call, and `nested+0xA0..+0xBF`. Before the call, require
  `RCX=RBP-0x80` and `RDX=RBP-0x10` as established by the assembly.
- Limit accepted hits to eight per window and 4,096 total. Invalid reads,
  address mismatches and per-window overflow request stop. Data writes are
  post-instruction traps; execute traps are pre-instruction, explicitly
  distinguished by DR6 and the CSV's `rip_at_trap` field.
- Restore all six saved debug-state fields; the new binary reads back every
  restoration. No candidate, velocity, final-writer or game-directory writes.

The shared DR7 helper passes the offline Windows test: two execute traps,
one write trap, correct RF resume, unchanged expected arithmetic, and exact
debug-register restoration. Build passes MSVC `/W4 /WX`. These are synthetic
checks, not live DS3 validation. `analyze_increment_mirror.mjs --self-test`
passes unchanged/reduced-vector, product, pairing and invalid-read tests.

## Acquisition order: target 165 first, reuse existing reference

Thomas accepted this reduced acquisition on 2026-09-19 after questioning the
need to repeat target 60. The existing 60/165 candidate-watch captures already
provide the returned vector and exact downstream arithmetic. They do not
contain the earlier call-input memory; the input register is overwritten
before the old observation point. Do not infer a missing input from an output.

Acquire only target 165 initially. An unchanged reduced input/output pair
localizes transmission within that call and directs the trace upstream. A
change across the call localizes a transformation requiring reconstruction;
it alone does not prove that transformation is erroneous or frame-dependent.
Request a new target-60 input mirror only if a specific unresolved comparison
requires it, explaining exactly which alternatives it will distinguish.
This does not waive fresh 60/165 validation of the eventual causal patch.

## Operator sequence

1. Confirm DS3 closed, exact build and no previous probe in the new process.
   Announce the discriminating sentence before Thomas launches DS3 offline.
2. Thomas returns to the accepted sprint anchor with active display at 280 Hz
   and says ready. The next target is 165, not a new target-60 control.
3. Run `Set-IncrementMirrorTarget.ps1 -Target 165`, verify hashes,
   inject `increment-instrumentation/bin/I0007IncrementMirror.dll` using its
   matching injector. Require READY and correct session condition/schema.
4. Thomas presses F4, then holds AZERTY Z+Space for six seconds, neutral
   controller and no camera movement; says done, keeping DS3 open.
5. Stop with the matching injector, run `Reset-IncrementMirrorConfig.ps1`,
   require COMPLETE/exact timing restoration. Analyze pairing, valid reads,
   no overflow, one full stimulus, cadence and semantic validity. Retain raw
   input/output bytes, code windows, binary hashes and all provenance.
6. Thomas closes DS3. Reuse the prior reference and continue the causal trace;
   do not schedule a target-60 mirror automatically.

Config is disabled at checkpoint. Binary SHA-256:

- DLL: `D00C39A4D83B13C0727557386373B19CA8343F2FEA357E524C3B4A532C23E60D`
- Injector: `6D25D99BF441BD6125371706EC05594F8C30C6A6B1B61814825D302D87A24647`

No causal patch or roll test before this input boundary is resolved. Finding
another reduced input requires tracing its producer, not declaring completion.
