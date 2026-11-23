// read-only: for each function entry in [start,end], list MOV imm and CALL targets. args: out start end [start end ...]
// @category DS3
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.symbol.Reference;
import java.nio.file.*;
public class FuncSummary extends GhidraScript {
    public void run() throws Exception {
        String[] a=getScriptArgs();StringBuilder sb=new StringBuilder();
        for(int k=1;k+1<a.length;k+=2){
            Address s=toAddr(a[k]),e=toAddr(a[k+1]);
            FunctionIterator it=currentProgram.getFunctionManager().getFunctions(s,true);
            while(it.hasNext()){Function f=it.next();if(f.getEntryPoint().compareTo(e)>0)break;
                sb.append("F "+f.getEntryPoint()+" size="+f.getBody().getNumAddresses()+" :");
                InstructionIterator ii=currentProgram.getListing().getInstructions(f.getBody(),true);
                while(ii.hasNext()){Instruction in=ii.next();String m=in.getMnemonicString();
                    if(m.equals("CALL")){Address[] fl=in.getFlows();sb.append(" call:"+(fl.length>0?fl[0].toString():"ind"));}
                    else if(m.equals("MOV")||m.equals("LEA")||m.equals("PUSH")){
                        for(int o=0;o<in.getNumOperands();o++){Object[] ops=in.getOpObjects(o);
                            for(Object x:ops) if(x instanceof Scalar){long v=((Scalar)x).getValue();
                                if(v!=0xfffffffffffffffeL&&v!=0&&Math.abs(v)>=1&&!(o==0&&ops.length>1&&false)) sb.append(" imm:0x"+Long.toHexString(v));}
                            if(m.equals("LEA")){Address r=in.getAddress(o)==null?null:null;}
                        }
                        if(m.equals("LEA")||m.equals("MOV")){ for(Reference r: in.getReferencesFrom()) if(r.isMemoryReference()&&r.getToAddress().getOffset()>0x142000000L&&r.getToAddress().getOffset()<0x144700000L) sb.append(" ref:"+r.getToAddress());}
                    }
                }
                sb.append("\n");
            }
        }
        Files.writeString(Paths.get(a[0]),sb.toString());
    }
}
