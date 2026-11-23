// I-0100 LOD scale study: read-only. Finds a 4-byte pattern (hex, little-endian, e.g. 0bbe753f for 0.959931f) in the
// initialized data blocks (not executable), lists the references to each hit and their functions, then decompiles up to
// N of those functions. Run only with -readOnly -noanalysis; nothing is saved. Arguments: output path, pattern, N.
// @category DS3
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.Reference;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

public class I0100FindFloat extends GhidraScript {
    public void run() throws Exception {
        String[] args=getScriptArgs();StringBuilder text=new StringBuilder();
        String hex=args[1];byte[] pattern=new byte[hex.length()/2];
        for(int i=0;i<pattern.length;++i)pattern[i]=(byte)Integer.parseInt(hex.substring(2*i,2*i+2),16);
        int limit=Integer.parseInt(args[2]);Set<Address> functions=new LinkedHashSet<>();
        for(MemoryBlock block:currentProgram.getMemory().getBlocks()){
            if(!block.isInitialized() || block.isExecute())continue;
            Address at=block.getStart();
            while(at!=null && at.compareTo(block.getEnd())<=0){
                Address hit=currentProgram.getMemory().findBytes(at,block.getEnd(),pattern,null,true,monitor);
                if(hit==null)break;
                Reference[] refs=getReferencesTo(hit);
                if(refs.length>0){text.append("=== "+block.getName()+" "+hit+" refs="+refs.length+"\n");
                    for(Reference r:refs){Address from=r.getFromAddress();Function f=getFunctionContaining(from);Instruction ins=getInstructionAt(from);
                        text.append("  "+from+" "+r.getReferenceType()+" fn="+(f==null?"none":f.getEntryPoint()+" "+f.getName())+" : "+(ins==null?"?":ins.toString())+"\n");
                        if(f!=null)functions.add(f.getEntryPoint());}}
                at=hit.add(1);
            }
        }
        DecompInterface decompiler=new DecompInterface();decompiler.openProgram(currentProgram);int n=0;
        for(Address entry:functions){if(n++>=limit)break;Function f=getFunctionAt(entry);
            DecompileResults result=decompiler.decompileFunction(f,120,monitor);
            text.append("\n=== function "+entry+" "+f.getName()+"\n"+(result.decompileCompleted()?result.getDecompiledFunction().getC():"(decompile failed)\n"));}
        decompiler.dispose();
        Files.writeString(Paths.get(args[0]),text.toString(),StandardCharsets.UTF_8);
    }
}
