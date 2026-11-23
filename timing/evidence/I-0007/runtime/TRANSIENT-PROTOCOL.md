# I-0007 eligibility and non-equilibrium comparison

Planning tranche 2 (cycles 13–15), authorized 2026-09-20; starts from `4ec3c9a`.
No accepted sprint/roll acquisition is repeated. The question is whether the
remaining per-invocation recurrence has a harmful elapsed-time dependence.

## Prepared instrument (cycle 13)

[Source and build identities](transient-capture/BUILD-VALIDATION.json) and
[analyzer](analyze_transient_capture.mjs). Build and native four-slot tests pass.
The shared `causal-patch/distance_normalization.hpp` is unchanged. This prototype
uses distinct `DS3_I0007_TRANSIENT_PATCH` control events and binary names.

- DR0: existing normalized comparison `005C73D8`.
- DR1: existing recurrence exit `005C7435`.
- DR2: existing final position write, only as a commit sentinel.
- DR3: `005C734C`, before `TEST AL,AL`, after the virtual eligibility call.

The new site captures AL, the builder scalar and identity, current clocks and
positions. Its exact opcode is checked before arming. Each interval must contain
one gate, one exit, one commit, and at most one comparison, in order, on the same
object/thread/context. AL=0 must skip comparison and preserve the scalar;
AL!=0 without comparison must reproduce recovery/reset. With comparison, the
original normalization and recurrence must reconstruct exactly. Entry scale
must match the prior consecutive exit. Wall timestamps are recorded separately
from broad and module deltas; none is promoted to a new governing elapsed time
without causal qualification.

The capture runs for eight accumulated seconds after F4, including idle and
release; it does not stop the correction merely when sprint keys are released.
Maximum two trials per process/instrument session (60 then 165), within the
8,192-hit capacity. Do not add a third trial to that session. Configs are disabled
at checkpoints. No scalar gain or final-writer change is made.

## Operator protocol (cycle 14)

DS3 starts offline, exact C1 build, Cemetery of Ash, no nearby hostile activity.
The operator chooses a solid obstacle with room to step sideways away from it.
Observe the actual scene before accepting a route; do not infer an obstacle
from old coordinates. Keep the same equipment, route and approximate facing.
The D-0005 manual anchoring limitation remains explicit.

Announced initial run: distinguish active recurrence from a skipped calculation
during approach/contact/release at60. Only a qualified non-unit transition is a
reference for165; a stable/ineligible trace is insufficient information.

1. Prepare target60 with `Set-TransientTarget.ps1`; inject the matching binary,
   verify READY, build/PID, initial timing and new gate metadata.
2. At the selected short approach, F4, then Z+Space into the obstacle for about
   two seconds. Move sideways clear using Q or D while keeping the motion
   input, then release all keys by six seconds. Leave at least two seconds
   idle. Use captured input/contact/movement boundaries, not operator timing
   estimates, for the actual comparison. No attack, roll or camera input.
3. Flush once the trial ends. Require valid chains, exact gate/recurrence/commit
   checks, no overflow/rejection, real60 cadence, and non-unit scalar transition.
   Review eligibility changes, inputs and measured displacement before165.
4. Only if qualified, reposition while not capturing, announce the matched165
   run, switch with the same injector, verify readback, and repeat the same
   transition. Do not change the normalization or add action-specific gains.
5. Stop with matching injector; require COMPLETE and exact timing/debug-state
   restoration. Run `Reset-TransientConfig.ps1`; keep the raw logs and manifest.

Control commands: `I0007TransientPatchInjector.exe inject <pid> <dll>`,
`flush <pid>`, `target <pid> 165`, `stop <pid>`.
Analysis: `node analyze_transient_capture.mjs <session> <analysis-directory>`.
DLL path is `transient-capture/bin/I0007TransientPatch.dll` relative to this
protocol. Never reinject that same DLL into an already instrumented process.

## Acceptance and stopping

