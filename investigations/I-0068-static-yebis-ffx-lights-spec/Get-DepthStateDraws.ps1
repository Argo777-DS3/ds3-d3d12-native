[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string[]]$Capture,   # host-era capture folders (events.jsonl, states.jsonl, resources.jsonl)
    [Parameter(Mandatory = $true)][string]$Out
)
# I-0068: every draw of a retained capture that binds a depth-stencil view, in submission order, with its depth-stencil
# state (states.jsonl kind 2: DepthEnable, DepthWriteMask, DepthFunc, StencilEnable, ...), its render targets (count and
# DXGI format of the first), its topology, its shader-resource views that are depth resources (typeless/depth formats 19, 20, 39-41,
# 44-46, 55) and its call-site return addresses (RVAs). One TSV row per draw; read-only.
$ErrorActionPreference = 'Stop'
$depthFormats = @(19, 20, 39, 40, 41, 44, 45, 46, 55)
$rows = foreach ($c in $Capture) {
    $ds = @{}
    foreach ($l in [IO.File]::ReadLines((Join-Path $c 'states.jsonl'))) {
        $m = [regex]::Match($l, '"id":(\d+),"kind":2,"desc":\[([^\]]*)\]')
        if ($m.Success) { $d = $m.Groups[2].Value -split ','; $ds[[int]$m.Groups[1].Value] = "$($d[0])/$($d[1])/$($d[2])/$($d[3])" }
    }
    $fmt = @{}
    foreach ($l in [IO.File]::ReadLines((Join-Path $c 'resources.jsonl'))) {
        $m = [regex]::Match($l, '"id":(\d+),.*"w":(\d+),"h":(\d+),.*"format":(\d+)')
        if ($m.Success) { $fmt[[int]$m.Groups[1].Value] = @([int]$m.Groups[4].Value, "$($m.Groups[2].Value)x$($m.Groups[3].Value)") }
    }
    foreach ($l in [IO.File]::ReadLines((Join-Path $c 'events.jsonl'))) {
        if ($l -match '"vs":0,') { continue }   # draws of every kind (Draw, DrawIndexed, instanced, indirect) carry a VS
        $dsv = [int64]([regex]::Match($l, '"dsv":(\d+)').Groups[1].Value) -band 0xFFFF
        if ($dsv -eq 0) { continue }
        $list = { param($f) @((([regex]::Match($l, '"' + $f + '":\[([^\]]*)\]').Groups[1].Value) -split ',') | Where-Object { $_ -and $_ -ne '0' } | ForEach-Object { [int64]$_ -band 0xFFFF }) }
        $rt = & $list 'rtv'
        $srvDepth = (& $list 'ps_srv') | Where-Object { $fmt.ContainsKey([int]$_) -and $depthFormats -contains $fmt[[int]$_][0] }
        $dsId = [int]([regex]::Match($l, '"depth_stencil":(\d+)').Groups[1].Value)
        $rt0 = if ($rt.Count) { $f = $fmt[[int]$rt[0]]; if ($f) { "$($f[0]):$($f[1])" } else { '?' } } else { '-' }
        $callers = (([regex]::Match($l, '"callers":\[([^\]]*)\]').Groups[1].Value) -split ',' | ForEach-Object { '{0:X}' -f [int64]$_ }) -join '/'
        "$c`t$([regex]::Match($l, '"seq":(\d+)').Groups[1].Value)`t$([regex]::Match($l, '"topology":(\d+)').Groups[1].Value)`t$($rt.Count)`t$rt0`t$dsv`t$dsId`t$($ds[$dsId])`t$($srvDepth -join '/')`t$([regex]::Match($l, '"ps":(\d+)').Groups[1].Value)`t$callers"
    }
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
@("capture`tseq`ttopology`trt_count`trt0_format_size`tdsv`tds_state`tenable/write/func/stencil`tdepth_srv`tps`tcallers") + $rows | Set-Content -LiteralPath $Out -Encoding utf8
"rows=$(@($rows).Count) -> $Out"
