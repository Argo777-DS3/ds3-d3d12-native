// Read-only: for each address argument (hex VA), the containing function's entry and name, and the instruction
// just before it (the call that returned there). Run only with -readOnly -noanalysis; nothing is saved.
// Arguments: output path, then addresses.
// @category DS3
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;

public class I0066FunctionOf extends GhidraScript {
    public void run() throws Exception {
        String[] args=getScriptArgs();StringBuilder text=new StringBuilder();
        for(int i=1;i<args.length;++i){
            Address at=toAddr(args[i]);Function f=getFunctionContaining(at);
            Instruction before=getInstructionBefore(at);
            text.append(at+" function="+(f==null?"none":f.getEntryPoint()+" "+f.getName())+" call="+(before==null?"none":before.getAddress()+" "+before)+"\n");
        }
        Files.writeString(Paths.get(args[0]),text.toString(),StandardCharsets.UTF_8);
    }
}
