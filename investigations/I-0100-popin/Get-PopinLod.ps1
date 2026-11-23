param([Parameter(Mandatory)][string[]]$Runs,[string]$Reference='',[double]$LoadGapMs=3000,[int]$LoadRows=200,[double]$Tolerance=0.05,[int]$Top=12,[string]$Out='')
# I-0100 tranche 4 (Thomas's theory, 2026-10-26: the native's draw distance too low and wrong; the LOD threshold nearer after a
# death): reads the popin_lod and popin_lod_scale rows of popin-observe.flag (DS3's LOD step D1CCE0, both renderers) from runs
# (evidence/I-0066/artifacts/<run>/startup.jsonl) or journal files (the launcher's play-*.jsonl with -PopinObserve).
# Loads: a load is a burst of at least -LoadRows controllers at their first step (first=1) within -LoadGapMs of each other; the
# launch into the zone is the first, each death's or travel's reload the next. Per run and load: the manager's scale and biases
# seen, the controllers, and by model the distances DS3 compared (d: metres x scale, m: metres) when it moved a controller's LOD
# up (0 to 1, 1 to 2) and when the geometry weight left 1 (the draw distance): their median, with the thresholds DS3 computed
# (fade_at, draw_at, in d). With -Reference <run>: each native load against the original's load of the same rank, by model and
# transition: the medians' ratio (native / original), the models beyond -Tolerance listed.
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
function Read-Rows([string]$run){
    $file=if(Test-Path -LiteralPath $run -PathType Leaf){ $run }else{ "$repo/evidence/I-0066/artifacts/$run/startup.jsonl" }
    if(-not (Test-Path -LiteralPath $file)){ throw "no journal for $run" }
    # A controller before its first centre has none (-3.4e38): its distances print as inf, and the LOD gate as nan; read as null.
    Select-String -LiteralPath $file -Pattern '"event":"popin_lod' | ForEach-Object { try { ($_.Line -replace '(?<=[:\[,])-?(inf|nan)(\([a-z]+\))?','null') | ConvertFrom-Json } catch {} }
}
function Median([double[]]$v){ if(-not $v.Count){ return $null };$s=@($v | Sort-Object);$n=$s.Count;if($n%2){ $s[($n-1)/2] }else{ ($s[$n/2-1]+$s[$n/2])/2 } }
function Summarise([string]$run){
    $rows=@(Read-Rows $run);$lod=@($rows | Where-Object event -eq 'popin_lod');$scales=@($rows | Where-Object event -eq 'popin_lod_scale')
    # Loads from the first-step bursts.
    $starts=[System.Collections.Generic.List[double]]::new();$burst=@();$last=-1e9
    foreach($r in ($lod | Where-Object first -eq 1)){ $t=[double]$r.ms
        if($t-$last -gt $LoadGapMs){ if($burst.Count -ge $LoadRows){ $starts.Add($burst[0]) };$burst=@() };$burst+=$t;$last=$t }
    if($burst.Count -ge $LoadRows){ $starts.Add($burst[0]) }
    # i100-t4a-al-n1/-o1: controllers start all along a session (characters, streaming), so the bursts merge; the manager's scale
    # changes at a reload (0.756695 at the launch, 1 after the death in both renderers): its rows split the session when present.
    if($scales.Count -gt 1){ $starts=[System.Collections.Generic.List[double]]::new();foreach($c in $scales){ $starts.Add([double]$c.ms) } }
    $loadOf={ param($t) $k=-1;for($i=0;$i -lt $starts.Count;$i++){ if($t -ge $starts[$i]){ $k=$i } };$k }
    $loads=@{}
    foreach($r in $lod){ $k=& $loadOf ([double]$r.ms);if($k -lt 0){ continue }
        if(-not $loads.ContainsKey($k)){ $loads[$k]=@{controllers=[System.Collections.Generic.HashSet[string]]::new();moves=@{};thresholds=@{};state=@{}} }
        $L=$loads[$k];[void]$L.controllers.Add($r.record)
        # Each controller's last state in the load, keyed by its model and centre (1 m): its LOD and its draw (geometry) weight.
        if($null -ne $r.centre[0] -and [Math]::Abs([double]$r.centre[0]) -lt 1e6){ $L.state[('{0}|{1}|{2}|{3}' -f $r.model,[Math]::Round([double]$r.centre[0]),[Math]::Round([double]$r.centre[1]),[Math]::Round([double]$r.centre[2]))]=@([int]$r.lod[0],[double]$r.w[1]) }
        $what=@()
        if([int]$r.was -ge 0 -and [int]$r.lod[0] -gt [int]$r.was){ $what+="lod$($r.was)to$($r.lod[0])" }
        if([int]$r.was_w[1] -eq 2 -and [double]$r.w[1] -lt 1){ $what+='draw_out' }
        foreach($w in $what){ $key="$($r.model)|$w";if(-not $L.moves.ContainsKey($key)){ $L.moves[$key]=[System.Collections.Generic.List[double]]::new();$L.thresholds[$key]=('fade_at {0:N1} draw_at {1:N1} scale {2}' -f [double]$r.fade_at,[double]$r.draw_at,$r.scale) }
            if($null -ne $r.d){ $L.moves[$key].Add([double]$r.d) } } }
    $result=[ordered]@{run=$run;rows=$lod.Count;scale_rows=@($scales | ForEach-Object { [ordered]@{ms=$_.ms;scale=$_.scale;bias=$_.bias;lod_bias=$_.lod_bias;lod_gate=$(if($null -ne $_.lod_gate_bits){ $_.lod_gate_bits }else{ $_.lod_gate })} });
        loads=@()}
    foreach($k in ($loads.Keys | Sort-Object)){ $L=$loads[$k]
        $medians=[ordered]@{};foreach($key in ($L.moves.Keys | Sort-Object)){ $medians[$key]=[ordered]@{n=$L.moves[$key].Count;median_d=[Math]::Round((Median $L.moves[$key].ToArray()),2);dds=$L.thresholds[$key]} }
        $histogram=[ordered]@{};foreach($v in $L.state.Values){ $h="lod$($v[0])";$histogram[$h]=1+[int]$histogram[$h] }
        $result.loads+=[ordered]@{load=$k;start_ms=$starts[$k];controllers=$L.controllers.Count;moves=$medians;lods=$histogram;state=$L.state} }
    $result
}
$summaries=@($Runs | ForEach-Object { Summarise $_ })
foreach($s in $summaries){
    Write-Host ("== {0}: {1} rows, {2} loads" -f $s.run,$s.rows,$s.loads.Count)
    foreach($c in $s.scale_rows){ Write-Host ("   scale {0} at {1} ms, bias [{2}], lod_bias {3}, lod_gate {4}" -f $c.scale,$c.ms,($c.bias -join ', '),$c.lod_bias,$c.lod_gate) }
    foreach($l in $s.loads){ $up=@($l.moves.Keys | Where-Object { $_ -like '*|lod*' }).Count;$out=@($l.moves.Keys | Where-Object { $_ -like '*|draw_out' }).Count
        Write-Host ("   load {0} at {1:N0} ms: {2} controllers, {3} model LOD moves, {4} model draw-distance exits; last LODs {5}" -f $l.load,$l.start_ms,$l.controllers,$up,$out,(($l.lods.GetEnumerator() | ForEach-Object { "$($_.Key) $($_.Value)" }) -join ', ')) } }
