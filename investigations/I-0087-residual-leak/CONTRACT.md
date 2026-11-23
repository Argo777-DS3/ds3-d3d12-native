# I-0087 - Fuite résiduelle

Capability: C3 (Wave 2, stability; the residual slopes of I-0085)

## Question

After I-0085's fix, the 94-minute zone loop still raises the CPU descriptors in use by about 2 a zone visit and the local video
memory by about 0.6 MB a visit, both in a straight line (`i85-long-fix`). Which native records are created per visit and never
released, and does retiring them flatten both slopes?

## Scope

Included: the texture ingress's records (the only holder of the counted CPU descriptors), the menu offscreen targets (the Load
Game portraits made at each opening, kept until the same proxy address comes back), FaceGen images, and any other per-load
committed resource the census shows; a fix bounded to their release; a short zone loop before the merge; the replayed path gate
with its memory leg (D-0202, D-0204).

Excluded: the private memory's warm-up of the first loop (the game's own caches); the engine thread's third tranche.

## Inputs

I-0085 RESULT (observation 6), `i85-long-fix` (visits.csv), the code reading of the counter (`renderer/backend.cpp`
AllocateCpuDescriptor, `game_texture_ingress.cpp`) and of `NativeOffscreenRendererRetire` (it erased the proxies' map entries
without retiring their ingress records).

## Hypotheses

- H1: the slope is the menu offscreen targets: each Load Game opening admits portrait targets (a committed RGBA8 target and a
  descriptor each) that only an address reuse retires.
- H2: retiring a view's targets with the view flattens the descriptors and the local memory at the title.

## Expected evidence

The `native_model_registry` row's ingress census (texture records, render targets and their MB) per title stay, before and after;
the zone loop's descriptors and local memory per visit (Get-EnduranceSummary).

## Exit conditions

- Success: both slopes under 0.5 descriptors and 0.2 MB a visit on a loop of at least eight visits, portraits still shown; Opus
  reviews; the gate passing.
- Refuted: the census flat while descriptors grow (an outside holder); recorded, the next candidate taken.
- Stagnation: no new discriminating evidence after 3 cycles.
- Budget exhausted: close the tranche and replan.

## Safety and human interaction

Live runs under D-0116/D-0117, announced 5 minutes before with the PC clock, offline, the Shebang zone save (Alpha untouched),
measure mode without the debug layer; the other threads stop their heavy work until « PC libre ».
