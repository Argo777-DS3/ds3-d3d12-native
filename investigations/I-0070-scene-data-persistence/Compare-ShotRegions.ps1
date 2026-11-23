param([Parameter(Mandatory)][string]$A,[Parameter(Mandatory)][string]$B,[int]$Columns=4,[int]$Rows=3,
    [int]$Left=8,[int]$Top=31,[int]$Width=1280,[int]$Height=720)
# I-0070: the mean linear luminance of each of Columns x Rows regions of the game window (client area at Left/Top, Width x
# Height in the screenshot) in two screenshots, and their ratio B/A (1.00: the same). Read-only.
Add-Type -AssemblyName System.Drawing
function Regions([string]$path){ $bmp=[Drawing.Bitmap]::new($path); try{
    $out=New-Object 'double[,]' $Rows,$Columns
    for($r=0;$r -lt $Rows;$r++){ for($c=0;$c -lt $Columns;$c++){ $sum=0.0;$n=0
        $x0=$Left+[int]($c*$Width/$Columns);$x1=$Left+[int](($c+1)*$Width/$Columns);$y0=$Top+[int]($r*$Height/$Rows);$y1=$Top+[int](($r+1)*$Height/$Rows)
        for($y=$y0;$y -lt $y1;$y+=4){ for($x=$x0;$x -lt $x1;$x+=4){ $p=$bmp.GetPixel($x,$y)
            $lin={param($v) $s=$v/255.0; if($s -le 0.04045){$s/12.92}else{[Math]::Pow(($s+0.055)/1.055,2.4)}}
            $sum+=0.2126*(& $lin $p.R)+0.7152*(& $lin $p.G)+0.0722*(& $lin $p.B);$n++ } }
        $out[$r,$c]=$sum/$n } }
    return ,$out }finally{ $bmp.Dispose() } }
$ra=Regions $A;$rb=Regions $B
for($r=0;$r -lt $Rows;$r++){ ($(for($c=0;$c -lt $Columns;$c++){ '{0:N2}' -f $(if($ra[$r,$c] -gt 0){$rb[$r,$c]/$ra[$r,$c]}else{0}) }) -join ' ') }
