[CmdletBinding(PositionalBinding = $false)]
param(
    [string]$Root = (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)),
    [string]$Out = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'evidence/I-0068/queries/water-ready/water-tess-blocks.tsv')
)
# I-0068 part BO: the reference words of the water and tessellation sheet, computed from the retained data and the constants read in
# the executable (nothing is launched):
#  - cbPNTriangles (0x30 bytes written by FUN_1402302C0, rows 0-2) for every GX04 record with a non-zero mode: the map materials of
#    the five zones (zone-water-tess.tsv of Get-ZoneWaterTess.ps1) and the character materials (all-chr-gx.tsv of Dump-FlverGx.ps1).
#    Constants: 64.0 (143D7EFAC), 0.001 (143D7ED10 = 3A83126F), 1.0 (143D7EE10), 0.99 (143D7EE00 = 3F7D70A4).
#  - the water default textures (the 17-colour table of FUN_140006C00): t54 default = entry 1 (black, 0xFF000000), t57 default =
#    entry 2 (grey 0xFF808080), and the neutral interaction value in R16G16_FLOAT.
#  - the planar reflection matrix of FUN_1400FB0E0 for sample planes: M = I - 2 n n^T with the translation row -2 d n (row-vector
#    convention, constants 143D1BF90.. = identity rows, 143D4EDC0 = (-2,-2,-2,0)), then the Y flip of the constants 143D7F4A0 etc.
$ErrorActionPreference = 'Stop'
$inv = [Globalization.CultureInfo]::InvariantCulture
function U32([single]$f) { [BitConverter]::ToUInt32([BitConverter]::GetBytes($f), 0) }
function Hex([uint32]$w) { '{0:X8}' -f $w }
function F32([string]$s) { [single]::Parse($s, $inv) }
function MinF([single]$a, [single]$b) { if ($b -le $a) { $b } else { $a } }   # the executable's compare-and-select: the second operand wins unless it is greater
$rows = New-Object System.Collections.Generic.List[string]
$rows.Add("kind`tkey`tfield`thex`tnote")
# --- cbPNTriangles
$seen = @{}
function PnBlock([string]$origin, [string]$name, [uint32]$disp, [uint32]$mode, [single]$edge, [single]$inside, [single]$d7, [single]$start, [single]$end, [single]$d10, [single]$d11, [bool]$e4) {
    if ($mode -eq 0) { return }
    $key = "$mode/$disp/$edge/$inside/$d7/$start/$end/$d10/$d11"
    if ($seen.ContainsKey($key)) { $seen[$key] += ",$origin"; return }
    $seen[$key] = $origin
    $e = [uint32]$edge; $i = [uint32]$inside      # +0x258 / +0x25C hold (int)float
    $r0x = if ([single]64.0 -le [single]$e) { [single]64.0 } else { [single]$e }
    $r0y = if ([single]64.0 -le [single]$i) { [single]64.0 } else { [single]$i }
    $diff = [single]($end - $start)
    $r0w = if ($diff -le [single]0.001) { [single]0.001 } else { $diff }
    $r1x = [single](1.0 - $d7); $r1y = [single]0.0; $r1z = [single]1.0; $r1w = [single]0.0
    $r2x = if ($disp -eq 0) { [single]0.0 } else { [single](1.0 * $d11) }
    $words = @($r0x, $r0y, $start, $r0w, $r1x, $r1y, $r1z, $r1w, $r2x, $d10)
    $script:pnList += ,@($key, $origin, $words)
}
$script:pnList = @()
$zoneTsv = Join-Path $Root 'evidence/I-0068/queries/water-ready/zone-water-tess.tsv'
foreach ($l in [IO.File]::ReadLines($zoneTsv)) {
    $c = $l -split "`t"; if ($c[0] -eq 'zone' -or $c[11] -eq '' -or $c[11] -eq '0') { continue }
    PnBlock "$($c[0]):$($c[1]):$($c[2])" $c[2] ([uint32]$c[10]) ([uint32]$c[11]) (F32 $c[12]) (F32 $c[13]) (F32 $c[16]) (F32 $c[14]) (F32 $c[15]) (F32 $c[17]) (F32 $c[18]) $false
}
$chrTsv = Join-Path $Root 'evidence/I-0068/queries/npc-families/all-chr-gx.tsv'
foreach ($l in [IO.File]::ReadLines($chrTsv)) {
    $c = $l -split "`t"; if ($c[0] -eq 'chr') { continue }
    $m = [regex]::Match($c[5], 'GX04\(([^)]*)\)'); if (-not $m.Success) { continue }
    $w = $m.Groups[1].Value -split ','
    function Val([string]$s) { if ($s -like '0x*') { [single]0.0 } else { F32 $s } }
    $modeV = if ($w[1] -like '0x*') { [Convert]::ToUInt32($w[1].Substring(2), 16) } else { [uint32]$w[1] }
    $dispV = if ($w[0] -like '0x*') { [Convert]::ToUInt32($w[0].Substring(2), 16) -band 1 } else { ([uint32](F32 $w[0])) -band 1 }
    PnBlock "$($c[0]):$($c[1])" $c[1] $dispV $modeV (Val $w[2]) (Val $w[3]) (Val $w[4]) (Val $w[5]) (Val $w[6]) (Val $w[7]) (Val $w[8]) $false
}
foreach ($e in $script:pnList) {
    $names = @('row0.x min(edge,64)', 'row0.y min(inside,64)', 'row0.z fade start', 'row0.w max(end-start,0.001)', 'row1.x 1-d7', 'row1.y min(+0x264,0.99)=0', 'row1.z 1.0', 'row1.w +0x268=0', 'row2.x disp*d11', 'row2.y d10')
    for ($k = 0; $k -lt 10; $k++) { $rows.Add("cbPNTriangles`t$($e[0]) [$($e[1])]`t$($names[$k])`t$(Hex (U32 ([single]$e[2][$k])))`t") }
}
# --- defaults and neutral map
$rows.Add("default`tt54 no reflection object`tARGB`tFF000000`ttable entry 1 of FUN_140006C00 (black, opaque)")
$rows.Add("default`tt57 interaction option off`tARGB`tFF808080`ttable entry 2 (grey); 0x80/255 = 0.50196")
function ToHalf([single]$f) {
    $b = [BitConverter]::ToUInt32([BitConverter]::GetBytes($f), 0); $s = ($b -shr 16) -band 0x8000; $e = (($b -shr 23) -band 0xFF) - 127 + 15; $m = $b -band 0x7FFFFF
    if ($e -le 0) { return [uint16]$s }
    if ($e -ge 31) { return [uint16]($s -bor 0x7C00) }
    $h = $s -bor ($e -shl 10) -bor ($m -shr 13); if (($m -band 0x1000) -ne 0) { $h++ }; [uint16]$h
}
$rows.Add("default`tinteraction map clear`tR16G16_FLOAT x=y`t$('{0:X4}' -f (ToHalf ([single](128.0 / 255.0))))`t128/255 as half (the clear value (0.502, 0.502) of the capture)")
# --- reflection matrix, row-vector convention
function Mirror([single]$nx, [single]$ny, [single]$nz, [single]$d) {
    $len = [single][math]::Sqrt([double]($nx * $nx + $ny * $ny + $nz * $nz)); $nx = $nx / $len; $ny = $ny / $len; $nz = $nz / $len; $d = $d / $len
    $s = @([single](-2 * $nx), [single](-2 * $ny), [single](-2 * $nz), [single]0)
    $n = @($nx, $ny, $nz, $d)
    $m = @(); for ($r = 0; $r -lt 4; $r++) { $row = @(0, 0, 0, 0); $row[$r] = 1; for ($c = 0; $c -lt 4; $c++) { $row[$c] = [single]($row[$c] + $n[$r] * $s[$c]) }; $m += , $row }
    $m
}
foreach ($p in @(@('m37 water plane y=-241.76', 0, 1, 0, 241.76), @('m31 water plane y=-155', 0, 1, 0, 155), @('tilted (0.6,0.8,0)', 0.6, 0.8, 0, 10))) {
    $m = Mirror ([single]$p[1]) ([single]$p[2]) ([single]$p[3]) ([single]$p[4])
    for ($r = 0; $r -lt 4; $r++) { for ($c = 0; $c -lt 4; $c++) { $rows.Add("mirror`t$($p[0])`trow$r.col$c`t$(Hex (U32 ([single]$m[$r][$c])))`t") } }
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
[IO.File]::WriteAllLines($Out, $rows)
Write-Output "ok $($rows.Count - 1) rows -> $Out"
