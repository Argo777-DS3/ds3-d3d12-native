# I-0139 tranche 5 (the endings-dlc sheet's D rows): the DLC entrances by their own events, natively then in the original. Each row:
# Eta's travel to the nearest bonfire, event flags set before the travel (-SetFlag) and after it (-SetFlagAfterTravel: an event that
# clears its flag when the map starts it, as m35's 13505100 does with 73500339), a teleport near the trigger, then -Steps (E for the
# action prompt, waits with shots). The flags the event reads are watched (witness-flag-watch.txt beside the module, removed after).
# Recipes: evidence/I-0139/artifacts/static/endings-dlc/dlc-entrances.md (a Sonnet worker; the event lines checked by the pilot).
# One process at a time; the ASUS by its key; evidence/I-0139/stop-boss.txt stops between two rows.
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Prefix,[Parameter(Mandatory)][string]$BuildFolder,[string]$Log='',
    [string[]]$Only=@(),[switch]$NoReference,[string]$Screen='1920x1080@60',[string]$NativeFlags='world-pictures,rt-scene,rt-fail-magenta')
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if(-not $Log){$Log="$repo/evidence/I-0139/artifacts/runs-d.log"}
$cast="$repo/investigations/I-0066-independent-native-startup/Run-AlphaCast.ps1"
$eta='C:\Users\User\.ds3-i0039-backups\shebang-zones-eta-m40-20261014-095203\DS30000.sl2';$etaHash='1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4'
$say={param($t) "$t" | Add-Content $Log}
$rows=@(
    # (Its -Teleport follows the after-flag, which starts the scene and the move to m45: protected only by the scene's length
    # against TeleportWait, the step review's L2; reached on 2026-11-15, not run again.)
    # D01: m35 event 13505100 (the player within 10 m of Gael, entity 3500725 at -258.0 -205.6 -593.8, and 73500339 on, which the event
    # clears when it starts; 1623 picks 35000050 or 35000051, INFERRED), then 2002[4] 35000050 with the move to 4502100 (Ariandel).
    @{Label='d01-ariandel-entry';Travel='503,191;400,313';Flags='1623 1';After='73500339 1';Point='-258.95 -205.62 -598.30';Steps='w20;69,200;w30;w30'
      Watch='73500339 1623 73500304 14500001'},
    # D03: m45 event 14505160 (9322 and 14500161 on, 14500162 off), E on the object 4501101 (-256.8 28.1 -230.1, text 9344), then the
    # warp 2003[14] to the Dreg Heap's region 5002110.
    @{Label='d03-ariandel-exit';Travel='923,191;400,445';Flags='9322 1;14500800 1;14500161 1;14500162 0;14500160 0';After='';Point='-254.8 27.4 -230.1'
      Steps='w5;69,200;w30;w30';Watch='9322 14500161 14500162 14500160 14500800'},
    # D04: m41 event 14105510 (14100511 on), E on the object 4101102 (-205.3 1558.9 -20.5, text 9344), then the warp to 5002110.
    @{Label='d04-kiln-dregheap';Travel='863,191;400,280';Flags='14100511 1;14100512 0;14100510 0';After='';Point='-204.3 1559.4 -21.5'
      Steps='w5;69,200;w30;w30';Watch='14100511 14100512 14100510'},
    # D06: m50 event 15005100 (15000800 on, the goods 2156 held: given by F7, test-give.txt « g2156 »), E on the object 5001100
    # (262.4 -339.4 122.1, text 9380), then 2002[4] 51000000 with the move to 5102110 (the Ringed City). The point 1.5 m short of the
    # object on the bonfire's side (the first try's point past it fell; the final review's H1).
    @{Label='d06-ringed-entry';Travel='983,191;400,379';Flags='15000800 1';After='';Point='261.15 -339.1 121.27';Give='g2156'
      Steps='w5;69,200;w30;w30';Watch='15000800'},
    # D03 again: closer to the object (0.9 m), then in a run of its own on its other side (two rows: a second teleport after a
    # successful warp would land in the Dreg Heap, the step review's M1).
    @{Label='d03b-ariandel-exit';Travel='923,191;400,445';Flags='9322 1;14500800 1;14500161 1;14500162 0;14500160 0';After='';Point='-255.9 27.4 -230.1'
      Steps='w5;69,200;w30;w30';Watch='9322 14500161 14500162 14500160 14500800'},
    @{Label='d03c-ariandel-exit';Travel='923,191;400,445';Flags='9322 1;14500800 1;14500161 1;14500162 0;14500160 0';After='';Point='-257.7 27.4 -230.1'
      Steps='w5;69,200;w30;w30';Watch='9322 14500161 14500162 14500160 14500800'})
