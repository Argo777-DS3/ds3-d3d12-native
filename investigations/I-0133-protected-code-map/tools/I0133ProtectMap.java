// I-0133 protection map (read-only): memory blocks, function counts per block, and for every run of the decrypted-code
// overlay (rva<TAB>len) the containing function, its namespace and up to four callers with their namespaces.
// Args: <out_dir> <runs.tsv>
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.symbol.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;

public class I0133ProtectMap extends GhidraScript {
    String ns(Function f) {
        if (f == null) return "-";
        Namespace n = f.getParentNamespace();
        return (n == null || n.isGlobal()) ? "-" : n.getName(true);
    }

    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        File out = new File(a[0]);
        out.mkdirs();
        Address base = currentProgram.getImageBase();
        Memory mem = currentProgram.getMemory();
        FunctionManager fm = currentProgram.getFunctionManager();
        ReferenceManager rm = currentProgram.getReferenceManager();
        try (PrintWriter w = new PrintWriter(new FileWriter(new File(out, "blocks.tsv")))) {
            w.println("block\tstart\tend\tsize\texec\tinitialized\tfunctions");
            for (MemoryBlock b : mem.getBlocks()) {
                int n = 0;
                FunctionIterator it = fm.getFunctions(new AddressSet(b.getStart(), b.getEnd()), true);
                while (it.hasNext()) { it.next(); n++; }
                w.printf("%s\t%s\t%s\t%d\t%b\t%b\t%d%n", b.getName(), b.getStart(), b.getEnd(), b.getSize(), b.isExecute(),
                        b.isInitialized(), n);
            }
            w.printf("total_functions\t%d%n", fm.getFunctionCount());
        }
        List<String> lines = Files.readAllLines(Paths.get(a[1]));
        Map<Address, long[]> perFunc = new LinkedHashMap<>();
        long noFunc = 0, noFuncBytes = 0;
        try (PrintWriter w = new PrintWriter(new FileWriter(new File(out, "runs-functions.tsv")))) {
            w.println("rva\tlen\tfunction\tentry\tstarts_at_entry\tnamespace");
            for (String l : lines) {
                if (l.startsWith("rva") || l.isBlank()) continue;
                String[] p = l.split("\t");
                long rva = Long.parseLong(p[0], 16);
                int len = Integer.parseInt(p[1]);
                Address ad = base.add(rva);
                Function f = fm.getFunctionContaining(ad);
                if (f == null) {
                    noFunc++; noFuncBytes += len;
                    w.printf("%x\t%d\t-\t-\t-\t-%n", rva, len);
                    continue;
                }
                long[] c = perFunc.computeIfAbsent(f.getEntryPoint(), k -> new long[3]);
                c[0]++; c[1] += len;
                long d = ad.subtract(f.getEntryPoint());
                if (d >= 0 && d < 16) c[2] = 1;
                w.printf("%x\t%d\t%s\t%s\t%b\t%s%n", rva, len, f.getName(), f.getEntryPoint(), d >= 0 && d < 16, ns(f));
            }
        }
        try (PrintWriter w = new PrintWriter(new FileWriter(new File(out, "functions.tsv")))) {
            w.println("entry\tname\tnamespace\tbody_bytes\truns\tdiff_bytes\tentry_run\tcallers\tcaller_namespaces");
            for (Map.Entry<Address, long[]> e : perFunc.entrySet()) {
                if (monitor.isCancelled()) break;
                Function f = fm.getFunctionAt(e.getKey());
                StringBuilder cs = new StringBuilder(), cn = new StringBuilder();
                int k = 0, total = 0;
                for (Reference r : rm.getReferencesTo(f.getEntryPoint())) {
                    total++;
                    if (k >= 4) continue;
                    Function cf = fm.getFunctionContaining(r.getFromAddress());
                    if (cf == null) continue;
                    cs.append(cf.getName()).append(',');
                    cn.append(ns(cf)).append(',');
                    k++;
                }
                long[] c = e.getValue();
                w.printf("%s\t%s\t%s\t%d\t%d\t%d\t%d\t%d:%s\t%s%n", f.getEntryPoint(), f.getName(), ns(f),
                        f.getBody().getNumAddresses(), c[0], c[1], c[2], total, cs, cn);
            }
            w.printf("# runs_outside_functions\t%d\tbytes\t%d%n", noFunc, noFuncBytes);
        }
        println("I0133 done: functions " + perFunc.size() + " runs_outside " + noFunc);
    }
}
