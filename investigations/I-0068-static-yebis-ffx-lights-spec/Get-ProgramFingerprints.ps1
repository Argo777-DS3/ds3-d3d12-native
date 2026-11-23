[CmdletBinding()]
param(
    [string]$Programs = 'C:\Users\User\.codex\I0063-cache\programs',   # loose DXBC programmes (<bundle>__<name>.<ext>)
    [string]$ShaderSet,                                                 # ffx-shader-set.tsv (Get-ShaderSetTable.ps1), optional
    [Parameter(Mandatory = $true)][string]$Out                          # output directory
)
# I-0068 part AM: fingerprints of the loose programmes, so that a draw seen in a capture (original or native) can be matched
# to its programme name without reflection. Two keys per blob:
#   sha256   SHA-256 of the whole DXBC file
#   dxbc     the 16-byte checksum stored in the DXBC header (offset 4), the "shader hash" capture tools show
# Output files (Out):
#   programs.tsv          one row per file: bundle, program, ext, stage, bytes, sha256, dxbc
#   ffx-index-hashes.tsv  (with -ShaderSet) one row per FFX shader-set index 0..157: VS and PS names, sha256 and dxbc of each,
#                         then the hull/domain variants of the tessellated programmes
$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Force -Path $Out | Out-Null
$stage = @{ vpo = 'vs'; ppo = 'ps'; hpo = 'hs'; dpo = 'ds'; gpo = 'gs'; cpo = 'cs' }
$sha = [Security.Cryptography.SHA256]::Create()
$rows = New-Object System.Collections.ArrayList
$null = $rows.Add("bundle`tprogram`text`tstage`tbytes`tsha256`tdxbc")
$byKey = @{}
foreach ($f in Get-ChildItem -LiteralPath $Programs -File | Sort-Object Name) {
    $b = [IO.File]::ReadAllBytes($f.FullName)
    if ($b.Length -lt 20 -or [Text.Encoding]::ASCII.GetString($b, 0, 4) -ne 'DXBC') { continue }
    $h = ([BitConverter]::ToString($sha.ComputeHash($b))).Replace('-', '')
    $c = ([BitConverter]::ToString($b, 4, 16)).Replace('-', '')
    $n = $f.BaseName; $ext = $f.Extension.TrimStart('.')
    $i = $n.IndexOf('__'); $bundle = if ($i -ge 0) { $n.Substring(0, $i) } else { '' }; $prog = if ($i -ge 0) { $n.Substring($i + 2) } else { $n }
    $null = $rows.Add("$bundle`t$prog`t$ext`t$($stage[$ext])`t$($b.Length)`t$h`t$c")
    if ($bundle -eq 'gxffxshader') { $byKey["$prog.$ext"] = @($h, $c, $b.Length) }
}
[IO.File]::WriteAllLines((Join-Path $Out 'programs.tsv'), $rows)
"programs.tsv: $($rows.Count - 1) rows"
if ($ShaderSet) {
    $vs = @{}; $ps = @{}
    foreach ($r in Import-Csv -Delimiter "`t" -LiteralPath $ShaderSet) {
        if ($r.array -eq 'apuStack_1d88') { $vs[[int]$r.index] = $r.name } elseif ($r.array -eq 'apuStack_1898') { $ps[[int]$r.index] = $r.name } }
    function Fp($name, $ext) { $k = "$name.$ext"; if ($byKey.ContainsKey($k)) { "$($byKey[$k][0].Substring(0, 16))/$($byKey[$k][1].Substring(0, 8))" } else { 'absent' } }
    $o = New-Object System.Collections.ArrayList
    $null = $o.Add("index`tvs`tvs_sha256_16/dxbc_8`tps`tps_file`tps_sha256_16/dxbc_8`ths_ds_variants")
    for ($i = 0; $i -lt 158; $i++) {
        $v = $vs[$i]; $p = $ps[$i]
        # glow twins (75..127, and the soft/distortion entries that carry a GlowD pixel programme) use <ps>GlowD.ppo when it exists
        $pf = $p; if ($i -ge 75 -and $i -le 127 -and $byKey.ContainsKey("${p}GlowD.ppo")) { $pf = "${p}GlowD" }
        $hd = ''
        if ($v -like 'GXFfxtessellate*') {
            $hd = (@('quad', 'octed', 'quincunx') | Where-Object { $byKey.ContainsKey("${v}_$_.hpo") } | ForEach-Object { "$_ hs=$(Fp "${v}_$_" 'hpo') ds=$(Fp "${v}_$_" 'dpo')" }) -join '; '
            if ($byKey.ContainsKey("$v.hpo")) { $hd = "single hs=$(Fp $v 'hpo') ds=$(Fp $v 'dpo')" }
        }
        $null = $o.Add("$i`t$v`t$(Fp $v 'vpo')`t$p`t$pf`t$(Fp $pf 'ppo')`t$hd")
    }
    [IO.File]::WriteAllLines((Join-Path $Out 'ffx-index-hashes.tsv'), $o)
    "ffx-index-hashes.tsv: 158 rows"
}
