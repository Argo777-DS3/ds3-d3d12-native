## C4 — Temporal systems and native high-framerate behavior

**Useful because:** rendering above 60 FPS is not sufficient if simulation, animation, camera, input, or effects remain incorrectly coupled.

**Depends on:** C1; timing observability; relevant executable control points.

**Principal unknowns:** timestep sources; fixed/variable update boundaries; frame-dependent logic; animation and effect cadence; online or physics constraints.

**Acquired when:** 30/60/120 FPS behavior can be compared deterministically and the required systems operate correctly beyond 60 FPS without hidden cadence substitution.

Thomas reaffirmed on 2026-09-19 (D-0011) that the objective is to resolve all
defects caused by unlocked high framerate, not just sprint and roll. Those are
causal witnesses and regression cases, not an exhaustive system list. Before
claiming C4 complete, explicitly cover the relevant temporal systems (movement,
animation, simulation/collision, input, camera and effects), distinguish proved
invariance from untested behavior, and resolve discovered independent causes.
This does not automatically open 240/unlimited, broad speculative hooks or
distribution: the current causal validation order and scope gates still apply.

The accepted temporal corrections remain separate canonical components under
D-0027: **LTF — Locomotion Timing Fix** is the I-0007 sprint/roll locomotion
correction, while **TRTF — Terrain Response Timing Fix** is the I-0013
terrain-response recurrence correction. A future shared user control may
orchestrate both, but does not combine their internal identities, guards,
evidence or validation boundaries.

I-0015 now reconstructs the surrounding engine architecture under D-0029.
K-0065 confirms the first reusable character-runtime spine from
`WorldChrManImp` through `ChrIns`, its module aggregate and PlayerCtrl/EnemyCtrl.
K-0066/K-0067 cross into SPRJ/FD4 scheduling, and K-0068 closes the adjacent
engine-wide timeline: 97 exact named groups span core managers through
character/physics/world/UI/audio/camera/LOD/graphics and frame lifecycle.
K-0069 then binds typed `GameMan`, `TaskItemAdapter`, in-game steps and network
steps to seven exact named groups. K-0070 crosses that boundary into an
`InGameStep`-owned world topology shared by peer area/block managers and a
separate three-phase `SprjFD4LocationStep` client. K-0071 closes the topology
producer into exact `WorldAreaInfo`, `WorldBlockInfo`, `WorldAreaRes` and
`WorldBlockRes` arrays while explicitly leaving disk I/O and complete streaming
semantics unproved. The next selected frontier is one typed downstream
activation/lifetime edge from those records into a world manager. K-0072 closes
that edge for exact `WorldObjManImp`: object records are constructed from info
descriptors and destroyed before the owner topology is reset. With the world
branch now yielding diminishing global information, the next tranche uses the
same exact `InGameStep` construction/teardown pair to recover a typed service
composition boundary across adjacent modules. This cartography strengthens
future C4 and remaster work but does not itself prove another temporal invariant
or complete C4.

K-0073 begins that continuation with four exact paired service lifetimes under
`InGameStep`: `MapItemManImpl`, `MiniMapImp`, `SprjObjImp` and `WindWorldImp`.
K-0074 selects `MiniMapImp` and binds a narrow typed chain from world/location,
exact `WorldBlockRes` mode-zero teardown, `MoveMapStep` and current-player state
into exact `MiniMapBlock`/`MiniMapLayer` materialization. Additional
resource-layout activation/transitions remain `SUPPORTED`. The smaller `SprjObjImp` call
surface is retained as negative evidence because its two examined direct
callbacks are return stubs. The next step leaves this bounded branch for the
adjacent exact service or timeline client with the best engine-wide gain, not
parallel exhaustive expansion or renderer redesign.

