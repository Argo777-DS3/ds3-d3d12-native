[CmdletBinding()]
param([Parameter(Mandatory)][int]$TargetProcessId,[Parameter(Mandatory)][string]$Label,[Parameter(Mandatory)][string]$Output)
# I-0066 (1145): read-only snapshot of the env-map region (IBL) manager root+0x42B0 (root = [144961B50]) used by the
# pass-4 selection 67070 and the draw executor 675B0: enable +10, region list +38..+40, buckets +58, count +60, flags
# +2B8 (own colour scale/cube), +2B9/+2BA/+2BB (diffuse variants), +2BD/+2C0/+2C4 (fade override), +2D8, +2DA, +2DC,
# +32C/+338 (SH volume source); [root+42A8] (1400416A0 lookup table), [root+4DD0]; and the first regions' records
# (area +0, id +4, half size +10, fade +C8/+D8, scale +DC, cube +E8, second cube +F0). Appends one JSON line.
$ErrorActionPreference='Stop'
$expected='C:\Program Files (x86)\Steam\steamapps\common\DARK SOULS III\Game\DarkSoulsIII.exe'
$target=Get-Process -Id $TargetProcessId
if($target.Path -ne $expected){ throw 'Read-only source identity mismatch' }
Add-Type -TypeDefinition @'
using System;using System.Runtime.InteropServices;
public static class EnvRegionRead {
 [DllImport("kernel32.dll",SetLastError=true)] public static extern IntPtr OpenProcess(uint access,bool inherit,int pid);
 [DllImport("kernel32.dll",SetLastError=true)] public static extern bool ReadProcessMemory(IntPtr process,IntPtr address,byte[] bytes,UIntPtr length,out UIntPtr read);
 [DllImport("kernel32.dll")] public static extern bool CloseHandle(IntPtr handle);
}
'@
$handle=[EnvRegionRead]::OpenProcess(0x0010,$false,$TargetProcessId)
if($handle -eq [IntPtr]::Zero){ throw 'Read-only process access unavailable' }
function B([int64]$address,[int]$size){ $b=New-Object byte[] $size; $n=[UIntPtr]::Zero
    if(-not $address -or -not [EnvRegionRead]::ReadProcessMemory($handle,[IntPtr]$address,$b,[UIntPtr]::new([uint64]$size),[ref]$n)){ return $null }; ,$b }
function Q([int64]$address){ $b=B $address 8; if($null -eq $b){ $null }else{ [BitConverter]::ToInt64($b,0) } }
try {
    $image=$target.MainModule.BaseAddress.ToInt64()
    $root=Q ($image+0x4961B50); $m=$root+0x42B0; $b=B $m 0x340
    $row=[ordered]@{utc=[DateTime]::UtcNow.ToString('o');label=$Label;root=$root;lookup_42a8=(Q ($root+0x42A8));device_4dd0=(Q ($root+0x4DD0))}
    # 1148: the cube provider [table+250] (14006CED0: entry+40 = provider vt+8("%s%04d" of prefix +10, id +8)) and the
    # default entry table+1A8 (+8 id, +10 prefix wstring, +C scale, +40 cube).
    $table=$row.lookup_42a8; if($table){ $provider=Q ($table+0x250); $row.provider_250=$provider; $row.provider_vtable_rva=$(if($provider){ (Q $provider)-$image }else{ 0 })
        $e=B ($table+0x1A8) 0x48; if($e){ $len=[BitConverter]::ToInt64($e,0x20); $cap=[BitConverter]::ToInt64($e,0x28)
            $prefix=if($cap -gt 7){ $pp=[BitConverter]::ToInt64($e,0x10); $pb=B $pp ([Math]::Min(128,2*$len)); if($pb){ [Text.Encoding]::Unicode.GetString($pb) } }else{ [Text.Encoding]::Unicode.GetString($e,0x10,[Math]::Min(16,2*$len)) }
            $row.default_entry=[ordered]@{id_8=[BitConverter]::ToInt32($e,8);scale_c=[BitConverter]::ToSingle($e,0xC);prefix=$prefix;cube_40=[BitConverter]::ToInt64($e,0x40)} } }
    if($b){
        $row.enable_10=[int]$b[0x10]; $begin=[BitConverter]::ToInt64($b,0x38); $end=[BitConverter]::ToInt64($b,0x40)
        $row.regions=$(if($begin -and $end -ge $begin){ ($end-$begin)/8 }else{ $null }); $row.count_60=[BitConverter]::ToUInt32($b,0x60)
        $row.flags_2b8_2bf=@(for($i=0;$i -lt 8;$i++){ [int]$b[0x2B8+$i] }); $row.fade_2c0_2c4=@([BitConverter]::ToSingle($b,0x2C0),[BitConverter]::ToSingle($b,0x2C4))
        $row.flags_2d8_2db=@(for($i=0;$i -lt 4;$i++){ [int]$b[0x2D8+$i] }); $row.value_2dc=[BitConverter]::ToSingle($b,0x2DC)
        $row.sh_mode_32c=[BitConverter]::ToUInt32($b,0x32C); $row.sh_source_338=[BitConverter]::ToInt64($b,0x338)
        $records=@()
        if($row.regions){ for($k=0;$k -lt [Math]::Min(8,$row.regions);$k++){ $r=Q ($begin+8*$k); $rb=if($r){ B $r 0x100 }else{ $null }
            if($rb){ $cube=[BitConverter]::ToInt64($rb,0xE8); $cubeTable=if($cube){ Q $cube }else{ 0 }
                $records+=[ordered]@{k=$k;area=[BitConverter]::ToUInt32($rb,0);id=[BitConverter]::ToInt32($rb,4)
                    half=@([BitConverter]::ToSingle($rb,0x10),[BitConverter]::ToSingle($rb,0x14),[BitConverter]::ToSingle($rb,0x18))
                    fade=@([BitConverter]::ToSingle($rb,0xC8),[BitConverter]::ToSingle($rb,0xD8));scale_dc=[BitConverter]::ToSingle($rb,0xDC)
                    cube_e8=$cube;cube_vtable_rva=$(if($cubeTable){ $cubeTable-$image }else{ 0 });cube2_f0=[BitConverter]::ToInt64($rb,0xF0)} } } }
        $row.records=$records }
    ($row | ConvertTo-Json -Compress -Depth 5) | Add-Content -LiteralPath $Output -Encoding utf8
    $row | ConvertTo-Json -Compress -Depth 5
} finally { [void][EnvRegionRead]::CloseHandle($handle) }
