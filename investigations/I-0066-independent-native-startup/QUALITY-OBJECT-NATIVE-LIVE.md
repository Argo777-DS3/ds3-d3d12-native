# Native GX quality object (menus workstream, 2026-10-11, branch `claude/options-quality` from main 8b5fb118)

Specification: I-0067 [QUALITY-OBJECT-NATIVE](../I-0067-static-menus-subsystems-map/QUALITY-OBJECT-NATIVE.md), checklist Q1-Q8 and
live rows QL1-QL4.

## What is in the build

- **Q1-Q2, Q5 (`PublishNativeSceneRoot`):** the root now gets `+0x5358` and `+0x5360`, as `220A0` builds them.
  - The table is 0x24C bytes, produced by running `219B0` into a scratch copy.
  - The levels are 2, then eleven 3, and the applied copy is equal to them.
  - The lock byte `+0x5350` is 0.
  - The 0x78-byte object is written from these before both pointers are published. Event:
    `native_quality_object published`.
- **Q3 (`game_quality_object.cpp`):** `WriteQualityObject` transcribes the rewrite in `23790` (lines 108-200).
  - Each field is written in place and the padding is left untouched.
  - Levels outside 0..4 are refused.
  - The unit test `game_quality_object_test` runs in the build. It checks the I-0067 oracle images (`Build-QualityObject.ps1`
    over `quality-table.tsv`) for the initial levels and Thomas's levels, that the padding is kept, and that level 5 is
    refused.
- **Q4 (`UpdateNativeQualityObject`):** natively, `CED9A0 -> 23660 -> 23790`.
  - It runs every render step and reads the levels at `[4796298]+0xA8 + 4·slot`. Slot 8 takes the applied copy, as
    `CED9A0` does.
  - Both reads must agree and every level must be 0..4, otherwise the frame is refused.
  - The lock byte is honoured.
  - The object is rewritten only when a level changed. Event: `native_quality_object updated`, with the levels and the
    object bytes (bounded).
- **Q6:** the texture pair `(obj[0], obj[1])` reaches the native samplers through the Texture level selector (see Open, Q6). The native sampler bank
  stays driven by the Texture level selector (`PublishExternalSelector`).
- **Q8 is followed:** no consumer was wired per feature. Each one reads its fields when its native pass lands.

## Live (Reference1255 / Native1255, Load Game on Argo, MotionBlur HIGH)

- **QL1, original:**
  - The levels are `[4,3,3,4,3,4,4,4,3,4,3,4]`.
  - Antialiasing `ON` is level 3 in the block, which settles QL1's open conversion.
  - MotionBlur `HIGH` is level 3, and the object gives `+0x20 = 01 01 .. .. 05 00 00 00` (QL4).
- **QL2, native:** the same levels. The object is equal to the original's on every byte that `219B0` writes. The
  review checked this against the `219B0` asm.
  - `219B0` builds the table from local stores that leave holes. At these levels the undefined object bytes are:
    - `+0x11..13` (DoF, table 0x91..93);
    - `+0x22..23` (MotionBlur, 0xBA..BB);
    - `+0x29..2B` (Shadow, 0x129..12B);
    - `+0x46..47` (Lighting, 0x166..167);
    - `+0x6F` (Reflection, 0x233).
  - In both modes these bytes hold stack garbage. Native1255 differs from Reference1255 at `+0x12..13` (original
    `00 00`, native `80 3f`), `+0x22..23` (`74 f1` / `08 42`), `+0x29..2B` (`7d 04 c4` / zeros) and `+0x6F` (`a6` / `a1`).
  - `+0x11` and `+0x46..47` happened to match. A difference there in a later run is not a regression.
- **Q7:** the native point-light fade now reads `+0x40` = 1.2 at Lighting `MAX` (it was 1 when the object was absent), as in
  the original. The in-game image at the Argo spot is unchanged.
- **Not caused by this change:** closing the window in game still ends on `native_env_region_cpu_generation`.

## Open

- **Q6, closed statically (2026-10-11):** the pair is the table row `s0*8` (quality-table.tsv rows 0x00..0x27: (1,1), (2,1), (3,4), (3,8), (3,16) for levels 0..4). These are exactly the five `profiles_` that `EngineSamplerBankOwner::PublishExternalSelector` applies from the Texture level (`native/content/sampler_bank.h`), the same input as `D3650(texmgr, obj[0], obj[1])`. The native samplers therefore already receive the pair; no code change.
- **QL3, read (Native1263):** with `QualitySetting` set to `LOW` in the file for the run (`Run-PilotPair.ps1 -QualityPreset LOW`; restored by hash afterwards, `CUSTOM` checked after the run), the block levels are still the per-key values `[4,3,3,4,3,4,4,4,3,4,3,4]`. The preset token does not recompute the levels at load: the per-key values decide. The edit is witnessed by the post-run copy `post-0-GraphicsConfig.xml` named in `restoration.json` (`QualitySetting` LOW); `quality_preset` is now also in the run metadata. Observed natively only: the block is filled by the game's own CPU loader in both modes, but no Reference run was made with LOW. The quit in game was clean again (exit 0).
- **The quality listeners `[root+0x4E08]`** do not exist natively. Consumers read the object.

## Pre-merge review (fresh Opus 5.5 agent, high effort, D-0163)

- Nothing blocking.
- Checked against 220A0, 23790, 23660, CED9A0, 236E0 and the 219B0 asm. The highest store of 219B0 is at +0x24B.
- Fixed: the QL2 paragraph (full undefined-byte set), a missing include in the test, and a comment on the 23790 parts with no native peer.
- Recorded: no write barrier before the pointer stores (plain stores on x64, as in the original), and the reference witness logs slot 8 from the file block.

QL3 pre-merge review (fresh Opus 5.5 agent, D-0163): nothing blocking. Restore verified: backup before the edit, restore in `finally` with a hash check, GraphicsConfig back to CUSTOM. Fixed: the preset is uppercased and the edit asserted; the note cites the post-run copy.
