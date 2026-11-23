// scratch: list instructions whose text contains any of the substring args (args[0]=output path). Read-only.
// @category DS3
import java.nio.file.*;
import java.util.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;

public class ScanSubstr extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        List<String> rows = new ArrayList<>();
        InstructionIterator it = currentProgram.getListing().getInstructions(true);
        while (it.hasNext() && !monitor.isCancelled()) {
            Instruction ins = it.next();
            String text = ins.toString();
            for (int i = 1; i < args.length; i++) {
                if (text.contains(args[i])) {
                    Function f = getFunctionContaining(ins.getAddress());
                    rows.add(args[i] + "\t" + ins.getAddress() + "\t" + (f == null ? "-" : f.getEntryPoint().toString()) + "\t" + text);
                    break;
                }
            }
        }
        Files.write(Paths.get(args[0]), rows);
    }
}
