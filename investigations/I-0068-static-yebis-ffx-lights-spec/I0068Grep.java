// I-0068: read-only instruction text search on the canonical database (run with -readOnly -noanalysis; nothing saved).
// Arguments: output directory, then queries "<mnemonics>:<text>[:<max>]". <mnemonics> is a '+'-separated list or "*";
// <text> is matched, case-insensitively, as a substring of the whole instruction text, for example "IMUL:0x3e8" lists every
// IMUL whose text contains 0x3e8 with its function. Use it for immediates, which I0068Find (memory displacements) cannot
// match. Output: grep.txt.
// @category DS3

import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;

public class I0068Grep extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        Path out = Paths.get(args[0]);
        StringBuilder sb = new StringBuilder();
        for (int i = 1; i < args.length; i++) {
            String[] f = args[i].split(":");
            Set<String> mn = new HashSet<>(Arrays.asList(f[0].toUpperCase().split("[+,]")));
            boolean any = mn.contains("*");
            String needle = f[1].toLowerCase();
            int max = f.length > 2 ? Integer.parseInt(f[2]) : 4000;
            sb.append("# ").append(args[i]).append('\n');
            int n = 0;
            InstructionIterator it = currentProgram.getListing().getInstructions(true);
            while (it.hasNext() && n < max) {
                Instruction ins = it.next();
                if (!any && !mn.contains(ins.getMnemonicString())) continue;
                if (!ins.toString().toLowerCase().contains(needle)) continue;
                Function g = getFunctionContaining(ins.getAddress());
                sb.append(ins.getAddress()).append("  ").append(ins).append("  ")
                  .append(g == null ? "nofunction" : g.getEntryPoint() + " " + g.getName()).append('\n');
                n++;
            }
        }
        Files.createDirectories(out);
        Files.write(out.resolve("grep.txt"), sb.toString().getBytes(StandardCharsets.UTF_8));
    }
}
