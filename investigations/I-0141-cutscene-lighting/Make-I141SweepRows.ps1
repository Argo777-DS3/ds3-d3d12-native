# I-0141 tranche 3: one row a moment, the same LookSweep shot (sweep-1-<s>.png) of several runs side by side, each run with its
# own second offset (the runs' scenes start at different instants): -Runs "folder|offset" ..., -Seconds the first run's seconds.
# Writes <Out> (a PNG), each cell -Width wide, a row a second.
param([Parameter(Mandatory)][string[]]$Runs,[Parameter(Mandatory)][int[]]$Seconds,[Parameter(Mandatory)][string]$Out,[int]$Width=640,[string]$Prefix='sweep-1')
$ErrorActionPreference='Stop'
$ff=Get-ChildItem "$env:LOCALAPPDATA/Microsoft/WinGet/Packages" -Recurse -Filter ffmpeg.exe | Select-Object -First 1 -ExpandProperty FullName
if(-not $ff){throw 'ffmpeg not found'}
$height=[int]([Math]::Round($Width*9/16/2)*2);$inputs=@();$filter='';$i=0;$rows=''
foreach($s in $Seconds){$row=''
    foreach($r in $Runs){$folder,$offset=$r -split '\|';$shot=Join-Path $folder "$Prefix-$($s+[int]$offset).png"
        if(Test-Path -LiteralPath $shot){$inputs+=@('-c:v','mjpeg','-i',$shot);$filter+="[$($inputs.Count/4-1):v]scale=${Width}:${height}:force_original_aspect_ratio=decrease,pad=${Width}:${height}:(ow-iw)/2:(oh-ih)/2[c$i];"}
        else{$filter+="color=c=black:s=${Width}x${height}[c$i];"}
        $row+="[c$i]";$i++}
    $n=$Seconds.IndexOf($s);$filter+=$(if($Runs.Count -gt 1){"${row}hstack=inputs=$($Runs.Count)[r$n];"}else{"${row}null[r$n];"});$rows+="[r$n]"}
$filter+=$(if($Seconds.Count -gt 1){"${rows}vstack=inputs=$($Seconds.Count)[out]"}else{"${rows}null[out]"})
& $ff -hide_banner -loglevel error -y @inputs -filter_complex $filter -map '[out]' -frames:v 1 $Out
if($LASTEXITCODE){throw "ffmpeg $LASTEXITCODE"}
