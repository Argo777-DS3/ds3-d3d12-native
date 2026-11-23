[CmdletBinding()]
param([Parameter(Mandatory)][int]$TargetProcessId,[Parameter(Mandatory)][string]$Label,[Parameter(Mandatory)][string]$Output)
# I-0066 (1155): read-only inputs of the in-game menu-button test FUN_1407FDB10 (I-0067 MENUS-MAP section 6, "complete
# test"): DAT_14477D550 (gesture/start swap), the menu-blocked mask FUN_1407642B0 recomputed from MenuMan [144763258]
# (state s[i] = +34+4i, fields) and four bytes, hit points [[1447572B8]+10]+18, [MM+83C], the talk manager
# [14489F1E0] (+1FD8/+1FDC/+1FEC/+1FF0), the local player and its flags, and the in-game input context (map
# [[1449647A0]+38] key 144751927): gate +20, key bitset [[[ctx+10]+8]+50] (64 bytes), +88/+A0 heads. One JSON line.
$ErrorActionPreference='Stop'
$expected='C:\Program Files (x86)\Steam\steamapps\common\DARK SOULS III\Game\DarkSoulsIII.exe'
$target=Get-Process -Id $TargetProcessId
if($target.Path -ne $expected){ throw 'Read-only source identity mismatch' }
Add-Type -TypeDefinition @'
using System;using System.Runtime.InteropServices;
public static class MenuButtonRead {
 [DllImport("kernel32.dll",SetLastError=true)] public static extern IntPtr OpenProcess(uint access,bool inherit,int pid);
 [DllImport("kernel32.dll",SetLastError=true)] public static extern bool ReadProcessMemory(IntPtr process,IntPtr address,byte[] bytes,UIntPtr length,out UIntPtr read);
 [DllImport("kernel32.dll")] public static extern bool CloseHandle(IntPtr handle);
}
'@
$handle=[MenuButtonRead]::OpenProcess(0x0010,$false,$TargetProcessId)
if($handle -eq [IntPtr]::Zero){ throw 'Read-only process access unavailable' }
function B([int64]$address,[int]$size){ $b=New-Object byte[] $size; $n=[UIntPtr]::Zero
    if(-not $address -or -not [MenuButtonRead]::ReadProcessMemory($handle,[IntPtr]$address,$b,[UIntPtr]::new([uint64]$size),[ref]$n)){ return $null }; ,$b }
