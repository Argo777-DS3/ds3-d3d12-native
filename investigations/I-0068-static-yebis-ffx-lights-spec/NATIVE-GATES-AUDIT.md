# I-0068 part AC: native gates for the material variants and the partial matrix rows

Request (coordinator, 2026-10-10):

1. decode `g_GlowScaleOffset` per step;
2. read, statically, the native code (`native/`) for checks V1–V3 of the material variants
   (POM, `Interior`, `_SnowLt`, crystal). For each family, name the native gates that would
   refuse it today, with file and line;
3. do the same for the COVERAGE-MATRIX rows marked partial, and list what is missing at file and
   line;
4. update the matrix.

Labels: STATIC, OBSERVED, INFERRED.

**Code read:** `main` at `af038661`. `native/` is unchanged since the matrix pin `2c4934bd`
(`git diff --stat 2c4934bd af038661 -- native/` is empty), so the matrix line references stay
valid.

**Evidence** (`evidence/I-0068/queries/variants/`):

- reflection and disassembly of the variant programmes from the local I-0063 programme cache;
- the disassembler script.

## 0. Answer first

| Family (row) | Would native draw it today? | First refusing gate (STATIC) |
|---|---|---|
| **POM** `GXFlver_ColDifSpcPom*` (V1, row 4) | **yes**: no gate refuses it | none; only the generic gates every map piece passes |
| **`Interior`** (V2, row 11) | **no** | class `0x100` (GXFT_Emissive) is filtered from engine pass 5 (`native/content/model_pass.cpp:51`, `:55`; gate 21 at `native/frame/game_bootstrap.cpp:3939`). The only row that takes `0x100`, engine pass 7, is added only for a model with `+0x224` bit 15 (`game_bootstrap.cpp:3873-3878`). Native produces that word only for character parts (`:3667-3671`). INFERRED: map parts keep it at 0 |
| **`_SnowLt`** (V3, row 7) | **no** | the snow PS reads three constant words native never produces. The draw is refused at `game_bootstrap.cpp:4498` (`native_selected_constant_unavailable`): b9 `FC_Snow*`, b7 `FC_SnowRate`, b2 `FC_SnowMaskParam` (§2.3) |
| **Crystal** `C[Crystal]` → `RecursiveCrystal` (row 12) | **no** | no native NPC body path (row 15). Behind it: class 2 (GXFT_Recursive) → forward row 0x29, which binds one target (`:4241`) |
| **Emi map families** `GXFlver_ColDifSpcBumpEmiIbl[Glow]`, `_em` MTDs (row 8) | **no** | the same as `Interior`: class `0x100`, no engine-7 row for map parts. This is the cause of the dark sconce bowl at the High Wall (MENUS-MAP §57.3, case 1/4) |

**`g_GlowScaleOffset` (§1):**

- levels: `x = (param+0x14) × clamp(factor, 0, 1)`;
- final composite: `x = (param+0x20) × clamp(factor, 0, 1)`;
- `y = param+0x18`, never read by the shader.

## 1. `g_GlowScaleOffset` per step (STATIC, `1BB810`)

The filter `1BB810(filter, frame, param)` uploads PS b2 `cbGlowParam` twice.

**Factor:** `factor = [[GXRS+0x5368]+0x80]` (gate: > 0). The code clamps `1 − factor` to
[0, 1] (constant `[143D7EE10]` = 1.0) and lerps with it, so the effective scale is
`clamp(factor, 0, 1)`.

| Upload | Decompile lines | `x` | `y` | Used by |
|---|---|---|---|---|
| 1 | 88–96 | `(param+0x14) × f` | dword `param+0x18` | `EmissiveToGlowPS` and the four level composites (L4→L3 … L1→source) |
| 2 | 363–370 | `(param+0x20) × f` | dword `param+0x18` | the final `GlowCompositeSinglePS` (source → final target) |

