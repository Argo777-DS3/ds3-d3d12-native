# I-0113 tranche 18, gap 21 (the hair during a fast travel, Thomas's clips of 2026-11-02 08:54 native and 08:56 original: a warp
# from the Archdragon Peak bonfire in daylight to Firelink Shrine). Alpha's warp at Firelink (tranche 15) was alike in both, but in
# the dark and with the hair seen from afar; Thomas's character has the same hair (HR_A_0101). One announced slot, Run-AlphaCast.ps1
# each (one process, restored by hash): Alpha's all-pyromancies copy loads at Firelink, rests, travels to Archdragon Peak, then rests
# there and travels back to Firelink, the second warp filmed (travel2.mp4), in daylight as Thomas's.
#   native    light-observe.flag (native_category_change, native_zero_fade, the forward blend rule)
#   original  the Reference with light-observe.flag (reference_part_row_state: the parts' fade bytes, mulCol.w and opacity)
# Both with decal-observe.flag and « !hr_a_0101|fg_a_0100 » in hide-texture.txt during the second travel (-SecondTravelNamed): the
# original's named oracle (each hair and face draw's PS, blend, depth and PS constant buffers) against native_named_draw_sources.
# -NotBefore HH:mm waits for the announced start (same day).
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Prefix,[Parameter(Mandatory)][string]$BuildFolder,
    [ValidateSet('native','original')][string[]]$Runs=@('native','original'),[string]$BackPress='380,613',[ValidateRange(-1,30)][int]$NamedAfter=-1,[string]$NotBefore='',[string]$GraphicsLevels='',
    # I-0113 tranche 32 (gap 23, the whole body's fade): the named oracle's names (data-name fragments), the body parts too.
    [ValidatePattern('^![a-z0-9_|]+$')][string]$Named='!hr_a_0101|fg_a_0100',
    # I-0113 tranche 35 (gap 23, whether DLAA smooths what the original leaves aliased): flags added to the native runs' (no-dlss)
    [ValidatePattern('^([a-z0-9-]+(,[a-z0-9-]+)*)?$')][string]$ExtraFlags='',
    # I-0140 (gap 23): the warp filmed without loss (Run-AlphaCast -VideoLossless), single frames judged.
    [switch]$VideoLossless,
    # I-0140 (gap 23): the first travel and the arrival at Archdragon Peak filmed too (Run-AlphaCast -VideoTravel, travel.mp4).
    [switch]$VideoTravel)
$ErrorActionPreference='Stop'
$cast=Join-Path (Split-Path -Parent $PSScriptRoot) 'I-0066-independent-native-startup/Run-AlphaCast.ps1'
$save='C:\Users\User\.ds3-i0039-backups\alpha-allspells-pyro-20261101-164410\DS30000.sl2';$hash='EF4634FDA017AC756EFF86A0F4002A902AFF1164ED5CBE12018ACDD2A8C1E786'
if(-not $env:DS3_PILOT_SCREEN){throw 'DS3_PILOT_SCREEN unset: take it from scripts/Get-Screens.ps1 -Key 1920x1080@60 first'}
if($NotBefore){$at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);while((Get-Date) -lt $at){Start-Sleep 5}}
foreach($r in $Runs){
    if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){"$r skipped: DarkSoulsIII already runs";break}
    $name="$Prefix-$r";"$(Get-Date -Format HH:mm:ss) start $name"
    $common=@{RunName=$name;BuildFolder=$BuildFolder;SaveCopy=$save;SaveCopySha256=$hash;Continue=$true;BonfirePages=$true;
        TravelPress='803,191;400,280';ArriveSeconds=45;SecondTravel=$BackPress;SecondTravelNamed=$Named;SecondTravelNamedAfter=$NamedAfter;Casts=0;Seconds=420}
    if($VideoLossless){$common.VideoLossless=$true};if($VideoTravel){$common.VideoTravel=$true}
    if($GraphicsLevels){$common.GraphicsLevels=$GraphicsLevels}   # tranche 19: the OLED at 2560x1440 (Run-AlphaCast)
    if($r -eq 'native'){& $cast @common -BootstrapMode Native -EffectFlags ('light-observe,decal-observe'+$(if($ExtraFlags){",$ExtraFlags"})) -NoDebugLayer}
    else{& $cast @common -BootstrapMode Reference -EffectFlags 'light-observe,decal-observe'}
    "$(Get-Date -Format HH:mm:ss) end $name"}
"SERIES-DONE $(Get-Date -Format HH:mm:ss)"