function Q([int64]$address){ $b=B $address 8; if($null -eq $b){ $null }else{ [BitConverter]::ToInt64($b,0) } }
function D([int64]$address){ $b=B $address 4; if($null -eq $b){ $null }else{ [BitConverter]::ToInt32($b,0) } }
function U8([int64]$address){ $b=B $address 1; if($null -eq $b){ $null }else{ [int]$b[0] } }
try {
    $image=$target.MainModule.BaseAddress.ToInt64(); $row=[ordered]@{utc=[DateTime]::UtcNow.ToString('o');label=$Label}
    $row.gesture_14477D550=U8 ($image+0x477D550)
    $mm=Q ($image+0x4763258); $row.menuman=$mm
    if($mm){ $sb=B ($mm+0x34) (4*0xC0); $s={param($i) [BitConverter]::ToInt32($sb,4*$i)}; $f={param($o) D ($mm+$o)}
        $mask=0
        if(& $s 2){$mask=$mask -bor 0x200}; if(& $s 3){$mask=$mask -bor 1}; if(& $f 0x4C){$mask=$mask -bor 0x20000}
        if((& $s 0x11) -or (& $s 0xB4) -or (& $s 0xB5) -or (U8 ($image+0x475F3A8))){$mask=$mask -bor 8}
        if((& $s 0x2A) -or (& $s 0x13) -or (& $s 0x3B) -or (& $s 0xB1) -or (U8 ($image+0x4760678)) -or ((& $s 0xB0) -and (U8 ($image+0x47605EA)))){$mask=$mask -bor 0x8000000}
        if(& $f 0x7C){$mask=$mask -bor 4}; if(& $s 0x1F){$mask=$mask -bor 0x800}; if(& $s 0x33){$mask=$mask -bor 0x40000000}
        if(& $f 0xB8){$mask=$mask -bor 0x20}; if(& $f 0xB4){$mask=$mask -bor 0x10}; if(& $f 0xA0){$mask=$mask -bor 0x400}
        if(& $s 8){$mask=$mask -bor 0x40}; $s9=& $s 9; if($s9 -eq 4){$mask=$mask -bor 0x80}; if($s9 -ge 1 -and $s9 -le 3){$mask=$mask -bor 0x80000}
        if(& $s 0x34){$mask=$mask -bor 0x100000}; if(& $f 0x60){$mask=$mask -bor 0x1000}; if(& $f 0x64){$mask=$mask -bor 0x2000}; if(& $f 0x68){$mask=$mask -bor 0x4000}
        if(& $s 0x40){$mask=$mask -bor 2}; if(& $s 0x38){$mask=$mask -bor 0x2000000}; if(& $f 0x138){$mask=$mask -bor 0x8000}; if(& $f 0xC4){$mask=$mask -bor 0x10000}
        if(& $s 0x28){$mask=$mask -bor 0x40000}; if(& $s 0x15){$mask=$mask -bor 0x400000}; if(& $f 0x90){$mask=$mask -bor 0x200000}; if(& $f 0x98){$mask=$mask -bor 0x800000}
        if(& $s 0x1A){$mask=$mask -bor 0x1000000}; if((& $s 0x2F) -or (U8 ($image+0x47609A0))){$mask=$mask -bor 0x10000000}; if(& $f 0x2DC){$mask=$mask -bor 0x100}; if(& $f 0x8C){$mask=$mask -bor 0x4000000}
        $row.blocked_mask_partial=('0x{0:X}' -f $mask); $row.mm_118=& $f 0x118; $row.mm_2e4=& $f 0x2E4
        $fb=B ($mm+0x83C) 4; $row.mm_83c=[BitConverter]::ToSingle($fb,0)
        $row.nonzero_states=@(for($i=0;$i -lt 0xC0;$i++){ $v=& $s $i; if($v){ "{0:X}={1}" -f $i,$v } }) }
    $hpo=Q ($image+0x47572B8); $hpi=if($hpo){ Q ($hpo+0x10) }else{ $null }; $row.hp=if($hpi){ D ($hpi+0x18) }else{ $null }
    $talk=Q ($image+0x489F1E0); $row.talk=$talk
    if($talk){ $row.talk_1fd8=U8 ($talk+0x1FD8); $row.talk_1fdc=D ($talk+0x1FDC); $tb=B ($talk+0x1FEC) 8; $row.talk_1fec_1ff0=@([BitConverter]::ToSingle($tb,0),[BitConverter]::ToSingle($tb,4)) }
    $row.byte_14477FE94=U8 ($image+0x477FE94)
    $pm=Q ($image+0x477FDB8); $player=if($pm){ Q ($pm+0x80) }else{ $null }; $row.player=$player
    if($player){ $row.player_11c8=('0x{0:X}' -f (Q ($player+0x11C8))) }
    # In-game input context: std::map at [[1449647A0]+38], node {left 0, parent 8, right 10, isnil 19, key 20, value 28}.
    $owner=Q ($image+0x49647A0); $map=if($owner){ Q ($owner+0x38) }else{ $null }; $head=if($map){ Q $map }else{ $null }; $ctx=$null
    if($head){ $node=Q ($head+8); $want=0x144751927-0x140000000+$image; $guard=0
        while($node -and $node -ne $head -and $guard -lt 64){ $guard++; if((U8 ($node+0x19)) -ne 0){ break }; $key=Q ($node+0x20)
            if($key -eq $want){ $ctx=Q ($node+0x28); break }; if($key -lt $want){ $node=Q ($node+0x10) }else{ $node=Q $node } } }
    $row.context=$ctx
    if($ctx){ $row.ctx_gate_20=U8 ($ctx+0x20); $k=Q ($ctx+0x10); $k8=if($k){ Q ($k+8) }else{ $null }
        $bits=if($k8){ B ($k8+0x50) 64 }else{ $null }; $row.key_bitset=if($bits){ ($bits | ForEach-Object { '{0:X2}' -f $_ }) -join '' }else{ $null }
        $row.ctx_88=Q ($ctx+0x88); $row.ctx_a0=Q ($ctx+0xA0); $row.ctx_38=Q ($ctx+0x38); $row.ctx_40=Q ($ctx+0x40) }
    ($row | ConvertTo-Json -Compress -Depth 4) | Add-Content -LiteralPath $Output -Encoding utf8
    $row | ConvertTo-Json -Compress -Depth 4
} finally { [void][MenuButtonRead]::CloseHandle($handle) }
