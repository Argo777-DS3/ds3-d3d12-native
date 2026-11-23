# YEBIS motion blur natively: velocity object, camera velocity, blur passes, inputs, constants, formats, frame order (STATIC; checklist Y1-Y16, live rows ML1-ML12)

Written 2026-10-11 for steps 7 and 8 of [VELOCITY-NATIVE](../I-0066-independent-native-startup/VELOCITY-NATIVE.md) (dilation, YEBIS blur). The YEBIS thread owns
the I-0068 notes: this file is separate and only cites them: [VELOCITY-MOTIONBLUR-NATIVE](../I-0068-static-yebis-ffx-lights-spec/VELOCITY-MOTIONBLUR-NATIVE.md),
[VELOCITY-IMPLEMENTATION-CHECKLIST](../I-0068-static-yebis-ffx-lights-spec/VELOCITY-IMPLEMENTATION-CHECKLIST.md),
[YEBIS-NATIVE](../I-0068-static-yebis-ffx-lights-spec/YEBIS-NATIVE.md), [YEBIS-HAL-READY](../I-0068-static-yebis-ffx-lights-spec/YEBIS-HAL-READY.md),
[YEBIS-TECHNIQUES](../I-0068-static-yebis-ffx-lights-spec/YEBIS-TECHNIQUES.md). Nothing was run. Labels: STATIC (decompile or DXBC read), INFERRED, OPEN.
Evidence: decompiled exports (list at the end) and the disassembled shaders in `evidence/I-0067/queries/yebis-mb-dxbc/` (DXBC of the embedded `ps_MotionBlur*`
techniques and of the loose `gxposteffect` programmes `MotionBlurDilateVelocity`, `MotionBlurWriteCMBVelocity`, `MotionBlur`).

Address convention: `1416Cxxxx` etc. are full virtual addresses of the YEBIS library inside `DarkSoulsIII.exe` (the I-0068 notes write `CAA20` for `1416CAA20`).

## 0. Answer first

| Item | Result |
|---|---|
| What runs | (7) the GX dilation of the object velocity, 2 passes; (8a) **YEBIS builds the camera velocity itself from depth** and overlays the object velocity (1 draw); (8b) **YEBIS blurs the HDR scene along that velocity** (R passes, R = 1 with `default.gparam`) |
| Who computes | everything is library CPU code. The native supplies **inputs** (section 1) and a **HAL** (YEBIS-HAL-READY); it re-implements none of the arithmetic below. Sections 4-5 give the arithmetic only so that the inputs and the witnesses can be checked |
| Mode | always **mode 2, velocity based**: the mode word of the MotionBlur parameter group is not serialized and the group constructor sets it to 2 (`1401D9BD0`, `+0xC`). Modes 0 and 1 (camera-only blur) are never used by DS3, except mode 1 on a frame whose reset flag bit 1 is set (never set by a static writer) |
| Velocity source | **`PFXVS_AUTOGEN_APPOVERWRITE` (1)** when the engine velocity object exists (the half-size RGBA8 target `pipeline+0xDF0` after dilation), and **no YEBIS motion blur at all without it**: `SetMotionBlurEnable` is called with `velocity object != 0 and P+0x1DC != 0`, so without the object `S+0x9C5` stays 0 and the stage never runs (the auto source of A7b is set but unused). The library renders `tech_MotionBlurMakeCameraVelocityOverwrite_<ch>` into its **internal velocity texture**: camera velocity from depth and the two cameras, replaced by the object velocity where the object alpha is 1 |
| Position | inside `EndPostEffectScene_InternalProcess` (`1416E5BC0`), **before the feedback stage and, in the normal frame, before depth of field** (INFERRED path B, section 3). With MotionBlur on, the frame has **+2 YEBIS draws** (1 velocity + 1 blur) at default |
| Encoding | one convention end to end: `RG = clamp_len1(delta_ndc * (0.5 k a, -0.5 k)) * 0.5 + 0.5` on UNORM targets (identity on float), `k = 1 / (MaxBlurLength * f * MotionBlurScale)`, `a` = aspect. It is the `FC_DifColMul` of the engine `Velo` rows, so object and camera velocity agree |
| Default numbers (`default.gparam`, 1280x720, vertical fov 60 degrees) | blur taps `n = 5` (shader `..._S5`), `R = 1` pass, step 18 px at full velocity, blur displacement = `BlurTimeRatio` (0.06) x the per-frame displacement, blur only where that displacement reaches 2 px (about 33 px per frame at 720p) |
| Native work | Y1-Y5 inputs and velocity object, Y6-Y9 library-side expectations and unit tests, Y10-Y12 HAL surfaces and quality gating, Y13-Y16 witnesses |

## 1. Inputs the native driver must give YEBIS (STATIC: `1400E69A0` and the CPostEffect setters)

In the original, `FUN_1400E69A0(pipeline, camera, P, ...)` sets the YEBIS library from the per-frame parameter block `P` (a copy whose first 0x370 bytes mirror the globals block at `144961F70`; `E4B50` writes the globals, `P+0x1DC` mirrors `14496214C`; the copier `1400E3CF0` was not read)
(`P+0x1DC` = `14496214C`), the current camera and the previous camera, once per frame, before `BeginPostEffectScene`. The native renderer does not run it,
so **the native YEBIS driver repeats the motion-blur part of it**. Library object `S` = `[1448E5E38]` (`CPostEffect`).

### 1.1 The MotionBlur parameter group and its words (STATIC)

The group object is `GXBS::GXMotionBlurFilterParam` (vtable `143D5EC20`, constructor `1401D9BD0`, serializer `1401DA920`, blend `1401D9910`, YEBIS side
`1400E4B50`). Blended words (dword index `w` = (group offset - 8) / 4), file names from the serializer, **defaults from the constructor**:

