// Read existing instructions without creating functions or changing analysis.
// @category DS3
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Instruction;
import ghidra.app.util.PseudoDisassembler;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
public class I0066ReadInstructions extends GhidraScript {
    public void run() throws Exception {
        String[] args=getScriptArgs();Path out=Paths.get(args[0]);Files.createDirectories(out);
        for(int i=1;i<args.length;i++) {
            Address at=toAddr(args[i]);StringBuilder lines=new StringBuilder();
            for(int n=0;n<32;n++) {
                Instruction ins=getInstructionAt(at);
                if(ins==null)ins=new PseudoDisassembler(currentProgram).disassemble(at);
                if(ins==null)break;
                lines.append(at).append("  ").append(ins.toString()).append('\n');
                if(ins.getMnemonicString().equals("RET") || ins.getMnemonicString().equals("JMP"))break;
                at=at.add(ins.getLength());
            }
            Files.write(out.resolve(args[i]+".asm.txt"),lines.toString().getBytes(StandardCharsets.UTF_8));
        }
    }
}
