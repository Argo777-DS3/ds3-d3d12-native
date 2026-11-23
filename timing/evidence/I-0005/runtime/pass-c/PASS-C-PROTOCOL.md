# I-0005 runtime Pass C protocol — semantic invariants

## Purpose

Pass C tests a bounded set of high-value gameplay sentinels at 60, 90, 120, and 165 FPS, then at 240 and 280 only if the bounded conditions remain stable. It does not treat a successful unlock or visual smoothness as gameplay correctness.

## Safety and intervention

- Exact C1 executable only: SHA-256 `EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C`.
- Offline only.
- The only cadence intervention is the audited central `+0x354/+0x358` pair.
- Writes are memory-only, logged before/after, and restored exactly before hooks are disabled.
- No game-directory file is written.
- No fixed `1/30`, `1/60`, or `60` constant is replaced.
- Raw capture files are immutable after collection.

## Two-stage method

1. **Discovery/control at 60:** validate the player and animation pointer chains against movement and animation transitions, quantify probe overhead, and reject any field whose behavior does not match its claimed semantic role.
2. **Matched comparison:** repeat the same short marker sequence at 60, 90, 120, and 165. Continue to 240 and 280 only if the prior bounded conditions are stable and the measured cadence is actually reached.

The marker sequence covers idle, continuous sprint/movement, roll, one representative animation, collision against a fixed obstacle, camera rotation, and one movement/physics integration sequence. Each marker has QPC time, update index, accumulated delta, position/facing, animation state, and observed input state.

## Required measurements

- Present and broad-update cadence;
- `context+0x08` distribution and accumulated delta;
- selector mode and I-0004 pacing state;
- position/displacement and derived speed during marked movement;
- position plateau/contact proxy during the matched obstacle sequence;
- animation identifier, clock, duration, transition times, and play-speed field;
- keyboard/XInput state and marker endpoints;
- active bounded consumers retained from Pass B;
- probe overhead, file volume, and achieved versus requested cadence.

Camera position/rotation requires a separately revalidated current-build field before it can be promoted beyond input/player-facing correlation. Physical display scan-out is not measured, especially under G-Sync.

## Epistemic gate

External tables and unlockers are lead generation only. A semantic field is usable in the comparison only after current-build runtime behavior supports its identity. Divergence triggers root-cause tracing before any consumer correction. Absence of divergence in these sentinels does not prove universal framerate independence.
