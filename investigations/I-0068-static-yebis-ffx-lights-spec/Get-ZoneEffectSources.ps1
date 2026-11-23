[CmdletBinding()]
param(
    [string]$MsbDir = 'C:\Users\User\.codex\I0063-cache\gamedata\msb',
    [string[]]$Maps = @('m30_00', 'm31_00', 'm37_00', 'm40_00', 'm45_00', 'm50_00', 'm51_00'),
    [Parameter(Mandatory = $true)][string]$ModelSfxParam,   # Get-ParamRows.ps1 -Param ModelSfxParam output
    [Parameter(Mandatory = $true)][string]$ObjectParamSfx,  # objectparam-sfx.tsv (id, sfx0..3, b0..b3, w54, f58)
    [Parameter(Mandatory = $true)][string]$Out
)
# I-0068 part AN: static effect sources of a map, the witnesses of the effect entry door (EFFECTS-ENTRY-DOOR-NATIVE).
# For each map (MSB3 of the retained gamedata): object parts whose model has a ModelSfxParam row (ObjModelSfxSlot) or an
# ObjectParam sfx row (ObjMultiSfxSlot / ObjBurnSfxSlot), MSB SFX regions and wind SFX regions (FFXID), treasure events
# (shine effect), enemy parts. One TSV row per (map, source kind, key).
$ErrorActionPreference = 'Stop'
Add-Type -Path 'C:\Users\User\.codex\tools\SoulsFormats-1.0.1\lib\net462\SoulsFormats.dll'
# ModelSfxParam: rows of 128 bytes, entries {id, dummy ref, 0, 0}; the dump lists (id, offset, hex, float, int)
$msp = @{}
foreach ($l in Get-Content -LiteralPath $ModelSfxParam | Select-Object -Skip 2) {
    $c = $l -split "`t"; $id = [int]$c[0]; $off = [Convert]::ToInt32($c[1].Substring(2), 16)
    if (-not $msp.ContainsKey($id)) { $msp[$id] = @{} }; $msp[$id][$off] = [int]$c[4] }
function ModelEntries([int]$row) {
    if (-not $msp.ContainsKey($row)) { return @() }
    $r = $msp[$row]; for ($e = 0; $e -lt 8; $e++) { $o = $e * 16; if ($r[$o] -gt 0 -and $r[$o + 4] -gt 0) { "$($r[$o])@$($r[$o + 4])" } }
}
$ops = @{}
foreach ($r in Import-Csv -Delimiter "`t" -LiteralPath $ObjectParamSfx) { $ops[[int]$r.id] = $r }
$rows = New-Object System.Collections.ArrayList
$null = $rows.Add("map`tsource`tkey`tcount`tdetail")
foreach ($map in $Maps) {
    $path = Join-Path $MsbDir "map_MapStudio_${map}_00_00.msb.dcx"
    if (-not (Test-Path -LiteralPath $path)) { $null = $rows.Add("$map`tMISSING`t`t0`t"); continue }
    $m = [SoulsFormats.MSB3]::Read($path)
    $objs = @($m.Parts.Objects)
    $null = $rows.Add("$map`tobjects`tall`t$($objs.Count)`t")
    $null = $rows.Add("$map`tenemies`tall`t$(@($m.Parts.Enemies).Count)`t")
    # object models
    $byModel = $objs | Group-Object { $_.ModelName }
    foreach ($g in $byModel) {
        $num = 0; if ($g.Name -match '^[oO](\d{6})$') { $num = [int]$Matches[1] }
        if ($num -eq 0) { continue }
        $me = @(ModelEntries ($num * 1000))
        if ($me.Count -gt 0) { $null = $rows.Add("$map`tModelSfxParam`t$($g.Name)`t$($g.Count)`t$($me -join ' ')") }
        if ($ops.ContainsKey($num) -and $ops[$num].sfx0 -ne '-1') {
            $o = $ops[$num]; $ids = @($o.sfx0, $o.sfx1, $o.sfx2, $o.sfx3 | Where-Object { $_ -ne '-1' })
            $null = $rows.Add("$map`tObjectParam`t$($g.Name)`t$($g.Count)`tsfx $($ids -join ' ') second $($o.b0) $($o.b1) $($o.b2) $($o.b3)")
        }
    }
    $withLantern = @($objs | Where-Object { $_.LanternID -gt 0 }).Count
    $null = $rows.Add("$map`tobjects_with_LanternID`tall`t$withLantern`t")
    $withLight = @($objs | Where-Object { $_.LightID -gt 0 }).Count
    $null = $rows.Add("$map`tobjects_with_LightID`tall`t$withLight`t")
    # regions
    $sfx = @($m.Regions.SFX)
    $null = $rows.Add("$map`tSFXRegion`tall`t$($sfx.Count)`tstart_disabled=$(@($sfx | Where-Object { $_.StartDisabled }).Count) with_entity=$(@($sfx | Where-Object { $_.EventEntityID -gt 0 }).Count) with_activation_part=$(@($sfx | Where-Object { $_.ActivationPartName }).Count)")
    foreach ($g in ($sfx | Group-Object { $_.FFXID } | Sort-Object Count -Descending)) {
        $null = $rows.Add("$map`tSFXRegion_FFXID`t$($g.Name)`t$($g.Count)`t")
    }
    $wind = @($m.Regions.WindSFX)
    $null = $rows.Add("$map`tWindSFXRegion`tall`t$($wind.Count)`t$((($wind | ForEach-Object { $_.FFXID }) | Sort-Object -Unique) -join ' ')")
    $null = $rows.Add("$map`tTreasureEvents`tall`t$(@($m.Events.Treasures).Count)`tstart_disabled=$(@($m.Events.Treasures | Where-Object { $_.StartDisabled }).Count)")
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
[IO.File]::WriteAllLines($Out, $rows)
"rows $($rows.Count - 1) -> $Out"
