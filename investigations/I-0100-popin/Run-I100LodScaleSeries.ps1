param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Series,[Parameter(Mandatory)][string]$BuildFolder,
    [string[]]$Runs=@('b1','a1','b2','a2'),[ValidatePattern('^[0-9]{1,2},-?[0-9]{1,4},[0-9]{1,2}$')][string]$Sweep='8,250,3',
    [ValidateRange(5,60)][int]$SettleAfterReload=20,[ValidateRange(60,400)][int]$HoldSeconds=170,[string]$NotBefore='')
# I-0100 LOD scale study (LOD-SCALE-STUDY-PLAN question 1, option A's measured impact): announced runs (D-0116) of Gamma at the
# Anor Londo bonfire without AI, native, measure mode without the debug layer, test-death.flag (the player killed 75 s after
# loading, the respawn at the same bonfire); b* runs DS3's own behaviour (B: the LOD scale 1 after the reload,
# no-lod-launch-scale.flag since D-0222), a* the native default (A: the launch scale 0.757 kept; lod-launch-scale.flag before). Before the death A and B are the same; after it, -SettleAfterReload s
# still at the respawn, then -Sweep "n,dx,s": n camera turns of dx mouse units each held s seconds. Marks (TickCount64, the
# journal's clock) in evidence/I-0100/lodscale/<series>-<run>-marks.txt; the GPU's 3D utilisation in <series>-<run>-gpu.txt
# beside it. Run folders: evidence/I-0066/artifacts/<series>-<run>. Study
# runs, never a gate record (test-no-ai). The screen is the only one on: the ASUS (Thomas away, WORKFLOW Live game rule).
# Limits (final review M2): Run-AlphaCast's path writes no launch-check.json and the runner reads no PresentMon (it failed here,
# RESULT « The LOD scale study » item 5); each run's screen is checked after it (display_refreshes at 60 Hz, else the series
# stops). Not a runner for a measure that decides a merge (Invoke-ReplayRun.ps1 is).
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot);$inv="$repo/investigations/I-0066-independent-native-startup"
$save='C:\Users\User\.ds3-i0039-backups\shebang-zones-gamma-m37-anorlondo-20261026-123818\DS30000.sl2';$hash='E96236897D815D1B1339FD49D3AA8D7308C185AA64AC9224A6F72A4638703411'
$out="$repo/evidence/I-0100/lodscale";New-Item -ItemType Directory -Force -Path $out | Out-Null
if($NotBefore){ $at=[datetime]::Today.Add([TimeSpan]::Parse($NotBefore));if($at -lt (Get-Date).AddHours(-12)){ $at=$at.AddDays(1) }   # a time after midnight given before it
    "waiting until $($at.ToString('yyyy-MM-dd HH:mm'))";while((Get-Date) -lt $at){ Start-Sleep 5 } }
foreach($run in $Runs){
    if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){ 'game running, stop';exit 1 }
    $name="$Series-$run";$flags='test-death;test-no-ai;update-observe'+$(if($run.StartsWith('a')){ '' }else{ ';no-lod-launch-scale' })   # D-0222: A is a native run's default since build t5 (the series i100-ls-* passed lod-launch-scale to its a* runs)
    $trace="$repo/evidence/I-0066/artifacts/$name/startup.jsonl";$marks="$out/$name-marks.txt"
    "== $name ($flags) $(Get-Date -Format HH:mm:ss)"
    # The post-death driver, its own hidden process: the GPU sampler once the title runs, then the death row, the reload (the
    # world update gate stopped), the settle and the turns; it gives up after 10 minutes or when the game is gone. PresentMon
    # left no CSV in any of i100-ls-b1..a4 (started before the game, then at the title: not understood), so the GPU's share is
    # read from Windows' per-process counter « GPU Engine » 3D utilisation, once a second (<run>-gpu.txt: tick and percent).
    $driver=@"
