[CmdletBinding()]
param(
    [string]$Programs = 'C:\Users\User\.codex\I0063-cache\programs',   # I-0063 Extract-Programs.ps1 cache (licensed bytes, outside the repository)
    [string]$Match = '^gxffxshader__',
    [Parameter(Mandatory = $true)][string]$Out
)
# I-0068: full reflection of the FFX programmes (every DXBC in the I-0063 programme cache whose name matches): stage,
# input signature, bound resources (name, type, register, count) and every constant-buffer variable (offset, size, used
# flag), one TSV row per programme. Read-only; writes only $Out.
$ErrorActionPreference = 'Stop'
$src = @'
using System; using System.IO; using System.Text; using System.Collections.Generic;
public static class DxbcFull {
  static string Z(byte[] b, int o) { int e = o; while (e < b.Length && b[e] != 0) e++; return Encoding.ASCII.GetString(b, o, e - o); }
  static uint U(byte[] b, int o) { return BitConverter.ToUInt32(b, o); }
  static readonly string[] RT = { "cbuffer", "tbuffer", "texture", "sampler", "uav", "struct", "uavrw", "byteaddr", "uavbyteaddr", "append", "consume", "uavcounter" };
  public static string Row(string path) {
    byte[] f = File.ReadAllBytes(path);
    int i = 0; while (i + 4 <= f.Length && !(f[i] == 'D' && f[i+1] == 'X' && f[i+2] == 'B' && f[i+3] == 'C')) i++;
    if (i + 32 > f.Length) return "nodxbc\t\t\t";
    uint n = U(f, i + 28); string stage = "?"; var res = new StringBuilder(); var cbs = new StringBuilder(); var isg = new StringBuilder(); var osg = new StringBuilder();
    for (int c = 0; c < n; c++) {
      int co = i + (int)U(f, i + 32 + 4 * c); string tag = Encoding.ASCII.GetString(f, co, 4); int d = co + 8;
      if (tag == "SHDR" || tag == "SHEX") { uint t = U(f, d) >> 16; stage = t == 0 ? "ps" : t == 1 ? "vs" : t == 2 ? "gs" : t == 3 ? "hs" : t == 4 ? "ds" : t == 5 ? "cs" : "t" + t; }
      if (tag == "ISGN" || tag == "ISG1") { uint cnt = U(f, d); int stride = tag == "ISGN" ? 24 : 32; int off = tag == "ISGN" ? 8 : 8;
        for (int k = 0; k < cnt && k < 32; k++) { int e = d + off + k * stride; int nameOff = tag == "ISGN" ? (int)U(f, e) : (int)U(f, e + 4); int idx = tag == "ISGN" ? (int)U(f, e + 4) : (int)U(f, e + 8);
          isg.Append(Z(f, d + nameOff)).Append(idx).Append(' '); } }
      if (tag == "OSGN" || tag == "OSG5" || tag == "OSG1") { uint cnt = U(f, d); int stride = tag == "OSGN" ? 24 : 28; for (int k = 0; k < cnt && k < 32; k++) { int e = d + 8 + k * stride; int nameOff = tag == "OSGN" ? (int)U(f, e) : (int)U(f, e + 4); int idx = tag == "OSGN" ? (int)U(f, e + 4) : (int)U(f, e + 8); osg.Append(Z(f, d + nameOff)).Append(idx).Append(" "); } }
      if (tag != "RDEF") continue;
      uint cbCount = U(f, d), cbOff = U(f, d + 4), bCount = U(f, d + 8), bOff = U(f, d + 12); int vstride = ((U(f, d + 16) >> 8) & 0xFF) >= 5 ? 40 : 24;
      for (int k = 0; k < bCount && k < 64; k++) { int b = d + (int)bOff + k * 32; uint ty = U(f, b + 4);
        res.Append(Z(f, d + (int)U(f, b))).Append(':').Append(ty < RT.Length ? RT[ty] : ty.ToString()).Append('@').Append(U(f, b + 20)).Append(U(f, b + 24) > 1 ? "x" + U(f, b + 24) : "").Append(' '); }
      for (int k = 0; k < cbCount && k < 32; k++) {
        int b = d + (int)cbOff + k * 24; string name = Z(f, d + (int)U(f, b)); uint vc = U(f, b + 4), vo = U(f, b + 8), csz = U(f, b + 12);
        cbs.Append(name).Append('[').Append(csz).Append("]{");
        for (int v = 0; v < vc && v < 256; v++) { int vb = d + (int)vo + v * vstride;
          cbs.Append(v == 0 ? "" : ",").Append(Z(f, d + (int)U(f, vb))).Append('@').Append(U(f, vb + 4)).Append(':').Append(U(f, vb + 8)).Append((U(f, vb + 12) & 2) != 0 ? "*" : ""); }
        cbs.Append("} ");
      }
    }
    return stage + "\t" + isg.ToString().Trim() + "\t" + osg.ToString().Trim() + "\t" + res.ToString().Trim() + "\t" + cbs.ToString().Trim();
  }
}
'@
Add-Type -TypeDefinition $src
$rows = foreach ($p in Get-ChildItem -LiteralPath $Programs -File | Where-Object { $_.Name -match $Match } | Sort-Object Name) {
    "$($p.Name -replace $Match, '')`t$([DxbcFull]::Row($p.FullName))"
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
@("program`tstage`tinputs`toutputs`tresources`tconstant buffers (name[size]{var@offset:size, * = used})") + $rows | Set-Content -LiteralPath $Out -Encoding utf8
"rows=$($rows.Count) -> $Out"
