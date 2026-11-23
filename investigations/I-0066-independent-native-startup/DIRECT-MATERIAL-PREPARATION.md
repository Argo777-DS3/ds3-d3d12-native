# Current material preparation

Continuation of the [direct design](DIRECT-CONNECTION.md) under D-0157. Source
recovery and the first native preparation contracts are implemented; complete
material eligibility, constants, textures, pass state and scene commands remain
open. Original read-only source observations740/742/746 qualify loaded code only;
no new native game run or zone rendering qualification.

## Source findings that change the entry

[734](../../evidence/I-0066/artifacts/direct-material-inputs-734/provenance.json)
recovers the bounded `D06E10` fade/call branches that ASM699 omitted. Its listing
ends in epilogue restoration, not at the function's final return. Pass0 uses
entity+16C; ordinary passes1..38 use+170. Passes5..8 additionally suppress the
entity when byte+A5D is set. Pass39 uses the alternate200/204/208/20C/218/21C
track. A selected geometry entity must not be discarded using only pass0's weight.

`D06FC9` copies the current400-byte state through `4E180`; +124 supplies base
opacity. The adjusted branch enables bytes+88/+89, multiplies base opacity by
the current fade, and emits the current variant. Different variants emit a
second branch with `(base opacity * weight) * (1 - blend)`. Native preparation
owns these scalar inputs and intents instead of reconstructing a pointer-bearing
legacy state or forwarding its allocator/retirement calls.

`82220` distinguishes the FLVER material, authored MTD and shader-family owner.
Its material's+70 shader template and+80 MTD provider are different associations.
`D24430` is the derived resolver's CPU lookup for the authored shader name,
returning the loaded effect prefix. Native selection accepts that identity only
when the current `effects` registry owns it; absence remains unavailable.
`83200` is the base resolver's reverse-priority catalogue lookup, not proof of
the derived lookup route. Its assembly returns the successful callback result
in RAX despite the decompiler's void signature.

[735](../../evidence/I-0066/artifacts/direct-program-selection-735/provenance.json)
checks the actual template vtable and `AA280/AB3A0/AA630` selectors and recipes.
The pass table and formatting operands are read from the pinned DS3 executable
by [Read-ProgramRecipeData.ps1](Read-ProgramRecipeData.ps1). Source table entries6/7
are `Gbuf/Gbuf2`; vertex names combine family, pass, rigid/`_Skin`, optional
`_Ins` and channel suffix`@[cl]`. Pixel alternatives are base, `_[A]`, `_[AA]`.
`AA280` tries the preferred VS channel, then the plain VS only if the former
is absent. A missing PS does not justify switching recipes.

`1A8580` copies the400-byte group into draw state+240. `5A120` derives flag6
from group+98 bit1 for ordinary contexts; `140680` passes its inverse to
`AA280`, which controls channel preference. Native geometry preparation uses
this scalar flag directly. The actual deformed/tessellated route includes
auxiliary stages and different tables; it is explicitly unsupported by this
initial programme entry, not reduced to ordinary VS/PS.

## Integrated native contracts

`ReadModelFadeState/SelectModelFade` owns the normal/alternate tracks, geometry
weight, suppression, group opacity and flags. Current-view selection retains
one model/resource snapshot and separate geometry fade intents, preserving
variant/branch requests. LOD0 buffers do not qualify a nonzero requested variant.

`model_programs` resolves exact stage/name pairs against one current effect
owner's catalogue. The existing material/layout/storage owners retain that
effect and its actual immutable shader allocations. A layout cache is keyed
by the effect lease and current channel preference; selection reacquires the
current engine name binding before using it. Withdrawal or a missing current
owner cannot satisfy a new selection with a cached old family. Existing held
selections retain their chosen old versions until released.

The collector resolves the bounded pass6/7 catalogue for all three pixel
alternatives. These are available recipe pairs, **not a decision that a mesh
belongs to a pass or that its material is ready**. Actual CPU classification,
material masks and pixel/deformation choice still govern later preparation.
This avoids choosing one universal GBuffer shader or inventing an old template
object solely to satisfy a virtual method. No legacy GPU field is populated.

ER716's current material selection followed by native constants/binding/emission
guides this boundary. DS3 tables and associations above are independently
recovered. D-0157 permits these engine-side contracts to replace the inherited
service boundary; it does not remove the remaining semantic obligations.

## Owned instance values737

The existing `5B3D0` constant producer now takes `InstanceRecordInputs`: owned
world rows, palette base, colour, bounds, opacity, lighting and clip inputs.
Its byte-view entry remains a diagnostic decoder into the same arithmetic;
the integrated collector never creates a fake0x130-byte old instance record.
The output constant record is0x110 bytes, distinct from that legacy record and
the400-byte CPU group.

Current selected entities provide group colour/origin/bounds, per-branch fade
opacity, instance flag and the held pose's root rows. The genuine executable
matrix supplies the matrix-disabled case. `D079C0` descriptor+90 points at
entity+3D0; `5C0E0` carries this to instance+68. `5B3D0`'s colour flags/values
are read directly from that CPU object, with its authored default when selected
by the source branch. Ordinary fresh records' bounds gates are zero; other
instance families remain outside this entry.

Optional absent inputs leave their words unproduced. Current lighting,
enabled clip transform and pass-specific type value are still unresolved;
missing pose rows do not become an invented identity. No constant is submitted
to the GPU yet. [Receipt737](../../evidence/I-0066/direct-instance-input-737.json)
records build/control identities and source dependencies.

## Current authored parameters738–739

[Source738](../../evidence/I-0066/artifacts/direct-material-policy-738/provenance.json)
checks the genuine CPU MTD reader separately from the old GPU factory.
`19343C0/1934C80/19343D0` own the temporary parser;
`19349A0/19348E0` enumerate parameter nodes. `19353F0`'s ASM establishes lazy
name preparation; `1935550` selects the actual scalar type. The pinned table
read by [Read-MaterialPolicyData.ps1](Read-MaterialPolicyData.ps1) establishes
bool1..4, signed int1..4 and float1..4. `1935010/1934E50/1934D60` supply their
actual values. Parameter-name/extent/type failures never create defaults.

Native material admission retains owned names and numeric values before the
temporary CPU parser retires. Unsupported types remain named unavailable inputs.
The existing MTD constant producer accepts these owned values directly; the CPU
control exercises that entry. Preparation reacquires the associated current
material incarnation/version before resolving its shader family. Withdrawal or
address reuse cannot satisfy a new selection; held old selections retain their
versions. This does not qualify the earlier name-based initial MTD association.

`2DD60` shows authored shader-name tests also changing material flags, while
`1407D0` uses context masks to choose pixel variants. The A7E40 old parameter
factory has an on-disk INT1 body; its static export cannot establish its actual
algorithm. Loaded sources740/742 below supersede that static limitation.
Classification is not yet implemented; CPU metadata presence bits from
`D430A0` are not substituted for the old template flags. No factory/GPU object
is created to obtain them. ER716's current material bytes → upload → CBV →
command chain guides this native boundary; DS3's values and getter ABIs come
from the independently checked sources above.

## Current shader declarations740–744

[Receipt744](../../evidence/I-0066/direct-material-declarations-744.json) indexes
the two bounded Original observations, their hash-checked restoration and
read-only loaded-code decompilation. `I0066LoadedCode.java` overlays only the
transient Ghidra session; the headless log records discarded changes.

Loaded `A7E40` first delegates to `1FEEC0`. Loaded `1FEEC0` looks up the current
parameter name in the shader's CPU declaration owner using `1B0D80`. A matching
SPX declaration supplies the destination fields to the numeric producer;
only an absent declaration reaches the built-in65-row parameter table.
`1FF100` obtains that current declaration owner from the shader-bank CPU record.
The declaration/value association therefore belongs to current material/effect
leases, rather than an independently reconstructed old factory object.

