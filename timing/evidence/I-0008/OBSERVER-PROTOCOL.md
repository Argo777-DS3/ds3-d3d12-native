# I-0008 minimal read-only identity observation

## Purpose and current gate

Discriminate the active +118 versus +140 getter/source objects and prove the
player ring insertion with paired native values. This is an identity/copy test,
not a high-framerate invariance test. No old locomotion acquisition is repeated.
Historical execution: Thomas signaled ready; the announced capture completed with180 exact pairs and verified debug restoration. DS3 was subsequently closed. No repeat is scheduled. See LIVE-AND-FORMULA.md.

## Operator protocol announced 2026-09-20

1. Fresh DS3 process, offline, character loaded in a quiet safe place.
2. Character remains stationary. No sprint, roll or target-cadence override.
3. When Thomas says ready, select/verify the process and exact executable hash.
4. Load the disabled observer and explicitly signal its PID-specific ARM event.
5. Record for at most3 seconds; STOP event can terminate early. Restore saved
   debug registers for every surviving armed thread; flush the capture.
6. Inspect restoration/overflow/thread coverage and paired-copy gates before
   accepting any result. Leave the DLL loaded/inert; do not unload or reinject.

This requires no input stimulation after ready. The observer changes only its
own buffer/control events and temporary debug registers, never game instructions,
clock values, time requests or gameplay outputs. DLL loading uses a temporary
remote path allocation. Four execute sites are exact-byte checked before arming.
All existing process threads except the observer worker are enumerated once;
new live threads at teardown invalidate coverage. A thread born and terminated
entirely inside the capture is not detected: no exhaustive thread-lifetime claim.
Concurrent existing hardware breakpoints reject arming rather than overwrite.

## Sites and capture

| DR slot | RVA | Observation |
|---|---|---|
| 0 | 00F1B070 | RCX source, +118 raw float, entry return address |
| 1 | 01078330 | RCX source, +140 raw float, entry return address |
| 2 | 009B4F8D | RBX live player module, RAX destination, ESI ID, XMM7 earlier time, XMM6 later time, XMM8 duration; index and full ring |
| 3 | 009B4FC3 | Same module after all four values and next index are stored; full ring |

Read player via image+0477FDB8 -> world+80 -> player+1F90 -> modules+10.
Require module vtable=image+02895238, both indices in[0,9], and destination
module+20+16*index. Pre/post pair on thread, object, record and incremented index.
Both getter families include non-player events deliberately; event order or a
matching float alone must not be promoted to causal source identity. Central
+268 is read only as a sentinel and has no assigned governing-input status.

Capacity65536 records, fixed280-byte schema, little-endian header of nine uint64:
magic4930303852494E47, record size, count, QPC frequency, armed, restored,
overflow, new-thread flag, image base. The record layout is capture.hpp; its
size is compile-time checked. Files go to the Windows temporary directory with
PID/tick identity, then Invoke-Observer.ps1 copies the one matching file into
evidence/I-0008/artifacts/live-<UTC>-pid<PID>/ before analysis. Do not delete the
original raw file until provenance is persisted.

## Offline validation (cycle6)

MSVC /W4 /WX compilation passes for DLL/controller and native tests. Capture
kernel tests all10 ring positions including wrap, bit-exact copies, both getter
offsets, invalid pointers/index/owner/destination, unchanged game buffer and
CONTEXT. Native execute test runs the actual DLL Arm/Restore/VEH code in an
isolated synthetic process: two4x256 execute-breakpoint arms,1024 exact getter
records, return-address reads, RF continuation and debug state re-read equal to
original after each arm. Analyzer fixtures reject mismatch, overflow, failed
restoration and truncation. The evidence manifest routes validation.json and
test-console.txt with binary/source hashes. No test starts or searches for DS3.

Offline tests do not validate real-game overhead, DLL loading/control/log output
against DS3, every thread lifetime, live source ownership or any timing formula.
Do not claim these passed before the announced run.

## Commands (operator signal required before invoking)

Build/test: powershell -NoProfile -ExecutionPolicy Bypass -File evidence/I-0008/observer/build.ps1

Observe an already-running verified offline process:
powershell -NoProfile -ExecutionPolicy Bypass -File evidence/I-0008/observer/Invoke-Observer.ps1 -GameProcessId <verifiedPID>

Read-only follow-up: node evidence/I-0008/observer/analyze_capture.mjs <capture.bin> <analysis.json>

No launch or timing-control command is provided. A missing COMPLETE, restoration
failure, overflow, or commit mismatch rejects the run; preserve evidence and
investigate before another attempt. Two failed local fixes require RCA.
