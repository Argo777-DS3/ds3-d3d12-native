# I-0068 part R: rendering coverage matrix (original frame → native `main` → specification)

Request (coordinator, 2026-10-10): a coverage matrix for the pilot and Thomas to plan the rest.
For each family or pass of the original frame:

- its native state on `main` (done / partial / absent), with a `file:line` proof;
- its specification note, or "no spec";
- the families no specification covers yet.

**Sources:**

- the original frame: K-0407 order (OBSERVED, m40), the I-0063 family inventory
  (`evidence/I-0063/inventory/family-status.tsv`, `drawn-families.tsv`), the five retained
  witness captures (`i63-live-10`) and the m40 listing (I-0053), as used in parts N–Q;
- the native state: code on `main` at **`2c4934bd`** (paths relative to the repository).
  Part AC re-read it at `af038661`: `native/` is unchanged, so the line references hold.
  "Done" means drawn by the production native path (`native/frame`, `native/renderer`,
  `native/content`), not by the host-era `native/world` replay.

**Legend:**

- **DONE**: drawn natively in game;
- **PARTIAL**: drawn with an explicit open policy or a known gap;
- **ABSENT**: not drawn natively (neutral input, bypass or nothing).

## 1. The matrix (in frame order)

### 1.1 Shadows and G-buffer

