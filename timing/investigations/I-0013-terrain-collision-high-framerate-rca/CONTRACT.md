# I-0013 — Terrain Hopping/Jitter High-Framerate RCA

Capability: C4

Canonical correction names: the inherited I-0007 correction is **LTF —
Locomotion Timing Fix**; the distinct correction derived by this RCA is **TRTF
— Terrain Response Timing Fix**. Their operations, sites, guards and proof
chains must remain separate even if a later user control activates both.

## Question

When the player traverses the same reproducible slope/stair route, is the
visible vertical hopping/jitter caused by native high-frequency behavior,
introduced by LTF, or already reduced by LTF; and what is the first measured boundary that distinguishes those
outcomes without assuming collision ownership?

## Workstream type and priority

This is a root-cause investigation opened under D-0023. The first live evidence
must be the ordered same-route comparison, not a speculative collision patch.
Measure the smallest sufficient player trajectory first; decompile or instrument
another subsystem only after the matrix identifies a discriminating boundary.

## Included scope

- Reuse only the validated I-0012 ReShade/API-20 host, its host-independent
  cadence/control core, and the unchanged I-0010 LTF guard/restoration
  implementation.
- Operate in standard windowed mode; exclusive fullscreen is outside the
  validated I-0012 host condition.
- Add the smallest bounded capture needed to record player position and timing
  over a reproducible slope/stair traversal without changing game behavior.
- Run the first live matrix in exactly this order on the same route:
  1. `60 FPS, correction OFF` — native reference;
  2. `240 FPS request, correction OFF`;
  3. `240 FPS request, correction ON`;
  4. `60 FPS, correction ON` only when its incremental cost is low.
- Record requested cadence, measured cadence, correction state, exact route and
  start/end markers, per-update 3D position, vertical deltas, horizontal
  progress, elapsed time, validity/drop counts and exact restoration.
- Use the matrix as the first discriminator among independent native behavior,
  correction-introduced behavior, and behavior already improved by correction.
- After that discriminator only, localize the first relevant producer/consumer
  boundary with targeted decompilation or read-only observation if necessary.
- Treat a general mathematical invariant as a mandatory acceptance condition
  for any eventual fix: use the actual elapsed time or a justified
  time-continuous recurrence, preserve native 60 behavior and vary continuously
  across intermediate/high rates.

## Excluded scope

- Assuming that collision, physics, animation, root motion or the accepted
  comparison site owns the defect before evidence distinguishes them.
- Any new correction, collision intervention, cadence-specific gain table or
  action/terrain-specific patch before the matrix establishes causal direction.
- Any hand-tuned vertical offset, 240-specific multiplier, terrain threshold or
  post-hoc visual clamp, even if it appears to hide the test-route symptom.
- Repeating I-0010/I-0012 acquisitions, changing their accepted formula or
  guards, or replacing their validated host/injection infrastructure.
- Exclusive fullscreen, Genuine Unlimited, online behavior, roll i-frames,
  packaging/distribution, exhaustive terrain coverage or a whole-C4 claim.
- Launching DS3, installing an add-on or changing the game directory without an
  advance short protocol and Thomas's intervention.

## Inputs

- Clean integrated baseline: `main` checkpoint `4f7734c`, which contains the
  completed I-0012 work despite the pre-transfer wording formerly retained in
  `.project/STATE.md`.
- Decision D-0023 and the completed I-0012 result/independent-test evidence.
- Exact game: Steam AppID 374320 / BuildID 10167187 /
  `DarkSoulsIII.exe` 1.15.2.0, SHA-256
  `EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C`.
- I-0012's ReShade-free cadence/control core and API-20 windowed settings host;
  its add-on is currently absent from the game directory.
- I-0010's unchanged LTF
  `raw * ((1/60) / previous_broad_delta)` and exact opcode, player/owner,
  consecutive-index, transform/position, finite-delta, build and restoration
  guards.
- Canonical claims K-0037–K-0039 and K-0054–K-0056, qualified by their explicit
  exclusions for terrain, actual 240 and whole-C4 behavior.

## Hypotheses and first discriminator

- H1 — Native high-frequency defect: `240 OFF` shows materially greater
  route-aligned vertical oscillation than `60 OFF`; `240 ON` does not introduce
  it and may leave it unchanged or reduce it.
- H2 — Correction side effect: `240 OFF` remains comparable to `60 OFF`, while
  `240 ON` reproducibly introduces or materially increases the oscillation. A
  cheap `60 ON` control distinguishes a general intervention side effect from a
  high-frequency interaction.
- H3 — Covered branch: `240 OFF` diverges from `60 OFF`, while `240 ON`
  reproducibly moves the same-route vertical trace toward the native reference.
- H4 — Insufficient/mixed: route variance, measured-cadence shortfall, capture
  invalidity or inconsistent ON/OFF results prevent causal classification.

These outcomes classify only LTF's relationship to the observation.
They do not by themselves identify collision or any other subsystem as owner.

## Expected evidence

- One provenance-bound capture per retained matrix cell, in the required order,
  with exact build, selected/measured cadence, correction state, route markers,
  raw timestamped position samples and cleanup state.
- A deterministic offline analyzer that aligns each trial by route progress and
  reports raw vertical excursion, signed/absolute vertical delta, local
  high-frequency residual/oscillation measures, sample cadence and exclusions.
- Both raw traces and derived summaries. Visual/operator notes remain
  `OBSERVED`; a causal outcome requires the measured matrix to reproduce it.
