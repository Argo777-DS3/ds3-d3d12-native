// I-0104: read-only. For each argument "<vtable hex>:<slots>" the slot functions (index, entry, name), then each distinct slot
// function decompiled once; a bare "<address hex>" argument is a function decompiled as is. Run only with -readOnly
// -noanalysis; nothing is saved. Arguments: output path, then the vtables and addresses.
// @category DS3
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

public class I0104Vt extends GhidraScript {
    public void run() throws Exception {
        String[] args=getScriptArgs();StringBuilder text=new StringBuilder();
        DecompInterface decompiler=new DecompInterface();decompiler.openProgram(currentProgram);
        LinkedHashSet<Address> todo=new LinkedHashSet<>();
        for(int i=1;i<args.length;++i){
            String[] p=args[i].split(":");Address at=toAddr(p[0]);
            if(p.length<2){todo.add(at);continue;}
            int n=Integer.parseInt(p[1]);text.append("=== vtable "+at+"\n");
            for(int s=0;s<n;++s){long target=getLong(at.add(8L*s));Address t=toAddr(target);Function f=getFunctionAt(t);
                if(f==null){f=createFunction(t,null);}
                text.append(String.format("  slot %d (+0x%X) %s %s\n",s,8*s,t,f==null?"(no function)":f.getName()));
                if(f!=null)todo.add(f.getEntryPoint());}
        }
        for(Address a:todo){Function f=getFunctionContaining(a);if(f==null){f=createFunction(a,null);}
            text.append("--- "+a+" "+(f==null?"(no function)":f.getEntryPoint()+" "+f.getName())+"\n");
            if(f==null)continue;
            DecompileResults result=decompiler.decompileFunction(f,180,monitor);
            text.append(result.decompileCompleted()?result.getDecompiledFunction().getC():"(decompile failed: "+result.getErrorMessage()+")\n");}
        decompiler.dispose();
        Files.writeString(Paths.get(args[0]),text.toString(),StandardCharsets.UTF_8);
    }
}
