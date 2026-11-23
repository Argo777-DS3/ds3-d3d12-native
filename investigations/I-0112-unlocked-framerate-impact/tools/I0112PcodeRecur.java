// I-0112, the decompiler-based recurrence pass (study thread, 2026-10-30, for the unlock workstream): the self-recurrences
// RECUR2 cannot see (a chain joined through a phi, an unconditional jump, or a register loaded before a branch: TRTF's decay
// branch in 1405E6580). For each listed function, decompiled, every STORE of a 4-byte value is walked back through its p-code
// data flow (COPY, CAST, INDIRECT, MULTIEQUAL, FLOAT_ADD/SUB/MULT/DIV/NEG/ABS, up to 24 ops) and kept when the flow reads the
// same address back (the same base variable and offset), scales or offsets it by a read-only constant or another field, and
// meets no float function input, no call result and no writable global (a delta time would arrive through one of them).
// Read-only (-readOnly -noanalysis); nothing is saved. Arguments: output TSV, a file of function entries (hex, one a line),
// then optional ranges `start-end` (hex) whose functions are added.
// @category DS3

import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.pcode.*;

public class I0112PcodeRecur extends GhidraScript {
    Memory mem;

    static final class Key {
        final String base; final long off;
        Key(String base, long off) { this.base = base; this.off = off; }
        @Override public boolean equals(Object o) { return o instanceof Key k && k.base.equals(base) && k.off == off; }
        @Override public int hashCode() { return base.hashCode() * 31 + Long.hashCode(off); }
        @Override public String toString() { return base + "+0x" + Long.toHexString(off); }
    }

    Key keyOf(Varnode v) {
        long off = 0; Varnode d = v;
        for (int i = 0; i < 8 && d != null; i++) {
            PcodeOp def = d.getDef();
            if (def == null) break;
            int oc = def.getOpcode();
            if ((oc == PcodeOp.INT_ADD || oc == PcodeOp.PTRSUB || oc == PcodeOp.PTRADD) && def.getNumInputs() >= 2 && def.getInput(1).isConstant()) {
                long c = def.getInput(1).getOffset();
                if (oc == PcodeOp.PTRADD && def.getNumInputs() == 3) c *= def.getInput(2).getOffset();
                off += c; d = def.getInput(0); continue;
            }
            if (oc == PcodeOp.COPY || oc == PcodeOp.CAST) { d = def.getInput(0); continue; }
            break;
        }
        if (d == null) return new Key("?", off);
        if (d.isConstant()) return new Key("abs", d.getOffset() + off);
        HighVariable h = d.getHigh();
        String base = h != null ? (h.getName() + "#" + System.identityHashCode(h)) : ("vn" + d.getSpace() + ":" + d.getOffset() + ":" + System.identityHashCode(d));
        return new Key(base, off);
    }

    boolean readOnly(long a) {
        try { MemoryBlock b = mem.getBlock(toAddr(a)); return b != null && b.isInitialized() && !b.isWrite() && !b.isExecute(); }
        catch (Exception e) { return false; }
    }

