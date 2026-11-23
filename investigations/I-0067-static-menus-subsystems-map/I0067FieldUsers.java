// I-0067: users of the fields of an object reached through a pointer member, e.g. the GX quality-settings object [GXRS+0x5360]
// (read-only, no analysis). Arguments: output directory, then one or more hex displacements D of the pointer member. For every
// instruction "MOV reg,[base + D]" it follows reg forward (straight-line, at most 80 instructions, until reg is overwritten, a CALL
// clobbers it, or a RET/unconditional JMP ends the run) and lists every memory operand [reg + disp] that uses it, with the access
// kind (read/write by mnemonic and operand position) and width. A register copy MOV r2,reg is followed as well.
// Output fields.tsv: pointerDisp, fieldDisp, kind, width, address, function. users.tsv: pointerDisp, fieldDisp, function, count.
// Limits: register-only tracking; values spilled to the stack or kept in callee-saved registers across calls are not followed.
// @category DS3

import java.nio.file.*;
import java.util.*;
import java.util.regex.*;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;

public class I0067FieldUsers extends GhidraScript {
    private static final Pattern MEMREG = Pattern.compile("\\[([A-Za-z0-9]+)(?:\\+([A-Za-z][A-Za-z0-9]*)\\*[0-9a-fx]+)?(?:\\+(-?0x[0-9a-fA-F]+))?\\]");
    private static final Set<String> READONLY = new HashSet<>(Arrays.asList("CMP", "TEST", "UCOMISS", "COMISS", "UCOMISD", "COMISD", "PUSH", "BT", "JMP", "CALL", "LEA"));
    private static final Set<String> VOLATILE = new HashSet<>(Arrays.asList("RAX", "RCX", "RDX", "R8", "R9", "R10", "R11"));

    private static String canon(String r) {
        r = r.toUpperCase();
        switch (r) {
            case "EAX": case "AX": case "AL": case "AH": return "RAX";
            case "ECX": case "CX": case "CL": case "CH": return "RCX";
            case "EDX": case "DX": case "DL": case "DH": return "RDX";
            case "EBX": case "BX": case "BL": case "BH": return "RBX";
            case "ESI": case "SI": case "SIL": return "RSI";
            case "EDI": case "DI": case "DIL": return "RDI";
            case "EBP": case "BP": case "BPL": return "RBP";
            default:
                if (r.matches("R(8|9|1[0-5])[DWB]")) return r.substring(0, r.length() - 1);
                return r;
        }
    }

    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        Path out = Paths.get(a[0]); Files.createDirectories(out);
        Set<Long> ptrs = new LinkedHashSet<>();
        for (int i = 1; i < a.length; i++) ptrs.add(Long.parseLong(a[i], 16));
        StringBuilder rows = new StringBuilder();
        Map<String, Integer> users = new TreeMap<>();
        InstructionIterator it = currentProgram.getListing().getInstructions(true);
        while (it.hasNext()) {
            Instruction ins = it.next();
            if (!ins.getMnemonicString().equals("MOV") || ins.getNumOperands() != 2) continue;
            String src = ins.getDefaultOperandRepresentation(1).replace(" ", "");
            String dst = ins.getDefaultOperandRepresentation(0).replace(" ", "");
            if (!dst.matches("(?i)R[A-Z0-9]+|E[A-Z]+")) continue;
            Matcher m = MEMREG.matcher(src);
            if (!m.find() || m.group(3) == null) continue;
            String pd = m.group(3).toLowerCase();
            long pdv = Long.parseLong(pd.replace("0x", ""), 16);
            if (!ptrs.contains(pdv) || (m.group(2) != null)) continue;
            Function g = getFunctionContaining(ins.getAddress());
            String gname = g == null ? "?" : g.getEntryPoint().toString();
            Set<String> live = new HashSet<>();
            live.add(canon(dst));
            Instruction cur = ins.getNext();
            for (int k = 0; k < 80 && cur != null && !live.isEmpty(); k++, cur = cur.getNext()) {
                String mn = cur.getMnemonicString();
                if (mn.equals("RET")) break;
                if (mn.equals("JMP") && cur.getFlowType().isUnConditional()) break;
                // memory operands using a live register
                for (int op = 0; op < cur.getNumOperands(); op++) {
                    String rep = cur.getDefaultOperandRepresentation(op).replace(" ", "");
                    if (!rep.contains("[")) continue;
                    Matcher mm = MEMREG.matcher(rep);
                    while (mm.find()) {
                        if (!live.contains(canon(mm.group(1)))) continue;
                        long fd = 0;
                        if (mm.group(3) != null) { String t = mm.group(3).toLowerCase(); boolean neg = t.startsWith("-"); fd = Long.parseLong(t.replace("-", "").replace("0x", ""), 16); if (neg) fd = -fd; }
                        boolean isWrite = op == 0 && !READONLY.contains(mn);
                        int w = rep.toLowerCase().startsWith("byte") ? 1 : rep.toLowerCase().startsWith("word") ? 2 : rep.toLowerCase().startsWith("dword") ? 4 : rep.toLowerCase().startsWith("qword") ? 8 : rep.toLowerCase().startsWith("xmmword") ? 16 : 0;
                        if (mn.equals("LEA")) continue;
                        String key = Long.toHexString(pdv) + "\t" + Long.toHexString(fd);
                        rows.append(Long.toHexString(pdv)).append('\t').append(Long.toHexString(fd)).append('\t').append(isWrite ? "W" : "R").append('\t').append(w).append('\t')
                            .append(cur.getAddress()).append('\t').append(gname).append('\t').append(cur).append('\n');
                        users.merge(key + "\t" + gname, 1, Integer::sum);
                    }
                }
                // register kills / copies
                if (mn.equals("CALL")) { live.removeAll(VOLATILE); continue; }
                if (cur.getNumOperands() >= 1) {
                    String d0 = cur.getDefaultOperandRepresentation(0).replace(" ", "");
                    if (d0.matches("(?i)R[A-Z0-9]+|E[A-Z]+|[A-D][LXH]|[SD]IL?|BPL?|R(8|9|1[0-5])[DWB]") && !READONLY.contains(mn)) {
                        String c = canon(d0);
                        if (mn.equals("MOV") && cur.getNumOperands() == 2) {
                            String s1 = cur.getDefaultOperandRepresentation(1).replace(" ", "");
                            if (s1.matches("(?i)R[A-Z0-9]+") && live.contains(canon(s1))) { live.add(c); continue; }
                        }
                        if (live.contains(c)) live.remove(c);
                    }
                }
            }
        }
        Files.writeString(out.resolve("fields.tsv"), rows.toString());
        StringBuilder u = new StringBuilder();
        for (Map.Entry<String, Integer> e : users.entrySet()) u.append(e.getKey()).append('\t').append(e.getValue()).append('\n');
        Files.writeString(out.resolve("users.tsv"), u.toString());
        println("done " + users.size());
    }
}