| w | Name | Kind | Constructor default | `default.gparam` | `mapbase` | `remobase` | Reaches |
|---|---|---|---|---|---|---|---|
| 0 | Enable | int | 0 | 1 | 1 | 1 | `14496214C` = `P+0x1DC` (forced 0 when the option byte or the factor is off, below) |
| 1 | (not serialized) mode | int | **2** | 2 | 2 | 2 | `144962150` = `P+0x1E0` |
| 2 | BlurTimeRatio | float | 0.05 | 0.06 | 0.2 | 0.15 | `P+0x1E4` |
| 3 | MaxBlurLength | float | 0.1 | 0.1 | 0.1 | 0.1 | `P+0x1E8`, clamped `>= 0.001`; also the view scale of the `Velo` rows (VELOCITY-NATIVE "view scale, closed") |
| 4 | NumBaseSamples | int | 4 | 4 | 4 | 5 | `P+0x1EC` |
| 5 | MaxRecurrences | int | 1 | 1 | 2 | 2 | `P+0x1F0` after the option cap and scale |
| 6 | SampleInterleaved | float | 1.0 | 1 | 1 | 1 | `P+0x1F4` |
| 7 | CameraRotationPerspectiveThreshold | float | 0 | 0 | 0 | 0 | `P+0x1F8` |
| 8 | CameraTranslationThreshold | float | 0 | 0 | 0 | 0 | `P+0x1FC` |
| 9 | (not serialized) | int | 0 | 0 | 0 | 0 | `P+0x200` |
| 10 | FalloffDistanceMin | float | 2.0 | 2 | 0 | 2 | `P+0x2AC` |
| 11 | FalloffDistanceMax | float | 0 | 0 | 3 | 3 | `P+0x2B0` |
| 12 | MotionBlurScale | float | 1.0 | 1 | 1 | 1 | `1449622C0` = `P+0x350` = factor x w12 (not blended in the interpolation branch of `1401D9910`) |
| 13 | Bidirectional PermissionEnable | int | 1 | 1 | 1 | 1 | `DAT_144962174` = `P+0x204` (byte) |
| 14.. | GS fields (Enable byte, Motion/Object/Camera Blur Intensity, ReccurenceCount, three inner-time bytes, Base Fps) | | | | | | GX side only (`filter+0x11C`, velocity pass), not YEBIS |

`1400E4B50` (called from `1400FB890` line 418 on the blend of the two area keys with weight `param_5+0x40C0`) adds the engine options:

- `option = [[GXRS+0x5360]+0x20]` (byte), `cap = [[GXRS+0x5360]+0x24]` (dword), `factor = [[GXRS+0x5368]+0xE0]` (float), `count scale = [[GXRS+0x5368]+0xF8]`.
- `Enable` (`P+0x1DC`) = `w0` only if `option != 0` and `factor > 0.01` (`143D7ED28`), else 0.
- `P+0x350` = `factor x w12`; `P+0x1F0` = `max(1, min(cap, int(w5 x count scale)))` (the decompile shows a pointer alias; the intent is this min/max).
- When `[144961B50]` or `[GXRS+0x5360]` is null (menus before the world): Enable = w0, factor 1.0, cap 8. On a failing `CAB90` the driver writes `144962150 = 1` (the global mode, seen by the next copy).
- Quality levels give `cap` = 0, 1, 1, 5, 8 for DISABLE, LOW, MEDIUM, HIGH, MAX (MENUS-MAP 77; native `NativeMotionBlurQualityRow`).

### 1.2 The calls, in order (`1400E69A0` lines 584-666 and the setters)

