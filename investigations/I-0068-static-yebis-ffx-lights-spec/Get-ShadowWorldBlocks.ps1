[CmdletBinding(PositionalBinding = $false)]
param(
    [string]$Gparam = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'evidence/I-0068/queries/gparam'),
    [string]$Out = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'evidence/I-0068/queries/shadow-ready/shadow-world-blocks.tsv'),
    [string]$Edges = '2048,4096,8192'    # atlas edges to evaluate (the atlas edge is the "shadow-map size" of UseShadowSetup)
)
# I-0068 part BN: the world ShadowParam record R (0x94 bytes, layout of Banks::Shadow, native/world/light_producers.cpp) from the decoded
# gparam dumps, and the words FUN_1401B5720 / ShadowFieldsFromBanks derive from it for a given atlas edge: the clamped splits (state
# +0xA70..+0xA84), cbSceneParam shadow block words +0x1C0 (FC_ShadowMapParam), +0x1D0 (FC_ShadowColor), +0x230, +0x240, +0x250.
# Constants read in the executable: eps 0.1 (143D7ED70), tiny 1e-6 (143D7ECF0), half 0.5, one 1.0, k = (1,1,1,1) (143D7FEF0), 255.0 (143D7EFF4).
# Zone files carry no ShadowParam group, so every world zone takes mapbase id 0 (Banks::Shadow falls back to mapbase when the map's group is empty).
$ErrorActionPreference = 'Stop'
function Group-Rows([string]$path, [int]$id) {
    $rows = New-Object System.Collections.Generic.List[object]
    foreach ($line in [IO.File]::ReadLines($path)) {
        if ($line.StartsWith('#') -or $line.StartsWith('group')) { continue }
        $c = $line -split "`t"; if ($c.Count -lt 6 -or $c[0] -ne 'ShadowParam' -or [int]$c[3] -ne $id) { continue }
        $w = @(); foreach ($h in ($c[5] -split ',')) { $w += [Convert]::ToUInt32($h, 16) }
        $rows.Add([pscustomobject]@{ Name = $c[1]; Words = $w })
    }
    , $rows
}
function F32([uint32]$w) { [BitConverter]::ToSingle([BitConverter]::GetBytes($w), 0) }
function U32([single]$f) { [BitConverter]::ToUInt32([BitConverter]::GetBytes($f), 0) }
function Max32([single]$a, [single]$b) { if ($a -lt $b) { $b } else { $a } }   # (a < b) ? b : a, as the executable's maxss chain
$map = @(@(0x00,2,0),@(0x24,1,0),@(0x34,1,0),@(0x40,1,0),@(0x44,1,0),@(0x48,1,0),@(0x38,1,0),@(0x3C,1,0),@(0x50,1,0),@(0x54,1,0),@(0x64,1,0),@(0x6C,1,0),
        @(0x70,1,1),@(0x71,1,1),@(0x74,1,0),@(0x78,1,0),@(0x7C,1,0),@(0x80,1,0),@(0x84,1,0),@(0x88,1,0),@(0x8C,1,0),@(0x90,1,0))