- The capture tool must be observation-only except for already accepted cadence
  and correction controls, start inert, bound its memory/output, fail closed on
  identity/validity errors and restore the inherited runtime state exactly.

## Initial eight-cycle plan

1. Reconcile clean `main`, open I-0013 and checkpoint its contract/control-plane
   state without live execution.
2. Audit the validated I-0012/I-0010 surfaces and specify the minimum trajectory
   record, bounded storage and route-alignment analysis.
3. Implement the observation-only capture in the I-0012 windowed host without
   changing the accepted timing/correction core.
4. Add deterministic synthetic traces for smooth grade, stair steps, cadence
   variation, repeated vertical impulses, invalid samples and buffer limits.
5. Build and pass exact-build, derivation identity, restoration, schema and
   analyzer tests; prepare a short operator protocol.
6. After advance announcement and Thomas's intervention, acquire the ordered
   `60 OFF`, `240 OFF`, `240 ON`, and cheap `60 ON` matrix on one route.
7. Analyze and, only if needed, repeat the minimum ambiguous cell or add one
   read-only boundary discriminator.
8. Classify H1/H2/H3/H4, preserve evidence, restore exactly and checkpoint the
   result without proposing an unproved collision fix.

## Exit conditions

- Success: the valid same-route matrix reproducibly distinguishes H1, H2 or H3
  and records the correction relationship plus the first justified next
  boundary, with exact cleanup.
- Refuted: the reported observation does not reproduce under the matrix's valid
  route/cadence conditions; preserve the negative result and its detection
  threshold without inventing a cause.
- Insufficient information: H4 remains after one bounded repeat or the route,
  actual cadence or position trace cannot support the comparison.
- Stagnation: two consecutive meaningful cycles yield no new discriminator, or
  two local capture/analysis fixes fail; stop patching and replan from the raw
  observations.
- Budget exhausted: close the eight-cycle tranche under `.project/WORKFLOW.md`;
  extend only within unchanged authorized scope and never to conceal stagnation.

## Cycles 11–12 extension after the crash-safe discriminator

Cycle 10 closes the second live discriminator with exact cleanup. The safe
broad-update session collapses the sampled source/final-root split but neither
reproduces nor refutes the hopping, and its cache gap remains phaseful under a
route-length mismatch. Under D-0014, two same-scope offline cycles are added:

11. reconstruct the cache matrix production around `005C8100` and its three
    writers, retaining protected operations as unknown unless evidence resolves
    them, and identify any explicit contact/ground-response state or recurrence;
12. test any resulting equation against existing 60/240 evidence and decide
    whether a smaller safe observation is justified or the investigation is
    insufficient. No live run or patch follows automatically.

## Cycles 13–15 extension after the vertical recurrence

Cycle 12 identifies an explicit vertical cache override and two exact
cadence-dependent recurrences. Under D-0014, three unchanged-scope cycles are
added:

13. extend and qualify the existing one-hook observer to record only the five
    recurrence fields, while proving the native-60-preserving continuous
    equations offline;
14. acquire one same-process 60/240 route pair and test activation, lane-7
    binding, recurrence fit and operator-visible symptom alignment;
15. only if cycle 14 establishes causality, implement the smallest reversible
    equation substitution and validate native-60 preservation plus high-rate
    terrain behavior. Otherwise close insufficient without a patch.

Cycle 14 refutes the candidate recurrence but yields a new positive
discriminator: visible hopping at 239.770 aligns with strongly amplified
opposing cache motion while sampled source/root stays monotone. Cycle 15
therefore closes this tranche without a patch by implementing and qualifying
the smallest pre-protected boundary observer at `009D97B0`.

## Cycles 16–18 extension after cache localization

Under D-0014 and the unchanged RCA scope, three cycles continue from the
cycle-15 checkpoint:

16. acquire one same-process target-60/target-240 route pair with the qualified
    replacement one-hook observer and exact restoration;
17. decide whether opposing motion is already present in the returned base
    transform or first appears in the later protected/local/cache path, then
    reconstruct only the selected side of that boundary;
18. identify and test an exact time-based recurrence if the selected path
    exposes one, or checkpoint insufficient information with the smallest next
    discriminator. No patch is allowed without a causal operation and a
    general native-60-preserving mathematical invariant.

## Safety and human interaction

## Cycles 19–21 extension after exact aggregator localization

Cycle 18 proves the final sampled equation and clears later cache transport,
but the carried scalar is a sum of two unresolved dynamic contributions. Under
D-0014 and the unchanged RCA scope, three cycles continue without a patch:

19. capture only the concrete virtual targets and bounded producer provenance
    around `005C75F0`, then decompile the resolved implementations;
20. measure the smallest individual contribution necessary to identify the
    cadence-dependent operation and derive its exact time-continuous invariant;
21. validate native-60 identity and continuous intermediate/high-rate behavior
    offline, then decide whether a reversible live substitution is justified.

No semantic collision label, low-pass filter, clamp, FPS table or empirical
terrain compensation is authorized. If the producer law remains unresolved,
checkpoint the exact boundary rather than inventing an equation.

Do not launch Dark Souls III, install or load the add-on, inject code, or change
the game directory unexpectedly. Complete offline validation first, announce a
short exact protocol, and wait for Thomas before any live step. Thomas controls
the character and repeats the route. The prototype starts inert, is limited to
offline/single-player use, uses standard windowed mode, modifies no persistent
game data, and must restore Native timing, correction state, hooks and temporary
artifacts exactly. No installation or live action is part of the opening
checkpoint.
