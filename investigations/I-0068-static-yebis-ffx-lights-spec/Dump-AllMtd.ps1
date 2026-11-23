param([string]$Out)
Add-Type -Path 'C:\Users\User\.codex\tools\SoulsFormats-1.0.1\lib\net462\SoulsFormats.dll'
$bnd=[SoulsFormats.BND4]::Read('C:\Users\User\.codex\I0063-cache\mtd\mtd_AllMaterialBnd.mtdbnd.dcx')
$sb=New-Object Text.StringBuilder
[void]$sb.AppendLine("mtd`tshader`tparam`ttype`tvalue")
$inv=[Globalization.CultureInfo]::InvariantCulture
foreach($f in $bnd.Files){
  $n=[IO.Path]::GetFileName($f.Name)
  $mtd=[SoulsFormats.MTD]::Read($f.Bytes)
  $sh=[IO.Path]::GetFileNameWithoutExtension($mtd.ShaderPath)
  foreach($p in $mtd.Params){ $v=$p.Value; if($v -is [System.Array]){$v=(($v|ForEach-Object{ if($_ -is [single] -or $_ -is [double]){([double]$_).ToString('R',$inv)}else{"$_"} }) -join ',')} elseif($v -is [single]){$v=([double]$v).ToString('R',$inv)}
    [void]$sb.AppendLine(("{0}`t{1}`t{2}`t{3}`t{4}" -f $n,$sh,$p.Name,$p.Type,$v)) }
  foreach($t in $mtd.Textures){ [void]$sb.AppendLine(("{0}`t{1}`ttex:{2}`tuv{3}`tidx{4}" -f $n,$sh,$t.Type,$t.UVNumber,$t.ShaderDataIndex)) }
}
[IO.File]::WriteAllText($Out,$sb.ToString())
