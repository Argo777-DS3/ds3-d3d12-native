[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$DumpDir,   # Dump-Gparam.ps1 (I-0045) output of param_drawparam_<zone>_0000 and _default
    [string[]]$Zone = @('m31_00', 'm37_00', 'm40_00', 'm45_00', 'm50_00', 'm51_00'),
    [string[]]$Group = @('Setting Info', 'Bloom', 'LightShaft', 'Dof'),
    [Parameter(Mandatory = $true)][string]$Out
)
# I-0068: the YEBIS gparam values of each witness zone (groups Setting Info, Bloom, LightShaft, Dof), with the fallback
# the engine applies when a zone leaves a parameter empty: the default file's value (TONEMAP-SKIN, "falls back to default
# id 0"). One TSV row per zone and parameter: group, parameter, zone value, default value, effective value. The dumps are
# licensed content and stay outside the repository; only the numbers are written.
$ErrorActionPreference = 'Stop'
function Read-Dump([string]$path) {
    $map = [ordered]@{}; $grp = ''; $par = ''
    foreach ($l in [IO.File]::ReadLines($path)) {
        if ($l -match '^GROUP \d+ .* / (.+) params=') { $grp = $Matches[1].Trim(); continue }
        if ($l -match '^  PARAM [\d.]+ (.+?) / (.+) type=\w+ values=(\d+)') { $par = $Matches[1].Trim(); $map["$grp|$par"] = $null; continue }
        if ($l -match '^    \[0\] (.+)$' -and $par) { if ($null -eq $map["$grp|$par"]) { $map["$grp|$par"] = ($Matches[1] -replace '\[[0-9A-F]{8}\]', '') } }
    }
    $map
}
$default = Read-Dump (Join-Path $DumpDir 'param_drawparam_default.gparam.txt')
$rows = foreach ($z in $Zone) {
    $d = Read-Dump (Join-Path $DumpDir "param_drawparam_$($z)_0000.gparam.txt")
    foreach ($k in $d.Keys) {
        $g, $p = $k -split '\|', 2
        if (-not ($Group | Where-Object { $g -like "$_*" })) { continue }
        $zv = $d[$k]; $dv = $default[$k]
        $ev = if ($null -ne $zv) { $zv } else { $dv }
        "$z`t$g`t$p`t$zv`t$dv`t$ev"
    }
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
@("zone`tgroup`tparameter`tzone_value`tdefault_value`teffective") + $rows | Set-Content -LiteralPath $Out -Encoding utf8
"rows=$(@($rows).Count) -> $Out"
