[CmdletBinding()]
param(
    # Where the module looks for it (beside game_bootstrap.dll): the launcher's folder by default; a test build's folder with -Out.
    [string]$Out = (Join-Path $env:LOCALAPPDATA 'DS3-DX12'),
    [string]$Game = 'C:\Program Files (x86)\Steam\steamapps\common\ELDEN RING\Game',
    [string[]]$Archives = @('Data0','Data1','Data2','Data3','DLC')
)
# I-0135 (Thomas, 2026-11-12 05:40: the Bandai Namco logo as Elden Ring shows it, on black). Run once, on the player's own PC,
# from his own Elden Ring; a local development script, never shipped in an installer (D-0267), and what it writes stays on
# that PC (outside Git, never published). Read-only on the game: it reads /menu/hi/02_title.tpf.dcx from the archives, takes
# the texture MENU_BNE_Logo (1920x440 BC1, opaque on black), fits it to DS3's page (ComposeLogo in ErArchive.cs: 768x176 in a
# 768x768 black square, centred where Elden Ring centres it) and writes er-bne-logo.dds into $Out, beside the module. The
# module draws it on the Bandai Namco page when the file is there; without it, DS3's own logo made for black (D-0182).
$ErrorActionPreference = 'Stop'
$exe = Join-Path $Game 'eldenring.exe'
$oodle = Join-Path $Game 'oo2core_6_win64.dll'
if (-not (Test-Path -LiteralPath $exe) -or -not (Test-Path -LiteralPath $oodle)) { throw "Elden Ring not found in $Game" }
if (-not (Test-Path -LiteralPath $Out)) { throw "No module folder at $Out" }
Add-Type -Path (Join-Path $PSScriptRoot 'ErArchive.cs')
$name = '/menu/hi/02_title.tpf.dcx'
$hash = [ErArchive]::Hash($name)
$keys = [ErArchive]::Keys($exe)
$file = $null
foreach ($archive in $Archives) {
    $bhd = Join-Path $Game "$archive.bhd"; if (-not (Test-Path -LiteralPath $bhd)) { continue }
    $index = $null
    foreach ($k in $keys) { $try = [ErArchive]::DecryptIndex($bhd, $k); if ([Text.Encoding]::ASCII.GetString($try, 0, 4) -eq 'BHD5') { $index = $try; break } }
    if (-not $index) { continue }
    $entry = [ErArchive]::Entries($index) | Where-Object { $_.Hash -eq $hash } | Select-Object -First 1
    if ($entry) { $file = [ErArchive]::Read((Join-Path $Game "$archive.bdt"), $entry); break }
}
if (-not $file) { throw "$name not found in the Elden Ring archives" }
$tpf = [ErArchive]::Undcx($file, $oodle)
$dds = [ErArchive]::TpfTexture($tpf, 'MENU_BNE_Logo')
if (-not $dds) { throw 'MENU_BNE_Logo not found in 02_title.tpf' }
$w = 0; $h = 0
$rgba = [ErArchive]::DecodeBc1Dds($dds, [ref]$w, [ref]$h)
if ($w -ne 1920 -or $h -ne 440) { throw "MENU_BNE_Logo is ${w}x${h}, not the 1920x440 this layout was read for" }
$square = [ErArchive]::ComposeLogo($rgba, $w, $h, 768, 400)
$target = Join-Path $Out 'er-bne-logo.dds'
[IO.File]::WriteAllBytes($target, [ErArchive]::Dds($square, 768, 768))
Write-Output ("er-bne-logo.dds written to {0} (768x768 RGBA8, SHA-256 {1}) from Elden Ring's MENU_BNE_Logo {2}x{3}" -f $Out, (Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash, $w, $h)
