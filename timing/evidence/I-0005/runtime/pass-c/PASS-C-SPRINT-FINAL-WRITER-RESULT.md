# I-0005 Pass C — target-60 sprint final-writer result

## Outcome

Pass 1 succeeded for its bounded purpose. During one complete deterministic
target-60 sprint window, a single instruction committed every sampled moving
final-transform update inside the already qualified
`A1429C -> DE50EC` transition:

```text
function RVA 0x009DC4D0..0x009DC5BA
writer RVA   0x009DC580
instruction  MOVDQA [RBX+0x80], XMM6
```

All 180 moving boundary intervals had exactly one accepted write-watch hit.
Every hit used one update thread, one shared boundary context, one transform,
one writer site, and valid live code. `XMM6` matched the four committed
float32 lanes exactly after the expected float32 round trip. No watch overflow
or hook failure occurred.

This confirms final-transform commit provenance only. It does not identify
requested/root motion, collision, physics, or the origin of the upstream
candidate vector.

## Runtime acquisition

- Raw session: `sessions/20260919T144001Z-pid18536`.
- Derived analysis:
  `analysis/20260919T144001Z-pid18536-locomotion-writer-watch-60`.
- One complete trial covered broad updates 1116 through 1296: 181 samples and
  180 moving intervals over 3.016656386 accumulated seconds.
- Net distance was 17.671175821 units; net distance per accumulated delta was
  5.857868302.
- Animation ID remained `300020210`; animation progress per accumulated delta
  was 0.994474873.
- The watch produced 180 hits, with zero overflow.
- The enclosing boundary snapshots matched exactly: arm position equaled the
  `A1429C` after snapshot, and hit position equaled the following `DE50EC`
  before snapshot.
- The reversible timing state restored exactly from `60/1` to `30/0` with
  `success=1`; the session has `READY` and `COMPLETE` markers and no `FAILED`
  marker.

The CSV decimal rendering differs from the captured SIMD lanes by at most
`6.18e-7` when compared as binary64. Converting the CSV coordinates back to
float32 yields exact equality with the captured `XMM6` bits, which is the
appropriate comparison for the four-float store.

## Live-byte and static bridge

The live byte window decodes the accepted writer as:

```text
66 0F 7F B3 80 00 00 00    MOVDQA [RBX+0x80], XMM6
```

Read-only exact-build Ghidra exports place it in
`DS3-1152-F-009DC4D0`. The instruction listing shows this bounded flow:

1. choose `RBX+0xB0`, falling back to `RBX+0xA8`;
2. `DS3-1152-F-00DDA140` copies a 16-byte vector from the selected object's
   nested `+0x40 -> +0x70` storage to a local candidate;
3. mask/add constants are applied;
4. interpolate from the current `RBX+0x80` transform toward that candidate
   using scalar `RBX+0x1C0`;
5. copy the old transform to `RBX+0x90`, reset the scalar to 1.0, and, unless
   `RBX+0x1E1` suppresses it, commit `XMM6` to `RBX+0x80` at RVA `0x009DC580`.

The calls at RVAs `0x007EA190` and `0x007EA4B0` are single-instruction `RET`
functions in this exact build; they do not supply a recovered solver boundary.
Two static callers of `009DC4D0` were found at call sites `005C74DC` and
`005C7901`.

This trace identifies an upstream source candidate and the final blended
commit, but it does not prove where collision/physics or requested/root motion
is produced. Consequently, the offline bridge required for Pass 2 has **not**
passed. No target-165 internal-stage run is justified yet.

## Decision

- Accept the unique final writer and its exact committed vector as confirmed.
- Keep all upstream stage labels neutral.
- Keep Pass 2, roll, correction, and 240 closed.
- The next admissible work is offline provenance analysis of the selected
  `+0xA8/+0xB0 -> +0x40 -> +0x70` candidate or another equally bounded
  data-flow route. A new live run requires a separate announced protocol.