Stable scalar1 alone does not pass this discriminator. Do not compare unmatched
branches, terrain or input trajectories, and do not call natural obstacle
slowdown a bug. Capture validity alone is not gameplay acceptance. A new fix
requires matched evidence for an unintended elapsed-time/displacement difference
and the causal clock law; a simple gain exponent is not pre-authorized as correct.
Any pointer/opcode mismatch, overflow, crash, patch failure or restoration failure
stops the run and dependent work. Lost causal understanding invokes the existing
RCA rules. A justified same-scope next plan renews the planning budget under D-0014.

Windows control is available for selecting/observing the game. Sustained key
duration has not been qualified through its exposed API, so the operator performs
the held-key stimulus for now. Do not confuse window access with deterministic
input timing. No live run has yet occurred with this extension.

## Live checkpoint 2026-09-20 (cycle 14 in progress)

Thomas took over the stimulus after physical Escape stopped Computer Use.
Exact executable and all BUILD-VALIDATION artifacts were hash-verified before
injection in PID19144. READY confirmed all hooks; override apply30/0 to60/1
succeeded. Session: `sessions/20260920T011956Z-pid19144`.
Analysis: `analysis/transient-20260920-60/transient-summary.json` and windows.
Trial1: 481 updates, 480 exact gate/recurrence/commit intervals, 1650 hits,
210 comparisons, 74 reductions, 81 non-unit outputs; all480 gates active.
Median delta0.0166666005. Interim analysis passes; restoration pending.
Input masks change at seconds0:0x0, 0.90:0x1, 1.02:0x21, 2.77:0x23,
4.87:0x22, 5.13:0x2, 5.67:0x0, 5.78:0x1. Z remains recorded through8s.
Thus trial_end marks the bounded window, NOT demonstrated input release.
Clarify this deviation before qualifying any matched165 trajectory. No new
causal/gameplay claim. Do not repeat this valid acquisition automatically.
Instrument remains loaded and target60 active; only one trial remains in this
session. Stop/COMPLETE and exact restoration are required before closure;
auto-stop is900s from injection. Do not reinject this DLL into PID19144.

### Session closure and scope clarification

After Thomas reaffirmed the already accepted sprint/roll correction and the
broader unlocked-framerate objective, no165 trial was performed. Stop returned
COMPLETE: hooks disabled, debug registers restored, logs flushed. Timing log
confirms exact60/1 to30/0 restoration success. Config reset disabled. Final
analyzer replay passes, timing_restored=true, with hashes of all stopped raw
files in transient-summary.json. CONTROL is an earlier flush marker and does
not override COMPLETE. DLL remains loaded but inert; never reinject it here.

Accepted K-0037–K-0039 remain closed and are not being reacquired. The new60
capture fills the previously absent eligible non-unit reference, but proves
neither a second defect nor harmful frame-rate dependence. Next work is offline
reconciliation of this transient with retained timing/assembly evidence. Any
further experiment must state the distinct unresolved question and how existing
proof cannot answer it. Sprint/roll are causal witnesses, not the whole objective.

## Offline reconciliation (cycle14 outcome)

[Reproducer](analyze_transient_episodes.mjs) verifies all23 stopped raw hashes
and the historical V2 update/window identities, then writes
`analysis/transient-20260920-60/transient-episodes.json`.

All480 gate observations are active, once per qualified broad interval. Both
sampled module clocks equal the current broad delta in all480 intervals. This
steady60 coincidence does not establish which clock governs recurrence elapsed
time when cadence changes or eligibility skips.

Two non-unit episodes occur: updates2563–2640 (78 outputs, 1.300s, net path
0.163071 units) and2648–2650 (3 outputs, 0.050s, path0.141992 units). The return
to1 is observed at2641 and2651. The second starts at3.01665s and returns by about
3.06665s, before Z resumes at5.78331s. Thus the late input deviation invalidates
the claimed idle tail, but does not discard the earlier valid transient data.
No collision/contact channel is recorded, so small displacement cannot by itself
be labeled intended obstacle response or another defect.

