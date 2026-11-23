# I-0113 tranche 24 (gap 22's Firelink half, Thomas 2026-11-06: the Firelink fire jumping like the Irithyll barrier; tranche 23
# fixed the barrier with the CopyRect statements and saw the bridge only): the Firelink fire and Fire Orb's casts (608, the radial
# blur that samples the same per-quad copy, STUDY-SILENT-CALLS row 1) filmed natively and in the original. One announced slot,
# Run-AlphaCast.ps1 each (one process, restored by hash): Alpha's all-pyromancies copy at Firelink, no travel, the camera half a
# turn (the bonfire's fire in view), Fire Orb attuned (Up twice from Seething Chaos), -Casts casts (-CastEvery
# seconds after each cast's three shots: about 12 s apart at 8),
# filmed (-Video: the game window at 60 frames/s, casts.mp4). Read with scripts/video/videosheet.py, the fire and the orbs'
# region against a still control region, the original's film beside.
# Runs (-Runs, in order): native (no debug layer, the default flags), original (the Reference).
# -NotBefore HH:mm waits for the announced start (same day).
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Prefix,[Parameter(Mandatory)][string]$BuildFolder,
    [ValidateSet('native','original')][string[]]$Runs=@('native','original'),[ValidateRange(1,8)][int]$Casts=4,[ValidateRange(4,20)][int]$CastEvery=8,
    [string]$LookSweep='1,2000,2',[string]$GraphicsLevels='',[string]$NotBefore='')   # -GraphicsLevels: the OLED's 2560x1440 window (WORKFLOW)
$ErrorActionPreference='Stop'
$cast=Join-Path (Split-Path -Parent $PSScriptRoot) 'I-0066-independent-native-startup/Run-AlphaCast.ps1'
$save='C:\Users\User\.ds3-i0039-backups\alpha-allspells-pyro-20261101-164410\DS30000.sl2';$hash='EF4634FDA017AC756EFF86A0F4002A902AFF1164ED5CBE12018ACDD2A8C1E786'
if(-not $env:DS3_PILOT_SCREEN){throw 'DS3_PILOT_SCREEN unset: take it from scripts/Get-Screens.ps1 first (.device)'}
if($NotBefore){$at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);while((Get-Date) -lt $at){Start-Sleep 5}}
foreach($r in $Runs){
    if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){"$r skipped: DarkSoulsIII already runs";break}
    $name="$Prefix-$r";"$(Get-Date -Format HH:mm:ss) start $name"
    $common=@{RunName=$name;BuildFolder=$BuildFolder;SaveCopy=$save;SaveCopySha256=$hash;Continue=$true;LookSweep=$LookSweep;SpellUpsFirst=2;
        Casts=$Casts;CastEvery=$CastEvery;Seconds=360;Video=$true}
    if($GraphicsLevels){$common.GraphicsLevels=$GraphicsLevels}
    switch($r){
        'native'  {& $cast @common -BootstrapMode Native -NoDebugLayer}
        'original'{& $cast @common -BootstrapMode Reference}}
    "$(Get-Date -Format HH:mm:ss) end $name"}
"SERIES-DONE $(Get-Date -Format HH:mm:ss)"
