# I-0068 part AA: the emissive and glow chain, ready to code

Request (coordinator, 2026-10-10): the emissive and glow chain, absent natively. It serves boss
auras, torch bowls, enchanted weapons and glowing eyes. To cover:

- the emissive sources (G-buffer or forward);
- extraction, blur and composite (YEBIS or the engine's own);
- the gates;
- checks in the witness zones.

TONEMAP-SKIN §17 (I-0067, not modified) already identifies the owner `GXGlowFilter` and its
modes. This note adds the observed chain, the programme interfaces and the source side, and
turns the whole into ordered native steps.

Labels: STATIC, OBSERVED, INFERRED.

**Evidence:**

- the five `i63-live-10` captures (programme ids 142/197/149, stable across sessions);
- the m40 named listing;
- the DXBC reflection (I-0063 programme cache).

## 0. Answer first

- **This is not YEBIS.** It is the engine's own **`GXGlowFilter`**.
- **It runs every frame in all five witness zones** (and m40), as 10 draws. It reads **GBuffer5
  alpha-weighted** and **adds** into the final LDR image after anti-aliasing.
- Its sources are three:
  - the G-buffer emissive output (RT5);
  - the forward programmes' **second target** (`*Glow*_Fwd`);
  - the FFX glow twins (`*GlowD`, second target).
- **Native:**
  - the G-buffer writes GB5 and the composite adds GB5 into the lit HDR
    (`native/renderer/scene_pass.cpp:1004`);
  - **forward draws bind one target** and drop the glow output
    (`native/frame/game_bootstrap.cpp:4241`, `render_target_count=1`);
  - FFX is absent;
  - **no glow chain exists**: the final display is a plain composition
    (`native/renderer/scene_pass.cpp:464`).

## 1. The observed chain (OBSERVED, all five zones and m40)

Per frame, same structure in m31, m37, m45, m50 and m51 (1280×720; counts per 3-frame capture:
3/12/15):

| # | Programme (m40 name) | Target | Input | State |
|---|---|---|---|---|
| 1 | `EmissiveToGlowPS` (ps 142) | **1280×720 R11G11B10** (glow source) | **t0 = GBuffer5** (RGBA16F) | opaque (blend 6) |
| 2–5 | `Sample13Weight` (ps 197) ×4 | **320×180 → 160×90 → 80×45 → 40×23**, R11G11B10 | the previous level (INFERRED from the order: the first reads the 1280×720 source) | opaque |
| 6–9 | `GlowCompositeSinglePS` (ps 149) ×4 | 80×45 ← 40×23, 160×90 ← 80×45, 320×180 ← 160×90, **1280×720** ← 320×180 | the smaller level | **additive**: blend 17 = SRC_ALPHA / ONE / ADD (output alpha 1), alpha ONE / ZERO, mask RGBA |
| 10 | `GlowCompositeSinglePS` | **the final 1280×720 RGBA8 target** | the 1280×720 glow | additive (blend 17) |

**Order** (m40 listing): after YEBIS, `PointSampleColor1` and `DLAA` (which writes the final
target), then glow steps 1–10. INFERRED: then the UI and the display gamma
(DISPLAY-SWAPCHAIN-NATIVE §9).

## 2. Programmes (STATIC, DXBC)

| Programme | Inputs | Constants | Output |
|---|---|---|---|
| `EmissiveToGlowPS` | t0 `g_SourceTex`, s3 ClampLinear | — | `o0.rgb = src.rgb × src.a`, `o0.a = saturate(src.a)` |
| `Sample13Weight` | t0, s3 ClampLinear | b1 `cbSampleOffset`: `g_avSampleOffsets[16]` with xy offset, z weight, 13 used; b0 `cbFilterParam`: `g_vScreenSpaceTexCoordScale` (+16) scales the offsets | `Σ_{i<13} w_i · src(uv + off_i · scale)` |
| `GlowCompositeSinglePS` | t0, s3 ClampLinear | b2 `cbGlowParam.g_GlowScaleOffset.x` | `o0 = (src.rgb × scale, 1)` |

The VS is the GX full-screen quad of the common post object (`[GXRS+0x4DD8]`), drawn by
`7F820` (the same helper as the light tiles).

## 3. Sources: who writes GBuffer5 (STATIC + DXBC)

| Source | Programmes (examples) | Writes GB5 | Native |
|---|---|---|---|
| **G-buffer emissive materials** | `GXFlver_ColDifSpcBumpEmiIbl_Gbuf` (6 outputs; `g_EmissiveTexture` t6, `FC_EmissiveColor` b2 +32): torch bowls (`M[ARSN]_em`, MENUS-MAP §54), lamps, eyes | RT5 of the G-buffer | written: native G-buffer pass with 6 targets (DONE for admitted families; the bowl family is not admitted, MENUS-MAP §54 family 4) |
| **Forward glow materials** | `GXFlver_ColDifSpcBumpEmiIblGlow_Fwd` (**2 outputs**; `FC_GlowScale` b3 +144): glowing translucent parts, enchanted blades | **RT1 = GB5** (the original binds HDR + GB5 for forward rows: m40 listing `rt=12439/12447`) | **dropped**: native forward binds one target |
| **FFX glow twins** | `GXFfx*GlowD` (2 outputs; `g_psGlowScale`): auras, fire, spells | RT1 = GB5 | absent (no effect system; FFX-ATTACHED-NATIVE) |

The plain `_Fwd` programmes declare one output: the original still binds GB5 as RT1 for them.
The D3D12 rule of DECALS-NATIVE §2.4 applies: **write mask 0** on undeclared outputs.

GB5 also feeds `GXLightComposite` (the emissive term in the lit HDR, already native). Glow is an
additional, post-AA bloom on top of it.

## 4. Gates (STATIC; TONEMAP-SKIN §17)

- `GXGlowFilter` slot 0 `1BB810(filter, frame, param)` runs when:
  - **`param+0x24` (enable)** is set;
  - **the global glow factor `[[GXRS+0x5368]+0x80]`** is > 0.
- Strength: `g_GlowScaleOffset = ((param+0x14) × clamp(factor, 0, 1), param+0x18)` for the
  extraction and the level composites; `(param+0x20) × clamp(factor, 0, 1)` for the final
  composite (STATIC, §8.3).
- **Mode B** (`param+0x1C != 0`): the four-level chain over the filter's own targets
  `filter+0x28..+0x40`. This is the observed shape: a source at full resolution, then four
  levels from ¼.
- gparam `Glow Filter` exists only at default id 10 (Enable 0). The world keys fall back with no
  Glow entry. INFERRED: the world enable is the filter default.
- Gates on the source side: a material's GB5 write depends only on its family (Emi/Glow/GlowD
  programmes) and `FC_EmissiveColor`/`FC_GlowScale`/`g_psGlowScale`. There is no separate
  switch.