K-0075 then binds exact `ChrWindSlot` from the character attachment system to
typed `WindRegion`/`WindIns` construction and exact `WindWorldImp` registry
lifetime. Exact `MoveMapStep` separately dispatches a tagged float to the wind
service, while the unresolved final jump prevents an update-formula or
subsystem-ownership claim. K-0076 consolidates the character, scheduler, game
flow, world topology/object, MiniMap and wind boundaries into one canonical
eight-role engine map while preserving LTF, I-0008 and TRTF as independent
mechanisms. The next bounded frontier classifies exact `MapItemManImpl`
clients before admitting at most one typed producer/consumer edge. K-0077
closes that classification across all 43 exact operands/33 functions and binds
`EventObjEntity` state/rematerialization through `MapItemManImpl` to exact
`WorldObjManImp` input. Map presentation semantics remain unknown. The next
frontier selected one peer world-manager lifetime or a typed location work
item. Cycle 18 rejects the peer lifetime promotion after an exact census finds
only shared `FieldArea` teardown writes, then closes K-0078 with two exact
reference-counted Location viewer classes and their slot-8 scheduled-consumer
match. Protected registration keeps the final second-root routing SUPPORTED.
The next frontier seeks one typed world-resource-to-preparation boundary
without beginning renderer redesign. Cycle 19 closes K-0079 on the exact
mode-zero half: one `WorldBlockInfo` index fans removal into exact per-block
`WorldChrManImp`, `WorldObjManImp` and `MiniMapImp` representations. The inverse
preparation fan-out is structurally matched but remains SUPPORTED without a
typed direct caller. The last planned cycle tests only the area-level
aggregation edge before consolidating the tranche. Cycle 20 closes K-0080:
exact `WorldAreaRes` destruction emits a separate area-index transition and
does not iterate child block resources in that path; all twelve examined
world-manager area targets are return stubs. Thomas opened a second 20-cycle
budget, with a cumulative 40-cycle ceiling. Cycle 21 closes K-0081: the six
static timeline relation masks reach a lower-level six-cell-per-group matrix
and are consumed by graph-state/callback branches. This proves a genuine
architectural relation consumer but not lane names, dependency direction or
execution order. Cycle 22's read-side discriminator leaves the lane semantics
UNKNOWN. Cycle 23 moves to a typed adjacent subsystem: K-0082 closes
selector-one `PadManipulator` creation and paired ownership under
`PlayerIns+0x58` while keeping `SprjPad`, `PadStep` and hardware sampling
unjoined. Cycle 24 closes K-0083's exact physics/control-to-manipulator
state handoff; cycle 25's upstream Pad path remains untyped/protected.
Cycles 26--28 establish K-0084: exact `SprjCameraStep` registration in
named group `0x43 CameraStep`, while `SprjCameraImp` and `CameraMan` have
separate construction/publication paths. No typed step-to-service join,
camera timing or renderer role is promoted. The bounded next test seeks
one exact camera-to-character/world-state read. Cycle 29 closes K-0085:
`SprjCameraImp` child data reaches a current-`PlayerIns` context request
through `WorldChrManImp+0x80`, without a proved player-state effect or
scheduled-camera caller. This bounded read is sufficient for now; pivot to
another adjacent subsystem with greater structural gain.
Cycle 30 maps exact `SprjFileStep`/`SprjResStep` registration in separate
named groups `0x03`/`0x04`; FileStep retains an exact
`FD4FailoverAllocator` pointer and an embedded update task. Protected
callbacks prevent a file-to-resource request/streaming claim. The next
bounded frontier tests one typed `ResMan` or world-resource owner bridge.
Cycle 31 distinguishes exact `ResMan`, `ResManagerImp` and
`SprjEmkResManImp`; `ResMan` registers an embedded task to group `0x0D`.
Exact `WorldAreaRes` destruction supplies its area index to
`SprjEmkResManImp`, which releases a paired indexed handle record.
Inverse preparation lacks a typed direct caller and remains SUPPORTED;
FileStep/ResStep-to-manager routing and streaming behavior are UNKNOWN.
Cycle 32 distinguishes exact `SoundStep` game-flow and `SprjSoundStep`
FD4 schedule clients in named groups `0x17` and `0x36`, then maps
`WorldSoundManImp` as a separate world-partition consumer. Their
execution/ownership joins remain UNKNOWN. The canonical map has 16
bounded roles, with no live-game access or timing-correction change.
Cycle 33 closes a distinct typed world-area sound-resource edge:
`WorldAreaRes` destruction reaches `SprjFD4MobankRepositoryImp` record
release through separately published sound services. The passed
`SprjSoundImp` pointer is unused by the release routine and no scheduled
sound-step caller is proved. The map has 17 bounded roles.
Cycle 34 confirms exact `FrpgNetManImp` publication and two distinct
scheduled forms: embedded `FrpgNetMan` task in `0x0C` and held
`FrpgNetSysStep` in `0x4A`. Connection steps in `0x4C/0x4D` remain
separate; protocol and group order are UNKNOWN. The map has 18 roles.
Cycle 35 closes a paired `FieldArea`-owned `SprjEventManImp` lifetime and
one exact `SprjEventDropItemInfo`-to-`EcTestEventRegionDrop` condition edge.
The separately constructed Lua proxy remains unjoined. The map has 19 roles.
Cycle 36 finds a bounded coordinator reading `SprjEventDeadResetState` and
the separate Lua root/proxy, then conditionally writing state `2`; direct
ownership, callback routing and online semantics remain UNKNOWN. The map
still has 19 roles.
Cycles 37--40 complete the cumulative 40-cycle I-0015 budget. Exact
`FrpgHavokManImp` registers seven physics/cloth tasks; its fields feed
separate exact `SprjBehaviorImp` and, through an untyped wrapper, exact
`SprjHkAiWorld`. Two AI-world tasks enter named groups; exact
`SprjHkAiSection` optionally owns `SprjHkAiNaviMesh` and enters a keyed
AI-world collection. The map closes at 23 bounded roles. I-0016 is the
direct continuation, beginning from this canonical map and prioritizing
the section/mesh resource producer before other adjacent subsystems.
I-0016 cycle 1/20 closes K-0097: exact `WorldAreaRes` removal directly
indexes the untyped fixed AI-area aggregate whose records and entries feed
exact section/optional mesh construction. The aggregate/entry type, handle
payload and exact NVM-repository join remain UNKNOWN after a complete bounded
direct-operand comparison. Cycle 2 pivots to a newly identified typed
file/resource request or completion edge. Cycle 2/20 preserves the direct
FileStep/ResStep-to-manager join as UNKNOWN, then closes K-0098's exact
`ResManagerImp +0x128` keyed binding into six exact resource repositories.
Physical I/O and completion semantics are not claimed. Cycle 3 tests the
event/script coordinator only from a typed caller or registration edge.
Renderer redesign and DX12/Vulkan remain unopened.

