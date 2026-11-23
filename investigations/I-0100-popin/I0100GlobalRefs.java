// I-0100 LOD scale study: read-only. For each address argument (hex VA of a global), its initial bytes in the image (16)
// and every reference to it (from address, its function, read or write, the instruction), then each referencing
// function's decompiled C once. Run only with -readOnly -noanalysis; nothing is saved. Arguments: output path, then
// addresses.
// @category DS3
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.symbol.Reference;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

public class I0100GlobalRefs extends GhidraScript {
    public void run() throws Exception {
        String[] args=getScriptArgs();StringBuilder text=new StringBuilder();Set<Address> functions=new LinkedHashSet<>();
        for(int i=1;i<args.length;++i){
            Address at=toAddr(args[i]);byte[] bytes=new byte[16];
            try{currentProgram.getMemory().getBytes(at,bytes);}catch(Exception e){text.append("(bytes unreadable: "+e.getMessage()+")\n");}
            StringBuilder hex=new StringBuilder();for(byte b:bytes)hex.append(String.format("%02x ",b&0xff));
            text.append("=== "+at+" initialized="+currentProgram.getMemory().getBlock(at).isInitialized()+" block="+currentProgram.getMemory().getBlock(at).getName()+
                " bytes="+hex+" float0="+Float.intBitsToFloat(java.nio.ByteBuffer.wrap(bytes).order(java.nio.ByteOrder.LITTLE_ENDIAN).getInt(0))+
                " float4="+Float.intBitsToFloat(java.nio.ByteBuffer.wrap(bytes).order(java.nio.ByteOrder.LITTLE_ENDIAN).getInt(4))+"\n");
            for(int d=-8;d<=8;d+=4){Address near=at.add(d);
                for(Reference r:getReferencesTo(near)){Address from=r.getFromAddress();Function f=getFunctionContaining(from);Instruction ins=getInstructionAt(from);
                    text.append("  ["+(d>=0?"+":"")+d+"] "+from+" "+r.getReferenceType()+" fn="+(f==null?"none":f.getEntryPoint()+" "+f.getName())+" : "+(ins==null?"?":ins.toString())+"\n");
                    if(f!=null)functions.add(f.getEntryPoint());}}
        }
        DecompInterface decompiler=new DecompInterface();decompiler.openProgram(currentProgram);
        for(Address entry:functions){Function f=getFunctionAt(entry);
            DecompileResults result=decompiler.decompileFunction(f,120,monitor);
            text.append("\n=== function "+entry+" "+f.getName()+"\n"+(result.decompileCompleted()?result.getDecompiledFunction().getC():"(decompile failed)\n"));}
        decompiler.dispose();
        Files.writeString(Paths.get(args[0]),text.toString(),StandardCharsets.UTF_8);
    }
}
