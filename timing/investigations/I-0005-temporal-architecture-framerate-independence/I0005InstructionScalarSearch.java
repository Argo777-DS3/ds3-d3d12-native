// Find instructions carrying selected scalar values or textual displacements.
// Matches are discovery leads; they do not establish field semantics by themselves.
// @category DS3

import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.scalar.Scalar;

public class I0005InstructionScalarSearch extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 2) {
            throw new IllegalArgumentException("Expected output CSV then one or more scalar values");
        }
        Path output = Paths.get(args[0]);
        Set<Long> targets = new LinkedHashSet<>();
        for (int i = 1; i < args.length; i++) targets.add(Long.decode(args[i]));

        List<String> rows = new ArrayList<>();
        rows.add("searched_value,instruction,function_id,function_entry,function_name,mnemonic,text,operand_index,match_kind");
        InstructionIterator iterator = currentProgram.getListing().getInstructions(true);
        while (iterator.hasNext()) {
            monitor.checkCancelled();
            Instruction instruction = iterator.next();
            Function function = currentProgram.getFunctionManager().getFunctionContaining(instruction.getAddress());
            for (int operand = 0; operand < instruction.getNumOperands(); operand++) {
                for (Object object : instruction.getOpObjects(operand)) {
                    if (!(object instanceof Scalar)) continue;
                    long value = ((Scalar)object).getUnsignedValue();
                    if (!targets.contains(value)) continue;
                    rows.add(csv(hex(value), instruction.getAddress(),
                        function == null ? "" : stableId(function),
                        function == null ? "" : function.getEntryPoint(),
                        function == null ? "" : function.getName(),
                        instruction.getMnemonicString(), instruction.toString(), operand, "scalar"));
                }
            }
        }
        Files.createDirectories(output.getParent());
        Files.write(output, rows, StandardCharsets.UTF_8);
        println("Scalar instruction matches=" + (rows.size() - 1));
    }

    private String stableId(Function function) {
        return String.format("DS3-1152-F-%08X",
            function.getEntryPoint().subtract(currentProgram.getImageBase()));
    }

    private String hex(long value) { return String.format("0x%X", value); }

    private String csv(Object... values) {
        List<String> out = new ArrayList<>();
        for (Object value : values) {
            String text = value == null ? "" : value.toString();
            out.add("\"" + text.replace("\"", "\"\"") + "\"");
        }
        return String.join(",", out);
    }
}