The short60 sequence .8 -> .96 ->1.152 ->1 exactly reproduces the scalar states
in the already retained V2-165 transient. Non-unit states cover49.9998ms at60
and18.1818ms in V2-165 (sum of observed current broad deltas). This is an observed
state-duration difference, NOT a matched input/trajectory comparison or proof of
harmful displacement. Overshoot itself is reference60 behavior and must not be
removed merely for appearing at165. No new correction is justified yet.

The analyzer now distinguishes trial_end_observed from keyboard_released_at_end;
the old label input_release_end confused an8s timeout with an input-release
observation. Current replay reports release=false without changing raw evidence
or any accepted historical sprint/roll analyzer.

Cycle14 closes with partial discrimination, not a passed matched60/165 gate.
Cycle15 next action: use these exact branch/clock/input records to define the
minimal remaining elapsed-time discriminator and reference-preservation rule;
only implement a change if those establish causality. No more live acquisition
is needed to rediscover the same states. If matched trajectory/clock evidence
remains unavailable, close the tranche as insufficient information and establish
the next bounded same-scope plan under D-0014. No renewed authorization barrier.

## Cycle15: causal boundary and tranche closure

New bounded read-only Ghidra exports in `../static/recurrence-callers/` cover
005C7160,0089CCE0,005C5AC0. Existing source-writer005CA570 and I-0005 009DC090
remain the consumer references (the latter was also redundantly exported in the
batch; no new acquisition or claim results from that duplicate).

005C7160 calls source construction005CA570 at005C7266, then recurrence005C72B0
at005C7271. The already documented source path only multiplies by+25C when it
is below1; overshoot above1 is not a speed boost at this consumer. The alternate
caller0089CCE0 uses the complementary result of0088CA90 and forwards a value
from its argument+8. Caller ordering alone does not bind this to a governing
elapsed interval. 005C5AC0 is a conditional state classifier with return values
0/1/2/3; assign no gameplay name to those values.

Tranche13–15 closes at15/15: instrument built, stopped60 reference acquired,
exact intervals and existing165 states reconciled, no residual gameplay defect
or safe new correction established. Accepted K-0037–K-0039 are unchanged.
Continuation is justified by the now-observed eligible transient and explicit
caller/time leads, not by a repeated stable sprint/roll test.

## Cycles16–18: remaining clock boundary and prepared165 discriminator

Cycle16: selected assembly establishes incoming XMM1 preserved through
00DD3850:XMM10 ->005C7160:XMM6 ->005CA570/005C72B0. 005C72B0 itself reloads
module+1C for the existing009DC090 command. Alternate0089CCE0 loads XMM1 from
its second argument+8. The mode getter0088CA90 exports privileged/implausible
instructions and must be treated as opaque disk code, NOT a decoded predicate.
No whole-program search or protected-code interpretation follows. 0089B560 has
explicit parameter2-scaled vector paths, but no whole-function correctness claim.
At the retained60 gate, XMM6 equals current delta480/480; caller identity was
not captured, so this correlation is not a runtime route/clock proof.

Cycle17: [isolated model](model_recurrence_response.mjs), report
`analysis/transient-20260920-60/recurrence-response-model.json`, checks native
conditional arithmetic and source-before-feedback consumption. One eligible
reduction yields.8; two half-time invocations yield.64. A proposed exponential
gain from.6 yields.384 for one2/60 interval but.5 for two1/60 intervals because
of the conditional reset. Inactive calls preserve state. Thus exponent scaling
alone and wall-gap catch-up are not justified replacements. These are explicit
conditional models, not measured game displacement or new gameplay failures.

Cycle18: minimal observer revision adds safe read of return address at RSP+78
at the existing gate/compare/exit; no new trap or correction formula. Analyzer
requires consistent recognized return005C7276 or0089CDDC for all sites and
records gateXMM6. Original60 schema remains accepted; absent caller fields are
not fabricated. [Current build identity](transient-capture/CLOCK-BUILD-VALIDATION.json)
records I0007TransientClock.dll, W4/WX build and native tests. This binary has
NOT been injected. The original BUILD-VALIDATION is historical at a46b026.

