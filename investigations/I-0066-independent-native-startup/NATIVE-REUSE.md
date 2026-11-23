# Existing native renderer inputs

Checkpoint696: `native/renderer/gbuffer_encoder.h` now extracts the existing
native clear, target/root/descriptor/geometry binding and indexed draw commands.
Both `live_host::GBufferPass` and `SceneRenderer` are actual clients. Their PSO,
descriptor allocation, producer coverage, transitions and lifetime owners remain
at the caller; the extracted command encoder accepts resolved D3D12 views, not
Capture/Join/resource numbers. This is bounded shared command encoding, not a
complete GBuffer pass or direct selected-view implementation. GPU controls695
exercise six MRTs, stencil/depth rejection and nonzero index/vertex/instance
offsets on WARP and hardware with completion/debug checks. No game ran.
[Qualification695](../../evidence/I-0066/qualification-gbuffer-695.json).

This inventory preserves components and their bounded qualification. Current
execution follows [STATE](../../.project/STATE.md) and D-0156 in the
[mandate](../../.project/NATIVE-D3D12-RESET-MANDATE.md): restart the complete direct
connection design after primary verification of [ER716](../../evidence/I-0066/ER-ENGINE-D3D12-DEEP-STUDY-716.md).
The old connection sequence below is historical; retaining a component does not
retain its host/capture/virtual-file interface as the production contract.
| Responsibility | Existing implementation | Startup connection / remaining dependency |
|---|---|---|
| Camera constants | `native/world/producers.cpp`, `ds3prod::ProduceSceneCamera` (I-0042) | `scene_ingress::ProduceCameraConstants` calls this producer on genuine CED670 camera output and native viewport. Only `scene_camera_fields` words are owned; other cbSceneParam words remain unwritten. |
| Instance/model, matrix palette, material and texture-tiling constants | `native/world/producers.h/.cpp`; `live_host.cpp` `gprod::AllProducers` | Supply actual engine CPU instance, group, animation, state and material snapshots. Preserve each producer's written-word mask. No former GX constant upload or oracle block required for these owned fields. |
| Lighting/shadow/forward/effect/post constants | `native/world/light_producers.*`, `effect_producers.*`, `live_host.cpp::ProduceOther` | Supply current engine writer inputs and owned game banks directly; retain algorithms, masks and coverage/refusals. Adapt their entry contracts rather than reproduce captured blocks. |
| GBuffer, lighting, subsequent passes, temporal state and presentation | `native/world/live_host.cpp`: `GBufferPass`, `Executor`, `Stages`, `RunFrame` | The host currently joins CPU frame submissions with development capture events and resource tables. Replace that input join with native scene/pass intent and native resource identities. Keep the existing D3D12 encoders. Its capture-derived defaults/initial temporal inputs are not production ownership. |
| CPU mesh tables and loaded model bytes | `native/world/world_adapter.cpp::Resolve`, `native/content/acquire.cpp::CopyGroup` | `game_scene_geometry` is a bounded ingress adaptation of these layouts for the real native storage wrapper. It adds bounds/lifetime checks; it does not create a renderer or qualify scene selection. Native92 joined 65 actual models, no scene draw. |
| UI primitive shader/geometry, textures, masks and blends | `native/renderer/primitive_pass.*`, `native/frame/primitive_work.*` and the current game UI owners | Continue using their actual native consumer. Filter cache/push/pop are CPU scope policy; offscreen FilterPrimitive targets/effects remain unavailable and must not be claimed ready. |

The small `native/frame/scene_work` / `native/renderer/scene_pass` material
envelope also exists. It is bounded to 128 draws and is not evidence that the
complete native pass integration exists. Reuse the full pass algorithms as
integrated backend components; do not build another renderer in this envelope.

Next coherent renderer block: current scene selection and genuine CPU
instance/material inputs -> existing producers -> existing native pass
encoders. The startup UI continuation is still a load-path prerequisite.
No independently started native zone has been rendered yet.

Checkpoint634: startup now preserves four actual decal CPU requests through
the genuine51BF0 allocator/object/query transaction (Native96-99), with real
generation handles and retained CPU descriptor/prepared results. These are
CPU inputs only: borrowed descriptor pointers are not native resource IDs;
no generated geometry or draw is claimed. Existing host decal encoding remains
the downstream consumer to connect, with19B690/198910 explicitly refused.
Real FXR construction/parsing and empty duplicate retirement are connected
(Native98); loaded retirement and SFX rendering remain unqualified. Real MTD
default selection and empty GPARAM admission are source/live qualified.

Native99's new boundary is CPU environment-map region registration through
the absent original renderer+42B0. Existing `native/world/light_producers.cpp`
`Banks::Env` already constructs native IBL region records from MSB inputs;
reuse it and the existing IBL/pass consumers. Preserve genuineD0B8A0's upstream
region calculation while adapting registration/update/retirement ownership.

Historical advice647 proposed the host typed memory facade as a migration seam.
Review686 explicitly narrows that advice: retain useful types, leases and native
algorithms, but do not adopt `host_input`/`LiveFrame`/`Capture` as the production
interface. Memory publication alone does not eliminate capture semantics. The
earlier advice and implementation evidence remain preserved in
[review647](../../evidence/I-0066/ASTRA-HOST-INPUT-647.json); its imperative to
expose the host facade as the integration seam is removed.