| # | Family / pass (original) | Native on `main` | Proof | Specification |
|---|---|---|---|---|
| 1 | **Cascaded sun shadow atlas** (8192² R16, 4 cascades) + **local-light tiles** (4096² R16), casters incl. characters | **ABSENT** | `native/renderer/model_frame.h:9` ("neutral screen-space shadow/AO") | **SHADOWS-NATIVE** (I-0068 parts O/P) |
| 2 | G-buffer M01 standard opaque scenery (map pieces, lightmaps) | **DONE** | `native/content/model_programs.cpp:48` (Gbuf/Gbuf2 recipes); `native/frame/game_bootstrap.cpp:4324` (lightmap t26); Native1045 (LEGACY-ROOT-INVENTORY-903 "First native world draw") | I-0066 DIRECT-MODEL-INPUTS, DIRECT-MATERIAL-PREPARATION |
| 3 | M02 blended ground (MultiBlend2/3/4) | PARTIAL (generic recipe and flags; not qualified per family). **Part AC:** no refusing gate found; the MultiBlend flags have no native reader except the character-part union | `native/content/model_parameters.cpp:439-441`, `native/frame/game_bootstrap.cpp:3669` | DIRECT-MATERIAL-PREPARATION (generic); NATIVE-GATES-AUDIT §3 |
| 4 | M03 parallax relief (POM: `GXFlver_ColDifSpcPom*`, `pom`, `Shininess_B_Height_A_pom`) | PARTIAL. **Part AC (STATIC): admitted**: class 1 on engine 5; programmes resolve; b2 POM words written. Missing: only the V1 image check | `native/content/model_parameters.cpp:444-450`; `native/frame/game_bootstrap.cpp:3506-3540` | REMAINING-FAMILIES §3; **NATIVE-GATES-AUDIT §2.2** |
| 5 | M04 vegetation (trees, foliage) | PARTIAL (generic); **grass** (m51: 34 draws a frame through `FUN_14029ADF0`) is an E03 path, absent. **Part AC:** the G-buffer `GXFlver_Grass*` VS reads the wind block of `cbvModelParam`. Native fills it only when the model animation has `+0x620`, otherwise the draw is refused at `:4498` | `native/content/model_programs.cpp:48`; `native/frame/game_bootstrap.cpp:4361-4365`, `:4376`; I-0064 RESULT "m51's grass" | grass: **correction, part AP (FOLIAGE-NATIVE):** the map-piece grass is not an E03 path; it is refused by the conservative read analysis of `cbvModelParam` (149 vertex shaders, all Grass) and fixed by writing the unproduced words as zero (F1); only the instanced FFX grass (m51, `_Ins@[cl]`) is E03. Other vegetation (`_l_e` Edge) is drawn; NATIVE-GATES-AUDIT §3 |
| 6 | M05 water and flows (`GXFlver_WaterBumpTri`: `WatDpt` underwater fog, then the lit surface `FwdSdw`; observed at Firelink) | **ABSENT** (shader passes 4/12 not selectable; no water-depth row; no HDR copy, reflection or interaction map) | `native/content/model_programs.cpp:41`, `native/frame/game_bootstrap.cpp:3869` | **WATER-NATIVE** (part T) |
| 7 | M06 snow on surfaces (`_SnowLt`) | **ABSENT (refused, part AC, STATIC)**: the snow PS reads three words with no native producer, so the draw stops at `:4498`. They are b9 `FC_SnowDir`/`FC_SnowTilingScale`/`FC_SnowReflectance`/`FC_GroundPlaneHeight`/`FC_RcpSnowHeightRange` (+0x60..+0x9B), b7 `FC_SnowRate` (+0x40) and b2 `FC_SnowMaskParam` (+0xA0, 22E930 flag +0x11) | `native/world/producers.cpp:372-377`, `:416-422`; `native/frame/game_bootstrap.cpp:3524-3540`, `:4380-4411`, `:4498` | **NATIVE-GATES-AUDIT §2.3**; producers and value sources ready to code in **PRODUCERS-UNION-SNOW-GLOW §2** (gparam Snow, SnowModel, FLVER `GX06`) |
| 8 | M07 emissive and glow (GBuffer5) | PARTIAL: emissive composed into the lit HDR; the glow post chain is absent. **Part AC:** the emissive map families (`*EmiIbl[Glow]`, `_em`) are refused. Class `0x100` is filtered from engine 5 (gate 21); the engine-7 row needs model `+0x224` bit 15, which native produces only for character parts | `native/renderer/scene_pass.cpp:1004` (t8 emissive); `native/content/model_pass.cpp:55`; `native/frame/game_bootstrap.cpp:3873-3878`, `:3667-3671` | MENUS-MAP §57 (emissive), TONEMAP-SKIN §17 (glow), **EMISSIVE-GLOW-NATIVE**, **NATIVE-GATES-AUDIT §2.5**; fix: the model union for every model, **PRODUCERS-UNION-SNOW-GLOW §1.4** |
| 9 | M08 translucent and veils (forward) | PARTIAL: forward blend modes 0 and 2 only. **Missing (part AC):** other blend modes; the second target (one target); depth write; IBL, SSAO, RLR and light lists (neutral) | `native/frame/game_bootstrap.cpp:4234-4237`, `:4241-4242`, `:3344-3355`, `:4264-4269` | **FORWARD-DEPTH-NATIVE**, MENUS-MAP §44–46, EMISSIVE-GLOW §5 step 1 |
| 10 | M09 ghost and phantom materials; summon signs; fog walls | **ABSENT**. Part AG: no `_Phantom_` PS selection for phantom draws (state `+0x87` ≠ 0; a normal draw needs none, part AL correction) (`native/content/model_programs.cpp:52-57`), no b2 rows 26–30, no forward list 0x21 (`game_bootstrap.cpp:3869`); signs and fog walls are SFX behind the `7CB820` deferral (`:6758`, `:8628`) | no `Phantom` material path (`native/frame/game_bootstrap.cpp:121` is the replay ghost CPU object only) | MENUS-MAP §41/§42/§47/§59/§72, **PHANTOMS-SIGNS-FOGWALLS-NATIVE** (part AG) |
| 11 | M10 interior windows (`Interior`) | **ABSENT (refused, part AC, STATIC)**: `GXFT_Emissive` gives class `0x100`, with the same refusal as row 8 (no engine-7 row for map parts). INFERRED: map parts keep `+0x224 = 0` natively | `native/content/model_parameters.cpp:424`, `:448`; `native/frame/game_bootstrap.cpp:3873-3878`, `:3939` | **NATIVE-GATES-AUDIT §2.4**; fix: **PRODUCERS-UNION-SNOW-GLOW §1.4** |
| 12 | M11 crystal (`C[Crystal]` → `RecursiveCrystal`) | **ABSENT** (not witnessed). **Part AC:** first gate, no NPC body path (row 15). Behind it: class 2 (GXFT_Recursive) → forward 0x29, blend 0 admitted, one target only (its emissive second output dropped) | `native/content/model_parameters.cpp:446`; `native/frame/game_bootstrap.cpp:4234-4241` | **NATIVE-GATES-AUDIT §2.6** |
| 13 | M12 skin, face, eyes, hair (player) | **DONE / PARTIAL**: body, armour, FaceGen face (Native1112–1113), hair (1182). **Missing:** hair depth (the 0x1F row takes the forward state with depth write off) | `native/frame/game_bootstrap.cpp:4229-4230`, `:4242` | MENUS-MAP §43–46, **FORWARD-DEPTH-NATIVE** |
| 14 | M13 equipment and cloth (player) | **DONE** (cloth static, open policy) | `native/frame/game_bootstrap.cpp:3541` (VC_isEnableCloth 1108) | MENUS-MAP §57–58 |
| 15 | M14 special creatures; **non-player characters** (enemies, NPCs) | **ABSENT** (no NPC path in production native) | no match for NPC/enemy in `native/frame/game_bootstrap.cpp` | MENUS-MAP §55 |
| 16 | M15 shadow-only casters | **ABSENT** | as row 1 | SHADOWS-NATIVE |

