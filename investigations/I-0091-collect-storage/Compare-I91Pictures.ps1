param([Parameter(Mandatory)][string[]]$Pairs,[string]$Picture='start.png')
# I-0091 (read-only): each -Pairs item "<run a>/<run b>" compares the two runs' standstill picture (-Picture, beside the trace)
# as the replayed path gate does (scripts/Invoke-ReplayGate.ps1): the game's client area scaled to 160x90, the mean absolute
# difference of the RGB channels (0-255) and the share of cells whose summed channel difference exceeds 120.
$root=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
Add-Type -AssemblyName System.Drawing
function PictureCells([string]$file){ $src=[Drawing.Bitmap]::new($file);$dst=[Drawing.Bitmap]::new(160,90);$g=[Drawing.Graphics]::FromImage($dst)
    try{ $g.InterpolationMode=[Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
        $client=Join-Path (Split-Path -Parent $file) ((Split-Path -LeafBase $file)+'-client.txt');$c=if(Test-Path -LiteralPath $client){ @((Get-Content -LiteralPath $client -Raw).Trim() -split ' ') }else{ @() }
        $from=if($c.Count -ge 4){[Drawing.Rectangle]::new([int]$c[0],[int]$c[1],[int]$c[2],[int]$c[3])}elseif($src.Width -ge 1288 -and $src.Height -ge 751){[Drawing.Rectangle]::new(8,31,1280,720)}else{[Drawing.Rectangle]::new(0,0,$src.Width,$src.Height)}
        $g.DrawImage($src,[Drawing.Rectangle]::new(0,0,160,90),$from,[Drawing.GraphicsUnit]::Pixel);$b=[int[]]::new(160*90*3)
        for($y=0;$y -lt 90;$y++){ for($x=0;$x -lt 160;$x++){ $px=$dst.GetPixel($x,$y);$i=($y*160+$x)*3;$b[$i]=$px.R;$b[$i+1]=$px.G;$b[$i+2]=$px.B } };,$b }
    finally{ $g.Dispose();$dst.Dispose();$src.Dispose() } }
foreach($pair in @($Pairs -split '[,;\s]+' | Where-Object { $_ })){
    $a,$b=$pair -split '/'
    $p=PictureCells "$root/evidence/I-0066/artifacts/$a/$Picture";$q=PictureCells "$root/evidence/I-0066/artifacts/$b/$Picture";$sum=0.0;$over=0
    for($i=0;$i -lt $p.Length;$i+=3){ $d=[Math]::Abs($p[$i]-$q[$i])+[Math]::Abs($p[$i+1]-$q[$i+1])+[Math]::Abs($p[$i+2]-$q[$i+2]);$sum+=$d;if($d -gt 120){$over++} }
    [pscustomobject]@{a=$a;b=$b;picture=$Picture;mean_difference=[math]::Round($sum/$p.Length,3);cells_over_120=[math]::Round($over/($p.Length/3),4)}
}
