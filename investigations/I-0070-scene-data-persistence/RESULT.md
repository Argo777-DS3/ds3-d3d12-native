# I-0070 result: scene data persistence, first tranche

Complete (2026-10-20, merged on Thomas's word, 00:39 Paris: « Fusionne déjà ce travail »). The next tranche follows the
architecture of Elden Ring (D-0195): the ER study of persistence, texture binding and distant fades is I-0071.

## Conclusion

H1 confirmed. The texture hitches of the Firelink walk, the bursts at Archdragon Peak and most camera-turn hitches had one
cause: every model record (C7-2) compared global epochs that move at every arrival while the world streams (a texture
admitted, made resident or retired; a model storage, material or layout inserted or erased; a shader, effect or material
admitted). One arrival anywhere invalidated every visible model's record and dropped every unvisited one; the next collect
admitted all of them afresh and looked every texture up again. Before the change the run outcomes show it directly: `epochs`
is the largest invalidation (9,124 in the Firelink walk, 68,921 in the Archdragon Peak walk), and the burst windows reach 50 to
390 fresh admissions and 1,000 to 6,000 texture lookups a collect.

H2 confirmed in part: the camera-turn hitches standing still at Archdragon Peak share the cause (185 frames over 34 ms with
main, 64 after), plus two retention limits: model records unseen for 120 collects and prepared draws unused for 8 prepares
were dropped, so models coming back into view were admitted and prepared afresh. H0 refuted: the bursts are not new models.

The change (native/frame/game_bootstrap 05, 08, 09, 13; game_texture_ingress):

- A record compares only the rarely moving epochs (`NativeRecordEpochs::SameStable`). The tracked ones are checked by the
  record itself when they moved since its last check: its meshes' material and layout entries are still the registries'
  (`SameNativeRecordRegistries`), their programme choice is the one the admission would make now (`SameNativeRecordPrograms`),
  none of its bound texture records was withdrawn (new `NativeTextureRecord::withdrawn`, set by the ingress on retirement,
  replacement and close; `TextureIngress::Withdrawals`), and its later-candidate and absent texture choices give the same
  outcome when looked up again (`RecheckNativeRecordTextures`, also at the 500 ms expiry, which before re-admitted the model).
  Only a change that concerns the model makes it admitted afresh: a texture arriving for it (`arrived`), a registry entry
  (`registry`), a programme (`programmes`), a withdrawn texture (`withdrawn`).
- A first-candidate texture choice survives registry and ingress versions while its record is not withdrawn.
- Unvisited records are no longer dropped when a tracked epoch moves; a record unseen for 120 collects is kept up to 3,600
  while its storage is registered and none of its textures is withdrawn (cap 8,192).
- Prepared draws unused are kept 600 prepares instead of 8 (bound 12,288, the least recently used first).
- Review corrections: the water depth t56 and the reflection t54 compared as current records (water records only for t54);
  a texture choice another thread wrote is never recorded as this mesh's.

Distant enemies at Archdragon Peak (the reserve of WAVE1-CLOSURE 2.4): the same strafing walk in front of them gives identical
admission outcomes with main and with the change (i70-native-16 / -17, `light-observe.flag`: c2160 4,755 / 4,760 admitted and as
many zero-fade transitions, c1390 8 and 5 in both); the native admission follows the engine's own fade weights. Whether the
original shows the same disappearances is not measured here (I-0071 studies ER's fades and LODs).

## Evidence

Runs `evidence/I-0066/artifacts/i70-native-NN-*` (measure mode, no debug layer, the zone saves Eta m40 and Epsilon m45, the
same key paths), summarised with `Get-RecordSeries.ps1`; builds `evidence/I-0070/artifacts/{main,i70,i70b,i70c,i70d}-build`
(main = 24b3005a). Frames over 34 ms over the whole run, p99 median of the 2 s windows, fresh admissions a collect (worst window):

| Scene | main | change (v1 to v3) |
|---|---|---|
| Firelink walk | 68 over 34 ms, p99 20.2, 105.7 fresh (01) | 33 / 23 / 26, p99 18.6-19.1, 15 fresh (05, 12, 18) |
| Archdragon Peak walk | 277, p99 22.3, 781 fresh (04) | 61 / 55 / 87, p99 20.8-21.3, 47-53 fresh (07, 14, 20) |
| Archdragon Peak camera turns | 185, 960 over 25 ms, 239 fresh (10) | 73 / 64 / 64, 125-158 over 25 ms, 41-51 fresh (08, 11, 21) |
| m45 Ariandel, still | 56.2 frames/s, collect 11.96 ms, 74 over 25 ms (03) | 57.3 / 57.8 / 57.3 frames/s, collect 11.5-11.7 ms, 23-28 over 25 ms (06, 13, 19) |

Records built over the run: Firelink 32,494 to 4,040-4,203; Archdragon Peak walk 137,040 to 14,553-15,335; m45 119,305 to
32,059. The m45 collect stays near 11.6 ms: there it is the walk of 1,570 reused records (the per-entity collect, OPTIMISATION-
REVIEW 8.5), not rebuilds.

Guards: `record-revalidation.flag` (every record admitted afresh and compared mesh by mesh every 8 collects) and
`-PrepareShadow` (every 30th kept draw rebuilt and compared): i70-native-09 (v1, 200,574 comparisons), 22 and 23 (v3, 199,062
and 444,676 comparisons, prepare shadow on), 24 and 25 (v4, the merged build: 199,734 and 422,552 comparisons, 20,106 and 4,774 kept-draw checks): no record mismatch, no kept-draw
mismatch (1.47 million record comparisons in all).
Pictures: start, arrival and sweep screenshots of main against the change, 4x3 luminance regions (`Compare-ShotRegions.ps1`):
Firelink and m45 0.97-1.01; Archdragon Peak within main's own run-to-run spread (0.92-1.12 between two main runs: sky and fog).
Sample profile at Archdragon Peak after the change (i70-native-15): the texture rechecks are under 0.2 % of the engine thread.

## Uncertainty / unresolved questions

- The hitches left are not record losses: a burst of texture admissions ~~decoded on the engine thread~~ (a 117 ms frame with 42
  textures at Firelink), the prepare of genuinely new rows (40-55 ms frames while walking at Archdragon Peak), and new models
  admitted. Wave 2's loading and threading workstream (decoding and preparation on workers, the copy queue) is their remedy.
  **Refuted cause (I-0074, K-0457, 2026-10-20):** no texture was decoded on the engine thread (`texture_decodes_owner` 0 in
  every I-0070/I-0071 run); the engine thread waited on DS3's blocking join of the texture fragments (I-0073 4.1) while each
  native fragment copied its whole TPF container.
- Kept draws and records hold their GPU resources longer (up to 10 s for an unused draw); the global descriptor heap's
  fragmentation heal now happens mainly at area changes (fallback to per-draw tables when it fills).
- A lightmap set object that never becomes resident keeps its record re-admitted at each ingress move (`arrived`: 147-3,714 a
  run, no churn seen).
- Volatile models (dynamic or cloth streams, FaceGen) are still admitted every collect (7-14 a collect).

## Artifacts produced or modified

Code above; `Get-RecordSeries.ps1`, `Compare-ShotRegions.ps1` (this folder); runners: `record-revalidation.flag` in
Run-StartupTrace, `-PrepareShadow` in Run-ZoneWalk and Run-AlphaCast. Pre-merge review (Opus): two medium findings corrected
(t56, the shared texture choice), the low ones applied (runner flag hygiene, one named bound, stale comment, unused operator)
or recorded above (lifetimes, lightmap pending).

## Suggested next step

I-0071 (D-0195): ER's scene persistence, texture binding and distant fades, then adapt this design to it.
