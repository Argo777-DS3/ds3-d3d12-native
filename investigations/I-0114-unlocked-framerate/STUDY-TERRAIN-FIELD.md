# The terrain field's « +18 % at 240 »: what it is and whether it matters (study thread, static, 2026-11-01)

Thomas (2026-11-01 23:25, on the R thread): « Je fais analyser les 18 % à l'étude ». The point: on Anor Londo's stairs the mean |x|
of the terrain field `[ctrl+0x2DC]` is 0.124 at 240 against 0.106 at 60 (series 35, the three comparable legs), while the motion
is the same (RESULT « Seventh tranche », « Open »). STATIC only (GhidraMCP, the decompiles, the native code); nothing launched.

## 1. What the field is

- **Its owners (VERIFIED)**: `1405E6580` is slot 27 (vt+0xD8) of `PlayerCtrl` (vtable `1427CF080`, RTTI `.?AVPlayerCtrl@NS_SPRJ@@`),
  `1405D2280` slot 28 (vt+0xE0) of `EnemyCtrl` (vtable `1427CCE08`, `.?AVEnemyCtrl@NS_SPRJ@@`); both delegate to `1405CA3F0` in
  one state. The vtable entries were found by searching the executable's `.rdata` for the functions' addresses (the game folder's
  `DarkSoulsIII.exe`, read only); Ghidra had no reference to them. So the field is a member of the character controller
  (`ChrCtrl`): one per character.
- **What updates it (VERIFIED, `1405E6580`)**: each call asks two foot probes (`[ctrl+0x358]`, `[ctrl+0x360]`, through
  `1407D7180` / `1407D7780`) for an answer; with a hit the field moves by `0.2 × min(answer_a, answer_b)`, with none it decays
  (`× [ctrl+0x378]`, or `× 0.6` in `1405CA3F0`), then it is clamped to ±0.5. When `14088CFF0` is false (it is `[WorldChrMan+0x80] == chr`, the local player; the final review of I-0114 tranche 8): reset to 0.
- **What the probes are (VERIFIED code, INFERRED name)**: `1407D7180` copies a Havok pose's bone transforms, calls a solver
  (`14126E2B0`) and writes `*answer = solver_vertical + field_in` (superseded, see the Decision: `solver + its eighth argument`; the field passed in when `ctrl+0x374` is set, else 0), then
  rewrites one bone of the pose; `1407D7D00` computes a ground height for the foot from the pose. This is the shape of Havok's foot
  placement IK (`hkaFootPlacementIkSolver`: a vertical error per foot, the lowest one moving a body offset with a gain): the field
  is **the vertical offset of the body (the hips) that foot IK applies so both feet reach the stairs** (INFERRED from the code's
  shape and the gain/clamp/decay pattern; Havok's own names are not in the binary).
- **Who reads it** (corrected after Thomas's « s'aider de TRTF comme base sans copier », 23:27: the first version said only the
  probes read it):
  - TRTF (the timing project's I-0013, `timing/investigations/I-0013-terrain-collision-high-framerate-rca/RESULT.md`, K-0062,
    K-0063, VERIFIED live there): `1405C75F0` sums the producers' contributions into the **vertical of the character's local
    transform**: every final output equals the source XYZ plus the carried value `+0x250`, and that carried value is
    bit-identical to this field in every row (the other producer `1408BC9A0` contributing zero). So the field **moves where the
    character is drawn, vertically**; it is what made the character hop on slopes and stairs at 240 before TRTF.
  - The mover does not take it: the move step `1405CA570` calls slot 27 at `1405CA642` and ignores the returned XMM0 (VERIFIED
    here); the movement vector `[ctrl+0x1D0]` goes on untouched, and TRTF found the source/root monotone at both rates. Speed,
    distance, collision and jumps do not read it.
  - The foot probes take it as an input: TRTF saw the mode byte `ctrl+0x374` at 1 in every row, the case where `1405E6580`
    passes the field to `1407D7180` / `1407D7780`, whose answer is `solver_vertical + field` (superseded, see the Decision).

## 2. Is « +18 % » a real difference with the original

- **Size**: 0.106 → 0.124 in mean |x| is about **1.8 cm** more average vertical offset of the drawn character on the stairs (the
  field is in metres, clamped at ±0.5).
- **Noise**: R's own figures put it inside run-to-run noise: series 30, two 60 runs differ by 70 % in the same measure (RESULT
  « The terrain field's level »); series 35 is one run per rate. The original uncorrected at 240 was 0.155 (+46 %): the correction
  removed most of the difference, and the field's variation (the hop, TRTF's measure) is back to 60's within 12 %.
- **Why a residual is expected (INFERRED)**: the native law is TRTF's (`x + (x' − x)·k` for the response, `retention^k` for the
  decay), which TRTF derived « for constant response over an interval » (`timing/evidence/I-0013/TIME-LAW-DERIVATION.md`). With the
  mode byte at 1 the response is not constant: the probes' answer contains the field itself (`solver_vertical + field`; superseded, see the Decision), so the
  update is a feedback loop on `x`, for which the linear conversion is exact only to first order; a small level difference at 240
  with the variation right is what such a residual looks like. How much of the +18 % it explains is not settled statically (the
  solver `14126E2B0` is Havok's).

## 3. Verdict and recommendation

- It is **a visual quantity** (how high the character is drawn over uneven ground, from foot IK), **not a gameplay difference**:
  speed, distance, collision and jumps do not read it. The hop it caused is fixed (TRTF's measure back to 60's). The +18 % is not
  established beyond noise, and if real it is about 2 cm of average vertical offset on stairs, below what an eye can tell.
- **No correction needed now.** If Thomas or R want to close it fully: one short run logging per call the field and the two probe
  answers on the stairs at 60 and 240 (`locomotion-observe.flag` already writes the field) gives the feedback's coefficient; an
  exact law for a linear feedback `x' = a·x + b` is `x·a^k + b·(a^k − 1)/(a − 1)`, a small change in `logic_rate.h` with its unit
  test.

**Decision (Thomas, 2026-11-01 23:35, corrected 2026-11-02 03:13):** Havok is not updated or replaced (statically linked in the
executable, also the animation, foot IK, ragdoll, cloth and collision engine; a replacement would change the game's feel), and Thomas
asked for the +18 % to be corrected without touching Havok (« je voulais corriger le +18 % sans mettre à jour Havok »; the first
reading, « no correction », was the coordinator's misunderstanding); tranche 8 found nothing to correct in the terrain law. I-0114 tranche 8 (RESULT): §1's « `solver_vertical + field_in` » is corrected (1407D7180 writes
`*answer = solver + its eighth argument`, the mover's `+0x1A4`; the field, its ninth argument, enters the foot's ground height
`1407D7D00`), but that branch is not the player's: on the stairs the player's calls are delegated to `1405CA3F0`, whose ground query
does not read the field, so §2's feedback reading does not apply to the measured value. Read leg by leg over 16 runs, the level is
the same at 60 and 240 from a standstill; the difference is on the leg after the turn, where the 240 runs walk another line; with the
enemies frozen (series 39) the climb to the last step gives the same level at both rates: nothing to correct in the terrain
law (what moves the line after the turn at 240 is open, RESULT).