| # | Library call (address) | Arguments and sources | Effect on `S` | Expected at default (`P` above, option HIGH, factor 1) |
|---|---|---|---|---|
| A0 | `FUN_1416C9450` / `1416C9490` (lines 167-177) | current camera: near `cam+0x54`, far `cam+0x58`, vertical fov `cam+0x50`; previous camera the same from its own struct | `+0x564` near, `+0x568` far, `+0x570` = tan(fov/2), `+0x56C`; previous set `+0x57C/+0x580/+0x588/+0x584` | near/far/fov of the two cameras (they differ only after a zoom) |
| A1 | `1416C95B0` / `1416C95F0`, then `1416C9590` / `1416C95D0` (lines 160-176, 1060-1078) | current and previous **camera matrix** (the 4x4 at `cam+0` through `FUN_14004BCA0`; previous = `param_7`, replaced by the current when bit 0 of `pipeline+0x227C` is set, which is also cleared by the read) | `S+0x594` (+ inverse at `+0x5D4`), `S+0x614` (+ inverse `+0x654`): current and previous camera transforms (view or camera-world depends on `FUN_1417007B0` / `FUN_1416FE370`, not decompiled). Two setter pairs write the same fields; the second pair (`9590`/`95D0`, raw matrices) overwrites the first (`95B0`/`95F0`, `FUN_14004BCA0` result) and is the one that receives the bit-0 replacement | rest: the two matrices equal; the option `DAT_144961BEF` (a fixed remix of the matrix rows, INFERRED handedness/flip) is read as 0 by default (OPEN) |
| A2 | `1416CB200(slot 0, depth key, w, h, fmt, ...)` | the engine depth texture (R24G8 SRV in the m40 capture), its size and the library format enum `FUN_1419294A0(fmt)`; `param_3[0x54]` | wraps the native texture as a library surface (map `[S+0xC0]` keyed by the texture pointer, created through `1416DE920` when absent) and sets `S+0x430`, `+0x438` (depth class), `+0x440..+0x44C` | depth is standard Z, `0..1`, read as channel R |
| A3 | `1416CAA20(S, en)` SetMotionBlurEnable | `en = (velocity object != 0) and P+0x1DC != 0` | `S+0x9C5`; **needs `S+0x12B` (init flag `PFXINIT_MOTIONBLUR`)**, else the library logs "Initialize() with PFXINIT_MOTIONBLUR flag" and clears it | 1 |
| A4 | `1416CAAA0(S, b)` | `b = byte P+0x204` = Bidirectional | `S+0x9CC` | 1 (odd sample counts, section 2.4) |
| A5 | `1416CAA60(S, mode)` | `1` if bit 1 of the reset word, else `P+0x1E0` | `S+0x9C8` | **2** |
| A6 | `1416CAB00(S, a, b)` | `P+0x2AC`, `P+0x2B0` | `S+0x9F0`, `S+0x9F4` (falloff distances) | (2, 0): no falloff. `mapbase` (0, 3): none. `remobase` (2, 3): falloff on |
| A7a | `1416CAB90(S, 1, tex, w, h, fmt, 1, &{s, s}, 0)` SetMotionBlurVelocitySource application-overwrite | only if `P+0x1DC != 0 and P+0x1E0 == 2` and the velocity object exists: `tex` = native texture of the object (`[[obj+0x28]+0x10]`), `w, h = obj+0x50, obj+0x54`, `fmt = FUN_1419294A0(0x1C)` (R8G8B8A8_UNORM), `s = P+0x350 x P+0x1E8` (or 1.0E-4 when `<= 1.0E-4`) | `S+0xA0C = 1`, `+0xA18` surface, `+0xA10 = 1`, `+0xA20 = 0`, `+0x9F8 = {s, s}` | `s = 1 x 0.1 = 0.1` |
| A7b | `1416CAB40(S, 0, 0, -2, &{1, 1}, 0)` auto | same gate, no velocity object | `S+0xA0C = 0`, `+0xA18 = 0`, `+0xA10 = -2`, `+0x9F8 = {1, 1}` | set when the velocity object is absent, **but then A3 passed 0 and the stage does not run**: unused in practice |
| A8 | `1416CAAE0(S, P+0x1E4, P+0x1E8, P+0x1EC, P+0x1F0, P+0x1F4)` | BlurTimeRatio, MaxBlurLength, NumBaseSamples, MaxRecurrences, SampleInterleaved | `S+0x9D0 = max(0, .)`, `+0x9D4 = max(0, .)`, `+0x9D8 = max(1, .)`, `+0x9DC` (raw), `+0x9E0 = max(1.0, .)` | (0.06, 0.1, 4, 1, 1.0) |
| A9 | `1416CAB20(S, P+0x1F8, P+0x1FC, P+0x200 != 0)` | CRPT, CTT, flag | `S+0x9E4`, `+0x9E8` (both `max(0, .)`), `+0x9EC` | (0, 0, 0): the activation test below passes whenever the velocity source is not pure auto |

The velocity object itself is passed to `E69A0` as `param_6` and only when `pipeline+0x3EB9` is 1 (`FB890` line 490: `pipeline+0xDF0`). Natively it is the
object of Y5.

## 2. What the library derives (STATIC)

### 2.1 Initialization flags (the YEBIS "Setting Info" group)

`1400E2B20` builds the init flag word from the **Setting Info** group (`1400E4070` copies its words to `144961F78..FBC`): base `0x90000000`, plus

| Setting Info word | Global | Adds |
|---|---|---|
| Use DepthTexture == 0 / != 0 | `144961F78` | `0x20000` / `0x10000` |
| Enable LensDistortion | `144961F7C` | `0x40000` |
| ApertureairyUnlimitedRadius | `144961F84` | `0x4C0000` = bits 18, 19 (`S+0x340 >= 2`), 22 |
| Enable MotionBlur-Dof | `144961F88` | `0x40000` (**PFXINIT_MOTIONBLUR**, `S+0x12B`; also set by LensDistortion, Chroma, Anti-Alias, UnlimitedRadius, Create...Tex) |
| **Create MotionBlurInternalVelocityTex** | `144961F90` | `0x240000` = `0x40000` + `0x200000` (**INTERNALVELOCITYTEXTURE**, `S+0x12C`) |
| Enable FeedBackEffect | `144961F98` | `0x800000` |
| Enable Anti-Alias | `144961FA0` | `0x8040000` |
| Enable Choromaticaberration | `144961FA8` | `0x40000` |
| AnamophicLensAspectratio / AnamorphicLensAperture | `144961FB0/FB4` | `0x20000000` / `0x40000000` |

With the defaults of `default.gparam` (all 1 except Use DepthTexture 0) the word is **`0xF8EE0000`**, so `S+0x12B = S+0x12C = 1`. Any change of a Setting Info word between
areas sets the re-initialization flag (`pipeline+0x2278`, `+0x20F8`): the library re-runs `Initialize` and **recreates its surfaces** (the HAL must survive a recreate).
`Initialize` (`1416EBF80`) creates the **internal velocity texture** `S+0x140` when `S+0x12C` is set: descriptor = width `min(int(base_width x s + 0.5), cap)`,
height from the scene aspect (override `S+0x3EC`), 1 mip, **format = `S+0x2F0` if nonzero, else `S+0x20C`, else the default of `FUN_1416DED60`, then the sRGB variants mapped to plain by `FUN_1416FFE40`** (0x1D -> 0x1C R8G8B8A8_UNORM; it maps sRGB enums only, not typeless).
INFERRED: the internal velocity texture is therefore **R8G8B8A8_UNORM**; the scale `s` (`S+0x3E8`) was not traced (ML3 reads the creation call).

### 2.2 Gates every frame (`1416E9050`, `1416E42B0`)

