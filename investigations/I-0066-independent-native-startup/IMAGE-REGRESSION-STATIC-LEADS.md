# Native scene too bright and washed out (Firelink ash white, Village veiled): static leads, ranked

Thread « Statique régression d'image » (2026-10-16), read-only for the pilot thread « Tests des menus ». Nothing was run in
the game; no save, no `main`. Labels: **OBSERVED** (an artifact I read or measured offline), **STATIC** (code or data read),
**INFERRED** (stated reasoning, not witnessed). Method rule kept: every lead below is a *mechanism* (a family of inputs), never a
fix for one zone.

## 0. What the pictures say (OBSERVED)

Native presented frame against the original's tone-map output on the same save and camera. Sampled luminance (sRGB, 0-255), patches
of 3-pixel steps; the original side is `reference-tonemap-output.raw` (before the grading and the display curve; the grading is bit
exact, so this does not create a native-only gap, but the ratios below are against the *ungraded* original: redo them against the
original's graded frame before quoting them).

| Zone (original capture / native run) | Surface | Original | Native | Remark |
|---|---|---|---|---|
| m40 (Reference1705 / Native1710) | ash mound, left | 39 | 126 | upward-facing |
| | floor right | 44 | 150 | upward-facing |
| | steps right | 35 | 113 | upward-facing, sun-side |
| | floor front-left | 61 | 121 | |
| | wall upper right | 32 | 45 | vertical |
| | back wall centre | 38 | 52 | vertical |
| | bonfire | 162 | 179 | point-light dominated: close |
| m37 (Reference1643 / Native1635 composition, 1644 tone map) | floor right | 47 | 133 / 156 | |
| | whole frame | 54 | 69 / 78 | |
| m31 (Reference1646 / Native1647) | whole frame | 55 | 122 | beige veil over everything, including the near beam |

Reading: the excess is **large on upward-facing surfaces (x3 in sRGB, about x10-x15 linear), small on vertical ones (x1.4) and
absent where a local light dominates** (the bonfire). The native ash is cold white with sparkle; the original is dim blue-grey. The
original's m37 floor carries the statue's shadow band and a wet reflection near the fire; native has neither. The m31 native
frame has a veil that lifts the blacks and desaturates, also on near objects (not distance fog). The character is *black* at m37 and
the hair is missing at m31/m37.

**Not a W10 effect (OBSERVED).** At m37 the sRGB composition (Native1635, no YEBIS pass) is already x2.8 on the floor; the YEBIS tone
map only adds about 8-15 % (Native1644). W10-NATIVE-PASSES §4 already wrote « the scene HDR, not the tone map » at m37 and m31 and
left it open. The tone words at m40 (exposure 2.3, contrast 0.15, ColorS 0.8) are the file's; `no-yebis*.flag` can only change
how the same HDR is shown, not why it is too bright.

## 0b. Measured live (pilot, 2026-10-16, OBSERVED)

The native lit HDR (R16G16B16A16, after the forward pass, before every YEBIS pass) is now written beside the presented picture
(`world-N-hdr.rgba16f`, `world-pictures.flag`) and compared with the original's tone-map input (`reference-tonemap-t0.raw`,
Reference1705, same save Eta and camera) by `Compare-SceneHdr.ps1` (mean linear luminance per region, native / original):

| Run (m40, frame 2400) | whole | floor right | floor left | wall | arch | character |
|---|---|---|---|---|---|---|
| Native1716 (defaults) | 4.58 | 6.27 | 12.19 | 3.11 | 2.64 | 1.69 |
| Native1717 (`no-yebis`) | 4.58 | 6.28 | 12.19 | 3.11 | 2.64 | 1.72 |
| Native1718 (`no-sun.flag`: the DDirLight pass skipped) | 0.87 | 0.76 | 0.59 | 0.91 | 1.13 | 0.24 |

