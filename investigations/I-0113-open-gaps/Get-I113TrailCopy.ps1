# I-0113 tranche 26 (the trails' copy, STUDY-SILENT-CALLS row 2): every extracted FXR's trail actions 606 (LegacyTracer) and 10012
# (Tracer) with the property the draw context's slot 20 (262A70) evaluates before its frame copy. 262A70 reads the action record
# (param_3): its fields2 count +0x14 above 0x1B and its props2 count +0x1C above 6, then the property at [+0x30]+0x280, the property
# array's entry 16 (stride 0x28, FxrDump's layout), which is P2.2 (distortionIntensity in @cccode/fxr's DarkSouls3 schema) when props1
# is 14. A copy needs that property above 0 at the particle's time. Read-only; -CsvPath one row per trail action.
param([string]$Root='C:\Users\User\.codex\I0033-assets\effects',[Parameter(Mandatory)][string]$CsvPath)
(Get-Process -Id $PID).PriorityClass='BelowNormal'
if(-not ('FxrDump' -as [type])){Add-Type -Path (Join-Path $PSScriptRoot '../I-0068-static-yebis-ffx-lights-spec/FxrDump.cs')}
$files=Get-ChildItem $Root -Recurse -Filter 'f*.fxr'
$fail=@();$out=New-Object System.Collections.ArrayList;$total=0
foreach($f in $files){$total++
 try{$d=[FxrDump]::Dump([IO.File]::ReadAllBytes($f.FullName))}catch{$fail+=$f.FullName;continue}
 if($d -eq 'not FXR'){$fail+=$f.FullName;continue}
 $lines=$d -split "`r?`n"
 for($i=0;$i -lt $lines.Count;$i++){$l=$lines[$i]
  if($l -notmatch '^( *)ACTION (606|10012) fields1=(\d+) fields2=(\d+) props1=(\d+) props2=(\d+)'){continue}
  $type=[int]$Matches[2];$f2=[int]$Matches[4];$p1=[int]$Matches[5];$p2=[int]$Matches[6]
  $want=if(16 -lt $p1){"P1.16"}else{"P2.$(16-$p1)"}
  $prop=$null;$mods=@()
  for($j=$i+1;$j -lt $lines.Count -and $lines[$j] -notmatch '^\s*(NODE|CONFIG|ACTION) ';$j++){
   $t=$lines[$j].Trim()
   if($null -eq $prop -and $t.StartsWith("$want ")){$prop=$t;$k=$j+1
    while($k -lt $lines.Count -and $lines[$k].Trim() -match '^(MOD|mp\d)'){$mods+=$lines[$k].Trim();$k++}}}
  $fn='';$vals=@();$max=$null
  if($prop -and $prop -match 'fn=(\w+)( loop)? \[(.*)\]'){$fn=$Matches[1];$vals=@($Matches[3] -split ', ' | ?{$_ -ne ''})}
  # Constant: one value; Stepped/Linear (fn 3, 4): [count, times..., values...]; Zero / One the constants 0 / 1; the others: values not parsed
  switch($fn){
   'Zero'{$max=0}'One'{$max=1}
   'Constant'{$max=[double]$vals[0]}
   {$_ -in 'Stepped','Linear'}{$n=[int]$vals[0];if($vals.Count -ge 1+2*$n){$max=($vals[(1+$n)..(2*$n)]|%{[double]$_}|measure -Maximum).Maximum}}
  }
  $gates=($f2 -gt 0x1B) -and ($p2 -gt 6)
  [void]$out.Add([pscustomobject]@{Bundle=$f.Directory.Name;File=$f.Name;Type=$type;Fields2=$f2;Props1=$p1;Props2=$p2;Gates=$gates;Slot=$want;Fn=$fn;Max=$max;Mods=$mods.Count;Property=$prop;ModLines=($mods -join ' | ')})
 }}
"total=$total failed=$($fail.Count)"; $fail
$out | Export-Csv -LiteralPath $CsvPath -NoTypeInformation
"trail actions: $($out.Count) in $(@($out|select -Unique File).Count) files"
"--- by type, gates"; $out | Group-Object Type,Gates | ft Count,Name -auto | out-string
"--- property slot and function"; $out | Group-Object Slot,Fn | sort Count -desc | ft Count,Name -auto | out-string
"--- property possibly above 0 (max above 0, or a curve not parsed, or modifiers)"
$pos=$out | ?{$_.Gates -and (($_.Max -gt 0) -or ($null -eq $_.Max -and $_.Fn) -or $_.Mods -gt 0)}
"$(@($pos).Count) actions in $(@($pos|select -Unique File).Count) files"
$pos | sort Bundle,File | ft Bundle,File,Type,Fn,Max,Mods,Property -auto | out-string -width 300
