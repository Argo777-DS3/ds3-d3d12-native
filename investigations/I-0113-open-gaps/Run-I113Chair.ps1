# I-0113 gap 2 (the Shrine Handmaid's chair o404854, white natively, FABLE-REVIEW 1.2): Eta at Firelink Shrine (hash-checked copy
# of its m40 zone save, Continue) teleported beside the chair as in Native2185/Native2188 (the teleport frame's (254.8, -54.5,
# 604.7)), the camera panned with a shot after each look, natively with world-pictures.flag and gbuffer-pictures.flag (the
# presented picture, the lit HDR and GB0..GB4 every 300th world frame) and the chair's draw sources (hide-texture "!o404854"), then the same pan on the original.
# One process; each Run-ZoneWalk.ps1 restores by hash at its end. -NotBefore HH:mm waits for the announced start (same day).
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Prefix,[Parameter(Mandatory)][string]$BuildFolder,
    [string]$Teleport='254.8 -54.5 604.7 90',[string]$Look='-300,0;-300,0;-300,0;-300,0;-300,0;-300,0',
    [ValidateSet('native','original','both')][string]$Modes='both',[string]$NotBefore='',[string]$MoreFlags='')
$ErrorActionPreference='Stop'
$walk=Join-Path (Split-Path -Parent $PSScriptRoot) 'I-0066-independent-native-startup/Run-ZoneWalk.ps1'
$save='C:\Users\User\.ds3-i0039-backups\shebang-zones-eta-m40-20261014-095203\DS30000.sl2'
$hash='1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4'
if($NotBefore){$at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);while((Get-Date) -lt $at){Start-Sleep 5}}
foreach($mode in 'native','original'){if($Modes -ne 'both' -and $Modes -ne $mode){continue}
    if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){"$mode skipped: DarkSoulsIII already runs";break}
    $run="$Prefix-$mode";"$(Get-Date -Format HH:mm:ss) $run"
    $extra=if($mode -eq 'native'){@{EffectFlags='test-no-ai,chr-positions,world-pictures,gbuffer-pictures';HideTextures='!o404854'}}else{@{EffectFlags='test-no-ai'}}
    if($MoreFlags){$extra.EffectFlags+=",$MoreFlags"}   # -MoreFlags light-observe: the instance rows (aSh) of both renderers
    & $walk -RunName $run -BootstrapMode $(if($mode -eq 'native'){'Native'}else{'Reference'}) -TestSave $save -TestSaveSha256 $hash `
        -Seconds 300 -SettleSeconds 20 -Teleport $Teleport -Look $Look -Shots 3 -BuildFolder $BuildFolder @extra
    "$(Get-Date -Format HH:mm:ss) $run done"}
