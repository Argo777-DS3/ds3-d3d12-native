param([Parameter(Mandatory)][string[]]$Run)
# I-0097 tranche 5 (2026-10-25): whether NVIDIA's overlay frame counter (white digits at the screen's top-right corner) was on the
# screen in a run's standstill picture (start.png, the whole screen): the white pixels of the 120x60 corner box. Runs with the
# counter are not in the gate's conditions (the gate's runs before 08:55 had none). Prints run, white pixels, counter yes/no.
$ErrorActionPreference='Stop';Add-Type -AssemblyName System.Drawing;$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
foreach($n in @($Run | ForEach-Object { $_ -split ',' } | Where-Object { $_ })){
    $d=if(Test-Path -LiteralPath $n){ $n }else{ "$repo/evidence/I-0066/artifacts/$n" };$f="$d/start.png"
    if(-not (Test-Path -LiteralPath $f)){ "$n no-picture";continue };$b=New-Object Drawing.Bitmap $f;$white=0
    try{ for($x=$b.Width-120;$x -lt $b.Width;$x++){ for($y=0;$y -lt 60;$y++){ $c=$b.GetPixel($x,$y);if([math]::Min($c.R,[math]::Min($c.G,$c.B)) -gt 215){ $white++ } } } }finally{ $b.Dispose() }
    "{0} {1} {2}" -f ([IO.Path]::GetFileName($d)),$white,$(if($white -ge 20){ 'counter' }else{ 'none' }) }
