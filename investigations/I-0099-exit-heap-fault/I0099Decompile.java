// I-0099: read-only. For each address argument (hex VA), the containing function's entry, name and the instruction just
// before the address (the call that returned there), then the function's decompiled C. Run only with -readOnly
// -noanalysis; nothing is saved. Arguments: output path, then addresses.
// @category DS3
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;

public class I0099Decompile extends GhidraScript {
    public void run() throws Exception {
        String[] args=getScriptArgs();StringBuilder text=new StringBuilder();
        DecompInterface decompiler=new DecompInterface();decompiler.openProgram(currentProgram);
        for(int i=1;i<args.length;++i){
            Address at=toAddr(args[i]);Function f=getFunctionContaining(at);Instruction before=getInstructionBefore(at);
            text.append("=== "+at+" function="+(f==null?"none":f.getEntryPoint()+" "+f.getName())+" call="+
                (before==null?"none":before.getAddress()+" "+before)+"\n");
            if(f==null)continue;
            DecompileResults result=decompiler.decompileFunction(f,120,monitor);
            text.append(result.decompileCompleted()?result.getDecompiledFunction().getC():"(decompile failed: "+result.getErrorMessage()+")\n");
        }
        decompiler.dispose();
        Files.writeString(Paths.get(args[0]),text.toString(),StandardCharsets.UTF_8);
    }
}
