# Current model resource selection — cycles732–733

Part of the [D-0156 design](DIRECT-CONNECTION.md), 2026-10-09. No game run.
This closes and implements the selected resource membership portion; material
programme/constant preparation and command submission remain open.

## DS3 sources checked

Read-only export732 fills the missing full ASM for `D079C0` and `59040` and
exports `236290`. [Provenance](../../evidence/I-0066/artifacts/direct-selection-abi-732/provenance.json)
also indexes reused sources; no historical removed evidence was restored.

The actual `D079C0 → 59040` call has eleven arguments:

| Argument | Current source |
|---|---|
| RCX / RDX / R8 / R9 | entity+D0, current context, [[entity+E8]+8] model, entity+A80 pose |
| stack20 /28 /30 | local CPU descriptor, caller's current400-byte state, null |
| stack38 /40 /48 /50 | inverse placement rows, transformed bounds, centre, sorting float |

`59040` constructs its CPU mode and model wrapper, performs `575C0`, then
passes ten actual arguments to `57A20`. Its assembly offsets account for
seven pushes and70 local bytes; the short C prototype of its caller is not
a callable ABI. `57A20` builds `5C0E0`'s bundle: current state, bounds,
descriptor, model, pose, optional source, lookup result, two indices and patch.

At `D07BAC`, the descriptor receives entity+A40. At `D07BD4–D07BE4`, its
patch association comes from `[entity+A60]+50`; `57CFA` and nearby code retain
the separate optional default-patch branch. The native registry resolves the
actual per-instance association when it belongs to its physical stream owner;
other/default patch families remain unavailable. No original ready flag is set.

`63CB0` ASM verifies mesh material id+0, flags+10, LOD bits+14, material
lookup and its current pass/category/mask tests. `144340` ASM at `1444E1`
and `144552` links this material to the mesh's native layout prefix at+40.
That prefix differs from the old input-layout provider at mesh+20.
`64610` resolves genuine authored IA streams; `D3FA10` overrides slot0 using
mesh+2 as the patch index. This is not necessarily its mesh ordinal. Existing
`D3F7C0/51B940` allocate/update the patch owner's per-mesh array.

The current-state source in `D06E10` C is entity+240, with pass-dependent fade
rules. Its retained ASM699 ends early after `D06EA5`; later recovery and native
preparation are recorded in [cycles734–736](DIRECT-MATERIAL-PREPARATION.md).
`5D5C0` ASM verifies the `5B3D0` hidden arguments: its instance input is the prepared
0x130-byte record, whose+100 group is the copied400-byte state. A raw400-byte
copy must not be mislabeled as this record or passed blindly to the producer.

## Integrated native change

`game_model_selection` reads the current CPU mesh membership, material array,
layout prefixes, pose association and instance patch association. It preserves
flags/type/LOD and distinguishes the separate instanced stream family. The
existing current-view collector resolves those identities against its existing
native material/layout/storage/pose/stream owners, checking their same-model
and same-material associations. Unsupported membership remains explicit.

Each selected mesh retains its authored storage, material, layout and chosen
stream version. `BindModelGeometry` produces IA views directly from those native
allocations; an overridden VB uses its immutable current stream, and its IB
continues using the authored storage. It performs no geometry reupload, host
join, file publication, old GPU call or second-device construction. Pass flags
and material eligibility remain for preparation; this is not pass readiness.

Creation/update/withdrawal use the previously owned engine resource entries.
Selection obtains registry leases, so later withdrawal prevents new acquisition
while held snapshots keep their allocations. No producer mutex remains held
for geometry binding. This initial synchronous backend still owes the full
recording-to-completion lease chain when actual commands are connected.

ER716's stream producer/owner/current-selection association guides this boundary;
primary receipt730 checked that nominal method. All offsets above are DS3-specific.
The patch ready flag and ER CPU delay are not GPU completion mechanisms.

## Executable validation and limits

Build733 passes `/W4 /WX` for both changed module units; no D3D11 import.
Module `35108473A14AD039FC67DC723BC0E22D982EE530CFEEDA4AC86B1196B47C79B5`;
controller unchanged `5F4040A4…`. [Receipt733](../../evidence/I-0066/direct-model-qualification-733.json).

`Test-GameModelSelection.ps1` passes on WARP and hardware. Synthetic CPU inputs
exercise material membership, patch-index/ordinal distinction, changed current
inputs, malformed membership refusal and non-publication after failure. Actual
D3D12 allocations are selected without copying them; old/new stream versions
and authored indices are copied by the native queue after producer references
are withdrawn, and readback verifies all three after actual fence completion.
No GPU error/corruption diagnostic is present. The first test attempt omitted
the existing recorder reset; that test setup was corrected before both passes.

These controls qualify the bounded CPU reader and native allocation retention,
not live selected DS3 consumption or a GBuffer draw. Existing LOD0/R16/single
stream limits remain; instanced overrides, special/default patches, pose palette
delivery, texture selection, programmes, written constant coverage, material
state/masks, scene recording and complete lifetime still need integration.

Next source question is current material preparation as part of the same chain:
recover the current state/fade and programme selection, supply typed inputs to
the existing constant producers, and bind their actual used fields and current
textures to the same native pass encoder. Native112's fault remains preserved;
it does not select this work.
