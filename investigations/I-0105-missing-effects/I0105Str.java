// I-0105: read-only. For each argument "<hex VA>:<byte count>" the bytes in hex, the bytes read as UTF-16LE strings (split at
// NUL, with their offsets), and every 8-byte slot that points into the image read as a UTF-16LE string and as an ASCII
// string. Run only with -readOnly -noanalysis; nothing is saved. Arguments: output path, then the ranges.
// @category DS3
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;

public class I0105Str extends GhidraScript {
    String wide(Address a,int max){StringBuilder s=new StringBuilder();try{for(int i=0;i<max;++i){int c=getShort(a.add(2L*i))&0xffff;if(c==0)break;s.append((char)c);}}catch(Exception e){return null;}return s.toString();}
    String ascii(Address a,int max){StringBuilder s=new StringBuilder();try{for(int i=0;i<max;++i){int c=getByte(a.add(i))&0xff;if(c==0)break;if(c<32||c>126)return null;s.append((char)c);}}catch(Exception e){return null;}return s.toString();}
    public void run() throws Exception {
        String[] args=getScriptArgs();StringBuilder text=new StringBuilder();
        for(int i=1;i<args.length;++i){
            String[] p=args[i].split(":");Address at=toAddr(p[0]);int n=Integer.parseInt(p[1]);
            text.append("=== "+at+" +"+n+" block="+(currentProgram.getMemory().getBlock(at)==null?"none":currentProgram.getMemory().getBlock(at).getName())+"\n");
            for(int o=0;o<n;o+=16){StringBuilder h=new StringBuilder();for(int k=0;k<16&&o+k<n;++k){try{h.append(String.format("%02x ",getByte(at.add(o+k))&0xff));}catch(Exception e){h.append("?? ");}}
                text.append(String.format("  +%03X %s\n",o,h));}
            text.append("  -- UTF-16 strings in range\n");
            for(int o=0;o<n;o+=2){String s=wide(at.add(o),64);if(s!=null&&s.length()>=2){text.append(String.format("  +%03X \"%s\"\n",o,s));o+=2*s.length();}}
            text.append("  -- 8-byte slots as pointers\n");
            for(int o=0;o+8<=n;o+=8){long v=getLong(at.add(o));if(v>0x140000000L&&v<0x150000000L){Address t=toAddr(v);String w=wide(t,64),a=ascii(t,64);
                text.append(String.format("  +%03X -> %s wide=%s ascii=%s\n",o,t,w==null?"-":"\""+w+"\"",a==null?"-":"\""+a+"\""));}}
        }
        Files.writeString(Paths.get(args[0]),text.toString(),StandardCharsets.UTF_8);
    }
}
