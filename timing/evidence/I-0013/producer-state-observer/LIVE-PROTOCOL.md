# I-0013 producer-state live protocol

After offline qualification and checkpoint, start DS3 offline and windowed at
the established slope/stair route. Inject the exact observer once.

1. Select target 60, press F4 once, and traverse the route normally for the
   bounded three-second window.
2. Return to the same start marker.
3. Select target 240, press F4 once, and repeat the same route.
4. Stop the observer immediately. Verify exact timing restoration, hook removal,
   complete captures, stable targets, and zero invalid/overflow rows; then close
   DS3 when requested.

Report only whether each traversal looked normal or hopped. Do not compensate
the route or change correction state. The observer is read-only apart from the
already accepted reversible cadence pair and starts inert.
