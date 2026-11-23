[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string[]]$Capture,   # capture folders holding events.jsonl (host-era live_host captures)
    [int]$FirstProgram = 15800, [int]$LastProgram = 15999,  # YEBIS programme ids of that host build (created lazily after GX's)
    [Parameter(Mandatory = $true)][string]$Out
)
# I-0068: the YEBIS draws of retained captures in submission order, with their programme ids. The bytecode of those
# captures was removed (retention), so the ids only say which draws share a VS or a PS; the vertex stride and the bound
# resources come with them, sized from resources.jsonl (w x h : DXGI format). One TSV row per draw.
$ErrorActionPreference = 'Stop'
$rows = foreach ($c in $Capture) {
    $res = @{}
    foreach ($l in [IO.File]::ReadLines((Join-Path $c 'resources.jsonl'))) {
        $m = [regex]::Match($l, '"id":(\d+),.*"w":(\d+),"h":(\d+),.*"format":(\d+)')
        if ($m.Success) { $res[[int]$m.Groups[1].Value] = "$($m.Groups[2].Value)x$($m.Groups[3].Value):$($m.Groups[4].Value)" }
    }
    $size = { param($list) (($list -split '/') | Where-Object { $_ } | ForEach-Object { $r = $res[[int]$_]; if ($r) { $r } else { '?' } }) -join '/' }
    foreach ($l in [IO.File]::ReadLines((Join-Path $c 'events.jsonl'))) {
        if ($l -notmatch '"kind":16,' -or $l -notmatch '"args":\[12,') { continue }
        $ps = [int]([regex]::Match($l, '"ps":(\d+)').Groups[1].Value)
        if ($ps -lt $FirstProgram -or $ps -gt $LastProgram) { continue }
        $ids = { param($f) ((([regex]::Match($l, '"' + $f + '":\[([^\]]*)\]').Groups[1].Value) -split ',') | Where-Object { $_ -and $_ -ne '0' } | ForEach-Object { [int64]$_ -band 0xFFFF }) -join '/' }
        $o = [pscustomobject]@{ rt_size = ''; srv_size = ''
            capture = $c; seq = [int]([regex]::Match($l, '"seq":(\d+)').Groups[1].Value)
            vs = [int]([regex]::Match($l, '"vs":(\d+)').Groups[1].Value); ps = $ps
            stride = ([regex]::Match($l, '"vb":\[\d+,(\d+)').Groups[1].Value)
            rt = (& $ids 'rtv'); srv = (& $ids 'ps_srv')
        }
        $o.rt_size = & $size $o.rt; $o.srv_size = & $size $o.srv; $o
    }
}
$rows = $rows | Sort-Object capture, seq
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
@("capture`tseq`tvs`tps`tstride`trt`trt_size`tsrv`tsrv_size") + ($rows | ForEach-Object { "$($_.capture)`t$($_.seq)`t$($_.vs)`t$($_.ps)`t$($_.stride)`t$($_.rt)`t$($_.rt_size)`t$($_.srv)`t$($_.srv_size)" }) | Set-Content -LiteralPath $Out -Encoding utf8
"rows=$(@($rows).Count) -> $Out"
