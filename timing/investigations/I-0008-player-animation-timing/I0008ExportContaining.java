// Export listing and decompilation for the functions containing selected addresses.
// @category DS3

import java.nio.charset.StandardCharsets;
import java.nio.file.*;

import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;

public class I0008ExportContaining extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 2) {
            throw new IllegalArgumentException(
                "Expected output directory then one or more contained addresses");
        }
        if (args.length == 4 && args[1].equals("range")) {
            java.util.ArrayList<String> selected = new java.util.ArrayList<>(); selected.add(args[0]);
            var it = currentProgram.getFunctionManager().getFunctions(toAddr(args[2]), true);
            while(it.hasNext()) { Function f=it.next(); if(f.getEntryPoint().compareTo(toAddr(args[3])) >= 0) break; selected.add(f.getEntryPoint().toString()); }
            args=selected.toArray(new String[0]);
        }
        Path out = Paths.get(args[0]);
        Files.createDirectories(out);

        DecompInterface ifc = new DecompInterface();
        ifc.toggleCCode(true);
        ifc.toggleSyntaxTree(true);
        if (!ifc.openProgram(currentProgram)) {
            throw new IllegalStateException("Decompiler open failed");
        }
        try {
            for (int i = 1; i < args.length; i++) {
                Address requested = toAddr(args[i]);
                Function function =
                    currentProgram.getFunctionManager().getFunctionContaining(requested);
                if (function == null) {
                    println("No containing function at " + requested);
                    continue;
                }
                String id = stableId(function);
                StringBuilder refs = new StringBuilder();
                for (var ref : getReferencesTo(function.getEntryPoint())) refs.append(ref.getFromAddress()).append(" ").append(ref.getReferenceType()).append("\n");
                Files.writeString(out.resolve(id + ".xrefs.txt"), refs.toString(), StandardCharsets.UTF_8);
                StringBuilder listing = new StringBuilder();
                listing.append("# ").append(id)
                    .append(" entry=").append(function.getEntryPoint())
                    .append(" name=").append(function.getName())
                    .append(" requested=").append(requested).append("\n");
                InstructionIterator instructions =
                    currentProgram.getListing().getInstructions(function.getBody(), true);
                while (instructions.hasNext()) {
                    Instruction instruction = instructions.next();
                    for (byte b : instruction.getBytes()) listing.append(String.format("%02X", b & 255));
                    listing.append("  ").append(instruction.getAddress()).append("  ")
                        .append(String.format("%-8s", instruction.getMnemonicString()))
                        .append(" ").append(instruction.toString()).append("\n");
                }
                Files.writeString(out.resolve(id + ".asm.txt"), listing.toString(),
                    StandardCharsets.UTF_8);

                DecompileResults result = ifc.decompileFunction(function, 180, monitor);
                String text = "/* " + id + " entry=" + function.getEntryPoint() +
                    " name=" + function.getName() + " requested=" + requested +
                    " completed=" + result.decompileCompleted() + " */\n\n";
                if (result.decompileCompleted() && result.getDecompiledFunction() != null) {
                    text += result.getDecompiledFunction().getC();
                } else {
                    text += "/* " + result.getErrorMessage() + " */\n";
                }
                Files.writeString(out.resolve(id + ".c"), text, StandardCharsets.UTF_8);
            }
        } finally {
            ifc.dispose();
        }
    }

    private String stableId(Function function) {
        return String.format("DS3-1152-F-%08X",
            function.getEntryPoint().subtract(currentProgram.getImageBase()));
    }
}


