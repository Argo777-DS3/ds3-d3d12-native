param([Parameter(Mandatory)][string]$Out,[int]$Seconds=150,[double]$Every=1.5,[string]$Screen='1920x1080@60',[ValidateRange(0.1,1.0)][double]$Scale=1.0)
# I-0097 tranche 5: pictures of a screen every -Every seconds for -Seconds (the loading screen and the first frames of an announced
# run, both renderers); on a screen Windows composes (the ASUS), as an Independent Flip screen can give a stale picture. Hidden helper.
# I-0100 tranche 8: -Scale under 1 saves each picture reduced (10 a second over a whole run), its name carrying its time in ms
# after start-tick.txt.
$ErrorActionPreference='Stop';Add-Type -AssemblyName System.Drawing,System.Windows.Forms
$target=& "$PSScriptRoot/Get-Screens.ps1" -Key $Screen;$b=[Windows.Forms.Screen]::AllScreens | Where-Object { $_.DeviceName -eq $target.device } | Select-Object -First 1
if(-not $b){ throw "screen $Screen not found" };$r=$b.Bounds;New-Item -ItemType Directory -Force -Path $Out | Out-Null
$watch=[Diagnostics.Stopwatch]::StartNew();$k=0
Set-Content -LiteralPath (Join-Path $Out 'start-tick.txt') -Value ([Environment]::TickCount64)   # the journals' clock (GetTickCount64) at the first picture
while($watch.Elapsed.TotalSeconds -lt $Seconds){ $k++;$bmp=New-Object Drawing.Bitmap $r.Width,$r.Height;$g=[Drawing.Graphics]::FromImage($bmp)
    try{ $g.CopyFromScreen($r.Location,[Drawing.Point]::Empty,$r.Size)
        if($Scale -lt 1){ $small=New-Object Drawing.Bitmap $bmp,([int]($r.Width*$Scale)),([int]($r.Height*$Scale))
            try{ $small.Save((Join-Path $Out ('cap-{0:D5}-{1:D7}.jpg' -f $k,[int]$watch.Elapsed.TotalMilliseconds)),[Drawing.Imaging.ImageFormat]::Jpeg) }finally{ $small.Dispose() } }
        else{ $bmp.Save((Join-Path $Out ('cap-{0:D3}.jpg' -f $k)),[Drawing.Imaging.ImageFormat]::Jpeg) } }finally{ $g.Dispose();$bmp.Dispose() }
    $next=$k*$Every*1000;$left=$next-$watch.Elapsed.TotalMilliseconds;if($left -gt 0){ Start-Sleep -Milliseconds ([int]$left) } }
"captures $k"
