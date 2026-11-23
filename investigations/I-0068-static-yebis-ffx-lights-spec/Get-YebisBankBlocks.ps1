[CmdletBinding(PositionalBinding = $false)]
param(
    [string]$Dumps = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'evidence/I-0068/queries/yebis/zone-gparam'),
    [string]$Out = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'evidence/I-0068/queries/yebis-ready/bank-blocks.tsv'),
    [int]$Id = 0
)
# I-0068 part BL: the three bank blocks the YEBIS converters read (FUN_1400E42A0 Tone Map float[35], FUN_1400E4460 Bloom
# float[42] in the engine's order, FUN_1400E4070 Setting Info int[21]), built from the decoded gparam dumps (Get-GparamValues
# output, one TSV per file: group, param, type, id, floats, hex). Nothing is written to Git: the golden table is an evidence file.
# Rules (YEBIS-WAVE1-READY section 4): an entry is a maximal run of rows with the same parameter name and strictly increasing ids;
# the value of an entry at an id is the hex word(s) of that row; float4 entries give four words, other types one.
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
function Entry-Words($entry, [int]$id) {
    if ($entry.Values.ContainsKey($id)) { return $entry.Values[$id] }
    return $null
}
$rows = New-Object System.Collections.Generic.List[string]
$rows.Add("file`tid`tblock`tword`tname`thex")
foreach ($file in (Get-ChildItem -LiteralPath $Dumps -Filter '*.tsv' | Sort-Object Name)) {
    $g = Read-Groups $file.FullName $Id
    $stem = $file.BaseName
    # Tone Map: the first 35 entries (the YEBIS half) in file order, one word each
    if ($g.Contains('Tone Map')) {
        $e = $g['Tone Map']
        for ($i = 0; $i -lt 35 -and $i -lt $e.Count; $i++) {
            $w = Entry-Words $e[$i] $Id; if ($null -eq $w) { continue }
            $rows.Add("$stem`t$Id`tToneMap35`t$i`t$($e[$i].Name)`t{0:X8}" -f $w[0])
        }
    }
    # Bloom: the first 36 entries expand to 42 file words; engine words 0-3, 5-41 are a permutation of file words 0-40, word 4 is a record-only default 1.0 and file word 41 (the second Enable) is record word 42, outside the 42-word block (YEBIS-WAVE1-READY 1.2)
    if ($g.Contains('Bloom')) {
        $e = $g['Bloom']; $file42 = New-Object System.Collections.Generic.List[object]
        for ($i = 0; $i -lt 36 -and $i -lt $e.Count; $i++) {
            $w = Entry-Words $e[$i] $Id; if ($null -eq $w) { $w = @(0) }
            for ($k = 0; $k -lt $w.Count; $k++) { $file42.Add([pscustomobject]@{ Name = $(if ($w.Count -gt 1) { "$($e[$i].Name)[$k]" } else { $e[$i].Name }); Word = $w[$k] }) }
        }
        if ($file42.Count -eq 42) {
            $engine = New-Object 'object[]' 42
            for ($j = 0; $j -lt 4; $j++) { $engine[$j] = $file42[$j] }
            $engine[4] = [pscustomobject]@{ Name = '(record-only default)'; Word = [uint32]0x3F800000 }   # not a file value: 1402147D0 default 1.0; the second Enable is record word 42
            for ($j = 4; $j -lt 12; $j++) { $engine[$j + 1] = $file42[$j] }
            $engine[13] = $file42[30]; $engine[14] = $file42[31]; $engine[15] = $file42[32]
            for ($j = 33; $j -lt 41; $j++) { $engine[$j - 17] = $file42[$j] }
            for ($j = 12; $j -lt 30; $j++) { $engine[$j + 12] = $file42[$j] }
            for ($j = 0; $j -lt 42; $j++) { $rows.Add("$stem`t$Id`tBloom42`t$j`t$($engine[$j].Name)`t{0:X8}" -f $engine[$j].Word) }
        }
    }
    # Setting Info: the type-3 entries after BG Color in file order are words 0..17; word 20 is SceneLuminanceScale
    if ($g.Contains('Setting Info')) {
        $e = $g['Setting Info']; $idx = 0
        foreach ($en in $e) {
            if ($en.Type -eq 3) { $w = Entry-Words $en $Id; if ($null -ne $w -and $idx -lt 18) { $rows.Add("$stem`t$Id`tSetting21`t$idx`t$($en.Name)`t{0:X8}" -f $w[0]) }; $idx++ }
            elseif ($en.Name -eq 'SceneLuminanceScale') { $w = Entry-Words $en $Id; if ($null -ne $w) { $rows.Add("$stem`t$Id`tSetting21`t20`t$($en.Name)`t{0:X8}" -f $w[0]) } }
        }
    }
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
[IO.File]::WriteAllLines($Out, $rows)
Write-Output "ok $($rows.Count - 1) rows -> $Out"