Cycle 3/20 classifies all three direct references to event/Lua coordinator
`0x00DE5A40` as exception/unwind metadata and finds no direct rel32 caller
in executable sections. Typed owner and schedule remain UNKNOWN; the next
bounded test seeks a direct consumer of exact `SprjEvdRepositoryImp` without
inferring an event link from its name.
Cycle 4/20 closes K-0099: exact `SprjEmkResManImp` owns the EVD/EDF/ELD
repository global lifetimes, while exact `SprjEvdFileCap` obtains a keyed
repository result with an exact `SprjEvdResCap` creation path and a paired
file-cap cleanup. Event/world ownership and scheduled file/resource
activation remain UNKNOWN. The next test starts at one typed file-cap
construction/caller edge.
Cycle 5/20 closes K-0100: exact `SprjEvdFileCap` is submitted with an EVD
name to exact `SprjFileImp` through its cap-selection slot, with one caller
retaining the returned handle. This is not a FileStep/ResStep schedule,
physical I/O or event-manager handoff. The next bounded test follows one
selected-cap/step edge toward a typed request or completion.
Cycle 6/20 finds exact `SprjFileStep` construction, organizer registration
and indexed state dispatch but no typed join from the selected EVD file
capability into that scheduled step or a completion path. K-0100 remains
bounded; the next test pivots to a direct EVD resource-cap consumer or
payload transfer toward event/world ownership.
Cycle 7/20 closes K-0101: exact `SprjEvdResCap` composes indexed EVD and
paired EDF/ELD repository results in a subordinate record. The EVD lookup
crosses protected code, and no typed event/world consumer or readiness
is established. The next test seeks a direct consumer of this composite
before pivoting to another ranked frontier.
Cycle 8/20 closes I-0016 as a bounded success: five new typed boundaries
K-0097--K-0101 and a 27-role map are validated. Exact EVD ResCap
teardown releases its subordinate record and indexed EVD result array,
but protected code blocks EDF/ELD teardown and no typed event/world
consumer is recovered. The next ranked frontiers are AI input origin
from a new typed producer, sound-step-to-service, then camera/physics
effects from direct typed edges. C4 itself is not acquired by this
static architecture work. I-0016 is complete and integrated into `main`
after Thomas's authorization; any continuation is a separate workstream.

