# I-0139 tranche 5 (sheets/endings-dlc.md F05, F06): the credits by the forced flag (witness-flag.txt « 20 1 » in Eta's loaded world,
# Run-AlphaCast -SetFlag: the ending chain's flag consumed by 1408F5930, then the credits movie and the NG+ dialog), shots through
# the movie and after it; natively then in the original. One process per mode; restored by hash.
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Prefix,[Parameter(Mandatory)][string]$BuildFolder,
    [string]$Log='',[string]$Screen='1920x1080@60',[string]$GraphicsLevels='',[switch]$NoReference,[switch]$NoNative,
    [ValidatePattern('^(2[0-3]|30)$')][string]$Flag='20',[ValidateRange(1,600)][int[]]$Shots=@(10,30,60,120,240,420,450,480,540,600))
$ErrorActionPreference='Stop'
if($NoNative -and $NoReference){throw 'Run-I139Credits: -NoNative and -NoReference leave no run'}
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$cast="$repo/investigations/I-0066-independent-native-startup/Run-AlphaCast.ps1"
$eta='C:\Users\User\.ds3-i0039-backups\shebang-zones-eta-m40-20261014-095203\DS30000.sl2';$etaHash='1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4'
$say={param($t) if($Log){"$t" | Add-Content $Log}else{"$t"}}
try{
    $device=(& "$repo/scripts/Get-Screens.ps1" -Key $Screen).device
    if(-not $device){throw "no $Screen screen"}
    $env:DS3_PILOT_SCREEN=$device;& $say "$(Get-Date -Format HH:mm:ss) screen $device"
    $modes=@();if(-not $NoNative){$modes+='Native'};if(-not $NoReference){$modes+='Reference'}
    foreach($mode in $modes){
        if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){& $say "$mode skipped: DarkSoulsIII already runs";continue}
        $run="$Prefix-flag$Flag-$(if($mode -eq 'Native'){'nat'}else{'orig'})"
        if(Test-Path -LiteralPath "$repo/evidence/I-0066/artifacts/$run/restoration.json"){& $say "$run done before: skipped";continue}
        & $say "$(Get-Date -Format HH:mm:ss) $run"
        # The flag is set about 100 s after the launch: the last shot plus 300 s, at most 900 (Run-AlphaCast's bound).
        $castArgs=@{RunName=$run;BootstrapMode=$mode;SaveCopy=$eta;SaveCopySha256=$etaHash;Continue=$true;Casts=0;StaySeconds=5
            Seconds=[Math]::Min(900,($Shots | Measure-Object -Maximum).Maximum+300);BuildFolder=$BuildFolder;EffectFlags='test-no-ai';SetFlag="$Flag 1";FlagShots=$Shots}
        if($GraphicsLevels){$castArgs.GraphicsLevels=$GraphicsLevels}
        & $cast @castArgs *>&1 | ForEach-Object { & $say "$_" } }
}catch{& $say "ERROR $_"}
finally{& $say "END $(Get-Date -Format HH:mm:ss)"}
