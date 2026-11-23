// Bounded primary listing when a retained function body omits later branches.
// Run only with -readOnly -noanalysis; no database changes are saved.
// Arguments: output path, inclusive start, exclusive end (hex VAs).
// @category DS3
import ghidra.app.script.GhidraScript;
import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Instruction;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;

public class I0066BoundedCode extends GhidraScript {
    public void run() throws Exception {
        String[] args=getScriptArgs();
        Address start=toAddr(args[1]),end=toAddr(args[2]);
        if(end.subtract(start)<=0 || end.subtract(start)>0x2000)throw new IllegalArgumentException("Bounded range required");
        new DisassembleCommand(start,null,true).applyTo(currentProgram,monitor);
        StringBuilder text=new StringBuilder("# inclusive "+start+" exclusive "+end+" read-only bounded code\n");
        for(Address at=start;at.compareTo(end)<0;) {
            Instruction instruction=currentProgram.getListing().getInstructionAt(at);
            if(instruction==null) {
                new DisassembleCommand(at,null,true).applyTo(currentProgram,monitor);
                instruction=currentProgram.getListing().getInstructionAt(at);
            }
            if(instruction==null){text.append(at+" undefined; stop\n");break;}
            text.append(at+" "+instruction.toString()+"\n");
            at=instruction.getMaxAddress().next();
        }
        Files.writeString(Paths.get(args[0]),text.toString(),StandardCharsets.UTF_8);
    }
}
