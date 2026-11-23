# I-0141: a cutscene's shots of three runs of Run-I139Cutscenes side by side, and their mean levels. For each cutscene-<s>.png the
# three runs hold (JPEG screen captures under a .png name, the runs' own size): one picture <Out>/<Label>-<s>.png, the original on
# the left, the native before in the middle, the native after on the right, each scaled to -Width; and a row of <Out>/<Label>-levels.txt:
# the mean luma (ffmpeg's signalstats YAVG, of 255) of each whole picture and of its centre (the middle half in both directions:
# where a cutscene frames its characters; a number to read beside the pictures, not a verdict). A run of the original and a
# native run are not at the same instant of a scene: the shots are taken by the clock from the cutscene's start (the E pilot's
# note, I-0140: the native's shots run later in 31000010).
param([Parameter(Mandatory)][string]$Original,[Parameter(Mandatory)][string]$Before,[Parameter(Mandatory)][string]$After,
    [Parameter(Mandatory)][string]$Out,[Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Label,[int]$Width=1280)
$ErrorActionPreference='Stop'
$ff=Get-ChildItem "$env:LOCALAPPDATA/Microsoft/WinGet/Packages" -Recurse -Filter ffmpeg.exe | Select-Object -First 1 -ExpandProperty FullName
if(-not $ff){throw 'ffmpeg not found'}
New-Item -ItemType Directory -Force -Path $Out | Out-Null
function Level([string]$picture,[string]$crop){
    $filter=$(if($crop){"$crop,"}else{''})+'signalstats,metadata=print:file=-'
    $lines=& $ff -hide_banner -loglevel error -c:v mjpeg -i $picture -frames:v 1 -vf $filter -f null - 2>$null
    $row=$lines | Where-Object { $_ -match 'lavfi\.signalstats\.YAVG=([0-9.]+)' } | Select-Object -First 1
    if($row -and $row -match 'YAVG=([0-9.]+)'){[double]::Parse($Matches[1],[Globalization.CultureInfo]::InvariantCulture)}else{[double]::NaN} }
$shots=@(Get-ChildItem -LiteralPath $After -Filter 'cutscene-*.png' | ForEach-Object { [int]($_.BaseName -replace '[^0-9]','') } | Sort-Object)
$table=@("shot  original  before  after   |  centre: original  before  after")
foreach($s in $shots){
    $files=@($Original,$Before,$After | ForEach-Object { Join-Path $_ "cutscene-$s.png" })
    if(@($files | Where-Object { -not (Test-Path -LiteralPath $_) }).Count){$table+="{0,4}  missing in a run" -f $s;continue}
    $sheet=Join-Path $Out "$Label-$s.png"
    & $ff -hide_banner -loglevel error -y -c:v mjpeg -i $files[0] -c:v mjpeg -i $files[1] -c:v mjpeg -i $files[2] `
        -filter_complex "[0:v]scale=${Width}:-2[a];[1:v]scale=${Width}:-2[b];[2:v]scale=${Width}:-2[c];[a][b][c]hstack=inputs=3" -frames:v 1 $sheet
    if($LASTEXITCODE){throw "ffmpeg failed on shot $s"}
    $whole=@($files | ForEach-Object { Level $_ '' });$centre=@($files | ForEach-Object { Level $_ 'crop=iw/2:ih/2' })
    $table+=("{0,4}  {1,8:F1}  {2,6:F1}  {3,5:F1}   |  {4,16:F1}  {5,6:F1}  {6,5:F1}" -f $s,$whole[0],$whole[1],$whole[2],$centre[0],$centre[1],$centre[2]) }
$table | Set-Content -LiteralPath (Join-Path $Out "$Label-levels.txt") -Encoding utf8
$table
