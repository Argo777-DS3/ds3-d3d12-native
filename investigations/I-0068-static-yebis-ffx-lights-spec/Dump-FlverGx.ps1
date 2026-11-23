param([string]$Chr,[string]$Root,[string]$Out)
if($Chr -eq "ALL"){ $list = Get-ChildItem (Join-Path $Root "chr") -Filter "c????.chrbnd" | ForEach-Object { $_.BaseName } } else { $list = $Chr -split "," }
Add-Type -Path 'C:\Users\User\.codex\tools\SoulsFormats-1.0.1\lib\net462\SoulsFormats.dll'
$sb=New-Object Text.StringBuilder
[void]$sb.AppendLine("chr`tmaterial`tmtd`tmeshes`tgx_items`tgx_values`ttexture_types")
$inv=[Globalization.CultureInfo]::InvariantCulture
foreach($c in $list){
  $p=Join-Path $Root "chr\$c.chrbnd"; if(-not (Test-Path $p)){ [void]$sb.AppendLine("$c`tMISSING"); continue }
  $bnd=[SoulsFormats.BND4]::Read($p)
  $f=$bnd.Files | Where-Object { $_.Name -like '*.flver' } | Select-Object -First 1
  $fl=[SoulsFormats.FLVER]::Read($f.Bytes)
  $cnt=@{}; foreach($m in $fl.Meshes){ $k=$m.MaterialIndex; if(-not $cnt.ContainsKey($k)){$cnt[$k]=0}; $cnt[$k]++ }
  for($k=0;$k -lt $fl.Materials.Count;$k++){
    $mat=$fl.Materials[$k]; $gx=@(); $vals=@(); $b=$mat.GXBytes
    if($b -ne $null){ $at=0; while($at+12 -le $b.Length){ $id=[BitConverter]::ToUInt32($b,$at); if($id -eq 0x7FFFFFFF){break}; $sz=[BitConverter]::ToUInt32($b,$at+8); if($sz -lt 12 -or $at+$sz -gt $b.Length){break}
        $name=[Text.Encoding]::ASCII.GetString($b,$at,4); $gx+=$name
        $w=@(); for($o=$at+12;$o+4 -le $at+$sz -and $o -lt $at+12+64;$o+=4){ $u=[BitConverter]::ToUInt32($b,$o); $fv=[BitConverter]::ToSingle($b,$o); if($u -ne 0 -and [Math]::Abs($fv) -gt 1e-6 -and [Math]::Abs($fv) -lt 1e6){ $w+=([double]$fv).ToString('G6',$inv) } else { $w+=("0x{0:X}" -f $u) } }
        $vals+=($name+"("+($w -join ',')+")"); $at+=$sz } }
    $tx=($mat.Params | ForEach-Object { $_.Param }) -join ','
    [void]$sb.AppendLine(("{0}`t{1}`t{2}`t{3}`t{4}`t{5}`t{6}" -f $c,$mat.Name,[IO.Path]::GetFileName($mat.MTD),$cnt[$k],($gx -join ' '),($vals -join ' '),$tx))
  }
}
[IO.File]::WriteAllText($Out,$sb.ToString())

