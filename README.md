# Dark Souls III — native D3D12 backend

Reverse engineering of Dark Souls III 1.15.2.0 toward a fully native D3D12
backend, from engine inputs through presentation. Decompilation is targeted at
the systems this replacement needs; exhaustive whole-game reconstruction is out
of scope.

| Folder | Content |
|---|---|
| [`native/`](native/README.md) | Native D3D12 backend source (C++) |
| [`decompiled/`](decompiled/README.md) | Ghidra exports of the studied game functions |
| [`investigations/`](investigations/README.md) | Bounded work units (contract, status, result) |
| [`evidence/`](evidence/INDEX.md) | Proof records and manifests (external store, through an ignored junction) |
| [`knowledge/`](knowledge/LEDGER.md) | Canonical technical claims |
| [`decisions/`](decisions/LOG.md) | Consequential decisions |
| [`timing/`](timing/IMPORT.md) | The high-framerate timing project (LTF/TRTF), imported by D-0196 with its own historical numbers |
| [`.project/`](.project/INDEX.md) | State, roadmap, mandate, workflow |

Start with [`.project/INDEX.md`](.project/INDEX.md), then follow its resume route
to the current checkpoint in [`.project/STATE.md`](.project/STATE.md). Wave 1,
I-0066 « DS3 DX12 Vague 1 », is closed: DS3 starts, loads saves and plays with the
native D3D12 renderer and no D3D11 device, the first milestone accepted by Thomas
with reserves (D-0194). Wave 2, the remaster « Dark Souls III: Rekindled », is
current and follows the architecture of Elden Ring's D3D12 engine (D-0195).
Production acceptance is not reached: all 13 obligations stay open in STATE, and
historical results never establish current qualification by themselves.

The high-framerate timing fixes (LTF/TRTF) left this repository on 2026-10-02 and
came back under [`timing/`](timing/IMPORT.md) on 2026-10-21 (D-0196: documents and
decompilation; the raw captures stay in the Desktop folder `DS3 Timing Fixes (LTF-TRTF)`).
Earlier states of the project files are archived in Git tags (`archive/pre-cleanup-*`,
listed in [`.project/ARCHIVE.json`](.project/ARCHIVE.json)).
