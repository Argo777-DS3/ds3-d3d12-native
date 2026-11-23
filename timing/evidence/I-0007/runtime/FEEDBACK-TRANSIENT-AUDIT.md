# Remaining feedback transient: offline continuation

I-0007 cycle 11/12, 2026-09-20. This is a continuation of the recovered RCA,
not a new investigation or repetition of accepted gameplay captures.

## Confirmed local computation

The existing exact-build assembly at `005C73EE` and `005C7419` multiplies the
source scalar by float32 `0.8` or `1.2` per eligible call. Neither multiplication
uses elapsed time. Conditional resets happen on later calls; these are not
strict clamps. [Assembly](../static/source-scale/DS3-1152-F-005C72B0.asm.txt)
and [constant bytes](../static/source-scale/constants.csv) establish the law.

With the branch held constant, the float32 sequences are approximately:

- reduction: `1 -> .8 -> .64 -> .512 -> .4096 -> .5`;
- recovery: `.5 -> .6 -> .72 -> .864 -> 1.0368 -> 1`.

Both take five eligible invocations. **If** the same branch runs once per update,
this corresponds to 83.33 ms at 60 Hz and 30.30 ms at 165 Hz. That is a
conditional model of a cadence dependency, not a measured second gameplay bug.

## Existing runtime evidence

[Offline reproducer](audit_feedback_transients.mjs) checks the current
reconstructor output against that recurrence and preserves the exact observed
transition in [the audit](reconciliation-20260920/feedback-transient-audit.json).

The V2 patched capture contains one measured reduction at update 20793 and three
non-unit outputs: `.8 -> .96 -> 1.152`, followed by `1`. This was already
disclosed in the sprint validation; it is not a new discovery of broken sprint.
V3 full-duration sprint and the roll captures have zero non-unit outputs across
1,260 and 1,124 captured windows respectively. Their successful stable behavior
cannot discriminate recurrence response time away from equilibrium.

## Interpretation and next discriminator

Residual cadence-dependent acceleration/recovery is a **hypothesis**. The local
per-invocation law is confirmed, but harmful gameplay impact and a correct
replacement are not. A fixed gain alone is insufficient evidence to patch it.

The next bounded step must identify the recurrence's invocation/eligibility
interval and obtain comparable non-equilibrium transitions at 60 and 165. Use
the known producer and current patch as the foundation. Qualify activation,
scalar input/output, elapsed time, position and input transitions. Do not repeat
the stable three-second sprint or single-roll acceptance windows: they supply
no new discrimination. Prefer existing captures; any necessary new live
stimulus must be short, operator-controlled and announced under the contract.

Acceptance would require the same reference transition over elapsed time, with
the accepted distance correction unchanged, and a displacement effect beyond
input/capture uncertainty. A lack of comparable active transitions is
insufficient information, not evidence of correctness or failure.

Simply replacing gains with `gain^(elapsed/(1/60))` is not yet justified: it
changes when the conditional reset and one-step overshoot occur. The correct
elapsed interval for recurrence evolution also cannot be assumed to equal the
already proven N-1 distance interval when invocation gates skip calls. No new
live patch has been implemented or enabled from this audit.

## Cycle 12: invocation-interval audit

[Reproducer](analyze_recurrence_intervals.mjs) and
[raw-bound interval report](reconciliation-20260920/recurrence-interval-audit.json)
reinspect the three existing patched sessions. The nine source-file hashes
match their historical summaries. No live acquisition or patch was performed.

| Trial | Recurrence exits | Adjacent same-identity pairs | Comparison observed | Non-unit outputs |
|---|---:|---:|---:|---:|
| V2 sprint 60 | 241 | 240 | 208 | 0 |
| V2 sprint 165 | 653 | 652 | 566 | 3 |
| V3 full sprint 165 | 1,260 | 1,259 | 1,042 | 0 |
| Roll 60 | 300 | 299 | 26 | 0 |
| Roll 165 | 824 | 823 | 69 | 0 |

Every complete captured broad interval has one exit at `005C7435`. All 1,911
compared scalar inputs equal the previous captured exit value, with matching
trial/thread/builder/context and consecutive broad indices. This is observed
net value continuity, not proof that no other writer could touch that field.
The normalized distance's elapsed time equals the preceding broad delta.

The crucial limitation is visible in the existing assembly: the outer virtual
eligibility result can jump from `005C734E` straight to that same exit. An exit
without comparison and with scalar 1 cannot distinguish an inactive recurrence
from an active recovery/reset. The stored record does not directly capture
that earlier return value. Likewise measured QPC exit-to-exit wall time does not
establish which simulation interval should govern a new gain law.

Timing-module `+18/+1C` snapshots differ from the same-index broad delta on
13 V2-165, 49 V3-165 and 15 roll-165 rows (zero in these 60 trials). This is
a further reason not to substitute a convenient timing field without its own
proven pairing. No causal interpretation of those differences is asserted.

**Result: insufficient information for a residual gameplay defect or new
correction.** There is no non-unit target-60 reference in these captures. The
single V2 target-165 transient remains unmatched; the accepted sprint/roll
results are unchanged. Historical cycle12 reached12/12. Thomas subsequently authorized cycles13–15; D-0014 governs automatic same-scope replanning.

## Bounded continuation (authorized 2026-09-20; original plan below)

Extend this same I-0007 by three cycles, rather than reopen I-0006 or create
another investigation:

1. Qualify eligibility and time identity in the same known producer. A candidate
   additional observation is `005C734C`, immediately after the virtual call,
   before `TEST AL,AL`. Capture AL, scalar-at-entry, object/thread/update and
   clocks. Pair with existing comparison/exit/commit records. AL=0 plus equal
   entry/exit proves a skipped invocation; AL!=0 and no comparison must match
   the native recovery/reset law. Validate exact bytes and four-slot hardware
   breakpoint handling offline before any live use. Keep the accepted distance
   correction unchanged and all configurations disabled by default.
2. Acquire a short matched 60/165 **non-equilibrium** control only if an eligible
   transition can be qualified: a bounded approach/contact/release at the same
   obstacle is a candidate stimulus, not yet a validated protocol. Observe input
   edges, position and scalar reduction/recovery rather than assuming the action
   causes it. Stop if no transition is observed, identity fails, or input/path
   mismatch prevents comparison. Do not repeat the already accepted stable
   sprint/roll windows. Thomas must receive the final discriminating protocol
   before launching or acting. Thomas subsequently authorized assistant launch/control with prior announcement; he currently performs the stimulus.
3. Reconstruct the resulting time law and displacement consequence. Implement
   and verify a minimal reversible correction only if the matched data proves
   a defect and its governing elapsed interval; otherwise record insufficient
   information. Preserve reference behavior, conditional resets and the existing
   comparison correction. No scalar bypass, arbitrary gain, i-frame work or
   240-FPS expansion.

These are budgeted exit gates, not a promise that three cycles guarantee a
corrected residual defect. Any failed prerequisite stops dependent patching.

Historical operational update (superseded by [current protocol](TRANSIENT-PROTOCOL.md)): Thomas reports DS3 window control is now fixed. A read-only
`@oai/sky` initialization and window inventory succeeded in this session, but
DS3 was not among returned windows. No game was launched or controlled. Future
window/input automation must first qualify the actual game target and input
timing against the instrument logs; it does not substitute for measured stimulus
identity or authorize an unannounced test.
