# DS3 native D3D12 programme mandate

Authority: Thomas, Project Director. D-0084 (2026-09-30), amended by D-0115,
the 2026-10-07 independent-startup instruction, D-0155, D-0156 and D-0157.
It replaced the project's earlier execution plans (archived, `archive/pre-cleanup-2026-10-13`). Historical results
retain their proof limits; current operating and delegation rules belong to [WORKFLOW](WORKFLOW.md). The sections below
keep Thomas's decisions as written: Astra there is today's Fable 5.2 or, by default, Opus 5.5 at high effort (D-0114, D-0198).

## Target and D3D11 policy

**Current DS3 engine data → integrated autonomous native D3D12 backend → native presentation.**

The backend owns initialization, device/queues, resource creation and updates,
identity, views/descriptors, shaders/root signatures/PSOs, geometry and pass
preparation, render targets/depth, recording/submission, barriers, synchronization,
retirement, composition, presentation/resize, failure handling and teardown.
Recover required engine semantics; replace backend mechanics with native contracts.

The final production build renders the complete normal game, including DLC, with
the original D3D11 renderer unavailable and ultimately removed. Production
cutover is binary: original D3D11 or D3D12-only. Unknown coverage blocks release.
No required graphics bridge, per-draw round trip, D3D11On12, forwarding wrapper,
call translation, D3D11-owned GPU resource or rendering fallback.

D3D11 remains a behavioral reference and decompilation source. Separate reference
runs, captures and comparisons are development tools. A responsibility's native
replacement must exist before legacy removal; stubs, missing imports and crashes
are not replacements. Real game resources and current inputs are required;
synthetic substitutes and fake legacy readiness do not establish integration.

## Current execution and validation mandate — 2026-10-07

Status (2026-10-21): the real game starts, loads saves and plays on the native backend at the witness zones ([STATE](STATE.md)); Thomas accepted this first milestone with reserves on 2026-10-19 (D-0194, « jeu jouable en natif »), wave 1 is closed and wave 2 (the remaster) is current; the final acceptance below is unchanged. The text below is the mandate as written.

I-0066 must make the real DS3 process start, load one real zone and render it
independently through native D3D12. Initial visual defects, missing effects and
incomplete behavior are acceptable when recorded explicitly. Clear-only output,
an isolated fixture or host replay does not satisfy this first milestone.
Logos, title/main menu, cursor and main-menu music are explicit startup
obligations. D-0153 requires all supported text/menu languages and glyph families;
Thomas selected English for current live tests, without reducing that coverage.

Compare separate original D3D11 and native D3D12 executions of the same zone under
reproducible conditions. Native frames must not depend on the original renderer's
GPU work, resources or presentation. This incremental milestone does not claim
complete-game acceptance. I-0065 stays closed; its cancelled later witness/fix
stages are not resumed automatically.

## Direct native engine integration — Thomas, 2026-10-08 (D-0155)

The project is in final native integration. Building adaptation scaffolding is
over: complete the direct backend and replace transitional runtime dependencies.
The engine supplies current inputs directly, as in a game designed for D3D12.

Reuse existing native producers, resource owners, command encoders, pass algorithms
and presentation components. Adapt their entry/resource/lifecycle contracts;
do not preserve a permanently grafted host, capture join, virtual-file publication
or investigation adapter as the production interface. Existing scaffolding may
serve bounded diagnostics. Preserve useful code, evidence and tools; historical
qualification stays limited to the execution that established it.

Before implementing a responsibility, classify its existing work as directly
reusable component, reusable knowledge/evidence, diagnostic tool, obsolete
coexistence mechanism, or unknown pending evaluation. Preservation does not imply
retaining obsolete runtime dependencies.

## Methodological restart — Thomas, 2026-10-09 (D-0156)

Status (2026-10-19): study716 is complete and the direct connection design was restarted from it ([DIRECT-CONNECTION](../investigations/I-0066-independent-native-startup/DIRECT-CONNECTION.md)); the rule against crash-driven mini-workstreams stays in force. The text below is the decision as written.

**Stop mini-workstreams driven by each native crash. Wait for Astra Medium's
deeper Elden Ring study716, then verify its sources, apply the recovered method
and restart the direct connection design from zero.** Study710 is intermediate
and does not fulfill Thomas's request for deeper analysis. Earlier automatic
next-patch/test plans are superseded; no crash-driven native patch/build/live loop
while the study is pending.

The new design must cover current engine producers and selected instances/materials,
native resource creation/updates, preparation, ordered commands, submission and
completion, lifetime and presentation. Reevaluate existing components against
this coherent architecture instead of carrying the current hooks forward by
default. Preserve code, evidence, failures and provenance. Crashes can inform
the design; they must not choose the next development stage.

## Elden Ring method study