### Next live step: fresh process,165 only; human stimulus required

Old PID19144 contains the stopped original DLL. Restart DS3 normally before
using the observer revision; never reuse old control events or reinject there.
Thomas currently performs held inputs because automated keyboard response and
duration remain unqualified. All config is disabled until he is positioned.

Discriminator: distinguish a shorter eligible recovery from a different input
or activation branch; if the same .5 plateau and subsequent recovery are
qualified, compare their time and movement; otherwise record unmatched evidence.
Do not repeat60, accepted sprint or roll. Keep the previous equipment, obstacle,
approach/facing and side (Q, as recorded in60); manual matching limits persist.
Prepare165 using Set-TransientTarget.ps1 -Target165 -DllName I0007TransientClock.dll
(with spaces between PowerShell parameter names and values). Verify actual new
PID, executable/build hashes, READY and165 override readback before F4.

One8s trial: F4, begin Z+Space after about1s; add Q around2.8s to move clear;
release all by5.7s and remain idle through8s. No attack, roll or camera motion.
Do not enforce time estimates as facts: align actual input/position/scalar logs.
Require all exact arithmetic/identity/caller checks and actual165 cadence.
Compare only intervals sharing qualified active recurrence, stable input and
source state; expected reference has a .5 plateau and recovery near2.83s.
First establish whether a matching branch sequence exists; no whole-trial
manual-distance ratio may substitute. A mismatch does not revoke accepted60.
Stop after the trial, verify COMPLETE and exact timing/debug restoration; disable
config and retain all raw evidence. No new scalar correction is armed.

Tranche16–18 closes with clock leads bounded, naive candidate refuted, and this
minimal observer prepared. It does not claim all three cycles validated a fix.

## Cycles19–21: acquired165, local response proof, offline candidate

Cycle19: new exact-build PID6976, session `sessions/20260920T013846Z-pid6976`,
clock observer hash ECCDB0BF653768A59C69E5FDE568EE63E7680D2EF806988551A5C38DB3398148.
All1320 intervals/4464 hits pass arithmetic, identity and caller gates. Median
broad delta0.00606059982;1321 measured samples. COMPLETE and timing165/1 to30/0
restoration succeeded; config disabled. Caller005C7276 at all1320 gates. Saved
XMM6 equals module+1C1320/1320 and next broad delta1320/1320; current broad delta
matches1317/1320. Three differing rows discriminate this local phase association,
not all elapsed-time semantics. Final stopped hashes are in
`analysis/transient-20260920-165/transient-summary.json`.

Cycle20: [comparison reproducer](compare_transient_recovery.mjs), report
`analysis/transient-20260920-165/recovery-comparison.json`, verifies both raw sets.
Recovery60 updates2637–2641 and165 updates6870–6874 have identical .5 initial
state, outputs .6/.72/.864/1.0368/1, active gates, no comparison, keyboard0x23
and animation22200. Their five calls consume83.3330ms versus30.3030ms.
The source's norm divided by (+260 pre-factor rate * module delta) matches
min(entry scalar,1) within2e-7 for every one of those ten rows. Nonzero source
rates and the known005CA707 consumer corroborate that this is actual source
attenuation, not a dead bookkeeping field. This establishes local temporal
response dependence; different source trajectories/facing prevent a universal
or whole-trial gameplay-distance claim. No accepted sprint/roll claim changes.

Cycle21: [offline candidate](replay_elapsed_recurrence.mjs) advances the original
float32 law on1/60 active-time quanta, retains inactive credit and delayed resets.
6561 eight-call branch/skip sequences preserve60; all480 real60 outputs match.
Frozen-branch165 replay changes24/1320 scalar outputs and ends at1, with480
virtual ticks. [Report](analysis/transient-20260920-165/elapsed-recurrence-shadow.json).
This is NOT live-corrected gameplay: future branch decisions would depend on
changed movement. Its discrete reference rule, roundoff tolerance, gap handling
and sampling error are explicit. No candidate has been injected or installed.