I-0017 closes at 4/20 with K-0102--K-0104 and a 30-role cumulative map.
The AI section handle has an exact cleanup-side edge to `SprjFileImp`, but
its typed source and NVM/world-resource input origin remain UNKNOWN. Exact
game-flow `SoundStep` callbacks reach the Mobank repository, while distinct
`SprjSoundStep` remains unjoined. Exact `FrpgHavokManImp` task callbacks
consume conditionally published exact `SprjClothImp`; physical effects and
execution order are UNKNOWN. The selected `SprjCameraStep` methods provide
no new typed service join. Further work should start only from a new typed
producer/consumer entry, including a graphics entry if one appears
naturally; this static work does not acquire C4. I-0017 is integrated into
`main` after Thomas's authorization; its source branch is preserved.

I-0018 closes at 4/20 with K-0105--K-0107 and a 33-role cumulative map.
Exact `SprjGraphicsStep` in group `0x51` conditionally publishes distinct
`SprjGraphicsImp` and `RendManImp`; exact `SprjDrawStep` in group `0x58`
publishes `SprjAdhocDrawImp` and `CSFakeLoadingScreenImp`. A draw-side
function reads these globals and an exact `RendManImp` method calls two
`SprjGraphicsImp` methods. Its DrawStep callback binding, GX/backend transfer,
renderer ownership, pass/resource model and camera/world/material inputs
remain UNKNOWN. A successor should test one natural typed transfer into GX
or into these services, not start from API calls. I-0018 is complete and
integrated into `main`; this static work does not acquire C4. Thomas authorized
I-0019 as a separate 20-cycle deep graphics/renderer reconstruction from clean
`main`: establish a typed GX bridge, then follow natural producer, draw/submission
and backend relationships through multiple layers. One GX edge alone is not
sufficient for closure; keep unknowns explicit and do not implement RHI or DX12.

I-0019 closes at 8/20 and is integrated into `main` with K-0108--K-0111.
An exact GraphicsStep callback through `SprjGraphicsImp` constructs and
publishes `GXRenderingSystem`; typed graphics children include a Flver draw
system whose keyed `TexRepositoryImp` results feed four GX fields. A separate
RTTI-backed `GXDrawSceneForTask` carries a scene pointer/range into a lower
context queue and calls a scene virtual method. Another GraphicsStep callback
initializes that lower context, which owns exact `GXRenderThread` and an
embedded D3D11 backend object, joining I-0002 from the top. The scene owner,
queue drain, direct GX system-to-task join, camera/world/model producers,
payload semantics and Present transfer remain UNKNOWN. The future RHI seam
is only a candidate across GX preparation/task submission and the lower
context/backend. Thomas authorized I-0020 as a separate 20-cycle maximum
static workstream from clean integrated `main`. Its central questions are the
exact owner/producer of the scene consumed by `GXDrawSceneForTask` and the
actual lower-context queue drain/task execution path toward `GXRenderThread`
and D3D11. Trace one typed world/model/camera producer if evidence permits;
refine the future RHI seam only from proved transfers. Do not infer a camera
edge, GPU command queue, draw call, or direct GX-system/task join from names
or proximity. Escalate only a precise, statically blocked discriminator to
Thomas for a minimal manual runtime protocol; do not implement renderer code.
  I-0020 has its own validation, checkpoint, and integration authorization gate.

I-0020 closes at 14/20 on preserved source branch
`codex/i-0020-scene-ownership-render-submission` with K-0112--K-0115; Thomas
authorized its fast-forward integration into `main` at `a3c1aad`.
`NS_SPRJ::StageRend`
constructs exact `GXSR::GXSgCompositeScene`, binds the `GXRenderingSystem`
global and a manager obtained from that system, and transfers typed
`SprjCameraImp`/`SprjPersCam` data into the scene. `GXInternalForTask` wrappers
are drained from the lower-context CPU queue `+0x2C8`; the composite-scene
render bridge consolidates manager records, conditionally writes frame-slot
commands, and posts that slot to `GXRenderThread`, where processing reaches a
COM call. The exact final COM pointer alias to D3D11 immediate context,
individual record-to-Present transfer, record payload, mesh/material semantics,
runtime order and camera temporal correctness remain UNKNOWN. A future RHI
contract needs those discriminators; no renderer code or live test was made.
Thomas authorized the distinct I-0021 Work conversation from clean integrated
`main`: **Backend Command Contract & DX11 Boundary Reconstruction**. Start from
frame slot `+0x8` COM identity, origin, ownership and lifetime, then trace a
small number of representative record families into concrete backend/API
operations. Determine the narrowest evidence-backed DX11 boundary and whether
a usable internal backend seam already exists. Present is secondary. Do not
launch DS3 without a specific static ambiguity and minimal test protocol; do
not implement DX12, Vulkan, a speculative RHI or renderer changes.

