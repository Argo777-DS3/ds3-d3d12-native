# I-0073 - ER study: texture loading off the engine thread, the copy queue, the remaining draws (D-0195)

Capability: C3 (wave 2, loading and threading; preparatory study for the next V2 workstream, the continuation of I-0071
point 3). Requested by the coordinator on 2026-10-20 05:34 Paris; static and read-only.

## Question

How does Elden Ring 2.7.1 (1) load and decode textures off the engine thread, (2) use a D3D12 copy queue with fences for
uploads, and (3) prepare and record the draws that DS3's native backend still prepares and records on one thread (I-0071
point 3c), and what of it adapts to DS3?

## Scope

Included: ER 2.7.1 static decompilation (read-only headless Ghidra on copies of the analysed project, D-0195) of the task
system and texture pipeline, the queues and fences of uploads, the chunked recording of the sorted draws; DS3's counterparts
(the original's texture creation threads, the native backend's ingress, frame lists and recording) read in DS3's code.

Excluded: any ER or DS3 launch, live test, native code change or heavy build (the persistence thread measures on the PC);
ER offsets or layouts as DS3 facts; the design and implementation themselves (the next workstream, on Thomas's word).

## Inputs

- [I-0071 ER-STUDY](../I-0071-er-scene-persistence/ER-STUDY.md) (branch `claude/i0071-er-persistence`) and its
  [POINT3-PLAN](../I-0071-er-scene-persistence/POINT3-PLAN.md); [D3D12-DESIGN-REVIEW](../I-0066-independent-native-startup/D3D12-DESIGN-REVIEW.md)
  E4-E6, R5, R16; [ER-ENGINE-D3D12-DEEP-STUDY-716](../I-0066-independent-native-startup/ER-ENGINE-D3D12-DEEP-STUDY-716.md),
  [ER-DEEPENING-897](../I-0066-independent-native-startup/ER-DEEPENING-897.md); ER Decompilation K-0008, K-0009.
- Three Sonnet 5.5 reports (session scratchpad `er-L`, `er-Q`, `er-R`), non-canonical until Opus checks their sources.

## Hypotheses

- H1: ER decodes and creates textures in task-system jobs off the engine thread and publishes them into stable holders.
- H2: ER's initial-data uploads go through COPY-type lists on a queue of their own with fences (E6), possibly waited for by
  the CPU per resource.
- H3: ER records the sorted draws in chunks on workers into separate command lists submitted in chunk order.

## Expected evidence

ER function addresses with decompile excerpts for each claim, the carrying functions checked by Opus against the exports;
the DS3 counterpart named in DS3's code or the native source before a DS3 adaptation relies on it.

## Exit conditions

- Success: a checked ER map for (1) to (3) with its DS3 comparison and the adaptation proposed for the next workstream.
- Refuted: ER does not do one of (1) to (3) the way assumed: record what it does instead.
- Insufficient information: the static reading cannot settle a point: name it.
- Stagnation: no new discriminating evidence after 3 cycles.
- Budget exhausted: close the study with what is checked.

## Safety and human interaction

Static work only, Ghidra headless at below-normal priority on mirrors no other session uses. No merge into `main` (the
persistence thread merges I-0071 point 3b); the result waits on its branch.
