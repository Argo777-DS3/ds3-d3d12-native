[CmdletBinding(PositionalBinding = $false)]
param(
    [Parameter(Mandatory = $true)][string]$Bundles,   # directory with the recovered frpg_sfxbnd_*_effect.ffxbnd.dcx files (Recover-GameData.ps1 output, outside the repository)
    [string]$Ids = '800001,800080,800081,800082,800083,31001,845010,850800,850801,850802',   # the FXR ids of the WindSFX regions (msb-wind-regions-all-maps.tsv)
    [string]$Out = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'evidence/I-0068/queries/ffx-ready/zone-force-fields.tsv'),
    [string]$SchemaDir = 'C:\Users\User\.codex\I0033-tools\fxr-32.1.0\package\dist'
)
# I-0068 part BM: for each WindSFX effect id, the bundles that carry it and the force-field actions inside (types 10200 CancelForce,
# 10300 WindForce, 10301 GravityForce, 10302 ForceCollision, 10303 TurbulenceForce of the @cccode/fxr schema) with their field list,
# plus the other action types of the effect (counts), read from the retained game data. Static, read-only; the bundles stay outside
# the repository. The ids come as one string because `pwsh -File` passes a comma list as a single argument.
$ErrorActionPreference = 'Stop'
Add-Type -Path 'C:\Users\User\.codex\tools\SoulsFormats-1.0.1\lib\net462\SoulsFormats.dll'
if (-not ('FxrDump' -as [type])) { Add-Type -Path (Join-Path $PSScriptRoot 'FxrDump.cs') }
$wanted = @($Ids -split ',' | ForEach-Object { 'f' + ([int]$_).ToString('D9') + '.fxr' })
$a = Get-Content (Join-Path $SchemaDir 'actions.json') -Raw | ConvertFrom-Json; $by = @{}; foreach ($x in $a) { $by[[int]$x.type] = $x.name }
$rows = New-Object System.Collections.Generic.List[string]
$rows.Add("fxr`tbundle`tbytes`tactions`tforce_field`tfields")
foreach ($bundle in (Get-ChildItem -LiteralPath $Bundles -Filter '*effect.ffxbnd.dcx' | Sort-Object Name)) {
    $d = [SoulsFormats.DCX]::Decompress([IO.File]::ReadAllBytes($bundle.FullName))
    $b = [SoulsFormats.BND4]::Read($d)
    foreach ($f in $b.Files) {
        $name = [IO.Path]::GetFileName($f.Name)
        if ($wanted -notcontains $name) { continue }
        $tmp = Join-Path ([IO.Path]::GetTempPath()) $name
        [IO.File]::WriteAllBytes($tmp, $f.Bytes)
        $text = (@(& (Join-Path $PSScriptRoot 'Read-Fxr.ps1') -Fxr $tmp -SchemaDir $SchemaDir)) -join "`n"
        $dump = @($text -split "`r?`n")
        Remove-Item -LiteralPath $tmp
        $counts = @{}; $force = New-Object System.Collections.Generic.List[string]
        for ($i = 0; $i -lt $dump.Count; $i++) {
            if ($dump[$i] -match '^\s*(\d+) ([A-Za-z0-9_]+)\s*$') {
                $t = [int]$Matches[1]; $nm = $Matches[2]
                $counts[$t] = 1 + [int]$counts[$t]
                if ($t -in 10200, 10300, 10301, 10302, 10303) { $force.Add("$nm($t): " + ($dump[$i + 1].Trim())) }
            }
        }
        $summary = ($counts.GetEnumerator() | Sort-Object Name | ForEach-Object { "$($by[[int]$_.Name])x$($_.Value)" }) -join ' '
        $rows.Add("$name`t$($bundle.Name)`t$($f.Bytes.Length)`t$summary`t$($force.Count)`t$($force -join ' | ')")
    }
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
[IO.File]::WriteAllLines($Out, $rows)
Write-Output "ok $($rows.Count - 1) rows -> $Out"

