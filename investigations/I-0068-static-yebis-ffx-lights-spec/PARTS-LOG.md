# I-0068 part-by-part log (2026-10-10 to 2026-10-11)

Moved out of [RESULT](RESULT.md) on 2026-10-21 (documentation cleanup, tag `archive/pre-cleanup-2026-10-21`), unchanged below:
the conclusion of each part (A to BQ), the evidence, the open questions and the artifacts of the whole investigation. Each
part keeps its own date; its « ready to code » and « next step » describe that moment.

## Conclusion

First tranche complete (static, parallel to I-0066, 2026-10-10). Both hypotheses supported.

- **Effect lights (part B, [EFFECT-LIGHTS](EFFECT-LIGHTS.md)), H2 supported (STATIC + OBSERVED).**
  - The 104 missing lights (Reference1152 163, Native1153 59) are absent in every parameter
    family, appended after the 59 map lights.
  - Every effect creation in `SprjSfxImp` requires its GX effect system `[SprjSfxImp+0x58]`
    (`GXFfxSceneCtrl`). The native SFX manager publishes it as null, by design, because its GPU
    members need the absent GX display object.
  - So no effect, no FFX update, no light host, no effect light.
  - The hide mask `[SceneCtrl+0x14C]` is set only from the map step during cutscenes (INFERRED
    Remo); the categories `+0x1F8` default to `0xFFFFFFFF`. Neither can hide lights in gameplay.
  - The light mask of effect lights comes from the create-info (`0x11/1`), not from the light
    defaults.
  - Runtime check R1 (`[[4799C98]+0x58]`) decides it in one read. Checks R0–R10 and a plan by
    effect family follow.
- **YEBIS (part A, [YEBIS-NATIVE](YEBIS-NATIVE.md)), H1 supported (STATIC).**
  - The library reaches D3D11 only through its HAL (`PPFX::IGPUDevice`, 77 slots, its resource
    classes and the `CTinyEffect` effect runtime). The HAL is decoded:
    - device layout;
    - 64 render-state combinations (5 blend modes × 3 write masks × scissor) and 16 sampler
      modes;
    - per-draw commit of one shared 4096-byte constant buffer, SRVs and per-texture samplers;
    - targets, creators, the full-screen-triangle draw path.
  - Recommended native route: implement the HAL on D3D12 and keep the library's CPU code, as
    for Scaleform.
  - Pass order: library structure (PreProcess, InternalProcess, PostProcess) mapped onto the 35
    draws of one m40 frame.
    - DoF → glare source → 6 bloom levels → `MadLumGammaT6` composite → afterimage →
      `TonemapHDR` into an intermediate → `MadColorGradingLutT1` into the LDR target.
    - INFERRED by inputs. It refines TONEMAP-SKIN §16: the LUT pass, not the tone map, is
      last.
  - Glare pyramid formula from the library: quality table `14469B4D0`, base
    `W·s·√(1/r)` with r = 1.5, levels rounded up by halves. It reproduces the observed 392×331
    and six levels.
  - D3D12 design: PSO key, one root signature, sampler handling, barriers, temporal resources,
    lifecycle.
- **Native `GXFfxSceneCtrl` (part C, [FFX-SCENECTRL-NATIVE](FFX-SCENECTRL-NATIVE.md), STATIC).**
  - The scene control is the GX adapter of the FFX middleware: CPU parts (type registration,
    creation, FXR resolution, library update, light hosts) and GPU parts (five constant buffers,
    a 2 MB ring, shader set, resource manager, half-resolution targets, GX contexts and jobs).
  - It is driven by `SprjSfxImp::Update` (`E6E750` → slot 8), by the GX scene graph through its
    `TGXSgEntity` (`+0x8`, slot 1 → `Draw` slot 10, GX pass ids 0x20/0x21/0x29/0x2D/0x31,
    0x39, 0x45), and by the display listener list for resize.
  - Native plan: phase 1 publishes an engine-layout object with the CPU code and a native
    transcription of the update (effects run, lights appear, no draw); phase 2 adds the draw
    path by appearance family, including the four direct D3D11 sites.

- **Phase 2 by family (part D, [FFX-FAMILIES-PHASE2](FFX-FAMILIES-PHASE2.md), STATIC + reflection + OBSERVED inventory).**
  - Shader-set table of `FUN_140291520` decoded (128 VS/PS entries, glow twins at +75; VFX
    tables).
  - Full reflection of the 327 FFX programmes: the five scene-control buffers match
    `VSCommonBuffer`/`DSCommonBuffer` (448), `PSCommonBuffer` (560), `PSLightBuffer` (400)
    and `cbFog` (224); 69 `GlowD` pixel programmes write two targets.
  - States from the GX common bank by FXR blend type.
  - The four direct D3D11 sites: one creates resources, one is the tessellated builder
    (1-control-point patch list), one is the triangle-list builder, one issues `Draw(4)`.
  - m40 order: E01 → E03 → E02 → (E06 phase 1) → E07 → E05 → E04 → E08.

- **Display modes, swap chain and screenshots (part E, [DISPLAY-SWAPCHAIN-NATIVE](DISPLAY-SWAPCHAIN-NATIVE.md), STATIC).**
  - The game has three screen modes: windowed, borderless, and exclusive full screen
    (SetFullscreenState). Windowed uses a fixed 12-entry 16:9 list filtered by the desktop;
    full screen uses the output's GetDisplayModeList(R8G8B8A8_UNORM).
  - Mode changes are queued (FUN_14000C240) and applied at the next GX frame start
    (FUN_140009EA0): ResizeTarget / SetFullscreenState / ResizeTarget, then
    ResizeBuffers on WM_SIZE (FUN_14000B700/B830), then the listeners.
  - The swap chain has an optional gamma pass: an intermediate RT and a 1025×3 R32_FLOAT curve,
    drawn before Present.
  - D3D12 equivalents are given step by step. Screenshots are a debug path (readback + WIC).
  - **Gamma pass (§9)**: always enabled in the shipped game; the curve is the brightness option, `x^(2.2/(2.2 + 0.1·(v−5)))`; shaders disassembled (full-screen strip quad, one 1D-array LUT lookup per channel); D3D12 root signature and PSO given.
  - The remaining D3D11 sites outside GX are listed.

- **YEBIS techniques, bloom arithmetic, HAL creators (part F, [YEBIS-TECHNIQUES](YEBIS-TECHNIQUES.md), STATIC + OBSERVED).**
  - Technique names come from handle tables filled at init: the base name with its row number
    (taps or textures) and one of 22 HDR-encoding suffixes.
  - Per draw: downsamples `MadT1_S{taps}` (taps from sizes), upsamples `MadT1_S4`, bright pass
    `BrightPassT1_[Vignette_]THRESHOLD_LUMINANCE_REMAP_S1`, bloom combine `MadLumGammaT{levels}`,
    composite `MadLumGammaT{components}`, blurs `MadT1_S{n}` with per-channel weights.
  - Read in the five witness zones from the retained captures' programme ids. YEBIS-NATIVE §3.2
    is corrected: draw 330 is the bloom combine, 337 the composite.
  - Bloom formulas: radii scaled by wavelength (615/540/465 nm), taps from a Gaussian threshold,
    level weights `L/q^i` with tint.
  - HAL slots 60–67 are named from their own diagnostics: 64 `CreateRenderTarget`, 65
    `CreateDepthStencilSurface`, 63 is a stub.

- **FFX static closure (step 3, [FFX-FAMILIES-PHASE2 §6](FFX-FAMILIES-PHASE2.md)).**
  - The five effect constant buffers are uploaded whole from draw-context blocks whose bytes are
    the reflection layout, so the native uploads the blocks as they are.
  - The direct builders have no private buffers.
  - The scene control's half-resolution targets have no reader (negative search) and are not
    needed natively.

- **Five witness zones (step 4, [FFX-FAMILIES-PHASE2 §7](FFX-FAMILIES-PHASE2.md)).**
  - m31, m37, m45, m50 and m51 carry the same seven families E01–E07. E08, E09 and E10 are
    absent, so no family is added.
  - Aggregate draw order: E01 → E03 → E02 → E05 → E04 → E07.
  - E06 lights rank second on m37, m50 and m51.
  - E07 comes first on Ariandel for the falling snow.

- **Frame copy for E04/E05 ([FFX-FAMILIES-PHASE2 §8](FFX-FAMILIES-PHASE2.md)).**
  - `g_frameBufferTexture` is pipeline `+0x1108`: view-size R16G16B16A16_FLOAT, or R8G8B8A8 on the
    LDR pipeline, created by `FUN_140085540`.
  - It is copied from the lit HDR target at the start of the forward pass 0x29 and bound at GX
    slot 45.
  - It reaches effect draws through `passCtx+0x4C0` → draw context `+0xA70`; slot 8 with a null
    texture binds it as t0, and it is also t4.

- **YEBIS closure ([YEBIS-TECHNIQUES §8](YEBIS-TECHNIQUES.md)).**
  - DoF pre-pass `DepthOfField_FrontBlurBlendFactor`.
  - DoF composite `CompositeBlurLevelsFrontBlur[_USNORM]_TS0_3` (front-blur mode, 3 front layers).
  - Blur tap rule with its zone fields (radius scale: m37 0.5, others 2), which matches the
    observed tap classes. The exact n is not reproduced numerically: one factor is missing.
  - Light shafts: a depth-to-scene-alpha pass `Alpha_<channel>`, then `LightShaftMask_SM3`
    (glare source + film-grain noise → base-size R11G11B10), a radial blur, and the composite.

- **Effects phase 1 ready to code ([EFFECTS-PHASE1-NATIVE](EFFECTS-PHASE1-NATIVE.md)).**
  - An ordered call / transcribe / native list from `E6C9D0`: sub-objects, the scene-control
    constructor without its five GX buffers, type registration, the resource manager and
    `GXFfxManager` with a device token, and the scene entity.
  - The per-frame update transcribed with native update passes whose draw-side work lists are
    not executed.
  - Minimal layouts and the R1–R7 checks (163 point lights at the m40 witness).

- **Effects phase 2 for E01/E02 ready to code ([EFFECTS-PHASE2-E01-E02-NATIVE](EFFECTS-PHASE2-E01-E02-NATIVE.md)).**
  - The 2 MB GPU ring and its map/unmap contract, the shader set and the SFX scene layer.
  - The scene-control draw at passes 0x20/0x21/0x29/0x2D/0x31 plus 0x39/0x45, and native
    `GXFfxDrawPass` and pass-context fields.
  - Command execution and the original draw context with only slot 27 replaced.
  - The PSO key and root signature; the eight E01 programmes drawn at m40; the E02 tracer set.
  - The three builders' GX statements and the frame-copy hookup.
  - Checks at m40 and m37 (about 68 patch-list draws per frame). Two are uncovered: the gamma shaders (not
    disassembled) and FUN_140EF61A0 (encrypted; the engine keeps it).

- **Native YEBIS HAL with the gamma pass, ready to code ([YEBIS-HAL-READY](YEBIS-HAL-READY.md)).**
  - The 77 `IGPUDevice` slots classified CALL or NATIVE: the device keeps its 0xA50 layout, and
    only the 29 slots that reach D3D11 are replaced.
  - The factory, the native `IPfxContext`, and creators 60–67 with their companion
    texture/surface slots.
  - Lifecycle and resize, the lazy PSO key built from `TexUtil_D3D11`, the `$Globals` root CBV,
    barriers, and the hook-up: native HDR → YEBIS → LDR → after-chain → gamma pass → back buffer.
  - Checks at m40 and m31 (48 draws with light shafts), and the risks.
  - **Correction:** the m40 frame has 41 YEBIS draw calls (42 draws), not 35 (YEBIS-NATIVE §3.2).


- **Native Bink player ready to code ([BINK-NATIVE-READY](BINK-NATIVE-READY.md)).**
  - Bink's CPU decoding is kept. Only the following change:
    - two `CSMovieIns` steps are transcribed (they read the old device);
    - the player's open, close and render are transcribed;
    - five graphics slots and the publish become native.
  - Planes: `R8_UNORM` Y, cR and cB, uploaded through a dedicated ring (about 3.1 MB per frame).
  - The PSO and root signature come from the disassembled shaders (c1..c4 = Bink's
    colour-space columns, sampler `MIN_MAG_LINEAR_MIP_POINT` clamp).
  - Audio is Bink's own XAudio2 (no D3D).
  - The bypass and the replacement detours; checks on `fdp_opening` (4750 frames, 197.9 s) and
    the risks.

