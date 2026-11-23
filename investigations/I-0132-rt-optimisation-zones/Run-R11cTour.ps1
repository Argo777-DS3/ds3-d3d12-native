# I-0132 R11c (R11-PLAN §4): the zones' tour. For each zone « name=tabX,rowX,rowY » (Eta's Travel page, pilot units): A, the travel
# from Eta's m40 zone save (Run-AlphaCast -Continue -BonfirePages -TravelPress, the RT level, the arrival's look sweep), the zone's
# bonfire kept as a hash-recorded test save (-KeepSaveLabel, WORKFLOW « Test characters' progress »); B, from that save, Run-R7aGi's
# views (Run-ZoneWalk: the settle, three looks, world-pictures) in -Modes (the RT level `taa`, the PT level `ptsrc` with dlss-dlaa,
# the original `orig`) at -Sizes. One process; -NotBefore HH:mm (same day); the ASUS by its key. Phase B alone with -SkipTravel and
# -ExtraSaves (Run-R7aGi's « zone=folder=sha256 »).
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Prefix,[Parameter(Mandatory)][string]$BuildFolder,
    [string[]]$Zones=@(),[string]$NotBefore='',[string]$Log='',[string]$Screen='1920x1080@60',
    [string]$TravelFlags='test-no-ai,world-pictures,rt-scene,rt-fail-magenta',[string[]]$Modes=@('taa','ptsrc','orig'),[string[]]$Sizes=@('1600x900'),
    [switch]$SkipTravel,[string[]]$ExtraSaves=@(),[ValidateRange(20,180)][int]$ArriveSeconds=45)
$ErrorActionPreference='Stop'
$Zones=@($Zones | ForEach-Object { $_ -split ' ' } | Where-Object { $_ });$Modes=@($Modes | ForEach-Object { $_ -split ',' } | Where-Object { $_ })
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$cast="$repo/investigations/I-0066-independent-native-startup/Run-AlphaCast.ps1";$views="$repo/investigations/I-0126-rt-gi/Run-R7aGi.ps1"
$eta='C:\Users\User\.ds3-i0039-backups\shebang-zones-eta-m40-20261014-095203\DS30000.sl2';$etaHash='1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4'
$say={param($t) if($Log){"$t" | Add-Content $Log}else{"$t"}}
try{
    $device=(& "$repo/scripts/Get-Screens.ps1" -Key $Screen).device
    if(-not $device){throw "no $Screen screen"}
    $env:DS3_PILOT_SCREEN=$device;& $say "$(Get-Date -Format HH:mm:ss) screen $device"
    if($NotBefore){$at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);while((Get-Date) -lt $at){Start-Sleep 5}}
    $kept=@($ExtraSaves | ForEach-Object { $_ -split ',' } | Where-Object { $_ })
    foreach($z in $Zones){ $name,$press=$z -split '=',2
        if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){& $say "$name skipped: DarkSoulsIII already runs";continue}
        if(-not $SkipTravel){
            $run="$Prefix-$name-travel";& $say "$(Get-Date -Format HH:mm:ss) $run"
            $p=@($press -split ',');$tp="$($p[0]),191;$($p[1]),$($p[2])"
            & $cast -RunName $run -SaveCopy $eta -SaveCopySha256 $etaHash -Continue -BonfirePages -TravelPress $tp -ArriveSeconds $ArriveSeconds -StaySeconds 10 `
                -LookSweep '4,-300,2' -Casts 0 -Seconds 600 -EffectFlags $TravelFlags -BuildFolder $BuildFolder -KeepSaveLabel "eta-r11c-$name" *>&1 | ForEach-Object { & $say "$_" }
            $rec="$repo/evidence/I-0066/artifacts/$run/kept-saves.jsonl"
            if(Test-Path -LiteralPath $rec){ $r=Get-Content -LiteralPath $rec | Select-Object -Last 1 | ConvertFrom-Json
                $folder=Split-Path -Leaf (Split-Path -Parent $r.copy);$kept+="$name=$folder=$($r.sha256)";& $say "$name kept $folder $($r.sha256)" }
            else{ & $say "${name}: no kept save (travel failed?)";continue } }
        $mine=@($kept | Where-Object { $_ -like "$name=*" })
        if(-not $mine){ & $say "${name}: no save for its views";continue }
        & $views -Prefix $Prefix -BuildFolder $BuildFolder -Zones $name -ExtraSaves $mine -Modes $Modes -Sizes $Sizes -Screen $Screen -Log $Log }
}catch{& $say "ERROR $_"}
finally{& $say "END $(Get-Date -Format HH:mm:ss)"}
