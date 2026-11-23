// Export exact instruction users of the candidate producer plugin-list fields.
// @category DS3

import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;

public class I0007FindPluginListFieldUsers extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 1) {
            throw new IllegalArgumentException("Expected output CSV path");
        }

        List<String> rows = new ArrayList<>();
        rows.add("function_id,function_entry,function_name,instruction_address,instruction");
        InstructionIterator instructions = currentProgram.getListing().getInstructions(true);
        while (instructions.hasNext()) {
            Instruction instruction = instructions.next();
            String text = instruction.toString();
            if (!text.contains("+ 0x100]") && !text.contains("+ 0x108]")) {
                continue;
            }
            Function function = currentProgram.getFunctionManager()
                .getFunctionContaining(instruction.getAddress());
            rows.add(csv(
                function == null ? "" : stableId(function),
                function == null ? "" : function.getEntryPoint(),
                function == null ? "" : function.getName(),
                instruction.getAddress(),
                text));
        }

        Path output = Paths.get(args[0]);
        Files.createDirectories(output.getParent());
        Files.write(output, rows, StandardCharsets.UTF_8);
        println("Exact field-user rows=" + (rows.size() - 1));
    }

    private String stableId(Function function) {
        return String.format("DS3-1152-F-%08X",
            function.getEntryPoint().subtract(currentProgram.getImageBase()));
    }

    private String csv(Object... values) {
        List<String> encoded = new ArrayList<>();
        for (Object value : values) {
            String text = value == null ? "" : value.toString();
            encoded.add("\"" + text.replace("\"", "\"\"") + "\"");
        }
        return String.join(",", encoded);
    }
}
