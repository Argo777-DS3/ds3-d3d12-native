# I-0139 tranche 3: one sheet per cutscene of Run-I139Cutscenes.ps1, the native's shots (top row) above the original's (bottom row),
# each capture's game client scaled to 640x360: the OLED's 2560x1440 capture from 8,31 (the window's frame and title bar left out),
# or on the ASUS (a whole 1920x1080 screen capture) the 1280x720 client at 8,31 (the final review's L4: the desktop left out); any
# other capture size refused (exit 1). -NatPrefix and -OrigPrefix
# may differ (a scene redone natively on a later build beside its earlier original). Writes <Out>/<id>.png; a missing shot is listed.
# ffmpeg 9 (WORKFLOW « Tools »); captures are JPEG under a .png name (-c:v mjpeg).
param([Parameter(Mandatory)][string]$NatPrefix,[Parameter(Mandatory)][string]$OrigPrefix,[Parameter(Mandatory)][string[]]$Labels,
    [Parameter(Mandatory)][string]$Out,[int[]]$Shots=@(5,15,30,60))
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$ff=Get-ChildItem "$env:LOCALAPPDATA/Microsoft/WinGet/Packages" -Recurse -Filter ffmpeg.exe | Select-Object -First 1 -ExpandProperty FullName
if(-not $ff){throw 'Make-I139CutsceneSheets: no ffmpeg (WORKFLOW « Tools »)'}
New-Item -ItemType Directory -Force -Path $Out | Out-Null
$n=$Shots.Count
foreach($label in $Labels){
    $sheet="$Out/$($label -replace '^c','').png";Remove-Item -LiteralPath $sheet -Force -ErrorAction SilentlyContinue
    $inputs=@();$missing=@();$crops=@();$unknown=@()
    foreach($pm in @(@($NatPrefix,'nat'),@($OrigPrefix,'orig'))){ foreach($s in $Shots){ $p="$repo/evidence/I-0066/artifacts/$($pm[0])-$label-$($pm[1])/cutscene-$s.png"
        if(-not (Test-Path -LiteralPath $p)){$missing+="$($pm[1])/$s";continue}
        $probe=(& $ff -hide_banner -c:v mjpeg -i $p 2>&1 | Out-String)
        $wh=$(if($probe -match ', (\d{3,5})x(\d{3,5})'){"$($Matches[1])x$($Matches[2])"}else{'?'})
        $crop=@{'1920x1080'='crop=1280:720:8:31';'2560x1440'='crop=iw-8:ih-31:8:31'}[$wh]
        if(-not $crop){$unknown+="$($pm[1])/$s $wh";continue}
        $inputs+=@('-c:v','mjpeg','-i',$p);$crops+=$crop } }
    if($missing){"$label missing: $($missing -join ', ')";continue}
    if($unknown){"$label FAILED: capture size not known ($($unknown -join ', '))";$script:failed=$true;continue}
    # The ASUS's 1920x1080 screen capture: the 1280x720 client at 8,31; the OLED's 2560x1440 capture: the client from 8,31 (the
    # step review's L1); another width is refused below by ffmpeg's crop (a 0 or negative size), not measured.
    $f='';for($k=0;$k -lt 2*$n;$k++){$f+="[$k]$($crops[$k]),scale=640:360[a$k];"}
    $f+=(0..(2*$n-1) | ForEach-Object {"[a$_]"}) -join ''
    $f+="xstack=inputs=$(2*$n):layout="+((0..(2*$n-1) | ForEach-Object {"$(640*($_%$n))_$(360*[math]::Floor($_/$n))"}) -join '|')
    & $ff -loglevel error -y @inputs -filter_complex $f $sheet
    if($LASTEXITCODE){"$label FAILED: ffmpeg exit $LASTEXITCODE";$script:failed=$true}else{"$label ok"} }
if($script:failed){exit 1}