$rows = New-Object System.Collections.Generic.List[string]
$rows.Add("source`tid`tedge`tfield`tname`thex")
foreach ($src in @(@('mapbase', 'gamedata__param_drawparam_mapbase.gparam.tsv', 0), @('default', 'gamedata__param_drawparam_default.gparam.tsv', 0), @('default', 'gamedata__param_drawparam_default.gparam.tsv', 10),
                   @('default', 'gamedata__param_drawparam_default.gparam.tsv', 11), @('default', 'gamedata__param_drawparam_default.gparam.tsv', 12), @('default', 'gamedata__param_drawparam_default.gparam.tsv', 13),
                   @('default', 'gamedata__param_drawparam_default.gparam.tsv', 14), @('default', 'gamedata__param_drawparam_default.gparam.tsv', 15))) {
    $g = Group-Rows (Join-Path $Gparam $src[1]) $src[2]
    if ($g.Count -ne 22) { throw "$($src[0]) id $($src[2]): $($g.Count) rows" }
    $R = New-Object 'uint32[]' 37   # 0x94 / 4 words
    $R[0x34/4] = U32 0.1; $R[0x38/4] = U32 1000.0; $R[0x3C/4] = U32 10.0; $R[0x40/4] = U32 10.0; $R[0x44/4] = U32 30.0; $R[0x48/4] = U32 70.0; $R[0x68/4] = [Convert]::ToUInt32("3BA3D70A",16)
    $R[0x5C/4] = U32 0.7; $R[0x60/4] = U32 1.0; $R[0x6C/4] = U32 3.0; $R[0x74/4] = U32 1.0; $R[0x78/4] = U32 5.0; $R[0x7C/4] = U32 1.5; $R[0x80/4] = [Convert]::ToUInt32("BDAE147B",16); $R[0x88/4] = U32 2.0; $R[0x8C/4] = [Convert]::ToUInt32("FF000000",16)
    for ($k = 0; $k -lt 22; $k++) {
        $at = $map[$k][0]; $n = $map[$k][1]; $byte = $map[$k][2]
        for ($c = 0; $c -lt $n; $c++) {
            $v = $g[$k].Words[$c]
            if ($byte) { $i = [int][math]::Floor($at / 4); $sh = 8 * ($at % 4); $mask = [Convert]::ToUInt32(("{0:X8}" -f ([uint64]0xFF -shl $sh)),16); $R[$i] = [uint32](($R[$i] -band (-bnot $mask) -band 0xFFFFFFFF) -bor (($v -band 0xFF) -shl $sh)) } else { $R[[int]($at / 4) + $c] = $v }
        }
    }
    for ($w = 0; $w -lt 37; $w++) { $rows.Add("$($src[0])`t$($src[2])`t-`tR+0x$('{0:X2}' -f (4 * $w))`t$(if ($w -eq 0) { 'Angle x' } else { '' })`t$('{0:X8}' -f $R[$w])") }
    foreach ($edge in ($Edges -split ',')) {
        $S = [single][int]$edge
        $near = F32 $R[0x34/4]; $c0 = F32 $R[0x40/4]; $c1 = F32 $R[0x44/4]; $c2 = F32 $R[0x48/4]; $fs = F32 $R[0x38/4]; $fd = F32 $R[0x3C/4]
        $eps = [single]0.1; $tiny = [single]1e-6
        $a7c = Max32 ($near + $eps) $c0; $a80 = Max32 ($c0 + $eps) $c1; $a84 = Max32 ($c1 + $eps) $c2; $a74 = Max32 ($c2 + $eps) $fs; $a78 = $fd; $fadeEnd = $a74 + $a78
        $d = [ordered]@{
            'state+0xA70 near' = U32 $near; 'state+0xA7C split0' = U32 $a7c; 'state+0xA80 split1' = U32 $a80; 'state+0xA84 split2' = U32 $a84; 'state+0xA74 fadeStart' = U32 $a74; 'state+0xA78 fadeDist' = U32 $a78
            'block+0x1C0.x FC_ShadowMapParam.x (fadeEnd)' = U32 $fadeEnd; 'block+0x1C0.y 1/max(fadeDist,tiny)' = U32 ([single]1.0 / (Max32 $tiny $a78))
            'block+0x1C0.z 0.5/edge' = U32 ([single]0.5 / $S); 'block+0x1C0.w 0.5/edge' = U32 ([single]0.5 / $S)
            'block+0x240.x 1/edge' = U32 ([single]1.0 / $S); 'block+0x240.y 1/edge' = U32 ([single]1.0 / $S); 'block+0x240.z R+0x80' = $R[0x80/4]
        }
        $d0 = $a7c - $near; $den = @(($a80 - $a7c), ($a84 - $a80), ($fadeEnd - $a84)); for ($i = 0; $i -lt 3; $i++) { if (-not ($den[$i] -gt 0)) { $den[$i] = $d0 + 1.0 } }
        $d['block+0x250.x one'] = U32 1.0; $d['block+0x250.y d0/den0'] = U32 ($d0 / $den[0]); $d['block+0x250.z d0/den1'] = U32 ($d0 / $den[1]); $d['block+0x250.w d0/den2'] = U32 ($d0 / $den[2])
        $col = $R[0x8C/4]; $b0 = [single]($col -band 0xFF); $b1 = [single](($col -shr 8) -band 0xFF); $b2 = [single](($col -shr 16) -band 0xFF); $b3 = [single](($col -shr 24) -band 0xFF)
        $cr = $b2 / 255.0; $cg = $b1 / 255.0; $cb = $b0 / 255.0; $ca = $b3 / 255.0
        $d['block+0x1D0.x ca*(1-cr)'] = U32 ([single]($ca * (1.0 - $cr))); $d['block+0x1D0.y ca*(1-cg)'] = U32 ([single]($ca * (1.0 - $cg))); $d['block+0x1D0.z ca*(1-cb)'] = U32 ([single]($ca * (1.0 - $cb)))
        $d['block+0x1D0.w R+0x68'] = $R[0x68/4]
        $d['block+0x230.x DepthRate R+0x74'] = $R[0x74/4]; $d['block+0x230.y DepthScale R+0x78'] = $R[0x78/4]; $d['block+0x230.z KernelScale R+0x7C'] = $R[0x7C/4]
        foreach ($k in $d.Keys) { $rows.Add("$($src[0])`t$($src[2])`t$edge`tderived`t$k`t$('{0:X8}' -f [uint32]$d[$k])") }
    }
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
[IO.File]::WriteAllLines($Out, $rows)
Write-Output "ok $($rows.Count - 1) rows -> $Out"

