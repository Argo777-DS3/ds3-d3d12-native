# Timing decisions (D-0003 .. D-0028)

Extracted from `decisions/LOG.md` of the DS3 Decompilation repository (tag `archive/pre-cleanup-2026-10-02`).

## D-0003 — Treat the central timing pair as the cadence-control boundary

Date: 2026-09-19
Status: accepted for future prototype work

**Decision:** any initial exact-build high-framerate prototype should operate only through the central timing object's `+0x354` requested-FPS float and `+0x358` enable byte, applied coherently and restored exactly. Do not add a Present patch, blanket constant replacement, or consumer-specific compensation without new evidence.

**Context:** I-0005 Pass B changed only this pair and causally obtained 60/90/120-class update cadence, approximately `1/target` broad deltas, and matching process-side Present cadence. The I-0004 manual wait stayed inactive and no forced Present override was required.

**Primary reason:** this is the smallest current-build intervention supported by causal runtime evidence and it preserves the engine's own variable-delta path.

**Important limit:** this is a cadence-control boundary, not a claim that animation, movement, camera, collision, physics, or all gameplay behavior is already framerate-independent.

**Revisit if:** deterministic semantic validation finds a reproducible high-level invariant that diverges despite the central pair, or fixed-refresh testing proves a separate presentation intervention necessary.

## D-0004 — Validate semantic consumers before permanent high-framerate implementation

Date: 2026-09-19
Status: accepted

**Decision:** continue I-0005 as one coherent Pass C semantic-invariant phase. Reuse the audited central pair and measure a bounded set of player movement, roll, animation, collision, camera, and movement/physics sentinels at 60/90/120/165, extending to 240/280 only after the lower bounded conditions remain stable. Do not build the final UI or permanent patch yet.

**Context:** Pass B established cadence, delta, and process-side Present behavior but its human-repeated sentinels lacked semantic state identity and deterministic endpoints. Current-build static revalidation now identifies the `WorldChrMan` singleton global; downstream player and animation fields remain secondary-source hypotheses awaiting runtime qualification.

**Primary reason:** framerate unlock is not equivalent to gameplay independence. The next evidence must compare wall time, update count, accumulated delta, position, animation state, input, contact proxies, and active consumers rather than visual impressions.

**Important limit:** the checked-in Pass C probe is preparation, not runtime proof. No Pass C capture had started at this decision checkpoint.

**Revisit if:** a reproducible divergence requires a new intervention class, or the candidate semantic fields fail current-build validation.

## D-0005 — Accept the 60-FPS sprint baseline with an anchoring deviation

Date: 2026-09-19
Status: accepted by Thomas

**Decision:** accept the three-trial target-60 AZERTY sprint series as the bounded sprint comparison baseline even though it failed the predeclared absolute start-position and start-facing tolerances. Preserve the automatic gate result as `false`; record this as an explicit deviation rather than rewriting the evidence or relaxing the general comparator.

**Context:** all three trials were complete and retained 100% valid input, position, and animation samples with the same animation-ID sequence. Net distance CV was 0.0774%, sampled path-length CV 0.3282%, and distance-per-accumulated-delta CV 0.0774%, but manually returning to the Cemetery of Ash bonfire produced a 7.6871-unit maximum start-position spread and 0.06218-radian facing spread.

**Primary reason:** the normalized sprint metrics are substantially inside the 1% within-condition reproducibility tolerance and directly support a useful bounded 60-FPS rate baseline. Thomas explicitly chose to pass this baseline despite the anchoring deviation.

**Important limit:** this decision does not establish exact spatial anchoring, terrain/collision independence, roll reproducibility, 165-FPS correctness, or authorization for 240. Cross-framerate sprint comparisons must retain the measured action/input/animation identity and disclose the baseline deviation.

**Revisit if:** later conditions approach the 2% locomotion threshold, use materially different terrain or animation sequences, or indicate that starting pose affects the result.

## D-0006 — Compare the targeted sprint anomaly directly at 165 FPS

Date: 2026-09-19
Status: accepted by Thomas

**Decision:** for the current locomotion RCA, skip new 90- and 120-FPS sprint acquisitions and compare the accepted bounded target-60 sprint baseline directly with target 165. Preserve the earlier 90/120 cadence and semantic evidence, but do not represent it as action-specific sprint proof.

