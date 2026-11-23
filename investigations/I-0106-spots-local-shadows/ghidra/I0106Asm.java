// I-0106: read-only. Assembly listing of address ranges (start end, hex VA) from the project's instructions; where the
// project has no instruction, one is disassembled in memory only (PseudoDisassembler, never saved). Arguments: output
// path, then pairs start end.
// @category DS3
import ghidra.app.script.GhidraScript;
import ghidra.app.util.PseudoDisassembler;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Instruction;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;

public class I0106Asm extends GhidraScript {
    public void run() throws Exception {
        String[] args=getScriptArgs();StringBuilder text=new StringBuilder();
        for(int i=1;i+1<args.length;i+=2){
            Address at=toAddr(args[i]);Address end=toAddr(args[i+1]);text.append("=== "+at+".."+end+"\n");
            while(at.compareTo(end)<0){
                Instruction ins=getInstructionAt(at);String tag="";
                if(ins==null){ins=new PseudoDisassembler(currentProgram).disassemble(at);tag=" ;pseudo";}
                if(ins==null){text.append(at+"  (no instruction)\n");at=at.add(1);continue;}
                text.append(at+"  "+ins+tag+"\n");at=at.add(ins.getLength());
            }
        }
        Files.writeString(Paths.get(args[0]),text.toString(),StandardCharsets.UTF_8);
    }
}
