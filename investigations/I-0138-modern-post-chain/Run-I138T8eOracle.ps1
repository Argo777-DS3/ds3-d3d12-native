# I-0138 tranche 8e: the original's library draws captured on a shape-3 frame (the « Standard » glare) of cutscene 32000020.
# In i138-t8e-c32000020-orig the stored shape S+0xBC4 holds 3 from the keeper's call 8170 to 10930 (INFERRED: about 2760
# library frames, one keeper call each; one context S in the keeper rows up to their cap), just after the scene is handed. The original: the part-4b oracle (the scratch build of the recovery commit 6a3b127a,
# its own Run-AlphaCast with -Cutscene; yebis-capture-cutscene.txt = the capture frame N library frames after the scene is handed;
# flags yebis-hal-observe and yebis-draws-capture: every library draw of that frame dumped, reference-draw-NN-*), as
# Run-I138Cutscenes.ps1 but reached as Run-I139Cutscenes reaches 32000020: Shebang's eta save (m40), Travel's page 803 row 280.
# One run per delay; offline, the ASUS by its key, the save restored by hash (Run-AlphaCast -> Run-StartupTrace).
param([Parameter(Mandatory)][string]$OracleTree,[Parameter(Mandatory)][string]$OracleBuild,[Parameter(Mandatory)][int[]]$Delays,
    [string]$Prefix='i138-t8e-oracle',[string]$Log='',[string]$Screen='1920x1080@60')
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$oracleCast="$OracleTree/investigations/I-0066-independent-native-startup/Run-AlphaCast.ps1"
$eta='C:\Users\User\.ds3-i0039-backups\shebang-zones-eta-m40-20261014-095203\DS30000.sl2';$etaHash='1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4'
$capture=Join-Path $OracleBuild 'yebis-capture-cutscene.txt';$witness=Join-Path $OracleBuild 'witness-cutscene.txt'   # both removed after each run and at the end (the oracle's Run-AlphaCast removes the witness only on its normal path)
$say={param($t) if($Log){"$t" | Add-Content $Log}else{"$t"}}
try{
    if(-not (Test-Path -LiteralPath $oracleCast)){throw "no oracle Run-AlphaCast at $oracleCast"}
    $device=(& "$repo/scripts/Get-Screens.ps1" -Key $Screen).device
    if(-not $device){throw "no $Screen screen"}
    Remove-Item -LiteralPath $capture,$witness -Force -ErrorAction SilentlyContinue   # none left by an earlier, killed process
    $env:DS3_PILOT_SCREEN=$device;& $say "$(Get-Date -Format HH:mm:ss) screen $device"
    foreach($d in $Delays){
        if($d -lt 1 -or $d -gt 5000){& $say "delay $d refused";continue}
        if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){& $say "delay $d skipped: DarkSoulsIII already runs";continue}
        $run="$Prefix-$d"
        # Tranche 8i: the oracle's Run-AlphaCast leaves the cutscene after its last shot, so the shots run past the capture frame (about
        # 60 library frames a second after the scene is handed, plus 10 s); the 8i lit captures at 2120 to 2450 got no draw with shots
        # ending at 30 s (their journals end near library frame 9900, before the capture frame 10178 to 10497).
        $shots=@(@(2,5,10,15,20,30)+@(45..120 | Where-Object { $_ % 15 -eq 0 -and $_ -le [math]::Ceiling($d/60)+25 }) | Sort-Object -Unique)
        $past=[math]::Ceiling($d/60)+10;if($shots[-1] -lt $past){$shots+=$past}
        if($shots[-1]+200 -gt 900){& $say "delay $d refused: its last shot at $($shots[-1]) s leaves the 900 s trace no margin";continue}
        # The trace's length follows the last shot (the scene handed about 135 s after the start in the 8i runs, OBSERVED: library frame
        # about 8050; 200 s of margin), 300 s at least.
        if(Test-Path -LiteralPath "$repo/evidence/I-0066/artifacts/$run/restoration.json"){& $say "$run done before: skipped";continue}
        & $say "$(Get-Date -Format HH:mm:ss) $run"
        Set-Content -LiteralPath $capture -Value "$d" -NoNewline -Encoding ascii
        $castArgs=@{RunName=$run;BootstrapMode='Reference';SaveCopy=$eta;SaveCopySha256=$etaHash;Continue=$true;BonfirePages=$true;TravelPress='803,191;400,280'
            ArriveSeconds=45;StaySeconds=5;Casts=0;Seconds=[math]::Min(900,[math]::Max(300,$shots[-1]+200));BuildFolder=$OracleBuild;EffectFlags='yebis-hal-observe,yebis-draws-capture,test-no-ai'
            Cutscene='32000020';CutsceneShots=$shots}
        & $oracleCast @castArgs *>&1 | ForEach-Object { & $say "$_" }
        Remove-Item -LiteralPath $capture,$witness -Force -ErrorAction SilentlyContinue
        $folder="$repo/evidence/I-0066/artifacts/$run"
        if(Test-Path -LiteralPath "$folder/startup.jsonl"){
            foreach($l in @(Select-String -LiteralPath "$folder/startup.jsonl" -Pattern '"event":"(witness_cutscene|reference_yebis_cutscene_capture|native_render_job_failure)"' | Select-Object -First 4)){ & $say "  $($l.Line)" }
            & $say "  draws captured: $(@(Select-String -LiteralPath "$folder/startup.jsonl" -Pattern '"event":"reference_yebis_draw_capture"').Count)" } }
}catch{& $say "ERROR $_"}
finally{Remove-Item -LiteralPath $capture,$witness -Force -ErrorAction SilentlyContinue;& $say "END $(Get-Date -Format HH:mm:ss)"}
