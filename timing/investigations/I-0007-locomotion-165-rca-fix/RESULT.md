# Investigation result

## Conclusion

**LTF — Locomotion Timing Fix: causal correction demonstrated; bounded sprint
and roll-motion checks pass.** LTF is distinct from I-0013's TRTF in operation,
site, guards and proof chain.
Thomas explicitly resumed on 2026-09-20 after recovery of the unmerged branch.
I-0007 continues under automatic bounded replanning D-0014 (57/60); broader high-framerate coverage and deferred
roll invulnerability have not been accepted (D-0011/D-0012). Cycle13 instrumentation passes synthetic tests; cycle14 target60 has 480 exact gate/recurrence/commit intervals and 81 non-unit outputs. Both non-unit episodes end before3.07s, so late Z at5.78s does not invalidate them. Same short scalar sequence exists in retained V2-165; observed duration50ms vs18.18ms is not a matched gameplay comparison. This is not gameplay acceptance. See [live protocol](../../evidence/I-0007/runtime/TRANSIENT-PROTOCOL.md).

Current extension: K-0040 establishes local source-recovery timing (83.33ms vs30.30ms). The continuous candidate has now completed its paired60/165 trial:1799 exact intervals,480/480 native-equal60 outputs, recovery83.33ms versus84.85ms and a2.387876s stationary165 tail. Session2428 is stopped/restored, config disabled. Only the recovery-clock gate passes; full movement equivalence is unproved. Offline28–30 quantifies the interpolation limitation and identifies a conditional0.65-per-call accumulator decay. All1976 historical snapshots of that accumulator are zero, so live impact is unproved. See the [bounded assessment](../../evidence/I-0007/static/velocity-threshold/ASSESSMENT.md). Original sprint/roll results remain accepted; no new capture during28–30.

## Complete demonstrated chain

1. Observable sprint travel at165 was approximately half the accepted60 reference
   while animation retained its delta-scaled progression (canonical I-0005/I-0006).
2. The selected A8 candidate already carried that loss; final writer selection,
   inhibition, scalar and local computation were noncausal and remain closed.
3. Upstream candidate integration, no-record increment path, source copy,
   rotation and inverse-delta conversion propagate the reduced amplitude.
4. In `005CA570`, a reference-compatible source vector is multiplied at
   `005CA707` by builder+25C=0.5: the first proved amplitude-creating operation.
5. Producer `005C72B0` tests float32 `30*per_update_distance < 1` without
   elapsed time. All494 original165 decisions reduce/retain the scalar at0.5.
   Smaller per-update distance at high cadence is wrongly treated as a smaller
   rate; existing feedback sustains the reduction.
6. Normalize only that comparison operand by `(1/60)/verified_elapsed`.
   The measured position difference N-1 to N requires broad deltaN-1, not
   necessarily the currently sampled timing-module value. Guard exact identity,
   position pairing, opcode and finite elapsed time. No output compensation,
   scalar overwrite, FPS table, action-specific gain or final-writer patch.
7. Patched60 sprint remains within0.215% of the accepted baseline. Full-duration
   patched165 versus patched60 differs by+0.639177% distance/delta and-0.460101%
   animation/delta, with exact commit checks and operator confirmation through
   stamina depletion. The initial late bug return occurred after V2 ceased
   correction; unchanged-kernel V3 extends application until input release.
8. Same-kernel roll motion at165 vs60: net distance after the unique Space
   release differs by-0.111901%; distance/delta+0.052581%. The observed input-edge
   sampling bracket remains within2%. All300/824 commits and scalar decisions
   reconstruct exactly; scalar1 throughout. Thomas confirms normal movement.
9. Every accepted session stopped with exact original timing30/0 and debug
   restoration. All on-disk instrument configurations are disabled.

## Evidence

