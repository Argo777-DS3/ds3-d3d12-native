# I-0112 - Le déblocage du framerate : ce qui dépend des 60 images/s

Capability: C3. Study thread (read-only on the code), opened 2026-10-30 at Thomas's choice (08:26, D-0240): the game logic is
unlocked by a variable step (method B), after a study of everything it affects.

## Question

Which parts of DS3's game logic change behaviour when the logic runs above 60 frames per second (movement, physics, animation,
camera, effects, AI, timers, interface), how each depends on the frame (a fixed step, a per-frame scale or recurrence without
the delta time, a clamp), and what a native method for each would be; what of the timing project (LTF, TRTF, I-0005 to I-0014)
holds as a lead (D-0240: studied, not copied).

## Scope

Included: the timing project's records (`timing/`); static decompilation of DS3 1.15.2 (GhidraMCP, headless on the mirror when
needed, Thomas 2026-10-30 05:14); a census of the frame-rate constants in the code and a reading of each use; the engine's delta
time distribution; public knowledge on DS3 and its engine family above 60 frames/s (labelled GENERAL); a live test plan for later.

Excluded: any code in `native/`, any build, any launch of the game; the cadence itself (MODERN-CADENCE-STUDY, D-0219, D-0221) and
Reflex (D-0240), except where they meet the logic's rate.

## Inputs

- `timing/knowledge/CLAIMS.md` (K-0010 to K-0064), `timing/investigations/I-0005` to `I-0014`.
- `investigations/I-0097-stutter/CADENCE-STUDY.md` (A.4, A.7), MODERN-CADENCE-STUDY (branch `claude/zen-mendel-263329`).
- Tools: `tools/I0112Const.java` (the constant census), GhidraMCP, the `ghidra-research` subagent (Sonnet), its classes re-read
  where a conclusion depends on them.

## Exit conditions

- Success: a table of the affected mechanisms with their dependency, evidence and label, the systems not reached named, and a
  proposed method and test plan for the unlock workstream. The target (Thomas, 2026-10-30 08:38, D-0240): a logic correct at
  any delta time, « Unlimited » included, as modern games are; not a fix tuned for a few rates. 120 frames/s is Thomas's own
  setting; the plan tests a range (for example 30, 60, 120, 240 and unlimited).
- Already done (I-0088 P0, K-0470, 2026-10-22, measurement only): both limiters lifted natively by `original-unlock.flag`
  (`15_cpu_service_and_input.inc:357`): DS3's requested rate 360 with its enable byte (`+0x354`/`+0x358`) and presents at once on
  a tearing chain; m45 125 frames/s, the Firelink walk 157. The logic is not corrected there, and 360 is a high target, not a
  limiter switched off (none found by the timing project, I-0011): how high DS3 accepts the request is a question for this study.
- Insufficient information: what only a live 60/120 comparison can settle, listed with the measurement it needs.

## Safety and human interaction

No live run, no build. A live 60/120 comparison is proposed for later, announced under WORKFLOW « Live game rule ».