### 1.2 Lighting

| # | Family / pass (original) | Native on `main` | Proof | Specification |
|---|---|---|---|---|
| 17 | Depth downsample + **SSAO** (half-res, temporal) → t33 | **ABSENT** (white) | `native/renderer/scene_pass.cpp:1001` | **SSAO-NATIVE** (part Q) |
| 18 | Screen-space sun shadow + blur + **composite** → t32 | **ABSENT** (white; paired composite pipeline fed offline only) | `native/renderer/scene_pass.cpp:453` (mask root), `scene_pass.h:67` (offline inputs) | SHADOWS-NATIVE, SSAO-NATIVE |
| 19 | Env-map regions (IBL boxes) | **DONE** (1147) | `native/frame/game_bootstrap.cpp:2395`, `native/renderer/scene_pass.cpp:893` | I-0066 NATIVE-LIGHTING-MAP |
| 20 | DeferredAO, sun, CmbAlbedo, Composite with fog | **DONE / PARTIAL** (1050): sun unshadowed `DDirLight1`. **Missing:** `DDirLightShadowed1` with the shadow atlas and t32 | `native/frame/game_bootstrap.cpp:2375` (only `GXLightAcc_DDirLight1` in the name list) | NATIVE-LIGHTING-MAP, SHADOWS-NATIVE |
| 21 | Point lights (GXLightOrganize + tiles) | **DONE** (1054) | `native/frame/game_bootstrap.cpp:2376`, `:2460` | NATIVE-LIGHTING-MAP |
| 22 | Spot lights, shadowed point/spot lights | **ABSENT** | `native/frame/game_bootstrap.cpp:2412` ("Spot lights … and shadowed lights stay open") | **SPOT-LINE-LIGHTS-NATIVE** (part U), SHADOWS-NATIVE §1.5 (shadowed) |
| 23 | Line lights (`GXLightAcc_DLineLight1..4`) | **ABSENT** | no line-light path | SPOT-LINE-LIGHTS-NATIVE §4 (record format; never drawn in any capture) |
| 24 | SSS (skin) | **DONE / PARTIAL** (1137/1141). **Missing:** nothing structural. The pass is skipped in any frame where `native_sss_inputs` are unavailable | `native/frame/game_bootstrap.cpp:2390`, `:2497-2560` (profiles), `:4640` (skip) | TONEMAP-SKIN §3/§7 |
| 25 | Character light probes (per-object probe blend `1AC840`) | **ABSENT** (clock only, explicit omission) | `native/frame/game_bootstrap.cpp:5866` | **TONEMAP-SKIN §10** (I-0067; this matrix first missed it, REMAINING-FAMILIES §1) |
| 26 | SSR, AO fields, cloud shadows (`GXSsr`, `GXLightDeferredAOWithSsr`, `GXAOFields_*`, `DDirLightShadowed1_Cloud`) | **ABSENT** | no matches | REMAINING-FAMILIES §2 (never drawn in any witness: native draws nothing) |
| 27 | Height and distance fog (D01) | **DONE** within the composite and forward `cbFog` | `native/frame/game_bootstrap.cpp:2672`, `:2813` | NATIVE-LIGHTING-MAP |

### 1.3 Forward, effects and post chain

