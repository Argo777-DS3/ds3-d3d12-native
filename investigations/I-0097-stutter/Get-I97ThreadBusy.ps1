param([Parameter(Mandatory)][string]$Run,[string]$SlowName="slow",[int]$Top=12)
# I-0097: which threads work more in the slow frames. From Get-I97FrameProfile.ps1's two reports, per thread (in report order):
# its busy samples per frame (all samples minus those whose leaf is a kernel wait) in the slow and the ordinary frames, then for
# the threads whose busy time grows most, the inclusive frames that grow most (Compare-I97Profiles' reading, per thread).
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot);$dir="$repo/evidence/I-0066/artifacts/$Run"
$waits='ntdll.dll!NtWaitForSingleObject','ntdll.dll!ZwWaitForAlertByThreadId','ntdll.dll!NtWaitForMultipleObjects','ntdll.dll!NtRemoveIoCompletion',
    'ntdll.dll!NtRemoveIoCompletionEx','ntdll.dll!NtDelayExecution','ntdll.dll!ZwWaitForWorkViaWorkerFactory','ntdll.dll!ZwDelayExecution','ntdll.dll!NtWaitForAlertByThreadId'
function Read-Threads([string]$file,[int]$frames){ $threads=[Collections.Generic.List[object]]::new();$t=$null;$section=''
    foreach($line in Get-Content -LiteralPath $file){
        if($line -match '^== thread (\S+): (\d+) samples'){ $t=[pscustomobject]@{name=$Matches[1];samples=[double]$Matches[2];leaf=@{};inclusive=@{}};$threads.Add($t);continue }
        if(-not $t){ continue }
        if($line -match '^  ([a-z].*)$'){ $section=if($Matches[1] -like 'leaf function*'){'leaf'}elseif($Matches[1] -like 'inclusive*'){'inclusive'}else{''};continue }
        if($section -and $line -match '^\s+[\d.]+%\s+(\d+)\s+(.+)$'){ $t.($section)[$Matches[2]]=[double]$Matches[1]/$frames } }
    foreach($x in $threads){ $idle=0.0;foreach($w in $waits){ $idle+=[double]$x.leaf[$w] };$x | Add-Member busy ([Math]::Max(0,$x.samples/$frames-$idle)) }
    $threads }
$slowFrames=@(Get-Content -LiteralPath "$dir/frames-$SlowName.txt").Count;$fastFrames=@(Get-Content -LiteralPath "$dir/frames-fast.txt").Count
$slow=Read-Threads "$dir/sample-profile-$SlowName-report.txt" $slowFrames;$fast=Read-Threads "$dir/sample-profile-fast-report.txt" $fastFrames
"busy samples per frame, slow ($slowFrames frames) against fast ($fastFrames frames)"
$rows=for($i=0;$i -lt [Math]::Min($slow.Count,$fast.Count);$i++){ [pscustomobject]@{index=$i;thread=$slow[$i].name;slow=[Math]::Round($slow[$i].busy,2);fast=[Math]::Round($fast[$i].busy,2);extra=[Math]::Round($slow[$i].busy-$fast[$i].busy,2)} }
$rows | Sort-Object extra -Descending | Format-Table -AutoSize | Out-String -Width 160
foreach($r in @($rows | Sort-Object extra -Descending | Select-Object -First 4)){ $s=$slow[$r.index];$f=$fast[$r.index]
    "== thread $($r.index) ($($r.thread)): busy extra $($r.extra)"
    @($s.inclusive.Keys+$f.inclusive.Keys | Sort-Object -Unique | ForEach-Object { [pscustomobject]@{frame=$_;slow=[Math]::Round([double]$s.inclusive[$_],2);fast=[Math]::Round([double]$f.inclusive[$_],2);extra=[Math]::Round([double]$s.inclusive[$_]-[double]$f.inclusive[$_],2)} }) |
        Sort-Object extra -Descending | Select-Object -First $Top | Format-Table -AutoSize | Out-String -Width 200 }
