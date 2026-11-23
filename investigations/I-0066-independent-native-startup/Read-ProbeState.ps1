[CmdletBinding()]
param([Parameter(Mandatory)][int]$TargetProcessId,[Parameter(Mandatory)][string]$Label,[Parameter(Mandatory)][string]$Output)
# I-0066 (1144): read-only snapshot of the player's character probe lighting (1AC840 on p = draw entity +0x4C0, the
# animation +0x3D0 then +0xF0): current SH rows p+40..+6F, flags +70/+71, clocks +74/+78, strength +7C, owned +80/+81,
# probe source +88, thirteen quantized shorts +90, bank key +B0; draw entity = [[[[144 77FDB8]+80]+20B0]+8];
# and [GXRS+4DD0]. Appends one JSON line.
$ErrorActionPreference='Stop'
$expected='C:\Program Files (x86)\Steam\steamapps\common\DARK SOULS III\Game\DarkSoulsIII.exe'
$target=Get-Process -Id $TargetProcessId
if($target.Path -ne $expected){ throw 'Read-only source identity mismatch' }
Add-Type -TypeDefinition @'
using System;using System.Runtime.InteropServices;
public static class ProbeStateRead {
 [DllImport("kernel32.dll",SetLastError=true)] public static extern IntPtr OpenProcess(uint access,bool inherit,int pid);
 [DllImport("kernel32.dll",SetLastError=true)] public static extern bool ReadProcessMemory(IntPtr process,IntPtr address,byte[] bytes,UIntPtr length,out UIntPtr read);
 [DllImport("kernel32.dll")] public static extern bool CloseHandle(IntPtr handle);
}
'@
$handle=[ProbeStateRead]::OpenProcess(0x0010,$false,$TargetProcessId)
if($handle -eq [IntPtr]::Zero){ throw 'Read-only process access unavailable' }
function B([int64]$address,[int]$size){ $b=New-Object byte[] $size; $n=[UIntPtr]::Zero
    if(-not $address -or -not [ProbeStateRead]::ReadProcessMemory($handle,[IntPtr]$address,$b,[UIntPtr]::new([uint64]$size),[ref]$n)){ return $null }; ,$b }
function Q([int64]$address){ $b=B $address 8; if($null -eq $b){ $null }else{ [BitConverter]::ToInt64($b,0) } }
try {
    $image=$target.MainModule.BaseAddress.ToInt64()
    $manager=Q ($image+0x477FDB8); $player=if($manager){ Q ($manager+0x80) }else{ $null }
    $asm=if($player){ Q ($player+0x20B0) }else{ $null }; $entity=if($asm){ Q ($asm+8) }else{ $null }
    $gxrs=Q ($image+0x4961B50); $device=if($gxrs){ Q ($gxrs+0x4DD0) }else{ $null }
    $p=if($entity){ $entity+0x4C0 }else{ $null }; $b=if($p){ B $p 0xC0 }else{ $null }
    $row=[ordered]@{utc=[DateTime]::UtcNow.ToString('o');label=$Label;entity=$entity;gxrs_4dd0=$device}
    if($b){
        $row.sh_rows=@(for($i=0;$i -lt 12;$i++){ [BitConverter]::ToSingle($b,0x40+4*$i) })
        $row.flags70_71=@([int]$b[0x70],[int]$b[0x71]); $row.clocks74_78=@([BitConverter]::ToSingle($b,0x74),[BitConverter]::ToSingle($b,0x78))
        $row.strength7c=[BitConverter]::ToSingle($b,0x7C); $row.owned80_81=@([int]$b[0x80],[int]$b[0x81])
        $row.probe88=[BitConverter]::ToInt64($b,0x88); $row.packed90=@(for($i=0;$i -lt 13;$i++){ [BitConverter]::ToInt16($b,0x90+2*$i) })
        $row.keyB0=@([BitConverter]::ToInt64($b,0xB0),[BitConverter]::ToInt64($b,0xB8))
        $probe=$row.probe88; if($probe){ $pb=B $probe 0x40; if($pb){ $row.probe_head=@(for($i=0;$i -lt 8;$i++){ [BitConverter]::ToInt64($pb,8*$i) }); $row.probe_vtable_rva=[BitConverter]::ToInt64($pb,0)-$image } } }
    ($row | ConvertTo-Json -Compress -Depth 4) | Add-Content -LiteralPath $Output -Encoding utf8
    $row | ConvertTo-Json -Compress -Depth 4
} finally { [void][ProbeStateRead]::CloseHandle($handle) }
