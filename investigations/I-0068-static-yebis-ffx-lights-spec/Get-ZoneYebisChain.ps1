[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$DumpDir,    # Get-GparamValues.ps1 TSVs: default.tsv and <zone>_0000.tsv
    [string[]]$Zone = @('m30_00', 'm31_00', 'm37_00', 'm40_00', 'm45_00', 'm50_00', 'm51_00'),
    [string[]]$Group = @('Tone Map', 'Lens Simulation', 'Dof', 'ColorGrading', 'FeedBackBlur', 'ColorParaffin Filter', 'Anti-Alias', 'Glow Filter', 'Bloom', 'LightShaft'),
    [Parameter(Mandatory = $true)][string]$Out
)
# I-0068 part AO: the value of every YEBIS-chain gparam parameter at gparam value id 0 for each witness zone (primary
# `<zone>_0000` file), with the default file's id 0 as the fallback of the lookup (area, id) -> (1, 0) (LIVE-REDUCTION-AM §5).
# One TSV row per group and parameter: default id 0, then one column per zone (blank = the zone file has no value at id 0, so the
# default applies), then the number of distinct value ids the zone files carry for the parameter's group.
$ErrorActionPreference = 'Stop'
function Load([string]$path) {
    $m = @{}; $ids = @{}
    foreach ($r in Import-Csv -Delimiter "`t" -LiteralPath $path) {
        if (-not $r.group -or $r.group.StartsWith('#')) { continue }
        $k = "$($r.group)|$($r.param)"
        if ($r.id -eq '0') { if ($m.ContainsKey($k)) { $m[$k] = $m[$k] + ' ; ' + $r.floats } else { $m[$k] = $r.floats } }   # a group can repeat a name (Tone Map: YEBIS half first, GX half second): joined with ' ; '
        if (-not $ids.ContainsKey($r.group)) { $ids[$r.group] = New-Object System.Collections.Generic.HashSet[string] }
        [void]$ids[$r.group].Add($r.id)
    }
    @{ v = $m; ids = $ids }
}
$def = Load (Join-Path $DumpDir 'default.tsv')
$zs = @{}; foreach ($z in $Zone) { $zs[$z] = Load (Join-Path $DumpDir "${z}_0000.tsv") }
$rows = New-Object System.Collections.ArrayList
$null = $rows.Add("group`tparameter`tdefault_id0`t$($Zone -join "`t")`tvalue_ids_in_zone_files")
$keys = New-Object System.Collections.Specialized.OrderedDictionary
foreach ($src in @($def) + @($Zone | ForEach-Object { $zs[$_] })) { foreach ($k in $src.v.Keys) { if (-not $keys.Contains($k)) { $keys.Add($k, 1) } } }
foreach ($k in $keys.Keys) {
    $g, $p = $k -split '\|', 2
    if (-not ($Group | Where-Object { $g -eq $_ })) { continue }
    $cols = foreach ($z in $Zone) { if ($zs[$z].v.ContainsKey($k)) { $zs[$z].v[$k] } else { '' } }
    $idc = ($Zone | ForEach-Object { if ($zs[$_].ids.ContainsKey($g)) { "$_=" + (($zs[$_].ids[$g] | Sort-Object { [int]$_ }) -join ',') } }) -join ' '
    $dv = if ($def.v.ContainsKey($k)) { $def.v[$k] } else { '' }
    $null = $rows.Add("$g`t$p`t$dv`t$($cols -join "`t")`t$idc")
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
[IO.File]::WriteAllLines($Out, $rows)
"rows $($rows.Count - 1) -> $Out"
