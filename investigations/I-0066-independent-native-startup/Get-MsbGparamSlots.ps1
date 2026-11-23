[CmdletBinding()]
param([Parameter(Mandatory)][string]$MsbDir,      # recovered map:/MapStudio/mAA_BB_00_00.msb.dcx (Recover-GameData output, outside the repository)
      [Parameter(Mandatory)][string]$GparamTsvDir, # Get-GparamValues.ps1 TSVs of every drawparam GPARAM (param_drawparam_<name>.tsv)
      [Parameter(Mandatory)][string]$Out)          # TSV report
# I-0066 W10 auto exposure (W10-NATIVE-PASSES 11): which map GPARAM files a collision part can select, and whether any of them turns
# auto exposure on. The map step applies, per map, file slot 0 (mAA_BB_<base>) and slots 1..4 chosen by the player's collision part:
# FUN_140CE6040 reads FUN_140E17640(part, slot) = (signed char)[[part+0x10]+0xC8][+0x3C + slot - 1], 0 meaning none, and the fade
# FUN_140E17690 = float [+0x40]. In the MSB the part entry's +0xC8 is the offset of that block (the fade is -1 in every collision read:
# "negative means the default", as the accessor says). Slot value v names the file mAA_BB_<v*100 + base%100>. Read-only.
$ErrorActionPreference = 'Stop'
$souls = 'C:\Users\User\.codex\tools\SoulsFormats-1.0.1\lib\net462\SoulsFormats.dll'
if ((Get-FileHash -LiteralPath $souls -Algorithm SHA256).Hash -cne '19706BA6528982B83C6B6DD0DBE0ABDDF0E73CD51404F44A9D92CD27DEAF6735') { throw 'SoulsFormats mismatch' }
Add-Type -Path $souls
# Auto exposure per GPARAM file: Tone Map "AutoExposure" or "Enable AutoExposure" non-zero at any value id.
$auto = @{}; $present = @{}
foreach ($t in Get-ChildItem -LiteralPath $GparamTsvDir -Filter 'param_drawparam_*.tsv') {
    $name = $t.BaseName -replace '^param_drawparam_', ''; $present[$name] = $true
    $on = @(Select-String -LiteralPath $t.FullName -Pattern "`t(AutoExposure|Enable AutoExposure)`t" | Where-Object { ($_.Line -split "`t")[4] -ne '0' })
    if ($on.Count) { $auto[$name] = $true } }
$pattern = [Text.Encoding]::Unicode.GetBytes('PARTS_PARAM')
$seen = @{}
$lines = [Collections.Generic.List[string]]::new(); $lines.Add("map`tcollisions`tslot_values`tfiles_selectable`tauto_exposure_files_selectable")
foreach ($f in Get-ChildItem -LiteralPath $MsbDir -Filter '*.msb.dcx') {
    if ($f.Name -notmatch '(m\d\d_\d\d)_00_00\.msb\.dcx$') { continue }; $map = $Matches[1]
    if ($seen[$map]) { continue }; $seen[$map] = $true   # a map recovered from two archives (m47_00: base and DLC1) is counted once
    $d = [byte[]][SoulsFormats.DCX]::Decompress([IO.File]::ReadAllBytes($f.FullName))
    $name = -1; for ($p = 0; $p -le $d.Length - $pattern.Length; $p += 2) { $ok = $true; for ($j = 0; $j -lt $pattern.Length; $j++) { if ($d[$p + $j] -ne $pattern[$j]) { $ok = $false; break } }; if ($ok) { $name = $p; break } }
    $hdr = -1; for ($p = 8; $p -lt $d.Length - 8; $p += 8) { if ([BitConverter]::ToInt64($d, $p) -eq $name) { $hdr = $p - 8; break } }
    if ($name -lt 0 -or $hdr -lt 0) { throw "PARTS_PARAM not found in $($f.Name)" }
    $count = [BitConverter]::ToInt32($d, $hdr + 4); $values = @{}; $collisions = 0
    for ($k = 0; $k -lt $count - 1; $k++) {
        $e = [BitConverter]::ToInt64($d, $hdr + 16 + 8 * $k); if ([BitConverter]::ToInt32($d, $e + 8) -ne 5) { continue }   # type 5: collision
        $collisions++; $g = $e + [BitConverter]::ToInt64($d, $e + 0xC8)
        for ($s = 0; $s -lt 4; $s++) { $v = [int]$d[$g + 0x3C + $s]; if ($v -gt 127) { $v -= 256 }; if ($v -gt 0) { $values[[int]$v] = 1 + $values[[int]$v] } } }
    $files = @(); $autoFiles = @()
    foreach ($v in $values.Keys | Sort-Object) { foreach ($base in 0..99) {
        $file = '{0}_{1:D4}' -f $map, ($v * 100 + $base); if ($present[$file]) { $files += $file; if ($auto[$file]) { $autoFiles += $file } } } }
    $lines.Add("$map`t$collisions`t$((($values.Keys | Sort-Object) | ForEach-Object { "$($_)x$($values[$_])" }) -join ' ')`t$($files -join ' ')`t$($autoFiles -join ' ')")
}
$lines | Set-Content -LiteralPath $Out -Encoding utf8
"auto-exposure files: $(($auto.Keys | Sort-Object) -join ' ')"
$lines