I-0021 closes at 8/20 and is integrated into `main` from preserved branch
`codex/i-0021-backend-command-contract` with K-0116--K-0119. The supposed
frame-slot `+0x8` COM field is refuted: it is a descriptor-vector end. A
distinct GX intermediate `+0x8` aliases the backend cell containing the
`ID3D11DeviceContext*` returned by `D3D11CreateDevice`. Two conditional
families reach `FinishCommandList`/`ExecuteCommandList` and
`OMSetRenderTargets`. GX preparation/slots/thread are established, but this
bounded crossing already carries D3D11 objects and does not prove a neutral
RHI. Complete manager payloads, view ownership, other record families,
runtime order and individual record-to-Present remain UNKNOWN. No renderer
change or live game action occurred. Thomas authorized its integration;
C4 remains open.

Thomas authorized [I-0022](../investigations/I-0022-d3d11-surface-backend-seam/CONTRACT.md)
from the clean integrated I-0021 checkpoint as a distinct Work conversation.
Its C2 question is the minimum evidenced D3D11 dependency surface and whether
a narrow backend seam exists or must be reconstructed. The bounded inventory
covers representative API responsibilities, resource coupling, deferred
contexts, command forms, centralization and minimum replacement obligations.
It starts with 20 meaningful cycles. Present is secondary; no DX12 backend,
renderer modification or unexpected game launch is authorized. The detailed
scope, exit criteria and epistemic limits live in its contract.

I-0022 closes at 12/20 on preserved source branch
`codex/i-0022-d3d11-surface-backend-seam` with K-0120–K-0123 and is
integrated into `main` after Thomas's authorization. The sampled
`0x14192…` wrappers span device creation, contexts, resources, pipeline,
draw/dispatch, transfer, query and presentation responsibilities, while
resource/view handles and type-0 frame records already expose D3D11 objects.
The backend Present route has an additional direct-context state sequence.
Thin wrappers are a feasible starting point for a reconstructed seam, but a
backend-neutral RHI is not established; view lifecycle, other callbacks and
direct COM islands block a correctness specification for a first DX12 backend.
No game run, patch or DX12 work occurred.

Thomas opened [I-0023](../investigations/I-0023-texture-view-lifetime-boundary/CONTRACT.md)
from the exact clean I-0022 integration commit `c615bec` as a distinct C2
workstream. Its [result](../investigations/I-0023-texture-view-lifetime-boundary/RESULT.md)
closes at 7/20 static cycles: one Texture2D → RTV/SRV → typed volume-renderer
pass → D3D11 use → local destruction chain, one distinct type-1 state callback,
and a bounded direct-island comparison. B2 is supported for that pass; C has
the previously established presentation island; D and global island coverage
remain UNKNOWN. A native DX12 backend still cannot be specified without
guesses. No implementation or game launch occurred. Thomas subsequently
authorized integration; I-0023 is in `main` at the clean I-0024 base
`de1e28e`, with its source branch preserved.

Thomas opened [I-0024](../investigations/I-0024-cross-pass-graphics-systems/CONTRACT.md)
as a major C2 cartography workstream from that clean base. Its
[result](../investigations/I-0024-cross-pass-graphics-systems/RESULT.md)
closes at 34/40 meaningful static cycles. Thomas authorized its fast-forward
integration into `main` at `e8b33c3`; the source branch remains preserved.
K-0128–K-0134 add five RTTI-backed scene families, their shared descriptor
stream and guarded transfer to `GXDrawSceneManager`, a second texture/RTV/SRV
owner under post-processing, typed resource repositories/input layouts,
Flver record ownership, and a type-1 compute-dispatch callback. The precise
scene-entry-to-frame-record translation, shader/material binds and frame order
remain UNKNOWN. DX12 is not implemented or specified; no game launch occurred.
The recommended next focused boundary is descriptor stream → frame record →
backend consumer, subject to Thomas's direction.