- `GlowCompositeSinglePS` reads only `.x` (EMISSIVE-GLOW §2).
- The `Sample13Weight` blurs take `cbFilterParam`/`cbSampleOffset` instead (EMISSIVE-GLOW §8.2).
- STATIC (part AD, PRODUCERS-UNION-SNOW-GLOW §3): `+0x14` = gparam `Glow Filter` Blur RgbScale and
  `+0x20` = NewGlow Final ColorScale. Values: the resolved gparam entry, or the built-in record (3 and 1). Read
  `param+0x14/+0x18/+0x20` once at runtime to pin them.

## 2. The material variants (V1–V3, crystal)

### 2.1 The native path a map material takes (STATIC)

| # | Step | Code | Refuses when |
|---|---|---|---|
| 1 | MTD builtins → policy | `native/content/model_parameters.cpp:409-428` | builtin kind 5, 8, 13 or > 41 (`:427`). None of the four families uses one (all MTDs read; `g_DepthBias`, kind 8, occurs in no MTD) |
| 2 | Family flags | `model_parameters.cpp:429-441` | — (DOL `0x400\|1`, Emi `0x8000`, Glow `0x10000`, MultiBlend `0x800000`/`0x1000000`/`0x2000000`) |
| 3 | Class | `ClassifyModelMaterial`, `model_parameters.cpp:444-450` | — (special → 0x10/8; transparent or bit `0x200000` (Recursive) → 2; forward → 0x80; Emi `0x8000` → **0x100**; alpha-ref → 4; else 1) |
| 4 | Rows | `game_bootstrap.cpp:3869`: engine 5 (mask 5, or split 1/4), 6 (8), **7 (0x100, only with model `+0x224` bit 15, `:3873-3878`)**, 0x1F (0x80, only with `+0x224` bit 12, `:3879-3880`), 0x29 (0x12) | gates 1–9 (`:3881-3900`); `EvaluateOrdinaryModelPass` gates 10+reason (`:3939`), class at `model_pass.cpp:55` |
| 5 | Programmes | `native/content/model_programs.cpp:34-61`: `<family>_Gbuf[_Skin][_Ins][@[cl]].vpo`, `<family>_Gbuf[_[A]].ppo`, case-insensitive (`:6-8`) | missing or ambiguous programme → gate 30 (`game_bootstrap.cpp:3958-3969`) |
| 6 | Instancing, deformation | `game_bootstrap.cpp:3954-3956` | instanced mesh (28), deformation from material `+0x250` (27; tessellation mode 0 in all four families) |
| 7 | Textures | `game_bootstrap.cpp:3359-3405` | a declared register with no binding or the wrong dimension |
| 8 | Constant blocks | `game_bootstrap.cpp:4251-4498` | **any word the shader reads that no producer wrote** (`:4498`) |

### 2.2 V1 POM (`GXFlver_ColDifSpcPomDOL`, `…PomMulMaskDOL`, `…PomIbl`, `…PomMulMaskIbl`; `M[ARSN]_2m_pom` → `MultiBlend2_et1_p`)

**Verdict: native draws it today** (STATIC). Step by step:

- **Builtins:** they use none of the refused kinds. `g_DisplacementEnable = 1` (`_l_p_m`, `_p`,
  `_p_m`) changes no native field.
- **Class:** 1 (blend 0, alpha-ref 0) → engine 5.
- **Programmes:** `…PomDOL_GBuf.vpo` and `…PomDOL_Gbuf.ppo` both exist and match
  case-insensitively.
- **b2 POM words** (`FC_BlendMaskParam` +48, `FC_PomHeightScale/Min/MaxSamples` +64/+72/+76,
  `FC_Pom2HeightScale` +112, `FC_TranslucencyScale` +16): the 22E930 blocks write them, as
  zero when the flag is clear (`game_bootstrap.cpp:3506-3540`).
- **b3, b8, b9 Dominant, b0:** the same producers as M01 DOL.

**What V1 still has to show:** only the image. Compare GB0–GB4 at m31/m37 against the
original. A POM mismatch would come from the data (height scale, samples), not from a gate.

