# Direct native startup services

Current question: which actual engine events move the save notice into the menu,
and how must the integrated native owner remain valid through normal shutdown?
This is a prerequisite of the selected scene chain870, within the restarted
D-0156/D-0157 design. It is not a new crash-selected implementation plan.

## Discriminating observations

Native871 and873, both module870, rendered the title and the previous-session
Quit Game notice, but did not reach a visible Continue menu. Longer timing and
several actual input attempts did not cross that boundary. Original875 was
confounded by the surviving native error modal; do not use it as a comparator.
Clean Original877 dismissed the same notice with the same642,474 click, displayed
Offline/Continue and entered a real loading card. Its120-second bound did not
provide a zone screenshot. [878](../../evidence/I-0066/direct-original-navigation-878.json)
owns that observation and the protected-file restoration evidence.

This supports a native event/progression/service discrepancy rather than a
failure of the exercised pilot coordinate. It does **not** identify the failing
service, prove that save I/O is blocked, or authorize forcing a readiness flag.
Native zone and current selected model drawing remain unobserved.

## Existing responsibilities and source boundaries

| Responsibility | Existing reusable owner / actual engine source | Current limit |
|---|---|---|
| Window and presentation extent | `engine_window`, startup `SceneRenderer`, current output dimensions | Full configuration, resize and normal shutdown remain open. |
| Mouse/button/wheel CPU query | A60700 -> A5B930/A608E0 -> EB6CC0/EB6C10; `NativeUiPointerState` | Source comparison preserves CPU fields and replaces old target-origin subtraction with the full-client native origin. First8 logs do not prove the later click reached the movie. |
| Genuine UI task dispatch and advancement | E4FB30/E50820; `NativeUiTaskDispatch` and `ObserveUiTaskProgress` |873 advanced through current10/pending11. That counter is startup progress, not the save-notice state. |
| CPU movie update | E4DD70 -> tail E5CDA0 -> E5C080 -> E53580 | Preserve actual movie selection, frame delta, event processing and loading dependencies. Do not treat a decompiler-expanded tail call as a new ABI. |
| Authored viewport and movie advancement | E53580 -> E53880 and movie vtable60/68/C0 |873's first8 native viewports were1280x720. Later notice hit testing and acknowledgement are still unknown. |
| Current UI geometry/images/programmes | `GameUiSession`, mesh/image/glyph owners and primitive programme bank | Reuse these current-input components. Offscreen filter effects and complete language families remain unavailable. |
| Recording/submission/presentation | `NativeRenderStep` -> current `PrepareFrame` -> `Clear` -> existing startup queue and primitive encoder | Native871/873 drew real UI; this does not qualify notice acknowledgement or selected scene delivery. |
| CPU admission, withdrawal and retirement | UI dispatcher admission, session loader/HAL/cache close, shared leases; GPU drain | The explicit owner close and the process-detach path currently have different behavior.879 identifies an actual static-destruction defect below. |

A608E0 returns its supplied handle address; therefore passing the owned handle
array into EB6CC0 in `NativeUiPointerState` matches that bounded source contract.
A5B930's genuine CPU eligibility gate remains. A60700 copies mouse bytes8/10,
position20/24 and wheel28 into the output before consulting the old viewport.
None of these static facts establish the input state at the notice in873.

##881–887: current CPU event and normal UI destruction contracts

The bounded read-only exports are in `artifacts/direct-startup-event-source-881`
through `-886` and `artifacts/direct-startup-contract-source-887`. Each retains
its script, console, exact instruction export and resolution report.887 creates
one transient function; the database is never saved or analyzed again.

The actual initializer268F200 publishes the UI task table4798F80. Stage11 calls
E504D0: while genuine menu controller4763258 exists (764130), it pumps the current
container through E4DD70. Its disappearance calls E4DD50/E5BDF0 and requests
stage12. Thus current10/pending11 in873 is not evidence of a save-readiness wait.
Stage12 E50160 destroys the genuine container through its virtual8 and original
allocator68, then withdraws4798EF8. Container destructor E4D720 unregisters its
CPU service, destroys its two children and then the manager through virtual8
E59260 -> E586E0. These are the normal CPU ownership boundaries to preserve.

A5BF70's field489F358+18 is **cursor movement**. F13310 compares actual cursor
coordinates with the previous coordinates before writing it; +19 records whether
the pointer lies inside the client. It is not a pressed-button flag. Separately,
A60A50 preserves the genuine A5B930 eligibility predicate and passes A608E0's
actual input handle to its condition. The examined condition1428B4BE8+10 is
A601B0, an exact two-instruction tail call to EB6EB0; it reads the handle's current
input object and queries EBB510 with code13.887 resolves that query to input+C4.
Its semantic name, the producer of C4 and its relevance to the save notice are
not established. Native A60700 pointer output alone does not qualify delivery
of this separate condition or a later movie event.

