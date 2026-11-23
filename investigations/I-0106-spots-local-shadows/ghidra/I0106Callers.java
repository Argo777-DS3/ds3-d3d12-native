// I-0106: read-only. Caller tree upwards from function entries: for each address (hex VA), its callers (call and jump
// references from code, and data references from sections other than .pdata, i.e. vtable slots) to a depth, each with the
// function containing the reference. Arguments: output path, depth, then addresses.
// @category DS3
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Reference;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

public class I0106Callers extends GhidraScript {
    StringBuilder text=new StringBuilder();Set<Address> seen=new HashSet<>();
    void walk(Address entry,int depth,int max,String pad){
        Function f=getFunctionContaining(entry);Address key=f==null?entry:f.getEntryPoint();
        text.append(pad+key+" "+(f==null?"":f.getName())+"\n");
        if(depth>=max||!seen.add(key))return;
        List<Address> next=new ArrayList<>();
        for(Reference r:getReferencesTo(key)){
            String blk=currentProgram.getMemory().getBlock(r.getFromAddress())==null?"?":currentProgram.getMemory().getBlock(r.getFromAddress()).getName();
            if(blk.equals(".pdata"))continue;
            Function c=getFunctionContaining(r.getFromAddress());
            if(r.getReferenceType().isCall()||r.getReferenceType().isJump()){
                text.append(pad+"  <- "+r.getFromAddress()+" "+r.getReferenceType()+"\n");if(c!=null)next.add(c.getEntryPoint());}
            else text.append(pad+"  <- data "+r.getFromAddress()+" ("+blk+")\n");}
        for(Address a:next)walk(a,depth+1,max,pad+"      ");
    }
    public void run() throws Exception {
        String[] args=getScriptArgs();int max=Integer.parseInt(args[1]);
        for(int i=2;i<args.length;++i)walk(toAddr(args[i]),0,max,"");
        Files.writeString(Paths.get(args[0]),text.toString(),StandardCharsets.UTF_8);
    }
}
