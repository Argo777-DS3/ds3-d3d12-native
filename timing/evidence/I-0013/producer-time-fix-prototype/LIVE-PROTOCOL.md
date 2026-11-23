# I-0013 producer time-law live protocol

After offline qualification and checkpoint, start DS3 offline/windowed at the
same route and inject the prototype once.

1. At target 60 with correction ON, press F4 and traverse for three seconds.
   Acceptance requires zero applied writes and bit-exact native passthrough.
2. Return to the same start, select target 240 with correction ON, press F4 and
   repeat. Acceptance requires exact formula reconstruction, no guard failure,
   and removal of the familiar hopping without introducing a new anomaly.
3. Stop immediately, verify correction disable, exact timing restoration and
   hook removal, then close DS3 when requested.

No correction-OFF repeat is required: the immediately preceding accepted
producer-state session supplies the exact 60/239.597 baseline with the same
route, identity and state boundary.
