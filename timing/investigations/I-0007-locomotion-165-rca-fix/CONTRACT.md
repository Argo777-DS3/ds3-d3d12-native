# I-0007 - RCA-to-fix locomotion at 165 FPS

Capability: C4

Canonical correction name: **LTF — Locomotion Timing Fix**. This contract and
its evidence define LTF; it is distinct from I-0013's TRTF.

## Question

Where is the first exact operation upstream of the confirmed selected
`A8 -> +0x40 -> +0x70` candidate that creates the approximately `0.5`
locomotion amplitude at target 165, why is it framerate-dependent, and what is
the smallest reversible causal correction that restores the target-60 behavior?

## Scope

Included:

- Use only canonical I-0005/I-0006 proof as the starting point.
- Begin from the candidate actually consumed during the accepted sprint and
  trace its active producer chain upstream.
- At each layer, reconstruct the smallest useful calculation and compare target
  60 with target 165 using the smallest discriminating instrument.
- Continue past intermediate divergences until evidence identifies the first
  incorrect value, exact instruction/formula creating it, and reason for its
  cadence dependency.
- Once cause is demonstrated, implement the smallest reversible patch directly
  at that operation, favoring a continuous formula based on time, delta, or the
  actual relevant quantity.
- Validate unchanged sprint at target 60 and restored target-165 sprint within
  the existing 2% distance-per-time/delta tolerance, with coherent animation,
  complete updates/commits, no new visible divergence, and exact restoration.
- After sprint passes, test roll immediately with the exact same patch and no
  roll-specific adaptation.

Excluded:

- Reopening or compensating final writer `009DC4D0/009DC580`, its source
  selection, inhibition, `+0x1C0` scalar, or local arithmetic.
- An arbitrary `x2` at 165, refresh-rate tables, or any symptom correction
  without a demonstrated causal formula.
- Broad exploratory campaigns, redundant hooks, or new engine regions without
  causal justification.
- Unproven gameplay semantics, 240 FPS/unlimited, general optimization or
  movement refactoring, and a distributable final patch.

## Inputs

- Canonical claims: K-0032, K-0033, and K-0034.
- Governing decisions: D-0005, D-0006, D-0007, D-0009, and D-0010.
- Artifacts and sources: the I-0005 final-writer static exports and algebra
  bridge; the accepted I-0006 target-60/target-165 sessions, reconstructions,
  and cross-condition comparison.
- Tools: exact-build static analysis, bounded reversible runtime
  instrumentation, deterministic Thomas-operated stimuli, offline bit-exact
  analysis, and reversible patch prototyping.

## Hypotheses

- H1: the active updater of nested `A8+0x40+0x70` receives compatible inputs at
  60 and 165 but contains the first frame-dependent amplitude operation.
- H2: that updater receives an already-reduced input at 165; it preserves the
  divergence and the trace must continue to the input's active producer.
- H3: the causal operation uses a per-update quantity, fixed-step assumption,
  counter, clamp, or discrete conversion where a continuous time/delta law is
  required.
- H4: correcting that operation restores sprint at 165 without changing 60;
  the unchanged patch also corrects roll, supporting a shared locomotion cause.
- H5: sprint passes but roll remains incorrect; only then is a separate roll
  RCA warranted.

## Expected evidence

At every layer, preserve the exact executable and instrument identities,
instruction site, raw input/output bits, pointer/context/update pairing,
temporal delta, and restoration record. If an input is already divergent, that
layer cannot be the first cause and the trace continues upstream. The first
causal operation requires compatible 60/165 inputs but an output ratio that
accounts for the measured candidate loss, plus an instruction-level formula
explaining the dependence on cadence.

The patch evidence must bind the reversible change to that same operation and
formula. Sprint validation reports target-60 and target-165 distance per
accumulated delta, animation ID/progression, update and commit coverage, visible
behavior, and exact restoration. Only after those sprint gates pass is the
unchanged patch evaluated on roll.

D-0012 adds an explicit roll non-regression gate: preserve baseline i-frames.
Qualify the actual invulnerability representation/consumer and compare its
action-relative active windows, elapsed duration and demonstrated logical count.
Rendered-frame count, travel distance or animation alone are insufficient.
Existing captures lack that state; a motion pass is not full roll acceptance.

