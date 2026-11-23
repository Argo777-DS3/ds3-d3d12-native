# I-0113 tranche 21 (gap 21): the original's draws of one model through a warp's fade, from a Reference journal with
# decal-observe and light-observe (Run-I113Warp.ps1 -Runs original): every named_oracle row of -Model in journal order, with the
# last reference_row_fade_inputs opacity of that model seen before it (the part's fade at that point of the journal), as one line
# a row: n, the time from the first row (ms), the PS's DXBC checksum, the blend of target 0 (enable, src, dest, op), alpha to
# coverage, the depth (enable, write, func), the cull mode and the index count. -Csv writes them; the console gets each
# signature's first row (with its opacity; the rows come from several threads, so the journal's order is only roughly the
# draws'), and the counts per signature with their lowest opacity.
param([Parameter(Mandatory)][string]$Journal,[string]$Model='HR_A_0101',[string]$Csv='')
$ErrorActionPreference='Stop'
$culture=[Threading.Thread]::CurrentThread.CurrentCulture;[Threading.Thread]::CurrentThread.CurrentCulture=[Globalization.CultureInfo]::InvariantCulture   # the CSV's decimals with a point (the final review's L4), put back at the end
$opacity=$null;$t0=$null;$rows=[System.Collections.Generic.List[object]]::new()
foreach($line in [IO.File]::ReadLines((Resolve-Path -LiteralPath $Journal))){
    if($line.Contains('"event":"reference_row_fade_inputs"') -and $line.Contains("""model"":""$Model""")){$j=$line|ConvertFrom-Json;$opacity=$j.opacity;continue}
    if(-not $line.Contains('"event":"named_oracle"') -or -not $line.Contains("""model"":""$Model""")){continue}
    $j=$line|ConvertFrom-Json;if($null -eq $t0){$t0=[int64]$j.us}
    $rt=if($j.blend){$j.blend.rt[0]}else{$null};$d=$j.depth
    $rows.Add([pscustomobject]@{n=$j.n;ms=[math]::Round(([int64]$j.us-$t0)/1000.0,1);opacity=$opacity;ps=$j.ps_dxbc;count=$j.count
        blend=$(if($rt){"$($rt[0]):$($rt[1])/$($rt[2])/$($rt[3])"}else{'none'});a2c=$(if($j.blend){$j.blend.a2c}else{''})
        depth=$(if($d){"$($d[0]):$($d[1]):$($d[2])"}else{'none'});cull=$(if($j.raster){$j.raster[1]}else{''})})}
if($Csv){$rows|Export-Csv -LiteralPath $Csv -NoTypeInformation -Encoding utf8}
"rows $($rows.Count) of $Model; first ms 0, last ms $(if($rows.Count){$rows[-1].ms})"
$seen=@{}
foreach($r in $rows){$sig="$($r.ps) $($r.blend) a2c$($r.a2c) $($r.depth) c$($r.cull) i$($r.count)";if(-not $seen.ContainsKey($sig)){$seen[$sig]=1;"first n=$($r.n) ms=$($r.ms) opacity=$($r.opacity) $sig"}}
''
$rows|Group-Object {"$($_.ps) $($_.blend) a2c$($_.a2c) $($_.depth) c$($_.cull) i$($_.count)"}|Sort-Object Count -Descending|ForEach-Object{
    $o=@($_.Group|Where-Object{$null -ne $_.opacity}|ForEach-Object opacity);$lo=if($o.Count){($o|Measure-Object -Minimum).Minimum}else{''}
    "{0,5}  {1}  opacity min {2}" -f $_.Count,$_.Name,$lo}
[Threading.Thread]::CurrentThread.CurrentCulture=$culture
