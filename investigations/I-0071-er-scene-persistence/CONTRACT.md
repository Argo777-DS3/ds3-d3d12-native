# I-0071 - Elden Ring architecture for scene persistence, texture binding and distant fades (D-0195), adapted to DS3

Capability: C3 (wave 2, scene data persistence, second tranche; Thomas 2026-10-20 00:36-00:44 Paris, D-0195).

## Question

How does Elden Ring's D3D12 engine keep model render data from one frame to the next, bind a texture that arrives, and fade
and cull distant characters, and how does DS3's native backend adopt that architecture in place of I-0070's own design
(transformed, not thrown away)?

## Scope

Included: ER 2.7.1 static decompilation (read-only headless Ghidra on copies of the analysed project) of (A) the per-frame
scene walk and what is cached per instance or material, its invalidation and its jobs; (B) texture slots, holders,
descriptors, streaming arrival and retirement; (C) LOD choice, fades and culling of distant characters. The adaptation to
DS3: what changes in the collect, the records, the texture choices and the prepare's kept draws.

Excluded: ER offsets or layouts as DS3 facts; any ER launch; the shader cache, loading/threading and full screen (later
workstreams), unless the ER reading shows they belong to the same mechanism.

## Inputs

- I-0070 (K-0454) and its design; ER-ENGINE-D3D12-DEEP-STUDY-716, ER-DEEPENING-897, ER Decompilation I-0008 boundary map.
- Three Sonnet 5.5 reports (scratchpad `er-study-A/B/C/report.md`, non-canonical until their sources are checked).

## Hypotheses

- H1: ER keeps per-instance draw data and invalidates it by events on the objects that changed (no global epoch).
- H2: ER binds textures through an indirection updated in place when a texture arrives (the draw is not rebuilt).
- H3: ER's distant characters follow LOD distances with hysteresis or fades that DS3's original also applies; the native
  path already follows DS3's weights (I-0070 i70-native-16/17).

## Expected evidence

ER function addresses with decompile excerpts for each claim, checked by Opus against the exports; the DS3 counterpart named
in DS3's code before a DS3 change relies on it.

## Exit conditions

- Success: a checked ER map for A, B and C, an adaptation proposed to Thomas and, on his word, implemented and measured
  against I-0070's runs (same saves and key paths).
- Refuted: ER does the same as I-0070's design: record it, keep the design.
- Insufficient information: the static reading cannot settle a point: name the runtime observation needed.
- Stagnation: no new discriminating evidence after 3 cycles.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`.

## Safety and human interaction

Static work on ER only (no ER launch). DS3 live runs under D-0116/D-0117 as usual for the measurements.
