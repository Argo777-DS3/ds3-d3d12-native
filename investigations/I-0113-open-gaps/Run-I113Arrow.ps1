# I-0113 tranche 32, gap 14 (Lightning Arrow's bow too tall, its string in segments: Thomas, 2026-11-08 07:13 native, 07:16
# original). One announced slot, Run-AlphaCast.ps1 each (one process, restored by hash): Alpha-faith's attuned copy (Great Lightning
# Spear, Lightning Spear, Lightning Stake, Lightning Arrow) at Firelink, the camera turned round the character in six looks (one
# look of 1400 units turned it about 15 degrees: the game limits the turn's speed; six gave the side view of i113-t32-arrow2-*)
# (-LookSweep, Thomas 2026-11-08 08:41: a better angle on the whole bow), the talisman taken (-SwitchRight twice), eight casts with
# Up before each (each spell twice), the button held 3 s (-CastHoldMs: the bow drawn), pictures inside the hold, filmed (-Video).
#   native    ffx-record.flag: ffx_model_rows (s08290's rows, its +0xB0/+0x1E0 bytes and mesh records, the palettes' inputs)
#   original  the Reference with ffx-record.flag (its ffx_model_rows are not reached with this flag alone: i113-t32-arrow-original wrote none)
# -NotBefore HH:mm waits for the announced start (same day).
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Prefix,[Parameter(Mandatory)][string]$BuildFolder,
    [ValidateSet('native','original')][string[]]$Runs=@('native','original'),[string]$NotBefore='',[string]$GraphicsLevels='')
$ErrorActionPreference='Stop'
$cast=Join-Path (Split-Path -Parent $PSScriptRoot) 'I-0066-independent-native-startup/Run-AlphaCast.ps1'
$save='C:\Users\User\.ds3-i0039-backups\alpha-faith-attuned-20261030-002542\DS30000.sl2';$hash='6D433A3B6801740DC92367E9A10633F4E76A1B5D589BC32367905F5712F15F3D'
if(-not $env:DS3_PILOT_SCREEN){throw 'DS3_PILOT_SCREEN unset: take it from scripts/Get-Screens.ps1 -Key 1920x1080@60 first'}
if((Get-FileHash -LiteralPath $save -Algorithm SHA256).Hash -ne $hash){throw "save copy hash mismatch: $save"}
if($NotBefore){$at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);while((Get-Date) -lt $at){Start-Sleep 5}}
foreach($r in $Runs){
    if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){"$r skipped: DarkSoulsIII already runs";break}
    $name="$Prefix-$r";"$(Get-Date -Format HH:mm:ss) start $name"
    $common=@{RunName=$name;BuildFolder=$BuildFolder;SaveCopy=$save;SaveCopySha256=$hash;Continue=$true;SwitchRight=$true;SwitchRightTimes=2;
        SpellsPerCatalyst=8;Casts=8;CastEvery=8;CastHoldMs=3000;ShotsMs=@(1200,2200,3200);LookSweep='6,1400,1';Video=$true;Seconds=300}
    if($GraphicsLevels){$common.GraphicsLevels=$GraphicsLevels}
    if($r -eq 'native'){& $cast @common -BootstrapMode Native -EffectFlags 'ffx-record' -NoDebugLayer}
    else{& $cast @common -BootstrapMode Reference -EffectFlags 'ffx-record'}
    "$(Get-Date -Format HH:mm:ss) end $name"}
"SERIES-DONE $(Get-Date -Format HH:mm:ss)"
