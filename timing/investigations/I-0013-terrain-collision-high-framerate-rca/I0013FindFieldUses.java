// Find bounded x64 memory operands using one structure displacement.
// @category DS3

import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;
import java.util.regex.*;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;

public class I0013FindFieldUses extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 4) {
            throw new IllegalArgumentException(
                "Expected output CSV, displacement hex, start address and end address");
        }
        Path output = Paths.get(args[0]);
        String displacement = args[1].toLowerCase(Locale.ROOT).replaceFirst("^0x", "");
        Address start = toAddr(args[2]);
        Address end = toAddr(args[3]);
        Pattern field = Pattern.compile("(?i)\\+\\s*0x0*" + Pattern.quote(displacement) + "\\]");
        Pattern stack = Pattern.compile("(?i)\\[(RSP|RBP)\\s*\\+");

        List<String> rows = new ArrayList<>();
        rows.add("instruction,function_entry,function_id,mnemonic,bytes,text");
        InstructionIterator instructions =
            currentProgram.getListing().getInstructions(start, true);
        while (instructions.hasNext()) {
            Instruction instruction = instructions.next();
            if (instruction.getAddress().compareTo(end) >= 0) break;
            String text = instruction.toString();
            if (!field.matcher(text).find() || stack.matcher(text).find()) continue;
            Function function = currentProgram.getFunctionManager()
                .getFunctionContaining(instruction.getAddress());
            String bytes = bytes(instruction.getBytes());
            rows.add(csv(
                instruction.getAddress(),
                function == null ? "" : function.getEntryPoint(),
                function == null ? "" : stableId(function),
                instruction.getMnemonicString(),
                bytes,
                text));
        }
        Files.createDirectories(output.getParent());
        Files.write(output, rows, StandardCharsets.UTF_8);
        println("Field-use rows=" + (rows.size() - 1));
    }

    private String stableId(Function function) {
        return String.format("DS3-1152-F-%08X",
            function.getEntryPoint().subtract(currentProgram.getImageBase()));
    }

    private String bytes(byte[] values) {
        StringBuilder output = new StringBuilder();
        for (byte value : values) output.append(String.format("%02X", value & 0xff));
        return output.toString();
    }

    private String csv(Object... values) {
        List<String> output = new ArrayList<>();
        for (Object value : values) {
            String text = value == null ? "" : value.toString();
            output.add("\"" + text.replace("\"", "\"\"") + "\"");
        }
        return String.join(",", output);
    }
}
