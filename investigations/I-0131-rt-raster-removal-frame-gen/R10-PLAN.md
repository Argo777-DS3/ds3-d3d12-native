# R10-PLAN: the raster lighting removed, DLSS frame generation

The pilot's plan (no study thread report for R10; RT-PT-PLAN §1, §5, §8 R10; D-0235, D-0237, D-0249, D-0251, D-0253, D-0258).
Statuses: OBSERVED (read in the code or a source), INFERRED, TO CHECK. The inventory is a Sonnet reading (2026-11-08) whose rows the
decisions below rest on were re-read by the pilot (`12_scene_collect.inc:3080-3125`, `scene_pass_lighting.cpp:255-470`, 755-800,
`scene_pass_frame.cpp:1374-1400`).

## 0. In short

Two tranches. **R10a** removes the raster lighting that still runs: the sun's cascades and the local lights' shadow atlas (both
still live: they come back whenever the rays do not serve a frame), the environment cubemaps' raster pass, DS3's local-light batches
on the world view, and the flags that select them. What is left without the rays is one fallback, the same for every frame the rays
cannot serve (no DXR device, a failed self-test, a latched failure, skipped walks): **the frame lit without visibility**, as the 1 to
29 skipped walks already are today (the sun unshadowed, the local lights unshadowed, DS3's baked ambient), never a shadow map or a
cubemap again. **R10b** adds DLSS frame generation (DLSS-G) through the Streamline proxy chain the module already makes, in a file
beside the module (`fg-mode.txt`, off by default) until R12's menu.

## 1. What runs today (OBSERVED)

| # | Raster path | Runs when | Traced equivalent |
|---|---|---|---|
| 1 | Sun cascades: the casters' walk (`CollectNativeShadowCasters`), the cascade fit and volumes, the 4-cascade atlas, `SunShadowStage` (Soft and blur), `DDirLightShadowed1` with the cascades | `sun_traced` false: the rays off (`no-rt-scene`, no DXR, a failed self-test), `RtSceneFailed()`, 30 skipped walks in a row (`12_scene_collect.inc:3092`); never offscreen | `rt_sun_shadow` (I-0118) |
| 2 | Local shadow atlas: `CollectNativeLocalCasters`, the atlas faces, the shadowed point and spot programmes (`local_atlas_frame_`) | `local_traced` false (the same cases, or `rt-lights-check`) | `rt_lights` with the shadowed lights traced (I-0122 R4b) |
| 3 | DS3's local-light batches (`EncodeNativePointLights`, `GXLightAcc_DPointLight*`, `DSpotLight*`) on the world view | the native lights pass did not draw (the rays off, a size in transition, a refused camera, a failure), `rt-lights-check`; always on the menu views | `rt_lights` (RT `shade`, PT `pt`) |
| 4 | The environment regions' cubemaps (`EncodeNativeEnvRegions`, `GXEnvMapRegion*`) into the specular ambient | `EncodeRtReflections` did not draw (the rays off, `no-rt-reflect`, a failure); the menu views make no region (cleared) | `rt_reflect` (I-0125) |
| 5 | DS3's baked ambient GB4 in pass 0 | frames without the GI (the cases above), the menu views | `rt_gi` (I-0126) |
| 6 | The fill lights 1 to 3 in pass 1 | every frame (D-0257 keeps them) | R9d's trial (`rt-fill-shadow.flag`, Thomas's verdict pending) |
| 7 | The forward rows' baked probe rows, the effects' 1DB60 | until a traced probe answers (a new object's first frames), the menu views, `no-rt-probes` | `rt_probe` (I-0129) |
| 8 | The forward light grid (`GXLightOrganize`) | every frame: the forward rows' local lights, unshadowed | none (RT-PT-PLAN §1: the forward rows stay raster, lit by the traced probe) |
| 9 | The menu views (character creation, Load Game portraits, the in-game menu views): DS3's sun pass unshadowed, GB4, the batches | always | none: their lights are a studio set without a world, not in the rays' scene |

Already gone: the SSAO (D-0251; its leftovers below) and the water's mirror view (D-0251).

## 2. What R10a removes, what it keeps, and why

- **Removed**: rows 1 and 2's shadow maps entirely (walks, fit volumes, atlases, the stage, the shadowed programmes' use, their
  observers `cascade-observe`, `shadow-observe`, `caster-place`, `no-caster-volume`, `no-local-shadows` as far as they serve only
  the maps); row 4's raster pass (`EncodeNativeEnvRegions`; the regions and cubes stay as data: the rays read them on a miss,
  `rt_reflect.hlsl` Regions/Cubes); row 3 on the world view while the native lights pass can run; `rt-lights-check.flag` (its
  comparison is the batches beside the pass, D-0237 option A: the reference is the original); `no-rt-reflect.flag` (it only went
  back to the cubemaps); the SSAO's leftovers (the `ssao` resource in `frame_plan.cpp`, the bits the plan key no longer uses).
  `shadow_state` keeps what the traced sun reads (the direction, the scene words, `FC_ShadowColor`, the shadow frame's `on`).
- **The fallback, one for every frame the rays cannot serve** (D-0251: « handled by the workstream's design, not by keeping a raster
  mode »): the frame lit without visibility, as the 1 to 29 skipped walks today: the mask white (the sun unshadowed), the local
  lights through the native lights pass unshadowed (it runs without a top level), the baked GB4 as the diffuse ambient (no code:
  pass 0 binds GB4 instead of the black on a frame without the GI), no specular ambient. DS3's batches draw on the world view only
  where the native lights pass cannot: a device without DXR (the pass needs a Device5) and a session whose pass latched off after a
  failure; then every light unshadowed (the shadowed ones binned into the unshadowed programmes, no atlas). It is a fault's
  fallback, logged (`native_rt_fallback` rows), not an option: no flag selects it but `no-rt-scene.flag`, which stays the test tool
  of exactly this fallback (D-0253).
- **Kept, raster by design** (RT-PT-PLAN §1, D-0249: faithful to the original): the G-buffer, DS3's own shading of the sun and the
  fill lights in pass 1 with the traced visibility in the mask (the sun is shaded where its visibility is traced, as the local
  lights are shaded by the native pass with their traced visibility: replacing DS3's directional programme would change its BRDF,
  not its tracing), the forward rows and their light grid (row 8), the effects, the menu views (row 9: a studio set the world's rays
  do not hold), the post-processing, the UI.
- **Rows 5 to 7**: no code removed (GB4 and the probe rows are DS3's data, read only where the traced answer is missing); R9d's
  verdict decides row 6's trial (its flag's path removed if Thomas rejects it, I-0129).

## 3. R10b: DLSS frame generation (SOURCE: Streamline 2.14.16 `ProgrammingGuideDLSS_G.md`)

- **Loading**: `fg-mode.txt` beside the module (`on` or `x2`, `x3`, `x4` (Thomas, 2026-11-08 18:06: the RTX 50's multi frame
  generation, capped by the adapter's maximum), off by default; `fg-on.flag`, `fg-x3.flag`, `fg-x4.flag` the test tools), read at start. With it
  on, `slInit` loads `kFeatureDLSS_G` and sets `eUseFrameBasedResourceTagging` (the global tags; DLSS's local tags unaffected,
  ProgrammingGuide §2.12); the chain is made through the proxy factory as today (manual hooking: DLSS-G attaches to the chain made
  while it is loaded, §4). `sl.dlss_g.dll` and `nvngx_dlssg.dll` beside the module, signature-checked.
- **Pacing** (§12.1): with DLSS-G loaded the chain is made without `FRAME_LATENCY_WAITABLE_OBJECT` (Streamline's pacer owns the
  object); the native's waits on it are already skipped without one (`scene_pass.cpp:214`, `scene_pass_velocity.cpp:406`); the frame
  start is Reflex's sleep (`17_render_step.inc:547`), Reflex forced On while frame generation is on (§8: required).
- **Inputs** (§5): the depth and the motion vectors of DLSS's evaluation (render extent, the same constants and token), tagged
  `eOnlyValidNow` on the frame's list (Streamline copies them: the next frame reuses the targets); the HUD-less colour: the gamma
  pass drawn once more into an RGBA8 target of the back buffer's size before the UI (the same colour space as the back buffer), tagged
  the same way. No UI alpha (the UI draws into the gamma intermediate; its alpha is not separable): TO CHECK by eye in the menus'
  overlays.
- **On and off** (§6.4, §12): `slDLSSGSetOptions` on the presenting thread before each Present, `eOn` on a frame DLSS evaluated and
  tagged, `eOff` otherwise (title, menus' full-screen views, loading, movies, a frame without DLSS), `eRetainResourcesWhenOff`;
  `eOff` before a resize or a screen-mode change. The YEBIS motion blur halved while it runs (NVIDIA's checklist).
- **Witness**: `slDLSSGGetState` (`numFramesActuallyPresented`, `status`, `bIsVsyncSupportAvailable`) in a `native_frame_generation`
  row; PresentMon's FrameType. The ASUS is composed by Windows (no Independent Flip): with V-Sync, DLSS-G's latency is high there
  (§22.6) and 60 Hz gains nothing; the ASUS run is functional (generated frames presented, no fault), the value is the OLED's
  (280 Hz, Independent Flip, G-SYNC) when it is on.

## 4. Order and checks

1. R10a in steps: (a) the fallback made the single path (the batches' binning, the world view's gates); (b) the cascades; (c) the
   local atlas; (d) the cubemaps' pass and the flags; each built with its unit tests. A step review (fresh Opus, high), then a slot:
   the ASUS, 1920x1080 and two other sizes (WORKFLOW), Firelink, the High Wall and Anor Londo's hall with the rays (the picture
   unchanged, the journals' passes), and `no-rt-scene` (the fallback: unshadowed, no fault, no shadow map made).
2. R10b, then its step review and a slot (fg off: unchanged; fg on: generated frames, no fault, a resize and a screen-mode change).
3. The final review (fresh Opus, xhigh) on the final tree, the merge, the launcher rebuilt, the start check.