`A97F0/1B0BC0` establish the raw SPX header,0x50-byte parameter rows and
blob-relative UTF16 names. Native effect admission now owns these declarations
before original relocation. Loaded sources746/747 and ASM748 subsequently correct
the initial field labels: row+28 is the destination vector, +30 the animation
lookup key, +2D the producer kind and +2E the RGB transform flag. No scalar
offset or generic dynamic flag is inferred. Actual game execution remains
unqualified.

Loaded `A7E40`, the pinned65-row table and callbacks741 distinguish raw numeric
policy inputs from `D430A0` presence bits. `2007C0/200AC0` derive transparency
from the blend parameter; `200D00/200F10` derive the unsigned alpha maximum.
The original generic parameter route and shader-name adjustments must still be
accounted for before native pass eligibility is published. Old GPU parameter
objects are evidence of semantics, not the new native interface.

## Direct current numeric writes745–752

[Receipt752](../../evidence/I-0066/direct-material-current-752.json) indexes the
loaded numeric constructor/writer, current animation routing and native controls.
Original746 read4,608 bytes from the two named CPU functions. Derived writer
bytes747 come from that same acquisition with parent hash/offset recorded;
no extra game run. Restoration is `ok:true` with no installed changes/extras.

Loaded `1FF700` converts actual signed integer inputs to float, preserves float
words and converts bool bytes to raw words0/1. `1FF130` establishes zero padding
outside the declared component count. Loaded `1FF910` writes4 bytes for a scalar
and16 for a vector at SPX vector-index×16. A matching current animation takes
precedence and interpolates all written lanes; otherwise the flag applies power
2.2 to RGB while preserving alpha. The pinned exponent is independently read
from143D7EE90; the existing native colour producers already use this operation.

Native authored owners now retain numeric types/raw words as well as the earlier
named values. The current material/effect cache owns these immutable numeric
producers. It does not substitute absent declarations, unsupported kinds/types
or unknown defaults. Each selected mesh owns its current produced ranges; an
unavailable animation lookup does not become the base branch.

`D079C0` descriptor+20 points to entity+A40, separately from the colour/light
object at entity+3D0. `5C0E0` carries a nonempty collection to draw-state+1C8;
`5A120` transfers it to context+1D8. `140850 → 2E5C0 → 1FF840 → 1FF910` supplies
that CPU collection plus the FLVER material name. The native selector owns its
0x90-byte rows and names directly. `146920` prefers the first matching named/key
row, then the first universal/key row. Genuine empty membership selects the base;
missing or invalid membership remains unavailable.

Factory vtable752 authenticates `A7DA0`, whose register lookup reads the3/2 pair
initialized by `A7DD0`. `2DD60` uses selector0 for the generic material buffer;
`2E5C0` binds it to the corresponding VS and PS slot. This establishes register3
for this bounded generic route, not every material family or constant producer.
Actual shader-used coverage, other constant families, policy, textures, commands
and live current consumption are still unqualified.

## Executable validation and next dependency

Build736 compiles all three affected module units with `/W4 /WX`; imports exclude
D3D11. Module`1B7158132507251B7EE0411FB9B28809D768C7533F617B3541012B13041A01C6`;
controller unchanged`5F4040A4…`. [Receipt736](../../evidence/I-0066/direct-model-preparation-736.json).

`Test-GameModelPreparation.ps1` passes: independent main/geometry/alternate fade,
two variant branches, suppression scope, invalid-input non-publication, actual
source recipe strings, stage/variant identity, preferred-channel fallback,
collision/wrong-stage refusal and explicit unsupported deformation/layouts.
The control uses synthetic CPU inputs; it does not qualify the game's name
lookup, selected consumption or GPU commands. Unchanged GPU controls733 were
not repeated. No native live test has begun.

Build737 compiles the five header-affected module units with `/W4 /WX`; imports
exclude D3D11. The CPU control additionally passes owned world/bounds/opacity,
independent lighting arithmetic, diagnostic decoder equivalence and missing
or withdrawn field coverage. This does not qualify actual selected consumption.
Bounded737 evidence is backed up separately at
`D:/DS3_evidence_backups/I-0066-instance-737`; the736 backup remains immutable.

Build739 compiles five header-affected units, then recompiles the material
incarnation refinement with `/W4 /WX`; imports exclude D3D11. ModuleB44627B1…;
[receipt739](../../evidence/I-0066/direct-material-values-739.json).
CPU controls additionally pass signed/float/bool values, MTD producer delivery,
short-vector, unsupported-type and nonfinite non-publication. Actual CPU getter
execution in DS3 is unqualified. Separate source/build backup:
`D:/DS3_evidence_backups/I-0066-material-739`.

Build744 compiles the affected bootstrap and new declaration unit with `/W4 /WX`;
module5E1B4888… and imports exclude D3D11. CPU controls additionally pass owned
SPX names/destination fields, source withdrawal, bounds and duplicate-name
refusal. Separate bounded source/build backup:
`D:/DS3_evidence_backups/I-0066-material-744`. Prior backups remain immutable.

At744 the tranche closes with current SPX declarations now owned and protected
factory/delegate bodies recovered. Extend the retained budget to752 for the same
material-to-command chain: verify numeric write semantics, implement direct
constant preparation against actual used ranges, then connect current texture
versions and native pass commands. These results reduce named dependencies;
they neither change the workstream nor reopen the earlier crash-driven plans.

Build751 compiles the three affected units with `/W4 /WX`; module9EF450B3… and
imports exclude D3D11. CPU controls pass typed conversion, scalar/vector extents,
padding, current interpolation/RGB order, unavailable/nonfinite refusal, actual
CPU animation layout, named/universal precedence and source withdrawal.
Backup `D:/DS3_evidence_backups/I-0066-material-752` preserves sources745–752
and this executable scope. No new native live test occurred.

At752 retain the count and extend to760 for this same chain: assemble actual
shader-used ranges at the recovered register using existing admission checks,
then complete current policy/texture/pass inputs and connect native commands.
The newly owned responsibility is per-selected-material numeric production;
neither complete material readiness nor independent rendering is claimed.

Build753 extracts material admission's USED-variable check into the shared
`shader_constants.h` packer. Both the retained admission and direct selected
material entry consume it; neither requires a host packet. The current
material/effect cache inspects the exact resolved VS/PS bytes with the existing
DXBC decoder. Selected meshes own separately packed register3 blocks for each
available recipe. Complete coverage is per actual shader variable, including
the full array/struct extent. Missing reflection/fields, overlapping producers
and invalid extents cannot publish a block. Available recipes still do not
establish pass eligibility or complete draw readiness.

Targeted bootstrap `/W4 /WX` and the CPU controls pass; the retained material
admission unit also compiles against the extracted helper. Module5EBF7426…;
[receipt753](../../evidence/I-0066/direct-material-used-753.json). Controls cover
partial USED arrays, exact offsets, unused/padding bytes, unrelated family-wide
ranges, ownership conflicts and refusal without publication. No native test or
GPU CBV submission occurred. Next dependencies are built-in/inherited parameter
routes, actual policy, other used constant families, current textures and commands.

Next: prepare actual shader-used constant ranges from these current owners and
remaining engine producers, close material classification/pass/pixel choice,
then native texture versions, pass state and the same native
encoder/submission/lifetime chain. Unknown authored defaults and unsupported
parameter types cannot establish used-field coverage. No crash site chooses
this next step.