- **E03 model particles ready to code ([EFFECTS-E03-MODEL-PARTICLES-NATIVE](EFFECTS-E03-MODEL-PARTICLES-NATIVE.md)).**
  - `GXFfxFlverModelRenderer` → model adapter → `FUN_1400636F0` → the FLVER mesh loop
    `FUN_140063CB0`. This is one level below the world entry `FUN_140059040` that the native
    selection reads.
  - Missing for the native FLVER path:
    - an entry at `FUN_1400636F0`;
    - native storage for FFX-owned FLVERs;
    - the world-space bone palette;
    - the 400-byte FXR draw state;
    - the FFX pass id;
    - the pass constants.
  - Ordered list, checks relative to a Reference run (m51, m45, m40), risks.

- **E05 radial blur and E04 distortion ready to code ([EFFECTS-E05-E04-NATIVE](EFFECTS-E05-E04-NATIVE.md)).**
  - **Correction:** the FFX shader set has 158 VS/PS entries, not 128. `Get-ShaderSetTable.ps1`
    is fixed and the TSV regenerated. The soft E05/E04 programmes are at 139–145.
  - Kinds 0x1F–0x25 map to indices 57–63, or 139–145 when soft. The distortion twins of bump
    billboards and tracers are chosen when `ctx+0x5A0 > 0`.
  - Inputs and used constants per programme; t0 = the frame copy through slot 8.
  - One copy per frame at the start of pass 0x29.
  - PSOs, checks L1–L7 (E04 OBSERVED in 47 of 1,800 m40 frames, E05 in i63-live-33), risks.

- **E07 GPU particles ready to code ([EFFECTS-E07-GPU-PARTICLES-NATIVE](EFFECTS-E07-GPU-PARTICLES-NATIVE.md)).**
  - The per-system GPU block (`S` = appearance `+0x700`).
  - Buffers: ping-pong 176-byte particle buffers with append/consume UAVs, a counter buffer
    copied into the draw and dispatch arguments.
  - Constants: `ccbSceneParam`, `ccbForceFiledParam`, `ccbEmitionSeed`, `gcbSceneParam`,
    `cbCubeEnvSH1`.
  - Simulation (`FUN_1402A6A30`): emit `Dispatch`, then update `DispatchIndirect`. It runs in the
    pass-0x45 flag-0x10 submission, which corrects FFX-SCENECTRL §3.1.
  - Sort and draw (`FUN_1402A7E70`): `InitSort`/`BitonicSort`, POINT + GS, `DrawInstancedIndirect`.
  - Statement-by-statement D3D12 translation: counters, `ExecuteIndirect`, copies, barriers.
  - Checks on the Ariandel snow (OBSERVED: two indirect draws, 2,048/8,192 records, 1,710/5,130
    live) and m40; risks.

- **Forward depth ready to code ([FORWARD-DEPTH-NATIVE](FORWARD-DEPTH-NATIVE.md)).**
  - The material pass apply `FUN_140140850` writes depth (LESS_EQUAL) unless `args+0x188 = 0` or
    the material sets `g_GX_bDisableDepthWrite`. The default mode is −1, from
    `FUN_140236290`. Hair MTDs write depth.
  - OBSERVED in five zones: about 20–24 depth-writing and 4–8 test-only forward draws per frame.
  - A readable depth copy (`CopyResource`, three per frame) made after the writes; the soft
    effects and lights read it.
  - Readers: FFX t3, E07, YEBIS DoF and light shafts.
  - Native contract: state rule, DEPTH_WRITE, copy after the writes, order. Checks D1–D5, risks.

- **Shadows ready to code ([SHADOWS-NATIVE](SHADOWS-NATIVE.md)).**
  - Native today draws no world or character shadows: no atlas, no casters, t32 white,
    unshadowed sun. Host-era producers exist in `native/world`.
  - The cascaded-shadow object (`FUN_1401B5390`): four cascades, `cbPerScene`, per-cascade bias.
    Sizes and clamps come from `FUN_1401B5720`.
  - OBSERVED: an 8192² R16 atlas holds the four 4096² sun cascades, and a 4096² R16 atlas holds
    512² light tiles. The player set (22 depth-only + 5 alpha-tested casters) repeats in every
    view that contains the player.
  - Dpt/DptA programme interfaces, and the receivers (screen-space PCF4/9/Soft, composite with
    SSAO into t32, `DDirLightShadowed1`, shadowed point/spot).
  - Ordered list; what characters lack (the Dpt/DptA recipes, one selection per shadow view);
    checks and risks (static shadow cache).

- **Shadow cache closed ([SHADOWS-NATIVE §1.4](SHADOWS-NATIVE.md)).**
  - `GXRend_CopyShadowMapDepth` (lighting set `+0x38`) is drawn only by `FUN_140108430`: an
    ALWAYS/write full-screen reprojection of an earlier cascade depth.
  - Per cascade (`FUN_1400F9B50`):
    - a change of light direction invalidates all four;
    - a cascade whose camera slice stays inside its previous volume is kept (no pass,
      `FUN_1401B5B40`);
    - otherwise it is recorded in full (`FUN_1401B6020`), possibly in reprojection mode (cache
      swap, copy, previous volume excluded from static casters, dynamic casters redrawn).
  - Native reference: redraw every cascade with all casters; the cache is an optional
    optimisation.

- **SSAO and the composite ready to code ([SSAO-NATIVE](SSAO-NATIVE.md)).**
  - Native has no SSAO: t33 and t32 are white. The constant producers already exist in
    `native/world`.
  - OBSERVED identical chain in the five zones, per frame:
    - depth downsample 640×360 and 320×180;
    - `ScreenSpaceAO2_WithNormal` at half resolution, temporal `SeparatedReProj` with a history
      copy, `BilateralUpScale` → t33 1280×720 RGBA8;
    - cascaded shadow, blur ×3;
    - `GXRend_CompositeShadowMask` ×2 (stencil refs 0/16) → t32.
  - Readers: DeferredAO and the sun (t32), local lights and forward (t33).
  - Ordered list (AO can ship before shadows); checks A1–A6; risks: history, stencil bit 0x10,
    programme 189.

- **Part R, rendering coverage matrix** (`COVERAGE-MATRIX.md`): 48 families and passes of the
  original frame in frame order, each with its native state on `main` (`2c4934bd`, `file:line`
  proof) and its specification. Done natively: map G-buffer, deferred lighting with point lights,
  env-map regions, SSS, fog, player model, menus and UI. Specified but absent: shadows, SSAO,
  YEBIS, effects E01–E07, forward depth, gamma, Bink. No spec yet: deferred decals (taken next,
  part S), water, spot/line lights, character light probes, velocity/motion blur, SSR/AO fields,
  material variants (POM, snow, windows, crystal), E08.

- **Part S, deferred decals ready to code** (`DECALS-NATIVE.md`, first family without a spec in
  the coverage matrix). Native has the CPU decal manager, requests and programme lookup; the GPU
  resources and the draw are refused, and the G-buffer stencil marks characters as receivers.
  The original pass is OBSERVED in Firelink-session captures and absent from the five witness
  zones: three copies (GBuffer0, GBuffer1, depth), then one `Decal_DifSpcAm` box per decal
  (`DrawIndexed(14)` strip) into seven targets with stencil bit 0 EQUAL. STATIC flow:
  51AA0 → 19B690 → 198910 → 52890, with the 47-programme selection, `cbDecalParam` and
  `cbMatrixWorld` sources, 18 texture slots, states (50720) and the receiver rule (140850,
  5A120). Ordered D3D12 steps, checks D1–D6, risks (request families, unobserved states).
  Also: SSAO-NATIVE programme 189 is `OutputColor` (INFERRED from the m40 listing).

- **Part T, water surfaces ready to code** (`WATER-NATIVE.md`, M05). Native has no water: shader
  passes 4/12 are not selectable and there is no water-depth row, HDR copy, reflection or
  interaction map. OBSERVED at Firelink, named at m40 and found by position in 12 retained
  captures (absent from the five witness zones):
  - clear of the 640×360 RG16F interaction map to 0.5;
  - `WaterBumpTri_WatDpt` (underwater fog by water thickness, depth read);
  - HDR copy;
  - `WaterBumpTri_FwdSdw` (surface, depth write);
  - readable-depth copy.

  STATIC: the pass table index 12 = `WatDpt`, the 57A20 water-depth row `{0xF, 12, 0x107}`
  (view group 0x44), the DXBC interfaces (t32 shadow, t54 reflection, t55 refraction, t56 depth,
  t57 interaction, forward light lists) and the binder `1BF180` for t54–t56. Ordered D3D12
  steps, checks W1–W5, risks (t54–t57 provenance, surface row, reflection view, interaction).

- **Part U, spot, shadowed and line lights ready to code** (`SPOT-LINE-LIGHTS-NATIVE.md`).
  - Native: point lights done; spots counted and skipped (`game_bootstrap.cpp:2436`); shadowed
    and line lights absent.
  - OBSERVED (I-0063 inventory): `DSpotLight1..4` at m31/m37; `DSpotLightShadowed1`;
    `DPointLightShadowed1/2` at m31/m37/m40. Line lights and spot masks are never drawn.
  - STATIC chain: spot class `F1010` → fade `1F1C60` → the 0x280-byte item `2136B0`
    (transcribe, as `212810` was) → `214710` records, `2144C0` rows, hull binning → `1A6690`
    batches of 4 in `1A1DA0`'s tile loop (plain then shadowed tables, then `1A60F0`, INFERRED
    line lights).
  - Includes the consumer field table, the reuse of the host producers, the order, checks
    L1–L5 and risks.

- **Part V, the remaining families** (`REMAINING-FAMILIES.md`).
  - Character light probes: already specified by I-0067 TONEMAP-SKIN §10; the matrix had
    missed it.
  - SSR, AO fields, cloud shadows, velocity and motion blur: never drawn in any retained
    capture with Thomas's settings (MotionBlur DISABLE). Native draws nothing; the STATIC
    owners are listed for later.
  - Material variants (POM, interior windows, snow `_SnowLt`, crystal): ordinary G-buffer
    families with the M01 blocks plus family inputs. No new pass; verify the native gates (V1–V3).
  - WATER-NATIVE corrected with the confirmed rows: kind 0x27 → `{0x24, 6}` `WatDpt` and
    `{0x27, 0}` surface (`5A120` preparation → shader pass table).
  - Still open: the water reflection view, water interaction (D07), E08, and the line-light
    item producer.

- **Part W, water reflection, water interaction, E08** (`WATER-REFLECTION-INTERACTION-E08.md`).
  - Correction: m37 draws the water pair too (9261 indices).
  - OBSERVED reflection at m37: a 640×360 HDR + depth view, three draws (two map parts and an
    INFERRED sky dome), read at t54.
  - STATIC driver `FB0E0`: mirror by the plane `R+0x3F90`, view group 0x39 (no water rows, no
    decals) through `FCE30`/`103510`; plane chosen per main frame with a 100-frame hold.
  - Interaction: no RG16F write in any of 123 captures; the neutral 0.5 map is exact; the
    `GXWaterInteraction*` programmes and the manager are listed.
  - E08: no witness (only m30/m33/m38). It is shader-set indices 71–74 plus the light-shaft
    fields of `PSCommonBuffer`, on the E01/E02 machinery.
- **Part X, character-attached effects** (`CHARACTER-EFFECTS-MAP.md`). Native state and static
  owner of:
  - weapon and spell FFX, boss auras;
  - death fade (`ChrFadeOutDeleteSlot`, instance opacity) and phantoms (`9CAF30`);
  - status looks (SFX slots + ExParams `9CB270` wet / `9CAA80` damage from the character
    update `8C1BC0`);
  - character lights (effect lights).

  All depend on the NPC body path (MENUS-MAP §55) and the native effect system.

- **Part Y, character fade, ExParams and the reflection filter, ready to code**
  (`CHR-FADE-EXPARAM-REFLECTION.md`).
  - Both chains are CPU work in the character update that native already runs, and native
    already consumes their results for the player's parts.
  - Fade: `888C70` request, `ChrFadeOutDeleteSlot` update `5B1520`, `890950` integration, and
    the product `P` handed to the model `M+0x168`. Native reads it as group opacity
    `group+0x124` (`ReadModelFadeStateAt`, `SelectModelFade`, instance opacity).
  - ExParams: `8C1BC0` → `9CAA80` (damage) / `9CB270` (wet) → queue `9CAE60` → table
    `[item+0x6E8]+8` and wet words `entity+0x2CC`. Native readers and producers exist.
  - For all characters: wire `fade_owner`, `parameter_animations` and the wet group for NPC
    parts as for the player; checks F1–F3, X1–X3.
  - Reflection filter: the same scene query with mask low byte 0x10 (`|1` when option
    `+0x6E`), on the mirrored view; native runs it with `(policy & ~0xFF) | 0x10`; check R3.

