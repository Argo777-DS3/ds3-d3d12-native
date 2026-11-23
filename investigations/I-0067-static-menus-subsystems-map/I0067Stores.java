// I-0067: every store whose destination byte range covers a given structure offset (run read-only, no analysis).
// Arguments: output directory, then <off> [<span>] : the byte offset to cover (hex), and the largest operand width considered (default 32).
// Reports stores of any width (byte..ymmword, string stores excluded) whose memory operand is [reg(+reg*k)+disp] with disp in
// (off-width, off], excluding RSP- and RIP-relative operands, grouped by (width, disp). Output: stores.tsv (address, function, text),
// summary.txt (count per width/disp). Read-modify-write forms (OR, AND, XOR, ADD, SUB, INC, DEC, NOT, NEG, SHL...) are included.
// @category DS3

import java.nio.file.*;
import java.util.*;
import java.util.regex.*;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;

public class I0067Stores extends GhidraScript {
    private static final Pattern MEM = Pattern.compile("^(?:(byte|word|dword|qword|xmmword|ymmword|tword|oword)\\s+ptr\\s+)?(?:[a-z]s:)?\\[(.*)\\]$", Pattern.CASE_INSENSITIVE);
    private static final Set<String> NOSTORE = new HashSet<>(Arrays.asList("CMP", "TEST", "PUSH", "JMP", "CALL", "LEA", "PREFETCHT0", "PREFETCHT1", "PREFETCHT2", "PREFETCHNTA", "NOP", "UCOMISS", "COMISS", "UCOMISD", "COMISD", "BT"));

    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        Path out = Paths.get(a[0]); Files.createDirectories(out);
        long off = Long.parseLong(a[1], 16);
        int maxw = a.length > 2 ? Integer.parseInt(a[2]) : 32;
        StringBuilder rows = new StringBuilder();
        Map<String, Integer> sum = new TreeMap<>();
        InstructionIterator it = currentProgram.getListing().getInstructions(true);
        while (it.hasNext()) {
            Instruction ins = it.next();
            String m = ins.getMnemonicString();
            if (NOSTORE.contains(m) || m.startsWith("J") || m.startsWith("CMOV") || m.startsWith("CMP") || m.startsWith("VCMP") || m.startsWith("MOVS") && m.length() <= 5 && !m.equals("MOVSS") && !m.equals("MOVSD") && !m.equals("MOVSX") && !m.equals("MOVSXD")) continue;
            if (ins.getNumOperands() < 1) continue;
            String rep = ins.getDefaultOperandRepresentation(0);
            Matcher mt = MEM.matcher(rep.trim());
            if (!mt.matches()) continue;
            String inner = mt.group(2).replace(" ", "");
            String low = inner.toLowerCase();
            if (low.contains("rsp") || low.contains("rip") || low.contains("esp")) continue;
            // displacement: trailing +0xNN or -0xNN
            long disp = 0;
            Matcher dm = Pattern.compile("([+-])0x([0-9a-fA-F]+)$").matcher(inner);
            if (dm.find()) { disp = Long.parseLong(dm.group(2), 16); if (dm.group(1).equals("-")) disp = -disp; }
            else if (inner.matches(".*[+-][0-9]+$")) continue;
            int w;
            String sz = mt.group(1) == null ? "" : mt.group(1).toLowerCase();
            switch (sz) { case "byte": w = 1; break; case "word": w = 2; break; case "dword": w = 4; break; case "qword": w = 8; break; case "xmmword": case "oword": w = 16; break; case "ymmword": w = 32; break; default: w = 0; }
            if (w == 0) {   // register operand sizes imply the width
                if (ins.getNumOperands() < 2) continue;
                Object[] o1 = ins.getOpObjects(1);
                String r1 = ins.getDefaultOperandRepresentation(1).toLowerCase();
                if (r1.startsWith("xmm")) w = m.equals("MOVSS") || m.equals("MOVD") ? 4 : (m.equals("MOVSD") || m.equals("MOVQ") || m.equals("MOVLPS") || m.equals("MOVHPS") ? 8 : 16);
                else if (r1.startsWith("ymm")) w = 32;
                else if (r1.matches("r(ax|bx|cx|dx|si|di|bp|8|9|1[0-5])d?w?")) { w = r1.endsWith("d") ? 4 : (r1.endsWith("w") ? 2 : 8); if (r1.matches("r(ax|bx|cx|dx|si|di|bp)")) w = 8; }
                else if (r1.matches("e(ax|bx|cx|dx|si|di|bp)")) w = 4;
                else if (r1.matches("(ax|bx|cx|dx|si|di|bp)")) w = 2;
                else if (r1.matches("(al|bl|cl|dl|sil|dil|bpl|ah|bh|ch|dh|r\\d+b)")) w = 1;
                else w = 8;
            }
            if (w > maxw) continue;
            if (disp > off || disp + w <= off) continue;
            Function g = getFunctionContaining(ins.getAddress());
            String key = "w" + w + "@+0x" + Long.toHexString(disp);
            sum.merge(key, 1, Integer::sum);
            rows.append(ins.getAddress()).append('\t').append(g == null ? "?" : g.getEntryPoint().toString()).append('\t').append(key).append('\t').append(ins).append('\n');
        }
        Files.writeString(out.resolve("stores.tsv"), rows.toString());
        StringBuilder s = new StringBuilder();
        for (Map.Entry<String, Integer> e : sum.entrySet()) s.append(e.getKey()).append('\t').append(e.getValue()).append('\n');
        Files.writeString(out.resolve("summary.txt"), s.toString());
        println("done " + sum.size() + " keys");
    }
}
