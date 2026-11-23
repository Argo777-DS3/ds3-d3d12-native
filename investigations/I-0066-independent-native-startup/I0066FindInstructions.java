// I-0066 (1136): list every instruction whose text matches a regular expression, with its containing function.
// Run only with -readOnly -noanalysis; nothing is saved. Arguments: output path, Java regular expression.
// @category DS3
import java.nio.file.*;
import java.util.*;
import java.util.regex.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;

public class I0066FindInstructions extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        Path out = Paths.get(args[0]);
        Pattern pattern = Pattern.compile(args[1]);
        List<String> rows = new ArrayList<>();
        InstructionIterator it = currentProgram.getListing().getInstructions(true);
        while (it.hasNext() && !monitor.isCancelled()) {
            Instruction ins = it.next();
            String text = ins.toString();
            if (!pattern.matcher(text).find()) continue;
            Function f = getFunctionContaining(ins.getAddress());
            rows.add(ins.getAddress() + "\t" + (f == null ? "-" : f.getEntryPoint().toString()) + "\t" + text);
            if (rows.size() > 5000) break;
        }
        Files.write(out, rows);
    }
}
