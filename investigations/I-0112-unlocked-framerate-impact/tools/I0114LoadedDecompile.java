// Overlay one hash-checked loaded function (I0066LoadedCode's checks) in a read-only, no-analysis session, then
// write its decompilation and listing. Arguments: output prefix, acquired JSON, hexadecimal function VA.
// @category DS3
import ghidra.app.script.GhidraScript;
import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import java.nio.file.*;
import java.security.MessageDigest;
import java.util.HexFormat;
import com.google.gson.*;

public class I0114LoadedDecompile extends GhidraScript {
    public void run() throws Exception {
        String[] args=getScriptArgs();
        String out=args[0];
        JsonObject source=JsonParser.parseString(Files.readString(Paths.get(args[1])).replace("﻿","")).getAsJsonObject();
        String pin="EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C";
        if(!source.get("read_only").getAsBoolean() || !pin.equals(source.get("game_sha256").getAsString()))
            throw new IllegalArgumentException("Loaded source identity mismatch");
        Address start=toAddr(args[2]),next=start;
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
        Function old=getFunctionAt(start);
        if(old!=null)currentProgram.getFunctionManager().removeFunction(start);
        clearListing(start,next.previous());
        for(JsonElement item:source.getAsJsonArray("records")) {
            JsonObject record=item.getAsJsonObject();
            currentProgram.getMemory().setBytes(toAddr(0x140000000L+record.get("rva").getAsLong()),HexFormat.of().parseHex(record.get("hex").getAsString()));
        }
        new DisassembleCommand(start,null,true).applyTo(currentProgram,monitor);
        Function function=createFunction(start,null);
        if(function==null)throw new IllegalStateException("No function");
        boolean bounded=function.getBody().getMaxAddress().compareTo(next)<0;
        StringBuilder asm=new StringBuilder();
        asm.append("; body ").append(function.getBody().getMinAddress()).append("..").append(function.getBody().getMaxAddress())
           .append(" acquired to ").append(next).append(bounded?" (bounded)":" (BODY EXCEEDS ACQUISITION)").append("\n");
        InstructionIterator it=currentProgram.getListing().getInstructions(function.getBody(),true);
        while(it.hasNext()){Instruction i=it.next();asm.append(i.getAddress()).append("  ").append(i.toString()).append("\n");}
        Files.writeString(Paths.get(out+".asm.txt"),asm.toString());
        DecompInterface d=new DecompInterface();d.openProgram(currentProgram);
        DecompileResults r=d.decompileFunction(function,180,monitor);
        String code=r.decompileCompleted()?r.getDecompiledFunction().getC():("/* decompile failed: "+r.getErrorMessage()+" */");
        Files.writeString(Paths.get(out+".c"),code);
        println("loaded "+total+" bytes; body max "+function.getBody().getMaxAddress()+"; bounded "+bounded+"; no DB save");
    }
}
