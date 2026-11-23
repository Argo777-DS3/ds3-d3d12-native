# CPU startup boundary: failure analysis

Historical technical record. Current execution and next action: [STATE](../../.project/STATE.md).
D-0156 supersedes the earlier continuation schedules; proof limits remain unchanged.

Two local extension runs failed; implementation retries paused for source analysis under CLAUDE.md.

Run 02 faulted at original RVA 1777A0F before service registration. The D59D40 assembly explicitly supplies allocator argument R8 from global 479C258; the recovered C omits it. Supplying that argument fixed this failure in run 03.

Run 03 registered genuine service key 58 with render-ready false and advanced phase 2 to progress 3. It then faulted at RVA A1F0, which reads renderer receiver +80. Native startup intentionally leaves lower renderer global 4961AB0 zero. D55330/D56780 invokes the lower renderer without checking DrawStep +C8 readiness. D553C0/D56930 likewise performs original renderer end-of-frame operations. Thus the CPU service's scheduler can run frame lifecycle before the resource transaction, despite render-ready false. Attribution to D56780 is supported by source/xrefs, but the recorded exception lacks a call stack; no complete dynamic caller proof is claimed.

Required repair: own frame begin/end GPU responsibilities and retain verified clock/scheduling CPU duties. Do not fabricate a lower renderer object or make A1F0 return a fake handle. Acquire original clock routines and their calling convention before implementing this boundary. Native resources and zone load remain unimplemented.

## Further discriminating evidence

Run 04: first frame GPU lifecycle dependency replaced, new fault at 6E271 in original shader GPU factory. Run 05: fifteen genuine shaders admitted natively, then AC57D dereferenced missing CPU allocator global. D56E90 and B5CD0 assignments establish the heap roles independently of GPU creation.

Run 07 exception module ntdll.dll; captured stack: 1799460 -> D4E16C -> D52822 -> D5209D. D4E100 enters critical section 4793D30. D6EB00 is the omitted CPU initializer for these resource registry locks. Source acquired in cpu-registry-decompilation. Run 08 passed the lock and four resource workers faulted at 2AD59 in 2AD40, an unconditional lower-renderer profiler dereference before GXTexture cleanup. Native texture creation and retirement must be paired; do not make lower renderer global non-null using a synthetic object.

## Texture admission and recurrence

Run 09 paired 2AD40 retirement with 2AF40 direct raw asset admission. It failed at AC89A in AC830 before that direct factory: the loaded source is an actual TPF package, not a raw mip array. Stack includes 2B2AE in 2B260 -> AC830. AC500 constructor assembly preserves TPF pointer at +8, size at +10, selected index at +14; D52640 callers supply the actual package pointer/size. Existing texture_container and dds_resource decode this input. Native 2B260 admission now uses those decoders and the package bank's shared GPU recorder.

Run 10 admitted 66 real textures before AC89A recurred through a different caller, AC15F in AC110. Therefore a second installed texture class enters the same old TPF GPU source method. Its admission/retirement/CPU metadata must be qualified together; no lower renderer object is introduced. The 66 residency receipts certify completed uploads only; native repository completion, shader PSOs and zone rendering remain unproved. After this recurrence, implementation pauses for targeted class layout/lifetime analysis (cube/secondary-texture source acquisitions).

Run11: paired cube path completed all146 startup textures (one cube:32x32,format10,six slices,49152bytes) and11,176 shader admissions. New fault23572B2 has stack D59E20 -> D57230 -> D10630 ->2357210. The latter reads lower-renderer4961AB0 dimensions at+198/+19C while constructing the old nine-view system. Therefore an additional GPU-only startup transaction occurs before D59E90. E8 shader repository had genuine state4 to enter D59E20. This failure is unrelated to any demonstrated logo/menu draw. Required replacement: native view/window ownership through existing SceneRenderer, retain the repository condition and actual completed native outputs, leave old view graph globals null. Do not fabricate nine original contexts. Logs/title/main menu are explicit validation obligations at Thomas's request.

