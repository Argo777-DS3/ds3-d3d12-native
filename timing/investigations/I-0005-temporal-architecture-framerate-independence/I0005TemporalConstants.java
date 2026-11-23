// Scan exact float/double timing constants and export their code references.
// Raw matches are observations only; semantic classification requires data flow.
// @category DS3

import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.symbol.*;

public class I0005TemporalConstants extends GhidraScript {
    private static final LinkedHashMap<Integer,String> FLOATS = new LinkedHashMap<>();
    private static final LinkedHashMap<Long,String> DOUBLES = new LinkedHashMap<>();
    static {
        addFloat("1_div_60_f32", 1.0f / 60.0f);
        addFloat("60_f32", 60.0f);
        addFloat("1_div_30_f32", 1.0f / 30.0f);
        addFloat("30_f32", 30.0f);
        addFloat("1_div_120_f32", 1.0f / 120.0f);
        addFloat("120_f32", 120.0f);
        addDouble("1_div_60_f64", 1.0 / 60.0);
        addDouble("60_f64", 60.0);
        addDouble("1_div_30_f64", 1.0 / 30.0);
        addDouble("30_f64", 30.0);
        addDouble("1_div_120_f64", 1.0 / 120.0);
        addDouble("120_f64", 120.0);
    }

    private static void addFloat(String name, float value) {
        FLOATS.put(Float.floatToRawIntBits(value), name);
    }
    private static void addDouble(String name, double value) {
        DOUBLES.put(Double.doubleToRawLongBits(value), name);
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 1) throw new IllegalArgumentException("Expected output directory");
        Path out = Paths.get(args[0]);
        Files.createDirectories(out);
        List<String> memoryRows = new ArrayList<>();
        List<String> refsRows = new ArrayList<>();
        List<String> immediateRows = new ArrayList<>();
        memoryRows.add("constant,width,value_bits,location,block");
        refsRows.add("constant,width,constant_location,reference_from,reference_type,function_id,function_entry,function_name");
        immediateRows.add("constant,width,value_bits,instruction,operand_index,function_id,function_entry,function_name");
        Memory memory = currentProgram.getMemory();
        ReferenceManager references = currentProgram.getReferenceManager();
        for (MemoryBlock block : memory.getBlocks()) {
            if (!block.isInitialized()) continue;
            Address start = block.getStart();
            long length = block.getSize();
            long align4 = (4 - (start.getOffset() & 3)) & 3;
            for (long offset = align4; offset + 4 <= length; offset += 4) {
                monitor.checkCancelled();
                Address at = start.add(offset);
                int bits = memory.getInt(at);
                String name = FLOATS.get(bits);
                if (name != null) addMemory(memoryRows, refsRows, name, 32, hex(bits & 0xffffffffL), at, block.getName(), references);
            }
            long align8 = (8 - (start.getOffset() & 7)) & 7;
            for (long offset = align8; offset + 8 <= length; offset += 8) {
                monitor.checkCancelled();
                Address at = start.add(offset);
                long bits = memory.getLong(at);
                String name = DOUBLES.get(bits);
                if (name != null) addMemory(memoryRows, refsRows, name, 64, hex(bits), at, block.getName(), references);
            }
        }

        InstructionIterator instructions = currentProgram.getListing().getInstructions(true);
        while (instructions.hasNext()) {
            monitor.checkCancelled();
            Instruction instruction = instructions.next();
            for (int operand = 0; operand < instruction.getNumOperands(); operand++) {
                for (Object object : instruction.getOpObjects(operand)) {
                    if (!(object instanceof Scalar)) continue;
                    Scalar scalar = (Scalar)object;
                    long value = scalar.getUnsignedValue();
                    String name = scalar.bitLength() <= 32 ? FLOATS.get((int)value) : null;
                    int width = 32;
                    if (name == null) { name = DOUBLES.get(value); width = 64; }
                    if (name == null) continue;
                    Function function = currentProgram.getFunctionManager().getFunctionContaining(instruction.getAddress());
                    immediateRows.add(csv(name, width, hex(value), instruction.getAddress(), operand,
                        function == null ? "" : stableId(function),
                        function == null ? "" : function.getEntryPoint(),
                        function == null ? "" : function.getName()));
                }
            }
        }
        Files.write(out.resolve("constant-locations.csv"), memoryRows, StandardCharsets.UTF_8);
        Files.write(out.resolve("constant-references.csv"), refsRows, StandardCharsets.UTF_8);
        Files.write(out.resolve("constant-immediates.csv"), immediateRows, StandardCharsets.UTF_8);
        println("Temporal constants locations=" + (memoryRows.size()-1) + ", refs=" + (refsRows.size()-1) + ", immediates=" + (immediateRows.size()-1));
    }

    private void addMemory(List<String> memoryRows, List<String> refsRows, String name, int width,
            String bits, Address at, String block, ReferenceManager references) {
        memoryRows.add(csv(name, width, bits, at, block));
        ReferenceIterator iterator = references.getReferencesTo(at);
        while (iterator.hasNext()) {
            Reference reference = iterator.next();
            Function function = currentProgram.getFunctionManager().getFunctionContaining(reference.getFromAddress());
            refsRows.add(csv(name, width, at, reference.getFromAddress(), reference.getReferenceType(),
                function == null ? "" : stableId(function),
                function == null ? "" : function.getEntryPoint(),
                function == null ? "" : function.getName()));
        }
    }

    private String stableId(Function function) {
        return String.format("DS3-1152-F-%08X", function.getEntryPoint().subtract(currentProgram.getImageBase()));
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
