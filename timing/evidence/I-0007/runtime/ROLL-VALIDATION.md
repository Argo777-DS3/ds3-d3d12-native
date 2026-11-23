# Same-correction roll validation — motion PASS

Session `20260919T185858Z-pid3804`, RollPatch DLL identity and protocol in
[same-patch protocol](ROLL-SAME-PATCH-PROTOCOL.md). Raw data remains live;
the session is now stopped with exact timing165/1 ->30/0 and debug restoration.
Final raw hashes are in the causal summary.

## Final bounded motion result

Thomas explicitly reported `termine, ca fonctionne`. The unchanged correction
keeps scalar1 in all300 target60 and824 target165 windows. All1,124 commits and
captured decisions reconstruct exactly; no missing index, overflow or rejected
patch. Of69 actual165 comparisons,35 would take the native reduction without
normalization; zero reduce with the same correction used for sprint.

Each trial has one Space impulse and one contiguous announced-roll ID27115
occurrence, neutral controller, and stationary endpoints. To avoid counting
different amounts of preceding forward movement, use the **same independently
observed Space-release edge** through the stationary endpoint, not an animation
ID whose sampled clock/identity fields change at different instants. This is
the response to the same announced impulse; it does not claim exact isolation
of a hidden root-motion channel or exact continuous animation phase.

| Metric | 60 | 165 | Difference |
|---|---:|---:|---:|
| Net distance after release | 4.364995706 | 4.360111240 | -0.111901% |
| Sampled path after release | 4.379167162 | 4.377916025 | -0.028570% |
| Release-to-final-motion wall time | 1.8331826 s | 1.8302812 s | -0.158271% |

Distance per governing delta ratio is1.000525813 (+0.052581%). Space press
durations are0.116818/0.115430s. Preserve release uncertainty: last held vs first
released samples bracket distances as[4.364996,4.430500] and[4.360111,4.384014].
The resulting sampled-boundary ratio range[0.984113,1.004357] stays within2%.
No phase interpolation or fit-selected sample exclusion is needed.

Target165 contains a left click from relative4.001–4.141s,23 updates/46 before-
after samples. Last motion occurs at relative2.352s, and animation is idle0
during the click with position unchanged thereafter. The strict full-window
input flag remains FALSE; this exception is explicitly retained, not silently
turned into a clean-input pass. Unexpected input during movement/non-idle
action would still fail the analyzer. Thomas explains he clicked ChatGPT on
his second screen, while DS3 remained on the main screen; this is consistent
with the captured timing and requires no repeated roll acquisition.

**Accepted: bounded roll motion with identical causal correction.** This
supports a common locomotion cause together with sprint; it does not establish
all roll variants, i-frames, damage behavior, all high-framerate systems or
distribution readiness. D-0012 invulnerability work is explicitly deferred.

Raw causal trace SHA-256:
`646957C5E58F9EA40E103961B47A7E26627AF63736AAA7B88E29C8045B2157B0`.
Reproduce using `analyze_causal_patch.mjs` and `analyze_roll_capture.mjs` against
the stopped session. Analyzer `accepted:false` is deliberate: the technical
owner combines measured gates, qualified input exception, operator report and
restoration in this record; a standalone metrics script does not accept gameplay.

## Earlier qualification notes

D-0012 requires unchanged i-frames as well. These logs contain no direct
invulnerability state or damage-rejection proof. Any forthcoming distance match
is **motion-only**, not full roll acceptance. The actual logical counting and
time window remain unqualified; no numerical i-frame claim is currently supported.

## Target 60 action qualification

Trial1: 301 measured rows over5.016647 accumulated seconds,300 exact commits.
26 actual scalar comparisons, no reduction; scalar1 for all300 windows.
All captured arithmetic, position/history/next-update pairings and registers
reconstruct exactly, no missing broad index, rejection, overflow or extra writer.

One Space impulse, initial/final keys0, neutral controller, initial/final
150ms stationary. Announced single forward roll produced one contiguous
animation-ID27115 occurrence, followed by idle. This qualifies the bounded
action for comparison, not all roll variants or global animation semantics.

Some forward movement precedes roll: Z first at2030; Space held2039..2045,
released2046; ID27115 appears2054..2176; all keys released2071. Total window
distance5.280857 includes preceding movement and cannot alone decide roll
invariance. The moving ID27115 portion has subsequent clock reset/duration
transition; keep raw boundaries and compare like intervals, not arbitrary
exclusions. Clock is sampled in plateaus, so interpolation requires explicit
uncertainty, not a claim of exact per-frame roll phase.

[Causal checks](analysis/20260919T185858Z-pid3804/causal-patch-summary.json),
[action/segment detail](analysis/20260919T185858Z-pid3804/roll-summary.json).
The same DLL was switched live60/1 ->165/1 after this qualification. Trial2
will repeat one forward roll. No roll-specific patch adaptation; no roll
correction claim until comparison and final cleanup. C4 remains globally open.