**Wave 2's first rule — Thomas, 2026-10-20 (D-0195):** the native backend copies the architecture of Elden Ring's D3D12
engine, recovered by deep decompilation, and improves it later; only the data differ (DS3's own effects, parameters,
models and shaders, verified on DS3); the existing work is transformed, not thrown away ([WORKFLOW](WORKFLOW.md#first-rule-of-wave-2-the-architecture-of-elden-ring-thomas-2026-10-20-d-0195)).
Tools: Elden Ring 2.7.1 analysed in `C:/Users/User/Tools/ER-Decompilation/ghidra/ER_2_7_1_25080141_I0001` (read-only
headless Ghidra `-process -readOnly -noanalysis` on a copy of the project, as ER-DEEPENING-897).

**Architectural reference — Thomas, 2026-10-09:** the complete
[deep study716 (made by Astra at medium effort)](../investigations/I-0066-independent-native-startup/ER-ENGINE-D3D12-DEEP-STUDY-716.md)
is the governing ER method reference for this programme. Thomas identifies the
original `C:/Users/User/Desktop/DS3_evidences/I-0066/ER-ENGINE-D3D12-DEEP-STUDY-716.md`;
the Git copy preserves its full scientific body, provenance and primary-source
routes. Consult its relevant recovered chains and stated limits before choosing
or changing direct engine/backend architecture, resource ownership, preparation,
submission/completion or retirement. Principal source check730 and the restarted
DS3 design are in [DIRECT-CONNECTION](../investigations/I-0066-independent-native-startup/DIRECT-CONNECTION.md).
The report is architectural evidence, not DS3 layout/ABI proof or a complete ER
lifetime guarantee; retain its unresolved boundaries when applying the method.

Seriously study FromSoftware's engine-to-D3D12 connection through actual ER
decompiled functions and call chains: producer data, selected resources/materials,
preparation, commands, submission/completion, retirement and presentation.
Advisory summaries alone are insufficient. Before consequential seams, inspect
the nearest recoverable ER analogue and state what it discriminates and its limits.

### Engine architecture authorization — Thomas, 2026-10-09 (D-0157)

**Same method, same architecture, different data.** Thomas explicitly authorizes
modifying DS3's engine architecture, using access obtained through decompilation,
to organize its direct engine-to-D3D12 connection like the recovered ER architecture.
This includes engine-side producers, preparation, resource ownership and lifecycle
contracts; the existing DS3 engine/backend boundary is not a constraint to preserve.
Apply the source-checked ER method coherently across that chain and reuse existing
native components under the resulting contracts.

Thomas further clarifies that this redesign must give the engine explicit
D3D12 control of resource states and updates, command preparation and ordering,
submission, synchronization and GPU lifetime. Make these responsibilities visible
in the engine's native contracts, following the recovered ER organization, instead
of retaining opaque assumptions from the former D3D11 service boundary. The
authorization covers the engine changes needed for that control, not only the
backend implementation.

Thomas's restatement for Claude on resumption, 2026-10-09: the withholding
method is abandoned as a development route; it built the mature base. The target
is an autonomous D3D12 renderer that uses the documented ER method as its
reference, with freedom of application rather than a restart from nothing.
Prefer changing the engine architecture over grafting pieces onto a design made
for D3D11. Keep the rendering work acquired during the withholding period.

ER gives the architecture (D-0195); DS3's data are verified on DS3. Keep DS3's own assets,
values and behavior; independently establish its layouts and ABIs wherever the
implementation consumes them. Do not transplant ER offsets or assume identical
structures. Internal engine contracts may be redesigned to achieve this architecture
while preserving required game behavior and direct current input delivery. This
authorization extends D-0155/D-0156; it does not establish rendering qualification.
Keep decompilation targeted to needed responsibilities, not whole-game reconstruction.

Thomas permits Astra to launch and drive ER when a runtime observation clarifies
the method. Before each launch announce a bounded protocol, run offline, back up
protected files, hash-check restoration and preserve installed files. Static and
runtime witnesses have separate proof limits. DS3 live rules remain in WORKFLOW.

## Design deliverables and acceptance

1. Direct responsibility map: current engine inputs, selected intent, native
   resources, commands, completion and output; required engine semantics versus
   replaceable backend internals.
2. Source-checked ER comparison and independently supported DS3 contracts,
   including unresolved obligations and refusal behavior.
3. Chosen coherent native entry and ownership boundaries, with explicit previous
   work reuse and eliminated transitional dependencies.
4. Compilable integrated implementation using existing native components, followed
   by the independent real-zone milestone above. Architectural understanding alone
   is insufficient completion.

Each acquisition must unlock a native implementation decision. At major tranches,
identify the newly owned responsibility, remaining dependencies and executable
capability. If progress only grows coexistence machinery, reframe under WORKFLOW.
Full production acceptance remains unmet; the 13 obligations are tracked in
[STATE](STATE.md#coverage-of-the-13-production-obligations).

I-0035 remains historically blocked119/119, `phase1_success=false`, with no
selected native draw. Its exceptional preservation merge did not qualify Phase1.
Historical decisions and transfers are recorded in [decisions/LOG](../decisions/LOG.md);
they are not current instructions to reopen old investigations or restore evidence.