## 5. Ready-to-code order

| # | Step | Native |
|---|---|---|
| 1 | **Forward second target** | forward rows (0x29/0x1F) bind **[HDR, GB5]**; the blend for RT1 follows the programme's blend mode, as for RT0 (to verify, risk 1); write mask 0 when the programme declares only o0 |
| 2 | **Admit the emissive G-buffer families** | `GXFlver_ColDifSpcBumpEmiIbl[Glow]` and the `_em` MTDs through the existing G-buffer path. Their class `0x100` has no native row for map parts: add the engine-7 row by producing the model `+0x220`/`+0x224` union for map parts (NATIVE-GATES-AUDIT §2.5). **Part AL:** also parse GX03 (`FC_EmissiveColor`, values 0.07 to 5) in `BuildModelMaterial`, and produce the union for enemy/NPC models too (NPC-FAMILIES-STATIC §2.1-2.2) |
| 3 | **Glow resources** | one 1280×720 R11G11B10 source and four R11G11B10 levels at ¼, ⅛, 1/16, 1/32 of the source (320×180 … 40×23 at 720p; round up) |
| 4 | **Extraction** | `EmissiveToGlowPS`: GB5 → source, opaque |
| 5 | **Blur down** | `Sample13Weight` ×4: source → L1 → L2 → L3 → L4. Constants: 13 taps (`cbSampleOffset`) and the texcoord scale (`cbFilterParam`); their producer is `7F950(…, count = param[2])` in `1BB810` (taps and weights not decoded statically, risk 2) |
| 6 | **Composite up** | `GlowCompositeSinglePS` ×5: L4 → L3, L3 → L2, L2 → L1, L1 → source, source → final; blend SRC_ALPHA/ONE/ADD with alpha 1; `g_GlowScaleOffset.x` = `(param+0x14) × f` for the four level composites and `(param+0x20) × f` for the last one, `f = clamp(factor, 0, 1)` (§8.3) |
| 7 | **Placement** | after the native final colour (and after AA once it exists), before the UI and the display gamma |
| 8 | **Gate** | run only when the glow factor `[[GXRS+0x5368]+0x80]` > 0 and the filter enable (`param+0x24`) holds; read both each frame |

