param([Parameter(Mandatory)][string[]]$Runs)
# I-0097 tranche 2 (Thomas, 2026-10-23 18:14: the load time against the original, side by side): for each endurance run made
# with Run-Endurance.ps1 (native, or -Original -Probe: the original with the module in Reference mode), each load's length from
# the tour's click on the save's OK (tour.txt, "zone <name> loading ... tick=<GetTickCount64>") to the first game step after it
# (the journal's game_step_resumed row, tick_ms, written alike by both renderers at the world update gate). One row per load,
# then per run and zone the count, mean, median, min and max in ms. A load with no resumed row before the next click is skipped.
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Runs=@($Runs | ForEach-Object { $_ -split '[,;]' } | ForEach-Object { $_.Trim() } | Where-Object { $_ })   # pwsh -File passes a list as one string
$rows=foreach($run in $Runs){ $dir="$repo/evidence/I-0066/artifacts/$run"
    $resumed=@(Select-String -LiteralPath "$dir/startup.jsonl" -Pattern '"event":"game_step_resumed".*"tick_ms":(\d+)' | ForEach-Object { [uint64]$_.Matches[0].Groups[1].Value })
    $clicks=@(Select-String -LiteralPath "$dir/tour.txt" -Pattern '^zone (.+) loading visit=(\d+) .* tick=(\d+)$' | ForEach-Object { $g=$_.Matches[0].Groups
        [pscustomobject]@{zone=$g[1].Value;visit=[int]$g[2].Value;tick=[uint64]$g[3].Value} })
    for($i=0;$i -lt $clicks.Count;$i++){ $c=$clicks[$i];$next=if($i+1 -lt $clicks.Count){ $clicks[$i+1].tick }else{ [uint64]::MaxValue }
        $end=$resumed | Where-Object { $_ -gt $c.tick -and $_ -lt $next } | Select-Object -First 1
        if($end){ [pscustomobject]@{run=$run;zone=$c.zone;visit=$c.visit;load_ms=[int]($end-$c.tick)} } } }
$rows | Format-Table -AutoSize | Out-String -Width 200
$rows | Group-Object run,zone | ForEach-Object { $v=@($_.Group.load_ms | Sort-Object);$n=$v.Count
    [pscustomobject]@{run=$_.Group[0].run;zone=$_.Group[0].zone;loads=$n;mean_ms=[int](($v | Measure-Object -Average).Average);
        median_ms=$(if($n%2){$v[[int][Math]::Floor($n/2)]}else{[int](($v[$n/2-1]+$v[$n/2])/2)});min_ms=$v[0];max_ms=$v[-1]} } | Format-Table -AutoSize | Out-String -Width 200
