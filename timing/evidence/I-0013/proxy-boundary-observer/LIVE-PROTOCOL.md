# I-0013 player proxy boundary live protocol

Status: prepared offline; no live run has yet been performed with this binary.

## Purpose

Acquire one player-only three-second traversal at native target 60 and one at
target 240 through the same slope/stair segment. The observer records the
movement source, proxy-cache exchange and final root without applying a
correction.

## Operator protocol

1. Start the exact supported offline executable and load the agreed route.
2. After the observer reports READY at target 60, stand at the same anchor and
   face the same direction used for both passes.
3. Press F4 once and immediately hold straight movement through the slope and
   stair segment for at least three seconds. Do not jump, roll or attack.
4. Wait for confirmation that the 60 trial ended and was structurally valid.
5. Reposition at the same anchor. After the live target change to 240 is
   confirmed, repeat the same F4 plus straight-movement pass once.
6. Release all input and wait while the observer is stopped and restoration is
   verified. Close the game only after confirmation.

The log defines the exact window; the operator does not need frame-perfect
timing. A displayed 239/240 value is acceptable for the target-240 cell.

## Technical gates

- Verify the exact executable identity, configured target and both correction
  flags off before injection.
- Require READY, exact four-site opcode checks and successful player-only
  filter initialization before asking for F4.
- Flush and structurally inspect the 60 trial before switching the same active
  observer to 240.
- Stop on any pointer failure, malformed/reordered group, overflow, input
  accident or restoration failure. Preserve rejected data; do not improvise a
  retry.
- On stop, require COMPLETE, exact timing restoration from the active target to
  `30/0`, original debug-register restoration and durable raw hashes.

## Interpretation gate

Run `../analysis/analyze_proxy_boundary.mjs` only after both captures pass the
structural gates. Compare the first stage whose vertical trajectory or
source-to-stage gap changes with rate. Do not infer a fix from amplitude alone
and do not introduce any FPS-specific or empirical compensation.