- [Candidate and increment provenance](../../evidence/I-0007/runtime/CANDIDATE-WATCH-COMPARISON.md).
- [Increment reconstruction](../../evidence/I-0007/runtime/INCREMENT-MIRROR-165.md).
- [Source/rotation reconstruction](../../evidence/I-0007/runtime/VELOCITY-MIRROR-165.md).
- [Amplitude-creating stage](../../evidence/I-0007/runtime/BUILDER-MIRROR-165.md).
- [Faulty feedback and exact elapsed interval](../../evidence/I-0007/runtime/FEEDBACK-MIRROR-165.md).
- [Accepted sprint and resolved lifetime limitation](../../evidence/I-0007/runtime/PATCH-SPRINT-VALIDATION.md).
- [Accepted roll motion and explicit input exception](../../evidence/I-0007/runtime/ROLL-VALIDATION.md).
- [Artifact identities and raw provenance](../../evidence/I-0007/manifest.json).

## Limits and unresolved work

- D-0005 accepts manually differing start/facing as a bounded reference; this
  is not all-terrain/all-action or whole-game invariance proof.
- Roll input includes a late left click on Thomas's second screen. It occurs
  after motion, during idle, and cannot overlap the compared action. The raw
  exception and false strict full-window input flag are preserved.
- Roll i-frames/damage rejection remain unmeasured and explicitly deferred by
  Thomas. Roll motion passing does not imply full gameplay roll acceptance.
- One V2 sprint decision during a long-delta transient followed the native
  scalar recurrence including overshoot; universal transient invariance is not
  asserted. The full-duration V3 measured/tail scalars remained1.
- The same corrected formula working across these action witnesses supports
  a shared cause; it does not prove every high-framerate defect has that cause.
- Other temporal-system coverage remains open. 240/unlimited, broad refactoring,
  unproved gameplay semantics and distributable/persistent patch stay closed.

## Reusable decompilation contribution

Exact-build function reconstructions, field/dataflow mappings, live virtual
identity and arithmetic/time pairing are preserved alongside raw inputs and
reproducers. This is a reusable causal engine reconstruction, not a pair of
sprint/roll hacks or a claim of exhaustive source reconstruction.

## Current resumption

The recovered `92c2b79` checkpoint is integrated on main. The raw-hash and
offline replay audit reconciles the accepted results without new acquisition:
[recovery](../../evidence/I-0007/runtime/RECOVERY-20260920.md).
Cycles 11–12 reviewed the existing per-invocation scalar recurrence, captured
transient and invocation intervals. All 3,278 complete intervals have a matched
exit, but outer eligibility is not directly captured and the target-60 trials
have no non-unit reference. This establishes no additional gameplay failure or
new patch. The authorized cycles13–15 now include a stopped/reconciled60 transient and synthetic four-slot instrument; no accepted capture was repeated:
[bounded next discriminator](../../evidence/I-0007/runtime/FEEDBACK-TRANSIENT-AUDIT.md).
Original ignored binaries are absent; a future instrument requires a recorded
rebuild. Existing source and synthetic native tests remain available.

Cycles16–18 traced caller/time forwarding, bounded opaque0088CA90, refuted naive exponential gains across native resets, and built a caller observer without changing correction. It is disabled and not live-validated. Cycle19 requires a fresh165 transient with human input; prior60 evidence is reused. See the live protocol for exact provenance and limits.

## Historical pause instructions (superseded by the explicit resume)

No automatic continuation or new live test. Resume only when Thomas requests it,
from .project/INDEX.md -> STATE.md -> this result and the two validation records.
Reuse existing captures; do not repeat accepted sprint/roll-motion acquisitions.
The next decision is the bounded continuation toward remaining C4 coverage,
keeping i-frame investigation deferred unless Thomas reopens it. Do not silently
declare C4 complete, exceed the remaining investigation budget or reinject a
stopped DLL into the existing process. The current instrument is a reversible
test prototype, not an installed permanent game modification.

Cycles32–33: the60-only accumulator observer is built and passes native four-execute/restoration and analyzer positive/negative tests. One12s slope-to-flat activation search is prepared; no live result yet. See [observer protocol](../../evidence/I-0007/runtime/accumulator-observer/README.md).

Cycle34 setup:18340 rejected before trial because effective duration was3s, not12; stopped/restored and empty trial-events log retained. Corrected parser plus strict12s validation pass native tests. Fresh24724 READY verified60FPS/12s; user given F4 go. Session20260920T024008Z-pid24724 awaits completion/restoration/analysis.

