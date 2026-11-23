# DS3 Timing Fixes — LTF / TRTF

Dark Souls III 1.15.2.0 (`DarkSoulsIII.exe`, Steam BuildID 10167187) high-framerate
timing work, moved out of the `DS3 Decompilation` repository on 2026-10-02 so that
repository can focus on the native D3D12 backend. Nothing was discarded: this folder
is a full copy (tracked files plus local, untracked artifacts), and the complete
original repository state is also kept in git under the tag
`archive/pre-cleanup-2026-10-02`.

## What was achieved

| Fix | Investigations | Status |
|---|---|---|
| **LTF — Locomotion Timing Fix** | I-0005 → I-0012 | Causal fix for sprint/roll travel being too short above 60 FPS. Validated live at 165 FPS (I-0009) and ~238 FPS (I-0010); enemy/boss panel accepted at `OBSERVED` strength (I-0012). |
| **TRTF — Terrain Response Timing Fix** | I-0013 | Fixes slope/stair hopping at high framerate. Additive branch validated live (bit-exact at native 60); multiplicative decay branch unvalidated. |
| Runtime FPS control & overlay | I-0011 | Test DLL: HOME overlay, 30–360 FPS target, LTF on/off. "Unlimited" unsupported. |
| Unified test add-on | I-0014 | ReShade-hosted test add-on combining the corrections (windowed mode only). |

Formulas and the required LTF/TRTF separation: [`TERMINOLOGY.md`](TERMINOLOGY.md).

## Contents

- `investigations/I-0005 … I-0014` — contracts, results, status (start with each `RESULT.md`).
- `evidence/I-0005 … I-0014` — all evidence, including runtime captures and build outputs that were never committed.
- `knowledge/CLAIMS.md` — the 55 knowledge-ledger claims about timing (`claims/` holds detail records).
- `decisions/DECISIONS.md` — decisions D-0003 … D-0028.
- `ROADMAP-C4.md` — the former C4 capability section (temporal systems / high framerate).

Links inside these files still use the original repository layout, so links to
`.project/`, `knowledge/LEDGER.md` or other investigations point to the repository
(tag `archive/pre-cleanup-2026-10-02`), not to this folder.

To restore this work into the repository later:

```
git checkout archive/pre-cleanup-2026-10-02 -- investigations/I-0005-* … evidence/I-0005 …
```