Thomas opened [I-0025](../investigations/I-0025-render-submission-material-draw/CONTRACT.md)
from clean integrated `main` `3d53da4`. Its [result](../investigations/I-0025-render-submission-material-draw/RESULT.md)
closes at 32/40 meaningful static cycles. Thomas authorized its fast-forward
integration into `main` at `86552cf`; the source branch remains preserved.
K-0135–K-0139 replace the generic entry→record unknown with a
bounded command-block/runner/group contract and one typed feedback/base
scene→type-0→`ExecuteCommandList` path. Repository records reach PS/VS/layout
binds and `Draw(4,0)`; Flver-associated texture keys/records reach PS or CS SRV
binds and one bounded four-vertex draw path. A FLVER mesh draw, exact block
lifetime, universal pass order and native DX12 specification remain UNKNOWN.
The next static discriminator is a mesh's geometry/material→draw path and its
scene record, if Thomas selects that scope. No game launch or implementation
occurred.

Thomas opened [I-0026](../investigations/I-0026-flver-geometry-command-lifetime/CONTRACT.md)
from clean integrated `main` `cea8629` as a multi-stage C2 renderer programme.
Its [final result](../investigations/I-0026-flver-geometry-command-lifetime/RESULT.md)
is closed at **180/180** by Thomas's decision, with K-0140–K-0265 and the technical strong exit explicitly
unmet. The static proof covers conditional FLVER indexed draws in ForLightMap,
MultiView and Default contexts, the geometry binds, command carrier, shared
linear arena, and guarded reset. A constructed `SprjFD4FlverModelData` reaches
the **same invocation's** Default IB/VB selection, material entry from a typed
factory, five shader-stage binds, some state commits and an indexed draw under
their branch guards. The draw record's runtime dispatch, actual shader/PSO
values, full callback coverage and embedded-pointer lifetime remain UNKNOWN.
[Default draw](../evidence/I-0026/static/SPRJ-FLVERMODEL-DEFAULT-INDEXED-DRAW.md),
[material join](../evidence/I-0026/static/SPRJ-MATERIAL-FACTORY-TO-DEFAULT-ENTRY.md),
[command arena](../evidence/I-0026/static/COMMAND-ARENA-SERVICE.md).

The frame-slot worker has a conditional CPU acknowledgment after polling
`TIMESTAMP_DISJOINT` and reading paired `TIMESTAMP` queries; the commands
covered by those measurements and global GPU retirement are UNKNOWN. A typed
`SprjDrawStep` slot can advance the lower-context epoch, and another can reach
the reset comparison, but its activation and inter-invocation order are not
proved. A keyed registration bucket keeps the DrawStep pointer; the lower
task graph uses the same lookup helper on an unaliased incoming tree. Two
typed immediate-context `Flush` callsites are asynchronous and do not close
the GPU lifetime rule. [Queries](../evidence/I-0026/static/GPU-TIMESTAMP-QUERY-RECORD-JOIN.md),
[DrawStep epoch/reset](../evidence/I-0026/static/DRAWSTEP-EPOCH-TO-RESET-GUARD.md),
[bucket/graph limit](../evidence/I-0026/static/DRAWSTEP-RECORD-CALLBACK-CONSUMERS.md),
[Flush](../evidence/I-0026/static/D3D11-FLUSH-PRESENTATION-PATH.md).

The frame-slot synchronization now resolves to three manual-reset Windows
events. The handoff waits before publishing a slot; the worker signals after
slot consumption and after its optional callback. A `0x7530` internal wait
argument becomes **30 ms** at `WaitForSingleObject`, correcting an earlier
30-second interpretation. These are CPU milestones; the relation to the next
FLVER arena reset and GPU retirement remains UNKNOWN.
[Event construction and wait](../evidence/I-0026/static/FRAME-SLOT-SYNC-READINESS-INVENTORY.md).

Two sequential query-helper callers now bracket a conditional type-0
`ExecuteCommandList` replay. Their Begin/End helpers select the same query
handle field if the pool pointer and index stay stable across the intervening
calls. The inspected immediate bodies contain no demonstrated direct writer
to those fields, but slot aliases and indirect callbacks remain open; neither
FLVER coverage nor a wait before arena reuse follows. [Interval and write audit](../evidence/I-0026/static/QUERY-END-HELPER-AND-CALLER-ORDER.md).

The SPRJ FLVER resource factory overrides its base null-return callback
`+0x48` with a global lookup. The node payload's `+0x28` can populate the
Default material's first shader-provider field, now typed as
`GXFlverShaderGroup` on the guarded route. Its slot produces a five-position
VS/PS/HS/DS/GS tuple from guarded tables; the key comes from the current
model record, and source adapters populate the tables through further virtual
calls. Runtime tuple values, concrete shader handles and source-object
lifetime remain UNKNOWN. [Provider chain](../evidence/I-0026/static/GXFLVER-MATERIAL-SHADER-BIND-CANDIDATE.md).

