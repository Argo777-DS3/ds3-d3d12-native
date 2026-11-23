# The fade of a draw-parameter layer in the original (static read, 2026-11-14)

A `ghidra-research` subagent's reading (Sonnet, GhidraMCP, DS3 1.15.2), asked by the R pilot for I-0141's next tranche
(RESULT « Read for the next tranches », item 1). **A worker's output: not canonical until the pilot re-reads the listings a
design relies on** (WORKFLOW « Roles and delegation »). The pilot read `FUN_140096560`, `FUN_140096B70`,
`GXArea_MarkRecordsDirty_140098170` and `AreaGparamController_Apply_140CE6350` itself; the rest below is the worker's.

## Where it runs

Each frame: `FUN_140D57900` → `FUN_140023410(GXRS, dt)` → `PostEffectUpdate_1400499C0` → `FUN_140095B00(registry, dt)` (the
registry `[GXRS+0x4788]`). Its second step calls `FUN_140092B50` for every area of every group slot (the layer entries
advance); its third walks the list at registry+0x330 and rebuilds the records.

## A layer entry (0x20 bytes, 17 an area, index = bank)

| Offset | Content |
|---|---|
| +0x00 | the file's handle (-1: none) |
| +0x08 | fading flag |
| +0x09 | done flag |
| +0x0C | elapsed time |
| +0x10 | duration |
| +0x14 | weight (the step's output; 1.0 at first) |
| +0x18, +0x1C | an override byte and its weight |

## 1. The advance and the weight (`FUN_140092B50(entries, dt)`, the worker's VERIFIED)

```
if (e[9]==0) { if (e[8]) { e[0xC]+=dt; if (e[0x10] <= e[0xC]) e[9]=1;
   if (e[0x18]==0) { if (e[0x10]<=0) e[0x14]=1.0; else e[0x14]=clamp(e[0xC]/e[0x10],0,1); }
   else e[0x14]=e[0x1C]; } }
else { e[8]=0; e[9]=0; e[0xC]=0; e[0x10]=0; }   // the frame after the end
```

The weight is linear in time and clamped to [0, 1], no easing. `dt` is the float at the task context's +8
(`140D57943: MOVSS XMM1,[RDX+8]`), zeroed when the task object's byte +0xD6 is set; that it is the frame's delta is INFERRED
(its writer was not read, nor any pause or time scale).

## 2. The rebuild of a record (`FUN_140095B00`, second loop; the items' layout INFERRED from the code)

- A list object L (its group +0x1C, its key +0x24, its items +0x2C / +0x30 of 0x28 bytes) is rebuilt while a layer of its
  area fades (L+0x40) or after a removal (`DAT_144961BA1`).
- The layers apply in increasing bank index: a higher bank overwrites a lower one. A layer whose file is absent is skipped.
- **Per value, not per record**: for each item the layer's value object (`vt+0x80(obj, key, A = the snapshot item+0x18,
  B = the live field item+0x10, w)`) writes the live field; `w` is the layer's weight while it fades (or 1.0 when the value's
  own flag `vt+0x88`, a byte at +0x4D, says the value does not fade), else the record's own weight (normally 1.0).
- `GXArea_MarkRecordsDirty` copies the live fields into the snapshots at a binding or an unbinding: the fade starts from the
  values as they were blended at that moment (INFERRED).
- The byte class of values (`FUN_1401E7120`, read as bytes): a key the layer lacks writes nothing (the lower layers'
  value stays); `w <= 0` keeps the snapshot; `w >= 1` takes the layer's value; between, `(1 - w) A + w V` truncated (INFERRED
  from partly decoded instructions). The float, vector and bool classes were not read (a plain lerp presumed).

## 3. An unbinding with a time (`FUN_140096B70`, the worker's VERIFIED; the pilot's own reading agrees)

The removed entry is cleared at once (no fade-out of its own); the nearest live entry beneath takes the duration and fades
in from the snapshot (the values blended just before the unbinding) to its own; higher live layers still overwrite. Without
a live layer beneath, nothing fades.

## Not read

The writer of the delta (task context +8) and of the byte +0xD6; the record-level fade's writer (L+0x09..+0x18); `vt+0x60` /
`vt+0x90`; `FUN_1401E1F70` (registry+0x198). Read by the pilot since (I-0141 RESULT « Tranche 23 »): every value class's
`vt+0x80` and the per-value flag (`vt+0x88`, a byte at a class's own offset, +0x4D for the byte classes), its source the definition's
byte +0x44 given at each AddParam call.

## What a native transcription needs (the pilot's note)

`Banks` reads a value from the latest layer that holds its id, whole. To follow the original: per area, the layers in bank
order with a weight advanced by the frame's delta (a binding's time; an unbinding's time on the layer beneath), and each
value read as the lerp from the value before the change to the top layer's by that weight, for the float values at least.
The readers are `Banks::Sun`, `Fog`, `Env`, `Shadow`, `GroupFloat`, `GroupEntries` and their callers: one blend point under
them (the value read), not one per producer.