| State | Value at default | Rule |
|---|---|---|
| `S+0xC5C` | 2 | `= S+0x9C8` when `S+0x9C5 != 0` and the flag `S+0x6DE` is 0 (INFERRED: a stage-disable flag, 0 in the normal frame), else -1 |
| `S+0xC59` | 1 | set only if `S+0x9D0 > 0` and `S+0x9D4 > 0` (BlurTimeRatio and MaxBlurLength strictly positive), then (planner, 2.4) |
| activation | true | mode 2: `S+0x9EC == 0 and S+0xA0C != 0`, **or** the camera-blur length from `FUN_14172CF70` (output of `14172FBD0`) / H is strictly above `S+0x9E4`, **or** the flag returned by `FUN_1416E4FE0`; in mode 2 the gate also exits early when that flag and `S+0xA0C` are both 0. At default the first term holds |
| `S+0xC60` | R (1) | recurrence count from the planner |
| `S+0xC64/C65` | 1/1 | mode > 1 sets both: the stage runs at the early position (section 3) |
| `S+0xC7C/C7D` | 1/0 | `C7C = C65 or (lens-distortion stage with mode 1-2)`; `C7D` (velocity done this frame) is 0 for source 1 and 0, set to 1 once generated; for source 2 (application only) it starts at 1 |
| `S+0xA24` | `{0.1, 0.1}` | copy of `+0x9F8`; replaced by `{c70, c70}` for source 0 when `c70 > 0` |
| `S+0xA2C` | -1 | written by the generator (`param_16`), **not read by the blur**; the encode flag the blur uses is `S+0xA10` (1 for source 1; -2 means detect from the velocity texture format, source 0) |

### 2.3 Maximum recurrences (`1416EADA0`)

`r = S+0x9DC`; `extra = 1` if mode 2, source without application texture (`S+0xA18 == 0`) and no internal texture (`S+0x140 == 0`), else 0; `a = S+0x340 - extra`.
If `a > 1` the result is `r`. If `a == 1` the allowance is 2 and `r` is forced into 1..2 (out of range -> 2). If `a <= 0` the result is 0.
`S+0x340` is the library capability set by `Initialize` from the init flags (bit 0x12 raises it to at least 1, bit 0x13 to at least 2): at `0xF8EE0000` it is at least 2,
so `R_max = S+0x9DC` = 1 at `default.gparam` and 2 at `mapbase`.

### 2.4 The planner (`14172FBD0`, `1416D7080`; the same code runs in the gate and in the stage)

- `L = H x MaxBlurLength` pixels (H = scene height). Default: `720 x 0.1 = 72`; `1080`: 108; `1440`: 144.
- `N0 = S+0x21694`, set by `1416E42B0` from `S+0x9D8` (NumBaseSamples; the setter `141717F80` is a one-line store; INFERRED); if `N0 < 2` then 5. Parity `S+0x21698 = 2 - (S+0x9CC != 0)`: **1 (odd)** when Bidirectional, 2 (even) otherwise. Odd: `N = (N0/2)*2 + 1`; even: `N = ((N0+1)/2)*2`. Default `N = 5`; Bidirectional off `N = 4`.
- `N` is lowered until a technique exists: the exe carries `DirDepth[Innerscreen][Clamp]_SM3_T2_S2..S8` only (84 techniques, no S1, no suffix columns), so `N` in 2..8.
- `Lp = L / SampleInterleaved + 1`; if `Lp <= 1.1` (`1427CDAB0`) no blur (R = 0). Else `R0 = ceil(ln Lp / ln N)` (min 1.0 in the ratio), `n0 = ceil(Lp x N / N^R0)`, odd/even adjusted and capped at `N`.
- If `0 < R_max < R0` then `n = N`, `R = R_max`. Output step (pixels between taps at full velocity) `step = L / (N^(R-1) x n - 1)`.
- Worked values (default, 720p): `Lp = 73`, `R0 = ceil(2.67) = 3`, `n0 = ceil(73 x 5 / 125) = 3`; `R_max = 1 < 3` -> **`n = 5`, `R = 1`, `step = 72 / (1 x 5 - 1) = 18 px`** (`step / L = 0.25`).
  `mapbase` (R_max 2): **`n = 5`, `R = 2`, `step = 72 / (5 x 5 - 1) = 3 px`**.
  With `R_max` large and no cap: `R = 3`, `n = 3`, `step = 72 / (25 x 3 - 1) = 0.97 px`.

## 3. Position in the frame and the draw list (STATIC for the code order; INFERRED for the branch)

`1416E5BC0` order (the lines of the export): scene preparation (`FUN_14172A660` copies, only with an external source), `c7b`, `c7e`, `c80`, `c81` stages (`1416E0040`, `1416E0640`,
`1416E2AF0`, `1416E0160` + `1416E3130`), then

1. `if +0x51C != 0` (`+0x51C = c79`): depth of field `1416E2910`, then `1416DF6E0`;
2. **`if +0xC64: 1416E21E0` (motion blur)**: mode 2 always takes this call site;
3. `if +0xC74`: feedback stage `1417328D0`;
4. `if +0x51C == 0`: depth of field `1416E2910`;
5. `if +0xC59 and not +0xC64: 1416E21E0` (modes 0 and 1 only, unused by DS3);
6. then the rest (glare `1417538E0`, tone map, ...).

`c79 = (S+0x70 != 0) and DoF and (S+0x68 != 0) and (an external depth)`; `S+0x70` is set by `FUN_1416CA5F0` from `E69A0` (`144962240 != 0`, forced 0 when `pipeline+0x20D0 == 0`).
INFERRED: in the normal frame `c79 = 0`, so **motion blur precedes depth of field** (path B). The m40 and m31 captures with MotionBlur off start with `ApplyEffects_DepthFactorAlpha` (draw 300) then DoF;
with it on, the library draws first (ML1 verifies the order, which decides what the scene alpha holds, 4.3):

