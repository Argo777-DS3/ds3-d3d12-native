// I-0067: one-pass MSVC x64 RTTI inventory of the canonical database (run with -readOnly -noanalysis; nothing saved).
// Argument: output directory. Writes classes.tsv (vtable, slot count, subobject offset, class name) and
// vslots.tsv (function entry, vtable, slot index, class name) for every vtable whose complete object locator is valid:
// signature 1, type-descriptor RVA at +0xC naming a ".?AV"/".?AU" descriptor, self RVA at +0x14.
// The vtable starts 8 bytes after a stored absolute pointer to the locator; slots are counted while they point
// into an executable block.
// @category DS3

import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.mem.*;

public class I0067Rtti extends GhidraScript {
    private final List<MemoryBlock> blocks = new ArrayList<>();
    private final List<byte[]> data = new ArrayList<>();

    private int blockOf(long va) {
        for (int i = 0; i < blocks.size(); i++) { MemoryBlock b = blocks.get(i); if (va >= b.getStart().getOffset() && va <= b.getEnd().getOffset()) return i; }
        return -1;
    }
    private long q(byte[] a, int o) { long v = 0; for (int k = 7; k >= 0; k--) v = (v << 8) | (a[o + k] & 0xff); return v; }
    private int d(byte[] a, int o) { return (a[o] & 0xff) | (a[o + 1] & 0xff) << 8 | (a[o + 2] & 0xff) << 16 | (a[o + 3] & 0xff) << 24; }

    @Override
    public void run() throws Exception {
        Path out = Paths.get(getScriptArgs()[0]);
        Files.createDirectories(out);
        long base = currentProgram.getImageBase().getOffset();
        for (MemoryBlock b : currentProgram.getMemory().getBlocks()) {
            if (!b.isInitialized() || b.getSize() > Integer.MAX_VALUE) continue;
            byte[] a = new byte[(int) b.getSize()];
            b.getBytes(b.getStart(), a);
            blocks.add(b); data.add(a);
        }
        // type descriptors: name at TD+0x10
        Map<Long, String> td = new HashMap<>();
        for (int i = 0; i < blocks.size(); i++) {
            byte[] a = data.get(i); long start = blocks.get(i).getStart().getOffset();
            for (int o = 0; o + 4 < a.length; o++) {
                if (a[o] != '.' || a[o + 1] != '?' || a[o + 2] != 'A' || (a[o + 3] != 'V' && a[o + 3] != 'U')) continue;
                int e = o; while (e < a.length && a[e] != 0 && e - o < 1024) e++;
                td.put(start + o - 0x10 - base, new String(a, o, e - o, StandardCharsets.US_ASCII));
            }
        }
        // complete object locators
        Map<Long, long[]> col = new HashMap<>();   // absolute COL -> {td rva, offset}
        for (int i = 0; i < blocks.size(); i++) {
            byte[] a = data.get(i); long start = blocks.get(i).getStart().getOffset();
            for (int o = 0; o + 24 <= a.length; o += 4) {
                if (d(a, o) != 1) continue;
                long rva = start + o - base;
                if ((d(a, o + 20) & 0xffffffffL) != rva) continue;
                long t = d(a, o + 12) & 0xffffffffL;
                if (!td.containsKey(t)) continue;
                col.put(start + o, new long[] { t, d(a, o + 4) & 0xffffffffL });
            }
        }
        StringBuilder classes = new StringBuilder("vtable\tslots\toffset\tclass\n");
        StringBuilder slots = new StringBuilder("function\tvtable\tslot\tclass\n");
        for (int i = 0; i < blocks.size(); i++) {
            byte[] a = data.get(i); long start = blocks.get(i).getStart().getOffset();
            for (int o = 0; o + 16 <= a.length; o += 8) {
                long[] c = col.get(q(a, o));
                if (c == null) continue;
                String name = td.get(c[0]);
                long vt = start + o + 8;
                int n = 0;
                for (int s = o + 8; s + 8 <= a.length && n < 1024; s += 8, n++) {
                    long target = q(a, s);
                    int bi = blockOf(target);
                    if (bi < 0 || !blocks.get(bi).isExecute()) break;
                    if (n > 0 && col.containsKey(q(a, s))) break;
                    slots.append(Long.toHexString(target)).append('\t').append(Long.toHexString(vt)).append('\t').append(n).append('\t').append(name).append('\n');
                }
                classes.append(Long.toHexString(vt)).append('\t').append(n).append('\t').append(Long.toHexString(c[1])).append('\t').append(name).append('\n');
            }
        }
        Files.write(out.resolve("classes.tsv"), classes.toString().getBytes(StandardCharsets.UTF_8));
        Files.write(out.resolve("vslots.tsv"), slots.toString().getBytes(StandardCharsets.UTF_8));
        println("td " + td.size() + " col " + col.size());
    }
}
