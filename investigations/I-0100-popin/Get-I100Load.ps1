param([Parameter(Mandatory)][string[]]$Run,[int]$Frames=12,[switch]$Json)
# I-0100 tranche 8 (POPIN-STUDY M2): each load's end in a run with popin-observe.flag, from its journal. Native: popin_page_close
# (the loading page's window asked to close) and popin_load_frame (one row a main prepare around a load): the first world frame
# after the close, the rows refused for a texture not resident (missing_textures) frame by frame, the frame they reach the window's
# floor, DS3's texture creations after the close. Reference (original-probe): popin_reference_window (one row a game step), its
# emitted entities and texture creations after the close; rows within 10 s of the close. Prints one block a load (the quit to the
# title closes the same window: its block has no frame); -Json returns the objects.
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$out=[Collections.Generic.List[object]]::new()
foreach($name in $Run){
    $file="$repo/evidence/I-0066/artifacts/$name/startup.jsonl"
    if(-not (Test-Path -LiteralPath $file)){ throw "no journal for $name" }
    $loads=[Collections.Generic.List[object]]::new();$current=$null;$native=$false
    foreach($l in [IO.File]::ReadLines($file)){
        if($l.StartsWith('{"event":"popin_page_close"')){ $r=$l|ConvertFrom-Json
            $current=[pscustomobject]@{run=$name;load=$loads.Count+1;close_tick=$r.tick;textures_at_close=$r.textures;frames=[Collections.Generic.List[object]]::new()};$loads.Add($current);continue }
        if(-not $current){ continue }
        if($l.StartsWith('{"event":"popin_load_frame"')){ $native=$true;$r=$l|ConvertFrom-Json;if($r.page_ms -ge 0 -and $r.page_ms -lt 10000 -and $current.frames.Count -lt 600){ $current.frames.Add($r) } }
        elseif($l.StartsWith('{"event":"popin_reference_window"')){ $r=$l|ConvertFrom-Json;if($r.page_ms -ge 0 -and $r.page_ms -lt 10000 -and $current.frames.Count -lt 600){ $current.frames.Add($r) } } }
    foreach($ld in $loads){ $f=$ld.frames
        if(-not $f.Count){ "$name load $($ld.load): no frame after the close";continue }
        if($native){
            $world=@($f | Where-Object { $_.rows -gt 0 })
            $floor=($f | Select-Object -Last 100 | Measure-Object missing_textures -Minimum).Minimum
            $settled=$world | Where-Object { $_.missing_textures -le $floor } | Select-Object -First 1
            $o=[pscustomobject]@{run=$name;load=$ld.load;renderer='native';first_world_ms=$(if($world){$world[0].page_ms});first_world_draws=$(if($world){$world[0].draws});
                first_world_drawn=$(if($world){$world[0].drawn});missing_peak=($world | Measure-Object missing_textures -Maximum).Maximum;missing_floor=$floor;
                settled_ms=$(if($settled){$settled.page_ms});settled_frames=$(if($settled){[array]::IndexOf($world,$settled)});
                textures_after_close=$f[-1].textures-$ld.textures_at_close;
                first=@($world | Select-Object -First $Frames | ForEach-Object { "$([math]::Round($_.page_ms)):$($_.draws)/$($_.missing_textures)/$($_.drawn)/$($_.not_ready)" }) -join ' '} }
        else{
            $world=@($f | Where-Object { $_.emits -gt 0 })
            $steady=($f | Select-Object -Last 100 | Measure-Object emits -Average).Average
            $o=[pscustomobject]@{run=$name;load=$ld.load;renderer='original';first_world_ms=$(if($world){$world[0].page_ms});first_world_emits=$(if($world){$world[0].emits});
                steady_emits=[math]::Round($steady);textures_after_close=$f[-1].textures-$ld.textures_at_close;
                first=@($world | Select-Object -First $Frames | ForEach-Object { "$([math]::Round($_.page_ms)):$($_.emits)" }) -join ' '} }
        $out.Add($o) } }
if($Json){ $out }else{ $out | Format-List | Out-String -Width 220 }