Native12 integrated the existing SceneRenderer and owned D59E20, but refused because an unqualified expected phase7 was hardcoded. Actual journal shows current/progress6. This is a native precondition error, no original access violation. Native13 authenticates the engine table callback address, preserving equal progress/current and clear pending semantics. It advances6 ->7, reaches8 with genuine F0 state4 and completes4,858 native clear frames and native close with faults/refusals/debug errors0. The original view-system crash is removed within this bounded startup scope. Phase8 remains intentionally held; CPU requests, native consumers and logos/title/menu are the next required responsibility.

## Resource continuation, cycles25-28

Native14 emitted the exact eleven genuine CPU requests and preserved phase8
with DrawStep render-ready false. It admitted11,261 shader programs and146
textures, then faulted once in a loading worker at A9696. Stack4FC56A -> D44793
-> D44BF2 -> A9696 joins the Shaderbdle loader4FC370 and CPU-named registry
D44700/D44B80 to mixed effect initializer A9370. The exact failing instruction
loads `[GX global4961B50+4DD0]` during original variant/pipeline construction.
It is not a shader validity query. Genuine decoded CPU programs exist before
this access; missing logo files are not demonstrated as the cause.

Native15 replaces the complete mixed A9370 initializer, retaining real CPU
archive enumeration ACC20 through the six owned shader factories, CPU
declarations and actual CPU containers. Original GPU variant object+28 stays
null. It stopped with controlled refusal `effect_native_program_missing`,
faults0, before admitting an effect bank. After these two unsuccessful runs,
patching pauses for this RCA; no unchanged-cause third retry is authorized.

The native precondition used an incorrect engine-kind -> DXBC-stage mapping:
it swapped hull and geometry. Existing primary [kind/vtable evidence](../../evidence/I-0026/static/GXFLVER-MATERIAL-SHADER-BIND-CANDIDATE.md)
already qualifies engine kinds0=compute,1=domain,2=hull,3=geometry,4=pixel,
5=vertex, whereas DXBC stages are pixel0,vertex1,geometry2,hull3,domain4,
compute5. The correct mapping is therefore **5,4,3,2,0,1**, checked against
the exact factory calls and actual bytecode reflection. This source correction
is discriminating; no renderer global/object or shader stage is fabricated.

The separate replacement owns immutable archive bytes, native shader leases,
engine names and copied declarations; actual CPU parser/container destruction
remains paired. Source A9230 frees CPU declarations/container and conditionally
releases+28; a native retirement hook verifies+28 stays null, withdraws its
registry entry and keeps owned bytes alive through real CPU destruction.
Retained shader leases survive engine registry withdrawal. Native pipeline
readiness, actual main-menu drawing, normal shutdown and zone work remain open.

Thomas's native14 witness saw the game-specific mouse cursor before the crash.
Primary EF46A0 loads cursor resource101 from the game's executable when it
creates the genuine window, before renderer startup. That CPU/Windows path is
preserved. A BitBlt game-window screenshot does not capture the cursor, so the
visual observation is explicitly a human witness, not a screenshot claim.
Both runs restored saves/configuration and the cleaned installed baseline.

## Material boundary, cycles29-31

Native16 passed the corrected stage mapping and admitted113 actual effect
banks,15,527 shaders and146 textures before a new worker access violation at
147D72. Stack147D72 ->2E0E4 ->D43CAC ->D435D5 ->4F7A6C identifies the MTD
loader4F7860, named CPU record factoryD43540 and mixed initializerD43BB0.
2DD60 calls147CA0 to construct an original constant buffer through lower GPU
global4961AB0. The effect failure did not recur; complete Shaderbdle loading
is still unproved because the material worker crash terminates the process.

Targeted material-cpu-ingress / metadata / metadata-owner / retirement sources
qualify the split. D438E0 constructs the genuine0x300 named CPU record with
original GPU material+60 null and embedded CPU metadata+68. D43BB0 first makes
the original0x120 GPU-associated material, then calls CPU decoderD42D50 with
that material as its fifth argument. Native ownership replaces this complete
mixed initializer; it retains raw MTD bytes, exact names and authored values,
and calls only D42D50 with a null original GPU association. D40790 stores that
argument at metadata+290 and otherwise reads actual CPU MTD data. No147CA0
buffer or fake material is constructed.

