param([string]$Path,[string]$Out)
# Dumps a DS3 EMEVD (version 0xCD, 64-bit): every event with its instructions (bank[id] int32 args) and parameter substitutions.
Add-Type -Path 'C:\Users\User\.codex\tools\SoulsFormats-1.0.1\lib\net462\SoulsFormats.dll'
$d=[byte[]][SoulsFormats.DCX]::Decompress([IO.File]::ReadAllBytes($Path))
function L($o){[BitConverter]::ToInt64($d,[int]$o)}
$evCount=L 0x10; $evOff=L 0x18; $inOff=L 0x28; $parOff=L 0x58; $argOff=L 0x78
$lines=@("events=$evCount argsOffset=0x{0:X}" -f $argOff)
for($e=0;$e -lt $evCount;$e++){
  $b=$evOff+0x30*$e; $id=L $b; $ic=L ($b+8); $io=L ($b+16); $pc=L ($b+24); $po=L ($b+32); $rest=[BitConverter]::ToInt32($d,[int]($b+40))
  $lines+="EVENT $id rest=$rest instr=$ic params=$pc"
  for($i=0;$i -lt $ic;$i++){
    $ib=$inOff+$io+0x20*$i; $bank=[BitConverter]::ToInt32($d,[int]$ib); $iid=[BitConverter]::ToInt32($d,[int]($ib+4)); $al=L ($ib+8); $ao=L ($ib+16)
    $args=@(); for($k=0;$k -lt $al;$k+=4){ $args+=[BitConverter]::ToInt32($d,[int]($argOff+$ao+$k)) }
    $lines+="  [$i] $bank[$iid] " + ($args -join ',')
  }
  for($p=0;$p -lt $pc;$p++){ $pb=$parOff+$po+0x20*$p; $lines+=("  param instr={0} dst={1} src={2} len={3}" -f (L $pb),(L ($pb+8)),(L ($pb+16)),[BitConverter]::ToInt32($d,[int]($pb+24))) }
}
$lines | Set-Content $Out

