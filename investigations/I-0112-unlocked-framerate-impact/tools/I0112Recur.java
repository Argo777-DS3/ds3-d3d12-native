// I-0112: read-only search for per-call recurrences without a delta time, the terrain bug's kind (TRTF, 1405E6580:
// `[RBX+0x2DC] = [RBX+0x2DC] + 0.2*x` and `[RBX+0x2DC] = [RBX+0x378]*[RBX+0x2DC]`). For every scalar float store
// `MOVSS [base+off], XMMn`, it walks back up to 12 straight-line instructions (no call, jump or return between) along the chain
// that produced XMMn and records: whether the chain reads the same field back (a self recurrence), whether it scales or adds a
// constant from read-only data (its value), whether it multiplies by another field, and whether it multiplies by a register
// (a delta time passed in a register would show there). Kinds: CONST_STEP (self, a constant added, subtracted or scaled, no
// register multiply), FIELD_DECAY (self multiplied by another field, no register multiply), OTHER (not reported). Run with
// -readOnly -noanalysis; nothing is saved. Output: recur.tsv.
// @category DS3

import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.symbol.*;

public class I0112Recur extends GhidraScript {
    Memory mem;

    String op(Instruction ins, int i) { return ins.getDefaultOperandRepresentation(i).toUpperCase(Locale.ROOT); }

    // The float at a read-only data address the operand references, or null.
    Float constOf(Instruction ins, int i) {
        for (Reference r : ins.getOperandReferences(i)) {
            Address a = r.getToAddress();
            MemoryBlock b = mem.getBlock(a);
            if (b == null || b.isExecute() || b.isWrite() || !b.isInitialized()) continue;
            try { return Float.intBitsToFloat(mem.getInt(a)); } catch (Exception e) { return null; }
        }
        return null;
    }

    boolean isMem(String o) { return o.contains("["); }

    @Override
    public void run() throws Exception {
        Path out = Paths.get(getScriptArgs()[0]);
        mem = currentProgram.getMemory();
        Listing listing = currentProgram.getListing();
        StringBuilder sb = new StringBuilder("kind\tfunction\tfunction_name\tstore_addr\tfield\tconstants\tfield_factor\n");
        int stores = 0, constStep = 0, fieldDecay = 0;
        Set<String> funcs = new HashSet<>();
        for (Function f : listing.getFunctions(true)) {
            if (monitor.isCancelled()) break;
            List<Instruction> body = new ArrayList<>();
            for (Instruction ins : listing.getInstructions(f.getBody(), true)) body.add(ins);
            for (int k = 0; k < body.size(); k++) {
                Instruction st = body.get(k);
                if (!st.getMnemonicString().equalsIgnoreCase("MOVSS") || st.getNumOperands() != 2) continue;
                String dst = op(st, 0), src = op(st, 1);
                if (!isMem(dst) || !src.startsWith("XMM") || dst.contains("RSP") || dst.contains("RBP")) continue;
                stores++;
                String reg = src;
                boolean self = false, regMul = false;
                List<String> consts = new ArrayList<>();
                String factor = "";
                for (int j = k - 1, n = 0; j >= 0 && n < 12; j--, n++) {
                    Instruction p = body.get(j);
                    String m = p.getMnemonicString().toUpperCase(Locale.ROOT);
                    if (p.getFlowType().isCall() || p.getFlowType().isJump() || p.getFlowType().isTerminal()) break;
                    if (p.getNumOperands() < 2 || !op(p, 0).equals(reg)) continue;
                    String a1 = op(p, 1);
                    if (m.equals("MOVAPS") || m.equals("MOVSS") && !isMem(a1)) {
                        if (a1.startsWith("XMM")) { reg = a1; continue; }
                        break;
                    }
                    if (m.equals("MOVSS")) {           // a load from memory ends the chain
                        if (a1.equals(dst)) self = true;
                        else { Float c = constOf(p, 1); if (c != null) consts.add("load " + c); }
                        break;
                    }
                    if (m.equals("ADDSS") || m.equals("SUBSS") || m.equals("MULSS")) {
                        if (a1.equals(dst)) { self = true; continue; }
                        Float c = isMem(a1) ? constOf(p, 1) : null;
                        if (c != null) { consts.add(m.substring(0, 3).toLowerCase(Locale.ROOT) + " " + c); continue; }
                        if (m.equals("MULSS") && isMem(a1)) { factor = a1; continue; }
                        if (m.equals("MULSS") && a1.startsWith("XMM")) regMul = true;
                        continue;
                    }
                    if (m.equals("MINSS") || m.equals("MAXSS")) continue;   // a clamp keeps the chain
                    break;
                }
                if (!self || regMul) continue;
                String kind = !consts.isEmpty() ? "CONST_STEP" : !factor.isEmpty() ? "FIELD_DECAY" : null;
                if (kind == null) continue;
                if (kind.equals("CONST_STEP")) constStep++; else fieldDecay++;
                funcs.add(f.getEntryPoint().toString());
                sb.append(kind).append('\t').append(f.getEntryPoint()).append('\t').append(f.getName()).append('\t')
                  .append(st.getAddress()).append('\t').append(dst).append('\t').append(String.join("; ", consts)).append('\t')
                  .append(factor).append('\n');
            }
        }
        Files.createDirectories(out.getParent());
        Files.write(out, sb.toString().getBytes(StandardCharsets.UTF_8));
        println("float stores " + stores + " CONST_STEP " + constStep + " FIELD_DECAY " + fieldDecay + " in " + funcs.size() +
            " functions -> " + out);
    }
}
