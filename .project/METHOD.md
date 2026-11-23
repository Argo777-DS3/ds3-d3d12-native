# Method: investigations, coverage by families, the frame challenge

Three sections of WORKFLOW moved here on 2026-11-15 (D-0291; their earlier wording in
[archive/2026-11-15/WORKFLOW.md](archive/2026-11-15/WORKFLOW.md)), read when an investigation opens, a coverage
inventory is built or a design gate comes; they are rules in force like WORKFLOW's. [WORKFLOW](WORKFLOW.md) keeps the rules
every turn needs.

## Investigations

Open with `scripts/New-Investigation.ps1 -Id I-NNNN -Slug name -Title "..." -Capability C3 -Owner "..." -Budget N`, then complete
`CONTRACT.md`: one precise question, scope in and out, known inputs and claims, discriminating evidence, exit conditions
(success, refutation, insufficient information, stagnation, budget) and a bounded budget. `status.json` records owner, state
(`planned`, `active`, `blocked`, `complete`, `abandoned`) and budget; one investigation, one owner. Count each meaningful
experiment or analysis cycle in `iterations_used`. A workstream in tranches stays `active` until its last tranche merges; at a
budget limit, close the tranche with a checkpoint, record the next bounded plan and extend `iterations_max` (never reset
`iterations_used`); continue only within unchanged authorized scope, never to hide stagnation. Raw outputs go under
`evidence/<id>/artifacts/`, or at an external location recorded with its hash in the manifest. Only the pilot assigns claim
statuses, never because a tool or worker asserted one. An experiment that cannot discriminate records *insufficient
information*. Close with `RESULT.md`: conclusion, exact evidence references, uncertainty, artifacts, optional next step; then
decide on ledger, decision and STATE updates, and run `scripts/Test-Project.ps1`. A series runner is written once per need
(D-0217): copy an earlier `Run-I*Series.ps1` only when its measure differs, and keep the earlier ones as the record.

## Coverage by families (Thomas, 2026-10-06, D-0150)

For every major subsystem, build an offline coverage inventory first: group assets, events and resources into the families that
matter to the implementation, map each family to in-game witnesses, and test family by family. A live discrepancy outside the
inventory names a missing family: add it before fixing it. Native production is written per family or engine mechanism (MTD
family, material class, pass row, pose owner, constant producer), never keyed to one asset; a fix that serves one asset is a
defect (D-0161). Graphics settings are one more axis: list offline what each changes (passes, programmes, target sizes,
parameters); a change of size or parameter is covered by the qualified input and parameter contracts; programmes or passes the
ultra runs never drew are credited by one run at the lowest settings whose drawn programmes are compared with the ultra runs'.
Resolution, aspect ratio, display mode (windowed, borderless, exclusive full screen), later HDR, internal scaling and upscaling are
inputs each frame supplies (D-0151): the renderer holds no size, ratio or mode of its own, and a fallback to one is a defect.
List offline what depends on each (the 128-pixel light tiles, half- and quarter-size targets, borders, screen-size constants,
shadow and atlas sizes, the presentation window), then check it live at sizes chosen to break hidden assumptions (not multiples
of 128 or 4, another aspect ratio) and once per display mode.

## Architectural frame challenge (anti-tunnel)

No native mini-workstream is chosen by successive crashes (D-0156): work follows the design and the [ROADMAP](ROADMAP.md); a
crash informs the design. Before a large tranche, a static-to-runtime step, a declared implementation readiness, a
high-consequence seam, or when one blocker persists, the pilot answers: what exact fact is sought and why the objective needs it;
whether it is an engine invariant, a historical detail, a compatibility constraint or something the replacement may define;
what decision proving it unlocks; whether recent results reduced the blocker or only added detail; whether a smaller experiment
would answer more directly.

| MUST RECOVER / PRESERVE | MAY BE REDESIGNED | UNKNOWN |
|---|---|---|
| Required game behaviour, DS3-specific data, layouts and ABIs wherever consumed, ownership and lifetime guarantees needed for direct current input delivery. | Engine-side architecture and internal contracts following the recovered ER method (D-0157), and backend mechanics: synchronisation, retirement, allocators, queues, descriptors, fences, caches. Existing boundaries, investigation transports and capture joins do not constrain the production interface. | Not yet established. |

Outcomes: the frame confirmed, an obligation reduced, static diminishing returns, implementation is the better experiment
(surface it to Thomas), the seam must move, or the question was misframed. Do not reverse-engineer a historical mechanism merely
because the original engine used it. Before a D3D12 seam, ownership boundary or backend contract, recover the ER mechanism that
does the same job (the first rule), state what DS3 changes and which DS3 data it consumes, and record when ER has no analogue.
Same method, same architecture, different data, with explicit engine control of D3D12 resources, commands, synchronisation and
lifetime (D-0157). The ER study's tools and sources are in the
[mandate](NATIVE-D3D12-RESET-MANDATE.md#elden-ring-method-study); the ER folders stay read-only.