$compare=@()
if($Reference){ $ref=Summarise $Reference
    Write-Host ("== reference {0}: {1} loads" -f $Reference,$ref.loads.Count)
    foreach($c in $ref.scale_rows){ Write-Host ("   scale {0} at {1} ms, bias [{2}], lod_bias {3}, lod_gate {4}" -f $c.scale,$c.ms,($c.bias -join ', '),$c.lod_bias,$c.lod_gate) }
    foreach($s in $summaries){ for($i=0;$i -lt [Math]::Min($s.loads.Count,$ref.loads.Count);$i++){
        $n=$s.loads[$i].moves;$o=$ref.loads[$i].moves;$ratios=[System.Collections.Generic.List[double]]::new();$off=@()
        foreach($key in $n.Keys){ if(-not $o.Contains($key)){ continue };$a=[double]$n[$key].median_d;$b=[double]$o[$key].median_d;if($b -le 0){ continue }
            $q=$a/$b;$ratios.Add($q);if([Math]::Abs($q-1) -gt $Tolerance){ $off+=[ordered]@{key=$key;native=$a;original=$b;ratio=[Math]::Round($q,3);native_n=$n[$key].n;original_n=$o[$key].n} } }
        $line=[ordered]@{run=$s.run;load=$i;common=$ratios.Count;median_ratio=if($ratios.Count){ [Math]::Round((Median $ratios.ToArray()),3) }else{ $null };beyond=$off.Count;
            only_native=@($n.Keys | Where-Object { -not $o.Contains($_) }).Count;only_original=@($o.Keys | Where-Object { -not $n.Contains($_) }).Count;
            listed=@($off | Sort-Object { [Math]::Abs($_.ratio-1) } -Descending | Select-Object -First $Top)}
        # The controllers' last states compared by key: the same LOD and the same draw weight band in both renderers.
        $ns=$s.loads[$i].state;$os=$ref.loads[$i].state;$both=0;$same=0;$diff=@()
        foreach($key in $ns.Keys){ if(-not $os.ContainsKey($key)){ continue };$both++;$a=$ns[$key];$b=$os[$key]
            if($a[0] -eq $b[0] -and ([Math]::Abs($a[1]-$b[1]) -lt 0.01)){ $same++ }else{ $diff+="$key native lod $($a[0]) w $($a[1]) original lod $($b[0]) w $($b[1])" } }
        $line.states=[ordered]@{both=$both;same=$same;differ=$diff.Count;listed=@($diff | Select-Object -First $Top)}
        Write-Host ("   last states: {0} controllers in both, {1} the same LOD and draw weight, {2} differ" -f $both,$same,$diff.Count);foreach($x in $line.states.listed){ Write-Host "      $x" }
        $compare+=$line
        Write-Host ("   {0} load {1} against the original's: {2} common moves, median ratio {3}, {4} beyond {5:P0}, {6} native only, {7} original only" -f $s.run,$i,$line.common,$line.median_ratio,$line.beyond,$Tolerance,$line.only_native,$line.only_original)
        foreach($x in $line.listed){ Write-Host ("      {0}: native {1} original {2} ({3})" -f $x.key,$x.native,$x.original,$x.ratio) } } } }
if($Out){ foreach($x in $summaries){ foreach($l in $x.loads){ $l.Remove('state') } };[ordered]@{runs=$summaries;reference=$Reference;compare=$compare} | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $Out -Encoding utf8 }
