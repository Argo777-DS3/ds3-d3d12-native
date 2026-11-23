[CmdletBinding()]
param(
    [string]$Reference = (Join-Path $PSScriptRoot '..\..\evidence\I-0066\artifacts\i66-reference-lights-1152\light-state.jsonl'),
    [string]$Native = (Join-Path $PSScriptRoot '..\..\evidence\I-0066\artifacts\i66-native-lights-1153\light-state.jsonl'),
    [string]$Output = (Join-Path $PSScriptRoot '..\..\evidence\I-0068\queries\ffx\effect-light-families.tsv')
)
# I-0068: groups the scene lights present in the original (Reference1152) and absent natively (Native1153) by their
# light parameters (kind, radius, colour, strengths, fade). One group = one FXR light definition instanced N times
# (scale varies per instance when the FXR curves animate it). Reads the last snapshot of each file (Read-LightState).
$ErrorActionPreference = 'Stop'
$r = (Get-Content -LiteralPath $Reference | Select-Object -Last 1 | ConvertFrom-Json).lights
$n = (Get-Content -LiteralPath $Native | Select-Object -Last 1 | ConvertFrom-Json).lights
$key = { param($l) "$($l.kind)|$($l.pos -join ',')|$($l.radius)" }
$nativeKeys = @{}; foreach ($l in $n) { $nativeKeys[(& $key $l)] = 1 }
$extra = @(); for ($i = 0; $i -lt $r.Count; $i++) { if (-not $nativeKeys.ContainsKey((& $key $r[$i]))) { $extra += [pscustomobject]@{ index = $i; light = $r[$i] } } }
$rows = $extra | Group-Object { $l = $_.light; "$($l.kind)`t$($l.radius)`t$($l.colour -join '/')`t$($l.strength)`t$($l.specular)`t$($l.fade)" } |
    Sort-Object Count -Descending | ForEach-Object {
        $scales = $_.Group | ForEach-Object { $_.light.scale }
        $xs = $_.Group | ForEach-Object { $_.light.pos[0] }; $zs = $_.Group | ForEach-Object { $_.light.pos[2] }
        "$($_.Count)`t$($_.Name)`t$(($scales | Measure-Object -Minimum).Minimum)..$(($scales | Measure-Object -Maximum).Maximum)`t" +
        "$(($xs | Measure-Object -Minimum).Minimum)..$(($xs | Measure-Object -Maximum).Maximum)`t$(($zs | Measure-Object -Minimum).Minimum)..$(($zs | Measure-Object -Maximum).Maximum)`t" +
        "$(($_.Group | ForEach-Object { $_.index } | Measure-Object -Minimum).Minimum)..$(($_.Group | ForEach-Object { $_.index } | Measure-Object -Maximum).Maximum)"
    }
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Output) | Out-Null
@("# reference=$($r.Count) native=$($n.Count) absent-natively=$($extra.Count) native-not-in-reference=$($refKeys = @{}; foreach ($l in $r) { $refKeys[(& $key $l)] = 1 }; @($n | Where-Object { -not $refKeys.ContainsKey((& $key $_)) }).Count)",
  "count`tkind`tradius`tcolour`tstrength`tspecular`tfade`tscale range`tx range`tz range`tvector index range") + $rows |
    Set-Content -LiteralPath $Output -Encoding utf8
Get-Content -LiteralPath $Output
