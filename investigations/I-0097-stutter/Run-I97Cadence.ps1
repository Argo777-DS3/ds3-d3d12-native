param([Parameter(Mandatory)][string]$Build,[Parameter(Mandatory)][string]$NotBefore,[string]$Path='settlement',
    [ValidatePattern('^[a-z0-9-]+$')][string]$Series='i97-t5k',[int]$Pairs=2,[switch]$DryRun)
# I-0097 tranche 5 step 3 (CADENCE-STUDY A): the camera turn's cadence on one timeline (cadence-trace.flag: the vertical blanks,
# DS3's game step and dt, the engine's post, the worker's waits and Present, the original's pacer; PresentMon on the same
# counter), native (« p ») against the original (« o »), enemies without AI, -Pairs pairs on the ASUS (the native chain made
# there, -LaunchOnScreen) then one on the OLED. The display-clock tries of 2026-10-25 (RESULT tranche 5 third step) used this
# script with display-clock.flag, since removed. One announced live test. Read with Get-I97Cadence.ps1 (-Segment sweep, walk).
$ErrorActionPreference='Stop';$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot);$file="$repo/scripts/replay/$Path.path.json"
if(-not (Test-Path -LiteralPath $file)){ throw "no path $Path" }
$flags='test-no-ai,cadence-trace,frame-dump';$plan=[Collections.Generic.List[object]]::new()
for($k=1;$k -le $Pairs;$k++){ $plan.Add(@('1920x1080@60',"asus-p$k",'Measure',$true));$plan.Add(@('1920x1080@60',"asus-o$k",'Reference',$false)) }
$plan.Add(@('2560x1440@280','oled-p1','Measure',$false));$plan.Add(@('2560x1440@280','oled-o1','Reference',$false))
if($DryRun){ foreach($r in $plan){ "$Series-$Path-$($r[1]) $($r[0]) $($r[2]) [$flags] launch-on-screen=$($r[3])" };return }
$at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);if($at -lt (Get-Date).AddHours(-12)){ $at=$at.AddDays(1) };while((Get-Date) -lt $at){ Start-Sleep 5 }
foreach($r in $plan){ for($try=0;$try -lt 2;$try++){ $name="$Series-$Path-$($r[1])$(if($try){'-r'})";"== $name $(Get-Date -Format HH:mm:ss) on $($r[0])"
        $extra=@{};if($r[3]){ $extra.LaunchOnScreen=$true }
        & "$repo/scripts/Invoke-ReplayRun.ps1" -RunName $name -Build $Build -Kind $r[2] -Screen $r[0] -PresentMon -PathFile $file -EffectFlags $flags @extra 2>&1 | Select-Object -Last 1
        $t=(Get-Date).AddMinutes(3);while((Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) -and (Get-Date) -lt $t){ Start-Sleep 3 };Start-Sleep 5
        if($try -or -not (Select-String -LiteralPath "$repo/evidence/I-0066/artifacts/$name/startup.jsonl" -SimpleMatch '"event":"prefix_refused"' -Quiet -ErrorAction SilentlyContinue)){ break } } }
"cadence runs done $(Get-Date -Format HH:mm:ss)"
