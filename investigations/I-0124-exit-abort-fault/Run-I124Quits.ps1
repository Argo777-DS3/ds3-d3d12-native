# I-0124: close-box quits from the world with the fix's build, each the one-minute start check (scripts/Invoke-StartCheck.ps1:
# launched, to the world on the Firelink path's save, closed by its close box; offline, restored by hash, the ASUS by its key). The
# runs are named start-<label> (hex labels a1240NN, not commits). One process; an end marker in finally.
param([Parameter(Mandatory)][string]$BuildFolder,[Parameter(Mandatory)][string]$Log,[int]$Count=4,[int]$First=1,[string]$NotBefore='')
$repo=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$say={param($t) "$(Get-Date -Format HH:mm:ss) $t" | Out-File -FilePath $Log -Append -Encoding utf8}
try{
    if($NotBefore){$at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);while((Get-Date) -lt $at){Start-Sleep 5}}
    for($i=$First;$i -lt $First+$Count;$i++){
        if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){& $say 'DarkSoulsIII already runs: stopped';break}
        $label='a1240{0:D2}' -f $i;& $say "quit $label"
        & "$repo/scripts/Invoke-StartCheck.ps1" -Commit $label -BuildFolder $BuildFolder *>&1 | Out-File -FilePath $Log -Append -Encoding utf8
        & $say "quit $label done"}
}catch{& $say "THROW $_"}
finally{& $say 'EXIT'}
