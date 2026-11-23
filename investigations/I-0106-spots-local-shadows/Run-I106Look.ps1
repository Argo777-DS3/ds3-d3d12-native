# I-0106 (Thomas, 2026-10-29 07:49: short automatic runs allowed before his Friday test): the spot lights and local shadows seen
# natively where the original was captured with the same save and sweep (I-0100's i100-t3a-al-o1 at the Anor Londo bonfire,
# i100-t2l-m37-o1 at Central Irithyll): one announced slot, the runs in sequence from this one process, each Run-AlphaCast.ps1 on a
# hash-checked copy of Gamma's save (Continue), no cast, eight camera turns of 45 degrees with two pictures each, the debug layer
# on, the restore by hash at its end. -NotBefore HH:mm waits for the announced start (same day).
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Prefix,[Parameter(Mandatory)][string]$BuildFolder,
    [string]$EffectFlags='test-no-ai',[string]$NotBefore='',[ValidateSet('al','m37','both')][string]$Zones='both',[int]$Seconds=200)
$ErrorActionPreference='Stop'
$cast=Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'investigations/I-0066-independent-native-startup/Run-AlphaCast.ps1'
$saves=[ordered]@{
    al=@('C:\Users\User\.ds3-i0039-backups\shebang-zones-gamma-m37-anorlondo-20261026-123818\DS30000.sl2','E96236897D815D1B1339FD49D3AA8D7308C185AA64AC9224A6F72A4638703411')
    m37=@('C:\Users\User\.ds3-i0039-backups\shebang-zones-gamma-m37-20261013-044623\DS30000.sl2','6222C1598CB4CC184825A64208481CC13886C3B31A9F558BBE0DEFDC32E0B1ED')}
if($NotBefore){$at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);while((Get-Date) -lt $at){Start-Sleep 5}}
foreach($zone in $saves.Keys){if($Zones -ne 'both' -and $Zones -ne $zone){continue}
    if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){"$zone skipped: DarkSoulsIII already runs";break}
    $run="$Prefix-$zone";"$(Get-Date -Format HH:mm:ss) $run"
    & $cast -RunName $run -SaveCopy $saves[$zone][0] -SaveCopySha256 $saves[$zone][1] -Continue -Casts 0 -Seconds $Seconds -LookSweep '8,500,2' -EffectFlags $EffectFlags -BuildFolder $BuildFolder
    "$(Get-Date -Format HH:mm:ss) $run done"}
