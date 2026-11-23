// I-0067: read-only static map queries on the canonical database (run with -readOnly -noanalysis; nothing saved).
// Arguments: output directory, then queries (absolute hex addresses or prefixed forms):
//   <addr>     decompile the containing function: <entry>.c, <entry>.asm.txt, and its callers/callees in index.txt
//   c:<addr>   also decompile every direct caller of the function at <addr>
//   s:<text>   list defined strings containing <text> (ASCII or UTF-16) and the functions referencing them
//   y:<text>   list symbols (namespace::name) containing <text>, with address (max 600 per query)
//   v:<addr>   list the 48 first slots of the vtable at <addr> (target, function name)
//   d:<addr>   decompile the containing function only if not done; same as <addr> (kept for readability)
//   V:<file>   load vslots.tsv (I0067Rtti) so function names carry their RTTI class#slot; put it first
//   k:<text>   RTTI classes whose name contains <text>, with vtables (slow; prefer I0067Rtti.java)
//   t:<addr>[:depth]  caller tree;  A:<addr> call sites with the 4 preceding instructions (argument setup)
//   w:<addr>:<len> Ghidra references into a range;  o:<addr>:<len> instructions whose operand falls in a range
//   M:<disp>:<global> stores (U: any use) with memory displacement <disp> in functions that read <global>
//   b:<addr> stored 8-byte pointers;  r:<addr> stored 4-byte image-relative offsets;  e:<addr> raw E8/E9 rel32 sites
//   x:<addr>:<len> hex dump;  l:<start>:<end> forced linear listing;  S:<text> raw ASCII/UTF-16 strings, refs, pointers
//   f:<addr> make undisassembled code decompilable;  C:<addr> create a function exactly at <addr>
//   L:<json> overlay Read-LoadedCode bytes (identity- and hash-checked) before decompiling them
// All program edits (disassembly, functions, overlays) are in memory only: the session is read-only.
// Reuses I-0064's I0064DecompileContaining pattern.
// @category DS3

import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;

public class I0067Map extends GhidraScript {
    private final StringBuilder index = new StringBuilder();
    private final Set<String> done = new HashSet<>();
    private DecompInterface ifc;
    private Path out;

    private final Map<String, String> vslot = new HashMap<>();   // function entry -> class#slot (from V:<vslots.tsv>)

    private String fname(Function f) {
        if (f == null) return "?";
        String v = vslot.get(f.getEntryPoint().toString());
        return v == null ? f.getName(true) : f.getName(true) + " {" + v + "}";
    }

    // Caller tree (callers of callers), ignoring exception-table (.pdata) references; virtual slots and other data
    // references are printed as leaves.
    private void tree(Address a, int depth) {
        Set<String> seen = new HashSet<>();
        Deque<Object[]> stack = new ArrayDeque<>();
        Function f0 = getFunctionContaining(a);
        if (f0 == null) { index.append("nofunction ").append(a).append('\n'); return; }
        stack.push(new Object[] { f0, 0 });
        int lines = 0;
        while (!stack.isEmpty() && lines < 3000) {
            Object[] e = stack.pop(); Function f = (Function) e[0]; int lv = (Integer) e[1];
            String pad = "  ".repeat(lv);
            boolean first = seen.add(f.getEntryPoint().toString());
            index.append(pad).append(f.getEntryPoint()).append(' ').append(fname(f)).append(first ? "" : " (seen)").append('\n'); lines++;
            if (!first || lv >= depth) continue;
            for (Reference ref : getReferencesTo(f.getEntryPoint())) {
                Address from = ref.getFromAddress();
                ghidra.program.model.mem.MemoryBlock b = currentProgram.getMemory().getBlock(from);
                String bn = b == null ? "?" : b.getName();
                if (bn.equals(".pdata")) continue;
                if (ref.getReferenceType().isCall() || ref.getReferenceType().isJump()) {
                    Function g = getFunctionContaining(from);
                    if (g != null) stack.push(new Object[] { g, lv + 1 });
                    else { index.append(pad).append("  <- ").append(from).append(" nofunction\n"); lines++; }
                } else { index.append(pad).append("  <- data ").append(from).append(' ').append(bn).append('\n'); lines++; }
            }
        }
    }

