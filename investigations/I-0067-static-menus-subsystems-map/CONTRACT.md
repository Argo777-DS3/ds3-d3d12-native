# I-0067 - Static map of menus, save loading, menu sound, transitions, effects and phantoms

Capability: C3

Parallel to I-0066 (Thomas, 2026-10-10): a second workstream that serves the native
renderer pilot. Static decompilation only; the pilot consumes the results through the
project coordinator.

## Question

For each engine subsystem the native D3D12 backend will reach next (in-game menu, title
and Load Game screens, menu music and sound effects, loading screens and transitions,
effects, phantoms), which engine mechanisms, classes and functions own it, what renderer
work does each mechanism request, and which of them explain the native defects already
observed (in-game menu does not open natively; Native1116 fault at 57F8A3 on Load Game)?

## Scope

Included:

- Static Ghidra reading of the canonical database (read-only, no analysis, nothing saved),
  RTTI class/vtable inventory, call chains, globals and data tables.
- Function maps per mechanism (D-0161: by engine mechanism/family, not by object), with
  the renderer-side requests each mechanism makes (offscreen targets, model renders,
  movies, fades, effect instances).
- Reuse first: existing claims, `decompiled/`, I-0015/I-0017 (steps, sound), I-0055/I-0062
  (interface and menus under the host), I-0066 (UI ingress, startup services, Reference890).

Excluded:

- Launching or driving the game; any live test (the I-0066 pilot owns live runs).
- Native code, the I-0066 branch and its files.
- A new heavy Ghidra project or database analysis/saves.
- Exhaustive decompilation; ER offsets (ER Decompilation is read-only and only an
  architectural witness).

## Inputs

- Canonical claims: K-0068/K-0069 (timeline groups, InGameMenuStep), K-0088/K-0089/K-0103
  (sound steps and services), K-0106 (CSFakeLoadingScreenImp), K-0441 (menus under the host).
- Live observations to explain: Native1113 (in-game menu does not open natively),
  Native1116 (Load Game fault 57F8A3, rcx 0x4E0), I-0066 LEGACY-ROOT-INVENTORY-903 on the
  I-0066 branch.
- Tools: `Invoke-Map.ps1` + `I0067Map.java` (queries), `I0067Rtti.java` (RTTI inventory),
  Ghidra 12.1.3 on `game/ghidra/DS3_1_15_2_10167187_I0002_FINAL` of the main checkout.

## Hypotheses

- H1: each target subsystem is owned by a small number of engine mechanisms that can be
  named from RTTI and reached by call chains from their menu/step owners.
- H2: the native menu defects come from renderer-side objects the menu mechanisms expect
  (offscreen model renders, GX textures) rather than from menu logic itself.

## Expected evidence

Decompiled functions and data with exact addresses, RTTI class names and vtables, caller
trees; for each native defect, the exact null/absent object and its owner/lifetime, with a
discriminating runtime check the pilot can run. Every claim carries its static source.

## Exit conditions

- Success: a documented mechanism map for the six subsystems with the renderer requests
  each one makes, and an explained static owner for both observed native menu defects.
- Refuted: H2 false for a defect (the cause is menu logic, not renderer objects): record it.
- Insufficient information: static reading cannot decide (dynamic dispatch, data-driven);
  record the exact runtime check that would decide it, for the pilot.
- Stagnation: no new mechanism or owner after 3 cycles on one subsystem: move on.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`; renew a bounded budget automatically within unchanged authorized scope and existing protected boundaries.

## Safety and human interaction

No live launch, no game-folder change, no native code. Static reading only; avoid long
Ghidra sessions during a live test announced by the I-0066 pilot. Fable is authorized by
Thomas for complex problems in this workstream; Haiku for bounded inventories.
