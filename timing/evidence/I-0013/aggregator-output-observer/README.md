# I-0013 aggregator-output boundary observer

This is the final bounded discriminator prepared after the base-transform
builder was cleared. It installs one exact-build hook at `005C8100`, retains
only the current player's cache owner and the three statically known caller
returns (`005C6977`, `005C6F47`, `005CAA80`), and calls the original function
exactly once.

Each retained call records source position before/after, prior cache, returned
48-byte output, carried scalar `+0x250`, modes `+0x254/+0x258`, vertical filter
`+0x2E4`, override gate `+0x2EC`, and the reconstructed optional-transform
predicate. The original game path naturally invokes protected `00D9A3C0`; the
observer adds no call to it and no engine write beyond the accepted reversible
cadence pair.

The capture is limited to 4,096 rows or three wall-clock seconds, fails closed
after five seconds, and restores timing plus the single hook on stop. It uses
no debug register, vectored exception handler, thread-context write, fix, clamp
or FPS-specific compensation.

If final output oscillates, later cache writing/consumption is cleared and the
predicate state helps split the protected versus optional-local stage. If final
output remains monotone while prior cache oscillates, the writer/consumer side
is selected. Neither outcome alone assigns collision/terrain semantics.
