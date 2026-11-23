# I-0008 clock owner, writer and retained refresh contract

Exact build: executable SHA-256 EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C.
Read-only Ghidra FINAL project; exports include instruction bytes and xrefs.
Artifacts are routed by manifest.json; external root is recorded there.

## Cycle 3: owner join

The four requested sites belong to 00826520/00826560 and 00826D20/00826EE0.
The latter member of each pair is destruction, not construction.
FrpgDefaultAnimationControl inherits arrays at +18 and +28; 008265E0 masks
+24 with 3fffffff and uses it as capacity. Its +24 is not the observed clock.
HvkAnim initializes two 0x28-byte pointer groups starting at +8. Neither name
binds the qualified player module.

The direct construction chain is decisive for the static layout:
008995F0 creates the container at character+1F90 and calls 009E6A10 for slot 2.
009E6A10 allocates D8 bytes and calls 009B4750. The constructor installs vtable
02895238; its COL/type descriptor decodes to SprjChrTimeActModule. 00A082D0
stores slot n at container+n*8: slot 2 is exactly modules+10. This refutes the
**direct-base identity** of the observed module as FrpgDefaultAnimationControl
or HvkAnim; it does not exclude those classes elsewhere upstream. Live vtable
identity in historical sessions was not recorded.

009B4D00 initializes ten records at +20+i*10 (hex), each {int id, float a,
float b, float duration}, to {-1,0,0,1}. +C0 is the next insertion index;
+C4 saves a prior index via 009B48D0. 005C7DB0 invokes that save through the
same character+1F90 -> modules+10 chain. Historical sampling always read
record zero's a at +24 and duration at +2C, without either index or record ID.

## Cycle 4: exact writer and callback provenance

009B4EB0 copies (id,a,b,duration) into record[next], then next=(next+1)%10.
For index zero, writer RVA 009B4F8D (F3 0F 11 78 04) copies XMM7 into +24;
009B4F97 (F3 44 0F 11 40 0C) copies XMM8 into +2C. At the first site RBX is
module, RAX=module+20+index*10, ESI=id, XMM7=a, XMM6=b, XMM8=duration.
Post site 009B4FC3 follows the index write. No delta arithmetic occurs here.
009B51E0 reorders its callback ABI to the writer: ECX=id, XMM1=a, XMM2=b,
XMM3=duration, fifth stack argument=character. It verifies the character and
loads character+1F90 -> modules+10 before calling 009B4EB0 at 009B523E.

A targeted raw-reference search found 00DD2921 registering 009B51E0 through
00DB1EB0 into [global 04794BA8]+0. 00DB3EA0 calls that pointer.
00DB3E10 registers 00DB3EA0 via 00F14F90 into global 0489F400.
00F14FE0 invokes that global; three direct callers survive:

| Producer | id | a (earlier sampled time) | b (later sampled time) | duration |
|---|---|---|---|---|
| 00F15EF0 | +DC | +D0, filled by 00F15EA0 | +D4 | +D8 |
| 00F1C2B0 | +B0 | +A4, filled by 00F1C200 | +A8 | +AC |
| 00F1C940 | +AC | +A0, filled by 00F1C900 | +A4 | +A8 |

All time reads route through 00F19200 -> 00F19230. Its runtime type dispatch
selects 01078330 (MOVSS XMM0,[RCX+140]) for descriptor 048A4C50 or 00F1B070
(MOVSS XMM0,[RCX+118]) for descriptor 0489F440, or recursively finds a child.
00F19420 supplies duration through a different asset/child path. No retained
capture chooses these source classes, objects or invocations. Ghidra signatures
that omit a register parameter are not authoritative; assembly retains it.
Some unrelated neighboring functions decode as bad data and are not used as
proof. Range export covers only 009B47B0..009B5900; no whole-program field scan.

## Cycle 5: new discriminator from existing captures

The original cycles 1-2 outputs are preserved unchanged. A supplemental audit
measures time since the last *change of the sampled value*, requiring stable
sampled identity, duration, speed and adjacent update indices throughout.
All 7,371 accepted refresh intervals have exactly ten updates (60:870,
90:1064,120:1961,165:3476). This supports a rotating cache interpretation of
factor ten rather than an intrinsic 10x animation-time scale.

At165, 3429/3476 increments match the sum of the preceding ten broad deltas
within absolute 1e-5, versus3384 with the current-ended window. Of128 original
ratio exceptions,108 remain in this stricter subset;61 of108 match the
preceding sum. Twenty excluded exceptions remain excluded due to the new
whole-interval stability/initialization gate, not silently declared resolved.
Forty-seven stable165 intervals still fail the summed-delta gate. Retained
identity comes from a different module and is not proof of ring-record identity.

## Interpretation and remaining discriminator

CONFIRMED static: module construction, ring layout, exact copy instructions and
registered callback chain. OBSERVED: ten-update refresh spacing and sum errors.
SUPPORTED: the historical factor ten is cache sampling cadence, not a 10x clock.
Unproved: active source class/object, same-node a/b pairing, input delta formula,
exception causes and animation invariance. No animation correction is justified.

Next minimal observation: simultaneously record both time getters and pre/post
ring insertion, qualify the module against the live player chain/vtable, retain
all ten records and indices, and identify the getter source feeding each writer.
Central +268 is a sentinel only, not an asserted timing input. No game run has
occurred in cycles3-5. No I-0008 integration into main is authorized.
