# The common model-draw envelope (commit W0a): variable target sets, hull/domain stages, depth-only draws, and the flat fallback of the GX04 parse (ready to implement, static)

Request (coordinator, 2026-10-11): an implementation-ready note for the common envelope of the model draws. It must cover variable render-target sets, hull/domain stages and D16 depth without a colour target, align with phase C (descriptors and persistent draws, root-signature families), and carry the flat fallback of the GX04 parse for the veil. To be merged after an independent Opus 5.5 review (the review log is §11). Static; nothing was built or run, no native code was changed. This is commit **W0a** of [WAVE1-PREP](WAVE1-PREP.md) §4.2.

Labels: **observed** = read in the native tree at main `cb20045e` (line numbers are that tree's and will move); **note** = a statement of a linked design note; **proposed** = this note's design; **open** = needs a read or a measurement first. Abbreviations: `MD` = `native/renderer/model_draw.cpp`, `MH` = `model_draw.h`, `GE` = `gbuffer_encoder.h`, `SP` = `scene_pass.cpp`, `GB` = `native/frame/game_bootstrap.cpp`.

## 0. Answer first

1. **One envelope descriptor replaces seven scattered count checks.** Today the target set of a model draw is implied by `render_target_count` (6 or 1) and checked in seven places (`MD:270`, `MD:322`, and five in `SP:1362-1366`, `SP:1551-1562`); it cannot express two targets (forward + GB5), zero targets (depth only) or a different depth format (D16 shadow atlas). The proposal is a small enumeration `ModelPassEnvelope {GBuffer6, Forward1, Forward2, Velocity1, DepthOnly}` with one specification table (count, whether a pixel shader is required), checked by one free function, `EnvelopeMatches`, declared in `model_draw.h` so the scene and the unit tests share it. Producers keep setting `render_target_count` and the formats exactly as today and additionally set the envelope; `Prepare` requires the two to agree (a forgotten envelope fails at preparation, not as a 6-target PSO thrown at the scene check).
2. **Tessellation is a second axis, not a sixth envelope.** The target set (what is written) and the stage family (which stages run: VS+PS, or VS+HS+DS+PS) are orthogonal: the veil is a forward draw with tessellation, a flow-displacement mesh a G-buffer draw. So the draw input gains a `ModelRootFamily {Model, ModelTess}` and optional hull/domain programmes; a draw selects its root signature by family, and **geometry shaders stay denied** (phase C's R7 pitfall keeps hull/domain denied until the tessellated families add their own signature: this is that signature).
3. **Depth-only draws need three relaxations, all small**: `BindGBufferDraw` accepts an empty target span when a depth handle is set (`GE:41`); `OwnedNativeModelDraw::Prepare` accepts `NumRenderTargets=0` and an optional pixel shader for the opaque depth-only envelope (`MD:244-245`, `MD:270`, `MD:283-286`); a `ClearDepth` helper beside `ClearGBuffer`. The D16 depth format is already part of the PSO key (`depth_format`, `MD:279`); creating the atlas resource and its DSV is slice S3a, not W0a.
4. **Phase C alignment is by construction**: every **new** field sits in one nested `NativeModelDrawShape` (envelope, family, topology class, hull and domain programmes); the immutable part R8a keeps across frames is that shape plus the fields that already exist for the same role (the two other programmes, formats, depth format, fixed-function state, elements), the constants and streams being the per-frame part; the PSO key is built from one POD `ModelPsoKeyFields` that R9's hashed key will serialise instead of today's byte string; the per-draw **resource** range is a per-family function so R6 can replace the 64-draw chunk pool (R6 moves samplers to stable bank indices, so it keeps only the resource-range size), and R7's per-shape root signatures extend the family key (§6).
5. **The flat fallback of the GX04 parse is already half-built**: `NativeModelPassFlag::DisableDeformation` exists (`content/model_pass.h:30`) and `EffectiveOrdinaryModelDeformation` returns 0 when it is set (`content/model_pass.cpp:16-18`), which yields the flat programme intent and keeps the veil out of gate 27; nothing sets it today. §7 sets it for every mesh unless the native tessellation switch is on **and** the tess recipes resolved, with the exact edit, the unit test and the ordering constraint with T1.
6. **Six commits, each reversible** (§5): W0a-1 the inert types (all three enumerations, the shape, the key struct), the counters and the test scaffold (no behaviour change); W0a-2 the single validator and the producers' envelope; W0a-3 depth-only draws; W0a-4 two-target forward acceptance; W0a-5 the tessellation family; W0a-6 (with the GX04 parse, W5e) the flat fallback. A new unit-test executable for the envelope is part of W0a-1 (there is none today: `OwnedNativeModelDraw` is only exercised live, and `renderer/gbuffer_encoder_test.cpp` is not built by any script; both are built from `Build-GameBootstrap.ps1`, whose output is outside the retired I-0036 evidence tree).

## 1. What the code does today (observed)

| Region | Lines | What |
|---|---|---|
| input | `MH:37-55` | `NativeModelDrawInput`: `vertex_shader`, `pixel_shader` (both required), geometry, elements, fixed-function state, `render_target_formats` (array of 6), `render_target_count{6}` ("6: the G-buffer envelope; 1: the forward pass"), `depth_format`, constants, SRVs, samplers, owners |
| owner | `MH:64-94` | `OwnedNativeModelDraw`: `root_`, `pipeline_`, `resources_/samplers_` heaps, `constants_[28]` (14 CBV × 2 stages), `heaps_[2]`, `tables_[4]`, `streams_[2]`, `pooled_` |
| stage index | `MD:20-23` | `Stage()` maps Vertex → 0, Pixel → 1 and refuses any other stage |
| validation | `MD:81-119` | `ValidateBindings` takes exactly two interfaces; arrays `[2][cbv_count/srv_count/sampler_count]`; records the root shape `{VS cbv, srv, sampler, PS cbv, srv, sampler}` (6 numbers) in `NativeModelRootShapes()` |
| caches | `MD:130-149` | per-device `ModelDrawCaches` under one recursive mutex: one root signature, null templates, `pipelines` keyed by a byte string, programs by fingerprint, constant chunks (8 MB), table chunks (64 draws) |
| root signature | `MD:190-203` | 4 parameters: table 0 = VS (14 CBV + 128 SRV), table 1 = PS (same), tables 2/3 = VS/PS samplers (16); flags `ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | DENY_HULL | DENY_DOMAIN | DENY_GEOMETRY` |
| table chunk | `MD:131`, `MD:171-183` | `table_chunk_draws=64`: `stage_resources*2*64` CBV/SRV descriptors and `sampler_count*2*64 = 2048` samplers (the shader-visible sampler heap limit) |
| Prepare | `MD:243-316` | requires VS and PS (`:244-245`), validates, builds the input layout, `Require(render_target_count==6 \|\| ==1)` (`:270`), builds the byte-string PSO key (`:272-279`), creates the PSO with `VS`, `PS`, `PrimitiveTopologyType=TRIANGLE`, `NumRenderTargets=input_.render_target_count`, RTV and DSV formats (`:282-287`), fills the null template (`:295-298`), copies constants/SRVs/samplers (`:299-312`) |
| Describe/Encode | `MD:320-330` | `Require(targets.size()==render_target_count)`; topology fixed `TRIANGLELIST`; `Encode` sets stencil ref and blend factor then `RecordGBufferDraw` |
| encoder | `GE:11-61` | `ClearGBuffer` (targets and depth), `BindGBufferDraw` requires `!targets.empty()`, `targets.size()<=8`, `tables.size()<=4`, topology list or strip; `OMSetRenderTargets(n, targets, FALSE, &depth)` |
| scene checks | `SP:1362-1366` (offscreen), `SP:1551-1562` (main) | `draws`: 6 targets and the 6 colour formats; `forward_draws`: 1 target `ForwardTargetFormat()`; `velocity_draws`: 1 target `VelocityTargetFormat()`; depth `ModelDepthFormat()` for all |
| encode sites | `SP:1380-1390`, `SP:1588-1609` | `std::array<CPU,6> targets` from `rtvs_`; forward `std::array<CPU,1>{native_light_rtvs_ slot 3}` |
| producers | `GB:4874`, `:4913`, `:4929`, `:5337` | formats set from `SceneRenderer::ModelTargetFormats()`/`ForwardTargetFormat()`/`VelocityTargetFormat()`; the draw is pushed to `frame->draws`, `forward_draws` or `velocity_draws` |
| tests | none | `OwnedNativeModelDraw` is exercised live only; `renderer/gbuffer_encoder_test.cpp` (a 6-target WARP test of the encoder, with its own PSO) is **not built by any script**; `ownership_test` is built by `renderer/Build.ps1` and does not touch model draws |

## 2. Requirements, from the three consumers and from phase C

| Consumer | Needs | Source |
|---|---|---|
| glow RT1 (G1) | forward draws writing `[HDR, GB5]`: 2 targets, `IndependentBlendEnable`, RT1 blend = RT0 blend; formats `ForwardTargetFormat()` and `ModelTargetFormats()[5]` | WAVE1-PREP §2.2 |
| tessellation (T3) | HS+DS programmes (`.hpo/.dpo` from the SPX packages), root visibility for HS b5,b6 and DS b5,b6,b8 (+ t5,s4 for `_D`), patch topology `3_CONTROL_POINT_PATCHLIST`, `PrimitiveTopologyType=PATCH` | TESS-SHADOW-FORWARD-NATIVE §1.2, WAVE1-PREP §2.4 |
| shadow atlas (S3) and casters (wave 3) | zero colour targets, D16 depth (typeless R16 resource with a D16 DSV), per-cascade viewport/scissor and depth-bias rasterizer variants, optional pixel shader (Dpt opaque) | SHADOWS-NATIVE §2, WAVE1-PREP §2.3 |
| R6 (global descriptor heaps, used-registers-only tables) | per-draw descriptor ranges sized by the declared registers; heaps set once per list | A-C-PREP §2.6 |
| R7 (root signature sized on use) | a root-signature family per shape; hull/domain denied until the tessellated families add their own | A-C-PREP §2.7 |
| R8a (persistent draws) | an immutable part (shape, PSO, descriptors) separable from the per-frame constants | A-C-PREP §2.9 |
| R9 (PSOs created ahead) | a keyed cache with a hashed key; PSO inputs known at admission | A-C-PREP §2.8 |

## 3. Design

### 3.1 The envelope enumeration and its table (`model_draw.h`)

```
enum class ModelPassEnvelope : uint8_t { GBuffer6, Forward1, Forward2, Velocity1, DepthOnly };
enum class ModelRootFamily  : uint8_t { Model, ModelTess };          // geometry shaders stay denied
enum class ModelTopologyClass : uint8_t { Triangle, Patch3 };
struct ModelEnvelopeSpec { uint8_t targets; bool pixel_required; };  // formats and depth checked by the owner of the target set
const ModelEnvelopeSpec& EnvelopeSpec(ModelPassEnvelope) noexcept;   // GBuffer6 {6,true}, Forward1 {1,true}, Forward2 {2,true}, Velocity1 {1,true}, DepthOnly {0,false}
```

`NativeModelDrawInput` gains one nested block holding **every new field** (the R8a split line); the existing fields keep their place and their meaning:

```
struct NativeModelDrawShape {
    ModelPassEnvelope envelope{ModelPassEnvelope::GBuffer6};
    ModelRootFamily family{ModelRootFamily::Model};
    ModelTopologyClass topology{ModelTopologyClass::Triangle};
    ds3content::CpuShaderProgram hull_shader, domain_shader;   // family ModelTess only
};
// in NativeModelDrawInput: NativeModelDrawShape shape;
//   render_target_count and render_target_formats stay PRODUCER-SET exactly as today; Prepare() requires them to agree with the envelope
```

Rules enforced by `Prepare` (all `Require`): `render_target_count == EnvelopeSpec(envelope).targets` (a producer that forgets to set the envelope keeps the default `GBuffer6` and fails here with a 6-versus-1 mismatch, at preparation, instead of surfacing as a thrown scene check); `pixel_shader` present unless `!pixel_required` and the envelope is `DepthOnly`; hull and domain present **iff** `family == ModelTess` and `topology == Patch3` (all three together), and **absent otherwise**: `ValidateBindings` refuses a hull or domain binding on a `Model`-family draw; `family == ModelTess` is refused for `Velocity1` and `DepthOnly` in W0a (the veil and the flow-displacement families are forward and G-buffer rows). **Known gap:** TESS-SHADOW-FORWARD-NATIVE §1.1 lists `Dpt_PN`/`DptA_PN` programme sets, so the original also casts tessellated shadows; W0a ships flat casters for the veil (wave 3 can add `DepthOnly + ModelTess` by lifting that refusal, no other change).

Why a table and not four booleans: the scene checks, the encoder, the PSO key and the unit tests all read the same two numbers; a single source cannot disagree with itself.

### 3.2 Validation moved to one function

`bool EnvelopeMatches(const NativeModelDrawInput& input, ModelPassEnvelope expected, std::span<const DXGI_FORMAT> formats_expected, DXGI_FORMAT depth_format_expected)` as a **free function declared in `model_draw.h`** and defined in `model_draw.cpp` (the unit-test executable links `model_draw` but not `scene_pass`), used by `SceneRenderer` at the existing sites (`SP:1362-1366`, `SP:1551-1562`) and later by the shadow list:

| List | Expected envelope | Formats | Depth |
|---|---|---|---|
| `draws` | `GBuffer6` | `ModelTargetFormats()` (6) | `ModelDepthFormat()` |
| `forward_draws` | `Forward1` **or** `Forward2` (a list may mix both only if the target array handed to `Encode` matches per draw; see §3.5) | `[ForwardTargetFormat()]` or `[ForwardTargetFormat(), ModelTargetFormats()[5]]` | `ModelDepthFormat()` |
| `velocity_draws` | `Velocity1` | `[VelocityTargetFormat()]` | `ModelDepthFormat()` |
| `shadow_draws[cascade]` (wave 3) | `DepthOnly` | none | the atlas depth format (`D16_UNORM`) |

The producers (`GB:4874`, `:4913`, `:4929`) set `shape.envelope` next to the existing count and formats (one added statement each); nothing else changes in `GB` for W0a.

### 3.3 Root-signature families and descriptor layout

**Family `Model`** (today): parameters `[0]` VS resource table, `[1]` PS resource table, `[2]` VS sampler table, `[3]` PS sampler table; flags deny hull/domain/geometry. Unchanged.

**Family `ModelTess`** (new): the same four parameters and, appended, `[4]` HS resource table, `[5]` DS resource table, `[6]` HS sampler table, `[7]` DS sampler table, with `D3D12_SHADER_VISIBILITY_HULL` / `DOMAIN`; flags: `ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | DENY_GEOMETRY_SHADER_ROOT_ACCESS` only. Eight descriptor tables cost 8 DWORDs of the 64-DWORD root budget. The per-stage block layout is the existing one repeated: stage `s` ∈ {VS 0, PS 1, HS 2, DS 3}, resource block `s*stage_resources` (14 CBV + 128 SRV), sampler block `s*sampler_count`.

**Per-draw descriptor budget and chunking** (replaces the constant `table_chunk_draws=64`): `resources_per_draw(family) = stage_resources * stages(family)` (284 or 568) and `samplers_per_draw(family) = sampler_count * stages(family)` (32 or 64). The two families fill the **same chunk size**: a `Model` chunk is 64 × 284 = 18,176 resource descriptors and 64 × 32 = 2,048 samplers (the shader-visible sampler heap limit); a `ModelTess` draw takes 568 and 64, i.e. 32 per chunk (32 × 568 = 18,176; 32 × 64 = 2,048). So there is **one pool**: `TableChunk` keeps a descriptor cursor (resources, samplers) instead of a draw count, and a draw advances it by its family's per-draw sizes; the chunk is full when either cursor cannot take the next draw. R6 deletes the pool and allocates frame-linear ranges; it keeps the **resource-range size function** of the family. It does not keep the sampler sizing: R6's C2c moves samplers to stable indices from the sampler bank (A-C-PREP §2.6), so one 2,048-sampler heap per frame is no longer consumed at 32 or 64 per draw.

**Null templates**: today `BindDevice` creates one null block of `stage_resources*2` and `sampler_count*2` descriptors and `Prepare` copies the whole template per draw. W0a creates `stage_resources*4` / `sampler_count*4` templates and `Prepare` copies only `stages(family)` blocks (the `Model` copy stays 284 + 32 descriptors: no regression for existing draws). Resource Binding Tier 3 on the test adapter (phase A) allows R6 to drop the fill later; until then the fill is what keeps Tier 1 correct.

**Root-shape ledger**: `NativeModelRootShapes()` returns 6-number shapes; add `NativeModelTessRootShapes()` returning 12-number shapes `{VS, HS, DS, PS}×{cbv, srv, sampler}` so the existing pacing row (`GB:713`) is untouched and R7 has the data it needs for the tessellated family.

### 3.4 The PSO key as a POD (`ModelPsoKeyFields`)

```
struct ModelPsoKeyFields {
    const void *vs, *ps, *hs, *ds;                 // verified program entries (stable addresses), null when absent
    ModelPassEnvelope envelope; ModelRootFamily family; ModelTopologyClass topology;
    uint32_t sample_mask; uint8_t render_target_count; DXGI_FORMAT depth_format;
    std::array<DXGI_FORMAT,6> render_target_formats;      // the input array is 6 (MH:44); W0a sets 0..6 targets
    // elements, rasterizer, depth-stencil and blend are appended as today (their serialisers are unchanged)
};
```

W0a-1 introduces the struct and a serialiser that produces the byte string the cache uses today **plus the new fields**; R9's C1a replaces the serialiser by a 64-bit hash of the same fields without touching the producers. `PrimitiveTopologyType` is derived from `topology` (`TRIANGLE` or `PATCH`). The blend section already serialises all eight `RenderTarget` entries and `IndependentBlendEnable` (`MD:236-240`), so the RT1 blend copy of glow needs no key work.

### 3.5 Encoder and recording

- **`BindGBufferDraw`** (`GE:41`): replace `draw.targets.empty()` by `(draw.targets.empty() && !draw.depth.ptr)`; keep `targets.size()<=8`; call `OMSetRenderTargets(n, n ? targets.data() : nullptr, FALSE, &draw.depth)` (D3D12 allows a null RTV array with zero targets). `draw.tables.size()<=4` becomes `<=8`; topology accepts `3_CONTROL_POINT_PATCHLIST` (value 35) in addition to list and strip. `live_host.cpp` and the offline `SP:1936` callers pass non-empty targets and at most four tables: unaffected.
- **`ClearDepth(list, depth, clear_depth, clear_stencil, has_stencil)`** beside `ClearGBuffer` (a DSV handle carries no format, so the caller says it): `ClearDepthStencilView` with `D3D12_CLEAR_FLAG_DEPTH` and, only when `has_stencil`, `D3D12_CLEAR_FLAG_STENCIL` (D16 has no stencil plane).
- **`OwnedNativeModelDraw::Describe`** returns `topology` from `shape.topology` (triangle list or patch list) and `tables` of size 4 or 8 by family (`tables_` becomes `std::array<..., 8>`, `stage_count_` stored).
- **Mixed forward lists**: `Encode` takes the target span per call. **Both** encode paths, `RenderNativeModels` (`SP:1606-1607`, block `SP:1593-1609`) and the offscreen `RenderNativeOffscreen` (`SP:1386-1390`), build two target arrays (`forward1`, `forward2`) and call `draw->Encode(list, draw->Input().shape.envelope==Forward2 ? forward2 : forward1, ...)`. Both arrays hold CPU handles from different heaps (`native_light_rtvs_` slot 3 for the HDR, `rtvs_` slot 5 for GB5): D3D12 accepts handles from different heaps in one `OMSetRenderTargets` call when `RTsSingleHandleToDescriptorRange` is FALSE (as here), so the RTV heap capacity (`native_light_rtvs_` is full at 8, `SP:975-976`) is not touched. Both are presenter-sized (`SP:985`).
- **GB5 state**: it is left in `RENDER_TARGET` by the lighting (`SP:1136`), which is the state a forward draw needs; nothing to transition for `Forward2` (a witness row asserts it).

## 4. The change list, file by file (what W0a edits)

| File | Edit |
|---|---|
| `renderer/model_draw.h` | add the three enums, `ModelEnvelopeSpec`, `EnvelopeSpec`, `EnvelopeMatches`, `NativeModelDrawShape` (envelope, family, topology, hull and domain programmes); `NativeModelDrawInput`: `shape`; `OwnedNativeModelDraw`: `tables_` to 8, `stage_count_`, `family` accessor; declare `NativeModelTessRootShapes()`, `ModelPsoKeyFields`, and the counters accessor (`ModelDrawCounters`: PSOs created, table chunks created, descriptors copied) |
| `renderer/model_draw.cpp` | `Stage()` → 4 stages (Hull 2, Domain 3) (`:20-23`); `ValidateBindings` takes a span of interfaces by stage and handles 2 or 4 (`:81-119`), arrays `[4][...]`, refuses HS/DS bindings on a `Model`-family draw; `constants_` to `28 → 56` (`MH:86`); `BindDevice`: second root signature `root_tess`, templates ×4 (`:184-219`); `TableChunk` with resource and sampler cursors (`:171-183`); `Prepare`: optional pixel shader for `DepthOnly`, HS/DS programmes, require count == envelope (`:244-250`, `:270`), `ModelPsoKeyFields` + serialiser (`:272-279`), PSO desc with `HS/DS`, `PrimitiveTopologyType`, `NumRenderTargets` 0..6 (`:282-287`), template copy by stage count (`:295-298`), constants/SRV/sampler copies indexed by stage (`:299-312`), counters; `Describe` (`:320-325`): topology and tables by family; `EnvelopeMatches` |
| `renderer/gbuffer_encoder.h` | `BindGBufferDraw` relaxations (`:41-47`); `ClearDepth(…, has_stencil)` |
| `renderer/scene_pass.cpp` | `EnvelopeMatches` replaces `:1362-1366` and `:1551-1562`; forward target arrays in **both** encode paths: offscreen `:1388` and main `:1606-1607` (block `:1593-1609`), two arrays each |
| `renderer/model_frame.h` | comment update at `:70` (forward draws: one or two targets); no new list in W0a |
| `frame/game_bootstrap.cpp` | `shape.envelope = GBuffer6 / Forward1 / Velocity1` at `:4874`, `:4913`, `:4929` (three one-line edits); a new row `native_model_draw_counters` next to the pacing row (`:700-712`; the pacing row's 1024-byte buffer is too small to extend); the flat-fallback edit is W0a-6 |
| `frame/Build-GameBootstrap.ps1` | build and run `model_draw_envelope_test` and `gbuffer_encoder_test` (both link `backend`, `intent`, `content`, `model_draw`, `shader_interface`, the encoder header; `:122` loop and `:162` primitive objects already include `model_draw`), output under the I-0066 build output; **not** `renderer/Build.ps1`/`Test.ps1`, which write to `evidence/I-0036/…` (a retired tree that must not be recreated: CLAUDE.md retention rule) |
| `frame/Test-GameModelPreparation.ps1` | for T7: add `model_pass.cpp` to the compile line (next to `model_programs.cpp`, `:14`) and `model_pass` to the object list (`:21`), which today has no `model_pass` |
| `content/model_pass.h/.cpp` | W0a-6: the pure function `ShouldDisableDeformation(bool tess_family_enabled, bool tess_recipes_resolved)` |
| `renderer/model_draw_envelope_test.cpp` (new) | §8.1 |

Total: about 240 added lines in `MD`/`MH`/`GE`, 15 in `SP`, 5 in `GB`, one new test file of about 300 lines.

## 5. Commit sequence (each builds, passes the unit tests, is reversible alone)

| # | Commit | Behaviour change | Test | Revert |
|---|---|---|---|---|
| W0a-1 | **all inert types**: the three enumerations, `EnvelopeSpec`, `NativeModelDrawShape` (default = today), `ModelPsoKeyFields` + serialiser, the counters (`ModelDrawCounters`, row `native_model_draw_counters`); `model_draw_envelope_test` scaffold built by `Build-GameBootstrap.ps1`, `gbuffer_encoder_test` wired there too | none | T1 (key struct serialised directly: equality/inequality over every field, no `Prepare` needed) | `git revert` |
| W0a-2 | `EnvelopeMatches`; producers set `shape.envelope`; `Prepare` requires count == envelope; the seven count checks use it | none (same accept/reject set) | T2a: `GBuffer6`, `Forward1`, `Velocity1` accepted for their lists, rejected elsewhere | `git revert` |
| W0a-3 | depth-only: `BindGBufferDraw` empty targets, `ClearDepth`, optional pixel shader, `NumRenderTargets=0`, D16 formats in the key | none for existing draws | T3: a depth-only D16 draw on WARP writes depth (readback); T2b: `DepthOnly` accepted | `git revert` |
| W0a-4 | `Forward2` accepted by the validators and by `Prepare`; `Encode` with two target handles in both paths | none (no producer creates `Forward2` yet) | T4: a 2-target forward draw writes both targets, RT1 blend copy; T2c: `Forward2` accepted in `forward_draws` only | `git revert` |
| W0a-5 | family `ModelTess`: HS/DS stages, second root signature, 8 tables, one descriptor pool with two cursors, patch topology, tess shape ledger | none (no producer creates `ModelTess` yet) | T5: a pass-through HS/DS patch draw produces the expected triangles; T6 root family isolation; T9 `Model` draw with HS/DS binding refused | `git revert` |
| W0a-6 | the flat fallback (§7), shipped with T1a of the GX04 parse (W5e) or before it | none while `native-tess.flag` is off | T7, T8 (§7.3, §8.1) | flag file / `git revert` |

W0a-1..5 change no pixel: the idle High Wall frame digest (master plan row C0) and `content_draws` must stay equal after each. They can land before or after phase C's C1a (hashed key): if C1a lands first it hashes the current fields and W0a-1 adds the new ones to the same struct; if W0a-1 lands first C1a replaces the serialiser. No ordering is required between them, but **W0a-1 must precede C3 (R7)** so R7's shape cache keys on the family.

## 6. Alignment with phase C

| Phase C reprise | What it changes in the same code | How W0a is built to coexist |
|---|---|---|
| R6 global heaps, used-registers-only tables (`MD:171-183`, `:295-312`) | deletes `TableChunk`, allocates frame-linear ranges, writes only declared registers | the per-family **resource**-range size function is the contract R6 keeps (R6's C2c moves samplers to stable bank indices, so it does not keep the sampler sizing); the template-fill is isolated in one function (`FillNullBlocks(family)`) R6 removes; `GBufferDraw.heaps` unchanged (heaps set once per list is R6's change in `RenderNativeModels`) |
| R7 root signature sized on use (`MD:190-203`, `ValidateBindings` bounds) | one root signature per shape and per stage | the root signature cache key becomes `(family, shape)`; `ModelTess` shapes are tracked in the 12-number ledger from W0a-5, so R7 sizes HS/DS tables from measured data (the notes suggest HS ≤ 7 CBV and DS ≤ 9 CBV + 6 SRV + 5 samplers, to be confirmed by reflection: `_FT_D` also needs a flow texture at t17, TESS-SHADOW-FORWARD-NATIVE §3, stage not stated) instead of the fixed 14/128/16 |
| R9 hashed keys and prewarm (`MD:272-289`) | replaces the string key by a hash, adds `PrewarmModelPipeline` | `ModelPsoKeyFields` is the single input of both; the prewarm call at admission needs only `shape` + formats + programme entries, all present in the input; HS/DS entries are part of the key from W0a-5 |
| R8a persistent draws (`MH:64-94`, `GB:4780+`) | splits `OwnedNativeModelDraw` into an immutable part and a per-frame part | `NativeModelDrawShape` (every new field) plus the fields that already exist for the same role (the other two programmes, formats, depth format, fixed-function state, elements) and the PSO/root/tables identity form the immutable part; constants and `streams_` form the per-frame part |
| R2 arena (constants) | constants from the frame arena instead of `PooledConstants` | `constants_[56]` indexes by `stage*cbv_count+slot` and keeps the `ConstantResource` accessor; the arena change swaps the resource source only |

Conflicts to coordinate (shared lines with phase C): `MD:20-23`, `:81-119`, `:171-219`, `:243-330` and `GE:41-61`. The note to the phase C owner is §5's last paragraph plus: **do not add fields to `NativeModelDrawInput` outside `shape`** after W0a-1, so R8a's split stays mechanical.

## 7. The flat fallback of the GX04 parse (T1b)

### 7.1 The problem (observed)

Once the GX04/GX90 parse (T1a, `GB:7833-7840`) writes material `+0x250/+0x254`, `GB:3615-3616` maps them to `base_deformation` 1..4 (1/2 = FT/FT_D, 3/4 = PN/PN_D). `PrepareNativeSelectedPasses` passes it to `EvaluateOrdinaryModelPass` (called at `GB:4476`) which, unless the pass flags disable deformation, returns an intent with `deformation = base` (`content/model_pass.cpp:16-18`, `:28-29`, intent built at `:69-72`); `SelectModelPrograms` refuses any deformation (`content/model_programs.cpp:40-42`), and the row is dropped at gate 27 (`GB:4515-4517`): the veil (c5270, `Veill_NoLight`) stops drawing instead of drawing flat as it does today.

### 7.2 The edit

In `PrepareNativeSelectedPasses` (`GB:4402-4558`) where the pass input is assembled: the split line at `GB:4467` today **assigns** (`if(split){input.flags=SplitEngine5; input.row_class_mask=...}`), so the new statement goes **after** that line (or the assignment becomes `|=`), otherwise it would be wiped for split rows:

```
// W0a-6: until the tessellated family is native, every mesh draws flat (the state before the GX04 parse).
if(ds3content::ShouldDisableDeformation(scene.tess_family_enabled, TessRecipesResolved(mesh, input.shader_pass)))
    input.flags |= uint32_t(ds3content::NativeModelPassFlag::DisableDeformation);
```

- `ShouldDisableDeformation(enabled, resolved)` is a **pure function** in `content/model_pass.h/.cpp` (`return !(enabled && resolved);`), so the decision is unit-testable without the DLL; the call site stays one statement.
- `scene.tess_family_enabled` is read **once at renderer start** from `native-tess.flag` next to the module (default **off**), exactly like `measure.flag` (`GB:8980-8986`: read at start into the global `measure_run`), into a global, and copied into `NativeSelectedScene` (struct at `GB:~2374`) each frame.
- `TessRecipesResolved(mesh, shader_pass)` is keyed by **(mesh, shader pass)**, not by the pixel variant: the pixel variant is only known after the evaluation (`decision.intent->pixel_variant`, `GB:4520`), so the helper answers "every pixel variant of this pass has its vertex, hull, domain and pixel programme resolved". It is constantly false in W0a-6 (the tess recipes do not exist until T2b); T2b implements it, so a **missing HS/DS name degrades to flat, never to a gate-27 refusal** (WAVE1-PREP T2 pitfall).
- `EvaluateOrdinaryModelPass` already accepts the flag (`:53` allows `DisableDeformation|SplitEngine5`), computes `effective_deformation = 0`, and returns an intent with `deformation 0` and the flat recipe. `base_deformation > 4` still returns `Deformation` (`:57`), unchanged.
- The mesh keeps `bound.base_deformation` (diagnostics, T2) and the log row `native_pass_refusals` must show **gate 27 empty** for the veil with the flag off and on.
- Velo (pass 1) stays flat with the flag on: it has no tess variant (`WAVE1-PREP` T2).
- The existing LOD rule (`requested_lod != 0` → deformation 0, `:16-18`) is untouched.

### 7.3 Ordering with T1a and the K workstream

T1a (parse) and T1b (this edit) land in the same commit or T1b first: with T1b first the flag changes nothing today (deformation is always 0 because nothing parses GX04); with T1a first and no T1b the veil is lost. W0a-6 is therefore **ordered before or with** W5e. Workstream K moves block production to row time (K2) and extends per-recipe arrays for the tess recipes (T2b): T1b sits upstream of both (it changes only the pass input flags), so no conflict.

## 8. Tests and witnesses

### 8.1 New unit-test executable `model_draw_envelope_test` (WARP, then hardware)

Built from `renderer/model_draw_envelope_test.cpp`; links `backend`, `intent`, `content`, `model_draw`, `shader_interface` and the encoder; HLSL compiled in the test with `D3DCompile` (as `gbuffer_encoder_test.cpp:12-16`). Cases:

| Case | What | Pass |
|---|---|---|
| T1 key | two `ModelPsoKeyFields` identical except one of {envelope, family, hull entry, domain entry, topology, depth format, RT count, RT format} serialise differently; identical ones serialise identically (the POD is serialised directly, no `Prepare`, so it builds in W0a-1) | distinct / equal |
| T2 envelope | `EnvelopeMatches`, cases added with the commit that introduces each envelope: T2a (W0a-2) `GBuffer6`, `Forward1`, `Velocity1` accepted for their lists and rejected elsewhere; T2b (W0a-3) `DepthOnly`; T2c (W0a-4) `Forward2` accepted in `forward_draws` only, `GBuffer6` rejected there | exact |
| T3 depth-only | a `DepthOnly` draw (D16 depth, no colour target, no pixel shader) of one triangle writes the expected depth value (readback of a `R16_TYPELESS` resource through a copy) and the root/PSO is the `Model` family | depth equals the VS output within 1/65535 |
| T4 two targets | a `Forward2` draw with `IndependentBlendEnable` and RT1 blend = RT0 blend writes the expected colour to both targets; a PS with a single output writes RT1 with write mask 0 untouched | pixel equality |
| T5 tessellation | a pass-through hull (3 control points, fixed factor 1) and a domain shader draw a patch list; the output covers the same pixels as the triangle list of the same index buffer (`Patch3` with `family ModelTess`) | pixel equality |
| T6 family isolation | a `Model` draw recorded after a `ModelTess` draw in the same list reads its own tables and root | pixel equality |
| T9 binding rule | a `Model`-family draw given an HS or DS binding is refused by `ValidateBindings`; a `ModelTess` draw missing its hull or domain programme is refused by `Prepare` | exact |
| T7 flat fallback | `EvaluateOrdinaryModelPass` with `base_deformation=3`: without the flag the intent has `deformation 3` (and `SelectModelPrograms` returns Unsupported); with `DisableDeformation` the intent has `deformation 0`; the veil set `Veill_NoLight_Fwd.vpo/.ppo` selects. Added to `game_model_preparation_test.cpp` next to `:74-75` and built by `Test-GameModelPreparation.ps1`, **after** adding `model_pass.cpp` to its compile line (`:14`) and `model_pass` to its object list (`:21`), which have neither today | exact |
| T8 decision function | `ShouldDisableDeformation(enabled, resolved)` over the four input combinations (true only when both hold gives false); and the live witness: the `native_pass_refusals` gate histogram shows **gate 27 empty** for the veil with the flag off and with it on (the pass walk itself lives in the DLL and no harness links it, so it is not unit-tested) | exact / live |

`renderer/gbuffer_encoder_test.cpp` is built and run from `Build-GameBootstrap.ps1` as part of W0a-1 (it is not built by any script today) and gains the empty-target and eight-table cases of `BindGBufferDraw`. `renderer/Build.ps1`/`Test.ps1` are not used: they write under `evidence/I-0036/…`, a retired tree that must not be recreated.

### 8.2 Live witnesses (non-regression; nothing visible changes in W0a-1..5)

Native1254 in-game (the idle frame digest and `content_draws` equal), Native1191 velocity (`Velocity1`), Status screen and portraits and character creation (the offscreen forward path, `SP:1362-1390`), the NPC and cloth witnesses, Native1262 quit, each once with the debug layer on (0 errors). Counters, **added by W0a-1** (they do not exist in native today): `ModelDrawCounters` in `model_draw.cpp` (atomic `pso_created`, `table_chunks_created`, `descriptors_copied`, by family) and the new row `native_model_draw_counters` emitted per pacing window next to the pacing row (not inside it: its buffer is 1024 bytes). Expected: PSOs created equal to today for `Model` and 0 for `ModelTess`; `NativeModelRootShapes` unchanged; `prepare_ms` not worse (±5 % of the phase A median M1: prepare 26 ms).

### 8.3 Measures

Before and after W0a-5, from the counters above and the existing rows: `prepare_ms`, `descriptors_copied` per frame (284 + 32 per `Model` draw, unchanged), process private bytes (one extra root signature, created lazily with the first tess draw: expected under 1 MB), PSOs created after warm-up (0). A regression on any of them reverts W0a-5 only (family isolation).

## 9. Risks

1. **Tier-1 descriptor validity for HS/DS tables**: the template fill covers it (all 4 stage blocks of a `ModelTess` draw are written); R6's fill removal must keep a null prefix for the tess family until the adapter tier is known.
2. **Per-draw descriptor footprint of the tess family** (568 resource descriptors, 64 samplers): 32 draws per chunk (one pool, two cursors); the veil is a few dozen meshes per frame at most, so two or three chunks; if a zone draws hundreds of tessellated meshes, R7's shape sizing (to be confirmed by reflection, §6) shrinks it.
3. **Shared forward target set per batch**: `Forward1` and `Forward2` draws must not interleave in a way that changes the bound target set mid-batch without re-binding: `Encode` binds the target span on every call (`BindGBufferDraw`), so mixing is correct, at the cost of one `OMSetRenderTargets` per switch (the current code already calls it per draw).
4. **`family == ModelTess` on `GBuffer6`**: allowed in the validator but no note asks for it yet; if `_FT_D` rows appear in the G-buffer, the six-target PSO with HS/DS follows the same path without further change.
5. **Patch topology and indexed draws**: `ValidateGeometry` requires `index_count % 3 == 0` (`MD:60-67`), which a 3-control-point patch list satisfies; a mesh built with strips would not be admitted (none are).
6. **`DENY_GEOMETRY_SHADER_ROOT_ACCESS` stays**; the effects family has its own root signature (D3D12-MASTER-PLAN §3.1) and does not use this one.
7. **The flat fallback depends on one flag**: with `native-tess.flag` on and T2b absent, `TessRecipeResolved` is false and everything stays flat: safe by construction.

## 10. Open points

The exact HS/DS register shapes of the `_PN_D` variants (the note names t5, s4, and the `_FT_D` flow texture at t17 without a stage; to confirm by reflection when the programmes are loaded in T2); the tessellated casters `Dpt_PN`/`DptA_PN` (a known gap, §3.1: flat casters for the veil until wave 3 lifts the `DepthOnly + ModelTess` refusal); whether the shadow atlas DSV should be a `D16_UNORM` view of an `R16_TYPELESS` resource with `ClearDepth` stencil flag off (S3a decides; `ClearDepth` supports both); the adapter's `ResourceBindingTier` on the user's machine (phase A logged tier 3 on the test adapter).

## 11. Review log

Independent review by an Opus 5.5 reader (read-only, given the note and the code, not my conclusions): **no blocker, verdict "ready after fixes"**; ten major and eight minor findings, all verified against the code before being applied. D3D12 validity was confirmed point by point (zero-target depth-only PSO and `OMSetRenderTargets(0, nullptr, FALSE, &dsv)`, optional pixel shader, D16 DSV on an R16_TYPELESS resource with a depth-only clear, eight tables = 8 DWORDs, HULL/DOMAIN visibility and flags, the sampler arithmetic 16×2×64 = 2,048 = 16×4×32, patch topology value 35 with an indexed draw, RTV handles from two heaps, GB5 presenter-sized and left in RENDER_TARGET). Disposition:

| # | Finding | Disposition |
|---|---|---|
| 1 | W0a-1 introduced types that only arrive later; T1/T2 unbuildable early | all inert types in W0a-1; T1 serialises the POD; T2 split T2a/b/c across commits (§5, §8.1) |
| 2 | `ValidateEnvelope` private to `SceneRenderer`, test does not link `scene_pass` | free function `EnvelopeMatches` in `model_draw.h/.cpp` (§3.2) |
| 3 | T8 needs the DLL's pass walk | the decision is a pure function `ShouldDisableDeformation`; T8 tests it, the gate-27 histogram is the live witness (§7.2, §8.1) |
| 4 | `Test-GameModelPreparation.ps1` has no `model_pass` | the script edit is in §4 and §8.1 T7 |
| 5 | `Test.ps1` writes under the retired `evidence/I-0036` tree | both tests built from `Build-GameBootstrap.ps1` (§4, §8.1) |
| 6 | `TessRecipeResolved` cannot know the pixel variant; the edit could be wiped by `input.flags=` | keyed by (mesh, shader pass) over all pixel variants; statement placed after `GB:4467` or `|=` (§7.2) |
| 7 | deriving the count hides a forgotten envelope | producers keep setting the count; `Prepare` requires equality (§0.1, §3.1) |
| 8 | §0.4/§6 said the shape holds the programmes and formats while §3.1 put HS/DS outside it | HS/DS moved into the shape; §0.4 and §6 state that the immutable part is the shape plus the pre-existing same-role fields |
| 9 | R6 does not keep per-draw sampler sizing | §3.3 and §6 say R6 keeps only the resource-range size |
| 10 | counters named in §8 do not exist | W0a-1 adds `ModelDrawCounters` and the row `native_model_draw_counters` (§4, §8.2) |
| 11 | `ClearDepth` has no format parameter | `has_stencil` added (§3.5) |
| 12 | key/NumRenderTargets 0..8 versus a 6-entry input array | 0..6, key array of 6 (§3.4, §4) |
| 13 | two pools unnecessary; Model-family draws must refuse HS/DS | one pool with two cursors (§3.3); rule and test T9 (§3.1, §8.1) |
| 14 | tessellated casters `Dpt_PN`/`DptA_PN` understated | recorded as a known gap (§3.1, §10) |
| 15 | DS bound may be low (`_FT_D` t17) | hedged (§6, §9, §10) |
| 16 | `measure.flag` is read at start, not per frame | §7.2 reads `native-tess.flag` at start into a global, copied per frame |
| 17 | stale references | corrected: seven count checks (`MD:270`, `MD:322`, five in `SP`); `model_pass.cpp:16-18`, `:28-29`, `:69-72`; `model_programs.cpp:40-42`; call at `GB:4476`; RTV heap `SP:975-976`; scene struct `GB:~2374`; forward array `SP:1388` (offscreen) and `SP:1606-1607` (main) |
| 18 | offscreen path not named for the two-array change | both paths named (§3.5, §4) |

WAVE1-PREP carried two of the same errors (`model_programs.cpp:46`, and the claim that `gbuffer_encoder_test` covers the 6-target envelope as a built test) and is corrected in the same commit.

## Exports

None. Sources read: `native/renderer/model_draw.h/.cpp`, `gbuffer_encoder.h`, `scene_pass.cpp` (`:355-484`, `:974`, `:1136`, `:1360-1392`, `:1546-1612`), `model_frame.h`, `native/content/model_pass.h/.cpp`, `model_programs.cpp`, `native/frame/game_bootstrap.cpp` (`:3615-3616`, `:4402-4558`, `:4874-4930`, `:5337`, `:7833-7842`, `:8975-8986`), `renderer/gbuffer_encoder_test.cpp`, `renderer/Build.ps1`, `frame/Build-GameBootstrap.ps1`; notes: D3D12-PHASES-A-C-PREP §2.6-§2.9, WAVE1-PREP §2.2-§2.4, TESS-SHADOW-FORWARD-NATIVE, SHADOWS-NATIVE §2, D3D12-MASTER-PLAN §3.1.

## Implementation log (pilot, 2026-10-12)

**W0a-1 to W0a-4 done, adapted to the current tree** (root CBVs C4-1/C4-1b, programme sets cP1-cP8, global descriptor heaps R6, kept draws C4-2). `ModelPassEnvelope` with its specification table (`EnvelopeSpec`, every case listed, no default), `NativeModelDrawShape` (envelope, stage family, topology class) in `NativeModelDrawInput`; `Prepare` requires the producer's count to agree with the envelope, a pixel programme unless the envelope is `DepthOnly` (then no pixel binding, and a depth format with depth enabled), and refuses the tessellation family and the patch topology until W0a-5; the shape is in the pipeline key and in the programme set's state words; `EnvelopeMatches` replaces the scene's seven count checks; `BindGBufferDraw` takes an empty target span for a depth-only draw and `ClearDepth` clears a depth target alone; the one producer names its envelope; the C4-2 kept-draw digest and comparison include the shape. **Deviations from sections 3-5, on purpose:** the scene lists admit only `GBuffer6`, `Forward1` and `Velocity1` until `Forward2` (glow RT1, W6) and `DepthOnly` (the shadow atlas, W9) declare their uses in the frame graph with their slices (phase E's rule: a new pass is shadowed before the executor drives it); W0a-5 (hull and domain stages, the second root signature) waits for the tessellation slice W7; no `ModelPsoKeyFields` struct (the key string and the state words carry the shape; R9's hashed key is still to come); the tests extend `model_draw_test` (T2 envelope agreement and `EnvelopeMatches` including the G-buffer formats, T3 a D32 depth-only draw with no pixel programme, also under the shadow and on the legacy tables, T4 a two-target draw with `IndependentBlendEnable` off) instead of a new executable, and the test now counts debug-layer warnings as the renderer does (`gbuffer_encoder_test` is still not built by any script). **Witnesses:** Native1459 (High Wall protocol with Load Game portraits, debug layer, both shadows): exit 0, 0 debug errors, 0 collect-shadow mismatches over 183 scenes, 0 graph mismatches; Native1460 (Bink and character creation): 0 debug errors, 0 graph mismatches, the known close fault `0xD12759`. **Opus review: no blocker**; applied: warnings counted in the test, the shape's family and topology in the state words, the G-buffer envelope cases, the depth-only draw under the shadow and the legacy tables, no default case, the depth-only depth requirement.
