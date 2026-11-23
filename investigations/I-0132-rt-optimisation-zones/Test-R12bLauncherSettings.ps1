# I-0132 R12b (the final review's M1): the play launcher's settings block run offline against scratch play folders, never the game
# nor Thomas's own folder. The block (tools/Launch-DS3-DX12.ps1, from « # I-0132 R12b: the player's settings » to « # The saves »)
# is cut from the launcher as it is and run with $play set to each fixture's folder; each case checks the settings file written,
# the old files renamed and the launcher's printed line. Exit 1 on the first failed case.
param([string]$Scratch=(Join-Path ([IO.Path]::GetTempPath()) "r12b-launcher-$PID"))
$ErrorActionPreference='Stop'
$launcher=Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'tools/Launch-DS3-DX12.ps1'
$text=[IO.File]::ReadAllText($launcher)
$begin=$text.IndexOf("# I-0132 R12b: the player's settings");$end=$text.IndexOf('# The saves, before the game can write them.')
if($begin -lt 0 -or $end -le $begin){throw 'the launcher block was not found'}
$block=[scriptblock]::Create($text.Substring($begin,$end-$begin))
function Pause-Exit([string]$message){throw "Pause-Exit: $message"}
$failures=0
try{
function Run-Case([string]$name,[hashtable]$files,[string]$FrameRate='',[string]$Reflex='',[string[]]$want,[string[]]$renamed=@(),[string]$printed){
    $play=Join-Path $Scratch $name;New-Item -ItemType Directory -Force $play|Out-Null
    foreach($f in $files.Keys){$v=$files[$f];if($v -is [byte[]]){[IO.File]::WriteAllBytes((Join-Path $play $f),$v)}else{[IO.File]::WriteAllText((Join-Path $play $f),$v,[Text.UTF8Encoding]::new($false))}}
    $Original=$false
    $out=& {. $block} 6>&1 | Out-String
    $ini=[IO.File]::ReadAllLines((Join-Path $play 'rekindled-settings.ini'))|Where-Object{$_ -and -not $_.StartsWith(';')}
    $ok=(@($ini) -join '|') -eq ($want -join '|')
    foreach($r in $renamed){if(-not (Test-Path -LiteralPath (Join-Path $play "$r.migrated")) -or (Test-Path -LiteralPath (Join-Path $play $r))){$ok=$false}}
    if($printed -and $out -notmatch [regex]::Escape($printed)){$ok=$false}
    if($ok){Write-Host "PASS $name"}else{Write-Host "FAIL $name : ini [$(@($ini) -join ' | ')] printed [$($out.Trim())]";$script:failures++}}
# Thomas's folder as it is today: dlss-mode.txt « performance » with CRLF.
Run-Case 'thomas' @{'dlss-mode.txt'="performance`r`n"} -want @('dlss=performance') -renamed @('dlss-mode.txt') -printed 'DLSS : performance'
# No old file: the file made with the level only.
Run-Case 'empty' @{} -want @('level=rt') -printed 'DLSS : off (TAA)'
# Every old word, a UTF-16 rr-mode.txt, a French decimal comma, an exponent, an unrecognised Reflex word (not moved), 60 (not moved).
[byte[]]$utf16=@(0xFF,0xFE)+[Text.Encoding]::Unicode.GetBytes("off")
Run-Case 'all' @{'dlss-mode.txt'='Ultra_Performance';'rr-mode.txt'=$utf16;'rt-level.txt'='PT';'rt-bounces.txt'='7';'reflex-mode.txt'='turbo';'fg-mode.txt'='x3';
    'frame-rate.txt'='60';'sharpen.txt'='0,5'} -want @('dlss=ultra-performance','rr=off','level=pt','bounces=7','frame_generation=x3','sharpen=0.5') `
    -renamed @('dlss-mode.txt','rr-mode.txt','rt-level.txt','rt-bounces.txt','reflex-mode.txt','fg-mode.txt','frame-rate.txt','sharpen.txt') -printed 'Reflex : off ; DLSS : ultra-performance'
Run-Case 'exponent' @{'sharpen.txt'='5e-1';'frame-rate.txt'='144'} -want @('frame_rate=144','sharpen=0.5') -printed 'Fréquence de la logique : 144'
# A settings file already there: the old files left alone; -FrameRate and -Reflex written; a refused value after an accepted one
# printed as the module runs it (the last accepted).
Run-Case 'existing' @{'rekindled-settings.ini'="dlss=quality`ndlss=bogus`nreflex=on`n";'dlss-mode.txt'='dlaa'} -FrameRate '120' -Reflex 'Boost' `
    -want @('dlss=quality','dlss=bogus','reflex=boost','frame_rate=120') -printed 'Fréquence de la logique : 120 ; Reflex : boost ; DLSS : quality'
if(Test-Path -LiteralPath (Join-Path $Scratch 'existing/dlss-mode.txt.migrated')){Write-Host 'FAIL existing: an old file renamed while the settings file existed';$failures++}
# The last accepted value of each printed key with no option given; leading zeros as the module reads them.
Run-Case 'accepted' @{'rekindled-settings.ini'="frame_rate=0144`nframe_rate=7`nreflex=boost`nreflex=x`n"} -want @('frame_rate=0144','frame_rate=7','reflex=boost','reflex=x') `
    -printed 'Fréquence de la logique : 144 ; Reflex : boost ; DLSS : off (TAA)'
# An old file whose .migrated already exists: neither touched (never overwritten).
Run-Case 'kept' @{'dlss-mode.txt'='quality';'dlss-mode.txt.migrated'='dlaa'} -want @('dlss=quality')
if(-not (Test-Path -LiteralPath (Join-Path $Scratch 'kept/dlss-mode.txt')) -or [IO.File]::ReadAllText((Join-Path $Scratch 'kept/dlss-mode.txt.migrated')) -ne 'dlaa'){Write-Host 'FAIL kept: an existing .migrated overwritten';$failures++}
}finally{Remove-Item -LiteralPath $Scratch -Recurse -Force -ErrorAction SilentlyContinue}
if($failures){Write-Host "$failures case(s) failed";exit 1}
Write-Host 'PASS the launcher settings block: every case'
