[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Dump,     # a minidump with an exception stream (I-0082's crash dumps)
    [int]$Frames = 40,
    [switch]$AllThreads,                     # also every other thread of the dump (2026-10-25)
    [string]$GameExe = 'C:\Users\User\Desktop\DS3 Decompilation\game\DarkSoulsIII.exe'
)
# I-0099: the faulting thread's stack, unwound from its exception context with the x64 unwind data (.pdata) of the modules'
# images on disk (the dump holds the stack, not the code), no debugger needed. A module whose image file is gone (an old
# worktree's game_bootstrap.dll) or whose file does not match the dump's timestamp is reported, and its frames are not
# unwound past. ntdll and kernel32 must be the same build as in the dump (checked by timestamp).
$ErrorActionPreference = 'Stop'
Add-Type -TypeDefinition @'
using System; using System.Collections.Generic; using System.IO; using System.Text;
public class Img { public byte[] B; public ulong Base; public uint Size; public string Name; public uint[] Sec; int ns; int so;
  public long Pdata, PdataSize;
  public Img(string path, ulong b, uint size, string name) { B = File.ReadAllBytes(path); Base = b; Size = size; Name = name;
    int pe = BitConverter.ToInt32(B, 0x3C); ns = BitConverter.ToUInt16(B, pe + 6); int opt = BitConverter.ToUInt16(B, pe + 20); so = pe + 24 + opt;
    Pdata = BitConverter.ToUInt32(B, pe + 24 + 112 + 3 * 8); PdataSize = BitConverter.ToUInt32(B, pe + 24 + 112 + 3 * 8 + 4); }
  public uint Stamp { get { return BitConverter.ToUInt32(B, BitConverter.ToInt32(B, 0x3C) + 8); } }
  public long Off(ulong rva) { for (int i = 0; i < ns; i++) { int o = so + 40 * i; uint va = BitConverter.ToUInt32(B, o + 12), rs = BitConverter.ToUInt32(B, o + 16), rp = BitConverter.ToUInt32(B, o + 20);
      if (rva >= va && rva < (ulong)va + rs) return rp + (long)(rva - va); } return -1; }
  public uint U32(ulong rva) { return BitConverter.ToUInt32(B, (int)Off(rva)); }
  public long Find(uint rva) { long po = Off((ulong)Pdata); int n = (int)(PdataSize / 12), lo = 0, hi = n - 1;
    while (lo <= hi) { int m = (lo + hi) / 2; uint b = BitConverter.ToUInt32(B, (int)po + 12 * m), e = BitConverter.ToUInt32(B, (int)po + 12 * m + 4);
      if (rva < b) hi = m - 1; else if (rva >= e) lo = m + 1; else return po + 12 * m; } return -1; } }
public class Unwinder {
  public byte[] D; public List<ulong[]> Mem = new List<ulong[]>(); public List<Img> Mods = new List<Img>(); public List<string> Notes = new List<string>();
  public ulong[] R = new ulong[16]; public ulong Rip;
  public bool Read(ulong va, out ulong v) { foreach (var m in Mem) if (va >= m[0] && va - m[0] + 8 <= m[1]) { v = BitConverter.ToUInt64(D, (int)(m[2] + (va - m[0]))); return true; } v = 0; return false; }
  public Img ModOf(ulong va) { foreach (var m in Mods) if (va >= m.Base && va < m.Base + m.Size) return m; return null; }
  ulong Pop() { ulong v; if (!Read(R[4], out v)) throw new Exception(string.Format("stack 0x{0:X} not in dump", R[4])); R[4] += 8; return v; }
  ulong Rd(ulong a) { ulong v; if (!Read(a, out v)) throw new Exception(string.Format("stack 0x{0:X} not in dump", a)); return v; }
  void Apply(Img m, uint uw, uint ipOff, bool all) {
    for (int g = 0; g < 32; g++) { long o = m.Off(uw); byte ver = (byte)(m.B[o] & 7), fl = (byte)(m.B[o] >> 3); int cnt = m.B[o + 2]; int freg = m.B[o + 3] & 15, foff = m.B[o + 3] >> 4;
      int i = 0; while (i < cnt) { long c = o + 4 + 2 * i; int at = m.B[c]; int op = m.B[c + 1] & 15, info = m.B[c + 1] >> 4; bool done = all || at <= ipOff;
        switch (op) {
          case 0: if (done) R[info] = Pop(); i += 1; break;
          case 1: { ulong size = info == 0 ? (ulong)BitConverter.ToUInt16(m.B, (int)c + 2) * 8 : BitConverter.ToUInt32(m.B, (int)c + 2); if (done) R[4] += size; i += info == 0 ? 2 : 3; break; }
          case 2: if (done) R[4] += (ulong)(info * 8 + 8); i += 1; break;
          case 3: if (done) R[4] = R[freg] - (ulong)(foff * 16); i += 1; break;
          case 4: if (done) R[info] = Rd(R[4] + (ulong)BitConverter.ToUInt16(m.B, (int)c + 2) * 8); i += 2; break;
          case 5: if (done) R[info] = Rd(R[4] + BitConverter.ToUInt32(m.B, (int)c + 2)); i += 3; break;
          case 6: i += (ver >= 2) ? 1 : 2; break;
          case 7: i += 3; break;
          case 8: i += 2; break;
          case 9: i += 3; break;
          case 10: { ulong b = R[4] + (ulong)(info != 0 ? 8 : 0); Rip = Rd(b); R[4] = Rd(b + 24); return; }
          default: throw new Exception("unknown unwind op " + op); } }
      if ((fl & 4) == 0) break;
      long ch = o + 4 + ((cnt + 1) & ~1) * 2; uw = BitConverter.ToUInt32(m.B, (int)ch + 8); all = true; }
    Rip = Pop(); }
  public bool Step(bool top) { Img m = ModOf(Rip); if (m == null) { Notes.Add(string.Format("0x{0:X}: no module", Rip)); return false; }
    uint rva = (uint)(Rip - m.Base); long f = m.Find(top ? rva : rva - 1);
    if (f < 0) { Rip = Pop(); return true; }   // a leaf
    uint begin = BitConverter.ToUInt32(m.B, (int)f), uw = BitConverter.ToUInt32(m.B, (int)f + 8);
    if ((uw & 1) != 0) { uw = BitConverter.ToUInt32(m.B, (int)m.Off(uw & ~1u) + 8); }
    Apply(m, uw, rva - begin, false); return true; }
}
'@
$b = [IO.File]::ReadAllBytes($Dump)
function U32($o) { [BitConverter]::ToUInt32($b, [int]$o) }
function U64($o) { [BitConverter]::ToUInt64($b, [int]$o) }
$streams = @{}; $n = U32 8; $dir = U32 12
for ($i = 0; $i -lt $n; $i++) { $e = $dir + 12 * $i; $streams[[int](U32 $e)] = U32 ($e + 8) }
$u = New-Object Unwinder; $u.D = $b
if ($streams.ContainsKey(9)) { $r = $streams[9]; $c = U64 $r; $off = U64 ($r + 8)
    for ($i = 0; $i -lt $c; $i++) { $d = $r + 16 + 16 * $i; $sz = U64 ($d + 8); $u.Mem.Add([uint64[]]@((U64 $d), $sz, $off)); $off += $sz } }
