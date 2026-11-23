# I-0013 broad-update stage observer

This is the safe replacement for the retired four-hardware-breakpoint observer.
It installs one MinHook detour at the already qualified broad-update RVA
`00D57900`. After each original update, an armed capture reads the current
player's movement source, cached 48-byte proxy transform and final root.
The cached transform uses matrix translation lanes 3/7/11; all 12 lanes are
also preserved bit-for-bit so the layout can be rechecked offline.
The cycle-13 extension additionally records cache-owner `+0x2E0..+0x2F0`, the
exact five-field state that can override vertical translation lane 7. This is
sampled by the same broad-update hook; no second hook was added.

The cycle-14 offline analyzer reports cache update cadence, source/cache lag
diagnostics and route-normalized opposing vertical steps. In the reproduced
239.770-Hz trial it localizes a strong cache-only divergence while the override
gate remains inactive. See `CACHE-RECURRENCE-DISCRIMINATOR-RESULT.md` one level
above. The observer remains evidence infrastructure, not a correction.

It deliberately contains no vectored exception handler, thread-context write,
debug-register use or call into the protected behavior-proxy exchange. The only
runtime write is the already accepted reversible `+0x354/+0x358` cadence pair.
The original pair is restored exactly at stop.

The observer starts at target 60 and remains inert until F4. Each capture is
bounded to three accumulated seconds and 2,048 rows. Any invalid read, pointer
identity change, update gap detected by analysis or overflow rejects the trial.

`build.ps1` verifies the exact executable hash, hook bytes, pinned MinHook
revision, forbidden-mechanism absence, imported DLL set and analyzer tests. Its
result is offline qualification only; it does not authorize or prove a live
run.
