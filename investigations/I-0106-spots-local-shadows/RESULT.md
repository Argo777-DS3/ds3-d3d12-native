# I-0106 - Spots et ombres locales: result

## Plan

| Step | What | State |
|---|---|---|
| 1 | The spot lights' per-frame volume state (why `2136B0` on native spot objects gave wrong sort keys, Native1509) | done (static): nothing missing |
| 2 | Spot items (`F1010` → `2136B0`) in the native collect, the hull binning, `DSpotLight1..4` per tile | done, on by default, seen |
| 3 | Local-light shadows on by default: selection (SW-7) with spots, local atlas, face passes, shadowed programmes | done, on by default, seen |
| 4 | The forward rows' spot buffer (`g_SpotLightBuffer`), if cheap | **not done**: the forward rows still bind an empty spot buffer (a forward-drawn surface is not lit by a spot) |

## Step 1: the spot lights' state (static, 2026-10-29)

Read with headless Ghidra on `ghidra-mirror-menus` (scripts in [ghidra/](ghidra/), outputs in `evidence/I-0106/spot-volume/`),
verified by the pilot on the decompiles where noted:

1. **No per-frame volume state is missing natively** (VERIFIED). The spot's cone (+0x1D0..+0x1DC) is set by `FUN_1400F10C0`,
   whose tail `FUN_1400F11B0` derives the cone projection +0x190..+0x1CC (`FUN_14004BA40`), the aspect +0x1E0, the extent
   +0x1E4, +0x1E8 and the eight local corners of the shape at +0x220 (pointer +0x220 to the inline buffer +0x270, count +0x228,
   world matrix copy +0x230); its callers are the constructor `FUN_1400F09B0`, the MSB light loader `FUN_140018490` (twice) and
   an effect light's slot `FUN_1402B49D0`: on load or change, never per frame. The map loader runs natively, so the state exists.
2. The only per-frame light work is slot 2 (`FUN_1400F0D40`) from the engine step (`FUN_140D5B140` → `FUN_140023410` →
   `FUN_1400499C0` over `[[GXRS]+0x41D0]`), not from the old renderer: the flicker output +0x218 (`FUN_1402124A0`, 1.0 without
   flicker amplitudes) and the projected texture +0x328. The point lights' flicker +0x19C goes through the same slot.
3. `FUN_140214270` copies the eight corners into the item (+0xF0 the points, +0xF8 the count); 2136B0 builds the item's own
   local-to-view matrix +0x100 and the view-depth range +0x20/+0x24 from the light's +0x150 and +0x190 and the pass record's view.
4. So the « wrong sort keys » of Native1509 (195 and 51970 against 6 to 11) are INFERRED to be a comparison of different spot
   lights: the native then had no visibility rule and built items for lights behind and beside the camera, the original's rows
   were the slot-3 observer's (lights already kept). The genuine 2136B0 on the native pass record is the spot item (step 2).

## Step 2: the spot lights drawn (code, `1984fd9d`)

In `CollectNativePointLights`: F1010's fade (1F1C60 on the centre light+0x180 over the extent +0x1E4), the genuine 2136B0 on the
native pass record, the types 0x67/0x6E kept (the projected-mask types 0x69/0x70 have no tile path in 1A1DA0), 2144C0's eight rows
and the hull. In the lighting schedule: one binning of the point spheres and the spot hulls (140244520), per tile the point batches
then the spot groups of at most 4 (`GXLightAcc_DSpotLight1..4`, the point batches' quad, targets, blend and bindings: the spot
programmes declare the same t0, t1, t3, t5, t33, b0, b1, b4). Opt-out `no-spot-lights.flag`. Builds, unit tests pass; not seen live.

## Step 3: the local-light shadows (code; opt-in at first, on by default since step 4)

