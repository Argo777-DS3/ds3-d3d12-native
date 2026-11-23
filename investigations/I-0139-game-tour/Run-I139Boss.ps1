# I-0139 tranche 2 (sheets/bosses.md): a boss's entry, phases and death, natively then in the original. For each row
# « label=tabX,rowY|x y z|flag » (Eta's Travel tab and row of the nearest bonfire, a teleport in the teleport frame near the fog gate,
# the defeat flag): test-tools.flag and test-revive.txt (the flag, one of the 26 registered) next to the module of -BuildFolder (as
# I-0089's Run-TestTools.ps1; removed after), Eta's travel to that bonfire (the zone reloads with the boss revived), the teleport,
# then -Steps (Run-AlphaCast -KeySteps: F4 invincible, E at the fog, waits with shots, F6 the kill). A flag outside the 26 is cleared
# with Run-AlphaCast -SetFlag "<flag> 0" before the travel instead (Stray Demon). The teleport pins the player for -TeleportHold
# seconds (witness-teleport.txt), so the steps start after it (trial 1: steps inside a 20 s hold could not move him). One process; -NotBefore HH:mm; the ASUS by its key.
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Prefix,[Parameter(Mandatory)][string]$BuildFolder,
    [Parameter(Mandatory)][string[]]$Rows,[string]$NotBefore='',[string]$Log='',[string]$Screen='1920x1080@60',
    [string]$NativeFlags='world-pictures,rt-scene,rt-fail-magenta',[switch]$NoReference,
    [string]$Steps='115,0;90,300;69,200;w30;T;117,0;w40;117,0;w40;117,0;w40;117,0;w40',[ValidateRange(1,120)][int]$TeleportHold=6,[string]$LookAfter='',   # trial 1: a sweep after the teleport turned the character from the fog
    # The night's end (Thomas, 2026-11-13 00:09: up to 06:00): no row starts unless it can end before -StopAt ("yyyy-MM-dd HH:mm") with
    # -RowMinutes for its runs; a file `stop-boss.txt` in evidence/I-0139 stops the series between two rows (a slot given back early).
    [string]$StopAt='',[ValidateRange(4,30)][int]$RowMinutes=12,
    # More event flags set before the travel, « id value;id value » (Run-AlphaCast -SetFlag): the Nameless King appears only once the
    # Great Belfry's bell flag 13200445 is on (m32 event 13205860), which a revive does not set.
    [string]$ExtraFlags='',
    # The phases and the death read by their event flags (the native test tool witness-flag-watch.txt, both modes): up to 16 ids,
    # space-separated; the module samples them every 60th schedule call and writes witness_flag_state rows at the first sample and at
    # each change (value -1: no block holds that flag, or its words unreadable), all printed after the run.
    [ValidatePattern('^([0-9]{1,8}( [0-9]{1,8}){0,15})?$')][string]$WatchFlags='',
    # Flags set once arrived (Run-AlphaCast -SetFlagAfterTravel): a flag a map event clears at its start (m45 event 14505750 clears
    # Friede's talk flag 74500300 unless 14500801 is on), the final review's H1 of 2026-11-15.
    [ValidatePattern('^([0-9]{1,8} [01](;[0-9]{1,8} [01])*)?$')][string]$AfterFlags='')
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$cast="$repo/investigations/I-0066-independent-native-startup/Run-AlphaCast.ps1"
$reader="$repo/investigations/I-0132-rt-optimisation-zones/Get-R11cJournal.py"
$eta='C:\Users\User\.ds3-i0039-backups\shebang-zones-eta-m40-20261014-095203\DS30000.sl2';$etaHash='1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4'
$registered=@(13000800,13000830,13000890,13010800,13100800,13200800,13200850,13300800,13300850,13410830,13410860,13500800,13700800,13700850,13800800,13800830,13900800,14000800,14000830,14100800,14500800,14500860,15000800,15100800,15100850,15110800)
$say={param($t) if($Log){"$t" | Add-Content $Log}else{"$t"}}
$flagPath=Join-Path $BuildFolder 'test-tools.flag';$revivePath=Join-Path $BuildFolder 'test-revive.txt';$watchPath=Join-Path $BuildFolder 'witness-flag-watch.txt';$script:made=@()
try{
    $device=(& "$repo/scripts/Get-Screens.ps1" -Key $Screen).device
    if(-not $device){throw "no $Screen screen"}
    $env:DS3_PILOT_SCREEN=$device;& $say "$(Get-Date -Format HH:mm:ss) screen $device"
    if($NotBefore){$at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);while((Get-Date) -lt $at){Start-Sleep 5}}
    foreach($row in @($Rows | ForEach-Object { $_ -split ' (?=[a-z0-9-]+=)' } | Where-Object { $_ })){
        if($row -notmatch '^([a-z0-9-]+)=([0-9]{3,4}),([0-9]{3})\|(-?[0-9.]+ -?[0-9.]+ -?[0-9.]+)\|([0-9]{8})(\|(-?[0-9.]+ -?[0-9.]+ -?[0-9.]+))?$'){& $say "$row refused: label=tabX,rowY|x y z|flag[|x y z]";continue}
        $label=$Matches[1];$tp="$($Matches[2]),191;400,$($Matches[3])";$at3=$Matches[4];$flag=[int]$Matches[5];$near=$Matches[7]
        if(Test-Path -LiteralPath "$repo/evidence/I-0139/stop-boss.txt"){& $say "stopped before ${label}: stop-boss.txt";break}
        if($StopAt -and (Get-Date).AddMinutes($RowMinutes) -gt [datetime]::ParseExact($StopAt,'yyyy-MM-dd HH:mm',$null)){& $say "stopped before ${label}: $StopAt";break}
        $modes=@('Native');if(-not $NoReference){$modes+='Reference'}
        foreach($mode in $modes){
            if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){& $say "$label $mode skipped: DarkSoulsIII already runs";continue}
            $run="$Prefix-$label-$(if($mode -eq 'Native'){'nat'}else{'orig'})";& $say "$(Get-Date -Format HH:mm:ss) $run"
            # The test files made here and removed only if made here (one left by another pilot is refused, not inherited; D-0161,
            # the step review's L3 of 2026-11-15).
            if((Test-Path -LiteralPath $flagPath) -or (Test-Path -LiteralPath $revivePath) -or (Test-Path -LiteralPath $watchPath) -or (Test-Path -LiteralPath (Join-Path $BuildFolder 'test-give.txt'))){& $say "$run refused: a test file already beside the module";continue}
            New-Item -ItemType File -Path $flagPath -Value 'test' | Out-Null;$script:made=@($flagPath)
            if($WatchFlags){New-Item -ItemType File -Path $watchPath -Value $WatchFlags | Out-Null;$script:made+=$watchPath}
            $castArgs=@{RunName=$run;BootstrapMode=$mode;SaveCopy=$eta;SaveCopySha256=$etaHash;Continue=$true;BonfirePages=$true;TravelPress=$tp
                ArriveSeconds=45;StaySeconds=5;Casts=0;Seconds=900;BuildFolder=$BuildFolder;Teleport="$at3 $TeleportHold";TeleportWait=([Math]::Min(30,$TeleportHold+4));KeySteps=$Steps}
            if($near){$castArgs.Teleport2="$near 3"}   # the second point, near the boss (T in -Steps); without it T does nothing
            if($LookAfter){$castArgs.LookSweepAfter=$LookAfter}
            $setFlags=@()
            if($registered -contains $flag){New-Item -ItemType File -Path $revivePath -Value "$flag" | Out-Null;$script:made+=$revivePath}else{$setFlags+="$flag 0"}
            if($ExtraFlags){$setFlags+=@($ExtraFlags -split ';' | Where-Object { $_.Trim() })}
            if($setFlags){$castArgs.SetFlag=$setFlags -join ';';$castArgs.FlagShots=@(5)}
            if($AfterFlags){$castArgs.SetFlagAfterTravel=$AfterFlags}
            if($mode -eq 'Native'){$castArgs.EffectFlags=$NativeFlags}
            try{ & $cast @castArgs *>&1 | ForEach-Object { & $say "$_" } }
            finally{ foreach($f in $script:made){ Remove-Item -LiteralPath $f -Force -ErrorAction SilentlyContinue }; $script:made=@() }
            $folder="$repo/evidence/I-0066/artifacts/$run"
            if(Test-Path -LiteralPath "$folder/startup.jsonl"){
                foreach($l in @(Select-String -LiteralPath "$folder/startup.jsonl" -Pattern '"event":"test_(revive|invincible|kill)' | Select-Object -First 6)){ & $say "  $($l.Line)" }
                # every flag row (the step review's L2: a cap shared with the test rows dropped the death's change first)
                foreach($l in @(Select-String -LiteralPath "$folder/startup.jsonl" -Pattern '"event":"witness_flag(_watch|_state)?"')){ & $say "  $($l.Line)" }
                if($mode -eq 'Native'){ & python -I $reader $folder 2>&1 | ForEach-Object { & $say "  $_" } } } } }
}catch{& $say "ERROR $_"}
finally{ foreach($f in @($script:made)){ Remove-Item -LiteralPath $f -Force -ErrorAction SilentlyContinue }; & $say "END $(Get-Date -Format HH:mm:ss)" }
