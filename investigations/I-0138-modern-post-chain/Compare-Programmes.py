#!/usr/bin/env python3
"""I-0138 tranche 4: compares a programme of ours (post_glare.hlsl, compiled as the build does: fxc, shader model 5 (the ghosts' pixel programmes 4), /O3 /Gis)
with the original's listing (fxc /dumpbin /Lx of the programme extracted from the game's image), two ways.

1. The listings, tranche 2's strict rule (compare_listings.sh): every instruction with every operand in order, its swizzle or
   write mask, its negation and its absolute value; the register numbers, the declarations and the shader model 5
   decorations are set aside. Alone that rule cannot tell which register is which (two listings of the same instructions
   reading another input, or two temporaries exchanged, pass it: the step review's M1), so it gives the verdict `listing`
   only with the second rule's `values`.
2. The values, always: the inputs' and outputs' declarations must be the same, and each listing is evaluated symbolically,
   component by component, into the expression
   that each output component holds at the end (the operations, their operands in the instruction's order, the constants'
   registers and components, the samples with their texture, sampler, coordinate expressions and component, the literals' bits).
   Two programmes with the same expressions compute the same values whatever the order the compiler scheduled the instructions
   in, the temporaries it chose and the components of a sample it wrote without reading them. `mov` is transparent, so
   `mad` then `mov_sat` is `mad_sat`. Verdict `values` when every output's expression is the same with each operation's
   operands in the same order; `values-commuted` when they are the same only once the operands of the commutative operations
   (add, mul, min, max, the two factors of a mad, the two vectors of a dot product) are put in one order: the same
   real-number results, but a NaN may meet another operand first; `DIFFERENT` otherwise. The operands of a bitwise `and` or
   `or` (they join the comparisons' masks here) are always put in one order: theirs cannot change a bit of the result.
   The evaluation knows a closed list of instructions (those of these programmes: no control flow, no discard, no load,
   no sample with an offset, a comparison or a gradient); another instruction, or a temporary read before it is written,
   is « not evaluated » and counts as different.

It proves nothing about a driver's lowering; the byte-for-byte tests against the library's programmes do (dof_stage_test,
mb_stage_test with a profile -ours).

Usage: python -I Compare-Programmes.py <post_glare.hlsl> <folder of original-*.asm> [--glare] [--only <substring>] [--show]
Without --glare, tranche 4's programmes (the depth of field's and the motion blur's; the folder
evidence/I-0138/artifacts/tranche-4-programmes); with it, tranche 2's glare family (63 programmes with the corona, tranche 8b, and GlareShape 3's, tranche 8f; the folder
evidence/I-0138/artifacts/tranche-2-programmes), which compare_listings.sh compared by the listings' rule alone.
"""
import os, re, subprocess, sys, tempfile

FXC = r"C:\Program Files (x86)\Windows Kits\10\bin\10.0.28000.0\x64\fxc.exe"

