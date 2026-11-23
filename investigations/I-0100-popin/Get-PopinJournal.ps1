param([Parameter(Mandatory)][string[]]$Runs,[string]$Reference='',[double]$Cell=0.5,[int]$Hold=30,[int]$LoadFrames=300,[int]$Top=15,[string]$Out='',[double]$Near=10)
# I-0100 T0 (POPIN-STUDY §4 M1): reads the popin-observe.flag rows of runs (evidence/I-0066/artifacts/<run>/startup.jsonl, or a
# journal file such as the launcher's play-*.jsonl with -PopinObserve; -Near: appearances nearer than that, listed with their frame):
# popin_native (the native's main view, per prepared frame), popin_reference (the original's main-view draws, per game step,
# with popin_reference_window: each window's emit count and the times of its first and last emit) and popin_first (the
# native's first admission of each model and its time).
# An object is keyed by its model name and its position rounded to -Cell metres (a moving character takes a new key each time
# it reappears, alike in both renderers). Each object's rows become appearances: an « off » followed by an « on » within -Hold
# frames is no gap (the original emits a static distant piece on 2 frames of about 15, i100-t0b-settlement-o1: its draws are
# not re-emitted every frame; the same hold applies to the native), and the appearances in the first -LoadFrames frames after
# the run's first drawn frame are the load's, set apart. Per run: objects, appearances after the load and their distances,
# reasons the native stopped drawing, the native's first admissions. With -Reference <run>: each native run against the
# original on the objects both draw: those that appear after the load natively and never in the original (drawn throughout
# there), those whose nearest appearance is nearer natively than the original's nearest (beyond 2 %), and the appearance
# counts; plus the objects the original draws after the load and the native never does.
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
function Read-Rows([string]$run){
    $file=if(Test-Path -LiteralPath $run -PathType Leaf){ $run }else{ "$repo/evidence/I-0066/artifacts/$run/startup.jsonl" }   # a run name, or a journal file (the launcher's play-*.jsonl with -PopinObserve)
    if(-not (Test-Path -LiteralPath $file)){ throw "no journal for $run" }
    Select-String -LiteralPath $file -Pattern '"event":"popin_' | ForEach-Object { try { $_.Line | ConvertFrom-Json } catch {} }
}
function Key($r){ $c=$r.centre; '{0}|{1}|{2}|{3}' -f $r.model,[Math]::Round($c[0]/$Cell),[Math]::Round($c[1]/$Cell),[Math]::Round($c[2]/$Cell) }
function Summarise([string]$run){
    $rows=@(Read-Rows $run)
    $draw=@($rows | Where-Object { $_.event -in 'popin_native','popin_reference' })
    $native=[bool]($draw | Where-Object event -eq 'popin_native' | Select-Object -First 1)
    $clock=if($native){'serial'}else{'step'}
    $start=if($draw.Count){ ($draw | Where-Object on -eq 1 | ForEach-Object { [int64]$_.$clock } | Measure-Object -Minimum).Minimum }else{ 0 }
    $objects=@{};$reasons=@{};$nearList=[System.Collections.Generic.List[string]]::new()
    foreach($r in $draw){ $k=Key $r;$t=[int64]$r.$clock
        if(-not $objects.ContainsKey($k)){ $objects[$k]=[pscustomobject]@{key=$k;model=$r.model;open=$false;last_off=$null;off_d=$null;load=$false;appear=[System.Collections.Generic.List[double]]::new();gaps=0} }
        $o=$objects[$k]
        if($r.on -eq 1){
            if($null -ne $o.last_off -and ($t-$o.last_off) -le $Hold){ $o.last_off=$null }   # the hold: no gap
            elseif(-not $o.open){ if($t -lt $start+$LoadFrames){ $o.load=$true }else{ $o.appear.Add([double]$r.d);if($null -ne $o.last_off){ $o.gaps++ }
                if([double]$r.d -ge 0 -and [double]$r.d -lt $Near){ $nearList.Add(('{0} at {1:N1} m ({2} {3})' -f $r.model,[double]$r.d,$clock,$t)) } } }
            $o.open=$true }
        else{ $o.last_off=$t;$o.open=$false; if($native){ $reasons[$r.why]=1+[int]$reasons[$r.why] } } }
    $first=@($rows | Where-Object event -eq 'popin_first')
    $times=@($draw | ForEach-Object { [int64]$_.$clock })
    $frames=if($times.Count){ ($times | Measure-Object -Maximum).Maximum-$start+1 }else{ 0 }
    [pscustomobject]@{run=$run;renderer=$(if($native){'native'}else{'original'});frames=$frames;objects=$objects.Count
        appearances=(@($objects.Values | ForEach-Object { $_.appear.Count }) | Measure-Object -Sum).Sum
        reappearances=(@($objects.Values | ForEach-Object { $_.gaps }) | Measure-Object -Sum).Sum;reasons=$reasons;near=$nearList
        first_admissions=$first.Count;first_ms=[Math]::Round((@($first | ForEach-Object { [double]$_.ms }) | Measure-Object -Sum).Sum,1)
        first_after_load=@($first | Where-Object { [int64]$_.serial -ge $start+$LoadFrames }).Count
        first_after_load_ms=[Math]::Round((@($first | Where-Object { [int64]$_.serial -ge $start+$LoadFrames } | ForEach-Object { [double]$_.ms }) | Measure-Object -Sum).Sum,1)
        first_slowest=@($first | Where-Object { [int64]$_.serial -ge $start+$LoadFrames } | Sort-Object { -[double]$_.ms } | Select-Object -First $Top | ForEach-Object { '{0} {1:N1} ms (serial {2}, {3:N1} m)' -f $_.model,[double]$_.ms,$_.serial,[double]$_.d })
        rebuilt_ms=[Math]::Round((@($rows | Where-Object event -eq 'popin_rebuilt' | ForEach-Object { [double]$_.ms }) | Measure-Object -Sum).Sum,1)
        pieces=@($rows | Where-Object event -eq 'popin_native_gate');shadows=(Shadows $rows $clock);table=$objects}
}
# I-0100 tranche 2: the shadow casters by model (popin_native_shadow by entity, popin_reference_shadow by model): how many times a
# model starts casting (its first instance in) and its share of the run's frames casting.
function Shadows($rows,[string]$clock){
    $cast=@($rows | Where-Object { $_.event -in 'popin_native_shadow','popin_reference_shadow' -and $_.why -ne 'cascades' })   # a cascade change is no start
    $times=@($rows | Where-Object { $null -ne $_.$clock } | ForEach-Object { [int64]$_.$clock })
    if(-not $cast.Count -or -not $times.Count){ return @{} }
    $first=($times | Measure-Object -Minimum).Minimum;$last=($times | Measure-Object -Maximum).Maximum;$span=[Math]::Max(1,$last-$first)
    $m=@{}
    foreach($r in $cast){ $t=[int64]$r.$clock
        if(-not $m.ContainsKey($r.model)){ $m[$r.model]=[pscustomobject]@{starts=0;count=0;since=0L;frames=0L} }
        $o=$m[$r.model];if($r.on -eq 1){ if($o.count -eq 0){ $o.starts++;$o.since=$t };$o.count++ }elseif($o.count -gt 0){ $o.count--;if($o.count -eq 0){ $o.frames+=$t-$o.since } } }
    $out=@{};foreach($k in $m.Keys){ $o=$m[$k];if($o.count -gt 0){ $o.frames+=$last-$o.since };$out[$k]=[pscustomobject]@{starts=$o.starts;share=[Math]::Round(100.0*$o.frames/$span,1)} }
    $out
}
$summaries=@($Runs | ForEach-Object { Summarise $_ })
$original=if($Reference){ Summarise $Reference }else{ $null }
$report=[ordered]@{hold=$Hold;load_frames=$LoadFrames;runs=@();reference=$null}
foreach($s in @($summaries)+@($original | Where-Object { $_ })){
    $entry=[ordered]@{run=$s.run;renderer=$s.renderer;frames=$s.frames;objects=$s.objects;appearances_after_load=$s.appearances;reappearances=$s.reappearances
        stop_reasons=$s.reasons;first_admissions=$s.first_admissions;first_ms=$s.first_ms;first_after_load=$s.first_after_load;first_after_load_ms=$s.first_after_load_ms
        first_slowest_after_load=$s.first_slowest;rebuilt_ms=$s.rebuilt_ms;appearances_nearer_than=$Near;near_appearances=@($s.near)}
    if($original -and $s -ne $original){
        $both=@($s.table.Values | Where-Object { $original.table.ContainsKey($_.key) })
        $popsNativeOnly=@($both | Where-Object { $_.appear.Count -and -not $original.table[$_.key].appear.Count })
        $nearer=@($both | Where-Object { $_.appear.Count -and $original.table[$_.key].appear.Count } | ForEach-Object {
            $n=($_.appear | Measure-Object -Minimum).Minimum;$o=($original.table[$_.key].appear | Measure-Object -Minimum).Minimum
            [pscustomobject]@{model=$_.model;native=$n;original=$o} } | Where-Object { $_.native -lt 0.98*$_.original })
        $neverNative=@($original.table.Values | Where-Object { -not $s.table.ContainsKey($_.key) })
        $entry.objects_in_both=$both.Count
        $entry.appear_natively_only=$popsNativeOnly.Count
        $entry.appear_natively_only_examples=@($popsNativeOnly | Sort-Object { ($_.appear | Measure-Object -Minimum).Minimum } | Select-Object -First $Top | ForEach-Object { '{0} at {1:N1} m (x{2})' -f $_.model,($_.appear | Measure-Object -Minimum).Minimum,$_.appear.Count })
        $entry.appear_nearer_natively=$nearer.Count
        $entry.appear_nearer_examples=@($nearer | Sort-Object { $_.native-$_.original } | Select-Object -First $Top | ForEach-Object { '{0}: native {1:N1} m, original {2:N1} m' -f $_.model,$_.native,$_.original })
        $entry.appearances_both_native=(@($both | ForEach-Object { $_.appear.Count }) | Measure-Object -Sum).Sum
        $entry.appearances_both_original=(@($both | ForEach-Object { $original.table[$_.key].appear.Count }) | Measure-Object -Sum).Sum
        $entry.never_drawn_natively=$neverNative.Count
        $entry.never_drawn_natively_examples=@($neverNative | Select-Object -First $Top | ForEach-Object { '{0}{1}' -f $_.model,$(if($_.appear.Count){' at {0:N1} m' -f ($_.appear | Measure-Object -Minimum).Minimum}else{' (at the load)'}) })
        # I-0100 tranche 2 (popin_native_gate): for each model the native never draws, the gates its entities met in the native
        # main walk (changes by gate, the nearest distance at each), or « never visited » (its tree node culled or masked).
        if($s.pieces.Count){ $entry.never_drawn_gates=@($neverNative | ForEach-Object model | Sort-Object -Unique | Select-Object -First $Top | ForEach-Object { $m=$_
            $hits=@($s.pieces | Where-Object { ($_.models -split ' ') -contains $m })
            if(-not $hits.Count){ '{0}: never visited' -f $m }
            else{ '{0}: {1}' -f $m,((@($hits | Group-Object gate | ForEach-Object { '{0} x{1} (nearest {2:N0} m)' -f $_.Name,$_.Count,(@($_.Group | ForEach-Object { [double]$_.d }) | Measure-Object -Minimum).Minimum })) -join ', ') } }) }
        # The shadow casters side by side: per model, its starts and its share of frames casting in each renderer.
        if($s.shadows.Count -or $original.shadows.Count){ $entry.shadow_casters=@(@($s.shadows.Keys)+@($original.shadows.Keys) | Sort-Object -Unique | ForEach-Object {
            $n=$s.shadows[$_];$o=$original.shadows[$_]
            '{0}: native {1}, original {2}' -f $_,$(if($n){'{0} starts {1} %' -f $n.starts,$n.share}else{'never'}),$(if($o){'{0} starts {1} %' -f $o.starts,$o.share}else{'never'}) }) } }
    if($s -eq $original){ $report.reference=$entry }else{ $report.runs+=$entry } }
$json=$report | ConvertTo-Json -Depth 6
if($Out){ Set-Content -LiteralPath $Out -Value $json -Encoding utf8 }
$json
