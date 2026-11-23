# I-0013 carried-producer provenance observer

This one-hook `005C8100` observer preserves the accepted output capture and adds
only ordinary memory reads for the concrete owner virtual target at slot `+0xE0`
and actor virtual target at slot `+0x178`. Targets and vtables are exported as
module-relative addresses; the actor pointer remains an identity sentinel.

It calls the original aggregator exactly once, invokes neither virtual target,
applies no correction, and uses the accepted reversible cadence pair only. A
single bounded target-60 traversal is sufficient to resolve stable concrete
implementations for read-only decompilation; no 240 repeat is planned unless
the targets themselves vary unexpectedly.
