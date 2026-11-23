// I-0099: read-only. For each address argument (hex VA, a vtable or a function), the references to it and the functions that
// hold them, each decompiled once. Run only with -readOnly -noanalysis; nothing is saved. Arguments: output path, addresses.
// @category DS3
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Reference;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

public class I0099Refs extends GhidraScript {
    public void run() throws Exception {
        String[] args=getScriptArgs();StringBuilder text=new StringBuilder();
        DecompInterface decompiler=new DecompInterface();decompiler.openProgram(currentProgram);
        Set<Function> done=new HashSet<>();
        for(int i=1;i<args.length;++i){
            Address at=toAddr(args[i]);text.append("=== refs to "+at+"\n");List<Function> todo=new ArrayList<>();
            for(Reference r:getReferencesTo(at)){Function f=getFunctionContaining(r.getFromAddress());
                text.append("  from "+r.getFromAddress()+" "+r.getReferenceType()+" in "+(f==null?"none":f.getEntryPoint()+" "+f.getName())+"\n");
                if(f!=null && done.add(f) && todo.size()<12)todo.add(f);}
            for(Function f:todo){DecompileResults result=decompiler.decompileFunction(f,120,monitor);
                text.append("--- "+f.getEntryPoint()+" "+f.getName()+"\n"+(result.decompileCompleted()?result.getDecompiledFunction().getC():"(decompile failed)\n"));}
        }
        decompiler.dispose();
        Files.writeString(Paths.get(args[0]),text.toString(),StandardCharsets.UTF_8);
    }
}