**Context:** the triggering operator observation concerns incorrect sprint and roll distance at real 165 FPS. The immediate discriminating question is whether the measured sprint distance per accumulated delta diverges from the bounded 60-FPS reference.

**Primary reason:** 165 is the first observed regime where the defect is visible and therefore provides the strongest immediate causal contrast with 60. Thomas's rationale is that once the 165 mechanism is localized to an explicit delta-, update-, animation-, displacement-, or collision-dependent law, that law can be evaluated and predicted for any target framerate without first collecting every intermediate rate. A direct 60/165 comparison is therefore the shortest RCA path.

**Important limit:** a mechanism shown at 165 may be mathematically generalizable, but that is distinct from empirical validation at every framerate. This sequencing choice does not itself validate sprint at 90/120, does not cover roll, retains D-0005's anchoring deviation, and does not authorize 240.

**Revisit if:** the 165 result is ambiguous, falls near the 2% threshold, changes animation identity, or suggests a nonlinear onset requiring intermediate localization.

## D-0007 — Discover the final transform writer before selecting internal vector hooks

Date: 2026-09-19
Status: accepted

**Decision:** separate the protected-boundary sprint RCA into two strictly ordered observation passes. First, at target 60 only, use per-thread write watchpoints on the already qualified final-position fields to identify the exact runtime writer and its immediate data flow. Only after that evidence identifies defensible pre-application and applied-vector sites may a second matched target-60/165 capture instrument those sites. Keep both modes disabled by default and observation-only.

**Context:** all sampled sprint displacement is localized between `02344730:after@00A1429C` and `02344730:before@00DE50EC`, but static bytes around `00A1429C` remain protected/transient. The two caller observations share one runtime context pointer at both 60 and 165, providing a narrow correlation gate without identifying a semantic owner.

**Primary reason:** watching the three known final-position writes is the smallest way to obtain exact commit provenance. It avoids speculative offsets, page-wide trapping, blind heap scans, and premature collision/root-motion labels while supplying the live instruction bytes needed for the next data-flow step.

**Important limit:** writer discovery alone separates only the final commit from its upstream source. Pass 2 remains forbidden unless offline review proves bounded vector sites on both sides of the relevant application/solver region. This decision does not authorize a correction, roll inference, or 240 FPS.

**Revisit if:** the writer watch cannot cover the measured final displacement without unacceptable perturbation, the writer bytes remain undecodable, or no bounded applied-vector source can be traced.

## D-0008 — Align Work conversations with coherent project workstreams

Date: 2026-09-19
Status: accepted by Thomas

**Decision:** use one Work conversation per coherent new workstream, but create
it only at a clean checkpoint. A change of central question, major experimental
phase, or type of work warrants a new conversation after all necessary state is
canonical in its source for that information type, the checkpoint is validated
and integrated on `main`, and resumption is possible exclusively from the
canonical source set routed by `.project/INDEX.md`.

**Context:** long reverse-engineering threads can accumulate refuted
hypotheses, several probe generations, superseded results, and historical noise.
Conversely, splitting every sub-step or micro-test would fragment one causal
investigation and make integration harder.

**Primary reason:** the repository must own durable project memory while each
conversation remains a bounded working context.

**Important limit:** do not split a conversation for a local variation while
the central question is unchanged, and do not transfer mid-checkpoint. A
saturated conversation prepares its handoff at the next coherent checkpoint.

**Revisit if:** clean canonical-source-only resumptions repeatedly lose
necessary working context, or a workstream remains too broad despite
checkpoint-aligned transfers.

## D-0009 — Move from the closed static writer bridge to one input mirror

Date: 2026-09-19
Status: accepted by Thomas

**Decision:** treat static reconstruction of the confirmed final writer as
complete. The next coherent workstream asks only which confirmed writer input
first explains the approximately `0.5` target-165 amplitude ratio. Use one
unchanged deterministic target-60/165 acquisition at pre-commit RVA
`009DC559`; do not reopen exploratory hook discovery or search new engine
regions.

**Required discrimination order:** first compare the effective candidate
offset, then the pre-reset `+0x1C0` scalar, then bit-reconstruct the local
packed-float calculation producing `XMM6`. Capture source selection
`+0xB0`/fallback `+0xA8`, nested-source validity, `+0x1E1` inhibition, the
qualified update/context identity, and the already validated temporal delta in
the same record set.