### 2.3 V3 `_SnowLt` (`GXFlver_ColDifSpcBump[Mul][Mask][Env]{DOL,Ibl}_SnowLt`; `M[ARSN]_l_snow`, `_snow`, `_l_e_Snow`, `_al_Snow`)

**Correction to REMAINING-FAMILIES §3:**

- the snow variant is in the **pixel** shader: the `_SnowLt_GBuf` VS declares the same blocks as
  the plain VS (b4, b5, b8, b11);
- the snow PS adds t15 `g_SnowMaskTexture`, t47 `g_SnowDiffuseTexture` (material textures),
  b7 `cbModelParam` and the cbPerFrame snow rows.

**Verdict: refused today** (STATIC). The class (1; 4 for `_l_e_Snow`; 2/forward for
`_al_Snow`) and the programmes pass. The draw then stops at `game_bootstrap.cpp:4498` because
three read words have no native producer:

| Block | Words the PS reads | Native producer (and what it lacks) | Original producer |
|---|---|---|---|
| **b9 `cbPerFrame`** | `FC_SnowDir` +0x60, `FC_SnowTilingScale` +0x70, `FC_SnowReflectance` +0x90, `FC_GroundPlaneHeight` +0x94, `FC_RcpSnowHeightRange` +0x98 | `ProducePerFrame` writes only +0xB0 (time) and +0xC0..+0xDF (`native/world/producers.cpp:416-422`; call `game_bootstrap.cpp:4404-4411`) | `23790` (decompile lines 307–336): the frame's two snow gparam entries `12F400([144961B50]+0x45F8, frame+0x3FC0 / +0x3FC8)`, blended by `131780` with `frame+0x3FD0`. +0x70 = `{a, b, (1−a)·k, (1−b)·k}`, +0x98 = `1/(c − d)` (k = `[143D7EDB4]`) |
| **b7 `cbModelParam`** | `FC_SnowRate` +0x40 (+0x44/+0x48 are produced) | `ProduceModelParamPs` skips +0x40, and +0x14/+0x18 (`producers.cpp:372-377`; call `game_bootstrap.cpp:4380-4403`) | `1A86C0` lines 206–223: `lerp(L(state+0x2B0)+8, L(state+0x2B8)+8, state+0x2C0) × [[144961B50]+0x4E70]`, with `L = 46300([144961B50]+0x4600, ·)`. INFERRED: `+0x4E70` is the word `23790` writes from the same `131780` blend (`param_1+0x4E70`, line 321) |
| **b2 `cbMatDynParam`** | `FC_SnowMaskParam` +0xA0 | the 22E930 blocks (`game_bootstrap.cpp:3524-3540`) lack this one | `22E930` flag `source+0x11` (asm `14022F4BD`): `+0xA0 = {s, 0, −0.5·s, 0}` (`[143D7F0C0]` = −0.5, EMISSIVE-GLOW §8.2), with `s = source+0x3C` |

**Order to admit it:** the b2 block (one line in the 22E930 transcription), then `FC_SnowRate`,
then the cbPerFrame snow rows (the native per-frame producer already reads the `134A20` rows
and the blend; the snow rows are the same pattern on `12F400`).

**V3:** after that, compare GB0–GB4 at m31 against the original.

### 2.4 V2 `Interior` (`M[Interior]` → `sat/…/Interior.spx`)

**Verdict: refused today.**

- **Class (STATIC):** `GXFT_Emissive=True` (kind 35 → bit 15 = `0x8000`,
  `model_parameters.cpp:408`, `:424`) → **class `0x100`** (`:448`).
- **Engine 5:** mask 5 filters it: reason MaterialClass, gate code 21
  (`model_pass.cpp:51`, `:55`; `game_bootstrap.cpp:3939`).
- **Engine 7** (mask `0x100`): added only when the model's `+0x224` has bit 15
  (`game_bootstrap.cpp:3873-3878`).
  - For map parts, native reads that word live from the game model.
  - Native writes its own union only for character parts (`c.neutral_instance_decal`,
    `:3667-3671`).
  - INFERRED (as Native1101 found for character parts): the original accumulating writer does
    not run under native, so map parts keep `+0x224 = 0` and the row is never added.
