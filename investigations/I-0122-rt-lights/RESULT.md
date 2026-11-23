# I-0122 - DS3 DX12 Vague 2: R4 of the RT/PT plan, the lights for the rays (Les lumières pour les rayons): result

## Conclusion

Complete (opened 2026-11-03 by the R pilot on branch `claude/elegant-goodall-6ffe9a`, after I-0117 T7; plan [R4-PLAN](R4-PLAN.md);
R4c on `claude/nice-feistel-526624`, 2026-11-04). R4c: the PT level's direct light, every light of the tile casting, one sampled a pixel
and traced, behind Ray Reconstruction, an option (`rt-level.txt` « pt »); ReSTIR's reuse written twice, found biased by its reviews and
left out (« R4c »).
R4a is written, reviewed (step and final reviews) and was seen at parity with the original at four witness zones, Firelink at three
sizes (« R4a »). R4b is written and reviewed (step review; « R4b »): the shadowed local lights are shaded by the same pass, their
shadows traced, one ray a light a pixel, in the `Shadowed` programmes' form; seen at the same zones against the local atlas on the same
frames (runs `i122-r4b-*`), the pictures shown to Thomas. Thomas said yes on 2026-11-03 18:26 (D-0247): the local atlas, its faces'
draws and its casters' walk are no longer made while the local shadows are traced (« The atlas removed », written on I-0123's branch
`claude/intelligent-villani-341aa7`); the fill lights and the baked ambient kept until R7 (D-0247).

## R4a, the one list and the native local lights (2026-11-03)

