# I-0139 after the full control of 2026-11-16 (Thomas, 2026-11-16 03:59: « Quand même bizarre que l'image se soit éclaircie »): which
# merge since 2026-11-12 made the gate's standstill picture brighter (the client area's mean luma 56.6 in the 2026-11-12 reference,
# 66.5 now). One replayed-path run per build (scripts/Invoke-ReplayRun.ps1, Kind Measure as the gate's, the ASUS), its start.png
# measured: the client area's mean luma (ffmpeg signalstats) and the gate's own picture difference to the 2026-11-12 reference is
# not recomputed here (the luma answers the question). Builds: evidence/I-0139/bisect-<commit> (made from detached worktrees of main
# at each commit), plus -Extra « label=folder » pairs (the launcher's build for today's main). One process; a stop file
# evidence/I-0139/stop-boss.txt stops between two runs.
param([Parameter(Mandatory)][string[]]$Commits,[string[]]$Extra=@(),[string]$Screen='1920x1080@60',[string]$Log='')
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if(-not $Log){$Log="$repo/evidence/I-0139/artifacts/bisect-runs.log"}
$ff=Get-ChildItem "$env:LOCALAPPDATA/Microsoft/WinGet/Packages" -Recurse -Filter ffmpeg.exe | Select-Object -First 1 -ExpandProperty FullName
$say={param($t) "$(Get-Date -Format HH:mm:ss) $t" | Add-Content $Log}
$items=@($Commits | ForEach-Object { @{Label=$_;Build="$repo/evidence/I-0139/bisect-$_"} })
$items+=@($Extra | ForEach-Object { $p=$_ -split '=',2; @{Label=$p[0];Build=$p[1]} })
try{
    foreach($it in $items){
        if(Test-Path -LiteralPath "$repo/evidence/I-0139/stop-boss.txt"){& $say 'stopped: stop-boss.txt';break}
        if(-not (Test-Path -LiteralPath "$($it.Build)/game_bootstrap.dll")){& $say "$($it.Label): no build";continue}
        $run="i139-bis-$($it.Label)";& $say "$run start"
        & "$repo/scripts/Invoke-ReplayRun.ps1" -RunName $run -Build $it.Build -Kind Measure -Screen $Screen *>&1 | Add-Content $Log
        $png="$repo/evidence/I-0066/artifacts/$run/start.png"
        if(Test-Path -LiteralPath $png){
            $y=(& $ff -hide_banner -i $png -vf 'crop=1280:720:8:31,signalstats,metadata=print:key=lavfi.signalstats.YAVG' -f null - 2>&1 | Select-String 'YAVG=([0-9.]+)' | Select-Object -Last 1).Matches.Groups[1].Value
            & $say "$run YAVG=$y"}
        else{& $say "${run}: no start.png"}
        $t=(Get-Date).AddMinutes(2);while((Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) -and (Get-Date) -lt $t){Start-Sleep 3};Start-Sleep 5 }
}catch{& $say "ERROR $_"}
finally{& $say 'END'}
