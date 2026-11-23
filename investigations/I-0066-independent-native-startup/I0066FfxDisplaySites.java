// I-0066 W4 FW-2b (D-0173): the reads of the GX display object [144961AB0] made by the effect GX module. Read-only.
// Arguments: output directory, the RTTI slot table (evidence/I-0067/rtti/vslots.tsv), then root addresses (hex) added to the
// slot functions of every class whose name matches the effect classes (GXFFX, GXVFX, FFX, FX4CG2, GXFXResolveEntity,
// SprjFfx). From those roots, a closure over direct calls that expands only functions inside the effect module's code
// ranges (0x140244000-0x1402C2000, 0x14036C000-0x140372000, 0x142150000-0x14215C000); in every reached function of those
// ranges, each instruction whose memory operand is the RIP-relative global 0x144961AB0 is listed with its length and bytes.
// Output: sites.tsv (rva, function rva, length, hex bytes), functions.txt (the module functions reached), and summary.txt.
// Run with -readOnly -noanalysis; nothing is saved.
// @category DS3

import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;
import java.util.regex.*;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;

public class I0066FfxDisplaySites extends GhidraScript {
    static final long DISPLAY = 0x144961AB0L;
    static final long[][] RANGES = {{0x140244000L, 0x1402C2000L}, {0x14036C000L, 0x140372000L}, {0x142150000L, 0x14215C000L}};
    static boolean inModule(long a) { for (long[] r : RANGES) if (a >= r[0] && a < r[1]) return true; return false; }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        Path out = Paths.get(args[0]);
        Files.createDirectories(out);
        Pattern classes = Pattern.compile("GXFFX|GXVFX|@FFX@@|FX4CG2|GXFXResolveEntity|SprjFfx");
        Set<Long> roots = new TreeSet<>();
        for (String line : Files.readAllLines(Paths.get(args[1]), StandardCharsets.UTF_8)) {
            String[] f = line.split("\t");
            if (f.length < 4 || f[0].equals("function")) continue;
            if (classes.matcher(f[3]).find()) roots.add(Long.parseLong(f[0], 16));
        }
        for (int i = 2; i < args.length; i++) roots.add(Long.parseLong(args[i], 16));
        ArrayDeque<Function> queue = new ArrayDeque<>();
        Set<Function> seen = new LinkedHashSet<>();
        for (long r : roots) {
            if (!inModule(r)) continue;
            Function f = getFunctionAt(toAddr(r));
            if (f == null) f = getFunctionContaining(toAddr(r));
            if (f != null && seen.add(f)) queue.add(f);
        }
        while (!queue.isEmpty()) {
            Function f = queue.poll();
            InstructionIterator it = currentProgram.getListing().getInstructions(f.getBody(), true);
            while (it.hasNext()) {
                Instruction ins = it.next();
                if (!ins.getFlowType().isCall()) continue;
                for (Address t : ins.getFlows()) {
                    if (!inModule(t.getOffset())) continue;
                    Function g = getFunctionAt(t);
                    if (g == null) g = getFunctionContaining(t);
                    if (g != null && seen.add(g)) queue.add(g);
                }
            }
        }
        StringBuilder sites = new StringBuilder("rva\tfunction\tlength\tbytes\n");
        StringBuilder functions = new StringBuilder();
        int count = 0, withSites = 0;
        for (Function f : seen) {
            functions.append(String.format("%x%n", f.getEntryPoint().getOffset() - 0x140000000L));
            boolean any = false;
            InstructionIterator it = currentProgram.getListing().getInstructions(f.getBody(), true);
            while (it.hasNext()) {
                Instruction ins = it.next();
                boolean hit = false;
                for (Reference r : ins.getReferencesFrom()) if (r.getToAddress().getOffset() == DISPLAY) hit = true;
                if (!hit) continue;
                byte[] b = ins.getBytes();
                StringBuilder hex = new StringBuilder();
                for (byte x : b) hex.append(String.format("%02X", x & 0xFF));
                sites.append(String.format("%x\t%x\t%d\t%s%n", ins.getAddress().getOffset() - 0x140000000L,
                    f.getEntryPoint().getOffset() - 0x140000000L, b.length, hex));
                count++; any = true;
            }
            if (any) withSites++;
        }
        Files.write(out.resolve("sites.tsv"), sites.toString().getBytes(StandardCharsets.UTF_8));
        Files.write(out.resolve("functions.txt"), functions.toString().getBytes(StandardCharsets.UTF_8));
        Files.write(out.resolve("summary.txt"), String.format("roots %d module_functions %d functions_with_sites %d sites %d%n",
            roots.size(), seen.size(), withSites, count).getBytes(StandardCharsets.UTF_8));
    }
}
