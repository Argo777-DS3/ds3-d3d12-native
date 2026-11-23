// I-0112: read-only census of DS3 code that loads a float NEAR a frame-rate reciprocal: within 1 % of 1/60, 1/30 or 1/120 but
// not exactly it (the exact ones are I0112Const's; `0x3C888723`, 0.016665, was found by reading). For each such 4-byte value in
// initialized non-executable memory, the references to it and their functions. Run with -readOnly -noanalysis; nothing is
// saved. Output: near-const-refs.tsv (target, value, constant address, use address, function, function name).
// @category DS3

import java.nio.charset.StandardCharsets;
import java.nio.file.*;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.symbol.*;

public class I0112NearConst extends GhidraScript {
    @Override
    public void run() throws Exception {
        Path out = Paths.get(getScriptArgs()[0]);
        float[] targets = {1f / 60f, 1f / 30f, 1f / 120f};
        String[] names = {"~1/60", "~1/30", "~1/120"};
        StringBuilder sb = new StringBuilder("target\tvalue\tconst_addr\tuse_addr\tfunction\tfunction_name\n");
        Memory mem = currentProgram.getMemory();
        ReferenceManager refs = currentProgram.getReferenceManager();
        int constants = 0, uses = 0;
        for (MemoryBlock b : mem.getBlocks()) {
            if (!b.isInitialized() || b.isExecute()) continue;
            byte[] buf = new byte[(int) Math.min(b.getSize(), Integer.MAX_VALUE - 16)];
            int n = b.getBytes(b.getStart(), buf);
            for (int i = 0; i + 4 <= n; i += 4) {
                int bits = (buf[i] & 0xFF) | (buf[i + 1] & 0xFF) << 8 | (buf[i + 2] & 0xFF) << 16 | (buf[i + 3] & 0xFF) << 24;
                float v = Float.intBitsToFloat(bits);
                for (int t = 0; t < targets.length; t++) {
                    if (v == targets[t] || Math.abs(v - targets[t]) > 0.01f * targets[t]) continue;
                    Address ca = b.getStart().add(i);
                    constants++;
                    for (Reference r : refs.getReferencesTo(ca)) {
                        Function f = getFunctionContaining(r.getFromAddress());
                        sb.append(names[t]).append('\t').append(v).append('\t').append(ca).append('\t').append(r.getFromAddress())
                          .append('\t').append(f == null ? "" : f.getEntryPoint().toString()).append('\t')
                          .append(f == null ? "" : f.getName()).append('\n');
                        uses++;
                    }
                }
            }
        }
        Files.createDirectories(out.getParent());
        Files.write(out, sb.toString().getBytes(StandardCharsets.UTF_8));
        println("near constants " + constants + " uses " + uses + " -> " + out);
    }
}