    private void decompile(Address a) throws Exception {
        Function f = getFunctionContaining(a);
        if (f == null) { index.append("nofunction ").append(a).append('\n'); return; }
        String entry = f.getEntryPoint().toString();
        index.append("contains ").append(a).append(" -> ").append(entry).append(' ').append(fname(f)).append('\n');
        if (!done.add(entry)) return;
        DecompileResults r = ifc.decompileFunction(f, 180, monitor);
        String c = r.decompileCompleted() ? r.getDecompiledFunction().getC() : ("/* failed: " + r.getErrorMessage() + " */");
        Files.write(out.resolve(entry + ".c"), c.getBytes(StandardCharsets.UTF_8));
        StringBuilder asm = new StringBuilder();
        InstructionIterator it = currentProgram.getListing().getInstructions(f.getBody(), true);
        Set<String> callees = new TreeSet<>();
        while (it.hasNext()) {
            Instruction ins = it.next();
            asm.append(ins.getAddress()).append("  ").append(ins.toString()).append('\n');
            if (ins.getFlowType().isCall()) for (Address t : ins.getFlows()) { Function g = getFunctionAt(t); callees.add(t + " " + fname(g)); }
        }
        Files.write(out.resolve(entry + ".asm.txt"), asm.toString().getBytes(StandardCharsets.UTF_8));
        index.append("  function ").append(entry).append(" size ").append(f.getBody().getNumAddresses()).append('\n');
        for (Reference ref : getReferencesTo(f.getEntryPoint())) {
            Function g = getFunctionContaining(ref.getFromAddress());
            index.append("  caller ").append(ref.getFromAddress()).append(' ').append(ref.getReferenceType()).append(' ').append(fname(g)).append('\n');
        }
        for (String s : callees) index.append("  callee ").append(s).append('\n');
    }

