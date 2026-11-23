// Analyse one bounded, hash-checked loaded CPU function in a transient copy of
// the pinned programme. Use only -readOnly -noanalysis; never save this DB.
// Arguments: acquired JSON, hexadecimal function VA; optionally a loaded
// DWORD RVA switch table VA and bounded case count. Export with I0002 afterwards.
// @category DS3
import ghidra.app.script.GhidraScript;
import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import java.nio.file.*;
import java.security.MessageDigest;
import java.util.HexFormat;
import com.google.gson.*;

public class I0066LoadedCode extends GhidraScript {
    public void run() throws Exception {
        String[] args=getScriptArgs();
        JsonObject source=JsonParser.parseString(Files.readString(Paths.get(args[0])).replace("\uFEFF","")).getAsJsonObject();
        String pin="EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C";
        if(!source.get("read_only").getAsBoolean() || !pin.equals(source.get("game_sha256").getAsString()))
            throw new IllegalArgumentException("Loaded source identity mismatch");
        Address start=toAddr(args[1]),next=start;
        int total=0;
        for(JsonElement item:source.getAsJsonArray("records")) {
            JsonObject record=item.getAsJsonObject();
            Address at=toAddr(0x140000000L+record.get("rva").getAsLong());
            byte[] bytes=HexFormat.of().parseHex(record.get("hex").getAsString());
            total+=bytes.length;
            if(!at.equals(next) || bytes.length!=record.get("bytes").getAsInt() || bytes.length>512 || total>0x4000 ||
               !HexFormat.of().formatHex(MessageDigest.getInstance("SHA-256").digest(bytes)).equalsIgnoreCase(record.get("sha256").getAsString()))
                throw new IllegalArgumentException("Loaded range extent/hash mismatch");
            next=at.add(bytes.length);
        }
        if(total==0)throw new IllegalArgumentException("Empty loaded function");
        Function old=getFunctionAt(start);
        if(old!=null)currentProgram.getFunctionManager().removeFunction(start);
        clearListing(start,next.previous());
        for(JsonElement item:source.getAsJsonArray("records")) {
            JsonObject record=item.getAsJsonObject();
            currentProgram.getMemory().setBytes(toAddr(0x140000000L+record.get("rva").getAsLong()),HexFormat.of().parseHex(record.get("hex").getAsString()));
        }
        new DisassembleCommand(start,null,true).applyTo(currentProgram,monitor);
        if(args.length==4){
            Address table=toAddr(args[2]);int count=Integer.parseInt(args[3]);
            if(count<1 || count>256 || table.compareTo(start)<0 || table.add(4L*count).compareTo(next)>0)
                throw new IllegalArgumentException("Loaded switch table extent");
            for(int i=0;i<count;++i){
                Address target=toAddr(0x140000000L+Integer.toUnsignedLong(getInt(table.add(4L*i))));
                if(target.compareTo(start)<0 || target.compareTo(table)>=0)
                    throw new IllegalArgumentException("Loaded switch target extent");
                new DisassembleCommand(target,null,true).applyTo(currentProgram,monitor);
            }
        }else if(args.length!=2)throw new IllegalArgumentException("Loaded source arguments");
        Function function=createFunction(start,null);
        if(function!=null && args.length==4){
            AddressSet body=new AddressSet();
            InstructionIterator instructions=currentProgram.getListing().getInstructions(new AddressSet(start,toAddr(args[2]).previous()),true);
            while(instructions.hasNext()){Instruction instruction=instructions.next();body.add(instruction.getAddress(),instruction.getMaxAddress());}
            function.setBody(body);
        }
        if(function==null || function.getBody().getMaxAddress().compareTo(next)>=0)
            throw new IllegalStateException("Loaded function body not bounded by acquisition");
        println("Transient loaded function "+start+" acquired bytes "+total+"; no DB save permitted");
    }
}
