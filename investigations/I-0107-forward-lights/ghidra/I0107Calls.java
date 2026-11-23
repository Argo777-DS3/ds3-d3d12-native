// I-0107: read-only. For each function-entry argument: its calls in address order (call address, target entry and name,
// indirect calls marked), to read the order of the passes a driver function records. Arguments: output path, then entries (hex VA).
// @category DS3
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;

public class I0107Calls extends GhidraScript {
    public void run() throws Exception {
        String[] args=getScriptArgs();StringBuilder text=new StringBuilder();
        for(int i=1;i<args.length;++i){
            Function f=getFunctionContaining(toAddr(args[i]));if(f==null){text.append("no function at "+args[i]+"\n");continue;}
            text.append("=== "+f.getEntryPoint()+" "+f.getName()+" body "+f.getBody().getMinAddress()+".."+f.getBody().getMaxAddress()+"\n");
            InstructionIterator it=currentProgram.getListing().getInstructions(f.getBody(),true);
            while(it.hasNext()){Instruction ins=it.next();
                if(ins.getFlowType().isCall()){
                    String t="indirect";
                    for(Reference r:ins.getReferencesFrom()){if(r.getReferenceType().isCall()){Function g=getFunctionAt(r.getToAddress());t=r.getToAddress()+(g==null?"":" "+g.getName());}}
                    text.append("  "+ins.getAddress()+" call "+t+"   ["+ins+"]\n");}
            }
        }
        Files.writeString(Paths.get(args[0]),text.toString(),StandardCharsets.UTF_8);
    }
}
