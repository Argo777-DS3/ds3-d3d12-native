# I-0139 (ROADMAP workstream 6): the tour's bonfire rows. A copy of I-0132's Run-R11cTour.ps1 travel loop (D-0217: its measure
# differs: the original travels to the same bonfire with the same look sweep, so each row has its native and original pictures from
# one arrival). For each row « label=tabX,rowY » (Eta's Travel page, pilot units; sheets/zones-bonfires.md gives them): the native
# travel (Run-AlphaCast -Continue -BonfirePages -TravelPress, the arrival's look sweep, the bonfire kept as a hash-recorded test save
# `eta-i139-<label>`), then the original's (-BootstrapMode Reference, same press and sweep, nothing kept), then the native journal's
# summary (I-0132's Get-R11cJournal.py) into the log. One process; -NotBefore HH:mm (same day); the ASUS by its key; a row is skipped
# when DarkSoulsIII already runs.
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Prefix,[Parameter(Mandatory)][string]$BuildFolder,
    [Parameter(Mandatory)][string[]]$Rows,[string]$NotBefore='',[string]$Log='',[string]$Screen='1920x1080@60',
    [string]$NativeFlags='test-no-ai,world-pictures,rt-scene,rt-fail-magenta',[string]$ReferenceFlags='test-no-ai',[switch]$NoReference,[switch]$NoKeep,
    [ValidateRange(20,180)][int]$ArriveSeconds=45,[string]$LookSweep='4,-300,2')
$ErrorActionPreference='Stop'
$Rows=@($Rows | ForEach-Object { $_ -split ' ' } | Where-Object { $_ })
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$cast="$repo/investigations/I-0066-independent-native-startup/Run-AlphaCast.ps1"
$reader="$repo/investigations/I-0132-rt-optimisation-zones/Get-R11cJournal.py"
$eta='C:\Users\User\.ds3-i0039-backups\shebang-zones-eta-m40-20261014-095203\DS30000.sl2';$etaHash='1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4'
$say={param($t) if($Log){"$t" | Add-Content $Log}else{"$t"}}
try{
    $device=(& "$repo/scripts/Get-Screens.ps1" -Key $Screen).device
    if(-not $device){throw "no $Screen screen"}
    $env:DS3_PILOT_SCREEN=$device;& $say "$(Get-Date -Format HH:mm:ss) screen $device"
    if($NotBefore){$at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);while((Get-Date) -lt $at){Start-Sleep 5}}
    foreach($row in $Rows){ $label,$press=$row -split '=',2
        if($label -notmatch '^[a-z0-9-]+$' -or $press -notmatch '^[0-9]{3,4},[0-9]{3}$'){& $say "$row refused: label=tabX,rowY";continue}
        $p=@($press -split ',');$tp="$($p[0]),191;400,$($p[1])"
        $modes=@('Native');if(-not $NoReference){$modes+='Reference'}
        foreach($mode in $modes){
            if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){& $say "$label $mode skipped: DarkSoulsIII already runs";continue}
            $run="$Prefix-$label-$(if($mode -eq 'Native'){'nat'}else{'orig'})";& $say "$(Get-Date -Format HH:mm:ss) $run"
            $castArgs=@{RunName=$run;BootstrapMode=$mode;SaveCopy=$eta;SaveCopySha256=$etaHash;Continue=$true;BonfirePages=$true;TravelPress=$tp
                ArriveSeconds=$ArriveSeconds;StaySeconds=10;LookSweep=$LookSweep;Casts=0;Seconds=600;BuildFolder=$BuildFolder}
            if($mode -ne 'Native' -and $ReferenceFlags){$castArgs.EffectFlags=$ReferenceFlags}; if($mode -eq 'Native'){$castArgs.EffectFlags=$NativeFlags;if(-not $NoKeep){$castArgs.KeepSaveLabel="eta-i139-$label"}}
            & $cast @castArgs *>&1 | ForEach-Object { & $say "$_" }
            $folder="$repo/evidence/I-0066/artifacts/$run"
            if($mode -eq 'Native' -and (Test-Path -LiteralPath $folder)){
                $rec="$folder/kept-saves.jsonl"
                if(Test-Path -LiteralPath $rec){$r=Get-Content -LiteralPath $rec | Select-Object -Last 1 | ConvertFrom-Json;& $say "$label kept $(Split-Path -Leaf (Split-Path -Parent $r.copy)) $($r.sha256)"}
                & python -I $reader $folder 2>&1 | ForEach-Object { & $say "  $_" } } } }
}catch{& $say "ERROR $_"}
finally{& $say "END $(Get-Date -Format HH:mm:ss)"}