E527B0 allocates the genuine28-byte loader through777A00 and E511E0.887's exact
constructor writes a **secondary pointer at loader+8**, not a shared reference
count. Its primary table298ECC0 resolves slot0 to1BB5EC0 (secondary StateBag
getter) and slot8 to E52310 (destructor ->1BB57E0; optional allocator release).
E586E0 obtains the allocation's actual heap through EAA9D0(loader), calls this
virtual8 with flags0, then heap virtual68 and clears manager988. Applying
1BB6CA0 to this loader would decrement the low32 bits of its secondary pointer;
the current native `GameUiSession::Close` therefore has a distinct incorrect
loader lifetime contract. The manager's HAL980 uses shared release1BB6CA0, while
wrapper978 uses virtual0 and allocator68. Native-owned HAL/wrapper allocations
must be withdrawn and retired through their native owners before the genuine
manager destructor consumes those publications.887 does not qualify their
normal native retirement or establish a save-notice cause.

The static event work has recovered discriminating CPU boundaries, but no event
value at the notice. Further neighboring exports cannot establish that value.
The smaller next experiment is one bounded, announced original CPU observation
of genuine pointer/condition/movie advancement, retaining return values and
arguments unchanged. It must resolve the actual delivery contract before the
grouped native navigation observation; no save-ready flag or button is forced.
The native design also needs owned immutable preparation results and explicit
session withdrawal at the recovered engine destruction boundary. Existing
submission/completion leases remain the native GPU retirement mechanism.

889 compiles a bounded CPU observer in the existing bootstrap diagnostic module:
A60700 output, A5B930 eligibility, A5C010 condition13, E53580 actual movie advance
arguments/current CPU fields, and original E586E0 entry/return. Every original
call retains its arguments and return value; the observer reads no original GPU
payload and forces no event or readiness. Records are capped and movie/position
heartbeats are throttled. Reference890 will use the original renderer with this
observer, so it is instrumented and remains separate from clean Original877.
The observer build does not implement or qualify native owner shutdown.

Reference890 executed observer889 with the original renderer. The642,554 title
click displayed the previous-session Quit Game notice;642,474 dismissed it into
the visible Offline/Continue menu. Three screenshots preserve these transitions.
The journal retains29 pointer,22 eligibility,8 condition13 and256 capped movie
records. Thomas then requested stopping technical work; Alt-F4 closed the owned
test process. Controller exit0/fault0 does not qualify native close. The runner
deferred restoration at its process-enumeration exit boundary; after verified
game absence, the retained restoration script restored all3 protected files
and verified their hashes, installed changed/extra empty, ok:true. No Continue,
zone test, new native run or service implementation followed. The event values
remain available for analysis after Thomas explicitly resumes work.

## 892–896: confirm-event producer, session withdrawal, input controls (Claude)

Thomas resumed I-0066 on 2026-10-09. Reference890's dismissal frame: condition13
returns1 only at the click (caller A54025 inside A53FE0, a two-target hit test),
then A5B3F0 -> A60700 reads buttons1 at client641,443. Read-only exports
[892](../../evidence/I-0066/artifacts/direct-startup-input-source-892) recover the
producer: EBB510(code0x13) reads action+C4; the only writers in the input range
are constructor EBB370 (C4=0, timer C0=-1) and update **EBB5B0**: binding query
14233DE40(action,0x13); on press C4=1 with a0.2s timer, auto-repeat at0x3D888889,
else C4=0 and C0=-1. condition13 is therefore the confirm action's one-frame
press/auto-repeat edge, not a pointer flag. EBB5B0 is called only from EB7090,
itself reached through step runner EB5B40 (states EB6150/EB64B0) and the task
scheduler 23C1DC0. Static return addresses near EDAD5F do not match the static
listing (runtime-protected code); runtime stacks are the authority there.

Module892/894 changes, compiled /W4 /WX, D3D11 import absent:
- pass-through observers of EBB5B0 and EB5B40 in both modes; native pointer
  replacement logs button transitions through the existing observer;
- native E586E0 hook: before the genuine manager destructor, the session is
  removed from the registry and closed under a bounded preparation lock shared
  with the render step (2s, explicit refusal instead of a hang); the destructor
  then finds978/980/988 clear;