## Built-in numeric route754–755

The existing Original740 acquisition contains A7E40's computed switch targets.
Export754 adds those already acquired instruction blocks to the transient
function body; no new game run or database save. Its ASM distinguishes the
built-in definition's category selector(+0C), vector index(+10), scalar lane
(+14), CRC key and descriptor(+20). Cases0/1 use numeric producer kind0;
case3 uses kind1. Case4 selects kind2 and cannot use the generic stage writer.
Current SPX name matches take precedence even when their producer is unsupported.

The native entry owns the pinned65 CPU definitions and prepares actual authored
numeric values through the same native producer. Missing authored values do not
create defaults. Current animation selects by row+10 category through146850 when
the selector is nonzero, otherwise by row+7C key through146920. Scalar output is
vector-index×16 + scalar-lane×4; vector output uses the vector index alone.

Reader755 checks every word of the pinned CRC table14472BE30 against polynomial
EDB88320 (table SHA25612F3E057…229D4C86) and independently derives the name keys.
CPU controls pass that actual key, scalar lane/refusal, category/name/universal
priority and ownership after source withdrawal. Targeted three-unit `/W4 /WX`
build passes; module063DD1BF… and imports exclude D3D11.
[Receipt755](../../evidence/I-0066/direct-material-builtin-755.json) and bounded
backup `D:/DS3_evidence_backups/I-0066-material-755` preserve source and executable
scope. Specialized producers, policy, remaining used constants, textures and
commands remain open. No native live test or independent rendering claimed.

## Native classification756–761

Transient exports756 establish generic callbacks+50/+58 as zero/false and
+80/+90 as no-ops. The pinned specialized vtables759 select blend2007C0/
200AC0 and alpha200D00/200F10, independently of numeric stage production.
Initial on-disk specialized constructors757 are protected or incomplete and
cannot support their algorithms. Original758 reads one named10,752-byte region,
without injection or input, under the announced40-second offline protocol.
Restoration is `ok:true`, installed changed/extra lists empty, game stopped.
Loaded constructor exports758 identify the actual specialized vtables; derived
callbacks759 distinguish tessellation and water fields from classification flags.
Constructor2D8E0/source760 establishes initial zero alpha/transparency.

The native policy consumes current authored numeric words, current SPX names
and owned built-in definitions. A declaration name match prevents a built-in
policy effect. It preserves the blend-any/unsigned-alpha-maximum callbacks,
authored flag assignments and alias order, material-type effects, and2DD60's
shader-name rules. Runtime shader catalog bits26/27 remain explicitly unknown;
they are not needed by the bounded142130 classification. Specialized numeric
constant algorithms and those runtime catalogs still need direct contracts.

Each current selected mesh uses that policy plus its real CPU material-instance
flag+D0 to produce142130's class. No old GPU template is populated or queried.
The existing ER material sources141ABFFF0/141ACF120/141E9BE20 were reread at this
boundary: current prepared bytes precede native storage/descriptors/commands;
their layouts do not establish any DS3 policy value or ABI.

Three-unit `/W4 /WX` build and CPU preparation controls PASS. Module654FF4A3…;
imports exclude D3D11. Controls distinguish SPX shadowing, unsigned alpha,
blend/forward/emissive/special precedence, alias order and invalid non-publication.
[Receipt761](../../evidence/I-0066/direct-material-policy-761.json), with bounded
backup `D:/DS3_evidence_backups/I-0066-material-761`, preserves sources and
executable scope. Actual selected game consumption, full pass/mask/pixel policy,
other constants, textures and commands remain unqualified. No new native run.

The760 tranche closes with the actual policy/declaration/callback sources
recovered. Retain the count and extend to768 for this same material-to-command
chain: use the newly owned classification to prepare actual pass/pixel intent,
finish shader-used producer ranges and current texture versions, then record
native commands on the startup queue. These acquisitions reduce required
data dependencies; they neither restart crash-driven work nor establish a zone.

## Current texture preparation762–763

Primary762 establishes the real texture table: SPX blob+98, count byte+C,
stride30, relative UTF16 name, exact first-name selection, slot byte+9,
kind byte+B, flags byte+8 and signed remap+C. The two builtin tables supply
35+3 exact CPU names. Source82220 chooses SPX first; its existing-SPX miss
passes zero flags despite83340's separate flags output. The no-SPX branch
passes16, or4 for kind11. Source1AE3F0 retains first-insertion metadata for
duplicate slots and replaces only the holder. These are source observations,
not a proposed change to original behavior.

The native effect owns unrelocated declarations; each model material now owns
FLVER texture path/type, scale, opaque two-byte metadata and authored ordinal.
No relative string offset survives as a borrowed source reference. Current
material/effect preparation preserves exact-name and alias order, then each
selected mesh acquires named current native texture generations through the
existing CPU resolverD24490 and nativeTextureIngress. A missing/unknown holder
leaves its lease unavailable. No old GPU texture virtual is called. File-local
cache/default-texture behavior, special families and actual selected use remain
unqualified; declared bindings alone do not establish complete shader coverage.
Completion must retain these same leases when commands are connected.

The parallel [pass-settings inventory762](../../evidence/I-0066/artifacts/direct-pass-settings-762/REPORT.md)
establishes distinct scene, transient-pass and worker-context owners. Transient
58C comes from scene+78;596 comes from keyed current CPU values and interpolation.
Worker714's current writer is unresolved. Native zero-initialized compatibility
storage supplies none of these values implicitly. Global39E can be consumed only
as an actual current input; its settings identity/update writer was not recovered.

Targeted `/W4 /WX` build and CPU preparation controls PASS; module8EF207B5…,
imports exclude D3D11. Controls distinguish first SPX names, exact case,
duplicate-slot metadata/current path, owner-dependent fallback flags, withdrawn
source storage and invalid non-publication. [Receipt763](../../evidence/I-0066/direct-material-textures-763.json)
and bounded backup `D:/DS3_evidence_backups/I-0066-material-763` preserve executable
scope and source762. No native live test, CBV/SRV table emission or zone rendering.

Next: verify preservedD24490's input ABI through2FF080, finish used-register
texture/constant coverage and actual native pass inputs, then record through the
existing encoder/startup queue. The reopened ER716 report reinforces this same
current-owner→preparation→binding→command chain; it supplies no DS3 offsets.

## Shader texture coverage764–765

[ABI764](../../evidence/I-0066/artifacts/direct-texture-name-764/REPORT.md) closes
the named lookup input: D24490 accepts a raw terminated UTF16 path inRDX,
ignoresRCX, normalizes through2FF080 and queries current CPU registry4792F58.
The inspected2270/2FBA60/2FB390/2FCCA0 helpers preserve the last path component
before its first dot; no external normalization is inserted. The getter returns
registry record+60 or zero. Native selection checks that the registry exists
before invoking the getter; the original null-registry assertion is not treated
as a missing texture result. Hash/allocator helper internals remain bounded.

Native preparation now compares actual RDEF resource registers with1AD990's
actual slot associations, reusing material admission's PS/2D/count1/float-return
envelope. The selected result retains the matching current native texture leases;
missing resources, array/cube/other stage families and conflicting declarations
stay explicit. This covers texture declarations/residency only. Sampler policy,
complete constants, descriptor tables and commands remain separate dependencies.

Two-unit `/W4 /WX` build and CPU controls PASS; module58D24C31…, noD3D11 import.
Controls distinguish exact-register association despite differing shader names,
missing and conflicting declarations, and unsupported resource/stage families.
[Receipt765](../../evidence/I-0066/direct-material-textures-765.json) and bounded
backup `D:/DS3_evidence_backups/I-0066-material-765` preserve this scope. ABI source
has its immutable backup `D:/DS3_evidence_backups/I-0066-texture-name-764`.
No native live execution or independent scene-rendering qualification.

