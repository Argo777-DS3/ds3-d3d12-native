[CmdletBinding()]
param([Parameter(Mandatory)][int]$TargetProcessId,[Parameter(Mandatory)][string]$Label,[Parameter(Mandatory)][string]$Output)
# I-0066 (1137): read-only snapshot of the skin SSS stage state (I-0067 TONEMAP-SKIN section 3): the SSS object
# [[[144961B50]+4DE8]+18] (+38 record copy, +40/+42 counts, +44/+45 enable), the gates [144961BF7], [144961BF6],
# [14473D39D], [[GXRS+5360]+75], and the profile vector [1449622F8] (per profile +0 kernel, +20..+2C, +30 index,
# +35 active, +36 used, +38 link). Appends one JSON line.
$ErrorActionPreference='Stop'
$expected='C:\Program Files (x86)\Steam\steamapps\common\DARK SOULS III\Game\DarkSoulsIII.exe'
$target=Get-Process -Id $TargetProcessId
if($target.Path -ne $expected){ throw 'Read-only source identity mismatch' }
Add-Type -TypeDefinition @'
using System;using System.Runtime.InteropServices;
public static class SssStateRead {
 [DllImport("kernel32.dll",SetLastError=true)] public static extern IntPtr OpenProcess(uint access,bool inherit,int pid);
 [DllImport("kernel32.dll",SetLastError=true)] public static extern bool ReadProcessMemory(IntPtr process,IntPtr address,byte[] bytes,UIntPtr length,out UIntPtr read);
 [DllImport("kernel32.dll")] public static extern bool CloseHandle(IntPtr handle);
}
'@
$handle=[SssStateRead]::OpenProcess(0x0010,$false,$TargetProcessId)
if($handle -eq [IntPtr]::Zero){ throw 'Read-only process access unavailable' }
function B([int64]$address,[int]$size){ $b=New-Object byte[] $size; $n=[UIntPtr]::Zero
    if(-not $address -or -not [SssStateRead]::ReadProcessMemory($handle,[IntPtr]$address,$b,[UIntPtr]$size,[ref]$n)){ return $null }; ,$b }
function Q([int64]$address){ $b=B $address 8; if($null -eq $b){ $null }else{ [BitConverter]::ToInt64($b,0) } }
function U8([int64]$address){ $b=B $address 1; if($null -eq $b){ $null }else{ [int]$b[0] } }
function F([byte[]]$b,[int]$o,[int]$n){ @(for($i=0;$i -lt $n;$i++){ [BitConverter]::ToSingle($b,$o+4*$i) }) }
try {
    $image=$target.MainModule.BaseAddress.ToInt64()
    $gxrs=Q ($image+0x4961B50); $holder=if($gxrs){ Q ($gxrs+0x4DE8) }else{ $null }; $object=if($holder){ Q ($holder+0x18) }else{ $null }
    $head=if($object){ B $object 0x48 }else{ $null }
    $records=if($head){ [BitConverter]::ToInt64($head,0x38) }else{ $null }
    $first=if($records){ B $records 0x120 }else{ $null }
    $settings=if($gxrs){ Q ($gxrs+0x5360) }else{ $null }
    $vector=Q ($image+0x49622F8); $begin=if($vector){ Q $vector }else{ $null }; $end=if($vector){ Q ($vector+8) }else{ $null }
    $profiles=@()
    if($begin -and $end -and $end -ge $begin){ $count=[Math]::Min(16,($end-$begin)/8)
        for($k=0;$k -lt $count;$k++){ $p=Q ($begin+8*$k); $b=if($p){ B $p 0x40 }else{ $null }
            if($b){ $kernel=[BitConverter]::ToInt64($b,0); $kb=if($kernel){ B $kernel 16 }else{ $null }
                $profiles+=[ordered]@{k=$k;strength_width=(F $b 0x20 4);index_30=[BitConverter]::ToUInt32($b,0x30);active_35=[int]$b[0x35];used_36=[int]$b[0x36]
                    link_38=[BitConverter]::ToInt64($b,0x38);kernel0=$(if($kb){ F $kb 0 4 }else{ $null })} } } }
    $row=[ordered]@{utc=[DateTime]::UtcNow.ToString('o');label=$Label;gxrs=$gxrs;sss_object=$object
        object_enable_44=$(if($head){ [int]$head[0x44] }else{ $null });object_loaded_45=$(if($head){ [int]$head[0x45] }else{ $null })
        object_count_40=$(if($head){ [BitConverter]::ToUInt16($head,0x40) }else{ $null });object_active_42=$(if($head){ [BitConverter]::ToUInt16($head,0x42) }else{ $null })
        record0=$(if($first){ F $first 0 8 }else{ $null })
        gate_144961BF7=(U8 ($image+0x4961BF7));gate_144961BF6=(U8 ($image+0x4961BF6));branch_14473D39D=(U8 ($image+0x473D39D))
        settings_5360_75=$(if($settings){ U8 ($settings+0x75) }else{ $null });profile_vector_count=$(if($begin -and $end){ ($end-$begin)/8 }else{ $null });profiles=$profiles}
    ($row | ConvertTo-Json -Compress -Depth 5) | Add-Content -LiteralPath $Output -Encoding utf8
    $row | ConvertTo-Json -Compress -Depth 5
} finally { [void][SssStateRead]::CloseHandle($handle) }
