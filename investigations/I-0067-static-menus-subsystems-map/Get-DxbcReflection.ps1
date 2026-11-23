# Read-only: lists every DXBC shader blob embedded in a file (offset, size, shader type,
# SHA-256, constant buffers with their used variables from the RDEF chunk, bound resources) as TSV.
# Usage: pwsh -NoProfile -File Get-DxbcReflection.ps1 -Path <exe> -Out <tsv> [-Filter <regex on variable names>]
param([Parameter(Mandatory)][string]$Path, [Parameter(Mandatory)][string]$Out, [string]$Filter = '')
$src = @'
using System; using System.IO; using System.Text; using System.Collections.Generic;
public static class Dxbc {
  static string Z(byte[] b, int o) { int e = o; while (e < b.Length && b[e] != 0) e++; return Encoding.ASCII.GetString(b, o, e - o); }
  static uint U(byte[] b, int o) { return BitConverter.ToUInt32(b, o); }
  public static List<string> Scan(string path) {
    byte[] f = File.ReadAllBytes(path); var rows = new List<string>();
    for (int i = 0; i + 32 < f.Length; i++) {
      if (f[i] != 'D' || f[i+1] != 'X' || f[i+2] != 'B' || f[i+3] != 'C') continue;
      uint size = U(f, i + 24), n = U(f, i + 28);
      if (size < 64 || size > 4000000 || n == 0 || n > 32 || i + size > f.Length) continue;
      string type = "?"; var cbs = new StringBuilder(); var res = new StringBuilder();
      for (int c = 0; c < n; c++) {
        int co = i + (int)U(f, i + 32 + 4 * c); if (co + 8 > f.Length) break;
        string tag = Encoding.ASCII.GetString(f, co, 4); int d = co + 8;
        if (tag == "SHDR" || tag == "SHEX") { uint ver = U(f, d); uint t = ver >> 16; type = t == 0 ? "ps" : t == 1 ? "vs" : t == 2 ? "gs" : t == 3 ? "hs" : t == 4 ? "ds" : t == 5 ? "cs" : "t" + t; }
        if (tag != "RDEF") continue;
        uint cbCount = U(f, d), cbOff = U(f, d + 4), bCount = U(f, d + 8), bOff = U(f, d + 12);
        for (int k = 0; k < bCount && k < 64; k++) { int b = d + (int)bOff + k * 32; res.Append(Z(f, d + (int)U(f, b))).Append(":t").Append(U(f, b + 4)).Append("s").Append(U(f, b + 20)).Append("b").Append(U(f, b + 24)).Append(' '); }
        for (int k = 0; k < cbCount && k < 32; k++) {
          int b = d + (int)cbOff + k * 24; string name = Z(f, d + (int)U(f, b)); uint vc = U(f, b + 4), vo = U(f, b + 8), csz = U(f, b + 12);
          cbs.Append(name).Append('[').Append(csz).Append("]{");
          bool first = true; for (int v = 0; v < vc && v < 256; v++) { int vb = d + (int)vo + v * 24; if ((U(f, vb + 12) & 2) == 0) continue; cbs.Append(first ? "" : ",").Append(Z(f, d + (int)U(f, vb))).Append('@').Append(U(f, vb + 4)).Append(':').Append(U(f, vb + 8)); first = false; }
          cbs.Append("} ");
        }
      }
      string sha; using (var h = System.Security.Cryptography.SHA256.Create()) sha = BitConverter.ToString(h.ComputeHash(f, i, (int)size)).Replace("-", "");
      rows.Add(string.Format("{0:X}\t{1}\t{2}\t{5}\t{3}\t{4}", i, size, type, cbs.ToString().Trim(), res.ToString().Trim(), sha));
      i += (int)size - 1;
    }
    return rows;
  }
}
'@
Add-Type -TypeDefinition $src
$rows = [Dxbc]::Scan((Resolve-Path $Path).Path)
if ($Filter) { $rows = $rows | Where-Object { $_ -match $Filter } }
@("offset`tbytes`ttype`tsha256`tused_constants`tresources") + $rows | Set-Content -Path $Out -Encoding utf8
"$($rows.Count) blobs"
