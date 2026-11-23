[CmdletBinding()]
param([Parameter(Mandatory)][int]$TargetProcessId,[Parameter(Mandatory)][string]$Label,[Parameter(Mandatory)][string]$Output)
# I-0066 (1152): read-only snapshot of the scene light list [[144961B50]+41D0] (vector +10..+18) used by FCE30/10A220:
# per light the class (point 3D4E808, spot 3D4E8E8, other), +A0 mask, +B0 enabled, position/radius +150..+15C, diffuse
# colour +70..+78 and strength +7C, specular strength +8C, scale +19C, fade distance +B8. Appends one JSON line.
$ErrorActionPreference='Stop'
$expected='C:\Program Files (x86)\Steam\steamapps\common\DARK SOULS III\Game\DarkSoulsIII.exe'
$target=Get-Process -Id $TargetProcessId
if($target.Path -ne $expected){ throw 'Read-only source identity mismatch' }
Add-Type -TypeDefinition @'
using System;using System.Runtime.InteropServices;
public static class LightStateRead {
 [DllImport("kernel32.dll",SetLastError=true)] public static extern IntPtr OpenProcess(uint access,bool inherit,int pid);
 [DllImport("kernel32.dll",SetLastError=true)] public static extern bool ReadProcessMemory(IntPtr process,IntPtr address,byte[] bytes,UIntPtr length,out UIntPtr read);
 [DllImport("kernel32.dll")] public static extern bool CloseHandle(IntPtr handle);
}
'@
$handle=[LightStateRead]::OpenProcess(0x0010,$false,$TargetProcessId)
if($handle -eq [IntPtr]::Zero){ throw 'Read-only process access unavailable' }
function B([int64]$address,[int]$size){ $b=New-Object byte[] $size; $n=[UIntPtr]::Zero
    if(-not $address -or -not [LightStateRead]::ReadProcessMemory($handle,[IntPtr]$address,$b,[UIntPtr]::new([uint64]$size),[ref]$n)){ return $null }; ,$b }
function Q([int64]$address){ $b=B $address 8; if($null -eq $b){ $null }else{ [BitConverter]::ToInt64($b,0) } }
try {
    $image=$target.MainModule.BaseAddress.ToInt64()
    $root=Q ($image+0x4961B50); $container=if($root){ Q ($root+0x41D0) }else{ $null }
    $first=if($container){ Q ($container+0x10) }else{ $null }; $last=if($container){ Q ($container+0x18) }else{ $null }
    $lights=@()
    if($first -and $last -ge $first){ for($at=$first;$at -lt $last -and $lights.Count -lt 256;$at+=8){ $l=Q $at; $b=if($l){ B $l 0x1A0 }else{ $null }
        if($b){ $vt=[BitConverter]::ToInt64($b,0)-$image; $kind=if($vt -eq 0x3D4E808){'point'}elseif($vt -eq 0x3D4E8E8){'spot'}else{('0x{0:X}' -f $vt)}
            $f={param($o) [Math]::Round([BitConverter]::ToSingle($b,$o),4)}
            $lights+=[ordered]@{kind=$kind;enabled=[int]$b[0xB0];mask=[BitConverter]::ToUInt32($b,0xA0);pos=@((& $f 0x150),(& $f 0x154),(& $f 0x158));radius=(& $f 0x15C)
                colour=@((& $f 0x70),(& $f 0x74),(& $f 0x78));strength=(& $f 0x7C);specular=(& $f 0x8C);scale=(& $f 0x19C);fade=(& $f 0xB8)} } } }
    $row=[ordered]@{utc=[DateTime]::UtcNow.ToString('o');label=$Label;count=$lights.Count;lights=$lights}
    ($row | ConvertTo-Json -Compress -Depth 5) | Add-Content -LiteralPath $Output -Encoding utf8
    "lights=$($lights.Count)"
} finally { [void][LightStateRead]::CloseHandle($handle) }
