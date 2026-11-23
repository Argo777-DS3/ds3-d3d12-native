[CmdletBinding(PositionalBinding = $false)]
param(
    [string]$Dumps = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'evidence/I-0068/queries/yebis/zone-gparam'),
    [string]$AllDumps = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'evidence/I-0068/queries/gparam'),
    [string]$Out = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'evidence/I-0068/queries/yebis-second/second-blocks.tsv'),
    [int]$Id = 0
)
# I-0068 part BP: the second-tranche YEBIS bank blocks, from the decoded gparam dumps (Get-GparamValues output: group, param, type, id,
# floats, hex). Blocks (record words in the engine's order; the file order is the entry order of the group):
#   Dof34       FUN_14021E720 / FUN_1400E4890 record, 34 words: word k = file entry k for k = 0..11, word 12 = file entry 13 (the bool Enable),
#               words 13..32 = file entries 14..33, word 33 = file entry 12 (BackgroundMaskThreshold). 34 file entries, one word each.
#   MotionBlur21 FUN_1401D9910 / FUN_1400E4B50 record, 21 words: w0 Enable (file 0), w1 mode (NOT in the file: constructor default 2),
#               w2 BlurTimeRatio (1), w3 MaxBlurLength (3), w4 NumBaseSamples (2), w5 MaxRecurrences (5), w6 SampleInterleaved (6),
#               w7 CameraRotationPerspectiveThreshold (7), w8 CameraTranslationThreshold (8), w9 (NOT in the file: default 0),
#               w10 FalloffDistanceMin (9), w11 FalloffDistanceMax (10), w12 MotionBlurScale (4), w13 Bidirectional (11),
#               w14.. = file 12..20 (the GX half: Enable byte, Motion/Object/Camera Blur Intensity, ReccurenceCount, three flag bytes, Base Fps).
#   Feedback8   FUN_140221640 record: w0 Enable then the six floats in file order (the order beyond Enable is OPEN, YEBIS-SECOND-READY).
#   ToneAuto    the AutoExposure words of the Tone Map group (file entries 9, 13..20 by name) of every gparam file at id 0.
# Nothing is written to Git: the golden table is an evidence file.
$ErrorActionPreference = 'Stop'
function Read-Groups([string]$path, [int]$id) {
    $groups = [ordered]@{}
    $prev = $null; $prevId = -1
    foreach ($line in [IO.File]::ReadLines($path)) {
        if ($line.StartsWith('#') -or $line.StartsWith('group')) { continue }
        $c = $line -split "`t"
        if ($c.Count -lt 6) { continue }
        $g = $c[0]; $name = $c[1]; $type = [int]$c[2]; $rid = [int]$c[3]
        if (-not $groups.Contains($g)) { $groups[$g] = New-Object System.Collections.Generic.List[object] }
        $list = $groups[$g]
        $newEntry = ($list.Count -eq 0) -or ($prev -ne "$g|$name") -or ($rid -le $prevId)
        if ($newEntry) { $list.Add([pscustomobject]@{ Name = $name; Type = $type; Values = @{} }) }
        $words = @(); foreach ($h in ($c[5] -split ',')) { $words += [Convert]::ToUInt32($h, 16) }
        $list[$list.Count - 1].Values[$rid] = $words
        $prev = "$g|$name"; $prevId = $rid
    }
    $groups
}
function W($entry, [int]$id) { if ($entry.Values.ContainsKey($id)) { $entry.Values[$id][0] } else { $null } }
$rows = New-Object System.Collections.Generic.List[string]
$rows.Add("file`tid`tblock`tword`tname`thex")
function Emit([string]$stem, [string]$block, [int]$word, [string]$name, $v) { if ($null -ne $v) { $rows.Add(("{0}`t{1}`t{2}`t{3}`t{4}`t{5:X8}" -f $stem, $Id, $block, $word, $name, [uint32]$v)) } }
$files = @(Get-ChildItem -LiteralPath $Dumps -Filter '*.tsv' | Sort-Object Name)
$extra = @(
    @{ Stem = 'mapbase'; Path = (Join-Path $AllDumps 'gamedata__param_drawparam_mapbase.gparam.tsv') },
    @{ Stem = 'remobase'; Path = (Join-Path $AllDumps 'gamedata-base__param_drawparam_remobase.gparam.tsv') })
