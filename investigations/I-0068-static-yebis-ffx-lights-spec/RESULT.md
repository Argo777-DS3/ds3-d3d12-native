# Investigation result

## Conclusion

Complete (static, parallel to I-0066, 2026-10-10 to 2026-10-11, 118 iterations). Both hypotheses supported: the original's
YEBIS post chain, FFX effect families, effect lights, shadows, SSAO, water, tessellation, glow, velocity, display modes and
the remaining material families were specified statically for native D3D12 production, family by family, and their D3D12
designs gathered in one ordered plan, [D3D12-MASTER-PLAN](D3D12-MASTER-PLAN.md). The wave-1 « ready » sheets (envelope, gamma,
glow, SSAO, YEBIS, effects, shadows, water and tessellation, the second YEBIS tranche, DLSS) are filed in
[I-0066](../I-0066-independent-native-startup/WAVE1-PREP.md) and were consumed by wave 1 (D-0194). What is still open for wave
2 is routed from the [ROADMAP](../../.project/ROADMAP.md) (shadowed local lights, tessellation in forward, the velocity
checklist, the native effect contracts of D-0190).

The conclusion of each part (A to BQ), the evidence, the open questions and the artifacts are in the
[part-by-part log](PARTS-LOG.md); the live checks it left are gathered in [LIVE-CHECKS-CONSOLIDATED](LIVE-CHECKS-CONSOLIDATED.md).

## Evidence

Exports in `decompiled/`, query outputs in `evidence/I-0068/queries/`, the scripts of this folder; per part in
[PARTS-LOG](PARTS-LOG.md).

## Uncertainty / unresolved questions

The static questions left are listed in [PARTS-LOG](PARTS-LOG.md) (« Uncertainty ») and the live ones in
[LIVE-CHECKS-CONSOLIDATED](LIVE-CHECKS-CONSOLIDATED.md); the current open gaps are in [STATE](../../.project/STATE.md).

## Suggested next step

None for this investigation: its specifications serve wave 2 through the ROADMAP.
