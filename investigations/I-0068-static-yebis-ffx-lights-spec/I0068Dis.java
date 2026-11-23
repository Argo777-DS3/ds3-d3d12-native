// I-0068: read-only disassembly of code Ghidra never defined as a function (tiny vtable setters such as the GXFfxDrawContext slots
// +8..+0x60). Run with -readOnly -noanalysis; the disassembly lives in the in-memory program only, nothing is saved.
// Arguments: output directory, then "<hex address>[:<instruction count>]" (the listing runs through RET, it stops after the count). Output: dis.txt.
// @category DS3

import java.nio.charset.StandardCharsets;
import java.nio.file.*;

import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;

public class I0068Dis extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        Path out = Paths.get(args[0]);
        StringBuilder sb = new StringBuilder();
        for (int i = 1; i < args.length; i++) {
            String[] f = args[i].split(":");
            Address a = toAddr(Long.parseLong(f[0], 16));
            int max = f.length > 1 ? Integer.parseInt(f[1]) : 24;
            sb.append("# ").append(args[i]).append('\n');
            if (getInstructionAt(a) == null) {
                int tx = currentProgram.startTransaction("dis");
                new DisassembleCommand(a, null, true).applyTo(currentProgram, monitor);
                currentProgram.endTransaction(tx, false);
            }
            Instruction ins = getInstructionAt(a);
            int n = 0;
            while (ins != null && n < max) {
                StringBuilder by = new StringBuilder();
                for (byte b : ins.getBytes()) by.append(String.format("%02x", b));
                sb.append(String.format("%s  %-16s %s%n", ins.getAddress(), by, ins.toString()));
                if (ins.getMnemonicString().equals("RET") && n >= max - 1) break;
                ins = ins.getNext();
                n++;
            }
        }
        Files.write(out.resolve("dis.txt"), sb.toString().getBytes(StandardCharsets.UTF_8));
    }
}
