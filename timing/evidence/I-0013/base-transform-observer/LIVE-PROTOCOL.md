# I-0013 base-transform boundary live protocol

Run only after the offline build/checkpoint and advance notice.

1. Start DS3 offline in the validated standard windowed configuration and
   stand at the same slope/stair route.
2. Inject the exact qualified base-transform observer. Confirm ready and target
   60. No locomotion correction or candidate terrain fix is enabled.
3. Press F4 once and traverse the route normally. The observer retains only the
   current-player `005C8100 -> 009D97B0` calls and stops after three wall-clock
   seconds.
4. Stop immediately if the 60 control has an invalid row, unexpected caller,
   overflow, abnormal behavior or instability.
5. Only after the 60 control passes, switch the same process to target 240,
   return to the same start, press F4 and repeat once.
6. Stop the observer, verify exact timing/hook restoration, then close DS3.

Thomas controls movement and reports whether hopping is visible. Any crash,
hang or abnormal response ends the protocol. This measures one pre-protected
transform boundary; it applies no fix and does not test collision ownership.

Executed once in session `20260921T054031Z-pid27432`: the 60 control was
normal, hopping reproduced at 239.940 Hz, and timing/hook restoration plus game
shutdown were confirmed. Do not repeat this accepted acquisition by default.
