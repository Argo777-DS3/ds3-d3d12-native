# Existing renderer reuse

Historical technical record. Current execution and next action: [STATE](../../.project/STATE.md).
D-0156 supersedes the earlier continuation schedules; proof limits remain unchanged.

Thomas explicitly reaffirmed reuse of the repository renderer on 2026-10-07.
The native startup owns installed delivery and lifecycle; existing rendering
algorithms remain the implementation base. Historical qualification does not
by itself qualify their delivery from this new process entry.

| Existing work | Reuse in I-0066 | Remaining work |
|---|---|---|
| `native/renderer/backend`: DeviceQueues, FrameRecorder, FramePresenter | Used through the existing SceneRenderer directly for actual DS3 startup, loading view, clear frames, worker uploads and close (native13) | Real scene submission, resize and normal shutdown |
| `native/frame/engine_window` | Used to acquire the actual engine window | Resize/message lifecycle |
| `native/content/texture_container`, `dds_resource` | Reused unchanged for genuine TPF/DDS decoding at the installed CPU texture boundary | 146 live startup textures including one cube qualified; subsequent material/primitive consumers remain |
| `native/content/gpu_texture_packages` | Its existing upload recording/view logic extracted into shared `gpu_texture_upload`; both package bank and new installed ingress call it | Live residency and subsequent native consumers |
| Shader bank/interface, material programmes/admission/resources | Preserve original bytecode and reuse these interfaces for native pipeline/material admission | Connect live CPU wrappers to the owned shader/material records |
| Scene work/owner, scene renderer, primitive work/pass | Reuse for semantic scene composition and native draws | Genuine zone geometry, instances, camera and frame requests from the running game |
| `native/world` qualified pass producers, skinning, lighting, post and UI | Reuse algorithms and CPU contracts as applicable; original GPU capture remains a diagnostic reference | Remove dependencies on captured original GPU handles/content at each required ingress |

The small new texture registry supplies worker-safe ownership and engine-address
incarnations. The earlier package bank has a single renderer-thread owner and
package ticket lifetime; those contracts are not interchangeable. They now
share the GPU implementation while retaining their distinct producer lifetimes.

Original frame comparisons, captures and old D3D11 objects remain reference
evidence. No D3D11 call forwarding or original GPU resource is admitted by this
native entry. A native clear and shader bytecode admission are not zone rendering.

Native18 additionally reuses `native/content/gparam` unchanged on actual installed-engine worker bytes:8 files, typed groups/parameters/IDs/values. Its own registry retains bytes and keys; actual engine fallback allocation/CPU destruction stay paired. No GX parameter handle is supplied.