- The excess is in the scene before YEBIS (1716 = 1717): not the exposure, the tone map or the grading.
- Without the sun the native scene falls to or below the original (0.6-0.9): **L1 confirmed**, the unshadowed, unoccluded sun is the
  whole excess at m40; the original's floor here receives almost none of the sun (shadow and AO).
- The character without the sun is 4 times too dark: the instance ambient SH rows are zero (**L2**, the black character).
- `native_dominant_rows` (Native1718): keys `-1` and the bank's built-in record `a = b = (0.5, 0, 1.25, 1, 1, 1, 1, 1, 1, 1, 1)`,
  blend 0, where `m40_00_0000` gives the Dominant Light group (0.5, 1, 0.86, 1.2, 0.6, 0.8, 1.0): **L2 confirmed** for the
  Dominant rows (default record; the keys themselves are unset).
- Decision D-0180 (Thomas, 08:57 Paris): the sun shadow mask and the SSAO join wave 1.
- **Resolved at m40 (Native1737, 2026-10-16):** with the sun shadow (D-0180 C4) and each model's light set index (envData,
  [SUN-SHADOWS-WAVE1](SUN-SHADOWS-WAVE1.md) §3), the lit HDR is 1.03 times the original (floor 1.01 and 1.00, wall 1.23,
  arch 1.07, character 0.98). L1 was the shadow plus the set index. The character region is at 0.98 here; whether L2 (instance
  SH rows zero) still shows elsewhere stays open.
  Remaining at m40: the wall (1.23) and the bonfire's fire (effects). Other witnesses and the second sky state: to compare.
## 1. Ranked leads

### L1. The sun is not shadowed and nothing is occluded: the shadow mask t32 and the AO are neutral white (STATIC, certain; size: dominant for upward faces)

- `SHADOWS-NATIVE.md` §0: native draws **no** world or character shadow; the sun is `GXLightAcc_DDirLight1` (unshadowed) and the
  screen-space mask t32 is a 1x1 white (`scene_pass.h` `native_light_neutral_`, bound as t32 in the lighting passes; frame-graph
  sheet row 5 « shadow mask chain: planned »). SSAO and the shadow atlas are W8/W9 resources « inert until the wave-2 passes »
  (STATE). The original at m40 runs both (K-0407, OBSERVED live frame m40).
- The data say the sun matters at m40: `m40_00_0000` LightSet id 100 (the id Native1710 uses) has a sun of colour
  (0.78, 0.85, 1.0), intensity 0.8, SsaoWeight 0.75; id 0 has 0.8 and 0.75. The three surfaces that gain x3 are exactly those a
  sun at about 33 degrees elevation lights (tops of steps, mounds, floor); the walls gain little.
- This is **not a regression**: no native build ever had these two terms. It was masked while only per-pass bit-exactness was checked
  and the composition was compared by eye.
- **Discriminating test (cheap, native only):** a development flag that skips the `DDirLight` pass (or multiplies the sun by 0) at
  m40 Eta: if the ash/floor/steps luminance falls to about the original's (39-61), L1 is the cause for those surfaces. The ratio
  should stay high on the bonfire and on point-lit surfaces.
- **What it implies (decision for Thomas, not mine):** « wave 1 = identical base » cannot hold visually without the screen-space sun
  shadow, its composite with SSAO, and the AO. ROADMAP puts them in wave 2 (SSAO S3-S8, shadow mask and composite). Thomas's new
  rule (side-by-side check on every witness before closing a visual workstream) conflicts with that order; the ready sheets
  (SSAO-WAVE1-READY, SHADOWS-WAVE1-READY, SSAO-D3D12-DESIGN-NATIVE, SHADOWS-NATIVE) say what to build.

### L2. The engine's GX parameter banks never receive a file natively, so every consumer that still asks the engine's own lookup gets built-in defaults (STATIC + OBSERVED, certain; size: tint and ambient, and character darkness)