## Current view and instance constant join766–768

The bounded [sampler source766](../../evidence/I-0066/artifacts/direct-sampler-contract-766/REPORT.txt)
recovers a common VS/DS/PS table and its current CPU filter/anisotropy settings.
Its checked callers are lighting/texture passes. Inheritance by selected ordinary
FLVER materials remains unproved; it is not substituted for that missing edge.
Original GPU GetDesc-based initialization is not a native input contract.

[Constant inventory767](../../evidence/I-0066/artifacts/direct-constant-contract-767/REPORT.md)
identifies the first direct join. Each selected scene now owns its exact current
camera input and masked camera constants from the same stage update. The existing
camera producer's transposed projection at256 is transposed back and fed, with
that owned pose, to existing ProduceViewMatrices. Only its128 produced bytes
are offered to the actual VS b0 `cbMatrixWVP` declaration. The frustum-plane
tail stays unavailable if USED. Independently recomputed lens math is not used
as a byte-identical substitute for this existing producer.

Preparation resolves actual register/name pairs and packs VS/PS b8 `cbSceneParam`
from marked camera words, b4 `cbInstanceData` from each selected fade intent's
marked instance words, and b3 from current numeric writes. A register number
alone cannot identify a constant family. Full USED extents, including arrays,
still require produced bytes; unused padding alone may be zeroed. Scene/view/
instance snapshots remain owned with their selected resources for later command
recording. No original pass-context or captured constant block is reconstructed.

Two-unit `/W4 /WX` build and current preparation controls PASS; moduleC573867B…,
imports exclude D3D11. Added controls discriminate the asymmetric projection
orientation, same-view serial, missing projection words, USED tail and ambiguous
or mismatched register families. [Receipt768](../../evidence/I-0066/direct-selected-constants-768.json)
and bounded backup `D:/DS3_evidence_backups/I-0066-constants-768` retain this scope.
Sampler766 and constant767 sources have their own immutable worker backups.
Actual game consumption, complete constant/pass bindings and native commands
remain open; no live execution or rendering qualification.

### Bounded continuation769–784

The768 preparation tranche closes at its retained count. Continue the same
selected-material-to-command chain within Thomas's unchanged scope: one
source-proved CPU reference observation should discriminate actual selected
shader requirements, sampler identity and current material inputs together.
Reuse the existing observers and restoration protocol; announce any launch.
Complete the missing current producers/bindings and actual pass/pixel intent,
then adapt the existing encoder to those owned resources on the startup queue.
Do not let Native112's fault choose this continuation, reconstruct legacy queues,
restore removed captures, or promote unknown defaults to readiness. If static
queries only add nearby detail, move to the bounded executable observation.

## Owned texture metadata770 and observation prerequisite769

Optional TPF metadata is now an owned tag and exact raw32-bit words in both the
container entry and decoded CPU texture. Absence and present-empty stay distinct;
unknown tags and nonfinite IEEE patterns are preserved without HDR interpretation.
The existing TextureIngress::AdmitOwned copies these values into the same resource
generation, so retained selections keep metadata after source withdrawal and close.
CPU decoder controls and a real native WARP admission/retention control pass after
a complete /WX header rebuild; moduleDB9BD4FC…. The first native control exposed
a stale UI test assertion: slot1 became supported in31623020. Its negative control
now rejects slot2, with no UI implementation change. [Receipt770](../../evidence/I-0066/direct-texture-metadata-770.json)
retains both the initial failure and final result. This does not establish current
HDR producers, shader consumption or game texture lifetime.

The source-checked [observation plan769](../../evidence/I-0066/artifacts/direct-selected-observation-plan-769/REPORT.md)
joins actual shader creation/selection, ordinary mesh-path entry, observed sampler
identities and current UV state. It is diagnostic Original-only work; its separate
observer771 must compile and be checked before the announced offline protocol.
Entering64610 can contain zero actual draw calls, so mesh-path entry alone is not
a GPU draw witness. No launch occurred. The plan also corrects report762's draw-state
pixel flags: actual5A120 stores and140850 reads are+200, not+100. Worker+714 is unchanged;
the backed-up report762 is preserved with this explicit erratum.

## Executable selected CPU observation771–772

The separate Original-only observer now forwards eleven source-checked entries
unchanged, owns bounded CPU DXBC bytes and records actual selected generations,
observed sampler bindings and current UV state. A replacement or retirement between
selection and mesh entry refuses the join. Shader retention stays within1024 recent
completed owners/64MiB and counts evictions; missing bytes remain unavailable.
The observer uses no graphics imports/getters/readback and contributes no production
input transport. Its module and trampolines stay resident until the owned process ends.

Full /WX build and generation/AL-success/sampler/eviction/retirement controls pass.
Bounded [preflight772](../../evidence/I-0066/artifacts/selected-observer-preflight-772/REPORT.txt)
checked retained ASM signatures, original forwarding, lock scope, process-age bound,
disable/drain and restoration. [Receipt772](../../evidence/I-0066/direct-selected-observer-772.json)
and reviewed backup preserve the exact build. No launch yet. The next discriminating
step is the announced offline reference773, capped at four CPU records or120 seconds
from process creation. Mesh-path entry remains separate from a GPU draw witness;
sampler bypasses and GX-context reuse are unresolved coverage.

The observer's256-byte snapshot from state+54 preserves raw bytes only. The
retained230610 writer establishes20 offset pairs ending at+F3; the wider snapshot
also includes other state fields. It does not prove32 available UV pairs or
initialization of any unwritten pair.

## Current UV source and negative observation773–775

The announced Original-only reference773 reached the visible saved scene and
closed at its120-second bound. It produced zero complete selected CPU records:
407244 unavailable associations,546 retained shader owners, zero evictions,
exceptions and faults. Its first reported failures were nonordinary pairs and
missing shader generations at selection. This is insufficient information for
selected RDEF/sampler/UV consumption, not evidence of an inherited default.
Restoration passed with matching protected hashes and no installed changes.
[Receipt775](../../evidence/I-0066/direct-selected-and-uv-775.json) preserves
the negative result; inspect missing creation coverage statically before deciding
whether another observation is useful.

Root checked the retained ASM behind [UV774](../../evidence/I-0066/artifacts/direct-current-uv-774/REPORT.txt):
140680 calls230610(material+D0,state) before1AD990/1ADEB0. The state constructor
clears time/mask together at+4C, sets rate+F4=1 and leaves offset pairs unwritten;
5A120 copies actual worker+4E34 into time.230610 advances16 ordered scroll rows
and writes at most20 offset pairs. Tiling writes only eligible destination rows;
unwritten words remain unavailable. Duplicate destinations overwrite in order.
Row+30 is the SPX remap>=0 gate, independently of opaque FLVER metadata.

[Owner inventory775](../../evidence/I-0066/artifacts/direct-uv-owner-inventory-775/REPORT.txt)
locates the existing per-model material and immutable pose/stream version pattern.
A selected C++ material lease does not keep original material+D0 alive after its
CPU destructor. Current scroll history must be copied into a native owned version
before withdrawal. Current history initialization/provider and authenticated
time/rate delivery remain open; scene serial is not an established UV clock.
Reuse the native arithmetic with explicit inputs and written masks; retain the
same selected version through preparation and eventual queue completion, following
ER716's checked current-material/upload/CBV chain. No live consumption or native
draw is established by these source contracts.

## Owned UV production777 and corrected diagnostic779