| # | Draw | Technique | Target | Inputs |
|---|---|---|---|---|
| 1 | camera + object velocity | `tech_MotionBlurMakeCameraVelocityOverwrite_<ch>` (ch = R for a depth-format SRV, A when the depth lives in the scene alpha, RGBA8 for packed depth) | internal velocity texture `S+0x140` | t0 depth, t1 object velocity (half-size RGBA8) |
| 2 | blur, pass 0 | `tech_MotionBlurVelocityDirDepthInnerscreen_SM3_T2_S<n>` | scratch scene target (`S+0x4C8` slot) | t0 scene (`S+0x4C0` slot), t1 internal velocity |
| 2+i | blur, pass i (R > 1) | same, `n` = N on non-final passes, planner `n` on the last | ping-pong between the `+0x4C4`/`+0x4C0` slots, last pass into `+0x4C8` | previous pass |

The plain `MakeCameraVelocity` techniques (no t1) belong to source 0 and never run for DS3 (the stage needs the velocity object, 1.2 A3); with `remobase` falloff (both distances > 0 and different) the `...Falloff` families.
The shaders are the embedded `ps_4_0` DXBC (SM4 `ps_MotionBlur*`); YEBIS-HAL-READY section 4 already creates a PSO per technique. **All draws are `DrawIndexed(3)` full-screen triangles with no blend, no depth.**

## 4. The passes

### 4.1 Step 7: GX dilation of the object velocity (STATIC, `102380` / `13BD10` / `14013DD50`, DXBC `gxposteffect__MotionBlurDilateVelocity`)

- Runs in group 0x35 when YEBIS exists (`view+0x9D0 = 1`), after the `Velo` rows (group 0x34). Command `13BD10(texA = pipeline+0xE08, texB = pipeline+0xE68, count = pipeline+0x3EB0, len)`, recorded only if `count != 0 and both textures`.
- **count = 2** (constructor store `140084D38`: `pipeline+0x3EB0 = 2`): two full-screen passes ping-ponging `E08 -> E68 -> E08`, so the dilated image ends in `E08`, the texture of the object YEBIS receives (`pipeline+0xDF0`). Even count: no copy back.
- Uniforms (`cbFilterParam`): `g_vScreenSize` (cb0[7]) = (w, h, 1/w, 1/h) of the target; `g_vblurLength` (cb0[9]) = 4 copies of `MotionBlurScale x MaxBlurLength x BlurTimeRatio` (words 12, 3, 2 of the blended group) = **0.006** at default, 0.02 at `mapbase`, in **texels**.
- Shader (SM5): 8 neighbours at `uv + dir x blurLength x (1/w, 1/h)` (directions 0/90/180/270 and the four diagonals x 0.7071) plus the centre; each sample decoded `v = 0.4 tex.xy - 0.199216`; a neighbour replaces the running choice when its blue (nearer, `w/(w+10)` smaller) is below the choice **and** the dot product `dot(dir to the neighbour, v_neighbour x length x size)` is positive (direction convention: previous minus current); output `v x 2.5 + 0.5` (xy), z and w of the chosen sample.
  The decode/encode pair is exactly `s + 0.00196` (`0.4 x 2.5 = 1`): it moves the zero of the engine clear `127/255 = 0.498039` to `0.5`.
- Consequence (STATIC arithmetic): with `blurLength = 0.006` texel the neighbour taps fall on the centre texel; **dilation as shipped is the zero remap plus a negligible spatial effect**. ML6 reads `cb0[9]` to confirm.
- Native contract (D-0162): the native velocity target is R16G16B16A16_UNORM at render size cleared to (0.5, 0.5, 1, 0). For YEBIS the native must present a **half-size R8G8B8A8_UNORM** object (`render w/2 x h/2`) whose zero is 0.5 after the pass: either
  (i) clear the native target to `(127/255, 127/255, 1, 0)`, reduce 2x2, run the genuine dilation shader twice (exact, zero -> 0.5); or (ii) clear to 0.5, reduce, **skip** the dilation (zero already 0.5; the missing `+0.00196` bias disappears too). Reduction rule per 2x2 block: take the texel with the largest alpha, ties by the smallest blue (nearest), which is what rasterizing the `Velo` rows at half size with a half-size depth test selects (INFERRED). Option (ii) is allowed only if ML6 confirms the sub-texel length.

### 4.2 Step 8a: the YEBIS camera velocity draw (STATIC; `1416E0160` / `1416E21E0` -> `141730F30` -> `141724410`; DXBC `MakeCamVelOverwrite_*`)

Selection (`141724410`): table by (app texture present, falloff, channel): `Overwrite` / `OverwriteFalloff` when t1 exists, else `MakeCameraVelocity` / `...Falloff`; channel index from `FUN_1416F4FA0` = the class of the depth texture (0..3 = R, G, B, A; a depth-format SRV gives R). The library offsets of the technique table are the I-0067/I-0068 TSV offsets **+0x38** (e.g. `Overwrite_R` is `S+0x1B31C+...`; this explains the apparent clash of the two tables).

Shader `Overwrite_<ch>` (`ps_4_0`, `$Globals` of 228 float4; registers by byte offset / 16):

```
d      = depth(t0 at v0)                       // channel R/G/B/A, or dp3(rgb, (0.996094, 0.003891, 0.000015)) for RGBA8
zn     = sat(M1.x * (d * C0.x + C0.y) + M1.y)  // C0 = cb0[0] = DepthCastScaleOffset (1, 0); M1 = cb0[155] = depth decode (1, 0 for NDC z)
s      = -PF.z / (PF.w * zn - PF.y)            // PF = cb0[15] = (near, far, near*far, far-near); s = view depth
clip   = (v1.x*s, v1.y*s, zn*s, s)             // v1 = NDC xy of the pixel from the VS
prev   = T * clip                              // T = cb0[224..227], rows dp4: prev clip (x, y, z, w); w = max(w, 0.000977)
dcam   = prev.xy / w - v1.xy                   // camera delta ndc (previous minus current)
w_out  = prev.z / w                            // stored in the alpha output
app    = t1(v2).wxyz :  a = sat(tex.w),  vapp = tex.xy * M2.z + M2.w     // M2 = cb0[156] = (1/p16.x, 1/p16.y, 1/p17.x, -p17.y/p17.x)
dapp   = vapp * M2.xy
d      = lerp(dcam, dapp, a)                   // object pixels (alpha 1) take the engine velocity
v      = d * M0.xy                             // M0 = cb0[154] = (p16.x, p16.y, p17.x, p17.y)
if |v| > 1: v = v / |v|
out    = (v * M0.z + M0.w, 0, w_out)
```