**Primary reason:** K-0032 and K-0033 already establish the writer, exact local
law, branch priority, scalar consumption, and commit gate. The shortest
remaining causal test is therefore an input comparison at that confirmed point,
not another localization campaign.

**Important limit:** the candidate source remains semantically neutral. This
decision authorizes preparation of the mirror, not a correction or an
unannounced DS3 launch. Semantic Pass 2, roll, correction, and 240 remain
closed. Immediately before any launch, state in one sentence what the run will
distinguish and how candidate, scalar, and local-calculation outcomes will be
interpreted.

**Revisit if:** the single point cannot be correlated deterministically with
qualified mobile commits, or captured inputs fail bit-exact reconstruction of
`XMM6`; in either case stop and revise the acquisition rather than expanding
the hook surface.

## D-0010 — Authorize causal correction upstream of the selected A8 candidate

Date: 2026-09-19
Status: accepted by Thomas

**Decision:** open I-0007 from K-0034 and trace the selected neutral
`A8 -> +0x40 -> +0x70` candidate upstream until the first operation creating
the target-165 amplitude loss is demonstrated. Once the first wrong value,
exact formula, and cadence dependency are established, implement the smallest
reversible correction at that operation and validate sprint at target 60 and
165. After sprint passes, test roll with the exact same patch and no
roll-specific adaptation.

**Primary reason:** I-0006 proves that source selection, inhibition, scalar,
and final-writer arithmetic are not the origin; the approximately `0.5` loss is
already present in the consumed candidate. Correction is therefore authorized
only at the first proven upstream causal operation.

**Important limit:** do not compensate `009DC4D0/009DC580`, use arbitrary
multipliers or refresh-rate tables, or assign gameplay semantics without proof.
If roll remains incorrect after sprint passes, open a separate roll RCA. 240,
unlimited, general refactoring/optimization, and a distributable patch remain
closed. Every live run remains announced and Thomas-operated.

**Revisit if:** two local fixes fail, protected or opaque code prevents causal
reconstruction, multiple upstream chains remain indistinguishable despite
adequate evidence, or the target-60 non-regression and target-165 2% sprint
gates conflict.

## D-0011 — Sprint and roll are witnesses, not the whole high-framerate objective

Date: 2026-09-19
Status: reaffirmed by Thomas

**Decision:** the intended result is correction of all problems caused by
unlocked high framerate. Sprint/roll are examples and symptoms used to prove
the current causal chain; their success must never be labeled overall C4
completion. Correct underlying formulas rather than action-specific symptoms.

**Execution consequence:** finish the already ordered same-correction roll
validation after sprint, then use canonical temporal knowledge to establish
explicit coverage of other relevant systems and investigate remaining defects.
Keep confirmed fixes, measured regressions and untested coverage distinct.
Closing the bounded I-0007 chain is at most one C4 checkpoint, not project success.

**Unchanged limits:** this clarification does not itself authorize 240/unlimited,
unjustified broad instrumentation, optimization/refactoring, gameplay semantics
without proof or a distributable patch. Future work stays evidence-led and bounded.

## D-0012 — Roll acceptance includes unchanged invulnerability

Date: 2026-09-19
Status: required by Thomas

**Decision:** roll must preserve the reference i-frames. Displacement and
animation matching alone cannot qualify gameplay roll correctness. Establish
the actual engine representation and consumption of invulnerability, then
compare active windows, action-relative boundaries, effective elapsed duration
and any demonstrated logical-frame count against60. Do not substitute the
number of rendered frames or assume that unchanged animation proves this gate.

**Current evidence gap:** the existing roll capture measures input, motion,
animation and the causal scalar/commit chain, not invulnerability state or
damage rejection. Finish the useful pending165 motion capture and retain it,
but label its result motion-only until direct invulnerability proof exists.
No guessed frame count, roll-specific compensation or extra game test without
an announced discriminating protocol.

**Sequencing clarification by Thomas:** invulnerability work is deferred;
finish the current same-patch60/165 roll motion comparison first. This does
not make the unmeasured i-frame gate pass or authorize that new investigation now.

## D-0013 — Recover from preserved branches before choosing a resume baseline

Date: 2026-09-20
Status: required by Thomas

**Decision:** maintain a short resume pointer in the existing `.project/STATE.md`
on a requested handoff, pause, conversation change or session end. Apply the
branch/evidence reconciliation procedure in `.project/WORKFLOW.md` on every
new or context-lost session. Never assume main is newest, repeat completed
work before checking retained evidence, or automatically merge for recovery.

