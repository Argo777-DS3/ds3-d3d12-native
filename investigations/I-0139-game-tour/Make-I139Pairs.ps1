# I-0139: one sheet a bonfire row of Run-I139Tour.ps1, the native's arrival and look sweep above the original's (the game's client
# area of each JPEG screen capture, 1280x720 at 8,31, halved): arrive, sweep-1-1 .. sweep-4-1. Writes <Out>/<label>.png; a row whose
# run lacks a picture is listed as such. ffmpeg 9 (WORKFLOW « Tools »); pictures are JPEG under a .png name (-c:v mjpeg).
param([Parameter(Mandatory)][string]$Prefix,[Parameter(Mandatory)][string]$RowsFile,[Parameter(Mandatory)][string]$Out)
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$ff=Get-ChildItem "$env:LOCALAPPDATA/Microsoft/WinGet/Packages" -Recurse -Filter ffmpeg.exe | Select-Object -First 1 -ExpandProperty FullName
New-Item -ItemType Directory -Force -Path $Out | Out-Null
$names=@('arrive','sweep-1-1','sweep-2-1','sweep-3-1','sweep-4-1')
foreach($line in Get-Content -LiteralPath $RowsFile){ $label=($line -split '=')[0]; if(-not $label){continue}
    $inputs=@();$missing=@()
    foreach($m in 'nat','orig'){ foreach($n in $names){ $p="$repo/evidence/I-0066/artifacts/$Prefix-$label-$m/$n.png"
        if(Test-Path -LiteralPath $p){$inputs+=@('-c:v','mjpeg','-i',$p)}else{$missing+="$m/$n"} } }
    if($missing){"$label missing: $($missing -join ', ')";continue}
    $f='';for($k=0;$k -lt 10;$k++){$f+="[$k]crop=1280:720:8:31,scale=640:360[a$k];"}
    $f+=(0..9 | ForEach-Object {"[a$_]"}) -join '';$f+='xstack=inputs=10:layout='+((0..9 | ForEach-Object {"$(640*($_%5))_$(360*[math]::Floor($_/5))"}) -join '|')
    & $ff -loglevel error -y @inputs -filter_complex $f "$Out/$label.png"; "$label ok" }
