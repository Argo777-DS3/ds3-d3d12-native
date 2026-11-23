param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Series,[Parameter(Mandatory)][string[]]$Builds,[int]$Pairs=8,[string]$NotBefore='')
# I-0097 (I-0095 §8): an A/B series of the replayed path (announced live runs, Invoke-ReplayRun.ps1, measure mode, frame-dump.flag),
# the builds alternated run by run (A B A B ...), -Pairs runs of each; run names <series>-<k>-<letter>. A launch refused before
# the module ran (prefix_refused) is launched once more as <name>-r. Read with Get-I97Series.ps1.
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if($NotBefore){ $at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);while((Get-Date) -lt $at){ Start-Sleep 5 } }
$letters='abcdefgh'
for($k=1;$k -le $Pairs;$k++){ for($b=0;$b -lt $Builds.Count;$b++){ $name="$Series-$k-$($letters[$b])"
    for($try=0;$try -lt 2;$try++){ "== $name $(Get-Date -Format HH:mm:ss) $($Builds[$b])"
        & "$repo/scripts/Invoke-ReplayRun.ps1" -RunName $name -Build $Builds[$b] -Kind Measure -EffectFlags 'frame-dump' 2>&1 | Select-Object -Last 1
        $t=(Get-Date).AddMinutes(3);while((Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) -and (Get-Date) -lt $t){ Start-Sleep 3 };Start-Sleep 5
        if($try -or -not (Select-String -LiteralPath "$repo/evidence/I-0066/artifacts/$name/startup.jsonl" -SimpleMatch '"event":"prefix_refused"' -Quiet -ErrorAction SilentlyContinue)){ break }
        $name="$name-r" } } }
"series done $(Get-Date -Format HH:mm:ss)"
