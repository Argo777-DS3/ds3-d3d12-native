[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string[]]$Capture,   # host-era capture folders (events.jsonl, resources.jsonl, states.jsonl)
    [Parameter(Mandatory = $true)][int[]]$Ps,              # pixel programme ids (capture-local) to list
    [Parameter(Mandatory = $true)][string]$Out
)
# I-0068: the draws of given pixel programmes in retained captures (identified by their position, see the notes), grouped by
# programme pair, with their targets (size:format), bound pixel SRV slots (slot=size:format), constant-buffer slots,
# blend / depth-stencil / raster state descriptors, topology, draw arguments, stencil reference and the deferred context.
# Counts are per capture (three frames). Read-only.
$ErrorActionPreference = 'Stop'
$rows = foreach ($c in $Capture) {
    $res = @{}
    foreach ($l in [IO.File]::ReadLines((Join-Path $c 'resources.jsonl'))) {
        $m = [regex]::Match($l, '"id":(\d+),.*"w":(\d+),"h":(\d+),.*"format":(\d+)')
        if ($m.Success) { $res[[int]$m.Groups[1].Value] = "$($m.Groups[2].Value)x$($m.Groups[3].Value):f$($m.Groups[4].Value)" }
    }
    $st = @{}
    foreach ($l in [IO.File]::ReadLines((Join-Path $c 'states.jsonl'))) {
        $m = [regex]::Match($l, '"id":(\d+),"kind":(\d+),"desc":\[([^\]]*)\]')
        if ($m.Success) { $st["$($m.Groups[2].Value):$($m.Groups[1].Value)"] = $m.Groups[3].Value }
    }
    $groups = [ordered]@{}
    foreach ($l in [IO.File]::ReadLines((Join-Path $c 'events.jsonl'))) {
        if ($l -match '"vs":0,') { continue }
        $rtv = @(([regex]::Match($l, '"rtv":\[([^\]]*)\]').Groups[1].Value -split ',') | ForEach-Object { [int]([int64]$_ -band 0xFFFF) })
        if ($Ps -notcontains [int]([regex]::Match($l, '"ps":(\d+)').Groups[1].Value)) { continue }
        $srv = @(([regex]::Match($l, '"ps_srv":\[([^\]]*)\]').Groups[1].Value -split ',') | ForEach-Object { [int]([int64]$_ -band 0xFFFF) })
        $slots = for ($i = 0; $i -lt $srv.Count; $i++) { if ($srv[$i]) { "t$i=$($res[$srv[$i]])" } }
        $cbs = { param($f) $v = @(([regex]::Match($l, '"' + $f + '":\[([^\]]*)\]').Groups[1].Value -split ',')); (0..($v.Count - 1) | Where-Object { $v[$_] -ne '0' } | ForEach-Object { "b$_" }) -join ',' }
        $g = { param($f) [regex]::Match($l, '"' + $f + '":(\d+)').Groups[1].Value }
        $key = "vs$(& $g 'vs') ps$(& $g 'ps')`t$(($rtv | ForEach-Object { if ($_) { $res[$_] } else { '-' } }) -join ' ')`t$($slots -join ' ')`tvs:$(& $cbs 'vs_cb') ps:$(& $cbs 'ps_cb')`tblend $(& $g 'blend')=[$($st['1:' + (& $g 'blend')])]`tds $(& $g 'depth_stencil')=[$($st['2:' + (& $g 'depth_stencil')])]`traster $(& $g 'raster')=[$($st['3:' + (& $g 'raster')])]`ttopo $(& $g 'topology') kind $(& $g 'kind') args [$([regex]::Match($l, '"args":\[([^\]]*)\]').Groups[1].Value)]`tref $(& $g 'stencil_ref')`tdsv $($res[[int]([int64](& $g 'dsv') -band 0xFFFF)])"
        if (-not $groups.Contains($key)) { $groups[$key] = @(0, (& $g 'seq'), (& $g 'ctx')) }
        $groups[$key][0]++
    }
    foreach ($k in $groups.Keys) { "$c`t$($groups[$k][0])`t$($groups[$k][1])`t$($groups[$k][2])`t$k" }
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
@("capture`tdraws`tfirst_seq`tctx`tprogrammes`ttargets`tps_srv`tcbuffers`tblend`tdepth_stencil`traster`tdraw`tstencil_ref`tdsv") + $rows | Set-Content -LiteralPath $Out -Encoding utf8
"rows=$(@($rows).Count) -> $Out"

