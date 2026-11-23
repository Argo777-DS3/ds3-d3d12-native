// List functions in a small selected exact-build range.
// @category DS3
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
public class I0007ListFunctionsRange extends GhidraScript {
    public void run() throws Exception {
        String[] a=getScriptArgs();
        if(a.length!=3) throw new IllegalArgumentException("output, inclusive start, inclusive end required");
        Address start=toAddr(a[1]),end=toAddr(a[2]);
        if(end.compareTo(start)<0 || end.subtract(start)>0x10000) throw new IllegalArgumentException("bounded to64KiB");
        StringBuilder out=new StringBuilder("entry,name,body_min,body_max\n");
        FunctionIterator it=currentProgram.getFunctionManager().getFunctions(new AddressSet(start,end),true);
        while(it.hasNext()) { Function f=it.next(); out.append(f.getEntryPoint()).append(',').append(f.getName()).append(',').append(f.getBody().getMinAddress()).append(',').append(f.getBody().getMaxAddress()).append('\n'); }
        Path p=Paths.get(a[0]); Files.createDirectories(p.getParent()); Files.writeString(p,out.toString(),StandardCharsets.UTF_8);
    }
}