if ($streams.ContainsKey(5)) { $r = $streams[5]; $c = U32 $r
    for ($i = 0; $i -lt $c; $i++) { $d = $r + 4 + 16 * $i; $u.Mem.Add([uint64[]]@((U64 $d), [uint64](U32 ($d + 8)), [uint64](U32 ($d + 12)))) } }
$r = $streams[4]; $c = U32 $r
for ($i = 0; $i -lt $c; $i++) { $m = $r + 4 + 108 * $i; $base = U64 $m; $size = U32 ($m + 8); $stamp = U32 ($m + 16)
    $nr = U32 ($m + 20); $path = [Text.Encoding]::Unicode.GetString($b, [int]$nr + 4, [int](U32 $nr)); $name = [IO.Path]::GetFileName($path)
    $file = if ($name -ieq 'DarkSoulsIII.exe') { $GameExe } else { $path }
    if (-not (Test-Path -LiteralPath $file)) { $u.Notes.Add("$name image not on disk ($path)"); continue }
    $img = New-Object Img $file, $base, $size, $name
    if ($img.Stamp -ne $stamp) { $u.Notes.Add(('{0}: timestamp 0x{1:X} on disk, 0x{2:X} in the dump: not used' -f $name, $img.Stamp, $stamp)); continue }
    $u.Mods.Add($img) }
# -AllThreads (2026-10-25): every thread of the dump's thread list, each from its own saved context (a waiting thread's
# handle is in r10 at the system call), to find what the other threads were doing at the fault (the destroying thread).
function Walk($tid, $ctx, $title) {
    for ($k = 0; $k -lt 16; $k++) { $u.R[$k] = U64 ($ctx + 0x78 + 8 * $k) }
    $u.Rip = U64 ($ctx + 0xF8)
    $title
    for ($k = 0; $k -lt $Frames; $k++) {
        $m = $u.ModOf($u.Rip); $label = if ($m) { '{0}+0x{1:X}' -f $m.Name, ($u.Rip - $m.Base) } else { '0x{0:X}' -f $u.Rip }
        '{0,2} {1,-34} rsp 0x{2:X}' -f $k, $label, $u.R[4]
        if (-not $u.Rip) { break }
        try { if (-not $u.Step($k -eq 0)) { break } } catch { "   stop: $($_.Exception.InnerException.Message ?? $_.Exception.Message)"; break } } }
$r = $streams[6]; $tid = U32 $r; $ctx = U32 ($r + 8 + 152 + 4)
Walk $tid $ctx ("thread $tid, exception 0x{0:X8}" -f (U32 ($r + 8)))
if ($AllThreads) { $t = $streams[3]; $c = U32 $t
    for ($i = 0; $i -lt $c; $i++) { $e = $t + 4 + 48 * $i; $other = U32 $e; if ($other -eq $tid) { continue }
        $oc = U32 ($e + 44); Walk $other $oc ('== thread {0} (r10 0x{1:X} at its top)' -f $other, (U64 ($oc + 0x78 + 8 * 10))) } }
$u.Notes | ForEach-Object { "note: $_" }
