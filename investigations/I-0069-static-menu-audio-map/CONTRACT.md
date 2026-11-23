# I-0069 - Static map of the menu audio path (music, cursor and confirm sounds, volumes) and its independence from D3D11

Capability: C3

## Question

How do the menus of the PC build make sound (engine, banks, event naming, triggers for cursor, decide and open sounds, title and Load Game music, volume
settings), what part of that depends on the D3D11 renderer, and is anything deferred or absent in the native build?

## Scope

Included:

- the FMOD wrapper and its configuration, the banks and event names that menus use, the MenuMan sound object and its request functions;
- the settings bytes that drive volumes and the per-frame sound update;
- the comparison with what `native/frame/game_bootstrap.cpp` and the earlier live logs show.

Excluded:

- any live run (static, parallel to I-0066); character, map and combat sounds beyond naming the bank families;
- (movie audio and video were added in iteration 2, see BINK-MOVIES-NATIVE.md; engine cutscenes in iteration 3, see REMO-CUTSCENES-NATIVE.md; fade plates in iteration 4, see FADE-PLATES-NATIVE.md; Scaleform filter passes in iteration 5, see SCALEFORM-FILTER-PASSES.md; the ending credits, a save-free test route and the native D3D12 fade design in iteration 6, see ENDING-CREDITS-NATIVE.md)

## Inputs

- Canonical claims: MENUS-MAP sections 3 and 28 (I-0067); I-0066 witness 1175 (menu sound heard natively).
- Artifacts and sources: DarkSoulsIII.exe 1.15.2.0 Ghidra mirror, Data1 `sound/*.fev` read offline, installed `sound/` folder, I-0066 live logs.
- Tools: the I-0067 query helper, `Inventory-Archives.ps1`.

## Hypotheses

- H1: all menu audio is engine-side FMOD and does not touch the graphics device.
- H2: the native build keeps the genuine audio initialization, so only the update loop and the menu logic matter.

## Expected evidence

The event names in the `.fev` files against the requested ids; the configuration text; the callers of the request functions; the native source around the audio service.

## Exit conditions

- Success: a checklist with static witnesses and live rows, written in `MENU-AUDIO-NATIVE.md`.
- Refuted: a menu sound path that needs a graphics object.
- Insufficient information: the volume mapping or the Load Game music cannot be resolved statically (then live rows).
- Stagnation: no new discriminating evidence after 2 cycles.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`; renew a bounded budget automatically within unchanged authorized scope and existing protected boundaries.
