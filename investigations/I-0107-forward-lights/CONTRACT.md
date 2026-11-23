# I-0107 - Lumières des surfaces forward

Capability: C3. Chosen by the second pilot after I-0106 (Thomas, 2026-10-29 20:36: « il prend le choix qu'il veut mais
j'autorise le chantier »); the effects pilot (I-0105) has priority for the ASUS slots and the launcher's rebuilds.

## Question

The forward-drawn surfaces (alpha-tested hair, water, translucent materials, the effect models: 1,849 pixel programmes) read
the map's point and spot lights through a clustered light grid: t59 `g_LightIndices`, t60 `g_LightLookupBuffer`, t61
`g_LightBuffer`, t62 `g_SpotLightBuffer` and the cbSceneParam words +880..+900 (`FC_FrustumNear`, `FC_FrustumSliceScale`,
`FC_FrustumSliceBias`, `FC_LightTile_Width`, `FC_LightTile_NumCellX`), which the original fills with the compute pass
`GXLightOrganize` (1A0080 writes its light buffers and constants, 1A0AE0 dispatches it). Natively the four buffers are empty
and the words zero (I-0068 COVERAGE-MATRIX row 28): no point or spot light reaches a forward surface. Make them as the
original.

## Scope

Included: the organize light buffers (point and spot records, I-0106's items), the organize constants, the native dispatch of
the genuine `GXLightOrganize*` programme into the grid buffers, the grid words of the forward rows' cbSceneParam, the four
SRVs bound to the forward rows (main view; the reflection view if it is the same rule).

Excluded: line lights (never drawn in a capture), the deferred tile passes (done by I-0106 and earlier), the forward rows'
other missing inputs (RLR, IBL) unless they block this.

## Method

As I-0106: static reading first, full production (D-0227), step review and a fresh final review at xhigh, short capture runs
only in slots the coordinator gives (the effects pilot first), the start check after the merge, Thomas's look in his play test.

## Exit conditions

- Success: the grid built natively each frame by the genuine programme, the forward rows lit by the point and spot lights as
  in the original at a witness (the Anor Londo bonfire's hair and water, or Firelink), 0 debug errors; merged.
- Insufficient: a piece whose engine state cannot be produced natively, recorded with its evidence.

## Safety and human interaction

Capture runs on Gamma's save copies, restored by hash; never the OLED; no measurement.