    // MSVC x64 RTTI: type descriptor name ".?AV<name>@@" at TD+0x10; a complete object locator has signature 1 and the
    // TD's image-relative offset at +0xC; the vtable starts 8 bytes after a stored absolute pointer to the locator.
    private void classes(String want) throws Exception {
        long base = currentProgram.getImageBase().getOffset();
        byte[] pat = want.getBytes(StandardCharsets.US_ASCII);
        Address at = currentProgram.getMinAddress();
        Set<String> seen = new HashSet<>();
        int n = 0;
        while ((at = find(at, pat)) != null && n < 400) {
            Address hit = at; at = at.add(1);
            Address s = null;
            for (int back = 0; back < 160; back++) {
                Address c = hit.subtract(back);
                byte b = getByte(c);
                if (b == '.' && getByte(c.add(1)) == '?' && getByte(c.add(2)) == 'A') { s = c; break; }
                if (b < 0x20 || b > 0x7e) break;
            }
            if (s == null || !seen.add(s.toString())) continue;
            StringBuilder name = new StringBuilder();
            for (Address c = s; name.length() < 300; c = c.add(1)) { byte b = getByte(c); if (b == 0) break; name.append((char) b); }
            Address td = s.subtract(0x10);
            int tdRva = (int) (td.getOffset() - base);
            byte[] rp = new byte[] { (byte) tdRva, (byte) (tdRva >>> 8), (byte) (tdRva >>> 16), (byte) (tdRva >>> 24) };
            StringBuilder line = new StringBuilder("class ").append(name).append(" td ").append(td);
            Address p = currentProgram.getMinAddress();
            while ((p = find(p, rp)) != null) {
                Address col = p.subtract(0xC); p = p.add(1);
                if (getInt(col) != 1) continue;
                long colAbs = col.getOffset();
                byte[] cp = new byte[8]; for (int k = 0; k < 8; k++) cp[k] = (byte) (colAbs >>> (8 * k));
                Address v = currentProgram.getMinAddress();
                while ((v = find(v, cp)) != null) {
                    Address vt = v.add(8); v = v.add(1);
                    line.append(" vtable ").append(vt).append(" off ").append(Integer.toHexString(getInt(col.add(4))))
                        .append(" slot0 ").append(Long.toHexString(getLong(vt)));
                }
            }
            index.append(line).append('\n');
            n++;
        }
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        out = Paths.get(args[0]);
        Files.createDirectories(out);
        ifc = new DecompInterface();
        ifc.toggleCCode(true);
        if (!ifc.openProgram(currentProgram)) throw new IllegalStateException("Decompiler open failed");
        try {
            for (int i = 1; i < args.length; i++) {
                String q = args[i];
                index.append("# ").append(q).append('\n');
                if (q.startsWith("s:")) {
                    String want = q.substring(2);
                    for (Data d : currentProgram.getListing().getDefinedData(true)) {
                        Object v = d.getValue();
                        if (!(v instanceof String) || !((String) v).contains(want)) continue;
                        index.append("string ").append(d.getAddress()).append(" \"").append(((String) v).replace("\n", "\\n")).append("\"\n");
                        for (Reference ref : getReferencesTo(d.getAddress())) {
                            Function g = getFunctionContaining(ref.getFromAddress());
                            index.append("  ref ").append(ref.getFromAddress()).append(' ').append(g == null ? "?" : g.getEntryPoint() + " " + fname(g)).append('\n');
                        }
                    }
                } else if (q.startsWith("y:")) {
                    String want = q.substring(2);
                    int n = 0;
                    SymbolIterator si = currentProgram.getSymbolTable().getAllSymbols(true);
                    while (si.hasNext() && n < 600) {
                        Symbol s = si.next();
                        String full = s.getName(true);
                        if (!full.contains(want)) continue;
                        index.append("symbol ").append(s.getAddress()).append(' ').append(s.getSymbolType()).append(' ').append(full).append('\n');
                        n++;
                    }
                } else if (q.startsWith("v:")) {
                    Address v = toAddr(q.substring(2));
                    for (int k = 0; k < 48; k++) {
                        long t = getLong(v.add(8L * k));
                        Address ta;
                        try { ta = toAddr(t); } catch (Exception e) { break; }
                        Function g = getFunctionAt(ta);
                        index.append("slot +").append(Integer.toHexString(8 * k)).append(' ').append(Long.toHexString(t)).append(' ').append(g == null ? "-" : fname(g)).append('\n');
                    }
                } else if (q.startsWith("c:")) {
                    Address t = toAddr(q.substring(2));
                    decompile(t);
                    Function tf = getFunctionContaining(t);
                    if (tf != null) for (Reference ref : getReferencesTo(tf.getEntryPoint()))
                        if (ref.getReferenceType().isCall() || ref.getReferenceType().isJump()) decompile(ref.getFromAddress());
                } else if (q.startsWith("V:")) {
                    for (String l : Files.readAllLines(Paths.get(q.substring(2)), StandardCharsets.UTF_8)) {
                        String[] f = l.split("\t"); if (f.length < 4 || f[0].equals("function")) continue;
                        String c = f[3].replaceFirst("^\\.\\?A[VU]", "").replaceFirst("@@$", "");
                        vslot.merge(f[0], c + "#" + f[2], (x, y) -> x + " | " + y);
                    }
                } else if (q.startsWith("w:")) {   // w:<addr>:<len hex> every reference into [addr, addr+len)
                    String[] f = q.substring(2).split(":");
                    Address a0 = toAddr(f[0]); long len = f.length > 1 ? Long.parseLong(f[1], 16) : 8;
                    for (long k = 0; k < len; k++) for (Reference ref : getReferencesTo(a0.add(k))) {
                        Function g = getFunctionContaining(ref.getFromAddress());
                        index.append("  ref +").append(Long.toHexString(k)).append(' ').append(ref.getFromAddress()).append(' ').append(ref.getReferenceType())
                            .append(' ').append(g == null ? "?" : g.getEntryPoint() + " " + fname(g)).append('\n');
                    }
                } else if (q.startsWith("o:")) {   // o:<addr>:<len hex> instructions whose operand (scalar or resolved address) is in range
                    String[] f = q.substring(2).split(":");
                    long lo = Long.parseUnsignedLong(f[0], 16), hi = lo + (f.length > 1 ? Long.parseLong(f[1], 16) : 8);
                    InstructionIterator it = currentProgram.getListing().getInstructions(true);
                    int n = 0;
                    while (it.hasNext() && n < 2000) {
                        Instruction ins = it.next();
                        boolean hit = false;
                        for (int op = 0; op < ins.getNumOperands() && !hit; op++) {
                            for (Object o : ins.getOpObjects(op)) {
                                long v = o instanceof ghidra.program.model.scalar.Scalar ? ((ghidra.program.model.scalar.Scalar) o).getUnsignedValue()
                                    : o instanceof Address ? ((Address) o).getOffset() : -1;
                                if (v >= lo && v < hi) { hit = true; break; }
                            }
                            if (!hit) for (Reference r : ins.getOperandReferences(op)) { long v = r.getToAddress().getOffset(); if (v >= lo && v < hi) { hit = true; break; } }
                        }
                        if (!hit) continue;
                        Function g = getFunctionContaining(ins.getAddress());
                        index.append("  use ").append(ins.getAddress()).append("  ").append(ins).append("  ").append(g == null ? "?" : g.getEntryPoint() + " " + fname(g)).append('\n');
                        n++;
                    }
                } else if (q.startsWith("b:") || q.startsWith("r:")) {   // b:<addr> stored 8-byte value; r:<addr> stored 4-byte image-relative offset
                    boolean rel = q.startsWith("r:");
                    long val = Long.parseUnsignedLong(q.substring(2), 16);
                    if (rel) val -= currentProgram.getImageBase().getOffset();
                    byte[] pat = new byte[rel ? 4 : 8]; for (int k = 0; k < pat.length; k++) pat[k] = (byte) (val >>> (8 * k));
                    Address at = currentProgram.getMinAddress();
                    while ((at = find(at, pat)) != null) {
                        ghidra.program.model.mem.MemoryBlock b = currentProgram.getMemory().getBlock(at);
                        index.append("  stored ").append(at).append(' ').append(b == null ? "?" : b.getName()).append('\n'); at = at.add(1);
                    }
                } else if (q.startsWith("x:")) {   // x:<addr>:<len hex> hex dump, 16 bytes a line
                    String[] f = q.substring(2).split(":");
                    Address a0 = toAddr(f[0]); int len = Integer.parseInt(f.length > 1 ? f[1] : "40", 16);
                    for (int k = 0; k < len; k += 16) {
                        index.append("  ").append(a0.add(k)).append(' ');
                        for (int j = 0; j < 16 && k + j < len; j++) index.append(String.format("%02x ", getByte(a0.add(k + j)) & 0xff));
                        index.append('\n');
                    }
                } else if (q.startsWith("e:")) {   // e:<addr> raw E8/E9 rel32 call/jump bytes targeting <addr> in executable blocks (also undisassembled code)
                    long target = Long.parseUnsignedLong(q.substring(2), 16);
                    for (ghidra.program.model.mem.MemoryBlock b : currentProgram.getMemory().getBlocks()) {
                        if (!b.isExecute() || !b.isInitialized()) continue;
                        byte[] a = new byte[(int) b.getSize()]; b.getBytes(b.getStart(), a);
                        long s0 = b.getStart().getOffset();
                        for (int k = 0; k + 5 <= a.length; k++) {
                            if (a[k] != (byte) 0xE8 && a[k] != (byte) 0xE9) continue;
                            int rel = (a[k + 1] & 0xff) | (a[k + 2] & 0xff) << 8 | (a[k + 3] & 0xff) << 16 | (a[k + 4] & 0xff) << 24;
                            if (s0 + k + 5 + rel != target) continue;
                            Address at = toAddr(s0 + k);
                            Instruction ins = getInstructionAt(at);
                            Function g = getFunctionContaining(at);
                            index.append("  raw ").append(at).append(a[k] == (byte) 0xE8 ? " call" : " jmp").append(ins == null ? " (not disassembled)" : "")
                                .append(' ').append(g == null ? "?" : g.getEntryPoint() + " " + fname(g)).append('\n');
                        }
                    }
                } else if (q.startsWith("f:")) {   // f:<addr> code the database never disassembled: find the entry after the
                    // preceding int3/nop padding or function end, disassemble and create the function in memory only (read-only session), then decompile
                    Address a = toAddr(q.substring(2));
                    if (getFunctionContaining(a) == null) {
                        Address e = a;
                        for (int k = 0; k < 0x8000; k++) {
                            Address p = e.subtract(1);
                            int pb = getByte(p) & 0xff;
                            if (pb == 0xCC || getFunctionContaining(p) != null) break;
                            e = p;
                        }
                        new ghidra.app.cmd.disassemble.DisassembleCommand(e, null, true).applyTo(currentProgram, monitor);
                        new ghidra.app.cmd.disassemble.DisassembleCommand(a, null, true).applyTo(currentProgram, monitor);
                        // first try: the code is an omitted part of the preceding function's body
                        Function fb = getFunctionBefore(a);
                        if (fb != null) index.append("before ").append(fb.getEntryPoint()).append(" body ").append(fb.getBody().getMinAddress())
                            .append("-").append(fb.getBody().getMaxAddress()).append('\n');
                        if (fb != null && a.subtract(fb.getEntryPoint()) < 0x4000) {
                            ghidra.app.cmd.function.CreateFunctionCmd.fixupFunctionBody(currentProgram, fb, monitor);
                            if (fb.getBody().contains(a)) index.append("extended ").append(fb.getEntryPoint()).append(" to ").append(a).append('\n');
                        }
                        if (getFunctionContaining(a) == null) {
                            new ghidra.app.cmd.function.CreateFunctionCmd(e).applyTo(currentProgram, monitor);
                            index.append("created ").append(e).append(" for ").append(a).append('\n');
                        }
                    }
                    decompile(a);
                } else if (q.startsWith("g:")) {   // g:<start>:<end>:<regex> existing instructions in range whose text matches (no disassembly)
                    String[] f = q.substring(2).split(":", 3);
                    java.util.regex.Pattern re = java.util.regex.Pattern.compile(f[2]);
                    InstructionIterator it = currentProgram.getListing().getInstructions(new ghidra.program.model.address.AddressSet(toAddr(f[0]), toAddr(f[1])), true);
                    int n = 0;
                    while (it.hasNext() && n < 3000) {
                        Instruction ins = it.next(); String text = ins.toString();
                        if (!re.matcher(text).find()) continue;
                        Function g = getFunctionContaining(ins.getAddress());
                        index.append("  match ").append(ins.getAddress()).append("  ").append(text).append("  ").append(g == null ? "?" : g.getEntryPoint() + " " + fname(g)).append('\n'); n++;
                    }
                } else if (q.startsWith("l:")) {   // l:<start>:<end> forced linear listing (I0066BoundedCode pattern), memory only
                    String[] f = q.substring(2).split(":");
                    Address start = toAddr(f[0]), end = toAddr(f[1]);
                    if (end.subtract(start) <= 0 || end.subtract(start) > 0x3000) throw new IllegalArgumentException("bounded range required");
                    for (Address at = start; at.compareTo(end) < 0;) {
                        Instruction ins = getInstructionAt(at);
                        if (ins == null) { new ghidra.app.cmd.disassemble.DisassembleCommand(at, null, false).applyTo(currentProgram, monitor); ins = getInstructionAt(at); }
                        if (ins == null) { index.append("  ").append(at).append(" ??\n"); at = at.add(1); continue; }
                        index.append("  ").append(at).append("  ").append(ins);
                        if (ins.getFlowType().isCall() || ins.getFlowType().isJump()) for (Address t : ins.getFlows()) { Function g = getFunctionAt(t); if (g != null) index.append("   ; ").append(fname(g)); }
                        index.append('\n');
                        at = ins.getMaxAddress().next();
                    }
                } else if (q.startsWith("L:")) {   // L:<Read-LoadedCode json> overlay hash-checked loaded (decrypted) bytes, memory only
                    com.google.gson.JsonObject src = com.google.gson.JsonParser.parseString(Files.readString(Paths.get(q.substring(2))).replace("﻿", "")).getAsJsonObject();
                    if (!src.get("read_only").getAsBoolean() || !"EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C".equals(src.get("game_sha256").getAsString()))
                        throw new IllegalArgumentException("loaded source identity mismatch");
                    for (com.google.gson.JsonElement it : src.getAsJsonArray("records")) {
                        com.google.gson.JsonObject r = it.getAsJsonObject();
                        byte[] bytes = java.util.HexFormat.of().parseHex(r.get("hex").getAsString());
                        String h = java.util.HexFormat.of().formatHex(java.security.MessageDigest.getInstance("SHA-256").digest(bytes));
                        if (!h.equalsIgnoreCase(r.get("sha256").getAsString())) throw new IllegalArgumentException("loaded hash mismatch");
                        Address at = toAddr(0x140000000L + r.get("rva").getAsLong());
                        Function old = getFunctionContaining(at);
                        if (old != null) currentProgram.getFunctionManager().removeFunction(old.getEntryPoint());
                        clearListing(at, at.add(bytes.length - 1));
                        currentProgram.getMemory().setBytes(at, bytes);
                        index.append("overlay ").append(at).append(' ').append(bytes.length).append(" bytes sha ").append(h.substring(0, 16)).append('\n');
                    }
                } else if (q.startsWith("C:")) {   // C:<addr> disassemble and create a function at exactly <addr> (memory only), then decompile
                    Address a = toAddr(q.substring(2));
                    new ghidra.app.cmd.disassemble.DisassembleCommand(a, null, true).applyTo(currentProgram, monitor);
                    if (getFunctionAt(a) == null) createFunction(a, null);
                    decompile(a);
                } else if (q.startsWith("M:") || q.startsWith("U:")) {   // M:<disp>:<global> stores (U: any use) whose memory operand uses displacement <disp>, in functions that read <global>
                    String[] f = q.substring(2).split(":");
                    long disp = Long.parseLong(f[0], 16), glob = Long.parseUnsignedLong(f[1], 16);
                    Map<String, Boolean> uses = new HashMap<>();
                    InstructionIterator it = currentProgram.getListing().getInstructions(true);
                    int n = 0;
                    while (it.hasNext() && n < 1500) {
                        Instruction ins = it.next();
                        String m = ins.getMnemonicString();
                        boolean anyUse = q.startsWith("U:");
                        if (!anyUse && !(m.startsWith("MOV") || m.equals("INC") || m.equals("DEC") || m.equals("OR") || m.equals("AND") || m.equals("ADD") || m.equals("SUB"))) continue;
                        if (ins.getNumOperands() < 1) continue;
                        String needle = "0x" + Long.toHexString(disp) + "]";
                        boolean hitOp = false;
                        for (int op = 0; op < (anyUse ? ins.getNumOperands() : 1) && !hitOp; op++) {
                            String rep = ins.getDefaultOperandRepresentation(op);
                            hitOp = rep.contains("[") && rep.toLowerCase().contains(needle);
                        }
                        if (!hitOp) continue;
                        Function g = getFunctionContaining(ins.getAddress());
                        if (g == null) continue;
                        Boolean u = glob == 0 ? Boolean.TRUE : uses.get(g.getEntryPoint().toString());
                        if (u == null) {
                            u = false;
                            InstructionIterator gi = currentProgram.getListing().getInstructions(g.getBody(), true);
                            while (gi.hasNext() && !u) { Instruction x = gi.next();
                                for (int op = 0; op < x.getNumOperands() && !u; op++) for (Object o : x.getOpObjects(op))
                                    if ((o instanceof Address && ((Address) o).getOffset() == glob) || (o instanceof ghidra.program.model.scalar.Scalar && ((ghidra.program.model.scalar.Scalar) o).getUnsignedValue() == glob)) { u = true; break; } }
                            uses.put(g.getEntryPoint().toString(), u);
                        }
                        if (!u) continue;
                        index.append("  store ").append(ins.getAddress()).append("  ").append(ins).append("  ").append(g.getEntryPoint()).append(' ').append(fname(g)).append('\n');
                        n++;
                    }
                } else if (q.startsWith("A:")) {   // A:<addr> every call site of <addr> with the 4 preceding instructions (argument setup)
                    Address t = toAddr(q.substring(2));
                    for (Reference ref : getReferencesTo(t)) {
                        if (!ref.getReferenceType().isCall() && !ref.getReferenceType().isJump()) continue;
                        Instruction ins = getInstructionAt(ref.getFromAddress());
                        Function g = getFunctionContaining(ref.getFromAddress());
                        Deque<String> prev = new ArrayDeque<>();
                        Instruction p = ins;
                        for (int k = 0; k < 4 && p != null; k++) { p = p.getPrevious(); if (p != null) prev.push(p.toString()); }
                        index.append("  site ").append(ref.getFromAddress()).append(' ').append(g == null ? "?" : g.getEntryPoint() + " " + fname(g)).append("  | ").append(String.join(" ; ", prev)).append('\n');
                    }
                } else if (q.startsWith("S:")) {   // S:<text> raw ASCII and UTF-16LE occurrences, their Ghidra references and stored pointers
                    String want = q.substring(2);
                    byte[] a8 = want.getBytes(StandardCharsets.US_ASCII), a16 = want.getBytes(StandardCharsets.UTF_16LE);
                    List<ghidra.program.model.mem.MemoryBlock> bl = new ArrayList<>(); List<byte[]> bd = new ArrayList<>();
                    for (ghidra.program.model.mem.MemoryBlock b : currentProgram.getMemory().getBlocks()) {
                        if (!b.isInitialized() || b.getSize() > Integer.MAX_VALUE) continue;
                        byte[] a = new byte[(int) b.getSize()]; b.getBytes(b.getStart(), a); bl.add(b); bd.add(a);
                    }
                    List<Long> hits = new ArrayList<>();
                    for (int bi = 0; bi < bl.size(); bi++) {
                        if (bl.get(bi).isExecute()) continue;
                        byte[] a = bd.get(bi); long s0 = bl.get(bi).getStart().getOffset();
                        for (byte[] p : new byte[][] { a8, a16 }) {
                            outer: for (int k = 0; k + p.length <= a.length && hits.size() < 200; k++) {
                                for (int j = 0; j < p.length; j++) if (a[k + j] != p[j]) continue outer;
                                hits.add(s0 + k);
                            }
                        }
                    }
                    for (long h : hits) {
                        Address ha = toAddr(h);
                        // widen to the enclosing string start (ASCII or UTF-16)
                        boolean wide = getByte(ha.add(1)) == 0;
                        Address st = ha;
                        for (int k = 0; k < 256; k++) { Address p = st.subtract(wide ? 2 : 1); byte c = getByte(p); if (c < 0x20 || c > 0x7e || (wide && getByte(p.add(1)) != 0)) break; st = p; }
                        StringBuilder txt = new StringBuilder();
                        for (Address c = st; txt.length() < 120; c = c.add(wide ? 2 : 1)) { byte b = getByte(c); if (b == 0) break; txt.append((char) b); }
                        index.append("string ").append(st).append(wide ? " u\"" : " \"").append(txt).append("\"\n");
                        for (Reference ref : getReferencesTo(st)) {
                            Function g = getFunctionContaining(ref.getFromAddress());
                            index.append("  ref ").append(ref.getFromAddress()).append(' ').append(g == null ? "?" : g.getEntryPoint() + " " + fname(g)).append('\n');
                        }
                        long v = st.getOffset();
                        for (int bi = 0; bi < bl.size(); bi++) {
                            byte[] a = bd.get(bi); long s0 = bl.get(bi).getStart().getOffset();
                            for (int k = 0; k + 8 <= a.length; k += 8) {
                                long x = 0; for (int j = 7; j >= 0; j--) x = (x << 8) | (a[k + j] & 0xff);
                                if (x == v) index.append("  stored ").append(toAddr(s0 + k)).append(' ').append(bl.get(bi).getName()).append('\n');
                            }
                        }
                    }
                } else if (q.startsWith("P:")) {   // P:<imm> calls whose 4 preceding instructions load <imm> into ECX/EDX/R8D/R9D (argument constant)
                    String imm = "0x" + q.substring(2).toLowerCase();
                    InstructionIterator it = currentProgram.getListing().getInstructions(true);
                    int n = 0;
                    while (it.hasNext() && n < 3000) {
                        Instruction ins = it.next();
                        if (!ins.getFlowType().isCall()) continue;
                        Instruction p = ins; String arg = null;
                        for (int k = 0; k < 4 && p != null && arg == null; k++) {
                            p = p.getPrevious(); if (p == null) break;
                            String s = p.toString();
                            if (p.getFlowType().isCall()) break;
                            if (s.matches("MOV (ECX|EDX|R8D|R9D),(?i)" + imm)) arg = s;
                        }
                        if (arg == null) continue;
                        Address[] fl = ins.getFlows();
                        Function g = getFunctionContaining(ins.getAddress());
                        Function t = fl.length > 0 ? getFunctionAt(fl[0]) : null;
                        index.append("  call ").append(ins.getAddress()).append(" -> ").append(fl.length > 0 ? fl[0].toString() : "indirect").append(' ').append(t == null ? "" : fname(t))
                            .append(" | ").append(arg).append(" | in ").append(g == null ? "?" : g.getEntryPoint() + " " + fname(g)).append('\n');
                        n++;
                    }
                } else if (q.startsWith("t:")) {
                    String[] f = q.substring(2).split(":");
                    tree(toAddr(f[0]), f.length > 1 ? Integer.parseInt(f[1]) : 6);
                } else if (q.startsWith("k:")) {
                    classes(q.substring(2));
                } else if (q.startsWith("d:")) {
                    decompile(toAddr(q.substring(2)));
                } else {
                    decompile(toAddr(q));
                }
            }
        } finally {
            ifc.dispose();
            Files.write(out.resolve("index.txt"), index.toString().getBytes(StandardCharsets.UTF_8));
        }
    }
}
