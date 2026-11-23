param([Parameter(Mandatory)][string]$Native,[Parameter(Mandatory)][string]$Original,[Parameter(Mandatory)][string]$Out,[double]$Every=8,[int]$RowsPerPage=8,[string]$Captures='')
# I-0097 tranche 5 (CADENCE-STUDY C-a, Thomas 2026-10-25: a sheet after every path, read on a phone): the native's pictures
# beside the original's at the same moments of the replayed path. Each run's pictures are Capture-ScreenSeries's
# (evidence/I-0097/captures/<run>, their file times), placed on the path by the run's motion_start (motion-marks.txt, local
# time); one row every -Every seconds of motion, native on the left, original on the right, pages of -RowsPerPage rows,
# 1080 pixels wide (JPEG): <Out>-1.jpg, <Out>-2.jpg, ... A row whose picture is more than 1.5 s from its moment is left grey.
# The character's position drifts between runs (Thomas): a difference is a hint to look at, not a count. Hidden helper.
# I-0100: -Captures, another pictures folder (evidence/I-0100/captures).
$ErrorActionPreference='Stop';Add-Type -AssemblyName System.Drawing
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
function Run($n){ $d=if(Test-Path -LiteralPath $n){ $n }else{ "$repo/evidence/I-0066/artifacts/$n" };$name=[IO.Path]::GetFileName($d.TrimEnd('/','\'))
    $caps=@(Get-ChildItem -LiteralPath "$(if($Captures){$Captures}else{"$repo/evidence/I-0097/captures"})/$name" -Filter 'cap-*.jpg' | Sort-Object Name)
    if(-not $caps.Count){ throw "no pictures for $name" }
    $marks=@{};foreach($l in Get-Content -LiteralPath "$d/motion-marks.txt"){ $f=$l -split ' ';$marks[$f[0]]=$f }
    $start=[datetime]::Parse($marks['motion_start'][2],[Globalization.CultureInfo]::InvariantCulture,[Globalization.DateTimeStyles]::RoundtripKind).ToLocalTime()
    $segments=@($marks.Keys | Where-Object { $_ -match '^segment_(.+)_start$' } | ForEach-Object { [pscustomobject]@{name=$Matches[1];at=([double]$marks[$_][1]-[double]$marks['motion_start'][1])/1000.0} } | Sort-Object at)
    # The game's client rectangle on its screen (start-client.txt: x y width height), the picture cropped to it.
    $c=@((Get-Content -LiteralPath "$d/start-client.txt" -ErrorAction SilentlyContinue) -split ' ');$crop=if($c.Count -ge 4){ [Drawing.Rectangle]::new([int]$c[0],[int]$c[1],[int]$c[2],[int]$c[3]) }else{ $null }
    [pscustomobject]@{name=$name;caps=$caps;start=$start;crop=$crop;length=([double]$marks['motion_end'][1]-[double]$marks['motion_start'][1])/1000.0;segments=$segments} }
function Pick($r,[double]$at){ $want=$r.start.AddSeconds($at);$best=$null;$gap=[double]::MaxValue
    foreach($c in $r.caps){ $g=[math]::Abs(($c.LastWriteTime-$want).TotalSeconds);if($g -lt $gap){ $gap=$g;$best=$c } };if($gap -le 1.5){ $best }else{ $null } }
$n=Run $Native;$o=Run $Original;$length=[math]::Min($n.length,$o.length)
$times=@(for($t=0.0;$t -le $length;$t+=$Every){ $t })
$cell=[Drawing.Size]::new(540,304);$label=26;$font=New-Object Drawing.Font('Segoe UI',13,[Drawing.FontStyle]::Bold,[Drawing.GraphicsUnit]::Pixel)
$codec=[Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() | Where-Object { $_.MimeType -eq 'image/jpeg' };$q=New-Object Drawing.Imaging.EncoderParameters 1
$q.Param[0]=New-Object Drawing.Imaging.EncoderParameter([Drawing.Imaging.Encoder]::Quality,[long]80);$pages=@()
for($p=0;$p*$RowsPerPage -lt $times.Count;$p++){ $rows=@($times | Select-Object -Skip ($p*$RowsPerPage) -First $RowsPerPage)
    $sheet=New-Object Drawing.Bitmap 1080,(($cell.Height+$label)*$rows.Count+$label);$g=[Drawing.Graphics]::FromImage($sheet)
    try{ $g.Clear([Drawing.Color]::FromArgb(24,24,24));$g.InterpolationMode=[Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
        $g.DrawString("natif $($n.name)",$font,[Drawing.Brushes]::White,6,4);$g.DrawString("original $($o.name)",$font,[Drawing.Brushes]::White,546,4)
        for($i=0;$i -lt $rows.Count;$i++){ $t=$rows[$i];$y=$label+$i*($cell.Height+$label)
            $seg=@($n.segments | Where-Object { $_.at -le $t + 0.001 } | Select-Object -Last 1).name
            $g.DrawString(('+{0:F0} s  {1}' -f $t,$seg),$font,[Drawing.Brushes]::Gold,6,$y+4)
            $k=0;foreach($r in @($n,$o)){ $x=$k*540;$k++;$c=Pick $r $t;$rect=[Drawing.Rectangle]::new($x,$y+$label,$cell.Width,$cell.Height)
                if(-not $c){ $g.FillRectangle([Drawing.Brushes]::DimGray,$rect);continue }
                $img=[Drawing.Image]::FromFile($c.FullName);try{ if($r.crop){ $g.DrawImage($img,$rect,$r.crop,[Drawing.GraphicsUnit]::Pixel) }else{ $g.DrawImage($img,$rect) } }finally{ $img.Dispose() } } }
        $file="$Out-$($p+1).jpg";$sheet.Save($file,$codec,$q);$pages+=$file }finally{ $g.Dispose();$sheet.Dispose() } }
$pages
