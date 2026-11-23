# I-0013 player proxy boundary observer

This observation-only derivative reuses the exact I-0007 four-site spatial
exchange boundary. It adds one fail-closed filter: rows are retained only when
`SprjBehaviorCharacterProxyDriver+0x90` equals the current `PlayerIns` pointer
at `WorldChrMan+0x80`. Enemy owners are ignored before the row counter advances.

The filter reduces the expected three-second capture to about 720 rows at 60
and 2,880 rows at 240, below the inherited 4,096-row per-trial guard. It keeps
the same four execute breakpoints, pointer guards, exact-build byte checks,
debug-register restoration and timing restoration. The only target change is
165 to 240. Both correction paths remain forbidden by the configuration gate.

`build.ps1` verifies the exact hashes of the inherited I-0007 sources, generates
temporary derived translation units beside those sources, builds into this
directory, and removes the generated source files in `finally`. It never edits
the canonical I-0007 files.

Each retained site also samples the statically traced `modules+0x68` current
position, the conditional `+0x3CC` lane-1 term and selector state, and the
current player-root position. The intended discriminator is therefore the
within-update vertical relationship among movement source, selected cache,
forward-derived block, reverse-selected block, current player-proxy block and
final root. It locates the first divergent boundary; it is not a correction
and does not authorize an empirical vertical offset or an FPS-specific
multiplier.

`../analysis/analyze_proxy_boundary.mjs` rejects malformed CSV rows, incomplete
or reordered four-site groups, invalid reads and non-finite blocks. It reports
stage gaps and consecutive-delta correlations without assigning collision
semantics to an address merely because it lies on this dataflow.

No live claim follows from a successful build. A run still requires an
announced operator protocol, READY/exact-build checks, bounded trials, STOP,
COMPLETE, exact timing/debug restoration and durable raw hashes.
