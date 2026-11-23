[CmdletBinding(PositionalBinding = $false)]
param(
    [string]$Dumps = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'evidence/I-0068/queries/yebis/zone-gparam'),
    [string]$Out = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'evidence/I-0068/queries/ffx-ready/bank-blocks.tsv'),
    [int]$Id = 0,
    [float]$Factor = 1.0   # [[GXRS+0x5368]+0x50]; 1.0 when the factor object is absent (E4700)
)
# I-0068 part BM: the two YEBIS bank blocks that FFX-WAVE1-READY adds to the feed of YEBIS-WAVE1-READY, built from the decoded
# gparam dumps (Get-GparamValues output) and the record layouts read in the executable:
#   LightShaft  = GXBS::GXLightShaftFilterParam record words (28), engine order (FUN_14021E2D0 reflection offsets, ctor FUN_14021D8D0
#                 defaults for the two record-only words 9 and 10), converted by FUN_1400E4700;
#   ColorGrading = block at record+8 (FUN_1400F8CE0 / FUN_14021C780: word 0 Enable, word 1 LutSourceId), converted by the
#                 blend FUN_14021C150 (weight 0 branch: both keys the same record) into the globals 144962228..238.
# Nothing is written to Git: the golden table is an evidence file. Rules: a group is the rows of the dump with that group name;
# the value of an entry is the hex word(s) of its row at the given value id.
$ErrorActionPreference = 'Stop'
function Read-Group([string]$path, [string]$group, [int]$id) {
    $entries = New-Object System.Collections.Generic.List[object]
    foreach ($line in [IO.File]::ReadLines($path)) {
        if ($line.StartsWith('#') -or $line.StartsWith('group')) { continue }
        $c = $line -split "`t"
        if ($c.Count -lt 6 -or $c[0] -ne $group -or [int]$c[3] -ne $id) { continue }
        $w = @(); foreach ($h in ($c[5] -split ',')) { $w += [Convert]::ToUInt32($h, 16) }
        $entries.Add([pscustomobject]@{ Name = $c[1]; Words = $w })
    }
    , $entries
}
function F32([uint32]$w) { [BitConverter]::ToSingle([BitConverter]::GetBytes($w), 0) }
function U32([float]$f) { [BitConverter]::ToUInt32([BitConverter]::GetBytes([single]$f), 0) }
$rows = New-Object System.Collections.Generic.List[string]
$rows.Add("file`tid`tblock`tword`tname`thex")
$dflt = @{ 9 = [uint32]0; 10 = [uint32]0x3F7FFF58 }   # record-only words from the ctor FUN_14021D8D0 (+0x34 = 0, +0x38 = 0x3F7FFF58)
foreach ($file in (Get-ChildItem -LiteralPath $Dumps -Filter '*.tsv' | Sort-Object Name)) {
    $stem = $file.BaseName
    $ls = Read-Group $file.FullName 'LightShaft' $Id
    if ($ls.Count -eq 15) {
        $engine = New-Object 'object[]' 28
        for ($i = 0; $i -lt 28; $i++) { $engine[$i] = [pscustomobject]@{ Name = '(pad)'; Word = [uint32]0 } }
        $map = @{ 0 = 0; 2 = 8; 3 = 11; 4 = 12; 5 = 13; 6 = 14; 7 = 15; 8 = 16; 9 = 17; 10 = 18; 11 = 19; 12 = 20; 13 = 21 }   # file entry -> engine word (scalars)
        $engine[0] = [pscustomobject]@{ Name = $ls[0].Name; Word = $ls[0].Words[0] }
        for ($k = 0; $k -lt 4; $k++) { $engine[4 + $k] = [pscustomobject]@{ Name = "ColorScale[$k]"; Word = $ls[1].Words[$k] }; $engine[24 + $k] = [pscustomobject]@{ Name = "RingOuterColor[$k]"; Word = $ls[14].Words[$k] } }
        foreach ($e in 2..13) { $engine[$map[$e]] = [pscustomobject]@{ Name = $ls[$e].Name; Word = $ls[$e].Words[0] } }
        $engine[9] = [pscustomobject]@{ Name = '(record-only, ctor default)'; Word = $dflt[9] }
        $engine[10] = [pscustomobject]@{ Name = '(record-only, ctor default)'; Word = $dflt[10] }
        for ($j = 0; $j -lt 28; $j++) { $rows.Add("$stem`t$Id`tLightShaft28`t$j`t$($engine[$j].Name)`t{0:X8}" -f $engine[$j].Word) }
        # E4700 outputs: factor applied to words 7 and 27, word 0 zeroed when the factor <= 0
        $fa = $Factor
        $g = [ordered]@{
            '1449620A0 Enable' = $(if ($fa -le 0) { [uint32]0 } else { $engine[0].Word })
            '1449620A4' = $engine[4].Word; '1449620A8' = $engine[5].Word; '1449620AC' = $engine[6].Word
            '1449620B0 (w7*factor)' = U32 ((F32 $engine[7].Word) * $fa)
            '1449620B4 ShaftLength' = $engine[8].Word; '1449620B8 GlareRatio' = $engine[11].Word
            '1449620BC (w10)' = $engine[10].Word; '1449620C0 (w9)' = $engine[9].Word
            '1449620C4 AngleAttenuation' = $engine[12].Word; '1449620C8 NoiseMask' = $engine[13].Word; '1449620CC NoiseFrequency' = $engine[14].Word
            '1449620D0 Rad-X' = $engine[15].Word; '1449620D4 Rad-Y' = $engine[16].Word; '1449621BC MaskDepthThereshold' = $engine[17].Word
            '1449620D8 DiffractionRing' = $engine[18].Word; '1449620DC DiffractionRingRadius' = $engine[19].Word
            '1449620E0 DiffractionRingAttenuation' = $engine[20].Word; '1449620E4 DiffractionRingSpectrumOrder' = $engine[21].Word
            '1449620E8 (w24)' = $engine[24].Word; '1449620EC (w25)' = $engine[25].Word; '1449620F0 (w26)' = $engine[26].Word
            '1449620F4 (w27*factor)' = U32 ((F32 $engine[27].Word) * $fa)
        }
        foreach ($k in $g.Keys) { $rows.Add("$stem`t$Id`tE4700-globals`t-`t$k`t{0:X8}" -f [uint32]$g[$k]) }
    }
    $cg = Read-Group $file.FullName 'ColorGrading' $Id
    if ($cg.Count -eq 2) {
        $enable = $cg[0].Words[0]; $lut = $cg[1].Words[0]
        # weight 0, both keys the same record, no fade in progress (record word 4 = 0): FUN_14021C150 first branch
        $o = [ordered]@{ '144962228 (word0: LUT copy trigger = Enable)' = $enable; '14496222C (word1)' = $lut; '144962230 (word2)' = $lut; '144962234 (word3)' = $lut; '144962238 (word4: weight)' = [uint32]0 }
        foreach ($k in $o.Keys) { $rows.Add("$stem`t$Id`tGrading-globals`t-`t$k`t{0:X8}" -f [uint32]$o[$k]) }
    }
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
[IO.File]::WriteAllLines($Out, $rows)
Write-Output "ok $($rows.Count - 1) rows -> $Out"