Constants set by `141724410` / `141730F30` (STATIC):

| Register | Meaning | Value at default (1280x720, UNORM internal texture) |
|---|---|---|
| `cb0[15]` | `(near, far, near x far, far - near)` of the current projection (`+0x564`, `+0x568`) | e.g. near 0.1, far 4000 -> (0.1, 4000, 400, 3999.9) |
| `cb0[0]` | DepthCastScaleOffset | (1.0, 0.0) |
| `cb0[155]` | depth decode scale/offset (`FUN_1416F4AF0`, from the depth class conversion) | (1, 0) for standard NDC depth, INFERRED |
| `cb0[154]` | `(p16.x, p16.y, p17.x, p17.y)`: `p16 = (0.5 a / s, -0.5 / s)`, `s = P+0x350 x P+0x1E8` (the A7a pair), `a` = aspect = `(1/P00)/(1/P11)` of the projection; `p17 = (1, 0)` for a signed/float target, **(0.5, 0.5)** for UNORM | **(8.889, -5.0, 0.5, 0.5)** (`s = 0.1`, `a = 16/9`) |
| `cb0[156]` | `(1/p16.x, 1/p16.y, 1/p17.x, -p17.y/p17.x)` | (0.1125, -0.2, 2.0, -1.0) |
| `cb0[224..227]` | `T = P_prev x V_prev x V_cur^-1 x P_cur^-1` (library column convention, `dp4 out_i = row_i . clip`), built from `S+0x594/+0x614` and the two projections | identity at rest (delta 0) |

`p16` equals the `FC_DifColMul` of the engine `Velo` rows (8.889, -5, 0, 1), so the overwrite is lossless. The encode flag passed to the generator is `S+0xA10` (1 for the application-overwrite source, so `p17 = (0.5, 0.5)` whatever the target format; auto-detected from the format only for source 0); the generator also stores it in `S+0xA2C`, which the blur does not read.
Object velocity scaling (derived algebra, INFERRED): the object delta is scaled by `factor x MotionBlurScale` relative to the true motion (encode `1/MaxBlurLength`, decode `1/(MaxBlurLength x factor x MotionBlurScale)`); the camera delta is not.

### 4.3 Step 8b: the blur draw (STATIC; `1417301D0` -> `141724840`; DXBC `DirDepth[Inner]_SM3_S<n>`)

Technique (`141744520`): `tech_MotionBlurVelocityDirDepthInnerscreen[Clamp]_SM3_T2_S<n>`: DirDepth is hard-coded (flags 1, 1), **Innerscreen = 1** (`param_20 = 1`), Clamp = 1 only if the clamp length `param_14 > 0` (0 at default), column 0.
Per output pixel (n = 5, taps -2s, -s, 0, +s, +2s; the even shaders use 0..n-1):

```
v0     = t1(uv).xy                              // internal velocity
d1     = v0 * O0.xy + O0.zw                     // O0 = cb0[186]; displacement x BlurTimeRatio in height-normalized units
s      = v0 * O1.xy + O1.zw                     // O1 = cb0[187]; tap spacing in uv
for each tap k: uvk = uv + k*s; accept the tap if  dot(decode(t1(uvk)), d1) > O3.x * |d1|^2   (similar velocity)
                                                or tap.a >= centre.a + O3.y               (scene alpha, "Depth")
                                                and uvk inside the rectangle (cb0[190], Innerscreen)
sum    = centre + accepted taps;  count = 1 + accepted
out    = (|d1.x| + |d1.y| >= O2.z) ? sum * (W / count) : centre      // W = cb0[154+n] = sum of weights = 1
```

Constants (`141724840`, handles `S+0x108` = `afRGBA_Modulate`, `S+0x10C` = `afRGBA_Offset`), default values (UNORM velocity decode `a = 2, b = -1`):

| Register | Meaning | Default |
|---|---|---|
| `cb0[154 .. 154+n-1]` | weights, `1/n` each (RGBA) | 0.2 x 5 |
| `cb0[154+n]` | their sum | 1.0 |
| `cb0[186]` (`O0`) | `(a sx k, a sy k, b sx k, b sy k)`, `(sx, sy) = P+0x350 x P+0x1E8 = 0.1`, `k = BlurTimeRatio` | (0.012, 0.012, -0.006, -0.006) |
| `cb0[187]` (`O1`) | `O0` x `(p12.x, p12.y)` with `p12 = (step/L, step/L) x (1/aspect, 1)` per pass | (0.001688, 0.003, -0.000844, -0.0015) |
| `cb0[188]` (`O2`) | `(p12.x, p12.y, 2/H, clamp)` | (0.1406, 0.25, 0.002778, 0) |
| `cb0[189]` (`O3`) | `(S+0xA00 = 0.15, S+0xA04 / depth_scale = -0.01, 0, 0)` | (0.15, -0.01, 0, 0) |
| `cb0[190]` | rectangle remap `2/(x1-x0), 2/(y1-y0), -1 - ..., -1 - ...` of the draw rectangle in normalized coordinates | (2, 2, -1, -1) |

