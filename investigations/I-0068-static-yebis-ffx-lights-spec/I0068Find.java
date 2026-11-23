// I-0068: read-only instruction search on the canonical database (run with -readOnly -noanalysis; nothing saved).
// Arguments: output directory, then queries "<mnemonics>:<disp>[:<max>]", for example "OR+AND:148" lists every
// instruction whose mnemonic is OR or AND and whose first operand is a memory operand with displacement 0x148, with
// its function. Mnemonic "*" accepts any mnemonic and any operand. Output: find.txt.
// @category DS3

import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;

public class I0068Find extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        Path out = Paths.get(args[0]);
        StringBuilder sb = new StringBuilder();
        for (int i = 1; i < args.length; i++) {
            String[] f = args[i].split(":");
            Set<String> mn = new HashSet<>(Arrays.asList(f[0].toUpperCase().split("[+,]")));
            boolean any = mn.contains("*");
            String needle = "0x" + Long.toHexString(Long.parseLong(f[1], 16)) + "]";
            int max = f.length > 2 ? Integer.parseInt(f[2]) : 4000;
            sb.append("# ").append(args[i]).append('\n');
            int n = 0;
            InstructionIterator it = currentProgram.getListing().getInstructions(true);
            while (it.hasNext() && n < max) {
                Instruction ins = it.next();
                String m = ins.getMnemonicString();
                if (!any && !mn.contains(m)) continue;
                int ops = ins.getNumOperands();
                if (ops < 1) continue;
                boolean hit = false;
                for (int op = 0; op < (any ? ops : 1) && !hit; op++) {
                    String rep = ins.getDefaultOperandRepresentation(op).toLowerCase();
                    hit = rep.contains("[") && rep.contains(needle);
                }
                if (!hit) continue;
                Function g = getFunctionContaining(ins.getAddress());
                sb.append(ins.getAddress()).append("  ").append(ins).append("  ")
                  .append(g == null ? "nofunction" : g.getEntryPoint() + " " + g.getName()).append('\n');
                n++;
            }
        }
        Files.createDirectories(out);
        Files.write(out.resolve("find.txt"), sb.toString().getBytes(StandardCharsets.UTF_8));
    }
}
