# I-0127 (the effect draws grouped the D3D12 way): the effect stage's measure at the heaviest effect scene known, the pyromancy
# casts at the Firelink Shrine (its bonfire's fire in view). One announced slot, a measuring run each (the PC free of builds and
# Ghidra): Run-AlphaCast.ps1 (one process, restored by hash) on Alpha's all-pyromancies copy at Firelink, without travel, the
# camera half a turn, then -Casts casts -CastEvery seconds apart, Up between casts (Seething Chaos, Flame Fan, Fire Orb, Profaned
# Flame, in turn), measure.flag and no debug layer (Run-StartupTrace -Measure -NoDebugLayer), effect-measure.flag (the module's
# native_effect_measure rows every 600 frames: the stage's render-thread time, the effect block's GPU time, the draws, the calls
# by kind and their repeats). No video (its encoder would load the CPU). Get-I127Measure.ps1 reads the rows.
# Runs (-Runs, in order): each a label=build folder pair (« before=<folder> », « after=<folder> »): an A/B series alternates
# them (before, after, before, after) so a drift of the PC falls on both. -NotBefore HH:mm waits for the announced start.
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Prefix,[Parameter(Mandatory)][string[]]$Runs,
    [ValidateRange(1,12)][int]$Casts=8,[ValidateRange(4,20)][int]$CastEvery=7,[string]$LookSweep='1,2000,2',[string]$GraphicsLevels='',
    [string]$EffectFlags='effect-measure',[string]$NotBefore='')
$ErrorActionPreference='Stop'
$cast=Join-Path (Split-Path -Parent $PSScriptRoot) 'I-0066-independent-native-startup/Run-AlphaCast.ps1'
$save='C:\Users\User\.ds3-i0039-backups\alpha-allspells-pyro-20261101-164410\DS30000.sl2';$hash='EF4634FDA017AC756EFF86A0F4002A902AFF1164ED5CBE12018ACDD2A8C1E786'
if(-not $env:DS3_PILOT_SCREEN){throw 'DS3_PILOT_SCREEN unset: take it from scripts/Get-Screens.ps1 first'}
$pairs=foreach($r in $Runs){$k=$r.IndexOf('=');if($k -lt 1){throw "run « $r »: label=build folder"};$label=$r.Substring(0,$k);$folder=$r.Substring($k+1)
    if($label -notmatch '^[a-z0-9]+$'){throw "label « $label »"};if(-not (Test-Path -LiteralPath (Join-Path $folder 'game_bootstrap.dll'))){throw "no module in $folder"}
    [pscustomobject]@{Label=$label;Folder=$folder}}
if($NotBefore){$at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);while((Get-Date) -lt $at){Start-Sleep 5}}
$n=0
foreach($p in $pairs){$n++
    if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){"$($p.Label) skipped: DarkSoulsIII already runs";break}
    $name="$Prefix-$n-$($p.Label)";"$(Get-Date -Format HH:mm:ss) start $name ($($p.Folder))"
    $common=@{RunName=$name;BuildFolder=$p.Folder;SaveCopy=$save;SaveCopySha256=$hash;Continue=$true;LookSweep=$LookSweep;Casts=$Casts;CastEvery=$CastEvery;
        SpellsPerCatalyst=12;Seconds=360;BootstrapMode='Native';EffectFlags=$EffectFlags;Measure=$true;NoDebugLayer=$true}
    if($GraphicsLevels){$common.GraphicsLevels=$GraphicsLevels}
    & $cast @common
    "$(Get-Date -Format HH:mm:ss) end $name"}
"SERIES-DONE $(Get-Date -Format HH:mm:ss)"
