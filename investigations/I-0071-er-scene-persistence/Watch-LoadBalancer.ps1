[CmdletBinding()]
param([Parameter(Mandatory)][string]$Output,[ValidateRange(60,1200)][int]$Seconds=480,[ValidateRange(50,2000)][int]$EveryMs=250)
# I-0071 (ER study C, D-0195): read-only poll of DS3's CSLoadBalancerImp while a run plays. The balancer is [image+0x479B9F8]
# (constructor 140EA3F50, update 140EA40B0): its input +0x08 is the engine's frame-rate measure, copied from
# [[image+0x489DD10]+0x2BC] each update; +0x10/+0x14 its band (23 / 27 at construction), +0x28 enabled, +0x2C the level
# 0..20 (up after +0x18 = 5 frames under the band, down after +0x1C = 20 frames over it, holds +0x24 / +0x20), +0x30 the hold
# left, +0x50 the ramp of the levels' features. Waits for DarkSoulsIII, then appends one JSON line per changed sample.
$ErrorActionPreference='Stop'
$expected='C:\Program Files (x86)\Steam\steamapps\common\DARK SOULS III\Game\DarkSoulsIII.exe'
Add-Type -TypeDefinition @'
using System;using System.Runtime.InteropServices;
public static class BalancerRead {
 [DllImport("kernel32.dll",SetLastError=true)] public static extern IntPtr OpenProcess(uint access,bool inherit,int pid);
 [DllImport("kernel32.dll",SetLastError=true)] public static extern bool ReadProcessMemory(IntPtr process,IntPtr address,byte[] bytes,UIntPtr length,out UIntPtr read);
 [DllImport("kernel32.dll")] public static extern bool CloseHandle(IntPtr handle);
}
'@
$deadline=(Get-Date).AddSeconds(180);$target=$null
while(-not $target -and (Get-Date) -lt $deadline){ $target=Get-Process DarkSoulsIII -ErrorAction SilentlyContinue | Select-Object -First 1; if(-not $target){ Start-Sleep 1 } }
if(-not $target){ throw 'no DarkSoulsIII process' }
if($target.Path -ne $expected){ throw 'Read-only source identity mismatch' }
$handle=[BalancerRead]::OpenProcess(0x0010,$false,$target.Id)
if($handle -eq [IntPtr]::Zero){ throw 'Read-only process access unavailable' }
function B([int64]$a,[int]$n){ $b=New-Object byte[] $n;$r=[UIntPtr]::Zero
    if(-not $a -or -not [BalancerRead]::ReadProcessMemory($handle,[IntPtr]$a,$b,[UIntPtr]::new([uint64]$n),[ref]$r)){ return $null };,$b }
try{
    $image=$target.MainModule.BaseAddress.ToInt64();$t0=Get-Date;$last=''
    while(((Get-Date)-$t0).TotalSeconds -lt $Seconds -and -not $target.HasExited){
        $p=B ($image+0x479B9F8) 8;$f=B ($image+0x489DD10) 8
        if($p -and $f){ $lb=[BitConverter]::ToInt64($p,0);$fr=[BitConverter]::ToInt64($f,0)
            $x=if($lb){ B $lb 0x68 }else{ $null };$m=if($fr){ B ($fr+0x2BC) 4 }else{ $null }
            if($x){ $row=[ordered]@{fps_source=$(if($m){[Math]::Round([BitConverter]::ToSingle($m,0),2)}else{$null});input=[Math]::Round([BitConverter]::ToSingle($x,8),2)
                    band=@([BitConverter]::ToSingle($x,0x10),[BitConverter]::ToSingle($x,0x14));enabled=$x[0x28];level=[BitConverter]::ToInt32($x,0x2C)
                    hold=[BitConverter]::ToInt32($x,0x30);under=[BitConverter]::ToInt32($x,0x34);over=[BitConverter]::ToInt32($x,0x38);ramp=[Math]::Round([BitConverter]::ToSingle($x,0x50),3)}
                $key=($row.Values | ForEach-Object { "$_" }) -join '|'
                if($key -ne $last){ $last=$key;$row.t=[Math]::Round(((Get-Date)-$t0).TotalSeconds,2)
                    Add-Content -LiteralPath $Output -Value ($row | ConvertTo-Json -Compress) } } }
        Start-Sleep -Milliseconds $EveryMs }
} finally { [void][BalancerRead]::CloseHandle($handle) }