# name, profile, original listing's stem, defines
def glare_units():
    out = []
    for n in range(1, 9):
        out.append((f"VS_OFFSET {n}", "vs_5_0", f"vs_2D_OffsetTex2D{n}", [f"VS_OFFSET={n}"]))
    for n in range(1, 5):
        out.append((f"VS_GIVEN {n}", "vs_5_0", f"vs_2D_Tex2D{n}", [f"VS_GIVEN={n}"]))
    for n in range(1, 16):
        out.append((f"PS_TAPS {n}", "ps_5_0", f"ps_MadT1_S{n}", [f"PS_TAPS={n}"]))
    for n in range(2, 5):
        out.append((f"PS_GIVEN {n}", "ps_5_0", f"ps_MadT1_S{n}", [f"PS_GIVEN={n}"]))
    for n in range(2, 9):
        out.append((f"PS_ADD {n}", "ps_5_0", f"ps_MadT{n}", [f"PS_ADD={n}"]))
    for n in range(2, 9):
        out.append((f"PS_ADD {n} GAMMA", "ps_5_0", f"ps_MadLumGammaT{n}", [f"PS_ADD={n}", "GAMMA"]))
    out.append(("PS_BRIGHT REMAP", "ps_5_0", "ps_BrightPassT1_THRESHOLD_LUMINANCE_REMAP_S1", ["PS_BRIGHT", "REMAP"]))
    out.append(("PS_BRIGHT VIGNETTE REMAP", "ps_5_0", "ps_BrightPassT1_Vignette_THRESHOLD_LUMINANCE_REMAP_S1", ["PS_BRIGHT", "VIGNETTE", "REMAP"]))
    # tranche 8f: GlareShape 3's programmes (the listings: evidence/I-0138/t8f-readings/dxbc)
    out.append(("PS_BRIGHT", "ps_5_0", "ps_BrightPassT1_THRESHOLD_LUMINANCE_S1", ["PS_BRIGHT"]))
    out.append(("PS_BRIGHT VIGNETTE", "ps_5_0", "ps_BrightPassT1_Vignette_THRESHOLD_LUMINANCE_S1", ["PS_BRIGHT", "VIGNETTE"]))
    for n in range(2, 9):
        out.append((f"PS_MAX {n}", "ps_5_0", f"ps_MaxColorT{n}", [f"PS_MAX={n}"]))
    out.append(("PS_MAD_OFFSET", "ps_5_0", "ps_MadOffsetT2", ["PS_MAD_OFFSET"]))
    out.append(("VS_POINT_SYMMETRY", "vs_5_0", "vs_2D_PointSymmetryTex2D8", ["VS_POINT_SYMMETRY"]))
    # Shader model 4, as the build compiles them (Build-GameBootstrap.ps1: the library's own model, no « precise » marks).
    out.append(("PS_GHOST", "ps_4_0", "ps_GlareGhost_PointSymmetryT4_S8_TrimT1_S8", ["PS_GHOST"]))
    out.append(("PS_GHOST DIST", "ps_4_0", "ps_GlareGhost_PointSymmetryDistScaleT4_S8_TrimT1_S8", ["PS_GHOST", "DIST"]))
    out.append(("PS_GHOST_T3", "ps_5_0", "ps_MadT3_S3S3S2", ["PS_GHOST_T3"]))
    out.append(("PS_SHAFT_MASK", "ps_5_0", "ps_LightShaftMask_SM3", ["PS_SHAFT_MASK"]))
    out.append(("PS_CORONA", "ps_5_0", "ps_Corona", ["PS_CORONA"]))   # tranche 8b
    out.append(("PS_ALPHA", "ps_5_0", "ps_Alpha_R", ["PS_ALPHA"]))
    return out

def units():
    out = []
    for n in (16, 20, 25):
        out.append((f"PS_TAPS {n}", "ps_5_0", f"ps_MadT1_S{n}", [f"PS_TAPS={n}"]))
    lut = "ps_DepthOfField_BlurFactorAlphaFromDepthTextureTypeCastLut"
    out.append(("PS_DOF_FACTOR", "ps_5_0", lut + "_RVALUE", ["PS_DOF_FACTOR"]))
    out.append(("PS_DOF_FRONT_FACTOR", "ps_5_0", "ps_DepthOfField_FrontBlurBlendFactor", ["PS_DOF_FRONT_FACTOR"]))
    for b in range(5):
        for f in range(8):
            out.append((f"PS_DOF_COMPOSITE {b} {f}", "ps_5_0", f"ps_DepthOfField_CompositeBlurLevelsFrontBlur_TS{b}_{f}",
                        ["PS_DOF_COMPOSITE", f"DOF_BACK={b}", f"DOF_FRONT={f}"]))
    # Part 4b: the composite under holder flags 0, T{n} = TS{n-1}_0 with the scene's factor taken absolute.
    for n in range(1, 9):
        out.append((f"PS_DOF_COMPOSITE {n - 1} 0 ABS", "ps_5_0", f"ps_DepthOfField_CompositeBlurredLevels_T{n}",
                    ["PS_DOF_COMPOSITE", f"DOF_BACK={n - 1}", "DOF_FRONT=0", "DOF_ABS"]))
    out.append(("PS_MB_VELOCITY", "ps_5_0", "ps_MotionBlurMakeCameraVelocityOverwrite_R", ["PS_MB_VELOCITY"]))
    for n in range(2, 9):
        out.append((f"PS_MB_BLUR {n}", "ps_5_0", f"ps_MotionBlurVelocityDirDepthInnerscreen_SM3_T2_S{n}", [f"PS_MB_BLUR={n}"]))
    out.append(("VS_GIVEN 1", "vs_5_0", "vs_2D_Tex2D1", ["VS_GIVEN=1"]))
    out.append(("VS_GIVEN 2", "vs_5_0", "vs_2D_Tex2D2", ["VS_GIVEN=2"]))
    out.append(("VS_GIVEN 3", "vs_5_0", "vs_2D_Tex2D3", ["VS_GIVEN=3"]))
    out.append(("VS_OFFSET 8", "vs_5_0", "vs_2D_OffsetTex2D8", ["VS_OFFSET=8"]))
    return out

