[CmdletBinding()]
param([Parameter(Mandatory)][int]$TargetProcessId,[Parameter(Mandatory)][string]$Label,[Parameter(Mandatory)][string]$Output)
# I-0066/I-0067 (1120): read-only snapshot of the in-game menu opening conditions (MENUS-MAP section 2): NewMenuSystem
# [14478DA40], its installed root window [+A0], the state word [[144763258]+54] and the input mapper's block word [[144763258]+2E4] (I-0067, 1121). Appends one JSON line.
$ErrorActionPreference='Stop'
$expected='C:\Program Files (x86)\Steam\steamapps\common\DARK SOULS III\Game\DarkSoulsIII.exe'
$target=Get-Process -Id $TargetProcessId
if($target.Path -ne $expected){ throw 'Read-only source identity mismatch' }
Add-Type -TypeDefinition @'
using System;using System.Runtime.InteropServices;
public static class MenuGateRead {
 [DllImport("kernel32.dll",SetLastError=true)] public static extern IntPtr OpenProcess(uint access,bool inherit,int pid);
 [DllImport("kernel32.dll",SetLastError=true)] public static extern bool ReadProcessMemory(IntPtr process,IntPtr address,byte[] bytes,UIntPtr length,out UIntPtr read);
 [DllImport("kernel32.dll")] public static extern bool CloseHandle(IntPtr handle);
}
'@
$handle=[MenuGateRead]::OpenProcess(0x0010,$false,$TargetProcessId)
if($handle -eq [IntPtr]::Zero){ throw 'Read-only process access unavailable' }
function Q([int64]$address,[int]$size=8){ $b=New-Object byte[] $size; $n=[UIntPtr]::Zero
    if(-not $address -or -not [MenuGateRead]::ReadProcessMemory($handle,[IntPtr]$address,$b,[UIntPtr]$size,[ref]$n)){ return $null }
    if($size -eq 8){ [BitConverter]::ToInt64($b,0) } else { [BitConverter]::ToUInt32($b,0) } }
try {
    $image=$target.MainModule.BaseAddress.ToInt64()
    $system=Q ($image+0x478DA40); $root=if($system){ Q ($system+0xA0) }else{ $null }
    $stateOwner=Q ($image+0x4763258); $state=if($stateOwner){ Q ($stateOwner+0x54) 4 }else{ $null }; $blocked=if($stateOwner){ Q ($stateOwner+0x2E4) 4 }else{ $null }
    $rootTable=if($root){ Q $root }else{ $null }
    # 1124: the MenuMan state table [[144763258]+34 .. +34+4*C0] and [[1447572B8]+10]+18 (I-0067 button test).
    $table=@(); if($stateOwner){ for($i=0;$i -lt 0xC0;$i++){ $table+=Q ($stateOwner+0x34+4*$i) 4 } }
    $hpOwner=Q ($image+0x47572B8); $hpInner=if($hpOwner){ Q ($hpOwner+0x10) }else{ $null }; $hp=if($hpInner){ Q ($hpInner+0x18) 4 }else{ $null }
    $row=[ordered]@{utc=[DateTime]::UtcNow.ToString('o');label=$Label;new_menu_system=$system;root_window=$root
        root_window_vtable_rva=$(if($rootTable){ $rootTable-$image }else{ $null });state_owner=$stateOwner;state_54=$state;menu_blocked_2e4=$blocked;state_table_34=$table;value_1447572B8_10_18=$hp}
    ($row | ConvertTo-Json -Compress) | Add-Content -LiteralPath $Output -Encoding utf8
    $row | ConvertTo-Json -Compress
} finally { [void][MenuGateRead]::CloseHandle($handle) }
