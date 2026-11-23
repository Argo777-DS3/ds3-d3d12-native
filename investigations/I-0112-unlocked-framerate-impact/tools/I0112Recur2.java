// I-0112, second recurrence pass (study thread, 2026-10-30, for the unlock workstream): what I0112Recur.java leaves out (RESULT
// §4 « Limits »). Three kinds, each a per-call update of a field with no delta time in its chain:
//  SPLIT_STEP / SPLIT_DECAY: the scalar chain of I0112Recur walked back across conditional jumps (the decay branch of TRTF's
//    site sat behind one), up to 20 instructions, still stopping at a call, an unconditional jump or a return;
//  PACKED_STEP / PACKED_DECAY: the same for packed stores (MOVAPS/MOVUPS [base+off], XMMn) through ADDPS/SUBPS/MULPS, a
//    16-byte read-only constant or another field, no register multiply;
//  INT_COUNTER: an integer field stepped by a constant per call (INC/DEC/ADD/SUB imm on [base+off], directly or through a
//    register loaded from and stored back to it within 6 instructions), with the immediate the field or its register is compared
//    with in the next 12 instructions (a frame count, if any).
// Stack fields (RSP, RBP) are skipped. Run with -readOnly -noanalysis; nothing is saved. Argument: the output TSV.
// @category DS3

import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.symbol.*;

public class I0112Recur2 extends GhidraScript {
    Memory mem;

    String op(Instruction ins, int i) { return ins.getDefaultOperandRepresentation(i).toUpperCase(Locale.ROOT); }
    boolean isMem(String o) { return o.contains("["); }
    boolean stackField(String o) { return o.contains("RSP") || o.contains("RBP"); }

    // The float(s) at a read-only data address the operand references, or null.
    String constOf(Instruction ins, int i, int count) {
        for (Reference r : ins.getOperandReferences(i)) {
            Address a = r.getToAddress();
            MemoryBlock b = mem.getBlock(a);
            if (b == null || b.isExecute() || b.isWrite() || !b.isInitialized()) continue;
            try {
                StringBuilder s = new StringBuilder();
                for (int k = 0; k < count; k++) { if (k > 0) s.append(','); s.append(Float.intBitsToFloat(mem.getInt(a.add(4 * k)))); }
                return s.toString();
            } catch (Exception e) { return null; }
        }
        return null;
    }

    // Walk back the chain feeding `reg` from index k (exclusive). packed: the packed forms; split: cross conditional jumps.
    String[] chain(List<Instruction> body, int k, String reg, String dst, boolean packed, boolean split) {
        boolean self = false, regMul = false, crossed = false;
        List<String> consts = new ArrayList<>();
        String factor = "";
        int limit = split ? 20 : 12;
        for (int j = k - 1, n = 0; j >= 0 && n < limit; j--, n++) {
            Instruction p = body.get(j);
            FlowType ft = p.getFlowType();
            if (ft.isCall() || ft.isTerminal()) break;
            if (ft.isJump()) { if (split && ft.isConditional()) { crossed = true; continue; } break; }
            String m = p.getMnemonicString().toUpperCase(Locale.ROOT);
            if (p.getNumOperands() < 2 || !op(p, 0).equals(reg)) continue;
            String a1 = op(p, 1);
            boolean move = packed ? (m.equals("MOVAPS") || m.equals("MOVUPS")) : (m.equals("MOVAPS") || m.equals("MOVSS"));
            if (move) {
                if (a1.startsWith("XMM")) { reg = a1; continue; }
                if (a1.equals(dst)) self = true;
                else { String c = constOf(p, 1, packed ? 4 : 1); if (c != null) consts.add("load " + c); }
                break;
            }
            String add = packed ? "ADDPS" : "ADDSS", sub = packed ? "SUBPS" : "SUBSS", mul = packed ? "MULPS" : "MULSS";
            if (m.equals(add) || m.equals(sub) || m.equals(mul)) {
                if (a1.equals(dst)) { self = true; continue; }
                String c = isMem(a1) ? constOf(p, 1, packed ? 4 : 1) : null;
                if (c != null) { consts.add(m.substring(0, 3).toLowerCase(Locale.ROOT) + " " + c); continue; }
                if (m.equals(mul) && isMem(a1)) { factor = a1; continue; }
                if (m.equals(mul) && a1.startsWith("XMM")) regMul = true;
                continue;
            }
            if (m.startsWith("MIN") || m.startsWith("MAX")) continue;   // a clamp keeps the chain
            break;
        }
        if (!self || regMul) return null;
        if (split && !crossed) return null;                              // already in I0112Recur's output
        String kind = !consts.isEmpty() ? "STEP" : !factor.isEmpty() ? "DECAY" : null;
        if (kind == null) return null;
        return new String[] { kind, String.join("; ", consts), factor };
    }

    Long immOf(Instruction ins, int i) {
        for (Object o : ins.getOpObjects(i)) if (o instanceof Scalar) return ((Scalar) o).getSignedValue();
        return null;
    }

