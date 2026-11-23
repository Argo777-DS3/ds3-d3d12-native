[CmdletBinding()]
param([Parameter(Mandatory)][int]$TargetProcessId,[Parameter(Mandatory)][string]$Label,[Parameter(Mandatory)][string]$Output)
# I-0066 (1157): read-only snapshot of the Load Game portrait family (I-0067 MENUS-MAP sections 1, 7, 8): the ten
# CSMenuProfModelRend slots 14478D680[0..9] (each: vtable RVA, step state), the NewMenuSystem root window
# [[14478DA40]+A0] and its vtable RVA (TitleTopDialog = 1428D39C8). Appends one JSON line.
$ErrorActionPreference='Stop'
$expected='C:\Program Files (x86)\Steam\steamapps\common\DARK SOULS III\Game\DarkSoulsIII.exe'
$target=Get-Process -Id $TargetProcessId
if($target.Path -ne $expected){ throw 'Read-only source identity mismatch' }
Add-Type -TypeDefinition @'
using System;using System.Runtime.InteropServices;
public static class PortraitRead {
 [DllImport("kernel32.dll",SetLastError=true)] public static extern IntPtr OpenProcess(uint access,bool inherit,int pid);
 [DllImport("kernel32.dll",SetLastError=true)] public static extern bool ReadProcessMemory(IntPtr process,IntPtr address,byte[] bytes,UIntPtr length,out UIntPtr read);
 [DllImport("kernel32.dll")] public static extern bool CloseHandle(IntPtr handle);
}
'@
$handle=[PortraitRead]::OpenProcess(0x0010,$false,$TargetProcessId)
if($handle -eq [IntPtr]::Zero){ throw 'Read-only process access unavailable' }
function Q([int64]$address){ $b=New-Object byte[] 8; $n=[UIntPtr]::Zero
    if(-not $address -or -not [PortraitRead]::ReadProcessMemory($handle,[IntPtr]$address,$b,[UIntPtr]::new(8),[ref]$n)){ return $null }; [BitConverter]::ToInt64($b,0) }
try {
    $image=$target.MainModule.BaseAddress.ToInt64()
    $slots=@(for($i=0;$i -lt 10;$i++){ $o=Q ($image+0x478D680+8*$i); if($o){ $vt=Q $o; ('0x{0:X}' -f ($vt-$image)) }else{ 0 } })
    $system=Q ($image+0x478DA40); $root=if($system){ Q ($system+0xA0) }else{ $null }; $rootVt=if($root){ (Q $root)-$image }else{ $null }
    $row=[ordered]@{utc=[DateTime]::UtcNow.ToString('o');label=$Label;portrait_slots=$slots;new_menu_system=$system;root_window=$root
        root_vtable_rva=$(if($null -ne $rootVt){ '0x{0:X}' -f $rootVt }else{ $null })}
    ($row | ConvertTo-Json -Compress) | Add-Content -LiteralPath $Output -Encoding utf8
    $row | ConvertTo-Json -Compress
} finally { [void][PortraitRead]::CloseHandle($handle) }