| # | Family / pass (original) | Native on `main` | Proof | Specification |
|---|---|---|---|---|
| 28 | **Forward rows 0x1F / 0x29** | PARTIAL: drawn (1115–1117, 1181). **Missing (part AC):** depth write (`:4242`); the second target [HDR, GB5] (one target, `:4241`); blend modes other than 0/2 (`:4234-4237`); IBL black, SSAO white, RLR zero, light lists empty and grid words zeroed (`:3344-3355`, `:4264-4269`) | `native/frame/game_bootstrap.cpp:4231`; LEGACY-ROOT-INVENTORY-903 "Native1115-1117" | **FORWARD-DEPTH-NATIVE** (part N), EMISSIVE-GLOW §5 step 1 |
| 29 | **Deferred decals** (`gxdecal`: 47 programmes, box-projected into the G-buffer; observed `Decal_DifSpcAm` at Firelink) | **PARTIAL**: the CPU manager, requests and programme lookup are native. **Missing:** the GPU resources and the draw (the consumer `19B690` is replaced by `UnownedNativeDecalSceneConsumer`); the receiver stencil bit (characters are marked as receivers) | `native/frame/game_bootstrap.cpp:810`, `:977`, `:8522`, `:4211` | **DECALS-NATIVE** (part S) |
| 30 | Blood on materials (`Bloody`, `BloodBlend`, decal provider) | PARTIAL (provider bound, policy flag). **Missing:** the hit blood decals (row 29); nothing on the material side | `native/frame/game_bootstrap.cpp:3183`, `native/content/model_parameters.cpp:415` | the material side: DIRECT-MATERIAL-PREPARATION; the hit blood decals: DECALS-NATIVE (`Decal_Blood_DifSpcBump`) |
| 31 | Frame copy (HDR → `g_frameBufferTexture`) | **ABSENT** | no frame-copy pass | FFX-FAMILIES-PHASE2 §8, EFFECTS-PHASE2 §8 |
| 32 | E01 billboards / E02 trails | **ABSENT** (no FFX code in production native) | no `GXFfx` match in `native/frame` | **EFFECTS-PHASE1-NATIVE**, **EFFECTS-PHASE2-E01-E02** |
| 33 | E03 model particles (and m51 grass) | **ABSENT** | as row 32 | **EFFECTS-E03** |
| 34 | E04 distortion / E05 radial blur | **ABSENT** | as row 32 | **EFFECTS-E05-E04** |
| 35 | E06 effect lights | **ABSENT** | as row 32 | **EFFECT-LIGHTS**, EFFECTS-PHASE1 |
| 36 | E07 GPU particles (Ariandel snow) | **ABSENT** | as row 32 | **EFFECTS-E07** |
| 37 | E08 effect light shafts (m30/m33/m38 only) | **ABSENT** | as row 32 | WATER-REFLECTION-INTERACTION-E08 §3 (rides on E01/E02; no witness) |
| 38 | Ambient effects A01–A12 (fog, snow, wind, ash, fire, leaves… as FXR regions) | **ABSENT** | as row 32 | through the E-family specs (they are FXR actions of E01/E02/E03/E07) |
| 39 | Velocity pass and motion blur (`_Velo`, `MotionBlur*`; setting-dependent) | **ABSENT**. Real family since Thomas enabled the option (2026-10-10) | pass table only, `native/frame/game_bootstrap.cpp:1818` | **VELOCITY-MOTIONBLUR-NATIVE** (part AE: gates, group 0x33 velocity pass, `Velo` programmes, YEBIS blur, captures M1–M5, upscaler preparation); skinning side I-0067 VELOCITY-SKINNING |
| 40 | **YEBIS** (DoF, glare/bloom, afterimage, light shafts, tone map, grading) | **ABSENT** (no tone map: the native world is brighter) | no YEBIS match in production native | **YEBIS-NATIVE**, **YEBIS-TECHNIQUES**, **YEBIS-HAL-READY** |
| 41 | GX after-chain: `PointSampleColor1`, DLAA/FXAA, glow (`EmissiveToGlow`, `GlowComposite`) | **ABSENT** | name list only, `native/frame/game_bootstrap.cpp:2325` | TONEMAP-SKIN §17 (owner) + **EMISSIVE-GLOW-NATIVE** (part AA, ready to code; `g_GlowScaleOffset` per step decoded in §8.3, part AC) |
| 42 | Water interaction ripples (D07), E09, E10 | **ABSENT** | — | WATER-REFLECTION-INTERACTION-E08 §2 (neutral map exact for all witnesses; simulation needs a witness) |

