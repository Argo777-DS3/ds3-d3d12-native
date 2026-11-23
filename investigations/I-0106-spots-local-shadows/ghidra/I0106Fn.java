// I-0106: read-only. For each address argument (hex VA): the function containing it, or, when Ghidra left the code
// undisassembled, the function created in memory only (disassemble + createFunction; never saved: -readOnly), then its
// callers (references to its entry), its assembly and its decompiled C. Arguments: output path, then addresses.
// @category DS3
import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.Reference;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;

public class I0106Fn extends GhidraScript {
    public void run() throws Exception {
        String[] args=getScriptArgs();StringBuilder text=new StringBuilder();
        DecompInterface decompiler=new DecompInterface();decompiler.openProgram(currentProgram);
        for(int i=1;i<args.length;++i){
            Address at=toAddr(args[i]);Function f=getFunctionContaining(at);String how="existing";
            if(f==null){
                DisassembleCommand cmd=new DisassembleCommand(at,null,true);cmd.applyTo(currentProgram,monitor);
                f=createFunction(at,null);how="created in memory (undisassembled in the project)";
            }
            text.append("=== "+at+" function="+(f==null?"none":f.getEntryPoint()+" "+f.getName()+" body="+f.getBody().getMinAddress()+".."+f.getBody().getMaxAddress())+" "+how+"\n");
            if(f==null){text.append("(no function)\n");continue;}
            text.append("callers:\n");
            for(Reference r:getReferencesTo(f.getEntryPoint())){Function c=getFunctionContaining(r.getFromAddress());
                text.append("  "+r.getFromAddress()+" "+r.getReferenceType()+" in "+(c==null?"none":c.getEntryPoint()+" "+c.getName())+"\n");}
            text.append("asm:\n");
            InstructionIterator it=currentProgram.getListing().getInstructions(f.getBody(),true);
            while(it.hasNext()){Instruction ins=it.next();text.append("  "+ins.getAddress()+"  "+ins+"\n");}
            DecompileResults result=decompiler.decompileFunction(f,120,monitor);
            text.append("C:\n"+(result.decompileCompleted()?result.getDecompiledFunction().getC():"(decompile failed: "+result.getErrorMessage()+")\n")+"\n");
        }
        decompiler.dispose();
        Files.writeString(Paths.get(args[0]),text.toString(),StandardCharsets.UTF_8);
    }
}
