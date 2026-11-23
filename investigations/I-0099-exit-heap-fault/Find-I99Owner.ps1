param([string]$Exe = 'C:\Users\User\Desktop\DS3 Decompilation\game\DarkSoulsIII.exe', [uint64[]]$Va, [uint64[]]$Vtable, [int]$Dis = 0, [uint64[]]$CodeRefs)
# For each VA: the function that contains it (.pdata, chained unwind info followed), the vtables whose slots point at that
# function's start, and their RTTI class names. For each -Vtable: its RTTI class name and first slots.
$ErrorActionPreference = 'Stop'
Add-Type -TypeDefinition @'
using System; using System.Collections.Generic; using System.Text;
public class Pe {
  public byte[] B; public ulong Base = 0x140000000; public List<uint[]> Sec = new List<uint[]>(); public List<string> Names = new List<string>();
  public Pe(string p) { B = System.IO.File.ReadAllBytes(p); int pe = BitConverter.ToInt32(B, 0x3C); int ns = BitConverter.ToUInt16(B, pe + 6); int opt = BitConverter.ToUInt16(B, pe + 20); int so = pe + 24 + opt;
    for (int i = 0; i < ns; i++) { int o = so + 40 * i; Names.Add(Encoding.ASCII.GetString(B, o, 8).TrimEnd('\0'));
      Sec.Add(new uint[] { BitConverter.ToUInt32(B, o + 12), BitConverter.ToUInt32(B, o + 8), BitConverter.ToUInt32(B, o + 20), BitConverter.ToUInt32(B, o + 16) }); } }
  public long Off(ulong rva) { foreach (var s in Sec) if (rva >= s[0] && rva < (ulong)s[0] + s[3]) return (long)(s[2] + (rva - s[0])); return -1; }
  public string SecOf(ulong rva) { for (int i = 0; i < Sec.Count; i++) { var s = Sec[i]; if (rva >= s[0] && rva < (ulong)s[0] + Math.Max(s[1], s[3])) return Names[i]; } return "?"; }
  public uint U32(ulong rva) { long o = Off(rva); return o < 0 ? 0 : BitConverter.ToUInt32(B, (int)o); }
  public ulong U64(ulong rva) { long o = Off(rva); return o < 0 ? 0 : BitConverter.ToUInt64(B, (int)o); }
  public bool IsCode(ulong va) { if (va < Base) return false; string s = SecOf(va - Base); return s == ".text"; }
  public uint[] Func(ulong rva) { // .pdata
    long po = -1; uint psz = 0; for (int i = 0; i < Names.Count; i++) if (Names[i] == ".pdata") { po = Sec[i][2]; psz = Sec[i][1]; }
    int n = (int)(psz / 12); int lo = 0, hi = n - 1;
    while (lo <= hi) { int m = (lo + hi) / 2; uint b = BitConverter.ToUInt32(B, (int)po + 12 * m), e = BitConverter.ToUInt32(B, (int)po + 12 * m + 4);
      if (rva < b) hi = m - 1; else if (rva >= e) lo = m + 1; else { uint u = BitConverter.ToUInt32(B, (int)po + 12 * m + 8); uint begin = b, end = e;
        for (int g = 0; g < 8; g++) { long uo = Off(u & ~1u); if (uo < 0) break; byte fl = (byte)(B[uo] >> 3); if ((fl & 4) == 0) break; int cnt = B[uo + 2]; long ch = uo + 4 + ((cnt + 1) & ~1) * 2; begin = BitConverter.ToUInt32(B, (int)ch); end = BitConverter.ToUInt32(B, (int)ch + 4); u = BitConverter.ToUInt32(B, (int)ch + 8); }
        return new uint[] { b, e, begin, end }; } }
    return null; }
  public string Rtti(ulong vt) { ulong col = U64(vt - Base - 8); if (col < Base || SecOf(col - Base) != ".rdata") return null; uint sig = U32(col - Base); if (sig != 1) return null;
    uint td = U32(col - Base + 12); long o = Off((ulong)td + 16); if (o < 0) return null; int e = (int)o; while (e < B.Length && B[e] != 0) e++; return Encoding.ASCII.GetString(B, (int)o, e - (int)o); }
  public List<ulong> RipRefs(ulong target) { var r = new List<ulong>(); for (int i = 0; i < Names.Count; i++) { if (Names[i] != ".text") continue; var s = Sec[i];
      for (uint k = 0; k + 7 <= s[3]; k++) { long o = s[2] + k; byte p = B[o]; if ((p & 0xF0) != 0x40) continue; byte op = B[o + 1]; if (op != 0x8D && op != 0x8B && op != 0x89 && op != 0xC7) continue;
        byte modrm = B[o + 2]; if ((modrm & 0xC7) != 0x05) continue; int disp = BitConverter.ToInt32(B, (int)o + 3); int len = op == 0xC7 ? 11 : 7;
        ulong next = Base + s[0] + k + (ulong)len; if ((ulong)((long)next + disp) == target) r.Add(Base + s[0] + k); } } return r; }
  public List<string> Refs(ulong target) { var r = new List<string>(); for (int i = 0; i < Names.Count; i++) { if (Names[i] != ".rdata" && Names[i] != ".data") continue; var s = Sec[i];
      for (uint k = 0; k + 8 <= s[3]; k += 8) { if (BitConverter.ToUInt64(B, (int)(s[2] + k)) != target) continue; ulong a = Base + s[0] + k; ulong st = a; int slot = 0;
        while (slot < 400 && IsCode(U64(st - Base - 8)) && Rtti(st) == null) { st -= 8; slot++; }
        string name = Rtti(st); r.Add(string.Format("0x{0:X} slot {1} of vtable 0x{2:X} {3}", a, slot, st, name ?? "(no RTTI)")); } } return r; }
}
'@
$pe = New-Object Pe $Exe
foreach ($v in $Va) { $rva = $v - $pe.Base; $f = $pe.Func($rva)
  if (-not $f) { '0x{0:X}: no .pdata entry' -f $v; continue }
  '0x{0:X}: fragment 0x{1:X}-0x{2:X}, function 0x{3:X}-0x{4:X}' -f $v, ($pe.Base + $f[0]), ($pe.Base + $f[1]), ($pe.Base + $f[2]), ($pe.Base + $f[3])
  foreach ($r in $pe.Refs($pe.Base + $f[2])) { '    ' + $r }
  if ($Dis) { $o = $pe.Off($f[2]); '    bytes: ' + (($pe.B[$o..($o + $Dis - 1)] | ForEach-Object { '{0:x2}' -f $_ }) -join ' ') } }
foreach ($t in $Vtable) { '{0:X}: {1}' -f $t, $pe.Rtti($t); for ($k = 0; $k -lt 6; $k++) { '    slot {0}: 0x{1:X}' -f $k, $pe.U64($t - $pe.Base + 8 * $k) } }
foreach ($t in $CodeRefs) { 'code referring to 0x{0:X} ({1}):' -f $t, $pe.Rtti($t); foreach ($a in $pe.RipRefs($t)) { $fn = $pe.Func($a - $pe.Base); '    0x{0:X} in function 0x{1:X}' -f $a, $(if ($fn) { $pe.Base + $fn[2] } else { 0 }) } }
