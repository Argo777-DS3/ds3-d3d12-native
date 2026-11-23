param([string]$Dir,[string]$Out)
$rows=@("file`tcb`tvariable`toffset`tsize")
foreach($f in Get-ChildItem $Dir -Filter *.asm){
  $cb=$null
  foreach($line in Get-Content $f.FullName){
    if($line -match '^// cbuffer (\w+)'){ $cb=$Matches[1]; continue }
    if($line -match '^// }' -and $cb){ $cb=$null; continue }
    if($line -match '^// Resource Bindings'){ break }
    if($cb -and $line -match '^//\s+(?:[\w<>]+\s+)(\w+)(?:\[\d+\])?;\s*// Offset:\s+(\d+)\s+Size:\s+(\d+)(.*)$'){
      if($Matches[4] -notmatch 'unused'){ $rows+=("{0}`t{1}`t{2}`t{3}`t{4}" -f $f.Name,$cb,$Matches[1],$Matches[2],$Matches[3]) } }
    elseif($cb -and $line -match '^//\s+\} (\w+)\[?\d*\]?;\s*// Offset:\s+(\d+)\s+Size:\s+(\d+)'){ $rows+=("{0}`t{1}`t{2}(struct)`t{3}`t{4}" -f $f.Name,$cb,$Matches[1],$Matches[2],$Matches[3]) }
  }
}
$rows | Set-Content $Out