1. **Selection** (`CollectNativeLocalShadows`, now before the walks): SW-7's on the engine's container, with the spot lights
   (visibility `1400F0FD0` on the shape +0x220, the genuine 2136B0 item, slot 3 `140214350`). Per hit, the container items it
   appended (mode 1: one item, +0x378 = 6 for a point light) give the faces: camera n at item + 0x90 n (pose +0x00, lens +0x50:
   field of view, near, far, aspect +0x64), the world-to-clip matrix = inverse(pose) × `14004BA40`(fov, aspect or 1, near, far)
   (as `14004B070` for a perspective camera; its words +0x68/+0x70 read 0 in i106-look1), the tile = the light's rectangle (point: the six +0xBC
   rectangles widened back by 3 texels; spot: +0x268). Each item's world box (its faces' clip corners) feeds the walk. Mode 1
   (one item a light, the original's +0x288) is the one seen; mode 0 (six single-face items) is written, never run (INFERRED).
2. **Casters** (`CollectNativeLocalCasters`): the world walk on category 0x04 (C2EC0's passes 9..0x38) inside the items' boxes;
   a model takes the items its bounds cross (`local_items`), a model no earlier walk admitted is shadow-only. The cascade row
   (57A20 prep 2, class 0x107, Dpt/DptA) is the same for passes 1..0x38 (SUN-SHADOWS-STATIC-PREP §1), so the casters reuse the
   cascade rows' draws (`shadow_draws`, `shadow_locals` beside `shadow_cascades`).
3. **Atlas**: the local atlas (4096², D16, SW-7c) cleared to 1.0, then each face's casters into its tile with its view block
   (+64 the world-to-face clip matrix, the cascades' convention); declared to the frame graph (`local_shadow_atlas`, key bits
   36-37), read between its casters and the next frame's.
4. **Receivers**: a shadowed light's rows from its frame item after slot 3 (212DB0: point +0x20..+0x50, {+0x90, +0x94, 0, 0},
   +0x50, +0x60, six rectangles; 2144C0: spot eight rows, {+0xE0, 0, 0, 0}, its rectangle), groups of at most 4 per tile in
   1A1DA0's order (0x65, 0x66, 0x6C, 0x6D, spots, shadowed spots), programmes `DPointLightShadowed1..4`,
   `LegacyDPointLightShadowed1..4`, `DSpotLightShadowed1..4`; `cbLightAccShadowInfo` {W, H, 1/W, 1/H} per light at b2, t7..t10 the
   local atlas (R16_UNORM), s12 SS_Shadow = the sampler bank's row 15 (comparison linear, border, LESS).

## Step 4: seen natively (i106-look1, 2026-10-29 08:00-08:05, ASUS, debug layer on; Thomas allowed short runs at 07:49)

`Run-I106Look.ps1` (Gamma's save copies, Continue, eight 45-degree turns; the originals are I-0100's `i100-t3a-al-o1` and
`i100-t2l-m37-o1`, same saves and sweep), module `build-shadows` with `shadow-local;test-no-ai`. Both runs exit 0, no fault,
0 debug-layer errors, restoration by hash OK. Pictures: `evidence/I-0106/look1/` (`pairs-al.png`, `pairs-m37.png`,
`stairs3-three.png` and `chapel3-three.png`: original, native before (I-0100's `-n1`), native now).

1. **Anor Londo, the great stairs** (OBSERVED): the balustrade posts' shadows run up the right staircase in the original and
   now natively, absent before. The journal: 45 spot items a collect (0 refused), the slot shadows a spot (variant 0x6F, tile
   (0, 0, 256, 256)) and a point light (0x66, six 512 tiles reduced by 3 texels, words (1.001, -0.006)); the container item's
   camera 0 sits at the light, lens (fov 0.4887, near 0.78, far 78, W 0, H 0, aspect 1: as SHADOWS-WAVE1-READY §1.5 predicted,
   the screen extent 0 natively, the aspect word set); 23 category-0x04 entities, 52 caster draws; 70 shadowed groups.
2. **Central Irithyll, the chapel** (OBSERVED): warm brown as in the original, cold white before: the spot lights' light.
3. The rest of both sweeps matches the original by eye (no black patch, no square, no flicker seen in 16 pictures a zone).

Step review (fresh Opus, read-only) fixes, `build-shadows2`: a light draws shadowed only when one of its faces reached the atlas
(else unshadowed, counted `refused`), the other texture branch of 2144C0 / 212DB0 (+0x264 / +0xB8 set) refused, the container
items bounded at 48 as the renderer, the shadowed views' descriptors null until bound, the sun stage only with a cascade caster,
the caster lists rebuilt only when a draw is dropped. Then on by default (`no-local-shadows.flag` opts out), `build-shadows3`.

**The final tree seen** (`i106-look2-al`, `-m37`, 2026-10-29 08:41-08:46, `build-final` = the branch with `main` merged, local
shadows by default, only `test-no-ai`): both exit 0, no fault, 0 debug-layer errors, restoration OK; the stairs' post shadows
as the original (`evidence/I-0106/look1/stairs3-final.png`) and the warm chapel (`chapel3-final.png`); journals: 1 and 2
shadowed lights applied, 0 refused, 60 shadowed groups at Anor Londo. The final review's fixes below (`build-final2`) were not
run in game before the merge: the start check after the merge and Thomas's play test of 2026-10-31 are their first runs.

**FLIP** (`flip.exe`, WORKFLOW « Tools », 2026-10-29 21:10): the game window of each sweep picture (1280x720), native against
the original's same sweep, mean FLIP error per picture (`evidence/I-0106/flip/means.txt`): Anor Londo, before (I-0100's `-n1`)
0.10-0.16, `look1` 0.05-0.10, final tree 0.07-0.12; Central Irithyll, before 0.10-0.20, `look1` 0.07-0.13, final 0.07-0.13. The
residual includes the camera's idle drift and the character between runs (not separated); the final tree's Anor Londo values
above `look1`'s are not explained (its module also holds I-0105 tranche 5).

## Final review (fresh Opus, xhigh, on the final tree) and its outcome

- **B1, blocking, fixed**: `14004BA40` stores its matrix with MOVDQA; the face projection's local is now `alignas(16)` (the
  runs did not fault only because the stack happened to be aligned).
- **I1, fixed**: STATE's rows, this plan table, step 3's heading, the evidence of the final tree (above), contract item 4 marked
  not done; FINISH-LINE line 6 and ROADMAP rewritten at the merge.
- Fixed: an offscreen view clears `local_atlas_frame_` (M1); a shadowed light whose class's shadowed programmes are not all
  loaded draws unshadowed (M5); `no-local-shadows` and `no-spot-lights` accepted and cleaned by `Run-StartupTrace.ps1` (M6);
  `no-spot-lights.flag` also keeps the spots out of the local shadows (M7); stale comments (M9).
- **Open, recorded**: M2, the local walk merges a known model's items without the pass-5 selection (`casts`) the cascades
  require: category 0x04 alone decides here (the stairs' pieces cast as in the original, OBSERVED; a piece seen only through the
  0x45 view could cast where the original does not, not seen). M3/M4, repeated work (the walk setup twice, 2136B0 twice per visible
  spot), casters per container item rather than per face (a point light's casters drawn into its six faces), the whole local
  atlas cleared each frame and recreated after a frame without local shadows: the cost, not measured (D-0227). M8, not seen yet:
  the spots in the water reflection view (its collect takes them) and the effect-created spot lights (`2B49D0`, e.g. spells).
- Also open: the casters' bias (the cascade rows' rasterizer state; closed by I-0113 tranche 8, gap 8: the original's mode-3 state), the map object's size words (+0x50/+0x54 of [+0xB0] /
  [+0x1C0]: the container's atlas object, never created natively; the native atlas edge is the configured 4096), the visitor
  masks of 222FF0 (W5 review).
- Step review of the fixes (fresh Opus): nothing blocking; low, recorded: the programme-set check redone per tile (cheap), the
  atlas still filled when the shadowed programmes are missing (the lights then draw unshadowed), a shadowed group skipped if
  the atlas resource ever fails (collect requires faces, so only on a renderer-side atlas failure).

## Conclusion

The map's spot lights and the local-light shadows draw natively by default (opt-outs `no-spot-lights.flag`,
`no-local-shadows.flag`): seen at the Anor Londo bonfire (the stairs' post shadows, Thomas's R7) and at Central Irithyll (the
warm chapel) as in the original, with 0 debug-layer errors. Not done: the forward rows' spot buffer. Thomas's play test (Friday
2026-10-31) is the next look; the open points above are recorded, none blocks the merge.

## After the merge

Merged `d2235f1d` (2026-10-29 09:13). The launcher's module rebuilt from `main` (unit tests pass). Start check `start-d2235f1d`
(09:27, ASUS): launched, 344 world frames, closed by itself, restoration OK, `passed: false` on a false positive: the spot
counter's journal key `spot_refused` (value 0) matched the check's `_refused"` pattern. The key renamed `spots_dropped`
(`2d7a89cc`), merged again, the launcher rebuilt and the check run again (below).
The launcher rebuilt from `main` `8f4e2c13` (2026-10-29 18:18, after the morning's interruption: no game, the protected files
equal to the last run's baseline) and the start check `start-8f4e2c13` (18:24, ASUS): 367 world frames, closed by itself, no
refusal, no failure, restoration OK, `passed: true`. I-0106 is complete; Thomas's play test of Friday 2026-10-31 is its next look.