**Observed incident and verification:** main at `7a7c357` stopped at I-0006;
the preserved `codex/i-0007-locomotion-165-rca-fix` ref held 23 further commits
through `92c2b79`, including accepted causal sprint/roll-motion results. Its
worktree was absent. Reading the preserved ref's state, result and manifest
recovers the actual position; the raw-evidence audit is recorded under I-0007.
The required cross-ref check also works when the checkpoint loaded from older
main lacks the new resume section. The protocol would select that retained
work branch, without merging or redoing the RCA.

**Limit:** this is a hardening of D-0008's existing lifecycle. Ownership,
delegation, canonical source allocation, budgets, validation and planned
clean-main transfer policy are unchanged. No new file or registry is needed.

## D-0014 — Treat cycle budgets as planning tranches

Date: 2026-09-20
Status: required by Thomas

**Decision:** an exhausted budget requires a recorded result/checkpoint and a
new bounded plan, not renewed user authorization when objective and scope stay
unchanged, evidence supports continuation and no protected boundary is crossed.
Use the existing contract/result/status files; retain cumulative usage and
extend the maximum explicitly. Existing scope, governance, irreversible/action,
merge and other protected gates remain effective, as does prior DS3 launch
announcement. Do not use automatic renewal to hide stagnation.

**Application:** tranche 1 of I-0007 closes at 12/12 in `4ec3c9a`; Thomas also
explicitly approved three further cycles, now 12/15. Continue eligibility/time
qualification and the remaining transient comparison without repeating accepted
sprint/roll proof or changing the causal patch speculatively.

## D-0015 — Converge on shared causal timing correction

Date: 2026-09-20
Status: required by Thomas

**Decision:** use discovered anomalies to identify a shared root cause or
temporal abstraction. Prefer the smallest justified set of causal patch points;
add a distinct correction only when evidence demonstrates an independent cause.
Do not stack symptom-specific or downstream compensations. A common abstraction
must preserve each measured interval and branch contract; a shared label alone
does not prove a shared clock or justify changing native elapsed-time behavior.

**Application:** the accepted distance correction and experimental recurrence
belong to the same feedback producer. Assess their joint temporal contract before
adding hooks. The unactivated accumulator remains a hypothesis, not a second fix.
Existing acceptance, scope and integration gates are unchanged.

## D-0016 — Add a disposable discussion interface over durable technical state

Date: 2026-09-20
Status: superseded by D-0017

**Decision:** during sustained technical execution, use one lightweight
Discussion Interface Agent when concurrent workers are available. It answers
ordinary questions and comments from the canonical repository while the
technical owner continues the workstream. It immediately forwards decisions,
scope or constraint changes, relevant corrections, required authorizations,
human-intervention results and explicit `PAUSE` or stop commands. When
concurrency is unavailable, apply the same classification within the current
session and resume technical execution after ordinary conversation.

**State boundary:** the interface owns no investigation, tools, experiment,
checkpoint or durable memory. It creates no second state store. The resume
checkpoint, investigation records, decisions, ledger and Evidence Stores remain
the only canonical route; verified repository state overrides chat history.
The interface can be discarded and reconstructed from those records after
compaction, model change, restart or conversation loss.

**Continuity and PAUSE:** conversational replies do not end a task, cycle or
workstream. Five questions or comments leave the technical next action
unchanged. An explicit `PAUSE` is the immediate exception: the interface routes
it to the technical owner, which performs the existing restoration, evidence,
resume-checkpoint and consistency procedure before stopping. The interface may
confirm the pause only after that checkpoint is ready.

**Limit:** this changes message routing only. Technical ownership, canonical
source allocation, scope, budgets, validation, live-game announcements,
integration and merge gates remain unchanged. No bus, registry or interface
handoff file is introduced. In runtimes where a worker cannot address the user
directly, the technical owner relays its answer and remains the sole input
router.

## D-0017 — Remove the discussion interface layer without replacement

Date: 2026-09-20
Status: directed by Thomas

**Decision:** after I-0009 reached its cycle-5 offline checkpoint `84aaf27`,
stop the active Discussion Interface Agent and remove only the rules specific to
that layer from `AGENTS.md` and `.project/WORKFLOW.md`. Do not replace it with
another routing mechanism for now.