## Cycles22–24: continuous experimental candidate, NOT live validated

Cycle22 refines the discrete shadow from21: [native state machine](transient-capture/elapsed_recurrence.hpp)
interpolates continuously between exact native float32 reference endpoints. It
advances on active caller delta; inactive calls hold value and phase. A branch
change starts a new segment at the current interpolated value. Endpoints use
1/60s and preserve delayed reset/overshoot; roundoff tolerance1.6667e-7s handles
logged nominal60 rounding. State remains updated each invocation: this does not
hold the movement subsystem at60Hz. The older step-only shadow is historical,
not the live candidate. Interpolation and branch-reset phase are explicit
prototype choices, not a uniquely recovered intended formula.

[test_elapsed_recurrence.ps1](transient-capture/test_elapsed_recurrence.ps1)
rebuilds/runs6561 native branch/skip sequences, partition/skip/invalid-delta tests,
and native-vs-independent-JS replay. All480 recorded60 outputs match native
exactly; both implementations agree1800/1800 rows. Frozen165 branches change
32 outputs, final1. Live future feedback can differ: this is not game acceptance.

Cycle23 guarded integration uses the same four existing sites. At the gate:
require caller005C7276, same owner/trial, consecutive update, exact incomingXMM6
and module+1C equality, and entry scalar equal to prior candidate output. At
exit: reconstruct and verify the native result before replacing builder+25C
with the elapsed recurrence result. Readback must match; failure attempts to
restore that native result and stops. Log native output, proposed output, delta,
application and failure separately. Analyzer reconstructs both native and
candidate values, then checks next-entry continuity. No final-writer change or
fixed scalar bypass. The accepted distance-comparison correction is unchanged.

Cycle24: W4/WX build, native model and four-slot/RF/restoration tests pass; old
stopped60/165 analyses still pass. Candidate config prepare/reset checked without
injection. [Current identity](transient-capture/ELAPSED-BUILD-VALIDATION.json)
routes I0007ElapsedRecurrence.dll and all source/analyzer hashes. Earlier build
manifests refer to their recorded historical revisions, not the changed source.
No live write from this candidate has occurred; all configurations disabled.

### Next candidate validation:60 then165, same new process if valid

Fresh offline DS3 process required (PID6976 contains stopped observer). Thomas
performs held input; automation remains unqualified. This is a regression of a
NEW intervention, not a repetition of the accepted old sprint/roll acquisition.
At the same obstacle/equipment/side, prepare with
`Set-TransientTarget.ps1 -Target 60 -DllName I0007ElapsedRecurrence.dll -ElapsedRecurrence`.
Verify ELAPSED-BUILD identities, new PID, READY, elapsed_recurrence_patch=1,
condition=i-0007-elapsed-60 and actual override. One8s approach/contact/left
release trial using the existing stimulus. Require exact candidate reconstruction,
no guard failure/overflow, valid60 cadence and baseline-like response. If it
passes, position again outside capture and announce165, switch target with the
same injector, verify control, and perform one matched trial. Maximum2 trials.
Stop/COMPLETE and exact timing/debug restoration, then reset config. Do not call
it a validated fix until closed-loop results justify it. No re-run of old raw
acquisitions merely to reproduce their already accepted outputs.

## Live candidate checkpoint: cycle25 control passed, cycle26 pending

Session20260920T020120Z-pid2428, candidate condition i-0007-elapsed-60.
Trial1:480 intervals,1761 hits; exact native/continuous/commit checks pass,
zero differences between480 candidate and native outputs. Qualified caller
005C7276 throughout, complete recovery and non-unit states captured. Keys begin
Z at1.800s, Z+Space1.867s, Z+Space+Q4.467s and remain held through8s. Thus no idle
release-tail claim; the earlier recurrence-control evidence is valid.
Interim analysis: `analysis/elapsed-20260920-60/transient-summary.json` and windows.
Original raw logs remain live; no immutable final hash claim yet.

