# I-0139, the fresh tour thread's series (2026-11-15): the Twin Princes' fight after their scene and the second phases' deaths read
# by their event flags (Run-I139Boss -WatchFlags, the native test tool witness-flag-watch.txt), then the Fire Keeper's talk
# (Run-AlphaCast -TalkKeeper on the m45 zone save, Alpha's copy restored by hash), each natively then in the original. One process at a
# time; -Part picks a block so the coordinator's slots can split the series; evidence/I-0139/stop-boss.txt stops between two rows.
param([Parameter(Mandatory)][string]$BuildFolder,[string]$Log='',[ValidateSet('all','bosses','keeper','friede')][string]$Part='all',
    # the keeper again (her place is random each load): the modes to run and a tag in the run names (e.g. -KeeperModes Reference -KeeperTag b)
    [ValidateSet('Native','Reference')][string[]]$KeeperModes=@('Native','Reference'),[ValidatePattern('^[a-z0-9]*$')][string]$KeeperTag='')
$ErrorActionPreference='Stop'
$here=$PSScriptRoot;$repo=Split-Path -Parent (Split-Path -Parent $here)
if(-not $Log){$Log="$repo/evidence/I-0139/artifacts/runs-s6.log"}
$boss="$here/Run-I139Boss.ps1";$cast="$repo/investigations/I-0066-independent-native-startup/Run-AlphaCast.ps1"
# F4, a step, E at the fog, the scene's wait, T to the second point, then F6 rounds 30 s apart (the Princes: 75 s for scene 34010000,
# 60 s after the third F6 for the mid-fight scene 34010010).
$rounds='117,0;w30;117,0;w30;117,0;w30;117,0;w30;117,0;w30;117,0;w30;117,0;w30'
$princes='115,0;90,300;69,200;w75;T;117,0;w30;117,0;w30;117,0;w60;117,0;w30;117,0;w30;117,0;w30;117,0;w30'
try{
$rows=@(
    @{Prefix='i139-t2i';Row=(Get-Content "$here/tranche2-princes.txt" -Raw).Trim();Steps=$princes
      Extra='13410831 0;13415835 0;13415836 0';Watch='13410830 13410831 13415835 13415836 13415830 13415831'},
    @{Prefix='i139-t2i';Row=(Get-Content "$here/tranche2-namelessking.txt" -Raw).Trim();Steps="115,0;90,300;69,200;w30;T;$rounds"
      Extra='13200855 0;13200856 0;13205855 0;13205851 0;13205852 0;13200440 1;13200445 1;13200446 1';Watch='13200850 13205852 13200855 13200856 13205855 13205851'},
    @{Prefix='i139-t2i';Row=(Get-Content "$here/tranche2-gravetender.txt" -Raw).Trim();Steps="115,0;90,300;69,200;w30;T;$rounds"
      Extra='14500861 0;14505861 0;14505863 0;14505865 0';Watch='14500860 14500861 14505861 14505863 14505865'},
    # Friede's intro 45000000 in the game's own request form (m45 event 14505810: 14500800 and 14500801 off, the talk flag 74500300 on
    # (its role INFERRED), the action 9373 on entity 4500803 (c6010_0001 at -246.7 27.3 -230.0), then 2002[4] 45000000 with the warp
    # to region 4502810): the player at the arena's player start c0000_0001 (-239.9 27.4 -230.1, 7 m), E twice, then F6 rounds.
    @{Prefix='i139-t3f';Row='b22-friede-intro=923,412|-239.9 27.4 -230.1|14500800';Steps='115,0;69,200;w30;69,200;w45;117,0;w30;117,0;w30'
      Extra='14500801 0;74500300 1;14505521 0';Watch='14500800 14500801 74500300 14505521'})
if($Part -eq 'friede'){   # the final review's H1: Friede's talk flag set once arrived, her intro by her event
    & $boss -Prefix 'i139-t3g' -BuildFolder $BuildFolder -Rows 'b22-friede-intro=923,412|-239.9 27.4 -230.1|14500800|-245.2 27.4 -230.0' -Steps '115,0;T;69,200;w30;69,200;w45;117,0;w30;117,0;w30' -ExtraFlags '14500801 0;14505521 0' -AfterFlags '74500300 1' -WatchFlags '14500800 14500801 74500300 14505521' -Log $Log -RowMinutes 20 }
if($Part -in 'all','bosses'){
    foreach($r in $rows){
        if(Test-Path -LiteralPath "$repo/evidence/I-0139/stop-boss.txt"){"stopped: stop-boss.txt" | Add-Content $Log;break}
        & $boss -Prefix $r.Prefix -BuildFolder $BuildFolder -Rows $r.Row -Steps $r.Steps -ExtraFlags $r.Extra -WatchFlags $r.Watch -Log $Log -RowMinutes 20 } }
if($Part -in 'all','keeper'){
    $device=(& "$repo/scripts/Get-Screens.ps1" -Key '1920x1080@60').device;if(-not $device){throw 'no ASUS screen'};$env:DS3_PILOT_SCREEN=$device   # as Run-I139Boss
    foreach($mode in $KeeperModes){
        if(Test-Path -LiteralPath "$repo/evidence/I-0139/stop-boss.txt"){"stopped: stop-boss.txt" | Add-Content $Log;break}
        if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){"keeper $mode skipped: DarkSoulsIII already runs" | Add-Content $Log;continue}
        $tests=@('test-tools.flag','test-revive.txt','witness-flag-watch.txt','test-give.txt' | Where-Object { Test-Path -LiteralPath (Join-Path $BuildFolder $_) })
        if($tests){"keeper $mode refused: $($tests -join ', ') already beside the module" | Add-Content $Log;continue}   # as Run-I139Boss (the step review's L4)
        $run="i139-t4k$KeeperTag-keeper-$(if($mode -eq 'Native'){'nat'}else{'orig'})";"$(Get-Date -Format HH:mm:ss) $run" | Add-Content $Log
        $a=@{RunName=$run;BootstrapMode=$mode;TalkKeeper=$true;Casts=0;Seconds=240;BuildFolder=$BuildFolder}
        if($mode -eq 'Native'){$a.EffectFlags='world-pictures,rt-scene,rt-fail-magenta'}
        try{ & $cast @a *>&1 | ForEach-Object { "$_" | Add-Content $Log } }catch{ "ERROR $run $_" | Add-Content $Log } } }
}catch{"ERROR $_" | Add-Content $Log}
finally{"END $(Get-Date -Format HH:mm:ss)" | Add-Content $Log}
