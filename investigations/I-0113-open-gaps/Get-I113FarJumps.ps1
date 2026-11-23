# I-0113 tranche 21 (gap 22, the Irithyll bridge): the far scene's jumps in a filmed visit (Run-I113Barrier.ps1's barrier.mp4). The
# study thread's reading of i113-t21-bridge2-native: everything past the railing right of the pillar jumps at once about every
# 61 frames, the original never. Per frame, ffmpeg's signalstats YDIF (the mean luma change from the frame before) over the far
# region (-Far, crop w:h:x:y in the film's pixels) and over a near region (-Near: the character's floor); a jump is a frame whose
# far change exceeds -Ratio times the far region's median change while the near one stays under its own median times 2. Prints
# the jumps' frames, their spacing and their count, and how many frames change at all (a continuous animation changes most frames,
# a frozen one with jumps few); -Seconds from -Start.
param([Parameter(Mandatory)][string]$Video,[string]$Far='440:150:820:80',[string]$Near='300:120:300:600',[double]$Start=2,[double]$Seconds=30,[double]$Ratio=5)
$ErrorActionPreference='Stop'
$ff=(Get-Command ffmpeg -ErrorAction SilentlyContinue).Source
if(-not $ff){$ff=(Get-ChildItem "$env:LOCALAPPDATA/Microsoft/WinGet/Packages" -Recurse -Filter ffmpeg.exe -ErrorAction SilentlyContinue | Select-Object -First 1).FullName}
if(-not $ff){throw 'no ffmpeg'}
$video=(Resolve-Path -LiteralPath $Video).Path
$work=Join-Path ([IO.Path]::GetTempPath()) "i113-farjumps-$PID";New-Item -ItemType Directory -Force $work | Out-Null
Push-Location $work   # the metadata filter's file= option takes a bare name (a drive's colon breaks its parsing)
try{
    $series=@{}
    foreach($r in @(@('far',$Far),@('near',$Near))){
        & $ff -hide_banner -loglevel error -y -ss $Start -t $Seconds -i $video -vf "crop=$($r[1]),signalstats,metadata=print:key=lavfi.signalstats.YDIF:file=$($r[0]).txt" -f null -
        if($LASTEXITCODE){throw "ffmpeg failed on the $($r[0]) region ($($r[1])): a crop outside the film?"}
        $series[$r[0]]=@(Select-String -Path "$($r[0]).txt" -Pattern 'YDIF=([0-9.]+)' | ForEach-Object {[double]$_.Matches[0].Groups[1].Value})}
}finally{Pop-Location;Remove-Item -LiteralPath $work -Recurse -Force -ErrorAction SilentlyContinue}
$f=$series.far;$n=$series.near
$median={param($v)$s=@($v|Sort-Object);if($s.Count){$s[[int]($s.Count/2)]}else{0}}
$mf=[math]::Max(0.05,(& $median $f));$mn=[math]::Max(0.05,(& $median $n))
$jumps=@(for($i=1;$i -lt $f.Count;$i++){if($f[$i] -gt $Ratio*$mf -and $n[$i] -lt 2*$mn -and $f[$i] -gt $f[$i-1]){$i}})
$gaps=@(for($k=1;$k -lt $jumps.Count;$k++){$jumps[$k]-$jumps[$k-1]})
"frames $($f.Count); far median $([math]::Round($mf,2)), near median $([math]::Round($mn,2)); jumps $($jumps.Count)"
"jump frames (counted from -Start, $Start s; at 60 frames/s add $([int]($Start*60)) for the film's own): $($jumps -join ' ')"
"spacing: $($gaps -join ' ')"
"far frames changing (YDIF over 0.5, the continuous animation): $(@($f | Where-Object {$_ -gt 0.5}).Count); over 3 (jumps): $(@($f | Where-Object {$_ -gt 3}).Count)"
