// I-0106: read-only. Every instruction of the project whose operand carries one of the given displacements/constants
// (hex), optionally within an address range: address, function, write-or-read (first operand a memory operand = store),
// the instruction. Arguments: output path, range start (hex VA or 0), range end (hex VA or 0), then displacements.
// @category DS3
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.scalar.Scalar;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

public class I0106Disp extends GhidraScript {
    public void run() throws Exception {
        String[] args=getScriptArgs();StringBuilder text=new StringBuilder();
        Set<Long> wanted=new HashSet<>();for(int i=3;i<args.length;++i)wanted.add(Long.parseLong(args[i],16));
        AddressSetView set=currentProgram.getMemory().getExecuteSet();
        if(!args[1].equals("0"))set=new AddressSet(toAddr(args[1]),toAddr(args[2]));
        InstructionIterator it=currentProgram.getListing().getInstructions(set,true);
        while(it.hasNext()){Instruction ins=it.next();
            for(int op=0;op<ins.getNumOperands();++op){boolean hit=false;
                for(Object o:ins.getOpObjects(op)){if(o instanceof Scalar&&wanted.contains(((Scalar)o).getUnsignedValue()))hit=true;
                    if(o instanceof Scalar&&wanted.contains(((Scalar)o).getSignedValue()&0xffffffffL))hit=true;}
                if(hit){Function f=getFunctionContaining(ins.getAddress());
                    boolean store=op==0&&ins.getOperandRefType(0).isWrite();
                    text.append(ins.getAddress()+" "+(f==null?"none":f.getEntryPoint().toString())+" "+(store?"W":"R")+" op"+op+" : "+ins+"\n");break;}}
        }
        Files.writeString(Paths.get(args[0]),text.toString(),StandardCharsets.UTF_8);
    }
}
