# I-0011 - Runtime FPS Control & Overlay

Capability: C4

Canonical correction name: **LTF — Locomotion Timing Fix**. The correction
control specified here is LTF only; TRTF did not yet exist and remains a
separate internal correction.

## Question

Can the exact causal LTF validated by I-0009/I-0010 be turned
into a first usable remaster feature: one compact HOME-toggle in-game overlay
that controls the runtime FPS limit, exposes real Unlimited mode, switches the
unchanged LTF ON/OFF, and restores every modified state
cleanly without restart, reinjection or cadence-specific fix profiles?

## Workstream type

Implementation. Reuse established causal knowledge and prototype infrastructure.
Do not reopen closed RCA or begin theoretical research unless a demonstrated
technical obstacle prevents this implementation.

## Required user surface

- Toggle a compact, readable overlay with `HOME`; support mouse interaction
  when the chosen environment permits it.
- Display `FPS Limit` with a practical bounded slider from 30 through 360 FPS.
- Provide presets: 30, 60, 90, 120, 144, 165, 240 and Unlimited.
- Numeric presets and the slider must use one runtime cadence controller.
- Unlimited must disable the remaster's runtime limiter when technically
  possible; it must not be simulated by an arbitrarily high numeric cap.
- Display target FPS as a number or `Unlimited`, reliably measured actual FPS
  when available, locomotion fix `ON/OFF`, and a clear general state such as
  `ACTIVE`, `NATIVE` or `STANDBY`.
- Keep ordinary debug detail in logs, not in the normal user interface.
- When open, avoid unintended gameplay input where cleanly possible. At minimum
  preserve the validated windowed mode and reasonable HOME toggle, mouse,
  Alt-Tab, resize and focus behavior.

## Locomotion contract

- Reuse exactly the causally validated I-0009/I-0010 correction:
  `raw * ((1/60) / previous_broad_delta)` at the accepted comparison boundary.
- Preserve its opcode, identity, consecutive-index, position-pair, finite-delta,
  exact-build and restoration guards.
- Do not recalibrate for 60, 165, 240 or any other cadence; do not add multiplier
  tables, per-cadence profiles, downstream compensation or action-specific gain.
- FPS controls select cadence only. They must never become the correction
  mechanism.
- `LTF — Locomotion Timing Fix ON` enables LTF; `OFF` restores
  exact native DS3 comparison behavior. ON/OFF must switch during the session at
  the smallest demonstrably safe boundary and never expose an ambiguous state.

## Runtime cadence contract

- The same injected DLL/session must support live transitions among numeric
  limits and Unlimited, including sequences such as
  `240 -> Unlimited -> 165 -> Unlimited`.
- No transition may require game restart, reinjection, a new DLL, a new hook or
  a framerate-specific correction profile.
- If the existing central timing pair can represent a genuine disabled limiter,
  use that native/runtime state for Unlimited. If it cannot, preserve the
  evidence, present the real architectural limit, and do not fake Unlimited.
- Measured FPS and selected target must remain distinct. Natural hardware,
  engine, display and external limits are not correction failures.

## Scope

Included:

- Derive from clean integrated `main` after I-0010 final checkpoint `fc80ce1`.
- Reuse I-0009/I-0010 controller, timing, locomotion guard, measured-cadence,
  cleanup and exact-build infrastructure where appropriate.
- Select the smallest maintainable in-game overlay integration; reuse a suitable
  existing project dependency if present, otherwise add a lightweight one with
  recorded provenance.
- Minimize hooks, writes, global state, dependencies and cadence-specific code.
- Build an extensible UI foundation, but prioritize functionality, robustness,
  reversibility and simple ergonomics over visual polish.
- Preserve explicit standby/active/native/stop state and bounded technical logs.

Excluded:

- A renderer-wide project, extensive UI polish, unrelated remaster features,
  installer/distribution work, online play or persistent game-file changes.
- Reopening I-0007 through I-0010 evidence, new gameplay semantics, roll
  i-frames/damage, or declaring attack/jump corrected without separate evidence.
- Hiding a real Unlimited/engine limit with ad hoc compensation or a fake cap.
- Reintroducing the retired Discussion Interface Agent or a replacement.

## Inputs

- Integrated baseline: `main` containing I-0009/I-0010 through `fc80ce1` plus
  this clean transfer checkpoint.
- Canonical claims K-0037 through K-0039, K-0054 through K-0056 and limits in
  the I-0009/I-0010 results.
- Exact build: Steam AppID 374320 / BuildID 10167187 / executable 1.15.2.0,
  SHA-256 `EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C`.
- Existing I-0010 source/controller, exact derivation audit, native tests,
  reversible timing override, measured cadence and cleanup evidence.

## Hypotheses

