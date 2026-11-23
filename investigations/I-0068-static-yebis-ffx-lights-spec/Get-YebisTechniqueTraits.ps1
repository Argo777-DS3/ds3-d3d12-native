[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Exe,          # game/DarkSoulsIII.exe (1.15.2)
    [Parameter(Mandatory = $true)][string]$Techniques,   # evidence/I-0067/queries/yebis-techniques.tsv
    [Parameter(Mandatory = $true)][string]$Out
)
# I-0068: per YEBIS technique, the traits a capture can see without bytecode: the VS input size (vertex stride, from the
# VS input signature) and the PS bound textures and samplers (RDEF). The VS and PS blobs are found in the executable by
# their SHA-256 from the technique table (every DXBC container of the file is hashed once). Read-only; writes only $Out.
$ErrorActionPreference = 'Stop'
$src = @'
using System; using System.Text; using System.Collections.Generic; using System.Security.Cryptography;
public static class YebisBlobs {
  static uint U(byte[] b, int o) { return BitConverter.ToUInt32(b, o); }
  static string Z(byte[] b, int o) { int e = o; while (e < b.Length && b[e] != 0) e++; return Encoding.ASCII.GetString(b, o, e - o); }
  public static Dictionary<string,int> Index(byte[] f) {
    var map = new Dictionary<string,int>(); var sha = SHA256.Create();
    for (int i = 0; i + 32 <= f.Length; i++) {
      if (f[i] != 'D' || f[i+1] != 'X' || f[i+2] != 'B' || f[i+3] != 'C') continue;
      uint size = U(f, i + 24); if (size < 32 || i + size > f.Length || U(f, i + 20) != 1) continue;
      string h = BitConverter.ToString(sha.ComputeHash(f, i, (int)size)).Replace("-", "");
      if (!map.ContainsKey(h)) map[h] = i;
    }
    return map;
  }
  // VS: total input bytes (4 per used component of every input element). PS: texture slot count (array sizes included), textures and samplers from RDEF.
  public static string Traits(byte[] f, int i) {
    uint n = U(f, i + 28); int inBytes = 0; var tex = new List<string>(); int samplers = 0; int texSlots = 0; var isg = new StringBuilder();
    for (int c = 0; c < n; c++) {
      int co = i + (int)U(f, i + 32 + 4 * c); string tag = Encoding.ASCII.GetString(f, co, 4); int d = co + 8;
      if (tag == "ISGN") { uint cnt = U(f, d); for (int k = 0; k < cnt; k++) { int e = d + 8 + k * 24; uint sys = U(f, e + 8); int mask = f[e + 20];
          string nm = Z(f, d + (int)U(f, e)); isg.Append(nm).Append(U(f, e + 4)).Append(' ');
          if (sys == 0) { for (int m = 0; m < 4; m++) if ((mask >> m & 1) != 0) inBytes += 4; } } }
      if (tag == "RDEF") { uint bCount = U(f, d + 8), bOff = U(f, d + 12);
        for (int k = 0; k < bCount; k++) { int b = d + (int)bOff + k * 32; uint ty = U(f, b + 4);
          if (ty == 2) { uint cnt2 = U(f, b + 24); texSlots += (int)cnt2; tex.Add(Z(f, d + (int)U(f, b)) + "@t" + U(f, b + 20) + (cnt2 > 1 ? "x" + cnt2 : "")); } if (ty == 3) samplers++; } }
    }
    return inBytes + "\t" + isg.ToString().Trim() + "\t" + texSlots + "\t" + string.Join(" ", tex) + "\t" + samplers;
  }
}
'@
Add-Type -TypeDefinition $src
$bytes = [IO.File]::ReadAllBytes((Resolve-Path $Exe).Path)
$index = [YebisBlobs]::Index($bytes)
$rows = foreach ($t in (Import-Csv -LiteralPath $Techniques -Delimiter "`t")) {
    $vs = if ($index.ContainsKey($t.vs_sha256)) { ([YebisBlobs]::Traits($bytes, $index[$t.vs_sha256]) -split "`t") } else { @('?', '?', '', '', '') }
    $ps = if ($index.ContainsKey($t.ps_sha256)) { ([YebisBlobs]::Traits($bytes, $index[$t.ps_sha256]) -split "`t") } else { @('', '', '?', '?', '?') }
    "$($t.technique)`t$($t.vs)`t$($vs[0])`t$($t.ps)`t$($ps[2])`t$($ps[3])`t$($ps[4])"
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
@("technique`tvs`tvs_input_bytes`tps`tps_texture_count`tps_textures`tps_samplers") + $rows | Set-Content -LiteralPath $Out -Encoding utf8
"blobs=$($index.Count) rows=$(@($rows).Count) -> $Out"
