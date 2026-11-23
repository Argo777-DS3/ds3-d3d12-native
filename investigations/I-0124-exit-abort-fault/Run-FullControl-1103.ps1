# D-0230: the full control of 2026-11-03 (main ca6e70e5), on the ASUS alone (1920x1080@60), Thomas at work: the replayed path gate
# with its memory leg, the control sizes with the launcher's module (1920x1080 full screen, a 1500x844 window), the night endurance.
# One process, sequential; every step's output in the log, an end marker in finally.
param([Parameter(Mandatory)][string]$Log,[string]$NotBefore='')
$repo='C:/Users/User/Desktop/DS3 Decompilation'
$launcher="$repo/evidence/I-0066/artifacts/game-bootstrap-build"
$say={param($t) "$(Get-Date -Format HH:mm:ss) $t" | Out-File -FilePath $Log -Append -Encoding utf8}
try{
    Set-Location $repo
    if($NotBefore){$at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);while((Get-Date) -lt $at){Start-Sleep 5}}
    & $say 'gate start'
    & ./scripts/Invoke-ReplayGate.ps1 -Tag full-1103-gate -Memory *>&1 | Out-File -FilePath $Log -Append -Encoding utf8
    & $say 'gate end'
    foreach($size in @(@('full-1103-1080fs','ScreenMode=FULLSCREEN;Resolution-FullScreenWidth=1920;Resolution-FullScreenHeight=1080'),
                       @('full-1103-1500w','ScreenMode=WINDOW;Resolution-WindowScreenWidth=1500;Resolution-WindowScreenHeight=844'))){
        if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){& $say "$($size[0]) skipped: DarkSoulsIII runs";continue}
        & $say "$($size[0]) start"
        & ./scripts/Invoke-ReplayRun.ps1 -RunName $size[0] -Build $launcher -Screen '1920x1080@60' -GraphicsLevels $size[1] *>&1 | Out-File -FilePath $Log -Append -Encoding utf8
        & $say "$($size[0]) end"}
    if(-not (Get-Process DarkSoulsIII -ErrorAction SilentlyContinue)){
        & $say 'night start'
        & ./investigations/I-0085-night-endurance/Run-Endurance.ps1 -RunName full-1103-night -Build $launcher -Screen '1920x1080@60' *>&1 | Out-File -FilePath $Log -Append -Encoding utf8
        & $say 'night end'}
}catch{& $say "THROW $_"}
finally{& $say 'EXIT'}
