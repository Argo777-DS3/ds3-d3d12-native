param($top,$bottom,$out,$l1,$l2,[int]$cw=1296,[int]$ch=759)
Add-Type -AssemblyName System.Drawing
$a=[Drawing.Image]::FromFile((Resolve-Path $top)); $b=[Drawing.Image]::FromFile((Resolve-Path $bottom))
$bmp=[Drawing.Bitmap]::new($cw,2*$ch); $g=[Drawing.Graphics]::FromImage($bmp)
$r=[Drawing.Rectangle]::new(0,0,$cw,$ch)
$g.DrawImage($a,[Drawing.Rectangle]::new(0,0,$cw,$ch),$r,[Drawing.GraphicsUnit]::Pixel); $g.DrawImage($b,[Drawing.Rectangle]::new(0,$ch,$cw,$ch),$r,[Drawing.GraphicsUnit]::Pixel)
$f=[Drawing.Font]::new('Segoe UI',20); $g.DrawString($l1,$f,[Drawing.Brushes]::Yellow,[float]($cw-280),[float]6); $g.DrawString($l2,$f,[Drawing.Brushes]::Yellow,[float]($cw-280),[float]($ch+6)); $g.Dispose()
$enc=[Drawing.Imaging.ImageCodecInfo]::GetImageEncoders()|?{$_.MimeType -eq 'image/jpeg'}; $p=[Drawing.Imaging.EncoderParameters]::new(1); $p.Param[0]=[Drawing.Imaging.EncoderParameter]::new([Drawing.Imaging.Encoder]::Quality,[long]88)
$bmp.Save($out,$enc,$p); $a.Dispose();$b.Dispose();$bmp.Dispose()
