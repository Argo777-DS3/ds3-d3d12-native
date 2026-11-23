# Carried producer provenance result

## Accepted live observation

Session `20260921T063407Z-pid28408` completed one bounded target-60 route with
181/181 valid rows, 60.005875 sampled calls/s, stable current-player identity,
and no invalid or overflow row. Thomas reported normal behavior. Shutdown
removed the hook and restored the timing pair exactly before DS3 was closed.

Ordinary vtable reads resolve the two dynamic calls accumulated by `005C75F0`:

- owner virtual slot `+0xE0` -> RVA `005E6580`;
- actor virtual slot `+0x178` -> RVA `008BC9A0`.

Both targets and the actor pointer are stable in all 181 rows. The observer did
not call either target and applied no correction.

## Exact-build reconstruction

`008BC9A0` is a bounded getter returning the pair stored at actor `+0x2038`.
At `005C75F0`, its second pointer selects a float at `+0x1C`; a null pointer
contributes zero.

`005E6580` maintains a float at owner `+0x2DC`. Its relevant exact-build paths
are:

- reset the state to zero when an upstream predicate is false;
- on a qualified response, update `state <- state + 0.2 * min(a, b)` where the
  available response determines `a`/`b`;
- otherwise update `state <- retention * state`, with retention at owner
  `+0x378`;
- clamp the stored result to `[-0.5, +0.5]` and return it.

The constants are exact float32 values (`0.2`, `+0.5`, `-0.5`, and
`FLT_MAX`). No elapsed-time operand is visible in this bounded recurrence.
This makes it a concrete per-call cadence-risk candidate, not yet the proved
source of the high-rate hopping: the active branch, individual contribution
values, and cross-rate behavior remain unmeasured.

## Decision

Cycle 19 resolves the concrete producer boundary. Cycle 20 uses one qualified
single-hook state observer to record `+0x2DC`, `+0x378`, `+0x374`, central delta,
carried output, and the same provenance during a matched 60/240 route pair.
Only that pair can distinguish the stateful contribution from the actor getter
and identify the active recurrence. No equation or semantic collision label is
promoted yet.