The existing native tiling arithmetic now accepts ordered current writes and
per-word coverage. Later unavailable writes clear a prior destination instead
of inheriting its value. The strict complete25-row interface keeps its earlier
refusal behavior. Typed16-row scroll history and20-pair selection offsets retain
230610's SUB/MUL/ADD order, duplicate overwrite and history-only slots>=20;
failure leaves the caller's owned history/offsets intact. Exact model bindings
provide scales, remap and stage gates. The existing RDEF packer receives only
written words; a USED400-byte array cannot become ready from one produced row.

[CPU controls777](../../evidence/I-0066/artifacts/direct-owned-uv-777/REPORT.txt)
pass45 assertions under /WX; the full native bootstrap rebuild also passes.
Constructed RDEF controls are not selected DS3 shader evidence. No current-game
UV owner/update/clock or native submission is qualified by these pure functions.
ER141ABFFF0/141ACF120/141E9BE20 were reread: current material bytes populate a
current allocation before its CBV. This motivates owned selected versions, not
transplanting ER offsets or a historical draw-state buffer.

[Targeted source778](../../evidence/I-0066/artifacts/direct-uv-initialization-778/REPORT.txt)
recovers22B2C0: active mask0 and16 rows with slot-1, zero coefficients/phase/time.
This is constructor initialization, not proof of later current state.13BDF0 is
a reader of worker-relative time; its writes go to its own receiver, not to the
worker clock. The missing clock producer remains open. Bounded field-query
argument failures and their causal analysis are retained; failed output was not
adopted as source evidence.

[Erratum776](../../evidence/I-0066/artifacts/selected-observer-negative-776/REPORT.txt)
establishes a concrete coverage error in771:6D700 creates Compute, and6D8F0 is
the Domain route. The proper VS chain is6E230→1925CC0→1927650→1930420; PS is
6DFD0→1926030→1927520→19308F0. Root checked these leaves/ABIs and retirements
6E180/6DF50. Preflight772 checked argument forwarding and lifetime, not these
entries' complete semantic chains. Old reports/backups remain immutable with
this correction; the exact cause of each unlogged773 refusal is still unknown.

Corrected observer779 uses the proven VS/PS creation/retirement entries and
passes its full /WX build and unchanged generation/sampler controls. Its shader
owner bound is16384, above the known11176 startup admissions; the64MiB byte cap,
four retained records and120-second process-age bound remain.
[Receipt779](../../evidence/I-0066/direct-owned-uv-and-observer-779.json)
preserves the reviewed build before observation781.

## Real selected CPU associations780–781

Announced offline Original781 produced four complete CPU associations with the
corrected observer779: actual VS/PS generations and DXBC, selected material
rows, observed sampler binder identities and raw UV state. The controller
closed at four records, fault0/exceptions0/callbacks0. Restoration passed with
matching protected hashes and no installed changes. The64MiB shader cap caused
9896 evictions; this bounded sample is not complete shader-family coverage.
Mesh-path entry is not a GPU draw witness. [Receipt781](../../evidence/I-0066/direct-selected-cpu-781.json)
retains the journal, eight exact selected bytecodes and restoration proof.

All four PS RDEFs mark the entire400-byte/25-element texture-tiling array USED.
Only4/6 material rows are present and all four UV masks are zero. Raw unwritten
UV bytes do not establish initialized offsets. This does not make a partial
array ready; shader instruction access coverage or independently produced
remaining words must be established before native binding. The observed sampler
overrides also preclude treating the common766 table as universal.

Root checked the full16-row GX table and entry chain in retained C/ASM behind
[source780](../../evidence/I-0066/artifacts/direct-uv-current-sources-780/REPORT.txt):
82220 passes header+description[5] to material virtual+58→141070. Ordered GX
records supply mask, slots and coefficients; phase/time retain constructor or
current history. GX38 targets row8/slot16, while GX28 targets row9/slot4.
The present native material constructor has not yet admitted these records.
Current clock, ownership, selected consumption and native commands remain open.

## Tranche checkpoint783–784 and implementation choice

Root checked the primary ASM behind [clock783](../../evidence/I-0066/artifacts/direct-uv-clock-783/REPORT.txt):
D5B140 forwards tick to D57900; tick+8 supplies a float delta unless receiver+D6
suppresses that invocation.23410 passes worker+4E30 to126EA0, which adds the
undivided delta to worker+4E34. QPC updates a separate field+4E3C. Units remain
those of the actual tick; physical seconds and dispatcher frequency are not
asserted. The native backend deliberately omits this original worker. Its
replacement time owner must therefore consume the genuine CPU entry/gate,
without substituting frame serial or wall time.

The [bounded GX inquiry784](../../evidence/I-0066/artifacts/direct-gx-owner-784/extent-worker/REPORT.txt)
did not recover the complete file allocation extent.1F9440 is unrelated to file
construction;1FA030's geometry address formula is not proof of a metadata boundary.
Do not infer header+C→payload equality. Further historical file-parser recovery
does not change the already established GX fields. The direct contract can copy
actual record headers and payloads synchronously while the FLVER loading caller
owns them, using checked reads, record-length/progression and native size/count
bounds, then own the bytes and typed history. These bounds limit native admission;
they do not qualify arbitrary malformed FLVERs or prove a whole-file allocation.

This closes source tranche769–784 with progress in actual selected associations,
UV authored inputs and supplied engine time. Continue785–800 within the same
authorized direct-integration scope: instruction access coverage, executable
owned UV versions/time, selected leases, samplers/pass bindings and native commands.
ER's checked current-material allocation/CBV chain governs version ownership;
DS3 supplies the independently recovered values and CPU update gate. Native
preparation, GPU consumption and completion remain unqualified.

## Current owners and effective reads785–790

Actual selected DXBC781, independently disassembled in
[785](../../evidence/I-0066/artifacts/direct-selected-accesses-785/REPORT.txt),
reads tiling rows0..2,0..5 or5, rather than every reflected element. The separate
conservative analyzer [787](../../evidence/I-0066/artifacts/direct-conservative-reads-787/REPORT.txt)
includes first-operand conditions omitted by the historical host analysis.
Static requirements retain every mentioned component; relative indexing requires
the entire declared buffer without assuming RDEF bounds.63 CPU controls pass.
This establishes requirements, not current values or dynamic index validity.

The [compiled integration788](../../evidence/I-0066/direct-owned-uv-integration-788.json)
owns actual GX bytes at material construction and immutable scroll versions at
selection, and receives the genuine supplied delta/gate through D57900.66 CPU
controls [786](../../evidence/I-0066/artifacts/direct-owned-uv-786/REPORT.txt) pass
against the real owner API, including withdrawal and retained versions. Current
game delivery has not been executed. Per-material offsets are contributions;
the eventual pass preparation must merge the eligible selection scope in its
proved order. Fresh ordinary rate1 does not cover later override families.

[Sampler source789](../../evidence/I-0066/artifacts/direct-material-samplers-789/REPORT.txt)
separates SRV/UV material preparation from pass sampler choices and current
filter/aniso settings. The common table is not universal; binder observations781
do not identify their issuer. Native initialization must own D21F0's CPU inputs
and settings policy, replacing its legacy GetDesc population. The
[reuse inventory790](../../evidence/I-0066/artifacts/direct-sampler-reuse-790/REPORT.txt)
identifies the existing dynamic sampler encoder and actual fence lifetime,
excluding diagnostic defaults, inherited missing slots and border approximation.
Selected sampler inputs, whole pass preparation and native commands remain open.