- Mechanism (STATIC): `FUN_140049D00` is the dispatcher that binds one gparam file into about 26 banks (`+0x28` 37340 LightSet,
  `+0x30` 3BC70 Fog, `+0xE8` 3FEB0 EnvMap, `+0x4C0` 134150 the Dominant Light bank, ...). Each binder reads the file through the
  legacy GX registry `[root+0x4788]` with a GX handle. Natively 49D00 is replaced by `BindNativeGparamScene` (bookkeeping only,
  `game_bootstrap.cpp:12552`), files are CPU-only (`legacy_handle:false`) and the comment at `game_bootstrap.cpp:4914` says it:
  « the engine's GX parameter banks never receive a file ... 32770/38E70 return their built-in default for every key ». The
  light, fog, tone map, bloom, glare, DoF, motion blur, shaft, AA, grading, glow, SSAO and shadow records were each
  re-implemented from `ds3light::Banks` (`light_producers.cpp`). **The consumers not yet moved still read defaults:**

| Consumer (native) | Engine lookup | What the original reads | Native today |
|---|---|---|---|
| cbPerFrame b9 `+0xC0..+0xDF`: Dominant Light rows (sharpness min/offset/exponent, colour scale, R/G/B scale) for every DOL lightmap material | `134A20` on `[root+0x4680]`, keys `ctx+0x4110/+0x4118` (`game_bootstrap.cpp:8298`, `ProducePerFrame` `producers.cpp:416`) | the file's `Dominant Light` group | the bank's default record (the draw is not refused, so a record is returned) |
| instance ambient SH rows (+0x80..+0xAF of the instance block) for characters, NPCs, objects (draw types 0/3/4) | `32A80` on `[root+0x41E0]`, record `+0x290` (12 floats) and `+0x2C0` (`game_bootstrap.cpp:4053`) | the light set's SH rows | **zeros**: Native1710 rows `native_instance_sh` `ash` = 0 x 12, `native_instance_probe_inputs` `bank0_290` = `bank1_290` = 0, `probe88` = 0 |
| env-map region colour scale `f[36]` | `416A0` entry `+0xC` (`game_bootstrap.cpp:4837`, `:4860`) | `EnvMap:ColorScale` of the region's id | the default entry (« Native1150 »), so 1.0 |
| `FC_EnvMapColorScale0/1` (`cbModelParam +0x14/+0x18`), snow words | `416A0`, `12F400`, `46300` | EnvMap, Snow, SnowModel groups | not produced (a shader that reads them is refused; none at m40: 0 refusals, STATIC) |
| LightScattering group, bank behind `FUN_14003C900` | ? | ? | no native reader at all (consumer to find) |
| point-light settings (`root+0x4610`, `15E920`, `CollectNativePointLights`) | `15E920` | ? | `settings [10,10,130,1.2]` in the log: probably a default |

- Why it matters per zone (STATIC, data from `evidence/I-0066/queries/gparam-all`, 168 files): the Dominant Light group is **not**
  a constant. Dominant colour scale: m37 0.45, m31_00_0000/0010 0.8 (offset 0.2), m31_00_0020/0030 1.0 with R 0.9 and offset 0.85,
  m30 0.5 (SH scale 0.5), m40 1.2 with R/G/B 0.6/0.8/1.0, m45 1.0, m50 0.5; 102 of 168 files have no such group (they inherit
  from below). A default row cannot equal all of these. At m40 the file's R/G/B scales (0.6/0.8/1.0) turn the baked dominant light
  blue; the native frame is warmer/whiter in exactly that direction (ash B/R 1.14 native against 1.39 original).
- The instance SH zeros explain the **black character at m37** (Native1635/1644) and are the likely cause of the dark/missing hair
  and skin gap noted at « 1142 witness: the hand/skin lighting gap ». They do not explain the white ash.
- **Discriminating test (no game needed for the first step):** add one log row with `rows.a`, `rows.b`, `rows.blend`
  (`game_bootstrap.cpp:8301`) and compare with the file values for the key (e.g. m40 id 0: 0.5, 1, 0.86, 1.2, 0.6, 0.8, 1.0; SH 1,1,1,1).
  Add a counter row « engine bank returned its default record » per (bank, key) to enumerate every residual consumer.
