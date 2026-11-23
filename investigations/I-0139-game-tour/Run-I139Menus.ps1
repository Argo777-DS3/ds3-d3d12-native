# I-0139 tranche 4 (sheets/menus.md, npcs-merchants.md): the menus and the Fire Keeper with the existing Run-AlphaCast tours, Eta's
# m40 zone save (-Continue at Firelink), natively then in the original: -OptionsTour (System's tabs), -MenuTour (Inventory, Status,
# System, the bonfire, Attune Spell), -TalkKeeper (the Fire Keeper's first lines). One process; the ASUS by its key; restored by hash.
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Prefix,[Parameter(Mandatory)][string]$BuildFolder,
    [string]$Log='',[string]$Screen='1920x1080@60',[string[]]$Tours=@('options','menu','keeper'))
$ErrorActionPreference='Stop'
$Tours=@($Tours | ForEach-Object { $_ -split ',' } | Where-Object { $_ })
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$cast="$repo/investigations/I-0066-independent-native-startup/Run-AlphaCast.ps1"
$eta='C:\Users\User\.ds3-i0039-backups\shebang-zones-eta-m40-20261014-095203\DS30000.sl2';$etaHash='1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4'
$say={param($t) if($Log){"$t" | Add-Content $Log}else{"$t"}}
try{
    $device=(& "$repo/scripts/Get-Screens.ps1" -Key $Screen).device
    if(-not $device){throw "no $Screen screen"}
    $env:DS3_PILOT_SCREEN=$device;& $say "$(Get-Date -Format HH:mm:ss) screen $device"
    foreach($tour in $Tours){ foreach($mode in 'Native','Reference'){
        if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){& $say "$tour $mode skipped: DarkSoulsIII already runs";continue}
        $run="$Prefix-$tour-$(if($mode -eq 'Native'){'nat'}else{'orig'})";& $say "$(Get-Date -Format HH:mm:ss) $run"
        $castArgs=@{RunName=$run;BootstrapMode=$mode;SaveCopy=$eta;SaveCopySha256=$etaHash;Continue=$true;Casts=0;Seconds=300;BuildFolder=$BuildFolder;EffectFlags='test-no-ai'}
        switch($tour){ 'options'{$castArgs.OptionsTour=$true} 'menu'{$castArgs.MenuTour=$true} 'keeper'{$castArgs.TalkKeeper=$true} default{throw "unknown tour $tour"} }
        & $cast @castArgs *>&1 | ForEach-Object { & $say "$_" } } }
}catch{& $say "ERROR $_"}
finally{& $say "END $(Get-Date -Format HH:mm:ss)"}