- **Behind that gate:**
  - the snp words are in **b2 `cbMatDynParam` +624..+656**, not b3 (correction to
    REMAINING-FAMILIES §3);
  - the SPX generated parameters produce them (`game_bootstrap.cpp:3422-3429`, as for the
    `SAT_Equip` word +624);
  - the PS writes **6 targets** (GB5 emissive); engine 7 draws with the 6-target G-buffer state.

**To admit it:** produce `+0x220`/`+0x224` for map-part models from the native classification,
as `:3667-3671` does for character parts, or read the original writer once it is located. Then
V2 compares GB0–GB5 at m37.

### 2.5 Emissive map families (row 8): the same gate

`GXFlver_ColDifSpcBumpEmiIbl[Glow]` (`_em` MTDs; the High Wall sconce bowl) contain `Emi`
(`model_parameters.cpp:437`) → class `0x100` → the same refusal as `Interior`.

This settles MENUS-MAP §57.3's first and fourth candidates:

- the family is not drawn at all;
- class `0x100` has no row for map parts.

So EMISSIVE-GLOW step 2 ("admit the Emi families") is this one change.

**Correction (part AL, NPC-FAMILIES-STATIC §2.2):** not the only change. The mesh also needs `FC_EmissiveColor` (b2 +0x20), which the native producer (`game_bootstrap.cpp:3586`) writes only when the material's GX03 flag is set; that flag is written by the GX item parser `141070`, which `BuildModelMaterial` does not run. Without it the emission is black even once the union lets the row exist. The same union fix is also needed for enemy models (`SprjModelDrawEntity`), where it is not produced either (§2.1 there).

### 2.6 Crystal (`C[Crystal]` → `sat/…/RecursiveCrystal.spx`)

**Verdict: refused today.**

**First gate:** native draws no NPC or enemy body (matrix row 15). Crystal belongs to
`c3100` (I-0064).

**Behind it (STATIC):**

- `GXFT_Recursive=True` (kind 39 → bit 21 = `0x200000`) → **class 2**
  (`model_parameters.cpp:446`) → forward row 0x29 (mask `0x12`);
- blend 0 is admitted (`game_bootstrap.cpp:4234-4237`).
- `RecursiveCrystal_Fwd` reads:
  - t41/t42 decal textures (bound for character parts, `:3340-3343`);
  - IBL (black), SSAO (white), RLR (zero) and the light buffers (empty): explicit policy at
    `:3344-3355`;
  - b5/b6 `cbLight`/`cbFog` (`:4424-4428`);
  - b3 `FC_GlowScale`/`FC_BloodAmount`.
- It writes **two** targets; native binds one (`:4241`). The emissive second target is dropped
  until EMISSIVE-GLOW step 1.

**To admit it:**

- the NPC body path (pilot, MENUS-MAP §55);
- then the forward [HDR, GB5] targets.

**Witness:** a capture with `c3100`.

## 3. The partial matrix rows: what is missing (STATIC, `main`)

