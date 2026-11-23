# Investigation result

## Conclusion

The inherited correction in this result is **LTF — Locomotion Timing Fix**.
The separately observed slope/stair symptom is not an LTF identity and was
resolved later by the distinct I-0013 TRTF.

I-0012 is complete at cycle 6/14 as a Thomas-accepted bounded success.

The lightweight ReShade 6.8.0/API-20 host loaded successfully, exposed the
validated ReShade-free cadence/LTF core, operated in standard windowed
mode and unregistered normally at shutdown. Thomas reports a visual/gameplay
pass for the requested minimal panel: Hollow, early-game dog and Iudex Gundyr.

This closes the practical tool-and-panel question at `OBSERVED` strength; it
does not claim the instrumented spatial/temporal equivalence originally
available as the stronger success gate. Thomas chose not to add actor
measurement before closure. No enemy-, boss- or cadence-specific patch was
introduced.

Exclusive fullscreen produced no functional effect. The ReShade log shows that
fullscreen entry recreated the graphics runtime, but does not expose enough
internal state to attribute the limitation to ReShade, the add-on UI lifecycle
or the timing core. Windowed mode is the retained operating condition.

The independent test also revealed a new, visibly distinct issue: player
hopping/jitter on slopes and stairs. That observation is not classified as an
I-0012 regression. It becomes the central question of successor RCA I-0013.

## Evidence

- [Independent test result](../../evidence/I-0012/INDEPENDENT-TEST-RESULT.md).
- [Preserved ReShade runtime log](../../evidence/I-0012/sessions/20260921-independent-test/ReShade.log).
- [Offline ReShade/core inventory](../../evidence/I-0012/RESHade-INVENTORY.md).
- [API selection and provenance](../../evidence/I-0012/prototype/API-SELECTION.md).
- [Tool guide](../../evidence/I-0012/prototype/README.md).
- [Build validation](../../evidence/I-0012/prototype/BUILD-VALIDATION.json).
- [Formula/guard and architecture validation](../../evidence/I-0012/prototype/DERIVATION-VALIDATION.json).
- [Exact-build validation](../../evidence/I-0012/prototype/EXACT-BUILD-VALIDATION.json).
- [PE/API-host validation](../../evidence/I-0012/prototype/PE-VALIDATION.json).
- [Ten-run safety output](../../evidence/I-0012/prototype/NATIVE-RESTORATION-STRESS.txt).
- [Evidence manifest](../../evidence/I-0012/manifest.json).

## Uncertainty and unresolved questions

- The Hollow/dog/Gundyr pass is unmeasured operator evidence. Exact actor
  identity, cadence, action alignment, transforms, path distance and recovery
  duration remain unqualified.
- LTF's non-player reach remains unknown; a visually good
  outcome cannot distinguish a shared corrected path from an already-correct
  independent path.
- Exclusive-fullscreen failure is bounded but not causally localized.
- Terrain collision/simulation behavior is explicitly not passed. The new
  slope/stair observation requires the 60-OFF / 240-OFF / 240-ON matrix before
  deciding whether LTF participates.
- Genuine Unlimited and whole-C4 correctness remain unqualified.

## Artifacts and cleanup

- Host-independent cadence/control core and ReShade settings host under
  `evidence/I-0012/prototype/`.
- Validated `DS3TimingValidation.addon64`, strict build records and a
  hash-guarded one-file manager.
- Preserved independent-test report and normalized ReShade lifecycle log.
- At closure DS3 is closed and the manager reports the add-on absent. Process
  exit unregistered the add-on; no persistent game executable patch exists.
  ReShade's own UI/configuration residue was left intact rather than rewritten
  without an exact original byte copy.

## Next step

Open I-0013 in a new Work conversation as a terrain-collision high-framerate
RCA. Start from a clean integrated `main`, reuse only validated I-0012/I-0010
infrastructure, and make the user-specified 60/240 OFF/ON matrix the first live
discriminator. Do not propose a collision fix before that matrix identifies
whether LTF participates.
