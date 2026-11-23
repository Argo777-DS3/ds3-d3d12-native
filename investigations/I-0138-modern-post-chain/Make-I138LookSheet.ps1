# I-0138 tranche 6: a sheet of one run made with tone-look-cycle.flag and world-pictures.flag (an SDR chain): the presented
# picture of each 300th world frame (world-<serial>.rgba, R8G8B8A8 rows at the presentation's extent) under the name of the look
# the journal's native_tone_look rows give that frame (the look changes on the frame after a picture). One PNG a picture at
# its own size (<out>/<run>-<k>-<look>.png) and one sheet of all of them, each scaled by -Scale, labelled (<out>/<run>-sheet.png).
# A picture's look is the last row at or before its frame; a run whose rows are missing, or whose journal says a look's pipeline
# failed (the frames after it are the record's own curve), is refused. The extent is the journal's (native_output_extent).
# The cycle goes round while the run lasts, so a look can have several pictures: the first one that shows the world is kept (a
# picture of the loading screen or of the run's closing frame is nearly black: its mean level under -Dark, of 255, on one pixel in
# sixteen; the runs i138-t6e took their first picture on the loading screen at three zones), and the sheet is in the cycle's order.
param([Parameter(Mandatory)][string]$Run,[Parameter(Mandatory)][string]$Out,[double]$Scale=0.5,[int]$Columns=3,[double]$Dark=12)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
$name=Split-Path -Leaf $Run
$rows=@(Select-String -LiteralPath (Join-Path $Run 'startup.jsonl') -Pattern '"event":"native_tone_look","frame"' | ForEach-Object { $_.Line | ConvertFrom-Json })
if(-not $rows.Count){throw "no native_tone_look row in $Run"}
if(Select-String -LiteralPath (Join-Path $Run 'startup.jsonl') -Pattern '"event":"native_tone_look","failed"' -Quiet){throw "a look's pipeline failed in $Run"}
$pictures=@(Get-ChildItem -LiteralPath $Run -Filter 'world-*.rgba' | Where-Object { $_.Name -match '^world-([0-9]+)\.rgba$' } | Sort-Object { [int64]($_.Name -replace '[^0-9]','') })
if(-not $pictures.Count){throw "no world picture in $Run"}
$extent=Select-String -LiteralPath (Join-Path $Run 'startup.jsonl') -Pattern '"event":"native_output_(extent","configured|resized","from":\[[0-9]+,[0-9]+\],"to)":\[([0-9]+),([0-9]+)\]' | Select-Object -Last 1
if(-not $extent){throw "no output extent in $Run's journal"}
$width=[int]$extent.Matches[0].Groups[2].Value;$height=[int]$extent.Matches[0].Groups[3].Value
if($pictures[0].Length -ne 4*$width*$height){throw "the pictures are not ${width}x$height RGBA8 ($($pictures[0].Length) bytes)"}
New-Item -ItemType Directory -Force -Path $Out | Out-Null
$labels=@{'original|False'='Original';'agx|False'='AgX + grade de la zone';'agx|True'='AgX neutre';'aces|False'='ACES + grade de la zone';'aces|True'='ACES neutre';'er|True'="D'apres Elden Ring"}
$order=@('original|False','agx|False','agx|True','aces|False','aces|True','er|True')
$tiles=@();$kept=@{};$left=@()
foreach($picture in $pictures){
    $frame=[int64]($picture.Name -replace '[^0-9]','')
    $row=$rows | Where-Object { [int64]$_.frame -le $frame } | Select-Object -Last 1
    if(-not $row){$row=[pscustomobject]@{look='original';neutral=$false}}
    $look="$($row.look)|$([bool]$row.neutral)";$label=$labels[$look];if(-not $label){$label=$look}
    if($kept[$look]){continue}
    $raw=[IO.File]::ReadAllBytes($picture.FullName)
    $sum=[int64]0;$count=0;for($i=0;$i -lt $raw.Length;$i+=64){$sum+=$raw[$i]+$raw[$i+1]+$raw[$i+2];$count+=3}
    if($sum/$count -lt $Dark){$left+="image $frame ($label): mean level $([math]::Round($sum/$count,1)), left out";continue}
    $kept[$look]=$true
    for($i=0;$i -lt $raw.Length;$i+=4){$r=$raw[$i];$raw[$i]=$raw[$i+2];$raw[$i+2]=$r;$raw[$i+3]=255}   # RGBA to BGRA, opaque
    $bitmap=[Drawing.Bitmap]::new($width,$height,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $data=$bitmap.LockBits([Drawing.Rectangle]::new(0,0,$width,$height),[Drawing.Imaging.ImageLockMode]::WriteOnly,$bitmap.PixelFormat)
    [Runtime.InteropServices.Marshal]::Copy($raw,0,$data.Scan0,$raw.Length);$bitmap.UnlockBits($data)
    $file=Join-Path $Out ("{0}-{1}-{2}{3}.png" -f $name,$frame,$row.look,$(if([bool]$row.neutral){'-neutral'}else{''}))
    $bitmap.Save($file,[Drawing.Imaging.ImageFormat]::Png)
    $tiles+=[pscustomobject]@{Bitmap=$bitmap;Label="$label  (image $frame)";File=$file;Place=[array]::IndexOf($order,$look)}
}
if(-not $tiles.Count){throw "no picture of the world in $Run"}
$tiles=@($tiles | Sort-Object @{Expression={if($_.Place -lt 0){99}else{$_.Place}}},Label)   # a look outside the cycle's six: last, by its name
$tw=[int]($width*$Scale);$th=[int]($height*$Scale);$band=[int](44*$Scale*2);$columns=[math]::Min($Columns,$tiles.Count);$lines=[math]::Ceiling($tiles.Count/$columns)
$sheet=[Drawing.Bitmap]::new($tw*$columns,($th+$band)*$lines,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
$g=[Drawing.Graphics]::FromImage($sheet);$g.Clear([Drawing.Color]::Black);$g.InterpolationMode=[Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$font=[Drawing.Font]::new('Segoe UI',[single](18*$Scale*2),[Drawing.FontStyle]::Bold,[Drawing.GraphicsUnit]::Pixel)
for($i=0;$i -lt $tiles.Count;$i++){$x=($i % $columns)*$tw;$y=[math]::Floor($i/$columns)*($th+$band)
    $g.DrawString($tiles[$i].Label,$font,[Drawing.Brushes]::White,[single]($x+8),[single]($y+6))
    $g.DrawImage($tiles[$i].Bitmap,$x,$y+$band,$tw,$th);$tiles[$i].Bitmap.Dispose()}
$g.Dispose();$sheetFile=Join-Path $Out "$name-sheet.png";$sheet.Save($sheetFile,[Drawing.Imaging.ImageFormat]::Png);$sheet.Dispose()
"sheet $sheetFile ($($tiles.Count) pictures, ${width}x$height)";$tiles | ForEach-Object { $_.Label + '  ' + $_.File };$left
