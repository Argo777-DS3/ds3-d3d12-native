# Spatial exchange observation boundary

Status: the original binary produced an accepted target-60 structural control
and a rejected target-165 crash capture. The hardened rebuild was then injected
once in a fresh process for a shortened target165 window. It stopped/restored
cleanly but reached the4,096-hit limit before trial end, so that trial is also
rejected. The configuration is disabled and no live run is active.

## Question

The static contract proves that00DD3850 derives and stores the module+28 spatial
block while forwarding an independent XMM1 value into005C7160. Companion
00DD3B50 forwards XMM1 into005C7470 and writes the current owner transform back
to the descriptor-selected buffer. The remaining question is the ordering and
temporal generation of that descriptor-selected exchange.

Retained captures begin downstream and omit the descriptor, selected48-byte
buffer, derived block and callback direction. They therefore cannot replay this
boundary across frame rates. Another manual route without these fields would
repeat the same ambiguity.

## Minimal four-site observation

Use four execute breakpoints only after the existing exact-build, thread,
restoration and overflow guards are retained:

| Slot | RVA | Event | Values available before the instruction |
|---|---:|---|---|
| DR0 | 00DD3850 | `forward_entry` | owner=RCX, descriptor=R9, elapsed lanes=XMM1 |
| DR1 | 00DD3A7A | `forward_write` | owner=RDI, selected source=RBX, module object=RCX, derived48=RDX, elapsed lanes=XMM10 |
| DR2 | 00DD3B50 | `reverse_entry` | owner=RCX, descriptor=R9, elapsed lanes=XMM1 |
| DR3 | 00DD3BB2 | `reverse_write` | owner=RDI, selected destination=RBX, current transform48=RAX |

At each entry, safely resolve `base=*descriptor`, signed offset at `base+0x14`,
and `selected=base+offset`; retain the raw base, offset and selected values.
Read48 bytes only when each pointer/read guard passes. Resolve owner+90 player,
player+1F90 modules and modules+28 object under the same guarded policy. Do not
call game functions from the exception handler.

Pair each write with the latest unmatched entry of the same direction, thread,
owner, selected address and broad-update index. Descriptor identity belongs to
the entry because neither write site preserves R9 across intervening calls. Forward pairs must
also have bit-identical XMM1/XMM10 lanes. Reject orphan, duplicate, reordered,
identity-mismatched or invalid-read pairs; do not repair them by nearest-time
matching. Reverse elapsed lanes come from the paired entry because00DD3B50 does
not preserve XMM1 through005C7470.

The event fields and pairing keys are fixed in
[`observation-schema.json`](observation-schema.json). The offline validator
[`validate_event_sequence.mjs`](validate_event_sequence.mjs) exercises valid,
interleaved, mismatched, orphan and duplicate sequences and writes
[`pairing-validation.json`](pairing-validation.json).
The matching native state machine passes the same identity and rejection classes;
its hashes and limits are recorded in [`BUILD-VALIDATION.json`](BUILD-VALIDATION.json).
An isolated Windows test passes two consecutive arms, 1,024 executions at each
of the four sites per arm, resume-flag handling and exact restoration of the
original debug-register state after both arms. This qualifies the generic
mechanism; [`BUILD-VALIDATION.json`](BUILD-VALIDATION.json) binds the compiled
observer and injector to their sources.

The accepted target-60 capture has 528 strict pairs in each direction, with no
rejection or overflow. The conditional target-165 trial crashed 2.8892191
seconds after arm and is rejected; see the
[`crash audit`](../analysis/20260920T091359Z-pid4128-spatial-crash/README.md).
The hardened rebuild writes completed spatial rows once per second and omits
the high-volume `02344730` phase log in spatial mode. Session27644 confirms
clean stop, log durability and exact timing/debug restoration under live load;
its overflow prevents accepted cross-rate validation.

## Completed acquisition sequence

This is a new boundary test, not a repeat of the release-tail maneuver. Use a
fresh exact-build offline process and the operator's physical keyboard input.

1. Set target60 with `Set-SpatialTarget.ps1`, inject the matching observer and
   verify READY, exact build identity, target60/1 and the four site checks.
2. In the already prepared clear route, press F4, move straight for about two
   seconds, release by four seconds and remain idle until the six-second trial
   completes. The log, not operator timing, defines the comparison window.
3. Flush and analyze `spatial-exchange.csv`. Require both directions, strict
   pairs, no rejected rows/overflow and valid input/payload blocks before any
   target165 trial.
4. The structural control passed and the same active instrument switched to165.
   That trial crashed2.8892191s after arm, before trial completion; it is
   rejected and no cross-rate comparison is accepted.
5. `Reset-SpatialConfig.ps1` restored the disabled on-disk state and raw/WER
   hashes were retained. Do not repeat the maneuver immediately. Any future
   trial must use a fresh process and the hardened build only after review.

## Shortened live gate outcome

The fresh-process hardened run used target165 with a1.5-second requested window.
Thomas held straight movement beyond the intended automatic end. The observer
hit its4,096-row per-trial capacity after about1.16 seconds, disabled the execute
breakpoints and stopped before `trial_end`. Controlled shutdown then produced
`COMPLETE`, exact `165/1 -> 30/0` restoration and durable rows. No crash occurred.

The prefix contains1,025 strict forward pairs and1,023 strict reverse pairs with
no pairing rejection, but overflow keeps the trial rejected. Do not repeat it
merely to enlarge the buffer. The retained prefix supports bounded engine mapping
in the [session assessment](../analysis/20260920T095812Z-pid27644-spatial-165-short/README.md).

Offline RTTI/vtable reconstruction now identifies the two entry functions as
slots 6 and 7 of `SprjBehaviorCharacterProxyDriver`; see the
[static owner assessment](../../static/spatial-owner-context/ASSESSMENT.md).
Because a type-agnostic search for call offsets `+0x30/+0x38` is nonselective,
the schema now includes a guarded entry-stack `caller_rva`. This is the smallest
field that can identify the call context selecting delta versus `30 * delta`.
The extension is offline-qualified only and does not retroactively change the
session27644 evidence or make another live run necessary.

`Set-SpatialTarget.ps1` defaults to `-MeasureSeconds 1.5` and accepts an explicit
value from0.5 to6 seconds. The checked-in disabled configuration also records1.5
seconds. Any future acquisition must first replan capacity and the required
window; this default is not a validated live acceptance protocol.

Control binary: `bin/I0007SpatialExchangeObserverInjector.exe`; DLL:
`bin/I0007SpatialExchangeObserver.dll`. Its valid configuration admits no
correction mode and rejects either causal patch flag. Do not interpret a
structural60 pass as a temporal or gameplay result.

## Acceptance boundary

A capture is structurally usable only if executable/opcodes match, debug state
restores exactly, no overflow occurs, every accepted write has exactly one valid
entry, forward elapsed lanes agree bit for bit, and all retained pointer reads
are valid. Report direction counts and rejected reasons before interpreting data.

Only then compare source48, derived48 and current-transform48 as vectors at
common accumulated elapsed positions. A cross-rate difference still requires
controlled source state and equivalent callback ordering; event pairing alone
does not prove a cadence defect or authorize a patch. The existing recurrence
captures remain the downstream reference and need not consume two of these four
hardware slots again.

## Stopping rules

Stop and restore on opcode/build mismatch, unqualified thread identity, pointer
read failure in a required field, overflow, sequence ambiguity or debug-state
restoration failure. Preserve the rejected capture as evidence. Do not repeat an
operator maneuver merely because the new boundary was invalid; first correct or
refute the instrument offline.
