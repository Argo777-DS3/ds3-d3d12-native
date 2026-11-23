// I-0067: for each function entry given, the defined strings it references (ASCII and UTF-16), its direct callers (first 6) and
// its callees count (read-only, no analysis). Arguments: output directory, then hex function addresses.
// Output funcstrings.tsv: entry, size, strings (up to 8, ' | '-separated), callers.
// @category DS3

import java.nio.file.*;
import java.util.*;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;

public class I0067FuncStrings extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        Path out = Paths.get(a[0]); Files.createDirectories(out);
        StringBuilder rows = new StringBuilder();
        for (int i = 1; i < a.length; i++) {
            Address ad = toAddr(a[i]);
            Function f = getFunctionContaining(ad);
            if (f == null) { rows.append(a[i]).append("\tnofunction\n"); continue; }
            LinkedHashSet<String> strs = new LinkedHashSet<>();
            InstructionIterator it = currentProgram.getListing().getInstructions(f.getBody(), true);
            while (it.hasNext() && strs.size() < 8) {
                Instruction ins = it.next();
                for (Reference r : ins.getReferencesFrom()) {
                    Data d = currentProgram.getListing().getDataAt(r.getToAddress());
                    if (d != null && d.hasStringValue()) {
                        String s = String.valueOf(d.getValue());
                        if (s.length() > 70) s = s.substring(0, 70);
                        strs.add(s.replace('\t', ' ').replace('\n', ' '));
                    }
                }
            }
            List<String> callers = new ArrayList<>();
            for (Reference r : getReferencesTo(f.getEntryPoint())) {
                if (!(r.getReferenceType().isCall() || r.getReferenceType().isJump())) continue;
                Function g = getFunctionContaining(r.getFromAddress());
                if (g != null && callers.size() < 6) callers.add(g.getEntryPoint().toString());
            }
            rows.append(f.getEntryPoint()).append('\t').append(f.getBody().getNumAddresses()).append('\t').append(String.join(" | ", strs)).append('\t').append(String.join(",", callers)).append('\n');
        }
        Files.writeString(out.resolve("funcstrings.tsv"), rows.toString());
        println("done");
    }
}