$watchPath=Join-Path $BuildFolder 'witness-flag-watch.txt';$script:made=@()   # the files made here, removed only those (the step review's L1)
try{
    $device=(& "$repo/scripts/Get-Screens.ps1" -Key $Screen).device;if(-not $device){throw "no $Screen screen"};$env:DS3_PILOT_SCREEN=$device
    foreach($r in $rows){
        if($Only -and $Only -notcontains $r.Label){continue}
        if(Test-Path -LiteralPath "$repo/evidence/I-0139/stop-boss.txt"){& $say "stopped before $($r.Label): stop-boss.txt";break}
        $modes=@('Native');if(-not $NoReference){$modes+='Reference'}
        foreach($mode in $modes){
            if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){& $say "$($r.Label) $mode skipped: DarkSoulsIII already runs";continue}
            $run="$Prefix-$($r.Label)-$(if($mode -eq 'Native'){'nat'}else{'orig'})";& $say "$(Get-Date -Format HH:mm:ss) $run"
            # every test file refused when another pilot left it (the final review's L2), then this row's own made
            $tests=@('witness-flag-watch.txt','test-tools.flag','test-revive.txt','test-give.txt' | Where-Object { Test-Path -LiteralPath (Join-Path $BuildFolder $_) })
            if($tests){& $say "$run refused: $($tests -join ', ') already beside the module";continue}
            # each file recorded once made, so a file another pilot wrote in between is never removed (the step review's L1)
            New-Item -ItemType File -Path $watchPath -Value $r.Watch | Out-Null;$script:made=@($watchPath)
            if($r.Give){foreach($g in @(@('test-tools.flag','test'),@('test-give.txt',$r.Give))){$p=Join-Path $BuildFolder $g[0]
                New-Item -ItemType File -Path $p -Value $g[1] | Out-Null;$script:made+=$p}}
            $a=@{RunName=$run;BootstrapMode=$mode;SaveCopy=$eta;SaveCopySha256=$etaHash;Continue=$true;BonfirePages=$true;TravelPress=$r.Travel
                ArriveSeconds=45;StaySeconds=5;Casts=0;Seconds=600;BuildFolder=$BuildFolder;Teleport="$($r.Point) 6";TeleportWait=10;KeySteps=$r.Steps
                SetFlag=$r.Flags;FlagShots=@(5)}
            if($r.After){$a.SetFlagAfterTravel=$r.After}
            if($r.Give){$a.GiveItems=$true;$a.GiveConfirms=1}
            if($mode -eq 'Native'){$a.EffectFlags=$NativeFlags}
            try{ & $cast @a *>&1 | ForEach-Object { & $say "$_" } }
            finally{ foreach($f in $script:made){Remove-Item -LiteralPath $f -Force -ErrorAction SilentlyContinue};$script:made=@() }
            $folder="$repo/evidence/I-0066/artifacts/$run"
            if(Test-Path -LiteralPath "$folder/startup.jsonl"){
                foreach($l in @(Select-String -LiteralPath "$folder/startup.jsonl" -Pattern '"event":"(witness_(flag|flag_watch|flag_state|teleport)|test_give(_done)?)"')){ & $say "  $($l.Line)" } } } }
}catch{& $say "ERROR $_"}
finally{ foreach($f in @($script:made)){Remove-Item -LiteralPath $f -Force -ErrorAction SilentlyContinue}; & $say "END $(Get-Date -Format HH:mm:ss)" }
