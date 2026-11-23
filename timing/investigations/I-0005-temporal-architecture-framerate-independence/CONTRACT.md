# I-0005 - Temporal Architecture and Framerate Independence

Capability: C4

## Question

For the exact C1 executable, what architecture advances and distributes engine time, which major systems are actually coupled to render/presentation cadence or 60 Hz assumptions, and what is the smallest defensible intervention boundary for true framerate independence without changing gameplay durations, distances, animation, collisions, or behavior?

## Scope

Included:

- Identify non-presentation clock sources and producers of frame time, delta time, fixed steps, accumulators, clamps, quantization, substeps, or frame counters.
- Trace time propagation only far enough to classify major consumers: animation, movement/gameplay, physics/collision, camera, and other high-value temporal systems.
- Search for 60 Hz dependencies by combining constants with call/data-flow and consumer semantics, not by promoting raw constant matches.
- Establish couplings or separations among presentation, renderer cadence, engine time, simulation, and gameplay.
- Classify proven dependencies and identify minimal intervention points for a future decoupling prototype.
- Use bounded internal branches when necessary, while keeping one canonical I-0005 result.
- Consult DS3 Remaster only as secondary historical lead generation; reverify every promoted fact on BuildID 10167187.

Excluded:

- Exhaustive decompilation of the engine, complete recovery of every timer, or blind traversal of every `60`/`1/60` constant.
- Treating I-0004 presentation pacing as gameplay timestep without independent data flow.
- Arbitrary constant replacement, a blanket `60 -> 120` patch, or modification without an understood mechanism and validation plan.
- Runtime launch, debugger attachment, hooks, or live game-directory modification before explicit notice and authorization.
- Render-pass, shader, post-process, or asset work unless directly required to discriminate temporal coupling.

## Inputs

- Canonical claims: K-0001, K-0005, K-0007, K-0009 and validated I-0004.
- Artifact: `DarkSoulsIII.exe` 1.15.2.0, Steam BuildID 10167187, SHA-256 `EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C`.
- Static base: canonical Ghidra 12.1.3 project/profile, PE imports, targeted xrefs, instruction listings, decompiler output as supporting material, bounded constant/reference scans, and exact data-flow exports.
- Secondary source: historical DS3 Remaster documents or artifacts only when exact-build revalidation is possible.

## Hypotheses

- H1: presentation pacing is separate from the producer that advances engine/simulation time.
- H2: the engine has one or more centralized time states whose deltas propagate to several major consumers.
- H3: some important systems consume a fixed or quantized 60 Hz step while others consume variable elapsed time or independent clocks.
- H4: major high-framerate failures arise from a small number of producer/distribution boundaries rather than independent constants in every consumer.
- H5: camera, animation, movement/gameplay, and physics/collision do not all share the same temporal contract and require different decoupling treatments.
- H6: static analysis can identify architecture and intervention candidates, but runtime instrumentation is required before classifying activation, cadence, or behavioral invariants as confirmed.

## Expected evidence

- Imported/native clock call sites, frequency initialization, time-state writers/readers, and direct call/data-flow graphs tied to stable function IDs.
- Instruction-level arithmetic for deltas, clamps, accumulators, quantization, fixed steps, substeps, or frame counters.
- Semantically grounded 60 Hz constant uses with containing functions, upstream producer, downstream consumer, and category.
- Bounded propagation maps into at least animation, movement/gameplay, physics/collision, and camera, including explicit negative/unknown results.
- A coupling matrix separating presentation pacing, engine time, simulation, and consumer-specific behavior.
- Intervention candidates with prerequisites, risks, and a measurable 60/90/120 FPS validation design.

## Exit conditions

- Success: sources, timestep/distribution architecture, major consumers, proven 60 Hz dependencies, and a minimal intervention strategy are mapped sufficiently to begin a non-blind implementation or a precisely scoped runtime validation.
- Refuted: retain any influential architecture or coupling hypothesis contradicted by instruction/data flow.
- Insufficient information: record the exact dynamic values, call cadence, or consumer invariant that static evidence cannot distinguish and stop before launching DS3.
- Stagnation: no new discriminating producer, propagation, consumer, or intervention evidence after three cycles on one branch.
- Budget exhausted: stop when `iterations_used == iterations_max`.

## Safety and human interaction

Static-first. A live-game launch and reversible instrumentation may become necessary to confirm activation, cadence, or behavioral invariants. Stop before execution and tell Thomas: the hypothesis, measurements, required framerates, and every hook/file affected. Never launch DS3 unexpectedly or modify its installation without notice. No prototype may be prepared until its mechanism and validation gate are supported by I-0005 evidence.