`$ErrorActionPreference='Continue';`$t0=Get-Date
function Seen([string]`$p){ (Test-Path -LiteralPath '$trace') -and [bool](Select-String -LiteralPath '$trace' -Pattern `$p -Quiet) }
function Mark([string]`$m){ Add-Content -LiteralPath '$marks' -Value "`$m `$([Environment]::TickCount64)" }
while(-not (Seen '"event":"title_step_state"') -and ((Get-Date)-`$t0).TotalMinutes -lt 5){ Start-Sleep 1 }
`$sampler={ param(`$file) while(`$g=Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){ try{ `$v=((Get-Counter -Counter "\GPU Engine(pid_`$(`$g[0].Id)_*engtype_3D)\Utilization Percentage" -ErrorAction Stop).CounterSamples | Measure-Object CookedValue -Sum).Sum
    Add-Content -LiteralPath `$file -Value ("{0} {1:N2}" -f [Environment]::TickCount64,`$v) }catch{ Start-Sleep 1 } } }
`$job=Start-Job -ScriptBlock `$sampler -ArgumentList '$out/$name-gpu.txt'
while(-not (Seen '"event":"test_death"') -and ((Get-Date)-`$t0).TotalMinutes -lt 10){ Start-Sleep 1 }
if(-not (Seen '"event":"test_death"')){ Mark 'no_death';exit }
Mark 'death_seen'
`$death=(Select-String -LiteralPath '$trace' -Pattern '"event":"test_death"' | Select-Object -First 1).LineNumber   # final review L5: the stop after the death, not any earlier one
while(-not (Select-String -LiteralPath '$trace' -Pattern '"event":"world_update_gate".*"running":0' | Where-Object { `$_.LineNumber -gt `$death }) -and ((Get-Date)-`$t0).TotalMinutes -lt 10){ Start-Sleep 1 }
Mark 'reload_seen';Start-Sleep $SettleAfterReload;Mark 'sweep_start'
`$v='$Sweep' -split ','
for(`$i=1;`$i -le [int]`$v[0] -and (Get-Process DarkSoulsIII -ErrorAction SilentlyContinue);`$i++){ pwsh -NoProfile -File '$inv/Pilot-Game.ps1' -Action look -X `$v[1] -Y 0 | Out-Null;Start-Sleep ([int]`$v[2]) }
Mark 'sweep_end';Wait-Job `$job -Timeout 600 | Out-Null   # the sampler lives as long as this process
"@
    Set-Content -LiteralPath "$out/$name-driver.ps1" -Value $driver -Encoding utf8;Remove-Item -LiteralPath $marks -ErrorAction SilentlyContinue
    $drive=Start-Process -PassThru -WindowStyle Hidden pwsh -ArgumentList "-NoProfile -File `"$out/$name-driver.ps1`""
    try { & "$inv/Run-AlphaCast.ps1" -RunName $name -BootstrapMode Native -EffectFlags $flags -Seconds 300 -Casts 0 -SaveCopy $save -SaveCopySha256 $hash -Continue -Measure -NoDebugLayer -HoldSeconds $HoldSeconds -BuildFolder $BuildFolder 2>&1 | Select-Object -Last 3 }
    catch { "run error: $_" }
    finally { if($drive -and -not $drive.HasExited){ Stop-Process -Id $drive.Id -Force -ErrorAction SilentlyContinue } }
    if(Test-Path -LiteralPath $marks){ Get-Content -LiteralPath $marks }
    # Final review M2: the screen the run measured (the module's display_refreshes rows); the series stops at a screen other than 60 Hz.
    $hz=Select-String -LiteralPath $trace -Pattern '"event":"display_refreshes".*?"refresh_hz":([0-9]+)' -ErrorAction SilentlyContinue | ForEach-Object { [int]$_.Matches[0].Groups[1].Value } | Measure-Object -Maximum | ForEach-Object Maximum   # the closing rows say 0
    "refresh_hz $hz";if($hz -ne 60){ 'screen not at 60 Hz: series stopped';exit 1 }
}
"== end $(Get-Date -Format HH:mm:ss)"