- **Part Z, FFX attached to objects and characters** (`FFX-ATTACHED-NATIVE.md`; the High Wall
  torch).
  - `SfxHandle` (`1427682C0`) has three creation routes, all into `E6DCD0`/`E6DA30`, so all
    empty natively. Native also defers the `7CB820` route on purpose.
  - Object slots `ObjMultiSfxSlot` `65AB10`: all dummies of a reference id (`662D60`), creation
    `7CC4B0` → block route `7CB8F0`, follow by `7CC880` → `1A87F50` each frame.
  - One-shot dummy effects: `52EC30` (objects), `5292A0` (entities).
  - Character handles: `7CBEA0`/`7CBBA0` with the follow binding `2447C0`, moved by the effect
    system update.
  - Missing natively, in order:
    1. the effect member and its update (EFFECTS-PHASE1);
    2. replacing the deferral hook;
    3. E01 draws;
    4. effect lights;
    5. the bowl emissive family.
  - Checks T1–T6 at Argo's torch (T1 logs the caller of id 300 to settle slot versus one-shot)
    and F1–F3 at m40 (bonfire, held flame).

- **Part AA, emissive and glow chain ready to code** (`EMISSIVE-GLOW-NATIVE.md`).
  - OBSERVED in all five zones and m40, every frame: `EmissiveToGlowPS` (GB5 × alpha →
    1280×720 R11G11B10), `Sample13Weight` ×4 (¼ … 1/32), `GlowCompositeSinglePS` ×5 additive
    (the last into the final LDR target after DLAA).
  - Sources: the G-buffer Emi RT5, the forward `*Glow*_Fwd` second target (native binds only
    one), and FFX `GlowD`.
  - Gates: `GXGlowFilter` enable and the glow factor `[[GXRS+0x5368]+0x80]`.
  - Native: no chain, forward glow dropped. Includes the order, checks G1–G6 and risks (forward
    RT1 blend, blur taps).

- **Part AB.**
  - The two EMISSIVE-GLOW risks are resolved (§8):
    - every two-target forward/FFX draw uses IndependentBlendEnable = 0, so RT1 follows RT0
      (m31 and m45, 95 and 105 draws, four states);
    - `Sample13Weight` taps and weights come from `7F950` → `7FED0`/`80B50`/`80D10`: three
      13-tap tables (2D, horizontal ±k/6, vertical), Gaussian `exp(−r²/2)` normalised; offsets
      × scale × texel size (`cbFilterParam +16`).
  - **NATIVE-PASS-ORDER.md**: the native frame end to end (CPU → shadows → reflection →
    G-buffer → copies → decals → SSAO/shadow composite → env → lighting → forward → water →
    effects → YEBIS → after-chain/glow → UI → gamma), with dependencies per spec, shared
    resources and a ranked implementation order with checks.

- **Part AC: native gates for the material variants and the partial rows** (`NATIVE-GATES-AUDIT.md`, STATIC, code at `af038661`, `native/` unchanged since `2c4934bd`):
  - **`g_GlowScaleOffset`** per step (`1BB810`):
    - levels: `(param+0x14) × clamp(factor)`;
    - final composite: `(param+0x20) × clamp(factor)`;
    - `y = param+0x18`, unused by the shader.
    - Added to EMISSIVE-GLOW §8.3.
  - **POM (V1):** admitted (class 1, programmes resolve, b2 POM words written). Only the image check remains.
  - **`Interior` (V2) and the Emi map families:** refused.
    - `GXFT_Emissive`/`Emi` → class `0x100`, filtered from engine 5 (gate 21).
    - The engine-7 row needs model `+0x224` bit 15, which native produces only for character parts (`game_bootstrap.cpp:3873-3878`, `:3667-3671`).
    - This explains the dark High Wall bowl.
  - **`_SnowLt` (V3):** refused at `game_bootstrap.cpp:4498`. The snow is in the PS, which reads three unproduced words: b9 `FC_Snow*` (`23790`), b7 `FC_SnowRate` (`1A86C0`), b2 `FC_SnowMaskParam` (`22E930` flag +0x11).
  - **Crystal:** first gate, no NPC bodies; then class 2 forward with one target.
  - **COVERAGE-MATRIX:** rows 3–5, 7–9, 11–13, 20, 24, 28–30, 41, 44 and 47 updated with what is missing at file and line.
  - **Corrections to REMAINING-FAMILIES §3:**
    - the Interior/Crystal snp values are in b2, not b3;
    - the snow variant is a PS, not a VS.

- **Part AD: producers of model +0x220/+0x224, of the three snow words, and the Glow Filter mapping** (`PRODUCERS-UNION-SNOW-GLOW.md`, STATIC):
  - **Model union:**
    - `+0x220` is the OR of the instance classes (`142130` → instance `+0x18`, stored by `140F90`).
    - `+0x224` is the OR of the instance policy flags (template `+0x88` → instance `+0x24`).
    - Both agree with Reference1102.
    - The original writer was not found in the listing or by a raw scan (negative result recorded).
    - Ready to code: the native union for every model.
  - **Snow:**
    - b9 rows come from the gparam Snow bank `[GXRS+0x45F8]` (keys `frame+0x3FC0/+0x3FC8`, weight `+0x3FD0`, blend `131780`; field layout and built-in defaults from `12DCC0`).
    - `FC_SnowRate` = SnowModel Snow Rate (`[GXRS+0x4600]`, keys `group+0x70/+0x78`, weight `+0x80`) × Scene Snow Rate `[GXRS+0x4E70]`.
    - `FC_SnowMaskParam` = `{s, 0, −0.5s, 0}` from the FLVER GX list item `GX06` (`141070`).
  - **Glow Filter (STATIC):** `+0x10` BlurSampleScale, `+0x14` Blur RgbScale, `+0x18` Blur AlphaScale, `+0x1C` New Glow Enable, `+0x20` NewGlow Final ColorScale, `+0x24` Enable (bank `[GXRS+0x4658]`, `FB890` → `1BB6E0`).
  - **Open:** with the dumped m40 gparams, Enable resolves to 0 while the chain is observed.

- **Part AE: velocity and motion blur** (`VELOCITY-MOTIONBLUR-NATIVE.md`, STATIC; Thomas enabled the option):
  - **Gates:** option byte `[GXRS+0x5360]+0x20`, factor `[GXRS+0x5368]+0xE0`, pipeline velocity support `+0x3EB9`/`+0x3ED2`; gparam MotionBlur (bank `+0x4640`, key index 6) → `E4B50` → YEBIS (`E69A0`).
  - **Velocity pass `102380`:**
    - group 0x33: targets `pipeline+0xE20/+0xE30`; clear (0.5, 0.5, 1, 0); camera velocity `13BFC0` (`MotionBlurWriteCMBVelocity`, `g_mCurToPrevScreen`); object `Velo` rows;
    - group 0x35: dilation `13BD10`;
    - YEBIS blurs with the source `pipeline+0xDF0`.
  - **Velo VS/PS (DXBC):**
    - previous camera `VC_PrevMatrixWVP` at b0 +64 (corrects I-0067 VELOCITY-SKINNING on this point);
    - previous palette b6 indexed without the `matricesData` offset;
    - encoding `RG = Δndc·scale·0.5 + 0.5`, `B = w/(w+10)`; 8-bit target (INFERRED).
  - **Captures to take:** M1–M5.
  - **Upscaler preparation:** RG16F unjittered motion vectors, standard Z depth, jitter injection points and the passes to keep unjittered, render vs output resolution, history resets, reactive mask for forward and effects.
- **Glow inconsistency resolved:**
  - `mapbase.gparam` id 0 carries Glow (Enable 1, BlurSampleScale 1, RgbScale 1.5, New Glow 1 = mode B, Final 0.5);
  - glow key index 10 is set only by `88AF0` (view key), never by `D52B70`;
  - the per-file group table of every retained gparam copy is in PRODUCERS-UNION-SNOW-GLOW §3.3.
- **Part AF:** `EFFECTS-IMPLEMENTATION-CHECKLIST.md` merges PHASE1, E01–E07, FFX-ATTACHED and EMISSIVE-GLOW into one ordered checklist, with checks and witness zones per step.
- NATIVE-PASS-ORDER: lighting order CmbAlbedo → SSS → Composite (I-0067 §71); velocity step 11b.

- **Part AG: phantoms, summon signs and fog walls** (`PHANTOMS-SIGNS-FOGWALLS-NATIVE.md`, STATIC; coordinated with MENUS-MAP §41/§42/§47/§59/§72, not modified):
  - **Phantom shader (CORRECTED by I-0067 MENUS-MAP 75-76: only phantom-id models use the alternate set; the host player keeps `<family>_Fwd`):** every `Fwd`/`FwdSdw` draw of the 64 families uses `<shader>_Phantom_Fwd`. Native builds `<family>_Fwd` (`model_programs.cpp:48-51`, also on `claude/npc-enemies`), so even the hair forward uses the wrong PS today.
  - **Tint:** b2 rows 26–30 from the PhantomParam row of the resolver `88A950` (`rgb²·a`, subtract sign); no native producer yet.
  - **Routing:** opaque phantom meshes go to list 0x21, or 0x29 when the fade is below 1. Native has no 0x21 row.
  - **Outline:** shader-side rim; auras are character SFX.
  - **Summon signs:** `SosSignMan`; each sign is a `7CB820` SFX built in `853800` (id from a sign-type × variant table).
  - **Fog walls:** objects `o000401`/`o000400` with an object SFX at dummy 101 (common events `20005780`/`20005820`; m30 ids 3001800, 3001830, 3001890, 3001780).
  - **Signs and fog walls** arrive with the effects checklist (deferral hook).
  - **Witnesses:** checks S1–S2, P1–P4, F1–F2 on Argo's save (Ember, sign 1 at 40 m, Vordt alive). Red NPC rows are listed; yellow and purple are online-only.

- **Part AH: every open live check in one file** (`LIVE-CHECKS-CONSOLIDATED.md`):
  - **Scope:** about 140 rows from 20 notes, each with its source ID kept.
  - **Kinds:** NOW (settles a static open point today, on the original and/or native) and ACC (acceptance after an implementation step).
  - **Grouping:**
    - Argo's save, High Wall (start, Vordt's fog and sign 2, the torch, the arena);
    - TestAnim's save (Cemetery of Ash start, Firelink bonfire view, `c5110`);
    - the I-0063 zone route (m31, m37, m45, m50, m51);
    - situations outside a fixed spot (Bink, kills, hits, spells).
  - **Minimal plan:** four launches cover every NOW row.

- **Budget checkpoint (iteration 65 of 64):** the tranche closes here; `iterations_max` is extended to 96 for the next bounded plan, within the same scope. The scope is static specifications for the native D3D12 backend on coordinator request: the effects and velocity workstreams, follow-ups to the live checks of `LIVE-CHECKS-CONSOLIDATED.md`, and the open points each note lists (the model-union writer, the phantom group bytes, the sign SFX table).

- **Part AI: the velocity and motion-blur workstream as one checklist** (`VELOCITY-IMPLEMENTATION-CHECKLIST.md`), merging VELOCITY-MOTIONBLUR-NATIVE and I-0067 VELOCITY-SKINNING §1–8:
  - **Settled while merging:**
    - the `Velo` VS of the Bloody family also uses `VC_PrevMatrixWVP` (b0 +64), so the previous camera is bound per draw;
    - the passes run in order: 0x33 (clear and camera velocity), 0x34 (object rows: preparation 5, class 0xFFFF), 0x35 (dilation);
    - two option words (`[5360]+0x20` and `+0x21`) are mirrored.
  - **Steps:** gates, targets, previous camera, camera pass, palette ring and binding (raw b6 indexing, rigid `DC+0x198` open), object rows, dilation, YEBIS blur.
  - **Each step** has M/NOW/ACC checks and witnesses.
  - **Upscaler preparation** is kept apart.

- **Part AJ, point 1: the m40 gparam set** (PRODUCERS-UNION-SNOW-GLOW §3.3, STATIC + regulation). Resident files by kind (`MapBase` = kind 6, a map kind applied by the map step); per map the ten `m40_00_{i·100+base}` files from the Ceremony row (`+0x08`) and `MapOverride_0000` (`+0x0A` = 0 on every m40 row). The Glow key resolves to `MapBase` id 0 (INFERRED: registered under the map area). One live read remains: the area of the kind-6 registration.

