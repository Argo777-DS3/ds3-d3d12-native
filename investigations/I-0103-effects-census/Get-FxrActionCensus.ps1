[CmdletBinding()]
param(
    [string]$Scan,                                       # default: evidence/I-0063/artifacts/scan/fxr.tsv of this checkout
    [Parameter(Mandatory = $true)][string]$Out           # a folder: writes actions.tsv and actions-by-bundle.tsv
)
# I-0103: the whole game's FXR action kinds from I-0063's scan (one row per bundle, FXR and action, with its occurrence count).
# actions.tsv: per action, its occurrences, the distinct FXRs that hold it and the bundles. actions-by-bundle.tsv: per action and
# bundle (commoneffects, dlc1, dlc2, mNN), occurrences and distinct FXRs. Read-only; writes only under $Out.
$ErrorActionPreference = 'Stop'
if (-not $Scan) { $Scan = Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'evidence\I-0063\artifacts\scan\fxr.tsv' }
$rows = Import-Csv -LiteralPath $Scan -Delimiter "`t"
$bundleOf = { param($b) if ($b -match 'frpg_sfxbnd_(.+?)_effect') { $Matches[1] } else { 'loose:' + ($b -replace '.*\\', '') } }
$byAction = @{}; $byPair = @{}
foreach ($r in $rows) {
    $a = [int]$r.action; $b = & $bundleOf $r.bundle; $n = [int]$r.count
    if (-not $byAction.ContainsKey($a)) { $byAction[$a] = @{ count = 0; fxr = @{}; bundles = @{} } }
    $byAction[$a].count += $n; $byAction[$a].fxr[$r.fxr] = 1; $byAction[$a].bundles[$b] = 1
    $k = "$a`t$b"
    if (-not $byPair.ContainsKey($k)) { $byPair[$k] = @{ count = 0; fxr = @{} } }
    $byPair[$k].count += $n; $byPair[$k].fxr[$r.fxr] = 1
}
New-Item -ItemType Directory -Force -Path $Out | Out-Null
$lines = foreach ($a in ($byAction.Keys | Sort-Object)) {
    $v = $byAction[$a]; "$a`t$($v.count)`t$($v.fxr.Count)`t$($v.bundles.Count)"
}
@("action`toccurrences`tfxrs`tbundles") + $lines | Set-Content -LiteralPath (Join-Path $Out 'actions.tsv') -Encoding utf8
$pairs = foreach ($k in ($byPair.Keys | Sort-Object { [int]($_ -split "`t")[0] }, { ($_ -split "`t")[1] })) {
    "$k`t$($byPair[$k].count)`t$($byPair[$k].fxr.Count)"
}
@("action`tbundle`toccurrences`tfxrs") + $pairs | Set-Content -LiteralPath (Join-Path $Out 'actions-by-bundle.tsv') -Encoding utf8
"actions=$(@($lines).Count) pairs=$(@($pairs).Count) rows=$($rows.Count) -> $Out"
