# I-0012 — Enemy & Boss Timing Validation

Capability: C4

Canonical correction name: **LTF — Locomotion Timing Fix**. All correction
ON/OFF controls and inherited formulas in this contract refer to LTF. The
terrain observation later led to the separate I-0013 TRTF.

## Question

Does the cadence-independent LTF already validated on the
player also preserve enemy and boss movement, attack displacement, animation
transitions and recovery at high framerate, or is there a causally distinct
non-player timing path?

## Workstream type and priority

This is a visible gameplay-validation workstream. Obtain concrete comparisons
early; extend mapping or decompilation only to explain a demonstrated result or
to design the next discriminating measurement. Do not begin with an open-ended
enemy/boss RCA.

## Included scope

- Retire the I-0011 independent overlay from active testing and replace its
  control role with a lightweight ReShade add-on.
- Keep cadence control and the accepted LTF core independent of
  ReShade so another host can reuse them later.
- Expose correction ON/OFF, numeric FPS control, useful presets, target FPS,
  reliable actual FPS when available, Native restoration and genuine Unlimited
  only if a real limiter-off mechanism is established. Do not fake Unlimited.
- Prefer functionality, stable interaction and fast test operation over visual
  polish. Record non-blocking UI limitations and continue.
- Establish or reuse a 60-FPS behavioral reference and compare one
  discriminating high cadence, defaulting to 165 FPS unless evidence makes 240
  materially more informative. Add OFF comparisons only when they discriminate
  cause or effect.
- Validate a minimal panel:
  1. an accessible humanoid trash mob;
  2. an accessible early-game dog;
  3. Iudex Gundyr as the primary complex actor.
- Compare actor speed and path distance, attack-integrated displacement,
  jumps/charges where present, movement-to-attack-to-recovery transitions,
  apparent recovery duration, rotations/direction changes, cadence-linked
  slowdowns or accelerations and overall animation coherence.
- Use repeatable actor identity, transform, animation/state and event-aligned
  timing evidence where practical. Keep Thomas's visual observations separate
  from instrumented measurements.
- Observe Gundyr phase 2 only when it can be obtained cleanly without turning
  this bounded study into a campaign.

## Excluded scope

- Product-quality ReShade UI, an installer, broad graphics work or treating
  ReShade as part of the causal correction.
- Exhaustive validation of every enemy, boss, attack, animation or game area.
- Mandatory Irithyll Knight, Vordt or additional morphologies unless the
  minimal panel leaves the central question ambiguous.
- Boss-by-boss, enemy-by-enemy or cadence-specific correction tables.
- A distinct enemy/boss patch without evidence of an independent cause.
- Roll i-frames/damage, online behavior, packaging/distribution and a blanket
  whole-C4 correctness claim.
- Reopening accepted player RCA or repeating I-0007 through I-0011 acquisitions
  unless a new non-player discriminator strictly requires a reused observation.

## Inputs

- Clean integrated baseline: `main` after I-0011 closure checkpoint `08d681d`.
- Exact game: Steam AppID 374320 / BuildID 10167187 /
  `DarkSoulsIII.exe` 1.15.2.0, SHA-256
  `EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C`.
- Accepted LTF: `raw * ((1/60) / previous_broad_delta)` with the exact
  I-0009/I-0010/I-0011 opcode, identity, consecutive-index, transform,
  position-pair, finite-delta, exact-build and restoration guards.
- I-0011's numeric cadence state machine, measured-FPS path, explicit
  unsupported Unlimited result, reversible cleanup, logs and operator control
  experience.
- Canonical claims K-0037 through K-0039 and K-0054 through K-0056, plus the
  bounded limits recorded in the I-0007 through I-0011 results.

## Hypotheses

- H1 — General/shared behavior: the tested non-player actors are already
  cadence-correct or traverse the same causally corrected temporal mechanism;
  their event-aligned spatial and temporal behavior remains comparable to 60
  with the unchanged correction and no actor-specific patch.
- H2 — Player-only reach: the current guarded correction fixes the player while
  enemies remain correct through another native delta-scaled path; the visible
  result is general game correctness but not direct correction of enemy state.
- H3 — Shared defect, insufficient reach: at least one non-player actor shows a
  cadence-dependent spatial/temporal divergence that the current guarded path
  does not reach. Generalization is allowed only after common cause is proved.
- H4 — Independent divergence: an actor class or Gundyr retains a defect from a
  separate temporal mechanism. Localize that boundary before proposing any
  additional correction.

## Evidence and comparison rules

- `CONFIRMED`: reproducible instrumented/event-aligned comparison with exact
  build, cadence, actor identity, intervention and cleanup provenance.
