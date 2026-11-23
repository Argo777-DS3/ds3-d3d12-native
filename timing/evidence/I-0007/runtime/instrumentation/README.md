# I-0007 selected-candidate write watch

This exact-build probe is derived from the already validated I-0006 runtime
framework. During the accepted sprint trial it resolves the canonical
`transform +0xA8 -> outer +0x40 -> nested +0x70` address, then arms three
per-thread four-byte write-only hardware breakpoints for one bounded update
window. Each hit records the post-instruction RIP, candidate values, registers,
SIMD state, and nearby code bytes. The probe never writes the candidate or the
final-writer output.

The live hook surface is limited to the broad update/timing anchor, Present,
and the already qualified dispatcher pair used to arm and disarm. Configuration
is disabled by default. Build with `build.ps1`; validate exact debug-register
restoration offline with `test_write_watch.ps1`.
