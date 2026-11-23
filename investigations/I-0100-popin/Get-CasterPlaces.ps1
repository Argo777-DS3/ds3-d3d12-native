param([Parameter(Mandatory)][string]$Original,[Parameter(Mandatory)][string[]]$Native,[string]$Out='')
# I-0100 tranche 5, tightened in tranche 6 (the final review's M1 of tranche 5): the sun casters by model, the original against
# native runs, on one replayed path, as one measure on both sides: per model, the share of the path's steps (original) or frames
# (native) in which at least one of its instances is drawn into at least one sun cascade, at most 1.
# The original's run (Reference, popin-observe and caster-place.flag) gives reference_sun_casters rows (every 600 game steps, per
# model, the steps it was drawn in any of the sort-key passes 5..8, all instances and grouped draws as one; build t6a on). A
# native run (popin-observe) gives popin_native_shadow rows (an entity starting or stopping to cast, with the cascades its rows
# draw after the per-mesh no-cast refusal since build t6a); per model the union of its entities' casting intervals.
# An original journal without reference_sun_casters rows (before t6a) falls back to tranche 5's figure (per window the most-used
# cascade's steps, summed over the instance argument: a share over 1 is several instances), reported as `original_measure`.
# Runs: a run name (evidence/I-0066/artifacts/<run>/startup.jsonl) or a journal file.
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
function Journal([string]$run){ if(Test-Path -LiteralPath $run -PathType Leaf){ $run }else{ "$repo/evidence/I-0066/artifacts/$run/startup.jsonl" } }
$byModel=@{};$windows=@{};$measure='presence'
foreach($line in Select-String -LiteralPath (Journal $Original) -Pattern '"event":"reference_sun_casters"'){
    $r=$line.Line | ConvertFrom-Json;$windows[$r.step]=1;$byModel[$r.model]+=$r.steps }
if(-not $byModel.Count){ $measure='most_used_cascade';$perPass=@{}
    foreach($line in Select-String -LiteralPath (Journal $Original) -Pattern '"event":"reference_caster_places"'){
        $r=$line.Line | ConvertFrom-Json;$windows[$r.step]=1
        if($r.pass -lt 5 -or $r.pass -gt 8){ continue }
        $perPass["$($r.step)|$($r.pass)|$($r.model)"]+=$r.steps }
    $best=@{};foreach($k in $perPass.Keys){ $p=$k.Split('|');$wm="$($p[0])|$($p[2])";$best[$wm]=[Math]::Max([double]$best[$wm],$perPass[$k]) }
    foreach($k in $best.Keys){ $byModel[$k.Split('|',2)[1]]+=$best[$k] } }
# The caster-place windows also hold every other pass, so a window without a sun caster still counts (the player always casts).
foreach($line in Select-String -LiteralPath (Journal $Original) -Pattern '"event":"reference_caster_places"'){ $windows[($line.Line | ConvertFrom-Json).step]=1 }
$total=600*[Math]::Max(1,$windows.Count)
$columns=[ordered]@{};foreach($run in $Native){
    $rows=@(Select-String -LiteralPath (Journal $run) -Pattern '"event":"popin_native_shadow"' | ForEach-Object { $_.Line | ConvertFrom-Json })
    $first=$rows[0].serial;$last=$rows[-1].serial;$open=@{};$spans=@{}
    foreach($r in $rows){ $id="$($r.entity)|$($r.model)"
        if($open.ContainsKey($id)){ $spans[$r.model]+=,@($open[$id],$r.serial);$open.Remove($id) }
        if($r.on -eq 1 -and $r.cascades){ $open[$id]=$r.serial } }
    foreach($id in $open.Keys){ $spans[$id.Split('|')[1]]+=,@($open[$id],$last) }
    $share=@{};foreach($m in $spans.Keys){ $covered=0.0;$end=[double]::MinValue
        foreach($s in @($spans[$m] | Sort-Object { $_[0] })){ $a=[Math]::Max([double]$s[0],$end);if($s[1] -gt $a){ $covered+=$s[1]-$a };$end=[Math]::Max($end,[double]$s[1]) }
        $share[$m]=[Math]::Round($covered/[Math]::Max(1,$last-$first),3) }
    $columns[$run]=$share }
$models=@($byModel.Keys)+@($columns.Values | ForEach-Object { $_.Keys }) | Sort-Object -Unique
$table=foreach($m in $models){ $row=[ordered]@{model=$m;original=[Math]::Round([double]$byModel[$m]/$total,3)}
    foreach($run in $columns.Keys){ $row[$run]=[double]$columns[$run][$m] };[pscustomobject]$row }
if($Out){ [ordered]@{original_steps=$total;original_measure=$measure;models=$table} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $Out -Encoding utf8 }
"original steps $total ($measure)"
$table | Format-Table -AutoSize
