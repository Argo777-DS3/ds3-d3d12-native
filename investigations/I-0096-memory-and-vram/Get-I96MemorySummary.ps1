param([Parameter(Mandatory)][string[]]$Run,[switch]$Json)
# I-0096: the medians of a memory run's counters (Run-Endurance.ps1's memory.csv, sampled from outside) per phase of its tour
# (tour.txt): the title before the first load, each stay in the zone (in world to leaving) and each title after a visit (the
# title reached, or the quit seen when no such line follows, to the next loading or the tour's end; Run-Endurance -TitleSeconds
# lengthens it). Private bytes and the GPU counters of a row are read about 1.4 s apart (one transition sample may mix them). System memory = private - GPU total committed (video memory is charged to the
# private bytes, I-0096 O1). The same for the original and the native runs.
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
function Utc([string]$line){ [DateTime]::Parse(([regex]::Match($line,'\d{4}-\d\d-\d\dT[\d:.]+Z')).Value).ToUniversalTime() }
function Median([double[]]$v){ if(-not $v.Count){ return $null }; $s=$v | Sort-Object; if($s.Count%2){ $s[[int][Math]::Floor($s.Count/2)] }else{ ($s[$s.Count/2-1]+$s[$s.Count/2])/2 } }
$all=foreach($name in $Run){
    $o="$repo/evidence/I-0066/artifacts/$name"
    $rows=Import-Csv -LiteralPath "$o/memory.csv" | ForEach-Object { $_ | Add-Member -NotePropertyName t -NotePropertyValue ([DateTime]::Parse($_.utc).ToUniversalTime()) -PassThru }
    $tour=Get-Content -LiteralPath "$o/tour.txt"
    $phases=[System.Collections.Generic.List[object]]::new()
    $start=Utc ($tour | Where-Object { $_ -like 'tour start*' } | Select-Object -First 1)
    $end=Utc ($tour | Where-Object { $_ -like 'tour end*' -or $_ -like 'tour visits done*' } | Select-Object -First 1)
    $open=@{name='title-0';from=$start.AddSeconds(-30)}   # the title's main menu was reached before the tour (Pilot-TitleContinue)
    foreach($line in $tour){
        if($line -match 'loading visit=(\d+)'){ if($open){ $phases.Add([pscustomobject]@{name=$open.name;from=$open.from;to=(Utc $line)}) };$open=$null }
        elseif($line -match 'in world visit=(\d+)'){ $open=@{name="zone-$($Matches[1])";from=(Utc $line)} }
        elseif($line -match 'leaving visit=(\d+)'){ if($open){ $phases.Add([pscustomobject]@{name=$open.name;from=$open.from;to=(Utc $line)}) };$open=$null }
        elseif($line -match ' left visit=(\d+)'){ $open=@{name="title-$($Matches[1])";from=(Utc $line)} }
        # final review 2: the title's own stay opens when the title is reached (the quit's load comes before it)
        elseif($line -match '^title .*title reached' -and $open -and $open.name -like 'title-*'){ $open.from=Utc $line } }
    if($open){ $phases.Add([pscustomobject]@{name=$open.name;from=$open.from;to=$end}) }
    foreach($p in $phases){
        # the first 10 s of a stay are left out (the load's tail, the title's arrival)
        $in=@($rows | Where-Object { $_.t -ge $p.from.AddSeconds(10) -and $_.t -le $p.to })
        $num={ param($c) @($in | Where-Object { $_.$c -ne '' -and $null -ne $_.$c } | ForEach-Object { [double]$_.$c }) }
        $private=Median (& $num 'private_mb');$committed=Median (& $num 'gpu_committed_mb')
        [pscustomobject]@{run=$name;phase=$p.name;samples=$in.Count;private_mb=$private;gpu_dedicated_mb=(Median (& $num 'gpu_dedicated_mb'));
            gpu_shared_mb=(Median (& $num 'gpu_shared_mb'));gpu_local_mb=(Median (& $num 'gpu_local_mb'));gpu_nonlocal_mb=(Median (& $num 'gpu_nonlocal_mb'));
            gpu_committed_mb=$committed;system_mb=$(if($null -ne $private -and $null -ne $committed){ $private-$committed }else{ $null })} } }
if($Json){ $all | ConvertTo-Json -Depth 3 }else{ $all | Format-Table -AutoSize | Out-String -Width 220 }