Original07/08 separately read512-byte loaded CPU ranges, without injection,
after protected on-disk helper decompilation proved unusable. D42F60 withdraws
a CPU subscription; D42FD0 registers one throughD401C0. Loaded callbackD42EB0
explicitly guards metadata+290 before its GPU-associated branch; the remaining
2333A70 notification takes the real CPU shader name. D401C0 constructs a
CPU callback descriptor and registers it with2330DD0, through actual resource
services. Its adjacent acquired ranges are joined at instructionD403BD in a
documented derived record. INT3 padding in the temporary disassembly PE avoids
a synthetic preceding zero instruction consuming an unaligned acquired byte.
The acquired JSON bytes/hashes remain authoritative.

Paired retirementD439A0 skips both GPU release branches when+60 is null and
callsD40540 to unsubscribe and free actual engine strings. Native ownership
withdraws the registry record before that real CPU destruction and retains
the owned bytes through it. The build checks authenticated loaded prefixes
at material admission; encrypted file bytes are never accepted as decoded
behavior. Both original observations restored verified baseline hashes.

Frame challenge: the exact obligation is preserving actual authored MTD data
and CPU lifetimes, required by the existing native material producers. The
old buffer allocation is a replaced-backend detail, not an engine invariant.
This source split unlocks direct CPU admission without reconstructing the old
GPU material class. ER's native CPU/GPU responsibility separation remains the
architectural analogue from the approved startup review; it cannot establish
these DS3 layouts or callbacks and is not cheaper than this bounded DS3 read.
The next discriminating experiment is native17, not further whole-loader
decompilation. Actual pipeline admission and startup primitive work remain open.

## GPARAM ownership and stable loading, cycles32-34

Native17 admitted512 actual MTD materials before faultD2737A. StackD2737A ->
D25DF5 ->4F1AA3 identifies GPARAM loader4F19E0 and mixed initializerD272C0;
the latter unconditionally reads GX4961B50+4788 after CPU name classification.
Source in gparam-cpu-ingress / owner / retirement-body establishes the split.
D26700 constructs the real0x150 CPU record with+70 invalid(-1), fallback+140
null. D27970/D27790/D27BA0 classify actual names and produce genuine numeric
keys without GPU accesses. D272C0's kinds2..5 also allocate/copy raw CPU
fallback bytes via actual heap479C2D8+48, needed by4F19E0's missing-file branch.

Native ownership now decodes real delivered bytes through the existing
native/content/gparam reader, retaining exact file bytes and typed groups,
parameters, IDs and values. It preserves those CPU keys and the actual engine
fallback allocation, but leaves+70 invalid instead of manufacturing a GX
registry handle. Paired retirementD26800 skips its entire GX branch when+70
is invalid, frees fallback bytes through CPU heap lookupEAA9D0 and frees the
real engine name. Native registry withdrawal retains its own bytes through
that destruction. Ordinary game shutdown/retirement delivery remains untested.

Native18 ran to the60-second controlled stop:2,900 native clear frames,
closed1, faults/refusals0, debug-errors0 in every frame. Actual admission totals:
15,527 shaders,113 effect banks,146 textures,512 MTDs and8 GPARAMs. All eight
GPARAM kinds match the requested1,2,3,4,5,12,6,10 order; their groups/parameters/
value counts are retained in the journal. This qualifies CPU delivery/decoding
and bounded clear-only stability, not readiness of shader PSOs, complete
resource workers, ordinary shutdown or UI drawing. The repository driver
capture is black. No renderer-ready/scene-ready flag or phase8 advancement was
published. Save/config/installed cleaned baseline restoration passed.

The native primitives must consume actual CPU geometry, programmes, values and
textures before loading progresses. Missing logos or menu music are still
observable obligations, not an established explanation of these GPU faults.
