param([Parameter(Mandatory)][string[]]$Path,[Parameter(Mandatory)][string]$Build,[Parameter(Mandatory)][string]$NotBefore,
    [ValidatePattern('^[a-z0-9-]+$')][string]$Series='i97-t5sheet',[string]$NativeFlags='test-no-ai')
# I-0097 tranche 5 (CADENCE-STUDY C-a, Thomas 2026-10-25): for each replayed path, one native and one original run on the ASUS
# (a composed screen: Capture-ScreenSeries's pictures follow the game there), enemies without AI, a picture every 2 s from the
# launch, then the side-by-side sheet (New-I97PathSheet.ps1) in evidence/I-0097/sheets/<series>-<path>-<page>.jpg. Not a base:
# the pictures' capture runs beside the game. One announced live test; the sheets' paths are printed at the end.
$ErrorActionPreference='Stop';$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Path=@($Path | ForEach-Object { $_ -split ',' } | Where-Object { $_ })
$at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);while((Get-Date) -lt $at){ Start-Sleep 5 }
New-Item -ItemType Directory -Force -Path "$repo/evidence/I-0097/sheets" | Out-Null;$sheets=@()
foreach($p in $Path){ $file="$repo/scripts/replay/$p.path.json";if(-not (Test-Path -LiteralPath $file)){ throw "no path $p" }
    foreach($r in @(@("$Series-$p-n",'Measure',$NativeFlags),@("$Series-$p-o",'Reference','test-no-ai'))){ "== $($r[0]) $(Get-Date -Format HH:mm:ss)"
        $cap=Start-Process pwsh -WindowStyle Hidden -PassThru -ArgumentList "-NoProfile -File `"$repo\scripts\Capture-ScreenSeries.ps1`" -Out `"$repo\evidence\I-0097\captures\$($r[0])`" -Seconds 270 -Every 2"
        & "$repo/scripts/Invoke-ReplayRun.ps1" -RunName $r[0] -Build $Build -Kind $r[1] -Screen '1920x1080@60' -PathFile $file -EffectFlags $r[2] 2>&1 | Select-Object -Last 1
        $t=(Get-Date).AddMinutes(3);while((Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) -and (Get-Date) -lt $t){ Start-Sleep 3 };Start-Sleep 5
        if(-not $cap.HasExited){ Stop-Process -Id $cap.Id -ErrorAction SilentlyContinue } }
    try{ $sheets+=& "$PSScriptRoot/New-I97PathSheet.ps1" -Native "$Series-$p-n" -Original "$Series-$p-o" -Out "$repo/evidence/I-0097/sheets/$Series-$p" }catch{ "sheet $p failed: $_" } }
"sheets done $(Get-Date -Format HH:mm:ss)";$sheets