## Exit conditions

D-0011 clarifies that these are exit conditions for one bounded causal chain,
not for the overall high-framerate RCA objective. Sprint and roll are witnesses;
all relevant high-framerate defects and untested temporal coverage still belong
to C4 after this chain passes. Do not declare the overall work finished here.

- Success: the full chain is evidenced from observable symptom to first wrong
  value, faulty calculation, framerate-dependence reason, causal patch, sprint
  165 correction without target-60 regression, and an explicit unchanged-patch
  roll result.
- Refuted: a proposed operation is proven not to create the divergence or its
  patch cannot pass valid sprint gates; record it and continue or replan rather
  than adding compensation.
- Insufficient information: protected code, invalid pairing, nondeterministic
  stimulus, missing provenance, or restoration failure prevents a conclusion.
- Stagnation: no new discriminating evidence after two analysis/experiment
  cycles, or two local fixes fail; stop patching and replan before another.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`; renew a bounded budget automatically within unchanged authorized scope and existing protected boundaries.

## Current planning tranche: cycles 13–15

Thomas authorized this continuation on 2026-09-20 from `4ec3c9a`. The first
12 cycles are closed with accepted K-0037–K-0039 and the inconclusive residual
transient audit. Scope and objective are unchanged.

- 13: qualify outer eligibility/time identity at the known producer; build and
  synthetically verify an observation extension with the accepted patch unchanged.
- 14: acquire and analyze a genuinely discriminating non-equilibrium 60/165
  comparison; do not repeat accepted steady sprint/roll acquisitions.
- 15: derive a correction only from demonstrated causal/time evidence, then
  verify it or record the remaining gap and bounded replan.

No automatic merge, i-frame work, 240/unlimited or distributable patch follows
from this planning renewal. Prior launch announcement remains required.

## Safety and human interaction

Thomas explicitly authorized assistant launch/control on 2026-09-20. Before every live run,
announce exactly one sentence of the form: `Ce run distinguera X de Y ; si A
alors conclusion A, si B alors conclusion B.` Thomas currently controls the test (after the Computer Use interruption) and
performs the specified stimulus. Instrumentation and patches remain disabled
by default, external to the game installation, reversible, and must restore
timing, debug state, and patched bytes exactly. Any mismatch, overflow, invalid
chain, crash, or restoration failure stops the run. Sol remains primary; Astra
is reserved for a genuine protected/opaque-code wall or multiple chains that
adequate evidence cannot distinguish.


## Planning tranche: cycles16–18 (automatic renewal under D-0014)

Prior tranche13–15 closes with stopped/restored60 transient, exact reconciliation,
and the unresolved elapsed-time/consumer boundary recorded in TRANSIENT-PROTOCOL.
Objective, scope and protected gates are unchanged. Budget extends cumulatively
from15 to18; used remains15 at entry. No new human permission is needed.

16. Trace only the exposed recurrence caller/time and mode leads; recover
    existing exports first, then targeted read-only decompilation as needed.
17. Reconstruct the scalar consumer's elapsed response offline from exact native
    arithmetic; distinguish a conditional model from gameplay evidence and
    preserve the60 reference, skipped invocations and reset ordering.
18. Review whether this supports a reversible causal prototype or instead needs
    a new discriminating observation; prepare only the minimal necessary step.
    No speculative gameplay patch, repeated accepted acquisition or new scope.

If the clock cannot be causally bound, do not compensate using a convenient
field. Record the remaining evidence gap and replan; do not hide stagnation.

## Planning tranche: cycles19–21 (automatic renewal under D-0014)

Tranche16–18 completed targeted time-route trace, offline reset counterexample,
and a built but untested caller observer. Budget extends18 to21, used18.
19: acquire the single new165 transient with human held input, retaining60;
    validate caller/time identity and exact restoration, reject unmatched paths.
20: compare only qualified elapsed recovery and its consumer consequence;
    distinguish conditional count dependence from a gameplay defect.
21: implement/test a causal reversible change only if evidence supports it,
    otherwise close with the specific gap and replan within existing boundaries.
The current wait is for physical game stimulus, not budget authorization.

## Planning tranche: cycles22–24 (automatic renewal under D-0014)

19–21 closes with stopped165, K-0040 local temporal-response proof and an offline
reference-preserving candidate; budget extends21 to24, used21. No scope change.
22: implement the bounded native arithmetic state machine and validate reference,
    partition, skip, reset and invalid-input behavior independently of the game.
23: integrate only if the causal invariants can be guarded; preserve accepted
    distance correction, verify native-output expectation before intervening,
    and record the changed state, clock and owner lifetime explicitly.
24: validate instrument build/restoration and prepare a minimal closed-loop
    comparison; live acceptance still requires actual evidence, not shadow replay.
No final-writer compensation, gameplay semantic relabeling or permanent patch.

## Planning tranche: cycles25–27 (automatic renewal under D-0014)

22–24 closes with native continuous candidate, guarded integration and offline
validation; no live candidate test. Budget extends24 to27, used24.
25: new-candidate60 control; exact reconstruction, source response and restoration.
26: candidate165 only after25 qualifies; match internal recovery and input gates.
27: decide bounded acceptance/rejection from closed-loop proof; retain accepted
    original sprint/roll, and replan remaining temporal coverage without scope drift.
The next required input is Thomas's game stimulus, not permission to renew budget.

## Planning tranche: cycles28–30 (automatic renewal under D-0014)

25–27 closes with candidate60/165 clock gate passing and exact restoration;
movement-equivalence limitations remain explicit. Budget extends27 to30, used27.
28: bound the interpolation/source-integration limitation from existing traces.
29: qualify the already-exported009DEE60 fixed position threshold and its gated
    state update, recovering constants/consumers before considering acquisition.
30: classify the lead and choose a discriminating next step only if justified;
    no third live trial or repeat of the accepted captures by default.

## Planning tranche: cycles31–33 (automatic renewal under D-0014)

28–30 closes with bounded candidate integration limits, exact disk constants,
and the existing1976-snapshot zero-accumulator audit. No new live acquisition.
Budget extends30 to33, used30; objective/scope and protected boundaries unchanged.
31: trace only the known accumulator's activation producers, especially+2D8;
    choose a stimulus only if it can distinguish nonzero fallback behavior.
32: define the minimal caller/owner/gate/time/factor/consumer observation;
    reuse existing instrumentation and validate it offline before any launch.
33: review readiness for one nonredundant activation test; if provenance remains
    opaque, state the exact gap rather than applying an unproved decay patch.
The continuous candidate remains experimental; do not silently equate clock
validation with integrated movement equivalence.

## Planning tranche: cycles34–36 (automatic renewal under D-0014)

31–33 closes with bounded activation setters, observer implementation and
native four-execute restoration plus analyzer positive/negative tests passing.
Budget extends33 to36, used33. Same scope; no new movement patch.
34: one announced60 observation of a user-selected slope-to-flat transition;
    verify exact build, READY,12s bound and automatic restoration.
35: qualify nonzero fallback, forwarded delta, exact decay and immediate
    consumer; zero-only evidence is insufficient, not a bug confirmation.
36: decide the specific next discriminator or causal correction only if proved.
    Do not automatically repeat the route or launch a165 comparison.

## Planning tranche: cycles37–39 (automatic renewal under D-0014)

34–36 closes with a setup mismatch caught before stimulus, corrected strict12s
duration, stopped/restored60 capture and718 exact but zero-only fallback calls.
Disk35-degree threshold reconstruction is consistent with the observed inactive
gate; setter-time direction/live threshold remain unknown. Extend36 to39,
used36. No repeat route or165 pairing is scheduled.
37: qualify an activation route from actual behavior, not screenshot steepness;
    preserve existing evidence if no such route is available.
38: close the narrow live threshold/activation observation gap before another
    capture; no speculative factor patch or fabricated nonzero state.
39: review this lead's priority and the experimental recurrence candidate's
    remaining movement gate; keep insufficient evidence explicit.

37–39 reconciliation: no convenient activating scene is available; a difficult
DLC trip is disproportionate for a hypothesis. Step38 therefore uses existing
candidate data instead of another live activation observation:327 usable
nonunit source samples match, including19 recovery samples. K-0041 qualifies
only the local causal response. No scope change or damping patch.

## Planning tranche: cycles40–42 (automatic renewal under D-0014)

37–39 closes with accumulator lead deferred, candidate source proof and K-0041.
Extend39 to42, used39. No new live trial.
40: replay the candidate's scalar law on already accepted sprint/roll windows
    to check conditional noninterference; do not relabel replay as live acceptance.
41: review remaining known velocity-owner branches dimensionally from existing
    exports; distinguish rate clamps, geometric tolerances and per-call state.
42: consolidate reusable reconstruction and prioritize only a remaining
    discriminating question. No speculative patch or acquisition for coverage.

## Convergence constraint (D-0015)

Minimize causal patch points. Anomalies are witnesses of a possible common root
or temporal abstraction; independent patches require proved independent causes.
Do not compensate in the final writer or add action-specific gains. Preserve
the qualified distance interval and recurrence eligibility/elapsed-time semantics
when assessing one producer-level correction. Physical hook consolidation remains
subject to equivalent behavior and restoration proof, not an assumed outcome.

## Planning tranche: cycles43–45 (automatic renewal under D-0014)

40–42 closes with conditional witness replay and bounded velocity audit; no new defect promoted. Extend42 to45, used42. Apply D-0015 within the same producer/scope.
43: inventory existing feedback intervention sites, governing intervals and eligibility from retained code/evidence.
44: assess a common producer-level temporal contract and whether physical patch consolidation preserves the existing causal behavior.
45: validate any proposed consolidation offline, or retain explicit impossibility/unknowns without speculative implementation. No new game acquisition or downstream compensation.

## Planning tranche: cycles46–48 (automatic renewal under D-0014)

43–45 closes with the producer temporal contract and offline single-write
placement proof; checkpoint preserves1799 retained outputs and explicit visibility
limits. Extend45 to48, used45. Same D-0015 scope, no game acquisition.
46: isolated native state transaction composing the existing normalization and
    recurrence kernels; do not alter accepted/acquisition DLLs.
47: validate event order, owner/update identity, native raw guard and rejection
    without committing partial state; compare retained inputs and independent JS.
48: assess whether consolidation reduces actual intervention complexity without
    hiding unqualified intermediate visibility or broadening reconstructed code.

## Planning tranche: cycles49–51 (automatic renewal under D-0014)

46–48 closes with isolated native transaction/rejection proof and explicit
placement limits. Extend48 to51, used48. Same investigation and causal scope.
49: compare the existing instrument and proposed exit-only write on observable
    intermediate state; use retained producer/caller evidence, no capture repeat.
50: assess a bounded internal-block alternative only if it preserves calls,
    measured intervals and side effects without speculative reconstruction.
51: choose the smallest justified implementation or defer physical consolidation
    explicitly; do not accumulate prototypes merely to reduce a hook count.
Prioritize the remaining candidate movement gate after this placement decision.

Thomas authorized a fresh continuation conversation if history is noisy. This is
continuation of this same I-0007 workstream from its clean unmerged branch checkpoint,
not a new-workstream transfer or authorization to merge main. Resume from canonical
sources and retained evidence; no acquisition is scheduled by the handoff itself.

## Planning tranche: cycles52–54 (automatic renewal under D-0014)

49–51 closes with explicit deferral of physical consolidation and retained-state
visibility audit. Extend51 to54, used51; same objective and protected boundaries.
52: qualify the remaining movement comparison against actual input/animation/
    geometry and preserved binaries. Thomas requests an OSK control test first;
    test response without instrumentation before treating keyboard control as valid.
53: acquire only a discriminating candidate movement trial after announced protocol,
    actual scene readiness and input qualification; reuse retained controls only
    when their matching gates pass. No repeat capture merely to recover artifacts.
54: analyze exact local and world-commit evidence with explicit matching/uncertainty
    gates; stop/restore and retain insufficient information if matching fails.
No scalar-law change, extra hook, i-frame work or automatic merge.

## Planning tranche: cycles55–57 (D-0014)

52–54 closes with stopped/restored release-control23800 and a proved stimulus/window mismatch. Extend54 to57, used54. Thomas authorizes one retry only; do not request another identical maneuver afterward.
55: validate unchanged candidate artifacts and effective10s capture/300s timeout in fresh process; simpler2s+1s physical stimulus.
56: the single authorized60 retry, then stop/restore regardless of outcome.
57: analyze release/movement/exact arithmetic; preserve any remaining gap and replan offline. No automatic165 or additional repeat.

## Planning tranche: cycles58–60 (D-0014, offline only)

55–57 closes with the missing60 release witness and exact restoration; extend57 to60, used57. Thomas's no-immediate-identical-retest constraint remains binding; no acquisition scheduled.
58: reconcile the candidate acceptance boundary: clock/source/60 arithmetic/bounded stop passed, cross-rate trajectory equivalence unproved.
59: bound the pre-factor source/integration contract from targeted retained decompilation, distinguishing captured norm evidence from missing vector/time inputs.
60: define a discriminating offline or controlled-input comparison from that contract; do not substitute another manual route, invent motion gains, or broaden hooks merely for coverage. No automatic live test or merge.

## Planning tranche: cycles61–63 (D-0014, offline only)

58–60 closes with the qualified pre-factor source boundary and explicit absence of shared upstream vector/time inputs. Extend60 to63, used60. No acquisition, patch change or automatic merge.
61: bounded exact-build writer discovery for module+28 block+30..5F, anchored to009A21A0 and verified object provenance; same-offset hits alone are leads.
62: reconstruct any qualified writer's clock/input dependencies; stop at protected or ambiguous ownership rather than fabricate timing semantics.
63: assess whether retained evidence can support a controlled source replay; record insufficiency if the upstream interval remains unknown. No repeated manual route or speculative compensation.

## Planning tranche: cycles64–66 (D-0014, offline only)

61–63 closes with exact module lifecycle, a unique bounded writer, its protected
callback producer and a bidirectional descriptor-selected spatial exchange.
Retained captures omit the descriptor/source block and cannot support the
controlled replay. Extend63 to66, used63. No acquisition, patch or merge.
64: define the minimum observation boundary and fields without duplicating the
    accepted downstream recurrence capture or spending slots on known sentinels.
65: validate strict entry/write pairing and rejection rules in independent
    JavaScript and native state machines.
66: qualify four execute breakpoints, resume handling and exact debug-state
    restoration offline; decide whether an injectable observer is justified.

## Planning tranche: cycles67–69 (D-0014)

64–66 closes with a strict four-site schema and qualified debug mechanism. The
remaining temporal-generation question now justifies a dedicated observer.
Extend66 to69, used66. This does not authorize an unannounced game run or merge.
67: implement a distinct observation-only binary from the validated capture
    architecture; fail closed on correction modes and retain exact site guards.
68: build it, validate disabled/active configuration transitions, strict CSV
    analysis and native/JavaScript rejection fixtures without injection.
69: audit restoration/readiness, persist hashes and protocol, then request only
    the human stimulus required for the new boundary control.

## Planning tranche: cycles70–72 (D-0014, live boundary gated)

67–69 closes with a disabled, hash-bound observer and clean checkpoint8b86238.
Extend69 to72, used69. The game run must be announced; Thomas supplies physical
input. This is a new spatial-boundary measurement, not a release-tail repeat.
70: one target60 six-second structural control: short straight movement then
    release/idle; stop and preserve evidence on any site, pairing or restoration failure.
71: analyze source/derived/transform exchange and callback order. Run one165
    trial only if60 has both strict directions with no rejection or overflow.
72: compare at common elapsed positions with recorded source state, preserve
    mismatch/insufficiency explicitly, stop/restore and replan without extra repeats.

## Planning tranche: cycles73–75 (D-0014, crash audit and offline hardening)

70–72 produced an accepted target60 structural control, then a rejected
target165 crash capture2.8892191s after arm near the operator-observed movement
stop. No retry is authorized or needed for this tranche. Extend72 to75, used72.
73: preserve WER/raw hashes and distinguish durable facts from crash-cause
    hypotheses; reject cross-rate interpretation and avoid a same-test retry.
74: extract all single-rate ordering, slot, lane and argument information that
    remains valid from the accepted60 control; update the knowledge map only at
    the observed boundary.
75: harden periodic spatial-row durability and remove superfluous dispatcher
    logging; qualify two consecutive high-count four-site arms/restorations
    offline, leave the rebuild disabled, and define the next live boundary only
    after review. No automatic launch, merge or correction change.

## Planning tranche: cycles76–78 (D-0014, offline crash-boundary review)

73–75 closes with accepted target60 mapping, rejected target165 payload and a
disabled hardened observer. Extend75 to78, used75; same RCA and engine-mapping
scope, no game launch or repeated maneuver.
76: compare the last durable `ECA94C -> 02344730` boundary with retained healthy
    target165 phase captures and quantify whether that caller is exceptional.
77: export and review the exact-build `02344730` body and containing caller;
    separate ordinary indirect-dispatch structure from evidence of corruption.
78: update the crash assessment, observer gate and canonical resume checkpoint;
    retain an explicit unknown if no dump/register state can identify the bad
    indirect target. No injection, patch-law change, merge or semantic promotion.

## Planning tranche: cycles79–81 (D-0014, shortened live discriminator)

76–78 closes with an ordinary caller boundary, a partial final log row and no
identified crash source. Thomas explicitly resumes the workstream. Extend78 to81,
used78; same spatial-exchange question and accepted target60 control.
79: launch one fresh exact-build process, activate only the hardened observer at
    target165 with a1.5-second measurement window, and verify READY/site/config
    identity before stimulus.
80: acquire one movement-only trial while input remains held beyond automatic
    trial end; then stop, restore and preserve evidence. Do not repeat on failure.
81: require COMPLETE, exact timing/debug restoration, strict bidirectional pairs
    and no overflow before a common-elapsed comparison with the accepted60
    control. Reject or retain insufficiency explicitly. No merge or patch change.

## Planning tranche: cycles82–84 (D-0014, offline owner/caller refinement)

79–81 closes with a cleanly stopped/restored but overflow-rejected target165
prefix. K-0044/K-0045 preserve the useful delta-regime and runtime-owner map.
Extend81 to84, used81; same spatial-generation question, offline only.
82: bind the live RTTI/vtable RVAs to exact-build on-disk table entries and
    determine whether00DD3850/00DD3B50 are direct virtual targets.
83: inspect the bounded call/reference neighborhood that can distinguish the
    transient scaled EnemyIns path from the persistent unscaled path; preserve
    protected/indirect provenance if static evidence cannot select it.
84: validate all new analyzers/artifacts and checkpoint the next distinct
    discriminator. No game launch, capture repeat, merge or correction change.

## Planning tranche: cycles85–87 (D-0014, offline 48-byte block semantics)

82–84 closes with the exact driver vtable/attachment map and an offline-qualified
caller-RVA extension. Extend84 to87, used84; reuse retained spatial rows only.
85: classify all12 float lanes in selected, derived and current-transform blocks
    by constancy, change and finite/unit-vector invariants at each stable slot.
86: test the bounded affine-transform hypothesis against independently sampled
    position and within-block norms without naming fields that fail correlation.
87: record the supported layout and contradictions, validate the reproducer and
    replan from the remaining semantic boundary. No game launch or patch change.

## Planning tranche: cycles88–90 (D-0014, offline derived/world relation)

85–87 closes with K-0047's stable transform layout. Extend87 to90, used87;
reuse the same complete rows and keep the165 rejection explicit.
88: test whether the derived translation group predicts selected-to-current
    world displacement through the selected orientation, including inactive rows.
89: compare displacement norm per broad delta at60 and165 as row-level mapping;
    do not promote the rejected165 prefix to an accepted trajectory.
90: persist the formula, tolerances and reproducer or record refutation; validate
    and checkpoint. No game launch, patch change or merge.
