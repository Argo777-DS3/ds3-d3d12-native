[CmdletBinding()]
param([Parameter(Mandatory)][string]$Native,[Parameter(Mandatory)][string]$Reference,[int]$Width=1280,[int]$Height=720,
    [string]$OutDirectory='',[string]$Regions='floor-right:820,400,1180,470;floor-left:40,380,300,440;wall:850,40,1150,200;sky-arch:980,90,1100,200;character:590,330,700,500')
# Image regression (2026-10-16): two R16G16B16A16_FLOAT pictures of the lit HDR (the native world-N-hdr.rgba16f and the
# original's YEBIS input reference-draw-00-t0.raw, or its tone-map input reference-tonemap-t0.raw) compared per region:
# mean linear R, G, B and luminance, and the native/original ratio. With -OutDirectory, both are written as PNGs with the same
# fixed display transform (x/(1+x), then gamma 2.2) so the difference is visible side by side.
$ErrorActionPreference='Stop'
function Load([string]$path){ $b=[IO.File]::ReadAllBytes($path); if($b.Length -ne $Width*$Height*8){ throw "$path is $($b.Length) bytes, not $Width x $Height RGBA16F" }
    $f=New-Object float[] ($Width*$Height*4); for($i=0;$i -lt $f.Length;$i++){ $f[$i]=[float][BitConverter]::ToHalf($b,$i*2) }; ,$f }
$n=Load $Native; $r=Load $Reference
function Stats($f,[int]$x0,[int]$y0,[int]$x1,[int]$y1){ $s=[double[]]::new(3);$c=0
    for($y=$y0;$y -lt $y1;$y++){ for($x=$x0;$x -lt $x1;$x++){ $i=($y*$Width+$x)*4; for($k=0;$k -lt 3;$k++){ $s[$k]+=$f[$i+$k] }; $c++ } }
    $m=@($s | ForEach-Object { $_/$c }); [pscustomobject]@{r=$m[0];g=$m[1];b=$m[2];lum=0.2126*$m[0]+0.7152*$m[1]+0.0722*$m[2]} }
$rows=foreach($entry in @("all:0,0,$Width,$Height")+@($Regions -split ';')){ $name,$box=$entry -split ':'; $v=[int[]]($box -split ',')
    $a=Stats $n $v[0] $v[1] $v[2] $v[3]; $o=Stats $r $v[0] $v[1] $v[2] $v[3]
    [pscustomobject]@{region=$name;native_rgb=('{0:F3} {1:F3} {2:F3}' -f $a.r,$a.g,$a.b);original_rgb=('{0:F3} {1:F3} {2:F3}' -f $o.r,$o.g,$o.b);
        native_lum=[math]::Round($a.lum,4);original_lum=[math]::Round($o.lum,4);ratio=$(if($o.lum -gt 0){[math]::Round($a.lum/$o.lum,2)}else{$null})} }
$rows | Format-Table -AutoSize | Out-String -Width 200
if($OutDirectory){ Add-Type -AssemblyName System.Drawing
    foreach($pair in @(@($n,'native'),@($r,'original'))){ $f=$pair[0]; $bmp=[Drawing.Bitmap]::new($Width,$Height)
        $bytes=New-Object byte[] ($Width*$Height*4)
        for($p=0;$p -lt $Width*$Height;$p++){ for($k=0;$k -lt 3;$k++){ $x=[math]::Max([double]0,[double]$f[$p*4+$k]); $bytes[$p*4+2-$k]=[byte][math]::Min([double]255,[math]::Round(255*[math]::Pow($x/(1+$x),1/2.2))) }; $bytes[$p*4+3]=255 }
        $data=$bmp.LockBits([Drawing.Rectangle]::new(0,0,$Width,$Height),[Drawing.Imaging.ImageLockMode]::WriteOnly,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
        for($y=0;$y -lt $Height;$y++){ [Runtime.InteropServices.Marshal]::Copy($bytes,$y*$Width*4,[IntPtr]::Add($data.Scan0,$y*$data.Stride),$Width*4) }
        $bmp.UnlockBits($data); $bmp.Save((Join-Path $OutDirectory "hdr-$($pair[1]).png"),[Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose() } }
