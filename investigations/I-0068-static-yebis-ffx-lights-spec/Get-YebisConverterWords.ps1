[CmdletBinding(PositionalBinding = $false)]
param(
    [Parameter(Mandatory = $true)][string[]]$Decompile,    # decompiled C files of the converters (E42A0, E4460, E4070, E4700, E4890 ...)
    [string]$Globals = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'evidence/I-0068/queries/yebis/yebis-globals.tsv'),
    [string]$Out = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'evidence/I-0068/queries/yebis-ready/converter-words.tsv')
)
# I-0068 part BL: for each YEBIS converter (a function of the form `_DAT_<global> = param_N[<word>]`), the word of the bank block
# each global is fed from, with the registered name of the global (Get-YebisGlobals.ps1 output). Static text parse of the decompile;
# a word that feeds a global through arithmetic is listed with the whole right-hand side.
$ErrorActionPreference = 'Stop'
$Decompile = @($Decompile | ForEach-Object { $_ -split ',' })
$names = @{}
foreach ($l in [IO.File]::ReadLines($Globals)) { $c = $l -split "`t"; if ($c.Count -ge 2) { $names[$c[0].ToLower()] = $c[1] } }
$rows = New-Object System.Collections.Generic.List[string]
$rows.Add("function`tword`tglobal`tname`trhs")
foreach ($file in $Decompile) {
    $fn = [IO.Path]::GetFileNameWithoutExtension($file)
    foreach ($line in [IO.File]::ReadLines($file)) {
        if ($line -notmatch '^\s*_DAT_([0-9a-fA-F]{9,10})\s*=\s*(.*);\s*$') { continue }
        $g = $Matches[1].ToLower(); $rhs = $Matches[2]
        foreach ($m in [regex]::Matches($rhs, 'param_2\[(0x[0-9a-fA-F]+|\d+)\]|\*param_2\b')) {
            $w = if ($m.Value -eq '*param_2') { 0 } else { [Convert]::ToInt32($m.Groups[1].Value.Replace('0x', ''), $(if ($m.Groups[1].Value.StartsWith('0x')) { 16 } else { 10 })) }
            $n = if ($names.ContainsKey($g)) { $names[$g] } else { '' }
            $rows.Add("$fn`t$w`t$g`t$n`t$rhs")
        }
    }
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
[IO.File]::WriteAllLines($Out, $rows)
Write-Output "ok $($rows.Count - 1) rows -> $Out"
