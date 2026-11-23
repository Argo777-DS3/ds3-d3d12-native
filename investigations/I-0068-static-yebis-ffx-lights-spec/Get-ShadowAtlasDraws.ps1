[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string[]]$Capture,   # host-era capture folders (events.jsonl, resources.jsonl, states.jsonl)
    [int]$MinSize = 1024,                                # depth targets at least this wide are treated as shadow atlases
    [Parameter(Mandatory = $true)][string]$Out
)
# I-0068: the shadow-caster draws of retained captures: every draw whose depth target is a square depth resource of at least
# MinSize texels, grouped by atlas (size, DXGI format), viewport (the cascade or light tile), depth-stencil state and whether a
# pixel shader is bound (alpha-tested casters). Counts are per capture (the I-0063 captures hold three frames). Read-only.
$ErrorActionPreference = 'Stop'
$rows = foreach ($c in $Capture) {
    $res = @{}
    foreach ($l in [IO.File]::ReadLines((Join-Path $c 'resources.jsonl'))) {
        $m = [regex]::Match($l, '"id":(\d+),.*"w":(\d+),"h":(\d+),.*"format":(\d+)')
        if ($m.Success) { $res[[int]$m.Groups[1].Value] = @([int]$m.Groups[2].Value, [int]$m.Groups[3].Value, [int]$m.Groups[4].Value) }
    }
    $ds = @{}
    foreach ($l in [IO.File]::ReadLines((Join-Path $c 'states.jsonl'))) {
        $m = [regex]::Match($l, '"id":(\d+),"kind":2,"desc":\[([^\]]*)\]')
        if ($m.Success) { $d = $m.Groups[2].Value -split ','; $ds[[int]$m.Groups[1].Value] = "$($d[0])/$($d[1])/$($d[2])/$($d[3])" }
    }
    $count = @{}
    foreach ($l in [IO.File]::ReadLines((Join-Path $c 'events.jsonl'))) {
        if ($l -match '"vs":0,') { continue }
        $dsv = [int]([int64]([regex]::Match($l, '"dsv":(\d+)').Groups[1].Value) -band 0xFFFF)
        $r = $res[$dsv]
        if (-not $r -or $r[0] -lt $MinSize -or $r[0] -ne $r[1]) { continue }
        $vp = ([regex]::Match($l, '"viewport":\[([^\]]*)\]').Groups[1].Value -split ',')[0..3] -join ','
        $ps = if ($l -match '"ps":0,') { 'depth-only' } else { 'pixel-shader' }
        $key = "$($r[0])x$($r[1])`t$($r[2])`t$vp`t$($ds[[int]([regex]::Match($l, '"depth_stencil":(\d+)').Groups[1].Value)])`t$ps"
        if (-not $count.ContainsKey($key)) { $count[$key] = 0 }
        $count[$key]++
    }
    foreach ($k in $count.Keys) { "$c`t$k`t$($count[$k])" }
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
@("capture`tatlas`tformat`tviewport`tenable/write/func/stencil`tcaster`tdraws") + ($rows | Sort-Object) | Set-Content -LiteralPath $Out -Encoding utf8
"rows=$(@($rows).Count) -> $Out"
