// I-0067: every indirect call or tail jump through a vtable slot displacement, e.g. CALL qword ptr [reg + 0xC0] (read-only, no analysis).
// Arguments: output directory, then one or more hex displacements. Writes vcalls.tsv: address, function, displacement,
// the call instruction and the 5 preceding instructions (argument setup). RSP-relative and RIP-relative operands are ignored.
// @category DS3

import java.nio.file.*;
import java.util.*;
import java.util.regex.*;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;

public class I0067VCalls extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        Path out = Paths.get(a[0]); Files.createDirectories(out);
        Set<String> want = new HashSet<>();
        for (int i = 1; i < a.length; i++) want.add("0x" + Long.toHexString(Long.parseLong(a[i], 16)));
        Pattern p = Pattern.compile("\\[(?!.*(?:rsp|rip))[a-z0-9]+(?:\\+[a-z0-9*]+)*\\+(0x[0-9a-f]+)\\]", Pattern.CASE_INSENSITIVE);
        StringBuilder rows = new StringBuilder();
        InstructionIterator it = currentProgram.getListing().getInstructions(true);
        while (it.hasNext()) {
            Instruction ins = it.next();
            String mn = ins.getMnemonicString(); if (!(mn.equals("CALL") || mn.equals("JMP")) || ins.getNumOperands() != 1) continue;
            String rep = ins.getDefaultOperandRepresentation(0).replace(" ", "");
            if (!rep.contains("[")) continue;
            Matcher m = p.matcher(rep);
            if (!m.find() || !want.contains(m.group(1).toLowerCase())) continue;
            Function g = getFunctionContaining(ins.getAddress());
            Deque<String> prev = new ArrayDeque<>();
            Instruction q = ins;
            for (int k = 0; k < 5 && q != null; k++) { q = q.getPrevious(); if (q != null) prev.push(q.toString()); }
            rows.append(ins.getAddress()).append('\t').append(g == null ? "?" : g.getEntryPoint().toString()).append('\t').append(m.group(1)).append('\t').append(ins).append('\t').append(String.join(" ; ", prev)).append('\n');
        }
        Files.writeString(out.resolve("vcalls.tsv"), rows.toString());
        println("done");
    }
}