Steps 1–2 make the sources exist. Steps 3–8 are the filter; they need no YEBIS.

## 6. Checks (witness zones)

| # | Zone | Check | Pass |
|---|---|---|---|
| G1 | all five + m40 | structure | per frame: 1 extraction, 4 blurs (target sizes above), 5 additive composites, the last into the final target |
| G2 | m31 / m37 | GB5 content | the native GB5 equals the original's at the same frame and camera over emissive materials (mean abs diff ≤ 2/255 after the 8-bit view) |
| G3 | m40 | forward glow | with step 1, GB5 receives the forward rows' second output; the m40 listing's forward draws all bind `rt=…/12447` |
| G4 | m31 / m45 / m40 | image | the final image near lamps and the bonfire against the original: the halo appears; no change where GB5.a = 0 |
| G5 | High Wall | torch bowl | with step 2, `o304863`'s `_em` bowl writes GB5 and glows (MENUS-MAP §54); the flame's `GlowD` halo needs FFX-ATTACHED-NATIVE first |
| G6 | any | gate | forcing the glow factor to 0 (read-only witness of `[[GXRS+0x5368]+0x80]`): the original skips all 10 draws; native follows |

## 7. Risks

1. **Forward RT1 blend.** The original's blend state for RT1 in forward draws is not read here.
   Take it from a capture (the blend descriptor of a `*_Fwd` draw with two targets) before
   coding step 1. Additive and alpha are both plausible.
2. **Blur taps and weights** (`cbSampleOffset`) and the per-step `g_GlowScaleOffset` are
   computed in `1BB810`/`7F950`; not decoded. The retained captures hold no constant bytes:
   take one frame's constants (the I-0054 route of TONEMAP-SKIN §17), or transcribe `7F950`.
3. **Order with YEBIS.** Native has no tone map yet. Composite the glow onto the native final
   colour now, and keep it after YEBIS/AA when those arrive, as in the original.

## 8. Risks 1 and 2 resolved (part AB)

### 8.1 Forward RT1 blend (risk 1): RT1 follows RT0 (OBSERVED + D3D11 rule)

Every two-target forward or FFX draw writing [HDR RGBA16F, GB5 RGBA16F] uses a blend state with
**IndependentBlendEnable = 0**: m31 and m45, `i63-live-10`, all 95 and 105 such draws.

So RT1 takes **RT0's blend**, as D3D11 does. The four states seen are:

| State (m31 / m45 id) | RT0 (= RT1) | Draws |
|---|---|---|
| 19 / 18 | SRC_ALPHA / INV_SRC_ALPHA / ADD, alpha ONE / ZERO, mask RGBA | 54 / 54 |
| 17 / 17 | SRC_ALPHA / **ONE** / ADD (additive), alpha ONE / ZERO | 18 / 27 |
| 6 / 6 | opaque ONE / ZERO | 20 / 21 |
| 22 / 24 | colour writes off (mask 0) | 3 / 3 |