### 1.4 Interface, video and presentation

| # | Family / pass (original) | Native on `main` | Proof | Specification |
|---|---|---|---|---|
| 43 | **UI Scaleform** (HUD, menus) | **DONE** (CPU capture + native draw) | `native/frame/game_bootstrap.cpp:6032` (`PrepareFrame`) | MENUS-MAP, I-0066 UI-INGRESS |
| 44 | Menu offscreen views (Status, ChrMake, portraits) | **DONE / PARTIAL** (1169–1182). The view tone map is written for modes 0 and 7 with UseTable, seen live in mode 0 (I-0113 tranche 27 after the merge: the Quit Game portrait's face matched, mode 7 not yet seen) (the GX filter, not YEBIS: I-0113 RESULT « Tranche 27 »). **Missing:** the Player-view shadow (rides on SHADOWS, row 1) | `native/frame/game_texture_ingress.h:20` | MENU-OFFSCREEN-MODELS, MENUS-MAP §48 (shadow) |
| 45 | FaceGen face composition | **DONE** (1113) | LEGACY-ROOT-INVENTORY-903 "Native1113" | — |
| 46 | **Bink** opening video | **ABSENT** (bypass) | `native/frame/game_bootstrap.cpp:5817` (`NativeMoviePlayerFactory` returns null), `:7836` (black placeholder) | **BINK-NATIVE-READY** |
| 47 | **Gamma pass** and presentation | PARTIAL: plain final display. **Missing:** the brightness curve pass | `native/renderer/scene_pass.cpp:464` (`FinalDisplay`) | **DISPLAY-SWAPCHAIN-NATIVE §9** |
| 48 | Sky | not a separate family in the programme banks. INFERRED: map FLVER models with ordinary materials (rows 2–3). To verify on a witness | — | — |

## 2. Families with no specification yet

In planning order (visibility × presence in the five zones):

1. **Deferred decals and blood decals** (`gxdecal`, row 29): Firelink and fights (none in the five witness captures).
   **Done: part S, DECALS-NATIVE.**
2. **Water** (M05, row 6): **done, part T, WATER-NATIVE**. Its interaction (D07/E09, row 42) and the planar reflection view remain without a spec.
3. **Spot and line lights** (rows 22–23): **done, part U, SPOT-LINE-LIGHTS-NATIVE** (line lights: never drawn, record only).
4. **Character light probes** (row 25): already specified in I-0067 TONEMAP-SKIN §10 (part V §1).
5. **Velocity / motion blur** (row 39): never drawn (MotionBlur DISABLE): native draws nothing (part V §2).
6. **SSR, AO fields, cloud shadows** (row 26): never drawn in any witness: native draws nothing (part V §2).
7. **POM, snow on surfaces, interior windows, crystal** (rows 4, 7, 11, 12): no new pass (part
   V §3). The gates were read in part AC (NATIVE-GATES-AUDIT):
   - POM is admitted;
   - `Interior` and the Emi map families need the engine-7 row for map parts;
   - snow needs three producers;
   - crystal waits for NPC bodies.
8. **E08** (row 37), the water reflection view and the water interaction (D07): part W, WATER-REFLECTION-INTERACTION-E08 (E08 and the interaction simulation await a witness). Character-attached effects: part X, CHARACTER-EFFECTS-MAP.

## 3. Reading for the pilot

- **The biggest visible gaps with a ready spec:**
  - YEBIS (tone map, bloom, DoF);
  - SSAO;
  - shadows (world and characters);
  - the effects E01/E02 (fire, embers) and E07 (snow);
  - forward depth;
  - gamma;
  - Bink.
- **The specs exist; the work is native implementation.**
- **The first gaps without a spec** were decals (part S, now specified), then water.

## Evidence

The proofs above are line references on `main` (`2c4934bd`); family names from
`evidence/I-0063/inventory/family-status.tsv`; programme names from
`evidence/I-0063/artifacts/program-resources.tsv`.
