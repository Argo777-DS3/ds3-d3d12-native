// Read-only scan (headless Ghidra postScript): loads of a dword/byte at [reg+DISP] followed by a bit test (straight line, 12 instructions),
// plus direct tests/stores on [reg+DISP]. Arguments: output file, displacement(s) in hex separated by "_" (a comma or pipe breaks
// analyzeHeadless.bat), range start, range end (hex). Output lines: L load+uses, M direct memory test/modify, S store.
// Run once per displacement set with -readOnly -noanalysis; used by MTD-FLAG-BITS-INVENTORY-SHEET.
// @category DS3

import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;
import java.util.regex.*;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;

public class Bits24Scan extends GhidraScript {
    static String fam(String r) {
        r = r.toUpperCase();
        String[][] f = {
            {"RAX","EAX","AX","AL","AH"},{"RCX","ECX","CX","CL","CH"},{"RDX","EDX","DX","DL","DH"},{"RBX","EBX","BX","BL","BH"},
            {"RSP","ESP","SP","SPL"},{"RBP","EBP","BP","BPL"},{"RSI","ESI","SI","SIL"},{"RDI","EDI","DI","DIL"},
            {"R8","R8D","R8W","R8B"},{"R9","R9D","R9W","R9B"},{"R10","R10D","R10W","R10B"},{"R11","R11D","R11W","R11B"},
            {"R12","R12D","R12W","R12B"},{"R13","R13D","R13W","R13B"},{"R14","R14D","R14W","R14B"},{"R15","R15D","R15W","R15B"}};
        for (String[] g : f) for (String s : g) if (s.equals(r)) return g[0];
        return null;
    }

    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        Path outp = Paths.get(a[0]);
        String disp = "(?:" + a[1].toLowerCase().replace("_", "|") + ")";
        Address start = toAddr(a[2]);
        Address end = toAddr(a[3]);
        Pattern load = Pattern.compile("^(MOV|MOVZX|MOVSX|MOVSXD)\\s+(\\w+),(?:\\w+ ptr )?\\[(\\w+) \\+ 0x" + disp + "\\]$");
        Pattern mem = Pattern.compile("^(TEST|CMP|BT|AND|OR|XOR)\\s+(?:\\w+ ptr )?\\[(\\w+) \\+ 0x" + disp + "\\],(\\w+)$");
        Pattern memBts = Pattern.compile("^(BTS|BTR|BTC)\\s+(?:\\w+ ptr )?\\[(\\w+) \\+ 0x" + disp + "\\],(\\w+)$");
        Pattern store = Pattern.compile("^MOV\\s+(?:\\w+ ptr )?\\[(\\w+) \\+ 0x" + disp + "\\],(\\w+)$");
        Pattern use = Pattern.compile("^(TEST|AND|BT|SHR|SHL|SAR|CMP|OR|XOR|ANDN|BEXTR)\\s+(\\w+),(\\w+)$");
        StringBuilder sb = new StringBuilder();
        FunctionIterator fit = currentProgram.getFunctionManager().getFunctions(new AddressSet(start, end), true);
        List<Instruction> win = new ArrayList<>();
        // rolling buffer
        while (fit.hasNext()) { Function fn = fit.next(); List<Instruction> all = new ArrayList<>(); InstructionIterator it = currentProgram.getListing().getInstructions(fn.getBody(), true);
        while (it.hasNext()) all.add(it.next());
        for (int i = 0; i < all.size(); i++) {
            Instruction ins = all.get(i);
            String t = ins.toString();
            Matcher m = load.matcher(t);
            if (m.matches()) {
                String fr = fam(m.group(2));
                if (fr == null) continue;
                String res = "";
                for (int j = i + 1; j < Math.min(all.size(), i + 13); j++) {
                    Instruction n = all.get(j);
                    if (n.getFlowType().isCall() && !n.getFlowType().isConditional()) { res += " [call]"; break; }
                    Matcher u = use.matcher(n.toString());
                    if (u.matches()) {
                        String f1 = fam(u.group(2));
                        if (fr.equals(f1)) {
                            res += " | " + n.getAddress() + " " + n.toString();
                            if (!u.group(1).equals("CMP")) { /* keep scanning for a bit more */ }
                            if (res.length() > 400) break;
                        }
                    }
                    String ns = n.toString();
                    Matcher w = Pattern.compile("^(MOV|LEA|MOVZX|MOVSX|MOVSXD|XOR|POP)\\s+(\\w+),.*").matcher(ns);
                    if (w.matches() && fr.equals(fam(w.group(2)))) break;
                }
                Function f = getFunctionContaining(ins.getAddress());
                sb.append("L ").append(ins.getAddress()).append(' ').append(f == null ? "?" : f.getEntryPoint().toString()).append(' ').append(t).append(res).append('\n');
                continue;
            }
            m = mem.matcher(t);
            if (m.matches()) {
                Function f = getFunctionContaining(ins.getAddress());
                sb.append("M ").append(ins.getAddress()).append(' ').append(f == null ? "?" : f.getEntryPoint().toString()).append(' ').append(t).append('\n');
                continue;
            }
            m = memBts.matcher(t);
            if (m.matches()) {
                Function f = getFunctionContaining(ins.getAddress());
                sb.append("M ").append(ins.getAddress()).append(' ').append(f == null ? "?" : f.getEntryPoint().toString()).append(' ').append(t).append('\n');
                continue;
            }
            m = store.matcher(t);
            if (m.matches()) {
                Function f = getFunctionContaining(ins.getAddress());
                sb.append("S ").append(ins.getAddress()).append(' ').append(f == null ? "?" : f.getEntryPoint().toString()).append(' ').append(t).append('\n');
            }
        }
        }
        Files.write(outp, sb.toString().getBytes(StandardCharsets.UTF_8));
    }
}
