// Bounded discovery only: matching displacements do not establish object identity.
// @category DS3
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
public class I0007BoundedFieldLeads extends GhidraScript {
    public void run() throws Exception {
        String[] a=getScriptArgs();
        if(a.length!=3) throw new IllegalArgumentException("output, inclusive start, inclusive end required");
        Address start=toAddr(a[1]),end=toAddr(a[2]);
        if(end.subtract(start)>0x4000 || end.compareTo(start)<0) throw new IllegalArgumentException("bounded to16KiB");
        StringBuilder out=new StringBuilder("# Discovery leads only; range="+start+".."+end+"\n");
        InstructionIterator it=currentProgram.getListing().getInstructions(new AddressSet(start,end),true);
        while(it.hasNext()) {
            monitor.checkCancelled(); Instruction ins=it.next();
            String text=ins.toString();
            if(!text.matches(".*\\[.* \\+ 0x(10|30|40|50)\\].*")) continue;
            Function f=currentProgram.getFunctionManager().getFunctionContaining(ins.getAddress());
            out.append(ins.getAddress()).append(" function=").append(f==null?"unknown":f.getEntryPoint()).append(" ").append(text).append("\n");
        }
        Files.writeString(Paths.get(a[0]),out.toString(),StandardCharsets.UTF_8);
    }
}
