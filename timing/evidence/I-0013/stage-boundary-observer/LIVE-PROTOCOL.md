# I-0013 safe stage-boundary live protocol

Run only after the offline checkpoint and advance notice.

1. Start DS3 offline in the already validated standard windowed configuration
   and stand at the same slope/stair route.
2. Inject the exact qualified stage observer. Confirm its ready signal and
   target 60; do not enable the locomotion correction or any candidate fix.
3. Press F4 once and traverse the route normally. The capture stops
   automatically after three accumulated seconds.
4. Stop and analyze immediately if the 60 control has any invalid row, update
   gap, overflow, abnormal behavior or process instability.
5. Only after the 60 control passes, switch the same process to target 240,
   return to the same start, press F4 once and repeat the route.
6. Stop the observer, verify exact timing restoration and close DS3 normally.

Thomas controls movement and reports whether the hopping was visible in each
cell. A crash, hang or abnormal response ends the protocol; do not relaunch or
repeat without a new offline audit. The trial compares only source, vertical
override state, cache and final-root behavior. It does not test a fix.