Compiled [792](../../evidence/I-0066/direct-read-integration-792.json) associates
conservative reads with each same owned programme/interface and uses them for
current numeric, camera/view and instance blocks. Unavailable analysis keeps
strict RDEF coverage; static out-of-declaration reads refuse rather than clip.
Whole-buffer dynamic coverage does not prove runtime indices or valid pose.
Read-packer controls [791](../../evidence/I-0066/artifacts/direct-read-constant-controls-791/REPORT.txt)
pass95 assertions, including actual tiling rows and whole-declaration dynamic
requirements. No native live run or new selected draw is established by the
complete /WX module build.

## Current sampler owner and ordered preparation793–800

The [selector chain797](../../evidence/I-0066/artifacts/direct-sampler-selector-797/REPORT.txt)
establishes the external CPU request at `*(image+4796298)+A8` through
D55190→DFDD20→CED9A0→23660. The original local `Ignore External Settings`
override is a separate unqualified family. Constructor [795](../../evidence/I-0066/artifacts/direct-sampler-profile-795/REPORT.txt)
supplies the five profile pairs; its initial selector2 is not a current setting.
Decoder [796](../../evidence/I-0066/artifacts/direct-sampler-inputs-796/REPORT.txt)
owns the exact13 CPU words with133 passing controls, excluding GetDesc output.
The immutable bank [798](../../evidence/I-0066/artifacts/direct-sampler-owner-798/REPORT.txt)
publishes only after an explicit current selector. Selection retains the exact
returned version; withdrawal blocks future acquisition without invalidating
held leases. No implicit profile is selected.

Compiled [800](../../evidence/I-0066/direct-sampler-integration-800.json) reads
that request at native render-step and at the actual current selection, then
retains16 decoded descriptions. Repeated field/owner reads bound the snapshot;
CPU writer synchronization, settings transition lifetime and actual game delivery
remain unqualified. Stage/slot choices and GPU descriptors are still separate.
The nearest checked ER analogue,710 `141ACD170`, gathers current backend inputs
against the selected layout and active stages before native root binding. This
supports explicit current binding ownership; its category meanings and offsets
do not establish DS3 sampler or pass policy.

[794](../../evidence/I-0066/artifacts/direct-selected-pass-policy-794/REPORT.txt)
distinguishes engine pass, preparation selector and shader catalogue pass.
Physical mesh eligibility precedes230610; mask/pairs persist through ordered
material preparation within one constructed scope, including late failures.
The800 collector now acquires history without advancing candidate materials.
Ordered pass preparation must perform the advance and merge; F8/F9 use the
actual current animation block, not universal false defaults. Callback effects
and special/instanced scopes remain outside the initial ordinary contract.

Constructor [799](../../evidence/I-0066/artifacts/direct-material-mask-source-799/REPORT.txt)
independently proves material+18 initiallyFFFF and+1C initially1; primary ASM
also writes all-ones64-bit masks at+8/+10. This resolves initial values, not
later mask immutability. Current native selection must acquire required CPU
masks explicitly; recipe availability is not a substitute for eligibility.

Tranche785–800 closes with direct owned UV/time, effective shader requirements
and current external sampler versions compiled. Native commands and zone remain
unmet. Continue801–816 in the same scope: typed ordinary pass eligibility and
ordered UV/tiling preparation, complete same-program binding requirements,
existing native PSO/descriptor/command components on the startup queue, then
retained submission/completion. Only a complete executable selected chain
permits the next announced native-zone protocol. No crash-selected detour.

## Owned preparation inputs801–807

[Compiled805](../../evidence/I-0066/direct-preparation-inputs-805.json) snapshots
current material capability/secondary masks, special flag and base deformation
under the registry lock that gates CPU-prefix retirement. Group+C0/C8 supplies
the two current shader masks; entity+3D0 supplies the independently recovered
animation gates. Base deformation is not the effective shader6/7 choice: requested
LOD and preparation flags still govern it. No constructor default substitutes
for a missing current input.

[Source801](../../evidence/I-0066/artifacts/direct-pass-inputs-801/REPORT.txt)
recovers the ordinary category, pass masks and pixel-choice priority.
[807](../../evidence/I-0066/artifacts/direct-pass-settings-source-807/REPORT.txt)
corrects its context shorthand: render context5A120(+714) differs from pass
context57A20(+124/+58C/+596). External AA request/profile bytes do not establish
the former's current pixel policy. The two CPU getters and material-provider
association are separate bounded work809; current T+7C production remains open.

[UV scope802](../../evidence/I-0066/artifacts/direct-uv-scope-802/REPORT.txt)
merges complete contributions atomically in one explicitly gated invocation,
preserving ordered inheritance and refusing partial masked writes.34 controls
pass; bootstrap consumption remains pending.
[Vertex decoder806](../../evidence/I-0066/artifacts/direct-owned-vertex-layout-806/REPORT.txt)
owns semantic names and translates the exact seven CPU words through checked
DS3 functions/tables.35 controls pass and805 consumes the decoder.804's initial
export did not resolve the mapping;806 records that closure and the redundant
export correction. Stride, format widths and programme signature remain separate
draw preconditions.

[Reuse803](../../evidence/I-0066/artifacts/direct-native-encoder-reuse-803/REPORT.txt)
selects the existing native PSO/heaps/gbuffer encoder, resident SRVs and actual
completion contracts. Its b0..11 tables exclude b12 and its constant packing
clips dynamic palettes to RDEF size: the direct draw component808 replaces those
interfaces jointly while retaining full payloads and the startup device/queue.
805's complete /WX build establishes compilation, not game delivery or GPU draws.

## Direct preparation and native frame808–816

[Compiled816](../../evidence/I-0066/direct-native-selected-chain-816.json) consumes
typed ordinary eligibility812 before ordered UV802, retains current scroll and
pose versions, and gathers available same-program bindings into owned draws808.
The startup renderer uses its existing device/queue, six targets and encoder,
then composes material color, presents and retires after actual completion.
[Frame814](../../evidence/I-0066/artifacts/direct-native-model-frame-814/control/REPORT.txt)
passes69 WARP assertions, including current texture replacement, repeated
presentations of CPU view2 and resize65x37→71x43. Its shaders are synthetic;
no selected DS3 game draw or zone is qualified.

[Pose810](../../evidence/I-0066/artifacts/direct-pose-binding-reuse-810/REPORT.txt)
and [owner811](../../evidence/I-0066/artifacts/direct-native-pose-owner-811/REPORT.txt)
separate current M, auxiliary cloth and cancel values;51 CPU controls pass.
[Index815](../../evidence/I-0066/artifacts/direct-palette-index-contract-815/REPORT.txt)
passes48 controls for the two actual selected VS families, but is not yet
consumed at816. Dynamic b5/b12 binding still requires explicit palette choice,
available full values and checks against the current physical vertices.
Missing cancel data never becomes identity.

[Getters809](../../evidence/I-0066/artifacts/direct-pass-getters-809/REPORT.txt)
checks material+80 and the provider/model association. It corrects807's claimed
missing getter body:297 already retained it. [Producer813](../../evidence/I-0066/artifacts/direct-template-alpha-policy-813/REPORT.txt)
connects authored alpha_ref to actual T+7C unsigned-max production. Current native
policy consumes that value directly, without claiming all old template writers.
816's primary ASM check also corrects801's category shorthand: the equal-scalar
branch preserves category2 when the instance-category byte is zero.

[Policy812](../../evidence/I-0066/artifacts/direct-native-pass-policy-812/REPORT.txt)
passes57 controls and preserves failed-pair invalidation and pixel-choice
priority. The integrated initial OrdinaryDefault policy, opaque PSO and
material-color composition are explicit native contracts under D-0157;
old render-context AA, further state flags and lighting parity remain open.
Sampler slots0/1/2→bank12 and8→bank1 cover only the narrow ordinary781 witness.
At816, VS b11, PS b2/b9 and scene SRVs still need current producer/resource joins.
Unsupported inputs refuse individual rows; clear/UI output is not a zone.

