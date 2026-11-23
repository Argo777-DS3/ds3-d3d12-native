// I-0107: read-only. Every call to a target function (references to its entry) whose preceding 10 instructions (same function,
// linear order) carry one of the given immediates; prints the call, the enclosing function and the instructions.
// Arguments: output path, target function entry (hex VA), then immediates (hex).
// @category DS3
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.symbol.Reference;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

public class I0107CallImm extends GhidraScript {
    public void run() throws Exception {
        String[] args=getScriptArgs();StringBuilder text=new StringBuilder();
        Set<Long> wanted=new HashSet<>();for(int i=2;i<args.length;++i)wanted.add(Long.parseLong(args[i],16));
        Address target=toAddr(args[1]);int calls=0,hits=0;
        for(Reference r:getReferencesTo(target)){
            if(!r.getReferenceType().isCall()&&!r.getReferenceType().isJump())continue;calls++;
            Instruction call=getInstructionAt(r.getFromAddress());if(call==null)continue;
            List<Instruction> prev=new ArrayList<>();Instruction p=call.getPrevious();
            for(int k=0;k<10&&p!=null;++k){prev.add(0,p);p=p.getPrevious();}
            boolean hit=false;
            for(Instruction ins:prev)for(int op=0;op<ins.getNumOperands();++op)for(Object o:ins.getOpObjects(op))
                if(o instanceof Scalar&&wanted.contains(((Scalar)o).getUnsignedValue()))hit=true;
            if(hit){hits++;Function f=getFunctionContaining(call.getAddress());
                text.append("call "+call.getAddress()+" in "+(f==null?"none":f.getEntryPoint()+" "+f.getName())+"\n");
                for(Instruction ins:prev)text.append("    "+ins.getAddress()+"  "+ins+"\n");text.append("    "+call.getAddress()+"  "+call+"\n");}
        }
        text.append("calls "+calls+" hits "+hits+"\n");
        Files.writeString(Paths.get(args[0]),text.toString(),StandardCharsets.UTF_8);
    }
}
