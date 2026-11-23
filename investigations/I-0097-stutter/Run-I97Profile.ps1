param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$RunName,[Parameter(Mandatory)][string]$Build,[int]$StartWorldFrame=2300,[switch]$Ds3Threads,
    [switch]$Original,[int]$StartCall=3600,[string]$Screen='',
    # Tranche 5 (D-0213): another path (scripts/replay/*.path.json) and extra effect flags (test-no-ai: its enemies without AI).
    [string]$PathFile='',[string]$ExtraFlags='',
    # Tranche 5: the presented-frame mark at which sampling ends and the samples are written (75,000 samples outlast a short path).
    [int]$EndMark=0)
# I-0097: one replayed-path run (an announced live run, Invoke-ReplayRun.ps1, measure mode) with frame-dump.flag and the sampler
# from world frame -StartWorldFrame (sample-profile-start.txt in the build folder, removed after the run: 75,000 samples a
# thread on the engine thread, the render worker, the pool and the loader threads met at an admission). World frame 2,300 is
# about the walk's start on this path (the title's frames are not world frames), so the samples cover the walk's eight cycles.
# -Ds3Threads also samples every thread DS3 started (sample-ds3-threads.flag: its task workers). Then Get-I97FrameProfile.ps1
# reads the slow and the ordinary frames.
# Tranche 4: -Original runs the original (Reference, its present pacer's calls as the frame marks) with the sampler on its
# presenting and main threads from pacer call -StartCall (3,600 is about the walk's start: i97-t3m1-ow280) and, with
# -Ds3Threads, on every thread DS3 started; Get-I97FrameProfile.ps1 -Original reads it.
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$start=Join-Path $Build 'sample-profile-start.txt'
$flags=if($Original){ 'frame-dump;sample-profile' }else{ 'frame-dump' };if($Ds3Threads){ $flags+=';sample-ds3-threads' };if($ExtraFlags){ $flags+=';'+$ExtraFlags }
$a=@{RunName=$RunName;Build=$Build;Kind=$(if($Original){'Reference'}else{'Measure'});EffectFlags=$flags};if($Screen){ $a.Screen=$Screen;$a.PresentMon=$true };if($PathFile){ $a.PathFile=$PathFile }
try{ Set-Content -LiteralPath $start -Value "$(if($Original){$StartCall}else{$StartWorldFrame})$(if($EndMark){" $EndMark"})" -NoNewline
    & "$repo/scripts/Invoke-ReplayRun.ps1" @a 2>&1 | Select-Object -Last 3 }
finally{ if(Test-Path -LiteralPath $start){ Remove-Item -LiteralPath $start } }