Tranche801–816 closes with this complete /WX build and bounded native mechanism
control. Used816, next maximum832 without reset: finish these missing binding
inputs in the same chain before an announced native-zone protocol. The13
production obligations remain OPEN.

## Selected palettes and group constants817–820

[Compiled819 and proof820](../../evidence/I-0066/direct-selected-binding-820.json)
consume815 on the exact selected shader and current physical vertex bytes.
[Selection817](../../evidence/I-0066/artifacts/direct-pose-selection-817/REPORT.txt)
checks64340's material/pose branch and concrete vtable slots, then explicitly
chooses current or auxiliary/cancel outputs. Unknown Q and missing data refuse;
normal M never receives a nonneutral cloth cancel. Instanced/previous/nongated/
fast families remain outside this initial native binding contract.

[Scroll818](../../evidence/I-0066/artifacts/direct-tex-scroll-818/REPORT.txt)
owns the current group's first64 bytes directly for VS b11, reusing the exact
copy operation of the existing producer. It is separate from ordered PS tiling.
[GPU820](../../evidence/I-0066/artifacts/direct-palette-gpu-820/REPORT.txt) passes55
WARP assertions with the unchanged actual selected VS, two blended bones0/124
and125 full matrices. Normal/cloth/forced outputs match the scalar oracle;
actual completion retains the pose after producer withdrawal. The diagnostic
PS and other inputs are synthetic: this does not qualify a selected DS3 draw.

The remaining same-chain inputs include PS b2/b9 and required scene SRVs.
No game launched; the native-zone milestone and13 obligations remain OPEN.

## Current dynamic scalar and clock inputs821–824

[Checkpoint824](../../evidence/I-0066/direct-material-dynamic-824.json) preserves
source821 and the full /WX module823. The direct b2 producer consumes owned
FLVER GXMD and SPX scalar descriptors separately from generic MTD/b3 values.
It preserves ordered or key/component/default scalar words, then140850's
dither word. Provider/colour routes, duplicate records and unresolved current
animation refuse; the initial route requires an available empty animation table.
Wet/damaged ranges remain absent, regardless of physical buffer zeroes.

The existing native producer now shares typed scalar, dither and frame-time
operations with direct integration. PS b9 consumes the immutable current
simulation-time owner already used by ordered UV; environment rows are separate.
[Control822](../../evidence/I-0066/artifacts/direct-material-dynamic-822/REPORT.txt)
passes31 CPU/read-set assertions, including retained actual PS: record3 accepts
dither-only b2, equipment refuses missing wet/damage b2, and equipment accepts
current B0 time for b9. Synthetic parser values do not qualify DS3 value delivery.

Continue wet/damaged source/provider updates and current scene SRVs in this same
chain. No game launched and no zone/production obligation closed.

[Ownership827](../../evidence/I-0066/direct-material-ownership-827.json) adds the
source-checked final-writer limit: generated destinations below270 refuse until
later fixed-field overlays are connected. Control825 passes32 assertions and
module826 compiles on full823. Source825 corrects821's GX18/GX28 labels to
GX81/GX82; hex keys and actual function associations were unchanged. Current
decal41/42 and dither38 need engine resource owners/initialization; the host's
capture texture input and withholding seed do not supply that production contract.

## Common and instance texture sources828–832

[Source832](../../evidence/I-0066/direct-scene-texture-source-832.json) identifies
7ECB0's16 generated64x64 noise images and one specific t38/index0 use in1D6D20.
**Corrected by833/834:**075BCD15 is consumed through incoming EDX by141828FD0,
forwarded through17AFFD0/17B0040. The local64-bit LCG produces four high words;
there is no global RNG or time dependency.832's contrary interpretation trusted
an incomplete decompiler signature and is superseded; its receipt stays immutable.
The two math functions have XMM0 input/output concealed by their void C bodies.
This does not establish the whole Gbuf phase-selection policy.

5EDA0 chooses current instance entries and binds actual images to t41/42;
disabled/absent image branches choose common270+8.6FFA0 explicitly binds that
same common image to both registers.833/834 also source-check06C00's common-bank
index1 as a1x1 black opaque BGRA8 image; current native instance association
remains open. The historical host seed is not initialization proof.
828/829 distinguish target/state/sampler setup from these SRV producers.

Reuse the existing native TextureIngress, upload recorder and retained SRV
ownership for joint material/scene selection. Tranche817–832 closes; extend
to848 without resetting832, within the same selected chain. No new game/GPU
execution or rendering qualification; dynamic wet/damage still needs producers.

## Native common-image ownership833–837

[Checkpoint837](../../evidence/I-0066/direct-common-textures-837.json) preserves
the ABI correction and a bounded CPU oracle built from five actual pure routines
in the hash-pinned executable.834 validates the four-word seed and all262144
image bytes against those original routines. The native producer preserves exact
SIMD coefficients/order and uses strict FP compilation; it also supplies the
source-checked common decal image without assigning it to an unknown instance.

835 publishes16 native dither leases through existing TextureIngress, on the
startup device/queue. Current selected scenes retain that owner; actual native
draw bindings retain resource/view ownership through completion. Joint selection
distinguishes material and scene registers and refuses ambiguous or absent inputs.
837 passes11 CPU assertions on actual retained PS records3/1: the former accepts
t38 plus its material image; equipment still refuses missing decal associations.
These typed control inputs do not qualify current DS3 delivery or GPU rendering.

The nearest checked ER paths716,141AC1D30/141ACF120/141E9BE20, discriminate common
versus selected material inputs, current upload copying and native descriptors.
DS3 pixels/ABIs remain independently sourced; native ownership reuses existing
fenced leases rather than ER offsets or delayed-retirement assumptions.

Full /WX build836 includes the joint-interface change; its common producer is
then recompiled with/fp:strict. Native ordinary dither index0 is an explicit
initial policy under D-0157, not proof of old temporal phase parity. Current
instance decal choices and wet/damage b2 values/updates remain required before
an announced native-zone run. Used837/848; all13 obligations remain OPEN.

## Direct damage inputs838–848

[Checkpoint848](../../evidence/I-0066/direct-damage-input-848.json) connects
current GX80 rate and GX81/GX82 UV inputs under the current FLVER/MTD/SPX owners.
It reuses the existing native numeric conversion and known-word writer. Loaded
1FF700 and845's1FF660 establish provider base values; later b3 animation/RGB
writes do not mutate that base. A same-name unqualified SPX replacement or
unavailable provider membership leaves its UV range absent. Actual22E930 ASM
uses GX81's override flag for both UV branches, despite GX82 storing its own flag.

Current animation rows now own22D620's material-index field+78, distinct from
name/key selection.140850 supplies material+60 to its outer gate; only an open
gate permits the whole-table damage-rate CRC lookup. An unavailable table or
nonempty table with unavailable index leaves rate coverage absent. The rate
writes four bytes, UVs sixteen each; disabled damage produces no known zeroes.
Generated scalar updates remain separately limited to an available empty table.

CPU847 passes57 assertions including earlier actual-PS controls, ownership,
provider selection, override precedence and animation gating. Module848 compiles
the three affected units on full836, preserving its strict-FP common producer.
These controls use typed fixtures and retained actual bytecode; no game/GPU run
or current selected draw is qualified. Wet ranges and instance decals remain open.

