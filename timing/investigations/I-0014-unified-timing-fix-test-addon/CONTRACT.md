# I-0014 - Unified Timing Fix Test Add-on

Capability: C4

## Question

Can one practical ReShade test add-on host the already accepted LTF and TRTF
implementations as independently controlled, independently observable and
fully reversible corrections, while retaining the validated cadence controls
and without changing either correction's causal formula or validation scope?

## Scope

Included:

- Start from the clean canonical `main` checkpoint after I-0013.
- Reuse I-0012's ReShade 6.8.0/API-20 host, cadence control, presets, measured
  FPS and native-restoration behavior.
- Reuse LTF exactly as accepted by I-0007 and carried unchanged through
  I-0010/I-0012: `raw * ((1/60) / previous_broad_delta)` with its exact-build,
  identity, opcode, temporal-pairing and debug-state guards.
- Reuse TRTF exactly as accepted by I-0013: additive
  `state + (native_state - state) * dt/dt60`, multiplicative
  `state * retention^(dt/dt60)`, native-reference no-write passthrough, reset
  and clamp passthrough, current-player identity and exact-build guards.
- Expose independent `LTF - Locomotion Timing Fix` and
  `TRTF - Terrain Response Timing Fix` toggles. Both start OFF.
- Keep the controls usable in both windowed and exclusive-fullscreen modes.
  Because I-0012 observed an unexplained exclusive-fullscreen control failure,
  provide a renderer-lifecycle-independent keyboard fallback in addition to
  the ReShade settings surface; do not alter either correction to achieve it.
- Verify offline each correction's OFF/ON/OFF/ON/restoration state sequence,
  simultaneous activation, state ownership isolation, failure behavior,
  hook removal, timing restoration, and add-on packaging.
- Produce a hash-guarded one-file install/remove manager and a concise manual
  test guide for Thomas's first 60/240 distance comparison.

Excluded:

- Any DS3 launch or live-game validation during the offline construction
  phase. Exclusive-fullscreen functionality remains an explicit item for
  Thomas's subsequent manual acceptance test.
- Distance probes, automated routes, dedicated acquisition, heavy experimental
  protocol or new locomotion/terrain RCA.
- Any change to the accepted LTF or TRTF equations, intervention sites,
  build/identity guards or causal claims.
- Per-FPS constants, empirical compensation, visual smoothing, actor tables,
  terrain thresholds, product UI polish, permanent game-file changes,
  distribution claims, online use or whole-C4 conclusions.

## Inputs

- Canonical claims K-0037-K-0039, K-0054-K-0056 and K-0062-K-0064;
  decisions D-0020-D-0027; `.project/TERMINOLOGY.md`.
- I-0012 host-independent cadence/LTF core and ReShade host under
  `evidence/I-0012/prototype/`.
- I-0013 producer time-fix source, validation and accepted live evidence under
  `evidence/I-0013/producer-time-fix-prototype/` and
  `evidence/I-0013/LIVE-TIME-FIX-VALIDATION.md`.
- Pinned MinHook v1.3.4 and ReShade v6.8.0/API 20 sources already used by the
  canonical prototypes; Visual C++ x64 toolchain and repository validators.

## Hypotheses

- H1: the two accepted corrections can coexist behind one ReShade control host
  while retaining separate state, commands, guards, counters and restoration.
- H2: shared hosting creates an ownership or lifecycle conflict that prevents
  safe independent control without changing a canonical implementation.

## Expected evidence

- Deterministic tests exercise LTF and TRTF equations independently, including
  native-60 passthrough and TRTF additive/decay/reset/clamp branches.
- A pure control/lifecycle model exercises initial OFF/OFF state, independent
  OFF/ON/OFF/ON sequences, both-ON coexistence, native restoration and stop.
- Source and deterministic input tests prove that the fullscreen fallback is
  handled by the host-independent runtime thread and does not depend on a
  ReShade swap-chain callback surviving runtime recreation.
- Source/architecture validators prove distinct state, request paths,
  telemetry, hook identities and allowed writes for LTF versus TRTF.
- Strict x64 compilation, pinned dependency/build identity checks, PE import
  inspection and repeat restoration stress complete without errors.
- Hashes and provenance are recorded in `evidence/I-0014/manifest.json` and
  generated validation records.

## Exit conditions

- Success: a compiled, offline-verified `.addon64` exposes independent LTF and
  TRTF toggles plus cadence controls; starts inert; passes individual and
  simultaneous lifecycle/isolation tests; and has a verified native/stop and
  uninstall path. Its overlay and keyboard fallback cover both windowed and
  exclusive-fullscreen operation structurally; the result is checkpointed for
  Thomas's manual confirmation in both display modes.
- Refuted: safe shared hosting requires changing either accepted formula/site,
  collapsing state ownership, or losing exact restoration.
- Insufficient information: required canonical source, exact-build identity or
  pinned toolchain dependency cannot be recovered well enough to establish
  implementation identity and safety offline.
- Stagnation: no new discriminating evidence after two consecutive bounded
  implementation/validation cycles following an RCA-style architecture audit.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`; renew a bounded budget automatically within unchanged authorized scope and existing protected boundaries.

## Safety and human interaction

This phase performs no live-game launch. The add-on must start with native
cadence and both corrections OFF, make no permanent game-file change, fail
closed on identity/guard mismatch, disable both corrections before hook
removal, restore debug registers and the exact saved timing pair, and uninstall
only its own hash-matched add-on file. Thomas performs the first in-game
distance test after the offline checkpoint; no conclusion about distance is
permitted before his report.

