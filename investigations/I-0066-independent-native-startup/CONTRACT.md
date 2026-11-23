# I-0066 - DS3 DX12 Vague 1 (formerly: Independent native D3D12 startup)

Capability: C3. Owner and budget: [status.json](status.json); current checkpoint: [STATE](../../.project/STATE.md).

Status (2026-10-19): the text below is the contract as written at the opening (2026-10-07). The question is answered in
practice (the real game plays on the native backend at the witness zones); no claim or decision declares the exit conditions met: their acceptance and the
wave-1 closure are Thomas's (D-0191). The D-0156 wait for Astra's study716 is over: the study is complete
([ER-ENGINE-D3D12-DEEP-STUDY-716](ER-ENGINE-D3D12-DEEP-STUDY-716.md)) and the connection design restarted from it
([DIRECT-CONNECTION](DIRECT-CONNECTION.md)). Results: [RESULT](RESULT.md).

Title and budget (Thomas, 2026-10-13, in the glare thread): « DS3 DX12 Vague 1 » (the remaster is Vague 2, later), budget 2400
iterations. The id and the directory stay `I-0066-independent-native-startup` (scripts, `evidence/I-0066`, run names `i66-*` and
the session permission rules cite that path; Thomas: « garde I-0066 »).

## Question

Can the real DS3 process bootstrap, load a genuine zone and render it using an independently owned D3D12 backend?

## Scope

Own all GPU GraphicsStep transactions (initial bootstrap, native view system and resources), retain only demonstrated CPU responsibilities, and connect genuine zone resources, instances and camera inputs. Compare with a separate original D3D11 execution. Real Bandai Namco/FromSoftware logos, title and main menu and the game cursor and main-menu music are explicit startup validation obligations (Thomas2026-10-07). Every supported text/menu language is required (Thomas2026-10-08, D-0153); preserve original localization/font selection and cover glyph families rather than calibrating on English. Initial rendering defects are allowed by the current mandate; a clear-only window does not satisfy the milestone.

Thomas2026-10-08 confirms English for current live tests. Keep that locale; the multilingual implementation/coverage obligation remains, without changing live language settings now.

D-0155 requires direct current engine data delivery into one integrated native
backend, reusing existing graphics components and replacing transitional adapters.
D-0156 stops crash-driven mini-workstreams: wait for Astra's deeper ER study716,
verify actual sources, then restart the connection design from zero using that
method and independently evidenced DS3 contracts. See the
[programme mandate](../../.project/NATIVE-D3D12-RESET-MANDATE.md).
Review686 remains preserved historical input, not approval of the restarted design.
## Inputs

Original executable SHA256 EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C. Main baseline b8e88c50. Primary decompilation: D58580, D59D40, D56E90, D59E90, D573B0, D53EE0, B5CD0, D60C0 and 192D650. Targeted Astra High architectural review explicitly authorized by Thomas.

## Hypotheses

- Replacing only the lower device factory is insufficient because bootstrap and GX construction consume D3D11 services immediately.
- Owning both startup transactions can separate native GPU ownership from genuine CPU loading responsibilities.

## Expected evidence

Pinned original code prefixes, actual startup phase and readiness observations, explicit refusal of legacy GPU entry points, native device/presentation receipts, and eventual genuine zone rendering with independent CPU inputs.

## Exit conditions

Success requires real game startup, zone load and native zone rendering. Compilation or native clears alone are insufficient. Park non-discriminating acquisition methods after two failed attempts; diagnose before retrying. Replan at the bounded budget under WORKFLOW.

## Safety and human interaction

Announce each live protocol, use offline play, preserve saves/configuration, and verify restoration hashes. No native module installed in the game folder. On Thomas's explicit cleanup request, ReShade and its dependent timing addon were moved to an external backup and NVIDIA overlay enable flags disabled; future runs must use this new installed baseline.