**Preserved unchanged:** the main technical workflow, default work continuity,
`PAUSE`, resume/recovery checkpoints, technical ownership and governance,
Evidence Stores, validation/integration boundaries and the active I-0009
contract remain in force. D-0016 is historical and no longer active.

## D-0018 — Validate the accepted locomotion correction at 240 FPS

Date: 2026-09-20
Status: directed by Thomas

**Decision:** open I-0010 from completed I-0009 checkpoint `e81316e` for one
bounded validation of the exact same accepted locomotion correction at actual
target 240. Keep the comparison formula and all guards unchanged; this is a
new empirical target validation, not a new fix and not a reopening of I-0009.

**Execution boundary:** first recover canonical state, create the dedicated
branch/contract, and verify primary-display 240-Hz headroom. Then derive and
validate a disabled-by-default I-0010 artifact offline. Before any DS3 launch
or injection, announce the exact short protocol and wait for Thomas. Preserve
PAUSE, checkpoints, governance, Evidence Stores and exact restoration.

**Unchanged limits:** unlimited FPS, a different formula or guard set, new
causal hooks, roll i-frames/damage, other actions/states, whole-C4 completion,
persistence, distribution and integration into `main` remain unauthorized.
The Discussion Interface Agent remains retired under D-0017 with no replacement.

## D-0019 — Accept the observed ~229 FPS condition for I-0010

Date: 2026-09-20
Status: directed by Thomas

**Decision:** after the first I-0010 session requested 240 and measured
229.002670 FPS at the original cadence checkpoint, Thomas explicitly accepted
229 FPS as a sufficiently high framerate for the bounded locomotion validation.
Continue I-0010 against a target-240 request with a declared measured-cadence
floor of 229.0 FPS. Report requested and actual cadence separately; never label
the resulting witness as actual 240 FPS.

Thomas identifies the shortfall from requested 240 as a hardware-performance
limit and explicitly does not treat it as a correction failure. Preserve that
operator interpretation, while keeping the specific limiting component
unconfirmed because this session did not instrument CPU/GPU ownership.

**Execution consequence:** the first process was already stopped and fully
restored, and its loaded DLL is inert. Require a clean DS3 restart before the
revised OFF/ON witness; do not reinject the stopped process. Extend the planning
tranche from 7 to 9 cycles for the revised live witness and closure.

**Unchanged limits:** formula, guards, hooks, sentinels and restoration remain
identical to I-0009. Unlimited FPS, i-frames/damage, other actions, persistence,
distribution and integration into `main` remain outside I-0010.

## D-0020 — Build runtime FPS control and overlay as I-0011

Date: 2026-09-20
Status: directed by Thomas

**Decision:** after authorized integration of completed I-0010 into `main`, open
I-0011 as an implementation workstream for one compact HOME-toggle overlay with
runtime numeric FPS control, presets, genuine limiter-off Unlimited when the
architecture permits, measured/target FPS display and locomotion correction
ON/OFF. Use the exact accepted causal correction independently of selected
cadence; no multiplier table or cadence-specific correction profile is allowed.

**Execution boundary:** prioritize functionality, robustness, reversibility and
simple ergonomics. Reuse I-0009/I-0010 infrastructure, minimize hooks and
dependencies, and do not turn I-0011 into renderer-wide work or reopen closed
RCA absent a demonstrated implementation blocker. The complete requirements,
validation matrix and exit conditions are canonical in the [I-0011 contract](../investigations/I-0011-runtime-fps-control-overlay/CONTRACT.md).

**Safety:** the implementation starts inert, modifies no game file, announces
every live launch/test, restores all runtime state exactly, and keeps attack,
jump, i-frames, broader C4, packaging and distribution outside the qualified
result unless separately authorized and validated.

## D-0021 — Accept I-0011 as test infrastructure and stop overlay polish

Date: 2026-09-20
Status: directed by Thomas

**Decision:** after the second I-0011 live session, treat the HOME overlay as a
test tool rather than the final product interface. Accept its remaining slight
jitter and requirement to open a DS3 menu before mouse interaction. Do not spend
another I-0011 iteration on window/input polish or interpret those UI defects as
a failure of the temporal correction.

**Accepted result:** Thomas reports that the unchanged correction behaves
correctly for every player animation he observed across the exercised
framerates, explicitly including light sprint and light roll, and that enemies
do not appear slowed. Preserve this as broad positive operator evidence for the
temporal-branch theory.

