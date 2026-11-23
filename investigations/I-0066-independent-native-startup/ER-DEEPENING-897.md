# ER method deepening 897–899 (D-0159)

Thomas authorized closing the open points of [study716](ER-ENGINE-D3D12-DEEP-STUDY-716.md)
by static decompilation of Elden Ring 2.7.1 (SHA-256 `1A354710…`), read-only
headless Ghidra (`-process -readOnly -noanalysis`, no saved database change, no
ER launch). Scripts, consoles and C/ASM exports are under
`evidence/I-0066/artifacts/er-deepen-897`. ER addresses below are absolute VAs.
ER is architectural evidence only; nothing here is a DS3 layout, offset or ABI.

## (a) Worker presentation callback registration — closed

Study716 found native Present inside `1419EC620` but no registration of that
callback with the render worker. The chain is now complete in source:

- Renderer setup `1419E9B60` allocates a 16-byte object, stores vtable
  `142F07CB8` (slot0 = `1419EC620`, RTTI locator at slot −1 `1433B2380`) and
  context `renderer+0x120`, then publishes it at `renderer+0x140`
  (`1419EA571` LEA, `1419EA58B` store). The same function stores the worker
  at `renderer+0x6D0`.
- Frame end `1419E82C0` calls `1419EC1F0`. In threaded mode (`renderer+0x11C`)
  it calls `141AF5060(worker, orderedRecords[ (renderer+0x708)^1 ], renderer+0x140)`
  (`1419EC24B` loads R8 from `+0x140`).
- `141AF5060` first waits for the worker's previous job (`141AF5360`: idle event
  `worker+0x4D0`, message pump while waiting), stores the records at
  `worker+0x498` and, when presentation is enabled (settings+0x8E), the callback
  at `worker+0x4A0`, then signals the worker.
- Worker loop `141AF5180` processes the records through `1419ECCA0`, then calls
  the callback's slot0 with the current presentation settings
  (+0x50/+0x5C/+0x60) and `worker+0x4A8`, clears `+0x4A0` and sets idle.

So the same frame's ordered records are executed before its Present on the one
worker thread, and the next publication cannot overlap it. The non-threaded
branch (`+0x11C == 0`) does not use this callback path and is not covered.

## (b) Delayed destruction versus GPU last use — closed for the threaded path

- Ring tokens: frame end calls `141AFC950(slot = ring[e80 % 6], records)`,
  which queues ordered callback `141AFC880` into that frame's records. When the
  worker reaches it, it increments the slot's target value and signals it on the
  queue (`141EA4AF0`, queue+fence). A slot therefore completes when all work
  recorded earlier in that frame has completed on the GPU.
- Frame lag: `1419EE770` (frame end, before publication and release) waits in
  `141AFC9A0` until slot `(e80 − lag)` completes. The only integer writers of
  `renderer+0xF1C` set it to 2 (`1419E68CE` in renderer function `1419E65B0`,
  `140B1B992` in game renderer setup `140B1B2A0`); the two other `+0xF1C`
  stores write floats into other object types.
- Delayed release: `141AF4380` enqueues (resource, policy) into an incoming
  bucket keyed by delay d; frame begin `141AF4850` moves buckets to the active
  list with counter d+1; frame end `141AF4580`, called after the wait above,
  decrements every counter and runs the policy when it drops below 1 (per-call
  budget `+0x4E4` can only postpone).
- Consequence: a resource withdrawn during frame F with delay d is destroyed at
  the end of frame F+d+1, after waiting for frame F+d+1−2 = F+d−1. Its last use
  is in frame F at the latest, so the rule is GPU-safe when d ≥ 1 with lag 2.
  The CSFg patch owner uses d = 2 (one frame of margin). Worker serialization
  (`141AF5360`) guarantees the awaited slot was signalled by then.
- Exceptions kept explicit: the wait is skipped for the first 6 frames
  (`e80 ≤ 6`); a diagnostic flag (`1447F56FC`) bounds the spin and may return
  early; the non-threaded path and d = 0 are not covered.

**DS3 design consequence (inference, not ER transfer):** the recovered ER
invariant is "destroy after the completion token of the last frame that could
record a use". The native DS3 backend's explicit submission-token retention is
the same invariant expressed per resource; a fixed frame delay may be used only
together with a frame-ring wait that bounds GPU lag below that delay, as ER does.

## (c) Dynamic source arrays — ownership closed, simulation producer open (899)

- Manager `1401EC180` allocates a 0xF0-byte CPU instance with `1401E7160`, then
  appends it to its list at `manager+0x70` under lock `manager+0xC0`.
- `1401E7160` retains its four inputs by reference count, takes the vertex
  count from the source mesh vector (12-byte elements), and allocates its own
  CPU arrays: `+0x28` and `+0x30` (count×12, float3) and `+0x38` (count×4). It
  then constructs the CSFg patch (`1401E9230`, vtable `1429D16C8`) with
  remap table = input 6 at `patch+0x30` and source = this instance at
  `patch+0x38`, and registers the patch (`140B74FB0`).
- Patch writer `1401E92F0` reads exactly those arrays: `1401E7870` returns
  `instance+0x30` (float3 written to the declared position element),
  `1401E7860` returns `instance+0x38` (int fallback for the tag-3 element),
  through the per-mesh remap getters `1401EE060/EE0C0/EE090`, inside a lock
  `143D78960` and the current render frame (`1419E76A0`), then records the
  update to the owner's buffer (`1419E4ED0`).
So the CPU arrays, their allocation, ownership and the copy into the native
update form one owner chain. The function that fills `instance+0x30/+0x38`
each step (the simulation) was not located statically: the patch writer and
the instance are reached only through virtual dispatch with no recovered
vtable reference. Live same-instance identity is not established.

## Frame decision for (c) and (d)

Under the WORKFLOW anti-tunnel questions: DS3's own dynamic producers are
already evidenced (pose/stream RCA703/712), and its material/texture families
are covered by the DS3 coverage inventory. Locating ER's simulation writer, or
reconstructing every ER material family and special draw branch, would add
nearby detail without changing a DS3 implementation decision. Both stay open
and deprioritized; a bounded ER lookup is reopened only when a named DS3 seam
needs it (D-0060 comparative gate).
