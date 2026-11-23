// Dump a bounded dword range and resolve values as image-relative function RVAs.
// @category DS3

import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.mem.Memory;

public class I0005DumpRvaDwords extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 3) {
            throw new IllegalArgumentException("Expected output CSV, start address, end address");
        }
        Path output = Paths.get(args[0]);
        Address start = toAddr(args[1]);
        Address end = toAddr(args[2]);
        Address imageBase = currentProgram.getImageBase();
        Memory memory = currentProgram.getMemory();
        List<String> rows = new ArrayList<>();
        rows.add("location,dword_hex,unsigned_value,resolved_address,function_id,function_entry,function_name");
        for (Address at = start; at.compareTo(end) <= 0; at = at.add(4)) {
            monitor.checkCancelled();
            long value = Integer.toUnsignedLong(memory.getInt(at));
            Address target = imageBase.add(value);
            Function function = currentProgram.getFunctionManager().getFunctionAt(target);
            rows.add(csv(at, String.format("0x%08X", value), value, target,
                function == null ? "" : stableId(function),
                function == null ? "" : function.getEntryPoint(),
                function == null ? "" : function.getName()));
        }
        Files.createDirectories(output.getParent());
        Files.write(output, rows, StandardCharsets.UTF_8);
        println("RVA dword rows=" + (rows.size() - 1));
    }

    private String stableId(Function function) {
        return String.format("DS3-1152-F-%08X",
            function.getEntryPoint().subtract(currentProgram.getImageBase()));
    }

    private String csv(Object... values) {
        List<String> out = new ArrayList<>();
        for (Object value : values) {
            String text = value == null ? "" : value.toString();
            out.add("\"" + text.replace("\"", "\"\"") + "\"");
        }
        return String.join(",", out);
    }
}