Cycles35–36: accumulator24724 stopped/restored after718 exact zero-only fallback intervals; no harmful decay shown. Disk35-degree gate reconstruction is consistent with the inactive route; live threshold/setter-time direction unknown. No repeat route or165 pair scheduled. [Assessment](../../evidence/I-0007/runtime/analysis/accumulator-20260920-60/ASSESSMENT.md).

Cycles37–39: accumulator lead deferred because no convenient activating scene is known. Reuse of candidate2428 proves327 usable nonunit source ratios, including19 recovery samples; K-0041 qualifies local causal response only. No new acquisition. [Source validation](../../evidence/I-0007/runtime/analysis/elapsed-20260920-paired/SOURCE-VALIDATION.md).

Cycles40–42: 2625 accepted witness rows pass conditional scalar noninterference; known velocity branches audited. Exact native isolated math qualifies .95^(30*delta) damping in an alternative branch, already elapsed-scaled; no additional correction justified. No new acquisition. See [offline closure](../../evidence/I-0007/static/velocity-threshold/OFFLINE-CLOSURE.md). D-0015 requires convergence on a shared causal temporal contract and minimal patch points.

Cycle43: [shared producer contract and intervention inventory](../../evidence/I-0007/static/velocity-threshold/CONVERGENCE.md) distinguishes two current causal write locations from observation sites. Compare-only misses bypass branches; exit scalar alone cannot recover eligibility. Producer replacement remains unproved, not implemented.

Cycles44–45: single-exit scalar-write placement reproduces1799 retained candidate outputs, including81 differing raw/normalized branches, with324 synthetic/guard cases. Local17-instruction tail writes only+25C and calls nothing. This is offline equivalence, not a consolidated live patch or proof against concurrent observation; see the convergence assessment. Acquisition DLLs unchanged.

Cycles46–48: isolated native single-write transaction passes1799 recorded outputs and20 rejection checks; no game writes/DLL changes. Full branch coverage, concurrent visibility and motion acceptance remain open. Placement decision: fewer writes alone does not justify larger producer replacement. Existing canonical convergence assessment routes code, native logs and provenance.

Cycles49–51: placement-visibility audit verifies23 raw hashes; existing versus exit-only intermediate writes differ in81 retained165 cases. Tail has bypass ingress; common gate encloses two calls. Physical consolidation deferred; preserve current experimental placement for the movement gate. See convergence assessment. Continued from138cb59 on codex/i-0007-placement-motion, not main.

Cycle52 in progress: retained-motion matching audit records8.071-degree direction and0.204256 animation-time difference; no trajectory acceptance ratio is valid. Exact candidate DLL/injector recovered with original hashes, no rebuild/injection. New exact-build PID23800 launched offline for announced OSK test. Continue loaded by direct double-click; OSK S showed no visible movement (operator-confirmed), but Thomas subsequently reported stopping Computer Use. Fresh resume attempt returned explicit physical-Escape stop; no further UI action. Input control remains unqualified; no new capture. Full movement gate remains open.

Cycles52–54 complete: one new60 release control23800 stopped/restored,480 exact/native-equal outputs, but release occurred after the8s capture; gate insufficient. audit_release_control.mjs qualifies this limitation from23 raw hashes. Thomas explicitly authorizes exactly one retry with10s capture and a shortened physical stimulus; forbids asking for the same test immediately afterward. No new165 trial, patch-law change or CU control.

Cycles55–57 complete: the single authorized retry13732 passes the missing60 release-tail witness:600 exact/native-equal outputs, nonzero pre-release movement,5.533312s zero-path released suffix. Stopped/restored, disabled;23 raw hashes. Existing165 zero tail retained; different input/animation/scalar release context prevents matched latency or whole-trajectory claims. No more identical maneuver. See TRANSIENT-PROTOCOL and release-retry-20260920-60/release-tail-evaluation.json.

Cycles58–60 complete: [pre-factor source contract](../../evidence/I-0007/static/source-temporal/CONTRACT.md) traces005E57F0, disk dispatch candidates005E4970/005C9DC0, preparation005C8260 and stored-block getter009A21A0. C1 hash,22 raw hashes and256 live/disk code bytes pass. All1976 later snapshots have pending vector zero/counter-1; no activation or new cadence defect promoted. Candidate clock/source/native60/bounded-stop gates remain passed; whole trajectory unproved. Module+28 block integration interval is the next bounded offline question; budget60/63. No game run, patch or ledger change.

