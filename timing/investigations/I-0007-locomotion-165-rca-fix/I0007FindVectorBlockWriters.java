// Find exact-build code that writes all three 16-byte block offsets +30/+40/+50
// through one register inside one function. Results are structural leads only.
// @category DS3
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;
import java.util.regex.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;

public class I0007FindVectorBlockWriters extends GhidraScript {
    private static final Pattern DEST = Pattern.compile(
        "^(?:MOV|VMOV)[A-Z0-9]*\\s+xmmword ptr \\[([A-Z0-9]+) \\+ 0x(30|40|50)\\],.*$");

    public void run() throws Exception {
        String[] args=getScriptArgs();
        if(args.length!=3) throw new IllegalArgumentException("output, inclusive start, inclusive end required");
        Address start=toAddr(args[1]),end=toAddr(args[2]);
        if(end.compareTo(start)<0 || end.subtract(start)>0x100000)
            throw new IllegalArgumentException("search range must be ordered and at most1MiB");
        Map<String,Set<String>> offsets=new LinkedHashMap<>();
        Map<String,List<String>> rows=new LinkedHashMap<>();
        InstructionIterator it=currentProgram.getListing().getInstructions(new AddressSet(start,end),true);
        while(it.hasNext()) {
            monitor.checkCancelled();
            Instruction ins=it.next();
            Matcher m=DEST.matcher(ins.toString());
            if(!m.matches()) continue;
            Function f=currentProgram.getFunctionManager().getFunctionContaining(ins.getAddress());
            if(f==null) continue;
            String key=f.getEntryPoint()+","+m.group(1);
            offsets.computeIfAbsent(key,k->new LinkedHashSet<>()).add(m.group(2));
            rows.computeIfAbsent(key,k->new ArrayList<>()).add(ins.getAddress()+" "+ins);
        }
        StringBuilder out=new StringBuilder("function_entry,base_register,instructions\n");
        for(String key:offsets.keySet()) {
            if(offsets.get(key).size()!=3) continue;
            int comma=key.indexOf(',');
            String detail=String.join(" | ",rows.get(key)).replace("\"","\"\"");
            out.append(key,0,comma).append(',').append(key.substring(comma+1))
               .append(",\"").append(detail).append("\"\n");
        }
        Path path=Paths.get(args[0]); Files.createDirectories(path.getParent());
        Files.writeString(path,out.toString(),StandardCharsets.UTF_8);
        println("Qualified structural writer leads="+(out.toString().split("\\n").length-1));
    }
}
