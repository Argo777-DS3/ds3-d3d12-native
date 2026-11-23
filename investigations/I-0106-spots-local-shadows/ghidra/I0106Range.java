// I-0106: read-only. Walks code ranges: at each address, the function containing it (or one created in memory only when
// the project left the code undisassembled; never saved: -readOnly), prints entry/size/callers and the decompiled C,
// then continues after the function; 0xCC padding is skipped. Arguments: output path, then pairs of start end (hex VA).
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

public class I0106Range extends GhidraScript {
    public void run() throws Exception {
        String[] args=getScriptArgs();StringBuilder text=new StringBuilder();
        DecompInterface decompiler=new DecompInterface();decompiler.openProgram(currentProgram);
        for(int i=1;i+1<args.length;i+=2){
            Address at=toAddr(args[i]);Address end=toAddr(args[i+1]);
            while(at.compareTo(end)<0){
                int b=getByte(at)&0xff;
                if(b==0xCC||b==0x90&&getFunctionContaining(at)==null){at=at.add(1);continue;}
                Function f=getFunctionContaining(at);String how="existing";
                if(f==null){
                    new DisassembleCommand(at,null,true).applyTo(currentProgram,monitor);
                    f=createFunction(at,null);how="created in memory";
                    if(f!=null&&f.getBody().getMaxAddress().compareTo(end.add(0x1000))>0){removeFunction(f);f=null;}
                    if(f==null){text.append("=== "+at+" no function could be made\n");at=at.add(1);continue;}
                }
                text.append("=== "+f.getEntryPoint()+" "+f.getName()+" "+f.getBody().getMinAddress()+".."+f.getBody().getMaxAddress()+" "+how+"\ncallers:");
                for(Reference r:getReferencesTo(f.getEntryPoint())){Function c=getFunctionContaining(r.getFromAddress());
                    text.append(" "+r.getFromAddress()+"("+(c==null?"none":c.getEntryPoint().toString())+")");}
                text.append("\n");
                DecompileResults result=decompiler.decompileFunction(f,120,monitor);
                text.append(result.decompileCompleted()?result.getDecompiledFunction().getC():"(decompile failed: "+result.getErrorMessage()+")\n").append("\n");
                Address next=f.getBody().getMaxAddress().add(1);
                at=next.compareTo(at)>0?next:at.add(1);
            }
        }
        decompiler.dispose();
        Files.writeString(Paths.get(args[0]),text.toString(),StandardCharsets.UTF_8);
    }
}
