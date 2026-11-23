# I-0013 safe boundary redesign

## Reason for redesign

The exact `image_base + 0x40DDB120` invalid-execute signature has now repeated
in two high-rate runs of the inherited four-hardware-breakpoint spatial
observer. The current 240 trial is rejected and that observation mechanism is
retired for high-rate I-0013 work.

## Replacement measurement

The replacement retains one code hook only: the already qualified broad-update
boundary at RVA `00D57900`. Once per update, after the original call returns,
it performs guarded direct reads for the current player:

1. `PlayerIns -> modules+0x68 -> +0x80..+0x8C`: movement-source position;
2. `PlayerIns+0x50 -> +0x120..+0x14F`: cached 48-byte transform;
3. the established final player transform `+0x80..+0x88`;
4. the broad-update delta, update index, QPC and exact pointer identities.

The capture is observation-only apart from the already accepted reversible
cadence control. It does not call the protected exchange and does not install a
vectored exception handler or set `DR0..DR7`. It starts inert, stores a bounded
number of fixed-size rows and rejects a trial on any player/identity/read
failure or overflow.

## What this can and cannot decide

A matched 60/240 route can determine whether rate divergence is already visible
in the movement source, first visible in the cached proxy transform, or appears
only at the final root. It cannot reconstruct the intra-update ordering of the
forward/reverse behavior-proxy exchange. If source and cache separate, a later
offline/static or safer single-site discriminator may be needed around
`005C8100` and its cache writers.

The first 240 attempt with the replacement is permitted only after an offline
exact-build/build/schema test, a target-60 structural control, advance operator
notice and explicit Thomas-operated movement. No correction is tested and no
fix is proposed at this stage.

## Fix rule remains unchanged

Any eventual correction must restore a demonstrated mathematical invariant,
such as actual-elapsed-time displacement/velocity integration or a continuous
form of a discrete recurrence. FPS-specific gains, empirical vertical offsets,
terrain thresholds and post-hoc visual clamps remain excluded.
