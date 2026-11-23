// I-0107: read-only. Every occurrence in initialised memory of the given ASCII strings (and their UTF-16 form), with the
// references to the occurrence address (and, for a hit inside a longer string, to the start of that string). Arguments: output path, then strings.
// @category DS3
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.symbol.Reference;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;

public class I0107Str extends GhidraScript {
    public void run() throws Exception {
        String[] args=getScriptArgs();StringBuilder text=new StringBuilder();
        for(int i=1;i<args.length;++i){
            for(int wide=0;wide<2;++wide){
                byte[] pat=wide==0?args[i].getBytes(StandardCharsets.US_ASCII):args[i].getBytes(StandardCharsets.UTF_16LE);
                byte[] mask=new byte[pat.length];java.util.Arrays.fill(mask,(byte)0xff);
                Address at=currentProgram.getMinAddress();
                while(true){
                    Address hit=currentProgram.getMemory().findBytes(at,pat,mask,true,monitor);
                    if(hit==null)break;
                    // walk back to the string start (ASCII only)
                    Address start=hit;
                    if(wide==0){while(true){Address p=start.subtract(1);try{int b=currentProgram.getMemory().getByte(p)&0xff;if(b>=0x20&&b<0x7f)start=p;else break;}catch(Exception e){break;}}}
                    text.append(args[i]+(wide==1?" (wide)":"")+" at "+hit+" string start "+start+"\n");
                    for(Address a:new Address[]{hit,start}){
                        for(Reference r:getReferencesTo(a)){Function c=getFunctionContaining(r.getFromAddress());
                            text.append("   ref "+a+" <- "+r.getFromAddress()+" "+r.getReferenceType()+" in "+(c==null?"none":c.getEntryPoint()+" "+c.getName())+"\n");}
                        if(start.equals(hit))break;
                    }
                    at=hit.add(1);
                }
            }
        }
        Files.writeString(Paths.get(args[0]),text.toString(),StandardCharsets.UTF_8);
    }
}