- **Part AJ, point 2: the sign SFX table** (PHANTOMS-SIGNS-FOGWALLS §2, STATIC + regulation). The key `variant·10000 + category` is a RoleParam row id (param index 0x45, `E3C4A0`), and row `+0x30` is the sign SFX id: 30001 white (an NPC white sign reads role 1), 30011 red, 30021 gold, 30031, 30041/30051. Check S1 now expects id 30001.

- **Part AJ, point 3: the velocity scale** (VELOCITY-MOTIONBLUR §3, STATIC). In the `Velo` preparation (`5A120` case 5) `FC_DifColMul` = (aspect·0.5·k, −0.5·k, 0, 1), with aspect = `camera+0x64` or width/height and k = 1/`view+0xCA4` (1.0 by default); `RG = saturate(Δndc·FC_DifColMul.xy·0.5 + 0.5)`. This replaces the earlier INFERRED scale.

- **Part AJ, point 4: static points left to live.** The bounded static searches for the writer of `E+0x184`, the writer of `GXFlverModelDC+0x198`, the writer of the view block `+0x1130` (`cbTessScene`) and the registration area of `MapBase` ended with negative results. They are recorded with their searches in LIVE-CHECKS-CONSOLIDATED §6 (E184, DC198, TESS, MAPBASE). Only live points remain.

- **Part AK, point 1: E01 pipeline states OBSERVED** (EFFECTS-PHASE2 §6.1). From the five retained zone frames: rasteriser cull none / CCW / depth clip; blended draws test on, write off, LESS_EQUAL (the INFERRED is now OBSERVED); the opaque draw writes depth; five blend states with their target counts; strides 36, 112 and 160. Layout formats stay live-only.

- **Part AK, point 2: GX handle internals.** The effect-handle functions used by attached effects (`1A879A0`, `1A879F0`, `1A87CB0`, `1A87F50`, `1A88370`, `2447C0`, `4D2AC0`) and their callees, two levels deep, touch no GX device global (FFX-ATTACHED risk 2). The effect-light constant at `143D7FEF0` is (1, 1, 1, 1).

- **Part AK, point 3: the E01 sampler.** Observed on all 296 patch-list draws of the five zones: s9 is bound as linear filtering with WRAP addressing (not clamp, despite the register name `g_ClampLinearSampler`); native should bind the observed state.

- **Part AK, point 4: static completeness audit of the effects checklist** (EFFECTS-IMPLEMENTATION-CHECKLIST §11). Every step is classified S (settled), O (observed) or L (live-only). Verdict: the checklist can be executed from step 1; no step is blocked by a static-only question. Live-only items: the input-layout formats of the 8 E01 programmes (to read before step 3.3), the owner of id 300, and the E07 CounterDownSize dispatch site. They are added to LIVE-CHECKS-CONSOLIDATED §6.

- **Part AL, point 1: the NPC families, the gates that cut across them** (NPC-FAMILIES-STATIC §0-§2). Replaying the native material classification over the 72 character MTDs and the GX lists of the 125 `Data3` characters: (a) the translucent family is **deferred** (class 1/4, MTD blend 0, depth write on), not forward as MENUS-MAP 69/72.4 say; (b) the model union is produced only for ASM and menu parts, so enemy and NPC models never get the engine-7 row: 37 characters / 202 meshes of class `0x100` (emissive, cinder, Fur, SSS-emissive) are not drawn; (c) the GX item parser `141070` is not run: `FC_EmissiveColor` (GX03: 0.07 to 5), `FC_TranslucencyScale` (GX05) and `FC_ParallaxParam` (GX09, zero for the witness eye) stay zero, which also amends EMISSIVE-GLOW step 2 for map pieces; (d) tessellation is never selected (GX04 unread), so veils and displacement take the flat variants.

- **Part AL, point 2: W2 translucent and enemy SSS** (NPC-FAMILIES-STATIC §3). The translucent programmes are the default G-buffer programmes plus one instruction (`GB1.y = COLOR1.y x FC_TranslucencyScale x 0.1`, read back by the directional-light programmes as a back-lit term), same interface, deferred row 5; the only native gap is GX05 (value 1.0 on the witness). Enemy SSS is the player skin family: stencil bit 4 from `g_GX_MaterialType` 3 (already produced), GX15 profile on all 77 materials (six strength triples, far below the 0xE3 entries), `UseCAParam` true everywhere so the instance profile always applies; `FC_SSS_ID` is an index (parity is visual, not numeric).

- **Part AL, point 3: W3/W7 emissive and vertex offset** (NPC-FAMILIES-STATIC §4). Emissive families are class `0x100`, row 7, one extra texture (t6) and a sixth output `GB5 = t6 x FC_EmissiveColor`; the native six-target pass exists, so the only gaps are the union (enemy models) and GX03 (values 1.0 `c2150`, 5.0 `c1090`/`c5110`, 0.5 `c5270`). `FlowmapVertexOffset` (class 1, `c1090`, `c5110`) has two certain native refusals read in source: the vertex stage declares a texture (`model_textures.cpp:86` makes any non-pixel texture `Unsupported`) and reads `cbPerFrame`, which is produced for the pixel stage only (`game_bootstrap.cpp:4518`); the vertex programme offsets by `Texture2D_5(uv + time x snp0) x COLOR1.x x snp1`.

- **Part AL, point 4: W4 veil, W5 crystal, W6 eyes and cinder** (NPC-FAMILIES-STATIC §5-§6). The veil is class 2, forward row `0x29`, a 53-instruction no-light Fresnel programme (no lights, IBL, SSAO or depth texture); nothing specific is missing natively. The crystal programme samples **t45 = the scene colour copy** at screen UV plus a normal offset (that is what `GXFT_Recursive` selects): t45 is unbound in the native build (a certain refusal), and the original copies HDR at the water pass, after forward row `0x29`, so the copy the crystal sees is LIVE-ONLY. Eyes (class 1) need nothing: the GX09 parallax is zero for `c1400`. Cinder (class `0x100`) needs only the union.

- **Part AL, point 5: the live reads in one block per witness stop, and the other open families** (NPC-FAMILIES-STATIC §7-§9). Five static changes (C1 union for every model, C2 GX03/GX05, C3 vertex-stage texture and `cbPerFrame`, C4 scene colour at slot 45, C5 phantom programme by group byte `+0x87`) are settled and can be coded before any run; the live reads are grouped as block 0 (union words, refusal tally) and blocks W2, W3, W5, W6, W4, W7 with the expected value of each and the programme hashes to match draws in a capture. Other families: Ghost and Fur need nothing beyond the union; enemy SSS closed; tessellation never selected. **Correction of part AG:** the `Phantom_Fwd` parity point is withdrawn (MENUS-MAP 75): only a draw whose state byte `+0x87` is set uses the `_Phantom_` programmes, so the native `<family>_Fwd` is right for the player and every normal draw; PHANTOMS-SIGNS-FOGWALLS-NATIVE, COVERAGE-MATRIX row 10 and LIVE-CHECKS P4 are corrected.

- **Part AL, close: STATE row.** `.project/STATE.md` carries the summary of parts AJ-AL (NPC families W2-W7: what is settled, the corrections to MENUS-MAP 69/72.4 and to parts AG and AC/AD, the withdrawn `Phantom_Fwd` parity point, and the live reads grouped per witness stop in NPC-FAMILIES-STATIC §7).

- **Part AM, point 1: the velocity scale `view+0xCA4`** (VELOCITY-CA4-WRITER, STATIC). The view is a stack object of the scene function `140024740`, which stores `view+0xCA4` once: for a YEBIS view (`[144961F70] != 0` and pipeline flag bit 10) the MotionBlur global `144962158` (the gparam `MaxBlurLength`, 0.1 in every file that has a MotionBlur group, clamped at 0.001), otherwise `0.2 / max(pipeline+0x3E88, 1e-6)`. This explains the live 0.1 (k = 10, `FC_DifColMul` = (8.889, -5, 0, 1) at 16/9) and replaces the "default 1.0" of part AJ point 3 (1.0 is the constructor value only). The five 16-byte writers the velocity pilot listed are other classes.

- **Part AM, point 2: the FFX input layouts** (FFX-INPUT-LAYOUTS, STATIC). The shader-set builder `140291520` creates the layouts from static tables (`14473EBC0` pointers, `143D22320` counts; GX 28-byte element records converted by `14192F090`): 11 layouts serve the 158 VS, with formats, offsets and strides (112 tessellated billboards, 36 point sprite, 160 multi-texture, 80 tracers, 76 non-tessellated, 20 distortion, 32 radial blur, 80 light shaft), and no ISGN input of any VS is missing from its layout (0 mismatches). `Get-FfxInputLayouts.ps1`. The one item that had to be read before coding step 3.3 is closed.

- **Part AM, point 3: `CounterDownSize`** (LIVE-REDUCTION-AM section 3, STATIC). Every GX compute dispatch goes through `1400C5EB0` (`Dispatch`) or `1400C5F40` (`DispatchIndirect`); their callers are the four E07 sites plus non-FFX post blocks; no site selects a `CounterDownSize` programme. Nothing to implement.

- **Part AM, point 4: programme fingerprints** (PROGRAM-FINGERPRINTS, STATIC). SHA-256 and DXBC checksum of the 11,641 loose programmes, the FFX index map (VS, PS, GlowD twin, hull and domain), and compact tables for the effect, glow, velocity, shadow, light, decal and post families, so a captured draw is matched to a name without reflection. `Get-ProgramFingerprints.ps1`.

- **Part AM, point 5: the torch flame owner (T1)** (LIVE-REDUCTION-AM section 4, STATIC + regulation). The torch `o304863` takes its flame from `NS_SPRJ::ObjModelSfxSlot` (`14065A010`), fed by `ModelSfxParam` row 304863000 {effect 800023, dummy 200}; handles are created through `140E6EB70`, not `7CB820`. The nine id-300 and two id-320 deferred `7CB820` requests are event or param-row routes (the twelve callers are classified), not the torch. T1 is replaced by three precise reads.

- **Part AM, point 6: the gparam (area, id) rules** (LIVE-REDUCTION-AM section 5, STATIC). Kind-to-id table (`MapBase` registers with id 4), per-area registration of map kinds (`area = ((b3+100)*100+b2)*10000` from the map id), and the four-step lookup (exact, fallback (1, 0), built-in); MAPBASE reduces to one read of the two Glow keys.

- **Part AM, close.** Still live-only after this part: the view key (area, id) of the Glow filter; T1 a/b/c; CA4 1; the NPC reads of NPC-FAMILIES-STATIC section 7 (values of runtime state and the model union words); `E184`, `DC198`, `TESS`. LIVE-REDUCTION-AM is the index.

- **Part AN: the entry door of effects** (EFFECTS-ENTRY-DOOR-NATIVE, STATIC). Every effect created through `SprjSfxImp` ends in `E6DCD0`, `E6DA30` or `E6DBC0` (the only callers of the scene-control creation core `24ED90`, through `24EFE0` and `24F0F0`), each testing `[SprjSfxImp+0x58]`. Callers listed exactly: the handle API `7CB820`/`7CB8F0`/`7CB750`, the single and multi `SfxIns` classes (started lazily inside `E6E750`), and the cutscene factory; every source class (object, character, event, sign, bullet slots) is engine CPU code. `E6E750` is already called each frame by `SprjGraphicsStep::STEP_Update` `D5B140`. Native work: publish `+0x58`, delete the `7CB820` replacement, run the check matrix; per-zone expected ids and upper-bound counts come from the MSB and params (`Get-ZoneEffectSources.ps1`). FXR child spawns use `GXFfxManager` slots 5-7, not the door. Integrated in EFFECTS-IMPLEMENTATION-CHECKLIST as section 1b.

