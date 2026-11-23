// Export references to native clock, wait, and scheduler imports relevant to temporal architecture.
// @category DS3

import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.*;

public class I0005ClockImports extends GhidraScript {
    private static final String[] TARGETS = {
        "QueryPerformanceCounter", "QueryPerformanceFrequency",
        "GetTickCount", "GetTickCount64", "timeGetTime",
        "GetSystemTime", "GetSystemTimeAsFileTime", "GetSystemTimePreciseAsFileTime",
        "GetLocalTime", "Sleep", "SleepEx", "WaitForSingleObject", "WaitForSingleObjectEx",
        "NtQueryPerformanceCounter"
    };

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 1) throw new IllegalArgumentException("Expected output CSV");
        List<String> rows = new ArrayList<>();
        rows.add("import,symbol_address,iat_or_thunk,reference_from,reference_type,function_id,function_entry,function_name");
        SymbolTable symbols = currentProgram.getSymbolTable();
        ReferenceManager references = currentProgram.getReferenceManager();
        for (String target : TARGETS) {
            SymbolIterator matches = symbols.getSymbols(target);
            boolean found = false;
            while (matches.hasNext()) {
                found = true;
                Symbol symbol = matches.next();
                Address imported = symbol.getAddress();
                ReferenceIterator level1 = references.getReferencesTo(imported);
                boolean hadLevel1 = false;
                while (level1.hasNext()) {
                    hadLevel1 = true;
                    Reference first = level1.next();
                    Address intermediate = first.getFromAddress();
                    ReferenceIterator level2 = references.getReferencesTo(intermediate);
                    boolean hadLevel2 = false;
                    while (level2.hasNext()) {
                        hadLevel2 = true;
                        Reference second = level2.next();
                        add(rows, target, imported, intermediate, second);
                    }
                    if (!hadLevel2) add(rows, target, imported, intermediate, first);
                }
                if (!hadLevel1) rows.add(csv(target, imported, "", "", "", "", "", ""));
            }
            if (!found) rows.add(csv(target, "NOT_FOUND", "", "", "", "", "", ""));
        }
        Path output = Paths.get(args[0]);
        Files.createDirectories(output.getParent());
        Files.write(output, rows, StandardCharsets.UTF_8);
        println("Clock import rows=" + (rows.size() - 1));
    }

    private void add(List<String> rows, String target, Address imported, Address intermediate, Reference ref) {
        Address from = ref.getFromAddress();
        Function function = currentProgram.getFunctionManager().getFunctionContaining(from);
        rows.add(csv(target, imported, intermediate, from, ref.getReferenceType(),
            function == null ? "" : stableId(function),
            function == null ? "" : function.getEntryPoint(),
            function == null ? "" : function.getName()));
    }

    private String stableId(Function function) {
        return String.format("DS3-1152-F-%08X",
            function.getEntryPoint().subtract(currentProgram.getImageBase()));
    }

    private String csv(Object... values) {
        List<String> out = new ArrayList<>();
        for (Object value : values) {
            String text = value == null ? "" : value.toString();
            out.add("\"" + text.replace("\"", "\"\"") + "\"");
        }
        return String.join(",", out);
    }
}
