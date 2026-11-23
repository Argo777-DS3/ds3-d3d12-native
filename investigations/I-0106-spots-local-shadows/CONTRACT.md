# I-0106 - Spots et ombres locales

Capability: C3. FINISH-LINE line 6 (D-0221; the order D-0224). Opened 2026-10-29 by the second pilot, in parallel with the
effects pilot (I-0105, line 5, which keeps the priority on the shared PC).

## Question

Draw the map's spot lights and the local-light shadows natively as the original does: the light of the 45 spot lights at the
Anor Londo bonfire and 46 at Central Irithyll (I-0100 RESULT tranche 3 step 1: the warm chapel), and the shadows of the local
lights (the Anor Londo stairs' posts, Thomas's R7 « l'ombre du feu de camp »).

## Scope

Included, in this order (the plan of [SPOT-LINE-LIGHTS-NATIVE](../I-0068-static-yebis-ffx-lights-spec/SPOT-LINE-LIGHTS-NATIVE.md)
§3 and [SHADOWS-WAVE1-READY](../I-0066-independent-native-startup/SHADOWS-WAVE1-READY.md) §1.5 and §3):

1. Spot items: the per-frame 0x280-byte item of each kept spot light (`F1010` → `2136B0`), from correct spot volume state.
2. Spot drawing: the tile binning by the spot's hull, `GXLightAcc_DSpotLight1..4` per tile after the point batches
   (`1A6690`), the `cbLightAccLightParam` rows (`2144C0`).
3. Local-light shadows: the selection (SW-7, today behind `shadow-local.flag`) on by default with spots, the 4096 local
   atlas, the face passes (passes 9..0x38, the category 0x04 casters), and the shadowed programmes `DPointLightShadowed1..4`
   / `DSpotLightShadowed1..3` with `cbLightAccShadowInfo` and the atlas rectangles.
4. The forward rows' `g_SpotLightBuffer` when the spot records exist (SPOT-LINE §3 step 6), if cheap.

Excluded: line lights (never drawn in any capture), the projected spot masks (`SpotLightMask*`, never witnessed), particle
shadows, the cascades (already drawn).

## Method

Full production (D-0227): no measurement gate; unit tests by the build; step reviews and a fresh final review at xhigh before
the merge (D-0198); after the merge the one-minute start check (D-0229). Parallel rules (Thomas, 2026-10-29, relayed by the
coordinator): own worktree and branch, no file of the effects workstream touched; every build announced to the coordinator
first, never during the other pilot's announced test; the second pilot launches no game itself: Thomas runs the visual checks
from the exact command this thread gives him (zones, what to look at).

## Exit conditions

- Success: spot lights and local-light shadows drawn by default natively, the unit tests and the reviews passed, merged into
  `main`; Thomas's look at the Anor Londo bonfire stairs and the Irithyll chapel (what he sees is recorded as he says it).
- Refuted / insufficient: a piece whose engine state cannot be produced natively is recorded as open with its evidence.
- Stagnation: two failed local fixes on one piece open an RCA (WORKFLOW).

## Safety and human interaction

No live run by this thread. Thomas's tests use the existing launcher; his saves are restored by fingerprint as always.