- `GameUiSession::Close` destroys the loader by E586E0's actual contract
  (EAA9D0(loader) heap, virtual8 flags0, heap68) instead of shared release
  1BB6CA0, and leaves a loader already withdrawn by the engine untouched;
- the UI session registry is never destroyed by CRT detach (RCA879); sessions
  close only at the genuine destructor or the explicit native Close.
Controls: geometry, mesh source, UI mesh output, image source, WARP/hardware
startup primitives and texture ingress PASS. The installed UI-uniform control
fails ("actual mask state ignores current color blend") in units untouched by
this change (last changed by5e78b3c2); it is a pre-existing stale control, open.

Runs (all offline, protected files restored with matching hashes, installed
files unchanged, no fault): Native893 240s/10399 frames/closed1, no pilot input
(access to the game window was not granted); Reference894 90s; Reference895 60s;
Native896 90s/3263 frames/closed1. Observation: the confirm action is updated at
frame rate in both renderers (Reference895 2915, Native896 3140 updates; same
step object class, states0->1, handlers EB6150/EB64B0). Native893's single update
coincided with an open access dialog over the game; treat it as a focus artifact,
not as a native defect. No click reached either renderer in893–896, so whether
the native click raises C4/condition13 at the notice remains **unobserved**.
No session withdrawal event occurred (UI stage12 not reached). Native zone unmet.

Next bounded step: one native run with granted pilot input (Thomas's approval of
the game-window access), title then notice click, recording C4/condition13/pointer
transitions; then Continue and zone observation. Static teardown and loader
contracts are implemented but not yet exercised by a genuine stage12 or quit.

## 897–898: native menu, Continue and zone loading (Claude)

Scripted pilot input became available (Thomas's permission rule for
`Pilot-Game.ps1`). Native897 (module894, 240s, offline, restored ok, fault0):
the title click and the notice click raise action+C4 and condition13 exactly as
in Reference890, but every native pointer read reports position0,0. Keyboard E
dismissed the notice and the **native main menu (Continue/Load/New Game) was
displayed for the first time**; the bound ended the run during Continue.

Cause of the click failure: the genuine mouse device update `23891E0` writes the
cursor position (mouse+0x20/+0x24, read by A60700) only when the old renderer's
display object `4961AB0` exists, scaling the client position by its extent
+0x198/+0x19C. Native execution has no such object. Module898 (1E1E6281…)
hooks `23891E0`: the original still produces wheel and buttons; when `4961AB0`
is absent, the position is computed by the same formula with the current native
presentation extent. No legacy display object is manufactured.

Native898 (300s bound, offline, restored ok): title click, notice click and
Continue click all worked by mouse; the game entered zone loading and the
native renderer presented the loading screen (item tip). Native current-view
selection and the scene CPU update ran (`native_current_view_selected` ×4, one
unsupported selected object vtable295B690 per frame, emit D636F0, no model intents, no draw).
Then an access violation at `23B79C9` (1849 native frames, closed0, fault1,
restoration verified). `23B7960` (export728) dereferences the absent `4961AB0`
to create, per admitted mesh, an old-API buffer of stride×count bytes with
initial data copied from the CPU record (+0x80), storing it in its owner's
array (+0x18). Caller chain: `236351B` (`23633A0` loader under lock4793CF0) ←
`DD625F` (`DD5FD0`, which constructs the 0x68-byte owner `2363700`). This is a
genuine engine resource-creation responsibility still routed to the old
renderer during zone load, the same boundary as historical Native112. Its full
lifecycle (creation, CPU updates, selected use, destruction) must be designed as
one native owner before implementation (D-0156 method), not patched at the fault.

## 901–902: dynamic mesh buffer family omitted explicitly; next boundary

Lifecycle export 901 (`direct-zone-buffer-owner-901`): owner `2363700`
(Update/PostUpdate tasks `234D390`, global registry `4964080`, world
registration `4964480`) loads through `23633A0`, which builds a buffer set
`23B7890` and calls `23B7960`, then a simulation object `239C0F0/239C180`.
Destruction `23630D0 -> 23B7DC0 -> 23B7280` releases the shared old buffers and
CPU copies. The only resolution `23B6DB0 -> 23B74A0` returns an empty descriptor
for a mesh without an entry; its caller `23D6010` checks descriptor+0x40 and
skips. Module902 (E082B1A3…) keeps the engine's own empty-set shape in native
execution (no old object created or invented, `native_dynamic_mesh_buffers_unavailable`),
an explicit initial omission until a native dynamic stream owner exists.