- **Mechanism fix:** one native bank service answering the engine lookups the native path uses (32A80, 134A20, 416A0, 12F400,
  46300, 15E920, the LightScattering one) from `Banks`, with the layering of L3. Do not patch rows one by one.

### L3. Layer resolution: one « latest » file per area, and an absent group falls back to `default.gparam` id 0 (STATIC, certain as code; not witnessed as the cause yet)

- `ApplyNativeGparamAreaBindings` (`game_bootstrap.cpp:12487`) keeps, per area, **the last kind-7 binding** (`:12494`; the comment
  says « not validated », with a log row `native_gparam_area_layers` that was empty in every run I read). `Banks::File`
  (`light_producers.cpp:1180`) then returns that single file, and `Banks::Resolve` (`:1196`) sends any (area, id) not found in
  *that file's* group 0 (LightSet) or 1 (Fog) to **(1, 0) = `default.gparam` id 0**, whose sun is white, intensity **2.5**, and
  whose Dominant/Env groups are absent.
- The original merges per group: a layer without the LightSet group leaves the group of the layer below. Many variants are
  partial: `m40_00_0510` is Fog only; `m37_00_0100..0410` and `m45_00_0100..0800`, `m33_00_05xx..07xx`, `m35_00_05xx`,
  `m39_00_0500`, `m30_01_0210` have no LightSet. When the ceremony binds such a file after the base one (Firelink with ceremony
  base 10: `m40_00_0010` then `m40_00_0510`), native takes the partial file and lights the zone with the default « plein soleil ».
  The m50 fix for YEBIS (`Banks::GroupEntries`, « the area's own file then mapbase ») was never applied to Sun, Fog, Env.
- The runs I read are not affected (Native1710/1712 m40: one kind-7 file `m40_00_0000` plus nine empty ones; Native1714 m31:
  `m31_00_0010`; Native1711 m30: `m30_00_0010`; sun0 equals the file's). **This is the progression-dependent class**: the later
  states Thomas wants tested (Shebang Beta, end of the game) are the ones whose ceremony binds several or partial files.
  Bindings also carry weight 0 at bind time (`weight_bits:0`) and the controller fades them afterwards; native ignores the weight.
- **Discriminating test:** for each save state, list `native_gparam_scene_binding_source` rows with kind 7 and compare the first
  `native_light_blocks.sun0` with the sun the TSV gives for the logged keys (the TSV are in `evidence/I-0068/queries/gparam` and
  `evidence/I-0066/queries/gparam-all`); a sun (1, 1, 1) x 2.5 or direction (0.497, 0.743, -0.448) is `default.gparam` id 0.
- **Mechanism fix:** per-area ordered layers (binding sequence, weight), resolution per group (first layer with a non-empty group
  and the key, then `mapbase`, then `(1, 0)` of `default`), shared by Sun, Fog, Env, Dominant, Snow and the YEBIS groups.

### L4. Progression changes the data, so test both states with the values in hand (STATIC)

The sky and light are different files, selected by the Ceremony row of the map (`ceremonyparam.tsv`; ten slot files
`mNN_BB_{i*100+base}`). m31 (id 0, light 0, sun colour/intensity, fog colour, exposure): `0000` sun (0.97, 0.95, 0.93) x 1.5,
fog (0.36, 0.41, 0.46), exposure 2.2; `0010` sun (1.0, 0.82, 0.69) x 1.5, fog (0.16, 0.19, 0.23), contrast 0.15 (the Shebang
dusk); `0020` sun (0.80, 0.70, 0.60) x 1.26, fog (0.71, 0.67, 0.63); `0030` sun (0.80, 0.64, 0.50) x 1.07, fog (0.71, 0.67, 0.63).
m40: `0000` id 0 sun x 0.8 with SsaoWeight 0.75; `0010` id 0 sun x 0.1 and SsaoWeight 0. A veil whose colour follows the fog
colour (beige at m31 native) points at the fog record of the wrong state or at its field mapping (`Banks::Fog` copies fields by
position: `fields[]` at `light_producers.cpp:1376`, qualified only against captured zones).

### L5. The m31 veil is additive: three candidates, one flag each (INFERRED)

Native m31 lifts the blacks and desaturates near objects, which a distance fog does not do. In order: (a) the glare composite on a
scene HDR that is several times too bright (the threshold passes far more of the image): `no-yebis-glare.flag`; (b) the specular
IBL accumulation t7, added in `GXLightComposite` (= t6 + t7 + t8) without the AO that scales it in the original (L1); (c) the fog
entry (L4). `no-yebis.flag` already separates the library passes from the scene.

### L6. Checked and ruled out (STATIC / OBSERVED; do not redo)

- Exposure, contrast, gamma, ColorS, vignette, grading: the words equal the original's bit for bit (T12, T17, K-0443..K-0450);
  `native_display_curve` is the identity 2.2 / 2.2 (brightness option at its default).
- Sun and fog records at m37, m31, m40: Native1544/1545/1547 equal the engine's records; Native1710 `sun0` (0.7473, 0.5446, -0.3807,
  0.2) is `m40_00_0000` id 100 (the first fallback hypothesis, `default.gparam`, is **false** for the runs read).
- Auto exposure: never on in reachable play (K-0448); `SceneLuminanceScale` is 1 in `m40_00_0000`.
- Env colour scales: 1.0 in almost every file (m40 id 100 is 0.5; m30 ids 102/211 0.9/0.8, 401 is 2.0): negligible beside L1.
- The snow family: 0 `native_selected_constant_unavailable` at m40, so the ash is not the `_SnowLt` shader (the draws would be
  refused); the white look is lighting, not a material.
- Grey/white ash as a *scene family* (the 2026-10-14 observation « snow family, not YEBIS »): the mounds exist in both images with
  the same geometry; the difference is the light on them.

## 2. Order of work I would take (for the pilot)

1. Offline, no game: the two log rows of L2 and the sun0 comparison of L3 on the existing runs and on the next two (TestAnim copy at the
   Village, Shebang Beta); `no-yebis-glare` and a no-sun switch on one m40 run (L1, L5). One run answers L1/L5.
2. Reference side, once: one Reference run per zone that logs the original's `cbPerFrame +0xC0` rows, the `32A80` rows (+0x290) and
   the mean of t32 on the floor. It turns L1 and L2 into numbers instead of arguments.
3. Fix L3 then L2 as one bank-service change (a general mechanism, testable against the TSV), then take L1 (the two wave-2 passes)
   to Thomas as a roadmap decision.
4. Quote ratios only against the original's graded frame (§0 caveat).

## 3. Not found / limits

- I could not read the Dominant bank's built-in default values (`FUN_140134150`'s constructor and the bank object are not in
  `decompiled/`; `FUN_140134150` is only the file binder). The first log row of L2 gives them for free.
- The writer of the SH rows at `light set record +0x290` was not read (`FUN_140037340` builds the record; 359 lines in my scratch
  decompile, not read to the end); « SH rows from Hemi Up/Down » is INFERRED from the record layout, not shown.
- The per-term split of the x10 excess between sun, AO, spec and DOL is not knowable statically.
- Source images: `evidence/I-0066/artifacts/i66-native-1710-m40-defaults/world-2400.png`, `i66-reference-1705-m40-yebis/reference-tonemap-output.raw`
  (RGBA8 1280x720), `i66-native-1635-m37-composition`, `i66-native-1644-m37-tonemap`, `i66-reference-1643-m37-tonemapcapture`,
  `i66-native-1647-m31-tonemap/world-3600.png`, `i66-reference-1646-m31-tonemapcapture`.