    Float floatAt(long a) { try { return Float.intBitsToFloat(mem.getInt(toAddr(a))); } catch (Exception e) { return null; } }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        Path out = Paths.get(args[0]);
        mem = currentProgram.getMemory();
        FunctionManager fm = currentProgram.getFunctionManager();
        Set<Address> entries = new LinkedHashSet<>();
        for (String line : Files.readAllLines(Paths.get(args[1]))) {
            String s = line.trim().replace("\"", ""); if (s.isEmpty() || s.startsWith("#")) continue;
            Function f = fm.getFunctionContaining(toAddr(s)); if (f != null) entries.add(f.getEntryPoint());
        }
        for (int i = 2; i < args.length; i++) {
            String[] r = args[i].split("-");
            AddressSet set = new AddressSet(toAddr(r[0]), toAddr(r[1]));
            for (Function f : fm.getFunctions(set, true)) entries.add(f.getEntryPoint());
        }
        DecompInterface ifc = new DecompInterface();
        ifc.setOptions(new DecompileOptions());
        ifc.openProgram(currentProgram);
        StringBuilder sb = new StringBuilder("function\tstore\tfield\tthrough_phi\tconstants\tfield_factors\tops\n");
        int done = 0, failed = 0, hits = 0;
        for (Address e : entries) {
            if (monitor.isCancelled()) break;
            Function f = fm.getFunctionAt(e);
            DecompileResults res = ifc.decompileFunction(f, 30, monitor);
            HighFunction hf = res == null ? null : res.getHighFunction();
            if (hf == null) { failed++; continue; }
            done++;
            Iterator<PcodeOpAST> it = hf.getPcodeOps();
            while (it.hasNext()) {
                PcodeOpAST st = it.next();
                if (st.getOpcode() != PcodeOp.STORE || st.getNumInputs() < 3) continue;
                Varnode value = st.getInput(2);
                if (value.getSize() != 4) continue;
                Key target = keyOf(st.getInput(1));
                if (target.base.equals("?")) continue;
                boolean self = false, phi = false, dtLike = false, floatOp = false;
                List<String> consts = new ArrayList<>(), fields = new ArrayList<>(), ops = new ArrayList<>();
                Deque<Varnode> work = new ArrayDeque<>(); Set<Varnode> seen = new HashSet<>();
                work.push(value); int steps = 0;
                while (!work.isEmpty() && steps < 24) {
                    Varnode v = work.pop(); if (!seen.add(v)) continue;
                    if (v.isConstant()) continue;
                    if (v.isInput()) { dtLike = true; continue; }   // a parameter in the flow: a dt may arrive through it
                    PcodeOp d = v.getDef(); if (d == null) continue;
                    steps++;
                    int oc = d.getOpcode();
                    switch (oc) {
                        case PcodeOp.LOAD -> {
                            Key k = keyOf(d.getInput(1));
                            if (k.equals(target)) self = true;
                            else if (k.base.equals("abs")) {
                                if (readOnly(k.off)) { Float c = floatAt(k.off); consts.add(c == null ? "?" : c.toString()); }
                                else dtLike = true;                                 // a writable global (a dt global, maybe)
                            } else fields.add(k.toString());
                        }
                        case PcodeOp.MULTIEQUAL -> { phi = true; for (Varnode in : d.getInputs()) work.push(in); }
                        case PcodeOp.COPY, PcodeOp.CAST, PcodeOp.INDIRECT, PcodeOp.FLOAT_NEG, PcodeOp.FLOAT_ABS -> work.push(d.getInput(0));
                        case PcodeOp.FLOAT_ADD, PcodeOp.FLOAT_SUB, PcodeOp.FLOAT_MULT, PcodeOp.FLOAT_DIV -> {
                            floatOp = true; ops.add(d.getMnemonic());
                            for (Varnode in : d.getInputs()) {
                                if (in.isConstant()) consts.add(Float.toString(Float.intBitsToFloat((int) in.getOffset())));
                                else work.push(in);
                            }
                        }
                        case PcodeOp.CALL, PcodeOp.CALLIND -> dtLike = true;          // a call's result: unknown, maybe a dt
                        default -> { }
                    }
                }
                if (!self || dtLike || !floatOp || (consts.isEmpty() && fields.isEmpty())) continue;
                hits++;
                sb.append(f.getEntryPoint()).append('\t').append(st.getSeqnum().getTarget()).append('\t').append(target).append('\t')
                  .append(phi).append('\t').append(String.join(";", consts)).append('\t').append(String.join(";", fields)).append('\t')
                  .append(String.join(";", ops)).append('\n');
            }
        }
        ifc.dispose();
        Files.write(out, sb.toString().getBytes(StandardCharsets.UTF_8));
        println("functions " + entries.size() + " decompiled " + done + " failed " + failed + " hits " + hits + " -> " + out);
    }
}
