// read-only: scan raw bytes of [start,end] for lea reg,[rip+rel] and mov [rip+d],reg / mov dword [rip+d],imm; print targets with UTF-16/ASCII text
// @category DS3
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import java.nio.file.*;
public class ScanInit extends GhidraScript {
    String str(Address t) throws Exception {
        StringBuilder u=new StringBuilder();
        try{ for(int i=0;i<80;i++){int c=(getByte(t.add(2*i))&0xff)|((getByte(t.add(2*i+1))&0xff)<<8); if(c==0)break; if(c<32||c>126){u=null;break;} u.append((char)c);} }catch(Exception e){u=null;}
        if(u!=null&&u.length()>=3) return "U16:"+u;
        StringBuilder a=new StringBuilder();
        try{ for(int i=0;i<60;i++){int c=getByte(t.add(i))&0xff; if(c==0)break; if(c<32||c>126){return "";} a.append((char)c);} }catch(Exception e){return "";}
        return a.length()>=3?"A:"+a:"";
    }
    public void run() throws Exception {
        String[] a=getScriptArgs();StringBuilder sb=new StringBuilder();
        long s=toAddr(a[1]).getOffset(),e=toAddr(a[2]).getOffset();
        for(long p=s;p<e;p++){
            Address ad=toAddr(p);
            int b0=getByte(ad)&0xff,b1=getByte(ad.add(1))&0xff,b2=getByte(ad.add(2))&0xff;
            if((b0==0x48||b0==0x4c)&&b1==0x8d&&(b2&0xc7)==0x05){int rel=getInt(ad.add(3));Address t=toAddr(p+7+rel);
                sb.append(Long.toHexString(p)+" lea r"+((b2>>3)&7)+(b0==0x4c?"+8":"")+" -> "+t+" "+str(t)+"\n");}
            else if((b0==0x48||b0==0x4c)&&b1==0x89&&(b2&0xc7)==0x05){int rel=getInt(ad.add(3));sb.append(Long.toHexString(p)+" store -> "+toAddr(p+7+rel)+"\n");}
            else if(b0==0xc7&&b1==0x05){int rel=getInt(ad.add(2));int imm=getInt(ad.add(6));sb.append(Long.toHexString(p)+" dword-store "+toAddr(p+10+rel)+" = "+imm+"\n");}
            else if(b0==0xc6&&b1==0x05){int rel=getInt(ad.add(2));int imm=getByte(ad.add(6));sb.append(Long.toHexString(p)+" byte-store "+toAddr(p+7+rel)+" = "+imm+"\n");}
        }
        Files.writeString(Paths.get(a[0]),sb.toString());
    }
}
