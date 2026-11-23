# I-0138 tranche 8b: the original's YEBIS draws captured in the cutscenes whose records our post plans still refuse (the light
# shafts' corona at m38, glare shape 3 at Archdragon Peak, the shafts without depth of field). Run-I138Cutscenes.ps1's oracle (the
# scratch build of the recovery commit 6a3b127a with yebis-hal-observe and yebis-draws-capture: every library draw of the frame
# dumped, reference-draw-NN-*, its blend logged). The oracle tree's Run-AlphaCast played -Cutscene before -BonfirePages/-TravelPress
# (the step review's H1: the six runs of 17:42 played their scenes at m40); patched in the scratch tree on 2026-11-16 to play it after
# the arrival with -TravelPress (evidence/I-0138/artifacts/t8-oracle-runner.diff). Original only. One process a run; the ASUS by its key; offline, restored by
# hash. Rows « label=tabX,rowY|cutscene id|capture delays (library frames after the scene is handed, comma) ».
# 2026-11-21 (the corona in play at m38): a delay past the scene's end captures the play that follows it, at the arrival's bonfire;
# -CutsceneShots then reaches past the delay so that the runner keeps the game open until the capture (one shot a listed second).
param([Parameter(Mandatory)][string]$OracleTree,[Parameter(Mandatory)][string]$OracleBuild,[Parameter(Mandatory)][string[]]$Rows,
    [string]$Log='',[string]$Screen='1920x1080@60',[ValidatePattern('^[a-z0-9-]+$')][string]$Prefix='i138-8b',
    [ValidateRange(1,170)][int[]]$CutsceneShots=@(2,5,10,15,20,30,45,60))
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$oracleCast="$OracleTree/investigations/I-0066-independent-native-startup/Run-AlphaCast.ps1"
$eta='C:\Users\User\.ds3-i0039-backups\shebang-zones-eta-m40-20261014-095203\DS30000.sl2';$etaHash='1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4'
$say={param($t) if($Log){"$t" | Add-Content $Log}else{"$t"}}
$delayFile=Join-Path $OracleBuild 'yebis-capture-cutscene.txt';$cutsceneFile=Join-Path $OracleBuild 'witness-cutscene.txt'
try{
    if(-not (Test-Path -LiteralPath $oracleCast)){throw "no oracle runner at $oracleCast"}
    $device=(& "$repo/scripts/Get-Screens.ps1" -Key $Screen).device
    if(-not $device){throw "no $Screen screen"}
    $env:DS3_PILOT_SCREEN=$device;& $say "$(Get-Date -Format HH:mm:ss) screen $device"
    foreach($row in @($Rows | Where-Object { $_ })){
        if($row -notmatch '^([a-z0-9-]+)=([0-9]{3,4}),([0-9]{3})\|((3[0-9]|4[0-9]|5[01])[0-9]{6})\|([0-9]{1,5}(,[0-9]{1,5})*)$'){& $say "$row refused: label=tabX,rowY|id|delays";continue}
        $label=$Matches[1];$tp="$($Matches[2]),191;400,$($Matches[3])";$scene=$Matches[4];$delays=@($Matches[6] -split ',')
        foreach($d in $delays){
            if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){& $say "$label $d skipped: DarkSoulsIII already runs";continue}
            $run="$Prefix-$label-orig-$d"
            if(Test-Path -LiteralPath "$repo/evidence/I-0066/artifacts/$run/restoration.json"){& $say "$run done before: skipped";continue}
            & $say "$(Get-Date -Format HH:mm:ss) $run"
            Set-Content -LiteralPath $delayFile -Value $d -NoNewline -Encoding ascii
            try{
                $castArgs=@{RunName=$run;BootstrapMode='Reference';SaveCopy=$eta;SaveCopySha256=$etaHash;Continue=$true;BonfirePages=$true;TravelPress=$tp
                    ArriveSeconds=45;StaySeconds=5;Casts=0;Seconds=420;BuildFolder=$OracleBuild;EffectFlags='yebis-hal-observe,yebis-draws-capture,test-no-ai'
                    Cutscene=$scene;CutsceneShots=$CutsceneShots}
                & $oracleCast @castArgs *>&1 | ForEach-Object { & $say "$_" } }
            finally{Remove-Item -LiteralPath $delayFile,$cutsceneFile -Force -ErrorAction SilentlyContinue}
            $folder="$repo/evidence/I-0066/artifacts/$run"
            if(Test-Path -LiteralPath "$folder/startup.jsonl"){
                foreach($l in @(Select-String -LiteralPath "$folder/startup.jsonl" -Pattern '"event":"(witness_cutscene|reference_yebis_cutscene_capture|native_render_job_failure)"' | Select-Object -First 4)){ & $say "  $($l.Line)" }
                & $say "  draws captured: $(@(Select-String -LiteralPath "$folder/startup.jsonl" -Pattern '"event":"reference_yebis_draw_capture"').Count)" } } }
}catch{& $say "ERROR $_"}
finally{Remove-Item -LiteralPath $delayFile,$cutsceneFile -Force -ErrorAction SilentlyContinue;& $say "END $(Get-Date -Format HH:mm:ss)"}
