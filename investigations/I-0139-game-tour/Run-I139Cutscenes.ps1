# I-0139 tranche 3 (sheets/cutscenes-events.md): a Remo cutscene by its id. For each row « label=tabX,rowY|id[ mode] » (Eta's Travel tab
# and row of a bonfire in the cutscene's map, so the map is loaded): Eta's travel there, then Run-AlphaCast -Cutscene (witness-cutscene.txt:
# the module plays the cutscene through the game's own EMEVD handler, the player its entity), shots through the scene; natively then in
# the original. One process; the ASUS by its key; restored by hash.
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Prefix,[Parameter(Mandatory)][string]$BuildFolder,
    [Parameter(Mandatory)][string[]]$Rows,[string]$Log='',[string]$Screen='1920x1080@60',
    # The OLED as the only screen (Thomas, 2026-11-14): -Screen '2560x1440@<hz>' and the window at its size, e.g.
    # 'ScreenMode=WINDOW;Resolution-WindowScreenWidth=2560;Resolution-WindowScreenHeight=1440' (Run-AlphaCast -GraphicsLevels; the pilot
    # scales its clicks onto the client).
    [string]$GraphicsLevels='',[switch]$NoReference,[switch]$NoNative,
    [int[]]$Shots=@(2,5,10,15,20,25,30,40,50,60,75,90),
    # I-0140: the runs' test flags, in place of the default test-no-ai (give it with the others: 'test-no-ai,effect-census');
    # Run-StartupTrace's pattern checks them.
    [string]$EffectFlags='test-no-ai',
    # I-0138 tranche 8g (D-0299): Run-AlphaCast -CutsceneHolds "ms t1 t2 ..." (the scene held at those scene frames, a shot in each
    # hold) in place of -Shots' wall-clock shots; the same in both modes.
    [string]$Holds='',[int]$HoldShotMs=2500)
$ErrorActionPreference='Stop'
if($NoNative -and $NoReference){throw 'Run-I139Cutscenes: -NoNative and -NoReference leave no run'}
if($EffectFlags -and $EffectFlags -notmatch '^[a-z0-9-]+(,[a-z0-9-]+)*$'){throw "Run-I139Cutscenes: -EffectFlags '$EffectFlags' is not a flag list"}
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$cast="$repo/investigations/I-0066-independent-native-startup/Run-AlphaCast.ps1"
$eta='C:\Users\User\.ds3-i0039-backups\shebang-zones-eta-m40-20261014-095203\DS30000.sl2';$etaHash='1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4'
$say={param($t) if($Log){"$t" | Add-Content $Log}else{"$t"}}
try{
    $device=(& "$repo/scripts/Get-Screens.ps1" -Key $Screen).device
    if(-not $device){throw "no $Screen screen"}
    $env:DS3_PILOT_SCREEN=$device;& $say "$(Get-Date -Format HH:mm:ss) screen $device"
    foreach($row in @($Rows | Where-Object { $_ })){
        if($row -notmatch '^([a-z0-9-]+)=([0-9]{3,4}),([0-9]{3})\|((3[0-9]|4[0-9]|5[01])[0-9]{6}( (0|2|8|10))?)$'){& $say "$row refused: label=tabX,rowY|id[ mode]";continue}
        $label=$Matches[1];$tp="$($Matches[2]),191;400,$($Matches[3])";$scene=$Matches[4]
        # A pause asked between two rows (the coordinator: other pilots' slots): `stop-tour.txt` in evidence/I-0139; a row whose runs both
        # have their restoration record is skipped, so the same command resumes the series.
        if(Test-Path -LiteralPath "$repo/evidence/I-0139/stop-tour.txt"){& $say "stopped before ${label}: stop-tour.txt";break}
        $modes=@();if(-not $NoNative){$modes+='Native'};if(-not $NoReference){$modes+='Reference'}
        foreach($mode in $modes){
            if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){& $say "$label $mode skipped: DarkSoulsIII already runs";continue}
            $run="$Prefix-$label-$(if($mode -eq 'Native'){'nat'}else{'orig'})"
            if(Test-Path -LiteralPath "$repo/evidence/I-0066/artifacts/$run/restoration.json"){& $say "$run done before: skipped";continue}
            & $say "$(Get-Date -Format HH:mm:ss) $run"
            $castArgs=@{RunName=$run;BootstrapMode=$mode;SaveCopy=$eta;SaveCopySha256=$etaHash;Continue=$true;BonfirePages=$true;TravelPress=$tp
                ArriveSeconds=45;StaySeconds=5;Casts=0;Seconds=600;BuildFolder=$BuildFolder;EffectFlags=$EffectFlags;Cutscene=$scene;CutsceneShots=$Shots}
            if($GraphicsLevels){$castArgs.GraphicsLevels=$GraphicsLevels}
            if($Holds){$castArgs.CutsceneHolds=$Holds;$castArgs.CutsceneHoldShotMs=$HoldShotMs}
            & $cast @castArgs *>&1 | ForEach-Object { & $say "$_" }
            $folder="$repo/evidence/I-0066/artifacts/$run"
            if(Test-Path -LiteralPath "$folder/startup.jsonl"){ foreach($l in @(Select-String -LiteralPath "$folder/startup.jsonl" -Pattern '"event":"witness_cutscene"|"event":"witness_cutscene_cuts"|"event":"native_render_job_failure"' | Select-Object -First 4)){ & $say "  $($l.Line)" } } } }
}catch{& $say "ERROR $_"}
finally{& $say "END $(Get-Date -Format HH:mm:ss)"}
