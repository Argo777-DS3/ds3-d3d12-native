# Thomas's hypothesis: one cause for the translucent materials and Chameleon (study thread, static, 2026-10-31 evening)

Thomas (19:44): « les matériaux translucides ont une cause commune, c'est comme l'écart pour le sort Chameleon ». STATIC only
(decompiles and assembly in `decompiled/`, GhidraMCP, the native code); nothing launched. Gaps concerned: 17 (Crystal Magic
Weapon's crystals), 5 (the Ashen flask), 11 (Chameleon alternates visible and invisible, Hidden Body's hair).

## 1. How the original chooses a model draw's blend and depth (VERIFIED in the code)

Every model draw, ordinary or E03, goes through the same consumer: the draw item Q → `5A120` (the draw-state preparation) →
`1A86C0` (the arguments' commit) → the mesh loop `63CB0` → the material's `Begin` `140140680` → `64610` / `19C590` (commit).

- **Depth** (`140140680:248-283`; corrected after the pilot's final review, §6): the material class `c` (Begin's
  `param_1+0x18`, the value 142130 computes) decides the branch.
  - `c & 0x10D == 0` (or `args+0x200` bit 17), and `args+0x18C` bit 10 clear: `args+0x188 == 1`, or `args+0x188 != 0` with the
    MTD's `+0x81` clear → `[[144961AB0]+0x2A0]+0x1E0` (LESS_EQUAL, written); otherwise `+0x1E8` (tested, not written).
  - otherwise the forward pair, which marks the stencil: `args+0x188 != 0` and the MTD's `+0x111` clear →
    `[[GXRS+0x4DE8]+0x860]`, else `+0x868` (§6 for what they are), with a stencil reference from `args+0x18C` bit 2, the MTD's
    `+0x80`, `args+0x1C == 6` and the bits 7 and 11 of the material's word `+0x24` (`140140680:278-283`).
  - `args+0x188`: 5A120 sets it to `Q+0x1AA` for preparation 0 (`0005A120.c:262-263`), but 29ADF0 **overwrites it after
    59470** for an E03 particle (§6).
- **Blend** (`1A86C0:180-190`): when the draw state's `+0x84` (`args+0x2C4`) is not -1, the blend
  `[[GXRS+0x4DD0]+0x2F0 + 8 × state+0x84]` is bound; otherwise none (the context keeps the blend it has).
- **Alpha pixel variant** (`5A120:157-165`): `Q+0x1A9` → `S+0x200` bit 20 (`_Fwd_[A]`).

What fills Q differs by family:

- **Ordinary models** (characters, phantoms, a fading character): `5C0E0` fills Q from the descriptor and the entity's fade
  group (`Q+0x1A9` = fade on and opacity < 1, `Q+0x1AA` = the descriptor's byte `+0x0C`), and the state's `+0x84` is the group's
  kind (recomputed by `140D3FE90`, set to the phantom kind by CFDE70).
- **E03 model particles** (crystals `s04020`-`s04022`, the flask `s07002`): `29ADF0` builds Q on its own stack and **writes
  neither `Q+0x1A9` nor `Q+0x1AA`** (assembly: no store to `RBP+0xF9` / `RBP+0xFA`; `59470` writes only `Q+0x1A4`, `+0x1A5`
  there), and sets the state's `+0x84` to -1 (no blend from 1A86C0). So the original's alpha variant for these particles comes
  from a byte left on the stack (STUDY-GAP-17); `Q+0x1AA` is a leftover too, but it does not reach the depth: 29ADF0 rewrites
  `args+0x188` from the FFX command (§6). Their blend comes from the material's blend parameter, §5.

## 2. What the native does instead (VERIFIED in the code)

The native runs none of 5A120, 1A86C0 or Begin for these draws; it has its own rules (`13b_prepare_model_frame.inc:405-444`):

- **E03 particles**: the effect context's pending blend and depth at 636F0's entry (additive 0x20, depth 0x1E8 unwritten in
  the witnessed rows), or the row before it in 0x29 (tranche 5's `ResolveNativeEffectRowStates`); the alpha variant from the
  5C0E0 formula, which the original does not apply to this path.
- **Ordinary fading models** (category 0x29, Chameleon's and Hidden Body's case if their disguise is a fading character, below):
  an **explicit policy**: alpha blend forced whatever the material, depth written on the 0x29 and 0x1F rows (« INFERRED from
  the picture », the phantom of Native1878); the category (0x21 opaque or 0x29 translucent) chosen at the model's admission
  from the group's `+0x84/+0x85/+0x87` and `+0xDC` against the scalar 1.0.

## 3. Verdict

- **Verified**: the two families share one cause in the native: **a translucent model draw's blend, depth and pixel variant
  are not the original's per-draw choice (5A120 → 1A86C0 → Begin), but native rules** (the effect context's states for E03,
  a fixed policy for fading characters). Transcribing that choice is the common fix: Begin's depth rule from `args+0x188` and
  the material, 1A86C0's blend from the state's `+0x84`, 5A120's `Q` fields.
- **Verified, and it limits the hypothesis**: the original's inputs differ by family. For E03 particles the alpha pixel
  variant is a stack leftover (`Q+0x1A9`), which varies from run to run and is not the cause of the look (the pilot: `_[A]` 274
  of 288 in one run, mostly `_Fwd` in the next); their blend and depth come from real data (the material, the FFX command).
  For fading characters every input comes from real data (the fade group), so a transcription can be exact.
- **Not verified**:
  - that Chameleon's disguise is drawn as a fading ordinary model: its cast FXRs (523550, 523551) carry no 605 model, so it
    is not an E03 particle; how the disguise model is drawn (a character fade, a camouflage flag of the SpEffect's VFX) was
    not read;
  - that Chameleon's alternation comes from the state rules: the native's admission flips between 0x21 (opaque) and 0x29
    (translucent) when the group's values sit at the scalar's edge (RESULT gap 11's lead), which is a second native rule
    (the category), not the blend or depth;
  - (found since: the blend's setter, §5).

## 4. Checks that would settle it

1. The pilot's `blend_by` for the crystals (in progress) and the same named oracle on Chameleon's disguise (its model name
   from a native journal of the cast): the original's blend, depth, PS variant and category per draw.
2. For Chameleon natively: the `native_zero_fade` / admission rows of the disguise over a few seconds, the category each
   frame (0x21 or 0x29) and `+0xDC`; a flip in step with the pictures puts the alternation in the admission rule.
3. If both families show the original's per-draw choice where the native applies its rules, transcribe 5A120's Q → args
   fields, Begin's depth rule and 1A86C0's blend table once, for every forward model row, with the E03 leftover (the pixel
   variant) replaced by its observed value.

## 5. The blend's setter, found (VERIFIED in the code; the pilot's oracle `i113-t9-crystal-oracle2` / `-flask-oracle2` saw it)

The oracle placed the crystals' and the flask's alpha blend (rt0 SRC_ALPHA / INV_SRC_ALPHA, every draw) at 140015220's call to
the blend wrapper, inside 63CB0. The caller is the material's blend parameter:

- **`140200840`**, slot 1 of **`GXFlverMaterialParamBlendMode`** (vtable `143D60590`, RTTI `.?AVGXFlverMaterialParamBlendMode@GXSR@@`),
  run per draw by the MTD producers `14002E5C0` that Begin calls (`140140680:190`) with **`args+0x200`** as the flags (Begin's
  `args` vt+0x50 is `1401A8570`, `lea rax,[rcx+0x200]; ret`; 1A86C0 is not on this path).
- Its rule, `f = args+0x200`:
  - `f` bit 0 set (1A86C0 already bound the kind blend `[[GXRS+0x4DD0]+0x2F0 + 8 × state+0x84]`, i.e. `+0x84 != -1`): nothing;
  - else `m` = the material's mode (the parameter's `+0x10`, or the per-instance value 140146920 finds by the key `+0x14`),
    `r` = `args+0x204` (1A86C0 sets it to the state's `+0x85` when `+0x84 == -1`), `i` = the table `14473E5C0[r × 6 + m]`:
    rows `r0 {0,0,1,2,3,4}`, `r1 {1,1,1,2,3,4}`, `r2..r4` all 2, 3, 4 (row 0: the material decides; row 1: at least alpha;
    rows 2-4: forced);
  - then `f` bit 16 → `i + 15`, bit 17 → `i + 20` (those two, and bit 11, also set the depth `0x1E8`), bit 12 → `i + 10`,
    bit 21 → `i + 0x33`, bit 15 without bit 21 → `i + 0x19`, bit 19 → `i = 0x32`;
  - bound: `[[144961AB0]+0x2A0] + 8 + 8 × i` (the FFX bank: `0x08` off, `0x10` alpha, `0x18` multiply, `0x20` additive,
    `0x28` reverse subtract, then the independent and twin rows).
- The crystals and the flask (material blend policy 0, `+0x84 = -1` from 29ADF0, `+0x85` = the FFX command's `+0x30`) get
  `0x10` with row 1 (INFERRED: `+0x85 = 1`, to read in the oracle). The ordinary models go through the same two choices
  (1A86C0's kind blend when the fade group's `+0x84` is set, otherwise this parameter with `+0x85` as the row; 140D3FE90
  recomputes both from the fade).
- **So the hypothesis holds at the code level for the blend: one rule (1A86C0's kind table, else the material's
  `GXFlverMaterialParamBlendMode` with the group's `+0x85` row and the draw flags) chooses every forward model draw's blend in
  the original, the E03 particles and the fading characters alike; the native runs it for neither.** Chameleon's own inputs
  (its `+0x84/+0x85`, its category flips) stay unread.

**Confirmed in a run (the pilot, OBSERVED, `i113-t9-crystal-oracle3`, the three crystals' 861 draws):** the blend's call stack
is `15254` (in 140015220) ← `200A03` (140200840, BlendMode slot 1) ← `2E6D2` (14002E5C0) ← `140BC0` (Begin 140140680); the depth
is bound at `19C65D` (19C590). The pilot's native fix (commit `8c88cf4d`, branch `claude/relaxed-nobel-cadf95`) applies the rule
(table `14473E5C0[group +0x85][material mode]` and `args+0x200`'s bits read at 636F0): the native crystals take blend 0x10
and depth 0x1E0 and look like the original's. Its depth is an explicit policy (written when the table's index is 0 or 1;
the context's or the previous row's state for an additive one not yet seen); the original's depth is Begin's rule (§1, §6),
which that policy matches for the draws seen.

## 6. Begin's depth inputs for an E03 particle (VERIFIED in the code; the pilot's final review found the first error)

- **`args+0x188`** (correction): 29ADF0 writes it right after 59470 returns: `14029B23E MOVZX EAX, byte [RBX]` (RBX = its third
  argument, the FFX command), `NEG AL; SBB ECX,ECX; MOV [RBP+0x468], ECX` (`args` = `RBP+0x2E0`), i.e. **`args+0x188 = -(cmd[0] != 0)`**,
  then 1A86C0. So it is 0 or -1, never the leftover `Q+0x1AA` (which 5A120 wrote there first). The earlier version of §1 said
  otherwise; withdrawn.
- **The forward pair** `[[GXRS+0x4DE8]+0x860]` / `+0x868`: created by `1400FF630` (called by the rendering system's setup
  `1400220A0` right after `1400FF3C0` builds the 0x9F0-byte object stored at `GXRS+0x4DE8`): each is the effect bank's state
  read back (`14192F760`) and re-created (`1419253B0`) with a stencil added: **`+0x860` = bank `+0x1E0` (depth on, written,
  LESS_EQUAL), `+0x868` = bank `+0x1E8` (depth on, not written, LESS_EQUAL)**. The stencil words written over the description
  (bytes 12-14 `01 00 FF`, faces 0, 0, 2, 7) read as stencil enabled, read mask 0, write mask 0xFF, fail KEEP, depth-fail KEEP,
  pass REPLACE, function ALWAYS: INFERRED (the GX description's layout and its conversion 14192F760 / 1419253B0 not read). So depth-wise the pair equals `0x1E0` / `0x1E8`; it also writes the stencil reference.
- **The class** (Begin's `param_1+0x18`, tested against `0x10D`): Begin ends by storing `142130(material)` there
  (`140140680:288-291`), and the native's `material_class` is `ClassifyModelMaterial`, « Exact142130 precedence »: the same value
  (the stored one is the previous Begin's result, equal in steady state). **Class 1 has bit 0, so the crystals and the flask
  take the forward pair branch** (unless `args+0x200` bit 17): written (`+0x860`) when `args+0x188 != 0` and the MTD's `+0x111` is
  clear, i.e. when the FFX command's first byte is nonzero, which matches the pilot's « written, LESS_EQUAL ».
- **`args+0x18C` bit 10**: 236290 (the arguments' base constructor, called by 1A8580) sets `+0x18C = 0`; 5A120 sets bit 10 only for
  preparation 1 under pass 0x3B (`| 0x404`, `0005A120.c:270`); for preparation 0 it sets bits 6, 7 and 11 (`| 0x40`, `| 0x80`,
  `| 0x800`, then calls `140075B30`, which never writes `+0x18C`: the step review read it); 29ADF0 writes no `+0x18C` (no store
  to `RBP+0x46C`). So **bit 10 is 0 on the E03 path** (VERIFIED); natively 5A120 does not run, so it is always 0 there. It only
  matters for the first branch, which class 1 does not take.
- Also on this path: 29ADF0 sets `args+0x200` bit 21 from the command's byte `+0x34` (`14029B204..B216`), which 140200840 turns
  into `i + 0x33` (a twin row) for the blend; bit 17 is set by 5A120 only for preparation 4 (`| 0x20100`).
- **The MTD's `+0x111`** (the pilot's question): `142130` returns class 2 when `gxMtd+0x111 != 0` or `gxMtd+0x88` bit 21, before
  it can return 1, 4 or 0x100 (`00142130.c:20`): **a material of class 1, 4 or 0x100 has `+0x111 = 0`** (VERIFIED); only class 8
  (a decal, `+0xD0` set) returns earlier with it unknown. So for the crystals and the flask the pair reduces to `+0x860` when
  `args+0x188 != 0`, else `+0x868`. The byte's MTD name was not found (no writer in the GX range; `2DD60` writes `+0x110` only);
  `140F90` copies it to `material+0x21`, the byte 141070 sets for SSS materials (`g_SSSWidth`): INFERRED, the MTD's SSS flag.

Pilot's note (I-0113 tranche 9, after the final review): the native depth no longer follows the policy §5 describes; it is Begin's rule
(§6): the class-0x10D branch's forward pair by args+0x188, the other branch by args+0x188 and MTD +0x81; the pair's stencil not modelled.
