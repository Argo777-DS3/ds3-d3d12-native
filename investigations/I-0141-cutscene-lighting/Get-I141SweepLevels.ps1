# I-0141 tranche 3: the mean luma (ffmpeg signalstats YAVG, of 255) of every sweep-1-<s>.png of a Run-AlphaCast run's LookSweep,
# whole picture, one line a second: to pair the ceremony scene's moments between runs (the shots are taken by the clock).
param([Parameter(Mandatory)][string]$Run,[string]$Prefix='sweep-1')
$ErrorActionPreference='Stop'
$ff=Get-ChildItem "$env:LOCALAPPDATA/Microsoft/WinGet/Packages" -Recurse -Filter ffmpeg.exe | Select-Object -First 1 -ExpandProperty FullName
if(-not $ff){throw 'ffmpeg not found'}
$shots=@(Get-ChildItem -LiteralPath $Run -Filter "$Prefix-*.png" | ForEach-Object { [int]($_.BaseName -replace '^.*-','') } | Sort-Object)
foreach($s in $shots){
    $text=& $ff -hide_banner -nostats -c:v mjpeg -i (Join-Path $Run "$Prefix-$s.png") -vf 'signalstats,metadata=print:key=lavfi.signalstats.YAVG' -f null - 2>&1 | Out-String
    $m=[regex]::Match($text,'YAVG=([0-9.]+)');"{0}`t{1}" -f $s,$(if($m.Success){[Math]::Round([double]$m.Groups[1].Value,1)}else{'NA'}) }
