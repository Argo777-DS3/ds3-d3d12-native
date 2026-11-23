# I-0008 generator/control delta discriminator

## Purpose and gate

This single three-second target-165 observation distinguishes whether the live
player clip-control path receives the variable broad delta or a substituted
fixed `1/60`. It joins one qualified chain in execution order:

`hkbClipGenerator entry -> hkaDefaultAnimationControl entry/return -> player TimeAct ring writer`.

The run captures the generator and control identities, input delta, central
delta sentinel, current time, speed/crop/loop fields, native output and exact
player-ring values. It does not repeat the earlier source-getter/ring-identity
capture and requires no locomotion input.

No live run is authorized by this document alone. Thomas first starts the exact
build offline, loads a character in a quiet safe place, leaves the character
stationary and says `ready`. Codex does not start DS3 unexpectedly.

## Bounded intervention

The controller verifies executable SHA-256
`EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C`,
the locally validated observer hashes and absence of an existing I-0005/6/7/8
instrument. Loading the DLL alone does not arm it. The PID-specific ARM event
starts this sequence:

1. Save central timing `+354/+358`, set `165.0/1` in memory and read it back.
2. Wait one second, then require the same central object and exact target.
3. Exact-byte check and arm four execution breakpoints on all existing process
   threads except the observer worker.
4. Capture for at most three seconds.
5. Stop recording, restore and re-read every surviving thread's original debug
   registers, then restore and re-read the exact original timing pair.
6. Flush the raw file and signal COMPLETE. The DLL remains loaded and inert; do
   not unload or reinject it. Thomas may close DS3 only after the result is read.

No instruction byte or game file is changed. The only game-state intervention
is the already qualified temporary central timing request, which is restored
even when arming fails. Central-object drift, timing drift, restoration failure,
new-thread coverage, overflow, a missing site or an invalid chain rejects the
run. Preserve the artifacts and investigate; do not reinject into that process.

Four sites are exact-byte checked before arming:

| DR slot | RVA | Captured contract |
|---|---:|---|
| 0 | `010768C0` | generator/context, generator vtable, `+D0` control, `+140` time, mode/flags, XMM2 delta |
| 1 | `01284D70` | control/vtable and fields, XMM1 actual delta, output pointers and caller |
| 2 | `01284EA8` | retained R10 control, native output time and loop count |
| 3 | `009B4F8D` | validated live player module/ring destination, earlier/later time and duration |

The observer capacity is 65,536 fixed 200-byte records. The little-endian
header has 22 uint64 values: magic, record size/count, QPC frequency,
arm/debug-restoration/overflow/new-thread state, image base, four raw hit counts,
timing-applied/restored state, timing object, original pair, target pair and
restored pair. `capture.hpp` and `analyze_capture.mjs` are the canonical layouts.

## Acceptance and interpretation

The analyzer accepts only exact same-thread chains where generator control/time
and delta equal the control entry, the native return output equals the player
ring's later value, and the earlier value/duration/player/module identities all
match. One leading boundary ring may be unpaired; any later unmatched or
ambiguous ring rejects the capture.

- If every qualified XMM1/XMM2 delta equals central `+268` and differs from
  `1/60`, the live path supports the variable-central-delta hypothesis.
- If the actual delta is `1/60` while central `+268` is non-60, the capture
  supports cadence substitution at or before the generator input.
- Mixed identities, missing sites, ambiguous chains or restoration failures are
  insufficient information and authorize no timing conclusion.

A pass is bounded to the observed idle clip/control branch at target165. It does
not establish every animation state, transition, target, actor, roll i-frame or
whole-C4 correctness, and it does not authorize a patch.

## Offline validation and command

Cycle10 compiles DLL/controller and native tests with MSVC `/W4 /WX`. The field
fixture validates all four record types and rejection cases. The real Windows
hardware-breakpoint test performs two arms over all four synthetic sites,
verifies RF continuation and exact debug-state restoration, and proves the
temporary `30/0 -> 165/1 -> 30/0` timing sequence. Analyzer fixtures accept
exact chains, reject value/restoration/truncation failures and distinguish a
variable delta from fixed `1/60`. These tests never open or search for DS3.

After Thomas says ready, the technical owner runs exactly:

`powershell -NoProfile -ExecutionPolicy Bypass -File evidence/I-0008/control-observer/Invoke-Observer.ps1 -GameProcessId <verifiedPID>`
