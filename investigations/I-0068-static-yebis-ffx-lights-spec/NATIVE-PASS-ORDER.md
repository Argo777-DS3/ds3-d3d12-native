# I-0068: the native frame, end to end: pass order, dependencies and implementation order

Request (coordinator, 2026-10-10): one file that orders every I-0068 specification inside the
native frame, from the G-buffer to gamma, with each spec's dependencies and a recommended
implementation order for the pilot.

**Basis:**

- the observed original order: the K-0407 order, the m40 named listing (I-0053), and the five
  `i63-live-10` captures;
- the specs of this investigation. They are the source of every detail below; this file only
  orders them.

Labels as in the specs. "Native today" refers to `main`, see COVERAGE-MATRIX.

## 1. The frame, in GPU order

| # | Pass | Main programmes / work | Native today | Spec |
|---|---|---|---|---|
| 0 | **CPU before the frame** | scene gather (`CollectNativeSelectedScene`), light items (points; spots `2136B0`), decal requests, the FX update `E6E750` (creates effect instances and **effect lights**), character fade and ExParams (`890950`) | partial: gather, points, decal CPU, character update | SPOT-LINE-LIGHTS §2, EFFECTS-PHASE1 §5, CHR-FADE-EXPARAM-REFLECTION §1–2 |
| 1 | **Shadow maps** | sun cascades (8192² R16, 4 cascades, cache and reprojection), local-light tiles (4096² R16), Dpt/DptA casters incl. characters (fading ones too) | absent | SHADOWS-NATIVE |
| 2 | **Water reflection view** (when armed) | second gather (mask low byte 0x10), mirrored camera, 640×360 HDR + depth, no water rows, no decals | absent | WATER-REFLECTION-INTERACTION-E08 §1, CHR-FADE-EXPARAM-REFLECTION §3 |
| 3 | **G-buffer** | 6 targets; stencil 9/8/12 (decal receivers); Emi families write GB5; material variants; character opacity (dither variant) | done for admitted families; stencil receiver bit wrong for characters; Emi map families and `Interior` refused (no engine-7 row for map parts); `_SnowLt` refused (three unproduced words) | DIRECT-MATERIAL-PREPARATION (I-0066), DECALS §3 step 1, EMISSIVE-GLOW §5 step 2, REMAINING-FAMILIES §3, NATIVE-GATES-AUDIT |
| 4 | **Copies** | GB0 → attr, GB1 → copy, depth → readable depth | absent | DECALS §1 |
| 5 | **Deferred decals** | `Decal_*` boxes into 7 targets, stencil bit 0 | CPU only | DECALS-NATIVE |
| 6 | **Depth downsample, SSAO, screen-space sun shadow, blur, composite** | `DepthScale2x2` ×2, `ScreenSpaceAO2_*`, `GXScreenSpaceCascadedShadow_*`, `OutputColor` + `GXScreenSpaceShadow_Blur` ×3, `GXRend_CompositeShadowMask` ×2 → t32/t33 | white t32/t33 | SSAO-NATIVE, SHADOWS-NATIVE step 8 |
| 7 | **Env-map regions** | `GXEnvMapRegion` → SpecularAcc | done (1147) | NATIVE-LIGHTING-MAP (I-0066) |
| 8 | **Lighting** | DeferredAO; sun `DDirLightShadowed1`; per tile: point batches, **spot / shadowed batches** (`1A6690`), line (`1A60F0`); CmbAlbedo; SSS (BlurX, BlurY); Composite + fog → HDR (adds GB5 emissive). Order corrected per K-0407 (I-0067 §71) | partial: points, SSS, composite; sun unshadowed; no spots | NATIVE-LIGHTING-MAP, SPOT-LINE-LIGHTS-NATIVE, SHADOWS §1.5 |
| 9 | **Forward opaque rows** (0x1F) and **forward 0x29** | depth write per rule; targets **[HDR, GB5]**; IBL, local-light lists, cbLight/cbFog | partial: one target, no depth write, IBL black | FORWARD-DEPTH-NATIVE, EMISSIVE-GLOW §5 step 1 / §8.1 |
| 10 | **Water** | clear the interaction map (0.5); `WatDpt` (row 0x24); **HDR frame copy** (t45 / t55); surface `Fwd`/`FwdSdw` (row 0x27, depth write); readable-depth copy | absent | WATER-NATIVE, WATER-REFLECTION-INTERACTION-E08 §2 |
| 11 | **Depth downsample #2, effects** | FFX E01/E02 (billboards, trails), E03 (model particles, m51 grass), E04 distortion / E05 radial blur (frame copy), E07 GPU particles, E08 light shafts; `GlowD` twins write GB5; object- and character-attached effects | absent | EFFECTS-PHASE2-E01-E02, EFFECTS-E03, EFFECTS-E05-E04, EFFECTS-E07, WATER-REFLECTION-INTERACTION-E08 §3, FFX-ATTACHED-NATIVE |
| 11b | **Velocity** (option MotionBlur) | group 0x33: clear, camera velocity `MotionBlurWriteCMBVelocity`, object `Velo` rows; group 0x35: dilation | absent | VELOCITY-MOTIONBLUR-NATIVE §2–§4 |
| 12 | **YEBIS** | DoF, glare/bloom, afterimage, light shafts, tone map, grading (41 calls at m40); motion blur with the velocity source when enabled | absent (no tone map) | YEBIS-NATIVE, YEBIS-TECHNIQUES, YEBIS-HAL-READY |
| 13 | **GX after-chain** | `PointSampleColor1`, `DLAA`; then **glow**: `EmissiveToGlowPS` → `Sample13Weight` ×4 → `GlowCompositeSinglePS` ×5 (additive onto the final target) | absent | TONEMAP-SKIN §17 (I-0067), EMISSIVE-GLOW-NATIVE |
| 14 | **UI** | Scaleform pages, menu offscreen views; movies (Bink) | UI done; Bink bypassed | MENUS-MAP / UI-INGRESS (I-0066/67), BINK-NATIVE-READY |
| 15 | **Gamma and present** | brightness curve, swap chain | plain final display | DISPLAY-SWAPCHAIN-NATIVE §9 |

