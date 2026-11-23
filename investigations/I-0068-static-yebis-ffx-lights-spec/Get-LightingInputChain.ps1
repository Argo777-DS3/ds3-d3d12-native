[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string[]]$Capture,   # host-era capture folders (events.jsonl, resources.jsonl)
    [int[]]$Slot = @(32, 33),                            # pixel SRV slots of the deferred lighting draws to trace (t32 shadow/AO mask, t33 SSAO)
    [int]$Depth = 4,                                     # how many producer levels to follow back
    [Parameter(Mandatory = $true)][string]$Out
)
# I-0068: the producer chain of the lighting inputs in retained captures. The lighting draws are the draws into the
# R11G11B10 pair (diffuse/specular accumulation) that bind both t32 and t33 (GXLightDeferredAO); for each traced slot, the texture bound there is followed back through
# the draws that render it (render target), their own inputs, and so on. Each producer draw is listed once per frame with
# its target and input sizes/formats, depth view and stencil reference. Deferred contexts record in parallel, so writers are taken from the whole capture, not by sequence. Read-only.
$ErrorActionPreference = 'Stop'
$rows = foreach ($c in $Capture) {
    $res = @{}
    foreach ($l in [IO.File]::ReadLines((Join-Path $c 'resources.jsonl'))) {
        $m = [regex]::Match($l, '"id":(\d+),.*"w":(\d+),"h":(\d+),.*"format":(\d+)')
        if ($m.Success) { $res[[int]$m.Groups[1].Value] = "$($m.Groups[2].Value)x$($m.Groups[3].Value):f$($m.Groups[4].Value)" }
    }
    $draws = New-Object System.Collections.Generic.List[object]
    foreach ($l in [IO.File]::ReadLines((Join-Path $c 'events.jsonl'))) {
        if ($l -match '"vs":0,') { continue }
        $ids = { param($f) @((([regex]::Match($l, '"' + $f + '":\[([^\]]*)\]').Groups[1].Value) -split ',') | ForEach-Object { [int]([int64]$_ -band 0xFFFF) }) }
        $rt = @((& $ids 'rtv') | Where-Object { $_ })
        $draws.Add([pscustomobject]@{ seq = [int]([regex]::Match($l, '"seq":(\d+)').Groups[1].Value); rt = $rt
            srv = (& $ids 'ps_srv'); dsv = [int]([int64]([regex]::Match($l, '"dsv":(\d+)').Groups[1].Value) -band 0xFFFF)
            ref = [regex]::Match($l, '"stencil_ref":(\d+)').Groups[1].Value; ps = [regex]::Match($l, '"ps":(\d+)').Groups[1].Value })
    }
    $lighting = $draws | Where-Object { $_.rt.Count -eq 2 -and (@($_.rt | ForEach-Object { $res[$_] }) -match ':f26$').Count -eq 2 -and $_.srv[32] -and $_.srv[33] } | Select-Object -First 1
    if (-not $lighting) { continue }
    $queue = foreach ($s in $Slot) { if ($lighting.srv[$s]) { [pscustomobject]@{ id = $lighting.srv[$s]; level = 0; from = "t$s" } } }
    $seen = @{}
    while ($queue) {
        $item, $queue = $queue
        if ($seen.ContainsKey($item.id) -or $item.level -ge $Depth) { continue }
        $seen[$item.id] = 1
        $writers = $draws | Where-Object { $_.rt -contains $item.id } | Group-Object ps | ForEach-Object { $_.Group[-1] }
        foreach ($w in $writers) {
            $inputs = ($w.srv | Where-Object { $_ } | ForEach-Object { "$_=$($res[$_])" }) -join ' '
            "$c`t$($item.from)`t$($item.level)`t$($item.id)=$($res[$item.id])`t$($w.seq)`t$($w.ps)`t$(($w.rt | ForEach-Object { "$_=$($res[$_])" }) -join ' ')`t$($w.dsv)=$($res[$w.dsv])`t$($w.ref)`t$inputs"
            foreach ($i in ($w.srv | Where-Object { $_ })) { $queue = @($queue) + [pscustomobject]@{ id = $i; level = $item.level + 1; from = "$($item.from)<$($item.id)" } }
        }
    }
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
@("capture`ttraced`tlevel`ttexture`twriter_seq`tps`ttargets`tdsv`tstencil_ref`tinputs") + $rows | Set-Content -LiteralPath $Out -Encoding utf8
"rows=$(@($rows).Count) -> $Out"
