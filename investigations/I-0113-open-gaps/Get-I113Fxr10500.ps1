# I-0113 tranche 18 (pass 0x23, STUDY-WATER-INTERACTION §4.3 and §6.1): every extracted FXR's root action 10500, its fields 6 (+0x18,
# the pass-0x23 routing) and 8 (+0x20, the reflection-view flag). Read-only; -CsvPath the CSV (one row per 10500 action).
param([string]$Root='C:\Users\User\.codex\I0033-assets\effects',[Parameter(Mandatory)][string]$CsvPath)
(Get-Process -Id $PID).PriorityClass='BelowNormal'
if(-not ('FxrDump' -as [type])){Add-Type -Path (Join-Path $PSScriptRoot '../I-0068-static-yebis-ffx-lights-spec/FxrDump.cs')}
$files=Get-ChildItem $Root -Recurse -Filter 'f*.fxr'
$fail=@();$out=New-Object System.Collections.ArrayList;$total=0
foreach($f in $files){$total++
 try{$d=[FxrDump]::Dump([IO.File]::ReadAllBytes($f.FullName))}catch{$fail+=$f.FullName;continue}
 if($d -eq 'not FXR'){$fail+=$f.FullName;continue}
 $lines=$d -split "`r?`n"; $nodeDepthOfAction=$null;$inRootCfg=$false;$cfgIndent=-1
 for($i=0;$i -lt $lines.Count;$i++){$l=$lines[$i]
  if($l -match '^( *)ACTION 10500 '){
    $ind=$Matches[1].Length; $f1=$lines[$i+1].Trim()
    # the final review's M2: the action's property counts too (5C0E0's second route to pass 0x23 is a property slot when field 6 is 0)
    $counts=[regex]::Match($l,'fields2=(\d+) props1=(\d+) props2=(\d+)')
    $vals=($f1 -replace '^F1 \[','' -replace '\]$','') -split ', '
    # context: indent 2 = action directly on root node; indent 4 = in a config of root; deeper = child nodes
    $kind= if($ind -eq 2){'root-node-action'}elseif($ind -eq 4){'root-config-action(or depth1 node)'}else{"deeper(indent $ind)"}
    [void]$out.Add([pscustomobject]@{Bundle=$f.Directory.Name;File=$f.Name;Indent=$ind;Kind=$kind;N=$vals.Count;F6=$vals[6];F8=$vals[8];Fields2=$counts.Groups[1].Value;Props1=$counts.Groups[2].Value;Props2=$counts.Groups[3].Value;F1=$f1})
  }}}
"total=$total failed=$($fail.Count)"; $fail
$out | Export-Csv -LiteralPath $CsvPath -NoTypeInformation
"all 10500 actions: $($out.Count)"
$out | Group-Object Kind | % {"$($_.Name): $($_.Count)"}
"--- F6 nonzero"; $out | ?{$_.F6 -and $_.F6 -ne '0'} | ft Bundle,File,Kind,F6,F8 -auto | out-string -width 250
"--- F8 nonzero"; $out | ?{$_.F8 -and $_.F8 -ne '0'} | ft Bundle,File,Kind,F6,F8 -auto | out-string -width 250
"--- distinct F6 values"; $out | group F6 | sort Count -desc | select -first 10 | ft Name,Count -auto | out-string
"--- property counts"; $out | Group-Object { "props1=$($_.Props1) props2=$($_.Props2) fields2=$($_.Fields2)" } | ft Count,Name -auto | out-string
"--- distinct F8 values"; $out | group F8 | sort Count -desc | select -first 10 | ft Name,Count -auto | out-string
