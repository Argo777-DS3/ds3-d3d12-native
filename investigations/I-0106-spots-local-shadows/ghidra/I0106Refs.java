// I-0106: read-only. For each address argument (hex VA of data, e.g. a vtable): 24 qwords from it (with the
// function each points to), and every reference to the address from code (from, its function, the instruction).
// Arguments: output path, qword count, then addresses.
// @category DS3
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.Reference;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;

public class I0106Refs extends GhidraScript {
    public void run() throws Exception {
        String[] args=getScriptArgs();StringBuilder text=new StringBuilder();int n=Integer.parseInt(args[1]);
        for(int i=2;i<args.length;++i){
            Address at=toAddr(args[i]);text.append("=== "+at+"\n");
            for(int k=0;k<n;++k){long q=currentProgram.getMemory().getLong(at.add(8*k));Address t=toAddr(q);Function f=null;
                try{f=getFunctionContaining(t);}catch(Exception e){}
                text.append("  +"+Integer.toHexString(8*k)+" "+String.format("%016x",q)+" fn="+(f==null?"none":f.getEntryPoint()+" "+f.getName())+"\n");}
            text.append("refs:\n");
            for(Reference r:getReferencesTo(at)){Function f=getFunctionContaining(r.getFromAddress());Instruction ins=getInstructionAt(r.getFromAddress());
                text.append("  "+r.getFromAddress()+" "+r.getReferenceType()+" fn="+(f==null?"none":f.getEntryPoint()+" "+f.getName())+" : "+(ins==null?"?":ins.toString())+"\n");}
        }
        Files.writeString(Paths.get(args[0]),text.toString(),StandardCharsets.UTF_8);
    }
}