def body(path):
    """The instructions of a listing, without declarations, comments and the shader model 5 decorations."""
    lines, started = [], False
    for raw in open(path, encoding="utf-8", errors="replace"):
        line = raw.strip()
        if re.match(r"^[pv]s_[45]_0", line):
            started = True
            continue
        if not started or not line or line.startswith("//") or line.startswith("dcl_") or line == "ret":
            continue
        line = re.sub(r" \[precise[^\]]*\]", "", line)
        line = line.replace("_indexable", "")
        line = re.sub(r"\(texture[23]d\)\(float,float,float,float\)", "", line)
        line = line.replace("_aoffimmi(0,0,0)", "")
        lines.append(line)
    return lines

def block_registers(hlsl):
    """I-0138 tranche 8b: our block's slots (native/world/yebis_glare.h glare_block_registers): slot i is the library's register
    glare_block_registers[i]. Our listings' cb0[i] are read as the library's cb0[glare_block_registers[i]]."""
    header = os.path.join(os.path.dirname(os.path.abspath(hlsl)), "..", "..", "world", "yebis_glare.h")
    text = open(header, encoding="utf-8").read()
    m = re.search(r"glare_block_registers\{(.*?)\};", text, re.S)
    if not m:
        raise SystemExit("glare_block_registers not found in " + header)
    return [int(v) for v in re.findall(r"\b[0-9]+\b", re.sub(r"//[^\n]*", "", m.group(1)))]

def library_registers(lines, registers):
    return [re.sub(r"\bcb0\[([0-9]+)\]", lambda m: f"cb0[{registers[int(m.group(1))]}]", line) for line in lines]

def signature(path):
    """The inputs' and outputs' declarations of a listing (register, mask, interpolation, system value), sorted."""
    return sorted(raw.strip() for raw in open(path, encoding="utf-8", errors="replace")
                  if raw.strip().startswith("dcl_input") or raw.strip().startswith("dcl_output"))

def strict(lines):
    return [re.sub(r"\b[vo][0-9]+", "io", re.sub(r"\br[0-9]+", "r", line)) for line in lines]

# ---- the symbolic evaluation ----
class Table:
    """Interned expressions: an expression is a tuple (operation, child ids or atoms...), named by a small integer."""
    def __init__(self):
        self.ids, self.nodes = {}, []
    def make(self, *node):
        if node not in self.ids:
            self.ids[node] = len(self.nodes)
            self.nodes.append(node)
        return self.ids[node]
    def show(self, i, depth=6):
        node = self.nodes[i]
        if node[0] in ("in", "cb", "lit", "undef"):
            return node[0] + ":" + ".".join(str(x) for x in node[1:])
        if depth == 0:
            return "..."
        parts = []
        for x in node[1:]:
            parts.append(self.show(x, depth - 1) if isinstance(x, int) and node[0] not in ("samplemeta",) else str(x))
        return node[0] + "(" + ", ".join(parts) + ")"

COMMUTATIVE = {"add", "mul", "min", "max"}
BITWISE = {"and", "or"}   # on the comparisons' masks: the operands' order cannot change a bit of the result
COMPONENT = "xyzw"
# The instructions evaluated component by component, with their operand counts; with dp2 to dp4, sample and sample_l the whole list.
COMPONENTWISE = {"mov": 1, "sqrt": 1, "rsq": 1, "exp": 1, "log": 1, "add": 2, "mul": 2, "div": 2, "min": 2, "max": 2,
                 "lt": 2, "ge": 2, "eq": 2, "ne": 2, "and": 2, "or": 2, "mad": 3, "movc": 3}

def split_operands(text):
    out, depth, current = [], 0, ""
    for ch in text:
        if ch in "([":
            depth += 1
        elif ch in ")]":
            depth -= 1
        if ch == "," and depth == 0:
            out.append(current.strip())
            current = ""
        else:
            current += ch
    if current.strip():
        out.append(current.strip())
    return out

