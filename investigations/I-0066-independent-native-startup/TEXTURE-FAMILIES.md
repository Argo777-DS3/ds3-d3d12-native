# Installed texture ingress families

Historical technical record. Current execution and next action: [STATE](../../.project/STATE.md).
D-0156 supersedes the earlier continuation schedules; proof limits remain unchanged.

The existing DDS resource decoder and package GPU uploader define native asset
coverage; the new installed entry must additionally qualify engine lifetimes.
The earlier whole-game inventory is routed by I-0063 RENDERER-INPUTS.

| Family | CPU producer and native route | Qualification / limit |
|---|---|---|
| Encoded 2D TPF texture | AC500 source pointer/size/index -> 2B260 -> existing TPF/DDS decode -> shared upload recorder | Native10 admitted66 actual textures. CPU object0x70, allocator metadata+60. |
| Encoded cube | Same TPF source -> AC110; cube object0x60, AC020 retirement | Missing family exposed by native10. Source-qualified layout and paired entry/queries/retirement added. Offline BC1 cube12 subresources exact; Native11 completed the actual32x32 format10 six-face startup cube. |
| Direct raw 2D | 2AF40, sixteen-byte CPU subresource records | Padded/non-square mip controls hardware/WARP pass; no live raw requests yet. |
| Encoded arrays/cube arrays | Existing DDS decoder and shared recorder support declared slices/views | Installed class delivery and engine query coverage unqualified. Do not assume another class shares the0x70 layout. |
| Volume / unsupported console packages | Explicit refusal in existing decoder | Separate future responsibility, no empty successful resource. |
| Writable/render-target/depth/UAV textures | Distinct native frame resource ownership | Outside this sampled CPU asset entry. Direct UAV request refuses. No fabricated input or original GPU handle. |

Sampling interpretation follows the decoded DDS declaration here. Historical
original TPF SRGB/view policy remains a separate compatibility question, not
silently certified by byte equality. GPU resources and descriptors stay in native
records. Engine CPU metadata, wrapper size and destructor semantics are preserved
per family. Known original TPF GPU factory AC830 refuses any unowned caller.