After announcing the165 comparison, same injector acknowledged target=165
success=1; timing log records60/1 to165/1. PID2428 remains instrumented; restoration
is PENDING. Exactly one trial remains allowed. Reposition at the same start,
perform the next stimulus once, then stop/COMPLETE, verify exact165/1 to30/0 and
debug restoration, reset config and finalize both trials. Do not reinject.

## Cycles26–27: paired candidate outcome and closure

Both trials in20260920T020120Z-pid2428 are stopped. COMPLETE confirms hooks/debug
restoration; timing165/1 to30/0 success and config reset verified. Final hashes
and exact1799-interval reconstruction are in `analysis/elapsed-20260920-paired/`.
[Evaluation](evaluate_elapsed_pair.mjs) verifies those hashes before analysis.
Trial1 remains480/480 native-equal; trial2 has1319 exact intervals,38 outputs
changed from the per-call native result, no guard failure or overflow.

The qualified .5 recovery reaches output>=1 at66.6664ms(control60) and66.6666ms
(candidate165); exact1 settles83.3330ms vs84.8484ms, a1.5154ms difference within
one165 update. Both local windows have keys0x23, animation22200. Thomas reports
immobile end; measured165 final394 samples cover2.387876s with zero path length.
The control60 idle tail remains absent and is not retroactively passed.

Decision: bounded recurrence-clock gate passes, not complete gameplay acceptance.
Different animation starting phases, paths and inputs prevent a whole-trajectory
claim. Under an explicitly hypothetical constant unit source over5/60s, integrated
factor differs+4.6633%; this illustrates sampling/interpolation sensitivity, not
an observed DS3 distance error. Do not silently equate clock equality with full
movement equivalence. Original accepted distance patch/sprint/roll are unchanged;
the candidate remains experimental, stopped and not distributed.

Further offline work can qualify the integration limitation and examine a
separate already-exported per-update position threshold at009DEE60 (owner+84 vs
+94, constant1439F3090, flag+130, optional+2E0 reset). This is a lead from the
known velocity chain, not a new demonstrated defect. Recover its existing data
and consumers before any new capture or patch.

## Cycle52: manual movement-reference readiness (2026-09-20)

Thomas abandons Computer Use/OSK control and performs physical game input. No
further UI automation. Continuation runs on codex/i-0007-placement-motion from
4ea3a45, source138cb59 preserved. Exact historical candidate DLL/injector recovered;
continuation-artifact-reconciliation.json records hashes and CRLF-only text changes.
No rebuild, new hook, scalar-law change or physical consolidation.

Question: can the contact-to-left-escape motion serve as a reproducible movement
reference, beyond the already proved local clock/source response? The retained
pair fails matching (heading differs8.071degrees; animation time differs0.204256).
Do not repeat it merely to revalidate its arithmetic. The new qualification is
within60 repeatability before spending a165 acquisition on an unmatched route.

One8s trial at60 first, unchanged candidate: idle start at a reproducible obstacle
approach; F4 once; after1s hold Z+Space; at3s add Q without releasing Z+Space;
release all at6s; remain idle until8s completes. Same equipment/camera/facing;
no attack, roll or controller input. Operator estimates are stimulus guidance;
actual logged edges govern analysis. Verify READY, build/PID, both correction
flags,8s duration, target60 readback and original30/0 before giving F4 go.

Review first trial before another F4. Require complete exact intervals,60 native
scalar equivalence, nonunit reduction and full recovery, stable released tail,
and safe usable route. Scalar1 throughout is non-discriminating. If qualified,
reposition to the same anchor and announce a second60 reference trial in this
same session (at most two trials total; no165 switch in this reference session).
A second60 trial tests a new repeatability gate, not recovery of lost evidence.

Compare actual input edges, starting position/facing, animation sequence/phase,
nonunit episode and world commits. Existing2% motion tolerance applies only after
matching; a manually different contact/escape is insufficient information, not a
candidate regression. Keep temporal endpoint brackets and source-rate histories
visible; do not select a favorable shifted window or normalize away disagreement.
Before165, require a reproducible60 reference and a prospectively documented
matching window. If matching cannot be achieved, replan the discriminator rather
than accumulating attempts. Local source proof remains accepted independently.