class Machine:
    def __init__(self, table):
        self.t, self.reg, self.out = table, {}, {}
    def op(self, name, *children):
        children = list(children)
        if name in BITWISE:
            children.sort()
        return self.t.make(name, *children)
    def read(self, operand, slot):
        """The expression of the operand's component that feeds the destination's component `slot` (0 to 3)."""
        neg = operand.startswith("-")
        if neg:
            operand = operand[1:]
        absolute = operand.startswith("|")
        if absolute:
            operand = operand.strip("|")
        if operand.startswith("l("):
            values = [v.strip() for v in operand[2:-1].split(",")]
            value = values[slot] if len(values) == 4 else values[0]
            bits = int(value, 16) if value.lower().startswith("0x") else None
            if bits is None:
                import struct
                bits = struct.unpack("<I", struct.pack("<f", float(value)))[0]
            e = self.t.make("lit", bits)
        else:
            m = re.match(r"^(cb[0-9]+\[[0-9]+\]|[a-z]+[0-9]+)(?:\.([xyzw]+))?$", operand)
            if not m:
                raise ValueError("operand " + operand)
            name, swizzle = m.group(1), m.group(2) or "xyzw"
            comp = swizzle[slot] if len(swizzle) == 4 else swizzle[0] if len(swizzle) == 1 else swizzle[min(slot, len(swizzle) - 1)]
            if name.startswith("cb"):
                e = self.t.make("cb", name, comp)
            elif name.startswith("r"):
                e = self.reg.get((name, comp))
                if e is None:
                    raise ValueError(f"{name}.{comp} read before it is written")
            else:
                e = self.t.make("in", name, comp)
        if absolute:
            e = self.t.make("abs", e)
        if neg:
            e = self.t.make("neg", e)
        return e
    def run(self, lines):
        for line in lines:
            opcode, _, rest = line.partition(" ")
            operands = split_operands(rest)
            sat = opcode.endswith("_sat")
            if sat:
                opcode = opcode[:-4]
            dest = operands[0]
            m = re.match(r"^([a-zA-Z]+[0-9]+)(?:\.([xyzw]+))?$", dest)
            name, mask = m.group(1), m.group(2) or "xyzw"
            results = {}
            if opcode in ("sample", "sample_l"):
                if len(operands) != (5 if opcode == "sample_l" else 4):
                    raise ValueError("operands of " + line)
                coord = tuple(self.read(operands[1], k) for k in range(2))
                tm = re.match(r"^(t[0-9]+)\.([xyzw]{4})$", operands[2])
                lod = (self.read(operands[4], 0),) if opcode == "sample_l" else ()
                for c in mask:
                    results[c] = self.t.make(opcode, tm.group(1), operands[3], *coord, *lod, tm.group(2)[COMPONENT.index(c)])
            elif opcode in ("dp2", "dp3", "dp4"):
                n = int(opcode[2])
                a = self.t.make("vec", *[self.read(operands[1], k) for k in range(n)])
                b = self.t.make("vec", *[self.read(operands[2], k) for k in range(n)])
                e = self.t.make(opcode, a, b)
                for c in mask:
                    results[c] = e
            elif COMPONENTWISE.get(opcode) == len(operands) - 1:
                for c in mask:
                    slot = COMPONENT.index(c)
                    args = [self.read(o, slot) for o in operands[1:]]
                    results[c] = args[0] if opcode == "mov" else self.op(opcode, *args)
            else:
                raise ValueError("instruction " + line)
            for c, e in results.items():
                if sat:
                    e = self.t.make("sat", e)
                (self.out if name.startswith("o") else self.reg)[(name, c)] = e
        return self.out

def values(table, ours, original):
    return Machine(table).run(ours), Machine(table).run(original)

def canon(table, i, memo):
    """The expression with the operands of each commutative operation in one order (by their own canonical names)."""
    if i in memo:
        return memo[i]
    node = table.nodes[i]
    if node[0] in ("in", "cb", "lit", "undef"):
        memo[i] = i
        return i
    children = [canon(table, x, memo) if isinstance(x, int) else x for x in node[1:]]
    if node[0] in COMMUTATIVE or node[0] in ("dp2", "dp3", "dp4"):
        children.sort()
    elif node[0] == "mad":
        children[:2] = sorted(children[:2])
    memo[i] = table.make(node[0], *children)
    return memo[i]

def swaps(table, a, b, memo, found, seen):
    """Walks two expressions that are the same once commuted and lists the operations whose operands are in another order."""
    if a == b or (a, b) in seen:
        return
    seen.add((a, b))
    na, nb = table.nodes[a], table.nodes[b]
    xs, ys = list(na[1:]), list(nb[1:])
    same = lambda x, y: (canon(table, x, memo) == canon(table, y, memo)) if isinstance(x, int) and isinstance(y, int) else x == y
    if not all(same(x, y) for x, y in zip(xs, ys)):
        found.append(na[0] + ": " + table.show(a, 2) + "   <>   " + table.show(b, 2))
        ys[0], ys[1] = ys[1], ys[0]
    for x, y in zip(xs, ys):
        if isinstance(x, int) and isinstance(y, int) and na[0] not in ("cb", "in", "lit", "undef"):
            swaps(table, x, y, memo, found, seen)