**Evidence boundary:** the operator conclusion does not by itself prove every
animation, enemy update or other temporal subsystem invariant. Keep whole-C4,
roll i-frames/damage, camera, effects, collision/simulation, online behavior,
product UI, persistence and distribution outside the proved I-0011 boundary.
Close I-0011 at its 12/12 budget with exact session cleanup and no further live
test scheduled.

## D-0022 — Open I-0012 for enemy and boss timing validation

Date: 2026-09-21
Status: directed by Thomas

**Decision:** integrate completed I-0011, then open I-0012 in a new Work
conversation to determine whether the accepted cadence-independent temporal
correction preserves non-player behavior or reaches a genuine enemy/boss timing
boundary. Prioritize concrete visible and measured gameplay results over broad
exploratory decompilation.

**Required panel:** establish or reuse 60-FPS references and compare high
framerate behavior for one accessible humanoid trash mob, one early-game dog
and Iudex Gundyr. Cover spatial displacement, attack-integrated movement,
charges/leaps where present, rotations, transitions and recovery sufficiently
to distinguish animation smoothness from behavioral equivalence.

**Test interface:** retire the I-0011 independent overlay from active use and
replace its control role with a lightweight ReShade add-on. ReShade is a test
host only; keep the cadence/correction core independent and reusable. UI polish
is subordinate to stable, practical validation. Preserve the real Unsupported
result for Unlimited unless a genuine limiter-off mechanism is established.

**Causal boundary:** do not create enemy-, boss- or cadence-specific patches.
If a non-player divergence survives the unchanged correction, localize an
independent cause before proposing another intervention. Retain CONFIRMED,
SUPPORTED, OBSERVED and UNKNOWN distinctions and do not promote operator
impressions to exhaustive whole-C4 proof.

**Safety:** I-0012 starts from clean integrated `main` with a fourteen-cycle
initial budget. Inventory ReShade/game-directory state and define a reversible
installation before any file change or live load. Announce every DS3 launch and
give Thomas short action protocols for aggro, attack baiting or boss phases.

## D-0023 — Accept I-0012 and redirect terrain hopping to I-0013

Date: 2026-09-21
Status: directed by Thomas

**Decision:** close I-0012 as a bounded success after Thomas's independent
functional test. Accept the Hollow, dog and Iudex Gundyr panel as a visual
gameplay pass without requiring the deferred actor measurement phase. Preserve
that result as `OBSERVED`; do not promote it to instrumented non-player timing
equivalence or whole-C4 correctness.

**Tool boundary:** retain standard windowed mode as the usable test condition.
Exclusive fullscreen produced no effect and is accepted as a host limitation
for this tool. Preserve the ReShade lifecycle log, remove the exact installed
add-on file after the game closes, and do not infer the fullscreen cause from
runtime recreation alone.

**Successor RCA:** open I-0013 in a new Work conversation for the newly observed
player hopping/jitter on slopes and stairs. Its first live discriminator is the
same-route matrix `60 OFF`, `240 OFF`, `240 ON`, plus inexpensive `60 ON`
non-regression control. The matrix must decide whether the issue is independent
of, introduced by, or already covered by the locomotion correction before any
new collision intervention is proposed.

## D-0024 — Keep I-0013 on terrain RCA; do not promote the speed side signal

Date: 2026-09-21
Status: directed by Thomas

**Decision:** accept the completed first I-0013 matrix as the initial causal
discriminator for visible terrain hopping. Normal behavior at both 60 cells and
the same hopping at both approximately 239-Hz cells support H1: the observation
exists independently of the accepted locomotion correction. Do not repeat the
matrix solely to investigate Thomas's tentative impression that 240 may be
slower.

**Evidence boundary:** preserve the active-window calculation showing an
apparent 3.7–5.3% high-frequency speed reduction, together with the unintended
attack in `240 OFF` and unequal manual traversal durations. Keep that result
unpromoted and do not treat it as a new temporal defect without a future clean
reproduction.

**Execution consequence:** continue I-0013 with offline exact-build analysis
and, only if needed, one targeted read-only discriminator for the terrain
hopping boundary. Collision/proxy response is a hypothesis to test, not an
assigned owner; no correction or collision intervention is authorized by this
decision.

## D-0025 — Require a general mathematical correction for I-0013

