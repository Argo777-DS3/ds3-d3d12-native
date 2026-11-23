# Causal distance normalization: local prototype validation

## Current continuation: V3 full held-input sprint

The V2 60/165 measured comparison passes, but Thomas observed the original bug
return after the correction stopped at measure_end. See
[validation and tail diagnosis](PATCH-SPRINT-VALIDATION.md). V2 is stopped with
exact restoration; DS3 PID 3804 remains open. Do not reinject V2.

V3 has distinct DLL/control names (`I0007CausalPatchV3`), extends correction
through the held-input tail and records `post_measure` until release. The same
comparison/commit handler is unchanged, LF-normalized SHA-256
`65E5A0DEC6D9D3E2BC5C514DAFD0077938F243B20E0C75650CF46545FB98D7DA`.
The normalization header and patch sites are unchanged. Build, native tests and
analyzer tests pass. Only capture lifetime and metadata/control names differ.

- DLL: `CA88261ACF9441651EB887A28E570DF91CB94EFADA71F6153C8A30B913BE2766`
- Injector: `EE05EE9DB88CBD11082875E3AD3BAEE83D3F177ECCAE2E8D31A6E8855EA5FEFE`

Next focused test: start V3 target165 in the still-open process after verifying
V2 COMPLETE/restoration. Announce that continued correction through stamina
depletion distinguishes a capture-window artifact from a remaining sprint bug.
F4 then hold Z+Space until stamina empties, then release immediately (do not
exceed ten seconds total). This retains the comparable first three measured
seconds and records the remaining sprint. Wait for trial_end before flush.
Verify all patch/commit checks and tail motion while sprint animation persists;
normal gait change at actual zero stamina is not the half-amplitude defect.
Do not accept the full sprint until the visible tail objection is resolved.
Roll remains next and uses exactly the same correction, without action-specific
gains. The earlier sections below retain V2 preparation/history.

## Scope and mechanism

Start from [qualified feedback](FEEDBACK-MIRROR-165.md), never the final writer
as a causal hypothesis. Isolated sources/binaries: [causal-patch](causal-patch/).
The same three established base hooks select the actual player's qualified
transition. DR0/DR1 observe comparison `005C73D8` / recurrence exit `005C7435`;
DR2 watches the established position commit only as a non-regression sentinel.
All six original debug fields are restored and read back after each window.

At the comparison only, replace saved XMM0 low float with
`raw_test * ((1.0f/60.0f) / previous_broad_delta)`. Preserve upper register lanes.
Require exact opcode, object identity, consecutive broad indexes, current/old
position identity, getter identity and finite positive elapsed time; otherwise
do not apply and stop. No game-file change, source-scalar write, final-writer
change or FPS-specific gain. Recurrence and its existing recovery remain native.
The prototype is active only in armed trial warmup/measurement windows, not a
persistent gameplay patch. One second held-input warmup precedes measurement.

Offline: W4/WX build, native two-execution plus write-watch test, actual XMM0
replacement, time-pair/maths rejection and exact debug restoration pass.
Analyzer math self-test is separate; neither test establishes DS3 behavior.

## Same-process sprint validation

Verify PID, prior COMPLETE/restoration and disabled old probe configs.
Do not inject a second copy of this same DLL into its existing process.
Set `Set-CausalPatchTarget.ps1 -Target 60`, inject matching DLL/injector, require
READY, schema `I-0007-causal-distance-patch-v1`, causal_distance_patch=1.
No restart if the previous distinctly named observer stopped cleanly.

Announce the comparison and decision before asking Thomas to act. At each
cadence: same accepted straight unobstructed route, recover stamina, F4 then
hold AZERTY Z+Space for six seconds, neutral controller/no camera. Record visible
animation/movement anomalies. Keep game open after `terminé`.

