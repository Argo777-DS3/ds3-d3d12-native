# Scalar feedback: qualify the suspected fixed-distance decision

## Narrow question

Does the selected `005C72B0` recurrence actively hold builder `+25C=0.5`
because the measured per-update displacement passes its fixed `30*d < 1`
test? See [the creating-stage result](BUILDER-MIRROR-165.md).

Isolated `feedback-instrumentation`, same three base hooks and selected
`A1429C -> DE50EC` interval. Two execution slots only:

- `005C73D8`: immediately before COMISS, after multiplication by 30.
  XMM0.low is tested value, XMM7.low is 1, XMM6 is old position, RSP+30 getter.
- `005C7435`: after the recurrence, before epilogue. It is reached even if
  earlier gates bypass the comparison; that case must remain distinguishable.

Filter selected transform through RDI and validate builder RBX via the same
player/module chain. `builder_bytes` retains `+0..26F`; `output_bytes` has
transform `+80` and `+90`, then getter output only at the comparison (otherwise
zero padding). `input_bytes` has transform `+3C0..3CF`, `+1F0..1FF`, `+A0..AF`
in successive 16-byte chunks; remainder zero padding. `timing_18_bytes`
contains timing module `+18..27`, delta at offset 4. `nested_bytes` remains
transform `+E0..FF`. Capture 512 live bytes at `005C72B0` and 128 at `005CA6ED`
to preserve the exact recurrence/application code. Legacy producer_target
column now identifies `005C72B0`; producer_vtable is deliberately zero.

Other hardware slots are disabled, original six debug fields restored and
read back. Existing read/identity/overflow stop gates and bounded hit buffer
remain. No causal patch, scalar change, movement compensation or game-file
write. Target timing override is the established reversible one.

## Offline checks and identity

W4/WX build and two-execute-slot RF/restoration test pass. Analyzer self-test
checks float32 distance summation order, both recurrence branches and invalid
vector rejection; live pairing remains unqualified until the acquisition.

- DLL: `AAF7B09D58CC24633B55F2D003A2A1E93F8A51901B3DEA3F13AC2E3B090B3A56`
- Injector: `11806F0921335DC548B20277F558249176B0C2FB1715C84E78CCEF7721B08F48`

## Acquisition without an unnecessary restart

The builder probe in PID 20840 was explicitly stopped, logs flushed, hooks
disabled, debug state/timing restored, and config disabled. Its DLL remains
loaded but inert. A differently named instrument with separate control events
may be injected into this still-open process after verifying that clean stop.
This is not reinjection of the stopped DLL. Require the new READY and schema
`I-0007-scale-feedback-mirror-v1`; on any conflict/failure stop and restore,
do not reuse or infer missing proof. If the process has closed, announce a new
launch rather than starting it automatically.

Announce: this run distinguishes a real fixed-distance reduction from a scalar
set elsewhere; if comparison and recurrence match, the cadence-dependent path
is qualified, otherwise continue the actual scalar producer trace.

Set `Set-FeedbackMirrorTarget.ps1 -Target 165`, inject matching DLL/injector,
verify READY and condition `i-0007-feedback-mirror-165`. Thomas returns to the
same start, F4 then AZERTY Z+Space six seconds, neutral controller/no camera.
Keep game open for matching stop, reset config and verify COMPLETE/exact
`165/1 -> 30/0`. Run `analyze_feedback_mirror.mjs <session> <analysis>` and
existing cadence/locomotion analyzers; preserve every raw observation.

The analyzer reports a **hypothetical** 60-equivalent distance test using
the captured timing-module delta. This is not an approved patch or proof that
this current delta belongs to the measured previous-position interval. Resolve
that time pairing before causal correction. No automatic new 60 reference.