Date: 2026-09-21
Status: directed by Thomas

**Decision:** any eventual I-0013 fix must restore a clearly identified general
mathematical invariant. Acceptable forms include converting displacement and
velocity with the actual elapsed time or converting a discrete recurrence to a
time-continuous law. The result must preserve native 60 behavior and vary
continuously across intermediate and high update rates.

**Rejected forms:** do not use a 240-specific multiplier, hand-tuned vertical
offset, route/terrain threshold, per-FPS table or post-hoc visual clamp. A
symptom reduction on the current stair route is not sufficient without a
causal boundary and justified formula.

**Execution consequence:** reuse I-0008 only as an independent timing control,
and use bounded Ghidra cartography only where it advances the terrain/proxy RCA.
The prepared read-only source/cache/proxy/root observation remains the next
discriminator; this decision authorizes no patch.

## D-0026 — Accept and close I-0013 with the producer time law

Date: 2026-09-21
Status: directed by Thomas

**Decision:** accept the I-0013 elapsed-time producer correction and close the
investigation. The target-60 correction-ON control made zero writes and returned
181/181 native results bit-for-bit. At 237.084 sampled calls/s, all 712 retained
rows matched the derived additive formula exactly, and Thomas reported that the
familiar slope/stair hopping disappeared with no observed anomaly.

**Mathematical boundary:** the active additive recurrence is converted by
`step * dt/dt60`; the separately detected multiplicative branch uses
`retention^(dt/dt60)`. The implementation uses the supplied live delta and no
FPS table, 240-specific multiplier, terrain threshold, empirical offset or
post-hoc visual clamp. Native60 is explicit no-write passthrough.

**Limits:** acceptance is bounded to the exact build, current-player identity,
captured mode-1 route and reversible prototype. The decay branch, other modes,
actors, exhaustive terrain, online use, persistence, packaging and distribution
remain unvalidated. Closing I-0013 does not establish whole-C4 completion.

**Lifecycle:** preserve the stopped live binary, raw rows and exact cleanup in
the Evidence Store. The branch remains unmerged until Thomas separately
authorizes integration. Do not open I-0014 automatically; Thomas will initiate
that workstream.

## D-0027 — Canonicalize LTF and TRTF as distinct corrections

Date: 2026-09-22
Status: directed by Thomas

**Decision:** use `LTF — Locomotion Timing Fix` for the I-0007 correction
validated on sprint and roll motion, and `TRTF — Terrain Response Timing Fix`
for the I-0013 producer-recurrence correction that removes the reproduced
high-rate slope/stair hopping. Use these names systematically in canonical
results, checkpoints, contracts, implementation state and future conversations.

**Separation:** LTF and TRTF are technically and causally distinct. Their
operations, intervention sites, guards, telemetry, proof chains and validation
limits must remain independently identifiable. Do not collapse either into the
other or use ambiguous relative labels such as "old fix", "new fix" or
"previous fix" when the applicable correction can be named.

**Future user surface:** a later workstream may study a common user-facing
activation for LTF and TRTF. A shared control must remain an orchestration layer
and must not erase their distinct internal identities or evidence boundaries.
Historical raw evidence remains unchanged and is interpreted through the
[canonical terminology](../.project/TERMINOLOGY.md).

## D-0028 — Open I-0014 as a unified timing-fix test host

Date: 2026-09-22
Status: directed by Thomas

**Decision:** open I-0014 from the clean integrated I-0013 checkpoint and build
one practical ReShade test add-on containing the already accepted LTF and TRTF.
Expose independent ON/OFF controls and retain cadence presets, target/measured
FPS, Native restoration and clean stop/removal. ReShade remains the control
host; LTF and TRTF retain separate formulas, sites, guards, state, telemetry and
restoration.

**Display-mode amendment:** the tool must work in both windowed and exclusive
fullscreen modes. Because I-0012 observed an unlocalized fullscreen failure,
I-0014 adds a renderer-lifecycle-independent keyboard fallback rather than
altering either correction. Offline structure is necessary but not a live
fullscreen claim; Thomas's manual phase validates actual behavior.

**Boundary:** do not add distance instrumentation, automated routes, empirical
FPS constants, visual compensation or another locomotion RCA before Thomas's
manual 60/240 comparison. No game launch occurs during the offline build phase.
I-0014 remains active after the phase-1 checkpoint until that report.

