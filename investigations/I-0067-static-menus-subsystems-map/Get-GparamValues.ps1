# Read-only: dumps every value of a DS3 GPARAM file (".gparam.dcx", game 3) with its value ID, as TSV
# (group, param, type, id, value words as float and hex). Same layout as the pilot's native/content/gparam.cpp;
# the DCX envelope is expanded with the pinned SoulsFormats.
# Usage: pwsh -NoProfile -File Get-GparamValues.ps1 -Path <file.gparam.dcx> -Out <tsv> [-Ids 0,11]
param([Parameter(Mandatory)][string]$Path, [Parameter(Mandatory)][string]$Out, [int[]]$Ids)
$ErrorActionPreference = 'Stop'
$souls = 'C:\Users\User\.codex\tools\SoulsFormats-1.0.1\lib\net462\SoulsFormats.dll'
if ((Get-FileHash -LiteralPath $souls -Algorithm SHA256).Hash -cne '19706BA6528982B83C6B6DD0DBE0ABDDF0E73CD51404F44A9D92CD27DEAF6735') { throw 'SoulsFormats mismatch' }
Add-Type -Path $souls
$raw = [IO.File]::ReadAllBytes((Resolve-Path $Path).Path)
$d = if ([Text.Encoding]::ASCII.GetString($raw, 0, 3) -eq 'DCX') { [byte[]][SoulsFormats.DCX]::Decompress($raw) } else { $raw }
function U32([int]$o) { [BitConverter]::ToUInt32($d, $o) }
function Utf16([int]$o) { $e = $o; while ([BitConverter]::ToUInt16($d, $e) -ne 0) { $e += 2 }; @([Text.Encoding]::Unicode.GetString($d, $o, $e - $o), ($e + 2)) }
$s = if ([Text.Encoding]::ASCII.GetString($d, 0, 8) -eq "f`0i`0l`0t`0") { 4 } else { 0 }
if ((U32 ($s + 4)) -ne 3) { throw 'not a DS3 GPARAM' }
$groupCount = U32 ($s + 12); $header = U32 ($s + 20); $groupHeaders = U32 ($s + 24); $pho0 = U32 ($s + 28)
$paramHeaders = U32 ($s + 32); $values = U32 ($s + 36); $valueIds = U32 ($s + 40)
$lines = @("# $(Split-Path $Path -Leaf) SHA-256 $((Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash)", "group`tparam`ttype`tid`tfloats`thex")
for ($gi = 0; $gi -lt $groupCount; $gi++) {
    $gh = $groupHeaders + (U32 ($header + 4 * $gi)); $pc = U32 $gh; $pho = U32 ($gh + 4)
    $n1 = Utf16 ($gh + 8); $n2 = Utf16 $n1[1]; $gname = $n2[0].Trim()
    for ($pi = 0; $pi -lt $pc; $pi++) {
        $ph = $paramHeaders + (U32 ($pho0 + $pho + 4 * $pi)); $vo = U32 $ph; $io = U32 ($ph + 4); $type = $d[$ph + 8]; $count = $d[$ph + 9]
        $p1 = Utf16 ($ph + 12); $p2 = Utf16 $p1[1]; $pname = $p2[0]
        $stride = switch ($type) { { $_ -in 1, 5, 0xB } { 1 } 2 { 2 } { $_ -in 0xC, 0xD, 0xE } { 16 } default { 4 } }
        $words = switch ($type) { 0xC { 2 } 0xD { 3 } 0xE { 4 } default { 1 } }
        for ($k = 0; $k -lt $count; $k++) {
            $id = [BitConverter]::ToInt32($d, $valueIds + $io + 4 * $k)
            if ($Ids -and ($Ids -notcontains $id)) { continue }
            $at = $values + $vo + $k * $stride
            if ($type -in 1, 5, 0xB) { $fl = "$($d[$at])"; $hx = '{0:X2}' -f $d[$at] }
            elseif ($type -eq 2) { $fl = "$([BitConverter]::ToInt16($d, $at))"; $hx = '{0:X4}' -f [BitConverter]::ToUInt16($d, $at) }
            else {
                $fs = @(); $hs = @(); for ($c = 0; $c -lt $words; $c++) { $hs += '{0:X8}' -f (U32 ($at + 4 * $c)); $fs += if ($type -in 3, 7, 0xF) { "$([BitConverter]::ToInt32($d, $at + 4 * $c))" } else { '{0:G6}' -f [BitConverter]::ToSingle($d, $at + 4 * $c) } }
                $fl = $fs -join ','; $hx = $hs -join ','
            }
            $lines += "$gname`t$pname`t$type`t$id`t$fl`t$hx"
        }
    }
}
$lines | Set-Content -LiteralPath $Out -Encoding utf8
"$($lines.Count - 2) values"