The [mandatory native-DX12 readiness assessment and Astra High audit](../evidence/I-0026/static/NATIVE-DX12-READINESS.md)
remain **B** for an integrated representative prototype. Before one is claimed,
the selected FLVER path needs a defensible last CPU read and capture before
arena reuse, coverage of resource mutations and callbacks, and an effective
runtime draw/pipeline-state contract. The new backend can define its own GPU
fences for resources it fully owns; recovering a historical D3D11 retirement
rule is not a universal prerequisite. Static VS and PS creation-to-bind field
chains are proven under guarded tuple selection, including one mode-0 kind-4
PS table route, but no runtime shader pair or PSO is selected by this proof.
Possible future discriminators are a minimal last-read table and backward
pipeline-state slice from existing exports, followed by a bounded announced
runtime witness only if static identification stalls. The work is static-only.
Thomas authorized this evidence-boundary closure, now integrated into `main`;
he retains the prototype-versus-decompilation decision. No DX12 implementation
or game launch has been authorized.

Thomas then opened **I-0026.2** (canonical repository ID [I-0027](../investigations/I-0027-native-dx12-pre-prototype-discriminators/CONTRACT.md)) from clean integrated `main` `e142b13`, with an initial 16-cycle ceiling and no prototype authorization. Seven static cycles found **outcome B**: the singleton opcode-4 callback copies element qwords and an embedded pointer value and can reach the Default class under guards, without deep-copying targets or proving identity with the separate constructed SPRJ model. Thomas authorized one bounded observation at the Cimetière des Cendres. Cycle 8 [observed](../evidence/I-0027/runtime/OBSERVATION-20260925.md) the direct callback read site before three resets of the same arena, plus a reused target address with changed bytes. It did not monitor all target reads or capture the effective D3D11 tuple; the Default slot was entered but the pipeline getters failed. The reset's observed caller was the separate `0x140023790` route, and the complete inter-invocation callback/runner order and post-reset old-generation reads remain UNKNOWN. [Default pipeline slice](../investigations/I-0027-native-dx12-pre-prototype-discriminators/DEFAULT-DRAW-PIPELINE-STATE-SLICE.md), [last-read table](../investigations/I-0027-native-dx12-pre-prototype-discriminators/FLVER-CALLBACK-LAST-READ-TABLE.md), [capture inputs](../investigations/I-0027-native-dx12-pre-prototype-discriminators/NATIVE-CAPTURE-BOUNDARY.md), [result](../investigations/I-0027-native-dx12-pre-prototype-discriminators/RESULT.md). Thomas closed I-0027 at 8/16 with outcome **B** and authorized its integration into `main` under D-0041. He will decide whether another runtime test is warranted. The game is closed; no DX12 prototype or historical D3D11 fence hunt occurred. The source branch is preserved.


Thomas subsequently authorized **I-0028**, the restricted standalone native D3D12 binding fixture (original mandate title reused I-0027). Its [result](../investigations/I-0028-restricted-d3d12-binding-fixture/RESULT.md) is **PASS restreint** at 4/8: count=1 synthetic color-view/no depth plus explicitly supplied resource/view recipe, private texture/RTV/state/queue/fence, 10 hardware/WARP runs, exact sentinel and negative controls. K-0268 confirms only constructed translation and exclusive-resource lifetime. Live identity/recipe acquisition, helper compatibility and ownership transfer remain UNKNOWN. No live integration or production backend readiness is promoted; Thomas authorized main integration under D-0043; the source branch is preserved. The next proposed discriminator is one existing texture/RTV family's complete creation/acquisition input contract, reusing I-0023 evidence first.

At the clean-main transfer, Thomas authorized I-0029 under D-0044: one real I-0023 color-view input/ownership contract, 16 initial meaningful cycles, and a second offline fixture only after a complete-input gate with Astra High review. [Mandate](../evidence/I-0029/DIRECTOR-MANDATE.txt). The mandate began in a new Work conversation and dedicated branch from8e90788, never directly on canonical main; no live integration or automatic merge. Its bounded result follows.
I-0029 now closes at9/16: [PASS](../investigations/I-0029-live-color-view-input-contract/RESULT.md)
for a real creation-derived color-view contract and11-case offline FLOAT mapper.
Creation texture+view plus logical bind is the bounded sufficient boundary;
native-bind-only acquisition is not sufficient. K-0269–K-0272 preserve retained
stores, one upper reader, exact recipe and direct-destructor lifetime correction.
Live creation snapshot publication and correct generation invalidation before
destruction/reuse remain UNKNOWN. Astra High gatesB/A cleared the restricted
implementation; no live adapter, production readiness, C4 completion or automatic
successor follows. Thomas explicitly authorized integration under D-0046;
I-0029 is merged into main with its source branch preserved. Thomas decides next scope.

