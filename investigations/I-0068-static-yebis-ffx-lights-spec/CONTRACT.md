# I-0068 - Static specification of the native YEBIS post chain and FFX effect lights

Capability: C3

Second static workstream parallel to I-0066 (Thomas, 2026-10-10, through the project
coordinator), of the same nature as I-0067, which continues on the menus. Static analysis only;
the I-0066 pilot consumes the results.

## Question

1. **World post-processing (YEBIS).** What exactly must a native D3D12 backend provide to
   replace YEBIS's D3D11 layer?
   - the `PPFX::IGPUDevice` methods and resources;
   - the passes in order (luminance, adaptation, glare/bloom with the library's pyramid sizes,
     formats and weights, DoF, filmic tone map, LUT, dither);
   - for each pass: the DXBC, constants and targets;
   - the D3D12 equivalent: PSOs, root signature and barriers.
2. **Effect lights (FFX).** Why does the native scene light list hold 59 lights against 163 in
   the original?
   - a diagnostic plan by effect family;
   - the owner categories `owner+0x1F8` and the mask `[owner+0x1F0]+0x14C` (defaults, writers);
   - runtime checks the pilot can run.

## Scope

Included:

- Static Ghidra reading on a private read-only copy of the canonical database
  (`C:/Users/User/Tools/DS3-Decompilation/ghidra-mirror-i0068`); the I-0067 query script, reused
  unchanged.
- Reading the existing light-state witnesses (Reference1152, Native1153) and the I-0066
  branch's native sources, read-only.
- Notes, exports to `decompiled/`, query outputs under `evidence/I-0068/`.

Excluded:

- Launching the game, or any live test (the PC is reserved for the pilot's live tests).
- Native code; I-0067's MENUS-MAP and TONEMAP-SKIN, which stay I-0067's.
- ER offsets (ER Decompilation is read-only and only an architectural witness).
- Exhaustive decompilation of the YEBIS or FFX libraries beyond what the specification needs.

## Inputs

- I-0067 TONEMAP-SKIN sections 1, 5, 6, 15, 16, 17, and `evidence/I-0067/queries/yebis-world/`,
  `yebis-techniques.tsv`, `yebis-vars.tsv`.
- I-0067 MENUS-MAP sections 5, 10, 14, 30, 36 (Scaleform HAL pattern) and 38.
- Claims K-0410, K-0412, K-0433; I-0054 notes; I-0053 GPU order of one m40 frame.
- I-0066 Reference1152 / Native1153 light states, LEGACY-ROOT-INVENTORY-903 (SFX manager).

## Hypotheses

- H1 (YEBIS): the library's GPU work goes entirely through its own HAL (`PPFX::IGPUDevice` and
  its resource and effect classes). A native implementation of that HAL lets the library's CPU
  code keep computing passes, sizes, weights and constants.
- H2 (FFX): the 104 missing lights share one upstream cause rather than a per-family filter.

## Expected evidence

- Decompiled HAL methods with their D3D11 calls, the device state model and its resource
  creators; the YEBIS frame function and its pass functions, with technique names; the glare
  pyramid sizing.
- For the effect lights: the creation gate, the mask writers and defaults, and a check
  list in which each row names an exact address and its expected value.

## Exit conditions

- Success: both specifications written, each claim with its static source, with the runtime
  checks that would decide what static reading cannot.
- Refuted: H1 false (YEBIS issues D3D11 calls outside its HAL), or H2 false (a family-specific
  loss): record it.
- Insufficient information: record the exact runtime read or capture that decides it.
- Stagnation: no new pass or owner after 3 cycles on one item: move on.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`.

## Safety and human interaction

No live launch, no game-folder change, no native code. Read-only database copy, so the
parallel I-0067 workstream is never locked out.