def first_difference(table, a, b, depth=0):
    if a == b:
        return None
    na, nb = table.nodes[a], table.nodes[b]
    if na[0] != nb[0] or len(na) != len(nb) or depth > 40:
        return table.show(a, 3) + "   <>   " + table.show(b, 3)
    for x, y in zip(na[1:], nb[1:]):
        if x != y:
            if isinstance(x, int) and isinstance(y, int) and na[0] not in ("cb", "in", "lit", "undef"):
                return first_difference(table, x, y, depth + 1)
            return table.show(a, 2) + "   <>   " + table.show(b, 2)
    return None

def main():
    hlsl, folder = sys.argv[1], sys.argv[2]
    only = sys.argv[sys.argv.index("--only") + 1] if "--only" in sys.argv else None
    show = "--show" in sys.argv
    registers = block_registers(hlsl)
    counts = {"listing": 0, "values": 0, "values-commuted": 0, "DIFFERENT": 0, "failed": 0}
    with tempfile.TemporaryDirectory() as tmp:
        for name, profile, original, defines in (glare_units() if "--glare" in sys.argv else units()):
            if only and only not in name:
                continue
            path = os.path.join(folder, f"original-{original}.asm")
            if not os.path.exists(path):
                print(f"{name}: no original listing ({original})")
                counts["failed"] += 1
                continue
            obj, asm = os.path.join(tmp, "o.dxbc"), os.path.join(tmp, "o.asm")
            command = [FXC, "/nologo", "/T", profile, "/E", "main", "/O3", "/Gis"]
            for d in defines:
                command += ["/D", d]
            done = subprocess.run(command + ["/Fo", obj, hlsl], capture_output=True, text=True)
            if done.returncode != 0:
                print(f"{name}: COMPILE FAILED: {(done.stdout + done.stderr).strip().splitlines()[:2]}")
                counts["failed"] += 1
                continue
            subprocess.run([FXC, "/nologo", "/dumpbin", "/Lx", obj, "/Fc", asm], capture_output=True, text=True, check=True)
            ours, theirs = library_registers(body(asm), registers), body(path)
            if signature(asm) != signature(path):
                print(f"{name} vs {original}: DIFFERENT (the inputs' or outputs' declarations: {signature(asm)} against {signature(path)})")
                counts["DIFFERENT"] += 1
                continue
            table = Table()
            try:
                a, b = values(table, ours, theirs)
                verdict = "values" if a == b else None
                if verdict and strict(ours) == strict(theirs):
                    counts["listing"] += 1
                    print(f"{name} = {original}: listing, and the same values ({len(ours)} instructions)")
                    continue
                found = []
                if verdict is None:
                    memo = {}
                    if set(a) == set(b) and all(canon(table, a[k], memo) == canon(table, b[k], memo) for k in a):
                        verdict = "values-commuted"
                        seen = set()
                        for k in sorted(a):
                            swaps(table, a[k], b[k], memo, found, seen)
                    else:
                        verdict = "DIFFERENT"
            except Exception as error:   # an instruction the evaluation does not know: said, never passed
                print(f"{name} vs {original}: DIFFERENT (not evaluated: {error})")
                counts["DIFFERENT"] += 1
                continue
            counts[verdict] += 1
            print(f"{name} vs {original}: {verdict} ({len(ours)} instructions against {len(theirs)})")
            for line in found:
                print("    swapped " + line)
            if verdict == "DIFFERENT":
                keys = sorted(set(a) | set(b))
                for key in keys:
                    if a.get(key) != b.get(key):
                        if key not in a or key not in b:
                            print(f"    {key}: written by one only")
                            continue
                        print(f"    {key[0]}.{key[1]}: {first_difference(table, a[key], b[key])}")
                        break
            if show:
                print("    ours:", *ours, sep="\n        ")
    total = sum(counts.values())
    print(f"{total} programmes: {counts['listing']} the same listing, {counts['values']} the same values in the same operand order, "
          f"{counts['values-commuted']} the same values with commuted operands, {counts['DIFFERENT']} different, {counts['failed']} not compared")
    return 0 if counts["DIFFERENT"] == 0 and counts["failed"] == 0 else 1

if __name__ == "__main__":
    sys.exit(main())