    // The immediate the field or `reg` is compared with in the next 12 instructions, or "".
    String comparedWith(List<Instruction> body, int k, String field, String reg) {
        for (int j = k + 1, n = 0; j < body.size() && n < 12; j++, n++) {
            Instruction p = body.get(j);
            if (p.getFlowType().isCall() || p.getFlowType().isTerminal()) break;
            String m = p.getMnemonicString().toUpperCase(Locale.ROOT);
            if (!m.equals("CMP") || p.getNumOperands() != 2) continue;
            String a0 = op(p, 0);
            if (a0.equals(field) || (!reg.isEmpty() && a0.equals(reg))) {
                Long v = immOf(p, 1);
                if (v != null) return Long.toString(v);
            }
        }
        return "";
    }

    @Override
    public void run() throws Exception {
        Path out = Paths.get(getScriptArgs()[0]);
        mem = currentProgram.getMemory();
        Listing listing = currentProgram.getListing();
        StringBuilder sb = new StringBuilder("kind\tfunction\tstore_addr\tfield\tconstants\tfield_factor_or_compare\n");
        Map<String, Integer> counts = new TreeMap<>();
        for (Function f : listing.getFunctions(true)) {
            if (monitor.isCancelled()) break;
            List<Instruction> body = new ArrayList<>();
            for (Instruction ins : listing.getInstructions(f.getBody(), true)) body.add(ins);
            for (int k = 0; k < body.size(); k++) {
                Instruction st = body.get(k);
                String m = st.getMnemonicString().toUpperCase(Locale.ROOT);
                if (st.getNumOperands() != 2) continue;
                String dst = op(st, 0), src = op(st, 1);
                String[] hit = null; String kind = null;
                if (m.equals("MOVSS") && isMem(dst) && src.startsWith("XMM") && !stackField(dst)) {
                    hit = chain(body, k, src, dst, false, true); kind = "SPLIT_";
                } else if ((m.equals("MOVAPS") || m.equals("MOVUPS")) && isMem(dst) && src.startsWith("XMM") && !stackField(dst)) {
                    hit = chain(body, k, src, dst, true, true);
                    if (hit == null) hit = chain(body, k, src, dst, true, false);
                    kind = "PACKED_";
                } else if (isMem(dst) && !stackField(dst) && (m.equals("INC") || m.equals("DEC"))) {
                    String cmp = comparedWith(body, k, dst, "");
                    hit = new String[] { "COUNTER", m.toLowerCase(Locale.ROOT) + " 1", cmp }; kind = "INT_";
                } else if (isMem(dst) && !stackField(dst) && (m.equals("ADD") || m.equals("SUB")) && immOf(st, 1) != null
                           && !src.startsWith("XMM")) {
                    long v = immOf(st, 1);
                    if (Math.abs(v) <= 16) { String cmp = comparedWith(body, k, dst, "");
                        hit = new String[] { "COUNTER", m.toLowerCase(Locale.ROOT) + " " + v, cmp }; kind = "INT_"; }
                } else if (m.equals("MOV") && isMem(dst) && !stackField(dst) && src.matches("E?[A-Z0-9]+D?|R[0-9]+D|E[A-Z]X")) {
                    // MOV reg,[f]; INC/ADD reg,imm; MOV [f],reg within 6 instructions
                    for (int j = k - 1, n = 0; j >= 0 && n < 6; j--, n++) {
                        Instruction p = body.get(j);
                        String pm = p.getMnemonicString().toUpperCase(Locale.ROOT);
                        if (p.getFlowType().isCall() || p.getFlowType().isJump() || p.getFlowType().isTerminal()) break;
                        if (p.getNumOperands() < 1 || !op(p, 0).equals(src)) continue;
                        if (pm.equals("INC") || pm.equals("DEC") || ((pm.equals("ADD") || pm.equals("SUB")) && p.getNumOperands() == 2 && immOf(p, 1) != null && Math.abs(immOf(p, 1)) <= 16)) {
                            for (int q = j - 1, r = 0; q >= 0 && r < 6; q--, r++) {
                                Instruction l = body.get(q);
                                if (l.getFlowType().isCall() || l.getFlowType().isJump()) break;
                                String lm = l.getMnemonicString().toUpperCase(Locale.ROOT);
                                if (l.getNumOperands() == 2 && op(l, 0).equals(src) && (lm.equals("MOV") || lm.equals("MOVZX") || lm.equals("MOVSX"))) {
                                    if (op(l, 1).equals(dst)) {
                                        String step = pm.equals("INC") || pm.equals("DEC") ? pm.toLowerCase(Locale.ROOT) + " 1" : pm.toLowerCase(Locale.ROOT) + " " + immOf(p, 1);
                                        hit = new String[] { "COUNTER", step, comparedWith(body, k, dst, src) }; kind = "INT_";
                                    }
                                    break;
                                }
                            }
                        }
                        break;
                    }
                }
                if (hit == null) continue;
                String name = kind + hit[0];
                counts.merge(name, 1, Integer::sum);
                sb.append(name).append('\t').append(f.getEntryPoint()).append('\t').append(st.getAddress()).append('\t')
                  .append(dst).append('\t').append(hit[1]).append('\t').append(hit[2]).append('\n');
            }
        }
        Files.createDirectories(out.toAbsolutePath().getParent());
        Files.write(out, sb.toString().getBytes(StandardCharsets.UTF_8));
        println("counts " + counts + " -> " + out);
    }
}
