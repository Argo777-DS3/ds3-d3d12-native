[CmdletBinding()]
param(
    [string]$ZoneFamilies,                               # default: evidence/I-0063/inventory/zone-families.tsv of this checkout
    [string[]]$Zone = @('m31_00', 'm37_00', 'm45_00', 'm50_00', 'm51_00', 'm40_00'),
    [Parameter(Mandatory = $true)][string]$Out
)
# I-0068: effect families E01-E10 per zone from the I-0063 FXR weights (action occurrences in the zone's effect bundles),
# with the shared bundles each zone also loads: shared:commoneffects everywhere, shared:dlc1 for m45, shared:dlc2 for
# m50/m51 (INFERRED from the DLC each map belongs to). One TSV row per zone and family: zone weight, shared weight, total,
# rank by zone weight. Read-only; writes only $Out.
$ErrorActionPreference = 'Stop'
if (-not $ZoneFamilies) { $ZoneFamilies = Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'evidence\I-0063\inventory\zone-families.tsv' }
$rows = Import-Csv -LiteralPath $ZoneFamilies -Delimiter "`t" | Where-Object { $_.family -match '^E\d\d ' }
$shared = @{ 'm45_00' = @('shared:commoneffects', 'shared:dlc1'); 'm50_00' = @('shared:commoneffects', 'shared:dlc2'); 'm51_00' = @('shared:commoneffects', 'shared:dlc2') }
$families = 1..10 | ForEach-Object { 'E{0:D2}' -f $_ }
$lines = foreach ($z in $Zone) {
    $bundles = if ($shared.ContainsKey($z)) { $shared[$z] } else { @('shared:commoneffects') }
    $list = foreach ($f in $families) {
        $zw = ($rows | Where-Object { $_.zone -eq $z -and $_.family.StartsWith($f) } | Measure-Object -Property weight -Sum).Sum
        $sw = ($rows | Where-Object { $bundles -contains $_.zone -and $_.family.StartsWith($f) } | Measure-Object -Property weight -Sum).Sum
        [pscustomobject]@{ zone = $z; family = $f; zone_weight = [int]$zw; shared_weight = [int]$sw; total = [int]$zw + [int]$sw }
    }
    $rank = 0
    foreach ($r in ($list | Sort-Object -Property @{ Expression = 'zone_weight'; Descending = $true }, family)) {
        $rank++; "$($r.zone)`t$($r.family)`t$($r.zone_weight)`t$($r.shared_weight)`t$($r.total)`t$(if ($r.zone_weight -gt 0) { $rank } else { '-' })"
    }
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
@("zone`tfamily`tzone_weight`tshared_weight`ttotal`trank_by_zone_weight") + $lines | Set-Content -LiteralPath $Out -Encoding utf8
"rows=$(@($lines).Count) -> $Out"
