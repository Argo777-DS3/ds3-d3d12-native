param([Parameter(Mandatory)][string[]]$Run,[string]$PathFile='')
# I-0097 tranche 5 study: a replayed run split by the path's cycles (each segment's cycles of scripts/replay/*.path.json laid
# from motion_start, as Invoke-ReplayPath.ps1 plays them), untrimmed: frames over 25 and 34 ms at the frame end
# (native_frame_times or native_original_frames), game steps over 25 ms (game_frame_times), and the native texture and model
# storage admissions (native_encoded_texture_resident, native_model_storage_admitted), each placed at the tick of the timed row
# before it. Means over the runs given. Reads the journal only.
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if(-not $PathFile){ $PathFile="$repo/scripts/replay/firelink.path.json" }
$recording=Get-Content -LiteralPath $PathFile -Raw | ConvertFrom-Json
$Run=@($Run | ForEach-Object { $_ -split ',' } | Where-Object { $_ })   # pwsh -File passes a comma list as one string (i97-t4s1)
$tot=[ordered]@{}
foreach($n in $Run){
    $d=if(Test-Path -LiteralPath $n){ $n }else{ "$repo/evidence/I-0066/artifacts/$n" };$marks=@{}
    foreach($l in Get-Content -LiteralPath "$d/motion-marks.txt"){ $f=$l -split ' ';$marks[$f[0]]=[int64]$f[1] }
    $at=[double]$marks['motion_start'];$bounds=@()
    foreach($s in $recording.segments){ for($k=0;$k -lt [int]$s.cycles;$k++){ $bounds+=,@("$($s.name)$($k+1)",($at+$k*[double]$s.cycle_ms),($at+($k+1)*[double]$s.cycle_ms)) };$at+=[int]$s.cycles*[double]$s.cycle_ms }
    $fe=[Collections.Generic.List[double[]]]::new();$gs=[Collections.Generic.List[double[]]]::new();$adm=[Collections.Generic.List[double[]]]::new();$last=$null
    foreach($line in [IO.File]::ReadLines("$d/startup.jsonl")){
        if($line -match '"event":"(native_frame_times|native_original_frames|game_frame_times)"'){
            $r=$line | ConvertFrom-Json;if($null -eq $r.tick_ms){continue};$last=[double]$r.tick_ms
            $isGs=$r.event -eq 'game_frame_times';$end=$last
            for($i=$r.ms.Count-1;$i -ge 0;$i--){ if($isGs){$gs.Add(@($end,[double]$r.ms[$i]))}else{$fe.Add(@($end,[double]$r.ms[$i]))};$end-=[double]$r.ms[$i] } }
        elseif($null -ne $last -and $line -match '"event":"native_encoded_texture_resident"'){ $adm.Add(@($last,0)) }
        elseif($null -ne $last -and $line -match '"event":"native_model_storage_admitted"'){ $adm.Add(@($last,1)) } }
    foreach($b in $bounds){ $a=$b[1];$z=$b[2]
        if(-not $tot.Contains($b[0])){ $tot[$b[0]]=[double[]]::new(5) };$s=$tot[$b[0]]
        foreach($x in $fe){ if($x[0] -ge $a -and $x[0] -lt $z){ if($x[1] -gt 25){$s[0]++};if($x[1] -gt 34){$s[1]++} } }
        foreach($x in $gs){ if($x[0] -ge $a -and $x[0] -lt $z -and $x[1] -gt 25){$s[2]++} }
        foreach($x in $adm){ if($x[0] -ge $a -and $x[0] -lt $z){ if($x[1] -eq 0){$s[3]++}else{$s[4]++} } } } }
$k=$Run.Count
"runs=$k (means a run): cycle frame_end>25 frame_end>34 game_step>25 textures storages"
foreach($e in $tot.GetEnumerator()){ $s=$e.Value;[string]::Format([Globalization.CultureInfo]::InvariantCulture,'{0,-9} {1,6:F1} {2,6:F1} {3,6:F1} {4,8:F1} {5,8:F1}',$e.Key,($s[0]/$k),($s[1]/$k),($s[2]/$k),($s[3]/$k),($s[4]/$k)) }
