[CmdletBinding()]
param([Parameter(Mandatory)][string]$RunFolder,[Parameter(Mandatory)][string]$CastLog,[Parameter(Mandatory)][int]$Cast,[ValidateRange(0,5000)][int]$FromMs=0,
    [ValidateRange(100,5000)][int]$SpanMs=1200,[ValidateRange(1,60)][int]$Fps=20,[ValidateRange(160,1280)][int]$Width=426,
    [ValidateRange(1,12)][int]$Columns=6,[string]$Out='')
# I-0105 (Thomas, 2026-10-29: frame by frame): one cast of a Run-AlphaCast -Video run as one sheet, -SpanMs from -FromMs after the
# click at -Fps frames a second (ffmpeg's fps and tile filters, WORKFLOW Tools), each tile -Width pixels wide, left to right by time.
# The cast's offset is the run's own "cast k ... video_ms=" line in -CastLog (Run-AlphaCast's output, saved by the slot).
$ErrorActionPreference='Stop'
$video=Join-Path $RunFolder 'casts.mp4';if(-not (Test-Path -LiteralPath $video)){ throw "no casts.mp4 in $RunFolder" }
$line=Get-Content -LiteralPath $CastLog | Where-Object { $_ -match "^cast $Cast .*video_ms=(-?\d+)" } | Select-Object -First 1
if(-not $line){ throw "cast $Cast has no video_ms line in $CastLog" }
$null=$line -match 'video_ms=(-?\d+)';$start=([int]$Matches[1]+$FromMs)/1000.0
$frames=[Math]::Ceiling($SpanMs*$Fps/1000.0);$rows=[Math]::Ceiling($frames/$Columns)
if(-not $Out){ $Out=Join-Path $RunFolder ("cast-{0}-frames-{1}-{2}.png" -f $Cast,$FromMs,($FromMs+$SpanMs)) }
$ff=(Get-Command ffmpeg).Source
& $ff -hide_banner -loglevel error -y -ss ("{0:0.000}" -f $start).Replace(',','.') -t ("{0:0.000}" -f ($SpanMs/1000.0)).Replace(',','.') -i $video `
    -vf "fps=$Fps,scale=${Width}:-2,tile=${Columns}x$rows" -frames:v 1 $Out
if($LASTEXITCODE){ throw "ffmpeg exit $LASTEXITCODE" }
"sheet $Out ($frames frames from $FromMs ms, $([Math]::Round(1000/$Fps)) ms apart)"
