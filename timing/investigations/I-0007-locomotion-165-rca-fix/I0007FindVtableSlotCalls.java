// Find indirect calls through vtable offsets +0x30/+0x38.
// @category DS3

import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;

public class I0007FindVtableSlotCalls extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 1) throw new IllegalArgumentException("Expected output CSV");
        List<String> rows = new ArrayList<>();
        rows.add("call,slot,function_id,function_entry,function_name,context");
        InstructionIterator iterator = currentProgram.getListing().getInstructions(true);
        while (iterator.hasNext() && !monitor.isCancelled()) {
            Instruction instruction = iterator.next();
            if (!instruction.getMnemonicString().equalsIgnoreCase("CALL")) continue;
            String text = instruction.toString();
            String slot = null;
            if (text.matches("(?i).*\\+\\s*0x30\\].*")) slot = "6";
            else if (text.matches("(?i).*\\+\\s*0x38\\].*")) slot = "7";
            if (slot == null) continue;
            Function function = currentProgram.getFunctionManager().getFunctionContaining(instruction.getAddress());
            List<String> context = new ArrayList<>();
            Instruction cursor = instruction;
            for (int count = 0; count < 9 && cursor != null; ++count) {
                context.add(cursor.getAddress() + " " + cursor.toString());
                cursor = currentProgram.getListing().getInstructionBefore(cursor.getAddress());
                if (cursor != null && function != null && !function.getBody().contains(cursor.getAddress())) break;
            }
            Collections.reverse(context);
            rows.add(csv(instruction.getAddress(), slot,
                function == null ? "" : stableId(function),
                function == null ? "" : function.getEntryPoint(),
                function == null ? "" : function.getName(), String.join(" | ", context)));
        }
        Path output = Paths.get(args[0]);
        Files.createDirectories(output.getParent());
        Files.write(output, rows, StandardCharsets.UTF_8);
        println("Indirect slot call rows=" + (rows.size() - 1));
    }
    private String stableId(Function function) {
        return String.format("DS3-1152-F-%08X",
            function.getEntryPoint().subtract(currentProgram.getImageBase()));
    }
    private String csv(Object... values) {
        List<String> escaped = new ArrayList<>();
        for (Object value : values) {
            String text = value == null ? "" : value.toString();
            escaped.add("\"" + text.replace("\"", "\"\"") + "\"");
        }
        return String.join(",", escaped);
    }
}
