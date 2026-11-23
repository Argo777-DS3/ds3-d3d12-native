# I-0101 - Les 3 plantages du 27 au soir

Capability: C3

FINISH-LINE line 1 (D-0228). Guided by the study of the evening refusals, copied here as [STUDY.md](STUDY.md) (study thread,
branch `claude/eager-mcnulty-166985`, 4735aa0f; its Ghidra readings were made by Sonnet and are re-read by the pilot before the
code that depends on them).

## Question

Make the three refusals of Thomas's 2026-10-27 evening play go on as the original does: the lost exclusive full screen (Present
887A0001 after a focus loss), the non-finite pose rows at Iudex Gundyr's death, the unowned model method (+0x38) at Archdragon
Peak. And replace « refuse and close the game » by « skip the object and go on » where the native module meets transient game
data the original tolerates (FABLE-REVIEW §4.2, FINISH-LINE line 1).

## Scope

Included:

- STUDY §3 (full screen): a failed Present not fatal unless the device is lost; ER's frame-start follow of the full-screen
  state (ResizeBuffers after DXGI leaves exclusive full screen); the minimise both games do; the focus state lifted again.
- STUDY §1 (pose): the refusal removed, the palette layer's skip kept, row 0 checked, one rate-limited diagnostic row.
- STUDY §2 (model method): slot 7 (and the slots its family meets) of the native material instance's table, returning a
  native GX MTD image whose +0x48 is the shader stem; a diagnostic at the first call.
- The rule « skip the object and go on »: a triage of the module's refusals on per-object game data, the ones that can skip an
  object and go on converted, the rule written in WORKFLOW.
- Unit tests for each fix; one announced run on exclusive full screen with focus losses (Thomas never tried Alt-Tab in full
  screen: the pilot checks it). Amended under D-0229 (Thomas, 02:48 and 03:33): no second run; a one-minute start check after the merge.

Excluded: the burn's look (a ROADMAP row), the NaN's producer, the exit fault (I-0099, line 2), the OLED (never without Thomas).

## Inputs

- Observations: I-0100 [RESULT](../I-0100-popin/RESULT.md) « Thomas's evening play »; the journals the STUDY names.
- Study outputs: `evidence/I-0100/evening-faults-study/` (`present/`, `pose/`, `method/`).
- Decompilation: the mirror `ghidra-mirror-i0066` (read-only), `decompiled/`.

## Exit conditions

- Success: the three fixes and the rule merged into `main` with their unit tests, the final review answered; the Alt-Tab run in
  exclusive full screen recovered every loss (run 1), its quit's refusal fixed and not run (D-0229).
- Refuted: a re-read contradicts the STUDY's reading a fix depends on; the fix is rewritten from the pilot's reading.
- Insufficient information: a fault that cannot be reproduced keeps its fix and its diagnostic row; said in the RESULT.
- Stagnation: two failed local fixes on one fault open an RCA.

## Safety and human interaction

Live runs are announced, offline, with a hash-checked restore (D-0116, D-0117), on the ASUS; never the OLED without Thomas. Test
characters keep their progress; Alpha is never overwritten.
