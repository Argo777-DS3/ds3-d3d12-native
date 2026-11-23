# D-0230: the full control of 2026-11-09 (main f3a49990: I-0132 R11e and the NRD specular), the launcher's module: the replayed path gate with
# its memory leg (the ASUS base, required; -AlsoOled adds the OLED base, never instead of it), the control sizes (1920x1080 full
# screen, a 1500x844 window, 1280x720 windowed) and the night endurance on -Screen (the ASUS by default). A copy of I-0125's Run-FullControl-1106.ps1 (itself I-0124's)
# with the date, the tags and the screen as parameters. One process, sequential; every step's output in the log, an end marker in
# finally.
param([Parameter(Mandatory)][string]$Log,[string]$NotBefore='',[string]$Screen='1920x1080@60',[switch]$AlsoOled)
$repo='C:/Users/User/Desktop/DS3 Decompilation'
$launcher="$repo/evidence/I-0066/artifacts/game-bootstrap-build"
$say={param($t) "$(Get-Date -Format HH:mm:ss) $t" | Out-File -FilePath $Log -Append -Encoding utf8}
try{
    Set-Location $repo
    if($NotBefore){$at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);if($at -lt (Get-Date).AddHours(-12)){$at=$at.AddDays(1)};while((Get-Date) -lt $at){Start-Sleep 5}}
    & $say 'gate start'
    $screens=if($AlsoOled){@('1920x1080@60','2560x1440@280')}else{@('1920x1080@60')}
    & ./scripts/Invoke-ReplayGate.ps1 -Tag full-1109-gate -Memory -Screens $screens *>&1 | Out-File -FilePath $Log -Append -Encoding utf8
    & $say 'gate end'
    foreach($size in @(@('full-1109-1080fs','ScreenMode=FULLSCREEN;Resolution-FullScreenWidth=1920;Resolution-FullScreenHeight=1080'),
                       @('full-1109-1500w','ScreenMode=WINDOW;Resolution-WindowScreenWidth=1500;Resolution-WindowScreenHeight=844'),
                       @('full-1109-720w','ScreenMode=WINDOW;Resolution-WindowScreenWidth=1280;Resolution-WindowScreenHeight=720'))){   # WORKFLOW: 720p, 1080p and one arbitrary size
        if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){& $say "$($size[0]) skipped: DarkSoulsIII runs";continue}
        & $say "$($size[0]) start"
        & ./scripts/Invoke-ReplayRun.ps1 -RunName $size[0] -Build $launcher -Screen $Screen -GraphicsLevels $size[1] *>&1 | Out-File -FilePath $Log -Append -Encoding utf8
        & $say "$($size[0]) end"}
    if(-not (Get-Process DarkSoulsIII -ErrorAction SilentlyContinue)){
        & $say 'night start'
        & ./investigations/I-0085-night-endurance/Run-Endurance.ps1 -RunName full-1109-night -Build $launcher -Screen $Screen *>&1 | Out-File -FilePath $Log -Append -Encoding utf8
        & $say 'night end'}
}catch{& $say "THROW $_"}
finally{& $say 'EXIT'}