**Effect lights.** They are created at step 0 (the FX update) and drawn at step 8 by the
point/spot paths. EFFECT-LIGHTS is therefore a dependency of the light passes, not of the effect
draws.

## 2. Dependencies per spec

| Spec | Needs first | Produces for | Can run without |
|---|---|---|---|
| **DECALS-NATIVE** | readable-depth copy and GB0/GB1 copies (its step 2); G-buffer stencil receiver fix | GB1–GB5, GB0 (normals) for lighting | shadows, SSAO, effects |
| **SSAO-NATIVE** | readable depth (or main depth); G-buffer normals | t33 (lighting, forward, points), t32 composite | shadows: composite with a white mask B |
| **SHADOWS-NATIVE** | caster rows (Dpt/DptA), cascade constants (host producers), the light-tile layout | sun shadow in t32 (with SSAO's composite), forward `FwdSdw`, shadowed lights | — |
| **NATIVE-LIGHTING-MAP + SPOT-LINE-LIGHTS** | point path (done), spot items (`2136B0` transcription), hull binning; shadowed variants need SHADOWS | HDR | shadows (unshadowed spots first) |
| **FORWARD-DEPTH-NATIVE** | readable-depth copy rule; forward rows | depth for water, effects, soft particles, DoF | — |
| **EMISSIVE-GLOW-NATIVE** | GB5 from the G-buffer (done) and forward RT1 (its step 1); `GlowD` sources need effects | the final image | YEBIS (composite onto the native final colour), effects |
| **WATER-NATIVE** | forward rows with passes 4/5/12; the HDR frame copy; readable depth; the interaction map (neutral); t54 (a default until the reflection exists); FwdSdw needs shadows, Fwd does not | HDR, depth | reflection, interaction simulation |
| **Water reflection** (part W/Y) | a second native gather and view; water planes from the main frame | t54 | — |
| **EFFECTS-PHASE1** | the native SFX manager (published), SceneCtrl construction | effect instances and **effect lights** | any draw |
| **EFFECTS-PHASE2 E01/E02, E03, E04/E05, E07, E08** | PHASE1; the frame copy (E04/E05); readable depth (soft particles) | HDR + GB5 (glow) | YEBIS |
| **EFFECT-LIGHTS** | PHASE1 (instances) | lights for step 8 | effect draws |
| **FFX-ATTACHED-NATIVE** | PHASE1 + replacing the `7CB820` deferral; E01 for the flame | torches, braziers, auras, held flame | — |
| **YEBIS-NATIVE / TECHNIQUES / HAL-READY** | the lit HDR (steps 8–11); depth for DoF | the LDR image | glow, UI |
| **CHR-FADE-EXPARAM** | NPC bodies (pilot, MENUS-MAP §55) | opacity and wet/damage looks | everything else |
| **BINK-NATIVE-READY** | the UI movie path | intro and cutscene videos | — |
| **DISPLAY-SWAPCHAIN §9** | the final LDR image | presentation | — |

**Shared resources:** create each once and reuse it.

- **readable-depth copy:** decals, SSAO upscale, forward, water, effects;
- **HDR frame copy:** water refraction, E04/E05, forward t45;
- **GB5:** composite, glow.

## 3. Recommended implementation order for the pilot

Order: dependencies first, then the largest visible gain per unit of work. Each item lists the
checks to pass before moving on.

| Rank | Work | Why now | Checks |
|---|---|---|---|
| 1 | **Shared copies** (readable depth, GB0/GB1 copies, HDR frame copy) | three later specs need them; cheap | DECALS §1 sizes/formats; WATER §1 step 3 |
| 2 | **Gamma** (DISPLAY-SWAPCHAIN §9) | every comparison image depends on it; small | its own checks |
| 3 | **YEBIS tone map first** (YEBIS-HAL-READY: the 29 native slots plus the tone-map techniques), then DoF/glare | the native world is visibly brighter without it; largest single gain | YEBIS-NATIVE and YEBIS-HAL-READY checks |
| 4 | **SSAO + composite with white mask B** | visible at every zone; needs only rank 1 | SSAO A1–A6 |
| 5 | **Forward depth + forward [HDR, GB5]** (EMISSIVE-GLOW step 1) | completes forward; prerequisite for water, effects and glow | FORWARD-DEPTH checks; glow §8.1 |
| 6 | **Glow chain + Emi families** | visible on lamps, eyes, bowls in all zones; no new system | EMISSIVE-GLOW G1–G6 |
| 7 | **Decals** (stencil receiver fix first) | blood and dirt in every fight; Firelink witness | DECALS D1–D6 |
| 8 | **Shadows** (sun cascades, then local tiles) | big visual change; enables FwdSdw and shadowed lights | SHADOWS S1–S6 |
| 9 | **Spot and shadowed lights** | m31/m37 interiors; needs rank 8 for the shadowed half | SPOT L1–L5 |
| 10 | **Effect system phase 1** (instances + effect lights, no draw) | unlocks lights of fires and held flames, then every effect | EFFECTS-PHASE1 §8, EFFECT-LIGHTS §5 |
| 11 | **E01/E02 draws + attached FFX** (torches, bonfires, auras, weapons) | the Argo torch and every fire | FFX-ATTACHED T1–T6, F1–F3; E01/E02 checks |
| 12 | **E03, E04/E05, E07** (then E08 with a witness) | grass, distortion, snow | their checks |
| 13 | **Water** (surface, then reflection view and filter) | m37 and Firelink; needs ranks 1, 5, 8 (FwdSdw) | WATER W1–W5, R1–R3 |
| 14 | **NPC fade and ExParams wiring** (as soon as NPC bodies exist) | small; rides on the pilot's NPC work | CHR-FADE F1–F3, X1–X3 |
| 15 | **Bink** | intro and cutscenes | BINK checks |
| — | without a witness (no work until a capture shows them) | water interaction simulation, E08, line lights, SSR, AO fields, cloud shadows | REMAINING-FAMILIES §2 |
| — | motion blur (now enabled by Thomas): after YEBIS, with captures M1–M5 | velocity pass, `Velo` rows, YEBIS blur | VELOCITY-MOTIONBLUR-NATIVE |
| — | the effects workstream as one sequence (ranks 10–12 and the glow) | | **EFFECTS-IMPLEMENTATION-CHECKLIST** |

## 4. Rules that apply everywhere

- **Run the original DXBC** for every pass. Build PSOs from the observed or STATIC states. Set
  write mask 0 on targets a programme does not declare (DECALS §2.4).
- **Two-target draws use non-independent blend:** RT1 follows RT0 (EMISSIVE-GLOW §8.1).
- **Constants come from the engine's own CPU producers** where they already run natively
  (`native/world` producers, the material path). New producers only where a spec says so.
- **Each spec's witnesses** are the five `i63-live-10` zones (m31, m37, m45, m50, m51) plus
  Firelink (m40) and the High Wall (m30) where named. No pass is "done" before its checks pass
  there.
