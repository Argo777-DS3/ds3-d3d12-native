# Phantoms natively (workstream started 2026-10-11 06:55, branch `claude/phantoms` from main 2b72f0e7)

Specification: I-0067 [PHANTOMS-SIGNS-NATIVE](../I-0067-static-menus-subsystems-map/PHANTOMS-SIGNS-NATIVE.md) (checklist P1-P18, live
rows PL1-PL10). Started in the NPC families thread while the coordinator opens the phantoms thread.

## Native1218-1223, Reference1218 and Reference1223 (Argo at the High Wall start)

- **Phantom witness (test tooling, both modes).** `witness-phantom.txt` next to the module holds a PhantomParam id; on every
  WorldChrMan schedule call the player's first resolver source `chr+0x1F38` takes it (88A950 returns it when not negative), so
  the genuine chain (9CAF30 pushes the seven `ExParam_Phantom*` overrides, slot 214 stores the id, the ChrAsm parts get it at
  descriptor `+0x44`) makes the player that phantom in either renderer. Pilot: `Pilot-LoadGameThomas.ps1 -Phantom id`,
  `Run-PilotPair.ps1 -Phantom id`; event `witness_phantom`. Reference1218 (80) shows the white phantom, Reference1223 (81)
  the red one.
- **P4, part state (CFDE70).** Native does not run CFDE70 (its selection visitor transcribes it). A ChrAsm part whose
  descriptor `+0x44` is an id with a PhantomParam row (E35EF0 lookup, pinned) now gets CFDE70's stack-state changes on the
  native copy of the group bytes: `+0x87 = 1`, alpha times the row fade (`+0x24`), `+0x84` = row kind (`+0x2C`; kind 0 gives
  1 below an alpha of 1, else 0xFF). Native1219: the player's parts move to category 0x21 (rows 0x21 and 0x22 appear).
- **P8, rows.** Category 2 keeps the ordinary rows; category 0x21 takes rows 0x21 (class mask 0xFEFF) and 0x22 (0x100, only
  with `+224` bit 15); category 0x29 takes the generic forward row 0x29 (mask 0xFFFF). Shader pass 0 (`Fwd`) for all three;
  0x22 would be `FwdSdw` (explicit policy while native has no shadows). The C5 alternate recipe (`<family>_Phantom_Fwd[_[A]]`)
  is chosen from the native phantom flag for ChrAsm parts (the shared group byte stays 0).
- **P9, states.** Rows 0x21/0x22: opaque, depth tested LESS_EQUAL and written. Row 0x29 of a category-0x29 model: alpha blend
  whatever the material blend mode (explicit policy). One HDR target (the SAT and Fur phantom programmes also write `o1`,
  unbound as for their normal `Fwd`).
- **P3, constants (22D620/22E930).** `cbMatDynParam` rows 26-30 (`+0x1A0..+0x1EF`) = `(r*r*a*s, g*g*a*s, b*b*a*s, a)` from
  the material phantom block (`material+0xD0`: flag `+0x22`, subtract bytes `+0x70/+0x71`, colours `+0x80..+0xCF`) after the
  override rows (category 8-14, else the key). Two native gaps found and closed:
  - 141070, which initialises the block (flag, zeros, Diff and Spec = the 16 bytes at `143D7FEF0`) for the 43 character
    shader names or the bundle resource flag, does not run natively (Native1219: player parts invisible, rows 26-30
    unwritten). Explicit policy: the same initial values for a material whose family has its `_Phantom_Fwd` programme.
  - The key globals `4962354..4962330` are zero natively (their initialiser does not run); the override rows carry kind 0, so
    the lookup goes by key (Native1221, witness `native_phantom_rows`). The keys are the inverted CRC-32 of the UTF-16 names
    (`ExParam_PhantomIsEdgeSubtract` 3062593120 ... `ExParam_PhantomLightColor` 1936581433, equal to the rows' keys); native
    computes them.
- **Result.** Native1222 (80): white silhouette like Reference1218; Native1223 (81) against Reference1223: red phantom in
  both. 0 debug errors, no fault, configuration restored each time. Unit test of rows 26-30 (white constants, subtract sign,
  no flag, unavailable overrides).

## Native1224 and Reference1224 (row 74: kind 3, fade 0.5)

- The CFDE70 changes now also reach the instance record (`+0x87` instance flag, mulCol.w times the row fade) and the PS
  `cbModelParam` draw-state copy (`+0x87`, `+0x84` kind, `+0xDC` alpha).
- Category 0x29 (kind 3): the generic forward row 0x29 (56 rows), alpha-blended. Native1224 against Reference1224: the
  translucent white phantom in both; native shows more of the inner surfaces (depth not written for 0x29, explicit policy).

## Open

- Single models (`SprjModelDrawEntity`, D07140 -> D3FE90/D40080 from `entity+0xC38`): not transcribed natively; NPC
  phantoms that are single models are not covered yet. The summoned NPC `c0000_0007` is ChrAsm (covered by P4).
- `FwdSdw` for row 0x22, the 0x21 position among the effect passes (P10), fading 0x29 states against the original (PL6),
  signs (P12-P14: effect system), PL1-PL10 reads on the original.

## Behaviour changes to note

- A category-0x29 model that is not a phantom (other `+0x84/+0x85` combinations, or the forced-translucent global
  `4962314`) was refused before; it is now drawn in the forward row 0x29 with forced alpha blend.
- Category 0x21/0x29 models take no velocity row 0x34 (whether the original gives them one is unverified).

## Pre-merge review (fresh Opus 5.5 agent, high effort, D-0163)

Nothing blocking: the depth write of rows 0x21/0x22 goes through the writable DSV with the depth in DEPTH_WRITE after
lighting, the PSO key includes the depth and blend states, category 2 is unchanged and rows 0x21/0x22 partition the meshes,
the override gate matches 22D620, the E35EF0 call runs where CFDE70 itself would, the diagnostics are bounded, no D3D11.
Fixed: the pilot's phantom checks used unquoted patterns and never matched (now the quoted JSON, as for the teleport); the
unit test also covers kind-0 rows found by key and the material-index gate; the leftover `keys_read` constant is gone; the
forced-id file can be replaced or released (0) and its log is bounded; the subtract test compares the target's bits (as the
original's integer compare). Not changed: the E35EF0 assertion if the param repository is absent (descriptor `+0x44` is
only set once params are loaded). After the fixes: full build with the control suites and the model-preparation test PASS;
no live rerun.