Thomas now authorizes [I-0030](../investigations/I-0030-native-resource-identity-generation-lifetime/CONTRACT.md)
under D-0047, from clean main containing I-0029: one selected texture/RTV family's
logical identity, publication, invalidation/recreation and generation-aware native
registry. Initial budget20; static stop at decision-quality event/lifetime evidence,
then the authorized offline registry discriminator and seven required controls.
CPU logical validity and backend GPU retirement remain separate; no persistent
D3D11 COM dependency, live action or automatic technical merge. I-0030 closes
PARTIAL at8/20, integrated into main under D-0049: local publication/mutation
edges and native registry4/4 hardware/WARP PASS; intended incarnation across
actual borrowed references before helper dereference remains UNKNOWN.
See result and D-0048/D-0049; no live authorization or successor mandate.

Thomas authorized [I-0031](../investigations/I-0031-retained-reference-incarnation-logical-handle-abi/CONTRACT.md) under D-0050 from clean main containing I-0030 and integrated its [PARTIAL result](../investigations/I-0031-retained-reference-incarnation-logical-handle-abi/RESULT.md) under D-0051. At8/16, the selected RTV's upper/saved copies contain pointer qwords without an intended-generation field; the direct caller also dereferences through `141920200` before helper `140005F98`. A replacement sidecar passes 9/9 synthetic ABA scenarios in4/4 hardware/WARP runs, but atomic capture, real propagation, CPU wrapper lifetime, conditional saved-target provenance and full event/reader coverage remain UNKNOWN. Model A is conditional; no live native replacement, production backend or automatic successor is authorized. C4 remains open.

Thomas authorized [I-0032](../investigations/I-0032-atomic-logical-capture-retained-handle-replacement-boundary/CONTRACT.md) under D-0052 and explicitly authorized its integration under D-0053. Its [result](../investigations/I-0032-atomic-logical-capture-retained-handle-replacement-boundary/RESULT.md) is PARTIAL at10/24. The selected creation/view-publication transaction is the candidate for atomic logical capture; an immutable qword handle passes the six required offline ABA/lifetime controls in4/4 hardware/WARP runs and removes the two modeled raw-wrapper reads. The shared retained-cell reader/writer set, foreign saved-view dispatch and real event synchronization are not closed. C4 remains open; no live action, production backend or automatic successor.

Thomas authorized [I-0033](../investigations/I-0033-shared-carrier-closure-atomic-native-handle-publication/CONTRACT.md) under D-0054 from clean main containing I-0032, optionally extended its ceiling to44 under D-0055, and later authorized live DS3 operation. Its [result](../investigations/I-0033-shared-carrier-closure-atomic-native-handle-publication/RESULT.md) was **closed PARTIAL at39/44** by Thomas and integrated into main under D-0058. The four-site observer passed neutral and [Cemetery operational preflight](../evidence/I-0033/LIVE-TRIAL-20260928.md), but the [High Wall attempt](../evidence/I-0033/HIGHWALL-CRASH-AUDIT-20260928.md) reproduced a prior exact-signature instrumentation crash before a hit; D-0056 retires that observer. A one-hook replacement passed neutral correlation and live detachment; [visible dragon fire](../evidence/I-0033/DRAGON-FIRE-AND-FXR-AUDIT-20260929.md) produced no selected hit. The proposed m30/m51 FXR's `0x2712` bytes are model IDs. A corrected complete parse and independent action-header audit find no type-`0x2712` action in 5,187 resolved FXRs/428,416 actions; extra effect bundles outside Data1 duplicate existing FXRs, and Data0 contains parameters rather than FXRs. Unnamed entries, resource contents and computed/indirect activation remain unresolved. The exact-build instruction-literal scan found no direct VFX-family scene producer, so saved/direct qword correlation remains unknown. [Narrower R2 candidates](../evidence/I-0033/R2-NARROWING-ASSESSMENT.md) did not close shared carrier, DS3 atomic acquisition or sibling SRV/content ownership. No native replacement or automatic successor. C4 remains open.