- Displacement (full encoded velocity |v| = 1): `BlurTimeRatio x MaxBlurLength` of the screen height = **4.3 px at 720p** with `default.gparam` (0.06 x 72), 14.4 px at `mapbase` (0.2 x 72). The taps spread over `(n-1) x s`: 4 intervals of ~1.08 px.
- The `2/H` threshold means only pixels whose `BlurTimeRatio x displacement >= 2 px` (height-normalized L1) are blurred: **about 33 px/frame at 720p with 0.06, 10 px with 0.2**; slower pixels pass through unchanged.
- Non-final passes (R > 1) use N taps; the spacing factor `p12` is multiplied by `N` after each pass; the last pass uses the planner `n`.
- Scene alpha: the `Depth` test reads the **scene alpha as a depth value**. What the scene alpha holds at this point (before `ApplyEffects_DepthFactorAlpha`) was not found statically: OPEN (ML1/ML2).

### 4.4 Targets and formats (STATIC, with INFERRED where marked)

| Surface | Created by | Format | Size |
|---|---|---|---|
| internal velocity `S+0x140` | `Initialize` when `S+0x12C` | R8G8B8A8_UNORM (INFERRED, 2.1) | scene aspect x `s` (ML3) |
| scene input / scratch `S+0x4C0`, `+0x4C4`, `+0x4C8` slots | the library's scene target pool | the YEBIS HDR scene format (RGBA16 in the m40 capture) | scene size |
| object velocity (YEBIS input) | the engine: `pipeline+0xDF0` | R8G8B8A8_UNORM | render size / 2 (640x360 at 720p: 921,600 bytes, Reference1186) |
| depth input | the engine | a depth-format SRV (R24_UNORM_X8 or R32_FLOAT) | scene size |

## 5. Draw count and cost (STATIC)

Default: **2 draws** (1 velocity + 1 blur) at scene resolution, plus the 2 GX dilation draws at half size. Per pixel: 1 depth tap + 2 velocity taps + 5-10 scene taps. `mapbase` (R = 2): 3 YEBIS draws.
With MotionBlur on, the YEBIS draw counts of YEBIS-HAL-READY section 8 become 43 (m40) and 50 (m31) before any other option changes; ML1 must confirm.

## 6. Implementation checklist (ordered; expected values and witnesses)

| # | Step | Expected | Witness |
|---|---|---|---|
| Y1 | **Init flags.** Build the init flag word from the Setting Info words of the active area key pair (blend by the key weight like the other groups) and call `Initialize`; re-initialize when it changes | `0xF8EE0000` with the defaults; `S+0x12B = 1`, `S+0x12C = 1`; no motion blur if `0x40000` is missing; no internal velocity texture if "Create MotionBlurInternalVelocityTex" is 0 | unit test of the word for the three files (`default`, `mapbase`, `remobase`); ML3 |
| Y2 | **Parameter block.** Read the MotionBlur group of the area keys with the genuine blend; apply the option byte, cap, factor, count scale like `1400E4B50` | table 1.1; `P+0x1DC = 0` when the option is off or `factor <= 0.01`; `P+0x1F0 = max(1, min(cap, int(w5 x scale)))` | unit test: HIGH/default (1,...), MAX/mapbase 2, MEDIUM/mapbase 1, LOW/mapbase 1, DISABLE 0 |
| Y3 | **Driver calls** A0-A9 in the order of 1.2, every frame, from the native camera pair (current pose, previous pose, fov, near, far, aspect) and the native depth | S fields of 2.2 after the first `BeginPostEffectScene`: `+0x9C5 = 1`, `+0x9C8 = 2`, `+0xA0C = 1`, `+0x9F8 = {0.1, 0.1}`, `+0x9D0..9E0 = (0.06, 0.1, 4, 1, 1.0)` | read-back of `S` in a debug build; ML4 |
| Y4 | **Previous camera and reset.** previous := the previous frame's camera; on a cut (new view, reset, > 10 m or 60 degrees, the policy of D-0162) set bit 0 of the reset word so that previous := current | at rest `T` is identity; a cut gives zero camera velocity on that frame | unit test; the lamp "cut" row of the velocity workstream |
| Y5 | **Velocity object.** Provide the half-size R8G8B8A8_UNORM object of 4.1 (option i or ii); wrap it as a HAL surface keyed by the native resource (A7a). Pass it only when the velocity target exists and the quality row has `+0x20` | zero velocity pixels read 0.5; object pixels alpha 1; the dilated texture ends in the first texture after an even number of passes | picture diff against Reference1188 half-size picture; ML5 |
| Y6 | **Planner unit test** (pure function) | 720p default `(N, R, n, step) = (5, 1, 5, 18)`; `mapbase` `(5, 2, 5, 3)`; 1080p default `(5, 1, 5, 27)`; Bidirectional off 720p `(4, 1, 4, 24)`; `Lp <= 1.1` -> R = 0 | host test from 2.4 |
| Y7 | **Gate chain test**: from the S fields of Y3 derive `+0xC59/+0xC5C/+0xC60/+0xC64/+0xC7C/+0xC7D` | (1, 2, 1, 1, 1, 0) at default; `BlurTimeRatio = 0` or `MaxBlurLength = 0` -> `+0xC59 = 0` | ML4 |
| Y8 | **Camera velocity draw.** Technique id by (app texture, falloff, depth class); targets; registers of 4.2 | at rest: output `(0.5, 0.5, 0, prevZ)` on camera-only pixels (alpha 0); with a yaw of `theta` rad per frame at vertical fov F, centre pixel: `v.x = 5 theta / tan(F/2)` (k = 10; clamp at 1), e.g. 1 degree at 60 degrees: **0.151**, clamp at **6.6 degrees** | ML7 (capture of the internal velocity texture) and the registers `cb0[154,156,15,224..227]` |
| Y9 | **Blur draw.** Technique `DirDepthInnerscreen_SM3_T2_S<n>`; ping-pong targets; registers of 4.3 | default: S5, one draw, `cb0[186..189]` as the table; a still camera and still objects: image unchanged bit for bit | ML8, ML9 |
| Y10 | **Surfaces.** HAL creates the internal velocity texture and the three scene-format targets on request of the library; `Recreate` after a flag change | formats of 4.4; no leak over 10 area changes | ML3; debug layer |
| Y11 | **Frame position.** Run the library MotionBlur stage in its own position: before DoF unless `c79`; no stage is skipped when MotionBlur is off | YEBIS draw count 41 (off) / 43 (on) at m40 | ML1 |
| Y12 | **Quality gating.** DISABLE: no draw, no velocity target (`P+0x1DC = 0`); LOW: the velocity target exists (option byte `+0x20` = 1) but the object rows are off (`+0x21` = 0), so the object texture holds only the clear (alpha 0): camera velocity only through source 1; MEDIUM/HIGH/MAX: object overlay; recurrence cap 1/1/5/8 | draw counts per level: 0 / 2 / 2 / 2 / 2 (one more blur pass for MAX when MaxRecurrences allows) | native run per level (configuration restored by hash) |
| Y13 | **Image witness.** Camera turning at the High Wall (Argo) against the original, same path | blur direction and length equal within 1 px at 720p; no blur below the 2 px rule; object blur on moving characters | ML10 |
| Y14 | **Alpha witness.** The scene alpha at the first MotionBlur draw equals the original's | see ML2 | ML2 |
| Y15 | **Debug layer clean** for 600 frames with MotionBlur on and the velocity workstream's checks | 0 errors, 0 faults | native run |
| Y16 | **Disable path**: MotionBlur DISABLE draws nothing of steps 7-8 and leaves the existing 41-draw YEBIS sequence unchanged | m40 41 draws | native run |

