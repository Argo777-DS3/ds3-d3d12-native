# I-0008 observed ring and bounded native temporal formula

## Cycle7: announced live identity/copy acquisition

Thomas prepared an offline idle character, explicitly said ready, and authorized
the announced3-second native-cadence observation. Codex did not launch DS3.
PID24896, capture20260920T111831Z,1944 records; raw SHA-256
90656AAAD1288207177E02A1DB529CFD7F666D2BB174C3B64E1ABD6B546EC411.
The manifest routes raw capture, analysis, source-candidate analysis and a
post-stop read-only RTTI snapshot. DLL identity comes from observer-build/validation.json.

All180 player pre/post pairs match ID, time a, time b, duration, destination,
module/vtable and modulo10 cursor progression exactly. No leading/trailing
unpaired rows, overflow or new-thread flag. Header reports arming/restoration
success; restoration was explicitly set then re-read for each surviving armed
thread. No gameplay/timing value was changed. DLL stayed loaded and inert until
Thomas closed the game. A subsequent process check found no DarkSoulsIII process.
Thread coverage has the protocol's born-and-exited-between-enumerations limit.

All source events use getter01078330, reading source+140; none use00F1B070.
For every insertion, the pair of float bit patterns a/b has exactly one matching
source within the preceding same-thread insertion interval:00007FF3EB174100.
This is strong **SUPPORTED value/order correlation**, not a causal pointer join.
Post-stop RTTI reads identify14 other/reachable pointers as hkbClipGenerator;
the selected pointer00007FF3EB174100 was already unreadable. Its class therefore
must not be promoted from the other objects' RTTI. A second pointer had a
non-image vtable and also remains unknown. Preserve post-read lifetime caveat.

Record ID is0 and duration2 throughout. Every next a equals the previous b
(179/179). Under the explicitly hypothetical unit-speed, zero-crop, unlimited-loop
model using central+268 as delta, all180 b values reconstruct bit-exactly from a,
including one2-second wrap. This does not prove the actual speed/crop fields or
delta argument: those were not captured. Native60 alone cannot discriminate
variable delta from fixed1/60. No165 runtime test occurred in I-0008.

## Cycle8: narrow static producer reconstruction

Exact-build hkbClipGenerator has vtable **02A0B9A8**, COL03E75638 and
type046748D8, as recorded in clip-rtti.json.
Vtable slot8 targets010768C0. It copies generator+140 into control+10, computes
new time through01078960, and commits XMM0 at010769F4 when generator+150 is0.
The control is generator+D0. Its ordinary construction path calls012846E0,
which sets the hkaDefaultAnimationControl vtable02A5D758. Slot5/+28 is01284D70.
Frpg's slot5 target00826650 is exactly JMP01284D70, not an independent formula.
Both previously missing Ghidra functions were disassembled only at these
vtable-proved entries in a read-only session; the headless log confirms changes
were discarded. No canonical Ghidra project modification occurred.

01078960 forwards the incoming float delta to control virtual+28. Before that,
01078DD0 computes speed from generator+B0 and optional duration normalization
through+B4, with mode3 direction reversal. Mode2 is explicit user-time control;
+150 and+BF also select boundary/ended behavior. These branches are not proved
active or inactive by the idle ring capture.

The base control function01284D70 accepts RCX=control, XMM1=delta, R8=outTime,
R9=outLoops. Let start=control+9C, end=control+A0, current=control+10,
speed=control+5C, and assetDuration=control+38 -> +18 -> +14:

- length=f32(f32(assetDuration-end)-start).
- If length<=0, retain current and zero loops, then clamp to nonnegative.
- Else relative=f32(f32(current-start)+f32(delta*speed)).
- Positive overshoot uses trunc(relative/length); negative overshoot uses
  trunc(relative/length)-1. Configured loop limit+68 and forward/reverse counts
  +60/+64 choose wrapping versus endpoint clamp, using the exact unsigned
  comparisons retained in control_formula_test.cpp.
- After wrap/clamp, add start in float32, then SSE MAXSS against zero.

The ordinary branch contains no fixed1/60 or factor10. This is a static formula
for the explicit delta parameter, not a proof of which domain supplies it live.
Generator01077610 offers another seek/update path; it remains distinct.

## Cycle9: validation and exit assessment

The exact313 bytes01284D70..01284EA8 contain no external/RIP-relative dependency;
a local synthetic harness runs that function on fabricated control/binding/asset
objects and compares a reconstruction using ordered SSE float32 operations.
51840 bounded finite cases cover zero/positive length, crops, positive/negative/
zero speeds,60/90/120/165 deltas, forward/reverse wrap, loop limits and endpoint
clamps. Output float bits, loop counts and input-object immutability all pass.
MXCSR at completion00001FA0; no claim for NaN, infinity, integer-conversion
overflow, alternate rounding mode or exhaustive native behavior. Exact artifact,
source and compiler command are routed by the manifest and TEST-CONTROL.ps1.
Byte validation additionally compares every enhanced export instruction to the
exact executable; semantic function boundaries remain separate from byte proof.

The scoped investigation reaches its **insufficient-information exit**, not its
success/invariance gate: retained data do not bind actual getter source/control
identity, delta argument, branch and output in one invocation across60/90/120/165.
The minimal validated observer and its successful first capture are preserved.
The existing evidence is not grounds for an animation patch or a whole-C4 claim.

The next discriminating experiment would pair generator010768C0 entry (RCX node,
RDX context, XMM2 delta), control01284D70 entry/exit (controller fields, actual
XMM1 and output), and the player callback chain. Capture source vtable and
context-to-character ownership **in the handler**, not after stopping. Reuse
current ring evidence rather than repeat its acquisition. At least a non60
comparison is needed to distinguish fixed60 from variable timing. That next
acquisition requires its own announced protocol and operator readiness; none
is scheduled, no game is running, and I-0008 is not merged into main.

## Cycles10-12: input ambiguity resolved

The announced target-165 discriminator subsequently captured495 complete
generator/control/ring chains. Every actual generator and control delta equals
central `+0x268` bit-for-bit, differs from `1/60`, and produces the exact native
and ring output. This resolves the input ambiguity for the captured stationary
player mode1 path. See [the final live control result](CONTROL-LIVE-RESULT.md)
for provenance, restoration gates, retained-residual comparison and limits.