- **Part AO: the native YEBIS chain except motion blur** (YEBIS-CHAIN-CHECKLIST, STATIC + OBSERVED notes). One ordered checklist P1-P24: the library keeps all the arithmetic, the native build gives it a D3D12 HAL and the engine driver's inputs (I1-I11: scene HDR, readable depth, previous glare 640x540, DoF table 2048x2 R32F, afterimage history, 16^3 LUT, camera, gparam values, option factors, LDR surface), then the GX steps around it (LUT copy, scene copy, DoF table), the DLAA and glow after-chain and the gamma pass; motion blur is reused from I-0067 (Y1-Y16). Sizes are library-derived, not constants. gparam defaults and the id-0 values of seven zones (Tone Map, Bloom, light shafts, colour grading, Setting Info; `Get-ZoneYebisChain.ps1`), with the filmic constants `T` computed per zone and one correction to TONEMAP-SKIN section 16 (`T.x` for contrast 0.3 is 2.3985, not 2.4029). The converters' bank structs are decoded: Tone Map in file order (32 of 32 verified), Bloom in engine order, Setting Info by name (`Get-YebisGlobals.ps1`). Live-only: `LutSourceId` to texture, the exact grading and light-shaft gates (the id-0 rows do not predict the captured draws), the option factors.
- **Part AP: why the High Wall foliage is absent natively, by family** (FOLIAGE-NATIVE, STATIC + OBSERVED notes). At W2 two populations are missing: the Grass family (`M[ARSN]_e_grass` and its three siblings, the I-0063 "M04 Vegetation": 42 materials, 705,000 vertices in `m30_00`; 187 in `m31`, 29 in `m37`, 66 in `m40`) and the tree canopy `m009002`. The Grass family is refused by a bookkeeping rule: its vertex shader indexes `cbvModelParam` with a register index (wind arrays), the native read analysis then treats all 316 words as read, and `+0x70..+0x7F` and `+0x490..+0x4AF` have no producer (the compiler marks them unused) while `VC_ForceParam` is written only when `anim+0x620` is set; exactly 149 retained vertex shaders are affected, all Grass or `Eclipse_grass`. Correction F1: write those words as zero (counts 0, rest pose, static foliage). The wind block layout (`ForceParamST`, two loops of 8) and its original producer chain (`FUN_140D3C4D0` node registration, `FUN_140D3E950` per-frame evaluation, WindSFX regions) are decoded, so real wind follows the effects phase. The Edge family (`_l_e`) is drawn. The canopy `m009002` is geometry-joined natively and named by no refusal: five candidates and three reads (OPEN). Earlier notes that called this grass an E03 path were wrong for map pieces. Live-only: FOL1-FOL5 (LIVE-CHECKS-CONSOLIDATED §1.5).
- **Part AQ: the ground summon signs through the effects system, and the general native card by effect family** (EFFECT-FAMILIES-SIGNS-NATIVE, STATIC). A sign is three small effect layers: the controller (`30001`…, one `NodeSound` 30001 summon / 30011 invasion, and a proxy), the visible effect `3000x2` (LOD node, five E01 billboards [six on the red variants], one E04 distortion `NormalMap` `s06000`, one E06 point light) and the paired ground glow `3000x0` (two E01 billboards and a proxy to the same `3000x2`); no material, no model. Native has none of it: `7CB820` is deferred (`game_bootstrap.cpp:7514`, hook `:9394`) and returns the empty CPU handle, although `sfx_assets` already holds the FXR bytes; the missing work is exactly the effects checklist (1.1-1.4, 2.1, 3.1-3.3, 4, 6.1-6.2), no phantom programme is involved. New engine facts: the draw-context vtable `143D65688` read slot by slot (the blend setter is slot `+0x60`, `1402607D0`, a function Ghidra never defined, which hid it from every earlier register search), the FXR `BlendMode` → blend-type table `FUN_141ACABF0` (**`Unk0` is additive**; three of the five `30002` billboards use value 0), the resolver object (`143D66198`), the BC7/BC1 formats of the six textures (native already reads BC7). One point stays open: how the resolved blend type becomes `ctx+0x960` for classic draws (two ints of the resource object, SG2); native baseline = the table. Corrections: `FUN_1402607E0` serves the GPU-particle callbacks only; the controller is not silent; `30000` is a ground glow. Live-only: SG1-SG7 (LIVE-CHECKS-CONSOLIDATED §1.6).
- **Part AR: the enemy witnesses W3, W4 and W7 (emissive, vertex offset, veil, boss rows): what native refuses or does not produce, by static replay** (NPC-W347-NATIVE, STATIC). The three witnesses are five families on four models: `c2150` and `c1090` (W3; `EmiIblGlow`, `EmiIbl`, `FlowmapVertexOffset` plus the ordinary Bloody family), `c5270` (W4 boss; `EmiIblGlow`, ordinary and cloth Bloody, `Veill_NoLight` and `Veill_NoLight_cloth`, 353 bones, short bone indices) and `c5110` (W7 boss; `EmiIbl`, `FlowmapVertexOffset`, cloth). By the Grass method, eight native rules were replayed on the game data (programme selection for every pass, layout and pixel variant; register coverage of 336 programmes; word coverage against the default family; the generated `snp` parameter reader; the effect-admission declaration readers on all 113 SAT effects; texture bindings including the engine's builtin tables; pixel output counts against the six G-buffer targets; geometry limits): **no static refusal remains** after C1-C5 and the index-width fix. Native will not do (fidelity): glow extraction (EmiIblGlow is read by lighting as emissive but does not bloom), tessellation of the veil, forward shadows. What can still refuse is runtime state (capability masks, category and fade of a boss, cloth ownership, spawn). W3 as defined cannot work (neither model exists at that spot in the original either, event-gated); `c2150` at Firelink (`m40_00` (309.3, -35.4, 617), 50 m from W6) and `c1090` with `c2150` at `m30_01` are the alternatives. Predicted rows per model in §3.3; live rows NW1-NW7 (LIVE-CHECKS-CONSOLIDATED §1.7).
- **Part AS: where the bloom of the emissive comes from, what native lacks, and the correction by family** (GLOW-CHAIN-NATIVE, STATIC). The bloom is the engine's `GXGlowFilter` (ten draws after YEBIS and the anti-aliasing), fed by the alpha of the G-buffer emissive target GB5. New: the weight is chosen per shader family in the pixel programme. A G-buffer programme writes `o5.xyz = g_EmissiveTexture x FC_EmissiveColor` and `o5.w` = `FC_GlowScale.w` for the 56 Glow families (MTD `g_GlowScale`, builtin parameter 55) but 0 for the plain `EmiIbl` (unless an alpha-coverage flag): `c2150` and `c5270` (EmiIblGlow) bloom, `c1090` and `c5110` (EmiIbl, GX03 5.0) are bright and do not bloom, in the original too. Bank census of 1,311 multi-output pixel programmes: 212 G-buffer o5, 348 forward o1 (276 write `FC_GlowScale.xyz`), 44 FFX `GlowD`. Native has GB5 (RGBA16F, six targets, read by the light composite as t8) but no filter, no Glow record (no gparam file reaches the engine banks; `NativeGparamRecords` builds light and fog only), a single forward target; the factor `[[GXRS+0x5368]+0x80]` is slot 2 of the object `FUN_127010` builds, value 1. Correction: G-buffer Glow families need only the filter, forward Glow families need RT1 = GB5 with RT0's blend plus the filter, FFX twins need the effects phase, plain Emi need nothing; eight ordered steps. Corrects NPC-W347 §3. Live rows GL1-GL4 (LIVE-CHECKS-CONSOLIDATED §1.8).
- **Part AT: the tessellation of the veil and the shadows on forward rows: the original mechanism, what native lacks, correction by family** (TESS-SHADOW-FORWARD-NATIVE, STATIC). Tessellation: a material whose FLVER has a `GX04`/`GX90` mode gets PN-triangle variants (`_PN` mode 2, `_FT` mode 1, `_D` with displacement): the same skinned vertex shader, a hull (domain_tri, fractional_odd, 3 control points, factor cap 64; frustum and back-face patch culling, silhouette and distance adaptive factors from `cbPNTriangles` and `cbTessScene`), a domain shader evaluating the cubic patch, the unchanged pixel shader; `Velo` has no tessellated variant. Native draws the veil flat because it never parses `GX04` (mode stays 0) and because the model PSO has no hull/domain stage and only triangle-list topology; correction: GX04/GX90 parse, `_PN/_FT` selection, HS/DS PSO with 3-control-point patches, `cbPNTriangles` (formula known), `cbTessScene` (camera block, writer not found); five tessellated characters (c5270, c2230, c6060 `_PN`; c3140, c3141 `_FT_D`). Shadows: the forward pass is `Fwd`, `FwdSdw` or `FwdSSdw` by the view state `view+0x710` (0 none, 1 cascade atlas with in-shader PCF, 3 screen-space mask), set by the shadow layers' pass commands (`FUN_1401DD490`, `FUN_1401B8F30`); the world views use the shadowed variants (hair, water, OBSERVED), so SHADOWS-NATIVE's "no shadow input" is corrected. Of 664 forward pixel programmes 620 differ from `Fwd` (310 families); 19 unlit families are identical, **including `Veill_NoLight[_cloth]`: the c5270 veil receives no shadow**. Native has no shadow system, accepts only passes 0/1/6/7, and the dynamic `cb8[r+39..41]` cascade-matrix read would make its read analysis require the whole `cbSceneParam`; `Fwd` is the exact interim policy. Live rows TS1-TS4 (LIVE-CHECKS-CONSOLIDATED §1.9).
- **Part AU: the world shadow system completed: layers, state, windows, matrices, mask, constants, D3D12 design, ordered steps** (SHADOWS-NATIVE §6–§13, STATIC). The three shadow layers are `GXCascadedShadowScene` (`FUN_1401B6B80`, commands 0–4 cascade work, 5 sets `view+0x710 = 1`, 6 sets 0), `GXScreenSpaceShadow` (`FUN_1401B8F30`, 0 draws the mask by mode, 2 sets 3 and binds the mask as t32, 3 returns to 1) and `GXRsmScene` (`FUN_1401DD490`, a reflective shadow map scene, not on the world path); `view+0x710` has three writers and drives the forward pass (0 `Fwd`, 1 `FwdSdw`, 3 `FwdSSdw`). The recorder `FUN_140103510` clears the mask to white and records no window when the shadow gate is closed, and records cascade windows ([0,0x3C] or [0x17,..] and [0x1E,0x2E]) and screen-space windows ([2,3] or [0x17,0x1B] and [0x1E,0x21]) when open, so forward rows in the world are `FwdSdw`. cb8 and cbTessScene are exec-context images copied by `FUN_14013AA60`; no field-wise writer exists, so native builds them whole from camera and shadow state. D3D12 design under D-0157: an explicit per-view shadow table instead of command-order state, one whole cbSceneParam per view, retained static-caster depth instead of GX reprojection, shared mask descriptors; ER offers only architecture inspiration (no shadow notes). Ten ordered native steps with witnesses and live rows SH1–SH6.
- **Part AV: the native D3D12 design of the FFX effect system (particles, ribbons, distortion, effect lights, foliage wind)** (EFFECTS-D3D12-DESIGN-NATIVE, STATIC). The engine effect CPU code stays (door, SfxIns, FXR evaluation, emitters, wind and light hosts); under D-0157 the GX immediate-mode draw is split into a record phase (typed `EffectDrawRecord` per original draw, vertices and constants in a per-frame arena) and an encode phase (one `PassKind::Effect` per engine pass of the native frame plan), so the live draw counts (m37 about 68 patch-list draws, m31 5, m51 6) become checkable without a GPU. One root signature covers the 328 non-compute FFX programmes (measured: VS b0 b1 b9, DS b0, GS b9, PS b0 b1 b6 b9 b11, t0-t7, s0-s9); PSOs keyed by the resolved state tuple; depth bound as read-only DSV plus SRV (no depth copy); the HDR frame copy only when a record needs it (distortion appears in about 47 of 1,800 frames at m40); E07 on `ExecuteIndirect` with plan-derived barriers; glow twins as a two-target PSO variant. Foliage wind and effect lights need only the door and the library update, so they come first (steps N1-N4). Elden Ring contributes only its D3D12 submission architecture (typed operation records, barriers finalised before encoding, bookkeeping retired after the fence); its folder has no effect study. Fourteen ordered steps with witnesses per zone (m31 door and wind, m37 E01/E04, m51 E03/E05/lights, m50 lights/E05, m45 E07) and live rows FX1-FX13.
- **Part AW: the content of `cbTessScene` (TS1) and the format of the screen-space shadow mask (SH5) closed statically** (TESSSCENE-SSMASK-NATIVE, STATIC). `cbTessScene` is copied by a virtual setter, slot `+0x70` of `GXRenderExecContext` (`FUN_14013B100`, an undefined function earlier searches missed; 0xB4 bytes into `+0x1130`), and uploaded by `FUN_14013AA60`; the producer of the source block stays OPEN and is not needed by native. The 282 hull and domain programmes fix the content: the view-projection is the **absolute-world** one (the domain applies it to the world-space patch position with no camera subtraction, unlike the camera-relative `VC_MatrixViewProj`), the eye is absolute, the view vector points from the scene toward the camera, the four planes are inward world planes, `GUIParams2.x` scales the distance fade, `g_fGlobalTime` is read only by the displaced families. The mask is a four-channel 8-bit target (INFERRED `R8G8B8A8_UNORM` from the 1/255 and 1/255² packing): `w` = shadow term, `xyz` = packed depth for the blur, read by `FwdSSdw` as `.w` with `s8` = `SS_WrapLinear`; only the choice between the layer `+8` and `+0x10` objects stays a live read. Corrections to SHADOWS-NATIVE §10 and §11.
- **Part AX: preparation of phases A to C of the D3D12 design review** (`../I-0066-independent-native-startup/D3D12-PHASES-A-C-PREP.md`, STATIC, no native change). Nine reprises (R15; R4, R2, R3, R5; R6, R7, R9, R8a) with their native files and functions, dependencies, a 17-commit safest order (tokens first, arena, ingress, geometry, then PSOs, descriptors, root signature, persistent draws), the phase A measurement design (runs M0 to M4, scenes S1 to S3, a counter set) and live witnesses. Findings that refine the review: a single mutex `gpu_submissions` serialises the frame and every upload; pools already exist and return by refcount, not by fence; `-NoDebugLayer` and the pacing row already give most of phase A; the phase B exit check must exclude the per-frame waits (they stay until R1); the adapter ResourceBindingTier decides R6.
- **Part AY: preparation of phases D to G of the D3D12 design review** (`../I-0066-independent-native-startup/D3D12-PHASES-D-G-PREP.md`, STATIC, no native change). R1, R12, R14 and the shutdown order (D); R10 and R11 (E); R13 and R8b (F); R16 and the beyond-ER options (G), with files and functions, a commit order (slots at 1, explicit target sets, passes in one list, then the flip to 2 frames in flight, swap chain, shutdown; graph in shadow mode before it drives barriers; aliasing after a memory measurement; worker thread before the engine-side publication) and witnesses. Findings: one owner thread does collect, prepare, record and present, with a second menu thread; prepare still reads live engine memory at eight sites, so R13 cuts after prepare; the offscreen views swap sixteen renderer members and share one allocator, so frames in flight need explicit target sets first; `FramePlan` has no states or subresources and is used only by fixtures, against 84 hand-written transitions; the presenter is a 2-buffer flip chain with `Present(0,0)` and the original passes the engine interval; R8b needs a static study before any slice. The phase B exit check (no `Drain` in ingress, retirement and stream paths) was accepted by the coordinator and recorded.
- **Part AZ: Scaleform cacheAsBitmap and colour matrix, and the witness pages for the filters** (`SCALEFORM-CACHE-COLORMATRIX.md`, STATIC, no native change). The original draws a cacheAsBitmap primitive as a filter with no pass: the subtree into a temporary target, stored as cache state 2 by `FUN_141C64F00` (`FUN_141F7B930` end), then one textured quad (key `0x103`/`0x104`/`0x143`, VS 375, PS 1000-1002, `FUN_141F7A590`); the HAL render-buffer object (`HAL+0x288`, slot `+0x18`, constant true) keeps caches between frames, which native replaces by an explicit non-persistent policy. Two placements use it (the colour-select slider shapes, `04_030` and its `win_` twin). The colour matrix (kind 8) is unreachable: no placement, no ABC mention in the 102 menu movies or the 8 font movies; the HAL reserves it for feature level below 2. Text-field filters never reach the HAL (119 of 137 drop shadows and all 24 of the 7x7 ones are on text; the HUD text shadows are drawn natively with no kind-1 plan), so the large separable branch has no page; simple drop-shadow witnesses are the Options page and the character-creation and inventory lists. Native correction F8a (empty set branch in `RunFilterPasses`, cached quad with `CacheUniforms`), F8b (persistence through the engine store), F9 (completeness, synthetic test), witnesses CB1-CB3, DS1-DS3, LB7, CM1, TX1.
- **Part BA: the SSAO of the original and a D3D12 design for the frame plan** (`SSAO-D3D12-DESIGN-NATIVE.md`, STATIC, complements SSAO-NATIVE). Chain and shaders: depth downsample, a normal-oriented 8-pair horizon pass with a 64x64 rotation texture, a reprojection with neighbourhood clamp and depth-aware history rejection, a depth-guided bilateral upscale into the full-size `t33` (x = AO, yzw = linear depth packed in base 255). Parameters: the GParam group `SSAO` bound to the layer fields by `FUN_1401B3FB0` (names and offsets recovered); the five zones have no SSAO group of their own and share the `mapbase` set (Sample Offset 1.5, Scale 1.2, kernel 10/0.2/2, previous-frame 0.7, fade 100+200); the per-zone difference is `LightSet.SsaoWeight` on the lighting side. Gate (`FUN_1401B26E0`): quality flag +0x0A (HIGH and MAX), UseReProj or the MAX flag, record enable and strength; the chain does not run at LOW or MEDIUM. Native has only snapshot-driven constant producers, no layer, no previous-frame object, no passes. Design: `AoPass` in the frame plan, history as a ping-pong pair (no history copy draw), transient targets aliased, depth as read-only DSV+SRV, arena constants, one post-pass root signature, ER for architecture only; eleven steps, rows SS1-SS12.
- **Part BB: the whole YEBIS chain as a D3D12 design in the frame plan** (`YEBIS-D3D12-DESIGN-NATIVE.md`, STATIC, from YEBIS-CHAIN-CHECKLIST). Native has none of the chain (the in-game path ends in an sRGB encode of the lit HDR). Design under D-0157: the HAL device becomes a recorder of typed `YebisDrawRecord`s (library CPU code and draw order unchanged), compiled into batched barriers, aliased transient targets, bulk descriptor tables and one encode; eleven plan groups after the lit HDR (LUT copy, scene input, motion blur, DoF, glare/bloom, afterimage, light shafts, combine, tone map, grading, DoF table, then anti-aliasing, glow, UI, gamma/present), with AO before the lighting; three improvements by data flow (LDR ping-pong instead of the pre-AA copy, scene copy as an explicit pass, temporal resources with cross-frame edges); one post-pass root signature family; fit with phases A to G; fourteen steps B1-B14, witnesses on m31/m37/m45/m50/m51 and live rows YB1-YB16.
- **Part BC: one ordered plan for the D3D12 designs** (`D3D12-MASTER-PLAN.md`, STATIC, synthesis, no fiche rewritten). Three layers (phases A–G; shared pass machinery: post root signature family, arena block list, FramePlan draw graph; the seven families); one frame order (NATIVE-PASS-ORDER plus the groups of the designs); family slices that need only A–C (door/lights/wind, glow 1–3, shadow producers, tessellation parse and PSO, SSAO parameters, YEBIS skeleton with tone map and gamma, Scaleform F8a/F9) against those that need frames in flight and the plan with states; six waves after A–C; common witnesses (five zones, non-regression runs, digest rows C0–C5); five disagreements between notes with a proposed settlement (sampler model, pass cap, frame copy service, glow order, quality gates); cross-cutting risks.
- **Part BD: the five disagreements of the master plan settled statically** (`D3D12-MASTER-PLAN.md` §6, STATIC; fiches updated). Samplers: GX passes bind rows of the engine sampler bank (the AO prologue `FUN_1401B22E0` sets rows 0, 1, 3, 4, 6 as s0..s4; the DXBC names match), the YEBIS library has its own 16 states of which ten have no bank row, so the post root signature uses a sampler table with two sources and no static samplers. Pass cap: `CompileFramePlan` refuses more than 128 passes (a safety check costing microseconds); the frame is about 75 to 90 grouped passes and over 150 if draws are passes, so passes become groups, the cap becomes 256 and a peak is counted; plan constraints for history (initialized input, two identities for a ping-pong). Frame copy: the water refraction copy and the effects frame copy are one object (`pipeline+0x1108`); the YEBIS input is a clamped draw and the LDR copy is removed by the ping-pong. Glow order: constructor member order AA, glow, tone map, with the captures. Quality gates in one table; **DoF effective quality = min(zone value, quality+0x14)** (`FUN_1400E4070`), and `144961FB8/FBC` are GlareQuality and DepthOfFieldQuality, not the scene size (checklist P1 and YEBIS-NATIVE corrected). Live rows PL1, PL2, FC1, FC2, QG1, QG2, YB17, YB18, PC1.
- **Part BE: the native collect step, causes and reprises** (`COLLECT-NATIVE-REPRISES.md`, STATIC + OBSERVED counts; the original's E006A0 side is a separate note of the menus static thread). Phase A: collect is 104 ms of 166 (prepare 26, render 23, about 12 outside). Native collect rebuilds, per mesh and per resolved recipe, all shader constant blocks and texture selections every frame: S1 log gives 4,464 resolved recipes against 1,077 prepared rows (three quarters of the work thrown away), about 34,000 PrepareReadShaderRegister calls, 2,000 to 9,000 texture lookups through the engine path normaliser, dynamic constants and a 5-6 KB record per mesh rebuilt and destroyed. Guarded Read and mutexes ruled out. Reprises C0 timers, C1 produce what rows use, C2-C3 share scene/view/instance blocks, C4-C6 version caches, C7 persistent admission record, C8 clean-ups, C9 row cache, C10 overlap with render; frame scenarios 166 to about 100-115 (C1-C6), 75-90 (C7), 35-55 with phases B-D, 20-37 with F (estimates to re-base on C0). Workstream K in the master plan, parallel to B and C. Rows CL1-CL8.
- **Part BF: preparation of workstream K, reprises C0-C7** (`investigations/I-0066-independent-native-startup/COLLECT-K-PREP.md`, static). Per reprise the files, functions and lines (main `01de1ce8`), dependencies, slices, pitfalls and expected effect; commit order K0 timers, K1 digest and verify mode, K2 produce-at-row, K3a/K3b shared blocks, K4 texture cache with a registry generation, K5 dynamic cache, K6 material-block cache, K7 persistent record, each behind a flag file; measurement runs K-M0..K-M4 with expected counters per commit; non-regression by an in-process old/new digest plus the A-C pilots and five cache-exercising cases. Findings: block consumers read only the row's recipe; Programs objects are per layout so shared tables are keyed by programme-bytes identity; caches are keyed by a digest of the inputs (no engine version needed); prepare copies the blocks again (shared_ptr blocks remove it). C7 estimate corrected to 25-35 ms (12-25 with C8-C9). The original's E006A0 note is pending (questions Q1-Q4).
- **Part BG: preparation of wave 1 of the master plan, and integration of the original-side collect note** (`investigations/I-0066-independent-native-startup/WAVE1-PREP.md`, static; native tree `aedb93f7`, code maps by four read-only readers with key claims re-checked). Seven items with files, functions, lines, dependencies, commit order W0a-W10, measures and witnesses. Findings: the GX04 parse alone breaks the veil (gate 27, deformation refused) so it needs a flat fallback; shadow step 2 (cascade fit) has no native owner and casters (step 4) touch six hard-coded shader-pass gates, so they move to wave 3; glow RT1, tessellation and the shadow atlas all need one shared model-draw envelope change (render-target count 0/1/2/6, depth format, hull/domain, patch topology) which also touches phase C; there is no native colour path (UI goes straight to the back buffer, brightness computed and unread) so the gamma pass ships first; the YEBIS slice cannot be validated piecewise and Banks has no integer accessor; Scaleform F8a/F9 line-level edits and a missing unit scaffold. Original-side note COLLECT-ORIGINAL-STATIC integrated in COLLECT-K-PREP §8 (E006A0 is not a collect; values per draw, no layout analysis; the 12 ms outside split; new slice K3c layout cache; no whole-record reuse rule).
- **Part BH: the common model-draw envelope (commit W0a) ready to implement, with the GX04 flat fallback** (`investigations/I-0066-independent-native-startup/MODEL-DRAW-ENVELOPE-READY.md`, static, reviewed independently by an Opus 5.5 reader: no blocker, ten major and eight minor findings applied). Envelope enumeration (GBuffer6, Forward1, Forward2, Velocity1, DepthOnly) with one validator replacing seven count checks; tessellation as a second axis (root family Model / ModelTess with HS/DS tables, one descriptor pool with two cursors, patch topology); depth-only draws (zero targets, optional pixel shader, D16 depth, ClearDepth); POD PSO key struct shared with phase C's hashed key; every new field in one NativeModelDrawShape (the R8a split line); six reversible commits W0a-1..6 with tests T1-T9 built from Build-GameBootstrap.ps1 (gbuffer_encoder_test is not built by any script today; renderer/Test.ps1 writes to the retired I-0036 tree); alignment table with R6/R7/R8a/R9/R2; flat fallback: ShouldDisableDeformation pure function and DisableDeformation set unless native-tess.flag and the tess recipes resolve (the veil otherwise stops drawing at gate 27). Iteration budget extended 112 to 130 per WORKFLOW (tranche closed at part BG, same authorised static scope).
- **Part BI: the native colour path and the gamma pass, first and independent of YEBIS, ready to implement** (`investigations/I-0066-independent-native-startup/GAMMA-COLOUR-PATH-READY.md`, static, facts re-measured on the executable, reviewed independently by an Opus 5.5 reader: no blocker, 16 findings applied). The two original shaders are DXBC blobs in the loaded image (VS RVA 0x46BD950, 696 bytes; PS RVA 0x46BDC10, 1056 bytes; sizes and SHA-256 measured); the curve is produced by the engine's own generator FUN_1400C83D0 (callable from the module, 0x300C bytes interleaved, x = i/1024, exponent src/dst, dst = 2.2 + 0.5*b); the pass is a bit-exact 8-bit identity at brightness 5 (max deviation 0.1236 LSB, 0 of 256 mismatches), so the pass changes no pixel at the default brightness and gives a same-frame witness (intermediate equals back buffer). Design: a DisplayStage component (intermediate R8G8B8A8, private RTV and two-descriptor SRV heap, 1025x3 R32_FLOAT curve texture, ticket-retired staging) behind a generator callback and SetDisplaySource, wired into RenderNativeModels and RenderStartupFrame (UI into the intermediate, then the pass, then readback and Present), brightness handed over from Clear(); the legacy offline path cannot be reused (bindings_ has 90 entries, display_binding is 95). Five commits G0-1..5 (flag no-gamma.flag; dev aids brightness-override.flag and gamma-verify.flag), tests T1-T8 and live witnesses L1-L7.
- **Part BJ: the glow of wave 1 (G1-G3, G6) ready to implement** (`investigations/I-0066-independent-native-startup/GLOW-WAVE1-READY.md`, static, facts re-measured on the executable, reviewed independently by an Opus 5.5 reader: no blocker, 13 findings applied). Ten fixed passes of the mode-B filter (extraction, four halving blurs, three level composites, one onto the source, one onto the final colour) with targets, sources, blends, scales and a 20-transition barrier table; the four original programmes pinned by SHA-256 and DXBC hash; the 13 tap bit images and weights, the three constant-buffer images; the quad read from the executable (4 vertices, clip-space strip). Two corrections to earlier notes: the blur taps are one texel of the TARGET level (the viewport the blur reads is the render target's), and BlurSampleScale is a float. Design: GlowStage (own targets, root, four PSOs, quad, ticket-retired constants) recording into the 8-bit colour target before the UI, inputs by Banks::GroupWord/Banks::Glow (the 2227C0 blend, built-in fallback record) and CollectNativeGlowInputs (keys at context+0x4028/+0x4090, weight +0x40D0), forward rows Forward2 iff the pixel shader declares SV_Target1. Eight commits GL-1..8 (flags no-glow.flag, no-forward-gb5.flag; aids glow-verify.flag, glow-factor.flag), tests T1-T8, live rows GL1-GL8, witnesses by family.
- **Part BK: the SSAO parameters of wave 1 (S1, S2) ready to implement** (`investigations/I-0066-independent-native-startup/SSAO-WAVE1-READY.md`, static, facts re-measured on the executable, reviewed independently by an Opus 5.5 reader: no unresolved blocker, 12 findings applied). Resolved from the original: the 17-dword layer record (103510 -> 14015AF60 twice -> 140223780 blend -> 1400FEFA0; index-8 keys, the unblended tail 3800/200/4000, fDepthClipRange), two call sites of the setup (the world one looks up key A twice), the view record written by 1401B1960 (current-to-previous matrix, validity = inverse(V_cur)*V_prev diagonal >= 0.7071, strength [[GXRS+0x5368]+0xB0], frame counter), the rotation textures (the 16 images of 14007ECB0, already native as the dither images), the half depth produced by 1CB9D0, seven programmes pinned. Corrections: the AO pass runs from LOW (flags only choose temporal/upscale/normals), the previous-frame object has a writer and no reset factor. Design: AoParams/Banks::Ssao, AoView/UpdateAoView, a pure PlanAo, the four producers on native structures, AoPrograms, an inert AoStage; eight commits AO-1..8 (flags no-ao.flag, no-ao-resources.flag; aids ao-quality.flag, ao-verify.flag), tests T1-T10, rows SS13-SS17.
- **Part BL: YEBIS and the tone map of wave 1 ready to implement, with the global validation strategy** (`investigations/I-0066-independent-native-startup/YEBIS-WAVE1-READY.md`, static, facts re-measured on the executable, reviewed independently by an Opus 5.5 reader: no blocker for the pure commits, 9 findings applied). Resolved from FB890 lines 304-460: the engine feeds the library from nine gparam groups (key index map i = 0..10 shared with SSAO i = 8, glow i = 10 and motion blur i = 6; banks root+0x4608..0x4658), each with a lookup, a blend and a converter; the three first records are determined (Tone Map 35 words with B-copy {2,9,28,29,34}; Bloom 42 words in engine order = file words 0-40 permuted plus a record-only word 4, B-copy {8,9,13,27,31,33,36,39}; Setting Info 0x80-byte record with 18 integers and word 20 SceneLuminanceScale), goldens generated for default and seven zones (673 words), the converters callable on a scratch object, the world technique set pinned (26 observed, 56 candidate), GX programmes fingerprinted, E2B20 shown to be a GX-context-dependent open step (observation commit first). Corrections: the Bloom unnamed words, the T.x constants are data words, the colour-grading bank (index 4) is a prerequisite. Design: typed Banks::GroupEntries (GroupWord defined once in GL-2), yebis_blocks builders, CollectNativeYebisInputs, converters, recorder HAL, record-only host, GPU encode, output after the colour path; eleven commits YB-1..YB-10 + YB-4b/4c/6b (all flags opt-in), tests T1-T12, validation ladder V0-V5 by observable (blocks, globals, draw list on a recording HAL, scratch GPU encode, output, default).
- **Part BM: the effects of wave 1 by family, with the wind registry chain and the light-shaft and colour-grading feeds of YEBIS, ready to implement** (`investigations/I-0066-independent-native-startup/FFX-WAVE1-READY.md`, static, facts re-measured on the executable, the retained FXR archives and the gparam dumps, reviewed independently by an Opus 5.5 reader: four important and six minor findings applied). Resolved: slot 8's node-manager update `FUN_14025B870` dispatches a GX job as soon as one node exists (EFFECTS-PHASE1 §5 listed it as CALL), so it is transcribed inline; the wind feed is closed end to end (node, world bucket of the registry `[144962398]` through the world handle, collector `14025C700` with masks, box test and cancel-priority rule, entries created by the force-field appearances `GXForceField{Wind,Cancel,Gravity}AreaAppearance` / `GXCollisionFieldAreaAppearance` at their build, masks from the root action `0x2774`); the WindSFX FXRs read (one `WindForce` Box action each, an alpha-0 billboard in six of seven, `31001` has no force field, `m31_02` also uses `800081`); `FUN_140245F90` does not read the node box (persistent per-node fade state, no exact region cache); light shafts (YEBIS bank 9) = 28-word record with the ctor defaults of the record-only words 9 and 10, blend, converter and goldens for the seven zones (E08 is absent from the five zones); colour grading (bank 4) = 5-word block, blend table, globals `144962228..238`, LUT name `<prefix>%04d`, textures read by `FUN_1400E5110` at `+0x358/+0x360` (the producer of that argument is the one open link). Design: seven families (objects and map, characters and events, effect lights, force fields and wind, light shafts, colour grading, draw families), nine commits FW-1..FW-9 (all flags opt-in), nine tests T-F1..T-F9, sixteen live rows FF1..FF16.
- **Part BN: the shadows of wave 1 (cascades, local-light shadows, filtering) ready to implement** (`investigations/I-0066-independent-native-startup/SHADOWS-WAVE1-READY.md`, static, reviewed independently by an Opus 5.5 reader: two blocking and six important findings applied). Resolved: every world zone uses the same ShadowParam record (`mapbase` id 0, FilterMode 2, class 3); the atlas edge formula and the five quality rows (1024 / 1024 / 2048 / 4096 / 8192 for class 3, blur count `max(0, Blur Count + blurAdd)`); goldens of the record, the state words and the cbSceneParam shadow words for atlas edges 2048 / 4096 / 8192; the fit (`140239610`), culling volume (`14023C1E0`) and matrix assembly (`1402388B0`) touch no GX object, while `23A0C0`, `1B5720` and `238680` do and are re-expressed; the local-light chain decoded from the gate (`FUN_14010A220`) through the frame items (`212810`/`2136B0`), slot 3, the quad-tree allocator and the 0x1850 items to the per-light recording (pass `i + 9`, 48 items), with the container already built by `PublishNativeSceneRoot`; the observed local tiles are spots; the filter order PCF4 / PCF9 / Soft = 0 / 1 / 2 and the blur gate. Design: nine commits SW-1..SW-9 (all flags opt-in), nine tests T-W1..T-W9, sixteen live rows SL1..SL16 (LIVE-CHECKS §1.14). Open: the cascade-fit input records and the sun-direction producer (SL4), the pipeline gate fields natively (SL1), the sources of the per-light scalars (SL9), the bias conversion (SL3).
- **Part BO: water, refraction/reflection and tessellation of wave 1 ready to implement** (`investigations/I-0066-independent-native-startup/WATER-TESS-WAVE1-READY.md`, static, reviewed independently by an Opus 5.5 reader: two blocking and five important findings applied). Resolved: the five zones contain water of one family (`GXFlver_WaterBumpTri`: m31 3 pieces, m37 6, m45 4, m50 3, m51 2) and the retained views draw it only at m37 (and Firelink/m39 outside the five), which corrects the "water at m31" citations; the frame renderer binds t54..t57 with black and grey 1x1 defaults (the grey equals the neutral interaction map, so no 640x360 target is needed); the frame copy is one resource copied twice with water (after `WatDpt`, then after the surface) and once without; the reflection camera is the plane mirror composed with a Y flip (determinant +1, winding kept), gathered with mask 0x10 over the MSB `DrawByReflectCam` parts, with the plane reset every frame and set by the nearest water mesh or a height default; the only tessellated map witness of the five zones is the m50 tree piece `m001523` through `GBuf_PN` (GX04 mode 2, factors 5/1), the only tessellated character witness `c6060` at m45, and the FFX effects draw 1-control-point patches in every zone but no retained capture draws a model patch; `cbPNTriangles` is transcribed with goldens; POM is a pixel-shader technique already admitted. Design: eight water commits WT-1..WT-8 and six tessellation commits TS-1..TS-6, twelve tests T-Q1..T-Q12, fourteen live rows WL1..WL14 (LIVE-CHECKS §1.15). Open: the meaning of the plane scalar `t` and the order of use, the reflection rows and mask-to-flag mapping, the recorder of the first copy, GB5 and the frame-copy alpha around the water, the `c6060` factors.
- **Part BP: the other YEBIS passes of wave 1 (depth of field, motion blur, feedback blur, auto exposure, history) ready to implement** (`investigations/I-0066-independent-native-startup/YEBIS-SECOND-TRANCHE-READY.md`, static, reviewed independently by an Opus 5.5 reader: one blocking and four important findings applied). Audit: glare, light shafts, colour grading, tone map and Setting Info already had ready sheets; depth of field, the motion-blur builder, feedback blur and auto exposure did not, so the DLSS sheet is not next. Resolved: the 34-word DoF record (word 12 = the bool `Enable`, word 33 = `BackgroundMaskThreshold`), its blend sets and the three driver overrides (words 2, 12, 24, correcting "word 3"); the 21-word MotionBlur record with two non-serialised words and the unwritten word 12 of the in-between blend; the zones feed the `mapbase` record for both groups (no zone file carries them); feedback blur is off in all 18 files; auto exposure is on in `MapOverride_0001/0003` (correcting "off in every file") with the stored-exposure blend table and the CPU read-back of exposure and focus; the engine never writes the history reset word. Design: five commits YB-4b.1..YB-4b.5, tests T2d..T2h, twelve live rows YP1..YP12 (LIVE-CHECKS §1.16). Open: which record the pipeline key selects (YP1), the scripted-focus producer, the feedback word order, the users of the two override files.
- **Part BQ: the temporal upscaler (DLSS) ready to implement** (`investigations/I-0066-independent-native-startup/UPSCALER-DLSS-READY.md`, static, reviewed independently by an Opus 5.5 reader: two blocking and six important findings applied). Engine facts: one extent for the whole frame today (61 reads of `presenter_`); the projection is already off-centre and one matrix feeds every `cbSceneParam` inverse, so jitter has a single entry point; the 2,206 pixel programmes that read `cbSceneParam` need no change (they read the inverses, none the off-centre rows of `FC_MatrixProj`); the rasteriser matrices outside `cbSceneParam` are listed (FFX, ocean, AO fields, env map region, particle compute, light tiles) with `GXSsr`'s reprojection. Resolved: a constant-NDC-shift derivation gives three reprojection matrices (SSAO and `GXSsr` jittered pair, motion blur and the upscaler's vectors with the shift added back, the `Velo` b0 block with the shift on both matrices), correcting "keep temporal reprojection unjittered"; the upscaler sits on the lit HDR between slot 12 and YEBIS (output-size YEBIS, glow, UI, gamma, a nearest-depth upsample for the depth-reading YEBIS stages); reactive mask as stencil bit 0x40 plus one resolve draw; a `HistoryEpoch` counter with six sources. Design: ten commits UP-1..UP-10 (neutral interface and bilinear stub first, direct NGX adapter last), tests T-U1..T-U13 with goldens, live rows UL1..UL14 (LIVE-CHECKS §1.17). The NVIDIA SDK contract is read from the public headers and labelled external. Open: the vector and jitter sign conventions, the SDK route and licence (decisions for Thomas), what `s8`/`s9` sample, the writer of stencil bit 0x10.

## Evidence

- Exports in `decompiled/` (listed at the end of each note).
- Query outputs in `evidence/I-0068/queries/{ffx,yebis,display}/`, `yebis/techniques/`.
- Light families computed by `Get-EffectLightFamilies.ps1`, `Get-ProgramLayouts.ps1`, `Get-ShaderSetTable.ps1`, `Get-YebisTechniqueTraits.ps1`, `Get-YebisDrawSequence.ps1`, `Get-ZoneEffectFamilies.ps1`, `Get-BinkMovieHeaders.ps1`.
- YEBIS method strings extracted from the executable (`yebis/yebis-method-strings.tsv`: string
  VA = file offset + `0x140001A00` for that section; corrected in part F).

## Uncertainty / unresolved questions

- Effects:
  - which engine path calls SceneCtrl slot 8 each frame (runtime check R4);
  - the four direct D3D11 FFX sites: constants closed in FFX-FAMILIES-PHASE2 §6.1 (shared blocks); the t0 frame-buffer input of E04/E05 is closed in §8.
- YEBIS:
  - the technique of each draw is closed in part F, except:
    - the exact blur tap counts per zone (one numeric factor, YEBIS-TECHNIQUES §8.3);
    - the DoF composite's USNORM suffix (edge-quality default).
  - The bloom arithmetic and the creator slot names are closed in part F. The other constant
    builders are not decoded (library internal; not needed with the HAL route).
- Display: `FUN_140EF61A0` (encrypted); the brightness option identity of byte `+0xA` (INFERRED from range and default).

## Artifacts produced or modified

- `EFFECT-LIGHTS.md`, `YEBIS-NATIVE.md`, `FFX-SCENECTRL-NATIVE.md`, `FFX-FAMILIES-PHASE2.md`, `DISPLAY-SWAPCHAIN-NATIVE.md`, `YEBIS-TECHNIQUES.md`, `EFFECTS-PHASE1-NATIVE.md`, `EFFECTS-PHASE2-E01-E02-NATIVE.md`, `YEBIS-HAL-READY.md`, `BINK-NATIVE-READY.md`, `EFFECTS-E03-MODEL-PARTICLES-NATIVE.md`, `EFFECTS-E05-E04-NATIVE.md`, `EFFECTS-E07-GPU-PARTICLES-NATIVE.md`, `FORWARD-DEPTH-NATIVE.md`, `SHADOWS-NATIVE.md`, `SHADOWS-NATIVE.md`, `SSAO-NATIVE.md`, `COVERAGE-MATRIX.md`.
- Part AN: `EFFECTS-ENTRY-DOOR-NATIVE.md`, `Get-ZoneEffectSources.ps1`.
- Part AO: `YEBIS-CHAIN-CHECKLIST.md`, `Get-ZoneYebisChain.ps1`, `Get-YebisGlobals.ps1`.
- Part AP: `FOLIAGE-NATIVE.md`, `Read-Dxbc.ps1`, `Scan-MapFlver.ps1`, `Project-Foliage.ps1`.
- Part AQ: `EFFECT-FAMILIES-SIGNS-NATIVE.md`, `Read-Fxr.ps1` + `FxrDump.cs`, `I0068Dis.java`.
- Part AR: `NPC-W347-NATIVE.md`, `Scan-ChrFlver.ps1`, `Scan-ProgramReads.ps1`, `Check-GeneratedParameters.ps1`, `Check-SpxAdmission.ps1`, `I0068Table.java`.
- Part AS: `GLOW-CHAIN-NATIVE.md`, `Scan-GlowOutputs.ps1`, `I0068Table.java` (extended).
- Part AT: `TESS-SHADOW-FORWARD-NATIVE.md`.
- Part AU: `SHADOWS-NATIVE.md` (§6–§13).
- Part AV: `EFFECTS-D3D12-DESIGN-NATIVE.md`.
- Part AW: `TESSSCENE-SSMASK-NATIVE.md`, `Scan-TessScene.ps1`.
- Part AX: `../I-0066-independent-native-startup/D3D12-PHASES-A-C-PREP.md` (filed in I-0066).
- Part AY: `../I-0066-independent-native-startup/D3D12-PHASES-D-G-PREP.md` (filed in I-0066).
- Part AZ: `SCALEFORM-CACHE-COLORMATRIX.md`, `Scan-GfxFilterContext.ps1`.
- Part BA: `SSAO-D3D12-DESIGN-NATIVE.md`.
- Part BB: `YEBIS-D3D12-DESIGN-NATIVE.md`.
- Part BC: `D3D12-MASTER-PLAN.md`.
- Part BD: `D3D12-MASTER-PLAN.md` §6 (settlements), corrections in YEBIS-CHAIN-CHECKLIST, YEBIS-NATIVE, YEBIS-D3D12-DESIGN-NATIVE §9, SSAO-D3D12-DESIGN-NATIVE §4.4, GLOW-CHAIN-NATIVE, EFFECTS-D3D12-DESIGN-NATIVE §2.4; evidence `evidence/I-0068/queries/samplers/`.
- Part BE: `COLLECT-NATIVE-REPRISES.md`; master plan workstream K; pointers in D3D12-DESIGN-REVIEW and D3D12-PHASES-A-C-PREP; evidence `evidence/I-0068/queries/collect/`.
- Part BF: `../I-0066-independent-native-startup/COLLECT-K-PREP.md`.
- Part BG: `../I-0066-independent-native-startup/WAVE1-PREP.md`; COLLECT-K-PREP §8 and K3c, COLLECT-NATIVE-REPRISES update, master plan wave 1 refined.
- Part BH: `../I-0066-independent-native-startup/MODEL-DRAW-ENVELOPE-READY.md`; WAVE1-PREP corrected (model_programs lines, gbuffer_encoder_test status).
- Part BI: `../I-0066-independent-native-startup/GAMMA-COLOUR-PATH-READY.md`; WAVE1-PREP item 6 linked; evidence `evidence/I-0068/queries/gamma-ready/`.
- Part BJ: `../I-0066-independent-native-startup/GLOW-WAVE1-READY.md`; WAVE1-PREP item 2, the master plan and D3D12-DESIGN-REVIEW linked; corrections in EMISSIVE-GLOW-NATIVE §8.2 and GLOW-CHAIN-NATIVE §1; live rows GL5-GL8; evidence `evidence/I-0068/queries/glow-ready/`.
- Part BK: `../I-0066-independent-native-startup/SSAO-WAVE1-READY.md`; WAVE1-PREP item 5, the master plan and D3D12-DESIGN-REVIEW linked; corrections in SSAO-D3D12-DESIGN-NATIVE; evidence `evidence/I-0068/queries/ssao-ready/`.
- Part BL: `../I-0066-independent-native-startup/YEBIS-WAVE1-READY.md`; scripts `Get-YebisBankBlocks.ps1`, `Get-YebisConverterWords.ps1`, `Get-YebisWorldTechniques.ps1`; WAVE1-PREP item 6, the master plan and D3D12-DESIGN-REVIEW linked; corrections in YEBIS-CHAIN-CHECKLIST §8; evidence `evidence/I-0068/queries/yebis-ready/`.
- Part BM: `../I-0066-independent-native-startup/FFX-WAVE1-READY.md`; scripts `Get-FfxBankBlocks.ps1`, `Get-ZoneForceFields.ps1`; WAVE1-PREP item 1, the master plan and D3D12-DESIGN-REVIEW linked; corrections in EFFECTS-PHASE1 §5, WIND-BLOCK open point 1, EFFECTS design §2.6/N14, YEBIS-WAVE1-READY (§1.4, YB-4b/4c); live rows FF1-FF16 (LIVE-CHECKS §1.13); evidence `evidence/I-0068/queries/ffx-ready/` and `yebis-ready/b3..b17`.
- Part BN: `../I-0066-independent-native-startup/SHADOWS-WAVE1-READY.md`; script `Get-ShadowWorldBlocks.ps1`; WAVE1-PREP item 3, the master plan, D3D12-DESIGN-REVIEW and SHADOWS-NATIVE linked; live rows SL1-SL16 (LIVE-CHECKS §1.14); evidence `evidence/I-0068/queries/shadow-ready/`.
- Part BO: `../I-0066-independent-native-startup/WATER-TESS-WAVE1-READY.md`; scripts `Get-ZoneWaterTess.ps1`, `Get-ZoneReflectParts.ps1`, `Get-WaterTessBlocks.ps1`, `Recover-NamedGameData.ps1`; WAVE1-PREP, the master plan, D3D12-DESIGN-REVIEW and WATER-NATIVE linked; live rows WL1-WL14 (LIVE-CHECKS §1.15); evidence `evidence/I-0068/queries/water-ready/`.
- Part BP: `../I-0066-independent-native-startup/YEBIS-SECOND-TRANCHE-READY.md`; script `Get-YebisSecondBlocks.ps1`; YEBIS-WAVE1-READY, the master plan, D3D12-DESIGN-REVIEW and YEBIS-CHAIN-CHECKLIST linked; live rows YP1-YP12 (LIVE-CHECKS §1.16); evidence `evidence/I-0068/queries/yebis-second/`.
- Part BQ: `../I-0066-independent-native-startup/UPSCALER-DLSS-READY.md`; scripts `Scan-SceneParamReaders.ps1`, `Scan-ProjectionVars.ps1`, `Get-UpscalerGoldens.ps1`; the master plan, D3D12-DESIGN-REVIEW, VELOCITY-MOTIONBLUR-NATIVE and the velocity checklist linked; live rows UL1-UL14 (LIVE-CHECKS §1.17); evidence `evidence/I-0068/queries/upscaler-ready/`.
- Part AM: `VELOCITY-CA4-WRITER.md`, `FFX-INPUT-LAYOUTS.md`, `LIVE-REDUCTION-AM.md`, `PROGRAM-FINGERPRINTS.md`, `Get-FfxInputLayouts.ps1`, `Get-ProgramFingerprints.ps1`, `I0068Grep.java`.
- `Invoke-Map.ps1` (private database copy), `I0068Find.java`, `Export-Decompiled.ps1`,
  `Get-EffectLightFamilies.ps1`, `Get-ProgramLayouts.ps1`, `Get-ShaderSetTable.ps1`, `Get-YebisTechniqueTraits.ps1`, `Get-YebisDrawSequence.ps1`, `Get-ZoneEffectFamilies.ps1`, `Get-BinkMovieHeaders.ps1`.

## Suggested next step

Pilot:

1. R1, then the native effect system of EFFECT-LIGHTS §2.5 (CPU parts first; lights need no
   draw pass).
2. For world post-processing, the native YEBIS HAL of YEBIS-NATIVE §5–§6, created at scene setup
   with a native context, the native scene HDR as input and the native LDR target as default
   surface.
