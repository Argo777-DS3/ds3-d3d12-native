import java.nio.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;
import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.app.cmd.function.CreateFunctionCmd;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
// I-0066 cycle936: in-memory only (-readOnly, never saved). Lays the decrypted code
// overlay (code_overlay.py overlay.bin: u32 rva, u32 len, bytes) over the static image,
// then for each target entry (arguments 3..n) clears the code units of the overlay runs
// near it, re-disassembles from the entry, creates the function and exports C and ASM.
// Arguments: out overlay.bin entry...
public class I0066Overlay936 extends GhidraScript {
 public void run()throws Exception{
  String[] args=getScriptArgs();Path out=Paths.get(args[0]);Files.createDirectories(out);List<String> report=new ArrayList<>();
  ByteBuffer bb=ByteBuffer.wrap(Files.readAllBytes(Paths.get(args[1]))).order(ByteOrder.LITTLE_ENDIAN);
  long base=currentProgram.getImageBase().getOffset();List<long[]> runs=new ArrayList<>();int applied=0;
  while(bb.remaining()>=8){long rva=bb.getInt()&0xffffffffL;int n=bb.getInt();byte[] b=new byte[n];bb.get(b);
   try{Address a=toAddr(base+rva);clearListing(a,a.add(n-1));currentProgram.getMemory().setBytes(a,b);runs.add(new long[]{base+rva,base+rva+n});applied++;}catch(Exception e){report.add("skip "+Long.toHexString(rva)+" "+e.getMessage());}}
  report.add("overlay runs applied "+applied);
  DecompInterface dc=new DecompInterface();dc.toggleCCode(true);dc.openProgram(currentProgram);
  try{for(int k=2;k<args.length;k++){long t=Long.parseUnsignedLong(args[k],16);Address e=toAddr(t);
   for(long[] r:runs)if(r[1]>t-0x200&&r[0]<t+0x2000){Address lo=toAddr(Math.min(r[0],t)),hi=toAddr(r[1]-1);
    FunctionIterator fi=currentProgram.getFunctionManager().getFunctions(new AddressSet(lo,hi),true);List<Function> del=new ArrayList<>();while(fi.hasNext())del.add(fi.next());for(Function f:del)removeFunction(f);
    clearListing(lo,hi);}
   Function old=getFunctionContaining(e);if(old!=null&&!old.getEntryPoint().equals(e)){removeFunction(old);}
   clearListing(e,e.add(0x10));new DisassembleCommand(e,null,true).applyTo(currentProgram,monitor);
   if(getFunctionAt(e)==null)new CreateFunctionCmd(e).applyTo(currentProgram,monitor);Function f=getFunctionAt(e);
   if(f==null){report.add(args[k]+" no function");continue;}
   DecompileResults r=dc.decompileFunction(f,180,monitor);
   if(r.decompileCompleted()&&r.getDecompiledFunction()!=null)Files.write(out.resolve(args[k]+".c"),r.getDecompiledFunction().getC().getBytes(StandardCharsets.UTF_8));
   StringBuilder asm=new StringBuilder();InstructionIterator it=currentProgram.getListing().getInstructions(f.getBody(),true);
   while(it.hasNext()){Instruction i=it.next();asm.append(i.getAddress()).append("  ").append(i).append('\n');}
   Files.write(out.resolve(args[k]+".asm.txt"),asm.toString().getBytes(StandardCharsets.UTF_8));report.add(args[k]+" body="+f.getBody());}}finally{dc.dispose();}
  report.add("readonly=true; in-memory overlay only");Files.write(out.resolve("overlay-export.txt"),report,StandardCharsets.UTF_8);println("936 complete");
 }
}
