param([string]$Out='',[double]$Near=8,[int]$WaitMinutes=60,[switch]$Test)
# I-0100 tranche 2 (Thomas, 2026-10-26 04:06: « tu peux utiliser un instrument de mesure en direct pendant que je joue »): beside
# Thomas's own play with tools/Launch-DS3-DX12-Popin.cmd (or tools/Launch-DS3-Original-Popin.cmd: the original's rows), read-only: waits for DarkSoulsIII, then runs Intel's PresentMon 2.6.0
# on it (hidden, D-0199; presentmon.csv with the performance counter, presentmon-clock.txt pairing the tick count and the counter),
# and follows the newest play journal (%LOCALAPPDATA%/DS3-DX12/play-*.jsonl): every native appearance nearer than -Near metres
# (popin_native « on » rows) is written to near.log with the local time, and clock.log pairs each second with the last frame
# serial seen, so a pop-in Thomas notes can be found by its minute. -Test follows the test launcher's journal instead
# (tools/Launch-DS3-DX12-Test-Popin.cmd: %LOCALAPPDATA%/DS3-DX12-Test/test-*.jsonl).
# Ends when the game exits. Nothing is written into the game's folder or the saves.
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if(-not $Out){ $Out="$repo/evidence/I-0100/play/$(Get-Date -Format yyyyMMdd-HHmmss)" }
New-Item -ItemType Directory -Force -Path $Out | Out-Null
$deadline=(Get-Date).AddMinutes($WaitMinutes);$game=$null
while(-not $game -and (Get-Date) -lt $deadline){ $game=Get-Process DarkSoulsIII -ErrorAction SilentlyContinue | Select-Object -First 1;if(-not $game){ Start-Sleep 2 } }
if(-not $game){ "no game within $WaitMinutes minutes" | Set-Content "$Out/watch.log";return }
"game $($game.Id) at $(Get-Date -Format HH:mm:ss)" | Set-Content "$Out/watch.log"
$pm='C:/Users/User/Tools/PresentMon/PresentMon-2.6.0-x64.exe'
Set-Content -LiteralPath "$Out/presentmon-clock.txt" -Value "$([Environment]::TickCount64) $([Diagnostics.Stopwatch]::GetTimestamp()) $([Diagnostics.Stopwatch]::Frequency)"
# The game's process id, not its name: started before PresentMon and unelevated, its name is not resolved (2026-10-26 05:01: no
# CSV). An orphan trace session of an earlier run (DS3Replay) lost this session's events until it was stopped (05:05).
# Review: only while the game watched is the only one running (a replay session of another thread then has no game left to trace).
if(@(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue).Count -eq 1){ & logman stop DS3Replay -ets 2>&1 | Out-Null }
$pmProcess=Start-Process -PassThru -WindowStyle Hidden -FilePath $pm -RedirectStandardOutput "$Out/presentmon.log" -RedirectStandardError "$Out/presentmon-errors.log" -ArgumentList `
    "--process_id $($game.Id) --output_file `"$Out\presentmon.csv`" --v1_metrics --qpc_time --no_console_stats --session_name DS3Play --stop_existing_session"
# The game's own journal: the newest one created after the game started (the module opens it after its injection).
$journal=$null
while(-not $journal -and -not $game.HasExited){ Start-Sleep 2
    $journal=Get-ChildItem -LiteralPath (Join-Path $env:LOCALAPPDATA $(if($Test){'DS3-DX12-Test'}else{'DS3-DX12'})) -Filter $(if($Test){'test-*.jsonl'}else{'play-*.jsonl'}) | Where-Object { $_.CreationTime -ge $game.StartTime.AddSeconds(-5) } |
        Sort-Object CreationTime -Descending | Select-Object -First 1 }
if(-not $journal){ "no journal before the game exited" | Add-Content "$Out/watch.log";return }
"journal $($journal.FullName)" | Add-Content "$Out/watch.log"
$position=0L
while(-not $game.HasExited){
    Start-Sleep 1
    try{ $stream=[IO.File]::Open($journal.FullName,'Open','Read','ReadWrite');$stream.Seek($position,'Begin') | Out-Null
        $reader=New-Object IO.StreamReader($stream);$text=$reader.ReadToEnd();$position=$stream.Position;$reader.Dispose() }catch{ continue }
    $now=Get-Date -Format HH:mm:ss
    # The clock: the last frame serial (the original's game step, -Original) seen each second, so a minute Thomas notes finds its frames (clock.log).
    $serials=[regex]::Matches($text,'"(?:serial|step)":(\d+)');if($serials.Count){ "$now $($serials[$serials.Count-1].Groups[1].Value)" | Add-Content "$Out/clock.log" }
    foreach($line in $text -split "`n"){ if($line -notmatch '"event":"popin_(native|reference)".*"on":1'){ continue }
        try{ $r=$line | ConvertFrom-Json }catch{ continue }
        if([double]$r.d -ge 0 -and [double]$r.d -lt $Near){ "$now serial $(if($null -ne $r.serial){$r.serial}else{$r.step}) $($r.model) at $([Math]::Round([double]$r.d,1)) m ($($r.centre -join ','))" | Add-Content "$Out/near.log" } } }
if($pmProcess -and -not $pmProcess.HasExited){ if(-not $pmProcess.WaitForExit(5000)){ Stop-Process -Id $pmProcess.Id -Force -ErrorAction SilentlyContinue } }   # no --terminate_on_proc_exit unelevated
& logman stop DS3Play -ets 2>&1 | Out-Null
"game ended at $(Get-Date -Format HH:mm:ss)" | Add-Content "$Out/watch.log"
