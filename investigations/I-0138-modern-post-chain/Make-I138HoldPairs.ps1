# I-0138 tranche 8g (D-0299): the native's and the original's cutscene shots taken at the same scene frame (Run-AlphaCast
# -CutsceneHolds: cutscene-hold-<k>.png in each run, the module's witness_cutscene_hold rows in startup.jsonl). A pair is kept only
# when both runs' hold rows name the same scene id, target, cut and frame set, a release whose frame equals it (drift 0) made on the
# runner's « shot taken » file naming that hold, and the runner's shot (cutscene-holds.tsv, .NET's TickCount64, not the
# module's GetTickCount64: two time sources, OBSERVED 2026-11-22 with pwsh 7.7 / .NET 11 on this PC: TickCount64 equals
# QueryUnbiasedInterruptTime to the ms, GetTickCount64 4 to 14 ms behind it, its 15.6 ms step, evidence/I-0138/t8g2-clock-sources.txt;
# the unbiased one leaves out sleep time) between the hold's tick and the release's in both runs. Since `9c89c0914` the module releases a hold only on the file naming
# its index (the release row does not name it; the t8g2 pairs come from an earlier build, released on any shot file, and rest on
# the tick test), so the order is then guaranteed for a « shot » release and the tick test is a sanity check (a shot ending up to 32 ms after the release's tick counts as
# inside: 8 ms in tranche 8g's run of 04:05; after a sleep of the PC the shots' ticks fall behind by the time slept: past the shot delay
# (about 2.5 s) every shot reads before its hold and every pair is refused; below it the end test only loosens, silently);
# any other pair is listed as not compared. The shot's offset from the hold's start is given for both runs:
# the Remo clock is held, the world's own simulation (effects, cloth, the exposure's adaptation) is not (step review M3). For each kept pair: the capture's mean luma below the title bar (ffmpeg signalstats YAVG, of 255) in both
# modes, and a sheet (the native above the original, each shot halved) into -Out/<label>-hold-<k>.png; a table -Out/<label>.tsv.
# Pictures are JPEG under a .png name (-c:v mjpeg).
param([Parameter(Mandatory)][string]$Native,[Parameter(Mandatory)][string]$Original,[Parameter(Mandatory)][string]$Out,[Parameter(Mandatory)][string]$Label)
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$ff=Get-ChildItem "$env:LOCALAPPDATA/Microsoft/WinGet/Packages" -Recurse -Filter ffmpeg.exe | Select-Object -First 1 -ExpandProperty FullName
if(-not $ff){throw 'no ffmpeg'}
New-Item -ItemType Directory -Force -Path $Out | Out-Null
function Get-Holds([string]$run){ $h=@{}
    foreach($l in @(Select-String -LiteralPath "$repo/evidence/I-0066/artifacts/$run/startup.jsonl" -Pattern '"event":"witness_cutscene_hold"')){ $r=$l.Line | ConvertFrom-Json
        if(-not $h.ContainsKey($r.index)){ $h[$r.index]=@{} }; $h[$r.index][$r.phase]=$r }
    $s=@{};$tsv="$repo/evidence/I-0066/artifacts/$run/cutscene-holds.tsv"
    if(Test-Path -LiteralPath $tsv){ foreach($l in @(Get-Content -LiteralPath $tsv)){ $f=$l -split "`t"; if($f.Count -eq 3){ $s[[int64]$f[0]]=@([int64]$f[1],[int64]$f[2]) } } }
    foreach($k in @($h.Keys)){ $h[$k]['shot']=$s[[int64]$k] }
    $h }
# I-0141 tranche 24 (its final review's H1): the luma leaves out the window's title bar, rows 0 to 30 of the 1920x1080 capture, cropped from row 32
# (about 35 natively and 235 in the original, OBSERVED on every hold of i141-t23a, t24b and t24c: about 4 of 255 added to the
# original's whole-capture mean, 1.5 taken from the native's, the difference moved by about 5.8); the sheets keep the whole capture.
function Get-Luma([string]$path){ $text=& $ff -hide_banner -c:v mjpeg -i $path -vf 'crop=iw:ih-32:0:32,signalstats,metadata=print:key=lavfi.signalstats.YAVG' -f null - 2>&1 | Out-String
    if($text -match 'lavfi\.signalstats\.YAVG=([0-9.]+)'){ [double]$Matches[1] }else{ $null } }
$hn=Get-Holds $Native;$ho=Get-Holds $Original
$rows=@("index`ttarget`tcut`tframe`tnative_luma`toriginal_luma`tnative_shot_ms`toriginal_shot_ms`tstatus")
foreach($k in @($hn.Keys + $ho.Keys | Sort-Object -Unique)){
    $a=$hn[$k];$b=$ho[$k];$why=''
    if(-not $a -or -not $b){$why='a run lacks this hold'}
    elseif(-not $a.hold -or -not $b.hold){$why='not held in both: '+$(if(-not $a.hold){"native $($a.missed.why)"}else{"original $($b.missed.why)"})}
    elseif($a.hold.id -ne $b.hold.id){$why="scenes differ: $($a.hold.id) and $($b.hold.id)"}
    elseif($a.hold.target -ne $b.hold.target -or $a.hold.cut -ne $b.hold.cut -or $a.hold.frame -ne $b.hold.frame){$why="targets differ: $($a.hold.cut)/$($a.hold.frame) and $($b.hold.cut)/$($b.hold.frame)"}
    elseif(-not $a.release -or -not $b.release -or $a.release.drift -ne 0 -or $b.release.drift -ne 0 -or $a.release.frame -ne $a.hold.frame -or $b.release.frame -ne $b.hold.frame){$why='the scene moved during a hold'}
    elseif($a.release.why -ne 'shot' -or $b.release.why -ne 'shot'){$why="a hold not released by its shot: $($a.release.why) / $($b.release.why)"}
    elseif(-not $a.shot -or -not $b.shot -or $a.shot[0] -le $a.hold.tick -or $a.shot[1] -gt $a.release.tick+32 -or $b.shot[0] -le $b.hold.tick -or $b.shot[1] -gt $b.release.tick+32){$why='a shot outside its hold'}
    $pn="$repo/evidence/I-0066/artifacts/$Native/cutscene-hold-$k.png";$po="$repo/evidence/I-0066/artifacts/$Original/cutscene-hold-$k.png"
    if(-not $why -and (-not (Test-Path -LiteralPath $pn) -or -not (Test-Path -LiteralPath $po))){$why='a shot is missing'}
    if($why){$rows+="$k`t`t`t`t`t`t`t`tnot compared: $why";continue}
    $ln=Get-Luma $pn;$lo=Get-Luma $po
    & $ff -hide_banner -loglevel error -y -c:v mjpeg -i $pn -c:v mjpeg -i $po -filter_complex '[0]scale=iw/2:ih/2[a];[1]scale=iw/2:ih/2[b];[a][b]vstack' "$Out/$Label-hold-$k.png"
    $rows+="$k`t$($a.hold.target)`t$($a.hold.cut)`t$($a.hold.frame)`t$ln`t$lo`t$($a.shot[0]-$a.hold.tick)`t$($b.shot[0]-$b.hold.tick)`tcompared"}
$rows | Set-Content -LiteralPath "$Out/$Label.tsv" -Encoding utf8
$rows