## 7. Live rows (the original, MotionBlur HIGH, configuration restored by hash; none was taken)

| Row | What to read | Expected |
|---|---|---|
| ML1 | GPU order capture of one world frame (m40 and High Wall): the YEBIS draws and their order | first two YEBIS draws are `MakeCameraVelocityOverwrite_<ch>` then `VelocityDirDepthInnerscreen_SM3_T2_S5` (path B), +2 draws against the MotionBlur-off capture; otherwise path A (after DoF) |
| ML2 | read back the scene alpha (YEBIS input) before the first YEBIS draw | constant, or the depth of the deferred pass; decides what `DirDepth` compares |
| ML3 | HAL surface creations at `Initialize` | the internal velocity texture: format R8G8B8A8_UNORM, size (and `S+0x3E8`); init flag word `0xF8EE0000` |
| ML4 | `S` fields at the `BeginPostEffectScene` of one frame | section 2.2 values |
| ML5 | the object velocity after dilation (`pipeline+0xDF0`) | zero pixels 0.5 exactly (127/255 -> 0.5); object pixels alpha 1 |
| ML6 | `cbFilterParam` of the dilation: `cb0[7]`, `cb0[9]`, pass count | (640, 360, 1/640, 1/360), 0.006, 2 |
| ML7 | the internal velocity texture of one frame (camera turn) and `cb0[154,156,15,224..227,155]` of the generator draw | section 4.2 values; at rest 0.5 |
| ML8 | `$Globals` of the blur draw, `cb0[154..159]`, `[186..190]` | section 4.3 table |
| ML9 | the HDR scene before/after the blur at a fast turn | blur only above the 2 px rule |
| ML10 | back buffer, camera turning, original vs native, same path | within 1 px |
| ML11 | `S+0x340`, `S+0x21694`, `S+0x21698` | 2 (or more), 4, 1 |
| ML12 | the reset word `pipeline+0x227C` on a bonfire warp and a load | which bits are set (bit 0 expected on a cut); bit 1 never |

## 8. Open items

- OPEN: the scene alpha at MotionBlur time (ML2) and the branch path A/B (ML1); the internal velocity texture size (`S+0x3E8`, ML3); the depth class `+0x438` and the decode pair `cb0[155]` for the engine depth (ML7).
- OPEN: `1416CAA20`/`CAAA0`/`CAB20` have no library name strings; the roles above are from the stored fields and their readers (names are descriptive).
- INFERRED: the object scaling by `factor x MotionBlurScale`, the R8G8B8A8 internal texture, the reduction rule of 4.1, the near-identity of the dilation (ML6).
- Not needed natively: the GX camera velocity pass (`13BFC0`, DXBC `MotionBlurWriteCMBVelocity`) and the GX `MotionBlur.ppo` fallback: with YEBIS present the original never runs them (VELOCITY-NATIVE).

## Exports

Decompiled exports in `decompiled/` (`DS3-1152-F-<RVA>.c` and `.asm.txt`, RVA = address - 140000000):
engine side `000E2B20`, `000E4070`, `000E4B50`, `000E69A0`, `000FB890`, `00102380`, `0013BD10`, `0013C7E0`, `0013DD50`, `001D9910`, `001D9BD0`, `001DA920`;
YEBIS library `016C9430`, `016C9450`, `016C9490`, `016C9590`, `016C95B0`, `016C95D0`, `016C95F0`, `016CAA20`, `016CAA60`, `016CAAA0`, `016CAAE0`, `016CAB00`, `016CAB20`, `016CAB40`, `016CAB90`,
`016CB200`, `016D0F00`, `016D15D0`, `016D7080`, `016E0160`, `016E21E0`, `016E42B0`, `016E5BC0`, `016E9050`, `016EADA0`, `016EAE70`, `016EBF80`, `016F4AF0`, `016F4EF0`, `016F4FA0`, `016FDEF0`,
`016FFE40`, `01700500`, `01724410`, `01724840`, `01744520`, `0172FBD0`, `01730F30`, `017301D0`, `019294A0`.
Shader disassembly (licensed bytes, not in Git): `evidence/I-0067/queries/yebis-mb-dxbc/` (`MakeCamVel*`, `DirDepth*`, `Velocity_SM3_*`, `GX_MotionBlurDilateVelocity`, `GX_MotionBlurWriteCMBVelocity`, `GX_MotionBlur`).