Checkpoint660 implements the first concrete source connection: the existing
host `io` facade is shared in `native/world/host_input.h`; the standalone host
still compiles. Startup marks its source root memory-only, publishes genuine
GPARAM bytes under the native short-name file keys, and uses the existing
`Banks::Load` parser through virtual byte/list readers. Real loaded executable
constants supply `Rdata`; no captured bank or output constants are seeded.
Immutable shared bank snapshots retain inputs across engine record withdrawal.
Native102 recognizes19 genuine bank files and preserves D26AB0's area list for
the observed generation7/area140000000/bank4/weight1 binding. This is an actual
producer-input connection, not a scene draw. The complete renderer core,
native frame/submit correspondences, pass intent, persistent state, shared
startup device and resource/fence lifecycle remain to connect. [Proof659](../../evidence/I-0066/qualification-host-input-659.json).

## Direct integration disposition — review686 / D-0155

The current connection is a temporary CPU migration, not the target architecture.
The primary code confirms that changing capture storage to memory leaves
`Join`, captured stream/state/resource identities, `Composer`'s template A,
captured initial state and standalone device/presentation ownership intact.
GPARAM and camera publication are authentic inputs, but no current scene frame
consumes the GPARAM bindings. [Review686](../../evidence/I-0066/ASTRA-ARCHITECTURE-686.json)
records the source checks, accepted interpretation and qualification limits.

| Integrated backend responsibility | Current native work to retain | Temporary machinery to remove from production dependency |
|---|---|---|
| Resource admission, updates and withdrawal | Genuine content parsers, shader/material/texture/geometry owners, identities, generations and retained references | Capture resource tables, original GPU identities and virtual-file transport |
| Current views and parameters | Actual camera/instance/material/GPARAM/region data; existing producers and written masks; native targets and views | Capture A template, comparisons and captured MatDyn/temporal/particle/post initialization |
| Ordered render work and completion | Existing D3D12 pass algorithms/PSOs/encoders; startup device/queue/fences and engine-window presentation | Join/JoinOther, captured command streams, standalone host device/window and diagnostic frame mapping |

Elden Ring's actual recovered path prepares engine context before native drawing
(`01C400A0 -> 01E8E0D0`), manages targets and publishes multiple views
(`01AFA720/01AFA4A0`), finalizes recorded work (`01ACB5E0`) and preserves
submission/callback order (`01AECB00`). This supports the three responsibilities
above rather than a fabricated complete capture packet. ER offsets, ABIs, lot
limits and unclosed GPU lifetimes are not DS3 evidence.

The next substantial block is one actual engine view consumed by the existing
encoders on startup-owned D3D12 services. Extract the necessary consumers from
the host into callable backend components; keep the host as a diagnostic client
of those components. Supply current scene selection, instances/materials,
GPARAM/regions and native resource identities directly. Own the corresponding
targets/views and retain each consumed input/resource through completion.
Activate only passes whose native initialization is defined. Additional isolated
`io` publication does not fulfill this block.

The built SFX681 unavailable-result prototype remains temporary and unexecuted;
its latest deferred request is diagnostic input, not a complete request queue,
native effect factory or simulation. Preserve it and the genuine CPU sources
without presenting it as a final SFX replacement.

## Preserved direct owners and ER evidence — checkpoints723/728

Genuine selected-view CPU delivery700 and native pose/physical stream owners712
replace named old GPU responsibilities, preserving current engine CPU inputs.
Shared native FLVER upload was extracted for physical streams; startup device/
completion remain common. Native111 crosses125-row pose and seven-stream
initialization, then836D32 FaceGen texture allocation; updates/selected draw not
qualified. [RCA712](../../evidence/I-0066/RCA-MODEL-STREAMS-712.md),
[proof714](../../evidence/I-0066/qualification-streams-714.json).

The actual ER entry is C400A0 tail-jump ACAF20 before preparation/native indexed
emission; the earlier abbreviated arrow is not the actual machine call edge.
[Study710](../../evidence/I-0066/ER-ENGINE-D3D12-METHOD-710.md) now supplies retained
actual ER source/provenance for current associations/descriptors/commands,
target withdrawal and submission. Thomas finds that first study insufficiently
exhaustive and explicitly requests further producer/dynamic-buffer/lifetime
chains from Astra Medium; study716 is delivered and its decisive nominal branches checked in primary receipt730. Optional discriminating offline ER live
runs are authorized by Thomas, with announced protocol and verified restore.
This work does not transfer ER layouts into DS3 or qualify a native DS3 zone.
External [manifest723](../../evidence/I-0066/native-startup-manifest-723.json), SHA256 5A1F6197F7887D83300F2E5363358B6B66FD987A63F0FB56AEE404D8FA038AB7. It pins current native sources, source exports, bounded proofs and restored live runs. Connect the Evidence Store before using these links; never restore removed historical evidence automatically.

Current primary design: [DIRECT-CONNECTION](DIRECT-CONNECTION.md), restarted after bounded source verification730 of ER716. Existing owners remain candidates until their complete DS3 contracts are closed.
Preservation [manifest728](../../evidence/I-0066/native-startup-manifest-728.json), SHA256 F046AD18F66E62213DDA4D9C2FE63EFAC2261677E25FE8EF1392669FCF0D04E5. Candidate725 is preserved pending the required methodological restart.