$defaultGroups = Read-Groups (Join-Path $AllDumps 'gamedata__param_drawparam_mapbase.gparam.tsv') $Id   # Banks::GroupFloat: an area file with an empty group takes the mapbase group (light_producers.cpp), the shape of the original lookup (LIVE-REDUCTION-AM 5)
$sources = @($files | ForEach-Object { @{ Stem = $_.BaseName; Path = $_.FullName; Fallback = $true } }) + @($extra | ForEach-Object { @{ Stem = $_.Stem; Path = $_.Path; Fallback = $false } })
foreach ($src in $sources) {
    $g = Read-Groups $src.Path $Id; $stem = $src.Stem
    # The zone files carry no Dof and no MotionBlur group: the stand-in is the mapbase file, id 0 (native Banks::GroupFloat; the original key may also resolve to default id 0: the default block is emitted as the file default)
    if ($src.Fallback) { foreach ($name in @('Dof', 'MotionBlur', 'FeedBackBlur')) { if (-not $g.Contains($name) -and $defaultGroups.Contains($name)) { $g[$name] = $defaultGroups[$name]; $rows.Add("$stem	$Id	fallback	0	$name	mapbase") } } }
    if ($g.Contains('Dof')) {
        $e = $g['Dof']; if ($e.Count -ne 34) { throw "$stem Dof entries $($e.Count)" }
        $map = @(0..11) + @(13) + @(14..33) + @(12)
        for ($k = 0; $k -lt 34; $k++) { Emit $stem 'Dof34' $k $e[$map[$k]].Name (W $e[$map[$k]] $Id) }
    }
    if ($g.Contains('MotionBlur')) {
        $e = $g['MotionBlur']; if ($e.Count -ne 21) { throw "$stem MotionBlur entries $($e.Count)" }
        $map = @(0, -1, 1, 3, 2, 5, 6, 7, 8, -2, 9, 10, 4, 11) + @(12..16)
        for ($k = 0; $k -lt 19; $k++) {
            if ($map[$k] -eq -1) { Emit $stem 'MotionBlur21' $k '(not serialized) mode' ([uint32]2) }
            elseif ($map[$k] -eq -2) { Emit $stem 'MotionBlur21' $k '(not serialized)' ([uint32]0) }
            else { Emit $stem 'MotionBlur21' $k $e[$map[$k]].Name (W $e[$map[$k]] $Id) }
        }
        $b0 = (W $e[17] $Id); $b1 = (W $e[18] $Id); $b2 = (W $e[19] $Id)
        if ($null -ne $b0) { Emit $stem 'MotionBlur21' 19 'UseInnerSimulationTime,EnableAverage,EnableFixed (bytes 0x4C..0x4E)' (($b0 -band 0xFF) -bor (($b1 -band 0xFF) -shl 8) -bor (($b2 -band 0xFF) -shl 16)) }
        Emit $stem 'MotionBlur21' 20 $e[20].Name (W $e[20] $Id)
    }
    if ($g.Contains('FeedBackBlur')) {
        $e = $g['FeedBackBlur']
        for ($k = 0; $k -lt $e.Count; $k++) { Emit $stem 'Feedback8' $k $e[$k].Name (W $e[$k] $Id) }
    }
}
foreach ($file in (Get-ChildItem -LiteralPath $AllDumps -Filter '*.tsv' | Sort-Object Name)) {
    $g = Read-Groups $file.FullName $Id; $stem = $file.BaseName
    if ($g.Contains('Tone Map')) { foreach ($en in $g['Tone Map']) { if ($en.Name -match 'AutoExposure|^MinExposure$|^MaxExposure$|Metering') { Emit $stem 'ToneAuto' 0 $en.Name (W $en $Id) } } }
    if ($g.Contains('FeedBackBlur')) { foreach ($en in $g['FeedBackBlur']) { if ($en.Name -eq 'Enable') { Emit $stem 'FeedbackEnable' 0 $en.Name (W $en $Id) } } }
    if ($g.Contains('Dof')) { $e = $g['Dof']; if ($e.Count -ge 26) { Emit $stem 'DofFlags' 0 'Enable' (W $e[0] $Id); Emit $stem 'DofFlags' 13 'Enable (bool)' (W $e[13] $Id); Emit $stem 'DofFlags' 25 'AutoFocus Enable' (W $e[25] $Id) } }
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
[IO.File]::WriteAllLines($Out, $rows)
Write-Output "ok $($rows.Count - 1) rows -> $Out"
