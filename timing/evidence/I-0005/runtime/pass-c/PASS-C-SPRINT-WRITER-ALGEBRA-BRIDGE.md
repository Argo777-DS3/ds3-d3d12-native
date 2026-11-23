# Pass C sprint — final-writer algebra bridge and one mirror acquisition

Date: 2026-09-19
Build: Steam BuildID 10167187 / `DarkSoulsIII.exe` 1.15.2.0
Mode: read-only static analysis; **no game launch occurred**

## Result

The final writer is now algebraically closed at instruction level. For one
invocation of `DS3-1152-F-009DC4D0`, define neutral quantities:

- `T = f32x4([RBX+0x80])`, the current vector;
- `selected = [RBX+0xB0]` when non-null, otherwise `[RBX+0xA8]`;
- `C = f32x4([[selected+0x40]+0x70])`, the raw candidate copied through the
  local at `[RSP+0x20]`;
- `s = f32([RBX+0x1C0])`;
- `M = (FFFFFFFF, FFFFFFFF, FFFFFFFF, 00000000)` as lane bit masks;
- `H = (0.0, 0.0, 0.0, 1.0)`.

The exact SSE sequence is:

```text
Q = C bitand M
P = ADDPS(Q, H)
D = fl32(P - T)
B = fl32(D * (s,s,s,s))
X = fl32(B + T)
```

Each `fl32` is the result of that individual packed single-precision
instruction under the captured `MXCSR`; the three operations must not be
contracted into an FMA. `XMM6 = X` immediately after RVA `009DC556`.
`Q` bit-preserves the three candidate XYZ lanes and clears the W bits. The
following `ADDPS` must still be reproduced as an instruction: for ordinary
finite XYZ values it leaves their numeric values unchanged and produces W
`1.0`, while signed-zero, denormal, or NaN behavior depends on SSE/MXCSR rules.
Mathematically on the ordinary finite domain, ignoring the mandated
intermediate rounding:

```text
X = T + s * (P - T)
P = (Cx, Cy, Cz, 1)
```

For XYZ, the applied offset is therefore the sequential-float32 form of
`s * (Cxyz - Txyz)`. The mask preserves candidate XYZ into `Q` and discards
its W bits; `ADDPS` with the constant forces the working W lane to `1.0`. No additional constant,
clamp, normalization, hidden call, or conditional arithmetic exists between
that calculation and the final write. RVAs `007EA190` and `007EA4B0` are
single-instruction `RET` functions in this build.

At RVA `009DC55E`, byte `[RBX+0x1E1]` is tested. If it is zero, the exact 128-bit
`XMM6` value is committed at RVA `009DC580`; if it is nonzero, that write is
skipped. In both cases the old `T` is copied to `[RBX+0x90]`, `+0x1C0` is reset
to `1.0`, and the inhibition byte is then cleared. This is an input/commit
gate description only; no semantic role is assigned.

## Candidate provenance and source selection

Selection at the writer is fully determined:

```text
if [RBX+0xB0] != 0:  selected = [RBX+0xB0]   # priority
else if [RBX+0xA8] != 0: selected = [RBX+0xA8]
else: invalid local candidate
```

`00DDA140` writes the caller local only when `selected+0x40` is non-null. A
null selected pointer or null nested pointer leaves the local unwritten by the
observed path. Such rows must be recorded as invalid and excluded, not treated
as zero vectors.

The containing object constructor `009D8340` initializes both pointers to
null. Setup `009DA810` always attempts to allocate/construct the `+0xA8`
object. It attempts the `+0xB0` object only when at least one of these neutral
conditions holds:

```text
query_result != 0
OR f32[owner+0x360] != f32[owner+0x36C]
OR f32[owner+0x364] != f32[owner+0x370]
```

The unordered `UCOMISS` paths also enter the optional branch. Allocation or
construction failure can still leave the corresponding pointer null.
`009D9390` releases and clears `+0xB0`, then `+0xA8`.

Both outers are built by `00DD9930`; `00DDA1C0` constructs their nested
`+0x40` object and initially writes its `+0x70` vector from neutral input-record
lanes `+0x00/+0x04/+0x08`, with W initialized to zero. `009DC090` subsequently
updates each non-null outer through `00DD9B50`. This establishes the concrete
pointer/value provenance needed at commit without assigning the outer, nested
object, update, or vector a gameplay-system name.

## Scalar provenance

The constructor initializes `+0x1C0` to `1.0`. The final writer reads it once
and resets it to `1.0` before deciding whether to commit. Therefore any
non-default value is a one-invocation input written after the previous reset
and before this consumption.

