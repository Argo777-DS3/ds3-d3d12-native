// Export direct accesses to the exact final-writer input offsets.
// This is a discovery inventory: equal displacements do not imply equal types.
// @category DS3

import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.symbol.RefType;

public class I0005WriterFieldProvenance extends GhidraScript {
    private static final Set<Long> TARGETS = new LinkedHashSet<>(Arrays.asList(
        0x68L, 0xa8L, 0xb0L, 0x1b8L, 0x1c0L, 0x1e1L, 0x1f90L));

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 1) {
            throw new IllegalArgumentException("Expected output CSV");
        }
        List<String> rows = new ArrayList<>();
        rows.add("offset,instruction,function_id,function_entry,function_name,mnemonic,text,operand,read,write");
        InstructionIterator iterator = currentProgram.getListing().getInstructions(true);
        while (iterator.hasNext()) {
            monitor.checkCancelled();
            Instruction instruction = iterator.next();
            Function function = currentProgram.getFunctionManager().getFunctionContaining(instruction.getAddress());
            for (int operand = 0; operand < instruction.getNumOperands(); operand++) {
                RefType refType = instruction.getOperandRefType(operand);
                for (Object object : instruction.getOpObjects(operand)) {
                    if (!(object instanceof Scalar)) continue;
                    long value = ((Scalar)object).getUnsignedValue();
                    if (!TARGETS.contains(value)) continue;
                    rows.add(csv(hex(value), instruction.getAddress(),
                        function == null ? "" : stableId(function),
                        function == null ? "" : function.getEntryPoint(),
                        function == null ? "" : function.getName(),
                        instruction.getMnemonicString(), instruction.toString(), operand,
                        refType != null && refType.isRead(),
                        refType != null && refType.isWrite()));
                }
            }
        }
        Path output = Paths.get(args[0]);
        Files.createDirectories(output.getParent());
        Files.write(output, rows, StandardCharsets.UTF_8);
        println("Writer-field access rows=" + (rows.size() - 1));
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
