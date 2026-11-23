// I-0068: read-only dump of a table of records {pointer to UTF-16 name, then N ints} in the canonical database (run with -readOnly
// -noanalysis; nothing saved). Arguments: output directory, then "<hex address>:<count>[:<stride>[:<ints>]]" (stride default 16, ints
// default 2, read from +8). With stride 0x40 the byte at +0x3C is printed too. Output: table.txt, one line per record: index, name, ints...
// Used for the engine's builtin texture tables (473CBB0 x 35, 473CB78 x 3) and the 65 builtin material parameters (3D20580, stride 0x40,
// 4 ints: kind, animation selector, vector index, scalar offset; native NativeBuiltinModelTextures / NativeBuiltinModelParameters).
// @category DS3

import java.nio.charset.StandardCharsets;
import java.nio.file.*;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.mem.*;

public class I0068Table extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        Path out = Paths.get(args[0]);
        StringBuilder sb = new StringBuilder();
        Memory mem = currentProgram.getMemory();
        for (int i = 1; i < args.length; i++) {
            String[] f = args[i].split(":");
            long base = Long.parseLong(f[0], 16);
            int count = Integer.parseInt(f[1]);
            int stride = 16;
            if (f.length > 2) stride = f[2].startsWith("0x") ? Integer.parseInt(f[2].substring(2), 16) : Integer.parseInt(f[2]);
            int ints = f.length > 3 ? Integer.parseInt(f[3]) : 2;
            sb.append("# ").append(args[i]).append('\n');
            for (int k = 0; k < count; k++) {
                Address at = toAddr(base + (long) stride * k);
                long ptr = mem.getLong(at);
                StringBuilder name = new StringBuilder();
                if (ptr != 0) {
                    Address p = toAddr(ptr);
                    for (int c = 0; c < 256; c++) { int ch = mem.getShort(p.add(2L * c)) & 0xFFFF; if (ch == 0) break; name.append((char) ch); }
                }
                sb.append(k).append('\t').append(name);
                for (int j = 0; j < ints; j++) sb.append('\t').append(mem.getInt(at.add(8 + 4L * j)));
                if (stride == 0x40) sb.append('\t').append(mem.getByte(at.add(0x3C)) & 0xFF);
                sb.append('\n');
            }
        }
        Files.write(out.resolve("table.txt"), sb.toString().getBytes(StandardCharsets.UTF_8));
    }
}
