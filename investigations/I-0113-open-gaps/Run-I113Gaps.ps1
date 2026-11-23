# I-0113 gaps 5 and 7, the original's states against the native's, one announced slot (Run-ZoneWalk.ps1 each, restored by hash):
# - flask: Eta at Firelink Shrine (its m40 zone save, Continue) turned to the camera (S) and drinking twice (R, the use key in the test
#   options), filmed: the Reference with ffx-draw-observe.flag (reference_e03_states: the blend and depth at 636F0's entry for s07002
#   and the callback that set each), natively with ffx-record.flag (native_effect_model_first: the same, from the native context).
# - wheel: Beta at the Undead Settlement bonfire (its m31 zone save), the wheels o001279 in view at the arrival, both modes with
#   light-observe.flag: the original's envData per model with its pass state type and light key (reference_instance_env_model, 5B3D0)
#   against the native's light set per model (native_model_light_key). (Tranche 4 ran "!o001279" with decal-observe.flag instead.)
# -NotBefore HH:mm waits for the announced start (same day).
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Prefix,[Parameter(Mandatory)][string]$BuildFolder,
    [ValidateSet('flask','wheel','both')][string]$Gaps='both',[ValidateSet('native','original','both')][string]$Modes='both',[string]$NotBefore='')
$ErrorActionPreference='Stop'
$walk=Join-Path (Split-Path -Parent $PSScriptRoot) 'I-0066-independent-native-startup/Run-ZoneWalk.ps1'
$saves=@{flask=@('C:\Users\User\.ds3-i0039-backups\shebang-zones-eta-m40-20261014-095203\DS30000.sl2','1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4')
    wheel=@('C:\Users\User\.ds3-i0039-backups\shebang-zones-beta-m31-20261013-044305\DS30000.sl2','93D3A76E8C8CC4EA5D6CB4FE76EBFBF23AA30F2A114422EF355968F651CEB0C0')}
if($NotBefore){$at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);while((Get-Date) -lt $at){Start-Sleep 5}}
foreach($gap in 'flask','wheel'){if($Gaps -ne 'both' -and $Gaps -ne $gap){continue}
    foreach($mode in 'native','original'){if($Modes -ne 'both' -and $Modes -ne $mode){continue}
        if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){"$gap $mode skipped: DarkSoulsIII already runs";break}
        $run="$Prefix-$gap-$mode";"$(Get-Date -Format HH:mm:ss) $run"
        $native=$mode -eq 'native'
        $extra=if($gap -eq 'flask'){@{EffectFlags=$(if($native){'test-no-ai,ffx-record'}else{'test-no-ai,ffx-draw-observe'});KeyPath='S300,R200,R200';Video=$true;Shots=2}}
            else{@{EffectFlags='test-no-ai,light-observe';Shots=1}}   # I-0113 gap 7: reference_instance_env_model (5B3D0) against native_model_light_key
        & $walk -RunName $run -BootstrapMode $(if($native){'Native'}else{'Reference'}) -TestSave $saves[$gap][0] -TestSaveSha256 $saves[$gap][1] `
            -Seconds 240 -SettleSeconds 25 -BuildFolder $BuildFolder @extra
        "$(Get-Date -Format HH:mm:ss) $run done"}}
