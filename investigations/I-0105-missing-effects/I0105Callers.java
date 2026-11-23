// I-0105: read-only. For each address argument (hex VA of a function), the callers (reference sites and their functions), no
// decompilation. Run only with -readOnly -noanalysis; nothing is saved. Arguments: output path, addresses.
// @category DS3
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Reference;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;

public class I0105Callers extends GhidraScript {
    public void run() throws Exception {
        String[] args=getScriptArgs();StringBuilder text=new StringBuilder();
        for(int i=1;i<args.length;++i){
            Address at=toAddr(args[i]);text.append("=== refs to "+at+"\n");
            for(Reference r:getReferencesTo(at)){Function f=getFunctionContaining(r.getFromAddress());
                text.append("  from "+r.getFromAddress()+" "+r.getReferenceType()+" in "+(f==null?"none":f.getEntryPoint()+" "+f.getName())+"\n");}
        }
        Files.writeString(Paths.get(args[0]),text.toString(),StandardCharsets.UTF_8);
    }
}