After 60: `I0007CausalPatchV2Injector.exe flush <pid>`. Require CONTROL success;
run `analyze_causal_patch.mjs <session> <analysis>` and the unchanged locomotion
analyzer. Compare trial metrics against D-0005 baseline, retaining its start/
facing caveat. Interim files are mutable and restoration remains pending.
If acceptable, announce 165 test, use `target <pid> 165`, require CONTROL success
and override readback, then repeat same sprint with identical DLL and formula.
Switches are rejected during any armed trial/watch. Only 60/165 are accepted.
Each flush writes only newly captured records; retain all target transitions.
The session-info target is the initial target, not both trials' actual cadence.

Acceptance: actual per-trial cadence/delta, distance per time/delta within 2%,
60 unchanged relative to accepted reference, normal animation, valid input and
position, no missing updates, every measured interval's exact commit/history/
next-update match, no failure/overflow and no visible regression. The arithmetic
analyzer alone deliberately never sets gameplay accepted.

## Roll and cleanup

Immediately after sprint passes, qualify roll with the identical causal formula
and patch sites; do not adapt the correction for roll. Roll stimulus/measurement
sentinel still needs preparation (current F4 automaton is sprint-specific).
Do not claim roll validation from sprint data or promise that the present
sprint-only automaton already supports it. No separate roll RCA unless unchanged
patch roll fails. 240/unlimited/distribution remain closed.

Stop with matching injector, require COMPLETE/debug restoration and exact
original timing 30/0, reset with `Reset-CausalPatchConfig.ps1`. Disabling hooks
ends operand replacement; the loaded DLL remains inert until game exit. The
source scalar is native evolving gameplay state, not a saved configuration;
do not overwrite it during cleanup. Preserve complete raw hashes only after stop.

## Preparation-only session and control revision

`20260919T183147Z-pid20840` was stopped before any F4 trial (zero events/hits).
The requested 1200-second deadline exceeded the parser's existing 900-second
limit and silently retained its 30-second default. This was detected in the
session metadata before operator stimulus; timing restored exactly to 30/0.
No correction executed and no gameplay outcome is inferred. Preserve this raw
session as preparation evidence, not a failed locomotion trial.

The corrected templates request 900 seconds. V2 uses a distinct DLL name and
distinct control-event namespace so the stopped first DLL remains inert without
restarting DS3. The causal formula and instrumentation sites are unchanged.
Always check the actual session deadline before inviting the operator to start.

`20260919T183329Z-pid20840` likewise ended before F4 (zero trial events).
READY and the corrected 900-second deadline passed. The idle flush control
acknowledged, but external read attempts returned sharing violations. A native
test reproduces the exact cause: legacy `_wfopen_s(..., wb)` excludes readers;
flushing does not release that file lock. `OpenSharedCaptureLog` now opens with
`_wfsopen(..., _SH_DENYWR)`, permitting read-only concurrent analysis while
still excluding a second writer. Native test proves legacy failure, corrected
read of two successive flushes, writer exclusion and temporary-file cleanup.
This changes log opening only, not the causal patch or observation sites.

V2 was explicitly stopped; timing 60/1 -> 30/0 and debug restoration passed.
Thomas offered a restart and was asked to close DS3. At this checkpoint the
source and native tests are corrected, but the DLL loaded by PID 20840 cannot
be replaced on disk until that process exits. Next: verify exit, rebuild normal
V2 DLL, record new hash, announce launch and have Thomas start the game. Verify
READY, deadline, target and actual concurrent log reads before inviting F4.
Do not reuse the earlier V2 hash as identity for the forthcoming rebuilt DLL.

Thomas subsequently closed DS3 and exit was verified. The normal V2 build and
all native/analyzer tests now pass. Ready for a newly announced launch; no live
patch validation yet. Current SHA-256 identities:

- DLL: `6F15BAA80026379083A2D8461328973DAC629DE88F6142419EB8F3D3C41354BE`
- Injector: `F5AD808791374F1C76D69ECC57FBA036F55A40D665C1F27DC20E3F6A491BC4E6`