- **What it does** (OBSERVED in the code): at collect, with the rays on and on the main view, the lights an unshadowed batch would
  draw become `RtLight` records (world space: the point's position from the light's +0x150, the spot's position and its
  view-to-light rows turned into world rows by the camera's pose; colours, 1 / R², the gloss clamp, the translucency, the spot's
  ShadingMode; the kind an integer) with, per 128-pixel tile, their indices in the batches' order. The renderer draws one
  full-screen triangle (`rt_lights.hlsl`, `rt_lights.hlsli`: the point, legacy point and spot terms of R4-PLAN §1) into the
  accumulation pair with the programmes' ONE/ONE blend, then the shadowed batches as before. The unshadowed batches are still made
  (step review M1): a frame whose pass cannot run draws them (counted in `native_rt_lights`'s `missed`). `rt-lights-check.flag`
  (a test tool) draws the original's unshadowed batches and the native pass into two separate pairs every 300th frame from 150 and
  reads them back (`rt-lights-<serial>-{batch,native}-{diffuse,specular}.r11g11b10`).
- **Unit tests** (`rt_scene_test.cpp`; build-r4a, then build-r4a3 with the reviews' fixes): the records (a spot's world rows give
  its view rows' values at the same point, the camera turned and moved); the pass on the hardware device against the point
  programme's terms computed in the test, from cleared targets, within one step of each channel's mantissa (red and green six bits,
  blue five). The spot, the legacy point, the translucency and t33 are checked by the runs only.
- **Runs** `i122-r4a-{m40-1280x720,m40-1600x900,m40-1920x1080,m30-1280x720,m37-1280x720,m45-1280x720}` (build-r4a, `779dbcd6`,
  2026-11-03 14:25-14:32, the ASUS `\\.\DISPLAY1` alone, offline, restored by hash; [Run-R4ZoneTour.ps1](Run-R4ZoneTour.ps1),
  rt-scene with rt-lights-check; no fault, 2,299 to 3,025 frames; m45's looks refused, the Claude app in the foreground; `missed`
  0 everywhere, so the fallback never ran). The three sizes at Firelink only (1600x900 a standard 16:9 size, not an arbitrary
  window); the other zones at 1280x720. Compared by [Compare-RtLights.py](Compare-RtLights.py) (rows in
  `evidence/I-0122/artifacts/compare-r4a-*.jsonl`): a pixel counts when either side is lit; its error in steps of 1/255 of
  max(1, the larger value).

  | Run | Diffuse lit pixels | Within 1 step | Within 8 | Worst | Specular lit | Within 1 | Worst |
  |---|---|---|---|---|---|---|---|
  | m40 1280x720 (5 pictures) | 3.73 M | 99.59 % | 100 % | 7.7 | 2.37 M | 100 % | 3.2 |
  | m40 1600x900 | 5.83 M | 99.60 % | 100 % | 8.0 | 3.70 M | 100 % | 2.0 |
  | m40 1920x1080 | 8.40 M | 99.59 % | 100 % | 8.1 | 5.32 M | 100 % | 4.0 |
  | m30 High Wall | 0.91 M | 99.95 % | 100 % | 3.9 | 0.41 M | 100 % | 2.0 |
  | m37 Anor Londo (533 lights in the list) | 0.48 M | 100 % | 100 % | 1.0 | 0.09 M | 99.997 % | 7.1 |
  | m45 Ariandel (2 pictures) | 0.82 M | 100 % | 100 % | 2.0 | 0.70 M | 100 % | 2.0 |

  No pixel off by more than 8.1 steps, none not finite. The tolerance the contract asked for, set from this measure: at most 8.1
  steps and at least 99.5 % of the lit pixels within one step, by target and zone. The differences are one-sided (m40 1920x1080,
  diffuse, five pictures: native above the batches at 562,260 pixels, below at 24,559; over two steps 11,105 above, 297 below;
  `compare-r4a-signs-m40-1920x1080.jsonl`), as R11G11B10's rounding toward zero after each blended batch predicts where the native
  pass adds once in float (INFERRED, the cause; OBSERVED, the sign). Firelink's spread is the largest: its tiles hold the most
  overlapping batches.
- **Step review** (fresh Opus, high, `20abd751..779dbcd6`): nothing high; the HLSL traced term by term against the three
  disassemblies, the world rows, the exclusivity of the list and the batches, the state after the pass, the slots' lifetimes and
  plan-key bit 46 found right. Answered: M1 (a frame with a list and no pass dropped its unshadowed lights) the batches kept as the
  fallback, the pass returning whether it drew; L1 the main view only; L2 the kind an integer field, not a float's bits; L3 the batch
  ring holds only the batches drawn, the list uploaded once a frame; L4 the check only after a pass that drew. Kept: L5, no GPU
  case per kind in the unit test (the spot's two modes, the legacy point, the translucency and t33 are compared against the original
  programmes themselves by the runs above, the stronger evidence), and no counting compute (the comparison is offline, on the
  pictures kept).
- **Final review** (fresh Opus, xhigh, `main...8ebeafe0`): nothing high or medium; the formulas, layouts, states, plan key,
  slots and mandate checked. Answered: L1 the check's pairs cleared before the pass (a frame whose pass did not draw reads back
  zeros); L2 the stale comments, the dead include and the aliases; L4 this section (the build the runs used, the frames, the sizes,
  the tolerance and the sign count); L5 STATE, the index and status; L6 the comparer and the tour script moved here; L7 the unit test
  from cleared targets at the format's step (no grazing pixel added). Kept for R4b (L3): the unshadowed batches still built every
  frame as the fallback (their CPU cost, small, not measured), the depth and G-buffer views held per pass in the bindless heap (the
  sun's, the control view's, the lights') and the t33 slot remade when t33 changes; one set of main-view views shared by the rays'
  passes when R4b adds its own. The fixes' step review (fresh Opus, high): nothing high or medium; L-a the unit test, from zero
  targets, no longer saw the blend: two pixels now cleared to 0.25 (the one without a surface left as it was, the first one's terms
  added to it); L-b the tolerance's comment. The test's 8-bit inputs set to 128 / 255 (0.5 lies halfway between two steps and rounds
  either way): worst relative error 1.8 % (build-r4a4).

## R4b, the local shadows traced (2026-11-03)

- **What it does** (OBSERVED in the code): the list holds every light a batch draws; a shadowed light (its class's `Shadowed`
  programmes all loaded and a face in the atlas, as before) carries `kRtLightShadowed`, its strength (the programmes' ProjParam.w for a
  point, ShadowParam.x for a spot) and a point's cube faces' near plane (−ProjParam.y / ProjParam.x; `RtShadowedOf`). The pass
  multiplies its diffuse (wrapped or Lambert) and specular by v = 1 + strength × (s − 1), the translucency left alone, as the three
  `Shadowed` disassemblies do (`DPointLightShadowed1`, `LegacyDPointLightShadowed1`, `DSpotLightShadowed1`); s is one inline ray toward
  the light through the frame's top level, the local casters' mask `RtMaskLocal` (the engine's category 0x04, the one the atlas's walk
  draws; R4-PLAN §3 first said RtMaskSun's) with the alpha test (`RtOccluded`, the query the traced sun shares). The ray leaves the
  surface toward the eye (the sun's offset), or along the normal toward the light for a translucent surface whose light and eye are on
  opposite sides of it (a thin translucent surface lit from behind), and stops where the atlas's view of the light begins: a spot's q.z = 0, a point's major-axis
  distance (on the view's axes, as the programmes pick the cube face) at its near plane; a near plane unknown leaves the light seen. A
  frame without a top level leaves the shadowed lights to their atlas batches, or shades them unshadowed when it has no atlas either;
  a frame whose pass cannot run draws every batch. On a traced frame without an atlas the shadowed lights are traced all the same, and
  shaded unshadowed on a frame with neither: `rt-lights-check` counts such a frame's shadowed lights as differences (the batches'
  pair lacks them). A point whose ShadowCenterPos is not its position (the programmes'
  cube is centred on it, the rays on the position) is counted in `native_point_light_batches` (`rt_centre_off`), and the shadowed points whose near plane
  cannot be read (`rt_near_unknown`: left unshadowed when their strength is not 0; counted also on a frame whose list is then dropped). The main view's
  bindless views (depth, GB0 to GB3) are made once a target generation and shared by the control view, the traced sun and the local
  lights (R4a's final review L3).
- **Unit test** (`rt_scene_test.cpp` LocalLights, build-r4b4): the light; the same light shadowed (strength 0.8), shadowed with
  strength 0 and unshadowed with a strength word (the flag decides); a shadowed light behind the plane lighting a translucent row
  through the wrapped diffuse and the translucency; a local caster on the rays of two columns, a quad of the camera and the sun alone on
  the others', the plane itself a local caster; traced, then without a top level. Then a point and a spot each beside its shadowed copy
  (strength 1), a local caster within the near plane (ignored) and one beyond it (hit); the spot also without a top level or an atlas
  (shaded unshadowed). Not covered: the alpha-tested candidates (the sun's query, tested by the sun's own unit test, now shared).
- **Runs** `i122-r4b-{m40-1280x720,m40-1920x1080,m40-1531x861,m30-1280x720,m37-1280x720,m45-1280x720}` (build-r4b2, `20fa6f13`, before
  the reviews' fixes: the near distance was then R / 1000 for a point and range / 100 for a spot; the points' words in the runs give the
  same plane (ProjParam {1.001001, −0.006006} for R = 6, {1.001001, −0.01001} for R = 10, runs i122-r4a); the spots' new cut is
  unit-tested only; 2026-11-03 16:22-16:29, the ASUS `\\.\DISPLAY1` alone, offline, restored by hash; [Run-R4ZoneTour.ps1](Run-R4ZoneTour.ps1),
  rt-scene with rt-lights-check; no fault, `missed` 0). The check now compares every batch (the shadowed ones from the atlas) with the
  native pass ([Compare-RtLights.py](Compare-RtLights.py); rows `evidence/I-0122/artifacts/compare-r4b-*.jsonl`): over the six runs,
  diffuse 98.35 % of 20.8 M lit pixels within one step (worst 65 steps), specular 99.91 % of 13.4 M (worst 110); the native mostly
  above (m40 1920x1080 diffuse: 1.13 M pixels above, 58,000 below over five pictures). The shadowed lights: the bonfire's point at
  Firelink, a point at Ariandel, a point and a legacy point at the High Wall, two spots at Anor Londo (`native_local_shadow`). Seen in the
  pictures ([Show-RtLights.py](Show-RtLights.py), `evidence/I-0122/artifacts/pictures-r4b`): the two shadows nearly alike; the traced
  one lighter where the atlas's four taps leave a grain (Firelink's bowl). Not a parity measure (the atlas is not the reference), a
  picture for Thomas.
- **Step review** (fresh Opus, high, `07701cb4..20fa6f13`): nothing high. Answered: M1 a shadowed light of strength 0 drawn twice on an
  untraced frame (the shadowed state a flag apart from the strength); M2 the near distance a ratio inferred from the lens words (now
  from the items' own words); L1 a strength outside [0, 1] dropped the frame's list (any finite strength now); L3 the test without
  translucency; L4 a shadowed name. Its fixes' step review (`20fa6f13..1a9bf1a0`): nothing high; M-1 the test could not tell the flag
  from the strength (two lights added), L-1 the major axis on the world's axes (now the view's), L-2 a spot's ray past its light
  (clamped), L-3 a ray to the light's centre when the near plane is unknown (seen instead), L-4 ShadowCenterPos (counted), L-5 the
  test's guards, a ray multiplied by 0 (skipped).
- **Final review** (fresh Opus, xhigh, `main...1a9bf1a0`): nothing high. Answered: M1 a thin translucent surface lit from behind would
  shadow itself (the ray's origin along the normal toward the light's side; the plane a caster in the test); M2 the tests (the near
  planes of a point and a spot, the collect's record as `RtShadowedOf`, tested); L1 this section's numbers and lights; L2 the record
  indirection removed (a list holds every light in order); L3 as L-1 above; L4, L7 the shadow query and the ray offset shared with the
  sun (`RtOccluded`, `kRtShadowRayOffset`); L5 a frame with neither a top level nor an atlas (the shadowed lights shaded unshadowed).
  Kept: L6, the atlas built every traced frame (until Thomas's yes). The second fixes' step review (`1a9bf1a0..10450735`): nothing
  high or medium; L1 and L2 the origin along the normal only for a translucent surface whose eye and light are on opposite sides (an
  opaque thin wall kept on the eye's side, a bump normal turned from the eye judged by the eye too); L3 above; L4 the unreadable near
  planes counted. Kept: L5, the test's camera unturned (the view's axes are the world's there) and no point without a near plane in it.
  Its own step review (`10450735..e9dc0965`): nothing high or medium; the count's scope written above; kept, no unit case of an opaque
  surface lit from behind by a legacy point (its specular has no N·L factor: the ray stays on the eye's side, through the surface).

## The atlas removed on traced frames (2026-11-03, Thomas's yes, D-0247)

- **What it does** (OBSERVED in the code): the collect sets `local_traced` with the traced sun's rule (the rays' walk made instances
  in the last 30 collects, the scene not failed) and the native pass on (`renderer.rt_lights`); then `CollectNativeLocalShadows` still
  selects the shadowed lights and their faces (their records keep `kRtLightShadowed`, the strength and the near plane for the rays),
  but `CollectNativeLocalCasters` is not walked and the frame's atlas edge is 0: the renderer retires its local atlas, draws no face
  and skips the shadowed batches; the native pass traces the shadowed lights, or shades them unshadowed on a frame without a top
  level (R4b's `traced` 2). The pass's failure (`rt_lights` off) or 30 walks without instances bring the atlas back.
- Runs: I-0123's `rr-a-*` (2026-11-03 18:56-19:13, 14 runs at four zones): no `native_local_shadow_draws` row in any (the atlas not
  made), 60 to 84 shadowed batches still collected at m30, m37 and m40 (their lights traced by the pass). I-0123's final review M1 and
  L1: a view without a face makes no atlas either; a refused atlas allocation no longer faults the renderer. Kept: a frame prepared without the atlas whose native pass then cannot run (a failure, a
  camera refused) loses its shadowed lights for one to three frames, until the next collect brings the atlas back.

## R4c, the PT level's direct light (2026-11-04)

- **What it does** (OBSERVED in the code; R4-PLAN §7): `rt-level.txt` « pt » beside the module (the player's option until R10's menu)
  or `rt-pt.flag` (the test runs) sets `renderer.rt_pt` at startup (row `native_rt_level`). On a main-view frame Ray Reconstruction
  denoises with a top level, the native local lights' pass draws `pt` (rt_lights.hlsl) in place of `shade`: every light of the pixel's
  tile a candidate, one chosen in proportion to the luminance of its unshadowed terms (resampled importance sampling with every
  candidate, streamed), one ray toward it (the RR-b sphere; every light casting whole: the `Shadowed` programmes' strength not kept; a
  point without a near plane stops at least 0.1 m short of its centre), its terms × Σp̂ / p̂(y) × v, plus every tile light's translucency
  unshadowed. One ray a pixel a frame. Any other frame (no RR, no top level, the check's pairs) shades the RT level as before. The frames
  shaded so are counted in the lights row (`pt_frames`). The default stays the RT level.
- **Two versions with reuse, written and reviewed, then removed** (R4-PLAN §7 « No reuse »): the first (`d61301ea`) reused four
  neighbours' and the previous frame's reservoirs; its step review found H1, a 1 / Z normalisation on the unshadowed target with
  visibility-weighted W that darkened every lit pixel beside a shadow, carried from frame to frame. The second (`e77bc622`) kept the
  temporal reuse alone; its review found H1', a bright overshoot for some 40 frames after every history restart (the empty reservoirs
  lost their M), checked by the reviewer's simulation of the shader's arithmetic (two lights, one hidden: 1.25 at frame 1, 1.02 at frame
  40, for 1.0). Its fixes would either lag a moving caster's shadow by up to 20 frames or darken the penumbrae. With every tile light
  already a candidate, the reuse only shares visibility; it stays out until a picture shows noise that RR leaves.
- **Unit test** (`rt_scene_test.cpp` LocalLights; build-r4c3, the test rebuilt alone after `64d17f14`, output in
  `evidence/I-0122/artifacts/build-r4c3/rt-scene.txt`): 256 PT frames of their own seeds over the R4b test's tile (five lights, all hidden
  behind a local caster on two of the four columns): the four front lights' rows within one format step of the terms with every light
  casting (worst 1.8 %), the translucent row (the light behind sampled) within 6 % (worst 3.2 %); the near-cut pair of R4b's test, a
  light seen and one hidden in the same pixel (worst 10 % of the mean, 20 % allowed: 256 draws of 0 or twice the value). Not covered: the
  lights' spheres.
- **Runs** (the ASUS `\\.\DISPLAY1` alone, offline, restored by hash; [Run-RrZoneTour.ps1](../I-0123-rt-ray-reconstruction/Run-RrZoneTour.ps1)
  with `rt-pt`; no fault in any): `i122-r4c-*` (build-r4c, the first version, 2026-11-04 02:50-03:02: six PT runs and the RT level at
  1280x720 at the four zones, the RT pictures the comparison's left side), `i122-r4c2-*` (build-r4c2, the second, 03:20-03:27, six PT runs)
  and `i122-r4c3-*` (build-r4c3, the merged form, 03:43-03:50): Firelink at 1280x720, 1920x1080 and 1531x861, the High Wall, Anor Londo and
  Ariandel at 1280x720; `pt_frames` at the 1,200th call 1,076 to 1,078 at Firelink, 971 at the High Wall, 640 at Anor Londo, 116 at
  Ariandel (frames without a local light in view draw nothing). The looks were refused (the Claude app in the foreground): one view a run.
- **Pictures** (`evidence/I-0122/artifacts/pictures-r4c`, the RT level left, the PT level right; `-build1`, `-build2` the two biased
  versions; `final-*` build-r4c6 at the final radii): judged by eye first, wrongly. The pilot told Thomas at 03:51 that Firelink was
  « clearly darker » at the PT level, and twice more that the new radii darkened it at the RT level; Thomas answered that it was
  expected (04:02), then that he saw no great difference (04:24). Measured afterwards (mean luma of the game area of `walk-1`, ffmpeg
  signalstats; 0 to 255): Firelink RT 49.71 (build-r4c), 49.85 (r4c6, the final radii), 49.65 (r4c7); PT 49.62 (r4c3), 49.61 (r4c7);
  the bonfire's region 83.9 to 85.1 in all of them; only the first, biased version was darker (47.95 overall, -3.6 %). The PT level and
  the final radii change the picture by less than the launches' own spread (rr-a 39.4 / 41.9 / 37.0 for the same view's three looks).
  The darker impression was the reduced side-by-side sheets. Corrected to Thomas (05:12). Why the difference is small: DS3's light is
  mostly the sun (traced already) and the baked ambient (until R7); its local lights are mostly weak fill lights.
- **The penumbrae's radii** (Thomas, 2026-11-04 02:13, « Je suis ta recommandation » on the pictures of I-0123 RR-b): the sun 2° and a
  traced local light's sphere 10 % of its range, at most 0.5 m (`kRtSunAngularRadius`, `kRtLightRadiusShare`, `RtLightRadiusMax`), for
  the RT and PT levels both; run at the four zones with build-r4c6 (`i122-r4c6-*`, 04:34-04:44, no fault), no change of the mean beyond
  the spread (above). A change that also stopped every traced ray at the light's sphere (`3d1ed5dc`, build-r4c7, run at Firelink
  05:00-05:03), written on the wrong reading of the pictures, was reverted before the merge: its step review found it let light through
  a thin wall within 0.5 m of a light (M1), and the measure showed nothing it fixed.
- **Step reviews** (fresh Opus, high): `4921e7c6..d61301ea` H1 (above), M1 the test blind to the spatial reuse, L1 an extent change
  keeping the pixel count, L2 history across gaps, L3 the PT clamp at zero, L4 work in empty tiles, L5 the binning assumed conservative;
  `d61301ea..e77bc622` H1' (above), L1 a salt shared, L2 to L4 words and evidence; `e77bc622..59546bea` nothing high: M1 the test could
  not tell which light's visibility was applied (answered: the near-cut pair), L1 and L2 leftover words (answered). Kept: L3 the clamp at
  zero in the PT target (DS3's colours are c² × k × scale × fade, non-negative: R4-PLAN §1; INFERRED), L4 an unshadowed spot cut at its
  matrix's near plane and the 0.1 m stop a guess (INFERRED, to see in the PT pictures), L5 one frame PT-shaded without RR when RR latches
  off within it, L6 the level row says « pt » even when RR is off (`pt_frames` 0 tells it).
- **Final review** (fresh Opus, xhigh, `main...7189833a`): nothing high; « mergeable ». Answered (`1f863aa7`, step-reviewed, then
  `60f88555` and `c318fab8`, step-reviewed): M1 the new radii unseen and able to dim a wall around its own light evenly (a disc point
  behind the receiver): a traced light's sphere now at most the light's height above the surface's planes, the shading one and the
  geometric one from the depth (the fixes' review M-a: a bump normal tilted off the wall), the PT stop the light's own sphere (M-b), and
  the radii run (« The penumbrae's radii »); L3 an unshadowed spot's housing stop at the PT level; L4 the PT pipeline made on its first
  frame, a refusal leaving the RT level and shown in the lights row (`pt_refused`); L6 the words. Kept: L2 the option file's parser
  copied from rr-mode.txt's (a UTF-16 file reads « p »: the RT level), L5 the unit test without the spheres or lights of unequal p̂ and
  visibility, L7 one hash a candidate. The fixes' review L1 kept: at a crease the geometric plane may mix two surfaces (the cap only
  shrinks: a sharper penumbra there).
- Builds: build-r4c (first), r4c2, r4c3 (the merged form's tests), r4c4 to r4c7 (the reviews' fixes; r4c6 the final tree but the L2
  gate, a cost only; r4c7 the reverted sphere stop).

## Evidence

- The light programmes' disassemblies: `evidence/I-0122/artifacts/dxbc/` (`fxc /dumpbin` of the I-0063 programme cache).
- The runs: `evidence/I-0066/artifacts/i122-r4a-*`; the tour's log `evidence/I-0122/artifacts/tour-r4a.log`; the comparisons
  `evidence/I-0122/artifacts/compare-r4a-*.jsonl`.
- R4c: the runs `evidence/I-0066/artifacts/i122-r4c-*`, `i122-r4c2-*`, `i122-r4c3-*`, `i122-r4c6-*`, `i122-r4c7-*`, their logs
  `evidence/I-0122/artifacts/tour-r4c*.log`, the pictures `evidence/I-0122/artifacts/pictures-r4c`, the builds `build-r4c*`.

## Uncertainty / unresolved questions

- The fill lights and the baked ambient: kept until R7 (Thomas, 2026-11-03 18:28, D-0247).
- The traced local shadows' look: accepted by Thomas (2026-11-03 18:26, D-0247).
- The pass's cost not measured (D-0227: no measure unless needed), the PT level's either.
- The PT level's picture: no measurable change of the mean at Firelink (above); the lights' adjustment with R7 (D-0247). The 0.1 m stop
  of a point without a near plane and the unshadowed spots' cut: INFERRED.
- ReSTIR's reuse (spatial, temporal): out until a picture shows noise RR leaves (R4-PLAN §7).

## Suggested next step

R6 (the traced reflections), whose hits need the lights in a world-space structure (R4-PLAN §7 « Not in R4c »); Thomas's verdict
on the PT level's look.