Stop/COMPLETE after the bounded session; require exact timing30/0 and debug/hook
restoration, reset configuration and preserve raw hashes. Never reinject into a
process containing the stopped DLL. No capture is armed by this protocol alone.

### Cycle52 refinement before injection

The repeatability pair proposed above is NOT scheduled: it would repeat the same
manual route without resolving phase/trajectory confounds. Thomas questioned the
repeat and confirmed he is at the intended obstacle. The bounded next trial is
one60 release-tail control only, filling the absent released tail in candidate2428
trial1. Existing165 release-tail evidence is retained; no new165 trial scheduled.
Stimulus remains F4,1s idle,Z+Space,add Q at3s,release ALL at6s,idle through8s.
Require actual movement before release, no held keys/controller commands afterward,
valid exact candidate/native60/commit checks, stationary terminal suffix and exact
restoration. Compare release input/animation/scalar context before interpreting
latency differences. This can qualify a bounded stop-response reference, not full
trajectory equivalence or a new gameplay fix. If non-moving before release, the
trial is non-discriminating; do not immediately repeat. Maximum one trial here.

## Cycles52–54 outcome and explicitly authorized single retry

Session20260920T033000Z-pid23800 is stopped/restored60/1 ->30/0 with COMPLETE.
480 exact intervals and480 native-equal60 outputs,204 nonunit outputs;23 raw
hashes retained under analysis/release-control-20260920-60. The capture ends at
8.016635s with Z+Q held; complete release is first sampled in the separate semantic
log at11.533288s. Thus the planned release gate is insufficient information.
No candidate correction remains active after that trial boundary. Do not use the
later passive semantic idle as proof of a patched release. Reproducer:
`audit_release_control.mjs`. There is no new demonstrated gameplay fault.

Thomas explicitly offers one retry, with the constraint: do not ask for the same
test again immediately afterward. Exactly ONE new trial is authorized/planned;
after it, stop this maneuver regardless of result. No automatic165 or third trial.
Use a fresh DS3 process; stopped DLL in23800 must never be reinjected. Existing
candidate binary unchanged; only capture duration changes8 ->10s, within the
already implemented parser bound1..10. Auto-stop stays300s. Start from the same
obstacle; after F4 immediately hold Z+Space for about2s, add Q for about1s, then
release ALL keys and wait. This leaves substantial tail margin without precise
counting against8s. Verify effective10s in session-info before go. If this still
misses release, fails movement or identity gates, preserve failure and replan
offline; no request for another identical maneuver. Thomas controls the game.

## Cycles55–57: single authorized retry complete; release witness acquired

Session20260920T033403Z-pid13732, exact recovered candidate, one10s trial at60.
Effective10s/300s settings and original30/0 ->60/1 verified before F4. Thomas
operated physical input. Config reset on disk after loading; stop/COMPLETE and
exact60/1 ->30/0 restoration confirmed. No new DLL source, hooks or scalar law.

600 exact intervals and600 native-equal outputs;57 nonunit outputs. Complete
keyboard release at4.466649s follows nonzero movement (1.91488 path units in the
preceding sampled half-second). The remaining5.533312s sampled position is exactly
unchanged, with zero released-tail path. Candidate correction remains in its
qualified capture window.23 raw hashes verify. See
`analysis/release-retry-20260920-60/release-tail-evaluation.json` and reproducer
`evaluate_release_tail.mjs`.

The retained165 trial has2.381816s between first/last released samples with zero
path (historical2.387876s includes the final sample delta). No discrepancy: different
span definitions. Its release animation22200, prior inputZ and scalar1.020496 differ
from new60 animation300020110, prior inputQ and scalar1. These observations qualify
bounded stop behavior separately, not matched stopping latency or full movement.
No165 repeat and no further identical maneuver requested. Acquisition tranche is
closed; later work is offline causal source/integration qualification.
