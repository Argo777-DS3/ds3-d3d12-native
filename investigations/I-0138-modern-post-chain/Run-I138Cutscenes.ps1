# I-0138 tranche 4 part 4b: the original's YEBIS draws captured in a cutscene, then the same scenes natively. One process, the
# ASUS by its key, offline, every save restored by hash (Run-AlphaCast -> Run-StartupTrace).
# The original (Reference): the scratch build of the recovery commit 6a3b127a (I-0102: the capture oracles removed since) with
# I-0139's witness-cutscene.txt and yebis-capture-cutscene.txt (the capture frame N library frames after the scene is handed),
# its own Run-AlphaCast (-Cutscene), flags yebis-hal-observe and yebis-draws-capture: every library draw of that frame dumped
# (reference-draw-NN-*, reference-dof-*), as W10's captures were. Never merged; the tree is at -OracleTree.
# Natively: this branch's build, the current Run-AlphaCast -Cutscene.
# Rows « label|cutscene id|save|save SHA-256|capture delays (frames, comma) ».
param([Parameter(Mandatory)][string]$OracleTree,[Parameter(Mandatory)][string]$OracleBuild,[Parameter(Mandatory)][string]$NativeBuild,
    [Parameter(Mandatory)][string[]]$Rows,[string]$Log='',[string]$Screen='1920x1080@60',[switch]$NoNative,[switch]$NoReference)
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$cast="$repo/investigations/I-0066-independent-native-startup/Run-AlphaCast.ps1"
$oracleCast="$OracleTree/investigations/I-0066-independent-native-startup/Run-AlphaCast.ps1"
$say={param($t) if($Log){"$t" | Add-Content $Log}else{"$t"}}
try{
    $device=(& "$repo/scripts/Get-Screens.ps1" -Key $Screen).device
    if(-not $device){throw "no $Screen screen"}
    $env:DS3_PILOT_SCREEN=$device;& $say "$(Get-Date -Format HH:mm:ss) screen $device"
    foreach($row in @($Rows | Where-Object { $_ })){
        $v=$row -split '\|';if($v.Count -ne 5 -or $v[0] -notmatch '^[a-z0-9-]+$' -or $v[1] -notmatch '^(3[0-9]|4[0-9]|5[01])[0-9]{6}$'){& $say "$row refused";continue}
        $label,$scene,$save,$hash=$v[0],$v[1],$v[2],$v[3];$delays=@($v[4] -split ',' | Where-Object { $_ })
        $runs=@();if(-not $NoReference){foreach($d in $delays){$runs+=,@('Reference',"i138-4b-$label-orig-$d",$d)}};if(-not $NoNative){$runs+=,@('Native',"i138-4b-$label-nat",'')}
        foreach($r in $runs){
            if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){& $say "$($r[1]) skipped: DarkSoulsIII already runs";continue}
            & $say "$(Get-Date -Format HH:mm:ss) $($r[1])"
            $common=@{RunName=$r[1];BootstrapMode=$r[0];SaveCopy=$save;SaveCopySha256=$hash;Continue=$true;Casts=0;Seconds=300;Cutscene=$scene;CutsceneShots=@(2,4,6,8,10,15,20,30,45,60)}
            if($r[0] -eq 'Reference'){
                Set-Content -LiteralPath (Join-Path $OracleBuild 'yebis-capture-cutscene.txt') -Value $r[2] -NoNewline -Encoding ascii
                & $oracleCast @common -BuildFolder $OracleBuild -EffectFlags 'yebis-hal-observe,yebis-draws-capture,test-no-ai' *>&1 | ForEach-Object { & $say "$_" }
                Remove-Item -LiteralPath (Join-Path $OracleBuild 'yebis-capture-cutscene.txt') -Force -ErrorAction SilentlyContinue}
            else{& $cast @common -BuildFolder $NativeBuild -EffectFlags 'test-no-ai' *>&1 | ForEach-Object { & $say "$_" }}
            $folder="$repo/evidence/I-0066/artifacts/$($r[1])"
            if(Test-Path -LiteralPath "$folder/startup.jsonl"){
                foreach($l in @(Select-String -LiteralPath "$folder/startup.jsonl" -Pattern '"event":"(witness_cutscene|reference_yebis_cutscene_capture|native_render_job_failure)"' | Select-Object -First 4)){ & $say "  $($l.Line)" }
                & $say "  draws captured: $(@(Select-String -LiteralPath "$folder/startup.jsonl" -Pattern '"event":"reference_yebis_draw_capture"').Count); dof rows: $(@(Select-String -LiteralPath "$folder/startup.jsonl" -Pattern '"event":"(native_yebis_dof|reference_yebis_dof_state)"').Count)" } } }
}catch{& $say "ERROR $_"}
finally{Remove-Item -LiteralPath (Join-Path $OracleBuild 'yebis-capture-cutscene.txt') -Force -ErrorAction SilentlyContinue;& $say "END $(Get-Date -Format HH:mm:ss)"}
