// Dump all defined instructions in a bounded address range, independent of function boundaries.
// @category DS3

import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;

public class I0005DumpInstructionRange extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 3) throw new IllegalArgumentException("Expected output, start, end");
        Path output = Paths.get(args[0]);
        Address start = toAddr(args[1]);
        Address end = toAddr(args[2]);
        StringBuilder text = new StringBuilder();
        InstructionIterator iterator = currentProgram.getListing().getInstructions(start, true);
        while (iterator.hasNext()) {
            monitor.checkCancelled();
            Instruction instruction = iterator.next();
            if (instruction.getAddress().compareTo(end) > 0) break;
            Function function = currentProgram.getFunctionManager().getFunctionContaining(instruction.getAddress());
            text.append(instruction.getAddress()).append("  ")
                .append(String.format("%-8s", instruction.getMnemonicString())).append(" ")
                .append(instruction.toString()).append("  ; ")
                .append(function == null ? "NO_FUNCTION" : stableId(function)).append("\n");
        }
        Files.createDirectories(output.getParent());
        Files.writeString(output, text.toString(), StandardCharsets.UTF_8);
        println("Instruction range characters=" + text.length());
    }

    private String stableId(Function function) {
        return String.format("DS3-1152-F-%08X",
            function.getEntryPoint().subtract(currentProgram.getImageBase()));
    }
}