- `SUPPORTED`: several coherent observations or measurements with bounded
  coverage but an incomplete invariant or sample.
- `OBSERVED`: Thomas's visual/gameplay report or an unmeasured visible result.
- `UNKNOWN`: untested, ambiguous or unexplained.

For each retained actor case, record selected and measured cadence separately,
the correction state, the matched action/state boundary, position/path/time
metrics when available, visible assessment, failures/exclusions and exact
restoration. Animation smoothness alone is not a spatial/timing pass.

## Bounded fourteen-cycle plan

1. Recover the integrated checkpoint; inventory installed ReShade state,
   I-0011 reusable cores and exact-build constraints without launching DS3.
2. Select the smallest official ReShade add-on surface and define reversible
   installation, IPC/state ownership and third-party provenance.
3. Refactor/build the cadence and temporal-correction core so the ReShade host
   is only UI/control integration.
4. Implement the minimal add-on controls and clean Native/stop restoration.
5. Pass strict build, API/provenance, exact-build, state-machine, formula/guard
   identity and restoration tests; prepare a short live smoke protocol.
6. Run one announced clean-process UI/control smoke test and preserve cleanup.
7. Select accessible instances and the smallest reusable non-player
   actor/transform/animation measurement surface.
8. Acquire/establish the humanoid 60 reference and one high-cadence comparison;
   stop early if the bounded control is already decisive.
9. Acquire the dog reference/comparison with emphasis on approach, leap,
   attack displacement, recovery and direction change.
10. Establish Gundyr's repeatable 60-FPS action/state reference.
11. Compare Gundyr at high cadence with correction OFF when informative.
12. Compare Gundyr at the same cadence with the unchanged correction ON,
    including phase 2 only if cleanly reachable.
13. Analyze the cross-actor boundary; perform only targeted decompilation or a
    minimal discriminator if a real divergence remains.
14. Repeat the minimum decisive case if necessary, classify the hypotheses,
    restore exactly, close the result and checkpoint.

## Exit conditions

- Success: humanoid, dog and Gundyr have 60-reference and high-cadence evidence
  sufficient to state that the same correction/infrastructure preserves their
  bounded spatial and temporal behavior, with no unjustified local patch and
  exact cleanup.
- Bounded divergence: one actor has a reproducible cadence-dependent mismatch;
  the first distinct temporal boundary is localized well enough to explain why
  the existing correction does not reach it. A new patch is optional and
  requires independent causal evidence.
- Insufficient information: the minimal panel cannot be matched or measured
  well enough to discriminate generality from divergence; preserve exact
  blockers rather than promoting visual impressions.
- Refuted: deterministic evidence shows the inherited intervention violates a
  safety/restoration invariant or systematically damages non-player behavior;
  disable it and preserve the counterexample.
- Stagnation: after two local fixes fail or two consecutive cycles yield no new
  discriminator, stop patching and open a bounded RCA/replan.
- Budget exhausted: close the fourteen-cycle tranche under
  `.project/WORKFLOW.md`; extend only within this unchanged scope and never to
  conceal stagnation.

## Safety and human interaction

Do not launch DS3, install into its directory or inject/load a new add-on
unexpectedly. First record the existing ReShade/game-directory state, exact
files and a reversible removal path; then announce a short protocol and wait
for Thomas's readiness. Thomas performs movement, aggro, attack baiting and boss
phase control. The add-on/prototype starts inert, modifies no persistent game
data, fails closed on identity/guard errors and restores Native timing, hooks,
debug state and temporary files/settings. Work automatically until a live
maneuver or another genuine human boundary is reached. `PAUSE`, Evidence Store,
checkpoint and clean-transfer governance remain unchanged.

Sequencing note (2026-09-21): Thomas will perform the first functional/gameplay
test independently. I-0012 therefore stops the initial implementation phase at
the validated, uninstalled tool artifact. Actor measurement instrumentation and
an accompanied humanoid/dog/Gundyr procedure are deferred until Thomas decides
they are useful after that first test; this does not change the bounded panel or
the evidence rules if those later cycles proceed.

Closure note (2026-09-21): Thomas completed the independent test and explicitly
accepted I-0012 as a bounded success at cycle 6/14. Hollow, dog and Iudex Gundyr
passed his visual/gameplay check; this remains `OBSERVED`, not an instrumented
proof of the original generality hypothesis. ReShade loading and clean shutdown
are log-corroborated. Exclusive fullscreen is an accepted host limitation. The
new slope/stair hopping observation changes the central question and is routed
to I-0013 rather than expanding this contract.