- H1: one lightweight overlay/controller can switch the existing timing state
  and unchanged correction safely in real time, including a genuine limiter-off
  Unlimited state, while preserving exact cleanup.
- H2: numeric live control and correction ON/OFF are implementable, but the
  current timing architecture cannot represent genuine Unlimited; document the
  blocker rather than substituting a fake high cap.
- H3: overlay/input/presentation integration requires a narrowly bounded new
  renderer hook or dependency; add only the minimum after proving necessity.
- H4: live state transitions violate a safety/restoration invariant; stop local
  patching after two failed fixes and open an RCA only for that demonstrated
  blocker.

## Offline validation gate

Before any DS3 launch or injection:

- strict clean build, exact executable/signature/hash validation and inert
  startup;
- deterministic tests for slider bounds, every preset, numeric/Unlimited state
  transitions, the single cadence-control path and measured/target display;
- exact identity tests showing the causal formula/guards unchanged;
- deterministic locomotion ON/OFF/native restoration and stop cleanup tests;
- overlay HOME visibility state, control interaction and input-capture logic
  tested outside the game where practical;
- no RCA-only instrumentation active in the release prototype;
- a short live protocol announced to Thomas before launch.

## Live validation gate

In one or more announced offline exact-build sessions controlled by Thomas:

1. Open/close the overlay with HOME and exercise mouse controls if available.
2. With correction ON, switch live through 60, 120, 165, 240 and Unlimited,
   returning from Unlimited to a numeric limit without restart/reinjection.
3. At minimum compare sprint and forward-roll travel for coherent behavior at
   the selected cadences. Unlimited requires stability under natural cadence
   variation, not a specific measured FPS.
4. With correction OFF, confirm native behavior returns; switch ON again and
   confirm corrected behavior returns at the safe boundary.
5. Verify target versus actual FPS display, general UI state, no ambiguous
   transition, acceptable windowed focus/Alt-Tab behavior, and no unintended
   gameplay input attributable to overlay navigation where avoidable.
6. Stop and prove exact timing, instruction, hook, debug/temporary-state and
   last-selected-FPS restoration. Preserve raw evidence and operator reports.

Attack and jump may be observed but remain unqualified without a separate
controlled validation. If Unlimited exposes a real engine limit, record it and
open a new RCA only if it prevents correct operation.

## Exit conditions

- Success: one reversible exact-build prototype provides the HOME overlay,
  30–360 slider, required presets, genuine Unlimited when the architecture
  permits, measured/target FPS, locomotion ON/OFF and clear state; all controls
  work live through the required scenario with the same cadence-independent
  correction, coherent sprint/roll behavior and exact cleanup.
- Partial/insufficient: the architecture demonstrably blocks genuine Unlimited
  or reliable overlay interaction while safe numeric control/correction works;
  preserve the exact discriminator and do not mislabel the limitation.
- Refuted: a deterministic safety or restoration result shows the proposed
  runtime integration cannot meet the reversible contract; remove/disable it
  and preserve the evidence.
- Stagnation: after two local fixes fail for the same unexplained cause, stop
  patching and open an RCA boundary.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`;
  renew only within this unchanged authorized implementation scope.

## Initial twelve-cycle plan

1. Recover clean integrated main and inventory reusable I-0010/runtime-renderer
   infrastructure without reopening accepted RCA.
2. Define one runtime cadence state machine, including genuine Unlimited
   semantics and exact restoration.
3. Separate the unchanged locomotion ON/OFF control from cadence selection and
   prove formula/guard identity.
4. Select and bind the smallest overlay/render/input integration for windowed
   DS3, with dependency provenance if added.
5. Implement compact HOME-toggle UI, visible state and measured FPS.
6. Implement the 30–360 slider and all numeric presets through one controller.
7. Implement Unlimited through the same controller as real limiter disable when
   supported, plus numeric re-entry.
8. Implement safe locomotion ON/OFF and unambiguous transition feedback.
9. Complete deterministic UI/state/cadence/formula/restoration tests and remove
   RCA-only instrumentation.
10. Pass strict build, exact-build/signature/config and release-artifact audits;
    prepare the live protocol.
11. Announce and execute the Thomas-operated live functional matrix; restore
    immediately on any stop condition.
12. Analyze evidence, fix only demonstrated in-scope defects, repeat the minimum
    discriminating validation if necessary, then close and checkpoint.

## Safety and human interaction

No game launch or injection before offline gates pass and the exact short
protocol is announced. Thomas controls all gameplay inputs and visible
assessment. The prototype starts inert, performs no persistent game-file write,
supports explicit stop, and must fail closed on identity/guard/restoration
errors. Preserve PAUSE, canonical checkpoints, governance, Evidence Stores and
the retirement of the Discussion Interface Agent with no replacement.
