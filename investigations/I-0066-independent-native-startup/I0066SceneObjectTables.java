// Targeted read-only RTTI search for scene object ABI. No analysis mutations.
// Args: output file. Search cached initialized .data/.rdata for six named
// rendering-object classes. No function discovery, analysis or mutations.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.mem.MemoryBlock;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
public class I0066SceneObjectTables extends GhidraScript {
    private byte[] ro,typesBytes;private Address roStart,typeStart;
    private Address find(Address start,Address end,byte[] pattern){
        boolean type=start.compareTo(typeStart)>=0 && start.compareTo(typeStart.add(typesBytes.length))<0;
        byte[] bytes=type?typesBytes:ro;Address base=type?typeStart:roStart;
        int begin=(int)start.subtract(base),last=(int)end.subtract(base)-pattern.length+1;
        for(int i=begin;i<=last;i++){if(bytes[i]!=pattern[0])continue;
            boolean equal=true;for(int j=1;j<pattern.length;j++)if(bytes[i+j]!=pattern[j]){equal=false;break;}
            if(equal)return base.add(i);
        }return null;
    }
    private byte[] little(long value,int n){byte[] b=new byte[n];for(int k=0;k<n;k++)b[k]=(byte)(value>>>(8*k));return b;}
    public void run() throws Exception {
        var mem=currentProgram.getMemory();MemoryBlock block=mem.getBlock(".rdata");
        if(block==null)throw new IllegalStateException("No .rdata");
        StringBuilder out=new StringBuilder();MemoryBlock types=mem.getBlock(".data");if(types==null)throw new IllegalStateException("No .data");Address cursor=types.getStart();int names=0;
        roStart=block.getStart();typeStart=types.getStart();ro=new byte[(int)block.getSize()];typesBytes=new byte[(int)types.getSize()];
        mem.getBytes(roStart,ro);mem.getBytes(typeStart,typesBytes);
        while(names<20000){
            Address name=find(cursor,types.getEnd(),".?AV".getBytes(StandardCharsets.US_ASCII));
            if(name==null)break;names++;cursor=name.add(1);StringBuilder type=new StringBuilder();
            for(int i=0;i<128;i++){byte b=mem.getByte(name.add(i));if(b==0)break;type.append((char)b);}
            if(!type.toString().matches(".*(SprjModelDrawEntity@|SprjAsmModelDrawEntity@|GXModelPainter@|GXFlverModelDC@|GXFlverModelDCBase@|GXModelDC@).*"))continue;
            Address td=name.subtract(16);long rva=td.subtract(currentProgram.getImageBase());
            Address colCursor=block.getStart();out.append("TYPE ").append(td).append(' ').append(type).append('\n');
            for(int c=0;c<32;c++){
                Address ref=find(colCursor,block.getEnd(),little(rva,4));if(ref==null)break;
                colCursor=ref.add(1);Address col=ref.subtract(12);
                if(mem.getInt(col)!=1 || mem.getInt(col.add(20))!=col.subtract(currentProgram.getImageBase()))continue;
                Address slotCursor=block.getStart();
                for(int v=0;v<16;v++){
                    Address ptr=find(slotCursor,block.getEnd(),little(col.getOffset(),8));if(ptr==null)break;
                    slotCursor=ptr.add(1);Address vt=ptr.add(8);out.append("VT ").append(vt).append(" offset=").append(mem.getInt(col.add(4)));
                    for(int k=0;k<5;k++)out.append(' ').append(Long.toHexString(mem.getLong(vt.add(k*8))));out.append('\n');
                }
            }
        }
        Files.writeString(Paths.get(getScriptArgs()[0]),out.toString(),StandardCharsets.UTF_8);
    }
}
