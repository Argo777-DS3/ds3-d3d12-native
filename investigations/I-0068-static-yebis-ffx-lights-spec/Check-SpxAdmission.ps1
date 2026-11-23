[CmdletBinding()]
param(
    [string]$ShaderBdle = 'C:\Users\User\.codex\I0063-cache\shaderbnd\shader_shaderbdle.shaderbdlebnd.dcx',
    [Parameter(Mandatory = $true)][string]$Out           # output TSV, one row per effect (shaderbdle) with a .metaparam
)
# I-0068 part AR: replays the two readers the native effect admission (game_bootstrap.cpp AdmitEffect) runs on the effect's
# declaration blob, ReadModelParameterDeclarations and ReadModelTextureDeclarations (native/content/model_parameters.cpp and
# model_textures.cpp), on the .metaparam of each of the 113 nested shaderbdle effects, read-only. A reader that throws makes
# AdmitEffect log effect_cpu_failure, so the family would have no native programmes. Also lists the texture declarations (name,
# slot, kind) and the numeric parameter count; the generated-parameter descriptors (0x60 each) are summarised (count, register).
$ErrorActionPreference = 'Stop'
Add-Type -Path 'C:\Users\User\.codex\tools\SoulsFormats-1.0.1\lib\net462\SoulsFormats.dll'
function Num([byte[]]$b, [long]$at, [int]$n) {
    if ($at -lt 0 -or $at -gt $b.Length -or $n -gt $b.Length - $at) { throw 'field extent' }
    [uint64]$v = 0; for ($i = 0; $i -lt $n; $i++) { $v = $v -bor ([uint64]$b[$at + $i] -shl (8 * $i)) }; $v
}
function Name([byte[]]$b, [long]$at) {
    if ($at -band 1) { throw 'name alignment' }
    $sb = New-Object Text.StringBuilder
    for ($i = 0; $i -le 2048; $i++) { $ch = [int](Num $b $at 2); if ($ch -eq 0) { if ($sb.Length -eq 0) { throw 'empty name' }; return $sb.ToString() }; if ($i -eq 2048) { throw 'name bound' }; [void]$sb.Append([char]$ch); $at += 2 }
    throw 'name extent'
}
$rows = New-Object System.Collections.ArrayList
[void]$rows.Add("effect`tmetaparam_bytes`tversion`tparameter_count`tparameter_reader`ttexture_count`ttexture_reader`ttexture_declarations`tgenerated_count`tgenerated_register")
$outer = [SoulsFormats.BND4]::Read($ShaderBdle)
foreach ($f in $outer.Files) {
    $n = [SoulsFormats.BND4]::Read($f.Bytes)
    $m = $n.Files | Where-Object { $_.Name -like '*.metaparam' } | Select-Object -First 1
    $eff = Split-Path -Leaf (Split-Path -Parent $f.Name)
    if (-not $m) { [void]$rows.Add("$eff`t0`t`t`tNO-METAPARAM`t`t`t`t`t"); continue }
    $b = $m.Bytes; $ver = [uint32](Num $b 8 4); $pcount = ''; $pread = 'OK'; $tcount = ''; $tread = 'OK'; $tdecl = ''
    try {
        if ($b.Length -lt 0x98 -or $ver -le 1) { throw 'version envelope' }
        $table = [long](Num $b 0x20 8); $pcount = [int](Num $b 0x2C 4)
        if ($pcount -gt 4096 -or $table -gt $b.Length -or $pcount -gt [long](($b.Length - $table) / 0x50)) { throw 'table extent' }
        $names = @{}
        for ($i = 0; $i -lt $pcount; $i++) { $at = $table + 0x50 * $i; $nm = Name $b ([long](Num $b $at 8)); if ($names.ContainsKey($nm)) { throw "name collision $nm" }; $names[$nm] = 1 }
    } catch { $pread = "THROWS: $($_.Exception.Message)" }
    try {
        if ($b.Length -lt 0x98 -or $ver -le 1) { throw 'version envelope' }
        $tcount = [int](Num $b 0xC 1)
        if ($tcount -gt [long](($b.Length - 0x98) / 0x30)) { throw 'table extent' }
        $list = @()
        for ($i = 0; $i -lt $tcount; $i++) { $at = 0x98 + $i * 0x30; $nm = Name $b ([long](Num $b $at 8)); $slot = [int](Num $b ($at + 9) 1); $kind = [int](Num $b ($at + 0xB) 1); if ($slot -ge 128) { throw 'slot envelope' }; $list += "${nm}:t${slot}:k${kind}" }
        $tdecl = $list -join ' '
    } catch { $tread = "THROWS: $($_.Exception.Message)" }
    $gc = [uint32](Num $b 0x28 4); $gr = '0x{0:X}' -f [uint32](Num $b 0x50 4)
    [void]$rows.Add(("{0}`t{1}`t{2}`t{3}`t{4}`t{5}`t{6}`t{7}`t{8}`t{9}" -f $eff, $b.Length, $ver, $pcount, $pread, $tcount, $tread, $tdecl, $gc, $gr))
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
[IO.File]::WriteAllLines($Out, $rows)
"rows $($rows.Count - 1) -> $Out"
