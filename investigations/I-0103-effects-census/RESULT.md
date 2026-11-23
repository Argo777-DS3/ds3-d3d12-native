# I-0103 result: the effects' census

## Conclusion

Complete (2026-10-28, FINISH-LINE line 3, static, no live run). The census is [CENSUS.md](CENSUS.md).

- The game draws its effects through 12 FXR action kinds; one engine registry of 22 types (read at `FUN_14024B260` and
  `FUN_14036D9A0`) maps each kind to one renderer class, so the native's allowed list of classes settles every family.
- Drawn natively: 602, 603, 604, 607, 608 and the GPU particles 10000 (OBSERVED), 600 (INFERRED: allowed, never skipped), 601 allowed and never witnessed: 87 % of
  the drawing occurrences. Skipped and used by the game: E03 model particles 605 (752 FXRs, every bundle, its flag faulting),
  606 (212 FXRs), 10012 (125 FXRs; OBSERVED skipped at m30, the High Wall, in Native1968's journal, which FFX-DRAW had missed), the light shafts 10003 (3 FXRs in m30, m33, m38): 13 % of the drawing occurrences. The effect spot lights 11000 wait
  for line 6.
- FINISH-LINE's nine skipped classes: three draw and are used (606, 10012, 10003), 10301 is a force field that draws nothing,
  five are never used by the game (10002, 10008, 10200, 10302, 10600).
- Outside the FFX list: the decal materials' class, the Burn MTDs' look, the blood's fade, the translucent NPC phantom, the
  surface snow rows never produced, four E01 correctness points.
- Corrected: the m37 snow is drawn natively since Native2021 (STATE named it from FFX-DRAW's earlier « Open »).
- Leads (INFERRED): the Estus Flask in the hand and Lightning Spear and White Corona drawn wrong fit the families skipped in
  `commoneffects`, 605 first.
- I-0102's leftovers: the producers' capture layer is kept for line 4 (it recomputes slot 27's effect blocks: reusable
  knowledge), the scene renderer's stage observer is dead and goes in line 4's first tranche.

H1 holds with one correction (I-0063's families did not list 10300 and 10301: they draw nothing). H2 stays INFERRED until line 5
reads the drink's FXR or one announced run counts the skipped class. H3 holds in part: 10003 exists only in m30, m33 and m38; m33 and m38 were never seen natively, and where m30 places its shaft is not read (m30 was compared, FFX-DRAW 1968, 2048).

## Evidence

- `evidence/I-0063/artifacts/scan/fxr.tsv` (I-0063's whole-game scan), summed by [Get-FxrActionCensus.ps1](Get-FxrActionCensus.ps1)
  into `evidence/I-0103/census/actions.tsv` and `actions-by-bundle.tsv`.
- The decompiled registry and constructors (Thomas's Ghidra 12.1.4, DS3 copy `ghidra-mcp-ds3`, read-only), the addresses in
  CENSUS §2; the native's transcription `native/frame/effect_system.cpp` `types[]` agrees slot for slot.
- The native allowed list: `native/frame/effect_draw.cpp` (`Execute`) at `main` `85ac5564`; the skipped counts in the
  `i66-native-20xx` journals (`evidence/I-0066/artifacts/`).

## Uncertainty / unresolved questions

- Weights count FXR files, not screen time or placement.
- Which FXRs the Estus drink, Lightning Spear and White Corona use is not read (the regulation and the animation events were
  not opened); the smallest settling run is in CENSUS §6.
- The surface snow's native gap is unknown until the draw parameters' snow rows are read or m38 is seen natively (line 8); m30 was compared without a snow difference noted.

## Artifacts produced or modified

- `CENSUS.md`, `Get-FxrActionCensus.ps1`; `evidence/I-0103/census/`.
- Ledger K-0485; STATE, FINISH-LINE (line 3 done, line 5 rewritten from the census), ROADMAP, the investigations index.

## Review

The final review (a fresh Opus at xhigh, `git diff main...HEAD`) found no high: the registry table matched the native's
`types[]`, the decompiled exports and FFX-DRAW's class census; every weight recomputed from the raw scan, the script reproduced
the census files byte for byte. Answered: M1, the journal sentence was false: 10012 was counted skipped at m30 in Native1968
(checked by the pilot in that journal), now OBSERVED; M2, 600's « drawn » rested on a creation count, now INFERRED, and the snow
no longer attributed to an action kind; M3 and M4, FINISH-LINE's line 5 and its limits, and STATE's « effects (E01-E07) »,
rewritten from the census; the lows (the classic registrations' count, the lights used, distinct FXR counts, the per-frame
producers, 10003 at m30, the `prefix_refused` condition) corrected. L9, `effect_draw`'s counters running outside their flag, is
given to line 4 (CENSUS §7).

## The merge

Merged into `main` as `0b9e10e6` (2026-10-28 14:16). The launcher's rebuild and the one-minute start check were not run:
the branch changes no file under `native/` or `scripts/` since `ce379c84`, whose rebuild and start check (`start-ce379c84`)
passed, so the module would be the same; the previous pilot's process was also still open (no build or run until it closes).
The full control's gate record left uncommitted in the main checkout (`REPLAY-GATE.json`, `full-1028-gate`) landed with it.

## Suggested next step

FINISH-LINE line 4, the native effect method (D-0190), in a new investigation: Elden Ring's effect rendering read first, then
the base on E01 with the correctness points, then the migration (CENSUS §7).