Cycle61: bounded module28 writer discovery is insufficient. The getter has only the known caller in the retained reference index; nearby field matches do not prove object identity. Two candidate exports supply no qualified writer. Continue62–63 through object/constructor/vtable provenance, not arbitrary same-offset expansion. Budget61/63; details and search limits in the source contract.

Architecture review: K-0042 now indexes the reusable function/field map from this pass, explicitly limited to exact-build static contracts and qualified runtime identity. This adds engine knowledge without promoting the candidate's gameplay gate. Thomas reaffirmed that engine mapping must continue alongside the correction work.

Cycles62–63 replace the negative writer lead with exact structure. Player
lifecycle initializes and tears down modules+28 through009A2F00/009A2D40.
Unique bounded writer009A3910 is called by00DD3850, which derives the three-part
block from an R9-selected48-byte source and current owner transform, then
forwards unchanged XMM1 into005C7160. Companion00DD3B50 forwards XMM1 through
005C7470 and writes the current transform back to the descriptor-selected
buffer. Exact bytes/calls pass against C1. The indirect caller, callback order
and descriptor source cadence remain unknown, so retained evidence cannot form
a controlled source replay.

Cycles64–66 define the smallest observation boundary at00DD3850,00DD3A7A,
00DD3B50 and00DD3BB2. The schema retains direction, descriptor selection,
48-byte source/derived/transform blocks, owner/update identity and raw elapsed
lanes. JavaScript and native validators accept interleaved valid pairs and reject
identity, elapsed, orphan, duplicate, ordering and pointer failures. An isolated
Windows test passes four execute breakpoints, resume handling and exact debug
state restoration. This is design/mechanism proof, not an injectable observer
or live/gameplay evidence. See the [observation boundary](../../evidence/I-0007/runtime/spatial-exchange-observer/README.md).

Cycles67–69 implement that boundary in a distinct observer derived from the
validated capture architecture. Its valid configuration requires observation
mode and rejects both correction flags and the legacy write-watch mode. It
checks all four exact instruction prefixes before arming, logs both exchange
directions, supports an interim flush without duplicate rows, and restores the
original debug state. DLL/injector builds, disabled/active config transitions,
native four-execute restoration, native/JavaScript pairing and analyzer
positive/negative fixtures pass. `BUILD-VALIDATION.json` records source/binary
hashes. The configuration is disabled; no injection or game capture occurred.

Cycles70–75: session4128 accepts the target60 structural control with528 strict
forward and528 strict reverse pairs, no rejection/overflow. All528 exchanges
order forward entry/write before reverse entry/write. One stable-owner selected
slot appears in all360 updates; its reverse payload lanes0/2 match sampled player
x/y in360/360 rows and change with the bounded movement interval. A second slot
is shared by14 owners and has168 no-op reverse writes. XMM1 lane0 is near1/60
for the stable slot in both directions, but near0.5 for every shared-slot
forward entry and near1/60 for its reverse entry, so this raw argument has no
single universal elapsed-time meaning at the callback boundary. K-0043 records
the bounded map.

The conditional165 trial crashed2.8892191s after arm near the operator-observed
movement stop. It has no trial_end/COMPLETE and is rejected; WER records BEX64,
`0xc0000005`, exception data8. The final phase line is a 20/21-column serialized
prefix, so it does not prove that logging returned or that the original function
began. The2,879 missing sequence slots are an upper bound on unrecovered spatial
hits, below both hit limits, but their payload is unrecoverable. Generic two-arm
4x1024 execution and exact restoration pass offline. The hardened rebuild
publishes completed spatial rows each second and suppresses full02344730 phase
logging in spatial mode. It is disabled and untested live; no repeat or
cross-rate claim.

Cycles76–78: retained phase comparison shows the same `ECA94C -> 02344730`
boundary completed830 times earlier in the crash session, including104 times
with animation22100, and494 times in an independent accepted165 capture.
Exact-build export places ECA94C directly after the call and shows eight
indirect object-table calls inside02344730. This is compatible with WER's invalid
execute classification but cannot identify the failing call or source without
faulting instruction/register/stack state. The hardened observer addresses the
avoidable logging load and spatial-row loss; crash causality remains unknown.
No game launch, injection, correction change or cross-rate promotion occurred.

