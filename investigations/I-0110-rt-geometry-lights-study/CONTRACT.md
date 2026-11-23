# I-0110 - Étude RT : géométrie et lumières

Capability: C3. Study thread (read-only, never the pilot), opened 2026-10-30 at Thomas's « Go » on the coordinator's proposal.

## Question

What does DS3 give, statically, to the next steps of [RT-PT-PLAN](../../.project/RT-PT-PLAN.md): (1) for R2 and R5, how the game
stores the geometry a BLAS and a TLAS are built from (vertex and index formats, skinning, alpha test, instances, LOD); (2) for R4
and the lighting pass (D-0237), where the maps' light sources and the emissive surfaces live; and what of it changes the plan.
Added 2026-10-30 05:25 (Thomas's « Vas y »): is RTX Mega Geometry relevant to DS3's geometry.

## Scope

Included: the native code as it stands on `main` (read only); the game's data files recovered read-only outside the repository
(FLVER map pieces of m30_00 and m40_00, one character, two armour parts, the MSB of both zones, every BTL, BTAB and BTPB light
bank); the existing ledger and investigations; DS3 1.15.2 static decompilation (GhidraMCP, or headless when the GUI is busy:
Thomas, 2026-10-30 05:14).

Excluded: any change to `native/`, any build or launch of the game (two pilots are active), Elden Ring's RT as a model (D-0236).

## Inputs

- Native code: `native/frame/game_bootstrap/*.inc`, `native/frame/game_scene_geometry.cpp`, `native/frame/game_model_selection.cpp`,
  `native/content/`, `native/renderer/`.
- Earlier maps: [NATIVE-LIGHTING-MAP](../I-0066-independent-native-startup/NATIVE-LIGHTING-MAP.md),
  [EMISSIVE-GLOW-NATIVE](../I-0068-static-yebis-ffx-lights-spec/EMISSIVE-GLOW-NATIVE.md), MENUS-MAP §54 (the m30 torch), I-0106, I-0107.
- Tools: `Recover-NamedGameData.ps1` (I-0068), SoulsFormats 1.0.1 (pinned), the I-0068 MTD dump `all-mtd.tsv`, this
  investigation's `tools/Get-BtlCensus.ps1` and `tools/Get-FlverCensus.ps1`.

## Exit conditions

- Success: RESULT answers both questions with sources and labels (OBSERVED, STATIC, INFERRED) and names what changes the plan.
- Insufficient information: the open points are listed with the reading that would settle each.

## Safety and human interaction

No live run, no build, no change to `main`'s code. Game bytes stay outside Git and the cloud
(`C:/Users/User/Tools/DS3-Decompilation/gamedata-i0110`); the evidence store keeps names, counts and tables only.
