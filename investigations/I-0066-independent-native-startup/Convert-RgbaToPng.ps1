param([string]$Path,[int]$Width,[int]$Height)
# Raw RGBA rows -> PNG (BGRA in memory), plus a count of pixels differing from the clear colour 0x141414.
Add-Type -AssemblyName System.Drawing
$b=[IO.File]::ReadAllBytes($Path); $n=$Width*$Height; $bgra=New-Object byte[] ($n*4); $other=0
for($i=0;$i -lt $n*4;$i+=4){ $bgra[$i]=$b[$i+2];$bgra[$i+1]=$b[$i+1];$bgra[$i+2]=$b[$i];$bgra[$i+3]=255
    if($b[$i] -ne 20 -or $b[$i+1] -ne 20 -or $b[$i+2] -ne 20){$other++} }
$bmp=[System.Drawing.Bitmap]::new($Width,$Height,[System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$rect=[System.Drawing.Rectangle]::new(0,0,$Width,$Height); $data=$bmp.LockBits($rect,[System.Drawing.Imaging.ImageLockMode]::WriteOnly,$bmp.PixelFormat)
for($y=0;$y -lt $Height;$y++){ [Runtime.InteropServices.Marshal]::Copy($bgra,$y*$Width*4,[IntPtr]::Add($data.Scan0,$y*$data.Stride),$Width*4) }
$bmp.UnlockBits($data); $out=[IO.Path]::ChangeExtension($Path,'.png'); $bmp.Save($out,[System.Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose()
"$out non-background $other / $n"