The bounded instruction scan found direct, exact-chain writers through
`... -> +0x1F90 -> +0x68` in functions `004DB8C0`, `005C2CD0`, `005C3940`,
`005C3D30`, `005D3610`, `005D3BC0`, `005EA7C0`, and `005EACB0`. Each writes
the same scalar to `+0x1B8` and `+0x1C0`; their concrete source fields and
write RVAs are preserved in `writer-scalar-provenance.csv`. This is exhaustive
for the direct exact-chain sites identified by the bounded scan, not a claim
that opaque aliasing or indirect code cannot write the field.

## Single mirrored 60/165 acquisition

One observation point is sufficient: an execute breakpoint at RVA
`009DC559`, before its no-op call. At this point, in the same writer invocation:

- `XMM1` still contains `T`;
- `[RSP+0x20]` contains raw `C` when the source chain is valid;
- `XMM0` and `[RBX+0x1C0]` contain `s` (`XMM0` broadcast to four lanes);
- `XMM6` contains final `X`;
- `+0xA8`, `+0xB0`, the selected pointer, nested `+0x40`, and inhibition
  `+0x1E1` remain readable before reset/clear;
- `MXCSR` supplies the required floating-point environment.

This replaces the three-position writer-watch mode for this acquisition; it
does not run alongside it. A row key is
`{session, target, thread, broad_update_index, writer_invocation_index, RBX}`.
The existing post-update transform snapshot confirms the same commit without
a second instruction breakpoint. Static flow permits acceptance only when
`inhibit==0`; the next effective instruction is then the confirmed commit.

Each row records raw bits and decoded floats for:

| Field | Capture |
|---|---|
| current | `XMM1` and `[RBX+0x80]`, requiring exact equality |
| raw candidate | `[RSP+0x20]` |
| scalar | low lane of `XMM0` and `[RBX+0x1C0]`, requiring exact equality; all XMM0 lanes equal |
| final | `XMM6` |
| source | enum `B0`, `A8`, or `NONE`; both outer pointers; selected pointer; nested pointer |
| inhibition | byte `[RBX+0x1E1]` and derived `will_commit` |
| arithmetic | `MXCSR`, reconstructed `P`, `D`, `B`, and `X` bit patterns |
| correlation | thread, update index, accumulated delta, marker/trial, RBX, transform pointer |

The acquisition is one unchanged instrument used first at target 60 and then
at target 165, with the already accepted sprint stimulus, equal accumulated
delta window, exact timing restoration, and no semantic label promotion.

## Discrimination rule

For every valid committed row compute, with sequential float32 operations:

```text
U = Pxyz - Txyz
A = Xxyz - Txyz
X_reconstructed = T + s * U
```

The first gate is bitwise: `X_reconstructed` must equal captured `XMM6`, and
the post-update transform must equal it. Any invalid pointer chain, mismatch,
overflow, unmatched update, or unexpected inhibition stops interpretation.

For the accepted equal-accumulated-delta windows, use the target-60 net
direction as a fixed projection axis and report both projected sums and path
norm sums. Factor the observed final ratio as:

```text
R_final = rate(A_165) / rate(A_60)
R_candidate = rate(U_165) / rate(U_60)
R_scalar_effective = R_final / R_candidate
```

Also compare the raw scalar distribution and source/inhibition frequencies.
Interpret in this order:

1. A changed `B0/A8/NONE` selection or inhibition rate is the first
   categorical divergent input.
2. If `R_candidate` explains the observed approximately `0.5` while the
   effective scalar ratio remains approximately `1`, the loss is already in
   the candidate offset at this writer.
3. If `R_candidate` remains approximately `1` and both the effective and raw
   scalar ratios explain approximately `0.5`, `+0x1C0` is the first numeric
   input that explains the loss.
4. If both differ, report their measured product; do not force a single owner.
5. If neither input factor explains `R_final`, reject alignment or capture
   validity before opening another stage.

“Explains” uses the existing 2% cross-condition locomotion tolerance and must
hold for both projected and path-based summaries. The output of this mirror is
only a computational discriminator. Pass 2 semantic naming, roll, correction,
and 240 remain closed.

## Evidence

- `writer-algebra.csv`
- `writer-candidate-provenance.csv`
- `writer-scalar-provenance.csv`
- exact listings/decompilations for `009DC4D0`, `00DDA140`, `009D8340`,
  `009DA810`, `009D9390`, `009DC090`, `00DD9930`, `00DDA1C0`, `00DD9B50`,
  `00DDA750`, and the chain-qualified scalar writers
- `writer-constants.csv`
