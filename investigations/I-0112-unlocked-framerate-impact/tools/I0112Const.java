// I-0112: read-only census of DS3 code that uses a frame-rate constant (run with -readOnly -noanalysis; nothing saved).
// Finds every 4-byte-aligned float and 8-byte-aligned double in initialized non-executable memory equal to 1/60, 1/30, 60 or
// 30 (and 1/120, 120), lists the references to each (the code that loads it), and every instruction with a 32-bit scalar
// operand equal to one of the float bit patterns (a constant built in a register). Output: const-refs.tsv, one row a use:
// kind (data or imm), constant, constant address, using instruction, its function's entry and name.
// @category DS3

import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.symbol.*;

public class I0112Const extends GhidraScript {
    @Override
    public void run() throws Exception {
        Path out = Paths.get(getScriptArgs()[0]);
        Map<Integer, String> floats = new LinkedHashMap<>();
        floats.put(Float.floatToRawIntBits(1f / 60f), "f1/60");
        floats.put(Float.floatToRawIntBits(1f / 30f), "f1/30");
        floats.put(Float.floatToRawIntBits(60f), "f60");
        floats.put(Float.floatToRawIntBits(30f), "f30");
        floats.put(Float.floatToRawIntBits(1f / 120f), "f1/120");
        floats.put(Float.floatToRawIntBits(120f), "f120");
        Map<Long, String> doubles = new LinkedHashMap<>();
        doubles.put(Double.doubleToRawLongBits(1.0 / 60.0), "d1/60");
        doubles.put(Double.doubleToRawLongBits(1.0 / 30.0), "d1/30");
        doubles.put(Double.doubleToRawLongBits(60.0), "d60");
        doubles.put(Double.doubleToRawLongBits(30.0), "d30");
        StringBuilder sb = new StringBuilder("kind\tconstant\tconst_addr\tuse_addr\tfunction\tfunction_name\n");
        Memory mem = currentProgram.getMemory();
        ReferenceManager refs = currentProgram.getReferenceManager();
        Listing listing = currentProgram.getListing();
        int constants = 0, uses = 0;
        for (MemoryBlock b : mem.getBlocks()) {
            if (!b.isInitialized() || b.isExecute()) continue;
            Address a = b.getStart();
            long size = b.getSize();
            byte[] buf = new byte[(int) Math.min(size, Integer.MAX_VALUE - 16)];
            int n = b.getBytes(a, buf);
            for (int i = 0; i + 4 <= n; i += 4) {
                int bits = (buf[i] & 0xFF) | (buf[i + 1] & 0xFF) << 8 | (buf[i + 2] & 0xFF) << 16 | (buf[i + 3] & 0xFF) << 24;
                String name = floats.get(bits);
                if (name == null && i % 8 == 0 && i + 8 <= n) {
                    long hi = (buf[i + 4] & 0xFFL) | (buf[i + 5] & 0xFFL) << 8 | (buf[i + 6] & 0xFFL) << 16 | (buf[i + 7] & 0xFFL) << 24;
                    long d = (bits & 0xFFFFFFFFL) | hi << 32;
                    name = doubles.get(d);
                }
                if (name == null) continue;
                Address ca = a.add(i);
                constants++;
                for (Reference r : refs.getReferencesTo(ca)) {
                    Address from = r.getFromAddress();
                    Function f = getFunctionContaining(from);
                    sb.append("data\t").append(name).append('\t').append(ca).append('\t').append(from).append('\t')
                      .append(f == null ? "" : f.getEntryPoint().toString()).append('\t').append(f == null ? "" : f.getName()).append('\n');
                    uses++;
                }
            }
        }
        InstructionIterator it = listing.getInstructions(true);
        while (it.hasNext()) {
            Instruction ins = it.next();
            for (int op = 0; op < ins.getNumOperands(); op++) {
                for (Object o : ins.getOpObjects(op)) {
                    if (!(o instanceof Scalar)) continue;
                    Scalar s = (Scalar) o;
                    if (s.bitLength() < 32) continue;
                    String name = floats.get((int) s.getUnsignedValue());
                    if (name == null || (s.getUnsignedValue() >>> 32) != 0) continue;
                    Function f = getFunctionContaining(ins.getAddress());
                    sb.append("imm\t").append(name).append("\t\t").append(ins.getAddress()).append('\t')
                      .append(f == null ? "" : f.getEntryPoint().toString()).append('\t').append(f == null ? "" : f.getName()).append('\n');
                    uses++;
                }
            }
        }
        Files.createDirectories(out.getParent());
        Files.write(out, sb.toString().getBytes(StandardCharsets.UTF_8));
        println("constants " + constants + " uses " + uses + " -> " + out);
    }
}
