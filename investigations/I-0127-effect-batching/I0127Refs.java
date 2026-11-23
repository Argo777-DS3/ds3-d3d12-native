// I-0127: read-only. Arguments: output path, mode, then hex VAs. Mode "refs": for each address, every reference to it
// (from, containing function, ref type, instruction). Mode "dec": the containing function's decompiled C (no refs).
// Mode "list": function entries in [first, second] address range. Mode "dis": the function's instruction listing. Run only with -readOnly -noanalysis; nothing is saved.
// @category DS3
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.Reference;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;

public class I0127Refs extends GhidraScript {
    public void run() throws Exception {
        String[] args=getScriptArgs();StringBuilder text=new StringBuilder();String mode=args[1];
        DecompInterface decompiler=new DecompInterface();decompiler.openProgram(currentProgram);
        for(int i=2;i<args.length;++i){
            Address at=toAddr(args[i]);
            if(mode.equals("list")){
                Address end=toAddr(args[++i]);
                text.append("=== functions in "+at+".."+end+"\n");
                for(Function f:currentProgram.getFunctionManager().getFunctions(at,true)){
                    if(f.getEntryPoint().compareTo(end)>0)break;
                    text.append(f.getEntryPoint()+"\t"+f.getBody().getNumAddresses()+"\t"+f.getName()+"\n");
                }
                continue;
            }
            if(mode.equals("mem")){
                byte[] b=new byte[32];currentProgram.getMemory().getBytes(at,b);StringBuilder h=new StringBuilder("=== bytes at "+at+": ");
                for(byte x:b)h.append(String.format("%02x ",x));text.append(h+"\n");continue;
            }
            if(mode.equals("refs")){
                text.append("=== refs to "+at+"\n");
                for(Reference r:getReferencesTo(at)){
                    Function f=getFunctionContaining(r.getFromAddress());Instruction ins=getInstructionAt(r.getFromAddress());
                    text.append(r.getFromAddress()+"\t"+(f==null?"-":f.getEntryPoint())+"\t"+r.getReferenceType()+"\t"+(ins==null?"":ins.toString())+"\n");
                }
            } else {
                Function f=getFunctionContaining(at);
                text.append("=== "+at+" function="+(f==null?"none":f.getEntryPoint()+" "+f.getName())+"\n");
                if(f==null)continue;
                if(mode.equals("dec")){
                    DecompileResults result=decompiler.decompileFunction(f,120,monitor);
                    text.append(result.decompileCompleted()?result.getDecompiledFunction().getC():"(decompile failed)\n");
                } else {
                    InstructionIterator it=currentProgram.getListing().getInstructions(f.getBody(),true);
                    while(it.hasNext()){Instruction ins=it.next();text.append(ins.getAddress()+"  "+ins+"\n");}
                }
            }
        }
        decompiler.dispose();
        Files.writeString(Paths.get(args[0]),text.toString(),StandardCharsets.UTF_8);
    }
}
