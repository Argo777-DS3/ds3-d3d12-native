// I-0100 LOD scale study: read-only. Every instruction with a memory operand [reg + D] for each hex displacement D given
// (no regular expression: the .bat launcher mangles brackets and bars), with its function; then, per D, the functions that
// read that displacement with a float instruction (MOVSS, MULSS, DIVSS, COMISS, ...) also listing which other given
// displacements they touch. Run only with -readOnly -noanalysis; nothing is saved. Arguments: output path, displacements.
// @category DS3
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.lang.Register;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

public class I0100DispUsers extends GhidraScript {
    public void run() throws Exception {
        String[] args=getScriptArgs();Set<Long> wanted=new LinkedHashSet<>();
        for(int i=1;i<args.length;++i)wanted.add(Long.parseLong(args[i].replace("0x",""),16));
        StringBuilder rows=new StringBuilder();Map<Long,Map<String,Set<Long>>> floatReaders=new TreeMap<>();Map<String,Set<Long>> touched=new HashMap<>();
        InstructionIterator it=currentProgram.getListing().getInstructions(true);
        while(it.hasNext() && !monitor.isCancelled()){
            Instruction ins=it.next();
            for(int op=0;op<ins.getNumOperands();++op){
                Object[] objs=ins.getOpObjects(op);boolean reg=false;Long disp=null;
                for(Object o:objs){if(o instanceof Register)reg=true;else if(o instanceof Scalar)disp=((Scalar)o).getUnsignedValue();}
                if(!reg || disp==null || !wanted.contains(disp))continue;
                String text=ins.toString();if(!text.contains("ptr ["))continue;
                Function f=getFunctionContaining(ins.getAddress());String fn=f==null?"-":f.getEntryPoint().toString();
                rows.append(ins.getAddress()+"\t"+fn+"\t0x"+Long.toHexString(disp)+"\t"+text+"\n");
                touched.computeIfAbsent(fn,k->new TreeSet<>()).add(disp);
                String m=ins.getMnemonicString().toUpperCase();
                if(op>0 && (m.endsWith("SS") || m.startsWith("COMIS") || m.startsWith("UCOMIS")))floatReaders.computeIfAbsent(disp,k->new TreeMap<>()).computeIfAbsent(fn,k->new TreeSet<>());
            }
        }
        StringBuilder head=new StringBuilder();
        for(Map.Entry<Long,Map<String,Set<Long>>> e:floatReaders.entrySet()){
            head.append("== float readers of 0x"+Long.toHexString(e.getKey())+": "+e.getValue().size()+"\n");
            for(String fn:e.getValue().keySet()){StringBuilder t=new StringBuilder();for(Long d:touched.get(fn))t.append(" 0x"+Long.toHexString(d));head.append("   "+fn+" touches"+t+"\n");}
        }
        Files.writeString(Paths.get(args[0]),head.toString()+"\n"+rows.toString(),StandardCharsets.UTF_8);
    }
}
