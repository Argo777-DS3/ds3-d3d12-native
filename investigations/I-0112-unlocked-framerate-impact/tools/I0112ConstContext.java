// I-0112: read-only. For each function of const-functions.tsv (arg 1; columns function, uses, constants "name@use_address"),
// decompiles it and writes (a) <outDir>/full/<entry>.c, the whole decompiled C, and (b) <outDir>/contexts.txt, for each use
// address the decompiled lines whose tokens sit at that address (fallback: within 16 bytes), with 4 lines of context, plus the
// function's size and its callers' count. Run only with -readOnly -noanalysis; nothing is saved.
// Arguments: out directory, const-functions.tsv.
// @category DS3
import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

public class I0112ConstContext extends GhidraScript {
    void collect(ClangNode n, List<ClangToken> out) {
        if (n instanceof ClangToken) { out.add((ClangToken) n); return; }
        for (int i = 0; i < n.numChildren(); i++) collect(n.Child(i), out);
    }
    public void run() throws Exception {
        String[] args = getScriptArgs();
        Path dir = Paths.get(args[0]);
        Files.createDirectories(dir.resolve("full"));
        List<String> rows = Files.readAllLines(Paths.get(args[1]), StandardCharsets.UTF_8);
        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        StringBuilder all = new StringBuilder();
        for (int r = 1; r < rows.size(); r++) {
            String[] c = rows.get(r).split("\t");
            if (c.length < 3) continue;
            String entry = c[0].replace("\"", "").trim();
            String consts = c[2].replace("\"", "").trim();
            Address ea = toAddr(entry);
            Function f = getFunctionAt(ea);
            if (f == null) { all.append("=== " + entry + " no function\n"); continue; }
            List<Address> uses = new ArrayList<>();
            List<String> names = new ArrayList<>();
            for (String t : consts.split(" ")) { String[] p = t.split("@"); if (p.length == 2) { names.add(p[0]); uses.add(toAddr(p[1])); } }
            int callers = 0;
            for (Object o : getReferencesTo(ea)) callers++;
            all.append("=== " + entry + " " + f.getName() + " bytes=" + f.getBody().getNumAddresses() + " callers=" + callers + " consts=" + consts + "\n");
            DecompileResults res = di.decompileFunction(f, 90, monitor);
            if (!res.decompileCompleted()) { all.append("(decompile failed: " + res.getErrorMessage() + ")\n"); continue; }
            Files.writeString(dir.resolve("full/" + entry + ".c"), res.getDecompiledFunction().getC(), StandardCharsets.UTF_8);
            ClangTokenGroup root = res.getCCodeMarkup();
            List<ClangToken> toks = new ArrayList<>();
            collect(root, toks);
            // group tokens into lines
            List<List<ClangToken>> lines = new ArrayList<>();
            List<ClangToken> cur = new ArrayList<>();
            int lastLine = -1;
            for (ClangToken t : toks) {
                int ln = t.getLineParent() == null ? -1 : t.getLineParent().getLineNumber();
                if (ln != lastLine && !cur.isEmpty()) { lines.add(cur); cur = new ArrayList<>(); }
                lastLine = ln; cur.add(t);
            }
            if (!cur.isEmpty()) lines.add(cur);
            List<String> text = new ArrayList<>();
            for (List<ClangToken> l : lines) { StringBuilder sb = new StringBuilder(); int ind = 0; if (!l.isEmpty() && l.get(0).getLineParent()!=null) ind = l.get(0).getLineParent().getIndent(); for (int k=0;k<ind;k++) sb.append(' '); for (ClangToken t : l) sb.append(t.getText()); text.add(sb.toString()); }
            for (int u = 0; u < uses.size(); u++) {
                Address ua = uses.get(u);
                int hit = -1;
                for (int i = 0; i < lines.size() && hit < 0; i++)
                    for (ClangToken t : lines.get(i)) { Address a = t.getMinAddress(); if (a != null && a.equals(ua)) { hit = i; break; } }
                String how = "exact";
                if (hit < 0) {
                    how = "near"; long best = 17;
                    for (int i = 0; i < lines.size(); i++)
                        for (ClangToken t : lines.get(i)) { Address a = t.getMinAddress(); if (a != null) { long d = Math.abs(a.subtract(ua)); if (d < best) { best = d; hit = i; } } }
                }
                all.append("-- use " + names.get(u) + "@" + ua + " (" + how + ")\n");
                if (hit < 0) { all.append("   (no decompiled line found)\n"); continue; }
                for (int i = Math.max(0, hit - 4); i <= Math.min(text.size() - 1, hit + 4); i++)
                    all.append((i == hit ? ">> " : "   ") + text.get(i) + "\n");
            }
            Files.writeString(dir.resolve("contexts.txt"), all.toString(), StandardCharsets.UTF_8);
        }
        Files.writeString(dir.resolve("contexts.txt"), all.toString(), StandardCharsets.UTF_8);
        di.dispose();
        println("done " + rows.size());
    }
}
