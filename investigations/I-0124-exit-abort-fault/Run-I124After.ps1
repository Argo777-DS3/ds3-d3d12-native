# I-0124 after its merge: the start check of main (D-0229), then the night endurance run again (D-0230's control completed), the
# launcher's module, the ASUS by its key. One process, sequential; an end marker in finally.
param([Parameter(Mandatory)][string]$Commit,[Parameter(Mandatory)][string]$Log,[string]$NotBefore='')
$repo='C:/Users/User/Desktop/DS3 Decompilation'
$launcher="$repo/evidence/I-0066/artifacts/game-bootstrap-build"
$say={param($t) "$(Get-Date -Format HH:mm:ss) $t" | Out-File -FilePath $Log -Append -Encoding utf8}
try{
    Set-Location $repo
    if($NotBefore){$at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);if($at -lt (Get-Date).AddHours(-12)){$at=$at.AddDays(1)};while((Get-Date) -lt $at){Start-Sleep 5}}
    & $say 'start check'
    & ./scripts/Invoke-StartCheck.ps1 -Commit $Commit *>&1 | Out-File -FilePath $Log -Append -Encoding utf8
    & $say 'start check end'
    if(-not (Get-Process DarkSoulsIII -ErrorAction SilentlyContinue)){
        & $say 'night start'
        & ./investigations/I-0085-night-endurance/Run-Endurance.ps1 -RunName "night-$Commit" -Build $launcher -Screen '1920x1080@60' *>&1 | Out-File -FilePath $Log -Append -Encoding utf8
        & $say 'night end'}
}catch{& $say "THROW $_"}
finally{& $say 'EXIT'}