**Native step 1:** give RT1 exactly RT0's blend descriptor and write mask; or write mask 0 when
the programme declares no second output. No separate table is needed.

### 8.2 `Sample13Weight` taps and weights (risk 2) (STATIC)

The blur helper `7F950(filter, frame, source, scale, …, ps_variant, mode)` fills
`cbSampleOffset` (13 × float4: x, y offset, z weight) through one of three generators. Each
evaluates **`w_i = exp(−0.5 · (x_i² + y_i²)) / 2π`** (`143D7F0C0 = −0.5`, `143D7EEFC = 6.28319`),
then **normalises the weights to sum 1**. Offsets are `(x_i, y_i) × scale` (the `float` argument).

| Mode | Generator | 13 taps `(x, y)` (from `.rdata`) |
|---|---|---|
| 0 (2D) | `7FED0` | (0,0), (−0.5, 0.7071), (0.5, 0.7071), (−1, 0), (1, 0), (−0.5, −0.7071), (0.5, −0.7071), (0.5, 0.2357), (0, 0.4714), (−0.5, 0.2357), (0.5, −0.2357), (0, −0.4714), (−0.5, −0.2357) |
| 1 (horizontal) | `80B50` | (0,0), (−1/6, 0), (1/6, 0), (−1/3, 0), (1/3, 0), (−1/2, 0), (1/2, 0), (2/3, 0) **twice** as read (`143D7F678` and `143D7F7A0`; INFERRED −2/3 for one of them, symmetry), (−5/6, 0), (5/6, 0), (1, 0), (−1, 0) |
| 2 (vertical) | `80D10` | the same values on y |

**`cbFilterParam`** (b0, `7F950`):

- `+0 = (w/64, h/64, 0, 0)` (`143D7ED38 = 1/64`);
- **`+16 = (1/w, 1/h, 0, 0)` = `g_vScreenSpaceTexCoordScale`**, the texel size of the **render target of the pass** (CORRECTED, part BJ: `w`, `h` are the viewport size that `1400C63D0` sets from the target, not the source's size; see I-0066 GLOW-WAVE1-READY §1.3).

The shader samples `uv + off_i · (1/w, 1/h)` with `(w, h)` the target's size (one target texel per tap unit at `scale` 1).

The PS variant comes from the table {7, 9, 10, 11, 12, 13} indexed by `ps_variant` (programmes
at `filter+0xB70[]`).

The glow calls (`1BB810`, TONEMAP-SKIN §17) pass `count = param[2]` as the scale and
alternate the modes ("directions (2, 2)/(1, 2)"). With the default `Glow Filter` group
(BlurSampleScale 20), INFERRED: scale = 20 texels.

### 8.3 `g_GlowScaleOffset` per step (part AC) (STATIC, `1BB810`)

`1BB810` uploads PS b2 `cbGlowParam` twice, with `f = clamp(factor, 0, 1)` and
`factor = [[GXRS+0x5368]+0x80]`. The code clamps `1 − factor` to [0, `[143D7EE10]` = 1] and
lerps from `param+0x14` towards 0.

| Upload | Decompile lines | Value | Bound for |
|---|---|---|---|
| 1 | 88–96 | `(x, y) = ((param+0x14) × f, dword param+0x18)` | `EmissiveToGlowPS` and the four level composites |
| 2 | 363–370 | `(x, y) = ((param+0x20) × f, dword param+0x18)` | the final composite onto the final target |

- The shader reads only `.x`.
- STATIC (part AD, PRODUCERS-UNION-SNOW-GLOW §3): `+0x14` = gparam `Glow Filter` **Blur RgbScale**,
  `+0x18` = Blur AlphaScale, `+0x20` = **NewGlow Final ColorScale**, `+0x1C` = New Glow Enable, `+0x24` = Enable.
- Pin the three words once at runtime (NATIVE-GATES-AUDIT §4, G4).
