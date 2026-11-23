param([Parameter(Mandatory)][string]$Run,[string]$SlowName="slow",[string]$Thread='owner',[ValidateSet('inclusive','leaf function')][string]$Section='inclusive',[int]$Top=40)
# I-0097: the slow frames' extra cost per frame. From Get-I97FrameProfile.ps1's two reports (the frames over 25 ms and the
# frames under 17.5 ms of one run), the samples per frame of each frame in a section of one thread's report (inclusive by
# default), slow against fast, sorted by the difference: what a slow frame holds that an ordinary one does not.
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot);$dir="$repo/evidence/I-0066/artifacts/$Run"
function Read-Section([string]$file,[int]$frames){ $rows=@{};$inThread=$false;$inSection=$false
    foreach($line in Get-Content -LiteralPath $file){
        if($line -match '^== thread (\S+):'){ if($inThread){ break };$inThread=$Matches[1] -eq $Thread;continue }
        if(-not $inThread){ continue }
        if($line -match '^  ([a-z].*)$'){ $inSection=$Matches[1] -like "$Section*";continue }
        if($inSection -and $line -match '^\s+[\d.]+%\s+(\d+)\s+(.+)$'){ $rows[$Matches[2]]=[double]$Matches[1]/$frames } }
    $rows }
$slowFrames=@(Get-Content -LiteralPath "$dir/frames-$SlowName.txt").Count;$fastFrames=@(Get-Content -LiteralPath "$dir/frames-fast.txt").Count
$slow=Read-Section "$dir/sample-profile-$SlowName-report.txt" $slowFrames;$fast=Read-Section "$dir/sample-profile-fast-report.txt" $fastFrames
"thread $Thread, $Section, samples per frame: slow ($slowFrames frames) against fast ($fastFrames frames)"
@($slow.Keys + $fast.Keys | Sort-Object -Unique | ForEach-Object { $s=[double]$slow[$_];$f=[double]$fast[$_]
    [pscustomobject]@{frame=$_;slow=[Math]::Round($s,2);fast=[Math]::Round($f,2);extra=[Math]::Round($s-$f,2)} }) | Sort-Object extra -Descending | Select-Object -First $Top
