# I-0068 part AM, point 1: who writes `view+0xCA4` (the `Velo` scale k = 1 / `view+0xCA4`)

Request (coordinator relay of the velocity pilot, 2026-10-11): in `5A120` case 5 (`Velo` preparation) k = 1 / `param_1+0xCA4`; the live value is 0.1 everywhere (title and game), hence `FC_DifColMul = (8.889, -5, 0, 1)` at 16/9. Who writes it?

Labels: **STATIC** (executable, gparam files, project notes), **INFERRED**, **LIVE-ONLY**. Nothing was launched.

## 1. Answer

`param_1` of `5A120` is the GX **view** object (vtable `143D52328`, constructed by `139CE0`, 0x1380 bytes; clone `139F40` = vtable slot +0x78, assignment `13AA60` = slot +0x08). The view is built **per frame on the stack** of the scene functions; its `+0xCA4` is stored **once, by `FUN_140024740` (the scene draw), at the `fStack_744` store** (stack offset `0x13E8 - 0xCA4`, the view starting at `apuStack_13e8`):

```
view+0xCA0 (byte)   = [pipeline+0x3EC4]        view+0xCA1 (byte) = [pipeline+0x3ECF]
view+0xCA4 (float)  = YEBIS view        ? [144962158]
                      other views       : 0.2 / max([pipeline+0x3E88], 1e-6)
YEBIS view = ( [144961F70] != 0 ) and ( ([pipeline+0x50] >> 10) & 1 )
```

(`140024740` lines 190-202; the constant `143D7ED94` = 0.2f (`3E4CCCCD`), `143D7ECF0` = 1e-6f (`358637BD`).) The condition is the same as `FB890`'s YEBIS gate (TONEMAP-SKIN, "YEBIS runs only when pipeline flag bit 10 is set"): the world view has it, the menu views do not (MENUS-MAP section 12).

**`144962158` is the motion-blur global written by `E4B50`** (from `FB890` line 418, once per frame): `144962158 = max(param_1[3], 0.001)` where `param_1` is the blended gparam **MotionBlur** struct (`1D9910` blend of the two keys of bank `[GXRS+0x4640]`). Field `[3]` is a lerped float. Its value:

- `MotionBlur::MaxBlurLength` = **0.1** in all five gparam files that have a MotionBlur group (`default` id 0, `mapbase` id 0 in both `gamedata` and `gamedata-base`, `remobase`, `m_template`); the 18 map files and the overrides have **no** MotionBlur group, so the blend is 0.1 everywhere. This is the live 0.1 (title uses `default`, game `mapbase`: same value). `BlurTimeRatio` (0.06 / 0.2) and `MotionBlurScale` (1.0) cannot give 0.1.
- So for the world view **k = 1 / 0.1 = 10**, and `FC_DifColMul = (aspect·0.5·10, −0.5·10, 0, 1)` = (8.889, −5, 0, 1) at 16/9, exactly what the pilot read.

INFERRED point: that struct field `[3]` is `MaxBlurLength` (file order and in-memory order are not identical: the blend treats fields 1, 4, 5, 9 as copied, 2, 3, 6, 7, 8, 10, 11 as lerped). The value 0.1 equality, the clamp at 0.001 and the unique float in the gparam group equal to 0.1 support it; a one-line live check closes it (§3).

## 2. Why nothing else showed up

- `view+0xCA4` stores in the executable (all `[reg+0xCA4]` forms, plus 16-byte stores at `+0xCA0`, `+0xC98..`): `139CE0` (constructor, 1.0), `139F40` / `13AA60` (copies), and the stack-local stores of the callers. The 16-byte candidates of the pilot's list are **other classes**:
  - `140F4A000`, `140F87CE0`, `14200CCF0`: an unrelated 2D transform class (rotation by sin/cos polynomial; `+0xCA0/+0xCA4` hold the origin x/y copied from `param_2`);
  - `140CFF200`, `140F459B0`: its constructor and copy constructor;
  - `1420A7BA0` (`[RBP+0xCA4]`): stack locals of a 27 KB function (`+0xC90 = 0x7F7FFFEE`, `+0xC98 = 0x7FFFFFFF` bounds initialisation), same for the other `[RBP]`/`[RSP]` hits.
- The view is a **stack** object (apuStack_13e8 in `140024740`, apuStack_13c8 in `140205220`): that is why a write-to-field search by displacement misses it (`RSP+0x744`).
- `140205220` (the second constructor caller) is a small utility pass that builds its own view and never stores `+0xCA4` (stays 1.0).

## 3. What stays live-only (one read, same launch as M1/M2)

| ID | Read | Expected |
|---|---|---|
| CA4·1 | at `5A120` entry in the main scene: `view+0xCA4`, `[144962158]`, `[144961F70]`, `[pipeline+0x50]` bit 10, `[pipeline+0x3E88]` | `0.1`, `0.1`, non-zero, 1, any (not used by the world view) |
| CA4·2 | a menu view (Status/Equipment, no bit 10): `view+0xCA4` and `[pipeline+0x3E88]` | `0.2 / max([+0x3E88], 1e-6)` (`+0x3E88` is a MotionBlur field, not an exposure: I-0113 RESULT « Tranche 27 »; menu views draw no `Velo` rows when `+0x3EB9` is off; informational) |

## 4. For the native implementation

Produce `view+0xCA4` exactly as `140024740` does: YEBIS view (the native world view) → `max(MotionBlur::MaxBlurLength blended, 0.001)` (the same blend and factor as `E4B50`); any other view → `0.2 / max([pipeline+0x3E88], 1e-6)` (a MotionBlur field, not an exposure). With the files on disk the native value is the constant 0.1 for every zone, so a constant would match today, but it must come from the gparam blend (D-0161: per mechanism, not per asset): a map or override file may later add a MotionBlur group.

Consistency note (no correction): `144962150` is the mode, struct field `[1]` (copied by the blend, an integer; `E69A0` line 660 sets it to 1 after `CAB90` accepts the application velocity source). The in-memory MotionBlur struct therefore is **not** in gparam file order (file index 1 is the float `BlurTimeRatio`), which is why field `[3]` = `MaxBlurLength` stays INFERRED until CA4·1.
