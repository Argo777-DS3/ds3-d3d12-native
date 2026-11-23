param([string]$Out,[string]$Programs)
# Concatenates the listed cached programmes (family<TAB>programme<TAB>path per line) into one blob file,
# reflects it once with the I-0067 scanner and maps blob offsets back to programme names.
$refl='C:\Users\User\Desktop\DS3 Decompilation\investigations\I-0067-static-menus-subsystems-map\Get-DxbcReflection.ps1'
$ms=New-Object IO.MemoryStream; $map=@{}
foreach($line in Get-Content $Programs){
  $c=$line -split "`t"; if(-not (Test-Path -LiteralPath $c[2])){ $map["MISSING:"+$c[0]+":"+$c[1]]=@($c[0],$c[1]); continue }
  $b=[IO.File]::ReadAllBytes($c[2]); $off=$ms.Position; $ms.Write($b,0,$b.Length); while($ms.Position % 16){ $ms.WriteByte(0) }
  $map[("{0:X}" -f $off)]=@($c[0],$c[1])
}
$blob=[IO.Path]::GetTempFileName(); [IO.File]::WriteAllBytes($blob,$ms.ToArray())
$tmp=[IO.Path]::GetTempFileName()
pwsh -NoProfile -File $refl -Path $blob -Out $tmp | Out-Null
$rows=@("family`tprogram`ttype`tbytes`tsha256`tused_constants`tresources")
foreach($l in Get-Content $tmp | Select-Object -Skip 1){ $c=$l -split "`t"; $m=$map[$c[0]]; if(-not $m){continue}; $rows+=("{0}`t{1}`t{2}`t{3}`t{4}`t{5}`t{6}" -f $m[0],$m[1],$c[2],$c[1],$c[3],$c[4],$c[5]) }
foreach($k in $map.Keys){ if($k -like 'MISSING:*'){ $rows+=("{0}`t{1}`tMISSING`t`t`t`t" -f $map[$k][0],$map[$k][1]) } }
$rows | Set-Content $Out
