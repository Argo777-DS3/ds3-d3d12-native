// Study thread (I-0111 cadence T0): a read-only linear listing of an address range with the pseudo-disassembler (nothing is
// created in the program; run with -readOnly -noanalysis). Arguments: output path, then start and end addresses (hex).
// @category DS3
import ghidra.app.script.GhidraScript;
import ghidra.app.util.PseudoDisassembler;
import ghidra.app.util.PseudoInstruction;
import ghidra.program.model.address.Address;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;

public class I0111LinearListing extends GhidraScript {
    public void run() throws Exception {
        String[] args=getScriptArgs();StringBuilder out=new StringBuilder();
        PseudoDisassembler pd=new PseudoDisassembler(currentProgram);
        for(int r=1;r+1<args.length;r+=2){
            Address a=toAddr(args[r]);Address end=toAddr(args[r+1]);
            out.append("=== listing "+a+".."+end+"\n");
            while(a.compareTo(end)<0 && !monitor.isCancelled()){
                PseudoInstruction ins=null;
                try{ins=pd.disassemble(a);}catch(Exception e){ins=null;}
                if(ins==null){out.append(a+"  ??\n");a=a.add(1);continue;}
                out.append(a+"  "+ins.toString()+"\n");a=a.add(ins.getLength());
            }
        }
        Files.writeString(Paths.get(args[0]),out.toString(),StandardCharsets.UTF_8);
    }
}
