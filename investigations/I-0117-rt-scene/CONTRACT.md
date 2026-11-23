# I-0117 - DS3 DX12 Vague 2: R2 of the RT/PT plan, the scene in rays (La scène en rayons)

Capability: C3

## Question

Can every surface DS3 has loaded around the player (map pieces, objects, characters skinned, cloth, physics objects, alpha-tested
foliage) be put in a standard DXR scene each frame (BLAS per mesh, one TLAS a frame), checked against the raster on the same camera,
at a measured cost that keeps Mega Geometry unnecessary (D-0238)?

## Scope

Included (the plan prepared ahead, [R2-PLAN](R2-PLAN.md), copied from I-0110 in this investigation's first commit):

- T0 the BLAS registry at model storage admission (compaction, release with the storage, its rows);
- T1 the loaded-world instances (the engine's walk with a forged culler, everything loaded: Thomas, 2026-10-30 15:41) and the TLAS
  rebuilt each frame, the instance mask from the engine's categories;
- T2 skinned characters (compute skinning), cloth and physics refits;
- T3 the alpha-tested geometry in the ray query (`_e` materials, `alpha_ref`);
- T4 the traced control view (`rt-view.flag`) and its numeric check against the raster depth;
- T5 the measures (BLAS memory, refit and TLAS ms, walk ms) and the Mega Geometry verdict.

Excluded: materials beyond the alpha test (R3), lights (R4), shadows (R5), reflections (R6), GI (R7), denoising, LOD for the rays,
opacity micromaps and shader execution reordering.

## Inputs

- Canonical claims and studies: [I-0110 RESULT](../I-0110-rt-geometry-lights-study/RESULT.md) (DS3's geometry), [R2-PLAN](R2-PLAN.md),
  [I-0109](../I-0109-modern-dx12-base/RESULT.md) (R0: 12_2, DXR 1.2, the self-test, the bindless heap), D-0236, D-0237, D-0238,
  [RT-PT-PLAN](../../.project/RT-PT-PLAN.md) §8.
- Tools: clangd, GhidraMCP (Thomas's Ghidra), PIX (developer mode on since 2026-11-01), the native's rows.

## Hypotheses

- H1: the model storage's per-model buffers (float3 at byte 0, triangle lists) build BLAS without a copy.
- H2: the engine's world-layer traverser with a forged culler lists every loaded instance at a CPU cost under about 1 ms a frame
  with a static cache.
- H3: everything loaded fits in memory (about 1 GB of BLAS a zone, I-0110 §4 estimated) and the TLAS builds in well under 1 ms GPU.

## Expected evidence

Rows per tranche (`native_rt_blas`, `native_rt_tlas`, refits), the control view's depth agreement against the raster (share of
opaque pixels within tolerance, per zone), the counts against the MSB (`msb-m40.tsv`), PresentMon on the frame time with the flag
on and off.

## Exit conditions

- Success: T4's agreement at least 99 % of opaque pixels at Firelink, the High Wall and m45 (threshold set from the first measure),
  every disagreement explained; T5's measures written; R5 can trace on the scene.
- Refuted: a family that cannot enter the rays without a new engine read (named, with its plan).
- Insufficient information: a measure that cannot separate the cause (named, its next experiment).
- Stagnation: no new discriminating evidence after two cycles on the same blocker (an RCA then).
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`.

## Safety and human interaction

Everything behind `rt-scene.flag` (off by default) until R5 uses it; live runs announced (WORKFLOW « Live game rule »), on the OLED
when Thomas is present, the ASUS otherwise; the start check after each merge (D-0229).
