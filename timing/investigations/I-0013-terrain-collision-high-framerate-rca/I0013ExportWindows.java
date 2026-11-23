// Export bounded byte/listing windows from the exact I-0013 Ghidra program.
// @category DS3

import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.CodeUnit;

public class I0013ExportWindows extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 3 || (args.length - 1) % 2 != 0) {
            throw new IllegalArgumentException("Expected output then address/length pairs");
        }
        StringBuilder out = new StringBuilder("address,length,bytes,code_unit\n");
        for (int index = 1; index < args.length; index += 2) {
            Address address = toAddr(args[index]);
            int length = Integer.decode(args[index + 1]);
            byte[] bytes = new byte[length];
            int read = currentProgram.getMemory().getBytes(address, bytes);
            StringBuilder hex = new StringBuilder();
            for (int offset = 0; offset < read; ++offset) {
                hex.append(String.format("%02X", bytes[offset] & 0xff));
            }
            CodeUnit unit = currentProgram.getListing().getCodeUnitAt(address);
            String text = unit == null ? "" : unit.toString().replace("\"", "\"\"");
            out.append(address).append(',').append(read).append(',')
                .append(hex).append(",\"").append(text).append("\"\n");
        }
        Path target = Paths.get(args[0]);
        Files.createDirectories(target.getParent());
        Files.writeString(target, out.toString(), StandardCharsets.UTF_8);
    }
}