| Row | Missing | Where |
|---|---|---|
| 3 M02 MultiBlend | no refusing gate found. The MultiBlend flags are set (`model_parameters.cpp:439-441`), and nothing reads them except the character-part union (`game_bootstrap.cpp:3669`). INFERRED: in the original they feed the model `+0x224` union like the Emi bit; their 57A20 row effect is unread. Remaining: image qualification per family | — |
| 4 POM | nothing (§2.2); V1 image | — |
| 5 vegetation | **grass E03** draws are absent (no FFX code). The G-buffer grass `GXFlver_Grass*` VS reads the `cbvModelParam` wind block +0x80..+0x48F. Native fills it only when `model.animation` has `+0x620` set (`game_bootstrap.cpp:4361-4365`), otherwise the draw stops at `:4498` (`native_vmodel_param_unproduced`, `:4376`) | **Correction (part AP, FOLIAGE-NATIVE §7-§8):** the map-piece grass is not E03. `+0x70..+0x7F` and `+0x490..+0x4AF` are never produced and the read analysis counts every word of a register-indexed buffer as read, so the draw stops even with `+0x620` set; the compiler marks those words unused. Fix F1: write them as zero. Only the instanced FFX grass is E03 | FOLIAGE-NATIVE; `:4344-4379` |
| 7 snow | the three words of §2.3 | `producers.cpp:372-377`, `:416-422`; `game_bootstrap.cpp:3524-3540` |
| 8 emissive | the engine-7 row for map parts (§2.5); the glow chain (row 41); forward GB5 (one target, `:4241`) | `game_bootstrap.cpp:3873-3878`, `:3667-3671`, `:4241` |
| 9 forward translucent | blend modes other than 0 and 2 are refused (`game_bootstrap.cpp:4234-4237`); one target (`:4241`); no depth write (`:4242`); IBL black, SSAO white, RLR zero, light lists empty (`:3344-3355`, `:4264-4269`) | FORWARD-DEPTH-NATIVE |
| 11 Interior | the engine-7 row (§2.4) | `game_bootstrap.cpp:3873-3878` |
| 12 crystal | the NPC body path; forward GB5 (§2.6) | row 15; `:4241` |
| 13 hair | depth not written on the 0x1F row (`game_bootstrap.cpp:4229-4230`, `:4242`) | FORWARD-DEPTH-NATIVE |
| 20 sun | `GXLightAcc_DDirLight1` (unshadowed) is the only sun programme (`game_bootstrap.cpp:2375`); `DDirLightShadowed1` needs the shadow atlas and t32 | SHADOWS-NATIVE |
| 24 SSS | the pass is skipped when `native_sss_inputs` are unavailable (`game_bootstrap.cpp:4640`); the profile table is native (`:2497-2560`) | TONEMAP-SKIN §3/§7 |
| 28 forward rows | as row 9, plus the readable-depth copy and depth write for 0x1F | FORWARD-DEPTH-NATIVE |
| 29 decals | the draw consumer is replaced by a refusal (`UnownedNativeDecalSceneConsumer`, `game_bootstrap.cpp:977`, hooked at `:8522`); G-buffer stencil receiver bit for characters (`:4211`) | DECALS-NATIVE |
| 30 blood | the material side is drawn (flag 4, `model_parameters.cpp:415`; provider `game_bootstrap.cpp:3183`); the hit decals wait for row 29 | DECALS-NATIVE |
| 44 menu views | view tone map written for modes 0 and 7 with UseTable, seen live in mode 0 (I-0113 tranche 27 after the merge: the Quit Game portrait's face matched, mode 7 not yet seen) (GX filter; I-0113 RESULT « Tranche 27 »); no Player-view shadow | MENUS-MAP §48 |
| 47 gamma | `FinalDisplay` copies without the brightness curve (`native/renderer/scene_pass.cpp:464`) | DISPLAY-SWAPCHAIN-NATIVE §9 |

A generic gate common to all map families: an instanced mesh is refused (gate 28,
`game_bootstrap.cpp:3954-3956`). It is not specific to any row above.

## 4. Runtime checks (read-only, for the next native run)

| # | Check | Expected |
|---|---|---|
| G1 | at m37: log `+0x220`/`+0x224` of the map-part models whose meshes classify `0x100` (extend the 1101 witness at `:3875` to `asm_part < 0`) | `+0x224` bit 15 clear; confirms §2.4/§2.5 |
| G2 | at m31: count `native_selected_constant_unavailable` with family `*_SnowLt` and slots 9/7/2 | refusals with `FC_Snow*` words; confirms §2.3 |
| G3 | at m31/m37: count POM-family prepared meshes and draws | drawn; then V1 image |
| G4 | `param+0x14/+0x18/+0x20` of the `GXGlowFilter` param, and the factor `[[GXRS+0x5368]+0x80]` | pins §1 |
