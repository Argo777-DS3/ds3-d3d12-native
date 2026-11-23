# I-0141: every cutscene-<s>.png of one Run-I139Cutscenes run on one sheet, in the order of the seconds, -Columns a row (row by
# row, left to right; the first picture is the lowest second), each scaled to -Width: to find, between a native run and the
# original's, the shots that show the same moment of a scene (the shots are taken by the clock, and the two runs' scenes do not
# start at the same instant). Writes <Out> (a PNG) and prints the seconds in the sheet's order, a line a row.
param([Parameter(Mandatory)][string]$Run,[Parameter(Mandatory)][string]$Out,[int]$Columns=6,[int]$Width=640,[int]$From=0,[int]$To=100000)
$ErrorActionPreference='Stop'
$ff=Get-ChildItem "$env:LOCALAPPDATA/Microsoft/WinGet/Packages" -Recurse -Filter ffmpeg.exe | Select-Object -First 1 -ExpandProperty FullName
if(-not $ff){throw 'ffmpeg not found'}
$shots=@(Get-ChildItem -LiteralPath $Run -Filter 'cutscene-*.png' | ForEach-Object { [int]($_.BaseName -replace '[^0-9]','') } | Where-Object { $_ -ge $From -and $_ -le $To } | Sort-Object)
if(-not $shots.Count){throw "no cutscene shot in $Run"}
$rows=[int][Math]::Ceiling($shots.Count/$Columns);$cells=$rows*$Columns
$inputs=@();foreach($s in $shots){$inputs+=@('-c:v','mjpeg','-i',(Join-Path $Run "cutscene-$s.png"))}
# A cell of 16:9; a picture of another shape is fitted inside it and padded, not stretched (the final review's L11).
$filter='';$height=[int]([Math]::Round($Width*9/16/2)*2)
for($i=0;$i -lt $cells;$i++){
    if($i -lt $shots.Count){$filter+="[${i}:v]scale=${Width}:${height}:force_original_aspect_ratio=decrease,pad=${Width}:${height}:(ow-iw)/2:(oh-ih)/2[c$i];"}
    else{$filter+="color=c=black:s=${Width}x${height}[c$i];"} }
$stack=''
for($r=0;$r -lt $rows;$r++){$row='';for($c=0;$c -lt $Columns;$c++){$row+="[c$($r*$Columns+$c)]"}
    $filter+=$(if($Columns -gt 1){"${row}hstack=inputs=$Columns[r$r];"}else{"${row}null[r$r];"});$stack+="[r$r]"}
$filter+=$(if($rows -gt 1){"${stack}vstack=inputs=$rows"}else{"${stack}null"})
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
& $ff -hide_banner -loglevel error -y @inputs -filter_complex $filter -frames:v 1 $Out
if($LASTEXITCODE){throw 'ffmpeg failed'}
for($r=0;$r -lt $rows;$r++){ ($shots | Select-Object -Skip ($r*$Columns) -First $Columns) -join '  ' }