Source838–845 also rejects two acquisition shortcuts: matching small RVA words
in exception/unwind tables does not establish callback registration, and an
absent Ghidra xref does not prove absence of a pointer.841 finds the actual
common binder pointer143D42FD0; its selected instance association is still open.
839 establishes MatDyn creation semantics, not current wet values.840's nearby
constructors are not assigned to the draw-state owner from coincident offsets.
845 finds GXFT_ValidWet's specialized apply method is RET, with base accessors;
it supplies no missing wet color/state producer.843's failed export is retained.

The checked ER716 selected-material/current-copy/native-descriptor paths still
govern ownership; DS3 GX/provider fields and animation gates use independent
DS3 sources. Tranche closes at848, maximum864 without reset, within the same
selected chain. Next: current wet material/draw-state producer and instance
decal selection, then complete native binding. All13 obligations remain OPEN.

## Direct current wet draw ownership849–852

[Checkpoint852](../../evidence/I-0066/direct-wet-draw-852.json) resolves the draw
values without recreating a legacy draw state. D06E10 selects group=entity+240;
4E180 preserves its wet words in fade copies.1A8580 copies all400 bytes to
state+240.22E930 ASM independently reads2D0/2CC/2D4 as a,b,c. Native selection
now owns the three current CPU floats directly in its immutable scene version.
CPU851 passes13 assertions for updates, retained ownership and failed/nonfinite
reads; module852 compiles the two affected units on full836 plus848.

849's6E780/6E770 class is the camera/view owner, whose coincident2D0/2D4 fields
are pixel height/lens inputs. That acquisition is preserved as a counterexample,
not used as a wet producer. The source-checked ER716 selected-material/current
copy method governs ownership; the DS3 fields use independent DS3 evidence.

The wet draw values are retained but do not yet establish complete b2 delivery:
material activation/base/updates and override handling remain required. No
constructor defaults substitute for current inputs. No game/GPU execution or
native zone qualification; current instance decal association remains open.
Used852/864, all13 obligations OPEN.

## Current wet material producer853–856

[Checkpoint856](../../evidence/I-0066/direct-wet-material-856.json) connects the
selected current CPU material fields to the existing native wet calculation.
82CC0/22B550 already construct the retained CPU prefix; selection copies current
material210..24F under its lifetime lock, without reading an old GPU buffer or
substituting constructor values. The selected scene owns the current43-row wet
shader catalog. Activation follows2DD60's exact names/catalog or141070's mask2.
A97F0 ASM establishes the embedded header at blob+18, so its mask is raw blob+58;
the earlier offset-word interpretation/control fixtures are explicitly superseded.

22D620's material-index gate and category-before-CRC selection update both colors;
parameter animation updates override a/c only. The extracted ProduceMatDynWet
calculation reuses the existing producer and writes four16-byte ranges230..26F.
Unavailable inputs and disabled wet add no known bytes; nonfinite results refuse.
CPU855 passes97 assertions including57 previous controls and actual retained PS
read sets. Equipment still refuses its independently missing b2 ranges. Module856
compiles four affected units, preserving836's strict-FP common-image producer.

The checked ER716 selected-input/current-copy/native-descriptor method still
governs the boundary; DS3 layout and updates use independent sources. Current
game delivery/GPU rendering, special callbacks, disabled-wet creation coverage
and full binding remain unqualified. Continue current instance decal selection;
used856/864, all13 obligations OPEN, no game launched.

## Current common-decal association857–860

[Checkpoint860](../../evidence/I-0066/direct-instance-decal-860.json) connects
D079C0's current descriptor+98 provider choice,57A20's model+224 bit2 and current
14473C7E5 option to the existing native common decal producer. A non-null provider
stays distinct and unavailable; only the proved common branch admits PS41/42.
Startup owns the image on its existing device/queue. Each selected mesh retains
paired registers/native texture leases, reused unchanged by submission.

CPU/hardware859 passes33 assertions, including the previous11 joint controls:
actual retained PS1 resolves41/42 at scene indexes1/2, PS3 resolves38 at index0.
Hardware readback preserves exact common-image bytes after CPU withdrawal and
owner retirement, with no D3D12 errors. Module860 compiles two affected units.
The checked ER716 selected-input/copy/descriptor boundaries govern ownership;
DS3 choice, ABI and image identity remain independently sourced. Material+60's
animation gate bit pattern is the name CRC established by1B0AB0, not an ordinal.

No game/model draw or native-zone proof. Custom providers,43 and instanced paths
remain unavailable. Complete independently missing b2 ranges and the same selected
chain before an announced live protocol. Used860/864; all13 obligations OPEN.

## Composite readiness and current source boundary861–864

[Checkpoint864](../../evidence/I-0066/direct-material-readiness-864.json) composes
the existing active generated/damage/wet outputs into Ready for all16 actual
equipment PS1 b2 read words. CPU861 passes6 assertions; removing270 or wet still
refuses, while adjacent unused words remain unknown. Earlier separate controls
did not establish this composed result or prove current DS3 delivery.

Source862/863 distinguishes the old shared CPU working arena from GPU creation.
1479B0 stores or allocates working memory without filling it;147CA0 supplies no
initial data on the772A0 creation path. The lower explicit memset applies to a
different emulation branch. Disabled wet/damage bytes cannot become known zeroes
from those sources. D-0157 permits a native per-material initial/update contract;
inactive behavior still needs qualification. A present animation owner with a
null table is not the absent-owner branch in22D620.

Module864 adds at most16 diagnostic rows for failed current native b2 preparation,
including exact missing words and owned source availability. It preserves values,
coverage and draw gates. /WX build passes, no game/new GPU draw. Tranche849–864
closes; continue865–880 without reset on inactive semantics, current input delivery
and the complete ordinary pass chain. All13 obligations OPEN, milestone unmet.

## Explicit native inactive material contract865–868

[Checkpoint868](../../evidence/I-0066/direct-inactive-material-868.json) defines
owned feature-off values under D-0157/D-0158. Actual equipment PS1 instructions
show zero damage rate and wet weights preserve finite dry inputs. This replaces
implicit shared-arena contents;862/863 still supply no old zero-initialization
proof. CPU867 passes31 assertions, including16 finite arithmetic cases and the
preceding6 active composition checks. Both known-disabled states plus current
generated scalar prepare Ready; unavailable/active missing inputs still refuse.

The consumer is limited to ordinary class1/4, Default pixel policy and pixel b2
with independently matched named float4 layouts. It supplies only damage200,
UV210/220 and wet230..26F; no whole-buffer readiness or missing texture substitute.
Current activation is independent of color availability. Other shader-family
semantics, current game delivery and rendering remain unqualified. Module868
compiles two affected units with /WX; no game/new GPU draw. ER716 selected material,
current copy, indexed emission and token attachment sources were rechecked.
Continue the same selected delivery and executable envelope; used868/880,
all13 obligations OPEN, native-zone milestone unmet.

## Ordinary entry closure869–870

[Checkpoint870](../../evidence/I-0066/direct-ordinary-entry-870.json) rechecks
current selection through preparation, recording and startup presentation.
57A20 C461/ASM140058474–498 reveals the current split-setting read was using
an incorrect RVA, width and predicate. It now reads dword14473CBAC==2 and tests
the selected model mask4 separately. CPU869 passes9 assertions, including
unavailable input and upper-byte counterexamples; module870 compiles with /WX.
Split engine5 stays unsupported. This fixes delivery before a live observation,
without manufacturing pass eligibility or claiming current DS3 values.

The connected ordinary path is ready for a single announced bounded integrated
observation. Existing component controls and current source checks do not prove
actual selected game delivery. Clear/UI only cannot qualify a zone; omitted
families and initial composition policies remain explicit. Used870/880.
