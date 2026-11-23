# I-0132 R11c: one zone's sheet from Run-R11cTour's views (Run-ZoneWalk's walk-<n>.png, the same moments of each run): one row a mode
# (the original, the RT level, the PT level), one column a shot (the settle's and the three looks'), each tile 640x360; each picture's
# mean luma (ffmpeg signalstats YAVG of 255, over the game's client, as the tiles; the final review's L7) printed. Run pictures are JPEG screen captures (LIVE-RUN-NOTES): for the light, not for banding.
param([Parameter(Mandatory)][string]$Prefix,[Parameter(Mandatory)][string]$Zone,[string]$Size='1600x900',[string[]]$Modes=@('orig','taa','ptsrc'),
    [int[]]$Shots=@(1,2,3,4,5,6),[string]$Out='C:/Users/User/Desktop/DS3_evidences/I-0132/artifacts/r11c-sheets')
$ErrorActionPreference='Stop'
$ff=(Get-ChildItem "$env:LOCALAPPDATA/Microsoft/WinGet/Packages" -Recurse -Filter ffmpeg.exe | Select-Object -First 1).FullName
$A='C:/Users/User/Desktop/DS3_evidences/I-0066/artifacts'
New-Item -ItemType Directory -Force $Out | Out-Null
$Modes=@($Modes | ForEach-Object { $_ -split ',' });$inputs=@();$n=0
foreach($m in $Modes){ foreach($s in $Shots){ $src="$A/$Prefix-$Zone-$m-$Size/walk-$s.png"
    if(Test-Path -LiteralPath $src){ $y=& $ff -hide_banner -f image2 -c:v mjpeg -i $src -vf "crop=$(($Size -split 'x')[0]):$(($Size -split 'x')[1]):8:31,signalstats,metadata=print:key=lavfi.signalstats.YAVG" -f null - 2>&1 | Select-String 'YAVG=' | Select-Object -First 1
        "$m walk-$s YAVG $((("$y") -split '=')[-1])"; $inputs+='-f','image2','-c:v','mjpeg','-i',$src }
    else{ "$m walk-$s missing"; $inputs+='-f','lavfi','-i','color=c=black:s=1920x1080:d=1' }
    $n++ } }
$w,$h=$Size -split 'x';$cols=$Shots.Count;$filter=(0..($n-1) | ForEach-Object {"[$_]crop=$($w):$($h):8:31,scale=640:360,setsar=1[s$_]"}) -join ';'   # the game's client (the window at the screen's top-left, its client at 8, 31)
$rows=@();for($r=0;$r -lt $Modes.Count;$r++){ $lab=(0..($cols-1) | ForEach-Object {"[s$($r*$cols+$_)]"}) -join '';$filter+=";$($lab)hstack=$cols[r$r]";$rows+="[r$r]" }
$filter+=";$($rows -join '')vstack=$($Modes.Count)"
& $ff -hide_banner -loglevel error -y @inputs -filter_complex $filter -frames:v 1 "$Out/$Prefix-$Zone.png"
"$Out/$Prefix-$Zone.png"
