# I-0099 (2026-10-25): the code that calls or jumps to each target (E8/E9 rel32) or takes its address (lea rip-relative),
# over the executable's code sections. Read-only, no debugger, no Ghidra. Pass targets as an array: -Targets @(0x14198E1E0).
param([string]$Exe = 'C:\Users\User\Desktop\DS3 Decompilation\game\DarkSoulsIII.exe', [uint64[]]$Targets)
$ErrorActionPreference = 'Stop'
Add-Type -TypeDefinition @'
using System; using System.Collections.Generic;
public static class Scan {
  public static List<string> Run(string path, ulong[] targets) {
    byte[] B = System.IO.File.ReadAllBytes(path); ulong Base = 0x140000000;
    int pe = BitConverter.ToInt32(B, 0x3C); int ns = BitConverter.ToUInt16(B, pe + 6); int opt = BitConverter.ToUInt16(B, pe + 20); int so = pe + 24 + opt;
    var r = new List<string>(); var set = new HashSet<ulong>(targets);
    for (int i = 0; i < ns; i++) { int o = so + 40 * i; string nm = System.Text.Encoding.ASCII.GetString(B, o, 8).TrimEnd('\0');
      uint va = BitConverter.ToUInt32(B, o + 12), rs = BitConverter.ToUInt32(B, o + 16), ro = BitConverter.ToUInt32(B, o + 20);
      uint ch = BitConverter.ToUInt32(B, o + 36); if ((ch & 0x20000000) == 0) continue;
      for (uint k = 0; k + 5 <= rs; k++) { byte op = B[ro + k];
        if (op == 0xE8 || op == 0xE9) { int d = BitConverter.ToInt32(B, (int)(ro + k + 1)); ulong t = (ulong)((long)(Base + va + k + 5) + d); if (set.Contains(t)) r.Add(string.Format("{0} 0x{1:X} -> 0x{2:X} ({3})", nm, Base + va + k, t, op == 0xE8 ? "call" : "jmp")); }
        if (k + 7 <= rs && (op & 0xF0) == 0x40 && (B[ro + k + 1] == 0x8D) && ((B[ro + k + 2] & 0xC7) == 0x05)) { int d = BitConverter.ToInt32(B, (int)(ro + k + 3)); ulong t = (ulong)((long)(Base + va + k + 7) + d); if (set.Contains(t)) r.Add(string.Format("{0} 0x{1:X} lea 0x{2:X}", nm, Base + va + k, t)); }
      } }
    return r; } }
'@
[Scan]::Run($Exe, $Targets)
