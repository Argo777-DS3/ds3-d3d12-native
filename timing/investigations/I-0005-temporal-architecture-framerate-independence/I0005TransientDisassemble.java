// Transiently disassemble selected protected/obfuscated entry points and roll back all changes.
// @category DS3

import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;

public class I0005TransientDisassemble extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 3) throw new IllegalArgumentException("Expected output, max bytes, addresses");
        Path output = Paths.get(args[0]);
        long maxBytes = Long.decode(args[1]);
        StringBuilder text = new StringBuilder();
        int transaction = currentProgram.startTransaction("I-0005 transient disassembly");
        try {
            for (int i = 2; i < args.length; i++) {
                Address start = toAddr(args[i]);
                boolean changed = disassemble(start);
                text.append("# start=").append(start).append(" disassembled=").append(changed).append("\n");
                Address end = start.add(maxBytes);
                InstructionIterator iterator = currentProgram.getListing().getInstructions(start, true);
                while (iterator.hasNext()) {
                    Instruction instruction = iterator.next();
                    if (instruction.getAddress().compareTo(end) >= 0) break;
                    text.append(instruction.getAddress()).append("  ")
                        .append(String.format("%-8s", instruction.getMnemonicString())).append(" ")
                        .append(instruction.toString()).append("\n");
                }
                text.append("\n");
            }
        }
        finally {
            currentProgram.endTransaction(transaction, false);
        }
        Files.createDirectories(output.getParent());
        Files.writeString(output, text.toString(), StandardCharsets.UTF_8);
        println("Transient disassembly characters=" + text.length());
    }
}
