param([Parameter(Mandatory)][ValidatePattern('^[a-z]+$')][string]$Path,[Parameter(Mandatory)][string]$Build,[Parameter(Mandatory)][string]$NotBefore,
    [ValidatePattern('^[a-z0-9-]+$')][string]$Series='i97-t5b',[int]$NativeOled=4,[int]$OriginalOled=2,[int]$NativeAsus=2,[int]$OriginalAsus=1)
# I-0097 tranche 5 (D-0213, TRANCHE5-STUDY §4): a replayed study path's bases on each screen, native (measure mode) and original
# (Reference), alternated on each screen, with PresentMon and the enemies without AI (test-no-ai.flag, both renderers): run names
# <series>-<path>-<screen>-<n|o><k>. One announced live test. Read with scripts/Get-ReplayMeasure.ps1 and Get-I97Cycles.ps1.
$ErrorActionPreference='Stop';$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot);$file="$repo/scripts/replay/$Path.path.json"
$at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);while((Get-Date) -lt $at){ Start-Sleep 5 }
$plan=[Collections.Generic.List[object]]::new()
foreach($s in @(@('2560x1440@280','oled',$NativeOled,$OriginalOled),@('1920x1080@60','asus',$NativeAsus,$OriginalAsus))){
    $n=0;$o=0;while($n -lt $s[2] -or $o -lt $s[3]){ if($n -lt $s[2]){ $n++;$plan.Add(@($s[0],"$Series-$Path-$($s[1])-n$n",'Measure','test-no-ai,frame-dump')) }
        if($o -lt $s[3]){ $o++;$plan.Add(@($s[0],"$Series-$Path-$($s[1])-o$o",'Reference','test-no-ai,frame-dump')) } } }
foreach($r in $plan){ for($try=0;$try -lt 2;$try++){ $name=if($try){"$($r[1])-r"}else{$r[1]};"== $name $(Get-Date -Format HH:mm:ss) on $($r[0])"
        & "$repo/scripts/Invoke-ReplayRun.ps1" -RunName $name -Build $Build -Kind $r[2] -Screen $r[0] -PresentMon -PathFile $file -EffectFlags $r[3] 2>&1 | Select-Object -Last 1
        $t=(Get-Date).AddMinutes(3);while((Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) -and (Get-Date) -lt $t){ Start-Sleep 3 };Start-Sleep 5
        if($try -or -not (Select-String -LiteralPath "$repo/evidence/I-0066/artifacts/$name/startup.jsonl" -SimpleMatch '"event":"prefix_refused"' -Quiet -ErrorAction SilentlyContinue)){ break } } }
"bases done $(Get-Date -Format HH:mm:ss)"