Cycles79–81: the hardened observer ran once in a fresh target165 process with a
1.5-second requested movement window. It reached the4,096-hit limit after about
1.16 seconds, before `trial_end`, so the trial remains rejected and is not a
cross-rate comparison. Unlike the prior crash, shutdown completed normally:
`COMPLETE` records disabled hooks, restored debug registers, flushed rows and
exact `165/1 -> 30/0` timing restoration. The game later closed cleanly.

The complete prefix contains1,025 strict forward pairs and1,023 strict reverse
pairs with no pairing rejection. Its six selected slots differ from the two-slot
target60 topology. Across all3,104 joined entry rows from the accepted60 control
and this retained165 prefix, XMM1 lane0 is always either current broad delta or
`30 * delta`; all reverse entries use delta and the scaled form occurs only at
forward entry. K-0044 records this bounded `OBSERVED` formula.

Post-stop read-only RTTI maps all15 retained owners to
`SprjBehaviorCharacterProxyDriver`, with one PlayerIns-backed and14
EnemyIns-backed chains; every reachable modules+28 object is a
`SprjChrBehaviorModule`. All10 scaled-forward owners are transient EnemyIns
instances, while the player and four persistent enemy owners are unscaled-only.
K-0045 confirms this instance-level identity. EnemyIns identity does not by
itself select the scaled regime, and no gameplay role or caller-side rule is
claimed. No repeat is needed merely to enlarge the capture buffer.

Cycles82–84: exact-build RTTI/vtable parsing identifies00DD3850/00DD3B50 as
slots6/7 of the17-entry SprjBehaviorCharacterProxyDriver vtable. The bounded
construction path allocates this driver from009A2F00, writes the owning
PlayerIns/EnemyIns at+90 and attaches it at SprjChrBehaviorModule+50. K-0046
preserves this class/attachment map.

A global static search for the two raw slot offsets yields3,025 calls and192
functions containing both; it is not type-selective and cannot explain which
caller supplies30x delta. The smallest next discriminator is the entry-stack
return RVA. The observer now records it with a guarded read and passes two
4x1024 offline execute arms, caller reads, RF resume and exact debug-state
restoration. The current build remains disabled and has not been injected; the
session27644 predecessor validation and binary hash are preserved separately.
No game run, cross-rate promotion, correction change or merge occurred.

Cycles85–87: deterministic lane analysis of the stable selected/current48-byte
blocks confirms three float4 groups consistent with position/homogeneous,
unit-orientation and unit-scale data. At accepted60, all360 blocks have lane3=1,
unit lanes4–7, unit lanes8–10 and lane11=1; yaw from lanes5/7 matches sampled
player angle360/360, and reverse-current lanes0/2 match player x/y360/360.
All checks repeat in192/192 retained165-prefix rows. The forward-derived block
keeps the unit orientation/scale invariants but has lane11=0 and does not track
player angle in all552 rows. K-0047 preserves this bounded layout; lane1, the
source type and cross-rate equality remain unknown. No acquisition or patch.

Cycles88–90: the forward-derived translation now has a bounded world-space
formula. All124 active accepted60 rows and65 active retained165-prefix rows
satisfy `current-selected ~= -rotate(selected.orientation,derived)` within2e-4;
the remaining236/127 rows are jointly inactive with no one-sided cases.
Derived/world norm ratios stay near1. Median derived norm per broad delta is
4.01140065 at60 and4.00818600 in the165 prefix, ratio0.99919862 (-0.08014%).
K-0048 confirms the accepted60 formula and retains the165 result as row-level
support because that trial remains overflow-rejected and topology-mismatched.
No new defect, gameplay pass, acquisition or patch follows.

## Closure and integration

The investigation is complete for its defined causal chain. The reversible
distance correction passes the bounded sprint and roll-motion gates; the later
spatial work adds reusable engine mapping without promoting the rejected165
trial. Thomas authorized integration on 2026-09-20, and
`codex/i-0007-placement-motion` was fast-forwarded into `main` at `e924104`.
Broader C4 coverage, full spatial equivalence and roll invulnerability remain
explicitly outside this completed result.
