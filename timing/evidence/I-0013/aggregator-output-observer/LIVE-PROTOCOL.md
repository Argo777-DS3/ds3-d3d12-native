# I-0013 aggregator-output observer live protocol

Run only after offline qualification, a clean Git checkpoint and advance notice.

1. Start DS3 offline in the validated standard windowed configuration and stand
   at the same slope/stair route.
2. Inject the exact qualified observer; confirm ready at target 60.
3. Press F4 once and traverse normally for the bounded three-second capture.
4. Stop on any invalid row, unexpected behavior or instability.
5. After the 60 control passes, switch the same process to target 240, return to
   the same start and repeat once.
6. Stop the observer, verify exact timing/hook restoration, then close DS3.

Thomas controls movement and reports visible hopping. The observer applies no
fix and does not test or assume collision ownership.
