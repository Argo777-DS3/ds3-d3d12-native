# I-0099 (2026-10-25): the import each IAT slot holds (DS3 calls its imports through rip-relative slots), from the import
# directory of the executable on disk. Read-only. Pass slots as an array: -Slots @(0x144DBBC64).
param([string]$Exe = 'C:\Users\User\Desktop\DS3 Decompilation\game\DarkSoulsIII.exe', [uint64[]]$Slots)
Add-Type -TypeDefinition @'
using System; using System.Collections.Generic; using System.Text;
public static class Iat {
  static byte[] B; static List<uint[]> Sec = new List<uint[]>();
  static long Off(uint rva) { foreach (var s in Sec) if (rva >= s[0] && rva < s[0] + Math.Max(s[1], s[3])) return s[2] + (rva - s[0]); return -1; }
  static string Str(uint rva) { long o = Off(rva); if (o < 0) return "?"; int e = (int)o; while (B[e] != 0) e++; return Encoding.ASCII.GetString(B, (int)o, e - (int)o); }
  public static List<string> Run(string p, ulong[] slots) {
    B = System.IO.File.ReadAllBytes(p); int pe = BitConverter.ToInt32(B, 0x3C); int ns = BitConverter.ToUInt16(B, pe + 6); int opt = BitConverter.ToUInt16(B, pe + 20); int so = pe + 24 + opt;
    for (int i = 0; i < ns; i++) { int o = so + 40 * i; Sec.Add(new uint[] { BitConverter.ToUInt32(B, o + 12), BitConverter.ToUInt32(B, o + 8), BitConverter.ToUInt32(B, o + 20), BitConverter.ToUInt32(B, o + 16) }); }
    int dd = pe + 24 + 112; uint impRva = BitConverter.ToUInt32(B, dd + 8);
    var map = new Dictionary<ulong, string>(); long d = Off(impRva);
    for (; ; d += 20) { uint ilt = BitConverter.ToUInt32(B, (int)d), nameR = BitConverter.ToUInt32(B, (int)d + 12), iat = BitConverter.ToUInt32(B, (int)d + 16); if (nameR == 0) break;
      string dll = Str(nameR); uint look = ilt != 0 ? ilt : iat;
      for (uint k = 0; ; k += 8) { long lo = Off(look + k); if (lo < 0) break; ulong v = BitConverter.ToUInt64(B, (int)lo); if (v == 0) break;
        string fn = (v >> 63) != 0 ? "#" + (v & 0xFFFF) : Str((uint)v + 2); map[0x140000000UL + iat + k] = dll + "!" + fn; } }
    var r = new List<string>(); r.Add("import entries: " + map.Count);
    foreach (var s in slots) { string n; r.Add(string.Format("0x{0:X} {1}", s, map.TryGetValue(s, out n) ? n : "(not an IAT slot)")); }
    return r; } }
'@
[Iat]::Run($Exe, $Slots)
