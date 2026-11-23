// I-0066 W4 (FFX-WAVE1-READY): read-only GX reachability of the functions the native effect system calls as they are.
// Arguments: output directory, then "depth:N" (optional, default 6), then root addresses (hex). For each root, a breadth-first
// walk over direct calls (indirect and virtual calls are not followed and are counted) lists every reached function whose
// instructions read or write one of the GX globals (the display object 144961AB0, its job pool 144961BB0 and their
// neighbours 144961B70/B78/B80/BA2), with the call chain from the root; each root is also decompiled (<root>.c).
// Output: gx-reach.txt. Run with -readOnly -noanalysis; nothing is saved.
// @category DS3

import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;

public class I0066GxReach extends GhidraScript {
    static final long[] GX = {0x144961AB0L, 0x144961BB0L, 0x144961B70L, 0x144961B78L, 0x144961B80L, 0x144961BA2L};

    String gxTouch(Function f) {
        StringBuilder sb = new StringBuilder();
        InstructionIterator it = currentProgram.getListing().getInstructions(f.getBody(), true);
        while (it.hasNext()) {
            Instruction ins = it.next();
            for (Reference r : ins.getReferencesFrom()) {
                long t = r.getToAddress().getOffset();
                for (long g : GX) if (t == g) sb.append(String.format(" %x@%s", g, ins.getAddress()));
            }
        }
        return sb.toString();
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        Path out = Paths.get(args[0]);
        Files.createDirectories(out);
        int depth = 6, first = 1;
        if (args.length > 1 && args[1].startsWith("depth:")) { depth = Integer.parseInt(args[1].substring(6)); first = 2; }
        DecompInterface ifc = new DecompInterface();
        ifc.toggleCCode(true);
        if (!ifc.openProgram(currentProgram)) throw new IllegalStateException("Decompiler open failed");
        StringBuilder sb = new StringBuilder();
        try {
            for (int i = first; i < args.length; i++) {
                Address a = toAddr(args[i]);
                Function root = getFunctionAt(a);
                if (root == null) {
                    new DisassembleCommand(a, null, true).applyTo(currentProgram, monitor);
                    root = createFunction(a, null);
                }
                if (root == null) { sb.append("# ").append(args[i]).append(" no function\n"); continue; }
                DecompileResults r = ifc.decompileFunction(root, 120, monitor);
                String c = r.decompileCompleted() ? r.getDecompiledFunction().getC() : ("/* failed: " + r.getErrorMessage() + " */");
                Files.write(out.resolve(args[i] + ".c"), c.getBytes(StandardCharsets.UTF_8));
                Map<Function, Function> parent = new LinkedHashMap<>();
                Map<Function, Integer> level = new HashMap<>();
                ArrayDeque<Function> queue = new ArrayDeque<>();
                parent.put(root, null); level.put(root, 0); queue.add(root);
                int indirect = 0;
                List<String> hits = new ArrayList<>();
                while (!queue.isEmpty()) {
                    Function f = queue.poll();
                    String touch = gxTouch(f);
                    if (!touch.isEmpty()) {
                        StringBuilder chain = new StringBuilder();
                        for (Function p = f; p != null; p = parent.get(p)) chain.insert(0, " <- " + p.getEntryPoint());
                        hits.add("  gx " + f.getEntryPoint() + " " + f.getName() + ":" + touch + "\n    chain" + chain + "\n");
                    }
                    if (level.get(f) >= depth) continue;
                    InstructionIterator it = currentProgram.getListing().getInstructions(f.getBody(), true);
                    while (it.hasNext()) {
                        Instruction ins = it.next();
                        if (!ins.getFlowType().isCall()) continue;
                        Address[] flows = ins.getFlows();
                        if (flows.length == 0) { indirect++; continue; }
                        for (Address t : flows) {
                            Function g = getFunctionAt(t);
                            if (g == null) g = getFunctionContaining(t);
                            if (g == null || parent.containsKey(g)) continue;
                            parent.put(g, f); level.put(g, level.get(f) + 1); queue.add(g);
                        }
                    }
                }
                sb.append("# ").append(args[i]).append(' ').append(root.getName()).append(" depth ").append(depth)
                  .append(" reached ").append(parent.size()).append(" indirect_calls ").append(indirect)
                  .append(" gx_functions ").append(hits.size()).append('\n');
                for (String h : hits) sb.append(h);
            }
        } finally { ifc.dispose(); }
        Files.write(out.resolve("gx-reach.txt"), sb.toString().getBytes(StandardCharsets.UTF_8));
    }
}
