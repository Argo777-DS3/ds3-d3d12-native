// Bounded instruction inventory around the already implicated owner functions.
// Matching offsets are leads, not proof of object identity.
// @category DS3
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.listing.*;

public class I0007AccumulatorFieldUsers extends GhidraScript {
    @Override public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 1) throw new IllegalArgumentException("Expected output CSV");
        List<String> rows = new ArrayList<>();
        rows.add("function_entry,instruction_address,instruction");
        InstructionIterator it = currentProgram.getListing().getInstructions(
            new AddressSet(toAddr("1409d0000"), toAddr("1409dffff")), true);
        while (it.hasNext()) {
            Instruction ins = it.next();
            String text = ins.toString().toLowerCase(Locale.ROOT);
            if (!text.contains("0x2d0") && !text.contains("0x2d8") && !text.contains("0x2e0")) continue;
            Function f = getFunctionContaining(ins.getAddress());
            rows.add(quote(f == null ? "" : f.getEntryPoint().toString()) + "," +
                quote(ins.getAddress().toString()) + "," + quote(ins.toString()));
        }
        Files.write(Paths.get(args[0]), rows, StandardCharsets.UTF_8);
        println("Bounded accumulator field-user rows=" + (rows.size() - 1));
    }
    private String quote(String s) { return "\"" + s.replace("\"", "\"\"") + "\""; }
}
