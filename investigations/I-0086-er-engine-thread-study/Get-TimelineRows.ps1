# Read-only decoder of the FD4 timeline rows (I-0086): six int lanes per 0x18-byte row, names from Get-TimelineSteps output.
#   ER: -Start 0x2C05040 -Count 137; DS3 1.15.2: -Start 0x29B97A0 -Count 88
param([string]$Exe,[long]$Start,[int]$Count,[string]$Steps)
$b=[IO.File]::ReadAllBytes($Exe)
$names=@{}; Get-Content $Steps | ForEach-Object { if($_ -match '^\s*(\d+)\s+0x[0-9A-F]+\s+(\S+)'){ $names[[int]$matches[1]]=$matches[2] } }
for($r=0;$r -lt $Count;$r++){ $o=$Start+0x18*$r; $cells=@(); for($j=0;$j -lt 6;$j++){ $v=[BitConverter]::ToInt32($b,$o+4*$j); if($names.ContainsKey($v)){$cells+=$names[$v]}else{$cells+="-"} }
 "{0,3}: {1}" -f $r,($cells -join ' | ') }
