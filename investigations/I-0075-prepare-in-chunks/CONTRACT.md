# I-0075 - V2: the render worker's prepare and the shadow casters' recording in chunks (D-0195)

Capability: C3, wave 2. Opened 2026-10-20 11:35 Paris from the I-0074 checkpoint (STATE « Next action »), in the I-0074 thread
while the fresh thread waited for Thomas's authorisation (coordinator, 11:34).

## Question

Can the render worker's prepare (8.8 ms at Archdragon Peak, 4.6 ms at m45, the longer worker path for 120 frames/s) and the
recording of the sun-shadow casters run in chunks on workers, after Elden Ring's chunked command runner (I-0073 section 3:
each draw prepared in the worker that records it, chunks planned per pass, ordered submission), with the same frames?

## Known inputs

- I-0074 RESULT and K-0457: the recording pool (`RecordingPool`, three workers beside the render worker), the G-buffer chunks.
- The prepare's shared state: the kept draws (`native_kept_draws`, read during the rows, changed only by new and dropped
  rows), the model versions, the per-call shared-block table, the frame lists, the counters, the D2 paint jobs; the caches it
  calls (programme sets, register plans, palette checks, material dynamic blocks) already have their own locks.

## Hypotheses

- H1: the rows split into chunks of 128, each with its own outputs, merged in row order, give the same frame (the same draws
  in the same order, the same kept-draw table) and divide the prepare by up to the worker count.
- H2: the shadow casters recorded in chunks of draws (each chunk all four cascades for its draws, with its own uploads) give the
  same atlas.

## Discriminating evidence

m45 prepare with and without the chunks in one build (`no-prepare-chunks.flag`), Archdragon Peak frames/s, the record
revalidation and prepare-shadow guards (kept-draw checks inside the chunks), the D3D12 validation run, pictures.

## Scope and exit

In: `PrepareNativeModelFrame` (main view), the caster recording. Out: the offscreen views, the collect. Success: the prepare
measurably shorter, 0 guard mismatches, 0 validation messages, pictures equal; reviewed and merged. Budget: 8 cycles.