Native899 (offline, restored ok): mouse title/notice/Continue, zone loading;
the dynamic set reported 12 meshes unavailable, then a new access violation at
`1AC921` (stack D0166D, C34467, C48796, 8B77E7, 8BEC7D, 88EA43, 891274;
1576 frames, fault1). Two successive loading faults in old-renderer dependencies
show that the zone-loading path still contains several such families. Per D-0156
and WORKFLOW, the next step is not another fault-selected patch: inventory, from
static sources, every zone-loading consumer of the old display/device objects
(`4961AB0` and the GX device it exposes), group them into families with their
lifecycles, and design their native owners together before the next run.

##879: exact native teardown RCA

The existing870 object set was relinked to a separate evidence directory with
a symbol map, without recompilation or source changes. The mapped DLL is
byte-identical to the executed870 DLL:
`63D2A190994CD871B328116CAFFD489ABD26A265FF53E0EBB06DA9014009A729`.
The object identity manifest, map, instruction excerpts and stack translation
are in `artifacts/direct-native-service-analysis-879`.

The873 instruction at RVA8ABFC is `GameUiMeshOwner`'s `Owner` lookup, at offsetCC:
it loads the hash bucket through `cache_owners` storage at RVA16DC88, then reads
`[rcx+r8*8+8]`. The exception records RCX0. Its RDX1099511628211 is the FNV
prime used to hash the **cache pointer key**, not evidence of framebuffer hashing.

The actual translated stack is:

`CRT process detach -> onexit -> ui_sessions destructor -> GameUiSession::Close`
`-> GameUiMeshOwner destructor -> CloseCache -> genuine CPU item retirement`
`-> DestroyItem -> ItemOwner -> Owner(cache)`.

The module's CRT initializer entries place `ui_sessions` at RVA117578 before
`cache_owners` at117620. Their initializers register destructors through the same
atexit entry. Reverse destruction therefore retires the mesh lookup registry
before sessions call back into it. The registry's bucket storage is null at the
failing lookup. The instruction, runtime stack and actual registration order
establish a native static-teardown lifetime violation. They do not establish an
active GPU-resource race, a model material fault or a save-notice cause.

The873 process reached DLL/CRT teardown with sessions still present;
873 records closed0. `Close()` currently gates executing UI dispatches, withdraws
sessions and other current owners, drains the queue and closes presentation,
but it is triggered by the diagnostic controller stop/frame route. That is
insufficient evidence for the engine's normal quit, cancellation and detached
CPU/GPU owner lifetime. Merely changing static link order would leave this
responsibility undefined.

## Architectural constraint and bounded continuation

The nearest recovered ER lifetime sources in study716 are queue-token attachment
01EAC840, completed-value retirement01EAC850, and frame wait01AFC9A0. They support
explicit owner/update/use/completion contracts in the same renderer domain;
they do not recover a Scaleform input ABI or prove ER's process-detach ordering.
No concrete ER save-notice/UI event analogue is established by this source set.
DS3 event fields and CPU task behavior require their own source evidence.

The native startup responsibility must cover current input -> genuine CPU
event/movie/menu/save progression -> prepared immutable UI/selected work ->
one native submission owner -> completion, plus engine shutdown -> stop new
admission -> finish admitted CPU scopes -> withdraw publications -> drain GPU
work -> destroy services while required CPU heaps/registries remain alive.
Process-detach behavior must not call destroyed service registries or assume
the diagnostic controller already closed them. Reuse existing components;
do not manufacture legacy ready objects, restore the old GPU graph, or alter
the protected save to hide the boundary.

Tranche865–880 closes with inactive ordinary material ownership868, corrected
ordinary entry870, native observations871/873 and clean comparator877.
Selected delivery and whole native zone remain unqualified. The next bounded
tranche881–896 stays within this same startup/ordinary selected-chain scope:

1. Recover the precise input/event/progression gates using existing targeted
   exports first; acquire only missing functions needed for a causal contract.
  881–887 resolve the task, condition and loader contracts above. One bounded
   original CPU event observation now discriminates the missing runtime values.
2. Define and implement the complete native service admission/withdrawal/normal
   engine close responsibility against those sources and the reused owners.
3. Run necessary CPU ownership/event controls and compile affected components.
4. Only after the causal contract is established, announce one grouped native
   navigation/Continue observation. Preserve limits and stop on its protocol.
5. Checkpoint at896, retain the count, and reframe from demonstrated progress.

No native patch or further native run is qualified by879 alone. All13 production
obligations remain OPEN; no main merge or programme completion follows.
